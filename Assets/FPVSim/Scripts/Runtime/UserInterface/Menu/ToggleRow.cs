using System;
using UnityEngine.EventSystems;

namespace FPVSim.UserInterface
{
    /// <summary>On/off setting: Cross or left/right flips it.</summary>
    public sealed class ToggleRow : MenuRow
    {
        private Func<bool> getter;
        private Action<bool> setter;

        public override string Hint => "Cross or Left / Right: toggle";

        public void Configure(Func<bool> get, Action<bool> set)
        {
            getter = get;
            setter = set;
            Refresh();
        }

        public override void OnSubmit(BaseEventData eventData)
        {
            Flip();
        }

        protected override bool OnHorizontal(int direction)
        {
            Flip();
            return true;
        }

        private void Flip()
        {
            if (getter == null || setter == null)
            {
                return;
            }

            setter(!getter());
            NotifyChanged();
            Refresh();
        }

        public override void Refresh()
        {
            if (getter != null && valueText != null)
            {
                valueText.text = getter() ? "ON" : "OFF";
                valueText.color = getter() ? MenuStyle.Accent : MenuStyle.TextDim;
            }
        }
    }
}
