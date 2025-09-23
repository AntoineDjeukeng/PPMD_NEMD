#include "pc.h"
#include <stdio.h>


static void start_threads(pthread_t *threads, int ac, char **av)
{
    int i;
    static int cidx[NUM_CONSUMERS];
	if (ac < 2) {
		fprintf(stderr,"Usage: %s file.gro [channel.json]\n", av[0]);
		return ;
	}
    //parse av to produce
    if (pthread_create(&threads[0], NULL, producer, av) != 0)
        perror("producer");
    i = 1;
    while (i < THREAD_COUNT) {
        cidx[i - 1] = i - 1;
        if (pthread_create(&threads[i], NULL, consumer, &cidx[i - 1]) != 0)
            perror("consumer");
        i++;
    }
}

static void join_threads(pthread_t *threads)
{
    int i;

    i = 0;
    while (i < THREAD_COUNT) {
        pthread_join(threads[i], NULL);
        i++;
    }
}

int main(int ac, char **av)
{
    pthread_t threads[THREAD_COUNT];

    set_stdout_line_buffered();
    ring_init();
    // (void)ac; (void)av;

    start_threads(threads, ac, av);
    join_threads(threads);
    return 0;
}
