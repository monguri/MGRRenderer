#pragma once

// TODO:�C���N���[�h�p�X��MGRRenderer/sources��ǉ����Ă���B���ƂŊO��
#include "renderer/BasicDataTypes.h"
#include "Shape.h"

// TODO:BasicDataTypes.h��mgrrenderer�ɂ���̂łƂ肠�����B���Ƃł�����Common�I�ȃv���W�F�N�g�Ɉړ�����
using namespace mgrrenderer;

namespace mgrphysics
{
// �`��R���e�i
struct Collidable
{
	static const int NUM_SHAPES = 5;

	// �ێ�����`��
	unsigned char numShapes;
	// �`��̔z��
	Shape shapes[NUM_SHAPES];
	// AABB�̒��S
	Vec3 center;
	// AABB�̃T�C�Y�̔���
	Vec3 half;

	// ������
	void reset()
	{
		numShapes = 0;
		center = Vec3(0.0f, 0.0f, 0.0f);
		half = Vec3(0.0f, 0.0f, 0.0f);
	}

	// �`���o�^����B�󂫂��Ȃ���Ζ�������B
	// @param shape �`��
	void addShape(const Shape& shape)
	{
		if (numShapes < NUM_SHAPES)
		{
			shapes[numShapes++] = shape;
		}
	}

	// �`��̓o�^������ʒm����
	// �S�Ă̌`���o�^������ɌĂяo���A�S�̂��͂�AABB���쐬����
	void finish()
	{
		Vec3 aabbMax(-FLT_MAX, -FLT_MAX, -FLT_MAX), aabbMin(FLT_MAX, FLT_MAX, FLT_MAX);

		for (unsigned int i = 0; i < numShapes; ++i)
		{
			const ConvexMesh& mesh = shapes[i].geometry;

			for (unsigned int v = 0; v < mesh.numVertices; ++v)
			{
				aabbMax = Vec3::maxVec3(aabbMax, shapes[i].offsetPosition + shapes[i].offsetOrientation * mesh.vertices[v]);
				aabbMin = Vec3::minVec3(aabbMin, shapes[i].offsetPosition + shapes[i].offsetOrientation * mesh.vertices[v]);
			}

			center = (aabbMax + aabbMin) / 2.0f;
			half = (aabbMax - aabbMin) / 2.0f;
		}
	}
};
} // namespace mgrphysics
