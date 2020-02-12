// ----------------------------------------------------------------------------------------------------
//  Title			FPatchSizePreset.h
//  Description		Header file for FPatchSizePreset.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: Ralph Wiedemeier $
//  $Revision: 1 $
//  $Date: 2011/08/14 $
// ----------------------------------------------------------------------------------------------------

#ifndef FPATCHSIZEPRESET_H
#define FPATCHSIZEPRESET_H

#include "FTrackMe.h"

#include <QString>
#include "FArchive.h"

// ----------------------------------------------------------------------------------------------------
//  Class FPatchSizePreset
// ----------------------------------------------------------------------------------------------------

class FPatchSizePreset
{
	//  Public enumerations ----------------------------------------------------

public:
	enum state_t
	{
		PatchSize_32x32,
		PatchSize_48x48,
		PatchSize_64x64,
		PatchSize_96x96,
		PatchSize_128x128,
		PatchSize_256x256,
		OptionCount
	};

	//  Constructors and destructor --------------------------------------------

	FPatchSizePreset() : m_state(PatchSize_64x64) { }
	FPatchSizePreset(state_t state) : m_state(state) { }
	FPatchSizePreset(int option) : m_state((state_t)option) { }

	//  Public queries ---------------------------------------------------------

	/// Returns the patch size as QSize object.
	QSize toSize() const;
	/// Returns a text representation of the object state.
	QString toString() const;
	/// Returns the number of available options.
	static size_t size() { return (size_t)OptionCount; }

	//  Operators --------------------------------------------------------------

	operator state_t() const { return m_state; }

	bool operator==(FPatchSizePreset rhs) const { return m_state == rhs.m_state; }
	bool operator!=(FPatchSizePreset rhs) const { return m_state != rhs.m_state; }

	bool operator==(state_t state) const { return m_state == state; }
	bool operator!=(state_t state) const { return m_state != state; }

	friend FArchive& operator<<(FArchive& ar, FPatchSizePreset obj) {
		ar << (quint8)obj.m_state; return ar; }
	friend FArchive& operator>>(FArchive& ar, FPatchSizePreset& obj) {
		quint8 val; ar >> val; obj.m_state = (state_t)val; return ar; }
	
	//  Internal data members --------------------------------------------------

private:
	state_t m_state;

};


// ----------------------------------------------------------------------------------------------------

#endif // FPATCHSIZEPRESET_H