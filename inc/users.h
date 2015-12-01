#ifndef __USERS_H
#define __USERS_H

#include <UbqList.h>
#include <vpTypes.h>
#include <glib.h>

//////////////////////
// UserRecord Class //
//////////////////////
class UserRecord {
 private:
  UbqUlong        id;
  VpMemberType    role;
  VpRegMode       regMode;
  CString         name;

 public:
  UserRecord(UbqUlong     id,
             VpMemberType role,
             VpRegMode    regMode,
             CString      name);
 ~UserRecord();
  static UserRecord*	lookup(CString name);
  static void	iterate( GHFunc f, gpointer d );
  UbqUlong	getPresenceId() { return id; };
  VpMemberType	getRole() { return role; };
};


////////////////////
// PtgUsers Class //
////////////////////
class PtgUsers : public UbqObject {
 private:
  
 public:
  PtgUsers(UbqUlong maxCapacity);
  ~PtgUsers();

  // state update events
  void         signOn(UbqUlong     id,
                      CString&     name,
                      VpMemberType role,
                      VpRegMode    regMode);
  void 	      signOff(UbqUlong     id,
                      CString&     name,
                      VpMemberType role,
                      VpRegMode    regMode);
};


#endif /* #ifndef __USERS_H */
