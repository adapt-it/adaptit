/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			CCTableEditDlg.h
/// \author			Bill Martin
/// \date_created	19 June 2007
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CCCTableEditDlg class. 
/// The CCCTableEditDlg class provides a simple dialog with a large text control 
/// for user editing of CC tables.
/// \derivation		The CCCTableEditDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef TableEditDlg_h
#define TableEditDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "TableEditDlg.h"
#endif

/// The CCCTableEditDlg class provides a simple dialog with a large text control 
/// for user editing of CC tables.
/// \derivation		The CCCTableEditDlg class is derived from AIModalDialog.
class CCCTableEditDlg : public AIModalDialog
{
public:
	CCCTableEditDlg(wxWindow* parent); // constructor
	virtual ~CCCTableEditDlg(void); // destructor
	//enum { IDD = IDD_TABLE_EDIT };
	// other methods
	wxString	m_ccTable;
	wxTextCtrl* pEditCCTable;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	//void OnOK(wxCommandEvent& event);

private:
	// class attributes
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* TableEditDlg_h */
