#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include "locker.h"

void infowrite(int connfd, struct locker *locker, int n)
{
    // 사물함 개수 클라이언트에게 전송.
    write(connfd, &n, sizeof(int));
    // 사물함 상황 클라이언트에게 전송.
    for(int i = 0; i < n; i++)
    {
        write(connfd, &locker[i].id, sizeof(int));
        //write(connfd, locker[i].password, strlen(locker[i].password)+1);
        //write(connfd, locker[i].storage, strlen(locker[i].storage)+1);
        write(connfd, &locker[i].in_use, sizeof(int));
    }
}

void inforead(int clientfd, int n)
{
    // 사물함 개수 받기.
    read(clientfd, &n, sizeof(int));

    // 사물함 개수 만큼 사물함에 대한 정보를 받고 출력.
    printf("------------------------------\n");
    for(int i = 0; i < n; i++) {
        int id, in_use;
        read(clientfd, &id, sizeof(int));
        //readLine(clientfd, password);
        //readLine(clientfd, storage);
        read(clientfd, &in_use, sizeof(int));
        printf("[사물함 ID : %d, 사용 여부 : %d]\n", id, in_use);
    }
    printf("------------------------------\n");
}