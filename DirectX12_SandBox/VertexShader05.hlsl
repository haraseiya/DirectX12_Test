#include "ShaderHeader05.hlsli"

Output VS05(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	Output output;	// ピクセルシェーダーに渡す
	output.svpos = pos;
	output.uv = uv;
	return output;
}

//float4 PS05( float4 pos : POSITION,float2 uv : TEXCOORD ) : SV_POSITION
//{
//	return pos;
//}