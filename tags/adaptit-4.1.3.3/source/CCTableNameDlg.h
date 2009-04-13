/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CCTableNameDlg.h
/// \author			Bill Martin
/// \date_created	19 June 2007
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CCCTableNameDlg class. 
/// The CCCTableNameDlg class provides a simple dialog for the input of a consistent
/// changes table name from the user.
/// \derivation		The CCCTableNameDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef CCTableNameDlg_h
#define CCTableNameDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "CCTableNameDlg.h"
#endif

/// The CCCTableNameDlg class provides a simple dialog for the input of a consistent
/// changes table name from the user.
/// \derivation		The CCCTableNameDlg class is derived from AIModalDialog.
class CCCTableNameDlg : public AIModalDialog
{
public:
	CCCTableNameDlg(wxWindow* parent); // constructor
	virtual ~CCCTableNameDlg(void); // destructor
	//
	//enum { IDD = IDD_DLG_CCT_FNAME };
	wxString m_tableName;
	wxTextCtrl* m_pEditTableName;
	wxTextCtrl* m_pEditCtrlAsStatic;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

private:
	// class attributes
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* CCTableNameDlg_h */
