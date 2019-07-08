#pragma once

#include <DirectXMath.h>



struct Renderable
{
	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT2 size;
	DirectX::XMFLOAT2 uv1;
	DirectX::XMFLOAT2 uv2;
	DirectX::XMFLOAT2 uv3;
	DirectX::XMFLOAT2 uv4;
	DirectX::XMFLOAT4 colorVector; //TODO: remove this and use uint
	unsigned int color;
	int texture;
	bool active;

};

static void 
SetColor(Renderable* renderable, const DirectX::XMFLOAT4& color)
{
	int r = color.x * 255.0f;
	int g = color.y * 255.0f;
	int b = color.z * 255.0f;
	int a = color.w * 255.0f;

	renderable->color = a << 24 | b << 16 | g << 8 | r;
	renderable->colorVector = color;
}

static void
Initialize(Renderable* renderable, const DirectX::XMFLOAT4& position, const DirectX::XMFLOAT2& size, const DirectX::XMFLOAT4& color, int texture)
{
	SetColor(renderable, color);

	renderable->texture = texture;
	renderable->active = true;
	renderable->position = position;
	renderable->size = size;
	renderable->colorVector = color;
}
/*
static void
Initialize(Renderable* renderable, const DirectX::XMFLOAT4& position, const DirectX::XMFLOAT2& size, Texture* texture)
{

}*/