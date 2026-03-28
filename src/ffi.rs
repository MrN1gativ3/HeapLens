use std::ffi::CStr;
use std::mem::MaybeUninit;
use std::os::raw::{c_char, c_int, c_ulong, c_void};

pub const PATH_MAX: usize = 4096;
pub const HEAPLENS_CHUNK_LABEL_MAX: usize = 32;
pub const HEAPLENS_CHUNK_VALIDATION_MAX: usize = 128;

#[repr(C)]
pub struct MallocChunk {
    pub prev_size: usize,
    pub size: usize,
    pub fd: *mut MallocChunk,
    pub bk: *mut MallocChunk,
    pub fd_nextsize: *mut MallocChunk,
    pub bk_nextsize: *mut MallocChunk,
}

#[repr(C)]
pub struct MallocState {
    pub flags: c_int,
    pub have_fastchunks: c_int,
    pub fastbins_y: [*mut MallocChunk; 10],
    pub top: *mut MallocChunk,
    pub last_remainder: *mut MallocChunk,
    pub bins: [*mut MallocChunk; 254],
    pub binmap: [u32; 4],
    pub next: *mut MallocState,
    pub next_free: *mut MallocState,
    pub attached_threads: usize,
    pub system_mem: usize,
    pub max_system_mem: usize,
}

#[repr(C)]
pub struct HeapLensChunkInfo {
    pub address: u64,
    pub raw: MallocChunk,
    pub decoded_size: usize,
    pub usable_size: usize,
    pub prev_inuse: bool,
    pub is_mmapped: bool,
    pub non_main_arena: bool,
    pub is_valid: bool,
    pub decoded_fd: usize,
    pub decoded_bk: usize,
    pub bin_name: [c_char; HEAPLENS_CHUNK_LABEL_MAX],
    pub validation_error: [c_char; HEAPLENS_CHUNK_VALIDATION_MAX],
}

#[repr(C)]
pub struct HeapLensHeapSnapshot {
    pub heap_start: u64,
    pub heap_end: u64,
    pub tcache_address: u64,
    pub arena: MallocState,
    pub have_arena: bool,
    pub chunks: *mut HeapLensChunkInfo,
    pub chunk_count: usize,
}

#[repr(C)]
pub struct HeapLensMemoryMapEntry {
    pub start: u64,
    pub end: u64,
    pub perms: [c_char; 5],
    pub offset: u64,
    pub dev: [c_char; 16],
    pub inode: c_ulong,
    pub pathname: [c_char; PATH_MAX],
}

#[repr(C)]
pub struct HeapLensMemoryMapSnapshot {
    pub entries: *mut HeapLensMemoryMapEntry,
    pub count: usize,
}

#[repr(C)]
pub struct HeapLensMemoryReader {
    pub pid: libc::pid_t,
    pub mem_fd: c_int,
    pub mem_path: [c_char; 64],
}

#[repr(C)]
pub struct HeapLensRegisterSnapshot {
    pub gpr: libc::user_regs_struct,
    pub fpregs: libc::user_fpregs_struct,
    pub have_fpregs: bool,
}

#[repr(C)]
pub struct HeapLensTargetProcess {
    pub pid: libc::pid_t,
    pub stage_fd: c_int,
    pub running: bool,
    pub traced: bool,
    pub binary_path: [c_char; PATH_MAX],
    pub preload_library: [c_char; PATH_MAX],
    pub glibc_version: [c_char; 16],
    pub last_stage: [c_char; 128],
    pub reader: HeapLensMemoryReader,
}

#[repr(C)]
pub struct HeapLensDisasmEngine {
    pub handle: usize,
    pub ready: bool,
}

#[repr(C)]
pub struct HeapLensTechniqueInfo {
    pub id: *const c_char,
    pub category: *const c_char,
    pub label: *const c_char,
}

extern "C" {
    pub fn heaplens_technique_registry_count() -> usize;
    pub fn heaplens_technique_registry_get(index: usize) -> *const HeapLensTechniqueInfo;
    pub fn heaplens_technique_registry_has_glibc_runtime(glibc_version: *const c_char) -> bool;
    pub fn heaplens_technique_registry_resolve_binary(
        info: *const HeapLensTechniqueInfo,
        glibc_version: *const c_char,
        buffer: *mut c_char,
        buffer_size: usize,
    ) -> bool;
    pub fn heaplens_technique_registry_build_theory(
        info: *const HeapLensTechniqueInfo,
        glibc_version: *const c_char,
    ) -> *mut c_char;
    pub fn heaplens_technique_registry_build_banner(
        info: *const HeapLensTechniqueInfo,
        glibc_version: *const c_char,
        binary_path: *const c_char,
        pid: libc::pid_t,
    ) -> *mut c_char;

    pub fn heaplens_disasm_engine_init(engine: *mut HeapLensDisasmEngine) -> bool;
    pub fn heaplens_disasm_engine_shutdown(engine: *mut HeapLensDisasmEngine);
    pub fn heaplens_disasm_process_region(
        engine: *mut HeapLensDisasmEngine,
        reader: *mut HeapLensMemoryReader,
        address: u64,
        size: usize,
    ) -> *mut c_char;

    pub fn heaplens_process_spawn(
        process: *mut HeapLensTargetProcess,
        binary_path: *const c_char,
        glibc_version: *const c_char,
        preload_library: *const c_char,
        argv: *mut *mut c_char,
    ) -> bool;
    pub fn heaplens_process_continue(process: *mut HeapLensTargetProcess) -> bool;
    pub fn heaplens_process_wait_for_stage(
        process: *mut HeapLensTargetProcess,
        timeout_ms: c_int,
        stage_line: *mut c_char,
        stage_line_size: usize,
    ) -> bool;
    pub fn heaplens_process_snapshot(
        process: *mut HeapLensTargetProcess,
        registers: *mut HeapLensRegisterSnapshot,
        heap_snapshot: *mut HeapLensHeapSnapshot,
    ) -> bool;
    pub fn heaplens_process_interrupt(process: *mut HeapLensTargetProcess) -> bool;
    pub fn heaplens_process_destroy(process: *mut HeapLensTargetProcess);

    pub fn heaplens_ptrace_wait_stopped(
        pid: libc::pid_t,
        status: *mut c_int,
        timeout_ms: c_int,
    ) -> bool;
    pub fn heaplens_heap_snapshot_free(snapshot: *mut HeapLensHeapSnapshot);

    pub fn heaplens_memory_reader_read_fully(
        reader: *mut HeapLensMemoryReader,
        address: u64,
        buffer: *mut c_void,
        size: usize,
    ) -> bool;
    pub fn heaplens_memory_reader_read_maps(
        pid: libc::pid_t,
        snapshot: *mut HeapLensMemoryMapSnapshot,
    ) -> bool;
    pub fn heaplens_memory_map_snapshot_free(snapshot: *mut HeapLensMemoryMapSnapshot);

    pub fn heaplens_format_syscall(
        syscall_number: libc::c_long,
        arguments: *const c_ulong,
        return_value: libc::c_long,
        buffer: *mut c_char,
        buffer_size: usize,
    );
}

pub fn empty_process() -> HeapLensTargetProcess {
    let mut process = unsafe { MaybeUninit::<HeapLensTargetProcess>::zeroed().assume_init() };
    process.stage_fd = -1;
    process.reader.mem_fd = -1;
    process
}

pub fn empty_register_snapshot() -> HeapLensRegisterSnapshot {
    unsafe { MaybeUninit::<HeapLensRegisterSnapshot>::zeroed().assume_init() }
}

pub fn empty_heap_snapshot() -> HeapLensHeapSnapshot {
    unsafe { MaybeUninit::<HeapLensHeapSnapshot>::zeroed().assume_init() }
}

pub fn empty_disasm_engine() -> HeapLensDisasmEngine {
    unsafe { MaybeUninit::<HeapLensDisasmEngine>::zeroed().assume_init() }
}

pub fn empty_memory_map_snapshot() -> HeapLensMemoryMapSnapshot {
    unsafe { MaybeUninit::<HeapLensMemoryMapSnapshot>::zeroed().assume_init() }
}

pub fn c_array_to_string(buffer: &[c_char]) -> String {
    let ptr = buffer.as_ptr();
    if ptr.is_null() {
        return String::new();
    }

    unsafe { CStr::from_ptr(ptr).to_string_lossy().into_owned() }
}

pub unsafe fn take_glib_string(ptr: *mut c_char) -> String {
    if ptr.is_null() {
        return String::new();
    }

    let string = CStr::from_ptr(ptr).to_string_lossy().into_owned();
    glib::ffi::g_free(ptr as *mut c_void);
    string
}

pub unsafe fn take_malloc_string(ptr: *mut c_char) -> String {
    if ptr.is_null() {
        return String::new();
    }

    let string = CStr::from_ptr(ptr).to_string_lossy().into_owned();
    libc::free(ptr as *mut c_void);
    string
}
