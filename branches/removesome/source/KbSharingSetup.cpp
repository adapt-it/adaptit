/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KbSharingSetup.cpp
/// \author			Bruce Waters
/// \date_created	8 October 2013
/// \rcs_id $Id: KBSharingSetup.cpp 3028 2013-01-15 11:38:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the KbSharingSetup class.
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

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KbSharingSetup.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

#if defined(_KBSERVER)

// other includes
//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include "Adapt_It.h"
#include "MainFrm.h"
#include "helpers.h"
#include "KbServer.h"
#include "KBSharingStatelessSetupDlg.h" //  misnamed, it's actually just an authentication class
#include "KbSharingSetup.h"

// event handler table
BEGIN_EVENT_TABLE(KbSharingSetup, AIModalDialog)
	EVT_INIT_DIALOG(KbSharingSetup::InitDialog)
	EVT_BUTTON(wxID_OK, KbSharingSetup::OnOK)
	EVT_BUTTON(wxID_CANCEL, KbSharingSetup::OnCancel)
	EVT_BUTTON(ID_BUTTON_REMOVE_MY_SETUP, KbSharingSetup::OnButtonRemoveSetup)
	EVT_UPDATE_UI(ID_BUTTON_REMOVE_MY_SETUP, KbSharingSetup::OnUpdateButtonRemoveSetup)
END_EVENT_TABLE()

// The non-stateless contructor (internally sets m_bStateless to FALSE)
KbSharingSetup::KbSharingSetup(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Setup Or Remove Knowledge Base Sharing"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{

    // This dialog function below is generated in wxDesigner, and defines the controls and
    // sizers for the dialog. The first parameter is the parent which is the pointer this.
    // (Warning, do not use the frame window as the parent - the dialog will show empty and
    // the frame window will resize down to the dialog's size!) The second and third
    // parameters should both be TRUE to utilize the sizers and create the right size
    // dialog.
	m_pApp = &wxGetApp();
	kb_share_setup_or_remove_func(this, TRUE, TRUE);
	// The declaration is: functionname( wxWindow *parent, bool call_fit, bool set_sizer );
	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	m_bStateless = FALSE;
 }

KbSharingSetup::~KbSharingSetup() // destructor
{
}

void KbSharingSetup::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_pAdaptingCheckBox = (wxCheckBox*)FindWindowById(ID_CHECKBOX_SHARE_MY_TGT_KB);
	m_pGlossingCheckBox = (wxCheckBox*)FindWindowById(ID_CHECKBOX_SHARE_MY_GLOSS_KB);
	m_pRemoveSetupBtn = (wxButton*)FindWindowById(ID_BUTTON_REMOVE_MY_SETUP);

    // If the project is currently a KB sharing project, then initialise to the current
    // values for which of the two KBs (or both) is being shared; otherwise, set the member
	// variables to the most commonly expected values (TRUE & FALSE, which is what the
	// wxDesigner initial values are, for adapting and glossing choices, respectively)
	m_bSharingAdaptations = TRUE;
	m_bSharingGlosses = FALSE;
	if (m_pApp->m_bIsKBServerProject || m_pApp->m_bIsGlossingKBServerProject)
	{
		// It's an existing shared kb project - so initialize to what the current settings
		// are, and make the checkboxes comply
		m_bSharingAdaptations = m_pApp->m_bIsKBServerProject;
		m_bSharingGlosses = m_pApp->m_bIsGlossingKBServerProject;
		m_pAdaptingCheckBox->SetValue(m_bSharingAdaptations);
		m_pGlossingCheckBox->SetValue(m_pApp->m_bIsGlossingKBServerProject);
	}

    // Save any existing values (from the app variables which are tied to the project
    // configuration file entries), so that if user is making changes to any of these and
    // the changes fail, the old settings can be restored
	m_saveSharingAdaptationsFlag = m_pApp->m_bIsKBServerProject;
	m_saveSharingGlossesFlag = m_pApp->m_bIsGlossingKBServerProject;

	// save old url and username, we need to test for changes in these
	m_saveOldURLStr = m_pApp->m_strKbServerURL; // save existing value (could be empty)
	m_saveOldUsernameStr = m_pApp->m_strUserID; // ditto
	m_savePassword = m_pApp->GetMainFrame()->GetKBSvrPassword();
}

void KbSharingSetup::OnOK(wxCommandEvent& myevent)
{
	// Get the final checkbox values; don't commit to the possibly new values until
	// authentication succeeds, but when we get to the commit stage write them out to the
	// app members so that the project config file can get the updated values
	m_bSharingAdaptations = m_pAdaptingCheckBox->GetValue(); // default is TRUE
	m_bSharingGlosses = m_pGlossingCheckBox->GetValue(); // default is FALSE

	// Test that at least one of the local KBs is to be shared. If neither, then put the
	// user back in the dialog and ask him to make a choice, or Cancel
	if (!m_bSharingAdaptations  && !m_bSharingGlosses)
	{
		wxString title = _("Warning: nothing is shared");
		wxString msg = _("Neither checkbox was turned on, so neither the adaptations knowledge base, nor the glossing one, has been chosen for sharing.\nMake a choice now. Or Cancel, which will restore the earlier settings.");
		wxMessageBox(msg,title,wxICON_WARNING | wxOK);
		return; // go back to the active dialog window
	}

	// Authenticate to the server. Authentication also chooses, via the url provided or
	// typed, which particular kbserver we connect to - there may be more than one available
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	KBSharingStatelessSetupDlg dlg(pFrame);
	dlg.Center();
	if (dlg.ShowModal() == wxID_OK)
	{
        // Check that needed language codes are defined for source, target, and if a
        // glossing kb share is also wanted, that source and glosses codes are set too. Get
        // them set up if not so. If user cancels, don't go ahead with the setup, and in
        // that case if app's m_bIsKBServerProject is TRUE, make it FALSE, and likewise for
        // m_bIsGlossingKBServerProject if relevant
		bool bUserCancelled = FALSE;
		// We want valid codes for source and target if sharing the adaptations KB, and
		// for source and glosses languages if sharing the glossing KB. (CheckLanguageCodes
		// is in helpers.h & .cpp) We'll start by testing adaptations KB, if that is
		// wanted. Then again for glossing KB if that is wanted (usually it won't be)
		bool bDidFirstOK = TRUE;
		bool bDidSecondOK = TRUE;
		if (m_bSharingAdaptations)
		{
			bDidFirstOK = CheckLanguageCodes(TRUE, TRUE, FALSE, FALSE, bUserCancelled);
			if (!bDidFirstOK || bUserCancelled)
			{
				// We must assume the src/tgt codes are wrong or incomplete, or that the
				// user has changed his mind about KB Sharing being on - so turn it off
				m_pApp->LogUserAction(_T("Wrong src/tgt codes, or user cancelled in CheckLanguageCodes() in KbSharingSetup::OnOK()"));
				wxString title = _("Adaptations language code check failed");
				wxString msg = _("Either the source or target language code is wrong, incomplete or absent; or you chose to Cancel.\nSharing has been turned off. First setup correct language codes, then try again.");
				wxMessageBox(msg,title,wxICON_WARNING | wxOK);
				m_pApp->ReleaseKBServer(1); // the adapting one
				m_pApp->ReleaseKBServer(2); // the glossing one
				m_pApp->m_bIsKBServerProject = FALSE;
				m_pApp->m_bIsGlossingKBServerProject = FALSE;

				// Restore the earlier settings for url, username & password
				m_pApp->m_strKbServerURL = m_saveOldURLStr;
				m_pApp->m_strUserID = m_saveOldUsernameStr;
				m_pApp->GetMainFrame()->SetKBSvrPassword(m_savePassword);
				m_pApp->m_bIsKBServerProject = m_saveSharingAdaptationsFlag;
				m_pApp->m_bIsGlossingKBServerProject = m_saveSharingGlossesFlag;

				myevent.Skip(); // the dialog will be exited now
				return;
			}
		}
		// Now, check for the glossing kb code, if that kb is to be shared
		bUserCancelled = FALSE; // re-initialize
		if (m_bSharingGlosses)
		{
			bDidSecondOK = CheckLanguageCodes(TRUE, FALSE, TRUE, FALSE, bUserCancelled);
			if (!bDidSecondOK || bUserCancelled)
			{
				// We must assume the src/gloss codes are wrong or incomplete, or that the
				// user has changed his mind about KB Sharing being on - so turn it off
				m_pApp->LogUserAction(_T("Wrong src/glossing codes, or user cancelled in CheckLanguageCodes() in KbSharingSetup::OnOK()"));
				wxString title = _("Glosses language code check failed");
				wxString msg = _("Either the source or glossing language code is wrong, incomplete or absent; or you chose to Cancel.\nSharing has been turned off. First setup correct language codes, then try again.");
				wxMessageBox(msg,title,wxICON_WARNING | wxOK);
				m_pApp->ReleaseKBServer(1); // the adapting one
				m_pApp->ReleaseKBServer(2); // the glossing one
				m_pApp->m_bIsKBServerProject = FALSE;
				m_pApp->m_bIsGlossingKBServerProject = FALSE;

				// Restore the earlier settings for url, username & password
				m_pApp->m_strKbServerURL = m_saveOldURLStr;
				m_pApp->m_strUserID = m_saveOldUsernameStr;
				m_pApp->GetMainFrame()->SetKBSvrPassword(m_savePassword);
				m_pApp->m_bIsKBServerProject = m_saveSharingAdaptationsFlag;
				m_pApp->m_bIsGlossingKBServerProject = m_saveSharingGlossesFlag;

				myevent.Skip(); // the dialog will be exited now
				return;
			}
		}
		// If control gets to here, we can go ahead and establish the setup(s)

		// Shut down the old settings, and reestablish connection using the new
		// settings (this may involve a url change to share using a different kbserver)
		m_pApp->ReleaseKBServer(1); // the adaptations one
		m_pApp->ReleaseKBServer(2); // the glossing one
		m_pApp->m_bIsKBServerProject = FALSE;
		m_pApp->m_bIsGlossingKBServerProject = FALSE;

		// Give the password to the frame instance which stores it because
		// SetupForKBServer() will look for it there
		pFrame->SetKBSvrPassword(dlg.m_strStatelessPassword);

		// Do the setup or setups
		if (m_bSharingAdaptations)
		{
			// We want to share the local adaptations KB
			m_pApp->m_bIsKBServerProject = TRUE;
			if (!m_pApp->SetupForKBServer(1)) // try to set up an adapting KB share
			{
				// an error message will have been shown, so just log the failure
				m_pApp->LogUserAction(_T("SetupForKBServer(1) failed in OnOK()"));
				m_pApp->m_bIsKBServerProject = FALSE; // no option but to turn it off
				// Tell the user
				wxString title = _("Setup failed");
				wxString msg = _("The attempt to share the adaptations knowledge base failed.\nYou can continue working, but sharing of this knowledge base will not happen.");
				wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
			}
		}
		if (m_bSharingGlosses)
		{
			// We want to share the local glossing KB
			m_pApp->m_bIsGlossingKBServerProject = TRUE;
			if (!m_pApp->SetupForKBServer(2)) // try to set up a glossing KB share
			{
				// an error message will have been shown, so just log the failure
				m_pApp->LogUserAction(_T("SetupForKBServer(2) failed in OnOK()"));
				m_pApp->m_bIsGlossingKBServerProject = FALSE; // no option but to turn it off
				// Tell the user
				wxString title = _("Setup failed");
				wxString msg = _("The attempt to share the glossing knowledge base failed.\nYou can continue working, but sharing of of this glossing knowledge base will not happen.");
				wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
			}
		}
		// ensure sharing starts off enabled
		if (m_pApp->GetKbServer(1) != NULL)
		{
			// Success if control gets to this line
			m_pApp->GetKbServer(1)->EnableKBSharing(TRUE);
		}
		if (m_pApp->GetKbServer(2) != NULL)
		{
			m_pApp->GetKbServer(2)->EnableKBSharing(TRUE);
		}
	} // end of TRUE block for test: if (dlg.ShowModal() == wxID_OK)
	else
	{
        // User Cancelled the authentication, so the old url, username and password have
        // been restored to their storage in the app and frame window instance; so it
        // remains only to restore the old flag values
		m_pApp->m_bIsKBServerProject = m_saveSharingAdaptationsFlag;
		m_pApp->m_bIsGlossingKBServerProject = m_saveSharingGlossesFlag;
		myevent.Skip(); // the dialog will be exited now
		return;
	}
	myevent.Skip(); // the dialog will be exited now
}

void KbSharingSetup::OnCancel(wxCommandEvent& myevent)
{
	// Done nothing successful, or changed no settings; so restore what was saved
	m_pApp->m_strKbServerURL = m_saveOldURLStr;
	m_pApp->m_strUserID = m_saveOldUsernameStr;
	m_pApp->GetMainFrame()->SetKBSvrPassword(m_savePassword);
	m_pApp->m_bIsKBServerProject = m_saveSharingAdaptationsFlag;
	m_pApp->m_bIsGlossingKBServerProject = m_saveSharingGlossesFlag;

	myevent.Skip();
}

void KbSharingSetup::OnUpdateButtonRemoveSetup(wxUpdateUIEvent& event)
{
	if (m_pApp->m_bIsKBServerProject || m_pApp->m_bIsGlossingKBServerProject)
	{
		event.Enable(TRUE);
	}
	else
	{
		event.Enable(FALSE);
	}
}

void KbSharingSetup::OnButtonRemoveSetup(wxCommandEvent& WXUNUSED(event))
{
	m_pApp->m_bIsKBServerProject = FALSE;
	m_pApp->m_bIsGlossingKBServerProject = FALSE;
	m_pApp->ReleaseKBServer(1); // the adaptations one
	m_pApp->ReleaseKBServer(2); // the glossings one

	// make the dialog close (a good way to say, "it's been done");
	// also, we don't want a subsequent OK button click, because the user would then see a
	// message about the empty top editctrl, etc
	EndModal(wxID_OK);
}

#endif
