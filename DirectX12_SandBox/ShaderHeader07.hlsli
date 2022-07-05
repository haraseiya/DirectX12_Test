// 頂点シェーダーからピクセルシェーダーへのやり取りに使用する構造体
struct Output
{
	float4 svpos : SV_POSITION;		// システム用頂点座標
	float4 normal : NORMAL;			// 法線
	float2 uv : TEXCOORD;			// UV値
};

Texture2D<float4> tex : register(t0);	// 0番スロットに設定されたテクスチャ
SamplerState smp : register(s0);		// 0番スロットに設定されたサンプラー

cbuffer cbuff0 : register(b0)			// 定数バッファ
{
	matrix world;						// ワールド変換行列
	matrix viewproj;					// ビュープロジェクション行列
}