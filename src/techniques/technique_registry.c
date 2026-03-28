#include "technique_registry.h"

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TECH(category, id, label) {id, category, label}

static const HeapLensTechniqueInfo heaplens_techniques[] = {
    TECH("GLIBC BASICS", "malloc_free_fundamentals", "malloc / free fundamentals (live chunk inspection)"),
    TECH("GLIBC BASICS", "heap_layout_walkthrough", "Heap layout walkthrough (visualize a real heap)"),
    TECH("GLIBC BASICS", "bin_state_explorer", "Bin state explorer (interactive bin manipulation)"),
    TECH("USE-AFTER-FREE", "uaf_basic", "UAF Basic - read dangling pointer after free"),
    TECH("USE-AFTER-FREE", "uaf_type_confusion", "UAF Type Confusion - reinterpret freed object as different type"),
    TECH("USE-AFTER-FREE", "uaf_to_arbitrary_read", "UAF to Arbitrary Read - leak memory via dangling fd/bk"),
    TECH("USE-AFTER-FREE", "uaf_to_arbitrary_write", "UAF to Arbitrary Write - overwrite via dangling pointer"),
    TECH("USE-AFTER-FREE", "uaf_to_code_execution", "UAF to Code Execution - overwrite function pointer in freed object"),
    TECH("USE-AFTER-FREE", "uaf_via_tcache", "UAF via tcache (glibc >= 2.26)"),
    TECH("USE-AFTER-FREE", "uaf_via_fastbin", "UAF via fastbin (glibc < 2.26)"),
    TECH("HEAP OVERFLOW", "classic_heap_overflow", "Classic Heap Overflow - overflow into next chunk's header"),
    TECH("HEAP OVERFLOW", "off_by_one", "Off-by-One - overflow exactly one byte past allocation"),
    TECH("HEAP OVERFLOW", "off_by_null", "Off-by-Null (Null Byte Poison) - write single NULL byte past allocation"),
    TECH("HEAP OVERFLOW", "off_by_one_into_size_field", "Off-by-One into Size Field - corrupt next chunk's size"),
    TECH("HEAP OVERFLOW", "heap_overflow_into_function_pointer", "Heap Overflow into Function Pointer - overwrite vtable/function ptr"),
    TECH("HEAP OVERFLOW", "heap_overflow_across_pages", "Heap Overflow Across Pages"),
    TECH("DOUBLE FREE", "classic_double_free", "Classic Double Free (glibc < 2.26) - free same chunk twice"),
    TECH("DOUBLE FREE", "fastbin_double_free", "Fastbin Double Free (glibc < 2.32) - bypass single-step check"),
    TECH("DOUBLE FREE", "tcache_double_free_pre_229", "Tcache Double Free (glibc < 2.29) - no key check"),
    TECH("DOUBLE FREE", "tcache_double_free_key_bypass", "Tcache Double Free (glibc >= 2.29) - bypass key check"),
    TECH("DOUBLE FREE", "double_free_safe_linking_bypass", "Double Free via Safe-Linking Bypass (glibc >= 2.32)"),
    TECH("DOUBLE FREE", "double_free_to_arbitrary_allocation", "Double Free to Arbitrary Allocation"),
    TECH("TCACHE EXPLOITATION", "tcache_poisoning", "Tcache Poisoning - corrupt next pointer to redirect allocation"),
    TECH("TCACHE EXPLOITATION", "tcache_dup", "Tcache Dup - allocate same chunk twice"),
    TECH("TCACHE EXPLOITATION", "tcache_metadata_corruption", "Tcache Metadata Corruption - corrupt counts[] or entries[]"),
    TECH("TCACHE EXPLOITATION", "tcache_perthread_struct_overwrite", "Tcache Perthread Struct Overwrite"),
    TECH("TCACHE EXPLOITATION", "tcache_stashing_unlink_attack", "Tcache Stashing Unlink Attack"),
    TECH("TCACHE EXPLOITATION", "tcache_stashing_unlink_attack_plus", "Tcache Stashing Unlink Attack Plus"),
    TECH("TCACHE EXPLOITATION", "tcache_house_of_spirit", "Tcache House of Spirit"),
    TECH("FASTBIN ATTACKS", "fastbin_dup", "Fastbin Dup - allocate same fastbin chunk twice"),
    TECH("FASTBIN ATTACKS", "fastbin_dup_into_stack", "Fastbin Dup into Stack - redirect fastbin to stack address"),
    TECH("FASTBIN ATTACKS", "fastbin_dup_consolidate", "Fastbin Dup Consolidate - use malloc_consolidate to bypass check"),
    TECH("FASTBIN ATTACKS", "fastbin_into_other_bin", "Fastbin into Other Bin"),
    TECH("FASTBIN ATTACKS", "house_of_spirit", "House of Spirit - fake chunk on stack/BSS freed into fastbin"),
    TECH("FASTBIN ATTACKS", "fastbin_corruption", "Fastbin Corruption - corrupt fd pointer directly"),
    TECH("UNSORTED BIN ATTACKS", "unsorted_bin_attack", "Unsorted Bin Attack - overwrite arbitrary qword with main_arena addr"),
    TECH("UNSORTED BIN ATTACKS", "unsorted_bin_leak", "Unsorted Bin Leak - leak libc base via fd/bk pointing to main_arena"),
    TECH("UNSORTED BIN ATTACKS", "unsorted_bin_into_stack", "Unsorted Bin into Stack"),
    TECH("UNSORTED BIN ATTACKS", "unsorted_bin_corruption", "Unsorted Bin Corruption"),
    TECH("SMALLBIN ATTACKS", "smallbin_corruption", "Smallbin Corruption"),
    TECH("SMALLBIN ATTACKS", "smallbin_into_stack", "Smallbin into Stack"),
    TECH("SMALLBIN ATTACKS", "fake_chunk_in_smallbin", "Fake Chunk in Smallbin"),
    TECH("LARGEBIN ATTACKS", "largebin_attack_classic", "Largebin Attack Classic (glibc < 2.30)"),
    TECH("LARGEBIN ATTACKS", "largebin_attack_bk_nextsize", "Largebin Attack (glibc >= 2.30) - via bk_nextsize"),
    TECH("LARGEBIN ATTACKS", "largebin_attack_fd_nextsize", "Largebin Attack (glibc >= 2.30) - via fd_nextsize"),
    TECH("LARGEBIN ATTACKS", "largebin_leak", "Largebin Leak"),
    TECH("UNLINK ATTACKS", "classic_unlink_attack", "Classic Unlink Attack (pre-safe-unlink)"),
    TECH("UNLINK ATTACKS", "safe_unlink_bypass_via_fake_chunk", "Safe-Unlink Bypass via Fake Chunk"),
    TECH("UNLINK ATTACKS", "unlink_to_arbitrary_write", "Unlink to Arbitrary Write"),
    TECH("HOUSE OF TECHNIQUES", "house_of_spirit_house", "House of Spirit"),
    TECH("HOUSE OF TECHNIQUES", "house_of_force", "House of Force"),
    TECH("HOUSE OF TECHNIQUES", "house_of_lore", "House of Lore"),
    TECH("HOUSE OF TECHNIQUES", "house_of_einherjar", "House of Einherjar"),
    TECH("HOUSE OF TECHNIQUES", "house_of_orange", "House of Orange"),
    TECH("HOUSE OF TECHNIQUES", "house_of_rabbit", "House of Rabbit"),
    TECH("HOUSE OF TECHNIQUES", "house_of_roman", "House of Roman"),
    TECH("HOUSE OF TECHNIQUES", "house_of_gods", "House of Gods"),
    TECH("HOUSE OF TECHNIQUES", "house_of_corrosion", "House of Corrosion"),
    TECH("HOUSE OF TECHNIQUES", "house_of_water", "House of Water"),
    TECH("HOUSE OF TECHNIQUES", "house_of_husk", "House of Husk"),
    TECH("HOUSE OF TECHNIQUES", "house_of_botcake", "House of Botcake"),
    TECH("HOUSE OF TECHNIQUES", "house_of_cain", "House of Cain"),
    TECH("HOUSE OF TECHNIQUES", "house_of_rust", "House of Rust"),
    TECH("HOUSE OF TECHNIQUES", "house_of_blindness", "House of Blindness"),
    TECH("HOUSE OF TECHNIQUES", "house_of_io", "House of Io"),
    TECH("HOUSE OF TECHNIQUES", "house_of_emma", "House of Emma"),
    TECH("HOUSE OF TECHNIQUES", "house_of_apple_123", "House of Apple 1/2/3"),
    TECH("HOUSE OF TECHNIQUES", "house_of_tangerine", "House of Tangerine"),
    TECH("HOUSE OF TECHNIQUES", "house_of_cat", "House of Cat"),
    TECH("HOUSE OF TECHNIQUES", "house_of_kiwi", "House of Kiwi"),
    TECH("FSOP", "io_file_struct_layout_walkthrough", "_IO_FILE struct layout walkthrough"),
    TECH("FSOP", "io_file_plus_and_vtable_pointer", "_IO_FILE_plus and vtable pointer"),
    TECH("FSOP", "vtable_hijack", "vtable hijack (glibc < 2.24 - no vtable check)"),
    TECH("FSOP", "io_str_jumps_abuse", "_IO_str_jumps abuse (glibc < 2.28)"),
    TECH("FSOP", "fsop_via_fflush_on_exit", "FSOP via fflush(stdout) on exit"),
    TECH("FSOP", "fsop_via_io_flush_all_lockp", "FSOP via _IO_flush_all_lockp"),
    TECH("FSOP", "io_wfile_jumps_abuse", "_IO_wfile_jumps abuse (House of Cat)"),
    TECH("FSOP", "io_obstack_jumps_abuse", "_IO_obstack_jumps abuse (House of Apple)"),
    TECH("FSOP", "fake_io_file_chain_construction", "Fake _IO_FILE chain construction"),
    TECH("FSOP", "fsop_to_arbitrary_function_call", "FSOP to arbitrary function call"),
    TECH("FSOP", "fsop_to_stack_pivot_rop", "FSOP to stack pivot + ROP"),
    TECH("HEAP GROOMING AND SHAPING", "heap_feng_shui", "Heap Feng Shui - arrange heap layout for reliable exploitation"),
    TECH("HEAP GROOMING AND SHAPING", "hole_punching", "Hole Punching - strategic alloc/free to create gaps"),
    TECH("HEAP GROOMING AND SHAPING", "filling_tcache_before_targeting_fastbin", "Filling tcache before targeting fastbin"),
    TECH("HEAP GROOMING AND SHAPING", "controlling_last_remainder", "Controlling last_remainder via unsorted bin split"),
    TECH("HEAP GROOMING AND SHAPING", "forcing_specific_bin_placement", "Forcing Specific Bin Placement"),
    TECH("INFORMATION LEAKS", "heap_address_leak_via_uninitialized_tcache_fd", "Heap Address Leak via Uninitialized tcache fd"),
    TECH("INFORMATION LEAKS", "libc_base_leak_via_unsorted_bin_fdbk", "libc Base Leak via Unsorted Bin fd/bk"),
    TECH("INFORMATION LEAKS", "libc_base_leak_via_smallbin_fdbk", "libc Base Leak via Smallbin fd/bk"),
    TECH("INFORMATION LEAKS", "heap_base_leak_via_safe_linking_reveal", "Heap Base Leak via Safe-Linking Reveal"),
    TECH("INFORMATION LEAKS", "stack_address_leak_via_environ", "Stack Address Leak via environ pointer in libc"),
    TECH("INFORMATION LEAKS", "leak_via_partial_overwrite", "Leak via Partial Overwrite (brute force ASLR nibble)"),
    TECH("ALLOCATOR CONFUSION", "glibc_vs_jemalloc_difference_demo", "glibc vs jemalloc difference demo"),
    TECH("ALLOCATOR CONFUSION", "glibc_vs_tcmalloc_difference_demo", "glibc vs tcmalloc difference demo"),
    TECH("ALLOCATOR CONFUSION", "interpose_malloc_with_ld_preload_demo", "Interpose malloc with LD_PRELOAD demo"),
    TECH("REAL-WORLD TECHNIQUE CHAINS", "tcache_poison_fsop_no_leak_rop", "tcache Poison + FSOP (no-leak ROP)"),
    TECH("REAL-WORLD TECHNIQUE CHAINS", "largebin_attack_house_of_apple", "Largebin Attack + House of Apple"),
    TECH("REAL-WORLD TECHNIQUE CHAINS", "off_by_null_house_of_einherjar_unsorted_bin_leak", "Off-by-Null + House of Einherjar + Unsorted Bin Leak"),
    TECH("REAL-WORLD TECHNIQUE CHAINS", "uaf_tcache_dup_overwrite_free_hook", "UAF + tcache Dup + Overwrite __free_hook (glibc < 2.34)"),
    TECH("REAL-WORLD TECHNIQUE CHAINS", "unsorted_bin_attack_house_of_orange", "Unsorted Bin Attack + House of Orange (glibc < 2.26)"),
    TECH("REAL-WORLD TECHNIQUE CHAINS", "house_of_roman_full_exploit_no_leak", "House of Roman (full exploit, no leak required)"),
    TECH("REAL-WORLD TECHNIQUE CHAINS", "heap_overflow_safe_unlink_bypass_got_overwrite", "Heap Overflow + Safe-Unlink Bypass + GOT overwrite")
};

static const char *heaplens_category_functions(const char *category) {
    if (g_strcmp0(category, "TCACHE EXPLOITATION") == 0) {
        return "Relevant glibc functions: tcache_get, tcache_put, _int_malloc, _int_free";
    }
    if (g_strcmp0(category, "FASTBIN ATTACKS") == 0) {
        return "Relevant glibc functions: _int_malloc, _int_free, malloc_consolidate";
    }
    if (g_strcmp0(category, "UNSORTED BIN ATTACKS") == 0 || g_strcmp0(category, "SMALLBIN ATTACKS") == 0 ||
        g_strcmp0(category, "LARGEBIN ATTACKS") == 0 || g_strcmp0(category, "UNLINK ATTACKS") == 0) {
        return "Relevant glibc functions: unlink_chunk, _int_malloc, _int_free";
    }
    if (g_strcmp0(category, "FSOP") == 0) {
        return "Relevant glibc functions: _IO_flush_all_lockp, _IO_file_overflow, vtable dispatch sites";
    }
    return "Relevant glibc functions: __libc_malloc, __libc_free, _int_malloc, _int_free";
}

size_t heaplens_technique_registry_count(void) {
    return G_N_ELEMENTS(heaplens_techniques);
}

const HeapLensTechniqueInfo *heaplens_technique_registry_get(size_t index) {
    return index < G_N_ELEMENTS(heaplens_techniques) ? &heaplens_techniques[index] : NULL;
}

const HeapLensTechniqueInfo *heaplens_technique_registry_find_by_label(const char *label) {
    size_t index = 0;
    for (index = 0; index < G_N_ELEMENTS(heaplens_techniques); ++index) {
        if (g_strcmp0(heaplens_techniques[index].label, label) == 0) {
            return &heaplens_techniques[index];
        }
    }
    return NULL;
}

bool heaplens_technique_registry_resolve_binary(const HeapLensTechniqueInfo *info,
                                                const char *glibc_version,
                                                char *buffer,
                                                size_t buffer_size) {
    if (!info || !buffer || buffer_size == 0) {
        return false;
    }

    if (glibc_version && glibc_version[0]) {
        snprintf(buffer, buffer_size, "build/demos/%s/demo_%s_%s", glibc_version, info->id, glibc_version);
        if (access(buffer, X_OK) == 0) {
            return true;
        }
    }

    snprintf(buffer, buffer_size, "build/demos/native/demo_%s_native", info->id);
    if (access(buffer, X_OK) == 0) {
        return true;
    }

    if (glibc_version && glibc_version[0]) {
        snprintf(buffer, buffer_size, "build/demos/%s/demo_generic_demo_%s", glibc_version, glibc_version);
        if (access(buffer, X_OK) == 0) {
            return true;
        }
    }

    snprintf(buffer, buffer_size, "build/demos/native/demo_generic_demo_native");
    return access(buffer, X_OK) == 0;
}

char *heaplens_technique_registry_build_theory(const HeapLensTechniqueInfo *info, const char *glibc_version) {
    if (!info) {
        return g_strdup("Select a technique to load its theory sidebar.");
    }

    return g_strdup_printf(
        "Technique: %s\nCategory: %s\nglibc target: %s\n\n"
        "Overview:\n"
        "This lesson walks through %s with an emphasis on allocator state transitions and why the primitive works. "
        "Use the heap visualizer, register panel, and syscall trace together so the metadata story stays anchored to a live process.\n\n"
        "Prerequisites:\n"
        "- Match the glibc version note embedded in the technique title.\n"
        "- Confirm which mitigations are active in the badge row before stepping.\n"
        "- Expect ptrace privileges and a stoppable demo binary.\n\n"
        "Exact exploitation steps:\n"
        "1. Prepare allocator state for the technique.\n"
        "2. Step until the free-list or chunk header of interest becomes visible.\n"
        "3. Compare the live metadata against the expected corruption pattern.\n"
        "4. Advance carefully and confirm the resulting allocation, leak, or control impact.\n\n"
        "ASCII memory sketch:\n"
        "BEFORE:\n  head -> chunk A -> chunk B -> NULL\n"
        "AFTER:\n  head -> corrupted chunk -> attacker-controlled target\n\n"
        "%s\n\n"
        "Common mistakes:\n"
        "- Landing in the wrong size class.\n"
        "- Clobbering flag bits and tripping allocator sanity checks.\n"
        "- Ignoring safe-linking or tcache-key behavior on newer builds.\n\n"
        "Real-world CVEs:\n"
        "Use this sidebar as the live lab companion and extend it with technique-specific case studies as you expand the demo set.\n\n"
        "What just happened?\n"
        "Watch for list-head changes, chunk-size drift, and control-data reuse after each stage.",
        info->label,
        info->category,
        glibc_version ? glibc_version : "native",
        info->label,
        heaplens_category_functions(info->category));
}

char *heaplens_technique_registry_build_banner(const HeapLensTechniqueInfo *info,
                                               const char *glibc_version,
                                               const char *binary_path,
                                               pid_t pid) {
    return g_strdup_printf(
        "Technique: %s\n"
        "glibc: %s\n"
        "binary: %s\n"
        "pid: %d\n"
        "Suggested commands:\n"
        "  gdb -p %d\n"
        "  (gdb) info registers\n"
        "  (gdb) x/16gx <chunk_addr>\n"
        "  (gdb) x/8i $rip\n",
        info ? info->label : "<none>",
        glibc_version ? glibc_version : "native",
        binary_path ? binary_path : "<unresolved>",
        pid,
        pid);
}
