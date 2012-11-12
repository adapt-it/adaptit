/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			WaitDlg.h
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CWaitDlg class. 
/// The CWaitDlg class provides a custom "Please wait" dialog to notify the
/// user that the current process will take some time to complete.
/// The CWaitDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// \derivation		The CWaitDlg class is derived from wxDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef WaitDlg_h
#define WaitDlg_h

#include <wx/animate.h>

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "WaitDlg.h"
#endif
/////////////////////////////////////////////////////////////////////////////
// CWaitDlg dialog

/// The CWaitDlg class provides a custom "Please wait" dialog to notify the
/// user that the current process will take some time to complete.
/// The CWaitDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// \derivation		The CWaitDlg class is derived from wxDialog.
class CWaitDlg : public wxDialog
{
// Construction
public:
	CWaitDlg(wxWindow* parent);   // standard constructor

// Dialog Data
	//enum { IDD = IDD_WAIT };
	wxStaticText* pStatic;
	wxSizer* pWaitDlgSizer;
	//wxPanel* pAnimatedPanel;
	wxAnimationCtrl* m_pAnimationCtrl;

// Overrides

// Implementation
protected:
	CAdapt_ItApp*	m_pApp;
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	DECLARE_EVENT_TABLE()
public:
	int m_nWaitMsgNum;
	wxString WaitMsg;
};

#endif /* WaitDlg_h */
