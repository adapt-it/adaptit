/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SetupEditorCollaboration.cpp
/// \author			Bill Martin
/// \date_created	8 April 2011
/// \rcs_id $Id$
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
#include <wx/docview.h>
#include <wx/fileconf.h>

#include "Adapt_It.h"
#include "MainFrm.h"
#include "Adapt_ItView.h"
#include "SplitDialog.h"
#include "SetupEditorCollaboration.h"
#include "CollabUtilities.h"
#include "CreateNewAIProjForCollab.h"

extern wxString szCollabProjectForSourceInputs;
extern wxString szCollabProjectForTargetExports;
extern wxString szCollabProjectForFreeTransExports;
extern wxString szCollabAIProjectName;
extern wxString szCollaborationEditor;
extern wxString szParatextVersionForProject; // whm added 24June2016 - the old obsolete project config file label
extern wxString szCollabParatextVersionForProject; // whm added 24June2016 the new project config file label
extern wxString szCollabExpectsFreeTrans;
extern wxString szCollabBookSelected;
extern wxString szCollabByChapterOnly;
extern wxString szCollabChapterSelected;
extern wxString szCollabSourceLangName;
extern wxString szCollabTargetLangName;
extern wxString szCollabBooksProtectedFromSavingToEditor; // whm added 2February2017
extern wxString szProjectConfiguration;
extern wxString szCollabDoNotShowMigrationDialogForPT7toPT8;

/// whm 4Feb2020 added.
/// The following bool values below represent what versions of Paratext are installed
/// as determined by the App's IsThisParatextVersionInstalled() function.
/// They are located here in the App's global space because both the App
/// and the App class and the CSetupEditorCollaboration class set them and use
/// their values.
extern bool gbPTVer7Installed;
extern bool gbPTVer8Installed;
extern bool gbPTVer9Installed;
extern bool gbPTLinuxVer7Installed;
extern bool gbPTLinuxVer8Installed;
extern bool gbPTLinuxVer9Installed;

extern bool gbPTNotInstalled;

extern bool gbPTVer7OnlyInstalled;
extern bool gbPTVer8OnlyInstalled;
extern bool gbPTVer9OnlyInstalled;
extern bool gbPTLinuxVer7OnlyInstalled;
extern bool gbPTLinuxVer8OnlyInstalled;
extern bool gbPTLinuxVer9OnlyInstalled;

// event handler table
BEGIN_EVENT_TABLE(CSetupEditorCollaboration, AIModalDialog)
	EVT_INIT_DIALOG(CSetupEditorCollaboration::InitDialog)
	EVT_BUTTON(ID_BUTTON_SELECT_FROM_LIST_SOURCE_PROJ, CSetupEditorCollaboration::OnBtnSelectFromListSourceProj)
	EVT_BUTTON(ID_BUTTON_SELECT_FROM_LIST_TARGET_PROJ, CSetupEditorCollaboration::OnBtnSelectFromListTargetProj)
	EVT_BUTTON(ID_BUTTON_SELECT_FROM_LIST_FREE_TRANS_PROJ, CSetupEditorCollaboration::OnBtnSelectFromListFreeTransProj)
	EVT_BUTTON(wxID_OK, CSetupEditorCollaboration::OnClose) // function is called OnClose, but uses the wxID_OK event
	EVT_BUTTON(wxID_CANCEL, CSetupEditorCollaboration::OnCancel)
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
: AIModalDialog(parent, -1, _("Setup or Remove Collaboration"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pSetupEditorCollabSizer = SetupCollaborationBetweenAIandEditorFunc(this, FALSE, TRUE); // second param FALSE enables resize);
	// The declaration is: SetupParatextCollaborationDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

    // whm 5Mar2019 Note: The dialog has a single "Close" (wxID_OK) button aligned at bottom right of
    // the dialog. We need not use the wxStdDialogButtonSizer, nor the ReverseOkCancelButtonsForMac()
    // function.

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

	pStaticTextDialogTopicLine = (wxStaticText*)FindWindowById(ID_TEXT_USE_THIS_DIALOG);
	wxASSERT(pStaticTextDialogTopicLine != NULL);

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

    pBtnSaveSetupForThisProjNow = (wxButton*)FindWindowById(ID_BUTTON_SAVE_SETUP_FOR_THIS_PROJ_NOW);
    wxASSERT(pBtnSaveSetupForThisProjNow != NULL);

	pBtnClose = (wxButton*)FindWindowById(wxID_OK); // the Close button uses the wxID_OK event id
	wxASSERT(pBtnClose != NULL);

    // These bool values below represent what versions of Paratext are installed when
    // this dialog is called up by the Administrator.
    // Here they are initialized to FALSE, but they will change to TRUE if that particular
    // version is found by calls made by the App'ss IsThisParatextVersionInstalled() function
    // within the InitDialog() function below.
    gbPTVer7Installed = FALSE;
    gbPTVer8Installed = FALSE;
    gbPTVer9Installed = FALSE;
    gbPTLinuxVer7Installed = FALSE;
    gbPTLinuxVer8Installed = FALSE;
    gbPTLinuxVer9Installed = FALSE;

    gbPTNotInstalled = FALSE;

    gbPTVer7OnlyInstalled = FALSE;
    gbPTVer8OnlyInstalled = FALSE;
    gbPTVer9OnlyInstalled = FALSE;
    gbPTLinuxVer7OnlyInstalled = FALSE;
    gbPTLinuxVer8OnlyInstalled = FALSE;
    gbPTLinuxVer9OnlyInstalled = FALSE;

	// We have no Cancel button so no need to reverse buttons
}

CSetupEditorCollaboration::~CSetupEditorCollaboration() // destructor
{

}

void CSetupEditorCollaboration::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

    // The following InventoryCollabEditorInstalls() function needs to be called
    // early before other code or functions that need collab editor install infor. 
    m_pApp->InventoryCollabEditorInstalls(); 

	// whm Note: The App's OnSetupEditorCollaboration() handler
	// ensures that no project is open when this
	// SetupEditorCollaboration dialog is shown to the user.

	// These m_Save... and m_bSave... values are used for holding the App's original
	// collaboration settings upon entry to the SetupEditorCollaboration dialog.
	// They are then used to restore those App values before closing the dialog. This
	// is designed to provide a safety net to help prevent the unintended
	// saving of bogus collaboration settings as a result of actions taken in
	// this dialog that require adjusting the value of the App's collaboration
	// settings temporarily before callling WriteConfigurationFile(szProjectConfiguration...
	m_bSaveCollaboratingWithParatext = m_pApp->m_bCollaboratingWithParatext;
	m_bSaveCollaboratingWithBibledit = m_pApp->m_bCollaboratingWithBibledit;
	m_SaveCollabProjectForSourceInputs = m_pApp->m_CollabProjectForSourceInputs;
	m_SaveCollabProjectForTargetExports = m_pApp->m_CollabProjectForTargetExports;
	m_SaveCollabProjectForFreeTransExports = m_pApp->m_CollabProjectForFreeTransExports;
	m_SaveCollabAIProjectName = m_pApp->m_CollabAIProjectName;
	m_SaveCollaborationEditor = m_pApp->m_collaborationEditor;
    m_SaveCollabEditorVersion = m_pApp->m_ParatextVersionForProject; // whm added 24June2016
	m_SaveCollabSourceProjLangName = m_pApp->m_CollabSourceLangName;
	m_SaveCollabTargetProjLangName = m_pApp->m_CollabTargetLangName;
	m_bSaveCollabByChapterOnly = m_pApp->m_bCollabByChapterOnly; // FALSE means the "whole book" option
	m_bSaveCollaborationExpectsFreeTrans = m_pApp->m_bCollaborationExpectsFreeTrans;
	m_SaveCollabBookSelected = m_pApp->m_CollabBookSelected;
	m_SaveCollabChapterSelected = m_pApp->m_CollabChapterSelected;
    m_SaveCollabBooksProtectedFromSavingToEditor = m_pApp->m_CollabBooksProtectedFromSavingToEditor; // whm added 2February2017
    m_SaveCollabDoNotShowMigrationDialogForPT7toPT8 = m_pApp->m_bCollabDoNotShowMigrationDialogForPT7toPT8; // whm added 6April2017

	m_bCollabChangedThisDlgSession = FALSE;

	// For InitDialog() empty the m_TempCollabAIProjectName
	m_TempCollabAIProjectName = _T("");
    m_TempCollaborationEditor = m_pApp->m_collaborationEditor;
    m_TempCollabEditorVersion = m_pApp->m_ParatextVersionForProject;
	// edb 17May2012: set the initial value of the collaboration editor if needed.
	// Reasons we'd need to:
	// - If there is no collaboration editor specified yet
	// - If there is a collaboration editor specified, but the editor
	//   isn't installed on the computer (maybe transferring the settings from
    //    another computer, or the user uninstalled something).
    // whm revised 24June2016 for Paratext 8 support.
    // whm revised 4Feb2020 for Paratext 9 support.
    // whm 4Feb2020 Note: the dialog's "Scripture Editor:" radio box now has these four options:
    //   (.) Paratext 7
    //   ( ) Paratext 8
    //   ( ) Paratext 9
    //   ( ) Bibledit
    // For either "Paratext 7", "Paratext 8", or "Paratext 9" options, the m_TempColllaborationEditor is still just "Paratext",
    // but the m_TempCollabEditorVersion will one of: "PTVersion7", "PTVersion8", "PTVersion9", "PTLinuxVersion7", "PTLinuxVersion8", or "PTLinuxVersion9".
    
    // These bool values below represent what versions of Paratext are installed when
    // this dialog is called up by the Administrator.
    // They were initialized to FALSE in the constructor, but below they are changed to TRUE 
    // if that particular PT version is found to be installed by calls made by the App's 
    // IsThisParatextVersionInstalled() function below.
    gbPTVer7Installed = m_pApp->IsThisParatextVersionInstalled(_T("PTVersion7"));
    gbPTVer8Installed = m_pApp->IsThisParatextVersionInstalled(_T("PTVersion8"));;
    gbPTVer9Installed = m_pApp->IsThisParatextVersionInstalled(_T("PTVersion9"));;
    gbPTLinuxVer7Installed = m_pApp->IsThisParatextVersionInstalled(_T("PTLinuxVersion7"));;
    gbPTLinuxVer8Installed = m_pApp->IsThisParatextVersionInstalled(_T("PTLinuxVersion8"));;
    gbPTLinuxVer9Installed = m_pApp->IsThisParatextVersionInstalled(_T("PTLinuxVersion9"));;

    // Set up the boolean logic for other possible PT installations.
    // Set some convenience bools that indicate when ONLY a certain version is installed on this machine
    if (gbPTVer7Installed && !gbPTVer8Installed && !gbPTVer9Installed && !gbPTLinuxVer7Installed && !gbPTLinuxVer8Installed && !gbPTLinuxVer9Installed)
        gbPTVer7OnlyInstalled = TRUE;
    if (gbPTVer8Installed && !gbPTVer7Installed && !gbPTVer9Installed && !gbPTLinuxVer7Installed && !gbPTLinuxVer8Installed && !gbPTLinuxVer9Installed)
        gbPTVer8OnlyInstalled = TRUE;
    if (gbPTVer9Installed && !gbPTVer7Installed && !gbPTVer8Installed && !gbPTLinuxVer7Installed && !gbPTLinuxVer8Installed && !gbPTLinuxVer9Installed)
        gbPTVer9OnlyInstalled = TRUE;
    if (gbPTLinuxVer7Installed && !gbPTVer7Installed && !gbPTVer8Installed && !gbPTVer9Installed && !gbPTLinuxVer8Installed && !gbPTLinuxVer9Installed)
        gbPTLinuxVer7OnlyInstalled = TRUE;
    if (gbPTLinuxVer8Installed && !gbPTVer7Installed && !gbPTVer8Installed && !gbPTVer9Installed && !gbPTLinuxVer7Installed && !gbPTLinuxVer9Installed)
        gbPTLinuxVer8OnlyInstalled = TRUE;
    if (gbPTLinuxVer9Installed && !gbPTVer7Installed && !gbPTVer8Installed && !gbPTVer9Installed && !gbPTLinuxVer7Installed && !gbPTLinuxVer8Installed)
        gbPTLinuxVer9OnlyInstalled = TRUE;
    // Set another convenience bool named gbPTNotInstalled if none of the PT versions are installed
    // on this machine.
    if (!gbPTVer7Installed && !gbPTVer8Installed && !gbPTVer9Installed && !gbPTLinuxVer7Installed && !gbPTLinuxVer8Installed && !gbPTLinuxVer9Installed)
        gbPTNotInstalled = TRUE;

    // whm 4Feb2020 Note: The above usage of the App's IsThisParatextVersionInstalled() need only be
    // done once for the duration of this CSetupEditorCollaboration dialog instance with its return
    // values stored within the dialog's bool members above. This eliminates calling the App's older
    // ParatextVersionInstalled() function and messing with its limited enum values

    if (m_TempCollaborationEditor.IsEmpty() || m_TempCollabEditorVersion.IsEmpty() ||
        (m_TempCollaborationEditor == _T("Paratext") && (gbPTNotInstalled)) ||
        (m_TempCollaborationEditor == _T("Bibledit") && (m_pApp->BibleditIsInstalled() == false)))
    {
        // The collaboration editor value is missing or points to an editor that isn't installed. 
        // Set it to a good initial value.
        // whm 4Feb2020 modified tests below to include PTVersion9 and PTLinuxVersion9; and also
        // using the more flexible bool values, rather than the enum values of PTVersionInstalled.

        m_TempCollabEditorVersion = m_pApp->ValidateCollabEditorAndVersionStrAgainstInstallationData(m_TempCollaborationEditor, m_TempCollabEditorVersion);

    } // end of when collaboration editor value is missing or points to an editor that isn't installed. Set it to a good initial value.

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

	DoInit(FALSE); // empties m_Temp... variables for a new collab setup // FALSE = don't prompt
}

void CSetupEditorCollaboration::DoInit(bool bPrompt)
{
	// DoInit() is called at the end of the dialog's InitiDialog() handler and also in the OnRadioBoxSelectBtn()
	// handler.

    // These were initialized in the InitDialog() function above to their App values if the App values
    // were set, or to a suggested value based on what PT versions were installed:
    //   m_TempCollaborationEditor
    //   m_TempCollabEditorVersion

	// whm revised 5Apr12. We now configure the SetupEditorCollaboration dialog
	// with the following initial setup:
	// 1. No Adapt It project selected in the step 1 combo box (which will be blank).
	// 2. The possible PT/BE projects listed in step 2 (no change in this revision).
	// 3. The "Get by Chapter Only" radio button in Step 3 is selected.
	// 4. The edit boxes in step 3 are set blank/empty.

	// zero out the local Temp... variables as we did in OnInit()
	m_TempCollabProjectForSourceInputs = _T("");
	m_TempCollabProjectForTargetExports = _T("");
	m_TempCollabProjectForFreeTransExports = _T("");
	// m_TempCollabAIProjectName // can stay set to its current value - InitDialog set it to _T(""); user sets it in OnComboBoxSelectAiProject()
    // m_TempCollaborationEditor // can stay set to its App value - user can set it in OnRadioBoxSelectBtn()
    // m_TempCollabEditorVersion // can stay set to its App value - user can set it in OnRadioBoxSelectBtn() // whm added 24June2016
	m_bTempCollaborationExpectsFreeTrans = FALSE; // defaults to FALSE for no free trans
	m_TempCollabBookSelected = _T("");
	m_TempCollabSourceProjLangName = _T("");
	m_TempCollabTargetProjLangName = _T("");
	m_bTempCollabByChapterOnly = TRUE; // defaults to TRUE for collab by chapter only
	m_TempCollabChapterSelected = _T("");
    m_TempCollabBooksProtectedFromSavingToEditor = _T(""); // whm added 2February2017
    m_TempCollabDoNotShowMigrationDialogForPT7toPT8 = FALSE; // whm added 6April2017

	SetStateOfRemovalButton(); // disables the Remov... button when m_TempCollabAIProjectName is empty
    SetStateOfAcceptSetupButton(); // disables the "Accept this setup and prepare for another" button if
                                    // any of project name, editor, source text, or target text project names
                                    // are empty

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

    // whm revised 4Feb2020 - we now distinguish between four possible editors:
    // 1. Paratext 7 (radio box index value 0)
    // 2. Paratext 8 (radio box index value 1)
    // 3. Paratext 9 (radio box index value 2), and
    // 3. Bibledit (radio box index value 3)
    //
    // The function IsThisParatextVersionInstalled() was called in InitDialog() on all possible
    // installations to determine which versions are currently installed.
    // Disable editor selection radio box items for any editor not installed, and enable
    // radio box items for those which are installed.
    pRadioBoxScriptureEditor->Enable(0, gbPTVer7Installed || gbPTLinuxVer7Installed); // radiobox index 0 is "Paratext 7"
    pRadioBoxScriptureEditor->Enable(1, gbPTVer8Installed || gbPTLinuxVer8Installed); // radiobox index 1 is "Paratext 8"
    pRadioBoxScriptureEditor->Enable(2, gbPTVer9Installed || gbPTLinuxVer9Installed); // radiobox index 2 is "Paratext 9"
    pRadioBoxScriptureEditor->Enable(3, m_pApp->m_bBibleditIsInstalled); // radiobox index 3 is "Bibledit"

	// Now that radiobox items are enabled/disabled appropriately, set a reasonable initial 
    // default for editor selection radio button.
    // Note: These initial selections should agree with the enabled/disabled state of the radio boxes set above!

    SetRadioBoxCollabEditorSelection();

    unsigned int nSel = pRadioBoxScriptureEditor->GetSelection();
    m_nPreviousSel = nSel; // keep track of the previous selection

    // Get list of PT/BE projects for projList wxArrayString
	int nProjectCount = 0;
	projList.Clear();
	if (m_TempCollaborationEditor == _T("Paratext"))
	{
        projList = m_pApp->GetListOfPTProjects(m_TempCollabEditorVersion); // as a side effect, it populates the App's m_pArrayOfCollabProjects
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
		pListOfProjects->Clear(); // for now, just clear out the list box
	}
	else
	{
		// repopulate list box with the pListOfProjects
		pListOfProjects->Clear();
		int i;
		for (i = 0; i < nProjectCount; i++)
		{
			wxString tempStr;
			tempStr = projList.Item(i);
			pListOfProjects->Append(tempStr);
		}
	}

	// We don't need to change any pComboAiProjects selection here in DoInit().

	pTextCtrlAsStaticSelectedSourceProj->ChangeValue(m_TempCollabProjectForSourceInputs);
	pTextCtrlAsStaticSelectedTargetProj->ChangeValue(m_TempCollabProjectForTargetExports);
	pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(m_TempCollabProjectForFreeTransExports);

	// whm added 5Jun12
	if (bPrompt)
	{
		if (m_TempCollabProjectForSourceInputs.IsEmpty() || m_TempCollabProjectForTargetExports.IsEmpty())
		{
			wxString msg = _("Please use the \"Select from list\" buttons to select the appropriate %s projects.");
			msg = msg.Format(msg, m_TempCollaborationEditor.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,_T(""),wxICON_INFORMATION | wxOK);
			if (m_TempCollabProjectForSourceInputs.IsEmpty())
				this->pBtnSelectFmListSourceProj->SetFocus();
			else
				this->pBtnSelectFmListTargetProj->SetFocus();
		}
	}

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
    if (gbPTNotInstalled && !m_pApp->BibleditIsInstalled())
    {
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_("Neither Paratext nor Bibledit are installed on this computer. Paratext or Bibledit must be installed before Adapt It can collaborate with it."),_("Cannot select any projects"),wxICON_EXCLAMATION | wxOK);
        return;
    }

	// Note: For source project, the project can be readable or writeable (all in list)
	// The OnOK() handler will check to ensure that the project selected for source
	// text inputs is different from the project selected for target text exports.

	// use a temporary array list for the list of projects
	wxString projShortName;
	wxArrayString tempListOfProjects;
	projList.Clear();
	if (m_TempCollaborationEditor == _T("Paratext"))
	{
        projList = m_pApp->GetListOfPTProjects(m_TempCollabEditorVersion); // as a side effect, it populates the App's m_pArrayOfCollabProjects
    }
	else if (m_TempCollaborationEditor == _T("Bibledit"))
	{
		projList = m_pApp->GetListOfBEProjects(); // as a side effect, it populates the App's m_pArrayOfCollabProjects
    }
	int ct,tot;
	tot = (int)projList.GetCount();
	for (ct = 0; ct < tot; ct++)
	{
		// load the rest of the projects into the temp array list
		tempListOfProjects.Add(projList.Item(ct));
	}
	wxString msg;
	msg = _("Choose a %s project that will be used for source text inputs");
	wxASSERT(!m_TempCollaborationEditor.IsEmpty());
	msg = msg.Format(msg,m_TempCollaborationEditor.c_str());
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

	// whm added 17Jul12. Ensure that the project selected for obtaining source texts
	// actually has some verse content in at least one book in the PT/BE project. This
	// check helps to prevent an administrator from accidentally swapping the PT/BE
	// projects by assigning the target PT/BE project here where a source PT/BE project
	// should be selected, and possibly assigning the source PT/BE project where a
	// target PT/BE projects should be selected. See similar protective code also in the
	// CSetupEditorCollaboration::OnBtnSelectFromListTargetProj() handler.
	// Note: Some code for the implementation of DoProjectAnalysis() was borrowed from the
	// CGetSourceTextFromEditor class.

	EditorProjectVerseContent projVerseContent;
	wxString emptyBooks = _T("");
	wxString booksWithContent = _T("");
	wxString errorMsg = _T("");
	// Note: The DoProjectAnalysis() function below sets up a progress dialog because the analysis
	// process can be disk intensive and take a significant amount of time to complete since it will
	// fetch each book in the PT/BE project via wxExecute() command-line access and analyze its contents.
	projVerseContent = DoProjectAnalysis(collabSrcText,m_TempCollabProjectForSourceInputs,
        m_TempCollaborationEditor,m_TempCollabEditorVersion,emptyBooks,booksWithContent,errorMsg);
	switch (projVerseContent)
	{
	case projHasVerseTextInAllBooks:
		{
			// This is the expected case. No need to warn the administrator so
			// do nothing
			break;
		}
	case projHasNoBooksWithVerseText:
		{
			// All books in source project are "empty" of content. Do not allow this
			// PT/BE project to be selected for obtaining source texts, since no viable
			// source texts can be obtained for adaptation from this project.
			wxString msg = _("The \"%s\" project only has the following \"empty\" book(s):\n\n%s\n\nThese books may have chapter and verse markers (\\c and \\v) but the verses contain no actual source text. Adapt It cannot use \"empty\" books for obtaining source texts.");
			msg = msg.Format(msg,m_TempCollabProjectForSourceInputs.c_str(),emptyBooks.c_str());
			wxString msgTitle = _("No books in this project are usable as source texts!");
			wxString msg2 = _T("\n\n");
			msg2 += _("Please select a different %s project that Adapt It can use for obtaining source texts.");
			msg2 = msg2.Format(msg2,m_TempCollaborationEditor.c_str());
			msg += msg2;
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,msgTitle,wxICON_EXCLAMATION | wxOK);
			// clear out the source control
			pTextCtrlAsStaticSelectedSourceProj->ChangeValue(wxEmptyString);
			m_TempCollabProjectForSourceInputs = _T(""); // invalid project for source inputs
			break;
		}
	case projHasSomeBooksWithVerseTextSomeWithout:
		{
			// This result indicates that the source PT/BE project has one or more
			// books that have no content other than \c n and \v n markers. This
			// suggests that the administrator may have accidentally swapped the PT/BE
			// projects for source and target in the collaboration setup. The string
			// emptyBooks has a list of books that are empty of content, and the string
			// booksWithContent has a list of books that have at least some content.
			wxString msg = _("The \"%s\" project you selected has the following \"empty\" book(s):\n\n%s\n\nThe above book(s) may have chapter and verse markers (\\c and \\v) but the verses contain no actual source text. Please note that Adapt It cannot use such \"empty\" books for obtaining source texts.");
			msg = msg.Format(msg,m_TempCollabProjectForSourceInputs.c_str(),emptyBooks.c_str());
			wxString msgTitle = _("Some books in project cannot be used as source texts!");
			wxString msg2 = _T("\n\n");
			msg2 += _("Only the following books currently have actual verse content that Adapt It can use as source texts:\n\n%s\n\nThe user will only be able to select from books that have some verse content. If this is not what you want or expect, please set up the %s project with the books containing text that Adapt It can use for its source texts.");
			msg2 = msg2.Format(msg2,booksWithContent.c_str(),m_TempCollaborationEditor.c_str());
			msg += msg2;
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,msgTitle, wxICON_EXCLAMATION | wxOK);
			// do not clear out source control in this case as it is only a warning about selecting this
			// PT/BE project
			break;
		}
	case projHasNoChaptersOrVerses:
		{
			projShortName = GetShortNameFromProjectName(m_TempCollabProjectForSourceInputs);
			// The book does not have at least one book in the Source project
			wxString msg,msg1,msg2,titleMsg;
			msg1 = _("This %s project has no books created in it. It cannot be used for obtaining source texts for Adapt It. Please run %s and select the %s project.");
			msg1 = msg1.Format(msg1,m_TempCollaborationEditor.c_str(),m_TempCollaborationEditor.c_str(),projShortName.c_str());
			if (m_TempCollaborationEditor == _T("Paratext"))
			{
				msg2 = _("Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and ensure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again.");
			}
			else if (m_TempCollaborationEditor == _T("Bibledit"))
			{
				msg2 = _("Then select File | Project | Properties. Then select \"Templates+\" from the Project properties dialog. Choose the book(s) to be created and click OK. Then return to Adapt It and try again.");
			}
			msg = msg1 + _T(' ') + msg2;
			titleMsg = _("No chapters and verses found in project: \"%s\"");
			titleMsg = titleMsg.Format(titleMsg,m_TempCollabProjectForSourceInputs.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,titleMsg,wxICON_EXCLAMATION | wxOK);
			// clear out the source control
			pTextCtrlAsStaticSelectedSourceProj->ChangeValue(wxEmptyString);
			m_TempCollabProjectForSourceInputs = _T(""); // invalid project for source inputs
			break;
		}
	case projHasNoBooks:
		{
			wxString msg,titleMsg;
			msg = _("This %s project contains no books. It cannot be used for obtaining source texts for Adapt It. Please go back to %s and import the books to be used as source texts into this project (or import them into a new %s project), then return to Adapt It and try again.");
			msg = msg.Format(msg,m_TempCollaborationEditor.c_str(),m_TempCollaborationEditor.c_str(),m_TempCollaborationEditor.c_str());
			titleMsg = _("No books found in project: \"%s\"");
			titleMsg = titleMsg.Format(titleMsg,m_TempCollabProjectForSourceInputs.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,titleMsg,wxICON_EXCLAMATION | wxOK);
			// clear out the source control
			pTextCtrlAsStaticSelectedSourceProj->ChangeValue(wxEmptyString);
			m_TempCollabProjectForSourceInputs = _T(""); // invalid project for source inputs
			break;
		}
	case processingError:
		{
			wxASSERT(!errorMsg.IsEmpty());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(errorMsg,_T(""),wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(msg);
			break;
		}
	}

	// If the value changed mark it dirty.
	if (saveCollabProjectForSourceInputs != m_TempCollabProjectForSourceInputs)
		m_bCollabChangedThisDlgSession = TRUE;

    // After selecting source project, the dialog may be ready to "Accept this setup..."
    SetStateOfRemovalButton();
    SetStateOfAcceptSetupButton(); // enables the "Accept this setup and prepare for another" button if
                                   // all of: project name, editor, source text, or target text project names
                                   // have non-empty strings
}

void CSetupEditorCollaboration::OnBtnSelectFromListTargetProj(wxCommandEvent& WXUNUSED(event))
{
    if (gbPTNotInstalled && !m_pApp->BibleditIsInstalled())
    {
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_("Neither Paratext nor Bibledit are installed on this computer. Paratext or Bibledit must be installed before Adapt It can collaborate with it."), _("Cannot select any projects"), wxICON_EXCLAMATION | wxOK);
        return;
    }

	// Note: For target project, we must ensure that the Paratext project is writeable
	// use a temporary array list for the list of projects

    wxString projShortName;
	wxArrayString tempListOfProjects;
	projList.Clear();
	if (m_TempCollaborationEditor == _T("Paratext"))
	{
        projList = m_pApp->GetListOfPTProjects(m_TempCollabEditorVersion); // as a side effect, it populates the App's m_pArrayOfCollabProjects
    }
	else if (m_TempCollaborationEditor == _T("Bibledit"))
	{
		projList = m_pApp->GetListOfBEProjects(); // as a side effect, it populates the App's m_pArrayOfCollabProjects
	}
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
	wxASSERT(!m_TempCollaborationEditor.IsEmpty());
	msg = msg.Format(msg,m_TempCollaborationEditor.c_str());
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

	// whm added 17Jul12. Ensure that the project selected for receiving target texts
	// has at least some verses without content in at least one book in the PT/BE project.
	// This check helps to prevent an administrator from accidentally swapping the PT/BE
	// projects by assigning the target PT/BE project here where a source PT/BE project
	// should be selected, and possibly assigning the source PT/BE project where a
	// target PT/BE projects should be selected. See similar protective code also in the
	// CSetupEditorCollaboration::OnBtnSelectFromListSourceProj() handler.
	// Note: Some code for the implementation of DoProjectAnalysis() was borrowed from the
	// CGetSourceTextFromEditor class.

	EditorProjectVerseContent projVerseContent;
	wxString emptyBooks = _T("");
	wxString booksWithContent = _T("");
	wxString errorMsg = _T("");
	// Note: The DoProjectAnalysis() function below sets up a progress dialog because the analysis
	// process can be disk intensive and take a significant amount of time to complete since it will
	// fetch each book in the PT/BE project via wxExecute() command-line access and analyze its contents.
	projVerseContent = DoProjectAnalysis(collabTgtText,m_TempCollabProjectForTargetExports,
        m_TempCollaborationEditor,m_TempCollabEditorVersion,emptyBooks,booksWithContent,errorMsg);
	switch (projVerseContent)
	{
	case projHasVerseTextInAllBooks:
		{
			// This should *not* be the case for a PT/BE target project when first set up
			// for collaboration with Adapt It. The selected PT/BE project is possibly the
			// project that is designed for obtaining source texts rather than one for
			// receiving exported translation/target texts from Adapt It. Warn the administrator
			// of this situation.
			//wxString msg = _("All books in the \"%s\" project already have verse content, so Adapt It thinks that this project may not be the one you intended for storing Adapt It's translated texts. All of the verses in the following book(s) already have text content:\n\n%s");
            // BEW 27Feb2017 and 8March2017 suggested the wording of the msg should be as follows:
            wxString msg = _("All books in the \"%s\" project already have verse content.\nThis is expected and normal if you are deliberately establishing collaboration for a finished translation that needs further work (for example, a revision).\nIt possibly is an error if you are setting up the collaboration for the first time - because you may be wrongly configuring a source text project as the destination for sending new target text translations.\nAll of the verses in the following book(s) already have text content:\n\n%s");
            msg = msg.Format(msg,m_TempCollabProjectForTargetExports.c_str(),booksWithContent.c_str());

			wxString msgTitle = _("No books in this project have untranslated texts!");
			wxString msg2 = _T("\n\n");
			msg2 += _("Are you sure you want to use the \"%s\" project for storing Adapt It's translation texts?");
			msg2 = msg2.Format(msg2,m_TempCollabProjectForTargetExports.c_str());
			msg += msg2;
			int response;
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            response = wxMessageBox(msg,msgTitle,wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT);
			if (response == wxYES)
			{
				; // do nothing but continue with selection
			}
			else // if (response == wxNO || response || wxCANCEL)
			{
				// clear out the source control
				pTextCtrlAsStaticSelectedTargetProj->ChangeValue(wxEmptyString);
				m_TempCollabProjectForTargetExports = _T(""); // invalid project for source inputs
			}
			break;
		}
	case projHasNoBooksWithVerseText:
		{
			// All books in target project are "empty" of content. This would be the
			// expected situation for an initial setup.
			break;
		}
	case projHasSomeBooksWithVerseTextSomeWithout:
		{
			// The target PT/BE project has one or more books that have no content
			// other than \c n and \v n markers, and that other books have verse
			// content. This situation is posssible and permissible. Tell the user
			// the status of the books.
			wxString msg = _("The following books in the \"%s\" project do not yet have any verse content:\n\n%s\n\nAdapt It will store its translations in these books as adaptation work proceeds.");
			msg = msg.Format(msg,m_TempCollabProjectForTargetExports.c_str(),emptyBooks.c_str());
			wxString msgTitle = _("Some books in this project have existing translations!");
			wxString msg2 = _T("\n\n");
			msg2 += _("Please note that the following books in the \"%s\" project already have translations:\n\n%s\n\nAdapt It will overwrite any existing translations with new translations if you adapt all the text in books such as these that already have translations.");
			msg2 = msg2.Format(msg2,m_TempCollaborationEditor.c_str(),booksWithContent.c_str());
			msg += msg2;
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,msgTitle, wxICON_EXCLAMATION | wxOK);
			break;
		}
	case projHasNoChaptersOrVerses:
		{
			projShortName = GetShortNameFromProjectName(m_TempCollabProjectForTargetExports);
			// The book does not have at least one book in the Source project
			wxString msg,msg1,msg2,titleMsg;
			msg1 = _("This %s project has no books created in it. It cannot be used for storing translation texts from Adapt It. Please run %s and select the %s project.");
			msg1 = msg1.Format(msg1,m_TempCollaborationEditor.c_str(),m_TempCollaborationEditor.c_str(),projShortName.c_str());
			if (m_TempCollaborationEditor == _T("Paratext"))
			{
				msg2 = _("Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and ensure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again.");
			}
			else if (m_TempCollaborationEditor == _T("Bibledit"))
			{
				msg2 = _("Then select File | Project | Properties. Then select \"Templates+\" from the Project properties dialog. Choose the book(s) to be created and click OK. Then return to Adapt It and try again.");
			}
			msg = msg1 + _T(' ') + msg2;
			titleMsg = _("No chapters and verses found in project: \"%s\"");
			titleMsg = titleMsg.Format(titleMsg,m_TempCollabProjectForTargetExports.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,titleMsg,wxICON_EXCLAMATION | wxOK);
			// clear out the source control
			pTextCtrlAsStaticSelectedTargetProj->ChangeValue(wxEmptyString);
			m_TempCollabProjectForTargetExports = _T(""); // invalid project for source inputs
			break;
		}
	case projHasNoBooks:
		{
			wxString msg,titleMsg;
			msg = _("This %s project contains no books. It cannot be used for storing translations from Adapt It. Please go back to %s and create some empty books (with chapter and verse markers only) for this project, then return to Adapt It and try again.");
			msg = msg.Format(msg,m_TempCollaborationEditor.c_str(),m_TempCollaborationEditor.c_str());
			titleMsg = _("No books found in project \"%s\"");
			titleMsg = titleMsg.Format(titleMsg,m_TempCollabProjectForTargetExports.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,titleMsg,wxICON_EXCLAMATION | wxOK);
			// clear out the source control
			pTextCtrlAsStaticSelectedTargetProj->ChangeValue(wxEmptyString);
			m_TempCollabProjectForTargetExports = _T(""); // invalid project for source inputs
			break;
		}
	case processingError:
		{
			wxASSERT(!errorMsg.IsEmpty());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(errorMsg,_T(""),wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(msg);
			break;
		}
	}

	// If the value changed mark it dirty.
	if (saveCollabProjectForTargetExports != m_TempCollabProjectForTargetExports)
		m_bCollabChangedThisDlgSession = TRUE;

    // After selecting source project, the dialog may be ready to "Accept this setup..."
    SetStateOfRemovalButton();
    SetStateOfAcceptSetupButton(); // enables the "Accept this setup and prepare for another" button if
                                   // all of: project name, editor, source text, or target text project names
                                   // have non-empty strings
}

void CSetupEditorCollaboration::OnBtnSelectFromListFreeTransProj(wxCommandEvent& WXUNUSED(event))
{
    if (gbPTNotInstalled && !m_pApp->BibleditIsInstalled())
    {
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_("Neither Paratext nor Bibledit are installed on this computer. Paratext or Bibledit must be installed before Adapt It can collaborate with it."), _("Cannot select any projects"), wxICON_EXCLAMATION | wxOK);
        return;
    }

	// Note: For free trans project, we must ensure that the Paratext project is writeable
	// use a temporary array list for the list of projects

    wxString projShortName;
	wxArrayString tempListOfProjects;
	pBtnNoFreeTrans->Enable(TRUE);
	projList.Clear();
	if (m_TempCollaborationEditor == _T("Paratext"))
	{
        projList = m_pApp->GetListOfPTProjects(m_TempCollabEditorVersion); // as a side effect, it populates the App's m_pArrayOfCollabProjects
    }
	else if (m_TempCollaborationEditor == _T("Bibledit"))
	{
		projList = m_pApp->GetListOfBEProjects(); // as a side effect, it populates the App's m_pArrayOfCollabProjects
	}
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
	wxASSERT(!m_TempCollaborationEditor.IsEmpty());
	msg = msg.Format(msg,m_TempCollaborationEditor.c_str());
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

	// whm added 17Jul12. Ensure that the project selected for receiving free trans texts
	// has at least some verses without content in at least one book in the PT/BE project.
	// This check helps to prevent an administrator from accidentally swapping the PT/BE
	// projects possibly assigning the source PT/BE project where a free trans PT/BE project
	// should be selected and vice versa. See similar protective code also in the
	// CSetupEditorCollaboration::OnBtnSelectFromListSourceProj() handler.
	// Note: Some code for the implementation of DoProjectAnalysis() was borrowed from the
	// CGetSourceTextFromEditor class.

	EditorProjectVerseContent projVerseContent;
	wxString emptyBooks = _T("");
	wxString booksWithContent = _T("");
	wxString errorMsg = _T("");

	// Note: The DoProjectAnalysis() function below sets up a progress dialog because the analysis
	// process can be disk intensive and take a significant amount of time to complete since it will
	// fetch each book in the PT/BE project via wxExecute() command-line access and analyze its contents.
	projVerseContent = DoProjectAnalysis(collabFreeTransText,m_TempCollabProjectForFreeTransExports,
        m_TempCollaborationEditor,m_TempCollabEditorVersion,emptyBooks,booksWithContent,errorMsg);
	switch (projVerseContent)
	{
	case projHasVerseTextInAllBooks:
		{
			// This should *not* be the case for a PT/BE free trans project when first set up
			// for collaboration with Adapt It. The selected PT/BE project is possibly the
			// project that is designed for obtaining source texts rather than one for
			// receiving exported free trans texts from Adapt It. Warn the administrator
			// of this situation.
			wxString msg = _("All books in the \"%s\" project already have verse content, so Adapt It thinks that this project may not be the one you intended for storing Adapt It's free translation texts. All of the verses in the following book(s) already have text content:\n\n%s");
			msg = msg.Format(msg,m_TempCollabProjectForFreeTransExports.c_str(),booksWithContent.c_str());
			wxString msgTitle = _("No books in this project have untranslated texts!");
			wxString msg2 = _T("\n\n");
			msg2 += _("Are you sure you want to use the \"%s\" project for storing Adapt It's free translation texts?");
			msg2 = msg2.Format(msg2,m_TempCollabProjectForFreeTransExports.c_str());
			msg += msg2;
			int response;
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            response = wxMessageBox(msg,msgTitle,wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT);
			if (response == wxYES)
			{
				; // do nothing but continue with selection
			}
			else // if (response == wxNO || response || wxCANCEL)
			{
				// clear out the source control
				pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(wxEmptyString);
				m_TempCollabProjectForFreeTransExports = _T(""); // invalid project for source inputs
			}
			break;
		}
	case projHasNoBooksWithVerseText:
		{
			// All books in free trans project are "empty" of content. This would be the
			// expected situation for an initial setup.
			break;
		}
	case projHasSomeBooksWithVerseTextSomeWithout:
		{
			// The free trans PT/BE project has one or more books that have no content
			// other than \c n and \v n markers, and that other books have verse
			// content. This situation is posssible and permissible. Tell the user
			// the status of the books.
			wxString msg = _("The following books in the \"%s\" project do not yet have any verse content:\n\n%s\n\nAdapt It will store its free translations in these books as adaptation work proceeds.");
			msg = msg.Format(msg,m_TempCollabProjectForFreeTransExports.c_str(),emptyBooks.c_str());
			wxString msgTitle = _("Some books in this project have existing free translations!");
			wxString msg2 = _T("\n\n");
			msg2 += _("Please note that the following books in the \"%s\" project already have free translations:\n\n%s\n\nAdapt It will overwrite any existing free translations with new free translations if you adapt all the text in books such as these that already have free translations.");
			msg2 = msg2.Format(msg2,m_TempCollaborationEditor.c_str(),booksWithContent.c_str());
			msg += msg2;
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,msgTitle, wxICON_EXCLAMATION | wxOK);
			break;
		}
	case projHasNoChaptersOrVerses:
		{
			projShortName = GetShortNameFromProjectName(m_TempCollabProjectForFreeTransExports);
			// The book does not have at least one book in the Source project
			wxString msg,msg1,msg2,titleMsg;
			msg1 = _("This %s project has no books created in it. It cannot be used for storing free translation texts from Adapt It. Please run %s and select the %s project.");
			msg1 = msg1.Format(msg1,m_TempCollaborationEditor.c_str(),m_TempCollaborationEditor.c_str(),projShortName.c_str());
			if (m_TempCollaborationEditor == _T("Paratext"))
			{
				msg2 = _("Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and ensure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again.");
			}
			else if (m_TempCollaborationEditor == _T("Bibledit"))
			{
				msg2 = _("Then select File | Project | Properties. Then select \"Templates+\" from the Project properties dialog. Choose the book(s) to be created and click OK. Then return to Adapt It and try again.");
			}
			msg = msg1 + _T(' ') + msg2;
			titleMsg = _("No chapters and verses found in project: \"%s\"");
			titleMsg = titleMsg.Format(titleMsg,m_TempCollabProjectForFreeTransExports.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,titleMsg,wxICON_EXCLAMATION | wxOK);
			// clear out the source control
			pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(wxEmptyString);
			m_TempCollabProjectForFreeTransExports = _T(""); // invalid project for source inputs
			break;
		}
	case projHasNoBooks:
		{
			wxString msg,titleMsg;
			msg = _("This %s project contains no books. It cannot be used for storing free translation from Adapt It. Please go back to %s and create some empty books (with chapter and verse markers only) for this project, then return to Adapt It and try again.");
			msg = msg.Format(msg,m_TempCollaborationEditor.c_str(),m_TempCollaborationEditor.c_str());
			titleMsg = _("No books found in project \"%s\"");
			titleMsg = titleMsg.Format(titleMsg,m_TempCollabProjectForFreeTransExports.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,titleMsg,wxICON_EXCLAMATION | wxOK);
			// clear out the source control
			pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(wxEmptyString);
			pBtnNoFreeTrans->Disable();
			m_TempCollabProjectForFreeTransExports = _T(""); // invalid project for free trans exports
			m_bTempCollaborationExpectsFreeTrans = FALSE;
			break;
		}
	case processingError:
		{
			wxASSERT(!errorMsg.IsEmpty());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(errorMsg,_T(""),wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(msg);
			break;
		}
	}

	// If we get here the administrator made a selection. If the value changed mark it dirty.
	if (saveCollabProjectForFreeTransExports != m_TempCollabProjectForFreeTransExports)
		m_bCollabChangedThisDlgSession = TRUE;

    // After selecting source project, the dialog may be ready to "Accept this setup..."
    SetStateOfRemovalButton();
    SetStateOfAcceptSetupButton(); // enables the "Accept this setup and prepare for another" button if
                                   // all of: project name, editor, source text, or target text project names
                                   // have non-empty strings
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
	DoSetControlsFromConfigFileCollabData(FALSE); // Sets all Temp collab values as read from proj config file FALSE = we're NOT creating a new project
}

// This function gets the Collab... settings and their values from the project by calling the
// App's GetCollaborationSettingsOfAIProject() function. It stores the retrieved values in the
// Dialog's m_Temp... and m_bTemp... local variables. It also checks the values for consistency
// and validity and then sets the dialog's controls from the config file's collab data.
// DoSetControlsFromConfigFileCollabData() is called from: OnComboBoxSelectAiProject() handler (above).
void CSetupEditorCollaboration::DoSetControlsFromConfigFileCollabData(bool bCreatingNewProject)
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
		wxString saveCollabEditor = m_TempCollaborationEditor;
        // Scan through the two arrays in parallel and assign values to the m_Temp... and m_bTemp... variables
		for (ct = 0; ct < tot; ct++)
		{
			// Note: The main m_Temp... and m_bTemp values used by the SetupEditorCollaboration
			// dialog are:
			//    m_TempCollabProjectForSourceInputs
			//    m_TempCollabProjectForTargetExports
			//    m_TempCollabProjectForFreeTransExports
			//    m_TempCollabAIProjectName
			//    m_TempCollaborationEditor
            //    m_TempCollabEditorVersion // whm added 25June2016
			//    m_bTempCollabByChapterOnly
			// These m_Temp... and m_bTemp... values are derived from others or are unused
			//      m_bTempCollaborationExpectsFreeTrans
			//      m_TempCollabBookSelected
			//      m_TempCollabChapterSelected
			//      m_TempCollabSourceProjLangName
			//      m_TempCollabTargetProjLangName
            //      m_TempCollabBooksProtectedFromSavingToEditor // whm added 2February2017
            //      m_TempCollabDoNotShowMigrationDialogForPT7toPT8 // whm added 6April2017
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
            {
                this->m_TempCollaborationEditor = collabItemStr;
            }
            else if (collabLabelStr == szParatextVersionForProject) // the old proj config file label in case it exists (only used in a pre-release version to developers)
            {
                this->m_TempCollabEditorVersion = collabItemStr; // whm added 24June2016
            }
            else if (collabLabelStr == szCollabParatextVersionForProject) // the new proj config file label for ongoing use
            {
                this->m_TempCollabEditorVersion = collabItemStr; // whm added 24June2016
            }
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
                if (collabItemStr == _T("0"))
                    this->m_bTempCollabByChapterOnly = FALSE;
                else
                    this->m_bTempCollabByChapterOnly = TRUE;
            }
            else if (collabLabelStr == szCollabChapterSelected)
                this->m_TempCollabChapterSelected = collabItemStr;
            else if (collabLabelStr == szCollabSourceLangName)
                this->m_TempCollabSourceProjLangName = collabItemStr;
            else if (collabLabelStr == szCollabTargetLangName)
                this->m_TempCollabTargetProjLangName = collabItemStr;
            else if (collabLabelStr == szCollabBooksProtectedFromSavingToEditor)
                this->m_TempCollabBooksProtectedFromSavingToEditor = collabItemStr;
            else if (collabLabelStr == szCollabDoNotShowMigrationDialogForPT7toPT8)
            {
                if (collabItemStr == _T("0"))
                    this->m_TempCollabDoNotShowMigrationDialogForPT7toPT8 = FALSE;
                else
                    this->m_TempCollabDoNotShowMigrationDialogForPT7toPT8 = TRUE;
            }
		}

		// whm Note: InitDialog() initializes m_TempCollaborationEditor to be "Paratext"
		// or "Bibledit" depending on which is installed, giving preference to "Paratext"
		// if both editors are installed. That initial value is now saved in saveCollabEditor.
		// Now we need to deal with the possibility that the collaboration settings read in
		// from the project config file may specify a different editor, or no editor at all.
        //
        // whm 25June2016 note: We also need to deal with the possibility that the collaboration
        // settings read in from the project config file may specify a different PT editor
        // version, or no PT version at all.
		//
		// First, deal with the situation if m_TempCollaborationEditor is now empty after reading
        // project configuration collab data into m_TempCollaborationEditor. We can
		// examine m_TempCollabProjectForSourceInputs to see if it has ':' delimiters. If so the
		// project was previously a Paratext project; if not the project was a Bibledit project.
		// If the currently installed editor is compatible we can allow the assignment of
		// collaboration editor to stand. However, if m_TempCollaborationEditor now specifies an
		// editor that is not currently installed we must use what is installed instead.
		if (m_TempCollaborationEditor.IsEmpty())
		{
			if (!m_TempCollabProjectForSourceInputs.IsEmpty())
			{
				// we can determine which external editor was used previously by inspecting the
				// m_TempCollabProjectForSourceInputs string. If it has at least one colon delimiter
				// in its string, it was using Paratext, otherwise it was using Bibledit
				if (m_TempCollabProjectForSourceInputs.Find(_T(':')) != wxNOT_FOUND)
				{
					// At least one colon ':' character is present in string therefore it was previously
					// using Paratext. Assign "Paratext" as editor (if it is installed).
                    // whm 4Feb2020 added tests for PTVer9 and PTLinuxVer9 below
                    if (gbPTVer7Installed || gbPTVer8Installed || gbPTVer9Installed || gbPTLinuxVer7Installed || gbPTLinuxVer8Installed || gbPTLinuxVer9Installed)
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
			else
			{
				// No source PT/BE project was specified in m_TempCollabProjectForSourceInputs, so we
				// reassign m_TempCollaborationEditor back to what InitDialog() set it to
				m_TempCollaborationEditor = saveCollabEditor;
			}
		}
		else // m_TempCollaborationEditor is not an empty string
		{
            // m_TempCollaborationEditor should designate either "Paratext" or "Bibledit". We now conduct
            // some sanity checks for consistency of the collab settings of this project configuration file
            // with the state of the current computer.
            //
            // whm 30Dec2016 modified to Notify administrator that the AI project has collaboration settings
            // for collaboration with PT or BE, but neither collab editor is detected as installed on this
            // computer. Hence, the project selected cannot be set up and saved as a collaboration project
            // until Paratext or Bibledit are installed and have appropriate source and target language
            // projects available within the editor that Adapt It can utilize for collaboration.
            // Present the administrator with a wxMessageDialog with wxICON_EXCLAMATION | wxYES_NO | wxNO_DEFAULT.
            //
            // Note: The same basic prompt can be used both here and in the ProjectPage when a user attempts to
            // open a project that was previously set up for collaboration but a collaboration editor (Paratext
            // or Bibledit) are not installed on the current computer. I've relegated the prompt to a function
            // in CollabUtilities.cpp called: AskIfCollabSettingsBeRemovedFromProjConfig(). That function
            // returns an int representing the possible responses of: wxYES, wxNO, or wxCANCEL. The wxCANCEL is
            // not possible within Win10 which doesn't have an X in title bar allowing closure of the
            // wxMessageBox, but wxCANCEL may be available as a valid response on other platforms. See the
            // AskIfCollabSettingsBeRemovedFromProjConfig() function in CollabUtilities.cpp for the exact
            // wording of the prompt.

            // The following boolean is only set to TRUE when neither Paratext nor Bibledit are installed
            bool bNoCollabEditorfIsInstalled = FALSE;
			// m_TempCollaborationEditor has a non-empty value after reading the project config file's
			// collaboration settings. Here we can just ensure that the editor specified is installed
			if (m_TempCollaborationEditor == _T("Paratext") && gbPTNotInstalled)
			{
                // Paratext is the editor according to the confi file, but Paratext is not installed,
                // so if Bibledit is installed, assign it instead.
                if (m_pApp->BibleditIsInstalled())
                    m_TempCollaborationEditor = _T("Bibledit");
                else
                    bNoCollabEditorfIsInstalled = TRUE;
			}
			if (m_TempCollaborationEditor == _T("Bibledit") && !m_pApp->BibleditIsInstalled())
			{
                // Bibledit is the editor according to the config file, but Bibledit is not installed,
                // so if Paratext is installed, assign it instead.
                // whm 4Feb2020 added tests for PTVer9 and PTLinuxVer9 below
                if (gbPTVer7Installed || gbPTVer8Installed || gbPTVer9Installed || gbPTLinuxVer7Installed || gbPTLinuxVer8Installed || gbPTLinuxVer9Installed)
                    m_TempCollaborationEditor = _T("Paratext");
                else
                    bNoCollabEditorfIsInstalled = TRUE;
            }
            if (bNoCollabEditorfIsInstalled)
            {
                // Neither Paratext nor Bibledit is installed, but this project specifies collaboration with one or the
                // other editors.
                // Ask the administrator how to handle the inconsistent setting within the project's configuration file.
                // This message is also used in the ProjectPage's OnWizardPageChanging() handler, so we call a function
                // defined in CollabUtilities.cpp called AskIfCollabSettingsBeRemovedFromProjConfig(). The heart of the
                // code for handling this situation is between the dashed comment lines below:
// ---------------------------------------------------------------------------------------------------------------
                int result;
                result = AskIfCollabSettingsBeRemovedFromProjConfig(
                    m_TempCollabAIProjectName,
                    m_TempCollaborationEditor,
                    m_TempCollabProjectForSourceInputs,
                    m_TempCollabProjectForTargetExports,
                    m_TempCollabProjectForFreeTransExports
                );

                switch (result)
                {
                case wxCANCEL:
                    // Tell user what happened and what should be done.
                    // Don't put any project data into controls, just return (user can select a different AI project or close the dialog
                    return;

                case wxYES:
                {
                    // Administrator/User wants to remove collaboration settings from the project's AI-ProjectConfiguration.aic file.
                    // We could call the OnRemoveThisAIProjectFromCollab() handler but it does things that aren't necessary for our
                    // present purpose, so we'll just borrow the code from there and make it general enough so that it can be used here
                    // and also within the ProjectPage's OnWizardPageChanging() handler.
                    // Save blank values for the collab settings into the project config file.
                    m_pApp->m_bCollaboratingWithParatext = FALSE;
                    m_pApp->m_bCollaboratingWithBibledit = FALSE;
                    m_pApp->m_CollabProjectForSourceInputs = _T("");
                    m_pApp->m_CollabProjectForTargetExports = _T("");
                    m_pApp->m_CollabProjectForFreeTransExports = _T("");
                    m_pApp->m_CollabAIProjectName = _T("");
                    m_pApp->m_collaborationEditor = _T("");
                    m_pApp->m_ParatextVersionForProject = _T(""); // whm added 25June2016
                    m_pApp->m_bCollaborationExpectsFreeTrans = FALSE;
                    m_pApp->m_CollabBookSelected = _T("");
                    m_pApp->m_CollabSourceLangName = _T("");
                    m_pApp->m_CollabTargetLangName = _T("");
                    m_pApp->m_bCollabByChapterOnly = TRUE;
                    m_pApp->m_CollabChapterSelected = _T("");
                    m_pApp->m_CollabBooksProtectedFromSavingToEditor = _T(""); // whm added 2February2017
                    m_pApp->m_bCollabDoNotShowMigrationDialogForPT7toPT8 = FALSE; // whm added 6April2017

                    // Re-save now removed settings to the AI-ProjectConfiguration.aic file
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
                    bOK = m_pApp->WriteConfigurationFile(szProjectConfiguration, newProjectPath, projectConfigFile);
                    if (bOK)
                    {
                        wxString newAIconfigFilePath = newProjectPath + m_pApp->PathSeparator + szProjectConfiguration + _T(".aic");
                        // Tell administrator that the setup has been saved.
                        wxString msg0 = _("The collaboration settings for the \"%s\" project were successfully removed from the project's configuration file at:\n\n%s\n\n");
                        //wxString msg1 = _("You may now select or create another Adapt It project (step 1) and make collaboration settings for that Adapt It project (setps 2 - 4).\n\n");
                        wxString msg2 = _("If you are finished, select \"Close\" to close the setup dialog and test your setup(s) using the Start Working Wizard.");
                        wxString msg;
                        msg = msg0 + msg2;
                        msg = msg.Format(msg, m_TempCollabAIProjectName.c_str(), newAIconfigFilePath.c_str());
                        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
                        wxMessageBox(msg, _("Removal of collaboration settings successful"), wxICON_INFORMATION | wxOK);
                        m_pApp->LogUserAction(msg);
                        m_bCollabChangedThisDlgSession = FALSE;

                        // whm 25Oct13 added. This SaveAppCollabSettingsToINIFile() needs to be called
                        // at every point that significant collaboration settings change to keep the
                        // Adapt_It_WX.ini file up to date. This does not remove the settings
                        // entirely from the ini file, it clears the existing settings to
                        // remove active collaboration.
                        m_pApp->SaveAppCollabSettingsToINIFile(newProjectPath);
                    }
                    else
                    {
                        // Writing of the project config file failed for some reason. This would be unusual, so
                        // just do an English notification
                        wxCHECK_RET(bOK, _T("OnRemoveThisAIProjectFromCollab(): WriteConfigurationFile() failed, line 1556 in SetupEditorCollaboration.cpp"));
                    }

                    // The following actions are only needed within the SetupEditorCollaboration dialog and not applicable in
                    // the ProjectPage's OnWizardPageChanging() handler.

                    // zero out the local Temp... variables as we did in OnInit()
                    m_TempCollabProjectForSourceInputs = _T("");
                    m_TempCollabProjectForTargetExports = _T("");
                    m_TempCollabProjectForFreeTransExports = _T("");
                    m_TempCollabAIProjectName = _T("");
                    m_TempCollaborationEditor = _T("");
                    m_TempCollabEditorVersion = _T(""); // whm 25June2016 added
                    m_bTempCollaborationExpectsFreeTrans = FALSE; // defaults to FALSE for no free trans
                    m_TempCollabBookSelected = _T("");
                    m_TempCollabSourceProjLangName = _T("");
                    m_TempCollabTargetProjLangName = _T("");
                    m_bTempCollabByChapterOnly = TRUE; // defaults to TRUE for collab by chapter only
                    m_TempCollabChapterSelected = _T("");
                    m_TempCollabBooksProtectedFromSavingToEditor = _T(""); // whm added 2February2017
                    m_TempCollabDoNotShowMigrationDialogForPT7toPT8 = FALSE; // whm added 6April2017

                    // Zero out all fields so the dialog is ready for another setup if desired
                    wxString strSelection = pComboAiProjects->GetStringSelection();
                    if (!strSelection.IsEmpty())
                    {
                        pComboAiProjects->Remove(0, strSelection.Length());
                    }
                    pComboAiProjects->SetSelection(-1); // remove any selection from the combo box
                    pComboAiProjects->Refresh();
                    pTextCtrlAsStaticSelectedSourceProj->ChangeValue(m_TempCollabProjectForSourceInputs);
                    pTextCtrlAsStaticSelectedTargetProj->ChangeValue(m_TempCollabProjectForTargetExports);
                    pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(m_TempCollabProjectForFreeTransExports);
                    pRadioBtnByChapterOnly->SetValue(m_bTempCollabByChapterOnly);
                    pRadioBtnByWholeBook->SetValue(!m_bTempCollabByChapterOnly);
                    SetStateOfRemovalButton();
                    SetStateOfAcceptSetupButton(); // disables the "Accept this setup and prepare for another" button if
                                                   // any of: project name, editor, source text, or target text project names
                                                   // are empty

                    // Just return from this DoSetControlsFromConfigFileCollabData() function to the dialog's
                    // main event loop, to allow administrator to either select another AI project or to Close
                    // the dialog.
                    return;
                }

                default:
                    wxASSERT(FALSE); // this would be a programming error

                case wxNO:
                    // Administrator/User decided not to remove the settings
                    // Tell user what happened and what should be done.
                    wxString projects;
                    projects = m_TempCollabProjectForSourceInputs + _T("\n") + m_TempCollabProjectForTargetExports, +_T("\n") + m_TempCollabProjectForFreeTransExports;

                    wxString msg =
                        _("The collaboration settings for the \"%s\" AI project were not changed.\n\n\
Reminder: The user will not be able to open this project until you install Paratext or Bibledit along with these projects:\n\n\
%s\n\n");
                    msg = msg.Format(msg, m_TempCollabAIProjectName.c_str(), projects.c_str());
                    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                    m_pApp->m_bUserDlgOrMessageRequested = TRUE;
                    wxMessageBox(msg, _("Collaboration settings remain unchanged for this project"), wxICON_INFORMATION | wxOK);
                    m_pApp->LogUserAction(msg);
                    // Disable and refresh the dialog AI Project selection controls
                    pComboAiProjects->SetSelection(-1); // remove any selection from the combo box
                    pComboAiProjects->Refresh();
                    SetStateOfRemovalButton();
                    SetStateOfAcceptSetupButton(); // disables the "Accept this setup and prepare for another" button if
                                                   // any of project name, editor, source text, or target text project names
                                                   // are empty
                    // The call to GetCollaborationSettingsOfAIProject() at the beginning of this function assigned
                    // project config values to all of the m_TempCollab... variables. We should zero out those temp
                    // variables here so that they don't influence something else in the dialog.
                    // zero out the local Temp... variables as we did in OnInit()
                    m_TempCollabProjectForSourceInputs = _T("");
                    m_TempCollabProjectForTargetExports = _T("");
                    m_TempCollabProjectForFreeTransExports = _T("");
                    m_TempCollabAIProjectName = _T("");
                    m_TempCollaborationEditor = _T("");
                    m_TempCollabEditorVersion = _T(""); // whm 25June2016 added
                    m_bTempCollaborationExpectsFreeTrans = FALSE; // defaults to FALSE for no free trans
                    m_TempCollabBookSelected = _T("");
                    m_TempCollabSourceProjLangName = _T("");
                    m_TempCollabTargetProjLangName = _T("");
                    m_bTempCollabByChapterOnly = TRUE; // defaults to TRUE for collab by chapter only
                    m_TempCollabChapterSelected = _T("");
                    m_TempCollabBooksProtectedFromSavingToEditor = _T(""); // whm added 2February2017
                    m_TempCollabDoNotShowMigrationDialogForPT7toPT8 = FALSE; // whm added 6April2017

                    return;

                }
// ---------------------------------------------------------------------------------------------------------------
            }
		} // end of else // m_TempCollaborationEditor is not an empty string

        // whm 25June2016 added: We now deal with the possibility that the collaboration
        // settings read in from the project config file may specify a different PT editor
        // version, or no PT version at all. In the previous blocks we checked the validity
        // of the editor itself and made adjustments to ensure that the editor specified is
        // actually installed. Here we assume we have a valid editor, but we make some checks
        // to verify that any specified PT version is actually installed.

        // Note: When the PT version string read in represents an existing/valid older PT 
        // installation (contiaining projects) we should leave the m_TempCollabEditorVersion
        // set to the value read from the project config file, even if there is a newer PT
        // version installed on the same computer. The Admin user can use the dialog's 
        // radio box buttons to adjust the version number if desired to point to a newer PT
        // version.
        // 
        // whm 4Feb2020 modification. The code to validate the collab editor and any PT version
        // was getting more complex and needed to be used in several places, so I created a
        // new function ValidateCollabEditorAndVersionStrAgainstInstallationData().

        m_TempCollabEditorVersion = m_pApp->ValidateCollabEditorAndVersionStrAgainstInstallationData(m_TempCollaborationEditor, m_TempCollabEditorVersion);
        
        // Check the AI project name values for consistency.
		if (!m_TempCollabAIProjectName.IsEmpty() && (this->m_TempCollabSourceProjLangName.IsEmpty() || this->m_TempCollabTargetProjLangName.IsEmpty()))
		{
			// Do sanity check to insure the m_TempCollabSourceProjLangName and m_TempCollabTargetLangName
			// values are consistent with those used in the m_TempCollabAIProjectName.
			// The AI project name is defined, but the individual source and/or target language names for
			// the project are empty, so parse them from the language name. The function assigns values to the 2nd and 3rd parmeters.
			GetAILangNamesFromAIProjectNames(m_TempCollabAIProjectName, m_TempCollabSourceProjLangName, m_TempCollabTargetProjLangName);
		}
	} // end of if (collabSettingsArray.GetCount() > 0)
	else
	{
        // The collabSettingsArray.GetCount() == 0, indicating that the currently selected AI project
        // has no collaboration settings present within its AI-ProjectConfiguration.aic file.
        // Initialize the m_TempCollab... variables that we can including m_TempCollabEditorVersion.
        // We can parse the drop down AI project selection string (setStr) and use
		// the values we parse out for m_TempCollabAIProjectName, m_TempCollabSourceProjLangName,
        // and m_TempCollabTargetProjLangName
        m_TempCollabProjectForSourceInputs = _T("");
		m_TempCollabProjectForTargetExports = _T("");
		m_TempCollabProjectForFreeTransExports = _T("");

		m_TempCollabAIProjectName = selStr; // AI project name selected in the combo box

		// Call the ParatextVersionInstalled() or BibleditIsInstalled() functions directly
		// here rather than using the App's m_bParatextIsInstalled or m_bBibleditIsInstalled
		// variables (which are only set in OnInit(). The user may have installed one of the
		// editors during this AI session.
        //PTVersionsInstalled PTver;
        //PTver = m_pApp->ParatextVersionInstalled();
        // whm 4Feb2020 added tests for PTVer9 and PTLinuxVer9 below

        m_TempCollabEditorVersion = m_pApp->ValidateCollabEditorAndVersionStrAgainstInstallationData(m_TempCollaborationEditor, m_TempCollabEditorVersion);

        /*
        if (PTver == PTVer7 || PTver == PTVer8 || PTver == PTVer9 || PTver == PTVer7and8 || PTver == PTLinuxVer7 || PTver == PTLinuxVer8 || PTver == PTLinuxVer9 || PTver == PTLinuxVer7and8)
        {
		     m_TempCollaborationEditor = _T("Paratext"); // default editor
             if (PTver == PTVer7)
             {
                 // Only PT version 7 is installed, so ensure that m_TempCollabEditorVersion is set to "PTVersion7"
                 m_TempCollabEditorVersion = _T("PTVersion7");
             }
             else if (PTver == PTVer8)
             {
                 // Only PT version 8 is installed, so ensure that m_TempCollabEditorVersion is set to "PTVersion8"
                 m_TempCollabEditorVersion = _T("PTVersion8");
             }
             // whm 4Feb2020 added test for PTVer9 below
             else if (PTver == PTVer9)
             {
                 // Only PT version 8 is installed, so ensure that m_TempCollabEditorVersion is set to "PTVersion8"
                 m_TempCollabEditorVersion = _T("PTVersion9");
             }
             else if (PTver == PTVer7and8)
             {
                 // This block was entered only when no project config collab settings were detected,
                 // so there won't be a PT version already in m_TempCollabEditorVersion to utilize, even
                 // though both PT versions 7 and 8 are installed on the machine.
                 // Assume that PT 8 will be the desired editor if it has a valid projects dir
                 // and at least 2 valid/useable projects, otherwise PT 7
                 wxString pt8ProjectsDirPath = m_pApp->GetParatextProjectsDirPath(_T("PTVersion8"));
                 wxString pt7ProjectsDirPath = m_pApp->GetParatextProjectsDirPath(_T("PTVersion7"));
                 wxArrayString pt8ListOfProj = m_pApp->GetListOfPTProjects(_T("PTVersion8"));
                 wxArrayString pt7ListOfProj = m_pApp->GetListOfPTProjects(_T("PTVersion7"));
                 int pt8Count = pt8ListOfProj.GetCount();
                 int pt7Count = pt7ListOfProj.GetCount();
                 if (!pt8ProjectsDirPath.IsEmpty() && pt8Count >= 2)
                 {
                     m_TempCollabEditorVersion = _T("PTVersion8");
                 }
                 else if (!pt7ProjectsDirPath.IsEmpty() && pt7Count >= 2)
                 {
                     m_TempCollabEditorVersion = _T("PTVersion7");
                 }
                 else
                 {
                     // neither PT 8 or PT 7 had valid projects dir path nor at least 2 useable projects, log the problem
                     wxString msg = _T("In CSetupEditorCollaboration::DoSetControlsFromConfigFileCollabData() both PT 7 and PT 8 are installed but neither has valid projects dir or neither has at least 2 useable projects.");
                     m_pApp->LogUserAction(msg);
                     // Assume the administrator will fix this situation. In the mean time, since both
                     // PT 8 and PT 7 are installed, default here in InitDialog() to PT 8.
                     m_TempCollabEditorVersion = _T("PTVersion8");
                 }
             }
             // whm 4Feb2020 added test for PTLinuxVer9 below
             else if (PTver == PTLinuxVer9)
             {
                 m_TempCollabEditorVersion = _T("PTLinuxVersion9");
             }
             else if (PTver == PTLinuxVer8)
             {
                 m_TempCollabEditorVersion = _T("PTLinuxVersion8");
             }
             else if (PTver == PTLinuxVer7)
             {
                 m_TempCollabEditorVersion = _T("PTLinuxVersion7");
             }
             else if (PTver == PTLinuxVer7and8)
             {
                 // This block was entered only when no project config collab settings were detected,
                 // so there won't be a PT version already in m_TempCollabEditorVersion to utilize, even
                 // though both PT versions 7 and 8 are installed on the machine.
                 // Assume that PT 8 will be the desired editor if it has a valid projects dir
                 // and at least 2 valid/useable projects, otherwise PT 7
                 wxString pt8ProjectsDirPath = m_pApp->GetParatextProjectsDirPath(_T("PTLinuxVersion8"));
                 wxString pt7ProjectsDirPath = m_pApp->GetParatextProjectsDirPath(_T("PTLinuxVersion7"));
                 wxArrayString pt8ListOfProj = m_pApp->GetListOfPTProjects(_T("PTLinuxVersion8"));
                 wxArrayString pt7ListOfProj = m_pApp->GetListOfPTProjects(_T("PTLinuxVersion7"));
                 int pt8Count = pt8ListOfProj.GetCount();
                 int pt7Count = pt7ListOfProj.GetCount();
                 if (!pt8ProjectsDirPath.IsEmpty() && pt8Count >= 2)
                 {
                     m_TempCollabEditorVersion = _T("PTLinuxVersion8");
                 }
                 else if (!pt7ProjectsDirPath.IsEmpty() && pt7Count >= 2)
                 {
                     m_TempCollabEditorVersion = _T("PTLinuxVersion7");
                 }
                 else
                 {
                     // neither PT 8 or PT 7 had valid projects dir path nor at least 2 useable projects, log the problem
                     wxString msg = _T("In CSetupEditorCollaboration::DoSetControlsFromConfigFileCollabData() both PT 7 and PT 8 are installed but neither has valid projects dir or neither has at least 2 useable projects.");
                     m_pApp->LogUserAction(msg);
                     // Assume the administrator will fix this situation. In the mean time, since both
                     // PT 8 and PT 7 are installed, default here in InitDialog() to PT 8.
                     m_TempCollabEditorVersion = _T("PTLinuxVersion8");
                 }
             }
        }
		else if (m_pApp->BibleditIsInstalled())
		{
		     m_TempCollaborationEditor = _T("Bibledit"); // don't localize
            // Bibledit is the only installed external editor, so it doesn't make
            // sense to set m_TempCollabEditorVersion
		}
        */

		m_bTempCollaborationExpectsFreeTrans = FALSE; // defaults to FALSE for no free trans
		m_TempCollabBookSelected = _T("");

		// Finally, parse the language names from the m_TempCollabAIProjectName
		m_pApp->GetSrcAndTgtLanguageNamesFromProjectName(selStr,
			m_TempCollabSourceProjLangName,m_TempCollabTargetProjLangName);

		m_bTempCollabByChapterOnly = TRUE; // defaults to TRUE for collab by chapter only
		m_TempCollabChapterSelected = _T("");
	} // end of else collabSettingsArray.GetCount() == 0 [no collab settings in proj config file]

	// Since the m_TempCollaborationEditor may have changed above we need to get a fresh
	// list of editor projects
	projList.Clear();
	if (m_TempCollaborationEditor == _T("Paratext"))
	{
        projList = m_pApp->GetListOfPTProjects(m_TempCollabEditorVersion); // as a side effect, it populates the App's m_pArrayOfCollabProjects
    }
	else if (m_TempCollaborationEditor == _T("Bibledit"))
	{
		projList = m_pApp->GetListOfBEProjects(); // as a side effect, it populates the App's m_pArrayOfCollabProjects
	}

    // =====================================================================================================
    // Note: The OnComboBoxSelectAiProject() handler calls this DoSetControlsFromConfigFileCollabData()
    // function. Therefore, regardless of what the value for m_TempCollabAIProjectName might have been in the
	// project config file collaboration settings (from above), we force it to be what the
	// administrator selected within the combo box, and force the associated language names too.

	m_TempCollabAIProjectName = selStr; // AI project name selected in the combo box
	// parse the language names from the m_TempCollabAIProjectName
	m_pApp->GetSrcAndTgtLanguageNamesFromProjectName(selStr,
		m_TempCollabSourceProjLangName,m_TempCollabTargetProjLangName);

    // If the m_TempCollaborationEditor string is still empty at this point, it indicates that
    // there was no editor specified in the config file. In such cases initialize it to an
    // installed editor if one is available, giving preference to Paratext
    if (m_TempCollaborationEditor.IsEmpty())
    {
        // m_TempCollaborationEditor is empty so make empty the m_TempCollabProjectFor... variables too
        // since they may not be up to date - user will have to select from updated list of projects
        m_TempCollabProjectForSourceInputs.Empty();
        m_TempCollabProjectForTargetExports.Empty();
        m_TempCollabProjectForFreeTransExports.Empty();
    }

    m_TempCollabEditorVersion = m_pApp->ValidateCollabEditorAndVersionStrAgainstInstallationData(m_TempCollaborationEditor, m_TempCollabEditorVersion);

    SetRadioBoxCollabEditorSelection();

	// Fill dialog controls with collab settings
	// Note: The combo box selection in step 1 was selected by the administrator so something should be selected!
	wxASSERT(pComboAiProjects->GetSelection() != -1);
	pTextCtrlAsStaticSelectedSourceProj->ChangeValue(m_TempCollabProjectForSourceInputs);
	pTextCtrlAsStaticSelectedTargetProj->ChangeValue(m_TempCollabProjectForTargetExports);
	pTextCtrlAsStaticSelectedFreeTransProj->ChangeValue(m_TempCollabProjectForFreeTransExports);
	pRadioBtnByChapterOnly->SetValue(m_bTempCollabByChapterOnly);
	pRadioBtnByWholeBook->SetValue(!m_bTempCollabByChapterOnly);

	SetStateOfRemovalButton(); // enables the Remove... button because m_TempCollabAIProjectName now has content
    SetStateOfAcceptSetupButton(); // disables the "Accept this setup and prepare for another" button if
                                   // any of project name, editor, source text, or target text project names
                                   // are empty

	// Don't set m_bCollabChangedThisDlgSession to TRUE in this handler for merely selecting an existing project

	// Refresh the list of PT/BE projects
	int nProjectCount = 0;
	projList.Clear();
	if (m_TempCollaborationEditor == _T("Paratext"))
	{
        projList = m_pApp->GetListOfPTProjects(m_TempCollabEditorVersion); // as a side effect, it populates the App's m_pArrayOfCollabProjects
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
        ;
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

    // whm added 30Dec206
    if (m_TempCollaborationEditor.IsEmpty())
    {
        // PT/BE are not installed so just return
        return;
    }

	// The PT/BE project source and/or target name variables may be empty if they were empty in
	// the config file or when this function is called by OnCreateNewAIProject(). If one or both
	// are empty strings, remind the user to use the "Select from List" buttons
	if (m_TempCollabProjectForSourceInputs.IsEmpty() || m_TempCollabProjectForTargetExports.IsEmpty())
	{
		wxString msg = _("Please use the \"Select from list\" buttons to select the appropriate %s projects.");
		msg = msg.Format(msg, m_TempCollaborationEditor.c_str());
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(msg,_T(""),wxICON_INFORMATION | wxOK);
		if (m_TempCollabProjectForSourceInputs.IsEmpty())
			this->pBtnSelectFmListSourceProj->SetFocus();
		else
			this->pBtnSelectFmListTargetProj->SetFocus();
		return; // return here otherwise the block below will also flag these projects as invalid because
				// they have no books created - the actual problem here is that they aren't projects found
				// in the PT/BE list of projects.
	}
	// whm added 5Jun12. If we were called from OnCreateNewAIProject, the PT/BE project names
	// won't have been entered, so we can just return without doing the checks below
	if (bCreatingNewProject)
	{
		return;
	}

	// First check if the projects exist in the list of PT/BE projects. If they don't exist there
	// then notify the user.

// ***************
	// The code for testing if project exist in the editor's list of projects below borrowed from
	// similar code in the App's GetAIProjectCollabStatus() function.
	//
	// both bFoundCollabSrcProj and bFoundBollabTgtProj are TRUE, so the project config file
	// contains strings for both. Now test that they exist in the inventory of current PT/BE
	// projects (in projList) by calling CollabProjectFoundInListOfEditorProjects() on each
	// PT/BE project name that appears in the project config file.
	wxString foundSrcProjString = _T("");
	wxString foundTgtProjString = _T("");
	wxString foundFreeTransProjString = _T("");
	// In the CollabProjectFoundInListOfEditorProjects() function calls below the short name
	// of the project is used (in PT) to match an existing PT/BE project (in projList). If
	// the returned value is FALSE, no project was found, if the returned value is TRUE the
	// project was found and the returned by reference foundProjString parameter will contain
	// the actual composed string of the project - this is used to correct any typos in the
	// full name part of a PT composite project id string.
	wxString msg = _T("");
	wxString projects = _T("");
	bool srcProjFoundInEditor = TRUE;
	bool tgtProjFoundInEditor = TRUE;
	bool freeTransProjFoundInEditor = TRUE;
	srcProjFoundInEditor = CollabProjectFoundInListOfEditorProjects(m_TempCollabProjectForSourceInputs,projList,foundSrcProjString);
	tgtProjFoundInEditor = CollabProjectFoundInListOfEditorProjects(m_TempCollabProjectForTargetExports,projList,foundTgtProjString);
	if (!m_TempCollabProjectForFreeTransExports.IsEmpty())
		freeTransProjFoundInEditor = CollabProjectFoundInListOfEditorProjects(m_TempCollabProjectForFreeTransExports,projList,foundFreeTransProjString);
	if (!srcProjFoundInEditor)
	{
		// The project config file has an entry for the CollabSrcProjStrFound, but that project
		// could not be found in the PT/BE editor's list of current projects.
		wxString msgAdd = _("The \"%s\" project cannot be found as a %s project for obtaining source texts.");
		msgAdd = msgAdd.Format(msgAdd,m_TempCollabProjectForSourceInputs.c_str(), m_TempCollaborationEditor.c_str());
		msgAdd = _T("1. ") + msgAdd; // src was not found and is item 1
		msg += msgAdd;
		projects = _T("source");
	}
	else
	{
		// The project was found in the PT/BE editor's list of current projects.
		wxASSERT(!foundSrcProjString.IsEmpty());
		// Ensure the spelling of of full project name part in the config file is
		// correct as used within the actual editor's full project name (user could
		// have changed it in PT).
		if (m_TempCollabProjectForSourceInputs != foundSrcProjString)
		{
			// There was an irregularity in spelling of a token in the 2nd through 4th fields of the
			// composite project string. Fix the App's value and set flag to save the project config
			// file changes.
			m_pApp->m_CollabProjectForSourceInputs = foundSrcProjString;
			//bChangeMadeToCollabSettings = TRUE;
		}
	}
	if (!tgtProjFoundInEditor)
	{
		// The project config file has an entry for the CollabTgtProjStrFound, but that project
		// could not be found in the PT/BE editor's list of current projects.
		wxString msgAdd = _("The \"%s\" project cannot be found as a %s project for storing translation texts.");
		msgAdd = msgAdd.Format(msgAdd,m_TempCollabProjectForTargetExports.c_str(), m_TempCollaborationEditor.c_str());
		if (!srcProjFoundInEditor)
		{
			// src was not found and was item 1; tgt is item 2.
			msgAdd = _T("\n2. ") + msgAdd;
		}
		else
		{
			// src was found; tgt is now item 1.
			msgAdd = _T("1. ") + msgAdd;
		}
		msg += msgAdd;
		if (!projects.IsEmpty())
			projects += _T(':');
		projects += _T("target");
	}
	else
	{
		// The project was found in the PT/BE editor's list of current projects.
		wxASSERT(!foundTgtProjString.IsEmpty());
		// Ensure the spelling of of full project name part in the config file is
		// correct as used within the actual editor's full project name (user could
		// have changed it in PT).
		if (m_TempCollabProjectForTargetExports != foundTgtProjString)
		{
			// There was an irregularity in spelling of a token in the 2nd through 4th fields of the
			// composite project string. Fix the App's value and set flag to save the project config
			// file changes.
			m_pApp->m_CollabProjectForTargetExports = foundTgtProjString;
			//bChangeMadeToCollabSettings = TRUE;
		}
	}
	if (!m_TempCollabProjectForFreeTransExports.IsEmpty() && !freeTransProjFoundInEditor)
	{
		// The project config file has an entry for the CollabFreeTransProjStrFound, but that project
		// could not be found in the PT/BE editor's list of current projects.
		wxString msgAdd = _("The \"%s\" project cannot be found as a %s project for storing free translation texts.");
		msgAdd = msgAdd.Format(msgAdd,m_TempCollabProjectForFreeTransExports.c_str(), m_TempCollaborationEditor.c_str());
		if (!srcProjFoundInEditor)
		{
			// src was not found as so was item 1
			if (!tgtProjFoundInEditor)
			{
				// both src and tgt were not found (items 1 and 2); free trans is now item 3.
				msgAdd = _T("\n3. ") + msgAdd;
			}
			else
			{
				// src was not found and was item 1, but tgt was found; free trans is now item 2.
				msgAdd = _T("\n2. ") + msgAdd;
			}
		}
		else
		{
			// src was found
			if (!tgtProjFoundInEditor)
			{
				// src was found but target not found and was item 1; free trans is item 2.
				msgAdd = _T("\n2. ") + msgAdd;
			}
			else
			{
				// both src and tgt were found; free trans is first item, item 1.
				msgAdd = _T("1. ") + msgAdd;
			}
		}
		msg += msgAdd;
		if (!projects.IsEmpty())
			projects += _T(':');
		projects += _T("freetrans");
	}
	else if (!m_TempCollabProjectForFreeTransExports.IsEmpty())
	{
		// The project was found in the PT/BE editor's list of current projects.
		wxASSERT(!foundFreeTransProjString.IsEmpty());
		// Ensure the spelling of of full project name part in the config file is
		// correct as used within the actual editor's full project name (user could
		// have changed it in PT).
		if (m_TempCollabProjectForFreeTransExports != foundFreeTransProjString)
		{
			// There was an irregularity in spelling of a token in the 2nd through 4th fields of the
			// composite project string. Fix the App's value and set flag to save the project config
			// file changes.
			m_pApp->m_CollabProjectForFreeTransExports = foundFreeTransProjString;
			//bChangeMadeToCollabSettings = TRUE;
		}
	}

	if (!srcProjFoundInEditor || !tgtProjFoundInEditor || (!m_TempCollabProjectForFreeTransExports.IsEmpty() && !freeTransProjFoundInEditor))
	{
		// msg has the info we need to notify the user
		wxString msg2 = _("Adapt It detected invalid collaboration settings for the \"%s\" project in its project configuration file. The invalid %s project data is:\n\n%s");
		wxASSERT(!m_TempCollaborationEditor.IsEmpty());
		msg2 = msg2.Format(msg2,selStr.c_str(),m_TempCollaborationEditor.c_str(),msg.c_str());
		msg2 += _T("\n\n");
		msg2 += _("Please use the \"Select from list\" buttons to select the appropriate %s projects.");
		msg2 = msg2.Format(msg2, m_TempCollaborationEditor.c_str());
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(msg2,_T(""),wxICON_INFORMATION | wxOK);
		return; // return here otherwise the block below will also flag these projects as invalid because
				// they have no books created - the actual problem here is that they aren't projects found
				// in the PT/BE list of projects.
	}

	// If the m_TempCollabProjectForFreeTransExports string is empty ensure that the
	// m_bTempCollaborationExpectsFreeTrans flag is also set to FALSE, or if it has content
	// that it is set to TRUE.
	m_bTempCollaborationExpectsFreeTrans = !m_TempCollabProjectForFreeTransExports.IsEmpty();

// ***************

	wxString errorStr = _T("");
	wxString errProj = _T("");
	// CollabProjectsAreValid() doesn't consider any empty string projects to be invalid, only those non-empty
	// string parameters that don't have at least one book created in them. Therefore, selecting an AI
	// project that has no collaboration settings won't trigger the "Invalid PT/BE projects detected" message below.
	if (!CollabProjectsAreValid(m_TempCollabProjectForSourceInputs, m_TempCollabProjectForTargetExports,
			m_TempCollabProjectForFreeTransExports, m_TempCollaborationEditor, m_TempCollabEditorVersion,
            errorStr, errProj))
	{
		wxString msg = _("Adapt It detected invalid collaboration settings for the \"%s\" project in its project configuration file. The invalid %s project data is:\n%s");
		wxASSERT(!m_TempCollaborationEditor.IsEmpty());
		msg = msg.Format(msg,selStr.c_str(),m_TempCollaborationEditor.c_str(),errorStr.c_str());
		// Note: The errProj returned string is not used here.
		msg += _T("\n\n");
		wxString msg1 = _("Please set up valid %s project(s) that Adapt It can use for collaboration.");
		msg1 = msg1.Format(msg1,m_TempCollaborationEditor.c_str());
		msg += msg1;
		wxString titleMsg = _("Invalid %s projects detected");
		titleMsg = titleMsg.Format(titleMsg,m_TempCollaborationEditor.c_str());
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(msg,titleMsg,wxICON_EXCLAMATION | wxOK);
		m_pApp->LogUserAction(titleMsg);
	}
    else
    {
        // Check if the selected collab project should be migrated from pointing to PT7 to PT8
        bool bBothPT7AndPT8InstalledPT8ProjectsWereMigrated = FALSE;
        if (srcProjFoundInEditor && tgtProjFoundInEditor)
        {
            // Check the PT Windows situation
            if (m_TempCollabEditorVersion == _T("PTVersion7"))
            {
                if (gbPTVer7Installed && gbPTVer8Installed) //if (PTver == PTVer7and8)
                {
                    wxString errStr = _T("");
                    wxString errProj = _T("");
                    // We know that the PT7 projects are valid. Check if the PT8 projects are also valid.
                    if (CollabProjectsAreValid(m_TempCollabProjectForSourceInputs, m_TempCollabProjectForTargetExports,
                        m_TempCollabProjectForFreeTransExports, m_TempCollaborationEditor, _T("PTversion8"),
                        errStr, errProj))
                    {
                        // Compare the GUID values of the PT7 and PT8 projects to determine if
                        // the PT7 projects were migrated to PT8. According to Tom H. the PT8
                        // project will have the same GUID as the PT7 project when the project
                        // has successfully migrated to PT8.
                        // Approach: Compare the GUIDs for the Src, Tgt (and any FreeTrans project)
                        //   between PT7 and PT8. If the GUIDs match for all collab projects, then
                        // we can return a TRUE value for bBothPT7AndPT8InstalledPT8ProjectsWereMigrated.
                        //
                        bBothPT7AndPT8InstalledPT8ProjectsWereMigrated = CollabProjectsMigrated(m_TempCollabProjectForSourceInputs, m_TempCollabProjectForTargetExports, m_TempCollabProjectForFreeTransExports,
                            m_TempCollaborationEditor, _T("PTVersion7"), _T("PTVersion8"));
                    }
                }
            }
            else if (m_TempCollabEditorVersion == _T("PTLinuxVersion7"))
            {
                // Check the PT Linux situation
                if (gbPTLinuxVer7Installed && gbPTLinuxVer8Installed) //if (PTver == PTLinuxVer7and8)
                {
                    wxString errStr = _T("");
                    wxString errProj = _T("");
                    // We know that the PT7 projects are valid. Check if the PT8 projects are also valid.
                    if (CollabProjectsAreValid(m_TempCollabProjectForSourceInputs, m_TempCollabProjectForTargetExports,
                        m_TempCollabProjectForFreeTransExports, m_TempCollaborationEditor, _T("PTLinuxversion8"),
                        errStr, errProj))
                    {
                        // Compare the GUID values of the PT7 and PT8 projects to determine if
                        // the PT7 projects were migrated to PT8. According to Tom H. the PT8
                        // project will have the same GUID as the PT7 project when the project
                        // has successfully migrated to PT8.
                        // Approach: Compare the GUIDs for the Src, Tgt (and any FreeTrans project)
                        //   between PT7 and PT8. If the GUIDs match for all collab projects, then
                        // we can return a TRUE value for bBothPT7AndPT8InstalledPT8ProjectsWereMigrated.
                        //
                        bBothPT7AndPT8InstalledPT8ProjectsWereMigrated = CollabProjectsMigrated(m_TempCollabProjectForSourceInputs, m_TempCollabProjectForTargetExports, m_TempCollabProjectForFreeTransExports,
                            m_TempCollaborationEditor, _T("PTLinuxVersion7"), _T("PTLinuxversion8"));
                    }

                }
            }
        }
        if (bBothPT7AndPT8InstalledPT8ProjectsWereMigrated)
        {
            wxString msg = _("This project is currently set up to collaborate with the selected Paratext 7 projects. Adapt It has determined that those Paratext 7 projects have been migrated to Paratext 8 on this computer. It is strongly recommended that you select Paratext 8 in Step 2 below, and the appropriate migrated projects in Step 3 below.\n\nReminder: If Paratext 7 continues to be the selected Scripture editor after collaboration project were migrated from Paratext 7 to Paratext 8, Adapt It collaboration work will appear only in the old Paratext 7 projects, and not in the Paratext 8 projects. ");
            wxString title;
            title = _("Important information about collaboration project: %s");
            title = title.Format(title, m_TempCollabAIProjectName.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
        }
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
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        response = wxMessageBox(msg,_T(""),wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);
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
	}

    // whm 4Feb2020 modification notes: 
    // Since no project is actually open when this dialog is closed the collaboration
    // settings, all the App's collab settings are irrelevant until an AI project
    // is selected from the ProjectPage's list in the StartWorking Wizard. 
    // Hence, restoring the App's collab values from the m_bSave... and m_Save... 
    // values below should not be needed.

    /*
    // The following code block of assignments from the save-values should be
    // identical to the corresponding code block of save-values in OnCancel().
    // Restore the App's original collab values before exiting the dialog
    // Use the m_Save... and m_bSave... values that were saved in InitDialog().
	m_pApp->m_bCollaboratingWithParatext = m_bSaveCollaboratingWithParatext;
	m_pApp->m_bCollaboratingWithBibledit = m_bSaveCollaboratingWithBibledit;
	m_pApp->m_CollabProjectForSourceInputs = m_SaveCollabProjectForSourceInputs;
	m_pApp->m_CollabProjectForTargetExports = m_SaveCollabProjectForTargetExports;
	m_pApp->m_CollabProjectForFreeTransExports = m_SaveCollabProjectForFreeTransExports;
	m_pApp->m_CollabAIProjectName = m_SaveCollabAIProjectName;
	m_pApp->m_collaborationEditor = m_SaveCollaborationEditor;
    m_pApp->m_ParatextVersionForProject = m_SaveCollabEditorVersion; // whm added 24June2014
	m_pApp->m_CollabSourceLangName = m_SaveCollabSourceProjLangName;
	m_pApp->m_CollabTargetLangName = m_SaveCollabTargetProjLangName;
	m_pApp->m_bCollabByChapterOnly = m_bSaveCollabByChapterOnly; // FALSE means the "whole book" option
	m_pApp->m_bCollaborationExpectsFreeTrans = m_bSaveCollaborationExpectsFreeTrans;
	m_pApp->m_CollabBookSelected = m_SaveCollabBookSelected;
	m_pApp->m_CollabChapterSelected = m_SaveCollabChapterSelected;
    m_pApp->m_CollabBooksProtectedFromSavingToEditor = m_SaveCollabBooksProtectedFromSavingToEditor; // whm added 2February2017
    m_pApp->m_bCollabDoNotShowMigrationDialogForPT7toPT8 = m_SaveCollabDoNotShowMigrationDialogForPT7toPT8; // whm added 6April2017
    */

    // whm modified 30Dec2016 The App's m_collaborationEditor and m_ParatextVersionForProject
    // could be empty strings.
    //wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
    //wxASSERT(!m_pApp->m_ParatextVersionForProject.IsEmpty());

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

void CSetupEditorCollaboration::OnCancel(wxCommandEvent& event)
{
    // The following code block of assignments from the save-values should be
    // identical to the corresponding code block of save-values in OnClose().
	// Restore the App's original collab values before exiting the dialog
    // Use the m_Save... and m_bSave... values that were saved in InitDialog().
    m_pApp->m_bCollaboratingWithParatext = m_bSaveCollaboratingWithParatext;
	m_pApp->m_bCollaboratingWithBibledit = m_bSaveCollaboratingWithBibledit;
	m_pApp->m_CollabProjectForSourceInputs = m_SaveCollabProjectForSourceInputs;
	m_pApp->m_CollabProjectForTargetExports = m_SaveCollabProjectForTargetExports;
	m_pApp->m_CollabProjectForFreeTransExports = m_SaveCollabProjectForFreeTransExports;
	m_pApp->m_CollabAIProjectName = m_SaveCollabAIProjectName;
	m_pApp->m_collaborationEditor = m_SaveCollaborationEditor;
    m_pApp->m_ParatextVersionForProject = m_SaveCollabEditorVersion; // whm added 24June2016
	m_pApp->m_CollabSourceLangName = m_SaveCollabSourceProjLangName;
	m_pApp->m_CollabTargetLangName = m_SaveCollabTargetProjLangName;
	m_pApp->m_bCollabByChapterOnly = m_bSaveCollabByChapterOnly; // FALSE means the "whole book" option
	m_pApp->m_bCollaborationExpectsFreeTrans = m_bSaveCollaborationExpectsFreeTrans;
	m_pApp->m_CollabBookSelected = m_SaveCollabBookSelected;
	m_pApp->m_CollabChapterSelected = m_SaveCollabChapterSelected;
    m_pApp->m_CollabBooksProtectedFromSavingToEditor = m_SaveCollabBooksProtectedFromSavingToEditor; // whm added 2February2017
    m_pApp->m_bCollabDoNotShowMigrationDialogForPT7toPT8 = m_SaveCollabDoNotShowMigrationDialogForPT7toPT8; // whm added 6April2017

	wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
    wxASSERT(!m_pApp->m_ParatextVersionForProject.IsEmpty()); // whm added 24June2016

	event.Skip();
}

void CSetupEditorCollaboration::OnCreateNewAIProject(wxCommandEvent& WXUNUSED(event)) // whm added 23Feb12
{
    if (gbPTNotInstalled && !m_pApp->BibleditIsInstalled())
    {
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_("Neither Paratext nor Bibledit are installed on this computer. Paratext or Bibledit must be installed before Adapt It can collaborate with it."), _("Cannot create a new project for collaboration"), wxICON_EXCLAMATION | wxOK);
        return;
    }

    CCreateNewAIProjForCollab newProjDlg(m_pApp->GetMainFrame());

	if (newProjDlg.ShowModal() == wxID_OK)
	{
		// Note: The newProjDlg's OnOK() handler check for empty strings

		// Initialize the collaboration values to their defaults. We need to use the App's
		// collaboration values here because the call of CreateNewAIProject() below calls
		// WriteConfigurationFile() which writes out the current App's project config file
		// values. We could initialize the m_CollabSrcProjLangName
		m_pApp->m_bCollaboratingWithParatext = FALSE;
		m_pApp->m_bCollaboratingWithBibledit = FALSE;
		m_pApp->m_CollabProjectForSourceInputs = _T("");
		m_pApp->m_CollabProjectForTargetExports = _T("");
		m_pApp->m_CollabProjectForFreeTransExports = _T("");
		m_pApp->m_CollabAIProjectName = _T("");

		// whm 4Jun12 Note: The m_TempCollaborationEditor value should be used for setting
		// the App's m_collaborationEditor member.
		m_pApp->m_collaborationEditor = m_TempCollaborationEditor; // selected editor
        // whm 24June2016 Note: The m_TempCollabEditorVersion value should be used for setting
        // the App's m_collaborationEditor member.
        m_pApp->m_ParatextVersionForProject = m_TempCollabEditorVersion; // selected editor version

		m_pApp->m_CollabSourceLangName = _T("");
		m_pApp->m_CollabTargetLangName = _T("");
		m_pApp->m_bCollabByChapterOnly = TRUE; // FALSE means the "whole book" option
		m_pApp->m_bCollaborationExpectsFreeTrans = FALSE;
		m_pApp->m_CollabBookSelected = _T("");
		m_pApp->m_CollabChapterSelected = _T("");
        m_pApp->m_CollabBooksProtectedFromSavingToEditor = _T(""); // whm added 2February2017
        m_pApp->m_bCollabDoNotShowMigrationDialogForPT7toPT8 = FALSE; // whm added 6April2017

		bool bDisableBookMode = TRUE;
		wxString srcLangName = newProjDlg.pTextCtrlSrcLangName->GetValue();
		wxString tgtLangName = newProjDlg.pTextCtrlTgtLangName->GetValue();
		wxString srcLangCode = newProjDlg.pTextCtrlSrcLangCode->GetValue();
		wxString tgtLangCode = newProjDlg.pTextCtrlTgtLangCode->GetValue();
		// Use the newProjDlg dialog's values for source and target lang names and codes
		// for the new AI project (which will have default collab values
		bool bProjOK = CreateNewAIProject(m_pApp, srcLangName, tgtLangName,
			srcLangCode, tgtLangCode, bDisableBookMode);
		if (!bProjOK)
		{
            // This is a fatal error to continuing this collaboration attempt, but it won't
            // hurt the application. The error shouldn't ever happen. We let the dialog
            // continue to run, but tell the user that the app is probably unstable now and
            // he should get rid of any folders created in the attempt, shut down,
            // relaunch, and try again. This message is localizable.
			wxString message;
			message = message.Format(_("Error: attempting to create an Adapt It project for supporting collaboration with an external editor, failed.\nThe application is not in a state suitable for you to continue working, but it will still run. You should now Cancel and then shut it down.\nThen (using a File Browser application) you should also manually delete this folder and its contents: %s  if it exists.\nThen relaunch, and try again."),
				m_TempCollaborationEditor.c_str());
			m_pApp->LogUserAction(message);
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(message,_("Project Not Created"), wxICON_ERROR | wxOK);
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

			// Update our m_Temp... variables with the source and target lang names and derived
			// AI proj name for use in setting controls below. Then later, after the call to
			// DoSetControlsFromConfigFileCollabData() - which reads the default values into
			// the three Temp collab variables, we'll override them again with the newProjDlg's
			// values.
			this->m_TempCollabSourceProjLangName = newProjDlg.pTextCtrlSrcLangName->GetValue();
			this->m_TempCollabTargetProjLangName = newProjDlg.pTextCtrlTgtLangName->GetValue();
			this->m_TempCollabAIProjectName = newProjDlg.pTextCtrlNewAIProjName->GetValue(); // this read-only wxTestCtrl composes the AI project name on the fly

			// add the new project to the combo box list of projects
			int nNewProjIndex;
			nNewProjIndex = pComboAiProjects->Append(m_TempCollabAIProjectName);
			wxASSERT(nNewProjIndex >= 0);
			pComboAiProjects->SetSelection(nNewProjIndex);
			// Select the new AI project's name in the pComboAiProjects combobox.
			wxASSERT(pComboAiProjects->GetStringSelection() == m_TempCollabAIProjectName);
			wxASSERT(!m_TempCollabSourceProjLangName.IsEmpty() && !m_TempCollabTargetProjLangName.IsEmpty());
			wxASSERT(!m_TempCollaborationEditor.IsEmpty());
            wxASSERT(!m_TempCollabEditorVersion.IsEmpty()); // whm 24June2016 added
			SetStateOfRemovalButton();
            SetStateOfAcceptSetupButton(); // enables the "Accept this setup and prepare for another" button if
                                           // all of: project name, editor, source text, or target text project names
                                           // have non-empty strings
            // confirm to the user that the project was created
			wxString msg = _("An Adapt It project called \"%s\" was successfully created. It will appear as an Adapt It project in the \"Select a Project\" list of the Start Working Wizard.\n\nContinue through steps 2 through 4 below to set up this Adapt It project to collaborate with %s.");
			msg = msg.Format(msg,m_pApp->m_curProjectName.c_str(), m_TempCollaborationEditor.c_str());
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg,_("New Adapt It project created"),wxICON_INFORMATION | wxOK);
			DoSetControlsFromConfigFileCollabData(TRUE); // Sets all Temp collab values as read from project config file TRUE = we're creating a new project
			 // Override the AI Proj Name related Temp values with the new AI project's name (from above)
			this->m_TempCollabSourceProjLangName = newProjDlg.pTextCtrlSrcLangName->GetValue();
			this->m_TempCollabTargetProjLangName = newProjDlg.pTextCtrlTgtLangName->GetValue();
			this->m_TempCollabAIProjectName = newProjDlg.pTextCtrlNewAIProjName->GetValue(); // this read-only wxTestCtrl composes the AI project name on the fly
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
    if (gbPTNotInstalled && !m_pApp->BibleditIsInstalled())
    {
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_("Neither Paratext nor Bibledit are installed on this computer. Paratext or Bibledit must be installed before Adapt It can collaborate with it."), _("Cannot save collaboration setup. "), wxICON_EXCLAMATION | wxOK);
        return FALSE;
    }

	// Check for completion of Step 1:
	// Check if the administrator has selected an AI project from the combo list of AI projects (step 1). If
	// not inform him that step 1 is necessary to identify an AI project for hookup to during collaboration
	// so that AI can save the collaboration settings for that project.
	if (pComboAiProjects->GetSelection() == -1 || m_TempCollabAIProjectName.IsEmpty())
	{
		wxString msg, msg1, msgTitle;
		msg1 = _("Please select an existing Adapt It project or create a new one in step 1.");
		msg = msg1;
		msg += _T(' ');
		msg += _("Adapt It will save the collaboration settings you make in this dialog for the Adapt It project you select (or create) in step 1.");
		msgTitle = _("No Adapt It project designated for this collaboration setup");
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(msg,msgTitle,wxICON_INFORMATION | wxOK);
		// set focus on the combo box
		pComboAiProjects->SetFocus();
		m_pApp->LogUserAction(msgTitle);
		return FALSE; // don't accept any changes - return FALSE to the caller
	}

    // Note: Step 2 selection of Scripture Editor is initially set somewhat automatically according to
    // what version of a Scripture editors has been installed on the user's computer. Step 2 allows an
    // administrator to optionally set a different Scripture editor if it is installed on the user's
    // computer (selections are disabled for any editor that is not installed).
    // For the automatic selection of editor, (or if an editor choice is installed and manually selected
    // by the administrator), the list box displays all the available/usable projects for that particular
    // editor that is selected at left.
    // Here we just save whatever selection is made automatically, or is overridden by an administrator.

	// Check for the completion of step 3:
	// Check if the administrator has selected an initial PT/BE project for obtaining source texts.
	if (m_TempCollabProjectForSourceInputs.IsEmpty())
	{
		wxString msg, msg1;
		msg = _("Please select a %s project for getting source texts. Use the \"Select Source Project from List\" button.");
		wxASSERT(!m_TempCollaborationEditor.IsEmpty());
		msg = msg.Format(msg,m_TempCollaborationEditor.c_str());
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(msg,_("No source language project selected for collaboration"),wxICON_INFORMATION | wxOK);
		pBtnSelectFmListSourceProj->SetFocus();
		m_pApp->LogUserAction(msg);
		return FALSE; // don't accept any changes - return FALSE to the caller
	}
	// Check if the administrator has selected an initial PT/BE project for receiving translation drafts.
	if (m_TempCollabProjectForTargetExports.IsEmpty())
	{
		wxString msg, msg1;
		msg = _("Please select a %s project for receiving translated drafts. Use the \"Select Target Project from List\" button.");
		wxASSERT(!m_TempCollaborationEditor.IsEmpty());
		msg = msg.Format(msg,m_TempCollaborationEditor.c_str());
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(msg,_("No target language project selected for collaboration"),wxICON_INFORMATION | wxOK);
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
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg);
			m_pApp->LogUserAction(msg);
			return FALSE; // don't accept any changes - return FALSE to the caller
		}

		if (m_TempCollabProjectForSourceInputs == m_TempCollabProjectForFreeTransExports
			&& m_bTempCollaborationExpectsFreeTrans)
		{
			wxString msg;
			msg = _("The projects selected for getting source texts and receiving free translation texts cannot be the same.\nPlease select one project for getting source texts, and a different project for receiving free translation texts.");
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            m_pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg);
			m_pApp->LogUserAction(msg);
			return FALSE; // don't accept any changes - return FALSE to the caller
		}
	}

	wxASSERT(!m_TempCollabAIProjectName.IsEmpty());
	GetAILangNamesFromAIProjectNames(m_TempCollabAIProjectName, m_TempCollabSourceProjLangName,m_TempCollabTargetProjLangName);
	wxASSERT(!m_TempCollabSourceProjLangName.IsEmpty() && !m_TempCollabTargetProjLangName.IsEmpty());

	// Check to see if book folder mode is activated. If so, warn the administrator that it
	// will be turned OFF if s/he continues setting up AI for collaboration with Paratext/Bibledit.
	if (m_pApp->m_bBookMode)
	{
		wxString strSel = pComboAiProjects->GetStringSelection();
		wxString msg;
		wxASSERT(!m_TempCollaborationEditor.IsEmpty());
		msg = msg.Format(_("Note: Book Folder Mode is currently in effect, but it must be turned OFF and disabled for the %s Adapt It project in order for Adapt It to collaborate with %s. If you continue, Book Folder Mode will be turned off and disabled.\n\nDo you want to continue setting up this project for collaboration with %s?"),
			strSel.c_str(), m_TempCollaborationEditor.c_str(),m_TempCollaborationEditor.c_str());
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        if (wxMessageBox(msg,_T(""),wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT) == wxNO)
		{
			m_pApp->LogUserAction(msg);
			return FALSE; // don't accept any changes - return FALSE to the caller
		}
	}

	wxString selStr = pComboAiProjects->GetStringSelection();
	wxString errorStr = _T("");
	wxString errProj = _T("");
	// CollabProjectsAreValid() doesn't consider any empty string projects to be invalid, only those non-empty
	// string parameters that don't have at least one book created in them. In this situation, the code
	// above has already ensured that at least the m_TempCollabProjectForSourceInputs and m_TempCollabProjectForTargetExports
	// strings are non-empty.
    // whm 27June2016 revision: Pass m_TempCollabEditorVersion into the CollabProjectsAreValid() function
    // below, and/or do any additional checks within that function for PT 7 and PT 8 validity checking.
	if (!CollabProjectsAreValid(m_TempCollabProjectForSourceInputs, m_TempCollabProjectForTargetExports,
		m_TempCollabProjectForFreeTransExports, m_TempCollaborationEditor, m_TempCollabEditorVersion,
        errorStr, errProj))
	{
		wxString msg = _("The following %s projects are not valid for collaboration:\n%s");
		wxASSERT(!m_TempCollaborationEditor.IsEmpty());
		msg = msg.Format(msg,m_TempCollaborationEditor.c_str(),errorStr.c_str());
		// Note: The errProj returned string is not used here.
		msg += _T("\n\n");
		wxString msg1 = _("This setup cannot be accepted for collaboration. Please set up valid %s project(s) that Adapt It can use for collaboration.");
		msg1 = msg1.Format(msg1,m_TempCollaborationEditor.c_str());
		msg += msg1;
		wxString titleMsg = _("Invalid %s projects detected");
		titleMsg = titleMsg.Format(titleMsg,m_TempCollaborationEditor.c_str());
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(msg,titleMsg,wxICON_EXCLAMATION | wxOK);
		m_pApp->LogUserAction(titleMsg);
		return FALSE; // don't accept any changes - return FALSE to the caller
	}

    // whm 4Feb2020 added following block to get the Scripture editor selection from the
    // radio box control and set the values for the m_pApp->m_bCollaboratingWithParatext
    // and m_pApp->m_bCollaboratingWithBibledit members
    int selRadioBox = pRadioBoxScriptureEditor->GetSelection();
    if (selRadioBox == 0 || selRadioBox == 1 || selRadioBox == 2)
    {
        m_bTempCollaboratingWithParatext = TRUE;
        m_bTempCollaboratingWithBibledit = FALSE;
    }
    else if (selRadioBox == 3)
    {
        m_bTempCollaboratingWithParatext = FALSE;
        m_bTempCollaboratingWithBibledit = TRUE;
    }

	// Store collab settings in the App's Collab members
    // whm 4Feb2020 modified: Previously the App's m_bCollaboratingWithParatext and
    // m_bCollaboratingWithBibledit members were unilaterally set to FALSE below,
    // but makes the ChooseCollabOptions dialog's radio buttons to automatically select the
    // middle Collaboration OFF button for the first time the project gets opened after setting it
    // up for collaboration. I think a user would expect to have the collaboration auto-selected to ON
    // when first opening the project that was just set up for collaboration, without having to deliberately
    // set the TOP button on that dialog. This change also required that the OnClose() handler not
    // restore the App's collab values back to what they were upon opening this dialog.
    // So, I've changed the code below to use the m_bTempCollaboratingWithParatext and m_bTempCollaboratingWith Bibledit
    // values as currently indicated in the pRadioBoxScriptureEditor detected above.
	//m_pApp->m_bCollaboratingWithParatext = FALSE; // Start with this project's newly set collaboration set to OFF
	//m_pApp->m_bCollaboratingWithBibledit = FALSE; // Start with this project's newly set collaboration set to OFF
    m_pApp->m_bCollaboratingWithParatext = m_bTempCollaboratingWithParatext; // Start with this project's collaboration set to the indicated radio button value
    m_pApp->m_bCollaboratingWithBibledit = m_bTempCollaboratingWithBibledit; // Start with this project's collaboration set to the indicated radio button value
	m_pApp->m_CollabProjectForSourceInputs = m_TempCollabProjectForSourceInputs;
	m_pApp->m_CollabProjectForTargetExports = m_TempCollabProjectForTargetExports;
	m_pApp->m_CollabProjectForFreeTransExports = m_TempCollabProjectForFreeTransExports;
	m_pApp->m_CollabAIProjectName = m_TempCollabAIProjectName;
	m_pApp->m_collaborationEditor = m_TempCollaborationEditor;
    m_pApp->m_ParatextVersionForProject = m_TempCollabEditorVersion; // whm added 25June2016
	m_pApp->m_bCollaborationExpectsFreeTrans = m_bTempCollaborationExpectsFreeTrans;
	m_pApp->m_CollabBookSelected = m_TempCollabBookSelected;
	m_pApp->m_CollabSourceLangName = m_TempCollabSourceProjLangName;
	m_pApp->m_CollabTargetLangName = m_TempCollabTargetProjLangName;
	m_pApp->m_bCollabByChapterOnly = m_bTempCollabByChapterOnly;
	m_pApp->m_CollabChapterSelected = m_TempCollabChapterSelected;
    m_pApp->m_CollabBooksProtectedFromSavingToEditor = m_TempCollabBooksProtectedFromSavingToEditor; // whm added 2February2017
    m_pApp->m_bCollabDoNotShowMigrationDialogForPT7toPT8 = m_TempCollabDoNotShowMigrationDialogForPT7toPT8; // whm added 6April2017

	wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
    wxASSERT(!m_pApp->m_ParatextVersionForProject.IsEmpty());

	// In order to write the collab settings to the current Adapt It project we need to compose the
	// path to the project folder for the second parameter of WriteConfigurationFile().
	wxString newProjectPath;
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
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(msg,_T("Save of collaboration settings successful"),wxICON_INFORMATION | wxOK);
		m_pApp->LogUserAction(msg);
		m_bCollabChangedThisDlgSession = FALSE;

		// whm 25Oct13 added. This SaveAppCollabSettingsToINIFile() needs to be called
		// at every point that significant collaboration settings change to keep the
		// Adapt_It_WX.ini file up to date.
		m_pApp->SaveAppCollabSettingsToINIFile(newProjectPath);
	}
	else
	{
		// Writing of the project config file failed for some reason. This would be unusual, so
		// just do an English notification
		wxString msg = _T("Error writing the project configuration file. Make sure it is not being used by another program and then try again.");
		wxMessageBox(msg);
		m_pApp->LogUserAction(msg);
		return FALSE;
	}


	m_TempCollabProjectForSourceInputs = _T("");
	m_TempCollabProjectForTargetExports = _T("");
	m_TempCollabProjectForFreeTransExports = _T("");
	m_TempCollabAIProjectName = _T("");
    // m_TempCollaborationEditor can stay set to its current value
    // m_TempCollabEditorVersion can stay set to its current value
	m_bTempCollaborationExpectsFreeTrans = FALSE; // defaults to FALSE for no free trans
	m_TempCollabBookSelected = _T("");
	m_TempCollabSourceProjLangName = _T("");
	m_TempCollabTargetProjLangName = _T("");
	m_bTempCollabByChapterOnly = TRUE; // defaults to TRUE for collab by chapter only
	m_TempCollabChapterSelected = _T("");
    m_TempCollabBooksProtectedFromSavingToEditor = _T(""); // whm added 2February2017
    m_TempCollabDoNotShowMigrationDialogForPT7toPT8 = FALSE; // whm added 6April2017

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

// whm 25June2016 revised for PT 8 compatibility.
void CSetupEditorCollaboration::OnRemoveThisAIProjectFromCollab(wxCommandEvent& WXUNUSED(event)) // whm added 23Feb12
{
	// "Remove this AI Project from Collaboration"
	// If no AI project is yet selected this handler is disabled, so we can assume that
	// if we get here an AI project will have been selected.
	wxString projName = m_TempCollabAIProjectName;
	wxString msg = _("You are about to remove the collaboration settings for the %s project. If you continue the user will not be able to turn on collaboration with %s for this project.\n\nDo you want to remove the collaboration settings for %s?");
	msg = msg.Format(msg, projName.c_str(),m_TempCollaborationEditor.c_str(),projName.c_str());
    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
    m_pApp->m_bUserDlgOrMessageRequested = TRUE;
    int response = wxMessageBox(msg,_("Confirm removal of collaboration settings"),wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT);
	if (response == wxNO)
		return;

	// This is similar to the save routine that is found in the OnSaveSetupForThisProjNow() handler,
	// except that we are saving blank values for the collab settings into the project config
	// file.
	m_pApp->m_bCollaboratingWithParatext = FALSE;
	m_pApp->m_bCollaboratingWithBibledit = FALSE;
	m_pApp->m_CollabProjectForSourceInputs = _T("");
	m_pApp->m_CollabProjectForTargetExports = _T("");
	m_pApp->m_CollabProjectForFreeTransExports = _T("");
	m_pApp->m_CollabAIProjectName = _T("");
	// m_pApp->m_collaborationEditor can stay set to its current value
    // m_pApp->m_ParatextVersionForProject can stay set to its current value - whm added 25June2016
	m_pApp->m_bCollaborationExpectsFreeTrans = FALSE;
	m_pApp->m_CollabBookSelected = _T("");
	m_pApp->m_CollabSourceLangName = _T("");
	m_pApp->m_CollabTargetLangName = _T("");
	m_pApp->m_bCollabByChapterOnly = TRUE;
	m_pApp->m_CollabChapterSelected = _T("");
    m_pApp->m_CollabBooksProtectedFromSavingToEditor = _T(""); // whm added 2February2017
    m_pApp->m_bCollabDoNotShowMigrationDialogForPT7toPT8 = FALSE; // whm added 6April2017

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
		//wxString msg = _("The collaboration settings for the \"%s\" project were successfully removed from the project's configuration file at:\n\n%s\n\nYou may now select or create another Adapt It project (step 1) and make collaboration settings for that Adapt It project (setps 2 - 4).\n\nIf you are finished, select \"Close\" to close the setup dialog and test your setup(s) using the Start Working Wizard.");
        wxString msg0 = _("The collaboration settings for the \"%s\" project were successfully removed from the project's configuration file at:\n\n%s\n\n");
        wxString msg1 = _("You may now select or create another Adapt It project (step 1) and make collaboration settings for that Adapt It project (setps 2 - 4).\n\n");
        wxString msg2 = _("If you are finished, select \"Close\" to close the setup dialog and test your setup(s) using the Start Working Wizard.");
        wxString msg;
        msg = msg0 + msg1 + msg2;
        msg = msg.Format(msg,m_TempCollabAIProjectName.c_str(),newAIconfigFilePath.c_str());
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(msg,_T("Removal of collaboration settings successful"),wxICON_INFORMATION | wxOK);
		m_bCollabChangedThisDlgSession = FALSE;

		// whm 25Oct13 added. This SaveAppCollabSettingsToINIFile() needs to be called
		// at every point that significant collaboration settings change to keep the
		// Adapt_It_WX.ini file up to date. This does not remove the settings
		// entirely from the ini file, it clears the existing settings to
		// remove active collaboration.
		m_pApp->SaveAppCollabSettingsToINIFile(newProjectPath);
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
    // m_TempCollabEditorVersion can stay set to its current value - whm 25June2016 added
	m_bTempCollaborationExpectsFreeTrans = FALSE; // defaults to FALSE for no free trans
	m_TempCollabBookSelected = _T("");
	m_TempCollabSourceProjLangName = _T("");
	m_TempCollabTargetProjLangName = _T("");
	m_bTempCollabByChapterOnly = TRUE; // defaults to TRUE for collab by chapter only
	m_TempCollabChapterSelected = _T("");
    m_TempCollabBooksProtectedFromSavingToEditor = _T(""); // whm added 2February2017
    m_TempCollabDoNotShowMigrationDialogForPT7toPT8 = FALSE; // whm added 6April2017

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
    // the only condition for removal of a project from collaboration is that an AI project name
    // must be selected
    if (pComboAiProjects->GetSelection() == -1)
        pBtnRemoveProjFromCollab->Disable();
    else
        pBtnRemoveProjFromCollab->Enable();
}

void CSetupEditorCollaboration::SetStateOfAcceptSetupButton()
{
    // Disable "Accept this setup and prepare for another" button unless minimum settings are established.
    // We enable the "Accept this setup and prepare for another" button only if/when these
    // settings are established within the dialog's controls: project name, editor, source text name, and target text project name.
    // whm modified 18March2017 removed disabling if FreeTrans edit box is empty. It is only an optional proj for collaboration!
    if (pComboAiProjects->GetSelection() == -1
        || pTextCtrlAsStaticSelectedSourceProj->GetValue().IsEmpty()
        || pTextCtrlAsStaticSelectedTargetProj->GetValue().IsEmpty())
        pBtnSaveSetupForThisProjNow->Disable();
    else
        pBtnSaveSetupForThisProjNow->Enable();
}

void CSetupEditorCollaboration::SetPTorBEsubStringsInControls()
{
	// adjust static strings substituting "Paratext" or "Bibledit" depending on
	// the string value in m_collaborationEditor
	// Substitute string in the "Use this dialog..." top static text
	wxString text;
	text = pStaticTextDialogTopicLine->GetLabel();
	// whm Note: The control's label text may have already been substituted in which
	// case there would be no $s to use in the .Format() statement. If substitutions
	// have already been done, just find and replace the "Paratext" or "Bibledit"
	// substring with the current value for m_TempCollaborationEditor.
	if (text.Find(_T("Paratext")) != wxNOT_FOUND)
		text.Replace(_T("Paratext"),m_TempCollaborationEditor);
	if (text.Find(_T("Bibledit")) != wxNOT_FOUND)
		text.Replace(_T("Bibledit"),m_TempCollaborationEditor);
	text = text.Format(text,m_TempCollaborationEditor.c_str());
	pStaticTextDialogTopicLine->SetLabel(text);
	pStaticTextDialogTopicLine->Refresh();

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
    // Clicking the "Paratext 7" button returns nSel = 0; clicking the "Paratext 8" button returns nSel 1;
    // clicking the "Paratext 9" button returns nSel = 2; clicking the "Bibledit" button returns nSel = 3
	unsigned int nSelNew = pRadioBoxScriptureEditor->GetSelection();
	if (nSelNew == 0)
	{
		// "Paratext 7" button selected
		m_TempCollaborationEditor = _T("Paratext");
        m_TempCollabEditorVersion = _T("PTVersion7");
	}
    else if (nSelNew == 1)
    {
        // "Paratext 8" button selected
        m_TempCollaborationEditor = _T("Paratext");
        m_TempCollabEditorVersion = _T("PTVersion8");
    }
    else if (nSelNew == 2)
    {
        // "Paratext 9" button selected
        m_TempCollaborationEditor = _T("Paratext");
        m_TempCollabEditorVersion = _T("PTVersion9");
    }
    else
    {
		// "Bibledit" button selected
		m_TempCollaborationEditor = _T("Bibledit");
        // for a Bibledit selection the m_TempCollabEditorVersion is ignored
    }

    if (m_TempCollabEditorVersion == _T("PTVersion7"))
    {
        // Warn the administrator that Paratext 7 is no longer supported
        // and that, while AI can still collaborate with PT7, the adaptations
        // sent to a PT7 project can no longer be stored remotely using its
        // Send/Receive feature. If you want to collaborate with PT7 it is at
        // own risk!
        wxString msg = _("WARNING: Paratext 7 is no longer supported.\n\nWhile Adapt It can still be configured to collatorate with Paratext 7, the adaptations sent to a Paratext 7 project can no longer be stored remotely using its Send/Receive feature.\n\nDo you really want to set up this project to collaborate with Paratext 7?");
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        m_pApp->m_bUserDlgOrMessageRequested = TRUE;
        if (wxMessageBox(msg, _("Important Warning"), wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT) == wxYES)
        {
            ; // Call DoInit() below and allow the process to continue with Paratext 7 radio button selected
        }
        else // if response is wxNO
        {
            // only wizard has event.Veto(), so we use a different means to
            // restore the previous selection using the class member m_nPreviousSel.
            if (m_nPreviousSel != nSelNew)
                pRadioBoxScriptureEditor->SetSelection(m_nPreviousSel);
            return;
        }
    }
	DoInit(TRUE); // TRUE = prompt reminder to use Select from List buttons
}

// whm 4Feb2020 added the following function to consolidate 2 places in code
// That set the selection for the pRadioBoxScriptureEditor control.
// This function uses the m_TempCollaborationEditor and m_TempCollabEditorVersion temp variables
// (assigned values above) to set an appropriate radio button for the radiobox in the dialog.
// This function is called from: CSetupEditorCollaboration::DoSetControlsFromConfigFileCollabData()
// and CSetupEditorCollaboration::DoInit().
void CSetupEditorCollaboration::SetRadioBoxCollabEditorSelection()
{
    if (m_TempCollaborationEditor == _T("Paratext"))
    {
        if (gbPTVer7Installed || gbPTVer8Installed || gbPTVer9Installed || gbPTLinuxVer7Installed || gbPTLinuxVer8Installed || gbPTLinuxVer9Installed)
        {
            // Only select a PT version radio button if that version of Paratext is installed
            if ((m_TempCollabEditorVersion == _T("PTVersion7") && gbPTVer7Installed) || (m_TempCollabEditorVersion == _T("PTLinuxVersion7") && gbPTLinuxVer7Installed))
            {
                pRadioBoxScriptureEditor->SetSelection(0); // radiobox index 0 is "Paratext 7"
            }
            else if ((m_TempCollabEditorVersion == _T("PTVersion8") && gbPTVer8Installed) || (m_TempCollabEditorVersion == _T("PTLinuxVersion8") && gbPTLinuxVer8Installed))
            {
                pRadioBoxScriptureEditor->SetSelection(1); // radiobox index 1 is "Paratext 8"
            }
            // whm 4Feb2020 added test below for PTVersion9 and PTLinuxVersion9
            else if ((m_TempCollabEditorVersion == _T("PTVersion9") && gbPTVer9Installed) || (m_TempCollabEditorVersion == _T("PTLinuxVersion9") && gbPTLinuxVer9Installed))
            {
                pRadioBoxScriptureEditor->SetSelection(2); // radiobox index 2 is "Paratext 9"
            }
        }
        else if (m_pApp->BibleditIsInstalled())
        {
            pRadioBoxScriptureEditor->SetSelection(3);
        }
    }
    else if (m_TempCollaborationEditor == _T("Bibledit") && m_pApp->BibleditIsInstalled())
    {
        pRadioBoxScriptureEditor->SetSelection(3); // radiobox index 3 is "Bibledit"
    }
}
