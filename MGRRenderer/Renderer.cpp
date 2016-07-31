#include "Renderer.h"
#include "Director.h"
#include "RenderCommand.h"
#include "GroupBeginRenderCommand.h"
#include "Logger.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "D3DTexture.h"
#endif

namespace mgrrenderer
{

static const size_t DEFAULT_RENDER_QUEUE_GROUP_INDEX = 0;

Renderer::Renderer()
#if defined(MGRRENDERER_USE_DIRECT3D)
: _gBufferDepthStencil(nullptr),
_gBufferColorSpecularIntensity(nullptr),
_gBufferNormal(nullptr),
_gBufferSpecularPower(nullptr)
#endif
{
	_groupIndexStack.push(DEFAULT_RENDER_QUEUE_GROUP_INDEX);

	std::vector<RenderCommand*> defaultRenderQueue;
	_queueGroup.push_back(defaultRenderQueue);
}

Renderer::~Renderer()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	if (_gBufferSpecularPower != nullptr)
	{
		_gBufferSpecularPower = nullptr;
	}

	if (_gBufferNormal != nullptr)
	{
		_gBufferNormal = nullptr;
	}

	if (_gBufferColorSpecularIntensity != nullptr)
	{
		_gBufferColorSpecularIntensity = nullptr;
	}

	if (_gBufferDepthStencil != nullptr)
	{
		_gBufferDepthStencil = nullptr;
	}
#endif
}

void Renderer::initView(const Size& windowSize)
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
	direct3dContext->RSSetViewports(1, Director::getInstance()->getDirect3dViewport());

	const Size& size = Director::getInstance()->getWindowSize();

	_gBufferDepthStencil = new D3DTexture();
	_gBufferDepthStencil->initDepthStencilTexture(size);

	_gBufferColorSpecularIntensity = new D3DTexture();
	_gBufferColorSpecularIntensity->initRenderTexture(size, DXGI_FORMAT_R8G8B8A8_UNORM);

	_gBufferNormal = new D3DTexture();
	_gBufferNormal->initRenderTexture(size, DXGI_FORMAT_R11G11B10_FLOAT);

	_gBufferSpecularPower = new D3DTexture();
	_gBufferSpecularPower->initRenderTexture(size, DXGI_FORMAT_R8G8B8A8_UNORM);

#elif defined(MGRRENDERER_USE_OPENGL)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	// TODO:ブレンドが必要ない時もブレンドをONにしている
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// OpenGL側でやるビューポート変換のためのパラメータを渡す
	glViewport(0, 0, windowSize.width, windowSize.height);
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // デフォルトのフレームバッファ
#endif
}

void Renderer::addCommand(RenderCommand* command)
{
	switch (command->getType())
	{
	case RenderCommand::Type::GROUP_BEGIN:
		{
			// スタックのトップのインデックスのキューに追加
			_queueGroup[_groupIndexStack.top()].push_back(command);

			// スタックにプッシュ
			size_t groupIndex = _queueGroup.size();
			GroupBeginRenderCommand* groupCommand = static_cast<GroupBeginRenderCommand*>(command);
			groupCommand->setGroupIndex(groupIndex);
			_groupIndexStack.push(groupIndex);

			std::vector<RenderCommand*> newRenderQueue;
			_queueGroup.push_back(newRenderQueue);
		}
		break;
	case RenderCommand::Type::GROUP_END:
		// addCommandでは、現在操作中のグループIDを知るためにスタックからは削除するが、キューグループからは削除しない。
		_groupIndexStack.pop();
		_queueGroup[_groupIndexStack.top()].push_back(command);
		break;
	case RenderCommand::Type::CUSTOM:
		// スタックのトップのキューに追加
		_queueGroup[_groupIndexStack.top()].push_back(command);
		break;
	default:
		Logger::logAssert(false, "対応していないコマンドタイプが入力された。");
		break;
	}
}

void Renderer::render()
{
	visitRenderQueue(_queueGroup[DEFAULT_RENDER_QUEUE_GROUP_INDEX]);

	Logger::logAssert(_groupIndexStack.size() == 1, "グループコマンド開始側で作られたインデックススタックは終了側で消されてるはず。_groupIndexStack.size() == %d", _queueGroup.size());

	// 0番目のキューを残してあとは削除。次フレームのグループコマンドでまた追加する。削除するのは、シーンの状況でグループ数は変わりうるので、増えたままで残しておいても無駄だから。
	while (_queueGroup.size() > 1)
	{
		_queueGroup.pop_back();
	}

	_queueGroup[DEFAULT_RENDER_QUEUE_GROUP_INDEX].clear();
}

void Renderer::prepareDifferedRendering()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
	direct3dContext->ClearState();

	float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	direct3dContext->ClearRenderTargetView(_gBufferColorSpecularIntensity->getRenderTargetView(), clearColor);
	direct3dContext->ClearRenderTargetView(_gBufferNormal->getRenderTargetView(), clearColor);
	direct3dContext->ClearRenderTargetView(_gBufferSpecularPower->getRenderTargetView(), clearColor);
	direct3dContext->ClearDepthStencilView(_gBufferDepthStencil->getDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	direct3dContext->RSSetViewports(1, Director::getInstance()->getDirect3dViewport());

	ID3D11RenderTargetView* gBuffers[3] = {_gBufferColorSpecularIntensity->getRenderTargetView(), _gBufferNormal->getRenderTargetView(), _gBufferSpecularPower->getRenderTargetView()};
	direct3dContext->OMSetRenderTargets(3, gBuffers, _gBufferDepthStencil->getDepthStencilView());
	direct3dContext->OMSetDepthStencilState(_gBufferDepthStencil->getDepthStencilState(), 1);
#endif
}

void Renderer::prepareFowardRendering()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
	direct3dContext->ClearState();

	float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	direct3dContext->ClearRenderTargetView(Director::getInstance()->getDirect3dRenderTarget(), clearColor);
	direct3dContext->ClearDepthStencilView(Director::getInstance()->getDirect3dDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	direct3dContext->RSSetViewports(1, Director::getInstance()->getDirect3dViewport());
	ID3D11RenderTargetView* renderTarget = Director::getInstance()->getDirect3dRenderTarget(); //TODO: 一度変数に入れないとコンパイルエラーが出てしまった
	direct3dContext->OMSetRenderTargets(1, &renderTarget, Director::getInstance()->getDirect3dDepthStencilView());
#elif defined(MGRRENDERER_USE_OPENGL)
	glDisable(GL_CULL_FACE);
	glViewport(0, 0, Director::getInstance()->getWindowSize().width, Director::getInstance()->getWindowSize().height);
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // デフォルトフレームバッファに戻す
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

void Renderer::visitRenderQueue(const std::vector<RenderCommand*> queue)
{
	for (RenderCommand* command : queue)
	{
		// TODO:for-eachでなくもっとコレクションに対するパイプみたいなメソッド使いたいな
		executeRenderCommand(command);
	}
}

void Renderer::executeRenderCommand(RenderCommand* command)
{
	switch (command->getType())
	{
	case RenderCommand::Type::GROUP_BEGIN:
		{
			GroupBeginRenderCommand* groupCommand = static_cast<GroupBeginRenderCommand*>(command);
			groupCommand->execute();
			visitRenderQueue(_queueGroup[groupCommand->getGroupIndex()]);
		}
		break;
	case RenderCommand::Type::GROUP_END:
		{
			GroupBeginRenderCommand* groupCommand = static_cast<GroupBeginRenderCommand*>(command);
			groupCommand->execute();
		}
		break;
	case RenderCommand::Type::CUSTOM:
		command->execute();
		break;
	default:
		Logger::logAssert(false, "対応していないコマンドタイプが入力された。");
		break;
	}
}

} // namespace mgrrenderer
