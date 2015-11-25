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
#include <wx/event.h>
#endif

#include "wxServDisc.h"
#include "ServiceDiscovery.h"
#include "ServDisc.h"


IMPLEMENT_DYNAMIC_CLASS(ServDisc, wxEvtHandler)

BEGIN_EVENT_TABLE(ServDisc, wxEvtHandler)
EVT_COMMAND(wxID_ANY, serviceDiscoveryHALTING, ServDisc::onServDiscHalting)
END_EVENT_TABLE()

// We don't use this creator
ServDisc::ServDisc()
{
	wxLogDebug(_T("\nInstantiating a default ServDisc class, ptr to instance = %p"), this);

	m_serviceStr = _T(""); // service to be scanned for
	m_workFolderPath.Empty();

	CServiceDiscovery* m_pServiceDisc = NULL;
	wxUnusedVar(m_pServiceDisc);
}

ServDisc::ServDisc(wxString workFolderPath, wxString serviceStr)
{
	wxLogDebug(_T("\nInstantiating a ServDisc class, passing in workFolderPath %s and serviceStr: %s, ptr to instance = %p"),
		workFolderPath.c_str(), serviceStr.c_str(), this);

	m_serviceStr = serviceStr; // service to be scanned for
	m_bSDIsRunning = TRUE;

	m_workFolderPath = workFolderPath; // pass this on to CServiceDiscovery instance

	CServiceDiscovery* m_pServiceDisc = new CServiceDiscovery(m_workFolderPath, m_serviceStr, this);
	wxUnusedVar(m_pServiceDisc); // prevent compiler warning
}

ServDisc::~ServDisc()
{
	// OnIdle() handler will delete this top level class of ours, lower level ones will
	// be deleted by passing up a halting event request to the parent class which will
	// then delete the child (when the child has no handler still running)
	wxLogDebug(_T("Deleting the ServDisc class instance by ~ServDisc() destructor"));
}

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

#endif // _KBSERVER