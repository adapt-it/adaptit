/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharing.h
/// \author			Bruce Waters
/// \date_created	14 January 2013
/// \rcs_id $Id: KBSharing.h 3025 2013-01-14 18:18:00Z jmarsden6@gmail.com $
/// \copyright		2013 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the KBSharing class. 
/// The KBSharing class provides a dialog for the turning on or off KB Sharing, and for
/// controlling the non-automatic functionalities within the KB sharing feature.
/// \derivation		The KBSharing class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in KBSharing.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KBSharing.h"
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
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include "Adapt_It.h"
#include "KBSharing.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(KBSharing, AIModalDialog)
	EVT_INIT_DIALOG(KBSharing::InitDialog)
	EVT_BUTTON(wxID_OK, KBSharing::OnOK)
	EVT_BUTTON(wxID_CANCEL, KBSharing::OnCancel)
	EVT_BUTTON(ID_GET_ALL, KBSharing::OnBtnGetAll)

END_EVENT_TABLE()


KBSharing::KBSharing(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Knowledge Base Sharing"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{

	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	kb_sharing_dlg_func(this, TRUE, TRUE);
	// The declaration is: GoToDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning

}

KBSharing::~KBSharing() // destructor
{
	
}

void KBSharing::OnOK(wxCommandEvent& myevent) 
{
	myevent.Skip();
}

void KBSharing::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_pBtnGetAll = (wxButton*)FindWindowById(ID_GET_ALL);

}

void KBSharing::OnCancel(wxCommandEvent& myevent) 
{
	myevent.Skip();
}

void KBSharing::OnBtnGetAll(wxCommandEvent& WXUNUSED(event))
{
	// TODO  put the kbserver code for a changedsince from time 0:0:0 here
	//int i = 1;
}




