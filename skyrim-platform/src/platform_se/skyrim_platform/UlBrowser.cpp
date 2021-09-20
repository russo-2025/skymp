#include "UlBrowser.h"
#include <cef/hooks/D3D11Hook.hpp>
#include <d3d11.h>

UlBrowser::UlBrowser()
{
	CEFUtils::D3D11Hook::Install();

    auto& d3d11 = CEFUtils::D3D11Hook::Get();

    m_createConnection = d3d11.OnCreate.Connect(std::bind(&UlBrowser::OnCreate, this, std::placeholders::_1));
    m_resetConnection = d3d11.OnLost.Connect(std::bind(&UlBrowser::OnReset, this, std::placeholders::_1));
    m_renderConnection = d3d11.OnPresent.Connect(std::bind(&UlBrowser::OnRender, this, std::placeholders::_1));
}

UlBrowser::~UlBrowser()
{
    auto& d3d11 = CEFUtils::D3D11Hook::Get();

    d3d11.OnCreate.Disconnect(m_createConnection);
    d3d11.OnLost.Disconnect(m_resetConnection);
    d3d11.OnPresent.Disconnect(m_renderConnection);
}

#include <thread>
#include <chrono>

std::string thread_id_string(std::thread::id tid) {
	std::stringstream ss;
	ss << tid;
	return ss.str();
}
void UlBrowser::Log(std::string msg) {
	logger->LogMessage(kLogLevel_Info, msg.c_str());
}

void UlBrowser::OnCreate(IDXGISwapChain* apSwapChain) {
	DXGI_SWAP_CHAIN_DESC desc;
	apSwapChain->GetDesc(&desc);

	apSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&d3d11_device);
	d3d11_device->GetImmediateContext(&d3d11_context);

	width = 1920;
	height = 1080;
	hWnd = desc.OutputWindow;

	m_pSpriteBatch = new DirectX::SpriteBatch(d3d11_context);
	m_pStates = new DirectX::CommonStates(d3d11_device);

	std::thread th([this]() {
		Run();
		Update();
		initialized = true;

		while (true) {
			Update();
		}
	});

	th.detach();
}

void UlBrowser::Update() {
	if (!tasks.empty())
	{
		Log("Run tasks");

		//std::unique_lock<std::mutex> lock(tasks_mutex);
		tasks_mutex.lock();

		while (!tasks.empty()) {
			auto task(std::move(tasks.front()));
			tasks.pop_front();

			// unlock during the task
			tasks_mutex.unlock();
			task();
			tasks_mutex.lock();
		}

		tasks_mutex.unlock();
	}
	
	renderer->Update();

	if (view->needs_paint()) {
		renderer->Render();

		D3D11BitmapTextureSurface* surface = (D3D11BitmapTextureSurface*)view->surface();
		m_pTextureView = surface->GetTextureView();
	}
}

void UlBrowser::Run() {
	config = new Config();
	config->resource_path = "D:\\Steam\\steamapps\\common\\Skyrim Special Edition\\Data\\Platform\\Distribution\\RuntimeDependencies\\resources";
	config->device_scale = 1.0;
	config->cache_path = "D:\\Steam\\steamapps\\common\\Skyrim Special Edition\\Data\\Platform\\Distribution\\RuntimeDependencies\\resources";

	Platform::instance().set_config(*config);
	Platform::instance().set_font_loader(GetPlatformFontLoader());
	Platform::instance().set_file_system(GetPlatformFileSystem("D:\\Steam\\steamapps\\common\\Skyrim Special Edition\\Data\\Platform\\Distribution\\RuntimeDependencies"));

	logger = GetDefaultLogger("ultralight.log");
	Platform::instance().set_logger(logger);

	factory = new D3D11TextureSurfaceFactory(d3d11_device, d3d11_context);
	Platform::instance().set_surface_factory(factory);

	renderer = &Renderer::Create().LeakRef();
	view = &renderer->CreateView(width, height, true, renderer->default_session(), true).LeakRef();

	Log("Ultralight initialized");
	//auto tstr = thread_id_string(std::this_thread::get_id());
	//Log(tstr.data());
	/*
	std::chrono::milliseconds interval_ms(4);
	std::chrono::steady_clock::time_point next_paint = std::chrono::steady_clock::now();
	
	while (true) {
		long long timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(next_paint - std::chrono::steady_clock::now()).count();
		//unsigned long timeout = timeout_ms <= 0 ? 0 : (unsigned long)timeout_ms;

		if (timeout_ms > 0)
			std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms)); //<<----

		//Log("while timeout");
		//Log(std::to_string(timeout_ms).c_str());

		if (!tasks.empty())
		{
			Log("Run tasks");

			//std::unique_lock<std::mutex> lock(tasks_mutex);
			tasks_mutex.lock();

			while (!tasks.empty()) {
				auto task(std::move(tasks.front()));
				tasks.pop_front();

				// unlock during the task
				tasks_mutex.unlock();
				task();
				tasks_mutex.lock();
			}

			tasks_mutex.unlock();
		}

		renderer->Update();

		timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(next_paint - std::chrono::steady_clock::now()).count();

		if (timeout_ms <= 0) {
			if (view->needs_paint()) {
				renderer->Render();

				D3D11BitmapTextureSurface* surface = (D3D11BitmapTextureSurface*)view->surface();
				m_pTextureView = surface->GetTextureView();
				initialized = true;
			}

			next_paint = std::chrono::steady_clock::now() + interval_ms;
		}
	}*/
}

void UlBrowser::OnRender(IDXGISwapChain* apSwapChain) {
	if (!initialized) {
		return;
	}

	/*
	if (!tasks.empty())
	{
		Log("Run tasks");

		std::unique_lock<std::mutex> lock(tasks_mutex);

		while (!tasks.empty()) {
			auto task(std::move(tasks.front()));
			tasks.pop_front();

			// unlock during the task
			lock.unlock();
			task();
			lock.lock();
		}

		lock.unlock();
	}

	renderer->Update();

	if (view->needs_paint()) {
		renderer->Render();

		D3D11BitmapTextureSurface* surface = (D3D11BitmapTextureSurface*)view->surface();
		m_pTextureView = surface->GetTextureView();
	}
	*/

	m_pSpriteBatch->Begin(DirectX::SpriteSortMode_Deferred, m_pStates->NonPremultiplied());
	m_pSpriteBatch->Draw(m_pTextureView, DirectX::SimpleMath::Vector2(0.f, 0.f), nullptr, DirectX::Colors::White, 0.f);
	m_pSpriteBatch->End();
}

void UlBrowser::OnReset(IDXGISwapChain* apSwapChain) {
}

void UlBrowser::ExecuteJavaScript(const std::string& src) {
	/*
	Log("ExecuteJavaScript Start");
	view->EvaluateScript(src.data());
	Log("ExecuteJavaScript End");
	*/

	
	Log("ExecuteJavaScript Start");

	std::packaged_task<void()> ptask([&]() {
		Log("!!!ExecuteJavaScript");
		view->EvaluateScript(src.data());
		});

	std::future<void> result = ptask.get_future();

	std::unique_lock<std::mutex> lock(tasks_mutex);
	tasks.push_back(std::move(ptask));
	lock.unlock();

	result.get();
	Log("ExecuteJavaScript End");
}

bool UlBrowser::LoadUrl(const std::string& url) {
	// https://stackoverflow.com/questions/17354260/c-stdasync-run-on-main-thread
	// https://thispointer.com/c11-multithreading-part-8-stdfuture-stdpromise-and-returning-values-from-thread/
	// https://stackoverflow.com/questions/18143661/what-is-the-difference-between-packaged-task-and-async#18143844
	// похоже поток отрисовки лочится, мб стоит вынести ul в отдельный поток
	
	/*
	Log("LoadUrl Start");
	view->LoadURL(url.data());
	Log("LoadUrl End");
	*/
	

	
	Log("LoadUrl Start");

	//bool test_result = false;// <<<<---------- если можно менять из таски внешние переменные то можно попробовать реализовать таски с помощью этого
	std::packaged_task<void()> ptask([&]() {
		Log("!!!LoadUrl");
		view->LoadURL(url.data());
		//test_result = true;// <<<<-
	});

	std::future<void> result = ptask.get_future();

	std::unique_lock<std::mutex> lock(tasks_mutex);
	tasks.push_back(std::move(ptask));
	lock.unlock();

	result.get();
	Log("LoadUrl End");
	

	return true;
}

//surface factory

D3D11BitmapTextureSurface::D3D11BitmapTextureSurface(ID3D11Device* a_device, ID3D11DeviceContext* a_context, uint32_t width, uint32_t height) {
	device = a_device;
	context = a_context;

	Resize(width, height);
}

D3D11BitmapTextureSurface::~D3D11BitmapTextureSurface() {
}

void* D3D11BitmapTextureSurface::LockPixels() {
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	if (FAILED(context->Map(m_pTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
		//logger->LogMessage(kLogLevel_Info, "D3D11BitmapTextureSurface::LockPixels nullptr");
		//RE::ConsoleLog::GetSingleton()->Print("D3D11BitmapTextureSurface::LockPixels nullptr");
		return nullptr;
	}

	return mappedResource.pData;
};

void D3D11BitmapTextureSurface::UnlockPixels() {
	context->Unmap(m_pTexture.Get(), 0);
};

void D3D11BitmapTextureSurface::Resize(uint32_t width, uint32_t height) {
	if (width_ == width && height_ == height)
		return;

	width_ = width;
	height_ = height;
	row_bytes_ = width_ * 4;
	size_ = row_bytes_ * height_;

	D3D11_TEXTURE2D_DESC textDesc;
	textDesc.Width = width;
	textDesc.Height = height;
	textDesc.MipLevels = textDesc.ArraySize = 1;
	textDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	textDesc.SampleDesc.Count = 1;
	textDesc.SampleDesc.Quality = 0;
	textDesc.Usage = D3D11_USAGE_DYNAMIC;
	textDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	textDesc.MiscFlags = 0;

	if (FAILED(device->CreateTexture2D(&textDesc, nullptr, m_pTexture.ReleaseAndGetAddressOf())))
		return;

	D3D11_SHADER_RESOURCE_VIEW_DESC sharedResourceViewDesc = {};
	sharedResourceViewDesc.Format = textDesc.Format;
	sharedResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sharedResourceViewDesc.Texture2D.MipLevels = 1;

	if (FAILED(device->CreateShaderResourceView(m_pTexture.Get(), &sharedResourceViewDesc, m_pTextureView.ReleaseAndGetAddressOf())))
		return;
}

ID3D11ShaderResourceView* D3D11BitmapTextureSurface::GetTextureView() {
	return m_pTextureView.Get();
}

uint32_t D3D11BitmapTextureSurface::width() const { return width_; }

uint32_t D3D11BitmapTextureSurface::height() const { return height_; }

uint32_t D3D11BitmapTextureSurface::row_bytes() const { return row_bytes_; }

size_t D3D11BitmapTextureSurface::size() const { return size_; }

//-----------------------------
//D3D11TextureSurfaceFactory

D3D11TextureSurfaceFactory::D3D11TextureSurfaceFactory(ID3D11Device* a_device, ID3D11DeviceContext* a_context) {
	device = a_device;
	context = a_context;
}

D3D11TextureSurfaceFactory::~D3D11TextureSurfaceFactory() {
}

ultralight::Surface* D3D11TextureSurfaceFactory::CreateSurface(uint32_t width, uint32_t height) {
	///
	/// Called by Ultralight when it wants to create a Surface.
	///
	return (ultralight::Surface*)new D3D11BitmapTextureSurface(device, context, width, height);
}

void D3D11TextureSurfaceFactory::DestroySurface(ultralight::Surface* surface) {
	///
	/// Called by Ultralight when it wants to destroy a Surface.
	///
	delete static_cast<D3D11BitmapTextureSurface*>(surface);
}
