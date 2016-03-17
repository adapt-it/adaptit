
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

extern CAdapt_ItApp* gpApp;
CServiceDiscovery* gpServiceDiscovery; // wxServDisc's creator needs the value we place here

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

	// Initialize my reporting context (Use .Clear() rather than
	// .Empty() because from one run to another we don't know if the
	// number of items discovered will be the same as discovered previously
	// Note: if no KBserver service is discovered, these will remain cleared.
	m_sd_servicenames.Clear();
	m_uniqueIpAddresses.Clear();
	m_urlsArr.Clear();
	m_theirHostnames.Clear();
	/*
	m_sd_lines.Clear(); //for finished  string:flag:flag:flag lines, string can be empty
	m_bArr_ScanFoundNoKBserver.Clear();  // stores 0, 1 or -1 per item
	m_bArr_HostnameLookupFailed.Clear(); // ditto
	m_bArr_IPaddrLookupFailed.Clear();   // ditto
	m_bArr_DuplicateIPaddr.Clear();      // ditto
	*/

    // wxServDisc creator is: wxServDisc::wxServDisc(void* p, const wxString& what, int
    // type) where p is pointer to the parent class & what is the service to scan for, its
    // type is QTYPE_PTR
	m_pWxSD = new wxServDisc(this, m_servicestring, QTYPE_PTR);
	wxUnusedVar(m_pWxSD);
	wxLogDebug(_T("wxServDisc %p: just now instantiated in CServiceDiscovery creator"), m_pWxSD);

	gpServiceDiscovery = this; // wxServDisc creator needs this
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

//void CServiceDiscovery::GetResults()
//{
//}

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
	wxLogDebug(_T("wxServDisc %p:  this [from CServiceDiscovery:onSDHalting()] = %p"), m_pWxSD, this);
	wxLogDebug(_T("wxServDisc %p:  m_pParent [from CServiceDiscovery:onSDHalting()] = %p"), m_pWxSD, m_pParent);

	m_bWxServDiscIsRunning = FALSE;
	wxLogDebug(_T("wxServDisc %p:  Starting CServiceDiscovery:onSDHalting(): m_bWxServDiscIsRunning initialized to FALSE"),
				m_pWxSD);

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
			m_pWxSD, this, m_pParent);

	delete m_pWxSD; // must have this, it gets ~wxServDisc() destructor called
}

CServiceDiscovery::~CServiceDiscovery()
{
	wxLogDebug(_T("Copying URLs to app::m_servDiscResults array, then Deleting the CServiceDiscovery instance = %p, in ~CServiceDiscovery()"), this);
}

// Copied wxItoa from helpers.cpp, as including helpers.h leads to problems
void CServiceDiscovery::wxItoa(int val, wxString& str)
{
	wxString valStr;
	valStr << val;
	str = valStr;
}

#endif // _KBSERVER
