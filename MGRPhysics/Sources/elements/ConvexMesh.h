#pragma once

// TODO:BasicDataTypes.h��mgrrenderer�ɂ���̂łƂ肠�����B���Ƃł�����Common�I�ȃv���W�F�N�g�Ɉړ�����
#include "renderer/BasicDataTypes.h"

// TODO:BasicDataTypes.h��mgrrenderer�ɂ���̂łƂ肠�����B���Ƃł�����Common�I�ȃv���W�F�N�g�Ɉړ�����
using namespace mgrrenderer;

namespace mgrphysics
{
enum class EdgeType : int
{
	CONVEX = 0,
	CONCAVE,
	FLAT,
};

// �G�b�W
struct Edge
{
	EdgeType type;
	unsigned char vertId[2];
	unsigned char facetId[2];
};

// �O�p�`��
struct Facet
{
	unsigned char vertId[3];
	unsigned char edgeId[3];
	Vec3 normal;
};

// �ʃ��b�V��
struct ConvexMesh
{
	static const int MAX_VERTICES = 34;
	static const int MAX_EDGES = 96;
	static const int MAX_FACETS = 64;

	unsigned char numVertices;
	unsigned char numFacets;
	unsigned char numEdges;

	Vec3 vertices[MAX_VERTICES];
	Edge edges[MAX_EDGES];
	Facet facets[MAX_FACETS];

	void reset()
	{
		numVertices = 0;
		numFacets = 0;
		numEdges = 0;
	}

	bool InitConvexMesh(const float* vertices, unsigned int numVertices, const unsigned short* indices, unsigned int numIndices, const Vec3& scale = Vec3(1.0f, 1.0f, 1.0f));

	void GetProjection(float& outMin, float& outMax, const Vec3& axis) const;
};
} // namespace mgrphysics
