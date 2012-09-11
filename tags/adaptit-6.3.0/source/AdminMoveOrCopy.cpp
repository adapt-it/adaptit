/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AdminMoveOrCopy.cpp
/// \author			Bruce Waters
/// \date_created	30 November 2009
/// \date_revised	
/// \copyright		2009 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the AdminMoveOrCopy class. 
/// The AdminMoveOrCopy class provides a dialog interface for the user (typically an administrator) to be able
/// to combine move or copy files or folders or both.
/// \derivation		The AdminMoveOrCopy class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////


// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "AdminMoveOrCopy.h"
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

// the following two must be deprecated, I can't find them anywhere and they don't compile
//#include "file.xpm"
//#include "folder.xpm"

// other includes
#include <wx/filename.h>
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
#include <wx/imaglist.h> // for wxImageList
#include <wx/datetime.h>
#include "Adapt_It.h"
#include "helpers.h" // it has the Get... functions for getting list of files, folders
					 // and optionally sorting
#include "Adapt_It_wdr.h" // needed for the AIMainFrameIcons(index) function which returns
						  // for index values 10 and 11 the folder icon and file icon
						  // respectively, which we use in the two wxListCtrl instances to
						  // distinguish folder names from filenames; the function returns
						  // the relevant wxBitmap*
#include "FilenameConflictDlg.h"
#include "AdminMoveOrCopy.h"
#include "PeekAtFile.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

extern bool gbDoingSplitOrJoin;

enum myicons {
	indxFolderIcon,
	indxFileIcon,
	indxEmptyIcon
};

// Event handler table (The first version of this code called the left and right panes the
// source and destination panes, respectively. I've switched to 'left' and 'right', in the
// code, but I've left the wxDesigner resource labels unchanged, and they have the words
// SOURCE and DESTINATION within them -- so just read them as LEFT and RIGHT, respectively,
// and it will make sense)
BEGIN_EVENT_TABLE(AdminMoveOrCopy, AIModalDialog)

	EVT_INIT_DIALOG(AdminMoveOrCopy::InitDialog)
	EVT_BUTTON(wxID_OK, AdminMoveOrCopy::OnOK)

	EVT_BUTTON(ID_BUTTON_LOCATE_SOURCE_FOLDER, AdminMoveOrCopy::OnBnClickedLocateLeftFolder)	
	EVT_BUTTON(ID_BUTTON_LOCATE_DESTINATION_FOLDER, AdminMoveOrCopy::OnBnClickedLocateRightFolder)
	EVT_BUTTON(ID_BITMAPBUTTON_SRC_OPEN_FOLDER_UP, AdminMoveOrCopy::OnBnClickedLeftParentFolder)
	EVT_BUTTON(ID_BITMAPBUTTON_DEST_OPEN_FOLDER_UP, AdminMoveOrCopy::OnBnClickedRightParentFolder)

	EVT_SIZE(AdminMoveOrCopy::OnSize)

	EVT_LIST_ITEM_SELECTED(ID_LISTCTRL_SOURCE_CONTENTS, AdminMoveOrCopy::OnLeftListSelectItem)
	EVT_LIST_ITEM_DESELECTED(ID_LISTCTRL_SOURCE_CONTENTS, AdminMoveOrCopy::OnLeftListDeselectItem)
	EVT_LIST_ITEM_SELECTED(ID_LISTCTRL_DESTINATION_CONTENTS, AdminMoveOrCopy::OnRightListSelectItem)
	EVT_LIST_ITEM_DESELECTED(ID_LISTCTRL_DESTINATION_CONTENTS, AdminMoveOrCopy::OnRightListDeselectItem)
	EVT_LIST_ITEM_ACTIVATED(ID_LISTCTRL_SOURCE_CONTENTS, AdminMoveOrCopy::OnLeftListDoubleclick)
	EVT_LIST_ITEM_ACTIVATED(ID_LISTCTRL_DESTINATION_CONTENTS, AdminMoveOrCopy::OnRightListDoubleclick)

	EVT_BUTTON(ID_BUTTON_DELETE, AdminMoveOrCopy::OnBnClickedDelete)
	EVT_BUTTON(ID_BUTTON_RENAME, AdminMoveOrCopy::OnBnClickedRename)
	EVT_BUTTON(ID_BUTTON_COPY, AdminMoveOrCopy::OnBnClickedCopy)
	EVT_BUTTON(ID_BUTTON_MOVE, AdminMoveOrCopy::OnBnClickedMove)
	EVT_BUTTON(ID_BUTTON_PEEK, AdminMoveOrCopy::OnBnClickedPeek)

	//EVT_LIST_KEY_DOWN(ID_LISTCTRL_SOURCE_CONTENTS, AdminMoveOrCopy::OnLeftListKeyDown)
	//EVT_LIST_KEY_DOWN(ID_LISTCTRL_DESTINATION_CONTENTS, AdminMoveOrCopy::OnRightListKeyDown)

END_EVENT_TABLE()

AdminMoveOrCopy::AdminMoveOrCopy(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Move Or Copy Folders Or Files"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	pLeftList = NULL;
	pRightList= NULL;
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pAdminMoveCopySizer = MoveOrCopyFilesOrFoldersFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );


	m_strLeftFolderPath = _T("");
	m_strRightFolderPath = _T("");

	leftFoldersArray.Empty();
	leftFilesArray.Empty();
	rightFoldersArray.Empty();
	rightFilesArray.Empty();
	leftSelectedFoldersArray.Empty();
	leftSelectedFilesArray.Empty();
	rightSelectedFilesArray.Empty();
	rightSelectedFoldersArray.Empty();
	arrCopiedOK.Empty();
}

AdminMoveOrCopy::~AdminMoveOrCopy() // destructor
{
	pIconImages->RemoveAll();
	if (pIconImages != NULL) // whm 11Jun12 added NULL test
		delete pIconImages;
	if (pTheColumnForLeftList != NULL) // whm 11Jun12 added NULL test
		delete pTheColumnForLeftList;
	if (pTheColumnForRightList != NULL) // whm 11Jun12 added NULL test
		delete pTheColumnForRightList;

	leftFoldersArray.Clear();
	leftFilesArray.Clear();
	rightFoldersArray.Clear();
	rightFilesArray.Clear();
	leftSelectedFoldersArray.Clear();
	leftSelectedFilesArray.Clear();
	rightSelectedFoldersArray.Clear();
	rightSelectedFilesArray.Clear();
	arrCopiedOK.Clear();
}

///////////////////////////////////////////////////////////////////////////////////
///
///    START OF GUI FUNCTIONS 
///
///////////////////////////////////////////////////////////////////////////////////

// InitDialog is method of wxWindow
void AdminMoveOrCopy::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp;
	pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp != NULL);
	pApp->m_bAdminMoveOrCopyIsInitializing = TRUE; // set TRUE, use this to suppress
			// a warning message in GetFoldersOnly() when InitDialog is running
			//  -- see Helpers.cpp
	
	leftFoldersCount = 0;
	leftFilesCount = 0;
	rightFoldersCount = 0;
	rightFilesCount = 0;

	m_bUserCancelled = FALSE;
	m_bDoTheSameWay = FALSE;

	sideWithFocus = neitherSideHasFocus;

	// set colours
#ifdef __WXMAC__
	// whm 10Sep10 made the pastelgreen to be the same as nocolor on the Mac (i.e., white)
	pastelgreen = wxColour(255,255,255); // wxColour(170,255,170);
#else
	pastelgreen = wxColour(210,255,210);
#endif
	nocolor = wxColour(255,255,255);

	// set up pointers to interface objects
	pLeftFolderPathTextCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SOURCE_PATH);
	//pLeftFolderPathTextCtrl->SetValidator(wxGenericValidator(&m_strLeftFolderPath)); // whm removed 4Feb11
	pRightFolderPathTextCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_DESTINATION_PATH);
	//pRightFolderPathTextCtrl->SetValidator(wxGenericValidator(&m_strRightFolderPath)); // whm removed 4Feb11

	pLocateLeftFolderButton = (wxButton*)FindWindowById(ID_BUTTON_LOCATE_SOURCE_FOLDER);
	wxASSERT(pLocateLeftFolderButton != NULL);
	pLocateRightFolderButton = (wxButton*)FindWindowById(ID_BUTTON_LOCATE_DESTINATION_FOLDER);
	wxASSERT(pLocateRightFolderButton != NULL);

	pPeekButton = (wxButton*)FindWindowById(ID_BUTTON_PEEK);;
	wxASSERT(pPeekButton != NULL);

	pLeftList = (wxListView*)FindWindowById(ID_LISTCTRL_SOURCE_CONTENTS);
	pRightList= (wxListView*)FindWindowById(ID_LISTCTRL_DESTINATION_CONTENTS);

	pUpLeftFolder = (wxBitmapButton*)FindWindowById(ID_BITMAPBUTTON_SRC_OPEN_FOLDER_UP);
	wxASSERT(pUpLeftFolder != NULL);
	pUpRightFolder = (wxBitmapButton*)FindWindowById(ID_BITMAPBUTTON_DEST_OPEN_FOLDER_UP);
	wxASSERT(pUpRightFolder != NULL);

	pMoveButton = (wxButton*)FindWindowById(ID_BUTTON_MOVE);
	pCopyButton = (wxButton*)FindWindowById(ID_BUTTON_COPY);
	pDeleteButton = (wxButton*)FindWindowById(ID_BUTTON_DELETE);
	pRenameButton = (wxButton*)FindWindowById(ID_BUTTON_RENAME);

	// start with lower buttons disabled (they rely on selections to become enabled)
	EnableButtons();

    // get the folder and file icons (bitmaps actually) into the image list which the two
    // wxListCtrl instances will use
    // Note: http://www.pnelsoncomposer.com/FAQs indicates that the error message
    // "Couldn't add an image to the i mage list" can arise when bitmap images are not the
    // expected size; I had the folder one as 16x14 (width x height) and the other as 13x14
    // - and this caused the error --but it was generated lazily, when the modal dialog loop
    // began to run, so it was not apparent that the error was in the next few lines; the
    // error disappeared when I made both 16x14. However, it can also crop up apparently if
    // the tool image size is greater than the bitmap dimensions, and so my Create below
    // using width and height of 16 may yet still generate the error. If that ever happens,
    // either make the icons 16x16, or make the Create() have the parameters
    // Create(16,14,...etc)
	//pIconImages->Create(16,16,FALSE,2); // FALSE is bool mask, and we don't need a mask
	//pIconImages->Create(16,14,TRUE,2); // TRUE is bool mask, I think we need one??
	
	pIconImages = new wxImageList(16,14,TRUE,3);

	// set up the single column object for each list, on the heap (each has to persist until
	// the dialog is dismissed)
	// Note: the wxListItem which is the column has to be on the heap, because if made a local
	// variable then it will go out of scope and be lost from the wxListCtrl before the
	// latter has a chance to display anything, and then nothing will display in the control
	int height; int width;
	pTheColumnForLeftList = new wxListItem;
	pTheColumnForRightList = new wxListItem;
	// set the left column's width & insert it into the left side's wxListCtrl
	pLeftList->GetClientSize(&width,&height);
	pTheColumnForLeftList->SetWidth(width);
	pLeftList->InsertColumn(0, *pTheColumnForLeftList);
	//pLeftList->SetColumnWidth(0,wxLIST_AUTOSIZE); // <- supposed to work but doesn't
	// so I made my own column-width adjusting code using OnSize()
	 
	// set the right column's width & insert it into the right side's wxListCtrl
	pRightList->GetClientSize(&width,&height);
	pTheColumnForRightList->SetWidth(width);
	pRightList->InsertColumn(0, *pTheColumnForRightList);
	//pRightList->SetColumnWidth(0, wxLIST_AUTOSIZE); // <- supposed to work but doesn't 
	// so I made my own column-width adjusting code using OnSize()

	// obtain the folder and file bitmap images which are to go at the start of folder
	// names or file names, respectively, in each list; also an 'empty faucet' icon for when
	// the message "The folder is empty" is displayed, otherwise it defaults to showing
	// the first icon of the image list, which is a folder icon (- that would be confusing)
	wxBitmap folderIcon = AIMainFrameIcons(10);
	wxBitmap fileIcon = AIMainFrameIcons(11);
	wxBitmap emptyIcon = AIMainFrameIcons(12);
	//wxBitmap folderIcon(AIMainFrameIcons(10)); // these two lines are probably ok but
	//wxBitmap fileIcon(AIMainFrameIcons(11));   // the above alternatives are ok too
	
	int iconIndex;
	iconIndex = pIconImages->Add(folderIcon);
	iconIndex = pIconImages->Add(fileIcon);
	iconIndex = pIconImages->Add(emptyIcon);
	iconIndex = iconIndex; // avoid warning
	// set up the wxListCtrl instances, for each set a column for an icon followed by text
	pLeftList->SetImageList(pIconImages, wxIMAGE_LIST_SMALL);
	pRightList->SetImageList(pIconImages, wxIMAGE_LIST_SMALL);

	emptyFolderMessage = _("The folder is empty (or maybe undefined)");

	// initialize for the "Locate...folder" buttons; we default the left side to the
	// work directory, and display its contents; we default the right side to the
	// current project's __SOURCE_INPUTS folder if there is one, if not, then to the current
	// project folder if there is one, if not the use an empty path
	if (gpApp->m_bUseCustomWorkFolderPath  && !gpApp->m_customWorkFolderPath.IsEmpty())
	{
		m_strLeftFolderPath = gpApp->m_customWorkFolderPath;
	}
	else
	{
		m_strLeftFolderPath = gpApp->m_workFolderPath;
	}
	SetupLeftList(m_strLeftFolderPath);
	SetupSelectionArrays(leftSide);

	m_strRightFolderPath = _T(""); // start with no path defined
	if (!gpApp->m_curProjectPath.IsEmpty() && ::wxDirExists(gpApp->m_curProjectPath))
	{
		// set project folder as default folder for right pane, because it exists
		m_strRightFolderPath = gpApp->m_curProjectPath;
		// go the next step if possible
		if (!gpApp->m_sourceInputsFolderPath.IsEmpty() && ::wxDirExists(gpApp->m_sourceInputsFolderPath))
		{
			// set __SOURCE_INPUTS folder as default folder for right pane, because it exists
			m_strRightFolderPath = gpApp->m_sourceInputsFolderPath;
		}
	}
	SetupRightList(m_strRightFolderPath);
	SetupSelectionArrays(rightSide);

	pApp->m_bAdminMoveOrCopyIsInitializing = FALSE; // restore default, now initializing is done
	pApp->RefreshStatusBarInfo();
}

void AdminMoveOrCopy::EnableCopyButton(bool bEnableFlag)
{
	if (bEnableFlag)
		pCopyButton->Enable(TRUE);
	else
		pCopyButton->Enable(FALSE);
}

void AdminMoveOrCopy::EnableMoveButton(bool bEnableFlag)
{
	if (bEnableFlag)
		pMoveButton->Enable(TRUE);
	else
		pMoveButton->Enable(FALSE);
}

void AdminMoveOrCopy::EnablePeekButton(bool bEnableFlag)
{
	if (bEnableFlag)
		pPeekButton->Enable(TRUE);
	else
		pPeekButton->Enable(FALSE);
}


void AdminMoveOrCopy::EnableDeleteButton(bool bEnableFlag)
{
	if (bEnableFlag)
		pDeleteButton->Enable(TRUE);
	else
		pDeleteButton->Enable(FALSE);
}

void AdminMoveOrCopy::EnableRenameButton(bool bEnableFlag)
{
	if (bEnableFlag)
		pRenameButton->Enable(TRUE);
	else
		pRenameButton->Enable(FALSE);
}

// redo using a wxListView class -- this is simpler
void AdminMoveOrCopy::DeselectSelectedItems(enum whichSide side)
{
	long index = 0;
	//long limit; // set but unused
	if (side == leftSide)
	{
		//limit = pLeftList->GetItemCount(); // index of last file is (limit - 1)
		if (leftFilesCount == 0 || pLeftList->GetSelectedItemCount() == 0)
		{
			return; // nothing to do
		}
		index = pLeftList->GetFirstSelected();
		if (index != (long)-1)
		{
			// there is one or more selections
			pLeftList->Select(index,FALSE); // FALSE turns selection off
		}
		do {
			index = pLeftList->GetNextSelected(index);
			if (index == -1)
				break;
			// *** BEWARE *** this call uses SetItemState() and it causes
			// m_nCount of wxArrayString to get set to 1 spuriously, for
			// the strLeftSelectedFilesArray after the latter is Empty()ied,
			// so make the .Empty() call be done after DeselectSelectedItems()
			pLeftList->Select(index,FALSE); // FALSE turns selection off
		} while (TRUE);
	}
	else
	{
		//limit = pRightList->GetItemCount(); // index of last file is (limit - 1)
		if (rightFilesCount == 0 || pRightList->GetSelectedItemCount() == 0)
		{
			return; // nothing to do
		}
		index = pRightList->GetFirstSelected();
		if (index != (long)-1)
		{
			// there is one or more selections
			pRightList->Select(index,FALSE); // FALSE turns selection off
		}
		do {
			index = pRightList->GetNextSelected(index);
			if (index == -1)
				break;
			// *** BEWARE *** this call uses SetItemState() and it might cause
			// m_nCount of wxArrayString to get set to 1 spuriously, -- see above
		} while (TRUE);
	}
	// unsigned int aCount = leftSelectedFilesArray.GetCount();  same bug manifests, so
	// wxListView doesn't help get round the problem
}

void AdminMoveOrCopy::GetListCtrlContents(enum whichSide side, wxString& folderPath,
										  bool& bHasFolders, bool& bHasFiles)
{
	// calls helpers.cpp functions GetFoldersOnly() and GetFilesOnly() which are made helpers
	// functions because they may be useful to reuse someday in other places in the app
	bHasFolders = FALSE;
	bHasFiles = FALSE;
	if (side == leftSide)
	{
		// left side of the dialog
		
		// clear out old content in the list and its supporting arrays
		leftFoldersArray.Empty();
		leftFilesArray.Empty();
		pLeftList->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too

		bHasFolders = GetFoldersOnly(folderPath, &leftFoldersArray,TRUE,
						gpApp->m_bAdminMoveOrCopyIsInitializing); // TRUE means to sort the array
		bHasFiles = GetFilesOnly(folderPath, &leftFilesArray); // default is to sort the array

	}
	else
	{
		// right side of the dialog
		
		// clear out old content in the list and its supporting arrays
		rightFoldersArray.Empty();
		rightFilesArray.Empty();
		pRightList->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too

		bHasFolders = GetFoldersOnly(folderPath, &rightFoldersArray, TRUE,
						gpApp->m_bAdminMoveOrCopyIsInitializing); // TRUE means sort the array
		bHasFiles = GetFilesOnly(folderPath, &rightFilesArray); // default is to sort the array
	}

	// debugging -- display what we got for left side & right side too
	/*
#ifdef _DEBUG
	if (side == leftSide)
	{
		size_t foldersCount = leftFoldersArray.GetCount();
		size_t counter;
		wxLogDebug(_T("Folders (sorted, & fetched in ascending index order):\n"));
		for (counter = 0; counter < foldersCount; counter++)
		{
			wxString aFolder = leftFoldersArray.Item(counter);
			wxLogDebug(_T("   %s \n"),aFolder);
		}
		wxLogDebug(_T("Files (sorted, & fetched in ascending index order):\n"));
		size_t filesCount = leftFilesArray.GetCount();
		for (counter = 0; counter < filesCount; counter++)
		{
			wxString aFile = leftFilesArray.Item(counter);
			wxLogDebug(_T("   %s \n"),aFile);
		}
	}
	// nice, both folders and files lists are sorted right and all names correct
	else
	{
		size_t foldersCount = rightFoldersArray.GetCount();
		size_t counter;
		wxLogDebug(_T("DESTINATION folders (sorted, & fetched in ascending index order):\n"));
		for (counter = 0; counter < foldersCount; counter++)
		{
			wxString aFolder = rightFoldersArray.Item(counter);
			wxLogDebug(_T("   %s \n"),aFolder);
		}
		wxLogDebug(_T("DESTINATION files (sorted, & fetched in ascending index order):\n"));
		size_t filesCount = rightFilesArray.GetCount();
		for (counter = 0; counter < filesCount; counter++)
		{
			wxString aFile = rightFilesArray.Item(counter);
			wxLogDebug(_T("   %s \n"),aFile);
		}
	}
#endif
	*/
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void AdminMoveOrCopy::OnOK(wxCommandEvent& event) 
{
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void AdminMoveOrCopy::SetupLeftList(wxString& folderPath)
{
	// put the path into the edit control
	pLeftFolderPathTextCtrl->ChangeValue(folderPath);

	// enumerate the files and folders, insert in list ctrl
	long rv = 0L; // for a return value
	bool bHasFiles;
	bool bHasFolders;
	wxString aFolder;
	wxString aFile;
	GetListCtrlContents(leftSide, folderPath, bHasFolders, bHasFiles);
	if (bHasFolders || bHasFiles)
	{
		leftFoldersCount = 0;
		leftFilesCount = 0;

		// now try put the lines of data in the list; first folders, then files
		size_t index = 0;
		leftFoldersCount = leftFoldersArray.GetCount();
		if (bHasFolders)
		{
			for (index = 0; index < leftFoldersCount; index++)
			{
				aFolder = leftFoldersArray.Item(index);
				rv = pLeftList->InsertItem(index,aFolder,indxFolderIcon);
				rv = rv; // avoid warning
			}
		}
		leftFilesCount = leftFilesArray.GetCount();
		if (bHasFiles)
		{
			// this loop has to start at the index value next after
			// the last value of the folders loop above
			for (index = 0; index < leftFilesCount; index++)
			{
				aFile = leftFilesArray.Item(index);
				rv = pLeftList->InsertItem(leftFoldersCount + index,aFile,indxFileIcon);
				rv = rv; // avoid warning
			}
		}
		// Disable the move and copy buttons at the bottom
		EnableButtons();
	}
	else
	{
        // no files or folders, so disable the move and copy buttons at the bottom, and put
		// a "The folder is empty" message into the list with an empty jug icon, because
		// without an explicit icon, the icon in the list with index = 0 gets shown, and
		// that is the folder icon - which would be confusing, as it would suggest a
		// folder was found with the name "The folder is empty".
		rv = pLeftList->InsertItem(0, emptyFolderMessage,indxEmptyIcon);
		rv = rv; // avoid warning
		leftFoldersCount = 0;
		leftFilesCount = 0;

		EnableButtons();
	}
	pAdminMoveCopySizer->Layout(); // whm added 4Feb11
}

void AdminMoveOrCopy::SetupRightList(wxString& folderPath)
{
	// put the path into the edit control
	pRightFolderPathTextCtrl->ChangeValue(folderPath);

	// enumerate the files and folders, insert in list ctrl
	long rv = 0L; // for a return value
	bool bHasFiles;
	bool bHasFolders;
	wxString aFolder;
	wxString aFile;
	GetListCtrlContents(rightSide, folderPath, bHasFolders, bHasFiles);
	if (bHasFolders || bHasFiles)
	{
		rightFoldersCount = 0;
		rightFilesCount = 0;

		// now try put the lines of data in the list; first folders, then files
		size_t index = 0;
		rightFoldersCount = rightFoldersArray.GetCount();
		if (bHasFolders)
		{
			for (index = 0; index < rightFoldersCount; index++)
			{
				aFolder = rightFoldersArray.Item(index);
				rv = pRightList->InsertItem(index,aFolder,indxFolderIcon);
				rv = rv; // avoid warning
			}
		}
		rightFilesCount = rightFilesArray.GetCount();
		if (bHasFiles)
		{
			// this loop has to start at the index value next after
			// the last value of the folders loop above
			for (index = 0; index < rightFilesCount; index++)
			{
				aFile = rightFilesArray.Item(index);
				rv = pRightList->InsertItem(rightFoldersCount + index,aFile,indxFileIcon);
				rv = rv; // avoid warning
			}
		}
	}
	else
	{
        // no files or folders, put a "The folder is empty" message into the list with an
        // empty faucet icon, because without an explicit icon, the icon in the list with
        // index = 0 gets shown, and that is the folder icon - which would be confusing, as
        // it would suggest a folder was found with the name "The folder is empty".
		rv = pRightList->InsertItem(0, emptyFolderMessage,indxEmptyIcon);
		rv = rv; // avoid warning
		rightFoldersCount = 0;
		rightFilesCount = 0;
	}
	EnableButtons();
	pAdminMoveCopySizer->Layout(); // whm added 4Feb11
}


void AdminMoveOrCopy::OnBnClickedLocateLeftFolder(wxCommandEvent& WXUNUSED(event))
{
	wxString msg = _("Locate the left side folder");
	//long style = wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST | wxDD_CHANGE_DIR;
		// second param suppresses a Create button being shown, 3rd makes chose directory 
		// the working directory, first param is for default dialog style with resizable
		// border (see wxDirDialog for details)
	//long style = wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST | wxDD_CHANGE_DIR;
	long style = wxDD_DEFAULT_STYLE | wxDD_CHANGE_DIR; // allow Create button in dialog
	wxPoint pos = wxDefaultPosition;
	// in the following call, which is a wx widget which can be called as below or as
	// ::wxDirSelector(...params...), if the user cancels from the browser window the
	// returned string is empty, otherwise it is the absolute path to whatever directory
	// was shown selected in the folder hierarchy when the OK button was pressed
	m_strLeftFolderPath_OLD = m_strLeftFolderPath; // save current path in case user cancels
	m_strLeftFolderPath = wxDirSelector(msg,m_strLeftFolderPath,style,pos,(wxWindow*)this);
	if (m_strLeftFolderPath.IsEmpty())
	{
		// restore the old path
		m_strLeftFolderPath = m_strLeftFolderPath_OLD;
	}
	SetupLeftList(m_strLeftFolderPath);
	EnableButtons();
	SetNeitherSideHasFocus();
}

void AdminMoveOrCopy::OnBnClickedLocateRightFolder(wxCommandEvent& WXUNUSED(event))
{
	wxString msg = _("Locate the right side folder");
	//long style = wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST | wxDD_CHANGE_DIR;
		// second param suppresses a Create button being shown, 3rd makes chose directory 
		// the working directory, first param is for default dialog style with resizable
		// border (see wxDirDialog for details)
	//long style = wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST | wxDD_CHANGE_DIR;
	long style = wxDD_DEFAULT_STYLE | wxDD_CHANGE_DIR; // allow Create button in dialog
	wxPoint pos = wxDefaultPosition;
	// in the following call, which is a wx widget which can be called as below or as
	// ::wxDirSelector(...params...), if the user cancels from the browser window the
	// returned string is empty, otherwise it is the absolute path to whatever directory
	// was shown selected in the folder hierarchy when the OK button was pressed
	m_strRightFolderPath_OLD = m_strRightFolderPath; // save current path in case user cancels
	m_strRightFolderPath = wxDirSelector(msg,m_strRightFolderPath,style,pos,(wxWindow*)this);
	if (m_strRightFolderPath.IsEmpty())
	{
		// restore the old path
		m_strRightFolderPath = m_strRightFolderPath_OLD;
	}
	SetupRightList(m_strRightFolderPath);
	EnableButtons();
	SetNeitherSideHasFocus();
}

void AdminMoveOrCopy::OnBnClickedLeftParentFolder(wxCommandEvent& WXUNUSED(event))
{
	wxFileName* pFN = new wxFileName;
	wxChar charSeparator = pFN->GetPathSeparator();
	int offset;
	wxString path = m_strLeftFolderPath;
	offset = path.Find(charSeparator,TRUE); // TRUE is bFromEnd
	if (offset != wxNOT_FOUND)
	{
		path = path.Left(offset);
		// check we have not obtained the nothing which precedes the Linux / root
		// folder marker, nor the "C:" or other volume indicator in Windows - as we've
		// moved up as far as we can go if we had either situation
		if (!path.IsEmpty())	
		{
			if (path.Last() == _T(':'))
			{
				// we are at the windows volume separator, so restore the following
				// backslash and then display the volume's contents
				path += _T("\\");
				m_strLeftFolderPath = path;
				SetupLeftList(m_strLeftFolderPath);
			}
			else
			{
				// we've a legitimate path, so make it the left path
				m_strLeftFolderPath = path;
				SetupLeftList(m_strLeftFolderPath);
			}
		}
		else
		{
			// we are at the root in Linux or Unix
			path = charSeparator;
			m_strLeftFolderPath = path;
			SetupLeftList(m_strLeftFolderPath);
		}
	}
	if (pFN != NULL) // whm 11Jun12 added NULL test
		delete pFN;
	SetNeitherSideHasFocus();
	EnableButtons();
}

void AdminMoveOrCopy::OnBnClickedRightParentFolder(wxCommandEvent& WXUNUSED(event))
{
	wxFileName* pFN = new wxFileName;
	wxChar charSeparator = pFN->GetPathSeparator();
	int offset;
	wxString path = m_strRightFolderPath;
	offset = path.Find(charSeparator,TRUE); // TRUE is bFromEnd
	if (offset != wxNOT_FOUND)
	{
		path = path.Left(offset);
		// check we have not obtained the nothing which precedes the Linux / root
		// folder marker, nor the "C:" or other volume indicator in Windows - as we've
		// moved up as far as we can go if we had either situation
		if (!path.IsEmpty())	
		{
			if (path.Last() == _T(':'))
			{
				// we are at the windows volume separator, so restore the following
				// backslash and then display the volume's contents
				path += _T("\\");
				m_strRightFolderPath = path;
				SetupRightList(m_strRightFolderPath);
			}
			else
			{
				// we've a legitimate path, so make it the left path
				m_strRightFolderPath = path;
				SetupRightList(m_strRightFolderPath);
			}
		}
		else
		{
			// we are at the root in Linux or Unix
			path = charSeparator;
			m_strRightFolderPath = path;
			SetupRightList(m_strRightFolderPath);
		}
	}
	if (pFN != NULL) // whm 11Jun12 added NULL test
		delete pFN;
	SetNeitherSideHasFocus();
	EnableButtons();
}

void AdminMoveOrCopy::OnSize(wxSizeEvent& event)
{
	if (this == event.GetEventObject())
	{
		// it's our AdminMoveOrCopy dialog
		// re-set the left column's width
		if (pLeftList == NULL || pRightList == NULL)
		{
			event.Skip();
			return;
		}
		// control can still get here when the control is not setup initially, so next
		// block should catch those (for which pLeftList is not NULL, but not defined)
		if (pLeftList->GetColumnCount() == 0 || pRightList->GetColumnCount() == 0 ||
			(wxImageList*)pLeftList->GetImageList(wxIMAGE_LIST_SMALL) == NULL ||
			(wxImageList*)pRightList->GetImageList(wxIMAGE_LIST_SMALL) == NULL)
		{
			event.Skip();
			return;
		}
		int colWidth_Left = pLeftList->GetColumnWidth(0);
		int colWidth_Right = pRightList->GetColumnWidth(0);
		int width_Left; int width_Right; int height_Left; int height_Right;
		pLeftList->GetClientSize(&width_Left,&height_Left);
		pRightList->GetClientSize(&width_Right,&height_Right);
		if (width_Left != colWidth_Left)
			pLeftList->SetColumnWidth(0,width_Left);
		if (width_Right != colWidth_Right)
			pRightList->SetColumnWidth(0,width_Right);
	}
	event.Skip();
}
/////////////////////////////////////////////////////////////////////////////////
/// \return     the suitably changed filename (so that the conflict is removed)
/// \param      pFilename   ->  pointer to the filename string from the right hand
///                             folder which conflicts with the filename being moved or
///                             copied and user has elected to do the copy with a modified
///                             name 
///  \remarks   The typical situation is a filename is passed in, and the filename with
///  (2) appended to the end of the name part and before the dot plus extension, if any,
///  is returned. If (2) or (3) or (n) where n is one or more digits is aleady present at
///  the end of the name part of the filename, then the value is bumped one higher in the
///  returned string. If the string between ( and ) is empty, or cannot be converted to a
///  number, or ( or ) occurs without a matching partner ) or ( respectively, or if the
///  (n) substring is internal to the filename, then any such cases just have "(2)"
///  appended to the end of the name part of the filename, as above.
//////////////////////////////////////////////////////////////////////////////////
wxString AdminMoveOrCopy::BuildChangedFilenameForCopy(wxString* pFilename)
{
	wxString newFilename = _T("");
	wxFileName fn(m_strRightFolderPath,*pFilename);
	wxString extn = _T("");
	//bool bHasExtension = FALSE; // set but unused
	if (fn.HasExt())
	{
		extn = fn.GetExt();
		//bHasExtension = TRUE; // to handle when only the . of an extension is present
	}
	wxString name = fn.GetName();
	wxString reversed = MakeReverse(name); // our utility from helpers.cpp
	// look for ")d...d(" at the start of the reversed string, where d...d is one or more
	// digits; we want to get the digit(s), convert to int, increment by 1, convert back
	// to digits, and build the new string with (n) at the end where n is the new larger
	// value. However, mostly no such end string is present, in which case we can just
	// create a name with "(2)" at the end immediately.
	wxString shortname;
	// whm 11Jun12 added test to ensure that reversed is not empty when GetChar(0) is called
	wxChar aChar;
	if (!reversed.IsEmpty())
		aChar = reversed.GetChar(0);
	else
		aChar = _T('\0');
	wxString ending = _T("");
	if (aChar == _T(')'))
	{
		// we've got a filename with the name part in the form name(n) where n is one or
		// more digits (or at least we'll assume so)
		ending = aChar;
		shortname = reversed.Mid(1);
		// get the digits -- look for matching '(', if not found, just add "(2)" as below,
		// but if found, the characters up to that point should be the digit string we want
		int offset = shortname.Find(_T('('));
		if (offset == wxNOT_FOUND)
		{
			newFilename = name + _T("(2)");
			newFilename += _T(".") + extn;
		}
		else
		{
			wxString digitStr = shortname.Left(offset);
			shortname = shortname.Mid(offset); // this is now "(reversednamepart"
			// reverse digitStr, to make it normal order
			digitStr = MakeReverse(digitStr);
			// convert digitStr to an unsigned long
			unsigned long value;
			bool bConvertedOK = digitStr.ToULong(&value);
			if (!bConvertedOK)
			{
				// it wasn't a valid digit string, so make a (2) at end of original name
				newFilename = name + _T("(2)");
				newFilename += _T(".") + extn;
			}
			else
			{
				// it converted correctly, so bump the value by 1 and rebuild the new
				// string's output form
				value++;
				digitStr.Printf(_T("%d"),value);
				// now reverse it again
				digitStr = MakeReverse(digitStr);
				// prepend to shortname
				shortname = digitStr + shortname;
				// add the ending
				shortname = ending + shortname; // remember this is still reversed!
				// now reverse it back to natural order
				shortname = MakeReverse(shortname);
				newFilename = shortname + _T(".") + extn;
			}
		}
	}
	else
	{
		// assume it is a normal filename with no (n) on the end
		newFilename = name + _T("(2)");
		newFilename += _T(".") + extn;
	}
	return newFilename;
}

void AdminMoveOrCopy::OnLeftListSelectItem(wxListEvent& event)
{
    // we repopulate the leftSelectedFoldersArray and the leftSelectedFilesArray each time
    // with the set of selected items 
	//wxLogDebug(_T("OnLeftListSelectItem -- provides data for doubleclick handler"));
	event.Skip();
	SetupSelectionArrays(leftSide); // update leftSelectedFoldersArray and
									 // leftSelectedFilesArray with current selections
	SetLeftSideHasFocus();
}

void AdminMoveOrCopy::OnRightListSelectItem(wxListEvent& event)
{
    // we repopulate the rightSelectedFoldersArray and the rightSelectedFilesArray each time
    // with the set of selected items
	//wxLogDebug(_T("OnRightListSelectItem"));
	event.Skip();
	SetupSelectionArrays(rightSide); // update rightSelectedFoldersArray and
								     // rightSelectedFilesArray with current selections
	SetRightSideHasFocus();
}

void AdminMoveOrCopy::OnLeftListDeselectItem(wxListEvent& event)
{
	event.Skip();

	// get the clicked item's index
	long itemToDeselect = event.GetIndex();
	long stateMask = 0;
	stateMask |= wxLIST_STATE_SELECTED;
	long focusMask = 0;
	focusMask |= wxLIST_STATE_FOCUSED;
	// deselect it (ignore returned boolean)
	// and if it's also focused, remove the focus
	pLeftList->SetItemState((long)itemToDeselect,(long)0,stateMask);
	pLeftList->SetItemState((long)itemToDeselect,(long)0,focusMask);

	// re-establish the arrays for any which remain selected
	SetupSelectionArrays(leftSide); // update leftSelectedFilesArray 
									 // with current selections 
}

void AdminMoveOrCopy::OnRightListDeselectItem(wxListEvent& event)
{
	event.Skip();

	// get the clicked item's index
	long itemToDeselect = event.GetIndex();
	long stateMask = 0;
	stateMask |= wxLIST_STATE_SELECTED;
	long focusMask = 0;
	focusMask |= wxLIST_STATE_FOCUSED;
	// deselect it (ignore returned boolean)
	// and if it's also focused, remove the focus
	pRightList->SetItemState((long)itemToDeselect,(long)0,stateMask);
	pRightList->SetItemState((long)itemToDeselect,(long)0,focusMask);

	// re-establish the arrays for any which remain selected
	SetupSelectionArrays(rightSide); // update rightSelectedFilesArray 
									 // with current selections 
}

void AdminMoveOrCopy::SetLeftSideHasFocus()
{
	DoDeselectionAndDefocus(rightSide);
	sideWithFocus = leftSideHasFocus;
	pRightList->SetBackgroundColour(nocolor);
	pRightList->Refresh();
	pLeftList->SetBackgroundColour(pastelgreen);
	pLeftList->Refresh();
}

void AdminMoveOrCopy::SetRightSideHasFocus()
{
	DoDeselectionAndDefocus(leftSide);
	sideWithFocus = rightSideHasFocus;
	pLeftList->SetBackgroundColour(nocolor);
	pLeftList->Refresh();
	pRightList->SetBackgroundColour(pastelgreen);
	pRightList->Refresh();
}

void AdminMoveOrCopy::SetNeitherSideHasFocus()
{
	DoDeselectionAndDefocus(rightSide);
	DoDeselectionAndDefocus(leftSide);
	sideWithFocus = neitherSideHasFocus;
	pLeftList->SetBackgroundColour(nocolor);
	pRightList->SetBackgroundColour(nocolor);
	pRightList->Refresh();
	pLeftList->Refresh();
}
/*
void AdminMoveOrCopy::OnLeftListKeyDown(wxListEvent& event)
{
	int keyCode = event.GetKeyCode();

	event.Skip();
}

void AdminMoveOrCopy::OnRightListKeyDown(wxListEvent& event)
{
	int keyCode = event.GetKeyCode();

	event.Skip();
}
*/



/////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      side    ->  enum with value either leftSide, or rightSide 
///  \remarks Depopulates leftSelectionArray, or rightSelectionArray to empty strings, and
///  most importantly, removes from the wxListCtrl all memory of its selections, and the
///  memory of which item, if any, had the focus. The clobbering of focus and selections is
///  needed because if the user clicks on the other pane, the visible selections in the
///  first pane are lost, but the control retains the memory of them and also which item
///  had the focus, and then code which relies on the focus changing when the other list is
///  clicked would not work if the user were to click on a formerly focused item, leading
///  to defeat of the code to find out which side we are dealing with. So whenever a
///  selection or deselection is made on one side, we always clobber everything on the
///  other side. (This function does more than DeselectSelectedItems() - the latter only
///  deals with selections.)
///  BEW 26July10, added, to support bidirectional moves and copying, and peeks, etc, on
///  either side
//////////////////////////////////////////////////////////////////////////////////
void AdminMoveOrCopy::DoDeselectionAndDefocus(enum whichSide side)
{
	size_t index = 0;
	size_t limit = 0;
	long stateMask = 0;
	stateMask |= wxLIST_STATE_SELECTED;
	long focusMask = 0;
	focusMask |= wxLIST_STATE_FOCUSED;
	long bothMask = stateMask | focusMask; // check for both selection and focus
	int isSelected = 0;
	if (side == leftSide)
	{
		leftSelectedFilesArray.Empty();
		leftSelectedFoldersArray.Empty();
		limit = pLeftList->GetItemCount();
		if ((leftFilesCount == 0 && leftFoldersCount == 0) || 
			pLeftList->GetSelectedItemCount() == 0)
		{
			return; // nothing to do
		}
		for (index = 0; index < limit; index++)
		{
			isSelected = pLeftList->GetItemState(index,bothMask);
			if (isSelected)
			{
				// this one is selected, deselect it (ignore returned boolean)
				// and if it's also focused, remove the focus
				pLeftList->SetItemState((long)index,(long)0,stateMask);
				pLeftList->SetItemState((long)index,(long)0,focusMask);
			}
		}
	}
	else
	{
		rightSelectedFilesArray.Empty(); // clear, we'll refill in the loop
		rightSelectedFoldersArray.Empty(); // ditto
		limit = pRightList->GetItemCount();
		if ((rightFilesCount == 0 && rightFoldersCount == 0) || 
			pRightList->GetSelectedItemCount() == 0)
		{
			return; // nothing to do
		}
		for (index = 0; index < limit; index++)
		{
			isSelected = pRightList->GetItemState(index,bothMask);
			if (isSelected)
			{
				// this one is selected, deselect it (ignore returned boolean)
				// and if it's also focused, remove the focus
				pRightList->SetItemState((long)index,(long)0,stateMask);
				pRightList->SetItemState((long)index,(long)0,focusMask);
			}
		}
	}
}

void AdminMoveOrCopy::OnLeftListDoubleclick(wxListEvent& event)
{
	//wxLogDebug(_T("OnLeftListDoubleclick"));
	size_t index = event.GetIndex();
	if (index < leftFoldersCount)
	{
		// we clicked on a folder name, so drill down to that child folder and display its
		// contents in the dialog;  check here for an empty string in m_leftItemText, and if so
		// then don't do anything in this handler
		if (event.GetText().IsEmpty())
			return;		

		// extend the path using this foldername, and then display the contents
		m_strLeftFolderPath += gpApp->PathSeparator + event.GetText();
		SetupLeftList(m_strLeftFolderPath);
		//event.Skip();
		SetNeitherSideHasFocus();
		EnableButtons();
	}
}


void AdminMoveOrCopy::OnRightListDoubleclick(wxListEvent& event)
{
	//wxLogDebug(_T("OnRightListDoubleclick"));
	//size_t index = m_rightIndex; // set from the preceding OnLeftListSelectItem() call,
							   // (the dbl click event is posted after the selection event)
	size_t index = event.GetIndex();
	if (index < rightFoldersCount)
	{
		// we clicked on a folder name, so drill down to that child folder and display its
		// contents in the dialog;  check here for an empty string in m_rightItemText, and if so
		// then don't do anything in this handler
		if (event.GetText().IsEmpty())
			return;		

		// extend the path using this foldername, and then display the contents
		m_strRightFolderPath += gpApp->PathSeparator + event.GetText();
		SetupRightList(m_strRightFolderPath);
		SetNeitherSideHasFocus();
		EnableButtons();
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      side    ->  enum with value either leftSide, or rightSide 
///  \remarks   Populates leftSelectionArray, or rightSelectionArray, with wxString filename
///  entries or foldername entries or both, depending on what is selected in the passed in
///  side's wxListCtrl.
///  A side effect is to enable or disable the Move, Copy, Rename and/or Delete buttons. If
///  multiple files or folders or both are selected, this function is only used at the top
///  level of the handler for such butons, because the handlers are recursive and for child
///  folders the selections are set up programmatically. Another job it does is to populate
///  the member arrays, leftSelectedFilesArray and leftSelectedFoldersArray, or if the
///  selections are on the right side, the rightSelectedFilesArray and
///  rightSelectedFoldersArray, as these are needed at the top level of the Copy or Move
///  handlers.
//////////////////////////////////////////////////////////////////////////////////
void AdminMoveOrCopy::SetupSelectionArrays(enum whichSide side)
{
	size_t index = 0;
	size_t limit = 0;
	long stateMask = 0;
	stateMask |= wxLIST_STATE_SELECTED;
	int isSelected = 0;
	if (side == leftSide)
	{
		leftSelectedFilesArray.Empty();
		leftSelectedFoldersArray.Empty();
		limit = pLeftList->GetItemCount();
		if ((leftFilesCount == 0 && leftFoldersCount == 0) || 
			pLeftList->GetSelectedItemCount() == 0)
		{
			EnableButtons();
			return; // nothing to do
		}
		leftSelectedFoldersArray.Alloc(leftFoldersCount);
		leftSelectedFilesArray.Alloc(leftFilesCount);
		for (index = 0; index < limit; index++)
		{
			isSelected = pLeftList->GetItemState(index,stateMask);
			if (isSelected)
			{
				// this one is selected, add it's name to the list of selected items
				wxString itemName = pLeftList->GetItemText(index);

				// is it a folder or a file - find out and add it to the appropriate array
				wxString path = m_strLeftFolderPath + gpApp->PathSeparator + itemName;
				size_t itsIndex = 0xFFFF;
				if (::wxDirExists(path.c_str()))
				{
					// it's a directory
					itsIndex = leftSelectedFoldersArray.Add(itemName);
					itsIndex = itsIndex; // avoid warning
				}
				else
				{
					// it'a a file
					itsIndex = leftSelectedFilesArray.Add(itemName);
					itsIndex = itsIndex; // avoid warning
				}
				isSelected = 0;
			}
		}
		if (m_strLeftFolderPath.IsEmpty() || m_strRightFolderPath.IsEmpty())
		{
			// don't use EnableButtons() here, because we can't move or copy if one of the
			// panes has no folder defined
			EnableCopyButton(FALSE);
			EnableMoveButton(FALSE);
		}
		else
		{
			EnableButtons();
		}
	}
	else
	{
		rightSelectedFilesArray.Empty(); // clear, we'll refill in the loop
		rightSelectedFoldersArray.Empty(); // ditto
		limit = pRightList->GetItemCount();
		if ((rightFilesCount == 0 && rightFoldersCount == 0) || 
			pRightList->GetSelectedItemCount() == 0)
		{
			EnableButtons();
			return; // nothing to do
		}
		rightSelectedFilesArray.Alloc(limit);
		rightSelectedFoldersArray.Alloc(limit);
		for (index = 0; index < limit; index++)
		{
			isSelected = pRightList->GetItemState(index,stateMask);
			if (isSelected)
			{
				// this one is selected, add it's name to the list of selected items
				wxString itemName = pRightList->GetItemText(index);

				// is it a folder or a file - find out and add it to the appropriate array
				wxString path = m_strRightFolderPath + gpApp->PathSeparator + itemName;
				size_t itsIndex = 0xFFFF;
				if (::wxDirExists(path.c_str()))
				{
					// it's a directory
					itsIndex = rightSelectedFoldersArray.Add(itemName);
					itsIndex = itsIndex; // avoid warning
				}
				else
				{
					// it'a a file
					itsIndex = rightSelectedFilesArray.Add(itemName);
					itsIndex = itsIndex; // avoid warning
				}
				isSelected = 0;
			}
		}
		if (m_strRightFolderPath.IsEmpty() || m_strLeftFolderPath.IsEmpty())
		{
			// don't use EnableButtons() here, because we can't move or copy if one of the
			// panes has no folder defined
			EnableCopyButton(FALSE);
			EnableMoveButton(FALSE);
		}
		else
		{
			EnableButtons();
		}
	}
}

void AdminMoveOrCopy::EnableButtons()
{
	// enable Rename button only if a single file is selected, or a single folder, either side
	if ( (rightSelectedFilesArray.GetCount() == 1 && rightSelectedFoldersArray.GetCount() == 0) ||
		 (leftSelectedFilesArray.GetCount() == 1 && leftSelectedFoldersArray.GetCount() == 0) ||
		 (rightSelectedFilesArray.GetCount() == 0 && rightSelectedFoldersArray.GetCount() == 1) ||
		 (leftSelectedFilesArray.GetCount() == 0 && leftSelectedFoldersArray.GetCount() == 1)
	   )
	{
		EnableRenameButton(TRUE);
	}
	else
	{
		EnableRenameButton(FALSE);
	}
	// the other three buttons should be enabled if a folder or file (or several of those)
	// are selected on one side or the other
	if (	rightSelectedFilesArray.GetCount() > 0 ||
			rightSelectedFoldersArray.GetCount() > 0 ||
			leftSelectedFilesArray.GetCount() > 0 ||
			leftSelectedFoldersArray.GetCount() > 0)
	{
		EnableMoveButton(TRUE);
		EnableCopyButton(TRUE);
		EnableDeleteButton(TRUE);
		EnablePeekButton(TRUE);
	}
	else
	{
		EnableMoveButton(FALSE);
		EnableCopyButton(FALSE);
		EnableDeleteButton(FALSE);
		EnablePeekButton(FALSE);
	}
}

///////////////////////////////////////////////////////////////////////////////////
///
///    END OF GUI FUNCTIONS                 START OF BUTTON HANDLERS 
///
///////////////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if the passed in paths are identical, FALSE otherwise
/// \param      leftPath    ->  reference to the left side's file path 
/// \param      rightPath   ->  reference to the right side's file path 
///  \remarks   Use to prevent copy or move of a file or folder to the same folder. We
///  allow both sides to have the same path shown, but in the button handlers we check as
///  necessary and prevent wrong action - and show a warning message
//////////////////////////////////////////////////////////////////////////////////
bool AdminMoveOrCopy::CheckForIdenticalPaths(wxString& leftPath, wxString& rightPath)
{
	if (leftPath == rightPath)
	{
		wxString msg;
		::wxBell();
		msg = msg.Format(_("The source and destination folders must not be the same folder."));
		wxMessageBox(msg,_("Copy or Move is not permitted"),wxICON_EXCLAMATION | wxOK);
		return TRUE;
	}
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if the file in the left list with index leftFileIndex has the
///             same filename as a file in the right hand folder; FALSE otherwise
/// \param      leftFile        ->   reference to the left file's filename
/// \param      pConflictIndex <-   pointer to 0 based index to whichever filename in
///                                 rightFilesArray (passed in as pRightFilesArr) has 
///                                 the same name as for the one specified by leftFile;
///                                 has value -1 if unset
/// \param      pRightFilesArr   ->  pointer to a string array of all the filenames in
///                                 the right folder
///  \remarks   Iterates through all the files in the pRightFilesArr array, looking for
///             a match with the filename specified by leftFileIndex, returning TRUE if
///             a match is made, otherwise FALSE
//////////////////////////////////////////////////////////////////////////////////
bool AdminMoveOrCopy::IsFileConflicted(wxString& leftFile, int* pConflictIndex, 
									  wxArrayString* pRightFilesArr)
{
	wxASSERT(!leftFile.IsEmpty());
	(*pConflictIndex) = wxNOT_FOUND; // -1
	size_t limit = pRightFilesArr->GetCount();
	if (limit == 0)
	{
		// there are no files in the right folder, so no conflict is possible
		return FALSE;
	}
	size_t rightIndex;
	wxString aFilename;
	for (rightIndex = 0; rightIndex < limit; rightIndex++) 
	{
		aFilename = pRightFilesArr->Item(rightIndex);
		if (aFilename == leftFile)
		{
			(*pConflictIndex) = (int)rightIndex;
			return TRUE;
		}
	}
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if the copy was successfully done (in the event of a filename clash,
///             successfully done means either that the user chose to copy and replace,
///             overwriting the file in the right folder of the same name, or he
///             chose to copy with a changed name; a choice to 'not copy' should be
///             interpretted as non-success and FALSE should be returned); return FALSE if
///             the copy was not successful. The "Cancel" response in the event of a
///             filename clash should also cause FALSE to be returned.
/// \param      leftPath     -> reference to the left folder path's string (no final 
///                             path separator)
/// \param      rightPath    -> reference to the right folder path's string (no final 
///                             path separator)
/// \param      filename    ->  the name of the file to be copied (prior to knowing whether 
///                             there is or isn't a name conflict with a file in the 
///                             right folder)
///  \param     bUserCancelled <-> reference to boolean indicating whether or not the user
///                               clicked the Cancel button in the FilenameConflictDlg
///  \remarks   This function handles the low level copying of a single file to the
///  right folder. It's return value is important for a "Move" choice in the GUI,
///  because we support Moving a file by first copying it, and if the copy was successful,
///  then we delete the original file in the left folder. In the event of a filename
///  clash, the protocol for handling that is encapsulated herein - a child dialog is put
///  up with 3 choices, similarly to what Windows Explorer offers. A flag is included in
///  the signature to return cancel information to the caller in the event of a filename
///  clash. Since our design allows for multiple filenames in the left folder to be
///  copied (or moved) with a single click of the Copy.. or Move.. buttons, this function
///  may be called multiple times by the caller - taking each filename from a string array
///  populated earlier. In the event of a filename clash, if the user hits the Cancel
///  button, it will cancel the Copy or Move command at that time, but any files already
///  copied or moved will remain so.
//////////////////////////////////////////////////////////////////////////////////
bool AdminMoveOrCopy::CopySingleFile(wxString& leftPath, wxString& rightPath, wxString& filename, 
						bool& bUserCancelled)
{
	wxString theRightPath; // build the path to the file here, from rightPath and filename
	bool bSuccess = TRUE;
	copyType = copyAndReplace; // set to default value
	wxString newFilenameStr = _T("");

	// make the path to the left file
	wxString theLeftPath = leftPath + gpApp->PathSeparator + filename;

	// get an array of the filenames in the right folder
	wxArrayString myRightFilesArray;
	bool bGotten = GetFilesOnly(rightPath, &myRightFilesArray, TRUE, TRUE);
	bGotten = bGotten; // avoid compiler warning

	// check for a name conflict (including extension, same names but different extensions
	// is not to be treated as a conflict)
	int nConflictedFileIndex = wxNOT_FOUND; // -1
	bool bIsConflicted = IsFileConflicted(filename,&nConflictedFileIndex,&myRightFilesArray);
	if (bIsConflicted)
	{
		// handle the filename conflict here; the existence of a conflict is testimony to
		// the fact that the right hand folder has the conflicting file in it, so we
		// don't need to test for it here, so we just construct the path and use it

		// put up the filename conflict dialog (similar options as Win Explorer has for
		// conflicts), but bypass it if the user has previous requested conflicts be
		// processed as he stipulated in an earlier pass thru the code
		if (m_bDoTheSameWay)
		{
			// m_bDoTheSameWay has been set to TRUE, so here we must copy the file, but
			// handle any conflict the same way as before without showing the
			// conflict dialog to the user -- the enum variable, lastWay, contains the
			// enum value used for the last conflict dealt with
			switch (lastWay)
			{
			case copyAndReplace:
				theRightPath = rightPath + gpApp->PathSeparator + filename;
				bSuccess = ::wxCopyFile(theLeftPath, theRightPath); //bool overwrite = true
				break;
			case copyWithChangedName:
				newFilenameStr = BuildChangedFilenameForCopy(&filename);
				theRightPath = rightPath + gpApp->PathSeparator + newFilenameStr;
				bSuccess = ::wxCopyFile(theLeftPath, theRightPath); //bool overwrite = true
				break;
			default:
			case noCopy:
				// treat this as an unsuccessful copy, & don't copy this file
				bSuccess = FALSE;
				break;
			}
			return bSuccess;
		}
		else
		{
			// put conflict dialog up
			FilenameConflictDlg dlg(this,&filename,&leftPath,&rightPath);
			dlg.Centre();
			if (dlg.ShowModal() == wxID_OK)
			{
				// Close button was pressed, so get the user's choices
				m_bDoTheSameWay = dlg.bSameWayValue;
				lastWay = copyType; // update lastWay enum value
                // The value of the copyType enum has been set at the Close button call in
                // the FilenameConflictDlg
				//m_bDoTheSameWay = m_bDoTheSameWay; // for checking value in debugger
				switch (copyType)
				{
				case copyAndReplace:
					theRightPath = rightPath + gpApp->PathSeparator + filename;
					bSuccess = ::wxCopyFile(theLeftPath, theRightPath); //bool overwrite = true
					break;
				case copyWithChangedName:
					newFilenameStr = BuildChangedFilenameForCopy(&filename);
					theRightPath = rightPath + gpApp->PathSeparator + newFilenameStr;
					bSuccess = ::wxCopyFile(theLeftPath, theRightPath); //bool overwrite = true
					break;
				default:
				case noCopy:
					// treat this as an unsuccessful copy, & don't copy this file
					bSuccess = FALSE;
					break;
				}
			}
			else
			{
				// Cancel button was pressed
				bUserCancelled = TRUE;
				bSuccess = FALSE;
			}
		}
		return bSuccess;
	}
	else
	{
		// there was no conflict, so the copy can proceed...
		// first, make the path to the right hand folder
		theRightPath = rightPath + gpApp->PathSeparator + filename;
		bool bAlreadyExists = ::wxFileExists(theRightPath);
		if (bAlreadyExists)
		{
            // This block should never be entered; we've already determined there is no
            // name conflict and so the destination folder should not have a file of the
            // same name. Because of the possibility of losing valuable data, what we do
            // here is to alert the user to the file which is in danger of being lost due
            // to the copy, then refraining from the copy automatically, keep the app
            // running but return FALSE so that if a Move... was requested, then the caller
            // will not go ahead with deletion of the other folder's file
			wxString msg;
			msg = msg.Format(_("The right folder's file with the name %s would be overwritten if this move or copy were to go ahead. To avoid this unexpected possibility for data loss, the move or copy will now be cancelled. Do something appropriate with the right folder's file, and then try again."),
				filename.c_str());
			wxMessageBox(msg,_("Unexpected Filename Conflict During Copy Or Move"),wxICON_EXCLAMATION | wxOK);
			return FALSE;
		}
		bool bSuccess2 = ::wxCopyFile(theLeftPath, theRightPath); //bool overwrite = true
		if (!bSuccess2)
		{
			wxString msg;
			msg = msg.Format(
_("Moving or copying the file with path %s failed unexpectedly. Possibly you forgot to use the button for locating the folder to show on the right. Do so then try again."),
theLeftPath.c_str());
			wxMessageBox(msg,_("Moving or copying failed"),wxICON_EXCLAMATION | wxOK);
			if (bSuccess)
				bSuccess = FALSE;
		}
	}
	return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if the removal was successfully done; return FALSE if
///             the removal was not successful. (FALSE should halt the removals.)
/// \param      thePath    ->   reference to the folder's path string (no final 
///                             path separator)-- removals are done from either side
/// \param      filename    ->  the name of the file to be removed
///  \remarks   This function handles the removal of one or more selected files and / or
///  folders. The selections can be made in either pane.
///  BEW 28July10, change variable names to better reflect the fact that the  function can
///  be used to remove a file from either left or right pane
//////////////////////////////////////////////////////////////////////////////////
bool AdminMoveOrCopy::RemoveSingleFile(wxString& thePath, wxString& filename)
{
	bool bSuccess = TRUE;

	// make the path to the folder where the file resides
	wxString thisPath = thePath + gpApp->PathSeparator + filename;

	// do the removal
	bSuccess = ::wxRemoveFile(thisPath); //bool overwrite = true
	if (!bSuccess)
	{
		wxString msg;
		msg = msg.Format(
_("Removing the file with path %s failed. Removals have been halted. Possibly an application has the file still open. Close the file and then you can try the removal operation again."),
		thisPath.c_str());
		wxMessageBox(msg,_("Removing a file failed"),wxICON_EXCLAMATION | wxOK);

		// use the m_bUserCancelled mechanism to force drilling up through any recursions,
		// but don't clobber the AdminMoveOrCopy dialog itself
		m_bUserCancelled = TRUE;
	}
	return bSuccess;
}

// BEW 28July10, tweaked the code to allow deletion or files and/or folders from either
// left or right pane
void AdminMoveOrCopy::OnBnClickedDelete(wxCommandEvent& WXUNUSED(event))
{
	m_bUserCancelled = FALSE;

	// on the heap, create the arrays of files and folders which are
	// to be removed & populate them
	wxArrayString* pSelectedFoldersArray = new wxArrayString;
	wxArrayString* pSelectedFilesArray = new wxArrayString;
	size_t index;
	size_t foldersLimit = 0;
	size_t filesLimit = 0;
	// set these three with a switch
	wxArrayString* pPaneSelectedFolders = NULL;
	wxArrayString* pPaneSelectedFiles = NULL;
	wxString pathToPane;

	switch (sideWithFocus)
	{ 
		case leftSideHasFocus:
			{
				pathToPane = m_strLeftFolderPath;
				pPaneSelectedFolders = &leftSelectedFoldersArray;
				pPaneSelectedFiles = &leftSelectedFilesArray;

			} // end of case for left side having focus
			break;
		case rightSideHasFocus:
			{
				pathToPane = m_strRightFolderPath;
				pPaneSelectedFolders = &rightSelectedFoldersArray;
				pPaneSelectedFiles = &rightSelectedFilesArray;
			} // end of case for right side having focus
			break;
		case neitherSideHasFocus:
		default:
			{
				wxASSERT(FALSE);
				wxBell();
				if (pSelectedFilesArray != NULL) // whm 11Jun12 added NULL test
					delete pSelectedFilesArray; // don't leak
				if (pSelectedFoldersArray != NULL) // whm 11Jun12 added NULL test
					delete pSelectedFoldersArray; // don't leak
				SetNeitherSideHasFocus();
				EnableButtons();
				return;
			}
	} // end of switch

	if (pPaneSelectedFolders != NULL)
		foldersLimit = pPaneSelectedFolders->GetCount(); // populated in SetSelectionArray() call earlier
	if (pPaneSelectedFiles != NULL)
		filesLimit = pPaneSelectedFiles->GetCount(); // populated in SetSelectionArray() call earlier
	if (filesLimit == 0 && foldersLimit == 0)
	{
		wxMessageBox(
_("You first need to select at least one item in either list before clicking the Delete button"),
		_("No Files Or Folders Selected"),wxICON_EXCLAMATION | wxOK);
		pSelectedFilesArray->Clear(); // this one is on heap
		pSelectedFoldersArray->Clear(); // this one is on heap
		if (pSelectedFilesArray != NULL) // whm 11Jun12 added NULL test
			delete pSelectedFilesArray; // don't leak it
		if (pSelectedFoldersArray != NULL) // whm 11Jun12 added NULL test
			delete pSelectedFoldersArray; // don't leak it
		SetNeitherSideHasFocus();
		EnableButtons();
		return;
	}
	for (index = 0; index < foldersLimit; index++)
	{
		pSelectedFoldersArray->Add(pPaneSelectedFolders->Item(index));
	}
	for (index = 0; index < filesLimit; index++)
	{
		pSelectedFilesArray->Add(pPaneSelectedFiles->Item(index));
	}

	// whm 9Feb12 added: Prompt with a warning before calling RemoveFilesAndFolders() below
	int response;
	response = wxMessageBox(_T("Delete the selected item(s)?"),_T(""),wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT);
	if (response == wxYES)
	{
		RemoveFilesAndFolders(pathToPane, pSelectedFoldersArray, pSelectedFilesArray);

		gpApp->LogUserAction(_T("Deleted file(s)"));
	}

	// clear the allocations to the heap
	pSelectedFilesArray->Clear(); // this one is on heap
	pSelectedFoldersArray->Clear(); // this one is on heap
	if (pSelectedFilesArray != NULL) // whm 11Jun12 added NULL test
		delete pSelectedFilesArray; // don't leak it
	if (pSelectedFoldersArray != NULL) // whm 11Jun12 added NULL test
		delete pSelectedFoldersArray; // don't leak it

	pPaneSelectedFiles->Clear();
	pPaneSelectedFolders->Clear();

	// update the pane's list
	if (sideWithFocus == leftSideHasFocus)
		SetupLeftList(pathToPane);
	else if (sideWithFocus == rightSideHasFocus)
		SetupRightList(pathToPane);
	
	// now defocus both lists & get the buttons disabled
	SetNeitherSideHasFocus();
	EnableButtons();
}

// BEW 28July10, changed the code to reflect the possibility of deleting from either pane,
// not just the right hand pane
void AdminMoveOrCopy::RemoveFilesAndFolders(wxString theFolderPath, 
				wxArrayString* pSelectedFoldersArray, 
				wxArrayString* pSelectedFilesArray)
{
	// copy the files first, then recurse for copying the one or more folders, if any
	size_t limitSelectedFiles = pSelectedFilesArray->GetCount();
	size_t limitSelectedFolders = pSelectedFoldersArray->GetCount();
	bool bRemovedOK = TRUE;

	// the passed in folder may have no files in it - check, and only do this
	// block if there is at least one file to be removed
	size_t nItemIndex = 0;
	if (limitSelectedFiles > 0)
	{
		wxString aFilename = pSelectedFilesArray->Item(nItemIndex);
		wxASSERT(!aFilename.IsEmpty());
		// loop across all files that were selected, deleting each as we go
		do {
			// do the removal of the file (an internal failure will not just return FALSE,
			// but will set the member boolean m_bUserCancelled to TRUE)
			bRemovedOK = RemoveSingleFile(theFolderPath, aFilename);
//			wxLogDebug(_T("\nRemoved  %s  ,  folder path = %s "),aFilename, theFolderPath);
			if (!bRemovedOK)
			{
				// if the removal did not succeed we want to halt proceedings ( a warning
				// will have been shown by RemoveSingleFile already) - use the
				// m_bUserCancelled mechanism, but don't clobber the AdminMoveOrCopy
				// dialog itself
				if (m_bUserCancelled)
				{
					// force return, and premature exit of loop
					return;
				}
			}

			// prepare for iteration, a new value for aFilename is required
			nItemIndex++;
			if (nItemIndex < limitSelectedFiles)
			{
					aFilename = pSelectedFilesArray->Item(nItemIndex);
//				wxLogDebug(_T("Prepare for iteration: next filename =  %s"),aFilename);
			}
		} while (nItemIndex < limitSelectedFiles);
	}

	// now handle any folder selections, these will be recursive calls within the loop
	if (limitSelectedFolders > 0)
	{
		// there are one or more folders to remove -- do it in a loop
		size_t indexForLoop;
		// loop over the selected directory names
		for (indexForLoop = 0; indexForLoop < limitSelectedFolders; indexForLoop++)
		{
			wxString theFolderPath2;
			wxArrayString* pSelectedFoldersArray2 = new wxArrayString;
			wxArrayString* pSelectedFilesArray2 = new wxArrayString;
			wxString aFoldername = pSelectedFoldersArray->Item(indexForLoop); // get next directory name
			theFolderPath2 = theFolderPath + gpApp->PathSeparator + aFoldername; // we know this folder exists
			// Now we must call GetFilesOnly() and GetFoldersOnly() in order to gather the
			// child directory's folder names and file names into the the arrays provided
			// (Each call internally resets the working directory.) First TRUE is bool
			// bSort, second TRUE is bool bSuppressMessage.
			bool bHasFolders = GetFoldersOnly(theFolderPath2, pSelectedFoldersArray2, TRUE, TRUE);
			bool bHasFiles = GetFilesOnly(theFolderPath2, pSelectedFilesArray2, TRUE, TRUE);
			bHasFolders = bHasFolders; // avoid compiler warning
			bHasFiles = bHasFiles; // avoid compiler warning

			// Reenter
//			wxLogDebug(_T("Re-entering: Path = %s, hasFolders %d , hasFiles %d"),
//				theFolderPath2, (int)bHasFolders, (int)bHasFiles);
			RemoveFilesAndFolders(theFolderPath2, pSelectedFoldersArray2, pSelectedFilesArray2);

			// on the return of the above call theFolderPath2 will have been emptied of
			// both files and folders, so remove it; first reset the working directory to a
			// directory not to be removed, and the removals will work
			bool bOK;
			if (sideWithFocus == leftSideHasFocus)
				bOK = ::wxSetWorkingDirectory(m_strLeftFolderPath); // the top level path (to left pane)
			else
				bOK = ::wxSetWorkingDirectory(m_strRightFolderPath); // the top level path (to right pane)
			bOK = ::wxRmdir(theFolderPath2);
			if (!bOK)
			{
				wxString msg;
				msg = msg.Format(_("Failed to remove the directory %s. \nPossibly it contains a hidden file that should not be removed. \nIf so, the rest of the contents may still have been removed."),theFolderPath2.c_str());
				wxMessageBox(msg,_("Could not remove directory"),wxICON_EXCLAMATION | wxOK);
				m_bUserCancelled = TRUE;
			}
//			else
//			{
//				wxLogDebug(_T("Removed directory  %s  "),theFolderPath2);
//			}
			// clean up for this iteration
			pSelectedFilesArray2->Clear();
			pSelectedFoldersArray2->Clear();
			if (pSelectedFilesArray2 != NULL) // whm 11Jun12 added NULL test
				delete pSelectedFilesArray2; // don't leak it
			if (pSelectedFoldersArray2 != NULL) // whm 11Jun12 added NULL test
				delete pSelectedFoldersArray2; // don't leak it

			// if there was a failure...
			if (m_bUserCancelled)
			{
				// exit loop immediately
				return;
			}
		} // end block for the for loop iterating over selected foldernames
	}
}

// handler for button to rename a single file or a single folder, in either pane
void AdminMoveOrCopy::OnBnClickedRename(wxCommandEvent& WXUNUSED(event))
{
	bool bOK = TRUE;
	// set these three with a switch
	wxArrayString* pPaneSelectedFolders = NULL;
	wxArrayString* pPaneSelectedFiles = NULL;
	wxString pathToPane;
	switch (sideWithFocus)
	{ 
		case leftSideHasFocus:
			{
				pathToPane = m_strLeftFolderPath;
				pPaneSelectedFolders = &leftSelectedFoldersArray;
				pPaneSelectedFiles = &leftSelectedFilesArray;

			} // end of case for left side having focus
			break;
		case rightSideHasFocus:
			{
				pathToPane = m_strRightFolderPath;
				pPaneSelectedFolders = &rightSelectedFoldersArray;
				pPaneSelectedFiles = &rightSelectedFilesArray;
			} // end of case for right side having focus
			break;
		case neitherSideHasFocus:
		default:
			{	
				wxASSERT(FALSE);
				wxBell();
				SetNeitherSideHasFocus();
				EnableButtons();
				return;
			}
	} // end of switch

	if (pPaneSelectedFolders->GetCount() == 1)
	{
		wxString theFolderPath = pathToPane + gpApp->PathSeparator;
		wxString oldName = pPaneSelectedFolders->Item(0); // old folder name
		theFolderPath += oldName; // path to selected folder which we want to rename

		// now get the user's wanted new directory name
		wxString msg = _("Type the new folder name");
		wxString caption = _("Type Name For Folder (spaces permitted)");
		wxString newName = ::wxGetTextFromUser(msg,caption,oldName,this); // use selection as default name
		if (newName.IsEmpty())
		{
			// the user cancelled, so return silently
			// update the pane's list
			if (sideWithFocus == leftSideHasFocus)
			{
				SetupLeftList(pathToPane);
			}
			else
			{
				SetupRightList(pathToPane);
			}
			SetNeitherSideHasFocus();
			EnableButtons();
			return; 
		}

		// Change to new name? -- use ::wxRenameFile(), but not on open
		// or working directory, and the overwrite parameter must be set TRUE
		bOK = ::wxSetWorkingDirectory(m_strRightFolderPath); // make parent directory be the working one
		wxString theNewPath = pathToPane + gpApp->PathSeparator + newName;
		bOK = ::wxRenameFile(theFolderPath,theNewPath,TRUE);
		// if the rename failed, return -- the rename failure will be visible as the folder
		// will just not be renamed
		if (!bOK)
		{
			wxBell();
			// update the pane's list
			if (sideWithFocus == leftSideHasFocus)
			{
				SetupLeftList(pathToPane);
			}
			else
			{
				SetupRightList(pathToPane);
			}
			SetNeitherSideHasFocus();
			EnableButtons();
			return;
		}
		gpApp->LogUserAction(_T("Renamed File"));

		// update the pane's list
		if (sideWithFocus == leftSideHasFocus)
		{
			SetupLeftList(pathToPane);
		}
		else
		{
			SetupRightList(pathToPane);
		}
		// if the panes have the same folder open, update the other as well
		if (m_strRightFolderPath == m_strLeftFolderPath)
		{
			if (sideWithFocus == leftSideHasFocus)
			{
				::wxSetWorkingDirectory(m_strRightFolderPath);
				SetupRightList(m_strRightFolderPath);
			}
			else
			{
				::wxSetWorkingDirectory(m_strLeftFolderPath);
				SetupLeftList(m_strLeftFolderPath);
			}
		}
	}
	else if (pPaneSelectedFiles->GetCount() == 1)
	{
		// renaming a file
		wxString theFilePath = pathToPane + gpApp->PathSeparator;
		wxString oldName = pPaneSelectedFiles->Item(0); // old file name
		theFilePath += oldName; // path to selected file which we want to rename

		// now get the user's wanted new filename
		wxString msg = _("Type the new file name");
		wxString caption = _("Type Name For File (spaces permitted)");
		wxString newName = ::wxGetTextFromUser(msg,caption,oldName,this); // use selection as default name
		if (newName.IsEmpty())
		{
			// the user cancelled, so return silently
			// update the pane's list
			if (sideWithFocus == leftSideHasFocus)
			{
				SetupLeftList(pathToPane);
			}
			else
			{
				SetupRightList(pathToPane);
			}
			SetNeitherSideHasFocus();
			EnableButtons();
			return; 
		}

		// Change to new name? -- use ::wxRenameFile(), but not on open
		// or working directory, and the overwrite parameter must be set TRUE
		bOK = ::wxSetWorkingDirectory(pathToPane); // make parent directory be the working one
		wxString theNewPath = pathToPane + gpApp->PathSeparator + newName;
		bOK = ::wxRenameFile(theFilePath,theNewPath,TRUE);
		// if the rename failed, return -- the rename failure will be visible as the folder
		// will just not be renamed
		if (!bOK)
		{
			wxBell();
			// update the pane's list
			if (sideWithFocus == leftSideHasFocus)
			{
				SetupLeftList(pathToPane);
			}
			else
			{
				SetupRightList(pathToPane);
			}
			SetNeitherSideHasFocus();
			EnableButtons();
			return;
		}

		gpApp->LogUserAction(_T("Renamed File"));
		
		// update the pane's list
		if (sideWithFocus == leftSideHasFocus)
		{
			SetupLeftList(pathToPane);
		}
		else
		{
			SetupRightList(pathToPane);
		}
		// if the panes have the same folder open, update the other as well
		if (m_strRightFolderPath == m_strLeftFolderPath)
		{
			if (sideWithFocus == leftSideHasFocus)
			{
				::wxSetWorkingDirectory(m_strRightFolderPath);
				SetupRightList(m_strRightFolderPath);
			}
			else
			{
				::wxSetWorkingDirectory(m_strLeftFolderPath);
				SetupLeftList(m_strLeftFolderPath);
			}
		}

//		SetupRightList(m_strRightFolderPath);
//		if (m_strRightFolderPath == m_strLeftFolderPath)
//		{
//			::wxSetWorkingDirectory(m_strLeftFolderPath);
//			SetupLeftList(m_strLeftFolderPath);
//		}
	}
	else
	{
		::wxBell();
	}
	SetNeitherSideHasFocus();
	EnableButtons();
}

void AdminMoveOrCopy::PutUpInvalidsMessage(wxString& strAllInvalids)
{
	wxString msg;
	msg = msg.Format(_(
"One or more files selected for copying to the __SOURCE_INPUTS folder were not copied.\nThey were: %s"),
	strAllInvalids.c_str());
	wxMessageBox(msg, _("Detected files not suitable for creating adaptation documents"),
	wxICON_EXCLAMATION | wxOK);
}

// The MoveOrCopyFilesAndFolders() function is used in both the Move and the Copy button
// handlers for file an/or folder moves, or file and/or folder copying. Moving is done by
// copying, and then removing from the source folder the original files which were copied.
// Copying does not do the removal step. 
// BEW 15July10, added 5th parameter, bool bToSourceDataFolder (default FALSE), because we
// want to do file validity checking if we are moving or copying to the __SOURCE_INPUTS folder,
// and reject any invalid ones; and also to test for an attempt to Move files to the
// "__SOURCE_INPUTS" folder - which we don't allow, and to warn when that happens and exit
// prematurely from the function before anything is done
// BEW 30July10, this function is not re-entrant when we are copying to the "__SOURCE_INPUTS"
// folder, even if the administrator selected folders to be copied. The reason for this is
// that the __SOURCE_INPUTS folder has to be a monocline list of just files, and so we only
// permit copying files to there. The administrator can, however, manually enter a
// succession of folders and move their files to the __SOURCE_INPUTS folder legally.
void AdminMoveOrCopy::MoveOrCopyFilesAndFolders(wxString srcFolderPath, wxString destFolderPath,
				wxArrayString* pSrcSelectedFoldersArray, wxArrayString* pSrcSelectedFilesArray, 
				bool bToSourceDataFolder, bool bDoMove)
{
	// copy the files first, then recurse for copying the one or more folders, if any
	size_t limitSelectedFiles = pSrcSelectedFilesArray->GetCount();
	size_t limitSelectedFolders = pSrcSelectedFoldersArray->GetCount();
	size_t index;
	size_t nItemIndex = 0;
	bool bCopyWasSuccessful = TRUE;
	arrCopiedOK.Clear();

    // when the "to" folder is the one named "__SOURCE_INPUTS", do not permit any Move request
    // - we want to ensure some other app's data only gets copied, not moved
	if (bDoMove && bToSourceDataFolder)
	{
		wxMessageBox(_(
"Moving files to the __SOURCE_INPUTS folder is not permitted.\nYou are allowed to copy them to that folder, so use the Copy button instead."),
		_("Only copy files to __SOURCE_INPUTS folder"), wxICON_INFORMATION | wxOK);
		return;
	}

	// prepare for the possibility that we are copying to the __SOURCE_INPUTS folder (when
	// doing that, this function is not re-entrant)
	bool bInvalidFiles = FALSE;
	wxString strAllInvalids; // where we build up a space delimited string of filenames
		// for any files which IsLoadableFile() says is not a suitable file for creating
		// and Adapt It document from. [Hopefully, if the administrator has done his job
		// right, he'll not put any such files in the __SOURCE_INPUTS folder in the first
		// place, but we can't rule out inappropriate use of a 3rd party app like Win
		// Explorer, which could move anything there (including folders)]

    // the passed in source folder may have no selected files in it - check, and only do
	// this block if there is at least one file to be copied (folder moves are handled
	// further below)
	if (limitSelectedFiles > 0)
	{
		arrCopiedOK.Alloc(limitSelectedFiles); // space for a flag for every 
											   // selected filename to have a flag value
		wxString aFilename = pSrcSelectedFilesArray->Item(nItemIndex);
		wxASSERT(!aFilename.IsEmpty());
		// loop across all files that were selected
		do {
            // BEW 15July10, add test for file contents validity (ie. loadable for creating
            // a document) if the __SOURCE_INPUTS folder is the place to copy to; construct a
            // string listing the invalid ones in sequence, set the bInvalidFiles boolean,
            // and after this do loop ends, check the boolean and inform the user of which
            // ones were invalid and so did not get copied
			if (bToSourceDataFolder)
			{
				wxString aPath = srcFolderPath + gpApp->PathSeparator + aFilename;
				if (!IsLoadableFile(aPath))
				{
					// can't create an adaptation doc from a file of this type, so don't
					// copy it to the __SOURCE_INPUTS folder, etc
					bInvalidFiles = TRUE;
					if (strAllInvalids.IsEmpty())
					{
						strAllInvalids = aFilename;
					}
					else
					{
						strAllInvalids += _T("  ") + aFilename;
					}
					arrCopiedOK.Add(0); // record the fact that this file copy did not take place
					// prepare for next iteration
					nItemIndex++;
					if (nItemIndex < limitSelectedFiles)
					{
						aFilename = pSrcSelectedFilesArray->Item(nItemIndex);
					}
					continue; // skip the CopySingleFile() call below
				}
			} // end of TRUE block for test: if (bToSourceDataFolder)

			// process one file per iteration until the list of selected filenames is empty
			bCopyWasSuccessful = CopySingleFile(srcFolderPath, destFolderPath, 
												aFilename, m_bUserCancelled);
//			wxLogDebug(_T("\nCopied  %s  ,  folder path = %s "),aFilename, pathToSourcePane);
			if (!bCopyWasSuccessful)
			{
				// if the copy did not succeed because the user chose to Cancel when a filename
				// conflict was detected, we want to use the returned TRUE value in
				// m_bUserCancelled to force the parent dialog to close; other failures, however,
				// need a warning to be given to the user
				arrCopiedOK.Add(0); // record the fact that the last file copy did not take place
				if (m_bUserCancelled)
				{
					// put up the message showing any invalid files (their names & extensions)
					// encountered up to this point
					if (bToSourceDataFolder && bInvalidFiles)
					{
						PutUpInvalidsMessage(strAllInvalids);
					}
					// force parent dialog to close, and if Move was being attempted, abandon that
					// to without removing anything more at the source pane's side
					return;
				}
			}
			else
			{
				arrCopiedOK.Add(1); // record the fact that the copy took place
			}

			// prepare for iteration, a new value for aFilename is required
			nItemIndex++;
			if (nItemIndex < limitSelectedFiles)
			{
				aFilename = pSrcSelectedFilesArray->Item(nItemIndex);
//				wxLogDebug(_T("Prepare for iteration: next filename =  %s"),aFilename);
			}
		} while (nItemIndex < limitSelectedFiles);
	}

	// tell the user if some files were not copied to __SOURCE_INPUTS folder, and which they are
	if (bToSourceDataFolder && bInvalidFiles)
	{
		PutUpInvalidsMessage(strAllInvalids);
	}

	// If Move was requested, remove each source filename for which there was a successful
	// copy from the pPaneSelectedFiles array, and then clear the pPaneSelectedFiles array. 
	// Since this is Move, remove those same files from the source pane's list
	if (bDoMove && limitSelectedFiles > 0 && !bToSourceDataFolder)
	{
		int flag; // will be 1 or 0, for each selected filename
		wxString aFilename;
		wxString theSourceFilePath;
		bool bRemovedSuccessfully = TRUE;
		for (index = 0; index < limitSelectedFiles; index++)
		{
			flag = arrCopiedOK.Item(index); // get whether or not it was successfully copied
			if (flag)
			{
				aFilename = pSrcSelectedFilesArray->Item(index); // get the filename
				// make the path to it
				theSourceFilePath = srcFolderPath + gpApp->PathSeparator + aFilename;

				// remove the file from the source pane's displayed folder contents
				bRemovedSuccessfully = ::wxRemoveFile(theSourceFilePath);
				if (!bRemovedSuccessfully)
				{
					// shouldn't ever fail, so an English message for developer will do
					wxString msg;
					msg = msg.Format(_T("MoveOrCopyFiles: Removing the file with path: %s  for the requested move, failed"),
							theSourceFilePath.c_str());
					wxMessageBox(msg, _T("Failed to remove from source folder a moved file"),wxICON_EXCLAMATION | wxOK);
				} // end block for test: if (!bRemovedSuccessfully)
//				else
//				{
//					wxLogDebug(_T("Removed file  %s  from path  %s"),aFilename.c_str(), theSourceFilePath.c_str());
//				}
			} // end block for test: if (flag)
		} // end for loop block
	}  // end block for test: if (bDoMove)

	// now handle any folder selections, these will be recursive calls within the loop
	// BEW 15July10, added test for supporting __SOURCE_INPUTS monocline list folder of
	// loadable source text files, and a message explaining to the user
	if (limitSelectedFolders > 0)
	{
        // test for right being the folder called __SOURCE_INPUTS, if so, we don't allow
        // copying folders & their file contents, as the __SOURCE_INPUTS contents has to be a
        // monocline list. So we force the user to instead manualaly open any folder with
        // source files for copying there and then he can select which files to copy.
		if (bToSourceDataFolder)
		{
            // the user has selected one or more folders to copy to __SOURCE_INPUTS, and this
            // is taboo, so tell him and return (this makes the function reentrant in this
            // circumstance, but only to a depth of one level as the return done here kills
            // subsequent re-entrancy)
			wxMessageBox(_(
"You are allowed only to copy files to the __SOURCE_INPUTS folder.\nThe folders you selected have not been copied.\nInstead, in the left pane, choose a folder and open it, select the files you want to copy, then click Copy."),
			_("Copy of folders to __SOURCE_INPUTS folder is not allowed"), wxICON_EXCLAMATION | wxOK);
			return;
		}
		else
		{
			// there are one or more folders to copy or move, and the folder to copy to is
			// not the __SOURCE_INPUTS folder. Do the directory plus files copy in a loop
			size_t indexForLoop;
			// loop over the selected directory names
			for (indexForLoop = 0; indexForLoop < limitSelectedFolders; indexForLoop++)
			{
				wxString srcFolderPath2;
				wxString destFolderPath2;
				wxArrayString* pSrcSelectedFoldersArray2 = new wxArrayString;
				wxArrayString* pSrcSelectedFilesArray2 = new wxArrayString;
				wxString aFoldername = pSrcSelectedFoldersArray->Item(indexForLoop); // get next directory name
				srcFolderPath2 = srcFolderPath + gpApp->PathSeparator + aFoldername; // we know this folder exists
				// the following folder may or may not exist
				destFolderPath2 = destFolderPath + gpApp->PathSeparator + aFoldername; 
				bool bDirExists = ::wxDirExists(destFolderPath2.c_str());
				bool bMadeOK = TRUE;
				if (!bDirExists)
				{
					// directory to copy to does not yet exist, so create it (note, on Linux,
					// or Mac/Unix, the permissions for this directory will be octal 0777,
					// specified by a default int perm = 0777 parameter in the call)
					bMadeOK = ::wxMkdir(destFolderPath2.c_str());

					// If we fail to make this directory, we cannot proceed with the copy or
					// move. The only bailout we support bails us out of the whole
					// AdminMoveOrCopy dialog, for a Cancel click when the user has a file
					// conflict to resolve. So we'll just use that, after giving the user a
					// warning message.
					if (!bMadeOK)
					{
						wxString msg;
						msg = msg.Format(
_("Failed to create the copy directory  %s  for the Copy or Move operation.\nThe parent dialog will now close.\nFiles and folders already copied or moved will remain so.\nYou may need to use your system's file browser to clean up before you try again."),
						destFolderPath2.c_str());
						wxMessageBox(msg, _("Error: could not make directory"), wxICON_EXCLAMATION | wxOK);
						m_bUserCancelled = TRUE; // causes call of EndModal() at top level call
						if (pSrcSelectedFoldersArray2 != NULL) // whm 11Jun12 added NULL test
							delete pSrcSelectedFoldersArray2; // don't leak memory
						if (pSrcSelectedFilesArray2 != NULL) // whm 11Jun12 added NULL test
							delete pSrcSelectedFilesArray2;   // ditto
						return;
					}
				}
				// The directory has now either been successfully made, or already exists.
				// Now we must call GetFilesOnly() and GetFoldersOnly() in order to gather the
				// source directory's folder names and file names into the the arrays provided
				// (Each call internally resets the working directory.) First TRUE is bool
				// bSort, second TRUE is bool bSuppressMessage.
				bool bHasFolders = GetFoldersOnly(srcFolderPath2, pSrcSelectedFoldersArray2, TRUE, TRUE);
				bool bHasFiles = GetFilesOnly(srcFolderPath2, pSrcSelectedFilesArray2, TRUE, TRUE);
				bHasFolders = bHasFolders; // avoid compiler warning
				bHasFiles = bHasFiles;     // avoid compiler warning

				// Reenter
	//			wxLogDebug(_T("Re-entering: sourcePath = %s , destinationPath = %s, hasFolders %d , hasFiles %d"),
	//				srcFolderPath2, destFolderPath2, (int)bHasFolders, (int)bHasFiles);
				MoveOrCopyFilesAndFolders(srcFolderPath2, destFolderPath2, pSrcSelectedFoldersArray2,
						pSrcSelectedFilesArray2, bToSourceDataFolder, bDoMove);

				// if Move was requested, the return of the above call will indicate that
				// srcFolderPath2 might have been emptied of both files and folders, so try remove the
				// possibly empty src folder -- if will be removed only if empty
				if (bDoMove)
				{
					// the directory within srcFolderPath will be currently set as the working
					// directory, and that prevents ::wxRmdir() from removing it, so first
					// reset the working directory to its parent, and the removals will work
					bool bOK = ::wxSetWorkingDirectory(srcFolderPath);
					bOK = ::wxRmdir(srcFolderPath2);
					// ::wxRmdir() will fail if it still contains a file or folder - this can
					// happen if there was a file conflict and the user elected to do noCopy,
					// which means the conflicting file remains in the left directory. We
					// don't want to have an annoying message put up for each such 'failure'
					// so just ignore the return value bOK
					bOK = bOK; // avoid warning
					/* // uncomment out if message and/or debug logging is wanted
					if (!bOK)
					{
						wxString msg;
						msg = msg.Format(_T("::Rmdir() failed to remove directory: %s "),srcFolderPath2.c_str());
						wxMessageBox(msg,_T("Couldn't remove directory"),wxICON_EXCLAMATION | wxOK);
					}
					else
					{
						wxLogDebug(_T("Removed directory  %s  "),srcFolderPath2);
					}
					*/
				}
				// clean up for this iteration
				pSrcSelectedFilesArray2->Clear();
				pSrcSelectedFoldersArray2->Clear();
				if (pSrcSelectedFilesArray2 != NULL) // whm 11Jun12 added NULL test
					delete pSrcSelectedFilesArray2;   // don't leak it
				if (pSrcSelectedFoldersArray2 != NULL) // whm 11Jun12 added NULL test
					delete pSrcSelectedFoldersArray2; // don't leak it

				// if the user asked for a cancel from the file conflict dialog, it halts
				// recursion and cancels the AdminMoveOrCopy dialog
				if (m_bUserCancelled)
				{
					// halt iterating over foldernames, prematurely return to caller, and top
					// level call will call EndModal()
					return;
				}
			} // end block for the for loop iterating over selected foldernames
		} // end of else block for test: 	if (bToSourceDataFolder)

	} //end of TRUE block for test:	if (limitSelectedFolders > 0)

	// uncomment out next lines if debug logging is wanted
	//unsigned int i;
	//for (i=0; i < *pSrcSelectedFilesArray.GetCount(); i++)
	//{
	//	wxString fn = *pSrcSelectedFilesArray.Item(i);
	//	wxLogDebug(_T("File:  %s   at index:  %d  for total of %d\n"), fn.c_str(),i, *pSrcSelectedFilesArray.GetCount());
	//}
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxCommandEvent fired by the user's button press
/// \remarks
///  Handler for the "Copy" button. It works out which is the source pane (the one to copy
///  from) and copies the one or more selected files and / or one or more selected folders
///  in the source pane to the folder which is the 'other' pane, the destination pane. It
///  recursively copies the files and folders of any child folders. File name conflicts are
///  handled internally, similarly to how Windows Explorer handles them (i.e. with a child
///  dialog opening to display file attribute details such as name, size and last
///  modification date & time for each of the files in conflict) and the user has 3 options
///  - continue, overwriting the file at the copy folder with the one from the other
///  folder; or to not copy; or to have the name of the other folder's file changed so as
///  to remove the conflict and then the copy goes ahead - resulting in two files with same
///  or similar data being in the copy folder. Cancelling, and opting to have all
///  subsequent filename conflicts handled the same way as the current one are also
///  supported from the child dialog.
////////////////////////////////////////////////////////////////////////////////////////////
void AdminMoveOrCopy::OnBnClickedCopy(wxCommandEvent& WXUNUSED(event))
{
	// set these four with a switch
	wxArrayString* pPaneSelectedFolders = NULL;
	wxArrayString* pPaneSelectedFiles = NULL;
	wxString pathToSourcePane;
	wxString pathToDestinationPane;

	switch (sideWithFocus)
	{ 
		case leftSideHasFocus:
			{
				pathToSourcePane = m_strLeftFolderPath;
				pathToDestinationPane = m_strRightFolderPath;
				pPaneSelectedFolders = &leftSelectedFoldersArray;
				pPaneSelectedFiles = &leftSelectedFilesArray;

			} // end of case for left side having focus
			break;
		case rightSideHasFocus:
			{
				pathToSourcePane = m_strRightFolderPath;
				pathToDestinationPane = m_strLeftFolderPath;
				pPaneSelectedFolders = &rightSelectedFoldersArray;
				pPaneSelectedFiles = &rightSelectedFilesArray;
			} // end of case for right side having focus
			break;
		case neitherSideHasFocus:
		default:
			{
				wxASSERT(FALSE);
				wxBell();
				SetNeitherSideHasFocus();
				EnableButtons();
				return;
			}
	} // end of switch

	// do nothing if the source folder is not yet defined, or the destination folder
	if (pathToSourcePane.IsEmpty())
	{
		wxMessageBox(
_("No source folder is defined. (The source folder is where your selections are made.)\nUse the appropriate 'Locate the folder' button to first open a source folder, then try again."),
		_("Cannot copy"), wxICON_EXCLAMATION | wxOK);
		SetNeitherSideHasFocus();
		EnableButtons();
		return;
	}
	if (pathToDestinationPane.IsEmpty())
	{
		wxMessageBox(
_("No destination folder is defined. (The destination folder is where your selections will be copied to.)\nUse the appropriate 'Locate the folder' button to first open a destination folder, then try again."),
		_("Cannot copy"), wxICON_EXCLAMATION | wxOK);
		SetNeitherSideHasFocus();
		EnableButtons();
		return;
	}
	if(CheckForIdenticalPaths(m_strLeftFolderPath, m_strRightFolderPath))
	{
		// identical paths, so bail out; CheckForIdenticalPaths() has put up a warning
		// message
		SetNeitherSideHasFocus();
		EnableButtons();
		return;
	}

	// on the heap, create the arrays of files and folders which are to be copied
	// & populate them 
	wxArrayString* pSrcSelectedFoldersArray	= new wxArrayString;
	wxArrayString* pSrcSelectedFilesArray	= new wxArrayString;
	size_t index;
	size_t foldersLimit = pPaneSelectedFolders->GetCount(); // populated in SetSelectionArray() call earlier
	size_t filesLimit = pPaneSelectedFiles->GetCount(); // populated in SetSelectionArray() call earlier
	if (filesLimit == 0 && foldersLimit == 0)
	{
		wxMessageBox(
_("Before you click the Copy button, you need to select at least one item from the list in whichever pane you want to copy data from."),
		_("No Files Or Folders Selected"),wxICON_EXCLAMATION | wxOK);
		pSrcSelectedFoldersArray->Clear(); // this one is on heap
		pSrcSelectedFilesArray->Clear(); // this one is on heap
		if (pSrcSelectedFilesArray != NULL) // whm 11Jun12 added NULL test
			delete pSrcSelectedFilesArray;   // don't leak it
		if (pSrcSelectedFoldersArray != NULL) // whm 11Jun12 added NULL test
			delete pSrcSelectedFoldersArray; // don't leak it
		SetNeitherSideHasFocus();
		EnableButtons();
		return;
	}
	for (index = 0; index < foldersLimit; index++)
	{
		pSrcSelectedFoldersArray->Add(pPaneSelectedFolders->Item(index));
	}
	for (index = 0; index < filesLimit; index++)
	{
		pSrcSelectedFilesArray->Add(pPaneSelectedFiles->Item(index));
	}

 	// initialize bail out flag, and boolean for doing it the same way henceforth,
	// and set the default lastWay value to noCopy
	m_bUserCancelled = FALSE;
	m_bDoTheSameWay = FALSE;
	lastWay = noCopy; // a safe default (for whatever way the last filename conflict, 
					  // if there was one, was handled (see CopyAction enum for values)

    // do not allow copying of whole folders if the copy folder is "__SOURCE_INPUTS" (because
    // we must have only a flat list of filenames in __SOURCE_INPUTS); and for a Move request,
    // with copy folder being the "__SOURCE_INPUTS", we do not allow anything to be moved, (so
    // as to protect source data files which may belong to a 3rd part app,) we allow only
    // copying. This protocol is implemented with the call to MoveOrCopyFilesAndFolders() -
    // OnBnClickedCopy() and OnBnClickedMove() just pass in the booleans so it can work out
    // what to do and what to tell the administrator
	bool bCopyingToSourceDataFolder = FALSE;
	if (pathToDestinationPane == gpApp->m_sourceInputsFolderPath)
	{
		bCopyingToSourceDataFolder = TRUE;
	}
	// do the copy
	if (bCopyingToSourceDataFolder)
	{
		// 5th param, bToSourceDataFolder is TRUE, final param (bDoMove) is FALSE
		MoveOrCopyFilesAndFolders(pathToSourcePane, pathToDestinationPane, pSrcSelectedFoldersArray,
					pSrcSelectedFilesArray, TRUE, FALSE); 
	}
	else
	{
		// 5th param, bToSourceDataFolder is FALSE, final param (bDoMove) is FALSE
		MoveOrCopyFilesAndFolders(pathToSourcePane, pathToDestinationPane, pSrcSelectedFoldersArray,
					pSrcSelectedFilesArray, FALSE, FALSE);
	}

	// clear the allocations to the heap
	pSrcSelectedFilesArray->Clear(); // this one is on heap
	pSrcSelectedFoldersArray->Clear(); // this one is on heap
	if (pSrcSelectedFilesArray != NULL) // whm 11Jun12 added NULL test
		delete pSrcSelectedFilesArray; // don't leak it
	if (pSrcSelectedFoldersArray != NULL) // whm 11Jun12 added NULL test
		delete pSrcSelectedFoldersArray; // don't leak it

	gpApp->LogUserAction(_T("Copied File(s)"));

	// if the user asked for a cancel from the file conflict dialog, it cancels the parent
	// dlg too
	if (m_bUserCancelled)
	{
		// force parent dialog to close, and if Move was being attempted, abandon that
		// too without removing anything more at the source side
		EndModal(wxID_OK);
		// get the lists to agree with the state of affairs at cancel time
		SetupLeftList(m_strLeftFolderPath);
		SetupRightList(m_strRightFolderPath);
		SetNeitherSideHasFocus();
		EnableButtons();
		return;
	}

    // Note: a bug in SetItemState() which DeselectSelectedItems() call uses causes the
    // m_nCount member of wxArrayString for pPaneSelectedFiles() array to be overwritten to
    // have value 1, so after emptying here do a further empty below
	pPaneSelectedFiles->Clear(); // the member wxArrayString variable, for top level
	if (sideWithFocus == leftSideHasFocus)
	{
		DeselectSelectedItems(leftSide); // this loops through wxListCtrl clearing the
										 // selection for each item which has it set
	}
	else
	{
		DeselectSelectedItems(rightSide);
	}

    // clear the source pane's array of filename selections - put it here because of a
    // wxWidgets bug where the SetItemState() call implicit in DeselectSelectedItems()
    // causes the emptied pPaneSelectedFiles array to become non-empty (its m_nCount
    // becomes 1, even though the AdminMoveOrCopy class and its members is on main thread
    // and the wxListCtrl, or wxListView class, is on a different work thread). I could
    // just do a unilateral call above of SetupLeftList(), or SetupRightList as the case
    // may be, to get round this problem, but that would be using a cannon to shoot a fly
	pPaneSelectedFiles->Clear(); // DO NOT DELETE THIS CALL HERE (even though it's above)

	// update the destination pane to reflect its copied contents, & the source pane too 
	SetupRightList(m_strRightFolderPath);
	SetupLeftList(m_strLeftFolderPath);
	SetNeitherSideHasFocus();
	EnableButtons();
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxCommandEvent fired by the user's button press
/// \remarks
///  Handler for the "Move" button. It works as described in the header comments for the
///  Copy button, but with the extra step at the end of deleting from the source folder
///  the files and / or folders copied to the destination folder.
////////////////////////////////////////////////////////////////////////////////////////////
void AdminMoveOrCopy::OnBnClickedMove(wxCommandEvent& WXUNUSED(event))
{
	// set these four with a switch
	wxArrayString* pPaneSelectedFolders = NULL;
	wxArrayString* pPaneSelectedFiles = NULL;
	wxString pathToSourcePane;
	wxString pathToDestinationPane;

	switch (sideWithFocus)
	{ 
		case leftSideHasFocus:
			{
				pathToSourcePane = m_strLeftFolderPath;
				pathToDestinationPane = m_strRightFolderPath;
				pPaneSelectedFolders = &leftSelectedFoldersArray;
				pPaneSelectedFiles = &leftSelectedFilesArray;

			} // end of case for left side having focus
			break;
		case rightSideHasFocus:
			{
				pathToSourcePane = m_strRightFolderPath;
				pathToDestinationPane = m_strLeftFolderPath;
				pPaneSelectedFolders = &rightSelectedFoldersArray;
				pPaneSelectedFiles = &rightSelectedFilesArray;
			} // end of case for right side having focus
			break;
		case neitherSideHasFocus:
		default:
			{
				wxASSERT(FALSE);
				wxBell();
				SetNeitherSideHasFocus();
				EnableButtons();
				return;
			}
	} // end of switch

	// do nothing if the source folder is not yet defined, or the destination folder
	if (pathToSourcePane.IsEmpty())
	{
		wxMessageBox(
_("No source folder is defined. (The source folder is where your selections are made.)\nUse the appropriate 'Locate the folder' button to first open a source folder, then try again."),
		_("Cannot move"), wxICON_EXCLAMATION | wxOK);
		SetNeitherSideHasFocus();
		EnableButtons();
		return;
	}
	if (pathToDestinationPane.IsEmpty())
	{
		wxMessageBox(
_("No destination folder is defined. (The destination folder is where your selections will be moved to.)\nUse the appropriate 'Locate the folder' button to first open a destination folder, then try again."),
		_("Cannot move"), wxICON_EXCLAMATION | wxOK);
		SetNeitherSideHasFocus();
		EnableButtons();
		return;
	}
	if(CheckForIdenticalPaths(m_strLeftFolderPath, m_strRightFolderPath))
	{
		// identical paths, so bail out; CheckForIdenticalPaths() has put up a warning
		// message
		SetNeitherSideHasFocus();
		EnableButtons();
		return;
	}

    // Do not allow moving of a selected "__SOURCE_INPUTS" folder. Not only would its contents
    // be inappropriate in some other place (such as a diffent project folder), but the
    // move would not work right, for as soon as the "__SOURCE_INPUTS" folder was opened or
    // created at the destination folder, the attempt to move files into it would trip the
    // test against doing this within MoveOrCopyFilesAndFolders() itself, resulting in a
    // warning message and nothing being copied. So it is better to here suppress the move
    // attempt before the __SOURCE_INPUTS folder gets moved or created in the other location
	if (pPaneSelectedFolders->GetCount() > 0)
	{
		bool bItsPresent = SelectedFoldersContainSourceDataFolder(pPaneSelectedFolders);
		if (bItsPresent)
		{
			// abort the Move
			wxMessageBox(
_("Trying to move the '__SOURCE_INPUTS' folder is not permitted.\nThe Move operation is cancelled. Nothing has been moved."),
			_("Illegal folder move attempt"),wxICON_EXCLAMATION | wxOK);
			pPaneSelectedFiles->Clear();
			pPaneSelectedFolders->Clear();
			if (sideWithFocus == leftSideHasFocus)
			{
				DeselectSelectedItems(leftSide);
			}
			else
			{
				DeselectSelectedItems(rightSide);
			}
			SetupRightList(m_strRightFolderPath);
			SetupLeftList(m_strLeftFolderPath);
			SetNeitherSideHasFocus();
			EnableButtons();
			return;
		}
	}

	// on the heap, create the arrays of files and folders which are to be moved
	// & populate them 
	wxArrayString* pSrcSelectedFoldersArray	= new wxArrayString;
	wxArrayString* pSrcSelectedFilesArray	= new wxArrayString;
	size_t index;
	size_t foldersLimit = pPaneSelectedFolders->GetCount(); // populated in SetSelectionArray() call earlier
	size_t filesLimit = pPaneSelectedFiles->GetCount(); // populated in SetSelectionArray() call earlier
	if (filesLimit == 0 && foldersLimit == 0)
	{
		wxMessageBox(
_("Before you click the Move button, you need to select at least one item from the list in whichever pane you want to move data from."),
		_("No Files Or Folders Selected"),wxICON_EXCLAMATION | wxOK);
		pSrcSelectedFoldersArray->Clear(); // this one is on heap
		pSrcSelectedFilesArray->Clear(); // this one is on heap
		if (pSrcSelectedFilesArray != NULL) // whm 11Jun12 added NULL test
			delete pSrcSelectedFilesArray;   // don't leak it
		if (pSrcSelectedFoldersArray != NULL) // whm 11Jun12 added NULL test
			delete pSrcSelectedFoldersArray; // don't leak it
		SetNeitherSideHasFocus();
		EnableButtons();
		return;
	}
	for (index = 0; index < foldersLimit; index++)
	{
		pSrcSelectedFoldersArray->Add(pPaneSelectedFolders->Item(index));
	}
	for (index = 0; index < filesLimit; index++)
	{
		pSrcSelectedFilesArray->Add(pPaneSelectedFiles->Item(index));
	}

	// initialize bail out flag, and boolean for doing it the same way henceforth,
	// and set the default lastWay value to noCopy
	m_bUserCancelled = FALSE;
	m_bDoTheSameWay = FALSE;
	lastWay = noCopy; // a safe default (for whatever way the last filename conflict, 
					  // if there was one, was handled (see CopyAction enum for values)

	// do not allow copying of whole folders if the destination folder is "__SOURCE_INPUTS"
	// (because we must have only a flat list of filenames in __SOURCE_INPUTS); and for a
	// Move request, with destination folder "__SOURCE_INPUTS", we do not allow anything to be
	// moved, (so as to protect source data files which may belong to a 3rd part app,) we
	// allow only copying. This protocol is implemented with the call to
	// MoveOrCopyFilesAndFolders() - OnBnClickedCopy() and OnBnClickedMove() just pass in
	// the booleans so it can work out what to do and what to tell the administrator 
	bool bMovingToSourceDataFolder = FALSE;
	if (pathToDestinationPane == gpApp->m_sourceInputsFolderPath)
	{
		bMovingToSourceDataFolder = TRUE;
	}
	// do the move (the function will internally work out if it needs to block a move to
	// the "__SOURCE_INPUTS" folder, and exit early with an information message for the user)
	if (bMovingToSourceDataFolder)
	{
        // the 5th param is bool bToSourceDataFolder, which is here set TRUE; last (6th,
        // unseen default) param bool bDoMove is TRUE
		MoveOrCopyFilesAndFolders(pathToSourcePane, pathToDestinationPane, pSrcSelectedFoldersArray,
					pSrcSelectedFilesArray, TRUE); // 
	}
	else
	{
        // the 5th param is bool bToSourceDataFolder, which is here set FALSE; last (6th,
        // unseen default) param bool bDoMove is TRUE
		MoveOrCopyFilesAndFolders(pathToSourcePane, pathToDestinationPane, pSrcSelectedFoldersArray,
					pSrcSelectedFilesArray, FALSE);
	}

	// clear the allocations to the heap
	pSrcSelectedFilesArray->Clear(); // this one is on heap
	pSrcSelectedFoldersArray->Clear(); // this one is on heap
	if (pSrcSelectedFilesArray != NULL) // whm 11Jun12 added NULL test
		delete pSrcSelectedFilesArray; // don't leak it
	if (pSrcSelectedFoldersArray != NULL) // whm 11Jun12 added NULL test
		delete pSrcSelectedFoldersArray; // don't leak it

	// if the user asked for a cancel from the file conflict dialog, it cancels the parent
	// dlg too
	if (m_bUserCancelled)
	{
		// force parent dialog to close, and if Move was being attempted, abandon that
		// too without removing anything more at the source side
		EndModal(wxID_OK);
		// get the lists to agree with the state of affairs at cancel time
		SetupLeftList(m_strLeftFolderPath);
		SetupRightList(m_strRightFolderPath);
		SetNeitherSideHasFocus();
		EnableButtons();
		return;
	}

	gpApp->LogUserAction(_T("Moved file(s)"));

    // Note: a bug in SetItemState() which DeselectSelectedItems() call uses causes the
    // m_nCount member of wxArrayString for pPaneSelectedFiles() array to be overwritten to
    // have value 1, so after emptying here do a further empty below
	pPaneSelectedFiles->Clear(); // the member wxArrayString variable, for top level
    // this loops through wxListCtrl clearing the selection for each item which has it set
	if (sideWithFocus == leftSideHasFocus)
	{
		DeselectSelectedItems(leftSide); 
	}
	else
	{
		DeselectSelectedItems(rightSide);
	}

    // clear the source pane's array of filename selections - put it here because of a
    // wxWidgets bug where the SetItemState() call implicit in DeselectSelectedItems()
    // causes the emptied pPaneSelectedFiles array to become non-empty (its m_nCount
    // becomes 1, even though the AdminMoveOrCopy class and its members is on main thread
    // and the wxListCtrl, or wxListView class, is on a different work thread). I could
    // just do a unilateral call above of SetupLeftList(), or SetupRightList as the case
    // may be, to get round this problem, but that would be using a cannon to shoot a fly
	pPaneSelectedFiles->Clear(); // DO NOT DELETE THIS CALL HERE (even though it's above)

	// update the destination pane to reflect its moved contents, & the source pane too 
	SetupRightList(m_strRightFolderPath);
	SetupLeftList(m_strLeftFolderPath);
	SetNeitherSideHasFocus();
	EnableButtons();
}

void AdminMoveOrCopy::NoSelectionMessage()
{
	// nothing to Peek at, tell the user what to do
	wxString str = _(
"The Peek... button will show you up to the first 16 kB of whichever file you selected in either list.\nBut first, click on a file to select it, then click the Peek... button.");
	wxMessageBox(str,_("Peek needs a file selection"), wxICON_INFORMATION | wxOK);
}

// BEW added 14July10
// BEW 23July10, changed to allow Peek to work with either pane
void AdminMoveOrCopy::OnBnClickedPeek(wxCommandEvent& WXUNUSED(event))
{
	// determine where the focus is, and peek at the first selected file
	if (rightFilesCount == 0 && leftFilesCount == 0)
	{
		//nothing to peek at, so return
		wxBell();
		EnableButtons();
		return;
	}
	switch (sideWithFocus)
	{ 
	case leftSideHasFocus:
		{
			if (leftFilesCount > 0 && (pLeftList->GetFocusedItem() >= -1))
			{
				int count = leftSelectedFilesArray.GetCount();
				if (count == 0)
				{
					NoSelectionMessage();
					SetupLeftList(m_strLeftFolderPath);
					SetNeitherSideHasFocus();
					EnableButtons();
					return;
				}
				else
				{
					// we've at least one file we can Peek at -- get the first in the array
					wxString filename = leftSelectedFilesArray.Item(0);
					wxASSERT(!filename.IsEmpty());
					// make the path
					wxString path = m_strLeftFolderPath + gpApp->PathSeparator + filename;

					// set up the dialog
					CPeekAtFileDlg peeker(this); // AdminMoveOrCopy is the parent dialog for it
												 // and also its friend
					peeker.m_filePath = path; // transfer the path to the file to be peeked at
					wxASSERT(!peeker.m_filePath.IsEmpty());
					peeker.m_pEditCtrl->SetFocus();

					if (peeker.ShowModal() == wxID_OK) // don't need the test, but no harm in it
					{
					}
					SetupLeftList(m_strLeftFolderPath);
					
					gpApp->LogUserAction(_T("Peeked at File"));
	
					// now defocus both lists
					SetNeitherSideHasFocus();
					EnableButtons();
					return;
				}
			}
		}
		break;
	case rightSideHasFocus:
		{
			if (rightFilesCount > 0 && (pRightList->GetFocusedItem() >= -1))
			{
				int count = rightSelectedFilesArray.GetCount();
				if (count == 0)
				{
					// nothing to Peek at, tell the user what to do
					NoSelectionMessage();
					SetupRightList(m_strRightFolderPath);
					SetNeitherSideHasFocus();
					EnableButtons();
					return;
				}
				else
				{
					// we've at least one file we can Peek at -- get the first in the array
					wxString filename = rightSelectedFilesArray.Item(0);
					wxASSERT(!filename.IsEmpty());
					// make the path
					wxString path = m_strRightFolderPath + gpApp->PathSeparator + filename;

					// set up the dialog
					CPeekAtFileDlg peeker(this); // AdminMoveOrCopy is the parent dialog for it
												 // and also its friend
					peeker.m_filePath = path; // transfer the path to the file to be peeked at
					wxASSERT(!peeker.m_filePath.IsEmpty());
					peeker.m_pEditCtrl->SetFocus();

					if (peeker.ShowModal() == wxID_OK) // don't need the test, but no harm in it
					{
					}
					SetupRightList(m_strRightFolderPath);
					
					gpApp->LogUserAction(_T("Peeked at File"));
	
					// now defocus both lists
					SetNeitherSideHasFocus();
					EnableButtons();
					return;
				}
			}
		}
		break;
	case neitherSideHasFocus:
	default:
		{
			wxASSERT(FALSE);
			wxBell();
		}
		break;
	}

	// update the pane's list
	if (sideWithFocus == leftSideHasFocus)
		SetupLeftList(m_strLeftFolderPath);
	else if (sideWithFocus == rightSideHasFocus)
		SetupRightList(m_strRightFolderPath);
	
	// now defocus both lists
	SetNeitherSideHasFocus();
	EnableButtons();
}

