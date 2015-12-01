/**
 * class: database access
 */
#ifndef ___DB
#define ___DB

#include <ubique.h>
#include <vpTypes.h>
#include <sys/types.h>
#include <sybfront.h>
#include <sybdb.h>
#include "sybdbex.h"
#include "glib.h"
#include "event.h"
#include "subscriber.h"

class Db {
  private:
    static DBPROCESS 	*dbproc;
    LOGINREC    	*login;
    RETCODE     	return_code;

    void getSubscriberInfo( Subscriber& s );

  public:
    Db( CString root );			// constructor
    ~Db();				// destructor
    
    GList*  getEvents();
    GList*  getSubscribers( Event& e );
    UbqBool getOfflinePrefs( Subscriber& s );
    void    updateEventStats( UbqUlong eventID, int onlineCtr, int emailCtr, int smsCtr, int subscrCtr );
};
#endif ___DB
