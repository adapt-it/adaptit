/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			UsernameInput.h
/// \author			Bruce Waters
/// \date_created	28 May 2013
/// \rcs_id $Id: Username.h 3254 2013-05-28 03:43:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the UsernameInputDlg class. 
/// The UsernameInputDlg class presents the user with a dialog with two wxTextCtrl fields:
/// the top one is for a usernameID (preferably unique, such as a full email address); the
/// lower one is for an informal username that is more human friendly, such as "bruce
/// waters". These are used in DVCS and KB Sharing features, at a minimum. Invoked from
/// within any project, but the names, once set, apply to all projects (but can be
/// changed, but the new version of either still applies to all projects)
/// \derivation		The UsernameInputDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef UsernameInputDlg_h
#define UsernameInputDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "UsernameInputDlg.h"
#endif

class UsernameInputDlg : public AIModalDialog
{
public:
	UsernameInputDlg(wxWindow* parent); // constructor
	virtual ~UsernameInputDlg(void); // destructor // whm make all destructors virtual

	wxTextCtrl* pUsernameMsgTextCtrl;
	wxTextCtrl* pUsernameTextCtrl;
	wxTextCtrl* pInformalUsernameTextCtrl;
	wxString usernameMsgTitle;
	wxString usernameMsg;
	wxString usernameInformalMsgTitle;
	wxString usernameInformalMsg;
	wxString m_finalUsername;
	wxString m_finalInformalUsername;

protected:

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& WXUNUSED(event)); 
	

private:

	DECLARE_EVENT_TABLE() 
};
#endif /* UsernameInputDlg_h */
