#include "stdafx.h"
#include <d3d11.h>
#include <dxgi.h>
#include <DirectXPackedVector.h> 
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <Windows.h>

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <stdint.h>
#include "WICTextureLoader.h"

#include "matchThree.h"
#include "Common.h"
#include "gameObject.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define SCREENWIDTH 1440
#define SCREENHEIGHT 1024

#define GRIDWIDTH (SCREENWIDTH * .55f)
#define GRIDHEIGHT (SCREENHEIGHT * .78f)

#define COMPILESHADERDEBUG 1

#define GRIDLEFT 400
#define GRIDTOP 100

#define HORIZONTALTILES 8
#define VERTICALTILES 8

using namespace DirectX;
t_int64 mouseX = 0, mouseY = 0;
t_int64 mouseDown = 0;
t_int32 mouseWasDownLastFrame = 0;

ID3D11Device *device;
ID3D11DeviceContext *context;
ID3D11VertexShader *vShader;
ID3D11PixelShader *pShader;
ID3D11Buffer *pvBuffer;
ID3D11Buffer *piBuffer;

IDXGISwapChain *swapChain;
ID3D11RenderTargetView *backBuffer;
ID3D11DepthStencilView *stencilView;

ID3D11InputLayout *pLayout;
ID3D11Texture2D *pDepthBuffer;

ID3D11Buffer *cBufferColorMod;
ID3D11Buffer *cBufferOffset;
ID3D11Buffer *cBufferWorld;

DirectX::XMMATRIX matTranslation;
DirectX::XMMATRIX projMatrix;
DirectX::XMMATRIX viewMatrix;

ID3D11RasterizerState *rasterState;
ID3D11DepthStencilState *depthStencilState;
ID3D11BlendState *blendState;

std::map<std::wstring,ID3D11ShaderResourceView*> textures;

FLOAT2 screenRect;

std::vector<std::shared_ptr<gameObject>> gameObjects;
std::shared_ptr<gameObject> levelTiles[8][8];
std::shared_ptr<gameObject> mouseCursor;
std::shared_ptr<gameObject> selectedTile;

struct CBUFFER
{
	DirectX::XMFLOAT4X4 world; //4*4*4 = 64
	DirectX::XMFLOAT4X4 rotation; //4*4*4 =64
	DirectX::XMFLOAT4 light; //4*4 =16
	DirectX::XMFLOAT4 lightColor; //4*4 = 16
	DirectX::XMFLOAT4 ambientColor; //4*4 =16
	DirectX::XMFLOAT3 padding;
	int selected;
};

D3D11_INPUT_ELEMENT_DESC inputDesc[] =
{
	{"POSITION",0, DXGI_FORMAT_R32G32B32_FLOAT ,0,0,D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"NORMAL",0, DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,0, 24, D3D11_INPUT_PER_VERTEX_DATA,0},
};

static_assert((sizeof(CBUFFER) % 16) == 0, "Constant Buffer size must be 16-byte aligned");

void ResizeBuffers(FLOAT2 screenSize);

LRESULT CALLBACK WindowProc(
	HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam
)
{
	switch (msg)
	{
	case WM_DESTROY:
	case WM_CLOSE:
	{
		PostQuitMessage(0);
		return 0;
	}

	case WM_SIZE:
	{
		RECT window;
		GetClientRect(hwnd, &window);

		screenRect = FLOAT2(((t_float32)window.right - window.left), (t_float32)(window.bottom - window.top));

		ResizeBuffers(screenRect);

	}break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}


bool zOrder(const std::shared_ptr<gameObject> left, const std::shared_ptr<gameObject> right)
{
	return left->tag < right->tag;
}

void ResizeBuffers(FLOAT2 screenSize)
{
	if (backBuffer)
	{
		context->OMSetRenderTargets(0, 0, 0);

		backBuffer->Release();

		context->Flush();
	
		swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

		ID3D11Texture2D *buffer;
		swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buffer);

	    device->CreateRenderTargetView(buffer, NULL, &backBuffer);
		buffer->Release();
		
		D3D11_VIEWPORT vport;
		vport.Height = screenSize.y;
		vport.Width = screenSize.x;
		vport.MinDepth = 0;
		vport.MaxDepth = 1.0f;
		vport.TopLeftX = 0;
		vport.TopLeftY = 0;

		context->OMSetRenderTargets(1, &backBuffer, NULL);
		context->RSSetViewports(1, &vport);
	}
}

void InitShaders()
{
	ID3D10Blob *vsBlob, *psBlob, *error;
	//load
	HRESULT h =D3DCompileFromFile(L"shaders.shader",0,0, "VShader", "vs_4_0", COMPILESHADERDEBUG, 0, &vsBlob, &error);

	D3DCompileFromFile(L"shaders.shader", 0, 0, "PShader", "ps_4_0", COMPILESHADERDEBUG, 0, &psBlob, &error);

	//create shaders
	device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &vShader);
	device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &pShader);

	//set in gpu
	context->VSSetShader(vShader, 0, 0);
	context->PSSetShader(pShader, 0, 0);

	device->CreateInputLayout(inputDesc, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &pLayout);
	context->IASetInputLayout(pLayout);
	
	//constant buffer World matrix
	D3D11_BUFFER_DESC cdesc2;
	ZeroMemory(&cdesc2, sizeof(cdesc2));
	cdesc2.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cdesc2.ByteWidth = sizeof(CBUFFER);
	cdesc2.Usage = D3D11_USAGE_DEFAULT;

	device->CreateBuffer(&cdesc2, NULL, &cBufferWorld);
	//copy the verts to the buffer
	context->VSSetConstantBuffers(0, 1, &cBufferWorld);
}

void LoadTextures()
{
	t_int32 textureLeng = _ARRAYSIZE(textureResources);

	for (t_int32 i = 0; i < textureLeng; ++i)
	{
		ID3D11ShaderResourceView *newTex;
		HRESULT r = DirectX::CreateWICTextureFromFile(device, context, textureResources[i].c_str(), nullptr, &newTex);

		if (SUCCEEDED(r))
		{
			textures.insert(std::make_pair(textureResources[i], newTex));
		}
	}
}

INT2 FindTilePosition(t_float32 x, t_float32 y, t_float32 tileWidth, t_float32 tileHeight)
{
	t_int32 xIndex = ((t_int32)(x) + tileWidth) / tileWidth;
	t_int32 yIndex = VERTICALTILES / 2 - (t_int32)(y) / tileHeight;

	//cap the mouse index in case its outside of the grid to the grid bounds
	if (xIndex < 0)
		xIndex = 0;

	if (xIndex >= HORIZONTALTILES)
		xIndex = HORIZONTALTILES - 1;

	if (yIndex < 0)
		yIndex = 0;

	if (yIndex >= VERTICALTILES)
		yIndex = VERTICALTILES - 1;

	return INT2(xIndex, yIndex);
}

FLOAT2 PositionOnTile(t_int32 xscale, t_int32 yscale, t_float32 gap, t_int32 windex, t_int32 hindex)
{
	if (windex < 0)
		windex = 0;

	if (windex >= VERTICALTILES)
		windex = VERTICALTILES-1;

	if (hindex < 0)
		hindex = 0;
	
	if (hindex >= HORIZONTALTILES)
		hindex = HORIZONTALTILES-1;

	hindex = 7 - hindex;
	t_float32 offset =  (t_float32)(xscale / 2.0f + (gap*windex));
	t_float32 x = ((t_float32)(xscale * windex));

	t_float32 yoffset = (t_float32)(yscale / 2 + (gap* hindex));
	t_float32 y = (t_float32)(yscale* hindex) ;

	t_float32 top = (t_float32)(-(t_float32)GRIDHEIGHT/2.0f);
	t_float32 leftSide = (t_float32)(-xscale);

	return FLOAT2(leftSide + x + offset, top + y + yoffset);
}

void SwapTiles(std::shared_ptr<gameObject> a, std::shared_ptr<gameObject> b, t_int32 newX, t_int32 newY)
{
	gameObject c = *a.get();

	a->targetVector = b->position2D;
	b->targetVector =a->position2D;

	a->targetIsSet = 1;
	b->targetIsSet = 1;

	INT2 selectedIndex = FindTilePosition(a->position2D.x, a->position2D.y, a->scale2D.x, a->scale2D.y);

	levelTiles[newX][newY] = a;
	levelTiles[selectedIndex.x][selectedIndex.y] = b;

}

void RunDropAnimation(std::shared_ptr<gameObject> go, t_int32 xTargetIndex, t_int32 yTargetIndex)
{
	go->isPlayable = false;
	go->targetIsSet = true;
	go->targetVector = PositionOnTile(go->scale2D.x, go->scale2D.y, 2, xTargetIndex, yTargetIndex);
}

std::shared_ptr<gameObject> AddGameObject(t_int32 xscale, t_int32 yscale, t_float32 rot, t_float32 gap, t_int32 windex, t_int32 hindex,
	t_int32 isPlayable, t_int32 texture, drawLayer tag)
{
	FLOAT2 pos = PositionOnTile(xscale, yscale, gap, windex, hindex);

	std::shared_ptr<gameObject> go = std::make_shared<gameObject>();

	go->SetPosition2D(pos.x, pos.y);
	go->targetIsSet = 0;
	go->targetVector = FLOAT2ZERO;

	go->SetRotation2D(rot, 0);
	go->SetScale2D((t_float32)xscale, (t_float32)yscale);
	go->isPlayable = isPlayable;
	go->tag = tag;
	go->selected = 0;

	if (isPlayable)
	{
		go->SetTextureRef(textureResources[texture]);
	}
	else
	{
		go->SetTextureRef(textureResources[2]);
	}

	gameObjects.push_back(go);
	return go;
}

void StartLevel()
{
	t_int32 tileWidth = GRIDWIDTH / HORIZONTALTILES;
	t_int32 tileHeight = GRIDHEIGHT / VERTICALTILES;
	t_float32 gap = 2.0f;
	
	t_int32 levelSize = ARRAYSIZE(squareLevel);
	t_int32 h, w;

	for (t_int32 i = 0; i < levelSize; i++)
	{
		w = i % 8;
		h = i / 8;

		if (hourglassLevel[i] != 0)
		{
			//Add grid tile
			AddGameObject(tileWidth, tileHeight, 0.0f, gap, w, h, hourglassLevel[i], (w + h) % 2, drawLayer::grid);
		}
		
		///add game tile
		std::shared_ptr<gameObject> go = AddGameObject(tileWidth, tileHeight, 270.0f, gap, w, h, hourglassLevel[i], (w + h) % 5 + 5, drawLayer::go);
		levelTiles[w][h] = go;
	}
	

	FLOAT2 cursorPos = PositionOnTile(tileHeight, tileHeight, gap, 0, 0);
	mouseCursor = std::make_shared<gameObject>();
	mouseCursor->SetPosition2D(cursorPos.x, cursorPos.y);
	mouseCursor->SetRotation2D(270, 0);
	mouseCursor->SetScale2D((t_float32)tileWidth, (t_float32)tileHeight);
	mouseCursor->SetTextureRef(textureResources[4]);
	mouseCursor->targetIsSet = 0;
	mouseCursor->targetVector = FLOAT2ZERO;
	gameObjects.push_back(mouseCursor);

	//sort by z to help rend
	std::sort(gameObjects.begin(), gameObjects.end(), zOrder);

	for (t_int32 i = 0; i < VERTICALTILES; ++i)
	{
		for (t_int32 o = HORIZONTALTILES-1; o>=0; --o)
		{
			gameObject *go = levelTiles[i][o].get();

			go->targetIsSet = 1;
			go->targetVector = go->position2D;
			go->position2D = FLOAT2(go->position2D.x, GRIDHEIGHT / 2 + go->scale2D.y*2);
			go->animationDelay =( 7-i);
		}
	}
	//Need to schedule animation here:::: for drop down 
}

void InitGraphics()
{
	//Vertex Buffer for all sprites
	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(spriteVertices);
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	
	device->CreateBuffer(&desc, NULL, &pvBuffer);

	D3D11_MAPPED_SUBRESOURCE sub;
	context->Map(pvBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &sub); //map the buffer
	memcpy(sub.pData, spriteVertices, sizeof(spriteVertices));
	context->Unmap(pvBuffer, NULL);
	
	//Index buffer for sprites
	D3D11_BUFFER_DESC idesc;
	idesc.Usage = D3D11_USAGE_DEFAULT;
	idesc.ByteWidth = sizeof(unsigned long)* ARRAYSIZE(spriteIndices);
	idesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	idesc.CPUAccessFlags = 0;
	idesc.MiscFlags = 0;
	
	D3D11_SUBRESOURCE_DATA idata = {};
	idata.pSysMem = spriteIndices;
	idata.SysMemPitch = 0;
	idata.SysMemSlicePitch = 0;
	
	device->CreateBuffer(&idesc,&idata, &piBuffer);
	
	//Texture Sampler
	D3D11_SAMPLER_DESC sDesc;
	sDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sDesc.MaxAnisotropy = 1;
	sDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	//Rasterizer
	D3D11_RASTERIZER_DESC rdesc = {};
	rdesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	rdesc.CullMode = D3D11_CULL_NONE;
	rdesc.AntialiasedLineEnable = 0;
	rdesc.MultisampleEnable = 0;
	rdesc.FrontCounterClockwise = 0;
	rdesc.ScissorEnable = 0;

	D3D11_DEPTH_STENCIL_DESC stenDesc;
	ZeroMemory(&stenDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	
	stenDesc.DepthEnable = 0;
	stenDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	stenDesc.DepthFunc = D3D11_COMPARISON_LESS;
	stenDesc.StencilEnable = 1;
	stenDesc.StencilReadMask = 0xFF;
	stenDesc.StencilWriteMask = 0xFF;
	stenDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	stenDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	stenDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	stenDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	stenDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	stenDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	stenDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	stenDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS; 


	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));
	blendDesc.RenderTarget[0].BlendEnable = 1;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	
	device->CreateBlendState(&blendDesc, &blendState);

	device->CreateDepthStencilState(&stenDesc,&depthStencilState);
	device->CreateRasterizerState(&rdesc, &rasterState);
	context->RSSetState(rasterState);
}

void InitD3D(HWND hwnd)
{
	DXGI_SWAP_CHAIN_DESC scd;

	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.Height = SCREENHEIGHT;
	scd.BufferDesc.Width = SCREENWIDTH;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = hwnd;
	scd.SampleDesc.Count=4; //how many multi samples
	scd.Windowed = TRUE;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; //allow screen size switch

	D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&scd,
		&swapChain,
		&device,
		NULL,
		&context);
	
	ID3D11Texture2D * pBackBuffer;

	//get address of backbuffer
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	//Create render target from buffer
	device->CreateRenderTargetView(pBackBuffer, NULL, &backBuffer);
	pBackBuffer->Release();

	D3D11_TEXTURE2D_DESC dBufferDesc;

	ZeroMemory(&dBufferDesc, sizeof(dBufferDesc));
	dBufferDesc.Width = SCREENWIDTH;
	dBufferDesc.Height = SCREENHEIGHT;
	dBufferDesc.ArraySize = 1;
	dBufferDesc.MipLevels = 1;
	dBufferDesc.SampleDesc.Count = 4;
	dBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	device->CreateTexture2D(&dBufferDesc, NULL, &pDepthBuffer);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsv;
	ZeroMemory(&dsv, sizeof(dsv));

	dsv.Format =DXGI_FORMAT_D32_FLOAT;
	dsv.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

	device->CreateDepthStencilView(pDepthBuffer, &dsv, &stencilView);
	pDepthBuffer->Release();
	
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	viewport.Height = SCREENHEIGHT;
	viewport.Width =SCREENWIDTH;

	context->OMSetRenderTargets(1, &backBuffer, stencilView);
	context->RSSetViewports(1, &viewport);

	InitShaders();
	InitGraphics();
}

void CleanD3D()
{
	for (auto i : textures)
	{
		i.second->Release();
		i.second = 0;
	}

	textures.clear();

	depthStencilState->Release();
	depthStencilState = 0;

	piBuffer->Release();
	piBuffer = 0;
	stencilView->Release();
	stencilView = 0;
	pDepthBuffer->Release();
	pDepthBuffer = 0;

	pLayout->Release();
	pLayout = 0;
	pvBuffer->Release();
	pvBuffer = 0;
	pShader->Release();
	pShader = 0;
	vShader->Release();
	vShader = 0;
	
	swapChain->SetFullscreenState(FALSE, NULL);
	swapChain->Release();
	swapChain = 0;
	
	backBuffer->Release();
	backBuffer = 0;
	context->Release();
	context = 0;
	device->Release();
	device = 0;
}

void UpdateRender(float dt)
{
	wchar_t buffer[256];
	wsprintf(buffer, L"mouseX: %d   mouseY: %d  down%d\n", mouseX, mouseY,mouseDown);
//	OutputDebugString(buffer);

	const float color[] =  { 0.0f, 0.2f, 0.4f, 1.0f };
	context->ClearRenderTargetView(backBuffer, color);
	context->ClearDepthStencilView(stencilView, D3D11_CLEAR_DEPTH, 1, 0);

	//Camera params
	DirectX::FXMVECTOR eye = { 0,0,0};
	DirectX::FXMVECTOR lookat = { 0,0,1};
	DirectX::FXMVECTOR up = { 0,1,0 };

	context->OMSetDepthStencilState(depthStencilState,1 );

	t_float32 factr[4] = { 0,0,0,0 };
	UINT mask = 0xffffffff;
	context->OMSetBlendState(blendState, factr, mask);
	//view transform
	viewMatrix =  DirectX::XMMatrixLookAtLH(eye, lookat, up);
	
	//projection
	//projMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45), (t_float32)SCREENWIDTH / (t_float32)SCREENHEIGHT, 0.1f, 100);
	projMatrix = DirectX::XMMatrixOrthographicLH(screenRect.x, screenRect.y, 0.1f, 100.0f);
	
	DirectX::XMFLOAT4 light = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);

	CBUFFER buff = {};
	buff.light = light;
	buff.lightColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	buff.ambientColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	UINT stride = sizeof(VERTEX);
	UINT offset = 0;

	//set the vertex buffer
	context->IASetVertexBuffers(0, 1, &pvBuffer, &stride, &offset);
	context->IASetIndexBuffer(piBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMFLOAT3 mouseTemp;
	XMVECTOR mouseVector =DirectX::XMVector3Unproject(DirectX::XMVectorSet(mouseX, mouseY, 1,1), 0, 0, screenRect.x, screenRect.y, 0.1f, 100.0f, projMatrix, viewMatrix,DirectX::XMMatrixIdentity());
	XMStoreFloat3(&mouseTemp, mouseVector);

	INT2 mouseIndex = FindTilePosition(mouseTemp.x, mouseTemp.y, mouseCursor->scale2D.x, mouseCursor->scale2D.y);

	if (levelTiles[mouseIndex.x][mouseIndex.y]->isPlayable)
	{
		FLOAT2 newMousCursorPos = PositionOnTile(mouseCursor->scale2D.x, mouseCursor->scale2D.y, 2, mouseIndex.x, mouseIndex.y);
		mouseCursor->SetPosition2D(newMousCursorPos.x, newMousCursorPos.y);

		if (mouseDown && !mouseWasDownLastFrame)
		{
			//if the clicked tile 
			if (selectedTile != nullptr)
			{
				INT2 selectedTileIndex = FindTilePosition(selectedTile->position2D.x, selectedTile->position2D.y, selectedTile->scale2D.x, selectedTile->scale2D.y);
				
				t_int32 xDist = abs(selectedTileIndex.x - mouseIndex.x);
				t_int32 yDist = abs(selectedTileIndex.y - mouseIndex.y);

				// If its too far to be swapped then we're just selecting another one
				if (xDist > 1 || yDist > 1) 
				{
					selectedTile->selected = 0;
					selectedTile = nullptr;
					
					selectedTile = levelTiles[mouseIndex.x][mouseIndex.y];
					selectedTile->selected = 1;
				}
				else if (xDist == 0 && yDist == 0)
				{
					selectedTile->selected = 0;
					selectedTile = nullptr;
				}
				else
				{
					SwapTiles(selectedTile, levelTiles[mouseIndex.x][mouseIndex.y],mouseIndex.x,mouseIndex.y);

					selectedTile->selected = 0;
					selectedTile = nullptr;
				}
			}
			else if (selectedTile == nullptr)
			{
				selectedTile = levelTiles[mouseIndex.x][mouseIndex.y];
				selectedTile->selected = 1;
			}
		}
	}

	std::wstring lastTexture;
	//Update game object, and push to pipeline
	for (std::shared_ptr<gameObject> go : gameObjects)
	{
		go->Update(dt);

		FLOAT2 rot = go->orientation2D;
		XMMATRIX rotMatrix = DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(rot.x));

		FLOAT2 pos = go->position2D;
		XMFLOAT3 fl3 = DirectX::XMFLOAT3(pos.x, pos.y,0);
		
		XMMATRIX transMatrix = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&fl3));

		FLOAT2 scale = go->scale2D;
		XMFLOAT3 scal3 = XMFLOAT3(scale.x, scale.y, 0);
		XMMATRIX scaleMatrix = DirectX::XMMatrixScalingFromVector(XMLoadFloat3(&scal3));
		XMMATRIX finalMatrix = scaleMatrix *rotMatrix *transMatrix;
	
		buff.selected = go->selected;
		finalMatrix *= viewMatrix* projMatrix;

		DirectX::XMStoreFloat4x4(&buff.world, finalMatrix);
		DirectX::XMStoreFloat4x4(&buff.rotation, rotMatrix);
		
		//need to order by texture to eliminate the swap only when neede
		/*if (lastTexture.compare(go->GetTextureRef())!=0)
		{*/
			//order the draw calls so that only texture change happens only at certain times
			context->PSSetShaderResources(0, 1, &textures.at(go->GetTextureRef()));
		//}

		lastTexture = go->GetTextureRef();
		context->UpdateSubresource(cBufferWorld, 0, 0, &buff, 0, 0);
		context->DrawIndexed(6, 0, 0);
	}
	
	//render on back buffer
	swapChain->Present(0, 0);
}

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hprevInstance,
	LPSTR lpCmlLn,
	int nCmdShow)
{
	//window handle
	HWND wnd;

	//window class info
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"WindClass";

	//Registser the window
	RegisterClassEx(&wc);

	RECT wr = { 0,0,SCREENWIDTH,SCREENHEIGHT };    // set the size, but not the position
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	wnd = CreateWindowEx(
		NULL,
		L"WindClass",
		L"MatchThree",
		WS_OVERLAPPEDWINDOW,
		0, 0,
		wr.right-wr.left,
		wr.bottom- wr.top,
		NULL,
		NULL,
		hInstance,
		NULL);

	ShowWindow(wnd, nCmdShow);

	//Initialize DirectX
	InitD3D(wnd);

	LoadTextures();
	StartLevel();

	MSG msg;

	static LARGE_INTEGER last, end;
	static LARGE_INTEGER s_frequency;
	long perfCountFrequency;
	QueryPerformanceFrequency(&s_frequency);
	QueryPerformanceCounter(&last);
	perfCountFrequency = (LONG)s_frequency.QuadPart;
	float dt = 0;
	
	while (TRUE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, TRUE))
		{
			TranslateMessage(&msg);

			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				break;
			}
		}	

		mouseDown = 0;

		if ((GetKeyState(VK_LBUTTON) & 0x80) != 0)
		{
			mouseDown = 1;
		}
		else
		{
			mouseDown = 0;
		}

	
		POINT mouse;
		GetCursorPos(&mouse);
		ScreenToClient(wnd, &mouse);

		mouseX = mouse.x;

		//cap the mouse position to stay within window bounds. 
		if (mouse.x< 0)
		{
			mouseX = 0;
		}
		else if(mouse.x> SCREENWIDTH)
		{
			mouseX = SCREENWIDTH;
		}
		
		mouseY = mouse.y;
		if (mouseY < 0)
		{
			mouseY = 0;
		}
		else if (mouseY> SCREENHEIGHT)
		{
			mouseY = SCREENHEIGHT;
		}
		
		UpdateRender(.03f);

		QueryPerformanceCounter(&end);
	
		long countDiff =(long) (end.QuadPart - last.QuadPart);
	
		int fps =  perfCountFrequency / countDiff;

		int MSPerFrame =(1000*countDiff) / perfCountFrequency;
		dt = (float)MSPerFrame/1000.0f;

		last = end;

		if (dt < .03f)
		{
			Sleep((DWORD)(.03f - dt) * 1000);
		}
		
		wchar_t buffer[256];
		wsprintf(buffer, L"%dms  fps %d \n", MSPerFrame, fps);

		mouseWasDownLastFrame = mouseDown;

	//	OutputDebugString(buffer);
	}
	
	//remove DirectX resources 
	CleanD3D();
	return msg.wParam;
}