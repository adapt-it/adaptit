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
//#include "ChangeCollabProjectsDlg.h"

extern const wxString createNewProjectInstead;
//extern wxSizer *pNewNamesSizer; // created in wxDesigner's GetSourceTextFromEditorDlgFunc()

extern wxChar gSFescapechar; // the escape char used for start of a standard format marker
extern bool gbIsGlossing;
extern bool gbGlossingUsesNavFont;
extern int gnOldSequNum;
extern CAdapt_ItApp* gpApp;
extern wxString szProjectConfiguration;

// event handler table
BEGIN_EVENT_TABLE(CGetSourceTextFromEditorDlg, AIModalDialog)
	EVT_INIT_DIALOG(CGetSourceTextFromEditorDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CGetSourceTextFromEditorDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, CGetSourceTextFromEditorDlg::OnCancel)
	//EVT_BUTTON(ID_BUTTON_CHANGE_PROJECTS, CGetSourceTextFromEditorDlg::OnBtnChangeProjects)
	EVT_LISTBOX(ID_LISTBOX_BOOK_NAMES, CGetSourceTextFromEditorDlg::OnLBBookSelected)
	EVT_LIST_ITEM_SELECTED(ID_LISTCTRL_CHAPTER_NUMBER_AND_STATUS, CGetSourceTextFromEditorDlg::OnLBChapterSelected)
	EVT_LISTBOX_DCLICK(ID_LISTCTRL_CHAPTER_NUMBER_AND_STATUS, CGetSourceTextFromEditorDlg::OnLBDblClickChapterSelected)
	//EVT_RADIOBOX(ID_RADIOBOX_WHOLE_BOOK_OR_CHAPTER, CGetSourceTextFromEditorDlg::OnRadioBoxSelected)
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
	
	//pRadioBoxChapterOrBook = (wxRadioBox*)FindWindowById(ID_RADIOBOX_WHOLE_BOOK_OR_CHAPTER);
	//wxASSERT(pRadioBoxChapterOrBook != NULL);

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

	//pBtnChangeProjects = (wxButton*)FindWindowById(ID_BUTTON_CHANGE_PROJECTS);
	//wxASSERT(pBtnChangeProjects != NULL);
	
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
	if (pTheFirstColumn != NULL) // whm 11Jun12 added NULL test
		delete pTheFirstColumn;
	if (pTheSecondColumn != NULL) // whm 11Jun12 added NULL test
		delete pTheSecondColumn;
}

void CGetSourceTextFromEditorDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

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
	m_TempCollabSourceProjLangName = m_pApp->m_CollabSourceLangName; // whm added 4Sep11
	m_TempCollabTargetProjLangName = m_pApp->m_CollabTargetLangName; // whm added 4Sep11
	m_TempCollabBookSelected = m_pApp->m_CollabBookSelected;
	m_bTempCollabByChapterOnly = m_pApp->m_bCollabByChapterOnly;
	m_TempCollabChapterSelected = m_pApp->m_CollabChapterSelected;
	m_bTempCollaborationExpectsFreeTrans = m_pApp->m_bCollaborationExpectsFreeTrans; // whm added 6Jul11

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
	
	/*
	// TODO: Revise below to remove most of the sanity checks from here since they have been done
	// in the GetAIProjectCollabStatus() function in the ProjectPage's OnWizardPageChanging() method.
	// 
	// Normally, the projects in PT/BE will have been set up by the administrator, and the 
	// project existence will pass the following validation checks. However, it is
	// possible that the user could have removed/changed projects in PT/BE since the last Adapt It
	// session, so we have to do sanity checking for the necessary projects on each invocation of
	// the GetSourceTextFromEditor dialog. 
	//bool bProjectsOK = TRUE;
	bool bSourceProjRequiredButNotFound = TRUE;
	bool bTargetProjRequiredButNotFound = TRUE;
	bool bFreeTransProjRequiredButNotFound = FALSE;

	wxASSERT(m_pApp->m_bCollaboratingWithParatext || m_pApp->m_bCollaboratingWithBibledit);
	
	// get list of PT/BE projects
	projList.Clear();
	if (m_pApp->m_collaborationEditor == _T("Paratext"))
	{
		projList = m_pApp->GetListOfPTProjects(); // as a side effect, it populates the App's m_pArrayOfCollabProjects
	}
	else if (m_pApp->m_collaborationEditor == _T("Bibledit"))
	{
		projList = m_pApp->GetListOfBEProjects(); // as a side effect, it populates the App's m_pArrayOfCollabProjects
	}
	bool bTwoOrMoreProjectsInList = TRUE;
	int nProjCount;
	nProjCount = (int)projList.GetCount();
	if (nProjCount < 2)
	{
		// Less than two PT/BE projects are defined. For AI-PT/BE collaboration to be possible 
		// at least two PT/BE projects must be defined - one for source text inputs and 
		// another for target exports.
		// Notify the user that Adapt It - Paratext/Bibledit collaboration cannot proceed 
		// until the administrator sets up the necessary projects within Paratext/Bibledit.
		bTwoOrMoreProjectsInList = FALSE;
	}
	*/

	// whm revised 1Mar12 at Bruce's request to use the whole composite string in 
	// the case of Paratext.
	// whm Note: GetAILangNamesFromAIProjectNames() below issues error message if the
	// m_TempCollabAIProjectName is mal-formed (empty, or has no " to " or " adaptations")
	pSrcProj->SetLabel(m_TempCollabProjectForSourceInputs);
	pTgtProj->SetLabel(m_TempCollabProjectForTargetExports);
	pFreeTransProj->SetLabel(m_TempCollabProjectForFreeTransExports);
	
	pUsingAIProjectName->SetLabel(m_TempCollabAIProjectName); // whm added 28Jan12

	/*
	// Confirm that we can find the active source and target projects as stored in the 
	// config file and now stored in our projList.
	// whm 19Apr12 modified: The source text is required for collaboration. If the project config 
	// file lists a project string for its CollabProjectForSourceInputs field (and 
	// therefore m_TempCollabProjecForSourceInputs is not empty), we check to ensure
	// that short project names match. If not, bSourceProjReuiredButNotFound is set
	// FALSE.
	if (!m_TempCollabProjectForSourceInputs.IsEmpty())
	{
		int ct;
		for (ct = 0; ct < (int)projList.GetCount(); ct++)
		{
			// Do the comparison only on the projects' short names which must be unique
			if (GetShortNameFromProjectName(projList.Item(ct)) == GetShortNameFromProjectName(m_TempCollabProjectForSourceInputs))
				bSourceProjRequiredButNotFound = FALSE;
		}
	}
	// whm 19Apr12 modified: The target text is rquired for collaboration. If the project config 
	// file lists a project string for its CollabProjectForTargetExports field (and 
	// therefore m_TempCollabProjectForFreeTransExports is not empty), we check to ensure
	// that short project names match. If not, bFreeTransProjRequiredButNotFound is set
	// FALSE.
	if (!m_TempCollabProjectForTargetExports.IsEmpty())
	{
		int ct;
		for (ct = 0; ct < (int)projList.GetCount(); ct++)
		{
			if (GetShortNameFromProjectName(projList.Item(ct)) == GetShortNameFromProjectName(m_TempCollabProjectForTargetExports))
				bTargetProjRequiredButNotFound = FALSE;
		}
	}

	// whm 19Apr12 modified: The free translation is optional. If the project config 
	// file lists a project string for its CollabProjectForFreeTransExports field (and 
	// therefore m_TempCollabProjectForFreeTransExports is not empty), we check to ensure
	// that short project names match. If not, bFreeTransProjRequiredButNotFound is set
	// FALSE.
	if (!m_TempCollabProjectForFreeTransExports.IsEmpty())
	{
		bFreeTransProjRequiredButNotFound = TRUE;
		int ct;
		for (ct = 0; ct < (int)projList.GetCount(); ct++)
		{
			if (GetShortNameFromProjectName(projList.Item(ct)) == GetShortNameFromProjectName(m_TempCollabProjectForFreeTransExports))
				bFreeTransProjRequiredButNotFound = FALSE;
		}
	}
	else
	{
		bFreeTransProjRequiredButNotFound = FALSE;
	}

	if (!bTwoOrMoreProjectsInList)
	{
		// This error is not likely to happen so use English message
		wxString str;
		if (m_pApp->m_bCollaboratingWithParatext)
		{
			str = _T("Your administrator has configured Adapt It to collaborate with Paratext.\nBut Paratext does not have at least two projects available for use by Adapt It.\nPlease ask your administrator to set up the necessary Paratext projects.");
			wxMessageBox(str, _T("Not enough Paratext projects defined for collaboration"), wxICON_ERROR | wxOK, this); // whm 28Nov12 added this as parent);
			m_pApp->LogUserAction(_T("PT Collaboration activated but less than two PT projects listed."));
		}
		else if (m_pApp->m_bCollaboratingWithBibledit)
		{
			str = _T("Your administrator has configured Adapt It to collaborate with Bibledit.\nBut Bibledit does not have at least two projects available for use by Adapt It.\nPlease ask your administrator to set up the necessary Bibledit projects.");
			wxMessageBox(str, _T("Not enough Bibledit projects defined for collaboration"), wxICON_ERROR | wxOK, this); // whm 28Nov12 added this as parent);
			m_pApp->LogUserAction(_T("BE Collaboration activated but less than two BE projects listed."));
		}
		// whm modified 25Jan12. Calling wxKill() on the current process is a quiet way to terminate.
		// whm changed 22Mar12. Leave the app running so an administrator can look at it.
		//wxKill(::wxGetProcessId(),wxSIGKILL); // abort();
		//return;
		// whm modified 19Apr12. It is not adequate to simply call return at this point since return from
		// InitDialog() does not stop the dialog from appearing. We need to call this dialog's EndModal()
		// method to destroy the dialog. We also need to set the m_bStartWorkUsingCollaboration flag to FALSE
		// to prevent the DoStartWorkingWizard() function in the App from calling up the 
		// CGetSourceTextFromEditorDlg again.
		m_pApp->m_bStartWorkUsingCollaboration = FALSE;
		this->EndModal(wxID_ABORT); // causes an assert because at the point in InitDialog() the dialog
		// is not yet fully "modal" but the assert won't be seen in release version, and at least doing it
		// this way provides an abort message that pops up over the dialog (see the else if (dlgResult == wxID_ABORT)
		// block in the App's DoStartWorkingWizard() where the message is generated.
		return; // return here prevents the dialog from loading book names etc below
	}

	wxString strProjectNotSel;
	strProjectNotSel.Empty();

	if (bSourceProjRequiredButNotFound)
	{
		strProjectNotSel += _T("\n   ");
		strProjectNotSel += _("Designate a project to use for obtaining source text inputs");
	}
	if (bTargetProjRequiredButNotFound)
	{
		strProjectNotSel += _T("\n   ");
		// BEW 16Jun11 changed "Texts" to "Drafts" in line with email discussion where we
		// agreed to use 'draft' or 'translation draft' instead of 'translation' so as to
		// avoid criticism for claiming to be a translation app, rather than a drafting app
		strProjectNotSel += _("Designate a project to use for Transferring Translation Drafts");
	}
	
	if (bFreeTransProjRequiredButNotFound)
	{
		strProjectNotSel += _T("\n   ");
		strProjectNotSel += _("Designate a project to use for Transferring Free Translations");
	}
	
	if (bSourceProjRequiredButNotFound || bTargetProjRequiredButNotFound || bFreeTransProjRequiredButNotFound)
	{
		wxString str;
		str = str.Format(_("Your administrator needs to setup Adapt It and %s as follows before you can begin working:%s"),m_collabEditorName.c_str(),strProjectNotSel.c_str());
		wxMessageBox(str, _("Collaboration setup required by administrator"), wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
		m_pApp->LogUserAction(str);
		// whm modified 25Jan12. Calling wxKill() on the current process is a quiet way to terminate.
		// whm changed 22Mar12. Leave the app running so an administrator can look at it.
		//wxKill(::wxGetProcessId(),wxSIGKILL); // abort();
		//return;
		// whm modified 19Apr12. It is not adequate to simply call return at this point since return from
		// InitDialog() does not stop the dialog from appearing. We need to call this dialog's EndModal()
		// method to destroy the dialog. We also need to set the m_bStartWorkUsingCollaboration flag to FALSE
		// to prevent the DoStartWorkingWizard() function in the App from calling up the 
		// CGetSourceTextFromEditorDlg again.
		m_pApp->m_bStartWorkUsingCollaboration = FALSE;
		this->EndModal(wxID_ABORT); // causes an assert because at the point in InitDialog() the dialog
		// is not yet fully "modal" but the assert won't be seen in release version, and at least doing it
		// this way provides an abort message that pops up over the dialog (see the else if (dlgResult == wxID_ABORT)
		// block in the App's DoStartWorkingWizard() where the message is generated.
		return; // return here prevents the dialog from loading book names etc below
	}
	*/

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

	CAdapt_ItView* pView = m_pApp->GetView();

	// BEW added 15Aug11, because of the potential for an embedded PlacePhraseBox() call
	// to cause the Choose Translation dialog to open before this OnOK() handler has
	// returned, we set the following boolean which suppresses the call of
	// PlacePhraseBox() until after OnOK() is called. Thereafter, OnIdle() detects the
	// flag is TRUE, and does the required PlacePhraseBox() call, and clears the flag
	m_pApp->bDelay_PlacePhraseBox_Call_Until_Next_OnIdle = TRUE;

	// whm 25Aug11 modified to use a wxProgressDialog rather than the CWaitDlg. I've 
	// started the progress dialog here because the call to UnLoadKBs() below can take 
	// a few seconds for very large KBs. Since there are various potentially time-consuming 
	// steps in the process, each step calling some sort of function, we will simply track 
	// progress as we go through the following steps:
	// 1. UnLoadKBs()
	// 2. TransferTextBetweenAdaptItAndExternalEditor() [for m_bTempCollabByChapterOnly]
	// 3. GetTextFromAbsolutePathAndRemoveBOM()
	//  [if an existing AI project exists]
	//   4. HookUpToExistingAIProject() 
	//      [if an existing AI doc exists in project:]
	//      5. GetTextFromFileInFolder()
	//      6. OpenDocWithMerger()
	//      7. MoveTextToFolderAndSave()
	//      [if no existing AI doc exists in project:]
	//      5. TokenizeTextString()
	//      6. SetupLayoutAndView()
	//      7. MoveTextToFolderAndSave()
	//  [no existing AI project exists]
	//   4. CreateNewAIProject()
	//   5. TokenizeTextString()
	//   6. SetupLayoutAndView()
	//   7. MoveTextToFolderAndSave()
	// 8. ExportTargetText_For_Collab() 
	//    and possibly ExportFreeTransText_For_Collab StoreFreeTransText_PreEdit
	// 9. End of process
	// == for nTotal steps of 9.
	// 
	// Therefore the wxProgressDialog has a maximum value of 9, and the progress indicator
	// gauge increments when each step has been completed.
	// Note: the progress dialog shows the progress of the various steps. With a fast hard
	// drive or cached data and/or a small KB the first 4 steps can happen so quickly that 
	// while the text message in the dialog shows the change of steps, the graphic bars 
	// don't have time to catch up until after step 5. In the middle of the process, the 
	// (same) dialog shows some intermediate activity being "progressed"
	// then resumes showing the progress of the steps until all steps are complete.
	// The wxProgressDialog is designed to not update itself if a particular Update()
	// call occurs within 3 ms of the last one. While this can help avoid showing the
	// dialog or its update for situations that happen to be able to complete quicker than
	// "normal", it also means that sometimes the dialog simply doesn't appear to indicate
	// much visual progress before finishing. I've also sprinkled some ::wxSafeYield()
	// calls to help the dialog display better.
	
	// whm 26Aug11 Open a wxProgressDialog instance here for Restoring KB operations.
	// The dialog's pProgDlg pointer is passed along through various functions that
	// get called in the process.
	// whm WARNING: The maximum range of the wxProgressDialog (nTotal below) cannot
	// be changed after the dialog is created. So any routine that gets passed the
	// pProgDlg pointer, must make sure that value in its Update() function does not 
	// exceed the same maximum value (nTotal).
	wxString msgDisplayed;
	const int nTotal = 9; // we will do up to 9 Steps
	int nStep = 0;
	wxString progMsg;
	if (m_bTempCollabByChapterOnly)
	{
		progMsg = _("Getting the chapter and laying out the document... step %d of %d");
	}
	else
	{
		progMsg = _("Getting the book and laying out the document... step %d of %d");
	}
	msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
	CStatusBar* pStatusBar = NULL;
	pStatusBar = (CStatusBar*)gpApp->GetMainFrame()->m_pStatusBar;
	pStatusBar->StartProgress(_("Getting Document for Collaboration"), msgDisplayed, nTotal);
	
	// Update for step 1 Creating a temporary KB backup for restoring the KB later
	msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
	pStatusBar->UpdateProgress(_("Getting Document for Collaboration"), nStep, msgDisplayed);

	nStep = 1;
	msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
	pStatusBar->UpdateProgress(_("Getting Document for Collaboration"), nStep, msgDisplayed);
	//::wxSafeYield();

	// check the KBs are clobbered, if not so, do so
	UnloadKBs(m_pApp);

	wxString bareChapterSelectedStr;
	int chSel = pListCtrlChapterNumberAndStatus->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
	if (chSel != wxNOT_FOUND)
	{
		chSel++; // the chapter number is one greater than the selected lb index
	}
	else // whm added 27Jul11
	{
		chSel = 0; // indicates we're dealing with Whole Book 
	}
	// if chSel is 0, the string returned to derivedChStr will be _T("0")
	bareChapterSelectedStr.Empty();
	bareChapterSelectedStr << chSel;
	wxString derivedChStr = GetBareChFromLBChSelection(m_TempCollabChapterSelected);
	wxASSERT(bareChapterSelectedStr == derivedChStr);

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
	m_pApp->m_CollabChapterSelected = bareChapterSelectedStr; // use the bare number string rather than the list box's book + ' ' + chnumber
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

	// Set up (or access any existing) project for the selected book or chapter
	// How this is handled will depend on whether the book/chapter selected
	// amounts to an existing AI project or not. The m_bCollabByChapterOnly flag now
	// guides what happens below. The differences for FALSE (the 'whole book' option) are
	// just three: 
	// (1) a filename difference (chapter number is omitted, the rest is the same)
	// (2) the command line for rdwrtp7.exe or equivalent Bibledit executable will have 0
	//     rather than a chapter number
	// (3) storage of the text is done to sourceWholeBookBuffer, rather than
	// sourceChapterBuffer
	
	// whm Note: We don't import or merge a free translation text; BEW added 4Jul11, but
	// we do need to get it so as to be able to compare it later with any changes to the
	// free translation (if, of course, there is one defined in the document) BEW 1Aug11,
	// removed the comment here about conflict resolution dialog- we won't be needing one
	// in our new design, but we do need to get the free translation text if it exists,
	// since our algorithms compare and make changes etc.

	// Get the latest version of the source and target texts from the PT project. We retrieve
	// only texts for the selected chapter. Also get free translation text, if it exists
	// and user wants free translation supported in the transfers of data
	
	// Build the command lines for reading the PT/BE projects using rdwrtp7.exe/bibledit-gtk.
	// whm modified 27Jul11 to pass _T("0") for whole book retrieval on the command-line;
	// get the book or chapter into the .temp folder with a suitable name...
	// TransferTextBetweenAdaptItAndExternalEditor() does it all, and it (and subsequent
	// functions) use the app member values set in the lines just above

	// BEW 1Aug11  ** NOTE **
	// We only need to get the target text, and the free trans text if expected, at the
	// time a File / Save is done. (However, OnLBBookSelected() will, when the user makes
	// his choice of book, automatically get the target text for the whole book, even if
	// he subsequently takes the "Get Chapter Only" radio button option, because we want
	// the whole book's list of chapters and how much work has been done in any to be
	// shown to the user in the dialog -- as an aid to help him make an intelligent
	// choice.)	
	long resultSrc = -1;
	wxArrayString outputSrc, errorsSrc;
	if (m_bTempCollabByChapterOnly)
	{
		nStep = 2;
		msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
		pStatusBar->UpdateProgress(_("Getting Document for Collaboration"), nStep, msgDisplayed);
		
		// get the single-chapter file; the Transfer...Editor() function does all the work
		// of generating the filename, path, commandLine, getting the text, & saving in
		// the .temp folder -- functions for this are in CollabUtilities.cpp
		TransferTextBetweenAdaptItAndExternalEditor(reading, collab_source_text, outputSrc, 
													errorsSrc, resultSrc);
		if (resultSrc != 0)
		{
			// not likely to happen so an English warning will suffice
			wxMessageBox(_(
"Could not read data from the Paratext/Bibledit projects. Please submit a problem report to the Adapt It developers (see the Help menu)."),
			_T(""),wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
			wxString temp;
			temp = temp.Format(_T("PT/BE Collaboration wxExecute returned error. resultSrc = %d "),resultSrc);
			m_pApp->LogUserAction(temp);
			wxLogDebug(temp);
			int ct;
			temp.Empty();
			for (ct = 0; ct < (int)outputSrc.GetCount(); ct++)
			{
				temp += outputSrc.Item(ct);
				m_pApp->LogUserAction(temp);
				wxLogDebug(temp);
			}
			for (ct = 0; ct < (int)errorsSrc.GetCount(); ct++)
			{
				temp += errorsSrc.Item(ct);
				m_pApp->LogUserAction(temp);
				wxLogDebug(temp);
			}
			pListBoxBookNames->SetSelection(-1); // remove any selection
			// clear lists and static text box at bottom of dialog
			pListBoxBookNames->Clear();
			// in next call don't use ClearAll() because it clobbers any columns too
			pListCtrlChapterNumberAndStatus->DeleteAllItems(); 
			pStaticTextCtrlNote->ChangeValue(_T(""));
			m_pApp->bDelay_PlacePhraseBox_Call_Until_Next_OnIdle = FALSE; // restore default value
			pStatusBar->FinishProgress(_("Getting Document for Collaboration"));
			return;
		}
	} // end of TRUE block for test: if (m_pApp->m_bCollabByChapterOnly)
	else
	{
		// Collaboration requested a whole-book file. It's already been done, when the
		// user selected which book he wanted - and if he selects no book, the dialog
		// stays open and a message nags him to select a book. As soon as a book is
		// selected, the selection handler has the whole book grabbed from PT or BE as the
		// case may be, and so we don't need to grab it a second time in this present
		// block. The whole-book file is in the .temp folder
		;
	}

    // whm: after grabbing the chapter source text from the PT project, and before copying
    // it to the appropriate AI project's "__SOURCE_INPUTS" folder, we should check the
    // contents of the __SOURCE_INPUTS folder to see if that source text chapter has been
    // grabbed previously - i.e., which will be the case if it already exists in the AI
    // project's __SOURCE_INPUTS folder. If it already exists, we could:
    // (1) do quick MD5 checksums on the existing chapter text file in the Source Data
    // folder and on the newly grabbed chapter file just grabbed that resides in the .temp
    // folder, using GetFileMD5(). Comparing the two MD5 checksums we quickly know if there
    // has been any changes since we last opened the AI document for that chapter. If there
    // have been changes, we can do a silent automatic merge of the edited source text with
    // the corresponding AI document, or
    // (2) just always do a merge using Bruce's routines in MergeUpdatedSrc.cpp, regardless
    // of whether there has been a change in the source text or not. This may be the
    // preferred option for chapter-sized text documents.
    // *** We need to always do (2), (1) is dangerous because the user may elect not to
    //     send adaptations back to PT (BEW 1Aug11 -- I don't think it would matter, but doing
    //     the merge is about the same level of complexity/processing-time as the checksums,
    //     so I'll leave it that we do the merge automatically), and that can get the saved
    //     source in __SOURCE_INPUTS out of sync with the source text in the CSourcePhrases
    //     of the doc as at an earlier state

	// if free translation transfer is wanted, any free translation grabbed from the
	// nominated 3rd project in PT or BE is to be stored in _FREETRANS_OUTPUTS folder
	
	// now read the tmp files into buffers in preparation for analyzing their chapter and
	// verse status info (1:1:nnnn).
	// Note: The files produced by rdwrtp7.exe for projects with 65001 encoding (UTF-8) have a 
	// UNICODE BOM of ef bb bf (we store BOM-less data, but we convert to UTF-16 first,
	// and remove the utf16 BOM, 0xFF 0xFE, before we store the utf-16 text later below)

	// Explanatory Notes....
	// SourceTempFileName is actually an absolute path to the file, not just a name
	// string, ditto for targetTempFileName etc. Store the target texts (these are pre-edit)
	// in the private app member strings m_sourceChapterBuffer_PreEdit, etc
	// 
	// BEW note on 1Aug11, (1) the *_PreEdit member variables on the app class store the
	// target text and free translation text as they are at the start of a editing session
	// - that is, when the doc is just constituted, and updated after each File / Save.
	// The text in them is generated by doing the appropriate export from the document.
	// (2) There isn't, unlike I previously thought, a 2-text versus 3-text distinction,
	// there is only a 3-text distinction. We need to always store the pre-edit adaptation
	// and free translation (if expected) for comparison purposes, even if they are empty
	// of all but USFMs. (3) We may have to pick up working on an adaptation after some
	// data has already been adapted, so we can't rely totally on the difference between
	// pre-edit and post-edit being zero as indicating that the text to be sent to the
	// external editor at that point doesn't need changing; if the latter is empty, we
	// still have to copy to it at that point in the document. (4) the storage for 
	// pre-edit texts being on the app class mean that the pre-edit versions of the
	// target text and free translation text will disappear if the app is closed down.
	// However, the app will ask for a File / Save if the doc is dirty, and that will
	// (if answered in the affirmative) guarantee the pre-edit versions are used for
	// informing the Save operation, and then they can be thrown away safely (they get
	// updated anyway at that time, if the app isn't closing down). (5) The user changing
	// the USFM structure (e.g introducing markers, or forming bridged verses etc) is a
	// very high cost action - it changes the MD5 checksums even if no words are altered,
	// and so we have to process that situation with much more complex code. (6) No 
	// editing of the source text or USFM structure is permitted in AI during collaboration.
	// Hence, the pre-edit and post-edit versions of the adaptation and free translation
	// can be relied on to have the same USFM structure. However, the from-PT or from_BE
	// adaptation or free translation can have USFM edits done in the external editor, so
	// this possibility (being more complex - see (5) above, has to be dealt with 
	// separately)

    // Get the pre-edit free translation and target text stored in the app's member
    // variables - these pre-edit versions are needed at File / Save time. Also get the
    // from-external-editor grabbed USFM source text into the sourceChapterBuffer, or if
    // whole-book adapting, the sourceWholeBookBuffer; & later below it is transferred to a
    // persistent file in __SOURCE_INPUTS - because a new session will look there for the
    // earlier source text in order to compare with what is coming from the external editor
    // in the new session; this call understands the difference between chapter and
    // wholebook filenames & builds accordingly
	wxString sourceTempFileName = MakePathToFileInTempFolder_For_Collab(collab_source_text);

	nStep = 3;
	msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
	pStatusBar->UpdateProgress(_("Getting Document for Collaboration"), nStep, msgDisplayed);
	
	wxString bookCode;
	bookCode = m_pApp->GetBookCodeFromBookName(m_pApp->m_CollabBookSelected);
	wxASSERT(!bookCode.IsEmpty());
		
	if (m_bTempCollabByChapterOnly)
	{
		// sourceChapterBuffer, a wxString, stores temporarily the USFM source text
		// until we further below have finished with it and store it in the
		// __SOURCE_INPUTS folder
		// whm 21Sep11 Note: When grabbing the source text, we need to ensure that 
		// an \id XXX line is at the beginning of the text, therefore the second 
		// parameter in the GetTextFromAbsolutePathAndRemoveBOM() call below is 
		// the bookCode. The addition of an \id XXX line will always be needed when
		// grabbing chapter-sized texts.
		sourceChapterBuffer = GetTextFromAbsolutePathAndRemoveBOM(sourceTempFileName,bookCode);
	}
	else
	{
		// likewise, the whole book USFM text is stored in sourceWholeBookBuffer until
		// later it is put into the __SOURCE_INPUTS folder
		// whm 21Sep11 Note: When grabbing the source text, we need to ensure that 
		// an \id XXX line is at the beginning of the text, therefore the second 
		// parameter in the GetTextFromAbsolutePathAndRemoveBOM() call below is 
		// the bookCode. The addition of an \id XXX line will not normally be needed
		// when grabbing whole books, but the check is made to be safe.
		sourceWholeBookBuffer = GetTextFromAbsolutePathAndRemoveBOM(sourceTempFileName,bookCode);
	}
	// Code for setting up the Adapt It project etc requires the following data be
	// setup beforehand also
	// we need to create standard filenames, so the following need to be calculated
	wxString shortProjNameSrc, shortProjNameTgt;
	shortProjNameSrc = GetShortNameFromProjectName(m_pApp->m_CollabProjectForSourceInputs);
	shortProjNameTgt = GetShortNameFromProjectName(m_pApp->m_CollabProjectForTargetExports);

	// update status bar
	// whm modified 7Jan12 to call RefreshStatusBarInfo which now incorporates collaboration
	// info within its status bar message
	m_pApp->RefreshStatusBarInfo();

    // Now, determine if the PT source and PT target projects exist together as an existing
    // AI project. If so we access that project and, if determine if an existing source
	// text exists as an AI chapter or book document, as the case may be.
	//
	// ************************************************************************************** 
    // Note: we **currently** make no attempt to sync data in a book file with that in one
    // or more single-chapter files, and the AI documents generated from these. This is an
    // obvious design flaw which should be corrected some day. For the moment we think
    // people will just work with a whole book or chapters, but not mix the two options.
    // Mixing the two options runs the risk of having two versions of many verses and/or
    // chapters available, one of which will always be older or inferior in meaning - and
	// possibly majorly so. Removing this problem (by storing just the whole book and
	// extracting chapters on demand, or alternatively, storing just chapters and
	// amalgamating them all into a book on demand) is a headache we can give ourselves at
	// some future time! Storing the whole book and extracting and inserting chapters as
	// needed is probably the best way to go (someday)
    // ***************************************************************************************
	// 
	// If an existing source AI document is obtained from the source text we get from PT
	// or BE, we quietly merge the externally derived one to the AI document; using the
    // import edited source text routine (no user intervention occurs in this case).
	
	wxASSERT(!m_pApp->m_curProjectPath.IsEmpty() && !m_pApp->m_curProjectName.IsEmpty());
	wxASSERT(!this->m_TempCollabAIProjectName.IsEmpty() && m_TempCollabAIProjectName == m_pApp->m_curProjectName);
	wxASSERT(::wxDirExists(m_pApp->m_curProjectPath));
 
	// whm 26Feb12 design note: With project-specific collaboration, we will only hookup
	// to existing AI projects

	wxString aiMatchedProjectFolder = m_pApp->m_curProjectName;
	wxString aiMatchedProjectFolderPath = m_pApp->m_curProjectPath;

	bool bAIProjectExists;
	bAIProjectExists = ::wxDirExists(m_pApp->m_curProjectPath);

	if (bAIProjectExists)
	{
		// The Paratext projects selected for source text and target texts have an existing
		// AI project in the user's work folder, so we use that AI project.
		// 
		// 1. Compose an appropriate document name to be used for the document that will
		//    contain the chapter grabbed from the PT source project's book.
		// 2. Check if a document by that name already exists in the local work folder..
		// 3. If the document does not exist create it by parsing/tokenizing the string 
		//    now existing in our sourceChapterBuffer, saving it's xml form to disk, and 
		//    laying the doc out in the main window.
		// 4. If the document does exist, we first need to do any merging of the existing
		//    AI document with the incoming one now located in our sourceChapterBuffer, so
		//    that we end up with the merged one's xml form saved to disk, and the resulting
		//    document laid out in the main window.
		// 5. Copy the just-grabbed chapter source text from the .temp folder over to the
		//    Project's __SOURCE_INPUTS folder (creating the __SOURCE_INPUTS folder if it 
		//    doesn't already exist).
        // 6. [BILL WROTE:] Check if the chapter text received from the PT target project
        //    (now in the targetChapterBuffer or targetWholeBookBuffer) has changed from its
        //    form in the AI document. If so we need to merge the two documents calling up a
        //    Conflict Resolution Dialog where needed.
		//    [BEW:] We can't do what Bill wants for 6. The PT text that we get may well
		//    have been edited, and is different from what we'd get from an USFM export of
		//    the AI doc's target text, but we can't in any useful way use a text-based 2
		//    pane conflict resultion dialog for merging -- we don't have any way to merge a
		//    USFM marked up plain text target language string back into the m_adaption
		//    and m_targetStr members of the CSourcePhrase instances list from which it
		//    was once generated. That's an interactive process we've not yet designed or
		//    implemented, and may never do so. 
        //    Any conflict resolution dialog could only be used at the point that the user
        //    has signed off on his interlinear edits (done via the AI phrase box) for the
        //    whole chapter. It's then that the check of the new export of the target text
        //    could possibly get checked against whatever is the chapter's text currently
        //    in Paratext or Bibledit - and at that point, any conflicts noted would have
        //    to be resolved.
        //    BEW 1Aug11, Design change: we won't support a conflict resolution dialog. We
        //    will instead get the target and/or free trans text from the external editor
        //    at the start of the handler for a File / Save request by the user, and that's
        //    when we'll do our GUI-less comparisons and updating.
        //     
        //    The appropriate time that we should grab the PT or BE target project's
        //    chapter should be as late as possible, so we do it at File / Save. We check
        //    PT or BE is not running before we allow the transfer to happen, and if it is,
        //    we nag the user to shut it down and we don't allow the Save to happen until
        //    he does. This way we avoid DVCS conflicts if the user happens to have made
        //    some edits back in the external editor while the AI session is in progress.
		//    
		// 7. BEW added this note 27Ju11. We have to ALWAYS do a resursive merge of the
		//    source text obtained from Paratext or Bibledit whenever there is an already
        //    saved chapter document (or whole book document) for that information stored
        //    in Adapt It. The reason is as follows. Suppose we didn't, and then the
        //    following scenario happened. The user declined to send the adapted data back
        //    to PT or BE; but while he has the option to do so or not, our code
        //    unilaterally copies the input (new) source text which lead to the AI document
        //    he claimed to save, to the __SOURCE_INPUTS folder. So the source text in AI
        //    is not in sync with what the source text in the (old) document in AI is. This
        //    then becomes a problem if he later in AI re-gets that source text (unchanged)
        //    from PT or BE. If we tested it for differences with what AI is storing for
        //    the source text, it would then detect no differences - because he's not made
        //    any, and if we used the result of that test as grounds for not bothering with
        //    a recursive merge of the source text to the document, then the (old) document
        //    would get opened and it would NOT reflect any of the changes which the user
        //    actually did at some earlier time to that source text. We can't let this
        //    happen. So either we have to delay moving the new source text to the
        //    __SOURCE_INPUTS folder until the user has actually saved the document rebuilt
        //    by merger or the edited source text (which is difficult to do and is a
        //    cumbersome design), or, and this is far better, we unilaterally do a source
        //    text merger every time the source text is grabbed from PT or BE -- then we
        //    guarantee any externally done changes will make it into the doc before it is
        //    displayed in the view window.
        
		nStep = 4;
		msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
		pStatusBar->UpdateProgress(_("Getting Document for Collaboration"), nStep, msgDisplayed);
		
		// first, get the project hooked up
		wxASSERT(!aiMatchedProjectFolder.IsEmpty());
		wxASSERT(!aiMatchedProjectFolderPath.IsEmpty());
		// do a wizard-less hookup to the matched project
		bool bSucceeded = HookUpToExistingAIProject(m_pApp, &aiMatchedProjectFolder, 
													&aiMatchedProjectFolderPath);
		if (bSucceeded)
		{
			// the paths are all set and the adapting and glossing KBs are loaded
			wxString documentName;
			// Note: we use a stardard Paratext naming for documents, but omit 
			// project short name(s)
		
			// whm 27Jul11 modified below for whole book vs chapter operation
			wxString chForDocName;
			if (m_bTempCollabByChapterOnly)
				chForDocName = bareChapterSelectedStr;
			else
				chForDocName = _T("");
			
			wxString docTitle = m_pApp->GetFileNameForCollaboration(_T("_Collab"), 
							bookCode, _T(""), chForDocName, _T(""));
			documentName = m_pApp->GetFileNameForCollaboration(_T("_Collab"), 
							bookCode, _T(""), chForDocName, _T(".xml"));
			// create the absolute path to the document we are checking for the existence of
			wxString docPath = aiMatchedProjectFolderPath + m_pApp->PathSeparator
				+ m_pApp->m_adaptationsFolder + m_pApp->PathSeparator
				+ documentName;
			// set the member used for creating the document name for saving to disk
			m_pApp->m_curOutputFilename = documentName;
			// make the backup filename too
			m_pApp->m_curOutputBackupFilename = m_pApp->GetFileNameForCollaboration(_T("_Collab"), 
							bookCode, _T(""), chForDocName, _T(".BAK"));

			// disallow Book mode, and prevent user from turning it on
			m_pApp->m_bBookMode = FALSE;
			m_pApp->m_pCurrBookNamePair = NULL;
			m_pApp->m_nBookIndex = -1;
			m_pApp->m_bDisableBookMode = TRUE;
			wxASSERT(m_pApp->m_bDisableBookMode);
	
			// check if document exists already
			if (::wxFileExists(docPath))
			{
				// it exists, so we have to merge in the source text coming from PT or BE
				// into the document we have already from an earlier collaboration on this
				// chapter
				
				nStep = 5;
				msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
				pStatusBar->UpdateProgress(_("Getting Document for Collaboration"), nStep, msgDisplayed);
		
                // set the private booleans which tell us whether or not the Usfm structure
                // has changed, and whether or not the text and/or puncts have changed. The
                // auxilary functions for this are in helpers.cpp
                // temporary...
                wxString oldSrcText = GetTextFromFileInFolder(m_pApp, 
												m_pApp->m_sourceInputsFolderPath, docTitle);
				if (m_bTempCollabByChapterOnly)
				{
					// LHS of next 2 lines are private members of CGetSourceTextFromEditorDlg class
					m_bTextOrPunctsChanged = IsTextOrPunctsChanged(oldSrcText, sourceChapterBuffer);
					m_bUsfmStructureChanged = IsUsfmStructureChanged(oldSrcText, sourceChapterBuffer);
				}
				else
				{
					m_bTextOrPunctsChanged = IsTextOrPunctsChanged(oldSrcText, sourceWholeBookBuffer);
					m_bUsfmStructureChanged = IsUsfmStructureChanged(oldSrcText, sourceWholeBookBuffer);
				}

				if (m_bTextOrPunctsChanged && m_pApp->m_bCopySource)
				{
					// for safety's sake, turn off copying of the source text, but tell the
					// user below that he can turn it back on if he wants
					m_pApp->m_bSaveCopySourceFlag_For_Collaboration = m_pApp->m_bCopySource;
					wxCommandEvent dummy;
					pView->ToggleCopySource(); // toggles m_bCopySource's value & resets menu item
				}

				// Get the layout, view etc done -- whether we merge in the external
				// editor's source text project's text data depends on whether or not
				// something was changed in the external editor's data since the last time
				// it was grabbed. If no changes to it were done externally, we can forgo
				// the merger and just open the Adapt It document again 'as is'.
				bool bDoMerger;
				if (m_bTextOrPunctsChanged || m_bUsfmStructureChanged)
				{
					bDoMerger = TRUE; // merge the from-editor source text into the AI document
				}
				else
				{
					bDoMerger = FALSE; // use the AI document 'as is'
				}
				// comment out next line when this modification is no longer wanted for 
				// debugging purposes
//#define _DO_NO_MERGER_BUT_INSTEAD_LEAVE_m_pSourcePhrases_UNCHANGED
#ifdef _DO_NO_MERGER_BUT_INSTEAD_LEAVE_m_pSourcePhrases_UNCHANGED
#ifdef _DEBUG
				bDoMerger = FALSE;
				m_bUsfmStructureChanged = m_bUsfmStructureChanged; // avoid compiler warning
#endif
#endif
				nStep = 6;
				msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
				pStatusBar->UpdateProgress(_("Getting Document for Collaboration"), nStep, msgDisplayed);
				
				m_pApp->maxProgDialogValue = 9; // temporary hack while calling OpenDocWithMerger() below
		
				bool bDoLayout = TRUE;
				bool bCopySourceWanted = m_pApp->m_bCopySource; // whatever the value now is
				bool bOpenedOK;
                // the next call always returns TRUE, otherwise wxWidgets doc/view
                // framework gets messed up if we pass FALSE to it
                // whm 26Aug11 Note: We cannot pass the pProgDlg pointer to the OpenDocWithMerger()
                // calls below, because within OpenDocWithMerger() it calls ReadDoc_XML() which 
                // requires a wxProgressDialog with a different kind of range values - a count of
                // source phrases, whereas here in OnOK() we are using 9 steps as the range for the
                // progress dialog.
				if (m_bTempCollabByChapterOnly)
				{
					bOpenedOK = OpenDocWithMerger(m_pApp, docPath, sourceChapterBuffer, 
												bDoMerger, bDoLayout, bCopySourceWanted);
				}
				else
				{
					bOpenedOK = OpenDocWithMerger(m_pApp, docPath, sourceWholeBookBuffer, 
												bDoMerger, bDoLayout, bCopySourceWanted);
				}
				bOpenedOK = bOpenedOK; // the function always returns TRUE (even if there was
									   // an error) because otherwise it messes with the doc/view
									   // framework badly; so protect from the compiler warning
									   // in the identity assignment way

// If the doc was corrupt, and we've recovered it, m_recovery_pending will be TRUE.  In this case we fake a
//    cancel of the dialog and bail out.

                if (m_pApp->m_recovery_pending)
                {    
					this->EndModal(wxID_CANCEL);
					return;
				}

				// whm 25Aug11 reset the App's maxProgDialogValue back to MAXINT
				m_pApp->maxProgDialogValue = 2147483647; //MAXINT; // temporary hack while calling OpenDocWithMerger() above
				
				// warn user about the copy flag having been changed, if such a warning is necessary					   
				if (m_pApp->m_bSaveCopySourceFlag_For_Collaboration && !m_pApp->m_bCopySource
					&& m_bTextOrPunctsChanged)
				{
                    // the copy flag was ON, and was just turned off above, and there are
                    // probably going to be new adaptable 'holes' in the document, so tell
                    // user Copy Source been turned off, etc
					wxMessageBox(_(
"The Copy Source (to Target) command on View Menu is temporarily turned off because the source text was edited outside of Adapt It.\nNow you need to adapt the changed parts again.\nAdapt It will guide you, displaying each location that was changed. Use the Enter key to jump to the next location after you enter each new adaptation.\nIf these locations have free translations, you should update each free translation too."),
					_T(""), wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent 
				}

				// update the copy of the source text in __SOURCE_INPUTS folder with the
				// newly grabbed source text
				wxString pathCreationErrors;
				wxString sourceFileTitle = m_pApp->GetFileNameForCollaboration(_T("_Collab"), bookCode, 
									_T(""), chForDocName, _T(""));
				
				nStep = 7;
				msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
				pStatusBar->UpdateProgress(_("Getting Document for Collaboration"), nStep, msgDisplayed);
		
				bool bMovedOK;
				if (m_bTempCollabByChapterOnly)
				{
					bMovedOK = MoveTextToFolderAndSave(m_pApp, m_pApp->m_sourceInputsFolderPath, 
										pathCreationErrors, sourceChapterBuffer, sourceFileTitle);
				}
				else
				{
					bMovedOK = MoveTextToFolderAndSave(m_pApp, m_pApp->m_sourceInputsFolderPath, 
										pathCreationErrors, sourceWholeBookBuffer, sourceFileTitle);
				}
				// don't expect a failure in this, but if so, tell developer (English
				// message is all that is needed)
				if (!bMovedOK)
				{
					// we don't expect failure, so an English message will do
					wxString msg;
					msg = _T("For the developer: When collaborating: error in MoveTextToFolderAndSave() within GetSourceTextFromEditor.cpp:\nUnexpected (non-fatal) error trying to move source text to a file in __SOURCE_INPUTS folder.\nThe source text USFM data hasn't been saved (or updated) to disk there.");
					wxMessageBox(msg, _T(""), wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent
					m_pApp->LogUserAction(msg);
				}
			} // end of TRUE block for test: if (::wxFileExists(docPath))
			else
			{
				nStep = 5;
				msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
				pStatusBar->UpdateProgress(_("Getting Document for Collaboration"), nStep, msgDisplayed);
		
				// it doesn't exist, so we have to tokenize the source text coming from PT
				// or BE, create the document, save it and lay it out in the view window...
				wxASSERT(m_pApp->m_pSourcePhrases->IsEmpty());
				// parse the input file
				int nHowMany;
				SPList* pSourcePhrases = new SPList; // for storing the new tokenizations
				// in the next call, 0 = initial sequ number value
				if (m_bTempCollabByChapterOnly)
				{
					nHowMany = pView->TokenizeTextString(pSourcePhrases, sourceChapterBuffer, 0);
				}
				else
				{
					nHowMany = pView->TokenizeTextString(pSourcePhrases, sourceWholeBookBuffer, 0);
				}
				// copy the pointers over to the app's m_pSourcePhrases list (the document)
				if (nHowMany > 0)
				{
					SPList::Node* pos = pSourcePhrases->GetFirst();
					while (pos != NULL)
					{
						CSourcePhrase* pSrcPhrase = pos->GetData();
						m_pApp->m_pSourcePhrases->Append(pSrcPhrase);
						pos = pos->GetNext();
					}
					// now clear the pointers from pSourcePhrases list, but leave their memory
					// alone, and delete pSourcePhrases list itself
					pSourcePhrases->Clear(); // no DeleteContents(TRUE) call first, ptrs exist still
					if (pSourcePhrases != NULL) // whm 11Jun12 added NULL test
						delete pSourcePhrases;

					// the single-chapter or whole-book document is now ready for displaying 
					// in the view window
					wxASSERT(!m_pApp->m_pSourcePhrases->IsEmpty());

					nStep = 6;
					msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
					pStatusBar->UpdateProgress(_("Getting Document for Collaboration"), nStep, msgDisplayed);
		
					SetupLayoutAndView(m_pApp, docTitle);
				}
				else
				{
					// we can't go on, there is nothing in the document's m_pSourcePhrases
					// list, so keep the dlg window open; tell user what to do - this
					// should never happen, so an English message will suffice
					wxString msg;
					msg = _T("Unexpected (non-fatal) error when trying to load source text obtained from the external editor - there is no source text!\nPerhaps the external editor's source text project file that is currently open has no data in it?\nIf so, rectify that in the external editor, then switch back to the running Adapt It and try again.\n Or you could Cancel the dialog and then try to fix the problem.");
					wxMessageBox(msg, _T(""), wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
					m_pApp->LogUserAction(msg);
					pStatusBar->FinishProgress(_("Getting Document for Collaboration"));
					return;
				}

 				nStep = 7;
				msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
				pStatusBar->UpdateProgress(_("Getting Document for Collaboration"), nStep, msgDisplayed);
		
               // 5. Copy the just-grabbed chapter or book source text from the .temp
                // folder over to the Project's __SOURCE_INPUTS folder (creating the
                // __SOURCE_INPUTS folder if it doesn't already exist). Note, the source
                // text in sourceChapterBuffer has had its initial BOM removed. We never
                // store the BOM permanently. Internally MoveTextToFolderAndSave() will add
                // .txt extension to the docTitle string, to form the correct filename for
                // saving. Since the AI project already exists, the
                // m_sourceInputsFolderName member of the app class will point to an
                // already created __SOURCE_INPUTS folder in the project folder.
				wxString pathCreationErrors;
				bool bMovedOK;
				if (m_bTempCollabByChapterOnly)
				{
					bMovedOK = MoveTextToFolderAndSave(m_pApp, m_pApp->m_sourceInputsFolderPath, 
										pathCreationErrors, sourceChapterBuffer, docTitle);
				}
				else
				{
					bMovedOK = MoveTextToFolderAndSave(m_pApp, m_pApp->m_sourceInputsFolderPath, 
										pathCreationErrors, sourceWholeBookBuffer, docTitle);
				}
				// we don't expect a failure in this, but if so, tell developer (English
				// message is all that is needed), and let processing continue
				if (!bMovedOK)
				{
					wxString msg;
					msg = _T("For the developer: When collaborating: error in MoveTextToFolderAndSave() within GetSourceTextFromEditor.cpp:\nUnexpected (non-fatal) error trying to move source text to a file in __SOURCE_INPUTS folder.\nThe source text USFM data hasn't been saved (or updated) to disk there.");
					wxMessageBox(msg, _T(""), wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
					m_pApp->LogUserAction(msg);
				}

			} // end of else block for test: if (::wxFileExists(docPath))
		} // end of TRUE block for test: if (bSucceeded)
		else
		{
			m_pApp->bDelay_PlacePhraseBox_Call_Until_Next_OnIdle = FALSE; // restore default

            // A very unexpected failure to hook up -- what should be done here? The KBs
            // are unloaded, but the app is in limbo - paths are current for a project
            // which didn't actually get set up, and so isn't actually current. But a
            // message has been seen (albeit, one for developer only as we don't expect
            // this error). I guess we just return from OnOK() and give a non-localizable 2nd
            // message tell the user to take the Cancel option & retry
            wxString msg;
			msg = _T("Collaboration: a highly unexpected failure to hook up to the identified pre-existing Adapt It adaptation project has happened.\nYou should Cancel from the current collaboration attempt.\nThen carefully check that the Adapt It project's source language and target language names exactly match the language names you supplied within Paratext or Bibledit.\n Fix the Adapt It project folder's language names to be spelled the same, then try again.");
			wxMessageBox(msg, _T(""), wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
			m_pApp->LogUserAction(msg);
			pStatusBar->FinishProgress(_("Getting Document for Collaboration"));
			return;
		}
	} // end of TRUE block for test: if (bCollaborationUsingExistingAIProject)
	else
	{
		// The App's m_curProjectPath member was unexpectedly empty!
		wxString msg = _T("The Adapt It project \"%s\" at the following path:\n\n%s\n\nunexpectedly went missing!\n\nYou should Cancel from the current collaboration attempt.\nThen carefully check that the Adapt It project folder exists - restoring it from backups if necessary, then try again.");
		msg = msg.Format(msg,m_pApp->m_CollabAIProjectName.c_str(), m_pApp->m_curProjectPath.c_str());
		m_pApp->LogUserAction(msg);
		pStatusBar->FinishProgress(_("Getting Document for Collaboration"));
		return;
	}
	/*
		// As of version 6.1.x, this block should never be entered. Collaboration is now on a project-by-project
		// basis and GetSourceTextFromEditor() should only be called via an existing project.
		wxString msg = _T("In GetSourceTextFromEditor::OnOK() entered else block of if (bCollaborationUsingExistingAIProject) - programming error for version 6.2.x!");
		wxCHECK_RET(FALSE,msg);
        // The Paratext/Bibledit project selected for source text and target texts do not
        // yet exist as a previously created AI project in the user's work folder, so we
        // need to create the AI project using information contained in the
        // Collab_Project_Info_Struct structs that are populated dynamically in
        // InitDialog() which calls the App's GetListOfPTProjects() or GetListOfBEprojects.
		
		m_pApp->bDelay_PlacePhraseBox_Call_Until_Next_OnIdle = FALSE; // restore default value
		
		// Get the Collab_Project_Info_Struct structs for the source and target PT or BE projects
		Collab_Project_Info_Struct* pInfoSrc;
		Collab_Project_Info_Struct* pInfoTgt;
		pInfoSrc = m_pApp->GetCollab_Project_Struct(shortProjNameSrc);  // gets pointer to the struct from the 
																	// pApp->m_pArrayOfCollabProjects
		pInfoTgt = m_pApp->GetCollab_Project_Struct(shortProjNameTgt);  // gets pointer to the struct from the 
																	// pApp->m_pArrayOfCollabProjects
		wxASSERT(pInfoSrc != NULL);
		wxASSERT(pInfoTgt != NULL);

		// Notes: The pInfoSrc and pInfoTgt structs above contain the following struct members:
		// bool bProjectIsNotResource; // AI only includes in m_pArrayOfCollabProjects the PT
		//                                 or BE projects where this is TRUE
		// bool bProjectIsEditable; // AI only allows user to see and select a PT or BE TARGET project 
		//                             where this is TRUE
		// wxString versification; // AI assumes same versification of Target as used in Source
		// wxString fullName; // This is 2nd field seen in "Get Source Texts from this project:" drop down
							  // selection (after first ':')
		// wxString shortName; // This is 1st field seen in "Get Source Texts from this project:" drop down
							   // selection (before first ':'). It is always the same as the project's folder
							   // name (and ssf file name) in the "My Paratext Projects" folder. It has to
							   // be unique for every PT or BE project.
		// wxString languageName; // This is 3rd field seen in "Get Source Texts from this project:" drop down
							      // selection (between 2nd and 3rd ':'). We use this as x and y language
							      // names to form the "x to y adaptations" project folder name in AI if it
							      // does not already exist
		// wxString ethnologueCode; // If it exists, this a 4th field seen in "Get Source Texts from this project:"
							        // drop down selection (after 3rd ':')
		// wxString projectDir;   // this has the path/directory to the project's actual files
		// wxString booksPresentFlags; // this is the long 123-element string of 0 and 1 chars indicating which
									   // books are present within the PT project
		// wxString chapterMarker; // default is _T("c")
		// wxString verseMarker;   // default is _T("v")
		// wxString defaultFont;   // default is _T("Arial")or whatever is specified in the PT
		//                         // or BE project's ssf file
		// wxString defaultFontSize; // default is _T("10") or whatever is specified in the PT
		//                            or BE project's ssf file
		// wxString leftToRight; // default is _T("T") unless "F" is specified in the PT project's ssf file
		// wxString encoding;    // default is _T("65001"); // 65001 is UTF8

		// For building an AI project we can use as initial values the information within the appropriate
		// field members of the above struct.
		// 
		// Code below does the following things:
		// 1. Create an AI project using the information from the PT structs, setting up
		//    the necessary directories and the appropriately constructed project config 
		//    file to disk.
		// 2. We need to decide if we will be using book folder mode automatically [ <- No!] 
		//    for chapter sized documents of if we will dump them all in the "adaptations" 
        //    folder [ <- Yes!]. The user won't know the difference except if the
        //    administrator decides at some future time to turn PT collaboration OFF. If we
        //    used the book folders during PT collaboration for chapter files, we would
        //    need to ensure that book folder mode stays turned on when PT collaboration
        //    was turned off.
		// 3. Compose an appropriate document name to be used for the document that will
		//    contain the chapter or book grabbed from the PT or BE source project.
		// 4. Create the document by parsing/tokenizing the string now existing in our 
		//    sourceChapterBuffer, saving it's xml form to disk, and laying the doc out in 
		//    the main window.
        // 5. Copy the just-grabbed chapter source text or book source text from the .temp
        //    folder over to the Project's __SOURCE_INPUTS folder (creating the
        //    __SOURCE_INPUTS folder if it doesn't already exist).
		
		nStep = 4;
		msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
		pProgDlg->Update(nStep,msgDisplayed);
		//::wxSafeYield();
		
		// Step 1 & 2
		bool bDisableBookMode = TRUE;
		// whm modified 7Sep11 to use the App's language name values
		//bool bProjOK = CreateNewAIProject(m_pApp, pInfoSrc->languageName, 
		//							pInfoTgt->languageName, pInfoSrc->ethnologueCode, 
		//							pInfoTgt->ethnologueCode, bDisableBookMode);
		bool bProjOK = CreateNewAIProject(m_pApp, m_pApp->m_CollabSourceLangName, 
									m_pApp->m_CollabTargetLangName, pInfoSrc->ethnologueCode, 
									pInfoTgt->ethnologueCode, bDisableBookMode);
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
			wxMessageBox(message,_("Project Not Created"), wxICON_ERROR | wxOK, this); // whm 28Nov12 added this as parent);
			pProgDlg->Destroy();
			return;
		}
		// Step 3. (code copied from above)
		wxString documentName;
		// Note: we use a stardard Paratext naming for documents, but omit 
		// project short name(s)
		
		// whm 27Jul11 modified below for whole book vs chapter operation
		wxString chForDocName;
		if (m_bTempCollabByChapterOnly)
			chForDocName = bareChapterSelectedStr;
		else
			chForDocName = _T("");
		
		wxString docTitle = m_pApp->GetFileNameForCollaboration(_T("_Collab"), 
									bookCode, _T(""), chForDocName, _T(""));
		documentName = m_pApp->GetFileNameForCollaboration(_T("_Collab"), 
									bookCode, _T(""), chForDocName, _T(".xml"));
		// create the absolute path to the document we are checking for the existence of
		wxString docPath = m_pApp->m_curProjectPath + m_pApp->PathSeparator
				+ m_pApp->m_adaptationsFolder + m_pApp->PathSeparator + documentName;
		// set the member used for creating the document name for saving to disk
		m_pApp->m_curOutputFilename = documentName;
		// make the backup filename too
		m_pApp->m_curOutputBackupFilename = m_pApp->GetFileNameForCollaboration(_T("_Collab"), 
						bookCode, _T(""), chForDocName, _T(".BAK"));
		// Step 4. (code copied from above)
		wxASSERT(m_pApp->m_pSourcePhrases->IsEmpty());
		// parse the input file
		int nHowMany;
		SPList* pSourcePhrases = new SPList; // for storing the new tokenizations
				
		nStep = 5;
		msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
		pProgDlg->Update(nStep,msgDisplayed);
		//::wxSafeYield();
		
		// in the next call, 0 = initial sequ number value
		if (m_bTempCollabByChapterOnly)
		{
			nHowMany = pView->TokenizeTextString(pSourcePhrases, sourceChapterBuffer, 0); 
		}
		else
		{
			nHowMany = pView->TokenizeTextString(pSourcePhrases, sourceWholeBookBuffer, 0); 
		}
		// copy the pointers over to the app's m_pSourcePhrases list (the document)
		if (nHowMany > 0)
		{
			SPList::Node* pos = pSourcePhrases->GetFirst();
			while (pos != NULL)
			{
				CSourcePhrase* pSrcPhrase = pos->GetData();
				m_pApp->m_pSourcePhrases->Append(pSrcPhrase);
				pos = pos->GetNext();
			}
			// now clear the pointers from pSourcePhrases list, but leave their memory
			// alone, and delete pSourcePhrases list itself
			pSourcePhrases->Clear(); // no DeleteContents(TRUE) call first, ptrs exist still
			if (pSourcePhrases != NULL) // whm 11Jun12 added NULL test
				delete pSourcePhrases;

			// the single-chapter or whole-book document is now ready for displaying 
			// in the view window
			wxASSERT(!m_pApp->m_pSourcePhrases->IsEmpty());

			nStep = 6;
			msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
			pProgDlg->Update(nStep,msgDisplayed);
			//::wxSafeYield();
		
			SetupLayoutAndView(m_pApp, docTitle);
		}
		else
		{
            // there was no source text data! so we can't go on, there is nothing in the
            // document's m_pSourcePhrases list, so keep the dlg window open; tell user
            // what to do - this should never happen, so an English message will suffice
			wxString msg;
			msg = _T("Unexpected (non-fatal) error when trying to load source text obtained from the external editor - there is no source text!\nPerhaps the external editor's source text project file that is currently open has no data in it?\nIf so, rectify that in the external editor, then switch back to the running Adapt It and try again.\n Or you could Cancel the dialog and then try to fix the problem.");
			wxMessageBox(msg, _T(""), wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
			m_pApp->LogUserAction(msg);
			pProgDlg->Destroy();
			return;
		}

 		nStep = 7;
		msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
		pProgDlg->Update(nStep,msgDisplayed);
		//::wxSafeYield();
		
       // Step 5. (code copied from above)
		wxString pathCreationErrors;
		bool bMovedOK;
		if (m_bTempCollabByChapterOnly)
		{
			bMovedOK = MoveTextToFolderAndSave(m_pApp, m_pApp->m_sourceInputsFolderPath, 
								pathCreationErrors, sourceChapterBuffer, docTitle);
		}
		else
		{
			bMovedOK = MoveTextToFolderAndSave(m_pApp, m_pApp->m_sourceInputsFolderPath, 
								pathCreationErrors, sourceWholeBookBuffer, docTitle);
		}
		// we don't expect a failure in this, but if it happens, tell developer (English
		// message is all that is needed) and continue processing
		if (!bMovedOK)
		{
			wxString msg;
			msg = _T("For the developer: When collaborating: error in MoveTextToFolderAndSave() within GetSourceTextFromEditor.cpp:\nUnexpected (non-fatal) error trying to move source text to a file in __SOURCE_INPUTS folder.\nThe source text USFM data hasn't been saved (or updated) to disk there.");
			wxMessageBox(msg, _T(""), wxICON_EXCLAMATION | wxOK, this); // whm 28Nov12 added this as parent);
			m_pApp->LogUserAction(msg);
		}

	}  // end of else block for test: if (bCollaborationUsingExistingAIProject)
	*/

	nStep = 8;
	msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
	pStatusBar->UpdateProgress(_("Getting Document for Collaboration"), nStep, msgDisplayed);
		
    // store the "pre-edit" version of the document's adaptation text in a member on the
    // app class, and if free translation is wanted for tranfer to PT or BE as well, get
    // the pre-edit free translation exported and stored in another app member for that
    // purpose
	if (m_bTempCollabByChapterOnly)
	{
		// targetChapterBuff, a wxString, stores temporarily the USFM target text as
		// exported from the document prior to the user doing any work, and we store it
        // persistently in the app private member m_targetTextBuffer_PreEdit; the
        // post-edit translation will be compared to it at the next File / Save - after
        // which the post-edit translation will replace the one held in
        // m_targetTextBuffer_PreEdit
		targetChapterBuffer = ExportTargetText_For_Collab(m_pApp->m_pSourcePhrases);
		m_pApp->StoreTargetText_PreEdit(targetChapterBuffer);

		// likewise, if free translation transfer is expected, we get the pre-edit free
		// translation exported and stored in app's private m_freeTransTextBuffer_PreEdit
		if (m_pApp->m_bCollaborationExpectsFreeTrans)
		{
			// this might return an empty string, or USFMs only string, if no work has yet
			// been done on the free translation
			freeTransChapterBuffer = ExportFreeTransText_For_Collab(m_pApp->m_pSourcePhrases);
			m_pApp->StoreFreeTransText_PreEdit(freeTransChapterBuffer);
		}
	}
	else
	{
		// we are working with a "whole book"
		targetWholeBookBuffer = ExportTargetText_For_Collab(m_pApp->m_pSourcePhrases);
		m_pApp->StoreTargetText_PreEdit(targetWholeBookBuffer);

		if (m_pApp->m_bCollaborationExpectsFreeTrans)
		{
			// this might return an empty string, or USFMs only string, if no work has yet
			// been done on the free translation
			freeTransWholeBookBuffer = ExportFreeTransText_For_Collab(m_pApp->m_pSourcePhrases);
			m_pApp->StoreFreeTransText_PreEdit(freeTransWholeBookBuffer);
		}
	}
	
	nStep = 9;
	msgDisplayed = progMsg.Format(progMsg,nStep,nTotal);
	pStatusBar->UpdateProgress(_("Getting Document for Collaboration"), nStep, msgDisplayed);

	// remove the progress dialog
	pStatusBar->FinishProgress(_("Getting Document for Collaboration"));
		
    // Note: we will store the post-edit free translation, if working with such as wanted
    // in collaboration mode, not in _FREETRANS_OUTPUTS, but rather in a local variable
    // while we use it at File / Save time, and then transfer it to the free translation
    // pre-edit wxString on the app. It won't ever be put in the _FREETRANS_OUTPUTS folder.
    // Only non-collaboration free translation exports will go there.

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
	// 1. Call rdwrtp7 to get a copies of the book in a temporary file at a specified location.
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
	// b. We can easily compare the usfm structure has changed between the source and 
	// destination texts to see if it has changed. 
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
		pListCtrlChapterNumberAndStatus->Disable();
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



