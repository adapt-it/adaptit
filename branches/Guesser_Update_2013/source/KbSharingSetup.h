/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KbSharingSetup.h
/// \author			Bruce Waters
/// \date_created	8 October 2013
/// \rcs_id $Id: KbSharingSetup.h 3028 2013-01-15 11:38:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the KbSharingSetupDlg class.
/// The KbSharingSetup class provides a dialog for the turning on, or off, KB Sharing for
/// the currently open Adapt It project. When setting up, checkboxes stipulate which of the
/// local KBs gets shared. Default is to share just the adapting KB. As long as at least
/// one of the local KBs is shared, we have a "shared project". When removing a setup,
/// whichever or both of the shared KBs are no longer shared. "No longer shared" just means
/// that the booleans, m_bIsKBServerProject and m_bIsGlossingKBServerProject are both
/// FALSE. (Note: temporary enabling/disabling is possible within the KbServer
/// instantiation itself, this does not destroy the setup however. By default, when a setup
/// is done, sharing is enabled.)
/// This dialog does everthing of the setup except the authentication step, the latter
/// fills out the url, username, and password - after that, sharing can go ahead.
/// \derivation		The KbSharingSetup class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef KbSharingSetup_h
#define KbSharingSetup_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KbSharingSetup.h"
#endif

#if defined(_KBSERVER)

class KbSharingSetup : public AIModalDialog
{
public:
	KbSharingSetup(wxWindow* parent); // constructor

	virtual ~KbSharingSetup(void); // destructor

	// A public boolean member to specify whether stateless (ie. opened by anyone
	// regardless of whether a project is open or whether it is one for kb sharing), or
	// not (default is not to be stateless). The stateless instantiation if for use by
	// the KB Sharing Manager - which needs to get a temporary instance of KbServer class
	// open (just the adapting one will do) in order to get access to it's methods and the
	// stateless storage - i.e. wxString stateless_username, etc. For normal adapting or
	// glossing work, m_bStateless must be FALSE. KbServer class sets its value of
	// m_bStateless from the one passed to it from here.
	bool m_bStateless;

	// other methods

	// Next two are needed because the user can invoke this dialog on an existing setup
	// not realizing it is already running, and so we need to be able to check for that
	// and tell him all's well; and we also need to be able to check for when the user
	// wants to alter what kbs are to be shared
	bool	m_saveSharingAdaptationsFlag;
	bool	m_saveSharingGlossesFlag;
	// Store checkbox settings here
	bool	m_bSharingAdaptations;
	bool	m_bSharingGlosses;
	// Need the following to be able to test for changes to url or username 
	wxString m_saveOldURLStr;
	wxString m_saveOldUsernameStr;
	wxString m_savePassword;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnButtonRemoveSetup(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonRemoveSetup(wxUpdateUIEvent& event);

	CAdapt_ItApp* m_pApp;
	wxButton*   m_pRemoveSetupBtn; //ID_KB_SHARING_REMOVE_SETUP_WHICH
	wxCheckBox* m_pAdaptingCheckBox;
	wxCheckBox* m_pGlossingCheckBox;

	DECLARE_EVENT_TABLE()
};

#endif

#endif /* KbSharingSetup_h */
