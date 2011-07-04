/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GetSourceTextFromEditorDlg.cpp
/// \author			Bill Martin
/// \date_created	10 April 2011
/// \date_revised	30 June 2011
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CGetSourceTextFromEditorDlg class. 
/// The CGetSourceTextFromEditorDlg class represents a dialog in which a user can obtain a source text
/// for adaptation from an external editor such as Paratext or Bibledit. 
/// \derivation		The CGetSourceTextFromEditorDlg class is derived from wxDialog.
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

#include "Adapt_It.h"
#include "MainFrm.h"
#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"
#include "Pile.h"
#include "Layout.h"
#include "helpers.h"
#include "GetSourceTextFromEditor.h"

extern wxChar gSFescapechar; // the escape char used for start of a standard format marker
extern bool gbIsGlossing;
extern bool gbGlossingUsesNavFont;
extern int gnOldSequNum;

// event handler table
BEGIN_EVENT_TABLE(CGetSourceTextFromEditorDlg, AIModalDialog)
	EVT_INIT_DIALOG(CGetSourceTextFromEditorDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_COMBOBOX(ID_COMBO_SOURCE_PROJECT_NAME, CGetSourceTextFromEditorDlg::OnComboBoxSelectSourceProject)
	EVT_COMBOBOX(ID_COMBO_DESTINATION_PROJECT_NAME, CGetSourceTextFromEditorDlg::OnComboBoxSelectDestinationProject)
	EVT_COMBOBOX(ID_COMBO_FREE_TRANS_PROJECT_NAME, CGetSourceTextFromEditorDlg::OnComboBoxSelectFreeTransProject)
	EVT_BUTTON(wxID_OK, CGetSourceTextFromEditorDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, CGetSourceTextFromEditorDlg::OnCancel)
	EVT_LISTBOX(ID_LISTBOX_BOOK_NAMES, CGetSourceTextFromEditorDlg::OnLBBookSelected)
	EVT_LIST_ITEM_SELECTED(ID_LISTCTRL_CHAPTER_NUMBER_AND_STATUS, CGetSourceTextFromEditorDlg::OnLBChapterSelected)
	EVT_LISTBOX_DCLICK(ID_LISTCTRL_CHAPTER_NUMBER_AND_STATUS, CGetSourceTextFromEditorDlg::OnLBDblClickChapterSelected)
	EVT_RADIOBOX(ID_RADIOBOX_WHOLE_BOOK_OR_CHAPTER, CGetSourceTextFromEditorDlg::OnRadioBoxSelected)
END_EVENT_TABLE()

CGetSourceTextFromEditorDlg::CGetSourceTextFromEditorDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Get Source Text from %s Project"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	GetSourceTextFromEditorDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	wxColour sysColorBtnFace; // color used for read-only text controls displaying
	// color used for read-only text controls displaying static text info button face color
	sysColorBtnFace = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(m_pApp != NULL);
	
	pComboSourceProjectName = (wxComboBox*)FindWindowById(ID_COMBO_SOURCE_PROJECT_NAME);
	wxASSERT(pComboSourceProjectName != NULL);

	pComboDestinationProjectName = (wxComboBox*)FindWindowById(ID_COMBO_DESTINATION_PROJECT_NAME);
	wxASSERT(pComboDestinationProjectName != NULL);

	pComboFreeTransProjectName = (wxComboBox*)FindWindowById(ID_COMBO_FREE_TRANS_PROJECT_NAME);
	wxASSERT(pComboFreeTransProjectName != NULL);

	pRadioBoxWholeBookOrChapter = (wxRadioBox*)FindWindowById(ID_RADIOBOX_WHOLE_BOOK_OR_CHAPTER);
	wxASSERT(pRadioBoxWholeBookOrChapter != NULL);

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

	// other attribute initializations
}

CGetSourceTextFromEditorDlg::~CGetSourceTextFromEditorDlg() // destructor
{
	delete pTheFirstColumn;
	delete pTheSecondColumn;
}

void CGetSourceTextFromEditorDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	
	// Note: the wxListItem which is the column has to be on the heap, because if made a local
	// variable then it will go out of scope and be lost from the wxListCtrl before the
	// latter has a chance to display anything, and then nothing will display in the control
	pTheFirstColumn = new wxListItem; // deleted in the destructor
	pTheSecondColumn = new wxListItem; // deleted in the destructor
	
	wxString title = this->GetTitle();
	title = title.Format(title,this->m_collabEditorName.c_str());
	this->SetTitle(title);

	m_TempCollabProjectForSourceInputs = m_pApp->m_CollabProjectForSourceInputs;
	m_TempCollabProjectForTargetExports = m_pApp->m_CollabProjectForTargetExports;
	m_TempCollabProjectForFreeTransExports = m_pApp->m_CollabProjectForFreeTransExports;
	m_TempCollabBookSelected = m_pApp->m_CollabBookSelected;
	m_TempCollabChapterSelected = m_pApp->m_CollabChapterSelected;

	// determine the path and name to rdwrtp7.exe
	// Note: Nathan M says that when we've tweaked rdwrtp7.exe to our satisfaction that he will
	// ensure that it gets distributed with future versions of Paratext 7.x. Since AI version 6
	// is likely to get released before that happens, and in case some Paratext users haven't
	// upgraded their PT version 7.x to the distribution that has rdwrtp7.exe installed along-side
	// Paratext.exe, we check for its existence here and use it if it is located in the PT
	// installation folder. If not present, we use our own copy in AI's m_appInstallPathOnly 
	// location (and copy the other dll files if necessary)

	if (::wxFileExists(m_pApp->m_ParatextInstallDirPath + m_pApp->PathSeparator + _T("rdwrtp7.exe")))
	{
		// rdwrtp7.exe exists in the Paratext installation so use it
		m_rdwrtp7PathAndFileName = m_pApp->m_ParatextInstallDirPath + m_pApp->PathSeparator + _T("rdwrtp7.exe");
	}
	else
	{
		// rdwrtp7.exe does not exist in the Paratext installation, so use our copy in AI's install folder
		m_rdwrtp7PathAndFileName = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator + _T("rdwrtp7.exe");
		wxASSERT(::wxFileExists(m_rdwrtp7PathAndFileName));
		// Note: The rdwrtp7.exe console app has the following dependencies located in the Paratext install 
		// folder (C:\Program Files\Paratext\):
		//    a. ParatextShared.dll
		//    b. ICSharpCode.SharpZipLib.dll
		//    c. Interop.XceedZipLib.dll
		//    d. NetLoc.dll
		//    e. Utilities.dll
		// I've not been able to get the build of rdwrtp7.exe to reference these by setting
		// either using: References > Add References... in Solution Explorer or the rdwrtp7
		// > Properties > Reference Paths to the "c:\Program Files\Paratext\" folder.
		// Until Nathan can show me how (if possible to do it in the actual build), I will
		// here check to see if these dependencies exist in the Adapt It install folder in
		// Program Files, and if not copy them there from the Paratext install folder in
		// Program Files (if the system will let me do it programmatically).
		wxString AI_appPath = m_pApp->m_appInstallPathOnly;
		wxString PT_appPath = m_pApp->m_ParatextInstallDirPath;
		// Check for any newer versions of the dlls (comparing to previously copied ones) 
		// and copy the newer ones if older ones were previously copied
		wxString fileName = _T("ParatextShared.dll");
		wxString ai_Path;
		wxString pt_Path;
		ai_Path = AI_appPath + m_pApp->PathSeparator + fileName;
		pt_Path = PT_appPath + m_pApp->PathSeparator + fileName;
		if (!::wxFileExists(ai_Path))
		{
			::wxCopyFile(pt_Path,ai_Path);
		}
		else
		{
			if (FileHasNewerModTime(pt_Path,ai_Path))
				::wxCopyFile(PT_appPath,ai_Path);
		}
		fileName = _T("ICSharpCode.SharpZipLib.dll");
		ai_Path = AI_appPath + m_pApp->PathSeparator + fileName;
		pt_Path = PT_appPath + m_pApp->PathSeparator + fileName;
		if (!::wxFileExists(ai_Path))
		{
			::wxCopyFile(pt_Path,ai_Path);
		}
		else
		{
			if (FileHasNewerModTime(pt_Path,ai_Path))
				::wxCopyFile(pt_Path,ai_Path);
		}
		fileName = _T("Interop.XceedZipLib.dll");
		ai_Path = AI_appPath + m_pApp->PathSeparator + fileName;
		pt_Path = PT_appPath + m_pApp->PathSeparator + fileName;
		if (!::wxFileExists(ai_Path))
		{
			::wxCopyFile(pt_Path,ai_Path);
		}
		else
		{
			if (FileHasNewerModTime(pt_Path,ai_Path))
				::wxCopyFile(pt_Path,ai_Path);
		}
		fileName = _T("NetLoc.dll");
		ai_Path = AI_appPath + m_pApp->PathSeparator + fileName;
		pt_Path = PT_appPath + m_pApp->PathSeparator + fileName;
		if (!::wxFileExists(ai_Path))
		{
			::wxCopyFile(pt_Path,ai_Path);
		}
		else
		{
			if (FileHasNewerModTime(pt_Path,ai_Path))
				::wxCopyFile(pt_Path,ai_Path);
		}
		fileName = _T("Utilities.dll");
		ai_Path = AI_appPath + m_pApp->PathSeparator + fileName;
		pt_Path = PT_appPath + m_pApp->PathSeparator + fileName;
		if (!::wxFileExists(ai_Path))
		{
			::wxCopyFile(pt_Path,ai_Path);
		}
		else
		{
			if (FileHasNewerModTime(pt_Path,ai_Path))
				::wxCopyFile(pt_Path,ai_Path);
		}
	}


	// Generally when the "Get Source Text from Paratext Project" dialog is called, we 
	// can be sure that some checks have been done to ensure that Paratext is installed,
	// that previously selected PT projects are still valid/exist, and that the administrator 
	// has switched on AI-PT Collaboration, etc.
	// But, there is a chance that PT projects could be changed while AI is running and
	// if so, AI would be unaware of such changes.
	wxASSERT(m_pApp->m_bCollaboratingWithParatext);
	projList.Clear();
	projList = m_pApp->GetListOfPTProjects();
	bool bTwoOrMorePTProjectsInList = TRUE;
	int nProjCount;
	nProjCount = (int)projList.GetCount();
	if (nProjCount < 2)
	{
		// Less than two PT projects are defined. For AI-PT collaboration to be possible 
		// at least two PT projects must be defined - one for source text inputs and 
		// another for target exports.
		// Notify the user that Adapt It - Paratext collaboration cannot proceed 
		// until the administrator sets up the necessary projects within Paratext
		bTwoOrMorePTProjectsInList = FALSE;
	}

	// populate the combo boxes with eligible projects
	int ct;
	for (ct = 0; ct < (int)projList.GetCount(); ct++)
	{
		wxString projShortName;
		projShortName = projList.Item(ct);
		projShortName = GetShortNameFromLBProjectItem(projShortName);
		pComboSourceProjectName->Append(projList.Item(ct));
		// We must restrict the list of potential destination projects to those
		// which have the <Editable>T</Editable> attribute
		if (PTProjectIsEditable(projShortName))
		{
			pComboDestinationProjectName->Append(projList.Item(ct));
			pComboFreeTransProjectName->Append(projList.Item(ct));
		}
	}
	
	pRadioBoxWholeBookOrChapter->SetSelection(0);

	// Current version 6.0 does not allow selecting texts by whole book,
	// so disable the "Get Whole Book" selection of the wxRadioBox.
	pRadioBoxWholeBookOrChapter->Enable(1,FALSE);

	bool bSourceProjFound = FALSE;
	int nIndex = -1;
	if (!m_TempCollabProjectForSourceInputs.IsEmpty())
	{
		nIndex = pComboSourceProjectName->FindString(m_TempCollabProjectForSourceInputs);
		if (nIndex == wxNOT_FOUND)
		{
			// did not find the PT project for source inputs that was stored in the config file
			bSourceProjFound = FALSE;
		}
		else
		{
			bSourceProjFound = TRUE;
			pComboSourceProjectName->SetSelection(nIndex);
		}
	}
	bool bTargetProjFound = FALSE;
	if (!m_TempCollabProjectForTargetExports.IsEmpty())
	{
		nIndex = pComboDestinationProjectName->FindString(m_TempCollabProjectForTargetExports);
		if (nIndex == wxNOT_FOUND)
		{
			// did not find the PT project for target exports that was stored in the config file
			bTargetProjFound = FALSE;
		}
		else
		{
			bTargetProjFound = TRUE;
			pComboDestinationProjectName->SetSelection(nIndex);
		}
	}

	bool bFreeTransProjFound = FALSE;
	if (!m_TempCollabProjectForFreeTransExports.IsEmpty())
	{
		nIndex = pComboFreeTransProjectName->FindString(m_TempCollabProjectForFreeTransExports);
		if (nIndex == wxNOT_FOUND)
		{
			// did not find the PT project for target exports that was stored in the config file
			bFreeTransProjFound = FALSE;
		}
		else
		{
			bFreeTransProjFound = TRUE;
			pComboFreeTransProjectName->SetSelection(nIndex);
		}
	}
	
	if (!bTwoOrMorePTProjectsInList)
	{
		// This error is not likely to happen so use English message
		wxString str;
		str = _T("Your administrator has configured Adapt It to collaborate with Paratext.\nBut Paratext does not have at least two projects available for use by Adapt It.\nPlease ask your administrator to set up the necessary Paratext projects.\nAdapt It will now abort...");
		wxMessageBox(str, _T("Not enough Paratext projects defined for collaboration"), wxICON_ERROR);
		m_pApp->LogUserAction(_T("PT Collaboration activated but less than two PT projects listed. AI aborting..."));
		abort();
		return;
	}

	wxString strProjectNotSel;
	strProjectNotSel.Empty();

	if (!bSourceProjFound)
	{
		strProjectNotSel += _T("\n   ");
		strProjectNotSel += _("Choose a project to use for obtaining source text inputs");
	}
	if (!bTargetProjFound)
	{
		strProjectNotSel += _T("\n   ");
		//strProjectNotSel += _("Choose a project to use for Transferring Translation Texts");
        // BEW 15Jun11 changed "Texts" to "Drafts" in line with email discussion where we
        // agreed to use 'draft' or 'translation draft' instead of 'translation' so as to
        // avoid criticism for claiming to be a translation app, rather than a drafting app
		strProjectNotSel += _("Choose a project to use for Transferring Translation Drafts");
	}
	
	if (!bSourceProjFound || !bTargetProjFound)
	{
		wxString str;
		str = str.Format(_("Select Paratext Project(s) by clicking on the drop-down lists at the top of the next dialog.\nYou need to do the following before you can begin working:%s"),strProjectNotSel.c_str());
		//wxMessageBox(str, _T("Select Paratext projects that Adapt It will use"), wxICON_ERROR);
		// BEW 15Jun11, changed wxICON_ERROR to be a warning icon. I feel the wxICON_ERROR should
		// only be used for an error serious enough to halt the app because it has become
		// too unstable for it to continue running safely.
		wxMessageBox(str, _T("Select Paratext projects that Adapt It will use"), wxICON_WARNING);
		
	}
	else
	{
		LoadBookNamesIntoList();


		pTheFirstColumn->SetText(_("Chapter"));
		pTheFirstColumn->SetImage(-1);
		pTheFirstColumn->SetAlign(wxLIST_FORMAT_CENTRE);
		pListCtrlChapterNumberAndStatus->InsertColumn(0, *pTheFirstColumn);

		pTheSecondColumn->SetText(_("Chapter Status"));
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
			}
		}
		// Normally at this point the "Select a book" list will be populated and any previously
		// selected book will now be selected. If a book is selected, the "Select a chapter" list
		// will also be populated and, if any previously selected chaper will now again be 
		// selected (from actions done within the OnLBBookSelected handler called above).
		
		// update status bar info (BEW added 27Jun11) - copy & tweak from app's OnInit()
		wxStatusBar* pStatusBar = m_pApp->GetMainFrame()->GetStatusBar(); //CStatusBar* pStatusBar;
		if (m_pApp->m_bCollaboratingWithBibledit || m_pApp->m_bCollaboratingWithParatext)
		{
			wxString message = _("Collaborating with ");
			message += m_pApp->m_collaborationEditor;
			pStatusBar->SetStatusText(message,0); // use first field 0
		}
	}
 }

// event handling functions

void CGetSourceTextFromEditorDlg::OnComboBoxSelectSourceProject(wxCommandEvent& WXUNUSED(event))
{
	int nSel;
	nSel = pComboSourceProjectName->GetSelection();
	m_TempCollabProjectForSourceInputs = pComboSourceProjectName->GetString(nSel);
	// when the selection changes for the Source project we need to reload the
	// "Select a book" list.
	LoadBookNamesIntoList(); // uses the m_TempCollabProjectForSourceInputs
	// Any change in the selection with the source project combo box 
	// requires that we compare the source and target project's books again, which we
	// can do by calling the OnLBBookSelected() handler explicitly here.
	// select LastPTBookSelected 
	if (!m_TempCollabBookSelected.IsEmpty())
	{
		int nSel = pListBoxBookNames->FindString(m_TempCollabBookSelected);
		if (nSel != wxNOT_FOUND)
		{
			// the pListBoxBookNames must have a selection before OnLBBookSelected() below will do anything
			pListBoxBookNames->SetSelection(nSel);
			// set focus on the Select a book list (OnLBBookSelected call below may change focus to Select a chapter list)
			pListBoxBookNames->SetFocus(); 
			wxCommandEvent evt;
			OnLBBookSelected(evt);
		}
	}
}

void CGetSourceTextFromEditorDlg::OnComboBoxSelectDestinationProject(wxCommandEvent& WXUNUSED(event))
{
	int nSel;
	wxString selStr;
	nSel = pComboDestinationProjectName->GetSelection();
	m_TempCollabProjectForTargetExports = pComboDestinationProjectName->GetString(nSel);
	// Any change in the selection with the destination/target project combo box 
	// requires that we compare the source and target project's books again, which we
	// can do by calling the OnLBBookSelected() handler explicitly here.
	LoadBookNamesIntoList(); // uses the m_TempCollabProjectForSourceInputs
	// Any change in the selection with the source project combo box 
	// requires that we compare the source and target project's books again, which we
	// can do by calling the OnLBBookSelected() handler explicitly here.
	// select LastPTBookSelected 
	if (!m_TempCollabBookSelected.IsEmpty())
	{
		int nSel = pListBoxBookNames->FindString(m_TempCollabBookSelected);
		if (nSel != wxNOT_FOUND)
		{
			// the pListBoxBookNames must have a selection before OnLBBookSelected() below will do anything
			pListBoxBookNames->SetSelection(nSel);
			// set focus on the Select a book list (OnLBBookSelected call below may change focus to Select a chapter list)
			pListBoxBookNames->SetFocus(); 
			wxCommandEvent evt;
			OnLBBookSelected(evt);
		}
	}
}

void CGetSourceTextFromEditorDlg::OnComboBoxSelectFreeTransProject(wxCommandEvent& WXUNUSED(event))
{
	int nSel;
	wxString selStr;
	nSel = pComboFreeTransProjectName->GetSelection();
	m_TempCollabProjectForFreeTransExports = pComboFreeTransProjectName->GetString(nSel);
	// Any change in the selection with the destination/target project combo box 
	// requires that we compare the source and target project's books again, which we
	// can do by calling the OnLBBookSelected() handler explicitly here.
	LoadBookNamesIntoList(); // uses the m_TempCollabProjectForSourceInputs
	// Any change in the selection with the source project combo box 
	// requires that we compare the source and target project's books again, which we
	// can do by calling the OnLBBookSelected() handler explicitly here.
	// select LastPTBookSelected 
	if (!m_TempCollabBookSelected.IsEmpty())
	{
		int nSel = pListBoxBookNames->FindString(m_TempCollabBookSelected);
		if (nSel != wxNOT_FOUND)
		{
			// the pListBoxBookNames must have a selection before OnLBBookSelected() below will do anything
			pListBoxBookNames->SetSelection(nSel);
			// set focus on the Select a book list (OnLBBookSelected call below may change focus to Select a chapter list)
			pListBoxBookNames->SetFocus(); 
			wxCommandEvent evt;
			OnLBBookSelected(evt);
		}
	}
}

void CGetSourceTextFromEditorDlg::OnLBBookSelected(wxCommandEvent& WXUNUSED(event))
{
	if (pListBoxBookNames->GetSelection() == wxNOT_FOUND)
		return;
	
	// Check if the same PT project is selected in both combo boxes. If so, warn user
	// and return until different projects are selected.
	wxString srcProj,destProj;
	srcProj = pComboSourceProjectName->GetStringSelection();
	destProj = pComboDestinationProjectName->GetStringSelection();
	if (srcProj == destProj)
	{

		wxString msg;
		msg = _("The Paratext projects selected for obtaining source texts, and for transferring translation texts cannot be the same. Use the drop down list boxes to select different projects.");
		wxMessageBox(msg,_T("Error: The same project is selected for inputs and exports"),wxICON_WARNING);
		// most likely the target drop down list would need to be changed so set focus to it before returning
		pComboDestinationProjectName->SetFocus();
		// clear lists and static text box at bottom of dialog
		pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		return;
	}

	// Since the wxExecute() and file read operations take a few seconds, change the "Select a chapter:"
	// static text above the right list box to read: "Please wait while I query Paratext..."
	pStaticSelectAChapter->SetLabel(_("Please wait while I query Paratext..."));
	
	int nSel;
	wxString fullBookName;
	nSel = pListBoxBookNames->GetSelection();
	fullBookName = pListBoxBookNames->GetString(nSel);
	// When the user selects a book, this handler does the following:
	// 1. Call rdwrtp7 to get a copies of the book in a temporary file at a specified location.
	//    One copy if made of the source PT project's book, and the other copy is made of the
	//    destination PT project's book
	// 2. Open each of the temporary book files and read them into wxString buffers.
	// 3. Scan through the buffers, collecting their usfm markers - simultaneously counting
	//    the number of characters associated with each marker (0 = empty).
	// 4. Only the actual marker, plus a : delimiter, plus the character count associated with
	//    the marker plus a : delimiter plus an MD5 checksum is collected and stored in two 
	//    wxArrayStrings, one marker (and count and MD5 checksum) per array element.
	// 5. This collection of the usfm structure (and extent) is done for both the source PT 
	//    book's data and the destination PT book's data.
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
	wxASSERT(!m_TempCollabProjectForSourceInputs.IsEmpty());
	wxASSERT(!m_TempCollabProjectForTargetExports.IsEmpty());
	sourceProjShortName = GetShortNameFromLBProjectItem(m_TempCollabProjectForSourceInputs);
	targetProjShortName = GetShortNameFromLBProjectItem(m_TempCollabProjectForTargetExports);
	wxString bookNumAsStr = m_pApp->GetBookNumberAsStrFromName(fullBookName);
	// use our App's
	
	wxString sourceTempFileName;
	sourceTempFileName = tempFolder + m_pApp->PathSeparator;
	sourceTempFileName += m_pApp->GetFileNameForCollaboration(_T("_Collab"), bookCode, sourceProjShortName, wxEmptyString, _T(".tmp"));
	wxString targetTempFileName;
	targetTempFileName = tempFolder + m_pApp->PathSeparator;
	targetTempFileName += m_pApp->GetFileNameForCollaboration(_T("_Collab"), bookCode, targetProjShortName, wxEmptyString, _T(".tmp"));
	
	
	// Build the command lines for reading the PT projects using rdwrtp7.exe.
	wxString commandLineSrc,commandLineTgt;
	commandLineSrc = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + sourceProjShortName + _T(" ") + bookCode + _T(" ") + _T("0") + _T(" ") + _T("\"") + sourceTempFileName + _T("\"");
	commandLineTgt = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + targetProjShortName + _T(" ") + bookCode + _T(" ") + _T("0") + _T(" ") + _T("\"") + targetTempFileName + _T("\"");
	wxLogDebug(commandLineSrc);

    // Note: Looking at the wxExecute() source code in the 2.8.11 library, it is clear that
    // when the overloaded version of wxExecute() is used, it uses the redirection of the
    // stdio to the arrays, and with that redirection, it doesn't show the console process
    // window by default. It is distracting to have the DOS console window flashing even
    // momentarily, so we will use that overloaded version of rdwrtp7.exe.

	long resultSrc = -1;
	long resultTgt = -1;
    wxArrayString outputSrc, errorsSrc;
	wxArrayString outputTgt, errorsTgt;
	// Use the wxExecute() override that takes the two wxStringArray parameters. This
	// also redirects the output and suppresses the dos console window during execution.
	resultSrc = ::wxExecute(commandLineSrc,outputSrc,errorsSrc);
	if (resultSrc == 0)
	{
		resultTgt = ::wxExecute(commandLineTgt,outputTgt,errorsTgt);

	}
	if (resultSrc != 0 || resultTgt != 0)
	{
		// not likely to happen so an English warning will suffice
		wxMessageBox(_T("Could not read data from the Paratext projects. Please submit a problem report to the Adapt It developers (see the Help menu)."),_T(""),wxICON_WARNING);
		wxString temp;
		temp = temp.Format(_T("PT Collaboration wxExecute returned error. resultSrc = %d resultTgt = %d"),resultSrc,resultTgt);
		m_pApp->LogUserAction(temp);
		wxLogDebug(temp);
		int ct;
		if (resultSrc != 0)
		{
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
		}
		if (resultTgt != 0)
		{
			temp.Empty();
			for (ct = 0; ct < (int)outputTgt.GetCount(); ct++)
			{
				temp += outputTgt.Item(ct);
				m_pApp->LogUserAction(temp);
				wxLogDebug(temp);
			}
			for (ct = 0; ct < (int)errorsTgt.GetCount(); ct++)
			{
				temp += errorsTgt.Item(ct);
				m_pApp->LogUserAction(temp);
				wxLogDebug(temp);
			}
		}
		pListBoxBookNames->SetSelection(-1); // remove any selection
		// clear lists and static text box at bottom of dialog
		pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		return;
	}

	// now read the tmp files into buffers in preparation for analyzing their chapter and
	// verse status info (1:1:nnnn).
	// Note: The files produced by rdwrtp7.exe for projects with 65001 encoding (UTF-8) have a 
	// UNICODE BOM of ef bb bf
	wxFile f_src(sourceTempFileName,wxFile::read);
	wxFileOffset fileLenSrc;
	fileLenSrc = f_src.Length();
	// read the raw byte data into pByteBuf (char buffer on the heap)
	char* pSourceByteBuf = (char*)malloc(fileLenSrc + 1);
	memset(pSourceByteBuf,0,fileLenSrc + 1); // fill with nulls
	f_src.Read(pSourceByteBuf,fileLenSrc);
	wxASSERT(pSourceByteBuf[fileLenSrc] == '\0'); // should end in NULL
	f_src.Close();
	sourceWholeBookBuffer = wxString(pSourceByteBuf,wxConvUTF8,fileLenSrc);
	free((void*)pSourceByteBuf);

	wxFile f_tgt(targetTempFileName,wxFile::read);
	wxFileOffset fileLenTgt;
	fileLenTgt = f_tgt.Length();
	// read the raw byte data into pByteBuf (char buffer on the heap)
	char* pTargetByteBuf = (char*)malloc(fileLenTgt + 1);
	memset(pTargetByteBuf,0,fileLenTgt + 1); // fill with nulls
	f_tgt.Read(pTargetByteBuf,fileLenTgt);
	wxASSERT(pTargetByteBuf[fileLenTgt] == '\0'); // should end in NULL
	f_tgt.Close();
	targetWholeBookBuffer = wxString(pTargetByteBuf,wxConvUTF8,fileLenTgt);
	// Note: the wxConvUTF8 parameter above works UNICODE builds and does nothing
	// in ANSI builds so this should work for both ANSI and Unicode data.
	free((void*)pTargetByteBuf);

	SourceTextUsfmStructureAndExtentArray.Clear();
	SourceTextUsfmStructureAndExtentArray = GetUsfmStructureAndExtent(sourceWholeBookBuffer);
	TargetTextUsfmStructureAndExtentArray.Clear();
	TargetTextUsfmStructureAndExtentArray = GetUsfmStructureAndExtent(targetWholeBookBuffer);
	
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
		msg1 = msg1.Format(_("The book %s in the Paratext project for obtaining source texts (%s) has no chapter and verse numbers."),fullBookName.c_str(),sourceProjShortName.c_str());
		msg2 = msg2.Format(_("Please run Paratext and select the %s project. Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and ensure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again."),sourceProjShortName.c_str());
		msg1 = msg1 + _T(' ') + msg2;
		wxMessageBox(msg1,_("No chapters and verses found"),wxICON_WARNING);
		pListBoxBookNames->SetSelection(-1); // remove any selection
		pBtnCancel->SetFocus();
		// clear lists and static text box at bottom of dialog
		pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		return;
	}
	if (TargetTextUsfmStructureAndExtentArray.GetCount() == 0)
	{
		wxString msg1,msg2;
		msg1 = msg1.Format(_("The book %s in the Paratext project for storing translation texts (%s) has no chapter and verse numbers."),fullBookName.c_str(),targetProjShortName.c_str());
		msg2 = msg2.Format(_("Please run Paratext and select the %s project. Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and ensure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again."),targetProjShortName.c_str());
		msg1 = msg1 + _T(' ') + msg2;
		wxMessageBox(msg1,_("No chapters and verses found"),wxICON_WARNING);
		pListBoxBookNames->SetSelection(-1); // remove any selection
		pBtnCancel->SetFocus();
		// clear lists and static text box at bottom of dialog
		//pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		return;
	}

	pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
	wxArrayString chapterListFromTargetBook;
	wxArrayString chapterStatusFromTargetBook;
	GetChapterListAndVerseStatusFromTargetBook(fullBookName,chapterListFromTargetBook,chapterStatusFromTargetBook);
	if (chapterListFromTargetBook.GetCount() == 0)
	{
		wxString msg1,msg2;
		msg1 = msg1.Format(_("The book %s in the Paratext project for storing translation texts (%s) has no chapter and verse numbers."),fullBookName.c_str(),targetProjShortName.c_str());
		msg2 = msg2.Format(_("Please run Paratext and select the %s project. Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and ensure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again."),targetProjShortName.c_str());
		msg1 = msg1 + _T(' ') + msg2;
		wxMessageBox(msg1,_("No chapters and verses found"),wxICON_WARNING);
		pListCtrlChapterNumberAndStatus->InsertItem(0,_("No chapters and verses found")); //pListCtrlChapterNumberAndStatus->Append(_("No chapters and verses found"));
		pListCtrlChapterNumberAndStatus->Enable(FALSE);
		pBtnCancel->SetFocus();
		// clear lists and static text box at bottom of dialog
		//pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
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
		int nSel = pListCtrlChapterNumberAndStatus->FindItem(-1,m_TempCollabChapterSelected,TRUE); // TRUE - partial, look for items beginning with m_TempCollabChapterSelected
		if (nSel != wxNOT_FOUND)
		{
			pListCtrlChapterNumberAndStatus->Select(nSel);
			pListCtrlChapterNumberAndStatus->SetFocus();
			// Update the wxTextCtrl at the bottom of the dialog with more detailed
			// info about the book and/or chapter that is selected. 
			pStaticTextCtrlNote->ChangeValue(m_staticBoxDescriptionArray.Item(nSel));
		}
		else
		{
			// Update the wxTextCtrl at the bottom of the dialog with more detailed
			// info about the book and/or chapter that is selected. In this case we can
			// just remind the user to select a chapter.
			pStaticTextCtrlNote->ChangeValue(_T("Please select a chapter in the list at right."));
		}
	}
	
	wxASSERT(pListBoxBookNames->GetSelection() != wxNOT_FOUND);
	m_TempCollabBookSelected = pListBoxBookNames->GetStringSelection();
	
	
}

void CGetSourceTextFromEditorDlg::OnLBChapterSelected(wxListEvent& WXUNUSED(event))
{
	// This handler is called both for single click and double clicks on an
	// item in the chapter list box, since a double click is sensed initially
	// as a single click.

	//int nSel = pListCtrlChapterNumberAndStatus->GetSelection();
	int itemCt = pListCtrlChapterNumberAndStatus->GetSelectedItemCount();
	wxASSERT(itemCt <= 1);
	long nSel = pListCtrlChapterNumberAndStatus->GetNextItem(-1, wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
	if (nSel != wxNOT_FOUND)
	{
		m_TempCollabChapterSelected = pListCtrlChapterNumberAndStatus->GetItemText(nSel);
		// Update the wxTextCtrl at the bottom of the dialog with more detailed
		// info about the book and/or chapter that is selected.
		pStaticTextCtrlNote->ChangeValue(m_staticBoxDescriptionArray.Item(nSel));
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

	// update status bar info (BEW added 27Jun11) - copy & tweak from app's OnInit()
	wxStatusBar* pStatusBar = m_pApp->GetMainFrame()->GetStatusBar(); //CStatusBar* pStatusBar;
	if (m_pApp->m_bCollaboratingWithBibledit || m_pApp->m_bCollaboratingWithParatext)
	{
		wxString message;
		message = message.Format(_("Collaborating with %s: getting source text was Cancelled"), 
									m_pApp->m_collaborationEditor.c_str());
		pStatusBar->SetStatusText(message,0); // use first field 0
	}
	event.Skip();
}


// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CGetSourceTextFromEditorDlg::OnOK(wxCommandEvent& event) 
{
	// Check that both drop down boxes have selections and that they do not
	// point to the same PT project.
	if (m_TempCollabProjectForSourceInputs == m_TempCollabProjectForTargetExports && m_TempCollabProjectForSourceInputs != _("[No Project Selected]"))
	{
		wxString msg, msg1;
		msg = _("The projects selected for getting source texts from, and for transferring translation drafts to, cannot be the same.\nPlease select one project for getting source texts, and a different project for receiving the translation drafts.");
		//msg1 = _("(or, if you select \"[No Project Selected]\" for a project here, the first time a source text is needed for adaptation, the user will have to choose a project from a drop down list of projects).");
		//msg = msg + _T("\n") + msg1;
		wxMessageBox(msg);
		// clear lists and static text box at bottom of dialog
		pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		return; // don't accept any changes - abort the OnOK() handler
	}

	if (m_TempCollabProjectForSourceInputs == m_TempCollabProjectForFreeTransExports && m_TempCollabProjectForSourceInputs != _("[No Project Selected]"))
	{
		wxString msg, msg1;
		msg = _("The projects selected for getting source texts from, and for transferring free translations to, cannot be the same.\nPlease select one project for getting source texts, and a different project for receiving free translations.");
		//msg1 = _("(or, if you select \"[No Project Selected]\" for a project here, the first time a source text is needed for adaptation, the user will have to choose a project from a drop down list of projects).");
		//msg = msg + _T("\n") + msg1;
		wxMessageBox(msg);
		// clear lists and static text box at bottom of dialog
		pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		return; // don't accept any changes - abort the OnOK() handler
	}

	if (m_TempCollabProjectForTargetExports == m_TempCollabProjectForFreeTransExports && m_TempCollabProjectForFreeTransExports != _("[No Project Selected]"))
	{
		wxString msg, msg1;
		msg = _("The projects selected for transferring the drafted translations to, and transferring the free translations to, cannot be the same.\nPlease select one project for receiving the translation drafts, and a different project for receiving free translations.");
		//msg1 = _("(or, if you select \"[No Project Selected]\" for a project here, the first time a source text is needed for adaptation, the user will have to choose a project from a drop down list of projects).");
		//msg = msg + _T("\n") + msg1;
		wxMessageBox(msg);
		// clear lists and static text box at bottom of dialog
		pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		return; // don't accept any changes - abort the OnOK() handler
	}

	if (pListBoxBookNames->GetSelection() == wxNOT_FOUND)
	{
		wxMessageBox(_("Please select a book from the list of books."),_T(""),wxICON_INFORMATION);
		pListBoxBookNames->SetFocus();
		return; // don't accept any changes until a book is selected
	}
	
	if (pListCtrlChapterNumberAndStatus->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED) == wxNOT_FOUND)
	{
		wxMessageBox(_("Please select a chapter from the list of chapters."),_T(""),wxICON_INFORMATION);
		pListCtrlChapterNumberAndStatus->SetFocus();
		return; // don't accept any changes until a book is selected
	}

	CAdapt_ItView* pView = m_pApp->GetView();

	// save new project selection to the App's variables for writing to config file
	m_pApp->m_CollabProjectForSourceInputs = m_TempCollabProjectForSourceInputs;
	m_pApp->m_CollabProjectForTargetExports = m_TempCollabProjectForTargetExports;
	m_pApp->m_CollabProjectForFreeTransExports = m_TempCollabProjectForFreeTransExports;
	if (m_pApp->m_CollabProjectForFreeTransExports.IsEmpty())
	{
		m_pApp->m_bCollaborationExpectsFreeTrans = TRUE;
	}
	m_pApp->m_CollabBookSelected = m_TempCollabBookSelected;
	m_pApp->m_CollabChapterSelected = m_TempCollabChapterSelected;
	wxString bareChapterSelectedStr;
	int chSel = pListCtrlChapterNumberAndStatus->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
	if (chSel != wxNOT_FOUND)
	{
		chSel++; // the chapter number is one greater than the selected lb index
	}
	bareChapterSelectedStr.Empty();
	bareChapterSelectedStr << chSel;
	wxString derivedChStr = GetBareChFromLBChSelection(m_TempCollabChapterSelected);
	wxASSERT(bareChapterSelectedStr == derivedChStr);
	
	// Set up (or access any existing) project for the selected book/chapter
	// How this is handled will depend on whether the book/chapter selected
	// amounts to an existing AI project or not
	wxString shortProjNameSrc, shortProjNameTgt, shortProjNameFreeTrans;
	shortProjNameSrc = this->GetShortNameFromLBProjectItem(m_pApp->m_CollabProjectForSourceInputs);
	shortProjNameTgt = this->GetShortNameFromLBProjectItem(m_pApp->m_CollabProjectForTargetExports);
	if (m_pApp->m_bCollaborationExpectsFreeTrans)
	{
		shortProjNameFreeTrans = this->GetShortNameFromLBProjectItem(m_pApp->m_CollabProjectForFreeTransExports);
	}
	// whm Note: We don't import or merge a free translation text; BEW added 4Jul11, but
	// we do need to get it so as to be able to compare it later with any changes to the
	// free translation (if, of course, there is one defined in the document) - so that
	// the conflict resolution dialog can operate in this situation as it would for
	// conflicts in the target text

	// Get the latest version of the source and target texts from the PT project. We retrieve
	// only texts for the selected chapter. Also get free translation text, if it exists
	// and user wants free translation supported in the transfers of data
	wxString tempFolder;
	wxString bookCode = m_pApp->GetBookCodeFromBookName(m_TempCollabBookSelected);;
	tempFolder = m_pApp->m_workFolderPath + m_pApp->PathSeparator + _T(".temp");
	wxString sourceTempFileName;
	sourceTempFileName = tempFolder + m_pApp->PathSeparator;
	sourceTempFileName += m_pApp->GetFileNameForCollaboration(_T("_Collab"), bookCode, 
							shortProjNameSrc, bareChapterSelectedStr, _T(".tmp"));
	wxString targetTempFileName;
	targetTempFileName = tempFolder + m_pApp->PathSeparator;
	targetTempFileName += m_pApp->GetFileNameForCollaboration(_T("_Collab"), bookCode, 
							shortProjNameTgt, bareChapterSelectedStr, _T(".tmp"));
	wxString freeTransTempFileName; freeTransTempFileName.Empty();
	if (m_pApp->m_bCollaborationExpectsFreeTrans)
	{
		freeTransTempFileName = tempFolder + m_pApp->PathSeparator;
		freeTransTempFileName += m_pApp->GetFileNameForCollaboration(_T("_Collab"), bookCode, 
							shortProjNameFreeTrans, bareChapterSelectedStr, _T(".tmp"));
	}
	
	// Build the command lines for reading the PT projects using rdwrtp7.exe.
	wxString commandLineSrc, commandLineTgt, commandLineFreeTrans;
	commandLineFreeTrans.Empty();
	commandLineSrc = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + shortProjNameSrc + _T(" ") + bookCode + _T(" ") + bareChapterSelectedStr + _T(" ") + _T("\"") + sourceTempFileName + _T("\"");
	commandLineTgt = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + shortProjNameTgt + _T(" ") + bookCode + _T(" ") + bareChapterSelectedStr + _T(" ") + _T("\"") + targetTempFileName + _T("\"");
	if (m_pApp->m_bCollaborationExpectsFreeTrans)
	{
		commandLineFreeTrans = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + shortProjNameFreeTrans + _T(" ") + bookCode + _T(" ") + bareChapterSelectedStr + _T(" ") + _T("\"") + freeTransTempFileName + _T("\"");
	}
	wxLogDebug(commandLineSrc);

	long resultSrc = -1;
	long resultTgt = -1;
	long resultFreeTrans = -1;
    wxArrayString outputSrc, errorsSrc;
	wxArrayString outputTgt, errorsTgt;
	wxArrayString outputFreeTrans, errorsFreeTrans;
	// Use the wxExecute() override that takes the two wxStringArray parameters. This
	// also redirects the output and suppresses the dos console window during execution.
	resultSrc = ::wxExecute(commandLineSrc,outputSrc,errorsSrc);
	if (resultSrc == 0)
	{
		resultTgt = ::wxExecute(commandLineTgt,outputTgt,errorsTgt);

	}
	// get any free translation too, if the expectation is to support it
	if (m_pApp->m_bCollaborationExpectsFreeTrans)
	{
		resultFreeTrans = ::wxExecute(commandLineFreeTrans,outputFreeTrans,errorsFreeTrans);
	}
	if (resultSrc != 0 || resultTgt != 0)
	{
		// not likely to happen so an English warning will suffice
		wxMessageBox(_T("Could not read data from the Paratext/Bibledit projects. Please submit a problem report to the Adapt It developers (see the Help menu)."),_T(""),wxICON_WARNING);
		wxString temp;
		temp = temp.Format(_T("PT Collaboration wxExecute returned error. resultSrc = %d resultTgt = %d"),resultSrc,resultTgt);
		m_pApp->LogUserAction(temp);
		wxLogDebug(temp);
		int ct;
		if (resultSrc != 0)
		{
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
		}
		if (resultTgt != 0)
		{
			temp.Empty();
			for (ct = 0; ct < (int)outputTgt.GetCount(); ct++)
			{
				temp += outputTgt.Item(ct);
				m_pApp->LogUserAction(temp);
				wxLogDebug(temp);
			}
			for (ct = 0; ct < (int)errorsTgt.GetCount(); ct++)
			{
				temp += errorsTgt.Item(ct);
				m_pApp->LogUserAction(temp);
				wxLogDebug(temp);
			}
		}
		pListBoxBookNames->SetSelection(-1); // remove any selection
		// clear lists and static text box at bottom of dialog
		pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		return;
	} // end of TRUE block for test: if (resultSrc != 0 || resultTgt != 0)
	// handle errors collecting the free translation
	if (m_pApp->m_bCollaborationExpectsFreeTrans && resultFreeTrans != 0)
	{
		// not likely to happen so an English warning will suffice
		wxMessageBox(_T("Could not read free translation data from the Paratext/Bibledit projects. Please submit a problem report to the Adapt It developers (see the Help menu)."),_T(""),wxICON_WARNING);
		wxString temp;
		temp = temp.Format(_T("PT Collaboration wxExecute returned error. resultFreeTrans = %d"),resultFreeTrans);
		m_pApp->LogUserAction(temp);
		wxLogDebug(temp);
		int ct;
		if (resultFreeTrans != 0)
		{
			temp.Empty();
			for (ct = 0; ct < (int)outputFreeTrans.GetCount(); ct++)
			{
				temp += outputFreeTrans.Item(ct);
				m_pApp->LogUserAction(temp);
				wxLogDebug(temp);
			}
		}
		pListBoxBookNames->SetSelection(-1); // remove any selection
		// clear lists and static text box at bottom of dialog
		pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
		return;
	} // end of TRUE block for test: if (m_pApp->m_bCollaborationExpectsFreeTrans && resultFreeTrans != 0)

	// whm TODO: after grabbing the chapter source text from the PT project, and before copying it
	// to the appropriate AI project's "__SOURCE_INPUTS" folder, we should check the contents of the
	// __SOURCE_INPUTS folder to see if that source text chapter has been grabbed previously - i.e., 
	// which will be the case if it already exists in the AI project's __SOURCE_INPUTS folder. If it 
	// already exists, we could:
	// (1) do quick MD5 checksums on the existing chapter text file in the Source 
	// Data folder and on the newly grabbed chapter file just grabbed that resides in the .temp 
	// folder, using GetFileMD5(). Comparing the two MD5 checksums we quickly know if there has 
	// been any changes since we last opened the AI document for that chapter. If there have been 
	// changes, we can do a silent automatic merge of the edited source text with the corresponding 
	// AI document, or
	// (2) just always do a merge using Bruce's routines in MergeUpdatedSrc.cpp, regardless of
	// whether there has been a change in the source text or not. This may be the preferred 
	// option for chapter-sized text documents.
	// *** We need to always do (2), (1) is dangerous because the user may elect not to
	// send adaptations back to PT, and that can get the saved source in __SOURCE_INPUTS
	// out of sync with the source text in the CSourcePhrases of the doc as at an earlier state

	// if free translation transfer is wanted, any free translation grabbed from the
	// nominated 3rd project in PT or BE is to be stored in _FREETRANS_OUTPUTS folder
	
	// now read the tmp files into buffers in preparation for analyzing their chapter and
	// verse status info (1:1:nnnn).
	// Note: The files produced by rdwrtp7.exe for projects with 65001 encoding (UTF-8) have a 
	// UNICODE BOM of ef bb bf (we store BOM-less data, but we convert to UTF-16 first,
	// and remove the utf16 BOM, 0xFF 0xFE, before we store the utf-16 text later below)
	
	// sourceTempFileName is actually an absolute path to the file, not just a name
	// string, ditto for targetTempFileName
	sourceChapterBuffer = GetTextFromAbsolutePathAndRemoveBOM(sourceTempFileName);
	targetChapterBuffer = GetTextFromAbsolutePathAndRemoveBOM(targetTempFileName);
	if (m_pApp->m_bCollaborationExpectsFreeTrans)
	{
		// this might return an empty string
		freeTransChapterBuffer = GetTextFromAbsolutePathAndRemoveBOM(freeTransTempFileName);
	}

	SourceChapterUsfmStructureAndExtentArray.Clear();
	SourceChapterUsfmStructureAndExtentArray = GetUsfmStructureAndExtent(sourceChapterBuffer);
	TargetChapterUsfmStructureAndExtentArray.Clear();
	TargetChapterUsfmStructureAndExtentArray = GetUsfmStructureAndExtent(targetChapterBuffer);
	if (m_pApp->m_bCollaborationExpectsFreeTrans)
	{
		FreeTransChapterUsfmStructureAndExtentArray.Clear();
		if (!freeTransChapterBuffer.IsEmpty())
		{
			FreeTransChapterUsfmStructureAndExtentArray = GetUsfmStructureAndExtent(freeTransChapterBuffer);
		}
	}

	// At this point the source text chapter text resides in the sourceChapterBuffer wxString buffer.
	// At this point the target text chapter text resides in the targetChapterBuffer wxString buffer.
	// At this point the target text chapter text resides in the targetChapterBuffer wxString buffer.
	// At this point the structure of the target text is in TargetChapterUsfmStructureAndExtentArray.
	// And if free translations are to be transferred and there was one in PT or BE...
	// At this point the structure of the free translation text is in FreeTransChapterUsfmStructureAndExtentArray.
	// At this point the structure of the free translation text is in FreeTransChapterUsfmStructureAndExtentArray.
	
    // this next block, which saves the free trans in _FREETRANS_OUTPUTS, is as far as we
    // take the free translation, the code next looks at it if and when the user has
    // produced some in the AI doc, or edited the free translation in the AI doc, and we
    // want to transfer the free translation to PT or BE (and may possibly need to use the
    // conflict resolution dialog
	if (m_pApp->m_bCollaborationExpectsFreeTrans && !freeTransChapterBuffer.IsEmpty())
	{
		wxString pathCreationErrors;
		wxString freeTransFileTitle = m_pApp->GetFileNameForCollaboration(_T("_Collab"), bookCode, 
							shortProjNameFreeTrans, bareChapterSelectedStr, _T(""));
		bool bMovedOK = MoveTextToFolderAndSave(m_pApp, m_pApp->m_freeTransOutputsFolderPath, 
								pathCreationErrors, freeTransChapterBuffer, freeTransFileTitle);
		// don't expect a failure in this, but if so, tell developer (English
		// message is all that is needed)
		if (!bMovedOK)
		{
			wxMessageBox(_T(
"For the developer: MoveTextToFolderAndSave() in GetSourceTextFromEditor.cpp: Unexpected (non-fatal) error trying to move free translation text to a file in _FREETRANS_OUTPUTS folder.\nThe free trans data won't have been saved to disk there."),
			_T(""), wxICON_WARNING);
		}
	}

	// update status bar
	wxStatusBar* pStatusBar = m_pApp->GetMainFrame()->GetStatusBar();
	if (m_pApp->m_bCollaboratingWithBibledit || m_pApp->m_bCollaboratingWithParatext)
	{
		wxString message = _("Collaborating with ");
		message += m_pApp->m_collaborationEditor;
		pStatusBar->SetStatusText(message,0); // use first field 0
	}
	
    // Now, determine if the PT source and PT target projects exist together as an existing
    // AI project. If so we access that project and, if determine if an existing source
    // text exists as an AI chapter document. If so, we quietly merge the source texts
    // using Bruce's import edited source text routine (no user intervention occurs in this
    // case).
    // We also compare any incoming target text with any existing target/adapted text in
    // the existing AI chapter document. If there are differences in the target documents,
    // we cannot merge them, but instead, at the time of sending the target text back to PT
    // or BE, show any conflicts to the user for resolution using the conflict resolution
    // dialog.
	// 
	// If the PT source and PT target projects do not yet exist as an existing AI project, we create
	// the project complete with project config file, empty KB, etc., using the information gleaned
	// from the PT source and PT target projects (i.e. language names, font choices and sizes,...) 
	bool bPTCollaborationUsingExistingAIProject;
	// BEW added 21Jun11, provide a storage location for the Adapt It project folder's
	// name, in the event that we successfully obtain a matchup from the
	// PTProjectsExistAsAIProject() call below (aiMatchedProjectFolder will be an empty
	// wxString if the call returns FALSE)
	wxString aiMatchedProjectFolder;
	wxString aiMatchedProjectFolderPath;
	bPTCollaborationUsingExistingAIProject = PTProjectsExistAsAIProject(shortProjNameSrc, 
						shortProjNameTgt, aiMatchedProjectFolder, aiMatchedProjectFolderPath);
	if (bPTCollaborationUsingExistingAIProject)
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
        //    (now in the targetChapterBuffer) has changed from its form in the AI document.
        //    If so we need to merge the two documents calling up the Conflict Resolution
        //    Dialog where needed.
		//    [BEW:] We can't do what Bill wants for 6. The PT text that we get may well
		//    have been edited, and is different from what we'd get from an USFM export of
		//    the AI doc's target text, but we can't in any useful way use a text-based 2
		//    pane conflict resultion dialog for merging -- we don't have any way to merge a
		//    USFM marked up plain text target language string back into the m_adaption
		//    and m_targetStr members of the CSourcePhrase instances list from which it
		//    was once generated. That's an interactive process we've not yet designed or
		//    implemented, and may never do so. 
        //    The conflict resolution dialog can only be used at the point that the user
        //    has signed off on his interlinear edits (done via the AI phrase box) for the
        //    whole chapter. It's then that the check of the new export of the target text
        //    get's checked against whatever is the chapter's text currently in Paratext or
        //    Bibledit - and at that point, any conflicts noted will have to be resolved on
        //    a per-conflict basis, verse by verse, until all conflicts in the chapter have
        //    been resolved - at which point the final versions of the verses are sent as
        //    the single chapter back to the native PT or BE storage in the target PT or BE
        //    project. 
        //    The appropriate time that we should grab the PT or BE target project's
        //    chapter should be as late as possible, just in case the user flips back to
        //    the running PT or BE editor and changes the target text there some more
        //    before flipping back to AI to have his work sent back to PT or BE, so that
        //    there is some hope that what we grab from PT or BE may have been saved to the
        //    repository before the chapter flows back from AI updated - failure to do the
        //    mercurial commit would make the AI changes coming back look like a conflict
        //    resolution to be resolved in PT or BE, and we would like to avoid that, but
        //    can't do anything to prevent it if the user breaks the rules.
		//    
		// 7. BEW added this note 27Ju11. We have to ALWAYS do a resursive merge of the
		//    source text obtained from Paratext or Bibledit whenever there is an already
		//    saved chapter document for that information stored in Adapt It. The reason
		//    is as follows. Suppose we didn't, and then the following scenario happened.
		//    The user declined to send the adapted data back to PT or BE; but while he
		//    has the option to do so or not, our code unilaterally copies the input (new)
		//    source text which lead to the AI document he claimed to save, to the
		//    __SOURCE_INPUTS folder. So the source text in AI is not in sync with what
		//    the source text in the (old) document in AI is. This then becomes a problem
		//    if he later in AI re-gets that source text (unchanged) from PT or BE. If we tested it
		//    for differences with what AI is storing for the source text, it would then detect no
		//    differences - because he's not made any, and if we used the result of that test as grounds for not
		//    bothering with a recursive merge of the source text to the document, then
		//    the (old) document would get opened and it would NOT reflect any of the
		//    changes which the user actually did at some earlier time to that source
		//    text. We can't let this happen. So either we have to delay moving the new
		//    source text to the __SOURCE_INPUTS folder until the user has actually saved
		//    the document rebuilt by merger or the edited source text (which is difficult
		//    to do and is a cumbersome design), or, and this is far better, we
		//    unilaterally do a source text merger every time the source text is grabbed
		//    from PT or BE -- then we can be sure any changes, no matter whether the user
		//    saved the doc previously or not, will make it into the doc before it is
		//    displayed in the view window. We therefore use the MD5 checksum tests only
		//    for determining which message to show the user, but not also for determining
		//    whether or not to merge source text to the current document when source has
		//    just been grabbed from PT or BE.
		
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
			wxString docTitle = m_pApp->GetFileNameForCollaboration(_T("_Collab"), 
							bookCode, _T(""), bareChapterSelectedStr, _T(""));
			documentName = m_pApp->GetFileNameForCollaboration(_T("_Collab"), 
							bookCode, _T(""), bareChapterSelectedStr, _T(".xml"));
			// create the absolute path to the document we are checking for
			wxString docPath = aiMatchedProjectFolderPath + m_pApp->PathSeparator
				+ m_pApp->m_adaptionsFolder + m_pApp->PathSeparator
				+ documentName;
			// set the member used for creating the document name for saving to disk
			m_pApp->m_curOutputFilename = documentName;
			// make the backup filename too
			m_pApp->m_curOutputBackupFilename = m_pApp->GetFileNameForCollaboration(_T("_Collab"), 
							bookCode, _T(""), bareChapterSelectedStr, _T(".BAK"));
			// check if it exists already
			if (::wxFileExists(docPath))
			{
				// it exists, so we have to merge in the source text coming from PT or BE
				// into the document we have already from an earlier collaboration on this
				// chapter
				
				// ensure there is no document currently open
				pView->ClobberDocument();

                // set the private booleans which tell us whether or not the Usfm structure
                // has changed, and whether or not the text and/or puncts have changed. The
                // auxilary functions for this are in helpers.cpp
                // temporary...
                wxString oldSrcText = GetTextFromFileInFolder(m_pApp, 
											m_pApp->m_sourceInputsFolderPath, docTitle);
				m_bTextOrPunctsChanged = IsTextOrPunctsChanged(oldSrcText, sourceChapterBuffer);
				m_bUsfmStructureChanged = IsUsfmStructureChanged(oldSrcText, sourceChapterBuffer);
				m_bUsfmStructureChanged = m_bUsfmStructureChanged; // avoid compiler warning
				if (m_bTextOrPunctsChanged && m_pApp->m_bCopySource)
				{
					// for safety's sake, turn off copying of the source text, but tell the
					// user below that he can turn it back on if he wants
					m_pApp->m_bSaveCopySourceFlag_For_Collaboration = m_pApp->m_bCopySource;
					//m_pApp->m_bCopySource = FALSE;
					wxCommandEvent dummy;
					pView->ToggleCopySource(); // toggles m_bCopySource's value & resets menu item
				}

				// get the layout, view etc done
				bool bDoMerger = TRUE;
				bool bDoLayout = TRUE;
				bool bCopySourceWanted = m_pApp->m_bCopySource; // whatever the value now is
				bool bOpenedOK = OpenDocWithMerger(m_pApp, docPath, sourceChapterBuffer, 
													bDoMerger, bDoLayout, bCopySourceWanted);
				bOpenedOK = bOpenedOK; // it always returns TRUE, otherwise wxWidgets doc/view
									   // framework gets messed up if we pass FALSE to it; save
									   // compiler warning
				// warn user, if needed					   
				if (m_pApp->m_bSaveCopySourceFlag_For_Collaboration && !m_pApp->m_bCopySource
					&& m_bTextOrPunctsChanged)
				{
                    // the copy flag was ON, and was just turned off above, and there are
                    // probably going to be new adaptable 'holes' in the document, so tell
                    // user Copy Source been turned off, etc
					wxMessageBox(_(
"To prevent a source text copy which you may fail to notice, the Copy Source command has been turned off.\nYou can manually turn it back on now if you wish - see the View menu.\nIt will automatically be returned to its original setting when you close off collaboration with this document."),
					_T(""), wxICON_WARNING);
				}

				// update the copy of the source text in __SOURCE_INPUTS folder with the
				// newly grabbed source text
				wxString pathCreationErrors;
				wxString sourceFileTitle = m_pApp->GetFileNameForCollaboration(_T("_Collab"), bookCode, 
									_T(""), bareChapterSelectedStr, _T(""));
				bool bMovedOK = MoveTextToFolderAndSave(m_pApp, m_pApp->m_sourceInputsFolderPath, 
										pathCreationErrors, sourceChapterBuffer, sourceFileTitle);
				// don't expect a failure in this, but if so, tell developer (English
				// message is all that is needed)
				if (!bMovedOK)
				{
					// we don't expect failure, so an English message will do
					wxMessageBox(_T(
"For the developer: MoveTextToFolderAndSave() in GetSourceTextFromEditor.cpp: Unexpected (non-fatal) error trying to move source text to a file in __SOURCE_INPUTS folder.\nThe free trans data won't have been saved to disk there."),
					_T(""), wxICON_WARNING);
				}
			} // end of TRUE block for test: if (::wxFileExists(docPath))
			else
			{
				// it doesn't exist, so we have to tokenize the source text coming from PT
				// or BE, create the document, save it and lay it out in the view window...
				wxASSERT(m_pApp->m_pSourcePhrases->IsEmpty());
				// parse the input file
				int nHowMany;
				SPList* pSourcePhrases = new SPList; // for storing the new tokenizations
				// in the next call, 0 = initial sequ number value
				nHowMany = pView->TokenizeTextString(pSourcePhrases, sourceChapterBuffer, 0); 
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
					delete pSourcePhrases;
					// the single-chapter document is now ready for displaying in the view window
				}

				SetupLayoutAndView(m_pApp, docTitle);

                // 5. Copy the just-grabbed chapter source text from the .temp folder over
                // to the Project's __SOURCE_INPUTS folder (creating the __SOURCE_INPUTS
                // folder if it doesn't already exist). Note, the source text in
                // sourceChapterBuffer has had its initial BOM removed. We never store the
                // BOM permanently. Internally MoveTextToFolderAndSave() will add .txt
                // extension to the docTitle string, to form the correct filename for
				// saving. Since the AI project already exists, the
				// m_sourceInputsFolderName member of the app class will point to an
				// already created __SOURCE_INPUTS folder in the project folder.
				wxString pathCreationErrors;
				bool bMovedOK = MoveTextToFolderAndSave(m_pApp, m_pApp->m_sourceInputsFolderPath, 
										pathCreationErrors, sourceChapterBuffer, docTitle);
				// don't expect a failure in this, but if so, tell developer (English
				// message is all that is needed)
				if (!bMovedOK)
				{
					wxMessageBox(_T(
"For the developer: MoveNewSourceTextToSOURCE_INPUTS() in GetSourceTextFromEditor.cpp: Unexpected (non-fatal) error trying to move the new source text to a file in __SOURCE_INPUTS.\nThe new source data won't have been saved to disk there."),
					_T(""), wxICON_WARNING);
				}

			} // end of else block for test: if (::wxFileExists(docPath))
		} // end of TRUE block for test: if (bSucceeded)
		else
		{
            // A very unexpected failure to hook up -- what should be done here? The KBs
            // are unloaded, but the app is in limbo - paths are current for a project
            // which didn't actually get set up, and so isn't actually current. But a
            // message has been seen (albeit, one for developer only as we don't expect
            // this error). I guess we just return from OnOK() and give a localizable 2nd
            // message tell the user to take the Cancel option & retry
			wxMessageBox(_(
"Unexpected failure to hook up to the identified pre-existing Adapt It adaptation project.\nYou should Cancel from the current collaboration attempt, then perhaps try again."),
			_T(""), wxICON_WARNING);
			return;
		}
	} // end of TRUE block for test: if (bPTCollaborationUsingExistingAIProject)
	else
	{
		// The Paratext project selected for source text and target texts do not yet exist
		// as a previously created AI project in the user's work folder, so we need to 
		// create the AI project using information contained in the PT_Project_Info_Struct structs
		// that are populated dynamically in InitDialog() which calls the App's 
		// GetListOfPTProjects().
		
		// Get the PT_Project_Info_Struct structs for the source and target PT projects
		PT_Project_Info_Struct* pPTInfoSrc;
		PT_Project_Info_Struct* pPTInfoTgt;
		pPTInfoSrc = m_pApp->GetPT_Project_Struct(shortProjNameSrc);  // gets pointer to the struct from the 
																	// pApp->m_pArrayOfPTProjects
		pPTInfoTgt = m_pApp->GetPT_Project_Struct(shortProjNameTgt);  // gets pointer to the struct from the 
																	// pApp->m_pArrayOfPTProjects
		wxASSERT(pPTInfoSrc != NULL);
		wxASSERT(pPTInfoTgt != NULL);

		// Notes: The pPTInfoSrc and pPTInfoTgt structs above contain the following struct members:
		//bool bProjectIsNotResource; // AI only includes in m_pArrayOfPTProjects the PT projects where this is TRUE
		//bool bProjectIsEditable; // AI only allows user to see and select a PT TARGET project where this is TRUE
		//wxString versification; // AI assumes same versification of Target as used in Source
		//wxString fullName; // This is 2nd field seen in "Get Source Texts from this project:" drop down
							 // selection (after first ':')
		//wxString shortName; // This is 1st field seen in "Get Source Texts from this project:" drop down
							  // selection (before first ':'). It is always the same as the project's folder
							  // name (and ssf file name) in the "My Paratext Projects" folder. It has to
							  // be unique for every PT project.
		//wxString languageName; // This is 3rd field seen in "Get Source Texts from this project:" drop down
							     // selection (between 2nd and 3rd ':'). We use this as x and y language
							     // names to form the "x to y adaptations" project folder name in AI if it
							     // does not already exist
		//wxString ethnologueCode; // If it exists, this a 3rd field seen in "Get Source Texts from this project:" drop down
							       // selection (after 3rd ':')
		//wxString projectDir; // this has the path/directory to the project's actual files
		//wxString booksPresentFlags; // this is the long 123-element string of 0 and 1 chars indicating which
									   // books are present within the PT project
		//wxString chapterMarker; // default is _T("c")
		//wxString verseMarker; // default is _T("v")
		//wxString defaultFont;  // default is _T("Arial")or whatever is specified in the PT project's ssf file
		//wxString defaultFontSize; // default is _T("10") or whatever is specified in the PT project's ssf file
		//wxString leftToRight; // default is _T("T") unless "F" is specified in the PT project's ssf file
		//wxString encoding; // default is _T("65001"); // 65001 is UTF8

		// For building an AI project we can use as initial values the information within the appropriate
		// field members of the above struct.
		// 
		// TODO:
		// 1. Create an AI project using the information from the PT structs, setting up
		//    the necessary directories and the appropriately constructed project config 
		//    file to disk.
		// 2. We need to decide if we will be using book folder mode automatically [ <- No!] 
		//    for chapter sized documents of if we will dump them all in the "adaptations" 
		//    folder [ <- Yes!]. The user won't know the difference except if the administrator 
		//    decides at some future time to turn PT collaboration OFF. If we used the 
		//    book folders during PT collaboration for chapter files, we would need to 
		//    ensure that book folder mode stays turned on when PT collaboration was 
		//    turned off.
		// 3. Compose an appropriate document name to be used for the document that will
		//    contain the chapter grabbed from the PT source project's book.
		// 4. Copy the just-grabbed chapter source text from the .temp folder over to the
		//    Project's __SOURCE_INPUTS folder (creating the __SOURCE_INPUTS folder if it doesn't
		//    already exist).
		// 5. Create the document by parsing/tokenizing the string now existing in our 
		//    sourceChapterBuffer, saving it's xml form to disk, and laying the doc out in 
		//    the main window.
		//    
		//    TODO: implement the above here
		
















	}  // end of else block for test: if (bPTCollaborationUsingExistingAIProject)

	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}

bool CGetSourceTextFromEditorDlg::PTProjectIsEditable(wxString projShortName)
{
	// check whether the projListItem has the <Editable>T</Editable> attribute which
	// we can just query our pPTInfo->bProjectIsEditable attribute for the project 
	// to see if it is TRUE or FALSE.
	PT_Project_Info_Struct* pPTInfo;
	pPTInfo = m_pApp->GetPT_Project_Struct(projShortName);  // gets pointer to the struct from the 
															// pApp->m_pArrayOfPTProjects
	if (pPTInfo != NULL && pPTInfo->bProjectIsEditable)
		return TRUE;
	else
		return FALSE;
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
			delete pPossibleProject;
		}
	}
	wxASSERT(pMatchedProject != NULL);
	return pMatchedProject;
}


// Attempts to find a pre-existing Adapt It project that adapts between the same language
// pair as is the case for the two Paratext or Bibledit project pairs which have been
// selected for this current collaboration. It first tries to find a match by building the
// "XXX to YYY adaptations" folder name using language names gleaned from the Paratext
// project struct; failing that, and only provided all the required ethnologue codes
// exist and are valid, it will try make the matchup by pairing the codes in Adapt It's projects with
// those from the Paratext or Bibledit projects - but only provided there is only one such
// matchup possible -- if there is any ambiguity, matchup is deemed to be
// (programmatically) unreliable and so FALSE is returned - in which case Adapt It would
// set up a brand new Adapt It project for handling the collaboration. The
// aiProjectFolderName and aiProjectFolderPath parameters return the matched Adapt It
// project folder's name and absolute path, and both are non-empty only when a valid match
// has been obtained, otherwise they return empty strings. shortProjNameSrc and
// shortProjNameTgt are input parameters - the short names for the Paratext or Bibledit
// projects which are involved in this collaboration.
bool CGetSourceTextFromEditorDlg::PTProjectsExistAsAIProject(wxString shortProjNameSrc, 
									wxString shortProjNameTgt, wxString& aiProjectFolderName,
									wxString& aiProjectFolderPath)
{
	aiProjectFolderName.Empty();
	aiProjectFolderPath.Empty();

	PT_Project_Info_Struct* pPTInfoSrc;
	PT_Project_Info_Struct* pPTInfoTgt;
	pPTInfoSrc = m_pApp->GetPT_Project_Struct(shortProjNameSrc);  // gets pointer to the struct from the 
																// pApp->m_pArrayOfPTProjects
	pPTInfoTgt = m_pApp->GetPT_Project_Struct(shortProjNameTgt);  // gets pointer to the struct from the 
																// pApp->m_pArrayOfPTProjects
	wxASSERT(pPTInfoSrc != NULL);
	wxASSERT(pPTInfoTgt != NULL);
	wxString srcLangStr = pPTInfoSrc->languageName;
	wxASSERT(!srcLangStr.IsEmpty());
	wxString tgtLangStr = pPTInfoTgt->languageName;
	wxASSERT(!tgtLangStr.IsEmpty());
	wxString projectFolderName = srcLangStr + _T(" to ") + tgtLangStr + _T(" adaptations");
	
	wxString workPath;
	// get the absolute path to "Adapt It Unicode Work" or "Adapt It Work" as the case may be
	// NOTE: m_bLockedCustomWorkFolderPath == TRUE is included in the test deliberately,
	// because without it, an (administrator) snooper might be tempted to access someone
	// else's remote shared Adapt It project and set up a PT or BE collaboration on his
	// behalf - we want to snip that possibility in the bud!! The snooper won't have this
	// boolean set TRUE, and so he'll be locked in to only being to collaborate from
	// what's on his own machine
	workPath = SetWorkFolderPath_For_Collaboration();

	wxArrayString possibleAIProjects;
	possibleAIProjects.Clear();
	m_pApp->GetPossibleAdaptionProjects(&possibleAIProjects);
	int ct, tot;
	tot = (int)possibleAIProjects.GetCount();
	if (tot == 0)
	{
		return FALSE;
	}
	else
	{
		for (ct = 0; ct < tot; ct++)
		{
			wxString tempStr = possibleAIProjects.Item(ct);
			if (projectFolderName == tempStr)
			{
				aiProjectFolderName = projectFolderName; // BEW added 21Jun11
				aiProjectFolderPath = workPath + m_pApp->PathSeparator + projectFolderName;
				return TRUE;
			}
		}
	}

	// BEW added 22Jun11, checking for a match based on 2- or 3-letter ethnologue codes
	// should also be attempted, if the language names don't match -- this will catch
	// projects where the language name(s) may have a typo, or a local spelling different
	// from the ethnologue standard name; checking for a names-based match first gives
	// some protection in the case of similar dialects, but we'd want to treat matched
	// source and target language codes as indicating a project match, I think -- but only
	// provided such a match is unique, if ambiguous (ie. two or more Adapt It projects
	// have the same pair of ethnologue codes) then don't guess, instead return FALSE so
	// that a new independent Adapt It project will get created instead
	EthnologueCodePair* pMatchedCodePair = NULL;
	wxString srcLangCode = pPTInfoSrc->ethnologueCode;
	if (srcLangCode.IsEmpty())
	{
		return FALSE; 
	}
	wxString tgtLangCode = pPTInfoTgt->ethnologueCode;
	if (tgtLangCode.IsEmpty())
	{
		return FALSE; 
	}
	if (!IsEthnologueCodeValid(srcLangCode) || !IsEthnologueCodeValid(tgtLangCode))
	{
		return FALSE;
	}
	else
	{
		// both codes from the PT or BE projects are valid, so try for a match with an
		// existing Adapt It project
		pMatchedCodePair = MatchAIProjectUsingEthnologueCodes(srcLangCode, tgtLangCode);
		if (pMatchedCodePair == NULL)
		{
			return FALSE;
		}
		else
		{
			// we have a successful match to an existing AI project
			aiProjectFolderName = pMatchedCodePair->projectFolderName;
			aiProjectFolderPath	= pMatchedCodePair->projectFolderPath;
			delete pMatchedCodePair;
		}
	}
	return TRUE;
}

bool CGetSourceTextFromEditorDlg::EmptyVerseRangeIncludesAllVersesOfChapter(wxString emptyVersesStr)
{
	// The incoming emptyVersesStr will be of the abbreviated form created by the
	// AbbreviateColonSeparatedVerses() function, i.e., "1, 2-6, 28-29" when the
	// chapter has been partly drafted, or "1-29" when all verses are empty. To 
	// determine whether the empty verse range includes all verses of the chapter
	// or not, we parse the incoming emptyVersesStr to see if it is of the later
	// "1-29" form. There will be a single '-' character and no comma delimiters.
	wxASSERT(!emptyVersesStr.IsEmpty());
	bool bAllVersesAreEmpty = FALSE;
	wxString tempStr = emptyVersesStr;
	// Check if there is no comma in the emptyVersesStr. Lack of a comma indicates
	// that all verses are empty
	if (tempStr.Find(',') == wxNOT_FOUND)
	{
		// There is no ',' that would indicate a gap in the emptyVersesStr
		bAllVersesAreEmpty = TRUE;
	}
	// Just to be sure do another test to ensure there is a '-' and only one '-'
	// in the string.
	if (tempStr.Find('-') != wxNOT_FOUND && tempStr.Find('-',FALSE) == tempStr.Find('-',TRUE))
	{
		// the position of '-' in the string is the same whether
		// determined from the left end or the right end of the string
		bAllVersesAreEmpty = TRUE;
	}

	return bAllVersesAreEmpty;
}

wxString CGetSourceTextFromEditorDlg::GetShortNameFromLBProjectItem(wxString LBProjItem)
{
	wxString ptProjShortName;
	ptProjShortName.Empty();
	int posColon;
	posColon = LBProjItem.Find(_T(':'));
	ptProjShortName = LBProjItem.Mid(0,posColon);
	ptProjShortName.Trim(FALSE);
	ptProjShortName.Trim(TRUE);
	return ptProjShortName;
}

void CGetSourceTextFromEditorDlg::GetChapterListAndVerseStatusFromTargetBook(wxString targetBookFullName, wxArrayString& chapterList, wxArrayString& statusList)
{
	// Called from OnLBBookSelected().
	// Retrieves a wxArrayString of chapters (and their status) from the target 
	// PT project's TargetTextUsfmStructureAndExtentArray. 
	wxArrayString chapterArray;
	wxArrayString statusArray;
	chapterArray.Clear();
	statusArray.Clear();
	m_staticBoxDescriptionArray.Clear();
	int ct,tot;
	tot = TargetTextUsfmStructureAndExtentArray.GetCount();
	wxString tempStr;
	bool bChFound = FALSE;
	bool bVsFound = FALSE;
	wxString projShortName = GetShortNameFromLBProjectItem(m_TempCollabProjectForTargetExports);
	PT_Project_Info_Struct* pPTInfo;
	pPTInfo = m_pApp->GetPT_Project_Struct(projShortName);  // gets pointer to the struct from the 
															// pApp->m_pArrayOfPTProjects
	wxASSERT(pPTInfo != NULL);
	wxString chMkr;
	wxString vsMkr;
	if (pPTInfo != NULL)
	{
		chMkr = _T("\\") + pPTInfo->chapterMarker;
		vsMkr = _T("\\") + pPTInfo->verseMarker;
	}
	else
	{
		chMkr = _T("\\c");
		vsMkr = _T("\\v");
	}
	// Check if the book lacks a \c (as might Philemon, 2 John, 3 John and Jude).
	// If the book has no chapter we give the chapterArray a chapter 1 and indicate
	// the status of that chapter.
	for (ct = 0; ct < tot; ct++)
	{
		tempStr = TargetTextUsfmStructureAndExtentArray.Item(ct);
		if (tempStr.Find(chMkr) != wxNOT_FOUND) // \c
			bChFound = TRUE;
		if (tempStr.Find(vsMkr) != wxNOT_FOUND) // \v
			bVsFound = TRUE;
	}
	
	if ((!bChFound && !bVsFound) || (bChFound && !bVsFound))
	{
		// The target book has no chapter and verse content to work with
		chapterList = chapterArray;
		statusList = statusArray;
		return; // caller will give warning message
	}

	wxString statusOfChapter;
	wxString nonDraftedVerses; // used to get string of non-drafted verse numbers/range below
	if (bChFound)
	{
		for (ct = 0; ct < tot; ct++)
		{
			tempStr = TargetTextUsfmStructureAndExtentArray.Item(ct);
			if (tempStr.Find(chMkr) != wxNOT_FOUND) // \c
			{
				// we're pointing at a \c element of a string that is of the form: "\c 1:0:MD5"
				// strip away the preceding \c and the following :0:MD5
				
				// Account for MD5 hash now at end of tempStr
				int posColon = tempStr.Find(_T(':'),TRUE); // TRUE - find from right end
				tempStr = tempStr.Mid(0,posColon); // get rid of the MD5 part and preceding colon (":0" in this case for \c markers)
				posColon = tempStr.Find(_T(':'),TRUE);
				tempStr = tempStr.Mid(0,posColon); // get rid of the char count part and preceding colon
				
				int posChMkr;
				posChMkr = tempStr.Find(chMkr); // \c
				wxASSERT(posChMkr == 0);
				int posAfterSp = tempStr.Find(_T(' '));
				wxASSERT(posAfterSp > 0);
				posAfterSp ++; // to point past space
				tempStr = tempStr.Mid(posAfterSp);
				tempStr.Trim(FALSE);
				tempStr.Trim(TRUE);
				int chNum;
				chNum = wxAtoi(tempStr);

				statusOfChapter = GetStatusOfChapter(TargetTextUsfmStructureAndExtentArray,ct,targetBookFullName,nonDraftedVerses);
				wxString listItemStatusSuffix,listItem;
				listItemStatusSuffix = statusOfChapter;
				listItem.Empty();
				listItem = targetBookFullName + _T(" ");
				listItem += tempStr;
				chapterArray.Add(listItem);
				statusArray.Add(statusOfChapter);
				// Store the description array info for this chapter in the m_staticBoxDescriptionArray.
				wxString emptyVsInfo;
				// remove padding spaces for the static box description
				tempStr.Trim(FALSE);
				tempStr.Trim(TRUE);
				emptyVsInfo = targetBookFullName + _T(" ") + _("chapter") + _T(" ") + tempStr + _T(" ") + _("details:") + _T(" ") + statusOfChapter + _T(". ") + _("The following verses are empty:") + _T(" ") + nonDraftedVerses;
				m_staticBoxDescriptionArray.Add(emptyVsInfo ); // return the empty verses string via the nonDraftedVerses ref parameter
			}
		}
	}
	else
	{
		// No chapter marker was found in the book, so just collect its verse 
		// information using an index for the "chapter" element of -1. We use -1
		// because GetStatusOfChapter() increments the assumed location of the \c 
		// line/element in the array before it starts collecting verse information
		// for that given chapter. Hence, it will actually start collecting verse
		// information with element 0 (the first element of the 
		// TargetTextUsfmStructureAndExtentArray)
		statusOfChapter = GetStatusOfChapter(TargetTextUsfmStructureAndExtentArray,-1,targetBookFullName,nonDraftedVerses);
		wxString listItemStatusSuffix,listItem;
		listItemStatusSuffix.Empty();
		listItem.Empty();
		listItemStatusSuffix = statusOfChapter;
		listItem += _T(" - ");
		listItem += listItemStatusSuffix;
		chapterArray.Add(_T("   ")); // arbitrary 3 spaces in first column
		statusArray.Add(listItem);
		// Store the description array info for this chapter in the m_staticBoxDescriptionArray.
		wxString chapterDetails;
		chapterDetails = targetBookFullName + _T(" ") + _("details:") + _T(" ") + statusOfChapter + _T(". ") + _("The following verses are empty:") + _T(" ") + nonDraftedVerses;
		m_staticBoxDescriptionArray.Add(chapterDetails ); // return the empty verses string via the nonDraftedVerses ref parameter
	}
	chapterList = chapterArray; // return array list via reference parameter
	statusList = statusArray; // return array list via reference parameter
}

void CGetSourceTextFromEditorDlg::LoadBookNamesIntoList()
{
	// clear lists and static text box at bottom of dialog
	pListBoxBookNames->Clear();
	pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
	pStaticTextCtrlNote->ChangeValue(_T(""));

	wxString ptProjShortName;
	ptProjShortName = GetShortNameFromLBProjectItem(m_TempCollabProjectForSourceInputs);
	int ct, tot;
	tot = (int)m_pApp->m_pArrayOfPTProjects->GetCount();
	wxString tempStr;
	PT_Project_Info_Struct* pArrayItem;
	pArrayItem = (PT_Project_Info_Struct*)NULL;
	bool bFound = FALSE;
	for (ct = 0; ct < tot; ct++)
	{
		pArrayItem = (PT_Project_Info_Struct*)(*m_pApp->m_pArrayOfPTProjects)[ct];
		tempStr = pArrayItem->shortName;
		if (tempStr == ptProjShortName)
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
	booksPresentArray = m_pApp->GetBooksArrayFromPTFlags(booksStr);
	if (booksPresentArray.GetCount() == 0)
	{
		wxString msg1,msg2;
		msg1 = msg1.Format(_("The Paratext project (%s) selected for obtaining source texts contains no books."),m_TempCollabProjectForSourceInputs.c_str());
		msg2 = _("Please select the Paratext Project that contains the source texts you will use for adaptation work.");
		msg1 = msg1 + _T(' ') + msg2;
		wxMessageBox(msg1,_T("No books found"),wxICON_WARNING);
		// clear lists and static text box at bottom of dialog
		pListBoxBookNames->Clear();
		pListCtrlChapterNumberAndStatus->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too
		pStaticTextCtrlNote->ChangeValue(_T(""));
	}
	pListBoxBookNames->Append(booksPresentArray);

}

void CGetSourceTextFromEditorDlg::ExtractVerseNumbersFromBridgedVerse(wxString tempStr,int& nLowerNumber,int& nUpperNumber)
{
	// The incoming tempStr will have a bridged verse string in the form "3-5".
	// We parse it and convert its lower and upper number sub-strings into int values to
	// return to the caller via the reference int parameters.
	int nPosDash;
	wxString str = tempStr;
	str.Trim(FALSE);
	str.Trim(TRUE);
	wxString subStrLower, subStrUpper;
	nPosDash = str.Find('-',TRUE);
	wxASSERT(nPosDash != wxNOT_FOUND);
	subStrLower = str.Mid(0,nPosDash);
	subStrLower.Trim(TRUE); // trim any whitespace at right end
	subStrUpper = str.Mid(nPosDash+1);
	subStrUpper.Trim(FALSE); // trim any whitespace at left end
	nLowerNumber = wxAtoi(subStrLower);
	nUpperNumber = wxAtoi(subStrUpper);
}

wxString CGetSourceTextFromEditorDlg::GetStatusOfChapter(const wxArrayString &TargetArray,int indexOfChItem,
													wxString targetBookFullName,wxString& nonDraftedVerses)
{
	// When this function is called from GetChapterListAndVerseStatusFromTargetBook() the
	// indexOfChItem parameter is pointing at a \c n:nnnn line in the TargetArray.
	// We want to get the status of the chapter by examining each verse of that chapter to see if it 
	// has content. The returned string will have one of these three values:
	//    1. "Drafted (all nn verses have content)"
	//    2. "Partly drafted (nn of a total of mm verses have content)"
	//    3. "Empty (no verses of a total of mm have content yet)"
	// When the returned string is 2 above ("Partly drafted...") the reference parameter nonDraftedVerses
	// will contain a string describing what verses/range of verses are empty, i.e. "The following 
	// verses are empty: 12-14, 20, 21-29".
	
	// Scan forward in the TargetArray until we reach the next \c element. For each \v we encounter
	// we examine the text extent of that \v element. When a particular \v element is empty we build
	// a string list of verse numbers that are empty, and collect verse counts for the number of
	// verses which have content and the total number of verses. These are used to construct the
	// string return values and the nonDraftedVerses reference parameter - so that the Select a chapter
	// list box will display something like:
	// Mark 3 - "Partly drafted (13 of a total of 29 verses have content)"
	// and the nonDraftedVerses string will contain: "12-14, 20, 21-29" which can be used in the 
	// read-only text control box at the bottom of the dialog used for providing more detailed 
	// information about the book or chapter selected.

	int nLastVerseNumber = 0;
	int nVersesWithContent = 0;
	int nVersesWithoutContent = 0;
	int index = indexOfChItem;
	int tot = (int)TargetArray.GetCount();
	wxString tempStr;
	wxString statusStr;
	statusStr.Empty();
	wxString emptyVersesStr;
	emptyVersesStr.Empty();
	wxString projShortName = GetShortNameFromLBProjectItem(m_TempCollabProjectForTargetExports);
	PT_Project_Info_Struct* pPTInfo;
	pPTInfo = m_pApp->GetPT_Project_Struct(projShortName);  // gets pointer to the struct from the 
															// pApp->m_pArrayOfPTProjects
	wxASSERT(pPTInfo != NULL);
	wxString chMkr;
	wxString vsMkr;
	if (pPTInfo != NULL)
	{
		chMkr = _T("\\") + pPTInfo->chapterMarker;
		vsMkr = _T("\\") + pPTInfo->verseMarker;
	}
	else
	{
		chMkr = _T("\\c");
		vsMkr = _T("\\v");
	}
	
	if (indexOfChItem == -1)
	{
		// When the incoming indexOfChItem parameter is -1 it indicates that this is
		// a book that does not have a chapter \c marker, i.e., Philemon, 2 John, 3 John, Jude
		do
		{
			index++; // point to first and succeeding lines
			if (index >= tot)
				break;
			tempStr = TargetArray.Item(index);
			if (tempStr.Find(vsMkr) == 0) // \v
			{
				// We're at a verse, so note if it is empty.
				// But, first get the last verse number in the tempStr. If the last verse 
				// is a bridged verse, store the higher verse number of the bridge, otherwise 
				// store the single verse number in nLastVerseNumber.
				// tempStr is of the form "\v n" or possibly "\v n-m" if bridged
				wxString str = GetNumberFromChapterOrVerseStr(tempStr);
				if (str.Find('-',TRUE) != wxNOT_FOUND)
				{
					// The tempStr has a dash in it indicating it is a bridged verse, so
					// store the higher verse number of the bridge.
					int nLowerNumber, nUpperNumber;
					ExtractVerseNumbersFromBridgedVerse(str,nLowerNumber,nUpperNumber);
					
					nLastVerseNumber = nUpperNumber;
				}
				else
				{
					// The tempStr is a plain number, not a bridged one, so just store
					// the number.
					nLastVerseNumber = wxAtoi(str);
				}
				// now determine if the verse is empty or not
				int posColon = tempStr.Find(_T(':'),TRUE); // TRUE - find from right end
				wxASSERT(posColon != wxNOT_FOUND);
				wxString extent;
				extent = tempStr.Mid(posColon + 1);
				extent.Trim(FALSE);
				extent.Trim(TRUE);
				if (extent == _T("0"))
				{
					// The verse is empty so get the verse number, and add it
					// to the emptyVersesStr
					emptyVersesStr += GetNumberFromChapterOrVerseStr(tempStr);
					emptyVersesStr += _T(':'); 
					// calculate the nVersesWithoutContent value
					if (str.Find('-',TRUE) != wxNOT_FOUND)
					{
						int nLowerNumber, nUpperNumber;
						ExtractVerseNumbersFromBridgedVerse(str,nLowerNumber,nUpperNumber);
						nVersesWithoutContent += (nUpperNumber - nLowerNumber + 1);
					}
					else
					{
						nVersesWithoutContent++;
					}
				}
				else
				{
					// The verse has content so calculate the number of verses to add to
					// nVersesWithContent
					if (str.Find('-',TRUE) != wxNOT_FOUND)
					{
						int nLowerNumber, nUpperNumber;
						ExtractVerseNumbersFromBridgedVerse(str,nLowerNumber,nUpperNumber);
						nVersesWithContent += (nUpperNumber - nLowerNumber + 1);
					}
					else
					{
						nVersesWithContent++;
					}
				}
			}
		} while (index < tot);
	}
	else
	{
		// this book has at least one chapter \c marker
		bool bStillInChapter = TRUE;
		do
		{
			index++; // point past this chapter
			if (index >= tot)
				break;
			tempStr = TargetArray.Item(index);
			
			if (index < tot && tempStr.Find(chMkr) == 0) // \c
			{
				// we've encountered a new chapter
				bStillInChapter = FALSE;
			}
			else if (index >= tot)
			{
				bStillInChapter = FALSE;
			}
			else
			{
				if (tempStr.Find(vsMkr) == 0) // \v
				{
					// We're at a verse, so note if it is empty.
					// But, first get the last verse number in the tempStr. If the last verse 
					// is a bridged verse, store the higher verse number of the bridge, otherwise 
					// store the single verse number in nLastVerseNumber.
					// tempStr is of the form "\v n:nnnn" or possibly "\v n-m:nnnn" if bridged
					
					// testing only below - uncomment to test the if-else block below
					//tempStr = _T("\\ v 3-5:0");
					// testing only above
					
					wxString str = GetNumberFromChapterOrVerseStr(tempStr);
					if (str.Find('-',TRUE) != wxNOT_FOUND)
					{
						// The tempStr has a dash in it indicating it is a bridged verse, so
						// store the higher verse number of the bridge.

						int nLowerNumber, nUpperNumber;
						ExtractVerseNumbersFromBridgedVerse(str,nLowerNumber,nUpperNumber);
						
						nLastVerseNumber = nUpperNumber;
					}
					else
					{
						// The tempStr is a plain number, not a bridged one, so just store
						// the number.
						nLastVerseNumber = wxAtoi(str);
					}
					// now determine if the verse is empty or not
					int posColon = tempStr.Find(_T(':'),TRUE); // TRUE - find from right end
					wxASSERT(posColon != wxNOT_FOUND);
					tempStr = tempStr.Mid(0,posColon); // get rid of the MD5 part and preceding colon (":0" in this case for \v markers)
					posColon = tempStr.Find(_T(':'),TRUE); // TRUE - find from right end
					wxString extent;
					extent = tempStr.Mid(posColon + 1);
					extent.Trim(FALSE);
					extent.Trim(TRUE);
					if (extent == _T("0"))
					{
						// The verse is empty so get the verse number, and add it
						// to the emptyVersesStr
						emptyVersesStr += GetNumberFromChapterOrVerseStr(tempStr);
						emptyVersesStr += _T(':');
						// calculate the nVersesWithoutContent value
						if (str.Find('-',TRUE) != wxNOT_FOUND)
						{
							int nLowerNumber, nUpperNumber;
							ExtractVerseNumbersFromBridgedVerse(str,nLowerNumber,nUpperNumber);
							nVersesWithoutContent += (nUpperNumber - nLowerNumber + 1);
						}
						else
						{
							nVersesWithoutContent++;
						}
					}
					else
					{
						// The verse has content so calculate the number of verses to add to
						// nVersesWithContent
						if (str.Find('-',TRUE) != wxNOT_FOUND)
						{
							int nLowerNumber, nUpperNumber;
							ExtractVerseNumbersFromBridgedVerse(str,nLowerNumber,nUpperNumber);
							nVersesWithContent += (nUpperNumber - nLowerNumber + 1);
						}
						else
						{
							nVersesWithContent++;
						}
					}
				}
			}
		} while (index < tot  && bStillInChapter);
	}
	// Shorten any emptyVersesStr by indicating continuous empty verses with a dash between
	// the starting and ending of continuously empty verses, i.e., 5-7, and format the list with
	// comma and space between items so that the result would be something like: 
	// "1, 3, 5-7, 10-22" which is returned via nonDraftedVerses parameter to the caller.
	
	// testing below
	//wxString wxStr1 = _T("1:3:4:5:6:9:10-12:13:14:20:22:24:25:26:30:");
	//wxString wxStr2 = _T("1:2:3:4:5:6:7:8:9:10:11:12:13:14:15:16:17:18:19:20:21:22:23:24:25:26:27:28:29:30:");
	//wxString testStr1;
	//wxString testStr2;
	//testStr1 = AbbreviateColonSeparatedVerses(wxStr1);
	//testStr2 = AbbreviateColonSeparatedVerses(wxStr2);
	// test results: testStr1 = _T("1, 3-6, 9, 10-12, 13-14, 20, 22, 24-26, 30")
	// test results: testStr2 = _T("1-30")
	// testing above
	
	if (!emptyVersesStr.IsEmpty())
	{
		// Handle the "partially drafted" and "empty" cases.
		// The emptyVersesStr has content, so there are one or more empty verses in the 
		// chapter, including the possibility that all verses have content.
		// Return the results to the caller via emptyVersesStr parameter and the 
		// function's return value.
		emptyVersesStr = AbbreviateColonSeparatedVerses(emptyVersesStr);
		if (EmptyVerseRangeIncludesAllVersesOfChapter(emptyVersesStr))
		{
			statusStr = statusStr.Format(_("Empty (no verses of a total of %d have content yet)"),nLastVerseNumber);
		}
		else
		{
			statusStr = statusStr.Format(_("Partly drafted (%d of a total of %d verses have content)"),nVersesWithContent,nLastVerseNumber);
		}
		nonDraftedVerses = emptyVersesStr;
	}
	else
	{
		// Handle the "fully drafted" case.
		// The emptyVersesStr has NO content, so there are no empty verses in the chapter. The
		// emptyVersesStr ref parameter remains empty.
		statusStr = statusStr.Format(_("Drafted (all %d verses have content)"),nLastVerseNumber);
		nonDraftedVerses = _T("");
	}

	
	return statusStr; // return the status string that becomes the list item's status in the caller
}

wxString CGetSourceTextFromEditorDlg::AbbreviateColonSeparatedVerses(const wxString str)
{
	// Abbreviates a colon separated list of verses that originally looks like:
	// 1:3:4:5:6:9:10-12:13:14:20:22:24:25,26:30:
	// changing it to this abbreviated from:
	// 1, 3-6, 9, 10-12, 13-14, 20, 22, 24-26, 30
	// Note: Bridged verses in the original are not combined with their contiguous 
	// neighbors, so 9, 10-12, 13-14 does not become 9-14.
	wxString tempStr;
	tempStr.Empty();
	wxStringTokenizer tokens(str,_T(":"),wxTOKEN_DEFAULT); // empty tokens are never returned
	wxString aToken;
	int lastVerseValue = 0;
	int currentVerseValue = 0;
	bool bBridgingVerses = FALSE;
	while (tokens.HasMoreTokens())
	{
		aToken = tokens.GetNextToken();
		aToken = aToken.Trim(FALSE); // FALSE means trim white space from left end
		aToken = aToken.Trim(TRUE); // TRUE means trim white space from right end
		int len = aToken.Length();
		int ct;
		bool bHasNonDigitChar = FALSE;
		for (ct = 0; ct < len; ct++)
		{
			if (!wxIsdigit(aToken.GetChar(ct)) && aToken.GetChar(ct) != _T('-'))
			{
				// the verse has a non digit char other than a '-' char so let it stand by
				// itself
				bHasNonDigitChar = TRUE;
			}
		}
		if (aToken.Find(_T('-')) != wxNOT_FOUND || bHasNonDigitChar)
		{
			// the token is a bridged verse number string, i.e., 2-3
			// or has an unrecognized non-digit char in it, so we let 
			// it stand by itself in the abbriviated tempStr
			tempStr += _T(", ") + aToken;
			bBridgingVerses = FALSE;
		}
		else
		{
			// the token is a normal verse number string
			currentVerseValue = wxAtoi(aToken);
			if (lastVerseValue == 0)
			{
				// we're at the first verse element, which will always get stored as is
				tempStr = aToken;
			}
			else if (currentVerseValue - lastVerseValue == 1)
			{
				// the current verse is in sequence with the last verse, continue 
				bBridgingVerses = TRUE;
			}
			else
			{
				// the currenttVerseValue and lastVerseValue are not contiguous
				if (bBridgingVerses)
				{
					tempStr += _T('-');
					tempStr << lastVerseValue;
					tempStr += _T(", ") + aToken;
				}
				else
				{
					tempStr += _T(", ") + aToken;
				}
				bBridgingVerses = FALSE;
			}
			lastVerseValue = currentVerseValue;
		}
	}
	if (bBridgingVerses)
	{
		// close off end verse of the bridge at the end
		tempStr += _T('-');
		tempStr << lastVerseValue;
	}

	return tempStr;
}

void CGetSourceTextFromEditorDlg::OnRadioBoxSelected(wxCommandEvent& WXUNUSED(event))
{
	if (pRadioBoxWholeBookOrChapter->GetSelection() == 1)
	{
		// The user selected "Get Whole Book" disable the "Select a chapter" list and label
		pListCtrlChapterNumberAndStatus->Disable();
		pStaticSelectAChapter->Disable();
	}
	else
	{
		// The user selected "Get Chapter Only"
		pListCtrlChapterNumberAndStatus->Enable(TRUE);
		pStaticSelectAChapter->Enable(TRUE);
	}
}

wxString CGetSourceTextFromEditorDlg::GetBareChFromLBChSelection(wxString lbChapterSelected)
{
	// the incoming lbChapterSelected is from the "Select a chapter" list and is of the form:
	// "Acts 2 - Empty(no verses of a total of n have content yet)"
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



