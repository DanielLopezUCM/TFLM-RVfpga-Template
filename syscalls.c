#include <sys/stat.h>
#include <stddef.h>

#define SIM_CONSOLE_ADDR (*(volatile char*)0xd0580000)

int _write(int file, char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        SIM_CONSOLE_ADDR = ptr[i];
    }
    return len;
}

void *_sbrk(ptrdiff_t incr) {
    extern char _end;
    static char *heap_ptr = &_end;
    char *base = heap_ptr;
    heap_ptr += incr;
    return (void*) base;
}

int _close(int file) {return -1;}
int _fstat(int file, struct stat *st) {st->st_mode = S_IFCHR; return 0;}
int _isatty(int file) {return 1;}
int _lseek(int file, int offset, int whence) {return 0;}
int _read(int file, char *ptr, int len) {return 0;}
void _exit(int status) {
    while(1);    
}

