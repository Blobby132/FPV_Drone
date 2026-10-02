using System;
using UnityEngine;
using UnityEngine.UI;

namespace FPVSim.UserInterface
{
    /// <summary>
    /// Numeric setting adjusted with left/right (D-pad, stick or arrow keys). Holding the direction repeats and
    /// accelerates, so wide ranges are quick to sweep while single presses stay precise.
    /// </summary>
    public sealed class SliderRow : MenuRow
    {
        private Func<float> getter;
        private Action<float> setter;
        private Func<float, string> formatter;
        private float min;
        private float max;
        private float step;
        private Image fill;
        private float lastAdjustTime = float.NegativeInfinity;
        private int repeatCount;

        public override string Hint => "Left / Right: adjust (hold to speed up)";

        public void Configure(Func<float> get, Action<float> set, float minimum, float maximum, float increment,
            Func<float, string> format)
        {
            getter = get;
            setter = set;
            min = minimum;
            max = maximum;
            step = Mathf.Max(increment, 1e-6f);
            formatter = format ?? (v => v.ToString("0.00"));
            Refresh();
        }

        protected override void OnBuilt()
        {
            // Thin bar along the bottom of the row showing where the value sits in its range.
            fill = UiFactory.CreateImage("Fill", transform, MenuStyle.FillBar);
            RectTransform rect = fill.rectTransform;
            rect.anchorMin = new Vector2(0f, 0f);
            rect.anchorMax = new Vector2(0f, 0f);
            rect.pivot = new Vector2(0f, 0f);
            rect.anchoredPosition = Vector2.zero;
            rect.sizeDelta = new Vector2(0f, 4f);
        }

        protected override bool OnHorizontal(int direction)
        {
            if (getter == null || setter == null)
            {
                return false;
            }

            // Repeated moves arrive every ~0.1 s while held; speed up after a while.
            float now = Time.unscaledTime;
            repeatCount = now - lastAdjustTime < 0.25f ? repeatCount + 1 : 0;
            lastAdjustTime = now;
            float multiplier = repeatCount > 20 ? 10f : repeatCount > 8 ? 4f : 1f;

            float value = getter() + direction * step * multiplier;
            value = Mathf.Round(value / step) * step; // stay on the step grid
            value = Mathf.Clamp(value, min, max);
            setter(value);
            NotifyChanged();
            Refresh();
            return true;
        }

        public override void Refresh()
        {
            if (getter == null || valueText == null)
            {
                return;
            }

            float value = getter();
            valueText.text = "<  " + formatter(value) + "  >";

            if (fill != null)
            {
                float t = max > min ? Mathf.Clamp01((value - min) / (max - min)) : 0f;
                float width = ((RectTransform)transform).rect.width;
                fill.rectTransform.sizeDelta = new Vector2(width * t, 4f);
            }
        }
    }
}
