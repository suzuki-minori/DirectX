#include "Logger.h"
#include<dxgidebug.h>

void Logger::Log(const std::string& message)
{
	OutputDebugStringA(message.c_str());
}
