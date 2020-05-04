#ifndef _PRIMEDECOMPOSE_H_
#define _PRIMEDECOMPOSE_H_
#include <gmp.h>
#define ENABLE_IO 1
#define DISABLE_IO 0
int decompose(mpz_t n, mpz_t *o, long *elapsed, int io_enable, char *filename);
#endif
