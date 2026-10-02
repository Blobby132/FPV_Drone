#include "Physics/FPVDroneSimCallback.h"

#include "PhysicsProxy/SingleParticlePhysicsProxy.h"
#include "Flight/FPVAirframeModel.h"
#include "Flight/FPVUnits.h"

namespace FPVChaosAccess
{
	// Chaos' physics-thread body handle (Chaos::FRigidBodyHandle_Internal) renamed its accessors
	// during UE5 (X()/R()/V()/W() -> GetX()/GetR()/GetV()/GetW(), with the old names deprecated).
	// These helpers use C++20 requires-expressions to call whichever name exists at compile time,
	// so this is the only place to touch if the API changes again. (NEEDS VERIFICATION in 5.8.)
	//
	// Values are in Unreal units: rotation body->world, velocity cm/s (world), angular velocity rad/s (world).

	template <typename THandle>
	FQuat GetRotation(THandle& Handle)
	{
		if constexpr (requires { Handle.GetR(); })
		{
			return FQuat(Handle.GetR());
		}
		else
		{
			return FQuat(Handle.R());
		}
	}

	template <typename THandle>
	FVector GetLinearVelocityCmPerSec(THandle& Handle)
	{
		if constexpr (requires { Handle.GetV(); })
		{
			return FVector(Handle.GetV());
		}
		else
		{
			return FVector(Handle.V());
		}
	}

	template <typename THandle>
	FVector GetAngularVelocityRadPerSec(THandle& Handle)
	{
		if constexpr (requires { Handle.GetW(); })
		{
			return FVector(Handle.GetW());
		}
		else
		{
			return FVector(Handle.W());
		}
	}

	/** World-space force (kg*cm/s^2) and torque (kg*cm^2/s^2) for this step only. */
	template <typename THandle>
	void AddForceAndTorque(THandle& Handle, const FVector& ForceUnreal, const FVector& TorqueUnreal)
	{
		Handle.AddForce(ForceUnreal);
		Handle.AddTorque(TorqueUnreal);
	}
}

FName FFPVDroneSimCallback::GetFNameForStatId() const
{
	static const FName StatName(TEXT("FPVDroneSimCallback"));
	return StatName;
}

void FFPVDroneSimCallback::OnPreSimulate_Internal()
{
	const float Dt = static_cast<float>(GetDeltaTime_Internal());
	if (!Bridge.IsValid() || Dt <= 0.0f)
	{
		return;
	}

	// Latest snapshot from the game thread.
	Bridge->ReadInput(Input);
	Bridge->ReadTuningIfChanged(TuningVersion, Tuning);

	if (Input.ResetCounter != LastResetCounter)
	{
		LastResetCounter = Input.ResetCounter;
		Controller.Reset();
		Motors.Reset();
	}

	Chaos::FSingleParticlePhysicsProxy* Proxy = Input.BodyProxy;
	if (Proxy == nullptr)
	{
		return;
	}
	Chaos::FRigidBodyHandle_Internal* Body = Proxy->GetPhysicsThreadAPI();
	if (Body == nullptr)
	{
		// Body not registered with the solver yet.
		return;
	}

	// --- Sense ---------------------------------------------------------------------------------
	FFPVBodyState State;
	State.Rotation = FPVChaosAccess::GetRotation(*Body);
	State.Rotation.Normalize();
	State.LinearVelocity = FPVUnits::CmToMeters(FPVChaosAccess::GetLinearVelocityCmPerSec(*Body));
	State.AngularVelocity = FPVChaosAccess::GetAngularVelocityRadPerSec(*Body);

	// --- Control -------------------------------------------------------------------------------
	FFPVControllerOutput ControllerOutput;
	Controller.Update(Input.Command, State, Tuning, Dt, ControllerOutput);
	Motors.Step(ControllerOutput.MotorCommands, Dt, Tuning.Airframe);

	// --- Actuate (SI -> Unreal units) ----------------------------------------------------------
	const FFPVAirframeForces Forces = FPVAirframeModel::ComputeForces(State, Motors, Tuning.Airframe, Input.GravityMps2);
	FPVChaosAccess::AddForceAndTorque(*Body,
		FPVUnits::NewtonsToUnrealForce(Forces.ForceWorldN),
		FPVUnits::NewtonMetersToUnrealTorque(Forces.TorqueWorldNm));

	// --- Report --------------------------------------------------------------------------------
	++StepCount;
	FFPVFlightTelemetry Telemetry;
	for (int32 Index = 0; Index < FPVQuad::NumMotors; ++Index)
	{
		Telemetry.MotorOutputs[Index] = Motors.GetOutput(Index);
	}
	Telemetry.MixerThrottle = ControllerOutput.MixerThrottle;
	Telemetry.RateSetpointDegS = ControllerOutput.RateSetpointDegS;
	Telemetry.GyroDegS = ControllerOutput.GyroDegS;
	Telemetry.TotalThrustN = Forces.TotalThrustN;
	Telemetry.bMixerSaturated = ControllerOutput.bMixerSaturated;
	Telemetry.FlightMode = Input.Command.FlightMode;
	Telemetry.PhysicsStepCount = StepCount;
	Telemetry.LastPhysicsDt = Dt;
	Bridge->WriteTelemetry(Telemetry);
}
