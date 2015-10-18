#include <iostream>
#include <assert.h>
#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include "MGRRenderer.h"

using namespace mgrrenderer;

// �錾
static void fwErrorHandler(int error, const char* description);
static void fwKeyInputHandler(GLFWwindow* window, int key, int scancode, int action, int mods);
static void initialize();
static void render();
static void finalize();

int main()
{
	glfwSetErrorCallback(fwErrorHandler);

	if (glfwInit() == GL_FALSE)
	{
		std::cerr << "Can't initilize GLFW" << std::endl;
		return 1;
	}

	// OpenGL3.2�̃T���v���ł͎g�p���Ă������AAPI���ς�����̂�����������Ă����glCreateShader(GL_VERTEX_SHADER)��GL_INVALID_ENUM�G���[���ԁF��
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *const window = glfwCreateWindow(640, 480, "MGRRendererSampleApplication", NULL, NULL);
	if (window == nullptr)
	{
		std::cerr << "Can't create GLFW window." << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, fwKeyInputHandler);

	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
		exit(EXIT_FAILURE);
	}

	// �`��̏�����
	initialize();

	while (glfwWindowShouldClose(window) == GL_FALSE)
	{
		// �`��̃��C�����[�v
		render();

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	// �`��̏I������
	finalize();

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

void fwErrorHandler(int error, const char* description)
{
	std::cerr << "glfw Error: " << description << std::endl;
}

void fwKeyInputHandler(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

// TODO:���T�C�Y�C�x���g���󂯂郁�\�b�h�����肽����
// �`�敨�̃f�[�^�\���ƁAOpenGL���g���ĕ`�悷�郍�W�b�N�͐؂藣���ď����ׂ��BOpenGL�ȊO�ɈڐA���邽�߂ɂˁB
void initialize()
{
	std::vector<Point2DData> positionAndPointSize{
		Point2DData(0.0f, 0.0f, 15.0f),
		Point2DData(0.75f, 0.75f, 15.0f),
	};
	Point2D* pointNode = new Point2D(positionAndPointSize);

	std::vector<Vec2> lineVertices{
		Vec2(-1.0f, 0.0f), Vec2(1.0f, 0.0f),
		Vec2(0.0f, 1.0f), Vec2(0.0f, -1.0f),
	};
	Line2D* lineNode = new Line2D(lineVertices);

	std::vector<Vec2> polygonVertices{
		Vec2(-0.75f, 0.75f), Vec2(-0.75f, 0.25f), Vec2(-0.25f, 0.75f),
		Vec2(-0.25f, 0.75f), Vec2(-0.75f, 0.25f), Vec2(-0.25f, 0.25f),
	};
	Polygon2D* polygonNode = new Polygon2D(polygonVertices);

	Scene* scene = new Scene();
	scene->pushNode(pointNode);
	scene->pushNode(lineNode);
	scene->pushNode(polygonNode);

	Director::getInstance()->initWithScene(*scene);
}

void render()
{
	Director::getInstance()->update();
}

void finalize() {
	Director::getInstance()->destroy();
}
