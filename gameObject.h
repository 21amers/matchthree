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
	void SetTextureRef(std::wstring tex);
	
	RECT GetBounds() { return boundingBox; }
	FLOAT2 GetRotation2D() { return orientation2D; }
	FLOAT2 GetPosition2D() { return position2D; }
	FLOAT2 GetScale2D() { return scale2D; }
	const wchar_t* GetTextureRef() { return texture.c_str(); }
	void SetDrawLayer(drawLayer layer) { tag = layer; }
	void SetPlayable(t_int32 playable) { isPlayable = playable; }
	t_int32 IsPlayable() { return isPlayable; }
	void Update(t_float32 dt);

private:
	RECT boundingBox;
	FLOAT2 position2D;
	FLOAT2 orientation2D;
	FLOAT2 scale2D;

	FLOAT3 position;
	FLOAT3 orientation;
	FLOAT3 scale;
	
	t_int32 isPlayable;
	t_int32 zposition;
	drawLayer tag;
	std::wstring texture;
};

