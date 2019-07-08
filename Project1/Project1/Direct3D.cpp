#pragma once

#pragma comment(lib, "d3d11.lib") // contains all Direct3D functionality
#pragma comment(lib, "dxgi.lib") //contains tools to get info about mointor refresh rate
#pragma comment(lib, "d3dcompiler.lib") //contains functionality for compiling shaders
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>
#include <Windows.h>
#include <d3dcompiler.h>

#include "Utilities.cpp"
#include "Renderables.cpp"
//TODO: this should probably be double checked later and separated when we have a better idea of what we need and where
struct Direct3D
{
	bool fullscreen = false; //TODO: this isnt working
	bool vsync = false;
	int videoCardMemory = 0;
	char videoCardDescription[128];
	IDXGISwapChain* swapChain = 0;
	ID3D11Device* device = 0;
	ID3D11DeviceContext* deviceContext = 0;
	ID3D11RenderTargetView* renderTargetView = 0;
	ID3D11Texture2D* depthStencilBuffer = 0;
	ID3D11DepthStencilState* depthStencilState = 0;
	ID3D11DepthStencilView* depthStencilView = 0;
	ID3D11RasterizerState* rasterState = 0;
};

struct Shader
{
	ID3D11VertexShader* m_vertexShader = 0;
	ID3D11PixelShader* m_pixelShader = 0;
	ID3D11InputLayout* m_layout = 0;
	ID3D11Buffer* m_matrixBuffer = 0;
	ID3D11SamplerState* m_sampleState = 0;
};

struct Texture
{
	unsigned char* m_data = 0;
	ID3D11Texture2D* m_texture = 0;
	ID3D11ShaderResourceView* m_textureView = 0;
	int width;
	int height;
};

struct VertexData
{
	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT2 texture;
	DirectX::XMFLOAT4 color;
	int textureID;
};

struct BatchedPrimitives
{
	ID3D11Buffer* vertexBuffer = 0;
	ID3D11Buffer* indexBuffer = 0;
	VertexData* vertices = 0;
	int vertexCount;
	int indexCount;
	D3D11_MAPPED_SUBRESOURCE mappedResource; // not sure if this should be here
	int activeIndexCount;
};

struct MatrixBufferType
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
};


static void 
BeginScene(Direct3D* direct3D, float red, float green, float blue, float alpha)
{
	float color[4];


	// Setup the color to clear the buffer to.
	color[0] = red;
	color[1] = green;
	color[2] = blue;
	color[3] = alpha;

	// Clear the back buffer.
	direct3D->deviceContext->ClearRenderTargetView(direct3D->renderTargetView, color);

	// Clear the depth buffer.
	direct3D->deviceContext->ClearDepthStencilView(direct3D->depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	return;
}


static void 
EndScene(Direct3D* direct3D)
{
	// Present the back buffer to the screen since rendering is complete.
	if (direct3D->vsync)
	{
		// Lock to screen refresh rate.
		direct3D->swapChain->Present(1, 0);
	}
	else
	{
		// Present as fast as possible.
		direct3D->swapChain->Present(0, 0);
	}

	return;
}

static void 
BindBuffers(BatchedPrimitives* batchedPrimitives, Direct3D* direct3D)
{
	unsigned int stride;
	unsigned int offset;

	stride = sizeof(VertexData);
	offset = 0;

	direct3D->deviceContext->IASetVertexBuffers(0, 1, &batchedPrimitives->vertexBuffer, &stride, &offset);
	direct3D->deviceContext->IASetIndexBuffer(batchedPrimitives->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	direct3D->deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//TODO: rename to bind shader and remove DrawIndexed
static void
BindShader(Direct3D* direct3D, Shader* shader)
{
	ID3D11DeviceContext* deviceContext = direct3D->deviceContext;
	//A pointer to the input-layout object (see ID3D11InputLayout), which describes the input buffers that will be read by the IA stage.
	deviceContext->IASetInputLayout(shader->m_layout);

	//finally something i like :^)
	deviceContext->VSSetShader(shader->m_vertexShader, NULL, 0);
	deviceContext->PSSetShader(shader->m_pixelShader, NULL, 0);

	deviceContext->PSSetSamplers(0, 1, &shader->m_sampleState);	
}

static void
DrawPrimitives(Direct3D* direct3D, unsigned int indexCount)
{
	ID3D11DeviceContext* deviceContext = direct3D->deviceContext;
	deviceContext->DrawIndexed(indexCount, 0, 0);
}

//TODO: maybe we dont need functions at all, should probably just declare the device context globally and roll with it
static void
BindTexture(Direct3D* direct3D, ID3D11ShaderResourceView* texture, unsigned int slot, unsigned int views)
{
	direct3D->deviceContext->PSSetShaderResources(slot, views, &texture);
}

//TODO: We should maybe make functions that set these individually 
static bool
SetShaderParameters(Direct3D* direct3D, Shader* shader, DirectX::XMMATRIX worldMatrix, DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* data;
	unsigned int bufferNumber;

	// Transpose the matrices to prepare them for the shader
	// TODO: this is from the tutorial, why wouldnt directx expect its own matrices to be set the correct way?

	worldMatrix = DirectX::XMMatrixTranspose(worldMatrix);
	viewMatrix = DirectX::XMMatrixTranspose(viewMatrix);
	projectionMatrix = DirectX::XMMatrixTranspose(projectionMatrix);

	result = direct3D->deviceContext->Map(shader->m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	data = (MatrixBufferType*)mappedResource.pData;

	data->world = worldMatrix;
	data->view = viewMatrix;
	data->projection = projectionMatrix;

	direct3D->deviceContext->Unmap(shader->m_matrixBuffer, 0);

	/// Set the position of the constant buffer in the vertex shader
	bufferNumber = 0;

	// update the buffer in the vertex shader with the new values

	direct3D->deviceContext->VSSetConstantBuffers(bufferNumber, 1, &shader->m_matrixBuffer);


	return true;
}

static bool
DeinitializeTexture(Texture* texture)
{
	// Release the texture view resource.
	if (texture->m_textureView)
	{
		texture->m_textureView->Release();
		texture->m_textureView = 0;
	}

	// Release the texture.
	if (texture->m_texture)
	{
		texture->m_texture->Release();
		texture->m_texture = 0;
	}

}

static bool 
InitializeTexture(Texture* texture, Direct3D* direct3D, const char* filename)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	HRESULT hResult;
	unsigned int rowPitch;

	unsigned char* textureData = LoadBMP(filename, texture->width, texture->height);


	// Setup the description of the texture.
	textureDesc.Height = texture->height;
	textureDesc.Width = texture->width;
	textureDesc.MipLevels = 0;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	// Create the empty texture.
	hResult = direct3D->device->CreateTexture2D(&textureDesc, NULL, &texture->m_texture);
	if (FAILED(hResult))
	{
		return false;
	}

	// Set the row pitch of the targa image data.
	rowPitch = (texture->width * 4) * sizeof(unsigned char);

	direct3D->deviceContext->UpdateSubresource(texture->m_texture, 0, NULL, textureData, rowPitch, 0);

	// Setup the shader resource view description.
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	// Create the shader resource view for the texture.
	hResult = direct3D->device->CreateShaderResourceView(texture->m_texture, &srvDesc, &texture->m_textureView);
	if (FAILED(hResult))
	{
		return false;
	}

	// Generate mipmaps for this texture.
	direct3D->deviceContext->GenerateMips(texture->m_textureView);

	delete[] textureData;
	return true;
}

static void 
BindBatchedPrimitiveBuffer(Direct3D* direct3D, BatchedPrimitives* batchedPrimitives)
{
	ZeroMemory(&batchedPrimitives->mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE)); // not sure if needed
	direct3D->deviceContext->Map(batchedPrimitives->vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &batchedPrimitives->mappedResource);
	//Reset counters
	batchedPrimitives->activeIndexCount = 0;
	int shit = 5;
}

static void 
UnbindBatchedPrimitiveBuffer(Direct3D* direct3D, BatchedPrimitives* batchedPrimitives)
{
	memcpy(batchedPrimitives->mappedResource.pData, batchedPrimitives->vertices, sizeof(VertexData) * batchedPrimitives->vertexCount);
	//  Reenable GPU access to the vertex buffer data.
	direct3D->deviceContext->Unmap(batchedPrimitives->vertexBuffer, 0);
}

static void 
SubmitRenderable(BatchedPrimitives* batchedPrimitives, Renderable* renderable)
{
	const DirectX::XMFLOAT4& position = renderable->position; //glm::vec4(renderable->GetPosition(), 1.0f);
	const DirectX::XMFLOAT2& size = renderable->size;
	const DirectX::XMFLOAT2 halfSize = DirectX::XMFLOAT2(size.x * 0.5f, size.y * 0.5f);
	const unsigned int color = renderable->color;
	const DirectX::XMFLOAT4& colorVec = renderable->colorVector;
	//const std::vector<glm::vec2>& uv = renderable->GetUVs();
	const DirectX::XMFLOAT2& uv1 = renderable->uv1;
	const DirectX::XMFLOAT2& uv2 = renderable->uv2;
	const DirectX::XMFLOAT2& uv3 = renderable->uv3;
	const DirectX::XMFLOAT2& uv4 = renderable->uv4;
	const int tid = renderable->texture;

	//TODO: use 4 vertices
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].position = DirectX::XMFLOAT4(position.x - halfSize.x, position.y - halfSize.y, position.z, 1.0f);  // Bottom left.
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].texture = DirectX::XMFLOAT2(0.0f, 1.0f);
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].textureID = tid;
	batchedPrimitives->activeIndexCount++;
	

	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].position = DirectX::XMFLOAT4(position.x - halfSize.x, position.y + halfSize.y, position.z, 1.0f);  // Top Left.
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].texture = DirectX::XMFLOAT2(0.0f, 0.0f);
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].textureID = tid;
	batchedPrimitives->activeIndexCount++;

	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].position = DirectX::XMFLOAT4(position.x + halfSize.x, position.y + halfSize.y, position.z, 1.0f);  // Top right.
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].texture = DirectX::XMFLOAT2(1.0f, 0.0f);
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].textureID = tid;
	batchedPrimitives->activeIndexCount++;

	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].position = DirectX::XMFLOAT4(position.x - halfSize.x, position.y - halfSize.y, position.z, 1.0f);  // Bottom left.
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].texture = DirectX::XMFLOAT2(0.0f, 1.0f);
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].textureID = tid;
	batchedPrimitives->activeIndexCount++;

	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].position = DirectX::XMFLOAT4(position.x + halfSize.x, position.y + halfSize.y, position.z, 1.0f);  // Top right.
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].texture = DirectX::XMFLOAT2(1.0f, 0.0f);
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].textureID = tid;
	batchedPrimitives->activeIndexCount++;

	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].position = DirectX::XMFLOAT4(position.x + halfSize.x, position.y - halfSize.y, position.z, 1.0f);  // Bottom right.
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].texture = DirectX::XMFLOAT2(1.0f, 1.0f);
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	batchedPrimitives->vertices[batchedPrimitives->activeIndexCount].textureID = tid;
	batchedPrimitives->activeIndexCount++;

	//batchedPrimitives->activeIndexCount += 6;
}

static bool 
InitializeBatchedPrimitivesBuffers(Direct3D* direct3D, BatchedPrimitives* batchedPrimitives)
{
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_BUFFER_DESC indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;
	D3D11_SUBRESOURCE_DATA indexData;
	HRESULT result;

	unsigned int maxSprites = 10000;

	batchedPrimitives->vertexCount = maxSprites *6;
	batchedPrimitives->indexCount = maxSprites *6;

	batchedPrimitives->vertices = new VertexData[batchedPrimitives->vertexCount];
	indices = new unsigned long[batchedPrimitives->indexCount];

	//DirectX is CLOCKWISE ORDER for triangles
	//Textures are top left = 0,0 bottom right = 1,1
	/*vertices[0].position = DirectX::XMFLOAT3(-16.0f, -16.0f, 5.0f);  // Bottom left.
	vertices[0].texture = DirectX::XMFLOAT2(0.0f, 1.0f);
	vertices[0].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	vertices[0].textureID = 1;

	vertices[1].position = DirectX::XMFLOAT3(-16.0f, 16.0f, 5.0f);  // Top Left.
	vertices[1].texture = DirectX::XMFLOAT2(0.0f, 0.0f);
	vertices[1].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	vertices[1].textureID = 1;

	vertices[2].position = DirectX::XMFLOAT3(16.0f, 16.0f, 5.0f);  // Top right.
	vertices[2].texture = DirectX::XMFLOAT2(1.0f, 0.0f);
	vertices[2].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	vertices[2].textureID = 1;

	vertices[3].position = DirectX::XMFLOAT3(-16.0f, -16.0f, 5.0f);  // Bottom left.
	vertices[3].texture = DirectX::XMFLOAT2(0.0f, 1.0f);
	vertices[3].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	vertices[3].textureID = 1;

	vertices[4].position = DirectX::XMFLOAT3(16.0f, 16.0f, 5.0f);  // Top right.
	vertices[4].texture = DirectX::XMFLOAT2(1.0f, 0.0f);
	vertices[4].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	vertices[4].textureID = 1;

	vertices[5].position = DirectX::XMFLOAT3(16.0f, -16.0f, 5.0f);  // Bottom right.
	vertices[5].texture = DirectX::XMFLOAT2(1.0f, 1.0f);
	vertices[5].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	vertices[5].textureID = 1;

	vertices[6].position = DirectX::XMFLOAT3(-16.0f + 50.0f, -16.0f, 5.0f);  // Bottom left.
	vertices[6].texture = DirectX::XMFLOAT2(0.0f, 1.0f);
	vertices[6].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	vertices[6].textureID = 0;

	vertices[7].position = DirectX::XMFLOAT3(-16.0f + 50.0f, 16.0f, 5.0f);  // Top Left.
	vertices[7].texture = DirectX::XMFLOAT2(0.0f, 0.0f);
	vertices[7].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	vertices[7].textureID = 0;

	vertices[8].position = DirectX::XMFLOAT3(16.0f + 50.0f, 16.0f, 5.0f);  // Top right.
	vertices[8].texture = DirectX::XMFLOAT2(1.0f, 0.0f);
	vertices[8].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	vertices[8].textureID = 0;

	vertices[9].position = DirectX::XMFLOAT3(-16.0f + 50.0f, -16.0f, 5.0f);  // Bottom left.
	vertices[9].texture = DirectX::XMFLOAT2(0.0f, 1.0f);
	vertices[9].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	vertices[9].textureID = 0;

	vertices[10].position = DirectX::XMFLOAT3(16.0f + 50.0f, 16.0f, 5.0f);  // Top right.
	vertices[10].texture = DirectX::XMFLOAT2(1.0f, 0.0f);
	vertices[10].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	vertices[10].textureID = 0;

	vertices[11].position = DirectX::XMFLOAT3(16.0f + 50.0f, -16.0f, 5.0f);  // Bottom right.
	vertices[11].texture = DirectX::XMFLOAT2(1.0f, 1.0f);
	vertices[11].color = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	vertices[11].textureID = 0;
	*/
	// TODO: turn into loop and use 4 vertices per quad instead of 6
	// Load the index array with data.
	indices[0] = 0;  // Bottom left.
	indices[1] = 1;  // Top middle.
	indices[2] = 2;  // Bottom right.
	indices[3] = 3;
	indices[4] = 4;
	indices[5] = 5;

	indices[6]  = 6;  // Bottom left.
	indices[7]  = 7;  // Top middle.
	indices[8]  = 8;  // Bottom right.
	indices[9]  = 9;
	indices[10] = 10;
	indices[11] = 11;

	for(int i = 0; i < maxSprites * 6; i++)
	{
		indices[i] = i;
	}

	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(VertexData) * batchedPrimitives->vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = batchedPrimitives->vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	result = direct3D->device->CreateBuffer(&vertexBufferDesc, &vertexData, &batchedPrimitives->vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * batchedPrimitives->indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = direct3D->device->CreateBuffer(&indexBufferDesc, &indexData, &batchedPrimitives->indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Release the arrays now that the vertex and index buffers have been created and loaded.
/*	delete[] vertices;
	vertices = 0;*/

	delete[] indices;
	indices = 0;

	return true;
}

static void 
DeinitalizeBatchedPrimitives(BatchedPrimitives* batchedPrimitives)
{
	batchedPrimitives->indexBuffer->Release();
	batchedPrimitives->indexBuffer = 0;
	batchedPrimitives->vertexBuffer->Release();
	batchedPrimitives->vertexBuffer = 0;
	delete[] batchedPrimitives->vertices;
}


static bool
DeinitializeShader(Shader* shader)
{
	// Release the sampler state.
	if (shader->m_sampleState)
	{
		shader->m_sampleState->Release();
		shader->m_sampleState = 0;
	}

	// Release the matrix constant buffer.
	if (shader->m_matrixBuffer)
	{
		shader->m_matrixBuffer->Release();
		shader->m_matrixBuffer = 0;
	}

	// Release the layout.
	if (shader->m_layout)
	{
		shader->m_layout->Release();
		shader->m_layout = 0;
	}

	// Release the pixel shader.
	if (shader->m_pixelShader)
	{
		shader->m_pixelShader->Release();
		shader->m_pixelShader = 0;
	}

	// Release the vertex shader.
	if (shader->m_vertexShader)
	{
		shader->m_vertexShader->Release();
		shader->m_vertexShader = 0;
	}

	return true;
}

static bool 
InitializeShader(Shader* shader, ID3D11Device* device, HWND hwnd, const wchar_t* vsFilename, const wchar_t* psFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	ID3D10Blob* vertexShaderBuffer;
	ID3D10Blob* pixelShaderBuffer;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[4];
	unsigned int numElements;
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc; // this i guess is the equivalent to openGL when you set how you sample a texture

	// Initialize the pointers this function will use to null.
	errorMessage = 0;
	vertexShaderBuffer = 0;
	pixelShaderBuffer = 0;

	// Compile the vertex shader code.
	// Pretty neato that you can specify which function to enter
	result = D3DCompileFromFile(vsFilename, NULL, NULL, "main", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &vertexShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if (errorMessage)
		{
			char* compileErrors;
			compileErrors = (char*)(errorMessage->GetBufferPointer());
			errorMessage->Release();
			errorMessage = 0;
			MessageBox(hwnd, compileErrors, (LPCSTR)vsFilename, MB_OK);
		}
		// If there was  nothing in the error message then it simply could not find the shader file itself.
		else
		{
			MessageBox(hwnd, (LPCSTR)vsFilename, "Missing Shader File", MB_OK);
		}

		return false;
	}

	// Compile the pixel shader code.
	result = D3DCompileFromFile(psFilename, NULL, NULL, "main", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &pixelShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if (errorMessage)
		{
			char* compileErrors;
			compileErrors = (char*)(errorMessage->GetBufferPointer());
			errorMessage->Release();
			errorMessage = 0;
			MessageBox(hwnd, compileErrors, (LPCSTR)psFilename, MB_OK);
		}
		// If there was nothing in the error message then it simply could not find the file itself.
		else
		{
			MessageBox(hwnd, (LPCSTR)psFilename, "Missing Shader File", MB_OK);
		}

		return false;
	}

	// Create the vertex shader from the buffer.
	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &shader->m_vertexShader);
	if (FAILED(result))
	{
		return false;
	}

	// Create the pixel shader from the buffer.
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &shader->m_pixelShader);
	if (FAILED(result))
	{
		return false;
	}

	// Create the vertex input layout description.
// This setup needs to match the VertexType stucture in the  and in the shader.
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "TEXCOORD";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	polygonLayout[2].SemanticName = "COLOR";
	polygonLayout[2].SemanticIndex = 0;
	polygonLayout[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[2].InputSlot = 0;
	polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[2].InstanceDataStepRate = 0;

	polygonLayout[3].SemanticName = "TEXTUREID";
	polygonLayout[3].SemanticIndex = 0;
	polygonLayout[3].Format = DXGI_FORMAT_R8_SINT;
	polygonLayout[3].InputSlot = 0;
	polygonLayout[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[3].InstanceDataStepRate = 0;

	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// Create the vertex input layout.
	result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(),
		vertexShaderBuffer->GetBufferSize(), &shader->m_layout);
	if (FAILED(result))
	{
		return false;
	}

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vertexShaderBuffer->Release();
	vertexShaderBuffer = 0;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = 0;

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = device->CreateBuffer(&matrixBufferDesc, NULL, &shader->m_matrixBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Create a texture sampler state description.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	result = device->CreateSamplerState(&samplerDesc, &shader->m_sampleState);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

//MEGA TODO: this one probably contains a bunch of stuff we want to initialize in other ways
static bool 
InitializeDirectX(Direct3D* direct3D, unsigned int width, unsigned int height, HWND window)
{
	HRESULT result;
	IDXGIFactory* factory; //factory used to create everything
	IDXGIAdapter* adapter; //the video card adapter
	IDXGIOutput* adapterOutput; //the monitor, or whatever else you fancy to use as output
	unsigned int numModes, i, numerator, denominator; //used for refresh rate
	unsigned long long stringLength;
	DXGI_MODE_DESC* displayModeList;
	DXGI_ADAPTER_DESC adapterDesc;
	int error;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	D3D_FEATURE_LEVEL featureLevel;
	ID3D11Texture2D* backBufferPtr;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	D3D11_RASTERIZER_DESC rasterDesc;

	//NOTES: (Johan) apparently this entire thing below is because DirectX needs to be passed the proper refresh rate based on individual GPUs and the result is that....
	// "If we don't do this and just set the refresh rate to a default value which may not exist on all computers then DirectX will respond by performing a blit instead of a buffer flip which will degrade performance and give us annoying errors in the debug output. "
	//TODO: see if all of this is really needed

	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	if (FAILED(result))
	{
		return false;
	}

	// Use the factory to create an adapter for the primary graphics interface (video card).
	result = factory->EnumAdapters(0, &adapter);
	if (FAILED(result))
	{
		return false;
	}

	// Enumerate the primary adapter output (monitor).
	//(Johan) Enumeration in directX seems to just be what they call when you list numbers of monitors, video cards etc
	result = adapter->EnumOutputs(0, &adapterOutput);
	if (FAILED(result))
	{
		return false;
	}

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if (FAILED(result))
	{
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	displayModeList = new DXGI_MODE_DESC[numModes];
	if (!displayModeList)
	{
		return false;
	}

	// Now fill the display mode list structures.
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	if (FAILED(result))
	{
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	for (i = 0; i < numModes; i++)
	{
		if (displayModeList[i].Width == (unsigned int)width)
		{
			if (displayModeList[i].Height == (unsigned int)height)
			{
				numerator = displayModeList[i].RefreshRate.Numerator;
				denominator = displayModeList[i].RefreshRate.Denominator;
			}
		}
	}
	// Get the adapter (video card) description.
	result = adapter->GetDesc(&adapterDesc);
	if (FAILED(result))
	{
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	direct3D->videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	error = wcstombs_s(&stringLength, direct3D->videoCardDescription, 128, adapterDesc.Description, 128);
	if (error != 0)
	{
		return false;
	}

	// Release the display mode list.
	delete[] displayModeList;
	displayModeList = 0;

	// Release the adapter output.
	adapterOutput->Release();
	adapterOutput = 0;

	// Release the adapter.
	adapter->Release();
	adapter = 0;

	// Release the factory.
	factory->Release();
	factory = 0;


	// Initialize the swap chain description.
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Set to a single back buffer.
	swapChainDesc.BufferCount = 1;

	// Set the width and height of the back buffer.
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;

	// Set regular 32-bit surface for the back buffer.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;



	// Set the refresh rate of the back buffer.
	if (direct3D->vsync)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = window;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	if (direct3D->fullscreen)
	{
		swapChainDesc.Windowed = false;
	}
	else
	{
		swapChainDesc.Windowed = true;
	}

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;

	// Set the feature level to DirectX 11.
	featureLevel = D3D_FEATURE_LEVEL_11_0;

	// Create the swap chain, Direct3D device, and Direct3D device context.
	result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &featureLevel, 1,
		D3D11_SDK_VERSION, &swapChainDesc, &direct3D->swapChain, &direct3D->device, NULL, &direct3D->deviceContext);
	if (FAILED(result))
	{
		return false;
	}

	// Get the pointer to the back buffer.
	result = direct3D->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	if (FAILED(result))
	{
		return false;
	}

	// Create the render target view with the back buffer pointer.
	result = direct3D->device->CreateRenderTargetView(backBufferPtr, NULL, &direct3D->renderTargetView);
	if (FAILED(result))
	{
		return false;
	}

	// Release pointer to the back buffer as we no longer need it.
	backBufferPtr->Release();
	backBufferPtr = 0;

	// Initialize the description of the depth buffer.
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = width;
	depthBufferDesc.Height = height;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	// Create the texture for the depth buffer using the filled out description.
	result = direct3D->device->CreateTexture2D(&depthBufferDesc, NULL, &direct3D->depthStencilBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Initialize the description of the stencil state.
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	result = direct3D->device->CreateDepthStencilState(&depthStencilDesc, &direct3D->depthStencilState);
	if (FAILED(result))
	{
		return false;
	}

	// Set the depth stencil state.
	direct3D->deviceContext->OMSetDepthStencilState(direct3D->depthStencilState, 1);

	// Initialize the depth stencil view.
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	result = direct3D->device->CreateDepthStencilView(direct3D->depthStencilBuffer, &depthStencilViewDesc, &direct3D->depthStencilView);
	if (FAILED(result))
	{
		return false;
	}

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	direct3D->deviceContext->OMSetRenderTargets(1, &direct3D->renderTargetView, direct3D->depthStencilView);


	// Setup the raster description which will determine how and what polygons will be drawn.
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	result = direct3D->device->CreateRasterizerState(&rasterDesc, &direct3D->rasterState);
	if (FAILED(result))
	{
		return false;
	}

	// Now set the rasterizer state.
	direct3D->deviceContext->RSSetState(direct3D->rasterState);


	ID3D11BlendState* d3dBlendState;
	D3D11_BLEND_DESC omDesc;
	ZeroMemory(&omDesc,sizeof(D3D11_BLEND_DESC));
	
	omDesc.RenderTarget[0].BlendEnable = true;
	omDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;


	result = direct3D->device->CreateBlendState(&omDesc, &d3dBlendState);
	if (FAILED(result))
	{
		return false;
	}
	direct3D->deviceContext->OMSetBlendState(d3dBlendState, 0, 0xffffffff);


	return true;
}