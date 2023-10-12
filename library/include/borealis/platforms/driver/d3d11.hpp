#pragma once
#include <nanovg.h>
#include <d3d11.h>
#include <d3d11_1.h>

#ifdef __GLFW__
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#elif defined(__SDL2__)
#include <SDL2/SDL.h>
#endif

namespace brls
{
    class D3D11Context {
        public:
            D3D11Context() = default;
            ~D3D11Context() { this->UnInitializeDX(); }
        private:
            bool InitializeDXInternal(HWND window, IUnknown *coreWindow, int width, int height);
            static const int SwapChainBufferCount = 2;
#ifdef __GLFW__
            GLFWwindow* glfwWindow;
            HWND hwnd;
#elif defined(__SDL2__)
            SDL_Window* sdlWindow;
            HWND hwnd;
#endif
        public:
#ifdef __GLFW__
            bool InitializeDX(GLFWwindow* window, int width, int height);
#elif defined(__SDL2__)
            bool InitializeDX(SDL_Window* window, int width, int height);
#endif
            void UnInitializeDX();
            float GetDpi();
            bool ResizeFramebufferSize(int width, int height, bool init = false);
            void ClearWithColor(NVGcolor color);
            void SetRenderTarget();
            void Present();
            ID3D11Device* GetDevice() { return this->device; }
            IDXGISwapChain* GetSwapChain() { return this->swapChain; }
        private:
            ID3D11Device* device = nullptr;
            ID3D11DeviceContext* deviceContext = nullptr;
            IDXGISwapChain1* swapChain = nullptr;
            ID3D11RenderTargetView* renderTargetView = nullptr;
            ID3D11DepthStencilView* depthStencilView = nullptr;
            DXGI_SAMPLE_DESC sampleDesc;
            bool tearingSupport;
    };
}
