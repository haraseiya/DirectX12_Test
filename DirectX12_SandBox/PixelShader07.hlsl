#include "ShaderHeader07.hlsli"

float4 PS07(Output input) :SV_TARGET
{
	float3 light = normalize(float3(1,-1,1));
	float brightness = dot(-light, input.normal);
	return float4(brightness, brightness, brightness, 1);
}
