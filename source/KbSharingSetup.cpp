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

#include <wx/docview.h>

#if defined(_KBSERVER)

// other includes
//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include "Adapt_It.h"
#include "MainFrm.h"
#include "helpers.h"
#include "KbServer.h"
#include "KBSharingAuthenticationDlg.h" //  misnamed, it's actually just an authentication class
#include "KbSharingSetup.h"

// event handler table
BEGIN_EVENT_TABLE(KbSharingSetup, AIModalDialog)
	EVT_INIT_DIALOG(KbSharingSetup::InitDialog)
	EVT_BUTTON(wxID_OK, KbSharingSetup::OnOK)
	EVT_BUTTON(wxID_CANCEL, KbSharingSetup::OnCancel)
	EVT_CHECKBOX(ID_CHECKBOX_SHARE_MY_TGT_KB, KbSharingSetup::OnCheckBoxShareAdaptations)
	EVT_CHECKBOX(ID_CHECKBOX_SHARE_MY_GLOSS_KB, KbSharingSetup::OnCheckBoxShareGlosses)
END_EVENT_TABLE()

// The non-stateless contructor (internally sets m_bForManager to FALSE)
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

    // whm 5Mar2019 Note: The kb_share_setup_or_remove_func() dialog now uses the
    // wxStdDialogButtonSizer, and so we need not call the ReverseOkCancelButtonsForMac()
    // function below.
	//bool bOK;
	//bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	//wxUnusedVar(bOK);
 }

KbSharingSetup::~KbSharingSetup() // destructor
{
}

void KbSharingSetup::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_pAdaptingCheckBox = (wxCheckBox*)FindWindowById(ID_CHECKBOX_SHARE_MY_TGT_KB);
	m_pGlossingCheckBox = (wxCheckBox*)FindWindowById(ID_CHECKBOX_SHARE_MY_GLOSS_KB);
	m_pSetupBtn = (wxButton*)FindWindowById(wxID_OK);
	pFrame = m_pApp->GetMainFrame();
	bAuthenticated = FALSE; // initialise

    // If the project is currently a KB sharing project, then initialise to the current
    // values for which of the two KBs (or both) is being shared; otherwise, set the member
	// variables to the most commonly expected values (TRUE & FALSE, which is what the
	// wxDesigner initial values are, for adapting and glossing choices, respectively)
	m_bSharingAdaptations = FALSE; // initialize (user's final choice on exit is stored here)
	m_bSharingGlosses = FALSE; // initialize (user's final choice on exit is stored here)
	// Get the checkboxes set to agree with the flag values
	bool bAdaptingBoxTicked = m_pAdaptingCheckBox->GetValue();
	bool bGlossingBoxTicked = m_pGlossingCheckBox->GetValue();
	if (m_pApp->m_bIsKBServerProject) // adaptations KBserver 
	{
		// It's an existing shared kb project - so initialize to what the current settings
		// are, and make the checkbox comply
		m_bSharingAdaptations = TRUE; // initialize
		if (!bAdaptingBoxTicked)
		{
			m_pAdaptingCheckBox->SetValue(TRUE);
		}
	}
	else
	{
		m_bSharingAdaptations = FALSE; // initialize
		if (bAdaptingBoxTicked)
		{
			m_pAdaptingCheckBox->SetValue(FALSE);
		}
	}
	if (m_pApp->m_bIsGlossingKBServerProject) // glosses KBserver
	{
		// It's an existing shared glossing kb project - so initialize to what the current
		// settings are, and make the checkbox comply
		m_bSharingGlosses = TRUE; // initialize
		if (!bGlossingBoxTicked)
		{
			m_pGlossingCheckBox->SetValue(TRUE);
		}
	}
	else
	{
		m_bSharingGlosses = FALSE; // initialize
		if (bGlossingBoxTicked)
		{
			m_pGlossingCheckBox->SetValue(FALSE);
		}
	}
	// BEW 15Sep20 additions

	// Determine if the ipAddress is known or not. If not, we need to 
	// programmatically call discovery and choose the correct kbserver	
	chosenIpAddr = m_pApp->m_chosenIpAddr; // initialize
	// Prepare to setup the ipAddresses (in m_theIpAddrs) and 
	// hostnames (in m_theHostnames)
	m_pApp->m_theIpAddrs.Clear(); // wxArrayString
	m_pApp->m_theHostnames.Clear(); // wxArrayString
	wxString aComposite; // for <ipaddr>@@@<hostname> string
	int count;
	int i;
	// Default the chosenHostname to <unknown>
	chosenHostname = _("<unknown>");
	// Has Discover KBservers menu item been called, and an ipAddress chosen?
	if (chosenIpAddr.IsEmpty())
	{
		// Get service discovery done
		wxCommandEvent discoveryEvent; // a dummy event, discovery doesn't use it
		pFrame->OnDiscoverKBservers(discoveryEvent);
		if (m_pApp->m_bServDiscSingleRunIsCurrent)
		{
			wxString title = _("Scanner busy error");
			wxString msg = _("Scanning for KBservers has not yet finished. Please wait, then try again.");
			wxMessageBox(msg, title, wxICON_WARNING | wxOK);
			return;
		}
		chosenIpAddr = m_pApp->m_chosenIpAddr;
	}

	// Next, get the kbservers found, and for the chosen one, separate
	// the composite string into the ipAddr and associated hostname str

	// The list of composites is updated. Decompose each string into the ipAddress and
	// hostname parts, and store in parallel in arrays m_theIpArrs, and m_theHostnames -
	wxString anIpAddress;
	wxString aHostname;
	count = (int)m_pApp->m_ipAddrs_Hostnames.GetCount();
	for (i = 0; i < count; i++)
	{
		anIpAddress = wxEmptyString;
		aHostname = wxEmptyString;
		aComposite = m_pApp->m_ipAddrs_Hostnames.Item(i);
#if defined(_DEBUG)
		if (!aComposite.IsEmpty())
		{
			wxLogDebug(_T("%s::%s() line= %d aComposite : %s , Extracting parts"),
				__FILE__, __FUNCTION__, __LINE__, aComposite.c_str());
		}
#endif
		m_pApp->ExtractIpAddrAndHostname(aComposite, anIpAddress, aHostname);
		wxUnusedVar(anIpAddress);
		m_pApp->m_theIpAddrs.Add(anIpAddress);
		m_pApp->m_theHostnames.Add(aHostname);
	} // end for loop
	// Find which was chosen, and get the matchine hostname, and store these
	// ready for later kbserver calls
	/* BEW 6Nov20 deprecate this test, the return prevents authentication from being done in OnInit()
	if (m_pApp->m_theIpAddrs.IsEmpty())
	{
		wxString title = _("No ipAddresses found");
		wxString msg = _("Extracting ipAddresses and their hostnames failed. The array was empty.");
		wxMessageBox(msg, title, wxICON_WARNING | wxOK);
		m_pApp->LogUserAction(msg);
		return;
	}
	//else
	*/
	if (!m_pApp->m_theIpAddrs.IsEmpty()) // BEW 6Nov20 changed, doesn't matter if empty when in this dialog
	{
		wxString strItem;
		wxString itsHostname = wxEmptyString;
		for (i = 0; i < count; i++)
		{
			strItem = m_pApp->m_theIpAddrs.Item(i);
			if (strItem == chosenIpAddr)
			{
				// This is the one the user chose, so get its accompanying
				// hostname
				itsHostname = m_pApp->m_theHostnames.Item(i);

				// Store these in the 'chosen' variables
				m_pApp->m_chosenHostname = itsHostname;
				m_pApp->m_chosenIpAddr = chosenIpAddr;
				m_pApp->m_strKbServerIpAddr = chosenIpAddr; // normal storage loc'n
				break;
			}
		}
	} // end of else block for test: if (m_pApp->m_theIpAddrs.IsEmpty())

	// We want now to call Authenticate2Dlg, which is part of the KbServer LookupUser()
	// function, so we need to have an instance of KbServer running on the heap, so
	// we can call its LookupUser().
	wxString execPath = m_pApp->execPath;
	wxString distPath = m_pApp->distPath;
	KbServer* pKbSvr = new KbServer(1, TRUE); // TRUE = for manager, but this
							// will be changed to FALSE internally because the
							// user1 and user2 values match
	wxString pwd = m_pApp->m_curNormalPassword; // probably empty, 
							// that's okay, LookupUser will set it
	// If this call succeeds as an authentication attempt, it will set app's
	// boolean m_bUserLoggedIn to TRUE  (and app's m_bUser1IsUser2 will also be
	// TRUE, it's the latter which is tested and if TRUE, then m_bUserLoggedIn
	// gets set to TRUE, otherwise it is FALSE)
	pKbSvr->LookupUser(chosenIpAddr, m_pApp->m_strUserID, pwd, m_pApp->m_strUserID);
	delete pKbSvr;
	bAuthenticated = m_pApp->m_bUserLoggedIn ;
	// What follows in OnOK() can only be done if there was successful authentication


#ifdef _DEBUG
//	int halt_here = 1;
#endif
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

	if (bAdaptationsTicked || bGlossesTicked)
	{
		// Sharing is wanted for one or both of the adapting & glossing KBs...
		// Show the dialog which allows the user to set the boolean: m_bServiceDiscoveryWanted, 
		// for the later AuthenticateCheckAndSetupKBSharing() call to use. Formerly the dialog
		// was opened here - but OSX would not accept the nesting, and froze the GUI, so now
		// it is opened from CMainFrame::OnIdle() when one or both of the booleans are TRUE
		pFrame->m_bKbSvrAdaptationsTicked = bAdaptationsTicked;
		pFrame->m_bKbSvrGlossesTicked = bGlossesTicked;
		m_bSharingAdaptations = bAdaptationsTicked; // need the local value too
		m_bSharingGlosses = bGlossesTicked; // need the local value too
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
		myevent.Skip(); // close the KbSharingSetup dialog
		return;
	}
	// Now we need to check if the nominated Kbserver type, or types, is/are running;
	// if so, leave it/them running. If not, get them running
	bool bSetupAdaptations = FALSE;
	bool bSetupGlosses = FALSE;
	if (bAuthenticated)  // BEW 6Nov20 InitDialog() does the authentication
	{
		bool bAdaptRunning = m_pApp->KbAdaptRunning();
		if (m_bSharingAdaptations)
		{
			if (bAdaptRunning)
			{
				// Shut it down
				bSetupAdaptations = FALSE;
				m_pApp->ReleaseKBServer(1); // the adaptations one
				bAdaptRunning = FALSE;
			}
			// Now it's not running, set it up again
			if (!bAdaptRunning)
			{
				// Get the adaptations KbServer class instantiated
				bSetupAdaptations = m_pApp->SetupForKBServer(1);
				if (bSetupAdaptations)
				{
					m_pApp->m_bAdaptationsKBserverReady = TRUE;
				}
			}
		} // end of TRUE block for test: if (m_bSharingAdaptations)

		bool bGlossesRunning = m_pApp->KbGlossRunning();
		if (m_bSharingGlosses)
		{
			if (bGlossesRunning)
			{
				// Shut it down 
				bSetupGlosses = FALSE;
				m_pApp->ReleaseKBServer(2); // the glosses one
				bGlossesRunning = FALSE;
			}
			// Now glossing KB is not running, set it up again
			if (!bGlossesRunning)
			{
				// Get the glosses KbServer class instantiated
				bSetupGlosses = m_pApp->SetupForKBServer(2);
				if (bSetupGlosses)
				{
					m_pApp->m_bGlossesKBserverReady = TRUE;
				}
			}
		} // end of TRUE block for test: if (m_bSharingGlosses)

		  // Do the feedback to the user with the special wait dialogs here
		if (!bAuthenticated || (!bSetupAdaptations && !bSetupGlosses))
		{
			// There was an error, and sharing was turned off
			ShortWaitSharingOff(); //displays "Knowledge base sharing is OFF" for 1.3 seconds
			m_pApp->m_bUserLoggedIn = FALSE;
		}
		else
		{
			// No error, authentication and setup succeeded
			m_pApp->m_bUserLoggedIn = TRUE;
			ShortWait();  // shows "Connected to KBserver successfully"
						  // for 1.3 secs (and no title in titlebar)
		}
		// TODO? -- what else? I think we are done here
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
