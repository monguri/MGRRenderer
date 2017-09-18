#pragma once

// TODO:インクルードパスにMGRRenderer/sourcesを追加している。あとで外す
#include "renderer/BasicDataTypes.h"
// TODO:BasicDataTypes.hがmgrrendererにあるのでとりあえず。あとでそれらはCommon的なプロジェクトに移動する
using namespace mgrrenderer;

namespace mgrphysics
{
struct ConvexMesh;

// 2つの凸メッシュの衝突検出
// @param convexA 凸メッシュA
// @param transformA Aのワールド変換行列
// @param convexB 凸メッシュB
// @param transformB Bのワールド変換行列
// @param[out] normal 衝突点の法線ベクトル（ワールド座標系）
// @param[out] penetrationDepth 貫通深度
// @param[out] contactPointA 衝突点（剛体Aのローカル座標系）
// @param[out] contactPointB 衝突点（剛体Bのローカル座標系）
// @return　衝突を検出した場合はtrueを返す。
bool convexConvexContact(
	const ConvexMesh& convexA,
	const Mat4& transformA,
	const ConvexMesh& convexB,
	const Mat4& transformB,
	Vec3& normal,
	float& penetrationDepth,
	Vec3& contactPointA,
	Vec3& contactPointB
);
} // namespace mgrphysics
