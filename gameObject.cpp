#include "stdafx.h"
#include "gameObject.h"

gameObject::gameObject()
{
	//animator = std::make_unique<Animator>();
}

gameObject::gameObject(FLOAT3 initialPos, FLOAT3 initOrientation, FLOAT3 initScale, std::wstring initTexture) : gameObject()
{
	 position = initialPos;
	 orientation = initOrientation;
	 scale = initScale;
	 texture = initTexture;
	
}

 gameObject::gameObject(FLOAT2 initialPos2D, FLOAT2 initOrientation2D, FLOAT2 initScale2D, std::wstring initTexture) : gameObject()
{
	position2D = initialPos2D;
	orientation2D = initOrientation2D;
	scale2D = initScale2D;
	texture = initTexture;
}

gameObject::~gameObject()
{
}

void gameObject::SetPosition2D(t_float32 x, t_float32 y)
{
	position2D = FLOAT2(x, y);
}

void gameObject::SetRotation2D(t_float32 x, t_float32 y) {
	
	orientation2D = FLOAT2(x, y);

	while (orientation2D.x > 360)
		orientation2D.x -= 360;

	while (orientation2D.x < 0)
	{
		orientation2D.x += 180;
	}
}

void gameObject::SetScale2D(t_float32 x, t_float32 y)
{
	scale2D = FLOAT2(x, y);
}

t_float32 currentDelay = 0;
void gameObject::Update(t_float32 dt)
{
	if (targetIsSet)
	{
		if (animationDelay >= currentDelay)
		{
			currentDelay += dt;
			return;
		}
		if (position2D != targetVector)
		{
			position2D = lerp(position2D, targetVector, dt * .9f);
		}
		else
		{
			targetIsSet = 0;
			targetVector = FLOAT2ZERO;
			currentDelay = 0;
		}
	}
}