#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_NUM	6


int main(int argc, char const *argv[])
{
    struct pollfd pfds[BUFFER_NUM];
    char dev_name[13];
    char buf[1];

    for (int8_t i = 0; i < BUFFER_NUM; i++)
    {
        sprintf(dev_name, "/dev/shofer%d", i);
        pfds[i].fd = open(dev_name, O_RDONLY);
        pfds[i].events = POLLIN;
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

        for (int i = 0; i < BUFFER_NUM; i++)
        {
            if (pfds[i].revents & POLLIN)
            {
                read(pfds[i].fd, buf, 1);
                printf("shofer%d čitam: %c\n", i, buf[0]);
            }
            else 
            {
                printf("shofer%d nema ništa.\n", i);
            }
        }
        
    }
}
