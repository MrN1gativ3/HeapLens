#include "demo_common.h"

int main(void) {
    char fake_region[0x80];
    memset(fake_region, 0, sizeof(fake_region));
    heaplens_demo_stage("house_of_spirit : fake region prepared");
    *((size_t *) (fake_region + 8)) = 0x41;
    heaplens_demo_stage("house_of_spirit : fake chunk header seeded");
    free(fake_region + 0x10);
    heaplens_demo_stage("house_of_spirit : fake chunk freed");
    return 0;
}
