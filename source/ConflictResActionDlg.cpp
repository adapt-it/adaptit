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

		// The three wxStaticBoxSizer instances have non-localized text, so we have
	// to reset the text of their labels with localizable ones
	wxString topStr = _("Retain the %s version of the verse");
	wxString middleStr = _("Transfer the %s version of the verse");
	wxString bottomStr = _("Show a dialog with a list, comparing the two versions of each verse");
	wxStaticBox* pTopStatic = pTopStaticSizer->GetStaticBox();
	wxStaticBox* pMiddleStatic = pMiddleStaticSizer->GetStaticBox();
	wxStaticBox* pBottomStatic = pBottomStaticSizer->GetStaticBox();
	appNameStr = _T("Adapt It");
	topStr = topStr.Format(topStr, m_collabEditorName.c_str());
	middleStr = middleStr.Format(middleStr, appNameStr.c_str());
	pTopStatic->SetLabel(topStr);
	pMiddleStatic->SetLabel(middleStr);
	pBottomStatic->SetLabel(bottomStr);

	// Localize the 3 radio button labels
	wxString topRadioLabel = _("Retain the %s form of the verse unchanged, for each identified conflict (this is the default option)");
	topRadioLabel = topRadioLabel.Format(topRadioLabel, m_collabEditorName.c_str());
	pRadioRetainPT->SetLabel(topRadioLabel);
	wxString middleRadioLabel = _("Force the Adapt It verse contents to be transferred to %s, for each identified conflict");
	middleRadioLabel = middleRadioLabel.Format(middleRadioLabel, m_collabEditorName.c_str());
	pRadioForceAI->SetLabel(middleRadioLabel);
	wxString bottomRadioLabel = _("You want to see the conflicting verses, and manually choose which versions to accept");
	pRadioConflictResDlg->SetLabel(bottomRadioLabel);

	// Localize the 3 text box messages
	wxString topTextBoxStr = _("For Adapt It versions earlier than 6.6.0, this was the only action available.  If there are no user editing changes (within Adapt It) for conflicting verses, then the Paratext forms of those verses are retained unchanged. The Adapt It forms of those conflicting verses will never be transferred unless you make further editing changes in them. (If this is unsatisfactory, tick one of the lower buttons. The bottom button gives best control, but requires you to interact with another dialog.)");
	wxString middleTextBoxStr = _("This dialog opens at the first verse conflict encountered. A verse conflict means that the Adapt It form of the verse differs from the same verse in Paratext. If you choose this radio button, no additional dialog appears. Instead, every time Adapt It finds a verse conflict, it will automatically choose the Adapt It form of the verse, and transfer that to Paratext. This will permanently erase the content of that verse in Paratext, replacing it with what was transferred from Adapt It. (If you need more control, choose the button below.)");
	wxString bottomTextBoxStr = _("For this Save operation, all the identified verse conflicts will be shown to you in a dialog which appears just once. The conflicting verses are listed. Clicking a verse reference in the list, shows the Adapt It and Paratext versions of that verse, and you can choose the best version. (No editing allowed.) The versions are displayed side by side; the source text is above. Make your choice either using radio buttons, or choose the Adapt It version by clicking the checkbox beside it. Leaving the checkbox empty chooses the Paratext version.");
	topBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_TOP);
	middleBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_MIDDLE);
	bottomBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_BOTTOM);
	topTextBoxStr = topTextBoxStr.Format(topTextBoxStr, m_collabEditorName.c_str());
	middleTextBoxStr = middleTextBoxStr.Format(middleTextBoxStr, m_collabEditorName.c_str(),
								m_collabEditorName.c_str(), m_collabEditorName.c_str());
	bottomTextBoxStr = bottomTextBoxStr.Format(bottomTextBoxStr, m_collabEditorName.c_str());
	topBox->ChangeValue(topTextBoxStr);
	middleBox->ChangeValue(middleTextBoxStr);
	bottomBox->ChangeValue(bottomTextBoxStr);

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
