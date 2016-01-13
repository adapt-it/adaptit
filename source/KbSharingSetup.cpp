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
	EVT_CHECKBOX(ID_CHECKBOX_SHARE_MY_TGT_KB, KbSharingSetup::OnCheckBoxShareAdaptations)
	EVT_CHECKBOX(ID_CHECKBOX_SHARE_MY_GLOSS_KB, KbSharingSetup::OnCheckBoxShareGlosses)
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
	m_pSetupBtn = (wxButton*)FindWindowById(wxID_OK);

    // If the project is currently a KB sharing project, then initialise to the current
    // values for which of the two KBs (or both) is being shared; otherwise, set the member
	// variables to the most commonly expected values (TRUE & FALSE, which is what the
	// wxDesigner initial values are, for adapting and glossing choices, respectively)
	m_bSharingAdaptations = FALSE; // initialize (user's final choice on exit is stored here)
	m_bSharingGlosses = FALSE; // initialize (user's final choice on exit is stored here)
	m_bOnEntry_AdaptationsKBserverRunning = FALSE;
	m_bOnEntry_GlossesKBserverRunning = FALSE;
	if (m_pApp->m_bIsKBServerProject && m_pApp->KbServerRunning(1)) // adaptations KBserver 
	{
		// It's an existing shared kb project - so initialize to what the current settings
		// are, and make the checkbox comply
		m_bOnEntry_AdaptationsKBserverRunning = TRUE;
		m_pAdaptingCheckBox->SetValue(TRUE); // "Share adaptations" checkbox is to be shown ticked
		m_bSharingAdaptations = TRUE; // initialize
	}
	else
	{
		m_pAdaptingCheckBox->SetValue(FALSE); // unticked
		m_bSharingAdaptations = FALSE; // initialize
	}
	if (m_pApp->m_bIsGlossingKBServerProject && m_pApp->KbServerRunning(2)) // glosses KBserver
	{
		// It's an existing shared glossing kb project - so initialize to what the current
		// settings are, and make the checkbox comply
		m_bOnEntry_GlossesKBserverRunning = TRUE;
		m_pGlossingCheckBox->SetValue(TRUE);
		m_bSharingGlosses = TRUE; // initialize
	}
	else
	{
		m_pGlossingCheckBox->SetValue(FALSE); // unticked
		m_bSharingGlosses = FALSE; // initialize
	}

    // Save any existing values (from the app variables which are tied to the project
    // configuration file entries), so that if user is making changes to any of these and
	// the changes fail, the old settings can be restored. Note, on first attempt at
	// sharing, these app variables will be empty or in the case of booleans, FALSE
	m_saveSharingAdaptationsFlag = m_bSharingAdaptations;
	m_saveSharingGlossesFlag = m_bSharingGlosses;

	// save old url and username, we need to test for changes in these
	m_saveOldURLStr = m_pApp->m_strKbServerURL; // save existing value (could be empty)
	m_saveOldUsernameStr = m_pApp->m_strUserID; // ditto
	m_savePassword = m_pApp->GetMainFrame()->GetKBSvrPassword();
}

void KbSharingSetup::OnCheckBoxShareAdaptations(wxCommandEvent& WXUNUSED(event))
{
	// This handler is entered AFTER the checkbox value has been changed
	bool bTicked = m_pAdaptingCheckBox->GetValue();
	if (bTicked)
	{
		m_bSharingAdaptations = TRUE;
	}
	else
	{
		m_bSharingAdaptations = FALSE;
	}
}

void KbSharingSetup::OnCheckBoxShareGlosses(wxCommandEvent& WXUNUSED(event))
{
	// This handler is entered AFTER the checkbox value has been changed
	bool bTicked = m_pGlossingCheckBox->GetValue();
	if (bTicked)
	{
		m_bSharingGlosses = TRUE;
	}
	else
	{
		m_bSharingGlosses = FALSE;
	}
}

void KbSharingSetup::OnOK(wxCommandEvent& myevent)
{
    // Don't commit to the possibly new checkbox values until authentication succeeds,
    // but when we get to the commit stage write them out to the app members so that
    // the project config file can get the updated values
    
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	bool bUserAuthenticating = TRUE; // when true, url is stored in app class, & pwd in the MainFrm class
	// There may be a password stored in this session from an earlier connect lost for
	// some reason, or there may be no password yet stored. Get the password, or empty
	// string as the case may be, and set a boolean to carry the result forward.
    wxString existingPassword = pFrame->GetKBSvrPassword();
	bool bPasswordExists = existingPassword.IsEmpty() ? FALSE : TRUE;
	bool bShowUrlAndUsernameDlg = TRUE; // initialize to the most likely situation
	bool bShowPasswordDlgOnly = FALSE;  // initialize
	bool bAutoConnectKBSvr = FALSE;     // initialize

	// If at least one of the checkboxes is ticked, then authentication is required
	if (m_bSharingAdaptations || m_bSharingGlosses)
	{

	// BEW added 11Jan16,  One (of 3?) locations for service discovery, and associated logic

    // The following is used elsewhere too. DoServiceDiscovery() internally creates an
    // instantiation of the ServiceDiscovery class. Internally it uses the wxServDisc class
    // to get the work done. That in turn uses lower level C functions. The pointer
    // CAdapt_ItApp::m_pServDisc points to the ServiceDiscovery instance and is non-NULL
    // while the service discovery runs, but when it is shut down, that pointer needs to
    // again be set to NULL
	wxString curURL = m_pApp->m_strKbServerURL;
	wxString chosenURL = _T("");
	enum ServDiscDetail returnedValue = SD_NoResultsYet;
	bool bOK = m_pApp->DoServiceDiscovery(curURL, chosenURL, returnedValue);
	if (bOK)
	{
		// Got a URL to connect to
		wxASSERT(returnedValue != SD_NoKBserverFound && (
			returnedValue == SD_FirstTime ||
			returnedValue == SD_SameUrl ||
			returnedValue == SD_UrlDiffers_UserAcceptedIt ||
			returnedValue == SD_MultipleUrls_UserChoseEarlierOne ||
			returnedValue == SD_MultipleUrls_UserChoseDifferentOne ) );

		// Make the chosen URL accessible to authentication (this is the hookup location
		// of the service discovery's url to the earlier KBserver GUI code) for this situation
		m_pApp->m_strKbServerURL = chosenURL;
		
		if (returnedValue == SD_FirstTime || returnedValue == SD_UrlDiffers_UserAcceptedIt)
		{
			// If first time, or if url differs, show url & username dlg. 
			bShowUrlAndUsernameDlg = TRUE;
			bShowPasswordDlgOnly = FALSE;
		}
		else if (returnedValue == SD_SameUrl)
		{
			// If same url, then autoconnect if there is a password stored; 
			// if no stored password, then just ask for that.
			bShowUrlAndUsernameDlg = FALSE; // no need for it
			if (bPasswordExists)
			{
				bShowPasswordDlgOnly = FALSE;
				bAutoConnectKBSvr = TRUE;
			}
			else
			{
				bShowPasswordDlgOnly = TRUE;
				bAutoConnectKBSvr = FALSE;
			}
		}

	} // end of TRUE block for test: if (bOK)
	else
	{
        // Something is wrong, or no KBserver has yet been set running; or what's running
        // is not the one the user wants to connect to (treat this as same as a
        // cancellation), or user cancelled, etc
		wxASSERT(returnedValue == SD_NoKBserverFound || 
				 returnedValue == SD_UrlDiffers_UserRejectedIt ||
				 returnedValue == SD_LookupHostnameFailed ||
				 returnedValue == SD_LookupIPaddrFailed ||
				 returnedValue == SD_MultipleUrls_UserCancelled ||
				 returnedValue == SD_UserCancelled
				 );
		// An error message will have been seen already; so just treat this as a cancellation
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

		ShortWaitSharingOff(35); //displays "Knowledge base sharing is OFF" for 3.5 seconds

		return;
	} // end of else block for test: if(bOK)

// End service discovery module call & associated logic; boolean values carry its results
// forward to the legacy connection code which follows...

	if (bShowUrlAndUsernameDlg == TRUE)
	{
		// Authenticate to the server. Authentication also chooses, via the url provided or
		// typed, which particular KBserver we connect to - there may be more than one available
		KBSharingStatelessSetupDlg dlg(pFrame, bUserAuthenticating);// bUserAuthenticating
								//  should be set FALSE only when someone who may not
								//  be the user is authenticating to the KB Sharing Manager
								//  tabbed dialog gui
		dlg.Center();
		int dlgReturnCode;
here:	dlgReturnCode = dlg.ShowModal();
		if (dlgReturnCode == wxID_OK)
		{
			// Since KBSharingSetup.cpp uses the above KBSharingstatelessSetupDlg, we have to
			// ensure that MainFrms's m_kbserverPassword member is set. Also...
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
					HandleBadLangCodeOrCancel(m_saveOldURLStr, m_saveOldUsernameStr, m_savePassword, 
								m_saveSharingAdaptationsFlag, m_saveSharingGlossesFlag);
					myevent.Skip(); // the dialog will be exited now

					ShortWaitSharingOff(35); //displays "Knowledge base sharing is OFF" for 3.5 seconds

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
					HandleBadGlossingLangCodeOrCancel(m_saveOldURLStr, m_saveOldUsernameStr,  
							m_savePassword, m_saveSharingAdaptationsFlag, m_saveSharingGlossesFlag);
					myevent.Skip(); // the dialog will be exited now

					ShortWaitSharingOff(35); //displays "Knowledge base sharing is OFF" for 3.5 seconds

					return;
				}
			}
			// If control gets to here, we can go ahead and establish the setup(s)

			// Shut down the old settings, and reestablish connection using the new
			// settings (this may involve a url change to share using a different KBserver)
			m_pApp->ReleaseKBServer(1); // the adaptations one
			m_pApp->ReleaseKBServer(2); // the glossing one
			m_pApp->m_bIsKBServerProject = FALSE;
			m_pApp->m_bIsGlossingKBServerProject = FALSE;

            // Give the password to the frame instance which stores it because
            // SetupForKBServer() will look for it there; for normal user authentications
            // it's already stored in pFrame, but for KBSharingManager gui, it needs to
            // store whatever password the manager person is using
			if (!bUserAuthenticating)
			{
				pFrame->SetKBSvrPassword(dlg.m_strStatelessPassword);
			}

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

					ShortWaitSharingOff(35); //displays "Knowledge base sharing is OFF" for 3.5 seconds
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

					ShortWaitSharingOff(35); //displays "Knowledge base sharing is OFF" for 3.5 seconds
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
			myevent.Skip(); // the dialog will be exited now
		} // end of TRUE block for test: if (dlg.ShowModal() == wxID_OK) -- for authenticating
		else if (dlgReturnCode == wxID_CANCEL)
		{
            // User Cancelled the authentication, so the old url, username and password
            // have been restored to their storage in the app and frame window instance; so
            // it remains only to restore the old flag values
			m_pApp->m_bIsKBServerProject = m_saveSharingAdaptationsFlag;
			m_pApp->m_bIsGlossingKBServerProject = m_saveSharingGlossesFlag;
			myevent.Skip(); // the dialog will be exited now

			ShortWaitSharingOff(35); //displays "Knowledge base sharing is OFF" for 3.5 seconds
			return;
		}
		else
		{
			// User clicked OK button but in the OnOK() handler, premature return was asked for
			// most likely due to an empty password submitted or a Cancel from within one of
			// the lower level calls, so allow retry, or a change to the settings, or a Cancel
			// button press at this level instead
			goto here;
		}

		ShortWait(25);  // shows "Connected to KBserver successfully" 
						// for 2.5 secs (and no title in titlebar)

	} // end of TRUE block for test: if (bShowUrlAndUsernameDlg == TRUE)
	else
	{
		// The Authentication dialog (with url and username) does not need to be shown. So
		// we either just show the password dialog (if no password yet is stored), or 
		// autoconnect (if a password is stored already - this latter option is only offered
		// when we know that the config file's url is the same as what was just created from
		// the service discovery results - in this situation, we can pretty safely assume
		// that the stored password applies)
		// Control would get here if the "Setup or Remove Knowledge Base Sharing" menu command
		// is clicked a second time, to change the settings (eg. turn on sharing of glossing
		// KB, or some change - such as turning off sharing to one of the KB types)
		wxString theUrl = m_pApp->m_strKbServerURL;
		wxString theUsername = m_pApp->m_strUserID;
		wxString thePassword;
		if (bPasswordExists && bAutoConnectKBSvr)
		{
			// The url, username and password are all in existence and known, so autoconnect
			thePassword = m_pApp->GetMainFrame()->GetKBSvrPassword();		
		}
		else if (bShowPasswordDlgOnly)
		{
			// The password is not stored, so we must ask for it
			thePassword = m_pApp->GetMainFrame()->GetKBSvrPasswordFromUser(); // show the password dialog
			if (thePassword.IsEmpty())
			{
				wxString title = _("No Password Typed");
				wxString msg = _("No password was typed in. This will be treated as a Cancellation.\nYou can continue working, but sharing of this knowledge base will not happen.\nYou can try again to connect if you wish.");
				wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);

				// User didn't cancel, but treat a empty password as equivalent to a cancel.
				// Leave url and username unchanged. Just restore the old flag values
				m_pApp->m_bIsKBServerProject = m_saveSharingAdaptationsFlag;
				m_pApp->m_bIsGlossingKBServerProject = m_saveSharingGlossesFlag;
				myevent.Skip(); // the dialog will be exited now

				ShortWaitSharingOff(35); //displays "Knowledge base sharing is OFF" for 3.5 seconds
				return;
			}
		}

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
				HandleBadLangCodeOrCancel(m_saveOldURLStr, m_saveOldUsernameStr, m_savePassword, 
							m_saveSharingAdaptationsFlag, m_saveSharingGlossesFlag);
				myevent.Skip(); // the dialog will be exited now

				ShortWaitSharingOff(35); //displays "Knowledge base sharing is OFF" for 3.5 seconds
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
				HandleBadGlossingLangCodeOrCancel(m_saveOldURLStr, m_saveOldUsernameStr,  
						m_savePassword, m_saveSharingAdaptationsFlag, m_saveSharingGlossesFlag);
				myevent.Skip(); // the dialog will be exited now

				ShortWaitSharingOff(35); //displays "Knowledge base sharing is OFF" for 3.5 seconds
				return;
			}
		}
		// If control gets to here, we can go ahead and establish the setup(s)

		// Shut down the old settings, and reestablish connection using the new
		// settings (this may involve a url change to share using a different KBserver)
		m_pApp->ReleaseKBServer(1); // the adaptations one
		m_pApp->ReleaseKBServer(2); // the glossing one
		m_pApp->m_bIsKBServerProject = FALSE;
		m_pApp->m_bIsGlossingKBServerProject = FALSE;

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

				ShortWaitSharingOff(35); //displays "Knowledge base sharing is OFF" for 3.5 seconds
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

				ShortWaitSharingOff(35); //displays "Knowledge base sharing is OFF" for 3.5 seconds
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

		ShortWait(25);  // shows "Connected to KBserver successfully"
						// for 2.5 secs (and no title in titlebar)

	} // end of else block for test: if (bShowUrlAndUsernameDlg == TRUE)

	myevent.Skip(); // the dialog will be exited now

	} // end of TRUE block for test: if (m_bSharingAdaptations || m_bSharingGlosses)
	else
	{
		// Neither checkbox was ticked. Possibly the user is shutting down the current KBserver
		// or KBservers. (He may want to then setup again for a different URL with a second
		// invocation of the dialog.) Handle the possibilities...

		// Shut down the old settings. (This also sets app's m_pKbServer[0] and m_pKbServer[1]
		// to each be NULL.)
		m_pApp->ReleaseKBServer(1); // the adaptations one
		m_pApp->ReleaseKBServer(2); // the glossing one
		m_pApp->m_bIsKBServerProject = FALSE;
		m_pApp->m_bIsGlossingKBServerProject = FALSE;
		myevent.Skip(); // the dialog will be exited now

		ShortWaitSharingOff(35); //displays "Knowledge base sharing is OFF" for 3.5 seconds
	}
}

void KbSharingSetup::OnCancel(wxCommandEvent& myevent)
{
	// Changed no settings; so restore what was saved
	m_pApp->m_strKbServerURL = m_saveOldURLStr;
	m_pApp->m_strUserID = m_saveOldUsernameStr;
	m_pApp->GetMainFrame()->SetKBSvrPassword(m_savePassword);
	m_pApp->m_bIsKBServerProject = m_saveSharingAdaptationsFlag;
	m_pApp->m_bIsGlossingKBServerProject = m_saveSharingGlossesFlag;
	myevent.Skip();
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

void KbSharingSetup::HandleBadLangCodeOrCancel(wxString& saveOldURLStr, wxString& saveOldUsernameStr,
		wxString& savePassword, bool& saveSharingAdaptationsFlag, bool& saveSharingGlossesFlag)
{
	m_pApp->LogUserAction(_T("Wrong src/tgt codes, or user cancelled in CheckLanguageCodes() in KbSharingSetup::OnOK()"));
	wxString title = _("Adaptations language code check failed");
	wxString msg = _("Either the source or target language code is wrong, incomplete or absent; or you chose to Cancel.\nSharing has been turned off. First setup correct language codes, then try again.");
	wxMessageBox(msg,title,wxICON_WARNING | wxOK);
	m_pApp->ReleaseKBServer(1); // the adapting one
	m_pApp->ReleaseKBServer(2); // the glossing one
	m_pApp->m_bIsKBServerProject = FALSE;
	m_pApp->m_bIsGlossingKBServerProject = FALSE;

	// Restore the earlier settings for url, username & password
	m_pApp->m_strKbServerURL = saveOldURLStr;
	m_pApp->m_strUserID = saveOldUsernameStr;
	m_pApp->GetMainFrame()->SetKBSvrPassword(savePassword);
	m_pApp->m_bIsKBServerProject = saveSharingAdaptationsFlag;
	m_pApp->m_bIsGlossingKBServerProject = saveSharingGlossesFlag;
}

void KbSharingSetup::HandleBadGlossingLangCodeOrCancel(wxString& saveOldURLStr, wxString& saveOldUsernameStr,
		wxString& savePassword, bool& saveSharingAdaptationsFlag, bool& saveSharingGlossesFlag)
{
	m_pApp->LogUserAction(_T("Wrong src/glossing codes, or user cancelled in CheckLanguageCodes() in KbSharingSetup::OnOK()"));
	wxString title = _("Glosses language code check failed");
	wxString msg = _("Either the source or glossing language code is wrong, incomplete or absent; or you chose to Cancel.\nSharing has been turned off. First setup correct language codes, then try again.");
	wxMessageBox(msg,title,wxICON_WARNING | wxOK);
	m_pApp->ReleaseKBServer(1); // the adapting one
	m_pApp->ReleaseKBServer(2); // the glossing one
	m_pApp->m_bIsKBServerProject = FALSE;
	m_pApp->m_bIsGlossingKBServerProject = FALSE;

	// Restore the earlier settings for url, username & password
	m_pApp->m_strKbServerURL = saveOldURLStr;
	m_pApp->m_strUserID = saveOldUsernameStr;
	m_pApp->GetMainFrame()->SetKBSvrPassword(savePassword);
	m_pApp->m_bIsKBServerProject = saveSharingAdaptationsFlag;
	m_pApp->m_bIsGlossingKBServerProject = saveSharingGlossesFlag;
}

#endif
