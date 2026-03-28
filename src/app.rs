use std::cell::{Cell, RefCell};
use std::ffi::{CStr, CString};
use std::fs;
use std::os::raw::{c_char, c_int, c_ulong};
use std::slice;

use crate::ffi;

#[derive(Clone, Debug)]
pub struct Technique {
    pub index: usize,
    pub id: String,
    pub category: String,
    pub label: String,
}

#[derive(Clone, Debug)]
pub struct PracticeSource {
    pub provider: &'static str,
    pub title: &'static str,
    pub summary: &'static str,
    pub url: &'static str,
    pub glibc_note: &'static str,
    pub category_match: Option<&'static str>,
    pub keyword: Option<&'static str>,
    pub generic: bool,
}

#[derive(Clone, Debug)]
pub struct MitigationBadge {
    pub name: &'static str,
    pub enabled: bool,
    pub detail: &'static str,
}

#[derive(Clone, Debug)]
pub struct DisasmRow {
    pub current_ip: bool,
    pub breakpoint: bool,
    pub address: String,
    pub bytes: String,
    pub mnemonic: String,
    pub comment: String,
}

#[derive(Clone, Debug)]
pub struct HeapChunkView {
    pub kind: String,
    pub title: String,
    pub detail: String,
    pub validation: String,
}

#[derive(Clone, Debug)]
pub struct StackRow {
    pub address: String,
    pub hex: String,
    pub decimal: String,
    pub ascii: String,
}

#[derive(Clone, Debug)]
pub struct RegisterRow {
    pub name: &'static str,
    pub hex: String,
    pub decimal: String,
    pub ascii: String,
    pub changed: bool,
}

#[derive(Clone, Debug)]
pub struct FlagState {
    pub name: &'static str,
    pub set: bool,
}

#[derive(Clone, Debug)]
pub struct SnapshotView {
    pub rip_address: String,
    pub rip_symbol: String,
    pub disasm_rows: Vec<DisasmRow>,
    pub heap_summary: String,
    pub heap_chunks: Vec<HeapChunkView>,
    pub stack_rows: Vec<StackRow>,
    pub register_rows: Vec<RegisterRow>,
    pub flags: Vec<FlagState>,
    pub process_text: String,
}

pub struct AppState {
    techniques: Vec<Technique>,
    glibc_versions: &'static [&'static str],
    selected_technique: Cell<usize>,
    selected_glibc: Cell<usize>,
    process: RefCell<ffi::HeapLensTargetProcess>,
    registers: RefCell<ffi::HeapLensRegisterSnapshot>,
    heap: RefCell<ffi::HeapLensHeapSnapshot>,
    disasm: RefCell<ffi::HeapLensDisasmEngine>,
    disasm_ready: Cell<bool>,
    have_snapshot: Cell<bool>,
    using_custom_target: Cell<bool>,
    custom_binary: RefCell<String>,
    custom_library: RefCell<String>,
    trace_lines: RefCell<Vec<String>>,
    disasm_text: RefCell<String>,
    last_event: RefCell<String>,
    previous_registers: RefCell<Vec<u64>>,
}

impl AppState {
    pub fn new() -> Self {
        let techniques = unsafe {
            (0..ffi::heaplens_technique_registry_count())
                .filter_map(|index| {
                    let info = ffi::heaplens_technique_registry_get(index);
                    if info.is_null() {
                        return None;
                    }

                    Some(Technique {
                        index,
                        id: copy_cstr((*info).id),
                        category: copy_cstr((*info).category),
                        label: copy_cstr((*info).label),
                    })
                })
                .collect()
        };

        let mut disasm = ffi::empty_disasm_engine();
        let disasm_ready = unsafe { ffi::heaplens_disasm_engine_init(&mut disasm) };

        Self {
            techniques,
            glibc_versions: &["2.23", "2.27", "2.29", "2.31", "2.32", "2.34", "2.35", "2.38", "2.39"],
            selected_technique: Cell::new(0),
            selected_glibc: Cell::new(6),
            process: RefCell::new(ffi::empty_process()),
            registers: RefCell::new(ffi::empty_register_snapshot()),
            heap: RefCell::new(ffi::empty_heap_snapshot()),
            disasm: RefCell::new(disasm),
            disasm_ready: Cell::new(disasm_ready),
            have_snapshot: Cell::new(false),
            using_custom_target: Cell::new(false),
            custom_binary: RefCell::new(String::new()),
            custom_library: RefCell::new(String::new()),
            trace_lines: RefCell::new(vec!["Exploit lab ready.".to_string()]),
            disasm_text: RefCell::new("No disassembly loaded.".to_string()),
            last_event: RefCell::new("ready".to_string()),
            previous_registers: RefCell::new(Vec::new()),
        }
    }

    pub fn techniques(&self) -> &[Technique] {
        &self.techniques
    }

    pub fn glibc_versions(&self) -> &'static [&'static str] {
        self.glibc_versions
    }

    pub fn selected_technique_index(&self) -> usize {
        self.selected_technique.get()
    }

    pub fn selected_glibc_index(&self) -> usize {
        self.selected_glibc.get()
    }

    pub fn selected_glibc(&self) -> &'static str {
        self.glibc_versions[self.selected_glibc.get()]
    }

    pub fn selected_glibc_runtime_available(&self) -> bool {
        let glibc = CString::new(self.selected_glibc()).expect("valid glibc version");
        unsafe { ffi::heaplens_technique_registry_has_glibc_runtime(glibc.as_ptr()) }
    }

    pub fn selected_technique(&self) -> Option<&Technique> {
        self.techniques.get(self.selected_technique.get())
    }

    pub fn set_selected_technique(&self, index: usize) {
        if index < self.techniques.len() && self.selected_technique.get() != index {
            self.selected_technique.set(index);
            self.reset_lab(false);
            self.set_event("selection changed");
        }
    }

    pub fn set_selected_glibc(&self, index: usize) {
        if index < self.glibc_versions.len() && self.selected_glibc.get() != index {
            self.selected_glibc.set(index);
            self.reset_lab(false);
            self.set_event("glibc changed");
        }
    }

    pub fn custom_binary(&self) -> String {
        self.custom_binary.borrow().clone()
    }

    pub fn custom_library(&self) -> String {
        self.custom_library.borrow().clone()
    }

    pub fn set_custom_binary(&self, value: String) {
        *self.custom_binary.borrow_mut() = value;
    }

    pub fn set_custom_library(&self, value: String) {
        *self.custom_library.borrow_mut() = value;
    }

    pub fn trace_text(&self) -> String {
        self.trace_lines.borrow().join("\n")
    }

    pub fn record_message(&self, line: &str) {
        self.append_trace(line);
        self.set_event(line);
    }

    pub fn theory_text(&self) -> String {
        unsafe {
            let glibc = CString::new(self.selected_glibc()).expect("valid glibc version");
            let info = self.current_technique_ptr();
            ffi::take_glib_string(ffi::heaplens_technique_registry_build_theory(info, glibc.as_ptr()))
        }
    }

    pub fn source_code(&self) -> String {
        let Some(technique) = self.selected_technique() else {
            return "No technique selected.".to_string();
        };
        let path = format!("src/techniques/demos/{}.c", technique.id);

        fs::read_to_string(&path).unwrap_or_else(|_| format!("/* No template found for {}. */\n", technique.id))
    }

    pub fn practice_sources(&self) -> Vec<PracticeSource> {
        let technique = self.selected_technique();
        let mut highlighted = Vec::new();
        let mut generic = Vec::new();

        for source in PRACTICE_SOURCES {
            if source.generic {
                generic.push(source.clone());
            } else if technique.map_or(false, |item| practice_matches(item, source)) {
                highlighted.push(source.clone());
            }
        }

        highlighted.extend(generic);
        highlighted
    }

    pub fn target_summary(&self) -> String {
        let binary_binding = self.custom_binary.borrow();
        let library_binding = self.custom_library.borrow();
        let binary = trimmed(binary_binding.as_str());
        let library = trimmed(library_binding.as_str());
        let glibc_note = self.glibc_runtime_note();

        if binary.is_empty() {
            format!(
                "Built-in demo active.\nRun and Step use the selected HeapLens demo.\nOptional preload library: {}\n{}",
                if library.is_empty() { "None" } else { library },
                glibc_note
            )
        } else {
            format!(
                "Custom target active.\nBinary: {}\nOptional library: {}\nRun and Step will trace the imported challenge target while technique notes, mitigations, and theory stay aligned to the current selection.\n{}",
                binary,
                if library.is_empty() { "None" } else { library },
                glibc_note
            )
        }
    }

    pub fn mitigation_badges(&self) -> Vec<MitigationBadge> {
        let version = version_code(self.selected_glibc());
        let technique = self.selected_technique();
        let code_exec = technique.map_or(false, |item| {
            item.category.contains("FSOP") || item.category.contains("REAL-WORLD")
        });
        let got_overwrite = technique.map_or(false, |item| item.label.contains("GOT"));

        vec![
            MitigationBadge {
                name: "ASLR",
                enabled: true,
                detail: "Address Space Layout Randomization randomizes heap and module bases.",
            },
            MitigationBadge {
                name: "PIE",
                enabled: true,
                detail: "Position Independent Executables randomize the main binary image.",
            },
            MitigationBadge {
                name: "RELRO",
                enabled: !got_overwrite,
                detail: "RELRO hardens relocation targets such as GOT entries.",
            },
            MitigationBadge {
                name: "NX",
                enabled: !code_exec,
                detail: "NX forces code reuse or data-only primitives instead of stack shellcode.",
            },
            MitigationBadge {
                name: "Canary",
                enabled: true,
                detail: "Stack canaries catch linear stack smashing before return.",
            },
            MitigationBadge {
                name: "Safe-Link",
                enabled: version >= 232,
                detail: "Safe-linking mangles singly-linked freelist pointers.",
            },
            MitigationBadge {
                name: "tcache-key",
                enabled: version >= 229,
                detail: "tcache key tagging detects common same-thread double frees.",
            },
            MitigationBadge {
                name: "vtable",
                enabled: version >= 224,
                detail: "libio vtable validation blocks stale FILE hijacks on newer builds.",
            },
        ]
    }

    pub fn reset_lab(&self, announce: bool) {
        {
            let mut process = self.process.borrow_mut();
            unsafe {
                ffi::heaplens_process_destroy(&mut *process);
            }
            *process = ffi::empty_process();
        }
        {
            let mut heap = self.heap.borrow_mut();
            unsafe {
                ffi::heaplens_heap_snapshot_free(&mut *heap);
            }
            *heap = ffi::empty_heap_snapshot();
        }

        *self.registers.borrow_mut() = ffi::empty_register_snapshot();
        self.have_snapshot.set(false);
        self.using_custom_target.set(false);
        self.disasm_text.replace("No disassembly loaded.".to_string());
        self.previous_registers.borrow_mut().clear();
        if announce {
            self.append_trace("Reset current session.");
            self.set_event("reset");
        }
    }

    pub fn run_selected(&self) -> Result<(), String> {
        self.start_or_advance()
    }

    pub fn step_selected(&self) -> Result<(), String> {
        self.start_or_advance()
    }

    pub fn pause(&self) -> Result<(), String> {
        let mut process = self.process.borrow_mut();
        if !process.running {
            return Ok(());
        }

        let interrupted = unsafe { ffi::heaplens_process_interrupt(&mut *process) };
        if !interrupted {
            return Err("Failed to interrupt traced process.".to_string());
        }

        unsafe {
            ffi::heaplens_ptrace_wait_stopped(process.pid, std::ptr::null_mut(), 1000);
        }
        drop(process);
        self.refresh_snapshot(Some("paused"))
    }

    pub fn write_snapshot_json(&self) -> Result<String, String> {
        if !self.process.borrow().running {
            return Err("No running target to snapshot.".to_string());
        }

        {
            let mut process = self.process.borrow_mut();
            if unsafe { ffi::heaplens_process_interrupt(&mut *process) } {
                unsafe {
                    ffi::heaplens_ptrace_wait_stopped(process.pid, std::ptr::null_mut(), 1000);
                }
            }
        }

        self.refresh_snapshot(Some("manual snapshot"))?;
        let pid = self.process.borrow().pid;
        let path = format!("/tmp/heaplens_snapshot_{}.json", pid);
        fs::write(&path, self.snapshot_json()).map_err(|error| error.to_string())?;
        self.append_trace(&path);
        Ok(path)
    }

    pub fn snapshot_json(&self) -> String {
        let process = self.process.borrow();
        let registers = self.registers.borrow();
        let heap = self.heap.borrow();
        let technique = self
            .selected_technique()
            .map(|item| item.id.as_str())
            .unwrap_or("none");

        let mut chunks = String::new();
        if !heap.chunks.is_null() && heap.chunk_count > 0 {
            let items = unsafe { slice::from_raw_parts(heap.chunks, heap.chunk_count) };
            for (index, chunk) in items.iter().enumerate() {
                let comma = if index + 1 == items.len() { "" } else { "," };
                chunks.push_str(&format!(
                    "      {{\"addr\":\"0x{:x}\",\"size\":\"0x{:x}\",\"bin\":\"{}\",\"valid\":{},\"fd\":\"0x{:x}\",\"bk\":\"0x{:x}\"}}{}\n",
                    chunk.address,
                    chunk.decoded_size,
                    json_escape(&ffi::c_array_to_string(&chunk.bin_name)),
                    chunk.is_valid,
                    chunk.decoded_fd,
                    chunk.decoded_bk,
                    comma
                ));
            }
        }

        format!(
            "{{\n  \"technique\": \"{}\",\n  \"glibc\": \"{}\",\n  \"target_mode\": \"{}\",\n  \"binary_path\": \"{}\",\n  \"preload_library\": \"{}\",\n  \"pid\": {},\n  \"running\": {},\n  \"registers\": {{\n    \"rip\": \"0x{:x}\",\n    \"rsp\": \"0x{:x}\",\n    \"rbp\": \"0x{:x}\",\n    \"rax\": \"0x{:x}\"\n  }},\n  \"heap\": {{\n    \"start\": \"0x{:x}\",\n    \"end\": \"0x{:x}\",\n    \"chunks\": [\n{}    ]\n  }}\n}}\n",
            json_escape(technique),
            json_escape(self.selected_glibc()),
            if self.using_custom_target.get() {
                "custom-or-imported"
            } else {
                "built-in-demo"
            },
            json_escape(&ffi::c_array_to_string(&process.binary_path)),
            json_escape(&ffi::c_array_to_string(&process.preload_library)),
            process.pid,
            process.running,
            registers.gpr.rip,
            registers.gpr.rsp,
            registers.gpr.rbp,
            registers.gpr.rax,
            heap.heap_start,
            heap.heap_end,
            chunks
        )
    }

    pub fn status_text(&self) -> String {
        let technique = self
            .selected_technique()
            .map(|item| item.label.as_str())
            .unwrap_or("No technique");
        let event = self.last_event.borrow().clone();
        let process = self.process.borrow();

        if process.running && self.have_snapshot.get() {
            let registers = self.registers.borrow();
            let heap = self.heap.borrow();
            format!(
                "PAUSED | pid {} | {} | glibc {} | rip 0x{:x} | heap 0x{:x}-0x{:x} | {}",
                process.pid,
                technique,
                self.selected_glibc(),
                registers.gpr.rip,
                heap.heap_start,
                heap.heap_end,
                event
            )
        } else if process.running {
            format!(
                "RUNNING | pid {} | {} | glibc {} | {}",
                process.pid,
                technique,
                self.selected_glibc(),
                event
            )
        } else {
            format!(
                "IDLE | {} | glibc {} | select a technique, then run or step | {}",
                technique,
                self.selected_glibc(),
                event
            )
        }
    }

    pub fn snapshot_view(&self) -> Option<SnapshotView> {
        if !self.have_snapshot.get() || !self.process.borrow().running {
            return None;
        }

        let process = self.process.borrow();
        let registers = self.registers.borrow();
        let heap = self.heap.borrow();
        let disasm_rows = parse_disasm(self.disasm_text.borrow().as_str(), registers.gpr.rip);
        let stack_rows = self.stack_rows(&process, &registers);
        let register_rows = build_register_rows(&registers.gpr, &self.previous_registers.borrow());
        let flags = build_flags(registers.gpr.eflags);
        let process_text = self.process_text(&process);
        let rip_symbol = if ffi::c_array_to_string(&process.last_stage).is_empty() {
            ffi::c_array_to_string(&process.binary_path)
        } else {
            ffi::c_array_to_string(&process.last_stage)
        };

        Some(SnapshotView {
            rip_address: format!("0x{:016x}", registers.gpr.rip),
            rip_symbol,
            disasm_rows,
            heap_summary: format!(
                "heap 0x{:x}-0x{:x} | {} chunks | tcache 0x{:x}",
                heap.heap_start,
                heap.heap_end,
                heap.chunk_count,
                heap.tcache_address
            ),
            heap_chunks: build_heap_chunks(&heap),
            stack_rows,
            register_rows,
            flags,
            process_text,
        })
    }

    fn start_or_advance(&self) -> Result<(), String> {
        let technique = self
            .selected_technique()
            .ok_or_else(|| "No technique selected.".to_string())?;
        let glibc = CString::new(self.selected_glibc()).expect("valid glibc");
        let custom_binary = trimmed(self.custom_binary.borrow().as_str()).to_string();
        let custom_library = trimmed(self.custom_library.borrow().as_str()).to_string();
        let missing_runtime = !self.selected_glibc_runtime_available();

        {
            let mut process = self.process.borrow_mut();

            if !process.running {
                let mut binary_buffer = [0 as c_char; ffi::PATH_MAX];
                let launch_path = if custom_binary.is_empty() {
                    let resolved = unsafe {
                        ffi::heaplens_technique_registry_resolve_binary(
                            self.current_technique_ptr(),
                            glibc.as_ptr(),
                            binary_buffer.as_mut_ptr(),
                            binary_buffer.len(),
                        )
                    };
                    if !resolved {
                        return Err("No demo binary found for the selected technique/version.".to_string());
                    }
                    ffi::c_array_to_string(&binary_buffer)
                } else {
                    if !path_access(&custom_binary, libc::X_OK) {
                        return Err("Custom challenge binary is missing or not executable.".to_string());
                    }
                    custom_binary.clone()
                };

                if !custom_library.is_empty() && !path_access(&custom_library, libc::R_OK) {
                    return Err("Attached runtime library is missing or unreadable.".to_string());
                }

                let binary_c = CString::new(launch_path.clone()).map_err(|error| error.to_string())?;
                let library_c = if custom_library.is_empty() {
                    None
                } else {
                    Some(CString::new(custom_library.clone()).map_err(|error| error.to_string())?)
                };
                let mut argv = vec![binary_c.as_ptr() as *mut c_char, std::ptr::null_mut()];
                let spawned = unsafe {
                    ffi::heaplens_process_spawn(
                        &mut *process,
                        binary_c.as_ptr(),
                        glibc.as_ptr(),
                        library_c
                            .as_ref()
                            .map(|value| value.as_ptr())
                            .unwrap_or(std::ptr::null()),
                        argv.as_mut_ptr(),
                    )
                };

                if !spawned {
                    return Err(if custom_binary.is_empty() {
                        "Failed to spawn traced demo process.".to_string()
                    } else {
                        "Failed to spawn traced custom challenge process.".to_string()
                    });
                }

                self.using_custom_target.set(!custom_binary.is_empty());
                let banner = if custom_binary.is_empty() {
                    unsafe {
                        ffi::take_glib_string(ffi::heaplens_technique_registry_build_banner(
                            self.current_technique_ptr(),
                            glibc.as_ptr(),
                            binary_c.as_ptr(),
                            process.pid,
                        ))
                    }
                } else {
                    format!(
                        "Technique notes: {}\nglibc selector: {}\nbinary: {}\nruntime library: {}\npid: {}\nImported challenge binaries may stop only at exec or external signals unless they emit HeapLens stage markers.",
                        technique.label,
                        self.selected_glibc(),
                        launch_path,
                        if custom_library.is_empty() { "<none>" } else { custom_library.as_str() },
                        process.pid
                    )
                };

                if custom_binary.is_empty() && missing_runtime {
                    self.append_trace(&format!(
                        "glibc {} bundle missing under glibc/{}/lib; launched the native demo runtime instead.",
                        self.selected_glibc(),
                        self.selected_glibc()
                    ));
                }

                for line in banner.lines() {
                    self.append_trace(line);
                }
                self.set_event(if custom_binary.is_empty() {
                    "demo attached"
                } else {
                    "imported target attached"
                });
            } else {
                let continued = unsafe { ffi::heaplens_process_continue(&mut *process) };
                if !continued {
                    return Err("Failed to continue traced process.".to_string());
                }
            }
        }

        let mut stage = [0 as c_char; 128];
        let waited = {
            let mut process = self.process.borrow_mut();
            unsafe {
                ffi::heaplens_process_wait_for_stage(&mut *process, 2000, stage.as_mut_ptr(), stage.len())
            }
        };

        if !waited {
            self.have_snapshot.set(false);
            self.set_event("process exited");
            self.append_trace("Process exited before the next stage.");
            return Err("Process exited before the next stage.".to_string());
        }

        let stage_line = unsafe { CStr::from_ptr(stage.as_ptr()).to_string_lossy().into_owned() };
        self.refresh_snapshot(Some(stage_line.trim()))
    }

    fn refresh_snapshot(&self, stage_line: Option<&str>) -> Result<(), String> {
        if self.have_snapshot.get() {
            let current = register_values(&self.registers.borrow().gpr);
            *self.previous_registers.borrow_mut() = current;
        } else {
            self.previous_registers.borrow_mut().clear();
        }

        {
            let mut heap = self.heap.borrow_mut();
            unsafe {
                ffi::heaplens_heap_snapshot_free(&mut *heap);
            }
            *heap = ffi::empty_heap_snapshot();
        }
        *self.registers.borrow_mut() = ffi::empty_register_snapshot();

        let snapped = {
            let mut process = self.process.borrow_mut();
            let mut registers = self.registers.borrow_mut();
            let mut heap = self.heap.borrow_mut();

            if !process.running {
                false
            } else {
                unsafe { ffi::heaplens_process_snapshot(&mut *process, &mut *registers, &mut *heap) }
            }
        };

        if !snapped {
            self.have_snapshot.set(false);
            self.set_event("snapshot failed");
            self.append_trace("Snapshot failed or process exited.");
            self.disasm_text.replace("Disassembly unavailable.".to_string());
            return Err("Snapshot failed or process exited.".to_string());
        }

        self.have_snapshot.set(true);

        if self.disasm_ready.get() {
            let mut disasm = self.disasm.borrow_mut();
            let mut process = self.process.borrow_mut();
            let rip = self.registers.borrow().gpr.rip;
            let ptr = unsafe {
                ffi::heaplens_disasm_process_region(&mut *disasm, &mut process.reader, rip, 96)
            };
            let text = unsafe { ffi::take_malloc_string(ptr) };
            self.disasm_text.replace(if text.is_empty() {
                "Disassembly unavailable.".to_string()
            } else {
                text
            });
        }

        let registers = self.registers.borrow();
        let mut syscall_buffer = [0 as c_char; 512];
        let args: [c_ulong; 6] = [
            registers.gpr.rdi as c_ulong,
            registers.gpr.rsi as c_ulong,
            registers.gpr.rdx as c_ulong,
            registers.gpr.r10 as c_ulong,
            registers.gpr.r8 as c_ulong,
            registers.gpr.r9 as c_ulong,
        ];

        unsafe {
            ffi::heaplens_format_syscall(
                registers.gpr.orig_rax as libc::c_long,
                args.as_ptr(),
                registers.gpr.rax as libc::c_long,
                syscall_buffer.as_mut_ptr(),
                syscall_buffer.len(),
            );
        }
        drop(registers);

        if let Some(line) = stage_line.filter(|value| !value.is_empty()) {
            self.append_trace(line);
            self.set_event(line);
        } else {
            self.set_event("snapshot");
        }
        self.append_trace(&ffi::c_array_to_string(&syscall_buffer));
        Ok(())
    }

    fn glibc_runtime_note(&self) -> String {
        if self.selected_glibc_runtime_available() {
            format!(
                "glibc {} runtime bundle is installed and available for built-in demos.",
                self.selected_glibc()
            )
        } else {
            format!(
                "glibc {} runtime bundle is missing; built-in demos fall back to the native host libc until glibc/{}/lib contains ld.so and libc.so.6.",
                self.selected_glibc(),
                self.selected_glibc()
            )
        }
    }

    fn stack_rows(
        &self,
        process: &ffi::HeapLensTargetProcess,
        registers: &ffi::HeapLensRegisterSnapshot,
    ) -> Vec<StackRow> {
        let mut words = [0u64; 12];
        let ok = unsafe {
            ffi::heaplens_memory_reader_read_fully(
                &process.reader as *const _ as *mut _,
                registers.gpr.rsp,
                words.as_mut_ptr() as *mut _,
                std::mem::size_of_val(&words),
            )
        };

        if !ok {
            return Vec::new();
        }

        words
            .iter()
            .enumerate()
            .map(|(index, value)| StackRow {
                address: format!("0x{:016x}", registers.gpr.rsp + (index as u64 * 8)),
                hex: format!("0x{:016x}", value),
                decimal: format!("{}", value),
                ascii: ascii_for_u64(*value),
            })
            .collect()
    }

    fn process_text(&self, process: &ffi::HeapLensTargetProcess) -> String {
        let preload_library = ffi::c_array_to_string(&process.preload_library);
        let last_stage = ffi::c_array_to_string(&process.last_stage);
        let mut lines = vec![
            format!(
                "Target: {}",
                if self.using_custom_target.get() {
                    "Imported challenge"
                } else {
                    "Built-in demo"
                }
            ),
            format!("pid: {}", process.pid),
            format!("binary: {}", ffi::c_array_to_string(&process.binary_path)),
            format!("glibc: {}", ffi::c_array_to_string(&process.glibc_version)),
            format!("preload library: {}", empty_or(preload_library.as_str(), "<none>")),
            format!("last stage: {}", empty_or(last_stage.as_str(), "<waiting>")),
        ];

        let mut maps = ffi::empty_memory_map_snapshot();
        let have_maps = unsafe { ffi::heaplens_memory_reader_read_maps(process.pid, &mut maps) };
        if have_maps && !maps.entries.is_null() {
            let entries = unsafe { slice::from_raw_parts(maps.entries, maps.count) };
            lines.push(String::new());
            lines.push(format!("Mapped regions: {}", entries.len()));

            for entry in entries.iter().take(14) {
                let pathname = ffi::c_array_to_string(&entry.pathname);
                lines.push(format!(
                    "0x{:016x}-0x{:016x} {} {}",
                    entry.start,
                    entry.end,
                    ffi::c_array_to_string(&entry.perms),
                    empty_or(pathname.as_str(), "<anonymous>")
                ));
            }
        }
        unsafe {
            ffi::heaplens_memory_map_snapshot_free(&mut maps);
        }

        lines.join("\n")
    }

    fn append_trace(&self, line: &str) {
        if !line.is_empty() {
            self.trace_lines.borrow_mut().push(line.to_string());
        }
    }

    fn current_technique_ptr(&self) -> *const ffi::HeapLensTechniqueInfo {
        unsafe { ffi::heaplens_technique_registry_get(self.selected_technique.get()) }
    }

    fn set_event(&self, event: &str) {
        self.last_event.replace(event.to_string());
    }
}

impl Drop for AppState {
    fn drop(&mut self) {
        unsafe {
            ffi::heaplens_heap_snapshot_free(self.heap.get_mut());
            ffi::heaplens_process_destroy(self.process.get_mut());
            if self.disasm_ready.get() {
                ffi::heaplens_disasm_engine_shutdown(self.disasm.get_mut());
            }
        }
    }
}

const PRACTICE_SOURCES: &[PracticeSource] = &[
    PracticeSource {
        provider: "shellphish/how2heap",
        title: "Versioned heap technique playground",
        summary: "Official single-file heap technique repository with per-glibc examples.",
        url: "https://github.com/shellphish/how2heap",
        glibc_note: "Covers multiple glibc versions in-repo",
        category_match: None,
        keyword: None,
        generic: true,
    },
    PracticeSource {
        provider: "pwn.college",
        title: "Dynamic Allocator Misuse",
        summary: "Intro heap practice centered on tcache, metadata corruption, and use-after-free.",
        url: "https://pwn.college/cse598-s2024/dynamic-allocator-misuse",
        glibc_note: "Intro heap and tcache practice",
        category_match: Some("USE-AFTER-FREE|DOUBLE FREE|TCACHE EXPLOITATION|HEAP OVERFLOW"),
        keyword: Some("tcache"),
        generic: false,
    },
    PracticeSource {
        provider: "pwn.college",
        title: "Dynamic Allocator II - Exploitation",
        summary: "Advanced heap practice with glibc 2.35 notes and technique-to-primitive guidance.",
        url: "https://pwn.college/cse598-se-s2025/dynamic-allocator-exploitation/",
        glibc_note: "Official practice module notes: glibc 2.35",
        category_match: Some("DOUBLE FREE|TCACHE EXPLOITATION|FASTBIN ATTACKS|UNSORTED BIN ATTACKS|SMALLBIN ATTACKS|LARGEBIN ATTACKS|UNLINK ATTACKS|HOUSE OF TECHNIQUES|INFORMATION LEAKS|FSOP|REAL-WORLD TECHNIQUE CHAINS|HEAP GROOMING AND SHAPING"),
        keyword: None,
        generic: false,
    },
    PracticeSource {
        provider: "pwn.college",
        title: "Advanced Exploitation Archive",
        summary: "Archived advanced exploitation dojo for longer exploit-chain practice.",
        url: "https://pwn.college/archive/advanced-exploitation/",
        glibc_note: "Archived advanced practice",
        category_match: Some("FSOP|REAL-WORLD TECHNIQUE CHAINS|INFORMATION LEAKS|ALLOCATOR CONFUSION"),
        keyword: None,
        generic: false,
    },
];

fn copy_cstr(ptr: *const c_char) -> String {
    if ptr.is_null() {
        String::new()
    } else {
        unsafe { CStr::from_ptr(ptr).to_string_lossy().into_owned() }
    }
}

fn trimmed(value: &str) -> &str {
    value.trim()
}

fn empty_or<'a>(value: &'a str, fallback: &'a str) -> &'a str {
    if value.is_empty() {
        fallback
    } else {
        value
    }
}

fn practice_matches(technique: &Technique, source: &PracticeSource) -> bool {
    if let Some(category_match) = source.category_match {
        if category_match.contains(technique.category.as_str()) {
            return true;
        }
    }
    if let Some(keyword) = source.keyword {
        if technique.label.contains(keyword) || technique.id.contains(keyword) {
            return true;
        }
    }
    false
}

fn path_access(path: &str, mode: c_int) -> bool {
    CString::new(path)
        .ok()
        .map(|value| unsafe { libc::access(value.as_ptr(), mode) == 0 })
        .unwrap_or(false)
}

fn version_code(version: &str) -> i32 {
    let mut parts = version.split('.');
    let major = parts.next().and_then(|item| item.parse::<i32>().ok()).unwrap_or(0);
    let minor = parts.next().and_then(|item| item.parse::<i32>().ok()).unwrap_or(0);
    major * 100 + minor
}

fn json_escape(value: &str) -> String {
    value.replace('\\', "\\\\").replace('"', "\\\"")
}

fn parse_disasm(text: &str, rip: u64) -> Vec<DisasmRow> {
    let mut rows = Vec::new();

    for line in text.lines() {
        let line = line.trim();
        if line.is_empty() {
            continue;
        }

        let mut address = String::new();
        let mut mnemonic = String::new();
        let mut comment = String::new();

        if let Some((lhs, rhs)) = line.split_once(':') {
            address = lhs.trim().to_string();
            let rhs = rhs.trim();
            if let Some((instruction, trailing_comment)) = rhs.split_once(';') {
                mnemonic = instruction.trim().to_string();
                comment = format!("; {}", trailing_comment.trim());
            } else {
                mnemonic = rhs.to_string();
            }
        } else {
            comment = line.to_string();
        }

        let current_ip = address
            .strip_prefix("0x")
            .and_then(|hex| u64::from_str_radix(hex, 16).ok())
            .map(|value| value == rip)
            .unwrap_or(false);

        rows.push(DisasmRow {
            current_ip,
            breakpoint: false,
            address,
            bytes: String::from(""),
            mnemonic,
            comment,
        });
    }

    if rows.is_empty() {
        rows.push(DisasmRow {
            current_ip: false,
            breakpoint: false,
            address: String::new(),
            bytes: String::new(),
            mnemonic: "no decode".to_string(),
            comment: "No disassembly available.".to_string(),
        });
    }

    rows
}

fn build_heap_chunks(heap: &ffi::HeapLensHeapSnapshot) -> Vec<HeapChunkView> {
    if heap.chunks.is_null() || heap.chunk_count == 0 {
        return Vec::new();
    }

    let chunks = unsafe { slice::from_raw_parts(heap.chunks, heap.chunk_count) };
    chunks
        .iter()
        .map(|chunk| {
            let bin_name = ffi::c_array_to_string(&chunk.bin_name);
            let validation = ffi::c_array_to_string(&chunk.validation_error);
            let kind = if !chunk.is_valid || !validation.is_empty() {
                "chunk-corrupt"
            } else if bin_name.contains("top") {
                "chunk-top"
            } else if bin_name.contains("tcache") {
                "chunk-tcache"
            } else if bin_name.contains("fast") {
                "chunk-fastbin"
            } else if bin_name.contains("free") || chunk.decoded_fd != 0 || chunk.decoded_bk != 0 {
                "chunk-free"
            } else {
                "chunk-alloc"
            };

            HeapChunkView {
                kind: kind.to_string(),
                title: format!(
                    "0x{:016x}  size=0x{:x}  usable=0x{:x}  {}",
                    chunk.address,
                    chunk.decoded_size,
                    chunk.usable_size,
                    if bin_name.is_empty() { "<unclassified>" } else { bin_name.as_str() }
                ),
                detail: format!(
                    "fd=0x{:x}  bk=0x{:x}  flags: P={} M={} A={}",
                    chunk.decoded_fd,
                    chunk.decoded_bk,
                    chunk.prev_inuse as u8,
                    chunk.is_mmapped as u8,
                    chunk.non_main_arena as u8
                ),
                validation,
            }
        })
        .collect()
}

fn build_register_rows(
    regs: &libc::user_regs_struct,
    previous: &[u64],
) -> Vec<RegisterRow> {
    let values = vec![
        ("RAX", regs.rax),
        ("RBX", regs.rbx),
        ("RCX", regs.rcx),
        ("RDX", regs.rdx),
        ("RSI", regs.rsi),
        ("RDI", regs.rdi),
        ("RBP", regs.rbp),
        ("RSP", regs.rsp),
        ("R8", regs.r8),
        ("R9", regs.r9),
        ("R10", regs.r10),
        ("R11", regs.r11),
        ("R12", regs.r12),
        ("R13", regs.r13),
        ("R14", regs.r14),
        ("R15", regs.r15),
        ("RIP", regs.rip),
        ("RFLAGS", regs.eflags),
        ("ORIG_RAX", regs.orig_rax),
    ];

    values
        .into_iter()
        .enumerate()
        .map(|(index, (name, value))| RegisterRow {
            name,
            hex: format!("0x{:016x}", value),
            decimal: format!("{}", value),
            ascii: ascii_for_u64(value),
            changed: previous.get(index).map(|old| *old != value).unwrap_or(false),
        })
        .collect()
}

fn build_flags(rflags: u64) -> Vec<FlagState> {
    [
        ("CF", 0),
        ("PF", 2),
        ("AF", 4),
        ("ZF", 6),
        ("SF", 7),
        ("TF", 8),
        ("IF", 9),
        ("DF", 10),
        ("OF", 11),
    ]
    .into_iter()
    .map(|(name, bit)| FlagState {
        name,
        set: (rflags & (1_u64 << bit)) != 0,
    })
    .collect()
}

fn register_values(regs: &libc::user_regs_struct) -> Vec<u64> {
    vec![
        regs.rax,
        regs.rbx,
        regs.rcx,
        regs.rdx,
        regs.rsi,
        regs.rdi,
        regs.rbp,
        regs.rsp,
        regs.r8,
        regs.r9,
        regs.r10,
        regs.r11,
        regs.r12,
        regs.r13,
        regs.r14,
        regs.r15,
        regs.rip,
        regs.eflags,
        regs.orig_rax,
    ]
}

fn ascii_for_u64(value: u64) -> String {
    let bytes = value.to_le_bytes();
    bytes
        .iter()
        .map(|byte| {
            if byte.is_ascii_graphic() || *byte == b' ' {
                *byte as char
            } else {
                '.'
            }
        })
        .collect()
}
