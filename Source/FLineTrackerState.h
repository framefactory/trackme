// ----------------------------------------------------------------------------------------------------
//  Title			FLineTrackerState.h
//  Description		Header file for FLineTrackerState.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: Ralph Wiedemeier $
//  $Revision: 1 $
//  $Date: 2011/08/16 $
// ----------------------------------------------------------------------------------------------------

#ifndef FLINETRACKERSTATE_H
#define FLINETRACKERSTATE_H

#include "FTrackMe.h"

#include <QString>
#include "FArchive.h"

// ----------------------------------------------------------------------------------------------------
//  Class FLineTrackerState
// ----------------------------------------------------------------------------------------------------

class FLineTrackerState
{
	//  Public enumerations ----------------------------------------------------

public:
	enum state_t
	{
		Disabled,
		Initializing,
		Tracking,
		Failed
	};

	//  Constructors and destructor --------------------------------------------

	FLineTrackerState() : m_state(Disabled) { }
	FLineTrackerState(state_t state) : m_state(state) { }

	//  Public queries ---------------------------------------------------------

	/// Returns a text representation of the object state.
	inline QString toString() const;

	//  Operators --------------------------------------------------------------

	operator state_t() const { return m_state; }

	bool operator==(FLineTrackerState rhs) const { return m_state == rhs.m_state; }
	bool operator!=(FLineTrackerState rhs) const { return m_state != rhs.m_state; }

	bool operator==(state_t state) const { return m_state == state; }
	bool operator!=(state_t state) const { return m_state != state; }

	friend FArchive& operator<<(FArchive& ar, FLineTrackerState obj) {
		ar << (quint8)obj.m_state; return ar; }
	friend FArchive& operator>>(FArchive& ar, FLineTrackerState& obj) {
		quint8 val; ar >> val; obj.m_state = (state_t)val; return ar; }
	
	//  Internal data members --------------------------------------------------

private:
	state_t m_state;

};

// ----------------------------------------------------------------------------------------------------

QString FLineTrackerState::toString() const
{
	switch (m_state)
	{
		case Disabled:					return "Disabled";
		case Initializing:				return "Initializing";
		case Tracking:					return "Tracking";
		case Failed:					return "Failed";
		default: assert(false);			return "UNKNOWN";
	}
}
	
// ----------------------------------------------------------------------------------------------------

#endif // FLINETRACKERSTATE_H