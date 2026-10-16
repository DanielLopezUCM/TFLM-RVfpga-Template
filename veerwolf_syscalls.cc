#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

extern "C" {
int printUartPutchar(char character);

int _write(int file, const char *buffer, int length) {
    if (file != 1 && file != 2) {
        errno = EBADF;
        return -1;
    }

    for (int i = 0; i < length; ++i) {
        printUartPutchar(buffer[i]);
    }
    return length;
}

int _read(int file, char *buffer, int length) {
    (void)file;
    (void)buffer;
    (void)length;
    errno = ENOSYS;
    return -1;
}

int _close(int file) {
    (void)file;
    errno = EBADF;
    return -1;
}

int _fstat(int file, struct stat *status) {
    (void)file;
    status->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    return file == 1 || file == 2;
}

int _lseek(int file, int offset, int whence) {
    (void)file;
    (void)offset;
    (void)whence;
    errno = ESPIPE;
    return -1;
}

void *_sbrk(ptrdiff_t increment) {
    (void)increment;
    errno = ENOMEM;
    return (void *)-1;
}

int _getpid(void) {
    return 1;
}

int _kill(int process, int signal) {
    (void)process;
    (void)signal;
    errno = EINVAL;
    return -1;
}

void __wrap__exit(int status) {
    (void)status;
    for (;;) {
        __asm__ volatile("nop");
    }
}
}
