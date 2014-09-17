/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GetSourceTextFromEditorDlg.cpp
/// \author			Bill Martin
/// \date_created	10 April 2011
/// \rcs_id $Id$
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for two friend classes: the CGetSourceTextFromEditorDlg and
/// the CChangeCollabProjectsDlg class. 
/// The CGetSourceTextFromEditorDlg class represents a dialog in which a user can obtain a source text
/// for adaptation from an external editor such as Paratext or Bibledit.
/// \derivation		The CGetSourceTextFromEditorDlg is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in GetSourceTextFromEditorDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "GetSourceTextFromEditorDlg.h"
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
#include <wx/tokenzr.h>
#include <wx/listctrl.h>
#include <wx/progdlg.h>

#include "Adapt_It.h"
#include "MainFrm.h"
#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"
#include "Pile.h"
#include "Layout.h"
#include "helpers.h"
#include "CollabUtilities.h"
#include "WaitDlg.h"
#include "GetSourceTextFromEditor.h"
#include "StatusBar.h"

extern wxChar gSFescapechar; // the escape char used for start of a standard format marker
extern bool gbIsGlossing;
extern bool gbGlossingUsesNavFont;
extern int gnOldSequNum;
extern CAdapt_ItApp* gpApp;
extern wxString szProjectConfiguration;

extern wxString m_collab_bareChapterSelectedStr; // defined in CollabUtilities.cpp

// event handler table
BEGIN_EVENT_TABLE(CGetSourceTextFromEditorDlg, AIModalDialog)
	EVT_INIT_DIALOG(CGetSourceTextFromEditorDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CGetSourceTextFromEditorDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, CGetSourceTextFromEditorDlg::OnCancel)
	EVT_LISTBOX(ID_LISTBOX_BOOK_NAMES, CGetSourceTextFromEditorDlg::OnLBBookSelected)
	EVT_LIST_ITEM_SELECTED(ID_LISTCTRL_CHAPTER_NUMBER_AND_STATUS, CGetSourceTextFromEditorDlg::OnLBChapterSelected)
	EVT_LISTBOX_DCLICK(ID_LISTCTRL_CHAPTER_NUMBER_AND_STATUS, CGetSourceTextFromEditorDlg::OnLBDblClickChapterSelected)
END_EVENT_TABLE()

CGetSourceTextFromEditorDlg::CGetSourceTextFromEditorDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Get Source Text from %s Project"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	// 
	// whm 10Sep11 Note: Normally the second parameter in the call of the wxDesigner function
	// below is true - which calls Fit() to make the dialog window itself size to fit the size of
	// all controls in the dialog. But in the case of this dialog, we don't call Fit() so we can
	// programmatically set the size of the dialog to be shorter when the project selection
	// controls are hidden. That is done in InitDialog().
	// whm 17Nov11 redesigned the CGetSourceTextFromEditorDlg, splitting it into two dialogs, so
	// that the project change options are now contained in the friend class CChangeCollabProjectsDlg.
	pGetSourceTextFromEditorSizer = GetSourceTextFromEditorDlgFunc(this, FALSE, TRUE); // second param FALSE enables resize
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	wxColour sysColorBtnFace; // color used for read-only text controls displaying
	// color used for read-only text controls displaying static text info button face color
	sysColorBtnFace = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(m_pApp != NULL);
	
	pListBoxBookNames = (wxListBox*)FindWindowById(ID_LISTBOX_BOOK_NAMES);
	wxASSERT(pListBoxBookNames != NULL);

	pListCtrlChapterNumberAndStatus = (wxListView*)FindWindowById(ID_LISTCTRL_CHAPTER_NUMBER_AND_STATUS);
	wxASSERT(pListCtrlChapterNumberAndStatus != NULL);

	pStaticTextCtrlNote = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_NOTE);
	wxASSERT(pStaticTextCtrlNote != NULL);
	pStaticTextCtrlNote->SetBackgroundColour(sysColorBtnFace);

	pStaticSelectAChapter = (wxStaticText*)FindWindowById(ID_TEXT_SELECT_A_CHAPTER);
	wxASSERT(pStaticSelectAChapter != NULL);

	pBtnCancel = (wxButton*)FindWindowById(wxID_CANCEL);
	wxASSERT(pBtnCancel != NULL);

	pBtnOK = (wxButton*)FindWindowById(wxID_OK);
	wxASSERT(pBtnOK != NULL);

	pSrcProj = (wxStaticText*)FindWindowById(ID_STATIC_TEXT_SRC_PROJ);
	wxASSERT(pSrcProj != NULL);
	
	pTgtProj = (wxStaticText*)FindWindowById(ID_STATIC_TEXT_TGT_PROJ);
	wxASSERT(pTgtProj != NULL);
	
	pFreeTransProj = (wxStaticText*)FindWindowById(ID_STATIC_TEXT_FREETRANS_PROJ);
	wxASSERT(pFreeTransProj != NULL);

	// Note: STATIC_TEXT_DESCRIPTION is a pointer to a wxStaticBoxSizer which wxDesigner casts back
	// to the more generic wxSizer*, so we'll use a wxDynamicCast() to cast it back to its original
	// object class. Then we can call wxStaticBoxSizer::GetStaticBox() in it.
	wxStaticBoxSizer* psbSizer = wxDynamicCast(STATIC_TEXT_PTorBE_PROJECTS,wxStaticBoxSizer); // use dynamic case because wxDesigner cast it to wxSizer*
	pStaticBoxUsingTheseProjects = (wxStaticBox*)psbSizer->GetStaticBox();
	wxASSERT(pStaticBoxUsingTheseProjects != NULL);

	pUsingAIProjectName = (wxStaticText*)FindWindowById(ID_TEXT_AI_PROJ);
	wxASSERT(pUsingAIProjectName != NULL);
	
	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	// other attribute initializations
}

CGetSourceTextFromEditorDlg::~CGetSourceTextFromEditorDlg() // destructor
{
	// BEW 15Sep14, added next two lines 
	pListBoxBookNames->Clear();
	pListCtrlChapterNumberAndStatus->DeleteAllItems(); 

	if (pTheFirstColumn != NULL) // whm 11Jun12 added NULL test
		delete pTheFirstColumn;
	if (pTheSecondColumn != NULL) // whm 11Jun12 added NULL test
		delete pTheSecondColumn;
}

void CGetSourceTextFromEditorDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual,  no call needed to a base class

// whm 5Jun12 added the define below for testing and debugging of Setup Collaboration dialog only
#if defined(FORCE_BIBLEDIT_IS_INSTALLED_FLAG)
	wxCHECK_RET(FALSE,_T("!!! Programming Error !!!\n\nComment out the FORCE_BIBLEDIT_IS_INSTALLED_FLAG define in Adapt_It.h for normal debug builds - otherwise the GetSourceTextFromEditor dialog will not work properly!"));
#endif

	// Note: the wxListItem which is the column has to be on the heap, because if made a local
	// variable then it will go out of scope and be lost from the wxListCtrl before the
	// latter has a chance to display anything, and then nothing will display in the control
	pTheFirstColumn = new wxListItem; // deleted in the destructor
	pTheSecondColumn = new wxListItem; // deleted in the destructor
	
	wxString title = this->GetTitle();
	title = title.Format(title,this->m_collabEditorName.c_str());
	this->SetTitle(title);

	// Substitute "Paratext" or "Bibledit" in "Using these %s projects:"
	wxString usingTheseProj = pStaticBoxUsingTheseProjects->GetLabel();
	wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
	usingTheseProj = usingTheseProj.Format(usingTheseProj,m_pApp->m_collaborationEditor.c_str());
	pStaticBoxUsingTheseProjects->SetLabel(usingTheseProj);

	// Assign the Temp values for use in this dialog until OnOK() executes
	m_TempCollabProjectForSourceInputs = m_pApp->m_CollabProjectForSourceInputs;
	m_TempCollabProjectForTargetExports = m_pApp->m_CollabProjectForTargetExports;
	m_TempCollabProjectForFreeTransExports = m_pApp->m_CollabProjectForFreeTransExports;
	m_TempCollabAIProjectName = m_pApp->m_CollabAIProjectName;
	m_TempCollabSourceProjLangName = m_pApp->m_CollabSourceLangName;
	m_TempCollabTargetProjLangName = m_pApp->m_CollabTargetLangName;
	m_TempCollabBookSelected = m_pApp->m_CollabBookSelected;
	m_bTempCollabByChapterOnly = m_pApp->m_bCollabByChapterOnly;
	m_TempCollabChapterSelected = m_pApp->m_CollabChapterSelected;
	m_bTempCollaborationExpectsFreeTrans = m_pApp->m_bCollaborationExpectsFreeTrans;

	m_SaveCollabProjectForSourceInputs = m_TempCollabProjectForSourceInputs;
	m_SaveCollabProjectForTargetExports = m_TempCollabProjectForTargetExports;
	m_SaveCollabProjectForFreeTransExports = m_TempCollabProjectForFreeTransExports;
	m_SaveCollabAIProjectName = m_TempCollabAIProjectName;
	m_SaveCollabSourceProjLangName = m_TempCollabSourceProjLangName;
	m_SaveCollabTargetProjLangName = m_TempCollabTargetProjLangName;
	m_SaveCollabBookSelected = m_TempCollabBookSelected;
	m_bSaveCollabByChapterOnly = m_bTempCollabByChapterOnly; // FALSE means the "whole book" option
	m_SaveCollabChapterSelected = m_TempCollabChapterSelected;
	m_bSaveCollaborationExpectsFreeTrans = m_bTempCollaborationExpectsFreeTrans;

	if (!m_pApp->m_bCollaboratingWithBibledit)
	{
		m_rdwrtp7PathAndFileName = GetPathToRdwrtp7(); // see CollabUtilities.cpp
	}
	else
	{
		m_bibledit_rdwrtPathAndFileName = GetPathToBeRdwrt(); // will return
								// an empty string if BibleEdit is not installed;
								// see CollabUtilities.cpp
	}

	// Generally when the "Get Source Text from Paratext/Bibledit Project" dialog is called, we 
	// can be sure that some checks have been done to ensure that Paratext/Bibledit is installed,
	// that previously selected PT/BE projects are still valid/exist, etc. Those consistency and
	// validity checks were done in the GetAIProjectCollabStatus() function call in ProjectPage's
	// OnWizardPageChanging() just after the user has selected the project to open and the
	// selected project's project config file has been read.
	
	// whm revised 1Mar12 at Bruce's request to use the whole composite string in 
	// the case of Paratext.
	// whm Note: GetAILangNamesFromAIProjectNames() below issues error message if the
	// m_TempCollabAIProjectName is mal-formed (empty, or has no " to " or " adaptations")
	pSrcProj->SetLabel(m_TempCollabProjectForSourceInputs);
	pTgtProj->SetLabel(m_TempCollabProjectForTargetExports);
	pFreeTransProj->SetLabel(m_TempCollabProjectForFreeTransExports);
	
	pUsingAIProjectName->SetLabel(m_TempCollabAIProjectName); // whm added 28Jan12

	LoadBookNamesIntoList();

	pTheFirstColumn->SetText(_("Chapter"));
	pTheFirstColumn->SetImage(-1);
	pTheFirstColumn->SetAlign(wxLIST_FORMAT_CENTRE);
	pListCtrlChapterNumberAndStatus->InsertColumn(0, *pTheFirstColumn);

	pTheSecondColumn->SetText(_("Target Text Status"));
	pTheSecondColumn->SetImage(-1);
	pListCtrlChapterNumberAndStatus->InsertColumn(1, *pTheSecondColumn);

	// select LastPTBookSelected 
	if (!m_TempCollabBookSelected.IsEmpty())
	{
		int nSel = pListBoxBookNames->FindString(m_TempCollabBookSelected);
		if (nSel != wxNOT_FOUND)
		{
			// get extent for the current book name for sizing first column
			wxSize sizeOfBookNameAndCh;
			wxClientDC aDC(this);
			wxFont tempFont = this->GetFont();
			aDC.SetFont(tempFont);
			aDC.GetTextExtent(m_TempCollabBookSelected,&sizeOfBookNameAndCh.x,&sizeOfBookNameAndCh.y);
			pTheFirstColumn->SetWidth(sizeOfBookNameAndCh.GetX() + 30); // 30 fudge factor
			
			int height,widthListCtrl,widthCol1;
			pListCtrlChapterNumberAndStatus->GetClientSize(&widthListCtrl,&height);
			widthCol1 = pTheFirstColumn->GetWidth();
			pTheSecondColumn->SetWidth(widthListCtrl - widthCol1);
			
			pListCtrlChapterNumberAndStatus->InsertColumn(0, *pTheFirstColumn);
			pListCtrlChapterNumberAndStatus->InsertColumn(1, *pTheSecondColumn);
			
			// the pListBoxBookNames must have a selection before OnLBBookSelected() below will do anything
			pListBoxBookNames->SetSelection(nSel);
			// set focus on the Select a book list (OnLBBookSelected call below may change focus to Select a chapter list)
			pListBoxBookNames->SetFocus(); 
			wxCommandEvent evt;
			OnLBBookSelected(evt);
			// whm added 29Jul11. If "Get Whole Book" is ON, we can set the focus
			// on the OK button
			if (!m_bTempCollabByChapterOnly)
			{
				// Get Whole Book is ON, so set focus on OK button
				pBtnOK->SetFocus();
			}
		}
	}
	// Normally at this point the "Select a book" list will be populated and any previously
	// selected book will now be selected. If a book is selected, the "Select a chapter" list
	// will also be populated and, if any previously selected chaper will now again be 
	// selected (from actions done within the OnLBBookSelected handler called above).
	
	// whm modified 7Jan12 to call RefreshStatusBarInfo which now incorporates collaboration
	// info within its status bar message
	m_pApp->RefreshStatusBarInfo();

	pGetSourceTextFromEditorSizer->Layout(); // update the layout for $s substitutions
	// Some control text may be truncated unless we resize the dialog to fit it. 
	// Note: The constructor's call of GetSourceTextFromEditorDlgFunc(this, FALSE, TRUE)
	// has its second parameter as FALSE to allow this resize here in InitDialog().
	wxSize dlgSize;
	dlgSize = pGetSourceTextFromEditorSizer->ComputeFittingWindowSize(this);
	this->SetSize(dlgSize);
	this->CenterOnParent();

	// whm added 8Sept14. The OK button seems to loose its focus after the above calls, so I'm
	// calling SetFocus() again here, as long as a book and chapter has been selected.
	if (!m_TempCollabChapterSelected.IsEmpty() && !m_TempCollabBookSelected.IsEmpty())
	{
		pBtnOK->SetFocus();
	}
 }

 // OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
// Note: because the user is not limited from changing to a different language in PT or
// BE, the next "get" of source text might map to a different AI project, so closure of a
// chapter-document or book-document must also unload the adapting and glossing KBs, and
// set their m_pKB and m_pGlossingKB pointers to NULL. The appropriate places for this
// would, I think, be in the handler for File / Close, and in the view's member function
// ClobberDocument(). Each "get" of a chapter or book from the external editor should set
// up the mapping to the appropriate AI project, if there is one, each time - and load the
// KBs for it.

void CGetSourceTextFromEditorDlg::OnOK(wxCommandEvent& event) 
{
	if (pListBoxBookNames->GetSelection() == wxNOT_FOUND)
	{
		wxMessageBox(_("Please select a book from the list of books."),_T(""),wxICON_INFORMATION | wxOK, this); // whm 28Nov12 added this as parent
		pListBoxBookNames->SetFocus();
		return; // don't accept any changes until a book is selected
	}
	
	// whm added 27Jul11 a test for whether we are working by chapter
	if (m_bTempCollabByChapterOnly && pListCtrlChapterNumberAndStatus->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED) == wxNOT_FOUND)
	{
		wxMessageBox(_("Please select a chapter from the list of chapters."),_T(""),wxICON_INFORMATION | wxOK, this); // whm 28Nov12 added this as parent);
		pListCtrlChapterNumberAndStatus->SetFocus();
		return; // don't accept any changes until a chapter is selected
	}

	// check the KBs are clobbered, if not so, do so
	UnloadKBs(m_pApp);

	int chSel = pListCtrlChapterNumberAndStatus->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
	if (chSel != wxNOT_FOUND)
	{
		chSel++; // the chapter number is one greater than the selected lb index
	}
	else
	{
		chSel = 0; // indicates we're dealing with Whole Book 
	}
	// if chSel is 0, the string returned to derivedChStr will be _T("0")
	m_collab_bareChapterSelectedStr.Empty();
	m_collab_bareChapterSelectedStr << chSel;
	wxString derivedChStr = GetBareChFromLBChSelection(m_TempCollabChapterSelected);
	wxASSERT(m_collab_bareChapterSelectedStr == derivedChStr);

	// do sanity check on AI project name and source and target language names
	m_pApp->GetSrcAndTgtLanguageNamesFromProjectName(m_TempCollabAIProjectName, m_TempCollabSourceProjLangName, m_TempCollabTargetProjLangName);
	wxASSERT(!m_TempCollabSourceProjLangName.IsEmpty() && !m_TempCollabTargetProjLangName.IsEmpty());
	// save new project selection to the App's variables for writing to config file
	// BEW 1Aug11, moved this block of assignments to after bareChapterSelectedStr is set
	// just above, otherwise the value that m_CollabChapterSelected is that which came
	// from reading the config file, not from what the user chose in the dialog (or so the
	// code seems to be saying, but if I'm wrong, this move is harmless)
	m_pApp->m_CollabProjectForSourceInputs = m_TempCollabProjectForSourceInputs;
	m_pApp->m_CollabProjectForTargetExports = m_TempCollabProjectForTargetExports;
	m_pApp->m_CollabProjectForFreeTransExports = m_TempCollabProjectForFreeTransExports;
	m_pApp->m_CollabAIProjectName = m_TempCollabAIProjectName;
	m_pApp->m_bCollaborationExpectsFreeTrans = m_bTempCollaborationExpectsFreeTrans;
	m_pApp->m_CollabBookSelected = m_TempCollabBookSelected;
	m_pApp->m_bCollabByChapterOnly = m_bTempCollabByChapterOnly;
	m_pApp->m_CollabChapterSelected = m_collab_bareChapterSelectedStr; // use the 
						// bare number string rather than the list box's book + ' ' + chnumber
	m_pApp->m_CollabSourceLangName = m_TempCollabSourceProjLangName;
	m_pApp->m_CollabTargetLangName = m_TempCollabTargetProjLangName;

	// whm 26Feb12 Note: With project-specific collaboration, the user's choice of 
	// m_CollabBookSelected and m_CollabChapterSelected within the current dialog has
	// been saved to the App's members, but has not yet been saved in the specific 
	// AI Project's config file. The HookUpToExistingAIProject() function below will
	// read the AI-ProjectConfiguration.aic file to get the general project settings
	// which would overwrite the above values for the App's m_CollabBookSelected and
	// m_CollabChapterSelected. It would seem necessary to update the external project
	// config file here before HookUpToExistingAIProject() reads those values.
	wxString projectFolderPath = m_pApp->m_curProjectPath + m_pApp->PathSeparator + szProjectConfiguration + _T(".aic");
	bool bReadOK;
	bReadOK = m_pApp->WriteConfigurationFile(szProjectConfiguration,m_pApp->m_curProjectPath,projectConfigFile);
	if (!bReadOK)
	{
		// Writing of the project config file failed for some reason. This would be unusual, so
		// just do an English notification
		wxCHECK_RET(bReadOK, _T("GetSourceTextFromEditor(): WriteConfigurationFile() failed, line 1247."));
	}

	m_pApp->m_bEnableDelayedGetChapterHandler = TRUE; // OnIdle() handler has a
		// block to be entered when this is TRUE, the code block calls
		// OK_btn_delayedHandler_GetSourceTextFromEditor(gpApp) and also
		// m_bEnableDelayedGetChapterHandler is set FALSE to ensure only one call

				
	/* not needed, I've put 2nd and 3rd lines instead into the class destructor
	pListBoxBookNames->SetSelection(-1); // remove any selection
	// clear lists and static text box at bottom of dialog
	pListBoxBookNames->Clear();
	// in next call don't use ClearAll() because it clobbers any columns too
	pListCtrlChapterNumberAndStatus->DeleteAllItems(); 
	pStaticTextCtrlNote->ChangeValue(_T(""));
	*/
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


// event handling functions

// BEW 1Aug11 **** NOTE ****
// The code for obtaining the source and target text from the external editor, which in
// this function, could be replaced by a single line call
// TransferTextBetweenAdaptItAndExternalEditor() from CollabUtilities() EXCEPT for one
// vital condition, namely: what is done here may be Cancelled by the user in the dialog,
// and so the code here uses various temporary variables; whereas 
// TransferTextBetweenAdaptItAndExternalEditor() accesses persistent (for the session)
// variables stored on the app, which get populated only after the user has clicked the
// dialog's OK button, hence invoking the OnOK() handler.
// 
// whm Note: OnLBBookSelected() is called from the OnComboBoxSelectSourceProject(), from
// OnComboBoxSelectTargetProject(), and from the dialog's InitDialog() when it receives
// config file values that enable it to select the last PT/BE book selected in a previous
// session.
void CGetSourceTextFromEditorDlg::OnLBBookSelected(wxCommandEvent& WXUNUSED(event))
{
	if (pListBoxBookNames->GetSelection() == wxNOT_FOUND)
		return;

	// Since the wxExecute() and file read operations take a few seconds, change the "Select a chapter:"
	// static text above the right list box to read: "Please wait while I query Paratext/Bibledit..."
	wxString waitMsg = _("Please wait while I query %s...");
	waitMsg = waitMsg.Format(waitMsg,m_collabEditorName.c_str());
	pStaticSelectAChapter->SetLabel(waitMsg);
	
	int nSel;
	wxString fullBookName;
	nSel = pListBoxBookNames->GetSelection();
	fullBookName = pListBoxBookNames->GetString(nSel);
	// When the user selects a book, this handler does the following:
	// 1. Call rdwrtp7 to get copies of the book in a temporary file at a specified location.
	//    One copy is made of the source PT/BE project's book, and the other copy is made of the
	//    destination/target PT/BE project's book
	// 2. Open each of the temporary book files and read them into wxString buffers.
	// 3. Scan through the buffers, collecting their usfm markers - simultaneously counting
	//    the number of characters associated with each marker (0 = empty).
	// 4. Only the actual marker, plus a : delimiter, plus the character count associated with
	//    the marker plus a : delimiter plus an MD5 checksum is collected and stored in two 
	//    wxArrayStrings, one marker (and count and MD5 checksum) per array element.
	// 5. This collection of the usfm structure (and extent) is done for both the source PT/BE 
	//    book's data and the destination PT/BE book's data.
	// 6. The two wxArrayString collections will make it an easy matter to compare the 
	//    structure and extent of the two texts even though the actual textual content will,
	//    of course, vary, because each text would normally be a different language.
	// Using the two wxArrayStrings:
	// a. We can easily find which verses of the destination text are empty and (if they are
	// not also empty in the source text) conclude that those destination text verses have
	// not yet been translated.
	// b. We can easily compare the usfm structure between the source and 
	// destination texts to see if the structure has changed. 
	// 
	// Each string element of the wxArrayStrings is of the following form:
	// \mkr:nnnn, where \mkr could be \s, \p, \c 2, \v 23, \v 42-44, etc., followed by
	// a colon (:), followed by a character count nnnn, followed by either another :0 or
	// an MD5 checksum.
	// Here is an example of what a returned array might look like:
	//	\id:49:2f17e081efa1f7789bac5d6e500fc3d5
	//	\mt:6:010d9fd8a87bb248453b361e9d4b3f38
	//	\c 1:0:0
	//	\s:16:e9f7476ed5087739a673a236ee810b4c
	//	\p:0:0
	//	\v 1:138:ef64c033263f39fdf95b7fe307e2b776
	//	\v 2:152:ec5330b7cb7df48628facff3e9ce2e25
	//	\v 3:246:9ebe9d27b30764602c2030ba5d9f4c8a
	//	\v 4:241:ecc7fb3dfb9b5ffeda9c440db0d856fb
	//	\v 5:94:aea4ba44f438993ca3f44e2ccf5bdcaf
	//	\p:0:0
	//	\v 6:119:639858cb1fc6b009ee55e554cb575352
	//	\f:322:a810d7d923fbedd4ef3a7120e6c4af93
	//	\f*:0:0
	//	\p:0:0
	//	\v 7:121:e17f476eb16c0589fc3f9cc293f26531
	//	\v 8:173:4e3a18eb839a4a57024dba5a40e72536
	//	\p:0:0
	//	\v 9:124:ad649962feeeb2715faad0cc78274227
	//	\p:0:0
	//	\v 10:133:3171aeb32d39e2da92f3d67da9038cf6
	//	\v 11:262:fca59249fe26ee4881d7fe558b24ea49
	//	\s:29:3f7fcd20336ae26083574803b7dddf7c
	//	\p:0:0
	//	\v 12:143:5f71299ac7c347b7db074e3c327cef7e
	//	\v 13:211:6df92d40632c1de539fa3eeb7ba2bc0f
	//	\v 14:157:5383e5a5dcd6976877b4bc82abaf4fee
	//	\p:0:0
	//	\v 15:97:47e16e4ae8bfd313deb6d9aef4e33ca7
	//	\v 16:197:ce14cd0dd77155fa23ae0326bac17bdd
	//	\v 17:51:b313ee0ee83a10c25309c7059f9f46b3
	//	\v 18:143:85b88e5d3e841e1eb3b629adf1345e7b
	//	\v 19:101:2f30dec5b8e3fa7f6a0d433d65a9aa1d
	//	\p:0:0
	//	\v 20:64:b0d7a2fc799e6dc9f35a44ce18755529
	//	\p:0:0
	//	\v 21:90:e96d4a1637d901d225438517975ad7c8
	//	\v 22:165:36f37b24e0685939616a04ae7fc3b56d
	//	\p:0:0
	//	\v 23:96:53b6c4c5180c462c1c06b257cb7c33f8
	//	\f:23:317f2a231b8f9bcfd13a66f45f0c8c72
	//	\fk:19:db64e9160c4329440bed9161411f0354
	//	\fk*:1:5d0b26628424c6194136ac39aec25e55
	//	\f*:7:86221a2454f5a28121e44c26d3adf05c
	//	\v 24-25:192:4fede1302a4a55a4f0973f5957dc4bdd
	//	\v 26:97:664ca3f0e110efe790a5e6876ffea6fc
	//	\c 2:0:0
	//	\s:37:6843aea2433b54de3c2dad45e638aea0
	//	\p:0:0
	//	\v 1:19:47a1f2d8786060aece66eb8709f171dc
	//	\v 2:137:78d2e04d80f7150d8c9a7c123c7bcb80
	//	\v 3:68:8db3a04ff54277c792e21851d91d93e7
	//	\v 4:100:9f3cff2884e88ceff63eb8507e3622d2
	//	\p:0:0
	//	\v 5:82:8d32aba9d78664e51cbbf8eab29fcdc7
	// 	\v 6:151:4d6d314459a65318352266d9757567f1
	//	\v 7:95:73a88b1d087bc4c5ad01dd423f3e45d0
	//	\v 8:71:aaeb79b24bdd055275e94957c9fc13c2
	// Note: The first number field after the usfm (delimited by ':') is a character count 
	// for any text associated with that usfm. The last number field represents the MD5 checksum,
	// except that only usfm markers that are associated with actual text have the 32 byte MD5
	// checksums. Other markers, i.e., \c, \p, have 0 in the MD5 checksum field.
	
	// Bridged verses might look like the following:
	// \v 23-25:nnnn:MD5
	
	// We also need to call rdwrtp7 to get a copy of the target text book (if it exists). We do the
	// same scan on it collecting an wxArrayString of values that tell us what chapters exist and have
	// been translated.
	// 
	// Usage: rdwrtp7 -r|-w project book chapter|0 fileName
	wxString bookCode;
	bookCode = m_pApp->GetBookCodeFromBookName(fullBookName);
	
	// ensure that a .temp folder exists in the m_workFolderPath
	wxString tempFolder;
	tempFolder = m_pApp->m_workFolderPath + m_pApp->PathSeparator + _T(".temp");
	if (!::wxDirExists(tempFolder))
	{
		::wxMkdir(tempFolder);
	}


	wxString sourceProjShortName;
	wxString targetProjShortName;
	wxString freeTransProjShortName; // whm added 6Feb12
	wxASSERT(!m_TempCollabProjectForSourceInputs.IsEmpty());
	wxASSERT(!m_TempCollabProjectForTargetExports.IsEmpty());
	sourceProjShortName = GetShortNameFromProjectName(m_TempCollabProjectForSourceInputs);
	targetProjShortName = GetShortNameFromProjectName(m_TempCollabProjectForTargetExports);
	if (m_bTempCollaborationExpectsFreeTrans)
	{
		freeTransProjShortName = GetShortNameFromProjectName(m_TempCollabProjectForFreeTransExports); // whm added 6Feb12
	}
	wxString bookNumAsStr = m_pApp->GetBookNumberAsStrFromName(fullBookName);
	
	wxString sourceTempFileName;
	sourceTempFileName = tempFolder + m_pApp->PathSeparator;
	sourceTempFileName += m_pApp->GetFileNameForCollaboration(_T("_Collab"), bookCode, sourceProjShortName, wxEmptyString, _T(".tmp"));
	wxString targetTempFileName;
	targetTempFileName = tempFolder + m_pApp->PathSeparator;
	targetTempFileName += m_pApp->GetFileNameForCollaboration(_T("_Collab"), bookCode, targetProjShortName, wxEmptyString, _T(".tmp"));
	
	// whm added 6Feb12
	wxString freeTransTempFileName;
	freeTransTempFileName = tempFolder + m_pApp->PathSeparator;
	freeTransTempFileName += m_pApp->GetFileNameForCollaboration(_T("_Collab"), bookCode, freeTransProjShortName, wxEmptyString, _T(".tmp"));
	
	// Build the command lines for reading the PT projects using rdwrtp7.exe
	// and BE projects using bibledit-rdwrt (or adaptit-bibledit-rdwrt).
	
	// whm 23Aug11 Note: We are not using Bruce's BuildCommandLineFor() here to build the 
	// commandline, because within GetSourceTextFromEditor() we are using temp variables 
	// for passing to the GetShortNameFromProjectName() function above, whereas 
	// BuildCommandLineFor() uses the App's values for m_CollabProjectForSourceInputs, 
	// m_CollabProjectForTargetExports, and m_CollabProjectForFreeTransExports. Those App 
	// values are not assigned until GetSourceTextFromEditor()::OnOK() is executed and the 
	// dialog is about to be closed.
	
	// whm 17Oct11 modified the commandline strings below to quote the source and target
	// short project names (sourceProjShortName and targetProjShortName). This is especially
	// important for the Bibledit projects, since we use the language name for the project
	// name and it can contain spaces, whereas in the Paratext command line strings the
	// short project name is used which doesn't generally contain spaces.
	wxString commandLineSrc,commandLineTgt,commandLineFT;
	if (m_pApp->m_bCollaboratingWithParatext)
	{
	    if (m_rdwrtp7PathAndFileName.Contains(_T("paratext"))) // note the lower case p in paratext
	    {
	        // PT on linux -- need to add --rdwrtp7 as the first param to the command line
            commandLineSrc = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" --rdwrtp7 ") + _T("-r") + _T(" ") + _T("\"") + sourceProjShortName + _T("\"") + _T(" ") + bookCode + _T(" ") + _T("0") + _T(" ") + _T("\"") + sourceTempFileName + _T("\"");
            commandLineTgt = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" --rdwrtp7 ") + _T("-r") + _T(" ") + _T("\"") + targetProjShortName + _T("\"") + _T(" ") + bookCode + _T(" ") + _T("0") + _T(" ") + _T("\"") + targetTempFileName + _T("\"");
            if (m_bTempCollaborationExpectsFreeTrans) // whm added 6Feb12
                commandLineFT = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" --rdwrtp7 ") + _T("-r") + _T(" ") + _T("\"") + freeTransProjShortName + _T("\"") + _T(" ") + bookCode + _T(" ") + _T("0") + _T(" ") + _T("\"") + freeTransTempFileName + _T("\"");
	    }
	    else
	    {
	        // PT on Windows
            commandLineSrc = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + _T("\"") + sourceProjShortName + _T("\"") + _T(" ") + bookCode + _T(" ") + _T("0") + _T(" ") + _T("\"") + sourceTempFileName + _T("\"");
            commandLineTgt = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + _T("\"") + targetProjShortName + _T("\"") + _T(" ") + bookCode + _T(" ") + _T("0") + _T(" ") + _T("\"") + targetTempFileName + _T("\"");
            if (m_bTempCollaborationExpectsFreeTrans) // whm added 6Feb12
                commandLineFT = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + _T("\"") + freeTransProjShortName + _T("\"") + _T(" ") + bookCode + _T(" ") + _T("0") + _T(" ") + _T("\"") + freeTransTempFileName + _T("\"");
	    }
	}
	else if (m_pApp->m_bCollaboratingWithBibledit)
	{
		commandLineSrc = _T("\"") + m_bibledit_rdwrtPathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + _T("\"") + sourceProjShortName + _T("\"") + _T(" ") + bookCode + _T(" ") + _T("0") + _T(" ") + _T("\"") + sourceTempFileName + _T("\"");
		commandLineTgt = _T("\"") + m_bibledit_rdwrtPathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + _T("\"") + targetProjShortName + _T("\"") + _T(" ") + bookCode + _T(" ") + _T("0") + _T(" ") + _T("\"") + targetTempFileName + _T("\"");
		if (m_bTempCollaborationExpectsFreeTrans) // whm added 6Feb12
			commandLineFT = _T("\"") + m_bibledit_rdwrtPathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + _T("\"") + freeTransProjShortName + _T("\"") + _T(" ") + bookCode + _T(" ") + _T("0") + _T(" ") + _T("\"") + freeTransTempFileName + _T("\"");
	}
	wxLogDebug(commandLineSrc);
	wxLogDebug(commandLineTgt);
	wxLogDebug(commandLineFT); // whm added 6Feb12

    // Note: Looking at the wxExecute() source code in the 2.8.11 library, it is clear that
    // when the overloaded version of wxExecute() is used, it uses the redirection of the
    // stdio to the arrays, and with that redirection, it doesn't show the console process
    // window by default. It is distracting to have the DOS console window flashing even
    // momentarily, so we will use that overloaded version of rdwrtp7.exe.

	long resultSrc = -1;
	long resultTgt = -1;
	long resultFT = -1; // whm added 6Feb12
    wxArrayString outputSrc, errorsSrc;
	wxArrayString outputTgt, errorsTgt;
	wxArrayString outputFT, errorsFT; // whm added 6Feb12
	// Note: _EXCHANGE_DATA_DIRECTLY_WITH_BIBLEDIT is defined near beginning of Adapt_It.h
	// Defined to 0 to use Bibledit's command-line interface to fetch text and write text 
	// from/to its project data files. Defined as 0 is the normal setting.
	// Defined to 1 to fetch text and write text directly from/to Bibledit's project data 
	// files (not using command-line interface). Defined to 1 was for testing purposes
	// only before Teus provided the command-line utility bibledit-rdwrt.
	if (m_pApp->m_bCollaboratingWithParatext || _EXCHANGE_DATA_DIRECTLY_WITH_BIBLEDIT == 0)
	{
		// Use the wxExecute() override that takes the two wxStringArray parameters. This
		// also redirects the output and suppresses the dos console window during execution.
		resultSrc = ::wxExecute(commandLineSrc,outputSrc,errorsSrc);
		if (resultSrc == 0)
		{
			resultTgt = ::wxExecute(commandLineTgt,outputTgt,errorsTgt);
			if (resultTgt == 0 && m_bTempCollaborationExpectsFreeTrans)
			{
				resultFT = ::wxExecute(commandLineFT,outputFT,errorsFT);
			}

		}
	}
	else if (m_pApp->m_bCollaboratingWithBibledit)
	{
		// Collaborating with Bibledit and _EXCHANGE_DATA_DIRECTLY_WITH_BIBLEDIT == 1
		// Note: This code block will not be used in production. It was only for testing
		// purposes.
		wxString beProjPath = m_pApp->GetBibleditProjectsDirPath();
		wxString beProjPathSrc, beProjPathTgt;
		beProjPathSrc = beProjPath + m_pApp->PathSeparator + sourceProjShortName;
		beProjPathTgt = beProjPath + m_pApp->PathSeparator + targetProjShortName;
		int chNumForBEDirect = -1; // for Bibledit to get whole book
		bool bWriteOK;
		bWriteOK = CopyTextFromBibleditDataToTempFolder(beProjPathSrc, fullBookName, chNumForBEDirect, sourceTempFileName, errorsSrc);
		if (bWriteOK)
			resultSrc = 0; // 0 means same as wxExecute() success
		else // bWriteOK was FALSE
			resultSrc = 1; // 1 means same as wxExecute() ERROR, errorsSrc will contain error message(s)
		if (resultSrc == 0)
		{
			bWriteOK = CopyTextFromBibleditDataToTempFolder(beProjPathTgt, fullBookName, chNumForBEDirect, targetTempFileName, errorsTgt);
			if (bWriteOK)
				resultTgt = 0; // 0 means same as wxExecute() success
			else // bWriteOK was FALSE
				resultTgt = 1; // 1 means same as wxExecute() ERROR, errorsSrc will contain error message(s)
		}
	}

	if (resultSrc != 0 || resultTgt != 0 || (m_bTempCollaborationExpectsFreeTrans && resultFT != 0))
	{
		// get the console output and error output, format into a string and 
		// include it with the message to user
		wxString outputStr,errorsStr;
		outputStr.Empty();
		errorsStr.Empty();
		int ct;
		if (resultSrc != 0)
		{
			for (ct = 0; ct < (int)outputSrc.GetCount(); ct++)
			{
				if (!outputStr.IsEmpty())
					outputStr += _T(' ');
				outputStr += outputSrc.Item(ct);
			}
			for (ct = 0; ct < (int)errorsSrc.GetCount(); ct++)
			{
				if (!errorsStr.IsEmpty())
					errorsStr += _T(' ');
				errorsStr += errorsSrc.Item(ct);
			}
		}
		if (resultTgt != 0)
		{
			for (ct = 0; ct < (int)outputTgt.GetCount(); ct++)
			{
				if (!outputStr.IsEmpty())
					outputStr += _T(' ');
				outputStr += outputTgt.Item(ct);
			}
			for (ct = 0; ct < (int)errorsTgt.GetCount(); ct++)
			{
				if (!errorsStr.IsEmpty())
					errorsStr += _T(' ');
				errorsStr += errorsTgt.Item(ct);
			}
		}
		
		if (m_bTempCollaborationExpectsFreeTrans && resultFT != 0)
		{
			for (ct = 0; ct < (int)outputFT.GetCount(); ct++)
			{
				if (!outputStr.IsEmpty())
					outputStr += _T(' ');
				outputStr += outputFT.Item(ct);
			}
			for (ct = 0; ct < (int)errorsFT.GetCount(); ct++)
			{
				if (!errorsStr.IsEmpty())
					errorsStr += _T(' ');
				errorsStr += errorsFT.Item(ct);
			}
		}

		wxString msg;
		wxString concatMsgs;
		if (!outputStr.IsEmpty())
			concatMsgs = outputStr;
		if (!errorsStr.IsEmpty())
			concatMsgs += errorsStr;
		if (m_pApp->m_bCollaboratingWithParatext)
		{
			msg = _("Could not read data from the Paratext projects.\nError(s) reported:\n   %s\n\nPlease submit a problem report to the Adapt It developers (see the Help menu).");
		}
		else if (m_pApp->m_bCollaboratingWithBibledit)
		{
			msg = _("Could not read data from the Bibledit projects.\nError(s) reported:\n   %s\n\nPlease submit a problem report to the Adapt It developers (see the Help menu).");
		}
		msg = msg.Format(msg,concatMsgs.c_str());
		wxMessageBox(msg,_T(""),wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
		m_pApp->LogUserAction(msg);
		wxLogDebug(msg);

		pListBoxBookNames->SetSelection(-1); // remove any selection
		// clear lists and static text box at bottom of dialog
		pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		pStaticSelectAChapter->SetLabel(_("Select a &chapter:"));
		pStaticSelectAChapter->Refresh();
		return;
	}

	// now read the tmp files into buffers in preparation for analyzing their chapter and
	// verse status info (1:1:nnnn).
	// Note: The files produced by rdwrtp7.exe for projects with 65001 encoding (UTF-8) have a 
	// UNICODE BOM of ef bb bf

	// whm 21Sep11 Note: When grabbing the source text, we need to ensure that 
	// an \id XXX line is at the beginning of the text, therefore the second 
	// parameter in the GetTextFromAbsolutePathAndRemoveBOM() call below is 
	// the bookCode. The addition of an \id XXX line will not normally be needed
	// when grabbing whole books, but the check is made here to be safe. This
	// call of GetTextFromAbsolutePathAndRemoveBOM() mainly gets the whole source
	// text for use in analyzing the chapter status, but I believe it is also
	// the main call that stores the text in the .temp folder which the app
	// would use if the whole book is used for collaboration, so we should
	// do the check here too just to be safe.
	sourceWholeBookBuffer = GetTextFromAbsolutePathAndRemoveBOM(sourceTempFileName,bookCode);

	// whm 21Sep11 Note: When grabbing the target text, we don't need to add
	// an \id XXX line at the beginning of the text, therefore the second 
	// parameter in the GetTextFromAbsolutePathAndRemoveBOM() call below is 
	// the wxEmptyString. Whole books will usually already have the \id XXX
	// line, but we don't need to ensure that it exists for the target text.
	// The call of GetTextFromAbsolutePathAndRemoveBOM() mainly gets the whole 
	// target text for use in analyzing the chapter status, but I believe it 
	// is also the main call that stores the text in the .temp folder which the 
	// app would use if the whole book is used for collaboration.
	targetWholeBookBuffer = GetTextFromAbsolutePathAndRemoveBOM(targetTempFileName,wxEmptyString);

	SourceTextUsfmStructureAndExtentArray.Clear();
	SourceTextUsfmStructureAndExtentArray = GetUsfmStructureAndExtent(sourceWholeBookBuffer);
	TargetTextUsfmStructureAndExtentArray.Clear();
	TargetTextUsfmStructureAndExtentArray = GetUsfmStructureAndExtent(targetWholeBookBuffer);
	
	// whm added 6Feb12
	if (m_bTempCollaborationExpectsFreeTrans)
	{
		freeTransWholeBookBuffer = GetTextFromAbsolutePathAndRemoveBOM(freeTransTempFileName,wxEmptyString);
		FreeTransTextUsfmStructureAndExtentArray.Clear();
		FreeTransTextUsfmStructureAndExtentArray = GetUsfmStructureAndExtent(freeTransWholeBookBuffer); 
	}
	
	/* !!!! whm testing below !!!!
	// =====================================================================================
	int testNum;
	int totTestNum = 4;
	// get the constant baseline buffer from the test cases which are on my 
	// local machine at: C:\Users\Bill Martin\Desktop\testing
	//wxString basePathAndName = _T("C:\\Users\\Bill Martin\\Desktop\\testing\\49GALNYNT_base1.SFM");
	wxString basePathAndName = _T("C:\\Users\\Bill Martin\\Desktop\\testing\\49GALNYNT_base2_change_usfm_text_and_length_shorter incl bridged.SFM");
	wxASSERT(::wxFileExists(basePathAndName));
	wxFile f_test_base(basePathAndName,wxFile::read);
	wxFileOffset fileLenBase;
	fileLenBase = f_test_base.Length();
	// read the raw byte data into pByteBuf (char buffer on the heap)
	char* pBaseLineByteBuf = (char*)malloc(fileLenBase + 1);
	memset(pBaseLineByteBuf,0,fileLenBase + 1); // fill with nulls
	f_test_base.Read(pBaseLineByteBuf,fileLenBase);
	wxASSERT(pBaseLineByteBuf[fileLenBase] == '\0'); // should end in NULL
	f_test_base.Close();
	wxString baseWholeBookBuffer = wxString(pBaseLineByteBuf,wxConvUTF8,fileLenBase);
	free((void*)pBaseLineByteBuf);

	wxArrayString baseLineArray;
	baseLineArray = GetUsfmStructureAndExtent(baseWholeBookBuffer);

	wxString testPathAndName[7] = {
					_T("C:\\Users\\Bill Martin\\Desktop\\testing\\49GALNYNT_base2_change_usfm_text_and_length_longer no bridged.SFM"),
					_T("C:\\Users\\Bill Martin\\Desktop\\testing\\49GALNYNT_test5_change_usfm_text_and_length incl bridged.SFM"),
					_T("C:\\Users\\Bill Martin\\Desktop\\testing\\49GALNYNT_test5_change_usfm_text_and_length.SFM"),
					_T("C:\\Users\\Bill Martin\\Desktop\\testing\\49GALNYNT_test2_change_text_only.SFM"),
					_T("C:\\Users\\Bill Martin\\Desktop\\testing\\49GALNYNT_test1_change_usfm_only.SFM"),
					_T("C:\\Users\\Bill Martin\\Desktop\\testing\\49GALNYNT_test3_change_both_usfm_and_text.SFM"),
					_T("C:\\Users\\Bill Martin\\Desktop\\testing\\49GALNYNT_test4_no_changes.SFM")
					};
	for (testNum = 0; testNum < totTestNum; testNum++)
	{
		wxASSERT(::wxFileExists(testPathAndName[testNum]));
		wxFile f_test_tgt(testPathAndName[testNum],wxFile::read);
		wxFileOffset fileLenTest;
		fileLenTest = f_test_tgt.Length();
		// read the raw byte data into pByteBuf (char buffer on the heap)
		char* pTestByteBuf = (char*)malloc(fileLenTest + 1);
		memset(pTestByteBuf,0,fileLenTest + 1); // fill with nulls
		f_test_tgt.Read(pTestByteBuf,fileLenTest);
		wxASSERT(pTestByteBuf[fileLenTest] == '\0'); // should end in NULL
		f_test_tgt.Close();
		wxString testWholeBookBuffer = wxString(pTestByteBuf,wxConvUTF8,fileLenTest);
		// Note: the wxConvUTF8 parameter above works UNICODE builds and does nothing
		// in ANSI builds so this should work for both ANSI and Unicode data.
		free((void*)pTestByteBuf);
		
		wxArrayString testLineArray;
		testLineArray = GetUsfmStructureAndExtent(testWholeBookBuffer);
		CompareUsfmTexts compUsfmTextType;
		compUsfmTextType = CompareUsfmTextStructureAndExtent(baseLineArray, testLineArray);
		switch (compUsfmTextType)
		{
			 
		case noDifferences:
			 {
				wxLogDebug(_T("compUsfmTextType = noDifferences"));
				break;
			 }
		case usfmOnlyDiffers:
			{
				wxLogDebug(_T("compUsfmTextType = usfmOnlyDiffers"));
				break;
			}
		case textOnlyDiffers:
			{
				wxLogDebug(_T("compUsfmTextType = textOnlyDiffers"));
				break;
			}
		case usfmAndTextDiffer:
			{
				wxLogDebug(_T("compUsfmTextType = usfmAndTextDiffer"));
				break;
			}
		}
	}
	// =====================================================================================
	// !!!! whm testing above !!!!
	*/ 

	// Note: The sourceWholeBookBuffer and targetWholeBookBuffer will not be completely empty even
	// if no Paratext book yet exists, because there will be a FEFF UTF-16 BOM char in it
	// after rdwrtp7.exe tries to copy the file and the result is stored in the wxString
	// buffer. So, we can tell better whether the book hasn't been created within Paratext
	// by checking to see if there are any elements in the appropriate 
	// UsfmStructureAndExtentArrays.
	if (SourceTextUsfmStructureAndExtentArray.GetCount() == 0)
	{
		wxString msg1,msg2;
		if (m_pApp->m_collaborationEditor == _T("Paratext"))
		{
			msg1 = msg1.Format(_("The book %s in the Paratext project for obtaining source texts (%s) has no chapter and verse numbers."),fullBookName.c_str(),sourceProjShortName.c_str());
			msg2 = msg2.Format(_("Please run Paratext and select the %s project. Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and ensure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again."),sourceProjShortName.c_str());
		}
		else if (m_pApp->m_collaborationEditor == _T("Bibledit"))
		{
			msg1 = msg1.Format(_("The book %s in the Bibledit project for obtaining source texts (%s) has no chapter and verse numbers."),fullBookName.c_str(),sourceProjShortName.c_str());
			msg2 = msg2.Format(_("Please run Bibledit and select the %s project. Select File | Project | Properties. Then select \"Templates+\" from the Project properties dialog. Choose the book(s) to be created and click OK. Then return to Adapt It and try again."),sourceProjShortName.c_str());
		}
		msg1 = msg1 + _T(' ') + msg2;
		wxMessageBox(msg1,_("No chapters and verses found"),wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
		pListBoxBookNames->SetSelection(-1); // remove any selection
		pBtnCancel->SetFocus();
		// clear lists and static text box at bottom of dialog
		pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		pStaticSelectAChapter->SetLabel(_("Select a &chapter:"));
		pStaticSelectAChapter->Refresh();
		return;
	}

	pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
	
	wxArrayString chapterListFromSourceBook;
	wxArrayString chapterStatusFromSourceBook;
	bool bBookIsEmpty = FALSE; // assume book has some content. This will be modified by GetChapterListAndVerseStatusFromBook() below
	GetChapterListAndVerseStatusFromBook(collabSrcText,
		SourceTextUsfmStructureAndExtentArray,
		m_TempCollabProjectForSourceInputs,
		fullBookName,
		m_staticBoxSourceDescriptionArray,
		chapterListFromSourceBook,
		chapterStatusFromSourceBook,
		bBookIsEmpty); // whm 19Jul12 note: The bBookIsEmpty should be FALSE for a source text project book
	
	// If this source text book is "empty" notify user
	if (bBookIsEmpty)
	{
		// remove info fields before dialog appears
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		pStaticSelectAChapter->SetLabel(_("Select a &chapter:"));
		pStaticSelectAChapter->Refresh();
		
		// This book in the source project is "empty" of content. Do not allow this
		// book to be selected for obtaining source texts, since no viable source text
		// can be obtained for adaptation from this book.
		wxString msg = _("The %s book %s has no adaptable verse content, therefore it cannot be used as source text for Adapt It.\n\nAdapt It cannot use such \"empty\" books for obtaining source texts.");
		msg = msg.Format(msg,m_pApp->m_collaborationEditor.c_str(),fullBookName.c_str());
		wxString msgTitle = _("No verse text usable as source texts in this book!");
		wxString msg2 = _T("\n\n");
		msg2 += _("Please select a different book, or use %s to import content into %s that is usable as source texts, then try again.");
		msg2 = msg2.Format(msg2,m_pApp->m_collaborationEditor.c_str(),fullBookName.c_str());
		msg += msg2;
		wxMessageBox(msg,msgTitle,wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
		return;
	}

	if (TargetTextUsfmStructureAndExtentArray.GetCount() == 0)
	{
		wxString msg1,msg2;
		if (m_pApp->m_collaborationEditor == _T("Paratext"))
		{
			msg1 = msg1.Format(_("The book %s in the Paratext project for storing translation drafts (%s) has no chapter and verse numbers."),fullBookName.c_str(),targetProjShortName.c_str());
			msg2 = msg2.Format(_("Please run Paratext and select the %s project. Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and ensure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again."),targetProjShortName.c_str());
		}
		else if (m_pApp->m_collaborationEditor == _T("Bibledit"))
		{
			msg1 = msg1.Format(_("The book %s in the Bibledit project for storing translation drafts (%s) has no chapter and verse numbers."),fullBookName.c_str(),targetProjShortName.c_str());
			msg2 = msg2.Format(_("Please run Bibledit and select the %s project. Select File | Project | Properties. Then select \"Templates+\" from the Project properties dialog. Choose the book(s) to be created and click OK. Then return to Adapt It and try again."),targetProjShortName.c_str());
		}
		msg1 = msg1 + _T(' ') + msg2;
		wxMessageBox(msg1,_("No chapters and verses found"),wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
		pListBoxBookNames->SetSelection(-1); // remove any selection
		pBtnCancel->SetFocus();
		// clear lists and static text box at bottom of dialog
		//pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		pStaticSelectAChapter->SetLabel(_("Select a &chapter:"));
		pStaticSelectAChapter->Refresh();
		return;
	}

	// whm added 6Feb12
	if (m_bTempCollaborationExpectsFreeTrans && FreeTransTextUsfmStructureAndExtentArray.GetCount() == 0)
	{
		wxString msg1,msg2;
		if (m_pApp->m_collaborationEditor == _T("Paratext"))
		{
			msg1 = msg1.Format(_("The book %s in the Paratext project for storing free translations (%s) has no chapter and verse numbers."),fullBookName.c_str(),freeTransProjShortName.c_str());
			msg2 = msg2.Format(_("Please run Paratext and select the %s project. Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and ensure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again."),freeTransProjShortName.c_str());
		}
		else if (m_pApp->m_collaborationEditor == _T("Bibledit"))
		{
			msg1 = msg1.Format(_("The book %s in the Bibledit project for storing free translations (%s) has no chapter and verse numbers."),fullBookName.c_str(),freeTransProjShortName.c_str());
			msg2 = msg2.Format(_("Please run Bibledit and select the %s project. Select File | Project | Properties. Then select \"Templates+\" from the Project properties dialog. Choose the book(s) to be created and click OK. Then return to Adapt It and try again."),freeTransProjShortName.c_str());
		}
		msg1 = msg1 + _T(' ') + msg2;
		wxMessageBox(msg1,_("No chapters and verses found"),wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
		pListBoxBookNames->SetSelection(-1); // remove any selection
		pBtnCancel->SetFocus();
		// clear lists and static text box at bottom of dialog
		//pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		pStaticSelectAChapter->SetLabel(_("Select a &chapter:"));
		pStaticSelectAChapter->Refresh();
		return;
	}
	/*
	pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
	
	wxArrayString chapterListFromSourceBook;
	wxArrayString chapterStatusFromSourceBook;
	bool bBookIsEmpty = FALSE; // assume book has some content. This will be modified by GetChapterListAndVerseStatusFromBook() below
	GetChapterListAndVerseStatusFromBook(collabSrcText,
		SourceTextUsfmStructureAndExtentArray,
		m_TempCollabProjectForSourceInputs,
		fullBookName,
		m_staticBoxSourceDescriptionArray,
		chapterListFromSourceBook,
		chapterStatusFromSourceBook,
		bBookIsEmpty); // whm 19Jul12 note: The bBookIsEmpty should be FALSE for a source text project book
	
	// If this source text book is "empty" notify user
	if (bBookIsEmpty)
	{
		// This book in the source project is "empty" of content. Do not allow this
		// book to be selected for obtaining source texts, since no viable source text
		// can be obtained for adaptation from this book.
		wxString msg = _("The book %s has no adaptable verse content, therefore it cannot be used as source text for Adapt It. The book may have chapter and verse markers (\\c and \\v) but it has no actual source text. Adapt It cannot use \"empty\" books for obtaining source texts.");
		msg = msg.Format(msg,fullBookName.c_str());
		wxString msgTitle = _("No verse text usable as source texts in this book!");
		wxString msg2 = _T("\n\n");
		msg2 += _("Please select a different book that has content that is usable as source texts.");
		msg += msg2;
		wxMessageBox(msg,msgTitle,wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		pStaticSelectAChapter->SetLabel(_("Select a &chapter:"));
		pStaticSelectAChapter->Refresh();
		return;
	}
	*/

	wxArrayString chapterListFromTargetBook;
	wxArrayString chapterStatusFromTargetBook;
	bBookIsEmpty = FALSE; // assume book has some content. This will be modified by GetChapterListAndVerseStatusFromBook() below
	GetChapterListAndVerseStatusFromBook(collabTgtText,
		TargetTextUsfmStructureAndExtentArray,
		m_TempCollabProjectForTargetExports,
		fullBookName,
		m_staticBoxTargetDescriptionArray,
		chapterListFromTargetBook,
		chapterStatusFromTargetBook,
		bBookIsEmpty); // whm 19Jul12 note: The bBookIsEmpty is usually TRUE for the initial state of a target text project
	
	if (m_bTempCollaborationExpectsFreeTrans)
	{
		wxArrayString chapterListFromFreeTransBook;
		wxArrayString chapterStatusFromFreeTransBook;
		bBookIsEmpty = FALSE; // assume book has some content. This will be modified by GetChapterListAndVerseStatusFromBook() below
		// BEW 24Jun13 - Bill cloned the src txt call, and forgot to change 1st param from
		// collabSrcText to collabFreeTransText
		//GetChapterListAndVerseStatusFromBook(collabSrcText,
		GetChapterListAndVerseStatusFromBook(collabFreeTransText,
			FreeTransTextUsfmStructureAndExtentArray,
			m_TempCollabProjectForFreeTransExports,
			fullBookName,
			m_staticBoxFreeTransDescriptionArray,
			chapterListFromFreeTransBook,
			chapterStatusFromFreeTransBook,
			bBookIsEmpty); // whm 19Jul12 note: The bBookIsEmpty is usually TRUE for the initial state of a free trans text project
		// TODO: Add some checks for the Free Trans here
	}

	if (chapterListFromTargetBook.GetCount() == 0)
	{
		wxString msg1,msg2;
		if (m_pApp->m_collaborationEditor == _T("Paratext"))
		{
			msg1 = msg1.Format(_("The book %s in the Paratext project for storing translation drafts (%s) has no chapter and verse numbers."),fullBookName.c_str(),targetProjShortName.c_str());
			msg2 = msg2.Format(_("Please run Paratext and select the %s project. Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and ensure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again."),targetProjShortName.c_str());
		}
		else if (m_pApp->m_collaborationEditor == _T("Bibledit"))
		{
			msg1 = msg1.Format(_("The book %s in the Bibledit project for storing translation drafts (%s) has no chapter and verse numbers."),fullBookName.c_str(),targetProjShortName.c_str());
			msg2 = msg2.Format(_("Please run Bibledit and select the %s project. Select File | Project | Properties. Then select \"Templates+\" from the Project properties dialog. Choose the book(s) to be created and click OK. Then return to Adapt It and try again."),targetProjShortName.c_str());
		}
		msg1 = msg1 + _T(' ') + msg2;
		wxMessageBox(msg1,_("No chapters and verses found"),wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
		pListCtrlChapterNumberAndStatus->InsertItem(0,_("No chapters and verses found")); //pListCtrlChapterNumberAndStatus->Append(_("No chapters and verses found"));
		pListCtrlChapterNumberAndStatus->Enable(FALSE);
		pBtnCancel->SetFocus();
		// clear lists and static text box at bottom of dialog
		//pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		pStaticSelectAChapter->SetLabel(_("Select a &chapter:"));
		pStaticSelectAChapter->Refresh();
		return;
	}
	else
	{
		// get extent for the current book name for sizing first column
		wxSize sizeOfBookNameAndCh,sizeOfFirstColHeader;
		wxClientDC aDC(this);
		wxFont tempFont = this->GetFont();
		aDC.SetFont(tempFont);
		wxString lastCh = chapterListFromTargetBook.Last();
		aDC.GetTextExtent(lastCh,&sizeOfBookNameAndCh.x,&sizeOfBookNameAndCh.y);
		aDC.GetTextExtent(pTheFirstColumn->GetText(),&sizeOfFirstColHeader.x,&sizeOfFirstColHeader.y);
		if (sizeOfBookNameAndCh.GetX() > sizeOfFirstColHeader.GetX())
			pTheFirstColumn->SetWidth(sizeOfBookNameAndCh.GetX() + 30); // 30 fudge factor
		else
			pTheFirstColumn->SetWidth(sizeOfFirstColHeader.GetX() + 30); // 30 fudge factor
		
		int height,widthListCtrl,widthCol1;
		pListCtrlChapterNumberAndStatus->GetClientSize(&widthListCtrl,&height);
		widthCol1 = pTheFirstColumn->GetWidth();
		pTheSecondColumn->SetWidth(widthListCtrl - widthCol1);
		
		pListCtrlChapterNumberAndStatus->InsertColumn(0, *pTheFirstColumn);
		pListCtrlChapterNumberAndStatus->InsertColumn(1, *pTheSecondColumn);
		
		int totItems = chapterListFromTargetBook.GetCount();
		wxASSERT(totItems == (int)chapterStatusFromTargetBook.GetCount());
		int ct;
		for (ct = 0; ct < totItems; ct++)
		{
			long tmp = pListCtrlChapterNumberAndStatus->InsertItem(ct,chapterListFromTargetBook.Item(ct));
			pListCtrlChapterNumberAndStatus->SetItemData(tmp,ct);
			pListCtrlChapterNumberAndStatus->SetItem(tmp,1,chapterStatusFromTargetBook.Item(ct));
		}
		pListCtrlChapterNumberAndStatus->Enable(TRUE);
		pListCtrlChapterNumberAndStatus->Show();
	}
	pStaticSelectAChapter->SetLabel(_("Select a &chapter:")); // put & char at same position as in the string in wxDesigner
	pStaticSelectAChapter->Refresh();

	if (!m_TempCollabChapterSelected.IsEmpty())
	{
		int nSel = -1;
		int ct;
		wxString tempStr;
		bool bFound = FALSE;
		for (ct = 0; ct < (int)pListCtrlChapterNumberAndStatus->GetItemCount();ct++)
		{
			tempStr = pListCtrlChapterNumberAndStatus->GetItemText(ct);
			tempStr.Trim(FALSE);
			tempStr.Trim(TRUE);
			if (tempStr.Find(fullBookName) == 0)
			{
				tempStr = GetBareChFromLBChSelection(tempStr);
				if (tempStr == m_TempCollabChapterSelected)
				{
					bFound = TRUE;
					nSel = ct;
					break;
				}
			}
		}

		if (bFound)
		{
			pListCtrlChapterNumberAndStatus->Select(nSel,TRUE);
			//pListCtrlChapterNumberAndStatus->SetFocus(); // better to set focus on OK button (see below)
			// Update the wxTextCtrl at the bottom of the dialog with more detailed
			// info about the book and/or chapter that is selected. 
			wxString noteStrToDisplay = m_staticBoxTargetDescriptionArray.Item(nSel);
			if (m_bTempCollaborationExpectsFreeTrans)
			{
				noteStrToDisplay += _T(' ');
				noteStrToDisplay += m_staticBoxFreeTransDescriptionArray.Item(nSel); // TODO: check that arrays have same count???
			}
			pStaticTextCtrlNote->ChangeValue(noteStrToDisplay);
			// whm added 29Jul11.
			// Set focus to the OK button since we have both a book and chapter selected
			// so if the user wants to continue work in the same book and chapter s/he
			// can simply press return.
			pBtnOK->SetFocus();
		}
		else
		{
			// Update the wxTextCtrl at the bottom of the dialog with more detailed
			// info about the book and/or chapter that is selected. In this case we can
			// just remind the user to select a chapter.
			pStaticTextCtrlNote->ChangeValue(_T("Please select a chapter in the list at right."));
			// whm added 29Jul11.
			// Set focus to the chapter list
			pListCtrlChapterNumberAndStatus->SetFocus();
		}
	}
	
	wxASSERT(pListBoxBookNames->GetSelection() != wxNOT_FOUND);
	m_TempCollabBookSelected = pListBoxBookNames->GetStringSelection();
	
	// whm added 27Jul11. Adjust the interface to Whole Book mode
	if (!m_bTempCollabByChapterOnly)
	{ 
		// For Whole Book mode we need to:
		// 1. Remove the chapter list's selection and disable it
		// 2. Change its listbox static text heading from "Select a chapter:" 
		//    to "Chapter status of selected book:"
		// 3. Change the informational text in the bottom edit box to something
		//    informative (see TODO: below)
		// 4. Empty the m_TempCollabChapterSelected variable to signal to
		//    other methods that no chapter is selected
		int itemCt;
		itemCt = pListCtrlChapterNumberAndStatus->GetSelectedItemCount();
		itemCt = itemCt; // avoid warning
		long nSelTemp = pListCtrlChapterNumberAndStatus->GetNextItem(-1, wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
		if (nSelTemp != wxNOT_FOUND)
		{
			pListCtrlChapterNumberAndStatus->Select(nSelTemp,FALSE); // FALSE - means unselect the item
			// whm added 29Jul11 Set focus on the OK button
			pBtnOK->SetFocus();
		}

		//pListCtrlChapterNumberAndStatus->Disable();
		pStaticSelectAChapter->SetLabel(_("Chapter status of selected book:"));
		pStaticSelectAChapter->Refresh();
		wxString noteStr;
		// whm TODO: For now I'm blanking out the text. Eventually we may want to compose 
		// a noteStr that says something about the status of the whole book, i.e., 
		// "All chapters have content", or "The following chapters have verses
		// that do not yet have content: 1, 5, 12-29"
		noteStr = _T("");
		pStaticTextCtrlNote->ChangeValue(noteStr);
		// empty the temp variable that holds the chapter selected
		m_TempCollabChapterSelected.Empty();
	}
}

void CGetSourceTextFromEditorDlg::OnLBChapterSelected(wxListEvent& WXUNUSED(event))
{
	// This handler is called both for single click and double clicks on an
	// item in the chapter list box, since a double click is sensed initially
	// as a single click.
	int itemCt;
	itemCt = pListCtrlChapterNumberAndStatus->GetSelectedItemCount();
	wxASSERT(itemCt <= 1);
	itemCt = itemCt; // avoid warning
	long nSel = pListCtrlChapterNumberAndStatus->GetNextItem(-1, wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
	if (nSel != wxNOT_FOUND)
	{
		// BEW 15Sep14, Do nothing, if "whole book" is the current option
		if (!m_bTempCollabByChapterOnly)
		{
			// Deselect immediately and refresh, so that the user's clicks
			// do nothing, and then return so that nothing is written to
			// the text control below the listctrl. But the scroll bars
			// will still work and the user can check what's there in
			// the book in terms of chapters
			pListCtrlChapterNumberAndStatus->Select(nSel,FALSE); // deselect
			pListCtrlChapterNumberAndStatus->Refresh();
			return; 
		}
		// Continue on, when the "by chapter only" option is in effect
		wxString tempStr = pListCtrlChapterNumberAndStatus->GetItemText(nSel);
		m_TempCollabChapterSelected = GetBareChFromLBChSelection(tempStr);
		// Update the wxTextCtrl at the bottom of the dialog with more detailed
		// info about the book and/or chapter that is selected.
		wxString noteStrToDisplay = m_staticBoxTargetDescriptionArray.Item(nSel);
		if (m_bTempCollaborationExpectsFreeTrans)
		{
			noteStrToDisplay += _T(' ');
			noteStrToDisplay += m_staticBoxFreeTransDescriptionArray.Item(nSel); // TODO: check that arrays have same count???
		}
		wxLogDebug(noteStrToDisplay);
		pStaticTextCtrlNote->ChangeValue(noteStrToDisplay);
		// whm note 29Jul11 We can't set focus on the OK button whenever the 
		// chapter selection changes, because working via keyboard, the up and
		// down button especially will them move the focus from OK to to one of
		// the combo boxes resulting in changing a project!
		//pBtnOK->SetFocus();
	}
	else
	{
		// Update the wxTextCtrl at the bottom of the dialog with more detailed
		// info about the book and/or chapter that is selected. In this case we can
		// just remind the user to select a chapter.
		pStaticTextCtrlNote->ChangeValue(_("Please select a chapter in the list at right."));
	}

    // Bruce felt that we should get a fresh copy of the PT target project's chapter file
    // at this point (but no longer does, BEW 17Jun11). I think it would be better to do so
    // in the OnOK() handler than here, since it is the OnOK() handler that will actually
    // open the chapter text as an AI document (Bruce agrees!)
}

void CGetSourceTextFromEditorDlg::OnLBDblClickChapterSelected(wxCommandEvent& WXUNUSED(event))
{
	wxLogDebug(_T("Chapter list double-clicked"));
	// This should do the same as clicking on OK button, then force the dialog to close
	wxCommandEvent evt(wxID_OK);
	this->OnOK(evt);
	// force parent dialog to close
	EndModal(wxID_OK);
}

void CGetSourceTextFromEditorDlg::OnCancel(wxCommandEvent& event)
{
	pListBoxBookNames->SetSelection(-1); // remove any selection
	// clear lists and static text box at bottom of dialog
	pListBoxBookNames->Clear();
	pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
	pStaticTextCtrlNote->ChangeValue(_T(""));

	// whm 7Jan12 can leave this status message as is
	// update status bar info (BEW added 27Jun11) - copy & tweak from app's OnInit()
	wxStatusBar* pStatusBar = m_pApp->GetMainFrame()->GetStatusBar(); //CStatusBar* pStatusBar;
	if (m_pApp->m_bCollaboratingWithBibledit || m_pApp->m_bCollaboratingWithParatext)
	{
		wxString message;
		message = message.Format(_("Collaborating with %s: getting source text was Cancelled"), 
									m_pApp->m_collaborationEditor.c_str());
		pStatusBar->SetStatusText(message,0); // use first field 0
		m_pApp->LogUserAction(message);
	}

	// whm 19Sep11 modified: 
	// Previously, OnCancel() here emptied the m_pApp->m_curProjectPath. But that I think should
	// not be done. It is not done when the user Cancel's from the Start Working Wizard when
	// not collaboration with PT/BE.
	//if (!m_pApp->m_bCollaboratingWithParatext && !m_pApp->m_bCollaboratingWithBibledit)
	//{
	//	m_pApp->m_curProjectPath.Empty();
	//}
	
	// Restore the App's original collab values before exiting the dialog
	m_pApp->m_CollabProjectForSourceInputs = m_SaveCollabProjectForSourceInputs;
	m_pApp->m_CollabProjectForTargetExports = m_SaveCollabProjectForTargetExports;
	m_pApp->m_CollabProjectForFreeTransExports = m_SaveCollabProjectForFreeTransExports;
	m_pApp->m_CollabAIProjectName = m_SaveCollabAIProjectName;
	m_pApp->m_CollabSourceLangName = m_SaveCollabSourceProjLangName;
	m_pApp->m_CollabTargetLangName = m_SaveCollabTargetProjLangName;
	m_pApp->m_CollabBookSelected = m_SaveCollabBookSelected;
	m_pApp->m_bCollabByChapterOnly = m_bSaveCollabByChapterOnly; // FALSE means the "whole book" option
	m_pApp->m_CollabChapterSelected = m_SaveCollabChapterSelected;
	m_pApp->m_bCollaborationExpectsFreeTrans = m_bSaveCollaborationExpectsFreeTrans;

	event.Skip();
}

EthnologueCodePair*  CGetSourceTextFromEditorDlg::MatchAIProjectUsingEthnologueCodes(
							wxString& editorSrcLangCode, wxString& editorTgtLangCode)
{
    // Note, the editorSrcLangCode and editorTgtLangCode have already been checked for
    // validity in the caller and passed the check, otherwise this function wouldn't be
    // called
	EthnologueCodePair* pPossibleProject = (EthnologueCodePair*)NULL;
	EthnologueCodePair* pMatchedProject = (EthnologueCodePair*)NULL; // we'll return
										// using this one after having set it below
	wxArrayPtrVoid codePairs; // to hold an array of EthnologueCodePair struct pointers
	bool bSuccessful = m_pApp->GetEthnologueLangCodePairsForAIProjects(&codePairs);
	if (!bSuccessful || codePairs.IsEmpty())
	{
		// for either of the above conditions, the count of items in codePairs array will
		// be zero, so no heap instances of EthnologueCodePair have to be deleted here 
		// first -- but then again, safety lies in being conservative...
		int count2 = codePairs.GetCount();
		int index2;
		for(index2 = 0; index2 < count2; index2++)
		{
			EthnologueCodePair* pCP = (EthnologueCodePair*)codePairs.Item(index2);
			if (pCP != NULL) // whm 11Jun12 added NULL test
				delete pCP;
		}
		return (EthnologueCodePair*)NULL;
	}
    // We now have an array of src & tgt ethnologue language code pairs, coupled with the
    // name and path for the associated Adapt It project folder, for all project folders
    // (the code pairs are read from each of the projects' AI-ProjectConfiguration.aic file
    // within each project folder). However, some (legacy) projects may not have any such
    // codes defined, and so would be returned as empty strings; and some codes may be
    // invalid or guesses which are not right, etc - so we must do validation checks. We
    // look for a unique matchup, no matchup or multiple matchups constitute a 'not
    // successful' matchup attempt
    wxArrayPtrVoid successfulMatches;
	int count = codePairs.GetCount();
	wxASSERT(count != 0);
	wxString srcEthCode;
	wxString tgtEthCode;
	int counter = 0;
	int index;
	for (index = 0; index < count; index++)
	{
		pPossibleProject = (EthnologueCodePair*)codePairs.Item(index);
		srcEthCode = pPossibleProject->srcLangCode;
		tgtEthCode = pPossibleProject->tgtLangCode;
		if (!IsEthnologueCodeValid(srcEthCode) || !IsEthnologueCodeValid(tgtEthCode))
		{
			// no matchup is possible, iterate, and don't bump the counter value
			continue;
		}
		else
		{
			// the codes are probably valid ones (if not, it's unlikely the ones coming
			// from Paratext or Bibledit would be invalid and also be perfect string
			// matches, so the odds of getting a false positive in the matchup test to be
			// done in this block are close to zero) Count the positive matches, and store
			// their EthnologueCodePair struct instances' pointers in an array
			if (srcEthCode == editorSrcLangCode && tgtEthCode == editorTgtLangCode)
			{
				// we have a matchup with an existing Adapt It project folder for the
				// language pairs being used in both AI and PT, or AI and BE
				counter++; // count and continue iterating, because there may be more than one match
				successfulMatches.Add(pPossibleProject);
			}
		}
	} // end of for loop
	if (counter != 1)
	{
		// oops, no matches, or too many matches, so we didn't succeed; delete the heap
		// objects before returning
		for (index = 0; index < count; index++)
		{
			pPossibleProject = (EthnologueCodePair*)codePairs.Item(index);
			if (pPossibleProject != NULL) // whm 11Jun12 added NULL test
				delete pPossibleProject;
		}
		return (EthnologueCodePair*)NULL;;
	}
	else
	{
		// Success! A unique matchup...
        // make a copy of the successful EthnologueCodePair which is to be returned, then
        // delete the original ones and return the successful one
		pPossibleProject = (EthnologueCodePair*)successfulMatches.Item(0); // the one and only successful match
		pMatchedProject = new EthnologueCodePair;
		// copy the values to the new instance we are going to return to the caller
		pMatchedProject->projectFolderName = pPossibleProject->projectFolderName;
		pMatchedProject->projectFolderPath = pPossibleProject->projectFolderPath;
		pMatchedProject->srcLangCode = pPossibleProject->srcLangCode;
		pMatchedProject->tgtLangCode = pPossibleProject->tgtLangCode;
		// delete the original ones that are in the codePairs array
		for (index = 0; index < count; index++)
		{
			pPossibleProject = (EthnologueCodePair*)codePairs.Item(index);
			if (pPossibleProject != NULL) // whm 11Jun12 added NULL test
				delete pPossibleProject;
		}
	}
	wxASSERT(pMatchedProject != NULL);
	return pMatchedProject;
}

void CGetSourceTextFromEditorDlg::LoadBookNamesIntoList()
{
	// clear lists and static text box at bottom of dialog
	pListBoxBookNames->Clear();
	pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
	pStaticTextCtrlNote->ChangeValue(_T(""));

	wxString collabProjShortName;
	collabProjShortName = GetShortNameFromProjectName(m_TempCollabProjectForSourceInputs);
	int ct, tot;
	tot = (int)m_pApp->m_pArrayOfCollabProjects->GetCount();
	wxString tempStr;
	Collab_Project_Info_Struct* pArrayItem;
	pArrayItem = (Collab_Project_Info_Struct*)NULL;
	bool bFound = FALSE;
	for (ct = 0; ct < tot; ct++)
	{
		pArrayItem = (Collab_Project_Info_Struct*)(*m_pApp->m_pArrayOfCollabProjects)[ct];
		tempStr = pArrayItem->shortName;
		if (tempStr == collabProjShortName)
		{
			bFound = TRUE;
			break;
		}
	}

	// If we get here, the projects for source inputs and translation exports are selected
	wxString booksStr;
	if (pArrayItem != NULL && bFound)
	{
		booksStr = pArrayItem->booksPresentFlags;
	}
	wxArrayString booksPresentArray;
	booksPresentArray = m_pApp->GetBooksArrayFromBookFlagsString(booksStr);
	if (booksPresentArray.GetCount() == 0)
	{
		wxString msg1,msg2;
		if (m_pApp->m_bCollaboratingWithParatext)
		{
			msg1 = msg1.Format(_("The Paratext project (%s) selected for obtaining source texts contains no books."),m_TempCollabProjectForSourceInputs.c_str());
			msg2 = _("Please select the Paratext Project that contains the source texts you will use for adaptation work.");
		}
		else if (m_pApp->m_bCollaboratingWithBibledit)
		{
			msg1 = msg1.Format(_("The Bibledit project (%s) selected for obtaining source texts contains no books."),m_TempCollabProjectForSourceInputs.c_str());
			msg2 = _("Please select the Bibledit Project that contains the source texts you will use for adaptation work.");
		}
		msg1 = msg1 + _T(' ') + msg2;
		wxMessageBox(msg1,_T("No books found"),wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
		// clear lists and static text box at bottom of dialog
		pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
	}
	pListBoxBookNames->Append(booksPresentArray);

}

wxString CGetSourceTextFromEditorDlg::GetBareChFromLBChSelection(wxString lbChapterSelected)
{
	// whm adjusted 27Jul11.
    // If we are dealing with the "Get Chapter Only" option, the incoming lbChapterSelected
    // is from the "Select a chapter" list and is of the form:
    // "Acts 2 - Empty(no verses of a total of n have content yet)" However, if we are
    // dealing with the "Get Whole Book" option, the incoming lbChapterSelected will be an
    // empty string, because once the whole book option is requested the user is unable to
    // make a selection in the Chapters list. In this case we return the string value
	// _T("0") -- the caller will use the "0" value for indicating a book is wanted, when
	// the command line is built
	if (lbChapterSelected.IsEmpty())
		return _T("0");
	// from this point on we are dealing with chapter only
	wxString chStr;
	int posHyphen = lbChapterSelected.Find(_T('-'));
	if (posHyphen != wxNOT_FOUND)
	{
		chStr = lbChapterSelected.Mid(0,posHyphen);
		chStr.Trim(FALSE);
		chStr.Trim(TRUE);
		int posSp = chStr.Find(_T(' '),TRUE); // TRUE - find from right end
		if (posSp != wxNOT_FOUND)
		{
			chStr = chStr.Mid(posSp);
			chStr.Trim(FALSE);
			chStr.Trim(TRUE);
		}
	}
	else
	{
		// no hyphen in the incoming string
		chStr = lbChapterSelected;
		chStr.Trim(FALSE);
		chStr.Trim(TRUE);
		int posSp = chStr.Find(_T(' '),TRUE); // TRUE - find from right end
		if (posSp != wxNOT_FOUND)
		{
			chStr = chStr.Mid(posSp);
			chStr.Trim(FALSE);
			chStr.Trim(TRUE);
		}
	}

	return chStr;
}



