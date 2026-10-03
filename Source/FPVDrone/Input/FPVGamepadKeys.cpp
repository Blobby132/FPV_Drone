#include "Input/FPVGamepadKeys.h"

const TArray<FKey>& FPVGamepadKeys::GetButtons()
{
	// Function-local static: built on first use, after the engine has registered EKeys.
	static const TArray<FKey> Buttons =
	{
		EKeys::Gamepad_FaceButton_Bottom,
		EKeys::Gamepad_FaceButton_Right,
		EKeys::Gamepad_FaceButton_Left,
		EKeys::Gamepad_FaceButton_Top,
		EKeys::Gamepad_DPad_Up,
		EKeys::Gamepad_DPad_Down,
		EKeys::Gamepad_DPad_Left,
		EKeys::Gamepad_DPad_Right,
		EKeys::Gamepad_LeftShoulder,
		EKeys::Gamepad_RightShoulder,
		EKeys::Gamepad_LeftTrigger,
		EKeys::Gamepad_RightTrigger,
		EKeys::Gamepad_LeftThumbstick,
		EKeys::Gamepad_RightThumbstick,
		EKeys::Gamepad_Special_Left,
		EKeys::Gamepad_Special_Right,
	};
	return Buttons;
}

const TArray<FKey>& FPVGamepadKeys::GetAxes()
{
	static const TArray<FKey> Axes =
	{
		EKeys::Gamepad_LeftX,
		EKeys::Gamepad_LeftY,
		EKeys::Gamepad_RightX,
		EKeys::Gamepad_RightY,
		EKeys::Gamepad_LeftTriggerAxis,
		EKeys::Gamepad_RightTriggerAxis,
	};
	return Axes;
}

FString FPVGamepadKeys::GetDisplayName(const FKey& Key)
{
	// Unreal's generic gamepad names follow the Xbox layout; map them to the DualSense labels.
	if (Key == EKeys::Gamepad_FaceButton_Bottom)	{ return TEXT("Cross"); }
	if (Key == EKeys::Gamepad_FaceButton_Right)		{ return TEXT("Circle"); }
	if (Key == EKeys::Gamepad_FaceButton_Left)		{ return TEXT("Square"); }
	if (Key == EKeys::Gamepad_FaceButton_Top)		{ return TEXT("Triangle"); }
	if (Key == EKeys::Gamepad_DPad_Up)				{ return TEXT("D-pad Up"); }
	if (Key == EKeys::Gamepad_DPad_Down)			{ return TEXT("D-pad Down"); }
	if (Key == EKeys::Gamepad_DPad_Left)			{ return TEXT("D-pad Left"); }
	if (Key == EKeys::Gamepad_DPad_Right)			{ return TEXT("D-pad Right"); }
	if (Key == EKeys::Gamepad_LeftShoulder)			{ return TEXT("L1"); }
	if (Key == EKeys::Gamepad_RightShoulder)		{ return TEXT("R1"); }
	if (Key == EKeys::Gamepad_LeftTrigger)			{ return TEXT("L2"); }
	if (Key == EKeys::Gamepad_RightTrigger)			{ return TEXT("R2"); }
	if (Key == EKeys::Gamepad_LeftThumbstick)		{ return TEXT("L3"); }
	if (Key == EKeys::Gamepad_RightThumbstick)		{ return TEXT("R3"); }
	if (Key == EKeys::Gamepad_Special_Left)			{ return TEXT("Create"); }
	if (Key == EKeys::Gamepad_Special_Right)		{ return TEXT("Options"); }
	if (Key == EKeys::Gamepad_LeftX)				{ return TEXT("Left X"); }
	if (Key == EKeys::Gamepad_LeftY)				{ return TEXT("Left Y"); }
	if (Key == EKeys::Gamepad_RightX)				{ return TEXT("Right X"); }
	if (Key == EKeys::Gamepad_RightY)				{ return TEXT("Right Y"); }
	if (Key == EKeys::Gamepad_LeftTriggerAxis)		{ return TEXT("L2 axis"); }
	if (Key == EKeys::Gamepad_RightTriggerAxis)		{ return TEXT("R2 axis"); }

	if (!Key.IsValid())
	{
		return TEXT("(none)");
	}
	return Key.GetDisplayName().ToString();
}

FString FPVGamepadKeys::GetDisplayName(FName KeyName)
{
	return GetDisplayName(FKey(KeyName));
}
