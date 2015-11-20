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
#include <wx/docview.h>   // for wxDocParentFrame base class for CMainFrame
#include "SourcePhrase.h" // CMainFrame needs this for the SPList declaration
#include "MainFrm.h"      // for the pFrame param
#include <wx/log.h>       // for wxLogDebug()

#endif

#include "wxServDisc.h"
#include "ServiceDiscovery.h"
#include "ServDisc.h"
#include <wx/event.h>


IMPLEMENT_DYNAMIC_CLASS(ServDisc, wxEvtHandler)
/*
#if wxVERSION_NUMBER < 2900
DEFINE_EVENT_TYPE(wxServDiscNOTIFY);
#else
wxDEFINE_EVENT(wxServDiscNOTIFY, wxCommandEvent);
#endif

#if wxVERSION_NUMBER < 2900
DEFINE_EVENT_TYPE(wxServDiscHALTING);
DEFINE_EVENT_TYPE(serviceDiscoveryHALTING);
#else
wxDEFINE_EVENT(wxServDiscHALTING, wxCommandEvent);
wxDEFINE_EVENT(serviceDiscoveryHALTING, wxCommandEvent);
#endif
*/
BEGIN_EVENT_TABLE(ServDisc, wxEvtHandler)
EVT_COMMAND(wxID_ANY, serviceDiscoveryHALTING, ServDisc::onServDiscHalting)
END_EVENT_TABLE()


ServDisc::ServDisc()
{
	wxLogDebug(_T("\nInstantiating a default ServDisc class, ptr to instance = %p"), this);

	m_serviceStr = _T(""); // service to be scanned for

	m_pFrame = NULL; 

	CServiceDiscovery* m_pServiceDisc = NULL;
	wxUnusedVar(m_pServiceDisc);
}

ServDisc::ServDisc(CMainFrame* pFrame, wxString serviceStr)
{
	wxLogDebug(_T("\nInstantiating a ServDisc class, passing in pFrame and serviceStr: %s, ptr to instance = %p"),
		serviceStr.c_str(), this);

	m_serviceStr = serviceStr; // service to be scanned for
	m_bSDIsRunning = TRUE;

	m_pFrame = pFrame;

	CServiceDiscovery* m_pServiceDisc = new CServiceDiscovery(m_pFrame, m_serviceStr, this);
	wxUnusedVar(m_pServiceDisc); // prevent compiler warning
}

ServDisc::~ServDisc()
{
	// We require the child, m_pServiceDisc to delete this parent class,
	// and then delete itself (otherwise we have to #include wxServDisc in
	// the app class, and that leads to heaps of name clashes between wx
	// and winsock2.h etc)
	wxLogDebug(_T("Deleting the ServDisc class instance by ~ServDisc() destructor"));
}

void ServDisc::onServDiscHalting(wxCommandEvent& event)
{
	wxUnusedVar(event);

	// What are pointers?
	wxLogDebug(_T("onServDiscHalting: this:  %p    The Child (m_pServiceDisc):  %p"), this, m_pServiceDisc);

	wxLogDebug(_T("onServDiscHalting: this:  %p    Backup: m_backup_ThisPtr:  %p"), this, m_backup_ThisPtr);

	if (m_pServiceDisc != m_backup_ThisPtr)
	{
		// this hack correctly restores the pointer value, if it has become 0xcdcdcdcd
		m_pServiceDisc = m_backup_ThisPtr; 
	}

	// BUT, it turns out we can't delete the child class now, because the onSDNotify() event has
	// not yet completed - it has only got as far as just prior to computing the URL value
	// which is to be stored in pFrame. So we've got to let that handler complete before
	// we try delete this CServiceDiscovery class instance - try doing that in pFrame's OnIdle()
	
	//delete m_pServiceDisc; // delete the CServiceDiscovery class

	//wxLogDebug(_T("Deleted the CServiceDiscovery instance from onServDiscHalting"));
	wxLogDebug(_T("Restored the CServiceDiscovery instance's this pointer, from onServDiscHalting (onSDHalting() still is running)"));
}

#endif // _KBSERVER