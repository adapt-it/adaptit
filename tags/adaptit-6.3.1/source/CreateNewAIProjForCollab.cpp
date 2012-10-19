/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CreateNewAIProjForCollab.cpp
/// \author			Bill Martin
/// \date_created	23 February 2012
/// \date_revised	23 February 2012
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CCreateNewAIProjForCollab class. 
/// The CCreateNewAIProjForCollab class implements a simple dialog that allows the user to
/// enter the source language and target language names that are to be used for a new Adapt It
/// project.
/// \derivation		The CCreateNewAIProjForCollab class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in CreateNewAIProjForCollab.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "CreateNewAIProjForCollab.h"
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
#include "Adapt_It.h"
#include "CreateNewAIProjForCollab.h"
#include "LanguageCodesDlg.h"

// event handler table
BEGIN_EVENT_TABLE(CCreateNewAIProjForCollab, AIModalDialog)
	EVT_INIT_DIALOG(CCreateNewAIProjForCollab::InitDialog)
	EVT_TEXT(ID_TEXTCTRL_SRC_LANG_NAME, CCreateNewAIProjForCollab::OnEnChangeSrcLangName)
	EVT_TEXT(ID_TEXTCTRL_TGT_LANG_NAME, CCreateNewAIProjForCollab::OnEnChangeTgtLangName)
	EVT_BUTTON(ID_BUTTON_LOOKUP_CODES, CCreateNewAIProjForCollab::OnBtnLookupCodes)
	EVT_BUTTON(wxID_OK, CCreateNewAIProjForCollab::OnOK)
END_EVENT_TABLE()

CCreateNewAIProjForCollab::CCreateNewAIProjForCollab(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Provide Language Names for New Adapt It Project Creation"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pCreateNewAIProjForCollabSizer = CreateNewAIProjForCollabFunc(this, FALSE, TRUE); // second param FALSE enables resize
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_pApp = &wxGetApp();
	
	wxColour sysColorBtnFace; // color used for read-only text controls displaying
	// color used for read-only text controls displaying static text info button face color
	sysColorBtnFace = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	
	pStaticTextTopInfoLine1 = (wxStaticText*)FindWindowById(ID_TEXT_TOP_INFO_1);
	wxASSERT(pStaticTextTopInfoLine1 != NULL);
	
	pStaticTextTopInfoLine2 = (wxStaticText*)FindWindowById(ID_TEXT_TOP_INFO_2);
	wxASSERT(pStaticTextTopInfoLine2 != NULL);
	
	pTextCtrlSrcLangName = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SRC_LANG_NAME);
	wxASSERT(pTextCtrlSrcLangName != NULL);

	pTextCtrlSrcLangCode = (wxTextCtrl*)FindWindowById(ID_EDIT_SOURCE_LANG_CODE);
	wxASSERT(pTextCtrlSrcLangCode != NULL);

	pTextCtrlTgtLangName = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_TGT_LANG_NAME);
	wxASSERT(pTextCtrlTgtLangName != NULL);

	pTextCtrlTgtLangCode = (wxTextCtrl*)FindWindowById(ID_EDIT_TARGET_LANG_CODE);
	wxASSERT(pTextCtrlTgtLangCode != NULL);

	pBtnLookupCodes = (wxButton*)FindWindowById(ID_BUTTON_LOOKUP_CODES);
	wxASSERT(pBtnLookupCodes != NULL);

	pTextCtrlNewAIProjName = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_NEW_AI_PROJ_NAME);
	wxASSERT(pTextCtrlNewAIProjName != NULL);
	pTextCtrlNewAIProjName->SetBackgroundColour(sysColorBtnFace);

}

CCreateNewAIProjForCollab::~CCreateNewAIProjForCollab() // destructor
{
	
}

void CCreateNewAIProjForCollab::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
	//InitDialog() is not virtual, no call needed to a base class
	wxString infoLine1,infoLine2;
	infoLine1 = pStaticTextTopInfoLine1->GetLabel();
	infoLine1 = infoLine1.Format(infoLine1,m_pApp->m_collaborationEditor.c_str());
	pStaticTextTopInfoLine1->SetLabel(infoLine1);

	infoLine2 = pStaticTextTopInfoLine2->GetLabel();
	infoLine2 = infoLine2.Format(infoLine2,m_pApp->m_collaborationEditor.c_str());
	pStaticTextTopInfoLine2->SetLabel(infoLine2);

	pCreateNewAIProjForCollabSizer->Layout();
	// The second radio button's label text is likely going to be truncated unless we resize the
	// dialog to fit it. Note: The constructor's call of CreateNewAIProjForCollabFunc(this, FALSE, TRUE)
	// has its second parameter as FALSE to allow this resize here in InitDialog().
	wxSize dlgSize;
	dlgSize = pCreateNewAIProjForCollabSizer->ComputeFittingWindowSize(this);
	this->SetSize(dlgSize);
	this->CenterOnParent();
}

// event handling functions

void CCreateNewAIProjForCollab::OnEnChangeSrcLangName(wxCommandEvent& WXUNUSED(event))
{
	// user is editing the source language name edit box
	// update the AI project name in the "New Adapt It project name will be:"
	// edit box
	wxString tempStrSrcProjName,tempStrTgtProjName;
	tempStrSrcProjName = pTextCtrlSrcLangName->GetValue();
	tempStrTgtProjName = pTextCtrlTgtLangName->GetValue();
	wxString projFolder = tempStrSrcProjName + _T(" to ") + tempStrTgtProjName + _T(" adaptations");
	pTextCtrlNewAIProjName->ChangeValue(projFolder);
}

void CCreateNewAIProjForCollab::OnEnChangeTgtLangName(wxCommandEvent& WXUNUSED(event))
{
	// user is editing the target language name edit box
	// update the AI project name in the "New Adapt It project name will be:"
	// edit box
	wxString tempStrSrcProjName,tempStrTgtProjName;
	tempStrSrcProjName = pTextCtrlSrcLangName->GetValue();
	tempStrTgtProjName = pTextCtrlTgtLangName->GetValue();
	wxString projFolder = tempStrSrcProjName + _T(" to ") + tempStrTgtProjName + _T(" adaptations");
	pTextCtrlNewAIProjName->ChangeValue(projFolder);
}

void CCreateNewAIProjForCollab::OnBtnLookupCodes(wxCommandEvent& WXUNUSED(event))
{
	CLanguageCodesDlg lcDlg(this); // make the CLanguagesPage the parent in this case
	lcDlg.Center();
	// initialize the language code edit boxes with the values currently in
	// the LanguagePage's edit boxes (which InitDialog initialized to the current 
	// values on the App, or which the user manually edited before pressing the 
	// Lookup Codes button).
	// 
    // BEW additional comment of 25Jul12, for xhtml exports we support not just src and tgt
    // language codes, but also language codes for glosses language, and free translation
    // language - all four languages are independently settable. However, while all four
    // can be set by repeated invokations of the Lookup Codes button, when setting up a new
    // Adapt It project for use in a collaboration with Paratext or Bibledit, only the
    // source and target languages are relevant, and so we here pick up and store only the
    // codes for either or both of these languages. To set codes for glosses language,
    // and/or free translation language, go to the Backups and Misc page of the Preferences
    // -- settings made there are remembered, and all four are saved to the basic and
    // project configuration files - whether the document is saved or not on closure -- set
    // up a free translation language and code there, before doing this collaboration
    // project setup, for best results
	lcDlg.m_sourceLangCode = pTextCtrlSrcLangCode->GetValue();
	lcDlg.m_targetLangCode = pTextCtrlTgtLangCode->GetValue();
	int returnValue = lcDlg.ShowModal();
	if (returnValue == wxID_CANCEL)
	{
		// user cancelled
		return;
	}
	// transfer language codes to the edit box controls and the App's members
	pTextCtrlSrcLangCode->SetValue(lcDlg.m_sourceLangCode);
	pTextCtrlTgtLangCode->SetValue(lcDlg.m_targetLangCode);
	m_pApp->m_sourceLanguageCode = lcDlg.m_sourceLangCode;
	m_pApp->m_targetLanguageCode = lcDlg.m_targetLangCode;
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CCreateNewAIProjForCollab::OnOK(wxCommandEvent& event) 
{
	// Check for empty values (user hit OK without entering at least a value for the
	// source and target language names. If one or both is empty, notify user and cancel
	if (pTextCtrlSrcLangName->GetValue().IsEmpty()) 
	{
		wxString msg = _("Please enter a name for the source language, or Cancel to quit.");
		wxMessageBox(msg,_T(""),wxICON_EXCLAMATION | wxOK);
		pTextCtrlSrcLangName->SetFocus();
		return;
	}

	if (pTextCtrlTgtLangName->GetValue().IsEmpty())
	{
		wxString msg = _("Please enter a name for the target language, or Cancel to quit.");
		wxMessageBox(msg,_T(""),wxICON_EXCLAMATION | wxOK);
		pTextCtrlTgtLangName->SetFocus();
		return;
	}

	// whm Note: The caller (SetupEditorCollaboration.cpp) retrieves the
	// values from the source and target lang name wxTextCtrl controls
	// after OnOK() is invoked.
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

