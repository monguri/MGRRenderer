#include "ClosestFunction.h"

struct Vec3;

namespace mgrphysics
{
/// ２つの線分の最近接点検出
/// @param segmentPointA0 線分Aの始点
/// @param segmentPointA1 線分Aの終点
/// @param segmentPointB0 線分Bの始点
/// @param segmentPointB1 線分Bの終点
/// @param[out] closestPointA 線分A上の最近接点
/// @param[out] closestPointB 線分B上の最近接点
void getClosestTwoSegments(const Vec3& segmentPointA0, const Vec3& segmentPointA1, const Vec3& segmentPointB0, const Vec3& segmentPointB1, Vec3& closestPointA, Vec3& closestPointB);

/// 頂点から３角形面への最近接点検出
/// @param point 頂点
/// @param trianglePoint0 ３角形面の頂点0
/// @param trianglePoint1 ３角形面の頂点1
/// @param trianglePoint2 ３角形面の頂点2
/// @param triangleNormal ３角形面の法線ベクトル
/// @param[out] closestPoint ３角形面上の最近接点
void getClosestPointTriangle(const Vec3& point, const Vec3& trianglePoint0, const Vec3& trianglePoint1, const Vec3& trianglePoint2, const Vec3& triangleNormal, Vec3& closestPoint);
} // namespace mgrphysics
