//
// VP Event Notification Service
//
// Users may schedule messages to be sent to various interest lists.
// Other users who subscribe to these lists will receive the messages.
// They may receive IMs, email messages, or SMS messages.
//
// Author: Tom Lang 5/2003
//
#include <ubique.h>

#include "place.h"
#include "main.h"
#include "db.h"
#include "event.h"
#include "subscriber.h"
#include "alert.h"
#include <VpSrvMsg.h>
#include <VpPrsnce.h>
#include <VpFun.h>
#include <VpConn.h>
#include <Ucm.h>
#include <UbqCtrl.h>
#include <ubqAppFrame.h>
#include <sys/signal.h>

class PtgConnection : public VpConnection {
 public:
  void connected();
  void disconnecting(VpErrCode reason);
};


//////////////////////////////
// Static variables section //
//////////////////////////////
static PtgConnection  myConnection;
static PtgPlace*      myPlace     = 0;
static UbqBool        trace       = UBQ_FALSE;
static CString        serverName;
static CString        jrServerName;
static UbqUlong       pollInterval;
static CString        myName      = MY_NAME;
static CString        myPassword  = MY_PASSWORD;
static UbqBool	      timeToPoll  = UBQ_FALSE;

//////////////////////////////
// Global variables section //
//////////////////////////////
UbqBool   gotDisconnected = UBQ_FALSE;
UbqBool   debug = UBQ_FALSE;


///////////////
// Utilities //
///////////////

// If we wanted to handle STDIN, we'd do it here
static void analyzeUserInput()
{
  char buf[4096];
  int  rc = read(0, buf, sizeof(buf));

  if (rc == -1)
    perror("read");
}

static void exitFunc()
{
  PtgPlace* p = myPlace;
  myPlace     = 0;
  myConnection.disconnect() ;
  vpTerminate();
  delete p;
}

static void memError()
{
  ubqDisplayError(MY_TITLE, "Out of virtual memory, exiting");
}

int parseArgs(int argc, char* argv[])
{
  int c;
  while ((c = getopt(argc, argv, "?dtu:p:")) != EOF)
    switch(c) {
       case 'd':
	 debug = UBQ_TRUE;
	 break;
       case 't':
	 trace = UBQ_TRUE;
	 break;
       case 'p':
         myPassword = optarg;
	 break;
       case 'u':
         myName = optarg;
	 break;
       default:
	 ubqDisplayError(MY_TITLE, "Unrecognized option %c ignored", (char)c);
	 break;
      }
  if (optind > argc) {
	ubqDisplayError(MY_TITLE, "Missing community name\n");
	return 0;
  }
  serverName = argv[optind++];
  if (optind > argc) {
	ubqDisplayError(MY_TITLE, "Missing secondary community name\n");
	return 0;
  }
  jrServerName = argv[optind];
  return 0;
}

static void timeEvent(int signalNum, ...)
{
  ubqAssert(signalNum == SIGALRM);
  ubqSetSignal(SIGALRM, timeEvent);
  timeToPoll = UBQ_TRUE;
}

////////////////
// START HERE //
////////////////

int ubqAppMain(int argc, char* argv[])
{
  //
  // Process initialization
  //
  ubqSetExitFunc(exitFunc);
  ubqMemSetExceptionRoutine(memError);
  ubqSetSignal(SIGPIPE, (UBQ_EXCEPTION)SIG_IGN);
  ubqSetSignal(SIGHUP, (UBQ_EXCEPTION)SIG_IGN);

  //
  // Parse command line arguments
  //
  if (parseArgs(argc, argv) < 0)
    return 0;
  if (serverName.IsEmpty()) {
    ubqDisplayError(MY_TITLE, "unknown server name");
    return 0;
  }

  //
  // Set up trace file and pid file
  //
  UbqControlString c("VPALERT_ROOT", "/u/vplaces/VPCOM/VPALERT");
  CString root = c.getVal();

  FILE* traceFilePtr = stderr;
  if (!trace) {
    CString fileName = root + "/" + MY_TRACE_FILE;
    FILE* traceFilePtr = fopen(fileName, "a");
    if (!traceFilePtr) {
      ubqDisplayError(MY_TITLE,
		      "Cannot open trace file `%s'",
		      (const char*)fileName);
      traceFilePtr = stderr;
    }
    else {
      // make trace the default stdout & stderr
      dup2(fileno(traceFilePtr),2);
      dup2(2, 1);
      fclose(traceFilePtr);
    }
  }

  fprintf(ubqGetErrorFile(),
      "Welcome to %s version %s, %s\nServer name: %s\n",
      MY_TITLE, MY_VERSION, ubqGetCurrTimeStr(), (const char *)serverName);

  CString pidFile = root + "/vp.pid";
  FILE* fp = fopen(pidFile, "w+");
  fprintf( fp, "%d\n", getpid() );
  fclose(fp);

  //
  // Initialize SQL server connection
  //
  Db db(root);

  fflush(ubqGetErrorFile());

  if (debug)
    UcmCommMngr::getTheCm().startReadNotify(0);

  //
  // Initialize the VP lib
  //
  VpErrCode rc = vpInitialize(0, 0, 0);
  if (rc != VP_OK) {
    ubqDisplayError(MY_TITLE, "cannot initialize (%d)", (int)rc);
    return 0;
  }

  //
  // Create the local client place
  //
  ubqAssert(!myPlace);
  VpPresenceState* state = new VpPresenceState;
  state->setName(myName);
  state->setFullName(myName);
  state->setAddress("No Address");
  state->setEmailAddress("No Email");	
  myPlace = new PtgPlace(new VpPresence(state));

  //
  // Connect to the community
  //
  gotDisconnected = UBQ_FALSE;
  rc              = myConnection.connect(serverName);
  if (rc != VP_OK) {
    ubqDisplayError(MY_TITLE, "Connect request failed (%d)", (int)rc);
    gotDisconnected = UBQ_TRUE;
  }

  //
  // Set up polling for database changes
  //
  ubqSetSignal(SIGALRM, timeEvent);
  pollInterval = UbqControlUlong("VPALERT_POLL_INTERVAL", 60).getVal();
  alarm(pollInterval);

  //
  // Process events
  //
  Alert alert(myPlace, root, (const char *)jrServerName);
  int width = 0;
  while (!gotDisconnected) {

    if( timeToPoll ) {
	alert.poll( db );
        timeToPoll = UBQ_FALSE;
  	alarm(pollInterval);
    }

    //
    // Check for action on any sockets or files
    //
    UbqFdSet r = UcmCommMngr::getTheCm().getReadMask();
    UbqFdSet w = UcmCommMngr::getTheCm().getWriteMask();
    int rWidth = r.getWidth();
    int wWidth = w.getWidth();
    width      = ((rWidth > wWidth) ? rWidth : wWidth);
    int n = select(width, r(), w(), 0, 0);
    if (n < 1)
      continue;		// interrupted system call
    if (r.isSet(0)) {
      r.unset(0);
      analyzeUserInput();
    }

    //
    // Messages from the VP server are caught here,
    // and handled in callback methods (see the PtgConnection
    // and PtgPlace classes for details)
    //
    UcmCommMngr::getTheCm().analyzeMasks(r, w);
  }

  ubqDoExit(1);
  return 1;
}


/////////////////////////
// PtgConnection Class //
/////////////////////////
void PtgConnection::connected()
{
  VpFullUserName name(myName, VP_REG_LOCAL);
  int            mask = VP_SRV_SIGN_ON_MASK  |
                        VP_SRV_SIGN_OFF_MASK;
  VpErrCode        rc = myPlace->signOn(myConnection,      name, myPassword,
					VP_MT_SERV_INV_BUD, mask, 0);
  if (rc != VP_OK) {
    ubqDisplayError(MY_TITLE, "Sign-on request failed (%d)", (int)rc);
    gotDisconnected = UBQ_TRUE;
  }    
}

void PtgConnection::disconnecting(VpErrCode rc)
{
  ubqDisplayError(MY_TITLE, "Disconnected (%d)", (int)rc);
  gotDisconnected = UBQ_TRUE;
}
