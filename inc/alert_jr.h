/**
 * class: set messages to subscribers to an event list - junior version
 *
 * The main event server handles lists of events and subscribers. The
 * junior verion is called iteratively for single event/subsciber pairs,
 * so there is no need to maintain event lists and subscribers lists.
 * All info about the event and subscriber are contained in this object.
 */
#ifndef ___ALERTJR
#define ___ALERTJR

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sock.h"

class PtgPlace;

class AlertJr : public Sock {
  private:
    PtgPlace*	thePlace;

    // event
    CString	description;
    CString	sender;
    CString	message;
    UbqUchar	type;
    char	buff[1024];

    // subscriber
    CString	name;
    UbqUchar	prefs;
    UbqUchar	fmtFlag;

    UbqBool	sendLocal(CString name, UbqBool doBuddy);
    void	broadcast();

    UbqBool	getEvent();
    void	ackEvent(UbqBool);
    int		readStr(Sock* s);

  public:
    AlertJr( PtgPlace* p, CString root );
    ~AlertJr() {
    }
    void	sendAlert();
    UbqUchar	getType() { return type; };
    char*	getMsg() { return buff; };
    PtgPlace*	getPlace() { return thePlace; };
};
#endif ___ALERTJR
