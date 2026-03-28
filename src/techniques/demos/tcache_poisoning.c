#include "demo_common.h"

int main(void) {
    char *a = heaplens_demo_make_chunk(0x40, 'A');
    char *b = heaplens_demo_make_chunk(0x40, 'B');
    heaplens_demo_stage("tcache_poisoning : allocated pair");
    free(a);
    free(b);
    heaplens_demo_stage("tcache_poisoning : freed into tcache");
    memset(b, 0x41, 0x10);
    heaplens_demo_stage("tcache_poisoning : overwritten freed metadata");
    (void) heaplens_demo_make_chunk(0x40, 'C');
    heaplens_demo_stage("tcache_poisoning : first reclamation");
    return 0;
}
