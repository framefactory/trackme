// ----------------------------------------------------------------------------------------------------
//  Title			FLineModel.cpp
//  Description		Implementation of class FLineModel
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-10 22:55:38 +0200 (Sa, 10 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FLineModel.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FLineModel
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FLineModel::FLineModel()
: m_numValidSamples(0),
  m_method(MultipleHypotheses)
{
	// clear candidate list
	for (size_t e = 0, ne = m_edges.size(); e < ne; e++)
	{
		m_edges[e].sampleCount = 0;
		for (size_t s = 0; s < FGlobalConstants::MAX_SAMPLES_PER_EDGE; s++)
		{
			sample_t& sample = m_edges[e].samples[s];

			sample.candidateCount = 0;
			sample.validCandidateCount = 0;
			sample.bestValidCandidate = 0;
			sample.isInlier = true;
			sample.wasPresent = false;
		}
	}
}

FLineModel::~FLineModel()
{
}

// Public commands ------------------------------------------------------------------------------------

void FLineModel::transform(const FMatrix4f& matMV, const FMatrix4f& matMVP)
{
	for (size_t i = 0, ni = m_edges.size(); i < ni; i++)
	{
		// transform line start and end points and calculate line normal
		edge_t& edge = m_edges[i];
		
		FVector4f mp0h = matMVP * edge.modelPoint[0];
		FVector4f mp1h = matMVP * edge.modelPoint[1];
	
		edge.imagePoint[0].set(mp0h.x() / mp0h.w(), mp0h.y() / mp0h.w());
		edge.imagePoint[1].set(mp1h.x() / mp1h.w(), mp1h.y() / mp1h.w());
		edge.imageNormal = (edge.imagePoint[1] - edge.imagePoint[0]).normal().normalize();
		edge.homImage[0] = mp0h;
		edge.homImage[1] = mp1h;

		edge.faceNormalTransformed[0] = matMV * edge.faceNormal[0];
		edge.faceNormalTransformed[1] = matMV * edge.faceNormal[1];

		FVector4f ctr = (edge.modelPoint[0] + edge.modelPoint[1]) * 0.5f;
		FVector4f pn0 = matMVP * (ctr + edge.faceNormal[0] * 2.0f);
		FVector4f pn1 = matMVP * (ctr + edge.faceNormal[1] * 2.0f);
		ctr = matMVP * ctr;
		edge.faceNormalImageStart.set(ctr.x() / ctr.w(), ctr.y() / ctr.w());
		edge.faceNormalImageEnd[0].set(pn0.x() / pn0.w(), pn0.y() / pn0.w());
		edge.faceNormalImageEnd[1].set(pn1.x() / pn1.w(), pn1.y() / pn1.w());
	}
}

void FLineModel::calculateHypothesis()
{
	if (m_method == SingleHypothesis)
		_calculateSingleHypothesis();
	else
		_calculateBestHypothesis();
}

void FLineModel::markOutliers(float distanceLimit)
{
	m_numValidSamples = 0;

	for (size_t e = 0, ne = m_edges.size(); e < ne; e++)
	{
		edge_t& edge = m_edges[e];

		for (size_t s = 0, ns = edge.sampleCount; s < ns; s++)
		{
			sample_t& sample = edge.samples[s];
			if (sample.validCandidateCount > 0)
			{
				candidate_t& cand = sample.candidates[sample.bestValidCandidate];
				F_ASSERT(cand.isValid);

				if (cand.absDistance < distanceLimit)
				{
					sample.isInlier = true;
					m_cost[m_numValidSamples] = cand.signedDistance;
					m_absCost[m_numValidSamples] = cand.absDistance;
					m_numValidSamples++;
				}
				else
				{
					sample.isInlier = false;
				}
			}
		}
	}
}

void FLineModel::updateJacobian(const FMatrix4f& mvpMatrix)
{
	calculateHypothesis();
	_updateJacobian(mvpMatrix);
}

void FLineModel::beginAddCandidates()
{
	// clear candidate list
	for (size_t e = 0, ne = m_edges.size(); e < ne; e++)
	{
		m_edges[e].sampleCount = 0;
		for (size_t s = 0; s < FGlobalConstants::MAX_SAMPLES_PER_EDGE; s++)
		{
			sample_t& sample = m_edges[e].samples[s];

			sample.isInlier = true;
			sample.wasPresent = (sample.candidateCount > 0 && sample.isInlier);

			sample.candidateCount = 0;
			sample.validCandidateCount = 0;
			sample.bestValidCandidate = 0;
		}
	}
}

void FLineModel::endAddCandidates()
{
	m_numValidSamples = 0;

	for (size_t e = 0, ne = m_edges.size(); e < ne; e++)
	{
		edge_t& edge = m_edges[e];
		for (size_t s = 0, ns = edge.sampleCount; s < ns; s++)
		{
			sample_t& sample = edge.samples[s];
			if (sample.validCandidateCount > 0)
				m_numValidSamples++;
		}
	}

	//F_TRACE(QString("FLineModel::endAddCandidates - Data count: %1").arg(m_validDataCount));
}

void FLineModel::drawSolidGL()
{
	F_ASSERT(m_solidMesh.isValid());
	if (!m_solidMesh.isValid())
		return;

	m_solidMesh.draw();
}

void FLineModel::drawLinesGL()
{
	F_ASSERT(m_lineArray.isValid());
	if (!m_lineArray.isValid())
		return;

	size_t numVertices = m_lines.size() * 2;
	m_lineArray.draw(FGLPrimitiveType::Lines, 0, numVertices);
}

void FLineModel::drawCandidates(FGLCanvas& canvas)
{
	for (size_t e = 0, ne = m_edges.size(); e < ne; e++)
	{
		// draw edge line
		edge_t& edge = m_edges[e];
		FLine2f line(edge.imagePoint[0], edge.imagePoint[1]);
		FColor col = edge.sampleCount ? FColor(1.0f, 0.0f, 0.3f) : FColor(0.1f, 0.2f, 0.8f);
		canvas.addLine(line, col);

		// draw adjacent face normals
		FLine2f normalLine0(edge.faceNormalImageStart, edge.faceNormalImageEnd[0]);
		canvas.addLine(normalLine0, FColor(1.0f, 0.3f, 0.8f));
		FLine2f normalLine1(edge.faceNormalImageStart, edge.faceNormalImageEnd[1]);
		canvas.addLine(normalLine1, FColor(1.0f, 0.3f, 0.8f));


		FVector2f direction = line.direction().normalize();

		for (size_t s = 0, ns = edge.sampleCount; s < ns; s++)
		{
			sample_t& sample = edge.samples[s];
			for (size_t c = 0, nc = sample.candidateCount; c < nc; c++)
			{
				candidate_t& cand = sample.candidates[c];

				float t = direction * (cand.position - edge.imagePoint[0]);
				canvas.addLine(FLine2f(edge.imagePoint[0] + t * direction, cand.position), FColor::colorLightGray);
				canvas.addMarker(edge.imagePoint[0] + t * direction, 1.5f, FColor::colorLightGray);

				FColor col = (sample.bestValidCandidate == c) ? FColor(1.0f, 0.8f, 0.0f) : FColor(0.3f, 0.8f, 0.0f);
				if (cand.colorMatch == 0.0f)
					col = FColor(0.0f, 0.5f, 1.0f);
				else if (cand.colorMatch == -1.0f)
					col = FColor(1.0f, 0.2f, 0.8f);

				if (!sample.wasPresent)
					col = FColor(1.0f, 0.6f, 0.6f);
				if (!sample.isInlier)
					col = FColor(0.5f, 0.3f, 0.6f);

				canvas.addMarker(cand.position.x(), cand.position.y(), 1.5f, col);
			}
		}
	}
}

void FLineModel::drawReferenceColors(FGLCanvas& canvas)
{
	for (size_t i = 0, ni = m_edges.size(); i < ni; i++)
	{
		edge_t& edge = m_edges[i];
		if (edge.sampleCount > 0)
		{
			FLine2f line(edge.imagePoint[0], edge.imagePoint[1]);
			canvas.addLine(line, FColor::colorLightGray);
			FVector2f direction = line.direction().normalize();

			for (size_t s = 0, ns = edge.sampleCount; s < ns; s++)
			{
				sample_t& sample = edge.samples[s];
				if (sample.validCandidateCount > 0 /* && sample.isInlier */)
				{
					candidate_t& cand = sample.candidates[sample.bestValidCandidate];
					float t = direction * (cand.position - edge.imagePoint[0]);
					FVector2f base = edge.imagePoint[0] + t * direction;
					FVector2f p0 = base + edge.imageNormal * 10.0f;
					FVector2f p1 = base - edge.imageNormal * 10.0f;
					canvas.addLine(FLine2f(p0, p1), FColor::colorLightGray);

					FColor c0 = sample.refColor0.toColor();
					FColor s0 = sample.refColor0.a < 1.0f ? FColor::colorBlack : FColor::colorWhite;
					c0.setAlpha(1.0f);
					canvas.addMarker(p0, 4.0f, c0);
					FRect2f r0(p0.x() - 4.0f, p0.y() - 4.0f, p0.x() + 4.0f, p0.y() + 4.0f);
					canvas.addRectangle(r0, s0);

					FColor c1 = sample.refColor1.toColor();
					FColor s1 = sample.refColor1.a < 1.0f ? FColor::colorBlack : FColor::colorWhite;
					c1.setAlpha(1.0f);
					canvas.addMarker(p1, 4.0f, c1);
					FRect2f r1(p1.x() - 4.0f, p1.y() - 4.0f, p1.x() + 4.0f, p1.y() + 4.0f);
					canvas.addRectangle(r1, s1);
				}
			}
		}
	}
}



void FLineModel::create()
{
	release();

	onCreateSolidMesh(m_solidMesh);
	onCreateEdges(m_edges);

	F_ASSERT(m_solidMesh.isValid());
	F_ASSERT(!m_edges.empty());

	size_t maxDataCount = m_edges.size()
		* FGlobalConstants::MAX_SAMPLES_PER_EDGE
		* FGlobalConstants::MAX_CANDIDATES_PER_SAMPLE;
	m_cost.resize(maxDataCount);
	m_absCost.resize(maxDataCount);
	m_jacobian.resize(maxDataCount * 6);

	// Create line list for OpenGL
	m_lines.resize(m_edges.size());
	for (size_t i = 0, n = m_edges.size(); i < n; i++)
		m_lines[i] = glLine_t(m_edges[i]);

	// Create OpenGL objects for drawing solid and hidden line geometry
	size_t numVertices = m_lines.size() * 2;
	size_t numBytes = m_lines.size() * sizeof(glLine_t);

	m_lineBuffer.createInitialize(&m_lines.front(), numBytes, FGLUsage::StaticDraw);

	size_t stride = sizeof(glLine_t) / 2;
	m_lineArray.create();
	m_lineArray.bindVertexBuffer(m_lineBuffer);
	// line position: start / end interleaved
	m_lineArray.setVertexAttribute(0, 3, FGLDataType::Float, false, stride, (void*)0);
	// face normals: normal0 / normal1 interleaved
	m_lineArray.setVertexAttribute(1, 3, FGLDataType::Float, false, stride, (void*)(3 * sizeof(float)));
	// sample density: density0 / density1 interleaved
	m_lineArray.setVertexAttribute(2, 1, FGLDataType::Float, false, stride, (void*)(6 * sizeof(float)));

	// Set sample and candidate count to zero for all edges
	for (size_t i = 0, n = m_edges.size(); i < n; i++)
	{
		m_edges[i].sampleCount = 0;
		for (size_t j = 0; j < FGlobalConstants::MAX_SAMPLES_PER_EDGE; j++)
			m_edges[i].samples[j].candidateCount = 0;
	}
}

void FLineModel::release()
{
	m_solidMesh.release();
	m_lineArray.release();
	m_lineBuffer.release();
	m_lines.clear();
	m_edges.clear();
}

// Public queries -------------------------------------------------------------------------------------

size_t FLineModel::sampleCount() const
{
	size_t sampleCount = 0;

	for (size_t e = 0, ne = m_edges.size(); e < ne; e++)
		sampleCount += m_edges[e].sampleCount;

	return sampleCount;
}

#ifdef QT_DEBUG
void FLineModel::dump(QDebug& debug) const
{
	debug << "\n----- FLineModel -----\n";

	for (size_t e = 0, ne = m_edges.size(); e < ne; e++)
	{
		const edge_t& edge = m_edges[e];
		debug << "\nEdge #" << e << " from " << edge.modelPoint[0].toString()
			<< " to " << edge.modelPoint[1].toString();
		debug << "\n   Total samples: #" << edge.sampleCount;

		for (size_t s = 0, ns = edge.sampleCount; s < ns; s++)
		{
			const sample_t& sample = edge.samples[s];
			debug << "\n   Sample #" << s << " has " << sample.candidateCount
				<< " (" << sample.validCandidateCount << " valid) edge candidates, best: "
				<< sample.bestValidCandidate;

			for (size_t c = 0, nc = sample.candidateCount; c < nc; c++)
			{
				const candidate_t& cand = sample.candidates[c];
				debug << "\n      Candidate #" << c << ", Strength: " << cand.edgeResponse
					<< ", ColorMatch: " << cand.colorMatch << ", Dist: " << cand.signedDistance;
			}
		}
	}
}
#endif

// Internal functions ---------------------------------------------------------------------------------

void FLineModel::_calculateSingleHypothesis()
{
	m_numValidSamples = 0;

	for (size_t e = 0, ne = m_edges.size(); e < ne; e++)
	{
		edge_t& edge = m_edges[e];

		for (size_t s = 0, ns = edge.sampleCount; s < ns; s++)
		{
			sample_t& sample = edge.samples[s];
			if (sample.validCandidateCount > 0 && sample.isInlier)
			{
				size_t bestCandId = 0;
				float minSquareDist = FLT_MAX;
				float minSignedDist = FLT_MAX;
				float maxEdgeResponse = 0.0f;
				for (size_t c = 0, nc = sample.candidateCount; c < nc; c++)
				{
					candidate_t& cand = sample.candidates[c];
					if (cand.isValid)
					{
						float sd = (cand.position - edge.imagePoint[0]) * edge.imageNormal;
						cand.signedDistance = sd;
						cand.absDistance = fabsf(sd);
						float dd = sd * sd;
						if (cand.edgeResponse > maxEdgeResponse)
						{
							bestCandId = c;
							minSquareDist = dd;
							minSignedDist = sd;
							maxEdgeResponse = cand.edgeResponse;
						}
					}
				}

				F_ASSERT(minSquareDist < FLT_MAX); // at least one valid candidate has been found
				sample.bestValidCandidate = bestCandId;
				m_cost[m_numValidSamples] = minSignedDist;
				m_absCost[m_numValidSamples] = fabs(minSignedDist);
				m_numValidSamples++;

			} // END IF (sample.validCandidates > 0)
		}
	}
}

void FLineModel::_calculateBestHypothesis()
{
	m_numValidSamples = 0;

	for (size_t e = 0, ne = m_edges.size(); e < ne; e++)
	{
		edge_t& edge = m_edges[e];

		for (size_t s = 0, ns = edge.sampleCount; s < ns; s++)
		{
			sample_t& sample = edge.samples[s];
			if (sample.validCandidateCount > 0 && sample.isInlier)
			{
				size_t bestCandId = 0;
				float minSquareDist = FLT_MAX;
				float minSignedDist = FLT_MAX;
				for (size_t c = 0, nc = sample.candidateCount; c < nc; c++)
				{
					candidate_t& cand = sample.candidates[c];
					if (cand.isValid)
					{
						float sd = (cand.position - edge.imagePoint[0]) * edge.imageNormal;
						cand.signedDistance = sd;
						cand.absDistance = fabsf(sd);
						float dd = sd * sd;
						if (dd < minSquareDist)
						{
							bestCandId = c;
							minSquareDist = dd;
							minSignedDist = sd;
						}
					}
				}

				F_ASSERT(minSquareDist < FLT_MAX); // at least one valid candidate has been found
				sample.bestValidCandidate = bestCandId;
				m_cost[m_numValidSamples] = minSignedDist;
				m_absCost[m_numValidSamples] = fabs(minSignedDist);
				m_numValidSamples++;

			} // END IF (sample.validCandidates > 0)
		}
	}
}

void FLineModel::_updateJacobian(const FMatrix4f& mvp)
{
	size_t dataIndex = 0;

	float scale = 1.0f;
	float invScale = 1.0f / scale;

	FVector4f g1(1.0f, 0.0f, 0.0f, 0.0f);
	FVector4f g2(0.0f, 1.0f, 0.0f, 0.0f);
	FVector4f g3(0.0f, 0.0f, 1.0f, 0.0f);

	for (size_t i = 0, ni = m_edges.size(); i < ni; i++)
	{
		edge_t& edge = m_edges[i];

		FVector4f m_g1 = mvp * (g1 * scale);
		FVector4f m_g2 = mvp * (g2 * scale);
		FVector4f m_g3 = mvp * (g3 * scale);

		FVector2f p0_g1 = edge.imagePoint[0] + _homogenizeDerivative(edge.homImage[0], m_g1);
		FVector2f p1_g1 = edge.imagePoint[1] + _homogenizeDerivative(edge.homImage[1], m_g1);
		FVector2f n_g1 = (p1_g1 - p0_g1).normal().normalize();

		FVector2f p0_g2 = edge.imagePoint[0] + _homogenizeDerivative(edge.homImage[0], m_g2);
		FVector2f p1_g2 = edge.imagePoint[1] + _homogenizeDerivative(edge.homImage[1], m_g2);
		FVector2f n_g2 = (p1_g2 - p0_g2).normal().normalize();

		FVector2f p0_g3 = edge.imagePoint[0] + _homogenizeDerivative(edge.homImage[0], m_g3);
		FVector2f p1_g3 = edge.imagePoint[1] + _homogenizeDerivative(edge.homImage[1], m_g3);
		FVector2f n_g3 = (p1_g3 - p0_g3).normal().normalize();

		FVector4f m0_g4(0.0f, -edge.modelPoint[0].z(), edge.modelPoint[0].y(), 0.0f);
		FVector4f m1_g4(0.0f, -edge.modelPoint[1].z(), edge.modelPoint[1].y(), 0.0f);
		FVector2f p0_g4 = edge.imagePoint[0] + _homogenizeDerivative(edge.homImage[0], mvp * (m0_g4 * scale));
		FVector2f p1_g4 = edge.imagePoint[1] + _homogenizeDerivative(edge.homImage[1], mvp * (m1_g4 * scale));
		FVector2f n_g4 = (p1_g4 - p0_g4).normal().normalize();

		FVector4f m0_g5(edge.modelPoint[0].z(), 0.0f, -edge.modelPoint[0].x(), 0.0f);
		FVector4f m1_g5(edge.modelPoint[1].z(), 0.0f, -edge.modelPoint[1].x(), 0.0f);
		FVector2f p0_g5 = edge.imagePoint[0] + _homogenizeDerivative(edge.homImage[0], mvp * (m0_g5 * scale));
		FVector2f p1_g5 = edge.imagePoint[1] + _homogenizeDerivative(edge.homImage[1], mvp * (m1_g5 * scale));
		FVector2f n_g5 = (p1_g5 - p0_g5).normal().normalize();

		FVector4f m0_g6(-edge.modelPoint[0].y(), edge.modelPoint[0].x(), 0.0f, 0.0f);
		FVector4f m1_g6(-edge.modelPoint[1].y(), edge.modelPoint[1].x(), 0.0f, 0.0f);
		FVector2f p0_g6 = edge.imagePoint[0] + _homogenizeDerivative(edge.homImage[0], mvp * (m0_g6 * scale));
		FVector2f p1_g6 = edge.imagePoint[1] + _homogenizeDerivative(edge.homImage[1], mvp * (m1_g6 * scale));
		FVector2f n_g6 = (p1_g6 - p0_g6).normal().normalize();

		for (size_t s = 0, ns = edge.sampleCount; s < ns; s++)
		{
			sample_t& sample = edge.samples[s];
			if (sample.candidateCount == 0)
				continue;

			candidate_t& cand = sample.candidates[sample.bestValidCandidate];

			m_jacobian[dataIndex++] = invScale * (fabsf((cand.position - p0_g1) * n_g1) - cand.signedDistance);
			m_jacobian[dataIndex++] = invScale * (fabsf((cand.position - p0_g2) * n_g2) - cand.signedDistance);
			m_jacobian[dataIndex++] = invScale * (fabsf((cand.position - p0_g3) * n_g3) - cand.signedDistance);
			m_jacobian[dataIndex++] = invScale * (fabsf((cand.position - p0_g4) * n_g4) - cand.signedDistance);
			m_jacobian[dataIndex++] = invScale * (fabsf((cand.position - p0_g5) * n_g5) - cand.signedDistance);
			m_jacobian[dataIndex++] = invScale * (fabsf((cand.position - p0_g6) * n_g6) - cand.signedDistance);
		}
	}

	F_ASSERT(dataIndex == m_numValidSamples * 6);
}

// ----------------------------------------------------------------------------------------------------