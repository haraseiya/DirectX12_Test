#include "ShaderHeader05.hlsli"

Output VS05(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	Output output;	// �s�N�Z���V�F�[�_�[�ɓn��
	output.svpos = pos;
	output.uv = uv;
	return output;
}

//float4 PS05( float4 pos : POSITION,float2 uv : TEXCOORD ) : SV_POSITION
//{
//	return pos;
//}