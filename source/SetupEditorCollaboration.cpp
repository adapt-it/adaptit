/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SetupEditorCollaboration.cpp
/// \author			Bill Martin
/// \date_created	8 April 2011
/// \date_revised	8 April 2011
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
#include "Adapt_ItView.h"
#include "SetupEditorCollaboration.h"
#include "GetSourceTextFromEditor.h"

// event handler table
BEGIN_EVENT_TABLE(CSetupEditorCollaboration, AIModalDialog)
	EVT_INIT_DIALOG(CSetupEditorCollaboration::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(ID_BUTTON_SELECT_FROM_LIST_SOURCE_PROJ, CSetupEditorCollaboration::OnBtnSelectFromListSourceProj)
	EVT_BUTTON(ID_BUTTON_SELECT_FROM_LIST_TARGET_PROJ, CSetupEditorCollaboration::OnBtnSelectFromListTargetProj)
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
: AIModalDialog(parent, -1, _("Set Up Paratext Collaboration"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	SetupEditorCollaborationFunc(this, TRUE, TRUE);
	// The declaration is: SetupParatextCollaborationDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(m_pApp != NULL);
	
	wxColour sysColorBtnFace; // color used for read-only text controls displaying
	// color used for read-only text controls displaying static text info button face color
	sysColorBtnFace = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	
	pStaticTextCtrlTopNote = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TOP_NOTE);
	wxASSERT(pStaticTextCtrlTopNote != NULL);
	pStaticTextCtrlTopNote->SetBackgroundColour(sysColorBtnFace);

	pStaticTextCtrlImportantBottomNote = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_IMPORTANT_BOTTOM_NOTE);
	wxASSERT(pStaticTextCtrlImportantBottomNote != NULL);
	pStaticTextCtrlImportantBottomNote->SetBackgroundColour(sysColorBtnFace);

	pStaticTextCtrlSelectedSourceProj = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_SELECTED_SRC_PROJ);
	wxASSERT(pStaticTextCtrlSelectedSourceProj != NULL);
	pStaticTextCtrlSelectedSourceProj->SetBackgroundColour(sysColorBtnFace);

	pStaticTextCtrlSelectedTargetProj = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_SELECTED_TGT_PROJ);
	wxASSERT(pStaticTextCtrlSelectedTargetProj != NULL);
	pStaticTextCtrlSelectedTargetProj->SetBackgroundColour(sysColorBtnFace);

	pListOfProjects = (wxListBox*)FindWindowById(IDC_LIST_OF_PT_PROJECTS);
	wxASSERT(pListOfProjects != NULL);

	pRadioBoxCollabOnOrOff = (wxRadioBox*)FindWindowById(ID_RADIOBOX_PT_COLLABORATION_ON_OFF);
	wxASSERT(pRadioBoxCollabOnOrOff != NULL);

	pBtnSelectFmListSourceProj = (wxButton*)FindWindowById(ID_BUTTON_SELECT_FROM_LIST_SOURCE_PROJ);
	wxASSERT(pBtnSelectFmListSourceProj != NULL);

	pBtnSelectFmListTargetProj = (wxButton*)FindWindowById(ID_BUTTON_SELECT_FROM_LIST_TARGET_PROJ);
	wxASSERT(pBtnSelectFmListTargetProj != NULL);


	// other attribute initializations
}

CSetupEditorCollaboration::~CSetupEditorCollaboration() // destructor
{
	
}

void CSetupEditorCollaboration::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	
	// initialize our dialog temp variables from those held on the App
	m_bTempCollaboratingWithParatext = m_pApp->m_bCollaboratingWithParatext;
	m_TempPTProjectForSourceInputs = m_pApp->m_PTProjectForSourceInputs;
	m_TempPTProjectForTargetExports = m_pApp->m_PTProjectForTargetExports;

	if (m_bTempCollaboratingWithParatext)
		pRadioBoxCollabOnOrOff->SetSelection(0);
	else
		pRadioBoxCollabOnOrOff->SetSelection(1);
	
	// get list of PT projects
	m_pApp->m_ListOfPTProjects.Clear();
	m_pApp->m_ListOfPTProjects = m_pApp->GetListOfPTProjects();

	// Check for at least two usable PT projects in list
	if (m_pApp->m_ListOfPTProjects.GetCount() < 2)
	{
		// error: PT is not set up with enought projects for collaboration
	}
	else
	{
		int i;
		for (i = 0; i < (int)m_pApp->m_ListOfPTProjects.GetCount(); i++)
		{
			wxString tempStr;
			tempStr = m_pApp->m_ListOfPTProjects.Item(i);
			pListOfProjects->Append(tempStr);
		}

	}
	// fill in the "Use this project initially ..." read-only edit boxes
	pStaticTextCtrlSelectedSourceProj->ChangeValue(m_TempPTProjectForSourceInputs);
	pStaticTextCtrlSelectedTargetProj->ChangeValue(m_TempPTProjectForTargetExports);
}

// event handling functions

void CSetupEditorCollaboration::OnBtnSelectFromListSourceProj(wxCommandEvent& WXUNUSED(event))
{
	// Note: For source project, the project can be readable or writeable (all in list)
	// The OnOK() handler will check to insure that the project selected for source
	// text inputs is different from the project selected for target text exports.
	
	// use a temporary array list
	wxArrayString tempListOfProjects;
	tempListOfProjects.Add(_("[No Project Selected]"));
	int ct;
	int tot = (int)m_pApp->m_ListOfPTProjects.GetCount();
	for (ct = 0; ct < tot; ct++)
	{
		// load the rest of the projects into the temp array list
		tempListOfProjects.Add(m_pApp->m_ListOfPTProjects.Item(ct));
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
		if (tempListOfProjects.Item(ct) == m_TempPTProjectForSourceInputs)
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
	}
	pStaticTextCtrlSelectedSourceProj->SetLabel(userSelectionStr);
	m_TempPTProjectForSourceInputs = userSelectionStr;
}

void CSetupEditorCollaboration::OnBtnSelectFromListTargetProj(wxCommandEvent& WXUNUSED(event))
{
	// Note: For target project, we must insure that the Paratext project is writeable
	// TODO: remove from list any that are not writeable
	
	// use a temporary array list
	wxArrayString tempListOfProjects;
	tempListOfProjects.Add(_("[No Project Selected]"));
	int ct;
	int tot = (int)m_pApp->m_ListOfPTProjects.GetCount();
	for (ct = 0; ct < tot; ct++)
	{
		// load the rest of the projects into the temp array list
		tempListOfProjects.Add(m_pApp->m_ListOfPTProjects.Item(ct));
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
		if (tempListOfProjects.Item(ct) == m_TempPTProjectForTargetExports)
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
	}
	pStaticTextCtrlSelectedTargetProj->SetLabel(userSelectionStr);
	m_TempPTProjectForTargetExports = userSelectionStr;
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
	// The OnOK() handler first checks to insure that the project selected for source
	// text inputs is different from the project selected for target text exports, unless
	// both are [No Project Selected].
	// If the same project is selected, inform the administrator and abort the OK
	// operation so the administrator can select different projects or abort and
	// set up the needed projects in Paratext first.
	if (m_TempPTProjectForSourceInputs == m_TempPTProjectForTargetExports && m_TempPTProjectForSourceInputs != _("[No Project Selected]"))
	{
		wxString msg, msg1;
		msg = _("The projects selected for getting source texts and receiving translation texts cannot be the same.\nPlease select one project for getting source texts, and a different project for receiving translation texts.");
		//msg1 = _("(or, if you select \"[No Project Selected]\" for a project here, the first time a source text is needed for adaptation, the user will have to choose a project from a drop down list of projects).");
		//msg = msg + _T("\n") + msg1;
		wxMessageBox(msg);
		return; // don't accept any changes - abort the OnOK() handler
	}
	
	// Set the App values for the PT projects to be used for PT collaboration
	m_pApp->m_PTProjectForSourceInputs = m_TempPTProjectForSourceInputs;
	m_pApp->m_PTProjectForTargetExports = m_TempPTProjectForTargetExports;

	// Get the state of collaboration
	int nSel;
	nSel = pRadioBoxCollabOnOrOff->GetSelection();
	if (nSel == 0)
		m_pApp->m_bCollaboratingWithParatext = TRUE;
	else if (nSel == 1)
		m_pApp->m_bCollaboratingWithParatext = FALSE;
	else
	{
		wxASSERT_MSG(FALSE,_T("Programming Error: Wrong pRadioBoxCollabOnOrOff index in CSetupEditorCollaboration::OnOK."));
	}

	// update the values related to PT collaboration in the Adapt_It_WX.ini file
	{
	bool bWriteOK = FALSE;
	wxString oldPath = m_pApp->m_pConfig->GetPath(); // is always absolute path "/Recent_File_List"
	m_pApp->m_pConfig->SetPath(_T("/Settings"));
	wxLogNull logNo; // eliminates spurious message from the system: "Can't read value 
		// of key 'HKCU\Software\Adapt_It_WX\Settings' Error" [valid until end of this block]
	bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collaboration"), m_pApp->m_bCollaboratingWithParatext);
	bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_src_proj"), m_pApp->m_PTProjectForSourceInputs);
	bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_tgt_proj"), m_pApp->m_PTProjectForTargetExports);
	bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_book_selected"), m_pApp->m_PTBookSelected);
	bWriteOK = m_pApp->m_pConfig->Write(_T("pt_collab_chapter_selected"), m_pApp->m_PTChapterSelected);
	m_pApp->m_pConfig->Flush(); // write now, otherwise write takes place when m_p is destroyed in OnExit().
	// restore the oldPath back to "/Recent_File_List"
	m_pApp->m_pConfig->SetPath(oldPath);
	}


	if (m_pApp->m_bCollaboratingWithParatext)
	{
		// Configure the menu interface for collaboration. The menu interface changes have
		// to be done in such a way that they are compatible with the user profile changes to the
		// user interface.
		// Some menu changes will override any user profile allowed menu items:
		// 
		// TODO: disable Book Folder Mode menu items
		// TODO: disable other menu items?
		
		// Other menu changes may involve adding tests within the menu item handler
		// such as a test within the File's "UnPack Document..." command to prevent 
		// the unpacking of documents that are not pertinent to the Paratext projects
		//
		// 
		// Close any open project and/or document if any are open
		CAdapt_ItView* pView = m_pApp->GetView();
		if (pView != NULL)
		{
			m_pApp->GetView()->CloseProject();
		}
	}
	// Force the Main Frame's OnIdle() handler to call DoStartupWizardOnLaunch()
	// which, when m_bCollaboratingWithParatext is TRUE, actually calls the "Get
	// Source Text from Paratext Project" dialog instead of the normal startup
	// wizard. When not collaborating with Paratext, the usual Start Working
	// Wizard will appear.
	m_pApp->m_bJustLaunched = TRUE;


	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

