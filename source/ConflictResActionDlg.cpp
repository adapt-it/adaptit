/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ConflictResActionDlg.h
/// \author			Bruce Waters
/// \date_created	15 July 2015
/// \rcs_id $Id$
/// \copyright		2015 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CConflictResActionDlg class. 
/// The CConflictResActionDlg class puts up a dialog for the user to indicate
/// whether the conflict resolution should be done the legacy way (favouring keeping
/// the PT or BE verse version unchanged), or to force the AI verse version to be
/// transferred to the external editor, or to request a further dialog for the visual
/// comparison of verse versions to be shown for the user to select which is best for
/// each listed conflict. The user's response will set one of three app variables,
/// which then guide what happens subsequently.
/// \derivation		The CConflictResActionDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ConflictResActionDlg.h"
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
#include "ConflictResActionDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CConflictResActionDlg, AIModalDialog)
	EVT_INIT_DIALOG(CConflictResActionDlg::InitDialog)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_PTorBE_RETAIN, CConflictResActionDlg::OnBnClickedRadioRetainPTorBE)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_FORCE_AI_VERSE_TRANSFER, CConflictResActionDlg::OnBnClickedRadioForceAI)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_USER_CHOICE_FOR_CONFLICT_RESOLUTION, CConflictResActionDlg::OnBnClickedRadioConflictResDlg)
	EVT_BUTTON(wxID_OK, CConflictResActionDlg::OnOK)
	END_EVENT_TABLE()


CConflictResActionDlg::CConflictResActionDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Choose how to resolve conflicting versions of verses"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	ConflictResolutionActionFunc(this, TRUE, TRUE);
	// The declaration is: ConflictResolutionActionFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	wxUnusedVar(bOK); // avoid warning
	wxASSERT(!gpApp->m_collaborationEditor.IsEmpty());
	m_collabEditorName = gpApp->m_collaborationEditor; // _T("Paratext") or _T("Bibledit");

	pTopStaticSizer    = (wxStaticBoxSizer*)TopStaticSizer;
	pMiddleStaticSizer = (wxStaticBoxSizer*)MiddleStaticSizer;
	pBottomStaticSizer = (wxStaticBoxSizer*)BottomStaticSizer;
	pWholeDlgSizer = (wxBoxSizer*)CollabActionDlgSizer;
}

void CConflictResActionDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	//InitDialog() is not virtual, no call needed to a base class

	// Get the app's current setting (as set by user's choice, on a per-doc basis) for
	// having the user-directed conflict resolution dialogs shown. If turned off, they
	// will be hidden and the legacy conflict resolution protocol is done automatically,
	// which is to retain the Paratext or Bibledit version of any conflicted verse
	m_bShowingConflictResolutionDialogs = gpApp->m_bConflictResolutionTurnedOn;

	pCheckboxConflictResolutionDlgsToBeTurnedOff = (wxCheckBox*)FindWindowById(ID_CHECKBOX_TURN_OFF_CONFRES);
	wxASSERT(pCheckboxConflictResolutionDlgsToBeTurnedOff != NULL);
	// If user requests no dlgs be shown, checkbox will be ticked (by him), so
	// the value here is the opposite
	pCheckboxConflictResolutionDlgsToBeTurnedOff->SetValue(!m_bShowingConflictResolutionDialogs);

	// Top button
	pRadioRetainPT = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_PTorBE_RETAIN);
	wxASSERT(pRadioRetainPT != NULL);

	// Middle button
	pRadioForceAI = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_FORCE_AI_VERSE_TRANSFER);
	wxASSERT(pRadioForceAI != NULL);

	// Bottom button
	pRadioConflictResDlg = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_USER_CHOICE_FOR_CONFLICT_RESOLUTION);
	wxASSERT(pRadioConflictResDlg != NULL);

	m_bLegacy_retain_PTorBE_version = TRUE; // the default when first opened
	m_bForce_AI_version_transfer = FALSE;
	m_bUserWantsVisualConflictResolution = FALSE;

	// setup initial button default choice; the button group has to be managed manually
	pRadioRetainPT->SetValue(TRUE);
	pRadioForceAI->SetValue(FALSE);
	pRadioConflictResDlg->SetValue(FALSE);

	// Set the static strings in controls that need to distinguish external editor string "Paratext" or "Bibledit"
	wxASSERT(!gpApp->m_collaborationEditor.IsEmpty());
	wxASSERT(gpApp->m_collaborationEditor == _T("Paratext") || gpApp->m_collaborationEditor == _T("Bibledit"));
	// The wxDesigner resource already has "Paratext" in its string resources,
	// we need only change those to "Bibledit" if we're using Bibledit
	if (gpApp->m_collaborationEditor == _T("Bibledit"))
	{
		// Get the wxStatisBox control associated with each wxStaticBoxSizer
		wxStaticBox* pTopStatic = pTopStaticSizer->GetStaticBox();
		wxStaticBox* pMiddleStatic = pMiddleStaticSizer->GetStaticBox();
		wxStaticBox* pBottomStatic = pBottomStaticSizer->GetStaticBox();

		wxString tempStr;
		tempStr = pTopStatic->GetLabel();
		tempStr.Replace(_T("Paratext"), _T("Bibledit"));
		pTopStatic->SetLabel(tempStr);

		tempStr = pMiddleStatic->GetLabel();
		tempStr.Replace(_T("Paratext"), _T("Bibledit"));
		pMiddleStatic->SetLabel(tempStr);

		tempStr = pBottomStatic->GetLabel();
		tempStr.Replace(_T("Paratext"), _T("Bibledit"));
		pBottomStatic->SetLabel(tempStr);

		tempStr = pRadioRetainPT->GetLabel();
		tempStr.Replace(_T("Paratext"), _T("Bibledit"));
		pRadioRetainPT->SetLabel(tempStr);

		tempStr = pRadioForceAI->GetLabel();
		tempStr.Replace(_T("Paratext"), _T("Bibledit"));
		pRadioForceAI->SetLabel(tempStr);

		tempStr = pRadioConflictResDlg->GetLabel();
		tempStr.Replace(_T("Paratext"), _T("Bibledit"));
		pRadioConflictResDlg->SetLabel(tempStr);

		topBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_TOP);
		middleBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_MIDDLE);
		bottomBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_BOTTOM);

		tempStr = topBox->GetValue();
		tempStr.Replace(_T("Paratext"), _T("Bibledit"));
		topBox->ChangeValue(tempStr);

		tempStr = middleBox->GetValue();
		tempStr.Replace(_T("Paratext"), _T("Bibledit"));
		middleBox->ChangeValue(tempStr);

		tempStr = bottomBox->GetValue();
		tempStr.Replace(_T("Paratext"), _T("Bibledit"));
		bottomBox->ChangeValue(tempStr);
	}

	pWholeDlgSizer->Layout(); // get all resized because of the string replacements done
}

CConflictResActionDlg::~CConflictResActionDlg() // destructor
{
	
}

	void OnBnClickedRadioRetainPTorBE(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioForceAI(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioConflictResDlg(wxCommandEvent& WXUNUSED(event));

void CConflictResActionDlg::OnBnClickedRadioRetainPTorBE(wxCommandEvent& WXUNUSED(event))
{
	pRadioRetainPT->SetValue(TRUE);
	pRadioForceAI->SetValue(FALSE);
	pRadioConflictResDlg->SetValue(FALSE);
	
	m_bLegacy_retain_PTorBE_version = TRUE;
	m_bForce_AI_version_transfer = FALSE;
	m_bUserWantsVisualConflictResolution = FALSE;
}

void CConflictResActionDlg::OnBnClickedRadioForceAI(wxCommandEvent& WXUNUSED(event))
{
	pRadioRetainPT->SetValue(FALSE);
	pRadioForceAI->SetValue(TRUE);
	pRadioConflictResDlg->SetValue(FALSE);
	
	m_bLegacy_retain_PTorBE_version = FALSE;
	m_bForce_AI_version_transfer = TRUE;
	m_bUserWantsVisualConflictResolution = FALSE;
}

void CConflictResActionDlg::OnBnClickedRadioConflictResDlg(wxCommandEvent& WXUNUSED(event))
{
	pRadioRetainPT->SetValue(FALSE);
	pRadioForceAI->SetValue(FALSE);
	pRadioConflictResDlg->SetValue(TRUE);
	
	m_bLegacy_retain_PTorBE_version = FALSE;
	m_bForce_AI_version_transfer = FALSE;
	m_bUserWantsVisualConflictResolution = TRUE;
}

void CConflictResActionDlg::OnOK(wxCommandEvent& event)
{
	// The value of the user's choice to show or hide subsequent dialogs has to be recorded
	// here, and saved to the app flag; and we must act on it if the user wants no dlgs shown -
	// acting on it means that the legacy protocol happens no matter what the user may have
	// chosen by clicking one of the three radio buttons earlier
	gpApp->m_bConflictResolutionTurnedOn = !pCheckboxConflictResolutionDlgsToBeTurnedOff->GetValue();
	// If the value is '' be turned off " then set the legacy protocol to be in effect
	if (!gpApp->m_bConflictResolutionTurnedOn)
	{
		pRadioRetainPT->SetValue(TRUE);
		pRadioForceAI->SetValue(FALSE);
		pRadioConflictResDlg->SetValue(FALSE);

		m_bLegacy_retain_PTorBE_version = TRUE;
		m_bForce_AI_version_transfer = FALSE;
		m_bUserWantsVisualConflictResolution = FALSE;
	}

	event.Skip();
}