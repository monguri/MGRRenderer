#include "Simulation.h"
#include "pipeline/BroadPhase.h"
#include "pipeline/CollisionDetection.h"
#include "pipeline/ConstraintSolver.h"
#include "pipeline/Integrate.h"
#include "elements/State.h"
#include "elements/RigidBody.h"
#include "elements/Collidable.h"
#include "elements/BallJoint.h"
#include "elements/Pair.h"
// TODO:インクルードパスにMGRRenderer/sourcesを追加している。あとで外す
#include "renderer/BasicDataTypes.h"

namespace mgrphysics
{
static const int MAX_RIGID_BODIES = 500;
static const int MAX_JOINTS = 100;
static const int MAX_PAIRS = 5000;
static const float TIME_STEP = 0.016f;
static const float CONTACT_BIAS = 0.1f;
static const float CONTACT_SLOP = 0.001f;
static const int ITERATION = 10;
static const Vec3 gravity(0.0f, -9.8f, 0.0f);

State states[MAX_RIGID_BODIES];
RigidBody bodies[MAX_RIGID_BODIES];
Collidable collidables[MAX_RIGID_BODIES];
unsigned int numRigidBodies = 0;

BallJoint joints[MAX_JOINTS];
unsigned int numJoints = 0;

unsigned int pairSwap = 0;
unsigned int numPairs[2];
Pair pairs[2][MAX_PAIRS];

void SimulatePhysics()
{
	pairSwap = 1 - pairSwap;

	broadPhase(
		states,
		collidables,
		numRigidBodies,
		pairs[1 - pairSwap],
		numPairs[1 - pairSwap],
		pairs[pairSwap],
		numPairs[pairSwap],
		MAX_PAIRS,
		nullptr
	);

	detectCollision(
		states,
		collidables,
		numRigidBodies,
		pairs[pairSwap],
		numPairs[pairSwap]
	);

	solveConstraint(
		states,
		bodies,
		numRigidBodies,
		pairs[pairSwap],
		numPairs[pairSwap],
		joints,
		numJoints,
		ITERATION,
		CONTACT_BIAS,
		CONTACT_SLOP,
		TIME_STEP
	);

	integrate(
		states,
		numRigidBodies,
		TIME_STEP
	);
}
} // namespace mgrphysics
