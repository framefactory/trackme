// ----------------------------------------------------------------------------------------------------
//  Title			FBoxModel.h
//  Description		Header file for FBoxModel.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 5 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FBOXMODEL_H
#define FBOXMODEL_H

#include "FTrackMe.h"
#include "FLineModel.h"

// ----------------------------------------------------------------------------------------------------
//  Class FBoxModel
// ----------------------------------------------------------------------------------------------------

class FBoxModel : public FLineModel
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FBoxModel();
	/// Virtual destructor.
	virtual ~FBoxModel();

	//  Public commands --------------------------------------------------------

public:
	/// Sets the dimensions of the box.
	void setSize(float width, float height, float depth);

	//  Public queries ---------------------------------------------------------

	/// Returns the dimensions of the box.
	const FVector3f& size() const { return m_size; }

	//  Overrides --------------------------------------------------------------

protected:
	virtual void onCreateSolidMesh(FGLMesh& solidMesh);
	virtual void onCreateEdges(edgeVec_t& edges);
	virtual void onCreateFaces(faceVec_t& faces);

	//  Internal data members --------------------------------------------------

private:
	FVector3f m_size;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FBOXMODEL_H