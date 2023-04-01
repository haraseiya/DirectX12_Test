#include "ShaderHeader08.hlsli"

Output VS08(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONE_NO, min16uint weight : WEIGHT)
{
	Output output;	// �s�N�Z���V�F�[�_�[�ɓn��
	output.svpos = mul(mul(viewproj, world), pos);
	normal.w = 0;									// ���s�ړ������𖳌��ɂ���
	output.normal = mul(world, normal);				// �@���ɂ����[���h�ϊ����s��
	output.vnormal = mul(view, output.normal);
	output.uv = uv;
	output.ray = normalize(pos.xyz - eye);			// �����x�N�g��
	return output;
}