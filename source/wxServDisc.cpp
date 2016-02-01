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

// BEW: ************* IMPORTANT **********************
// The wx documentation for threads says that TestDestroy() should be used for determining
// when a thread should be shut down. (Test state of the thread's job, in a loop, and when
// the job is done, have TestDestroy() return true, otherwise, while the job is unfinished
// then return false. TestDestroy() is then typically used in the test condition of the
// thread's loop for processing the job, so as to obtain break from the loop at job end.
// However, this cannot be made to work in Beier's solution. The reason is that Beier's
// solution, on it's own thread, posts a wxServDiscNOTIFY() event to the calling thread
// (which could be the main thread, or in our case, another worker thread), and so the
// calling thread and the wxServDisc thread work on independently. Typically, the
// wxServDisc thread finishes long before the calling thread's onSDNotify() has finished
// the hostname and ip address and port lookups. But both the onSDNotify() handler, and the
// wxServDisc thread, rely on mdnsd.h and .c, and that in turn on 10.35.h and .c. If
// TestDestroy() was used as recommended in the wx documention for determining when
// wxServDisc is to be terminated, doing so would destroy the code resources that the
// running onSDNotify() handler is using to do its job, and crash the app. So our approach
// is to have both the GetResults() handler, and the wx::ServDisc::Entry() function, when
// each is done, post a custom event which I have called wxServDiscHALTING, to the parent
// CServiceDiscovery class instance. It's handler in turn posts a further wxServDiscHALTING
// event to the caller, which is my ServDisc thread - and there the handler for the halt
// can delete the CServiceDiscovery instance. And that handler then also posts a further
// wxServDiscHALTING event to ServDisc's caller, which is the app instance, and the app's
// handler for that event can then delete the ServDisc instance. That's how we get cleanup
// done. Clearly, cleanup requires that the main thread must not be asleep, and since we
// use a mutex and condition in our solution, the Signal() call which ends the main
// thread's sleep must be given before the cleanup event handling does its work, and
// *after* any results have been stored in the app's m_servDiscResults wxArrayString

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#if defined(_KBSERVER) // whm 2Dec2015 added otherwise build breaks in Linux when _KBSERVER is not defined

#include "wx/object.h"
#include "wx/thread.h"
#include "wx/intl.h"
#include "wx/log.h"

#include <fcntl.h>
#include <cerrno>
#include <csignal>

// only used by VC++
#ifdef WIN32

#pragma comment(lib, "Ws2_32.lib")

#endif

#include "wxServDisc.h"
#include "ServiceDiscovery.h"

// Compatability defines
#ifdef __APPLE__
#ifndef IPV6_ADD_MEMBERSHIP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif
#endif

// define our new notify event! (BEW added the ...HALTING one)

//#if wxVERSION_NUMBER < 2900
//DEFINE_EVENT_TYPE(wxServDiscNOTIFY);
//#else
//wxDEFINE_EVENT(wxServDiscNOTIFY, wxCommandEvent);
//#endif

#if wxVERSION_NUMBER < 2900
DEFINE_EVENT_TYPE(wxServDiscHALTING);
DEFINE_EVENT_TYPE(serviceDiscoveryHALTING);
#else
wxDEFINE_EVENT(wxServDiscHALTING, wxCommandEvent);
wxDEFINE_EVENT(serviceDiscoveryHALTING, wxCommandEvent);
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

  d = mdnsd_new(1,1000); // class is 1, frame is 1000

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

	  // BEW addition. If there is no KBserver multicasting, this loop behaves poorly.
	  // It does a few quick iterations, and then suddenly msecs jumps to 86221000 microsecs,
	  // and so it's timeout loop runs for a minute and a half (maybe, but it times out
	  // and does not enter the after-loop cleanup code where my HALTING event gets posted,
	  // so here I'll do a hack. If msecs goes > 86000000, the break from the outer loop.
	  if (msecs > 4000000) // it goes (when nothing discovered) to 86400000, but if I delay
							// when debugging, or if takes a while, msecs > 860000000 may
							// be false and then the break doesn't happen, another wxServDisc
							// gets created and p passed in is NULL, and so the wxPostEvent
							// below post to dest = NULL, which gives wxASSERT error. So,
							// use a smaller limit which msecs when it goes large is likely
							// to be much bigger than. Try 4,000,000 (4 secs)
	  {
		break;  // clean up and shut down the module
	  }
	  // end BEW addition
	  
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
    } // end of outer loop


  // BEW 2Dec15 added cache freeing (d's shutdown is not yet 1, but it doesn't test for it
  // so do this first, as shutdown will be set to 1 in mdnsd_shutdown(d) immediately after
  my_gc(d); // is based on Beier's _gd(d), but removing every instance 
            // of the cached struct regardless in the cache array (it's a sparse array
            // because he puts structs in it by a hashed index)

  // Beier's two cleanup functions (they ignore cached structs)
  mdnsd_shutdown(d);
  mdnsd_free(d);

  if(mSock != INVALID_SOCKET)
    closesocket(mSock);

  // Skip the posting of the halting event if GetResults() has been called, because
  // it will be done from the end of that function if it gets called; otherwise,
  // when m_bGetResultsStart is FALSE still here, then no running KBserver was
  // discovered and we then should do it here. But to get event handling to happen,
  // the main thread needs to be awakened, so Signal() is to be called so that 
  // waiting finishes - so we do that before we post the halting event
  wxLogDebug(_T("wxServDisc::Entry(): m_bGetResultsStarted is %s "),
	  m_bGetResultsStarted ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str());

  if (!m_bGetResultsStarted)
  {
	  // No KBserver was discovered, so GetResults() will not have been called, so
	  // we need to initiate the cleanup of the owning classes from here

	  // BEW: Post a custom serviceDiscoveryHALTING event here, for the parent class to
	  // supply the handler needed for destroying this CServiceDiscovery instance
	  // Note: attempting to precede this with the code which includes the Signal() call
	  // leads to an app crash, because the mutex & associated condition objects are
	  // clobbered before the code gets called - so we must rely on DoServiceDiscovery()'s
	  // WaitTimeout(3500) to awaken the main thread, when no KBserver can be found because
	  // one is not yet running on the LAN
	  wxCommandEvent upevent(wxServDiscHALTING, wxID_ANY);
	  upevent.SetEventObject(this); // set sender

	#if wxVERSION_NUMBER < 2900
	  wxPostEvent((CServiceDiscovery*)parent, upevent);
	#else
	  wxQueueEvent((CServiceDiscovery*)parent, upevent.Clone());
	#endif
	  wxLogDebug(_T("from wxServDisc after timeout of Entry()'s loop, no KBserver running, so posting wxServDiscHALTING event")); 
  }
  wxLogDebug(wxT("wxServDisc %p: scanthread exiting, after loop has ended, now at end of Entry(), returning NULL"), this);

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
#ifdef WIN32
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

#ifdef WIN32
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
	wxLogDebug(wxT("wxServDisc: got answer: ans(mdnsda a, void* arg) just entered"));

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

#ifdef WIN32
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

  m_bGetResultsStarted = FALSE; // Only onSDNotify() sets it to TRUE

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

  wxLogDebug(wxT("In ~wxServDisc() wxServDisc %p: scanthread deleted, wxServDisc destroyed, hostname was '%s', lifetime was %ld"),
	  this, query.c_str(), mWallClock.Time());
  wxLogDebug(wxT("End of ~wxServDisc() Finished call of ~wxServDisc()"));
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
	// BEW Tell the running wxServDisc thread that GetResults() was invoked
	m_bGetResultsStarted = TRUE;
	wxLogDebug(_T("m_pSD->m_bGetResultsStarted SET to TRUE. Doing nonEvent approach.")); // interested 
																	// in the time this log is displayed
	// BEW - the nonEvent approach follows...
	if (parent)
	{
		((CServiceDiscovery*)parent)->m_pSD = this;

		//wxCommandEvent dummy;
		//((CServiceDiscovery*)parent)->onSDNotify(dummy);
		
		((CServiceDiscovery*)parent)->GetResults();
	}

  // Beier's code follows
  /*
  if(parent)
    {
 	  wxLogDebug(_T("post_notify():  posting event")); // BEW added this call
		
		// new NOTIFY event, we got no window id
      wxCommandEvent event(wxServDiscNOTIFY, wxID_ANY);
      event.SetEventObject(this); // set sender

      // Send it
	#if wxVERSION_NUMBER < 2900
		  wxPostEvent((CServiceDiscovery*)parent, event);
	#else
		  wxQueueEvent((CServiceDiscovery*)parent, event.Clone());
	#endif
  }
  */
}

#endif // _KBSERVER // whm 2Dec2015 added otherwise Linux build breaks when _KBSERVER is not defined

