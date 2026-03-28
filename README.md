# HeapLens

HeapLens is a Linux-native GTK4 desktop application for learning glibc heap internals and stepping through heap exploitation primitives against small demo binaries. The codebase is written in C11, uses raw ptrace and `/proc/<pid>/mem` for process inspection, and keeps allocator metadata parsing in hand-written C structs that mirror glibc internals.

## Features

- Learning mode with fifteen allocator-focused chapters.
- Exploitation lab with real-time heap, register, disassembly, syscall, terminal, and theory panels.
- ptrace-based stepping and `/proc/<pid>/mem` heap inspection.
- Capstone-backed disassembly.
- VTE terminal embedded directly in the application.
- Version-aware technique registry prepared for multiple glibc builds.

## Requirements

HeapLens targets Linux `x86_64` only. Build and run it on a Linux machine with:

- `build-essential`
- `make`
- `pkg-config`
- `libgtk-4-dev`
- `libvte-2.91-gtk4-dev`
- `libcapstone-dev`
- root privileges or a ptrace policy that allows tracing the target process

Ubuntu/Debian example:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  make \
  pkg-config \
  libgtk-4-dev \
  libvte-2.91-gtk4-dev \
  libcapstone-dev
```

## Repository Layout

```text
HeapLens/
├── Makefile
├── assets/
│   └── styles/dark.css
├── glibc/
│   ├── 2.27/
│   ├── 2.31/
│   ├── 2.35/
│   └── 2.39/
└── src/
    ├── core/
    ├── techniques/
    └── ui/
```

## Build Steps

1. Clone the repository on a Linux `x86_64` machine.
2. Optionally place version-specific glibc loader/runtime files under `glibc/<version>/lib/`.
   Or run `./scripts/install_glibc_bundles.sh` to populate the supported amd64 versions from `matrix1001/glibc-all-in-one`.
3. Run the top-level Makefile:

```bash
make
```

This Makefile does three things:

- compiles the main GTK4 application into `build/heaplens`
- compiles the demo binaries into `build/demos/`
- stores object files in `build/obj/`

Useful Makefile targets:

```bash
make        # Build the app and all demos
make demos  # Build only the demo binaries
make run    # Build and launch HeapLens
make clean  # Remove build artifacts
```

## Running HeapLens

Run the application with:

```bash
./build/heaplens
```

Because HeapLens relies on `ptrace`, `/proc/<pid>/mem`, and live process control, you will usually want one of these:

```bash
sudo ./build/heaplens
```

or:

```bash
sudo sysctl kernel.yama.ptrace_scope=0
./build/heaplens
```

If you change `ptrace_scope`, restore your preferred system policy after testing.

## Demo Binary Build Format

The demo build system lives in `src/techniques/demos/Makefile.demos`.

Native demo build pattern:

```bash
build/demos/native/demo_<technique>_native
```

Versioned demo build pattern:

```bash
build/demos/<glibc-version>/demo_<technique>_<glibc-version>
```

When a matching `glibc/<version>/lib/ld.so` and `glibc/<version>/lib/libc.so.6` directory exists, the demo Makefile uses:

```bash
gcc -g -O0 -m64 demo_<technique>.c -o demo_<technique>_<glibcver> \
  -Wl,--rpath=./glibc/<ver>/lib \
  -Wl,--dynamic-linker=./glibc/<ver>/lib/ld.so
```

Otherwise it falls back to a normal local build.

## Deployment Steps

HeapLens is a desktop tool, so deployment is usually a Linux build-and-run workflow.

### Option 1: Run From Source

1. Clone the repository on the target Linux host.
2. Install the packages listed above.
3. Run `make`.
4. Start the tool with `./build/heaplens` or `sudo ./build/heaplens`.

### Option 2: Package the Built Artifacts

1. Build on a Linux `x86_64` machine with the required GTK4/VTE/Capstone packages.
2. Copy these directories to the target machine:
   - `build/`
   - `assets/`
   - `glibc/` if you are shipping custom glibc runtime directories
3. Ensure the target host also has the required shared libraries installed.
4. Launch `build/heaplens`.

### Option 3: System Integration

To make HeapLens easier to launch on Linux desktops, create a `.desktop` entry:

```ini
[Desktop Entry]
Name=HeapLens
Exec=/absolute/path/to/HeapLens/build/heaplens
Type=Application
Terminal=false
Categories=Development;Security;
```

## Notes About glibc Versioned Labs

- The UI lets you pick versions such as `2.23`, `2.27`, `2.29`, `2.31`, `2.32`, `2.34`, `2.35`, `2.38`, and `2.39`.
- The corresponding demo binaries are built into per-version folders under `build/demos/`.
- To make version-specific runtime behavior meaningful, populate `glibc/<version>/lib/` with the matching `ld.so` and `libc.so.6` files before building demos.
- If no runtime bundle exists for the selected version, HeapLens falls back to the native demo binary and logs that fallback in the trace panel.

## Troubleshooting

### GTK4 or VTE not found

Check that `pkg-config` can see the packages:

```bash
pkg-config --cflags gtk4
pkg-config --cflags vte-2.91-gtk4
pkg-config --cflags capstone
```

### ptrace or `/proc/<pid>/mem` access denied

- Run as root.
- Or relax `kernel.yama.ptrace_scope`.
- Or attach only to processes permitted by your system policy.

### Demo binary not found in the UI

- Run `make demos`.
- Confirm the expected binary exists under `build/demos/`.
- If you want version-specific binaries, confirm the matching `glibc/<version>/lib/` files exist before building.

## Compiler Flags

The main application is designed around:

```bash
gcc -O2 -Wall -Wextra -std=c11 -D_GNU_SOURCE
```

The demo binaries intentionally use `-g -O0 -m64` so the lab steps are easier to inspect under ptrace and external debuggers.
