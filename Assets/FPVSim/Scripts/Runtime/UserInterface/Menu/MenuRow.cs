using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace FPVSim.UserInterface
{
    /// <summary>
    /// Base of every selectable menu line: a label on the left, a value on the right, a highlight background.
    /// Up/down navigation is explicit (set by <see cref="MenuPage"/>); left/right and Cross (submit) are
    /// handled by the subclasses; Circle (cancel) goes back.
    /// </summary>
    public abstract class MenuRow : Selectable, ISubmitHandler, ICancelHandler, IPointerClickHandler
    {
        protected Text labelText;
        protected Text valueText;
        protected Image background;

        public MenuPage Page { get; private set; }

        /// <summary>Help text shown at the bottom of the menu while this row is selected.</summary>
        public virtual string Hint => "Cross: select";

        /// <summary>Creates the visuals. Called once right after AddComponent.</summary>
        public void Build(MenuPage page, string label)
        {
            Page = page;

            var rect = (RectTransform)transform;
            background = gameObject.AddComponent<Image>();
            background.color = Color.white;
            background.raycastTarget = true;
            targetGraphic = background;
            transition = Transition.ColorTint;

            ColorBlock block = ColorBlock.defaultColorBlock;
            block.normalColor = MenuStyle.RowNormal;
            block.highlightedColor = MenuStyle.RowHighlighted;
            block.selectedColor = MenuStyle.RowSelected;
            block.pressedColor = MenuStyle.RowPressed;
            block.disabledColor = new Color(1f, 1f, 1f, 0.02f);
            block.colorMultiplier = 1f;
            block.fadeDuration = 0.05f;
            colors = block;

            labelText = UiFactory.CreateText("Label", rect, label, MenuStyle.RowTextSize, TextAnchor.MiddleLeft, MenuStyle.Text, false);
            RectTransform labelRect = labelText.rectTransform;
            labelRect.anchorMin = new Vector2(0f, 0f);
            labelRect.anchorMax = new Vector2(0.58f, 1f);
            labelRect.offsetMin = new Vector2(18f, 0f);
            labelRect.offsetMax = Vector2.zero;
            labelText.horizontalOverflow = HorizontalWrapMode.Wrap;

            valueText = UiFactory.CreateText("Value", rect, "", MenuStyle.RowTextSize, TextAnchor.MiddleRight, MenuStyle.Text, false);
            RectTransform valueRect = valueText.rectTransform;
            valueRect.anchorMin = new Vector2(0.58f, 0f);
            valueRect.anchorMax = new Vector2(1f, 1f);
            valueRect.offsetMin = Vector2.zero;
            valueRect.offsetMax = new Vector2(-18f, 0f);

            OnBuilt();
        }

        /// <summary>Extra visuals for subclasses.</summary>
        protected virtual void OnBuilt()
        {
        }

        /// <summary>Re-reads the bound value and updates the text.</summary>
        public abstract void Refresh();

        public virtual void OnSubmit(BaseEventData eventData)
        {
        }

        public void OnCancel(BaseEventData eventData)
        {
            Page?.Menu.Back();
        }

        public virtual void OnPointerClick(PointerEventData eventData)
        {
            if (eventData.button == PointerEventData.InputButton.Left)
            {
                Select();
                OnSubmit(eventData);
            }
        }

        public override void OnSelect(BaseEventData eventData)
        {
            base.OnSelect(eventData);
            Page?.OnRowSelected(this);
        }

        /// <summary>Left/right adjustments; return true if handled.</summary>
        protected virtual bool OnHorizontal(int direction)
        {
            return false;
        }

        public override void OnMove(AxisEventData eventData)
        {
            if (eventData.moveDir == MoveDirection.Left || eventData.moveDir == MoveDirection.Right)
            {
                int direction = eventData.moveDir == MoveDirection.Right ? 1 : -1;
                if (OnHorizontal(direction))
                {
                    eventData.Use();
                    return;
                }
            }

            base.OnMove(eventData);
        }

        /// <summary>Tell the menu a setting changed (marks settings dirty, refreshes info lines).</summary>
        protected void NotifyChanged()
        {
            Page?.Menu.NotifySettingChanged();
        }
    }
}
