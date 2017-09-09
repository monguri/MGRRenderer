#include "BroadPhase.h"
#include "../elements/State.h"
#include "../elements/Collidable.h"
#include "../elements/Pair.h"
// TODO:インクルードパスにMGRRenderer/sourcesを追加している。あとで外す
#include "utility/Logger.h"


namespace mgrphysics
{
void broadPhase(
	const State* states,
	const Collidable* collidables,
	unsigned int numRigidBodies,
	Pair* oldPairs,
	unsigned int numOldPairs,
	Pair* newPairs,
	unsigned int numNewPairs,
	const unsigned int maxPairs,
	void* userData
)
{
}
} // namespace mgrphysics
