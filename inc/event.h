/**
 * class: event list from the database
 */
#ifndef ___EVENT
#define ___EVENT

enum alertType {
  ALERT_TYPE_B_VPCHAT    = 0x01,
  ALERT_TYPE_B_VPADULT   = 0x02,
  ALERT_TYPE_B_CHAT      = 0x04,
  ALERT_TYPE_B_ADULT     = 0x08,
  ALERT_TYPE_ADULT_EVENT = 0x010,
  ALERT_TYPE_INTERNAL    = 0x020
};

#define BROADCAST_VPCHAT(p)	(((p)&0x01) != 0)
#define BROADCAST_VPADULT(p)	(((p)&0x02) != 0)
#define BROADCAST_CHAT(p)	(((p)&0x04) != 0)
#define BROADCAST_BUDDY(p)	(((p)&0x08) != 0)
#define ADULT_EVENT(p)		(((p)&0x10) != 0)
#define INTERNAL_EVENT(p)	(((p)&0x20) != 0)

class Subscriber;
class PtgPlace;

class Event {
  private:
    CString	description;
    CString	sender;
    CString	message;
    UbqUlong	eventID;
    UbqUlong	notifyID;
    UbqUlong	tournID;
    GList*	subscribers;
    UbqUchar	type;
    PtgPlace*	thePlace;

  public:
    Event( UbqUlong eventID,
           UbqUlong notifyID,
           UbqUlong tournID,
           char * description,
           char * sender,
           char * message,
	   UbqUchar type );

    ~Event();

    UbqUlong    getID() { return notifyID; };
    UbqUlong    getEventID() { return eventID; };
    UbqUlong    getTournID() { return tournID; };
    const char* getSender() { return sender; };
    const char* getDescription() { return description; };
    const char* getMessage() {return message; };
    UbqUchar    getType() { return type; };
    UbqBool     isBroadcast() { return ((type & 0xf) > 0) ? UBQ_TRUE : UBQ_FALSE; }
    PtgPlace*	getPlace() { return thePlace; };
    void	setPlace (PtgPlace* p) { thePlace = p; };
};
#endif ___EVENT
