/*
   wxServDisc.cpp: wxServDisc implementation

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
// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
// Include your minimal set of headers here
#include <wx/arrstr.h>
#include <wx/docview.h>
//#include "Adapt_It.h" omit - heaps of clashes with ws2tcpip.h
#include "SourcePhrase.h" // needed for definition of SPList, which MainFrm.h uses
#include "MainFrm.h"
#endif


#include "wx/object.h"
#include "wx/thread.h"
#include "wx/intl.h"
#include "wx/log.h"

#include <fcntl.h>
#include <cerrno>
#include <csignal>

// only used by VC++
#ifdef _WIN32

#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h> // moved there from wxServDisc.h, otherwise get name clashes

#endif

#include "wxServDisc.h"
#include "ServDisc.h"

// Compatability defines
#ifdef __APPLE__
#ifndef IPV6_ADD_MEMBERSHIP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif
#endif

// define our new notify event! (BEW added the ...HALTING one)
#if wxVERSION_NUMBER < 2900
DEFINE_EVENT_TYPE(wxServDiscNOTIFY);
//DEFINE_EVENT_TYPE(wxServDiscHALTING);
#else
wxDEFINE_EVENT(wxServDiscNOTIFY, wxCommandEvent);
//wxDEFINE_EVENT(wxServDiscHALTING, wxCommandEvent);
#endif

/*
  private member functions

*/


wxThread::ExitCode wxServDisc::Entry()
{
  mdnsd d;
  struct message m;
  unsigned long int ip;
  unsigned short int port;
  struct timeval *tv;
  fd_set fds;
  bool exit = false;

  mWallClock.Start();

  d = mdnsd_new(1,1000);

  // register query(w,t) at mdnsd d, submit our address for callback ans()
  mdnsd_query(d, query.char_str(), querytype, ans, this);

#ifdef __WXGTK__
  // this signal is generated when we pop up a file dialog wwith wxGTK
  // we need to block it here cause it interrupts the select() call
  sigset_t            newsigs;
  sigset_t            oldsigs;
  sigemptyset(&newsigs);
  sigemptyset(&oldsigs);
  sigaddset(&newsigs, SIGRTMIN-1);
#endif

  // BEW I want to count the outer loop iterations
  unsigned long BEWcount = 0;

  while(!GetThread()->TestDestroy() && !exit)
    {
      tv = mdnsd_sleep(d);

      long msecs = tv->tv_sec == 0 ? 100 : tv->tv_sec*1000; // so that the while loop beneath gets executed once
      wxLogDebug(wxT("wxServDisc %p: scanthread waiting for data, timeout %i seconds"), this, (int)tv->tv_sec);

	  // BEW put this test here
	  if ((int)tv->tv_sec > 101)
	  {
		  // halt & shut the discovery module down because no KBserver service was discovered
		wxLogDebug(_T("BEW: No KB server discovered. Approx 7 seconds elapsed. Halting now..."));
		// BEW copied next 5 lines from code which follows main loop (which never got called)
		// so hopefully this may clean up without memory leaks
		mdnsd_shutdown(d);
		mdnsd_free(d);
		if(mSock != INVALID_SOCKET)
			closesocket(mSock);
		wxLogDebug(wxT("wxServDisc %p: scanthread exiting"), this);

		// post a custom wxServDiscHALTING event here
        wxCommandEvent event(wxServDiscHALTING, wxID_ANY);
        event.SetEventObject(this); // set sender

        // BEW added this posting...  Send it
#if wxVERSION_NUMBER < 2900
        wxPostEvent((wxEvtHandler*)parent, event);
#else
        wxQueueEvent((wxEvtHandler*)parent, event.Clone());
#endif
		// BEW copied next line from code which follows main loop
		return NULL;
	  } // end of BEW addition

      // we split the one select() call into several ones every 100ms
      // to be able to catch TestDestroy()...
      int datatoread = 0;
      while(msecs > 0 && !GetThread()->TestDestroy() && !datatoread)
	{
	  // the select call leaves tv undefined, so re-set
	  tv->tv_sec = 0;
	  tv->tv_usec = 100000; // 100 ms

	  FD_ZERO(&fds);
	  FD_SET(mSock,&fds);


#ifdef __WXGTK__
	  sigprocmask(SIG_BLOCK, &newsigs, &oldsigs);
#endif
	  datatoread = select(mSock+1,&fds,0,0,tv); // returns 0 if timeout expired

#ifdef __WXGTK__
	  sigprocmask(SIG_SETMASK, &oldsigs, NULL);
#endif

	  if(!datatoread) // this is a timeout
	    msecs-=100;
	  if(datatoread == -1)
	    break;
	}

      wxLogDebug(wxT("wxServDisc %p: scanthread woke up, reason: incoming data(%i), timeout(%i), error(%i), deletion(%i)"),
		 this, datatoread>0, msecs<=0, datatoread==-1, GetThread()->TestDestroy() );

      // receive
      if(FD_ISSET(mSock,&fds))
        {
	  while(recvm(&m, mSock, &ip, &port) > 0)
	    mdnsd_in(d, &m, ip, port);
        }

      // send
      while(mdnsd_out(d,&m,&ip,&port))
	if(!sendm(&m, mSock, ip, port))
	  {
	    exit = true;
	    break;
	  }
	// BEW log what iteration this is:
	BEWcount++;
	wxLogDebug(_T("BEW  outer loop iteration:  %d"), BEWcount);
    }

  mdnsd_shutdown(d);
  mdnsd_free(d);


  if(mSock != INVALID_SOCKET)
    closesocket(mSock);


  wxLogDebug(wxT("wxServDisc %p: scanthread exiting"), this);

  return NULL;
}

bool wxServDisc::sendm(struct message* m, SOCKET s, unsigned long int ip, unsigned short int port)
{
  struct sockaddr_in to;

  memset(&to, '\0', sizeof(to));

  to.sin_family = AF_INET;
  to.sin_port = port;
  to.sin_addr.s_addr = ip;

  if(sendto(s, (char*)message_packet(m), message_packet_len(m), 0,(struct sockaddr *)&to,sizeof(struct sockaddr_in)) != message_packet_len(m))
    {
      err.Printf(_("Can't write to socket: %s\n"),strerror(errno));
      return false;
    }

  return true;
}

int wxServDisc::recvm(struct message* m, SOCKET s, unsigned long int *ip, unsigned short int *port)
{
  struct sockaddr_in from;
  int bsize;
  static unsigned char buf[MAX_PACKET_LEN];
#ifdef __WIN32__
  int ssize  = sizeof(struct sockaddr_in);
#else
  socklen_t ssize  = sizeof(struct sockaddr_in);
#endif


  if((bsize = recvfrom(s, (char*)buf, MAX_PACKET_LEN, 0, (struct sockaddr*)&from, &ssize)) > 0)
    {
      memset(m, '\0', sizeof(struct message));
      message_parse(m,buf);
      *ip = (unsigned long int)from.sin_addr.s_addr;
      *port = from.sin_port;
      return bsize;
    }

#ifdef __WIN32__
  if(bsize < 0 && WSAGetLastError() != WSAEWOULDBLOCK)
#else
    if(bsize < 0 && errno != EAGAIN)
#endif
      {
	err.Printf(_("Can't read from socket %d: %s\n"),
		      errno,strerror(errno));
	return bsize;
      }

  return 0;
}

int wxServDisc::ans(mdnsda a, void *arg)
{
  wxServDisc *moi = (wxServDisc*)arg;

  wxString key;
  switch(a->type)
    {
    case QTYPE_PTR:
      // query result is key
      key = wxString((char*)a->rdname, wxConvUTF8);
      break;
    case QTYPE_A:
    case QTYPE_SRV:
      // query name is key
      key = wxString((char*)a->name, wxConvUTF8);
      break;
    default:
      break;
    }

  // insert answer data into result
  // depending on the query, not all fields have a meaning
  wxSDEntry result;

  result.name = wxString((char*)a->rdname, wxConvUTF8);

  result.time = moi->mWallClock.Time();

  struct in_addr ip;
  ip.s_addr =  ntohl(a->ip);
  result.ip = wxString(inet_ntoa(ip), wxConvUTF8);

  result.port = a->srv.port;

  if(a->ttl == 0)
    // entry was expired
    moi->results.erase(key);
  else
    // entry update
    moi->results[key] = result;

  moi->post_notify();

  wxLogDebug(wxT("wxServDisc %p: got answer:"), moi);
  wxLogDebug(wxT("wxServDisc %p:    key:  %s"), moi, key.c_str());
  wxLogDebug(wxT("wxServDisc %p:    ttl:  %i"), moi, (int)a->ttl);
  wxLogDebug(wxT("wxServDisc %p:    time: %lu"), moi, result.time);
  if(a->ttl != 0) {
    wxLogDebug(wxT("wxServDisc %p:    name: %s"), moi, moi->results[key].name.c_str());
    wxLogDebug(wxT("wxServDisc %p:    ip:   %s"), moi, moi->results[key].ip.c_str());
    wxLogDebug(wxT("wxServDisc %p:    port: %u"), moi, moi->results[key].port);
  }
  wxLogDebug(wxT("wxServDisc %p: answer end"),  moi);

  return 1;
}

// create a multicast 224.0.0.251:5353 socket,
// aproppriate for receiving and sending,
// windows or unix style
SOCKET wxServDisc::msock()
{
  SOCKET sock;

  int multicastTTL = 255; // multicast TTL, must be 255 for zeroconf!
  const char* mcAddrStr = "224.0.0.251";
  const char* mcPortStr = "5353";

  int flag = 1;
  int status;
  struct addrinfo hints;   // Hints for name lookup
  struct addrinfo* multicastAddr;  // Multicast address
  struct addrinfo* localAddr;      // Local address to bind to

#ifdef __WIN32__
  /*
    Start up WinSock
   */
  WORD		wVersionRequested;
  WSADATA	wsaData;
  wVersionRequested = MAKEWORD(2, 2);
  if(WSAStartup(wVersionRequested, &wsaData) != 0)
    {
      WSACleanup();
      err.Printf(_("Failed to start WinSock!"));
      return INVALID_SOCKET;
    }
#endif

  /*
     Resolve the multicast group address
  */
  memset(&hints, 0, sizeof hints); // make sure the struct is empty
  hints.ai_family = PF_UNSPEC; // IPv4 or IPv6, we don't care
  hints.ai_flags  = AI_NUMERICHOST;
  if ((status = getaddrinfo(mcAddrStr, NULL, &hints, &multicastAddr)) != 0) {
    err.Printf(_("Could not get multicast address: %s\n"), gai_strerror(status));
    return INVALID_SOCKET;
  }

  wxLogDebug(wxT("wxServDisc %p: Using %s"), this, multicastAddr->ai_family == PF_INET6 ? wxT("IPv6") : wxT("IPv4"));

  /*
     Resolve a local address with the same family (IPv4 or IPv6) as our multicast group
  */
  memset(&hints, 0, sizeof hints); // make sure the struct is empty
  hints.ai_family   = multicastAddr->ai_family;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags    = AI_NUMERICHOST|AI_PASSIVE; // no name resolving, wildcard address
  if ((status = getaddrinfo(NULL, mcPortStr, &hints, &localAddr)) != 0 ) {
    err.Printf(_("Could not get local address: %s\n"), gai_strerror(status));
    freeaddrinfo(multicastAddr);
    return INVALID_SOCKET;
  }

  /*
    Create socket
  */
  if((sock = socket(localAddr->ai_family, localAddr->ai_socktype, 0)) == INVALID_SOCKET) {
    err.Printf(_("Could not create socket: %s\n"), strerror(errno));
    // not yet a complete cleanup!
    freeaddrinfo(localAddr);
    freeaddrinfo(multicastAddr);
    return INVALID_SOCKET;
  }

  /*
    set to reuse address (and maybe port - needed on OSX)
  */
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));
#if defined (SO_REUSEPORT)
  setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
#endif

  /*
    Bind this socket to localAddr
   */
  if(bind(sock, localAddr->ai_addr, localAddr->ai_addrlen) != 0) {
    err.Printf(_("Could not bind socket: %s\n"), strerror(errno));
    goto CompleteCleanUp;
  }

  /*
    Set the multicast TTL.
  */
  if ( setsockopt(sock,
		  localAddr->ai_family == PF_INET6 ? IPPROTO_IPV6        : IPPROTO_IP,
		  localAddr->ai_family == PF_INET6 ? IPV6_MULTICAST_HOPS : IP_MULTICAST_TTL,
		  (char*) &multicastTTL, sizeof(multicastTTL)) != 0 ) {
    err.Printf(_("Could not set multicast TTL: %s\n"), strerror(errno));
    goto CompleteCleanUp;
   }

   /*
      Join the multicast group. We do this seperately depending on whether we
      are using IPv4 or IPv6.
   */
  if ( multicastAddr->ai_family  == PF_INET &&
       multicastAddr->ai_addrlen == sizeof(struct sockaddr_in) ) /* IPv4 */
    {
      struct ip_mreq multicastRequest;  // Multicast address join structure

      /* Specify the multicast group */
      memcpy(&multicastRequest.imr_multiaddr,
	     &((struct sockaddr_in*)(multicastAddr->ai_addr))->sin_addr,
	     sizeof(multicastRequest.imr_multiaddr));

      /* Accept multicast from any interface */
      multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);

      /* Join the multicast address */
      if ( setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &multicastRequest, sizeof(multicastRequest)) != 0 )  {
	err.Printf(_("Could not join multicast group: %s\n"), strerror(errno));
	goto CompleteCleanUp;
      }

    }
  else if ( multicastAddr->ai_family  == PF_INET6 &&
	    multicastAddr->ai_addrlen == sizeof(struct sockaddr_in6) ) /* IPv6 */
    {
      struct ipv6_mreq multicastRequest;  /* Multicast address join structure */

      /* Specify the multicast group */
      memcpy(&multicastRequest.ipv6mr_multiaddr,
	     &((struct sockaddr_in6*)(multicastAddr->ai_addr))->sin6_addr,
	     sizeof(multicastRequest.ipv6mr_multiaddr));

      /* Accept multicast from any interface */
      multicastRequest.ipv6mr_interface = 0;

      /* Join the multicast address */
      if ( setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char*) &multicastRequest, sizeof(multicastRequest)) != 0 ) {
	err.Printf(_("Could not join multicast group: %s\n"), strerror(errno));
	goto CompleteCleanUp;
      }
    }
  else {
    err.Printf(_("Neither IPv4 or IPv6"));
    goto CompleteCleanUp;
  }

  /*
     Set to nonblock
  */
#ifdef _WIN32
  unsigned long block=1;
  ioctlsocket(sock, FIONBIO, &block);
#else
  flag =  fcntl(sock, F_GETFL, 0);
  flag |= O_NONBLOCK;
  fcntl(sock, F_SETFL, flag);
#endif

  /*
    whooaa, that's it
  */
  freeaddrinfo(localAddr);
  freeaddrinfo(multicastAddr);

  return sock;

 CompleteCleanUp:

  closesocket(sock);
  freeaddrinfo(localAddr);
  freeaddrinfo(multicastAddr);
  return INVALID_SOCKET;
}

/*
  public member functions

*/

wxServDisc::wxServDisc(void* p, const wxString& what, int type)
{
  // save our caller
  parent = p;

  // save query
  query = what;
  querytype = type;

  wxLogDebug(wxT(""));
  wxLogDebug(wxT("wxServDisc %p: about to query '%s'"), this, query.c_str());

  if((mSock = msock()) == INVALID_SOCKET) {
    wxLogDebug(wxT("Ouch, error creating socket: ") + err);
    return;
  }

#if wxVERSION_NUMBER >= 2905 // 2.9.4 still has a bug here: http://trac.wxwidgets.org/ticket/14626
  if( CreateThread(wxTHREAD_DETACHED) != wxTHREAD_NO_ERROR )
#else
  if( Create() != wxTHREAD_NO_ERROR )
#endif
    err.Printf(_("Could not create scan thread!"));
  else
    if( GetThread()->Run() != wxTHREAD_NO_ERROR )
      err.Printf(_("Could not start scan thread!"));
}

wxServDisc::~wxServDisc()
{
  wxLogDebug(wxT("wxServDisc %p: before scanthread delete"), this);
  if(GetThread() && GetThread()->IsRunning())
    GetThread()->Delete(); // blocks, this makes TestDestroy() return true and cleans up the thread

  wxLogDebug(wxT("wxServDisc %p: scanthread deleted, wxServDisc destroyed, query was '%s', lifetime was %ld"), this, query.c_str(), mWallClock.Time());
  wxLogDebug(wxT(""));
}

std::vector<wxSDEntry> wxServDisc::getResults() const
{
  std::vector<wxSDEntry> resvec;

  wxSDMap::const_iterator it;
  for(it = results.begin(); it != results.end(); it++)
    resvec.push_back(it->second);

  return resvec;
}

size_t wxServDisc::getResultCount() const
{
  return results.size();
}

void wxServDisc::post_notify()
{
  if(parent)
    {
      // new NOTIFY event, we got no window id
      wxCommandEvent event(wxServDiscNOTIFY, wxID_ANY);
      event.SetEventObject(this); // set sender

      // Send it
#if wxVERSION_NUMBER < 2900
      wxPostEvent((wxEvtHandler*)parent, event);
#else
      wxQueueEvent((wxEvtHandler*)parent, event.Clone());
#endif
    }
}

#if defined(_KBSERVER)

///////////////////////////////////////////////////////////////////////////
/// NOTE: in the class below, we cannot #include all the wx headers - it results
/// in massive amounts of clashes with winsock2.h etc; just include the ones
/// actually needed, such as (see line 28) <wx/arrstr.h> provided there are no
/// conflicts. Keep to just C++ and std library in CServiceDiscover class, when
/// otherwise conflicts would arise. Or experiment with using namespace ...
///////////////////////////////////////////////////////////////////////////


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
/// the MyFrameMain class, modified and simplified in order to suit our own
/// requirements - which do not require a GUI, nor spawning of a command 
/// process for automated connection to some other resource.
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ServiceDiscovery.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "wxServDisc.h"

IMPLEMENT_DYNAMIC_CLASS(CServiceDiscovery, wxEvtHandler)

BEGIN_EVENT_TABLE(CServiceDiscovery, wxEvtHandler)
	EVT_COMMAND (wxID_ANY, wxServDiscNOTIFY, CServiceDiscovery::onSDNotify)
	EVT_COMMAND (wxID_ANY, wxServDiscHALTING, CServiceDiscovery::onSDHalting)
END_EVENT_TABLE()

CServiceDiscovery::CServiceDiscovery()
{
	m_servicestring = _T("");
	m_bWxServDiscIsRunning = TRUE;
	wxUnusedVar(m_servicestring);
}

CServiceDiscovery::CServiceDiscovery(CMainFrame* pFrame, wxString servicestring, ServDisc* pParentClass)
{
	
	wxLogDebug(_T("\nInstantiating a CServiceDiscovery class, passing in pFrame and servicestring: %s, ptr to instance: %p"),
		servicestring.c_str(), this);

	m_servicestring = servicestring; // service to be scanned for
	m_pFrame = pFrame; // Adapt It app's frame window
	m_pParent = pParentClass; // so we can delete the parent from within this class
	m_bWxServDiscIsRunning = TRUE; // Gets set FALSE only in my added onServDiscHalting() handler
		// and the OnIdle() hander will use the FALSE to get CServiceDiscovery and ServDisc
		// class instances deleted, and app's m_pServDisc pointer reset to NULL afterwards

	m_hostname = _T("");
	m_addr = _T("");
	m_port = _T("");

	// Clear the reporting member variables on the CMainFrame instance, as we are 
	// doing a new discovery attempt
	m_pFrame->m_urlsArr.Clear();
	m_pFrame->m_bArr_ScanFoundNoKBserver.Clear();
	m_pFrame->m_bArr_HostnameLookupFailed.Clear();
	m_pFrame->m_bArr_IPaddrLookupFailed.Clear();

	// wxServDisc creator is: wxServDisc::wxServDisc(void* p, const wxString& what, int type)
	// where p is pointer to the parent class & what is the service to scan for, its type 
	// is QTYPE_PTR
	// ********************** NOTE ******************************
	// The "parent" class for instantiating wxServDisc, is NOT the parent to
	// this CServiceDiscovery class instance! The latter is the pParentClass 
	// passed in; but for wxServDisc, the p parameter is the destination object
	// to which the pending custom event (ie. wxServDiskNOTIFY event) is to be
	// sent -- and that is to this CServiceDiscovery instance. So pass in this
	m_pSD = new wxServDisc(this, m_servicestring, QTYPE_PTR);

	// Try putting a destructor here -- nope the same thread.cpp line 170 m_buffer freed
	// crash happens with this too. 
	//delete this;
}

// When wxServDisc discovers a service (one or more of them) that it is scanning
// for, it reports its success here. Adapt It needs just the ip address(s), and
// maybe the port, so that we can present URL(s) to the AI code which it can use
// for automatic connection to the running KBserver. Ideally, only one KBserver
// runs at any given time, but we have to allow for users being perverse and running
// more than one. If they do, we'll have to let them make a choice of which URL to
// connect to. onSDNotify is an amalgamation of a few separate functions from the
// original sdwrap code. We don't have a gui, so all we need do is the minimal task
// of looking up the ip address and port of each discovered _kbserver service, and
// turning each such into an ip address from which we can construct the appropriate
// URL (one or more) which we'll make available to the rest of Adapt It via a
// wxArrayString plus some booleans for error states, on the CMainFrame class instance.
// The logic for using the one or more URLs will be constructed externally to this
// service discovery module. This service discovery module will just be instantiated,
// scan for _kbserver._tcp.local. , lookup the ip addresses, deposit finished URL or 
// URLs in the wxArrayString in CMainFrame, and then kill itself. We'll probably
// run it at the entry to any Adapt It project, and those projects which are supporting
// KBserver syncing, will then make use of what has been returned, to make the connection
// as simple and automatic for the user as possible. Therefore, onSDNotify will do
// all this stuff, and at the end, kill the module, and it's parent classes.
void CServiceDiscovery::onSDNotify(wxCommandEvent& event)
{
	if (event.GetEventObject() == m_pSD)
	{
		// initialize my reporting context (Use .Clear() rather than
		// .Empty() because from one run to another we don't know if the 
		// number of items discovered will be the same as discovered previously
		m_sd_items.Clear();
		m_pFrame->m_urlsArr.Clear();
		m_pFrame->m_bArr_ScanFoundNoKBserver.Clear();
		m_pFrame->m_bArr_HostnameLookupFailed.Clear();
		m_pFrame->m_bArr_IPaddrLookupFailed.Clear();
		m_hostname.Empty();
		m_addr.Empty();
		m_port.Empty();
		size_t entry_count = 0;
		int timeout;
		// How will the rest of Adapt It find out if there was no _kbserver service
		// discovered? That is, nobody has got one running on the LAN yet.
		// Answer: m_pFrame->m_urlsArr will be empty, and m_pFrame->m_bArr_ScanFoundNoKBserver
		// will also be empty. The wxServDisc module doesn't post a wxServDiscNotify event,
		// as far as I know, if no service is discovered, so we can't rely on this
		// handler being called if what we want to know is that no KBserver is currently
		// running. Each explicit attempt to run this service discovery module will start
		// by clearing the results arrays on the CFrameInstance.

		// length of query plus leading dot
		size_t qlen = m_pSD->getQuery().Len() + 1;
#if defined(_DEBUG)
		wxString theQuery = m_pSD->getQuery(); // temp, to see what's in it
		wxLogDebug(_T("theQuery:  %s  Its Length plus 1: %d"), theQuery.c_str(), (int)qlen);
#endif
		vector<wxSDEntry> entries = m_pSD->getResults();
		vector<wxSDEntry>::const_iterator it;
		for (it = entries.begin(); it != entries.end(); it++)
		{
#if defined(_DEBUG)
			// let's have a look at what is returned
			wxString aName = it->name;
			int nameLen = aName.Len();
			wxString ip = it->ip;
			int port = it->port;
			long time = it->time;
			wxLogDebug(_T("name: %s  Len(): %d  ip: %s  port: %d  time: %d"), 
				aName.c_str(), nameLen, ip.c_str(), port, time);
#endif
			// what's really wanted - just the bit before the first ._ sequence
			entry_count++;
#if defined(_DEBUG)
			wxLogDebug(_T("m_sd_items receives string:  %s   for entry index = %d"), 
				it->name.Mid(0, it->name.Len() - qlen), entry_count - 1);
#endif
			m_sd_items.Add(it->name.Mid(0, it->name.Len() - qlen));
			m_pFrame->m_bArr_ScanFoundNoKBserver.Add(0); // add FALSE, as we successfully
				// discovered one (but that does not necessarily mean we will
				// subsequently succeed at geting hostname, port, and ip address)

		} // end of loop: for (it = entries.begin(); it != entries.end(); it++)

		// Next, lookup hostname and port. Code for this is copied and tweaked,
		// from the list_select() event handler of MyFrameMain. Since we have
		// this now in a loop in case more than one KBserver is running, we
		// must then embed the associated addrscan() call within the loop
		size_t i;
		for (i = 0; i < entry_count; i++)
		{
			wxServDisc namescan(0, m_pSD->getResults().at(i).name, QTYPE_SRV);

			timeout = 3000;
			while(!namescan.getResultCount() && timeout > 0)
			{
				wxMilliSleep(25);
				timeout-=25;
			}
			if(timeout <= 0)
			{
				wxLogError(_("Timeout looking up hostname. Entry index: %d"), i);
				m_hostname = m_addr = m_port = wxEmptyString;
				m_pFrame->m_bArr_HostnameLookupFailed.Add(1); // adding TRUE
				m_pFrame->m_bArr_IPaddrLookupFailed.Add(-1); // because we won't try addrscan() for this index
#if defined(_DEBUG)
				wxLogDebug(_T("Found: [Service:  %s  ] Timeout:  m_hostname:  %s   m_port  %s   for entry index = %d"), 
				m_sd_items.Item(i).c_str(), m_hostname.c_str(), m_port.c_str(), i);
#endif
				//return;
				continue; // don't return, we need to try every iteration
			}
			else
			{
				// The namescan found something...
				m_hostname = namescan.getResults().at(0).name;
				m_port = wxString() << namescan.getResults().at(0).port;
				m_pFrame->m_bArr_HostnameLookupFailed.Add(0); // adding FALSE
#if defined(_DEBUG)
				wxLogDebug(_T("Found: [Service:  %s  ] Looked up:  m_hostname:  %s   m_port  %s   for entry index = %d"), 
				m_sd_items.Item(i).c_str(), m_hostname.c_str(), m_port.c_str(), i);
#endif
				// For each successful namescan(), we must do an addrscan, so as to fill
				// out the full info needed for constucting a URL; if the namescan was
				// not successful, the m_bArr_IPaddrLookupFailed entry for this index
				// should be neither true (1) or false (0), so use -1 for "no test was made"
				{
					wxServDisc addrscan(0, m_hostname, QTYPE_A);
			  
					timeout = 3000;
					while(!addrscan.getResultCount() && timeout > 0)
					{
						wxMilliSleep(25);
						timeout-=25;
					}
					if(timeout <= 0)
					{
						wxLogError(_("Timeout looking up IP address."));
						m_hostname = m_addr = m_port = wxEmptyString;
						m_pFrame->m_bArr_IPaddrLookupFailed.Add(1); // for TRUE
#if defined(_DEBUG)
						wxLogDebug(_T("Found: [Service:  %s  ] Timeout:  ip addr:  %s   for entry index = i"), 
						m_sd_items.Item(i).c_str(), m_addr.c_str(), i);
#endif
						//return;
						continue; // do all iterations
					}
					else
					{
						// succeeded in getting the service's ip address
						m_addr = addrscan.getResults().at(0).ip;
						m_pFrame->m_bArr_IPaddrLookupFailed.Add(0); // for FALSE
#if defined(_DEBUG)
						wxLogDebug(_T("Found: [Service:  %s  ] Looked up:  ip addr:  %s   for entry index = i"), 
						m_sd_items.Item(i).c_str(), m_addr.c_str(), i);
#endif
					}
				} // end of TRUE block for namescan() finding something

			} // end of else block for test: if(timeout <= 0) for namescan() attempt

			// Put it all together to get URL(s) & store in CMainFrame's m_urlsArr.
			// Since we here are still within the loop, we are going to try create a
			// url for what this iteration has succeeded in looking up. We can do so
			// provided hostname, ip, and port are not empty strings. We'll let
			// port be empty, as long as hostname and ip are not empty. ( I won't
			// construct a url with :port appended, unless Jonathan says I should.)
			wxString protocol = _T("https://");
			if (m_pFrame->m_bArr_HostnameLookupFailed.Item(i) == 0 &&
				m_pFrame->m_bArr_IPaddrLookupFailed.Item(i) == 0)
			{
				m_pFrame->m_urlsArr.Add(protocol + m_addr);
#if defined(_DEBUG)
				wxLogDebug(_T("Found: [Service:  %s  ] URL:  %s   for entry index = %d"), 
						m_sd_items.Item(i).c_str(), (protocol + m_addr).c_str(), i);
#endif
			}
		} // end of loop: for (i = 0; i < entry_count; i++)

	} // end of TRUE block for test: if (event.GetEventObject() == m_pSD)
	else
	{
		// major error, so don't wipe out what an earlier run may have
		// stored in the CMainFrame instance
		;
	} // end of else block for test: if (event.GetEventObject() == m_pSD)

	// here is where we might clear any heap blocks remaining, and kill the module
	// At this point, the destructor of the scan thread (code embedded within
	// ~wxServDisc() ) has been run, because ~wxServDisc() has been run - I didn't
	// need to cause that) has destroyed the tread. It remains only to destroy
	// this CServiceDiscovery instance, and its parent ServDisc instance
	// BUT: if there is no service to find, wxServDisc clobbers itself early
	// without calling onSDNotify() and so my delete this call is not done, and
	// memory leaks result. Consider posting a custom event from ~wxServDisc()
	// with a handler in CServiceDiscovery instance that gets it and parent etc
	// deleted from the heap...
	// If I debug slowly enough, the wxServDisc thread will be restarted - my last
	// output had two sets of log entries indicating the ~wxServDisc() destructor
	// was called.  Try explicitly calling it again here....
	m_pSD->~wxServDisc();
	delete m_pSD;
	delete this;
}

// BEW Getting the module shut down in the two circumstances we need, is a can of worms.
// (a) after one or more KBserver instances running have been discovered (onServDiscNotify()
// gets called, and we can put shut-down code in the end of that handler - but I may need
// to improve on it some more)
// (b) when no KBserver instance is runnning (I use an onServDiscHalting() handler of my
// own to get ~wxServDisc() destructor called, but that's all it can do safely).
// 
// In the (b) case, my CServiceDiscovery instance, and my ServDisc instance, live on. 
// So I've resorted to a hack, using a m_bWxServDiscIsRunning boolean, set to FALSE
// when no KBserver was discovered, at end of the ...Halting() handler,
// and the OnIdle() handler in frame window, to get the latter two classes clobbered.
// 
// SDWrap embedded the wxServDisc instance in the app, and so when the app got shut down,
// the lack of well-designed shutdown code didn't matter, as everything got blown away. 
// But for me, ServDisc needs to exist only for a single try at finding a KBserver instance,
// and then die forever - or until explicitly instantiated again (which currently, 
// requires a relaunch of AI)
void CServiceDiscovery::onSDHalting(wxCommandEvent& event)
{
	wxUnusedVar(event);
	// BEW made this handler, for shutting down the module when no KBserver
	// service was discovered
	wxLogDebug(wxT("BEW: No KBserver found, removing module by means of onSDHalting()"));
	wxLogDebug(_T("this [ from CServiceDiscovery:onSDHalting() ] = %p"), this);
	wxLogDebug(_T("m_pParent [ from CServiceDiscovery:onSDHalting() ] = %p"), m_pParent);
	delete m_pSD; // must have this, it gets ~wxServDisc() destructor called
	m_bWxServDiscIsRunning = FALSE;
	m_pParent->m_bSDIsRunning = FALSE; // enables our code in CMainFrame::OnIdle() to
		// figure out that no KBserver was found and so a partial removal of the
		// module has been done so far (ie. wxServDisc instance only), and so the
		// class ServDisc (the parent of the CServiceDiscovery instance) can be
		// deleted from within OnIdle() when this flag is FALSE, and the app's 
		// m_pServDisc pointer reset to NULL. That doesn't however get the 
		// CServiceDiscovery instance deleted, so it will leak memory unless we can
		// get it deleted from within itself. Trying to do it from elsewhere means
		// that #include wxServDisc has to be mixed with #include "Adapt_It.h" and
		// that leads to hundreds of name clashes.
		// Ugly hacks, but hey, I didn't write the wxServDisc code.
	//delete this;  // must not call this here, let this handler complete, otherwise
	// event.cpp, line 1211, when entered, will crash because m_buffer has been freed
	
	// So far I'm at an impass; I can't delete this ptr from within an event being handled
	// by this; and name conflicts prevent me doing it from anywhere where Adapt_It.h is
	// in scope! Ouch.
}

CServiceDiscovery::~CServiceDiscovery()
{
	wxLogDebug(_T("Deleting the CServiceDiscovery instance"));
}



#endif // _KBSERVER