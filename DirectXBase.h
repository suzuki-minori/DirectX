#pragma once

#include<cassert>
#include<Windows.h>
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
#include"Matrix3x3.h"
#define _USE_MATH_DEFINES
#include<cmath>
#include<math.h>
#include<fstream>
#include<sstream>
#include<wrl.h>
#include <array>
#include<chrono>

#include "Input.h"
#include"WinApp.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")

using namespace Microsoft::WRL;

class WinApp;

class DirectXBase
{
public:
	DirectXBase() {
	
	};
	//
	void Initialize(WinApp*winApp);

	Microsoft::WRL::ComPtr < ID3D12Resource>  CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr < ID3D12Device> device, int32_t width, int32_t height);


	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);

	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);




public:

	//
	Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	//Microsoft::WRL::ComPtr < ID3D12Device> GetDevice()const { return device; }

	ID3D12GraphicsCommandList* GetCommandList()const { return commandList.Get(); }

	Microsoft::WRL::ComPtr < ID3D12Fence> GetFence()const { return fence; }

	uint64_t GetFenceValue()const { return fenceValue; }

	Microsoft::WRL::ComPtr < ID3D12CommandQueue> GetCommandQueue()const{return commandQueue;}

	Microsoft::WRL::ComPtr < ID3D12CommandAllocator> GetCommandAllocator()const { return commandAllocator; }

	Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> GetRtvDescriptorHeap()const {return rtvDescriptorHeap;}
	//srv															   
	Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> GetSrvDescriptorHeap()const {return srvDescriptorHeap;}
	//dsv															  
	Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> GetDsvDescriptorHeap()const {return dsvDescriptorHeap;}

	uint32_t GetDescriptorSizeSRV()const { return descriptorSizeSRV; }

	D3D12_RECT GetScissorRect()const { return scissorRect; }

	Microsoft::WRL::ComPtr < IDxcUtils> GetDxcUtils()const{ return dxcUtils; }
	Microsoft::WRL::ComPtr < IDxcCompiler3> GetDxcCompiler()const { return dxcCompiler; }
	Microsoft::WRL::ComPtr < IDxcIncludeHandler> GetIncludeHandler()const { return includeHandler; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandles()const { return rtvHandles[2]; }

	ID3D12Device* GetDevice()const { return device.Get(); }
	ID3D12GraphicsCommandList* GetCommandList()const { return commandList.Get(); }

	//DirectXBase* dxBase_ = nullptr;

private:
	//
	Microsoft::WRL::ComPtr<ID3D12Device>device = nullptr;
	//
	Microsoft::WRL::ComPtr<IDXGIFactory7>dxgiFactory = nullptr;
	//
	Microsoft::WRL::ComPtr < ID3D12CommandQueue> commandQueue = nullptr;
	//
	Microsoft::WRL::ComPtr < ID3D12CommandAllocator> commandAllocator = nullptr;
	//
	Microsoft::WRL::ComPtr < ID3D12GraphicsCommandList> commandList = nullptr;
	//
	Microsoft::WRL::ComPtr < IDXGISwapChain4> swapChain = nullptr;
	//
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//
	Microsoft::WRL::ComPtr < ID3D12Resource> depthStencilResource=nullptr;
	//
	D3D12_RESOURCE_DESC resourceDesc{};
	//
	D3D12_HEAP_PROPERTIES heapProperties{};
	//
	D3D12_CLEAR_VALUE depthClearValue{};
	//
	WinApp* winApp = nullptr;

	uint32_t descriptorSizeDSV;
	uint32_t descriptorSizeSRV;
	uint32_t descriptorSizeRTV;

	//rtv
	Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> rtvDescriptorHeap=nullptr;
	//srv
	Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> srvDescriptorHeap=nullptr;
	//dsv
	Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> dsvDescriptorHeap=nullptr;
	
	//
	Microsoft::WRL::ComPtr < ID3D12Resource> swapChainResources[2] = { nullptr };

	//
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	//
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle;

	Microsoft::WRL::ComPtr < ID3D12Fence> fence = nullptr;
	
	static const uint64_t fenceValue = 0;

	//
	D3D12_VIEWPORT viewport{};

	//
	D3D12_RECT scissorRect{};


	Microsoft::WRL::ComPtr < IDxcUtils> dxcUtils = nullptr;
	Microsoft::WRL::ComPtr < IDxcCompiler3> dxcCompiler = nullptr;
	Microsoft::WRL::ComPtr < IDxcIncludeHandler> includeHandler = nullptr;

	//
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2>swapChainResource;

	//
	Microsoft::WRL::ComPtr < IDxcBlob> CompileShader(

		//CompilerするShaderファイルへのパス
		const std::wstring& filePath,

		//Compilerに使用するProfile
		const wchar_t* profile);
	
	Microsoft::WRL::ComPtr < ID3D12Resource> CreateBufferResource(size_t sizeInBytes);


	Microsoft::WRL::ComPtr < ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);


	void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

	
	static DirectX::ScratchImage LoadTexture(const std::string& filePath);


	std::chrono::steady_clock::time_point reference_;


private:
	//Initializeで呼び出す関数
	void DeviceInitialize();

	void CommandInitialize();

	void SwapChainCreate();

	void DepthStencilCreate();
	
	void DepthStencilViewInitialize();

	void FenceInitialize();

	void ViewPortInitialize();

	void ScissorInitialize();

	void DxcCompiler();

	void ImGuiInitialize(WinApp*winApi, DXGI_SWAP_CHAIN_DESC1 swapChainDesc, D3D12_RENDER_TARGET_VIEW_DESC rtvDesc);
	
	void CreateDescriptorHeap();

	void RenderTargetViewInitialize();
	

	//
	void PreDraw();
	//
	void PostDraw();


	//
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr < ID3D12DescriptorHeap>descriptorHeap, uint32_t descriptorSize, uint32_t index);

	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);


	void InitializeFixFPS();

	void UpdateFixFPS();


};

