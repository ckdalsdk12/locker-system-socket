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
#include <pthread.h>
#include "reader.c"
#include "locker_info.c"
#define DEFAULT_PROTOCOL 0
#define MAXLINE 50 // NULL 문자 포함 크기 50의 password와 strorage를 위한 매크로.

// 모든 사물함 구조체 파일 저장 함수
void save_to_file_all(FILE *fp, struct locker *locker, int n, char **argv)
{
    fp = fopen(argv[1], "wb");
    fseek(fp, 0L, SEEK_SET);
    fwrite(locker, sizeof(struct locker), n, fp);
    fflush(fp);
    fclose(fp);
}

// 파일에서 모든 사물함 구조체를 받는 함수
void read_file_all(FILE *fp, struct locker *locker, int n, char **argv)
{
    fp = fopen(argv[1], "rb");
    fseek(fp, 0L, SEEK_SET);
    fread(locker, sizeof(struct locker), n, fp);
    fclose(fp);
}

// 스레드를 사용해 모든 사물함 구조체 정보를 반복하여 출력하는 함수의 매개변수를 위한 구조체
struct for_thread
{
    FILE *fp;
    struct locker *locker;
    int n;
    char **argv;
};

// 스레드를 사용해 모든 사물함 구조체 정보를 반복하여 출력하는 함수
void* repeat_print_info(void *for_thread)
{
    struct for_thread *info = (struct for_thread *)for_thread;
    while(1) {
        read_file_all(info->fp, info->locker, info->n, info->argv); // 파일 정보 불러오기
        // 5초마다 노쇼 사물함 체크 -> 해제
        for(int i = 0; i < info->n; i++) {
            if (strcmp(info->locker[i].password, "MasterKey") == 0 && strcmp(info->locker[i].storage, "No-Show") == 0 && \
            0 != kill(info->locker[i].pid, 0)) { // pid를 이용해, 클라이언트 프로세스가 죽었는지 파악. 패스워드 입력 도중 삭제 방지.
                info->locker[i].in_use = 0;
            }
        }
        save_to_file_all(info->fp, info->locker, info->n, info->argv); // 파일로 저장하기
        // 5초마다 사물함 구조체 정보 출력
        printf("------------------------------\n");
        for(int i = 0; i < info->n; i++) {
            printf("[사물함 ID : %d, 사용 여부 : %d]\n", info->locker[i].id, info->locker[i].in_use);
        }
        printf("------------------------------\n");
        sleep(5);
    }
}

int main(int argc, char *argv[])
{
    pthread_t thread;
    FILE *fp; // 사물함 구조체를 저장할 포인터
    setbuf(stdout, NULL);
    // 프로그램 실행 시 첫번째 매개변수로 사물함 개수 지정.
    const int n = atoi(argv[1]);
    struct locker *locker;
    locker = (struct locker *) malloc (n * sizeof(struct locker));

    // 소켓을 위한 fd 생성 및 소켓 이름 지정을 위해 구조체 생성.
    int listenfd, connfd, clientlen;
    struct sockaddr_un serverUNIXaddr, clientUNIXaddr;

    signal(SIGCHLD, SIG_IGN); // SIGCHLD 무시

    clientlen = sizeof(clientUNIXaddr);

    // 유닉스 소켓을 생성하고 path라는 이름을 지정.
    listenfd = socket(AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
    serverUNIXaddr.sun_family = AF_UNIX;
    strcpy(serverUNIXaddr.sun_path, "path");
    unlink("path");
    bind(listenfd, (struct sockaddr *)&serverUNIXaddr, sizeof(serverUNIXaddr));

    listen(listenfd, 128); // 클라이언트의 connect 요청 대기 

    // 사물함 개수 출력
    printf("%d개의 사물함을 만들었습니다!\n", n);
    fflush(stdout);

    // 5초마다 사물함 구조체 정보를 출력하는 스레드를 돌리기 전, 파일 존재 확인, 없으면 생성. 
    fp = fopen(argv[1], "rb");
    if (fp == NULL) // 만약 n 파일이 없으면 사물함 구조체 직접 초기화
    {
        for(int i = 0; i < n; i++)
        {
            locker[i].id = i;
            strcpy(locker[i].password, "MasterKey"); // 패스워드 지정과 내용 저장을 마지지 않고 클라이언트 종료시 기본값
            strcpy(locker[i].storage, "No-Show"); // 패스워드 지정과 내용 저장을 마지지 않고 클라이언트 종료시 기본값
            locker[i].in_use = 0;
            locker[i].pid = -1;
        }
        save_to_file_all(fp, locker, n, argv);
    }
    else fclose(fp);

    // 5초마다 사물함 구조체 정보를 출력 및 노쇼 사물함을 제거하는 스레드 생성.
    struct for_thread *for_thread;
    for_thread = (struct for_thread *)malloc(sizeof(struct for_thread));
    for_thread -> fp = fp;
    for_thread -> locker = locker;
    for_thread -> n = n;
    for_thread -> argv = argv;
    pthread_create(&thread, 0, repeat_print_info, (void *)for_thread);

    while (1) {
        connfd = accept(listenfd, (struct sockaddr *)&clientUNIXaddr, &clientlen);
        if (fork() == 0) {
            // 사물함 개수 클라이언트에게 전송.
            write(connfd, &n, sizeof(int));
            // n 파일 열기 시도
            fp = fopen(argv[1], "rb");
            if (fp == NULL) // 만약 n 파일이 없으면 사물함 구조체 직접 초기화
            {
                for(int i = 0; i < n; i++)
                {
                    locker[i].id = i;
                    strcpy(locker[i].password, "MasterKey"); // 패스워드 지정과 내용 저장을 마지지 않고 클라이언트 종료시 기본값
                    strcpy(locker[i].storage, "No-Show"); // 패스워드 지정과 내용 저장을 마지지 않고 클라이언트 종료시 기본값
                    locker[i].in_use = 0;
                    locker[i].pid = -1;
                }
            }
            else { // n 파일이 있으면 파일 내용으로 사물함 구조체 초기화
                fseek(fp, 0L, SEEK_SET);
                fread(locker, sizeof(struct locker), n, fp);
                fclose(fp);
            }
            // 클라이언트에게 구조체 정보 전송
            for(int i = 0; i < n; i++)
            {
                write(connfd, &locker[i].id, sizeof(int));
                write(connfd, &locker[i].in_use, sizeof(int));
            }
            // 모든 사물함 구조체의 내용을 n 파일에 저장
            save_to_file_all(fp, locker, n, argv);

            // 패스워드 틀린 횟수 카운트
            int fail_count = 0;

            while(1)
            {
                // 클라이언트에게 원하는 사물함 id 받기.
                int id;
                char password[MAXLINE];
                char storage[MAXLINE];
                read(connfd, &id, sizeof(int));
                read_file_all(fp, locker, n, argv);
                if (id == -1)
                {
                    infowrite(connfd, locker, n);
                    continue;
                }
                if (locker[id].in_use == 0) // 사물함이 비어있을 때
                {
                    strcpy(locker[id].password, "MasterKey"); // 패스워드 지정과 내용 저장을 마지지 않고 클라이언트 종료시 기본값
                    strcpy(locker[id].storage, "No-Show"); // 패스워드 지정과 내용 저장을 마지지 않고 클라이언트 종료시 기본값
                    write(connfd, &locker[id].in_use, sizeof(int)); // in_use 값 전송
                    // locker[id]의 in_use를 1로 변경하고, 해당 locker[id] 부분만 파일 입출력.
                    locker[id].in_use = 1;
                    int pid;
                    read(connfd, &pid, sizeof(int));
                    locker[id].pid = pid; // 클라이언트의 pid를 전송받아 저장
                    fp = fopen(argv[1], "rb+");
                    fseek(fp, sizeof(struct locker) * id, SEEK_SET);
                    fwrite(&locker[id], sizeof(struct locker), 1, fp);
                    fflush(fp);
                    fclose(fp);
                    // 패스워드 전달 받아 locker[id]에 기록.
                    readLine(connfd, password);
                    strcpy(locker[id].password, password);
                    readLine(connfd, storage);
                    strcpy(locker[id].storage, storage);
                    infowrite(connfd, locker, n);
                }
                else if (locker[id].in_use == 1) { // 사물함이 사용중일 때
                    write(connfd, &locker[id].in_use, sizeof(int)); // in_use 값 전송
                    readLine(connfd, password);
                    if(strcmp(locker[id].password, password) != 0) // 패스워드가 틀릴 때
                    {
                        int allowed = 0;
                        write(connfd, &allowed, sizeof(int)); // 비밀번호가 틀렸으므로 0 전송
                        fail_count++;
                        write(connfd, &fail_count, sizeof(int)); // fail_count 전송
                        if (fail_count == 3) // fail_count가 3이 되면 10초 대기 및 0으로 초기화
                        {
                            sleep(10);
                            fail_count = 0;
                        }
                        infowrite(connfd, locker, n);
                        continue;
                    }
                    else if (strcmp(locker[id].password, password) == 0) { // 패스워드가 맞을 때
                        int allowed = 1;
                        write(connfd, &allowed, sizeof(int)); // 비밀번호가 맞았으므로 1 전송
                        fail_count = 0; // fail_count 0으로 초기화
                        strcpy(storage, locker[id].storage);
                        write(connfd, storage, strlen(storage)+1);
                        locker[id].in_use = 0;
                        strcpy(locker[id].password, "MasterKey"); // 패스워드 지정과 내용 저장을 마지지 않고 클라이언트 종료시 기본값
                        strcpy(locker[id].storage, "No-Show"); // 패스워드 지정과 내용 저장을 마지지 않고 클라이언트 종료시 기본값
                        save_to_file_all(fp, locker, n, argv);
                        infowrite(connfd, locker, n);
                        continue;
                    }
                }
                // 모든 사물함 구조체의 내용을 n 파일에 저장
                save_to_file_all(fp, locker, n, argv);
            }
        } else {
            close(connfd);
        }
    }
    return 0;
}