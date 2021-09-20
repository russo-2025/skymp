#include "IBrowser.h"
#include <cef/hooks/D3D11Hook.hpp>
#include <cef/hooks/DInputHook.hpp>
#include <cef/hooks/WindowsHook.hpp>
#include <skse64/NiRenderer.h>
#include "BrowserApi.h"
#include "FlowManager.h"

//extern std::shared_ptr<BrowserApi::State> g_browserApiState;

//---------------------------------------
// Cef

PCefBrowser::PCefBrowser() {
    inputConverter.reset(new InputConverter);

    myInputListener.reset(new MyInputListener);

    CEFUtils::D3D11Hook::Install();
    CEFUtils::DInputHook::Install(myInputListener);
    CEFUtils::WindowsHook::Install();

    CEFUtils::DInputHook::Get().SetToggleKeys({ VK_F6 });
    CEFUtils::DInputHook::Get().SetEnabled(true);

    overlayService.reset(new OverlayService);
    overlayService->GetMyChromiumApp();
    myInputListener->Init(overlayService, inputConverter);
    //g_browserApiState->overlayService = overlayService;

    // inputService.reset(new InputService(*overlayService));
    renderSystem.reset(new RenderSystemD3D11(*overlayService));
    renderSystem->m_pSwapChain = reinterpret_cast<IDXGISwapChain*>(BSRenderManager::GetSingleton()->swapChain);
}

PCefBrowser::~PCefBrowser() {
    FlowManager::CloseProcess(L"SkyrimPlatformCEF.exe");
}

void PCefBrowser::ExecuteJavaScript(const std::string& src) {
    overlayService->GetMyChromiumApp()->ExecuteJavaScript(src);
}

bool PCefBrowser::LoadUrl(const std::string& url) {
    return overlayService->GetMyChromiumApp()->LoadUrl(url.data());
}