#pragma once

#include <cef/hooks/IInputListener.h>
#include "TPOverlayService.h"
#include "InputConverter.h"

class MyInputListener : public IInputListener
{
public:
    bool IsBrowserFocused();

    MyInputListener();

    void Init(std::shared_ptr<OverlayService> service_, std::shared_ptr<InputConverter> conv_);

    void InjectChar(uint8_t code);

    void InjectKey(uint8_t code, bool down);

    int VscToVk(int code);

    void OnKeyStateChange(uint8_t code, bool down) noexcept override;

    void OnMouseWheel(int32_t delta) noexcept override;

    void OnMouseMove(float deltaX, float deltaY) noexcept override;

    void OnMouseStateChange(MouseButton mouseButton, bool down) noexcept override;

    void OnUpdate() noexcept override;

private:
    std::shared_ptr<OverlayService> service;
    std::shared_ptr<InputConverter> conv;
    std::array<clock_t, 256> vkCodeDownDur;
    float* pCursorX = nullptr;
    float* pCursorY = nullptr;
    bool switchLayoutDownWas = false;
};

inline uint32_t GetCefModifiers_(uint16_t aVirtualKey);