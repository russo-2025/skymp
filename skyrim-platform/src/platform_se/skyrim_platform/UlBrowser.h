#include "IBrowser.h"
#include <Ultralight/Ultralight.h>
#include <AppCore/Platform.h>
#include <AppCore/Window.h>

#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXTK/SpriteBatch.h>
#include <DirectXTK/SimpleMath.h>
#include <DirectXTK/CommonStates.h>

#include <deque>
#include <mutex>
#include <future>
#include <thread>

using namespace ultralight;

class D3D11TextureSurfaceFactory;
/*
struct Task {
	TaskType type;
	union
	{
		std::packaged_task<void()> r_void;
		std::packaged_task<bool()> r_bool;
	} ptask;
};
*/

union TaskPromise {
	std::promise<bool> p_bool;
};

struct Task {
	TaskPromise promise;
};

class UlBrowser : public IBrowser {
public:
	UlBrowser();
	~UlBrowser();

	void OnCreate(IDXGISwapChain* apSwapChain);
	void OnRender(IDXGISwapChain* apSwapChain);
	void OnReset(IDXGISwapChain* apSwapChain);

	void ExecuteJavaScript(const std::string& src) override;
	bool LoadUrl(const std::string& url) override;

	size_t m_createConnection;
	size_t m_renderConnection;
	size_t m_resetConnection;

	void Update();
	void Log(std::string msg);

private:
	void Run();

	HWND hWnd;
	uint32_t width;
	uint32_t height;
	//WindowsUtil* util;

	Config* config;
	Renderer* renderer;
	View* view;
	Logger* logger;

	D3D11TextureSurfaceFactory* factory;
	ID3D11Device* d3d11_device;
	ID3D11DeviceContext* d3d11_context;
	ID3D11ShaderResourceView* m_pTextureView;
	DirectX::SpriteBatch* m_pSpriteBatch;
	DirectX::CommonStates* m_pStates;

	std::deque<std::packaged_task<void()>> tasks;
	std::mutex tasks_mutex;
	bool initialized = false;
	std::mutex tex_mutex;
};

//surface factory

#include <Ultralight/Ultralight.h>
#include <AppCore/Platform.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXTK/CommonStates.h>

class D3D11BitmapTextureSurface : public ultralight::Surface {
public:
	D3D11BitmapTextureSurface(ID3D11Device* a_device, ID3D11DeviceContext* a_context, uint32_t width, uint32_t height);
	~D3D11BitmapTextureSurface();

	uint32_t width() const override;
	uint32_t height() const override;
	uint32_t row_bytes() const override;
	size_t size() const override;
	void* LockPixels() override;
	void UnlockPixels() override;
	void Resize(uint32_t width, uint32_t height) override;
	ID3D11ShaderResourceView* GetTextureView();

protected:
	uint32_t width_;
	uint32_t height_;
	uint32_t row_bytes_;
	uint32_t size_;
	ID3D11Device* device;
	ID3D11DeviceContext* context;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pTextureView;
};

class D3D11TextureSurfaceFactory : public ultralight::SurfaceFactory {
public:
	D3D11TextureSurfaceFactory(ID3D11Device* a_device, ID3D11DeviceContext* a_context);

	~D3D11TextureSurfaceFactory();

	ultralight::Surface* CreateSurface(uint32_t width, uint32_t height) override;

	void DestroySurface(ultralight::Surface* surface) override;

protected:
	ID3D11Device* device;
	ID3D11DeviceContext* context;
};
