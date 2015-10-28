#pragma once
#include <gles/include/glew.h>
// TODO:ssize_tを使うためだが、もっとましな方法はないかな。。
#include <ShlObj.h>

#ifndef __SSIZE_T
#define __SSIZE_T
typedef SSIZE_T ssize_t;
#endif // __SSIZE_T

namespace mgrrenderer
{

struct Vec2
{
	GLfloat x;
	GLfloat y;

	Vec2() : x(0), y(0) {}
	Vec2(GLfloat x, GLfloat y) : x(x), y(y) {}
};

struct Size
{
	float width;
	float height;

	Size() : width(0), height(0) {}
	Size(float width, float height) : width(width), height(height) {}
};

struct Rect
{
	Vec2 origin;
	Size size;

	Rect() : origin(Vec2()), size(Size()) {}
	Rect(GLfloat x, GLfloat y, GLfloat w, GLfloat h) : origin(Vec2(x, y)), size(Size(w, h)) {}
};

struct TextureCoordinates
{
	GLfloat u;
	GLfloat v;

	TextureCoordinates() : u(0), v(0) {}
	TextureCoordinates(GLfloat u, GLfloat v) : u(u), v(v) {}
};

struct Position2DTextureCoordinates
{
	Vec2 pos;
	TextureCoordinates texCoords;

	Position2DTextureCoordinates() :
	pos(Vec2()),
	texCoords(TextureCoordinates())
	{}

	Position2DTextureCoordinates(const Vec2& position, const TextureCoordinates& textureCoordinates) :
	pos(position),
	texCoords(textureCoordinates)
	{}
};

struct Quadrangle2D
{
	Position2DTextureCoordinates topLeft;
	Position2DTextureCoordinates bottomLeft;
	Position2DTextureCoordinates topRight;
	Position2DTextureCoordinates bottomRight;
};

struct OpenGLProgramData
{
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint shaderProgram;
	GLint attributeVertexPosition;
	GLint attributeTextureCoordinates;
	GLint uniformTexture;
};

} // namespace mgrrenderer
