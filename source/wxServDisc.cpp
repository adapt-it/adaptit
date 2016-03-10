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
// can delete the CServiceDiscovery instance- eventually, in OnIdle() - I've documented
// why there in the code for CServiceDiscovery. Since we use a mutex and condition in our
// solution, the Signal() call which ends the main thread's sleep must be given before the
// cleanup event handling does its work, and *after* any results have been stored in the 
// app's m_servDiscResults wxArrayString

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

#include "wxServDisc.h"
#include "ServiceDiscovery.h"

extern CServiceDiscovery* gpServiceDiscovery; // wxServDisc's creator needs the value we place here


// Compatability defines
#ifdef __APPLE__
#ifndef IPV6_ADD_MEMBERSHIP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif
#endif

// define our new notify event! (BEW added the ...HALTING one)

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

wxServDisc::wxServDisc(void* p, const wxString& what, int type)
{
  // save our caller
  parent = p;
  m_pSD = NULL; // initialize, the pointer to the parent class instance, CServiceDiscovery
  // Additional instances get spawned, but not by the parent, so those will have p = NULL.
  // So we will have the parent CServiceDiscovery instance set a global pointer to itself,
  // gpServiceDiscovery, and then we'll test here for p=NULL, and set it from that
  if (p == NULL)
  {
	  parent = (void*)gpServiceDiscovery;
  }
  wxLogDebug(_T("wxServDisc CREATOR: I am %p , and (CServiceDiscovery*)parent = %p  <<--- my parent"), this, (CServiceDiscovery*)parent);

  m_pSD = (CServiceDiscovery*)parent;

  m_bGetResultsStarted = FALSE; // Only DiscoverResults() sets it to TRUE

  // save query
  query = what;
  querytype = type;

  wxLogDebug(wxT(""));
  wxLogDebug(wxT("wxServDisc %p: about to query '%s' with querytype %d"), this, query.c_str(), querytype);

  // scratch variables...
  m_hostname = _T("");
  m_addr = _T("");
  m_port = _T("");

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

// BEW 25Feb16 created so that hostname lookup and ipaddress lookup can take place
// in wxServDisc and do mutex protected reporting to the arrays in the parent
// CServiceDiscovery instance - which in turn will report them to m_servDiscResults
// array on the app instance
void wxServDisc::DiscoverResults(CServiceDiscovery* pReportTo)
{
	/* no access to CAdapt_ItApp from here now
	if (gpApp->m_pServDisc == NULL)
	{
		// The parent class no longer exists, so just exit now
		wxLogDebug(_T("ServiceDiscovery::GetResults() if (m_pApp->m_pServDisc == NULL) test:  was TRUE, return immediately"));
		return;
	}
	*/
	/* don't set any limit on number of calls of this function
	// Allow only the first call of this
	m_postNotifyCount++;  // set to 0 in the CServicDiscovery constructor
						  
						  // BEW 24Feb16 at this point in time I'll not try to limit it (to say, 10 post_notify() calls,
						  // but if issues arise, uncomment out this and put whatever limit is wanted - remember, that
						  // some calls may be refinding one already found, so don't assume each discovery is to a
						  // different KBserver
						  if (m_postNotifyCount > 10)
						  {
						  return;
						  }
	*/
	if (m_pSD != NULL)
	{
		m_hostname.Empty();
		m_addr.Empty();
		m_port.Empty();
		size_t entry_count = 0;
		int timeout;
		// How will the rest of Adapt It find out if there was no _kbserver service
		// discovered? That is, nobody has got one running on the LAN yet. Answer:
		// m_pFrame->m_urlsArr will be empty, and m_pFrame->m_bArr_ScanFoundNoKBserver will
		// also be empty. The wxServDisc module doesn't call post_notify() if no service is
		// discovered, so we can't rely on this handler being called if what we want to
		// know is that no KBserver is currently running. Each explicit attempt to run this
		// service discovery module will start by clearing the results arrays on the
		// CFrameInstance.

		// length of query plus leading dot
		size_t qlen = getQuery().Len() + 1;
#if defined(_DEBUG)
		wxString theQuery = getQuery(); // temp, to see what's in it
		wxLogDebug(_T("wxServDisc %p:  BEW theQuery:  %s  Its Length plus 1: %d"), this, theQuery.c_str(), (int)qlen);
#endif

		vector<wxSDEntry> entries = getResults();
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
			wxLogDebug(_T("wxServDisc %p: (BEW)  name: %s  Len(): %d  ip: %s  port: %d  time: %d"),
				this, aName.c_str(), nameLen, ip.c_str(), port, time);
#endif
			// what's really wanted - just the bit before the first ._ sequence
			entry_count++;
#if defined(_DEBUG)
			wxString astring = it->name.Mid(0, it->name.Len() - qlen);
			wxLogDebug(_T("wxServDisc %p:  m_sd_servicenames receives servicename:  %s   for entry index = %d"),
				this, astring.c_str(), entry_count - 1);
#endif
			kbsvr_arrays.Lock();
			m_pSD->m_sd_servicenames.Add(it->name.Mid(0, it->name.Len() - qlen));

			m_pSD->m_bArr_ScanFoundNoKBserver.Add(0); // add FALSE, as we successfully
					// discovered one (but that does not necessarily mean we will
					// subsequently succeed at looking up hostname, port, and ip address)
			kbsvr_arrays.Unlock();

		} // end of loop: for (it = entries.begin(); it != entries.end(); it++)

		// Next, lookup hostname and port. Code for this is copied and tweaked,
		// from the list_select() event handler of MyFrameMain. Since we have
		// this now in a loop in case more than one KBserver is running, we
		// must then embed the associated addrscan() call within the loop
		size_t i;
		bool bThrowAwayDuplicateIPaddr = FALSE;

#if defined(_DEBUG)
		wxLogDebug(_T("wxServDisc %p:  OUTER LOOP commencing; it will iterate entry_count ( %d ) times"),
			this, entry_count);
#endif
		for (i = 0; i < entry_count; i++)
		{
			wxServDisc namescan(0, getResults().at(i).name, QTYPE_SRV); // remember, this looks
					// like a local function, but actually it runs as a detached thread, so 
					// control will immediately move on - so the timeout loop below is mandatory
			bThrowAwayDuplicateIPaddr = FALSE; // initialize for every iteration

			//timeout = 3700; // BEW allow a little more time to get a result
			timeout = 3000; // go back to original Beier value
			// The timeout loop exits as soon as a result is found, or when it times out
			while (!namescan.getResultCount() && timeout > 0)
			{
				wxMilliSleep(25);
				timeout -= 25;
			}
			if (timeout <= 0)
			{
				//wxLogError(_T("wxServDisc %p:  Timeout looking up hostname. Entry index: %d"), this, i); <<-- this one is a nuisance
				m_hostname = m_addr = m_port = wxEmptyString;
				kbsvr_arrays.Lock();
				m_pSD->m_bArr_HostnameLookupFailed.Add(1); // adding TRUE
				m_pSD->m_bArr_IPaddrLookupFailed.Add(-1); // undefined, addrscan() for this index is not tried
				m_pSD->m_bArr_DuplicateIPaddr.Add(-1); // undefined, whether a duplicate ipaddr is untried
				kbsvr_arrays.Unlock();
#if defined(_DEBUG)
				kbsvr_arrays.Lock();
				wxLogDebug(_T("wxServDisc %p:  namescan() Timed out: [Service: %s ]  m_hostname:  %s   m_port  %s   for entry index = %d"),
					this, m_pSD->m_sd_servicenames.Item(i).c_str(), m_hostname.c_str(), m_port.c_str(), i);
				kbsvr_arrays.Unlock();
#endif
				continue; // don't return, we need to try every iteration
			}
			else
			{
				// The namescan found something... 
				m_hostname = namescan.getResults().at(0).name;
				m_port = wxString() << namescan.getResults().at(0).port;
				kbsvr_arrays.Lock();
				m_pSD->m_bArr_HostnameLookupFailed.Add(0); // adding FALSE, the lookup succeeded
				kbsvr_arrays.Unlock();
#if defined(_DEBUG)
				kbsvr_arrays.Lock();
				wxLogDebug(_T("wxServDisc %p:  Found: [Service: %s ] Looked up:  m_hostname:  %s   m_port  %s   for entry index = %d"),
					this, m_pSD->m_sd_servicenames.Item(i).c_str(), m_hostname.c_str(), m_port.c_str(), i);
				kbsvr_arrays.Unlock();

#endif
				// For each successful namescan(), we must do an addrscan, so as to fill
				// out the full info needed for constructing a URL; if the namescan was
				// not successful, the m_bArr_IPaddrLookupFailed entry for this index
				// should be neither true (1) nor false (0), so use -1 for "no test was made"
				{
					wxServDisc addrscan(0, m_hostname, QTYPE_A);  // remember, this looks
					// like a local function, but actually it runs as a detached thread, so 
					// control will immediately move on - so the timeout loop below is mandatory

					//timeout = 3700; // BEW allow a little more time for getting a result
					timeout = 3000; // Beier's original value
					// The timeout loop exits when a result is gotten, or timeout trips
					while (!addrscan.getResultCount() && timeout > 0)
					{
						wxMilliSleep(25);
						timeout -= 25;
					}
					if (timeout <= 0)
					{
						wxLogError(_T("Timeout looking up IP address."));
						m_hostname = wxEmptyString;
						m_addr = wxEmptyString;
						m_port = wxEmptyString;
						kbsvr_arrays.Lock();
						m_pSD->m_bArr_IPaddrLookupFailed.Add(1); // for TRUE, unsuccessful lookup
						m_pSD->m_bArr_DuplicateIPaddr.Add(-1); // undefined, whether a duplicate ipaddr is untried
						kbsvr_arrays.Unlock();
#if defined(_DEBUG)
						kbsvr_arrays.Lock();
						wxLogDebug(_T("wxServDisc %p: ip Not Found: [Service: %s ] Timed out:  ip addr:  %s  for entry index = %d"),
							this, m_pSD->m_sd_servicenames.Item(i).c_str(), m_addr.c_str(), i);
						kbsvr_arrays.Unlock();

#endif
						continue; // do all iterations
					}
					else
					{
						// succeeded in getting the service's ip address
						m_addr = addrscan.getResults().at(0).ip;

						// Check for a unique ip address, if not unique, abandon this
						// iteration (do a case sensitive compare)
						kbsvr_arrays.Lock();
						bThrowAwayDuplicateIPaddr = IsDuplicateStrCase(&m_pSD->m_uniqueIpAddresses, m_addr, TRUE);
						kbsvr_arrays.Unlock();
						if (!bThrowAwayDuplicateIPaddr)
						{
#if defined(_DEBUG)
							kbsvr_arrays.Lock();
							wxLogDebug(_T("wxServDisc %p: if (!bThrowAwayDuplicateIPaddr) test is TRUE: ip addr:  %s   name:  %s   So storing 0 in DuplicateIpadd array"),
								this, m_addr.c_str(), m_pSD->m_sd_servicenames.Item(i).c_str());
							kbsvr_arrays.Unlock();
#endif
							// Not a duplicate, so don't throw it away, store it
							kbsvr_arrays.Lock();
							AddUniqueStrCase(&m_pSD->m_uniqueIpAddresses, m_addr, TRUE);
							m_pSD->m_bArr_IPaddrLookupFailed.Add(0); // for FALSE, a successful ip lookup
							m_pSD->m_bArr_DuplicateIPaddr.Add(0); // it's not a duplicate
							kbsvr_arrays.Unlock();
#if defined(_DEBUG)
							//kbsvr_arrays.Lock();
							//wxLogDebug(_T("wxServDisc %p: Found: [Service:  %s  ] Looked up:  ip addr:  %s   for entry index = %d"),
							//	this, m_pSD->m_sd_servicenames.Item(i).c_str(), m_addr.c_str(), i);
							//kbsvr_arrays.Unlock();
#endif
						}
						else
						{
#if defined(_DEBUG)
							kbsvr_arrays.Lock();
							wxLogDebug(_T("wxServDisc %p: if (!bThrowAwayDuplicateIPaddr) test is FALSE **DUPLICATE**: ip addr:  %s   name:  %s   So storing 1 in DuplicateIpadd array"),
								this, m_addr.c_str(), m_pSD->m_sd_servicenames.Item(i).c_str());
							kbsvr_arrays.Unlock();
#endif

							// It's a duplicate ip address
							kbsvr_arrays.Lock();
							m_pSD->m_bArr_IPaddrLookupFailed.Add(0); // for FALSE, a successful ip lookup
							m_pSD->m_bArr_DuplicateIPaddr.Add(1); // it's a duplicate
									//continue;  <<- no continue here, we'll allow the url to be
									//constructed below; the m_bArr_DuplicateIPaddr array entry
									//being 1 will allow us to ignore it later on
							kbsvr_arrays.Unlock();
						}
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
			// Note: this DiscoverResults() function will not have been called if no service
			// was discovered, so we don't need to test m_bArr_ScanFoundNoKBserver, as it will be 0
			kbsvr_arrays.Lock();
			if (m_pSD->m_bArr_HostnameLookupFailed.Item(i) == 0 &&
				m_pSD->m_bArr_IPaddrLookupFailed.Item(i) == 0)
			{
				// Both the first, and any duplicate ipaddr are added to the m_urlsArr
				// here, but duplicates are marked by the flag for a duplicate being 1
				m_pSD->m_urlsArr.Add(protocol + m_addr);
#if defined(_DEBUG)
				wxLogDebug(_T("wxServDisc %p:  Found: [Service: %s ] Constructed URL:  %s  for entry index = %d"),
					this, m_pSD->m_sd_servicenames.Item(i).c_str(), (protocol + m_addr).c_str(), i);
#endif
			}
			else
			{
				wxString emptyStr = _T("");
				m_pSD->m_urlsArr.Add(emptyStr);
			}
			kbsvr_arrays.Unlock();

		} // end of loop: for (i = 0; i < entry_count; i++)

		  // Make the results accessible: store them as 1 or more strings in the array
		  // m_localDiscResultsArr in the CServiceDiscovery instance
		  // Generate the one (usually only one) or more lines, each corresponding to
		  // a discovery of a multicasting KBserver instance (not all lookups might
		  // have been error free, so some urls may be absent, and such lines may just
		  // end up containing error data; & ipaddr duplicates are included here too)
		wxString colon = _T(":");
		wxString intStr;
		if (m_pSD->m_urlsArr.IsEmpty())
		{
			return;
		}
		for (i = 0; i < (size_t)entry_count; i++)
		{
			kbsvr_arrays.Lock();
			wxString aLine = m_pSD->m_urlsArr.Item((size_t)i); // either a URL, or an empty string
			aLine += colon;
			wxItoa(m_pSD->m_bArr_ScanFoundNoKBserver.Item((size_t)i), intStr);
			aLine += intStr + colon;
			wxItoa(m_pSD->m_bArr_HostnameLookupFailed.Item((size_t)i), intStr);
			aLine += intStr + colon;
			wxItoa(m_pSD->m_bArr_IPaddrLookupFailed.Item((size_t)i), intStr);
			aLine += intStr + colon;
			wxItoa(m_pSD->m_bArr_DuplicateIPaddr.Item((size_t)i), intStr);
			aLine += intStr;
			m_pSD->m_localDiscResultsArr.Add(aLine); // BEW 25Feb16 changed so that they are stored in CServiceDiscovery
											  // instance which will later send them to app's m_servDiscResults array
			kbsvr_arrays.Unlock();
#if defined(_DEBUG)
			wxLogDebug(_T("wxServDisc %p:  Storing URL %s in CServiceDiscovery::m_localDiscResultsArr"), this, aLine.c_str());
#endif
		}
#if defined(_DEBUG)
		wxLogDebug(_T("wxServDisc %p:  Finished storing constructed URLs in CServiceDiscovery's m_localDiscResultsArr  ******"), this);
#endif

	} // end of TRUE block for test: if (m_pSD != NULL)
	else
	{
		// major error, but the program counter has never entered here, so
		// just a message for developer will do
		//gpApp->LogUserAction(_T("GetResults():unexpected error: ptr to wxServDisc instance, m_pWxSD, is NULL"));
		wxLogDebug(_T("wsServDisc::DiscoverResults() (m_pSD != NULL) test:  was FALSE, m_pSD =  %p"), m_pSD);
		return;
	}
	/* just let the timeout in main thread kill the discovery process
	// App's DoServiceDiscovery() function can exit from WaitTimeout(), awakening main
	// thread and it's event handling etc, so do Signal() to make this happen
	wxLogDebug(_T("wxServDisc %p:  At end of CServiceDiscovery::GetResults() m_pMutex  =  %p"),
		this, m_pMutex);

	wxMutexLocker locker(*m_pMutex); // make sure it is locked
	bool bIsOK = locker.IsOk(); // check the locker object successfully acquired the lock
	wxUnusedVar(bIsOK);

	// BEW 23Feb16 commenting it out, so that more than one KBserver can be handled
	// and the main thread will rely on WaitTimeout() to timeout instead, to awaken
		
	wxCondError condError = m_pCondition->Signal(); // tell main thread to awaken
	// The WaitTimeout(value) will pass in the value (default 8000) from the
	// basic config file, but the user can manually put a different value there,
	// but for safety's sake a minimum of 8 secs  is best - even though most
	// successful discovery runs will succeed within 4 seconds, usually within 3;
	// but the timeout being 8 or more does not mean it waits for the timeout
	// to happen, instead, Signal() determines when the awakening happens and
	// the array access then happens, and typically that's after 2 to 3 seconds
	// of run time
	wxUnusedVar(condError); // it's used in the debug build, for logging
	#if defined(_DEBUG)
	wxString cond0 = _T("wxCOND_NO_ERROR");
	wxString cond1 = _T("wxCOND_INVALID");
	wxString cond2 = _T("wxCOND_TIMEOUT");
	wxString cond3 = _T("wxCOND_MISC_ERROR");
	wxString myError = _T("");
	if (condError == wxCOND_NO_ERROR)
	{
	myError = cond0;
	}
	else if (condError == wxCOND_INVALID)
	{
	myError = cond1;
	}
	else if (condError == wxCOND_TIMEOUT)
	{
	myError = cond2;
	}
	else if (condError == wxCOND_INVALID)
	{
	myError = cond3;
	}
	wxLogDebug(_T("wxServDisc %p: ServiceDiscovery::GetResults() error condition for Signal() call: %s   locker.IsOk() returns %s"),
	m_pWxSD, myError.c_str(), bIsOK ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str());
	#endif
	*/
	wxLogDebug(_T("wxServDisc %p: BEW: DiscoverResults() finished. pReportTo is %p"), this, pReportTo);

/* in the new design, we don't want multiple processes each posting a wxServDiscHALTING event, let time-out's do their thing	
	//  Post a custom wxServDiscHALTING event here, to get rid of my parent classes
	{
		wxCommandEvent event(wxServDiscHALTING, wxID_ANY);
		event.SetEventObject(this); // set sender

									// BEW added this posting...  Send it
#if wxVERSION_NUMBER < 2900
		wxPostEvent(this, event);
#else
		wxQueueEvent(this, event.Clone());
#endif
		wxLogDebug(_T("wxServDisc %p: BEW: GetResults(), block finished. Now have posted event wxServDiscHALTING."), m_pWxSD);
	}

	wxLogDebug(wxT("wxServDisc %p: BEW: A KBserver was found. GetResults() is exiting, wxServDiscHALTING has been posted"), m_pWxSD);
*/

	// OI!!! Where are the cleanup functions????

}

void wxServDisc::post_notify()
{	

	// BEW Tell the running wxServDisc thread that GetResults() was invoked
	// BEW - the nonEvent approach follows...
	if (parent)
	{
		/*
		((CServiceDiscovery*)parent)->m_pWxSD = this;

		m_bGetResultsStarted = TRUE;
		wxLogDebug(_T("In post_notify(): Resetting (CServiceDiscovery*)parent->m_pWxSD to this wxServDisc %p:   (CServiceDiscovery*)parent = %p"), this, (CServiceDiscovery*)parent);

		//wxCommandEvent dummy;
		//((CServiceDiscovery*)parent)->onSDNotify(dummy);
			
		((CServiceDiscovery*)parent)->GetResults();
		*/
		m_bGetResultsStarted = TRUE;
		wxLogDebug(_T("In post_notify(): for wxServDisc instance  %p   Now calling DiscoverResults(m_pSD) with m_pSD = %p"), this, m_pSD);
		DiscoverResults(m_pSD);
	}

  // Beier's code follows - we don't need to do it this way
  /*
  if(parent)
    {
 	  wxLogDebug(_T("wxServDisc %p:  post_notify():  posting event"), this); // BEW added this call
		
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

wxServDisc::~wxServDisc()
{
  wxLogDebug(wxT("wxServDisc %p: before scanthread delete"), this);
  if(GetThread() && GetThread()->IsRunning())
    GetThread()->Delete(); // blocks, this makes TestDestroy() return true and cleans up the thread

  wxLogDebug(wxT("In ~wxServDisc() wxServDisc %p: scanthread deleted, wxServDisc destroyed, hostname was '%s', lifetime was %ld"),
	  this, query.c_str(), mWallClock.Time());
  wxLogDebug(wxT("wxServDisc %p:  End of ~wxServDisc() Finished call of ~wxServDisc()"), this);
}

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
		  // 4 secs was fairly consistently not enough ... go to larger comment
		  // I'm increasing it to 10 seconds, that keeps this loop running that long - we
		  // need it to run long enough so that main thread's CServiceDiscovery::GetResults()
		  // can get through its lookups and get the URL calculated & stored (it's taking
		  // about 8 seconds using XP laptop with KBserver on Ubuntu laptop) before the
		  // Entry() loop times out - because when it times out, post_notify() has no way
		  // to then get called, and the HALTING events happen and so failure is inevitable.
		  // I tried 7 secs, after 4 kept often failing, and 7 was 'just enough' I think.
		  // So I'll try 10. Also, I should probably make GetResults() go to sleep while
		  // it's lookups are happening in wxServDisc, and use a condition with Signal()
		  // to awaken as each result comes back; otherwise a delay loop hogs the processor
		  // so if probably is better to use the mutext & Signal() approach to give
		  // the thread a bit more access to the CPU (at least that's what I'm thinking).
		  // Nah, 10 sec makes no diff, go back to 7
	  {
		  int nah = 1; wxUnusedVar(nah); // useful to determining when we break via here, values > 4 make no diff
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
	wxLogDebug(_T("wxServDisc %p:   BEW  end of outer loop iteration:  %d"), this, BEWcount);
    } // end of outer loop

  // BEW 2Dec15 added cache freeing (d's shutdown is not yet 1, but it doesn't test for it
  // so do this first, as shutdown will be set to 1 in mdnsd_shutdown(d) immediately after
  my_gc(d); // is based on Beier's _gd(d), but removing every instance of the
            // cached struct regardless in the cache array (it's a sparse array
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
  wxLogDebug(_T("wxServDisc %p: wxServDisc::Entry(): m_bGetResultsStarted is %s "), this,
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
	  // WaitTimeout() to awaken the main thread, when no KBserver can be found because
	  // one is not yet running on the LAN
	  /* try not posting a halting event, just let it die
	  wxCommandEvent upevent(wxServDiscHALTING, wxID_ANY);
	  upevent.SetEventObject(this); // set sender

	  if ((CServiceDiscovery*)parent != NULL)
	  {
#if wxVERSION_NUMBER < 2900
		wxPostEvent((CServiceDiscovery*)parent, upevent);
#else
		wxQueueEvent((CServiceDiscovery*)parent, upevent.Clone());
#endif
		wxLogDebug(_T("wxServDisc %p: after timeout of Entry()'s loop, so posting wxServDiscHALTING event"),
					this);
	  }
	  */
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
  {
    // entry was expired
    moi->results.erase(key);
  }
  else
  {
    // entry update
    moi->results[key] = result;
  }

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


// BEW created 5Jan16, needed for GetResults() in CServiceDiscovery instance
bool wxServDisc::IsDuplicateStrCase(wxArrayString* pArrayStr, wxString& str, bool bCase)
{
	int count = pArrayStr->GetCount();
	if (count == 0)
	{
		return FALSE;
	}
	else
	{
		int index;
		if (!bCase)
		{
			// case insensitive compare
			index = pArrayStr->Index(str, FALSE); // bCase is FALSE, so A and a
												  // are the same character (wxWidgets comparison used)
		}
		else
		{
			//case sensitive (ie. case differentiates)
			index = pArrayStr->Index(str); // bCase is default TRUE, so A and a
										   // are different characters (wxWidgets comparison used)
		}
		if (index == wxNOT_FOUND)
		{
			// it's not in there yet, so add it
			return FALSE;
		}
		else
		{
			// it's in the array already, so ignore it
			return TRUE;
		}
	}
}

// BEW created 5Jan16
// This is similar to AddUniqueString() above, but AddUniqueString() does
// case or caseless compare using the global gbAutoCaps; but for
// AddUniqueStrCase() I want to control whether the comparison is cased or
// caseless from the signature. What prompted me to make this version is
// for comparison of ipaddr strings in the KBserver's service discovery
// module
void wxServDisc::AddUniqueStrCase(wxArrayString* pArrayStr, wxString& str, bool bCase)
{
	int count = pArrayStr->GetCount();
	if (count == 0)
	{
		pArrayStr->Add(str);
	}
	else
	{
		int index;
		if (!bCase)
		{
			// case insensitive compare
			index = pArrayStr->Index(str, FALSE); // bCase is FALSE, so A and a
												  // are the same character (wxWidgets comparison used)
		}
		else
		{
			//case sensitive (ie. case differentiates)
			index = pArrayStr->Index(str); // bCase is default TRUE, so A and a
										   // are different characters (wxWidgets comparison used)
		}
		if (index == wxNOT_FOUND)
		{
			// it's not in there yet, so add it
			pArrayStr->Add(str);
		}
		else
		{
			// it's in the array already, so ignore it
			return;
		}
	}
}

void wxServDisc::wxItoa(int val, wxString& str)
{
	wxString valStr;
	valStr << val;
	str = valStr;
}


#endif // _KBSERVER // whm 2Dec2015 added otherwise Linux build breaks when _KBSERVER is not defined

