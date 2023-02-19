#pragma once

#ifndef __BOREALIS_USE_D3D11_INCLUDE
#define __BOREALIS_USE_D3D11_INCLUDE
#include <GLFW/glfw3.h>
#include <nanovg.h>
#include <d3d11.h>

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
            depthStencilView(nullptr){}
            ~D3D11Context() {
                this->UnInitializeDX();
            }
        public:
            bool InitializeDX(GLFWwindow* window, int width, int height);
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
            IDXGISwapChain* swapChain;
            ID3D11RenderTargetView* renderTargetView;
            ID3D11Texture2D* depthStencil;
            ID3D11DepthStencilView* depthStencilView;
            D3D_FEATURE_LEVEL featureLevel;
            DXGI_SWAP_CHAIN_DESC swapDesc;
    };
}
#endif
