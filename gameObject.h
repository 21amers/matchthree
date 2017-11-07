#pragma once
#include "Common.h"
#include <d3d11.h>
#include <dxgi.h>
#include <string>
#include "Animator.h"

class gameObject
{
public:
	gameObject();
	gameObject(FLOAT3 initialPos, FLOAT3 initOrientation, FLOAT3 scale, tile tl);
	gameObject(FLOAT2 initialPos2D, FLOAT2 initOrientation2D, FLOAT2 scale2D, tile tl);
	~gameObject();

	void SetPosition2D(t_float32 x, t_float32 y);
	void SetRotation2D(t_float32 x, t_float32 y);
	void SetScale2D(t_float32 x, t_float32 y);
	
	void SetTileRef(tile tl) { objectTile = tl; };
	void Update(t_float32 dt);

	const wchar_t* GetTextureRef() { return objectTile.Id.c_str(); }
	
	//Svoid SetAnimation();
	t_float32 animationDelay;
	t_int32 targetIsSet;
	FLOAT2 targetVector;

	t_int32 selected;
	t_int32 isPlayable;
	t_int32 zposition;

	drawLayer tag;
	tile objectTile;

	FLOAT2 orientation2D;
	FLOAT2 scale2D;
	FLOAT2 position2D;

private:
	RECT boundingBox;
	
	FLOAT3 position;
	FLOAT3 orientation;
	FLOAT3 scale;

//	std::unique_ptr<Animator> animator;

};

