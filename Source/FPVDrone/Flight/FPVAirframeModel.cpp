#include "Flight/FPVAirframeModel.h"
#include "Flight/FPVMotorModel.h"
#include "Flight/FPVQuadMixer.h"
#include "Flight/FPVUnits.h"

float FPVAirframeModel::GetMaxThrustPerMotorN(const FFPVAirframeSettings& Airframe, float GravityMps2)
{
	const float WeightN = FMath::Max(Airframe.MassKg, 0.01f) * FMath::Max(GravityMps2, 0.0f);
	return FMath::Max(Airframe.ThrustToWeight, 0.0f) * WeightN / static_cast<float>(FPVQuad::NumMotors);
}

FFPVAirframeForces FPVAirframeModel::ComputeForces(const FFPVBodyState& State, const FFPVMotorModel& Motors,
	const FFPVAirframeSettings& Airframe, float GravityMps2)
{
	using namespace FPVQuadLayout;

	FFPVAirframeForces Result;

	const float MaxThrustPerMotor = GetMaxThrustPerMotorN(Airframe, GravityMps2);
	// Motor offset from the center along each body axis (m): arm length * cos(45 deg).
	const float AxisOffsetM = FPVUnits::CmToMeters(Airframe.ArmLengthCm) * 0.70710678f;

	// --- Motors (body frame) ---------------------------------------------------------------
	// Applying each motor's thrust at its arm position == thrust at the center of mass plus
	// the torque r x F. Prop drag adds a yaw reaction torque opposite to the prop's spin.
	FVector ForceBody = FVector::ZeroVector;
	FVector TorqueBody = FVector::ZeroVector;
	for (int32 Index = 0; Index < FPVQuad::NumMotors; ++Index)
	{
		const float ThrustN = Motors.GetThrustN(Index, MaxThrustPerMotor, Airframe.ThrustExponent);
		const FVector Arm(PosX[Index] * AxisOffsetM, PosY[Index] * AxisOffsetM, 0.0);
		const FVector Thrust(0.0, 0.0, ThrustN);

		ForceBody += Thrust;
		TorqueBody += FVector::CrossProduct(Arm, Thrust);
		TorqueBody.Z += Spin[Index] * Airframe.YawTorquePerThrust * ThrustN;
		Result.TotalThrustN += ThrustN;
	}

	// --- Air drag (body frame, so the flat underside can have more drag) --------------------
	const FVector VelocityBody = State.Rotation.UnrotateVector(State.LinearVelocity);
	const double Speed = VelocityBody.Size();
	FVector DragBody = -(static_cast<double>(Airframe.LinearDrag) + static_cast<double>(Airframe.QuadraticDrag) * Speed) * VelocityBody;
	DragBody.Z *= Airframe.VerticalDragMultiplier;
	ForceBody += DragBody;

	// --- Angular drag ----------------------------------------------------------------------
	const FVector AngularVelocityBody = State.Rotation.UnrotateVector(State.AngularVelocity);
	TorqueBody += FVector(
		-Airframe.AngularDragRollPitch * AngularVelocityBody.X,
		-Airframe.AngularDragRollPitch * AngularVelocityBody.Y,
		-Airframe.AngularDragYaw * AngularVelocityBody.Z);

	Result.ForceWorldN = State.Rotation.RotateVector(ForceBody);
	Result.TorqueWorldNm = State.Rotation.RotateVector(TorqueBody);
	return Result;
}
