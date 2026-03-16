#include "Platform.h"
#include "Win32Platform.h"
#include "Config.h"

Platform* Platform::Create(const Config& config)
{
#if SD_WIN32
	return new Win32Platform(config);
#endif
	return nullptr;
}