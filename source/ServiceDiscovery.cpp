
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
/// ======================================================================
/// *********** IMPORTANT ****************** for future maintainers **************
/// It would be possible to remove this class from the service discovery module
/// except for one thing. #include "Adapt_It.h" would need to be included in
/// wxServDisc.cpp if the results are to be reported to a wxArrayString member
/// of the CAdapt_ItApp class instance. Doing that creates a multitude of name
/// conflicts with winsock.h and other Microsoft classes. To avoid this, the
/// CServiceDiscovery serves to isolate the namespace for the service discovery
/// code from clashes with the GUI support's namespace. So it must be retained.
/// 
/// Also, DO NOT return GetResults() to be an event handler (formerly it was
/// onSDNotify() for a wxServDiscNOTIFY event posted within Post_Notify())
/// because our mutex & condition solution uses .WaitTimeout() in he app's
/// DoServiceDiscovery() function, and when waiting, main thread event handling
/// is asleep - so if you tried to do things Beier's way, the essential
/// address lookup etc would not get called until the main thread's sleep ended -
/// which would be too late because DoServiceDiscovery() would have been exited
/// before the service discovery results could be computed.
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ServiceDiscovery.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if defined(_KBSERVER)

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
// Include your minimal set of headers here
#include <wx/arrstr.h>
#include "wx/log.h"
#include <wx/textfile.h>
#include <wx/thread.h>
#include <wx/msgdlg.h>

#endif

#include <vector>
#define _WINSOCKAPI_ // keeps winsock.h from being included in <Windows.h>, it's here just in case
#define WIN32_LEAN_AND_MEAN // does the same job as above, likewise here just in case
#include "helpers.h"
#include "wxServDisc.h"
#include "ServiceDiscovery.h"

#ifdef __WXMSW__
// The following include prevents the #include of Adapt_It.h from generating Yield() macro
// conflicts, because somewhere I've got windows.h already defined and the latter also has
// its own Yield() macro. Vadim Zeitlin said to add the following include "after including
// windows.h" and since that would have been in wxServDisc.h, it needs to come before
// the include of Adapt_It.h which internally has the wx version. Did so, and the compile
// errors disappeared from here
#include <wx/msw/winundef.h>
#endif
#include "Adapt_It.h"

extern CAdapt_ItApp* gpApp;

BEGIN_EVENT_TABLE(CServiceDiscovery, wxEvtHandler)
	EVT_COMMAND (wxID_ANY, wxServDiscHALTING, CServiceDiscovery::onSDHalting)
END_EVENT_TABLE()

CServiceDiscovery::CServiceDiscovery()
{
	m_servicestring = _T("");
	m_bWxServDiscIsRunning = TRUE;
	wxUnusedVar(m_servicestring);
}

CServiceDiscovery::CServiceDiscovery(wxMutex* mutex, wxCondition* condition,
							wxString servicestring, CAdapt_ItApp* pParentClass)
{
	wxLogDebug(_T("\nInstantiating a CServiceDiscovery class, passing in servicestring: %s, ptr to instance: %p"),
		servicestring.c_str(), this);

	m_pMutex = mutex;
	m_pCondition = condition;

	m_servicestring = servicestring; // service to be scanned for
	m_pParent = pParentClass; // so we can delete this class from a handler in the parent class
	m_bWxServDiscIsRunning = TRUE; // Gets set FALSE only in my added onServDiscHalting() handler
		// and the OnIdle() hander will use the FALSE to get CServiceDiscovery and ServDisc
		// class instances deleted, and app's m_pServDisc pointer reset to NULL afterwards
	m_postNotifyCount = 0; // use this int as a filter to allow only one GetResults() call
	m_pParent->m_bCServiceDiscoveryCanBeDeleted = FALSE; // initialize, it gets set TRUE in the
				// handler of the wxServDiscHALTING custom event, and then OnIdle()
				// will attempt the CServiceDiscovery* m_pServDisc deletion (if done
				// there, deletions are reordered so that the parent is deleted last -
				// which avoids a crash) Reinitialize it there afterwards, to NULL

	// scratch variables...
	m_hostname = _T("");
	m_addr = _T("");
	m_port = _T("");

	// Initialize my reporting context (Use .Clear() rather than
	// .Empty() because from one run to another we don't know if the
	// number of items discovered will be the same as discovered previously
	// Note: if no KBserver service is discovered, these will remain cleared.
	m_sd_servicenames.Clear();
	m_uniqueIpAddresses.Clear();
	m_urlsArr.Clear();
	m_sd_lines.Clear(); //for finished  string:flag:flag:flag lines, string can be empty
	m_bArr_ScanFoundNoKBserver.Clear();  // stores 0, 1 or -1 per item
	m_bArr_HostnameLookupFailed.Clear(); // ditto
	m_bArr_IPaddrLookupFailed.Clear();   // ditto
	m_bArr_DuplicateIPaddr.Clear();      // ditto

    // wxServDisc creator is: wxServDisc::wxServDisc(void* p, const wxString& what, int
    // type) where p is pointer to the parent class & what is the service to scan for, its
    // type is QTYPE_PTR
	m_pSD = new wxServDisc(this, m_servicestring, QTYPE_PTR);
	wxUnusedVar(m_pSD);
	wxLogDebug(_T("wxServDisc %p: just now instantiated in CServiceDiscovery creator"), m_pSD);
}

// This function in Beier's solution was an even handler called onSDNotify(), but in my
// solution it is called directly and I've renamed it GetResults(). When wxServDisc
// discovers a service (one or more of them) that it is scanning for, it reports its
// success here. Adapt It needs just the ip address(s), but not the port. We present URL(s)
// to the AI code which it can use for automatic connection to the running KBserver.
// Ideally, only one KBserver runs at any given time, but we have to allow for users being
// perverse and running more than one, or for a workshop in which different groups each
// have a running KBserver on the LAN. If they do, we'll have to let them make a choice of
// which URL to connect to. GetResults is an amalgamation of a few separate functions from
// the original sdwrap code. We don't have a gui, so all we need do is the minimal task of
// looking up the ip address and port of each discovered _kbserver service, and turning
// each such into an ip address from which we can construct the appropriate URL (one or
// more) which we'll make available to the rest of Adapt It via a wxArrayString plus some
// booleans for error states, on the CAdapt_ItApp instance. The logic for using the one or
// more URLs will be constructed externally to this service
// discovery module - see DoServiceDiscovery().
// This service discovery module will just be instantiated, scan for _kbserver._tcp.local.
// lookup the ip addresses, deposit finished URL or URLs in the wxArrayString in the
// CAdapt_ItApp::m_servDiscResults array, and then kill itself.
void CServiceDiscovery::GetResults()
{
	if (gpApp->m_pServDisc == NULL)
	{
		// The parent class no longer exists, so just exit now
		wxLogDebug(_T("ServiceDiscovery::GetResults() if (m_pApp->m_pServDisc == NULL) test:  was TRUE, return immediately"));
		return;
	}
	// Allow only the first call of this
	m_postNotifyCount++;  // set to 0 in the CServicDiscovery constructor
	if (m_postNotifyCount > 1)
	{
		return;
	}

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
		size_t qlen = m_pSD->getQuery().Len() + 1;
#if defined(_DEBUG)
		wxString theQuery = m_pSD->getQuery(); // temp, to see what's in it
		wxLogDebug(_T("wxServDisc %p:  BEW theQuery:  %s  Its Length plus 1: %d"), m_pSD, theQuery.c_str(), (int)qlen);
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
			wxLogDebug(_T("wxServDisc %p: (BEW)  name: %s  Len(): %d  ip: %s  port: %d  time: %d"),
							m_pSD, aName.c_str(), nameLen, ip.c_str(), port, time);
#endif
			// what's really wanted - just the bit before the first ._ sequence
			entry_count++;
#if defined(_DEBUG)
			wxString astring = it->name.Mid(0, it->name.Len() - qlen);
			wxLogDebug(_T("wxServDisc %p:  m_sd_servicenames receives servicename:  %s   for entry index = %d"), 
							m_pSD, astring.c_str(), entry_count - 1);
#endif
			m_sd_servicenames.Add(it->name.Mid(0, it->name.Len() - qlen));

			m_bArr_ScanFoundNoKBserver.Add(0); // add FALSE, as we successfully
				// discovered one (but that does not necessarily mean we will
				// subsequently succeed at looking up hostname, port, and ip address)

		} // end of loop: for (it = entries.begin(); it != entries.end(); it++)

		// Next, lookup hostname and port. Code for this is copied and tweaked,
		// from the list_select() event handler of MyFrameMain. Since we have
		// this now in a loop in case more than one KBserver is running, we
		// must then embed the associated addrscan() call within the loop
		size_t i;
		bool bThrowAwayDuplicateIPaddr = FALSE;
		for (i = 0; i < entry_count; i++)
		{
			wxServDisc namescan(0, m_pSD->getResults().at(i).name, QTYPE_SRV);

			bThrowAwayDuplicateIPaddr = FALSE; // initialize for every iteration
			//timeout = 3000;
			timeout = 5000; // try 6 seconds, it's safer for a slow network
					// I've had a few timeouts happen at 5 sec, in debug build
					// with KBserver on my Ubuntu laptop in a VM
			while (!namescan.getResultCount() && timeout > 0)
			{
				wxMilliSleep(25);
				timeout-=25;
			}
			if(timeout <= 0)
			{
				wxLogError(_T("wxServDisc %p:  Timeout looking up hostname. Entry index: %d"), m_pSD, i);
				m_hostname = m_addr = m_port = wxEmptyString;
				m_bArr_HostnameLookupFailed.Add(1); // adding TRUE
				m_bArr_IPaddrLookupFailed.Add(-1); // undefined, addrscan() for this index is not tried
				m_bArr_DuplicateIPaddr.Add(-1); // undefined, whether a duplicate ipaddr is untried
#if defined(_DEBUG)
				wxLogDebug(_T("wxServDisc %p:  Found: [Service: %s ] but timed out:  m_hostname:  %s   m_port  %s   for entry index = %d"),
				m_pSD, m_sd_servicenames.Item(i).c_str(), m_hostname.c_str(), m_port.c_str(), i);
#endif
				//return;
				continue; // don't return, we need to try every iteration
			}
			else
			{
				// The namescan found something...  (we only find kbserver hostname)
				m_hostname = namescan.getResults().at(0).name;
				m_port = wxString() << namescan.getResults().at(0).port;
				m_bArr_HostnameLookupFailed.Add(0); // adding FALSE, the lookup succeeded
#if defined(_DEBUG)
				wxLogDebug(_T("wxServDisc %p:  Found: [Service: %s ] Looked up:  m_hostname:  %s   m_port  %s   for entry index = %d"),
				m_pSD, m_sd_servicenames.Item(i).c_str(), m_hostname.c_str(), m_port.c_str(), i);
#endif
				// For each successful namescan(), we must do an addrscan, so as to fill
				// out the full info needed for constructing a URL; if the namescan was
				// not successful, the m_bArr_IPaddrLookupFailed entry for this index
				// should be neither true (1) nor false (0), so use -1 for "no test was made"
				{
					wxServDisc addrscan(0, m_hostname, QTYPE_A);

					//timeout = 3000;
					timeout = 5000; // try 5 seconds (so far seems to be enough, no, it timed out once)
					while (!addrscan.getResultCount() && timeout > 0)
					{
						wxMilliSleep(25);
						timeout-=25;
					}
					if(timeout <= 0)
					{
						wxLogError(_T("Timeout looking up IP address."));
						m_hostname = m_addr = m_port = wxEmptyString;
						m_bArr_IPaddrLookupFailed.Add(1); // for TRUE, unsuccessful lookup
						m_bArr_DuplicateIPaddr.Add(-1); // undefined, whether a duplicate ipaddr is untried
#if defined(_DEBUG)
						wxLogDebug(_T("wxServDisc %p: ip Not Found: [Service: %s ] Timed out:  ip addr:  %s  for entry index = %d"),
						m_pSD, m_sd_servicenames.Item(i).c_str(), m_addr.c_str(), i);
#endif
						//return;
						continue; // do all iterations
					}
					else
					{
						// succeeded in getting the service's ip address
						m_addr = addrscan.getResults().at(0).ip;

						// Check for a unique ip address, if not unique, abandon this
						// iteration (do a case sensitive compare)
						bThrowAwayDuplicateIPaddr = IsDuplicateStrCase(&m_uniqueIpAddresses, m_addr, TRUE);
						if (!bThrowAwayDuplicateIPaddr)
						{
							// Not a duplicate, so don't throw it away, store it
							AddUniqueStrCase(&m_uniqueIpAddresses, m_addr, TRUE);
							m_bArr_IPaddrLookupFailed.Add(0); // for FALSE, a successful ip lookup
							m_bArr_DuplicateIPaddr.Add(0); // it's not a duplicate
#if defined(_DEBUG)
							wxLogDebug(_T("wxServDisc %p: Found: [Service:  %s  ] Looked up:  ip addr:  %s   for entry index = %d"),
							m_pSD, m_sd_servicenames.Item(i).c_str(), m_addr.c_str(), i);
#endif
						}
						else
						{
							// It's a duplicate ip address
							m_bArr_IPaddrLookupFailed.Add(0); // for FALSE, a successful ip lookup
							m_bArr_DuplicateIPaddr.Add(1); // it's a duplicate
							//continue;  <<- no continue here, we'll allow the url to be
							//constructed below; the m_bArr_DuplicateIPaddr array entry
							//being 1 will allow us to ignore it later on
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
			// Note: onSDNotify() will not have been called if no service was discovered,
			// so we don't need to test m_bArr_ScanFoundNoKBserver, as it will be 0
			if (m_bArr_HostnameLookupFailed.Item(i) == 0 &&
				m_bArr_IPaddrLookupFailed.Item(i) == 0)
			{
				// Both the first, and any duplicate ipaddr are added to the m_urlsArr
				// here, but duplicates are marked by the flag for a duplicate being 1
				m_urlsArr.Add(protocol + m_addr);
#if defined(_DEBUG)
				wxLogDebug(_T("wxServDisc %p:  Found: [Service: %s ] Constructed URL:  %s  for entry index = %d"),
				m_pSD, m_sd_servicenames.Item(i).c_str(), (protocol + m_addr).c_str(), i);
#endif
			}
			else
			{
				wxString emptyStr = _T("");
				m_urlsArr.Add(emptyStr);
			}

		} // end of loop: for (i = 0; i < entry_count; i++)

		// Make the results accessible: store them as 1 or more strings in 
		// m_pApp->m_servDiscResults
		// Generate the one (usually only one) or more lines, each corresponding to
		// a discovery of a multicasting KBserver instance (not all lookups might
		// have been error free, so some urls may be absent, and such lines may just
		// end up containing error data; & ipaddr duplicates are included here too)
		wxString colon = _T(":");
		wxString intStr;
		if (m_urlsArr.IsEmpty())
		{
			return;
		}
		for (i = 0; i < (size_t)entry_count; i++)
		{
			wxString aLine = m_urlsArr.Item((size_t)i); // either a URL, or an empty string
			aLine += colon;
			wxItoa(m_bArr_ScanFoundNoKBserver.Item((size_t)i), intStr);
			aLine += intStr + colon;
			wxItoa(m_bArr_HostnameLookupFailed.Item((size_t)i), intStr);
			aLine += intStr + colon;
			wxItoa(m_bArr_IPaddrLookupFailed.Item((size_t)i), intStr);
			aLine += intStr + colon;
			wxItoa(m_bArr_DuplicateIPaddr.Item((size_t)i), intStr);
			aLine += intStr;
			m_pApp->m_servDiscResults.Add(aLine); // BEW 4Jan16
		}
#if defined(_DEBUG)
		wxLogDebug(_T("wxServDisc %p:  Finished storing constructed URLs in m_pApp->m_pServDiscResults  ******"));
#endif

	} // end of TRUE block for test: if (m_pSD != NULL)
	else
	{
		// major error, but the program counter has never entered here, so
		// just log it if it happens
		gpApp->LogUserAction(_T("GetResults():unexpected error: ptr to wxServDisc instance, m_pSD, is NULL"));
		wxLogDebug(_T("ServiceDiscovery::GetResults() (m_pSD != NULL) test:  was FALSE, m_pSD =  %p"), m_pSD);
		return;
	}

	// App's DoServiceDiscovery() function can exit from WaitTimeout(), awakening main
	// thread and it's event handling etc, so do Signal() to make this happen
	wxLogDebug(_T("wxServDisc %p:  At end of CServiceDiscovery::GetResults() m_pMutex  =  %p"), 
				m_pSD, m_pMutex);
	if (gpApp->m_bResultsAccessedOnce == FALSE)
	{
        // This is the first access by one of the possibly several post_notify() calls made
        // by a few (mostly 3) wxServDisc processes, to get this far. We want only the
        // first such one to actually access the locker and call Signal(), because calling
        // GetResults() can happen again because another owning wxServDisc process is still
        // not timed out and is still runing, AFTER the CServiceDiscovery instance which
        // kicked the whole service discovery functionality off has been deleted in
        // response to the wxServDiscHALTING event having been posted earlier. If such
        // access is made, of course there would be an app crash. So we set
        // m_bResultsAccessedOnce to TRUE after having entered this block here once, and
        // thereafter, other processes getting to this point subsequently will have access
        // to this block skipped, and go to the else block where they will exit
        // immediately, letting the function die without causing a crash
        // BEW additional note: the system is lazy for class and window deletions, it
        // apparently waits for idle time to do so. An unwanted consequence is that the
        // wxServDisc instance which succeeded first in the discovery gets deleted after
        // the parent CServiceDiscovery instance it depends on has been deleted - that 
        // persistently led to a crash on every test. The solution was to use the halting
        // event for CServiceDiscovery to just set a boolean to TRUE, and in the OnIdle()
        // handler, check for this boolaean being TRUE, and if so, do the deletion of
        // CServiceDiscovery there, and set its m_pServDisc pointer to NULL. That removed
        // the crash, because it resulted in a reordering of the sequence of deletions.
		gpApp->m_bResultsAccessedOnce = TRUE;
		wxMutexLocker locker(*m_pMutex); // make sure it is locked
		bool bIsOK = locker.IsOk(); // check the locker object successfully acquired the lock
		wxUnusedVar(bIsOK);
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
			m_pSD, myError.c_str(), bIsOK ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str());
#endif
	}
	else
	{
		// Go no further -- see the comments in the block above
		return;
	}

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
    wxLogDebug(_T("wxServDisc %p: BEW: GetResults(), block finished. Now have posted event wxServDiscHALTING."), m_pSD);
	}

	wxLogDebug(wxT("wxServDisc %p: BEW: A KBserver was found. GetResults() is exiting, wxServDiscHALTING has been posted"), m_pSD);
}

// BEW Getting the module shut down in the two circumstances we need:
// (a) after one or more KBserver instances running have been discovered -- GetResults()
// is called, and we can put shut-down code in the end of that handler - we post an
// event to say we are halting service discovery -- event is wxServDiscHALTING
// (b) when no KBserver instance is runnning (I let a WaitTimeout() do it's job and
// the main thread wakes up, tests and finds no results, etc).
//
// Beier's SDWrap embedded the wxServDisc instance in the app, and so when the app got shut
// down, the lack of well-designed shutdown code didn't matter, as the memory leaks got blown
// away. But for me, ServDisc needs to exist only for a single try at finding a KBserver
// instance, and then die forever - or until explicitly instantiated again, and die without
// leaking memory. So I've beefed up the code for returning blocks to the heap, and
// encapsulated it all within a DoServiceDiscovery() function, an app member function
void CServiceDiscovery::onSDHalting(wxCommandEvent& event)
{
	wxUnusedVar(event);

	// If wxServDisc at process end has posted this wxServDiscHALTING event, then
	// it's almost certain that the GetResults() function has not yet finished its
	// work (it's on the main thread, while wxServDisc is a detached thread)

	// BEW made this handler, for shutting down the module when no KBserver
	// service was discovered, and for when one was discovered and post_notify()
	// posted a wxServDiscNOTIFY event to get the lookup and reporting done for
	// the discovered service(s) (there could be 2 or more KBserver instances running
	// so more than one could be discovered)
	wxLogDebug(_T("wxServDisc %p:  this [from CServiceDiscovery:onSDHalting()] = %p"), m_pSD, this);
	wxLogDebug(_T("wxServDisc %p:  m_pParent [from CServiceDiscovery:onSDHalting()] = %p"), m_pSD, m_pParent);

	m_bWxServDiscIsRunning = FALSE;
	wxLogDebug(_T("wxServDisc %p:  Starting CServiceDiscovery:onSDHalting(): m_bWxServDiscIsRunning initialized to FALSE"),
				m_pSD);

    // It's not necessary to clear the following, the destructor would do it,
    // but no harm in it
	m_sd_servicenames.Clear();
	m_uniqueIpAddresses.Clear();
	m_urlsArr.Clear();
	m_sd_lines.Clear();
	m_bArr_ScanFoundNoKBserver.Clear();
	m_bArr_HostnameLookupFailed.Clear();
	m_bArr_IPaddrLookupFailed.Clear();
	m_bArr_DuplicateIPaddr.Clear();

    // BEW: Post the custom wxServDiscHALTING event here, for the parent class to
    // supply the handler needed for destroying this CServiceDiscovery instance
	wxCommandEvent upevent(wxServDiscHALTING, wxID_ANY);
	upevent.SetEventObject(this); // set sender

#if wxVERSION_NUMBER < 2900
	wxPostEvent((CAdapt_ItApp*)m_pParent, upevent);
#else
	wxQueueEvent((CAdapt_ItApp*)m_pParent, upevent.Clone());
#endif
	wxLogDebug(_T("wxServDisc %p:  [from CServiceDiscovery:onSDHalting()] AFTER posting wxServDiscHALTING event, this = %p, m_pParent (the app) = %p"), 
			m_pSD, this, m_pParent);

	delete m_pSD; // must have this, it gets ~wxServDisc() destructor called
}

CServiceDiscovery::~CServiceDiscovery()
{
	wxLogDebug(_T("Deleting  %p , the CServiceDiscovery instance, in ~CServiceDiscovery()"), this);
}

// Copied wxItoa from helpers.cpp, as including helpers.h leads to problems
void CServiceDiscovery::wxItoa(int val, wxString& str)
{
	wxString valStr;
	valStr << val;
	str = valStr;
}

#endif // _KBSERVER
