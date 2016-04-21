

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

DECLARE_EVENT_TYPE(wxEVT_End_Namescan, -1)
DECLARE_EVENT_TYPE(wxEVT_End_Addrscan, -1)

class CServiceDiscovery : public wxEvtHandler
{

public:
	CServiceDiscovery();
	CServiceDiscovery(CAdapt_ItApp* pParentClass);
	virtual ~CServiceDiscovery();

	CAdapt_ItApp* m_pApp;

	wxServDisc* m_pWxSD; // main service scanner (a child class of this one)
	CAdapt_ItApp* m_pParent; // BEW 4Jan16

	int	m_postNotifyCount;  // count the number of Post_Notify() calls, we use it to 
							// limit one discovery run to finding one running KBserver
							// at random from however many are currently running
	unsigned long processID;

	// scratch variables, used in the loop in onSDNotify() handler
	wxString m_hostname;
	wxString m_addr;
	wxString m_port;

	wxString m_serviceStr;

	void	 ShutdownWxServDisc(wxServDisc* pOwnedWxServDisc); // call this when shutting down 
															   // the service discovery module

	// The next two booleans are defaulted to FALSE when CServiceDiscovery is instantiated,
	// and to avoid access violations, we have to shut down namescan() and addrscan() 
	// local instances of wxServDisc owned by the instance CServiceDiscovery instance,
	// before we try delete CServiceDiscovery and the Thread_ServiceDiscovery(). These
	// two booleans must become TRUE before the Thread_ServiceDiscovery::TestDestroy()
	// function can return a TRUE value to shut down the thread. Designing this way was
	// necessitated by the fact that the (child) namescan() and addrscan() instances
	// continued processing after the Thread_ServiceDiscovery() and CServiceDiscovery
	// instance and its owned wxServDisc instance were all deleted - resulting in an
	// eventual access violation after what appeared to be successful service discovery
	// timed runs over a few minutes without any problem. The solution is to kill the
	// kids before destroying their parent. Harsh, but necessary.
	bool     m_bNamescanIsEnded;
	bool	 m_bAddrscanIsEnded;

	bool IsDuplicateStrCase(wxArrayString* pArrayStr, wxString& str, bool bCase); // BEW created 5Jan16
	bool AddUniqueStrCase(wxArrayString* pArrayStr, wxString& str, bool bCase); // BEW created 5Jan16

	// These arrays receive results, which will get passed back to app's DoServiceDiscovery() etc.
	wxArrayString m_sd_servicenames;   // for servicenames, as discovered from query (these are NOT hostnames)

	wxArrayString m_ipAddrs_Hostnames; // stores unique set of <ipaddress>@@@<hostname> composite strings

protected:
	  void onSDNotify(wxCommandEvent& WXUNUSED(event));
	  void onSDHalting(wxCommandEvent& event); // needed to get the last wxServDisc deleted from here
	  void onEndNamescan(wxCommandEvent& WXUNUSED(event));
	  void onEndAddrscan(wxCommandEvent& WXUNUSED(event));
private:

	DECLARE_EVENT_TABLE();
};

#endif // _KBSERVER

#endif // SERVICEDISCOVERY_h

