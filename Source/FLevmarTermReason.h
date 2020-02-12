// ----------------------------------------------------------------------------------------------------
//  Title			FLevmarTermReason.h
//  Description		Header file for FLevmarTermReason.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FLEVMARTERMREASON_H
#define FLEVMARTERMREASON_H

#include "FTrackMe.h"

#include <QString>
#include "FArchive.h"

// ----------------------------------------------------------------------------------------------------
//  Class FLevmarTermReason
// ----------------------------------------------------------------------------------------------------

class FLevmarTermReason
{
	//  Public enumerations ----------------------------------------------------

public:
	enum state_t
	{
		Unknown				= 0,
		SmallGradient		= 1,
		SmallDeltaP			= 2,
		MaxInterations		= 3,
		SingularMatrix		= 4,
		NoFurtherReduction	= 5,
		SmallError			= 6,
		InvalidFuncValues	= 7
	};

	//  Constructors and destructor --------------------------------------------

	FLevmarTermReason() : m_state(Unknown) { }
	FLevmarTermReason(state_t state) : m_state(state) { }
	explicit FLevmarTermReason(int state) : m_state((state_t)state) { }

	//  Public queries ---------------------------------------------------------

	/// Returns a text representation of the object state.
	inline QString toString() const;

	//  Operators --------------------------------------------------------------

	operator state_t() const { return m_state; }

	bool operator==(FLevmarTermReason rhs) const { return m_state == rhs.m_state; }
	bool operator!=(FLevmarTermReason rhs) const { return m_state != rhs.m_state; }

	bool operator==(state_t state) const { return m_state == state; }
	bool operator!=(state_t state) const { return m_state != state; }

	friend FArchive& operator<<(FArchive& ar, FLevmarTermReason obj) {
		ar << (quint8)obj.m_state; return ar; }
	friend FArchive& operator>>(FArchive& ar, FLevmarTermReason& obj) {
		quint8 val; ar >> val; obj.m_state = (state_t)val; return ar; }
	
	//  Internal data members --------------------------------------------------

private:
	state_t m_state;

};

// ----------------------------------------------------------------------------------------------------

QString FLevmarTermReason::toString() const
{
	switch (m_state)
	{
		case Unknown:				return "Unknown";
		case SmallGradient:			return "Small gradient";
		case SmallDeltaP:			return "Small dp";
		case MaxInterations:		return "Maximum iterations";
		case SingularMatrix:		return "Singular matrix";
		case NoFurtherReduction:	return "No further improvement";
		case SmallError:			return "Small error";
		case InvalidFuncValues:		return "Invalid function values";

		default: assert(false); return "UNKNOWN";
	}
}
	
// ----------------------------------------------------------------------------------------------------

#endif // FLEVMARTERMREASON_H