#include "ShaderHeader08.hlsli"

float4 PS08(Output input) :SV_TARGET
{
	float3 light = normalize(float3(1,-1,1));	// ���̌������x�N�g��
	float3 lightColor = float3(1, 1, 1);		// ���C�g�̃J���[

	float diffuseB = dor(-light, input.normal);						// �f�B�t���[�Y�v�Z
	float3 refLight = normalize(reflect(light, input.normal.xyz));	// ���̔��˃x�N�g��
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);

	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	float4 texColor = tex.Sample(smp, input.uv);	// �e�N�X�`���J���[

	return max(diffuseB * diffuse * texColor * sph.Sample(smp, sphereMapUV) + spa.Sample(smp, sphereMapUV) * texColor + float4(specularB * specular.rgb, 1), float4(texColor * ambient, 1));
}
