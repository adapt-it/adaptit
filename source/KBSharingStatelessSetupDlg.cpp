/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharingStatelessSetupDlg.cpp
/// \author			Bruce Waters
/// \date_created	7 October 2013
/// \rcs_id $Id: KBSharingStatelessSetupDlg.cpp 3028 2013-01-15 11:38:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the KBSharingStatelessSetupDlg class.
/// The KBSharingStatelessSetupDlg class provides a dialog for turning on KB sharing, in a
/// context where no connection to any local KB is wanted, and the username can be anyone
/// listed for the kbserver with given url.
/// The dialog is not used in the legacy way, but rather as stateless, when
/// authenticating someone-or-other (an administrator typically) to use
/// the Knowledge Base Sharing Manager GUI - available from the Administrator menu, to
/// add,edit or remove users of the kbserver, and/or add or edit definitions for shared
/// KBs, etc. A flag m_bStateless is always TRUE when this class is instantiated.
/// Note: This stateless instantiation creates a stateless instance of KbServer on the
/// heap, in the creator, and deletes it in the destructor. This instance of KbServer
/// supplies needed resources for the Manager GUI, but makes no assumptions about whether
/// or not any kb definitions are in place, and no assumptions about any projects being
/// previously made sharing ones, or not.
/// \derivation		The KBSharingStatelessSetupDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KBSharingStatelessSetupDlg.h"
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
#include "Adapt_It.h"
#include "MainFrm.h"
#include "helpers.h"
#include "KbServer.h"
#include "KBSharingStatelessSetupDlg.h"

// event handler table
BEGIN_EVENT_TABLE(KBSharingStatelessSetupDlg, AIModalDialog)
	EVT_INIT_DIALOG(KBSharingStatelessSetupDlg::InitDialog)
	EVT_BUTTON(wxID_OK, KBSharingStatelessSetupDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, KBSharingStatelessSetupDlg::OnCancel)
END_EVENT_TABLE()

// The stateless contructor (default, internally sets m_bStateless to TRUE)
KBSharingStatelessSetupDlg::KBSharingStatelessSetupDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Manage Knowledge Base Sharing"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{

	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	kb_sharing_stateless_setup_func(this, TRUE, TRUE);
	// The declaration is: functionname( wxWindow *parent, bool call_fit, bool set_sizer );
	m_pApp = &wxGetApp();
	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	m_bStateless = TRUE;
	m_strStatelessUsername.Empty();
	m_strStatelessURL.Empty();
	m_strStatelessPassword.Empty();
#if defined(_DEBUG)
	// For my use, to save time getting authenticated to kbserver.jmarsden.org, I'll
	// initialize to me and his server here, for the debug build, & my personal pwd too
	m_strStatelessUsername = _T("bruce_waters@sil.org");
	m_strStatelessURL = _T("https://kbserver.jmarsden.org");
	// m_strStatelessPassword <- similarly set to my personal password in Mainfrm.cpp
	// line 2630, within GetKBSvrPasswordFromUser()
#endif
	// This contructor doesn't make any attempt to link to any CKB instance, or to support
	// only the hardware's current user; params are in t whichType (we use 1, for an adapting
	// type even though type is internally irrelevant for the stateless one), 
	// and bool bStateless, which must be TRUE -- since KbServer class has the bool
	// m_bStateless member also
	m_pStatelessKbServer = new KbServer(1, TRUE); 
}

KBSharingStatelessSetupDlg::~KBSharingStatelessSetupDlg() // destructor
{
	// The stateless KbServer instance that was being used has already been freed in
	// either OnOK() or OnCancel() of the KBSharingMgrTabbedDlg instance, which was
	// destroyed before this present destructor gets called
}

void KBSharingStatelessSetupDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_saveOldURLStr = m_pApp->m_strKbServerURL; // save existing value (could be empty)
	m_saveOldUsernameStr = m_pApp->m_strUserID; // ditto

	m_savePassword = m_pApp->GetMainFrame()->GetKBSvrPassword(); // might be empty
	// It's a project which has sharing turned on, if one or both of the adapting or
	// glossing KBs has been designated as for sharing
	m_saveIsONflag = m_pApp->m_bIsKBServerProject || m_pApp->m_bIsGlossingKBServerProject;

	m_pURLCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SERVER_URL_STATELESS);
	m_pUsernameCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_USERNAME_STATELESS);
    // BEW added 14Jul13, When m_bStateless is TRUE, we want anybody (e.g. the user's
	// administrator or project supervisor) to be able to grab his machine, open the
	// Knowledge Base Sharing Manager GUI and do things like add users, create kb
	// definitions, and so forth without having to impersonate the actual machine owner,
	// and without any project yet having become a KB Sharing one
	
	// If the app members have values for the url and username already (from having been
	// just set earlier, or from the project config file, then reinstate them so that if
	// the kb sharing was turned off it can be quickly re-enabled
	// When running statelessly, don't try initialize these boxes, they start off empty
	if (m_bStateless)
	{
		wxString emptyStr = _T("");
		m_pURLCtrl->ChangeValue(emptyStr);
		m_pUsernameCtrl->ChangeValue(emptyStr);

#if defined(_DEBUG)
		// Simplify my life during development
		m_pURLCtrl->ChangeValue(m_strStatelessURL);
		m_pUsernameCtrl->ChangeValue(m_strStatelessUsername);
#endif
	}
}

void KBSharingStatelessSetupDlg::OnOK(wxCommandEvent& myevent)
{
	wxString strURL;
	strURL = m_pURLCtrl->GetValue();
	if (strURL.IsEmpty())
	{
		wxString msg = _("The URL text box at the top is empty, please type the address of the server.");
		wxString title = _("Type URL");
		wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
		return;
	}
	// We now have something in the URL box -- it might not be a valid URL, but we'll try
	// it; And since we are running stateless, then we don't store the URL in the app, but
	// only in a public member m_strStatelessURL, of this class, where (after the dialog
	// is closed) it can be grabbed by the stateless KbServer instance, and the KB Sharing
	// Manager's GUI before the dialog's members are destroyed
	m_strStatelessURL = strURL;

#if defined(_DEBUG)
		wxLogDebug(_T("KBSharingStatelessSetupDlg.cpp m_strStatelessURL = %s"), m_strStatelessURL.c_str());
#endif

    // If running stateless, the username box is read-write enabled, and should
    // start off empty
	wxString strUsername;
	strUsername = m_pUsernameCtrl->GetValue();
	if (strUsername.IsEmpty() || (strUsername == _T("****")))
	{
		wxString msg = _("The username text box is empty or its contents invalid.\nPlease type a username known to this server.");
		wxString title = _("Type correct username");
		wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
		return;
	}
	m_strStatelessUsername = strUsername;
#if defined(_DEBUG)
		wxLogDebug(_T("KBSharingSetupDlg.cpp m_strStatelessUsername = %s"), m_strStatelessUsername.c_str());
#endif

	// Get the server password. Returns an empty string if nothing is typed, or if the
	// user Cancels from the dialog. m_bStateless being TRUE, the password will be the one
	// belonging to the administrator currently using the KB Sharing Manager GUI, not the
	// owner of the machine who will later do adapting work
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	wxString pwd = pFrame->GetKBSvrPasswordFromUser();

	if (m_bStateless)
	{
		if (!pwd.IsEmpty())
		{
			m_strStatelessPassword = pwd; // store it publicly only in this class's instance
#if defined(_DEBUG)
		wxLogDebug(_T("KBSharingSetupDlg.cpp m_strStatelessPassword = %s"), m_strStatelessPassword.c_str());
#endif

            // Since a password has now been typed, we can check if this username is listed
            // in the user table. If he isn't, he cannot authenticate and so is denied
            // access to the KB Sharing Manager GUI

			// The following call will set up a temporary instance of the adapting KbServer in
			// order to call it's LookupUser() member, to check that this user has an entry in
			// the entry table; and delete the temporary instance before returning
			bool bUserIsValid = CheckForValidUsernameForKbServer(m_strStatelessURL,
											m_strStatelessUsername, m_strStatelessPassword);
			if (!bUserIsValid)
			{
				// Access to the Manager GUI is denied to this user
				m_pApp->LogUserAction(_T("Stateless Kbserver user is unknown; in OnOK() of KBSharingStatelessSetupDlg.cpp"));
				wxString msg = _("The username ( %s ) is not in the list of users for this knowledge base server.\nYou are not permitted to access the Knowledge Base Sharing Manager dialog.\nAsk your kbserver administrator to do it for you. Click Cancel to continue working.");
				msg = msg.Format(msg, m_strStatelessUsername.c_str());
				wxMessageBox(msg, _("Unknown username"), wxICON_WARNING | wxOK);
				return;
			}
			else
			{
				// Once control reaches here, we have a valid URL, we have accessed the
				// server once to verify that the user specified by m_strStatelessUsername
				// is listed in that particular kbserver, we have an instance of KbServer
				// ready for use - and it knows it is stateless. So opening the manager
				// can now be done -- but first, get the URL and password and username
				// into the stateless KbServer instance we've created on the heap using
				// the creator function for the present class's instance
				wxASSERT(m_pStatelessKbServer->m_bStateless);
				m_pStatelessKbServer->SetKBServerPassword(m_strStatelessPassword);
				m_pStatelessKbServer->SetKBServerUsername(m_strStatelessUsername);
				m_pStatelessKbServer->SetKBServerURL(m_strStatelessURL);
			}
		}
	} // end of TRUE block for test: if (m_bStateless)

	myevent.Skip(); // the dialog will be exited now
}

void KBSharingStatelessSetupDlg::OnCancel(wxCommandEvent& myevent)
{
	myevent.Skip();
}

#endif