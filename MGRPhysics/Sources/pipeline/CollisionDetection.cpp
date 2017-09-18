#include "CollisionDetection.h"
#include "../elements/State.h"
#include "../elements/Collidable.h"
#include "../elements/Pair.h"
#include "../elements/Contact.h"
#include "../collide/ConvexConvexContact.h"

// TODO:インクルードパスにMGRRenderer/sourcesを追加している。あとで外す
#include "renderer/BasicDataTypes.h"

// TODO:BasicDataTypes.hがmgrrendererにあるのでとりあえず。あとでそれらはCommon的なプロジェクトに移動する
using namespace mgrrenderer;


namespace mgrphysics
{
void detectCollision(
	const State* states,
	const Collidable* collidables,
	unsigned int numRigidBodies,
	const Pair* pairs,
	unsigned int numPairs
)
{
	Logger::logAssert(states != nullptr, "states引数がnullptr。");
	Logger::logAssert(collidables != nullptr, "collidables引数がnullptr。");
	Logger::logAssert(pairs != nullptr, "pairs引数がnullptr。");

	for (unsigned int i = 0; i < numPairs; ++i)
	{
		const Pair& pair = pairs[i];
		Logger::logAssert(pair.contact != nullptr, "pairのcontactがnullptr。");

		const State& stateA = states[pair.rigidBodyA];
		const State& stateB = states[pair.rigidBodyB];

		const Collidable& collidableA = collidables[pair.rigidBodyA];
		const Collidable& collidableB = collidables[pair.rigidBodyB];

		const Mat4& transformA = Mat4::createTransform(stateA.position, stateA.orientation, Vec3(1.0f, 1.0f, 1.0f));
		const Mat4& transformB = Mat4::createTransform(stateB.position, stateB.orientation, Vec3(1.0f, 1.0f, 1.0f));

		for (unsigned int j = 0; j < collidableA.numShapes; ++j)
		{
			const Shape& shapeA = collidableA.shapes[j];
			const Mat4& offsetTransformA = Mat4::createTransform(shapeA.offsetPosition, shapeA.offsetOrientation, Vec3(1.0f, 1.0f, 1.0f));
			const Mat4& worldTransformA = transformA * offsetTransformA;

			for (unsigned int k = 0; k < collidableB.numShapes; ++k)
			{
				const Shape& shapeB = collidableB.shapes[j];
				const Mat4& offsetTransformB = Mat4::createTransform(shapeB.offsetPosition, shapeB.offsetOrientation, Vec3(1.0f, 1.0f, 1.0f));
				const Mat4& worldTransformB = transformB * offsetTransformB;

				Vec3 normal;
				float penetrationDepth;
				Vec3 contactPointA;
				Vec3 contactPointB;

				// TODO:中身を実装
				if (convexConvexContact(
						shapeA.geometry,
						worldTransformA,
						shapeB.geometry,
						worldTransformB,
						normal,
						penetrationDepth,
						contactPointA,
						contactPointB
					)
					&& penetrationDepth < 0.0f
				)
				{
					Vec4 contactPointAvec4 = Vec4(contactPointA);
					// 衝突点を剛体のローカル座標系に変換し、コンタクトに追加する
					Vec4 offsetContactPointA = offsetTransformA * Vec4(contactPointA);
					Vec4 offsetContactPointB = offsetTransformB * Vec4(contactPointB);

					// TODO:中身を実装
					pair.contact->addContact(penetrationDepth, normal,
						Vec3(offsetContactPointA.x, offsetContactPointA.y, offsetContactPointA.z),
						Vec3(offsetContactPointB.x, offsetContactPointB.y, offsetContactPointB.z);
				}
			}
		}
	}
}
} // namespace mgrphysics
