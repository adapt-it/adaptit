/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharingAuthenticationDlg.cpp
/// \author			Bruce Waters
/// \date_created	7 October 2013
/// \rcs_id $Id: KBSharingAuthenticationDlg.cpp 3028 2013-01-15 11:38:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the KBSharingAuthenticationDlg class.
/// The KBSharingAuthenticationDlg class provides a dialog for turning on KB sharing, in two
/// contexts: (1) where no connection to any local KB is wanted, and the username can be anyone
/// listed for the KBserver with given ipAddress - a constructor with extra boolean bForManager
/// in the signature is used. (2) where connection to the local KB is
/// wanted - for normal adapting work's syncing - the constructor only has one param in signature.
///
/// The dialog is not used in the legacy way, but rather as 'forManager' type, when
/// authenticating someone-or-other (an administrator typically) to use
/// the Knowledge Base Sharing Manager GUI - available from the Administrator menu, to
/// add,edit or remove users of the KBserver, and/or add or edit definitions for shared
/// KBs, etc. A flag m_bForManager is always TRUE when the class is instantiated for
/// this purpose.
/// Note: This 'ForManager' instantiation creates a bForManager == TRUE instance of KbServer
/// on the heap, using a creator overload, and deletes it in the destructor. This instance
/// of KbServer supplies needed resources for the Manager GUI, but makes no assumptions about
/// whether or not any kb definitions are in place, and no assumptions about any projects being
/// previously made sharing ones, or not.
// \derivation		The KBSharingAuthenticationDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KBSharingAuthenticationDlg.h"
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

//#if defined(_KBSERVER)

// other includes
#include "Adapt_It.h"
#include "MainFrm.h"
#include "helpers.h"
#include "KbServer.h"
#include "KBSharingAuthenticationDlg.h"

// event handler table
BEGIN_EVENT_TABLE(KBSharingAuthenticationDlg, AIModalDialog)
	EVT_INIT_DIALOG(KBSharingAuthenticationDlg::InitDialog)
	EVT_BUTTON(wxID_OK, KBSharingAuthenticationDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, KBSharingAuthenticationDlg::OnCancel)
END_EVENT_TABLE()

// The 'ForManager' constructor if bUserAuthenticating is FALSE, the normal user
// constructor instead if bUserAuthenticating is TRUE(default, internally sets 
// m_bForManager to TRUE)
// BEW 3Sep20 heavily refactored simpler. Need it for Leon's LookupUser() but the legacy
// version internall calls LookupUser() in an internal function, leading to infinite regression.
// My new version just wants to grab the values from user's typing into the resource
// kb_sharing_stateless_setup_func(this, TRUE, TRUE); so that they can be passed on to
// my UpdatebcurUseradmin() & similar function calls in AI.cpp


KBSharingAuthenticationDlg::KBSharingAuthenticationDlg(wxWindow* parent, bool bUserAuthenticating) // dialog constructor
	: AIModalDialog(parent, -1, _("Authenticate"),
	wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{

	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	kb_sharing_stateless_setup_func(this, TRUE, TRUE);
	// The declaration is: functionname( wxWindow *parent, bool call_fit, bool set_sizer );

    // whm 5Mar2019 Note: The kb_sharing_stateless_setup_func() dialog now uses the
    // wxStdDialogButtonSizer, and so it need not call the ReverseOkCancelButtonsForMac()
    // function below.
	bUsrAuthenticate = bUserAuthenticating;
	m_pApp = &wxGetApp();

	// This dialog has no user2 field, it's called from OnIdle()
	// and m_bUseForeignOption will be FALSE
	wxASSERT(m_pApp->m_bUseForeignOption == FALSE); // check it's so

	if (bUsrAuthenticate)
	{ 
		m_bForManager = FALSE;
		m_strNormalUsername.Empty();
		m_strNormalIpAddr.Empty();
		m_strNormalPassword.Empty();

	}
	/* Since m_bUseForeignOption is false within this dialog, comment out code for TRUE option
	else
	{
		m_bForManager = TRUE;
		m_strForManagerUsername.Empty();
		m_strForManagerIpAddr.Empty();
		m_strForManagerPassword.Empty();
	}
	*/
}

KBSharingAuthenticationDlg::~KBSharingAuthenticationDlg() // destructor
{
}

void KBSharingAuthenticationDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	// If service discovery succeeded, a valid ipAddress should now be in the app member
	// m_strKbServerIpAddr. But other scenarios are possible -- see next comment...
	m_bError = FALSE;
	obfuscatedPassword = wxEmptyString;
    // In a new session, there may have been an ipAddress stored in the app config file, and
    // so that is now available, but the user may want to type a different ipAddress. It's
    // also possible, for example, for the first session for running a KBserver, that no 
	// ipAddress has yet been stored in the config file.
    // Don't show the top message box if the user is wanting service discovery done, or if
    // not wanting service discovery but there is no ipAddress yet to show in the box
	if (bUsrAuthenticate)
	{
		m_saveOldNormalUsernameStr = m_pApp->m_strUserID; // a first guess, we can override	
		m_saveOldNormalIpAddrStr = m_pApp->m_chosenIpAddr; // save value locally in this class 
							// (value could be empty) Note, app has a member identically named
		m_saveNormalPassword = m_pApp->GetMainFrame()->GetKBSvrPassword(); // might be empty, 
														// & is reserved as the 'normal' pwd
	}
	/*
	else
	{
		m_saveOldUsernameStr.Empty();
		if (!m_pApp->m_strForManagerUsername.IsEmpty())
		{
			// If there was something from before, take it as a first guess
			m_strForManagerUsername = m_pApp->m_strForManagerUsername;
		}
		m_saveOldIpAddrStr = m_pApp->m_chosenIpAddr; // save value locally in this class 
														  // (value could be empty) Note, app has a member identically named
		m_savePassword = m_pApp->m_strForManagerPassword; // might be empty
	}
	*/

	// BEW 27Jul20, 'STATELESS' in the following label names come from the wxDesigner
	// resources, and I won't change those  to 'FORMANAGER' names, since user never sees these
	m_pIpAddrCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SERVER_URL_STATELESS);
	m_pUsernameCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_USERNAME_STATELESS);
	m_pUsernameLabel = (wxStaticText*)FindWindowById(ID_TEXT_USERNAME_LABEL_STATELESS);
	m_pUsernameMsgLabel = (wxStaticText*)FindWindowById(ID_TEXT_USERNAME_MSG_LABEL_STATELESS);
	m_pPasswordCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_KBSERVER_PWD);

	wxString otherLabel = _("This is your current unique username");
	wxString managerLabel = _("Set up a username for someone else, or yourself");
	if (bUsrAuthenticate)
	{
		// Flag is TRUE, so user is authenticating and user is an owner of this KBserver
		// If a username string is available, claim it as the current unique one; if not,
		// hide this message line
		if (m_saveOldUsernameStr.IsEmpty())
		{
			m_pUsernameMsgLabel->Show(FALSE);
		}
		else
		{
			m_pUsernameMsgLabel->SetLabel(otherLabel);
		}
	} // end of TRUE block for test: if (m_bUserIsAuthenticating)
	/*
	else
	{
		// Flag is FALSE, user is probably an administrator who is setting up on behalf of
		// this computer's owner; so don't claim the username belongs to the project owner
		m_pUsernameMsgLabel->SetLabel(managerLabel);
	} // end of else block for test: if (m_bUserIsAuthenticating)
	*/
    // BEW added 14Jul13, When m_bForManager is TRUE, we want anybody (e.g. the user's
	// administrator or project supervisor) to be able to grab his machine, open the
	// Knowledge Base Sharing Manager GUI and do things like add users, create kb
	// definitions, and so forth without having to impersonate the actual machine owner,
	// and without any project yet having become a KB Sharing one

	// If the app members have values for the ipAddress and username already (from having
	// been just set earlier, or from the project config file, then reinstate them so that
	// if the kb sharing was turned off it can be quickly re-enabled.
	// When running for authentication to the KB Sharing Manager, try initialize
	// these boxes -- they can start off empty. Also for normal authentication, try to
	// initialize them, but allow the username one to be editable
	m_pIpAddrCtrl->ChangeValue(m_pApp->m_saveOldIpAddrStr);
	if (bUsrAuthenticate)
	{
		//m_pUsernameCtrl->ChangeValue(m_pApp->m_saveOldNormalUsernameStr);
		m_pUsernameCtrl->ChangeValue(m_pApp->m_strUserID);
		m_pUsernameCtrl->SetSelection(0L, -1L); // select it all, as it may be
			// incorrect so it's a good idea to make it instantly removable
	}
	/*
	else
	{
		m_pUsernameCtrl->ChangeValue(m_pApp->m_strForManagerUsername);
		m_pUsernameCtrl->SetSelection(0L,-1L); // select it all, as it may be
			// incorrect so it's a good idea to make it instantly removable
	}
	*/
}

void KBSharingAuthenticationDlg::OnOK(wxCommandEvent& myevent)
{
	wxUnusedVar(myevent);

	wxString strIpAddr;
	strIpAddr = m_pIpAddrCtrl->GetValue();
	if (strIpAddr.IsEmpty())
	{
		wxString msg = _("The IpAddress text box at the top is empty, please type the ipAddress of the server if you know it. Otherwise, Cancel and click Discover Kbservers menu item.");
		wxString title = _("Type IpAddress");
		wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
		this->Raise(); // make sure the Authenticate dialog will be at the top of the z-order
		return; // to the dialog window to have another try
	}
	// We now have something in the ipAddress box -- it might not be a valid ipAddress,
	// but we'll try it. We save values on the app, because the KbServer instance we are using is
	// m_pApp->m_pKbServer_ForManager, and the latter we destroy immediately on exit
	// from this Authentication dialog - so no use trying to store values in any of it's
	// members. Store its ipAddress locally
	if (bUsrAuthenticate)
	{
		m_strNormalIpAddr = strIpAddr;
		m_pApp->UpdateNormalIpAddr(strIpAddr); // Leon's .exe will use this
	}
	/*
	else
	{
		m_strForManagerIpAddr = strIpAddr;
		m_pApp->UpdateIpAddr(strIpAddr); // Leon's .exe will use this
	}
	*/
	// Now the username... - from the dialog's box
	wxString strUsername;
	strUsername = m_pUsernameCtrl->GetValue();
	if (strUsername.IsEmpty())
	{
		// No username, so tell him what to do and let him retry
		wxString msg = _("The username text box is empty. Please type a valid username known to the KBserver.");
		wxString title = _("Type username");
		wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
		this->Raise(); // make sure the Authenticate dialog will be at the top of the z-order
		return; // to the active dialog window
	}

	// A username was present so set it
	if (bUsrAuthenticate)
	{
		m_strNormalUsername = strUsername;
		m_pApp->UpdateCurNormalUsername(strUsername); // Leon's .exe will use this
#if defined(_DEBUG)
		wxLogDebug(_T("Authenticate Dlg (KBSharingSetupDlg.cpp) username: %s , stored locally in: m_strNormalUsername"),
			m_strNormalUsername.c_str());
#endif
	}
	/*
	else
	{
		m_strForManagerUsername = strUsername; // LHS is local
		m_pApp->UpdateCurAuthUsername(strUsername); // Leon's .exe will use this
#if defined(_DEBUG)
		wxLogDebug(_T("Authenticate Dlg (KBSharingSetupDlg.cpp) username: %s , stored locally in: m_strForManagerUsername"),
			m_strForManagerUsername.c_str());
#endif
	}
	*/
	// Get the server password that was typed. If the password box is left empty, 
	// then authentication fails
	wxString pwd = m_pPasswordCtrl->GetValue();
	// Provide box contents to 'normal' storage m_curNormalPassword, and to
	// m_curAuthPassword (in case the user wants to enter the KB Sharing Manager
	// with a valid password from the credentials typed into this Authentication
	// dialog. (But the user may have left the box empty - handle that below)
	m_pApp->m_curNormalPassword = pwd;
	m_pApp->m_curAuthPassword = pwd; /// in case user1 wants to go into KB Sharing Mgr
	if (pwd.IsEmpty())
	{
		// User may have left it empty, so pwd would be an empty string. Empty string
		// will absolutely clobber the Authentication attempt. We may be able to
		// help him/her out by trying passwords already used and stored; we may hit on
		// a winner. Show it obfuscated a bit. Eg, instead of "Clouds2093", show it
		// as "Cl******93" - it may prompt user recollection, or explain why it failed
		wxString testPwd = wxEmptyString;
		/*
		if (bUsrAuthenticate == FALSE)
		{
			testPwd = m_pApp->m_strForeignPassword;
			wxString anotherPwd = m_pApp->m_strForManagerPassword;   // which do I use here?
			wxString stillAnotherPwd = m_pApp->m_curAuthPassword;

			if (testPwd.IsEmpty())
			{
				testPwd = m_pApp->m_curAuthPassword;
			}
			if (!testPwd.IsEmpty())
			{
				pwd = testPwd;
			}
		}
		*/
		if (bUsrAuthenticate == TRUE) // was an else block
		{
			testPwd = m_pApp->m_curNormalPassword; // try this location 
			if (testPwd.IsEmpty())
			{
				testPwd = m_pApp->GetMainFrame()->GetKBSvrPassword();
			}
		} // end of TRUE block for test: if (bUsrAuthenticate == TRUE)
		if (!testPwd.IsEmpty())
		{
			pwd = testPwd; // we've got something to try
		}
		// wxMessageBox to alert user this obfuscated pwd is being tried 
		// because one was not supplied; or if still empty, let authentication
		// fail at Leon's code
		if (pwd.IsEmpty())
		{
			// authentication will fail
			if (!bUsrAuthenticate)
			{
				m_pApp->m_curAuthPassword = wxEmptyString;
			}
			else
			{
				m_pApp->m_curNormalPassword = wxEmptyString;
			}
		}
		else
		{
			// obfuscate the guessed password that will be shown
			int len = pwd.Len();
			wxString firstTwo = pwd.Left(2);
			wxString lastTwo = pwd.Mid(len - 2);
			wxChar asterisk = _T('*');
			wxString middle(asterisk, len - 4);

			if (len > 4)
			{
				// middle has **** etc, show 2 at each end
				obfuscatedPassword = firstTwo + middle + lastTwo;
			}
			else
			{
				// just show first 1 or 2 characters and however many *'s are needed
				if (len > 2)
				{
					wxString firstTwo = pwd.Left(2);
					wxChar asterisk = _T('*');
					wxString middle(asterisk, len - 2);
					obfuscatedPassword = firstTwo + middle;
				}
				else
				{
					// must be a 2-letter password, so make the last char *
					wxString first = pwd.GetChar(0);
					wxChar asterisk = _T('*');
					obfuscatedPassword = first + asterisk;
				}
			}
			// now message the user so he/she knows it's a guess
			wxString msg2 = _("The password box was empty, so Adapt It is guessing based on previous passwords still available. It might be wrong. We do not show passwords fully, but Adapt It is trying one like this (without the asterisks): %s");
			msg2 = msg2.Format(msg2, obfuscatedPassword);
			wxString msgTitle = _("Password is guessed");
			wxMessageBox(msg2, msgTitle, wxICON_WARNING | wxOK);
			m_pApp->LogUserAction(msg2);
		}
	} // end of TRUE block for test: if (pwd.IsEmpty())
	else
	{
		// User provided the pwd, so make use of it
		if (bUsrAuthenticate)
		{
			m_strNormalPassword = pwd; // LHS is the local variable
			m_pApp->UpdateCurNormalPassword(pwd); // Leon's .exe will use this
		}
		/*
		else
		{
			m_strForManagerPassword = pwd; // LHS is the local variable
			m_pApp->UpdateCurAuthPassword(pwd); // Leon's .exe will use this
			m_pApp->m_strForeignPassword = pwd; // another storage location
													// to save in as well
		}
		*/
	}
	myevent.Skip(); // the dialog will be exited now
}

void KBSharingAuthenticationDlg::OnCancel(wxCommandEvent& myevent)
{
	myevent.Skip();
}

//#endif
