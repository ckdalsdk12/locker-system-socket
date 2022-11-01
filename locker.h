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

struct locker {
    int id; // 사물함 번호
    char password[50]; // 패스워드 저장
    char storage[50]; // 사물함 내부 저장 공간
    int in_use; // 사용 여부를 나타냄. 0 = 미사용, 1 = 사용
    int pid; // 사물함에 접근한 클라이언트의 pid 저장 용도
};