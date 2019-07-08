#pragma once


#include "Direct3D.cpp"

//need this to open the console for debug output
#pragma warning(disable:4996)
















Direct3D direct3D;
Shader shader;
BatchedPrimitives batchedPrimitives;
Texture texture[16];
Renderable renderables[10000];
int activeRenderables = 0;

DirectX::XMMATRIX projectionMatrix;
DirectX::XMMATRIX worldMatrix;
DirectX::XMMATRIX orthoMatrix;

bool running = true;

int width = 1280;
int height = 720;

struct KeyState
{
	bool currentKeyState[256];
	bool previousKeyState[256];
};

KeyState keyState;

static LRESULT CALLBACK
window_callback(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;

	switch (msg)
	{
	case WM_SIZE:
	{
		RECT rect = {};
		GetClientRect(window, &rect);
		//glViewPort(0, 0, rect.right, rect.bottom);
	}
	break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		unsigned int VKCode = wparam;
		bool wasDown = ((lparam & (1 << 30)) != 0);
		bool isDown = ((lparam & (1 << 31)) == 0);
		std::cout << (char)VKCode << " wasDown: " << wasDown << std::endl;
		std::cout << (char)VKCode << " isDown: " << isDown << std::endl;
		
		keyState.currentKeyState[VKCode] = isDown;
		keyState.previousKeyState[VKCode] = wasDown;
	}
	break;

	default:
		result = DefWindowProcA(window, msg, wparam, lparam);
		break;
	}
	return result;
}
static void
fatal_error(char *msg)
{
	MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONEXCLAMATION);
	exit(EXIT_FAILURE);
}
static HWND
create_window(HINSTANCE inst)
{
	//Create a window and register it, basically having windows call " new window "
	WNDCLASSA window_class = {};
	window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	window_class.lpfnWndProc = window_callback;
	window_class.hInstance = inst;
	window_class.hCursor = LoadCursor(0, IDC_ARROW);
	window_class.hbrBackground = 0;
	window_class.lpszClassName = "Game";

	if (!RegisterClassA(&window_class))
	{
		fatal_error((char*)"Failed to register window.");
	}
	//Use the rect to set the desired window size
	RECT rect = {};
	rect.right = 1280;
	rect.bottom = 720;
	DWORD window_style = WS_OVERLAPPEDWINDOW;
	AdjustWindowRect(&rect, window_style, false);

	//now windows will use that new window to actually make it
	HWND window = CreateWindowExA(
		0,
		window_class.lpszClassName,
		"Game",
		window_style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		0,
		0,
		inst,
		0);

	if (!window)
	{
		fatal_error((char*)"Failed to create window.");
	}

	return window;
}
int WINAPI 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int show)
{

	HWND window = create_window(hInstance);


	ShowWindow(window, show);
	UpdateWindow(window);

	//Allocate a console for debug output, even with SetFocus(window) it will start on top though :(
	AllocConsole();
	freopen("CONOUT$", "w", stdout);



	if(!InitializeDirectX(&direct3D, width, height, window))
	{
		MessageBox(window, "Could not initialize Direct3D", "Error", MB_OK);
		return 0;
	}

	//TODO: set viewport somewhere else?
	D3D11_VIEWPORT viewport;

	// Setup the viewport for rendering.
	viewport.Width = (float)width;
	viewport.Height = (float)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	// Create the viewport.
	direct3D.deviceContext->RSSetViewports(1, &viewport);

	float fieldOfView, screenAspect;

	// Setup the projection matrix.
	fieldOfView = 3.141592654f / 4.0f;
	screenAspect = (float)width / (float)height;

	// Create the projection matrix for 3D rendering.
	//projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, /*screenNear*/ 1.0f, /*screenDepth*/ 10.0f);
	projectionMatrix = DirectX::XMMatrixIdentity();

	// Initialize the world matrix to the identity matrix.
	worldMatrix = DirectX::XMMatrixIdentity();

	// Create an orthographic projection matrix for 2D rendering.
	orthoMatrix = DirectX::XMMatrixOrthographicLH((float)width, (float)height, /*screenNear*/ 1.0f, /*screenDepth*/ 10.0f);






	if(!InitializeShader(&shader, direct3D.device, window, L"textureVertex.txt",L"texturePixel.txt"))
	{
		return 0;
	}


	if(!InitializeBatchedPrimitivesBuffers(&direct3D, &batchedPrimitives))
	{
		return 0;
	}

	if (!InitializeTexture(&texture[0], &direct3D, "test.bmp"))
	{
		return 0;
	}
	if (!InitializeTexture(&texture[1], &direct3D, "testblue.bmp"))
	{
		return 0;
	}

	SetShaderParameters(&direct3D, &shader, worldMatrix, worldMatrix, orthoMatrix);
	BindTexture(&direct3D, texture[0].m_textureView, 0, 1);
	BindTexture(&direct3D, texture[1].m_textureView, 1, 1);

	//InitializeShader(device)
	float startX = -640.0f;
	float startY = 360.0f;
	for(int i = 0; i < 10000; i ++)
	{
		//TODO: pass texture* and get its ID that way since it will be easier to read
		Initialize(&renderables[i], DirectX::XMFLOAT4(startX, startY, 5.0f, 1.0f), DirectX::XMFLOAT2(8.0f, 8.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), i % 2 == 0 ? 0 : 1);
		activeRenderables++;
		startX += 8.0f;
		if (startX >= 640.0f)
		{
			startX = -640.0f;
			startY -= 8.0f;
		}
	}

	while(running)
	{
		MSG msg;
		while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
			{
				running = false;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			BeginScene(&direct3D, 1.0f, 0.0f, 0.0f, 1.0f);
			//SetShaderParameters(&direct3D, &shader, worldMatrix, worldMatrix, projectionMatrix, texture.m_textureView);

			BindBatchedPrimitiveBuffer(&direct3D, &batchedPrimitives);

			for(int i = 0; i < 10000; i++)
			{
				SubmitRenderable(&batchedPrimitives, &renderables[i]);
			}

			UnbindBatchedPrimitiveBuffer(&direct3D, &batchedPrimitives);

			BindBuffers(&batchedPrimitives, &direct3D);
			BindShader(&direct3D, &shader);
			DrawPrimitives(&direct3D, batchedPrimitives.activeIndexCount);

			/*BindBuffers(&batchedPrimitives, &direct3D);
			BindShader(&direct3D, &shader);
			DrawPrimitives(&direct3D, 12);*/
			EndScene(&direct3D);
		}
		//SwapBuffers(gldc);
	}
	return 0;
}