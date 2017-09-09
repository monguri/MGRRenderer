#pragma once

// TODO:インクルードパスにMGRRenderer/sourcesを追加している。あとで外す
#include "renderer/BasicDataTypes.h"
#include "Shape.h"

// TODO:BasicDataTypes.hがmgrrendererにあるのでとりあえず。あとでそれらはCommon的なプロジェクトに移動する
using namespace mgrrenderer;

namespace mgrphysics
{
// 形状コンテナ
struct Collidable
{
	static const int NUM_SHAPES = 5;

	// 保持する形状数
	unsigned char numShapes;
	// 形状の配列
	Shape shapes[NUM_SHAPES];
	// AABBの中心
	Vec3 center;
	// AABBのサイズの半分
	Vec3 half;

	// 初期化
	void reset()
	{
		numShapes = 0;
		center = Vec3(0.0f, 0.0f, 0.0f);
		half = Vec3(0.0f, 0.0f, 0.0f);
	}

	// 形状を登録する。空きがなければ無視する。
	// @param shape 形状
	void addShape(const Shape& shape)
	{
		if (numShapes < NUM_SHAPES)
		{
			shapes[numShapes++] = shape;
		}
	}

	// 形状の登録完了を通知する
	// 全ての形状を登録した後に呼び出し、全体を囲むAABBを作成する
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
