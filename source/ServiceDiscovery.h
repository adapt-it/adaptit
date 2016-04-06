

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
/// because our mutex & condition solution uses .WaitTimeout() in the app's
/// DoServiceDiscovery() function, and when waiting, main thread event handling
/// is asleep - so if you tried to do things Beier's way, the essential
/// address lookup etc would not get called until the main thread's sleep ended -
/// which would be too late because DoServiceDiscovery() would have been exited
/// before the service discovery results could be computed.
/////////////////////////////////////////////////////////////////////////////

#ifndef SERVICEDISCOVERY_h
#define SERVICEDISCOVERY_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ServiceDiscovery.h"
#endif

#if defined(_KBSERVER)

class wxServDisc;
class wxThread;
class CAdapt_ItApp; // BEW 4Jan16 

class CServiceDiscovery : public wxEvtHandler
{

public:
	CServiceDiscovery();
	CServiceDiscovery(wxString servicestring, CAdapt_ItApp* pParentClass);
	virtual ~CServiceDiscovery();

	wxString m_servicestring; // service to be scanned for
	CAdapt_ItApp* m_pApp;

	wxServDisc* m_pWxSD; // main service scanner (a child class of this one)
	CAdapt_ItApp* m_pParent; // BEW 4Jan16
	bool m_bWxServDiscIsRunning; // I'll use a FALSE value of this set in onSDHalting
								 // informing CServiceDiscovery instance that we are done
	int	m_postNotifyCount;  // count the number of Post_Notify() call attempts
            // and only allow the function to be actually called when the count value is
            // zero -- this is to prevent multiple accesses to the
            // CServiceDiscovery::GetResults() code - it appears that this can otherwise be
            // running after the CServiceDiscovery instance has been deleted which could
            // lead to a crash

	// scratch variables, used in the loop in onSDNotify() handler
	wxString m_hostname;
	wxString m_addr;
	wxString m_port;

	// onSDNotify() takes longer to complete than the service discovery thread, so we can't
	// assume that initiating service discovery halting from the end of onSDNotify will
	// allow clean leak elimination - the latter crashes. So I'm creating two booleans,
	// each defaults to FALSE, and if onSDNotify() is called, both will be TRUE by the time
	// onSDNotify finishes. Then, in the shutdown code in Entry(), we have a waiting loop which
	// waits for a short interval and checks for the 2nd boolean TRUE, then it will know that
	// module completion and leak elimination can happen, safely we hope... let's see...
	// 
	// Beier, in ~wxServDisc() destructor, has GetThread()->Delete(). But his thread is not
	// a joinable one, it's detached, and as far as I can determine, and despite the wx
	// documentation saying otherwise, Delete() doesn't stop the wxServDisc's thread from
	// running and so it goes on until timeout happens and then cleanup. Because of this,
	// it can be running long after everything else has done their job and been cleaned up,
	// so it must not rely on classes above it being in existence when it nears its end.
	// The following flag is defaulted to FALSE, and if a KBserver is found, it is handled
	// by the calling CServiceDiscovery::onSDNotify() handler, and at the end of the latter
	// a wxServDiscHALTING event is posted to shut everything down; but wxServDisc::Entry()
	// has it's own code for posting that event to CServiceDiscovery instance. If the latter
	// has already been destroyed because onSDNotify has completed, and wxServDisc's thread
	// runs on for a while, when it gets to its wxPostEvent() call, the CServiceDiscovery
	// instances pointer has become null, and so there is an app crash(accessing null ptr).
	// To prevent this, we have onSDNotify(), when it starts to run, set to TRUE the
	// following boolean, and in wxServDisc::Entry() we use that TRUE value to cause the
	// posting of the wxServDiscHALTING event to be skipped; we just let cleanup happen and
	// the thread then destroys itself. wxServDisc::Entry() needs to retain the event posting
	// code, because when no KBserver is running on the LAN, then the only place the event
	// posting that gets the calling classes cleaned up is at the end of wxServDisc::Entry()
	bool m_bOnSDNotifyEnded;
	bool m_bOnSDNotifyStarted;

	bool IsDuplicateStrCase(wxArrayString* pArrayStr, wxString& str, bool bCase); // BEW created 5Jan16
	bool AddUniqueStrCase(wxArrayString* pArrayStr, wxString& str, bool bCase); // BEW created 5Jan16
	int nPostedEventCount;


	// BEW 23Mar16, To govern the service discovery shutdown mechanism, we need a boolean
	// here, m_bServiceDiscoveryCanFinish which will be set FALSE in the CServiceDiscovery's
	// creator, and set TRUE only after a predetermined service discovery run (whether
	// a foreground call of DoServiceDiscovery(), or a background thread call of the
	// backgrounded equivalent of the latter) is to be halted, and the heap freed.
	// In the case of a foreground run, the main thread re-awakening after the
	// wxWaitTimeout() has expired will generate the TRUE value which then permits
	// the owned wxServDisc instance to poll for this boolean being true, and when so,
	// the halting mechanism gets invoked; or in the case of a background thread, a
	// timer will fire and likewise set this boolean TRUE, and so initiate the halting
	// mechanism in that scenario.
	bool m_bServiceDiscoveryCanFinish;
	int nCleanupCount; // count and log number of entries into CleanUpSD()


	// These arrays receive results, which will get passed back to app's DoServiceDiscovery() etc.
	wxArrayString m_sd_servicenames;   // for servicenames, as discovered from query (these are NOT hostnames)
	wxArrayString m_uniqueIpAddresses; // for each 192.168.n.m  (we store unique ip addresses)
	wxArrayString m_theirHostnames;    // from the namescan() lookup
	wxArrayString m_urlsArr;

	void wxItoa(int val, wxString& str); // copied from helpers.h & .cpp, it creates 
										 // name conflict problems to #include "helpers.h"
protected:
	  void onSDNotify(wxCommandEvent& event);
	  void onSDHalting(wxCommandEvent& event); // we do cleanup by handlers of this type
					   // invoked by our posting of custom events at the right time
private:
	DECLARE_EVENT_TABLE();
};

#endif // _KBSERVER

#endif // SERVICEDISCOVERY_h

