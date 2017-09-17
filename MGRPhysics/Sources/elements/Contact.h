#pragma once

// TODO:インクルードパスにMGRRenderer/sourcesを追加している。あとで外す
#include "renderer/BasicDataTypes.h"
#include "Constraint.h"

// TODO:BasicDataTypes.hがmgrrendererにあるのでとりあえず。あとでそれらはCommon的なプロジェクトに移動する
using namespace mgrrenderer;

namespace mgrphysics
{
// 衝突点
struct ContactPoint
{
	// 貫通深度
	float distance;
	// 衝突点（剛体Aのローカル座標系）
	Vec3 pointA;
	// 衝突点（剛体Bのローカル座標系）
	Vec3 pointB;
	// 衝突点の法線ベクトル（ワールド座標系）
	Vec3 normal;
	// 拘束
	Constraint constraints[3];

	void reset()
	{
		constraints[0].accumImpulse = 0.0f;
		constraints[1].accumImpulse = 0.0f;
		constraints[2].accumImpulse = 0.0f;
	}
};

// 衝突情報
struct Contact
{
	static const int NUM_CONTACTS = 4;

	// 衝突の数
	unsigned int numContacts;
	// 摩擦
	float friction;
	// 衝突点の配列
	ContactPoint contactPoints[NUM_CONTACTS];

	// 同一衝突点を探索する
	// @param newPointA 衝突点（剛体Aのローカル座標系）
	// @param newPointB 衝突点（剛体Bのローカル座標系）
	// @param newNormal 衝突点の法線ベクトル（ワールド座標系）
	// @return 同一衝突点をみつけたらそのインデックス。なければ-1
	int findNearestContactPoint(const Vec3& newPointA, const Vec3& newPointB, const Vec3& newNormal);

	// 衝突点を入れ替える
	// @param newPoint 衝突点（剛体のローカル座標系）
	// @param newDistance 貫通深度
	// @return 破棄する衝突点のインデックスを返す
	int sort4ContactPoints(const Vec3& newPoint, float newDistance);

	// 衝突点を破棄する
	// @param i 破棄する衝突点のインデックス
	void removeContactPoint(int i);

	// 初期化
	void reset();

	// 衝突点をリフレッシュする
	// @param pA 剛体Aの位置
	// @param qA 剛体Aの姿勢
	// @param pB 剛体Bの位置
	// @param qB 剛体Bの姿勢
	void refresh(const Vec3& pA, const Quaternion& qA, const Vec3& pB, const Quaternion& qB);

	// 衝突点をマージする
	// @param contact 合成する衝突情報
	void merge(const Contact& contact);

	// 衝突点を追加する
	// @param penetrationDepth 貫通深度
	// @param normal 衝突点の法線ベクトル（ワールド座標系）
	// @param contactPointA 衝突点（剛体Aのローカル座標系）
	// @param contactPointB 衝突点（剛体Bのローカル座標系）
	void addContact(float penetrationDepth, const Vec3& normal, const Vec3& contactPointA, const Vec3& contactPointB);
};
} // namespace mgrphysics
