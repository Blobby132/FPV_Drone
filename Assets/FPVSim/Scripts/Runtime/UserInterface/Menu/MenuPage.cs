using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace FPVSim.UserInterface
{
    /// <summary>
    /// One screen of the pause menu: a vertical list of rows and info lines inside a clipped viewport. Rows are
    /// laid out manually (fixed heights), linked with explicit up/down navigation that wraps around, and the
    /// list scrolls to keep the selected row visible.
    /// </summary>
    public sealed class MenuPage
    {
        private readonly List<MenuRow> rows = new List<MenuRow>();
        private readonly List<Action> infoRefreshers = new List<Action>();
        private readonly RectTransform content;
        private float contentHeight;
        private MenuRow lastSelected;

        public MenuPage(PauseMenu menu, string title, RectTransform viewport)
        {
            Menu = menu;
            Title = title;
            Viewport = viewport;
            content = UiFactory.CreateRect("Page_" + title, viewport);
            content.anchorMin = new Vector2(0f, 1f);
            content.anchorMax = new Vector2(1f, 1f);
            content.pivot = new Vector2(0.5f, 1f);
            content.anchoredPosition = Vector2.zero;
            content.sizeDelta = Vector2.zero;
            content.gameObject.SetActive(false);
        }

        public PauseMenu Menu { get; }
        public string Title { get; }
        public RectTransform Viewport { get; }
        public IReadOnlyList<MenuRow> Rows => rows;
        public bool IsVisible => content.gameObject.activeSelf;

        // ------------------------------------------------------------------ building

        public SliderRow AddSlider(string label, Func<float> get, Action<float> set, float min, float max, float step,
            Func<float, string> format)
        {
            SliderRow row = AddRow<SliderRow>(label);
            row.Configure(get, set, min, max, step, format);
            return row;
        }

        public ToggleRow AddToggle(string label, Func<bool> get, Action<bool> set)
        {
            ToggleRow row = AddRow<ToggleRow>(label);
            row.Configure(get, set);
            return row;
        }

        public ChoiceRow AddChoice(string label, string[] options, Func<int> get, Action<int> set)
        {
            ChoiceRow row = AddRow<ChoiceRow>(label);
            row.Configure(options, get, set);
            return row;
        }

        public ButtonRow AddButton(string label, Action onSubmit, Func<string> value = null, string hint = null)
        {
            ButtonRow row = AddRow<ButtonRow>(label);
            row.Configure(onSubmit, value, hint);
            return row;
        }

        public RebindRow AddRebind(string label, UnityEngine.InputSystem.InputAction action)
        {
            RebindRow row = AddRow<RebindRow>(label);
            row.Configure(action);
            return row;
        }

        /// <summary>Non-selectable text line. A getter makes it refresh whenever settings change.</summary>
        public void AddInfo(string text, Func<string> dynamicText = null, bool heading = false)
        {
            int size = heading ? MenuStyle.RowTextSize : MenuStyle.InfoTextSize;
            Color color = heading ? MenuStyle.Accent : MenuStyle.TextDim;
            Text line = UiFactory.CreateText("Info", content, text, size, TextAnchor.MiddleLeft, color, false);
            Place(line.rectTransform, MenuStyle.InfoLineHeight, 18f);
            if (dynamicText != null)
            {
                Action refresh = () => line.text = dynamicText();
                infoRefreshers.Add(refresh);
                refresh();
            }
        }

        public void AddSpacer(float height = 14f)
        {
            contentHeight += height;
            content.sizeDelta = new Vector2(0f, contentHeight);
        }

        /// <summary>Links rows with explicit up/down navigation (wrapping). Call after adding all rows.</summary>
        public void FinishLayout()
        {
            for (int i = 0; i < rows.Count; i++)
            {
                MenuRow previous = rows[(i - 1 + rows.Count) % rows.Count];
                MenuRow next = rows[(i + 1) % rows.Count];
                rows[i].navigation = new Navigation
                {
                    mode = Navigation.Mode.Explicit,
                    selectOnUp = previous,
                    selectOnDown = next,
                };
            }
        }

        private T AddRow<T>(string label) where T : MenuRow
        {
            RectTransform rect = UiFactory.CreateRect("Row_" + label, content);
            Place(rect, MenuStyle.RowHeight, 0f);
            T row = rect.gameObject.AddComponent<T>();
            row.Build(this, label);
            rows.Add(row);
            return row;
        }

        private void Place(RectTransform rect, float height, float indent)
        {
            rect.anchorMin = new Vector2(0f, 1f);
            rect.anchorMax = new Vector2(1f, 1f);
            rect.pivot = new Vector2(0.5f, 1f);
            // Stretched horizontally: width = parent width - indent, shifted right by half the indent.
            rect.sizeDelta = new Vector2(-indent, height - 4f);
            rect.anchoredPosition = new Vector2(indent * 0.5f, -contentHeight);
            contentHeight += height;
            content.sizeDelta = new Vector2(0f, contentHeight);
        }

        // ------------------------------------------------------------------ runtime

        public void Show(MenuRow select = null)
        {
            content.gameObject.SetActive(true);
            content.anchoredPosition = Vector2.zero;
            RefreshAll();
            MenuRow target = select != null ? select : (lastSelected != null ? lastSelected : FirstRow);
            if (target != null && EventSystem.current != null)
            {
                EventSystem.current.SetSelectedGameObject(target.gameObject);
                EnsureVisible(target);
            }
        }

        public void Hide()
        {
            content.gameObject.SetActive(false);
        }

        public MenuRow FirstRow => rows.Count > 0 ? rows[0] : null;

        public void RefreshAll()
        {
            for (int i = 0; i < rows.Count; i++)
            {
                rows[i].Refresh();
            }

            for (int i = 0; i < infoRefreshers.Count; i++)
            {
                infoRefreshers[i]();
            }
        }

        public void RefreshInfo()
        {
            for (int i = 0; i < infoRefreshers.Count; i++)
            {
                infoRefreshers[i]();
            }
        }

        public void OnRowSelected(MenuRow row)
        {
            lastSelected = row;
            EnsureVisible(row);
            Menu.ShowHint(row.Hint);
        }

        /// <summary>Scrolls so the row is fully inside the viewport.</summary>
        public void EnsureVisible(MenuRow row)
        {
            var rect = (RectTransform)row.transform;
            float top = -rect.anchoredPosition.y;
            float bottom = top + rect.rect.height;
            float viewHeight = Viewport.rect.height;
            float scroll = content.anchoredPosition.y;
            const float margin = 12f;

            if (top - margin < scroll)
            {
                scroll = top - margin;
            }
            else if (bottom + margin > scroll + viewHeight)
            {
                scroll = bottom + margin - viewHeight;
            }

            scroll = Mathf.Clamp(scroll, 0f, Mathf.Max(0f, contentHeight - viewHeight));
            content.anchoredPosition = new Vector2(0f, scroll);
        }
    }
}
