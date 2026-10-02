using System;
using UnityEngine.EventSystems;

namespace FPVSim.UserInterface
{
    /// <summary>Pick one of several options with left/right (Cross cycles forward).</summary>
    public sealed class ChoiceRow : MenuRow
    {
        private string[] options = new string[0];
        private Func<int> getter;
        private Action<int> setter;

        public override string Hint => "Left / Right: change";

        public void Configure(string[] choices, Func<int> get, Action<int> set)
        {
            options = choices ?? new string[0];
            getter = get;
            setter = set;
            Refresh();
        }

        public override void OnSubmit(BaseEventData eventData)
        {
            Cycle(1);
        }

        protected override bool OnHorizontal(int direction)
        {
            Cycle(direction);
            return true;
        }

        private void Cycle(int direction)
        {
            if (getter == null || setter == null || options.Length == 0)
            {
                return;
            }

            int index = (getter() + direction + options.Length) % options.Length;
            setter(index);
            NotifyChanged();
            Refresh();
        }

        public override void Refresh()
        {
            if (getter == null || valueText == null || options.Length == 0)
            {
                return;
            }

            int index = Math.Max(0, Math.Min(options.Length - 1, getter()));
            valueText.text = "<  " + options[index] + "  >";
        }
    }
}
