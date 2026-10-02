using UnityEngine;

namespace FPVSim.Core
{
    /// <summary>Free flight: respawn at the spawn point on request or when the drone leaves the world.</summary>
    [DisallowMultipleComponent]
    public sealed class FreeFlyMode : MonoBehaviour, IGameMode
    {
        [Tooltip("Respawn automatically below this world height (fell off the map).")]
        [SerializeField] private float killHeight = -50f;

        [Tooltip("Respawn automatically beyond this horizontal distance from the spawn point.")]
        [SerializeField] private float maxDistance = 2500f;

        private GameSession session;

        public string DisplayName => "Free Fly";

        public void Begin(GameSession owner)
        {
            session = owner;
            session.RespawnDroneAtSpawn();
        }

        public void Tick(float deltaTime)
        {
            if (session == null || session.Drone == null)
            {
                return;
            }

            Vector3 position = session.Drone.transform.position;
            Vector3 fromSpawn = position - session.SpawnPose.position;
            fromSpawn.y = 0f;
            if (position.y < killHeight || fromSpawn.magnitude > maxDistance)
            {
                session.RespawnDroneAtSpawn();
            }
        }

        public void OnRespawnRequested()
        {
            session?.RespawnDroneAtSpawn();
        }

        public void End()
        {
            session = null;
        }
    }
}
