#include <ubique.h>
#include <vpTypes.h>
#include "subscriber.h"

///////////////////
// Subscriber class
//////////////////

Subscriber::Subscriber( char * n,
              		UbqUlong u,
	      		UbqUchar p )
{
	name        = n;
	userID      = u;
	notifyPrefs = p;
	active      = UBQ_FALSE;
	email	    = "";
	sms  	    = "";
}

/**
 * Update a subscriber's offline notification info
 */
void
Subscriber::updateOfflinePrefs( UbqBool a,
              			char *  e,
				char *  s )
{
	active      = a;
	email	    = e;
	sms  	    = s;
}
