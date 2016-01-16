#pragma once
#include <gles/include/glew.h>
#include <glfw3/include/glfw3.h>
#include "BasicDataTypes.h"

namespace mgrrenderer
{

class Node
{
public:
	// TODO:本来はdtはスケジューラに渡せばいいのだが、今は各ノードでupdateメソッドでアニメーションをやってるのでdtをvisitとupdateに渡している
	virtual void visit(float dt);
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
	Color getColor() const { return _color; }
	void setColor(const Color& color) { _color = color; }

protected:
	Node();

	// この場所にあるのは変だが、Utilityクラスは後で作ろう
	OpenGLProgramData createOpenGLProgram(const GLchar* vertexShaderSource, const GLchar* fragmentShaderSource);
	void destroyOpenGLProgram(const OpenGLProgramData& programData) const;

	GLuint createVertexShader(const GLchar* source) const;
	GLuint createFragmentShader(const GLchar* source) const;
	GLuint createShaderProgram(const GLuint vertexShader, const GLuint fragmentShader) const;
	virtual void update(float dt);
	virtual void render();

private:
	Vec3 _position;
	Quaternion _rotation;
	Vec3 _scale;
	Mat4 _modelMatrix;
	Color _color;

	// TODO:本来はdtはスケジューラに渡せばいいのだが、今は各ノードでupdateメソッドでアニメーションをやってるのでdtをvisitとupdateに渡している
	GLint compileShader(GLuint shader, const GLchar* source) const;
};

} // namespace mgrrenderer
