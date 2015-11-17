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

#include <wx/event.h>
#include <wx/string.h>
#include <wx/hashmap.h>
#include <wx/stopwatch.h>
#include <vector>
// BEW requires the next one
#include <wx/dynarray.h>


#include "1035.h"

// all the nice socket includes in one place here
#ifdef _WIN32
// https://stackoverflow.com/questions/5004858/stdmin-gives-error
#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS   /* allow the old inet_addr() call in implementation file */

namespace std {}
using namespace std;

// mingw/ visual studio socket includes
//#include <winsock2.h> <- No! put it in wxServDisc.cpp
#include <ws2tcpip.h>
#define SHUT_RDWR SD_BOTH
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
#endif // _WIN32

#include "mdnsd.h"



// make available custom notify event if getResults() would yield something new
#if wxVERSION_NUMBER < 2900
DECLARE_EVENT_TYPE(wxServDiscNOTIFY, -1);
DEFINE_EVENT_TYPE(wxServDiscHALTING);
DEFINE_EVENT_TYPE(serviceDiscoveryHALTING);
#else
wxDECLARE_EVENT(wxServDiscNOTIFY, wxCommandEvent);
wxDEFINE_EVENT(wxServDiscHALTING, wxCommandEvent);
wxDEFINE_EVENT(serviceDiscoveryHALTING, wxCommandEvent);
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
  wxServDisc(void* parent, const wxString& what, int type);
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



#endif // WXSERVDISC_H



#if defined(_KBSERVER)

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ServiceDiscovery.h
/// \author			Bruce Waters
/// \date_created	10 November 2015
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CServiceDiscovery class.
/// The CServiceDiscovery class automates the detection of the service 
/// _kbserver._tcp.local. (the final period is obligatory) on the LAN.
/// It is used in order to avoid having the user take explicit steps
/// to determine the ipv4 address of the running service to be used
/// when sharing KB data between multiple users within the same AI project.
/// \derivation		The CServiceDiscovery class is derived from wxObject.
/// ======================================================================
/// Thanks to Christian Beier, <dontmind@freeshell.org>
/// who made the SDWrap application available under the GNU General Public License
/// along with the wxServDisc and related software which forms the core of the
/// Zeroconf service discovery wrapper.
/// SDWrap was distributed in the hope that it would be useful, but WITHOUT
/// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
/// FITNESS FOR A PARTICULAR PURPOSE. His original license details are available
/// at the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
/// ======================================================================
/// This CServiceDiscovery class utilizes the six core resources, wxServDisc.h,
/// wxServDisc.cpp, mdnsd.h, mdnsd.c, 1035.h and 1035; and also some parts of
/// the sdwrap application, modified and simplified in order to suit our own
/// requirements - which do not require a GUI, nor spawning of a command
/// process for automated connection to some other resource.
/////////////////////////////////////////////////////////////////////////////

#ifndef SERVICEDISCOVERY_h
#define SERVICEDISCOVERY_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ServiceDiscovery.h"
#endif

class ServDisc;

class CServiceDiscovery : public wxEvtHandler
{

public:
	CServiceDiscovery();
	CServiceDiscovery(CMainFrame* pFrame, wxString servicestring, ServDisc* pParentClass);
	virtual ~CServiceDiscovery();

	wxString m_servicestring; // service to be scanned for
	CMainFrame* m_pFrame; // Adapt It app's frame window

	wxServDisc* m_pSD; // main service scanner
	ServDisc* m_pParent;
	bool m_bWxServDiscIsRunning; // I'll use a FALSE value of this in Frame's OnIdle()
								 // for deleting CServiceDiscovery & ServDisc instances

	// Temporarily store these
	wxString m_hostname;
	wxString m_addr;
	wxString m_port;

	wxArrayString m_sd_items;

    // bools (as int 0 or 1, in int arrays) for error conditions are on the CMainFrame
    // instance and the array of URLs for the one or more _kbserver._tcp.local. services
    // that are discovered is there also. It is NAUGHTY to have two or more KBservers
    // running on the LAN at once, but we can't prevent someone from doing so - when that
    // happens, we'll need to let them choose which URL to connect to. The logic for all
    // that will be in a function called from elsewhere, the function will make use of the
    // data we sent to the CMainFrame class from here. (See MainFrm.h approx lines 262-7)
protected:

	void onSDNotify(wxCommandEvent& event);
	void onSDHalting(wxCommandEvent& event);

private:
	DECLARE_EVENT_TABLE();

    DECLARE_DYNAMIC_CLASS(CServiceDiscovery)
};

#endif // SERVICEDISCOVERY_h

#endif // _KBSERVER



