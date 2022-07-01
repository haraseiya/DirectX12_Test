#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <DirectXMath.h>
#include <d3dcompiler.h>

#ifdef _DEBUG
#include <iostream>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace std;
using namespace DirectX;

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

	XMFLOAT3 vertices[] =
	{
		{-0.4f,-0.7f,0.0f},	// ����
		{-0.4f,0.7f,0.0f},	// ����
		{0.4f,-0.7f,0.0f},	// �E��
		{0.4f,0.7f,0.0f},	// �E��
	};


	// ���_�o�b�t�@�̐���
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices);	// ���_��񂪓��邾���̃T�C�Y
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* vertBuff = nullptr;
	result = _dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resdesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertBuff));

	XMFLOAT3* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);

	// ���_�o�b�t�@�r���[�̍쐬
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();	// GPU��̃o�b�t�@�[�̉��z�A�h���X���擾����iGPU���͂ǂ̃o�b�t�@�[����f�[�^���ǂ̂��炢����΂悢���킩��j
	vbView.SizeInBytes = sizeof(vertices);						// �S�o�C�g��
	vbView.StrideInBytes = sizeof(vertices[0]);					// 1���_������̃o�C�g��

	// �C���f�b�N�X
	unsigned short indices[] =
	{
		0,1,2,
		2,1,3
	};

	// �C���f�b�N�X�o�b�t�@�[����
	ID3D12Resource* idxBuff = nullptr;
	resdesc.Width = sizeof(indices);
	result = _dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resdesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&idxBuff));

	// ������o�b�t�@�ɃC���f�b�N�X�o�b�t�@���쐬
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices),std::end(indices),mappedIdx);
	idxBuff->Unmap(0, nullptr);

	// �C���f�b�N�X�o�b�t�@�[�r���[���쐬
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);

	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	// ���_�V�F�[�_�[�̃R���p�C��
	result = D3DCompileFromFile(L"BasicVertexShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "BasicVS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_vsBlob, &errorBlob);
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("�t�@�C������������܂���");
			return 0;
		}
		else
		{
			std::string errstr;							// �󂯎��pstring
			errstr.resize(errorBlob->GetBufferSize());	// �K�v�ȃT�C�Y���m��

			// �f�[�^���R�s�[
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			::OutputDebugStringA(errstr.c_str());
		}
		exit(1);
	}

	// �s�N�Z���V�F�[�_�[�̃R���p�C��
	result = D3DCompileFromFile(L"BasicPixelShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "BasicPS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_psBlob, &errorBlob);
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("�t�@�C������������܂���");
			return 0;
		}
		else
		{
			std::string errstr;							// �󂯎��pstring
			errstr.resize(errorBlob->GetBufferSize());	// �K�v�ȃT�C�Y���m��

			// �f�[�^���R�s�[
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			::OutputDebugStringA(errstr.c_str());
		}
		exit(1);
	}

	// ���_���C�A�E�g��ݒ�
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	// �O���t�B�b�N�X�p�C�v���C���X�e�[�g�̐ݒ�i���_�V�F�[�_�[�ƃs�N�Z���V�F�[�_�[�j
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr;
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();	
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

	// �T���v���}�X�N�ƃ��X�^���C�U�X�e�[�g�̐ݒ�
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;			// �f�t�H���g�̃T���v���}�X�N��\���萔
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;
	gpipeline.RasterizerState.MultisampleEnable = false;		// �A���`�G�C���A�X�͎g��Ȃ�����false
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// �w�ʃJ�����O�͍s��Ȃ�
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;	// �|���S���̒��g��h��Ԃ�
	gpipeline.RasterizerState.DepthClipEnable = true;			// �[�x�����̃N���b�s���O�͗L����

	// �u�����h�X�e�[�g�̐ݒ�
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	// ���̓��C�A�E�g�̐ݒ�
	gpipeline.InputLayout.pInputElementDescs = inputLayout;						// ���C�A�E�g�̐擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(inputLayout);					// ���C�A�E�g�z��̗v�f��(_countof�͔z��̗v�f�����擾����}�N���j
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	// ���_���m����ɐ؂藣���Ȃ�
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// �O�p�`�ō\��
	
	// �����_�[�^�[�Q�b�g�̐ݒ�
	gpipeline.NumRenderTargets = 1;	// ���͂P�̂�
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	// �O�`�P�ɐ��K�����ꂽRGBA

	// �A���`�G�C���A�V���O�̂��߂̃T���v�����ݒ�
	gpipeline.SampleDesc.Count = 1;		// �T���v�����O��1�s�N�Z���ɂ��P
	gpipeline.SampleDesc.Quality = 0;	// �N�I���e�B�͍Œ�

	// ��̃��[�g�V�O�l�`�����쐬����
	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;	// ���_���i���̓A�Z���u���j������
	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc,D3D_ROOT_SIGNATURE_VERSION_1_0,&rootSigBlob,&errorBlob);

	// ���[�g�V�O�l�`���I�u�W�F�N�g�̍쐬�i�V�F�[�_�[�쐬�̎��̓��l�j
	result = _dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
	rootSigBlob->Release();
	gpipeline.pRootSignature = rootsignature;

	// �O���t�B�b�N�X�p�C�v���C���X�e�[�g�I�u�W�F�N�g�̐���
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));

	// �r���[�|�[�g�̍쐬
	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;		// �o�͐�̕�
	viewport.Height = window_height;	// �o�͐�̍���
	viewport.TopLeftX = 0;				// �o�͐�̍���̍��WX
	viewport.TopLeftY = 0;				// �o�͐�̍���̍��WY
	viewport.MaxDepth = 1.0f;			// �[�x�ő�l
	viewport.MinDepth = 0.0f;			// �[�x�ŏ��l

	// �V�U�[��`�i�r���[�|�[�g�ɏo�͂��ꂽ�摜�̂ǂ�����ǂ��܂ł����ۂɉ�ʂɉf���o�����j
	D3D12_RECT scissorrect = {};
	scissorrect.top = 0;									// �؂蔲������W
	scissorrect.left = 0;									// �؂蔲�������W
	scissorrect.right = scissorrect.left + window_width;	// �؂蔲���E���W
	scissorrect.bottom = scissorrect.top + window_height;	// �؂蔲�������W

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

		// �o���A�̐ݒ�
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;	// �J��
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;		// �w��Ȃ�
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];		// �o�b�N�o�b�t�@�[���\�[�X
		BarrierDesc.Transition.Subresource = 0;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;		// ���O��PRESENT���
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;	// �����烌���_�[�^�[�Q�b�g���
		_cmdList->ResourceBarrier(1, &BarrierDesc);			// �o���A�w����s

		// �p�C�v���C���X�e�[�g�̃Z�b�g
		_cmdList->SetPipelineState(_pipelinestate);

		// �����_�[�^�[�Q�b�g���w��
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		// ��ʐF�N���A
		float clearColor[] = { 1.0f,1.0f,0.0f,1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		_cmdList->RSSetViewports(1, &viewport);									// �r���[�|�[�g�̃Z�b�g
		_cmdList->RSSetScissorRects(1, &scissorrect);							// �V�U�[��`�̃Z�b�g
		_cmdList->SetGraphicsRootSignature(rootsignature);						// ���[�g�V�O�l�`���̃Z�b�g
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	// �g���C�A���O�����X�g�Ƃ��Đݒ�
		_cmdList->IASetVertexBuffers(0, 1, &vbView);							// ���_�o�b�t�@�[�̃Z�b�g
		_cmdList->IASetIndexBuffer(&ibView);									// �C���f�b�N�X�o�b�t�@�[�̃Z�b�g
		_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);							// �`�施�߁i�C���f�b�N�X���A�C���X�^���X���E�E�E�j
		//_cmdList->DrawInstanced(4, 1, 0, 0);									// �`�施�߁i���_���A�C���X�^���X���A���_�f�[�^�̃I�t�Z�b�g�A�C���X�^���X�̃I�t�Z�b�g�j


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
		_cmdList->Reset(_cmdAllocator, _pipelinestate);

		_swapchain->Present(1,0);
	}

	// �����N���X�͎g��Ȃ��̂œo�^��������
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}
