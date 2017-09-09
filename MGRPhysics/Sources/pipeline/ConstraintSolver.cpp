#include "ConstraintSolver.h"
#include "../elements/State.h"
#include "../elements/Collidable.h"
#include "../elements/Pair.h"
#include "../elements/BallJoint.h"

namespace mgrphysics
{
void solveConstraint(
	const State* states,
	const Collidable* collidables,
	unsigned int numRigidBodies,
	const Pair* pairs,
	unsigned int numPairs,
	BallJoint* joints,
	unsigned int numJoints,
	unsigned int iteration,
	float bias,
	float slop,
	float timeStep
)
{
}
} // namespace mgrphysics
