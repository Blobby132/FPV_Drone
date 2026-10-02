using System;
using UnityEngine.EventSystems;

namespace FPVSim.UserInterface
{
    /// <summary>A menu entry that runs an action on Cross (open a page, save, quit, ...).</summary>
    public sealed class ButtonRow : MenuRow
    {
        private Action onSubmit;
        private Func<string> valueGetter;
        private string hint = "Cross: select";

        public override string Hint => hint;

        public void Configure(Action action, Func<string> value = null, string hintText = null)
        {
            onSubmit = action;
            valueGetter = value;
            if (!string.IsNullOrEmpty(hintText))
            {
                hint = hintText;
            }

            Refresh();
        }

        public override void OnSubmit(BaseEventData eventData)
        {
            onSubmit?.Invoke();
        }

        public override void Refresh()
        {
            if (valueText != null)
            {
                valueText.text = valueGetter != null ? valueGetter() : "";
            }
        }
    }
}
