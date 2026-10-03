// Manages the drone's two cameras: the FPV camera fixed to the frame (uptilt + FOV) and a chase
// camera on a spring arm for debugging. Only one camera is active at a time; the pawn's view uses
// whichever camera component is active.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Settings/FPVSettingsTypes.h"
#include "FPVCameraRigComponent.generated.h"

class UCameraComponent;
class UPrimitiveComponent;
class USpringArmComponent;

UCLASS(ClassGroup = (FPV))
class FPVDRONE_API UFPVCameraRigComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPVCameraRigComponent();

	/**
	 * Called from the owning actor's constructor.
	 * @param InHiddenInFpv  Meshes hidden while flying FPV (the drone's own frame would clip the lens).
	 */
	void Setup(UCameraComponent* InFpvCamera, USpringArmComponent* InChaseArm, UCameraComponent* InChaseCamera,
		const TArray<UPrimitiveComponent*>& InHiddenInFpv);

	/** Apply uptilt, FOVs and chase distance. Does not change the active view. */
	void ApplySettings(const FFPVCameraSettings& NewSettings);

	void SetView(EFPVCameraView NewView);
	void ToggleView();
	EFPVCameraView GetView() const { return View; }

	float GetFpvUptiltDeg() const { return Settings.FpvUptiltDeg; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UCameraComponent> FpvCamera;

	UPROPERTY()
	TObjectPtr<USpringArmComponent> ChaseArm;

	UPROPERTY()
	TObjectPtr<UCameraComponent> ChaseCamera;

	UPROPERTY()
	TArray<TObjectPtr<UPrimitiveComponent>> HiddenInFpv;

	FFPVCameraSettings Settings;
	EFPVCameraView View = EFPVCameraView::FPV;
};
