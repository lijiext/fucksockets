#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <map>
//only used in vc++
//#pragma comment( lib, "ws2_32.lib" )


using namespace std;
map<int,string> errorMap;
//代理服务LISTEN端口8080
int port = 8080;

SOCKET listen_sock;
SOCKET client_sock;

char FR_recv_buf [10240000] = "";
//char * FR_recv_buf = (char *) malloc(sizeof(999999999));
//char * FR_recv_buf = NULL;


char recv_buf [10240000] = "";

int Receive();
int Listen();

//初始化winsock
bool InitializeWinsock()
{

    errorMap[10004] = "WSAEINTR Interrupted function call.";
    errorMap[10038] = "WSAENOTSOCK Socket operation on nonsocket.";
    errorMap[10093] = "WSANOTINITIALISED Successful WSAStartup not yet performed";
    // 指针指向WSADATA数据结构，接受winsock实现的细节
    WSADATA wsaData;

    //WSAStartup必须是程序中第一个调用的winsocke函数，只有当其成功调用后才能使用进一步的winsocket函数
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if(iResult != 0)
    {
        //不成功将返回错误码，下面是几种可能的错误码
        //10022 WSAEINVAL 应用程序指出的Windows Sockets 版本不被该DLL 支持.
        //10092 WSAVERNOTSUPPORTED 所需的Windows Sockets API 的版本未由特定的Windows Sockets 实现提供.
        //10091 WSASYSNOTREADY 由WSAStartup()函数返回，表明底层的网络子系统无法使用。
        cout << "WinSock启动失败，错误码: " << iResult << endl;
        return false;
    }
    else
    {
        cout << "WinSock成功初始化。代理服务器成功启动" << endl;
        return true;
    }
}

//Forward
int ForwardResponse()
{
    //客户端socket出错，抛出异常
    if (send(client_sock, FR_recv_buf, sizeof(FR_recv_buf), 0) == SOCKET_ERROR)
    {
        cout << "向客户端send()出错，错误码: " << WSAGetLastError() << endl;
        closesocket(client_sock);
        return 0;
    }
    else
    {

//        cout << "向客户端发送如下数据：" << endl;
//        cout << FR_recv_buf << endl;
        cout << "向客户端send()成功.\n";
        memset(&FR_recv_buf,0,sizeof(FR_recv_buf));
        //go back to begginning again?
        Receive();
//        CreateThread(0,0,(LPTHREAD_START_ROUTINE)Receive, 0, 0,0);
    }
    return 1;
}

//Function to parse hostname from http request
//从rev_buf中寻找目的地的hostname
string ParseHostname(char * buf)
{
    size_t pos;

    //copy request to string for easier parsing
    string httpheader = buf;

    //string to hold hostname substring
    string hostname_t;


    //找到buffer中的Host所在行之后，pos + 6得到完整的hostname
    pos = httpheader.find("Host: ");
    hostname_t = httpheader.substr(pos + 6);

    //找到行尾的回车换行符\r\n
    pos = hostname_t.find("\r\n");

    //擦除行尾其他部分
    hostname_t.erase(pos);

    return hostname_t;
}

//Function to forward HTTP request from browser to webserver
// 将存在recv_buf中的数据转发给远程服务器
int ForwardRequest()
{
    int bytes_received;
    SOCKADDR_IN Dest;
    SOCKET frecv_sock;
    hostent *Host;

    cout << "开始处理客户端的转发请求" << endl;
    //获取buffer请求中的hostname
    string hostname = ParseHostname(recv_buf);

//  gethostbyname用于通过给定的hostname得到hostent结构，结构如下：
//    struct hostent
//    {
//        char FAR * h_name; /* official name of host */
//        char FAR * FAR * h_aliases; /* alias list */
//        short h_addrtype; /* host address type */
//        short h_length; /* length of address */
//        char FAR * FAR * h_addr_list; /* list of addresses */
//        #define h_addr h_addr_list[0] /* address, for backward compat，此处用于向后兼容*/
//    };
//  发生错误将返回空指针

    if((Host=gethostbyname(hostname.c_str()))==NULL)
    {
        //如果返回空指针，查找winsock报错原因进行如下处理
        DWORD dwError = WSAGetLastError();
        if (dwError != 0)
        {
            if(dwError == WSAHOST_NOT_FOUND)
            {
                cout << "客户端请求的Host主机： " << hostname.c_str()  << " 无法找到\n";
//                WSACleanup();
                return FALSE;
            }
            else if (dwError == WSANO_DATA)
            {
                cout << "有效的Host，但无法直接访问，通常由于DNS解析有误\n";;
//                WSACleanup();
                return FALSE;
            }
            else
            {
                cout << "查找远程服务器时发生严重错误，错误码： " << dwError << endl;
//                WSACleanup();
                return FALSE;
            }
        }
    }
    else
    {
        //如果返回的不是空指针，说明host指针获取成功
        cout << "成功获取到客户端请求中的HostName: " <<  hostname.c_str() << endl;
    }

    Dest.sin_family=AF_INET;
    cout<< "即将与远程服务器的80端口进行连接"<<endl;
    Dest.sin_port=htons(80);


    //如下命令从将从复制Host的h_addr到Dest的sin_addr，长度为address的长度
    memcpy(&Dest.sin_addr,Host->h_addr,Host->h_length);

    // Create a SOCKET for connecting to server
    //新建socket连接远程服务器
    if((frecv_sock = socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET)
    {
        cout << "建立与远程服务器的socket失败，错误码：" << WSAGetLastError() << endl;

        closesocket(frecv_sock);
        return FALSE;
    }

    // Connect to server
    //连接服务器
    // int PASCAL FAR connect( SOCKET s, const struct sockaddr FAR* name,int namelen);
    // 这里使用SOCKET_STREAM，使用名字与远程连接建立连接，成功调用则可以开始进行发送数据
    if(connect( frecv_sock,(SOCKADDR*)&Dest,sizeof(Dest))==SOCKET_ERROR)
    {
        cout << "与远程服务器connect()失败，错误码: " << WSAGetLastError() << endl;
        closesocket( frecv_sock);
        return FALSE;
    }

    //send intercepted request to server
    //已经与服务器建立了连接，可以开始发送数据
    //int PASCAL FAR send( SOCKET s, const char FAR* buf, int len, int flags);
    if (send(frecv_sock, recv_buf, strlen(recv_buf), 0) == SOCKET_ERROR)
    {
        cout << "向远程服务器send()失败,错误码: " << WSAGetLastError() << endl;
        closesocket(frecv_sock);
        return 0;
    }
    else
    {
        //成功向远程服务器发送数据，将会返回发送数据的总数
        cout << "向远程服务器send()成功，发送了"<< strlen(recv_buf)<< "Bytes"<<endl;
        memset(&recv_buf,0,sizeof(recv_buf));
    }

    //receive request from server
    //从远程服务器接收数据
    int i = 1;
    do
    {
        //接收数据，在FR_recv_buf中接受从远程服务器传送的数据，flags为标志位参数
        //int PASCAL FAR recv( SOCKET s, char FAR* buf, int len, int flags);
        bytes_received = recv(frecv_sock,FR_recv_buf,sizeof(FR_recv_buf) - 1,0);
        if (bytes_received > 0)
        {
            //没有错误发生，正常收到了数据，现在将开启一个子线程用来转发FR_recv_buf

            cout << "第"<< i <<"次: "<<"从远程服务器recv()成功,收到 " << bytes_received << " Bytes" << endl;
//            cout << "第" << i << "次从远程服务器recv()到的内容为：" << endl;
//            cout << FR_recv_buf << endl;
            cout << "第" << i << "次 建立线程将从远程客户端收到的响应写回客户端" << endl;
            strcat (FR_recv_buf, "\0");
            CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ForwardResponse, 0,0,0);

            i ++;


            //ForwardResponse();
        }
        else if ( bytes_received == 0 )
        {
            //连接已终止，返回0

            cout << "从远程服务器recv()完成，结束与远程服务器的连接\n";
            closesocket(frecv_sock);
        }
        else if ( bytes_received == SOCKET_ERROR)
        {
            cout << "从远程服务器recv()失败，错误码: " << WSAGetLastError() << endl;
            closesocket(frecv_sock);
            //WSACleanup();
            return 0;
        }
    }
    while (bytes_received > 0);
    return 1;
}

//Function to accept connection and receive data from browser
int Receive()
{
    SOCKADDR_IN csin;
    int csin_len = sizeof(csin);
    int iResult;

    //accept client connection
//  在一个套接口接受一个连接
//  listen_sock套接口描述字
//  csin为指向缓冲区的指针
//  指向csin长度的指针

    client_sock = accept(listen_sock, (LPSOCKADDR)&csin, &csin_len);
    //pauses here to wait for connection from client
    if (client_sock == INVALID_SOCKET)
    {
        cout << "accept()客户端连接出错: "<< WSAGetLastError() << endl;
        closesocket(client_sock);
        //WSACleanup();
        exit(0);
        return 1;
    }
    else
    {
        cout << "客户端已连接: " << inet_ntoa(csin.sin_addr) << ":" << csin.sin_port << endl;
    }
    //Start another thread to accept.
//  递归开启另一个客户端线程

    cout << "开启新的Receive()线程供客户端连接" << endl;

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Receive, 0, 0,0);

    do
    {
//      从一个套接口接受数据
//      client_sock表示已连接的套接口描述字
//      recv_buf用于接受数据的缓冲区
//      sizeof(recv_buf)缓冲区长度
//      0指定调用方式，读取tcp 缓冲区中的数据到buffer中，并从tcp 缓冲区中移除已读取的数据
//      无错误返回读入的字节数
        iResult = recv(client_sock, recv_buf, sizeof(recv_buf) - 1, 0);
        strcat(recv_buf, "\0");
        if (iResult == SOCKET_ERROR)
        {
            closesocket(client_sock);
            cout << "从客户端recv()出错，错误码: "<< WSAGetLastError() << endl;
        }
        else if (iResult > 0)
        {
//          将\0追加在recv_buffer后

            cout <<"从客户端recv()成功，共收到: " << iResult << "Bytes" <<endl;
            //forward HTTP request from browser to web server
            cout << recv_buf << endl;
            //此时recv_buf中已经存储了从客户端收到的请求信息，
            //现在需要将请求转发给目的地并接受响应，并写回给客户端
            //开启子线程处理这个过程
            HANDLE pChildThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ForwardRequest, 0, 0,0);
            WaitForSingleObject(pChildThread,6000);  //Wait for connection between proxy and remote server
            CloseHandle(pChildThread);
        }
        else if ( iResult == 0 )
        {
            cout << "从客户端recv()结束，断开与客户端的连接\n";
        }
    }
    while ( iResult > 0 );
    return 0;
}

//Function which listens for incoming connections to the proxy
int Listen()
{
    SOCKADDR_IN local;
//    socket地址
//    struct sockaddr_in
//    {
//        short sin_family;
//        u_short sin_port;
//        struct in_addr sin_addr;
//        char sin_zero[8];
//    };

//  内存空间初始化，将local指向的内存字节全部置为0，避免出现莫名奇妙的值，memset是一种对结构或数组清零的最快方法
    memset(&local,0,sizeof(local));

//  AF_INET指定TCP/IP协议簇
    local.sin_family = AF_INET;

//  htons将主机的16位无符号短整形数转换成网络字节顺序
    local.sin_port = htons(port);


//  如果一个应用并不关心分配给它的地址，则可将Internet 地址设置为INADDR_ANY，或将端口号置为0。
//  如果Internet 地址段为INADDR_ANY，则可使用任意网络接口；在有多种主机环境下可简化编程。
//  如果端口号置为0，则WINDOWS 套接口实现将给应用程序分配一个值在1024 到5000 之间的唯一的端口。
    local.sin_addr.s_addr = INADDR_ANY;


//  create socket for listening to
//  指定地址格式AF_INET，
//  数据类型为流式套接口（可靠的，双向的基于连接的字节流，使用带外数据传送机制），
//  0为协议缺省
//  成功将返回一个引用新套接口的描述字
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);

    //bind function associates a local address with a socket.
//  将一个socket与本地地址绑定
//  成功将返回0，错误将返回SOCKET_ERROR，可通过错误码来进行追踪
    if (bind(listen_sock, (LPSOCKADDR)&local, sizeof(local)) == 0)
    {
        cout << "服务端bind()成功" << endl;
//      创建一个套接口，并监听申请的连接
//      5表示等待连接队列的最大长度，默认为5，队列已满将返回WSAECONNREFUSED(10061)
        if (listen(listen_sock, 10) == 0)
        {
            cout << "服务端listen()成功" << endl;
            cout << "WinSock正在监听端口: " << port << endl;
        }
        else
        {
            cout << "无法监听端口.\n";
        }
    }
    else
    {
        cout << "服务端bind()出错，错误码: "<< WSAGetLastError() << endl;
    }

    //accept and start receiving data from browser
//    HANDLE CreateThread(
//        LPSECURITY_ATTRIBUTES   lpThreadAttributes, 内核安全属性
//        SIZE_T                  dwStackSize, 线程栈空间大小
//        LPTHREAD_START_ROUTINE  lpStartAddress, 新线程执行线程的函数地址
//        __drv_aliasesMem LPVOID lpParameter, 传递给线程函数的参数
//        DWORD                   dwCreationFlags, 控制线程运行的标识符，0表示在创建后立即运行
//        LPDWORD                 lpThreadId 返回线程的ID号
//    );

//  创建线程接收来自客户端的连接
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Receive, 0, 0,0);
    return 0;
}

int CloseServer()
{
    closesocket(client_sock);
    WSACleanup();
    return 1;
}

int _tmain(int argc, _TCHAR* argv[])
{
    InitializeWinsock();
    Listen();
    cin.get();
    CloseServer();
    return 0;
}


