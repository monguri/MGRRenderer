#include "Renderer.h"
#include "RenderCommand.h"
#include "Logger.h"

namespace mgrrenderer
{

void Renderer::initView()
{
	
}

void Renderer::addCommand(RenderCommand* command)
{
	_queue.push_back(command);
}

void Renderer::render()
{
	for (RenderCommand* command : _queue)
	{
		// TODO:for-each�łȂ������ƃR���N�V�����ɑ΂���p�C�v�݂����ȃ��\�b�h�g��������
		executeRenderCommand(command);
	}

	_queue.clear();
}

void Renderer::executeRenderCommand(RenderCommand* command)
{
	switch (command->getType())
	{
	case RenderCommand::Type::CUSTOM:
		command->execute();
		break;
	default:
		Logger::logAssert(false, "�Ή����Ă��Ȃ��R�}���h�^�C�v�����͂��ꂽ�B");
		break;
	}
}

} // namespace mgrrenderer
