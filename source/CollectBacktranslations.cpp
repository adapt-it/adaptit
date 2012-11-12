/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CollectBacktranslations.cpp
/// \author			Bill Martin
/// \date_created	173 June 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CCollectBacktranslations class. 
/// The CCollectBacktranslations class allows the user to collect back translations across
/// the whole document using either the adaptation text or the glossing text, placing the
/// back translation within filtered \bt markers.
/// \derivation		The CCollectBacktranslations class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in CollectBacktranslations.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "CollectBacktranslations.h"
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
#include "CollectBacktranslations.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CCollectBacktranslations, AIModalDialog)
	EVT_INIT_DIALOG(CCollectBacktranslations::InitDialog)
	EVT_BUTTON(wxID_OK, CCollectBacktranslations::OnOK)
END_EVENT_TABLE()


CCollectBacktranslations::CCollectBacktranslations(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Collect Text As Back Translations"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	CollectBackTranslationsDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning (retain this line unchanged)
	m_bUseAdaptations = FALSE;
	m_bUseGlosses = FALSE;

	// other attribute initializations
}

CCollectBacktranslations::~CCollectBacktranslations() // destructor
{
	
}

void CCollectBacktranslations::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	// get the button pointers
	wxRadioButton* pAdaptationsBtn = (wxRadioButton*)FindWindowById(IDC_RADIO_COLLECT_ADAPTATIONS);
	wxRadioButton* pGlossesBtn = (wxRadioButton*)FindWindowById(IDC_RADIO_COLLECT_GLOSSES);

	wxTextCtrl* pTextCtrlAsStaticCollectBT = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_COLLECT_BT);
	wxASSERT(pTextCtrlAsStaticCollectBT != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	//pTextCtrlAsStaticCollectBT->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticCollectBT->SetBackgroundColour(gpApp->sysColorBtnFace);

	// set the adaptations radio button on as the default
	m_bUseAdaptations = TRUE;
	pAdaptationsBtn->SetValue(m_bUseAdaptations);
	m_bUseGlosses = FALSE;
	pGlossesBtn->SetValue(m_bUseGlosses);


}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CCollectBacktranslations::OnOK(wxCommandEvent& event) 
{
	wxRadioButton* pAdaptationsBtn = (wxRadioButton*)FindWindowById(IDC_RADIO_COLLECT_ADAPTATIONS);
	m_bUseAdaptations = (bool)pAdaptationsBtn->GetValue();
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

