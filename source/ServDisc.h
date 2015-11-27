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
class CServiceDiscovery;
//#include "wx/thread.h"

namespace std {}
using namespace std;

//#include <wx/event.h>


class ServDisc : public wxThread  // earlier, base class was wxEvtHandler
{
public:
	ServDisc(wxThreadKind = wxTHREAD_DETACHED);
	//ServDisc(wxString workFolderPath, wxString serviceStr);
	virtual ~ServDisc();

	wxString m_workFolderPath; // location where we'll temporarily store a file of results
	wxString m_serviceStr; // service to be scanned for
	bool     m_bSDIsRunning;

	CServiceDiscovery* m_pServiceDisc;
	CServiceDiscovery* m_backup_ThisPtr; // m_pServiceDisc gets reset to 0xcdcdcdcd before
		// it can be deleted, so I'll store a copy here, and use ithat the pointer in the
		// onServDiscHalting() handler when I want to get m_pServiceDisc deleted

	// for support of subclassing from wxThread...
	// wxThread::OnExit() is called when the thread exits at termination - for self
	// destruction termination or by Delete(), but not if Kill() is used - the latter
	// should never be used, it can leave resources in an indeterminate state
	virtual void		OnExit();

	// This must be defined to have our work content - this is where thread execution
	// begins. Our thread will be of the joinable type (wxTHREAD_JOINABLE)
	virtual void*		Entry();

	//virtual bool    TestDestroy(); <<-- don't provide one, rely on the internal one from thread.h
	// which does the test:  return m_internal->GetState() == STATE_CANCELLED;

protected:
	//void onServDiscHalting(wxCommandEvent& event); <<-- we are not an event handler object

private:
	//DECLARE_EVENT_TABLE();

    //DECLARE_DYNAMIC_CLASS(ServDisc)
};

#endif // SERVDISC_h

#endif // _KBSERVER


