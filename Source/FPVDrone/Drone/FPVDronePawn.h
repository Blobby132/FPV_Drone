// The drone: a single Chaos-simulated box body with purely visual meshes attached.
// The pawn does not read input itself; the player controller sends it FFPVPilotCommands.
// All flight forces are applied on the physics thread by FFPVDroneSimCallback.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Drone/FPVBatterySim.h"
#include "Flight/FPVFlightTypes.h"
#include "Physics/FPVDronePhysicsBridge.h"
#include "Settings/FPVSettingsTypes.h"
#include "FPVDronePawn.generated.h"

class UBoxComponent;
class UCameraComponent;
class UFPVCameraRigComponent;
class USpringArmComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class FFPVDroneSimCallback;

UCLASS()
class FPVDRONE_API AFPVDronePawn : public APawn
{
	GENERATED_BODY()

public:
	AFPVDronePawn();

	virtual void Tick(float DeltaSeconds) override;

	/** Latest processed stick input. Forwarded to the physics thread immediately. */
	void SetPilotCommand(const FFPVPilotCommand& Command);

	/** Apply new tuning (forwarded to the physics thread; mass/inertia applied to the body). */
	void ApplyTuning(const FFPVDroneTuning& NewTuning);
	const FFPVDroneTuning& GetTuning() const { return Tuning; }

	void SetFlightMode(EFPVFlightMode NewMode);
	void ToggleFlightMode();
	EFPVFlightMode GetFlightMode() const { return FlightMode; }

	/** Teleport to a transform with zero velocity and a fresh flight controller. */
	void ResetDrone(const FTransform& SpawnTransform);

	/** Latest data published by the physics thread. */
	FFPVFlightTelemetry GetTelemetry() const;

	/** World velocity in m/s. */
	FVector GetVelocityMps() const;

	/** The last command sent to the flight controller (RC throttle etc.). */
	const FFPVPilotCommand& GetLastCommand() const { return LastCommand; }

	UBoxComponent* GetBody() const { return Body; }
	UCameraComponent* GetFpvCamera() const { return FpvCamera; }
	UFPVCameraRigComponent* GetCameraRig() const { return CameraRig; }

	/** Apply FPV uptilt / FOVs / chase distance. */
	void ApplyCameraSettings(const FFPVCameraSettings& CameraSettings);
	void SetCameraView(EFPVCameraView View);
	void ToggleCameraView();
	EFPVCameraView GetCameraView() const;

	/** Cosmetic battery (OSD). */
	void ApplyBatterySettings(const FFPVBatterySettings& NewBatterySettings);
	const FFPVBatteryState& GetBatteryState() const { return Battery.GetState(); }

	/** Seconds since the last spawn/reset. */
	float GetFlightTimeSeconds() const { return FlightTimeSeconds; }

	/** Height of the drone's underside above the ground at the launch point (m). */
	float GetAltitudeMeters() const;

	/** Impacts softer than this speed change (m/s) are ignored (resting / sliding contact). */
	UPROPERTY(EditAnywhere, Category = "Drone|Impacts")
	float MinImpactSpeedMps = 0.75f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BuildVisuals(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh);
	UStaticMeshComponent* AddVisualPart(const FName& Name, UStaticMesh* Mesh, const FVector& LocationCm,
		const FRotator& Rotation, const FVector& SizeCm, const FLinearColor& Color);
	void ApplyVisualMaterials();
	void ApplyBodyMassProperties();
	void RegisterPhysicsCallback();
	void UnregisterPhysicsCallback();
	void PushBodyToPhysicsThread();
	void UpdatePropVisuals(float DeltaSeconds);
	void UpdateBattery(float DeltaSeconds);
	/** Finds the ground below a location (line trace) and stores it as the altitude reference. */
	void UpdateHomeGround(const FVector& FromLocation);

	/** Collision callback: measures how hard the hit was and broadcasts UFPVGameplayEvents::OnDroneImpact. */
	UFUNCTION()
	void HandleBodyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	/** Physics body (root). Collision box roughly covering the frame and props. */
	UPROPERTY(VisibleAnywhere, Category = "Drone")
	TObjectPtr<UBoxComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Drone")
	TObjectPtr<UCameraComponent> FpvCamera;

	/** Debug chase camera on a spring arm. */
	UPROPERTY(VisibleAnywhere, Category = "Drone")
	TObjectPtr<USpringArmComponent> ChaseArm;

	UPROPERTY(VisibleAnywhere, Category = "Drone")
	TObjectPtr<UCameraComponent> ChaseCamera;

	UPROPERTY(VisibleAnywhere, Category = "Drone")
	TObjectPtr<UFPVCameraRigComponent> CameraRig;

	/** All visual-only meshes (no collision). */
	UPROPERTY(VisibleAnywhere, Category = "Drone|Visuals")
	TArray<TObjectPtr<UStaticMeshComponent>> VisualParts;

	/** Base color per entry in VisualParts. */
	UPROPERTY()
	TArray<FLinearColor> VisualPartColors;

	/** Spinning prop meshes, indexed by motor (see FPVQuadLayout). */
	UPROPERTY(VisibleAnywhere, Category = "Drone|Visuals")
	TArray<TObjectPtr<UStaticMeshComponent>> PropParts;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** Current tuning (normally driven by the settings menu; editable here for quick experiments). */
	UPROPERTY(EditAnywhere, Category = "Drone|Tuning")
	FFPVDroneTuning Tuning;

	UPROPERTY(VisibleInstanceOnly, Category = "Drone")
	EFPVFlightMode FlightMode = EFPVFlightMode::Angle;

	FFPVPilotCommand LastCommand;
	FFPVBatterySim Battery;
	FFPVBatterySettings BatterySettings;
	float FlightTimeSeconds = 0.0f;
	/** World Z (cm) of the ground at the launch point. */
	double HomeGroundZCm = 0.0;
	/** Velocity at the end of the previous tick (cm/s), used to estimate impact speed. */
	FVector LastVelocityCmPerSec = FVector::ZeroVector;
	double LastImpactTimeSeconds = -1.0;
	float PropAnglesDeg[FPVQuad::NumMotors] = { 0.0f, 45.0f, 90.0f, 135.0f };

	TSharedPtr<FFPVDronePhysicsBridge, ESPMode::ThreadSafe> PhysicsBridge;

	/** Owned by the Chaos solver once registered; freed via UnregisterAndFreeSimCallbackObject_External. */
	FFPVDroneSimCallback* SimCallback = nullptr;
};
