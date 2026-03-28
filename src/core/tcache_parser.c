#include "tcache_parser.h"

#include <string.h>

bool heaplens_tcache_parser_probe(HeapLensMemoryReader *reader, uint64_t address) {
    struct tcache_perthread_struct candidate;
    size_t index = 0;

    if (!reader) {
        return false;
    }

    if (!heaplens_memory_reader_read_fully(reader, address, &candidate, sizeof(candidate))) {
        return false;
    }

    for (index = 0; index < TCACHE_MAX_BINS; ++index) {
        /*
         * A default bin never exceeds seven entries, but allowing some slack avoids rejecting tweaked
         * experiments while still stopping us from misclassifying arbitrary heap bytes as tcache state.
         */
        if (candidate.counts[index] > 32) {
            return false;
        }
    }

    return true;
}

bool heaplens_tcache_parser_read(HeapLensMemoryReader *reader,
                                 uint64_t address,
                                 HeapLensTcacheSnapshot *snapshot) {
    size_t bin_index = 0;

    if (!reader || !snapshot) {
        return false;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->address = address;
    if (!heaplens_memory_reader_read_fully(reader, address, &snapshot->raw, sizeof(snapshot->raw))) {
        return false;
    }

    for (bin_index = 0; bin_index < TCACHE_MAX_BINS; ++bin_index) {
        HeapLensTcacheBinSnapshot *bin = &snapshot->bins[bin_index];
        uint64_t current = (uint64_t) (uintptr_t) snapshot->raw.entries[bin_index];

        bin->count = snapshot->raw.counts[bin_index];
        bin->head = current;

        while (current != 0 && bin->depth < HEAPLENS_TCACHE_ENTRY_DEPTH) {
            HeapLensTcacheEntryInfo *entry = &bin->entries[bin->depth];
            struct tcache_entry raw_entry;

            if (!heaplens_memory_reader_read_fully(reader, current, &raw_entry, sizeof(raw_entry))) {
                break;
            }

            entry->address = current;
            entry->raw_next = (uintptr_t) raw_entry.next;
            entry->decoded_next = heaplens_safe_link_reveal(current, entry->raw_next);
            entry->key = raw_entry.key;

            ++bin->depth;
            current = entry->decoded_next;
        }
    }

    return true;
}
