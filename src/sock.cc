/*
 * Common socket I/O stuff
 *
 * The listener keeps track of two sockets, one for listening and
 * one for exchanging messages.
 *
 * The sender just has one socket, for exchanging messages.
 *
 * Send and Receive operations implicitly use the message socket.
 */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <UbqCtrl.h>
#include "sock.h"

extern UbqBool debug;

/**
 * Constructor for the listener
 */
Sock::Sock()
{
	UbqControlUlong c("VPALERT_PORT", 2222);
	port = c.getVal();

	if ((listenFd = socket(AF_INET, SOCK_STREAM,0)) < 0) {
	  fprintf(stderr, "Errno: %d - can't open stream socket\n", errno);
	  exit(1);
	}

	memset((char *)&listenAddr, 0, sizeof(listenAddr));
	listenAddr.sin_family      = AF_INET;
	listenAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	listenAddr.sin_port        = htons(port);

	int i = 1;
	struct linger l;
	l.l_onoff = 1;
	l.l_linger = 90;	// non-zero is usually good enough
	if ((setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (char*)&i, sizeof(i)) == -1) ||
	    (setsockopt(listenFd, SOL_SOCKET, SO_LINGER, (char*)&l, sizeof(l)) == -1) ||
	    (bind(listenFd, (struct sockaddr*)&listenAddr, sizeof(listenAddr)) < 0)) {
	  fprintf(stderr, "Errno: %d - can't bind local address\n", errno);
	  exit(1);
	}
	listen(listenFd, SOMAXCONN);
	fprintf(stderr, "Listening on port %d\n", port);
	msgFd = -1;
}

/**
 * Constructor for the sender.
 */
Sock::Sock( const char* jrServerName )
{
   if (strlen(jrServerName) == 0) {
	fprintf(stderr, "Not using a secondary server\n");
	port = 0;
   }
   else {
	UbqControlUlong c("VPALERT_PORT", 2222);
	port = c.getVal();
	fprintf(stderr, "Using secondary server %s on port %d\n", jrServerName, port);

	ip = inet_addr(jrServerName);
	struct hostent *hp;
	if (ip == -1) {
		hp = gethostbyname(jrServerName);
		if (hp == NULL) {
		   fprintf(stderr, "Warning: Lookup failure for %s\n", jrServerName);
		   ip = 0;
		}
		else 
		   ip = ((struct in_addr *)(hp->h_addr))->s_addr;
	}
	else {
		ip = ((struct in_addr *)(hp->h_addr))->s_addr;
	}
	ip = ntohl(ip);
   }
   msgFd = -1;
}

/**
 * Connect to the junior alert server
 */
int
Sock::doConnect()
{
   if (msgFd > -1) {
	return msgFd;
   }

   if (port == 0) {
	return -2;	// not using a JR server
   }

   memset((char *)&msgAddr, 0, sizeof(msgAddr));
   msgAddr.sin_addr.s_addr = htonl(ip);
   msgAddr.sin_family	= AF_INET;
   msgAddr.sin_port	= htons(port);

   msgFd = socket(AF_INET, SOCK_STREAM, 0);

   fprintf(stderr, "Attempting to connect to JR server ...\n");
   if (msgFd >= -1) {
	if (connect(msgFd, (struct sockaddr*)&msgAddr, sizeof(msgAddr)) > -1) {
		fprintf(stderr, "Connected to junior server\n");
	}
	else {
		fprintf(stderr, "Connection to junior server failed\n");
		msgFd = -1;
	}
   }
   else {
	fprintf(stderr, "Open failed\n");
   }
   if (msgFd > -1)
        fprintf(stderr,"Connected on port: %d IP: %s\n", port, ubqGetIpStr(ip));
   return msgFd;
}

/**
 * Receive data from a socket.
 */
int
Sock::recvData(char* ptr, int nbytes)
{
	char *tp = ptr;
	int nleft = nbytes;
	int received = 0;
	while (nleft > 0) {
	  errno = 0;
	  int nrecv = recv(msgFd, ptr, nleft, 0);
	  if (nrecv == -1) {
	    if(errno == EINTR) {
		continue;
	    }
	    else {
		//
		// Read from a non-blocking socket, and
		// there's no more data, though the connection
		// is still open.
		//
		nbytes = received;
		break;
	    }
	  }
	  if (nrecv > nleft)
	    return -1;
	  received += nrecv;
	  if (nrecv > 0) {
	    nleft -= nrecv;
	    ptr   += nrecv;
	  }
	  if (nrecv == 0) {
	    //
	    // Read from a non-blocking socket, and
	    // there no more data, and the connection is closed.
	    //
	    nbytes = received;
	    break;
	  }
	}
	trace( 'R', (unsigned char *)tp, nbytes);
	return nbytes;
}

/**
 * Send data to a socket
 */
int
Sock::sendData(char* ptr, int nbytes)
{
	trace( 'S', (unsigned char *)ptr, nbytes);
	int nleft = nbytes;
	while (nleft > 0) {
	  int nsent = send(msgFd, ptr, nleft, 0);
	  if (nsent > nleft) 
	    return -1;
	  if (nsent > 0) {
	    nleft -= nsent;
	    ptr   += nsent;
	  }
	  else if (!(nsent == -1 && IS_TRANSIENT_ERRNO(errno)))
	    return nsent;
	}
	return nbytes;
}

/**
 * debug trace
 */
void
Sock::trace (char direction, unsigned char *p, int bytes)
{
	if (!debug)
		return;

	int f, l, i;
	unsigned char d[16];
	f = 0;

	while (bytes >= 16) {
		for (i=0; i<16; i++) {
			d[i] = isprint(p[i]) ? p[i] : '.';
		}
		l = f+15;
		fprintf(stderr, "%4.4X - %4.4X %c %2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X - %2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", f, l, direction, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
		f += 16;
		p += 16;
		bytes -= 16;
	}
	if (bytes > 0) {
		l = f + bytes - 1;
		fprintf(stderr, "%4.4X - %4.4X %c ", f, l, direction);
		for (i = 0; i < bytes; i++) {
			fprintf(stderr, "%2.2X", p[i]);
			d[i] = isprint(p[i]) ? p[i] : '.';
			if (i == 7)
				fprintf(stderr, " - ");
		}
		for (i = 0; i < 32-(bytes*2); i++)
			fprintf(stderr, " ");
		if (bytes<8)
			fprintf(stderr, "   ");
		fprintf(stderr, "  ");
		for (i = 0; i < bytes; i++) {
			fprintf(stderr, "%c", d[i]);
		}
		fprintf(stderr, "\n");
	}
}
/**
 * Accept an incoming connection
 */
void
Sock::doAccept()
{
	if (msgFd > -1) {
		fprintf(stderr, "Got second connection, dropping old one\n");
		close(msgFd);	// only handle one connection at a time
		msgFd = -1;
	}

	int msgLen = sizeof(msgAddr);
	while (msgFd <= 0) {
		msgFd = accept(listenFd, (struct sockaddr*)&msgAddr, &msgLen);
		if (!(msgFd == -1 && IS_TRANSIENT_ERRNO(errno)))
			break;
	}
	if (msgFd < 0) {
		fprintf(stderr, "ERROR: Accept failed, returned: %d for fd: %d - errno: %d\n", msgFd, listenFd, errno);
		return;
	} else {
		char IPaddr[32];
		strcpy(IPaddr, inet_ntoa(msgAddr.sin_addr));
		fprintf(stderr, "Accepted connection from: %s\n", IPaddr);
	}
	return;
}
