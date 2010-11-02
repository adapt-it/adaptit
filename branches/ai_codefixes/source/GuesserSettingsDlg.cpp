/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GuesserSettingsDlg.cpp
/// \author			Bill Martin
/// \date_created	27 January 2010
/// \date_revised	27 January 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CGuesserSettingsDlg class. 
/// The CGuesserSettingsDlg class provides a dialog in which the user can change the basic
/// settings for the Guesser.
/// \derivation		The CGuesserSettingsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in GuesserSettingsDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "GuesserSettingsDlg.h"
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
//#include <wx/valtext.h> // for wxTextValidator
#include <wx/colordlg.h>
#include "Adapt_It.h"
#include "GuesserSettingsDlg.h"

// event handler table
BEGIN_EVENT_TABLE(CGuesserSettingsDlg, AIModalDialog)
	EVT_INIT_DIALOG(CGuesserSettingsDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(ID_BUTTON_GUESS_HIGHLIGHT_COLOR, CGuesserSettingsDlg::OnChooseGuessHighlightColor)
	// Samples:
	//EVT_BUTTON(wxID_OK, CGuesserSettingsDlg::OnOK)
	//EVT_MENU(ID_SOME_MENU_ITEM, CGuesserSettingsDlg::OnDoSomething)
	//EVT_UPDATE_UI(ID_SOME_MENU_ITEM, CGuesserSettingsDlg::OnUpdateDoSomething)
	//EVT_CHECKBOX(ID_SOME_CHECKBOX, CGuesserSettingsDlg::OnDoSomething)
	//EVT_RADIOBUTTON(ID_SOME_RADIOBUTTON, CGuesserSettingsDlg::DoSomething)
	//EVT_LISTBOX(ID_SOME_LISTBOX, CGuesserSettingsDlg::DoSomething)
	//EVT_COMBOBOX(ID_SOME_COMBOBOX, CGuesserSettingsDlg::DoSomething)
	//EVT_TEXT(IDC_SOME_EDIT_CTRL, CGuesserSettingsDlg::OnEnChangeEditSomething)
	// ... other menu, button or control events
END_EVENT_TABLE()

CGuesserSettingsDlg::CGuesserSettingsDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Dialog Title Here"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	GuesserSettingsDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	// use wxValidator for simple dialog data transfer
	// sample text control initialization below:
	pCheckUseGuesser = (wxCheckBox*)FindWindowById(ID_CHECK_USE_GUESSER);
	wxASSERT(pCheckUseGuesser != NULL);
	pCheckUseGuesser->SetValidator(wxGenericValidator(&bUseAdaptationsGuesser));

	pSlider = (wxSlider*)FindWindowById(ID_SLIDER_GUESSER);
	wxASSERT(pSlider != NULL);
	pSlider->SetValidator(wxGenericValidator(&nGuessingLevel));

	pAllowCCtoOperateOnUnchangedOutput = (wxCheckBox*)FindWindowById(ID_CHECK_ALLOW_GUESSER_ON_UNCHANGED_CC_OUTPUT);
	wxASSERT(pAllowCCtoOperateOnUnchangedOutput != NULL);
	pAllowCCtoOperateOnUnchangedOutput->SetValidator(wxGenericValidator(&bAllowCConUnchangedGuesserOutput));

	pStaticTextNumCorInAdaptationsGuesser = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_NUM_CORRESP_ADAPTATIONS_GUESSER);
	wxASSERT(pStaticTextNumCorInAdaptationsGuesser != NULL);

	pStaticTextNumCorInGlossingGuesser = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_NUM_CORRESP_GLOSSING_GUESSER);
	wxASSERT(pStaticTextNumCorInGlossingGuesser != NULL);

	//wxTextCtrl* pEdit;
	//pEdit = (wxTextCtrl*)FindWindowById(IDC_TEXTCONTROL);
	//pEdit->SetValidator(wxGenericValidator(&m_stringVariable));
	//pEdit->SetBackgroundColour(sysColorBtnFace);

	// sample radio button control initialization below:
	//wxRadioButton* pRadioB;
	//pRadioB = (wxRadioButton*)FindWindowById(IDC_RADIO_BUTTON);
	//pRadioB->SetValue(TRUE);
	//pRadioB->SetValidator(wxGenericValidator(&m_bVariable));

	// other attribute initializations
}

CGuesserSettingsDlg::~CGuesserSettingsDlg() // destructor
{
	
}

void CGuesserSettingsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	CAdapt_ItApp* pApp = &wxGetApp();
	//InitDialog() is not virtual, no call needed to a base class
	bUseAdaptationsGuesser = pApp->m_bUseAdaptationsGuesser;
	nGuessingLevel = pApp->m_nGuessingLevel;
	bAllowCConUnchangedGuesserOutput = pApp->m_bAllowGuesseronUnchangedCCOutput;
	pCheckUseGuesser->SetValue(bUseAdaptationsGuesser);
	pSlider->SetValue(nGuessingLevel);
	pAllowCCtoOperateOnUnchangedOutput->SetValue(bAllowCConUnchangedGuesserOutput);
	tempGuessHighlightColor = pApp->m_GuessHighlightColor;
	nCorrespondencesLoadedInAdaptationsGuesser = pApp->m_nCorrespondencesLoadedInAdaptationsGuesser;
	nCorrespondencesLoadedInGlossingGuesser = pApp->m_nCorrespondencesLoadedInGlossingGuesser;
	wxString nCAStr;
	nCAStr.Empty();
	nCAStr << nCorrespondencesLoadedInAdaptationsGuesser;
	wxString nCGStr;
	nCGStr.Empty();
	nCGStr << nCorrespondencesLoadedInGlossingGuesser;
	pStaticTextNumCorInAdaptationsGuesser->SetLabel(nCAStr);
	pStaticTextNumCorInGlossingGuesser->SetLabel(nCGStr);

}

// event handling functions

void CGuesserSettingsDlg::OnChooseGuessHighlightColor(wxCommandEvent& WXUNUSED(event))
{
	// Invoke the wxColorDlg
	wxColourData colorData;
	colorData.SetColour(tempGuessHighlightColor);
	colorData.SetChooseFull(TRUE);
	wxColourDialog colorDlg(this,&colorData);
	colorDlg.Centre();
	if(colorDlg.ShowModal() == wxID_OK)
	{
		colorData = colorDlg.GetColourData();
		tempGuessHighlightColor  = colorData.GetColour();
	}	
}

//CGuesserSettingsDlg::OnUpdateDoSomething(wxUpdateUIEvent& event)
//{
//	if (SomeCondition == TRUE)
//		event.Enable(TRUE);
//	else
//		event.Enable(FALSE);	
//}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CGuesserSettingsDlg::OnOK(wxCommandEvent& event) 
{
	CAdapt_ItApp* pApp = &wxGetApp();
	// Set the App's member values in the caller (View) after comparing them with the 
	// corresponding class values (for detecting changes made in this class).
	pApp->m_GuessHighlightColor = tempGuessHighlightColor;
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

