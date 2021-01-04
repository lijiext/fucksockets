#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<bits/stdc++.h>
#include <WinSock2.h>
using namespace std;
char response[] = "HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/html; charset=UTF-8\r\n\r\n"
                  "<!DOCTYPE html>"
                  "<html lang=\"zh-CN\">"
                  "<head>"
                  "<meta charset=\"utf-8\">"
                  "<title>Hello World</title>"
                  "<style>body {width: 35em;margin: 200px auto;font-family: Tahoma, Verdana, Arial, sans-serif;}"
                  "</style>"
                  "</head>"
                  "<body><h1>Hello World!</h1>"
                  "<p>If you see this page, the JJWebserver is successfully"
                  " working. </p>"
                  "<p><em>Thank you for using JJWebserver</em></p>"
                  "</body></html>\r\n";

int main()
{
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    int client_fd;
    SOCKADDR_IN svr_addr, cli_addr;
    int sin_len = sizeof(cli_addr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        cout<< "无法开启socket";
        exit(0);
    }

    int port = 80;
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port = htons(port);

    if (bind(sock, ( SOCKADDR *) &svr_addr, sizeof(svr_addr)) == -1)
    {
        closesocket(sock);
        cout<<"无法绑定\n";
    }

    listen(sock, 5);
    while (1)
    {
        int len = sizeof(SOCKADDR);
        client_fd = accept(sock, (SOCKADDR *) &cli_addr, &sin_len);
        printf("客户端已连接\n");
        cout<<inet_ntoa(cli_addr.sin_addr)<<endl;
        if (client_fd == -1)
        {
            perror("accept()客户端连接出错");
            continue;
        }
        int x = send(client_fd,response, sizeof(response)-1,0);
        cout<<response<<endl;
        cout<<x<< "Bytes已发送" <<endl;

    }
    closesocket(client_fd);
    WSACleanup();
}
