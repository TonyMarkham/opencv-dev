# Building OpenCV on macOS

This repository builds **OpenCV 5.0.0** (tag `5.0.0`) from source on
**macOS (Apple Silicon, arm64)** and installs the result into the repo's
gitignored `dist/` tree. The install is validated with the headless
consumer in `sample/`, which builds a deterministic 320 × 240 BGR image,
writes it to a PNG file, reads the file back, asserts the round trip is
pixel-exact, and then runs a threshold operation whose result is checked
pixel by pixel against the expected value.

The repository keeps one recipe per platform with a shared structure
(`opencv-win.md`, `opencv-linux.md`); where content overlaps, keep the docs
consistent.

- Repo: <https://github.com/opencv/opencv>
- Tag: <https://github.com/opencv/opencv/releases/tag/5.0.0>
  (commit `40738fb16ceddb5fb3fea747585f7ce6abb0605b`; the tag itself is an
  annotated tag object)
- License: Apache License 2.0 (`LICENSE` in the source; carried into every
  archive this guide produces)

> ## Prerequisites
>
> Everything comes from two sources on macOS — Apple's **Xcode Command
> Line Tools** (compiler, git) and **Homebrew** (CMake, Ninja). No
> third-party bundles to download; `zip` ships with macOS itself.
>
> 1. Install the Command Line Tools (skip when a full Xcode or the CLT
>    is already installed):
>
>    ```
>    xcode-select --install
>    ```
>
>    A GUI prompt appears on a bare machine; if the tools are already
>    present, the command prints a note saying so. Either outcome is
>    fine — the `clang` check below is the gate.
> 2. Install Homebrew if absent (see <https://brew.sh> for the current
>    installer line), then:
>
>    ```
>    brew install cmake ninja
>    ```
>
>    If this aborts with `Your Xcode ... is too outdated` or `Your
>    Command Line Tools are too outdated` on a new macOS, that is
>    Homebrew's own toolchain policy, not this guide's requirement —
>    see Troubleshooting.
>
> The pieces, and why each is here:
>
> - Xcode CLT — Apple clang with C++17 support, plus `git`. OpenCV 5.0
>   defaults its libraries to C++17 and its CMake warns when a compiler
>   without C++17 support is detected (`cmake/OpenCVDetectCXXCompiler.cmake`
>   at 5.0.0); any Command Line Tools release of the last several years
>   clears it. The machine this guide was written against runs macOS
>   27.0.1. Keep Xcode and the Command Line Tools at consistent
>   versions — a mixed state breaks linking before OpenCV is even
>   involved (the recovery steps are in section 2.2a).
> - `cmake` — OpenCV declares
>   `cmake_minimum_required (VERSION "${MIN_VER_CMAKE}")` with
>   `MIN_VER_CMAKE 3.13` (`cmake/OpenCVMinDepVersions.cmake` at 5.0.0),
>   which CMake 4 accepts (CMake 4 removed compatibility only below 3.5).
> - `ninja` — the Ninja build generator.
> - `/usr/bin/zip` — Info-ZIP, part of macOS itself; used by the
>   packaging section. Nothing to install; the check below confirms it.
>
> No Homebrew *libraries* are needed. OpenCV vendors its still-image
> codecs (zlib, libjpeg-turbo, libpng, libtiff, libwebp, OpenJPEG) under
> `3rdparty/` and — on APPLE — builds them from that source by stock
> default (`BUILD_ZLIB`, `BUILD_TIFF`, `BUILD_JPEG`, `BUILD_PNG`,
> `BUILD_WEBP`, `BUILD_OPENJPEG` all default ON; top-level
> `CMakeLists.txt` at 5.0.0). GUI needs only the system Cocoa framework,
> video I/O the system AVFoundation framework, the parallel framework
> the system GCD, and the BLAS/LAPACK calls the system Accelerate
> framework.
>
> Three stock behaviors *would* touch the network at configure time, and
> this guide switches all three off (see section 2.2 for the switches):
>
> - **Intel IPP** (`WITH_IPP`, default ON) downloads an IPPICV archive
>   pinned by an MD5 in `3rdparty/ippicv/ippicv.cmake`.
> - **KleidiCV** (`WITH_KLEIDICV`, default ON on AArch64 UNIX) downloads
>   a KleidiCV source tarball pinned in `hal/kleidicv/kleidicv.cmake`
>   (`kleidicv-26.03.tar.gz`, MD5 `b85a745bfe0e87e67e30be9533eb6b24`).
> - **Unicode font** (`WITH_UNIFONT`, default ON) downloads the WQY
>   MicroHei font (MD5 pinned in `modules/imgproc/CMakeLists.txt`) and
>   compiles it into libopencv_imgproc for Unicode text drawing — and
>   its GPL-2.0-with-font-exception terms are the one non-permissive
>   license in OpenCV's dependency set. Pinned off here.
>
> With all three off, the entire build works from the clone alone — the
> same zero-download property the sibling OCCT guides have. Each is an
> optional accelerator or feature; how to enable them (and what that
> means for redistribution) is in the optional section below.
>
> A second leak class is specific to machines with Homebrew packages
> installed: with stock defaults, configure *searches* and links
> **FFmpeg** (`WITH_FFMPEG`), **OpenEXR** (`WITH_OPENEXR`), **VTK**
> (`WITH_VTK`), and **Eigen** (`WITH_EIGEN`) from Homebrew when they are
> present — a first configure on this machine produced all four YES
> lines before they were pinned off. FFmpeg/OpenEXR/VTK become runtime
> dependencies on Homebrew's dylib trees; Eigen is header-only but
> couples the binaries to that machine's package set. All four are
> pinned off for the same self-contained property; section 2.2 documents
> each switch.
>
> Run every command from the repository root:
>
> ```
> cd /Users/tony/git/opencv-dev
> ```
>
> After installing, confirm the toolchain resolves. Versions printed
> here are informational — anything the CLT and Homebrew give you clears
> OpenCV's floors (CMake ≥ 3.13, C++17-capable clang), so this is a
> check that the installs above actually happened, not a gate:
>
> ```
> clang --version | head -1
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
> xcrun --show-sdk-path
> ```
>
> The SDK this prints must belong to the same toolchain as the clang
> above — it should live under the developer directory that
> `xcode-select -p` names. A path under a *different* installation
> (Xcode selected, SDK under `/Library/Developer/CommandLineTools`, or
> the reverse) is the mixed state that breaks linking — the recovery
> steps are in section 2.2a.
>
> **A source patch is required on macOS.** Unlike OCCT's Windows guide,
> this guide makes exactly one local modification to the OpenCV source:
> `cmake/OpenCVModule.cmake` sets `VERSION`/`SOVERSION` target
> properties on every module, which on Mach-O produce versioned
> `libopencv_core.5.0.0.dylib` files plus a symlink chain
> (`libopencv_core.dylib → libopencv_core.500.dylib →
> libopencv_core.5.0.0.dylib` — the SOVERSION is the major version
> followed by the zero-padded two-digit minor, `OpenCVVersion.cmake`
> line 15 at 5.0.0). This repository installs plain unversioned
> `libopencv_*.dylib` files, so the patch (section 2.1) removes that
> block. No stock option produces unversioned module names at this tag
> (the CMake scripts have `OPENCV_*_SKIP` escapes for Windows version
> resources only, not for the library file names). The modification is
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
> OpenCV installs directly into `dist/opencv/arm64-macos`; there is no
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
> `tee`; run `set -o pipefail` first when scripting these stages (both
> `bash` and the default `zsh` support it), and otherwise check
> `echo $?` right after each stage — it must be `0`.

> ### 2.1 Apply the unversioned-library patch
>
> In `tmp/opencv/opencv/cmake/OpenCVModule.cmake`, replace this block
> (lines 1029–1037 at 5.0.0, verified against the tag):
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
>   # so that plain unversioned libopencv_*.dylib files are built and
>   # installed (no symlink chain).
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
> and that the original property lines are gone (must print nothing):
>
> ```
> grep -n "SOVERSION" tmp/opencv/opencv/cmake/OpenCVModule.cmake
> ```
>
> Two facts nearby, both verified at the tag and both fine to leave
> alone: `cmake/OpenCVVersion.cmake` still *defines* `OPENCV_SOVERSION`
> after the patch (it becomes an unused variable — harmless), and the
> `INSTALL_NAME_DIR "@rpath"` block in `cmake/OpenCVUtils.cmake` (lines
> ~1584–1606) is dead code here because it only runs for
> `APPLE_FRAMEWORK` builds, which this guide does not make.

> ### 2.2 Configure — generates the build plan; nothing compiles yet
>
> ```
> cmake -S tmp/opencv/opencv -B tmp/opencv/build -G Ninja \
>       -DCMAKE_BUILD_TYPE=Release \
>       -DCMAKE_INSTALL_PREFIX="$PWD/dist/opencv/arm64-macos" \
>       -DBUILD_SHARED_LIBS=ON \
>       -DBUILD_TESTS=OFF \
>       -DBUILD_PERF_TESTS=OFF \
>       -DBUILD_EXAMPLES=OFF \
>       -DBUILD_opencv_apps=OFF \
>       -DBUILD_JAVA=OFF \
>       -DBUILD_opencv_python3=OFF \
>       -DWITH_IPP=OFF \
>       -DWITH_KLEIDICV=OFF \
>       -DWITH_FFMPEG=OFF \
>       -DWITH_OPENEXR=OFF \
>       -DWITH_VTK=OFF \
>       -DWITH_EIGEN=OFF \
>       -DWITH_UNIFONT=OFF \
>       2>&1 | tee tmp/opencv/configure.log
> ```
>
> The switches:
>
> - `-S tmp/opencv/opencv` / `-B tmp/opencv/build` — source (the clone
>   from step 1) and build directory (all intermediate output; deleting
>   it deletes the build and nothing else).
> - `-G Ninja` — the Ninja generator; Homebrew's `ninja` supplies it.
> - `-DCMAKE_BUILD_TYPE=Release` — explicitly select an optimized build.
>   With a single-config generator this keeps the release output in
>   unsuffixed directories (`lib`, not `lib/Debug` variants).
> - `-DCMAKE_INSTALL_PREFIX=...` — OpenCV's install prefix (the standard
>   CMake variable; there is no OCCT-style `INSTALL_DIR` alias).
> - `-DBUILD_SHARED_LIBS=ON` — build `.dylib` files to distribute. This
>   is the default (except on Android/framework builds); spelled out for
>   clarity.
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
>   download and compiles without Intel's acceleration kernels.
> - `-DWITH_KLEIDICV=OFF` — no Arm KleidiCV: skips the configure-time
>   KleidiCV tarball download and compiles without it (carotene remains
>   the ARM HAL, built from `hal/carotene` in-tree).
> - `-DWITH_FFMPEG=OFF` — no FFmpeg in videoio. Stock default ON:
>   configure searches for a system ffmpeg via pkg-config and links it
>   when found (this machine's Homebrew has one — the first configure
>   run linked it). Video I/O keeps the system AVFoundation backend.
> - `-DWITH_OPENEXR=OFF` — no EXR codec. Stock default ON: with no
>   system OpenEXR nothing is linked, but a Homebrew OpenEXR would be
>   (this machine has one).
> - `-DWITH_VTK=OFF` — no VTK support and no viz integration. Stock
>   default ON; VTK means Homebrew's large VTK dylib tree.
> - `-DWITH_EIGEN=OFF` — no Eigen fast paths (OpenCV's built-in code is
>   used instead). Eigen is header-only, so nothing is linked, but
>   building against a Homebrew Eigen couples the binaries to that
>   machine's package set — off for reproducibility. Verified
>   consequence on this machine: the `geometry` module still builds, but
>   its posegraph-optimization part is compiled out (the configure log
>   notes "Eigen is Required for Posegraph optimization").
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
> - The bundled codecs (zlib, JPEG, PNG, TIFF, WebP, OpenJPEG) build
>   from `3rdparty/` and are linked statically into the modules —
>   nothing is searched for in Homebrew. (JasPer, also vendored, is not
>   built whenever OpenJPEG is present — `WITH_JASPER AND NOT
>   HAVE_OPENJPEG`, `cmake/OpenCVFindLibsGrfmt.cmake` at 5.0.0.)
> - The parallel framework resolves to the system GCD on macOS —
>   nothing to install.
> - `WITH_LAPACK` (default ON) resolves to the system Accelerate
>   framework on Apple platforms.
>
> A successful configure ends with `Configuring done` / `Generating done`
> (~21s on the machine this guide was written against). Three things in
> the log are expected and are not problems:
>
> - The summary's `Version control:` line reads `5.0.0-dirty` — the
>   section-2.1 patch dirties the source tree.
> - CMake policy deprecation warnings appear (`CMP0146`, `CMP0148` from
>   the top-level CMakeLists, `CMP0218`/`CMP0219` from OpenCV's own
>   scripts) — stock noise from the tag at CMake 4.x, harmless here.
> - `Found PythonInterp: /usr/bin/python3` appears even though the
>   bindings stay off — the interpreter is build tooling; the summary's
>   module list must still show `python3` under Unavailable.
>
> In the printed summary, the third-party section should show **no IPP**,
> **no KleidiCV**, **no FFMPEG**, no OpenEXR, no VTK, and no Eigen; the
> image-codec entries should come from the bundled builds; the Video I/O
> section should list only `GStreamer: NO` and `AVFoundation: YES`; the
> GUI section should show only Cocoa (no VTK support line); no
> "Built-in Unicode font" line should appear; the parallel framework
> should read GCD; and the Python/Java/bindings entries should read
> off. Any YES line for ffmpeg, OpenEXR, VTK, or Eigen means one of the
> leak switches above was dropped. If a previous configure attempt
> failed partway, delete the half-configured build directory first
> (`rm -rf tmp/opencv/build`) so its stale cache cannot interfere. If
> you previously customized install subdirectory cache variables in this
> build cache, make them match the switches above or use a fresh build
> directory.
>
> If configure instead fails with `Check for working C compiler ...
> - broken` and an `ld: tapi error`, go straight to section 2.2a.

> ### 2.2a Recovery — mixed Xcode / Command Line Tools (`ld: tapi error`)
>
> The machine's Xcode and Command Line Tools can end up mixed versions:
> the linker is older than the SDK it links against (`cc`/`xcrun` pick
> the SDK matching the running macOS from whichever installation has
> it). Select one consistent toolchain — the CLT set — then rebuild the
> plan:
>
> **R1** Select the CLT (asks for your password):
>
> ```
> sudo xcode-select --switch /Library/Developer/CommandLineTools
> ```
>
> **R2** Confirm the selection moved. Must print
> `/Library/Developer/CommandLineTools`:
>
> ```
> xcode-select -p
> ```
>
> **R3** Confirm the linker followed:
>
> ```
> ld -v 2>&1 | head -2
> ```
>
> **R4** Confirm the SDK resolves under the CLT. Must print
> `/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk`:
>
> ```
> xcrun --show-sdk-path
> ```
>
> **R5** Delete the half-configured build directory:
>
> ```
> rm -rf tmp/opencv/build
> ```
>
> **R6** Re-run the configure command above and continue with 2.3.

> ### 2.3 Build — about two minutes on the machine this guide was written against
>
> ```
> cmake --build tmp/opencv/build --parallel 2>&1 | tee tmp/opencv/build.log
> ```
>
> `--parallel` uses Ninja's default parallelism (all cores). Measured on
> the M4 this guide was written against: 1112 Ninja steps (1078
> compiles, 25+ links), ~1 minute 41 seconds wall time, build tree
> ~120 MB — budget minutes, not hours, on other machines. When it
> finishes, `echo $?` must be `0`. The
> `grep -inE "error|fatal" tmp/opencv/build.log` check prints exactly
> three lines here, all *file names* from the bundled codecs
> (`jerror.c`, `tif_error.c`, `pngerror.c`) and no actual errors; one
> benign linker note may appear (`ignoring duplicate libraries:
> '3rdparty/lib/libtegra_hal.a'`). Anything beyond that: resolve the
> first error in the log; rerunning install alone cannot produce
> missing files.

> ### 2.4 Install — copies headers, libraries, data into the dist
>
> Run this only after the build has completed successfully. If reusing a
> dist from an older installation, first delete its obsolete
> `libopencv_*.dylib` files; CMake does not remove stale installed
> files.
>
> ```
> cmake --install tmp/opencv/build 2>&1 | tee tmp/opencv/install.log
> ```
>
> Installed layout (`dist/opencv/arm64-macos/`, verified against a real
> install):
>
> ```
> <prefix>/
> ├── bin/
> │   └── setup_vars_opencv5.sh       env script: exports DYLD_LIBRARY_PATH
> │                                   pointing at ../lib
> ├── lib/
> │   ├── libopencv_core.dylib ...    (unversioned .dylib, one per module)
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
> No static third-party archives are installed: the bundled libraries
> compile directly into the module dylibs, and their ARCHIVE install
> rules run only `if(NOT BUILD_SHARED_LIBS)` (`3rdparty/zlib/
> CMakeLists.txt` at 5.0.0) — dormant in this shared build.
>
> Because of the 2.1 patch there are no version suffixes or symlink
> chains to check for — `libopencv_core.dylib` is a plain file. Verify
> the install (each line must print its OK message):
>
> ```
> test -f dist/opencv/arm64-macos/lib/libopencv_core.dylib && test ! -L dist/opencv/arm64-macos/lib/libopencv_core.dylib && echo "unversioned OK"
> ```
>
> ```
> test -f dist/opencv/arm64-macos/lib/cmake/opencv5/OpenCVConfig.cmake && echo "cmake config OK"
> ```
>
> ```
> test -f dist/opencv/arm64-macos/share/licenses/opencv5/zlib-LICENSE && echo "license OK"
> ```
>
> All three must print OK — the first is the proof the 2.1 patch took
> effect. The CMake package resolves its install prefix from the config
> file's own location, so consumers can point `CMAKE_PREFIX_PATH` at the
> prefix above (or at any directory containing it) and
> `find_package(OpenCV)` works.
>
> One more fact worth having before section 3 — what dyld sees:
>
> ```
> otool -D dist/opencv/arm64-macos/lib/libopencv_core.dylib
> ```
>
> prints the dylib's install name — expected
> `@rpath/libopencv_core.dylib`. That is CMake's macOS default with
> OpenCV's settings: OpenCV sets no `INSTALL_NAME_DIR` for non-framework
> builds, and with `cmake_minimum_required (VERSION 3.13)` the CMP0042
> policy defaults to NEW, which makes `@rpath` the install name.
> Consumers therefore resolve the dylibs through the rpath entries baked
> into the binaries and/or `DYLD_LIBRARY_PATH` — never through absolute
> library paths. (OpenCV does bake the install prefix into
> `CMAKE_INSTALL_RPATH` for its own targets, but that affects OpenCV's
> own test/sample binaries, not yours.)

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
> test -f dist/opencv/arm64-macos/lib/cmake/opencv5/OpenCVConfig.cmake && echo OK
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
> `dist/opencv/arm64-macos/...`:
>
> ```
> cmake -S sample -B sample/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PWD/dist/opencv/arm64-macos"
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
> **3.5** Make the OpenCV libraries findable at runtime. The dylibs
> carry `@rpath` install names (see the `otool -D` note in 2.4), so dyld
> expands them against the rpath entries baked into the binaries; this
> export adds a directory dyld always searches first and keeps the run
> deterministic regardless of how those entries came out. (The
> `setup_vars_opencv5.sh` script OpenCV installs under `bin/` performs
> the same export — and also appends a placeholder PYTHONPATH entry;
> harmless with the bindings off.) Lasts only
> for this session:
>
> ```
> export DYLD_LIBRARY_PATH="$PWD/dist/opencv/arm64-macos/lib:$DYLD_LIBRARY_PATH"
> ```
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
> OpenCV 5.0 headers, the dylibs load from `lib/` (via the `@rpath`
> install names plus the `DYLD_LIBRARY_PATH` fallback from 3.5), the
> bundled libpng+zlib stack encodes and decodes a real PNG, and imgproc
> computes a predictable result. It does not exercise video I/O, GUI
> windows, DNN inference, or deployment to another machine — but nothing
> about this path needs extra environment setup (`OPENCV_*` data
> variables are not required for this round trip).
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
>   config, static archives of the bundled third-party libraries,
>   runtime data, per-component licenses. For consumers who build
>   against OpenCV.
> - **libs-only** — just the `lib/*.dylib` files, flattened at the zip
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
> omits the platform whose binaries they contain. Confirm its macOS
> block is current for the build you are packaging (as committed, it
> matches: 2026-10-13, macOS 27.0.1, Apple clang 21.0.0 with Command
> Line Tools 27.0.0, CMake 4.4.4 + Ninja 1.13.2). If the toolchain or
> the build changed since — for example a KleidiCV-enabled rebuild —
> update the block before zipping:
>
> ```
> grep -n "macOS arm64" third_party/opencv/THIRD_PARTY_NOTICES.md
> ```
>
> ✅ Prints the macOS provenance line. If it prints nothing, the file is
> missing its macOS block — stop and fix the file before zipping.
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
> cd dist/opencv/arm64-macos
> ```
>
> **4.4** Build the dev zip from the whole install, preserving relative
> layout (a consumer extracts it anywhere and points
> `CMAKE_PREFIX_PATH` at the extraction root). The `.` stores the
> current directory's *contents* at the zip root — no `dist/opencv/...`
> prefix inside:
>
> ```
> zip -qr ../../zips/opencv-5.0.0-arm64-macos-dev.zip .
> ```
>
> **4.5** Back to the repo root — the remaining steps use
> `third_party/opencv/` paths. Three levels up from
> `dist/opencv/arm64-macos` (`cd ../..` would land in `dist/` — not far
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
> zip -qj dist/zips/opencv-5.0.0-arm64-macos-dev.zip third_party/opencv/THIRD_PARTY_NOTICES.md third_party/opencv/LICENSE third_party/opencv/COPYRIGHT
> ```
>
> **4.7** Build the libs-only zip — flattened `.dylib` files at the root:
>
> ```
> zip -qj dist/zips/opencv-5.0.0-arm64-macos-libs-only.zip dist/opencv/arm64-macos/lib/*.dylib
> ```
>
> **4.8** Add the same three legal files at the libs-only zip root:
>
> ```
> zip -qj dist/zips/opencv-5.0.0-arm64-macos-libs-only.zip third_party/opencv/THIRD_PARTY_NOTICES.md third_party/opencv/LICENSE third_party/opencv/COPYRIGHT
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
>   `.dylib` files and no symlinks, so the `lib/*.dylib` glob in 4.7
>   captures exactly the module libraries — no symlink targets or
>   versioned duplicates slip in. The bundled third-party libraries are
>   compiled into the dylibs, and their static archives are not
>   installed with shared builds (see 2.4), so nothing extra can slip
>   in either.
> - The libs-only zip is flat by design; extract it next to an
>   executable (or onto a `DYLD_LIBRARY_PATH`-listed directory) —
>   nothing inside references folders. It is *not* self-sufficient for
>   building: without headers and the CMake package, consumers cannot
>   link new code against these libraries.
> - The Apple linker ad-hoc-signs every binary it produces (the default
>   on arm64, per `ld(1)`); nothing in this guide signs or notarizes
>   beyond that.
> - Names pin the version on purpose (`5.0.0`): when swapping versions
>   (see that section below), re-run this section afterwards so the
>   archives never claim the wrong version.
> - The three legal files at each zip root come from this repository:
>   `LICENSE` is the verbatim Apache 2.0 text and `COPYRIGHT` the
>   verbatim copyright-attribution block from the OpenCV source tag, and
>   `THIRD_PARTY_NOTICES.md` records the provenance and the
>   bundled-component inventory (verified against the 5.0.0 tree —
>   unlike OCCT, OpenCV statically bundles its codec stack, so the
>   notices enumerate zlib, libjpeg-turbo, libpng, libtiff, libwebp,
>   OpenJPEG, JasPer, protobuf, MLAS, ittnotify, and the smaller vendored
>   pieces). Re-derive that inventory when swapping versions or changing
>   the configuration.

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
> `dist/opencv/arm64-macos` and the zips. Re-cloning the stock tag does
> **not** restore the build inputs exactly: the section-2.1 patch must
> be reapplied from this document. Unlike OCCT, OpenCV's configure does
> not call `export(PACKAGE)` — no entry is added to the user's CMake
> package registry, so there is nothing to clean up under
> `~/.cmake/packages`.

> ## Optional components and backends
>
> Each of these changes the build's character; reconfigure in a **fresh
> build directory** (`rm -rf tmp/opencv/build` or a new `-B` path) —
> stale caches leave half-set third-party variables behind. After any of
> them: re-run section 3 (validation), update the provenance block in
> `third_party/opencv/THIRD_PARTY_NOTICES.md` if the binary contents
> changed, and re-run section 4 so the zips match.
>
> ### Intel IPP (configure-time download)
>
> Remove `-DWITH_IPP=OFF`. Configure downloads the IPPICV archive
> (pinned by MD5 in `3rdparty/ippicv/ippicv.cmake`) into the build tree
> and links Intel's optimized kernels into the modules — the biggest
> single performance lever in OpenCV's CPU dispatch. Redistribution
> caveat: IPPICV is Intel-licensed, not Apache-2.0 — its terms differ
> and shipping it in the zips requires checking Intel's redistribution
> conditions first. Record the change in `THIRD_PARTY_NOTICES.md`.
>
> ### KleidiCV (configure-time download)
>
> Remove `-DWITH_KLEIDICV=OFF`. Configure downloads
> `kleidicv-26.03.tar.gz` (MD5 pinned in `hal/kleidicv/kleidicv.cmake`)
> and builds Arm's AArch64-optimized image-processing kernels as the
> OpenCV HAL. Check the tarball's own `LICENSE` for terms before
> redistributing, and record the component in `THIRD_PARTY_NOTICES.md`.
>
> ### TBB
>
> ```
> brew install tbb
> ```
>
> then add `-DWITH_TBB=ON`. OpenCV's parallel framework switches from
> the stock GCD (macOS) to TBB. Caveat: the dylibs then reference
> Homebrew's `libtbb*.dylib` by absolute install path, so binaries built
> this way depend on Homebrew being present on the consuming machine —
> carrying TBB into the zips is out of scope here.
>
> ### World build
>
> Add `-DBUILD_opencv_world=ON`: all modules fold into a single
> `libopencv_world.dylib` (consumers link `opencv_world` only). The
> libs-only zip then contains one dylib; the dev layout changes
> accordingly. Keep it OFF unless a consumer specifically wants the
> single-library form.
>
> ### Python bindings
>
> Requires a Python 3 interpreter plus numpy (Homebrew's python supplies
> both predictably). Remove `-DBUILD_opencv_python3=OFF`: the build
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
> extension linked against this repo's dylibs both resolve libraries by
> the same `@rpath/libopencv_*.dylib` install names, and dyld binds both
> to whichever copy loaded first, silently mixing versions. Keep them in
> separate processes (Python training → ONNX → C++ inference), which is
> the intended flow anyway.
>
> ### FFmpeg for more video formats
>
> ```
> brew install ffmpeg
> ```
>
> then reconfigure without `-DWITH_FFMPEG=OFF`: videoio gains
> ffmpeg-backed containers and codecs on top of AVFoundation. Same
> Homebrew-dependency caveat as TBB; the prebuilt ffmpeg *download* is
> Windows-only and never happens on macOS.
>
> ### OpenEXR / VTK / Eigen (system-detected libraries)
>
> Remove the matching `-DWITH_OPENEXR=OFF`, `-DWITH_VTK=OFF`, or
> `-DWITH_EIGEN=OFF` and reconfigure. On this machine all three resolve
> against Homebrew packages: OpenEXR gives imgcodecs an EXR codec and
> VTK gives highgui its viz support — both become Homebrew runtime
> dependencies (same caveat as TBB); Eigen is header-only but couples
> the binaries to that machine's package set.
>
> ### Built-in Unicode font
>
> Remove `-DWITH_UNIFONT=OFF` and reconfigure: the WQY MicroHei font is
> downloaded (pinned MD5) and compiled into libopencv_imgproc, giving
> `putText` Unicode coverage. Redistribution caveat: the font is
> GPL-2.0 with a font embedding exception — check the terms before the
> zips ship, and record the component in `THIRD_PARTY_NOTICES.md`.

> ## Useful switches (add to the `cmake -S` line)
>
> | Switch | What it does |
> |---|---|
> | `-DBUILD_opencv_world=ON` | One `libopencv_world.dylib` instead of per-module dylibs (see above). |
> | `-DOPENCV_GENERATE_PKGCONFIG=ON` | Generate `opencv5.pc` for pkg-config consumers. Stock default OFF (the option's own description calls it deprecated). |
> | `-DWITH_TBB=ON` | TBB parallel framework (`brew install tbb`; without it, GCD on macOS, pthreads elsewhere). |
> | `-DWITH_QT=ON` / `-DWITH_OPENGL=ON` | GUI toolkit / OpenGL interop for highgui; both default OFF (Cocoa needs no help). Qt comes from `brew install qt`. |
> | `-DENABLE_PRECOMPILED_HEADERS=ON` | Precompiled headers; stock default OFF on non-MSVC. |
> | `-DBUILD_LIST=core,imgproc,...` | Build only the listed modules (and their dependencies) — a smaller dist when the consumer set is known. |
> | `-DWITH_OPENCL=OFF` | Compile out the OpenCL paths (default ON; links only the system OpenCL framework). |
> | `-DOPENCV_ENABLE_NONFREE=ON` | Includes patent-encumbered algorithms (SIFT et al.). Changes the licensing story — do not combine with the zips without a legal review. |
> | `-DCV_DISABLE_OPTIMIZATION=ON` | Baseline build without SIMD dispatch (debug aid). |
>
> OpenCV 5.0 builds as C++17 by default (`CMAKE_CXX_STANDARD 17`,
> `cmake/OpenCVDetectCXXCompiler.cmake`), with `-std=c++17` (not
> gnu++17) and `CMAKE_CXX_STANDARD_REQUIRED` on. Do not lower the
> standard: the headers of a C++17-built OpenCV expect a C++17 consumer.
> The linker flag set on Apple targets adds `-Wl,-dead_strip`
> (unreferenced-code removal, not symbol stripping); binaries remain
> ad-hoc signed by the Apple linker as everywhere on arm64.

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
> 5.0). Install into a fresh prefix or delete the old installed files
> first; reinstalling does not clean them up. Repeat validation, then
> re-run section 4 — zip names pin the version.
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

> ## Troubleshooting
>
> - **`echo $?` prints `0` even though the stage failed** — you are
>   reading the exit code of `tee`, not of CMake. In a pipeline the exit
>   status comes from the last command; run `set -o pipefail` first when
>   scripting, or check the log's tail for the actual error.
> - **`Check for working C compiler ... - broken` with `ld: tapi error:
>   malformed file` / `unknown architecture arm64e.x1-macos`** — a
>   mixed-toolchain state: the linker is older than the SDK it links
>   against (Xcode and Command Line Tools from different releases
>   selected at once). The recovery steps live with the failure —
>   section 2.2a, steps R1–R6: select one consistent toolchain, verify,
>   delete `tmp/opencv/build`, reconfigure.
> - **`Your Xcode (...) is too outdated` / `Your Command Line Tools are
>   too outdated`** (aborting `brew install`) — Homebrew's own toolchain
>   policy, unrelated to OpenCV's floors: on new macOS releases it
>   requires up-to-date Xcode **and** Command Line Tools, and the two
>   checks are independent — an old `Xcode.app` in `/Applications` trips
>   the first check even when `xcode-select` points at the CLT. Fix by
>   updating Xcode from the App Store **and** installing the matching
>   Command Line Tools (Software Update, or the download from
>   <https://developer.apple.com/download/all/>), then re-run
>   `brew install cmake ninja`. OpenCV's own floors are not the issue.
> - **Configure fails or hangs on `IPPICV: Download ...` / `kleidicv`
>   download errors** — one of the two stock downloads ran, which means
>   the corresponding OFF switch was dropped from the configure line (or
>   you are deliberately enabling it — then check connectivity or point
>   the download at a local copy via the documented URL variables,
>   `OPENCV_IPPICV_URL` / `OPENCV_KLEIDICV_URL`). For the base recipe:
>   restore `-DWITH_IPP=OFF -DWITH_KLEIDICV=OFF`, delete
>   `tmp/opencv/build`, reconfigure.
> - **The summary shows `FFMPEG: YES`, an OpenEXR line, `VTK support:
>   YES`, or `Eigen: YES`** — the machine's Homebrew packages leaked
>   into the build: the corresponding OFF switch was dropped from the
>   configure line (a first configure on this machine linked all four
>   before they were pinned off). Delete `tmp/opencv/build`, restore
>   the full switch list from section 2.2, and reconfigure — ffmpeg,
>   OpenEXR, and VTK become Homebrew runtime dependencies, and a dist
>   built that way is not the self-contained one this guide describes.
> - **`Could not find a package configuration file provided by
>   "OpenCV"`** when configuring the sample — `CMAKE_PREFIX_PATH` must
>   point at the install prefix (`dist/opencv/arm64-macos`), under which
>   `lib/cmake/opencv5/OpenCVConfig.cmake` exists. A prefix set only at
>   install time (`--prefix`) does not work — the prefix is baked at
>   configure time.
> - **The sample keeps finding the wrong OpenCV after a rebuild** — the
>   sample's CMake *cache* pins `OpenCV_DIR`. OpenCV does not
>   self-register its build tree in the user package registry (no
>   `export(PACKAGE)` call, unlike OCCT), so a cache pin is the only way
>   this happens: delete `sample/build` and reconfigure with the prefix
>   set.
> - **`dyld: Library not loaded: ... libopencv_core.dylib ... Reason:
>   image not found`** when starting `hello_opencv` — the OpenCV `lib/`
>   directory is not reachable in that session. Re-run the
>   `export DYLD_LIBRARY_PATH=...` line from section 3.5 (a fresh
>   terminal loses it). To inspect the resolution state directly, use
>   the `otool -D` note from 2.4 on a dylib and
>   `otool -l sample/build/hello_opencv | grep -A2 LC_RPATH` on the
>   executable.
> - **`hello_opencv: FAILED: PNG write`** — `imwrite` could not encode
>   PNG, which means the bundled PNG codec is missing from the build
>   (the stock `BUILD_PNG=ON` default on APPLE was overridden).
>   Reconfigure with the recipe's switches in a fresh build directory.
> - **OpenCV headers fail with C++ errors in the sample** — the
>   consumer must build as C++17 (OpenCV 5.0 requires it on both
>   sides); `sample/CMakeLists.txt` sets it. Do not lower
>   `CMAKE_CXX_STANDARD`.
> - **`OpenCV 5.x requires C++17 support, but it was not detected`**
>   warnings in the configure log — the selected compiler is too old;
>   update Xcode or the Command Line Tools (`xcode-select --install`,
>   or Software Update), delete `tmp/opencv/build`, and reconfigure.
