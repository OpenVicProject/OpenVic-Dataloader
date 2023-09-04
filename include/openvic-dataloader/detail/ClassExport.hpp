#pragma once

#ifdef _MSC_VER
#define OVDL_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#define OVDL_EXPORT __attribute__((visibility("default")))
#else
#define OVDL_EXPORT
#endif