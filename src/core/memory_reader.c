#include "memory_reader.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool heaplens_memory_reader_open(HeapLensMemoryReader *reader, pid_t pid) {
    if (!reader) {
        return false;
    }

    memset(reader, 0, sizeof(*reader));
    reader->pid = pid;
    snprintf(reader->mem_path, sizeof(reader->mem_path), "/proc/%d/mem", pid);
    reader->mem_fd = open(reader->mem_path, O_RDONLY | O_CLOEXEC);
    return reader->mem_fd >= 0;
}

void heaplens_memory_reader_close(HeapLensMemoryReader *reader) {
    if (!reader) {
        return;
    }

    if (reader->mem_fd >= 0) {
        close(reader->mem_fd);
        reader->mem_fd = -1;
    }
}

ssize_t heaplens_memory_reader_read(HeapLensMemoryReader *reader,
                                    uint64_t address,
                                    void *buffer,
                                    size_t size) {
    if (!reader || reader->mem_fd < 0 || !buffer || size == 0) {
        errno = EINVAL;
        return -1;
    }

    /* pread64() avoids mutating a shared file offset while multiple panels inspect the target. */
    return pread64(reader->mem_fd, buffer, size, (off64_t) address);
}

bool heaplens_memory_reader_read_fully(HeapLensMemoryReader *reader,
                                       uint64_t address,
                                       void *buffer,
                                       size_t size) {
    size_t completed = 0;

    while (completed < size) {
        ssize_t chunk = heaplens_memory_reader_read(reader,
                                                    address + completed,
                                                    (char *) buffer + completed,
                                                    size - completed);
        if (chunk <= 0) {
            return false;
        }

        completed += (size_t) chunk;
    }

    return true;
}

bool heaplens_memory_reader_read_maps(pid_t pid, HeapLensMemoryMapSnapshot *snapshot) {
    char maps_path[64];
    char line[PATH_MAX + 128];
    FILE *maps_file = NULL;
    size_t capacity = 0;

    if (!snapshot) {
        return false;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    maps_file = fopen(maps_path, "r");
    if (!maps_file) {
        return false;
    }

    while (fgets(line, sizeof(line), maps_file)) {
        HeapLensMemoryMapEntry entry;
        unsigned long long start = 0;
        unsigned long long end = 0;
        unsigned long long offset = 0;
        char perms[5] = {0};
        char dev[16] = {0};
        char path[PATH_MAX] = {0};
        int scanned = 0;

        memset(&entry, 0, sizeof(entry));
        scanned = sscanf(line,
                         "%llx-%llx %4s %llx %15s %lu %4095[^\n]",
                         &start,
                         &end,
                         perms,
                         &offset,
                         dev,
                         &entry.inode,
                         path);
        if (scanned < 6) {
            continue;
        }

        entry.start = start;
        entry.end = end;
        entry.offset = offset;
        snprintf(entry.perms, sizeof(entry.perms), "%s", perms);
        snprintf(entry.dev, sizeof(entry.dev), "%s", dev);
        if (scanned == 7) {
            const char *trimmed = path[0] == ' ' ? path + 1 : path;
            snprintf(entry.pathname, sizeof(entry.pathname), "%s", trimmed);
        }

        if (snapshot->count == capacity) {
            HeapLensMemoryMapEntry *grown = NULL;
            capacity = capacity == 0 ? 16 : capacity * 2;
            grown = realloc(snapshot->entries, capacity * sizeof(*snapshot->entries));
            if (!grown) {
                heaplens_memory_map_snapshot_free(snapshot);
                fclose(maps_file);
                return false;
            }
            snapshot->entries = grown;
        }

        snapshot->entries[snapshot->count++] = entry;
    }

    fclose(maps_file);
    return true;
}

void heaplens_memory_map_snapshot_free(HeapLensMemoryMapSnapshot *snapshot) {
    if (!snapshot) {
        return;
    }

    free(snapshot->entries);
    snapshot->entries = NULL;
    snapshot->count = 0;
}

const HeapLensMemoryMapEntry *heaplens_memory_map_find_named(const HeapLensMemoryMapSnapshot *snapshot,
                                                             const char *needle) {
    size_t index = 0;

    if (!snapshot || !needle) {
        return NULL;
    }

    for (index = 0; index < snapshot->count; ++index) {
        if (strstr(snapshot->entries[index].pathname, needle)) {
            return &snapshot->entries[index];
        }
    }

    return NULL;
}

const HeapLensMemoryMapEntry *heaplens_memory_map_find_containing(const HeapLensMemoryMapSnapshot *snapshot,
                                                                  uint64_t address) {
    size_t index = 0;

    if (!snapshot) {
        return NULL;
    }

    for (index = 0; index < snapshot->count; ++index) {
        const HeapLensMemoryMapEntry *entry = &snapshot->entries[index];
        if (address >= entry->start && address < entry->end) {
            return entry;
        }
    }

    return NULL;
}
