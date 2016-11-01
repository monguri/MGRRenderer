#include "Node.h"
#include "renderer/Director.h"

namespace mgrrenderer
{

Node::Node() : _scale(Vec3(1.0f, 1.0f, 1.0f)), _modelMatrix(Mat4::IDENTITY), _color(Color3B::WHITE)
{
}

void Node::update(float dt)
{
	(void)dt; //���g�p�ϐ��x���}��
}

void Node::prepareRendering()
{
	// update�̎��ɌĂ΂��O��Ń��f���s��v�Z����
	_modelMatrix = Mat4::createTransform(_position, _rotation, _scale);
}

void Node::renderGBuffer()
{
	// �������Ȃ�
}

void Node::renderShadowMap()
{
	// �������Ȃ�
}

void Node::renderForward()
{
	// �������Ȃ�
}

void Node::setRotation(const Vec3& angleVec) {
	_rotation = Quaternion(angleVec);
}

Mat4 Node::getRotationMatrix() const
{
	return Mat4::createRotation(_rotation);
}

} // namespace mgrrenderer