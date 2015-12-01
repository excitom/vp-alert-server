/*
 * Communicate with the SQL server
 */
#include <ubique.h>
#include <vpTypes.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <glib.h>

#include "db.h"
#include "event.h"
#include "subscriber.h"

//
// SQL server communication object
//
DBPROCESS *Db::dbproc;

///////////////////
// Db class
//////////////////

/**
 * Unrecoverable DB error exit
 */
void doexit( char *s1 )
{
    fprintf(stderr, s1);
    dbexit();            /* always call dbexit before returning to OS */
    exit(ERREXIT);
}

/**
 * SQL server callbacks
 */

int CS_PUBLIC
msg_handler(
	DBPROCESS       *dbproc,
	DBINT           msgno,
	int             msgstate,
	int             severity,
	char            *msgtext,
	char            *srvname,
	char            *procname,
	int            line)

{
	// don't care about these informational msgs
	if ((msgno == 5701) || (msgno == 5703))
		return(0);

	fprintf (stderr, "Msg %ld, Level %d, State %d\n", 
	        msgno, severity, msgstate);

	if (strlen(srvname) > 0)
		fprintf (stderr, "Server '%s', ", srvname);
	if (strlen(procname) > 0)
		fprintf (stderr, "Procedure '%s', ", procname);
	if (line > 0)
		fprintf (stderr, "Line %d", line);

	fprintf (stderr, "\n\t%s\n", msgtext);

	return(0);
}

int CS_PUBLIC
err_handler(
	DBPROCESS    *dbproc,
	int          severity,
	int          dberr,
	int          oserr,
	char         *dberrstr,
	char         *oserrstr)
{
    if ((dbproc == (DBPROCESS *)NULL) || (DBDEAD(dbproc)))
        return(INT_EXIT);
    else 
    {
	//
	// Can't seem to avoid this if a stored procedure returns
	// zero rows ... it seems like there's one (empty) row anyway
	// and dbbind returns this error since there are no columns
	// to bind ...
	if (dberr == SYBEABNC)
        	return(INT_CANCEL);

        fprintf (stderr, "DB-Library error:\n\t%s\n", dberrstr);

        if (oserr != DBNOERR)
            fprintf (stderr, "Operating-system error:\n\t%s\n", oserrstr);

        return(INT_CANCEL);
    }
}

/**
 * Constuctor
 */
Db::Db( CString root )
{

  //
  // Initialize DB-Library.
  //
  if (dbinit() == FAIL)
	exit(ERREXIT);

  //
  // Install error-handling and message-handling callbacks
  //
  dberrhandle((EHANDLEFUNC)err_handler);
  dbmsghandle((MHANDLEFUNC)msg_handler);
	
  //
  // Get login info
  //
  CString dbrcFile = root + "/.Dbrc";
  FILE* fp = fopen(dbrcFile, "r");

  if (!fp) {
    fprintf(stderr, "ERROR: Can't open database config file: %s\n", (const char *)dbrcFile);
    exit(1);
  }

  login = dblogin( );
  char key[256], val[256], Server[256];
  while( fscanf( fp, "%s %s", key, val ) != EOF ) {
    if( strcmp(key, "login") == 0 )
      DBSETLUSER(login, val);
    if( strcmp(key, "password") == 0 )
      DBSETLPWD(login, val);
    if( strcmp(key, "dataserver") == 0 )
      strcpy(Server, val);
  }
  fclose(fp);
    
  //
  // Allocate and initialize the LOGINREC structure to be used
  // to open a connection to SQL Server.
  //

  DBSETLAPP(login, "vpalert");
	
  dbproc = dbopen(login, Server);
}

/**
 * Destructor
 */
Db::~Db()
{
}

/**
 * Query the database for pending events
 */
GList* 
Db::getEvents()
{
	GList* eventList = NULL;

	//
	// Use RPC to execute a SQL stored procedure, 
	// which gets events, if any.
	//
	if (dbrpcinit(dbproc, "doNotifyEvents", (DBSMALLINT)0) == FAIL)
		doexit("dbrpcinit failed.\n");
	
	if (dbrpcsend(dbproc) == FAIL)
		doexit("dbrpcsend failed.\n");

	if (dbsqlok(dbproc) == FAIL)
		doexit("dbsqlok failed.\n");

	DBINT status = dbretstatus(dbproc);
	//fprintf(stderr, "RPC result = %d\n", status);

	//
	// Process result rows
	//
	while ((return_code = dbresults(dbproc)) != NO_MORE_RESULTS)
	{
	   if (return_code == FAIL)
		doexit("dbresults failed.\n");
	   if (return_code == SUCCEED)
	   {
		int eventID = 0;
		dbbind(dbproc, 1, INTBIND, (DBINT)0, (BYTE DBFAR*)&eventID);
		int notifyID = 0;
		dbbind(dbproc, 2, INTBIND, (DBINT)0, (BYTE DBFAR*)&notifyID);
		int tournID = 0;
		dbbind(dbproc, 3, INTBIND, (DBINT)0, (BYTE DBFAR*)&tournID);
		char description[256] = "";
		dbbind(dbproc, 4, NTBSTRINGBIND, (DBINT)0, (BYTE DBFAR*)description);
		char nickName[256] = "";
		dbbind(dbproc, 5, NTBSTRINGBIND, (DBINT)0, (BYTE DBFAR*)nickName);
		char message[256] = "";
		dbbind(dbproc, 6, NTBSTRINGBIND, (DBINT)0, (BYTE DBFAR*)message);
		int type = 0;
		dbbind(dbproc, 7, INTBIND, (DBINT)0, (BYTE DBFAR*)&type);

		while (dbnextrow(dbproc) != NO_MORE_ROWS)
		{

fprintf(stderr, "event %d notify %d tourn %d desc %s name %s message %s type %d\n", eventID, notifyID, tournID, description, nickName, message, type);

			Event* e = new Event( eventID, notifyID, tournID, description, nickName, message, type );
			eventList = g_list_append( eventList, (gpointer)e );
		}

	   }
	}
	return eventList;
}

/**
 * Query the database for a subscriber list
 */
GList* 
Db::getSubscribers( Event& e )
{
	GList* subscrList = NULL;

	//
	// Use RPC to execute a SQL stored procedure, 
	// which gets events, if any.
	//
	if (dbrpcinit(dbproc, "getNotifySubscrs2", (DBSMALLINT)0) == FAIL)
		doexit("dbrpcinit failed.\n");

	UbqUlong notifyID = e.getID();
	UbqUlong tournID  = e.getTournID();

	if (dbrpcparam
	  (dbproc, "@notifyID", 0, SYBINT4, -1, -1, (BYTE *)&notifyID) == FAIL )
		doexit("dbrpcparam 1 failed.\n");

	if (dbrpcparam
	  (dbproc, "@tournID", 0, SYBINT4, -1, -1, (BYTE *)&tournID) == FAIL )
		doexit("dbrpcparam 1 failed.\n");

	if (dbrpcsend(dbproc) == FAIL)
		doexit("dbrpcsend failed.\n");

	if (dbsqlok(dbproc) == FAIL)
		doexit("dbsqlok failed.\n");

	//
	// Process result rows
	//
	while ((return_code = dbresults(dbproc)) != NO_MORE_RESULTS)
	{
	   if (return_code == FAIL)
		doexit("dbresults failed.\n");
	   if (return_code == SUCCEED)
	   {
		UbqUlong userID;
		dbbind(dbproc, 1, INTBIND, (DBINT)0, (BYTE *)&userID);
		char nickName[256];
		dbbind(dbproc, 2, NTBSTRINGBIND, (DBINT)0, (BYTE DBFAR*)nickName);
		UbqUchar notifyPrefs;
		dbbind(dbproc, 3, TINYBIND, (DBINT)0, (BYTE *)&notifyPrefs);

		while (dbnextrow(dbproc) != NO_MORE_ROWS)
		{
			Subscriber* s = new Subscriber( nickName, userID, notifyPrefs );
			subscrList = g_list_append( subscrList, (gpointer)s );
		}

	   }
	}
	//DBINT status = dbretstatus(dbproc);
	//fprintf(stderr, "All rows processed, RPC result = %d\n", status);
	return subscrList;
}

/**
 * Query the database for a subscriber's offline notification info
 */
UbqBool
Db::getOfflinePrefs( Subscriber& s )
{
	UbqBool active = UBQ_FALSE;
	//
	// Use RPC to execute a SQL stored procedure, 
	// which gets events, if any.
	//
	if (dbrpcinit(dbproc, "getNotifyPrefs", (DBSMALLINT)0) == FAIL)
		doexit("dbrpcinit failed.\n");

	UbqUlong userID = s.getID();

	if (dbrpcparam
	  (dbproc, "@userID", 0, SYBINT4, -1, -1, (BYTE *)&userID) == FAIL )
		doexit("dbrpcparam 1 failed.\n");

	if (dbrpcsend(dbproc) == FAIL)
		doexit("dbrpcsend failed.\n");

	if (dbsqlok(dbproc) == FAIL)
		doexit("dbsqlok failed.\n");

	//
	// Process result rows
	//
	while ((return_code = dbresults(dbproc)) != NO_MORE_RESULTS)
	{
	   if (return_code == FAIL)
		doexit("dbresults failed.\n");
	   if (return_code == SUCCEED)
	   {
		char nickName[256];
		dbbind(dbproc, 1, NTBSTRINGBIND, (DBINT)0, (BYTE DBFAR*)nickName);
		UbqUchar activeFlag;
		dbbind(dbproc, 2, BITBIND, (DBINT)0, (BYTE *)&activeFlag);
		char email[256];
		dbbind(dbproc, 3, NTBSTRINGBIND, (DBINT)0, (BYTE DBFAR*)email);
		char sms[256];
		dbbind(dbproc, 4, NTBSTRINGBIND, (DBINT)0, (BYTE DBFAR*)sms);

		while (dbnextrow(dbproc) != NO_MORE_ROWS)
		{
			active = (activeFlag != 0) ? UBQ_TRUE : UBQ_FALSE;
			s.updateOfflinePrefs( active, email, sms );
		}

	   }
	}
	//DBINT status = dbretstatus(dbproc);
	//fprintf(stderr, "All rows processed, RPC result = %d\n", status);
	return active;
}

/**
 * Update statistics for an event
 */
void
Db::updateEventStats( UbqUlong eventID, int onlineCtr, int emailCtr, int smsCtr, int subscrCtr )
{
	// Use RPC to execute a SQL stored procedure, 
	// which updates statistics for an event.
	//
	if (dbrpcinit(dbproc, "updateNotifyEventStats", (DBSMALLINT)0) == FAIL)
		doexit("dbrpcinit failed.\n");

	if (dbrpcparam
	  (dbproc, "@eventID", 0, SYBINT4, -1, -1, (BYTE *)&eventID) == FAIL )
		doexit("dbrpcparam 1 failed.\n");

	if (dbrpcparam
	  (dbproc, "@onlineCtr", 0, SYBINT4, -1, -1, (BYTE *)&onlineCtr) == FAIL )
		doexit("dbrpcparam 2 failed.\n");

	if (dbrpcparam
	  (dbproc, "@emailCtr", 0, SYBINT4, -1, -1, (BYTE *)&emailCtr) == FAIL )
		doexit("dbrpcparam 3 failed.\n");

	if (dbrpcparam
	  (dbproc, "@smsCtr", 0, SYBINT4, -1, -1, (BYTE *)&smsCtr) == FAIL )
		doexit("dbrpcparam 4 failed.\n");

	if (dbrpcparam
	  (dbproc, "@subscrCtr", 0, SYBINT4, -1, -1, (BYTE *)&subscrCtr) == FAIL )
		doexit("dbrpcparam 5 failed.\n");
	
	if (dbrpcsend(dbproc) == FAIL)
		doexit("dbrpcsend failed.\n");

	if (dbsqlok(dbproc) == FAIL)
		doexit("dbsqlok failed.\n");

	DBINT status = dbretstatus(dbproc);
	if (status != 0) {
		fprintf(stderr, "updateNotifyEventStats result = %d\n", status);
	}

	//
	// Process result rows
	//
	while ((return_code = dbresults(dbproc)) != NO_MORE_RESULTS)
	{
	   if (return_code == FAIL)
		doexit("dbresults failed.\n");
	}
}
