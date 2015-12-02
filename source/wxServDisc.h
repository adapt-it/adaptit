// -*- C++ -*- 
/* 
   wxServDisc.h: wxServDisc API definition

   This file is part of wxServDisc, a crossplatform wxWidgets
   Zeroconf service discovery module.
 
   Copyright (C) 2008 Christian Beier <dontmind@freeshell.org>
 
   wxServDisc is free software; you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation; either version 2 of the License, or 
   (at your option) any later version. 
 
   wxServDisc is distributed in the hope that it will be useful, 
   but WITHOUT ANY WARRANTY; without even the implied warranty of 
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
   GNU General Public License for more details. 
 
   You should have received a copy of the GNU General Public License 
   along with this program; if not, write to the Free Software 
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
*/

#ifndef WXSERVDISC_H
#define WXSERVDISC_H

#if defined(_KBSERVER) // whm 2Dec2015 added otherwise build breaks in Linux when _KBSERVER is not defined

//namespace std {}
//using namespace std;

#include <wx/event.h>
#include <wx/string.h>
#include <wx/hashmap.h>
#include <wx/stopwatch.h>

namespace std {}
using namespace std;

#include <vector>

// all the nice socket includes in one place here (I changed _WIN32 to WIN32)
#ifdef WIN32

// the next is supposed to prevent winsock.h being included in <windows.h>
#define _WINSOCKAPI_
// this is supposed to do the same job
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS   /* allow the old inet_addr() call in implementation file */
// mingw/ visual studio socket includes
#define SHUT_RDWR SD_BOTH

#include <winsock2.h>
#include <ws2tcpip.h>
// BEW requires the next one
#include <wx/dynarray.h>


#else // proper UNIX

typedef int SOCKET;       // under windows, SOCKET is unsigned
#define INVALID_SOCKET -1 // so there also is no -1 return value
#define closesocket(s) close(s) // under windows, it's called closesocket
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#endif // WIN32

#include "1035.h"
#include "mdnsd.h"

// Forward declaration
//class CServiceDiscovery;
#include "ServiceDiscovery.h"

// make available custom notify event if getResults() would yield something new

#if wxVERSION_NUMBER < 2900
DECLARE_EVENT_TYPE(wxServDiscNOTIFY, -1);
DECLARE_EVENT_TYPE(wxServDiscHALTING, -1);
//DECLARE_EVENT_TYPE(serviceDiscoveryHALTING, -1);
#else
wxDECLARE_EVENT(wxServDiscNOTIFY, wxCommandEvent);
wxDECLARE_EVENT(wxServDiscHALTING, wxCommandEvent);
//wxDECLARE_EVENT(serviceDiscoveryHALTING, wxCommandEvent);
#endif

// resource name with ip addr and port number
struct wxSDEntry
{
  wxString name;
  wxString ip;
  int port;
  long time;
  wxSDEntry() { port=0; time=0; }
};


// our main class
class wxServDisc: public wxObject, public wxThreadHelper
{
public:
  // type can be one of QTYPE_A, QTYPE_NS, QTYPE_CNAME, QTYPE_PTR or QTYPE_SRV 
  wxServDisc(void* p, const wxString& what, int type);
  ~wxServDisc();
 
  /// Returns true if service discovery successfully started. If not, getErr() may contain a hint.
  bool isOK() const { return err.length() == 0; };
 
  // yeah well...
  std::vector<wxSDEntry> getResults() const;
  size_t getResultCount() const;

  // get query name
  const wxString& getQuery() const { const wxString& ref = query; return ref; };
  // get error string
  const wxString& getErr() const { const wxString& ref = err; return ref; };

  // onSDNotify() takes longer to complete than the service discovery thread, so we can't
  // assume that initiating service discovery halting from the end of onSDNotify will
  // allow clean leak elimination - the latter crashes. So I'm creating two booleans,
  // each defaults to FALSE, and if onSDNotify() is called, both will be TRUE by the time
  // onSDNotify finishes. Then, in the shutdown code in Entry(), we have a waiting loop which
  // waits for a short interval and checks for the 2nd boolean TRUE, then it will know that
  // module completion and leak elimination can happen, safely we hope... let's see...
  // 
  // Beier, in ~wxServDisc() destructor, has GetThread()->Delete(). But his thread is not
  // a joinable one, it's detached, and as far as I can determine, and despite the wx
  // documentation saying otherwise, Delete() doesn't stop the wxServDisc's thread from
  // running and so it goes on until timeout happens and then cleanup. Because of this,
  // it can be running long after everything else has done their job and been cleaned up,
  // so it must not rely on classes above it being in existence when it nears its end.
  // The following flag is defaulted to FALSE, and if a KBserver is found, it is handled
  // by the calling CServiceDiscovery::onSDNotify() handler, and at the end of the latter
  // a wxServDiscHALTING event is posted to shut everything down; but wxServDisc::Entry()
  // has it's own code for posting that event to CServiceDiscovery instance. If the latter
  // has already been destroyed because onSDNotify has completed, and wxServDisc's thread
  // runs on for a while, when it gets to its wxPostEvent() call, the CServiceDiscovery
  // instances pointer has become null, and so there is an app crash(accessing null ptr).
  // To prevent this, we have onSDNotify(), when it starts to run, set to TRUE the
  // following boolean, and in wxServDisc::Entry() we use that TRUE value to cause the
  // posting of the wxServDiscHALTING event to be skipped; we just let cleanup happen and
  // the thread then destroys itself. wxServDisc::Entry() needs to retain the event posting
  // code, because when no KBserver is running on the LAN, then the only place the event
  // posting that gets the calling classes cleaned up is at the end of wxServDisc::Entry()
  bool m_bSdNotifyStarted;


private:
  SOCKET mSock;
  wxString err;
  void *parent;
  wxString query;
  int querytype; 
WX_DECLARE_STRING_HASH_MAP(wxSDEntry, wxSDMap);
  wxSDMap results;
  wxStopWatch mWallClock;
  
  // this runs as a separate thread
  virtual wxThread::ExitCode Entry();

  // create a multicast 224.0.0.251:5353 socket, windows or unix style
  SOCKET msock(); 
  // send/receive message m
  bool sendm(struct message *m, SOCKET s, unsigned long int ip, unsigned short int port);
  int  recvm(struct message *m, SOCKET s, unsigned long int *ip, unsigned short int *port);
  // callback for the mdns resolver
  static int ans(mdnsda a, void *caller);

  void post_notify();
};

#endif // _KBSERVER // whm 2Dec2015 added otherwise Linux build breaks when _KBSERVER is not defined

#endif // WXSERVDISC_H



