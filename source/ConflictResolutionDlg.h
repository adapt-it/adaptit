/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ConflictResolutionDlg.h
/// \author			Bill Martin
/// \date_created	11 April 2011
/// \date_revised	11 April 2011
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CConflictResolutionDlg class. 
/// The CConflictResolutionDlg class provides a dialog in which a user can resolve the conflicts between
/// different translations of a given portion (verse) of source text.
/// \derivation		The CConflictResolutionDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ConflictResolutionDlg_h
#define ConflictResolutionDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ConflictResolutionDlg.h"
#endif

class CConflictResolutionDlg : public AIModalDialog
{
public:
	CConflictResolutionDlg(wxWindow* parent); // constructor
	virtual ~CConflictResolutionDlg(void); // destructor
	// other methods

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

private:
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ConflictResolutionDlg_h */
