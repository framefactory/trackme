// ----------------------------------------------------------------------------------------------------
//  Title			FCameraPose.cpp
//  Description		Implementation of class FCameraPose
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-14 21:08:00 +0200 (So, 14 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"
#include "FQuaternionT.h"
#include "FMath.h"

#include "FCameraPose.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FCameraPose
// ----------------------------------------------------------------------------------------------------

// Public commands ------------------------------------------------------------------------------------

void FCameraPose::setRotationEuler(const FVector3f& yawPitchRoll, bool inverseOrder /* = false */)
{
	FQuaternion4f rYaw  (FVector3f::unitY, yawPitchRoll.x());
	FQuaternion4f rPitch(FVector3f::unitX, yawPitchRoll.y());
	FQuaternion4f rRoll (FVector3f::unitZ, yawPitchRoll.z());

	FQuaternion4f rq = inverseOrder ? (rYaw * rPitch * rRoll) : (rRoll * rPitch * rYaw);

	float angle = rq.angle();
	if (angle > (float)FMath::pi)
		setRotation(-rq.axis() * ((float)FMath::pi2 - angle));
	else
		setRotation(rq.axis() * angle);
}

void FCameraPose::setRotationFromMatrix(const FMatrix3f& matRotation)
{
	FQuaternion4f rq;
	matRotation.getRotation(rq);

	float angle = rq.angle();
	if (angle > (float)FMath::pi)
		setRotation(-rq.axis() * ((float)FMath::pi2 - angle));
	else
		setRotation(rq.axis() * angle);
}

// Public queries -------------------------------------------------------------------------------------

void FCameraPose::getModelViewMatrix(FMatrix4f& modelViewMatrix) const
{
	FVector3f r = rotation();
	float a = r.length();
	if (a != 0.0f)
		r /= a;

	modelViewMatrix.makeRotation(r, a);
	modelViewMatrix.setTranslation(translation());
}

void FCameraPose::getProjectivePoseMatrix(FMatrix3f& projectivePoseMatrix) const
{
	FVector3f r = rotation();
	float a = r.length();
	if (a != 0.0f)
		r /= a;

	FMatrix4f matMV;
	matMV.makeRotation(r, a);

	projectivePoseMatrix(0, 0) = matMV(0, 0);
	projectivePoseMatrix(0, 1) = matMV(0, 1);
	projectivePoseMatrix(1, 0) = matMV(1, 0);
	projectivePoseMatrix(1, 1) = matMV(1, 1);
	projectivePoseMatrix(2, 0) = matMV(2, 0);
	projectivePoseMatrix(2, 1) = matMV(2, 1);
	projectivePoseMatrix(0, 2) = translation().x();
	projectivePoseMatrix(1, 2) = translation().y();
	projectivePoseMatrix(2, 2) = translation().z();
}

void FCameraPose::dump(QDebug& debug) const
{
	debug << "\n----- FCameraPose -----";
}

// ----------------------------------------------------------------------------------------------------