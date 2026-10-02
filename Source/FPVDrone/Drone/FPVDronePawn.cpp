#include "Drone/FPVDronePawn.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodyInstance.h"
#include "UObject/ConstructorHelpers.h"

// Chaos (solver access for registering the sim callback).
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "PBDRigidsSolver.h"

#include "Flight/FPVQuadMixer.h"
#include "Flight/FPVUnits.h"
#include "Physics/FPVDroneSimCallback.h"
#include "FPVDrone.h"

namespace FPVDroneGeometry
{
	/** Collision box half extents (cm): roughly frame + props (24 x 24 x 6.4 cm). */
	const FVector BodyHalfExtentCm(12.0, 12.0, 3.2);

	/** FPV camera position on the frame (cm). */
	const FVector FpvCameraLocationCm(7.5, 0.0, -0.9);

	/** Visual prop spin rate at full motor output (deg/s). Kept low enough to read as spinning at 60 fps. */
	constexpr float PropVisualMaxDegPerSec = 2400.0f;

	const FLinearColor CarbonColor(0.02f, 0.02f, 0.025f);
	const FLinearColor MotorColor(0.55f, 0.55f, 0.6f);
	const FLinearColor FrontPropColor(1.0f, 0.35f, 0.05f);
	const FLinearColor RearPropColor(0.06f, 0.06f, 0.06f);
	const FLinearColor BatteryColor(0.1f, 0.2f, 0.55f);
	const FLinearColor CameraColor(0.01f, 0.01f, 0.01f);
	const FLinearColor LensColor(0.1f, 0.3f, 0.5f);
	const FLinearColor AntennaColor(0.8f, 0.1f, 0.1f);
}

AFPVDronePawn::AFPVDronePawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	// Physics drives the rotation; never let controller rotation fight it.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	AutoPossessAI = EAutoPossessAI::Disabled;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// --- Physics body ------------------------------------------------------------------------
	Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetBoxExtent(FPVDroneGeometry::BodyHalfExtentCm);
	Body->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	Body->SetSimulatePhysics(true);
	Body->SetEnableGravity(true);
	// Drag is modelled explicitly in FPVAirframeModel; disable the engine's damping.
	Body->SetLinearDamping(0.0f);
	Body->SetAngularDamping(0.0f);
	// Hit events (crash detection / rumble) and overlap events (gates, triggers).
	Body->SetNotifyRigidBodyCollision(true);
	Body->SetGenerateOverlapEvents(true);
	// Continuous collision: the drone easily exceeds 40 m/s.
	Body->BodyInstance.bUseCCD = true;
	Body->SetMassOverrideInKg(NAME_None, Tuning.Airframe.MassKg, true);
	Body->SetHiddenInGame(true);

	// --- FPV camera --------------------------------------------------------------------------
	FpvCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FpvCamera"));
	FpvCamera->SetupAttachment(Body);
	FpvCamera->SetRelativeLocation(FPVDroneGeometry::FpvCameraLocationCm);
	FpvCamera->bUsePawnControlRotation = false;
	FpvCamera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	FpvCamera->PostProcessSettings.MotionBlurAmount = 0.0f;
	ApplyCameraSettings(FFPVCameraSettings());

	// --- Visuals (engine basic shapes only) --------------------------------------------------
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BaseMaterialFinder.Object;
	BuildVisuals(CubeMeshFinder.Object, CylinderMeshFinder.Object);
}

UStaticMeshComponent* AFPVDronePawn::AddVisualPart(const FName& Name, UStaticMesh* Mesh, const FVector& LocationCm,
	const FRotator& Rotation, const FVector& SizeCm, const FLinearColor& Color)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetupAttachment(Body);
	Part->SetStaticMesh(Mesh);
	Part->SetRelativeLocation(LocationCm);
	Part->SetRelativeRotation(Rotation);
	// Engine basic shapes are 100 cm across.
	Part->SetRelativeScale3D(SizeCm / 100.0);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetGenerateOverlapEvents(false);
	Part->SetCanEverAffectNavigation(false);
	VisualParts.Add(Part);
	VisualPartColors.Add(Color);
	return Part;
}

void AFPVDronePawn::BuildVisuals(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh)
{
	using namespace FPVDroneGeometry;

	const float ArmLengthCm = Tuning.Airframe.ArmLengthCm;
	const float MotorOffsetCm = ArmLengthCm * 0.70710678f;

	AddVisualPart(TEXT("BottomPlate"), CubeMesh, FVector(0.0, 0.0, -2.2), FRotator::ZeroRotator, FVector(14.0, 6.5, 0.4), CarbonColor);
	AddVisualPart(TEXT("TopPlate"), CubeMesh, FVector(0.0, 0.0, 0.4), FRotator::ZeroRotator, FVector(11.0, 5.0, 0.3), CarbonColor);
	AddVisualPart(TEXT("Battery"), CubeMesh, FVector(0.0, 0.0, 2.2), FRotator::ZeroRotator, FVector(7.5, 3.6, 3.3), BatteryColor);
	AddVisualPart(TEXT("CameraBody"), CubeMesh, FVector(5.5, 0.0, -0.9), FRotator::ZeroRotator, FVector(2.0, 2.2, 2.2), CameraColor);
	AddVisualPart(TEXT("CameraLens"), CylinderMesh, FVector(6.8, 0.0, -0.9), FRotator(90.0, 0.0, 0.0), FVector(1.6, 1.6, 0.8), LensColor);
	AddVisualPart(TEXT("Antenna"), CylinderMesh, FVector(-6.5, 0.0, 1.8), FRotator(-25.0, 0.0, 0.0), FVector(0.4, 0.4, 5.0), AntennaColor);

	PropParts.SetNum(FPVQuad::NumMotors);
	for (int32 Index = 0; Index < FPVQuad::NumMotors; ++Index)
	{
		const FVector Direction(FPVQuadLayout::PosX[Index], FPVQuadLayout::PosY[Index], 0.0);
		const FVector Motor = Direction * MotorOffsetCm;
		const float ArmYaw = FMath::RadiansToDegrees(FMath::Atan2(FPVQuadLayout::PosY[Index], FPVQuadLayout::PosX[Index]));
		const bool bFront = FPVQuadLayout::PosX[Index] > 0.0f;

		// Arm from the center to the motor.
		AddVisualPart(*FString::Printf(TEXT("Arm%d"), Index), CubeMesh,
			Direction.GetSafeNormal() * (ArmLengthCm * 0.5f) + FVector(0.0, 0.0, -2.0),
			FRotator(0.0, ArmYaw, 0.0), FVector(ArmLengthCm + 1.5f, 2.2, 0.6), CarbonColor);

		// Motor bell.
		AddVisualPart(*FString::Printf(TEXT("Motor%d"), Index), CylinderMesh,
			Motor + FVector(0.0, 0.0, -0.9), FRotator::ZeroRotator, FVector(2.8, 2.8, 1.6), MotorColor);

		// Two-blade 5" prop (a thin bar). Front props are orange, rear black, like real quads.
		PropParts[Index] = AddVisualPart(*FString::Printf(TEXT("Prop%d"), Index), CubeMesh,
			Motor + FVector(0.0, 0.0, 0.1), FRotator(0.0, PropAnglesDeg[Index], 0.0), FVector(12.7, 1.3, 0.25),
			bFront ? FrontPropColor : RearPropColor);
	}

	// Don't draw the drone's own meshes into the FPV view (they would clip the near plane).
	for (UStaticMeshComponent* Part : VisualParts)
	{
		Part->SetOwnerNoSee(true);
	}
}

void AFPVDronePawn::BeginPlay()
{
	Super::BeginPlay();

	ApplyVisualMaterials();

	PhysicsBridge = MakeShared<FFPVDronePhysicsBridge, ESPMode::ThreadSafe>();
	ApplyTuning(Tuning);
	SetFlightMode(FlightMode);
	Body->SetPhysicsMaxAngularVelocityInDegrees(7200.0f);

	RegisterPhysicsCallback();
	PushBodyToPhysicsThread();
}

void AFPVDronePawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unregister before the body is destroyed so the physics thread never sees a stale body.
	UnregisterPhysicsCallback();
	Super::EndPlay(EndPlayReason);
}

void AFPVDronePawn::ApplyVisualMaterials()
{
	if (BaseMaterial == nullptr)
	{
		UE_LOG(LogFPVDrone, Warning, TEXT("Drone: /Engine/BasicShapes/BasicShapeMaterial not found; using default material."));
		return;
	}

	for (int32 Index = 0; Index < VisualParts.Num(); ++Index)
	{
		UStaticMeshComponent* Part = VisualParts[Index];
		if (Part == nullptr)
		{
			continue;
		}
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		const FLinearColor Color = VisualPartColors.IsValidIndex(Index) ? VisualPartColors[Index] : FLinearColor::Gray;
		// NEEDS VERIFICATION: BasicShapeMaterial exposes a vector parameter named "Color".
		// Setting a parameter that doesn't exist is harmless.
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
		Part->SetMaterial(0, Material);
	}
}

void AFPVDronePawn::ApplyBodyMassProperties()
{
	const FFPVAirframeSettings& Airframe = Tuning.Airframe;
	Body->SetMassOverrideInKg(NAME_None, Airframe.MassKg, true);

	FBodyInstance* BodyInstance = Body->GetBodyInstance();
	if (BodyInstance == nullptr)
	{
		return;
	}

	// Chaos derives inertia from the collision box. Scale it so the body has exactly the
	// configured roll/pitch/yaw inertia (solid box: I_xx = m * (y^2 + z^2) / 12, sizes in m).
	const FVector SizeM = Body->GetUnscaledBoxExtent() * 2.0 * static_cast<double>(FPVUnits::MetersPerCm);
	const double Mass = FMath::Max(Airframe.MassKg, 0.01f);
	const double BoxIxx = Mass * (SizeM.Y * SizeM.Y + SizeM.Z * SizeM.Z) / 12.0;
	const double BoxIyy = Mass * (SizeM.X * SizeM.X + SizeM.Z * SizeM.Z) / 12.0;
	const double BoxIzz = Mass * (SizeM.X * SizeM.X + SizeM.Y * SizeM.Y) / 12.0;
	BodyInstance->InertiaTensorScale = FVector(
		Airframe.RollPitchInertia / FMath::Max(BoxIxx, 1e-9),
		Airframe.RollPitchInertia / FMath::Max(BoxIyy, 1e-9),
		Airframe.YawInertia / FMath::Max(BoxIzz, 1e-9));
	BodyInstance->UpdateMassProperties();
}

void AFPVDronePawn::RegisterPhysicsCallback()
{
	if (SimCallback != nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	FPhysScene* PhysicsScene = World ? World->GetPhysicsScene() : nullptr;
	auto* Solver = PhysicsScene ? PhysicsScene->GetSolver() : nullptr;
	if (Solver == nullptr)
	{
		UE_LOG(LogFPVDrone, Error, TEXT("Drone: no Chaos solver; flight controller not running."));
		return;
	}

	// The solver owns the callback object. Registration is queued and only takes effect on a
	// later physics step, so setting the bridge right after creation is safe.
	SimCallback = Solver->CreateAndRegisterSimCallbackObject_External<FFPVDroneSimCallback>();
	if (SimCallback != nullptr)
	{
		SimCallback->SetBridge_External(PhysicsBridge);
	}
}

void AFPVDronePawn::UnregisterPhysicsCallback()
{
	if (PhysicsBridge.IsValid())
	{
		PhysicsBridge->ClearBody();
	}

	if (SimCallback == nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	FPhysScene* PhysicsScene = World ? World->GetPhysicsScene() : nullptr;
	auto* Solver = PhysicsScene ? PhysicsScene->GetSolver() : nullptr;
	if (Solver != nullptr)
	{
		Solver->UnregisterAndFreeSimCallbackObject_External(SimCallback);
	}
	SimCallback = nullptr;
}

void AFPVDronePawn::PushBodyToPhysicsThread()
{
	if (!PhysicsBridge.IsValid())
	{
		return;
	}

	// The proxy is re-read every frame in case the engine ever recreates the physics body.
	Chaos::FSingleParticlePhysicsProxy* Proxy = nullptr;
	if (FBodyInstance* BodyInstance = Body->GetBodyInstance())
	{
		// NEEDS VERIFICATION in 5.8: FBodyInstance::GetPhysicsActorHandle() returns
		// FPhysicsActorHandle (== Chaos::FSingleParticlePhysicsProxy*).
		Proxy = BodyInstance->GetPhysicsActorHandle();
	}

	const UWorld* World = GetWorld();
	const float GravityMps2 = World ? FMath::Abs(World->GetGravityZ()) * FPVUnits::MetersPerCm : 9.81f;
	PhysicsBridge->SetBody(Proxy, GravityMps2);
}

void AFPVDronePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Forces applied on the physics thread don't wake a sleeping body; keep it awake.
	if (Body->IsSimulatingPhysics() && !Body->RigidBodyIsAwake())
	{
		Body->WakeRigidBody();
	}

	PushBodyToPhysicsThread();
	UpdatePropVisuals(DeltaSeconds);
}

void AFPVDronePawn::UpdatePropVisuals(float DeltaSeconds)
{
	const FFPVFlightTelemetry Telemetry = GetTelemetry();
	for (int32 Index = 0; Index < FPVQuad::NumMotors && Index < PropParts.Num(); ++Index)
	{
		UStaticMeshComponent* Prop = PropParts[Index];
		if (Prop == nullptr)
		{
			continue;
		}
		// Counter-clockwise (seen from above) is negative yaw in Unreal.
		const float Speed = Telemetry.MotorOutputs[Index] * FPVDroneGeometry::PropVisualMaxDegPerSec;
		PropAnglesDeg[Index] = FMath::Fmod(PropAnglesDeg[Index] - FPVQuadLayout::Spin[Index] * Speed * DeltaSeconds, 360.0f);
		Prop->SetRelativeRotation(FRotator(0.0, PropAnglesDeg[Index], 0.0));
	}
}

void AFPVDronePawn::SetPilotCommand(const FFPVPilotCommand& Command)
{
	LastCommand = Command;
	LastCommand.FlightMode = FlightMode;
	if (PhysicsBridge.IsValid())
	{
		PhysicsBridge->SetPilotCommand(LastCommand);
	}
}

void AFPVDronePawn::ApplyTuning(const FFPVDroneTuning& NewTuning)
{
	Tuning = NewTuning;
	if (PhysicsBridge.IsValid())
	{
		PhysicsBridge->SetTuning(Tuning);
	}
	ApplyBodyMassProperties();
}

void AFPVDronePawn::SetFlightMode(EFPVFlightMode NewMode)
{
	if (NewMode != FlightMode)
	{
		UE_LOG(LogFPVDrone, Log, TEXT("Flight mode: %s"), *UEnum::GetDisplayValueAsText(NewMode).ToString());
	}
	FlightMode = NewMode;
	// Re-send the last command so the physics thread switches mode immediately.
	SetPilotCommand(LastCommand);
}

void AFPVDronePawn::ToggleFlightMode()
{
	SetFlightMode(FlightMode == EFPVFlightMode::Angle ? EFPVFlightMode::Acro : EFPVFlightMode::Angle);
}

void AFPVDronePawn::ResetDrone(const FTransform& SpawnTransform)
{
	Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	SetActorLocationAndRotation(SpawnTransform.GetLocation(), SpawnTransform.GetRotation(), false, nullptr, ETeleportType::ResetPhysics);
	Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	LastCommand = FFPVPilotCommand();
	LastCommand.FlightMode = FlightMode;
	if (PhysicsBridge.IsValid())
	{
		PhysicsBridge->SetPilotCommand(LastCommand);
		PhysicsBridge->RequestControllerReset();
	}
}

FFPVFlightTelemetry AFPVDronePawn::GetTelemetry() const
{
	return PhysicsBridge.IsValid() ? PhysicsBridge->GetTelemetry() : FFPVFlightTelemetry();
}

FVector AFPVDronePawn::GetVelocityMps() const
{
	return FPVUnits::CmToMeters(Body->GetPhysicsLinearVelocity());
}

void AFPVDronePawn::ApplyCameraSettings(const FFPVCameraSettings& CameraSettings)
{
	if (FpvCamera == nullptr)
	{
		return;
	}
	FpvCamera->SetRelativeRotation(FRotator(CameraSettings.FpvUptiltDeg, 0.0f, 0.0f));
	FpvCamera->SetFieldOfView(CameraSettings.FpvFovDeg);
}
