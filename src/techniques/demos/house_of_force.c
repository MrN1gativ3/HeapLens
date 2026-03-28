#include "demo_common.h"

int main(void) {
    char *a = heaplens_demo_make_chunk(0x100, 'F');
    heaplens_demo_stage("house_of_force : baseline allocation");
    ((size_t *) (a - 0x8))[0] = (size_t) -1;
    heaplens_demo_stage("house_of_force : top-size style corruption");
    (void) heaplens_demo_make_chunk(0x2000, 'G');
    heaplens_demo_stage("house_of_force : oversized request issued");
    return 0;
}
