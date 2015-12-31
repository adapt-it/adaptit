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

BEGIN_EVENT_TABLE(ServDisc, wxEvtHandler)
EVT_COMMAND(wxID_ANY, wxServDiscHALTING, ServDisc::onServDiscHalting)
END_EVENT_TABLE()

// ******************** NOTE *****************
// The 38 memory leaks from mdnsd, a couple of queries, and the strings and cached structs
// of a number of _cache structs, are leaked into the main thread's namespace. Putting the
// service discovery into a thread process does not solve our problem of the leaks. It
// looks like the only way to do it is for me to learn how to fix Beier's incomplete
// cleanup. Yep, did so, making use of a function he created but did not use.

ServDisc::ServDisc(wxMutex* mutex, wxCondition* condition, wxThreadKind kind)
{
	wxLogDebug(_T("\nInstantiating a detached ServDisc thread class, ptr to instance = %p"), this);
	wxUnusedVar(kind);
	wxASSERT(kind != wxTHREAD_JOINABLE);

	m_pMutex = mutex;
	m_pCondition = condition;

	// Check again, my logging says that it is running as a detached thread....
	bool bIsDetached = FALSE;
	bool bIsMain = FALSE;
	bIsMain = wxThread::IsMain(); // This returns TRUE, which is correct
	bIsDetached = wxThread::IsDetached();
	wxLogDebug(_T("ServDisc is a thread of kind: %s "), bIsDetached ? wxString(_T("DETACHED")).c_str() :
					wxString(_T("JOINABLE")).c_str());

	m_bServDiscCanExit = FALSE;
	m_serviceStr = _T(""); // service to be scanned for
	m_pApp = &wxGetApp();

	// initializations, the needed values will be assigned after instantiation
	CServiceDiscovery* m_pServiceDisc = NULL;
	CServiceDiscovery* m_backup_ThisPtr = NULL; 
	wxUnusedVar(m_pServiceDisc);
	wxUnusedVar(m_backup_ThisPtr);
}

ServDisc::~ServDisc()
{
	wxLogDebug(_T("Deleting the ServDisc class instance by ~ServDisc() destructor"));
}

void ServDisc::onServDiscHalting(wxCommandEvent& event)
{
	wxUnusedVar(event);
	// What is the pointer value?
	wxLogDebug(_T("onServDiscHalting: this:  %p    The Child instance (m_pServiceDisc):  %p"), this, m_pServiceDisc);
	
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
	m_pServiceDisc = new CServiceDiscovery(m_serviceStr, this);
	wxUnusedVar(m_pServiceDisc); // prevent compiler warning

	while (!TestDestroy())
	{
		wxMilliSleep(100); // check TestDestroy every 1/10 of a second
	}

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
	wxLogDebug(_T("ServDisc::OnExit() called. [ It has nothing to do except acquire the lock and call Signal() ]"));
	wxLogDebug(_T("ServDisc::OnExit() m_pMutex  =  %p"), m_pMutex);
	wxMutexLocker locker(*m_pMutex);
	bool bIsOK = locker.IsOk();
	wxCondError condError = m_pCondition->Signal();
#if defined(_DEBUG)
	wxString cond0 = _T("wxCOND_NO_ERROR");
	wxString cond1 = _T("wxCOND_INVALID");
	wxString cond2 = _T("wxCOND_TIMEOUT");
	wxString cond3 = _T("wxCOND_MISC_ERROR");
	wxString myError = _T("");
	if (condError == wxCOND_NO_ERROR)
	{
		myError = cond0;
	}
	else if (condError == wxCOND_INVALID)
	{
		myError = cond1;
	}
	else if (condError == wxCOND_TIMEOUT)
	{
		myError = cond2;
	}
	else if (condError == wxCOND_INVALID)
	{
		myError = cond3;
	}
	wxLogDebug(_T("ServDisc::OnExit() error condition for Signal() call: %s   locker.IsOk() returns %s"), 
		myError.c_str(), bIsOK ? wxString(_T("TRUE")).c_str() : wxString(_T("FALSE")).c_str());
#endif
}

bool ServDisc::TestDestroy()
{
  return m_bServDiscCanExit == TRUE;
}

#endif // _KBSERVER