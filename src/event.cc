#include <ubique.h>
#include <vpTypes.h>
#include <glib.h>
#include "event.h"

///////////////////
// Event class
//////////////////

Event::Event( UbqUlong e,
	      UbqUlong n,
	      UbqUlong te,
              char * d,
              char * s,
              char * m,
	      UbqUchar t)
{
	eventID     = e;
	notifyID    = n;
	tournID     = te;
	description = d;
	sender      = s;
	message     = m;
	type        = t;
}
Event::~Event()
{
}
