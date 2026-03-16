#ifndef CONFIG_H
#define CONFIG_H

#define KB(value) ((value)*1024LL)
#define MB(value) (KB(value)*1024LL)
#define GB(value) (MB(value)*1024LL)
#define TB(value) (GB(value)*1024LL)

#define SD_PI 3.14159265359f
#define SD_TAU (2.0*ANT_PI)

enum class Platforms
{
	Windows,
	MacOS,
	Linux,
	Android,
	Ios
};

struct Config
{
	Platforms Platform;
	const char *Title;
	unsigned int ScreenWidth;
	unsigned int ScreenHeight;
	bool Fullscreen;
};

#endif