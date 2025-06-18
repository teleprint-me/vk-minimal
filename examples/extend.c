/// @file examples/extend.c
#include "vk/extension.h"

#include <stdio.h>

#define EXT_COUNT 1
const char* const EXT_NAMES[EXT_COUNT] = {"VK_EXT_debug_utils"};

int main(void) {
    VkcExtension* ext = vkc_extension_create(EXT_NAMES, EXT_COUNT, 1024);
    if (!ext) return 1;
    vkc_extension_log_info(ext);
    if (!vkc_extension_match_request(ext)) {
        printf("Required extension(s) missing\n");
    }
    vkc_extension_free(ext);
    return 0;
}
