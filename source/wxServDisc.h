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

#include <vector>

#if defined(WIN32)

// the next is supposed to prevent winsock.h being included in <windows.h>
#define _WINSOCKAPI_
// this is supposed to do the same job
#define WIN32_LEAN_AND_MEAN

// to get it to compile (BEW 7Mar16) -- it has the definition of SOCKET in it, which is needed below

#include "WinSock2.h"
#include <wx/msw/winundef.h>
#endif

// to get it to compile (BEW 7Mar16)
#if !defined(WIN32)
typedef int SOCKET;
#endif // WIN32

#include "1035.h"
#include "mdnsd.h"

// Forward declaration
class CServiceDiscovery;

// make available custom notify event if getResults() would yield something new

#if wxVERSION_NUMBER < 2900
DECLARE_EVENT_TYPE(wxServDiscNOTIFY, -1);
#else
wxDECLARE_EVENT(wxServDiscNOTIFY, wxCommandEvent);
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

  unsigned long processID;
  bool m_bKillZombie; // quickly destroy unwanted wxServDisc instances that hang around too long
					  // (this is precautionary, there don't appear to be any created in the
					  // present solution)

  // Returns true if service discovery successfully started. If not, getErr() may contain a hint.
  bool isOK() const { return err.length() == 0; };

  // CheckDeathNeeded checks the CServiceDiscovery instance for its m_bDestroyChildren boolean
  // going TRUE, once that happens the wxServDisc instance exits from the scanning loops and 
  // proceeds to die
  bool CheckDeathNeeded(CServiceDiscovery* pSDParent, int BEWcount, int querytype, bool& exit, bool& bBrokeFromLoop);

  // For communication back to the parent CServiceDiscovery unit, a global (which was gpServiceDiscovery)
  // is unhelpful, because if too Thread_ServiceDiscovery instances overlap temporally, then the shutting
  // down of the first destroys' gpServiceDiscovery, leading to access violation in the second running thread.
  // A better solution is to have a pointer to the particular CServiceDiscovery instance which created the
  // owned wxServDisc instance - then Thread_ServiceDiscovery instances can robustly overlap temporally. This
  // design then means that instead of running the threads from a timer, we can instead run them in a burst
  // of partially overlapping instances - getting what would take 1.5 minutes with a timer approach done in
  // about 20 seconds. The following pointer allows the needed communication with CServiceDiscovery. Of course,
  // the Thread which runs the pointed at CServiceDiscovery instance must exist until no more communications
  // are required. We handle that with event passing, booleans, and wait loops, as required.
  CServiceDiscovery* m_pCSD;

  // yeah well...  <<-- Beier's comment. Not exactly helpful!
  std::vector<wxSDEntry> getResults() const;
  size_t getResultCount() const;

  // BEW 6Apr16 made next one in the hope that it will clear up a lot of leaks - it did
  void clearResults();
 
  // get query name
  const wxString& getQuery() const { const wxString& ref = query; return ref; };
  // get error string
  const wxString& getErr() const { const wxString& ref = err; return ref; };

 private:

  SOCKET    mSock;
  wxString  err;
  void     *parent;
  wxString  query;
  int       querytype; 
  WX_DECLARE_STRING_HASH_MAP(wxSDEntry, wxSDMap);
  wxSDMap   results;
  wxStopWatch mWallClock;

  // this runs as a separate thread
  virtual  wxThread::ExitCode Entry();

  // create a multicast 224.0.0.251:5353 socket, windows or unix style
  SOCKET   msock(); 

  // send/receive message m
  bool     sendm(struct message *m, SOCKET s, unsigned long int ip, unsigned short int port);
  int      recvm(struct message *m, SOCKET s, unsigned long int *ip, unsigned short int *port);
  // callback for the mdns resolver
  static int ans(mdnsda a, void *caller);

  void post_notify();

};

#endif // _KBSERVER // whm 2Dec2015 added otherwise Linux build breaks when _KBSERVER is not defined

#endif // WXSERVDISC_H



