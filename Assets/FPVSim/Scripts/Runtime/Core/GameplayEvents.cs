using System;
using FPVSim.Flight;
using UnityEngine;

namespace FPVSim.Core
{
    /// <summary>Which way the drone crossed a <see cref="PassThroughTrigger"/>.</summary>
    public enum PassDirection
    {
        /// <summary>Entered from the trigger's -Z side and left on its +Z side.</summary>
        Forward = 0,

        /// <summary>Entered from the +Z side and left on the -Z side.</summary>
        Backward = 1,
    }

    /// <summary>Data for "a drone flew through a trigger" (rings now; race gates and checkpoints later).</summary>
    public readonly struct TriggerPassInfo
    {
        public readonly PassThroughTrigger trigger;
        public readonly DroneController drone;
        public readonly PassDirection direction;

        /// <summary>Time.time at which the drone left the trigger volume.</summary>
        public readonly float time;

        public TriggerPassInfo(PassThroughTrigger trigger, DroneController drone, PassDirection direction, float time)
        {
            this.trigger = trigger;
            this.drone = drone;
            this.direction = direction;
            this.time = time;
        }
    }

    /// <summary>
    /// Global, decoupled gameplay event bus. Producers (drone, triggers) raise events without knowing who
    /// listens; future systems such as race gates, lap timers or a level manager just subscribe.
    /// Subscribers must unsubscribe in OnDisable/OnDestroy.
    /// </summary>
    public static class GameplayEvents
    {
        public static event Action<TriggerPassInfo> DronePassedTrigger;
        public static event Action<DroneController, DroneImpact> DroneCrashed;
        public static event Action<DroneController> DroneRespawned;
        public static event Action<DroneController, FlightMode> FlightModeChanged;

        public static void RaiseDronePassedTrigger(in TriggerPassInfo info)
        {
            DronePassedTrigger?.Invoke(info);
        }

        public static void RaiseDroneCrashed(DroneController drone, in DroneImpact impact)
        {
            DroneCrashed?.Invoke(drone, impact);
        }

        public static void RaiseDroneRespawned(DroneController drone)
        {
            DroneRespawned?.Invoke(drone);
        }

        public static void RaiseFlightModeChanged(DroneController drone, FlightMode mode)
        {
            FlightModeChanged?.Invoke(drone, mode);
        }

        /// <summary>
        /// Clears all subscribers when entering Play mode, so stale handlers can't survive when
        /// "Enter Play Mode Options" (domain reload disabled) is used.
        /// </summary>
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.SubsystemRegistration)]
        private static void ResetStatics()
        {
            DronePassedTrigger = null;
            DroneCrashed = null;
            DroneRespawned = null;
            FlightModeChanged = null;
        }
    }
}
