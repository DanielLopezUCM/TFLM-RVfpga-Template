# RUTA AL ÁRBOL DE FUENTES GENERADO
TFLM_DIR ?= $(CURDIR)/tflm

# Configuración del compilador de Linux
CXX = riscv64-unknown-elf-g++
CC = riscv64-unknown-elf-gcc
CXXFLAGS = -O3 -std=c++17 -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -fno-threadsafe-statics -march=rv32imc_zicsr_zifencei -mabi=ilp32
PLATFORM_CFLAGS = -O3 -ffunction-sections -fdata-sections -march=rv32imc_zicsr_zifencei -mabi=ilp32

LDFLAGS = -nostartfiles -Wl,--gc-sections

DEFINES = -D TF_LITE_STATIC_MEMORY -D TF_LITE_USE_GLOBAL_MAX -D TF_LITE_USE_GLOBAL_MIN -D TF_LITE_USE_GLOBAL_CMATH_FUNCTIONS

# Directorios de inclusión (Headers)
INCLUDES = -I$(TFLM_DIR) -I$(TFLM_DIR)/signal -I$(TFLM_DIR)/third_party/flatbuffers/include -I$(TFLM_DIR)/third_party/kissfft -I. # -I$(TFLM_DIR)/third_party/gemmlowp

LINKER = -T tflm.ld

# Buscamos automáticamente TODOS los archivos .cc que requiere TFLite Micro para funcionar
TFLM_SRCS = $(shell find $(TFLM_DIR)/tensorflow -name "*.cc" ! -name "*_test.cc") \
            $(shell find $(TFLM_DIR)/third_party -name "*.cc")

# Nuestro archivo principal
MAIN_SRC = crt0.S main.cc
PLATFORM_C_OBJS = syscalls.o printf.o

# Nombre del binario de salida
TARGET = mnist_riscv_test.elf

all: $(TARGET)

$(TARGET): $(MAIN_SRC) $(PLATFORM_C_OBJS) $(TFLM_SRCS) mnist_model.h imagen.h tflm.ld
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(LINKER) $(LDFLAGS) $(MAIN_SRC) $(PLATFORM_C_OBJS) $(TFLM_SRCS) -o $@

%.o: %.c
	$(CC) $(PLATFORM_CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(TARGET) $(PLATFORM_C_OBJS)
