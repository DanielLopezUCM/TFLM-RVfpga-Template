#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Newlib low-level I/O stubs.
 *
 * TFLM itself does not use these for tensor allocation.
 * They only satisfy libc/newlib references on this bare-metal target.
 */

/* ------------------------------------------------------------------ */
/* Process / exit                                                      */
/* ------------------------------------------------------------------ */

void __wrap__exit(int code)
{
    (void)code;

    /*
     * There is no operating system to return to.
     * Stop here forever.
     */
    while (1) {
        __asm__ volatile ("nop");
    }
}


/* ------------------------------------------------------------------ */
/* Memory                                                               */
/* ------------------------------------------------------------------ */

/*
 * Deliberately disable the traditional newlib heap.
 *
 * TFLM uses its own static tensor arena, so we do not want
 * newlib to allocate dynamic memory.
 */
void *_sbrk(ptrdiff_t incr)
{
    (void)incr;

    return (void *)-1;
}

void *__wrap_sbrk(ptrdiff_t incr)
{
    return _sbrk(incr);
}


/* ------------------------------------------------------------------ */
/* File descriptor / POSIX-like stubs                                  */
/* ------------------------------------------------------------------ */

int _write(int fd, const void *buf, size_t count)
{
    (void)fd;
    (void)buf;
    (void)count;

    return -1;
}


int _read(int fd, void *buf, size_t count)
{
    (void)fd;
    (void)buf;
    (void)count;

    return -1;
}


int _close(int fd)
{
    (void)fd;

    return -1;
}


int _fstat(int fd, void *st)
{
    (void)fd;
    (void)st;

    return -1;
}


int _isatty(int fd)
{
    (void)fd;

    return 0;
}


off_t _lseek(int fd, off_t offset, int whence)
{
    (void)fd;
    (void)offset;
    (void)whence;

    return (off_t)-1;
}


/* ------------------------------------------------------------------ */
/* Signals / process                                                   */
/* ------------------------------------------------------------------ */

int _getpid(void)
{
    return 1;
}


int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;

    return -1;
}


#ifdef __cplusplus
}
#endif