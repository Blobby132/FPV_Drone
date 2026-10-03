// A racing-style gate built from engine basic shapes, with a pass-through trigger in its opening.
// Free-fly mode only uses it as an obstacle (passes are announced on UFPVGameplayEvents); a future
// race mode can collect gates by GetGateIndex() and listen for passes.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPVGate.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UFPVPassThroughTriggerComponent;

UENUM(BlueprintType)
enum class EFPVGateShape : uint8
{
	/** Square frame on two posts. */
	Square	UMETA(DisplayName = "Square"),
	/** Round ring on a single post. */
	Ring	UMETA(DisplayName = "Ring")
};

UCLASS()
class FPVDRONE_API AFPVGate : public AActor
{
	GENERATED_BODY()

public:
	AFPVGate();

	/**
	 * Builds the frame and the trigger. Call once, right after spawning.
	 * The opening faces the actor's +X axis (a "forward" pass flies along +X).
	 * @param OpeningSizeCm     Inner width/height (square) or inner diameter (ring).
	 * @param BottomHeightCm    Height of the bottom of the opening above the actor origin (ground).
	 */
	void InitializeGate(EFPVGateShape InShape, float OpeningSizeCm, float BottomHeightCm, const FLinearColor& Color, int32 InGateIndex);

	UFPVPassThroughTriggerComponent* GetTrigger() const { return Trigger; }
	int32 GetGateIndex() const { return GateIndex; }

private:
	UStaticMeshComponent* AddPart(UStaticMesh* Mesh, const FVector& LocationCm, const FRotator& Rotation, const FVector& SizeCm);

	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<UFPVPassThroughTriggerComponent> Trigger;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ShapeMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GateMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Gate")
	int32 GateIndex = -1;

	bool bInitialized = false;
};
