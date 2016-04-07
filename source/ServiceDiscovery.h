

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
	CServiceDiscovery(CAdapt_ItApp* pParentClass);
	virtual ~CServiceDiscovery();

	wxString m_servicestring; // service to be scanned for
	CAdapt_ItApp* m_pApp;

	wxServDisc* m_pWxSD; // main service scanner (a child class of this one)
	CAdapt_ItApp* m_pParent; // BEW 4Jan16
	//bool m_bWxServDiscIsRunning; // I'll use a FALSE value of this set in onSDHalting <<-- BEW 6Apr16 deprecated
								 // informing CServiceDiscovery instance that we are done

	int	m_postNotifyCount;  // count the number of Post_Notify() call attempts - a debug logging aid or more

	// Try providing named wxStrings to be used instead of Beier's anonymous ones (the latter can't be freed
	// at heap cleanup time; but if named we can do so)
	//wxString m_addrscan_ipStr;

	// scratch variables, used in the loop in onSDNotify() handler
	wxString m_hostname;
	wxString m_addr;
	wxString m_port;

	bool IsDuplicateStrCase(wxArrayString* pArrayStr, wxString& str, bool bCase); // BEW created 5Jan16
	bool AddUniqueStrCase(wxArrayString* pArrayStr, wxString& str, bool bCase); // BEW created 5Jan16
	int nPostedEventCount; // <<-- deprecate later, but check first, I think this is no longer of any value, I use m_postNotifyCount instead now
	int nDestructorCallCount; // how many times the ~wxServDisc() destructor gets called -
			// once it gets to 2 (ie. both namescan() and addrscan() have worked and died, we
			// can cause the original (owned) wxServDisc to shut down - the logging shows that
			// using GC value to cause 'expire all', bypasses my_gc(d) call - which may account
			// for why there are unexpected results. My idea is to provide tests in Entry()'s
			// while loop, which when true, will break from the loop with exit set to true -
			// and that should get the cleanup code done for the original wxServDisc

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
	bool m_bServiceDiscoveryCanFinish; // BEW 6Apr16  <<-- deprecate later

	int nCleanupCount; // count and log number of entries into CleanUpSD()


	// These arrays receive results, which will get passed back to app's DoServiceDiscovery() etc.
	wxArrayString m_sd_servicenames;   // for servicenames, as discovered from query (these are NOT hostnames)

	wxArrayString m_ipAddrs_Hostnames; // stores unique set of <ipaddress>@@@<hostname> composite strings

	void wxItoa(int val, wxString& str); // copied from helpers.h & .cpp, it creates  <<-- deprecate ?? check later, BEW 6Apr16
										 // name conflict problems to #include "helpers.h"
protected:
	  void onSDNotify(wxCommandEvent& WXUNUSED(event));
	  void onSDHalting(wxCommandEvent& event); // needed to get the last wxServDisc deleted from here
private:
	wxString m_serviceStr;

	DECLARE_EVENT_TABLE();
};

#endif // _KBSERVER

#endif // SERVICEDISCOVERY_h

