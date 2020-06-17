#pragma once

//======================================================================================================
//
//======================================================================================================
#define NOMINMAX
#define FBXSDK_SHARED

//======================================================================================================
//
//======================================================================================================
#include <windows.h>
#include <stdio.h>
#include <omp.h>
#include <map>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>

//======================================================================================================
//
//======================================================================================================
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")

//======================================================================================================
//
//======================================================================================================
#include <fbxsdk.h>
#pragma comment(lib, "libfbxsdk.lib")

//======================================================================================================
//
//======================================================================================================
typedef char				int8;
typedef unsigned char		uint8;
typedef short				int16;
typedef unsigned short		uint16;
typedef int					int32;
typedef unsigned int		uint32;
typedef long long			int64;
typedef unsigned long long	uint64;
typedef float				fp32;
typedef double				fp64;

//======================================================================================================
//
//======================================================================================================
static const wchar_t*	APPLICATION_TITLE		= L"Rasterizer";
static const int32		SCREEN_WIDTH			= 640;
static const int32		SCREEN_HEIGHT			= 360;

//======================================================================================================
//
//======================================================================================================
static const fp32		SCREEN_WIDTH_F			= fp32(SCREEN_WIDTH);
static const fp32		SCREEN_HEIGHT_F			= fp32(SCREEN_HEIGHT);
static const int32		SCREEN_WIDTH_HALF		= SCREEN_WIDTH / 2;
static const int32		SCREEN_HEIGHT_HALF		= SCREEN_HEIGHT / 2;
static const fp32		SCREEN_WIDTH_HALF_F		= fp32(SCREEN_WIDTH_HALF);
static const fp32		SCREEN_HEIGHT_HALF_F	= fp32(SCREEN_HEIGHT_HALF);

//======================================================================================================
//
//======================================================================================================
#if defined(_DEBUG)
#define ASSERT(c)		if(!(c)) { *reinterpret_cast<int*>(0) = 0; }
#else//defined(_DEBUG)
#define ASSERT(c)
#endif//defined(_DEBUG)
