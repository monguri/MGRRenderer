#pragma once

#include "ConvexMesh.h"

namespace mgrphysics
{
struct Shape
{
	ConvexMesh geometry;
	Vec3 offsetPosition;
	Quaternion offsetOrientation;
	void *userData;

	void reset()
	{
		geometry.reset();
		offsetPosition = Vec3::ZERO;
		offsetOrientation = Quaternion::IDENTITY;
		userData = NULL; // reset�ł͋����I��NULL�ɂ���̂Œ���
	}
};
} // namespace mgrphysics
