#include "Node.h"
#include "renderer/Director.h"

namespace mgrrenderer
{

Node::Node() : _scale(Vec3(1.0f, 1.0f, 1.0f)), _modelMatrix(Mat4::IDENTITY), _color(Color3B::WHITE)
{
}

void Node::update(float dt)
{
	(void)dt; //未使用変数警告抑制
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

void Node::renderDirectionalLightShadowMap(const DirectionalLight* light)
{
	// 何もしない
	(void)light;
}

void Node::renderPointLightShadowMap(size_t index, const PointLight* light, CubeMapFace face)
{
	// 何もしない
	(void)index;
	(void)light;
	(void)face;
}

void Node::renderSpotLightShadowMap(size_t index, const SpotLight* light)
{
	// 何もしない
	(void)index;
	(void)light;
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
