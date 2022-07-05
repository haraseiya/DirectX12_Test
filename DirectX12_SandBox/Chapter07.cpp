#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <d3dx12.h>

#ifdef _DEBUG
#include <iostream>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

using namespace std;
using namespace DirectX;

// コンソール画面にフォーマット付き文字列を表示
// この関数はデバッグ用
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif
}

// 面倒だけど書かなければいけない関数
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ウインドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY)
	{
		// OSに対して「もうこのアプリは終わる」と伝える
		PostQuitMessage(0);
		return 0;
	}
	// 既定の処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
IDXGISwapChain4* _swapchain = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;

void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));

	// デバッグレイヤーを有効化する
	debugLayer->EnableDebugLayer();

	// 有効化したらインターフェイスを解放する
	debugLayer->Release();
}

#ifdef _DEBUG
int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINNSTANCE, LPSTR, int)
{
#endif
	DebugOutputFormatString("Show window test.");

	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;	// コールバック関数の指定
	w.lpszClassName = _T("DirectX12_SandBox");	// アプリケーションクラス名
	w.hInstance = GetModuleHandle(nullptr);		// ハンドルの取得
	RegisterClassEx(&w);						// アプリケーションクラス（ウィンドウクラスの指定をOSに伝える）

	RECT wrc{ 0,0,window_width,window_height };	// ウィンドウサイズを決める
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(
		w.lpszClassName			//クラス名指定
		, _T("DirectX12_Test")	// タイトルバーの文字
		, WS_OVERLAPPEDWINDOW	// タイトルバーと境界線があるウィンドウ
		, CW_USEDEFAULT			// 表示x座標はOSにお任せ
		, CW_USEDEFAULT			// 表示y座標はOSにお任せ
		, wrc.right - wrc.left	// ウィンドウ幅
		, wrc.bottom - wrc.top	// ウィンドウ高さ
		, nullptr				// 親ウィンドウハンドル
		, nullptr				// メニューハンドル
		, w.hInstance			// 呼び出しアプリケーションハンドル
		, nullptr);				// 追加パラメーター

#ifdef _DEBUG
	// デバッグレイヤーをオン
	EnableDebugLayer();
#endif

	// DirectX12まわりの初期化
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	HRESULT result = S_OK;
	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory)))) {
		if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&_dxgiFactory)))) {
			return -1;
		}
	}

	// アダプターの列挙用
	std::vector<IDXGIAdapter*> adapters;

	// ここに特定の名前のアダプターオブジェクトが入る
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};

		// アダプターの説明オブジェクト取得
		adpt->GetDesc(&adesc);

		std::wstring strDesc = adesc.Description;

		// 探したいアダプターの名前を確認
		// ここではNVIDIAを探す
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels)
	{
		if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&_dev)) == S_OK)
		{
			featureLevel = lv;
			break; // 生成可能なバージョンが見つかったらループを打ち切り
		}
	}

	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));

	// コマンドキュー作成
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.NodeMask = 0;										// アダプターを1つしか使わないときは０でよい
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// プライオリティは特になし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;				// コマンドリストと合わせる
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));	// キュー作成

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;					// バックバッファーは伸び縮み可能
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// フリップ後は速やかに破棄
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;			// 特に指定なし
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// ウィンドウ⇔フルスクリーン切り替え可能

	result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue, hwnd, &swapchainDesc, nullptr, nullptr, (IDXGISwapChain1**)&_swapchain);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		// どんなビューを作るか（レンダーターゲットビューを指定）
	heapDesc.NodeMask = 0;								// 複数のGPUがある場合に識別を行うためのビットフラグ（１つだけの想定なので０）
	heapDesc.NumDescriptors = 2;						// ディスクリプタの数を表す（表画面と裏画面それぞれのバッファーに対応するビューなので２を指定）
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// ビューにあたる情報をシェーダー側から参照する必要があるかどうかを指定する（テクスチャバッファーや定数バッファーであれば必要）
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	// ガンマ補正あり
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int idx = 0; idx < swcDesc.BufferCount; ++idx)
	{
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		_dev->CreateRenderTargetView(_backBuffers[idx], &rtvDesc, handle);
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// 深度バッファの作成
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 2次元のテクスチャデータ
	depthResDesc.Width = window_width;								// 幅はレンダーターゲットと同じ
	depthResDesc.Height = window_height;							// 高さもレンダーターゲットと同じ
	depthResDesc.DepthOrArraySize = 1;								// テクスチャ配列でも３Dテクスチャでもない
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;					// 深度値書き込み用フォーマット
	depthResDesc.SampleDesc.Count = 1;								// サンプルはピクセルあたり１つ
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// デプスステンシルとして使用

	// 深度値用のヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;					// デフォルトなのであとはUNKNOWでよい
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// クリアバリュー（重要）
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;		// 深さ1.0f（最大値）でクリア
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;	// 32ビットfloat値としてクリア

	// デプスバッファ作成
	ID3D12Resource* depthBuffer = nullptr;
	result = _dev->CreateCommittedResource(&depthHeapProp, D3D12_HEAP_FLAG_NONE, &depthResDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&depthBuffer));

	// 深度のためのディスクリプタヒープを作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;						// 深度ビューは１つ
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;	// デプスステンシルビューとして使う
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

	// 深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;					// 深度値に32ビット使用
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// 2Dテクスチャ
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;					// フラグは特に無し
	_dev->CreateDepthStencilView(depthBuffer, &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

	// フェンスを作成
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	ShowWindow(hwnd, SW_SHOW);

	// PMDヘッダーの構造体
	struct PMDHeader
	{
		float version;			// バージョン
		char model_name[20];	// モデル名
		char comment[256];		// モデルコメント
	};

	// ファイル読み込み
	char signature[3] = {};
	PMDHeader pmdHeader = {};
	auto fp = fopen("Asset/初音ミク/初音ミク@七葉1052式.pmd", "rb");
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	constexpr size_t pmdVertex_size = 38;	// 頂点1つあたりのサイズ（座標１２、法線１２、uv８、ボーン４、ボーン影響度１、輪郭線１）
	unsigned int vertNum;					// 頂点数
	fread(&vertNum, sizeof(vertNum), 1, fp);

	std::vector<unsigned char> vertices(vertNum* pmdVertex_size);	// バッファの確保
	fread(vertices.data(), vertices.size(), 1, fp);					// 読み込み

	// 頂点データ構造体
	struct Vertex
	{
		XMFLOAT3 pos;	// xyz座標
		XMFLOAT2 uv;	// uv座標
	};

	//Vertex vertices[] =
	//{
	//	{{-1.0f,-1.0f,0.0f},{0.0f,1.0f}},	// 左下
	//	{{-1.0f,1.0f,0.0f},{0.0f,0.0f}},		// 左上
	//	{{1.0f,-1.0f,0.0f},{1.0f,1.0f}},	// 右下
	//	{{1.0f,1.0f,0.0f} ,{1.0f,0.0f}},	// 右上
	//};


	ID3D12Resource* vertBuff = nullptr;
	CD3DX12_HEAP_PROPERTIES vertHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC vertResDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size());
	result = _dev->CreateCommittedResource(&vertHeapProp, D3D12_HEAP_FLAG_NONE, &vertResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertBuff));

	//// d3dx12.hを使用した簡易的な書き方
	//ID3D12Resource* vertBuff = nullptr;
	//result = _dev->CreateCommittedResource(
	//	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)
	//	, D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices))
	//	, D3D12_RESOURCE_STATE_GENERIC_READ
	//	, nullptr
	//	, IID_PPV_ARGS(&vertBuff));

	unsigned char* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);

	// 頂点バッファビューの作成
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();	// GPU上のバッファーの仮想アドレスを取得する（GPU側はどのバッファーからデータをどのくらい見ればよいかわかる）
	vbView.SizeInBytes = vertices.size();						// 全バイト数
	vbView.StrideInBytes = pmdVertex_size;						// 1頂点当たりのバイト数

	// インデックス
	std::vector<unsigned short> indices;	// インデックス配列
	unsigned int indicesNum;				// インデックス数

	// インデックス数読み込み
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	fclose(fp);

	// インデックスバッファー生成
	ID3D12Resource* idxBuff = nullptr;
	CD3DX12_HEAP_PROPERTIES idxHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC idxResDesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));

	vertResDesc.Width = sizeof(indices);
	result = _dev->CreateCommittedResource(&idxHeapProp, D3D12_HEAP_FLAG_NONE, &idxResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&idxBuff));

	// 作ったバッファにインデックスバッファを作成
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	// インデックスバッファービューを作成
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);

	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	// 頂点シェーダーのコンパイル
	result = D3DCompileFromFile(L"VertexShader07.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS07", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_vsBlob, &errorBlob);
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見当たりません");
			return 0;
		}
		else
		{
			std::string errstr;							// 受け取り用string
			errstr.resize(errorBlob->GetBufferSize());	// 必要なサイズを確保

			// データをコピー
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			::OutputDebugStringA(errstr.c_str());
		}
		exit(1);
	}

	// ピクセルシェーダーのコンパイル
	result = D3DCompileFromFile(L"PixelShader07.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PS07", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_psBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);
	}

	// 頂点レイアウトを設定
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"BONE_NO",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"WEIGHT",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"EDGE_FLG",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	// グラフィックスパイプラインステートの設定（頂点シェーダーとピクセルシェーダー）
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr;
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

	// サンプルマスクとラスタライザステートの設定
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;			// デフォルトのサンプルマスクを表す定数
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;
	gpipeline.RasterizerState.MultisampleEnable = false;		// アンチエイリアスは使わないためfalse
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// 背面カリングは行わない
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;	// ポリゴンの中身を塗りつぶす
	gpipeline.RasterizerState.DepthClipEnable = true;			// 深度方向のクリッピングは有効に

	// ブレンドステートの設定
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	// 入力レイアウトの設定
	gpipeline.InputLayout.pInputElementDescs = inputLayout;						// レイアウトの先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout);					// レイアウト配列の要素数(_countofは配列の要素数を取得するマクロ）
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	// 頂点同士を特に切り離さない
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// 三角形で構成

	// レンダーターゲットの設定
	gpipeline.NumRenderTargets = 1;	// 今は１つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	// ０～１に正規化されたRGBA

	// 深度ステンシルの設定
	gpipeline.DepthStencilState.DepthEnable = true;								// 深度バッファを使う
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	// ピクセル描画時に深度バッファに深度値を書き込む
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;			// 深度値が小さいほうを採用
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;								// 32ビットfloatを深度値として使用する

	// アンチエイリアシングのためのサンプル数設定
	gpipeline.SampleDesc.Count = 1;		// サンプリングは1ピクセルにつき１
	gpipeline.SampleDesc.Quality = 0;	// クオリティは最低

	// 空のルートシグネチャを作成する
	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;	// 頂点情報（入力アセンブラ）がある

	// ディスクリプタテーブルの設定（複数のテクスチャバッファービューをまとめて指定できる。今回はテクスチャと定数の２つ）
	D3D12_DESCRIPTOR_RANGE descTblRange[2] = {};				// テクスチャと定数の２つ
	descTblRange[0].NumDescriptors = 1;							// テクスチャ１つ
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// 種別はテクスチャ
	descTblRange[0].BaseShaderRegister = 0;						// 0番スロットから
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	descTblRange[1].NumDescriptors = 1;							// 定数１つ
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;// 種別はテクスチャ
	descTblRange[1].BaseShaderRegister = 0;						// 0番スロットから
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータの作成
	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.DescriptorTable.pDescriptorRanges = descTblRange;
	rootParam.DescriptorTable.NumDescriptorRanges = 2;						// ディスクリプタレンジ数
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;				// ピクセルシェーダーから見える

	rootSignatureDesc.NumParameters = 1;		// ルートパラメータ数
	rootSignatureDesc.pParameters = &rootParam;	// ルートパラメータの先頭アドレス

	// サンプラーの設定（uv値によってテクスチャデータからどう色を取り出すかを決めるための設定
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;					// 横方向の繰り返し
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;					// 縦方向の繰り返し
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;					// 奥行きの繰り返し
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	// ボーダーは黒
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;					// 線形補完
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;									// ミップマップ最大値
	samplerDesc.MinLOD = 0.0f;												// ミップマップ最小値
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;			// ピクセルシェーダーから見える
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;				// サンプリングしない

	// 上記の情報をルートシグネチャに設定
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &samplerDesc;

	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);

	// ルートシグネチャオブジェクトの作成（シェーダー作成の時の同様）
	result = _dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
	rootSigBlob->Release();
	gpipeline.pRootSignature = rootsignature;

	// グラフィックスパイプラインステートオブジェクトの生成
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));

	// ビューポートの作成
	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;		// 出力先の幅
	viewport.Height = window_height;	// 出力先の高さ
	viewport.TopLeftX = 0;				// 出力先の左上の座標X
	viewport.TopLeftY = 0;				// 出力先の左上の座標Y
	viewport.MaxDepth = 1.0f;			// 深度最大値
	viewport.MinDepth = 0.0f;			// 深度最小値

	// シザー矩形（ビューポートに出力された画像のどこからどこまでを実際に画面に映し出すか）
	D3D12_RECT scissorrect = {};
	scissorrect.top = 0;									// 切り抜き上座標
	scissorrect.left = 0;									// 切り抜き左座標
	scissorrect.right = scissorrect.left + window_width;	// 切り抜き右座標
	scissorrect.bottom = scissorrect.top + window_height;	// 切り抜き下座標

	struct TexRGBA
	{
		unsigned char R, G, B, A;
	};

	std::vector<TexRGBA> texturedata(256 * 256);

	for (auto& rgba : texturedata)
	{
		rgba.R = rand() % 256;
		rgba.G = rand() % 256;
		rgba.B = rand() % 256;
		rgba.A = rand() % 255;
	}

	// WICテクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	result = LoadFromWICFile(L"Asset/nier.png", WIC_FLAGS_NONE, &metadata, scratchImg);		// BMP・PNG・JPGなど基本的なファイル形式のロードはこの関数
	auto img = scratchImg.GetImage(0, 0, 0);												// 生の画像データを取り出す

	// テクスチャを転送するためのヒープを設定
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	// テクスチャリソースの設定
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = metadata.format;												// RGBAフォーマット
	resDesc.Width = metadata.width;													// 幅
	resDesc.Height = metadata.height;												// 高さ
	resDesc.DepthOrArraySize = metadata.arraySize;									// 2Dで配列でもないので１
	resDesc.SampleDesc.Count = 1;													// 通常テクスチャなのでアンチエイリアシングしない
	resDesc.SampleDesc.Quality = 0;													// クオリティは最低
	resDesc.MipLevels = metadata.mipLevels;											// ミップマップしないのでミップ数は１つ
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);	// テクスチャ２Dテクスチャ用
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;									// レイアウトは決定しない
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;										// 特にフラグ無し

	// テクスチャリソースの設定
	ID3D12Resource* texBuff = nullptr;
	result = _dev->CreateCommittedResource(&texHeapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&texBuff));
	result = texBuff->WriteToSubresource(
		0					// サブリソースのインデックス
		, nullptr			// 書き込み領域の指定(nullptrなら先頭から全領域
		, img->pixels		// 書き込みたいデータのアドレス
		, img->rowPitch		// 1行当たりのデータサイズ
		, img->slicePitch);	// スライス当たりのデータサイズ

	// シェーダーに渡すのための基本的な行列データ
	struct MatricesData
	{
		XMMATRIX world;		// モデル本体を回転させたり移動させたりする行列
		XMMATRIX viewproj;	// ビューとプロジェクション合成行列
	};
	MatricesData* mapMatrix;	// マップ先を示すポインタ

	auto worldMat = XMMatrixRotationY(XM_PIDIV4);
	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	auto projMat = XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(window_width) / static_cast<float>(window_height), 1.0f, 100.0f);

	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC rscDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatricesData) + 0xff) & ~0xff);

	// 定数バッファの作成
	ID3D12Resource* constBuff = nullptr;
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&rscDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff)
	);
									// マップ先を示すポインタ
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix);	// マップ
	mapMatrix->world = worldMat;
	mapMatrix->viewproj = viewMat * projMat;

	// シェーダーリソースビュー用のディスクリプタヒープを作る
	ID3D12DescriptorHeap* basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// シェーダーから見えるように
	descHeapDesc.NodeMask = 0;										// マスクは０
	descHeapDesc.NumDescriptors = 2;								// テクスチャビュー（SRV）と定数バッファビュー（CBV）の２つ
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		// シェーダーリソースビュー用
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));


	// シェーダーリソースビューを作る（構成が分からないメモリの塊をどう解釈するかを指定する）
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;												// RGBAを0.0～1.0fに正規化
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;		// 指定されたフォーマットにデータ通りの順番で割り当てられていることを表す
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;							// ２Dテクスチャを使用
	srvDesc.Texture2D.MipLevels = 1;												// ミップマップは使用しないため１
	_dev->CreateShaderResourceView(
		texBuff												// ビューと関連付けるバッファ
		, &srvDesc												// 先ほど設定したテクスチャ情報
		, basicDescHeap->GetCPUDescriptorHandleForHeapStart());	// ヒープのどこに割り当てるか

	// ディスクリプタの先頭ハンドルを取得しておく
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();

	// シェーダーリソースビューの作成
	_dev->CreateShaderResourceView(
		texBuff			// テクスチャビューと関連付ける
		, &srvDesc			// 先ほど設定したテクスチャ情報
		, basicHeapHandle);	// 先頭の場所を示すハンドル

	// 次の場所に移動
	basicHeapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constBuff->GetDesc().Width;

	// 定数バッファビューの作成
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);

	MSG msg = {};

	float angle = 0.0f;

	while (true)
	{
		worldMat = XMMatrixRotationY(angle);
		mapMatrix->world = worldMat;
		mapMatrix->viewproj = viewMat * projMat;
		angle += 0.001f;

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// アプリケーションが終わるときにmessageがWM_QUITになる
		if (msg.message == WM_QUIT)
		{
			break;
		}

		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		// バリアの設定
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;	// 遷移
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;		// 指定なし
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];		// バックバッファーリソース
		BarrierDesc.Transition.Subresource = 0;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;		// 直前はPRESENT状態
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;	// 今からレンダーターゲット状態
		_cmdList->ResourceBarrier(1, &BarrierDesc);			// バリア指定実行

		// パイプラインステートのセット
		_cmdList->SetPipelineState(_pipelinestate);

		// レンダーターゲットを指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);

		// 画面色クリア
		float clearColor[] = { 1.0f,1.0f,1.0f,1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		_cmdList->RSSetViewports(1, &viewport);									// ビューポートのセット
		_cmdList->RSSetScissorRects(1, &scissorrect);							// シザー矩形のセット
		_cmdList->SetGraphicsRootSignature(rootsignature);						// ルートシグネチャのセット
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	// トライアングルリストとして設定
		_cmdList->IASetVertexBuffers(0, 1, &vbView);							// 頂点バッファーのセット
		_cmdList->IASetIndexBuffer(&ibView);									// インデックスバッファーのセット

		_cmdList->SetGraphicsRootSignature(rootsignature);
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);
		_cmdList->SetGraphicsRootDescriptorTable(0, basicDescHeap->GetGPUDescriptorHandleForHeapStart());

		//auto heapHandle = basicDescHeap->GetGPUDescriptorHandleForHeapStart();
		//heapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//_cmdList->SetGraphicsRootDescriptorTable(1, heapHandle);

		_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0,0);					// 描画命令（インデックス数、インスタンス数・・・）
		//_cmdList->DrawInstanced(4, 1, 0, 0);									// 描画命令（頂点数、インスタンス数、頂点データのオフセット、インスタンスのオフセット）


		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		_cmdList->Close();

		ID3D12CommandList* cmdlists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);

		_cmdQueue->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal)
		{
			// イベントハンドルの取得
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);

			// イベントが発生するまで待ち続ける
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}

		_cmdAllocator->Reset();
		_cmdList->Reset(_cmdAllocator, _pipelinestate);

		_swapchain->Present(1, 0);
	}

	// もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}
