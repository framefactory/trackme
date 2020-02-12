// ----------------------------------------------------------------------------------------------------
//  Title			FPatchSizePreset.cpp
//  Description		Implementation of class FPatchSizePreset
// ----------------------------------------------------------------------------------------------------
//  $Author: Ralph Wiedemeier $
//  $Revision: 1 $
//  $Date: 2011/08/14 $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FPatchSizePreset.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FPatchSizePreset
// ----------------------------------------------------------------------------------------------------

// Public queries -------------------------------------------------------------------------------------

QString FPatchSizePreset::toString() const
{
	switch (m_state)
	{
	case PatchSize_32x32: return "32 x 32"; 
	case PatchSize_48x48: return "48 x 48";
	case PatchSize_64x64: return "64 x 64";
	case PatchSize_96x96: return "96 x 96";
	case PatchSize_128x128: return "128 x 128";
	case PatchSize_256x256: return "256 x 256";
	default: assert(false); return "UNKNOWN";
	}
}

QSize FPatchSizePreset::toSize() const
{
	switch (m_state)
	{
	case PatchSize_32x32:   return QSize(32, 32);
	case PatchSize_48x48:   return QSize(48, 48);
	case PatchSize_64x64:   return QSize(64, 64);
	case PatchSize_96x96:   return QSize(96, 96);
	case PatchSize_128x128: return QSize(128, 128);
	case PatchSize_256x256: return QSize(256, 256);
	default: assert(false); return QSize();
	}
}
// ----------------------------------------------------------------------------------------------------