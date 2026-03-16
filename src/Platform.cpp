#include "Platform.h"
#include "Win32Platform.h"
#include "Config.h"

Platform* Platform::Create(const Config& config)
{
	switch(config.Platform)
	{
	case Platforms::Windows: return new Win32Platform(config);
	case Platforms::MacOS:   return nullptr;
	case Platforms::Linux:   return nullptr;
	case Platforms::Android: return nullptr;
	case Platforms::Ios:     return nullptr;
	}
	return nullptr;
}