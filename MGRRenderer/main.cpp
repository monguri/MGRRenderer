#include <iostream>
#include <assert.h>
#include <gles/include/glew.h>
#include <glfw3/include/glfw3.h>
#include "MGRRenderer.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>
#include <tchar.h>

static const int WINDOW_WIDTH = 640;
static const int WINDOW_HEIGHT = 480;

using namespace mgrrenderer;

// �錾
static void fwErrorHandler(int error, const char* description);
static void fwKeyInputHandler(GLFWwindow* window, int key, int scancode, int action, int mods);
static void initialize();
static void render();
static void finalize();

int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
//int main()
//{
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

	GLFWwindow *const window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "MGRRendererSampleApplication", NULL, NULL);
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
	return 0;
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

void initialize()
{
	//
	// �`�悷��V�[����MGRRenderer�ɓo�^���Ă���
	//
	bool isSucceeded = false;

	Director::getInstance()->init(Size(WINDOW_WIDTH, WINDOW_HEIGHT));

	// �e�m�[�h�̍쐬��Director::init�̌�ɌĂԁBDirector::init�̂����Ă���E�B���h�E�T�C�Y���g�p����ꍇ������̂ŁB

	//std::vector<Point2DData> positionAndPointSize{
	//	Point2DData(320.0f, 240.0f, 15.0f),
	//	Point2DData(80.0f, 60.0f, 15.0f),
	//};
	//Point2D* pointNode = new Point2D();
	//pointNode->initWithPointArray(positionAndPointSize);

	//std::vector<Vec2> lineVertices{
	//	Vec2(0.0f, 240.0f), Vec2(640.0f, 240.0f),
	//	Vec2(320.0f, 480.0f), Vec2(320.0f, 0.0f),
	//};
	//Line2D* lineNode = new Line2D();
	//isSucceeded = lineNode->initWithVertexArray(lineVertices);
	//assert(isSucceeded);

	//std::vector<Vec2> polygonVertices{
	//	Vec2(80.0f, 420.0f), Vec2(80.0f, 300.0f), Vec2(240.0f, 420.0f),
	//	Vec2(240.0f, 420.0f), Vec2(80.0f, 300.0f), Vec2(240.0f, 300.0f),
	//};
	//Polygon2D* polygonNode = new Polygon2D();
	//isSucceeded = polygonNode->initWithVertexArray(polygonVertices);
	//assert(isSucceeded);

	Sprite2D* spriteNode = new Sprite2D();
	isSucceeded = spriteNode->init("../Resources/Hello.png");
	spriteNode->setPosition(Vec3(400.0f, 300.0f, 0.0f));
	assert(isSucceeded);

	std::vector<Point3DData> positionAndPointSize3D {
		Point3DData(320.0f, 240.0f, -240.0f, 15.0f),
		Point3DData(80.0f, 80.0f, 240.0f, 15.0f),
	};
	Point3D* point3DNode = new Point3D();
	point3DNode->initWithPointArray(positionAndPointSize3D);

	std::vector<Vec3> lineVertices3D {
		Vec3(0.0f, 240.0f, -360.0f), Vec3(640.0f, 240.0f, 360.0f),
		Vec3(320.0f, 480.0f, 360.0f), Vec3(320.0f, 0.0f, -360.0f),
	};
	Line3D* line3DNode = new Line3D();
	isSucceeded = line3DNode->initWithVertexArray(lineVertices3D);
	assert(isSucceeded);

	std::vector<Vec3> polygonVertices3D {
		Vec3(80.0f, 420.0f, 120.0f), Vec3(80.0f, 300.0f, -120.0f), Vec3(240.0f, 420.0f, 120.0f),
		Vec3(240.0f, 420.0f, -120.0f), Vec3(-80.0f, 300.0f, 120.0f), Vec3(240.0f, 420.0f, -120.0f),
	};
	Polygon3D* polygon3DNode = new Polygon3D();
	isSucceeded = polygon3DNode->initWithVertexArray(polygonVertices3D);
	assert(isSucceeded);

	Sprite3D* sprite3DObjNode = new Sprite3D();
	isSucceeded = sprite3DObjNode->initWithModel("../Resources/boss1.obj");
	sprite3DObjNode->setScale(5.0f);
	assert(isSucceeded);
	sprite3DObjNode->setTexture("../Resources/boss.png");

	Sprite3D* sprite3DC3tNode = new Sprite3D();
	isSucceeded = sprite3DC3tNode->initWithModel("../Resources/orc.c3b");
	sprite3DC3tNode->setPosition(Vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f - 100, WINDOW_HEIGHT / 1.1566f)); // �J�����̃f�t�H���g�̎��_�ʒu�ɒu����
	sprite3DC3tNode->setRotation(Vec3(0.0f, 180.0f, 0.0f));
	sprite3DC3tNode->setScale(10.0f);
	sprite3DC3tNode->startAnimation("Take 001", true);
	assert(isSucceeded);

	Scene* scene = new Scene();
	scene->init();
	//scene->pushNode(pointNode);
	//scene->pushNode(lineNode);
	//scene->pushNode(polygonNode);
	//scene->pushNode(spriteNode);
	//scene->pushNode(point3DNode);
	//scene->pushNode(line3DNode);
	//scene->pushNode(polygon3DNode);
	//scene->pushNode(sprite3DObjNode);
	scene->pushNode(sprite3DC3tNode);

	Director::getInstance()->setScene(*scene);
}

static float cameraAngle = 30.0; //TODO:obj��c3t�������₷���ʒu

void render()
{
	const Vec3& cameraPos = Director::getCamera().getPosition();
	//cameraAngle += 0.0001;
	//cameraAngle += 0.001; // TODO:�A�j���[�V�����Ńp�t�H�[�}���X�����Ă�̂ł��傤���Ȃ���]�p�𑝂₵�Ă���
	cameraAngle += 0.003; // TODO:�A�j���[�V�����Ńp�t�H�[�}���X�����Ă�̂ł��傤���Ȃ���]�p�𑝂₵�Ă���
	Vec3 newCameraPos = Vec3(WINDOW_HEIGHT * sin(cameraAngle) + WINDOW_WIDTH / 2.0f, cameraPos.y, WINDOW_HEIGHT * cos(cameraAngle) + WINDOW_HEIGHT / 2.0f);
	Director::getCamera().setPosition(newCameraPos);

	// MGRRenderer�ɖ��t���[���̕`�施��
	Director::getInstance()->update();
}

void finalize() {
	// MGRRenderer�I������
	Director::getInstance()->destroy();
}
