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
//�������LISTEN�˿�8080
int port = 8080;

SOCKET listen_sock;
SOCKET client_sock;

char FR_recv_buf [10240000] = "";
//char * FR_recv_buf = (char *) malloc(sizeof(999999999));
//char * FR_recv_buf = NULL;


char recv_buf [10240000] = "";

int Receive();
int Listen();

//��ʼ��winsock
bool InitializeWinsock()
{

    errorMap[10004] = "WSAEINTR Interrupted function call.";
    errorMap[10038] = "WSAENOTSOCK Socket operation on nonsocket.";
    errorMap[10093] = "WSANOTINITIALISED Successful WSAStartup not yet performed";
    // ָ��ָ��WSADATA���ݽṹ������winsockʵ�ֵ�ϸ��
    WSADATA wsaData;

    //WSAStartup�����ǳ����е�һ�����õ�winsocke������ֻ�е���ɹ����ú����ʹ�ý�һ����winsocket����
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if(iResult != 0)
    {
        //���ɹ������ش����룬�����Ǽ��ֿ��ܵĴ�����
        //10022 WSAEINVAL Ӧ�ó���ָ����Windows Sockets �汾������DLL ֧��.
        //10092 WSAVERNOTSUPPORTED �����Windows Sockets API �İ汾δ���ض���Windows Sockets ʵ���ṩ.
        //10091 WSASYSNOTREADY ��WSAStartup()�������أ������ײ��������ϵͳ�޷�ʹ�á�
        cout << "WinSock����ʧ�ܣ�������: " << iResult << endl;
        return false;
    }
    else
    {
        cout << "WinSock�ɹ���ʼ��������������ɹ�����" << endl;
        return true;
    }
}

//Forward
int ForwardResponse()
{
    //�ͻ���socket�����׳��쳣
    if (send(client_sock, FR_recv_buf, sizeof(FR_recv_buf), 0) == SOCKET_ERROR)
    {
        cout << "��ͻ���send()����������: " << WSAGetLastError() << endl;
        closesocket(client_sock);
        return 0;
    }
    else
    {

//        cout << "��ͻ��˷����������ݣ�" << endl;
//        cout << FR_recv_buf << endl;
        cout << "��ͻ���send()�ɹ�.\n";
        memset(&FR_recv_buf,0,sizeof(FR_recv_buf));
        //go back to begginning again?
        Receive();
//        CreateThread(0,0,(LPTHREAD_START_ROUTINE)Receive, 0, 0,0);
    }
    return 1;
}

//Function to parse hostname from http request
//��rev_buf��Ѱ��Ŀ�ĵص�hostname
string ParseHostname(char * buf)
{
    size_t pos;

    //copy request to string for easier parsing
    string httpheader = buf;

    //string to hold hostname substring
    string hostname_t;


    //�ҵ�buffer�е�Host������֮��pos + 6�õ�������hostname
    pos = httpheader.find("Host: ");
    hostname_t = httpheader.substr(pos + 6);

    //�ҵ���β�Ļس����з�\r\n
    pos = hostname_t.find("\r\n");

    //������β��������
    hostname_t.erase(pos);

    return hostname_t;
}

//Function to forward HTTP request from browser to webserver
// ������recv_buf�е�����ת����Զ�̷�����
int ForwardRequest()
{
    int bytes_received;
    SOCKADDR_IN Dest;
    SOCKET frecv_sock;
    hostent *Host;

    cout << "��ʼ����ͻ��˵�ת������" << endl;
    //��ȡbuffer�����е�hostname
    string hostname = ParseHostname(recv_buf);

//  gethostbyname����ͨ��������hostname�õ�hostent�ṹ���ṹ���£�
//    struct hostent
//    {
//        char FAR * h_name; /* official name of host */
//        char FAR * FAR * h_aliases; /* alias list */
//        short h_addrtype; /* host address type */
//        short h_length; /* length of address */
//        char FAR * FAR * h_addr_list; /* list of addresses */
//        #define h_addr h_addr_list[0] /* address, for backward compat���˴�����������*/
//    };
//  �������󽫷��ؿ�ָ��

    if((Host=gethostbyname(hostname.c_str()))==NULL)
    {
        //������ؿ�ָ�룬����winsock����ԭ��������´���
        DWORD dwError = WSAGetLastError();
        if (dwError != 0)
        {
            if(dwError == WSAHOST_NOT_FOUND)
            {
                cout << "�ͻ��������Host������ " << hostname.c_str()  << " �޷��ҵ�\n";
//                WSACleanup();
                return FALSE;
            }
            else if (dwError == WSANO_DATA)
            {
                cout << "��Ч��Host�����޷�ֱ�ӷ��ʣ�ͨ������DNS��������\n";;
//                WSACleanup();
                return FALSE;
            }
            else
            {
                cout << "����Զ�̷�����ʱ�������ش��󣬴����룺 " << dwError << endl;
//                WSACleanup();
                return FALSE;
            }
        }
    }
    else
    {
        //������صĲ��ǿ�ָ�룬˵��hostָ���ȡ�ɹ�
        cout << "�ɹ���ȡ���ͻ��������е�HostName: " <<  hostname.c_str() << endl;
    }

    Dest.sin_family=AF_INET;
    cout<< "������Զ�̷�������80�˿ڽ�������"<<endl;
    Dest.sin_port=htons(80);


    //��������ӽ��Ӹ���Host��h_addr��Dest��sin_addr������Ϊaddress�ĳ���
    memcpy(&Dest.sin_addr,Host->h_addr,Host->h_length);

    // Create a SOCKET for connecting to server
    //�½�socket����Զ�̷�����
    if((frecv_sock = socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET)
    {
        cout << "������Զ�̷�������socketʧ�ܣ������룺" << WSAGetLastError() << endl;

        closesocket(frecv_sock);
        return FALSE;
    }

    // Connect to server
    //���ӷ�����
    // int PASCAL FAR connect( SOCKET s, const struct sockaddr FAR* name,int namelen);
    // ����ʹ��SOCKET_STREAM��ʹ��������Զ�����ӽ������ӣ��ɹ���������Կ�ʼ���з�������
    if(connect( frecv_sock,(SOCKADDR*)&Dest,sizeof(Dest))==SOCKET_ERROR)
    {
        cout << "��Զ�̷�����connect()ʧ�ܣ�������: " << WSAGetLastError() << endl;
        closesocket( frecv_sock);
        return FALSE;
    }

    //send intercepted request to server
    //�Ѿ�����������������ӣ����Կ�ʼ��������
    //int PASCAL FAR send( SOCKET s, const char FAR* buf, int len, int flags);
    if (send(frecv_sock, recv_buf, strlen(recv_buf), 0) == SOCKET_ERROR)
    {
        cout << "��Զ�̷�����send()ʧ��,������: " << WSAGetLastError() << endl;
        closesocket(frecv_sock);
        return 0;
    }
    else
    {
        //�ɹ���Զ�̷������������ݣ����᷵�ط������ݵ�����
        cout << "��Զ�̷�����send()�ɹ���������"<< strlen(recv_buf)<< "Bytes"<<endl;
        memset(&recv_buf,0,sizeof(recv_buf));
    }

    //receive request from server
    //��Զ�̷�������������
    int i = 1;
    do
    {
        //�������ݣ���FR_recv_buf�н��ܴ�Զ�̷��������͵����ݣ�flagsΪ��־λ����
        //int PASCAL FAR recv( SOCKET s, char FAR* buf, int len, int flags);
        bytes_received = recv(frecv_sock,FR_recv_buf,sizeof(FR_recv_buf) - 1,0);
        if (bytes_received > 0)
        {
            //û�д������������յ������ݣ����ڽ�����һ�����߳�����ת��FR_recv_buf

            cout << "��"<< i <<"��: "<<"��Զ�̷�����recv()�ɹ�,�յ� " << bytes_received << " Bytes" << endl;
//            cout << "��" << i << "�δ�Զ�̷�����recv()��������Ϊ��" << endl;
//            cout << FR_recv_buf << endl;
            cout << "��" << i << "�� �����߳̽���Զ�̿ͻ����յ�����Ӧд�ؿͻ���" << endl;
            strcat (FR_recv_buf, "\0");
            CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ForwardResponse, 0,0,0);

            i ++;


            //ForwardResponse();
        }
        else if ( bytes_received == 0 )
        {
            //��������ֹ������0

            cout << "��Զ�̷�����recv()��ɣ�������Զ�̷�����������\n";
            closesocket(frecv_sock);
        }
        else if ( bytes_received == SOCKET_ERROR)
        {
            cout << "��Զ�̷�����recv()ʧ�ܣ�������: " << WSAGetLastError() << endl;
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
//  ��һ���׽ӿڽ���һ������
//  listen_sock�׽ӿ�������
//  csinΪָ�򻺳�����ָ��
//  ָ��csin���ȵ�ָ��

    client_sock = accept(listen_sock, (LPSOCKADDR)&csin, &csin_len);
    //pauses here to wait for connection from client
    if (client_sock == INVALID_SOCKET)
    {
        cout << "accept()�ͻ������ӳ���: "<< WSAGetLastError() << endl;
        closesocket(client_sock);
        //WSACleanup();
        exit(0);
        return 1;
    }
    else
    {
        cout << "�ͻ���������: " << inet_ntoa(csin.sin_addr) << ":" << csin.sin_port << endl;
    }
    //Start another thread to accept.
//  �ݹ鿪����һ���ͻ����߳�

    cout << "�����µ�Receive()�̹߳��ͻ�������" << endl;

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Receive, 0, 0,0);

    do
    {
//      ��һ���׽ӿڽ�������
//      client_sock��ʾ�����ӵ��׽ӿ�������
//      recv_buf���ڽ������ݵĻ�����
//      sizeof(recv_buf)����������
//      0ָ�����÷�ʽ����ȡtcp �������е����ݵ�buffer�У�����tcp ���������Ƴ��Ѷ�ȡ������
//      �޴��󷵻ض�����ֽ���
        iResult = recv(client_sock, recv_buf, sizeof(recv_buf) - 1, 0);
        strcat(recv_buf, "\0");
        if (iResult == SOCKET_ERROR)
        {
            closesocket(client_sock);
            cout << "�ӿͻ���recv()����������: "<< WSAGetLastError() << endl;
        }
        else if (iResult > 0)
        {
//          ��\0׷����recv_buffer��

            cout <<"�ӿͻ���recv()�ɹ������յ�: " << iResult << "Bytes" <<endl;
            //forward HTTP request from browser to web server
            cout << recv_buf << endl;
            //��ʱrecv_buf���Ѿ��洢�˴ӿͻ����յ���������Ϣ��
            //������Ҫ������ת����Ŀ�ĵز�������Ӧ����д�ظ��ͻ���
            //�������̴߳����������
            HANDLE pChildThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ForwardRequest, 0, 0,0);
            WaitForSingleObject(pChildThread,6000);  //Wait for connection between proxy and remote server
            CloseHandle(pChildThread);
        }
        else if ( iResult == 0 )
        {
            cout << "�ӿͻ���recv()�������Ͽ���ͻ��˵�����\n";
        }
    }
    while ( iResult > 0 );
    return 0;
}

//Function which listens for incoming connections to the proxy
int Listen()
{
    SOCKADDR_IN local;
//    socket��ַ
//    struct sockaddr_in
//    {
//        short sin_family;
//        u_short sin_port;
//        struct in_addr sin_addr;
//        char sin_zero[8];
//    };

//  �ڴ�ռ��ʼ������localָ����ڴ��ֽ�ȫ����Ϊ0���������Ī�������ֵ��memset��һ�ֶԽṹ�������������췽��
    memset(&local,0,sizeof(local));

//  AF_INETָ��TCP/IPЭ���
    local.sin_family = AF_INET;

//  htons��������16λ�޷��Ŷ�������ת���������ֽ�˳��
    local.sin_port = htons(port);


//  ���һ��Ӧ�ò������ķ�������ĵ�ַ����ɽ�Internet ��ַ����ΪINADDR_ANY���򽫶˿ں���Ϊ0��
//  ���Internet ��ַ��ΪINADDR_ANY�����ʹ����������ӿڣ����ж������������¿ɼ򻯱�̡�
//  ����˿ں���Ϊ0����WINDOWS �׽ӿ�ʵ�ֽ���Ӧ�ó������һ��ֵ��1024 ��5000 ֮���Ψһ�Ķ˿ڡ�
    local.sin_addr.s_addr = INADDR_ANY;


//  create socket for listening to
//  ָ����ַ��ʽAF_INET��
//  ��������Ϊ��ʽ�׽ӿڣ��ɿ��ģ�˫��Ļ������ӵ��ֽ�����ʹ�ô������ݴ��ͻ��ƣ���
//  0ΪЭ��ȱʡ
//  �ɹ�������һ���������׽ӿڵ�������
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);

    //bind function associates a local address with a socket.
//  ��һ��socket�뱾�ص�ַ��
//  �ɹ�������0�����󽫷���SOCKET_ERROR����ͨ��������������׷��
    if (bind(listen_sock, (LPSOCKADDR)&local, sizeof(local)) == 0)
    {
        cout << "�����bind()�ɹ�" << endl;
//      ����һ���׽ӿڣ����������������
//      5��ʾ�ȴ����Ӷ��е���󳤶ȣ�Ĭ��Ϊ5����������������WSAECONNREFUSED(10061)
        if (listen(listen_sock, 10) == 0)
        {
            cout << "�����listen()�ɹ�" << endl;
            cout << "WinSock���ڼ����˿�: " << port << endl;
        }
        else
        {
            cout << "�޷������˿�.\n";
        }
    }
    else
    {
        cout << "�����bind()����������: "<< WSAGetLastError() << endl;
    }

    //accept and start receiving data from browser
//    HANDLE CreateThread(
//        LPSECURITY_ATTRIBUTES   lpThreadAttributes, �ں˰�ȫ����
//        SIZE_T                  dwStackSize, �߳�ջ�ռ��С
//        LPTHREAD_START_ROUTINE  lpStartAddress, ���߳�ִ���̵߳ĺ�����ַ
//        __drv_aliasesMem LPVOID lpParameter, ���ݸ��̺߳����Ĳ���
//        DWORD                   dwCreationFlags, �����߳����еı�ʶ����0��ʾ�ڴ�������������
//        LPDWORD                 lpThreadId �����̵߳�ID��
//    );

//  �����߳̽������Կͻ��˵�����
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


