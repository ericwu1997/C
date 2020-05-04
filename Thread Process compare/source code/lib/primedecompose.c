#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "primedecompose.h"

int decompose(mpz_t n, mpz_t *o, long *elapsed, int io_enable, char *filename)
{
	FILE *fptr;
	char str[128];
	int i = 0, count, index = 0;
	mpz_t tmp, d;
	struct timeval tv_start, tv_end;

	mpz_init(tmp);
	mpz_init(d);

	gettimeofday(&tv_start, 0);
	switch (io_enable)
	{
	case ENABLE_IO:
		fptr = fopen(filename, "a");
		while (mpz_cmp_si(n, 1))
		{
			mpz_set_ui(d, 1);
			do
			{
				mpz_add_ui(tmp, d, 1);
				mpz_swap(tmp, d);
			} while (!mpz_divisible_p(n, d));
			mpz_divexact(tmp, n, d);
			mpz_swap(tmp, n);
			mpz_init(o[i]);
			mpz_set(o[i], d);
			i++;
		}
		for (count = 0; count < i; count++)
			index += gmp_snprintf(&str[index], 128 - index, "%s %Zd ", (count ? "*" : ""), o[count]);
		gmp_fprintf(fptr, "%-50s ", str);

		gettimeofday(&tv_end, 0);
		*elapsed = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec;

		fprintf(fptr, "%10ld msec\n", *elapsed);
		fclose(fptr);
		break;
	case DISABLE_IO:
	default:
		while (mpz_cmp_si(n, 1))
		{
			mpz_set_ui(d, 1);
			do
			{
				mpz_add_ui(tmp, d, 1);
				mpz_swap(tmp, d);
			} while (!mpz_divisible_p(n, d));
			mpz_divexact(tmp, n, d);
			mpz_swap(tmp, n);
			mpz_init(o[i]);
			mpz_set(o[i], d);
			i++;
		}
		gettimeofday(&tv_end, 0);
		*elapsed = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec;
		break;
	}
	return i;
}