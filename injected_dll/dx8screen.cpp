#include "dxscreen.h"
#include "hook.h"

#include <d3d8.h>
#include <d3d8types.h>


struct THookCtxDX8 {
    void* PresentFun = nullptr;
    TScreenCallback Callback;
};
static THookCtxDX8* HookCtx = new THookCtxDX8();


void GetDX8Screenshot(IDirect3DDevice8* device) {
    HRESULT hr;

    IDirect3DSurface8* backbuffer;
    hr = device->GetRenderTarget(&backbuffer);
    if (FAILED(hr)) {
        return;
    }

    D3DSURFACE_DESC desc;
    hr = backbuffer->GetDesc(&desc);
    if (FAILED(hr)) {
        return;
    }

    IDirect3DSurface8* buffer;
    hr = device->CreateImageSurface(desc.Width, desc.Height, desc.Format, &buffer);
    if (FAILED(hr)) {
        return;
    }

    hr = device->CopyRects(backbuffer, NULL, 0, buffer, NULL);
    backbuffer->Release();

    if (FAILED(hr)) {
        return;
    }

    D3DLOCKED_RECT rect;
    hr = buffer->LockRect(&rect, NULL, D3DLOCK_READONLY);
    if (FAILED(hr)) {
        return;
    }

    QByteArray screen = PackImageData(NQtScreen::BF_B8G8R8A8, (char*)rect.pBits, desc.Height, desc.Width);
    HookCtx->Callback(screen);

    buffer->Release();
}

static HRESULT STDMETHODCALLTYPE HookPresent(IDirect3DDevice8* device,
                    CONST RECT* srcRect, CONST RECT* dstRect,
                    HWND overrideWindow, CONST RGNDATA* dirtyRegion)
{
    UnHook(HookCtx->PresentFun);
    GetDX8Screenshot(device);
    return device->Present(srcRect, dstRect, overrideWindow, dirtyRegion);
}

bool MakeDX8Screen(const TScreenCallback& callback, uint64_t presentOffset) {
    HMODULE dx8module = GetSystemModule("d3d8.dll");
    if (!dx8module) {
        return false;
    }
    HookCtx->PresentFun = (void*)((uintptr_t)dx8module + (uintptr_t)presentOffset);
    HookCtx->Callback = callback;
    Hook(HookCtx->PresentFun, HookPresent);
    return true;
}
