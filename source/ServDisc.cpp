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
//#include <wx/event.h>
//#include <wx/thread.h>
#endif

#include "wxServDisc.h"
#include "ServiceDiscovery.h"
#include "ServDisc.h"



//IMPLEMENT_DYNAMIC_CLASS(ServDisc, wxThread)// earlier, base class was wxEvtHandler

/* wxThread is based on nothing, can't be an event hander unless I mix in using wxThreadHelper
BEGIN_EVENT_TABLE(ServDisc, wxEvtHandler)
EVT_COMMAND(wxID_ANY, serviceDiscoveryHALTING, ServDisc::onServDiscHalting)
END_EVENT_TABLE()
*/
// We don't use this creator
ServDisc::ServDisc(wxThreadKind)
{
	wxLogDebug(_T("\nInstantiating a joinable ServDisc thread class, ptr to instance = %p"), this);

	m_serviceStr = _T(""); // service to be scanned for
	m_workFolderPath.Empty();

	CServiceDiscovery* m_pServiceDisc = NULL;
	CServiceDiscovery* m_backup_ThisPtr = NULL; 
	wxUnusedVar(m_pServiceDisc);
	wxUnusedVar(m_backup_ThisPtr);
}
/*
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
	// OnIdle() handler will delete this top level class of ours, lower level ones will
	// be deleted by passing up a halting event request to the parent class which will
	// then delete the child (when the child has no handler still running)
	wxLogDebug(_T("Deleting the ServDisc class instance by ~ServDisc() destructor"));
}

/* We are not an event handler, so we can't have this unless we use wxThreadHelper & mix with wxEvtHandler
// Need a hack here, the this point gets clobbered and I don't know why, so store a copy
// so it can be restored when we want to destroy the service discovery module
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
		wxLogDebug(_T("It was necessary to restore the CServiceDiscovery instance's this pointer, within onServDiscHalting()"));
	}

	// BUT, it turns out we can't delete the child class now, because the onSDNotify() event has
	// not yet completed - it has only got as far as just prior to computing the URL value
	// which is to be stored in pFrame. So we've got to let that handler complete before
	// we try delete this CServiceDiscovery class instance - do that in pFrame's OnIdle()
}
*/

// Do our service discovery work here; we don't pause our thread, so we'll not use TestDestroy()
void* ServDisc::Entry()
{
	//CServiceDiscovery* m_pServiceDisc = new CServiceDiscovery(m_workFolderPath, m_serviceStr, this);
	//wxUnusedVar(m_pServiceDisc); // prevent compiler warning

	return (void*)NULL;
}

// Probably a good place to do any cleanup, but we can't call Delete() from here, or we'll
// crash the app. We may have to put bool m_bSDIsRunning set to FALSE, onto the app instance
// and then rely on frame's OnIdle() to determine that thread Exit() is wanted -- nope, won't
// work, as Exit() is a protected member of wxThread therefore I can only call it from ServDisc
// hmmm, how to I get cleanup? I may have to use wxThreadHelper, mixing in wxEvtHandler --
// let's try how things go before I make a decision on this issue...
void ServDisc::OnExit()
{

}


// We won't use this, because if we did we'd get instance (and premature) thread destruction
bool ServDisc::TestDestroy()
{
  return true;
}



#endif // _KBSERVER