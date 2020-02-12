// ----------------------------------------------------------------------------------------------------
//  Title			FCameraPose.h
//  Description		Header file for FCameraPose.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-14 21:08:00 +0200 (So, 14 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FCAMERAPOSE_H
#define FCAMERAPOSE_H

#include "FTrackMe.h"
#include "FVectorT.h"
#include "FMatrix3T.h"
#include "FMatrix4T.h"
#include "FArchive.h"

// ----------------------------------------------------------------------------------------------------
//  Class FCameraPose
// ----------------------------------------------------------------------------------------------------

class FCameraPose
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FCameraPose() {
		for (size_t i = 0; i < NUM_COMPONENTS; i++)
			m_data[i] = 0.0f;
	}

	//  Public commands --------------------------------------------------------

public:
	/// Sets the 3D translation component of the object pose.
	void setTranslation(const FVector3f& translation) {
		m_data[0] = translation.x(); m_data[1] = translation.y(); m_data[2] = translation.z();
	}

	/// Sets the 3D rotation component (exponential map format) of the object pose.
	void setRotation(const FVector3f& rotation) {
		m_data[3] = rotation.x(); m_data[4] = rotation.y(); m_data[5] = rotation.z();
	}
	/// Sets the focal distance of the camera.
	void setFocalLength(float focalLength) {
		m_data[6] = focalLength;
	}

	/// Sets the 3D rotation component from the given Euler angles.
	void setRotationEuler(const FVector3f& yawPitchRoll, bool inverseOrder = false);
	/// Sets the 3D rotation component from the given rotation matrix.
	void setRotationFromMatrix(const FMatrix3f& matRotation);

	/// Replaces the pose data by the values of the given float array.
	void copyFrom(float* pPoseArray) {
		for (size_t i = 0; i < NUM_COMPONENTS; i++)
			m_data[i] = pPoseArray[i];
	}
	/// Replaces the pose data by the values of the given double array.
	void copyFrom(double* pPoseArray) {
		for (size_t i = 0; i < NUM_COMPONENTS; i++)
			m_data[i] = (float)pPoseArray[i];
	}

	//  Serialization operators ------------------------------------------------

	/// Serialization: write operator.
	friend FArchive& operator<<(FArchive& ar, const FCameraPose& obj);
	/// Serialization: read operator.
	friend FArchive& operator>>(FArchive& ar, FCameraPose& obj);

	//  Public queries ---------------------------------------------------------

	/// Returns the number of data components of the pose.
	static const size_t count() { return NUM_COMPONENTS; }

	/// Returns the translation component of the object pose.
	FVector3f translation() const { return FVector3f(m_data[0], m_data[1], m_data[2]); }
	/// Returns the rotation component (exp. map. format) of the object pose.
	FVector3f rotation() const { return FVector3f(m_data[3], m_data[4], m_data[5]); }
	/// Returns the focal length component of the camera.
	float focalLength() const { return m_data[6]; }

	/// Returns a pointer to an array of 7 float values representing the object pose.
	float* data() {return &m_data[0]; }

	/// Copies the object pose data to the given float array.
	void copyTo(float* pPoseArray) const {
		for (size_t i = 0; i < NUM_COMPONENTS; i++)
			pPoseArray[i] = m_data[i];
	}
	/// Copies the object pose data to the given double array.
	void copyTo(double* pPoseArray) const {
		for (size_t i = 0; i < NUM_COMPONENTS; i++)
			pPoseArray[i] = (double)m_data[i];
	}

	/// Generates a 4x4 model view matrix from the pose.
	void getModelViewMatrix(FMatrix4f& modelViewMatrix) const;
	/// Generates a 3x3 projective model view matrix from the pose.
	void getProjectivePoseMatrix(FMatrix3f& reducedPoseMatrix) const;

	/// Writes information about the internal state to the given debug object.
	void dump(QDebug& debug) const;

	//  Internal data members --------------------------------------------------

private:
	static const size_t NUM_COMPONENTS = 7;
	float m_data[NUM_COMPONENTS];
};

// Serialization operators ----------------------------------------------------------------------------

inline FArchive& operator<<(FArchive& ar, const FCameraPose& obj)
{
	for (size_t i = 0; i < obj.NUM_COMPONENTS; i++)
		ar << obj.m_data[i];

	return ar;
}

inline FArchive& operator>>(FArchive& ar, FCameraPose& obj)
{
	for (size_t i = 0; i < obj.NUM_COMPONENTS; i++)
		ar >> obj.m_data[i];

	return ar;
}

// ----------------------------------------------------------------------------------------------------


#endif // FCAMERAPOSE_H