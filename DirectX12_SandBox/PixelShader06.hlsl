#include "ShaderHeader06.hlsli"

float4 PS06(Output input) :SV_TARGET
{
	return float4(tex.Sample(smp,input.uv));
}
