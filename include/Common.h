#ifndef COMMON_H
#define COMMON_H

#ifdef SD_EXPORT
#define SD_API __declspec(dllexport)
#else
#define SD_API __declspec(dllimport)
#endif

#endif