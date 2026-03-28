#ifndef HEAPLENS_CORE_MEMORY_READER_H
#define HEAPLENS_CORE_MEMORY_READER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
    uint64_t start;
    uint64_t end;
    char perms[5];
    uint64_t offset;
    char dev[16];
    unsigned long inode;
    char pathname[PATH_MAX];
} HeapLensMemoryMapEntry;

typedef struct {
    HeapLensMemoryMapEntry *entries;
    size_t count;
} HeapLensMemoryMapSnapshot;

typedef struct {
    pid_t pid;
    int mem_fd;
    char mem_path[64];
} HeapLensMemoryReader;

bool heaplens_memory_reader_open(HeapLensMemoryReader *reader, pid_t pid);
void heaplens_memory_reader_close(HeapLensMemoryReader *reader);
ssize_t heaplens_memory_reader_read(HeapLensMemoryReader *reader,
                                    uint64_t address,
                                    void *buffer,
                                    size_t size);
bool heaplens_memory_reader_read_fully(HeapLensMemoryReader *reader,
                                       uint64_t address,
                                       void *buffer,
                                       size_t size);

bool heaplens_memory_reader_read_maps(pid_t pid, HeapLensMemoryMapSnapshot *snapshot);
void heaplens_memory_map_snapshot_free(HeapLensMemoryMapSnapshot *snapshot);
const HeapLensMemoryMapEntry *heaplens_memory_map_find_named(const HeapLensMemoryMapSnapshot *snapshot,
                                                             const char *needle);
const HeapLensMemoryMapEntry *heaplens_memory_map_find_containing(const HeapLensMemoryMapSnapshot *snapshot,
                                                                  uint64_t address);

#endif
