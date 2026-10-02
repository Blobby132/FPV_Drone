using FPVSim.Controls;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;

namespace FPVSim.UserInterface
{
    /// <summary>Shows an action's gamepad button; Cross starts interactive rebinding.</summary>
    public sealed class RebindRow : MenuRow
    {
        private string waitingMessage;

        public InputAction Action { get; private set; }

        public override string Hint => "Cross: rebind, then press the new button (wait to cancel)";

        public void Configure(InputAction action)
        {
            Action = action;
            Refresh();
        }

        public override void OnSubmit(BaseEventData eventData)
        {
            Page?.Menu.StartRebind(this);
        }

        /// <summary>Shows a status message instead of the binding (null = show the binding again).</summary>
        public void SetWaiting(string message)
        {
            waitingMessage = message;
            Refresh();
        }

        public override void Refresh()
        {
            if (valueText == null)
            {
                return;
            }

            if (!string.IsNullOrEmpty(waitingMessage))
            {
                valueText.text = waitingMessage;
                valueText.color = MenuStyle.Accent;
                return;
            }

            valueText.text = ControlNames.ForAction(Action);
            valueText.color = MenuStyle.Text;
        }
    }
}
