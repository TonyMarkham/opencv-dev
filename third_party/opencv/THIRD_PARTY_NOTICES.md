# Third-party notices — OpenCV 5.0.0

These notices cover the binaries contained in the
`opencv-5.0.0-arm64-macos-*.zip` archives built from this repository.
(Windows and Linux archives gain their own provenance blocks when their
guides land; the macOS block below must stay current with the build it
packages — `opencv-mac.md`, section 4, checks it before zipping.)

## OpenCV

- **Source:** <https://github.com/opencv/opencv>, tag `5.0.0`
  (commit `40738fb16ceddb5fb3fea747585f7ce6abb0605b`; the tag itself is an
  annotated tag object, `9e2ede9628a55ec2742a1b180d3e69b0322281b9`).
  - **macOS build:** one local modification — the `VERSION`/`SOVERSION`
    target properties in `cmake/OpenCVModule.cmake` (the "For dynamic link
    numbering conventions" block, lines 1029–1037 at the tag) are removed so
    the modules build as plain unversioned `libopencv_*.dylib` files (see
    `opencv-mac.md`, section 2.1). The modification is part of the build
    inputs, not a binary patch.
- **License:** Apache License 2.0.
- **Full license text, included in every archive:** `LICENSE` (verbatim
  copy of the file at the source root), together with the project's
  copyright-attribution block (`COPYRIGHT`, also verbatim from the source
  root). The dev archive additionally carries the per-component license
  texts the OpenCV build installs under `share/licenses/opencv5/`.
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

## Bundled third-party code compiled into the dylibs

Unlike OCCT — whose sibling guides record "none" — OpenCV vendors codec and
support libraries under `3rdparty/` and compiles them statically into the
module dylibs. With the switches above, these are the vendored components
compiled in on macOS arm64; the inventory and the component versions were
verified against a real configure summary on this machine (2026-10-13).
Verbatim license texts for each component ship in this directory's
`licenses/` folder and at the root of every archive:

| Component | Vendored version | License | Compiled into |
|---|---|---|---|
| zlib | 1.3.2 | zlib license | imgcodecs (PNG), libtiff's compression |
| libjpeg-turbo | 3.1.2 | IJG + BSD-style (see its LICENSE.md roll-up) | imgcodecs (JPEG) |
| libpng | 1.6.57 | libpng license | imgcodecs (PNG) |
| libtiff | 4.7.1 | BSD-style (its LICENSE.md) | imgcodecs (TIFF) |
| libwebp | vendored snapshot | Google BSD-3 (COPYING) | imgcodecs (WebP) |
| OpenJPEG | 2.5.3 | BSD 2-clause | imgcodecs (JPEG 2000) |
| protobuf | 3.19.1 (bundled build) | BSD-3 | dnn |
| MLAS | vendored from ONNX Runtime | MIT | dnn (SGEMM kernels; builds on arm64) |
| flatbuffers | 25.9.23 (builtin) | Apache 2.0 (header-only) | dnn (TFLite importer) |
| dlpack | vendored | MIT (header-only) | tensor interchange headers |
| ittnotify | 3.25.4 | BSD-3-Clause (Intel); the vendored directory also carries a GPL-2.0-only text, installed alongside by the build | profiling hooks (ITT) |
| SoftFloat | vendored | Berkeley-style (COPYING.txt) | core (float16 support) |
| carotene | vendored | part of the OpenCV source tree (Apache 2.0) | core/imgproc HAL ("tegra_hal") |
| Rubik font | vendored | SIL Open Font License 1.1 | imgproc (embedded drawing font) |
| annoy, mscr | vendored | per-file licenses (see their license files) | features (nearest-neighbor header, chi-table data) |

Not compiled in this configuration:

- **Intel IPP** (`WITH_IPP=OFF`), **Arm KleidiCV** (`WITH_KLEIDICV=OFF`),
  and the **WQY MicroHei Unicode font** (`WITH_UNIFONT=OFF`) — all three
  are *downloads* performed at configure time when enabled; see the
  optional sections of `opencv-mac.md` for what enabling them means for
  provenance and licensing (the font is GPL-2.0 with a font embedding
  exception — the one non-permissive term in the set).
- **FFmpeg** (`WITH_FFMPEG=OFF`), **OpenEXR** (`WITH_OPENEXR=OFF`),
  **VTK** (`WITH_VTK=OFF`), and **Eigen** (`WITH_EIGEN=OFF`) — stock
  defaults search the machine and pick up Homebrew packages when
  present (a first configure on this machine linked all four before
  they were pinned off). Pinned off so the dylibs carry no Homebrew
  runtime dependencies: video I/O uses the system AVFoundation
  framework and there is no EXR codec.
- **JasPer** — vendored but not built: the JPEG-2000 codec uses the
  bundled OpenJPEG whenever it is present (`WITH_JASPER AND NOT
  HAVE_OPENJPEG`, `cmake/OpenCVFindLibsGrfmt.cmake` at 5.0.0).
- Android's cpufeatures, the Windows-only prebuilt ffmpeg bundle, libspng,
  zlib-ng, TBB, fastcv, orbbecsdk, libtim-vx — default-off or
  platform-gated.

The dev archive also contains OpenCV's own `share/licenses/opencv5/`
directory, carrying the per-component license texts the build installs
(zlib, libpng, libjpeg-turbo, libtiff, openjpeg, protobuf, ittnotify,
SoftFloat, dlpack, flatbuffers, the Rubik OFL text, mscr, annoy).

## External libraries linked by the binaries

- **macOS:** none redistributed. The only non-OpenCV code beyond the
  bundled table above comes from operating-system components supplied by
  macOS: the Cocoa framework (highgui), the AVFoundation/CoreMedia/
  CoreVideo frameworks (videoio), the Accelerate framework (vecLib
  BLAS/LAPACK), OpenCL, CoreFoundation, and libSystem — system-provided,
  none shipped in the archives.
