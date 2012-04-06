/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SetupEditorCollaboration.cpp
/// \author			Bill Martin
/// \date_created	8 April 2011
/// \date_revised	28 February 2012
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CSetupEditorCollaboration class. 
/// The CSetupEditorCollaboration class represents a dialog in which an administrator can set up Adapt It to
/// collaborate with an external editor such as Paratext or Bibledit. Once set up Adapt It will use projects
/// under the control of the external editor; obtaining its input (source) texts from one or more of the
/// editor's projects, and transferring its translation (target) texts to one of the editor's projects.
/// \derivation		The CSetupEditorCollaboration class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in SetupEditorCollaboration.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "SetupEditorCollaboration.h"
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
#include <wx/fileconf.h>

#include "Adapt_It.h"
#include "MainFrm.h"
#include "Adapt_ItView.h"
#include "SetupEditorCollaboration.h"
#include "CollabUtilities.h"
#include "CreateNewAIProjForCollab.h"

extern wxString szCollabProjectForSourceInputs;
extern wxString szCollabProjectForTargetExports;
extern wxString szCollabProjectForFreeTransExports;
extern wxString szCollabAIProjectName;
extern wxString szCollaborationEditor;
extern wxString szCollabExpectsFreeTrans;
extern wxString szCollabBookSelected;
extern wxString szCollabByChapterOnly;
extern wxString szCollabChapterSelected;
extern wxString szCollabSourceLangName;
extern wxString szCollabTargetLangName;
extern wxString szProjectConfiguration;

// event handler table
BEGIN_EVENT_TABLE(CSetupEditorCollaboration, AIModalDialog)
	EVT_INIT_DIALOG(CSetupEditorCollaboration::InitDialog)
	EVT_BUTTON(ID_BUTTON_SELECT_FROM_LIST_SOURCE_PROJ, CSetupEditorCollaboration::OnBtnSelectFromListSourceProj)
	EVT_BUTTON(ID_BUTTON_SELECT_FROM_LIST_TARGET_PROJ, CSetupEditorCollaboration::OnBtnSelectFromListTargetProj)
	EVT_BUTTON(ID_BUTTON_SELECT_FROM_LIST_FREE_TRANS_PROJ, CSetupEditorCollaboration::OnBtnSelectFromListFreeTransProj)
	EVT_BUTTON(wxID_OK, CSetupEditorCollaboration::OnClose) // function is called OnClose, but uses the wxID_OK event
	EVT_BUTTON(ID_BUTTON_NO_FREE_TRANS, CSetupEditorCollaboration::OnNoFreeTrans)
	EVT_BUTTON(ID_BUTTON_CREATE_NEW_AI_PROJECT, CSetupEditorCollaboration::OnCreateNewAIProject) // whm added 23Feb12
	EVT_BUTTON(ID_BUTTON_SAVE_SETUP_FOR_THIS_PROJ_NOW, CSetupEditorCollaboration::OnSaveSetupForThisProjNow) // whm added 23Feb12
	EVT_BUTTON(ID_BUTTON_REMOVE_THIS_PROJ_FROM_COLLAB, CSetupEditorCollaboration::OnRemoveThisAIProjectFromCollab) // whm added 23Feb12
	EVT_RADIOBUTTON(ID_RADIO_BY_CHAPTER_ONLY, CSetupEditorCollaboration::OnRadioBtnByChapterOnly)
	EVT_RADIOBUTTON(ID_RADIO_BY_WHOLE_BOOK, CSetupEditorCollaboration::OnRadioBtnByWholeBook)
	EVT_COMBOBOX(ID_COMBO_AI_PROJECTS, CSetupEditorCollaboration::OnComboBoxSelectAiProject)
	EVT_RADIOBOX(ID_RADIOBOX_EXTERNAL_SCRIPTURE_EDITOR, CSetupEditorCollaboration::OnRadioBoxSelectBtn)
	// ... other menu, button or control events
END_EVENT_TABLE()

CSetupEditorCollaboration::CSetupEditorCollaboration(wxWindow* parent) // dialog constructor
: AIModalDialog(parent, -1, _("Setup Collaboration"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pSetupEditorCollabSizer = SetupCollaborationBetweenAIandEditorFunc(this, FALSE, TRUE); // second param FALSE enables resize);
	// The declaration is: SetupParatextCollaborationDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(m_pApp != NULL);
	
	wxColour sysColorBtnFace; // color used for read-only text controls displaying
	// color used for read-only text controls displaying static text info button face color
	sysColorBtnFace = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	
	pTextCtrlAsStaticTopNote = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TOP_NOTE);
	wxASSERT(pTextCtrlAsStaticTopNote != NULL);
	pTextCtrlAsStaticTopNote->SetBackgroundColour(sysColorBtnFace);

	pStaticTextListOfProjects = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_LIST_OF_PROJECTS);
	wxASSERT(pStaticTextListOfProjects != NULL);

	pTextCtrlAsStaticSelectedSourceProj = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_SELECTED_SRC_PROJ);
	wxASSERT(pTextCtrlAsStaticSelectedSourceProj != NULL);
	pTextCtrlAsStaticSelectedSourceProj->SetBackgroundColour(sysColorBtnFace);

	pTextCtrlAsStaticSelectedTargetProj = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_SELECTED_TGT_PROJ);
	wxASSERT(pTextCtrlAsStaticSelectedTargetProj != NULL);
	pTextCtrlAsStaticSelectedTargetProj->SetBackgroundColour(sysColorBtnFace);

	pTextCtrlAsStaticSelectedFreeTransProj = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_SELECTED_FREE_TRANS_PROJ);
	wxASSERT(pTextCtrlAsStaticSelectedFreeTransProj != NULL);
	pTextCtrlAsStaticSelectedFreeTransProj->SetBackgroundColour(sysColorBtnFace);

	pStaticTextUseThisDialog = (wxStaticText*)FindWindowById(ID_TEXT_USE_THIS_DIALOG);
	wxASSERT(pStaticTextUseThisDialog != NULL);

	pStaticTextSelectWhichProj = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_SELECT_WHICH_PROJECTS);
	wxASSERT(pStaticTextSelectWhichProj != NULL);
	
	pStaticTextSrcTextFromThisProj = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_SRC_FROM_THIS_PROJECT);
	wxASSERT(pStaticTextSrcTextFromThisProj != NULL);

	pStaticTextTgtTextToThisProj = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_TARGET_TO_THIS_PROJECT);
	wxASSERT(pStaticTextTgtTextToThisProj != NULL);

	pStaticTextFtTextToThisProj = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_TO_THIS_FT_PROJECT);
	wxASSERT(pStaticTextFtTextToThisProj != NULL);

	pListOfProjects = (wxListBox*)FindWindowById(IDC_LIST_OF_COLLAB_PROJECTS);
	wxASSERT(pListOfProjects != NULL);

	pRadioBoxScriptureEditor = (wxRadioBox*)FindWindowById(ID_RADIOBOX_EXTERNAL_SCRIPTURE_EDITOR);
	wxASSERT(pRadioBoxScriptureEditor != NULL);

	pComboAiProjects = (wxComboBox*)FindWindowById(ID_COMBO_AI_PROJECTS);
	wxASSERT(pComboAiProjects != NULL);

	pBtnSelectFmListSourceProj = (wxButton*)FindWindowById(ID_BUTTON_SELECT_FROM_LIST_SOURCE_PROJ);
	wxASSERT(pBtnSelectFmListSourceProj != NULL);

	pBtnSelectFmListTargetProj = (wxButton*)FindWindowById(ID_BUTTON_SELECT_FROM_LIST_TARGET_PROJ);
	wxASSERT(pBtnSelectFmListTargetProj != NULL);

	pBtnSelectFmListFreeTransProj = (wxButton*)FindWindowById(ID_BUTTON_SELECT_FROM_LIST_FREE_TRANS_PROJ);
	wxASSERT(pBtnSelectFmListFreeTransProj != NULL);
	
	pBtnNoFreeTrans = (wxButton*)FindWindowById(ID_BUTTON_NO_FREE_TRANS);
	wxASSERT(pBtnNoFreeTrans != NULL);

	pRadioBtnByChapterOnly = (wxRadioButton*)FindWindowById(ID_RADIO_BY_CHAPTER_ONLY);
	wxASSERT(pRadioBtnByChapterOnly != NULL);
	
	pRadioBtnByWholeBook = (wxRadioButton*)FindWindowById(ID_RADIO_BY_WHOLE_BOOK);
	wxASSERT(pRadioBtnByWholeBook != NULL);

	pBtnRemoveProjFromCollab = (wxButton*)FindWindowById(ID_BUTTON_REMOVE_THIS_PROJ_FROM_COLLAB);
	wxASSERT(pBtnRemoveProjFromCollab != NULL);

	pBtnClose = (wxButton*)FindWindowById(wxID_OK); // the Close button uses the wxID_OK event id
	wxASSERT(pBtnClose != NULL);

	// We have no Cancel button so no need to reverse buttons
	//bool bOK;
	//bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	//bOK = bOK; // avoid warning

}

CSetupEditorCollaboration::~CSetupEditorCollaboration() // destructor
{
	
}

void CSetupEditorCollaboration::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	
	if (m_pApp->IsAIProjectOpen())
	{
		// whm Note: for the following block we use the App's m_collaborationEditor value instead of
		// our local m_TempCollaborationEditor which below this block is initialized to a null string.
		wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
		CAdapt_ItView* pView = m_pApp->GetView();
		if (pView != NULL)
		{
			wxString msg = _("Adapt It needs to close the currently open project (%s) in order to set up collaboration with %s.");
			msg = msg.Format(msg,m_pApp->m_curProjectName.c_str(),m_pApp->m_collaborationEditor.c_str());
			wxMessageBox(msg,_T(""),wxICON_INFORMATION);
			m_pApp->GetView()->CloseProject();
		}
	}

	m_bCollabChangedThisDlgSession = FALSE;
	
	// For InitDialog() empty the m_TempCollabAIProjectName
	m_TempCollabAIProjectName = _T("");
	m_TempCollaborationEditor = m_pApp->m_collaborationEditor;
	
	DoInit(); // empties m_Temp... variables for a new collab setup
}

void CSetupEditorCollaboration::DoInit()
{
	// DoInit() is called in the dialog's InitiDialog() handler and also in the OnRadioBoxSelectBtn()
	// handler.

	// whm revised 5Apr12. We now start the SetupEditorCollaboration dialog
	// with the following initial setup:
	// 1. No Adapt It project selected in the step 1 combo box.
	// 2. The possible PT/BE projects listed in step 2 (no change in this revision).
	// 3. The "Get by Chapter Only" radio button in Step 3 is selected.
	// 4. The edit boxes in step 3 are set blank/empty.

	// zero out the local Temp... variables as we did in OnInit()
	m_TempCollabProjectForSourceInputs = _T("");
	m_TempCollabProjectForTargetExports = _T("");
	m_TempCollabProjectForFreeTransExports = _T("");
	// m_TempCollabAIProjectName // can stay set to its current value - user sets it in OnComboBoxSelectAiProject()
	// m_TempCollaborationEditor // can stay set to its App value - user can set it in OnRadioBoxSelectBtn()
	m_bTempCollaborationExpectsFreeTrans = FALSE; // defaults to FALSE for no free trans
	m_TempCollabBookSelected = _T("");
	m_TempCollabSourceProjLangName = _T("");
	m_TempCollabTargetProjLangName = _T("");
	m_bTempCollabByChapterOnly = TRUE; // defaults to TRUE for collab by chapter only
	m_TempCollabChapterSelected = _T("");
	
	SetStateOfRemovalButton(); // disables the Remov... button when m_TempCollabAIProjectName is empty
	
	SetPTorBEsubStringsInControls(); // whm added 4Apr12. Sets %s substitutions with m_TempCollaborationEditor
	
	// whm added 28Jan12 after moving the "Get by Chapter Only" and "Get by Whole Book" 
	// radio buttons here from the GetSourceTextFromEditor dialog.
	// Initialize the "Get by Chapter Only" and "Get by Whole Book" radio buttons
	pRadioBtnByChapterOnly->SetValue(TRUE);
	pRadioBtnByWholeBook->SetValue(FALSE);

	// whm added 4Apr12 for selection of scripture editor.
	// Note: The SetupEditorCollaboration dialog will only be accessible from the
	// Administrator menu if at least one of the possible editors (Paratext or Bibledit) 
	// is currently installed, hence here in InitDialog() we can assume that either
	// Paratext or Bibledit will be installed when control gets here.
	// Disable any editor selection which is not installed.
	if (!m_pApp->m_bParatextIsInstalled)
		pRadioBoxScriptureEditor->Enable(0,FALSE);
	if (!m_pApp->m_bBibleditIsInstalled)
		pRadioBoxScriptureEditor->Enable(1,FALSE);
	
	if (m_TempCollaborationEditor == _T("Paratext"))
	{
		pRadioBoxScriptureEditor->SetSelection(0);
	}
	else if (m_TempCollaborationEditor == _T("Bibledit"))
	{
		pRadioBoxScriptureEditor->SetSelection(1);
	}
	else
	{
		// no editor has been selected yet, so make the default be Paratext
		pRadioBoxScriptureEditor->SetSelection(0);
	}

	int nProjectCount = 0;
	// get list of PT/BE projects
	projList.Clear();
	if (m_TempCollaborationEditor == _T("Paratext"))
	{
		projList = m_pApp->GetListOfPTProjects(); // as a side effect, it populates the App's m_pArrayOfCollabProjects
	}
	else if (m_TempCollaborationEditor == _T("Bibledit"))
	{
		projList = m_pApp->GetListOfBEProjects(); // as a side effect, it populates the App's m_pArrayOfCollabProjects
	}
	nProjectCount = (int)projList.GetCount();

	// Check for at least two usable PT projects in list
	if (nProjectCount < 2)
	{
		// error: PT/BE is not set up with enough projects for collaboration
		// this error will be reported when the administrator clicks on the "Save
		// Setup for this Collaboration Project Now" button.
	}
	else
	{
		// repopulate the pListOfProjects
		pListOfProjects->Clear();
		int i;
		for (i = 0; i < nProjectCount; i++)
		{
			wxString tempStr;
			tempStr = projList.Item(i);
			pListOfProjects->Append(tempStr);
		}
	}

	// Get a potential list/array of AI projects for the pComboAiProjects combo box.
	wxArrayString aiProjectNamesArray;
	m_pApp->GetPossibleAdaptionProjects(&aiProjectNamesArray);
	aiProjectNamesArray.Sort();
	
	// Clear the combo box and load the sorted list of ai projects into it.
	pComboAiProjects->Clear();
	int ct;
	for (ct = 0; ct < (int)aiProjectNamesArray.GetCount(); ct++)
	{
		pComboAiProjects->Append(aiProjectNamesArray.Item(ct));
	}
	wxString strSelection = pComboAiProjects->GetStringSelection();
	if (!strSelection.IsEmpty())
	{
		pComboAiProjects->Remove(0,strSelection.Length());
	}
	pComboAiProjects->SetSelection(-1); // remove any selection from the combo box
	pComboAiProjects->Refresh();
	
	pTextCtrlAsStaticSelectedSourceProj->ChangeValue(m_TempCollabProjectForSourceInputs);
	pTextCtrlAsStaticSelectedTargetProj->ChangeValue(m_TempCollabProjectForTargetExports);
	pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(m_TempCollabProjectForFreeTransExports);

	pSetupEditorCollabSizer->Layout();
	// The second radio button's label text is likely going to be truncated unless we resize the
	// dialog to fit it. Note: The constructor's call of ChooseCollabOptionsDlgFunc(this, FALSE, TRUE)
	// has its second parameter as FALSE to allow this resize here in InitDialog().
	wxSize dlgSize;
	dlgSize = pSetupEditorCollabSizer->ComputeFittingWindowSize(this);
	this->SetSize(dlgSize);
	this->CenterOnParent();
}

void CSetupEditorCollaboration::OnBtnSelectFromListSourceProj(wxCommandEvent& WXUNUSED(event))
{
	// Note: For source project, the project can be readable or writeable (all in list)
	// The OnOK() handler will check to ensure that the project selected for source
	// text inputs is different from the project selected for target text exports.
	
	// use a temporary array list for the list of projects
	wxString projShortName;
	wxArrayString tempListOfProjects;
	int ct,tot;
	tot = (int)projList.GetCount();
	for (ct = 0; ct < tot; ct++)
	{
		// load the rest of the projects into the temp array list
		tempListOfProjects.Add(projList.Item(ct));
	}
	wxString msg;
	msg = _("Choose a %s project that will be used for source text inputs");
	wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
	msg = msg.Format(msg,m_pApp->m_collaborationEditor.c_str());
	wxSingleChoiceDialog ChooseProjectForSourceTextInputs(this,msg,_("Select a project from this list"),tempListOfProjects);
	// preselect the project that was last selected if any
	int nPreselectedProjIndex = -1;
	// check to see if one was previously selected in the temp array list
	tot = (int)tempListOfProjects.GetCount();
	for (ct = 0; ct < tot; ct++)
	{
		if (tempListOfProjects.Item(ct) == m_TempCollabProjectForSourceInputs)
		{
			nPreselectedProjIndex = ct;
			break;
		}
	}
	if (nPreselectedProjIndex != -1)
	{
		// choose the preselected project
		ChooseProjectForSourceTextInputs.SetSelection(nPreselectedProjIndex);
	}
	wxString userSelectionStr;
	wxString saveCollabProjectForSourceInputs = m_TempCollabProjectForSourceInputs;
	userSelectionStr.Empty();
	if (ChooseProjectForSourceTextInputs.ShowModal() == wxID_OK)
	{
		userSelectionStr = ChooseProjectForSourceTextInputs.GetStringSelection();
		m_pApp->LogUserAction(_T("Selected Project for Source Text Inputs"));
		// Set temp value for source project name and update names in controls
		m_TempCollabProjectForSourceInputs = userSelectionStr;
		pTextCtrlAsStaticSelectedSourceProj->ChangeValue(userSelectionStr);
	}
	else
	{
		// user cancelled, don't change anything, just return
		return;
	}

	// whm Note: Within the SetupEditorCollaboration dialog the administrator is only
	// tasked with selecting the appropriate PT/BE projects for source, target, and
	// (optionally) free translation collaboration. He does not make any selection of
	// books in this dialog, so we can't check to see if a book exists in the PT/BE
	// target project (as we do in the GetSourceTestFromEditor dialog). 
	// We can, however, check to see if that project does not have any books created, 
	// in which case, we disallow the choice of that PT/BE project for storing target
	// texts.
	if (!CollabProjectHasAtLeastOneBook(m_TempCollabProjectForSourceInputs))
	{
		projShortName = GetShortNameFromProjectName(m_TempCollabProjectForSourceInputs);
		// The book does not have at least one book in the Source project
		wxString msg, msg1,msg2;
		if (m_pApp->m_collaborationEditor == _T("Paratext"))
		{
			msg1 = msg1.Format(_("The Paratext project for obtaining source texts (%s) does not yet have any books created for that project."),m_TempCollabProjectForSourceInputs.c_str());
			msg2 = msg2.Format(_("Please run Paratext and select the %s project. Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and ensure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again."),projShortName.c_str());
		}
		else if (m_pApp->m_collaborationEditor == _T("Bibledit"))
		{
			msg1 = msg1.Format(_("The Bibledit project for obtaining source texts (%s) does not yet have any books created for that project."),m_TempCollabProjectForSourceInputs.c_str());
			msg2 = msg2.Format(_("Please run Bibledit and select the %s project. Select File | Project | Properties. Then select \"Templates+\" from the Project properties dialog. Choose the book(s) to be created and click OK. Then return to Adapt It and try again."),projShortName.c_str());
		}
		msg = msg1 + _T(' ') + msg2;
		wxMessageBox(msg,_("No chapters and verses found"),wxICON_WARNING);
		// clear out the free translation control
		pTextCtrlAsStaticSelectedSourceProj->ChangeValue(wxEmptyString);
		m_TempCollabProjectForSourceInputs = _T(""); // invalid project for target exports
	}

	// If we get here the administrator made a selection. If the value changed mark it dirty.
	if (saveCollabProjectForSourceInputs != m_TempCollabProjectForSourceInputs)
		m_bCollabChangedThisDlgSession = TRUE;
}

void CSetupEditorCollaboration::OnBtnSelectFromListTargetProj(wxCommandEvent& WXUNUSED(event))
{
	// Note: For target project, we must ensure that the Paratext project is writeable
	// use a temporary array list for the list of projects
	wxArrayString tempListOfProjects;
	wxString projShortName;
	int ct,tot;
	tot = (int)projList.GetCount();
	for (ct = 0; ct < tot; ct++)
	{
		// Load the rest of the projects into the temp array list.
		// We must restrict the list of potential destination projects to those
		// which have have an editable attribute.
		projShortName = GetShortNameFromProjectName(projList.Item(ct));
		if (CollabProjectIsEditable(projShortName))
		{
			tempListOfProjects.Add(projList.Item(ct));
		}
	}
	wxString msg;
	msg = _("Choose a %s project that will be used for receiving translation text exports");
	wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
	msg = msg.Format(msg,m_pApp->m_collaborationEditor.c_str());
	wxSingleChoiceDialog ChooseProjectForTargetTextExports(this,msg,_("Select a project from this list"),tempListOfProjects);
	// preselect the project that was last selected if any
	int nPreselectedProjIndex = -1;
	// check to see if one was previously selected in the temp array list
	tot = (int)tempListOfProjects.GetCount();
	for (ct = 0; ct < tot; ct++)
	{
		if (tempListOfProjects.Item(ct) == m_TempCollabProjectForTargetExports)
		{
			nPreselectedProjIndex = ct;
			break;
		}
	}
	if (nPreselectedProjIndex != -1)
	{
		ChooseProjectForTargetTextExports.SetSelection(nPreselectedProjIndex);
	}
	wxString userSelectionStr;
	wxString saveCollabProjectForTargetExports = m_TempCollabProjectForTargetExports;
	userSelectionStr.Empty();
	if (ChooseProjectForTargetTextExports.ShowModal() == wxID_OK)
	{
		userSelectionStr = ChooseProjectForTargetTextExports.GetStringSelection();
		m_pApp->LogUserAction(_T("Selected Project for Target Text Exports"));
		// Set temp value for target project name and update names in controls
		m_TempCollabProjectForTargetExports = userSelectionStr;
		pTextCtrlAsStaticSelectedTargetProj->ChangeValue(userSelectionStr);
	}
	else
	{
		// user cancelled, don't change anything, just return
		return;
	}
	
	// whm Note: Within the SetupEditorCollaboration dialog the administrator is only
	// tasked with selecting the appropriate PT/BE projects for source, target, and
	// (optionally) free translation collaboration. He does not make any selection of
	// books in this dialog, so we can't check to see if a book exists in the PT/BE
	// target project (as we do in the GetSourceTestFromEditor dialog). 
	// We can, however, check to see if that project does not have any books created, 
	// in which case, we disallow the choice of that PT/BE project for storing target
	// texts.
	if (!CollabProjectHasAtLeastOneBook(m_TempCollabProjectForTargetExports))
	{
		projShortName = GetShortNameFromProjectName(m_TempCollabProjectForTargetExports);
		// The book does not have at least one book in the Target project
		wxString msg, msg1,msg2;
		if (m_pApp->m_collaborationEditor == _T("Paratext"))
		{
			msg1 = msg1.Format(_("The Paratext project for storing target texts (%s) does not yet have any books created for that project."),m_TempCollabProjectForTargetExports.c_str());
			msg2 = msg2.Format(_("Please run Paratext and select the %s project. Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and ensure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again."),projShortName.c_str());
		}
		else if (m_pApp->m_collaborationEditor == _T("Bibledit"))
		{
			msg1 = msg1.Format(_("The Bibledit project for storing target texts (%s) does not yet have any books created for that project."),m_TempCollabProjectForTargetExports.c_str());
			msg2 = msg2.Format(_("Please run Bibledit and select the %s project. Select File | Project | Properties. Then select \"Templates+\" from the Project properties dialog. Choose the book(s) to be created and click OK. Then return to Adapt It and try again."),projShortName.c_str());
		}
		msg = msg1 + _T(' ') + msg2;
		wxMessageBox(msg,_("No chapters and verses found"),wxICON_WARNING);
		// clear out the free translation control
		pTextCtrlAsStaticSelectedTargetProj->ChangeValue(wxEmptyString);
		m_TempCollabProjectForTargetExports = _T(""); // invalid project for target exports
	}
	
	// If we get here the administrator made a selection. If the value changed mark it dirty.
	if (saveCollabProjectForTargetExports != m_TempCollabProjectForTargetExports)
		m_bCollabChangedThisDlgSession = TRUE;
}

void CSetupEditorCollaboration::OnBtnSelectFromListFreeTransProj(wxCommandEvent& WXUNUSED(event))
{
	// Note: For free trans project, we must ensure that the Paratext project is writeable
	// use a temporary array list for the list of projects
	wxArrayString tempListOfProjects;
	wxString projShortName;
	pBtnNoFreeTrans->Enable(TRUE);
	int ct,tot;
	tot = (int)projList.GetCount();
	for (ct = 0; ct < tot; ct++)
	{
		// Load the rest of the projects into the temp array list.
		// We must restrict the list of potential destination projects to those
		// which have an editable attribute.
		projShortName = GetShortNameFromProjectName(projList.Item(ct));
		if (CollabProjectIsEditable(projShortName))
		{
			tempListOfProjects.Add(projList.Item(ct));
		}
	}
	wxString msg;
	msg = _("Choose a %s project that will be used for receiving free translation text exports");
	wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
	msg = msg.Format(msg,m_pApp->m_collaborationEditor.c_str());
	wxSingleChoiceDialog ChooseProjectForFreeTransTextInputs(this,msg,_("Select a project from this list"),tempListOfProjects);
	// preselect the project that was last selected if any
	int nPreselectedProjIndex = -1;
	// check to see if one was previously selected in the temp array list
	tot = (int)tempListOfProjects.GetCount();
	for (ct = 0; ct < tot; ct++)
	{
		if (tempListOfProjects.Item(ct) == m_TempCollabProjectForFreeTransExports)
		{
			nPreselectedProjIndex = ct;
			break;
		}
	}
	if (nPreselectedProjIndex != -1)
	{
		ChooseProjectForFreeTransTextInputs.SetSelection(nPreselectedProjIndex);
	}
	wxString userSelectionStr;
	wxString saveCollabProjectForFreeTransExports = m_TempCollabProjectForFreeTransExports;
	userSelectionStr.Empty();
	if (ChooseProjectForFreeTransTextInputs.ShowModal() == wxID_OK)
	{
		userSelectionStr = ChooseProjectForFreeTransTextInputs.GetStringSelection();
		m_pApp->LogUserAction(_T("Selected Project for Free Trans Text Exports"));
		// Set temp value for free trans project name and update names in controls
		m_TempCollabProjectForFreeTransExports = userSelectionStr;
		pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(userSelectionStr);
		m_bTempCollaborationExpectsFreeTrans = TRUE; // additional flag needed for free translations
	}
	else
	{
		// user cancelled, don't change anything, just return
		return;
	}
	
	// whm Note: Within the SetupEditorCollaboration dialog the administrator is only
	// tasked with selecting the appropriate PT/BE projects for source, target, and
	// (optionally) free translation collaboration. He does not make any selection of
	// books in this dialog, so we can't check to see if a book exists in the PT/BE
	// free translation project (as we do in the GetSourceTestFromEditor dialog). 
	// We can, however, check to see if that project does not have any books created, 
	// in which case, we disallow the choice of that PT/BE project for storing free 
	// translations.
	if (!CollabProjectHasAtLeastOneBook(m_TempCollabProjectForFreeTransExports))
	{
		projShortName = GetShortNameFromProjectName(m_TempCollabProjectForFreeTransExports);
		// The book does not have at least one book in the Free Trans project
		wxString msg, msg1,msg2;
		if (m_pApp->m_collaborationEditor == _T("Paratext"))
		{
			msg1 = msg1.Format(_("The Paratext project for storing free translations (%s) does not yet have any books created for that project."),m_TempCollabProjectForFreeTransExports.c_str());
			msg2 = msg2.Format(_("Please run Paratext and select the %s project. Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and ensure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again."),projShortName.c_str());
		}
		else if (m_pApp->m_collaborationEditor == _T("Bibledit"))
		{
			msg1 = msg1.Format(_("The Bibledit project for storing free translations (%s) does not yet have any books created for that project."),m_TempCollabProjectForFreeTransExports.c_str());
			msg2 = msg2.Format(_("Please run Bibledit and select the %s project. Select File | Project | Properties. Then select \"Templates+\" from the Project properties dialog. Choose the book(s) to be created and click OK. Then return to Adapt It and try again."),projShortName.c_str());
		}
		msg = msg1 + _T(' ') + msg2;
		wxMessageBox(msg,_("No chapters and verses found"),wxICON_WARNING);
		// clear out the free translation control
		pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(wxEmptyString);
		pBtnNoFreeTrans->Disable();
		m_TempCollabProjectForFreeTransExports = _T(""); // invalid project for free trans exports
		m_bTempCollaborationExpectsFreeTrans = FALSE;
	}
	
	// If we get here the administrator made a selection. If the value changed mark it dirty.
	if (saveCollabProjectForFreeTransExports != m_TempCollabProjectForFreeTransExports)
		m_bCollabChangedThisDlgSession = TRUE;
}

void CSetupEditorCollaboration::OnNoFreeTrans(wxCommandEvent& WXUNUSED(event))
{
	// If the free trans value changed mark it dirty.
	if (m_TempCollabProjectForFreeTransExports != pTextCtrlAsStaticSelectedFreeTransProj->GetValue())
	{
			m_bCollabChangedThisDlgSession = TRUE;

	}
	// Clear the pTextCtrlAsStaticSelectedFreeTransProj
	// read-only text box, empty the temp variable and disable the button.
	pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(wxEmptyString);
	m_TempCollabProjectForFreeTransExports.Empty();
	m_bTempCollaborationExpectsFreeTrans = FALSE;
	pBtnNoFreeTrans->Disable();
}

void CSetupEditorCollaboration::OnComboBoxSelectAiProject(wxCommandEvent& WXUNUSED(event))
{
	int nSel = pComboAiProjects->GetSelection();
	wxString selStr = pComboAiProjects->GetStringSelection();
	if (nSel == wxNOT_FOUND)
	{
		::wxBell();
		return;
	}

	// Get the AI project's collab settings if any. If the selected project already has 
	// collab settings put them into the m_Temp... variables and into the SetupEditorCollaboration 
	// dialog's controls, so the administrator can see what settings if any have already been made 
	// for that AI project. If the selected project has no collab settings yet, parse the three
	// m_Temp... ones from the selected project's name (m_TempCollabAIProjectName,
	// m_TempCollabSourceProjLangName, and m_TempCollabTargetProjLangName), and supply defaults
	// for the others.
	wxArrayString collabSettingsArray;
	wxArrayString collabLabelsArray;
	
	m_pApp->GetCollaborationSettingsOfAIProject(selStr, collabLabelsArray, collabSettingsArray);
	wxASSERT(collabLabelsArray.GetCount() == collabSettingsArray.GetCount());
	// Assign the m_Temp... and m_bTemp... local variables for the selected project..
	// If the existing project has collaboration settings use those, if not, assign
	// default values
	if (collabSettingsArray.GetCount() > 0)
	{
		// The AI project has collaboration settings
		int ct;
		int tot = (int)collabSettingsArray.GetCount();
		wxString collabLabelStr;
		wxString collabItemStr;
		for (ct = 0; ct < tot; ct++)
		{
			// Note: The main m_Temp... and m_bTemp values used by the SetupEditorCollaboration
			// dialog are:
			//    m_TempCollabProjectForSourceInputs
			//    m_TempCollabProjectForTargetExports
			//    m_TempCollabProjectForFreeTransExports
			//    m_TempCollabAIProjectName
			//    m_TempCollaborationEditor
			//    m_bTempCollabByChapterOnly
			// These m_Temp... and m_bTemp... values are derived from others or are unused
			//      m_bTempCollaborationExpectsFreeTrans
			//      m_TempCollabBookSelected
			//      m_TempCollabChapterSelected
			//      m_TempCollabSourceProjLangName
			//      m_TempCollabTargetProjLangName
			collabLabelStr = collabLabelsArray.Item(ct);
			collabItemStr = collabSettingsArray.Item(ct);
			if (collabLabelStr == szCollabProjectForSourceInputs)
				this->m_TempCollabProjectForSourceInputs = collabItemStr;
			else if (collabLabelStr == szCollabProjectForTargetExports)
				this->m_TempCollabProjectForTargetExports = collabItemStr;
			else if (collabLabelStr == szCollabProjectForFreeTransExports)
				this->m_TempCollabProjectForFreeTransExports = collabItemStr;
			else if (collabLabelStr == szCollabAIProjectName)
				this->m_TempCollabAIProjectName = collabItemStr;
			else if (collabLabelStr == szCollaborationEditor)
				this->m_TempCollaborationEditor = collabItemStr;
			else if (collabLabelStr == szCollabExpectsFreeTrans)
			{
				if (collabItemStr == _T("1"))
					this->m_bTempCollaborationExpectsFreeTrans = TRUE;
				else 
					this->m_bTempCollaborationExpectsFreeTrans = FALSE;
			}
			else if (collabLabelStr == szCollabBookSelected)
				this->m_TempCollabBookSelected = collabItemStr;
			else if (collabLabelStr == szCollabByChapterOnly)
			{
				if (collabItemStr == _T("1"))
					this->m_bTempCollabByChapterOnly = TRUE;
				else
					this->m_bTempCollabByChapterOnly = FALSE;
			}
			else if (collabLabelStr == szCollabChapterSelected)
				this->m_TempCollabChapterSelected = collabItemStr;
			else if (collabLabelStr == szCollabSourceLangName)
				this->m_TempCollabSourceProjLangName = collabItemStr;
			else if (collabLabelStr == szCollabTargetLangName)
				this->m_TempCollabTargetProjLangName = collabItemStr;
		}
		if (m_TempCollaborationEditor.IsEmpty() && !m_TempCollabProjectForSourceInputs.IsEmpty())
		{
			// we can determine which external editor was used previously by inspecting the
			// m_TempCollabProjectForSourceInputs string. If it has at least one color delimiter
			// in its string, it was using Paratext, otherwise it was using Bibledit
			if (m_TempCollabProjectForSourceInputs.Find(_T(':')) != wxNOT_FOUND)
			{
				// At least one colon ':' character is present in string therefore it was previously
				// using Paratext. Assign "Paratext" as editor (if it is installed).
				if (m_pApp->ParatextIsInstalled())
					m_TempCollaborationEditor = _T("Paratext");
			}
			else
			{
				// No colons found in the project string, therefore it was previously using Bibledit.
				// Assign "Bibledit" as editor (if it is installed).
				if (m_pApp->BibleditIsInstalled())
					m_TempCollaborationEditor = _T("Bibledit");
			}
		}
	}
	else
	{
		// The AI project has no collaboration settings, so parse the 
		// selStr and fill in values for m_TempCollabAIProjectName,
		// m_TempCollabSourceProjLangName, and m_TempCollabTargetProjLangName.
		m_TempCollabProjectForSourceInputs = _T("");
		m_TempCollabProjectForTargetExports = _T("");
		m_TempCollabProjectForFreeTransExports = _T("");
		
		m_TempCollabAIProjectName = selStr; // AI project name selected in the combo box

		// Call the ParatextIsInstalled() or BibleditIsInstalled() functions directly
		// here rather than using the App's m_bParatextIsInstalled or m_bBibleditIsInstalled
		// variables (which are only set in OnInit(). The user may have installed one of the
		// editors during this AI session.
		if (m_pApp->ParatextIsInstalled())
		{
		     m_TempCollaborationEditor = _T("Paratext"); // default editor
		}
		else if (m_pApp->BibleditIsInstalled())
		{
		     m_TempCollaborationEditor = _T("Bibledit"); // don't localize
		}
		
		m_bTempCollaborationExpectsFreeTrans = FALSE; // defaults to FALSE for no free trans
		m_TempCollabBookSelected = _T("");
		
		// parse the language names from the m_TempCollabAIProjectName
		m_pApp->GetSrcAndTgtLanguageNamesFromProjectName(selStr, 
			m_TempCollabSourceProjLangName,m_TempCollabTargetProjLangName);
		
		m_bTempCollabByChapterOnly = TRUE; // defaults to TRUE for collab by chapter only
		m_TempCollabChapterSelected = _T("");
	}
	// Note: Regardless of what the value for m_TempCollabAIProjectName might have been in the
	// project config file collaboration settings (from above), we force it to be what the 
	// administrator selected within the combo box, and force the associated language names too.
	m_TempCollabAIProjectName = selStr; // AI project name selected in the combo box
	// parse the language names from the m_TempCollabAIProjectName
	m_pApp->GetSrcAndTgtLanguageNamesFromProjectName(selStr, 
		m_TempCollabSourceProjLangName,m_TempCollabTargetProjLangName);

	// If the m_TempCollaborationEditor string is empty, it indicates that there was no editor
	// specified in the config file. In such cases initialize it to an installed editor
	// if available, giving preference to Paratext
	if (m_TempCollaborationEditor.IsEmpty())
	{
		// m_TempCollaborationEditor is empty so make empty the m_TempCollabProjectFor... variables too
		// since they may not be up to date - user will have to select from updated list of projects
		m_TempCollabProjectForSourceInputs.Empty();
		m_TempCollabProjectForTargetExports.Empty();
		m_TempCollabProjectForFreeTransExports.Empty();
		// The text controls will be updated with the empty values below
		
		// Assign a default editor so the text substitutions for
		if (m_pApp->ParatextIsInstalled())
		{
		     m_TempCollaborationEditor = _T("Paratext"); // default editor
		}
		else if (m_pApp->BibleditIsInstalled())
		{
		     m_TempCollaborationEditor = _T("Bibledit"); // don't localize
		}
	}

	if (m_TempCollaborationEditor == _T("Paratext"))
	{
		// set the "Paratext" radio box button
		pRadioBoxScriptureEditor->SetSelection(0);
	}
	else if (m_TempCollaborationEditor == _T("Bibledit"))
	{
		// set the "Bibledit" radio box button
		pRadioBoxScriptureEditor->SetSelection(1);
	}

	SetStateOfRemovalButton(); // enables the Remove... button because m_TempCollabAIProjectName now has content

	// Fill dialog controls with collab settings
	// Note: The combo box selection in step 1 was selected by the administrator so something should be selected!
	wxASSERT(pComboAiProjects->GetSelection() != -1);
	pTextCtrlAsStaticSelectedSourceProj->ChangeValue(m_TempCollabProjectForSourceInputs);
	pTextCtrlAsStaticSelectedTargetProj->ChangeValue(m_TempCollabProjectForTargetExports);
	pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(m_TempCollabProjectForFreeTransExports);
	pRadioBtnByChapterOnly->SetValue(m_bTempCollabByChapterOnly);
	pRadioBtnByWholeBook->SetValue(!m_bTempCollabByChapterOnly);
	
	// Don't set m_bCollabChangedThisDlgSession to TRUE in this handler for merely selecting an existing project

	// Refresh the list of PT/BE projects
	int nProjectCount = 0;
	projList.Clear();
	if (m_TempCollaborationEditor == _T("Paratext"))
	{
		projList = m_pApp->GetListOfPTProjects(); // as a side effect, it populates the App's m_pArrayOfCollabProjects
	}
	else if (m_TempCollaborationEditor == _T("Bibledit"))
	{
		projList = m_pApp->GetListOfBEProjects(); // as a side effect, it populates the App's m_pArrayOfCollabProjects
	}
	nProjectCount = (int)projList.GetCount();
	
	// Check for at least two usable PT projects in list
	if (nProjectCount < 2)
	{
		// error: PT/BE is not set up with enough projects for collaboration
		// this error will be reported when the administrator clicks on the "Save
		// Setup for this Collaboration Project Now" button.
	}
	else
	{
		// repopulate the pListOfProjects
		pListOfProjects->Clear();
		int i;
		for (i = 0; i < nProjectCount; i++)
		{
			wxString tempStr;
			tempStr = projList.Item(i);
			pListOfProjects->Append(tempStr);
		}
	}

	wxString errorStr = _T("");
	// CollabProjectsAreValid() doesn't consider any empty string projects to be invalid, only those non-empty
	// string parameters that don't have at least one book created in them. Therefore, selecting an AI
	// project that has no collaboration settings won't trigger the "Invalid PT/BE projects detected" message below.
	if (!CollabProjectsAreValid(m_TempCollabProjectForSourceInputs, m_TempCollabProjectForTargetExports, 
							m_TempCollabProjectForFreeTransExports, errorStr))
	{
		wxString msg = _("Adapt It detected invalid collaboration settings for the %s project in its project configuration file. The invalid %s project data is:\n%s");
		wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
		msg = msg.Format(msg,selStr.c_str(),m_pApp->m_collaborationEditor.c_str(),errorStr.c_str());
		msg += _T("\n\n");
		wxString msg1 = _("Please set up valid %s project(s) that Adapt It can use for collaboration.");
		msg1 = msg1.Format(msg1,m_pApp->m_collaborationEditor.c_str());
		msg += msg1;
		wxString titleMsg = _("Invalid %s projects detected");
		titleMsg = titleMsg.Format(titleMsg,m_pApp->m_collaborationEditor.c_str());
		wxMessageBox(msg,titleMsg,wxICON_WARNING);
		m_pApp->LogUserAction(titleMsg);
	}

}

void CSetupEditorCollaboration::OnRadioBtnByChapterOnly(wxCommandEvent& WXUNUSED(event))
{
    // Clicked "Get by Chapter Only" radio button
	m_bTempCollabByChapterOnly = TRUE;
	pRadioBtnByChapterOnly->SetValue(TRUE);
	pRadioBtnByWholeBook->SetValue(FALSE);
}

void CSetupEditorCollaboration::OnRadioBtnByWholeBook(wxCommandEvent& WXUNUSED(event))
{
    // Clicked "Get by Whole Book" radio button
	m_bTempCollabByChapterOnly = FALSE;
	pRadioBtnByChapterOnly->SetValue(FALSE);
	pRadioBtnByWholeBook->SetValue(TRUE);
}

// Note: This OnClose() internally uses the wxID_OK event, so it is 
// really an OnOK() handler.
void CSetupEditorCollaboration::OnClose(wxCommandEvent& event) 
{
	// whm revised 24Feb12. The OnClose() handler should now just check for any changes
	// made within the dialog that were not saved using the "Save Setup for this
	// Collaboration Project Now" button. Then prompt the user to either save the
	// changes or abandon the changes. The button handler for "Save Setup for this
	// Collaboration Project Now" handles all the checks that are needed (previously
	// handled by this OnOK() handler) to ensure that all information fields have
	// been filled, that source and target projects aren't the same, etc.
	if (m_bCollabChangedThisDlgSession)
	{
		wxString msg = _("You made changes to this collaboration setup - Do you want to save those changes?");
		int response;
		response = wxMessageBox(msg,_T(""),wxICON_QUESTION | wxYES_NO);
		if (response == wxYES)
		{
			if (!DoSaveSetupForThisProject())
			{
				// The DoSaveSetupForThisProject() encountered something that 
				// indicates we cannot save the collaboration configuration,
				// so do not Close. The user will have a chance to correct the 
				// problem, or next time s/he clicks Close, s/he can respond No 
				// to the prompt to save if necessary to Close the dialog.
				return;
			}
		}
		if (response == wxNO)
		{
			event.Skip(); // call event.Skip() to allow the handler to close the dialog
			return;
		}
	}

	// /////////////////////////////////////////////////////////////////////////////////////////
	// whm Note: Put code that aborts the OnClose() handler above this point
	// /////////////////////////////////////////////////////////////////////////////////////////

	// Set the App values for the PT projects to be used for PT collaboration
	m_pApp->m_CollabProjectForSourceInputs = m_TempCollabProjectForSourceInputs;
	m_pApp->m_CollabProjectForTargetExports = m_TempCollabProjectForTargetExports;
	m_pApp->m_CollabProjectForFreeTransExports = m_TempCollabProjectForFreeTransExports;
	m_pApp->m_CollabAIProjectName = m_TempCollabAIProjectName;
	m_pApp->m_collaborationEditor = m_TempCollaborationEditor;
	m_pApp->m_bCollaborationExpectsFreeTrans = m_bTempCollaborationExpectsFreeTrans;
	m_pApp->m_CollabSourceLangName = m_TempCollabSourceProjLangName;
	m_pApp->m_CollabTargetLangName = m_TempCollabTargetProjLangName;
	m_pApp->m_bCollabByChapterOnly = m_bTempCollabByChapterOnly; // whm added 28Jan12

	// Force the Main Frame's OnIdle() handler to call DoStartupWizardOnLaunch()
	// which, when appropriate calls DoStartWorkingWizard(). That in turn calls the 
	// "Get Source Text from Paratext Project" dialog instead of the normal startup
	// wizard when m_bStartWorkUsingCollaboration is TRUE. When 
	// m_bStartWorkUsingCollaboration is FALSE (as in the current case) the usual 
	// Start Working Wizard will appear.
	m_pApp->m_bJustLaunched = TRUE;
	// when leaving the SetupEditorCollaboration dialog we want the normal wizard
	// to appear, not the GetSourceTextFromEditor dialog.
	m_pApp->m_bStartWorkUsingCollaboration = FALSE; 

	// update status bar
	// whm modified 7Jan12 to call RefreshStatusBarInfo which now incorporates collaboration
	// info within its status bar message
	m_pApp->RefreshStatusBarInfo();

	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void CSetupEditorCollaboration::OnCreateNewAIProject(wxCommandEvent& WXUNUSED(event)) // whm added 23Feb12
{
	CCreateNewAIProjForCollab newProjDlg(m_pApp->GetMainFrame());

	if (newProjDlg.ShowModal() == wxID_OK)
	{
		// The CCreateNewAIProjForCollab dialog's OnOK() handler could assign the 
		// values but we do it here in the parent dialog, by accessing the
		// sub-dialog's wxTextCtrls.
		m_pApp->m_CollabSourceLangName = newProjDlg.pTextCtrlSrcLangName->GetValue();
		m_pApp->m_CollabTargetLangName = newProjDlg.pTextCtrlTgtLangName->GetValue();
		
		bool bDisableBookMode = TRUE;
		bool bProjOK = CreateNewAIProject(m_pApp, m_pApp->m_CollabSourceLangName, 
					m_pApp->m_CollabTargetLangName, m_pApp->m_sourceLanguageCode, 
					m_pApp->m_targetLanguageCode, bDisableBookMode);
		if (!bProjOK)
		{
            // This is a fatal error to continuing this collaboration attempt, but it won't
            // hurt the application. The error shouldn't ever happen. We let the dialog
            // continue to run, but tell the user that the app is probably unstable now and
            // he should get rid of any folders created in the attempt, shut down,
            // relaunch, and try again. This message is localizable.
			wxString message;
			message = message.Format(_("Error: attempting to create an Adapt It project for supporting collaboration with an external editor, failed.\nThe application is not in a state suitable for you to continue working, but it will still run. You should now Cancel and then shut it down.\nThen (using a File Browser application) you should also manually delete this folder and its contents: %s  if it exists.\nThen relaunch, and try again."),
				m_pApp->m_curProjectPath.c_str());
			m_pApp->LogUserAction(message);
			wxMessageBox(message,_("Project Not Created"), wxICON_ERROR);
			return;
		}
		else
		{
			// A new AI project was created OK.
			// A side effect of CreateNewAIProject() above is that it calls SetupDirectories()
			// which creates and loads the KBs for the new project. We want the new project 
			// created along with new KBs, but at this point we don't want the KBs loaded, so we 
			// unload them here before proceeding. If we don't do this the wizard will open at 
			// the DocPage rather than at the ProjectPage when "Close" is selected to exit the
			// SetupEditorCollaboration dialog.
			UnloadKBs(m_pApp);

			// add the new project to the combo box list of projects
			int nNewProjIndex;
			nNewProjIndex = pComboAiProjects->Append(m_pApp->m_curProjectName);
			wxASSERT(nNewProjIndex >= 0);
			pComboAiProjects->SetSelection(nNewProjIndex);
			// fill out the Temp variables we know about
			m_TempCollabAIProjectName = m_pApp->m_curProjectName;
			this->m_TempCollabSourceProjLangName = m_pApp->m_CollabSourceLangName;
			this->m_TempCollabTargetProjLangName = m_pApp->m_CollabTargetLangName;
			SetStateOfRemovalButton();
			// confirm to the user that the project was created
			wxString msg = _("An Adapt It project called \"%s\" was successfully created. It will appear as an Adapt It project in the \"Select a Project\" list of the Start Working Wizard.\n\nContinue through steps 2 through 4 below to set up this Adapt It project to collaborate with %s.");
			wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
			msg = msg.Format(msg,m_pApp->m_curProjectName.c_str(), m_pApp->m_collaborationEditor.c_str());
			wxMessageBox(msg,_("New Adapt It project created"),wxICON_INFORMATION);
			wxCommandEvent evt;
			OnComboBoxSelectAiProject(evt);
		}
	}
}

void CSetupEditorCollaboration::OnSaveSetupForThisProjNow(wxCommandEvent& WXUNUSED(event)) // whm added 23Feb12
{
	if (!DoSaveSetupForThisProject())
	{
		return;
	}

}

bool CSetupEditorCollaboration::DoSaveSetupForThisProject()
{
	// Check for completion of Step 1:
	// Check if the administrator has selected an AI project from the combo list of AI projects (step 1). If
	// not inform him that step 1 is necessary to identify an AI project for hookup to during collaboration
	// so that AI can save the collaboration settings for that project.
	if (pComboAiProjects->GetSelection() == -1)
	{
		wxString msg, msg1, msgTitle;
		msg1 = _("Please select an existing Adapt It project or create a new one in step 1.");
		msg = msg1;
		msg += _T(' ');
		msg += _("Adapt It will save the collaboration settings you make in this dialog for the Adapt It project you select (or create) in step 1.");
		msgTitle = _("No Adapt It project designated for this collaboration setup");
		wxMessageBox(msg,msgTitle,wxICON_INFORMATION);
		// set focus on the combo box
		pComboAiProjects->SetFocus();
		m_pApp->LogUserAction(msgTitle);
		return FALSE; // don't accept any changes - return FALSE to the caller
	}

	// Check for the completion of step 3:
	// Check if the administrator has selected an initial PT/BE project for obtaining source texts.
	if (m_TempCollabProjectForSourceInputs.IsEmpty())
	{
		wxString msg, msg1;
		msg = _("Please select a %s project for getting source texts. Use the \"Select from List\" button.");
		wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
		msg = msg.Format(msg,m_pApp->m_collaborationEditor.c_str());
		wxMessageBox(msg,_("No source language project selected for collaboration"),wxICON_INFORMATION);
		pBtnSelectFmListSourceProj->SetFocus();
		m_pApp->LogUserAction(msg);
		return FALSE; // don't accept any changes - return FALSE to the caller
	}
	// Check if the administrator has selected an initial PT/BE project for receiving translation drafts.
	if (m_TempCollabProjectForTargetExports.IsEmpty())
	{
		wxString msg, msg1;
		msg = _("Please select a %s project for receiving translated drafts. Use the \"Select from List\" button.");
		wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
		msg = msg.Format(msg,m_pApp->m_collaborationEditor.c_str());
		wxMessageBox(msg,_("No target language project selected for collaboration"),wxICON_INFORMATION);
		pBtnSelectFmListTargetProj->SetFocus();
		m_pApp->LogUserAction(msg);
		return FALSE; // don't accept any changes - return FALSE to the caller
	}
	
	// Check to ensure that the project selected for source text inputs is different 
	// from the project selected for target text exports, unless both are empty
	// strings, i.e., ("[No Project Selected]" was selected by the administrator).
	if (!m_TempCollabProjectForSourceInputs.IsEmpty() && !m_TempCollabProjectForTargetExports.IsEmpty())
	{
		// If the same project is selected, inform the administrator and abort the Save
		// operation so the administrator can select different projects.
		if (m_TempCollabProjectForSourceInputs == m_TempCollabProjectForTargetExports)
		{
			wxString msg;
			msg = _("The projects selected for getting source texts and receiving translation drafts cannot be the same.\nPlease select one project for getting source texts, and a different project for receiving translation texts.");
			wxMessageBox(msg);
			m_pApp->LogUserAction(msg);
			return FALSE; // don't accept any changes - return FALSE to the caller
		}
		
		if (m_TempCollabProjectForSourceInputs == m_TempCollabProjectForFreeTransExports
			&& m_bTempCollaborationExpectsFreeTrans)
		{
			wxString msg;
			msg = _("The projects selected for getting source texts and receiving free translation texts cannot be the same.\nPlease select one project for getting source texts, and a different project for receiving free translation texts.");
			wxMessageBox(msg);
			m_pApp->LogUserAction(msg);
			return FALSE; // don't accept any changes - return FALSE to the caller
		}
	}

	wxASSERT(!m_TempCollabSourceProjLangName.IsEmpty() && !m_TempCollabTargetProjLangName.IsEmpty());
	
	// Check to see if book folder mode is activated. If so, warn the administrator that it
	// will be turned OFF if s/he continues setting up AI for collaboration with Paratext/Bibledit.
	if (m_pApp->m_bBookMode)
	{
		wxString strSel = pComboAiProjects->GetStringSelection();
		wxString msg;
		wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
		msg = msg.Format(_("Note: Book Folder Mode is currently in effect, but it must be turned OFF and disabled for the %s Adapt It project in order for Adapt It to collaborate with %s. If you continue, Book Folder Mode will be turned off and disabled.\n\nDo you want to continue setting up this project for collaboration with %s?"),strSel.c_str(), m_pApp->m_collaborationEditor.c_str(),m_pApp->m_collaborationEditor.c_str());
		if (wxMessageBox(msg,_T(""),wxYES_NO | wxICON_INFORMATION) == wxNO)
		{
			m_pApp->LogUserAction(msg);
			return FALSE; // don't accept any changes - return FALSE to the caller
		}
	}

	wxString selStr = pComboAiProjects->GetStringSelection();
	wxString errorStr = _T("");
	// CollabProjectsAreValid() doesn't consider any empty string projects to be invalid, only those non-empty
	// string parameters that don't have at least one book created in them. In this situation, the code
	// above has already ensured that at least the m_TempCollabProjectForSourceInputs and m_TempCollabProjectForTargetExports
	// strings are non-empty.
	if (!CollabProjectsAreValid(m_TempCollabProjectForSourceInputs, m_TempCollabProjectForTargetExports, 
							m_TempCollabProjectForFreeTransExports, errorStr))
	{
		wxString msg = _("The following %s projects are not valid for collaboration:\n%s");
		wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
		msg = msg.Format(msg,m_pApp->m_collaborationEditor.c_str(),errorStr.c_str());
		msg += _T("\n\n");
		wxString msg1 = _("This setup cannot be accepted for collaboration. Please set up valid %s project(s) that Adapt It can use for collaboration.");
		msg1 = msg1.Format(msg1,m_pApp->m_collaborationEditor.c_str());
		msg += msg1;
		wxString titleMsg = _("Invalid %s projects detected");
		titleMsg = titleMsg.Format(titleMsg,m_pApp->m_collaborationEditor.c_str());
		wxMessageBox(msg,titleMsg,wxICON_WARNING);
		m_pApp->LogUserAction(titleMsg);
		return FALSE; // don't accept any changes - return FALSE to the caller
	}

	// Store collab settings in the App's members
	m_pApp->m_CollabProjectForSourceInputs = m_TempCollabProjectForSourceInputs;
	m_pApp->m_CollabProjectForTargetExports = m_TempCollabProjectForTargetExports;
	m_pApp->m_CollabProjectForFreeTransExports = m_TempCollabProjectForFreeTransExports;
	m_pApp->m_CollabAIProjectName = m_TempCollabAIProjectName;
	m_pApp->m_collaborationEditor = m_TempCollaborationEditor;
	m_pApp->m_bCollaborationExpectsFreeTrans = m_bTempCollaborationExpectsFreeTrans;
	m_pApp->m_CollabBookSelected = m_TempCollabBookSelected;
	m_pApp->m_CollabSourceLangName = m_TempCollabSourceProjLangName;
	m_pApp->m_CollabTargetLangName = m_TempCollabTargetProjLangName;
	m_pApp->m_bCollabByChapterOnly = m_bTempCollabByChapterOnly;
	m_pApp->m_CollabChapterSelected = m_TempCollabChapterSelected;

	// In order to write the collab settings to the selected project file we need to compose the
	// path to the project for the second parameter of WriteConfigurationFile().
	wxString newProjectPath;	// a local string to avoid unnecessarily changing the App's m_curProjectName
								// and m_curProjectPath.
	if (!m_pApp->m_customWorkFolderPath.IsEmpty() && m_pApp->m_bUseCustomWorkFolderPath)
	{
		newProjectPath = m_pApp->m_customWorkFolderPath + m_pApp->PathSeparator 
								 + m_pApp->m_CollabAIProjectName;
	}
	else
	{
		newProjectPath = m_pApp->m_workFolderPath + m_pApp->PathSeparator 
								 + m_pApp->m_CollabAIProjectName;
	}
	// Call WriteConfigurationFile(szProjectConfiguration, pApp->m_curProjectPath,projectConfigFile)
	// to save the settings in the project config file.
	bool bOK;
	bOK = m_pApp->WriteConfigurationFile(szProjectConfiguration, newProjectPath,projectConfigFile);
	if (bOK)
	{
		wxString newAIconfigFilePath = newProjectPath + m_pApp->PathSeparator + szProjectConfiguration + _T(".aic");
		// Tell administrator that the setup has been saved.
		wxString msg = _("The collaboration settings for the \"%s\" project were successfully saved in the project's configuration file at:\n\n%s\n\nYou may now select or create another Adapt It project (step 1) and make collaboration settings for that Adapt It project (setps 2 - 4).\n\nIf you are finished, select \"Close\" to close the setup dialog and test your setup(s) using the Start Working Wizard.");
		msg = msg.Format(msg,m_pApp->m_CollabAIProjectName.c_str(),newAIconfigFilePath.c_str());
		wxMessageBox(msg,_T("Save of collaboration settings successful"),wxICON_INFORMATION);
		m_bCollabChangedThisDlgSession = FALSE;
	}
	else
	{
		// Writing of the project config file failed for some reason. This would be unusual, so
		// just do an English notification
		wxMessageBox(_T("Error writing the project configuration file. Make sure it is not being used by another program and then try again."));
		return FALSE;
	}
	
	m_TempCollabProjectForSourceInputs = _T("");
	m_TempCollabProjectForTargetExports = _T("");
	m_TempCollabProjectForFreeTransExports = _T("");
	m_TempCollabAIProjectName = _T("");
	// m_TempCollaborationEditor can stay set to its current value
	m_bTempCollaborationExpectsFreeTrans = FALSE; // defaults to FALSE for no free trans
	m_TempCollabBookSelected = _T("");
	m_TempCollabSourceProjLangName = _T("");
	m_TempCollabTargetProjLangName = _T("");
	m_bTempCollabByChapterOnly = TRUE; // defaults to TRUE for collab by chapter only
	m_TempCollabChapterSelected = _T("");
	
	// Zero out all fields so the dialog is ready for another setup if desired
	wxString strSelection = pComboAiProjects->GetStringSelection();
	if (!strSelection.IsEmpty())
	{
		pComboAiProjects->Remove(0,strSelection.Length());
	}
	pComboAiProjects->SetSelection(-1); // remove any selection from the combo box
	pComboAiProjects->Refresh();
	pTextCtrlAsStaticSelectedSourceProj->ChangeValue(m_TempCollabProjectForSourceInputs);
	pTextCtrlAsStaticSelectedTargetProj->ChangeValue(m_TempCollabProjectForTargetExports);
	pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(m_TempCollabProjectForFreeTransExports);
	pRadioBtnByChapterOnly->SetValue(m_bTempCollabByChapterOnly);
	pRadioBtnByWholeBook->SetValue(!m_bTempCollabByChapterOnly);
	return TRUE; // success
}

void CSetupEditorCollaboration::OnRemoveThisAIProjectFromCollab(wxCommandEvent& WXUNUSED(event)) // whm added 23Feb12
{
	// "Remove this AI Project from Collaboration"
	// If no AI project is yet selected this handler is disabled, so we can assume that
	// if we get here an AI project will have been selected. 
	wxString projName = m_TempCollabAIProjectName;
	wxString msg = _("You are about to remove the collaboration settings for the %s project. If you continue the user will not be able to turn on collaboration with %s for this project.\n\nDo you want to remove the collaboration settings for %s?");
	wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
	msg = msg.Format(msg, projName.c_str(),m_pApp->m_collaborationEditor.c_str(),projName.c_str());
	int response = wxMessageBox(msg,_("Confirm removal of collaboration settings"),wxICON_QUESTION | wxYES_NO); 
	if (response == wxNO)
		return;

	// This is similar to the save routine that is found in the OnSaveSetupForThisProjNow() handler,
	// except that we are saving blank values for the collab settings into the project config
	// file.
	m_pApp->m_CollabProjectForSourceInputs = _T("");
	m_pApp->m_CollabProjectForTargetExports = _T("");
	m_pApp->m_CollabProjectForFreeTransExports = _T("");
	m_pApp->m_CollabAIProjectName = _T("");
	// m_pApp->m_collaborationEditor can stay set to its current value
	m_pApp->m_bCollaborationExpectsFreeTrans = FALSE;
	m_pApp->m_CollabBookSelected = _T("");
	m_pApp->m_CollabSourceLangName = _T("");
	m_pApp->m_CollabTargetLangName = _T("");
	m_pApp->m_bCollabByChapterOnly = TRUE;
	m_pApp->m_CollabChapterSelected = _T("");
	
	// In order to write the collab settings to the selected project file we need to compose the
	// path to the project for the second parameter of WriteConfigurationFile().
	wxString newProjectPath;	// a local string to avoid unnecessarily changing the App's m_curProjectName
								// and m_curProjectPath.
	if (!m_pApp->m_customWorkFolderPath.IsEmpty() && m_pApp->m_bUseCustomWorkFolderPath)
	{
		newProjectPath = m_pApp->m_customWorkFolderPath + m_pApp->PathSeparator 
								 + m_TempCollabAIProjectName;
	}
	else
	{
		newProjectPath = m_pApp->m_workFolderPath + m_pApp->PathSeparator 
								 + m_TempCollabAIProjectName;
	}
	// Call WriteConfigurationFile(szProjectConfiguration, pApp->m_curProjectPath,projectConfigFile)
	// to save the settings in the project config file.
	bool bOK;
	bOK = m_pApp->WriteConfigurationFile(szProjectConfiguration, newProjectPath,projectConfigFile);
	if (bOK)
	{
		wxString newAIconfigFilePath = newProjectPath + m_pApp->PathSeparator + szProjectConfiguration + _T(".aic");
		// Tell administrator that the setup has been saved.
		wxString msg = _("The collaboration settings for the \"%s\" project were successfully removed from the project's configuration file at:\n\n%s\n\nYou may now select or create another Adapt It project (step 1) and make collaboration settings for that Adapt It project (setps 2 - 4).\n\nIf you are finished, select \"Close\" to close the setup dialog and test your setup(s) using the Start Working Wizard.");
		msg = msg.Format(msg,m_TempCollabAIProjectName.c_str(),newAIconfigFilePath.c_str());
		wxMessageBox(msg,_T("Removal of collaboration settings successful"),wxICON_INFORMATION);
		m_bCollabChangedThisDlgSession = FALSE;
	}
	else
	{
		// Writing of the project config file failed for some reason. This would be unusual, so
		// just do an English notification
		wxCHECK_RET(bOK, _T("OnRemoveThisAIProjectFromCollab(): WriteConfigurationFile() failed, line 1189 in SetupEditorCollaboration.cpp"));
	}

	// zero out the local Temp... variables as we did in OnInit()
	m_TempCollabProjectForSourceInputs = _T("");
	m_TempCollabProjectForTargetExports = _T("");
	m_TempCollabProjectForFreeTransExports = _T("");
	m_TempCollabAIProjectName = _T("");
	// m_TempCollaborationEditor can stay set to its current value
	m_bTempCollaborationExpectsFreeTrans = FALSE; // defaults to FALSE for no free trans
	m_TempCollabBookSelected = _T("");
	m_TempCollabSourceProjLangName = _T("");
	m_TempCollabTargetProjLangName = _T("");
	m_bTempCollabByChapterOnly = TRUE; // defaults to TRUE for collab by chapter only
	m_TempCollabChapterSelected = _T("");
	
	// Zero out all fields so the dialog is ready for another setup if desired
	wxString strSelection = pComboAiProjects->GetStringSelection();
	if (!strSelection.IsEmpty())
	{
		pComboAiProjects->Remove(0,strSelection.Length());
	}
	pComboAiProjects->SetSelection(-1); // remove any selection from the combo box
	pComboAiProjects->Refresh();
	pTextCtrlAsStaticSelectedSourceProj->ChangeValue(m_TempCollabProjectForSourceInputs);
	pTextCtrlAsStaticSelectedTargetProj->ChangeValue(m_TempCollabProjectForTargetExports);
	pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(m_TempCollabProjectForFreeTransExports);
	pRadioBtnByChapterOnly->SetValue(m_bTempCollabByChapterOnly);
	pRadioBtnByWholeBook->SetValue(!m_bTempCollabByChapterOnly);

	m_bCollabChangedThisDlgSession = FALSE;
}

void CSetupEditorCollaboration::SetStateOfRemovalButton()
{
	if (m_TempCollabAIProjectName.IsEmpty())
		pBtnRemoveProjFromCollab->Disable();
	else
		pBtnRemoveProjFromCollab->Enable();
}

void CSetupEditorCollaboration::SetPTorBEsubStringsInControls()
{
	// adjust static strings substituting "Paratext" or "Bibledit" depending on
	// the string value in m_collaborationEditor
	// Substitute string in the "Use this dialog..." top static text
	wxString text;
	text = pStaticTextUseThisDialog->GetLabel();
	// whm Note: The control's label text may have already been substituted in which 
	// case there would be no $s to use in the .Format() statement. If substitutions
	// have already been done, just find and replace the "Paratext" or "Bibledit" 
	// substring with the current value for m_TempCollaborationEditor. 
	if (text.Find(_T("Paratext")) != wxNOT_FOUND)
		text.Replace(_T("Paratext"),m_TempCollaborationEditor);
	if (text.Find(_T("Bibledit")) != wxNOT_FOUND)
		text.Replace(_T("Bibledit"),m_TempCollaborationEditor);
	text = text.Format(text,m_TempCollaborationEditor.c_str());
	pStaticTextUseThisDialog->SetLabel(text);
	pStaticTextUseThisDialog->Refresh();

	// Substitute strings in the static text above the list box
	text = pStaticTextListOfProjects->GetLabel();
	// whm Note: The control's label text may have already been substituted in which 
	// case there would be no $s to use in the .Format() statement. If substitutions
	// have already been done, just find and replace the "Paratext" or "Bibledit" 
	// substring with the current value for m_TempCollaborationEditor. 
	if (text.Find(_T("Paratext")) != wxNOT_FOUND)
		text.Replace(_T("Paratext"),m_TempCollaborationEditor);
	if (text.Find(_T("Bibledit")) != wxNOT_FOUND)
		text.Replace(_T("Bibledit"),m_TempCollaborationEditor);
	int posP = text.Find(_T('('));
	if (posP != wxNOT_FOUND)
	{
		text = text.Mid(0,text.Find(_T('('))); // get only text before '(' to remove any old parenthetical text
		text.Trim(TRUE); // trim any white space from right end of string
	}
	text = text.Format(text,m_TempCollaborationEditor.c_str());
	text += _T(' ');
	if (m_TempCollaborationEditor == _T("Paratext"))
		text += _("(name : full name: language : code)");
	else if (m_TempCollaborationEditor == _T("Bibledit"))
		text += _("(name)");
	pStaticTextListOfProjects->SetLabel(text);
	pStaticTextListOfProjects->Refresh();

	// Substitute strings in the "Important" read-only edit box
	text = pTextCtrlAsStaticTopNote->GetValue(); // text has four %s sequences
	// whm Note: The control's label text may have already been substituted in which 
	// case there would be no $s to use in the .Format() statement. If substitutions
	// have already been done, just find and replace the "Paratext" or "Bibledit" 
	// substring with the current value for m_TempCollaborationEditor. 
	if (text.Find(_T("Paratext")) != wxNOT_FOUND)
		text.Replace(_T("Paratext"),m_TempCollaborationEditor);
	if (text.Find(_T("Bibledit")) != wxNOT_FOUND)
		text.Replace(_T("Bibledit"),m_TempCollaborationEditor);
	text = text.Format(text, m_TempCollaborationEditor.c_str(), 
		m_TempCollaborationEditor.c_str(), m_TempCollaborationEditor.c_str(),m_TempCollaborationEditor.c_str());
	pTextCtrlAsStaticTopNote->ChangeValue(text);
	pTextCtrlAsStaticTopNote->Refresh();

	// Substitute strings in the static text of step 3
	text = pStaticTextSelectWhichProj->GetLabel();
	// whm Note: The control's label text may have already been substituted in which 
	// case there would be no $s to use in the .Format() statement. If substitutions
	// have already been done, just find and replace the "Paratext" or "Bibledit" 
	// substring with the current value for m_TempCollaborationEditor. 
	if (text.Find(_T("Paratext")) != wxNOT_FOUND)
		text.Replace(_T("Paratext"),m_TempCollaborationEditor);
	if (text.Find(_T("Bibledit")) != wxNOT_FOUND)
		text.Replace(_T("Bibledit"),m_TempCollaborationEditor);
	text = text.Format(text,m_TempCollaborationEditor.c_str());
	pStaticTextSelectWhichProj->SetLabel(text);
	pStaticTextSelectWhichProj->Refresh();
	
	// Substitute string in the "Get source texts from..." static text
	text = pStaticTextSrcTextFromThisProj->GetLabel();
	// whm Note: The control's label text may have already been substituted in which 
	// case there would be no $s to use in the .Format() statement. If substitutions
	// have already been done, just find and replace the "Paratext" or "Bibledit" 
	// substring with the current value for m_TempCollaborationEditor. 
	if (text.Find(_T("Paratext")) != wxNOT_FOUND)
		text.Replace(_T("Paratext"),m_TempCollaborationEditor);
	if (text.Find(_T("Bibledit")) != wxNOT_FOUND)
		text.Replace(_T("Bibledit"),m_TempCollaborationEditor);
	text = text.Format(text,m_TempCollaborationEditor.c_str());
	pStaticTextSrcTextFromThisProj->SetLabel(text);
	pStaticTextSrcTextFromThisProj->Refresh();

	// Substitute string in the "Transfer translation drafts to..." static text
	text = pStaticTextTgtTextToThisProj->GetLabel();
	// whm Note: The control's label text may have already been substituted in which 
	// case there would be no $s to use in the .Format() statement. If substitutions
	// have already been done, just find and replace the "Paratext" or "Bibledit" 
	// substring with the current value for m_TempCollaborationEditor. 
	if (text.Find(_T("Paratext")) != wxNOT_FOUND)
		text.Replace(_T("Paratext"),m_TempCollaborationEditor);
	if (text.Find(_T("Bibledit")) != wxNOT_FOUND)
		text.Replace(_T("Bibledit"),m_TempCollaborationEditor);
	text = text.Format(text,m_TempCollaborationEditor.c_str());
	pStaticTextTgtTextToThisProj->SetLabel(text);
	pStaticTextTgtTextToThisProj->Refresh();

	// Substitute string in the "Transfer free translations to..." static text
	text = pStaticTextFtTextToThisProj->GetLabel();
	// whm Note: The control's label text may have already been substituted in which 
	// case there would be no $s to use in the .Format() statement. If substitutions
	// have already been done, just find and replace the "Paratext" or "Bibledit" 
	// substring with the current value for m_TempCollaborationEditor. 
	if (text.Find(_T("Paratext")) != wxNOT_FOUND)
		text.Replace(_T("Paratext"),m_TempCollaborationEditor);
	if (text.Find(_T("Bibledit")) != wxNOT_FOUND)
		text.Replace(_T("Bibledit"),m_TempCollaborationEditor);
	text = text.Format(text,m_TempCollaborationEditor.c_str());
	pStaticTextFtTextToThisProj->SetLabel(text);
	pStaticTextFtTextToThisProj->Refresh();
}

void CSetupEditorCollaboration::OnRadioBoxSelectBtn(wxCommandEvent& WXUNUSED(event))
{
    // Clicking the "Paratext" button returns nSel = 0; clicking the "Bibledit" button 
    // returns nSel = 1
	unsigned int nSel = pRadioBoxScriptureEditor->GetSelection();
	if (nSel == 0)
	{
		// "Paratext" button selected
		m_TempCollaborationEditor = _T("Paratext");
	}
	else
	{
		// "Bibledit" button selected
		m_TempCollaborationEditor = _T("Bibledit");
	}
	DoInit();
}