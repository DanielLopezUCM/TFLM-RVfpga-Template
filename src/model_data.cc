// model_data.cc

#include "model_data.h"

alignas(16)
const unsigned char g_model[] = {
    0x1c, 0x00, 0x00, 0x00,
    // ...
};

const unsigned int g_model_len = sizeof(g_model);