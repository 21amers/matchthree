cbuffer ConstantBuffer
{
	float4x4 finalMatrix;
	float4x4 rotation;
	float4 light;
	float4 color;
	float4 ambientColor;
}

Texture2D Texture;
SamplerState ss;

struct VOut
{
	float4 color : COLOR;
	float2 texcoord : TEXCOORD;
	float4 position : SV_POSITION;
};




VOut VShader(float4 position : POSITION, float4 normal : NORMAL, float2 texcord: TEXCOORD, uint instanceID : SV_InstanceID)
{
	VOut output;

	output.position =  mul(finalMatrix,position);
	output.color = ambientColor;

	float4 norm = normalize(mul(rotation, normal));
	float4 bright = saturate(dot(norm, light));

	output.color += color * bright;
	output.texcoord = texcord;
	return output;
}

float4 PShader(float4 color : COLOR, float2 texcoord: TEXCOORD) : SV_TARGET
{
	return color * Texture.Sample(ss, texcoord);
}