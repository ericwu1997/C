#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "./lib/primedecompose.h"

#define MAX_FACTORS 1024

/********************************************************************************
* This program creates a number of worker process to perform a basic prime number
* factorization.
* The original process creates n worker, where n is the user specify value
* at command line. 
********************************************************************************/

int main(int argc, char *argv[])
{
    int i, j, l, p_num, task_count, range, index, status = 0;
    pid_t child_pid, wpid;
    long elapsed, total_elapsed = 0;
    int flag = DISABLE_IO;

    mpz_t dest[MAX_FACTORS];
    mpz_t *n;
    mpz_t start_num;

    char *filename;

    int c;

    opterr = 0;

    while ((c = getopt(argc, argv, "w:")) != -1) // checking and apply option
        switch (c)
        {
        case 'w':
            flag = ENABLE_IO;
            filename = optarg;
            break;
        case '?':
            if (optopt == 'w')
                fprintf(stderr, "Option -%c requires an argument (valid filename).\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
            return 1;
        default:
            abort();
        }

    if (argc - optind != 3)
    { // check for valid number of command-line arguments
        fprintf(stderr, "Usage: %s <number of worker> <number of task per worker> <number to be factored> \n", argv[0]);
        return 1;
    }

    p_num = atoi(argv[optind]);          // number of process
    task_count = atoi(argv[optind + 1]); // number of task to run per process
    range = p_num * task_count;          // amount of number to factorize
    mpz_init_set_str(start_num, argv[optind + 2], 10);

    n = (mpz_t *)malloc(sizeof(mpz_t) * range); // initialize number list

    for (i = 0; i < range; i++)
    {
        mpz_add_ui(*(n + i), start_num, (unsigned long int)i); // add 1 to create a range of number
    }                                                          // starting from start_num

    for (i = 0; i < p_num; i++)
    {
        index = (i * task_count); // fork() and assign indices
        child_pid = fork();       // each process should work from
        if (child_pid <= 0)
            break;
    }

    if (child_pid == -1)
    {
        perror("Fork Failed!");
        return 1;
    }
    if (child_pid == 0)
    {
        for (i = 0; i < task_count; i++)
        {
            l = decompose(*(n + index + i), dest, &elapsed, flag, filename);
            for (j = 0; j < l; j++)
            {
                mpz_clear(dest[j]);
            }
            total_elapsed += elapsed;
        }
        printf("Process ID: %ld, time used: %ld msec\n", (long)getpid(), total_elapsed);

        exit(0);
    }
    else
    {
        while ((wpid = wait(&status)) > 0) // Wait for all child process
        {
        }
        free(n);
    }
    return 0;
}
