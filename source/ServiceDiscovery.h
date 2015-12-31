

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
/////////////////////////////////////////////////////////////////////////////

#ifndef SERVICEDISCOVERY_h
#define SERVICEDISCOVERY_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ServiceDiscovery.h"
#endif

#if defined(_KBSERVER)

class ServDisc;
class wxServDisc;
class wxThread;

class CServiceDiscovery : public wxEvtHandler
{

public:
	CServiceDiscovery();
	CServiceDiscovery(wxString servicestring, ServDisc* pParentClass);
	virtual ~CServiceDiscovery();

	wxString m_servicestring; // service to be scanned for

	wxServDisc* m_pSD; // main service scanner (a child class of this one)
	ServDisc* m_pParent;
	bool m_bWxServDiscIsRunning; // I'll use a FALSE value of this set in onSDHalting
								 // informing CServiceDiscovery instance that we are done
	// scratch variables, used in the loop in onSDNotify() handler
	wxString m_hostname;
	wxString m_addr;
	wxString m_port;

	wxArrayString m_sd_servicenames; // for servicenames, as discovered
	// The follow int arrays are for storing booleans, 1 for TRUE, 0 for FALSE
	// in parallel with the URLs (or empty strings) in m_urlsArr
	wxArrayInt m_bArr_ScanFoundNoKBserver;
	wxArrayInt m_bArr_HostnameLookupFailed;
	wxArrayInt m_bArr_IPaddrLookupFailed;
	// Flags are 1 (true), 0 (false), -1 (undefined)
	// put out constructed lines here: each is  url:0:0:0 (the failure flags are
	// each zero if a url is constructed), or if no KBserver was discovered:  :1:-1:-1,
	// or if some other error,  :0:1:-1 or :0:0:-1
	wxArrayString m_sd_lines;
	wxArrayString m_urlsArr;
    bool m_bOnSDNotifyEnded;
	bool m_bOnSDNotifyStarted;

	void wxItoa(int val, wxString& str); // copied from helpers.h & .cpp, it creates problems to #include "helpers.h"

    // bools (as int 0 or 1, in int arrays) for error conditions are on the CMainFrame
    // instance and the array of URLs for the one or more _kbserver._tcp.local. services
    // that are discovered is there also. It is NAUGHTY to have two or more KBservers
    // running on the LAN at once, but we can't prevent someone from doing so - when that
    // happens, we'll need to let them choose which URL to connect to. The logic for all
    // that will be in a function called from elsewhere, the function will make use of the
    // data we sent to the CMainFrame class from here. (See MainFrm.h approx lines 262-7)
protected:
	void onSDNotify(wxCommandEvent& event);
	void onSDHalting(wxCommandEvent& event);
 
private:
	DECLARE_EVENT_TABLE();

    //DECLARE_DYNAMIC_CLASS(CServiceDiscovery)
};

#endif // SERVICEDISCOVERY_h

#endif // _KBSERVER

