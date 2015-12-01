/**
 * class: set messages to subscribers to an event list
 */
#ifndef ___ALERT
#define ___ALERT

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sock.h"

class PtgPlace;

class Alert : public Sock {
  private:
    PtgPlace*	thePlace;
    CString 	smsFileName;
    CString 	emailFileName;
    int		onlineCount;
    int		smsCount;
    int		emailCount;
    int		subscriberCount;
    UbqBool	usingJrServer;
    void	dumpStats( Db&, Event& );
    void	dumpMsg(const char *);
    void    	send( Db& db, Event& e, Subscriber& s);
    void    	broadcast( Event* e );
    void    	broadcastToJr( Event* e );
    UbqBool 	sendLocal( Event& e, Subscriber& s, UbqBool b, char* m );
    UbqBool 	sendToJr(  Event& e, Subscriber& s, UbqUchar p );
    void 	sendSms(   Event& e, Subscriber& s);
    void 	sendEmail( Event& e, Subscriber& s);

  public:
    Alert( PtgPlace* p, CString root, const char* s );
    ~Alert() {
    }
    void    poll( Db& );
};
#endif ___ALERT
