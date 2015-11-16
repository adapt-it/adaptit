/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ServDisc.h
/// \author			Bruce Waters
/// \date_created	10 November 2015
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for a lightweight C++ class
///                 which has no wx.h dependency, and which instantiates
///                 the CServiceDiscovery class (doing so to avoid namespace
///                 classes with std:: names versus wxWidgets names in app.h
///                 in particular - the Yield() macro conflicts).
/// \derivation		The ServDisc class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#if defined(_KBSERVER)

#ifndef SERVDISC_h
#define SERVDISC_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ServDisc.h"
#endif

// Forward declarations
class wxString;
class CMainFrame;
class CServiceDiscovery;

namespace std {}
using namespace std;
//#include "wxServDisc.h"


#include <wx/object.h>

class ServDisc : public wxObject
{
	friend class CMainFrame;
	friend class CServiceDiscovery;

public:
	ServDisc();
	ServDisc(CMainFrame* pFrame, wxString serviceStr);
	virtual ~ServDisc();

	wxString m_serviceStr; // service to be scanned for
	bool m_bSDIsRunning;

	CMainFrame* m_pFrame;  // pointer to Adapt It's frame window

	CServiceDiscovery* m_pServiceDisc;
private:

    DECLARE_DYNAMIC_CLASS(CServDisc)
};

#endif // SERVDISC_h

#endif // _KBSERVER


