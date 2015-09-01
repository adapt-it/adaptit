/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ChooseConsistencyCheckTypeDlg.cpp
/// \author			Bill Martin
/// \date_created	11 July 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CChooseConsistencyCheckTypeDlg class.
/// The CChooseConsistencyCheckTypeDlg class puts up a dialog for the user to indicate
/// whether the consistency check should be done on only the current document or also on
/// other documents in the current project.
/// \derivation		The CChooseConsistencyCheckTypeDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ChooseConsistencyCheckTypeDlg.cpp (in order of importance): (search for "TODO")
// 1.
//
// Unanswered questions: (search for "???")
// 1.
//
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ChooseConsistencyCheckTypeDlg.h"
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
#include "ChooseConsistencyCheckTypeDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CChooseConsistencyCheckTypeDlg, AIModalDialog)
	EVT_INIT_DIALOG(CChooseConsistencyCheckTypeDlg::InitDialog)
	EVT_RADIOBUTTON(IDC_RADIO_CHECK_OPEN_DOC_ONLY, CChooseConsistencyCheckTypeDlg::OnBnClickedRadioCheckOpenDocOnly)
	EVT_RADIOBUTTON(IDC_RADIO_CHECK_SELECTED_DOCS, CChooseConsistencyCheckTypeDlg::OnBnClickedRadioCheckSelectedDocs)
	EVT_BUTTON(wxID_OK, CChooseConsistencyCheckTypeDlg::OnOK)
	END_EVENT_TABLE()


CChooseConsistencyCheckTypeDlg::CChooseConsistencyCheckTypeDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Consistency Check"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	ChooseConsistencyCheckTypeDlgFunc(this, TRUE, TRUE);
	// The declaration is: ChooseConsistencyCheckTypeDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	wxUnusedVar(bOK); // avoid warning
	wxColor backgrndColor = this->GetBackgroundColour();

	pTextCtrlAsStaticChooseConsChkType = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_MSG);
	pTextCtrlAsStaticChooseConsChkType->SetBackgroundColour(gpApp->sysColorBtnFace);

	pTextCtrlAsStaticBlindFixes = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_MSG2);
	pTextCtrlAsStaticBlindFixes->SetBackgroundColour(gpApp->sysColorBtnFace);

	pCheckBoxBlindFixes = (wxCheckBox*)FindWindowById(ID_CHECKBOXBLIND_FIXES);
	pCheckBoxBlindFixes->SetValue(FALSE);
}

CChooseConsistencyCheckTypeDlg::~CChooseConsistencyCheckTypeDlg() // destructor
{

}

void CChooseConsistencyCheckTypeDlg::OnOK(wxCommandEvent& WXUNUSED(event))
{
	bool value = pCheckBoxBlindFixes->GetValue();
	gpApp->m_bBlindFixInConsCheck = value; // pass user's choice back to the app
}

void CChooseConsistencyCheckTypeDlg::OnBnClickedRadioCheckOpenDocOnly(wxCommandEvent& WXUNUSED(event))
{
	// Make "Check this document only" the default selection
	wxRadioButton* pRadio = (wxRadioButton*)FindWindowById(IDC_RADIO_CHECK_OPEN_DOC_ONLY);
	wxASSERT(pRadio != NULL);
	m_bCheckOpenDocOnly = TRUE;
	pRadio->SetValue(TRUE);
}

void CChooseConsistencyCheckTypeDlg::OnBnClickedRadioCheckSelectedDocs(wxCommandEvent& WXUNUSED(event))
{
	// Make "Check this document only" the default selection
	wxRadioButton* pRadio = (wxRadioButton*)FindWindowById(IDC_RADIO_CHECK_OPEN_DOC_ONLY);
	wxASSERT(pRadio != NULL);
	m_bCheckOpenDocOnly = FALSE;
	pRadio->SetValue(FALSE);
}

// BEW 22Aug14, refactored to prevent choosing anything but the current doc being checked
// when in collaboration mode with PT or BE
void CChooseConsistencyCheckTypeDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	// Make "Check this document only" the default selection
	wxRadioButton* pRadio = (wxRadioButton*)FindWindowById(IDC_RADIO_CHECK_OPEN_DOC_ONLY);
	wxASSERT(pRadio != NULL);

	wxRadioButton* pRadioAll = (wxRadioButton*)FindWindowById(IDC_RADIO_CHECK_SELECTED_DOCS);
	wxASSERT(pRadioAll != NULL);
	wxUnusedVar(pRadioAll);

	wxTextCtrl* pTextCtrlMsg = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_MSG);
	wxASSERT(pTextCtrlMsg != NULL);
	wxUnusedVar(pTextCtrlMsg);

	/*
	wxTextCtrl* pTextCtrlAsStaticChooseConsChkType = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_CHOOSE_CONSISTENCY_CHECK_TYPE);
	wxASSERT(pTextCtrlAsStaticChooseConsChkType != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	//pTextCtrlAsStaticChooseConsChkType->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticChooseConsChkType->SetBackgroundColour(gpApp->sysColorBtnFace);
	*/
	m_bCheckOpenDocOnly = TRUE;
	pRadio->SetValue(TRUE);

    // BEW 22Aug14 In collaboration mode, Adapt It must only work with a document that is
    // known to be in collaboration. Having a consistency check jump to other documents
    // which may be scripture ones, and potentially make changes, would have them become
    // out of sync with what's in PT or BE. We should avoid situations where the result of
    // allowing something has the potential to make unwelcome hassles arise consequentially.
    // BEW 6.5.9 (unreleased, but released as 6.6.0) we allow all docs to be consistency checked
    // in collab mode, because now we have detection of conflicts (and untransferred AI doc edits
    // will be seen to be conflicts) and the possibility of getting them safely transferred, so
    // comment this restriction out
	//if (gpApp->m_bCollaboratingWithParatext || gpApp->m_bCollaboratingWithBibledit)
	//{
	//	pRadioAll->Hide();
	//	pTextCtrlMsg->Hide();
	//}
}

