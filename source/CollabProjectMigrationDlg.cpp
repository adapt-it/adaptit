/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CollabProjectMigrationDlg.cpp
/// \author			Bill Martin
/// \date_created	5 April 2017
/// \rcs_id $Id$
/// \copyright		2017 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CCollabProjectMigrationDlg class. 
/// The CCollabProjectMigrationDlg class creates a dialog that allows a user who currently collaborates with
/// Paratext projects in Paratext 7 to migrate the Adapt It project to collaborate with the same Paratext
/// projects once they have been migrated to PT8.
/// \derivation		The CCollabProjectMigrationDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "CollabProjectMigrationDlg.h"
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
#include "Adapt_It.h"
#include "CollabProjectMigrationDlg.h"

// event handler table
BEGIN_EVENT_TABLE(CCollabProjectMigrationDlg, AIModalDialog)
EVT_INIT_DIALOG(CCollabProjectMigrationDlg::InitDialog)
EVT_BUTTON(wxID_OK, CCollabProjectMigrationDlg::OnOK)
EVT_CHECKBOX(ID_CHECKBOX_DONT_SHOW_AGAIN, CCollabProjectMigrationDlg::OnCheckDontShowAgain)
EVT_RADIOBUTTON(ID_RADIOBUTTON_PT9, CCollabProjectMigrationDlg::OnRadioBtnPT9)
EVT_RADIOBUTTON(ID_RADIOBUTTON_PT8, CCollabProjectMigrationDlg::OnRadioBtnPT8)
EVT_RADIOBUTTON(ID_RADIOBUTTON_PT7, CCollabProjectMigrationDlg::OnRadioBtnPT7)
EVT_BUTTON(wxID_CANCEL, CCollabProjectMigrationDlg::OnCancel)
END_EVENT_TABLE()

CCollabProjectMigrationDlg::CCollabProjectMigrationDlg(wxWindow* parent, wxString aiProj, wxString srcProject, wxString tgtProject, wxString freeTransProject) // dialog constructor
    : AIModalDialog(parent, -1, _("Your Paratext 7 collaboration projects were migrated to Paratext 8"),
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // This dialog function below is generated in wxDesigner, and defines the controls and sizers
    // for the dialog. The first parameter is the parent which should normally be "this".
    // The second and third parameters should both be TRUE to utilize the sizers and create the right
    // size dialog.
    CollabProjectMigrationDlgFunc(this, TRUE, TRUE);
    // The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

    // whm 5Mar2019 Note: The CollabProjectMigrationDlgFunc() dialog now uses the
    // wxStdDialogButtonSizer, and so there is no need to call the ReverseOkCancelButtonsForMac()
    // function in this case.

    pRadioBtnPT9 = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_PT9);
    wxASSERT(pRadioBtnPT9 != NULL);
    pRadioBtnPT8 = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_PT8);
    wxASSERT(pRadioBtnPT8 != NULL);
    pRadioBtnPT7 = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_PT7);
    wxASSERT(pRadioBtnPT7 != NULL);
    pStaticAICollabProj = (wxStaticText*)FindWindowById(ID_TEXT_AIPROJECT);
    wxASSERT(pStaticAICollabProj != NULL);
    pStaticTextSrcProj = (wxStaticText*)FindWindowById(ID_TEXT_SRC_PROJ);
    wxASSERT(pStaticTextSrcProj != NULL);
    pStaticTextTgtProj = (wxStaticText*)FindWindowById(ID_TEXT_TGT_PROJ);
    wxASSERT(pStaticTextTgtProj != NULL);
    pStaticTextFreeTransProj = (wxStaticText*)FindWindowById(ID_TEXT_FREE_TRANS_PROJ);
    wxASSERT(pStaticTextFreeTransProj != NULL);
    pCheckBoxDoNotShowAgain = (wxCheckBox*)FindWindowById(ID_CHECKBOX_DONT_SHOW_AGAIN);
    wxASSERT(pCheckBoxDoNotShowAgain != NULL);

    // other attribute initializations
    aiProject = aiProj;
    sourceProject = srcProject;
    targetProject = tgtProject;
    freeTranslationProject = freeTransProject;
}

CCollabProjectMigrationDlg::~CCollabProjectMigrationDlg() // destructor
{

}

void CCollabProjectMigrationDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
    //InitDialog() is not virtual, no call needed to a base class

    // Start with the "Paratext 9" button selected (and "Paratext 7" and "Paratext 8" buttons unselected)
    pRadioBtnPT9->SetValue(TRUE);
    pRadioBtnPT8->SetValue(FALSE);
    pRadioBtnPT7->SetValue(FALSE);
    m_bPT8BtnSelected = pRadioBtnPT8->GetValue();
    m_bPT9BtnSelected = pRadioBtnPT9->GetValue();

    // Start with the "Do not show this message again" checkbox selected
    pCheckBoxDoNotShowAgain->SetValue(TRUE);
    m_bDoNotShowAgain = pCheckBoxDoNotShowAgain->GetValue();

    // Enable the Do not show again checkbox only when Paratext 7 is selected
    // Here in InitDialog Paratext 9 is selected, so the Do not show again checkbox starts ticked, but disabled
    pCheckBoxDoNotShowAgain->Enable(!m_bPT8BtnSelected && !m_bPT9BtnSelected);

    wxString tempStr;

    tempStr = pStaticAICollabProj->GetLabel();
    tempStr = tempStr.Format(tempStr, aiProject.c_str());
    pStaticAICollabProj->SetLabel(tempStr);

    tempStr = pStaticTextSrcProj->GetLabel();
    tempStr = tempStr.Format(tempStr, sourceProject.c_str());
    pStaticTextSrcProj->SetLabel(tempStr);

    tempStr = pStaticTextTgtProj->GetLabel();
    tempStr = tempStr.Format(tempStr, targetProject.c_str());
    pStaticTextTgtProj->SetLabel(tempStr);

    tempStr = pStaticTextFreeTransProj->GetLabel();
    tempStr = tempStr.Format(tempStr, freeTranslationProject.c_str());
    pStaticTextFreeTransProj->SetLabel(tempStr);

}

// event handling functions

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CCollabProjectMigrationDlg::OnOK(wxCommandEvent& event)
{
    // The caller of CCollabProjectMigrationDlg (ProjectPage.cpp) will make any needed changes 
    // to the App's members when this dialog closes. The following dialog members are public
    // in order to allow the calling routine to determine the user's response if any.
    m_bPT9BtnSelected = pRadioBtnPT9->GetValue();
    m_bPT8BtnSelected = pRadioBtnPT8->GetValue();
    m_bDoNotShowAgain = pCheckBoxDoNotShowAgain->GetValue();
    event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void CCollabProjectMigrationDlg::OnCancel(wxCommandEvent& event)
{
    // User cancelled. The caller (ProjectPage.cpp) will deal with the cancellation.
    event.Skip();
}


void CCollabProjectMigrationDlg::OnRadioBtnPT9(wxCommandEvent & WXUNUSED(event))
{
    pRadioBtnPT9->SetValue(TRUE);
    pRadioBtnPT8->SetValue(FALSE);
    pRadioBtnPT7->SetValue(FALSE);
    m_bPT9BtnSelected = pRadioBtnPT9->GetValue();
    m_bPT8BtnSelected = pRadioBtnPT8->GetValue();
    // Enable the Do not show again checkbox only when Paratext 7 is selected
    // Here Paratext 9 is selected, so the Do not show again checkbox is ticked but disabled
    pCheckBoxDoNotShowAgain->SetValue(m_bPT9BtnSelected);
    m_bDoNotShowAgain = pCheckBoxDoNotShowAgain->GetValue();
    pCheckBoxDoNotShowAgain->Enable(!m_bPT8BtnSelected && !m_bPT9BtnSelected);
}

void CCollabProjectMigrationDlg::OnRadioBtnPT8(wxCommandEvent & WXUNUSED(event))
{
    pRadioBtnPT9->SetValue(FALSE);
    pRadioBtnPT8->SetValue(TRUE);
    pRadioBtnPT7->SetValue(FALSE);
    m_bPT8BtnSelected = pRadioBtnPT8->GetValue();
    m_bPT9BtnSelected = pRadioBtnPT9->GetValue();
    // Enable the Do not show again checkbox only when Paratext 7 is selected
    // Here Paratext 8 is selected, so the Do not show again checkbox is ticked but disabled
    pCheckBoxDoNotShowAgain->SetValue(m_bPT8BtnSelected);
    m_bDoNotShowAgain = pCheckBoxDoNotShowAgain->GetValue();
    pCheckBoxDoNotShowAgain->Enable(!m_bPT8BtnSelected && !m_bPT9BtnSelected);
}

void CCollabProjectMigrationDlg::OnRadioBtnPT7(wxCommandEvent & WXUNUSED(event))
{
    pRadioBtnPT9->SetValue(FALSE);
    pRadioBtnPT8->SetValue(FALSE);
    pRadioBtnPT7->SetValue(TRUE);
    m_bPT8BtnSelected = pRadioBtnPT8->GetValue();
    m_bPT9BtnSelected = pRadioBtnPT9->GetValue();
    // Enable the Do not show again checkbox only when Paratext 7 is selected
    // Here Paratext 7 is explicitly selected, so the Do not show again checkbox is unticked and enabled
    pCheckBoxDoNotShowAgain->SetValue(FALSE);
    pCheckBoxDoNotShowAgain->Enable(!m_bPT8BtnSelected && !m_bPT9BtnSelected);
}

void CCollabProjectMigrationDlg::OnCheckDontShowAgain(wxCommandEvent & WXUNUSED(event))
{
    m_bDoNotShowAgain = pCheckBoxDoNotShowAgain->GetValue();
}


// other class methods

