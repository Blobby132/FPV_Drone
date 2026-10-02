// Chaos sim callback that runs the drone's flight controller on the physics thread,
// once per physics step (1/240 s with the project's async physics settings).
//
// Why a sim callback instead of Tick(): Tick runs at the (variable) render frame rate. Forces
// added there are held constant for the whole frame, which makes a stiff rate controller unstable
// at low frame rates. The callback runs inside every fixed physics step, reads the body's current
// attitude and rates, and applies fresh motor forces for exactly that step.

#pragma once

#include "CoreMinimal.h"
#include "Chaos/SimCallbackObject.h"
#include "Flight/FPVFlightController.h"
#include "Flight/FPVMotorModel.h"
#include "Physics/FPVDronePhysicsBridge.h"

/**
 * Chaos requires input/output types for a sim callback. We don't use Chaos' own input
 * marshaling (the FFPVDronePhysicsBridge snapshot is used instead, so the physics thread always
 * reads the latest command regardless of how many steps run per frame), so these are empty.
 */
struct FFPVDroneSimCallbackInput : public Chaos::FSimCallbackInput
{
	void Reset() {}
};

struct FFPVDroneSimCallbackOutput : public Chaos::FSimCallbackOutput
{
	void Reset() {}
};

// NOTE (needs verification in 5.8): TSimCallbackObject's third template parameter
// (ESimCallbackOptions) defaults to Presimulate in UE 5.1+, which is what we need.
class FFPVDroneSimCallback : public Chaos::TSimCallbackObject<FFPVDroneSimCallbackInput, FFPVDroneSimCallbackOutput>
{
public:
	/** Game thread, immediately after creation (before the physics thread can run the callback). */
	void SetBridge_External(const TSharedPtr<FFPVDronePhysicsBridge, ESPMode::ThreadSafe>& InBridge)
	{
		Bridge = InBridge;
	}

	// Declared without "override" on purpose: in some engine versions this is a virtual on
	// ISimCallbackObject (used for stat names); in others it may not exist. Either way this compiles.
	virtual FName GetFNameForStatId() const;

protected:
	virtual void OnPreSimulate_Internal() override;

private:
	/** Kept alive by this shared pointer even if the pawn is destroyed first. */
	TSharedPtr<FFPVDronePhysicsBridge, ESPMode::ThreadSafe> Bridge;

	// ---- Physics-thread-only state ----------------------------------------------------------
	FFPVPhysicsInput Input;
	FFPVDroneTuning Tuning;
	uint32 TuningVersion = 0;
	uint32 LastResetCounter = 0;
	uint64 StepCount = 0;
	FFPVFlightController Controller;
	FFPVMotorModel Motors;
};
