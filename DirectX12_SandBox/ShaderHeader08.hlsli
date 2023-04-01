// 頂点シェーダーからピクセルシェーダーへのやり取りに使用する構造体
struct Output
{
	float4 svpos : SV_POSITION;		// システム用頂点座標
	float4 pos : POSITION;			// 頂点座標
	float4 normal : NORMAL0;		// 法線
	float4 vnormal : NORMAL1;		// ビュー変換後の法線ベクトル
	float2 uv : TEXCOORD;			// UV値
	float3 ray:VECTOR;
};

Texture2D<float4> tex : register(t0);	// 0番スロットに設定されたテクスチャ
Texture2D<float4> sph : register(t1);	// 1番スロットに設定されたテクスチャ
Texture2D<float4> spa : register(t2)

SamplerState smp : register(s0);		// 0番スロットに設定されたサンプラー

cbuffer SceneBuffer : register(b0)		// 定数バッファ
{
	matrix world;						// ワールド変換行列
	matrix view;						// ビュープロジェクション行列
	matrix proj;
	float3 eye;
}

cbuffer Material : register(b1)
{
	float4 diffuse;		// ディフューズ色
	float4 specular;	// スぺキュラ色
	float3 ambient;		// アンビエント色
}