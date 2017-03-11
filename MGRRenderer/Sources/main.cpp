#include <iostream>
#include "Config.h"
#include "MGRRenderer.h"

#if defined(MGRRENDERER_USE_DIRECT3D)
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#elif defined(MGRRENDERER_USE_OPENGL)
#define GLEW_STATIC
#include <glew/include/glew.h>
#include <glfw3/include/glfw3.h>
#endif

// ����STRICT�͒�`����Ă���悤��
//#define STRICT					// �^�`�F�b�N�������ɍs�Ȃ�
#define WIN32_LEAN_AND_MEAN		// �w�b�_�[���炠�܂�g���Ȃ��֐����Ȃ�

// Windows Header Files:
#include <windows.h>
#include <tchar.h>

static const int WINDOW_WIDTH = 960;
static const int WINDOW_HEIGHT = 720;
static const float NEAR_CLIP = 10.0f;
static const float FAR_CLIP = WINDOW_WIDTH * 2.0f;
static const int FPS = 60;

using namespace mgrrenderer;

// �錾
#if defined(MGRRENDERER_USE_DIRECT3D)
LRESULT CALLBACK mainWindowProc(HWND handleWindow, UINT message, UINT windowParam, LONG param);
static void initialize(HWND handleWindow);
#elif defined(MGRRENDERER_USE_OPENGL)
static void fwErrorHandler(int error, const char* description);
static void fwKeyInputHandler(GLFWwindow* window, int key, int scancode, int action, int mods);
void APIENTRY debugMessageHandler(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam);
static void initialize();
#endif
static void render();
static void finalize();

int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	(void)nCmdShow; // ���g�p�ϐ��x���}��
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	// �f�o�b�O �q�[�v �}�l�[�W���ɂ�郁�������蓖�Ă̒ǐՕ��@��ݒ�
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

#if defined(MGRRENDERER_USE_DIRECT3D)
	WCHAR windowClass[] = L"MGRRendererSampleApplication";

	// �E�C���h�E �N���X�̓o�^
	WNDCLASS wndClass;
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = (WNDPROC)mainWindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = windowClass;

	if (!RegisterClass(&wndClass))
	{
		std::cerr << "Error:" << GetLastError() << " RegisterClass failed." << std::endl;
		PostQuitMessage(EXIT_FAILURE);
	}

	// ���C���E�B���h�E�쐬
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
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);
	if (handleWindow == nullptr)
	{
		std::cerr << "Error:" << GetLastError() << " CreateWindow failed." << std::endl;
		PostQuitMessage(EXIT_FAILURE);
	}

	// �E�C���h�E�\��
	ShowWindow(handleWindow, SW_SHOWNORMAL);
	UpdateWindow(handleWindow);

	// IDXGIFactory�C���^�[�t�F�C�X�̎擾
	IDXGIFactory* dxgiFactory = nullptr;
	HRESULT result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
	if (FAILED(result))
	{
		std::cerr << "Error:" << GetLastError() << " CreateDXGIFactory failed." << std::endl;
		PostQuitMessage(EXIT_FAILURE);
	}

	// [Alt]+[Enter]�L�[�ɂ���ʃ��[�h�؂�ւ��@�\��ݒ肷��
	result = dxgiFactory->MakeWindowAssociation(handleWindow, 0);
	if (FAILED(result))
	{
		std::cerr << "Error:" << GetLastError() << " CreateDXGIFactory failed." << std::endl;
		PostQuitMessage(EXIT_FAILURE);
	}
#elif defined(MGRRENDERER_USE_OPENGL)
	(void)hInstance; // ���g�p�ϐ��x���}��

	glfwSetErrorCallback(fwErrorHandler);

	if (glfwInit() == GL_FALSE)
	{
		std::cerr << "Can't initilize GLFW" << std::endl;
		exit(EXIT_FAILURE);
	}

	// OpenGL3.2�̃T���v���ł͎g�p���Ă������AAPI���ς�����̂�����������Ă����glCreateShader(GL_VERTEX_SHADER)��GL_INVALID_ENUM�G���[���Ԃ�
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	GLint flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if ((flags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(debugMessageHandler, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
	}

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

#if defined(MGRRENDERER_USE_DIRECT3D)
	// �`��̏�����
	initialize(handleWindow);

	// ���C�����[�v
	MSG msg;

	// TODO:60FPS�ŉ񂷏����������ĂȂ�
	do
	{
		// ���b�Z�[�W�|�[�����O
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// �`��̃��C�����[�v
			render();

			Director::getRenderer().getDirect3dSwapChain()->Present(0, 0);
		}
	} while (msg.message != WM_QUIT);
#elif defined(MGRRENDERER_USE_OPENGL)
	// �`��̏�����
	initialize();

	// FPS����1���[�v�̒����̌v�Z
	LARGE_INTEGER nFreq;
	QueryPerformanceFrequency(&nFreq);
	LARGE_INTEGER loopInterval;
	loopInterval.QuadPart = (LONGLONG)(1.0 / FPS * nFreq.QuadPart);

	LARGE_INTEGER nLast;
	LARGE_INTEGER nNow;
	QueryPerformanceCounter(&nLast);

	while (glfwWindowShouldClose(window) == GL_FALSE)
	{
		QueryPerformanceCounter(&nNow);
		if (nNow.QuadPart - nLast.QuadPart > loopInterval.QuadPart)
		{
			nLast.QuadPart = nNow.QuadPart - (nNow.QuadPart % loopInterval.QuadPart); // �]�蕪������v�Z�ŗp����B�����v�Z�ɂ��Ă���̂Ő��x������

			// �`��̃��C�����[�v
			render();

			// �o�b�N�o�b�t�@�ƃt�����g�o�b�t�@����ւ�
			glfwSwapBuffers(window);

			// �C�x���g�|�[�����O
			glfwPollEvents();
		}
		else
		{
			Sleep(1);
		}
	}
#endif

	// �`��̏I������
	finalize();

#if defined(MGRRENDERER_USE_DIRECT3D)
	UnregisterClass(windowClass, hInstance);
#elif defined(MGRRENDERER_USE_OPENGL)
	glfwDestroyWindow(window);
	glfwTerminate();
#endif
	return 0;
}

#if defined(MGRRENDERER_USE_DIRECT3D)
LRESULT CALLBACK mainWindowProc(HWND handleWindow, UINT message, UINT windowParam, LONG param)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		// TODO:��������������ĂȂ�
		return 0;
	default:
		//TODO: ���ɉ�������̂��m��Ȃ��̂ŏ������Ȃ�
		break;
	}
	return DefWindowProc(handleWindow, message, windowParam, param);
}
#elif defined(MGRRENDERER_USE_OPENGL)
void fwErrorHandler(int error, const char* description)
{
	(void)error; // ���g�p�ϐ��x���}��
	std::cerr << "glfw Error: " << description << std::endl;
}

void fwKeyInputHandler(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	(void)scancode; // ���g�p�ϐ��x���}��
	(void)mods; // ���g�p�ϐ��x���}��

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

void APIENTRY debugMessageHandler(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
	(void)source;
	(void)type;
	(void)id;
	(void)severity;
	(void)length;
	(void)userParam;
	Logger::logAssert(false, message);
}
#endif

#if defined(MGRRENDERER_USE_DIRECT3D)
void initialize(HWND handleWindow)
#elif defined(MGRRENDERER_USE_OPENGL)
void initialize()
#endif
{
	//
	// �`�悷��V�[����MGRRenderer�ɓo�^���Ă���
	//
	bool isSucceeded = false;

#if defined(MGRRENDERER_USE_DIRECT3D)
	Director::getInstance()->init(handleWindow, SizeUint(WINDOW_WIDTH, WINDOW_HEIGHT), NEAR_CLIP, FAR_CLIP);
#elif defined(MGRRENDERER_USE_OPENGL)
	Director::getInstance()->init(SizeUint(WINDOW_WIDTH, WINDOW_HEIGHT), NEAR_CLIP, FAR_CLIP);
#endif
	Director::getInstance()->setDisplayStats(true);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	Director::getInstance()->setDisplayGBuffer(true);
#endif

	// �e�m�[�h�̍쐬��Director::init�̌�ɌĂԁBDirector::init�̂����Ă���E�B���h�E�T�C�Y���g�p����ꍇ������̂ŁB

	std::vector<Point2DData> positionAndPointSize{
		Point2DData(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 15.0f),
		Point2DData(80.0f, 60.0f, 15.0f),
	};
	Point2D* pointNode = new Point2D();
	pointNode->setColor(Color3B::RED);
	pointNode->initWithPointArray(positionAndPointSize);

	std::vector<Vec2> lineVertices{
		Vec2(0.0f, WINDOW_HEIGHT / 2.0f), Vec2((float)WINDOW_WIDTH, WINDOW_HEIGHT / 2.0f),
		Vec2(WINDOW_WIDTH / 2.0f, (float)WINDOW_HEIGHT), Vec2(WINDOW_WIDTH / 2.0f, 0.0f),
	};
	Line2D* lineNode = new Line2D();
	lineNode->setColor(Color3B::GREEN);
	isSucceeded = lineNode->initWithVertexArray(lineVertices);
	Logger::logAssert(isSucceeded, "�m�[�h�̏��������s");

	std::vector<Vec2> polygonVertices{
		Vec2(80.0f, 420.0f), Vec2(80.0f, 300.0f), Vec2(240.0f, 420.0f),
		Vec2(240.0f, 420.0f), Vec2(80.0f, 300.0f), Vec2(240.0f, 300.0f),
	};
	Polygon2D* polygonNode = new Polygon2D();
	polygonNode->setColor(Color3B::BLUE);
	isSucceeded = polygonNode->initWithVertexArray(polygonVertices);
	Logger::logAssert(isSucceeded, "�m�[�h�̏��������s");

	//Sprite2D* spriteNode = new Sprite2D();
	//isSucceeded = spriteNode->init("../MGRRenderer/Resources/Hello.png");
	//spriteNode->setPosition(Vec3(400.0f, 300.0f, 0.0f));
	//Logger::logAssert(isSucceeded, "�m�[�h�̏��������s");
	BillBoard* spriteNode = new BillBoard();
	isSucceeded = spriteNode->init("../MGRRenderer/Resources/Hello.png", BillBoard::Mode::VIEW_PLANE_ORIENTED);
	//spriteNode->setOpacity(0.5f);
	spriteNode->setPosition(Vec3(WINDOW_WIDTH / 2.0f + 80.0f, WINDOW_HEIGHT / 2.0f + 60.0f, 80.0f));
	Logger::logAssert(isSucceeded, "�m�[�h�̏��������s");


	std::vector<Point3DData> positionAndPointSize3D{
		Point3DData(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, -WINDOW_HEIGHT / 2.0f, 15.0f),
		Point3DData(80.0f, 80.0f, WINDOW_HEIGHT / 2.0f, 15.0f),
	};
	Point3D* point3DNode = new Point3D();
	point3DNode->setColor(Color3B::RED);
	point3DNode->initWithPointArray(positionAndPointSize3D);

	std::vector<Vec3> lineVertices3D{
		Vec3(0.0f, WINDOW_HEIGHT / 2.0f, -360.0f), Vec3((float)WINDOW_WIDTH, WINDOW_HEIGHT / 2.0f, 360.0f),
		Vec3(WINDOW_WIDTH / 2.0f, (float)WINDOW_HEIGHT, 360.0f), Vec3(WINDOW_WIDTH / 2.0f, 0.0f, -360.0f),
	};
	Line3D* line3DNode = new Line3D();
	line3DNode->setColor(Color3B::GREEN);
	isSucceeded = line3DNode->initWithVertexArray(lineVertices3D);
	Logger::logAssert(isSucceeded, "�m�[�h�̏��������s");

	std::vector<Vec3> polygonVertices3D{
		Vec3(80.0f, 420.0f, 120.0f), Vec3(80.0f, 300.0f, -120.0f), Vec3(240.0f, 420.0f, 120.0f),
	};
	Polygon3D* polygon3DNode = new Polygon3D();
	polygon3DNode->setColor(Color3B::BLUE);
	isSucceeded = polygon3DNode->initWithVertexArray(polygonVertices3D);
	Logger::logAssert(isSucceeded, "�m�[�h�̏��������s");

	// y����񂩂猩���낵���}
	// ============== x�����s
	// ||           |
	// ||           |
	// ||           |
	// ||___________|
	// z�����s

	// 1.0f�����E�������炷���Ƃŋ��E����������悤�ɂ��Ă���
	// (WINDOW_WIDTH / 3.0f - WINDOW_WIDTH / 3.0f = 0, 0, -WINDOW_WIDTH / 3.0f)��3�̂����Ă̒��S�ƂȂ�
	// y���ɐ����ȕ���
	std::vector<Vec3> planeVertices3D1{
		Vec3(WINDOW_WIDTH / 3.0f + WINDOW_WIDTH / 3.0f, -1.0f, WINDOW_WIDTH / 3.0f), Vec3(WINDOW_WIDTH / 3.0f + WINDOW_WIDTH / 3.0f, -1.0f, -WINDOW_WIDTH / 3.0f), Vec3(WINDOW_WIDTH / 3.0f - WINDOW_WIDTH / 3.0f, -1.0f, WINDOW_WIDTH / 3.0f),
		Vec3(WINDOW_WIDTH / 3.0f - WINDOW_WIDTH / 3.0f, -1.0f, -WINDOW_WIDTH / 3.0f), Vec3(WINDOW_WIDTH / 3.0f - WINDOW_WIDTH / 3.0f, -1.0f, WINDOW_WIDTH / 3.0f), Vec3(WINDOW_WIDTH / 3.0f + WINDOW_WIDTH / 3.0f, -1.0f, -WINDOW_WIDTH / 3.0f),
	};
	Polygon3D* plane3DNode1 = new Polygon3D();
	isSucceeded = plane3DNode1->initWithVertexArray(planeVertices3D1);
	Logger::logAssert(isSucceeded, "�m�[�h�̏��������s");

	// x���ɐ����ȕ���
	std::vector<Vec3> planeVertices3D2{
		Vec3(WINDOW_WIDTH / 3.0f - WINDOW_WIDTH / 3.0f, 1.0f, WINDOW_WIDTH / 3.0f), Vec3(WINDOW_WIDTH / 3.0f - WINDOW_WIDTH / 3.0f, 1.0f, -WINDOW_WIDTH / 3.0f + 1.0f), Vec3(WINDOW_WIDTH / 3.0f - WINDOW_WIDTH / 3.0f, WINDOW_WIDTH / 3.0f, WINDOW_WIDTH / 3.0f),
		Vec3(WINDOW_WIDTH / 3.0f - WINDOW_WIDTH / 3.0f, WINDOW_WIDTH / 3.0f, -WINDOW_WIDTH / 3.0f + 1.0f), Vec3(WINDOW_WIDTH / 3.0f - WINDOW_WIDTH / 3.0f, WINDOW_WIDTH / 3.0f, WINDOW_WIDTH / 3.0f), Vec3(WINDOW_WIDTH / 3.0f - WINDOW_WIDTH / 3.0f, 1.0f, -WINDOW_WIDTH / 3.0f + 1.0f),
	};
	Polygon3D* plane3DNode2 = new Polygon3D();
	isSucceeded = plane3DNode2->initWithVertexArray(planeVertices3D2);
	Logger::logAssert(plane3DNode2 != nullptr, "�m�[�h�̏��������s");

	// z���ɐ����ȕ���
	std::vector<Vec3> planeVertices3D3{
		Vec3(WINDOW_WIDTH / 3.0f - WINDOW_WIDTH / 3.0f + 1.0f, 1.0f, -WINDOW_WIDTH / 3.0f), Vec3(WINDOW_WIDTH / 3.0f + WINDOW_WIDTH / 3.0f, 1.0f, -WINDOW_WIDTH / 3.0f), Vec3(WINDOW_WIDTH / 3.0f - WINDOW_WIDTH / 3.0f + 1.0f, WINDOW_WIDTH / 3.0f, -WINDOW_WIDTH / 3.0f),
		Vec3(WINDOW_WIDTH / 3.0f + WINDOW_WIDTH / 3.0f, WINDOW_WIDTH / 3.0f, -WINDOW_WIDTH / 3.0f), Vec3(WINDOW_WIDTH / 3.0f - WINDOW_WIDTH / 3.0f + 1.0f, WINDOW_WIDTH / 3.0f, -WINDOW_WIDTH / 3.0f), Vec3(WINDOW_WIDTH / 3.0f + WINDOW_WIDTH / 3.0f, 1.0f, -WINDOW_WIDTH / 3.0f),
	};
	Polygon3D* plane3DNode3 = new Polygon3D();
	isSucceeded = plane3DNode3->initWithVertexArray(planeVertices3D3);
	Logger::logAssert(isSucceeded, "�m�[�h�̏��������s");

	Sprite3D* sprite3DObjNode = new Sprite3D();
	isSucceeded = sprite3DObjNode->initWithModel("../MGRRenderer/Resources/boss1.obj");
	sprite3DObjNode->setPosition(Vec3(WINDOW_WIDTH / 2.0f - 100, WINDOW_HEIGHT / 2.0f - 100, 0)); // �J�����̃f�t�H���g�̎��_�ʒu���班�����ꂽ�ꏊ�ɒu����
	sprite3DObjNode->setScale(10.0f);
	Logger::logAssert(isSucceeded, "�m�[�h�̏��������s");
	sprite3DObjNode->setTexture("../MGRRenderer/Resources/boss.png");

	// ��ʂ͂��̃I�[�N�A(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0)�𒆐S�ɍ\������Ă���
	Sprite3D* sprite3DC3tNode = new Sprite3D();
	isSucceeded = sprite3DC3tNode->initWithModel("../MGRRenderer/Resources/orc.c3b");
	sprite3DC3tNode->setPosition(Vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0)); // �J�����̃f�t�H���g�̎��_�ʒu�ɒu����
	sprite3DC3tNode->setRotation(Vec3(0.0f, 180.0f, 0.0f));
	sprite3DC3tNode->setScale(10.0f);
	sprite3DC3tNode->startAnimation("Take 001", true);
	Logger::logAssert(isSucceeded, "�m�[�h�̏��������s");

	Particle3D::Parameter parameter;
	parameter.loopFlag = true;
	parameter.gravity = Vec3(0.0f, -100.0f, 0.0f);
	parameter.initVelocity = 300.0f;
	parameter.lifeTime = 10.0f;
	parameter.numParticle = 100;
	parameter.textureFilePath = "../MGRRenderer/Resources/bluewater.png";
	parameter.pointSize = 5.0f;

	Particle3D* particle3DNode = new Particle3D();
	isSucceeded = particle3DNode->initWithParameter(parameter);
	particle3DNode->setPosition(Vec3(WINDOW_WIDTH / 2.0f - 200.0f, WINDOW_HEIGHT / 2.0f - 300.0f, 0));
	Logger::logAssert(isSucceeded, "�m�[�h�̏��������s");

	Scene* scene = new Scene();
	scene->init();
	AmbientLight* ambientLight = scene->getAmbientLight();
	if (ambientLight != nullptr)
	{
		ambientLight->setIntensity(0.3f);
		ambientLight->setColor(Color3B::WHITE);
	}

	Sprite2D* depthTextureSprite = nullptr;

	DirectionalLight* dirLight = new (std::nothrow) DirectionalLight(Vec3(-1.0f, -1.0f, -1.0f), Color3B::WHITE);
	dirLight->setIntensity(0.7f);
	dirLight->setColor(Color3B(0, 255, 0));
	dirLight->initShadowMap(
		// Polygon3D�̂����Ă̒��S���^�[�Q�b�g�ɂ��A�V�[���S�̂�����悤�ɁA�J������WINDOW_HEIGHT / 1.1566f�ɔ�ׂ�Ɖ��ڂ�
		Vec3(0.0f, 0.0f, -WINDOW_WIDTH / 3.0f) - Vec3(-1.0f, -1.0f, -1.0f) * (WINDOW_HEIGHT / 1.1566f),
		NEAR_CLIP,
		FAR_CLIP,
		SizeUint(WINDOW_WIDTH, WINDOW_HEIGHT));
	scene->setDirectionalLight(dirLight);

	if (dirLight->hasShadowMap())
	{
		depthTextureSprite = new Sprite2D();
		const SizeFloat& contentSize = Director::getInstance()->getWindowSize() / 5.0f;
		depthTextureSprite->initWithDepthStencilTexture(dirLight->getShadowMapData().getDepthTexture(), Sprite2D::RenderBufferType::DEPTH_TEXTURE_ORTHOGONAL, dirLight->getNearClip(), dirLight->getFarClip(), dirLight->getShadowMapData().projectionMatrix);
		depthTextureSprite->setScale(1 / 5.0f);
		depthTextureSprite->setPosition(Vec3(WINDOW_WIDTH - contentSize.width, 0.0f, 0.0f));
	}

	// �I�[�N��(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0)��(0,-1,0)�����̌����~�蒍���悤�ɂ��Ă���
	//PointLight* light = new (std::nothrow) PointLight(Vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0.0f + WINDOW_WIDTH), Color3B::WHITE, 3000.0f);
	//light->setIntensity(0.7f);
	//light->setColor(Color3B(255, 0, 0));
	//light->initShadowMap(NEAR_CLIP, WINDOW_WIDTH);
	//scene->addPointLight(light);

	//if (light->hasShadowMap())
	//{
	//	depthTextureSprite = new Sprite2D();
	//	const SizeFloat& contentSize = Director::getInstance()->getWindowSize() / 5.0f;
	//	depthTextureSprite->setScale(1 / 5.0f);
	//	depthTextureSprite->initWithDepthStencilTexture(light->getShadowMapData().getDepthTexture(), Sprite2D::RenderBufferType::DEPTH_CUBEMAP_TEXTURE, light->getNearClip(), light->getRange(), light->getShadowMapData().projectionMatrix, CubeMapFace::Z_NEGATIVE);
	//	depthTextureSprite->setPosition(Vec3(WINDOW_WIDTH - contentSize.width, 0.0f, 0.0f));
	//}

	// �I�[�N��(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0)��(0,-1,0)�����̌����~�蒍���悤�ɂ��Ă���
	//PointLight* light2 = new (std::nothrow) PointLight(Vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f + WINDOW_WIDTH, 0.0f), Color3B::WHITE, 3000.0f);
	//light2->setIntensity(0.7f);
	//light2->setColor(Color3B(0, 0, 255));
	//light2->initShadowMap(NEAR_CLIP, WINDOW_WIDTH);
	//scene->addPointLight(light2);

	//if (light2->hasShadowMap())
	//{
	//	depthTextureSprite = new Sprite2D();
	//	const SizeFloat& contentSize = Director::getInstance()->getWindowSize() / 5.0f;
	//	depthTextureSprite->setScale(1 / 5.0f);
	//	depthTextureSprite->initWithDepthStencilTexture(light2->getShadowMapData().getDepthTexture(), Sprite2D::RenderBufferType::DEPTH_CUBEMAP_TEXTURE, light2->getNearClip(), light2->getRange(), light2->getShadowMapData().projectionMatrix, CubeMapFace::Y_NEGATIVE);
	//	depthTextureSprite->setPosition(Vec3(WINDOW_WIDTH - contentSize.width, 0.0f, 0.0f));
	//}

	// �I�[�N��(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0)��(0,0,-1)�����̌����~�蒍���悤�ɂ��Ă���
	SpotLight* light = new (std::nothrow) SpotLight(Vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0.0f + WINDOW_WIDTH), Vec3(0.0f, 0.0f, -1.0f), Color3B::WHITE, 3000.0f, 0.0f, 30.0f);
	light->setIntensity(0.7f);
	light->setColor(Color3B(255, 0, 0));
	light->initShadowMap(NEAR_CLIP, SizeUint(WINDOW_WIDTH, WINDOW_HEIGHT));
	scene->addSpotLight(light);

	//if (light->hasShadowMap())
	//{
	//	depthTextureSprite = new Sprite2D();
	//	const SizeFloat& contentsize = Director::getInstance()->getWindowSize() / 5.0f;
	//	depthTextureSprite->initWithDepthStencilTexture(light->getShadowMapData().getDepthTexture(), Sprite2D::RenderBufferType::DEPTH_TEXTURE, light->getNearClip(), light->getRange(), light->getShadowMapData().projectionMatrix);
	//	depthTextureSprite->setScale(1 / 5.0f);
	//	depthTextureSprite->setPosition(Vec3(WINDOW_WIDTH - contentsize.width, 0.0f, 0.0f));
	//}

	//// �I�[�N��(window_width / 2.0f, window_height / 2.0f, 0)��(-1,0,0)�����̌����~�蒍���悤�ɂ��Ă���
	//SpotLight* light2 = new (std::nothrow) SpotLight(Vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f + WINDOW_WIDTH, 0.0f), Vec3(0.0f, -1.0f, 0.0f), Color3B::WHITE, 3000.0f, 0.0f, 30.0f);
	//light2->setIntensity(0.7f);
	//light2->setColor(Color3B(0, 0, 255));
	//light2->initShadowMap(NEAR_CLIP, SizeUint(WINDOW_WIDTH, WINDOW_HEIGHT));
	//scene->addSpotLight(light2);

	//if (light2->hasShadowMap())
	//{
	//	depthTextureSprite = new Sprite2D();
	//	const SizeFloat& contentSize = Director::getInstance()->getWindowSize() / 5.0f;
	//	depthTextureSprite->initWithDepthStencilTexture(light2->getShadowMapData().getDepthTexture(), Sprite2D::RenderBufferType::DEPTH_TEXTURE, light2->getNearClip(), light2->getRange(), light2->getShadowMapData().projectionMatrix);
	//	depthTextureSprite->setScale(1 / 5.0f);
	//	depthTextureSprite->setPosition(Vec3(WINDOW_WIDTH - contentSize.width, 0.0f, 0.0f));
	//}

	scene->pushNode(point3DNode);
	scene->pushNode(line3DNode);
	scene->pushNode(polygon3DNode);
	scene->pushNode(plane3DNode1);
	scene->pushNode(plane3DNode2);
	scene->pushNode(plane3DNode3);
	scene->pushNode(sprite3DObjNode);
	scene->pushNode(sprite3DC3tNode);
	//scene->pushNode(particle3DNode);
	scene->pushNode2D(pointNode);
	scene->pushNode2D(lineNode);
	scene->pushNode2D(polygonNode);
	//scene->pushNode2D(spriteNode);
	scene->pushNode(spriteNode);
	//if (dirLight->hasShadowMap())
	//{
	//	scene->pushNode2D(depthTextureSprite);
	//}
	if (light->hasShadowMap())
	{
		scene->pushNode2D(depthTextureSprite);
	}
	//if (light2->hasShadowMap())
	//{
	//	scene->pushNode2D(depthTextureSprite);
	//}

	Director::getInstance()->setScene(*scene);
}

static float cameraAngle = 0.0f; //TODO:obj��c3t�������₷���ʒu

void render()
{
	const Vec3& cameraPos = Director::getCamera().getPosition();
	cameraAngle += 0.003f;
	Vec3 newCameraPos = Vec3(WINDOW_HEIGHT / 1.1566f * abs(sin(cameraAngle)) + WINDOW_WIDTH / 2.0f, cameraPos.y, WINDOW_HEIGHT / 1.1566f * abs(cos(cameraAngle)));
	//Vec3 newCameraPos = Vec3(WINDOW_HEIGHT / 1.1566f * sin(cameraAngle) + WINDOW_WIDTH / 2.0f, cameraPos.y, WINDOW_HEIGHT / 1.1566f * cos(cameraAngle));
	Director::getCamera().setPosition(newCameraPos);

	// MGRRenderer�ɖ��t���[���̕`�施��
	Director::getInstance()->update();
}

void finalize() {
	// MGRRenderer�I������
	Director::getInstance()->destroy();
}
