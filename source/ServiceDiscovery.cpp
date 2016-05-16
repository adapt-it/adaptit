
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
/////////////////////////////////////////////////////////////////////////////


// BEW 6Apr16 Call ServDiscBackground() which runs in the background in a thread
// under timer control. The biggest problem is memory leaks, the ultimate
// solution required forcing the detachable wxServDisc instances to die
// before the parent CServiceDiscovery class does, and put the latter in a thread.
// (The thread was necessary to avoid blocking the main thread where the GUI is.)
// ServeDiscBackground internally instantiates the CServiceDiscovery class, one 
// only per thread, instance only, m_pServDisc[index] pointer. There can be up to
// MAX_SERV_DISC_RUNS (20) of such pointers. Default 1 (most users will never
// encounter more than one running at a time). We use a custom event
// (wxEVT_End_ServiceDiscovery -- see MainFrm.h & .cpp) to get
// the top level threads shut down, by posting the event from the thread's OnExit()
// function, and providing a handler in MainFrm.cpp to do the job of setting
// m_pServDiscThread[index] to NULL; we don't have to actually delete it as it is
// detachable and so destroys itself - which we cause at the appropriate time.
//
// The CServiceDiscovery constructor, when it runs, instantiates the first wxServDisc
// instance, which kicks off the service discovery which runs on further detached threads.
// Unique results are sent back to CServiceDiscovery with the help of a ptr, m_pCSD,
// within each wxServDisc, which points at the CServiceDiscovery instance that created
// the first in each such set. Under timer control, timer notifications send the set
// of unique results aggregated within CServiceDiscovery back to the m_ipAddrs_Hostnames
// wxArrayString, where the GUI for service discovery, ConnectUsingDiscoveryResults(),
// can access them to display to the user as a URL and its associated hostname.
//
// A few things are necessary for a maintainer to know: (1) CServiceDiscovery is mandatory,
// it is app-facing, and so it can #include Adapt_It.h, but wxServDisc must never see
// Adapt_It.h, because then hundreds of name conflicts arise with the Microsoft socket
// implementations. wxServDisc, for Windows build, uses winsock2.h which clashes
// horribly with the older winsock.h resources.
// (2) wxServDisc uses events posted to the CServiceDiscovery instance, to get
// hostname and ipaddress lookups done - these are done from the stackframe of an
// onSDNotify() event handler within CServiceDiscovery. This is Beier's original design,
// and it is efficient, because wxServDisc may spawn multiple new instances of itself,
// and some of those will post notifications to CServiceDiscovery to get onSDNotify()
// called, doing so more than once - so we take steps to avoid multiple entries to
// onSDNotify(). It doesn't appear to need mutext protection, so I've
// not provided it (except where necessary, for array .Add() calls). The in-parallel set
// of running wxServDisc instances can quickly swamp the CPUs, even on 4 core or higher
// machines, so the trick is to run the discovery for only a few seconds - I'm setting
// the limit to be 5 seconds (Beier's GC value, his was num of seconds in a day!) and
// then it needs to shut itself down. In practice, we force shutdown before the 5 second
// limit is reached. Out solution finds one and shuts down in a fraction over 4 secs.
// The first paramater of the wxServDisc() signature is very important. It is
// a (void*) for the parent class. Beier's solution makes use of the fact that if null
// is passed in, the new wxServDisc instance is unable to call post_notify() which
// otherwise would result in an embedded calling of the onSDNotify() handler - leading
// to chaos. We, instead, pass in the pointer to the owning CServiceDiscovery instance.
// If an instance is spawned with null as the first param value, we regard it as a
// unwelcome nuisance (a zombie) and we shut it down immediately. I was getting two of
// these early on, but the final solution does not generate any for an unknown reason.
// Our solution deliberately is designed to find only one KBserver per run.
// To try find more leads to many difficult problems to solve, and CPU-binding problems.
// Finding one is quick, usually less than 3 seconds, and a bit more time for cleanup
// and staged shutdowns. The best solution is to run this simpler solution in a burst
// of instantiations, in the background, and accumulate a list of discovered running 
// KBservers - their urls and hostnames.
// To facilitate the multiple intermittent timed service discovery instantiations, their 
// self-destruction *must* be leakless. Unfortunately, Beier's original Zeroconf solution
// leaks like a sieve, and so extra work had to be done to plug the leaks.
// 
// The above comments are a distillation of the knowledge gained from debug logging,
// and visual leak detection, done over 18 months of frustrating testing and tweaking.
// Ignore this and fiddle with it yourself at your own peril. You've been warned!
// VisLeakDetector can be turned on or off at line 279 of Adapt_It.cpp.
// At the top of the Adapt_It.cpp, Thread_ServiceDiscovery.cpp, CServiceDiscovery.cpp
// and wxServDisc.cpp files are some commented out #defines. Uncomment the ones you want
// to get very useful wxLogDebug output sent to the Output window. I could not have
// tamed this zeroconf solution of Beier's without them. DO NOT REMOVE THEM PLEASE.
//
// The timer interval (in milliseconds), and the number of (single) service discovery
// runs in a burst, are parameters in the AI-BasicConfiguration.aic file - and at this
// point in time we provide no GUI support for changing them. It is possible to safely
// change them by manually editing the config file and resaving it. The range of values
// allows are: timer interval - greater or equal to 7.111 seconds (7111 millisecs),
// and number of runs per burst, 1 or more, 20 or less. The config files enforce these.
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

#include <vector>
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_ // keeps winsock.h from being included in <Windows.h>, it's here just in case

#include "wxServDisc.h"
#include "Adapt_It.h"
#include "MainFrm.h"
#include "ServiceDiscovery.h"
#include "helpers.h"

// to enable or suppress logging, comment out to suppress
//#define _zerocsd_

// a few of the ones _zerocsd_ turned on, a minimal number, are not turned on with _minimalsd_
//#define _minimalsd_

// this one, if defined, displays each <url>@@@<hostname> string sent to the app, but a 
// WITHHOLDING message if it is a duplicate. Comment out to suppress displaying that info
#define _tracking_transfers_

// Comment out the next line to disable wxLogDebug() logging related to shutdown of the 
// service discovery run - same define is in Thread_ServiceDiscovery.cpp and wxServDisc.cpp
// This is the one I used to give me logging helpful for tracking down the cause of access
// violations - the final hurdle for successful service discovery multiple runs.
//#define _shutdown_

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

BEGIN_EVENT_TABLE(CServiceDiscovery, wxEvtHandler)

EVT_COMMAND(wxID_ANY, wxServDiscNOTIFY, CServiceDiscovery::onSDNotify)
EVT_COMMAND(wxID_ANY, wxServDiscHALTING, CServiceDiscovery::onSDHalting)

END_EVENT_TABLE()

CServiceDiscovery::CServiceDiscovery()
{
}

CServiceDiscovery::CServiceDiscovery(CAdapt_ItApp* pParentClass)
{
	m_serviceStr = _T("_kbserver._tcp.local.");
	m_bDestroyChildren = FALSE;

	wxASSERT((void*)&wxGetApp() == (void*)pParentClass);
	m_pApp = pParentClass;

#if defined(_shutdown_)
	wxLogDebug(_T("\nCServiceDiscovery  %p  Instantiating class. The m_serviceStr = %s"),
		this, m_serviceStr.c_str());
#endif
	m_pParent = pParentClass; 

	m_postNotifyCount = 0; // use this int as a filter to allow only one onSDNotify() call

	// initialize scratch variables...
	m_hostname = _T("");
	m_addr = _T("");
	m_port = _T("");

	// Initialize my reporting context (Use .Clear() rather than
	// .Empty() because from one run to another we don't know if the
	// number of items discovered will be the same as discovered previously
	// Note: if no KBserver service is discovered, these will remain cleared.
	m_sd_servicenames.Clear();
	m_ipAddrs_Hostnames.Clear(); //stores <ipaddr>@@@<hostname>

    // wxServDisc creator is: wxServDisc::wxServDisc(void* p, const wxString& what, int
    // type) where p is pointer to the parent class & what is the service to scan for, its
    // type is QTYPE_PTR (value = 12), and for the record, QTYPE_SVR is 33, and QTYPE_A is 1
	//m_pWxSD = new wxServDisc(this, *m_pServiceStr, QTYPE_PTR);
	m_pWxSD = new wxServDisc(this, m_serviceStr, QTYPE_PTR);
	wxUnusedVar(m_pWxSD);

#if defined(_zerocsd_)
	wxLogDebug(_T("wxServDisc %p: just now instantiated in CServiceDiscovery creator"), m_pWxSD);
#endif
}

// This function in Beier's solution is an event handler called onSDNotify(). When wxServDisc
// discovers a service that it is scanning for, it reports its success here. Adapt It needs 
// just the ip address(s), but not the port. We present URL(s) and their hostnames to the AI 
// code which it can then use, by user choosing one, for connection to the running KBserver.
// Ideally, only one KBserver runs at any given time, but we have to allow for users being
// perverse and running more than one, or for a workshop in which different groups each
// have a running KBserver on the LAN. If they do, we'll have to let them make a choice of
// which URL to connect to. Our service discovery module, however, only can find a single
// one of them quickly (less than 4 secs), so we'll limit it to that. Our support for
// multiple KBservers running will be by using a timer to call the discovery code running
// on a background thread periodically, and to aggregate a list of the unique URLs found
// in a burst of 9 discovery attempts spread over about 1.5 minutes. We let the user,
// by a menu choice, initiate one or more bursts - for example, if a new KBserver is 
// set running on the LAN during an AI session. We also need to remove (programmatically) 
// any that users turn off during the life of the app - but that is independent of this 
// discovery module - and I've not yet tried to code for doing so.
// This service discovery module will just be instantiated, scan for _kbserver._tcp.local.
// lookup the ip addresses, deposit finished URL or URLs in the wxArrayString in the
// CAdapt_ItApp::m_servDiscResults array, and then kill itself; but periodically for the
// life of the app session. 
void CServiceDiscovery::onSDNotify(wxCommandEvent& WXUNUSED(event))
{
	size_t i; m_hostname.Empty(); m_addr.Empty(); m_port.Empty();
	size_t entry_count = 0;
	int timeout;

	// length of query plus leading dot
	size_t qlen = m_pWxSD->getQuery().Len() + 1;

#if defined(_shutdown_)
	wxLogDebug(_T("CServiceDiscovery = %p (200), onSDNotify() Begins... for the OWNED wxServDisc  %p"), this, m_pWxSD);
#endif
	// BEW 12Apr16 I get size() values for entries of over 100 million, which suggests
	// some rubbish value is defining capacity. So I'll request a capacity of 99.
	// entry_count can got to a value greater than 1, but I want only one traverse
	// of the second for loop below
	vector<wxSDEntry> entries;
	entries.reserve(99);
	entries = m_pWxSD->getResults();
	vector<wxSDEntry>::const_iterator it;
	for (it = entries.begin(); it != entries.end(); it++)
	{
		// what's wanted - just the bit before the first ._ sequence
		entry_count++;
#if defined(_zerocsd_)
		// BEW added: let's have a look at what is returned
		wxString aName = it->name;
		int nameLen = aName.Len();
		wxString ip = it->ip;
		int port = it->port;
		long time = it->time;
		wxLogDebug(_T("name: %s  Len(): %d  ip: %s  port: %d  time: %d"),
			aName.c_str(), nameLen, ip.c_str(), port, time);
#endif
#if defined(_zerocsd_)
		wxString astring = it->name.Mid(0, it->name.Len() - qlen);
		wxLogDebug(_T("m_sd_servicenames receives servicename:  %s   for entry index = %d"),
			astring.c_str(), entry_count - 1);
#endif
		kbsvr_arrays.Lock();
		m_sd_servicenames.Add(it->name.Mid(0, it->name.Len() - qlen));
		kbsvr_arrays.Unlock();

	} // end of loop: for (it = entries.begin(); it != entries.end(); it++)
	if (entry_count > 1) entry_count = 1; // BEW 21Apr16, do one traverse only

	// BEW: Next, lookup hostname and port -  the for loop is Beier's (I think)
#if defined(_zerocsd_)
	wxLogDebug(_T("onSDNotify() (238) wxServDisc  %p  &  entry_count =  %d"), m_pWxSD, entry_count);
#endif
	bool bSuccessful = FALSE; // BEW added 21Apr16
	for (i = 0; i < entry_count; i++)
	{
		//wxServDisc namescan(0, m_pWxSD->getResults().at(i).name, QTYPE_SRV); // Beier's original
		// 3rd param = 33, note namescan() is local, so runs as a detached thread 
		// from onSDNotify()'s stackframe
		// BEW 21Apr16 pass in the CServiceDiscovery instance, we'll keep null for any zombies that get created
		wxServDisc namescan(this, m_pWxSD->getResults().at(i).name, QTYPE_SRV);

#if defined(_zerocsd_)
		wxLogDebug(_T("onSDNotify() (250) wxServDisc %p  &  passing into namescan's first param: %p :  for loop starts"), m_pWxSD, m_pWxSD);
#endif
		timeout = 3000;
		while (!namescan.getResultCount() && timeout > 0)
		{
			wxMilliSleep(25);
			timeout -= 25;
		}
		if (timeout <= 0)
		{
			// Beier's next message is a nuisance, it puts up a warning even in Release mode - so comment it out
			//wxLogError(_T("Timeout looking up hostname. Entry index: %d"), i);
			m_hostname = m_addr = m_port = wxEmptyString;
#if defined(_zerocsd_)
			wxLogDebug(_T("onSDNotify() (264) wxServDisc  %p  &  parent %p:  namescan() Timed out:  m_hostname:  %s   m_port  %s"),
				m_pWxSD, this, m_hostname.c_str(), m_port.c_str());
#endif
			return;
		}
		else
		{
			// The namescan found something...
			m_hostname = namescan.getResults().at(0).name;
			m_port = wxString() << namescan.getResults().at(0).port;
#if defined(_minimalsd_)
			wxLogDebug(_T("wxServDisc %p (275)  onSDNotify:  Found Something"), m_pWxSD);
#endif
			// BEW For each successful namescan(), we must do an addrscan, so as to fill
			// out the full info needed for constructing a URL later on
			//wxServDisc addrscan(0, m_hostname, QTYPE_A); // <<-- Beier's original
			// QTYPE_A is 1, note addrscan() is local, so runs as a detached thread 
			// from onSDNotify()'s stackframe
			// BEW 21Apr16 pass in the CServiceDiscovery instance, we'll keep null for any zombies that get created
			wxServDisc addrscan(this, m_hostname, QTYPE_A);

#if defined(_zerocsd_)
			wxLogDebug(_T("wxServDisc %p (286) onSDNotify() Passing into addrscan's first param: %p"), m_pWxSD, m_pWxSD);
#endif
			timeout = 3000;
			while (!addrscan.getResultCount() && timeout > 0)
			{
				wxMilliSleep(25);
				timeout -= 25;
			}
			if (timeout <= 0)
			{
#if defined(_zerocsd_)
				wxLogError(_T("wxServDisc %p (297) onSDNotify()  Timeout looking up IP address."), m_pWxSD);
#endif
				m_hostname = m_addr = m_port = wxEmptyString;
#if defined(_zerocsd_)
				wxLogDebug(_T("wxServDisc %p  (301) ip Not Found: Timed out:  ip addr:  %s"), m_pWxSD, m_addr.c_str());
#endif
				return;
			}
			else  // We have SUCCESS! -- Now access addrscan()'s getResults to get the ip address
			{
				bSuccessful = TRUE; // BEW added 21Apr16
				m_addr = addrscan.getResults().at(0).ip; // Beier's original
				m_port = wxString() << addrscan.getResults().at(0).port; // Beier's original, it leaks (I fixed with clearResults())					

				// BEW 6Apr16, make composite:  <ipaddr>@@@<hostname> to pass back to CServiceDiscovery instance
				wxString composite = m_addr;
				wxString ats = _T("@@@");
				composite += ats + m_hostname;

#if defined(_minimalsd_)
				wxLogDebug(_T("wxServDisc %p  (316) Made composite:  %s  FROM PARTS:  %s    %s    %s"),
					m_pWxSD, composite.c_str(), m_addr.c_str(), ats.c_str(), m_hostname.c_str());
#endif
				// BEW 28Apr16 Handle the possibility that this session has found an ipaddr which
				// has a different hostname than what the m_ipAddrs_Hostnames wxArrayString has
				// in one of its entries (typically one put there from the project config file)
				kbsvr_arrays.Lock();
				bool bReplacedOne = UpdateExistingAppCompositeStr(m_addr, m_hostname, composite);
				kbsvr_arrays.Unlock();
				
				if (!bReplacedOne)
				{
					kbsvr_arrays.Lock();
					// Only add this ip address to m_uniqueIpAddresses array if it is not already in the array
					bool bItIsUnique = AddUniqueStrCase(&m_ipAddrs_Hostnames, composite, TRUE); // does .Add() if it is unique
					kbsvr_arrays.Unlock();
					wxUnusedVar(bItIsUnique);

#if defined(_minimalsd_)
					if (bItIsUnique) // tell me so
					{
						kbsvr_arrays.Lock();
						wxLogDebug(_T("wxServDisc %p  (330)  onSDNotify()  SUCCESS, composite  %s  was stored in CServiceDiscovery"),
							m_pWxSD, composite.c_str());
						kbsvr_arrays.Unlock();
					} // end of TRUE block for test: if (bItIsUnique)
					else
					{
						wxLogDebug(_T("wxServDisc %p  (336)  onSDNotify()  SUCCESS, but NOT UNIQUE so not stored"));
					}
#endif
					// Try cleanup here for m_addr and m_port, these get leaked in Beier's solution
					m_addr.clear();
					m_port.clear();

					// Transfer the data to the app's array m_ipAddrs_Hostnames
					if (!m_ipAddrs_Hostnames.empty())
					{
						size_t count = m_ipAddrs_Hostnames.size();
						size_t index;
						for (index = 0; index < count; index++)
						{
							wxString composite = m_ipAddrs_Hostnames.Item(index);
							// Only transfer ones not already in the app's array of same name
							int result = m_pApp->m_ipAddrs_Hostnames.Index(composite);
							if (result == wxNOT_FOUND)
							{
								// Do the transfer, the app's array does not have this one yet
								m_pApp->m_ipAddrs_Hostnames.Add(composite);
								// Log what got sent, in Unicode Debug build, to check that it doesn't keep getting just the same one
#if defined(_tracking_transfers_)
								wxLogDebug(_T("In onSDNotify(), TRANSFERRING a composite ipaddr/hostname string to app array: %s"),
									composite.c_str());
#endif
							}
							else
							{
#if defined(_tracking_transfers_)
								// This one is already present, so bin it
								wxLogDebug(_T("In onSDNotify(), WITHHOLDING duplicate ipaddr/hostname string from app array: %s"),
									composite.c_str());
#endif
								index = index; // a do-nothing statement to avoid any compiler warning
							}
							// The local arrays here are no longer needed until the next timer 
							// notification, so clear them
							m_ipAddrs_Hostnames.clear();
							m_sd_servicenames.clear();
						}
					}
#if defined(_zerocsd_)
					wxLogDebug(wxT("wxServDisc %p (379) onSDNotify(), Success block ending. onSDNotify() exits soon"), m_pWxSD);
#endif
				} // end of TRUE block: if (!bReplacedOne)
				else
				{
#if defined(_tracking_transfers_)
					wxLogDebug(_T("In onSDNotify(), REPLACING a composite ipaddr/hostname string in app's array, with this one: %s"),
						composite.c_str());
#endif
					m_addr.clear();
					m_hostname.clear();
					m_ipAddrs_Hostnames.clear();
					m_sd_servicenames.clear();
				}
			} // end of SUCCESS block (else block) for test: if (timeout <= 0) for addrscan attempt

		} // end of else block for test: if(timeout <= 0) for namescan() attempt

	} // end of loop: for (i = 0; i < entry_count; i++)

	if (bSuccessful)
	{
#if defined(_zerocsd_)
		wxLogDebug(_T("CServiceDiscovery:onSDNotify() %p  (390)  m_bDestroyChildren set TRUE"), this);
#endif
		// BEW 22Apr16 signal that the all can be shutdown; the wxServDisc instances can die, and this & 
		// it's owning thread must be destroyed (after suitable small delays possibly, and the wxServDisc
		// instances undergo staged deaths, first namescan, then addrscan, and finally the owned instance
		// - a short delay between each of these will ensure we avoid access violations, the last is most
		// critical)
		m_bDestroyChildren = TRUE;
#if defined(_shutdown_)
		wxLogDebug(_T("CServiceDiscovery = %p  (399)  onSDNotify()  m_bDestroyChildren now set to TRUE"), this);
#endif
	}
}

// The following handler is only called when no running KBservers were discovered on the LAN
void CServiceDiscovery::onSDHalting(wxCommandEvent& WXUNUSED(event))
{
	// The following line gets OnCustomEventEndServiceDiscovery() in CMainFrame to prematurely
	// shut down a burst of service discovery runs after the first yielded no discoveries
	m_pApp->m_bServDiscRunFoundNothing = TRUE;

	// The following, even though a run with no discoveries does not call post_notify() and
	// hence no child wxServDisc instances (namescan() and addrscan()) are created, setting
	// this boolean to true causes the thread's TestDestroy() function to return TRUE. That
	// in turn gets CServiceDiscovery and the thread shut down (and the serviceStr cleared)
	m_bDestroyChildren = TRUE;
}

CServiceDiscovery::~CServiceDiscovery()
{
	// Logging shows that ~wxServDisc() for the owned instance does not get called, so
	// destroy it from here now (its results hashmap has already been cleared, & its 
	// cache structs etc). Note, can't do this from Thread_ServiceDiscovery::Entry()
	// because the latter sees the #include of Adapt_It.h, and Destorying from
	// Entry() would require #include "wxServDisc" which would reopen the can of worms
	// of hundreds of winsock etc name clashes
	m_pWxSD->GetThread()->Delete();
	delete m_pWxSD;

	// don't need these here (but I'll keep them as insurance)
	m_sd_servicenames.clear();
	m_ipAddrs_Hostnames.clear();

#if defined (_shutdown_)
	processID = wxGetProcessId();
	wxLogDebug(_T("CServiceDiscovery* = %p  (421)  Exiting from  ~CServiceDiscovery() destructor.  Process ID = %lx"), this, processID);
#endif
}

// The next function is necessary because the app storage for the composite string may have a different
// hostname than the one just discovered, but the same ipaddress. If this is the case, we want to detect
// the fact, and remove the one from the app storage, and replace with the one we pass in here - and then
// in onSDNotify() neither TRANSFER nor WITHHOLD, as we are in effect just updating. (The one in the app
// storage typically has come from an earlier session, via the project config file, when entering the
// project to do more work in the current session.)
//
// BEW 16May16, a further loop needs to be added, to cope with the following scenario. I had used the login
// from the KB Sharing Manager ( it would have also have worked the same if I'd logging in using the Setup Or
// Remove K B Sharing dlg) to connect to https://kbserver.jmarsden.org (in California, I'm in Melbourne Australia)
// and having got a connection, my code puts the url I manually typed into the app's m_ipAddrs_Hostnames array
// as https://kbserver.jmarsden.org with hostname "unknown" following after a number of spaces. All good. Then
// I had a KBserver also running on a laptop on my LAN, so I did a service discovery for that, it was found and
// had the url https://192.168.2.9 with hostname kbserver.local. -- again, all good. Wireless connections. But
// when, at the end of service discovery the wxMessageBox showed the results, there were three rather than two
// lines of entry. They were as follows:
// https://kbserver.jmarsden.org      unknown
// https://kbserver.jmarsden.org
// https://192.168.2.9           kbserver.local.
// Somehow, a spurious repeat of the top entry got "found" (it didn't appear to be as the result of a lookup,
// but only appears when I already had an existing connection to https://kbserver.jmarsden.org that was active.
// So my second loop has to detect such a spurious entry and remove it. It would appear that I just need to match
// the ipaddr part exactly, and remove the spurious entry before a spurious extra url can be produced
bool CServiceDiscovery::UpdateExistingAppCompositeStr(wxString& ipaddr, wxString& hostname, wxString& composite)
{
	int count = (int)m_pApp->m_ipAddrs_Hostnames.GetCount();
	bool bMadeAChange = FALSE;
	if (count == 0)
	{
		// There is no issue - no strings in the app storage yet, so return FALSE
		return FALSE;
	}
	else
	{
		int i;
		for (i = 0; i < count; i++)
		{
			wxString aFarComposite = m_pApp->m_ipAddrs_Hostnames.Item(i);
			// Check if it has the same ip address as was just discovered, if so we want
			// to update by removing aFarComposite and adding composite (order change in
			// the array does not matter) providing the passed in hostname cannot be
			// found in aFarComposite
			int offset = aFarComposite.Find(ipaddr);
			if (offset != wxNOT_FOUND)
			{
				// There is a match for the ip address, so check further
				offset = aFarComposite.Find(hostname);
				if (offset == wxNOT_FOUND && !hostname.IsEmpty())
				{
					// The one in the app storage has a different and non-empty hostname
					// than what was passed in here, so update the app storage to have
					// the more uptodate hostname
					m_pApp->m_ipAddrs_Hostnames.RemoveAt(i);
					m_pApp->m_ipAddrs_Hostnames.Add(composite);
					// We need to fix the m_strKbServerHostname member's value too,
					// otherwise the problem will persist due to the wrong value
					// being retained in the project config file
					m_pApp->m_strKbServerHostname = hostname; // job done
					// This hostname problem, because the composite strings in the app
					// storage are unique, can only occur in one such string. If we've
					// just fixed one, we've fixed the only possible one with this issue,
					// so return TRUE
					bMadeAChange = TRUE;
				}
				else
				{
					// The hostname passed in is also in the app's existing entry, so
					// we've got the same ipaddr and same hostname, so don't change
					// anything - don't do any replacement, but return TRUE as if we
					// had done so (so no change gets done to the app's entry)
					// OR
					// The hostname passed in was empty, so nothing is to be gained
					// by any replacement.
					// In either case, return TRUE because then the present values
					// passed in here will produce no change in the entry the app
					// already is storing
					bMadeAChange = TRUE;
				}
			}
		}
	}
	if (bMadeAChange)
	{
		return TRUE; // one or more replacements done, or, 
					 // some passed in values needed not to be used
	}
	return FALSE; // no replacement done to any of them
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
