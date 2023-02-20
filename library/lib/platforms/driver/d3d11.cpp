#include <borealis/platforms/driver/d3d11.hpp>
#define NANOVG_D3D11_IMPLEMENTATION
#include <nanovg_d3d11.h>
#ifdef __ALLOW_TEARING__
#include <dxgi1_6.h>
#endif
#ifdef __GLFW__
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif definde(__SDL__)
#include <SDL_syswm.h>
#endif

namespace brls {
#ifdef __GLFW__
    bool D3D11Context::InitializeDX(GLFWwindow* window, int width, int height) {
        HWND hWndMain = glfwGetWin32Window(window);
        return InitializeDXInternal(hWndMain, nullptr, width, height);
    }
#elif definde(__SDL__)
    bool D3D11Context::InitializeDX(SDL_Window* window, int width, int height) {
        SDL_SysWMinfo windowinfo;
        SDL_GetWindowWMInfo(renderer->window, &windowinfo);
#ifdef __WINRT__
        // winrt 代码需要特别编译
        if (windowinfo.subsystem == SDL_SYSWM_WINRT) {
            ABI::Windows::UI::Core::ICoreWindow *coreWindow = NULL;
            if (FAILED(windowinfo.info.winrt.window->QueryInterface(&coreWindow))) {
                return false;
            }
            IUnknown *coreWindowAsIUnknown = NULL;
            coreWindow->QueryInterface(&coreWindowAsIUnknown);
            coreWindow->Release();
            return InitializeDXInternal(nullptr, coreWindowAsIUnknown, width, height);
        }
#endif
        return InitializeDXInternal(windowinfo.info.win.window, nullptr, width, height);
    }
#endif
    bool D3D11Context::InitializeDXInternal(HWND hWndMain, IUnknown *coreWindow, int width, int height) {
        HRESULT hr = S_OK;
        UINT deviceFlags = 0;
        IDXGIDevice *pDXGIDevice = NULL;
        IDXGIAdapter *pAdapter = NULL;
        IDXGIFactory2 *pDXGIFactory = NULL;
        static const D3D_DRIVER_TYPE driverAttempts[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE,
        };
        static const D3D_FEATURE_LEVEL levelAttempts[] =
        {
            D3D_FEATURE_LEVEL_11_0,  // Direct3D 11.0 SM 5
            D3D_FEATURE_LEVEL_10_1,  // Direct3D 10.1 SM 4
            D3D_FEATURE_LEVEL_10_0,  // Direct3D 10.0 SM 4
            D3D_FEATURE_LEVEL_9_3,   // Direct3D 9.3  SM 3
            D3D_FEATURE_LEVEL_9_2,   // Direct3D 9.2  SM 2
            D3D_FEATURE_LEVEL_9_1,   // Direct3D 9.1  SM 2
        };

        for (int driver = 0; driver < ARRAYSIZE(driverAttempts); driver++)
        {
            hr = D3D11CreateDevice(
                NULL,
                driverAttempts[driver],
                NULL,
                deviceFlags,
                levelAttempts,
                ARRAYSIZE(levelAttempts),
                D3D11_SDK_VERSION,
                &this->device,
                &this->featureLevel,
                &this->deviceContext);

            if (SUCCEEDED(hr))
            {
                break;
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = this->device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&pDXGIDevice));
        }
        if (SUCCEEDED(hr))
        {
            hr = pDXGIDevice->GetAdapter(&pAdapter);
        }
        if (SUCCEEDED(hr))
        {
            hr = pAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&pDXGIFactory);
        }
#ifdef __ALLOW_TEARING__
        IDXGIFactory6* factory6;
        if (SUCCEEDED(hr))
        {
            hr = pAdapter->GetParent(__uuidof(IDXGIFactory6), (void**)&factory6);
        }
        if (SUCCEEDED(hr)) {
            BOOL allowTearing = FALSE;
            factory6->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            this->tearingSupport = allowTearing == TRUE;
        }
#endif
        if (SUCCEEDED(hr))
        {
            ZeroMemory(&this->swapDesc, sizeof(this->swapDesc));
            this->swapDesc.SampleDesc.Count = 1;
            this->swapDesc.SampleDesc.Quality = 0;
            this->swapDesc.Width = width;
            this->swapDesc.Height = height;
            this->swapDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            this->swapDesc.Stereo = FALSE;
            this->swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            this->swapDesc.BufferCount = 2;
            this->swapDesc.Scaling = DXGI_SCALING_STRETCH;
            this->swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            this->swapDesc.Flags = 0;
            if (this->tearingSupport) {
                this->swapDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }
            // this->swapDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
            if (coreWindow) {
                hr = pDXGIFactory->CreateSwapChainForCoreWindow(
                    (IUnknown*)this->device,
                    coreWindow,
                    &this->swapDesc,
                    NULL,
                    &this->swapChain
                );
            } else {
                hr = pDXGIFactory->CreateSwapChainForHwnd(
                    (IUnknown*)this->device,
                    hWndMain,
                    &this->swapDesc,
                    NULL,
                    NULL,
                    &this->swapChain
                );
            }
        }
        D3D_API_RELEASE(pDXGIDevice);
        D3D_API_RELEASE(pAdapter);
        D3D_API_RELEASE(pDXGIFactory);
        if (!SUCCEEDED(hr))
        {
            // Fail
            this->UnInitializeDX();
            return FALSE;
        }
        return TRUE;
    }

    void D3D11Context::UnInitializeDX() {
        // Detach RTs
        if (this->deviceContext)
        {
            ID3D11RenderTargetView *viewList[1] = { NULL };
            this->deviceContext->OMSetRenderTargets(1, viewList, NULL);
        }
        D3D_API_RELEASE(this->deviceContext);
        D3D_API_RELEASE(this->device);
        D3D_API_RELEASE(this->swapChain);
        D3D_API_RELEASE(this->renderTargetView);
        D3D_API_RELEASE(this->depthStencil);
        D3D_API_RELEASE(this->depthStencilView);
    }

    bool D3D11Context::ResizeFramebufferSize(int width, int height) {
        D3D11_RENDER_TARGET_VIEW_DESC renderDesc;
        ID3D11RenderTargetView *viewList[1] = { NULL };
        HRESULT hr = S_OK;

        ID3D11Resource *pBackBufferResource = NULL;
        D3D11_TEXTURE2D_DESC texDesc;
        D3D11_DEPTH_STENCIL_VIEW_DESC depthViewDesc;
        this->deviceContext->OMSetRenderTargets(1, viewList, NULL);

        D3D_API_RELEASE(this->renderTargetView);
        D3D_API_RELEASE(this->depthStencilView);
        UINT resizeBufferFlags = 0;
        if (this->tearingSupport) {
            resizeBufferFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }
        hr = this->swapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, resizeBufferFlags);
        if (FAILED(hr))
        {
            return false;
        }
        hr = this->swapChain->GetBuffer(
            0,
            __uuidof(ID3D11Texture2D),
            (void**)&pBackBufferResource
        );
        if (FAILED(hr))
        {
            return false;
        }
        renderDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        renderDesc.ViewDimension = (swapDesc.SampleDesc.Count>1) ?
            D3D11_RTV_DIMENSION_TEXTURE2DMS:
            D3D11_RTV_DIMENSION_TEXTURE2D;
        renderDesc.Texture2D.MipSlice = 0;
        hr = this->device->CreateRenderTargetView(
            pBackBufferResource,
            &renderDesc,
            &this->renderTargetView);
        D3D_API_RELEASE(pBackBufferResource);
        if (FAILED(hr))
        {
            return false;
        }
        texDesc.ArraySize = 1;
        texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        texDesc.CPUAccessFlags = 0;
        texDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        texDesc.Height = (UINT)height;
        texDesc.Width = (UINT)width;
        texDesc.MipLevels = 1;
        texDesc.MiscFlags = 0;
        texDesc.SampleDesc.Count = swapDesc.SampleDesc.Count;
        texDesc.SampleDesc.Quality = swapDesc.SampleDesc.Quality;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        D3D_API_RELEASE(this->depthStencil);
        hr = this->device->CreateTexture2D(
            &texDesc,
            NULL,
            &this->depthStencil);
        if (FAILED(hr))
        {
            return false;
        }
        depthViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthViewDesc.ViewDimension = (swapDesc.SampleDesc.Count>1) ?
            D3D11_DSV_DIMENSION_TEXTURE2DMS:
            D3D11_DSV_DIMENSION_TEXTURE2D;
        depthViewDesc.Flags = 0;
        depthViewDesc.Texture2D.MipSlice = 0;
        hr = this->device->CreateDepthStencilView(
            (ID3D11Resource*)this->depthStencil,
            &depthViewDesc,
            &this->depthStencilView);
        if (FAILED(hr))
        {
            return false;
        }

        this->deviceContext->OMSetRenderTargets(1, &this->renderTargetView, this->depthStencilView);
        D3D11_VIEWPORT viewport;
        viewport.Height = (float)height;
        viewport.Width = (float)width;
        viewport.MaxDepth = 1.0f;
        viewport.MinDepth = 0.0f;
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        this->deviceContext->RSSetViewports(1, &viewport);
        return true;
    }

    void D3D11Context::ClearWithColor(NVGcolor color) {
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
            0.0f,
            (UINT8)0);
    }

    void D3D11Context::Present() {
        // https://learn.microsoft.com/zh-cn/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
        UINT presentFlags = 0;
        UINT syncInterval = 1;
        if (this->tearingSupport) {
            presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
            syncInterval = 0;
        }
        this->swapChain->Present1(syncInterval, presentFlags, NULL);
    }
}
