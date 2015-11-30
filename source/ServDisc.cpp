#if defined(_KBSERVER)

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ServDisc.h
/// \author			Bruce Waters
/// \date_created	10 November 2015
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for a lightweight C++ class
///                 which has no wx.h dependency, and which instantiates
///                 the CServiceDiscovery class (doing so to avoid namespace
///                 classes with std:: names versus wxWidgets names in app.h
///                 in particular - the Yield() macro conflicts).
/// \derivation		The ServDisc class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ServDisc.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/list.h>
#include <wx/string.h>    // for wxString definition
#include <wx/log.h>       // for wxLogDebug()
#include <wx/utils.h>
//#include <wx/event.h>
//#include <wx/thread.h>
#endif

#include "wxServDisc.h"
#include "ServiceDiscovery.h"
#include "ServDisc.h"
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

//IMPLEMENT_DYNAMIC_CLASS(ServDisc, wxThread)// earlier, base class was wxEvtHandler

BEGIN_EVENT_TABLE(ServDisc, wxEvtHandler)
EVT_COMMAND(wxID_ANY, wxServDiscHALTING, ServDisc::onServDiscHalting)
END_EVENT_TABLE()

// ******************** NOTE *****************
// The 38 memory leaks from mdnsd, a couple of queries, and the strings and cached structs
// of a number of _cache structs, are leaked into the main thread's namespace. Putting the
// service discovery into a thread process does not solve our problem of the leaks. It
// looks like the only way to do it is for me to learn how to fix Beier's incomplete
// cleanup.

ServDisc::ServDisc(wxThreadKind kind)
{
	wxLogDebug(_T("\nInstantiating a detached ServDisc thread class, ptr to instance = %p"), this);
	wxUnusedVar(kind);
	wxASSERT(kind != wxTHREAD_JOINABLE);

	// Check again, my logging says that it is running as a detached thread....
	bool bIsDetached = FALSE;
	bool bIsMain = FALSE;
	bIsMain = wxThread::IsMain(); // This returns TRUE, which is correct
	bIsDetached = wxThread::IsDetached();
	wxLogDebug(_T("ServDisc is a thread of kind: %s "), bIsDetached ? wxString(_T("DETACHED")).c_str() :
					wxString(_T("JOINABLE")).c_str());

	m_bServDiscCanExit = FALSE;
	m_serviceStr = _T(""); // service to be scanned for
	m_workFolderPath.Empty();
	m_pApp = NULL; // initializations, the needed values will be assigned after instantiation
	CServiceDiscovery* m_pServiceDisc = NULL;
	CServiceDiscovery* m_backup_ThisPtr = NULL; 
	wxUnusedVar(m_pServiceDisc);
	wxUnusedVar(m_backup_ThisPtr);
}
/* old code, before I made this a threaded solution
ServDisc::ServDisc(wxString workFolderPath, wxString serviceStr)
{
	wxLogDebug(_T("\nInstantiating a ServDisc class, passing in workFolderPath %s and serviceStr: %s, ptr to instance = %p"),
		workFolderPath.c_str(), serviceStr.c_str(), this);

	m_serviceStr = serviceStr; // service to be scanned for

	m_bSDIsRunning = TRUE;

	m_workFolderPath = workFolderPath; // pass this on to CServiceDiscovery instance

	//this will be moved to the Entry() function of the thread, so that memory leaks will stay in the thread's process
	//CServiceDiscovery* m_pServiceDisc = new CServiceDiscovery(m_workFolderPath, m_serviceStr, this);
	//wxUnusedVar(m_pServiceDisc); // prevent compiler warning
}
*/
ServDisc::~ServDisc()
{
	wxLogDebug(_T("Deleting the ServDisc class instance by ~ServDisc() destructor"));
}

void ServDisc::onServDiscHalting(wxCommandEvent& event)
{
	wxUnusedVar(event);

	// What are the pointer values?
	wxLogDebug(_T("onServDiscHalting: this:  %p    The Child (m_pServiceDisc):  %p"), this, m_pServiceDisc);

	wxLogDebug(_T("onServDiscHalting: this:  %p    Backup: m_backup_ThisPtr:  %p"), this, m_backup_ThisPtr);

	if (m_pServiceDisc != m_backup_ThisPtr)
	{
		// this hack correctly restores the pointer value, if it has become 0xcdcdcdcd
		// (which it regularly does, unfortunately)
		m_pServiceDisc = m_backup_ThisPtr; 
		wxLogDebug(_T("NEEDED to RESTORE the CServiceDiscovery instance's this pointer, within onServDiscHalting()"));
	}
	
	// We can safely shutdown the CServiceDiscovery instance. It creates no heap blocks
	// using new, so a simple delete is all we need here
	delete m_pServiceDisc;

	// Set the boolean TRUE which tells this thread it can destroy itself
	m_bServDiscCanExit = TRUE;	
}


// Do our service discovery work here; we don't pause our thread, so we'll not use TestDestroy()
void* ServDisc::Entry()
{
	m_pApp->m_servDiscResults.Clear(); // clear the array where we'll deposit results

	// Commence the service discovery
	CServiceDiscovery* m_pServiceDisc = new CServiceDiscovery(m_workFolderPath, m_serviceStr, this);
	wxUnusedVar(m_pServiceDisc); // prevent compiler warning

	while (!TestDestroy())
	{
		wxMilliSleep(100); // check TestDestroy every 1/10 of a second
	}

/*
	// Need a timeout loop, of 5 seconds, so that the return NULL statement will not be
	// executed before the service discovery has sufficient time to get it's job done
	// (which takes about 3 seconds for wxServDisc timeout to expire, or 1.5 seconds
	// if a _kbserver._tcp.local. service is present waiting for discovery). We want
	// a wxServDiscHALTING event to cause the app instance to deleted ServDisc, rather
	// than it halt itself and leak memory
	int mytimeout = 5000; // milliseconds
	int mytime = mytimeout;
	while (mytime >= 0)
	{
		if (m_pServiceDisc->m_bWxServDiscIsRunning)
		{
			wxMilliSleep(200);
			mytime -= 200;
			wxLogDebug(_T("ServDisc::Entry: waiting for 5 seconds, mytime  %d  milliseconds"), mytime);
		}
		else
		{
			break;
		}
	}

	wxLogDebug(_T("ServDisc::Entry: timed out; returning (void*)NULL now. ****** We don't want to see this log message. We want app to delete ServDisc instance.******"));
*/
	// Now post a wxServDiscHALTING event to the app instance, so it can shut down this
	// ServDisc instance
	{
    wxCommandEvent event(wxServDiscHALTING, wxID_ANY);
    event.SetEventObject(this); // set sender

    // BEW added this posting...  Send it
#if wxVERSION_NUMBER < 2900
    wxPostEvent(m_pApp, event);
#else
    wxQueueEvent(m_pApp, event.Clone());
#endif
	wxLogDebug(_T("BEW: ServDisc::Entry() m_bServDiscCanExit has gone TRUE, just posted wxServDiscHalting to m_pApp"));
	}

	wxLogDebug(_T("ServDisc::Entry() returning (void*)NULL  so ServDisc will delete itself now"));
	return (void*)NULL;
}

// We can't call Delete() from here, or we'll crash the app. We'll use the event handling
// mechanism, with a wxServDiscHALTING event instead; and TestDestroy()
void ServDisc::OnExit()
{
	wxLogDebug(_T("ServDisc::OnExit() called. (It has nothing to do though.)"));
}


bool ServDisc::TestDestroy()
{
  return m_bServDiscCanExit == TRUE;
}

#endif // _KBSERVER