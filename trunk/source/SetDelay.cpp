/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SetDelay.cpp
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CSetDelay class. 
/// The CSetDelay class provides a dialog that allows the user to set a time delay
/// to slow down the automatic insertions of adaptations. This can be helpful to some
/// who want to read what is being inserted as it is being inserted.
/// \derivation		The CSetDelay class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in SetDelay.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "SetDelay.h"
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
#include <wx/valgen.h> // for wxGenericValidator

#include "Adapt_It.h"
#include "SetDelay.h"
#include "helpers.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CSetDelay, AIModalDialog)
	EVT_INIT_DIALOG(CSetDelay::InitDialog)
END_EVENT_TABLE()


CSetDelay::CSetDelay(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Set A Delay For Automatic Insertions"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	SetDelayDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);

	m_nDelay = 0;

	m_pDelayBox = (wxTextCtrl*)FindWindowById(IDC_SPIN_DELAY_TICKS);
	wxASSERT(m_pDelayBox != NULL);
	m_pDelayBox->SetValidator(wxGenericValidator(&m_nDelay));
}

CSetDelay::~CSetDelay() // destructor
{
	
}

void CSetDelay::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	m_nDelay = gpApp->m_nCurDelay;

	CopyFontBaseProperties(gpApp->m_pNavTextFont,gpApp->m_pDlgSrcFont);
	gpApp->m_pDlgSrcFont->SetPointSize(11); // 11 point
	gpApp->m_pDlgSrcFont->SetWeight(wxFONTWEIGHT_NORMAL); // not bold or light font
		
	TransferDataToWindow(); 
}

// event handling functions

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CSetDelay::OnOK(wxCommandEvent& event) 
{
	// update the variable m_nDelay to user-set value
	TransferDataFromWindow();
	
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

