// ----------------------------------------------------------------------------------------------------
//  Title			FBoxModel.cpp
//  Description		Implementation of class FBoxModel
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 9 $
//  $Date: 2011-08-30 20:28:52 +0200 (Di, 30 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FBoxModel.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FBoxModel
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FBoxModel::FBoxModel()
: m_size(1.0f, 1.0f, 1.0f)
{
}

FBoxModel::~FBoxModel()
{
}

// Public commands ------------------------------------------------------------------------------------

void FBoxModel::setSize(float width, float height, float depth)
{
	m_size.set(width, height, depth);
}

// Overrides ------------------------------------------------------------------------------------------

void FBoxModel::onCreateSolidMesh(FGLMesh& solidMesh)
{
	FMeshBox m_box;
	m_box.setSize(m_size);
	m_box.setPivot(m_size * 0.5f);
	m_box.createMesh();

	solidMesh.create(m_box);
}

void FBoxModel::onCreateEdges(edgeVec_t& edges)
{
	float x0 = m_size.x() * -0.5f;
	float y0 = m_size.y() * -0.5f;
	float z0 = m_size.z() * -0.5f;
	float x1 = -x0;
	float y1 = -y0;
	float z1 = -z0;

	edges.push_back(edge_t(x0, y1, z0, x1, y1, z0));
	edges.push_back(edge_t(x1, y1, z0, x1, y0, z0));
	edges.push_back(edge_t(x1, y0, z0, x0, y0, z0));
	edges.push_back(edge_t(x0, y0, z0, x0, y1, z0));
	edges.push_back(edge_t(x0, y1, z1, x1, y1, z1));
	edges.push_back(edge_t(x1, y1, z1, x1, y0, z1));
	edges.push_back(edge_t(x1, y0, z1, x0, y0, z1));
	edges.push_back(edge_t(x0, y0, z1, x0, y1, z1));
	edges.push_back(edge_t(x0, y0, z0, x0, y0, z1));
	edges.push_back(edge_t(x1, y0, z0, x1, y0, z1));
	edges.push_back(edge_t(x1, y1, z0, x1, y1, z1));
	edges.push_back(edge_t(x0, y1, z0, x0, y1, z1));

	/*
	for (size_t e = 0; e < edges.size(); e++)
	{
		edge_t& ed = edges[e];
		F_TRACE(QString("#%1: %2 - %3")
			.arg(e).arg(ed.modelPoint[0].toString()).arg(ed.modelPoint[1].toString()));
	}
	*/
}

void FBoxModel::onCreateFaces(faceVec_t& faces)
{
}

// ----------------------------------------------------------------------------------------------------