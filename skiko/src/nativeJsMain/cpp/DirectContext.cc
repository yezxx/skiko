#include "ganesh/GrDirectContext.h"
#include "ganesh/gl/GrGLInterface.h"
#include "common.h"
#include "ganesh/gl/GrGLDirectContext.h" // TODO: skia update: check if it's correct

#ifdef __linux__
#include "ganesh/gl/GrGLAssembleInterface.h"
#include <dlfcn.h>
#endif

#ifdef __linux__
// The C++ toolchain that builds these bridges (a recent libstdc++) references the glibc 2.32 data symbol
// `__libc_single_threaded` from its shared_ptr fast path, while the Kotlin/Native link sysroot ships glibc 2.19
// and cannot resolve it. Defining it here as 0 selects the conservative multi-threaded path (the same behaviour
// as a process that has ever started a second thread), which is what this UI does anyway.
extern "C" char __libc_single_threaded = 0;
#endif

#ifdef SK_METAL
#include "ganesh/mtl/GrMtlBackendContext.h"
#include "ganesh/mtl/GrMtlDirectContext.h"
#endif

#ifdef SK_DIRECT3D
#include "ganesh/d3d/GrD3DBackendContext.h"
#include "ganesh/d3d/GrD3DDirectContext.h"
#endif

#ifdef __linux__
// Skia's default GL interface on Linux is GLX-based, and GLX needs an X server. Wayland sessions have no GLX:
// SDL hands out an EGL context instead, so bind Skia to that context by assembling the interface from
// `eglGetProcAddress`. libEGL is dlopen'ed (no extra link dependency; the process already has it loaded through
// SDL), and sessions with a current GLX context keep the previous code path.
typedef void* (*EglGetCurrentContextFn)();
typedef void* (*EglGetProcAddressFn)(const char*);

namespace {

GrGLFuncPtr eglProcLoader(void* /* ctx */, const char name[]) {
    static EglGetProcAddressFn getProcAddress = nullptr;
    static bool resolved = false;
    if (!resolved) {
        resolved = true;
        if (void* egl = dlopen("libEGL.so.1", RTLD_LAZY | RTLD_LOCAL)) {
            getProcAddress = reinterpret_cast<EglGetProcAddressFn>(dlsym(egl, "eglGetProcAddress"));
        }
    }
    return getProcAddress == nullptr ? nullptr : reinterpret_cast<GrGLFuncPtr>(getProcAddress(name));
}

bool hasCurrentEglContext() {
    static EglGetCurrentContextFn getCurrentContext = nullptr;
    static bool resolved = false;
    if (!resolved) {
        resolved = true;
        if (void* egl = dlopen("libEGL.so.1", RTLD_LAZY | RTLD_LOCAL)) {
            getCurrentContext = reinterpret_cast<EglGetCurrentContextFn>(dlsym(egl, "eglGetCurrentContext"));
        }
    }
    return getCurrentContext != nullptr && getCurrentContext() != nullptr;
}

} // namespace
#endif

SKIKO_EXPORT KNativePointer org_jetbrains_skia_DirectContext__1nMakeGL
  () {
#ifdef __linux__
    if (hasCurrentEglContext()) {
        sk_sp<const GrGLInterface> interface = GrGLMakeAssembledInterface(nullptr, eglProcLoader);
        if (interface != nullptr) {
            sk_sp<GrDirectContext> context = GrDirectContexts::MakeGL(interface);
            if (context != nullptr) {
                return static_cast<KNativePointer>(context.release());
            }
        }
    }
#endif
    return static_cast<KNativePointer>(GrDirectContexts::MakeGL().release());
}

SKIKO_EXPORT KNativePointer org_jetbrains_skia_DirectContext__1nMakeGLWithInterface
  (KNativePointer ptr) {
    sk_sp<GrGLInterface> iface = sk_ref_sp(reinterpret_cast<GrGLInterface*>(ptr));
    return static_cast<KNativePointer>(GrDirectContexts::MakeGL(iface).release());
}

SKIKO_EXPORT KNativePointer org_jetbrains_skia_DirectContext__1nMakeMetal
  (KNativePointer devicePtr, KNativePointer queuePtr) {
#ifdef SK_METAL
    GrMtlBackendContext backendContext = {};
    GrMTLHandle device = reinterpret_cast<GrMTLHandle>((devicePtr));
    GrMTLHandle queue = reinterpret_cast<GrMTLHandle>((queuePtr));
    backendContext.fDevice.retain(device);
    backendContext.fQueue.retain(queue);
    sk_sp<GrDirectContext> instance = GrDirectContexts::MakeMetal(backendContext);
    return static_cast<KNativePointer>(instance.release());
#else
    return nullptr;
#endif // SK_METAL
}

SKIKO_EXPORT KNativePointer org_jetbrains_skia_DirectContext__1nMakeDirect3D
  (KNativePointer adapterPtr, KNativePointer devicePtr, KNativePointer queuePtr) {
#ifdef SK_DIRECT3D
    GrD3DBackendContext backendContext = {};
    IDXGIAdapter1* adapter = reinterpret_cast<IDXGIAdapter1*>(adapterPtr);
    ID3D12Device* device = reinterpret_cast<ID3D12Device*>(devicePtr);
    ID3D12CommandQueue* queue = reinterpret_cast<ID3D12CommandQueue*>(queuePtr);
    backendContext.fAdapter.retain(adapter);
    backendContext.fDevice.retain(device);
    backendContext.fQueue.retain(queue);
    sk_sp<GrDirectContext> instance = GrDirectContexts::MakeD3D(backendContext);
    return static_cast<KNativePointer>(instance.release());
#else // SK_DIRECT3D
    return nullptr;
#endif // SK_DIRECT3D
}

SKIKO_EXPORT void org_jetbrains_skia_DirectContext__1nFlushDefault
  (KNativePointer ptr) {
    GrDirectContext* context = reinterpret_cast<GrDirectContext*>((ptr));
    context->flush(GrFlushInfo());
}

SKIKO_EXPORT void org_jetbrains_skia_DirectContext__1nFlush
  (KNativePointer ptr, KNativePointer skSurfacePtr) {
    GrDirectContext* context = reinterpret_cast<GrDirectContext*>(ptr);
    SkSurface* skSurface = reinterpret_cast<SkSurface*>(skSurfacePtr);
    context->flush(skSurface);
}

SKIKO_EXPORT KLong org_jetbrains_skia_DirectContext__1nGetResourceCacheLimit
  (KNativePointer ptr) {
    GrDirectContext* context = reinterpret_cast<GrDirectContext*>(ptr);
    return (KLong) context->getResourceCacheLimit();
}

SKIKO_EXPORT void org_jetbrains_skia_DirectContext__1nSetResourceCacheLimit
  (KNativePointer ptr, KLong maxResourceBytes) {
    GrDirectContext* context = reinterpret_cast<GrDirectContext*>(ptr);
    context->setResourceCacheLimit((size_t) maxResourceBytes);
}

GrSyncCpu grSyncCpuFromBool(bool syncCpu) {
    if (syncCpu) return GrSyncCpu::kYes;
    return GrSyncCpu::kNo;
}

SKIKO_EXPORT void org_jetbrains_skia_DirectContext__1nFlushAndSubmit
  (KNativePointer ptr, KNativePointer skSurfacePtr, KBoolean syncCpu) {
    GrDirectContext* context = reinterpret_cast<GrDirectContext*>(ptr);
    SkSurface* skSurface = reinterpret_cast<SkSurface*>(skSurfacePtr);
    context->flushAndSubmit(skSurface, grSyncCpuFromBool(syncCpu));
}

SKIKO_EXPORT void org_jetbrains_skia_DirectContext__1nSubmit
  (KNativePointer ptr, KBoolean syncCpu) {
    GrDirectContext* context = reinterpret_cast<GrDirectContext*>((ptr));
    context->submit(grSyncCpuFromBool(syncCpu));
}

SKIKO_EXPORT void org_jetbrains_skia_DirectContext__1nReset
  (KNativePointer ptr, KInt flags) {
    GrDirectContext* context = reinterpret_cast<GrDirectContext*>((ptr));
    context->resetContext((uint32_t) flags);
}

SKIKO_EXPORT void org_jetbrains_skia_DirectContext__1nAbandon
  (KNativePointer ptr, KInt flags) {
    GrDirectContext* context = reinterpret_cast<GrDirectContext*>((ptr));
    context->abandonContext();
}

