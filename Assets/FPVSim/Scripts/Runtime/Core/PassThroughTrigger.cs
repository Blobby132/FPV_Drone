using System;
using System.Collections.Generic;
using FPVSim.Flight;
using UnityEngine;

namespace FPVSim.Core
{
    /// <summary>
    /// A trigger volume the drone can fly through (rings today; race gates / checkpoints later). It raises
    /// <see cref="GameplayEvents.DronePassedTrigger"/> and its own <see cref="Passed"/> event when a drone
    /// enters on one side of the local XY plane and leaves on the other, so brushing the edge doesn't count.
    ///
    /// Put it on a child object with a trigger collider spanning the opening; the trigger's +Z axis is the
    /// "forward" fly-through direction.
    /// </summary>
    [RequireComponent(typeof(Collider))]
    [DisallowMultipleComponent]
    public sealed class PassThroughTrigger : MonoBehaviour
    {
        [Tooltip("Identifier for game modes (e.g. a gate number). Free-form.")]
        [SerializeField] private string triggerId = "";

        [Tooltip("Ordering hint for game modes that need a sequence (race gates).")]
        [SerializeField] private int order;

        /// <summary>Drone -> side of the plane it entered from (+1 = +Z side, -1 = -Z side).</summary>
        private readonly Dictionary<DroneController, float> entrySides = new Dictionary<DroneController, float>();

        public event Action<TriggerPassInfo> Passed;

        public string TriggerId
        {
            get => triggerId;
            set => triggerId = value;
        }

        public int Order
        {
            get => order;
            set => order = value;
        }

        private void Reset()
        {
            GetComponent<Collider>().isTrigger = true;
        }

        private void Awake()
        {
            Collider ownCollider = GetComponent<Collider>();
            if (!ownCollider.isTrigger)
            {
                Debug.LogWarning($"[FPV Sim] PassThroughTrigger '{name}' collider was not a trigger; fixing.", this);
                ownCollider.isTrigger = true;
            }
        }

        private void OnDisable()
        {
            entrySides.Clear();
        }

        private void OnTriggerEnter(Collider other)
        {
            DroneController drone = FindDrone(other);
            if (drone == null || entrySides.ContainsKey(drone))
            {
                return;
            }

            entrySides.Add(drone, SideOf(drone));
        }

        private void OnTriggerExit(Collider other)
        {
            DroneController drone = FindDrone(other);
            if (drone == null || !entrySides.TryGetValue(drone, out float entrySide))
            {
                return;
            }

            entrySides.Remove(drone);
            float exitSide = SideOf(drone);
            if (Mathf.Approximately(Mathf.Sign(entrySide), Mathf.Sign(exitSide)))
            {
                return; // left the way it came in
            }

            var info = new TriggerPassInfo(this, drone,
                exitSide > 0f ? PassDirection.Forward : PassDirection.Backward, Time.time);
            Passed?.Invoke(info);
            GameplayEvents.RaiseDronePassedTrigger(info);
        }

        private float SideOf(DroneController drone)
        {
            float z = transform.InverseTransformPoint(drone.Body != null ? drone.Body.position : drone.transform.position).z;
            return z >= 0f ? 1f : -1f;
        }

        private static DroneController FindDrone(Collider other)
        {
            Rigidbody attached = other.attachedRigidbody;
            return attached != null ? attached.GetComponent<DroneController>() : other.GetComponentInParent<DroneController>();
        }
    }
}
