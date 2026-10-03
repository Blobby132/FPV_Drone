#include "World/FPVGate.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#include "Gameplay/FPVPassThroughTriggerComponent.h"

namespace FPVGateGeometry
{
	/** Frame bar thickness (cm). */
	constexpr float Thickness = 15.0f;
	/** Frame depth along the flight direction (cm). */
	constexpr float Depth = 15.0f;
	/** Trigger depth (cm). Deep enough that a fast drone overlaps it for at least one frame. */
	constexpr float TriggerDepth = 150.0f;
	/** Ring segments. */
	constexpr int32 RingSegments = 16;
}

AFPVGate::AFPVGate()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = CubeFinder.Object;
	CylinderMesh = CylinderFinder.Object;
	ShapeMaterial = MaterialFinder.Object;
}

UStaticMeshComponent* AFPVGate::AddPart(UStaticMesh* Mesh, const FVector& LocationCm, const FRotator& Rotation, const FVector& SizeCm)
{
	if (Mesh == nullptr)
	{
		return nullptr;
	}
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
	Part->SetMobility(EComponentMobility::Static);
	Part->SetStaticMesh(Mesh);
	Part->SetupAttachment(Root);
	Part->SetRelativeLocation(LocationCm);
	Part->SetRelativeRotation(Rotation);
	Part->SetRelativeScale3D(SizeCm / 100.0);
	Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Part->SetGenerateOverlapEvents(false);
	if (GateMaterial != nullptr)
	{
		Part->SetMaterial(0, GateMaterial);
	}
	Part->RegisterComponent();
	AddInstanceComponent(Part);
	Parts.Add(Part);
	return Part;
}

void AFPVGate::InitializeGate(EFPVGateShape InShape, float OpeningSizeCm, float BottomHeightCm, const FLinearColor& Color, int32 InGateIndex)
{
	if (bInitialized)
	{
		return;
	}
	bInitialized = true;
	GateIndex = InGateIndex;

	using namespace FPVGateGeometry;

	if (ShapeMaterial != nullptr)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(ShapeMaterial, this);
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
		GateMaterial = Material;
	}

	const float Opening = FMath::Max(OpeningSizeCm, 50.0f);
	const float Bottom = FMath::Max(BottomHeightCm, 0.0f);
	const float CenterZ = Bottom + Opening * 0.5f;

	if (InShape == EFPVGateShape::Ring)
	{
		// Cylinder segments around a circle in the Y/Z plane. A cylinder's axis is its local Z;
		// rolling by -Angle turns that axis to the circle's tangent (0, -sin, cos).
		const float Radius = Opening * 0.5f + Thickness * 0.5f;
		const float SegmentLength = 2.0f * UE_PI * Radius / static_cast<float>(RingSegments) * 1.1f;
		for (int32 Index = 0; Index < RingSegments; ++Index)
		{
			const float Angle = 2.0f * UE_PI * static_cast<float>(Index) / static_cast<float>(RingSegments);
			const FVector Location(0.0, Radius * FMath::Cos(Angle), CenterZ + Radius * FMath::Sin(Angle));
			const FRotator Rotation(0.0, 0.0, -FMath::RadiansToDegrees(Angle));
			AddPart(CylinderMesh, Location, Rotation, FVector(Thickness, Thickness, SegmentLength));
		}
		// Single post from the ground to the bottom of the ring.
		const float PostHeight = CenterZ - Radius;
		if (PostHeight > 1.0f)
		{
			AddPart(CylinderMesh, FVector(0.0, 0.0, PostHeight * 0.5f), FRotator::ZeroRotator, FVector(Thickness, Thickness, PostHeight));
		}
	}
	else
	{
		// Two posts from the ground to the top, a top bar and (if raised) a bottom bar.
		const float TotalHeight = Bottom + Opening + Thickness;
		const float PostY = Opening * 0.5f + Thickness * 0.5f;
		AddPart(CubeMesh, FVector(0.0, -PostY, TotalHeight * 0.5f), FRotator::ZeroRotator, FVector(Depth, Thickness, TotalHeight));
		AddPart(CubeMesh, FVector(0.0, PostY, TotalHeight * 0.5f), FRotator::ZeroRotator, FVector(Depth, Thickness, TotalHeight));
		AddPart(CubeMesh, FVector(0.0, 0.0, Bottom + Opening + Thickness * 0.5f), FRotator::ZeroRotator, FVector(Depth, Opening + Thickness * 2.0f, Thickness));
		if (Bottom > Thickness)
		{
			AddPart(CubeMesh, FVector(0.0, 0.0, Bottom - Thickness * 0.5f), FRotator::ZeroRotator, FVector(Depth, Opening, Thickness));
		}
	}

	// Trigger filling the opening (a ring uses the square inscribed in the circle).
	const float HalfOpening = (InShape == EFPVGateShape::Ring) ? Opening * 0.5f * 0.7f : Opening * 0.5f;
	Trigger = NewObject<UFPVPassThroughTriggerComponent>(this, TEXT("PassTrigger"));
	Trigger->SetMobility(EComponentMobility::Static);
	Trigger->SetupAttachment(Root);
	Trigger->SetRelativeLocation(FVector(0.0, 0.0, CenterZ));
	Trigger->InitBoxExtent(FVector(TriggerDepth * 0.5f, HalfOpening, HalfOpening));
	Trigger->TriggerIndex = GateIndex;
	Trigger->RegisterComponent();
	AddInstanceComponent(Trigger);
}
