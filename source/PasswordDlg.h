/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			PasswordDlg.h
/// \author			Bruce Waters
/// \date_created	11 February 2016
/// \rcs_id $Id: PasswordDlg.h 3028 2016-01-25 11:38:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the PasswordDlg class.
/// The PasswordDlg class provides a relocatable dialog for letting the user 
/// type in a password. This dialog is used in the KB Sharing module.
/// \derivation		The PasswordDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef PasswordDlg_h
#define PasswordDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "PasswordDlg.h"
#endif

#if defined(_KBSERVER)

class PasswordDlg : public AIModalDialog
{
public:
	PasswordDlg(wxWindow* parent); // constructor

	virtual ~PasswordDlg(void); // destructor

	bool     m_bUserClickedCancel;
	wxString m_password;

protected:
	CAdapt_ItApp* m_pApp;
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	wxTextCtrl* m_pTextPasswordCtrl;

	DECLARE_EVENT_TABLE()
};

#endif

#endif /* PasswordDlg_h */
