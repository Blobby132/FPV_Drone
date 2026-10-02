using UnityEngine;

namespace FPVSim.UserInterface
{
    /// <summary>Layout constants and colors of the pause / settings menu.</summary>
    public static class MenuStyle
    {
        public const float PanelWidth = 960f;
        public const float PanelHeight = 820f;
        public const float RowHeight = 48f;
        public const float InfoLineHeight = 34f;
        public const int TitleSize = 40;
        public const int RowTextSize = 26;
        public const int InfoTextSize = 23;
        public const int HintTextSize = 22;

        public static readonly Color Backdrop = new Color(0f, 0f, 0f, 0.55f);
        public static readonly Color Panel = new Color(0.07f, 0.08f, 0.1f, 0.95f);
        public static readonly Color Accent = new Color(1f, 0.5f, 0.1f, 1f);
        public static readonly Color Text = new Color(0.95f, 0.95f, 0.95f, 1f);
        public static readonly Color TextDim = new Color(0.7f, 0.72f, 0.76f, 1f);
        public static readonly Color FillBar = new Color(1f, 0.5f, 0.1f, 0.35f);

        public static readonly Color RowNormal = new Color(1f, 1f, 1f, 0.04f);
        public static readonly Color RowHighlighted = new Color(1f, 1f, 1f, 0.12f);
        public static readonly Color RowSelected = new Color(1f, 0.5f, 0.1f, 0.55f);
        public static readonly Color RowPressed = new Color(1f, 0.65f, 0.3f, 0.75f);
    }
}
