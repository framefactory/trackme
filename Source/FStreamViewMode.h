// ----------------------------------------------------------------------------------------------------
//  Title			FStreamViewMode.h
//  Description		Header file for FStreamViewMode.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 9 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FSTREAMVIEWMODE_H
#define FSTREAMVIEWMODE_H

#include <QString>
#include "FTrackMe.h"
#include "FArchive.h"

// ----------------------------------------------------------------------------------------------------
//  Class FStreamViewMode
// ----------------------------------------------------------------------------------------------------

class FStreamViewMode
{
	//  Public enumerations ----------------------------------------------------

public:
	enum state_t
	{
		Input,
		SolidModel,
		DepthPass,
		InitialModel,
		FittedModel,
		ReferenceColors,
		AugmentedImage,
		CannyEdges,
		DistanceTransform,
		Contours,
		DetectedPose,
		HarrisCorners
	};

	static const size_t optionCount = 12;

	static FStreamViewMode optionAt(size_t index) {
		F_ASSERT(index < optionCount); return FStreamViewMode((state_t)index);
	}

	//  Constructors and destructor --------------------------------------------

	FStreamViewMode() : m_state(Input) { }
	FStreamViewMode(state_t state) : m_state(state) { }

	//  Public queries ---------------------------------------------------------

	/// Returns a text representation of the object state.
	inline QString toString() const;

	//  Operators --------------------------------------------------------------

	operator state_t() const { return m_state; }

	bool operator==(FStreamViewMode rhs) const { return m_state == rhs.m_state; }
	bool operator!=(FStreamViewMode rhs) const { return m_state != rhs.m_state; }

	bool operator==(state_t state) const { return m_state == state; }
	bool operator!=(state_t state) const { return m_state != state; }

	friend FArchive& operator<<(FArchive& ar, FStreamViewMode obj) {
		ar << (quint8)obj.m_state; return ar; }
	friend FArchive& operator>>(FArchive& ar, FStreamViewMode& obj) {
		quint8 val; ar >> val; obj.m_state = (state_t)val; return ar; }
	
	//  Internal data members --------------------------------------------------

private:
	state_t m_state;

};

// ----------------------------------------------------------------------------------------------------

QString FStreamViewMode::toString() const
{
	switch (m_state)
	{
		case Input:				return "Input Image";
		case SolidModel:		return "Solid Model";
		case DepthPass:			return "Depth Pass";
		case InitialModel:		return "Initial Model";
		case FittedModel:		return "Fitted Model";
		case ReferenceColors:	return "Reference Colors";
		case AugmentedImage:	return "Augmented Image";
		case CannyEdges:		return "Canny Edges";
		case DistanceTransform: return "Distance Transform";
		case Contours:			return "Contours";
		case DetectedPose:		return "Detected Pose";
		case HarrisCorners:		return "Harris Corners";

		default: assert(false); return "UNKNOWN";
	}
}
	
// ----------------------------------------------------------------------------------------------------


#endif // FSTREAMVIEWMODE_H