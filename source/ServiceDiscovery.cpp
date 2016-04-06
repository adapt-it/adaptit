
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

// The following are copied from 1035.h lines 26-28, as they are needed here (trying to avoid
// an #include "mdnsd.h line here, because that leads to winsock.h name collisions with winsock2.h")
#define QTYPE_A 1
#define QTYPE_PTR 12
#define QTYPE_SRV 33
/*
// and a copy of the definition of mdnsa, from mdnsd.h, for the same reason
typedef struct mdnsda_struct
{
	unsigned char *name;
	unsigned short int type;
	unsigned long int ttl;
	unsigned short int rdlen;
	unsigned char *rdata;
	unsigned long int ip; // A
	unsigned char *rdname; // NS/CNAME/PTR/SRV
	struct { unsigned short int priority, weight, port; } srv; // SRV
} *mdnsda;
*/
#include <vector>
#define WIN32_LEAN_AND_MEAN // does the same job as above, likewise here just in case
#define _WINSOCKAPI_ // keeps winsock.h from being included in <Windows.h>, it's here just in case
//#include "winsock2.h"


#include "wxServDisc.h"
#include "ServiceDiscovery.h"
#include "helpers.h"


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

extern wxMutex	kbsvr_arrays;

extern CAdapt_ItApp* gpApp;
CServiceDiscovery* gpServiceDiscovery; // wxServDisc's creator needs the value we place here

BEGIN_EVENT_TABLE(CServiceDiscovery, wxEvtHandler)
EVT_COMMAND(wxID_ANY, wxServDiscNOTIFY, CServiceDiscovery::onSDNotify)
EVT_COMMAND (wxID_ANY, wxServDiscHALTING, CServiceDiscovery::onSDHalting)
END_EVENT_TABLE()

CServiceDiscovery::CServiceDiscovery()
{
	m_servicestring = _T("");
	m_bWxServDiscIsRunning = TRUE;
	wxUnusedVar(m_servicestring);
}

CServiceDiscovery::CServiceDiscovery(wxString servicestring, CAdapt_ItApp* pParentClass)
{
	gpServiceDiscovery = this; // wxServDisc creator needs this; set it early, so that it has
							   // it's correct value before wxServDisc is instantiated below

	wxLogDebug(_T("\nInstantiating a CServiceDiscovery class, passing in servicestring: %s, ptr to instance: %p"),
		servicestring.c_str(), this);

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

	// initialize scratch variables...
	m_hostname = _T("");
	m_addr = _T("");
	m_port = _T("");

	// Initialize our flag which helps this process and the wxServDisc child process,
	// to work out when to initiate the posting of wxServDiscHALTING to get cleanup done
	m_bOnSDNotifyEnded = FALSE;
	// Also need this one, because we must exit from onSDHalting() early only when
	// onSDNotify() has started, but not yet ended
	m_bOnSDNotifyStarted = FALSE;
	m_pParent->m_bCServiceDiscoveryCanBeDeleted = FALSE; // initialize
	nCleanupCount = 0; // initialize
	nPostedEventCount = 0; // bump it by 1 each time a posting of wxServDiscNOTIFY event happens & log it

	// Initialize my reporting context (Use .Clear() rather than
	// .Empty() because from one run to another we don't know if the
	// number of items discovered will be the same as discovered previously
	// Note: if no KBserver service is discovered, these will remain cleared.
	m_sd_servicenames.Clear();
	m_uniqueIpAddresses.Clear();
	m_urlsArr.Clear();
	m_theirHostnames.Clear();

    // wxServDisc creator is: wxServDisc::wxServDisc(void* p, const wxString& what, int
    // type) where p is pointer to the parent class & what is the service to scan for, its
    // type is QTYPE_PTR (value = 12), and for the record, QTYPE_SVR is 33, and QTYPE_A is 1
	m_pWxSD = new wxServDisc(this, m_servicestring, QTYPE_PTR);
	wxUnusedVar(m_pWxSD);
	wxLogDebug(_T("wxServDisc %p: just now instantiated in CServiceDiscovery creator"), m_pWxSD);
}

// This function in Beier's solution is an even handler called onSDNotify(). When wxServDisc
// discovers a service that it is scanning for, it reports its success here. Adapt It needs 
// just the ip address(s), but not the port. We present URL(s) to the AI code which it can 
// use for automatic connection to the running KBserver.
// Ideally, only one KBserver runs at any given time, but we have to allow for users being
// perverse and running more than one, or for a workshop in which different groups each
// have a running KBserver on the LAN. If they do, we'll have to let them make a choice of
// which URL to connect to. Our service discovery module, however, only can find a single
// one of them quickly (less than 4 secs), so we'll limit it to that. Our support for
// multiple KBservers running will be by using a time to call the disovery code to run
// on a background thread periodically, and to aggregate a list of the unique URLs found
// over the life of the app. We also need to remove (programmatically) any that users
// turn off during the life of the app.
// This service discovery module will just be instantiated, scan for _kbserver._tcp.local.
// lookup the ip addresses, deposit finished URL or URLs in the wxArrayString in the
// CAdapt_ItApp::m_servDiscResults array, and then kill itself.

void CServiceDiscovery::onSDNotify(wxCommandEvent& event)
{
	size_t i;
	if (event.GetEventObject() == m_pWxSD)
	{
		// Each entry into onSDNotify() will set this bool true, but the first entry should
		// ultimately get to finished here first - and when that happens, any others should
		// be further back in their processing and we can abandon them.
		m_bOnSDNotifyStarted = TRUE;

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
		size_t qlen = m_pWxSD->getQuery().Len() + 1;
		#if defined(_DEBUG)
		wxString theQuery = m_pWxSD->getQuery(); // temp, to see what's in it
		wxLogDebug(_T("theQuery:  %s  Its Length plus 1: %d"), theQuery.c_str(), (int)qlen);
		#endif
		vector<wxSDEntry> entries = m_pWxSD->getResults();
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
			wxString astring = it->name.Mid(0, it->name.Len() - qlen);
			wxLogDebug(_T("m_sd_servicenames receives servicename:  %s   for entry index = %d"),
				astring.c_str(), entry_count - 1);
			#endif
			m_sd_servicenames.Add(it->name.Mid(0, it->name.Len() - qlen));

		} // end of loop: for (it = entries.begin(); it != entries.end(); it++)

		// Next, lookup hostname and port. Code for this is copied and tweaked,
		// from the list_select() event handler of MyFrameMain. 
		
		// BEW 14Mar16 parent ptr (void*) passed in in Beier's solution is NULL. But a plethora of
		// wxServDisc instances get created, and they consume so much CPU time that the discovery
		// gets severe constapation (actually, block bowel). I've found one wxServDisc query instance
		// will find all the KBservers, so my changes are to get all the wxServDisc instances created
		// instantly exited without doing anything, except the one that CServiceDiscover::m_pWxSD points
		// at (the one it creates), and the two that that wxServDisc instance, namescan, and addrscan,
		// are created for completing the discovery of hostname, port, and ip address. Any others, we'll
		// let them be instantiated, but within them do nothing but cause immediate disposal. Our third
		// one, the addrscan instance, is what will write results data to CServiceDiscovery's arrays.
		void* pVoidPtr = 0; // pVoidPtr is what we'll pass into namescan and addrscan. If this ptr is
							//the CServiceDiscovery instance, then pass in its m_pWxSD ptr which is the 
							// it owns; any other wxServDisc instances will receive NULL (0) - and in those
							// we'll test for the zero and skip doing anything except initiate disposal
/*		
		if ((void*)this == (void*)gpServiceDiscovery->m_pWxSD)
		{
			// If I am wxServDisc instance that CServiceDiscovery instantiated (and therefore owns)
			// then I need to call two local child instances in this handler, namescan() and after that,
			// addrscan(). I need to pass a non-NULL pointer to their parent, which is the ptr to the 
			// wxServDisc instance that CServiceDiscovery instantiated & owns (and which is the only one
			// which we'll allow to cause onSDNotify() to run by trapping a posted wxServDiscNOTIFY
			// event). (Any other internally generated wxServDisc instance will have the first parameter
			// passed as NULL, and for those, we'll alter Entry() so that none of Entry()'s scanning
			// code runs. This saves time.)
			pVoidPtr = m_pWxSD; // the ptr to pass in is the ptr to the wxServDisc instance which
								// CServiceDiscovery instantiated & owns
		}
*/		
		for (i = 0; i < entry_count; i++)
		{
			wxServDisc namescan(0, m_pWxSD->getResults().at(i).name, QTYPE_SRV); // Beier's original
			//wxServDisc namescan(pVoidPtr, m_pWxSD->getResults().at(i).name, QTYPE_SRV); // remember,
						// this looks like a local function, but actually it runs as a detached thread,
						// so control will immediately move on - so the timeout loop below is mandatory.
						// 3rd param = 33
			wxLogDebug(_T("onSDNotify() (304) wxServDisc %p  &  passing into namescan's first param: %p:  for loop starts, entry_count = %d  index = %d"),
				m_pWxSD, pVoidPtr, entry_count, i);

			//timeout = 4500;
			timeout = 3000;
			while (!namescan.getResultCount() && timeout > 0)
			{
				wxMilliSleep(25);
				timeout -= 25;
			}
			if (timeout <= 0)
			{
				wxLogError(_T("Timeout looking up hostname. Entry index: %d"), i);
				m_hostname = m_addr = m_port = wxEmptyString;
				#if defined(_DEBUG)
				wxLogDebug(_T("onSDNotify() (319) wxServDisc  %p  &  parent %p:  namescan() Timed out:  m_hostname:  %s   m_port  %s"),
						m_pWxSD, this, m_hostname.c_str(), m_port.c_str());
				#endif
				return;
			}
			else
			{
				// The namescan found something...
				m_hostname = namescan.getResults().at(0).name;
				m_port = wxString() << namescan.getResults().at(0).port;

				#if defined(_DEBUG)
				wxLogDebug(_T("wxServDisc %p (331)  &  parent %p: onSDNotify:  Found Something"),
							m_pWxSD, this);
				#endif

				// For each successful namescan(), we must do an addrscan, so as to fill
				// out the full info needed for constructing a URL
				wxServDisc addrscan(0, m_hostname, QTYPE_A); // Beier's original
				//wxServDisc addrscan(pVoidPtr, m_hostname, QTYPE_A);  // remember, this looks
							// like a local function, but actually it runs as a detached thread, so 
							// control will immediately move on - so the timeout loop below is mandatory
				wxLogDebug(_T("onSDNotify() (341) wxServDisc %p  &  passing into addrscan's first param: %p:  entry_count = %d  index = %d"),
						m_pWxSD, pVoidPtr, entry_count, i);
				timeout = 3000;
				while (!addrscan.getResultCount() && timeout > 0)
				{
					wxMilliSleep(25);
					timeout -= 25;
				}
				if (timeout <= 0)
				{
					wxLogError(_T("wxServDisc %p   Timeout looking up IP address."), m_pWxSD);
					m_hostname = m_addr = m_port = wxEmptyString;
					#if defined(_DEBUG)
					wxLogDebug(_T("wxServDisc %p  (354) ip Not Found: [Service:  %s  ] Timed out:  ip addr:  %s   for entry index = %d"),
								m_pWxSD, m_sd_servicenames.Item(i).c_str(), m_addr.c_str(), i);
					#endif
					return;
				}
				else  // we have SUCCESS! -- Now access addrscan()'s getResults to get the ip address
				{
					// Didn't time out...
					m_addr = addrscan.getResults().at(0).ip; // Beier's original
					m_port = wxString() << addrscan.getResults().at(0).port; // Beier's original

					kbsvr_arrays.Lock();
					// Only add this ip address to m_uniqueIpAddresses array if it is not already in the array
					bool bItWasUniqueIpAddr = AddUniqueStrCase(&m_uniqueIpAddresses, m_addr, TRUE); // does .Add() if it is unique
					kbsvr_arrays.Unlock();




					if (bItWasUniqueIpAddr)
					{
						#if defined(_DEBUG)
						kbsvr_arrays.Lock();
						wxLogDebug(_T("onSDNotify() (359) wxServDisc %p  &  parent %p: SUCCESS, m_hostname:  %s  & m_port:  %s  &  m_addr:  %s  for iteration = %d of %d"),
							m_pWxSD, this, m_hostname.c_str(), m_port.c_str(), m_addr.c_str(), i, entry_count);
						kbsvr_arrays.Unlock();
						#endif

						kbsvr_arrays.Lock();
						m_uniqueIpAddresses.Add(m_addr);
						m_theirHostnames.Add(m_hostname);
						wxLogDebug(_T("onSDNotify() (385) wxServDisc %p  &  parent %p: Store m_addr ( %s ) in array m_uniqueIpAddresses. Store m_hostname ( %s ) in array m_theirHostnames."),
							m_pWxSD,this, m_addr.c_str(), m_hostname.c_str());
						// NOTE: we don't here use m_uniqueIpAddresses array, but we can't eliminate it as we
						// used it above in the AddUniqueStrCase() call, as first param
						kbsvr_arrays.Unlock();

						// We are only wanting one discovery, and we have it now, so break from this loop
						break;
					} // end of TRUE block for test: if (bItWasUniqueIpAddr)

				}

			} // end of else block for test: if(timeout <= 0) for namescan() attempt


		} // end of loop: for (i = 0; i < entry_count; i++)

		wxASSERT(!m_addr.IsEmpty());
		// Put it all together to get URL(s) & store in CMainFrame's m_urlsArr.
		// Since we here are still within the loop, we are going to try create a
		// url for what this iteration has succeeded in looking up. We can do so
		// provided hostname, ip, and port are not empty strings. We'll let
		// port be empty, as long as hostname and ip are not empty. ( I won't
		// construct a url with :port appended, unless Jonathan says I should.)
		wxString protocol = _T("https://");

		kbsvr_arrays.Lock();
		m_urlsArr.Add(protocol + m_addr);
		#if defined(_DEBUG)
		wxLogDebug(_T("onSDNotify() (414) wxServDisc %p  &  parent %p: for hostname: %s   Constructed URL for m_urlsArr:  %s   for iteration = %d of %d, m_urlsArr count is now: %d"),
			m_pWxSD, this, m_hostname.c_str(), (protocol + m_addr).c_str(), i, entry_count, m_urlsArr.GetCount());
		#endif
		kbsvr_arrays.Unlock();

		m_bOnSDNotifyEnded = TRUE; // module shutdown can now happen

	} // end of TRUE block for test: if (event.GetEventObject() == m_pWxSD)
	else
	{
		// major error, so don't wipe out what an earlier run may have stored
		// in the CMainFrame instance - program counter has never entered here
		;
	} // end of else block for test: if (event.GetEventObject() == m_pSD)

	  //  Post a custom wxServDiscHALTING event here, to get rid of my parent classes
	  // Post only one such event (the count starts as 0, and if post_notify() posts
	  // the event, the count is bumped. So allow this halting event to be posted only
	  // once - that is, when nPostedEventCount is 1 only
	if (gpServiceDiscovery->nPostedEventCount == 1)
	{
		wxCommandEvent event(wxServDiscHALTING, wxID_ANY);
		event.SetEventObject(this); // set sender

	// BEW added this posting...  Send it
#if wxVERSION_NUMBER < 2900
		wxPostEvent(this, event);
#else
		wxQueueEvent(this, event.Clone());
#endif
		wxLogDebug(_T("wxServDisc %p  &  parent %p: onSDNotify, block finished. Now have posted event wxServDiscHALTING."),
					m_pWxSD, this);
	}

	wxLogDebug(wxT("wxServDisc %p  &  parent %p: onSDNotify, A KBserver was found. onSDNotify() is exiting right now"),
					m_pWxSD, this);
}

// BEW Getting the module shut down in the two circumstances we need:
// (a) after a running KBserver instance has been discovered -- onSDNotify()
// is called, and we can put shut-down code in the end of that handler - we post an
// event to say we are halting service discovery -- event is wxServDiscHALTING
// (b) when no KBserver instance is runnning (I use an onServDiscHalting() handler of my
// own to get ~wxServDisc() destructor called, but that's all it can do safely).
//
// In the (b) case, my CServiceDiscovery instance, and my ServDisc instance, live on.
// So I've resorted to a hack, using a m_bWxServDiscIsRunning boolean, set to FALSE
// when no KBserver was discovered, at end of the ...Halting() handler,
// and the OnIdle() handler in frame window, to get the latter two classes clobbered.
//
// Beier's SDWrap embedded the wxServDisc instance in the app, and so when the app got shut
// down, the lack of well-designed shutdown code didn't matter, as the memory leaks got blown
// away. But for me, ServDisc needs to exist only for a single try at finding a KBserver
// instance, and then die forever - or until explicitly instantiated again, and die without
// leaking memory. So I've beefed up the code for returning blocks to the heap, and
// encapsulated the whole service discovery module within a DoServiceDiscovery() function,
//an app member function


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
	wxLogDebug(_T("In CServiceDiscovery:onSDHalting() m_pWxSD =  %p  will be deleted here."), m_pWxSD);

	m_bWxServDiscIsRunning = FALSE;
	wxLogDebug(_T("In CServiceDiscovery:onSDHalting(): m_bWxServDiscIsRunning restored to FALSE"));


/* temporary out, to see what happens if I have no DoServiceDiscovery() timeout loop and just let the threads move along unhindered by sleeps
	// Where does my_gc(d) 7 cleanup friends get called in this old solution? Try here, just before deleting
	// because the only other place they occur is at the end of Entry()'s loop, and the first (ie. owned)
	// wxServDisc instance will run on endlessly unless it is explicitly shut down, and to avoid leaking
	// memory we must call those cleanup functions here since here is where we call  delete m_pWxSD

	// If I get leaks when shutting down after successful discovery, then
	// a) make d a member var of wxServDisc, and use it to
	// b) call the cleanup functions here  -- I'll give it a try now
	if (m_bOnSDNotifyEnded)
	{
		m_pWxSD->CleanUpSD(m_pWxSD, m_pWxSD->d);
	}
	wxLogDebug(_T("In CServiceDiscovery:onSDHalting(): nCleanupCount = %d"), nCleanupCount);


	delete m_pWxSD; // must have this, it gets ~wxServDisc() destructor called

	wxLogDebug(_T("wxServDisc %p:  [from CServiceDiscovery:onSDHalting()] AFTER posting wxServDiscHALTING event, this = %p, m_pParent (the app) = %p"),
		m_pWxSD, this, m_pParent);


    // BEW: Post the custom wxServDiscHALTING event here, for the parent class to
    // supply the handler needed for destroying this CServiceDiscovery instance
	wxCommandEvent upevent(wxServDiscHALTING, wxID_ANY);
	upevent.SetEventObject(this); // set sender

#if wxVERSION_NUMBER < 2900
	wxPostEvent((CAdapt_ItApp*)m_pParent, upevent);
#else
	wxQueueEvent((CAdapt_ItApp*)m_pParent, upevent.Clone());
#endif
	wxLogDebug(_T("In CServiceDiscovery:onSDHalting()] posted wxServDiscHALTING event, CServiceDiscoverythis = %p,  m_pParent (the app) = %p"),
			this, m_pParent);
*/
}

CServiceDiscovery::~CServiceDiscovery()
{
	m_sd_servicenames.clear();
	m_uniqueIpAddresses.clear();
	m_theirHostnames.clear();
	m_urlsArr.clear();

	wxLogDebug(_T("Copying URLs to app::m_servDiscResults array, then Deleting the CServiceDiscovery instance = %p, in ~CServiceDiscovery()"), this);
}

// Copied wxItoa from helpers.cpp, as including helpers.h leads to problems
void CServiceDiscovery::wxItoa(int val, wxString& str)
{
	wxString valStr;
	valStr << val;
	str = valStr;
}

// BEW created 5Jan16, needed for GetResults() in CServiceDiscovery instance
bool CServiceDiscovery::IsDuplicateStrCase(wxArrayString* pArrayStr, wxString& str, bool bCase)
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
// Return TRUE if the passed in str gets Add()ed to pArrayStr, FALSE if it doesn't
bool CServiceDiscovery::AddUniqueStrCase(wxArrayString* pArrayStr, wxString& str, bool bCase)
{
	int count = pArrayStr->GetCount();
	if (count == 0)
	{
		pArrayStr->Add(str);
		return TRUE;
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
			return TRUE;
		}
		else
		{
			// it's in the array already, so ignore it
			return FALSE;
		}
	}
}
#endif // _KBSERVER
