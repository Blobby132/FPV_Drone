using System;
using System.Collections.Generic;
using FPVSim.Controls;
using FPVSim.Core;
using UnityEngine;

namespace FPVSim.Flight
{
    /// <summary>
    /// The quadcopter. Runs the whole flight loop in FixedUpdate, in a fixed order, so the simulation is
    /// deterministic for a given input sequence:
    ///
    ///   pilot command -> flight controller (setpoint + rate PID) -> mixer -> motor lag -> forces on the Rigidbody
    ///
    /// Thrust is applied per motor at its arm position (so roll/pitch torque comes from real lever arms), yaw
    /// comes from the props' reaction torque, and drag/angular drag are applied explicitly. Gravity comes from
    /// the Rigidbody.
    /// </summary>
    [RequireComponent(typeof(Rigidbody))]
    [DisallowMultipleComponent]
    public sealed class DroneController : MonoBehaviour
    {
        /// <summary>Unity clamps angular velocity to 7 rad/s by default; real quads flip at 1000+ deg/s.</summary>
        private const float MaxAngularVelocity = 60f;

        /// <summary>How long after the last collision the drone still counts as "in contact".</summary>
        private const float ContactMemory = 0.1f;

        /// <summary>Visual prop spin at full output, deg/s (purely cosmetic).</summary>
        private const float MaxVisualPropSpeed = 2400f;

        [Tooltip("Tuning used when nothing (GameSession) provides a runtime copy.")]
        [SerializeField] private DroneTuning defaultTuning;

        [Tooltip("Mount point of the FPV camera. Its forward (+Z) is the camera's direction at 0 deg uptilt.")]
        [SerializeField] private Transform cameraMount;

        [Tooltip("Propeller visuals in Betaflight motor order: rear right, front right, rear left, front left.")]
        [SerializeField] private Transform[] propellers = new Transform[QuadAirframe.MotorCount];

        private readonly FlightController flightController = new FlightController();
        private readonly QuadMixer mixer = new QuadMixer();
        private readonly MotorModel motors = new MotorModel();
        private readonly float[] motorCommands = new float[QuadAirframe.MotorCount];
        private readonly BatterySimulator battery = new BatterySimulator();

        private Rigidbody body;
        private DroneTuning tuning;
        private IPilotCommandSource commandSource;

        private FlightMode mode = FlightMode.Angle;
        private bool crashed;
        private float lastContactTime = float.NegativeInfinity;
        private Vector3 launchPosition;
        private float flightTime;
        private PilotCommand lastCommand;
        private FlightOutput lastOutput;

        private float appliedMass = -1f;
        private Vector3 appliedInertia;

        /// <summary>Raised when the flight mode changes.</summary>
        public event Action<FlightMode> FlightModeChanged;

        /// <summary>Raised for every collision with its impact speed (used for rumble).</summary>
        public event Action<DroneImpact> Impacted;

        /// <summary>Raised when an impact exceeds the crash threshold and the motors are cut.</summary>
        public event Action Crashed;

        /// <summary>Raised after <see cref="Respawn"/>.</summary>
        public event Action Respawned;

        public FlightMode Mode => mode;
        public bool IsCrashed => crashed;
        public DroneTuning Tuning => tuning;
        public Rigidbody Body => body;
        public Transform CameraMount => cameraMount != null ? cameraMount : transform;
        public IPilotCommandSource CommandSource => commandSource;

        /// <summary>Last pilot command (after input processing).</summary>
        public PilotCommand LastCommand => lastCommand;

        /// <summary>Collective throttle 0..1 after the throttle curve (what the OSD shows).</summary>
        public float Throttle => lastOutput.throttle;

        /// <summary>Rate setpoint of the last step, RPY in rad/s.</summary>
        public Vector3 SetpointRpy => lastOutput.setpointRpy;

        public IReadOnlyList<float> MotorOutputs => motors.Outputs;

        /// <summary>Cosmetic battery state for the OSD.</summary>
        public BatterySimulator Battery => battery;
        public float AverageMotorOutput => motors.AverageOutput;

        /// <summary>Speed in m/s.</summary>
        public float Speed => body != null ? body.linearVelocity.magnitude : 0f;

        /// <summary>Height above the last spawn point (like a baro altitude reset on arming), m.</summary>
        public float Altitude => transform.position.y - launchPosition.y;

        /// <summary>Seconds since the last respawn while the motors were running.</summary>
        public float FlightTime => flightTime;

        /// <summary>Body rates as an RPY vector, rad/s.</summary>
        public Vector3 BodyRatesRpy =>
            body != null ? BodyAxes.LocalToRpy(Quaternion.Inverse(body.rotation) * body.angularVelocity) : Vector3.zero;

        /// <summary>Assign the default tuning (used by the editor scene builder).</summary>
        public DroneTuning DefaultTuning
        {
            get => defaultTuning;
            set => defaultTuning = value;
        }

        private void Awake()
        {
            body = GetComponent<Rigidbody>();
            if (tuning == null)
            {
                tuning = defaultTuning;
            }

            ConfigureRigidbody();
            launchPosition = transform.position;
            battery.Reset(tuning);
        }

        /// <summary>Called by the GameSession with the runtime tuning copy and the pilot.</summary>
        public void Initialize(DroneTuning runtimeTuning, IPilotCommandSource source, FlightMode startMode)
        {
            if (body == null)
            {
                body = GetComponent<Rigidbody>();
            }

            tuning = runtimeTuning != null ? runtimeTuning : defaultTuning;
            commandSource = source;
            mode = startMode;
            appliedMass = -1f; // force mass properties to be re-applied
            ApplyMassProperties();
            flightController.Reset();
            battery.Reset(tuning);
        }

        /// <summary>Swap the pilot (e.g. a replay or AI later on).</summary>
        public void SetCommandSource(IPilotCommandSource source)
        {
            commandSource = source;
        }

        /// <summary>Assign visual references (used by the editor scene builder).</summary>
        public void SetVisuals(Transform mount, Transform[] props)
        {
            cameraMount = mount;
            propellers = props;
        }

        public void SetFlightMode(FlightMode newMode)
        {
            if (newMode == mode)
            {
                return;
            }

            mode = newMode;
            flightController.OnModeChanged();
            FlightModeChanged?.Invoke(mode);
            GameplayEvents.RaiseFlightModeChanged(this, mode);
        }

        /// <summary>Switches between Angle (self-level) and Acro (rate) mode.</summary>
        public void ToggleFlightMode()
        {
            SetFlightMode(mode == FlightMode.Angle ? FlightMode.Acro : FlightMode.Angle);
        }

        /// <summary>Puts the drone at <paramref name="pose"/>, at rest, motors armed, everything reset.</summary>
        public void Respawn(Pose pose)
        {
            if (body == null)
            {
                body = GetComponent<Rigidbody>();
            }

            body.linearVelocity = Vector3.zero;
            body.angularVelocity = Vector3.zero;
            body.position = pose.position;
            body.rotation = pose.rotation;
            transform.SetPositionAndRotation(pose.position, pose.rotation);

            launchPosition = pose.position;
            crashed = false;
            flightTime = 0f;
            lastContactTime = float.NegativeInfinity;
            lastCommand = PilotCommand.Idle;
            lastOutput = default;
            motors.Reset();
            flightController.Reset();
            battery.Reset(tuning);
            commandSource?.ResetState();

            Respawned?.Invoke();
            GameplayEvents.RaiseDroneRespawned(this);
        }

        private void FixedUpdate()
        {
            if (tuning == null)
            {
                return;
            }

            float dt = Time.fixedDeltaTime;
            ApplyMassProperties();
            var airframe = new QuadAirframe(tuning, Physics.gravity.magnitude);

            Quaternion attitude = body.rotation;
            var state = new FlightState
            {
                attitude = attitude,
                bodyRatesRpy = BodyAxes.LocalToRpy(Quaternion.Inverse(attitude) * body.angularVelocity),
                inContact = Time.time - lastContactTime < ContactMemory,
            };

            lastCommand = commandSource != null ? commandSource.ReadCommand(dt, mode) : PilotCommand.Idle;

            bool running = !crashed;
            if (running)
            {
                lastOutput = flightController.Step(lastCommand, state, mode, tuning, airframe, dt);
                mixer.Mix(lastOutput.throttle, lastOutput.axisCommandRpy, tuning.airmode, motorCommands);
                flightTime += dt;
            }
            else
            {
                lastOutput = default;
                Array.Clear(motorCommands, 0, motorCommands.Length);
            }

            motors.Step(motorCommands, tuning.motorIdle, tuning.motorSpinUpTime, tuning.motorSpinDownTime, dt, running);
            ApplyMotorForces(airframe, attitude);
            ApplyAerodynamics(attitude);
            battery.Step(motors.AverageOutput, tuning, dt);
        }

        private void ApplyMotorForces(in QuadAirframe airframe, Quaternion attitude)
        {
            Vector3 up = attitude * Vector3.up;
            Vector3 origin = body.position;
            float yawTorque = 0f;
            IReadOnlyList<float> outputs = motors.Outputs;

            for (int i = 0; i < QuadAirframe.MotorCount; i++)
            {
                float thrust = outputs[i] * airframe.maxThrustPerMotor;
                if (thrust <= 0f)
                {
                    continue;
                }

                Vector3 motorWorld = origin + attitude * airframe.MotorLocalPosition(i);
                body.AddForceAtPosition(up * thrust, motorWorld, ForceMode.Force);
                yawTorque += QuadAirframe.SpinDirection[i] * tuning.yawTorqueCoefficient * thrust;
            }

            if (yawTorque != 0f)
            {
                body.AddTorque(up * yawTorque, ForceMode.Force);
            }
        }

        private void ApplyAerodynamics(Quaternion attitude)
        {
            Vector3 v = Quaternion.Inverse(attitude) * body.linearVelocity;
            Vector3 q = tuning.quadraticDrag;
            float rotor = tuning.rotorDrag;

            // Quadratic drag per body axis plus linear rotor drag in the prop plane (x/z).
            Vector3 dragLocal = new Vector3(
                -(q.x * Mathf.Abs(v.x) * v.x + rotor * v.x),
                -(q.y * Mathf.Abs(v.y) * v.y),
                -(q.z * Mathf.Abs(v.z) * v.z + rotor * v.z)) * tuning.dragMultiplier;
            body.AddForce(attitude * dragLocal, ForceMode.Force);

            if (tuning.angularDrag > 0f)
            {
                body.AddTorque(-tuning.angularDrag * body.angularVelocity, ForceMode.Force);
            }
        }

        private void Update()
        {
            SpinPropellerVisuals(Time.deltaTime);
        }

        private void SpinPropellerVisuals(float deltaTime)
        {
            if (propellers == null)
            {
                return;
            }

            IReadOnlyList<float> outputs = motors.Outputs;
            int count = Mathf.Min(propellers.Length, QuadAirframe.MotorCount);
            for (int i = 0; i < count; i++)
            {
                Transform prop = propellers[i];
                if (prop == null)
                {
                    continue;
                }

                // Counter-clockwise from above is a negative rotation about Unity's +Y.
                float degrees = -QuadAirframe.SpinDirection[i] * outputs[i] * MaxVisualPropSpeed * deltaTime;
                prop.Rotate(0f, degrees, 0f, Space.Self);
            }
        }

        private void OnCollisionEnter(Collision collision)
        {
            lastContactTime = Time.time;

            Vector3 normal = Vector3.up;
            Vector3 point = transform.position;
            if (collision.contactCount > 0)
            {
                ContactPoint contact = collision.GetContact(0);
                normal = contact.normal;
                point = contact.point;
            }

            float impactSpeed = Mathf.Abs(Vector3.Dot(collision.relativeVelocity, normal));
            var impact = new DroneImpact(impactSpeed, point, normal, collision.collider);
            Impacted?.Invoke(impact);

            if (!crashed && tuning != null && tuning.disarmOnCrash && impactSpeed >= tuning.crashSpeed)
            {
                crashed = true;
                Crashed?.Invoke();
                GameplayEvents.RaiseDroneCrashed(this, impact);
            }
        }

        private void OnCollisionStay(Collision collision)
        {
            lastContactTime = Time.time;
        }

        private void ConfigureRigidbody()
        {
            body.useGravity = true;
            body.linearDamping = 0f;
            body.angularDamping = 0f;
            body.maxAngularVelocity = MaxAngularVelocity;
            body.interpolation = RigidbodyInterpolation.Interpolate;
            body.collisionDetectionMode = CollisionDetectionMode.ContinuousDynamic;
            body.sleepThreshold = 0f;
            body.centerOfMass = Vector3.zero;
            ApplyMassProperties();
        }

        /// <summary>Applies mass and inertia from the tuning when they changed (cheap check every step).</summary>
        private void ApplyMassProperties()
        {
            if (tuning == null || body == null)
            {
                return;
            }

            Vector3 inertia = tuning.InertiaTensorLocal;
            // ReSharper disable CompareOfFloatsByEqualityOperator
            bool changed = appliedMass != tuning.massKg || appliedInertia.x != inertia.x ||
                           appliedInertia.y != inertia.y || appliedInertia.z != inertia.z;
            // ReSharper restore CompareOfFloatsByEqualityOperator
            if (!changed)
            {
                return;
            }

            // Mass first: setting mass afterwards could rescale a manually set tensor.
            body.mass = tuning.massKg;
            body.centerOfMass = Vector3.zero;
            body.inertiaTensor = inertia;
            body.inertiaTensorRotation = Quaternion.identity;
            appliedMass = tuning.massKg;
            appliedInertia = inertia;
        }
    }
}
