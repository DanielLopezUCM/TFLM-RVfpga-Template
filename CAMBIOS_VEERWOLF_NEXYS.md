# Cambios del portado a VeeRwolf y Nexys A7

Este documento describe los cambios realizados para ejecutar la inferencia
MNIST con TensorFlow Lite Micro (TFLM) en VeeRwolf EH1 y la placa Nexys A7.
El proyecto conserva además su modo original para el testbench de
Cores-VeeR-EH1 con Verilator.

## 1. Los dos destinos del firmware

El directorio contiene ahora dos formas independientes de compilar el mismo
programa de inferencia:

| Destino | Compilación | Arranque y memoria | Salida |
|---|---|---|---|
| Testbench Cores-VeeR-EH1 | `Makefile` y `run_sim.sh` | Arranque y linker propios: `crt0.S` y `tflm.ld` | Mailbox del testbench mediante `printf.c` |
| VeeRwolf EH1 / Nexys A7 | PlatformIO, entorno `swervolf_nexys` | SDK `wd-riscv-sdk`: arranque, linker y mapa de memoria de placa | UART de Nexys A7 mediante el BSP del SDK |

La separación evita reutilizar en la FPGA componentes que son específicos del
testbench, como su dirección de mailbox, su configuración de pila o su señal
de terminación `tohost`.

## 2. Configuración de PlatformIO: `platformio.ini`

Se añadió [platformio.ini](./platformio.ini) para describir el destino de
placa:

- La sección `[platformio]` configura `src_dir = .`, porque el proyecto ya
  tiene sus fuentes en la raíz de este directorio y no en el `src/` que
  PlatformIO usa por defecto.
- `[env:swervolf_nexys]` selecciona la plataforma `chipsalliance`, la placa
  `swervolf_nexys` y el framework bare-metal `wd-riscv-sdk`.
- `build_src_filter` limita el firmware a `main.cc`, los hooks de sistema
  específicos de VeeRwolf y las fuentes C++ de TFLM bajo `tflm/tensorflow` y
  `tflm/third_party`. Excluye los archivos de pruebas (`*_test.cc`) para no
  compilar pruebas que no forman parte de la aplicación.
- Las opciones de compilación conservan las necesarias para TFLM bare-metal:
  C++17, optimización, sin RTTI, excepciones ni inicialización thread-safe de
  estáticos. Las definiciones `TF_LITE_*` habilitan el modo de memoria estática
  y las funciones matemáticas comunes de la copia de TFLM incluida.
- Las rutas `-I` hacen visibles los headers de TFLM, FlatBuffers y KissFFT.
- `-lm` enlaza la biblioteca matemática; las fuentes TFLM utilizan funciones
  como `round` y `ceilf`.
- `board_build.bitstream_file` selecciona el bitstream VeeRwolf EH1 para
  Nexys A7 que viene con el paquete del SDK. La ruta usa
  `${platformio.packages_dir}` en lugar de una ruta absoluta ligada a otra
  computadora.

El toolchain y el mapa de memoria concretos los proporciona el entorno de
placa del SDK. En esta configuración se compila para `rv32imac`; no se usa la
configuración `rv32imc` específica del firmware del testbench.

## 3. Aplicación de inferencia: `main.cc`

El programa sigue usando el modelo `mnist_tflite`, la imagen de entrada
`imagen` y el intérprete TFLM. La modificación permite compartir el código de
inferencia sin forzar el mismo hardware de salida o el mismo emplazamiento de
memoria en ambos destinos.

### Selección de consola

Cuando se define `MNIST_VEERWOLF`, el archivo incluye `bsp_printf.h` con
enlace C y usa `printfNexys`. En el build del testbench no se define esa macro:
se incluye `<stdio.h>` y se conserva `printf`.

El `extern "C"` alrededor del header evita que C++ cambie los nombres de
`uartInit` y `printfNexys` al compilar, ya que las funciones de BSP están
implementadas en C.

### Inicialización UART

`uartInit()` se invoca solo en el destino VeeRwolf y antes del primer mensaje.
El BSP configura la UART integrada del SoC para la consola serie de la placa.
La salida se puede observar con un terminal serie a 115200 baudios.

El `printfNexys` del SDK no soporta imprimir valores de punto flotante. Por
ello, al compilar para VeeRwolf los diez scores se muestran como enteros
escalados por 10000. El build original de simulación conserva su salida decimal
con cuatro posiciones fraccionarias.

### Arena y memoria

La arena TFLM continúa teniendo 256 KiB y alineación de 16 bytes.

- En VeeRwolf se ubica en `.bss`. El startup del SDK limpia esa sección antes
  de llamar a `main`, de modo que la arena queda inicializada sin una rutina
  manual adicional.
- En el testbench se conserva la sección `.tflm_arena`, que su startup y
  `tflm.ld` inicializan y colocan según el mapa de memoria de ese destino.

Esta diferencia es importante: `.tflm_arena` pertenece al linker personalizado
del testbench, mientras que el linker de VeeRwolf no define esa sección.

### Validación de tensores y predicción

Antes de cargar la imagen se comprueba que los tensores de entrada y salida
existan. La entrada solo se procesa si es `float32` de 784 elementos o `int8`
de 784 elementos. Después de la inferencia, se comprueba que la salida sea
`float32` o `int8` y que contenga diez scores. Un tipo o tamaño inesperado se
notifica por la consola en lugar de continuar leyendo memoria con una forma
incorrecta.

El cálculo del índice de mayor score no cambia: `max_index` identifica la
clase de dígito con mayor valor.

## 4. Modelo de solo lectura: `mnist_model.h`

El arreglo `mnist_tflite` se declaró `const`. Los bytes del modelo son datos
inmutables; esto evita tratarlos como estado modificable y permite al linker
colocarlos en la sección de solo lectura junto al resto de los datos constantes.
El intérprete recibe el modelo como `const tflite::Model*`.

## 5. Hooks de sistema: `veerwolf_syscalls.cc`

Se añadió este archivo para los símbolos de sistema newlib que puede requerir
el runtime enlazado por PlatformIO. Es código específico de la compilación
VeeRwolf; no sustituye ni modifica `syscalls.c`, usado por el testbench.

- `_write` permite salida estándar y de error (descriptores 1 y 2), enviando
  cada carácter a `printUartPutchar`, la rutina de transmisión UART del BSP.
  Los demás descriptores devuelven `EBADF`.
- `_read` informa que la lectura no está implementada (`ENOSYS`); este
  programa solo necesita emitir resultados.
- `_close`, `_fstat`, `_isatty` y `_lseek` proporcionan las respuestas de
  dispositivo de consola esperadas por newlib.
- `_sbrk` falla con `ENOMEM`. La aplicación reserva su arena TFLM estáticamente
  y no configura un heap dinámico para inferencia.
- `_getpid` y `_kill` satisfacen referencias de la biblioteca C.
- `__wrap__exit` detiene el procesador en un bucle si newlib intenta terminar
  el proceso. En bare-metal no existe un sistema operativo al que devolver el
  control.

Las definiciones están dentro de `extern "C"` aunque el archivo tenga extensión
`.cc`, para exponer exactamente los nombres C que espera newlib y enlazar la
función UART implementada por el BSP en C.

## 6. Archivos del destino de simulación que se conservan

El destino original sigue usando estos archivos, sin reemplazarlos por los del
SDK:

- `Makefile` compila `main.cc`, las fuentes de TFLM y el startup propio para el
  ELF del testbench.
- `crt0.S` configura `gp` y `sp`, limpia BSS y arena, ejecuta constructores y
  llama a `main`.
- `tflm.ld` define las ubicaciones de código, modelo, arena, DCCM, pila y datos
  de control del testbench.
- `syscalls.c` dirige `write` a la dirección de mailbox de la simulación.
- `printf.c` implementa el formato de impresión compatible con el mailbox.
- `run_sim.sh` construye el ELF, genera la imagen de programa, compila el
  harness y comprueba la señal de éxito de la simulación.

Para el build de PlatformIO, estos archivos quedan fuera del filtro de fuentes
y el SDK proporciona las piezas equivalentes de arranque, linker, BSP y carga
JTAG.

## 7. Artefactos generados: `.gitignore`

Se añadió [.gitignore](./.gitignore) para excluir `.pio/`, `build/`, objetos
`*.o` y `mnist_riscv_test.elf`. Son resultados locales de compilación o
simulación, no fuentes del firmware.

## 8. Documentación operativa existente

El [README](./README) se amplió con la descripción resumida del destino
VeeRwolf/Nexys y los comandos de compilación, programación de bitstream, carga
del firmware y monitor serie. Este documento complementario explica por qué se
añadieron esas piezas y qué hace cada una.

La secuencia usual para la Nexys A7 es:

```sh
pio run -e swervolf_nexys
pio run -e swervolf_nexys --target program_fpga
pio run -e swervolf_nexys --target upload
pio device monitor -b 115200
```

`program_fpga` carga el diseño VeeRwolf en la FPGA. `upload` transfiere y
ejecuta el firmware en la RAM del SoC; no programa una memoria no volátil.
Estas operaciones requieren que la placa y su conexión JTAG estén disponibles.

## 9. Comprobaciones realizadas

El firmware de VeeRwolf se compiló con:

```sh
pio run -e swervolf_nexys
```

La compilación terminó correctamente. PlatformIO informó 277032 bytes de RAM
usados de 1216512 bytes disponibles, aproximadamente un 22,8 %. La arena de
262144 bytes quedó en BSS, después de `__bss_start` y antes del final de los
datos BSS; el startup de la placa la limpia al arrancar.

La ejecución física ya fue probada por el usuario en la Nexys A7. El resultado
de compilación anterior no sustituye esa validación sobre hardware.
