// ----------------------------------------------------------------------------------------------------
//  Title			FStudioModelVirtEco.h
//  Description		Header file for FStudioModelVirtEco.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 5 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FSTUDIOMODELVIRTECO_H
#define FSTUDIOMODELVIRTECO_H

#include "FTrackMe.h"
#include "FLineModel.h"

// ----------------------------------------------------------------------------------------------------
//  Class FStudioModelVirtEco
// ----------------------------------------------------------------------------------------------------

class FStudioModelVirtEco : public FLineModel
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FStudioModelVirtEco();
	/// Virtual destructor.
	virtual ~FStudioModelVirtEco();

	//  Public commands --------------------------------------------------------

public:
	void setOffset(const FVector3f& offset);

	//  Public queries ---------------------------------------------------------

	const FVector3f& offset() const { return m_offset; }

	//  Overrides --------------------------------------------------------------

protected:
	virtual void onCreateSolidMesh(FGLMesh& solidMesh);
	virtual void onCreateEdges(edgeVec_t& edges);
	virtual void onCreateFaces(faceVec_t& faces);

	//  Internal data members --------------------------------------------------

private:
	FVector3f m_offset;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FSTUDIOMODELVIRTECO_H