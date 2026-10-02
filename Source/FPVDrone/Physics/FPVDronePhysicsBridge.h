// Thread-safe hand-off between the game thread (input, settings, OSD) and the physics thread
// (flight controller). Each side only ever copies small structs in or out under a lock, so
// neither thread can see a half-written value. "Latest value wins": the physics thread always
// uses the most recent command, whether it runs 0, 1 or several steps per game frame.

#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "Flight/FPVFlightTypes.h"
#include "Settings/FPVSettingsTypes.h"

namespace Chaos
{
	class FSingleParticlePhysicsProxy;
}

/** Everything the physics thread needs from the game thread each step (except tuning, see below). */
struct FFPVPhysicsInput
{
	FFPVPilotCommand Command;
	/** The drone body's Chaos proxy (owned by the engine; refreshed by the game thread every frame). */
	Chaos::FSingleParticlePhysicsProxy* BodyProxy = nullptr;
	/** |gravity| in m/s^2 (from the world's gravity setting). */
	float GravityMps2 = 9.81f;
	/** Incremented by the game thread to ask the physics thread to reset controller/motor state. */
	uint32 ResetCounter = 0;
};

class FFPVDronePhysicsBridge
{
public:
	// ---- Game thread ----------------------------------------------------------------------
	void SetPilotCommand(const FFPVPilotCommand& Command);
	void SetBody(Chaos::FSingleParticlePhysicsProxy* BodyProxy, float GravityMps2);
	void ClearBody();
	void SetTuning(const FFPVDroneTuning& NewTuning);
	void RequestControllerReset();
	FFPVFlightTelemetry GetTelemetry() const;

	// ---- Physics thread -------------------------------------------------------------------
	void ReadInput(FFPVPhysicsInput& OutInput) const;
	/** Copies the tuning only when it changed since InOutVersion (tuning is large-ish). Returns true if copied. */
	bool ReadTuningIfChanged(uint32& InOutVersion, FFPVDroneTuning& OutTuning) const;
	void WriteTelemetry(const FFPVFlightTelemetry& NewTelemetry);

private:
	mutable FCriticalSection Mutex;
	FFPVPhysicsInput Input;
	FFPVDroneTuning Tuning;
	/** Starts at 1 so the physics thread (which starts at 0) always copies the initial tuning. */
	uint32 TuningVersion = 1;
	FFPVFlightTelemetry Telemetry;
};
