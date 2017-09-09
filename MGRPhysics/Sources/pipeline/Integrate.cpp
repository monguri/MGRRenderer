#include "Integrate.h"
#include "../elements/State.h"
#include "../elements/RigidBody.h"

namespace mgrphysics
{
void applyExternalForce(
	State& state,
	RigidBody& body,
	const Vec3& externalForce,
	const Vec3& externalTorque,
	float timeStep
)
{
}

void integrate(
	State* states,
	unsigned int numRigidBodies,
	float timeStep
)
{
}
} // namespace mgrphysics
