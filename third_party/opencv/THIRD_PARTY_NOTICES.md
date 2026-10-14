# Third-party notices — OpenCV 5.0.0

These notices cover the binaries contained in the
`opencv-5.0.0-<platform>-*.zip` archives built from this repository
(macOS arm64 and Windows x64 today; Linux gains its own block when its
guide lands). Each platform's block must stay current with the build it
packages — the packaging step of every platform guide checks it before
zipping (`opencv-mac.md` / `opencv-win.md`, section 4).

## OpenCV

- **Source:** <https://github.com/opencv/opencv>, tag `5.0.0`
  (commit `40738fb16ceddb5fb3fea747585f7ce6abb0605b`; the tag itself is an
  annotated tag object, `9e2ede9628a55ec2742a1b180d3e69b0322281b9`).
- **License:** Apache License 2.0.
- **Full license text, included in every archive:** `LICENSE` (verbatim
  copy of the file at the source root), together with the project's
  copyright-attribution block (`COPYRIGHT`, also verbatim from the source
  root). The dev archive additionally carries the per-component license
  texts the OpenCV build installs.
- **Build inputs, per platform:**
  - **macOS arm64:** one local modification — the `VERSION`/`SOVERSION`
    target properties in `cmake/OpenCVModule.cmake` (the "For dynamic link
    numbering conventions" block, lines 1029–1037 at the tag) are removed so
    the modules build as plain unversioned `libopencv_*.dylib` files (see
    `opencv-mac.md`, section 2.1). The modification is part of the build
    inputs, not a binary patch.
  - **Windows x64:** built unmodified. The block the macOS build patches is
    inert on Windows — CMake applies `VERSION`/`SOVERSION` to ELF/Mach-O
    file names only, and Windows has no library symlinks. DLL names instead
    carry OpenCV's own `OPENCV_DLLVERSION` digits
    (`opencv_core500.dll`, top-level `CMakeLists.txt` lines 558–561 at the
    tag), with the `5.0.0` version quad embedded in each DLL's version
    resource (`vs_version.rc`, `cmake/OpenCVVersion.cmake` lines 22–27).
    See `opencv-win.md`, section 2.
- **Binary provenance:**
  - **macOS arm64:** built 2026-10-13 on macOS 27.0.1 with Apple clang
    21.0.0 (Command Line Tools 27.0.0), CMake 4.4.4 + Ninja 1.13.2,
    Release, shared libraries (.dylib). Configure switches:
    `BUILD_SHARED_LIBS=ON`, `BUILD_TESTS=OFF`, `BUILD_PERF_TESTS=OFF`,
    `BUILD_EXAMPLES=OFF`, `BUILD_opencv_apps=OFF`, `BUILD_JAVA=OFF`,
    `BUILD_opencv_python3=OFF`, `WITH_IPP=OFF`, `WITH_KLEIDICV=OFF`,
    `WITH_FFMPEG=OFF`, `WITH_OPENEXR=OFF`, `WITH_VTK=OFF`,
    `WITH_EIGEN=OFF`, `WITH_UNIFONT=OFF` — full command in
    `opencv-mac.md`, section 2.2.
  - **Windows x64:** built with MSVC 19.44.35229 (Visual Studio 2022
    17.14; toolset 14.44.35207, OpenCV runtime tag vc17), CMake 4.4.4 +
    Ninja 1.12.1, Release, shared libraries (DLLs) — verified against
    the configure run of 2026-10-13 recorded in `tmp\opencv\configure.log`;
    re-check section 4.1 when the toolchain or configuration changes.
    Configure switches:
    `BUILD_SHARED_LIBS=ON`, `BUILD_TESTS=OFF`, `BUILD_PERF_TESTS=OFF`,
    `BUILD_EXAMPLES=OFF`, `BUILD_opencv_apps=OFF`, `BUILD_JAVA=OFF`,
    `BUILD_opencv_python3=OFF`, `WITH_IPP=OFF`, `WITH_FFMPEG=OFF`,
    `WITH_OPENEXR=OFF`, `WITH_VTK=OFF`, `WITH_EIGEN=OFF`,
    `WITH_UNIFONT=OFF` (no KleidiCV switch — the option exists only on
    AArch64 Android/UNIX) — full command in `opencv-win.md`, section 2.

## Bundled third-party code compiled into the binaries

Unlike OCCT — whose sibling guides record "none" — OpenCV vendors codec and
support libraries under `3rdparty/` and compiles them statically into the
module binaries. With the switches above, these are the vendored components
compiled in; the table and the component versions were verified against
real configure summaries on both platforms (2026-10-13): macOS arm64 and
Windows x64. Verbatim license texts for each
component ship in this directory's `licenses/` folder and at the root of
every archive:

| Component | Vendored version | License | Compiled into |
|---|---|---|---|
| zlib | 1.3.2 | zlib license | imgcodecs (PNG), libtiff's compression |
| libjpeg-turbo | 3.1.2 | IJG + BSD-style (see its LICENSE.md roll-up) | imgcodecs (JPEG) |
| libpng | 1.6.57 | libpng license | imgcodecs (PNG) |
| libtiff | 4.7.1 | BSD-style (its LICENSE.md) | imgcodecs (TIFF) |
| libwebp | vendored snapshot | Google BSD-3 (COPYING) | imgcodecs (WebP) |
| OpenJPEG | 2.5.3 | BSD 2-clause | imgcodecs (JPEG 2000) |
| protobuf | 3.19.1 (bundled build) | BSD-3 | dnn |
| MLAS | vendored from ONNX Runtime | MIT | dnn (SGEMM kernels) — **macOS arm64 only** in these builds: on Windows x64 the MSVC/Ninja toolchain provides no ASM compiler, so MLAS auto-disables (dnn falls back to its built-in SGEMM) |
| flatbuffers | 25.9.23 (builtin) | Apache 2.0 (header-only) | dnn (TFLite importer) |
| dlpack | vendored | MIT (header-only) | tensor interchange headers |
| ittnotify | 3.25.4 | BSD-3-Clause (Intel); the vendored directory also carries a GPL-2.0-only text, installed alongside by the build | profiling hooks (ITT) |
| SoftFloat | vendored | Berkeley-style (COPYING.txt) | core (float16 support) |
| carotene | vendored | part of the OpenCV source tree (Apache 2.0) | core/imgproc HAL ("tegra_hal") — **macOS arm64 only** |
| Rubik font | vendored | SIL Open Font License 1.1 | imgproc (embedded drawing font) |
| annoy, mscr | vendored | per-file licenses (see their license files) | features (nearest-neighbor header, chi-table data) |

### Windows x64 differences (verified against the tag and the 2026-10-13 configure run)

- The codec and support set above is identical on Windows x64:
  `BUILD_ZLIB`/`BUILD_TIFF`/`BUILD_JPEG`/`BUILD_PNG`/`BUILD_WEBP`/
  `BUILD_OPENJPEG` default ON on WIN32 (as on APPLE), and protobuf,
  flatbuffers, dlpack, ittnotify, SoftFloat, the Rubik font, annoy, and
  mscr build from the vendored tree. **MLAS is the one exception**: its
  x86_64 SGEMM kernels are GNU-syntax assembly and the build requires an
  ASM compiler (`check_language(ASM)`, `3rdparty/mlas/CMakeLists.txt`),
  which the MSVC/Ninja toolchain does not provide — configure warns
  `MLAS: ASM language unavailable on AMD64` and dnn compiles with its
  built-in SGEMM instead (summary: `DNN MLAS: NO (no ASM compiler
  available for AMD64)`). A related performance note: libjpeg-turbo
  builds without SIMD (`SIMD extensions disabled: could not find NASM
  compiler`) — correctness unaffected.
- **built-in CLAPACK** — an additional component on Windows x64 only:
  OpenCV 5.0 requires LAPACK (the top-level CMakeLists asserts
  `HAVE_LAPACK`) and `BUILD_CLAPACK` defaults ON on WIN32, so configure
  builds the vendored CLAPACK 3.9.0 (`3rdparty/clapack`,
  `lapack_LICENSE`) as the "Built-In" LAPACK without searching the
  machine — no MKL/OpenBLAS leak is possible. Its static archive is not
  installed in shared builds (install rule gated
  `if(NOT BUILD_SHARED_LIBS)`); its license text installs as
  `clapack-lapack_LICENSE`.
- **carotene** — not built on Windows x64: the `WITH_CAROTENE` option is
  only created for ARM/AARCH64 targets (and the HAL disables itself
  without NEON). The `tegra_hal` row above therefore does not apply; the
  CPU dispatch is OpenCV's own SIMD code.
- **opencl-headers** — the vendored Khronos OpenCL 1.2 headers
  (`3rdparty/include/opencl/1.2`, `LICENSE.txt`, MIT) are used and their
  license text installed on non-Apple builds; macOS instead uses the
  system framework headers. No OpenCL library is linked on Windows — the
  runtime DLL is loaded dynamically when a program runs.
- **Not built on either platform in this configuration:** Intel IPP
  (`WITH_IPP=OFF`), the WQY MicroHei Unicode font (`WITH_UNIFONT=OFF`) —
  both configure-time *downloads* when enabled (the font is GPL-2.0 with
  a font embedding exception, the one non-permissive term in the set) —
  and **FFmpeg** (`WITH_FFMPEG=OFF`; on Windows that switch suppresses a
  download of prebuilt `opencv_videoio_ffmpeg` DLLs — LGPL — rather than
  a search), **OpenEXR** (`WITH_OPENEXR=OFF`), **VTK** (`WITH_VTK=OFF`),
  and **Eigen** (`WITH_EIGEN=OFF`) — stock defaults search the machine
  (Homebrew on macOS; CMake package paths and a `%ProgramFiles%` scan on
  Windows) — pinned off so the binaries carry no machine-package
  dependencies. **JasPer** — vendored but not built: the JPEG-2000 codec
  uses the bundled OpenJPEG whenever it is present (`WITH_JASPER AND NOT
  HAVE_OPENJPEG`, `cmake/OpenCVFindLibsGrfmt.cmake` at 5.0.0).
  Android's cpufeatures, libspng, zlib-ng, TBB, fastcv, orbbecsdk,
  libtim-vx — default-off or platform-gated.

The dev archive contains OpenCV's own per-component license directory —
`share/licenses/opencv5/` on macOS, `etc\licenses\` on Windows — carrying
the license texts the build installs (zlib, libpng, libjpeg-turbo,
libtiff, openjpeg, protobuf, ittnotify, SoftFloat, dlpack, flatbuffers,
the Rubik OFL text, mscr, annoy; Windows adds `clapack-lapack_LICENSE`
and `opencl-headers-LICENSE.txt`). The verbatim source texts for every
component ship in this directory's `licenses/` folder.

## External libraries linked by the binaries

- **macOS:** none redistributed. The only non-OpenCV code beyond the
  bundled table above comes from operating-system components supplied by
  macOS: the Cocoa framework (highgui), the AVFoundation/CoreMedia/
  CoreVideo frameworks (videoio), the Accelerate framework (vecLib
  BLAS/LAPACK), OpenCL, CoreFoundation, and libSystem — system-provided,
  none shipped in the archives.
- **Windows:** none redistributed. The only non-OpenCV code beyond the
  bundled table above comes from operating-system components supplied by
  Windows: system DLLs (KERNEL32, USER32, GDI32, OLE32, SETUPAPI, WS2_32,
  COMCTL32, and the DirectShow/Media Foundation/DirectX components the
  videoio and core interop paths use) — none shipped in the archives.
  The shared build links the dynamic CRT (`/MD`; `BUILD_WITH_STATIC_CRT`
  applies only to static-library builds, `cmake/OpenCVCRTLinkage.cmake`
  line 40 at the tag), so machines without Visual Studio need the
  Microsoft Visual C++ 2015–2022 Redistributable — the archives do not
  and should not ship the runtime. OpenCL, when a GPU driver provides
  it, is loaded dynamically at run time and is never linked or
  redistributed.