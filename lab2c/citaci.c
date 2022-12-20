#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>

#define NUM_THREADS 2
#define BUFFER_SIZE 64

void *thread_job(void *vargp)
{
    srand(time(NULL));

    int myid = *(int *)vargp;
    int t;
    int fd = open("/dev/pipenos", O_RDONLY);
    char buf[BUFFER_SIZE];
    for(;;)
    {
        print("Thread %d: Citam...\n");
        int nbytes = read(fd, buf, 1);
        printf("Thread %d: Procitao %s\n", myid, buf);
        t = rand() % 5 + 1;
        printf("Thread %d: Spavam %ds\n", myid, t);
        sleep(t);
    }
    
    return NULL;
}

int main(int argc, char const *argv[])
{
    
    int fd = open("/dev/pipenos", O_RDWR);
    pthread_t threads[NUM_THREADS];
    int tids[NUM_THREADS];

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
    {
        tids[i] = i;
        pthread_create(&threads[i], NULL, thread_job, (void *)&tids[i]);
    }

    pthread_exit(NULL);
    return 0;
}
