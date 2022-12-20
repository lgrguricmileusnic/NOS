#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#define NUM_THREADS 2

void *thread_job(void *vargp)
{
    int *myid = (int *)vargp;

    for(;;)
    {
        printf("Thread %d: Spavam 5s\n", *myid);
        sleep(1);
    }
    
    return NULL;
}

int main(int argc, char const *argv[])
{
    
    int fd = open("/dev/pipenos", O_RDWR);
    pthread_t threads[NUM_THREADS];

    if (fd == -1)
    {
        printf("Nisam uspio sam otvoriti pipenos za citanje i pisanje\n");
    }

    fd = open("/dev/pipenos", O_RDONLY);
    if (fd != -1)
    {
        printf("Uspio sam otvoriti pipenos za citanje\n");
    }
    close(fd);
    fd = open("/dev/pipenos", O_WRONLY);
    if (fd != -1)
    {
        printf("Uspio sam otvoriti pipenos za pisanje\n");
    }
    close(fd);

    // Let us create three threads
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_create(&threads[i], NULL, thread_job, (void *)&i);

    pthread_exit(NULL);
    return 0;
}
