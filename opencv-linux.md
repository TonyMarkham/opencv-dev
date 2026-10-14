# Building OpenCV on Linux

This repository builds **OpenCV 5.0.0** (tag `5.0.0`) from source on
**Ubuntu 26.04 LTS x86_64** and installs the result into the repo's
gitignored `dist/` tree. The install is validated with the headless
consumer in `sample/`, which builds a deterministic 320 × 240 BGR image,
writes it to a PNG file, reads the file back, asserts the round trip is
pixel-exact, and then runs a threshold operation whose result is checked
pixel by pixel against the expected value.

The repository keeps one recipe per platform with a shared structure
(`opencv-win.md`, `opencv-mac.md`); where content overlaps, keep the docs
consistent.

- Repo: <https://github.com/opencv/opencv>
- Tag: <https://github.com/opencv/opencv/releases/tag/5.0.0>
  (commit `40738fb16ceddb5fb3fea747585f7ce6abb0605b`; the tag itself is an
  annotated tag object)
- License: Apache License 2.0 (`LICENSE` in the source; carried into every
  archive this guide produces)

> ## Prerequisites
>
> Ubuntu supplies everything through `apt` — no third-party bundles to
> download. Run this on a fresh machine (or whenever in doubt — it is
> idempotent); nothing else is assumed to be pre-installed:
>
> ```
> sudo apt install build-essential cmake ninja-build git zip pkg-config libgtk-3-dev
> ```
>
> The packages, and why each is here:
>
> - `build-essential` — GCC/G++ with C++17 support. OpenCV 5.0 defaults
>   its libraries to C++17 and warns when the compiler is below its
>   floors: *OpenCV requires GCC >= 8.x* (`cmake/OpenCVDetectCXXCompiler.cmake`
>   lines 222–223 at 5.0.0) and *OpenCV 5.x requires C++17 support* when
>   the standard is not detected. Ubuntu 26.04 ships GCC 15.2 as the
>   default `gcc`/`g++` — far above both.
> - `cmake` — OpenCV declares
>   `cmake_minimum_required (VERSION "${MIN_VER_CMAKE}")` with
>   `MIN_VER_CMAKE 3.13` (`cmake/OpenCVMinDepVersions.cmake` at 5.0.0),
>   which CMake 4 accepts (CMake 4 removed compatibility only below 3.5).
>   Ubuntu 26.04 ships CMake 4.2.3.
> - `ninja-build`, `git`, `zip` — the Ninja build generator, the clone in
>   step 1, and the packaging section.
> - `pkg-config` — OpenCV detects the GUI toolkit through pkg-config
>   (`ocv_check_modules(GTK3 gtk+-3.0)`,
>   `modules/highgui/cmake/detect_gtk.cmake` at 5.0.0); the binary is
>   small, and spelling it here keeps the recipe working on a truly bare
>   machine even if a dependency stops pulling it in.
> - `libgtk-3-dev` — the GUI backend. On Linux, OpenCV's stock GUI is
>   GTK (`WITH_GTK`, default ON on UNIX non-Apple — top-level
>   `CMakeLists.txt` line 217): the analog of the system Cocoa framework
>   on macOS and Win32UI on Windows. With the dev package installed,
>   configure reports the GTK3 backend; without it, configure still
>   succeeds and highgui builds headless (`GUI: NO`) — the sample needs
>   no GUI either way, but this guide keeps the backend for parity with
>   the sibling platforms.
>
> Everything else the build touches is OpenCV itself: the still-image
> codecs (zlib, libjpeg-turbo, libpng, libtiff, libwebp, OpenJPEG) are
> vendored under `3rdparty/` and — **unlike macOS and Windows** — are
> *not* built from that source by stock default on Linux (see below),
> which is why section 2.2 pins them on. Nothing else is searched: with
> the switches in 2.2, the parallel framework resolves to pthreads, and
> the LAPACK calls go to OpenCV's vendored CLAPACK.
>
> Two stock behaviors *would* touch the network at configure time, and
> this guide switches both off (see section 2.2 for the switches):
>
> - **Intel IPP** (`WITH_IPP`, default ON on x86_64) downloads an IPPICV
>   archive — on Linux specifically
>   `ippicv_2026.0.0_lnx_intel64_20260327_general.tgz`, MD5
>   `9a3ee0c5c3c02102faa422d60bfd1f4a`, pinned in
>   `3rdparty/ippicv/ippicv.cmake` at 5.0.0.
> - **Unicode font** (`WITH_UNIFONT`, default ON) downloads the WQY
>   MicroHei font (MD5 pinned in `modules/imgproc/CMakeLists.txt`) and
>   compiles it into libopencv_imgproc for Unicode text drawing — and
>   its GPL-2.0-with-font-exception terms are the one non-permissive
>   license in OpenCV's dependency set. Pinned off here.
>
> The third stock download of the macOS recipe, **KleidiCV**
> (`WITH_KLEIDICV`), cannot happen on this platform at all: the option is
> only *created* on AArch64 Android/UNIX (`VISIBLE_IF` in the top-level
> CMakeLists; elsewhere the scripts never touch it), so there is nothing
> to switch off on x86_64 Linux — exactly as on Windows x64. ARM64 Linux
> builds are out of scope for this guide; if you ever attempt one, the
> macOS recipe's KleidiCV section is the reference.
>
> A second leak class is specific to machines with extra `*-dev` apt
> packages installed — the Linux analog of the macOS recipe's Homebrew
> leaks: with stock defaults, configure *searches* and links **FFmpeg**
> (`WITH_FFMPEG`), **GStreamer** (`WITH_GSTREAMER`, stock ON — line
> 214), **AVIF** (`WITH_AVIF`, stock ON on every platform — line 169;
> 5.0 vendors no AVIF codec), **OpenEXR** (`WITH_OPENEXR`), **VTK**
> (`WITH_VTK`), **Eigen** (`WITH_EIGEN`), and **VA-API**
> (`WITH_VA`/`WITH_VA_INTEL`, stock ON on x86_64 — lines 357–360) from
> the machine when their dev packages are present. This was not
> hypothetical on the machine this guide was written against: its
> first configure linked the system libavif 1.3.0 (summary
> `AVIF: avif (ver 1.3.0)`). FFmpeg/GStreamer/AVIF/OpenEXR/VTK/VA
> become runtime dependencies on apt's shared-library trees; Eigen is
> header-only but couples the binaries to that machine's package set.
> All are pinned off for the same self-contained property; section 2.2
> documents each switch. (GTK is the deliberate exception — it is
> installed on purpose above.)
>
> Run every command from the repository root:
>
> ```
> cd /home/tony/git/opencv-dev
> ```
>
> After installing, confirm the toolchain resolves. Versions printed
> here are informational — anything `apt` gives you on 26.04 clears
> OpenCV's floors (GCC ≥ 8, CMake ≥ 3.13), so this is a check that the
> install above actually happened, not a gate:
>
> ```
> g++ --version | head -1
> ```
>
> ```
> cmake --version | head -1
> ```
>
> ```
> ninja --version
> ```
>
> ```
> zip -v 2>&1 | head -1
> ```
>
> ```
> pkg-config --modversion gtk+-3.0
> ```
>
> **A source patch is required on Linux.** Like the macOS guide (and
> unlike the Windows guide), this guide makes exactly one local
> modification to the OpenCV source: `cmake/OpenCVModule.cmake` sets
> `VERSION`/`SOVERSION` target properties on every module, which on ELF
> systems produce versioned `libopencv_core.so.5.0.0` files plus a
> symlink chain (`libopencv_core.so → libopencv_core.so.500 →
> libopencv_core.so.5.0.0` — the SOVERSION is the major version followed
> by the zero-padded two-digit minor, `OpenCVVersion.cmake` line 15 at
> 5.0.0). This repository installs plain unversioned `libopencv_*.so`
> files, so the patch (section 2.1) removes that block. No stock option
> produces unversioned module names at this tag. The modification is
> kept recorded in a dated comment — it must be disclosed when the
> binaries are redistributed.

> ## 1. Get the OpenCV source at the release tag
>
> Clone the tag into the repo's gitignored source/build area:
>
> ```
> git clone --depth 1 --branch 5.0.0 https://github.com/opencv/opencv.git tmp/opencv/opencv
> ```
>
> Git prints a detached-HEAD note (`You are in 'detached HEAD' state`);
> that is expected when checking out a tag and can be ignored. If
> `tmp/opencv/opencv` already exists, use the existing `5.0.0` checkout
> rather than cloning over it — and make sure the section-2.1 patch is
> present in it (verify with the `grep` in 2.1).

> ## 2. Configure, build, install to the repo's dist
>
> Confirm step 1's clone is where the commands expect it (prints `clone
> OK`); the dist directory is allowed to not exist until 2.4:
>
> ```
> test -d tmp/opencv/opencv && echo "clone OK"
> ```
>
> OpenCV installs directly into `dist/opencv/x86_64-linux`; there is no
> external install and no copy step afterwards. The install prefix is
> **baked in at configure time** — the generated CMake package files
> resolve their prefix from their own installed location, but the build
> tree's rpath defaults and any consumer configured against the *build*
> tree derive from the prefix, so
> `cmake --install --prefix <elsewhere>` afterwards is not the way to
> relocate an install. Decide the prefix now.
>
> Each stage command pipes its output through `tee` into a log file in
> `tmp/opencv/` — `configure.log`, `build.log`, `install.log` — so
> progress streams to the console while the full output is captured for
> sharing a failed stage. In a pipeline the exit status comes from
> `tee`; run `set -o pipefail` first when scripting these stages, and
> otherwise check `echo $?` right after each stage — it must be `0`.

> ### 2.1 Apply the unversioned-library patch
>
> In `tmp/opencv/opencv/cmake/OpenCVModule.cmake`, replace this block
> (lines 1029–1037 at 5.0.0, verified against the tag — the same block
> the macOS guide patches):
>
> ```cmake
>   # For dynamic link numbering conventions
>   if(NOT ANDROID)
>     # Android SDK build scripts can include only .so files into final .apk
>     # As result we should not set version properties for Android
>     set_target_properties(${the_module} PROPERTIES
>       VERSION ${OPENCV_LIBVERSION}
>       SOVERSION ${OPENCV_SOVERSION}
>     )
>   endif()
> ```
>
> with:
>
> ```cmake
>   # Local modification 2026-10-13: removed the VERSION/SOVERSION
>   # set_target_properties block ("For dynamic link numbering conventions")
>   # so that plain unversioned libopencv_*.so files are built and
>   # installed (no symlink chain and no version suffix; the SONAME
>   # remains the plain libopencv_*.so filename instead of
>   # libopencv_*.so.500).
> ```
>
> The comment is not decoration: it marks the one local modification
> that must be preserved (and disclosed) when the binaries are
> redistributed. Verify the patch is in place (must print the dated
> comment line):
>
> ```
> grep -n "Local modification" tmp/opencv/opencv/cmake/OpenCVModule.cmake
> ```
>
> and that the original property lines are gone (must print nothing —
> probe `OPENCV_SOVERSION`, which appeared only in the removed block;
> a bare `SOVERSION` grep would trip on the disclosure comment above,
> which names the removed block):
>
> ```
> grep -n "OPENCV_SOVERSION" tmp/opencv/opencv/cmake/OpenCVModule.cmake
> ```
>
> Two facts nearby, both verified at the tag and both fine to leave
> alone: `cmake/OpenCVVersion.cmake` still *defines* `OPENCV_SOVERSION`
> after the patch (it becomes an unused variable — harmless), and
> `cmake/OpenCVInstallLayout.cmake` still sets `CMAKE_INSTALL_RPATH` to
> the prefix's `lib/` — that affects OpenCV's own test/sample binaries
> only, not consumers of the installed libraries.

> ### 2.2 Configure — generates the build plan; nothing compiles yet
>
> ```
> cmake -S tmp/opencv/opencv -B tmp/opencv/build -G Ninja \
>       -DCMAKE_BUILD_TYPE=Release \
>       -DCMAKE_INSTALL_PREFIX="$PWD/dist/opencv/x86_64-linux" \
>       -DBUILD_SHARED_LIBS=ON \
>       -DBUILD_ZLIB=ON \
>       -DBUILD_TIFF=ON \
>       -DBUILD_JPEG=ON \
>       -DBUILD_PNG=ON \
>       -DBUILD_WEBP=ON \
>       -DBUILD_OPENJPEG=ON \
>       -DBUILD_CLAPACK=ON \
>       -DBUILD_TESTS=OFF \
>       -DBUILD_PERF_TESTS=OFF \
>       -DBUILD_EXAMPLES=OFF \
>       -DBUILD_opencv_apps=OFF \
>       -DBUILD_JAVA=OFF \
>       -DBUILD_opencv_python3=OFF \
>       -DWITH_IPP=OFF \
>       -DWITH_AVIF=OFF \
>       -DWITH_FFMPEG=OFF \
>       -DWITH_GSTREAMER=OFF \
>       -DWITH_OPENEXR=OFF \
>       -DWITH_VTK=OFF \
>       -DWITH_EIGEN=OFF \
>       -DWITH_VA=OFF \
>       -DWITH_VA_INTEL=OFF \
>       -DWITH_UNIFONT=OFF \
>       2>&1 | tee tmp/opencv/configure.log
> ```
>
> The switches:
>
> - `-S tmp/opencv/opencv` / `-B tmp/opencv/build` — source (the clone
>   from step 1) and build directory (all intermediate output; deleting
>   it deletes the build and nothing else).
> - `-G Ninja` — the Ninja generator; `ninja-build` supplies it.
> - `-DCMAKE_BUILD_TYPE=Release` — explicitly select an optimized build.
>   With a single-config generator this keeps the release output in
>   unsuffixed directories (`lib`, not `lib/Debug` variants).
> - `-DCMAKE_INSTALL_PREFIX=...` — OpenCV's install prefix (the standard
>   CMake variable; there is no OCCT-style `INSTALL_DIR` alias).
> - `-DBUILD_SHARED_LIBS=ON` — build `.so` files to distribute. This is
>   the default (except on Android/framework builds); spelled out for
>   clarity.
> - `-DBUILD_ZLIB=ON -DBUILD_TIFF=ON -DBUILD_JPEG=ON -DBUILD_PNG=ON
>   -DBUILD_WEBP=ON -DBUILD_OPENJPEG=ON` — **the load-bearing
>   Linux-specific block.** On macOS and Windows these six default ON
>   and build the vendored codecs from `3rdparty/`; on Linux they do
>   not: the stock defaults are `(WIN32 OR ANDROID OR APPLE ...)` (their
>   `OCV_OPTION` lines are 146–152 in the top-level `CMakeLists.txt` at
>   5.0.0), so a stock Linux configure *searches* the machine for
>   system zlib, libjpeg, libpng, libtiff, libwebp, and OpenJPEG and
>   links whatever `*-dev` packages are installed — or leaves codecs
>   missing where none are. Pinned ON, the vendored copies are compiled
>   statically into the modules and nothing is searched: the zips become
>   self-contained the same way the sibling builds are.
> - `-DBUILD_CLAPACK=ON` — **the second Linux-specific pin.** OpenCV
>   5.0 requires LAPACK: with `WITH_LAPACK` at its stock ON, the
>   top-level CMakeLists asserts it — `ocv_assert(HAVE_LAPACK)  # Lapack
>   is required for OpenCV 5.0+` (line 775 at 5.0.0). On Windows the
>   `BUILD_CLAPACK` option defaults ON, so the vendored CLAPACK is used
>   without ever searching; on Linux it defaults OFF (the option's
>   condition is `(WIN32 OR ANDROID) AND NOT APPLE`, line 159) and
>   configure instead searches MKL → OpenBLAS → ATLAS → a generic
>   `cblas.h`/`lapacke.h` scan (`cmake/OpenCVFindLAPACK.cmake`), linking
>   whatever apt has — a machine dependency. Pinned ON, all searches are
>   skipped and the vendored `3rdparty/clapack` builds statically into
>   the modules — the Built-In implementation, exactly what the Windows
>   build gets by stock default. (The same vendored fallback would also
>   kick in on a bare machine with nothing found — but only after the
>   searches, so a machine with `libopenblas-dev` installed would leak
>   OpenBLAS into the binaries.)
> - `-DBUILD_TESTS=OFF` / `-DBUILD_PERF_TESTS=OFF` — skip the accuracy
>   and performance test executables. Both default **ON** on a native
>   build (`NOT INSTALL_CREATE_DISTRIB`), so these two switches are load
>   bearing: they cut a large amount of build time and disk, and with
>   them off the `ts` module auto-disables too
>   (`modules/ts/CMakeLists.txt` at 5.0.0). Validation in this guide
>   goes through section 3 instead.
> - `-DBUILD_EXAMPLES=OFF` — skip the C++ example programs (stock
>   default; spelled out so the recipe is stable against default
>   changes).
> - `-DBUILD_opencv_apps=OFF` — skip the utility applications
>   (annotation, classifier trainers, calibration tools). Stock default
>   is ON.
> - `-DBUILD_JAVA=OFF` — no Java/JNI bindings (stock default is ON on a
>   native build; disabling also skips the Java tooling search).
> - `-DBUILD_opencv_python3=OFF` — no Python `cv2` bindings. Stock
>   behavior is to *auto-detect* a Python 3 interpreter plus numpy and
>   build the bindings when both are found — nondeterministic across
>   machines, so pinned off. (There is no Python 2 in 5.0.)
> - `-DWITH_IPP=OFF` — no Intel IPP: skips the configure-time IPPICV
>   download (the *Linux* archive — see Prerequisites) and compiles
>   without Intel's acceleration kernels.
> - `-DWITH_FFMPEG=OFF` — no FFmpeg in videoio. Stock default ON on
>   Linux: configure searches for a system ffmpeg (via pkg-config in
>   `modules/videoio/cmake/detect_ffmpeg.cmake`) and links it when
>   found — a runtime dependency on apt's ffmpeg library tree, and LGPL
>   code besides. Off, video I/O keeps the kernel V4L2 backend (see the
>   no-switch list below).
> - `-DWITH_GSTREAMER=OFF` — no GStreamer in videoio. Stock default ON
>   on every desktop platform (`WITH_GSTREAMER`, top-level line 214) —
>   on Windows and macOS the search simply finds nothing, so their
>   summaries print `GStreamer: NO`; apt has GStreamer, and configure
>   would link it. Pinned off for the same machine-dependency reason as
>   FFmpeg. With the pin, the summary's Video I/O section shows no
>   GStreamer line at all (the line prints only when the option is on).
> - `-DWITH_AVIF=OFF` — no AVIF codec in imgcodecs. `WITH_AVIF` is
>   stock ON on every platform (top-level line 169) and searched with
>   CMake `find_package(libavif)`; OpenCV vendors no AVIF codec, so
>   whatever the machine has gets linked — on this machine the first
>   configure linked the system libavif 1.3.0 (summary
>   `AVIF: avif (ver 1.3.0)`), an apt runtime dependency exactly like
>   FFmpeg. (Windows and macOS machines without `libavif` dev files
>   print `AVIF: NO` on their own, which is why the sibling guides need
>   no switch.) With the pin, the search does not run and the Media
>   I/O section shows no AVIF line at all.
> - `-DWITH_OPENEXR=OFF` — no EXR codec. Stock default ON on Linux
>   (5.0 dropped the bundled OpenEXR on every platform, so there is no
>   in-tree fallback): configure searches the machine and links whatever
>   it finds — pinned off.
> - `-DWITH_VTK=OFF` — no VTK support and no viz integration. Stock
>   default ON; VTK means apt's large VTK library tree.
> - `-DWITH_EIGEN=OFF` — no Eigen fast paths (OpenCV's built-in code is
>   used instead). Eigen is header-only, so nothing is linked, but
>   building against the machine's `libeigen3-dev` couples the binaries
>   to that machine's package set — off for reproducibility.
> - `-DWITH_VA=OFF` / `-DWITH_VA_INTEL=OFF` — no VA-API (libva) video
>   acceleration hooks. Both stock ON on x86_64 (`WITH_VA`,
>   `WITH_VA_INTEL`, top-level lines 357–360) and both are machine
>   searches: bare systems report NO, a machine with `libva-dev` would
>   leak it into the videoio binaries. Pinned off.
> - `-DWITH_UNIFONT=OFF` — no built-in Unicode font; `putText` handles
>   ASCII only. Stock default ON downloads the WQY MicroHei font and
>   compiles it into libopencv_imgproc (GPL-2.0 with font exception).
>
> What needs **no** switch, verified at the tag:
>
> - `js` (WebAssembly) bindings build only under Emscripten; the
>   Objective-C bindings return early unless `APPLE_FRAMEWORK` is set
>   (`modules/objc/CMakeLists.txt`); the `ts` module auto-disables when
>   the test switches above are off.
> - `BUILD_opencv_world` defaults OFF (it is force-enabled only by
>   `INSTALL_CREATE_DISTRIB` with shared libs, which this guide does not
>   set).
> - **GTK** is stock ON on Linux — the `libgtk-3-dev` package from
>   Prerequisites supplies it; no switch needed and none wanted.
> - **V4L2** video capture needs only the kernel headers:
>   `modules/videoio/cmake/detect_v4l.cmake` runs a
>   `check_include_file(linux/videodev2.h ...)` — present on every Linux
>   build environment, no package, no download.
> - The parallel framework resolves to **pthreads** — `WITH_TBB`,
>   `WITH_OPENMP`, and `WITH_HPX` all stock OFF (`WITH_PTHREADS_PF`
>   stock ON); nothing to install.
> - **OpenCL** compiles in using the vendored Khronos headers
>   (`3rdparty/include/opencl/1.2`) and links **no** OpenCL library
>   (the runtime library is loaded dynamically when a program runs) —
>   the same situation the Windows guide documents.
> - The ITT profiling hooks build from the vendored `3rdparty/ittnotify`
>   (`BUILD_ITT` defaults ON on x86_64) — no download.
> - **MLAS** builds on Linux, unlike on Windows x64: its x86_64 SGEMM
>   kernels are GNU-syntax assembly and need an ASM compiler, which the
>   GNU toolchain provides (the MSVC/Ninja toolchain of the Windows
>   guide does not, so dnn there falls back to its built-in SGEMM).
> - **carotene** and **KleidiCV** are AArch64-only and are not built on
>   x86_64 — nothing to switch.
> - JasPer, also vendored, is not built whenever OpenJPEG is present —
>   `WITH_JASPER AND NOT HAVE_OPENJPEG` (`cmake/OpenCVFindLibsGrfmt.cmake`
>   at 5.0.0) — and `BUILD_OPENJPEG=ON` above keeps OpenJPEG present.
>
> A successful configure ends with `Configuring done` / `Generating
> done`. These things in the log are expected and are not problems:
>
> - The summary's `Version control:` line reads `5.0.0-dirty` — the
>   section-2.1 patch dirties the source tree.
> - CMake policy deprecation warnings *may* appear (`CMP0146`,
>   `CMP0148` from the top-level CMakeLists, `CMP0218`/`CMP0219` from
>   OpenCV's own scripts) — stock noise from the tag, harmless here.
>   This machine's CMake 4.2.3 emits none of them (the macOS guide's
>   CMake 4.4.4 emits all four); the exact set depends on the CMake in
>   use.
> - `Found PythonInterp: /usr/bin/python3` appears even though the
>   bindings stay off — the interpreter is build tooling; the summary's
>   module list must still show `python3` under Unavailable. (This
>   machine's interpreter is 3.14.4.) Right after it, a short Python
>   **`Traceback ... ModuleNotFoundError: No module named 'numpy'`**
>   block appears when numpy is not installed: the build probes numpy
>   for the bindings it is not going to build anyway
>   (`BUILD_opencv_python3=OFF`), so the traceback is harmless noise —
>   the summary still ends with the bindings off. Installing numpy
>   silences it; it changes nothing about the binaries.
> - `Lapack not found - fallback to built-in`, followed by
>   `LAPACK(Built-In): LAPACK_LIBRARIES: libclapack` and `Skipped LAPACK
>   build check` — expected with `BUILD_CLAPACK=ON`: the finder first
>   probes the machine, finds nothing, and supplies the vendored
>   CLAPACK. The "fallback" wording is the vendored path doing its job,
>   not a missing LAPACK.
> - `Could NOT find OpenJPEG ... OpenJPEG will be built from sources` —
>   expected: the finder probes the system first, finds no suitable
>   OpenJPEG, and the `BUILD_OPENJPEG=ON` pin supplies the vendored
>   2.5.3 (`JPEG 2000: build (ver 2.5.3)` in the summary).
> - `Package 'gtk+-2.0' not found` — the GTK detector probes GTK 2 as
>   well as GTK 3; only GTK 3 is wanted (`WITH_GTK_2_X` stock OFF) and
>   the summary reports `GUI: GTK3`.
> - `Geometry: Eigen support is disabled. Eigen is Required for
>   Posegraph optimization` — the verified consequence of
>   `-DWITH_EIGEN=OFF`: the `geometry` module still builds, but its
>   posegraph-optimization part is compiled out (the same note the
>   macOS guide records).
> - `SIMD extensions disabled: could not find NASM compiler. Performance
>   will suffer.` — libjpeg-turbo builds without its SIMD kernels unless
>   NASM is on PATH; encode/decode correctness is unaffected (`sudo apt
>   install nasm` and reconfigure in a fresh build directory if JPEG
>   throughput matters).
>
> In the printed summary (verified against a real run — the configure
> in `tmp/opencv/configure.log`, with every switch of this section
> including `-DWITH_AVIF=OFF`):
> `Version control:` reads `5.0.0-dirty` — no local modification beyond
> the 2.1 patch. The Media I/O section shows `build` entries for ZLib
> (1.3.2), JPEG (build-libjpeg-turbo 3.1.2, `SIMD Support: NO` — see
> the NASM note above), WEBP, PNG (`SIMD Support: YES (Intel SSE)`),
> TIFF (4.7.1), and JPEG 2000 (2.5.3), and **no AVIF line at all**
> (the section prints it only when the option is on). The Video I/O
> section shows `v4l/v4l2: YES (linux/videodev2.h)` and `Orbbec: YES`
> — Orbbec's 3D-camera support with the built-in V4L2 backend, stock
> `WITH_OBSENSOR=ON`, detected from the same kernel header and linking
> **no** library (the vendored `orbbecsdk` download path is the
> separate `OBSENSOR_USE_ORBBEC_SDK` option, stock OFF) — with **no
> FFMPEG or GStreamer lines at all** (both print only when the option
> is on). `Trace:` reads `YES (with Intel ITT(3.25.4))` — the vendored
> ittnotify from the no-switch list; `ONNX Runtime:` reads `NO` — the
> option is stock OFF and the line prints unconditionally, so do not
> read it as a search. The GUI section
> reads `GUI: GTK3` / `GTK+: YES (ver 3.24.52)` with no QT or VTK
> lines; no "Built-in Unicode font" line appears. `Parallel
> framework:` reads pthreads. `Lapack:` reads `YES (Built-In
> libclapack)`; `DNN MLAS:` reads `YES (SGEMM-only, vendored)` with
> `ASM kernels: YES (X86_64: SSE2, AVX, FMA3, AVX512F)` — the GNU
> toolchain supplies the assembly that Windows' MSVC/Ninja toolchain
> lacks, where that guide's summary reads NO. Any FFMPEG, GStreamer,
> or AVIF line, any OpenEXR/VTK/Eigen/VA YES line — or `Lapack: YES
> (OpenBLAS ...)` — means one of the leak switches above was dropped.
> If a previous configure attempt failed partway, delete
> the half-configured build directory first (`rm -rf tmp/opencv/build`)
> so its stale cache cannot interfere. If you previously customized
> install subdirectory cache variables in this build cache, make them
> match the switches above or use a fresh build directory.

> ### 2.3 Build — budget three to ten minutes on a typical desktop
>
> ```
> cmake --build tmp/opencv/build --parallel 2>&1 | tee tmp/opencv/build.log
> ```
>
> `--parallel` uses Ninja's default parallelism (all cores). To bound
> memory usage on a smaller machine, give it a job count, for example
> `--parallel 8`. The verified run of this guide (2026-10-13, all 16
> modules + the vendored codecs) took **1179 Ninja steps in 3 minutes
> 12 seconds** wall time — 772 C++ compiles, 364 C compiles, 25
> shared-library links, and 8 ASM/generate steps (the MLAS x86_64
> kernels among them); the build tree ended at ~177 MB. The step
> count is in the same range as the Windows build (~1200 Ninja steps
> — one compile/link per module per source file plus the vendored
> codecs), and GCC compiles more slowly than Apple clang: budget
> minutes, not hours, and keep a few GB free for the build tree.
> When it finishes, `echo $?` must be `0`. The
> `grep -inE "error|fatal" tmp/opencv/build.log` check prints exactly
> three lines here, all *file names* from the bundled codecs
> (`jerror.c`, `tif_error.c`, `pngerror.c`) and no actual errors —
> verified against the real log; a `grep -i warning` likewise matches
> only a file name (`tif_warning.c`). Anything beyond that: resolve
> the first error in the log; rerunning
> install alone cannot produce missing files.

> ### 2.4 Install — copies headers, libraries, data into the dist
>
> Run this only after the build has completed successfully. If reusing a
> dist from an older installation, first delete its obsolete
> `libopencv_*.so` files; CMake does not remove stale installed files.
>
> ```
> cmake --install tmp/opencv/build 2>&1 | tee tmp/opencv/install.log
> ```
>
> Installed layout (`dist/opencv/x86_64-linux/` — the UNIX branch of
> `cmake/OpenCVInstallLayout.cmake` at 5.0.0; same shape as the macOS
> install):
>
> ```
> <prefix>/
> ├── bin/
> │   └── setup_vars_opencv5.sh       env script: exports LD_LIBRARY_PATH
> │                                   pointing at ../lib
> ├── lib/
> │   ├── libopencv_core.so ...       (unversioned .so, one per module)
> │   └── cmake/opencv5/              OpenCVConfig.cmake,
> │                                   OpenCVConfig-version.cmake,
> │                                   OpenCVModules.cmake
> ├── include/opencv5/opencv2/        public headers, cvconfig.h,
> │                                   opencv_modules.hpp
> └── share/
>     ├── opencv5/                    valgrind suppression files
>     └── licenses/opencv5/           per-component license texts installed
>                                     by the build
> ```
>
> No static third-party archives are installed: the vendored libraries
> compile directly into the module `.so` files, and their ARCHIVE
> install rules run only `if(NOT BUILD_SHARED_LIBS)`
> (`3rdparty/zlib/CMakeLists.txt` at 5.0.0) — dormant in this shared
> build.
>
> Because of the 2.1 patch there are no version suffixes or symlink
> chains to check for — `libopencv_core.so` is a plain file. Verify
> the install (each line must print its OK message):
>
> ```
> test -f dist/opencv/x86_64-linux/lib/libopencv_core.so && test ! -L dist/opencv/x86_64-linux/lib/libopencv_core.so && echo "unversioned OK"
> ```
>
> ```
> test -f dist/opencv/x86_64-linux/lib/cmake/opencv5/OpenCVConfig.cmake && echo "cmake config OK"
> ```
>
> ```
> test -f dist/opencv/x86_64-linux/share/licenses/opencv5/zlib-LICENSE && echo "license OK"
> ```
>
> All three must print OK — the first is the proof the 2.1 patch took
> effect. The CMake package resolves its install prefix from the config
> file's own location, so consumers can point `CMAKE_PREFIX_PATH` at the
> prefix above (or at any directory containing it) and
> `find_package(OpenCV)` works.
>
> One more fact worth having before section 3 — what the ELF loader
> sees. With the 2.1 patch in place, each module's SONAME is its plain
> unversioned filename: with the VERSION/SOVERSION properties gone,
> CMake still passes a soname flag, but with the plain name — verified
> at 5.0.0, where
>
> ```
> objdump -p dist/opencv/x86_64-linux/lib/libopencv_core.so | grep -i soname
> ```
>
> prints exactly `SONAME  libopencv_core.so` (and `lib/` contains no
> versioned `.so.*` files and no symlinks). Because an ELF dependent
> records each dependency's *SONAME* in its `DT_NEEDED` entry, every
> inter-module reference is the plain name `libopencv_core.so` — the
> loader resolves those names through `LD_LIBRARY_PATH` and the
> standard system paths, never through absolute library paths. The
> SONAME is what makes the plain-name resolution work for transitive
> dependencies: without it, a module's dependents would record its
> absolute build-tree path. (Without the 2.1 patch, the SONAME would
> instead be `libopencv_core.so.500` and every consumer would demand
> exactly that versioned file plus its symlink chain — one more reason
> the patch is load-bearing on ELF.) To inspect a module's full
> dependency set:
>
> ```
> ldd dist/opencv/x86_64-linux/lib/libopencv_core.so
> ```
>
> The listed dependencies should be the other `libopencv_*.so` modules
> plus system libraries (libc, libpthread via libc, libdl via libc, and
> — through the GTK-enabled highgui — the GTK/GLib tree). Any
> non-system, non-OpenCV entry that is not GTK/GLib is a leaked
> third-party dependency — re-check the configure switches.

> ## 3. Validate with sample/ (headless PNG round trip)
>
> Section 2 left you a working OpenCV install. This section proves it
> works by running a small program from `sample/` that builds a
> deterministic 320 × 240 BGR image, writes it to a PNG file, reads the
> file back, asserts every pixel survived exactly, and then exercises
> imgproc with a threshold check whose expected result is recomputed per
> pixel. If the last printed line is `Threshold check: exact`, headers,
> shared libraries, and the bundled PNG codec stack all work — end to
> end.
>
> The program is two files:
>
> - `sample/hello_opencv.cpp` — the program: image → PNG → read back →
>   assert pixel-exact → grayscale → threshold → assert exact.
> - `sample/CMakeLists.txt` — its build file: declares C++17, finds the
>   installed OpenCV with components (`core`, `imgproc`, `imgcodecs`),
>   links `${OpenCV_LIBS}` (the requested modules plus their dependency
>   closure follow through the imported targets).
>
> Run these commands from the repo root, in order, one at a time:
>
> **3.1** Confirm the install from section 2 is present. Must print `OK`
> — if it prints nothing, finish section 2 before continuing:
>
> ```
> test -f dist/opencv/x86_64-linux/lib/cmake/opencv5/OpenCVConfig.cmake && echo OK
> ```
>
> **3.2** Remove any previous sample build (stale caches point at the
> wrong OpenCV):
>
> ```
> rm -rf sample/build
> ```
>
> **3.3** Point the sample at the install and generate its build plan.
> Success: a line starting with `-- Found OpenCV:` naming
> `dist/opencv/x86_64-linux/...`:
>
> ```
> cmake -S sample -B sample/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PWD/dist/opencv/x86_64-linux"
> ```
>
> If you instead see `Could not find a package configuration file
> provided by "OpenCV"`, see Troubleshooting.
>
> **3.4** Compile the sample (seconds, not minutes):
>
> ```
> cmake --build sample/build --parallel
> ```
>
> **3.5** Make the OpenCV libraries findable at runtime. Without this,
> the exe fails to start (`error while loading shared libraries:
> libopencv_core.so`). Lasts only for this session:
>
> ```
> export LD_LIBRARY_PATH="$PWD/dist/opencv/x86_64-linux/lib:$LD_LIBRARY_PATH"
> ```
>
> (The `setup_vars_opencv5.sh` script OpenCV installs under `bin/`
> performs the same export — and also appends a placeholder PYTHONPATH
> entry; harmless with the bindings off.)
>
> **3.6** Go where the built exe is:
>
> ```
> cd sample/build
> ```
>
> **3.7** Run the validation:
>
> ```
> ./hello_opencv
> ```
>
> **3.8** Print the exit code. Must be `0`:
>
> ```
> echo $?
> ```
>
> Expected on screen after 3.7:
>
> ```
> Image: 320x240 BGR
> PNG written: hello_opencv.png
> PNG read back: identical
> Threshold check: exact
> ```
>
> **3.9** Back to the repo root:
>
> ```
> cd ../..
> ```
>
> The run proves the full loop: CMake finds the installed package
> through `CMAKE_PREFIX_PATH`, a C++17 consumer compiles against the
> OpenCV 5.0 headers, the `.so` files load from `lib/` via
> `LD_LIBRARY_PATH` (the 2.4 `objdump` note explains why the plain
> names suffice), the vendored libpng+zlib stack encodes and decodes a
> real PNG, and imgproc computes a predictable result. It does not
> exercise video I/O, GUI windows, DNN inference, or deployment to
> another machine — but nothing about this path needs extra environment
> setup (`OPENCV_*` data variables are not required for this round
> trip).
>
> Note the sample asserts properties, not magic numbers: the PNG is
> lossless, so read-back equality is a hard assertion, and threshold is
> a point operation, so its per-pixel expected value is recomputed in
> the program. The PNG file lands in `sample/build/` and is gitignored
> with that directory.

> ## 4. Package the install as zips
>
> Section 2's install is a tree; this section turns it into two archive
> files under `dist/zips/`:
>
> - **dev** — everything: shared libraries, headers, CMake package
>   config, runtime data, per-component licenses. For consumers who
>   build against OpenCV.
> - **libs-only** — just the `lib/*.so` files, flattened at the zip
>   root. For consumers who only need to *run* something already built
>   against these libraries.
>
> Both archives carry the legal texts at the zip root (`LICENSE`,
> `COPYRIGHT`, `THIRD_PARTY_NOTICES.md`) — required when the archives
> are used in other repositories. The legal files live in the repo under
> `third_party/opencv/`; the dev archive additionally carries the
> per-component texts under `share/licenses/opencv5/` (installed by
> section 2).
>
> Run from the repo root, in order, one at a time:
>
> **4.1** Provenance first. `third_party/opencv/THIRD_PARTY_NOTICES.md`
> is a repo-tracked file that describes **all** platform builds, and the
> archives inherit it verbatim — the zips must never carry a record that
> omits the platform whose binaries they contain. As committed, the file
> covers macOS arm64 and Windows x64 and explicitly defers Linux
> (*"Linux gains its own block when its guide lands"*); this guide is
> that guide, so **add the Linux x86_64 block now**, before zipping —
> a build-inputs entry (the section-2.1 local modification plus the
> Linux-specific switch block: the six `BUILD_*` codec pins, the
> `BUILD_CLAPACK` pin, and the off pins from section 2.2) and a
> binary-provenance entry (build date, Ubuntu 26.04 LTS x86_64, the
> `g++ --version` and `cmake --version`/`ninja --version` output from
> Prerequisites), mirroring the shape of the existing macOS and Windows
> entries. Check the other blocks' wording stays accurate for this
> build while you are in there — in particular the MLAS row, which
> currently says *"macOS arm64 only in these builds"* and must now name
> Linux x86_64 as well (the GNU toolchain supplies the ASM compiler
> MSVC lacks). When the block is in place, confirm:
>
> ```
> grep -n "Linux x86_64" third_party/opencv/THIRD_PARTY_NOTICES.md
> ```
>
> ✅ Prints the Linux provenance lines. If it prints nothing, the file
> is still missing its Linux block — stop and fix the file before
> zipping.
>
> If the file changes after the zips were built, re-run 4.6 and 4.8 so
> the archives carry the updated file.
>
> **4.2** Create the output folder (gitignored, like all of `dist/`):
>
> ```
> mkdir -p dist/zips
> ```
>
> **4.3** Go into the install tree. Everything after this must run from
> the repo root again, so this and step 4.5 are a pair:
>
> ```
> cd dist/opencv/x86_64-linux
> ```
>
> **4.4** Build the dev zip from the whole install, preserving relative
> layout (a consumer extracts it anywhere and points
> `CMAKE_PREFIX_PATH` at the extraction root). The `.` stores the
> current directory's *contents* at the zip root — no `dist/opencv/...`
> prefix inside:
>
> ```
> zip -qr ../../zips/opencv-5.0.0-x86_64-linux-dev.zip .
> ```
>
> **4.5** Back to the repo root — the remaining steps use
> `third_party/opencv/` paths. Three levels up from
> `dist/opencv/x86_64-linux` (`cd ../..` would land in `dist/` — not far
> enough):
>
> ```
> cd ../../..
> ```
>
> **4.6** Add the three legal files at the dev zip root (`-j` junk-paths
> flattens them out of `third_party/opencv/`):
>
> ```
> zip -qj dist/zips/opencv-5.0.0-x86_64-linux-dev.zip third_party/opencv/THIRD_PARTY_NOTICES.md third_party/opencv/LICENSE third_party/opencv/COPYRIGHT
> ```
>
> **4.7** Build the libs-only zip — flattened `.so` files at the root:
>
> ```
> zip -qj dist/zips/opencv-5.0.0-x86_64-linux-libs-only.zip dist/opencv/x86_64-linux/lib/*.so
> ```
>
> **4.8** Add the same three legal files at the libs-only zip root:
>
> ```
> zip -qj dist/zips/opencv-5.0.0-x86_64-linux-libs-only.zip third_party/opencv/THIRD_PARTY_NOTICES.md third_party/opencv/LICENSE third_party/opencv/COPYRIGHT
> ```
>
> **4.9** Check both zips exist and are not empty:
>
> ```
> ls -lh dist/zips
> ```
>
> ✅ Exactly two `.zip` rows, both well above zero; dev is the larger of
> the two.
>
> Notes:
>
> - `zip` updates an existing archive silently (and asks nothing when
>   re-adding); to rebuild from scratch, delete the zip first.
> - Because of the 2.1 patch, `lib/` contains only plain unversioned
>   `.so` files and no symlinks, so the `lib/*.so` glob in 4.7 captures
>   exactly the module libraries — no symlink targets or versioned
>   duplicates slip in. The vendored third-party libraries are compiled
>   into the `.so` files, and their static archives are not installed
>   with shared builds (see 2.4), so nothing extra can slip in either.
> - The libs-only zip is flat by design; extract it next to an
>   executable (or onto an `LD_LIBRARY_PATH`-listed directory) —
>   nothing inside references folders. It is *not* self-sufficient for
>   building: without headers and the CMake package, consumers cannot
>   link new code against these libraries.
> - These zips are built for Ubuntu 26.04's runtime (glibc and the GTK
>   library versions of that release); moving them to older distros can
>   fail at load time. Record the target release in the provenance block
>   and treat the zips as tied to it.
> - Names pin the version on purpose (`5.0.0`): when swapping versions
>   (see that section below), re-run this section afterwards so the
>   archives never claim the wrong version.
> - The three legal files at each zip root come from this repository:
>   `LICENSE` is the verbatim Apache 2.0 text and `COPYRIGHT` the
>   verbatim copyright-attribution block from the OpenCV source tag, and
>   `THIRD_PARTY_NOTICES.md` records the provenance and the
>   bundled-component inventory (verified against the 5.0.0 tree —
>   OpenCV statically bundles its codec stack, so the notices enumerate
>   zlib, libjpeg-turbo, libpng, libtiff, libwebp, OpenJPEG, protobuf,
>   MLAS, ittnotify, and the smaller vendored pieces; on Linux the
>   inventory gains the built-in CLAPACK and the vendored Khronos
>   OpenCL headers, and — like macOS, unlike stock Windows — MLAS is
>   compiled in). Re-derive that inventory when swapping versions or
>   changing the configuration.

> ## 5. Optional cleanup
>
> After installation and validation, if you do not need the source/build
> tree for further work:
>
> ```
> rm -rf tmp/opencv
> ```
>
> This deletes the source checkout and the build output, but leaves
> `dist/opencv/x86_64-linux` and the zips. Re-cloning the stock tag does
> **not** restore the build inputs exactly: the section-2.1 patch must
> be reapplied from this document. OpenCV's configure does not call
> `export(PACKAGE)` — no entry is added to the user's CMake package
> registry (unlike OCCT, whose Linux configure self-registers), so
> there is nothing to clean up under `~/.cmake/packages`.

> ## Optional components and backends
>
> Each of these changes the build's character; reconfigure in a **fresh
> build directory** (`rm -rf tmp/opencv/build` or a new `-B` path) —
> stale caches leave half-set third-party variables behind. After any of
> them: re-run section 3 (validation), update the Linux block in
> `third_party/opencv/THIRD_PARTY_NOTICES.md` if the binary contents
> changed, and re-run section 4 so the zips match.
>
> ### Intel IPP (configure-time download)
>
> Remove `-DWITH_IPP=OFF`. Configure downloads the Linux IPPICV archive
> (`ippicv_2026.0.0_lnx_intel64_20260327_general.tgz`, pinned by MD5 in
> `3rdparty/ippicv/ippicv.cmake`) into the build tree and links Intel's
> optimized kernels into the modules — the biggest single performance
> lever in OpenCV's CPU dispatch. Redistribution caveat: IPPICV is
> Intel-licensed, not Apache-2.0 — its terms differ and shipping it in
> the zips requires checking Intel's redistribution conditions first.
> Record the change in `THIRD_PARTY_NOTICES.md`.
>
> ### TBB
>
> ```
> sudo apt install libtbb-dev
> ```
>
> then add `-DWITH_TBB=ON`. OpenCV's parallel framework switches from
> the stock pthreads to TBB. Caveat: the `.so` files then reference
> apt's `libtbb*.so` at load time, so binaries built this way depend on
> the TBB package being present on the consuming machine — carrying TBB
> into the zips is out of scope here.
>
> ### World build
>
> Add `-DBUILD_opencv_world=ON`: all modules fold into a single
> `libopencv_world.so` (consumers link `opencv_world` only). The
> libs-only zip then contains one `.so`; the dev layout changes
> accordingly. Keep it OFF unless a consumer specifically wants the
> single-library form.
>
> ### Python bindings
>
> Requires a Python 3 interpreter plus numpy (`sudo apt install
> python3-numpy` supplies both predictably; Ubuntu's `python3` is
> always present). Remove `-DBUILD_opencv_python3=OFF`: the build
> produces `cv2` and installs it into the prefix's Python package path.
> The bindings are coupled to the interpreter version they were built
> against — this is the least portable thing OpenCV installs, and the
> reason the base recipe pins it off.
>
> For this repository the plan is different: the Python side (training
> and inference scripts) gets its `cv2` from package-manager wheels
> (`uv pip install opencv-python`), not from this repo — those wheels
> are independent builds with their own bundled libraries, so neither
> the recipe nor the zips change for them. One hard boundary: never mix
> the two OpenCV copies in one process — a wheel's `cv2` and a C++
> extension linked against this repo's `.so` files both resolve
> libraries by the same plain `libopencv_*.so` names, and the loader
> binds both to whichever copy loaded first, silently mixing versions.
> Keep them in separate processes (Python training → ONNX → C++
> inference), which is the intended flow anyway.
>
> ### FFmpeg for more video formats
>
> ```
> sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libavdevice-dev
> ```
>
> then reconfigure without `-DWITH_FFMPEG=OFF`: configure finds the
> system ffmpeg via pkg-config and videoio gains ffmpeg-backed
> containers and codecs on top of V4L2. Same apt-dependency caveat as
> TBB. Redistribution caveat: FFmpeg is LGPL — check the terms before
> the zips ship, and record the component in
> `THIRD_PARTY_NOTICES.md`. The prebuilt-ffmpeg *download* of the
> Windows guide is Windows-only and never happens on Linux — the
> `-DOPENCV_FFMPEG_SKIP_DOWNLOAD=ON` / `OPENCV_FFMPEG_URL` escape
> hatches do not apply here; on Linux the only knobs are the apt
> packages and `WITH_FFMPEG` itself.
>
> ### GStreamer for more video formats
>
> ```
> sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
> ```
>
> then reconfigure without `-DWITH_GSTREAMER=OFF`: videoio gains the
> GStreamer backend (pipelines, not just files — the usual choice for
> camera/RTSP work). Same apt-dependency caveat as TBB.
>
> ### OpenEXR / VTK / Eigen / VA-API (system-detected libraries)
>
> Remove the matching `-DWITH_OPENEXR=OFF` (`sudo apt install
> libopenexr-dev`), `-DWITH_VTK=OFF` (`libvtk9-dev`), or
> `-DWITH_EIGEN=OFF` (`libeigen3-dev`) and reconfigure. All resolve
> against apt packages: OpenEXR gives imgcodecs an EXR codec and VTK
> gives highgui its viz support — both become apt runtime dependencies
> (same caveat as TBB); Eigen is header-only but couples the binaries
> to that machine's package set. VA-API (`-DWITH_VA=OFF` /
> `-DWITH_VA_INTEL=OFF`, `libva-dev`) adds hardware-accelerated video
> hooks to videoio — an apt dependency the same way. Note 5.0 does not
> bundle OpenEXR (unlike 4.x), so there is no in-tree fallback.
>
> ### Built-in Unicode font
>
> Remove `-DWITH_UNIFONT=OFF` and reconfigure: the WQY MicroHei font is
> downloaded (pinned MD5 in `modules/imgproc/CMakeLists.txt`) and
> compiled into libopencv_imgproc, giving `putText` Unicode coverage.
> Redistribution caveat: the font is GPL-2.0 with a font embedding
> exception — check the terms before the zips ship, and record the
> component in `THIRD_PARTY_NOTICES.md`.
>
> ### KleidiCV
>
> Not applicable on x86_64 Linux: the `WITH_KLEIDICV` option is only
> created on AArch64 Android/UNIX, so there is nothing to enable here.
> ARM64 Linux builds are out of scope for this guide; if you ever
> attempt one, the macOS recipe's KleidiCV section is the reference.

> ## Useful switches (add to the `cmake -S` line)
>
> | Switch | What it does |
> |---|---|
> | `-DBUILD_opencv_world=ON` | One `libopencv_world.so` instead of per-module `.so` files (see above). |
> | `-DOPENCV_GENERATE_PKGCONFIG=ON` | Generate `opencv5.pc` for pkg-config consumers. Stock default OFF (the option's own description calls it deprecated). |
> | `-DWITH_TBB=ON` | TBB parallel framework (`sudo apt install libtbb-dev`; without it, pthreads on Linux). |
> | `-DWITH_QT=ON` / `-DWITH_OPENGL=ON` | GUI toolkit / OpenGL interop for highgui; both default OFF (GTK needs no help). Qt6 comes from `sudo apt install qt6-base-dev`. OpenGL additionally needs `libgl-dev`. |
> | `-DENABLE_PRECOMPILED_HEADERS=ON` | Precompiled headers; stock default OFF — and unlike the Windows/Ninja build, PCH works with GCC under Ninja, so this is a real build-time lever here. |
> | `-DBUILD_LIST=core,imgproc,...` | Build only the listed modules (and their dependencies) — a smaller dist when the consumer set is known. |
> | `-DWITH_OPENCL=OFF` | Compile out the OpenCL paths (default ON; on Linux nothing is linked either way — vendored headers, runtime-loaded library). |
> | `-DOPENCV_ENABLE_NONFREE=ON` | Includes patent-encumbered algorithms (SIFT et al.). Changes the licensing story — do not combine with the zips without a legal review. |
> | `-DCV_DISABLE_OPTIMIZATION=ON` | Baseline build without SIMD dispatch (debug aid). |
>
> OpenCV 5.0 builds as C++17 by default (`CMAKE_CXX_STANDARD 17`,
> `cmake/OpenCVDetectCXXCompiler.cmake` lines 202–204), with
> `CMAKE_CXX_STANDARD_REQUIRED` on. Do not lower the standard: the
> headers of a C++17-built OpenCV expect a C++17 consumer. The GCC
> floor is 8.x — older compilers earn a configure warning (*OpenCV
> requires GCC >= 8.x ...*), and Ubuntu 26.04's GCC 15.2 is far above
> it.
>
> On the vendored codecs: the six `BUILD_*` pins of section 2.2 are what
> keep the zips self-contained on Linux. Do not "simplify" them away in
> favor of apt's `libpng-dev`/`libjpeg-dev`/... — that reintroduces the
> machine-dependency leak the sibling guides pin off, and the codec
> versions would then drift with the archive instead of being pinned by
> the vendored tree.

> ## Swapping versions
>
> Replace `5.0.0` in the step-1 clone command and re-run steps 2–3 — the
> dist is rebuilt from the new tag. OpenCV's tags are dotted with no
> prefix (`5.0.0`, `4.14.0`, `3.4.20`). If retaining the old source
> checkout, replace it with the requested tag before rebuilding and
> **re-apply the section-2.1 patch** — check the new tag's
> `cmake/OpenCVModule.cmake` first: the "For dynamic link numbering
> conventions" block must still be the `set_target_properties` block
> (its position shifts between tags; it is identical from 4.x through
> 5.0). Also re-check the Linux-specific switch block against the new
> tag's top-level `CMakeLists.txt`: the `BUILD_*` codec defaults
> (lines ~146–152), `BUILD_CLAPACK` (line ~159), the
> `ocv_assert(HAVE_LAPACK)` requirement (line ~775), and the stock-ON
> searches (`WITH_GSTREAMER`, `WITH_VA`, `WITH_VA_INTEL`, `WITH_AVIF`)
> can all shift or change condition between tags. Install into a fresh
> prefix or delete the old installed files first; reinstalling does not
> clean them up. Repeat validation, then re-run section 4 — zip names
> pin the version.
>
> Version-specific cautions:
>
> - **4.x ↔ 5.x install paths differ**: 4.x installs to
>   `include/opencv4`, `lib/cmake/opencv4`, `share/licenses/opencv4`;
>   5.x to the `opencv5` equivalents. The sample's `find_package(OpenCV)`
>   keeps working either way (the config file's location moves with the
>   names), but any path spellings in scripts or README tables must be
>   updated.
> - **C++ floor differs**: 4.x defaults to C++11 (and fatals if a lower
>   standard is forced); 5.x defaults to C++17. The sample's
>   `CMAKE_CXX_STANDARD` should match the libraries' standard.
> - **The bundled 3rd-party set changes between majors** — 5.0 dropped
>   the bundled OpenEXR, added MLAS, moved carotene under `hal/` — so
>   re-derive the bundled-component inventory in
>   `third_party/opencv/THIRD_PARTY_NOTICES.md` from the new tag's
>   `3rdparty/` tree and `3rdparty/readme.txt` (OpenCV's own summary of
>   the vendored libraries), and re-check the license files copied into
>   `third_party/opencv/licenses/`.
> - **The module set changes too** (5.0 split `calib3d` into
>   `calib`/`geometry`/`stereo`, renamed `features2d` to `features` and
>   dropped `ml`/`gapi` from the main repo). Consumers linking by module
>   name need the new names.
> - **The vendored-codec and CLAPACK defaults are per-platform**: on a
>   future tag where Linux gains stock-ON codec builds (or loses the
>   LAPACK assert), the pins may become redundant — but keep them until
>   the new tag's `OCV_OPTION` lines prove it.

> ## Troubleshooting
>
> - **`echo $?` prints `0` even though the stage failed** — you are
>   reading the exit code of `tee`, not of CMake. In a pipeline the exit
>   status comes from the last command; run `set -o pipefail` first when
>   scripting, or check the log's tail for the actual error.
> - **`OpenCV requires GCC >= 8.x` or `OpenCV 5.x requires C++17
>   support, but it was not detected`** warnings in the configure log —
>   the selected compiler is too old. Ubuntu 26.04's stock GCC 15.2 is
>   far above the floor; if a non-default compiler is on PATH
>   (`update-alternatives`, a toolchain module), reselect
>   `build-essential`'s default, delete `tmp/opencv/build`, and
>   reconfigure.
> - **`Could NOT find ZLIB` / configure errors naming jpeg, png, tiff,
>   webp, or openjpeg headers** — the six `BUILD_*` codec
>   pins were dropped from the configure line: on Linux the stock
>   defaults *search* instead of building the vendored codecs, and a
>   bare machine has none of the `*-dev` packages. Restore the full
>   switch list from section 2.2, delete `tmp/opencv/build`,
>   reconfigure. (The system-package alternative — installing
>   `zlib1g-dev libjpeg-dev libpng-dev libtiff-dev libwebp-dev
>   libopenjp2-7-dev` and dropping the pins — works but leaks apt
>   libraries into the binaries; see the closing note of *Useful
>   switches*.)
> - **`ocv_assert(HAVE_LAPACK)` fails / `Lapack not found` fatal** —
>   `-DBUILD_CLAPACK=ON` was dropped. OpenCV 5.0 requires LAPACK
>   (top-level `CMakeLists.txt` line 775); restore the pin, delete
>   `tmp/opencv/build`, reconfigure. (Installing `liblapack-dev` and
>   dropping the pin also satisfies the assert but links apt's LAPACK —
>   the leak the pin exists to avoid.)
> - **Configure fails or hangs on `IPPICV: Download ...`** — the Linux
>   IPPICV archive download ran, which means `-DWITH_IPP=OFF` was
>   dropped from the configure line (or you are deliberately enabling
>   IPP — then check connectivity or point the download at a local copy
>   via `OPENCV_IPPICV_URL`). For the base recipe: restore
>   `-DWITH_IPP=OFF`, delete `tmp/opencv/build`, reconfigure.
> - **The summary shows `FFMPEG: YES`, `GStreamer: YES`, an `AVIF`
>   line, an OpenEXR line, `VTK support: YES`, `Eigen: YES`, a `VA`
>   line, or `Lapack: YES (OpenBLAS ...)`** — the machine's apt dev
>   packages leaked into the build: the corresponding OFF/pinning
>   switch was dropped from the configure line (the first configure on
>   this machine leaked libavif this way, before `-DWITH_AVIF=OFF` was
>   added). Delete `tmp/opencv/build`, restore the full switch list
>   from section 2.2, and reconfigure — ffmpeg, GStreamer, libavif,
>   OpenEXR, VTK, and VA become apt runtime dependencies, and a dist
>   built that way is not the self-contained one this guide describes.
> - **`GUI: NO` in the summary when you expected GTK** — `libgtk-3-dev`
>   is not installed (or `pkg-config` is missing so the
>   `gtk+-3.0` probe cannot run). Install both (Prerequisites), delete
>   `tmp/opencv/build`, reconfigure. The build is otherwise healthy —
>   the sample does not need GUI — but the sibling platforms' builds
>   all carry a working window backend.
> - **`Could not find a package configuration file provided by
>   "OpenCV"`** when configuring the sample — `CMAKE_PREFIX_PATH` must
>   point at the install prefix (`dist/opencv/x86_64-linux`), under
>   which `lib/cmake/opencv5/OpenCVConfig.cmake` exists. A prefix set
>   only at install time (`--prefix`) does not work — the prefix is
>   baked at configure time.
> - **The sample keeps finding the wrong OpenCV after a rebuild** — the
>   sample's CMake *cache* pins `OpenCV_DIR`. OpenCV does not
>   self-register its build tree in the user package registry (no
>   `export(PACKAGE)` call, unlike OCCT), so a cache pin is the only way
>   this happens: delete `sample/build` and reconfigure with the prefix
>   set.
> - **`error while loading shared libraries: libopencv_core.so: cannot
>   open shared object file`** when starting `hello_opencv` — the
>   OpenCV `lib/` directory is not on `LD_LIBRARY_PATH` in that session.
>   Re-run the `export LD_LIBRARY_PATH=...` line from section 3.5 (a
>   fresh terminal loses it). To inspect the resolution state directly,
>   use the `objdump`/`ldd` notes from 2.4.
> - **`hello_opencv: FAILED: PNG write`** — `imwrite` could not encode
>   PNG, which means the vendored PNG codec is missing from the build
>   (a `BUILD_PNG=ON` pin was overridden, leaving the system search to
>   come up empty). Reconfigure with the recipe's switches in a fresh
>   build directory.
> - **OpenCV headers fail with C++ errors in the sample** — the
>   consumer must build as C++17 (OpenCV 5.0 requires it on both
>   sides); `sample/CMakeLists.txt` sets it. Do not lower
>   `CMAKE_CXX_STANDARD`.
> - **`file INSTALL cannot find .../libopencv_core.so`** — the build did
>   not finish. Check `tmp/opencv/build.log` for the first error,
>   rebuild to success, then run `cmake --install` again.
