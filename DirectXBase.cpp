#include "DirectXBase.h"
#include "StringUtility.h"
#include "Logger.h"

using namespace StringUtility;


void DirectXBase::Initialize(WinApp* winApi)
{
	//
	assert(winApi);

	//
	this->winApp = winApi;

	void DeviceInitialize();

	void CommandInitialize();

	void SwapChainCreate();

	void DepthStencilCreate();

	void CreateDescriptorHeap();

	void RenderTargetViewInitialize();

	void DepthStencilViewInitialize();

	void FenceInitialize();

	void ViewPortInitialize();

	void ScissorInitialize();

	void DxcCompiler();

	void ImGuiInitialize(WinApp * winApi, DXGI_SWAP_CHAIN_DESC1 swapChainDesc, D3D12_RENDER_TARGET_VIEW_DESC rtvDesc);

//
//
//#pragma region rtvDescriptorHeap生成
//
//	//rtvDesc
//	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
//	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
//	rtvDescriptorHeapDesc.NumDescriptors = 2;
//	hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
//	assert(SUCCEEDED(hr));
//
//
//#pragma endregion


}

void DirectXBase::DeviceInitialize()
{

#ifdef _DEBUG

	Microsoft::WRL::ComPtr < ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		//
		debugController->EnableDebugLayer();
		//
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
	;

#endif // _DEBUG

	//DXGIファクトリーの生成
	Microsoft::WRL::ComPtr < IDXGIFactory7> dxgiFactory = nullptr;

	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

	assert(SUCCEEDED(hr));

	//使用するアダプタ用の変数。最初にnullptrを入れておく
	Microsoft::WRL::ComPtr < IDXGIAdapter4> useAdapter = nullptr;
	//よい順にアダプタを頼む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
		//アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));//取得できないのは一大事
		//ソフトウェアアダプタでなければ採用！
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			//採用したアダプタの情報をログに出力。wstringの方なので注意
			Logger::Log(ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;//ソフトウェアアダプタの場合は見なかったことにする
	}
	//適切なアダプタが見つからなかった
	assert(useAdapter != nullptr);



	//デバイスの生成
	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;
	//機能レベルとログ出力の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};

	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };

	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));

		if (SUCCEEDED(hr)) {
			Logger::Log(std::format("FeatureLevel:{}\n", featureLevelStrings[i]));
			break;
		}
	}
	//デバイスの生成がうまくいかなかったので起動できない
	assert(device != nullptr);
	Logger::Log("Complete create D3D12device!!!\n");


#ifdef _DEBUG

	Microsoft::WRL::ComPtr < ID3D12InfoQueue> infoQueue = nullptr;

	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		//
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		//
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		//
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		//制御するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {
			//
			//
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		//
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		//
		infoQueue->PushStorageFilter(&filter);

		//
		/*infoQueue->Release();*/
	}


#endif // _DEBUG

}

void DirectXBase::CommandInitialize()
{
	HRESULT hr;

	//コマンドキューを生成する
	Microsoft::WRL::ComPtr < ID3D12CommandQueue> commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	//コマンドキューの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//コマンドアロケータを生成する
	Microsoft::WRL::ComPtr < ID3D12CommandAllocator> commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	//コマンドアロケータの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//コマンドリストを生成する
	Microsoft::WRL::ComPtr < ID3D12GraphicsCommandList> commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS
	(&commandList));
	//コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

}

void DirectXBase::SwapChainCreate()
{
	HRESULT hr;

	#pragma region swapchainの設定
	
		
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
		swapChainDesc.Width = WinApp::kClientWidth;
		swapChainDesc.Height = WinApp::kClientHeight;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	
		hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), winApp->GetHwnd(), &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
		assert(SUCCEEDED(hr));
	
	#pragma endregion

}

void DirectXBase::DepthStencilCreate()
{
	//
	Microsoft::WRL::ComPtr < ID3D12Resource> depthStencilResource = DirectXBase::CreateDepthStencilTextureResource(device.Get(), WinApp::kClientWidth, WinApp::kClientHeight);

#pragma region 深度バッファの生成

	//
	depthStencilDesc.DepthEnable = true;
	//
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

#pragma endregion
}

Microsoft::WRL::ComPtr < ID3D12Resource> DirectXBase::CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height)
{
	//D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	//
	//D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	//
	//D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//
	Microsoft::WRL::ComPtr < ID3D12Resource> resource = nullptr;

	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr));
	return resource;
}

void DirectXBase::CreateDescriptorHeap()
{

#pragma region descriptorHeapの設定

#pragma region DescriptorSize取得

	//ディスクリプターヒープのサイズの設定
	const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

#pragma endregion

	//rtv
	Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	//srv
	Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	//dsv
	Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);


#pragma endregion

}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXBase::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
{
	return Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>();
}


#pragma region レンダーターゲットビューの初期化

void DirectXBase::RenderTargetViewInitialize() {

	HRESULT hr;

	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	//bufferの取得
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));


#pragma region RTV設定

	//renderTargetViewの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	//descriptorの先頭の取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	
	//要素数変更
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

#pragma endregion

	for (uint32_t i = 0; i < 3; ++i) {

		rtvHandles[0] = rtvStartHandle;
		device->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);
		//
		rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		//
		device->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);

	}

}

void DirectXBase::PreDraw()
{
	//
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
	//
	D3D12_RESOURCE_BARRIER barrier{};
	//
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
	//
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	//
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//
	dxBase->GetCommandList()->ResourceBarrier(1, &barrier);



	dxBase->GetCommandList()->OMSetRenderTargets(1, rtvHandles[backBufferIndex], false, &dsvHandle);

	float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
	dxBase->GetCommandList()->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
	//
	dxBase->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);



	//ビューポートの設定
	dxBase->GetCommandList()->RSSetViewports(1, &viewport);//
	dxBase->GetCommandList()->RSSetScissorRects(1, &dxBase->GetScissorRect());//
}

void DirectXBase::PostDraw()
{
	//
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

#pragma region トランジションバリアの設定

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxBase->GetCommandList());

	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	//
	dxBase->GetCommandList()->ResourceBarrier(1, &barrier);

#pragma endregion

	//コマンドリストの内容を確定
	hr = dxBase->GetCommandList()->Close();
	assert(SUCCEEDED(hr));

	//コマンドリストの実行
	Microsoft::WRL::ComPtr < ID3D12CommandList> commandLists[] = { dxBase->GetCommandList() };
	dxBase->GetCommandQueue()->ExecuteCommandLists(1, commandLists->GetAddressOf());
	//
	swapChain->Present(1, 0);

	//
	fenceValue++;
	//
	dxBase->GetCommandQueue()->Signal(dxBase->GetFence(), dxBase->GetFenceValue());
	//
	//
	if (dxBase->GetFence()->GetCompletedValue() < dxBase->GetFenceValue()) {
		//
		dxBase->GetFence()->SetEventOnCompletion(dxBase->GetFenceValue(), fenceEvent);
		//
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	//
	HRESULT hr;
	hr = dxBase->GetCommandAllocator()->Reset();
	assert(SUCCEEDED(hr));
	hr = dxBase->GetCommandList()->Reset(dxBase->GetCommandAllocator(), nullptr);
	assert(SUCCEEDED(hr));



}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXBase::GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	return D3D12_CPU_DESCRIPTOR_HANDLE();
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXBase::GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	return D3D12_GPU_DESCRIPTOR_HANDLE();
}

#pragma endregion

D3D12_CPU_DESCRIPTOR_HANDLE DirectXBase::GetSRVCPUDescriptorHandle(uint32_t index)
{
	return GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);;
}

void DirectXBase::DepthStencilViewInitialize()
{
	//DSV設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	//DSV生成
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

}

void DirectXBase::FenceInitialize()
{
	HRESULT hr;

	//初期値0でFenceを作る
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

}

void DirectXBase::ViewPortInitialize()
{

#pragma region ビューポートの初期化
	
	//
	viewport.Width = WinApp::kClientWidth;
	viewport.Height = WinApp::kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

#pragma endregion

}

void DirectXBase::ScissorInitialize()
{

#pragma region シザーの初期化
	
	//
	scissorRect.left = 0;
	scissorRect.right = WinApp::kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WinApp::kClientHeight;

#pragma endregion

}

void DirectXBase::DxcCompiler()
{
	HRESULT hr;

	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	//現時点でincludeはしないが、includeに対応するための設定を行っておく
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));


}

void DirectXBase::ImGuiInitialize(WinApp*winApi, DXGI_SWAP_CHAIN_DESC1 swapChainDesc, D3D12_RENDER_TARGET_VIEW_DESC rtvDesc)
{
	#pragma region ImGuiの初期化

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApi->GetHwnd());
	ImGui_ImplDX12_Init(device.Get(),
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap.Get(),
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

#pragma endregion
}



