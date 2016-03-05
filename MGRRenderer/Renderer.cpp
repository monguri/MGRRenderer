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
	// TODO:�u�����h���K�v�Ȃ������u�����h��ON�ɂ��Ă���
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// OpenGL���ł��r���[�|�[�g�ϊ��̂��߂̃p�����[�^��n��
	glViewport(0, 0, windowSize.width, windowSize.height);
}

void Renderer::addCommand(RenderCommand* command)
{
	switch (command->getType())
	{
	case RenderCommand::Type::GROUP_BEGIN:
		{
			// �X�^�b�N�̃g�b�v�̃C���f�b�N�X�̃L���[�ɒǉ�
			_queueGroup[_groupIndexStack.top()].push_back(command);

			// �X�^�b�N�Ƀv�b�V��
			size_t groupIndex = _queueGroup.size();
			GroupBeginRenderCommand* groupCommand = static_cast<GroupBeginRenderCommand*>(command);
			groupCommand->setGroupIndex(groupIndex);
			_groupIndexStack.push(groupIndex);

			std::vector<RenderCommand*> newRenderQueue;
			_queueGroup.push_back(newRenderQueue);
		}
		break;
	case RenderCommand::Type::GROUP_END:
		// addCommand�ł́A���ݑ��쒆�̃O���[�vID��m�邽�߂ɃX�^�b�N����͍폜���邪�A�L���[�O���[�v����͍폜���Ȃ��B
		_groupIndexStack.pop();
		_queueGroup[_groupIndexStack.top()].push_back(command);
		break;
	case RenderCommand::Type::CUSTOM:
		// �X�^�b�N�̃g�b�v�̃L���[�ɒǉ�
		_queueGroup[_groupIndexStack.top()].push_back(command);
		break;
	default:
		Logger::logAssert(false, "�Ή����Ă��Ȃ��R�}���h�^�C�v�����͂��ꂽ�B");
		break;
	}
}

void Renderer::render()
{
	visitRenderQueue(_queueGroup[DEFAULT_RENDER_QUEUE_GROUP_INDEX]);

	Logger::logAssert(_groupIndexStack.size() == 1, "�O���[�v�R�}���h�J�n���ō��ꂽ�C���f�b�N�X�X�^�b�N�͏I�����ŏ�����Ă�͂��B_groupIndexStack.size() == %d", _queueGroup.size());

	// 0�Ԗڂ̃L���[���c���Ă��Ƃ͍폜�B���t���[���̃O���[�v�R�}���h�ł܂��ǉ�����B�폜����̂́A�V�[���̏󋵂ŃO���[�v���͕ς�肤��̂ŁA�������܂܂Ŏc���Ă����Ă����ʂ�����B
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
		// TODO:for-each�łȂ������ƃR���N�V�����ɑ΂���p�C�v�݂����ȃ��\�b�h�g��������
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
		Logger::logAssert(false, "�Ή����Ă��Ȃ��R�}���h�^�C�v�����͂��ꂽ�B");
		break;
	}
}

} // namespace mgrrenderer
