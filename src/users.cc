/*
 * Maintain a hash table containing all users signed in to the 
 * community. The user records can be searched by name. Each
 * record contains the name's current presence ID (a unique
 * number assigned to the user by the VP server for the duration
 * of each chat session). The presence ID is required for sending
 * an IMs to the user.
 */

#include <ubique.h>
#include <users.h>
#include <main.h>
#include <UbqArray.h>
#include <UbqCtrl.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include <glib.h>

static GHashTable* anchor = NULL;

//////////////////////
// UserRecord Class //
//////////////////////
UserRecord::UserRecord(UbqUlong     _id,
                       VpMemberType _role,
                       VpRegMode    _regMode,
                       CString      _name)
{
  if (anchor == NULL) {
    anchor = g_hash_table_new( (GHashFunc) g_str_hash, (GCompareFunc) g_str_equal );
  }

  id        = _id;
  role      = _role;
  regMode   = _regMode;
  name      = _name;

  gpointer _n = (gpointer)((const char*) name);
  g_hash_table_insert( anchor, _n, (UserRecord *)this );
}

UserRecord::~UserRecord()
{
  if (anchor == NULL) {
    return;
  }
  g_hash_table_remove( anchor, this->name );
  if (anchor == NULL) {
    g_hash_table_destroy( anchor );
  }
}

/** 
 * Lookup user by name
 */
UserRecord*
UserRecord::lookup( CString n )
{
  if (!anchor) return NULL;	// empty table
  gpointer _n = (gpointer)((const char *)n);
  UserRecord* u = (UserRecord* )g_hash_table_lookup( anchor, (gpointer)_n );
  return u;
}

/**
 * Iterate through all users
 *
 * The input function is called once for each user record in the 
 * table, and is passed the key/value pair for the user record (i.e. the
 * user name and user record object) and also the input data.
 */
void
UserRecord::iterate( GHFunc func, gpointer eData )
{
  g_hash_table_foreach( anchor, func, eData );
}


////////////////////
// PtgUsers Class //
////////////////////
PtgUsers::PtgUsers(UbqUlong maxCapacity)
{
}

PtgUsers::~PtgUsers()
{
}

void PtgUsers::signOn(UbqUlong     id,
                      CString&     name,
                      VpMemberType role,
                      VpRegMode    regMode)
{
  if (UserRecord::lookup(name) != NULL) {
    ubqDisplayError(MY_TITLE, "Duplicate sign on for %lu (%s)", id, (const char*)name);
    return;
  }   

  UserRecord* userRecord = new UserRecord(id, role, regMode, name);
}

void PtgUsers::signOff(UbqUlong     id,
                       CString&     name,
                       VpMemberType role,
                       VpRegMode    regMode)
{
  UserRecord* u = UserRecord::lookup(name);
  if (u == NULL) {
    ubqDisplayError(MY_TITLE, "Duplicate sign off for %lu (%s)", id, (const char*)name);
  } else {
    delete u;
  }
}
