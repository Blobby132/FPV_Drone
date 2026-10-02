#include "World/FPVTestEnvironment.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#include "FPVDrone.h"

namespace FPVEnvironmentColors
{
	const FLinearColor Pad(0.08f, 0.08f, 0.09f);
	const FLinearColor PadMarking(0.95f, 0.75f, 0.05f);
	const FLinearColor Ground(0.25f, 0.32f, 0.2f);
}

AFPVTestEnvironment::AFPVTestEnvironment()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GridMaterialFinder(TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));

	CubeMesh = CubeFinder.Object;
	SphereMesh = SphereFinder.Object;
	CylinderMesh = CylinderFinder.Object;
	ConeMesh = ConeFinder.Object;
	ShapeMaterial = ShapeMaterialFinder.Object;
	GridMaterial = GridMaterialFinder.Object;
}

void AFPVTestEnvironment::BuildEnvironment()
{
	if (bBuilt)
	{
		return;
	}
	bBuilt = true;

	BuildGround();
	BuildLaunchPad();

	UE_LOG(LogFPVDrone, Log, TEXT("Test environment built with %d shapes."), Shapes.Num());
}

UMaterialInterface* AFPVTestEnvironment::GetColorMaterial(const FLinearColor& Color)
{
	if (ShapeMaterial == nullptr)
	{
		return nullptr;
	}

	const uint32 Key = Color.ToFColor(true).DWColor();
	if (TObjectPtr<UMaterialInstanceDynamic>* Existing = ColorMaterials.Find(Key))
	{
		return Existing->Get();
	}

	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(ShapeMaterial, this);
	// NEEDS VERIFICATION: parameter name of BasicShapeMaterial's tint ("Color").
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
	ColorMaterials.Add(Key, Material);
	return Material;
}

UStaticMeshComponent* AFPVTestEnvironment::AddShape(UStaticMesh* Mesh, const FVector& LocationCm, const FRotator& Rotation,
	const FVector& SizeCm, const FLinearColor& Color, UMaterialInterface* Material)
{
	if (Mesh == nullptr)
	{
		return nullptr;
	}

	UStaticMeshComponent* Shape = NewObject<UStaticMeshComponent>(this);
	// Static mobility and the mesh must be set before the component is registered.
	Shape->SetMobility(EComponentMobility::Static);
	Shape->SetStaticMesh(Mesh);
	Shape->SetupAttachment(Root);
	Shape->SetRelativeLocation(LocationCm);
	Shape->SetRelativeRotation(Rotation);
	Shape->SetRelativeScale3D(SizeCm / 100.0);
	Shape->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Shape->SetGenerateOverlapEvents(false);

	UMaterialInterface* AppliedMaterial = Material ? Material : GetColorMaterial(Color);
	if (AppliedMaterial != nullptr)
	{
		Shape->SetMaterial(0, AppliedMaterial);
	}

	Shape->RegisterComponent();
	AddInstanceComponent(Shape);
	Shapes.Add(Shape);
	return Shape;
}

void AFPVTestEnvironment::BuildGround()
{
	// A huge, 1 m thick slab whose top surface is at Z = 0. A cube is used instead of the
	// basic Plane because its box collision is solid (nothing can tunnel through it).
	const double Size = GroundHalfSizeCm * 2.0;
	UMaterialInterface* Material = GridMaterial ? GridMaterial.Get() : nullptr;
	AddShape(CubeMesh, FVector(0.0, 0.0, -50.0), FRotator::ZeroRotator, FVector(Size, Size, 100.0), FPVEnvironmentColors::Ground, Material);
}

void AFPVTestEnvironment::BuildLaunchPad()
{
	// 3 m launch pad at the origin (top at Z = +1 cm) with an arrow pointing along +X (spawn heading).
	AddShape(CylinderMesh, FVector(0.0, 0.0, -1.0), FRotator::ZeroRotator, FVector(300.0, 300.0, 4.0), FPVEnvironmentColors::Pad);
	AddShape(CubeMesh, FVector(40.0, 0.0, 1.2), FRotator::ZeroRotator, FVector(120.0, 12.0, 0.6), FPVEnvironmentColors::PadMarking);
	AddShape(CubeMesh, FVector(88.0, 18.0, 1.2), FRotator(0.0, -40.0, 0.0), FVector(50.0, 10.0, 0.6), FPVEnvironmentColors::PadMarking);
	AddShape(CubeMesh, FVector(88.0, -18.0, 1.2), FRotator(0.0, 40.0, 0.0), FVector(50.0, 10.0, 0.6), FPVEnvironmentColors::PadMarking);
}
