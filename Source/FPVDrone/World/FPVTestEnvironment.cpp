#include "World/FPVTestEnvironment.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"
#include "UObject/ConstructorHelpers.h"

#include "World/FPVGate.h"
#include "FPVDrone.h"

namespace FPVEnvironmentColors
{
	const FLinearColor Pad(0.08f, 0.08f, 0.09f);
	const FLinearColor PadMarking(0.95f, 0.75f, 0.05f);
	const FLinearColor Ground(0.25f, 0.32f, 0.2f);
	const FLinearColor Hill(0.16f, 0.3f, 0.1f);
	const FLinearColor Trunk(0.22f, 0.13f, 0.06f);
	const FLinearColor LeafA(0.08f, 0.3f, 0.06f);
	const FLinearColor LeafB(0.12f, 0.38f, 0.08f);
	const FLinearColor Conifer(0.04f, 0.2f, 0.07f);
	const FLinearColor Concrete(0.45f, 0.45f, 0.43f);
	const FLinearColor DarkConcrete(0.25f, 0.25f, 0.26f);
	const FLinearColor PoleRed(0.85f, 0.1f, 0.08f);
	const FLinearColor PoleWhite(0.9f, 0.9f, 0.9f);
	const FLinearColor GateOrange(1.0f, 0.4f, 0.0f);
	const FLinearColor GateBlue(0.05f, 0.35f, 1.0f);
	const FLinearColor GateMagenta(0.9f, 0.05f, 0.6f);

	const FLinearColor BuildingPalette[] =
	{
		FLinearColor(0.55f, 0.53f, 0.5f),
		FLinearColor(0.42f, 0.4f, 0.38f),
		FLinearColor(0.5f, 0.32f, 0.24f),
		FLinearColor(0.62f, 0.58f, 0.48f),
		FLinearColor(0.3f, 0.33f, 0.38f),
	};
}

namespace FPVEnvironmentLayout
{
	// All distances in cm. The drone spawns at the origin facing +X.
	constexpr double Meter = 100.0;

	/** Nothing random is placed within this radius of the launch pad. */
	constexpr double ClearRadius = 45.0 * Meter;

	/** Town: a grid of buildings in front of the pad. */
	constexpr double TownMinX = 140.0 * Meter;
	constexpr double TownMaxX = 300.0 * Meter;
	constexpr double TownMinY = -90.0 * Meter;
	constexpr double TownMaxY = 90.0 * Meter;

	/** Forest behind the pad. */
	constexpr double ForestMinX = -420.0 * Meter;
	constexpr double ForestMaxX = -140.0 * Meter;
	constexpr double ForestMinY = -300.0 * Meter;
	constexpr double ForestMaxY = 300.0 * Meter;

	/** Hills: big spheres, mostly buried (values in meters). */
	struct FEnvironmentHill
	{
		double X;
		double Y;
		double Radius;
		double VisibleHeight;

		/** Radius of the circle where the sphere meets the ground (m). */
		double FootprintRadius() const
		{
			const double Buried = Radius - VisibleHeight;
			return FMath::Sqrt(FMath::Max(Radius * Radius - Buried * Buried, 0.0));
		}
	};

	inline constexpr FEnvironmentHill Hills[] =
	{
		{ -320.0, 260.0, 60.0, 12.0 },
		{ 460.0, 420.0, 85.0, 18.0 },
		{ -160.0, -460.0, 50.0, 9.0 },
		{ 620.0, -320.0, 75.0, 16.0 },
		{ -560.0, -120.0, 95.0, 24.0 },
		{ 160.0, 720.0, 65.0, 11.0 },
		{ 40.0, -620.0, 110.0, 28.0 },
	};

	/** Axis-aligned areas (meters) kept free of random scatter: gate course + slalom, window wall, bando. */
	struct FEnvironmentArea
	{
		double MinX;
		double MaxX;
		double MinY;
		double MaxY;
	};

	inline constexpr FEnvironmentArea ReservedAreas[] =
	{
		{ -45.0, 110.0, -55.0, 95.0 },
		{ -100.0, -80.0, -25.0, 25.0 },
		{ -75.0, -45.0, 60.0, 100.0 },
	};
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
	BuildHills();
	BuildTown();
	BuildBando();
	BuildWindowWall();
	BuildTrees();
	BuildSlalomPoles();
	BuildGates();

	UE_LOG(LogFPVDrone, Log, TEXT("Test environment built: %d shapes, %d gates."), Shapes.Num(), Gates.Num());
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

bool AFPVTestEnvironment::IsInReservedArea(const FVector2D& PointCm) const
{
	using namespace FPVEnvironmentLayout;
	if (PointCm.Size() < ClearRadius)
	{
		return true;
	}

	// The town gets its own grid; keep random scatter out of it.
	if (PointCm.X > TownMinX - 10.0 * Meter && PointCm.X < TownMaxX + 10.0 * Meter
		&& PointCm.Y > TownMinY - 10.0 * Meter && PointCm.Y < TownMaxY + 10.0 * Meter)
	{
		return true;
	}

	const FVector2D PointM = PointCm / Meter;
	for (const FEnvironmentArea& Area : ReservedAreas)
	{
		if (PointM.X > Area.MinX && PointM.X < Area.MaxX && PointM.Y > Area.MinY && PointM.Y < Area.MaxY)
		{
			return true;
		}
	}

	// Don't plant trees halfway into a hill.
	for (const FEnvironmentHill& Hill : Hills)
	{
		if (FVector2D::Distance(PointM, FVector2D(Hill.X, Hill.Y)) < Hill.FootprintRadius() + 3.0)
		{
			return true;
		}
	}
	return false;
}

void AFPVTestEnvironment::BuildHills()
{
	// "Flattened spheres": large spheres mostly buried in the ground, so only a low dome shows.
	// Uniform scale is deliberate: Chaos sphere collision does not support non-uniform scale (a
	// squashed sphere would collide as a smaller round sphere), so we bury big round ones instead.
	using namespace FPVEnvironmentLayout;
	for (const FEnvironmentHill& Hill : Hills)
	{
		const double Radius = Hill.Radius * Meter;
		const double CenterZ = Hill.VisibleHeight * Meter - Radius;
		AddShape(SphereMesh, FVector(Hill.X * Meter, Hill.Y * Meter, CenterZ), FRotator::ZeroRotator,
			FVector(Radius * 2.0), FPVEnvironmentColors::Hill);
	}
}

void AFPVTestEnvironment::BuildTown()
{
	using namespace FPVEnvironmentLayout;
	FRandomStream Random(RandomSeed);

	// 5 x 5 blocks (40 m x 45 m spacing); a few blocks are left empty as plazas.
	const int32 Columns = 5;
	const int32 Rows = 5;
	const double SpacingX = (TownMaxX - TownMinX) / static_cast<double>(Columns - 1);
	const double SpacingY = (TownMaxY - TownMinY) / static_cast<double>(Rows - 1);
	for (int32 Column = 0; Column < Columns; ++Column)
	{
		for (int32 Row = 0; Row < Rows; ++Row)
		{
			if (Random.FRand() < 0.15f)
			{
				continue;
			}
			const double CenterX = TownMinX + SpacingX * Column + Random.FRandRange(-3.0f, 3.0f) * Meter;
			const double CenterY = TownMinY + SpacingY * Row + Random.FRandRange(-3.0f, 3.0f) * Meter;
			const double SizeX = Random.FRandRange(12.0f, 24.0f) * Meter;
			const double SizeY = Random.FRandRange(12.0f, 24.0f) * Meter;
			// Taller towards the middle of town.
			const double DistanceFromMiddle = FMath::Abs(Column - Columns / 2) + FMath::Abs(Row - Rows / 2);
			const double Height = Random.FRandRange(8.0f, 18.0f) * Meter + (4 - DistanceFromMiddle) * Random.FRandRange(3.0f, 8.0f) * Meter;
			const int32 PaletteSize = static_cast<int32>(UE_ARRAY_COUNT(FPVEnvironmentColors::BuildingPalette));
			const FLinearColor Color = FPVEnvironmentColors::BuildingPalette[Random.RandRange(0, PaletteSize - 1)];

			AddShape(CubeMesh, FVector(CenterX, CenterY, Height * 0.5), FRotator::ZeroRotator, FVector(SizeX, SizeY, Height), Color);

			// Rooftop box on some buildings (something to skim over or around).
			if (Random.FRand() < 0.4f)
			{
				const double RoofSize = FMath::Min(SizeX, SizeY) * 0.35;
				AddShape(CubeMesh, FVector(CenterX, CenterY, Height + RoofSize * 0.25), FRotator::ZeroRotator,
					FVector(RoofSize, RoofSize, RoofSize * 0.5), FPVEnvironmentColors::DarkConcrete);
			}
		}
	}
}

void AFPVTestEnvironment::BuildBando()
{
	// Two towers joined by a bridge: a big "window" (10 m wide, 12 m tall) to fly through,
	// plus a lower bridge you can dive under.
	using FPVEnvironmentLayout::Meter;
	const FVector Base(-60.0 * Meter, 80.0 * Meter, 0.0);
	const double TowerSize = 8.0 * Meter;
	const double TowerHeight = 20.0 * Meter;
	const double Gap = 10.0 * Meter;
	const double TowerOffsetY = Gap * 0.5 + TowerSize * 0.5;

	AddShape(CubeMesh, Base + FVector(0.0, -TowerOffsetY, TowerHeight * 0.5), FRotator::ZeroRotator,
		FVector(TowerSize, TowerSize, TowerHeight), FPVEnvironmentColors::DarkConcrete);
	AddShape(CubeMesh, Base + FVector(0.0, TowerOffsetY, TowerHeight * 0.5), FRotator::ZeroRotator,
		FVector(TowerSize, TowerSize, TowerHeight), FPVEnvironmentColors::DarkConcrete);

	// Upper bridge (window between ground+12 m and 17 m) and a low walkway at 4 m.
	AddShape(CubeMesh, Base + FVector(0.0, 0.0, 14.5 * Meter), FRotator::ZeroRotator,
		FVector(TowerSize, Gap, 5.0 * Meter), FPVEnvironmentColors::Concrete);
	AddShape(CubeMesh, Base + FVector(0.0, 0.0, 4.0 * Meter), FRotator::ZeroRotator,
		FVector(3.0 * Meter, Gap, 0.6 * Meter), FPVEnvironmentColors::Concrete);
}

void AFPVTestEnvironment::BuildWindowWall()
{
	// A 40 m long, 12 m high wall with three 4 x 4 m windows at different heights.
	using FPVEnvironmentLayout::Meter;
	const double WallX = -90.0 * Meter;
	const double Thickness = 1.0 * Meter;
	const double WallHeight = 12.0 * Meter;
	const double WindowSize = 4.0 * Meter;

	struct FWallWindow { double CenterY; double BottomZ; };
	const FWallWindow Windows[] = { { -12.0 * Meter, 2.0 * Meter }, { 0.0, 5.0 * Meter }, { 12.0 * Meter, 3.0 * Meter } };

	// Solid columns between/around windows.
	const double WallMinY = -20.0 * Meter;
	const double WallMaxY = 20.0 * Meter;
	double SegmentStart = WallMinY;
	for (const FWallWindow& Window : Windows)
	{
		const double SegmentEnd = Window.CenterY - WindowSize * 0.5;
		AddShape(CubeMesh, FVector(WallX, (SegmentStart + SegmentEnd) * 0.5, WallHeight * 0.5), FRotator::ZeroRotator,
			FVector(Thickness, SegmentEnd - SegmentStart, WallHeight), FPVEnvironmentColors::Concrete);

		// Below and above the window opening.
		if (Window.BottomZ > 1.0)
		{
			AddShape(CubeMesh, FVector(WallX, Window.CenterY, Window.BottomZ * 0.5), FRotator::ZeroRotator,
				FVector(Thickness, WindowSize, Window.BottomZ), FPVEnvironmentColors::Concrete);
		}
		const double TopZ = Window.BottomZ + WindowSize;
		AddShape(CubeMesh, FVector(WallX, Window.CenterY, (TopZ + WallHeight) * 0.5), FRotator::ZeroRotator,
			FVector(Thickness, WindowSize, WallHeight - TopZ), FPVEnvironmentColors::Concrete);

		SegmentStart = Window.CenterY + WindowSize * 0.5;
	}
	AddShape(CubeMesh, FVector(WallX, (SegmentStart + WallMaxY) * 0.5, WallHeight * 0.5), FRotator::ZeroRotator,
		FVector(Thickness, WallMaxY - SegmentStart, WallHeight), FPVEnvironmentColors::Concrete);
}

void AFPVTestEnvironment::AddTree(const FVector& GroundLocationCm, float HeightCm, bool bConifer)
{
	const float TrunkDiameter = FMath::Clamp(HeightCm * 0.06f, 30.0f, 70.0f);
	if (bConifer)
	{
		// Short trunk + tall cone.
		const float TrunkHeight = HeightCm * 0.25f;
		AddShape(CylinderMesh, GroundLocationCm + FVector(0.0, 0.0, TrunkHeight * 0.5f), FRotator::ZeroRotator,
			FVector(TrunkDiameter, TrunkDiameter, TrunkHeight), FPVEnvironmentColors::Trunk);
		const float ConeHeight = HeightCm - TrunkHeight;
		const float ConeDiameter = HeightCm * 0.45f;
		AddShape(ConeMesh, GroundLocationCm + FVector(0.0, 0.0, TrunkHeight + ConeHeight * 0.5f), FRotator::ZeroRotator,
			FVector(ConeDiameter, ConeDiameter, ConeHeight), FPVEnvironmentColors::Conifer);
	}
	else
	{
		// Trunk + round canopy (uniformly scaled sphere: exact collision).
		const float TrunkHeight = HeightCm * 0.6f;
		AddShape(CylinderMesh, GroundLocationCm + FVector(0.0, 0.0, TrunkHeight * 0.5f), FRotator::ZeroRotator,
			FVector(TrunkDiameter, TrunkDiameter, TrunkHeight), FPVEnvironmentColors::Trunk);
		const float CanopyDiameter = HeightCm * 0.6f;
		const FLinearColor Leaf = (FMath::Fmod(GroundLocationCm.X + GroundLocationCm.Y, 2.0) > 1.0) ? FPVEnvironmentColors::LeafA : FPVEnvironmentColors::LeafB;
		AddShape(SphereMesh, GroundLocationCm + FVector(0.0, 0.0, TrunkHeight + CanopyDiameter * 0.35f), FRotator::ZeroRotator,
			FVector(CanopyDiameter), Leaf);
	}
}

void AFPVTestEnvironment::BuildTrees()
{
	using namespace FPVEnvironmentLayout;
	FRandomStream Random(RandomSeed + 1);

	// Dense forest behind the pad.
	for (int32 Index = 0; Index < 90; ++Index)
	{
		const FVector2D Point(
			Random.FRandRange(static_cast<float>(ForestMinX), static_cast<float>(ForestMaxX)),
			Random.FRandRange(static_cast<float>(ForestMinY), static_cast<float>(ForestMaxY)));
		if (IsInReservedArea(Point))
		{
			continue;
		}
		AddTree(FVector(Point.X, Point.Y, 0.0), Random.FRandRange(5.0f, 11.0f) * Meter, Random.FRand() < 0.5f);
	}

	// Scattered trees everywhere else (outside the launch area and the town).
	for (int32 Index = 0; Index < 60; ++Index)
	{
		const float Angle = Random.FRandRange(0.0f, 2.0f * UE_PI);
		const float Distance = Random.FRandRange(50.0f, 600.0f) * Meter;
		const FVector2D Point(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance);
		if (IsInReservedArea(Point))
		{
			continue;
		}
		AddTree(FVector(Point.X, Point.Y, 0.0), Random.FRandRange(4.0f, 10.0f) * Meter, Random.FRand() < 0.3f);
	}
}

void AFPVTestEnvironment::BuildSlalomPoles()
{
	// A line of tall poles to weave through, to the right of the launch pad.
	using FPVEnvironmentLayout::Meter;
	for (int32 Index = 0; Index < 9; ++Index)
	{
		const FVector Location(30.0 * Meter + Index * 12.0 * Meter, -45.0 * Meter, 7.5 * Meter);
		AddShape(CylinderMesh, Location, FRotator::ZeroRotator, FVector(40.0, 40.0, 15.0 * Meter),
			(Index % 2 == 0) ? FPVEnvironmentColors::PoleRed : FPVEnvironmentColors::PoleWhite);
	}
}

AFPVGate* AFPVTestEnvironment::SpawnGate(const FVector& LocationCm, float YawDeg, bool bRing, float OpeningCm, float BottomCm, const FLinearColor& Color)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AFPVGate* Gate = World->SpawnActor<AFPVGate>(AFPVGate::StaticClass(), FTransform(FRotator(0.0, YawDeg, 0.0), LocationCm), Params);
	if (Gate != nullptr)
	{
		Gate->InitializeGate(bRing ? EFPVGateShape::Ring : EFPVGateShape::Square, OpeningCm, BottomCm, Color, Gates.Num());
		Gates.Add(Gate);
	}
	return Gate;
}

void AFPVTestEnvironment::BuildGates()
{
	// A loose loop of gates around the launch pad (numbered in order). Free-fly ignores the order;
	// a race mode can use GetGateIndex() and UFPVGameplayEvents::OnTriggerPassed.
	using namespace FPVEnvironmentColors;
	using FPVEnvironmentLayout::Meter;
	SpawnGate(FVector(25.0 * Meter, 0.0, 0.0), 0.0f, false, 200.0f, 60.0f, GateOrange);
	SpawnGate(FVector(60.0 * Meter, 15.0 * Meter, 0.0), 20.0f, true, 300.0f, 100.0f, GateBlue);
	SpawnGate(FVector(95.0 * Meter, 45.0 * Meter, 0.0), 90.0f, false, 250.0f, 600.0f, GateMagenta);
	SpawnGate(FVector(65.0 * Meter, 80.0 * Meter, 0.0), 180.0f, true, 350.0f, 200.0f, GateOrange);
	SpawnGate(FVector(15.0 * Meter, 65.0 * Meter, 0.0), 215.0f, false, 200.0f, 60.0f, GateBlue);
	SpawnGate(FVector(-30.0 * Meter, 25.0 * Meter, 0.0), 270.0f, true, 500.0f, 300.0f, GateMagenta);
}
