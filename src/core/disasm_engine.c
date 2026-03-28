#include "disasm_engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool heaplens_disasm_engine_init(HeapLensDisasmEngine *engine) {
    if (!engine) {
        return false;
    }

    memset(engine, 0, sizeof(*engine));
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &engine->handle) != CS_ERR_OK) {
        return false;
    }

    cs_option(engine->handle, CS_OPT_DETAIL, CS_OPT_OFF);
    engine->ready = true;
    return true;
}

void heaplens_disasm_engine_shutdown(HeapLensDisasmEngine *engine) {
    if (!engine || !engine->ready) {
        return;
    }

    cs_close(&engine->handle);
    memset(engine, 0, sizeof(*engine));
}

char *heaplens_disasm_buffer(HeapLensDisasmEngine *engine,
                             uint64_t address,
                             const uint8_t *bytes,
                             size_t size) {
    cs_insn *instructions = NULL;
    size_t count = 0;
    char *output = NULL;
    size_t output_size = 0;
    FILE *stream = NULL;
    size_t index = 0;

    if (!engine || !engine->ready || !bytes || size == 0) {
        return NULL;
    }

    stream = open_memstream(&output, &output_size);
    if (!stream) {
        return NULL;
    }

    count = cs_disasm(engine->handle, bytes, size, address, 0, &instructions);
    if (count == 0) {
        fprintf(stream, "0x%llx: <no instruction decode>\n", (unsigned long long) address);
        fclose(stream);
        return output;
    }

    for (index = 0; index < count; ++index) {
        fprintf(stream,
                "0x%llx: %-8s %s\n",
                (unsigned long long) instructions[index].address,
                instructions[index].mnemonic,
                instructions[index].op_str);
    }

    cs_free(instructions, count);
    fclose(stream);
    return output;
}

char *heaplens_disasm_process_region(HeapLensDisasmEngine *engine,
                                     HeapLensMemoryReader *reader,
                                     uint64_t address,
                                     size_t size) {
    uint8_t *buffer = NULL;
    char *output = NULL;

    if (!reader || size == 0) {
        return NULL;
    }

    buffer = calloc(1, size);
    if (!buffer) {
        return NULL;
    }

    if (!heaplens_memory_reader_read_fully(reader, address, buffer, size)) {
        free(buffer);
        return NULL;
    }

    output = heaplens_disasm_buffer(engine, address, buffer, size);
    free(buffer);
    return output;
}
