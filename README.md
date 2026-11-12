# MNIST con TensorFlow Lite Micro en VeeR EH1

Este ejemplo ejecuta una inferencia de clasificación de dígitos MNIST con TensorFlow Lite Micro (TFLM) en un procesador VeeR EH1. El mismo programa se puede compilar y ejecutar en dos entornos:

1. **Simulación** en el testbench bare-metal de [Cores-VeeR-EH1](https://github.com/chipsalliance/Cores-VeeR-EH1), usando Verilator.
2. **Placa** VeeRwolf EH1 en una Nexys A7, usando PlatformIO y el framework `wd-riscv-sdk`. Usando el SoC[VeeRwolf](https://github.com/chipsalliance/VeeRwolf)

Ambos entornos comparten tanto el modelo y la imagen de entrada, como el código encargado de la inferencia. Se diferencian en el arranque, el mapeo de memoria y la interfaz que usan para mostrar informacion por consola. Esta guía explica cómo ejecutar cada entorno y que archivos seria necesario cambiar para adaptar el ejemplo.

## Contenido del directorio

| Archivo o directorio | Función |
|---|---|
| [`main.cc`](./main.cc) | Código principal encargado de ejecutar el modelo. Registra los operadores TFLM, inicializa el intérprete, carga la imagen, ejecuta la inferencia e imprime los scores. |
| [`mnist_model.h`](./mnist_model.h) | Modelo TFLite convertido a un array C/C++. |
| [`imagen.h`](./imagen.h) | Imagen MNIST de entrada representada como valores `int8_t`. |
| [`tflm/`](./tflm/) | Fuentes de TensorFlow Lite Micro y dependencias incluidas en el proyecto. Podrían generarse del repositorio original de [TFLM](https://github.com/tensorflow/tflite-micro). |
| [`run_sim.sh`](./run_sim.sh) | Script encargado de la simulacion. Construye el ELF y el testbench, ejecuta Verilator y comprueba el resultado de la simulación. |
| [`Makefile`](./Makefile) | Compilación bare-metal para la simulación. |
| [`platformio.ini`](./platformio.ini) | Configuración del entorno PlatformIO `swervolf_nexys`. |
| [`crt0.S`](./crt0.S), [`tflm.ld`](./tflm.ld) | Arranque y mapa de memoria propios de la simulación. |
| [`syscalls.c`](./syscalls.c), [`printf.c`](./printf.c) | Llamadas al sistema de la simulación, incluyendo la salida por terminal. |
| [`veerwolf_syscalls.cc`](./veerwolf_syscalls.cc) | Hooks newlib y salida UART para el destino VeeRwolf. |

## Requisitos

### Para simular

- Linux, Bash y GNU Make.
- Toolchain bare-metal RISC-V en `PATH`: `riscv64-unknown-elf-g++`, `riscv64-unknown-elf-gcc` y `riscv64-unknown-elf-objcopy`.
- Verilator y el compilador C++ del sistema (`g++`).
- El repositorio de Cores-VeeR-EH1, que debe incluir `tools/Makefile` y `configs/veer.config`.

Por defecto, `run_sim.sh` supone que este ejemplo está dentro del árbol VeeR y detecta su raíz tres niveles por encima del directorio del script. Si el árbol está en otra ubicación, indícalo con `RV_ROOT` (véase [Opciones de simulación](#opciones-de-simulación)). TFLM ya está incluido en `tflm/`; no hace falta descargarlo por separado para la configuración normal.

### Para compilar y ejecutar en Nexys A7

- PlatformIO Core (`pio`).
- Una Nexys A7 conectada al equipo por la interfaz JTAG/USB.
- El framework `wd-riscv-sdk` y la plataforma `chipsalliance`, que PlatformIO obtiene al preparar el entorno.
- Un terminal serie para observar la UART (el monitor de PlatformIO sirve).

La compilación se puede hacer sin la placa conectada. Programar la FPGA y cargar el firmware requieren la placa y la conexión JTAG.

## Reproducir la simulación

Desde este directorio, ejecuta:

```sh
./run_sim.sh
```

El script realiza el flujo completo:

1. Compila `mnist_riscv_test.elf` con el `Makefile` local y las fuentes de TFLM.
2. Convierte el ELF en `build/program.hex`.
3. Genera la configuración y compila el testbench de Verilator del árbol VeeR.
4. Ejecuta `Vtb_top`, guarda la salida y busca `TEST_PASSED`.

La simulación se considera correcta solo si el proceso termina sin error y aparece `TEST_PASSED`. La inferencia se ejecuta en software sobre VeeR. El script establece por defecto un límite de **20 000 000 ciclos**. Si se alcanza el límite antes de la terminación normal, aumenta `MAX_CYCLES` y vuelve a ejecutar.
> [!NOTA]
> El core VeeR-EH1 no cuenta con unidad FPU por lo que si el modelo a ejecutar contiene operaciones flotantes puede tener un tiempo muy largo de ejecución. El modelo actual que es muy libiano tiene tarda algo menos de 19 millones de ciclos, por lo que posiblemente sea necesario aumentar el número de ciclos de ejecución.

Los archivos de simulación se guardan en `build/`: imagen de programa, `.hex` de Verilator, log de salida y trazas generadas por el testbench. El ELF `mnist_riscv_test.elf` queda en este directorio. Estos resultados locales se ignoran en Git.

### Opciones de simulación

Las variables de entorno permiten adaptar rutas y límites sin editar el script:

```sh
MAX_CYCLES=200000000            # Número máximo de ciclos de la simulación
BUILD_DIR=/tmp/tflm-build       # Dirección dónde iran los archivos de compilación
VEER_TARGET=default             # Target del core, mejor dejar a default
RV_ROOT=/ruta/a/Cores-VeeR-EH1  # Ruta del repositorio Cores-VeeR-EH1
TFLM_DIR=/ruta/a/tflm           # Ruta al árbol de archivos de TFLM generados del repositorio original
VERILATOR=verilator             # Simulador a usar, mejor dejar verilator
```

`MAX_CYCLES` debe ser un entero positivo. Si no se define, el script establece 20 000 000 ciclos.

Los argumentos después de `--` se envían directamente al ejecutable del testbench:

```sh
./run_sim.sh -- +dumpon
```

Para eliminar los archivos habituales del build de simulación:

```sh
make clean
rm -rf build
```

## Reproducir en Nexys A7

Desde este directorio, compila el firmware:

```sh
pio run -e swervolf_nexys
```

Con la placa conectada, programa el bitstream VeeRwolf en la FPGA, carga y ejecuta el firmware, y abre el monitor UART:

```sh
pio run -e swervolf_nexys --target program_fpga
pio run -e swervolf_nexys --target upload
pio device monitor -b 115200
```

`program_fpga` usa el bitstream `swervolf_eh1.bit` proporcionado por el SDK. `upload` carga el firmware en la RAM del SoC (**no** programa memoria flash no volátil). La aplicación inicializa la UART y muestra por ella los pesos de predicción y el dígito con mayor peso.

## Cómo adaptar el ejemplo

### Cambiar la imagen de entrada

Edita [`imagen.h`](./imagen.h). La imagen actual contiene 784 valores `int8_t` (28 × 28) cuantizados. Los datos que se copien al tensor deben respetar el tipo, la forma y el preprocesamiento que espera el modelo. No basta con que la entrada tenga 784 elementos: un modelo `float32` puede requerir normalización distinta a la representación cuantizada que usa el ejemplo actual. Al cambiar los datos de entrada también será necesario cambiar la zona encargada de administar los datos al modelo en el código `main.cc`.

### Sustituir el modelo

Reemplaza el array `mnist_tflite` de [`mnist_model.h`](./mnist_model.h) por los bytes del nuevo modelo convertido a un array C/C++. Después, revisa en [`main.cc`](./main.cc):

- Que el tipo y el tamaño del tensor de entrada coincidan con el modelo y con el preprocesamiento de los datos.
- Que el tensor de salida tenga la forma esperada.
- Que `MicroMutableOpResolver` registre todos los operadores usados por el modelo. Actualiza tanto la capacidad de la plantilla como las llamadas `Add...()`; un operador ausente impide preparar o ejecutar el modelo.
- Que la arena TFLM alcance para el nuevo modelo. El tamaño actual es 256 KiB.

Un modelo distinto puede necesitar otros kernels, mayor arena o una forma distinta de interpretar resultados. Si cambia la estructura de los datos de entrada o salida, adapta también la carga y la impresión en `main.cc`.

### Cambiar plataforma, salida o memoria

| Necesidad | Archivos que revisar |
|---|---|
| Cambiar placa, framework, flags o fuentes compiladas en PlatformIO | [`platformio.ini`](./platformio.ini) |
| Cambiar operadores, entrada, interpretación de scores o tamaño de arena | [`main.cc`](./main.cc) |
| Cambiar consola de la Nexys o inicialización UART | [`main.cc`](./main.cc), [`veerwolf_syscalls.cc`](./veerwolf_syscalls.cc) |
| Cambiar compilador, flags o fuentes del destino de simulación | [`Makefile`](./Makefile) |
| Cambiar límite de ciclos, rutas o argumentos de simulación | [`run_sim.sh`](./run_sim.sh) |
| Cambiar arranque, direcciones de memoria, pila o secciones de la simulación | [`crt0.S`](./crt0.S), [`tflm.ld`](./tflm.ld) |
| Cambiar salida por mailbox de la simulación | [`syscalls.c`](./syscalls.c), [`printf.c`](./printf.c) |

No mezcles las piezas de arranque y consola de los dos destinos. La simulación usa `crt0.S`, `tflm.ld`, `syscalls.c` y `printf.c`; el firmware de Nexys usa el startup, el linker, el BSP y la carga JTAG que aporta `wd-riscv-sdk`. Asimismo, la simulación compila para `rv32imc` y el entorno del SDK configura `rv32imac`.

## Diagnóstico rápido

- **`RV_ROOT is not a VeeR repository`**: apunta `RV_ROOT` al directorio que contiene `tools/Makefile` y `configs/veer.config`.
- **Falta una herramienta**: instala el compilador/herramienta indicada y verifica que está en `PATH`.
- **No aparece `TEST_PASSED`**: revisa `build/simulation.log`; si la ejecución llegó al límite de ciclos, sube `MAX_CYCLES`. Un log sin esa marca no demuestra por sí solo que haya fallado la compilación.
- **Falla `AllocateTensors` o `Invoke`**: comprueba compatibilidad del modelo, operadores registrados, tipo y dimensiones de entrada, y tamaño de arena (Para esto es muy util [Netron](https://netron.app/) para poder visualizar los modelos en formato .`tflite`).
- **No se ve salida en Nexys**: confirma que se programó el bitstream, que se cargó el firmware y que el monitor serie está a 115200.
