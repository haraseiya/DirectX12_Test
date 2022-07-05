#include "ShaderHeader07.hlsli"

Output VS07(float4 pos : POSITION, float4 normal : NORMAL , float2 uv : TEXCOORD , min16uint2 boneno : BONE_NO , min16uint weight : WEIGHT)
{
	Output output;	// ピクセルシェーダーに渡す
	output.svpos = mul(mul(viewproj, world), pos);
	normal.w = 0;									// 平行移動成分を無効にする
	output.normal = mul(world, normal);				// 法線にもワールド変換を行う
	output.uv = uv;
	return output;
}

//float4 PS05( float4 pos : POSITION,float2 uv : TEXCOORD ) : SV_POSITION
//{
//	return pos;
//}