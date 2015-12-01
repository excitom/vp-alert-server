/**
 * class: subscribers to an event list 
 */
#ifndef ___SUBSCR
#define ___SUBSCR

#define IM_VPCHAT(p)	(((p)&0x01) != 0)
#define IM_VPADULT(p)	(((p)&0x02) != 0)
#define IM_CHAT(p)	(((p)&0x04) != 0)
#define IM_BUDDY(p)	(((p)&0x08) != 0)
#define OFFL_EMAIL(p)	(((p)&0x10) != 0)
#define OFFL_SMS(p)	(((p)&0x20) != 0)
#define OFFL_WANTED(p)	(((p)&0x30) != 0)

class Subscriber {
  private:
    CString	name;
    UbqUlong	userID;
    UbqUchar	notifyPrefs;
    UbqBool	active;
    CString	email;
    CString	sms;

  public:
    Subscriber( char *    name,
		UbqUlong  userID,
		UbqUchar  notifyPrefs );
    ~Subscriber() {
    }
    UbqUlong getID() { return userID; };
    UbqUchar getNotifyPrefs() { return notifyPrefs; };
    const char* getName()  { return (const char *)name; };
    const char* getEmail() { return (const char *)email; };
    const char* getSms()   { return (const char *)sms; };
    void updateOfflinePrefs( UbqBool, char *, char * );
};
#endif ___SUBSCR
