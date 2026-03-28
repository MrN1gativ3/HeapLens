#include "chapter_mitigations.h"

#include "chapter_common.h"

GtkWidget *heaplens_chapter_mitigations_new(void) {
    static const char *topics[] = {
        "Allocator checks such as fastbin head comparisons, tcache key validation, safe-linking, and unlink size checks all raise the difficulty floor for exploitation.",
        "System mitigations like ASLR, PIE, RELRO, NX, and canaries shape what an attacker must leak or overwrite after achieving a heap primitive.",
        "HeapLens exposes mitigation badges in the lab tab so learners can tie each demo to the defenses it assumes or bypasses."
    };

    return heaplens_learning_build_chapter_page(
        "Chapter 15 - Security Mitigations",
        "This chapter gathers the checks that modern heap exploitation must defeat. The goal is not just memorizing names, but understanding where each defense lives and what heap state causes it to fire.",
        topics,
        G_N_ELEMENTS(topics),
        "The exploitation tab uses green and red badges to keep the currently relevant mitigations visible while stepping a technique.",
        "safe-linking: stored = (slot >> 12) XOR ptr\ntcache key: chunk->key == tcache -> abort",
        NULL);
}
