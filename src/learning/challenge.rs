use std::fs;
use std::path::Path;

#[derive(Clone, Debug)]
pub struct ChallengeDescriptor {
    pub title: String,
    pub description: String,
    pub hint: String,
    pub language: String,
    pub solution: String,
}

struct UpstreamDemo {
    chapter_id: i32,
    title: &'static str,
    description: &'static str,
    hint: &'static str,
    path: &'static str,
}

const UPSTREAM_DEMOS: &[UpstreamDemo] = &[
    UpstreamDemo {
        chapter_id: 1,
        title: "Virtual Memory Walkthrough",
        description: "GitHub-backed demo source from HeapLens showing the allocator layout and address-space context.",
        hint: "Read the upstream demo and reuse its stage markers instead of writing a new harness.",
        path: "src/techniques/demos/heap_layout_walkthrough.c",
    },
    UpstreamDemo {
        chapter_id: 2,
        title: "malloc/free Fundamentals",
        description: "Real upstream demo source for allocator basics, used here in place of the local handwritten chapter challenge.",
        hint: "This corpus comes from the repository's tracked demo set, not the local scratch challenge engine.",
        path: "src/techniques/demos/malloc_free_fundamentals.c",
    },
    UpstreamDemo {
        chapter_id: 3,
        title: "Generic Allocator Demo",
        description: "Upstream reference source used as the mmap-era chapter example when no dedicated challenge file exists in the repo.",
        hint: "Prefer repository-backed examples even when they are generic.",
        path: "src/techniques/demos/generic_demo.c",
    },
    UpstreamDemo {
        chapter_id: 4,
        title: "Chunk Layout Walkthrough",
        description: "Tracked HeapLens source demonstrating chunk metadata and allocator stages.",
        hint: "Use the repo demo as the starting point for chunk-field reading instead of a synthetic worksheet.",
        path: "src/techniques/demos/malloc_free_fundamentals.c",
    },
    UpstreamDemo {
        chapter_id: 5,
        title: "Bin State Explorer",
        description: "Repository-backed example for arena/bin state inspection.",
        hint: "This file is part of the upstream demo corpus and maps well onto malloc_state exploration.",
        path: "src/techniques/demos/bin_state_explorer.c",
    },
    UpstreamDemo {
        chapter_id: 6,
        title: "Specific Bin Placement",
        description: "Real upstream example for chunk routing and allocator frontier behavior.",
        hint: "Use the tracked example rather than a handwritten top-chunk exercise.",
        path: "src/techniques/demos/forcing_specific_bin_placement.c",
    },
    UpstreamDemo {
        chapter_id: 7,
        title: "Fastbin Dup",
        description: "GitHub-backed fastbin example from the official HeapLens technique corpus.",
        hint: "The upstream fastbin demos are the correct source of truth for this chapter.",
        path: "src/techniques/demos/fastbin_dup.c",
    },
    UpstreamDemo {
        chapter_id: 8,
        title: "Tcache Poisoning",
        description: "Real tracked demo source for the tcache chapter.",
        hint: "This is fetched from the repository corpus and already includes stage markers used by the debugger view.",
        path: "src/techniques/demos/tcache_poisoning.c",
    },
    UpstreamDemo {
        chapter_id: 9,
        title: "Unsorted Bin Leak",
        description: "Official upstream unsorted-bin example used as chapter content.",
        hint: "Use the live repo example rather than a local scaffold.",
        path: "src/techniques/demos/unsorted_bin_leak.c",
    },
    UpstreamDemo {
        chapter_id: 10,
        title: "Smallbin Corruption",
        description: "Tracked repository example for the smallbin chapter.",
        hint: "The upstream technique corpus already covers this behavior.",
        path: "src/techniques/demos/smallbin_corruption.c",
    },
    UpstreamDemo {
        chapter_id: 11,
        title: "Largebin Attack Classic",
        description: "GitHub-backed largebin example used instead of local handwritten challenge text.",
        hint: "Stay aligned to the upstream technique file.",
        path: "src/techniques/demos/largebin_attack_classic.c",
    },
    UpstreamDemo {
        chapter_id: 12,
        title: "Classic Unlink Attack",
        description: "Repository source covering consolidation-era metadata corruption.",
        hint: "This tracked demo is a better fit than a synthetic coalescing puzzle.",
        path: "src/techniques/demos/classic_unlink_attack.c",
    },
    UpstreamDemo {
        chapter_id: 13,
        title: "Heap Layout Walkthrough",
        description: "Upstream source showing allocator path and heap layout movement.",
        hint: "Read the tracked source and stage labels to understand the malloc path.",
        path: "src/techniques/demos/heap_layout_walkthrough.c",
    },
    UpstreamDemo {
        chapter_id: 14,
        title: "malloc/free Fundamentals",
        description: "Repository-backed free-path reference used in place of the local scratch challenge.",
        hint: "The tracked upstream fundamentals demo is the authoritative source here.",
        path: "src/techniques/demos/malloc_free_fundamentals.c",
    },
    UpstreamDemo {
        chapter_id: 15,
        title: "Safe-Linking Bypass",
        description: "Tracked mitigation-era example from the HeapLens demo corpus.",
        hint: "Use the upstream mitigation demo rather than maintaining local-only challenge stubs.",
        path: "src/techniques/demos/double_free_safe_linking_bypass.c",
    },
];

pub fn load_challenge(chapter_id: i32) -> Option<ChallengeDescriptor> {
    let demo = UPSTREAM_DEMOS.iter().find(|item| item.chapter_id == chapter_id)?;
    let source = fs::read_to_string(Path::new(demo.path)).ok()?;

    Some(ChallengeDescriptor {
        title: demo.title.to_string(),
        description: format!("{}\nSource: {}", demo.description, demo.path),
        hint: demo.hint.to_string(),
        language: infer_language(demo.path).to_string(),
        solution: source,
    })
}

fn infer_language(path: &str) -> &'static str {
    match Path::new(path).extension().and_then(|item| item.to_str()) {
        Some("py") => "python",
        Some("sh") => "sh",
        _ => "c",
    }
}
