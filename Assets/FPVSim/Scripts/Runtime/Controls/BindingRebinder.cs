using System;
using UnityEngine.InputSystem;

namespace FPVSim.Controls
{
    /// <summary>
    /// Interactive rebinding of a button action's gamepad binding. Sticks are excluded (they are reserved for
    /// flying); if the new button is already used by another rebindable action, the two bindings are swapped.
    /// </summary>
    public static class BindingRebinder
    {
        public const float TimeoutSeconds = 6f;

        /// <param name="actions">The asset that holds all rebindable actions (for duplicate swapping).</param>
        /// <param name="completed">Called with true when a new button was bound, false when cancelled / timed out.</param>
        public static InputActionRebindingExtensions.RebindingOperation Start(InputActionAsset actions, InputAction action,
            Action<bool> completed)
        {
            int bindingIndex = action.GetBindingIndex(group: FpvInputActions.GamepadGroup);
            if (bindingIndex < 0)
            {
                completed?.Invoke(false);
                return null;
            }

            string previousPath = action.bindings[bindingIndex].effectivePath;

            // Actions must be disabled while they are being rebound.
            bool wasEnabled = action.enabled;
            if (wasEnabled)
            {
                action.Disable();
            }

            InputActionRebindingExtensions.RebindingOperation operation = action
                .PerformInteractiveRebinding(bindingIndex)
                .WithControlsHavingToMatchPath("<Gamepad>")
                .WithControlsExcluding("<Gamepad>/leftStick")
                .WithControlsExcluding("<Gamepad>/rightStick")
                .WithExpectedControlType("Button")
                .WithCancelingThrough("<Keyboard>/escape")
                .WithTimeout(TimeoutSeconds)
                .OnMatchWaitForAnother(0.1f);

            operation
                .OnComplete(op =>
                {
                    op.Dispose();
                    SwapDuplicates(actions, action, bindingIndex, previousPath);
                    if (wasEnabled)
                    {
                        action.Enable();
                    }

                    completed?.Invoke(true);
                })
                .OnCancel(op =>
                {
                    op.Dispose();
                    if (wasEnabled)
                    {
                        action.Enable();
                    }

                    completed?.Invoke(false);
                });

            operation.Start();
            return operation;
        }

        /// <summary>Removes all binding overrides from the rebindable actions (back to the defaults).</summary>
        public static void ResetAll(InputActionAsset actions)
        {
            foreach (string actionName in FpvInputActions.RebindableActions)
            {
                InputAction action = actions.FindAction(actionName, false);
                if (action != null)
                {
                    action.RemoveAllBindingOverrides();
                }
            }
        }

        private static void SwapDuplicates(InputActionAsset actions, InputAction rebound, int bindingIndex, string previousPath)
        {
            if (actions == null)
            {
                return;
            }

            string newPath = rebound.bindings[bindingIndex].effectivePath;
            foreach (string actionName in FpvInputActions.RebindableActions)
            {
                InputAction other = actions.FindAction(actionName, false);
                if (other == null || other == rebound)
                {
                    continue;
                }

                int otherIndex = other.GetBindingIndex(group: FpvInputActions.GamepadGroup);
                if (otherIndex >= 0 && ControlNames.SameControl(other.bindings[otherIndex].effectivePath, newPath))
                {
                    other.ApplyBindingOverride(otherIndex, previousPath);
                }
            }
        }
    }
}
