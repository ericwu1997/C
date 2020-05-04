#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

#include "./lib/primedecompose.h"

#define MAX_FACTORS 1024

void *task(void *);

struct Data
{
    int id;
    int flag;
    char *filename;
    int count;
    mpz_t *list;
    long *elapsed;
    mpz_t *dest;
};

/********************************************************************************
* This program creates a number of worker thread to perform a basic prime number
* factorization.
* The original process creates n thread, where n is the user specify value
* at command line. 
********************************************************************************/

int main(int argc, char *argv[])
{
    int i, t_num, task_count, range;
    long elapsed;
    int flag = DISABLE_IO;
    pthread_t *threads;
    struct Data **data_list;

    mpz_t **dest;
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

    t_num = atoi(argv[optind]);          // number of process
    task_count = atoi(argv[optind + 1]); // number of task to run per process
    range = t_num * task_count;          // amount of number to factorize
    mpz_init_set_str(start_num, argv[optind + 2], 10);

    n = (mpz_t *)malloc(sizeof(mpz_t) * range); // initialize number list
    for (i = 0; i < range; i++)
    {
        mpz_add_ui(*(n + i), start_num, (unsigned long int)i); // add 1 to create a range of number
    }                                                          // starting from start_num

    threads = malloc(sizeof(pthread_t) * t_num);       // initialize worker list
    data_list = malloc(sizeof(struct Data *) * range); // initialize data list
    dest = malloc(sizeof(mpz_t *) * t_num);            // dest list for workers to store result
    for (i = 0; i < t_num; i++)                        // assign tasks to workers
    {
        data_list[i] = (struct Data *)malloc(sizeof(struct Data));
        data_list[i]->id = i;
        data_list[i]->flag = flag;
        data_list[i]->filename = filename;
        data_list[i]->count = task_count;
        data_list[i]->list = n + (i * task_count);
        data_list[i]->elapsed = &elapsed;
        dest[i] = malloc(sizeof(mpz_t) * MAX_FACTORS);
        data_list[i]->dest = dest[i];
        pthread_create(&threads[i], NULL, task, (void *)data_list[i]);
    }
    for (i = 0; i < t_num; i++)
        pthread_join(threads[i], NULL);

    for (i = 0; i < range; i++)
    {
        free(data_list[i]);
    }
    for (i = 0; i < t_num; i++)
    {
        free(dest[i]);
    }

    free(data_list);
    free(threads);
    free(dest);
    free(n);

    return 0;
}

void *task(void *_data)
{
    int i, j, l = 0;
    long total_elapsed = 0;
    struct Data *data = (struct Data *)_data;
    for (i = 0; i < data->count; i++)
    {
        l = decompose(*((data->list) + i), data->dest, data->elapsed, data->flag, data->filename);
        for (j = 0; j < l; j++)
        {
            mpz_clear(data->dest[j]);
        }
        total_elapsed += *(data->elapsed);
    }
    printf("Thread ID: %d, time used: %ld msec\n", data->id, total_elapsed);
    return NULL;
}
