#include "Physics/FPVDronePhysicsBridge.h"
#include "Misc/ScopeLock.h"

void FFPVDronePhysicsBridge::SetPilotCommand(const FFPVPilotCommand& Command)
{
	FScopeLock Lock(&Mutex);
	Input.Command = Command;
}

void FFPVDronePhysicsBridge::SetBody(Chaos::FSingleParticlePhysicsProxy* BodyProxy, float GravityMps2)
{
	FScopeLock Lock(&Mutex);
	Input.BodyProxy = BodyProxy;
	Input.GravityMps2 = GravityMps2;
}

void FFPVDronePhysicsBridge::ClearBody()
{
	FScopeLock Lock(&Mutex);
	Input.BodyProxy = nullptr;
}

void FFPVDronePhysicsBridge::SetTuning(const FFPVDroneTuning& NewTuning)
{
	FScopeLock Lock(&Mutex);
	Tuning = NewTuning;
	++TuningVersion;
	if (TuningVersion == 0)
	{
		// Never collide with the physics thread's initial "nothing read yet" value.
		TuningVersion = 1;
	}
}

void FFPVDronePhysicsBridge::RequestControllerReset()
{
	FScopeLock Lock(&Mutex);
	++Input.ResetCounter;
}

FFPVFlightTelemetry FFPVDronePhysicsBridge::GetTelemetry() const
{
	FScopeLock Lock(&Mutex);
	return Telemetry;
}

void FFPVDronePhysicsBridge::ReadInput(FFPVPhysicsInput& OutInput) const
{
	FScopeLock Lock(&Mutex);
	OutInput = Input;
}

bool FFPVDronePhysicsBridge::ReadTuningIfChanged(uint32& InOutVersion, FFPVDroneTuning& OutTuning) const
{
	FScopeLock Lock(&Mutex);
	if (InOutVersion == TuningVersion)
	{
		return false;
	}
	OutTuning = Tuning;
	InOutVersion = TuningVersion;
	return true;
}

void FFPVDronePhysicsBridge::WriteTelemetry(const FFPVFlightTelemetry& NewTelemetry)
{
	FScopeLock Lock(&Mutex);
	Telemetry = NewTelemetry;
}
