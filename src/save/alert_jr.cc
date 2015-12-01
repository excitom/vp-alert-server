/*
 * Alert a subscriber to a list that an event has occured - Junior verion
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
#include <UbqCtrl.h>

#include "alert_jr.h"
#include "event.h"
#include "users.h"
#include "place.h"
#include "subscriber.h"

extern UbqBool debug;

///////////////////////////
// AlertJr class 
///////////////////////////

AlertJr::AlertJr( PtgPlace* p, CString root ) : Sock()
{
	thePlace = p;
}

/**
 * Send an alert message to a subscriber
 */
void
AlertJr::sendAlert()
{
fprintf(stderr, "sendAlert called\n");
   if (!getEvent()) {
	return;
   }

   if (BROADCAST_CHAT(type) || BROADCAST_BUDDY(type)) {
	broadcast();
	ackEvent(UBQ_TRUE);
   }
   else {
	UbqBool sent = UBQ_FALSE;
	UbqBool doBuddy;
	if( IM_CHAT(prefs) ) {		// user wants a chat IM
          doBuddy = UBQ_FALSE;
          if (sendLocal( name, doBuddy ))
		sent = UBQ_TRUE;
	}
	if( IM_BUDDY(prefs) ) {		// user wants a buddy IM
          doBuddy = UBQ_TRUE;
          if (sendLocal( name, doBuddy ))
		sent = UBQ_TRUE;
	}
	ackEvent(sent);
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
 * the data passed from the 'broadcast' method (which is the message).
 */
void
broadcastMsg( const char* n, UserRecord* u, AlertJr* a )
{
   UbqBool send = UBQ_FALSE;
   if (VP_IS_A_BL_PRESENCE(u->getRole())) {
	if (BROADCAST_BUDDY(a->getType()))
	   send = UBQ_TRUE;
   }
   else {
	if (BROADCAST_CHAT(a->getType()))
	   send = UBQ_TRUE;
   }
   if (send) {
	VpErrCode rc = a->getPlace()->whisper(u->getPresenceId(), a->getMsg(), UbqOpaque(0, NULL));
   }
}

void
AlertJr::broadcast()
{
   sprintf(buff, "Broadcast message from %s: %s",
			(const char *)sender,
			(const char *)message);
   UserRecord::iterate( (GHFunc)broadcastMsg, (gpointer)this );
}

/**
 * Attempt to send an IM to a user
 */
UbqBool
AlertJr::sendLocal( CString nm, UbqBool doBuddy )
{
   CString altName = nm;
   if (doBuddy) {
        nm += "@buddy";
   }
   UserRecord* u = UserRecord::lookup( nm );
   if (u) {
   	char m[1024];
        if (fmtFlag == 0) {
   		sprintf(m, "Message from %s to the %s list: %s",
   			(const char *)sender,
   			(const char *)description,
   			(const char *)message);
		if (debug) {
		  fprintf(stderr, "Send to: %s Message from %s to the %s list: %s\n",
		    (const char *)nm,
		    (const char *)sender,
		    (const char *)description,
		    (const char *)message);
		}
	}
	else {
		strcpy(m, (const char *)message);
		if (debug) {
		  fprintf(stderr, "Send to: %s - %s", (const char *)nm, (const char *)message);
		}
	}
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
     altName += "@host";
     UserRecord* u = UserRecord::lookup( altName );
     if (u) {
	char m[1024];
	sprintf(m, "Message from %s to the %s list: %s",
   			(const char *)sender,
   			(const char *)description,
   			(const char *)message);
	VpErrCode rc = thePlace->whisper(u->getPresenceId(), m, UbqOpaque(0, NULL));
	if (rc == VP_OK) {
		return UBQ_TRUE;
	}
     }
   }
   return UBQ_FALSE;
}

/**
 * Get event details from the main alert server
 *
 * Message format:
 * (bytes)	(description)
 *	4	length of total message
 *	1	event type
 *	1	subscriber notification preferences
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
AlertJr::getEvent()
{
fprintf(stderr, "getEvent called\n");
	//
	// Get the message length
	//
	UbqUlong	msgLen;
	if (recvData((char *)&msgLen, sizeof(msgLen)) != sizeof(msgLen)) {
		fprintf(stderr, "Failed to read from socket\n");
		doClose();
		return UBQ_FALSE;
	}
	msgLen = ntohl(msgLen);
	if (msgLen > sizeof(buff)) {
		fprintf(stderr, "Message too big (%d), ignored\n", msgLen);
		doClose();
		return UBQ_FALSE;
	}
	if (msgLen < 4) {
		fprintf(stderr, "Message too short (%d), ignored\n", msgLen);
		doClose();
		return UBQ_FALSE;
	}
	msgLen -= 4;	// subtract header length

	//
	// Read in the remainder of the message
	//
	char* p = buff;
	if (recvData(p, msgLen) != msgLen) {
		fprintf(stderr, "Failed to read from socket\n");
		doClose();
		return UBQ_FALSE;
	}

	type = *p++;
	prefs = *p++;
	fmtFlag = *p++;

	UbqUshort strLen;
	memcpy(&strLen, p, sizeof(strLen));
	p += sizeof(strLen);
	strLen = ntohs(strLen);
	if ((strLen == 0) || (((p+strLen)-buff) > sizeof(buff))) {
		doClose();
		return UBQ_FALSE;
	}

	//
	// To make it easy to extact strings from the message buffer,
	// the sender includes the null terminating character.
	// 
	description = p;
fprintf(stderr, "Description: %s\n", p);
	p += strLen;

	memcpy(&strLen, p, sizeof(strLen));
	p += sizeof(strLen);
	strLen = ntohs(strLen);
	if ((strLen == 0) || (((p+strLen)-buff) > sizeof(buff))) {
		doClose();
		return UBQ_FALSE;
	}

	sender = p;
fprintf(stderr, "Sender: %s\n", p);
	p += strLen;


	memcpy(&strLen, p, sizeof(strLen));
	p += sizeof(strLen);
	strLen = ntohs(strLen);
	if ((strLen == 0) || (((p+strLen)-buff) > sizeof(buff))) {
		doClose();
		return UBQ_FALSE;
	}

	message = p;
fprintf(stderr, "Message: %s\n", p);
	p += strLen;

	memcpy(&strLen, p, sizeof(strLen));
	p += sizeof(strLen);
	strLen = ntohs(strLen);
	if (((p+strLen)-buff) > sizeof(buff)) {
		doClose();
		return UBQ_FALSE;
	}

fprintf(stderr, "Got type %x\n", type);
	if (strLen == 0) {
	   if (BROADCAST_CHAT(type) || (BROADCAST_BUDDY(type)))
		return UBQ_TRUE;	// OK if broadcast
	   else
		doClose();
		return UBQ_FALSE;
	}

	name = p;

	return UBQ_TRUE;
}

/**
 * Send ack message for an event
 */
void
AlertJr::ackEvent(UbqBool result)
{
	UbqUchar ack = (result == UBQ_TRUE) ? 1 : 0;
	sendData((char *)&ack, sizeof(ack));
}
