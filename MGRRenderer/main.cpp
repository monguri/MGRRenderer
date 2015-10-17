#include <iostream>
#include <assert.h>
#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

typedef struct {
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint shaderProgram;
	// 点、線、ポリゴンの範囲ならこのデータだけで大丈夫
	GLint attributePosition;
	GLint attributePointSize;
} RenderingContext;

// 宣言
static void fwErrorHandler(int error, const char* description);
static void fwKeyInputHandler(GLFWwindow* window, int key, int scancode, int action, int mods);
static void initialize(std::vector<RenderingContext>& contexts);
static void render(const std::vector<RenderingContext>& contexts);
static void finalize(const std::vector<RenderingContext>& contexts);

int main()
{
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

	std::vector<RenderingContext> contexts;

	// 描画の初期化
	initialize(contexts);

	while (glfwWindowShouldClose(window) == GL_FALSE)
	{
		// 描画のメインループ
		render(contexts);

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	// 描画の終了処理
	finalize(contexts);

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

// TODO:リサイズイベントを受けるメソッドもつくりたいな
static void initializePoint(RenderingContext& context);
static void initializeLine(RenderingContext& context);
static void renderPoint(const RenderingContext& context);
static void renderLine(const RenderingContext& context);

static GLuint createVertexShader(const GLchar* source);
static GLuint createFragmentShader(const GLchar* source);
static GLuint createShaderProgram(const GLuint vertexShader, const GLuint fragmentShader);
static void destroyShaderProgram(const GLuint program, const GLuint vertexShader, const GLuint fragmentShader);

// 描画物のデータ構造と、OpenGLを使って描画するロジックは切り離して書くべき。OpenGL以外に移植するためにね。
void initialize(std::vector<RenderingContext>& contexts)
{
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

	RenderingContext pointContext;
	initializePoint(pointContext);
	contexts.emplace_back(pointContext);

	RenderingContext lineContext;
	initializeLine(lineContext);
	contexts.emplace_back(lineContext);

	//RenderingContext polygonContext;
	//contexts.push_back(polygonContext);

}

void render(const std::vector<RenderingContext>& contexts)
{
	glClear(GL_COLOR_BUFFER_BIT);

	renderPoint(contexts[0]);

	renderLine(contexts[1]);
}

void finalize(const std::vector<RenderingContext>& contexts) {
	for (const RenderingContext& context : contexts)
	{
		destroyShaderProgram(context.shaderProgram, context.vertexShader, context.fragmentShader);
	}
}

void initializePoint(RenderingContext& context)
{
	context.vertexShader = createVertexShader(
		"attribute mediump vec4 attr_pos;"
		"attribute mediump float attr_point_size;"
		"void main()"
		"{"
		"	gl_Position = attr_pos;"
		"	gl_PointSize = attr_point_size;"
		"}"
		);
	context.fragmentShader = createFragmentShader(
		"void main()"
		"{"
		"	gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);"
		"}"
		);
	context.shaderProgram = createShaderProgram(context.vertexShader, context.fragmentShader);

	context.attributePosition = glGetAttribLocation(context.shaderProgram, "attr_pos");
	assert(glGetError() == GL_NO_ERROR);
	assert(context.attributePosition >= 0);

	context.attributePointSize = glGetAttribLocation(context.shaderProgram, "attr_point_size");
	assert(glGetError() == GL_NO_ERROR);
	assert(context.attributePointSize >= 0);
}

void initializeLine(RenderingContext& context)
{
	context.vertexShader = createVertexShader(
		"attribute mediump vec4 attr_pos;"
		"void main()"
		"{"
		"	gl_Position = attr_pos;"
		"}"
		);
	context.fragmentShader = createFragmentShader(
		"void main()"
		"{"
		"	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);"
		"}"
		);
	context.shaderProgram = createShaderProgram(context.vertexShader, context.fragmentShader);

	context.attributePosition = glGetAttribLocation(context.shaderProgram, "attr_pos");
	assert(glGetError() == GL_NO_ERROR);
	assert(context.attributePosition >= 0);
}

void renderPoint(const RenderingContext& context)
{
	glUseProgram(context.shaderProgram);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray(context.attributePosition);
	assert(glGetError() == GL_NO_ERROR);
	glEnableVertexAttribArray(context.attributePointSize);
	assert(glGetError() == GL_NO_ERROR);

	// 本来、これはノードのデータとしてノードを作った段階であるはず
	// 毎フレーム作るものではない。（staticなので毎回は作られてないけど）
	static const GLfloat positionAndPointSize[] = {
		// 座標x, 座標y, ポイントサイズ
		0.0f, 0.0f, 15.0f,
		0.75f, 0.75f, 15.0f,
	};

	glVertexAttribPointer(context.attributePosition, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (GLvoid*)positionAndPointSize);
	assert(glGetError() == GL_NO_ERROR);
	glVertexAttribPointer(context.attributePointSize, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (GLvoid*)&positionAndPointSize[2]);
	assert(glGetError() == GL_NO_ERROR);

	glDrawArrays(GL_POINTS, 0, 2);
	assert(glGetError() == GL_NO_ERROR);
}

void renderLine(const RenderingContext& context)
{
	glUseProgram(context.shaderProgram);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray(context.attributePosition);
	assert(glGetError() == GL_NO_ERROR);
	glLineWidth(1.0f);
	assert(glGetError() == GL_NO_ERROR);

	// 本来、これはノードのデータとしてノードを作った段階であるはず
	// 毎フレーム作るものではない。（staticなので毎回は作られてないけど）
	static const GLfloat position[] = {
		-1.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		0.0f, -1.0f,
	};

	glVertexAttribPointer(context.attributePosition, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)position);
	assert(glGetError() == GL_NO_ERROR);
	glDrawArrays(GL_LINES, 0, 4);
	assert(glGetError() == GL_NO_ERROR);
}

GLuint createVertexShader(const GLchar* source)
{
	GLuint ret = glCreateShader(GL_VERTEX_SHADER);
	assert(ret != 0);
	assert(glGetError() == GL_NO_ERROR);
	if (ret == 0)
	{
		return ret;
	}

	glShaderSource(ret, 1, &source, nullptr);
	glCompileShader(ret);

	GLint compileResult;
	glGetShaderiv(ret, GL_COMPILE_STATUS, &compileResult);
	if (compileResult == GL_FALSE)
	{
		GLint errorLen;
		glGetShaderiv(ret, GL_INFO_LOG_LENGTH, &errorLen);
		if (errorLen> 1)
		{
			GLchar* errorMsg = (GLchar*)calloc(errorLen, sizeof(GLchar));
			glGetShaderInfoLog(ret, errorLen, nullptr, errorMsg);
			std::cout << errorMsg;
			free(errorMsg);
		}
		else
		{
			std::cout << "compile error but no info.";
		}
	}
	assert(compileResult == GL_TRUE);
	if (compileResult == GL_FALSE)
	{
		return 0;
	}

	return ret;
}

GLuint createFragmentShader(const GLchar* source)
{
	GLuint ret = glCreateShader(GL_FRAGMENT_SHADER);
	assert(ret != 0);
	assert(glGetError() == GL_NO_ERROR);
	if (ret == 0)
	{
		return 0;
	}

	glShaderSource(ret, 1, &source, nullptr);
	glCompileShader(ret);

	GLint compileResult;
	glGetShaderiv(ret, GL_COMPILE_STATUS, &compileResult);
	if (compileResult == GL_FALSE)
	{
		GLint errorLen;
		glGetShaderiv(ret, GL_INFO_LOG_LENGTH, &errorLen);
		if (errorLen> 1)
		{
			GLchar* errorMsg = (GLchar*)calloc(errorLen, sizeof(GLchar));
			glGetShaderInfoLog(ret, errorLen, nullptr, errorMsg);
			std::cout << errorMsg;
			free(errorMsg);
		}
		else
		{
			std::cout << "compile error but no info.";
		}
	}
	assert(compileResult == GL_TRUE);
	if (compileResult == GL_FALSE)
	{
		return 0;
	}

	return ret;
}

GLuint createShaderProgram(const GLuint vertexShader, const GLuint fragmentShader)
{
	GLuint ret = glCreateProgram();
	assert(ret != 0);
	assert(glGetError() == GL_NO_ERROR);
	if (ret == 0)
	{
		return 0;
	}

	glAttachShader(ret, vertexShader);
	assert(glGetError() == GL_NO_ERROR);
	glAttachShader(ret, fragmentShader);
	assert(glGetError() == GL_NO_ERROR);

	glLinkProgram(ret);

	GLint linkResult;
	glGetProgramiv(ret, GL_LINK_STATUS, &linkResult);
	if (linkResult == GL_FALSE)
	{
		GLint errorLen;
		glGetProgramiv(ret, GL_INFO_LOG_LENGTH, &errorLen);
		if (errorLen> 1)
		{
			GLchar* errorMsg = (GLchar*)calloc(errorLen, sizeof(GLchar));
			glGetProgramInfoLog(ret, errorLen, nullptr, errorMsg);
			std::cout << errorMsg;
			free(errorMsg);
		}
		else
		{
			std::cout << "link error but no info.";
		}
	}
	assert(linkResult == GL_TRUE);
	if (linkResult == GL_FALSE)
	{
		return 0;
	}
	return ret;
}

void destroyShaderProgram(const GLuint program, const GLuint vertexShader, const GLuint fragmentShader)
{
	glUseProgram(0);
	assert(glGetError() == GL_NO_ERROR);

	glDeleteProgram(program);
	assert(glGetError() == GL_NO_ERROR);

	assert(glIsProgram(program) == GL_FALSE);

	glDeleteShader(vertexShader);
	assert(glGetError() == GL_NO_ERROR);

	glDeleteShader(fragmentShader);
	assert(glGetError() == GL_NO_ERROR);
}
