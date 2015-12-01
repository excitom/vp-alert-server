#include <UbqOpq.h>
#include <string.h>
#include <alertMsg.h>
#include <glib.h>
#include <event.h>

/*
 * Messages exchanged between the primary and secondary alert servers.
 *
 * Tom Lang 1/2005
 */

AlertMsg::AlertMsg(   UbqUchar type,
		      UbqUchar prefs,
		      CString& description,
		      CString& sender,
		      CString& message,
		      CString& receiver ) : VpNdr()
{
  dump(type);
  dump(prefs);
  UbqUchar flag = ALERT_FMT_NORMAL;
  dump(flag);
  dump(description);
  dump(sender);
  dump(message);
  dump(receiver);
}

AlertMsg::AlertMsg(   UbqUchar type,
		      UbqUchar prefs,
		      CString& message,
		      CString& receiver ) : VpNdr()
{
  dump(type);
  dump(prefs);
  UbqUchar flag = ALERT_FMT_COMBINED;
  dump(flag);
  dump(message);
  dump(receiver);
}

Broadcast::Broadcast(   UbqUchar prefs,
			CString& description,
			CString& sender,
			CString& message,
			CString& receiver )
{
  UbqUchar type = ALERT_TYPE_B_VPADULT | ALERT_TYPE_B_CHAT | ALERT_TYPE_B_ADULT;
  dump(type);
  dump(prefs);
  UbqUchar flag = ALERT_FMT_NORMAL;
  dump(flag);
  dump(description);
  dump(sender);
  dump(message);
}
