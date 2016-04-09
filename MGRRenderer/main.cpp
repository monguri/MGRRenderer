#include <iostream>
#include "Config.h"
#include "MGRRenderer.h"

#if defined(MGRRENDERER_USE_DIRECT3D)
#include <d3dx11.h>
#include <dxerr.h>
#elif defined(MGRRENDERER_USE_OPENGL)
#include <gles/include/glew.h>
#include <glfw3/include/glfw3.h>
#endif


#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>
#include <tchar.h>

static const int WINDOW_WIDTH = 640;
static const int WINDOW_HEIGHT = 480;
static const int FPS = 60;

using namespace mgrrenderer;

// 宣言
#if defined(MGRRENDERER_USE_DIRECT3D)
LRESULT CALLBACK mainWindowProc(HWND handleWindow, UINT message, UINT windowParam, LONG param);
#elif defined(MGRRENDERER_USE_OPENGL)
static void fwErrorHandler(int error, const char* description);
static void fwKeyInputHandler(GLFWwindow* window, int key, int scancode, int action, int mods);
#endif
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
	// デバッグ ヒープ マネージャによるメモリ割り当ての追跡方法を設定
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

#if defined(MGRRENDERER_USE_DIRECT3D)
	WCHAR windowClass[] = L"MGRRendererSampleApplication";
	IDXGISwapChain* swapChain = nullptr;

	// ウインドウ クラスの登録
	WNDCLASS wndClass;
	wndClass.style   = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc  = (WNDPROC)mainWindowProc;
	wndClass.cbClsExtra  = 0;
	wndClass.cbWndExtra  = 0;
	wndClass.hInstance  = hInstance;
	wndClass.hIcon   = LoadIcon(nullptr, IDI_APPLICATION);
	wndClass.hCursor   = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wndClass.lpszMenuName  = nullptr;
	wndClass.lpszClassName = windowClass;

	if (!RegisterClass(&wndClass))
	{
		std::cerr << "Error:" << GetLastError() <<  " RegisterClass failed." << std::endl;
		exit(EXIT_FAILURE);
	}

	// メインウィンドウ作成
	RECT rect;
	rect.top = 0;
	rect.left = 0;
	rect.right = WINDOW_WIDTH;
	rect.bottom = WINDOW_HEIGHT;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);

	HWND handleWindow = CreateWindow(
		windowClass,
		L"MGRRendererSampleApplication",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr
		);
	if (handleWindow == nullptr)
	{
		std::cerr << "Error:" << GetLastError() << " CreateWindow failed." << std::endl;
	}

	// ウインドウ表示
	ShowWindow(handleWindow, SW_SHOWNORMAL);
	UpdateWindow(handleWindow);

#elif defined(MGRRENDERER_USE_OPENGL)
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
#endif

	// 描画の初期化
	initialize();

	// FPSから1ループの長さの計算
	LARGE_INTEGER nFreq;
	QueryPerformanceFrequency(&nFreq);
	LARGE_INTEGER loopInterval;
	loopInterval.QuadPart = (LONGLONG)(1.0 / FPS * nFreq.QuadPart);

	LARGE_INTEGER nLast;
	LARGE_INTEGER nNow;
	QueryPerformanceCounter(&nLast);

#if defined(MGRRENDERER_USE_DIRECT3D)
	MSG msg;
	PeekMessage(&msg, 0, 0, 0, PM_REMOVE);

	while (msg.message != WM_QUIT)
#elif defined(MGRRENDERER_USE_OPENGL)
	while (glfwWindowShouldClose(window) == GL_FALSE)
#endif
	{
		QueryPerformanceCounter(&nNow);
		if (nNow.QuadPart - nLast.QuadPart > loopInterval.QuadPart)
		{
			nLast.QuadPart = nNow.QuadPart - (nNow.QuadPart % loopInterval.QuadPart); // 余り分も次回計算で用いる。整数計算にしているので精度が高い

			// 描画のメインループ
			render();

#if defined(MGRRENDERER_USE_DIRECT3D)
			//swapChain->Present(0, 0); // TODO:もしかして、第一引数に秒数入れれば、GLFWみたいに自分でFPSコントロールしなくていいのかも

			// メッセージポーリング
			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
#elif defined(MGRRENDERER_USE_OPENGL)
			// バックバッファとフロントバッファ入れ替え
			glfwSwapBuffers(window);

			// イベントポーリング
			glfwPollEvents();
#endif
		}
		else
		{
			Sleep(1);
		}
	}

	// 描画の終了処理
	finalize();

#if defined(MGRRENDERER_USE_DIRECT3D)
	UnregisterClass(windowClass, hInstance);
#elif defined(MGRRENDERER_USE_OPENGL)
	glfwDestroyWindow(window);
	glfwTerminate();
#endif
	exit(EXIT_SUCCESS);
	return 0;
}

#if defined(MGRRENDERER_USE_DIRECT3D)
LRESULT CALLBACK mainWindowProc(HWND handleWindow, UINT message, UINT windowParam, LONG param)
{
	//TODO:実装
	return DefWindowProc(handleWindow, message, windowParam, param);
}
#elif defined(MGRRENDERER_USE_OPENGL)
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
#endif

void initialize()
{
	//
	// 描画するシーンをMGRRendererに登録している
	//
	bool isSucceeded = false;

	Director::getInstance()->init(Size(WINDOW_WIDTH, WINDOW_HEIGHT));
	Director::getInstance()->setDisplayStats(true);

	// 各ノードの作成はDirector::initの後に呼ぶ。Director::initのもっているウィンドウサイズを使用する場合があるので。

	std::vector<Point2DData> positionAndPointSize{
		Point2DData(320.0f, 240.0f, 15.0f),
		Point2DData(80.0f, 60.0f, 15.0f),
	};
	Point2D* pointNode = new Point2D();
	pointNode->setColor(Color3B::RED);
	pointNode->initWithPointArray(positionAndPointSize);

	std::vector<Vec2> lineVertices{
		Vec2(0.0f, 240.0f), Vec2(640.0f, 240.0f),
		Vec2(320.0f, 480.0f), Vec2(320.0f, 0.0f),
	};
	Line2D* lineNode = new Line2D();
	lineNode->setColor(Color3B::GREEN);
	isSucceeded = lineNode->initWithVertexArray(lineVertices);
	Logger::logAssert(isSucceeded, "ノードの初期化失敗");

	std::vector<Vec2> polygonVertices{
		Vec2(80.0f, 420.0f), Vec2(80.0f, 300.0f), Vec2(240.0f, 420.0f),
		Vec2(240.0f, 420.0f), Vec2(80.0f, 300.0f), Vec2(240.0f, 300.0f),
	};
	Polygon2D* polygonNode = new Polygon2D();
	polygonNode->setColor(Color3B::BLUE);
	isSucceeded = polygonNode->initWithVertexArray(polygonVertices);
	Logger::logAssert(isSucceeded, "ノードの初期化失敗");

#if defined(MGRRENDERER_USE_OPENGL)
	Sprite2D* spriteNode = new Sprite2D();
	isSucceeded = spriteNode->init("../Resources/Hello.png");
	spriteNode->setPosition(Vec3(400.0f, 300.0f, 0.0f));
	Logger::logAssert(isSucceeded, "ノードの初期化失敗");
#endif
	//// TODO:現状表示に成功してない
	//BillBoard* spriteNode = new BillBoard();
	//isSucceeded = spriteNode->init("../Resources/Hello.png", BillBoard::Mode::VIEW_PLANE_ORIENTED);
	//spriteNode->setPosition(Vec3(400.0f, 300.0f, 0.0f));
	//Logger::logAssert(isSucceeded, "ノードの初期化失敗");


	std::vector<Point3DData> positionAndPointSize3D {
		Point3DData(320.0f, 240.0f, -240.0f, 15.0f),
		Point3DData(80.0f, 80.0f, 240.0f, 15.0f),
	};
	Point3D* point3DNode = new Point3D();
	point3DNode->setColor(Color3B::RED);
	point3DNode->initWithPointArray(positionAndPointSize3D);

	std::vector<Vec3> lineVertices3D {
		Vec3(0.0f, 240.0f, -360.0f), Vec3(640.0f, 240.0f, 360.0f),
		Vec3(320.0f, 480.0f, 360.0f), Vec3(320.0f, 0.0f, -360.0f),
	};
	Line3D* line3DNode = new Line3D();
	line3DNode->setColor(Color3B::GREEN);
	isSucceeded = line3DNode->initWithVertexArray(lineVertices3D);
	Logger::logAssert(isSucceeded, "ノードの初期化失敗");

	std::vector<Vec3> polygonVertices3D {
		Vec3(80.0f, 420.0f, 120.0f), Vec3(80.0f, 300.0f, -120.0f), Vec3(240.0f, 420.0f, 120.0f),
	};
	Polygon3D* polygon3DNode = new Polygon3D();
	polygon3DNode->setColor(Color3B::BLUE);
	isSucceeded = polygon3DNode->initWithVertexArray(polygonVertices3D);
	Logger::logAssert(isSucceeded, "ノードの初期化失敗");

	// 1.0fずつ境界線をずらすことで境界線を見えるようにしている
	std::vector<Vec3> planeVertices3D1 {
		Vec3(WINDOW_WIDTH / 2.0f + 320.0f, -1.0f, 320.0f), Vec3(WINDOW_WIDTH / 2.0f + 320.0f, -1.0f, -320.0f), Vec3(WINDOW_WIDTH / 2.0f - 320.0f, -1.0f, 320.0f),
		Vec3(WINDOW_WIDTH / 2.0f - 320.0f, -1.0f, -320.0f), Vec3(WINDOW_WIDTH / 2.0f - 320.0f, -1.0f, 320.0f), Vec3(WINDOW_WIDTH / 2.0f + 320.0f, -1.0f, -320.0f),
	};
	Polygon3D* plane3DNode1 = new Polygon3D();
	isSucceeded = plane3DNode1->initWithVertexArray(planeVertices3D1);
	Logger::logAssert(plane3DNode1, "ノードの初期化失敗");

	std::vector<Vec3> planeVertices3D2 {
		Vec3(WINDOW_WIDTH / 2.0f - 320.0f, 1.0f, 320.0f), Vec3(WINDOW_WIDTH / 2.0f - 320.0f, 1.0f, -320.0f + 1.0f), Vec3(WINDOW_WIDTH / 2.0f - 320.0f, 320.0f, 320.0f),
		Vec3(WINDOW_WIDTH / 2.0f - 320.0f, 320.0f, -320.0f + 1.0f), Vec3(WINDOW_WIDTH / 2.0f - 320.0f, 320.0f, 320.0f), Vec3(WINDOW_WIDTH / 2.0f - 320.0f, 1.0f, -320.0f + 1.0f),
	};
	Polygon3D* plane3DNode2 = new Polygon3D();
	isSucceeded = plane3DNode2->initWithVertexArray(planeVertices3D2);
	Logger::logAssert(plane3DNode2, "ノードの初期化失敗");

	std::vector<Vec3> planeVertices3D3 {
		Vec3(WINDOW_WIDTH / 2.0f - 320.0f + 1.0f, 1.0f, -320.0f), Vec3(WINDOW_WIDTH / 2.0f + 320.0f, 1.0f, -320.0f), Vec3(WINDOW_WIDTH / 2.0f - 320.0f + 1.0f, 320.0f, -320.0f),
		Vec3(WINDOW_WIDTH / 2.0f + 320.0f, 320.0f, -320.0f), Vec3(WINDOW_WIDTH / 2.0f - 320.0f + 1.0f, 320.0f, -320.0f), Vec3(WINDOW_WIDTH / 2.0f + 320.0f, 1.0f, -320.0f),
	};
	Polygon3D* plane3DNode3 = new Polygon3D();
	isSucceeded = plane3DNode3->initWithVertexArray(planeVertices3D3);
	Logger::logAssert(plane3DNode3, "ノードの初期化失敗");

#if defined(MGRRENDERER_USE_OPENGL)
	Sprite3D* sprite3DObjNode = new Sprite3D();
	isSucceeded = sprite3DObjNode->initWithModel("../Resources/boss1.obj");
	sprite3DObjNode->setPosition(Vec3(WINDOW_WIDTH / 2.0f - 100, WINDOW_HEIGHT / 2.0f - 100, 0)); // カメラのデフォルトの視点位置から少しずれた場所に置いた
	sprite3DObjNode->setScale(10.0f);
	Logger::logAssert(isSucceeded, "ノードの初期化失敗");
	sprite3DObjNode->setTexture("../Resources/boss.png");

	Sprite3D* sprite3DC3tNode = new Sprite3D();
	isSucceeded = sprite3DC3tNode->initWithModel("../Resources/orc.c3b");
	sprite3DC3tNode->setPosition(Vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0)); // カメラのデフォルトの視点位置に置いた
	sprite3DC3tNode->setRotation(Vec3(0.0f, 180.0f, 0.0f));
	sprite3DC3tNode->setScale(10.0f);
	sprite3DC3tNode->startAnimation("Take 001", true);
	Logger::logAssert(isSucceeded, "ノードの初期化失敗");
#endif

	Particle3D::Parameter parameter;
	parameter.loopFlag = true;
	parameter.gravity = Vec3(0.0f, -100.0f, 0.0f);
	parameter.initVelocity = 300.0f;
	parameter.lifeTime = 10.0f;
	parameter.numParticle = 100;
	parameter.textureFilePath = "../Resources/bluewater.png";
	parameter.pointSize = 5.0f;

	Particle3D* particle3DNode = new Particle3D();
	isSucceeded = particle3DNode->initWithParameter(parameter);
	particle3DNode->setPosition(Vec3(WINDOW_WIDTH / 2.0f - 200.0f, WINDOW_HEIGHT / 2.0f - 300.0f, 0));
	Logger::logAssert(isSucceeded, "ノードの初期化失敗");

	Scene* scene = new Scene();
	scene->init();
	Light* defaultLight = scene->getDefaultLight();
	defaultLight->setIntensity(0.3f);
	defaultLight->setColor(Color3B::WHITE);

#if defined(MGRRENDERER_USE_OPENGL)
	DirectionalLight* directionalLight = new (std::nothrow) DirectionalLight(Vec3(-1.0f, -1.0f, -1.0f), Color3B::WHITE);
	directionalLight->setIntensity(0.7f);
	directionalLight->prepareShadowMap(sprite3DC3tNode->getPosition(), WINDOW_HEIGHT / 1.1566f, Size(WINDOW_WIDTH, WINDOW_HEIGHT));
	scene->addLight(directionalLight);
#endif

	// TODO:シャドウマップをスプライトで描画したかったが機能してない
#if defined(MGRRENDERER_USE_OPENGL)
	Sprite2D* depthTextureSprite = new Sprite2D();
	const Size& contentSize = Director::getInstance()->getWindowSize() / 4.0f;
	depthTextureSprite->initWithTexture(directionalLight->getShadowMapData().textureId, contentSize, Texture::PixelFormat::RGBA8888); // pixelformatは適当
	depthTextureSprite->setPosition(Vec3(WINDOW_WIDTH - contentSize.width, 0.0f, 0.0f));
#endif


	////PointLight* pointLight = new (std::nothrow) PointLight(Vec3(1000, 1000, 1000), Color3B::WHITE, 100000);
	////pointLight->setIntensity(0.7f);
	////scene->addLight(pointLight);

	//SpotLight* spotLight = new (std::nothrow) SpotLight(Vec3(1000, 1000, 1000), Vec3(-1.0f, -1.0f, -1.0f), Color3B::WHITE, 10000, 0.0, 0.5);
	//spotLight->setIntensity(0.7f);
	//scene->addLight(spotLight);

	scene->pushNode(point3DNode);
	scene->pushNode(line3DNode);
	scene->pushNode(polygon3DNode);
	scene->pushNode(plane3DNode1);
	scene->pushNode(plane3DNode2);
	scene->pushNode(plane3DNode3);
#if defined(MGRRENDERER_USE_OPENGL)
	scene->pushNode(sprite3DObjNode);
	scene->pushNode(sprite3DC3tNode);
#endif
	scene->pushNode(particle3DNode);
	scene->pushNode(pointNode);
	scene->pushNode(lineNode);
	scene->pushNode(polygonNode);
#if defined(MGRRENDERER_USE_OPENGL)
	scene->pushNode(spriteNode);
	scene->pushNode(depthTextureSprite); // TODO:深度テクスチャをうまく表示できない
#endif

	Director::getInstance()->setScene(*scene);
}

static float cameraAngle = 0.0f; //TODO:objとc3tが見えやすい位置

void render()
{
	const Vec3& cameraPos = Director::getCamera().getPosition();
	cameraAngle += 0.003;
	Vec3 newCameraPos = Vec3(WINDOW_HEIGHT / 1.1566f * sin(cameraAngle) + WINDOW_WIDTH / 2.0f, cameraPos.y, WINDOW_HEIGHT / 1.1566f * cos(cameraAngle));
	Director::getCamera().setPosition(newCameraPos);

	// MGRRendererに毎フレームの描画命令
	Director::getInstance()->update();
}

void finalize() {
	// MGRRenderer終了処理
	Director::getInstance()->destroy();
}
