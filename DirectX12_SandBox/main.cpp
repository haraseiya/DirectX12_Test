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

// �R���\�[����ʂɃt�H�[�}�b�g�t���������\��
// ���̊֐��̓f�o�b�O�p
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif
}

// �ʓ|�����Ǐ����Ȃ���΂����Ȃ��֐�
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// �E�C���h�E���j�����ꂽ��Ă΂��
	if (msg == WM_DESTROY)
	{
		// OS�ɑ΂��āu�������̃A�v���͏I���v�Ɠ`����
		PostQuitMessage(0);
		return 0;
	}
	// ����̏������s��
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

	// �f�o�b�O���C���[��L��������
	debugLayer->EnableDebugLayer();

	// �L����������C���^�[�t�F�C�X���������
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
	w.lpfnWndProc = (WNDPROC)WindowProcedure;	// �R�[���o�b�N�֐��̎w��
	w.lpszClassName = _T("DirectX12_SandBox");	// �A�v���P�[�V�����N���X��
	w.hInstance = GetModuleHandle(nullptr);		// �n���h���̎擾
	RegisterClassEx(&w);						// �A�v���P�[�V�����N���X�i�E�B���h�E�N���X�̎w���OS�ɓ`����j

	RECT wrc{ 0,0,window_width,window_height };	// �E�B���h�E�T�C�Y�����߂�
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E�I�u�W�F�N�g�̐���
	HWND hwnd = CreateWindow(
		w.lpszClassName			//�N���X���w��
		, _T("DirectX12_Test")	// �^�C�g���o�[�̕���
		, WS_OVERLAPPEDWINDOW	// �^�C�g���o�[�Ƌ��E��������E�B���h�E
		, CW_USEDEFAULT			// �\��x���W��OS�ɂ��C��
		, CW_USEDEFAULT			// �\��y���W��OS�ɂ��C��
		, wrc.right - wrc.left	// �E�B���h�E��
		, wrc.bottom - wrc.top	// �E�B���h�E����
		, nullptr				// �e�E�B���h�E�n���h��
		, nullptr				// ���j���[�n���h��
		, w.hInstance			// �Ăяo���A�v���P�[�V�����n���h��
		, nullptr);				// �ǉ��p�����[�^�[

#ifdef _DEBUG
	// �f�o�b�O���C���[���I��
	EnableDebugLayer();
#endif

	// DirectX12�܂��̏�����
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

	// �A�_�v�^�[�̗񋓗p
	std::vector<IDXGIAdapter*> adapters;

	// �����ɓ���̖��O�̃A�_�v�^�[�I�u�W�F�N�g������
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};

		// �A�_�v�^�[�̐����I�u�W�F�N�g�擾
		adpt->GetDesc(&adesc);

		std::wstring strDesc = adesc.Description;

		// �T�������A�_�v�^�[�̖��O���m�F
		// �����ł�NVIDIA��T��
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
			break; // �����\�ȃo�[�W���������������烋�[�v��ł��؂�
		}
	}

	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&_cmdAllocator));
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));

	// �R�}���h�L���[�쐬
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.NodeMask = 0;										// �A�_�v�^�[��1�����g��Ȃ��Ƃ��͂O�ł悢
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// �v���C�I���e�B�͓��ɂȂ�
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;				// �R�}���h���X�g�ƍ��킹��
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));	// �L���[�쐬

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;					// �o�b�N�o�b�t�@�[�͐L�яk�݉\
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// �t���b�v��͑��₩�ɔj��
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;			// ���Ɏw��Ȃ�
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// �E�B���h�E�̃t���X�N���[���؂�ւ��\

	result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue, hwnd, &swapchainDesc, nullptr, nullptr, (IDXGISwapChain1**)&_swapchain);
	
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		// �ǂ�ȃr���[����邩�i�����_�[�^�[�Q�b�g�r���[���w��j
	heapDesc.NodeMask = 0;								// ������GPU������ꍇ�Ɏ��ʂ��s�����߂̃r�b�g�t���O�i�P�����̑z��Ȃ̂łO�j
	heapDesc.NumDescriptors = 2;						// �f�B�X�N���v�^�̐���\���i�\��ʂƗ���ʂ��ꂼ��̃o�b�t�@�[�ɑΉ�����r���[�Ȃ̂łQ���w��j
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// �r���[�ɂ���������V�F�[�_�[������Q�Ƃ���K�v�����邩�ǂ������w�肷��i�e�N�X�`���o�b�t�@�[��萔�o�b�t�@�[�ł���ΕK�v�j
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

		// �A�v���P�[�V�������I���Ƃ���message��WM_QUIT�ɂȂ�
		if (msg.message == WM_QUIT)
		{
			break;
		}

		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;	// �J��
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;		// �w��Ȃ�
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];		// �o�b�N�o�b�t�@�[���\�[�X
		BarrierDesc.Transition.Subresource = 0;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;		// ���O��PRESENT���
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;	// �����烌���_�[�^�[�Q�b�g���
		_cmdList->ResourceBarrier(1, &BarrierDesc);			// �o���A�w����s

		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		// ��ʐF�N���A
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
			// �C�x���g�n���h���̎擾
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);

			// �C�x���g����������܂ő҂�������
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}

		_cmdAllocator->Reset();
		_cmdList->Reset(_cmdAllocator, nullptr);

		_swapchain->Present(1,0);
	}

	// �����N���X�͎g��Ȃ��̂œo�^��������
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}
