#ifndef NETLAYER_H_INCLUDED
#define NETLAYER_H_INCLUDED

#include <iostream>
#include <cstring>
#include <stdint.h>
#ifdef LINUX
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <unistd.h>
#elif WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <Ws2tcpip.h>
    #include <winsock2.h>
    #include <Windows.h>
#endif

using namespace std;

class BaseSocket
{
    friend class Multiplexer;
private:
    void Close();
protected:
    addrinfo *m_addr;
    addrinfo m_target;
    sockaddr m_peer;
    int m_bufsize;
    int m_sockfd;
    bool m_type;
    BaseSocket() {}
    BaseSocket(bool type,const string host,const string port);
    ~BaseSocket();
    void Bind();
public:
    void SetRecvTimeOut(int sec);;
    string GetPeerAddr();
    uint16_t GetPeerPortNumber();
    string GetPeerPortString();
    string GetHostAddr();
    uint16_t GetHostPortNumber();
};

#ifdef FD_CLR
#undef FD_CLR
#endif

#define FD_CLR(fd,set)							\
  do {									\
	unsigned __i;							\
	for(__i = 0; __i < ((fd_set *)(set))->fd_count; __i++) {	\
		if (((fd_set *)(set))->fd_array[__i] == (unsigned)fd) {		\
			while (__i < ((fd_set *)(set))->fd_count - 1) {	\
				((fd_set *)(set))->fd_array[__i] =	\
				 ((fd_set *)(set))->fd_array[__i + 1];	\
				__i++;					\
			}						\
			((fd_set *)(set))->fd_count--;			\
			break;						\
		}							\
	}								\
  } while(0)

class Multiplexer
{
private:
    fd_set m_set;
    timeval m_time;
public:
    Multiplexer(long sec=0,long usec=0){m_time.tv_sec=sec;m_time.tv_usec=usec;Zero();}
    void Clear(BaseSocket&socket){FD_CLR(socket.m_sockfd,&m_set);}
    void Zero(){FD_ZERO(&m_set);}
    void Add(BaseSocket&socket){FD_SET(socket.m_sockfd,&m_set);}
    bool Check(BaseSocket&socket){return FD_ISSET(socket.m_sockfd,&m_set);}
    int Select(){return select(0,&m_set,NULL,NULL,NULL);}
};

class UDPSocket:public BaseSocket
{
public:
    UDPSocket(const string host,const string port);
    void Bind()
    {
        BaseSocket::Bind();
    }
    int RecvFrom(char*buf,int size,int flag=0,sockaddr *peer=NULL);
    int RecvFromAll(char*buf,int size,int flag=0,sockaddr *peer=NULL);
    int SendTo(const char*buf,int size,int flag=0,const sockaddr *peer=NULL);
    int SendTo(const string&str,int flag=0,const sockaddr *peer=NULL);
};

class TCPSocket:public BaseSocket
{
    int m_queue;
protected:
    TCPSocket(int sockfd,sockaddr t,int queue=16);
public:
    TCPSocket(const string host,const string port,int queue=16);
    void Bind()
    {
        BaseSocket::Bind();
    }
    TCPSocket* Accept();
    bool Listen();
    bool Connect();
    int Recv(char*buff,int size,int flag=0);
    int Send(const char*buff,int size,int flag=0);
    int Send(const string&string,int flag=0);
    void Shutdown(int stat=0)
    {
        shutdown(m_sockfd,stat);
    }
};

class NetHelper
{
private:

public:
    static void Init();
    static void Cleanup();
    static int GetLastError();
    //static bool Compress(char*out,long unsigned int*outlen,const char*in,long unsigned int inlen);
    //static bool Uncompress(char*out,long unsigned int*outlen,const char*in,long unsigned int inlen);
    static string GetMyPrimaryIP();
    static string GetBroadcastIP();
};



#endif // NETLAYER_H_INCLUDED
