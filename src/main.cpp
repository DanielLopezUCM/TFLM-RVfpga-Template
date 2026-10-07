#include <new>
#include <cstdint>

#include "model_data.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/schema/schema_generated.h"

#if defined(D_NEXYS_A7)
    extern "C" {
        #include <bsp_printf.h>
        #include <bsp_mem_map.h>
        #include <bsp_version.h>
    }
#else
    PRE_COMPILED_MSG("no platform was defined")
#endif

extern "C" {
    #include <psp_api.h>
}

namespace {

constexpr size_t kTensorArenaSize = 8 * 1024;

alignas(tflite::MicroInterpreter)
static uint8_t interpreter_storage[sizeof(tflite::MicroInterpreter)];

static tflite::MicroInterpreter* interpreter = nullptr;

alignas(16)
uint8_t tensor_arena[kTensorArenaSize];

const tflite::Model* model = nullptr;
// tflite::MicroInterpreter* interpreter = nullptr;

TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

tflite::MicroMutableOpResolver<3> resolver;

}  // namespace

bool ai_init()
{
    model = tflite::GetModel(g_model);

    if (model == nullptr) {
        MicroPrintf("ERROR: model == nullptr");
        return false;
    }

    if (model->version() != TFLITE_SCHEMA_VERSION) {
        MicroPrintf("ERROR: schema version mismatch");
        return false;
    }

    if (resolver.AddFullyConnected() != kTfLiteOk) {
        return false;
    }

    if (resolver.AddRelu() != kTfLiteOk) {
        return false;
    }

    if (resolver.AddSoftmax() != kTfLiteOk) {
        return false;
    }

    interpreter =
        new (interpreter_storage)
        tflite::MicroInterpreter(
            model,
            resolver,
            tensor_arena,
            kTensorArenaSize
        );

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        printfNexys("ERROR: AllocateTensors()\n");
        MicroPrintf("ERROR: AllocateTensors()");
        return false;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    return input != nullptr && output != nullptr;
}

// bool ai_init()
// {
//     model = tflite::GetModel(g_model);

//     if (model == nullptr) {
//         MicroPrintf("ERROR: model == nullptr");
//         return false;
//     }

//     if (model->version() != TFLITE_SCHEMA_VERSION) {
//         MicroPrintf("ERROR: schema version mismatch");
//         return false;
//     }

//     if (resolver.AddFullyConnected() != kTfLiteOk) {
//         return false;
//     }

//     if (resolver.AddRelu() != kTfLiteOk) {
//         return false;
//     }

//     if (resolver.AddSoftmax() != kTfLiteOk) {
//         return false;
//     }

//     static tflite::MicroInterpreter static_interpreter(
//         model,
//         resolver,
//         tensor_arena,
//         kTensorArenaSize
//     );

//     interpreter = &static_interpreter;

//     if (interpreter->AllocateTensors() != kTfLiteOk) {
//         MicroPrintf("ERROR: AllocateTensors()");
//         return false;
//     }

//     input = interpreter->input(0);
//     output = interpreter->output(0);

//     if (input == nullptr || output == nullptr) {
//         return false;
//     }

//     MicroPrintf(
//         "Arena used: %u bytes",
//         static_cast<unsigned>(interpreter->arena_used_bytes())
//     );

//     return true;
// }

int main ()
{
    
    uartInit();
    printfNexys("Iniciando IA\n");

    ai_init();
    int i;
    while(1){
        printfNexys("IA iniciada\n");
        for(i=0; i <10000000; i++);

    }
    
    return 0;
}