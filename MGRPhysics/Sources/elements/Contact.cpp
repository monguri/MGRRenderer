#include "Contact.h"

int mgrphysics::Contact::findNearestContactPoint(const Vec3 & newPointA, const Vec3 & newPointB, const Vec3 & newNormal)
{
	return -1;
}

int mgrphysics::Contact::sort4ContactPoints(const Vec3 & newPoint, float newDistance)
{
	return 0;
}

void mgrphysics::Contact::removeContactPoints(int i)
{
}

void mgrphysics::Contact::refresh(const Vec3 & pA, const Quaternion & qA, const Vec3 & pB, const Quaternion & qB)
{
}

void mgrphysics::Contact::merge(const Contact & contact)
{
}

void mgrphysics::Contact::addContact(float penetrationDepth, const Vec3 & normal, const Vec3 & contactPointA, const Vec3 & contactPointB)
{
}
