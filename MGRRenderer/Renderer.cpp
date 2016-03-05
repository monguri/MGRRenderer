#include "Renderer.h"
#include "RenderCommand.h"
#include "GroupBeginRenderCommand.h"
#include "Logger.h"

namespace mgrrenderer
{

static const size_t DEFAULT_RENDER_QUEUE_GROUP_INDEX = 0;

Renderer::Renderer()
{
	_groupIndexStack.push(DEFAULT_RENDER_QUEUE_GROUP_INDEX);

	std::vector<RenderCommand*> defaultRenderQueue;
	_queueGroup.push_back(defaultRenderQueue);
}

void Renderer::initView(const Size& windowSize)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	// TODO:ブレンドが必要ない時もブレンドをONにしている
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// OpenGL側でやるビューポート変換のためのパラメータを渡す
	glViewport(0, 0, windowSize.width, windowSize.height);
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
