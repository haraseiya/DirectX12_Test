// ���_�V�F�[�_�[����s�N�Z���V�F�[�_�[�ւ̂����Ɏg�p����\����
struct Output
{
	float4 svpos : SV_POSITION;		// �V�X�e���p���_���W
	float4 pos : POSITION;			// ���_���W
	float4 normal : NORMAL0;		// �@��
	float4 vnormal : NORMAL1;		// �r���[�ϊ���̖@���x�N�g��
	float2 uv : TEXCOORD;			// UV�l
	float3 ray:VECTOR;
};

Texture2D<float4> tex : register(t0);	// 0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> sph : register(t1);	// 1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> spa : register(t2)

SamplerState smp : register(s0);		// 0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[

cbuffer SceneBuffer : register(b0)		// �萔�o�b�t�@
{
	matrix world;						// ���[���h�ϊ��s��
	matrix view;						// �r���[�v���W�F�N�V�����s��
	matrix proj;
	float3 eye;
}

cbuffer Material : register(b1)
{
	float4 diffuse;		// �f�B�t���[�Y�F
	float4 specular;	// �X�؃L�����F
	float3 ambient;		// �A���r�G���g�F
}