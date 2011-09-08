/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SetupEditorCollaboration.cpp
/// \author			Bill Martin
/// \date_created	8 April 2011
/// \date_revised	30 June 2011
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CSetupEditorCollaboration class. 
/// The CSetupEditorCollaboration class represents a dialog in which an administrator can set up Adapt It to
/// collaborate with an external editor such as Paratext or Bibledit. Once set up Adapt It will use projects
/// under the control of the external editor; obtaining its input (source) texts from one or more of the
/// editor's projects, and transferring its translation (target) texts to one of the editor's projects.
/// \derivation		The CSetupEditorCollaboration class is derived from wxDialog.
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
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
//#include <wx/valtext.h> // for wxTextValidator
#include <wx/fileconf.h>

#include "Adapt_It.h"
#include "MainFrm.h"
#include "Adapt_ItView.h"
#include "SetupEditorCollaboration.h"
#include "GetSourceTextFromEditor.h"
#include "CollabUtilities.h"

extern const wxString createNewProjectInstead;

// event handler table
BEGIN_EVENT_TABLE(CSetupEditorCollaboration, AIModalDialog)
	EVT_INIT_DIALOG(CSetupEditorCollaboration::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(ID_BUTTON_SELECT_FROM_LIST_SOURCE_PROJ, CSetupEditorCollaboration::OnBtnSelectFromListSourceProj)
	EVT_BUTTON(ID_BUTTON_SELECT_FROM_LIST_TARGET_PROJ, CSetupEditorCollaboration::OnBtnSelectFromListTargetProj)
	EVT_BUTTON(ID_BUTTON_SELECT_FROM_LIST_FREE_TRANS_PROJ, CSetupEditorCollaboration::OnBtnSelectFromListFreeTransProj)
	EVT_BUTTON(wxID_OK, CSetupEditorCollaboration::OnOK)
	//EVT_MENU(ID_SOME_MENU_ITEM, CSetupEditorCollaboration::OnDoSomething)
	//EVT_UPDATE_UI(ID_SOME_MENU_ITEM, CSetupEditorCollaboration::OnUpdateDoSomething)
	//EVT_BUTTON(ID_SOME_BUTTON, CSetupEditorCollaboration::OnDoSomething)
	//EVT_CHECKBOX(ID_SOME_CHECKBOX, CSetupEditorCollaboration::OnDoSomething)
	//EVT_RADIOBUTTON(ID_SOME_RADIOBUTTON, CSetupEditorCollaboration::DoSomething)
	//EVT_LISTBOX(ID_SOME_LISTBOX, CSetupEditorCollaboration::DoSomething)
	EVT_COMBOBOX(ID_COMBO_AI_PROJECTS, CSetupEditorCollaboration::OnComboBoxSelectAiProject)
	EVT_TEXT(ID_TEXTCTRL_SRC_LANG_NAME, CSetupEditorCollaboration::OnEnChangeSrcLangName)
	EVT_TEXT(ID_TEXTCTRL_TGT_LANG_NAME, CSetupEditorCollaboration::OnEnChangeTgtLangName)
	// ... other menu, button or control events
END_EVENT_TABLE()

CSetupEditorCollaboration::CSetupEditorCollaboration(wxWindow* parent) // dialog constructor
: AIModalDialog(parent, -1, _("Set Up %s Collaboration"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pSetupEditorCollabSizer = SetupEditorCollaborationFunc(this, TRUE, TRUE);
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

	pTextCtrlAsStaticNewAIProjName = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_NEW_AI_PROJ_NAME);
	wxASSERT(pTextCtrlAsStaticNewAIProjName != NULL);
	pTextCtrlAsStaticNewAIProjName->SetBackgroundColour(sysColorBtnFace);

	pTextCtrlAsStaticSelectedSourceProj = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_SELECTED_SRC_PROJ);
	wxASSERT(pTextCtrlAsStaticSelectedSourceProj != NULL);
	pTextCtrlAsStaticSelectedSourceProj->SetBackgroundColour(sysColorBtnFace);

	pTextCtrlAsStaticSelectedTargetProj = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_SELECTED_TGT_PROJ);
	wxASSERT(pTextCtrlAsStaticSelectedTargetProj != NULL);
	pTextCtrlAsStaticSelectedTargetProj->SetBackgroundColour(sysColorBtnFace);

	pTextCtrlAsStaticSelectedFreeTransProj = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_SELECTED_FREE_TRANS_PROJ);
	wxASSERT(pTextCtrlAsStaticSelectedFreeTransProj != NULL);
	pTextCtrlAsStaticSelectedFreeTransProj->SetBackgroundColour(sysColorBtnFace);

	pStaticTextEnterLangNames = (wxStaticText*)FindWindowById(ID_TEXT_EXISTS_OR_WILL_BE_CREATED);
	wxASSERT(pStaticTextEnterLangNames != NULL);

	pStaticTextNewAIProjName = (wxStaticText*)FindWindowById(ID_TEXT_AS_STATIC_NEW_AI_PROJ_NAME);
	wxASSERT(pStaticTextNewAIProjName != NULL);
	
	pTextCtrlSourceLanguageName = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SRC_LANG_NAME);
	wxASSERT(pTextCtrlSourceLanguageName != NULL);
	
	pTextCtrlTargetLanguageName = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_TGT_LANG_NAME);
	wxASSERT(pTextCtrlTargetLanguageName != NULL);
	
	pStaticTextSrcLangName = (wxStaticText*)FindWindowById(ID_TEXT_SRC_NAME_LABEL);
	wxASSERT(pStaticTextSrcLangName != NULL);
	
	pStaticTextTgtLangName = (wxStaticText*)FindWindowById(ID_TEXT_TGT_NAME_LABEL);
	wxASSERT(pStaticTextTgtLangName != NULL);

	pStaticTextSelectWhichProj = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_SELECT_WHICH_PROJECTS);
	wxASSERT(pStaticTextSelectWhichProj != NULL);
	
	pStaticTextSelectAThirdProj = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_SELECT_THIRD_PROJECT);
	wxASSERT(pStaticTextSelectAThirdProj != NULL);

	pStaticTextSrcTextxFromThisProj = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_SRC_FROM_THIS_PROJECT);
	wxASSERT(pStaticTextSrcTextxFromThisProj != NULL);

	pStaticTextTgtTextxToThisProj = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_TARGET_TO_THIS_PROJECT);
	wxASSERT(pStaticTextTgtTextxToThisProj != NULL);

	pStaticTextFtTextxToThisProj = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_TO_THIS_FT_PROJECT);
	wxASSERT(pStaticTextFtTextxToThisProj != NULL);

	pListOfProjects = (wxListBox*)FindWindowById(IDC_LIST_OF_COLLAB_PROJECTS);
	wxASSERT(pListOfProjects != NULL);

	pComboAiProjects = (wxComboBox*)FindWindowById(ID_COMBO_AI_PROJECTS);
	wxASSERT(pComboAiProjects != NULL);

	pRadioBoxCollabOnOrOff = (wxRadioBox*)FindWindowById(ID_RADIOBOX_COLLABORATION_ON_OFF);
	wxASSERT(pRadioBoxCollabOnOrOff != NULL);

	pBtnSelectFmListSourceProj = (wxButton*)FindWindowById(ID_BUTTON_SELECT_FROM_LIST_SOURCE_PROJ);
	wxASSERT(pBtnSelectFmListSourceProj != NULL);

	pBtnSelectFmListTargetProj = (wxButton*)FindWindowById(ID_BUTTON_SELECT_FROM_LIST_TARGET_PROJ);
	wxASSERT(pBtnSelectFmListTargetProj != NULL);

	pBtnSelectFmListFreeTransProj = (wxButton*)FindWindowById(ID_BUTTON_SELECT_FROM_LIST_FREE_TRANS_PROJ);
	wxASSERT(pBtnSelectFmListFreeTransProj != NULL);

	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	// other attribute initializations
}

CSetupEditorCollaboration::~CSetupEditorCollaboration() // destructor
{
	
}

void CSetupEditorCollaboration::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	wxString curTitle = this->GetTitle();
	curTitle = curTitle.Format(curTitle,m_pApp->m_collaborationEditor.c_str());
	this->SetTitle(curTitle);
	
	// initialize our dialog temp variables from those held on the App
	m_bTempCollaboratingWithParatext = m_pApp->m_bCollaboratingWithParatext;
	m_bTempCollaboratingWithBibledit = m_pApp->m_bCollaboratingWithBibledit;
	m_TempCollabProjectForSourceInputs = m_pApp->m_CollabProjectForSourceInputs;
	m_TempCollabProjectForTargetExports = m_pApp->m_CollabProjectForTargetExports;
	m_TempCollabProjectForFreeTransExports = m_pApp->m_CollabProjectForFreeTransExports;
	m_TempCollabAIProjectName = m_pApp->m_CollabAIProjectName; // whm added 7Sep11
	m_bTempCollaborationExpectsFreeTrans = m_pApp->m_bCollaborationExpectsFreeTrans; // whm added 6Jul11
	m_TempCollabSourceProjLangName = m_pApp->m_CollabSourceLangName; // whm added 4Sep11
	m_TempCollabTargetProjLangName = m_pApp->m_CollabTargetLangName; // whm added 4Sep11

	m_projectSelectionMade = FALSE;

	// adjust static strings substituting "Paratext" or "Bibledit" depending on
	// the string value in m_collaborationEditor
	// Substitute strings in the top edit box
	wxString text = pTextCtrlAsStaticTopNote->GetValue(); // text has four %s sequences
	text = text.Format(text, m_pApp->m_collaborationEditor.c_str(), 
		m_pApp->m_collaborationEditor.c_str(), m_pApp->m_collaborationEditor.c_str(),m_pApp->m_collaborationEditor.c_str());
	pTextCtrlAsStaticTopNote->ChangeValue(text);

	// Substitute strings in the static text above the list box
	text = pStaticTextListOfProjects->GetLabel();
	text = text.Format(text,m_pApp->m_collaborationEditor.c_str());
	pStaticTextListOfProjects->SetLabel(text);

	// Substitute strings in the static text of point 2
	text = pStaticTextSelectWhichProj->GetLabel();
	text = text.Format(text,m_pApp->m_collaborationEditor.c_str());
	pStaticTextSelectWhichProj->SetLabel(text);
	
	text = pStaticTextSelectAThirdProj->GetLabel();
	text = text.Format(text,m_pApp->m_collaborationEditor.c_str());
	pStaticTextSelectAThirdProj->SetLabel(text);
	
	text = pStaticTextSrcTextxFromThisProj->GetLabel();
	text = text.Format(text,m_pApp->m_collaborationEditor.c_str());
	pStaticTextSrcTextxFromThisProj->SetLabel(text);

	text = pStaticTextTgtTextxToThisProj->GetLabel();
	text = text.Format(text,m_pApp->m_collaborationEditor.c_str());
	pStaticTextTgtTextxToThisProj->SetLabel(text);

	text = pStaticTextFtTextxToThisProj->GetLabel();
	text = text.Format(text,m_pApp->m_collaborationEditor.c_str());
	pStaticTextFtTextxToThisProj->SetLabel(text);

	// start with 3a items hidden
	pTextCtrlAsStaticNewAIProjName->Hide();
	pStaticTextEnterLangNames->Hide();
	pTextCtrlSourceLanguageName->Hide();
	pTextCtrlTargetLanguageName->Hide();
	pTextCtrlAsStaticNewAIProjName->Hide();
	pStaticTextNewAIProjName->Hide();
	pStaticTextSrcLangName->Hide();
	pStaticTextTgtLangName->Hide();

	if (m_bTempCollaboratingWithParatext || m_bTempCollaboratingWithBibledit)
		pRadioBoxCollabOnOrOff->SetSelection(0);
	else
		pRadioBoxCollabOnOrOff->SetSelection(1);
	
	// Substitute strings for the radio box's button labels
	text = pRadioBoxCollabOnOrOff->GetString(0);
	text = text.Format(text,m_pApp->m_collaborationEditor.c_str());
	pRadioBoxCollabOnOrOff->SetString(0,text);
	text = pRadioBoxCollabOnOrOff->GetString(1);
	text = text.Format(text,m_pApp->m_collaborationEditor.c_str());
	pRadioBoxCollabOnOrOff->SetString(1,text);
	pSetupEditorCollabSizer->Layout();

	int nProjectCount = 0;
	// get list of PT/BE projects
	if (m_pApp->m_collaborationEditor == _T("Paratext"))
	{
		m_pApp->m_ListOfPTProjects.Clear();
		m_pApp->m_ListOfPTProjects = m_pApp->GetListOfPTProjects();
		nProjectCount = (int)m_pApp->m_ListOfPTProjects.GetCount();
	}
	else if (m_pApp->m_collaborationEditor == _T("Bibledit"))
	{
		m_pApp->m_ListOfBEProjects.Clear();
		m_pApp->m_ListOfBEProjects = m_pApp->GetListOfBEProjects();
		nProjectCount = (int)m_pApp->m_ListOfBEProjects.GetCount();
	}

	// Check for at least two usable PT projects in list
	if (nProjectCount < 2)
	{
		// error: PT/BE is not set up with enough projects for collaboration
	}
	else
	{
		int i;
		for (i = 0; i < nProjectCount; i++)
		{
			wxString tempStr;
			if (m_pApp->m_collaborationEditor == _T("Paratext"))
			{
				tempStr = m_pApp->m_ListOfPTProjects.Item(i);
			}
			else if (m_pApp->m_collaborationEditor == _T("Bibledit"))
			{
				tempStr = m_pApp->m_ListOfBEProjects.Item(i);
			}
			pListOfProjects->Append(tempStr);
		}

	}

	// fill in the 3 read-only and 2 editable edit boxes
	pTextCtrlAsStaticSelectedSourceProj->ChangeValue(m_TempCollabProjectForSourceInputs);
	pTextCtrlAsStaticSelectedTargetProj->ChangeValue(m_TempCollabProjectForTargetExports);
	pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(m_TempCollabProjectForFreeTransExports);
	pTextCtrlSourceLanguageName->ChangeValue(m_TempCollabSourceProjLangName);
	pTextCtrlTargetLanguageName->ChangeValue(m_TempCollabTargetProjLangName);
	
	// Load potential AI projects into the pComboAiProjects combo box, and in the process
	// check for the existence of the aiProjectFolder project. Select it if it is in the
	// list.
	wxArrayString aiProjectNamesArray;
	bool bAiProjectFound = FALSE;
	int indexOfFoundProject = wxNOT_FOUND;
	m_pApp->GetPossibleAdaptionProjects(&aiProjectNamesArray);
	aiProjectNamesArray.Sort();
	
	//const wxString createNewProjectInstead = _("<Create a new project instead>");
	// insert the "<Create a new project instead>" item at the beginning
	// of the sorted list of AI projects
	aiProjectNamesArray.Insert(createNewProjectInstead,0);
	
	wxString aiProjectFolder = _T("");
	// Get an AI project name. 
	// Note: This assumes that the m_TempCollabAIProjectName, m_TempCollabSourceProjLangName,
	// and m_TempCollabTargetProjLangName, are set from the basic config file because
	// at this point in InitDialog, only the basic config file will have been read in.
	aiProjectFolder = GetAIProjectFolderForCollab(m_TempCollabAIProjectName, 
				m_TempCollabSourceProjLangName, m_TempCollabTargetProjLangName, 
				m_TempCollabProjectForSourceInputs, m_TempCollabProjectForTargetExports);
	
	// Locate and select the aiProjectFolder
	pComboAiProjects->Clear();
	int ct;
	for (ct = 0; ct < (int)aiProjectNamesArray.GetCount(); ct++)
	{
		pComboAiProjects->Append(aiProjectNamesArray.Item(ct));
		if (aiProjectNamesArray.Item(ct) == aiProjectFolder)
		{
			// workFolder exists as an AI project folder
			indexOfFoundProject = ct;
			bAiProjectFound = TRUE;
		}
	}
	if (indexOfFoundProject != wxNOT_FOUND)
	{
		pComboAiProjects->Select(indexOfFoundProject);
	}
}

// event handling functions

void CSetupEditorCollaboration::OnBtnSelectFromListSourceProj(wxCommandEvent& WXUNUSED(event))
{
	// Note: For source project, the project can be readable or writeable (all in list)
	// The OnOK() handler will check to ensure that the project selected for source
	// text inputs is different from the project selected for target text exports.
	
	// use a temporary array list for the list of projects
	wxArrayString tempListOfProjects;
	tempListOfProjects.Add(_("[No Project Selected]"));
	int ct,tot;
	if (m_pApp->m_collaborationEditor == _T("Paratext"))
	{
		tot = (int)m_pApp->m_ListOfPTProjects.GetCount();
		for (ct = 0; ct < tot; ct++)
		{
			// load the rest of the projects into the temp array list
			tempListOfProjects.Add(m_pApp->m_ListOfPTProjects.Item(ct));
		}
	}
	else if (m_pApp->m_collaborationEditor == _T("Bibledit"))
	{
		tot = (int)m_pApp->m_ListOfBEProjects.GetCount();
		for (ct = 0; ct < tot; ct++)
		{
			// load the rest of the projects into the temp array list
			tempListOfProjects.Add(m_pApp->m_ListOfBEProjects.Item(ct));
		}
	}
	wxString msg;
	msg = _("Choose a default project the user will see initially for source text inputs");
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
	int userSelectionInt;
	wxString userSelectionStr;
	userSelectionStr.Empty();
	if (ChooseProjectForSourceTextInputs.ShowModal() == wxID_OK)
	{
		userSelectionStr = ChooseProjectForSourceTextInputs.GetStringSelection();
		userSelectionInt = ChooseProjectForSourceTextInputs.GetSelection();
		m_projectSelectionMade = TRUE;
		m_TempCollabSourceProjLangName = GetLanguageNameFromProjectName(userSelectionStr);
		if (m_TempCollabSourceProjLangName.IsEmpty())
		{
			// There was no Language field in the project name composite string
			// that was selected, indicating that we are probably working with 
			// Bibledit, so remind the administrator to type a source language
			// name in the edit box that corresponds to any existing AI project
			// or that will be used to create an AI project for collaboration.
			wxString msg = _("Please use the drop-down box below to select or create an Adapt It project for collaboration purposes.");
			wxMessageBox(msg,_("Unknown Source Language Name"),wxICON_INFORMATION);
		}
		m_pApp->LogUserAction(_T("Selected Project for Source Text Inputs"));
	}
	else
	{
		// user cancelled, don't change anything, just return
		return;
	}
	// if we get here the administrator made a selection, show the
	// composite project string in the read-only edit box, get the
	// computed language name from the composite userSelectionStr
	// and save it in m_TempCollabSourceProjLangName so that it will 
	// get assigned to the App's value in OnOK() for the project 
	// config file, and then show the computed Language name in the 
	// Source Language Name edit box.
	pTextCtrlAsStaticSelectedSourceProj->ChangeValue(userSelectionStr);
	m_TempCollabSourceProjLangName = GetLanguageNameFromProjectName(userSelectionStr);
	pTextCtrlSourceLanguageName->ChangeValue(m_TempCollabSourceProjLangName);
}

void CSetupEditorCollaboration::OnBtnSelectFromListTargetProj(wxCommandEvent& WXUNUSED(event))
{
	// Note: For target project, we must ensure that the Paratext project is writeable
	// use a temporary array list for the list of projects
	wxArrayString tempListOfProjects;
	wxString projShortName;
	tempListOfProjects.Add(_("[No Project Selected]"));
	int ct,tot;
	if (m_pApp->m_collaborationEditor == _T("Paratext"))
	{
		tot = (int)m_pApp->m_ListOfPTProjects.GetCount();
		for (ct = 0; ct < tot; ct++)
		{
			// Load the rest of the projects into the temp array list.
			// We must restrict the list of potential destination projects to those
			// which have the <Editable>T</Editable> attribute
			projShortName = GetShortNameFromProjectName(m_pApp->m_ListOfPTProjects.Item(ct));
			if (CollabProjectIsEditable(projShortName))
			{
				tempListOfProjects.Add(m_pApp->m_ListOfPTProjects.Item(ct));
			}
		}
	}
	else if (m_pApp->m_collaborationEditor == _T("Bibledit"))
	{
		tot = (int)m_pApp->m_ListOfBEProjects.GetCount();
		for (ct = 0; ct < tot; ct++)
		{
			// load the rest of the projects into the temp array list
			// We must restrict the list of potential destination projects to those
			// which have the <Editable>T</Editable> attribute
			projShortName = GetShortNameFromProjectName(m_pApp->m_ListOfBEProjects.Item(ct));
			if (CollabProjectIsEditable(projShortName))
			{
				tempListOfProjects.Add(m_pApp->m_ListOfBEProjects.Item(ct));
			}
		}
	}
	wxString msg;
	msg = _("Choose a default project the user will see initially for translation text exports");
	wxSingleChoiceDialog ChooseProjectForTargetTextInputs(this,msg,_("Select a project from this list"),tempListOfProjects);
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
		ChooseProjectForTargetTextInputs.SetSelection(nPreselectedProjIndex);
	}
	int userSelectionInt;
	wxString userSelectionStr;
	userSelectionStr.Empty();
	if (ChooseProjectForTargetTextInputs.ShowModal() == wxID_OK)
	{
		userSelectionStr = ChooseProjectForTargetTextInputs.GetStringSelection();
		userSelectionInt = ChooseProjectForTargetTextInputs.GetSelection();
		m_projectSelectionMade = TRUE;
		// The basic config file did not have a value stored for the target
		// project's language name, so get a default one from the project
		m_TempCollabTargetProjLangName = GetLanguageNameFromProjectName(userSelectionStr);
		if (m_TempCollabTargetProjLangName.IsEmpty())
		{
			// There was no Language field in the project name composite string
			// that was selected, indicating that we are probably working with 
			// Bibledit, so remind the administrator to type a target language
			// name in the edit box that corresponds to any existing AI project
			// or that will be used to create an AI project for collaboration.
			wxString msg = _("Please use the drop-down box below to select or create an Adapt It project for collaboration purposes.");
			wxMessageBox(msg,_("Unknown Target Language Name"),wxICON_INFORMATION);
		}
		m_pApp->LogUserAction(_T("Selected Project for Target Text Inputs"));
	}
	else
	{
		// user cancelled, don't change anything, just return
		return;
	}
	// if we get here the administrator made a selection, show the
	// composite project string in the read-only edit box, get the
	// computed language name from the composite userSelectionStr
	// and save it in m_TempCollabTargetProjLangName so that it will 
	// get assigned to the App's value in OnOK() for the project 
	// config file, and then show the computed Language name in the 
	// Target Language Name edit box.
	pTextCtrlAsStaticSelectedTargetProj->ChangeValue(userSelectionStr);
	m_TempCollabTargetProjLangName = GetLanguageNameFromProjectName(userSelectionStr);
	pTextCtrlTargetLanguageName->ChangeValue(m_TempCollabTargetProjLangName);
}

void CSetupEditorCollaboration::OnBtnSelectFromListFreeTransProj(wxCommandEvent& WXUNUSED(event))
{
	// Note: For free trans project, we must ensure that the Paratext project is writeable
	// use a temporary array list for the list of projects
	wxArrayString tempListOfProjects;
	wxString projShortName;
	tempListOfProjects.Add(_("[No Project Selected]"));
	int ct,tot;
	if (m_pApp->m_collaborationEditor == _T("Paratext"))
	{
		tot = (int)m_pApp->m_ListOfPTProjects.GetCount();
		for (ct = 0; ct < tot; ct++)
		{
			// Load the rest of the projects into the temp array list.
			// We must restrict the list of potential destination projects to those
			// which have the <Editable>T</Editable> attribute
			projShortName = GetShortNameFromProjectName(m_pApp->m_ListOfPTProjects.Item(ct));
			if (CollabProjectIsEditable(projShortName))
			{
				tempListOfProjects.Add(m_pApp->m_ListOfPTProjects.Item(ct));
			}
		}
	}
	else if (m_pApp->m_collaborationEditor == _T("Bibledit"))
	{
		tot = (int)m_pApp->m_ListOfBEProjects.GetCount();
		for (ct = 0; ct < tot; ct++)
		{
			// Load the rest of the projects into the temp array list.
			// We must restrict the list of potential destination projects to those
			// which have the <Editable>T</Editable> attribute
			projShortName = GetShortNameFromProjectName(m_pApp->m_ListOfBEProjects.Item(ct));
			if (CollabProjectIsEditable(projShortName))
			{
				tempListOfProjects.Add(m_pApp->m_ListOfBEProjects.Item(ct));
			}
		}
	}
	wxString msg;
	msg = _("Choose a default project the user will see initially for free translation text exports");
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
	int userSelectionInt;
	wxString userSelectionStr;
	userSelectionStr.Empty();
	if (ChooseProjectForFreeTransTextInputs.ShowModal() == wxID_OK)
	{
		userSelectionStr = ChooseProjectForFreeTransTextInputs.GetStringSelection();
		userSelectionInt = ChooseProjectForFreeTransTextInputs.GetSelection();
		m_projectSelectionMade = TRUE;
		m_pApp->LogUserAction(_T("Selected Project for Free Trans Text Inputs"));
	}
	else
	{
		// user cancelled, don't change anything, just return
		return;
	}
	pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(userSelectionStr);
	m_TempCollabProjectForFreeTransExports = userSelectionStr;
	m_bTempCollaborationExpectsFreeTrans = TRUE;
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
	if (selStr == createNewProjectInstead)
	{
		// unhide the controls in 3a
		pTextCtrlAsStaticNewAIProjName->Show();
		pStaticTextEnterLangNames->Show();
		pTextCtrlSourceLanguageName->Show();
		pTextCtrlTargetLanguageName->Show();
		pTextCtrlAsStaticNewAIProjName->Show();
		pStaticTextNewAIProjName->Show();
		pStaticTextSrcLangName->Show();
		pStaticTextTgtLangName->Show();
		pTextCtrlSourceLanguageName->Enable();
		pTextCtrlTargetLanguageName->Enable();
		pSetupEditorCollabSizer->Layout();
		// move focus to the source language name edit box
		pTextCtrlSourceLanguageName->SetFocus();
	}
	else
	{
		// The administrator selected an existing AI project from the
		// combobox. Parse the language names from the AI project name.
		m_pApp->GetSrcAndTgtLanguageNamesFromProjectName(selStr, 
			m_TempCollabSourceProjLangName,m_TempCollabTargetProjLangName);
		// Populate the Source Language Name and the Target Language Name edit 
		// boxes with the language names parsed from the combobox's selected
		// string.
		
		// To make the change persist, we need to also change the temp values
		// for the newly selected AI project that gets stored in the
		// basic config file within OnOK().
		m_TempCollabAIProjectName = selStr; 

		pTextCtrlSourceLanguageName->ChangeValue(m_TempCollabSourceProjLangName);
		pTextCtrlTargetLanguageName->ChangeValue(m_TempCollabTargetProjLangName);
		// Also, disable the Source Language Name and the Target Language Name edit 
		// boxes now that they have the computed language names in them, because 
		// they can't be changed as long as an existing AI project
		// is selected in the combobox. Note: we enable them whenever the
		// createNewProjectInstead string is selected.
		pTextCtrlSourceLanguageName->Disable();
		pTextCtrlTargetLanguageName->Disable();
		// finally, remove any project name that might have been in the "New
		// Adapt It project name edit box
		pTextCtrlAsStaticNewAIProjName->ChangeValue(wxEmptyString);
	}
}

void CSetupEditorCollaboration::OnEnChangeSrcLangName(wxCommandEvent& WXUNUSED(event))
{
	// user is editing the source language name edit box
	// update the AI project name in the "New Adapt It project name"
	// edit box
	wxString tempStrSrcProjName,tempStrTgtProjName;
	tempStrSrcProjName = pTextCtrlSourceLanguageName->GetValue();
	tempStrTgtProjName = pTextCtrlTargetLanguageName->GetValue();
	wxString projFolder = tempStrSrcProjName + _T(" to ") + tempStrTgtProjName + _T(" adaptations");
	pTextCtrlAsStaticNewAIProjName->ChangeValue(projFolder);
}

void CSetupEditorCollaboration::OnEnChangeTgtLangName(wxCommandEvent& WXUNUSED(event))
{
	// user is editing the target language name edit box
	// update the AI project name in the "New Adapt It project name"
	// edit box
	wxString tempStrSrcProjName,tempStrTgtProjName;
	tempStrSrcProjName = pTextCtrlSourceLanguageName->GetValue();
	tempStrTgtProjName = pTextCtrlTargetLanguageName->GetValue();
	wxString projFolder = tempStrSrcProjName + _T(" to ") + tempStrTgtProjName + _T(" adaptations");
	pTextCtrlAsStaticNewAIProjName->ChangeValue(projFolder);
}


//CSetupEditorCollaboration::OnUpdateDoSomething(wxUpdateUIEvent& event)
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
void CSetupEditorCollaboration::OnOK(wxCommandEvent& event) 
{
	// Check to see if book folder mode is activated. If so, warn the administrator that it
	// will be turned OFF if s/he continues setting up AI for collaboration with Paratext/Bibledit.
	if (m_pApp->m_bBookMode)
	{
		wxString msg;
		msg = msg.Format(_("Note: Book Folder Mode is currently in effect, but it must be turned OFF and disabled in order for Adapt It to collaborate with %s. If you continue, Book Folder Mode will be turned off and disabled. If you later turn collaboration OFF, you will need to turn book folder mode on again if that is desired.\n\nDo you want to continue setting up Adapt It for collaboration with %s?"),m_pApp->m_collaborationEditor.c_str(),m_pApp->m_collaborationEditor.c_str());
		if (wxMessageBox(msg,_T(""),wxYES_NO | wxICON_INFORMATION) == wxNO)
		{
			m_pApp->LogUserAction(msg);
			return;
		}
	}
	// Check to ensure that the project selected for source text inputs is different 
	// from the project selected for target text exports, unless both are [No Project 
	// Selected].
	// If the same project is selected, inform the administrator and abort the OK
	// operation so the administrator can select different projects or abort and
	// set up the needed projects in Paratext first.
	if (m_TempCollabProjectForSourceInputs == m_TempCollabProjectForTargetExports && m_TempCollabProjectForSourceInputs != _("[No Project Selected]"))
	{
		wxString msg, msg1;
		msg = _("The projects selected for getting source texts and receiving translation drafts cannot be the same.\nPlease select one project for getting source texts, and a different project for receiving translation texts.");
		//msg1 = _("(or, if you select \"[No Project Selected]\" for a project here, the first time a source text is needed for adaptation, the user will have to choose a project from a drop down list of projects).");
		//msg = msg + _T("\n") + msg1;
		wxMessageBox(msg);
		m_pApp->LogUserAction(msg);
		return; // don't accept any changes - abort the OnOK() handler
	}
	
	if (m_bTempCollaborationExpectsFreeTrans && m_TempCollabProjectForSourceInputs == m_TempCollabProjectForFreeTransExports)
	{
		wxString msg, msg1;
		msg = _("The projects selected for getting source texts and receiving free translation texts cannot be the same.\nPlease select one project for getting source texts, and a different project for receiving free translation texts.");
		//msg1 = _("(or, if you select \"[No Project Selected]\" for a project here, the first time a source text is needed for adaptation, the user will have to choose a project from a drop down list of projects).");
		//msg = msg + _T("\n") + msg1;
		wxMessageBox(msg);
		m_pApp->LogUserAction(msg);
		return; // don't accept any changes - abort the OnOK() handler
	}
	
	// whm added 7Sep11. The user may have changed the AI project to collaborate with
	// or changed the source language or target langauge names, so ensure that
	// we have the current values. But, do this only if the admin/user selected
	// <Create a new project instead> in the combo box.
	if (pComboAiProjects->GetStringSelection() == createNewProjectInstead)
	{
		m_TempCollabSourceProjLangName = pTextCtrlSourceLanguageName->GetValue();
		m_TempCollabTargetProjLangName = pTextCtrlTargetLanguageName->GetValue();
	}

	if (m_TempCollabSourceProjLangName.IsEmpty())
	{
		wxString msg, msg1, msgTitle;
		msg1 = _("Please enter a Source Language Name.");
		msg = msg1;
		msg += _T(' ');
		msg += _("Adapt It will use this name to identify any existing project folder (or to create a new project folder) of the form: \"<source name> to <target name> adaptations\".");
		msgTitle = _("No Source Language Name entered");
		wxMessageBox(msg);
		// set focus on the edit box with missing data
		pTextCtrlSourceLanguageName->SetFocus();
		m_pApp->LogUserAction(msgTitle);
		return; // don't accept any changes - abort the OnOK() handler
	}

	if (m_TempCollabTargetProjLangName.IsEmpty())
	{
		wxString msg, msg1, msgTitle;
		msg1 = _("Please enter a Target Language Name.");
		msg = msg1;
		msg += _T(' ');
		msg += _("Adapt It will use this name to identify any existing project folder (or to create a new project folder) of the form: \"<source name> to <target name> adaptations\".");
		msgTitle = _("No Target Language Name entered");
		wxMessageBox(msg);
		// set focus on the edit box with missing data
		pTextCtrlTargetLanguageName->SetFocus();
		m_pApp->LogUserAction(msgTitle);
		return; // don't accept any changes - abort the OnOK() handler
	}

	if (!m_TempCollabSourceProjLangName.IsEmpty() && !m_TempCollabTargetProjLangName.IsEmpty())
	{
		// Both the source and the target language name edit boxes have content.
		// Check to see if there is an existing AI project representing those
		// names. If not, notify the administrator of that fact, and that if 
		// he continues AI will create a new project folder called 
		// "<source name> to <target name> adaptations" in Adapt It's current 
		// work folder.
		wxString aiProjFolder = m_TempCollabSourceProjLangName + _T(" to ") + m_TempCollabTargetProjLangName + _T(" adaptations");
		wxArrayString possibleAIProjects;
		possibleAIProjects.Clear();
		m_pApp->GetPossibleAdaptionProjects(&possibleAIProjects);
		int ct, tot;
		wxString msg;
		tot = (int)possibleAIProjects.GetCount();
		if (tot == 0)
		{
			// Notify the administrator that AI will create a new project folder 
			// for use in collaboration.
			msg = _("Adapt It will create an new project folder called \"%s\" for use in collaboration with %s");
			msg = msg.Format(msg, aiProjFolder.c_str(), m_pApp->m_collaborationEditor.c_str());
			wxMessageBox(msg,_T(""),wxICON_INFORMATION);
		}
		else
		{
			// Check existing AI projects to see if an existing project will be 
			// used. If one doesn't exist notify the administrator.
			bool bProjectFound = FALSE;
			for (ct = 0; ct < tot; ct++)
			{
				wxString tempStr = possibleAIProjects.Item(ct);
				if (aiProjFolder == tempStr)
				{
					bProjectFound = TRUE;
					break;
				}
			}
			if (!bProjectFound)
			{
				msg = _("Adapt It will create an new project folder called \"%s\" for use in collaboration with %s");
				msg = msg.Format(msg, aiProjFolder.c_str(), m_pApp->m_collaborationEditor.c_str());
				wxString msg1;
				msg1 += _T(' ');
				msg1 += _("If this is OK select Yes, or select No to change the language name(s)");
				int result = 0;
				result = wxMessageBox(msg,_T(""),wxYES_NO);
				if (result == wxNO)
					return;
			}
		}
	}

	// /////////////////////////////////////////////////////////////////////////////////////////
	// whm Note: Put code that aborts the OnOK() handler above this point
	// /////////////////////////////////////////////////////////////////////////////////////////

	// Set the App values for the PT projects to be used for PT collaboration
	m_pApp->m_CollabProjectForSourceInputs = m_TempCollabProjectForSourceInputs;
	m_pApp->m_CollabProjectForTargetExports = m_TempCollabProjectForTargetExports;
	m_pApp->m_CollabProjectForFreeTransExports = m_TempCollabProjectForFreeTransExports;
	m_pApp->m_CollabAIProjectName = m_TempCollabAIProjectName;
	m_pApp->m_bCollaborationExpectsFreeTrans = m_bTempCollaborationExpectsFreeTrans;
	m_pApp->m_CollabSourceLangName = m_TempCollabSourceProjLangName;
	m_pApp->m_CollabTargetLangName = m_TempCollabTargetProjLangName;

	m_pApp->LogUserAction(m_pApp->m_collaborationEditor);
	if (m_pApp->m_bCollabByChapterOnly)
		m_pApp->LogUserAction(_T("Collab Type: Chapter Only"));
	else
		m_pApp->LogUserAction(_T("Collab Type: Whole Book"));

	// Get the state of collaboration
	int nSel;
	nSel = pRadioBoxCollabOnOrOff->GetSelection();
	if (nSel == 0)
	{
		// Collaboration radio button is ON
		if (m_pApp->m_collaborationEditor == _T("Bibledit"))
		{
			m_pApp->m_bCollaboratingWithBibledit = TRUE;
			m_pApp->LogUserAction(_T("Collab with Bibledit ON"));
		}
		else
		{
			m_pApp->m_bCollaboratingWithParatext = TRUE;
			m_pApp->LogUserAction(_T("Collab with Paratext ON"));
		}
	}
	else if (nSel == 1)
	{
		// Collaboration radio button is OFF
		if (m_pApp->m_collaborationEditor == _T("Bibledit"))
		{
			m_pApp->m_bCollaboratingWithBibledit = FALSE;
			m_pApp->LogUserAction(_T("Collab with Bibledit OFF"));
		}
		else
		{
			m_pApp->m_bCollaboratingWithParatext = FALSE;
			m_pApp->LogUserAction(_T("Collab with Paratext OFF"));
		}
		
		// PT/BE collaboration is OFF, but selection(s) were made using the "Select from List"
		// button(s). Ask the admin if he really wanted to turn PT/BE collaboration ON or not.
		if (m_projectSelectionMade && pRadioBoxCollabOnOrOff->GetSelection() == 1)
		{
			wxString msg;
			msg = msg.Format(_("You pre-selected one or both projects, but you did not turn %s collaboration ON. Do you want to turn on %s collaboration now?"),
				m_pApp->m_collaborationEditor.c_str(),m_pApp->m_collaborationEditor.c_str());
			int result = wxMessageBox(msg,_("Collaboration is currently turned OFF"),wxYES_NO);
			if (result == wxYES)
			{
				// Admin actually wanted Collaboration ON
				if (m_pApp->m_collaborationEditor == _T("Bibledit"))
				{
					m_pApp->m_bCollaboratingWithBibledit = TRUE;
					m_pApp->LogUserAction(_T("Collab with Bibledit ON"));
				}
				else
				{
					m_pApp->m_bCollaboratingWithParatext = TRUE;
					m_pApp->LogUserAction(_T("Collab with Paratext ON"));
				}
			}
		}
	}
	else
	{
		wxASSERT_MSG(FALSE,_T("Programming Error: Wrong pRadioBoxCollabOnOrOff index in CSetupEditorCollaboration::OnOK()."));
	}

	// ================================================================================
	// whm 8Sep11 NOTE: The administrator's settings are saved below in the Adapt_It_WX.ini
	// file. This is a safeguard that enables the settings to be restored if the user does
	// a SHIFT-DOWN startup. In GetSourceTextFromEditor.cpp, however, we don't save the
	// collaboration settings that the user may have changed - via selecting a different AI
	// project to hook up with or even using the <Create a new project instead> selection
	// in the drop-down combo box list of AI project). That is by design, so that if the 
	// user has fouled things up because of making one of those selections to point AI to 
	// a different/new AI project by mistake, the user can still (be told to) do a 
	// SHIFT-DOWN startup and get back the settings that the administrator made when he 
	// set things up initially in SetupEditorCollaboration.
	// ================================================================================

	// update the values related to PTcollaboration in the Adapt_It_WX.ini file
	bool bWriteOK = FALSE;
	wxString oldPath = m_pApp->m_pConfig->GetPath(); // is always absolute path "/Recent_File_List"
	m_pApp->m_pConfig->SetPath(_T("/Settings"));
	if (m_pApp->m_bCollaboratingWithParatext == TRUE)
	{
		wxLogNull logNo; // eliminates spurious message from the system
		bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collaboration"), m_pApp->m_bCollaboratingWithParatext);
		bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_src_proj"), m_pApp->m_CollabProjectForSourceInputs);
		bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_tgt_proj"), m_pApp->m_CollabProjectForTargetExports);
		bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_free_trans_proj"), m_pApp->m_CollabProjectForFreeTransExports);
		bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_ai_proj_name"), m_pApp->m_CollabAIProjectName);
		bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_book_selected"), m_pApp->m_CollabBookSelected);
		bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_by_chapter_only"), m_pApp->m_bCollabByChapterOnly);
		bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_chapter_selected"), m_pApp->m_CollabChapterSelected);
		bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_src_lang_name"), m_pApp->m_CollabSourceLangName);
		bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_tgt_lang_name"), m_pApp->m_CollabTargetLangName);
		
		m_pApp->m_pConfig->Flush(); // write now, otherwise write takes place when m_pConfig is destroyed in OnExit().
	}
	// update the values related to BE collaboration in the Adapt_It_WX.ini file
	if (m_pApp->m_bCollaboratingWithBibledit == TRUE)
	{
		wxLogNull logNo; // eliminates spurious message from the system
		bWriteOK = m_pApp->m_pConfig->Write(_T("be_collaboration"), m_pApp->m_bCollaboratingWithBibledit);
		bWriteOK = m_pApp->m_pConfig->Write(_T("be_collab_src_proj"), m_pApp->m_CollabProjectForSourceInputs);
		bWriteOK = m_pApp->m_pConfig->Write(_T("be_collab_tgt_proj"), m_pApp->m_CollabProjectForTargetExports);
		bWriteOK = m_pApp->m_pConfig->Write(_T("be_collab_free_trans_proj"), m_pApp->m_CollabProjectForFreeTransExports);
		bWriteOK = m_pApp->m_pConfig->Write(_T("be_collab_ai_proj_name"), m_pApp->m_CollabAIProjectName);
		bWriteOK = m_pApp->m_pConfig->Write(_T("be_collab_book_selected"), m_pApp->m_CollabBookSelected);
		bWriteOK = m_pApp->m_pConfig->Write(_T("be_collab_by_chapter_only"), m_pApp->m_bCollabByChapterOnly);
		bWriteOK = m_pApp->m_pConfig->Write(_T("be_collab_chapter_selected"), m_pApp->m_CollabChapterSelected);
		bWriteOK = m_pApp->m_pConfig->Write(_T("be_collab_src_lang_name"), m_pApp->m_CollabSourceLangName);
		bWriteOK = m_pApp->m_pConfig->Write(_T("be_collab_tgt_lang_name"), m_pApp->m_CollabTargetLangName);
		m_pApp->m_pConfig->Flush(); // write now, otherwise write takes place when m_pConfig is destroyed in OnExit().
	}
	// restore the oldPath back to "/Recent_File_List"
	m_pApp->m_pConfig->SetPath(oldPath);

	// Configure the menu interface for collaboration. The menu interface changes have
	// to be done in such a way that they are compatible with the user profile changes to the
	// user interface.
	// Some menu changes may need to override any user profile allowed menu items:
	
	if (m_pApp->m_bCollaboratingWithParatext || m_pApp->m_bCollaboratingWithBibledit)
	{
		// disallow Book mode, and prevent user from turning it on
		m_pApp->m_bBookMode = FALSE;
		m_pApp->m_pCurrBookNamePair = NULL;
		m_pApp->m_nBookIndex = -1;
		m_pApp->m_bDisableBookMode = TRUE;
		wxASSERT(m_pApp->m_bDisableBookMode);
	}
	else
	{
		// all collaboration was turned OFF, so allow book folder mode to be changed 
		// if desired
		m_pApp->m_bDisableBookMode = FALSE;
	}
	
	// Disabling of other menu items is done below in MakeMenuInitializationsAndPlatformAdjustments().
	
	// Other menu changes may involve adding tests within the menu item handler
	// such as a test within the File's "UnPack Document..." command to prevent 
	// the unpacking of documents that are not pertinent to the Paratext projects

	// Close any open project and/or document if any are open
	CAdapt_ItView* pView = m_pApp->GetView();
	if (pView != NULL)
	{
		m_pApp->GetView()->CloseProject();
	}

	// whm added 3Jul11 Need to call MakeMenuInitializationsAndPlatformAdjustments() here to 
	// immediately add/remove the parenthetical info to the File > Open... and File > Save menu 
	// labels.
	m_pApp->MakeMenuInitializationsAndPlatformAdjustments();
	
	// Force the Main Frame's OnIdle() handler to call DoStartupWizardOnLaunch()
	// which, when m_bCollaboratingWithParatext is TRUE, actually calls the "Get
	// Source Text from Paratext Project" dialog instead of the normal startup
	// wizard. When not collaborating with Paratext, the usual Start Working
	// Wizard will appear.
	m_pApp->m_bJustLaunched = TRUE;

	// update status bar info (BEW added 27Jun11) - copy & tweak from app's OnInit()
	wxStatusBar* pStatusBar = m_pApp->GetMainFrame()->GetStatusBar(); //CStatusBar* pStatusBar;
	if (m_pApp->m_bCollaboratingWithBibledit || m_pApp->m_bCollaboratingWithParatext)
	{
		wxString message = _("Collaborating with ");
		message += m_pApp->m_collaborationEditor;
		pStatusBar->SetStatusText(message,0); // use first field 0
	}
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

