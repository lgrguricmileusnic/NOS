#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define BUFFER_SIZE	64
#define BUFFER_NUM	6


int main(int argc, char const *argv[])
{
    struct pollfd pfds[BUFFER_NUM];
    char dev_name[13];
    char buf[] = "Z";

    srand(time(NULL));

    for (int8_t i = 0; i < BUFFER_NUM; i++)
    {
        sprintf(dev_name, "/dev/shofer%d", i);
        pfds[i].fd = open(dev_name, O_WRONLY);
        pfds[i].events = POLLOUT;
    }

    for(;;)
    {
        printf("Čekam neku napravu...\n");
        int poll_count = poll(pfds, BUFFER_NUM, -1);
        printf("Spremno je %d naprava\n", poll_count);
        if (poll_count == -1)
        {
            perror("poll");
            exit(1);
        }
        
        int poll_counter = rand() % poll_count;

        for (int i = 0; i < BUFFER_NUM; i++)
        {
            if (pfds[i].revents & POLLOUT)
            {
                if (--poll_counter <= 0)
                {
                    write(pfds[i].fd, buf, 1);
                    printf("shofer%d pišem: %c\n", i, buf[0]);
                    break;
                }

            }
            else 
            {
                printf("shofer%d nema ništa.\n", i);
            }
        }

        printf("Spavam 5s.\n");
        sleep(5);
        
    }
}
