/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharingSetupDlg.h
/// \author			Bruce Waters
/// \date_created	15 January 2013
/// \rcs_id $Id: KBSharingSetupDlg.h 3028 2013-01-15 11:38:00Z jmarsden6@gmail.com $
/// \copyright		2013 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the KBSharingSetupDlg class. 
/// The KBSharingSetupDlg class provides two text boxes, the first for the KB server's URL,
/// the second for the username --the user's email address is recommended, but not 
/// enforced. It's best because it will be unique. The KB server's password is not
/// entered in the dialog, otherwise it would be visible. Instead, a password dialog is
/// shown when the OK button is clicked. (The password dialog appears at other times too -
/// such as when turning ON KB sharing after it has been off, or when reentering a KB
/// sharing project previously designed as one for sharing of the KB.)
/// \derivation		The KBSharingSetupDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in KBSharingSetupDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KBSharingSetupDlg.h"
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
#include "Adapt_It.h"
#include "KBSharingSetupDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(KBSharingSetupDlg, AIModalDialog)
	EVT_INIT_DIALOG(KBSharingSetupDlg::InitDialog)
	EVT_BUTTON(wxID_OK, KBSharingSetupDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, KBSharingSetupDlg::OnCancel)
	//EVT_BUTTON(ID_GET_ALL, KBSharingSetup::OnBtnGetAll)

END_EVENT_TABLE()


KBSharingSetupDlg::KBSharingSetupDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Setup Or Remove Knowledge Base Sharing"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{

	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	kb_sharing_setup_func(this, TRUE, TRUE);
	// The declaration is: functionname( wxWindow *parent, bool call_fit, bool set_sizer );
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning

}

KBSharingSetupDlg::~KBSharingSetupDlg() // destructor
{
	
}

void KBSharingSetupDlg::OnOK(wxCommandEvent& myevent) 
{
	myevent.Skip();
}

void KBSharingSetupDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	//m_pBtnGetAll = (wxButton*)FindWindowById(ID_GET_ALL);

}

void KBSharingSetupDlg::OnCancel(wxCommandEvent& myevent) 
{
	myevent.Skip();
}

/*
void KBSharingSetupDlg::OnBtnGetAll(wxCommandEvent& WXUNUSED(event))
{
	// TODO  put the kbserver code for a changedsince from time 0:0:0 here
	//int i = 1;
}
*/
