/*
 * Class: Socket handling. 
 */
#ifndef ___Sock
#define ___Sock

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/filio.h>
#include <unistd.h>

#define IS_TRANSIENT_ERRNO(sysErr) \
   ((sysErr == EWOULDBLOCK) || (sysErr == EINTR))

class Sock {
  private:
    struct sockaddr_in	listenAddr, msgAddr;	
    int		listenFd, msgFd;
    int		ip;
    int		port;

    void trace(char, unsigned char *, int);

  public:
    Sock();
    Sock(const char* jrServerName);
    ~Sock() {
    }

    char	buff[1024];

    int  doConnect();
    int  recvData(char*, int);
    int  sendData(char*, int);
    void doAccept();
    int  getListenFd() { return listenFd; };
    int  getMsgFd() { return msgFd; };
    void doClose() { close(msgFd); msgFd = -1; };

};

#endif
