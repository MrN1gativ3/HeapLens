#include "symbol_resolver.h"

#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool heaplens_read_file(const char *path, uint8_t **buffer, size_t *size) {
    int fd = -1;
    struct stat st;
    uint8_t *contents = NULL;
    ssize_t total = 0;

    if (!path || !buffer || !size) {
        return false;
    }

    fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }

    if (fstat(fd, &st) != 0 || st.st_size <= 0) {
        close(fd);
        return false;
    }

    contents = malloc((size_t) st.st_size);
    if (!contents) {
        close(fd);
        return false;
    }

    while (total < st.st_size) {
        ssize_t chunk = read(fd, contents + total, (size_t) st.st_size - (size_t) total);
        if (chunk <= 0) {
            free(contents);
            close(fd);
            return false;
        }
        total += chunk;
    }

    close(fd);
    *buffer = contents;
    *size = (size_t) st.st_size;
    return true;
}

bool heaplens_symbol_find_module(pid_t pid,
                                 const char *needle,
                                 HeapLensResolvedSymbol *module_info) {
    HeapLensMemoryMapSnapshot maps;
    size_t index = 0;

    if (!needle || !module_info) {
        return false;
    }

    memset(module_info, 0, sizeof(*module_info));
    memset(&maps, 0, sizeof(maps));
    if (!heaplens_memory_reader_read_maps(pid, &maps)) {
        return false;
    }

    for (index = 0; index < maps.count; ++index) {
        const HeapLensMemoryMapEntry *entry = &maps.entries[index];
        if (strstr(entry->pathname, needle)) {
            snprintf(module_info->module_path, sizeof(module_info->module_path), "%s", entry->pathname);
            module_info->module_base = entry->start;
            heaplens_memory_map_snapshot_free(&maps);
            return true;
        }
    }

    heaplens_memory_map_snapshot_free(&maps);
    return false;
}

bool heaplens_symbol_resolve(const char *module_path,
                             uint64_t module_base,
                             const char *symbol_name,
                             HeapLensResolvedSymbol *out_symbol) {
    uint8_t *file_bytes = NULL;
    size_t file_size = 0;
    const Elf64_Ehdr *ehdr = NULL;
    const Elf64_Shdr *sections = NULL;
    const char *section_names = NULL;
    size_t section_index = 0;

    if (!module_path || !symbol_name || !out_symbol) {
        return false;
    }

    memset(out_symbol, 0, sizeof(*out_symbol));
    if (!heaplens_read_file(module_path, &file_bytes, &file_size)) {
        return false;
    }

    ehdr = (const Elf64_Ehdr *) file_bytes;
    if (file_size < sizeof(*ehdr) ||
        memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
        ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
        free(file_bytes);
        return false;
    }

    sections = (const Elf64_Shdr *) (file_bytes + ehdr->e_shoff);
    section_names = (const char *) (file_bytes + sections[ehdr->e_shstrndx].sh_offset);

    for (section_index = 0; section_index < ehdr->e_shnum; ++section_index) {
        const Elf64_Shdr *symbol_table = &sections[section_index];
        const char *section_name = section_names + symbol_table->sh_name;

        if (symbol_table->sh_type != SHT_DYNSYM && symbol_table->sh_type != SHT_SYMTAB) {
            continue;
        }

        if (strcmp(section_name, ".dynsym") != 0 && strcmp(section_name, ".symtab") != 0) {
            continue;
        }

        {
            const Elf64_Shdr *string_table = &sections[symbol_table->sh_link];
            const Elf64_Sym *symbols = (const Elf64_Sym *) (file_bytes + symbol_table->sh_offset);
            const char *strings = (const char *) (file_bytes + string_table->sh_offset);
            size_t symbol_count = symbol_table->sh_size / sizeof(Elf64_Sym);
            size_t symbol_index = 0;

            for (symbol_index = 0; symbol_index < symbol_count; ++symbol_index) {
                const Elf64_Sym *symbol = &symbols[symbol_index];
                const char *name = strings + symbol->st_name;
                if (strcmp(name, symbol_name) == 0 && symbol->st_value != 0) {
                    snprintf(out_symbol->name, sizeof(out_symbol->name), "%s", symbol_name);
                    snprintf(out_symbol->module_path, sizeof(out_symbol->module_path), "%s", module_path);
                    out_symbol->module_base = module_base;
                    out_symbol->offset = symbol->st_value;
                    out_symbol->address = module_base + symbol->st_value;
                    free(file_bytes);
                    return true;
                }
            }
        }
    }

    free(file_bytes);
    return false;
}

bool heaplens_symbol_resolve_address(const char *module_path,
                                     uint64_t module_base,
                                     uint64_t address,
                                     HeapLensResolvedSymbol *out_symbol) {
    uint8_t *file_bytes = NULL;
    size_t file_size = 0;
    const Elf64_Ehdr *ehdr = NULL;
    const Elf64_Shdr *sections = NULL;
    const char *section_names = NULL;
    size_t section_index = 0;
    uint64_t relative = 0;
    bool found = false;
    Elf64_Sym best_symbol;
    const char *best_name = NULL;

    if (!module_path || !out_symbol || address < module_base) {
        return false;
    }

    relative = address - module_base;
    memset(out_symbol, 0, sizeof(*out_symbol));
    memset(&best_symbol, 0, sizeof(best_symbol));

    if (!heaplens_read_file(module_path, &file_bytes, &file_size)) {
        return false;
    }

    ehdr = (const Elf64_Ehdr *) file_bytes;
    if (file_size < sizeof(*ehdr) ||
        memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
        ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
        free(file_bytes);
        return false;
    }

    sections = (const Elf64_Shdr *) (file_bytes + ehdr->e_shoff);
    section_names = (const char *) (file_bytes + sections[ehdr->e_shstrndx].sh_offset);

    for (section_index = 0; section_index < ehdr->e_shnum; ++section_index) {
        const Elf64_Shdr *symbol_table = &sections[section_index];
        const char *section_name = section_names + symbol_table->sh_name;

        if (symbol_table->sh_type != SHT_DYNSYM && symbol_table->sh_type != SHT_SYMTAB) {
            continue;
        }

        if (strcmp(section_name, ".dynsym") != 0 && strcmp(section_name, ".symtab") != 0) {
            continue;
        }

        {
            const Elf64_Shdr *string_table = &sections[symbol_table->sh_link];
            const Elf64_Sym *symbols = (const Elf64_Sym *) (file_bytes + symbol_table->sh_offset);
            const char *strings = (const char *) (file_bytes + string_table->sh_offset);
            size_t symbol_count = symbol_table->sh_size / sizeof(Elf64_Sym);
            size_t symbol_index = 0;

            for (symbol_index = 0; symbol_index < symbol_count; ++symbol_index) {
                const Elf64_Sym *symbol = &symbols[symbol_index];
                const char *name = strings + symbol->st_name;

                if (!name[0] || symbol->st_value == 0 || ELF64_ST_TYPE(symbol->st_info) == STT_SECTION) {
                    continue;
                }
                if (relative < symbol->st_value) {
                    continue;
                }
                if (!found || symbol->st_value > best_symbol.st_value) {
                    best_symbol = *symbol;
                    best_name = name;
                    found = true;
                }
            }
        }
    }

    if (found) {
        snprintf(out_symbol->name, sizeof(out_symbol->name), "%s", best_name);
        snprintf(out_symbol->module_path, sizeof(out_symbol->module_path), "%s", module_path);
        out_symbol->module_base = module_base;
        out_symbol->offset = relative - best_symbol.st_value;
        out_symbol->address = module_base + best_symbol.st_value;
    }

    free(file_bytes);
    return found;
}
