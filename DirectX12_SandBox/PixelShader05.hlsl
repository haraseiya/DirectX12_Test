#include "ShaderHeader05.hlsli"

float4 PS05(Output input) :SV_TARGET
{
	return float4(tex.Sample(smp,input.uv));
}
