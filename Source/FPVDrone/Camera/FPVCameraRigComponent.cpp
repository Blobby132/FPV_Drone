#include "Camera/FPVCameraRigComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/SpringArmComponent.h"

UFPVCameraRigComponent::UFPVCameraRigComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFPVCameraRigComponent::Setup(UCameraComponent* InFpvCamera, USpringArmComponent* InChaseArm,
	UCameraComponent* InChaseCamera, const TArray<UPrimitiveComponent*>& InHiddenInFpv)
{
	FpvCamera = InFpvCamera;
	ChaseArm = InChaseArm;
	ChaseCamera = InChaseCamera;
	HiddenInFpv.Reset();
	for (UPrimitiveComponent* Component : InHiddenInFpv)
	{
		HiddenInFpv.Add(Component);
	}
}

void UFPVCameraRigComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplySettings(Settings);
	SetView(View);
}

void UFPVCameraRigComponent::ApplySettings(const FFPVCameraSettings& NewSettings)
{
	Settings = NewSettings;

	if (FpvCamera != nullptr)
	{
		// Positive pitch tilts the camera up, like FPV camera uptilt.
		FpvCamera->SetRelativeRotation(FRotator(Settings.FpvUptiltDeg, 0.0f, 0.0f));
		FpvCamera->SetFieldOfView(Settings.FpvFovDeg);
	}
	if (ChaseArm != nullptr)
	{
		ChaseArm->TargetArmLength = Settings.ChaseDistanceCm;
	}
	if (ChaseCamera != nullptr)
	{
		ChaseCamera->SetFieldOfView(Settings.ChaseFovDeg);
	}
}

void UFPVCameraRigComponent::SetView(EFPVCameraView NewView)
{
	View = NewView;
	const bool bFpv = (View == EFPVCameraView::FPV);

	// AActor::CalcCamera uses the first *active* camera component, so exactly one is active.
	if (FpvCamera != nullptr)
	{
		FpvCamera->SetActive(bFpv);
	}
	if (ChaseCamera != nullptr)
	{
		ChaseCamera->SetActive(!bFpv);
	}

	for (UPrimitiveComponent* Component : HiddenInFpv)
	{
		if (Component != nullptr)
		{
			Component->SetOwnerNoSee(bFpv);
		}
	}
}

void UFPVCameraRigComponent::ToggleView()
{
	SetView(View == EFPVCameraView::FPV ? EFPVCameraView::Chase : EFPVCameraView::FPV);
}
