/*
 * Handle messages from the VP server relating to
 * the "place". There are many possibilities, but 
 * the only ones we care about are users entering 
 * and exiting the place.
 */
#include <main.h>
#include <place.h>
#include <users.h>
#include <VpLocMsg.h>
#include <VpSrvMsg.h>
#include <VpInfo.h>
#include <UbqArray.h>
#include <UbqCtrl.h>
#include <ctype.h>


//
// Return the canonical form of a user name, i.e.
// white space removed and folded to lower case.
//
static void canonName(CString& name)
{
  char* org = name.GetBuffer(name.GetLength());
  char* cur = org;
  for (char* p = org; *p; p++)
    if (!isspace(*p))
      *cur++ = tolower(*p);
  name.ReleaseBuffer(cur-org);
}

////////////////////
// PtgPlace Class //
////////////////////
PtgPlace::PtgPlace(VpPresence* myself) :
  VpClientPlace(myself, new VpGroup, new VpGroup), users(1)
{
}

/**
 * Handle user sign-on message
 */
void PtgPlace::signOnUser(VpNdr& msg)
{
  CString   name;
  VpRegMode regMode;
  UbqUlong  id;
  UbqUlong  ipAddr;
  CString   fullName;
  CString   email;
  VpMemberType role;
  if (!msg.load(name)              || !msg.loadEnum(regMode)  ||
      !msg.load(id)                || !msg.loadEnum(role)     ||
      !msg.load(ipAddr, UBQ_FALSE) || !msg.load(fullName)     ||
      !msg.load(email)) {
    ubqDisplayError(MY_TITLE, "Bad sign-on notification ignored");
    return;
  }
  canonName(name);
  users.signOn(id, name, role, regMode);
}

/**
 * Handle user sign-off message
 */
void PtgPlace::signOffUser(VpNdr& msg)
{
  CString   name;
  VpRegMode regMode;
  UbqUlong  id;
  VpMemberType role;
  if (!msg.load(name) || !msg.loadEnum(regMode) || 
      !msg.load(id)   || !msg.loadEnum(role)) {
    ubqDisplayError(MY_TITLE, "Bad sign-off notification ignored");
    return;
  }
  canonName(name);
  users.signOff(id, name, role, regMode);
}

/**
 * Handle disconnect message from server
 */
void PtgPlace::disconnecting(VpErrCode err, UbqUlong duration)
{
  ubqDisplayError(MY_TITLE, "signed-off (%d)", (int)err);
  gotDisconnected = UBQ_TRUE;
}

/** 
 * receive a message from the server
 */
void PtgPlace::sentFromService(UbqUlong             reqId,
			       const VpServiceInfo& sender, 
			       UbqUshort            type,
			       const CString&       string,
			       const UbqOpaque&     data)
{
  VpNdr msg(data.getLength(), data);
  if (sender.getId() == 0) {		// from community service
    switch (type) {
     case VP_SRV_SIGN_ON:
      signOnUser(msg);
      break;
    
     case VP_SRV_SIGN_OFF:
      signOffUser(msg);
      break;
    
     default:
      ubqDisplayError(MY_TITLE, "invalid server notification (%d) ignored", (int)type);
      break;
    }
  }
}
