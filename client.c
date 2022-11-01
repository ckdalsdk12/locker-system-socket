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
#include "reader.c"
#include "locker_info.c"
#define DEFAULT_PROTOCOL 0
#define MAXLINE 50 // NULL 문자 포함 크기 50의 password와 strorage를 위한 매크로.

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    // 소켓을 위한 fd 생성 및 소켓 이름 지정을 위해 구조체 생성.
    // password, storage를 위한 char형 배열도 선언. 
    int clientfd, result;
    // char password[MAXLINE], storage[MAXLINE];
    struct sockaddr_un serverUNIXaddr;

    // 유닉스 소켓을 생성하고 path라는 이름을 지정.
    clientfd = socket(AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
    serverUNIXaddr.sun_family = AF_UNIX;
    strcpy(serverUNIXaddr.sun_path, "path");

    do { // 연결 요청
    result = connect(clientfd, (struct sockaddr *)&serverUNIXaddr, sizeof(serverUNIXaddr));
    if (result == -1) sleep(1);
    } while (result == -1);
    
    // 사물함 개수 받기.
    int n;
    read(clientfd, &n, sizeof(int));

    // 사물함 개수 만큼 사물함에 대한 정보를 받고 출력.
    for(int i = 0; i < n; i++) {
        int id, in_use;
        read(clientfd, &id, sizeof(int));
        read(clientfd, &in_use, sizeof(int));
        printf("[사물함 ID : %d, 사용 여부 : %d]\n", id, in_use);
    }

    START :
    while(1)
    {
        // 클라이언트가 원하는 사물함의 id 입력 및 전송.
        printf("접근 할 사물함의 id를 입력해주세요.\n");
        printf("사물함 정보를 새로고침 하려면 -1을 입력해주세요.\n");
        int id;
        scanf("%d", &id);
        while(getchar() != '\n'); // 입력 버퍼 비워 줌
        if (id >= n || id < -1)
        {
            printf("입력한 id에 맞는 사물함이 없습니다. 다시 입력해주세요.\n");
            continue;
        }
        if (id == -1) // -1일 경우 사물함 정보를 새로고침
        {
            write(clientfd, &id, sizeof(int));
            inforead(clientfd, n);
            continue;
        }
        // 서버에 전송 후 응답을 받음.
        write(clientfd, &id, sizeof(int));
        int in_use;
        read(clientfd, &in_use, sizeof(int));
        // 응답에 따라 새롭게 사물함을 사용하는 모드와 사물함을 반납하는 모드로 나뉨.
        int retry = 0; // 비밀번호 일치하지 않아 재시도 시 서버에 pid 재전송을 막는 조건
        while(1) {
            // 사물함이 비어있으면
            if (in_use == 0)
            {
                int pid = getpid();
                if (retry == 0) write(clientfd, &pid, sizeof(int)); // 클라이언트의 pid 전달
                printf("사물함에 설정할 패스워드를 입력해 주세요.\n");
                char password1[MAXLINE];
                scanf("%s", password1);
                while(getchar() != '\n'); // 입력 버퍼 비워 줌
                printf("패스워드를 한번 더 입력해 주세요.\n");
                char password2[MAXLINE];
                scanf("%s", password2);
                while(getchar() != '\n'); // 입력 버퍼 비워 줌
                if (strcmp(password1, password2) != 0) // 두 번의 비밀번호가 일치하지 않으면 다시 입력.
                {
                    printf("패스워드가 일치하지 않습니다. 다시 입력해 주세요.\n");
                    retry++;
                    continue;
                }
                // 두 번의 비밀번호가 일치하면 서버에 전송 후 사물함에 저장할 내용 입력.
                else if (strcmp(password1, password2) == 0) {
                    write(clientfd, password1, strlen(password1)+1);
                    printf("사물함에 저장할 내용을 입력해 주세요.\n");
                    char storage[MAXLINE];
                    scanf("%s", storage);
                    while(getchar() != '\n'); // 입력 버퍼 비워 줌
                    write(clientfd, storage, strlen(storage)+1);
                    inforead(clientfd, n);
                    retry = 0;
                    goto START;
                }
            }
            // 사물함이 사용중이면
            else if (in_use == 1) {
                printf("이전에 설정했던 패스워드를 입력해 주세요.\n");
                char password[MAXLINE];
                scanf("%s", password);
                while(getchar() != '\n'); // 입력 버퍼 비워 줌
                write(clientfd, password, strlen(password)+1);
                int allowed;
                read(clientfd, &allowed, sizeof(int));
                // 패스워드가 틀렸으면
                if (allowed == 0)
                {
                    printf("패스워드가 틀렸습니다.\n");
                    // fail_count가 3일 경우 서버측에서 10초 대기
                    int fail_count;
                    read(clientfd, &fail_count, sizeof(int));
                    if (fail_count == 3)
                    {
                        printf("패스워드를 연속으로 3회 틀려 10초간 대기합니다.\n");
                    }
                    inforead(clientfd, n);
                    goto START;
                }
                // 패스워드가 맞았으면
                else if (allowed == 1) {
                    char storage[MAXLINE];
                    readLine(clientfd, storage);
                    printf("%d번 사물함에 보관 되었던 내용 : \n%s\n", id, storage);
                    inforead(clientfd, n);
                    goto START;
                }
            }
        }
    }
}