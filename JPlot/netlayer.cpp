#include "netlayer.h"
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace std;

// BaseSocket Section
//
//
#ifdef WIN32
#define itoa _itoa
#elif LINUX
#include <sys/socket.h>
char* itoa(int value, char*buf, int bufsize)
{
	sprintf(buf,"%d",value);
	return buf;
}
#endif

bool BaseSocket::Initialized = false;

BaseSocket::BaseSocket(bool type,const string host,const string port)
    :m_type(type)
{
	InitializeOnce();
    addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = type?SOCK_STREAM:SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP


    int ret;
    if ((ret = getaddrinfo(host.length()>0?host.c_str():NULL,port.c_str(),&hints,&m_addr)) != 0)
    {
        cerr<< "getaddrinfo: "<< gai_strerror(ret)<<endl;
        throw;
    }
    for(addrinfo*p = m_addr; p != NULL; p = p->ai_next)
    {
        m_target=*p;
        if ((m_sockfd = socket(p->ai_family, p->ai_socktype,
                               p->ai_protocol)) == -1)
        {
            cerr<<"server: socket"<<endl;
            continue;
        }

        // We are not listening, enough
        if(host.length()>0)
            break;

        if (bind(m_sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            Close();
            perror("server: bind");
            continue;
        }
        Close();
        m_sockfd = socket(p->ai_family, p->ai_socktype,
                          p->ai_protocol);
        //int m=sizeof m_bufsize;
        //getsockopt(m_sockfd,SOL_SOCKET,SO_RCVBUF,(char *)&m_bufsize, &m);
#ifdef LINUX
		int set = 1;
		setsockopt(m_sockfd, SOL_SOCKET, MSG_NOSIGNAL, (void *)&set, sizeof(int));
#endif
        break;
    }
}

BaseSocket::~BaseSocket()
{
    if(!m_addr)
        freeaddrinfo(m_addr);
    Close();
}

void BaseSocket::SetRecvTimeOut(int sec)
{
    struct timeval tv;
#ifdef WIN32
    tv.tv_sec = sec*1000;
#else
    tv.tv_sec = sec;
#endif
    tv.tv_usec = 0;  // Not init'ing this can cause strange errors

    setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
}

void BaseSocket::Close()
{
#ifdef WIN32
    closesocket(m_sockfd);
#else
    close(m_sockfd);
#endif
}

int BaseSocket::Bind()
{
    if (bind(m_sockfd, m_target.ai_addr, m_target.ai_addrlen) == -1)
    {
#ifdef WIN32
        closesocket(m_sockfd);
#elif LINUX
        close(m_sockfd);
#endif
		return 0;
    }
	return 1;
}

string BaseSocket::GetPeerAddr()
{
    sockaddr host;
    socklen_t len=sizeof(host);
    getpeername(m_sockfd,&host,&len);
    return string(inet_ntoa(((sockaddr_in*)&host)->sin_addr));
}

uint16_t BaseSocket::GetPeerPortNumber()
{
    sockaddr host;
    socklen_t len=sizeof(host);
    getpeername(m_sockfd,&host,&len);
    return ntohs(((sockaddr_in*)&host)->sin_port);
}

string BaseSocket::GetPeerPortString()
{
    char buf[16];
    itoa(GetPeerPortNumber(),buf,16);
    return string(buf);
}

string BaseSocket::GetHostAddr()
{
    sockaddr host;
    socklen_t len=sizeof(host);
    getsockname(m_sockfd,&host,&len);
    return string(inet_ntoa(((sockaddr_in*)&host)->sin_addr));
}

uint16_t BaseSocket::GetHostPortNumber()
{
    sockaddr host;
    socklen_t len=sizeof(host);
    getsockname(m_sockfd,&host,&len);
    return ntohs(((sockaddr_in*)&host)->sin_port);
}

// UDPSocket Section
//
//

UDPSocket::UDPSocket(const string host,const string port)
    :BaseSocket(false,host,port)
{
    int a=1;
    setsockopt(m_sockfd,SOL_SOCKET,SO_BROADCAST,(const char*)&a,sizeof(int));
}

int UDPSocket::RecvFrom(char*buf,int size,int flag,sockaddr *peer)
{
    sockaddr remote;
    socklen_t len=sizeof remote;
    int ret=recvfrom(m_sockfd,buf,size,flag,peer?peer:&remote,&len);
    return ret;
}

int UDPSocket::RecvFromAll(char*buf,int size,int flag,sockaddr *peer)
{
    cerr<<"RecvFromAll() not implemented\n";
    return 0;
}

int UDPSocket::SendTo(const char*buf,int size,int flag,const sockaddr*peer)
{
    int ret=sendto(m_sockfd,buf,size,flag,peer?peer:m_target.ai_addr,peer?sizeof *peer:m_target.ai_addrlen);
    return ret;
}

int UDPSocket::SendTo(const string&str,int flag,const sockaddr*peer)
{
    return SendTo(str.c_str(),str.length()+1,flag,peer);
}

// TCPSocket Section
//
//

TCPSocket::TCPSocket(const string host,const string port,int queue)
    :BaseSocket(true,host,port)
{
    m_queue=queue;
}

TCPSocket::TCPSocket(int sockfd,sockaddr t,int queue)
{
    m_sockfd=sockfd;
    m_peer=t;
    m_queue=queue;
}

TCPSocket* TCPSocket::Accept()
{
    sockaddr peer;
    socklen_t len=sizeof peer;
    int peersock=accept(m_sockfd,&peer,&len);
    if(-1==peersock)
    {
        cerr<<"NetHelper::GetLastError: "<<NetHelper::GetLastError()<<endl;
        perror("accept");
        return NULL;
    }
    return new TCPSocket(peersock,peer);
}

bool TCPSocket::Listen()
{
    int ret=listen(m_sockfd, m_queue);
    return ret==0;
}

bool TCPSocket::Connect()
{
    if(-1==connect(m_sockfd, m_target.ai_addr, m_target.ai_addrlen))
    {
        return 0;
    }
    return 1;
}

int TCPSocket::Recv(char*buff,int size,int flag)
{
    int ret=recv(m_sockfd,buff,size,0);
    return ret;
}

int TCPSocket::Send(const char*buff,int size,int flag)
{
    int ret=send(m_sockfd,buff,size,flag);
    return ret;
}

int TCPSocket::Send(const string&msg,int flag)
{
    return Send(msg.c_str(),msg.length()+1,flag);
}


// NetHelper Section
//
//


void NetHelper::Init()
{
#ifdef WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
    {
        /* Tell the user that we could not find a usable */
        /* Winsock DLL.                                  */
        cerr<<"WSAStartup failed with error: "<< err<<endl;
        exit(1);
    }

    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions greater    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        cerr<<"Could not find a usable version of Winsock.dll\n";
        WSACleanup();
        exit(1);
    }
#endif

}

void NetHelper::Cleanup()
{
#ifdef WIN32
    WSACleanup();
#endif

}

int NetHelper::GetLastError()
{
#ifdef WIN32
    return WSAGetLastError();
#endif
}
/*
bool NetHelper::Compress(char*out,long unsigned int*outlen,const char*in,long unsigned int inlen)
{
    return Z_OK==compress2((Bytef*)out,outlen,(const Bytef*)in,inlen,2);
}

bool NetHelper::Uncompress(char*out,long unsigned int*outlen,const char*in,long unsigned int inlen)
{
    return Z_OK==uncompress((Bytef*)out,outlen,(const Bytef*)in,inlen);
}
*/
in_addr* GetMyIPptr()
{
    char ac[80];
    if (gethostname(ac, sizeof(ac)) == -1)
    {
        cerr << "Error " << NetHelper::GetLastError() <<
             " when getting local host name." << endl;
        return NULL;
    }
    struct hostent *phe = gethostbyname(ac);
    if (phe == 0)
    {
        cerr << "Yow! Bad host lookup." << endl;
        return NULL;
    }
    for (int i = 0; phe->h_addr_list[i] != 0; ++i)
    {
        return (in_addr*)phe->h_addr_list[i];
    }
    return NULL;
}

string NetHelper::GetMyPrimaryIP()
{
    return string(inet_ntoa(*GetMyIPptr()));
}

string NetHelper::GetBroadcastIP()
{/*
    in_addr ip=*GetMyIPptr();
    ip.S_un.S_un_b.s_b4=255;
    return string(inet_ntoa(ip));
*/
		throw;
		return "";
}
