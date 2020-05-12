//***************************************************************************************
// CrateApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "Common/d3dApp.h"
#include "Common/MathHelper.h"
#include "Common/UploadBuffer.h"
#include "Common/GeometryGenerator.h"
#include "FrameResource.h"
#include "PerlinNoise.h"
#include "Camera.h"
#include <stdlib.h>  
#include <time.h>  
#include <stdio.h>

using namespace std; 

//#include "<stdio.h>"


using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;
bool wired = false;
//handles camera state tracking
bool cam1 = false;
bool cam3 = true;
bool camfree = false;

//Sets up character position variables
float charX = 50;
float charY = 2;
float charZ = 50;
float XSpeed = 0;
float YSpeed = 0; 
float ZSpeed = 0;
float charRotation = 0;

//determines size of the world generated, World height values must always = world size
int Worldsize = 100;
float WorldHeight[100][100];

//sets up a 0,0,0 vector for reference and the character position vector
XMVECTOR V0 = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
XMVECTOR CharPos = XMVectorSet(charX, charY, charZ, 1.0f);
XMVECTOR CharDir; 

//XMVECTOR *outScale = &V0;
//XMVECTOR *outRotQuat = &V0;
//XMVECTOR *outTrans = &V0; 



// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;
	

	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 CharLocal = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Count
};

class CrateApp : public D3DApp
{
public:
	CrateApp(HINSTANCE hInstance);
	CrateApp(const CrateApp& rhs) = delete;
	CrateApp& operator=(const CrateApp& rhs) = delete;
	~CrateApp();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);


	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildGrassGeo(); //builds a quad shape
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems(); // builds the world
	double RandomNum(int range_min, int range_max, int n); //random range number generator taken from MSDN 
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	void UpdateWireframe(bool wire);

	
	void BuildSkyGeometry();

	void UpdateChar(float x, float y, float z, float Xs, float Ys, float Zs, float angle); //draws and updates the character


	
	
	
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();


private:

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	//Render items divided by PSO, So different layers 
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];



	// Render items divided by PSO.
	std::vector<RenderItem*> mOpaqueRitems;

	PassConstants mMainPassCB;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.3f*XM_PI;
	float mPhi = 0.4f*XM_PI;
	float mRadius = 2.5f;

	Camera freeCam;
	POINT mLastMousePos;
	
};



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		CrateApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

CrateApp::CrateApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

CrateApp::~CrateApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool CrateApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);



	freeCam.SetPosition(charX, charY, (charZ-5)); //moves the camera to the character at the start 

	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildSkyGeometry();
	BuildGrassGeo();
	BuildMaterials();
	UpdateChar(charX, charY, charZ, XSpeed, YSpeed, ZSpeed, charRotation);
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();
	

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void CrateApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	//XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	//XMStoreFloat4x4(&mProj, P);

	freeCam.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void CrateApp::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);
	SetCapture(mhMainWnd);
	

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
	UpdateChar(charX, charY, charZ, XSpeed, YSpeed, ZSpeed, charRotation);

	
	
	
	
}

void CrateApp::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	//mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	//mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	mCommandList->SetPipelineState(mPSOs["opaque"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);


	// Enable the alpha tested PSO for the chain cube 
	mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);

	// Enable the Transparent PSO
	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);


	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void CrateApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void CrateApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CrateApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0 )
	{
		// Make each pixel correspond to a quarter of a degree.
		
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));


		float dy = XMConvertToRadians(0.5f*static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		//mTheta -= dx;
		// mPhi -= dy;

		
			//freeCam.Pitch(90);
		
		
			//freeCam.Pitch(-90);
		
		
		if (camfree || cam1)
		{
			freeCam.Pitch(dy);
			freeCam.RotateY(dx);
		}

		//charRotation += dx; 
		
	

		// Restrict the angle mPhi.
		// mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	//else if((btnState & MK_RBUTTON) != 0)
	//{
	//    // Make each pixel correspond to 0.2 unit in the scene.
	//    float dx = 0.05f*static_cast<float>(x - mLastMousePos.x);
	//    float dy = 0.05f*static_cast<float>(y - mLastMousePos.y);

	//    // Update the camera radius based on input.
	//    mRadius += dx - dy;

	//    // Restrict the radius.
	//    mRadius = MathHelper::Clamp(mRadius, 1.0f, 1000.0f);
	//}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void CrateApp::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	//gets all the keyboard input

	if (GetAsyncKeyState('W') & 0x8000)
	{
		if (camfree)
		{
			freeCam.Walk(10.0f*dt);
		}
		else if (cam1)
		{
			freeCam.WalkLocked(10.0f*dt);
		}
	}

	//Moves the free cam
	if (GetAsyncKeyState('S') & 0x8000)
		freeCam.Walk(-10.0f*dt);

	if (GetAsyncKeyState('A') & 0x8000)
		freeCam.Strafe(-10.0f*dt);

	if (GetAsyncKeyState('D') & 0x8000)
		freeCam.Strafe(10.0f*dt);

	if (GetAsyncKeyState('F') & 0x8000)
	{
		wired = true;
		
	}

	if (GetAsyncKeyState('E') & 0x8000)
	{	
		freeCam.FlyUp(10.0f*dt);
	}
	if (GetAsyncKeyState('Q') & 0x8000)
	{
		freeCam.FlyUp(-10.0f*dt);
	}

	if (GetAsyncKeyState('G') & 0x8000)
	{
		wired = false;

	}

	//Moves the 3rd person camera
	if (GetAsyncKeyState(VK_UP) )
	{
		charZ += 10.0f*dt;
		//charX = charZ * charRotation; 

	}
	if (GetAsyncKeyState(VK_DOWN))
	{
		charZ += -10.0f*dt;

	}

	if (GetAsyncKeyState(VK_LEFT))
	{
		charRotation += -2 * dt;
		charX += -10.0f*dt;

	}

	if (GetAsyncKeyState(VK_RIGHT))
	{
		charRotation += 2 * dt;
		charX += 10.0f*dt;

	}

	//changes camera states
	if (GetAsyncKeyState('1'))
	{
		cam1 = true;
		cam3 = false;
		camfree = false;
	}
	if (GetAsyncKeyState('2'))
	{
		cam1 = false;
		cam3 = true;
		camfree = false;
	}
	if (GetAsyncKeyState('3'))
	{
		cam1 = false;
		cam3 = false;
		camfree = true;
	}

	
	//updates the camera matrix after moving it
	freeCam.UpdateViewMatrix();
}




void CrateApp::AnimateMaterials(const GameTimer& gt)
{
	//unused
}

void CrateApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void CrateApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void CrateApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = freeCam.GetView();
	XMMATRIX proj = freeCam.GetProj();
	//XMMATRIX view = XMLoadFloat4x4(&mView);
	//XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = freeCam.GetPosition3f();
	//mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	
	float pulse = sin(gt.TotalTime() / 10); //creates a pulse effect for the lights 
	pulse = modff(pulse, &pulse);

	//the main lighting used in the world 
	mMainPassCB.AmbientLight = { 0.7f, 0.7f, 1.0f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { pulse, pulse, pulse };  //pulse is used in the strengths to create a varied intensity of the light which simulates a day/night cycle 
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.33f, 0.33f, 0.33f };
	mMainPassCB.Lights[2].Direction = { 0.57735f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { pulse, pulse, pulse };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);

}


void CrateApp::LoadTextures() //loads in all of the textures used for the world
{
	//Load grass texture
	auto grassMatTex = std::make_unique<Texture>();
	grassMatTex->Name = "grassMatTex";
	grassMatTex->Filename = L"Textures/minecraft_grass3.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), grassMatTex->Filename.c_str(),
		grassMatTex->Resource, grassMatTex->UploadHeap));

	mTextures[grassMatTex->Name] = std::move(grassMatTex);

	//Load dirt texture
	auto dirtMatTex = std::make_unique<Texture>();
	dirtMatTex->Name = "dirtMatTex";
	dirtMatTex->Filename = L"Textures/minecraft_dirt.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), dirtMatTex->Filename.c_str(),
		dirtMatTex->Resource, dirtMatTex->UploadHeap));

	mTextures[dirtMatTex->Name] = std::move(dirtMatTex);

	//Load stone texture
	auto stoneMatTex = std::make_unique<Texture>();
	stoneMatTex->Name = "stoneMatTex";
	stoneMatTex->Filename = L"Textures/minecraft_stone.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), stoneMatTex->Filename.c_str(),
		stoneMatTex->Resource, stoneMatTex->UploadHeap));

	mTextures[stoneMatTex->Name] = std::move(stoneMatTex);

	//Load bedrock texture
	auto bedrockMatTex = std::make_unique<Texture>();
	bedrockMatTex->Name = "bedrockMatTex";
	bedrockMatTex->Filename = L"Textures/minecraft_bedrock2.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), bedrockMatTex->Filename.c_str(),
		bedrockMatTex->Resource, bedrockMatTex->UploadHeap));

	mTextures[bedrockMatTex->Name] = std::move(bedrockMatTex);

	//Load water texture
	auto waterMatTex = std::make_unique<Texture>();
	waterMatTex->Name = "waterMatTex";
	waterMatTex->Filename = L"Textures/minecraft_water2.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), waterMatTex->Filename.c_str(),
		waterMatTex->Resource, waterMatTex->UploadHeap));

	mTextures[waterMatTex->Name] = std::move(waterMatTex);

	//load coal texture
	auto coalMatTex = std::make_unique<Texture>();
	coalMatTex->Name = "coalMatTex";
	coalMatTex->Filename = L"Textures/minecraft_coal.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), coalMatTex->Filename.c_str(),
		coalMatTex->Resource, coalMatTex->UploadHeap));

	mTextures[coalMatTex->Name] = std::move(coalMatTex);

	//load iron texture
	auto ironMatTex = std::make_unique<Texture>();
	ironMatTex->Name = "ironMatTex";
	ironMatTex->Filename = L"Textures/minecraft_iron.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), ironMatTex->Filename.c_str(),
		ironMatTex->Resource, ironMatTex->UploadHeap));

	mTextures[ironMatTex->Name] = std::move(ironMatTex);

	//load diamond texture
	auto diamondMatTex = std::make_unique<Texture>();
	diamondMatTex->Name = "diamondMatTex";
	diamondMatTex->Filename = L"Textures/minecraft_diamond.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), diamondMatTex->Filename.c_str(),
		diamondMatTex->Resource, diamondMatTex->UploadHeap));

	mTextures[diamondMatTex->Name] = std::move(diamondMatTex);

	//load redstone texture
	auto redsMatTex = std::make_unique<Texture>();
	redsMatTex->Name = "redsMatTex";
	redsMatTex->Filename = L"Textures/minecraft_redstone.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), redsMatTex->Filename.c_str(),
		redsMatTex->Resource, redsMatTex->UploadHeap));

	mTextures[redsMatTex->Name] = std::move(redsMatTex);

	//load sand texture
	auto sandMatTex = std::make_unique<Texture>();
	sandMatTex->Name = "sandMatTex";
	sandMatTex->Filename = L"Textures/minecraft_sand.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), sandMatTex->Filename.c_str(),
		sandMatTex->Resource, sandMatTex->UploadHeap));

	mTextures[sandMatTex->Name] = std::move(sandMatTex);

	//load long grass texture
	auto longGrassMatTex = std::make_unique<Texture>();
	longGrassMatTex->Name = "longGrassMatTex";
	longGrassMatTex->Filename = L"Textures/MineCraft_Tall_Grass.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), longGrassMatTex->Filename.c_str(),
		longGrassMatTex->Resource, longGrassMatTex->UploadHeap));

	mTextures[longGrassMatTex->Name] = std::move(longGrassMatTex);

	//load wood texture
	auto woodMatTex = std::make_unique<Texture>();
	woodMatTex->Name = "woodMatTex";
	woodMatTex->Filename = L"Textures/minecraft_tree_wood.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), woodMatTex->Filename.c_str(),
		woodMatTex->Resource, woodMatTex->UploadHeap));

	mTextures[woodMatTex->Name] = std::move(woodMatTex);

	//load leaves texture
	auto leafMatTex = std::make_unique<Texture>();
	leafMatTex->Name = "leafMatTex";
	leafMatTex->Filename = L"Textures/minecraft_tree_leaves.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), leafMatTex->Filename.c_str(),
		leafMatTex->Resource, leafMatTex->UploadHeap));

	mTextures[leafMatTex->Name] = std::move(leafMatTex);

	//load yellow flower texture
	auto flowerYMatTex = std::make_unique<Texture>();
	flowerYMatTex->Name = "flowerYMatTex";
	flowerYMatTex->Filename = L"Textures/minecraft_flower_yellow.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), flowerYMatTex->Filename.c_str(),
		flowerYMatTex->Resource, flowerYMatTex->UploadHeap));

	mTextures[flowerYMatTex->Name] = std::move(flowerYMatTex);

	//load red flower texture
	auto flowerRMatTex = std::make_unique<Texture>();
	flowerRMatTex->Name = "flowerRMatTex";
	flowerRMatTex->Filename = L"Textures/minecraft_flower_red.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), flowerRMatTex->Filename.c_str(),
		flowerRMatTex->Resource, flowerRMatTex->UploadHeap));

	mTextures[flowerRMatTex->Name] = std::move(flowerRMatTex);

	//load sugarcane texture
	auto sugarMatTex = std::make_unique<Texture>();
	sugarMatTex->Name = "sugarMatTex";
	sugarMatTex->Filename = L"Textures/minecraft_sugar.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), sugarMatTex->Filename.c_str(),
		sugarMatTex->Resource, sugarMatTex->UploadHeap));

	mTextures[sugarMatTex->Name] = std::move(sugarMatTex);

	auto skyTex = std::make_unique<Texture>();
	skyTex->Name = "skyTex";
	skyTex->Filename = L"Textures/plainSky.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), skyTex->Filename.c_str(),
		skyTex->Resource, skyTex->UploadHeap));

	mTextures[skyTex->Name] = std::move(skyTex);
}





void CrateApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void CrateApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 17;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	//Adds the grass texture to the descriptor heap
	auto grassMatTex = mTextures["grassMatTex"]->Resource;


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = grassMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = grassMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;


	md3dDevice->CreateShaderResourceView(grassMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	
	//Adds the dirt texture to the descriptor heap
	auto dirtMatTex = mTextures["dirtMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = dirtMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = dirtMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(dirtMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//Adds the stone texture to the descriptor heap
	auto stoneMatTex = mTextures["stoneMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = stoneMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = stoneMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(stoneMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//Adds the bedrock texture to the descriptor heap
	auto bedrockMatTex = mTextures["bedrockMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = bedrockMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = bedrockMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(bedrockMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//Adds the water texture to the descriptor heap
	auto waterMatTex = mTextures["waterMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = waterMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = waterMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(waterMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//Adds the coal texture to the descriptor heap
	auto coalMatTex = mTextures["coalMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = coalMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = coalMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(coalMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//Adds the iron texture to the descriptor heap
	auto ironMatTex = mTextures["ironMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = ironMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = ironMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(ironMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//Adds the diamond texture to the descriptor heap
	auto diamondMatTex = mTextures["diamondMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = diamondMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = diamondMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(diamondMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//Adds the redstone texture to the descriptor heap
	auto redsMatTex = mTextures["redsMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = redsMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = redsMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(redsMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//Adds the sand texture to the descriptor heap
	auto sandMatTex = mTextures["sandMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = sandMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = sandMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(sandMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//Adds the long grass texture to the descriptor heap
	auto longGrassMatTex = mTextures["longGrassMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = longGrassMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = longGrassMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(longGrassMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	
	//Adds the Wood texture to the descriptor heap
	auto woodMatTex = mTextures["woodMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = woodMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = woodMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(woodMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//Adds the leaves texture to the descriptor heap
	auto leafMatTex = mTextures["leafMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = leafMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = leafMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(leafMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//Adds the yellow flower texture to the descriptor heap
	auto flowerYMatTex = mTextures["flowerYMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = flowerYMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = flowerYMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(flowerYMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//Adds the red flower texture to the descriptor heap
	auto flowerRMatTex = mTextures["flowerRMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = flowerRMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = flowerRMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(flowerRMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//Adds the sugar cane texture to the descriptor heap
	auto sugarMatTex = mTextures["sugarMatTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = sugarMatTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = sugarMatTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(sugarMatTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	//Adds the skybox texture to the descriptor heap
	auto skyTex = mTextures["skyTex"]->Resource;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = skyTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = skyTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(skyTex.Get(), &srvDesc, hDescriptor);

}




void CrateApp::BuildShadersAndInputLayout()
{

	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	//loads in shaders in from file and stores in the local shader list
	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_1");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void CrateApp::BuildShapeGeometry() //builds the geometry of a box that can be used for a render item
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBoxGrass(1.0f, 1.0f, 1.0f, 0); //Set this to zero, no need to tessilate cubes. it also improved performance 

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = 0;
	boxSubmesh.BaseVertexLocation = 0;


	std::vector<Vertex> vertices(box.Vertices.size());

	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		vertices[i].Pos = box.Vertices[i].Position;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexC = box.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices = box.GetIndices16();

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;

	mGeometries[geo->Name] = std::move(geo);

}

void CrateApp::BuildGrassGeo()  //builds the geometry of a quad that can be used for a render item such as grass or flowers etc.
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData quad = geoGen.CreateQuad(1.0f, 1.0f, 1.0f, 1, 0.0f); //Set this to zero, no need to tessilate cubes. it also improved performance 

	std::vector<Vertex> vertices(quad.Vertices.size());
	for (size_t i = 0; i < quad.Vertices.size(); ++i)
	{
		auto& p = quad.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = quad.Vertices[i].Normal;
		vertices[i].TexC = quad.Vertices[i].TexC;
	}


	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = quad.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "quadGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["quad"] = submesh;

	mGeometries["quadGeo"] = std::move(geo);
}

void CrateApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	if (!wired)
	{
		opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_BACK,
			FALSE,
			D3D12_DEFAULT_DEPTH_BIAS,
			D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
			D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
			FALSE,
			FALSE,
			FALSE,
			0,
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
	}
	else if (wired)
	{
		opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(
			D3D12_FILL_MODE_WIREFRAME,
			D3D12_CULL_MODE_BACK,
			FALSE,
			D3D12_DEFAULT_DEPTH_BIAS,
			D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
			D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
			TRUE,
			FALSE,
			FALSE,
			0,
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
	}
	//Changed this from D3D12_DEFAULT to play about with it while investigating performance issues


	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));


	

	//PSO for transparent objects. 
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;

	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));


	//PSO for alpha tested objects 
	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
		mShaders["alphaTestedPS"]->GetBufferSize()
	};

	//Disable back face culling for Alpha tested objects 
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));



}


void CrateApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
	}
}

void CrateApp::BuildMaterials()
{
	//material for grass
	auto grassMat = std::make_unique<Material>();
	grassMat->Name = "grassMat";
	grassMat->MatCBIndex = 0;
	grassMat->DiffuseSrvHeapIndex = 0;
	grassMat->DiffuseAlbedo = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	grassMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	grassMat->Roughness = 0.2f;

	//material for dirt
	auto dirtMat = std::make_unique<Material>();
	dirtMat->Name = "dirtMat";
	dirtMat->MatCBIndex = 1;
	dirtMat->DiffuseSrvHeapIndex = 1;
	dirtMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	dirtMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	dirtMat->Roughness = 0.2f;

	//material for stone
	auto stoneMat = std::make_unique<Material>();
	stoneMat->Name = "stoneMat";
	stoneMat->MatCBIndex = 2;
	stoneMat->DiffuseSrvHeapIndex = 2;
	stoneMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	stoneMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	stoneMat->Roughness = 0.2f;

	//material for bedrock
	auto bedrockMat = std::make_unique<Material>();
	bedrockMat->Name = "bedrockMat";
	bedrockMat->MatCBIndex = 3;
	bedrockMat->DiffuseSrvHeapIndex = 3;
	bedrockMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bedrockMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	bedrockMat->Roughness = 0.2f;

	//material for water
	auto waterMat = std::make_unique<Material>();
	waterMat->Name = "waterMat";
	waterMat->MatCBIndex = 4;
	waterMat->DiffuseSrvHeapIndex = 4;
	waterMat->DiffuseAlbedo = XMFLOAT4(0.5f, 0.5f, 1.0f, 1.0f);
	waterMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	waterMat->Roughness = 0.2f;

	//material for Coal
	auto coalMat = std::make_unique<Material>();
	coalMat->Name = "coalMat";
	coalMat->MatCBIndex = 5;
	coalMat->DiffuseSrvHeapIndex = 5;
	coalMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	coalMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	coalMat->Roughness = 0.2f;

	//material for iron
	auto ironMat = std::make_unique<Material>();
	ironMat->Name = "ironMat";
	ironMat->MatCBIndex = 6;
	ironMat->DiffuseSrvHeapIndex = 6;
	ironMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	ironMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	ironMat->Roughness = 0.2f;

	//material for diamond
	auto diamondMat = std::make_unique<Material>();
	diamondMat->Name = "diamondMat";
	diamondMat->MatCBIndex = 7;
	diamondMat->DiffuseSrvHeapIndex = 7;
	diamondMat->DiffuseAlbedo = XMFLOAT4(0.85f, 0.85f, 0.85f, 1.0f);
	diamondMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	diamondMat->Roughness = 0.2f;

	//material for redstone
	auto redsMat = std::make_unique<Material>();
	redsMat->Name = "redsMat";
	redsMat->MatCBIndex = 8;
	redsMat->DiffuseSrvHeapIndex = 8;
	redsMat->DiffuseAlbedo = XMFLOAT4(0.85f, 0.85f, 0.85f, 1.0f);
	redsMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	redsMat->Roughness = 0.2f;

	//material for sand
	auto sandMat = std::make_unique<Material>();
	sandMat->Name = "sandMat";
	sandMat->MatCBIndex = 9;
	sandMat->DiffuseSrvHeapIndex = 9;
	sandMat->DiffuseAlbedo = XMFLOAT4(0.85f, 0.85f, 0.85f, 1.0f);
	sandMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	sandMat->Roughness = 0.2f;

	//material for long grass
	auto longGrassMat = std::make_unique<Material>();
	longGrassMat->Name = "longGrassMat";
	longGrassMat->MatCBIndex = 10;
	longGrassMat->DiffuseSrvHeapIndex = 10;
	longGrassMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	longGrassMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	longGrassMat->Roughness = 0.2f;

	//material for wood
	auto woodMat = std::make_unique<Material>();
	woodMat->Name = "woodMat";
	woodMat->MatCBIndex = 11;
	woodMat->DiffuseSrvHeapIndex = 11;
	woodMat->DiffuseAlbedo = XMFLOAT4(0.9f, 1.0f, 0.75f, 1.0f);
	woodMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	woodMat->Roughness = 0.2f;

	//material for leaves 
	auto leafMat = std::make_unique<Material>();
	leafMat->Name = "leafMat";
	leafMat->MatCBIndex = 12;
	leafMat->DiffuseSrvHeapIndex = 12;
	leafMat->DiffuseAlbedo = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	leafMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	leafMat->Roughness = 0.2f;

	//material for yellow flowers 
	auto flowerYMat = std::make_unique<Material>();
	flowerYMat->Name = "flowerYMat";
	flowerYMat->MatCBIndex = 13;
	flowerYMat->DiffuseSrvHeapIndex = 13;
	flowerYMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 0.5f, 1.0f);
	flowerYMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	flowerYMat->Roughness = 0.2f;

	//material for red flowers 
	auto flowerRMat = std::make_unique<Material>();
	flowerRMat->Name = "flowerRMat";
	flowerRMat->MatCBIndex = 14;
	flowerRMat->DiffuseSrvHeapIndex = 14;
	flowerRMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 0.5f, 1.0f);
	flowerRMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	flowerRMat->Roughness = 0.2f;

	//material for sugar canes 
	auto sugarMat = std::make_unique<Material>();
	sugarMat->Name = "sugarMat";
	sugarMat->MatCBIndex = 15;
	sugarMat->DiffuseSrvHeapIndex = 15;
	sugarMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 0.5f, 1.0f);
	sugarMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	sugarMat->Roughness = 0.2f;

	auto skyMat = std::make_unique<Material>();
	skyMat->Name = "skyMat";
	skyMat->MatCBIndex = 16;
	skyMat->DiffuseSrvHeapIndex = 16;
	skyMat->DiffuseAlbedo = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	skyMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	skyMat->Roughness = 0.2f;

	mMaterials["grassMat"] = std::move(grassMat);
	mMaterials["dirtMat"] = std::move(dirtMat);
	mMaterials["stoneMat"] = std::move(stoneMat);
	mMaterials["bedrockMat"] = std::move(bedrockMat);
	mMaterials["waterMat"] = std::move(waterMat);
	mMaterials["coalMat"] = std::move(coalMat);
	mMaterials["ironMat"] = std::move(ironMat);
	mMaterials["diamondMat"] = std::move(diamondMat);
	mMaterials["redsMat"] = std::move(redsMat);
	mMaterials["sandMat"] = std::move(sandMat);
	mMaterials["longGrassMat"] = std::move(longGrassMat);
	mMaterials["woodMat"] = std::move(woodMat);
	mMaterials["leafMat"] = std::move(leafMat);
	mMaterials["flowerYMat"] = std::move(flowerYMat);
	mMaterials["flowerRMat"] = std::move(flowerRMat);
	mMaterials["sugarMat"] = std::move(sugarMat);
	mMaterials["skyMat"] = std::move(skyMat);

}


void CrateApp::BuildRenderItems()
{
	int i = 1; //object index value


	//DRAW SKY BOX//
	auto skyRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skyRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f)*XMMatrixTranslation(50.0,0.0,50.0));
	skyRitem->ObjCBIndex = i;
	i++;
	skyRitem->Mat = mMaterials["skyMat"].get();
	skyRitem->Geo = mGeometries["skyBoxGeo"].get();
	skyRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skyRitem->IndexCount = skyRitem->Geo->DrawArgs["skyBox"].IndexCount;
	skyRitem->StartIndexLocation = skyRitem->Geo->DrawArgs["skyBox"].StartIndexLocation;
	skyRitem->BaseVertexLocation = skyRitem->Geo->DrawArgs["skyBox"].BaseVertexLocation;
	mAllRitems.push_back(std::move(skyRitem));


	srand((size_t)time(NULL)); //reinitializes the random seed using the current time
	PerlinNoise p; //initializes an object of perlinNoise
	double persistence = 0.0; 
	double frequency = 0.15; 
	
	double octaves = 1.0;
	double amplitude = RandomNum(10, 20, 1); //randomises the amplitude between 10 and 20
	double randomseed = RandomNum(1, 100, 1); //randomises the seed that is used by the perlin noise object

	p.Set(persistence, frequency, amplitude, octaves, randomseed); //plugs the above variables into the perlin noise generator

	//DRAW LAND//
	for (int z = 0; z <= Worldsize; z++) // draw along the Z axis
	{
		for (int x = 0; x <= Worldsize; x++) //draw along the X axis
		{
			WorldHeight[x][z] = p.GetHeight(x, z); //stores the height of the surface block into the world height array at the X&Z so it can be referenced later.  
			//GENERATE A BLOCK//
			auto boxRitem = std::make_unique<RenderItem>();
			XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f)*XMMatrixTranslation(x, p.GetHeight(x, z), z)); //sets the Y height of the block equal to the perlin noise value 
			XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
			boxRitem->ObjCBIndex = i; //store the object index of the render item.
			i++;//increment the index value for the next item

			if (WorldHeight[x][z] <= -2)
			{ 
				boxRitem->Mat = mMaterials["sandMat"].get(); //draw a sand block by changing obj material //if block is equal to water height (-2) make it sand
			}
			else
			{
				boxRitem->Mat = mMaterials["grassMat"].get(); //draw a grass block as it will be above -2 (water level) 
			}

			boxRitem->Geo = mGeometries["boxGeo"].get(); //gets the box geometry 
			boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
			boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
			boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
			mAllRitems.push_back(std::move(boxRitem));

			//Draw Grass or a tree or Flowers or sugarCane//
			if ((RandomNum(1, 100, 1) > 99) && p.GetHeight(x, z) == -2) // 1% chance of creating sugar cane at water level if the block is not water
			{
				int mh = RandomNum(1, 4, 1); 
				for (int h = 0; h < mh; h++)
				{ //draws sugar cane
					auto sugarRitem = std::make_unique<RenderItem>();
					XMStoreFloat4x4(&sugarRitem->World, XMMatrixScaling(1.0, 1.0, 1.0) * XMMatrixRotationY(0.785398) * XMMatrixTranslation(x - 1, p.GetHeight(x, z) + 0.25 + h, z + 1.2));				//XMStoreFloat4x4(&grassRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
					sugarRitem->ObjCBIndex = i;
					i++;
					sugarRitem->Mat = mMaterials["sugarMat"].get();
					sugarRitem->Geo = mGeometries["quadGeo"].get();
					sugarRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					sugarRitem->IndexCount = sugarRitem->Geo->DrawArgs["quad"].IndexCount;
					sugarRitem->StartIndexLocation = sugarRitem->Geo->DrawArgs["quad"].StartIndexLocation;
					sugarRitem->BaseVertexLocation = sugarRitem->Geo->DrawArgs["quad"].BaseVertexLocation;
					mAllRitems.push_back(std::move(sugarRitem));

					auto sugarRitem2 = std::make_unique<RenderItem>();
					XMStoreFloat4x4(&sugarRitem2->World, XMMatrixScaling(1.0, 1.0, 1.0) * XMMatrixRotationY(-0.785398) * XMMatrixTranslation(x - 1, p.GetHeight(x, z) + 0.25 + h, z - 1));
					sugarRitem2->ObjCBIndex = i;
					i++;
					sugarRitem2->Mat = mMaterials["sugarMat"].get();
					sugarRitem2->Geo = mGeometries["quadGeo"].get();
					sugarRitem2->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					sugarRitem2->IndexCount = sugarRitem2->Geo->DrawArgs["quad"].IndexCount;
					sugarRitem2->StartIndexLocation = sugarRitem2->Geo->DrawArgs["quad"].StartIndexLocation;
					sugarRitem2->BaseVertexLocation = sugarRitem2->Geo->DrawArgs["quad"].BaseVertexLocation;
					mAllRitems.push_back(std::move(sugarRitem2));
				}
			}

			if ((RandomNum(1, 100, 1) > 99) && p.GetHeight(x, z) > -2) //creates a yellow flower above water level ontop of a block with a 1% chance
			{
				auto flowerRitem = std::make_unique<RenderItem>();
				XMStoreFloat4x4(&flowerRitem->World, XMMatrixScaling(0.5, 0.5, 0.5) * XMMatrixRotationY(0.785398) * XMMatrixTranslation(x - 0.5, p.GetHeight(x, z) + 0.5, z + 0.6));
				//XMStoreFloat4x4(&grassRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
				flowerRitem->ObjCBIndex = i;
				i++;
				flowerRitem->Mat = mMaterials["flowerYMat"].get();
				flowerRitem->Geo = mGeometries["quadGeo"].get();
				flowerRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				flowerRitem->IndexCount = flowerRitem->Geo->DrawArgs["quad"].IndexCount;
				flowerRitem->StartIndexLocation = flowerRitem->Geo->DrawArgs["quad"].StartIndexLocation;
				flowerRitem->BaseVertexLocation = flowerRitem->Geo->DrawArgs["quad"].BaseVertexLocation;
				mAllRitems.push_back(std::move(flowerRitem));

				auto flowerRitem2 = std::make_unique<RenderItem>();
				XMStoreFloat4x4(&flowerRitem2->World, XMMatrixScaling(0.5, 0.5, 0.5) * XMMatrixRotationY(-0.785398) * XMMatrixTranslation(x - 0.5, p.GetHeight(x, z) + 0.5, z - 0.5));
				//XMStoreFloat4x4(&grassRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
				flowerRitem2->ObjCBIndex = i;
				i++;
				flowerRitem2->Mat = mMaterials["flowerYMat"].get();
				flowerRitem2->Geo = mGeometries["quadGeo"].get();
				flowerRitem2->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				flowerRitem2->IndexCount = flowerRitem2->Geo->DrawArgs["quad"].IndexCount;
				flowerRitem2->StartIndexLocation = flowerRitem2->Geo->DrawArgs["quad"].StartIndexLocation;
				flowerRitem2->BaseVertexLocation = flowerRitem2->Geo->DrawArgs["quad"].BaseVertexLocation;
				mAllRitems.push_back(std::move(flowerRitem2));
			}

			else if ((RandomNum(1, 100, 1) > 99) && p.GetHeight(x, z) > -2) //creates a red flower above water level ontop of a block with a 1% chance if it hasnt made a yellow flower
			{
				auto flowerRitem = std::make_unique<RenderItem>();
				XMStoreFloat4x4(&flowerRitem->World, XMMatrixScaling(0.5, 0.5, 0.5) * XMMatrixRotationY(0.785398) * XMMatrixTranslation(x - 0.5, p.GetHeight(x, z) + 0.5, z + 0.6));
				//XMStoreFloat4x4(&grassRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
				flowerRitem->ObjCBIndex = i;
				i++;
				flowerRitem->Mat = mMaterials["flowerRMat"].get();
				flowerRitem->Geo = mGeometries["quadGeo"].get();
				flowerRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				flowerRitem->IndexCount = flowerRitem->Geo->DrawArgs["quad"].IndexCount;
				flowerRitem->StartIndexLocation = flowerRitem->Geo->DrawArgs["quad"].StartIndexLocation;
				flowerRitem->BaseVertexLocation = flowerRitem->Geo->DrawArgs["quad"].BaseVertexLocation;
				mAllRitems.push_back(std::move(flowerRitem));

				auto flowerRitem2 = std::make_unique<RenderItem>();
				XMStoreFloat4x4(&flowerRitem2->World, XMMatrixScaling(0.5, 0.5, 0.5) * XMMatrixRotationY(-0.785398) * XMMatrixTranslation(x - 0.5, p.GetHeight(x, z) + 0.5, z - 0.5));
				//XMStoreFloat4x4(&grassRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
				flowerRitem2->ObjCBIndex = i;
				i++;
				flowerRitem2->Mat = mMaterials["flowerRMat"].get();
				flowerRitem2->Geo = mGeometries["quadGeo"].get();
				flowerRitem2->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				flowerRitem2->IndexCount = flowerRitem2->Geo->DrawArgs["quad"].IndexCount;
				flowerRitem2->StartIndexLocation = flowerRitem2->Geo->DrawArgs["quad"].StartIndexLocation;
				flowerRitem2->BaseVertexLocation = flowerRitem2->Geo->DrawArgs["quad"].BaseVertexLocation;
				mAllRitems.push_back(std::move(flowerRitem2));
			}


			else if ((RandomNum(1, 100, 1) > 70) && p.GetHeight(x, z) > -2) //if no flower was created, have a 30% chance of drawing long grass
			{
				auto grassRitem = std::make_unique<RenderItem>();
				XMStoreFloat4x4(&grassRitem->World, XMMatrixScaling(1.0, 1.0, 1.0) * XMMatrixRotationY(0.785398) * XMMatrixTranslation(x - 1, p.GetHeight(x, z) + 0.25, z + 1.2));
				grassRitem->ObjCBIndex = i;
				i++;
				grassRitem->Mat = mMaterials["longGrassMat"].get();
				grassRitem->Geo = mGeometries["quadGeo"].get();
				grassRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				grassRitem->IndexCount = grassRitem->Geo->DrawArgs["quad"].IndexCount;
				grassRitem->StartIndexLocation = grassRitem->Geo->DrawArgs["quad"].StartIndexLocation;
				grassRitem->BaseVertexLocation = grassRitem->Geo->DrawArgs["quad"].BaseVertexLocation;
				mAllRitems.push_back(std::move(grassRitem));

				auto grassRitem2 = std::make_unique<RenderItem>();
				XMStoreFloat4x4(&grassRitem2->World, XMMatrixScaling(1.0, 1.0, 1.0) * XMMatrixRotationY(-0.785398) * XMMatrixTranslation(x -1, p.GetHeight(x, z) + 0.25 , z -1 ));
				//XMStoreFloat4x4(&grassRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
				grassRitem2->ObjCBIndex = i;
				i++;
				grassRitem2->Mat = mMaterials["longGrassMat"].get();
				grassRitem2->Geo = mGeometries["quadGeo"].get();
				grassRitem2->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				grassRitem2->IndexCount = grassRitem2->Geo->DrawArgs["quad"].IndexCount;
				grassRitem2->StartIndexLocation = grassRitem2->Geo->DrawArgs["quad"].StartIndexLocation;
				grassRitem2->BaseVertexLocation = grassRitem2->Geo->DrawArgs["quad"].BaseVertexLocation;
				mAllRitems.push_back(std::move(grassRitem2));
			}

			else if ((RandomNum(1, 200, 1) > 199) && p.GetHeight(x, z) > -2) // if no flower, or grass was created, have a 0.5% chance of creating a tree
			{
				for (int treeH = 0; treeH < 5; treeH++) //create the tree trunk
				{
					auto trunkRitem = std::make_unique<RenderItem>();
					XMStoreFloat4x4(&trunkRitem->World, XMMatrixScaling(1.0, 1.0, 1.0)* XMMatrixTranslation(x , p.GetHeight(x, z) + 1 + treeH, z));
					//XMStoreFloat4x4(&grassRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
					trunkRitem->ObjCBIndex = i;
					i++;
					trunkRitem->Mat = mMaterials["woodMat"].get();
					trunkRitem->Geo = mGeometries["boxGeo"].get();
					trunkRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					trunkRitem->IndexCount = trunkRitem->Geo->DrawArgs["box"].IndexCount;
					trunkRitem->StartIndexLocation = trunkRitem->Geo->DrawArgs["box"].StartIndexLocation;
					trunkRitem->BaseVertexLocation = trunkRitem->Geo->DrawArgs["box"].BaseVertexLocation;
					mAllRitems.push_back(std::move(trunkRitem));
				}

				int maxHeight = 4; 
				

				for (int treeH = 0; treeH < maxHeight; treeH++) //nested for loops to create the leaves of the trees around and above the top of the trunk
				{
					for (int treeW = -2; treeW < 3; treeW++)
					{
						for (int treeL = -2; treeL < 3; treeL++)
						{
							auto trunkRitem = std::make_unique<RenderItem>();
							XMStoreFloat4x4(&trunkRitem->World, XMMatrixScaling(1.0, 1.0, 1.0)* XMMatrixTranslation(x + treeW, p.GetHeight(x, z) + 3 + treeH, z + treeL));
							//XMStoreFloat4x4(&grassRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
							trunkRitem->ObjCBIndex = i;
							i++;
							trunkRitem->Mat = mMaterials["leafMat"].get();
							trunkRitem->Geo = mGeometries["boxGeo"].get();
							trunkRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
							trunkRitem->IndexCount = trunkRitem->Geo->DrawArgs["box"].IndexCount;
							trunkRitem->StartIndexLocation = trunkRitem->Geo->DrawArgs["box"].StartIndexLocation;
							trunkRitem->BaseVertexLocation = trunkRitem->Geo->DrawArgs["box"].BaseVertexLocation;
							mAllRitems.push_back(std::move(trunkRitem));	
						}
					}	
				}
			}

			//DRAW DOWN//
			for (int depth = 0; depth < 28; depth++) //after creating top block and/or foliage, start drawing down from the block to create depth layers 
			{
				///makes world hollow
			//	if (((z == 0) || (z == Worldsize) || ((x == 0) || x == Worldsize)) || ((p.GetHeight(x, z) == WorldHeight[x][z] -1) || (p.GetHeight(x, z) == WorldHeight[x][z] - depth)))
			//	{
				auto boxRitem = std::make_unique<RenderItem>();
				XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f)*XMMatrixTranslation(x, p.GetHeight(x, z) - depth, z));  //subtract the depth from the height of the block 
				XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
				boxRitem->ObjCBIndex = i;
				i++;

				if (depth <= 0 && p.GetHeight(x, z) == -2)
				{
					boxRitem->Mat = mMaterials["sandMat"].get();  //if the block is equal in height to the water or less, make it sand 

				}
				else if (depth <= 0)
				{
					boxRitem->Mat = mMaterials["sandMat"].get();

				}
				else if (depth>0 && depth <= 5)
				{
					boxRitem->Mat = mMaterials["dirtMat"].get(); // if block is below 0 depth and less than 5 depth make it dirt
				}

				else if (depth > 5 && depth <= 15) // else if block is deeper than 5 but not as deep as 15 decide what to make it: 
				{
					if (RandomNum(1, 100, 1) > 95)
					{
						boxRitem->Mat = mMaterials["coalMat"].get(); //5% chance of creating a coal block -- coal is more common than iron above 15
					}
					else if (RandomNum(1, 100, 1) > 98)
					{
						boxRitem->Mat = mMaterials["ironMat"].get(); //2% chance of creating iron ore
					}
					else
					{
						boxRitem->Mat = mMaterials["stoneMat"].get(); // if no ores selected, just draw stone 
					}
				}

				else if (depth > 15  && depth < 25) //else if the block is deeper than 15 but not as deep as 25
				{
					if (RandomNum(1, 200, 1) > 199)
					{
						boxRitem->Mat = mMaterials["diamondMat"].get(); //0.5% chance of diamond
					}
					else if (RandomNum(1, 100, 1) > 99)
					{
						boxRitem->Mat = mMaterials["redsMat"].get(); //1% chance of red stone 
					}

					else if (RandomNum(1, 100, 1) > 98)
					{
						boxRitem->Mat = mMaterials["coalMat"].get(); // 2% chance of coal
					}
					else if (RandomNum(1, 100, 1) > 95)
					{
						boxRitem->Mat = mMaterials["ironMat"].get(); // 5% chance of iron ore -- iron ore is more common than coal below 15
					}
					else
					{
						boxRitem->Mat = mMaterials["stoneMat"].get(); // draw stone if no ore is created
					}
				}
				else
				{
					boxRitem->Mat = mMaterials["bedrockMat"].get(); // else if none of the above are true, create bedrock
				}

				boxRitem->Geo = mGeometries["boxGeo"].get();
				boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
				boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
				boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
				mAllRitems.push_back(std::move(boxRitem));
				//	}
			}
		}
	}

	for (int waterz = 0; waterz <= Worldsize; waterz++) //DRAW WATER
	{
		for (int waterx = 0; waterx <= Worldsize; waterx++)
		{
			for (int watery = -2; watery >= -10; watery--) //draw water at a set level if -2 and down
			{
				auto Water = std::make_unique<RenderItem>();
				XMStoreFloat4x4(&Water->World, XMMatrixScaling(1.0f, 1.0f, 1.0f)*XMMatrixTranslation(waterx, watery, waterz));
				XMStoreFloat4x4(&Water->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));

				Water->ObjCBIndex = i;
				i++;
				Water->Mat = mMaterials["waterMat"].get();
				Water->Geo = mGeometries["boxGeo"].get();
				Water->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				Water->IndexCount = Water->Geo->DrawArgs["box"].IndexCount;
				Water->StartIndexLocation = Water->Geo->DrawArgs["box"].StartIndexLocation;
				Water->BaseVertexLocation = Water->Geo->DrawArgs["box"].BaseVertexLocation;
				mAllRitems.push_back(std::move(Water));
			}
		}
	}



	
	for (auto& e : mAllRitems)
	{	//if the render item material, is equal to one that is alpha tested...
		if (e.get()->Mat == mMaterials["longGrassMat"].get() || e.get()->Mat == mMaterials["leafMat"].get() || e.get()->Mat == mMaterials["flowerYMat"].get() || e.get()->Mat == mMaterials["flowerRMat"].get() || e.get()->Mat == mMaterials["sugarMat"].get())
		{
			mRitemLayer[(int)RenderLayer::AlphaTested].push_back(e.get()); //...pass it to the alpha tested render layer
		}
		else if(e.get()->Mat == mMaterials["waterMat"].get()) // if the render item's material is transparent...
		{
			mRitemLayer[(int)RenderLayer::Transparent].push_back(e.get());  //...pass it to the transparent render layer
			
		}
		else //else pass all other items to the opaque render layer
		{
			mRitemLayer[(int)RenderLayer::Opaque].push_back(e.get());
		}
	}
}


double CrateApp::RandomNum(int range_min, int range_max, int n)  //COPIED FROM MSDN// - Then modified - //Origional Link https://msdn.microsoft.com/en-us/library/398ax69y.aspx //
{
	// Generate random numbers in the half-closed interval  
	// [range_min, range_max). In other words,  
	// range_min <= random number < range_max  
	

	//removed for loop because we only want one value
		double u = (double)rand() / (RAND_MAX + 1) * (range_max - range_min)
			+ range_min;
		return(u);  //modified this line to return a value rather than print it.
}

void CrateApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		
			auto ri = ritems[i];

			cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
			cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
			cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

			CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

			D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
			D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;

			cmdList->SetGraphicsRootDescriptorTable(0, tex);
			cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
			cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

			cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
		
		
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> CrateApp::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,
		8);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f,
		8);

	return{
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}


void CrateApp::BuildSkyGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData skyBox = geoGen.CreateSkyBox(250.0f, 250.0f, 250.0f, 0);

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)skyBox.Indices32.size();
	boxSubmesh.StartIndexLocation = 0;
	boxSubmesh.BaseVertexLocation = 0;

	std::vector<Vertex> vertices(skyBox.Vertices.size());

	for (size_t i = 0; i < skyBox.Vertices.size(); ++i)
	{
		vertices[i].Pos = skyBox.Vertices[i].Position;
		vertices[i].Normal = skyBox.Vertices[i].Normal;
		vertices[i].TexC = skyBox.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices = skyBox.GetIndices16();

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "skyBoxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["skyBox"] = boxSubmesh;

	mGeometries["skyBoxGeo"] = std::move(geo);
}

void CrateApp::UpdateChar(float x, float y, float z , float Xs, float Ys, float Zs, float angle)
{
	
	charRotation = angle;

	//charX += Xs; 
	//charY += Ys;
	//charZ += Zs; 


	int X = charX;
	//int Y = charY;
	int Z = charZ;
	float Y = (WorldHeight[X][Z]); //COLLISION//  set the character height equal to  the charX and charZ values in the World height array (Make the charater the same height as the block he is equal to on the X and Z axis)
	charY = Y; 

	//XMStoreFloat4x4(&character->World, XMMatrixRotationY(30.0f));
	//XMStoreFloat4x4(&character->World, XMMatrixRotationRollPitchYaw(90.0f, 30.0f, 90.0f));
	//Declare charPresets
	/*XMVECTOR ScalingOrigin =  XMVectorSet(charX, charY, charZ, 0.0f);
	XMVECTOR ScalingOrientationQuaternion = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR Scaling = XMVectorSet(1.0f, 2.0f, 1.0f, 1.0f);
	XMVECTOR RotationOrigin = XMVectorSet(charX, charY, charZ, 1.0f);
	XMVECTOR RotationQuaternion = XMVectorSet(0.0f, charRotation, 0.0f, 1.0f);
	XMVECTOR Translation = XMVectorSet(charX, charY, charZ, 1.0f);
*/
	DirectX::XMFLOAT3 Look = { 0.0f, 0.0f, 1.0f };

	/*XMMATRIX Local = XMMatrixTransformation(
		ScalingOrigin,
		ScalingOrientationQuaternion,
		Scaling,
		RotationOrigin,
		RotationQuaternion,
		Translation
	);*/

	// SRT  //Scale, rotate, translate//
	auto character = std::make_unique<RenderItem>();
	CharPos = XMVectorSet(charX, Y, charZ , 1.0f); 
	
	XMMATRIX Local = (XMMatrixScaling(1.0f, 2.0f, 1.0f)*XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f)*XMMatrixTranslation(charX, Y + 1.5, charZ)); //set the position = to the charX, Y, charZ
	XMStoreFloat4x4(&character->World, Local);
	//XMVECTOR *outScale = &V0; 
	//XMVECTOR *outRotQuat = &V0;
	//XMVECTOR *outTrans = &V0;

	//XMMatrixDecompose(
	//	outScale,
	//	outRotQuat,
	//	outTrans,
	//	Local
	//);

	//CharPos = *outTrans;
	
	//float tempX = XMVectorGetX(V0);
	//float tempY = XMVectorGetY(V0);
	//float tempZ = XMVectorGetZ(V0);

	XMStoreFloat4x4(&character->TexTransform, XMMatrixScaling(1.0f, 2.0f, 1.0f));
	//XMMATRIX R = XMMatrixRotationY();
	//XMStoreFloat3(&Look, XMVector3TransformNormal(XMLoadFloat3(&Look), R));

	character->ObjCBIndex = 0;
	character->Mat = mMaterials["stoneMat"].get();
	character->Geo = mGeometries["boxGeo"].get();
	character->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	character->IndexCount = character->Geo->DrawArgs["box"].IndexCount;
	character->StartIndexLocation = character->Geo->DrawArgs["box"].StartIndexLocation;
	character->BaseVertexLocation = character->Geo->DrawArgs["box"].BaseVertexLocation;

	mAllRitems.push_back(std::move(character));

	if (cam3 == true )
	{
		//FXMVECTOR pos = XMVectorSet(charX, Y + 5, charZ - 10, 1.0f);

		FXMVECTOR pos = XMVectorSet(charX, Y + 5, charZ - 10 , 1.0f); // CharPos;				(charX, Y + 5, charZ - 10 , 1.0f)
		FXMVECTOR target = XMVectorSet(charX, Y+2, charZ, 0.0f);
		FXMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);

		//FXMVECTOR temp = XMVectorSet(tempX, tempY+2, tempZ, 1.0f);

		//freeCam.RotateY(charRotation);
		//freeCam.CustRotateY(charRotation);


		freeCam.CustLookAt(pos, target, up); //follows and looks at character
		
		//freeCam.SetPosition(tempX, tempY+2, tempZ-5); //(charX, charY, charZ - 5);
		//freeCam.SetPosition(charX, charY, charZ - 5);

		freeCam.UpdateViewMatrix();
	}

	else if (cam1 == true)
	{
		freeCam.FP(true);
		int tx = (int)XMVectorGetIntX(freeCam.GetPosition());
		int tz = (int)XMVectorGetIntZ(freeCam.GetPosition());
		
		int  ty = WorldHeight[X][Z];

		freeCam.ClampHeight = Y + 2; 
	}

}


