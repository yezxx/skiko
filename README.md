# maven (branch)

Plain Maven repository (GitHub-served, no server) for the **EGL-patched skiko 0.150.1** used by the KMP Showcase
desktop target. Artifacts keep the upstream coordinates `org.jetbrains.skiko:skiko{,-linuxx64}:0.150.1`, because the
prebuilt compose-desktop-native klibs reference skiko by its klib `unique_name`, which carries the Maven group.

Source of truth: upstream skiko `v0.150.1` (commit 3956e988) + `source/skiko-wayland-egl.patch`
(Kotlin 2.3.20, Skia m150-1f14f1166a). What the patch does:
- `DirectContext.makeGL()` assembles Skia's GL interface from `eglGetProcAddress` when an EGL context is current
  (Wayland), keeping the GLX path for X11 - without it Skia fails on Wayland and the window falls back to CPU raster.
- defines `__libc_single_threaded`, which the Kotlin/Native link sysroot (glibc 2.19) cannot resolve.

Consume it with (must precede `mavenCentral`: identical coordinates, so repository order decides):

```kotlin
maven("https://raw.githubusercontent.com/yezxx/skiko/maven") {
    content { includeGroupAndSubgroups("org.jetbrains.skiko") }
}
```

or, with GitHub Pages enabled for this repo: `https://yezxx.github.io/skiko`.

Rebuild: see `tools/rebuild.sh` (clones upstream v0.150.1, applies the patch, refreshes the artifacts).

Licensing: skiko Apache-2.0 (`LICENSE`, `NOTICE`); Skia (embedded in the klib) BSD-3 (`LICENSES/skia-LICENSE.txt`).
