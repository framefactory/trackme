// ----------------------------------------------------------------------------------------------------
//  Title			FLineModel.h
//  Description		Header file for FLineModel.cpp - Base class for models used for line tracking.
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FLINEMODEL_H
#define FLINEMODEL_H

#include <algorithm>
#include "FTrackMe.h"
#include "FlowGL.h"
#include "FlowMath.h"
#include "FVertex.h"
#include "FPixelStruct.h"

// ----------------------------------------------------------------------------------------------------
//  Class FLineModel
// ----------------------------------------------------------------------------------------------------

/// Base class for models used for edge tracking. Each model consists of a solid mesh representation
/// and a list of edges (the outline). The solid mesh is used by the hidden line suppression.
class FLineModel
{
	//  Private types ----------------------------------------------------------

public:
	struct face_t
	{
	};

	struct candidate_t
	{
		FVector2f position;
		float edgeResponse;
		float colorMatch;
		float signedDistance;
		float absDistance;
		bool isValid;
	};

	struct sample_t
	{
		size_t slotId;
		size_t candidateCount;
		size_t validCandidateCount;
		size_t bestValidCandidate;

		bool isInlier;
		bool wasPresent;

		candidate_t candidates[FGlobalConstants::MAX_CANDIDATES_PER_SAMPLE];
		FPixelRGBA32f refColor0;
		FPixelRGBA32f refColor1;
	};

	struct edge_t
	{
		edge_t() { }

		edge_t(const FVector4f& _start, const FVector4f& _end, const FVector4f& n0,
			const FVector4f& n1, float _samplingDensity = 1.0f)
		{
			modelPoint[0] = _start;
			modelPoint[1] = _end;
			faceNormal[0] = n0;
			faceNormal[1] = n1;
			sampleCount = 0;
			samplingDensity = _samplingDensity;
		}
		
		edge_t(float x0, float y0, float z0, float x1, float y1, float z1) {
			modelPoint[0].set(x0, y0, z0, 1.0f);
			modelPoint[1].set(x1, y1, z1, 1.0f);
			sampleCount = 0;
			samplingDensity = 1.0f;
		}
		
		bool isEqual(const edge_t& other) {
			return ((modelPoint[0] == other.modelPoint[0] && modelPoint[1] == other.modelPoint[1])
				|| (modelPoint[0] == other.modelPoint[1] && modelPoint[1] == other.modelPoint[0]));
		}

		FVector4f modelPoint[2];
		FVector4f homImage[2];
		FVector2f imagePoint[2];
		FVector2f imageNormal;
		FVector4f faceNormal[2];
		FVector4f faceNormalTransformed[2];
		FVector2f faceNormalImageStart;
		FVector2f faceNormalImageEnd[2];
		float samplingDensity;
		size_t sampleCount;
		sample_t samples[FGlobalConstants::MAX_SAMPLES_PER_EDGE];
	};

	struct glLine_t
	{
		glLine_t() { }
		glLine_t(const edge_t& edge) {
			linePoint0.set(edge.modelPoint[0]);
			faceNormal0 = edge.faceNormal[0].toVector3();
			sampleDensity0 = edge.samplingDensity;
			linePoint1.set(edge.modelPoint[1]);
			faceNormal1 = edge.faceNormal[1].toVector3();
			sampleDensity1 = edge.samplingDensity;
		}

		FVector3f linePoint0;
		FVector3f faceNormal0;
		float sampleDensity0;
		FVector3f linePoint1;
		FVector3f faceNormal1;
		float sampleDensity1;
	};

	typedef std::vector<glLine_t> lineVec_t;
	typedef std::vector<edge_t> edgeVec_t;
	typedef std::vector<sample_t> sampleVec_t;
	typedef std::vector<candidate_t> candidateVec_t;
	typedef std::vector<face_t> faceVec_t;
	typedef std::vector<double> doubleVec_t;

	//  Public enumerations ----------------------------------------------------

public:
	enum method_t
	{
		SingleHypothesis,
		MultipleHypotheses,
	};

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FLineModel();
	/// Virtual destructor.
	virtual ~FLineModel();

	//  Public commands --------------------------------------------------------

	/// Sets the method used for candidate evaluation.
	void setMethod(method_t method) { m_method = method; }

	/// Transforms the object to image space using the given MVP matrix.
	void transform(const FMatrix4f& matMV, const FMatrix4f& matMVP);
	
	void calculateHypothesis();
	void markOutliers(float distanceLimit);

	/// Updates the Jacobian matrix based on the current transform matrix.
	void updateJacobian(const FMatrix4f& mvpMatrix);
	
	
	/// Starts to add candidates. Clears the list of edge candidates.
	void beginAddCandidates();
	/// Adds a candidate edge point belonging to the given line index and sample index.
	inline void addCandidate(size_t lineId, size_t sampleId,
		const FVector2f& position, float edgeStrength, float colorDifference);
	/// Adds the reference colors for a sample.
	inline void FLineModel::addSampleColors(size_t edgeId, size_t sampleId,
		const FPixelRGBA32f& color0, const FPixelRGBA32f& color1);
	/// Ends adding candidates and counts the valid samples (samples with one or more candidates found).
	void endAddCandidates();

	/// Draws the solid mesh of the model.
	void drawSolidGL();
	/// Draws the edges of the model.
	void drawLinesGL();
	/// Draws the reprojected edges and candidate points in image space.
	void drawCandidates(FGLCanvas& canvas);
	/// Draws the reference colors for all active samples.
	void drawReferenceColors(FGLCanvas& canvas);

	/// Creates the mesh and line data and the OpenGL resources.
	void create();
	/// Releases all data and all OpenGL resources.
	void release();

	//  Public queries ---------------------------------------------------------

	/// Returns the number of valid data points in the model.
	inline size_t costVectorSize() const { return m_numValidSamples; }

	/// Counts the total number of samples for all edges.
	size_t sampleCount() const;
	
	/// Copies the Jacobian matrix to the given array.
	inline void getJacobian(double* pJac) const {
		for (int i = 0; i < m_numValidSamples * 6; i++)
			pJac[i] = m_jacobian[i];
	}
	/// Copies the cost vector to the given double array.
	inline void getCostVector(double* pCost) const {
		for (size_t i = 0; i < costVectorSize(); i++)
			pCost[i] = m_cost[i];
	}

	/// Copies the residual of valid samples to the given array at the position of their slotIds.
	inline void fillResidualData(float* pResidualData, float maxResidual) const
	{
		float imr = 1.0f / (maxResidual - 1.0f);

		for (size_t e = 0, ne = m_edges.size(); e < ne; e++)
		{
			const edge_t& edge = m_edges[e];
			for (size_t s = 0, ns = edge.sampleCount; s < ns; s++)
			{
				const sample_t& sample = edge.samples[s];

				float residual = 1.0f;

				if (sample.validCandidateCount > 0 && sample.isInlier)
					residual = imr * (sample.candidates[sample.bestValidCandidate].absDistance - 1.0f);

				size_t id = e * FGlobalConstants::MAX_SAMPLES_PER_EDGE + s;
				pResidualData[id] = fMax(0.0f, fMin(1.0f, residual));
			}
		}
	}

	/// Returns the total cost (sum of cost vector).
	inline double totalCost() const {
		double c = 0.0;
		for (size_t i = 0; i < costVectorSize(); i++)
			c += m_absCost[i];
		return c;
	}

	/// Returns the average cost per data point
	inline double averageCost() const {
		return totalCost() / (double)costVectorSize();
	}

	/// Returns the cost total, mean and standard deviation
	inline void getCost(double& median, double& mean, double& stddev) {
		double s = 0.0;
		for (size_t i = 0; i < m_numValidSamples; i++)
			s += m_absCost[i];
		double m = s / (double)costVectorSize();
		double dd = 0.0;
		for (size_t i = 0; i < m_numValidSamples; i++) {
			double d = m_absCost[i] - m; dd += d*d;
		}
		mean = m; stddev = sqrt(dd / (double)costVectorSize());


		size_t mp = costVectorSize() / 2;
		std::nth_element(m_absCost.begin(), m_absCost.begin() + mp, m_absCost.begin() + costVectorSize());
		median = m_absCost[mp];
	}

	/// Returns true if the model has been created successfully.
	bool isValid() const { return m_solidMesh.isValid(); }

	/// Const access to the OpenGL line array.
	const lineVec_t& lines() const { return m_lines; }
	/// Const access to the edge array.
	const edgeVec_t& edges() const { return m_edges; }

	/// The number of edges/lines of the model.
	size_t edgeCount() const { return m_edges.size(); }

#ifdef QT_DEBUG
	/// Writes information about the internal state to the given debug object.
	virtual void dump(QDebug& debug) const;
#endif

	//  Overridables -----------------------------------------------------------

protected:
	/// Override to create mesh data for the given FGLMesh object.
	virtual void onCreateSolidMesh(FGLMesh& solidMesh) = 0;
	/// Override to fill a list with the model's edge segments.
	virtual void onCreateEdges(edgeVec_t& edges) = 0;
	/// Override to fill a list with the model's faces.
	virtual void onCreateFaces(faceVec_t& faces) = 0;

	//  Internal functions -----------------------------------------------------

private:
	void _calculateSingleHypothesis();
	void _calculateBestHypothesis();
	void _markOutliersSingleHypothesis(float distanceLimit);
	void _markOutliersBestHypothesis(float distanceLimit);

	void _updateJacobian(const FMatrix4f& mvp);

	inline FVector2f _homogenizeDerivative(const FVector4f& p, const FVector4f& dp)
	{
		FVector2f h;
		float ww = p.w() * p.w();
		h.setX(dp.x() / p.w() - p.x() * dp.w() / ww);
		h.setY(dp.y() / p.w() - p.y() * dp.w() / ww);
		return h;
	}

	//  Internal data members --------------------------------------------------

private:
	FGLMesh        m_solidMesh;
	FGLVertexArray m_lineArray;
	FGLBuffer      m_lineBuffer;
	
	lineVec_t      m_lines;
	edgeVec_t      m_edges;
	faceVec_t      m_faces;

	size_t         m_numValidSamples;
	doubleVec_t    m_jacobian;
	doubleVec_t    m_cost;
	doubleVec_t	   m_absCost;

	method_t       m_method;
};

// Inline members -------------------------------------------------------------------------------------

void FLineModel::addCandidate(size_t edgeId, size_t sampleId,
							  const FVector2f& position, float edgeStrength, float colorMatch)
{
	F_ASSERT(edgeId < m_edges.size());
	F_ASSERT(sampleId < FGlobalConstants::MAX_SAMPLES_PER_EDGE);

	edge_t& edge = m_edges[edgeId];
	sample_t& sample = edge.samples[sampleId];
	sample.slotId = edgeId * FGlobalConstants::MAX_SAMPLES_PER_EDGE + sampleId;
	edge.sampleCount = fMax(edge.sampleCount, sampleId + 1);

	if (sample.candidateCount >= FGlobalConstants::MAX_CANDIDATES_PER_SAMPLE)
	{
		F_TRACE("FLineModel::addCandidate - Maximum number of candidates reached");
		return;
	}

	size_t cId = sample.candidateCount;
	sample.candidateCount++;
	candidate_t& cand = sample.candidates[cId];

	cand.position = position;
	cand.edgeResponse = edgeStrength;
	cand.colorMatch = colorMatch;

	if (!sample.wasPresent || cand.colorMatch > 0.0f)
	{
		cand.isValid = true;
		sample.validCandidateCount++;
	}
	else
		cand.isValid = false;
}

void FLineModel::addSampleColors(size_t edgeId, size_t sampleId,
								 const FPixelRGBA32f& color0, const FPixelRGBA32f& color1)
{
	F_ASSERT(edgeId < m_edges.size());
	F_ASSERT(sampleId < FGlobalConstants::MAX_SAMPLES_PER_EDGE);

	sample_t& sample = m_edges[edgeId].samples[sampleId];

	sample.refColor0 = color0;
	sample.refColor1 = color1;
}
	
// ----------------------------------------------------------------------------------------------------

#endif // FLINEMODEL_H