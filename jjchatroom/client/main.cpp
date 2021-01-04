#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <process.h>
#include <math.h>
#include <iostream>
#define    BUFLEN        2000
#define WSVERS        MAKEWORD(2, 2)

using namespace std;


SOCKET    sock,sockets[100] = {NULL};
char *packet = NULL;
char *pts,*input;
HANDLE hThread;
unsigned threadID;

unsigned int __stdcall Chat(PVOID PM )
{
    time_t    now;
    (void) time(&now);
    pts = ctime(&now);
    char buf[2000];

    while(1)
    {
        int cc = recv(sock, buf, BUFLEN, 0);
        cout<< endl;
        if(cc == SOCKET_ERROR|| cc == 0)
        {

            cout<< "错误，与服务器断开连接" << GetLastError()<< endl;
            CloseHandle(hThread);
            (void)closesocket(sock);
            break;
        }
        else if(cc > 0)
        {
            printf("\n%s\n",buf);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    time_t    now;
    (void) time(&now);
    pts = ctime(&now);
    char    *host = "127.0.0.1";
    char *service = "5050";
    struct  sockaddr_in sin;

    WSADATA wsadata;

    WSAStartup(WSVERS, &wsadata);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons((u_short)atoi(service));
    sin.sin_addr.s_addr = inet_addr(host);

    sock = socket(PF_INET, SOCK_STREAM,0);

    connect(sock, (struct sockaddr *)&sin, sizeof(sin));

    printf("聊天室客户端(Client)已启动\n");
    hThread = (HANDLE)_beginthreadex(NULL, 0,Chat, NULL, 0, &threadID);

    while(1)
    {
        char buf1[2000];
        printf("请输入要发送的信息：");
//        scanf("%s",&buf1);
        cin>> buf1;
        cout<< endl;
        if(!strcmp(buf1,"exit"))
            goto end;
        send(sock,buf1, sizeof(buf1), 0);
        cout<< endl;
    }

end:
    CloseHandle(hThread);
    closesocket(sock);
    WSACleanup();
    printf("按回车键继续...");
    getchar();
    return 0;
}
