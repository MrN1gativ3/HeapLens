#include "demo_common.h"

int main(void) {
    char *a = heaplens_demo_make_chunk(0x30, 'A');
    char *b = heaplens_demo_make_chunk(0x50, 'B');
    heaplens_demo_stage("malloc/free fundamentals : allocations ready");
    free(a);
    heaplens_demo_stage("malloc/free fundamentals : freed a");
    a = heaplens_demo_make_chunk(0x30, 'C');
    heaplens_demo_stage("malloc/free fundamentals : reallocated a");
    free(a);
    free(b);
    heaplens_demo_stage("malloc/free fundamentals : complete");
    return 0;
}
