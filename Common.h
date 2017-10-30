#pragma once
#include <d3d11.h>
#include <string>
#include <memory>

typedef  float  t_float32;
typedef  double t_float64;
typedef  short  t_int16;
typedef  int    t_int32;
typedef  long   t_int64;
typedef  UINT   t_uint32;

struct FLOAT2
{
	t_float32 x, y;
	FLOAT2(t_float32 _x, t_float32 _y) : x(_x), y(_y) {}
	FLOAT2() = default;
};

struct FLOAT3
{
	t_float32 x, y, z;
	FLOAT3(t_float32 _x, t_float32 _y, t_float32 _z) : x(_x), y(_y) , z(_z){}
	FLOAT3() = default;
};

struct FLOAT4
{
	t_float32 x, y, z, w;
	FLOAT4(t_float32 _x, t_float32 _y, t_float32 _z, t_float32 _w) : x(_x), y(_y), z(_z), w(_w) {}

};

struct VERTEX
{
	FLOAT3 position;
	FLOAT3 normal;
	FLOAT2 uv;
};

enum drawLayer
{
	background,
	grid,
	go,
	ui,
};

//convert this to spritesheet
static std::wstring textureResources[] =
{
	L"images/bk1.png", //background
	L"images/bk2.png",
	L"images/bk3.png",
	L"images/bk4.png",

	//--
	L"images/cursor.png", //mouse cursor
	L"images/cir.png", //textures
	L"images/dmnd.png",
	L"images/rect.png",
	L"images/tri.png",
	L"images/hrt.png"
};


//Index buffer
static DWORD spriteIndices[] =
{
	0, 1, 2,    // side 1
	2, 1, 3
};

static VERTEX spriteVertices[] =
{
	{ FLOAT3(-.5f, -.5f, 0.0f), FLOAT3(0.0f, 0.0f, 0.0f), FLOAT2(0.0f, 0.0f) },    // side 1
	{ FLOAT3(.5f, -.5f, 0.0f), FLOAT3(0.0f, 0.0f, 0.0f), FLOAT2(0.0f, 1.0f) },
	{ FLOAT3(-.5f, .5f, 0.0f), FLOAT3(0.0f, 0.0f, 0.0f), FLOAT2(1.0f, 0.0f) },
	{ FLOAT3(.5f, .5f, 0.0f), FLOAT3(0.0f, 0.0f, 0.0f), FLOAT2(1.0f, 1.0f) }
};

static t_int32 squareLevel[64] = { 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
static t_int32 hourglassLevel[64] = { 1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,0,1,1,1,0,1,1,1,1,0,1,0,1,1,1,1,1,1,0 };
static t_int32 randomLevel[64] = { 1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1 };