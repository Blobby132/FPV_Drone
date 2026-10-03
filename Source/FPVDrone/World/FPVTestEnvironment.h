// Runtime-built test world made only from engine basic shapes (/Engine/BasicShapes).
// Every piece is a static mesh component with collision, created when the game mode calls
// BuildEnvironment(); nothing is authored as an asset.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPVTestEnvironment.generated.h"

class AFPVGate;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class FPVDRONE_API AFPVTestEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AFPVTestEnvironment();

	/** Create all environment components. Safe to call once; later calls do nothing. */
	virtual void BuildEnvironment();

protected:
	/**
	 * Adds a static, colliding mesh.
	 * @param SizeCm    Final size in cm (engine basic shapes are 100 cm, so scale = SizeCm / 100).
	 * @param Material  Optional material; otherwise the basic shape material tinted with Color.
	 */
	UStaticMeshComponent* AddShape(UStaticMesh* Mesh, const FVector& LocationCm, const FRotator& Rotation,
		const FVector& SizeCm, const FLinearColor& Color, UMaterialInterface* Material = nullptr);

	/** One shared dynamic material per color. */
	UMaterialInterface* GetColorMaterial(const FLinearColor& Color);

	void BuildGround();
	void BuildLaunchPad();
	void BuildHills();
	void BuildTown();
	void BuildBando();
	void BuildWindowWall();
	void BuildTrees();
	void BuildSlalomPoles();
	void BuildGates();

	void AddTree(const FVector& GroundLocationCm, float HeightCm, bool bConifer);
	AFPVGate* SpawnGate(const FVector& LocationCm, float YawDeg, bool bRing, float OpeningCm, float BottomCm, const FLinearColor& Color);

	/** True if a point (cm) is inside the area kept free around the launch pad and gate course. */
	bool IsInReservedArea(const FVector2D& PointCm) const;

	UPROPERTY(VisibleAnywhere, Category = "Environment")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> ConeMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ShapeMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GridMaterial;

	UPROPERTY(Transient)
	TMap<uint32, TObjectPtr<UMaterialInstanceDynamic>> ColorMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Shapes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFPVGate>> Gates;

	/** Seed for the randomized parts (trees, building sizes). Same seed = same world. */
	UPROPERTY(EditAnywhere, Category = "Environment")
	int32 RandomSeed = 1337;

	/** Half size of the square ground plane (cm). */
	UPROPERTY(EditAnywhere, Category = "Environment")
	float GroundHalfSizeCm = 100000.0f;

	bool bBuilt = false;
};
