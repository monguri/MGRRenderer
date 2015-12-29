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
	void visit(float dt);
	void setPosition(const Vec3& position) { _position = position; };
	void setRotation(const Quaternion& rotation) { _rotation = rotation; };
	void setRotation(const Vec3& rotation);
	void setScale(const Vec3& scale) { _scale = scale; };
	void setScale(float scale) { _scale = Vec3(scale, scale, scale); };

protected:
	Node();

	// この場所にあるのは変だが、Utilityクラスは後で作ろう
	OpenGLProgramData createOpenGLProgram(const GLchar* vertexShaderSource, const GLchar* fragmentShaderSource);
	void destroyOpenGLProgram(const OpenGLProgramData& programData) const;

	GLuint createVertexShader(const GLchar* source) const;
	GLuint createFragmentShader(const GLchar* source) const;
	GLuint createShaderProgram(const GLuint vertexShader, const GLuint fragmentShader) const;

	const Mat4& getModelMatrix() const { return _modelMatrix; }

private:
	Vec3 _position;
	Quaternion _rotation;
	Vec3 _scale;
	Mat4 _modelMatrix;

	// TODO:本来はdtはスケジューラに渡せばいいのだが、今は各ノードでupdateメソッドでアニメーションをやってるのでdtをvisitとupdateに渡している
	virtual void update(float dt);
	virtual void render();
	GLint compileShader(GLuint shader, const GLchar* source) const;
};

} // namespace mgrrenderer
