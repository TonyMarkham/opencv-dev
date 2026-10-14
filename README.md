# opencv-dev

Reproducible builds of **OpenCV 5.0.0** (upstream tag
[`5.0.0`](https://github.com/opencv/opencv/releases/tag/5.0.0),
commit `40738fb16ceddb5fb3fea747585f7ce6abb0605b`) with per-platform
build guides, a headless validation harness, and the packaged results
attached to this repository's
[Releases](https://github.com/TonyMarkham/opencv-dev/releases).

Three platforms, one recipe structure, one relative install shape under
`dist/`:

| Guide | Target | Toolchain | OpenCV's compiler floor |
|---|---|---|---|
| [`opencv-win.md`](opencv-win.md) | Windows x64 | MSVC (VS 2019/2022, `vc17` toolsets) | MSVC ≥ 19.14 |
| [`opencv-linux.md`](opencv-linux.md) | Linux x86_64 | GCC (Ubuntu 26.04 LTS base) | GCC ≥ 8.x |
| [`opencv-mac.md`](opencv-mac.md) | macOS arm64 | Apple Clang (Xcode Command Line Tools) | C++17-capable clang |

All guides assume CMake ≥ 3.13 and the Ninja generator, and produce
Release shared libraries built as C++17.

## What ships in the release

Six archives — per platform a **dev** zip (the full install tree:
shared libraries, headers, CMake package config, runtime data,
per-component licenses) and a **libs-only** zip (just the shared
libraries plus the three legal texts, flattened at the zip root):

| Archive | Contents |
|---|---|
| `opencv-5.0.0-x86_64-windows-dev.zip` | 16 module DLLs (`opencv_*500.dll`, versioned digits by design), MSVC import libraries, headers, `OpenCVConfig.cmake` at the prefix root, `setup_vars_opencv5.cmd` |
| `opencv-5.0.0-x86_64-windows-libs-only.zip` | the 16 DLLs, flattened |
| `opencv-5.0.0-x86_64-linux-dev.zip` | 16 unversioned `libopencv_*.so`, headers, `lib/cmake/opencv5`, `setup_vars_opencv5.sh` |
| `opencv-5.0.0-x86_64-linux-libs-only.zip` | the 16 `.so` files, flattened |
| `opencv-5.0.0-arm64-macos-dev.zip` | 16 unversioned `libopencv_*.dylib`, headers, `lib/cmake/opencv5`, `setup_vars_opencv5.sh` |
| `opencv-5.0.0-arm64-macos-libs-only.zip` | the 16 `.dylib` files, flattened |

Build configuration on every platform: Release, shared libraries, with
tests, perf tests, examples, apps, and Java/Python bindings off, the
vendored codec stack (zlib, libjpeg-turbo, libpng, libtiff, libwebp,
OpenJPEG) compiled in from `3rdparty/`, and every machine-detected
library (FFmpeg, GStreamer, AVIF, OpenEXR, VTK, Eigen, VA-API, Intel
IPP, the GPL Unicode font) pinned off — the binaries are
self-contained. The GUI backend is the platform's own system toolkit
(Win32UI, GTK3, Cocoa), so nothing third-party is added for windows.
The bundled-component inventory and per-platform provenance live in
[`third_party/opencv/THIRD_PARTY_NOTICES.md`](third_party/opencv/THIRD_PARTY_NOTICES.md),
which ships at the root of every archive.

## Using the dev archives

Extract anywhere and point CMake at the extraction root. Consumers
must build as C++17 (OpenCV 5.0 requires it on both sides):

```cmake
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
    # -DCMAKE_PREFIX_PATH=<extracted root>
target_link_libraries(my_app PRIVATE ${OpenCV_LIBS})
```

The shared libraries must be findable at runtime:

- **Windows** — the DLLs from `x64\vc17\bin\` on `PATH` (or next to
  the exe).
- **Linux** — `lib/` on `LD_LIBRARY_PATH`.
- **macOS** — `lib/` on `DYLD_LIBRARY_PATH`.

The libs-only zips are for *running* something already built against
these libraries — extract next to the executable or onto the platform's
library search path. They are not self-sufficient for building.

## Building from source

Each guide is self-contained: prerequisites → source patch (where
required) → configure/build/install → validation → packaging.

- **Windows** needs no source modification.
- **Linux and macOS** apply exactly one local modification (section 2.1
  of each guide): the `VERSION`/`SOVERSION` target properties in
  `cmake/OpenCVModule.cmake` are removed so the modules build as plain
  unversioned libraries with no symlink chain. The modification is
  recorded in a dated comment in the source and disclosed in
  `THIRD_PARTY_NOTICES.md`.

## Validation

`sample/` is the harness every guide runs after installing: it builds a
deterministic 320 × 240 BGR image, writes it to a PNG file, reads the
file back, asserts every pixel survived exactly, then runs a threshold
operation whose result is checked per pixel:

```
Image: 320x240 BGR
PNG written: hello_opencv.png
PNG read back: identical
Threshold check: exact
```

with exit code 0. That proves CMake package discovery, the C++17
headers, the shared libraries at runtime, and a real PNG codec round
trip through imgcodecs and imgproc — end to end.

## Repository layout

```
opencv-win.md          Windows build recipe
opencv-linux.md        Linux build recipe
opencv-mac.md          macOS build recipe
sample/                headless validation consumer (hello_opencv.cpp, CMakeLists.txt)
third_party/opencv/    verbatim license texts from the source tag + THIRD_PARTY_NOTICES.md
dist/                  gitignored — installed trees and packaged zips
tmp/                   gitignored — source checkouts, build trees, logs
```

## License

OpenCV is Apache License 2.0 — the verbatim text (`LICENSE`,
`COPYRIGHT`) lives in `third_party/opencv/` and ships at the root of
every archive; dev archives additionally carry the per-component
license texts (`share/licenses/opencv5/` on Linux/macOS, `etc\licenses\`
on Windows).

Two heavier-licensed components are deliberately off in these builds and
therefore absent from the archives: the WQY MicroHei Unicode font
(GPL-2.0 with font exception) and Intel IPP (Intel license). Each guide
documents how to enable them and the redistribution caveats that come
with doing so.
