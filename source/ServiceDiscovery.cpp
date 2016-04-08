
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
#include "ServiceDiscovery.h"
#include "helpers.h"

// to enable or suppress logging, comment out to suppress
#define _zerocsd_
// this one, if defined, displays each <url>@@@<hostname> string sent to the app, but a 
// WITHHOLDING message if it is a duplicate. Comment out to suppress displaying that info
#define _tracking_transfers_


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
CServiceDiscovery* gpServiceDiscovery; // a global for communicating with CServiceDiscovery instance

BEGIN_EVENT_TABLE(CServiceDiscovery, wxEvtHandler)
EVT_COMMAND(wxID_ANY, wxServDiscNOTIFY, CServiceDiscovery::onSDNotify)
EVT_COMMAND (wxID_ANY, wxServDiscHALTING, CServiceDiscovery::onSDHalting)
END_EVENT_TABLE()

CServiceDiscovery::CServiceDiscovery()
{
	m_servicestring.clear();
	wxUnusedVar(m_servicestring);
}

CServiceDiscovery::CServiceDiscovery(CAdapt_ItApp* pParentClass)
{
	gpServiceDiscovery = this; // wxServDisc creator needs this; set it early, so that it has
							   // it's correct value before wxServDisc is instantiated below
	m_serviceStr = _T("_kbserver._tcp.local.");
	m_pApp = pParentClass;
#if defined(_zerocsd_)
	wxLogDebug(_T("\nInstantiating a CServiceDiscovery class, for servicestring: %s, ptr to instance: %p"),
		m_serviceStr.c_str(), this);
#endif
	m_servicestring = m_serviceStr; // service to be scanned for
	m_pParent = pParentClass; 

	m_postNotifyCount = 0; // use this int as a filter to allow only one onSDNotify() call
	nDestructorCallCount = 0; // bump by one in each ~wxServDisc() when called, use in Entry() to test for >= 2

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
	m_pWxSD = new wxServDisc(this, m_servicestring, QTYPE_PTR);
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
// over the life of the app. We also need to remove (programmatically) any that users
// turn off during the life of the app - but that is independent of this discovery module.
// This service discovery module will just be instantiated, scan for _kbserver._tcp.local.
// lookup the ip addresses, deposit finished URL or URLs in the wxArrayString in the
// CAdapt_ItApp::m_servDiscResults array, and then kill itself; but periodically for the
// life of the app session. Splitting the discovery functionality from the gui was needed

void CServiceDiscovery::onSDNotify(wxCommandEvent& WXUNUSED(event))
{
	size_t i;

	m_hostname.Empty();
	m_addr.Empty();
	m_port.Empty();
	size_t entry_count = 0;
	int timeout;
	
	// length of query plus leading dot
	size_t qlen = m_pWxSD->getQuery().Len() + 1;
#if defined(_zerocsd_)
	wxString theQuery = m_pWxSD->getQuery(); // BEW, to see what's in it
	wxLogDebug(_T("theQuery:  %s  Its Length plus 1: %d"), theQuery.c_str(), (int)qlen);
#endif
	vector<wxSDEntry> entries = m_pWxSD->getResults();
	vector<wxSDEntry>::const_iterator it;
	for (it = entries.begin(); it != entries.end(); it++)
	{
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
		// what's really wanted - just the bit before the first ._ sequence
		entry_count++;
#if defined(_zerocsd_)
		wxString astring = it->name.Mid(0, it->name.Len() - qlen);
		wxLogDebug(_T("m_sd_servicenames receives servicename:  %s   for entry index = %d"),
			astring.c_str(), entry_count - 1);
#endif
		kbsvr_arrays.Lock();
		m_sd_servicenames.Add(it->name.Mid(0, it->name.Len() - qlen));
		kbsvr_arrays.Unlock();

	} // end of loop: for (it = entries.begin(); it != entries.end(); it++)

	// BEW: Next, lookup hostname and port -  the for loop is Beier's (I think)
#if defined(_zerocsd_)
	wxLogDebug(_T("onSDNotify() (224) wxServDisc  %p  &  entry_count =  %d"), m_pWxSD, entry_count);
#endif
	for (i = 0; i < entry_count; i++)
	{
		wxServDisc namescan(0, m_pWxSD->getResults().at(i).name, QTYPE_SRV); // Beier's original
					// 3rd param = 33, note namescan() is local, so runs as a detached thread 
					// from onSDNotify()'s stackframe
#if defined(_zerocsd_)
		wxLogDebug(_T("onSDNotify() (232) wxServDisc %p  &  passing into namescan's first param: %p :  for loop starts"), m_pWxSD, 0L);
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
			wxLogDebug(_T("onSDNotify() (246) wxServDisc  %p  &  parent %p:  namescan() Timed out:  m_hostname:  %s   m_port  %s"),
					m_pWxSD, this, m_hostname.c_str(), m_port.c_str());
#endif
			return;
		}
		else
		{
			// The namescan found something...
			m_hostname = namescan.getResults().at(0).name;
			m_port = wxString() << namescan.getResults().at(0).port;
#if defined(_zerocsd_)
			wxLogDebug(_T("wxServDisc %p (257)  onSDNotify:  Found Something"), m_pWxSD);
#endif
			// BEW For each successful namescan(), we must do an addrscan, so as to fill
			// out the full info needed for constructing a URL later on
			wxServDisc addrscan(0, m_hostname, QTYPE_A); // Beier's original
					// QTYPE_A is 1, note addrscan() is local, so runs as a detached thread 
					// from onSDNotify()'s stackframe
#if defined(_zerocsd_)
			wxLogDebug(_T("wxServDisc %p (265) onSDNotify() Passing into addrscan's first param: %p"), m_pWxSD, 0L);
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
				wxLogError(_T("wxServDisc %p (276) onSDNotify()  Timeout looking up IP address."), m_pWxSD);
#endif
				m_hostname = m_addr = m_port = wxEmptyString;
#if defined(_zerocsd_)
				wxLogDebug(_T("wxServDisc %p  (280) ip Not Found: Timed out:  ip addr:  %s"), m_pWxSD, m_addr.c_str());
#endif
				return;
			}
			else  // We have SUCCESS! -- Now access addrscan()'s getResults to get the ip address
			{
				// Didn't time out...
				m_addr = addrscan.getResults().at(0).ip; // Beier's original
				m_port = wxString() << addrscan.getResults().at(0).port; // Beier's original, it leaks (I fixed with clearResults())					

				// BEW 6Apr16, make composite:  <ipaddr>@@@<hostname> to pass back to CServiceDiscovery instance
				wxString composite = m_addr;
				wxString ats = _T("@@@");
				composite += ats + m_hostname;
#if defined(_zerocsd_)
				wxLogDebug(_T("wxServDisc %p  (295) Made composite:  %s  FROM PARTS:  %s    %s    %s"), 
					m_pWxSD, composite.c_str(), m_addr.c_str(), ats.c_str(), m_hostname.c_str());
#endif

				kbsvr_arrays.Lock();
				// Only add this ip address to m_uniqueIpAddresses array if it is not already in the array
				bool bItIsUnique = AddUniqueStrCase(&m_ipAddrs_Hostnames, composite, TRUE); // does .Add() if it is unique
				kbsvr_arrays.Unlock();
				wxUnusedVar(bItIsUnique);

#if defined(_zerocsd_)
				if (bItIsUnique) // tell me so
				{
					kbsvr_arrays.Lock();
					wxLogDebug(_T("wxServDisc %p  (309)  onSDNotify()  SUCCESS, composite  %s  was stored in CServiceDiscovery"),
						m_pWxSD, composite.c_str());
					kbsvr_arrays.Unlock();
				} // end of TRUE block for test: if (bItIsUnique)
#endif

				// Try cleanup here for m_addr and m_port, these get leaked in Beier's solution
				m_addr.clear();
				m_port.clear();
			} // end of SUCCESS block (else block) for test: if (timeout <= 0) for addrscan attempt

		} // end of else block for test: if(timeout <= 0) for namescan() attempt


	} // end of loop: for (i = 0; i < entry_count; i++)
#if defined(_zerocsd_)
	wxLogDebug(wxT("wxServDisc %p (325) onSDNotify(), One KBserver was found. onSDNotify() is exiting now"), m_pWxSD);
#endif
}

// BEW 8Apr18 We need this handler in order to delete the original wxServDisc instance
void CServiceDiscovery::onSDHalting(wxCommandEvent& event)
{
	wxUnusedVar(event);

	// BEW made this handler, for shutting down the module when no KBserver
	// service was discovered, and for when one was discovered and post_notify()
	// posted a wxServDiscNOTIFY event to get the lookup and reporting done for
	// the discovered service. (Beier's original solution can discover more than
	// one running KBserver in a single discovery session, but at the cost of a
	// lot of time, copious amounts of memory leaks, and choking the CPU to death.)
#if defined(_zerocsd_)
	wxLogDebug(_T("In CServiceDiscovery:onSDHalting() m_pWxSD =  %p  will be deleted now"), m_pWxSD);
#endif
	m_pWxSD->clearResults();

	delete m_pWxSD; // must have this, it gets ~wxServDisc() destructor called

#if defined(_zerocsd_)
	wxLogDebug(_T("wxServDisc %p:  [from CServiceDiscovery:onSDHalting()] AFTER posting wxServDiscHALTING event, this = %p, m_pParent (the app) = %p"),
				m_pWxSD, this, m_pParent);
#endif
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
				wxLogDebug(_T("In onSDHalting(), TRANSFERRING a composite ipaddr/hostname string to app array: %s"),
					composite.c_str());
#endif
			}
			else
			{
#if defined(_tracking_transfers_)
				// This one is already present, so bin it
				wxLogDebug(_T("In onSDHalting(), WITHHOLDING duplicate ipaddr/hostname string from app array: %s"),
					composite.c_str());
#endif
				wxNO_OP; // for release build
			}
			// The arrays here are no longer needed until the next timer Notify(), so clear them
			m_ipAddrs_Hostnames.clear();
			m_sd_servicenames.clear();
		}
	}
}


CServiceDiscovery::~CServiceDiscovery()
{
	m_sd_servicenames.clear();
	m_ipAddrs_Hostnames.clear();
	m_serviceStr.clear();
#if defined(_zerocsd_)
	wxLogDebug(_T("CServiceDiscovery* = %p  Deleting the CServiceDiscovery instance = %p, in ~CServiceDiscovery() and clearing its member arrays"), this);
#endif
}

// Copied wxItoa from helpers.cpp, as including helpers.h leads to problems   BEW 6Apr16, is it still needed here?  - check later
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
