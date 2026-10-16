#if defined(MNIST_VEERWOLF)
extern "C" {
#include <bsp_printf.h>
}
#define MNIST_PRINTF printfNexys
#else
#include <stdio.h>
#define MNIST_PRINTF printf
#endif

// Includes esenciales de TensorFlow Lite Micro
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/schema/schema_generated.h"

// Incluimos el modelo que convertimos con xxd
#include "mnist_model.h"
#include "imagen.h"

// Definimos el tamaño del espacio de memoria (Arena) para las variables del modelo
// Para un modelo MNIST estándar, 64KB suele ser más que suficiente.
const int kTensorArenaSize = 256 * 1024;
#if defined(MNIST_VEERWOLF)
uint8_t tensor_arena[kTensorArenaSize]
    __attribute__((section(".bss"), aligned(16)));
#else
uint8_t tensor_arena[kTensorArenaSize]
    __attribute__((section(".tflm_arena"), aligned(16)));
#endif

int main() {
#if defined(MNIST_VEERWOLF)
    uartInit();
#endif
    MNIST_PRINTF("--- Inicializando TensorFlow Lite Micro (MNIST) ---\n");

    // 1. Cargar el modelo desde el array generado por xxd
    const tflite::Model* model = tflite::GetModel(mnist_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        MNIST_PRINTF("Error: La version del esquema del modelo no coincide!\n");
        return 1;
    }

    // 2. Registrar los operadores necesarios (OpResolver)
    // Para MNIST típicamente necesitas Conv2D, FullyConnected, MaxPool2D, Reshape, Softmax.
    // Usamos MicroMutableOpResolver para registrar solo los que usa tu modelo y ahorrar memoria.
    tflite::MicroMutableOpResolver<7> op_resolver;
    op_resolver.AddConv2D();
    op_resolver.AddMaxPool2D();
    op_resolver.AddShape();
    op_resolver.AddStridedSlice();
    op_resolver.AddPack();
    op_resolver.AddReshape();
    op_resolver.AddFullyConnected();

    // 3. Construir el intérprete
    tflite::MicroInterpreter interpreter(
        model, op_resolver, tensor_arena, kTensorArenaSize);

    // Asignar los tensores de la estructura del modelo al Tensor Arena
    if (interpreter.AllocateTensors() != kTfLiteOk) {
        MNIST_PRINTF("Error al asignar los tensores en el Arena.\n");
        return 1;
    }

    // 4. Obtener punteros a los tensores de Entrada y Salida
    TfLiteTensor* input = interpreter.input(0);
    TfLiteTensor* output = interpreter.output(0);

    // 5. Preparar datos de entrada (Simulación de una imagen de MNIST)
    // MNIST usa imágenes de 28x28 píxeles = 784 valores.
    // NOTA: Ajusta el tipo de datos (float o int8_t) según si tu modelo está cuantizado o no.
    if (input == nullptr || output == nullptr) {
        MNIST_PRINTF("Error: no se pudieron obtener los tensores del modelo.\n");
        return 1;
    }
    if (input->type == kTfLiteFloat32 &&
        input->bytes == 784 * sizeof(float)) {
        float* input_buffer = input->data.f;
        // Llenamos con un patrón dummy o ceros (normalizado entre 0.0 y 1.0)
        for (int i = 0; i < 784; ++i) {
            input_buffer[i] = imagen[i]; 
        }
    } else if (input->type == kTfLiteInt8 && input->bytes == 784) {
        int8_t* input_buffer = input->data.int8;
        // Si está cuantizado, el rango suele ser de -128 a 127
        for (int i = 0; i < 784; ++i) {
            input_buffer[i] = imagen[i]; 
        }
    } else {
        MNIST_PRINTF("Error: tipo o tamano de tensor de entrada no soportado.\n");
        return 1;
    }

    // 6. Ejecutar la inferencia
    MNIST_PRINTF("Ejecutando inferencia...\n");
    if (interpreter.Invoke() != kTfLiteOk) {
        MNIST_PRINTF("Error al ejecutar Invoke().\n");
        return 1;
    }

    // 7. Leer los resultados (Predicción de los dígitos 0-9)
    MNIST_PRINTF("--- Resultados de la prediccion ---\n");
    int max_index = 0;
    float max_value = -1.0f;

    if (output->type == kTfLiteFloat32) {
        if (output->bytes != 10 * sizeof(float)) {
            MNIST_PRINTF("Error: tamano de tensor de salida no soportado.\n");
            return 1;
        }
        for (int i = 0; i < 10; ++i) {
#if defined(MNIST_VEERWOLF)
            int value = static_cast<int>(output->data.f[i] * 10000.0f);
            MNIST_PRINTF("Digito %d: score x10000 = %d", i, value);
#else
            int value = static_cast<int>(output->data.f[i] * 10000.0f);
            printf("Digito %d: %d.%04d\n", i, value / 10000, value % 10000);
#endif
            if (output->data.f[i] > max_value) {
                max_value = output->data.f[i];
                max_index = i;
            }
        }
    } else if (output->type == kTfLiteInt8) {
        if (output->bytes != 10) {
            MNIST_PRINTF("Error: tamano de tensor de salida no soportado.\n");
            return 1;
        }
        // Para modelos cuantizados int8, puedes des-cuantizar para ver probabilidades (opcional)
        float scale = output->params.scale;
        int32_t zero_point = output->params.zero_point;
        
        for (int i = 0; i < 10; ++i) {
            float val = (output->data.int8[i] - zero_point) * scale;
#if defined(MNIST_VEERWOLF)
            int value = static_cast<int>(val * 10000.0f);
            MNIST_PRINTF("Digito %d: score x10000 = %d (int8: %d)", i, value,
                         static_cast<int>(output->data.int8[i]));
#else
            int value = static_cast<int>(val * 10000.0f);
            printf("Digito %d: %d.%04d (int8: %d)\n", i, value / 10000,
                   value % 10000, static_cast<int>(output->data.int8[i]));
#endif
            if (val > max_value) {
                max_value = val;
                max_index = i;
            }
        }
    } else {
        MNIST_PRINTF("Error: tipo de tensor de salida no soportado.\n");
        return 1;
    }

    MNIST_PRINTF("\nEl modelo predice que el numero es un: %d\n", max_index);

    return 0;
}
