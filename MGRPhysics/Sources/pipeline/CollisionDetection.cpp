#include "CollisionDetection.h"
#include "../elements/State.h"
#include "../elements/Collidable.h"
#include "../elements/Pair.h"
#include "../elements/Contact.h"
#include "../collide/ConvexConvexContact.h"

// TODO:�C���N���[�h�p�X��MGRRenderer/sources��ǉ����Ă���B���ƂŊO��
#include "renderer/BasicDataTypes.h"

// TODO:BasicDataTypes.h��mgrrenderer�ɂ���̂łƂ肠�����B���Ƃł�����Common�I�ȃv���W�F�N�g�Ɉړ�����
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
	Logger::logAssert(states != nullptr, "states������nullptr�B");
	Logger::logAssert(collidables != nullptr, "collidables������nullptr�B");
	Logger::logAssert(pairs != nullptr, "pairs������nullptr�B");

	for (unsigned int i = 0; i < numPairs; ++i)
	{
		const Pair& pair = pairs[i];
		Logger::logAssert(pair.contact != nullptr, "pair��contact��nullptr�B");

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

				// TODO:���g������
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
					// �Փ˓_�����̂̃��[�J�����W�n�ɕϊ����A�R���^�N�g�ɒǉ�����
					Vec4 offsetContactPointA = offsetTransformA * Vec4(contactPointA);
					Vec4 offsetContactPointB = offsetTransformB * Vec4(contactPointB);

					// TODO:���g������
					pair.contact->addContact(penetrationDepth, normal,
						Vec3(offsetContactPointA.x, offsetContactPointA.y, offsetContactPointA.z),
						Vec3(offsetContactPointB.x, offsetContactPointB.y, offsetContactPointB.z);
				}
			}
		}
	}
}
} // namespace mgrphysics
