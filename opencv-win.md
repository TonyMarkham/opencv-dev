# Building OpenCV on Windows

This repository builds **OpenCV 5.0.0** (tag `5.0.0`) from source on
**Windows x64 (MSVC)** and installs the result into the repo's gitignored
`dist/` tree. The install is validated with the headless consumer in
`sample/`, which builds a deterministic 320 × 240 BGR image, writes it to
a PNG file, reads the file back, asserts the round trip is pixel-exact,
and then runs a threshold operation whose result is checked pixel by
pixel against the expected value.

The repository keeps one recipe per platform with a shared structure
(`opencv-mac.md`, `opencv-linux.md`); where content overlaps, keep the
docs consistent.

- Repo: <https://github.com/opencv/opencv>
- Tag: <https://github.com/opencv/opencv/releases/tag/5.0.0>
  (commit `40738fb16ceddb5fb3fea747585f7ce6abb0605b`; the tag itself is an
  annotated tag object)
- License: Apache License 2.0 (`LICENSE` in the source; carried into every
  archive this guide produces)

> ## Prerequisites
>
> - **Visual Studio 2019 or 2022** with the *Desktop development with
>   C++* workload. OpenCV 5.0.0 warns at configure time when MSVC is
>   older than 19.14 (VS 2017 15.7) —
>   `cmake/OpenCVDetectCXXCompiler.cmake` lines 232–234 at the tag — and
>   any VS 2019/2022 toolset clears it. The workload supplies the
>   compiler, CMake, and Ninja. The guide's paths assume **VS 2022**
>   (whose toolset OpenCV tags `vc17` — see section 2 for the mapping).
> - **Git for Windows** for the clone in step 1.
> - **No third-party downloads.** Everything the build touches is either
>   Windows SDK system libraries or code vendored under OpenCV's
>   `3rdparty/` tree. The stock options that *would* fetch anything from
>   the network are switched off in section 2 — see *Why no third-party
>   bundle?* below.
>
> **Run every command from the repository root in *Developer PowerShell
> for VS 2022*** (Start menu → the Visual Studio 2022 folder → Developer
> PowerShell for VS 2022). That shell configures the compiler
> environment; a plain PowerShell window does not, and CMake fails there
> with "Ninja not found" and no compiler. To convert the current window
> instead of opening a new one (adjust the edition folder if not
> `Community`):
>
> ```
> & "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64
> ```
>
> **Optional: pin this shell as a Windows Terminal profile.** Windows
> Terminal auto-creates a **Developer PowerShell for VS 2022** profile
> when VS 2022 is installed — check the tab-bar dropdown (`˅`) first. If
> it is there, duplicate it (Settings → that profile → *Duplicate*) and
> set its *Starting directory* to the repository root; the dedicated
> profile below is then optional. To create it explicitly with a pinned
> x64 toolset, use the settings UI (no JSON editing):
>
> 1. Windows Terminal → the dropdown (`˅`) beside the tabs → **Settings**
>    (or `Ctrl+,`).
> 2. Left sidebar, under *Profiles*: **Add a new profile** → **New empty
>    profile**.
> 3. *Name*: `Dev Pwsh (VS 2022, x64)`
> 4. *Command line*:
>    `pwsh.exe -NoExit -ExecutionPolicy Bypass -File "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64`
>    (paste exactly as written — the UI field takes the unescaped form).
> 5. *Starting directory*: `D:\git\opencv-dev` — the repo root; the
>    commands in this guide expand `$PWD` and assume it.
> 6. **Save** (bottom right). The profile appears in the dropdown.
>
> To edit the JSON file directly instead of using the UI:
>
> 1. Open the file: in the Settings page, click **Open JSON file** — the
>    small link in the bottom-left corner of the Settings window, next to
>    *Save* — or run from any shell:
>
>    ```
>    Invoke-Item "$env:LOCALAPPDATA\Packages\Microsoft.WindowsTerminal_8wekyb3d8bbwe\LocalState\settings.json"
>    ```
>
>    (that path is the Store/winget install; the *Open JSON file* link
>    works for every install channel).
> 2. Find `"profiles"` and, inside it, the `"list"` array of profile
>    objects. Paste the new entry after the last profile object, adding a
>    comma to the object before it:
>
>    ```json
>        { "...last existing profile..." },
>        {
>          "guid": "{CE111691-96CE-46F8-B2DB-048749CDB95C}",
>          "name": "Dev Pwsh (VS 2022, x64)",
>          "commandline": "pwsh.exe -NoExit -ExecutionPolicy Bypass -File \"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\Tools\\Launch-VsDevShell.ps1\" -Arch amd64",
>          "startingDirectory": "D:\\git\\opencv-dev"
>        }
>      ]
>    ```
>
>    The `guid` is technically optional for hand-added profiles (Terminal
>    mints one from the profile name when absent), but an explicit one
>    keeps the profile addressable — `wt -p {guid}`, `defaultProfile` —
>    and matches the shape of every profile Terminal itself writes.
>
> 3. Save. Terminal hot-reloads `settings.json` on save; the profile
>    appears in the dropdown immediately.
>
> JSON strings need doubled backslashes in paths (`D:\\git\\opencv-dev`),
> as shown. The launcher works under both `pwsh.exe` (PowerShell 7) and
> `powershell.exe` (Windows PowerShell 5.1) — use whichever host you
> prefer — and `-Arch amd64` pins the x64 toolset, whereas VS's
> auto-generated profile leaves the arch at the host default.
> `-ExecutionPolicy Bypass` keeps the profile working when script
> execution is policy-restricted. Whichever profile you use, still run
> the verification block above after the shell opens.
>
> After opening it, verify:
>
> ```powershell
> Get-Command cl, ninja, cmake | Format-Table Name, Source
> ```
>
> ```powershell
> $env:VSCMD_ARG_TGT_ARCH
> ```
>
> All three commands must resolve, and the arch variable must print
> `x64`. Commands in this guide use PowerShell's backtick (`` ` ``) for
> multi-line continuation: paste each block in one shot, and keep every
> backtick as the very last character of its line — a trailing space
> after a backtick breaks the command. Never paste cmd-style `^`
> continuations; PowerShell treats `^` as a stray argument.
>
> *Why no third-party bundle?* OpenCV vendors its still-image codecs
> (zlib, libjpeg-turbo, libpng, libtiff, libwebp, OpenJPEG) under
> `3rdparty/` and — on WIN32, as on APPLE — builds them from that source
> by stock default (`BUILD_ZLIB`, `BUILD_TIFF`, `BUILD_JPEG`,
> `BUILD_PNG`, `BUILD_WEBP`, `BUILD_OPENJPEG` all default ON; top-level
> `CMakeLists.txt` at 5.0.0). GUI needs only the Win32 libraries
> (`WITH_WIN32UI`, default ON — user32/gdi32 class), video I/O the
> system DirectShow and Media Foundation backends, the parallel
> framework MSVC's Concurrency (PPL). Unlike macOS, nothing searches a
> package manager — but three stock behaviors *would* touch the network
> at configure time, and this guide switches all three off (see section
> 2 for the switches):
>
> - **Intel IPP** (`WITH_IPP`, default ON on x64) downloads an IPPICV
>   archive — on Windows specifically
>   `ippicv_2026.0.0_win_intel64_20260327_general.zip`, MD5-pinned in
>   `3rdparty/ippicv/ippicv.cmake`.
> - **FFmpeg** (`WITH_FFMPEG`, default ON on non-Android) is, on
>   Windows, a **download of prebuilt binaries**: `modules/videoio/cmake/
>   detect_ffmpeg.cmake` fetches `opencv_videoio_ffmpeg.dll`,
>   `opencv_videoio_ffmpeg_64.dll`, and `ffmpeg_version.cmake` from
>   OpenCV's `opencv_3rdparty` GitHub branch (MD5-pinned in
>   `3rdparty/ffmpeg/ffmpeg.cmake`). This prebuilt-ffmpeg mechanism is
>   Windows-only — on macOS/Linux configure searches for a system
>   ffmpeg instead.
> - **Unicode font** (`WITH_UNIFONT`, default ON) downloads the WQY
>   MicroHei font (MD5 pinned in `modules/imgproc/CMakeLists.txt`) and
>   compiles it into libopencv_imgproc for Unicode text drawing — and
>   its GPL-2.0-with-font-exception terms are the one non-permissive
>   license in OpenCV's dependency set. Pinned off here.
>
> A fourth stock download, **KleidiCV** (`WITH_KLEIDICV`), cannot happen
> on this platform at all: the option is only *created* on AArch64
> Android/UNIX (`VISIBLE_IF` in the top-level CMakeLists; elsewhere the
> scripts never touch it), so there is nothing to switch off on x64
> Windows. The macOS recipe's `-DWITH_KLEIDICV=OFF` is deliberately
> absent from this guide's configure line.
>
> A second leak class is machine-installed packages, the Windows analog
> of the macOS recipes' Homebrew leaks: with stock defaults, configure
> *searches* and links **OpenEXR** (`WITH_OPENEXR`), **VTK**
> (`WITH_VTK`), and **Eigen** (`WITH_EIGEN`) from wherever they are
> installed — CMake package paths, and for Eigen a fallback scan of
> `%ProgramFiles%` (`cmake/OpenCVFindLibsPerf.cmake` at 5.0.0). All
> three are pinned off so the DLLs depend only on what ships with
> Windows; section 2 documents each switch.
>
> **LAPACK is not optional in OpenCV 5.0** — the top-level CMakeLists
> asserts `HAVE_LAPACK` ("Lapack is required for OpenCV 5.0+") — but on
> Windows the stock default needs no machine package: `BUILD_CLAPACK`
> defaults ON on WIN32, which sends the LAPACK detection straight to the
> vendored `3rdparty/clapack` tree ("Built-In" implementation) without
> ever searching for MKL or OpenBLAS. Leave it alone; the built-in
> CLAPACK is part of the bundled-component inventory the packaging step
> records.
>
> With all of the above off (or system-provided), the entire build works
> from the clone alone — the same zero-download property the sibling
> guides have.
>
> Run every command from the repository root:
>
> ```powershell
> cd D:\git\opencv-dev
> ```

> ## 1. Get the OpenCV source at the release tag
>
> Clone the tag into the repo's gitignored source/build area:
>
> ```
> git clone --depth 1 --branch 5.0.0 https://github.com/opencv/opencv.git tmp\opencv\opencv
> ```
>
> Git prints a detached-HEAD note (`You are in 'detached HEAD' state`);
> that is expected when checking out a tag and can be ignored. If
> `tmp\opencv\opencv` already exists, use the existing `5.0.0` checkout
> rather than cloning over it. Unlike the macOS recipe, this guide makes
> **no source modifications**, so a stock checkout is all that is needed
> (the reasoning is in section 2).

> ## 2. Configure, build, install to the repo's dist
>
> A dev shell gives no promise about the directory it opens in (the
> `Launch-VsDevShell.ps1` one-liner inherits whatever directory the
> window was in; Start-menu profiles open in the home directory unless
> the profile sets *Starting directory*). Pin it and confirm step 1's
> clone is where the commands expect it; the dist directory is allowed
> to not exist until section 2 finishes:
>
> ```powershell
> Test-Path tmp\opencv\opencv, dist\opencv\x86_64-windows
> ```
>
> The `Test-Path` must print `True` (the clone exists; the dist line is
> allowed to print `False`). OpenCV installs directly into
> `dist\opencv\x86_64-windows`; there is no external install and no copy
> step afterwards. The install prefix is **baked in at configure time** —
> decide it now; `cmake --install --prefix <elsewhere>` afterwards is
> not the way to relocate an install. (Left to its own devices, OpenCV
> would default the prefix to `build\install` on Windows — top-level
> `CMakeLists.txt` lines 99–101 at 5.0.0 — which is why every command
> below passes it explicitly.)
>
> **No CMake source patch is needed on Windows.** The macOS guide
> removes the `VERSION`/`SOVERSION` `set_target_properties` block from
> `cmake/OpenCVModule.cmake` ("For dynamic link numbering conventions",
> lines 1029–1037 at 5.0.0) to get plain unversioned
> `libopencv_*.dylib` files. On Windows that block is inert: CMake only
> applies `VERSION`/`SOVERSION` to ELF/Mach-O file names, and there are
> no library symlinks on Windows. Windows DLL names come from a
> different mechanism entirely — OpenCV's own `OPENCV_DLLVERSION`
> postfix (`${MAJOR}${MINOR}${PATCH}`, top-level `CMakeLists.txt` lines
> 558–561, applied as `OUTPUT_NAME` in `cmake/OpenCVModule.cmake` lines
> 1010–1020) — so the modules build as `opencv_core500.dll`,
> `opencv_imgproc500.dll`, ... with matching `opencv_core500.lib` import
> libraries, and the `5.0.0` version quad is embedded in each DLL's
> version resource (`vs_version.rc`, generated from
> `cmake/OpenCVVersion.cmake` lines 22–27). The `500` digits are stock
> Windows naming, present in every official OpenCV Windows build — not
> a patch artifact. Leave the file untouched.
>
> Each stage command below pipes its output through `Tee-Object` into a
> log file in `tmp\opencv\` — `configure.log`, `build.log`,
> `install.log` — so progress streams to the console while the full
> output is captured for sharing a failed stage. In Windows PowerShell
> 5.1, native stderr lines land in the log wrapped in error-record
> noise; PowerShell 7+ writes them cleanly — the text itself is
> unaffected. Check `$LASTEXITCODE` after each stage; it must be `0`
> (`$?` alone only says whether the pipeline ran, not whether CMake
> succeeded).
>
> **Configure** — generates the build plan; nothing compiles yet:
>
> ```
> cmake -S tmp\opencv\opencv -B tmp\opencv\build -G Ninja `
>       -DCMAKE_BUILD_TYPE=Release `
>       -DCMAKE_INSTALL_PREFIX="$PWD\dist\opencv\x86_64-windows" `
>       -DBUILD_SHARED_LIBS=ON `
>       -DBUILD_TESTS=OFF `
>       -DBUILD_PERF_TESTS=OFF `
>       -DBUILD_EXAMPLES=OFF `
>       -DBUILD_opencv_apps=OFF `
>       -DBUILD_JAVA=OFF `
>       -DBUILD_opencv_python3=OFF `
>       -DWITH_IPP=OFF `
>       -DWITH_FFMPEG=OFF `
>       -DWITH_OPENEXR=OFF `
>       -DWITH_VTK=OFF `
>       -DWITH_EIGEN=OFF `
>       -DWITH_UNIFONT=OFF `
>       2>&1 | Tee-Object tmp\opencv\configure.log
> ```
>
> The switches:
>
> - `-S tmp\opencv\opencv` / `-B tmp\opencv\build` — source (the clone
>   from step 1) and build directory (all intermediate output; deleting
>   it deletes the build and nothing else).
> - `-G Ninja` — the Ninja generator. It requires the compiler
>   environment of the developer shell (see Prerequisites). Ninja ships
>   with the VS C++ workload.
> - `-DCMAKE_BUILD_TYPE=Release` — explicitly select an optimized build.
>   With a single-config generator this keeps the release output in
>   unsuffixed directories (`bin`, not `bin\Debug` variants), and the
>   DLL names carry no debug suffix (the `d` postfix, top-level
>   `CMakeLists.txt` line 561, is Debug-only).
> - `-DCMAKE_INSTALL_PREFIX=...` — the install prefix (the standard
>   CMake variable; there is no OCCT-style `INSTALL_DIR` alias).
>   `$PWD` is the repo root; the whole switch is quoted so a path
>   containing spaces stays one argument.
> - `-DBUILD_SHARED_LIBS=ON` — build DLLs (plus MSVC import libraries)
>   to distribute. This is the default (except Android/framework
>   builds); spelled out for clarity.
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
>   native build; disabling also skips the Java/Ant tooling search).
> - `-DBUILD_opencv_python3=OFF` — no Python `cv2` bindings. Stock
>   behavior is to *auto-detect* a Python 3 interpreter plus numpy and
>   build the bindings when both are found — nondeterministic across
>   machines, so pinned off. (There is no Python 2 in 5.0.)
> - `-DWITH_IPP=OFF` — no Intel IPP: skips the configure-time IPPICV
>   download (the *Windows* archive — see Prerequisites) and compiles
>   without Intel's acceleration kernels.
> - `-DWITH_FFMPEG=OFF` — no FFmpeg in videoio. Stock default ON, and
>   on Windows that means downloading the prebuilt
>   `opencv_videoio_ffmpeg_64.dll` from the network (the mechanism is
>   Windows-only). Off, video I/O keeps the system DirectShow and Media
>   Foundation backends.
> - `-DWITH_OPENEXR=OFF` — no EXR codec. Stock default ON: 5.0 dropped
>   the bundled OpenEXR, so configure searches the machine (CMake
>   package paths) and links whatever it finds — pinned off.
> - `-DWITH_VTK=OFF` — no VTK support and no viz integration. Stock
>   default ON; VTK means an external (vcpkg-style) DLL tree.
> - `-DWITH_EIGEN=OFF` — no Eigen fast paths (OpenCV's built-in code is
>   used instead). Eigen is header-only, so nothing is linked, but
>   building against whatever Eigen sits in `%ProgramFiles%` couples
>   the binaries to that machine's package set — off for
>   reproducibility.
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
>   `INSTALL_CREATE_DISTRIB` with shared libs, which this guide does
>   not set).
> - The bundled codecs (zlib, JPEG, PNG, TIFF, WebP, OpenJPEG) build
>   from `3rdparty/` and are linked statically into the module DLLs —
>   nothing is searched for on the machine. (JasPer, also vendored, is
>   not built whenever OpenJPEG is present — `WITH_JASPER AND NOT
>   HAVE_OPENJPEG`, `cmake/OpenCVFindLibsGrfmt.cmake` at 5.0.0.)
> - The parallel framework resolves to **Concurrency** — MSVC's PPL,
>   detected with a `try_compile` in `cmake/OpenCVFindFrameworks.cmake`
>   — nothing to install. (TBB is stock OFF; GCD is Apple-only.)
> - **LAPACK** resolves to the vendored built-in CLAPACK (`3rdparty/
>   clapack`, "Built-In" implementation) because `BUILD_CLAPACK`
>   defaults ON on WIN32 — no machine search, no download. OpenCV 5.0
>   requires LAPACK, so the recipe leaves `WITH_LAPACK` at its default.
> - **OpenCL** compiles in using the vendored Khronos headers
>   (`3rdparty/include/opencl/1.2`) and links **no** OpenCL library on
>   Windows (`cmake/OpenCVDetectOpenCL.cmake` leaves `OPENCL_LIBRARY`
>   empty off-Apple; the runtime DLL is loaded dynamically when a
>   program runs). Nothing to install, nothing to switch off.
> - The GUI backend is **Win32 UI** (`WITH_WIN32UI`, default ON —
>   user32/gdi32); video I/O is **DirectShow** (`WITH_DSHOW`) and
>   **Media Foundation** (`WITH_MSMF`), both default ON and both part
>   of the OS.
> - The ITT profiling hooks build from the vendored `3rdparty/ittnotify`
>   (`BUILD_ITT` defaults ON on x64 MSVC) — no download.
> - Precompiled headers: the `ENABLE_PRECOMPILED_HEADERS` option
>   defaults ON on MSVC, but OpenCV's PCH support resolves NO outside
>   GCC and the MSBuild generator (`PCHSupport_FOUND`,
>   `cmake/OpenCVPCHSupport.cmake`) — under Ninja the summary reports
>   `Precompiled headers: NO` and nothing is switched.
>
> A successful configure ends with `Configuring done` / `Generating
> done` (~25s on the machine this guide was written against). These
> things in the log are expected and are not problems:
>
> - CMake policy warnings: deprecated-OLD warnings for `CMP0146` and
>   `CMP0148` (top-level CMakeLists), unset-policy warnings for
>   `CMP0177`, `CMP0218`, and `CMP0219` (OpenCV's own scripts) — stock
>   noise from the tag at CMake 4.x, harmless here. (The machine this
>   guide was written against has CMake 4.4.4 on PATH; the exact
>   warning set depends on the CMake in use.)
> - `Found PythonInterp` / `Found PythonLibs` lines and, when the
>   interpreter has no numpy, a Python `ModuleNotFoundError: No module
>   named 'numpy'` traceback — the numpy probe for the (disabled)
>   bindings; the summary's module list must still show `python3` under
>   Unavailable.
> - `Looking for a ASM compiler - NOTFOUND` plus the warning
>   `MLAS: ASM language unavailable on AMD64; MLAS disabled` and the
>   summary line `DNN MLAS: NO (no ASM compiler available for AMD64)` —
>   MLAS's x86_64 SGEMM kernels are GNU-syntax assembly, and MSVC/Ninja
>   provides no ASM compiler; dnn falls back to its built-in SGEMM.
>   Performance note only, no action.
> - `SIMD extensions disabled: could not find NASM compiler.
>   Performance will suffer.` — libjpeg-turbo builds without its SIMD
>   kernels unless NASM is on PATH; encode/decode correctness is
>   unaffected. Install NASM and reconfigure in a fresh build directory
>   if JPEG throughput matters.
> - `Could NOT find AVIF` — searched, not found, built-in codecs cover
>   the rest.
> - A warning about a *future* MSVC would mean the toolset is newer
>   than the tag recognizes — see Swapping versions and
>   Troubleshooting.
>
> In the printed summary (verified against a real run, 2026-10-13):
> `Version control:` reads `5.0.0` — no patch on Windows, so the tree
> is not dirty. The Video I/O section shows **no FFMPEG line at all**
> (the section prints it only when the option is on), `GStreamer: NO`,
> `DirectShow: YES`, `Media Foundation: YES` with `DXVA: YES`, and
> `Orbbec: YES` (UVC camera support, built-in). The GUI section reads
> `GUI: WIN32UI` / `Win32 UI: YES` with no QT, GTK, or VTK lines; no
> "Built-in Unicode font" line appears. `Parallel framework:` reads
> Concurrency. `Lapack:` reads `YES (Built-In libclapack)`.
> `DNN MLAS:` reads NO (see above). `OpenCL:` reads YES with
> `Include path:` naming the vendored `3rdparty/include/opencl/1.2` and
> `Link libraries: Dynamic load` — nothing is linked. The Media I/O
> section shows `build` entries for ZLib (1.3.2), JPEG
> (build-libjpeg-turbo 3.1.2, `SIMD Support: NO`), WEBP, PNG
> (`SIMD Support: YES (Intel SSE)`), TIFF (4.7.1), and JPEG 2000
> (2.5.3), plus `AVIF: NO`. The module list shows `world` under
> Disabled and `java python3 ts` under Unavailable, and
> `Install to:` names `dist/opencv/x86_64-windows`. Any
> `FFMPEG: YES (prebuilt binaries)`, OpenEXR, VTK, or Eigen YES line
> means one of the leak switches above was dropped. If a previous
> configure attempt failed partway, delete the half-configured build
> directory first (`Remove-Item -Recurse -Force tmp\opencv\build`) so
> its stale cache cannot interfere. If you previously customized
> install subdirectory cache variables in this build cache, make them
> match the switches above or use a fresh build directory.
>
> **Build** — about five minutes on the machine this guide was written
> against (measured 2026-10-13):
>
> ```
> cmake --build tmp\opencv\build --parallel 2>&1 | Tee-Object tmp\opencv\build.log
> ```
>
> `--parallel` uses Ninja's default parallelism (all cores; OpenCV also
> adds MSVC's `/MP` for per-file parallelism). To bound memory usage,
> give it a job count, for example `--parallel 8`. Measured on the
> machine this guide was written against: 1198 Ninja steps (16 module
> DLLs linked into `bin\`), ~4¾ minutes wall time, build tree ~1.8 GB —
> budget under 10 minutes and keep a few GB free on other machines.
> When it finishes, `$LASTEXITCODE` must be `0`. A
> `Select-String -Pattern "error" tmp\opencv\build.log` prints exactly
> three lines on this build, all *file names* from the bundled codecs
> (`jerror.c`, `tif_error.c`, `pngerror.c`) and no actual errors; a
> `"warning"` scan likewise hits only `tif_warning.c`; there are no
> linker diagnostics. Anything beyond that: resolve the first error in
> the log; rerunning install alone cannot produce missing files.
>
> **Install** — copies headers, libraries, data into the dist:
>
> Run this only after the build has completed successfully. If reusing
> a dist from an older installation, first delete its obsolete
> `opencv_*.dll`/`opencv_*.lib` files; CMake does not remove stale
> installed files.
>
> ```
> cmake --install tmp\opencv\build 2>&1 | Tee-Object tmp\opencv\install.log
> ```
>
> Installed layout (`dist\opencv\x86_64-windows\` — the install rules
> are `cmake/OpenCVInstallLayout.cmake` + `cmake/OpenCVGenConfig.cmake`
> at 5.0.0). The binaries nest under a toolset tag derived from the
> compiler that configured the build: VS 2022's cl (MSVC 19.30–19.4x)
> writes `x64\vc17`, VS 2019 writes `x64\vc16`, and the post-2022
> toolsets (MSVC 19.5x) write `x64\vc18` — the mapping lives in
> `cmake/OpenCVDetectCXXCompiler.cmake`. This guide spells `vc17`:
>
> ```
> <prefix>\
> ├── OpenCVConfig.cmake            root CMake package config (+
> │                                 OpenCVConfig-version.cmake) — Windows
> │                                 installs put the package at the prefix
> │                                 root, not under lib\cmake\opencv5
> ├── setup_vars_opencv5.cmd         env script: prepends x64\vc17\bin
> │                                 to PATH (its Windows analog of the
> │                                 macOS bin\setup_vars_opencv5.sh)
> ├── x64\vc17\
> │   ├── bin\                       opencv_core500.dll ... (one DLL per
> │   │                              module, versioned digits by design)
> │   └── lib\                       opencv_core500.lib ... (import
> │                                  libraries) + OpenCVConfig.cmake,
> │                                  OpenCVConfig-version.cmake,
> │                                  OpenCVModules.cmake
> ├── include\opencv2\               public headers, cvconfig.h,
> │                                  opencv_modules.hpp
> └── etc\licenses\                  per-component license texts
>                                    installed by the build
> ```
>
> No static third-party archives are installed: the bundled libraries
> compile directly into the module DLLs, and their ARCHIVE install
> rules run only `if(NOT BUILD_SHARED_LIBS)` (`3rdparty/zlib/
> CMakeLists.txt` at 5.0.0) — dormant in this shared build. PDB files
> are also absent: OpenCV adds PDB install rules on MSVC shared builds,
> but places them in a `pdb` component marked `EXCLUDE_FROM_ALL`
> (`cmake/OpenCVUtils.cmake` lines 1259–1322), which plain
> `cmake --install` does not process.
>
> Verify the install (each line must print `True`):
>
> ```powershell
> Test-Path dist\opencv\x86_64-windows\OpenCVConfig.cmake
> ```
>
> ```powershell
> Test-Path dist\opencv\x86_64-windows\x64\vc17\bin\opencv_core500.dll
> ```
>
> ```powershell
> Test-Path dist\opencv\x86_64-windows\x64\vc17\lib\OpenCVModules.cmake
> ```
>
> ```powershell
> Test-Path dist\opencv\x86_64-windows\include\opencv2\opencv.hpp
> ```
>
> ```powershell
> Test-Path dist\opencv\x86_64-windows\etc\licenses\zlib-LICENSE
> ```
>
> ```powershell
> Test-Path dist\opencv\x86_64-windows\setup_vars_opencv5.cmd
> ```
>
> All six must print `True`. The CMake package is relocatable: the root
> config resolves the nested one and every install path relative to its
> own file location, so consumers can point `CMAKE_PREFIX_PATH` at the
> prefix above (or at any directory containing it) and
> `find_package(OpenCV)` works.
>
> One more fact worth having before section 3 — how the DLLs load.
> Windows has no rpath/install-name concept; the loader finds
> `opencv_core500.dll` through the standard search order (the exe's
> directory, then `PATH`, then system directories), which is why the
> consumer must put `x64\vc17\bin` on `PATH` (section 3.5). To inspect
> what a DLL binds to directly, run in the dev shell:
>
> ```
> dumpbin /dependents dist\opencv\x86_64-windows\x64\vc17\bin\opencv_core500.dll
> ```
>
> The listed imports should be Windows system DLLs plus the C++ runtime
> (`VCRUNTIME140.dll`, `MSVCP140.dll`) — the shared build links the
> dynamic CRT: OpenCV's `BUILD_WITH_STATIC_CRT` option (default ON on
> MSVC) only applies when `BUILD_SHARED_LIBS` is OFF
> (`cmake/OpenCVCRTLinkage.cmake` line 40; the shared path enforces
> `/MD`). Practical consequence: a machine without Visual Studio needs
> the Microsoft Visual C++ 2015–2022 Redistributable installed to load
> these DLLs. Any non-system, non-CRT import in that dump is a leaked
> third-party dependency — re-check the configure switches.

> ## 3. Validate with sample/ (headless PNG round trip)
>
> Section 2 left you a working OpenCV install. This section proves it
> works by running a small program from `sample/` that builds a
> deterministic 320 × 240 BGR image, writes it to a PNG file, reads the
> file back, asserts every pixel survived exactly, and then exercises
> imgproc with a threshold check whose expected result is recomputed per
> pixel. If the last printed line is `Threshold check: exact`, headers,
> DLLs, and the bundled PNG codec stack all work — end to end.
>
> The program is two files:
>
> - `sample\hello_opencv.cpp` — the program: image → PNG → read back →
>   assert pixel-exact → grayscale → threshold → assert exact.
> - `sample\CMakeLists.txt` — its build file: declares C++17, finds the
>   installed OpenCV with components (`core`, `imgproc`, `imgcodecs`),
>   links `${OpenCV_LIBS}` (the requested modules plus their dependency
>   closure follow through the imported targets).
>
> Run these commands from the repo root, in order, one at a time:
>
> **3.1** Confirm the install from section 2 is present. Must print
> `True` — if it prints `False`, finish section 2 before continuing:
>
> ```powershell
> Test-Path dist\opencv\x86_64-windows\OpenCVConfig.cmake
> ```
>
> **3.2** Remove any previous sample build (stale caches point at the
> wrong OpenCV):
>
> ```powershell
> Remove-Item -Recurse -Force sample\build
> ```
>
> **3.3** Point the sample at the install and generate its build plan.
> Success: the root config announces the detected consumer toolset
> (`-- OpenCV ARCH: x64`, `-- OpenCV RUNTIME: vc17`, `-- OpenCV
> STATIC: OFF`) and a `Found OpenCV` line naming
> `dist\opencv\x86_64-windows\...`:
>
> ```powershell
> cmake -S sample -B sample\build -G Ninja `
>       -DCMAKE_BUILD_TYPE=Release `
>       "-DCMAKE_PREFIX_PATH=$PWD\dist\opencv\x86_64-windows"
> ```
>
> The root config also prints a hint line — `You might need to add
> ...\x64\vc17\bin to your PATH to be able to run your applications.`
> — which is expected and is exactly what step 3.5 does. If you instead
> see `Could not find a package configuration file provided by
> "OpenCV"`, see Troubleshooting.
>
> **3.4** Compile the sample (seconds, not minutes):
>
> ```powershell
> cmake --build sample\build --parallel
> ```
>
> **3.5** Make the OpenCV DLLs findable at runtime. Without this,
> Windows refuses to start the exe (`0xc0000135`). Lasts only for this
> session:
>
> ```powershell
> $env:PATH = "$PWD\dist\opencv\x86_64-windows\x64\vc17\bin;$env:PATH"
> ```
>
> (The `setup_vars_opencv5.cmd` script OpenCV installs at the prefix
> root performs the same prepend for cmd sessions — and also appends a
> placeholder PYTHONPATH entry; harmless with the bindings off.)
>
> **3.6** Go where the built exe is:
>
> ```powershell
> cd sample\build
> ```
>
> **3.7** Run the validation:
>
> ```powershell
> .\hello_opencv.exe
> ```
>
> **3.8** Print the exit code. Must be `0`:
>
> ```powershell
> $LASTEXITCODE
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
> ```powershell
> cd ..\..
> ```
>
> The run proves the full loop: CMake finds the installed package
> through `CMAKE_PREFIX_PATH` (root config → `x64\vc17\lib` imported
> targets), a C++17 consumer compiles against the OpenCV 5.0 headers,
> the DLLs load from `bin\` via the `PATH` entry from 3.5, the bundled
> libpng+zlib stack encodes and decodes a real PNG, and imgproc
> computes a predictable result. It does not exercise video I/O, GUI
> windows, DNN inference, or deployment to another machine — but
> nothing about this path needs extra environment setup (`OPENCV_*`
> data variables are not required for this round trip).
>
> Note the sample asserts properties, not magic numbers: the PNG is
> lossless, so read-back equality is a hard assertion, and threshold is
> a point operation, so its per-pixel expected value is recomputed in
> the program. The PNG file lands in `sample\build\` and is gitignored
> with that directory.

> ## 4. Package the install as zips
>
> Section 2's install is a tree; this section turns it into two archive
> files under `dist\zips\`:
>
> - **dev** — everything: DLLs, import libraries, headers, the root CMake
>   package config, runtime data, per-component licenses. For consumers
>   who build against OpenCV.
> - **libs-only** — just the `x64\vc17\bin\*.dll` files, flattened at the
>   zip root. For consumers who only need to *run* something already
>   built against these DLLs.
>
> Both archives carry the legal texts at the zip root (`LICENSE`,
> `COPYRIGHT`, `THIRD_PARTY_NOTICES.md`) — required when the archives
> are used in other repositories. The legal files live in the repo under
> `third_party\opencv\`; the dev archive additionally carries the
> per-component texts under `etc\licenses\` (installed by section 2).
>
> Run from the repo root, in order, one at a time:
>
> **4.1** Provenance first. `third_party\opencv\THIRD_PARTY_NOTICES.md`
> is a repo-tracked file that describes **all** platform builds, and the
> archives inherit it verbatim — the zips must never carry a record that
> omits the platform whose binaries they contain. Confirm its Windows
> x64 block is present and current for the build you are packaging:
>
> ```powershell
> Select-String -Path third_party\opencv\THIRD_PARTY_NOTICES.md -Pattern "Windows x64"
> ```
>
> ✅ Prints the Windows provenance line. As committed (2026-10-13), the
> block records this machine's toolchain — MSVC 19.44.35229 (Visual
> Studio 2022 17.14, toolset 14.44.35207, OpenCV runtime tag vc17),
> CMake 4.4.4 + Ninja 1.12.1 — and the Windows inventory (the "Windows
> x64 differences" subsection: built-in CLAPACK compiled in, carotene
> and MLAS out, the opencl-headers text installed), matching the
> configure run in `tmp\opencv\configure.log`. If the toolchain or the
> configuration changes (a MLAS-capable rebuild with an ASM compiler, a
> NASM-enabled libjpeg-turbo), update the block to match before
> zipping. If the step prints nothing, the file is missing its Windows
> block — stop and fix the file before zipping. If the file changes
> after the zips were built, re-run 4.3 and 4.4 so the archives carry
> the updated file.
>
> **4.2** Create the output folder (gitignored, like all of `dist\`):
>
> ```powershell
> New-Item -ItemType Directory -Force dist\zips
> ```
>
> **4.3** Build the dev zip from the whole install plus the legal files
> at the root. The wildcard entry keeps the install's relative layout
> (a consumer extracts it anywhere and points `CMAKE_PREFIX_PATH` at
> the extraction root) while the three legal files, listed as plain
> file paths, land at the zip root — `Compress-Archive` accepts both
> in one call:
>
> ```powershell
> Compress-Archive -Path dist\opencv\x86_64-windows\*, third_party\opencv\THIRD_PARTY_NOTICES.md, third_party\opencv\LICENSE, third_party\opencv\COPYRIGHT -DestinationPath dist\zips\opencv-5.0.0-x86_64-windows-dev.zip
> ```
>
> **4.4** Build the libs-only zip — flattened DLLs plus the same three
> legal files at the root:
>
> ```powershell
> Compress-Archive -Path dist\opencv\x86_64-windows\x64\vc17\bin\*.dll, third_party\opencv\THIRD_PARTY_NOTICES.md, third_party\opencv\LICENSE, third_party\opencv\COPYRIGHT -DestinationPath dist\zips\opencv-5.0.0-x86_64-windows-libs-only.zip
> ```
>
> **4.5** Check both zips exist and are not empty:
>
> ```powershell
> Get-ChildItem dist\zips | Select-Object Name, @{n='MB';e={[math]::Round($_.Length/1MB)}}
> ```
>
> ✅ Exactly two `.zip` rows, both with MB values well above zero
> (dev is the larger of the two).
>
> Notes:
>
> - `Compress-Archive` overwrites an existing zip of the same name
>   silently — no "file exists" prompt.
> - Because of the `OPENCV_DLLVERSION` naming (see section 2), `bin\`
>   contains only module DLLs (`opencv_*500.dll`) — the bundled
>   third-party code is compiled into them, apps/tests are off, and no
>   PDBs or static archives are installed — so the `bin\*.dll` glob in
>   4.4 captures exactly the module libraries. Nothing extra can slip
>   in.
> - The libs-only zip is flat by design; extract it next to an exe (or
>   onto a `PATH`-listed directory) — nothing inside references
>   folders. It is *not* self-sufficient for building: without the
>   import libraries (`x64\vc17\lib\*.lib`), headers, and the CMake
>   package, MSVC cannot link new code against these DLLs.
> - The dev zip's root carries `OpenCVConfig.cmake` because that is
>   where the Windows install puts it (see the layout in section 2) —
>   consumers point `CMAKE_PREFIX_PATH` at the extraction root, not at
>   a `lib\cmake` subdirectory.
> - The dev zip does not carry the VC++ runtime: the DLLs link the
>   dynamic CRT (`/MD`), and consumers without Visual Studio need the
>   Microsoft Visual C++ 2015–2022 Redistributable (see the `dumpbin`
>   note in section 2).
> - Names pin the version on purpose (`5.0.0`): when swapping versions
>   (see that section below), re-run this section afterwards so the
>   archives never claim the wrong version.
> - The three legal files at each zip root come from this repository:
>   `LICENSE` is the verbatim Apache 2.0 text and `COPYRIGHT` the
>   verbatim copyright-attribution block from the OpenCV source tag,
>   and `THIRD_PARTY_NOTICES.md` records the provenance and the
>   bundled-component inventory (verified against the 5.0.0 tree —
>   OpenCV statically bundles its codec stack, so the notices enumerate
>   zlib, libjpeg-turbo, libpng, libtiff, libwebp, OpenJPEG, JasPer,
>   protobuf, MLAS, ittnotify, and the smaller vendored pieces). On
>   Windows the inventory gains the built-in CLAPACK; re-derive it when
>   swapping versions or changing the configuration.

> ## 5. Optional cleanup
>
> After installation and validation, if you do not need the source/build
> tree for further work:
>
> ```powershell
> Remove-Item -Recurse -Force tmp\opencv
> ```
>
> This deletes the source checkout and the build output, but leaves
> `dist\opencv\x86_64-windows` and the zips. This guide made no local
> source modifications, so re-cloning the stock tag restores the build
> inputs exactly. OpenCV's configure does not call `export(PACKAGE)` —
> no entry is added to the user's CMake package registry (unlike OCCT,
> whose Windows configure self-registers), so there is nothing to clean
> up under `HKCU:\Software\Kitware\CMake\Packages`.

> ## Optional components and backends
>
> Each of these changes the build's character; reconfigure in a **fresh
> build directory** (`Remove-Item -Recurse -Force tmp\opencv\build` or
> a new `-B` path) — stale caches leave half-set third-party variables
> behind. After any of them: re-run section 3 (validation), update the
> provenance block in `third_party\opencv\THIRD_PARTY_NOTICES.md` if the
> binary contents changed, and re-run section 4 so the zips match.
>
> ### Intel IPP (configure-time download)
>
> Remove `-DWITH_IPP=OFF`. Configure downloads
> `ippicv_2026.0.0_win_intel64_20260327_general.zip` (pinned by MD5 in
> `3rdparty/ippicv/ippicv.cmake`) into the build tree and links Intel's
> optimized kernels into the modules — the biggest single performance
> lever in OpenCV's CPU dispatch. Redistribution caveat: IPPICV is
> Intel-licensed, not Apache-2.0 — its terms differ and shipping it in
> the zips requires checking Intel's redistribution conditions first.
> Record the change in `THIRD_PARTY_NOTICES.md`.
>
> ### KleidiCV
>
> Not applicable on x64 Windows: the `WITH_KLEIDICV` option is only
> created on AArch64 Android/UNIX, so there is nothing to enable here.
> ARM64 Windows builds are out of scope for this guide; if you ever
> attempt one, the macOS recipe's KleidiCV section is the reference and
> the toolset-tag mapping in `cmake/OpenCVDetectCXXCompiler.cmake`
> gains an ARM64 row.
>
> ### TBB
>
> Windows has no system package provider for TBB — obtain it yourself
> (oneTBB release from GitHub, or a vcpkg install) and point CMake at
> it, then add `-DWITH_TBB=ON`. OpenCV's parallel framework switches
> from the stock Concurrency to TBB. Caveat: the DLLs then reference
> the TBB DLLs at load time, so binaries built this way depend on
> whatever delivered TBB being present on the consuming machine —
> carrying TBB into the zips is out of scope here.
>
> ### World build
>
> Add `-DBUILD_opencv_world=ON`: all modules fold into a single
> `opencv_world500.dll` (consumers link `opencv_world` only). The
> libs-only zip then contains one DLL; the dev layout changes
> accordingly. Keep it OFF unless a consumer specifically wants the
> single-library form.
>
> ### Python bindings
>
> Requires a Python 3 interpreter plus numpy. Remove
> `-DBUILD_opencv_python3=OFF`: the build produces `cv2` and installs it
> into the prefix's Python package path. The bindings are coupled to the
> interpreter version they were built against — this is the least
> portable thing OpenCV installs, and the reason the base recipe pins it
> off.
>
> For this repository the plan is different: the Python side (training
> and inference scripts) gets its `cv2` from package-manager wheels
> (`uv pip install opencv-python`), not from this repo — those wheels
> are independent builds with their own bundled libraries, so neither
> the recipe nor the zips change for them. One hard boundary: never mix
> the two OpenCV copies in one process — on Windows, a wheel's `cv2`
> DLL and this repo's `opencv_*500.dll` can collide through the same
> DLL search order (exe directory, `PATH`) and the loader binds both to
> whichever copy loaded first, silently mixing versions. Keep them in
> separate processes (Python training → ONNX → C++ inference), which is
> the intended flow anyway.
>
> ### FFmpeg for more video formats
>
> Remove `-DWITH_FFMPEG=OFF` and reconfigure: on Windows this downloads
> OpenCV's prebuilt FFmpeg binaries (`opencv_videoio_ffmpeg_64.dll`
> plus the 32-bit variant and `ffmpeg_version.cmake`, MD5-pinned in
> `3rdparty/ffmpeg/ffmpeg.cmake`, from the `opencv_3rdparty` GitHub
> branch `ffmpeg/5.x_20260602`) and installs the prebuilt DLL next to
> the module DLLs, giving videoio ffmpeg-backed containers and codecs on
> top of DirectShow/Media Foundation. Redistribution caveat: FFmpeg is
> LGPL — the download carries `license.txt`/`readme.txt` that the build
> installs, check the terms before the zips ship, and record the
> component in `THIRD_PARTY_NOTICES.md`. Escape hatches if the download
> must not run: `-DOPENCV_FFMPEG_SKIP_DOWNLOAD=ON` (no ffmpeg backend)
> or the `OPENCV_FFMPEG_URL` cache/env variable to point at a local
> copy.
>
> ### OpenEXR / VTK / Eigen (system-detected libraries)
>
> Remove the matching `-DWITH_OPENEXR=OFF`, `-DWITH_VTK=OFF`, or
> `-DWITH_EIGEN=OFF` and reconfigure. On Windows all three resolve
> against machine-installed packages (CMake package paths / vcpkg
> prefixes; Eigen additionally scans `%ProgramFiles%`): OpenEXR gives
> imgcodecs an EXR codec and VTK gives highgui its viz support — both
> become external runtime dependencies (same caveat as TBB); Eigen is
> header-only but couples the binaries to that machine's package set.
> Note 5.0 does not bundle OpenEXR (unlike 4.x), so there is no
> in-tree fallback.
>
> ### Built-in Unicode font
>
> Remove `-DWITH_UNIFONT=OFF` and reconfigure: the WQY MicroHei font is
> downloaded (pinned MD5 in `modules/imgproc/CMakeLists.txt`) and
> compiled into libopencv_imgproc, giving `putText` Unicode coverage.
> Redistribution caveat: the font is GPL-2.0 with a font embedding
> exception — check the terms before the zips ship, and record the
> component in `THIRD_PARTY_NOTICES.md`.

> ## Useful switches (add to the `cmake -S` line)
>
> | Switch | What it does |
> |---|---|
> | `-DBUILD_opencv_world=ON` | One `opencv_world500.dll` instead of per-module DLLs (see above). |
> | `-DOPENCV_GENERATE_PKGCONFIG=ON` | Generate `opencv5.pc` for pkg-config consumers (MSYS2-style). Stock default OFF (the option's own description calls it deprecated). |
> | `-DWITH_TBB=ON` | TBB parallel framework (supply TBB yourself; without it, Concurrency/PPL on MSVC). |
> | `-DWITH_QT=ON` / `-DWITH_OPENGL=ON` | GUI toolkit / OpenGL interop for highgui; both default OFF (Win32 UI needs no help). OpenGL needs only the SDK's `opengl32.lib`. |
> | `-DENABLE_PRECOMPILED_HEADERS=ON` | Precompiled headers — moot under the Ninja generator (`PCHSupport_FOUND` resolves NO; the summary prints `Precompiled headers: NO`). The knob matters only for MSBuild-generator builds, where the MSVC default is ON. |
> | `-DBUILD_LIST=core,imgproc,...` | Build only the listed modules (and their dependencies) — a smaller dist when the consumer set is known. |
> | `-DWITH_OPENCL=OFF` | Compile out the OpenCL paths (default ON; on Windows nothing is linked either way — vendored headers, runtime-loaded DLL). |
> | `-DOPENCV_ENABLE_NONFREE=ON` | Includes patent-encumbered algorithms (SIFT et al.). Changes the licensing story — do not combine with the zips without a legal review. |
> | `-DCV_DISABLE_OPTIMIZATION=ON` | Baseline build without SIMD dispatch (debug aid). |
> | `-DINSTALL_PDB=OFF` | Removes the PDB install rules (dormant by default — see section 2; only relevant when installing the `pdb` component explicitly). |
> | `-DBUILD_SHARED_LIBS=OFF` | Static libs instead of DLLs; with MSVC's default `BUILD_WITH_STATIC_CRT=ON` this also switches the CRT from `/MD` to `/MT` (no VC++ Redistributable needed). Changes the packaging story — out of scope for this guide. |
>
> OpenCV 5.0 builds as C++17 by default (`CMAKE_CXX_STANDARD 17`,
> `cmake/OpenCVDetectCXXCompiler.cmake`), with `CMAKE_CXX_STANDARD_REQUIRED`
> on and no compiler extensions. Do not lower the standard: the headers
> of a C++17-built OpenCV expect a C++17 consumer. The compiler floor is
> MSVC 19.14 (VS 2017 15.7) — older toolsets earn a configure warning
> ("OpenCV requires MSVC >= 2017 15.7 / 1914 ..."). Release builds get
> MSVC's `/MP` (per-process parallel compilation, controlled by
> `OPENCV_MSVC_PARALLEL` in `cmake/OpenCVCompilerOptions.cmake`) — that
> is compiler parallelism only; the OpenCV parallel framework remains
> Concurrency.
>
> On the CRT: shared OpenCV DLLs link the dynamic CRT (`/MD`) —
> `BUILD_WITH_STATIC_CRT` (default ON on MSVC) is applied only to
> static-library builds. Consumers without Visual Studio need the
> Microsoft Visual C++ 2015–2022 Redistributable; the zips do not and
> should not ship the runtime.

> ## Swapping versions
>
> Replace `5.0.0` in the step-1 clone command and re-run steps 2–3 — the
> dist is rebuilt from the new tag. OpenCV's tags are dotted with no
> prefix (`5.0.0`, `4.14.0`, `3.4.20`). If retaining the old source
> checkout, replace it with the requested tag before rebuilding —
> **no patch needs re-applying on Windows**, but check the new tag's
> CMake scripts first:
>
> - `cmake/OpenCVDetectCXXCompiler.cmake` — the `OpenCV_RUNTIME` mapping
>   decides the `x64\vcN` install directory name. A toolset the tag does
>   not recognize prints `OpenCV does not recognize MSVC_VERSION ...`
>   and installs **without** the toolset prefix, which the root
>   consumer config then cannot resolve — see Troubleshooting.
> - `cmake/OpenCVInstallLayout.cmake` (Windows branch) — the install
>   path set; verify the layout block in section 2 still matches before
>   re-running the `Test-Path` checks.
> - The top-level `OPENCV_DLLVERSION` block — the DLL digits change with
>   the version (`5.0.0` → `opencv_core500.dll`; `5.1.0` →
>   `opencv_core510.dll`), and every `Test-Path`, zip glob, and
>   `PATH` line in this guide spells them. CMake's imported targets
>   handle the names for consumers, but this guide's literal paths must
>   be updated.
> - The bundled 3rd-party set — re-derive the inventory in
>   `third_party\opencv\THIRD_PARTY_NOTICES.md` from the new tag's
>   `3rdparty/` tree and `3rdparty/readme.txt` (OpenCV's own summary of
>   the vendored libraries), and re-check the license files copied into
>   `third_party\opencv\licenses\`.
> - The module set — 5.0 split `calib3d` into `calib`/`geometry`/
>   `stereo`, renamed `features2d` to `features` and dropped `ml`/`gapi`
>   from the main repo. Consumers linking by module name need the new
>   names; further major-version changes follow the macOS doc's notes.
>
> Install into a fresh prefix or delete the old installed files first;
> reinstalling does not clean them up. Repeat validation, then re-run
> section 4 — zip names pin the version.

> ## Troubleshooting
>
> - **`$LASTEXITCODE` prints `0` even though the stage failed** — you
>   read `$?` instead: in a pipeline that only reports the pipeline
>   itself. Check `$LASTEXITCODE` right after each stage, or read the
>   log's tail for the actual error.
> - **`CMake was unable to find a build program corresponding to
>   "Ninja"`, `CMAKE_C_COMPILER not set`, `cl.exe` not found** — the
>   shell is a plain PowerShell window without the VS environment. Open
>   *Developer PowerShell for VS 2022* from the Start menu, or run the
>   `Launch-VsDevShell.ps1` one-liner from Prerequisites in the current
>   window; verify with the `Get-Command cl, ninja, cmake` check, delete
>   the half-configured `tmp\opencv\build`, and reconfigure there.
> - **`^` passed to CMake (`Ignoring extra path from command line`) or
>   `-D...` lines run as separate commands** — cmd-style line
>   continuation pasted into PowerShell. Paste the blocks as written —
>   PowerShell continues lines with a trailing backtick, not `^` — and
>   check that no trailing space follows a backtick.
> - **Configure fails or hangs on `IPPICV: Download ...`** — the
>   Windows IPPICV archive download ran, which means
>   `-DWITH_IPP=OFF` was dropped from the configure line (or you are
>   deliberately enabling IPP — then check connectivity or point the
>   download at a local copy via `OPENCV_IPPICV_URL`). For the base
>   recipe: restore `-DWITH_IPP=OFF`, delete `tmp\opencv\build`,
>   reconfigure.
> - **Configure fails or hangs on the `opencv_videoio_ffmpeg` download**
>   — the Windows-only prebuilt FFmpeg download ran
>   (`modules/videoio/cmake/detect_ffmpeg.cmake`), which means
>   `-DWITH_FFMPEG=OFF` was dropped (or you are deliberately enabling
>   it — then see the FFmpeg section for `OPENCV_FFMPEG_URL` /
>   `OPENCV_FFMPEG_SKIP_DOWNLOAD`). Restore the switch, delete
>   `tmp\opencv\build`, reconfigure.
> - **The summary shows `FFMPEG: YES (prebuilt binaries)`, an OpenEXR
>   line, `VTK support: YES`, or `Eigen: YES`** — a machine-installed
>   package leaked into the build: the corresponding OFF switch was
>   dropped from the configure line. Delete `tmp\opencv\build`, restore
>   the full switch list from section 2, and reconfigure — OpenEXR and
>   VTK become external runtime dependencies, and a dist built that way
>   is not the self-contained one this guide describes. Eigen's
>   `%ProgramFiles%` fallback scan means a YES line can appear without
>   any vcpkg at all.
> - **`OpenCV requires MSVC >= 2017 15.7 / 1914 (detected ...) ...`** —
>   a configure *warning*: the selected toolset is older than OpenCV
>   5.0's floor. Install/upgrade the C++ workload and reconfigure in a
>   fresh build directory.
> - **`OpenCV does not recognize MSVC_VERSION "..." Cannot set
>   OpenCV_RUNTIME`** — the toolset is newer than the tag knows. The
>   install then lands without the `x64\vcN` prefix and the root
>   consumer config (`OpenCVConfig.root-WIN32.cmake.in`) cannot find
>   the package. Update OpenCV to a tag that maps the toolset (see
>   Swapping versions) or build with VS 2022's toolset.
> - **`Could not find a package configuration file provided by
>   "OpenCV"`** when configuring the sample — `CMAKE_PREFIX_PATH` must
>   point at the install prefix (`dist\opencv\x86_64-windows`), under
>   which the root `OpenCVConfig.cmake` sits. A prefix set only at
>   install time (`--prefix`) does not work — the prefix is baked at
>   configure time.
> - **`Found OpenCV Windows Pack but it has no binaries compatible with
>   your configuration`** — the root config derived the consumer's
>   architecture/runtime from the compiler it found and no matching
>   `x64\vcN\lib\OpenCVConfig.cmake` exists (32-bit consumer toolset,
>   or an unrecognized runtime). Configure the sample from the x64 dev
>   shell (`-Arch amd64`).
> - **The sample keeps finding the wrong OpenCV after a rebuild** — the
>   sample's CMake *cache* pins `OpenCV_DIR`. OpenCV does not
>   self-register its build tree in the user package registry (no
>   `export(PACKAGE)` call, unlike OCCT), so a cache pin is the only way
>   this happens: delete `sample\build` and reconfigure with the prefix
>   set.
> - **`0xc0000135` (STATUS_DLL_NOT_FOUND)** when starting
>   `hello_opencv.exe` — the OpenCV `x64\vc17\bin\` directory is not on
>   PATH in that session. Re-run the `$env:PATH = ...` line from
>   section 3.5 (a fresh terminal loses it).
> - **`VCRUNTIME140.dll / MSVCP140.dll not found` on a machine without
>   Visual Studio** — the DLLs link the dynamic CRT. Install the
>   Microsoft Visual C++ 2015–2022 Redistributable on deployment
>   machines (see the CRT note in section 2).
> - **`hello_opencv: FAILED: PNG write`** — `imwrite` could not encode
>   PNG, which means the bundled PNG codec is missing from the build
>   (the stock `BUILD_PNG=ON` default on WIN32 was overridden).
>   Reconfigure with the recipe's switches in a fresh build directory.
> - **OpenCV headers fail with C++ errors in the sample** — the consumer
>   must build as C++17 (OpenCV 5.0 requires it on both sides);
>   `sample\CMakeLists.txt` sets it. Do not lower
>   `CMAKE_CXX_STANDARD`.
> - **`OpenCV 5.x requires C++17 support, but it was not detected`**
>   warnings in the configure log — the selected compiler is too old;
>   update the VS C++ workload, delete `tmp\opencv\build`, and
>   reconfigure.