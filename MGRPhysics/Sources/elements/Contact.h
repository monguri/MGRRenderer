#pragma once

// TODO:�C���N���[�h�p�X��MGRRenderer/sources��ǉ����Ă���B���ƂŊO��
#include "renderer/BasicDataTypes.h"
#include "Constraint.h"

// TODO:BasicDataTypes.h��mgrrenderer�ɂ���̂łƂ肠�����B���Ƃł�����Common�I�ȃv���W�F�N�g�Ɉړ�����
using namespace mgrrenderer;

namespace mgrphysics
{
// �Փ˓_
struct ContactPoint
{
	// �ђʐ[�x
	float distance;
	// �Փ˓_�i����A�̃��[�J�����W�n�j
	Vec3 pointA;
	// �Փ˓_�i����B�̃��[�J�����W�n�j
	Vec3 pointB;
	// �Փ˓_�̖@���x�N�g���i���[���h���W�n�j
	Vec3 normal;
	// �S��
	Constraint constraints[3];

	void reset()
	{
		constraints[0].accumImpulse = 0.0f;
		constraints[1].accumImpulse = 0.0f;
		constraints[2].accumImpulse = 0.0f;
	}
};

// �Փˏ��
struct Contact
{
	static const int NUM_CONTACTS = 4;

	// �Փ˂̐�
	unsigned int numContacts;
	// ���C
	float friction;
	// �Փ˓_�̔z��
	ContactPoint contactPoints[NUM_CONTACTS];

	// ����Փ˓_��T������
	// @param newPointA �Փ˓_�i����A�̃��[�J�����W�n�j
	// @param newPointB �Փ˓_�i����B�̃��[�J�����W�n�j
	// @param newNormal �Փ˓_�̖@���x�N�g���i���[���h���W�n�j
	// @return ����Փ˓_���݂����炻�̃C���f�b�N�X�B�Ȃ����-1
	int findNearestContactPoint(const Vec3& newPointA, const Vec3& newPointB, const Vec3& newNormal);

	// �Փ˓_�����ւ���
	// @param newPoint �Փ˓_�i���̂̃��[�J�����W�n�j
	// @param newDistance �ђʐ[�x
	// @return �j������Փ˓_�̃C���f�b�N�X��Ԃ�
	int sort4ContactPoints(const Vec3& newPoint, float newDistance);

	// �Փ˓_��j������
	// @param i �j������Փ˓_�̃C���f�b�N�X
	void removeContactPoint(int i);

	// ������
	void reset();

	// �Փ˓_�����t���b�V������
	// @param pA ����A�̈ʒu
	// @param qA ����A�̎p��
	// @param pB ����B�̈ʒu
	// @param qB ����B�̎p��
	void refresh(const Vec3& pA, const Quaternion& qA, const Vec3& pB, const Quaternion& qB);

	// �Փ˓_���}�[�W����
	// @param contact ��������Փˏ��
	void merge(const Contact& contact);

	// �Փ˓_��ǉ�����
	// @param penetrationDepth �ђʐ[�x
	// @param normal �Փ˓_�̖@���x�N�g���i���[���h���W�n�j
	// @param contactPointA �Փ˓_�i����A�̃��[�J�����W�n�j
	// @param contactPointB �Փ˓_�i����B�̃��[�J�����W�n�j
	void addContact(float penetrationDepth, const Vec3& normal, const Vec3& contactPointA, const Vec3& contactPointB);
};
} // namespace mgrphysics
