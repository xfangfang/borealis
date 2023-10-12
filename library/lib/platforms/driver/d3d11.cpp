#include <borealis/platforms/driver/d3d11.hpp>
#define NANOVG_D3D11_IMPLEMENTATION
#include <nanovg_d3d11.h>
#include <versionhelpers.h>
#ifdef __ALLOW_TEARING__
#include <dxgi1_6.h>
#endif
#ifdef __GLFW__
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif defined(__SDL2__)
#include <SDL_syswm.h>
#endif
#ifdef __WINRT__
#include <winrt/Windows.Graphics.Display.h>
#include <borealis/platforms/driver/winrt.hpp>
#endif

namespace brls {
#ifdef __GLFW__
    bool D3D11Context::InitializeDX(GLFWwindow* window, int width, int height) {
        HWND hWndMain = glfwGetWin32Window(window);
        this->glfwWindow = window;
        this->hwnd = hWndMain;
        return InitializeDXInternal(hWndMain, nullptr, width, height);
    }
#elif defined(__SDL2__)
    bool D3D11Context::InitializeDX(SDL_Window* window, int width, int height) {
#ifndef __WINRT__
        SDL_SysWMinfo windowinfo;
        SDL_GetVersion(&windowinfo.version);
        SDL_GetWindowWMInfo(window, &windowinfo);
        this->sdlWindow = window;
        this->hwnd = windowinfo.info.win.window;
        return InitializeDXInternal(this->hwnd, nullptr, width, height);
#else
        // winrt 代码需要特别编译
        IUnknown *coreWindow = SDL_GetCoreWindow(window);
        return InitializeDXInternal(nullptr, coreWindow, width, height);
#endif
    }
#endif
    bool D3D11Context::InitializeDXInternal(HWND hWndMain, IUnknown *coreWindow, int width, int height) {
        HRESULT hr = S_OK;
        UINT deviceFlags = 0;
        IDXGIDevice *pDXGIDevice = nullptr;
        IDXGIAdapter *pAdapter = nullptr;
        IDXGIFactory2 *pDXGIFactory = nullptr;
        static const D3D_DRIVER_TYPE driverAttempts[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE,
        };
        static const D3D_FEATURE_LEVEL levelAttempts[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,  // Direct3D 11.1 SM 6
            D3D_FEATURE_LEVEL_11_0,  // Direct3D 11.0 SM 5
            D3D_FEATURE_LEVEL_10_1,  // Direct3D 10.1 SM 4
            D3D_FEATURE_LEVEL_10_0,  // Direct3D 10.0 SM 4
            D3D_FEATURE_LEVEL_9_3,   // Direct3D 9.3  SM 3
            D3D_FEATURE_LEVEL_9_2,   // Direct3D 9.2  SM 2
            D3D_FEATURE_LEVEL_9_1,   // Direct3D 9.1  SM 2
        };

        for (size_t driver = 0; driver < ARRAYSIZE(driverAttempts); driver++)
        {
            hr = D3D11CreateDevice(
                nullptr,
                driverAttempts[driver],
                nullptr,
                deviceFlags,
                levelAttempts,
                ARRAYSIZE(levelAttempts),
                D3D11_SDK_VERSION,
                &this->device,
                nullptr,
                &this->deviceContext);

            if (SUCCEEDED(hr))
            {
                break;
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = this->device->QueryInterface(IID_PPV_ARGS(&pDXGIDevice));
        }
        if (SUCCEEDED(hr))
        {
            hr = pDXGIDevice->GetAdapter(&pAdapter);
        }
        if (SUCCEEDED(hr))
        {
            hr = pAdapter->GetParent(IID_PPV_ARGS(&pDXGIFactory));
        }
#if defined(__ALLOW_TEARING__)
        IDXGIFactory6* factory6;
        if (SUCCEEDED(hr))
        {
            hr = pAdapter->GetParent(IID_PPV_ARGS(&factory6));
        }
        if (SUCCEEDED(hr)) {
            BOOL allowTearing = FALSE;
            factory6->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            this->tearingSupport = allowTearing == TRUE;
        }
#endif
        if (SUCCEEDED(hr))
        {
            DXGI_SWAP_CHAIN_DESC1 swapDesc;
            ZeroMemory(&swapDesc, sizeof(swapDesc));
            ZeroMemory(&this->sampleDesc, sizeof(this->sampleDesc));
            this->sampleDesc.Count = 1;
            this->sampleDesc.Quality = 0;
            swapDesc.SampleDesc.Count = this->sampleDesc.Count;
            swapDesc.SampleDesc.Quality = this->sampleDesc.Quality;
            swapDesc.Width = width;
            swapDesc.Height = height;
            swapDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            swapDesc.Stereo = FALSE;
            swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapDesc.BufferCount = SwapChainBufferCount;
            swapDesc.Flags = 0;
            swapDesc.Scaling = DXGI_SCALING_STRETCH;
            swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
#ifdef __WINRT__
            if (IsWindows8OrGreater()) {
                swapDesc.Scaling = DXGI_SCALING_NONE;
            } else {
                swapDesc.Scaling = DXGI_SCALING_STRETCH;
            }
#endif
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
            swapDesc.Scaling = DXGI_SCALING_STRETCH;
            swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
#endif
            if (this->tearingSupport) {
                swapDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }
            // this->swapDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
            if (coreWindow) {
                hr = pDXGIFactory->CreateSwapChainForCoreWindow(
                    this->device,
                    coreWindow,
                    &swapDesc,
                    nullptr,
                    &this->swapChain
                );
            } else {
                hr = pDXGIFactory->CreateSwapChainForHwnd(
                    this->device,
                    hWndMain,
                    &swapDesc,
                    nullptr,
                    nullptr,
                    &this->swapChain
                );
            }
        }
        D3D_API_RELEASE(pDXGIDevice);
        D3D_API_RELEASE(pAdapter);
        D3D_API_RELEASE(pDXGIFactory);
        if (FAILED(hr))
        {
            this->UnInitializeDX();
            return FALSE;
        }
        ResizeFramebufferSize(width, height, true);
        return TRUE;
    }

    void D3D11Context::UnInitializeDX() {
        // Detach RTs
        if (this->deviceContext)
        {
            ID3D11RenderTargetView *viewList[1] = { nullptr };
            this->deviceContext->OMSetRenderTargets(1, viewList, nullptr);
        }
        D3D_API_RELEASE(this->deviceContext);
        D3D_API_RELEASE(this->device);
        D3D_API_RELEASE(this->swapChain);
        D3D_API_RELEASE(this->renderTargetView);
        D3D_API_RELEASE(this->depthStencilView);
    }

    float D3D11Context::GetDpi() {
        float dpi = 1.0f;
#ifdef __SDL2__
#ifdef __WINRT__
        static winrt::Windows::Graphics::Display::DisplayInformation displayInformation = winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
        dpi = (unsigned int)displayInformation.LogicalDpi() / 96.0f;
#else
        dpi = GetDpiForWindow(this->hwnd) / 96.0f;
#endif
#else
        dpi = GetDpiForWindow(this->hwnd) / 96.0f;
#endif
        return dpi;
    }

    bool D3D11Context::ResizeFramebufferSize(int width, int height, bool init) {
        ID3D11RenderTargetView *viewList[1] = { nullptr };
        HRESULT hr = S_OK;

        ID3D11Texture2D *backBuffer = nullptr;
        ID3D11Texture2D* depthStencil = nullptr;
        this->deviceContext->OMSetRenderTargets(1, viewList, nullptr);

        D3D_API_RELEASE(this->renderTargetView);
        D3D_API_RELEASE(this->depthStencilView);
        if (!init) {
            // 初始化时无需重建 swapChain 的 buffers
            UINT resizeBufferFlags = 0;
            if (this->tearingSupport) {
                resizeBufferFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }
            hr = this->swapChain->ResizeBuffers(SwapChainBufferCount, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, resizeBufferFlags);
            if (FAILED(hr))
            {
                return false;
            }
        }

        hr = this->swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        if (FAILED(hr))
        {
            return false;
        }
        D3D11_RENDER_TARGET_VIEW_DESC renderDesc;
        renderDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        renderDesc.ViewDimension = (this->sampleDesc.Count>1) ?
            D3D11_RTV_DIMENSION_TEXTURE2DMS:
            D3D11_RTV_DIMENSION_TEXTURE2D;
        renderDesc.Texture2D.MipSlice = 0;
        hr = this->device->CreateRenderTargetView(
            backBuffer,
            &renderDesc,
            &this->renderTargetView);
        D3D_API_RELEASE(backBuffer);
        if (FAILED(hr))
        {
            return false;
        }
        D3D11_TEXTURE2D_DESC texDesc;
        texDesc.ArraySize = 1;
        texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        texDesc.CPUAccessFlags = 0;
        texDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        texDesc.Height = (UINT)height;
        texDesc.Width = (UINT)width;
        texDesc.MipLevels = 1;
        texDesc.MiscFlags = 0;
        texDesc.SampleDesc.Count = this->sampleDesc.Count;
        texDesc.SampleDesc.Quality = this->sampleDesc.Quality;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        hr = this->device->CreateTexture2D(
            &texDesc,
            nullptr,
            &depthStencil);
        if (FAILED(hr))
        {
            return false;
        }
        D3D11_DEPTH_STENCIL_VIEW_DESC depthViewDesc;
        depthViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthViewDesc.ViewDimension = (this->sampleDesc.Count>1) ?
            D3D11_DSV_DIMENSION_TEXTURE2DMS:
            D3D11_DSV_DIMENSION_TEXTURE2D;
        depthViewDesc.Flags = 0;
        depthViewDesc.Texture2D.MipSlice = 0;
        hr = this->device->CreateDepthStencilView(
            depthStencil,
            &depthViewDesc,
            &this->depthStencilView);
        D3D_API_RELEASE(depthStencil);
        if (FAILED(hr))
        {
            return false;
        }
        D3D11_VIEWPORT viewport;
        viewport.Width = (float)width;
        viewport.Height = (float)height;
        viewport.MaxDepth = 1.0f;
        viewport.MinDepth = 0.0f;
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        this->deviceContext->RSSetViewports(1, &viewport);
        return true;
    }

    void D3D11Context::SetRenderTarget() {
        this->deviceContext->OMSetRenderTargets(1, &this->renderTargetView, this->depthStencilView);
    }

    void D3D11Context::ClearWithColor(NVGcolor color) {
        // 清空前必须设置 RenderTarget 否则 SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL 情况会丢失绘制画面
        this->deviceContext->OMSetRenderTargets(1, &this->renderTargetView, this->depthStencilView);
        float clearColor[4];
        clearColor[0] = color.r;
        clearColor[1] = color.g;
        clearColor[2] = color.b;
        clearColor[3] = color.a;
        this->deviceContext->ClearRenderTargetView(
            this->renderTargetView,
            clearColor);
        this->deviceContext->ClearDepthStencilView(
            this->depthStencilView,
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
            0.0f, 0);
    }

    void D3D11Context::Present() {
        // https://learn.microsoft.com/zh-cn/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
        UINT presentFlags = 0;
        UINT syncInterval = 1;
        if (this->tearingSupport) {
            presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
            syncInterval = 0;
        }
        DXGI_PRESENT_PARAMETERS presentParameters;
        ZeroMemory(&presentParameters, sizeof(DXGI_PRESENT_PARAMETERS));
        this->swapChain->Present1(syncInterval, presentFlags, &presentParameters);
        // this->deviceContext->DiscardView(this->renderTargetView);
    }
}
