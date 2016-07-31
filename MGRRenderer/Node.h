#pragma once
#include "Config.h"
#include "BasicDataTypes.h"

namespace mgrrenderer
{

class Node
{
public:
	// TODO:本来はdtはスケジューラに渡せばいいのだが、今は各ノードでupdateメソッドでアニメーションをやってるのでdtをvisitとupdateに渡している
	virtual void update(float dt);
	virtual void prepareRendering();
	virtual void renderGBuffer();
	virtual void renderShadowMap();
	virtual void renderWithShadowMap();
	const Vec3& getPosition() const { return _position; }
	virtual void setPosition(const Vec3& position) { _position = position; };
	const Quaternion& getRotation() const { return _rotation; }
	void setRotation(const Quaternion& rotation) { _rotation = rotation; };
	void setRotation(const Vec3& rotation);
	const Vec3& getScale() const { return _scale; }
	void setScale(const Vec3& scale) { _scale = scale; };
	void setScale(float scale) { _scale = Vec3(scale, scale, scale); };
	const Mat4& getModelMatrix() const { return _modelMatrix; }
	void setModelMatrix(const Mat4& mat) { _modelMatrix = mat; }
	Mat4 getRotationMatrix() const;
	const Color3B& getColor() const { return _color; }
	virtual void setColor(const Color3B& color) { _color = color; }

protected:
	Node();

private:
	Vec3 _position;
	Quaternion _rotation;
	Vec3 _scale;
	Mat4 _modelMatrix;
	Color3B _color;
};

} // namespace mgrrenderer
