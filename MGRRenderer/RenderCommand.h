#pragma once

namespace mgrrenderer
{

class RenderCommand
{
public:
	enum class Type : int
	{
		NONE = -1,
		CUSTOM,
		NUM_TYPES
	};

	virtual Type getType() = 0;
	virtual void execute() = 0;

protected:
	virtual ~RenderCommand() {}
};

} // namespace mgrrenderer
