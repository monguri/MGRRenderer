#include "Node.h"
#include "Director.h"

namespace mgrrenderer
{

Node::Node() : _scale(Vec3(1.0f, 1.0f, 1.0f)), _modelMatrix(Mat4::IDENTITY), _color(Color3B::WHITE)
{
}

void Node::update(float dt)
{
}

void Node::prepareRendering()
{
	// updateの次に呼ばれる前提でモデル行列計算する
	_modelMatrix = Mat4::createTransform(_position, _rotation, _scale);
}

void Node::renderGBuffer()
{
	// 何もしない
}

void Node::renderShadowMap()
{
	// 何もしない
}

void Node::renderForward()
{
	// 何もしない
}

void Node::setRotation(const Vec3& angleVec) {
	_rotation = Quaternion(angleVec);
}

Mat4 Node::getRotationMatrix() const
{
	return Mat4::createRotation(_rotation);
}

} // namespace mgrrenderer
