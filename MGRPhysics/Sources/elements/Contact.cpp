#include "Contact.h"

namespace mgrphysics
{

int Contact::findNearestContactPoint(const Vec3 & newPointA, const Vec3 & newPointB, const Vec3 & newNormal)
{
	return -1;
}

int Contact::sort4ContactPoints(const Vec3 & newPoint, float newDistance)
{
	return 0;
}

void Contact::reset()
{
	numContacts = 0;
	for (int i = 0; i < NUM_CONTACTS; ++i)
	{
		contactPoints[i].reset();
	}
}

void Contact::removeContactPoint(int i)
{
	// �����̗v�f�ŏ㏑������
	contactPoints[i] = contactPoints[numContacts - 1];
	numContacts--;
}

void Contact::refresh(const Vec3 & pA, const Quaternion & qA, const Vec3 & pB, const Quaternion & qB)
{
	// �Փ˓_��臒l�i���ʕ����j
	static const float CONTACT_THRESHOLD_TANGENT = 0.002f;

	// �Փ˓_�̍X�V
	// ���Փ˓_�̋�����臒l�iCONTACT_THRESHOLD�j�𒴂��������
	for (int i = 0; i < (int)numContacts; ++i)
	{
		const Vec3& normal = contactPoints[i].normal;
		// pointA�ApointB�ɂ��Ă͑O�Ɍv�Z�Ɏg�������̂�p���āA����AB�̈ʒu�ƃ|�[�Y�̕ύX�̂ݔ��f����
		const Vec3& cpA = pA + Mat3::createRotation(qA) * contactPoints[i].pointA;
		const Vec3& cpB = pB + Mat3::createRotation(qB) * contactPoints[i].pointB;

		// �ђʐ[�x���v���X�ɓ]�������`�F�b�N�iTODO:������ӁA���_���悭�킩��񂭂Ȃ��Ă����j
		// �Փ˓_��臒l�i�@�������j
		static const float CONTACT_THRESHOLD_NORMAL = 0.01f;
		float distanceNormalDir = Vec3::dot(normal, (cpA - cpB));
		if (distanceNormalDir > CONTACT_THRESHOLD_NORMAL)
		{
			removeContactPoint(i);
			i--;
			continue;
		}
		contactPoints[i].distance = distanceNormalDir;

		// �[�x�����̋������������ĕ��ʏ�̋������`�F�b�N
		const Vec3& cpAsubtractedNormal = cpA - normal * distanceNormalDir;
		float distanceOnPlane = (cpA - cpB).lengthSquare();
		// �Փ˓_��臒l�i���ʕ����A2��j
		static const float CONTACT_THRESHOLD_TANGENT = 0.002f;
		if (distanceOnPlane > CONTACT_THRESHOLD_TANGENT)
		{
			removeContactPoint(i);
			i--;
			continue;
		}
	}
}

void Contact::merge(const Contact & contact)
{
}

void Contact::addContact(float penetrationDepth, const Vec3 & normal, const Vec3 & contactPointA, const Vec3 & contactPointB)
{
}
} // namespace mgrphysics
