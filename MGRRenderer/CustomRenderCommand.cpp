#include "CustomRenderCommand.h"

namespace mgrrenderer
{

void CustomRenderCommand::init(const std::function<void()>& function)
{
	_function = function;
}

void CustomRenderCommand::execute()
{
	_function();
}

} // namespace mgrrenderer
