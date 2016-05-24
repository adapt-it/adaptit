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
	wxUnusedVar(bOK);
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
	if (m_pApp->m_bIsKBServerProject) // adaptations KBserver 
	{
		// It's an existing shared kb project - so initialize to what the current settings
		// are, and make the checkbox comply
		m_bSharingAdaptations = TRUE; // initialize
	}
	else
	{
		m_bSharingAdaptations = FALSE; // initialize
	}
	if (m_pApp->m_bIsGlossingKBServerProject) // glosses KBserver
	{
		// It's an existing shared glossing kb project - so initialize to what the current
		// settings are, and make the checkbox comply
		m_bSharingGlosses = TRUE; // initialize
	}
	else
	{
		m_bSharingGlosses = FALSE; // initialize
	}
}

void KbSharingSetup::OnCheckBoxShareAdaptations(wxCommandEvent& WXUNUSED(event))
{
	// This handler is entered AFTER the checkbox value has been changed
	bool bTicked = m_pAdaptingCheckBox->GetValue();
	if (bTicked)
	{
		// The values set in the dialog must win
		m_pApp->m_bIsKBServerProject = TRUE;
		m_pApp->m_bIsKBServerProject_FromConfigFile = TRUE; // sets m_bSharingAdaptations to TRUE
					// within the function AuthenticateCheckAndSetupKBSharing()
	}
	else
	{
		// The values set in the dialog must win
		m_pApp->m_bIsKBServerProject = FALSE;
		m_pApp->m_bIsKBServerProject_FromConfigFile = FALSE; // sets m_bSharingAdaptations to FALSE
					// within the function AuthenticateCheckAndSetupKBSharing()
	}
}

void KbSharingSetup::OnCheckBoxShareGlosses(wxCommandEvent& WXUNUSED(event))
{
	// This handler is entered AFTER the checkbox value has been changed
	bool bTicked = m_pGlossingCheckBox->GetValue();
	if (bTicked)
	{
		// The values set in the dialog must win
		m_pApp->m_bIsGlossingKBServerProject = TRUE;
		m_pApp->m_bIsGlossingKBServerProject_FromConfigFile = TRUE; // sets m_bSharingGlosses to TRUE
					// within the function AuthenticateCheckAndSetupKBSharing()
	}
	else
	{
		// The values set in the dialog must win
		m_pApp->m_bIsGlossingKBServerProject = FALSE;
		m_pApp->m_bIsGlossingKBServerProject_FromConfigFile = FALSE; // sets m_bSharingGlosses to FALSE
					// within the function AuthenticateCheckAndSetupKBSharing()
	}
}

void KbSharingSetup::OnOK(wxCommandEvent& myevent)
{
	// Get the latest setting of the checkbox values - & set app flags accordingly
	bool bAdaptationsTicked = m_pAdaptingCheckBox->GetValue();
	bool bGlossesTicked = m_pGlossingCheckBox->GetValue();
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	wxASSERT(pFrame != NULL);
	bool bDoNotShare = FALSE;

	if (bAdaptationsTicked || bGlossesTicked)
	{
		// Sharing is wanted for one or both of the adapting & glossing KBs...
		// Show the dialog which allows the user to set the boolean: m_bServiceDiscoveryWanted, 
		// for the later AuthenticateCheckAndSetupKBSharing() call to use. Formerly the dialog
		// was opened here - but OSX would not accept the nesting, and froze the GUI, so now
		// it is opened from CMainFrame::OnIdle() when one or both of the booleans are TRUE
		pFrame->m_bKbSvrAdaptationsTicked = bAdaptationsTicked;
		pFrame->m_bKbSvrGlossesTicked = bGlossesTicked;
	}
	else
	{
		// Neither box is ticked, so turn off sharing...
		// ReleaseKBServer(int) int = 1 or 2, does several things. It stops the download timer, deletes it
		// and sets its pointer to NULL; it also saves the last timestamp value to its on-disk file; it
		// then deletes the KbServer instance that was in use for supplying resources to the sharing code
		if (m_pApp->KbServerRunning(1))
		{
			m_pApp->ReleaseKBServer(1); // the adaptations one
		}
		if (m_pApp->KbServerRunning(2))
		{
			m_pApp->ReleaseKBServer(2); // the glossings one
		}

		m_pApp->m_bIsKBServerProject = FALSE;
		m_pApp->m_bIsGlossingKBServerProject = FALSE;

		ShortWaitSharingOff(); //displays "Knowledge base sharing is OFF" for 1.3 seconds
	}
	myevent.Skip(); // close the KbSharingSetup dialog
}

void KbSharingSetup::OnCancel(wxCommandEvent& myevent)
{
	// Cancelling from this dialog should leave the older setting unchanged. The way to
	// remove the setup is now to untick both checkboxes, and then dismiss the dialog
	myevent.Skip();
}

#endif
