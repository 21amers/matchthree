#pragma once
#include "Common.h"
#include <d3d11.h>
#include <dxgi.h>
#include <string>

class gameObject
{
public:
	gameObject();
	gameObject(FLOAT3 initialPos, FLOAT3 initOrientation, FLOAT3 scale, std::wstring texture);
	gameObject(FLOAT2 initialPos2D, FLOAT2 initOrientation2D, FLOAT2 scale2D, std::wstring texture);
	~gameObject();

	void SetPosition2D(t_float32 x, t_float32 y);
	void SetRotation2D(t_float32 x, t_float32 y);
	void SetScale2D(t_float32 x, t_float32 y);
	
	void SetTileIndex(t_int32 x, t_int32 y) { xTileIndex = x; yTileIndex = y; }
	void SetTargetTileIndex(t_int32 x, t_int32 y) { targetxTileIndex = x; targetyTileindex = y; }
	void SetTextureRef(std::wstring tex) { texture = tex; };
	void Update(t_float32 dt);

	const wchar_t* GetTextureRef() { return texture.c_str(); }
	

	t_int32 xTileIndex;
	t_int32 yTileIndex;

	t_int32 targetIsSet;
	FLOAT2 targetVector;

	t_int32 targetxTileIndex;
	t_int32 targetyTileindex;

	t_int32 selected;
	t_int32 isPlayable;
	t_int32 zposition;

	drawLayer tag;
	std::wstring texture;

	FLOAT2 orientation2D;
	FLOAT2 scale2D;
	FLOAT2 position2D;

private:
	RECT boundingBox;
	
	FLOAT3 position;
	FLOAT3 orientation;
	FLOAT3 scale;

	

};

