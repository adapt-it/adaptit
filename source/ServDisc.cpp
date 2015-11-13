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

//namespace std {}
//using namespace std;


#ifndef WX_PRECOMP
// Include your minimal set of wx headers here
//#include <wx/arrstr.h> 
//#include <wx/docview.h>
//#include "SourcePhrase.h" // needed for definition of SPList, which MainFrm.h uses
//#include "MainFrm.h"
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
#include "ServDisc.h"


IMPLEMENT_DYNAMIC_CLASS(ServDisc, wxObject)


ServDisc::ServDisc()
{
	wxLogDebug(_T("\nInstantiating a default ServDisc class"));

	m_serviceStr = _T(""); // service to be scanned for

	m_pFrame = NULL; 

	CServiceDiscovery* m_pServiceDisc = NULL;
	wxUnusedVar(m_pServiceDisc);
}

ServDisc::ServDisc(CMainFrame* pFrame, wxString serviceStr)
{
	wxLogDebug(_T("\nInstantiating a ServDisc class, passing in pFrame and serviceStr: %s"),
		serviceStr.c_str());

	m_serviceStr = serviceStr; // service to be scanned for

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
	wxLogDebug(_T("Deleting the ServDisc class instance"));
}



#endif // _KBSERVER