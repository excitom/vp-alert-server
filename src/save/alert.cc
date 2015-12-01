/*
 * Alert a subscriber to a list that an event has occured.
 *
 */
#include <ubique.h>
#include <vpTypes.h>
#include <CString.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <glib.h>

#include "db.h"
#include "alert.h"
#include "event.h"
#include "users.h"
#include "place.h"
#include "subscriber.h"

///////////////////////////
// Alert class 
///////////////////////////

Alert::Alert( PtgPlace* p, CString root, const char* jrServerName ) : Sock( jrServerName )
{
   thePlace = p;
   emailFileName = root + "/email.pending";
   smsFileName   = root + "/sms.pending";
   onlineCount = smsCount = emailCount = subscriberCount = 0;
}

/**
 * Poll the database for events
 */
void
Alert::poll( Db& db )
{
   GList* eventList = db.getEvents();
   while( eventList ) {
      Event* e = (Event*)(eventList->data);
      if (e->isBroadcast()) {
         broadcast( e );
      }
      else {
        GList* subscrList = db.getSubscribers( *e );
        while( subscrList ) {
           Subscriber* s = (Subscriber*)(subscrList->data);
           send( db, *e, *s );
           subscrList = g_list_remove( subscrList, (gpointer)s );
           delete s;
        }
        dumpStats( db, *e );
      }
      eventList = g_list_remove( eventList, (gpointer)e );
      delete e;
   }
}

/** 
 * Send a broadcast message.
 *
 * This function is divided into two parts. The 'broadcast' method calls
 * the user record iterator method, passing it a function to call (and 
 * data to pass to that function) for each online user.
 *
 * The passed function is 'broadcastMessage', and it expects the iterator
 * to pass it the name and user record pointer for each user, as well as
 * the data passed from the 'broadcast' method (which is the event object).
 */
void
broadcastMsg( const char* n, UserRecord* u, Event* e )
{
   UbqBool send = UBQ_FALSE;
   if (VP_IS_A_BL_PRESENCE(u->getRole())) {
	if (BROADCAST_BUDDY(e->getType()))
	   send = UBQ_TRUE;
   }
   else {
	if (BROADCAST_CHAT(e->getType()))
	   send = UBQ_TRUE;
   }
   if (send) {
	char m[1024];
   	sprintf(m, "Broadcast message from %s: %s",
			e->getSender(),
			e->getMessage());
	VpErrCode rc = e->getPlace()->whisper(u->getPresenceId(), m, UbqOpaque(0, NULL));
   }
}

void
Alert::broadcast( Event* e )
{
   e->setPlace( thePlace );
   UbqUchar type = e->getType();
   if (BROADCAST_VPCHAT(type)) {
	UserRecord::iterate( (GHFunc)broadcastMsg, (gpointer)e );
	dumpMsg("Broadcast sent");
   }
   if (BROADCAST_VPADULT(type)) {
	//
	// Connect to the junior server and send the broadcast msg,
	// which will be propagated to the online users of that community.
	//
	broadcastToJr(e);
   }
}

/**
 * Send an alert message to a subscriber
 */
void
Alert::send( Db& db, Event& e, Subscriber& s )
{
	subscriberCount++;
	UbqUchar prefs = s.getNotifyPrefs();
	UbqBool sent = UBQ_FALSE;
	UbqBool doBuddy;
	char m[1024];
        if (e.getTournID() == 0) {
		sprintf(m, "Message from %s to the %s list: %s",
				e.getSender(),
				e.getDescription(),
				e.getMessage());
	}
	else {
		strcpy(m, e.getMessage());
	}
	if( IM_VPCHAT(prefs) ) {		// user wants msg to vpchat
	  if( IM_CHAT(prefs) ) {		// user wants a chat IM
            doBuddy = UBQ_FALSE;
            if (sendLocal( e, s, doBuddy, m ))
		sent = UBQ_TRUE;
	  }
	  if( IM_BUDDY(prefs) ) {		// user wants a buddy IM
            doBuddy = UBQ_TRUE;
            if (sendLocal( e, s, doBuddy, m ))
		sent = UBQ_TRUE;
	  }
	}
	if( IM_VPADULT(prefs) ) {		// user wants msg to vpadult
          if (sendToJr( e, s, prefs )) {
		sent = UBQ_TRUE;
	  }
	}
	if (sent) {
	   onlineCount++;
	}
	else {
	  if (OFFL_WANTED(prefs)) {
	    if( db.getOfflinePrefs( s ) ) {
		if( OFFL_SMS(prefs) ) {
        	  sendSms( e, s );
		}
		if( OFFL_EMAIL(prefs) ) {
        	  sendEmail( e, s );
		}
	    }
	  }
	}
}

/**
 * Attempt to send an IM to user in vpchat
 */
UbqBool
Alert::sendLocal( Event& e, Subscriber& s, UbqBool doBuddy, char* m )
{
	CString name(s.getName());
	if (doBuddy) {
        	name += "@buddy";
	}
	UserRecord* u = UserRecord::lookup( name );
	if (u) {
		VpErrCode rc = thePlace->whisper(u->getPresenceId(), m, UbqOpaque(0, NULL));
		if (rc == VP_OK) {
			return UBQ_TRUE;
		}
	}

	//
	// Kludge: Privileged users often sign in to buddy list with
	// '@host' names instead of '@buddy'. Grrr.
	//
	if (doBuddy) {
	    CString altName(s.getName());
            altName += "@host";
	    UserRecord* u = UserRecord::lookup( altName );
	    if (u) {
		VpErrCode rc = thePlace->whisper(u->getPresenceId(), m, UbqOpaque(0, NULL));
		if (rc == VP_OK) {
			return UBQ_TRUE;
		}
	    }
	}
	
	return UBQ_FALSE;
}

/**
 * Send an SMS message to a user
 */
void
Alert::sendSms( Event& e, Subscriber& s )
{
	FILE* fp = fopen(smsFileName, "a+");
	if (fp == NULL) {
		perror(smsFileName);
		return;
	}
	fprintf(fp, "%s\t%s\tMessage from %s: %s\n",
				s.getSms(),
				e.getDescription(),
				e.getSender(),
				e.getMessage());
	fclose(fp);
	smsCount++;
}

/**
 * Send email to a user
 */
void
Alert::sendEmail( Event& e, Subscriber& s )
{
	FILE* fp = fopen(emailFileName, "a+");
	if (fp == NULL) {
		perror(emailFileName);
		return;
	}
	fprintf(fp, "%s\t%s\tMessage from %s: %s\n",
				s.getEmail(),
				e.getDescription(),
				e.getSender(),
				e.getMessage());
	fclose(fp);
	emailCount++;
}

/**
 * Attempt to pass a message to a subscriber in vpadult.
 *
 * Message format:
 * (bytes)	(description)
 *	4	length of total message
 *	1	event type
 *	1	subscriber's notification preference
 *      1	flag: 0 = normal, 1 = just use the message text
 * 	2	length of description (n1)
 *	n1	description
 * 	2	length of sender's name (n2)
 *	n2	sender's name
 * 	2	length of message text (n3)
 *	n3	message text
 * 	2	length of subscriber's name (n4)
 *	n4	subscriber's name
 */
UbqBool
Alert::sendToJr( Event& e, Subscriber& subscr, UbqUchar prefs )
{
   int fd = doConnect();
   if (fd == -2) {
     return UBQ_FALSE;		// not using a JR server
   }

   if (doConnect() > -1) {

	char* p = buff;
	UbqUlong  msgLen = sizeof(msgLen)	// msg length field
                           + sizeof(UbqUchar)	// type flag
                           + sizeof(UbqUchar)   // notify prefs
                           + sizeof(UbqUchar);  // tournament flag
	p += sizeof(msgLen);
	*p++ = e.getType();
	*p++ = prefs;
	*p++ = (e.getTournID()) ? 1 : 0;

	UbqUshort strLen = strlen(e.getDescription()) + 1;
	msgLen += (sizeof(strLen) + strLen);
	if (msgLen > sizeof(buff)) {
		fprintf(stderr, "Message too big, ignored.\n");
		return UBQ_FALSE;
	}
	UbqUshort s = strLen;
	s = htons(s);
	memcpy(p, &s, sizeof(s));
	p += sizeof(s);
	memcpy(p, (const char *)e.getDescription(), strLen);
	p += strLen;

	strLen = strlen(e.getSender()) + 1;
	msgLen += (sizeof(strLen) + strLen);
	if (msgLen > sizeof(buff)) {
		fprintf(stderr, "Message too big, ignored.\n");
		return UBQ_FALSE;
	}
	s = strLen;
	s = htons(s);
	memcpy(p, &s, sizeof(s));
	p += sizeof(s);
	memcpy(p, (const char *)e.getSender(), strLen);
	p += strLen;

	strLen = strlen(e.getMessage()) + 1;
	msgLen += (sizeof(strLen) + strLen);
	if (msgLen > sizeof(buff)) {
		fprintf(stderr, "Message too big, ignored.\n");
		return UBQ_FALSE;
	}
	s = strLen;
	s = htons(s);
	memcpy(p, &s, sizeof(s));
	p += sizeof(s);
	memcpy(p, (const char *)e.getMessage(), strLen);
	p += strLen;

	strLen = strlen(subscr.getName()) + 1;
	msgLen += (sizeof(strLen) + strLen);
	if (msgLen > sizeof(buff)) {
		fprintf(stderr, "Message too big, ignored.\n");
		return UBQ_FALSE;
	}
	s = strLen;
	s = htons(s);
	memcpy(p, &s, sizeof(s));
	p += sizeof(s);
	memcpy(p, (const char *)subscr.getName(), strLen);
	p += strLen;

	p = buff;	// go back and fill in the total length
	UbqUlong m = msgLen;
	m = htonl(m);
	memcpy(p, &m, sizeof(m));

	//
	// Send to the junior server
	//
	if (sendData( p, msgLen ) != msgLen) {
		doClose();
		return UBQ_FALSE;
	}

	//
	// Wait for ack
	//
	int c = recvData( p, 1);
	if (c == 1) {
	   if (*p == 1) {
		return UBQ_TRUE;
	   }
	   else
		return UBQ_FALSE;
	}
	else {
	   doClose();
	   return UBQ_FALSE;
	}
   }
   else {
	fprintf(stderr, "No connection to Jr server ...\n");
	return UBQ_FALSE;
   }
}

/**
 * Pass a broadcast message to the junior server for propagation.
 *
 * Message format:
 * (bytes)	(description)
 *	4	length of following message
 *	1	broadcast type
 *      1	flag: 0 = normal, 1 = just use the message text
 *	2	length of description (n1)
 *	n1	description
 *	2	length of sender's name (n2)
 *	n2	sender's name
 *	2	length of message text (n3)
 *	n3	message text
 *      2       0x0000 (no recipient name since it's implied)
 */
void
Alert::broadcastToJr( Event* e )
{
fprintf(stderr, "Broadcast to JR, msg: %d\n", (const char *)e->getMessage());
   if (doConnect() > -1) {

fprintf(stderr, "Sending to JR\n");

	char* p = buff;
	UbqUlong  msgLen = sizeof(msgLen) + sizeof(UbqUchar) + sizeof(UbqUchar);
	p += sizeof(msgLen);
	*p++ = e->getType();
	*p++ = 0;

	UbqUshort strLen = strlen(e->getDescription()) + 1;
	msgLen += (sizeof(strLen) + strLen);
	if (msgLen > sizeof(buff)) {
		fprintf(stderr, "Broadcast too big, ignored.\n");
		return;
	}
	UbqUshort s = strLen;
	s = htons(s);
	memcpy(p, &s, sizeof(s));
	p += sizeof(s);
	memcpy(p, (const char *)e->getDescription(), strLen);
	p += strLen;

	strLen = strlen(e->getSender()) + 1;
	msgLen += (sizeof(strLen) + strLen);
	if (msgLen > sizeof(buff)) {
		fprintf(stderr, "Broadcast too big, ignored.\n");
		return;
	}
	s = strLen;
	s = htons(s);
	memcpy(p, &s, sizeof(s));
	p += sizeof(s);
	memcpy(p, (const char *)e->getSender(), strLen);
	p += strLen;

	strLen = strlen(e->getMessage()) + 1;
	msgLen += (sizeof(strLen) + strLen);
	if (msgLen > sizeof(buff)) {
		fprintf(stderr, "Broadcast too big, ignored.\n");
		return;
	}
	s = strLen;
	s = htons(s);
	memcpy(p, &s, sizeof(s));
	p += sizeof(s);
	memcpy(p, (const char *)e->getMessage(), strLen);
	p += strLen;

	s = 0;		// no recipient name (it is implied)
	msgLen += sizeof(strLen);
	memcpy(p, &s, sizeof(s));
fprintf(stderr, "Msg len: %d\n", msgLen);

	p = buff;	// go back and fill in the total length
	UbqUlong m = msgLen;
	m = htonl(m);
	memcpy(p, &m, sizeof(m));

	//
	// Send to the junior server
	//
	if (sendData( p, msgLen ) != msgLen) {
		doClose();
		return;
	}

	//
	// Wait for ack
	//
	int c = recvData( p, 1);
	if (c == 1) {
	   if (*p == 1) 
		dumpMsg("Broadcast propagated to junior server");
	   else
		dumpMsg("Broadcast to junior server failed");
	}
	else {
	   dumpMsg("No response from Jr server ...");
	   doClose();
	}
   }
   else {
	dumpMsg("No connection to Jr server ...");
	return;
   }
}
/** 
 * Manage statistics counters
 */
void
Alert::dumpStats( Db& db, Event& e )
{
	char timeStamp[100];
	time_t t = time(0);
	struct tm *tp = localtime(&t);
	strftime(timeStamp, sizeof(timeStamp), "%Y%m%d %H:%M:%S", tp);
	fprintf(stderr, "%s alert with %d subscribers, sent to %d online users, %d SMS, %d email\n",
		timeStamp, subscriberCount, onlineCount, smsCount, emailCount);
        // 
        // No history kept for tournament messages
        //
        if (e.getTournID() == 0) {
	  db.updateEventStats(e.getEventID(), onlineCount, emailCount, smsCount, subscriberCount);
        }
	onlineCount = smsCount = emailCount = subscriberCount = 0;
}
void
Alert::dumpMsg( const char *msg )
{
	char timeStamp[100];
	time_t t = time(0);
	struct tm *tp = localtime(&t);
	strftime(timeStamp, sizeof(timeStamp), "%Y%m%d %H:%M:%S", tp);
	fprintf(stderr, "%s %s\n", timeStamp, msg);
}
