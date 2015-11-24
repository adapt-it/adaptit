
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
#include <wx/docview.h>
#include "SourcePhrase.h" // needed for definition of SPList, which MainFrm.h uses
#include "MainFrm.h"
#endif
#include "wx/log.h"

#define _WINSOCKAPI_
#define WIN32_LEAN_AND_MEAN
#include "wxServDisc.h"
#include "ServDisc.h"
#include "ServiceDiscovery.h"


IMPLEMENT_DYNAMIC_CLASS(CServiceDiscovery, wxEvtHandler)

BEGIN_EVENT_TABLE(CServiceDiscovery, wxEvtHandler)
	EVT_COMMAND (wxID_ANY, wxServDiscNOTIFY, CServiceDiscovery::onSDNotify)
	EVT_COMMAND (wxID_ANY, wxServDiscHALTING, CServiceDiscovery::onSDHalting)
END_EVENT_TABLE()

CServiceDiscovery::CServiceDiscovery()
{
	m_servicestring = _T("");
	m_bWxServDiscIsRunning = TRUE;
	wxUnusedVar(m_servicestring);
}

CServiceDiscovery::CServiceDiscovery(CMainFrame* pFrame, wxString servicestring, ServDisc* pParentClass)
{

	wxLogDebug(_T("\nInstantiating a CServiceDiscovery class, passing in pFrame and servicestring: %s, ptr to instance: %p"),
		servicestring.c_str(), this);

	m_servicestring = servicestring; // service to be scanned for
	m_pFrame = pFrame; // Adapt It app's frame window
	m_pParent = pParentClass; // so we can delete the parent from within this class
	m_pParent->m_backup_ThisPtr = NULL; // initialize
	m_bWxServDiscIsRunning = TRUE; // Gets set FALSE only in my added onServDiscHalting() handler
		// and the OnIdle() hander will use the FALSE to get CServiceDiscovery and ServDisc
		// class instances deleted, and app's m_pServDisc pointer reset to NULL afterwards

	m_hostname = _T("");
	m_addr = _T("");
	m_port = _T("");

	// Clear the reporting member variables on the CMainFrame instance, as we are
	// doing a new discovery attempt
	m_pFrame->m_urlsArr.Clear();
	m_pFrame->m_bArr_ScanFoundNoKBserver.Clear();
	m_pFrame->m_bArr_HostnameLookupFailed.Clear();
	m_pFrame->m_bArr_IPaddrLookupFailed.Clear();

	// wxServDisc creator is: wxServDisc::wxServDisc(void* p, const wxString& what, int type)
	// where p is pointer to the parent class & what is the service to scan for, its type
	// is QTYPE_PTR
	// ********************** NOTE ******************************
	// The "parent" class for instantiating wxServDisc, is NOT the parent to
	// this CServiceDiscovery class instance! The latter is the pParentClass
	// passed in; but for wxServDisc, the p parameter is the destination object
	// to which the pending custom event (ie. wxServDiskNOTIFY event) is to be
	// sent -- and that is to this CServiceDiscovery instance. So pass in this
	m_pSD = new wxServDisc(this, m_servicestring, QTYPE_PTR);

	// Could posting serviceDiscoveryHALTING event here work? Test that control gets here...
	// It does, but this constructor then is exitted before the service discovery
	// gets a chance to do anything, so halting from here is too early
}

// When wxServDisc discovers a service (one or more of them) that it is scanning
// for, it reports its success here. Adapt It needs just the ip address(s), and
// maybe the port, so that we can present URL(s) to the AI code which it can use
// for automatic connection to the running KBserver. Ideally, only one KBserver
// runs at any given time, but we have to allow for users being perverse and running
// more than one. If they do, we'll have to let them make a choice of which URL to
// connect to. onSDNotify is an amalgamation of a few separate functions from the
// original sdwrap code. We don't have a gui, so all we need do is the minimal task
// of looking up the ip address and port of each discovered _kbserver service, and
// turning each such into an ip address from which we can construct the appropriate
// URL (one or more) which we'll make available to the rest of Adapt It via a
// wxArrayString plus some booleans for error states, on the CMainFrame class instance.
// The logic for using the one or more URLs will be constructed externally to this
// service discovery module. This service discovery module will just be instantiated,
// scan for _kbserver._tcp.local. , lookup the ip addresses, deposit finished URL or
// URLs in the wxArrayString in CMainFrame, and then kill itself. We'll probably
// run it at the entry to any Adapt It project, and those projects which are supporting
// KBserver syncing, will then make use of what has been returned, to make the connection
// as simple and automatic for the user as possible. Therefore, onSDNotify will do
// all this stuff, and at the end, kill the module, and it's parent classes.
void CServiceDiscovery::onSDNotify(wxCommandEvent& event)
{
	if (event.GetEventObject() == m_pSD)
	{
		 m_pSD->m_bOnSDNotifyStarted = TRUE;

		// initialize my reporting context (Use .Clear() rather than
		// .Empty() because from one run to another we don't know if the
		// number of items discovered will be the same as discovered previously
		m_sd_items.Clear();
		m_pFrame->m_urlsArr.Clear();
		m_pFrame->m_bArr_ScanFoundNoKBserver.Clear();
		m_pFrame->m_bArr_HostnameLookupFailed.Clear();
		m_pFrame->m_bArr_IPaddrLookupFailed.Clear();
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
		size_t qlen = m_pSD->getQuery().Len() + 1;
#if defined(_DEBUG)
		wxString theQuery = m_pSD->getQuery(); // temp, to see what's in it
		wxLogDebug(_T("theQuery:  %s  Its Length plus 1: %d"), theQuery.c_str(), (int)qlen);
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
			wxLogDebug(_T("name: %s  Len(): %d  ip: %s  port: %d  time: %d"),
				aName.c_str(), nameLen, ip.c_str(), port, time);
#endif
			// what's really wanted - just the bit before the first ._ sequence
			entry_count++;
#if defined(_DEBUG)
			wxString astring = it->name.Mid(0, it->name.Len() - qlen);
			wxLogDebug(_T("m_sd_items receives string:  %s   for entry index = %d"), 
				astring.c_str(), entry_count - 1);
#endif
			m_sd_items.Add(it->name.Mid(0, it->name.Len() - qlen));
			m_pFrame->m_bArr_ScanFoundNoKBserver.Add(0); // add FALSE, as we successfully
				// discovered one (but that does not necessarily mean we will
				// subsequently succeed at geting hostname, port, and ip address)

		} // end of loop: for (it = entries.begin(); it != entries.end(); it++)

		// Next, lookup hostname and port. Code for this is copied and tweaked,
		// from the list_select() event handler of MyFrameMain. Since we have
		// this now in a loop in case more than one KBserver is running, we
		// must then embed the associated addrscan() call within the loop
		size_t i;
		for (i = 0; i < entry_count; i++)
		{
			wxServDisc namescan(0, m_pSD->getResults().at(i).name, QTYPE_SRV);

			timeout = 3000;
			while(!namescan.getResultCount() && timeout > 0)
			{
				wxMilliSleep(25);
				timeout-=25;
			}
			if(timeout <= 0)
			{
				wxLogError(_("Timeout looking up hostname. Entry index: %d"), i);
				m_hostname = m_addr = m_port = wxEmptyString;
				m_pFrame->m_bArr_HostnameLookupFailed.Add(1); // adding TRUE
				m_pFrame->m_bArr_IPaddrLookupFailed.Add(-1); // because we won't try addrscan() for this index
#if defined(_DEBUG)
				wxLogDebug(_T("Found: [Service:  %s  ] Timeout:  m_hostname:  %s   m_port  %s   for entry index = %d"),
				m_sd_items.Item(i).c_str(), m_hostname.c_str(), m_port.c_str(), i);
#endif
				//return;
				continue; // don't return, we need to try every iteration
			}
			else
			{
				// The namescan found something...
				m_hostname = namescan.getResults().at(0).name;
				m_port = wxString() << namescan.getResults().at(0).port;
				m_pFrame->m_bArr_HostnameLookupFailed.Add(0); // adding FALSE
#if defined(_DEBUG)
				wxLogDebug(_T("Found: [Service:  %s  ] Looked up:  m_hostname:  %s   m_port  %s   for entry index = %d"),
				m_sd_items.Item(i).c_str(), m_hostname.c_str(), m_port.c_str(), i);
#endif
				// For each successful namescan(), we must do an addrscan, so as to fill
				// out the full info needed for constucting a URL; if the namescan was
				// not successful, the m_bArr_IPaddrLookupFailed entry for this index
				// should be neither true (1) or false (0), so use -1 for "no test was made"
				{
					wxServDisc addrscan(0, m_hostname, QTYPE_A);

					timeout = 3000;
					while(!addrscan.getResultCount() && timeout > 0)
					{
						wxMilliSleep(25);
						timeout-=25;
					}
					if(timeout <= 0)
					{
						wxLogError(_("Timeout looking up IP address."));
						m_hostname = m_addr = m_port = wxEmptyString;
						m_pFrame->m_bArr_IPaddrLookupFailed.Add(1); // for TRUE
#if defined(_DEBUG)
						wxLogDebug(_T("ip Not Found: [Service:  %s  ] Timeout:  ip addr:  %s   for entry index = %d"),
						m_sd_items.Item(i).c_str(), m_addr.c_str(), i);
#endif
						//return;
						continue; // do all iterations
					}
					else
					{
						// succeeded in getting the service's ip address
						m_addr = addrscan.getResults().at(0).ip;
						m_pFrame->m_bArr_IPaddrLookupFailed.Add(0); // for FALSE
#if defined(_DEBUG)
						wxLogDebug(_T("Found: [Service:  %s  ] Looked up:  ip addr:  %s   for entry index = %d"),
						m_sd_items.Item(i).c_str(), m_addr.c_str(), i);
#endif
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
			if (m_pFrame->m_bArr_HostnameLookupFailed.Item(i) == 0 &&
				m_pFrame->m_bArr_IPaddrLookupFailed.Item(i) == 0)
			{
				m_pFrame->m_urlsArr.Add(protocol + m_addr);
#if defined(_DEBUG)
				wxLogDebug(_T("Found: [Service:  %s  ] URL:  %s   for entry index = %d"),
						m_sd_items.Item(i).c_str(), (protocol + m_addr).c_str(), i);
#endif
			}
		} // end of loop: for (i = 0; i < entry_count; i++)

		m_pSD->m_bOnSDNotifyEnded = TRUE; // leak elimination and module shutdown can now happen

	} // end of TRUE block for test: if (event.GetEventObject() == m_pSD)
	else
	{
		// major error, so don't wipe out what an earlier run may have stored
		// in the CMainFrame instance - program counter has never entered here
		;
	} // end of else block for test: if (event.GetEventObject() == m_pSD)
	
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
    wxLogDebug(_T("BEW: onSDNotify, block finished. Now have posted event wxServDiscHALTING."));
	}

	wxLogDebug(wxT("BEW: A KBserver was found. onSDNotify() is exiting right now"));
}

// BEW Getting the module shut down in the two circumstances we need, is a can of worms.
// (a) after one or more KBserver instances running have been discovered (onServDiscNotify()
// gets called, and we can put shut-down code in the end of that handler - but I may need
// to improve on it some more)
// (b) when no KBserver instance is runnning (I use an onServDiscHalting() handler of my
// own to get ~wxServDisc() destructor called, but that's all it can do safely).
//
// In the (b) case, my CServiceDiscovery instance, and my ServDisc instance, live on.
// So I've resorted to a hack, using a m_bWxServDiscIsRunning boolean, set to FALSE
// when no KBserver was discovered, at end of the ...Halting() handler,
// and the OnIdle() handler in frame window, to get the latter two classes clobbered.
//
// SDWrap embedded the wxServDisc instance in the app, and so when the app got shut down,
// the lack of well-designed shutdown code didn't matter, as everything got blown away.
// But for me, ServDisc needs to exist only for a single try at finding a KBserver instance,
// and then die forever - or until explicitly instantiated again (which currently,
// requires a relaunch of AI)
void CServiceDiscovery::onSDHalting(wxCommandEvent& event)
{
	wxUnusedVar(event);

	// BEW made this handler, for shutting down the module when no KBserver
	// service was discovered
	wxLogDebug(_T("this [ from CServiceDiscovery:onSDHalting() ] = %p"), this);
	wxLogDebug(_T("m_pParent [ from CServiceDiscovery:onSDHalting() ] = %p"), m_pParent);

	// The this pointer is valid here, but gets lost by the time the parent
	// class's onServDiscHalting() handler is called (by lost I mean it has
	// the value 0xcdcdcdcd in the handler, instead of this class's correct
	// this ptr value. So I'll store a copy in the parent, and use that in
	// the handler to get the deletion of this class done.
	m_pParent->m_backup_ThisPtr = this;

	delete m_pSD; // must have this, it gets ~wxServDisc() destructor called

	// Check if the above deletion is the culprit for the loss of the this ptr value;
	// answer? No it's not the culprit. Ptr value is still the same here.
	wxLogDebug(_T("this [ from CServiceDiscovery:onSDHalting() ] AFTER delete m_pSD call = %p"), this);

	m_bWxServDiscIsRunning = FALSE;
	m_pParent->m_bSDIsRunning = FALSE; // enables our code in CMainFrame::OnIdle() to
		// figure out that no KBserver was found and so a partial removal of the
		// module has been done so far (ie. wxServDisc instance only), and so the
		// class ServDisc (the parent of the CServiceDiscovery instance) can be
		// deleted from within OnIdle() when this flag is FALSE, and the app's
		// m_pServDisc pointer reset to NULL. That doesn't however get the
		// CServiceDiscovery instance deleted, so it will leak memory unless we can
		// get it deleted from within itself. Trying to do it from elsewhere means
		// that #include wxServDisc has to be mixed with #include "Adapt_It.h" and
		// that leads to hundreds of name clashes.
		// Ugly hacks, but hey, I didn't write the wxServDisc code.

	// post a custom serviceDiscoveryHALTING event here, for the parent class to
	// supply the handler needed. Doing this here, as this makes the posting of
	// the event and the handler be as temporally close to each other as possible.
	// Nevertheless, the this pointer somehow gets clobbered (see above)
	wxCommandEvent upevent(serviceDiscoveryHALTING, wxID_ANY);
	upevent.SetEventObject(this); // set sender

	// BEW added this posting...  Send it
#if wxVERSION_NUMBER < 2900
	wxPostEvent((wxEvtHandler*)m_pParent, upevent);
#else
	wxQueueEvent((wxEvtHandler*)m_pParent, upevent.Clone());
#endif
	wxLogDebug(_T("[ from CServiceDiscovery:onSDHalting() ] AFTER posting serviceDiscoveryHALTING event, this = %p"), this);
}

CServiceDiscovery::~CServiceDiscovery()
{
	wxLogDebug(_T("Deleting the CServiceDiscovery instance"));
}


#endif // _KBSERVER
