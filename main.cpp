#include<Windows.h>
#include<cstdint>
#include<string>
#include<format>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<cassert>
#include<dxgidebug.h>
#include<dxcapi.h>
#include"Vector4.h"
#include"MyMath.h"
#include"MatrixMath.h"
#include"Vector3.h"
#include"externals/DirectXTex/DirectXTex.h"
#include"light.h"
#include"transformationMatrix.h"
#define _USE_MATH_DEFINES
#include<cmath>
#include<math.h>



#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")

#include"externals/imgui/imgui.h"
#include"externals/imgui/imgui_impl_dx12.h"
#include"externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


struct Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};


struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};


struct MaterialData {
	Vector4 color;
	int32_t enableLighting;
};


//ウィンドウプロシージャー
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

//文字列を格納する
std::string str0{ "STRING!!!" };

//整数を文字列にする
std::string str1{ std::to_string(10) };

//変換する関数
std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}


void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}



IDxcBlob* CompileShader(

	//CompilerするShaderファイルへのパス
	const std::wstring& filePath,

	//Compilerに使用するProfile
	const wchar_t* profile,

	//初期化で生成したものを3つ
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler)

{
	//ここからシェーダーをコンパイルする旨をログに出す
	Log(ConvertString(std::format(L"Begin CompileShader,path:{},profile:{}\n", filePath, profile)));

	//hlslファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);

	//読めなかったら止める
	assert(SUCCEEDED(hr));

	//読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;


	LPCWSTR arguments[] = {
		filePath.c_str(),//コンパイル対象のhlslファイル名
		L"-E",L"main",//エントリーポイントの指定、基本的にmian以外にはしない
		L"-T",profile,//ShaderPrifileの設定
		L"-Zi",L"-Qembed_debug",//デバッグ用の情報を埋め込む
		L"-Od",	//最適化を外しておく
		L"-Zpr",//メモリレイアウトは行優先
	};

	//実際にShaderをコンパイルする
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,//読み込んだファイル
		arguments,			//コンパイルオプション
		_countof(arguments),//コンパイルオプションの数
		includeHandler,		//includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult)//コンパイル結果
	);

	//コンパイルエラーではなくdxcが起動できないなど致命的な状況
	assert(SUCCEEDED(hr));


	//警告・エラーが出てたらログに出して止める
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer());
		//警告・エラーダメゼッタイ
		assert(false);
	}


	//コンパイル結果から実行用のバイナリ部分を取得
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	//成功したログを出す
	Log(ConvertString(std::format(L"Compile Succeeded,path:{}\n", filePath, profile)));

	//もう使わないリソースを解放
	shaderSource->Release();
	shaderResult->Release();

	//実行用のバイナリを返却
	return shaderBlob;

}


//
ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {


	//
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;


	D3D12_RESOURCE_DESC vertexResourseDesc{};
	//
	vertexResourseDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourseDesc.Width = sizeInBytes;
	//
	vertexResourseDesc.Height = 1;
	vertexResourseDesc.DepthOrArraySize = 1;
	vertexResourseDesc.MipLevels = 1;
	vertexResourseDesc.SampleDesc.Count = 1;
	//
	vertexResourseDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr;

	//
	ID3D12Resource* vertexResource = nullptr;
	hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourseDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));

	return vertexResource;




}

//
ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

DirectX::ScratchImage LoadTexture(const std::string& filePath) {

	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	return mipImages;
}


ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata) {

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr));
	return resource;

}


void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages) {

	const DirectX::TexMetadata metadata = mipImages.GetMetadata();

	for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel)
	{
		const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);

		HRESULT hr = texture->WriteToSubresource(
			UINT(mipLevel),
			nullptr,
			img->pixels,
			UINT(img->rowPitch),
			UINT(img->slicePitch)
		);
		assert(SUCCEEDED(hr));
	}
}


ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height) {

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	//
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	//
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//
	ID3D12Resource* resource = nullptr;
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


//
D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {

	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {

	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}





//Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	CoInitializeEx(0, COINIT_MULTITHREADED);


#pragma region ウィンドウの生成
	WNDCLASS wc{};
	wc.lpfnWndProc = WindowProc;
	wc.lpszClassName = L"CG2WindowClass";
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	RegisterClass(&wc);

	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	AdjustWindowRect(&wrc, WS_EX_OVERLAPPEDWINDOW, false);

	HWND hwnd = CreateWindow(
		wc.lpszClassName,
		L"CG2",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr);

	ShowWindow(hwnd, SW_SHOW);



#pragma endregion

#ifdef _DEBUG
	ID3D12Debug1* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		//
		debugController->EnableDebugLayer();
		//
		debugController->SetEnableGPUBasedValidation(TRUE);
	}

#endif // _DEBUG



	//DXGIファクトリーの生成
	IDXGIFactory7* dxgiFactory = nullptr;

	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

	assert(SUCCEEDED(hr));

	//使用するアダプタ用の変数。最初にnullptrを入れておく
	IDXGIAdapter4* useAdapter = nullptr;
	//よい順にアダプタを頼む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
		//アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));//取得できないのは一大事
		//ソフトウェアアダプタでなければ採用！
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			//採用したアダプタの情報をログに出力。wstringの方なので注意
			Log(ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;//ソフトウェアアダプタの場合は見なかったことにする
	}
	//適切なアダプタが見つからなかった
	assert(useAdapter != nullptr);

	ID3D12Device* device = nullptr;
	//機能レベルとログ出力の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};

	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };

	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));

		if (SUCCEEDED(hr)) {
			Log(std::format("FeatureLevel:{}\n", featureLevelStrings[i]));
			break;
		}
	}
	//デバイスの生成がうまくいかなかったので起動できない
	assert(device != nullptr);
	Log("Complete create D3D12device!!!\n");

	//
#ifdef _DEBUG
	ID3D12InfoQueue* infoQueue = nullptr;

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
		infoQueue->Release();
	}


#endif // _DEBUG


	//texture読み込み用配列
	DirectX::ScratchImage mipImages[2] = {};

	ID3D12Resource* textureResource[2] = {};


	//texture読み込み
	//1枚目
	mipImages[0] = LoadTexture("resources/uvChecker.png");
	const DirectX::TexMetadata& metadata1 = mipImages[0].GetMetadata();
	textureResource[0] = CreateTextureResource(device, metadata1);
	UploadTextureData(textureResource[0], mipImages[0]);

	//2枚目
	mipImages[1] = LoadTexture("resources/monsterBall.png");
	const DirectX::TexMetadata& metadata2 = mipImages[1].GetMetadata();
	textureResource[1] = CreateTextureResource(device, metadata2);
	UploadTextureData(textureResource[1], mipImages[1]);




	//コマンドキューを生成する
	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	//コマンドキューの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//コマンドアロケータを生成する
	ID3D12CommandAllocator* commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	//コマンドアロケータの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//コマンドリストを生成する
	ID3D12GraphicsCommandList* commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS
	(&commandList));
	//コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

#pragma region swapchainの設定

	//
	IDXGISwapChain4* swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth;
	swapChainDesc.Height = kClientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain));
	assert(SUCCEEDED(hr));

#pragma endregion

#pragma region descriptorHeapの設定

	//ディスクリプターヒープのサイズの設定
	const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	//rtv
	ID3D12DescriptorHeap* rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	//srv
	ID3D12DescriptorHeap* srvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	//dsv
	ID3D12DescriptorHeap* dsvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

#pragma endregion


	//rtvDesc
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.NumDescriptors = 2;
	hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	assert(SUCCEEDED(hr));

#pragma region srvDescの設定

	//srvDesc配列
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc[2] = {};

	//
	srvDesc[0].Format = metadata1.format;
	srvDesc[0].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc[0].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc[0].Texture2D.MipLevels = UINT(metadata1.mipLevels);

	//
	srvDesc[1].Format = metadata2.format;
	srvDesc[1].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc[1].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc[1].Texture2D.MipLevels = UINT(metadata2.mipLevels);

#pragma endregion

#pragma region textureSrvHandleの設定

	//textureSrvHandle配列
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU[2] = {};
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU[2] = {};

	//1枚目()
	textureSrvHandleCPU[0] = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	textureSrvHandleGPU[0] = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	textureSrvHandleCPU[0].ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureSrvHandleGPU[0].ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	device->CreateShaderResourceView(textureResource[0], &srvDesc[0], textureSrvHandleCPU[0]);

	//2枚目
	textureSrvHandleCPU[1] = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
	textureSrvHandleGPU[1] = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
	
	textureSrvHandleCPU[1].ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureSrvHandleGPU[1].ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	//
	device->CreateShaderResourceView(textureResource[1], &srvDesc[1], textureSrvHandleCPU[1]);

#pragma endregion



	/*D3D12_DESCRIPTOR_HEAP_DESC srvDescriptorHeapDesc{};
	srvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvDescriptorHeapDesc.NumDescriptors = 2;
	hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&srvDescriptorHeap));
	assert(SUCCEEDED(hr));*/

	//
	ID3D12Resource* swapChainResources[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	//bufferの取得
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));

	//renderTargetViewの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	//descriptorの先頭の取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	//
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);
	//
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//
	device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);




	//出力ウィンドウへの文字出力
	Log("Hello,DirectX!\n");

	MSG msg{};

	//初期値0でFenceを作る
	ID3D12Fence* fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	//FenceのSignalを持つためのイベントを作成する
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);

	//dxxCompilerを初期化
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	//現時点でincludeはしないが、includeに対応するための設定を行っておく
	IDxcIncludeHandler* includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));



	//RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


	//
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


	//RootParameter作成、複数設定できるので配列。
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 0;
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1;
	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);


	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);






	ID3D12Resource* wvpResource = CreateBufferResource(device, sizeof(TransformationMatrix));
	//
	TransformationMatrix* transformationMatrixData = nullptr;
	//
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	//
	transformationMatrixData->WVP =MatrixMath::MakeIdentity4x4();
	transformationMatrixData->World= MatrixMath::MakeIdentity4x4();


	//
	ID3D12Resource* depthStencilResource = CreateDepthStencilTextureResource(device, kClientWidth, kClientHeight);


	//
	ID3D12Resource* transformationMatrixResourceSprite = CreateBufferResource(device, sizeof(TransformationMatrix));
	//
	TransformationMatrix* transformationMatrixDataSprite = nullptr;
	//
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
	//
	transformationMatrixDataSprite->WVP = MatrixMath::MakeIdentity4x4();





	//
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	//
	ID3D12RootSignature* rootSignature = nullptr;
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));



	//
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};

	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);




	//
	D3D12_BLEND_DESC blendDesc{};
	//
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;


	//
	D3D12_RASTERIZER_DESC rasterrizerDesc{};
	//
	rasterrizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	//pixelshaderout
	rasterrizerDesc.FillMode = D3D12_FILL_MODE_SOLID;


	//
	IDxcBlob* vertexShaderBlob = CompileShader(L"Object3D.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(vertexShaderBlob != nullptr);

	IDxcBlob* pixelShaderBlob = CompileShader(L"Object3D.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(pixelShaderBlob != nullptr);



	//
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	//
	device->CreateDepthStencilView(depthStencilResource, &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	//
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//
	depthStencilDesc.DepthEnable = true;
	//
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;



	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipeLineStateDesc{};
	graphicsPipeLineStateDesc.pRootSignature = rootSignature;
	graphicsPipeLineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipeLineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),vertexShaderBlob->GetBufferSize() };
	graphicsPipeLineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),pixelShaderBlob->GetBufferSize() };
	graphicsPipeLineStateDesc.BlendState = blendDesc;
	graphicsPipeLineStateDesc.RasterizerState = rasterrizerDesc;

	//
	graphicsPipeLineStateDesc.NumRenderTargets = 1;
	graphicsPipeLineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//
	graphicsPipeLineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//
	graphicsPipeLineStateDesc.SampleDesc.Count = 1;
	graphicsPipeLineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//
	graphicsPipeLineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipeLineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;


	//
	ID3D12PipelineState* graphicPipeLineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipeLineStateDesc, IID_PPV_ARGS(&graphicPipeLineState));
	assert(SUCCEEDED(hr));



#pragma region マテリアルリソースの設定
	ID3D12Resource* materialResource = CreateBufferResource(device, sizeof(MaterialData));
	//
	MaterialData* materialData = nullptr;
	//
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	//
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//
	materialData->enableLighting = true;


#pragma endregion

#pragma region スプライトのマテリアルリソースの設定

	//
	ID3D12Resource* materialResourceSprite = CreateBufferResource(device, sizeof(MaterialData));
	//
	MaterialData* materialDataSprite = nullptr;
	//
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	//
	materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//
	materialDataSprite->enableLighting = false;


#pragma endregion

#pragma region TriangleスプライトのvertexResourceの設定

	//
	ID3D12Resource* vertexResourceTriangle = CreateBufferResource(device, sizeof(VertexData) * 6);

	//
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewTriangle{};
	//
	vertexBufferViewTriangle.BufferLocation = vertexResourceTriangle->GetGPUVirtualAddress();
	//
	vertexBufferViewTriangle.SizeInBytes = sizeof(VertexData) * 6;
	//
	vertexBufferViewTriangle.StrideInBytes = sizeof(VertexData);



	//
	VertexData* vertexDataTriangle = nullptr;
	//
	vertexResourceTriangle->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataTriangle));
	//
	vertexDataTriangle[0].position = { -0.5f,-0.5f,0.0f,1.0f };
	vertexDataTriangle[0].texcoord = { 0.0f,1.0f };
	//
	vertexDataTriangle[1].position = { 0.0f,0.5f,0.0f,1.0f };
	vertexDataTriangle[1].texcoord = { 0.5f,0.0f };
	//
	vertexDataTriangle[2].position = { 0.5f,-0.5f,0.0f,1.0f };
	vertexDataTriangle[2].texcoord = { 1.0f,1.0f };


	//
	vertexDataTriangle[3].position = { -0.5f,-0.5f,0.5f,1.0f };
	vertexDataTriangle[3].texcoord = { 0.0f,1.0f };
	//
	vertexDataTriangle[4].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexDataTriangle[4].texcoord = { 0.5f,0.0f };
	//
	vertexDataTriangle[5].position = { 0.5f,-0.5f,-0.5f,1.0f };
	vertexDataTriangle[5].texcoord = { 1.0f,1.0f };

#pragma endregion

#pragma region スプライトのvertexResouceの設定
	//
	ID3D12Resource* vertexResourceSprite = CreateBufferResource(device, sizeof(VertexData) * 6);
	//
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	//
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	//
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	//
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	//
	VertexData* vertexDataSprite = nullptr;
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	//
	vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f };
	vertexDataSprite[0].texcoord = { 0.0f,1.0f };
	vertexDataSprite[0].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[1].texcoord = { 0.0f,0.0f };
	vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f };
	vertexDataSprite[2].texcoord = { 1.0f,1.0f };
	//
	vertexDataSprite[3].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[3].texcoord = { 0.0f,0.0f };
	vertexDataSprite[4].position = { 640.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[4].texcoord = { 1.0f,0.0f };
	vertexDataSprite[5].position = { 640.0f,360.0f,0.0f,1.0f };
	vertexDataSprite[5].texcoord = { 1.0f,1.0f };

#pragma endregion

#pragma region スフィアの頂点リソースの設定
	uint32_t kSubdivision = 16;

	ID3D12Resource* vertexResourceSphere = CreateBufferResource(device, sizeof(VertexData) * kSubdivision * kSubdivision * 6);
	//
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere{};
	//
	vertexBufferViewSphere.BufferLocation = vertexResourceSphere->GetGPUVirtualAddress();
	//
	vertexBufferViewSphere.SizeInBytes = sizeof(VertexData) * kSubdivision * kSubdivision * 6;
	//
	vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);

	VertexData* vertexDataSphere = nullptr;
	vertexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSphere));


	//経度一分割の角度
	const float kLonEvery = (2 * (static_cast<float>(M_PI))) / kSubdivision;
	//緯度一分割の角度
	const float kLatEvery = (static_cast<float>(M_PI)) / kSubdivision;



	//for文でsphere頂点計算

	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		float lat = -(static_cast<float>(M_PI)) / 2.0f + kLatEvery * latIndex;//緯度

		//経度の方向に分割
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			float u = float(lonIndex) / float(kSubdivision);
			float v = 1.0f - float(latIndex) / float(kSubdivision);

			uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;
			float lon = lonIndex * kLonEvery;//経度

			////頂点a計算
			VertexData vertA = {};
			vertA.position.x = cos(lat) * cos(lon);
			vertA.position.y = sin(lat);
			vertA.position.z = cos(lat) * sin(lon);
			vertA.position.w = 1.0f;
			vertA.texcoord.x = float(lonIndex) / float(kSubdivision);
			vertA.texcoord.y = 1.0f - float(latIndex) / float(kSubdivision);
			vertA.normal.x = vertA.position.x;
			vertA.normal.y = vertA.position.y;
			vertA.normal.z = vertA.position.z;


			VertexData vertB = {};
			vertB.position.x = cos(lat + kLatEvery) * cos(lon);
			vertB.position.y = sin(lat + kLatEvery);
			vertB.position.z = cos(lat + kLatEvery) * sin(lon);
			vertB.position.w = 1.0f;
			vertB.texcoord.x = float(lonIndex) / float(kSubdivision);
			vertB.texcoord.y = 1.0f - float(latIndex + 1) / float(kSubdivision);
			vertB.normal.x = vertB.position.x;
			vertB.normal.y = vertB.position.y;
			vertB.normal.z = vertB.position.z;


			VertexData vertC = {};
			vertC.position.x = cos(lat) * cos(lon + kLonEvery);
			vertC.position.y = sin(lat);
			vertC.position.z = cos(lat) * sin(lon + kLonEvery);
			vertC.position.w = 1.0f;
			vertC.texcoord.x = float(lonIndex + 1) / float(kSubdivision);
			vertC.texcoord.y = 1.0f - float(latIndex) / float(kSubdivision);
			vertC.normal.x = vertC.position.x;
			vertC.normal.y = vertC.position.y;
			vertC.normal.z = vertC.position.z;


			VertexData vertD = {};
			vertD.position.x = cos(lat + kLatEvery) * cos(lon + kLonEvery);
			vertD.position.y = sin(lat + kLatEvery);
			vertD.position.z = cos(lat + kLatEvery) * sin(lon + kLonEvery);
			vertD.position.w = 1.0f;
			vertD.texcoord.x = float(lonIndex + 1) / float(kSubdivision);
			vertD.texcoord.y = 1.0f - float(latIndex + 1) / float(kSubdivision);
			vertD.normal.x = vertD.position.x;
			vertD.normal.y = vertD.position.y;
			vertD.normal.z = vertD.position.z;





			vertexDataSphere[start].position = vertA.position;
			vertexDataSphere[start].texcoord = vertA.texcoord;
			vertexDataSphere[start].normal = vertA.normal;
			vertexDataSphere[start + 1].position = vertB.position;
			vertexDataSphere[start + 1].texcoord = vertB.texcoord;
			vertexDataSphere[start + 1].normal = vertB.normal;
			vertexDataSphere[start + 2].position = vertC.position;
			vertexDataSphere[start + 2].texcoord = vertC.texcoord;
			vertexDataSphere[start + 2].normal = vertC.normal;
			vertexDataSphere[start + 3].position = vertD.position;
			vertexDataSphere[start + 3].texcoord = vertD.texcoord;
			vertexDataSphere[start + 3].normal = vertD.normal;
			vertexDataSphere[start + 4].position = vertC.position;
			vertexDataSphere[start + 4].texcoord = vertC.texcoord;
			vertexDataSphere[start + 4].normal = vertC.normal;
			vertexDataSphere[start + 5].position = vertB.position;
			vertexDataSphere[start + 5].texcoord = vertB.texcoord;
			vertexDataSphere[start + 5].normal = vertB.normal;

		}
	}
#pragma endregion

	ID3D12Resource* directionalLightResource = CreateBufferResource(device, sizeof(DirectionalLight));
	//
	DirectionalLight* directionalLightData = nullptr;
	//
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	//
	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = { 1.0f,0.0f,0.0f };
	directionalLightData->intensity = 1.0f;

#pragma region ビューポートの初期化
	//
	D3D12_VIEWPORT viewport{};
	//
	viewport.Width = kClientWidth;
	viewport.Height = kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

#pragma endregion

#pragma region シザーの初期化
	//
	D3D12_RECT scissorRect{};
	//
	scissorRect.left = 0;
	scissorRect.right = kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;

#pragma endregion

#pragma region ImGuiの初期化

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(device,
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap,
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

#pragma endregion


	Transform transform{
		{1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f}
	};

	//
	Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };


	//
	bool useMonsterBall = true;








	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {

			//p16
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			//			ImGui::ShowDemoWindow();
						/*ImGui::Begin("Window");
						ImGui::DragFloat3("color", &materialData->x, 0.01f);
						ImGui::End();*/
			ImGui::Begin("texture");
			ImGui::Checkbox("useMonsterBall", &useMonsterBall);
			ImGui::SliderAngle("Agnle", &transform.rotate.y);
			ImGui::End();


			//
			ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
			commandList->SetDescriptorHeaps(1, descriptorHeaps);

			//			transform.rotate.y += 0.03f;
			Transform cameraTransform{ {1.0f,1.0f,1.0f} ,{0.0f,0.0f,0.0f},{0.0f,0.0f,-5.0f} };


			Matrix4x4 worldMatrix = MatrixMath::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
			Matrix4x4 cameraMatrix = MatrixMath::MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrix = MatrixMath::Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = MatrixMath::MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 worldViewProjectionMatrix = MatrixMath::Multiply(worldMatrix, MatrixMath::Multiply(viewMatrix, projectionMatrix));
			transformationMatrixData->WVP = worldViewProjectionMatrix;
			transformationMatrixData->World = worldMatrix;

			//
			Matrix4x4 worldMatrixSprite = MatrixMath::MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
			Matrix4x4 viewMatrixSprite = MatrixMath::MakeIdentity4x4();
			Matrix4x4 projectionMatrixSprite = MatrixMath::MakeOrthographicMatrix(0.0f, 0.0f, float(kClientWidth), float(kClientHeight), 0.0f, 100.0f);
			Matrix4x4 worldViewProjectionMatrixSprite = MatrixMath::Multiply(worldMatrixSprite, MatrixMath::Multiply(viewMatrixSprite, projectionMatrixSprite));
			transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;
			transformationMatrixDataSprite->World = worldMatrixSprite;


			//
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();


			//描画。処理しない。

			ImGui::Render();

			//
			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
			//
			D3D12_RESOURCE_BARRIER barrier{};
			//
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			//
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			//
			barrier.Transition.pResource = swapChainResources[backBufferIndex];
			//
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			//
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			//
			commandList->ResourceBarrier(1, &barrier);



			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);

			////
			//commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
			//
			float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
			//
			commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);



			//ビューポートの設定
			commandList->RSSetViewports(1, &viewport);//
			commandList->RSSetScissorRects(1, &scissorRect);//
			//
			commandList->SetGraphicsRootSignature(rootSignature);
			commandList->SetPipelineState(graphicPipeLineState);
			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
			//
			commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			//マテリアルリソースの設定
			commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

			//wvpリソースの設定
			commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());



			commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU[1]);


			//
			commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());


			//
			commandList->DrawInstanced(6, 1, 0, 0);


#pragma region 球体の描画

			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSphere);

			//
			commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

			//
			commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU[1] : textureSrvHandleGPU[0]);

			//
			commandList->DrawInstanced((kSubdivision * kSubdivision * 6), 1, 0, 0);
#pragma endregion
			////
			//commandList->SetGraphicsRootConstantBufferView(1, materialResource->GetGPUVirtualAddress());

#pragma region スプライト描画
			//vbvの設定
			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
			//wvpの設定
			commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
			//srvのディスクリプターテーブルの設定
			commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU[0]);

			//描画
			commandList->DrawInstanced(6, 1, 0, 0);
#pragma endregion

#pragma region トランジションバリアの設定
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;

			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			//
			commandList->ResourceBarrier(1, &barrier);
#pragma endregion




			//コマンドリストの内容を確定
			hr = commandList->Close();
			assert(SUCCEEDED(hr));

			//コマンドリストの実行
			ID3D12CommandList* commandLists[] = { commandList };
			commandQueue->ExecuteCommandLists(1, commandLists);
			//
			swapChain->Present(1, 0);

			//
			fenceValue++;
			//
			commandQueue->Signal(fence, fenceValue);
			//
			//
			if (fence->GetCompletedValue() < fenceValue) {
				//
				fence->SetEventOnCompletion(fenceValue, fenceEvent);
				//
				WaitForSingleObject(fenceEvent, INFINITE);
			}

			//
			hr = commandAllocator->Reset();
			assert(SUCCEEDED(hr));
			hr = commandList->Reset(commandAllocator, nullptr);
			assert(SUCCEEDED(hr));






		}
	}



	//出力ウィンドウへの文字出力
	OutputDebugStringA("Hello,DirectX!\n");

	//

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	CloseHandle(fenceEvent);

	//
	wvpResource->Release();
	materialResource->Release();
	vertexResourceTriangle->Release();
	graphicPipeLineState->Release();
	signatureBlob->Release();
	if (errorBlob) {
		errorBlob->Release();
	}
	rootSignature->Release();
	pixelShaderBlob->Release();
	vertexShaderBlob->Release();



	fence->Release();

	rtvDescriptorHeap->Release();
	srvDescriptorHeap->Release();
	swapChainResources[0]->Release();
	swapChainResources[1]->Release();
	swapChain->Release();

	commandList->Release();
	commandAllocator->Release();
	commandQueue->Release();
	useAdapter->Release();
	dxgiFactory->Release();

	dxcUtils->Release();
	dxcCompiler->Release();
	includeHandler->Release();
	device->Release();


#ifdef _DEBUG
	debugController->Release();
#endif
	CloseWindow(hwnd);









	//
	IDXGIDebug1* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}


	CoUninitialize();


	return 0;



};


