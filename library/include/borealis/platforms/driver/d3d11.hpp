#pragma once
#include <nanovg.h>
#include <d3d11.h>
#include <d3d11_1.h>

#ifdef __GLFW__
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#elif definde(__SDL__)
#include <SDL.h>
#endif

namespace brls
{
    class D3D11Context {
        public:
            D3D11Context():
            device(nullptr),
            deviceContext(nullptr),
            swapChain(nullptr),
            renderTargetView(nullptr),
            depthStencil(nullptr),
            depthStencilView(nullptr),
            tearingSupport(false){}
            ~D3D11Context() {
                this->UnInitializeDX();
            }
        private:
            bool InitializeDXInternal(HWND window, IUnknown *coreWindow, int width, int height);
            static const int SwapChainBufferCount = 2;
        public:
#ifdef __GLFW__
            bool InitializeDX(GLFWwindow* window, int width, int height);
#elif definde(__SDL__)
            bool InitializeDX(SDL_Window* window, int width, int height);
#endif
            void UnInitializeDX();
            bool ResizeFramebufferSize(int width, int height);
            void ClearWithColor(NVGcolor color);
            void Present();
            ID3D11Device* GetDevice() {
                return this->device;
            }
        private:
            ID3D11Device* device;
            ID3D11DeviceContext* deviceContext;
            IDXGISwapChain1* swapChain;
            ID3D11RenderTargetView* renderTargetView;
            ID3D11Texture2D* depthStencil;
            ID3D11DepthStencilView* depthStencilView;
            D3D_FEATURE_LEVEL featureLevel;
            DXGI_SAMPLE_DESC sampleDesc;
            bool tearingSupport;
    };
}
