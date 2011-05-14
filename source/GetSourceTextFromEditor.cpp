/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GetSourceTextFromEditorDlg.cpp
/// \author			Bill Martin
/// \date_created	10 April 2011
/// \date_revised	10 April 2011
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

#include "Adapt_It.h"
#include "helpers.h"
#include "GetSourceTextFromEditor.h"

extern wxChar gSFescapechar; // the escape char used for start of a standard format marker

// event handler table
BEGIN_EVENT_TABLE(CGetSourceTextFromEditorDlg, AIModalDialog)
	EVT_INIT_DIALOG(CGetSourceTextFromEditorDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_COMBOBOX(ID_COMBO_SOURCE_PT_PROJECT_NAME, CGetSourceTextFromEditorDlg::OnComboBoxSelectSourceProject)
	EVT_COMBOBOX(ID_COMBO_DESTINATION_PT_PROJECT_NAME, CGetSourceTextFromEditorDlg::OnComboBoxSelectDestinationProject)
	EVT_BUTTON(wxID_OK, CGetSourceTextFromEditorDlg::OnOK)
	EVT_LISTBOX(ID_LISTBOX_BOOK_NAMES, CGetSourceTextFromEditorDlg::OnLBBookSelected)
	EVT_LISTBOX(ID_LISTBOX_CHAPTER_NUMBER_AND_STATUS, CGetSourceTextFromEditorDlg::OnLBChapterSelected)
	EVT_LISTBOX_DCLICK(ID_LISTBOX_CHAPTER_NUMBER_AND_STATUS, CGetSourceTextFromEditorDlg::OnLBDblClickChapterSelected)
	EVT_RADIOBOX(ID_RADIOBOX_WHOLE_BOOK_OR_CHAPTER, CGetSourceTextFromEditorDlg::OnRadioBoxSelected)

	//EVT_MENU(ID_SOME_MENU_ITEM, CGetSourceTextFromEditorDlg::OnDoSomething)
	//EVT_UPDATE_UI(ID_SOME_MENU_ITEM, CGetSourceTextFromEditorDlg::OnUpdateDoSomething)
	//EVT_BUTTON(ID_SOME_BUTTON, CGetSourceTextFromEditorDlg::OnDoSomething)
	//EVT_CHECKBOX(ID_SOME_CHECKBOX, CGetSourceTextFromEditorDlg::OnDoSomething)
	//EVT_RADIOBUTTON(ID_SOME_RADIOBUTTON, CGetSourceTextFromEditorDlg::DoSomething)
	//EVT_TEXT(IDC_SOME_EDIT_CTRL, CGetSourceTextFromEditorDlg::OnEnChangeEditSomething)
	// ... other menu, button or control events
END_EVENT_TABLE()

CGetSourceTextFromEditorDlg::CGetSourceTextFromEditorDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Get Source Text from Paratext Project"),
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
	
	pComboSourceProjectName = (wxComboBox*)FindWindowById(ID_COMBO_SOURCE_PT_PROJECT_NAME);
	wxASSERT(pComboSourceProjectName != NULL);

	pComboDestinationProjectName = (wxComboBox*)FindWindowById(ID_COMBO_DESTINATION_PT_PROJECT_NAME);
	wxASSERT(pComboDestinationProjectName != NULL);

	pRadioBoxWholeBookOrChapter = (wxRadioBox*)FindWindowById(ID_RADIOBOX_WHOLE_BOOK_OR_CHAPTER);
	wxASSERT(pRadioBoxWholeBookOrChapter != NULL);

	pListBoxBookNames = (wxListBox*)FindWindowById(ID_LISTBOX_BOOK_NAMES);
	wxASSERT(pListBoxBookNames != NULL);

	pListBoxChapterNumberAndStatus = (wxListBox*)FindWindowById(ID_LISTBOX_CHAPTER_NUMBER_AND_STATUS);
	wxASSERT(pListBoxChapterNumberAndStatus != NULL);

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
	
}

void CGetSourceTextFromEditorDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	m_TempPTProjectForSourceInputs = m_pApp->m_PTProjectForSourceInputs;
	m_TempPTProjectForTargetExports = m_pApp->m_PTProjectForTargetExports;
	m_TempPTBookSelected = m_pApp->m_PTBookSelected;
	m_TempPTChapterSelected = m_pApp->m_PTChapterSelected;

	// determine the path and name to rewrtp7.exe
	// Note: Nathan M says that when we've tweaked rdwrtp7.exe to our satisfaction that he will
	// insure that it gets distributed with future versions of Paratext 7.x. Since AI version 6
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
		// rdwrtp7.exe does not exist in the Paratext installation, so use our in AI's install folder
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
		// Program Files (if the system will let me to it programmatically).
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
	// can be sure that some checks have been done to insure that Paratext is installed,
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
		}
	}
	bool bSourceProjFound = FALSE;
	int nIndex = -1;
	if (!m_TempPTProjectForSourceInputs.IsEmpty())
	{
		nIndex = pComboSourceProjectName->FindString(m_TempPTProjectForSourceInputs);
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
	if (!m_TempPTProjectForTargetExports.IsEmpty())
	{
		nIndex = pComboDestinationProjectName->FindString(m_TempPTProjectForTargetExports);
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

	if (!bTwoOrMorePTProjectsInList)
	{
		// This error is not likely to happen so use English message
		wxString str;
		str = _T("Your administrator has configured Adapt It to collaborate with Paratext.\nBut Paratext does not have at least two projects available for use by Adapt It.\nPlease aask your administrator to set up the necessary Paratext projects.\nAdapt It will now abort...");
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
		strProjectNotSel += _("Choose a project to use for Transferring Translation Texts");
	}
	
	if (!bSourceProjFound || !bTargetProjFound)
	{
		wxString str;
		str = str.Format(_("Select Paratext Project(s) by clicking on the drop-down lists at the top of the next dialog.\nYou need to do the following before you can begin working:%s"),strProjectNotSel.c_str());
		wxMessageBox(str, _T("Select Paratext projects that Adapt It will use"), wxICON_ERROR);
		
	}
	else
	{
		LoadBookNamesIntoList();

		// select LastPTBookSelected 
		if (!m_TempPTBookSelected.IsEmpty())
		{
			int nSel = pListBoxBookNames->FindString(m_TempPTBookSelected);
			if (nSel != wxNOT_FOUND)
			{
				pListBoxBookNames->SetSelection(nSel);
				// set focus on the Select a book list (OnLBBookSelected call below may change focus to Select a chapter list)
				pListBoxBookNames->SetFocus(); 
				wxCommandEvent evt;
				OnLBBookSelected(evt);
			}
		}

		// TODO:
		// Initialize/populate the "Select a chapter:" list
		
		// TODO:
		// select LastPTChapterSelected 
		
	}

 }

// event handling functions

void CGetSourceTextFromEditorDlg::OnComboBoxSelectSourceProject(wxCommandEvent& WXUNUSED(event))
{
	int nSel;
	nSel = pComboSourceProjectName->GetSelection();
	m_TempPTProjectForSourceInputs = pComboSourceProjectName->GetString(nSel);
	// when the selection changes for the Source project we need to reload the
	// "Select a book" list.
	LoadBookNamesIntoList(); // uses the m_TempPTProjectForSourceInputs
}

void CGetSourceTextFromEditorDlg::OnComboBoxSelectDestinationProject(wxCommandEvent& WXUNUSED(event))
{
	int nSel;
	wxString selStr;
	nSel = pComboDestinationProjectName->GetSelection();
	m_TempPTProjectForTargetExports = pComboDestinationProjectName->GetString(nSel);
	wxCommandEvent evt;
	OnLBBookSelected(evt);
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
	//    the marker is collected and stored in two wxArrayStrings, one marker (and count) per
	//    array element.
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
	// a colon (:), followed by a character count nnnn.
	// For example, here are the first 18 elements of a sample wxArraySting:
	//    \id:50
	//    \c 1:0
	//    \s:56
	//    \p:0
	//    \v 1:69
	//    \v 2:40
	//    \v 3:107
	//    \v 4:178
	//    \v 5:218
	//    \p:0
	//    \v 6:188
	//    \v 7:183
	//    \v 8:85
	//    \s:15
	//    \p:0
	//    \v 9:93
	//    \v 10:133
	//    \v 11:124
	//    ...
	// The character count of all non eol characters occurring after the marker and before the
	// next marker (or end of file). Bridged verses might look like the following:
	// \v 23-25:nnnn
	// We also need to call rdwrtp7 to get a copy of the target text book (if it exists). We do the
	// same scan on it collecting an wxArrayString of values that tell us what chapters exist and have
	// been translated.
	// 
	// Usage: rdwrtp7 -r|-w project book chapter|0 fileName
	wxString bookCode;
	bookCode = m_pApp->GetBookCodeFromBookName(fullBookName);
	// insure that a .temp folder exists in the m_workFolderPath
	wxString tempFolder;
	tempFolder = m_pApp->m_workFolderPath + m_pApp->PathSeparator + _T(".temp");
	if (!::wxDirExists(tempFolder))
	{
		::wxMkdir(tempFolder);
	}

	wxString sourceProjShortName;
	wxString targetProjShortName;
	wxASSERT(!m_TempPTProjectForSourceInputs.IsEmpty());
	wxASSERT(!m_TempPTProjectForTargetExports.IsEmpty());
	sourceProjShortName = GetShortNameFromLBProjectItem(m_TempPTProjectForSourceInputs);
	targetProjShortName = GetShortNameFromLBProjectItem(m_TempPTProjectForTargetExports);
	wxString bookNumAsStr = m_pApp->GetBookNumberAsStrFromName(fullBookName);
	wxString sourceTempFileName = tempFolder + m_pApp->PathSeparator + bookNumAsStr + bookCode + _T("_") + sourceProjShortName + _T(".tmp");
	wxString targetTempFileName = tempFolder + m_pApp->PathSeparator + bookNumAsStr + bookCode + _T("_") + targetProjShortName + _T(".tmp");
	
	
	// Build the command line for  reading the source PT project with rdwrtp7.exe.
	wxString commandLineSrc,commandLineTgt;
	commandLineSrc = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + sourceProjShortName + _T(" ") + bookCode + _T(" ") + _T("0") + _T(" ") + _T("\"") + sourceTempFileName + _T("\"");
	commandLineTgt = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + targetProjShortName + _T(" ") + bookCode + _T(" ") + _T("0") + _T(" ") + _T("\"") + targetTempFileName + _T("\"");
	wxLogDebug(commandLineSrc);

	/*
	// Looking at the wxExecute() source code in the 2.8.11 library, it is clear that
	// when the overloaded version of wxExecute() is used, it uses the the redirection
	// of the stdio to the arrays, and with that redirection, it doesn't show the 
	// console process window by default. It is distracting to have the DOS console
	// window flashing even momentarily, so we will use that overloaded version of 
	// rdwrtp7.exe.
	// This code snippit is from the wxWidgets' exec sample and shows how to call the
	// overloaded version.
    wxArrayString output, errors;
    int code = wxExecute(cmd, output, errors);
    wxLogStatus(_T("command '%s' terminated with exit code %d."),
                cmd.c_str(), code);
	*/

	long resultSrc = -1;
	long resultTgt = -1;
    wxArrayString outputSrc, errorsSrc;
	wxArrayString outputTgt, errorsTgt;
	// Use the wxExecute() override that takes the two wxStringArray parameters. This
	// also redirects the output and suppresses the dos console window.
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
	sourceFileBuffer = wxString(pSourceByteBuf,wxConvUTF8,fileLenSrc);
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
	targetFileBuffer = wxString(pTargetByteBuf,wxConvUTF8,fileLenTgt);
	// Note: the wxConvUTF8 parameter above works UNICODE builds and does nothing
	// in ANSI builds so this should work for both ANSI and Unicode data.
	free((void*)pTargetByteBuf);

	SourceTextUsfmStructureAndExtentArray.Clear();
	SourceTextUsfmStructureAndExtentArray = GetUsfmStructureAndExtent(sourceFileBuffer);
	TargetTextUsfmStructureAndExtentArray = GetUsfmStructureAndExtent(targetFileBuffer);
	
	// Note: The sourceFileBuffer and targetFileBuffer will not be completely empty even
	// if no Paratext book yet exists, because there will be a FEFF UTF-16 BOM char in it
	// after rdwrtp7.exe tries to copy the file and the result is stored in the wxString
	// buffer. So, we can tell better whether the book hasn't been created within Paratext
	// by checking to see if there are any elements in the appropriate 
	// UsfmStructureAndExtentArrays.
	if (SourceTextUsfmStructureAndExtentArray.GetCount() == 0)
	{
		wxString msg1,msg2;
		msg1 = msg1.Format(_("The book %s in the Paratext project for obtaining source texts (%s) has no chapter and verse numbers."),fullBookName.c_str(),sourceProjShortName.c_str());
		msg2 = msg2.Format(_("Please run Paratext and select the %s project. Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and insure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again."),sourceProjShortName.c_str());
		msg1 = msg1 + _T(' ') + msg2;
		wxMessageBox(msg1,_("No chapters and verses found"),wxICON_WARNING);
		pListBoxBookNames->SetSelection(-1); // remove any selection
		pBtnCancel->SetFocus();
		return;
	}
	if (TargetTextUsfmStructureAndExtentArray.GetCount() == 0)
	{
		wxString msg1,msg2;
		msg1 = msg1.Format(_("The book %s in the Paratext project for storing translation texts (%s) has no chapter and verse numbers."),fullBookName.c_str(),targetProjShortName.c_str());
		msg2 = msg2.Format(_("Please run Paratext and select the %s project. Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and insure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again."),targetProjShortName.c_str());
		msg1 = msg1 + _T(' ') + msg2;
		wxMessageBox(msg1,_("No chapters and verses found"),wxICON_WARNING);
		pListBoxBookNames->SetSelection(-1); // remove any selection
		pBtnCancel->SetFocus();
		return;
	}

	pListBoxChapterNumberAndStatus->Clear();
	wxArrayString chapterListFromTargetBook;
	chapterListFromTargetBook = GetChapterListFromTargetBook(fullBookName);
	if (chapterListFromTargetBook.GetCount() == 0)
	{
		wxString msg1,msg2;
		msg1 = msg1.Format(_("The book %s in the Paratext project for storing translation texts (%s) has no chapter and verse numbers."),fullBookName.c_str(),targetProjShortName.c_str());
		msg2 = msg2.Format(_("Please run Paratext and select the %s project. Then select \"Create Book(s)\" from the Paratext Project menu. Choose the book(s) to be created and insure that the \"Create with all chapter and verse numbers\" option is selected. Then return to Adapt It and try again."),targetProjShortName.c_str());
		msg1 = msg1 + _T(' ') + msg2;
		wxMessageBox(msg1,_("No chapters and verses found"),wxICON_WARNING);
		pListBoxChapterNumberAndStatus->Append(_("No chapters and verses found"));
		pListBoxChapterNumberAndStatus->Enable(FALSE);
		pBtnCancel->SetFocus();
		return;
	}
	else
	{
		pListBoxChapterNumberAndStatus->Append(chapterListFromTargetBook);
		pListBoxChapterNumberAndStatus->Enable(TRUE);
	}
	pStaticSelectAChapter->SetLabel(_("Select a &chapter:")); // put & char at same position as in the string in wxDesigner
	pStaticSelectAChapter->Refresh();

	if (!m_TempPTChapterSelected.IsEmpty())
	{
		int nSel = pListBoxChapterNumberAndStatus->FindString(m_TempPTChapterSelected);
		if (nSel != wxNOT_FOUND)
		{
			pListBoxChapterNumberAndStatus->SetSelection(nSel);
			pListBoxChapterNumberAndStatus->SetFocus(); // focus is set on Select a chapter
		}
	}
	
	wxASSERT(pListBoxBookNames->GetSelection() != wxNOT_FOUND);
	m_TempPTBookSelected = pListBoxBookNames->GetStringSelection();
	
	// TODO: Update the wxTextCtrl at the bottom of the dialog with more detailed
	// info about the book and/or chapter that is selected. We could use the
	// wxTextCtrl to explain why the "Get Whole Book" radio button is disabled
	// in circumstances whenever there are too many discontinuous untranslated 
	// parts making whole book adapting too tedious and adapting chapter-by-chapter
	// the allowed option. TODO: Discuss this restriction further with Bruce who
	// has suggested that we only allow whole-book adaptation when the whole book 
	// has no adapted content in any chapters.
}

void CGetSourceTextFromEditorDlg::OnLBChapterSelected(wxCommandEvent& WXUNUSED(event))
{
	// This handler is called both for single click and double clicks on an
	// item in the chapter list box, since a double click is sensed initially
	// as a single click. Therefore, this handler should only handle the changes
	// in the dialog's interface that need to be updated (the info text box at
	// the bottom). It should not provide the setup necessary to access the AI 
	// project represented by the PT source and target projects, setup KB's,
	// etc. - that setup should be done in the OnOK() handler - which is invoked
	// by the OnLBDblClickChapterSelected() handler.
	
	// TODO: Update the wxTextCtrl at the bottom of the dialog with more detailed
	// info about the book and/or chapter that is selected. We could use the
	// wxTextCtrl to explain why the "Get Whole Book" radio button is disabled
	// in circumstances whenever there are too many discontinuous untranslated 
	// parts making whole book adapting too tedious and adapting chapter-by-chapter
	// the allowed option. TODO: Discuss this restriction further with Bruce who
	// has suggested that we only allow whole-book adaptation when the whole book 
	// has no adapted content in any chapters.

	if (pListBoxChapterNumberAndStatus->GetSelection() != wxNOT_FOUND)
	{
		m_TempPTChapterSelected = pListBoxChapterNumberAndStatus->GetStringSelection();
	}
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

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CGetSourceTextFromEditorDlg::OnOK(wxCommandEvent& event) 
{
	// Check that both drop down boxes have selections and that they do not
	// point to the same PT project.
	if (m_TempPTProjectForSourceInputs == m_TempPTProjectForTargetExports && m_TempPTProjectForSourceInputs != _("[No Project Selected]"))
	{
		wxString msg, msg1;
		msg = _("The projects selected for getting source texts and receiving translation texts cannot be the same.\nPlease select one project for getting source texts, and a different project for receiving translation texts.");
		//msg1 = _("(or, if you select \"[No Project Selected]\" for a project here, the first time a source text is needed for adaptation, the user will have to choose a project from a drop down list of projects).");
		//msg = msg + _T("\n") + msg1;
		wxMessageBox(msg);
		return; // don't accept any changes - abort the OnOK() handler
	}
	
	// save new project selection to the App's variables for writing to config file
	m_pApp->m_PTProjectForSourceInputs = m_TempPTProjectForSourceInputs;
	m_pApp->m_PTProjectForTargetExports = m_TempPTProjectForTargetExports;
	m_pApp->m_PTBookSelected = m_TempPTBookSelected;
	m_pApp->m_PTChapterSelected = m_TempPTChapterSelected;
	
	// TODO: 
	// Set up (or access any existing) project for the selected book/chapter
	
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

// whm Note: We could use the doc's and helpers's version of these below
// instead of having them be members of CGetSourceTextFromEditorDlg.
bool CGetSourceTextFromEditorDlg::IsAnsiLetter(wxChar c)
{
	return wxIsalpha(c) != FALSE;
}

bool CGetSourceTextFromEditorDlg::IsChapterMarker(wxChar* pChar)
{
	if (*pChar != _T('\\'))
		return FALSE;
	wxChar* ptr = pChar;
	ptr++;
	if (*ptr == _T('c'))
	{
		ptr++;
		return IsNonEolWhiteSpace(ptr);
	}
	else
		return FALSE;
}

bool CGetSourceTextFromEditorDlg::IsVerseMarker(wxChar *pChar, int& nCount)
{
	if (*pChar != _T('\\'))
		return FALSE;
	wxChar* ptr = pChar;
	ptr++;
	if (*ptr == _T('v'))
	{
		ptr++;
		if (*ptr == _T('n'))
		{
			// must be an Indonesia branch \vn 'verse number' marker
			// if white space follows
			ptr++;
			nCount = 3;
		}
		else
		{
			nCount = 2;
		}
		return IsNonEolWhiteSpace(ptr);
	}
	else
		return FALSE;
}

wxString CGetSourceTextFromEditorDlg::GetStringFromBuffer(const wxChar* ptr, int itemLen)
{
	return wxString(ptr,itemLen);
}

int CGetSourceTextFromEditorDlg::ParseNumber(wxChar *pChar)
{
	wxChar* ptr = pChar;
	int length = 0;
	while (!IsNonEolWhiteSpace(ptr) && *ptr != _T('\n') && *ptr != _T('\r'))
	{
		ptr++;
		length++;
	}
	return length;
}

bool CGetSourceTextFromEditorDlg::IsWhiteSpace(wxChar *pChar, bool& IsEOLchar)
{
	// The standard white-space characters are the following: space 0x20, tab 0x09, 
	// carriage-return 0x0D, newline 0x0A, vertical tab 0x0B, and form-feed 0x0C.
	// returns true if pChar points to a standard white-space character.
	// We also let the caller know if it is an eol char by returning TRUE in the
	// isEOLchar reference param, or FALSE if it is not an eol char.
	if (*pChar == _T('\r')) // 0x0D CR 
	{
		// this is an eol char so we let the caller know
		IsEOLchar = TRUE;
		return TRUE;
	}
	else if (*pChar == _T('\n')) // 0x0A LF (newline)
	{
		// this is an eol char so we let the caller know
		IsEOLchar = TRUE;
		return TRUE;
	}
	else if (*pChar == _T(' ')) // 0x20 space
	{
		IsEOLchar = FALSE;
		return TRUE;
	}
	else if (*pChar == _T('\t')) // 0x09 tab
	{
		IsEOLchar = FALSE;
		return TRUE;
	}
	else if (*pChar == _T('\v')) // 0x0B vertical tab
	{
		IsEOLchar = FALSE;
		return TRUE;
	}
	else if (*pChar == _T('\f')) // 0x0C FF form-feed
	{
		IsEOLchar = FALSE;
		return TRUE;
	}
	else
	{
		IsEOLchar = FALSE;
		return FALSE;
	}
	
	//if (wxIsspace(*pChar) == 0)// _istspace not recognized by g++ under Linux
	//	return FALSE;
	//else
	//	return TRUE;
}

bool CGetSourceTextFromEditorDlg::IsNonEolWhiteSpace(wxChar *pChar)
{
	// The standard white-space characters are the following: space 0x20, tab 0x09, 
	// carriage-return 0x0D, newline 0x0A, vertical tab 0x0B, and form-feed 0x0C.
	// returns true if pChar points to a non EOL white-space character, but FALSE
	// if pChar points to an EOL char or any other character.
	if (*pChar == _T(' ')) // 0x20 space
	{
		return TRUE;
	}
	else if (*pChar == _T('\t')) // 0x09 tab
	{
		return TRUE;
	}
	else if (*pChar == _T('\v')) // 0x0B vertical tab
	{
		return TRUE;
	}
	else if (*pChar == _T('\f')) // 0x0C FF form-feed
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

int CGetSourceTextFromEditorDlg::ParseNonEolWhiteSpace(wxChar *pChar)
{
	int	length = 0;
	wxChar* ptr = pChar;
	while (IsNonEolWhiteSpace(ptr))
	{
		length++;
		ptr++;
	}
	return length;
}

int CGetSourceTextFromEditorDlg::ParseMarker(wxChar *pChar, wxChar *pEnd)
{
    // whm: see note in the Doc's version of ParseMarker for reasons why I've modified
    // this function to add a pointer to the end of the buffer in its signature.
	int len = 0;
	wxChar* ptr = pChar;
	wxChar* pBegin = ptr;
	while (ptr < pEnd && !IsNonEolWhiteSpace(ptr) && *ptr != _T('\n') && *ptr != _T('\r') && *ptr != _T('\0') && m_pApp->m_forbiddenInMarkers.Find(*ptr) == wxNOT_FOUND)
	{
		if (ptr != pBegin && (*ptr == gSFescapechar || *ptr == _T(']'))) 
			break; 
		ptr++;
		len++;
		if (*(ptr -1) == _T('*')) // whm ammended 17May06 to halt after asterisk (end marker)
			break;
	}
	return len;
}

bool CGetSourceTextFromEditorDlg::IsMarker(wxChar *pChar, wxChar *pEnd)
{
	// whm modified 2May11 from the IsMarker function in the Doc to add checks for pEnd
	// to prevent it from accessing memory beyond the buffer. The Doc's version should
	// also have this check, otherwise a spurious '\' at or near the end of the buffer 
	// would result in trying to access a char in memory beyond the actual end of the
	// buffer.
	// 
    // also use bool IsAnsiLetter(wxChar c) for checking character after backslash is an
    // alphabetic one; and in response to issues Bill raised in any email on Jan 31st about
    // spurious marker match positives, make the test smarter so that more which is not a
    // genuine marker gets rejected (and also, use IsMarker() in ParseWord() etc, rather
    // than testing for *ptr == gSFescapechar)
	if (*pChar == gSFescapechar)
	{
		// reject \n but allow the valid USFM markers \nb \nd \nd* \no \no* \ndx \ndx*
		if ((pChar + 1) < pEnd && *(pChar + 1) == _T('n')) // whm added (pChar + 1) < pEnd test
		{
			if ((pChar + 2) < pEnd && IsAnsiLetter(*(pChar + 2))) // whm added (pChar + 2) < pEnd test
			{
				// assume this is one of the allowed USFM characters listed in the above
				// comment
				return TRUE;
			}
			else if ((pChar + 2) < pEnd && (IsNonEolWhiteSpace((pChar + 2)) || (*pChar + 2) == _T('\n') || (*pChar + 2) == _T('\r'))) // whm added (pChar + 2) < pEnd test
			{
				// it's an \n escaped linefeed indicator, not an SFM
				return FALSE;
			}
			else
			{
                // the sequence \n followed by some nonalphabetic character nor
                // non-whitespace character is unlikely to be a value SFM or USFM, so
                // return FALSE here too -- if we later want to make the function more
                // specific, we can put extra tests here
                return FALSE;
			}
		}
		else if ((pChar + 1) < pEnd && !IsAnsiLetter(*(pChar + 1))) // whm added (pChar + 1) < pEnd test
		{
			return FALSE;
		}
		else
		{
			// after the backslash is an alphabetic character, so assume its a valid marker
			return TRUE;
		}
	}
	else
	{
		// not pointing at a backslash, so it is not a marker
		return FALSE;
	}
}

wxArrayString CGetSourceTextFromEditorDlg::GetUsfmStructureAndExtent(wxString& sourceFileBuffer)
{
	// process the buffers extracting wxArrayStrings representing the 1:1:nnnn data from the
	// source and target file buffers
	
	wxArrayString UsfmStructureAndExtentArray;
	const wxChar* pBuffer = sourceFileBuffer.GetData();
	int nBufLen = sourceFileBuffer.Length();
	int itemLen = 0;
	wxChar* ptrSrc = (wxChar*)pBuffer;	// point to the first char (start) of the buffer text
	wxChar* pEnd = ptrSrc + nBufLen;	// point to one char past the end of the buffer text
	wxASSERT(*pEnd == '\0');

	// Note: the wxConvUTF8 parameter of the above targetFileBuffer constructor also
	// removes the initial BOM from the string when converting to a wxString
	// but we'll check here to make sure and skip it if present. Curiously, the string's
	// buffer after conversion also contains the FEFF UTF-16 BOM as its first char in the
	// buffer! The wxString's converted buffer is indeed UTF-16, but why add the BOM to a 
	// memory buffer in converting from UTF8 to UTF16?!
	const int nBOMLen = 3;
	wxUint8 szBOM[nBOMLen] = {0xEF, 0xBB, 0xBF};
	bool bufferHasUtf16BOM = FALSE;
	if (!memcmp(pBuffer,szBOM,nBOMLen))
	{
		ptrSrc = ptrSrc + nBOMLen;
	}
	else if (*pBuffer == 0xFEFF)
	{
		bufferHasUtf16BOM = TRUE;
		// skip over the UTF16 BOM in the buffer
		ptrSrc = ptrSrc + 1;
	}
	
	wxString temp;
	temp.Empty();
	int nMkrLen = 0;

	// charCount includes all chars in file except for eol chars
	int charCountSinceLastMarker = 0;
	int eolCount = 0;
	int charCount = 0;
	int charCountMarkersOnly = 0; // includes any white space within and after markers, but not eol chars
	wxString lastMarker;
	wxString lastMarkerNumericAugment;
	// Scan the buffer and extract the chapter:verse:count information
	while (ptrSrc < pEnd)
	{
		while (ptrSrc < pEnd && !IsMarker(ptrSrc,pEnd))
		{
			// This loop handles the parsing and counting of all characters not directly 
			// associated with sfm markers, including eol characters.
			if (*ptrSrc == _T('\n') || *ptrSrc == _T('\r'))
			{
				// its an eol char
				ptrSrc++;
				eolCount++;
			}
			else
			{
				ptrSrc++;
				charCount++;
				charCountSinceLastMarker++;			
			}
		}
		if (ptrSrc < pEnd)
		{
			// This loop handles the parsing and counting of all sfm markers themselves.
			if (IsMarker(ptrSrc,pEnd))
			{
				if (!lastMarker.IsEmpty())
				{
					// output the data for the last marker
					wxString usfmDataStr;
					// construct data string for the lastMarker
					usfmDataStr = lastMarker;
					usfmDataStr += lastMarkerNumericAugment;
					usfmDataStr += _T(':');
					usfmDataStr << charCountSinceLastMarker;
					charCountSinceLastMarker = 0;
					lastMarker.Empty();
					UsfmStructureAndExtentArray.Add(usfmDataStr);
				
					lastMarkerNumericAugment.Empty();
				}
			}
			
			if (IsChapterMarker(ptrSrc))
			{
				// its a chapter marker
				// parse the chapter marker and following white space

				itemLen = ParseMarker(ptrSrc, pEnd); // does not parse through eol chars
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get the marker
				lastMarker = temp;
				
				ptrSrc += itemLen; // point past the \c marker
				charCountMarkersOnly += itemLen;

				itemLen = ParseNonEolWhiteSpace(ptrSrc);
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get non-eol whitespace
				lastMarkerNumericAugment += temp;

				ptrSrc += itemLen; // point at chapter number
				charCountMarkersOnly += itemLen;

				itemLen = ParseNumber(ptrSrc); // ParseNumber doesn't parse over eol chars
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get the number
				lastMarkerNumericAugment += temp;
				
				ptrSrc += itemLen; // point past chapter number
				charCountMarkersOnly += itemLen;

				itemLen = ParseNonEolWhiteSpace(ptrSrc); // parse the non-eol white space following 
													     // the number
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get the non-eol white space
				lastMarkerNumericAugment += temp;
				ptrSrc += itemLen; // point past it
				charCountMarkersOnly += itemLen;
				// we've parsed the chapter marker and number and non-eol white space
			}
			else if (IsVerseMarker(ptrSrc,nMkrLen))
			{
				// Its a verse marker

				// Parse the verse number and following white space
				itemLen = ParseMarker(ptrSrc, pEnd); // does not parse through eol chars
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get the verse marker
				lastMarker = temp;
				
				if (nMkrLen == 2)
				{
					// its a verse marker
					ptrSrc += 2; // point past the \v marker
					charCountMarkersOnly += itemLen;
				}
				else
				{
					// its an Indonesia branch verse marker \vn
					ptrSrc += 3; // point past the \vn marker
					charCountMarkersOnly += itemLen;
				}

				itemLen = ParseNonEolWhiteSpace(ptrSrc);
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get the non-eol white space
				lastMarkerNumericAugment += temp;
				ptrSrc += itemLen; // point at verse number
				charCountMarkersOnly += itemLen;

				itemLen = ParseNumber(ptrSrc); // ParseNumber doesn't parse over eol chars
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get the verse number
				lastMarkerNumericAugment += temp;
				ptrSrc += itemLen; // point past verse number
				charCountMarkersOnly += itemLen;

				itemLen = ParseNonEolWhiteSpace(ptrSrc); // parse white space which is 
														 // after the marker
				// we don't need the white space on the lastMarkerNumericAugment which we
				// would have to remove during parsing of fields on the : delimiters
				ptrSrc += itemLen; // point past the white space
				charCountMarkersOnly += itemLen; // count following white space with markers
			}
			else if (IsMarker(ptrSrc,pEnd))
			{
				// Its some other marker.

				itemLen = ParseMarker(ptrSrc, pEnd); // does not parse through eol chars
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get the marker
				lastMarker = temp;
				ptrSrc += itemLen; // point past the marker
				charCountMarkersOnly += itemLen;

				itemLen = ParseNonEolWhiteSpace(ptrSrc);
				// we don't need the white space but we count it
				ptrSrc += itemLen; // point past the white space
				charCountMarkersOnly += itemLen; // count following white space with markers
			}
		}
		else
		{
			// ptrSrc is pointing at or past the pEnd
			break;
		}
	}
	
	// output data for any lastMarker that wan't output in above main while 
	// loop (at the end of the file)
	if (!lastMarker.IsEmpty())
	{
		wxString usfmDataStr;
		// construct data string for the lastMarker
		usfmDataStr = lastMarker;
		usfmDataStr += lastMarkerNumericAugment;
		usfmDataStr += _T(':');
		usfmDataStr << charCountSinceLastMarker;
		charCountSinceLastMarker = 0;
		lastMarker.Empty();
		UsfmStructureAndExtentArray.Add(usfmDataStr);
	
		lastMarkerNumericAugment.Empty();
	}
	//int ct;
	//for (ct = 0; ct < (int)UsfmStructureAndExtentArray.GetCount(); ct++)
	//{
	//	wxLogDebug(UsfmStructureAndExtentArray.Item(ct));
	//}
	// Note: Our pointer is always incremented to pEnd at the end of the file which is one char beyond
	// the actual last character so it represents the total number of characters in the buffer. 
	// Thus the Total Count below doesn't include the beginning UTF-16 BOM character, which is also the length
	// of the wxString buffer as reported in nBufLen by the sourceFileBuffer.Length() call.
	// Actual size on disk will be 2 characters larger than charCount's final value here because
	// the file on disk will have the 3-char UTF8 BOM, and the sourceFileBuffer here has the 
	// 1-char UTF-16 BOM.
	int utf16BomLen;
	if (bufferHasUtf16BOM)
		utf16BomLen = 1;
	else
		utf16BomLen = 0;
	wxLogDebug(_T("Total Count = %d [charCount (%d) + eolCount (%d) + charCountMarkersOnly (%d)] Compare to nBufLen = %d"),
		charCount+eolCount+charCountMarkersOnly,charCount,eolCount,charCountMarkersOnly,nBufLen - utf16BomLen);
	return UsfmStructureAndExtentArray;
}

wxArrayString CGetSourceTextFromEditorDlg::GetChapterListFromTargetBook(wxString targetBookFullName)
{
	// retrieves a wxArrayString of chapters (and their status) from the target 
	// PT project's TargetTextUsfmStructureAndExtentArray. 
	wxArrayString chapterArray;
	chapterArray.Clear();
	int ct,tot;
	tot = TargetTextUsfmStructureAndExtentArray.GetCount();
	wxString tempStr;
	bool bChFound = FALSE;
	bool bVsFound = FALSE;
	wxString projShortName = GetShortNameFromLBProjectItem(m_TempPTProjectForTargetExports);
	PT_Project_Info_Struct* pPTInfo;
	pPTInfo = m_pApp->GetPT_Project_Struct(projShortName);  // gets pointer to the struct from the 
															// pApp->m_pArrayOfPTProjects
	wxString chMkr = _T("\\") + pPTInfo->chapterMarker;
	wxString vsMkr = _T("\\") + pPTInfo->verseMarker;
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
		return chapterArray; // caller will give warning message
	}

	wxString statusOfChapter;
	if (bChFound)
	{
		for (ct = 0; ct < tot; ct++)
		{
			tempStr = TargetTextUsfmStructureAndExtentArray.Item(ct);
			if (tempStr.Find(chMkr) != wxNOT_FOUND) // \c
			{
				// we're pointing at a \c element of a string that is of the form: "\c 1:0"
				// strip away the preceding \c and the following :0
				int posColon = tempStr.Find(_T(':'),TRUE); // TRUE - find from right end
				tempStr = tempStr.Mid(0,posColon);
				
				int posChMkr;
				posChMkr = tempStr.Find(chMkr); // \c
				wxASSERT(posChMkr == 0);
				int posAfterSp = tempStr.Find(_T(' '));
				wxASSERT(posAfterSp > 0);
				posAfterSp ++; // to point past space
				tempStr = tempStr.Mid(posAfterSp);
				statusOfChapter = GetStatusOfChapter(TargetTextUsfmStructureAndExtentArray,ct);
				wxString listItemStatusSuffix,listItem;
				if (!statusOfChapter.IsEmpty())
				{
					listItemStatusSuffix = _("Chapter has empty verse(s): ") + statusOfChapter;
				}
				else
				{
					listItemStatusSuffix = _("Translated (all verses have content)");
				}
				listItem.Empty();
				listItem = targetBookFullName + _T(" ");
				listItem += tempStr;
				listItem += _T(" - ");
				listItem += listItemStatusSuffix;
				chapterArray.Add(listItem);
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
		// information with element 0 (the first element)
		statusOfChapter = GetStatusOfChapter(TargetTextUsfmStructureAndExtentArray,-1);
		wxString listItemStatusSuffix,listItem;
		listItemStatusSuffix.Empty();
		listItem.Empty();
		if (!statusOfChapter.IsEmpty())
		{
			listItemStatusSuffix = _("Chapter has empty verse(s): ") + statusOfChapter;
		}
		else
		{
			listItemStatusSuffix = _("Translated (all verses have content)");
		}
		listItem = targetBookFullName + _T(" ");
		listItem += _T("1");
		listItem += _T(" - ");
		listItem += listItemStatusSuffix;
		chapterArray.Add(listItem);
	}
	return chapterArray;
}

void CGetSourceTextFromEditorDlg::LoadBookNamesIntoList()
{
	pListBoxBookNames->Clear();
	
	wxString ptProjShortName;
	ptProjShortName = GetShortNameFromLBProjectItem(m_TempPTProjectForSourceInputs);
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
		msg1 = msg1.Format(_("The Paratext project (%s) selected for obtaining source texts contains no books."),m_TempPTProjectForSourceInputs.c_str());
		msg2 = _("Please select the Paratext Project that contains the source texts you will use for adaptation work.");
		msg1 = msg1 + _T(' ') + msg2;
		wxMessageBox(msg1,_T("No books found"),wxICON_WARNING);
	}
	pListBoxBookNames->Append(booksPresentArray);

}

wxString CGetSourceTextFromEditorDlg::GetStatusOfChapter(const wxArrayString &TargetArray,int indexOfChItem)
{
	// When this function is called indexOfChItem is pointing at a \c n:nnnn line in the TargetArray
	// We want to get the status of the chapter by examining each verse of that chapter to see if it 
	// has content. If none of the verses of the chapter have content we will return "Not Translated"; 
	// if one or more verses of that chapter have content but one or more other verses have no content 
	// we will return "Partly Translated"
	// TODO: Add to the status string an indication of whether the chapter has been transferred to
	// the target PT project or not (if it has not yet been done).
	
	// scan forward in the TargetArray until we reach the next \c element. For each \v we encounter
	// we examine the text extent of that \v element. When a particular \v element is empty we make
	// a string list of verse numbers that are empty, which will display in the "Select a chapter"
	// list box something like:
	// 3 - Partly translated -empty verse(s) 12-14, 20, 21-29
	int index = indexOfChItem;
	int tot = (int)TargetArray.GetCount();
	wxString tempStr;
	wxString statusStr;
	statusStr.Empty();
	wxString projShortName = GetShortNameFromLBProjectItem(m_TempPTProjectForTargetExports);
	PT_Project_Info_Struct* pPTInfo;
	pPTInfo = m_pApp->GetPT_Project_Struct(projShortName);  // gets pointer to the struct from the 
															// pApp->m_pArrayOfPTProjects
	wxString chMkr = _T("\\") + pPTInfo->chapterMarker;
	wxString vsMkr = _T("\\") + pPTInfo->verseMarker;
	
	if (indexOfChItem == -1)
	{
		// When the incoming indexOfChItem parameter is -1 it indicates that this is
		// a book that does not have a chapter \c marker
		do
		{
			index++; // point to first and succeeding lines
			if (index >= tot)
				break;
			tempStr = TargetArray.Item(index);
			if (tempStr.Find(vsMkr) == 0) // \v
			{
				// we're at a verse, so note if it is empty
				int posColon = tempStr.Find(_T(':'),TRUE); // TRUE - find from right end
				wxASSERT(posColon != wxNOT_FOUND);
				wxString extent;
				extent = tempStr.Mid(posColon + 1);
				extent.Trim(FALSE);
				extent.Trim(TRUE);
				if (extent == _T("0"))
				{
					// The verse is empty so get the verse number, and add it
					// to the statusStr
					statusStr += GetVerseNumberFromVerseStr(tempStr);
					statusStr += _T(':'); 
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
					// we're at a verse, so note if it is empty
					int posColon = tempStr.Find(_T(':'),TRUE); // TRUE - find from right end
					wxASSERT(posColon != wxNOT_FOUND);
					wxString extent;
					extent = tempStr.Mid(posColon + 1);
					extent.Trim(FALSE);
					extent.Trim(TRUE);
					if (extent == _T("0"))
					{
						// The verse is empty so get the verse number, and add it
						// to the statusStr
						statusStr += GetVerseNumberFromVerseStr(tempStr);
						statusStr += _T(':'); 
					}
				}
			}
		} while (index < tot  && bStillInChapter);
	}
	// TODO: Shorten any statusStr by indicating continuous empty verses with a dash between
	// the starting and ending continuously empty verses, i.e., 5-7, and format the list with
	// comma and space between items. Then prefix a localizable " - empty verses: " so that the
	// result would be something like: "- empty verses: 1, 3, 5-7, 10-22"
	
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
	
	if (!statusStr.IsEmpty())
	{
		statusStr = AbbreviateColonSeparatedVerses(statusStr);

	}
	return statusStr;
}

wxString CGetSourceTextFromEditorDlg::GetVerseNumberFromVerseStr(const wxString& verseStr)
{
	// Parse the string which is of the form: \v n:nnnn
	// 
	// Since the string is of the form: \v n:nnnn, we cannot use the ParseNumber() to
	// get the number itself since, with the :nnnn suffixed to it, it does not end in
	// whitespace or CR LF as would a normal verse number, so we get the number differently.

	wxString numStr = verseStr;
	int posColon = numStr.Find(_T(':'),TRUE); // TRUE - find from right end
	wxASSERT(posColon != wxNOT_FOUND);
	numStr = numStr.Mid(0,posColon);
	int posSpace = numStr.Find(_T(' '),TRUE);
	numStr = numStr.Mid(posSpace);
	numStr.Trim(FALSE);
	numStr.Trim(TRUE);
	return numStr;
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
		pListBoxChapterNumberAndStatus->Disable();
		pStaticSelectAChapter->Disable();
	}
	else
	{
		// The user selected "Get Chapter Only"
		pListBoxChapterNumberAndStatus->Enable(TRUE);
		pStaticSelectAChapter->Enable(TRUE);
	}
}