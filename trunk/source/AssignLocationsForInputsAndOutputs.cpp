/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AssignLocationsForInputsAndOutputs.cpp
/// \author			Bill Martin
/// \date_created	12 June 2011
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CAssignLocationsForInputsAndOutputs class. 
/// The CAssignLocationsForInputsAndOutputs class provides a dialog in which an administrator can
/// indicate which of the predefined AI folders should be assigned for AI's inputs and outputs. When
/// selected AI provides navigation protection for the user from navigating away from the assigned
/// folders during the selection of inputs and generation of outputs.
/// \derivation		The CAssignLocationsForInputsAndOutputs class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in AssignLocationsForInputsAndOutputs.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "AssignLocationsForInputsAndOutputs.h"
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
#include <wx/fileconf.h>

#include "Adapt_It.h"
#include "AssignLocationsForInputsAndOutputs.h"
#include "AdminMoveOrCopy.h"

// event handler table
BEGIN_EVENT_TABLE(CAssignLocationsForInputsAndOutputs, AIModalDialog)
	EVT_INIT_DIALOG(CAssignLocationsForInputsAndOutputs::InitDialog)// not strictly necessary for dialogs based on wxDialog
	// Samples:
	EVT_BUTTON(wxID_OK, CAssignLocationsForInputsAndOutputs::OnOK)
	//EVT_MENU(ID_SOME_MENU_ITEM, CAssignLocationsForInputsAndOutputs::OnDoSomething)
	//EVT_UPDATE_UI(ID_SOME_MENU_ITEM, CAssignLocationsForInputsAndOutputs::OnUpdateDoSomething)
	EVT_BUTTON(ID_BUTTON_SELECT_ALL_CHECKBOXES, CAssignLocationsForInputsAndOutputs::OnSelectAllCheckBoxes)
	EVT_BUTTON(ID_BUTTON_UNSELECT_ALL_CHECKBOXES, CAssignLocationsForInputsAndOutputs::OnUnSelectAllCheckBoxes)
	EVT_BUTTON(ID_BUTTON_PRE_LOAD_SOURCE_TEXTS, CAssignLocationsForInputsAndOutputs::OnPreLoadSourceTexts)
	//EVT_CHECKBOX(ID_SOME_CHECKBOX, CAssignLocationsForInputsAndOutputs::OnDoSomething)
	//EVT_RADIOBUTTON(ID_SOME_RADIOBUTTON, CAssignLocationsForInputsAndOutputs::DoSomething)
	//EVT_LISTBOX(ID_SOME_LISTBOX, CAssignLocationsForInputsAndOutputs::DoSomething)
	//EVT_COMBOBOX(ID_SOME_COMBOBOX, CAssignLocationsForInputsAndOutputs::DoSomething)
	//EVT_TEXT(IDC_SOME_EDIT_CTRL, CAssignLocationsForInputsAndOutputs::OnEnChangeEditSomething)
	// ... other menu, button or control events
END_EVENT_TABLE()

CAssignLocationsForInputsAndOutputs::CAssignLocationsForInputsAndOutputs(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Assign Locations For Inputs and Outputs"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	AssignLocationsForInputsOutputsFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	wxColour sysColorBtnFace; // color used for read-only text controls displaying
	// color used for read-only text controls displaying static text info button face color
	sysColorBtnFace = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	
	pTextCtrlInfo = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TOP_INFO);
	wxASSERT(pTextCtrlInfo != NULL);
	pTextCtrlInfo->SetBackgroundColour(sysColorBtnFace);

	pProtectSourceInputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_SOURCE_INPUTS);
	wxASSERT(pProtectSourceInputs != NULL);
	pProtectFreeTransOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_FREETRANS_OUTPUTS);
	wxASSERT(pProtectFreeTransOutputs != NULL);
	pProtectFreeTransRTFOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_FREETRANS_RTF_OUTPUTS);
	wxASSERT(pProtectFreeTransRTFOutputs != NULL);
	pProtectGlossOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_GLOSS_OUTPUTS);
	wxASSERT(pProtectGlossOutputs != NULL);
	pProtectGlossRTFOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_GLOSS_RTF_OUTPUTS);
	wxASSERT(pProtectGlossRTFOutputs != NULL);
	pProtectInterlinearRTFOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_INTERLINEAR_RTF_OUTPUTS);
	wxASSERT(pProtectInterlinearRTFOutputs != NULL);
	pProtectSourceOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_SOURCE_OUTPUTS);
	wxASSERT(pProtectSourceOutputs != NULL);
	pProtectSourceRTFOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_SOURCE_RTF_OUTPUTS);
	wxASSERT(pProtectSourceRTFOutputs != NULL);
	pProtectTargetOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_TARGET_OUTPUTS);
	wxASSERT(pProtectTargetOutputs != NULL);
	pProtectTargetRTFOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_TARGET_RTF_OUTPUTS);
	wxASSERT(pProtectTargetRTFOutputs != NULL);
	pProtectXhtmlOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_XHTML_OUTPUTS); // whm added 23Jul12
	wxASSERT(pProtectXhtmlOutputs != NULL);
	pProtectPathwayOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_PATHWAY_OUTPUTS); // whm added 14Aug12
	wxASSERT(pProtectPathwayOutputs != NULL);
	pProtectKBInputsAndOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_KB_INPUTS_AND_OUTPUTS);
	wxASSERT(pProtectKBInputsAndOutputs != NULL);
	pProtectLIFTInputsAndOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_LIFT_INPUTS_AND_OUTPUTS);
	wxASSERT(pProtectLIFTInputsAndOutputs != NULL);
	pProtectPackedInputsAndOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_PACKED_INPUTS_AND_OUTPUTS);
	wxASSERT(pProtectPackedInputsAndOutputs != NULL);
	pProtectCCTableInputsAndOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_CCTABLE_INPUTS_AND_OUTPUTS);
	wxASSERT(pProtectCCTableInputsAndOutputs != NULL);
	pProtectReportsLogsOutputs = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PROTECT_REPORTS_OUTPUTS);
	wxASSERT(pProtectReportsLogsOutputs != NULL);

	pBtnPreLoadSourceTexts = (wxButton*)FindWindowById(ID_BUTTON_PRE_LOAD_SOURCE_TEXTS);
	wxASSERT(pBtnPreLoadSourceTexts != NULL);
	pSelectAllCheckBoxes = (wxButton*)FindWindowById(ID_BUTTON_SELECT_ALL_CHECKBOXES);
	wxASSERT(pSelectAllCheckBoxes != NULL);
	pUnSelectAllCheckBoxes = (wxButton*)FindWindowById(ID_BUTTON_UNSELECT_ALL_CHECKBOXES);
	wxASSERT(pUnSelectAllCheckBoxes != NULL);

	// other attribute initializations
}

CAssignLocationsForInputsAndOutputs::~CAssignLocationsForInputsAndOutputs() // destructor
{
	
}

void CAssignLocationsForInputsAndOutputs::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	// tick all the checkboxes that were indicated to be ticked in the project config file,
	// which upon reading set all the App's boolean flags
	pProtectSourceInputs->SetValue(m_pApp->m_bProtectSourceInputsFolder);
	pProtectFreeTransOutputs->SetValue(m_pApp->m_bProtectFreeTransOutputsFolder);
	pProtectFreeTransRTFOutputs->SetValue(m_pApp->m_bProtectFreeTransRTFOutputsFolder);
	pProtectGlossOutputs->SetValue(m_pApp->m_bProtectGlossOutputsFolder);
	pProtectGlossRTFOutputs->SetValue(m_pApp->m_bProtectGlossRTFOutputsFolder);
	pProtectInterlinearRTFOutputs->SetValue(m_pApp->m_bProtectInterlinearRTFOutputsFolder);
	pProtectSourceOutputs->SetValue(m_pApp->m_bProtectSourceOutputsFolder);
	pProtectSourceRTFOutputs->SetValue(m_pApp->m_bProtectSourceRTFOutputsFolder);
	pProtectTargetOutputs->SetValue(m_pApp->m_bProtectTargetOutputsFolder);
	pProtectTargetRTFOutputs->SetValue(m_pApp->m_bProtectTargetRTFOutputsFolder);
	pProtectXhtmlOutputs->SetValue(m_pApp->m_bProtectXhtmlOutputsFolder);
	pProtectPathwayOutputs->SetValue(m_pApp->m_bProtectPathwayOutputsFolder);
	pProtectKBInputsAndOutputs->SetValue(m_pApp->m_bProtectKbInputsAndOutputsFolder);
	pProtectLIFTInputsAndOutputs->SetValue(m_pApp->m_bProtectLiftInputsAndOutputsFolder);
	pProtectPackedInputsAndOutputs->SetValue(m_pApp->m_bProtectPackedInputsAndOutputsFolder);
	pProtectCCTableInputsAndOutputs->SetValue(m_pApp->m_bProtectCCTableInputsAndOutputsFolder);
	pProtectReportsLogsOutputs->SetValue(m_pApp->m_bProtectReportsOutputsFolder);

	// If Paratext/Bibledit collaboration is ON, disable the "Pre-Load Source Texts" button
	// and change "Protect..." labels for __SOURCE_INPUTS and _TARGET_OUTPUTS
	if (m_pApp->m_bCollaboratingWithParatext)
	{
		pBtnPreLoadSourceTexts->Disable();
		pProtectSourceInputs->SetValue(TRUE);
		pProtectSourceInputs->SetLabel(_("Using Paratext project"));
		pProtectSourceInputs->Disable();
		pProtectTargetOutputs->SetValue(TRUE);
		pProtectTargetOutputs->SetLabel(_("Using Paratext project"));
		pProtectTargetOutputs->Disable();
		if (!m_pApp->m_CollabProjectForFreeTransExports.IsEmpty())
		{
			pProtectFreeTransOutputs->SetValue(TRUE);
			pProtectFreeTransOutputs->SetLabel(_("Using Paratext project"));
			pProtectFreeTransOutputs->Disable();
		}
	}
	if (m_pApp->m_bCollaboratingWithBibledit)
	{
		pBtnPreLoadSourceTexts->Disable();
		pProtectSourceInputs->SetValue(TRUE);
		pProtectSourceInputs->SetLabel(_("Using Bibledit project"));
		pProtectSourceInputs->Disable();
		pProtectTargetOutputs->SetValue(TRUE);
		pProtectTargetOutputs->SetLabel(_("Using Bibledit project"));
		pProtectTargetOutputs->Disable();
		if (!m_pApp->m_CollabProjectForFreeTransExports.IsEmpty())
		{
			pProtectFreeTransOutputs->SetValue(TRUE);
			pProtectFreeTransOutputs->SetLabel(_("Using Bibledit project"));
			pProtectFreeTransOutputs->Disable();
		}
	}

	// whm 3Mar12 removed. The administrator now does not determine if collaboration
	// is ON or OFF - that is now up to the user at the time that an AI project is
	// selected at the wizard's ProjectPage.
	/*
	if ((m_pApp->ParatextIsInstalled() && !m_pApp->m_bCollaboratingWithParatext)
		|| (m_pApp->BibleditIsInstalled() && !m_pApp->m_bCollaboratingWithBibledit))
	{
		// Paratext/Bibledit is installed, but the administrator has not turned ON
		// collaboration at this point, remind him that, if he plans to switch ON 
		// collaboration with paratext/Bibledit, he should do it before assigning 
		// locations for Inputs and Outputs.
		wxString msg,titleMsg,collabEditor;
		if (m_pApp->ParatextIsInstalled())
			collabEditor = _T("Paratext");
		else if (m_pApp->BibleditIsInstalled())
			collabEditor = _T("Bibledit");
		msg = msg.Format(_("%s is installed on this computer. If you intend for the user to collaborate with %s, you should turn ON that collaboration before you assign locations for inputs and outputs in the following dialog."),
			collabEditor.c_str(),collabEditor.c_str());
		titleMsg = titleMsg.Format(_("Set up %s collaboration before assigning locations for inputs and outputs"),collabEditor.c_str());
		wxMessageBox(msg,titleMsg,wxICON_INFORMATION | wxOK);
	}
	*/
}

// event handling functions

//CAssignLocationsForInputsAndOutputs::OnDoSomething(wxCommandEvent& event)
//{
//	// handle the event
	
//}

void CAssignLocationsForInputsAndOutputs::OnSelectAllCheckBoxes(wxCommandEvent& WXUNUSED(event))
{
	// tick all the checkboxes
	pProtectSourceInputs->SetValue(TRUE);
	pProtectFreeTransOutputs->SetValue(TRUE);
	pProtectFreeTransRTFOutputs->SetValue(TRUE);
	pProtectGlossOutputs->SetValue(TRUE);
	pProtectGlossRTFOutputs->SetValue(TRUE);
	pProtectInterlinearRTFOutputs->SetValue(TRUE);
	pProtectSourceOutputs->SetValue(TRUE);
	pProtectSourceRTFOutputs->SetValue(TRUE);
	pProtectTargetOutputs->SetValue(TRUE);
	pProtectTargetRTFOutputs->SetValue(TRUE);
	pProtectXhtmlOutputs->SetValue(TRUE); // whm added 23Jul12
	pProtectPathwayOutputs->SetValue(TRUE); // whm added 14Aug12
	pProtectKBInputsAndOutputs->SetValue(TRUE);
	pProtectLIFTInputsAndOutputs->SetValue(TRUE);
	pProtectPackedInputsAndOutputs->SetValue(TRUE);
	pProtectCCTableInputsAndOutputs->SetValue(TRUE);
	pProtectReportsLogsOutputs->SetValue(TRUE);
}

void CAssignLocationsForInputsAndOutputs::OnUnSelectAllCheckBoxes(wxCommandEvent& WXUNUSED(event))
{
	// un-tick all the checkboxes
	// whm modification. We don't unselect source inputs or target outputs
	// when collaboration with PT/BE is ON. Also we don't unselect free trans
	// outputs when collaboration with PT/BE is ON and m_bCollaborationExpectsFreeTrans
	// is TRUE.
	if (!m_pApp->m_bCollaboratingWithParatext && !m_pApp->m_bCollaboratingWithBibledit)
	{
		pProtectSourceInputs->SetValue(FALSE);
	}
	if ((!m_pApp->m_bCollaboratingWithParatext && !m_pApp->m_bCollaboratingWithBibledit)
		|| !m_pApp->m_bCollaborationExpectsFreeTrans)
	{
		pProtectFreeTransOutputs->SetValue(FALSE);
	}
	pProtectFreeTransRTFOutputs->SetValue(FALSE);
	pProtectGlossOutputs->SetValue(FALSE);
	pProtectGlossRTFOutputs->SetValue(FALSE);
	pProtectInterlinearRTFOutputs->SetValue(FALSE);
	pProtectSourceOutputs->SetValue(FALSE);
	pProtectSourceRTFOutputs->SetValue(FALSE);
	if (!m_pApp->m_bCollaboratingWithParatext && !m_pApp->m_bCollaboratingWithBibledit)
	{
		pProtectTargetOutputs->SetValue(FALSE);
	}
	pProtectTargetRTFOutputs->SetValue(FALSE);
	pProtectXhtmlOutputs->SetValue(FALSE); // whm added 23Jul12
	pProtectPathwayOutputs->SetValue(FALSE); // whm added 14Aug12
	pProtectKBInputsAndOutputs->SetValue(FALSE);
	pProtectLIFTInputsAndOutputs->SetValue(FALSE);
	pProtectPackedInputsAndOutputs->SetValue(FALSE);
	pProtectCCTableInputsAndOutputs->SetValue(FALSE);
	pProtectReportsLogsOutputs->SetValue(FALSE);
}

void CAssignLocationsForInputsAndOutputs::OnPreLoadSourceTexts(wxCommandEvent& WXUNUSED(event))
{
	m_pApp->LogUserAction(_T("Pre-Load Source Text selected"));
	bool bDirExists = TRUE;
	if (!::wxDirExists(m_pApp->m_sourceInputsFolderPath) && !::wxFileExists(m_pApp->m_sourceInputsFolderPath))
	{
		// there is no such file or folder, so create the folder
		bDirExists = ::wxMkdir(m_pApp->m_sourceInputsFolderPath,0777);
	}
	wxASSERT(bDirExists);
	bDirExists = bDirExists; // avoid warning
	wxCommandEvent dummyEvent;
	m_pApp->OnMoveOrCopyFoldersOrFiles(dummyEvent);
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CAssignLocationsForInputsAndOutputs::OnOK(wxCommandEvent& event) 
{
	wxString foldersProtectedFromNavigation = _T("");
	m_pApp->m_bProtectSourceInputsFolder = pProtectSourceInputs->GetValue();
	if (m_pApp->m_bProtectSourceInputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_sourceInputsFolderName + _T(':');
	m_pApp->m_bProtectFreeTransOutputsFolder = pProtectFreeTransOutputs->GetValue();
	if (m_pApp->m_bProtectFreeTransOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_freeTransOutputsFolderName + _T(':');
	m_pApp->m_bProtectFreeTransRTFOutputsFolder = pProtectFreeTransRTFOutputs->GetValue();
	if (m_pApp->m_bProtectFreeTransRTFOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_freeTransRTFOutputsFolderName + _T(':');
	m_pApp->m_bProtectGlossOutputsFolder = pProtectGlossOutputs->GetValue();
	if (m_pApp->m_bProtectGlossOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_glossOutputsFolderName + _T(':');
	m_pApp->m_bProtectGlossRTFOutputsFolder = pProtectGlossRTFOutputs->GetValue();
	if (m_pApp->m_bProtectGlossRTFOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_glossRTFOutputsFolderName + _T(':');
	m_pApp->m_bProtectInterlinearRTFOutputsFolder = pProtectInterlinearRTFOutputs->GetValue();
	if (m_pApp->m_bProtectInterlinearRTFOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_interlinearRTFOutputsFolderName + _T(':');
	m_pApp->m_bProtectSourceOutputsFolder = pProtectSourceOutputs->GetValue();
	if (m_pApp->m_bProtectSourceOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_sourceOutputsFolderName + _T(':');
	m_pApp->m_bProtectSourceRTFOutputsFolder = pProtectSourceRTFOutputs->GetValue();
	if (m_pApp->m_bProtectSourceRTFOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_sourceRTFOutputsFolderName + _T(':');
	m_pApp->m_bProtectTargetOutputsFolder = pProtectTargetOutputs->GetValue();
	if (m_pApp->m_bProtectTargetOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_targetOutputsFolderName + _T(':');
	m_pApp->m_bProtectTargetRTFOutputsFolder = pProtectTargetRTFOutputs->GetValue();
	if (m_pApp->m_bProtectTargetRTFOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_targetRTFOutputsFolderName + _T(':');
	m_pApp->m_bProtectXhtmlOutputsFolder = pProtectXhtmlOutputs->GetValue();
	if (m_pApp->m_bProtectXhtmlOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_xhtmlOutputsFolderName + _T(':');
	m_pApp->m_bProtectPathwayOutputsFolder = pProtectPathwayOutputs->GetValue(); // whm added 14Aug12
	if (m_pApp->m_bProtectPathwayOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_pathwayOutputsFolderName + _T(':');
	m_pApp->m_bProtectKbInputsAndOutputsFolder = pProtectKBInputsAndOutputs->GetValue();
	if (m_pApp->m_bProtectKbInputsAndOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_kbInputsAndOutputsFolderName + _T(':');
	m_pApp->m_bProtectLiftInputsAndOutputsFolder = pProtectLIFTInputsAndOutputs->GetValue();
	if (m_pApp->m_bProtectLiftInputsAndOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_liftInputsAndOutputsFolderName + _T(':');
	m_pApp->m_bProtectPackedInputsAndOutputsFolder = pProtectPackedInputsAndOutputs->GetValue();
	if (m_pApp->m_bProtectPackedInputsAndOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_packedInputsAndOutputsFolderName + _T(':');
	m_pApp->m_bProtectCCTableInputsAndOutputsFolder = pProtectCCTableInputsAndOutputs->GetValue();
	if (m_pApp->m_bProtectCCTableInputsAndOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_ccTableInputsAndOutputsFolderName + _T(':');
	m_pApp->m_bProtectReportsOutputsFolder = pProtectReportsLogsOutputs->GetValue();
	if (m_pApp->m_bProtectReportsOutputsFolder)
		foldersProtectedFromNavigation += m_pApp->m_reportsOutputsFolderName + _T(':');

	m_pApp->m_foldersProtectedFromNavigation = foldersProtectedFromNavigation;
	
	m_pApp->LogUserAction(_T("Folders protected from Navigation: ") + m_pApp->m_foldersProtectedFromNavigation);
	
	// update the value related to m_foldersProtectedFromNavigation in the Adapt_It_WX.ini file
	bool bWriteOK = FALSE;
	wxString oldPath = m_pApp->m_pConfig->GetPath(); // is always absolute path "/Recent_File_List"
	m_pApp->m_pConfig->SetPath(_T("/Settings"));
	// whm 30Sep11 Note: we want even a null string value for the m_foldersProtextedFromNavigation string
	// to be saved in Adapt_It_WX.ini.
	{ // block for wxLogNull
		wxLogNull logNo; // eliminates spurious message from the system
		bWriteOK = m_pApp->m_pConfig->Write(_T("folders_protected_from_navigation"), m_pApp->m_foldersProtectedFromNavigation);
		if (!bWriteOK)
		{
			wxMessageBox(_T("AssignLocationsForInputsAndOutputs.cpp, OnOK() m_pConfig->Write() returned FALSE at line 353, processing will continue, but save, shutdown and restart would be wise"));
		}
		m_pApp->m_pConfig->Flush(); // write now, otherwise write takes place when m_pConfig is destroyed in OnExit().
	}
	// restore the oldPath back to "/Recent_File_List"
	m_pApp->m_pConfig->SetPath(oldPath);
	
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

// other class methods

