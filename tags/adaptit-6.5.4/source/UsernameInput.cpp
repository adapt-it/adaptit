/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			UsernameInput.h
/// \author			Bruce Waters
/// \date_created	28 May 2013
/// \rcs_id $Id: Username.cpp 3254 2013-05-28 03:55:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the UsernameInputDlg class. 
/// The UsernameInputDlg class presents the user with a dialog with two wxTextCtrl fields:
/// the top one is for a usernameID (preferably unique, such as a full email address); the
/// lower one is for an informal username that is more human friendly, such as "bruce
/// waters". These are used in DVCS and KB Sharing features, at a minimum. Invoked from
/// within any project, but the names, once set, apply to all projects (but can be
/// changed, but the new version of either still applies to all projects)
/// \derivation		The UsernameInputDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "UsernameInputDlg.h"
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

// other includes
//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
//#include <wx/valgen.h> // for wxGenericValidator
#include "Adapt_It.h"
#include "UsernameInput.h"
//#include "Adapt_ItView.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(UsernameInputDlg, AIModalDialog)
	EVT_INIT_DIALOG(UsernameInputDlg::InitDialog)
	EVT_BUTTON(wxID_OK, UsernameInputDlg::OnOK)
END_EVENT_TABLE()


UsernameInputDlg::UsernameInputDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Username Input (applies to all projects)"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// The dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	UsernameInputFunc(this, TRUE, TRUE);
	// The declaration is: UsernameInputFunc( wxWindow *parent, bool call_fit, bool set_sizer );

	// other attribute initializations
}

UsernameInputDlg::~UsernameInputDlg() // destructor
{

}

void UsernameInputDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
    
	CAdapt_ItApp*   pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

    wxString        localUserID = pApp->m_strUserID;
    wxString        localUsername = pApp->m_strUsername;

	pUsernameMsgTextCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_USERNAME_MSG);
	pUsernameMsgTextCtrl->SetBackgroundColour(pApp->sysColorBtnFace);
	pUsernameTextCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_USERNAME);
	// and for the informal username text box
	pInformalUsernameTextCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_USERNAME_INFORMAL);

	usernameMsgTitle = _T("Warning: No Unique Username");
	usernameMsg = _T("You must supply a username in the Unique Username text box.");
	usernameInformalMsgTitle = _T("Warning: No Informal Username");
	usernameInformalMsg = _T("You must supply an informal username in the Informal Username text box.\nWhat you type will not be made public.\nA false name is acceptable if your co-workers know it.");

    // Transfer the m_strUserID username string (loaded from basic config file before
    // InitDialog() is called) to the pUsernameTextCtrl textbox where the user can do
    // nothing if it is correct, type something different if necessary, or if the box is
	// empty, add a (preferably unique) string for the first time. If "****" is currently
	// set (that's the NOOWNER #define), then convert it to an empty string first. We
	// don't want users to see **** in the GUI anywhere.
    
	if ( localUserID == NOOWNER )  localUserID.Empty();
	pUsernameTextCtrl->ChangeValue(localUserID); // note, it could be empty (& if the
							// user leaves it empty, the dialog will persist; the only way
							// round that is to instead Cancel)
	// Likewise, transfer the m_strUsername informal human-readable name to the
	// pInformalUsernameTextCtrl edit box - box has to have a value before the dialog can
	// be closed, or user must Cancel. Convert **** to empty string if the former is current.
    
	if ( localUsername == NOOWNER )  localUsername.Empty();
	pInformalUsernameTextCtrl->ChangeValue(localUsername);
	
	// Set focus on the unique username text box. Otherwise the read-only control gets
	// focus on some systems/platforms.
	pUsernameTextCtrl->SetFocus();	// whm added 22Oct2013, 
}

void UsernameInputDlg::OnOK(wxCommandEvent& event)
{
//	CAdapt_ItApp* pApp = &wxGetApp();
//	wxASSERT(pApp != NULL);

    // Don't allow dialog dismissal if the username textctrl is empty , or **** is there;
    // but if not so then set m_strUserID to what it contains.
	wxString strBox1 = pUsernameTextCtrl->GetValue();
	if ( strBox1.IsEmpty() || strBox1 == NOOWNER )
	{
		wxMessageBox(usernameMsg, usernameMsgTitle, wxICON_WARNING | wxOK);
		return;
	}
	else
	{
		m_finalUsername = pUsernameTextCtrl->GetValue();
	}

    // Don't allow dialog dismissal if the informal username textctrl is empty, or **** is
    // there; but if not so then set m_strUsername to what it contains
	wxString strBox2 = pInformalUsernameTextCtrl->GetValue();
	if ( strBox2.IsEmpty() || strBox2 == NOOWNER )
	{
		wxMessageBox(usernameInformalMsg, usernameInformalMsgTitle, wxICON_WARNING | wxOK);
		return;
	}
	else
	{
		m_finalInformalUsername = pInformalUsernameTextCtrl->GetValue();
	}
	event.Skip();
}

// other class methods

