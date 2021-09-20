#pragma once

#include "TPInputService.h"
#include "TPOverlayService.h"
#include "TPRenderSystemD3D11.h"
#include "InputConverter.h"
#include "MyInputListener.h"
#include <memory>
#include <string>

class IBrowser {
public:
	virtual void ExecuteJavaScript(const std::string& src) = 0;
	virtual bool LoadUrl(const std::string& url) = 0;
};

class PCefBrowser : public IBrowser {
public:
	PCefBrowser();
	~PCefBrowser();

	void ExecuteJavaScript(const std::string& src) override;
	bool LoadUrl(const std::string& url) override;

private:
	std::shared_ptr<OverlayService> overlayService;
	// std::shared_ptr<InputService> inputService;
	std::shared_ptr<RenderSystemD3D11> renderSystem;
	std::shared_ptr<MyInputListener> myInputListener;
	std::shared_ptr<InputConverter> inputConverter;
};