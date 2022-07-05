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

	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
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
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	// �K���}�␳����
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int idx = 0; idx < swcDesc.BufferCount; ++idx)
	{
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		_dev->CreateRenderTargetView(_backBuffers[idx], &rtvDesc, handle);
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// �[�x�o�b�t�@�̍쐬
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 2�����̃e�N�X�`���f�[�^
	depthResDesc.Width = window_width;								// ���̓����_�[�^�[�Q�b�g�Ɠ���
	depthResDesc.Height = window_height;							// �����������_�[�^�[�Q�b�g�Ɠ���
	depthResDesc.DepthOrArraySize = 1;								// �e�N�X�`���z��ł��RD�e�N�X�`���ł��Ȃ�
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;					// �[�x�l�������ݗp�t�H�[�}�b�g
	depthResDesc.SampleDesc.Count = 1;								// �T���v���̓s�N�Z��������P��
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// �f�v�X�X�e���V���Ƃ��Ďg�p

	// �[�x�l�p�̃q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;					// �f�t�H���g�Ȃ̂ł��Ƃ�UNKNOW�ł悢
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// �N���A�o�����[�i�d�v�j
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;		// �[��1.0f�i�ő�l�j�ŃN���A
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;	// 32�r�b�gfloat�l�Ƃ��ăN���A

	// �f�v�X�o�b�t�@�쐬
	ID3D12Resource* depthBuffer = nullptr;
	result = _dev->CreateCommittedResource(&depthHeapProp, D3D12_HEAP_FLAG_NONE, &depthResDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&depthBuffer));

	// �[�x�̂��߂̃f�B�X�N���v�^�q�[�v���쐬
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;						// �[�x�r���[�͂P��
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;	// �f�v�X�X�e���V���r���[�Ƃ��Ďg��
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

	// �[�x�r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;					// �[�x�l��32�r�b�g�g�p
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// 2D�e�N�X�`��
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;					// �t���O�͓��ɖ���
	_dev->CreateDepthStencilView(depthBuffer, &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

	// �t�F���X���쐬
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	ShowWindow(hwnd, SW_SHOW);

	// PMD�w�b�_�[�̍\����
	struct PMDHeader
	{
		float version;			// �o�[�W����
		char model_name[20];	// ���f����
		char comment[256];		// ���f���R�����g
	};

	// �t�@�C���ǂݍ���
	char signature[3] = {};
	PMDHeader pmdHeader = {};
	auto fp = fopen("Asset/�����~�N/�����~�N@���t1052��.pmd", "rb");
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	constexpr size_t pmdVertex_size = 38;	// ���_1������̃T�C�Y�i���W�P�Q�A�@���P�Q�Auv�W�A�{�[���S�A�{�[���e���x�P�A�֊s���P�j
	unsigned int vertNum;					// ���_��
	fread(&vertNum, sizeof(vertNum), 1, fp);

	std::vector<unsigned char> vertices(vertNum* pmdVertex_size);	// �o�b�t�@�̊m��
	fread(vertices.data(), vertices.size(), 1, fp);					// �ǂݍ���

	// ���_�f�[�^�\����
	struct Vertex
	{
		XMFLOAT3 pos;	// xyz���W
		XMFLOAT2 uv;	// uv���W
	};

	//Vertex vertices[] =
	//{
	//	{{-1.0f,-1.0f,0.0f},{0.0f,1.0f}},	// ����
	//	{{-1.0f,1.0f,0.0f},{0.0f,0.0f}},		// ����
	//	{{1.0f,-1.0f,0.0f},{1.0f,1.0f}},	// �E��
	//	{{1.0f,1.0f,0.0f} ,{1.0f,0.0f}},	// �E��
	//};


	ID3D12Resource* vertBuff = nullptr;
	CD3DX12_HEAP_PROPERTIES vertHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC vertResDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size());
	result = _dev->CreateCommittedResource(&vertHeapProp, D3D12_HEAP_FLAG_NONE, &vertResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertBuff));

	//// d3dx12.h���g�p�����ȈՓI�ȏ�����
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

	// ���_�o�b�t�@�r���[�̍쐬
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();	// GPU��̃o�b�t�@�[�̉��z�A�h���X���擾����iGPU���͂ǂ̃o�b�t�@�[����f�[�^���ǂ̂��炢����΂悢���킩��j
	vbView.SizeInBytes = vertices.size();						// �S�o�C�g��
	vbView.StrideInBytes = pmdVertex_size;						// 1���_������̃o�C�g��

	// �C���f�b�N�X
	std::vector<unsigned short> indices;	// �C���f�b�N�X�z��
	unsigned int indicesNum;				// �C���f�b�N�X��

	// �C���f�b�N�X���ǂݍ���
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	fclose(fp);

	// �C���f�b�N�X�o�b�t�@�[����
	ID3D12Resource* idxBuff = nullptr;
	CD3DX12_HEAP_PROPERTIES idxHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC idxResDesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));

	vertResDesc.Width = sizeof(indices);
	result = _dev->CreateCommittedResource(&idxHeapProp, D3D12_HEAP_FLAG_NONE, &idxResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&idxBuff));

	// ������o�b�t�@�ɃC���f�b�N�X�o�b�t�@���쐬
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	// �C���f�b�N�X�o�b�t�@�[�r���[���쐬
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);

	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	// ���_�V�F�[�_�[�̃R���p�C��
	result = D3DCompileFromFile(L"VertexShader07.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS07", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_vsBlob, &errorBlob);
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
	result = D3DCompileFromFile(L"PixelShader07.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PS07", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_psBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("�t�@�C������������܂���");
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

	// ���_���C�A�E�g��ݒ�
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"BONE_NO",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"WEIGHT",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"EDGE_FLG",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
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

	// �[�x�X�e���V���̐ݒ�
	gpipeline.DepthStencilState.DepthEnable = true;								// �[�x�o�b�t�@���g��
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	// �s�N�Z���`�掞�ɐ[�x�o�b�t�@�ɐ[�x�l����������
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;			// �[�x�l���������ق����̗p
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;								// 32�r�b�gfloat��[�x�l�Ƃ��Ďg�p����

	// �A���`�G�C���A�V���O�̂��߂̃T���v�����ݒ�
	gpipeline.SampleDesc.Count = 1;		// �T���v�����O��1�s�N�Z���ɂ��P
	gpipeline.SampleDesc.Quality = 0;	// �N�I���e�B�͍Œ�

	// ��̃��[�g�V�O�l�`�����쐬����
	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;	// ���_���i���̓A�Z���u���j������

	// �f�B�X�N���v�^�e�[�u���̐ݒ�i�����̃e�N�X�`���o�b�t�@�[�r���[���܂Ƃ߂Ďw��ł���B����̓e�N�X�`���ƒ萔�̂Q�j
	D3D12_DESCRIPTOR_RANGE descTblRange[2] = {};				// �e�N�X�`���ƒ萔�̂Q��
	descTblRange[0].NumDescriptors = 1;							// �e�N�X�`���P��
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// ��ʂ̓e�N�X�`��
	descTblRange[0].BaseShaderRegister = 0;						// 0�ԃX���b�g����
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	descTblRange[1].NumDescriptors = 1;							// �萔�P��
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;// ��ʂ̓e�N�X�`��
	descTblRange[1].BaseShaderRegister = 0;						// 0�ԃX���b�g����
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ���[�g�p�����[�^�̍쐬
	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.DescriptorTable.pDescriptorRanges = descTblRange;
	rootParam.DescriptorTable.NumDescriptorRanges = 2;						// �f�B�X�N���v�^�����W��
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;				// �s�N�Z���V�F�[�_�[���猩����

	rootSignatureDesc.NumParameters = 1;		// ���[�g�p�����[�^��
	rootSignatureDesc.pParameters = &rootParam;	// ���[�g�p�����[�^�̐擪�A�h���X

	// �T���v���[�̐ݒ�iuv�l�ɂ���ăe�N�X�`���f�[�^����ǂ��F�����o���������߂邽�߂̐ݒ�
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;					// �������̌J��Ԃ�
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;					// �c�����̌J��Ԃ�
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;					// ���s���̌J��Ԃ�
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	// �{�[�_�[�͍�
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;					// ���`�⊮
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;									// �~�b�v�}�b�v�ő�l
	samplerDesc.MinLOD = 0.0f;												// �~�b�v�}�b�v�ŏ��l
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;			// �s�N�Z���V�F�[�_�[���猩����
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;				// �T���v�����O���Ȃ�

	// ��L�̏������[�g�V�O�l�`���ɐݒ�
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &samplerDesc;

	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);

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

	// WIC�e�N�X�`���̃��[�h
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	result = LoadFromWICFile(L"Asset/nier.png", WIC_FLAGS_NONE, &metadata, scratchImg);		// BMP�EPNG�EJPG�ȂǊ�{�I�ȃt�@�C���`���̃��[�h�͂��̊֐�
	auto img = scratchImg.GetImage(0, 0, 0);												// ���̉摜�f�[�^�����o��

	// �e�N�X�`����]�����邽�߂̃q�[�v��ݒ�
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	// �e�N�X�`�����\�[�X�̐ݒ�
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = metadata.format;												// RGBA�t�H�[�}�b�g
	resDesc.Width = metadata.width;													// ��
	resDesc.Height = metadata.height;												// ����
	resDesc.DepthOrArraySize = metadata.arraySize;									// 2D�Ŕz��ł��Ȃ��̂łP
	resDesc.SampleDesc.Count = 1;													// �ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
	resDesc.SampleDesc.Quality = 0;													// �N�I���e�B�͍Œ�
	resDesc.MipLevels = metadata.mipLevels;											// �~�b�v�}�b�v���Ȃ��̂Ń~�b�v���͂P��
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);	// �e�N�X�`���QD�e�N�X�`���p
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;									// ���C�A�E�g�͌��肵�Ȃ�
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;										// ���Ƀt���O����

	// �e�N�X�`�����\�[�X�̐ݒ�
	ID3D12Resource* texBuff = nullptr;
	result = _dev->CreateCommittedResource(&texHeapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&texBuff));
	result = texBuff->WriteToSubresource(
		0					// �T�u���\�[�X�̃C���f�b�N�X
		, nullptr			// �������ݗ̈�̎w��(nullptr�Ȃ�擪����S�̈�
		, img->pixels		// �������݂����f�[�^�̃A�h���X
		, img->rowPitch		// 1�s������̃f�[�^�T�C�Y
		, img->slicePitch);	// �X���C�X������̃f�[�^�T�C�Y

	// �V�F�[�_�[�ɓn���̂��߂̊�{�I�ȍs��f�[�^
	struct MatricesData
	{
		XMMATRIX world;		// ���f���{�̂���]��������ړ��������肷��s��
		XMMATRIX viewproj;	// �r���[�ƃv���W�F�N�V���������s��
	};
	MatricesData* mapMatrix;	// �}�b�v��������|�C���^

	auto worldMat = XMMatrixRotationY(XM_PIDIV4);
	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	auto projMat = XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(window_width) / static_cast<float>(window_height), 1.0f, 100.0f);

	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC rscDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatricesData) + 0xff) & ~0xff);

	// �萔�o�b�t�@�̍쐬
	ID3D12Resource* constBuff = nullptr;
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&rscDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff)
	);
									// �}�b�v��������|�C���^
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix);	// �}�b�v
	mapMatrix->world = worldMat;
	mapMatrix->viewproj = viewMat * projMat;

	// �V�F�[�_�[���\�[�X�r���[�p�̃f�B�X�N���v�^�q�[�v�����
	ID3D12DescriptorHeap* basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// �V�F�[�_�[���猩����悤��
	descHeapDesc.NodeMask = 0;										// �}�X�N�͂O
	descHeapDesc.NumDescriptors = 2;								// �e�N�X�`���r���[�iSRV�j�ƒ萔�o�b�t�@�r���[�iCBV�j�̂Q��
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		// �V�F�[�_�[���\�[�X�r���[�p
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));


	// �V�F�[�_�[���\�[�X�r���[�����i�\����������Ȃ��������̉���ǂ����߂��邩���w�肷��j
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;												// RGBA��0.0�`1.0f�ɐ��K��
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;		// �w�肳�ꂽ�t�H�[�}�b�g�Ƀf�[�^�ʂ�̏��ԂŊ��蓖�Ă��Ă��邱�Ƃ�\��
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;							// �QD�e�N�X�`�����g�p
	srvDesc.Texture2D.MipLevels = 1;												// �~�b�v�}�b�v�͎g�p���Ȃ����߂P
	_dev->CreateShaderResourceView(
		texBuff												// �r���[�Ɗ֘A�t����o�b�t�@
		, &srvDesc												// ��قǐݒ肵���e�N�X�`�����
		, basicDescHeap->GetCPUDescriptorHandleForHeapStart());	// �q�[�v�̂ǂ��Ɋ��蓖�Ă邩

	// �f�B�X�N���v�^�̐擪�n���h�����擾���Ă���
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();

	// �V�F�[�_�[���\�[�X�r���[�̍쐬
	_dev->CreateShaderResourceView(
		texBuff			// �e�N�X�`���r���[�Ɗ֘A�t����
		, &srvDesc			// ��قǐݒ肵���e�N�X�`�����
		, basicHeapHandle);	// �擪�̏ꏊ�������n���h��

	// ���̏ꏊ�Ɉړ�
	basicHeapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constBuff->GetDesc().Width;

	// �萔�o�b�t�@�r���[�̍쐬
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
		auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);

		// ��ʐF�N���A
		float clearColor[] = { 1.0f,1.0f,1.0f,1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		_cmdList->RSSetViewports(1, &viewport);									// �r���[�|�[�g�̃Z�b�g
		_cmdList->RSSetScissorRects(1, &scissorrect);							// �V�U�[��`�̃Z�b�g
		_cmdList->SetGraphicsRootSignature(rootsignature);						// ���[�g�V�O�l�`���̃Z�b�g
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	// �g���C�A���O�����X�g�Ƃ��Đݒ�
		_cmdList->IASetVertexBuffers(0, 1, &vbView);							// ���_�o�b�t�@�[�̃Z�b�g
		_cmdList->IASetIndexBuffer(&ibView);									// �C���f�b�N�X�o�b�t�@�[�̃Z�b�g

		_cmdList->SetGraphicsRootSignature(rootsignature);
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);
		_cmdList->SetGraphicsRootDescriptorTable(0, basicDescHeap->GetGPUDescriptorHandleForHeapStart());

		//auto heapHandle = basicDescHeap->GetGPUDescriptorHandleForHeapStart();
		//heapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//_cmdList->SetGraphicsRootDescriptorTable(1, heapHandle);

		_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0,0);					// �`�施�߁i�C���f�b�N�X���A�C���X�^���X���E�E�E�j
		//_cmdList->DrawInstanced(4, 1, 0, 0);									// �`�施�߁i���_���A�C���X�^���X���A���_�f�[�^�̃I�t�Z�b�g�A�C���X�^���X�̃I�t�Z�b�g�j


		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
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

		_swapchain->Present(1, 0);
	}

	// �����N���X�͎g��Ȃ��̂œo�^��������
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}
