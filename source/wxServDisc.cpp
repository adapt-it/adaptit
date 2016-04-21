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

// BEW: ************* IMPORTANT An Overview **********************
// The wx documentation for threads says that TestDestroy() should be used for determining
// when a thread should be shut down. (Test state of the thread's job, in a loop, and when
// the job is done, have TestDestroy() or exit return true, otherwise, while the job is
// unfinished return false. TestDestroy() is then typically used in the test condition of the
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
// running onSDNotify() handler is using to do its job, and crash the app. To get control
// to break from the loop in Entry() we resort to a hack. At all relevant points in the
// code in the loop, we test for shutdown conditions. There are two subtests separated by
// || (OR), one for when a KBserver has been discovered, the other for when no running
// KBserver is available for discovery. Beier's solution becomes obstreperous if left
// running too long, and trying to support multiple discoveries in the one run results
// in a plethora of memory leaks. I've settled on discovering just one - which can be
// done from a background thread under timer control, and so discover multiples that way.

// So our approach is to have the wx::ServDisc::Entry() function, when halting is to be
// done, forced to break from the outer loop in Entry() and so do the cleanup functions.
// After the cleanups are done (and I had to add to them because Beier's code leaked like
// a sieve), I post a custom event which I have called wxServDiscHALTING, to the parent
// CServiceDiscovery class instance. It's handler, onSDNotify(), then accomplishes 3
// necessary things for shutdown. Calling the cleanup functions on the one wxServDisc
// originally instantiated to kick the whole discovery process off. Destroying that
// instance after that. Finally, transferring from CServiceDiscovery any results that
// are needed for the user to peruse from the main thread's GUI, in order to select
// the KBserver he wants to connect to.

// **********************************  BEWARE  ********************************************
// wxServDisc and CAdapt_ItApp must *NEVER* see each other by means of #include statements.
// If they do you will get hundreds of name conflicts with the socket resources code.
// **********************************  BEWARE  ********************************************

// For logging support, uncomment-out the following #defines. Tracking the Zeroconf-based
// behaviours is difficult without the (extensive) logging provided; most are turned on with
// _zero_, a minimal few additional ones are turned on with _minimal_; _shutdown_ was the 
// most helpful when I was tracking down the cause of occasional access violations
#define _zero_
//#define _minimal_
#define _shutdown_

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#if defined(_KBSERVER) // whm 2Dec2015 added otherwise build breaks in Linux when _KBSERVER is not defined

#include "wx/object.h"
#include "wx/thread.h"
#include "wx/intl.h"
#include "wx/log.h"
#include "wx/utils.h"

#include <fcntl.h>
#include <cerrno>
#include <csignal>

extern wxMutex	kbsvr_arrays;

// only used by VC++
#ifdef WIN32

#pragma comment(lib, "Ws2_32.lib")

#endif

#ifdef WIN32

// the next is supposed to prevent winsock.h being included in <windows.h>
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_

#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS   // allow the old inet_addr() call in implementation file
// mingw/ visual studio socket includes
#define SHUT_RDWR SD_BOTH

#include <winsock2.h>
#include <ws2tcpip.h>
// BEW requires the next one
#include <wx/dynarray.h>

#else // proper UNIX

#define INVALID_SOCKET -1 // so there also is no -1 return value
#define closesocket(s) close(s) // under windows, it's called closesocket
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#endif // WIN32

#include "wxServDisc.h"
#include "ServiceDiscovery.h"

// A low cost hack used for passing data back to CServiceDiscovery
// (yes I know globals are evil)
extern CServiceDiscovery* gpServiceDiscovery; 

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
#else
wxDEFINE_EVENT(wxServDiscNOTIFY, wxCommandEvent);
#endif

#if wxVERSION_NUMBER < 2900
DEFINE_EVENT_TYPE(wxServDiscHALTING);
#else
wxDEFINE_EVENT(wxServDiscHALTING, wxCommandEvent);
#endif

/*   private member functions    */
wxThread::ExitCode wxServDisc::Entry()
{
	processID = wxGetProcessId();
#if defined(_shutdown_)
	wxLogDebug(_T("wxServDisc(): %p   Entry(): I am process ID = %lx"), this, processID);
#endif
	mdnsd d;
	struct message m;
	unsigned long int ip;
	unsigned short int port;
	struct timeval *tv;
	fd_set fds;
	bool exit = false;

	// If this instance is a zombie, then get it shut down pronto
	if (!m_bKillZombie)
	{
		mWallClock.Start();

		d = mdnsd_new(1, 1000); // class is 1, frame is 1000

		// register query(w,t) at mdnsd d, submit our address for callback ans() <<-- this
		// callback was a memory leak source, which I plugged with a clearResults() function
		mdnsd_query(d, query.char_str(), querytype, ans, this);

#ifdef __WXGTK__
		// this signal is generated when we pop up a file dialog wwith wxGTK
		// we need to block it here cause it interrupts the select() call
		sigset_t            newsigs;
		sigset_t            oldsigs;
		sigemptyset(&newsigs);
		sigemptyset(&oldsigs);
		sigaddset(&newsigs, SIGRTMIN - 1);
#endif

		// BEW I want to count the outer loop iterations, for debug logging purposes - very helpful
		unsigned long BEWcount = 0;
		// If shutdown is wanted from within an inner loop, we need to break from that loop and the
		// outer loop too without doing any more mdnsd etc accesses, so these booleans enable that to happen
		bool bBrokeFromInnerLoop = FALSE;
		bool bBrokeFromInnerLoop2 = FALSE;
		bool bBrokeFromInnerLoop3 = FALSE;

		while (!GetThread()->TestDestroy() && !exit)
		{
			tv = mdnsd_sleep(d);

			long msecs = tv->tv_sec == 0 ? 100 : tv->tv_sec * 1000; // so that the while loop beneath gets executed once
#if defined(_zero_) // comment out next one if it generates too much pointless logging; owned one and children use it concurrently
//			wxLogDebug(wxT("wxServDisc %p: (204) scanthread waiting for data, timeout %i seconds"), this, (int)tv->tv_sec);
#endif
			if ((m_bDestroyChildren == TRUE) || ((gpServiceDiscovery->m_postNotifyCount == 0) && (BEWcount > 11)))
			{
				// Force shutdown once the owning wxServDisc instance sets this instance's
				// m_bDestroyChildren flag to TRUE, or if no discovery has been made by the
				// time 9 iterations of the loop have been done - 9 is enough to determine
				// that there are no KBservers running on the LAN currently. Note, these
				// two tests are first, because they do not rely on gpServiceDiscovery
				// being non-NULL. The latter can go NULL before forced shutdown succeeded, so
				// I had to alter the code to have this order of the tests
#if defined (_shutdown_)
				wxLogDebug(wxT("line 216 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx"),
					this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
				exit = true;
				break;
			}
			else if ((gpServiceDiscovery != NULL) && !gpServiceDiscovery->m_ipAddrs_Hostnames.empty())
			{
				// Once the ~wxServDisc() destructor has been called at least twice, and provided
				// that there has been at least one string stored in m_ipAddrs_Hostnames, force
				// shutdown of this instance
#if defined (_shutdown_)
				wxLogDebug(wxT("line 228 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx"),
					this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
				exit = true;
				break;
			}

			// we split the one select() call into several ones every 100ms
			// to be able to catch TestDestroy()...
			int datatoread = 0;
			while (msecs > 0 && !GetThread()->TestDestroy() && !datatoread)
			{
				// the select call leaves tv undefined, so re-set
				tv->tv_sec = 0;
				tv->tv_usec = 100000; // 100 ms

				if ((m_bDestroyChildren == TRUE) || ((gpServiceDiscovery->m_postNotifyCount == 0) && (BEWcount > 11)))
				{
					bBrokeFromInnerLoop = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 248 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}
				else if ((gpServiceDiscovery != NULL) && !gpServiceDiscovery->m_ipAddrs_Hostnames.empty())
				{
					bBrokeFromInnerLoop = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 258 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}

				FD_ZERO(&fds);

				if ((m_bDestroyChildren == TRUE) || ((gpServiceDiscovery->m_postNotifyCount == 0) && (BEWcount > 11)))
				{
					bBrokeFromInnerLoop = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 271 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}
				else if ((gpServiceDiscovery != NULL) && !gpServiceDiscovery->m_ipAddrs_Hostnames.empty())
				{
					bBrokeFromInnerLoop = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 281 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}

				FD_SET(mSock, &fds);

				if ((m_bDestroyChildren == TRUE) || ((gpServiceDiscovery->m_postNotifyCount == 0) && (BEWcount > 11)))
				{
					bBrokeFromInnerLoop = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 294 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}
				else if ((gpServiceDiscovery != NULL) && !gpServiceDiscovery->m_ipAddrs_Hostnames.empty())
				{
					bBrokeFromInnerLoop = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 304 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}

#ifdef __WXGTK__
				sigprocmask(SIG_BLOCK, &newsigs, &oldsigs);
#endif
				datatoread = select(mSock + 1, &fds, 0, 0, tv); // returns 0 if timeout expired

				if ((m_bDestroyChildren == TRUE) || ((gpServiceDiscovery->m_postNotifyCount == 0) && (BEWcount > 11)))
				{
					bBrokeFromInnerLoop = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 320 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}
				else if ((gpServiceDiscovery != NULL) && !gpServiceDiscovery->m_ipAddrs_Hostnames.empty())
				{
					bBrokeFromInnerLoop = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 330 wxServDisc %p:FORCING SHUTDOWN: m_ipAddrs_Hostnames empty? %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx , gpServiceDiscovery = %lx"),
						this, gpServiceDiscovery->m_ipAddrs_Hostnames.empty() ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), 
						BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}

#ifdef __WXGTK__
				sigprocmask(SIG_SETMASK, &oldsigs, NULL);
#endif

				if (!datatoread) // this is a timeout
					msecs -= 100;

				if ((m_bDestroyChildren == TRUE) || ((gpServiceDiscovery->m_postNotifyCount == 0) && (BEWcount > 11)))
				{
					bBrokeFromInnerLoop = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 348 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}
				else if ((gpServiceDiscovery != NULL) && !gpServiceDiscovery->m_ipAddrs_Hostnames.empty())
				{
					bBrokeFromInnerLoop = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 358 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}

				if (datatoread == -1)
				{
					bBrokeFromInnerLoop = TRUE;
					break;
				}
			} // end of first inner loop
#if defined(_minimal_)
			wxLogDebug(wxT("wxServDisc %p: scanthread woke up, reason: incoming data(%i), timeout(%i), error(%i), deletion(%i)"),
				this, datatoread > 0, msecs <= 0, datatoread == -1, GetThread()->TestDestroy());
#endif
			// If shutting down, and bBrokeFromInnerLoop is TRUE, no more calls to mdnsd code can be made, because gpServiceDiscovery
			// can at this point be NULL, and the owned wxServDisc instance may have been destroyed, so use the boolean to force
			// an immediate outer loop break
			if (bBrokeFromInnerLoop)
			{
				break; // from outer loop too
			}
			// receive
			if (FD_ISSET(mSock, &fds))
			{
				if ((m_bDestroyChildren == TRUE) || ((gpServiceDiscovery->m_postNotifyCount == 0) && (BEWcount > 11)))
				{
					bBrokeFromInnerLoop = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 389 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}
				else if ((gpServiceDiscovery != NULL) && !gpServiceDiscovery->m_ipAddrs_Hostnames.empty())
				{
					bBrokeFromInnerLoop = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 399 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}

				while (recvm(&m, mSock, &ip, &port) > 0)
				{
					if ((m_bDestroyChildren == TRUE) || ((gpServiceDiscovery->m_postNotifyCount == 0) && (BEWcount > 11)))
					{
						bBrokeFromInnerLoop2 = TRUE;
#if defined (_shutdown_)
						wxLogDebug(wxT("line 412 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx"),
							this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
						exit = true;
						break;
					}
					else if ((gpServiceDiscovery != NULL) && !gpServiceDiscovery->m_ipAddrs_Hostnames.empty())
					{
						bBrokeFromInnerLoop2 = TRUE;
#if defined (_shutdown_)
						wxLogDebug(wxT("line 422 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx ,gpServiceDiscovery = %lx"),
							this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
						exit = true;
						break;
					}

					mdnsd_in(d, &m, ip, port);

					if ((m_bDestroyChildren == TRUE) || ((gpServiceDiscovery->m_postNotifyCount == 0) && (BEWcount > 11)))
					{
						bBrokeFromInnerLoop2 = TRUE;
#if defined (_shutdown_)
						wxLogDebug(wxT("line 435 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx"),
							this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
						exit = true;
						break;
					}
					else if ((gpServiceDiscovery != NULL) && !gpServiceDiscovery->m_ipAddrs_Hostnames.empty())
					{
						bBrokeFromInnerLoop2 = TRUE;
#if defined (_shutdown_)
						wxLogDebug(wxT("line 445 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx , gpServiceDiscovery = %lx"),
							this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
						exit = true;
						break;
					}
				} // end of second inner loop: while (recvm(&m, mSock, &ip, &port) > 0)
			}
			// break from outer loop immediately if either bBrokeFromInnerLoop or bBrokeFromInnerLoop2 is TRUE
			if (bBrokeFromInnerLoop || bBrokeFromInnerLoop2)
			{
				break;
			}

			// send
			while (mdnsd_out(d, &m, &ip, &port))
			{
				if ((m_bDestroyChildren == TRUE) || ((gpServiceDiscovery->m_postNotifyCount == 0) && (BEWcount > 11)))
				{
					bBrokeFromInnerLoop3 = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 466 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}
				else if ((gpServiceDiscovery != NULL) && !gpServiceDiscovery->m_ipAddrs_Hostnames.empty())
				{
					bBrokeFromInnerLoop3 = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 476 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}

				if (!sendm(&m, mSock, ip, port))
				{
					bBrokeFromInnerLoop3 = TRUE;
					exit = true;
					break;
				}

				if ((m_bDestroyChildren == TRUE) || ((gpServiceDiscovery->m_postNotifyCount == 0) && (BEWcount > 11)))
				{
					bBrokeFromInnerLoop3 = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 494 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}
				else if ((gpServiceDiscovery != NULL) && !gpServiceDiscovery->m_ipAddrs_Hostnames.empty())
				{
					bBrokeFromInnerLoop3 = TRUE;
#if defined (_shutdown_)
					wxLogDebug(wxT("line 504 wxServDisc %p:FORCING SHUTDOWN: m_bDestroyChildren %s , BEWcount %ld , Query type %d , gpServiceDiscovery = %lx , gpServiceDiscovery = %lx"),
						this, m_bDestroyChildren ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str(), BEWcount, querytype, gpServiceDiscovery);
#endif
					exit = true;
					break;
				}
			} // end of third inner loop: while (mdnsd_out(d, &m, &ip, &port))
			BEWcount++; // BEW log what iteration this is: this is NOT cosmetic, it's used for shutdown when no KBserver is running

			// If control broke from the third inner loop, break immediately from the outer loop too
			if (bBrokeFromInnerLoop3)
			{
				break;
			}

#if defined(_minimal_)
			wxLogDebug(_T("BEW wxServDisc: %p   Entry()'s outer loop iteration:  %d"), this, BEWcount);
#endif
		} // end of outer loop

		// Beier's cleanup functions, which I augmented with my own my_gc() which handles
		// _cache struct deletions - something he forgot or didn't bother to do
		// Note, gpServiceDiscovery can go NULL, and the owned wxServDisc instance be destroyed, before
		// control gets here in a child instance, so only do the following cleanups if gpServiceDiscovery
		// is not null
		if (gpServiceDiscovery != NULL)
		{
#if defined(_zero_)
			wxLogDebug(_T("wxServDisc: %p  (532) Entry(): my_gc(d) & friends, about to be called "), this);
#endif
			my_gc(d); // Is based on Beier's _gd(d), but removing every instance of the
					  // cached struct by brute force in the cache array (it's a
					  // sparse array because he puts structs in it by a hashed index)
			// Beier's two cleanup functions (they ignore cached structs, & therefore leaked memory,  which my_gc() fixes)
			mdnsd_shutdown(d);
			mdnsd_free(d);

			if (mSock != INVALID_SOCKET)
				closesocket(mSock);
#if defined(_zero_)
			wxLogDebug(_T("wxServDisc: %p  (544) Entry():  Executed the cleanup functions for this instance "), this);
#endif
		}
		// BEW 7Apr16, Post a custom wxServDiscHALTING event to CServiceDiscovery instance to get
		// the original (owned) wxServeDisc instance deleted; we can't do it from within itself, 
		// so we post this event to ask CServiceDiscovey to oblige instead, in the 
		// onSDHalting() handler - but we only do so from the wxServDisc instance that is the one
		// we can't delete from - the one that CServiceDiscovery created to kick discovery off.
		// BEW 18Apr16, control can get here (due to the joinable thread having been deleted) with
		// gpServiceDiscovery set to NULL, so we cannot have any access to it from here on...
		//if (parent) // Beier's original test
		if ((void*)gpServiceDiscovery->m_pWxSD == (void*)this)
		{
			if (gpServiceDiscovery != NULL) // don't post if the destination class has been deleted
			{
				wxCommandEvent event(wxServDiscHALTING, wxID_ANY);
				event.SetEventObject(this); // set sender
#if wxVERSION_NUMBER < 2900
				wxPostEvent(gpServiceDiscovery, event); // first param indicates which object is to receive the event
#if defined(_zero_)
				wxLogDebug(_T("wxServDisc() %p (564) In Entry(): About to post wxServDiscHALTING event"),
					this, (void*)parent);
#endif
#else
				wxQueueEvent(gpServiceDiscovery, event.Clone());
#if defined(_zero_)
				wxLogDebug(_T("\nIn Entry (570) wxServDisc %p  &  parent %p: About to post wxServDiscHALTING event"),
					this, (void*)parent);
#endif
#endif
#if defined (_shutdown_)
				wxLogDebug(_T("InEntry (575) wxServDisc %p  Just posted event wxServDiscHALTING to CServiceDiscovery instance"), this);
#endif
			} // end of TRUE block for test: if (gpServiceDiscovery != NULL)
			else
			{
				wxNO_OP;
#if defined (_shutdown_)
				wxLogDebug(_T("wxServDisc %p (582) In Entry(): gpServiceDiscovery now NULL, so no post of wxServDiscHALTING before Entry() returns"), this);
#endif
			}
		}

		// The namescan() and addrscan() child processes must terminate before the parent
		// wxServDisc instance, which the CServiceDiscovery instance created, is deleted
		// (otherwise the child ones live on and then access violations crash the app).
		// So here we must provide tests for those two child processes, and post custom
		// events to the CServiceDiscovery instance for the latter's handlers to set
		// two booleans, m_bNamescanIsEnded & m_bAddrscanIsEnded to TRUE. Only after both
		// are TRUE can the top level thread, Thread_ServiceDiscovery()'s TestDestroy()
		// function return TRUE, in order to delete the thread, CServiceDiscovery instance,
		// the owned (parent) wxServDisc instance, and various other things, robustly
		//if ((parent == NULL) && (this->m_bDestroyChildren == TRUE) &&
		//	(this->querytype == QTYPE_SRV) && (gpServiceDiscovery != NULL))
		if (gpServiceDiscovery != NULL)
		{
			if ((parent == gpServiceDiscovery->m_pWxSD) && (this->m_bDestroyChildren == TRUE) &&
				(this->querytype == QTYPE_SRV))
			{
				// I am a namescan() instance - there is only one of these per service discovery run,
				// and I (because I am a detached thread) am about to disappear. So tell the 
				// CServiceDiscovery instance that I'm departing the scene so it can set its
				// m_bNamescanIsEnded boolean to TRUE
				wxCommandEvent eventCustom_EndNamescan(wxEVT_End_Namescan);

				// BEW 20Apr16. Analysis of logging of the timer-based runs indicates that all the
				// successful (non-access-violation) runs have the order of posting events:
				// QTYPE_A one (eventCustom_EndAddrscan) is posted before QTYPE_SRV (eventCustom_EndNamescan)
				// and as soon as the opposite order happens, there is an access violation crash because the
				// addrscan() is still trying to kill itself while the whole shutdown & destruction sequence
				// has completed - destroying the code resources on which the addrscan() wxServDisc instance's
				// destructor relies on - the crash happens in the wx pending events critical section.
				// All the shutdown stuff happens, including the flags m_bNamescanIsEnded and m_bAddrscanIsEnded
				// going TRUE, within a single millisecond - whether the shutdown is successful or crashes. What
				// is going on is that addrscan() takes slightly longer to progress from the posting of the event
				// to get m_bAddrscanIsEnded set TRUE, than does namescan() for it's event posting. Then, when
				// addrscan() is thus delayed a little, both flags have gone TRUE, the thread has been told to
				// shut down, and it does so, deleting the owned wxServDisc, and CServiceDiscovery, and setting
				// gpServiceDiscovery global pointer to NULL. In the time taken for this to happen, the wxServDisc
				// which is for the QTYPE_A scan, has control preceding the final wxLogDebug call in Entry(),
				// and so that log call shows gpServiceDiscovery with value 0  - meaning that a subsequent
				// access violation is inevitable because addrscan is still running but the code has disappeared.
				// To fix this problem, we need to ensure that namescan(), that is, the scan for type QTYPE_A,
				// posts its shutdown event first. To ensure this, a small time delay before posting namescan()'s
				// shutdown event, will guarantee the kosher ordering of the events which avoids the above
				// problem. So that's why the following .2 second delay is here. DO NOT REMOVE IT!
				// For additional protection, I'll also put a .2 second delay before the destruction of the thread
				// and CServiceDiscovery instance, etc, because that final shutdown must be guaranteed to occur 
				// after both of namescan() and addrscan() are well and truly dead.
				wxMilliSleep(200);

				wxPostEvent(gpServiceDiscovery, eventCustom_EndNamescan); // handler in CServiceDiscovery instance
#if defined (_shutdown_)
				wxLogDebug(_T("wxServDisc: %p (637) (QTYPE_SRV) Posting event wxEVT_End_Namescan to CServiceDiscovery"), (void*)this);
#endif
			}
		}
		//if ((parent == NULL) && (this->m_bDestroyChildren == TRUE) &&
		//	(this->querytype == QTYPE_A) && (gpServiceDiscovery != NULL))
		if (gpServiceDiscovery != NULL)
		{
			if ((parent == gpServiceDiscovery->m_pWxSD) && (this->m_bDestroyChildren == TRUE) &&
				(this->querytype == QTYPE_A))
			{

				// I am an addrscan() instance - there is only one of these per service discovery run,
				// and I (because I am a detached thread) am about to disappear. So tell the 
				// CServiceDiscovery instance that I'm departing the scene so it can set its
				// m_bNamescanIsEnded boolean to TRUE
				wxCommandEvent eventCustom_EndAddrscan(wxEVT_End_Addrscan);
				wxPostEvent(gpServiceDiscovery, eventCustom_EndAddrscan); // handler in CServiceDiscovery instance
#if defined (_shutdown_)
				wxLogDebug(_T("wxServDisc: %p (656) (QTYPE_A) Posting event wxEVT_End_Addrscan to CServiceDiscovery"), (void*)this);
#endif
			}
		}

#if defined(_shutdown_)
		wxLogDebug(wxT("wxServDisc %p: (662) scanthread exiting, after loop has ended, now at end of Entry(), returning NULL; gpServiceDiscovery = %lx"), 
					this, gpServiceDiscovery);
#endif
	} // end of TRUE block for test: if (!m_bKillZombie)
	else
	{
#if defined(_shutdown_)
		wxLogDebug(wxT("wxServDisc %p: (669) End of Entry(): NULL passed in for parent. This ZOMBIE is returning NULL"), this);
#endif
		exit = TRUE;
		return NULL;
	}

	return NULL;
}

bool wxServDisc::sendm(struct message* m, SOCKET s, unsigned long int ip, unsigned short int port)
{
	struct sockaddr_in to;

	memset(&to, '\0', sizeof(to));

	to.sin_family = AF_INET;
	to.sin_port = port;
	to.sin_addr.s_addr = ip;

	if (sendto(s, (char*)message_packet(m), message_packet_len(m), 0, (struct sockaddr *)&to, sizeof(struct sockaddr_in)) != message_packet_len(m))
	{
		err.Printf(_("Can't write to socket: %s\n"), strerror(errno));
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
	int ssize = sizeof(struct sockaddr_in);
#else
	socklen_t ssize = sizeof(struct sockaddr_in);
#endif


	if ((bsize = recvfrom(s, (char*)buf, MAX_PACKET_LEN, 0, (struct sockaddr*)&from, &ssize)) > 0)
	{
		memset(m, '\0', sizeof(struct message));
		message_parse(m, buf);
		*ip = (unsigned long int)from.sin_addr.s_addr;
		*port = from.sin_port;
		return bsize;
	}

#ifdef WIN32
	if (bsize < 0 && WSAGetLastError() != WSAEWOULDBLOCK)
#else
	if (bsize < 0 && errno != EAGAIN)
#endif
	{
		err.Printf(_("Can't read from socket %d: %s\n"),
			errno, strerror(errno));
		return bsize;
	}

	return 0;
}

int wxServDisc::ans(mdnsda a, void *arg)
{
	wxServDisc *moi = (wxServDisc*)arg;

	wxString key;
	switch (a->type)
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
	ip.s_addr = ntohl(a->ip);
	result.ip = wxString(inet_ntoa(ip), wxConvUTF8);

	result.port = a->srv.port;

	if (a->ttl == 0)
		// entry was expired
		moi->results.erase(key);
	else
		// entry update
		moi->results[key] = result;

	moi->post_notify();
#if defined(_zero_)

	// These & next block are Beier's loggings
	wxLogDebug(wxT("wxServDisc %p: got answer:"), moi);
	wxLogDebug(wxT("wxServDisc %p:    key:  %s"), moi, key.c_str());
	wxLogDebug(wxT("wxServDisc %p:    ttl:  %i"), moi, (int)a->ttl);
	wxLogDebug(wxT("wxServDisc %p:    time: %lu"), moi, result.time);
#endif
#if defined(_zero_)
	if (a->ttl != 0) {
		wxLogDebug(wxT("wxServDisc %p:    name: %s"), moi, moi->results[key].name.c_str());
		wxLogDebug(wxT("wxServDisc %p:    ip:   %s"), moi, moi->results[key].ip.c_str());
		wxLogDebug(wxT("wxServDisc %p:    port: %u"), moi, moi->results[key].port);
	}
	wxLogDebug(wxT("wxServDisc %p: answer end"), moi);
#endif
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
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
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
	hints.ai_flags = AI_NUMERICHOST;
	if ((status = getaddrinfo(mcAddrStr, NULL, &hints, &multicastAddr)) != 0) {
		err.Printf(_("Could not get multicast address: %s\n"), gai_strerror(status));
		return INVALID_SOCKET;
	}
#if defined(_zero_)
	wxLogDebug(wxT("wxServDisc %p: Using %s"), this, multicastAddr->ai_family == PF_INET6 ? wxT("IPv6") : wxT("IPv4"));
#endif
	/*
	Resolve a local address with the same family (IPv4 or IPv6) as our multicast group
	*/
	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = multicastAddr->ai_family;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE; // no name resolving, wildcard address
	if ((status = getaddrinfo(NULL, mcPortStr, &hints, &localAddr)) != 0) {
		err.Printf(_("Could not get local address: %s\n"), gai_strerror(status));
		freeaddrinfo(multicastAddr);
		return INVALID_SOCKET;
	}

	/*
	Create socket
	*/
	if ((sock = socket(localAddr->ai_family, localAddr->ai_socktype, 0)) == INVALID_SOCKET) {
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
	if (bind(sock, localAddr->ai_addr, localAddr->ai_addrlen) != 0) {
		err.Printf(_("Could not bind socket: %s\n"), strerror(errno));
		goto CompleteCleanUp;
	}

	/*
	Set the multicast TTL.
	*/
	if (setsockopt(sock,
		localAddr->ai_family == PF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP,
		localAddr->ai_family == PF_INET6 ? IPV6_MULTICAST_HOPS : IP_MULTICAST_TTL,
		(char*)&multicastTTL, sizeof(multicastTTL)) != 0) {
		err.Printf(_("Could not set multicast TTL: %s\n"), strerror(errno));
		goto CompleteCleanUp;
	}

	/*
	Join the multicast group. We do this seperately depending on whether we
	are using IPv4 or IPv6.
	*/
	if (multicastAddr->ai_family == PF_INET &&
		multicastAddr->ai_addrlen == sizeof(struct sockaddr_in)) /* IPv4 */
	{
		struct ip_mreq multicastRequest;  // Multicast address join structure

										  /* Specify the multicast group */
		memcpy(&multicastRequest.imr_multiaddr,
			&((struct sockaddr_in*)(multicastAddr->ai_addr))->sin_addr,
			sizeof(multicastRequest.imr_multiaddr));

		/* Accept multicast from any interface */
		multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);

		/* Join the multicast address */
		if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&multicastRequest, sizeof(multicastRequest)) != 0) {
			err.Printf(_("Could not join multicast group: %s\n"), strerror(errno));
			goto CompleteCleanUp;
		}

	}
	else if (multicastAddr->ai_family == PF_INET6 &&
		multicastAddr->ai_addrlen == sizeof(struct sockaddr_in6)) /* IPv6 */
	{
		struct ipv6_mreq multicastRequest;  /* Multicast address join structure */

											/* Specify the multicast group */
		memcpy(&multicastRequest.ipv6mr_multiaddr,
			&((struct sockaddr_in6*)(multicastAddr->ai_addr))->sin6_addr,
			sizeof(multicastRequest.ipv6mr_multiaddr));

		/* Accept multicast from any interface */
		multicastRequest.ipv6mr_interface = 0;

		/* Join the multicast address */
		if (setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char*)&multicastRequest, sizeof(multicastRequest)) != 0) {
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
	unsigned long block = 1;
	ioctlsocket(sock, FIONBIO, &block);
#else
	flag = fcntl(sock, F_GETFL, 0);
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
	m_bDestroyChildren = FALSE; // a caller setting this to TRUE causes break from loop, 
								// so instance will then die
	m_bKillZombie = FALSE; // initialize

#if defined (_shutdown_)
	wxLogDebug(_T("\nwxServDisc CREATOR: (978) I am %p , and parent passed in =  %p"), this, parent);
#endif

	// If p is passed in as NULL, consider it a zombie needing speedy destruction - which
	// means that Entry() will use a value TRUE to skip all code except its shutdown stuff
	if (parent == NULL)
	{
		m_bKillZombie = TRUE;
#if defined(_shutdown_)
		wxLogDebug(wxT("wxServDisc %p: In creator: NULL passed in. This instance is a ZOMBIE"), this);
#endif
	}

	// The global gpServiceDiscovery is the pointer to the parent class instance, CServiceDiscovery
	// Additional instances of wxServDisc get spawned, but not by the CServiceDiscovery parent, so 
	// those will have p = NULL, unless I explicitly provide a p ptr (which I'll do only for the two 
	// child scans done in DiscoverResults() -- and of course those p pointers will be to instances
	// of wxServDisc run from stack frames, for the hostname scan, and the ipaddr scan.
	// Beier's original code passed in p = NULL (0) for the first parameter of the two local 
	// instantiations of wxServDisc, what I call the child ones. An important point from this is
	// the following. His original code, in post_notify() did NOT post a wxServDiscNOTIFY event
	// when either of those two child instances was running, because in post_notify() he had a 
	// test:   if (parent) {  do the posting of the wxServDiscNOTIFY event }. So when parent is NULL
	// the child instances cannot possibly get onSDNotify() handler called. Any changes I make must
	// ensure that onSDNotify() cannot be asked for from a running child instance. If we can, unwanted
	// wxServDisc instances (spawned but needlessly for getting a single discovery done) should have
	// their internal code in Entry() skipped, and just proceed to thread death.
	// save query & type
	query = what;
	querytype = type;
#if defined(_zero_)
	wxLogDebug(wxT("\nwxServDisc %p: about to query '%s'"), this, query.c_str());
#endif
	if ((mSock = msock()) == INVALID_SOCKET) {
		wxLogDebug(wxT("Ouch, error creating socket: ") + err);
		return;
	}

#if wxVERSION_NUMBER >= 2905 // 2.9.4 still has a bug here: http://trac.wxwidgets.org/ticket/14626
	if (CreateThread(wxTHREAD_DETACHED) != wxTHREAD_NO_ERROR)
#else
	if (Create() != wxTHREAD_NO_ERROR)
#endif
		err.Printf(_("Could not create scan thread!"));
	else
		if (GetThread()->Run() != wxTHREAD_NO_ERROR)
			err.Printf(_("Could not start scan thread!"));
}

wxServDisc::~wxServDisc()
{
#if defined(_shutdown_)
	wxLogDebug(wxT("~wxServDisc %p: (1031) In destructor: before delete of the wxServDisc instance"), this);
#endif
	processID = wxGetProcessId();

	if (m_bKillZombie)
	{
		if (GetThread() && GetThread()->IsRunning())
		{
			GetThread()->Delete(); // blocks, this makes TestDestroy() return true and cleans up the thread
		}
#if defined (_shutdown_)
		wxLogDebug(wxT("~wxServDisc %p: (1042) Finished ZOMBIE's destructor.  processID = %lx  decimal = %ld"),
			this, processID, processID);
#endif
	}
	else
	{
		if (GetThread() && GetThread()->IsRunning())
		{
#if defined(_shutdown_)

			wxLogDebug(wxT("~wxServDisc %p: (1052) scanthread deleted, wxServDisc destroyed, query was '%s', lifetime was %ld"), 
						this, query.c_str(), mWallClock.Time());
#endif
#if defined (_shutdown_)
			wxLogDebug(wxT("~wxServDisc %p: (1056) Finished destructor ~wxServDisc().  processID = %lx  decimal = %ld  querytype = %d"),
				this, processID, processID, querytype);
#endif
			GetThread()->Delete(); // blocks, this makes TestDestroy() return true and cleans up the thread
		}
	}
}

// BEW added 6Apr16 in the hope that this will clear up some leaks - it does,
// the ones from data coming into onSDNotify from the ans() function, but the wxSDMap
// hashtable itself persists. The latter is destroyed when its wxServDisc is destroyed
void wxServDisc::clearResults()
{
	wxSDMap::const_iterator it;
	for (it = results.begin(); it != results.end(); it++)
	{
		wxSDEntry anEntry = it->second;
		anEntry.ip.clear();
		anEntry.name.clear();
	}
	results.clear();
}

std::vector<wxSDEntry> wxServDisc::getResults() const
{
	std::vector<wxSDEntry> resvec;

	wxSDMap::const_iterator it;
	for (it = results.begin(); it != results.end(); it++)
		resvec.push_back(it->second);

	return resvec;
}

size_t wxServDisc::getResultCount() const
{
	return results.size();
}

void wxServDisc::post_notify()
{
	gpServiceDiscovery->m_postNotifyCount++; // BEW added
	if (gpServiceDiscovery->m_postNotifyCount > 1)
		return; // BEW 21Apr16, exit if the count goes over 1

#if defined(_zero_) && defined(_DEBUG)
	wxLogDebug(_T("wxServDisc:  %p  (1102) post_notify() Entered:  parent = %p , gpServiceDiscovery->m_postNotifyCount = %d"), 
		this, (void*)parent, gpServiceDiscovery->m_postNotifyCount); // BEW added
#endif
	// Beier's code follows, but tests added by BEW in order to do minimal processing etc
	// BEW changed 21Apr16, Beier's test was: if (parent) because all but the owned wxServDisc instance
	// were created with first param of the signature, void* p, being null
	// But now I'm passing in non-null for 3 instances, the original owned instance, and the two
	// children namescan() and addrscan() which are created in CServiceDiscovery::onSDNotify()
	// So any wxServDisc(void*, wxString, int type) with 0 for the void* param, is just a nuisance
	// and if we let it live after the other stuff is finished and deleted, we will get an access
	// violation (it manifests at line 170 of thread.cpp) when the zombie tries to do something but
	// its code resources are gone. So we preserve Beier's protocol: only the owned wxServDisc instance
	// is allowed to post the wxServDiscNOTIFY event.   
	if ((void*)parent == (void*)gpServiceDiscovery) // was:  if (parent)
	{
		wxCommandEvent event(wxServDiscNOTIFY, wxID_ANY);
		event.SetEventObject(this); // set sender

		// Send it
#if defined(_zero_)
		wxLogDebug(_T("wxServDisc:  %p (1122) post_notify():  posting the wxServDiscNOTIFY event, once only"), this);
#endif
		#if wxVERSION_NUMBER < 2900
		wxPostEvent((CServiceDiscovery*)parent, event);
		#else
		wxQueueEvent((CServiceDiscovery*)parent, event.Clone());
		#endif		
	}
}

#endif // _KBSERVER // whm 2Dec2015 added otherwise Linux build breaks when _KBSERVER is not defined

