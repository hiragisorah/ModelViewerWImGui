#include "graphics.h"
#include "window.h"

#if defined(DEBUG) || defined(_DEBUG)
constexpr DWORD SHADER_FLAGS = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;
#else
constexpr DWORD SHADER_FLAGS = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

ComPtr<ID3D11Device> device_;
ComPtr<ID3D11DeviceContext> context_;
ComPtr<IDXGISwapChain> swap_chain_;

ComPtr<ID3D11RenderTargetView> back_buffer_rtv_;
ComPtr<ID3D11DepthStencilView> dsv_;

struct Shader
{
	ComPtr<ID3D11VertexShader> vertex_shader_;
	ComPtr<ID3D11GeometryShader> geometry_shader_;
	ComPtr<ID3D11HullShader> hull_shader_;
	ComPtr<ID3D11DomainShader> domain_shader_;
	ComPtr<ID3D11PixelShader> pixel_shader_;
	ComPtr<ID3D11InputLayout> input_layout_;
	std::vector<ComPtr<ID3D11Buffer>> constant_buffer_;
};

std::shared_ptr<Shader> shader_;

D3D11_VIEWPORT viewport_;
RenderingObject rendering_object_;

void Graphics::Initalize(void)
{
	{
		// デバイスとスワップチェーンの作成
		DXGI_SWAP_CHAIN_DESC sd;
		memset(&sd, 0, sizeof(sd));
		sd.BufferCount = 1;
		sd.BufferDesc.Width = Window::width();
		sd.BufferDesc.Height = Window::height();
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = Window::hwnd();
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = true;

		D3D_FEATURE_LEVEL feature_levels = D3D_FEATURE_LEVEL_11_0;
		D3D_FEATURE_LEVEL * feature_level = nullptr;

		D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			0, &feature_levels, 1, D3D11_SDK_VERSION, &sd, &swap_chain_, &device_,
			feature_level, &context_);
	}

	{
		ComPtr<ID3D11Texture2D> tex_2d;

		// バックバッファーテクスチャーを取得
		swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&tex_2d);

		// そのテクスチャーに対しレンダーターゲットビュー(RTV)を作成
		device_->CreateRenderTargetView(tex_2d.Get(), nullptr, back_buffer_rtv_.GetAddressOf());
	}

	{
		ComPtr<ID3D11Texture2D> tex_2d;

		//深度マップテクスチャをレンダーターゲットにする際のデプスステンシルビュー用のテクスチャーを作成
		D3D11_TEXTURE2D_DESC tex_desc = {};
		tex_desc.Width = Window::width();
		tex_desc.Height = Window::height();
		tex_desc.MipLevels = 1;
		tex_desc.ArraySize = 1;
		tex_desc.Format = DXGI_FORMAT_D32_FLOAT;
		tex_desc.SampleDesc.Count = 1;
		tex_desc.SampleDesc.Quality = 0;
		tex_desc.Usage = D3D11_USAGE_DEFAULT;
		tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		tex_desc.CPUAccessFlags = 0;
		tex_desc.MiscFlags = 0;

		device_->CreateTexture2D(&tex_desc, nullptr, tex_2d.GetAddressOf());
		device_->CreateDepthStencilView(tex_2d.Get(), nullptr, dsv_.GetAddressOf());
	}

	{
		auto & vp = viewport_;

		vp.Width = Window::width<float>();
		vp.Height = Window::height<float>();
		vp.MinDepth = 0.f;
		vp.MaxDepth = 1.f;
		vp.TopLeftX = 0.f;
		vp.TopLeftY = 0.f;
	}

	LoadShader("test");
}

DXGI_FORMAT GetDxgiFormat(D3D_REGISTER_COMPONENT_TYPE type, BYTE mask)
{
	if (mask == 0x0F)
	{
		// xyzw
		switch (type)
		{
		case D3D_REGISTER_COMPONENT_FLOAT32:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case D3D_REGISTER_COMPONENT_UINT32:
			return DXGI_FORMAT_R32G32B32A32_UINT;
		}
	}

	if (mask == 0x07)
	{
		// xyz
		switch (type)
		{
		case D3D_REGISTER_COMPONENT_FLOAT32:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		case D3D_REGISTER_COMPONENT_UINT32:
			return DXGI_FORMAT_R32G32B32_UINT;
		}
	}

	if (mask == 0x3)
	{
		// xy
		switch (type)
		{
		case D3D_REGISTER_COMPONENT_FLOAT32:
			return DXGI_FORMAT_R32G32_FLOAT;
		case D3D_REGISTER_COMPONENT_UINT32:
			return DXGI_FORMAT_R32G32_UINT;
		}
	}

	if (mask == 0x1)
	{
		// x
		switch (type)
		{
		case D3D_REGISTER_COMPONENT_FLOAT32:
			return DXGI_FORMAT_R32_FLOAT;
		case D3D_REGISTER_COMPONENT_UINT32:
			return DXGI_FORMAT_R32_UINT;
		}
	}

	return DXGI_FORMAT_UNKNOWN;
}

void CreateInputLayoutAndConstantBufferFromShader(const std::shared_ptr<Shader>& shader, ID3DBlob * blob)
{
	ID3D11ShaderReflection * reflector = nullptr;
	D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&reflector);

	D3D11_SHADER_DESC shader_desc;
	reflector->GetDesc(&shader_desc);

	shader->constant_buffer_.resize(shader_desc.ConstantBuffers);

	for (unsigned int n = 0; n < shader_desc.ConstantBuffers; ++n)
	{
		int size = 0;
		auto cb = reflector->GetConstantBufferByIndex(n);
		D3D11_SHADER_BUFFER_DESC desc;
		cb->GetDesc(&desc);

		for (size_t j = 0; j < desc.Variables; ++j)
		{
			auto v = cb->GetVariableByIndex(j);
			D3D11_SHADER_VARIABLE_DESC vdesc;
			v->GetDesc(&vdesc);
			if (vdesc.Size % 16)
				size += vdesc.Size + 16 - (vdesc.Size % 16);
			else
				size += vdesc.Size;
		}

		D3D11_BUFFER_DESC bd;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.ByteWidth = size;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		bd.Usage = D3D11_USAGE_DEFAULT;

		if (FAILED(device_->CreateBuffer(&bd, nullptr, shader->constant_buffer_[n].GetAddressOf())))
			std::cout << "コンスタントバッファーの作成に失敗しました。" << std::endl;
	}

	std::vector<D3D11_INPUT_ELEMENT_DESC> element;
	for (unsigned int i = 0; i < shader_desc.InputParameters; ++i) {
		D3D11_SIGNATURE_PARAMETER_DESC sigdesc;
		reflector->GetInputParameterDesc(i, &sigdesc);

		auto format = GetDxgiFormat(sigdesc.ComponentType, sigdesc.Mask);

		D3D11_INPUT_ELEMENT_DESC eledesc =
		{
			sigdesc.SemanticName
			, sigdesc.SemanticIndex
			, format
			, 0
			, D3D11_APPEND_ALIGNED_ELEMENT
			, D3D11_INPUT_PER_VERTEX_DATA
			, 0
		};

		element.emplace_back(eledesc);
	}

	if (!element.empty())
		if (FAILED(device_->CreateInputLayout(&element[0], element.size(),
			blob->GetBufferPointer(), blob->GetBufferSize(), shader->input_layout_.GetAddressOf())))
			std::cout << "インプットレイアウトの作成に失敗しました。" << std::endl;
}

void Graphics::LoadShader(std::string path)
{
	shader_ = std::make_shared<Shader>();

	std::string p = "resource/shader/" + path + ".hlsl";

	auto & s = shader_;

	ID3DBlob * blob = nullptr;
	ID3DBlob * error = nullptr;

	if (FAILED(D3DCompileFromFile(std::wstring(p.begin(), p.end()).c_str(), nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", SHADER_FLAGS, 0, &blob, &error)))
	{
		if (error != nullptr)
			std::cout << __FUNCTION__ << " " << (char*)error->GetBufferPointer() << std::endl;
		else
			std::cout << __FUNCTION__ << " シェーダーの読み込みに失敗しました。" << std::endl;

		return;
	}
	else
	{
		device_->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, s->vertex_shader_.GetAddressOf());
		CreateInputLayoutAndConstantBufferFromShader(s, blob);
	}

	if (SUCCEEDED(D3DCompileFromFile(std::wstring(p.begin(), p.end()).c_str(), nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS", "gs_5_0", SHADER_FLAGS, 0, &blob, &error)))
	{
		device_->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, s->geometry_shader_.GetAddressOf());
	}

	if (SUCCEEDED(D3DCompileFromFile(std::wstring(p.begin(), p.end()).c_str(), nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "HS", "hs_5_0", SHADER_FLAGS, 0, &blob, &error)))
	{
		device_->CreateHullShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, s->hull_shader_.GetAddressOf());
	}

	if (SUCCEEDED(D3DCompileFromFile(std::wstring(p.begin(), p.end()).c_str(), nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "DS", "ds_5_0", SHADER_FLAGS, 0, &blob, &error)))
	{
		device_->CreateDomainShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, s->domain_shader_.GetAddressOf());
	}

	if (SUCCEEDED(D3DCompileFromFile(std::wstring(p.begin(), p.end()).c_str(), nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", SHADER_FLAGS, 0, &blob, &error)))
	{
		device_->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, s->pixel_shader_.GetAddressOf());
	}
}

bool Graphics::Begin(void)
{
	DirectX::XMVECTOR clear_color = { .2f, .4f, .8f, 1.f };
	context_->ClearRenderTargetView(back_buffer_rtv_.Get(), (float*)&clear_color);
	context_->ClearDepthStencilView(dsv_.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);

	context_->OMSetRenderTargets(1, back_buffer_rtv_.GetAddressOf(), dsv_.Get());
	context_->RSSetViewports(1, &viewport_);

	struct CB
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
	} cb;

	cb.world = DirectX::XMMatrixScaling(1.f, 1.f, 1.f);
	cb.view = DirectX::XMMatrixLookAtLH(DirectX::XMVectorSet(0.f, 20.f, -20.f, 0.f), DirectX::XMVectorZero(), DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f));
	cb.proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1280.f / 720.f, 0.1f, 1000.f);

	context_->VSSetShader(shader_->vertex_shader_.Get(), nullptr, 0);
	context_->PSSetShader(shader_->pixel_shader_.Get(), nullptr, 0);

	void * pcb = &cb;

	if (shader_->constant_buffer_[0])
		context_->UpdateSubresource(shader_->constant_buffer_[0].Get(), 0, nullptr, pcb, 0, 0);

	for (unsigned int n = 0; n < shader_->constant_buffer_.size(); ++n)
	{
		if (shader_->constant_buffer_[n])
		{
			context_->VSSetConstantBuffers(n, 1, shader_->constant_buffer_[n].GetAddressOf());
			context_->PSSetConstantBuffers(n, 1, shader_->constant_buffer_[n].GetAddressOf());
		}
	}

	context_->IASetInputLayout(shader_->input_layout_.Get());

	for (auto & mesh : rendering_object_.mesh_)
	{
		context_->IASetIndexBuffer(mesh.index_buffer_.Get(), DXGI_FORMAT_R32_UINT, 0);
		auto stride = 32U;
		auto offset = 0U;
		context_->IASetVertexBuffers(0, 1, mesh.vertex_buffer_.GetAddressOf(), &stride, &offset);
		context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		context_->DrawIndexed(mesh.index_cnt_, 0, 0);
	}

	return true;
}

bool Graphics::End(void)
{
	swap_chain_->Present(1, 0);

	return true;
}

void Graphics::Finalize(void)
{
}

ComPtr<ID3D11Device>& Graphics::device(void)
{
	return device_;
}

ComPtr<ID3D11DeviceContext>& Graphics::context(void)
{
	return context_;
}

void Graphics::SetupModel(Model & model)
{
	rendering_object_.mesh_.resize(model.meshes_.size());

	for (unsigned int n = 0; n < model.meshes_.size(); ++n)
	{
		auto & vertex_buffer = rendering_object_.mesh_[n].vertex_buffer_;
		auto & index_buffer = rendering_object_.mesh_[n].index_buffer_;
		auto & index_cnt = rendering_object_.mesh_[n].index_cnt_;

		auto & vertices = model.meshes_[n].vertices_;
		auto & indices = model.meshes_[n].indices_;

		{
			D3D11_BUFFER_DESC bd = {};
			bd.ByteWidth = vertices.size() * sizeof(Vertex);
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.Usage = D3D11_USAGE_DEFAULT;

			D3D11_SUBRESOURCE_DATA sd = {};
			sd.pSysMem = vertices.data();

			device_->CreateBuffer(&bd, &sd, vertex_buffer.GetAddressOf());
		}

		{
			index_cnt = indices.size();

			D3D11_BUFFER_DESC bd = {};
			bd.ByteWidth = index_cnt * sizeof(unsigned int);
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bd.Usage = D3D11_USAGE_DEFAULT;

			D3D11_SUBRESOURCE_DATA sd = {};
			sd.pSysMem = indices.data();

			device_->CreateBuffer(&bd, &sd, index_buffer.GetAddressOf());
		}
	}
}