#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>

#ifdef _DEBUG
#include <iostream>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

using namespace std;

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

	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&_cmdAllocator));
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
	for (int idx = 0; idx < swcDesc.BufferCount; ++idx)
	{
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		_dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	ShowWindow(hwnd, SW_SHOW);

	MSG msg = {};

	while (true)
	{
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

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;	// 遷移
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;		// 指定なし
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];		// バックバッファーリソース
		BarrierDesc.Transition.Subresource = 0;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;		// 直前はPRESENT状態
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;	// 今からレンダーターゲット状態
		_cmdList->ResourceBarrier(1, &BarrierDesc);			// バリア指定実行

		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		// 画面色クリア
		float clearColor[] = { 1.0f,1.0f,0.0f,1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter= D3D12_RESOURCE_STATE_PRESENT;
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
		_cmdList->Reset(_cmdAllocator, nullptr);

		_swapchain->Present(1,0);
	}

	// もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}
