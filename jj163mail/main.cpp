#include <iostream>
#include <string>
#include <WinSock2.h>
using namespace std;

string base64(string str)
{
    string base64_table="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int str_len = str.length();
    string res="";
    for (int strp=0; strp<str_len/3*3; strp+=3)
    {
        res+=base64_table[str[strp]>>2];
        res+=base64_table[(str[strp]&0x3)<<4 | (str[strp+1])>>4];
        res+=base64_table[(str[strp+1]&0xf)<<2 | (str[strp+2])>>6];
        res+=base64_table[(str[strp+2])&0x3f];
    }
    if (str_len%3==1)
    {
        int pos=str_len/3 * 3;
        res += base64_table[str[pos]>>2];
        res += base64_table[(str[pos]&0x3)<<4];
        res += "=";
        res += "=";
    }
    else if (str_len%3==2)
    {
        int pos=str_len/3 * 3;
        res += base64_table[str[pos]>>2];
        res += base64_table[(str[pos]&0x3)<<4 | (str[pos+1])>>4];
        res += base64_table[(str[pos+1]&0xf)<<2];
        res += "=";
    }
    return res;
}

int main()
{
    cout<< "\t\t\t网易邮箱电子邮件客户端 " ;
    char buff[500];
    string message;
    string info;
    string subject;
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    int err = WSAStartup(wVersionRequested, &wsaData);
    if(err != 0)
    {
        cout << "启动失败，错误码: " << err << endl;
        exit(0);
    }
    else
    {
        cout << "服务成功启动" <<endl;
    }
    SOCKET sockClient;
    sockClient = socket(AF_INET, SOCK_STREAM, 0);
    HOSTENT* pHostent;
    pHostent = gethostbyname("smtp.163.com");
    SOCKADDR_IN addrServer;
    addrServer.sin_addr.S_un.S_addr = *((DWORD *)pHostent->h_addr_list[0]);
    addrServer.sin_family = AF_INET;
    addrServer.sin_port = htons(25);
    err = connect(sockClient, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
    buff[recv(sockClient, buff, 500, 0)] = '\0';
    message = "helo 163.com\r\n";
    send(sockClient, message.c_str(), message.length(), 0);
    buff[recv(sockClient, buff, 500, 0)] = '\0';
    message = "auth login\r\n";
    send(sockClient, message.c_str(), message.length(), 0);
    buff[recv(sockClient, buff, 500, 0)] = '\0';
    string Smail;
    cout<< "请输入网易邮箱用户名（不包括@163.com）：";
    cin>>Smail;
    message = base64(Smail) + "\r\n";
    send(sockClient, message.c_str(), message.length(), 0);
    buff[recv(sockClient, buff, 500, 0)] = '\0';
    string Swd;
    cout << "请输入您的授权码：";
    cin>>Swd;
    message = base64(Swd) + "\r\n";
    send(sockClient, message.c_str(), message.length(), 0);
    buff[recv(sockClient, buff, 500, 0)] = '\0';
    string Rmail;
    cout << "输入收件人邮箱：";
    cin >> Rmail;
    message = "mail from:<" + Smail + "@163.com>\r\n";
    send(sockClient, message.c_str(), message.length(), 0);
    buff[recv(sockClient, buff, 500, 0)] = '\0';
    message = "rcpt to:<";
    message.append(Rmail);
    message.append(">\r\n");
    send(sockClient, message.c_str(), message.length(), 0);
    buff[recv(sockClient, buff, 500, 0)] = '\0';
    message = "DATA\r\n";
    send(sockClient, message.c_str(), message.length(), 0);
    buff[recv(sockClient, buff, 500, 0)] = '\0';
    message = "From: " + Smail + "@163.com\r\nTo: " + Rmail + "\r\nsubject:";
    cout<<"主题：";
    cin>>subject;
    message.append(subject);
    message.append("\r\n\r\n");
    cout<<"内容：";
    cin>>info;
    message.append(info);
    message.append("\r\n.\r\n");
    send(sockClient, message.c_str(), message.length(), 0);
    buff[recv(sockClient, buff, 500, 0)] = '\0';
    message = "QUIT\r\n";
    send(sockClient, message.c_str(), message.length(), 0);
    buff[recv(sockClient, buff, 500, 0)] = '\0';
    cout<<"发送成功!"<<endl;
    system("pause");
}

