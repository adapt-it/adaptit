/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KbSharingSetup.h
/// \author			Bruce Waters
/// \date_created	8 October 2013
/// \rcs_id $Id: KbSharingSetup.h 3028 2013-01-15 11:38:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the KbSharingSetup class.
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

//#if defined(_KBSERVER)

class KbSharingSetup : public AIModalDialog
{
public:
	// Expose all overloads of InitDialog from the base class (wxWindowBase)
	using wxWindowBase::InitDialog; // whm 6Dec2025 added to avoid gcc warning
	KbSharingSetup(wxWindow* parent); // constructor

	virtual ~KbSharingSetup(void); // destructor

	bool m_bSharingAdaptations;
	bool m_bSharingGlosses;
	wxString chosenIpAddr;
	wxString chosenHostname;
	CMainFrame* pFrame;
	bool bAuthenticated;
	bool bCannotInitialiseDlg; // BEW 24Jan22 added, default FALSE. Set TRUE if InitDialog() fails to
							   // properly initialise (i.e. empty ipAddr and/or empty password)

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnCheckBoxShareAdaptations(wxCommandEvent& WXUNUSED(event));
	void OnCheckBoxShareGlosses(wxCommandEvent& WXUNUSED(event));

	CAdapt_ItApp* m_pApp;
	wxCheckBox* m_pAdaptingCheckBox;
	wxCheckBox* m_pGlossingCheckBox;
	wxButton*   m_pSetupBtn; //wxID_OK
	wxRadioBox*		m_pRadioBoxHow;
	int			m_nRadioBoxSelection;

	DECLARE_EVENT_TABLE()
};

//#endif

#endif /* KbSharingSetup_h */
