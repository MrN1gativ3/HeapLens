#ifndef HEAPLENS_TECHNIQUES_TECHNIQUE_REGISTRY_H
#define HEAPLENS_TECHNIQUES_TECHNIQUE_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

typedef struct {
    const char *id;
    const char *category;
    const char *label;
} HeapLensTechniqueInfo;

size_t heaplens_technique_registry_count(void);
const HeapLensTechniqueInfo *heaplens_technique_registry_get(size_t index);
const HeapLensTechniqueInfo *heaplens_technique_registry_find_by_label(const char *label);
bool heaplens_technique_registry_resolve_binary(const HeapLensTechniqueInfo *info,
                                                const char *glibc_version,
                                                char *buffer,
                                                size_t buffer_size);
char *heaplens_technique_registry_build_theory(const HeapLensTechniqueInfo *info, const char *glibc_version);
char *heaplens_technique_registry_build_banner(const HeapLensTechniqueInfo *info,
                                               const char *glibc_version,
                                               const char *binary_path,
                                               pid_t pid);

#endif
