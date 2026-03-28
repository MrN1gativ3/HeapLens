#include "demo_common.h"

int main(void) {
    char *a = heaplens_demo_make_chunk(0x40, 'D');
    heaplens_demo_stage("tcache_double_free_key_bypass : allocated");
    free(a);
    heaplens_demo_stage("tcache_double_free_key_bypass : first free");
    memset(a, 0, 0x10);
    heaplens_demo_stage("tcache_double_free_key_bypass : scribbled freed entry");
    free(a);
    heaplens_demo_stage("tcache_double_free_key_bypass : second free");
    return 0;
}
