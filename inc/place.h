#ifndef __PLACE_H
#define __PLACE_H

#include <users.h>
#include <VpClnPlc.h>


class PtgPlace : public VpClientPlace {
 private:
  PtgUsers users;

  void     signOnUser(VpNdr& msg);
  void    signOffUser(VpNdr& msg);

 public:
  PtgPlace(VpPresence* myself);

  virtual void   disconnecting(VpErrCode            err,
			       UbqUlong             duration);

  virtual void sentFromService(UbqUlong        	    reqId,
			       const VpServiceInfo& serviceInfo,
			       UbqUshort            type,
			       const CString&       string,
			       const UbqOpaque&     data);

  void                 sendMsg(UbqUlong	            reqId,
			       UbqUlong	            receiverId,
			       UbqUshort            type,
			       const CString&       string,
			       const VpNdr&         data);

};


#endif /* __PLACE_H */
