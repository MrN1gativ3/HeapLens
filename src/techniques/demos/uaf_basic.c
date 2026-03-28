#include "demo_common.h"

int main(void) {
    char *victim = heaplens_demo_make_chunk(0x40, 'U');
    heaplens_demo_stage("uaf_basic : allocated victim");
    free(victim);
    heaplens_demo_stage("uaf_basic : freed victim");
    victim[0] = 'X';
    heaplens_demo_stage("uaf_basic : wrote through dangling pointer");
    return 0;
}
