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
	//EVT_COMBOBOX(ID_SOME_COMBOBOX, CSetupEditorCollaboration::DoSomething)
	//EVT_TEXT(IDC_SOME_EDIT_CTRL, CSetupEditorCollaboration::OnEnChangeEditSomething)
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
	
	pStaticTextCtrlTopNote = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TOP_NOTE);
	wxASSERT(pStaticTextCtrlTopNote != NULL);
	pStaticTextCtrlTopNote->SetBackgroundColour(sysColorBtnFace);

	pStaticTextListOfProjects = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_LIST_OF_PROJECTS);
	wxASSERT(pStaticTextListOfProjects != NULL);

	pStaticTextCtrlImportantBottomNote = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_IMPORTANT_BOTTOM_NOTE);
	wxASSERT(pStaticTextCtrlImportantBottomNote != NULL);
	pStaticTextCtrlImportantBottomNote->SetBackgroundColour(sysColorBtnFace);

	pStaticTextCtrlSelectedSourceProj = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_SELECTED_SRC_PROJ);
	wxASSERT(pStaticTextCtrlSelectedSourceProj != NULL);
	pStaticTextCtrlSelectedSourceProj->SetBackgroundColour(sysColorBtnFace);

	pStaticTextCtrlSelectedTargetProj = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_SELECTED_TGT_PROJ);
	wxASSERT(pStaticTextCtrlSelectedTargetProj != NULL);
	pStaticTextCtrlSelectedTargetProj->SetBackgroundColour(sysColorBtnFace);

	pStaticTextCtrlSelectedFreeTransProj = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_SELECTED_FREE_TRANS_PROJ);
	wxASSERT(pStaticTextCtrlSelectedFreeTransProj != NULL);
	pStaticTextCtrlSelectedFreeTransProj->SetBackgroundColour(sysColorBtnFace);

	pListOfProjects = (wxListBox*)FindWindowById(IDC_LIST_OF_COLLAB_PROJECTS);
	wxASSERT(pListOfProjects != NULL);

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
	m_bTempCollaborationExpectsFreeTrans = m_pApp->m_bCollaborationExpectsFreeTrans; // whm added 6Jul11

	m_projectSelectionMade = FALSE;

	// adjust static strings substituting "Paratext" or "Bibledit" depending on
	// the string value in m_collaborationEditor
	// Substitute strings in the top edit box
	wxString text = pStaticTextCtrlTopNote->GetValue(); // text has four %s sequences
	text = text.Format(text, m_pApp->m_collaborationEditor.c_str(), 
		m_pApp->m_collaborationEditor.c_str(), m_pApp->m_collaborationEditor.c_str(),m_pApp->m_collaborationEditor.c_str());
	pStaticTextCtrlTopNote->ChangeValue(text);

	// Substitute strings in the static text above the list box
	text = pStaticTextListOfProjects->GetLabel();
	text = text.Format(text,m_pApp->m_collaborationEditor.c_str());
	pStaticTextListOfProjects->SetLabel(text);

	// Substitute strings in the bottom text box that says "Important:..."
	text = pStaticTextCtrlImportantBottomNote->GetValue(); // text has 1 %s sequence
	text = text.Format(text, m_pApp->m_collaborationEditor.c_str()); 
	pStaticTextCtrlImportantBottomNote->ChangeValue(text);

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
	// fill in the "Use this project initially ..." read-only edit boxes
	pStaticTextCtrlSelectedSourceProj->ChangeValue(m_TempCollabProjectForSourceInputs);
	pStaticTextCtrlSelectedTargetProj->ChangeValue(m_TempCollabProjectForTargetExports);
	pStaticTextCtrlSelectedFreeTransProj->ChangeValue(m_TempCollabProjectForFreeTransExports);
}

// event handling functions

void CSetupEditorCollaboration::OnBtnSelectFromListSourceProj(wxCommandEvent& WXUNUSED(event))
{
	// Note: For source project, the project can be readable or writeable (all in list)
	// The OnOK() handler will check to ensure that the project selected for source
	// text inputs is different from the project selected for target text exports.
	
	// use a temporary array list
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
	if (ChooseProjectForSourceTextInputs.ShowModal() == wxID_OK)
	{
		userSelectionStr = ChooseProjectForSourceTextInputs.GetStringSelection();
		userSelectionInt = ChooseProjectForSourceTextInputs.GetSelection();
		m_projectSelectionMade = TRUE;
	}
	pStaticTextCtrlSelectedSourceProj->ChangeValue(userSelectionStr);
	m_TempCollabProjectForSourceInputs = userSelectionStr;
}

void CSetupEditorCollaboration::OnBtnSelectFromListTargetProj(wxCommandEvent& WXUNUSED(event))
{
	// Note: For target project, we must ensure that the Paratext project is writeable
	// TODO: remove from list any that are not writeable
	
	// use a temporary array list
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
	if (ChooseProjectForTargetTextInputs.ShowModal() == wxID_OK)
	{
		userSelectionStr = ChooseProjectForTargetTextInputs.GetStringSelection();
		userSelectionInt = ChooseProjectForTargetTextInputs.GetSelection();
		m_projectSelectionMade = TRUE;
	}
	pStaticTextCtrlSelectedTargetProj->ChangeValue(userSelectionStr);
	m_TempCollabProjectForTargetExports = userSelectionStr;
}

void CSetupEditorCollaboration::OnBtnSelectFromListFreeTransProj(wxCommandEvent& WXUNUSED(event))
{
	// Note: For free trans project, we must ensure that the Paratext project is writeable
	// TODO: remove from list any that are not writeable
	
	// use a temporary array list
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
	if (ChooseProjectForFreeTransTextInputs.ShowModal() == wxID_OK)
	{
		userSelectionStr = ChooseProjectForFreeTransTextInputs.GetStringSelection();
		userSelectionInt = ChooseProjectForFreeTransTextInputs.GetSelection();
		m_projectSelectionMade = TRUE;
	}
	pStaticTextCtrlSelectedFreeTransProj->ChangeValue(userSelectionStr);
	m_TempCollabProjectForFreeTransExports = userSelectionStr;
	m_bTempCollaborationExpectsFreeTrans = TRUE;
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
		return; // don't accept any changes - abort the OnOK() handler
	}
	
	if (m_bTempCollaborationExpectsFreeTrans && m_TempCollabProjectForSourceInputs == m_TempCollabProjectForFreeTransExports)
	{
		wxString msg, msg1;
		msg = _("The projects selected for getting source texts and receiving free translation texts cannot be the same.\nPlease select one project for getting source texts, and a different project for receiving free translation texts.");
		//msg1 = _("(or, if you select \"[No Project Selected]\" for a project here, the first time a source text is needed for adaptation, the user will have to choose a project from a drop down list of projects).");
		//msg = msg + _T("\n") + msg1;
		wxMessageBox(msg);
		return; // don't accept any changes - abort the OnOK() handler
	}

	// whm Note: Put code that aborts the OnOK() handler above

	// Set the App values for the PT projects to be used for PT collaboration
	m_pApp->m_CollabProjectForSourceInputs = m_TempCollabProjectForSourceInputs;
	m_pApp->m_CollabProjectForTargetExports = m_TempCollabProjectForTargetExports;
	m_pApp->m_CollabProjectForFreeTransExports = m_TempCollabProjectForFreeTransExports;
	m_pApp->m_bCollaborationExpectsFreeTrans = m_bTempCollaborationExpectsFreeTrans;

	// Get the state of collaboration
	int nSel;
	nSel = pRadioBoxCollabOnOrOff->GetSelection();
	if (nSel == 0)
	{
		// Collaboration radio button is ON
		if (m_pApp->m_collaborationEditor == _T("Bibledit"))
			m_pApp->m_bCollaboratingWithBibledit = TRUE;
		else
			m_pApp->m_bCollaboratingWithParatext = TRUE;
	}
	else if (nSel == 1)
	{
		// Collaboration radio button is OFF
		if (m_pApp->m_collaborationEditor == _T("Bibledit"))
			m_pApp->m_bCollaboratingWithBibledit = FALSE;
		else
			m_pApp->m_bCollaboratingWithParatext = FALSE;
		
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
					m_pApp->m_bCollaboratingWithBibledit = TRUE;
				else
					m_pApp->m_bCollaboratingWithParatext = TRUE;
			}
		}
	}
	else
	{
		wxASSERT_MSG(FALSE,_T("Programming Error: Wrong pRadioBoxCollabOnOrOff index in CSetupEditorCollaboration::OnOK()."));
	}

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
		bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_book_selected"), m_pApp->m_CollabBookSelected);
		bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_by_chapter_only"), m_pApp->m_bCollabByChapterOnly);
		bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_chapter_selected"), m_pApp->m_CollabChapterSelected);
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
		bWriteOK = m_pApp->m_pConfig->Write(_T("be_collab_book_selected"), m_pApp->m_CollabBookSelected);
		bWriteOK = m_pApp->m_pConfig->Write(_T("be_collab_by_chapter_only"), m_pApp->m_bCollabByChapterOnly);
		bWriteOK = m_pApp->m_pConfig->Write(_T("be_collab_chapter_selected"), m_pApp->m_CollabChapterSelected);
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

