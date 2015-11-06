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

// 宣言
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

	// OpenGL3.2のサンプルでは使用していたが、APIが変わったのかこれを書いているとglCreateShader(GL_VERTEX_SHADER)でGL_INVALID_ENUMエラーが返：る
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

	// 描画の初期化
	initialize();

	while (glfwWindowShouldClose(window) == GL_FALSE)
	{
		// 描画のメインループ
		render();

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	// 描画の終了処理
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
	// 描画するシーンをMGRRendererに登録している
	//
	bool isSucceeded = false;

	Director::getInstance()->init(Size(WINDOW_WIDTH, WINDOW_HEIGHT));

	// 各ノードの作成はDirector::initの後に呼ぶ。Director::initのもっているウィンドウサイズを使用する場合があるので。

	//std::vector<Point2DData> positionAndPointSize{
	//	Point2DData(0.0f, 0.0f, 15.0f),
	//	Point2DData(-0.75f, -0.75f, 15.0f),
	//};
	//Point2D* pointNode = new Point2D();
	//pointNode->initWithPointArray(positionAndPointSize);

	//std::vector<Vec2> lineVertices{
	//	Vec2(-1.0f, 0.0f), Vec2(1.0f, 0.0f),
	//	Vec2(0.0f, 1.0f), Vec2(0.0f, -1.0f),
	//};
	//Line2D* lineNode = new Line2D();
	//isSucceeded = lineNode->initWithVertexArray(lineVertices);
	//assert(isSucceeded);

	//std::vector<Vec2> polygonVertices{
	//	Vec2(-0.75f, 0.75f), Vec2(-0.75f, 0.25f), Vec2(-0.25f, 0.75f),
	//	Vec2(-0.25f, 0.75f), Vec2(-0.75f, 0.25f), Vec2(-0.25f, 0.25f),
	//};
	//Polygon2D* polygonNode = new Polygon2D();
	//isSucceeded = polygonNode->initWithVertexArray(polygonVertices);
	//assert(isSucceeded);

	Sprite2D* spriteNode = new Sprite2D();
	isSucceeded = spriteNode->init(Vec2(0.25f, 0.25f), "../Resources/Hello.png");
	assert(isSucceeded);

	std::vector<Point3DData> positionAndPointSize3D {
		Point3DData(0.0f, 0.0f, -0.5f, 15.0f),
		Point3DData(-0.75f, -0.75f, 0.5f, 15.0f),
	};
	Point3D* point3DNode = new Point3D();
	point3DNode->initWithPointArray(positionAndPointSize3D);

	std::vector<Vec3> lineVertices3D {
		Vec3(-1.0f, 0.0f, -0.75f), Vec3(1.0f, 0.0f, 0.75f),
		Vec3(0.0f, 1.0f, 0.75f), Vec3(0.0f, -1.0f, -0.75f),
	};
	Line3D* line3DNode = new Line3D();
	isSucceeded = line3DNode->initWithVertexArray(lineVertices3D);
	assert(isSucceeded);

	std::vector<Vec3> polygonVertices3D {
		Vec3(-0.75f, 0.75f, 0.25f), Vec3(-0.75f, 0.25f, -0.25f), Vec3(-0.25f, 0.75f, 0.25f),
		Vec3(-0.25f, 0.75f, -0.25f), Vec3(-0.75f, 0.25f, 0.25f), Vec3(-0.25f, 0.25f, -0.25f),
	};
	Polygon3D* polygon3DNode = new Polygon3D();
	isSucceeded = polygon3DNode->initWithVertexArray(polygonVertices3D);
	assert(isSucceeded);

	Scene* scene = new Scene();
	scene->init();
	//scene->pushNode(pointNode);
	//scene->pushNode(lineNode);
	//scene->pushNode(polygonNode);
	scene->pushNode(spriteNode);
	scene->pushNode(point3DNode);
	scene->pushNode(line3DNode);
	scene->pushNode(polygon3DNode);

	Director::getInstance()->setScene(*scene);
}

static float cameraAngle = 0;

void render()
{
	const Vec3& cameraPos = Director::getCamera().getPosition();
	cameraAngle += 0.0001;
	Vec3 newCameraPos = Vec3(-5.0f * sin(cameraAngle), cameraPos.z, -5.0f * cos(cameraAngle));
	Director::getCamera().setPosition(newCameraPos);

	// MGRRendererに毎フレームの描画命令
	Director::getInstance()->update();
}

void finalize() {
	// MGRRenderer終了処理
	Director::getInstance()->destroy();
}
