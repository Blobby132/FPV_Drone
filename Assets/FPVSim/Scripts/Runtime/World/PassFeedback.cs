using FPVSim.Core;
using UnityEngine;

namespace FPVSim.World
{
    /// <summary>
    /// Visual confirmation for a <see cref="PassThroughTrigger"/>: briefly brightens the given renderers when the
    /// drone flies through. Uses a MaterialPropertyBlock so the shared material is untouched. Also serves as a
    /// minimal example of reacting to pass events (a race gate would do the same with lap logic).
    /// </summary>
    [DisallowMultipleComponent]
    public sealed class PassFeedback : MonoBehaviour
    {
        private static readonly int EmissionColorId = Shader.PropertyToID("_EmissionColor");
        private static readonly int BaseColorId = Shader.PropertyToID("_BaseColor");

        [SerializeField] private PassThroughTrigger trigger;
        [SerializeField] private Renderer[] renderers = new Renderer[0];
        [SerializeField] private Color flashColor = new Color(0.4f, 1f, 0.5f);
        [SerializeField] private float flashIntensity = 6f;
        [SerializeField] private float flashDuration = 0.6f;

        private MaterialPropertyBlock block;
        private float flashUntil = float.NegativeInfinity;
        private bool flashing;

        /// <summary>Assign references (used by the editor scene builder).</summary>
        public void Configure(PassThroughTrigger passTrigger, Renderer[] targets)
        {
            trigger = passTrigger;
            renderers = targets;
        }

        private void OnEnable()
        {
            if (trigger != null)
            {
                trigger.Passed += OnPassed;
            }
        }

        private void OnDisable()
        {
            if (trigger != null)
            {
                trigger.Passed -= OnPassed;
            }

            ClearFlash();
        }

        private void OnPassed(TriggerPassInfo info)
        {
            flashUntil = Time.time + flashDuration;
            flashing = true;
            Apply(1f);
        }

        private void Update()
        {
            if (!flashing)
            {
                return;
            }

            float remaining = flashUntil - Time.time;
            if (remaining <= 0f)
            {
                ClearFlash();
                return;
            }

            Apply(remaining / flashDuration);
        }

        private void Apply(float strength)
        {
            if (block == null)
            {
                block = new MaterialPropertyBlock();
            }

            Color emission = flashColor * (flashIntensity * strength);
            for (int i = 0; i < renderers.Length; i++)
            {
                if (renderers[i] == null)
                {
                    continue;
                }

                renderers[i].GetPropertyBlock(block);
                block.SetColor(EmissionColorId, emission);
                block.SetColor(BaseColorId, Color.Lerp(Color.white, flashColor, 0.5f));
                renderers[i].SetPropertyBlock(block);
            }
        }

        private void ClearFlash()
        {
            flashing = false;
            for (int i = 0; i < renderers.Length; i++)
            {
                if (renderers[i] != null)
                {
                    renderers[i].SetPropertyBlock(null);
                }
            }
        }
    }
}
