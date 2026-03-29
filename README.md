# HeapLens

HeapLens is a Linux `x86_64` desktop application for practical glibc heap exploitation work against small demo binaries.

The current codebase is split into two active layers:

- a Rust GTK4 frontend in `src/*.rs`
- a C backend in `src/core/` and `src/techniques/` that handles ptrace, memory reading, disassembly, heap parsing, and technique metadata

The older C-only GUI has been removed from the active tree. The build now produces:

- `build/libheaplens.so`: C backend shared library
- `build/heaplens`: Rust GTK4 application
- `build/demos/...`: native and versioned demo binaries

## Features

- Single practical exploitation workspace with technique list, disassembly, register, process, heap, and notes views
- ptrace-based process control and `/proc/<pid>/mem` inspection
- Capstone-backed disassembly through the C core
- Versioned demo runtime support via bundled glibc trees

## Requirements

HeapLens targets Linux `x86_64` only.

You need:

- `build-essential`
- `make`
- `pkg-config`
- `rustc`
- `cargo`
- `libgtk-4-dev`
- `libvte-2.91-gtk4-dev`
- `libcapstone-dev`
- ptrace permission to control child processes

Ubuntu/Debian example:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  make \
  pkg-config \
  rustc \
  cargo \
  libgtk-4-dev \
  libvte-2.91-gtk4-dev \
  libcapstone-dev
```

## Repository Layout

```text
HeapLens/
├── Makefile
├── Cargo.toml
├── build.rs
├── assets/
│   └── styles/app.css
├── glibc/
│   └── <version>/lib/
├── scripts/
│   └── install_glibc_bundles.sh
└── src/
    ├── app.rs
    ├── ffi.rs
    ├── main.rs
    ├── core/
    ├── techniques/
    └── ui/
```

## Build

Build everything:

```bash
make
```

That builds:

- the C backend shared library
- the Rust GTK4 app
- the demo binaries

Useful targets:

```bash
make        # Build app, shared library, and demos
make demos  # Build only demos
make run    # Build and launch HeapLens
make clean  # Remove build artifacts
```

## Run

Launch with:

```bash
./build/heaplens
```

Because HeapLens uses ptrace and `/proc/<pid>/mem`, you may need either:

```bash
sudo ./build/heaplens
```

or:

```bash
sudo sysctl kernel.yama.ptrace_scope=0
./build/heaplens
```

Restore your preferred ptrace policy after testing if you change it system-wide.

## Demo Runtimes

HeapLens supports these glibc versions in the UI:

- `2.23`
- `2.27`
- `2.29`
- `2.31`
- `2.32`
- `2.34`
- `2.35`
- `2.38`
- `2.39`

Native demo path:

```text
build/demos/native/demo_<technique>_native
```

Versioned demo path:

```text
build/demos/<glibc-version>/demo_<technique>_<glibc-version>
```

If `glibc/<version>/lib/ld.so` and `glibc/<version>/lib/libc.so.6` exist, demos for that version are linked against the matching runtime. Otherwise HeapLens falls back to the native demo binary and reports that fallback in the trace output.

## Install glibc Bundles

To populate or refresh the bundled runtimes:

```bash
./scripts/install_glibc_bundles.sh
```

This uses `matrix1001/glibc-all-in-one` to fetch the supported Ubuntu amd64 glibc packages and install them under `glibc/<version>/lib/`.

After updating runtimes, rebuild demos:

```bash
make -B demos
```

## Packaging

For a portable checkout on another Linux host, copy:

- `build/`
- `assets/`
- `glibc/` if you want versioned demo runtimes available

The target system still needs the required GTK4, VTE, and Capstone shared libraries.

## Troubleshooting

### GTK4 or VTE headers not found

Check:

```bash
pkg-config --cflags gtk4
pkg-config --cflags vte-2.91-gtk4
pkg-config --cflags capstone
```

### Rust build issues

Check:

```bash
rustc --version
cargo --version
```

### ptrace or `/proc/<pid>/mem` denied

- run as root
- or relax `kernel.yama.ptrace_scope`
- or use a system policy that allows tracing child processes

### Version switch appears ineffective

Confirm:

- `glibc/<version>/lib/ld.so` exists
- `glibc/<version>/lib/libc.so.6` exists
- demos were rebuilt after installing runtimes

The following command is a good reset point:

```bash
make -B demos
```
