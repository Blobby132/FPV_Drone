using UnityEngine;

namespace FPVSim.Core
{
    /// <summary>Where the drone (re)spawns. Its forward (+Z) is the drone's starting heading.</summary>
    [DisallowMultipleComponent]
    public sealed class SpawnPoint : MonoBehaviour
    {
        [Tooltip("Extra height above this transform so the drone's collider starts clear of the ground.")]
        [SerializeField] private float heightOffset = 0.15f;

        public Pose Pose
        {
            get
            {
                Vector3 flatForward = Vector3.ProjectOnPlane(transform.forward, Vector3.up);
                if (flatForward.sqrMagnitude < 1e-6f)
                {
                    flatForward = Vector3.forward;
                }

                return new Pose(
                    transform.position + Vector3.up * heightOffset,
                    Quaternion.LookRotation(flatForward.normalized, Vector3.up));
            }
        }

        private void OnDrawGizmos()
        {
            Gizmos.color = new Color(1f, 0.55f, 0.1f, 0.9f);
            Vector3 p = transform.position + Vector3.up * heightOffset;
            Gizmos.DrawWireCube(p, new Vector3(0.25f, 0.06f, 0.25f));
            Gizmos.DrawLine(p, p + transform.forward * 0.6f);
        }
    }
}
