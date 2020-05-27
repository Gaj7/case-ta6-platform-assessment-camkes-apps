#include <camkes.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define LIST_LEN 12

// Dummy AM Gate which just prints changes in the id_list
int run(void) {
    static uint8_t cached_list[LIST_LEN];
    memset(id_list, 0, LIST_LEN);

    while (1) {
        int changed = 0;
        for (int i = 0; !changed && i < LIST_LEN; i++)
            changed |= cached_list[i] != ((uint8_t *)id_list)[i];

        if (changed) {
            memcpy((void *)cached_list, id_list, LIST_LEN);

            printf("New id_list: 0x");
            for (int i = 0; i < LIST_LEN; i++)
                printf("%02X", cached_list[i]);
            printf("\n");
        }
    }

    return 0;
}
