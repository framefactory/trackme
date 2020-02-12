// ----------------------------------------------------------------------------------------------------
//  Title			FGenericModel.h
//  Description		Header file for FGenericModel.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-30 20:28:52 +0200 (Di, 30 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FGENERICMODEL_H
#define FGENERICMODEL_H

#include "FTrackMe.h"
#include "FMatrix4T.h"
#include "FLineModel.h"

class aiScene;
class aiNode;

// ----------------------------------------------------------------------------------------------------
//  Class FGenericModel
// ----------------------------------------------------------------------------------------------------

class FGenericModel : public FLineModel
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FGenericModel();
	/// Virtual destructor.
	virtual ~FGenericModel();

	//  Public commands --------------------------------------------------------

public:
	/// Tries to load separate solid and edge models by changing the file name
	/// to *_edges.* and *_solid.*. If this fails, loads the same model twice
	/// for solid and edge representation.
	void createImportModel(const QString& fileName);

	/// Sets the file name for the solid model file.
	void setSolidModelFile(const QString& fileName);
	/// Sets the file name for the edge model file.
	void setEdgeModelFile(const QString& fileName);

	//  Public queries ---------------------------------------------------------

	const QString& solidModelFile() const { return m_solidModelFile; }
	const QString& edgeModelFile() const { return m_edgeModelFile; }

	//  Overrides --------------------------------------------------------------

protected:
	virtual void onCreateSolidMesh(FGLMesh& solidMesh);
	virtual void onCreateEdges(edgeVec_t& edges);
	virtual void onCreateFaces(faceVec_t& faces);

	//  Internal functions -----------------------------------------------------

private:
	bool _importLineModel(const QString& fileName, edgeVec_t& edges);
	void _getEdgesFromNode(const aiScene* pScene, const aiNode* pNode,
		FMatrix4f trafo, edgeVec_t& edges);

	//  Internal data members --------------------------------------------------

private:
	QString m_solidModelFile;
	QString m_edgeModelFile;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FGENERICMODEL_H