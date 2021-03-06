#pragma once
#include <d3d11.h>
#include <string>
#include <memory>
#include <math.h>
#include <DirectXMath.h>

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

struct INT2
{
	t_int32 x, y;
	INT2(t_int32 _x, t_int32 _y) : x(_x), y(_y) {}
	INT2() = default;
};

struct VERTEX
{
	FLOAT3 position;
	FLOAT3 normal;
	FLOAT2 uv;
};

enum tileType
{
	diamond,
	square,
	triangle,
	circle,
	dummy

};

enum drawLayer
{
	background,
	grid,
	go,
	ui,
};

// Vector functions
static FLOAT2 FLOAT2ZERO(0.0f, 0.0f);

inline FLOAT2 operator+(FLOAT2 a, FLOAT2 b)
{
	return FLOAT2(a.x + b.x, a.y + b.y);
}

inline FLOAT2 operator-(FLOAT2 a, FLOAT2 b)
{
	return FLOAT2(a.x - b.x, a.y - b.y);
}

inline bool operator==(FLOAT2 a, FLOAT2 b)
{
	return a.x == b.x && a.y == b.y;
}

inline bool operator!=(FLOAT2 a, FLOAT2 b)
{
	return !(a == b);
}

inline FLOAT2 operator*(t_float32 a, FLOAT2 b)
{
	return FLOAT2(b.x * a, b.y * a);
}

inline FLOAT2 operator*(FLOAT2 a, t_float32 b)
{
	return FLOAT2(a.x * b, a.y * b);
}

inline t_float32 vectorMagnitude(FLOAT2 vec)
{
	return (t_float32) std::sqrt(vec.x * vec.x + vec.y*vec.y);
}

inline t_float32 vectorMagnitudeSquared(FLOAT2 vec)
{
	return vec.x * vec.x + vec.y*vec.y;
}

inline FLOAT2 normalized(FLOAT2 vector)
{
	float magnitude = vectorMagnitude(vector);
	return FLOAT2(vector.x / magnitude, vector.y / magnitude);
}

inline FLOAT2 lerp(FLOAT2 start, FLOAT2 end, t_float32 prc)
{
	return (start + (prc *(end-start)));
}

inline t_float32 strongEaseOut(t_float32 t, t_float32 d)
{
	return 1.0f - pow(1 - (t / 2.0f), 5);
}

struct tile
{
	tile(std::wstring id, const std::wstring &texture, tileType tt, t_int32 isplayable) : Id(id) , textureResource(texture), tileType(tt), isPlayable(isplayable) {}
	tile() = default;

	std::wstring Id;
	std::wstring textureResource;
	tileType tileType;
	t_int32 isPlayable;
};

//convert this to spritesheet
static tile textureResources[] =
{
	tile(L"0",L"images/bk1.png",tileType::dummy, 0), //background
	tile(L"1",L"images/bk2.png",tileType::dummy, 0),
	tile(L"2",L"images/bk3.png",tileType::dummy, 0),

	tile(L"3",L"images/bk4.png",tileType::dummy, 0),
	tile(L"4",L"images/cursor.png",tileType::dummy, 0),

	tile(L"5",L"images/cir.png",tileType::dummy, 1),
	tile(L"6",L"images/dmnd.png",tileType::dummy, 1),
	tile(L"7",L"images/rect.png",tileType::dummy, 1),
	tile(L"8",L"images/tri.png",tileType::dummy, 1),
	tile(L"9",L"images/hrt.png",tileType::dummy, 1),
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


template<typename T> T clamp(T min, T max, T value)
{
	if (value < min)
		return min;

	if (value >= max)
		return max;

	return value;
}

static t_int32 squareLevel[64] = { 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
static t_int32 hourglassLevel[64] = { 1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,0,1,1,1,0,1,1,1,1,0,1,0,1,1,1,1,1,1,0 };
static t_int32 randomLevel[64] = { 1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1 };


//static enum animationType
//{
//	spriteSheet,
//	transformation,
//	rotation,
//	scale,
//	custom
//};
//
//typedef void (*animation)(FLOAT2 target);
////typedef void(*animation)(t_float32 rotationTarget);
//
//struct Animation
//{
//	animation *animRunner;
//};
