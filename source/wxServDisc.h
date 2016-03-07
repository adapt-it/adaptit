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

#include <wx/event.h>
#include <wx/string.h>
#include <wx/hashmap.h>
#include <wx/stopwatch.h>

namespace std {}
using namespace std;

// the next is supposed to prevent winsock.h being included in <windows.h>
#define _WINSOCKAPI_
// this is supposed to do the same job
#define WIN32_LEAN_AND_MEAN

#include <vector>

// temporary to get it to compile (BEW 7Mar16)
#ifdef WIN32
typedef UINT_PTR SOCKET;
#else
typedef int SOCKET;
#endif // WIN32

/*
// all the nice socket includes in one place here (I changed _WIN32 to WIN32)
#ifdef WIN32

// the next is supposed to prevent winsock.h being included in <windows.h>
#define _WINSOCKAPI_
// this is supposed to do the same job
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS   // allow the old inet_addr() call in implementation file
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
*/
/*
#include "1035.h"
#include "mdnsd.h"
*/
//#include <wx\event.h>

// Forward declaration
class CServiceDiscovery;

// make available custom notify event if getResults() would yield something new

#if wxVERSION_NUMBER < 2900
DECLARE_EVENT_TYPE(wxServDiscNOTIFY, -1);
DECLARE_EVENT_TYPE(wxServDiscHALTING, -1);
#else
wxDECLARE_EVENT(wxServDiscNOTIFY, wxCommandEvent);
wxDECLARE_EVENT(wxServDiscHALTING, wxCommandEvent);
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

  CServiceDiscovery* m_pSD; // BEW added

  // BEW 25Feb16 moved the guts of the CServiceDiscovery::GetResults() function to here,
  // and calling this new function DiscoverResults(), it needs to internally point back
  // to the parent CServiceDiscovery instance
  void DiscoverResults(CServiceDiscovery* pReportTo);

  // yeah well...
  std::vector<wxSDEntry> getResults() const;
  size_t getResultCount() const;

  // get query name
  const wxString& getQuery() const { const wxString& ref = query; return ref; };
  // get error string
  const wxString& getErr() const { const wxString& ref = err; return ref; };

  // GetResults() takes longer to complete than the service discovery thread, so we can't
  // assume that initiating service discovery halting from the end of onSDNotify will
  // allow clean leak elimination - the latter crashes. So I'm using a mutex and condition
  // approach to cause the main thread to sleep while the service discovery job is done

  // I've left this bool in the solution, but I think it no longer plays any useful function
  bool m_bGetResultsStarted;

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



