/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Authenticate2Dlg.cpp
/// \author			Bruce Waters
/// \date_created	8 Sepember 2020
/// \rcs_id $Id: Authenticate2Dlg.cpp 3128 2020-09-08 15:40:00Z bruce_waters@sil.org $
/// \copyright		2020 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CAuthenticate2Dlg class.
/// The Authenticate2Dlg class provides an authentication dialog for KB sharing, for
/// just one function, the LookupUser() function, which takes a first username for 
/// authentication purposes, and a second username (same or different) which is to be
/// looked up. If the same name is used for both usernames, it becomes a sufficent 
/// authentication call for setting m_strUserID up as the owner of the kbserver KB
/// in use for that username. If different, it's just a lookup for the credentials for
/// any non-work user in the user table.
// \derivation		The Authenticate2Dlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "Authenticate2Dlg.h"
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
#include "Adapt_It.h"
#include "MainFrm.h"
#include "helpers.h"
#include "KbServer.h"
#include "Authenticate2Dlg.h"

// event handler table
BEGIN_EVENT_TABLE(Authenticate2Dlg, AIModalDialog)
EVT_INIT_DIALOG(Authenticate2Dlg::InitDialog)
EVT_BUTTON(wxID_OK, Authenticate2Dlg::OnOK)
EVT_BUTTON(wxID_CANCEL, Authenticate2Dlg::OnCancel)
END_EVENT_TABLE()

// BEW12Nov20 the bUserAuthenticating value passed in from the caller is now:
// bAllow = m_bUserAuthenticating || m_bUseForeignOption, the latter is true
// when authenticating to the KB Sharing Manager. I've retained the legacy
// bool's name, but when needing a different code path when the manager is being
// accessed, I use pApp's m_bUseForeignOption (it's true at such a time)
// to vary the path thru the code appropriately.
Authenticate2Dlg::Authenticate2Dlg(wxWindow* parent, bool bUserAuthenticating) // dialog constructor
	: AIModalDialog(parent, -1, _("Authenticate For Lookup User"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	user1_user2_lookup_func(this, TRUE, TRUE);
	// The declaration is: functionname( wxWindow *parent, bool call_fit, bool set_sizer );

	// whm 5Mar2019 Note: The user1_user2_lookup_func() dialog now uses the
	// wxStdDialogButtonSizer, and so it need not call the ReverseOkCancelButtonsForMac()
	// function below.
	bUsrAuthenticate = bUserAuthenticating;
	m_pApp = &wxGetApp();
	if (bUsrAuthenticate)
	{
		m_bForManager = FALSE;
		m_strNormalUsername.Empty();
		m_strNormalIpAddr.Empty();
		m_strNormalPassword.Empty();
		m_strNormalUsername2.Empty();

	}
//#if defined (_DEBUG)
//	int halt_here = 1;
//#endif
}

Authenticate2Dlg::~Authenticate2Dlg() // destructor
{
}

void Authenticate2Dlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	// If service discovery succeeded, a valid ipAddress should now be in the app member
	// m_strKbServerIpAddr. But other scenarios are possible -- see next comment...
	m_bError = FALSE;
	obfuscatedPassword = wxEmptyString; // there are 12 refs to this, so don't remove it
			//  If we show a pwd, it gets some content of it replaced by * characters
	// In a new session, there may have been an ipAddress stored in the app config file, and
	// so that is now available, but the user may want to type a different ipAddress. It's
	// also possible, for example, for the first session for running a KBserver, that no 
	// ipAddress has yet been stored in the config file.
	// Don't show the top message box if the user is wanting service discovery done, or if
	// not wanting service discovery but there is no ipAddress yet to show in the box
	if (bUsrAuthenticate) // remember bUsrAuthenticate is whichever of m_bUserAuthenticating
						  // or m_bUseForeignOption is TRUE, the latter is TRUE when
						  // seeking access to the KB Sharing Manager
	{
		// these saved values may be empty
		m_saveOldNormalIpAddrStr = m_strNormalIpAddr;
		m_saveOldNormalUsernameStr = m_strNormalUsername;
		m_saveNormalPassword = m_strNormalPassword;

		m_strNormalUsername = m_pApp->m_strUserID; 
		m_strNormalIpAddr = m_pApp->m_chosenIpAddr; // could be empty 
		if (!m_pApp->m_bUseForeignOption)
		{
			m_strNormalPassword = m_pApp->GetMainFrame()->GetKBSvrPassword(); // might be empty
		}
		else
		{
			// I'll store the pwd for user1 in m_curAuthPassword. May never need this, but no harm
			m_pApp->UpdateCurAuthPassword(m_pApp->GetMainFrame()->GetKBSvrPassword()); // might be empty
		}
	}

	// BEW 11Nov20, 'STATELESS' in the following label names come from the wxDesigner
	// resources  -- the 2-user Authenticate uses these too
	m_pIpAddrCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SERVER_URL_STATELESS);
	m_pUsernameCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_USERNAME_STATELESS); 
	m_pUsername2Ctrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_USER2);
	m_pUsernameLabel = (wxStaticText*)FindWindowById(ID_TEXT_USERNAME_LABEL_STATELESS);
	m_pUsernameMsgLabel = (wxStaticText*)FindWindowById(ID_TEXT_USERNAME_MSG_LABEL_STATELESS);
	m_pPasswordCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_KBSERVER_PWD);

	// Set an appropriate label line in the dialog
	wxString otherLabel = _("This is your current unique username");
	wxString managerLabel = _("In the lower box, type a username to be checked for in the list of users");
	if (!m_pApp->m_bUseForeignOption)
	{
		m_pUsernameMsgLabel->SetLabel(otherLabel);	
	}
	else
	{
		m_pUsernameMsgLabel->SetLabel(managerLabel);
	}

	  // BEW added 14Jul13, When m_bForManager is TRUE, we want anybody (e.g. the user's
	  // administrator or project supervisor) to be able to grab his machine, open the
	  // Knowledge Base Sharing Manager GUI and do things like add users, edit passwords,
	  // etc, without having to impersonate the settings of the current project's user;
	  // and without any project yet having become a KB Sharing one

	  // If the app members have values for the ipAddress and username already - from having
	  // been just set earlier, or from the project config file, then reinstate them so that
	  // if the kb sharing was turned off it can be quickly re-enabled.
	  // When running for authentication to the KB Sharing Manager, try initialize
	  // some of these boxes -- but they can start off empty, provided valid contents
	  // can be typed in.
	m_pIpAddrCtrl->ChangeValue(m_pApp->m_strKbServerIpAddr); 

	if (!m_pApp->m_bUseForeignOption)
	{
		// When being used for authentication purposes
		m_pUsernameCtrl->ChangeValue(m_pApp->m_strUserID); // for user1
		m_pUsernameCtrl->SetSelection(0L, -1L); // select it all, as it may be
			// incorrect so it's a good idea to make it instantly removable
		m_pUsername2Ctrl->ChangeValue(m_pApp->m_strUserID); // same as user1
		m_pUsername2Ctrl->SetSelection(0L, -1L); // select it all
		// We help the user out by pre-filling both username text boxes.
		// He/she only then needs to supply the right password
	}
	else
	{
		// Authenticating for access to the KB Sharing Manager
		m_pUsernameCtrl->ChangeValue(m_pApp->m_strUserID); // for user1
		m_pUsernameCtrl->SetSelection(0L, -1L); // select it all, as it may be
			// incorrect so it's a good idea to make it instantly removable
		m_pUsername2Ctrl->ChangeValue(wxEmptyString); // for user2 - needs user
			// to type it in
	}
}

void Authenticate2Dlg::OnOK(wxCommandEvent& myevent)
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
	//if (bUsrAuthenticate)
	// Hmm, I may be over-engineering the use of this distinction
	if (!m_pApp->m_bUseForeignOption)
	{
		// wanting normal adapting work
		m_strNormalIpAddr = strIpAddr; // LHS is local
		m_pApp->UpdateNormalIpAddr(strIpAddr); // Leon's .exe will use this
	}
	else
	{
		// wanting KB Sharing Manager access
		m_strForManagerIpAddr = strIpAddr; // LHS is local
		m_pApp->UpdateIpAddr(strIpAddr); // Leon's .exe will use this, m_curIpAddr
	}
	// Now the username... - from the dialog's top username box
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

	// An authenticating username was present so set it
	if (bUsrAuthenticate)
	{
		m_strNormalUsername = strUsername;
		m_pApp->UpdateCurNormalUsername(strUsername); // Leon's .exe will use this
#if defined(_DEBUG)
		wxLogDebug(_T("Authenticate2 Dlg username1: %s , in local: m_strNormalUsername"),
			m_strNormalUsername.c_str());
#endif
		// check for a 'look for user2 name', store it for external access
		m_strNormalUsername2 = m_pUsername2Ctrl->GetValue();
		if (!m_strNormalUsername2.IsEmpty())
		{
			m_pApp->UpdateUsername2(m_strNormalUsername2); // for app's m_curNormalUsername2 string

#if defined(_DEBUG)
			wxLogDebug(_T("Authenticate2 Dlg m_strNormalUsername2: %s , in app: m_curNormalUsername2"),
				m_strNormalUsername2.c_str());
#endif
		}
	} // end of TRUE block for test: if (bUsrAuthenticate)
/*  BEW 13Nov20, don't need this block any more I think
	else
	{
		m_strForManagerUsername = strUsername; // LHS is local
		m_pApp->UpdateCurAuthUsername(strUsername); // Leon's .exe will use this, m_curAuthUsername
#if defined(_DEBUG)
		wxLogDebug(_T("Authenticate2 Dlg  username1: %s , in local: m_strForManagerUsername"),
			m_strForManagerUsername.c_str());
#endif
		// Get the lower 'search for' username (user2)
		m_strForManagerUsername2 = m_pUsername2Ctrl->GetValue();
		m_pApp->UpdateUsername2(m_strForManagerUsername2); // overwrites m_Username2
#if defined(_DEBUG)
		wxLogDebug(_T("Authenticate2 Dlg m_strForManagerUsername2: %s , in app: m_Username2"),
			m_strForManagerUsername2.c_str());
#endif
	} // end of else block for test: if (bUsrAuthenticate)
*/
	// Get the server password that was typed. If the password box is left empty, 
	// then authentication fails. (The password goes with username1)
	wxString pwd = m_pPasswordCtrl->GetValue();
	if (pwd.IsEmpty())
	{
		// User may have left it empty, so pwd would be an empty string. Empty string
		// will absolutely clobber the Authentication attempt. We may be able to
		// help him/her out by trying passwords already used and stored; we may hit on
		// a winner. Show it obfuscated a bit. Eg, instead of "Clouds2093", show it
		// as "Cl******93" - it may prompt user recollection, or explain why it failed
		wxString testPwd = wxEmptyString;
		if (m_pApp->m_bUseForeignOption)
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
		} // end of TRUE block for test: if (bUsrAuthenticate == FALSE)
		else
		{
			testPwd = m_saveNormalPassword; // local one set from 
											// calling frame's GetKBSvrPassword()
			if (testPwd.IsEmpty())
			{
				testPwd = m_pApp->m_curNormalPassword; // try this location
			}
		} // end of else block for test: if (bUsrAutenticate == FALSE)

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
			if (m_pApp->m_bUseForeignOption)
			{
				m_pApp->m_curAuthPassword = wxEmptyString;
			}
			// BEW 13Nov20, never never clobber the normal password on the user unawares
			//else
			//{
			//	m_pApp->m_curNormalPassword = wxEmptyString;
			//}
		} // end of TRUE block for test: if (m_pApp->m_bUseForeignOption)
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
		}  // end of else block for test: if (pwd.IsEmpty())
	} // end of TRUE block for for test of this kind: if (pwd.IsEmpty()) (line 302)
	else
	{
		// User provided the pwd, so make use of it
		if (bUsrAuthenticate)
		{
			m_strNormalPassword = pwd; // LHS is the local variable
			// Must also set it on the frame, as setting up the KbServer class will
			// want it from the stored pwd there
			m_pApp->GetMainFrame()->SetKBSvrPassword(pwd); // legacy KbServer.cpp code wants this
			if (!m_pApp->m_bUseForeignOption)
			{
				// normal adapting...
				m_pApp->UpdateCurNormalPassword(pwd); // Leon's .exe will use this, m_curNormalPassword
			}
			else
			{
				// KB Sharing Mgr access
				m_strForManagerPassword = pwd; // LHS is the local variable
				m_pApp->UpdateCurAuthPassword(pwd); // Leon's .exe will use this, m_curAuthPassword
				m_pApp->m_strForeignPassword = pwd; // another storage location
													// to save in as well
			}

			// Also store it in m_possibleNormalPassword, in case ConfigureMovedDatFile() changes
			// m_bUserAuthenticating to TRUE, if so, the this stored value should be re-stored
			// in the frame member, so subsequent code can pick it up as needed for 'Normal' actions
			m_pApp->m_possibleNormalPassword = pwd;
		}
	}
	myevent.Skip(); // the dialog will be exited now
}

void Authenticate2Dlg::OnCancel(wxCommandEvent& myevent)
{
	myevent.Skip();
}

#endif
