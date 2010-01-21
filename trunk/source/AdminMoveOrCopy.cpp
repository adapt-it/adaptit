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

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

extern bool gbDoingSplitOrJoin;

enum myicons {
	indxFolderIcon,
	indxFileIcon,
	indxEmptyIcon
};

// event handler table
BEGIN_EVENT_TABLE(AdminMoveOrCopy, AIModalDialog)

	EVT_INIT_DIALOG(AdminMoveOrCopy::InitDialog)
	EVT_BUTTON(wxID_OK, AdminMoveOrCopy::OnOK)

	EVT_BUTTON(ID_BUTTON_LOCATE_SOURCE_FOLDER, AdminMoveOrCopy::OnBnClickedLocateSrcFolder)	
	EVT_BUTTON(ID_BUTTON_LOCATE_DESTINATION_FOLDER, AdminMoveOrCopy::OnBnClickedLocateDestFolder)
	EVT_BUTTON(ID_BITMAPBUTTON_SRC_OPEN_FOLDER_UP, AdminMoveOrCopy::OnBnClickedSrcParentFolder)
	EVT_BUTTON(ID_BITMAPBUTTON_DEST_OPEN_FOLDER_UP, AdminMoveOrCopy::OnBnClickedDestParentFolder)

	EVT_SIZE(AdminMoveOrCopy::OnSize)

	EVT_LIST_ITEM_SELECTED(ID_LISTCTRL_SOURCE_CONTENTS, AdminMoveOrCopy::OnSrcListSelectItem)
	EVT_LIST_ITEM_DESELECTED(ID_LISTCTRL_SOURCE_CONTENTS, AdminMoveOrCopy::OnSrcListDeselectItem)
	EVT_LIST_ITEM_SELECTED(ID_LISTCTRL_DESTINATION_CONTENTS, AdminMoveOrCopy::OnDestListSelectItem)
	EVT_LIST_ITEM_DESELECTED(ID_LISTCTRL_DESTINATION_CONTENTS, AdminMoveOrCopy::OnDestListDeselectItem)
	EVT_LIST_ITEM_ACTIVATED(ID_LISTCTRL_SOURCE_CONTENTS, AdminMoveOrCopy::OnSrcListDoubleclick)
	EVT_LIST_ITEM_ACTIVATED(ID_LISTCTRL_DESTINATION_CONTENTS, AdminMoveOrCopy::OnDestListDoubleclick)

	EVT_BUTTON(ID_BUTTON_DELETE, AdminMoveOrCopy::OnBnClickedDelete)
	EVT_BUTTON(ID_BUTTON_RENAME, AdminMoveOrCopy::OnBnClickedRename)
	EVT_BUTTON(ID_BUTTON_COPY, AdminMoveOrCopy::OnBnClickedCopy)
	EVT_BUTTON(ID_BUTTON_MOVE, AdminMoveOrCopy::OnBnClickedMove)

END_EVENT_TABLE()

AdminMoveOrCopy::AdminMoveOrCopy(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Move or Copy Folders Or Files"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	pSrcList = NULL;
	pDestList= NULL;
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	MoveOrCopyFilesOrFoldersFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );


	m_strSrcFolderPath = _T("");
	m_strDestFolderPath = _T("");

	srcFoldersArray.Empty();
	srcFilesArray.Empty();
	destFoldersArray.Empty();
	destFilesArray.Empty();
	srcSelectedFoldersArray.Empty();
	srcSelectedFilesArray.Empty();
	destSelectedFilesArray.Empty();
	destSelectedFoldersArray.Empty();
	arrCopiedOK.Empty();
}

AdminMoveOrCopy::~AdminMoveOrCopy() // destructor
{
	pIconImages->RemoveAll();
	delete pIconImages;
	delete pTheColumnForSrcList;
	delete pTheColumnForDestList;

	srcFoldersArray.Clear();
	srcFilesArray.Clear();
	destFoldersArray.Clear();
	destFilesArray.Clear();
	srcSelectedFoldersArray.Clear();
	srcSelectedFilesArray.Clear();
	destSelectedFoldersArray.Clear();
	destSelectedFilesArray.Clear();
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
	
	srcFoldersCount = 0;
	srcFilesCount = 0;
	destFoldersCount = 0;
	destFilesCount = 0;

	m_bUserCancelled = FALSE;
	m_bDoTheSameWay = FALSE;

	// set up pointers to interface objects
	pSrcFolderPathTextCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SOURCE_PATH);
	pSrcFolderPathTextCtrl->SetValidator(wxGenericValidator(&m_strSrcFolderPath));
	pDestFolderPathTextCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_DESTINATION_PATH);
	pDestFolderPathTextCtrl->SetValidator(wxGenericValidator(&m_strDestFolderPath));

	pLocateSrcFolderButton = (wxButton*)FindWindowById(ID_BUTTON_LOCATE_SOURCE_FOLDER);
	wxASSERT(pLocateSrcFolderButton != NULL);
	pLocateDestFolderButton = (wxButton*)FindWindowById(ID_BUTTON_LOCATE_DESTINATION_FOLDER);
	wxASSERT(pLocateDestFolderButton != NULL);

	//pSrcList = (wxListCtrl*)FindWindowById(ID_LISTCTRL_SOURCE_CONTENTS);
	//pDestList= (wxListCtrl*)FindWindowById(ID_LISTCTRL_DESTINATION_CONTENTS);
	pSrcList = (wxListView*)FindWindowById(ID_LISTCTRL_SOURCE_CONTENTS);
	pDestList= (wxListView*)FindWindowById(ID_LISTCTRL_DESTINATION_CONTENTS);

	pUpSrcFolder = (wxBitmapButton*)FindWindowById(ID_BITMAPBUTTON_SRC_OPEN_FOLDER_UP);
	wxASSERT(pUpSrcFolder != NULL);
	pUpDestFolder = (wxBitmapButton*)FindWindowById(ID_BITMAPBUTTON_DEST_OPEN_FOLDER_UP);
	wxASSERT(pUpDestFolder != NULL);

	pMoveButton = (wxButton*)FindWindowById(ID_BUTTON_MOVE);
	pCopyButton = (wxButton*)FindWindowById(ID_BUTTON_COPY);
	pDeleteButton = (wxButton*)FindWindowById(ID_BUTTON_DELETE);
	pRenameButton = (wxButton*)FindWindowById(ID_BUTTON_RENAME);

	// start with lower buttons disabled (they rely on selections to become enabled)
	EnableDeleteButton(FALSE);
	EnableRenameButton(FALSE);
	EnableCopyButton(FALSE);
	EnableMoveButton(FALSE);


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
	pTheColumnForSrcList = new wxListItem;
	pTheColumnForDestList = new wxListItem;
	// set the source column's width & insert it into the src side's wxListCtrl
	pSrcList->GetClientSize(&width,&height);
	pTheColumnForSrcList->SetWidth(width);
	pSrcList->InsertColumn(0, *pTheColumnForSrcList);
	//pSrcList->SetColumnWidth(0,wxLIST_AUTOSIZE); // <- supposed to work but doesn't
	// so I made my own column-width adjusting code using OnSize()
	 
	// set the destination column's width & insert it into the destination side's wxListCtrl
	pDestList->GetClientSize(&width,&height);
	pTheColumnForDestList->SetWidth(width);
	pDestList->InsertColumn(0, *pTheColumnForDestList);
	//pDestList->SetColumnWidth(0, wxLIST_AUTOSIZE); // <- supposed to work but doesn't 
	// so I made my own column-width adjusting code using OnSize()

	// obtain the folder and file bitmap images which are to go at the start of folder
	// names or file names, respectively, in each list; also an 'empty pot' icon for when
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

	// set up the wxListCtrl instances, for each set a column for an icon followed by text
	pSrcList->SetImageList(pIconImages, wxIMAGE_LIST_SMALL);
	pDestList->SetImageList(pIconImages, wxIMAGE_LIST_SMALL);

	emptyFolderMessage = _("The folder is empty (or maybe undefined)");

	// initialize for the "Locate...folder" buttons; we default the source side to the
	// work directory, and display its contents; we default the destination side to an
	// empty path
	if (gpApp->m_bUseCustomWorkFolderPath  && !gpApp->m_customWorkFolderPath.IsEmpty())
	{
		m_strSrcFolderPath = gpApp->m_customWorkFolderPath;
	}
	else
	{
		m_strSrcFolderPath = gpApp->m_workFolderPath;
	}
	SetupSrcList(m_strSrcFolderPath);
	SetupSelectionArrays(sourceSide);

	m_strDestFolderPath = _T(""); // start with no path defined
	SetupDestList(m_strDestFolderPath);
	SetupSelectionArrays(destinationSide);

	// make the font show the user's desired dialog font point size ( I think this dialog can
	// instead just rely on the system font supplied to the dialog by default)
	#ifdef _RTL_FLAGS
//	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, pNewFileName, NULL,
//					pAcceptedFiles, pRejectedFiles, gpApp->m_pDlgGlossFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
//	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, pNewFileName, NULL, 
//					pAcceptedFiles, pRejectedFiles, gpApp->m_pDlgGlossFont);
	#endif

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
void AdminMoveOrCopy::DeselectSelectedFiles(enum whichSide side)
{
	long index = 0;
	long limit;
	if (side == sourceSide)
	{
		limit = pSrcList->GetItemCount(); // index of last file is (limit - 1)
		if (srcFilesCount == 0 || pSrcList->GetSelectedItemCount() == 0)
		{
			return; // nothing to do
		}
		index = pSrcList->GetFirstSelected();
		if (index != (long)-1)
		{
			// there is one or more selections
			pSrcList->Select(index,FALSE); // FALSE turns selection off
		}
		do {
			index = pSrcList->GetNextSelected(index);
			if (index == -1)
				break;
			// *** BEWARE *** this call uses SetItemState() and it causes
			// m_nCount of wxArrayString to get set to 1 spuriously, for
			// the strSrcSelectedFilesArray after the latter is Empty()ied,
			// so make the .Empty() call be done after DeselectSelectedFiles()
			pSrcList->Select(index,FALSE); // FALSE turns selection off
		} while (TRUE);
	}
	else
	{
		limit = pDestList->GetItemCount(); // index of last file is (limit - 1)
		if (destFilesCount == 0 || pDestList->GetSelectedItemCount() == 0)
		{
			return; // nothing to do
		}
		index = pDestList->GetFirstSelected();
		if (index != (long)-1)
		{
			// there is one or more selections
			pDestList->Select(index,FALSE); // FALSE turns selection off
		}
		do {
			index = pDestList->GetNextSelected(index);
			if (index == -1)
				break;
			// *** BEWARE *** this call uses SetItemState() and it might cause
			// m_nCount of wxArrayString to get set to 1 spuriously, -- see above
		} while (TRUE);
	}
	// unsigned int aCount = srcSelectedFilesArray.GetCount();  same bug manifests, so
	// wxListView doesn't help get round the problem
}

void AdminMoveOrCopy::GetListCtrlContents(enum whichSide side, wxString& folderPath,
										  bool& bHasFolders, bool& bHasFiles)
{
	// calls helpers.cpp functions GetFoldersOnly() and GetFilesOnly() which are made helpers
	// functions because they may be useful to reuse someday in other places in the app
	bHasFolders = FALSE;
	bHasFiles = FALSE;
	if (side == sourceSide)
	{
		// left side of the dialog (source folder's side)
		
		// clear out old content in the list and its supporting arrays
		srcFoldersArray.Empty();
		srcFilesArray.Empty();
		pSrcList->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too

		bHasFolders = GetFoldersOnly(folderPath, &srcFoldersArray,TRUE,
						gpApp->m_bAdminMoveOrCopyIsInitializing); // TRUE means to sort the array
		bHasFiles = GetFilesOnly(folderPath, &srcFilesArray); // default is to sort the array

	}
	else
	{
		// right side of the dialog (destination folder's side)
		
		// clear out old content in the list and its supporting arrays
		destFoldersArray.Empty();
		destFilesArray.Empty();
		pDestList->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too

		bHasFolders = GetFoldersOnly(folderPath, &destFoldersArray, TRUE,
						gpApp->m_bAdminMoveOrCopyIsInitializing); // TRUE means sort the array
		bHasFiles = GetFilesOnly(folderPath, &destFilesArray); // default is to sort the array
	}

	// debugging -- display what we got for source side & destination side too
	/*
#ifdef __WXDEBUG__
	if (side == sourceSide)
	{
		size_t foldersCount = srcFoldersArray.GetCount();
		size_t counter;
		wxLogDebug(_T("Folders (sorted, & fetched in ascending index order):\n"));
		for (counter = 0; counter < foldersCount; counter++)
		{
			wxString aFolder = srcFoldersArray.Item(counter);
			wxLogDebug(_T("   %s \n"),aFolder);
		}
		wxLogDebug(_T("Files (sorted, & fetched in ascending index order):\n"));
		size_t filesCount = srcFilesArray.GetCount();
		for (counter = 0; counter < filesCount; counter++)
		{
			wxString aFile = srcFilesArray.Item(counter);
			wxLogDebug(_T("   %s \n"),aFile);
		}
	}
	// nice, both folders and files lists are sorted right and all names correct
	else
	{
		size_t foldersCount = destFoldersArray.GetCount();
		size_t counter;
		wxLogDebug(_T("DESTINATION folders (sorted, & fetched in ascending index order):\n"));
		for (counter = 0; counter < foldersCount; counter++)
		{
			wxString aFolder = destFoldersArray.Item(counter);
			wxLogDebug(_T("   %s \n"),aFolder);
		}
		wxLogDebug(_T("DESTINATION files (sorted, & fetched in ascending index order):\n"));
		size_t filesCount = destFilesArray.GetCount();
		for (counter = 0; counter < filesCount; counter++)
		{
			wxString aFile = destFilesArray.Item(counter);
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
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}

void AdminMoveOrCopy::SetupSrcList(wxString& folderPath)
{
	// put the path into the edit control
	pSrcFolderPathTextCtrl->ChangeValue(folderPath);

	// enumerate the files and folders, insert in list ctrl
	long rv = 0L; // for a return value
	bool bHasFiles;
	bool bHasFolders;
	wxString aFolder;
	wxString aFile;
	GetListCtrlContents(sourceSide, folderPath, bHasFolders, bHasFiles);
	if (bHasFolders || bHasFiles)
	{
		srcFoldersCount = 0;
		srcFilesCount = 0;

		// now try put the lines of data in the list; first folders, then files
		size_t index = 0;
		srcFoldersCount = srcFoldersArray.GetCount();
		if (bHasFolders)
		{
			for (index = 0; index < srcFoldersCount; index++)
			{
				aFolder = srcFoldersArray.Item(index);
				rv = pSrcList->InsertItem(index,aFolder,indxFolderIcon);
			}
		}
		srcFilesCount = srcFilesArray.GetCount();
		if (bHasFiles)
		{
			// this loop has to start at the index value next after
			// the last value of the folders loop above
			for (index = 0; index < srcFilesCount; index++)
			{
				aFile = srcFilesArray.Item(index);
				rv = pSrcList->InsertItem(srcFoldersCount + index,aFile,indxFileIcon);
			}
		}
		// Disable the move and copy buttons at the bottom
		pMoveButton->Enable(FALSE);
		pCopyButton->Enable(FALSE);
	}
	else
	{
        // no files or folders, so disable the move and copy buttons at the bottom, and put
		// a "The folder is empty" message into the list with an empty jug icon, because
		// without an explicit icon, the icon in the list with index = 0 gets shown, and
		// that is the folder icon - which would be confusing, as it would suggest a
		// folder was found with the name "The folder is empty".
		rv = pSrcList->InsertItem(0, emptyFolderMessage,indxEmptyIcon);
		srcFoldersCount = 0;
		srcFilesCount = 0;

		pMoveButton->Disable();
		pCopyButton->Disable();
	}
}

void AdminMoveOrCopy::SetupDestList(wxString& folderPath)
{
	// put the path into the edit control
	pDestFolderPathTextCtrl->ChangeValue(folderPath);

	// enumerate the files and folders, insert in list ctrl
	long rv = 0L; // for a return value
	bool bHasFiles;
	bool bHasFolders;
	wxString aFolder;
	wxString aFile;
	GetListCtrlContents(destinationSide, folderPath, bHasFolders, bHasFiles);
	if (bHasFolders || bHasFiles)
	{
		destFoldersCount = 0;
		destFilesCount = 0;

		// now try put the lines of data in the list; first folders, then files
		size_t index = 0;
		destFoldersCount = destFoldersArray.GetCount();
		if (bHasFolders)
		{
			for (index = 0; index < destFoldersCount; index++)
			{
				aFolder = destFoldersArray.Item(index);
				rv = pDestList->InsertItem(index,aFolder,indxFolderIcon);
			}
		}
		destFilesCount = destFilesArray.GetCount();
		if (bHasFiles)
		{
			// this loop has to start at the index value next after
			// the last value of the folders loop above
			for (index = 0; index < destFilesCount; index++)
			{
				aFile = destFilesArray.Item(index);
				rv = pDestList->InsertItem(destFoldersCount + index,aFile,indxFileIcon);
			}
		}
	}
	else
	{
        // no files or folders, put a "The folder is empty" message into the list with an
        // empty jug icon, because without an explicit icon, the icon in the list with
        // index = 0 gets shown, and that is the folder icon - which would be confusing, as
        // it would suggest a folder was found with the name "The folder is empty".
		rv = pDestList->InsertItem(0, emptyFolderMessage,indxEmptyIcon);
		destFoldersCount = 0;
		destFilesCount = 0;
	}
	EnableDeleteButton(FALSE);
	EnableRenameButton(FALSE);
}


void AdminMoveOrCopy::OnBnClickedLocateSrcFolder(wxCommandEvent& WXUNUSED(event))
{
	CMainFrame* pFrame = gpApp->GetMainFrame();
	wxString msg = _("Locate the source folder");
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
	m_strSrcFolderPath_OLD = m_strSrcFolderPath; // save current path in case user cancels
	m_strSrcFolderPath = wxDirSelector(msg,m_strSrcFolderPath,style,pos,(wxWindow*)pFrame);
	if (m_strSrcFolderPath.IsEmpty())
	{
		// restore the old path
		m_strSrcFolderPath = m_strSrcFolderPath_OLD;
	}
	SetupSrcList(m_strSrcFolderPath);
	EnableCopyButton(FALSE);
	EnableMoveButton(FALSE);
}

void AdminMoveOrCopy::OnBnClickedLocateDestFolder(wxCommandEvent& WXUNUSED(event))
{
	CMainFrame* pFrame = gpApp->GetMainFrame();
	wxString msg = _("Locate the destination folder");
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
	m_strDestFolderPath_OLD = m_strDestFolderPath; // save current path in case user cancels
	m_strDestFolderPath = wxDirSelector(msg,m_strDestFolderPath,style,pos,(wxWindow*)pFrame);
	if (m_strDestFolderPath.IsEmpty())
	{
		// restore the old path
		m_strDestFolderPath = m_strDestFolderPath_OLD;
	}
	SetupDestList(m_strDestFolderPath);
	EnableRenameButton(FALSE);
	EnableDeleteButton(FALSE);
}

void AdminMoveOrCopy::OnBnClickedSrcParentFolder(wxCommandEvent& WXUNUSED(event))
{
	wxFileName* pFN = new wxFileName;
	wxChar charSeparator = pFN->GetPathSeparator();
	int offset;
	wxString path = m_strSrcFolderPath;
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
				m_strSrcFolderPath = path;
				SetupSrcList(m_strSrcFolderPath);
			}
			else
			{
				// we've a legitimate path, so make it the source path
				m_strSrcFolderPath = path;
				SetupSrcList(m_strSrcFolderPath);
			}
		}
		else
		{
			// we are at the root in Linux or Unix
			path = charSeparator;
			m_strSrcFolderPath = path;
			SetupSrcList(m_strSrcFolderPath);
		}
	}
	delete pFN;
	EnableCopyButton(FALSE); // start off disabled until a file is selected
	EnableMoveButton(FALSE); // ditto
}

void AdminMoveOrCopy::OnBnClickedDestParentFolder(wxCommandEvent& WXUNUSED(event))
{
	wxFileName* pFN = new wxFileName;
	wxChar charSeparator = pFN->GetPathSeparator();
	int offset;
	wxString path = m_strDestFolderPath;
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
				m_strDestFolderPath = path;
				SetupDestList(m_strDestFolderPath);
			}
			else
			{
				// we've a legitimate path, so make it the source path
				m_strDestFolderPath = path;
				SetupDestList(m_strDestFolderPath);
			}
		}
		else
		{
			// we are at the root in Linux or Unix
			path = charSeparator;
			m_strDestFolderPath = path;
			SetupDestList(m_strDestFolderPath);
		}
	}
	delete pFN;
}

void AdminMoveOrCopy::OnSize(wxSizeEvent& event)
{
	if (this == event.GetEventObject())
	{
		// it's our AdminMoveOrCopy dialog
		// re-set the source column's width
		if (pSrcList == NULL || pDestList == NULL)
		{
			event.Skip();
			return;
		}
		// control can still get here when the control is not setup initially, so next
		// block should catch those (for which pSrcList is not NULL, but not defined)
		if (pSrcList->GetColumnCount() == 0 || pDestList->GetColumnCount() == 0 ||
			(wxImageList*)pSrcList->GetImageList(wxIMAGE_LIST_SMALL) == NULL ||
			(wxImageList*)pDestList->GetImageList(wxIMAGE_LIST_SMALL) == NULL)
		{
			event.Skip();
			return;
		}
		int colWidth_Src = pSrcList->GetColumnWidth(0);
		int colWidth_Dest = pDestList->GetColumnWidth(0);
		int width_Src; int width_Dest; int height_Src; int height_Dest;
		pSrcList->GetClientSize(&width_Src,&height_Src);
		pDestList->GetClientSize(&width_Dest,&height_Dest);
		if (width_Src != colWidth_Src)
			pSrcList->SetColumnWidth(0,width_Src);
		if (width_Dest != colWidth_Dest)
			pDestList->SetColumnWidth(0,width_Dest);
	}
	event.Skip();
}
/////////////////////////////////////////////////////////////////////////////////
/// \return     the suitably changed filename (so that the conflict is removed)
/// \param      pFilename   ->  pointer to the filename string from the destination
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
	wxFileName fn(m_strDestFolderPath,*pFilename);
	wxString extn = _T("");
	bool bHasExtension = FALSE;
	if (fn.HasExt())
	{
		extn = fn.GetExt();
		bHasExtension = TRUE; // to handle when only the . of an extension is present
	}
	wxString name = fn.GetName();
	wxString reversed = MakeReverse(name); // our utility from helpers.cpp
	// look for ")d...d(" at the start of the reversed string, where d...d is one or more
	// digits; we want to get the digit(s), convert to int, increment by 1, convert back
	// to digits, and build the new string with (n) at the end where n is the new larger
	// value. However, mostly no such end string is present, in which case we can just
	// create a name with "(2)" at the end immediately.
	wxString shortname;
	wxChar aChar = reversed.GetChar(0);
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


void AdminMoveOrCopy::OnDestListSelectItem(wxListEvent& event)
{
	// we repopulate the srcSelectionArray each time with the set of selected items 
	//wxLogDebug(_T("OnDestListSelectItem"));
	event.Skip();
	SetupSelectionArrays(destinationSide); // update destSelectionArray 
										  // with current selections
}

void AdminMoveOrCopy::OnSrcListDeselectItem(wxListEvent& event)
{
	event.Skip();
	SetupSelectionArrays(sourceSide); // update srcSelectedFilesArray 
									 // with current selections 
}

void AdminMoveOrCopy::OnDestListDeselectItem(wxListEvent& event)
{
	event.Skip();
	SetupSelectionArrays(destinationSide); // update destSelectedFilesArray 
										  // with current selections 
}

void AdminMoveOrCopy::OnSrcListSelectItem(wxListEvent& event)
{
	// we repopulate the srcSelectionArray each time with the set of selected items 
	//wxLogDebug(_T("OnSrcListSelectItem -- provides data for doubleclick handler"));
	event.Skip();
	SetupSelectionArrays(sourceSide); // update srcSelectionArray 
									 // with current selections
}

void AdminMoveOrCopy::OnSrcListDoubleclick(wxListEvent& event)
{
	//wxLogDebug(_T("OnSrcListDoubleclick"));
	size_t index = event.GetIndex();
	if (index < srcFoldersCount)
	{
		// we clicked on a folder name, so drill down to that child folder and display its
		// contents in the dialog;  check here for an empty string in m_srcItemText, and if so
		// then don't do anything in this handler
		if (event.GetText().IsEmpty())
			return;		

		// extend the path using this foldername, and then display the contents
		m_strSrcFolderPath += gpApp->PathSeparator + event.GetText();
		SetupSrcList(m_strSrcFolderPath);
		//event.Skip();
		EnableCopyButton(FALSE);
		EnableMoveButton(FALSE);
	}
}


void AdminMoveOrCopy::OnDestListDoubleclick(wxListEvent& event)
{
	//wxLogDebug(_T("OnDestListDoubleclick"));
	//size_t index = m_destIndex; // set from the preceding OnSrcListSelectItem() call,
							   // (the dbl click event is posted after the selection event)
	size_t index = event.GetIndex();
	if (index < destFoldersCount)
	{
		// we clicked on a folder name, so drill down to that child folder and display its
		// contents in the dialog;  check here for an empty string in m_destItemText, and if so
		// then don't do anything in this handler
		if (event.GetText().IsEmpty())
			return;		

		// extend the path using this foldername, and then display the contents
		m_strDestFolderPath += gpApp->PathSeparator + event.GetText();
		SetupDestList(m_strDestFolderPath);
		EnableRenameButton(FALSE);
		EnableDeleteButton(FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      side    ->  enum with value either sourceSide, or destinationSide 
///  \remarks   Populates srcSelectionArray, or destSelectionArray, with wxString filename
///  entries or foldername entries or both, depending on what is selected in the passed in
///  side's wxListCtrl.
///  A side effect is to enable or disable the Move, Copy, Rename and/or Delete buttons.
///  If multiple files or folders or both are selected, this function is only used at the
///  top level of the handler for such butons, because the handlers are recursive and for
///  child folders the selections are set up programmatically.
///  Another job it does is to populate the member arrays, srcSelectedFilesArray and
///  srcSelectedFoldersArray, needed at the top level of the Copy or Move handlers.
//////////////////////////////////////////////////////////////////////////////////
void AdminMoveOrCopy::SetupSelectionArrays(enum whichSide side)
{
	size_t index = 0;
	size_t limit = 0;
	long stateMask = 0;
	stateMask |= wxLIST_STATE_SELECTED;
	int isSelected = 0;
	if (side == sourceSide)
	{
		srcSelectedFilesArray.Empty();
		srcSelectedFoldersArray.Empty();
		limit = pSrcList->GetItemCount();
		if ((srcFilesCount == 0 && srcFoldersCount == 0) || 
			pSrcList->GetSelectedItemCount() == 0)
		{
			EnableCopyButton(FALSE);
			EnableMoveButton(FALSE);
			return; // nothing to do
		}
		srcSelectedFoldersArray.Alloc(srcFoldersCount);
		srcSelectedFilesArray.Alloc(srcFilesCount);
		for (index = 0; index < limit; index++)
		{
			isSelected = pSrcList->GetItemState(index,stateMask);
			if (isSelected)
			{
				// this one is selected, add it's name to the list of selected items
				wxString itemName = pSrcList->GetItemText(index);

				// is it a folder or a file - find out and add it to the appropriate array
				wxString path = m_strSrcFolderPath + gpApp->PathSeparator + itemName;
				size_t itsIndex = 0xFFFF;
				if (::wxDirExists(path.c_str()))
				{
					// it's a directory
					itsIndex = srcSelectedFoldersArray.Add(itemName);
				}
				else
				{
					// it'a a file
					itsIndex = srcSelectedFilesArray.Add(itemName);
				}
				isSelected = 0;
			}
		}
		if (m_strSrcFolderPath.IsEmpty() || m_strDestFolderPath.IsEmpty())
		{
			EnableCopyButton(FALSE);
			EnableMoveButton(FALSE);
		}
		else
		{
			if (srcSelectedFilesArray.GetCount() > 0 || srcSelectedFoldersArray.GetCount() > 0)
			{
				EnableCopyButton(TRUE);
				EnableMoveButton(TRUE);
			}
			else
			{
				EnableCopyButton(FALSE);
				EnableMoveButton(FALSE);
			}
		}
	}
	else
	{
		destSelectedFilesArray.Empty(); // clear, we'll refill in the loop
		destSelectedFoldersArray.Empty(); // ditto
		limit = pDestList->GetItemCount();
		if ((destFilesCount == 0 && destFoldersCount == 0) || 
			pDestList->GetSelectedItemCount() == 0)
		{
			EnableRenameButton(FALSE);
			EnableDeleteButton(FALSE);
			return; // nothing to do
		}
		destSelectedFilesArray.Alloc(limit);
		destSelectedFoldersArray.Alloc(limit);
		for (index = 0; index < limit; index++)
		{
			isSelected = pDestList->GetItemState(index,stateMask);
			if (isSelected)
			{
				// this one is selected, add it's name to the list of selected items
				wxString itemName = pDestList->GetItemText(index);

				// is it a folder or a file - find out and add it to the appropriate array
				wxString path = m_strDestFolderPath + gpApp->PathSeparator + itemName;
				size_t itsIndex = 0xFFFF;
				if (::wxDirExists(path.c_str()))
				{
					// it's a directory
					itsIndex = destSelectedFoldersArray.Add(itemName);
				}
				else
				{
					// it'a a file
					itsIndex = destSelectedFilesArray.Add(itemName);
				}
				isSelected = 0;
			}
		}
		//if (destSelectionArray.IsEmpty())
		if (destSelectedFoldersArray.IsEmpty() && destSelectedFilesArray.IsEmpty())
		{
			// disable the Rename and Delete buttons if nothing is selected in the
			// destination side's list
			EnableRenameButton(FALSE);
			EnableDeleteButton(FALSE);
		}
		else
		{
			// enable Rename button only if a single file is selected, or a single folder
			if (destSelectedFilesArray.GetCount() == 1 && destSelectedFoldersArray.GetCount() == 0)
				EnableRenameButton(TRUE);
			else if (destSelectedFoldersArray.GetCount() == 1 && destSelectedFilesArray.GetCount() == 0)
				EnableRenameButton(TRUE);
			else
				EnableRenameButton(FALSE);
			// allow multiple file or folder deletions
			EnableDeleteButton(TRUE);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////
///
///    END OF GUI FUNCTIONS                 START OF BUTTON HANDLERS 
///
///////////////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if the passed in paths are identical, FALSE otherwise
/// \param      srcPath    ->  reference to the source side's file path 
/// \param      destPath   ->  reference to the destination side's file path 
///  \remarks   Use to prevent copy or move of a file or folder to the same folder. We
///  allow both sides to have the same path shown, but in the button handlers we check as
///  necessary and prevent wrong action - and show a warning message
//////////////////////////////////////////////////////////////////////////////////
bool AdminMoveOrCopy::CheckForIdenticalPaths(wxString& srcPath, wxString& destPath)
{
	if (srcPath == destPath)
	{
		wxString msg;
		::wxBell();
		msg = msg.Format(_("The source and destination folders must not be the same folder."));
		wxMessageBox(msg,_("Copy or Move is not permitted"),wxICON_WARNING);
		return TRUE;
	}
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if the file in the source list with index srcFileIndex has the
///             same filename as a file in the destination folder; FALSE otherwise
/// \param      srcFile        ->   reference to the source file's filename
/// \param      pConflictIndex <-   pointer to 0 based index to whichever filename in
///                                 destFilesArray (passed in as pDestFilesArr) has 
///                                 the same name as for the one specified by srcFile;
///                                 has value -1 if unset
/// \param      pDestFilesArr   ->  pointer to a string array of all the filenames in
///                                 the destination folder
///  \remarks   Iterates through all the files in the pDestFilesArr array, looking for
///             a match with the filename specified by srcFileIndex, returning TRUE if
///             a match is made, otherwise FALSE
//////////////////////////////////////////////////////////////////////////////////
bool AdminMoveOrCopy::IsFileConflicted(wxString& srcFile, int* pConflictIndex, 
									  wxArrayString* pDestFilesArr)
{
	wxASSERT(!srcFile.IsEmpty());
	(*pConflictIndex) = wxNOT_FOUND; // -1
	size_t limit = pDestFilesArr->GetCount();
	if (limit == 0)
	{
		// there are no files in the destination folder, so no conflict is possible
		return FALSE;
	}
	size_t destIndex;
	wxString aFilename;
	for (destIndex = 0; destIndex < limit; destIndex++) 
	{
		aFilename = pDestFilesArr->Item(destIndex);
		if (aFilename == srcFile)
		{
			(*pConflictIndex) = (int)destIndex;
			return TRUE;
		}
	}
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if the copy was successfully done (in the event of a filename clash,
///             successfully done means either that the user chose to copy and replace,
///             overwriting the file in the destination folder of the same name, or he
///             chose to copy with a changed name; a choice to 'not copy' should be
///             interpretted as non-success and FALSE should be returned); return FALSE if
///             the copy was not successful. The "Cancel" response in the event of a
///             filename clash should also cause FALSE to be returned.
/// \param      srcPath     ->  reference to the source folder path's string (no final 
///                             path separator)
/// \param      destPath    ->  reference to the destination folder path's string (no final 
///                             path separator)
/// \param      filename    ->  the name of the file to be copied (prior to knowing whether 
///                             there is or isn't a name conflict with a file in the 
///                             destination folder)
///  \param     bUserCancelled <-> reference to boolean indicating whether or not the user
///                               clicked the Cancel button in the FilenameConflictDlg
///  \remarks   This function handles the low level copying of a single file to the
///  destination folder. It's return value is important for a "Move" choice in the GUI,
///  because we support Moving a file by first copying it, and if the copy was successful,
///  then we delete the original file in the source folder. In the event of a filename
///  clash, the protocol for handling that is encapsulated herein - a child dialog is put
///  up with 3 choices, similarly to what Windows Explorer offers. A flag is included in
///  the signature to return cancel information to the caller in the event of a filename
///  clash. Since our design allows for multiple filenames in the source folder to be
///  copied (or moved) with a single click of the Copy.. or Move.. buttons, this function
///  may be called multiple times by the caller - taking each filename from a string array
///  populated earlier. In the event of a filename clash, if the user hits the Cancel
///  button, it will cancel the Copy or Move command at that time, but any files already
///  copied or moved will remain so.
//////////////////////////////////////////////////////////////////////////////////
bool AdminMoveOrCopy::CopySingleFile(wxString& srcPath, wxString& destPath, wxString& filename, 
						bool& bUserCancelled)
{
	wxString theDestinationPath; // build the path to the file here, from destPath and filename
	bool bSuccess = TRUE;
	copyType = copyAndReplace; // set to default value
	wxString newFilenameStr = _T("");

	// make the path to the source file
	wxString theSourcePath = srcPath + gpApp->PathSeparator + filename;

	// get an array of the filenames in the destination folder
	wxArrayString myDestFilesArray;
	bool bGotten = GetFilesOnly(destPath, &myDestFilesArray, TRUE, TRUE);
	bGotten = bGotten; // avoid compiler warning

	// check for a name conflict (including extension, same names but different extensions
	// is not to be treated as a conflict)
	int nConflictedFileIndex = wxNOT_FOUND; // -1
	bool bIsConflicted = IsFileConflicted(filename,&nConflictedFileIndex,&myDestFilesArray);
	if (bIsConflicted)
	{
		// handle the filename conflict here; the existence of a conflict is testimony to
		// the fact that the destination folder has the conflicting file in it, so we
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
				theDestinationPath = destPath + gpApp->PathSeparator + filename;
				bSuccess = ::wxCopyFile(theSourcePath, theDestinationPath); //bool overwrite = true
				break;
			case copyWithChangedName:
				newFilenameStr = BuildChangedFilenameForCopy(&filename);
				theDestinationPath = destPath + gpApp->PathSeparator + newFilenameStr;
				bSuccess = ::wxCopyFile(theSourcePath, theDestinationPath); //bool overwrite = true
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
			FilenameConflictDlg dlg(this,&filename,&srcPath,&destPath);
			dlg.Centre();
			if (dlg.ShowModal() == wxID_OK)
			{
				// Close button was pressed, so get the user's choices
				m_bDoTheSameWay = dlg.bSameWayValue;
				lastWay = copyType; // update lastWay enum value
				// The value of the copyType enum has been set at the Close button call in the
				// FilenameConflictDlg
				//m_bDoTheSameWay = m_bDoTheSameWay; // for checking value in debugger
				switch (copyType)
				{
				case copyAndReplace:
					theDestinationPath = destPath + gpApp->PathSeparator + filename;
					bSuccess = ::wxCopyFile(theSourcePath, theDestinationPath); //bool overwrite = true
					break;
				case copyWithChangedName:
					newFilenameStr = BuildChangedFilenameForCopy(&filename);
					theDestinationPath = destPath + gpApp->PathSeparator + newFilenameStr;
					bSuccess = ::wxCopyFile(theSourcePath, theDestinationPath); //bool overwrite = true
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
		// first, make the path to the destination folder
		theDestinationPath = destPath + gpApp->PathSeparator + filename;
		bool bAlreadyExists = ::wxFileExists(theDestinationPath);
		if (bAlreadyExists)
		{
			// This block should never be entered; we've already determined there is no
			// name conflict and so the destination folder should not have a file of the
			// same name. Because of the possibility of losing valuable data, what we do
			// here is to alert the user to the file which is in danger of being lost due
			// to the copy, then refraining from the copy automatically, keep the app
			// running but return FALSE so that if a Move... was requested, then the
			// caller will not go ahead with deletion of the source folder's file
			wxString msg;
			msg = msg.Format(_("The destination folder's file with the name %s would be overwritten if this move or copy were to go ahead. To avoid this unexpected possibility for data loss, the move or copy will now be cancelled. Do something appropriate with the destination folder's file, and then try again."),
				filename.c_str());
			wxMessageBox(msg,_("Unexpected Filename Conflict During Copy Or Move"),wxICON_WARNING);
			return FALSE;
		}
		bool bSuccess2 = ::wxCopyFile(theSourcePath, theDestinationPath); //bool overwrite = true
		if (!bSuccess2)
		{
			wxString msg;
			msg = msg.Format(
_("Moving or copying the file with path %s failed unexpectedly. Possibly you forgot to use the button for locating a destination folder. Do so then try again."),
theSourcePath.c_str());
			wxMessageBox(msg,_("Moving or copying failed"),wxICON_WARNING);
			if (bSuccess)
				bSuccess = FALSE;
		}
	}
	return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if the removal was successfully done; return FALSE if
///             the removal was not successful. (FALSE should halt the removals.)
/// \param      destPath    ->  reference to the destination folder path's string (no final 
///                             path separator)-- removals are done from right side
///                             wxListCtrl only
/// \param      filename    ->  the name of the file to be removed
///  \remarks   This function handles the low level copying of a single file to the
///  destination folder. It's return value is important for a "Move" choice in the GUI,
///  because we support Moving a file by first copying it, and if the copy was successful,
///  then we delete the original file in the source folder. In the event of a filename
///  clash, the protocol for handling that is encapsulated herein - a child dialog is put
///  up with 3 choices, similarly to what Windows Explorer offers. A flag is included in
///  the signature to return cancel information to the caller in the event of a filename
///  clash. Since our design allows for multiple filenames in the source folder to be
///  copied (or moved) with a single click of the Copy.. or Move.. buttons, this function
///  may be called multiple times by the caller - taking each filename from a string array
///  populated earlier. In the event of a filename clash, if the user hits the Cancel
///  button, it will cancel the Copy or Move command at that time, but any files already
///  copied or moved will remain so.
//////////////////////////////////////////////////////////////////////////////////
bool AdminMoveOrCopy::RemoveSingleFile(wxString& destPath, wxString& filename)
{
	bool bSuccess = TRUE;

	// make the path to the destination folder's file
	wxString theDestPath = destPath + gpApp->PathSeparator + filename;

	// do the removal
	bSuccess = ::wxRemoveFile(theDestPath); //bool overwrite = true
	if (!bSuccess)
	{
		wxString msg;
		msg = msg.Format(
_("Removing the file with path %s failed. Removals have been halted. Possibly an application has the file still open. Close the file and then you can try the removal operation again."),
		theDestPath.c_str());
		wxMessageBox(msg,_("Removing a file failed"),wxICON_WARNING);

		// use the m_bUserCancelled mechanism to force drilling up through any recursions,
		// but don't clobber the AdminMoveOrCopy dialog itself
		m_bUserCancelled = TRUE;
	}
	return bSuccess;
}

void AdminMoveOrCopy::OnBnClickedDelete(wxCommandEvent& WXUNUSED(event))
{
	m_bUserCancelled = FALSE;

	// on the heap, create the arrays of files and folders which are to be removed & populate them
	wxArrayString* pDestSelectedFoldersArray	= new wxArrayString;
	wxArrayString* pDestSelectedFilesArray	= new wxArrayString;
	size_t index;
	size_t foldersLimit = destSelectedFoldersArray.GetCount(); // RHS populated in SetSelectionArray() call earlier
	size_t filesLimit = destSelectedFilesArray.GetCount(); // RHS populated in SetSelectionArray() call earlier
	if (destSelectedFilesArray.GetCount() == 0 && destSelectedFoldersArray.GetCount() == 0)
	{
		wxMessageBox(
_("You first need to select at least one item in the right hand list before clicking the Delete button"),
		_("No Files Or Folders Selected"),wxICON_WARNING);
		pDestSelectedFilesArray->Clear(); // this one is on heap
		pDestSelectedFoldersArray->Clear(); // this one is on heap
		delete pDestSelectedFilesArray; // don't leak it
		delete pDestSelectedFoldersArray; // don't leak it
		return;
	}
	for (index = 0; index < foldersLimit; index++)
	{
		pDestSelectedFoldersArray->Add(destFoldersArray.Item(index));
	}
	for (index = 0; index < filesLimit; index++)
	{
		pDestSelectedFilesArray->Add(destFilesArray.Item(index));
	}

	RemoveFilesAndFolders(m_strDestFolderPath, pDestSelectedFoldersArray, pDestSelectedFilesArray);

	// clear the allocations to the heap
	pDestSelectedFilesArray->Clear(); // this one is on heap
	pDestSelectedFoldersArray->Clear(); // this one is on heap
	delete pDestSelectedFilesArray; // don't leak it
	delete pDestSelectedFoldersArray; // don't leak it

	destSelectedFilesArray.Clear();
	destSelectedFoldersArray.Clear();

	// update the destination list
	SetupDestList(m_strDestFolderPath);
}

void AdminMoveOrCopy::RemoveFilesAndFolders(wxString destFolderPath, 
				wxArrayString* pDestSelectedFoldersArray, 
				wxArrayString* pDestSelectedFilesArray)
{
	// copy the files first, then recurse for copying the one or more folders, if any
	size_t limitSelectedFiles = pDestSelectedFilesArray->GetCount();
	size_t limitSelectedFolders = pDestSelectedFoldersArray->GetCount();
	bool bRemovedOK = TRUE;

    // the passed in destination folder may have no files in it - check, and only do this
    // block if there is at least one file to be removed
	size_t nItemIndex = 0;
	if (limitSelectedFiles > 0)
	{
		wxString aFilename = pDestSelectedFilesArray->Item(nItemIndex);
		wxASSERT(!aFilename.IsEmpty());
		// loop across all files that were selected, deleting each as we go
		do {
			// do the removal of the file (an internal failure will not just return FALSE,
			// but will set the member boolean m_bUserCancelled to TRUE)
			bRemovedOK = RemoveSingleFile(destFolderPath, aFilename);
//			wxLogDebug(_T("\nRemoved  %s  ,  folder path = %s "),aFilename, destFolderPath);
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
				aFilename = pDestSelectedFilesArray->Item(nItemIndex);
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
			wxString destFolderPath2;
			wxArrayString* pDestSelectedFoldersArray2 = new wxArrayString;
			wxArrayString* pDestSelectedFilesArray2 = new wxArrayString;
			wxString aFoldername = pDestSelectedFoldersArray->Item(indexForLoop); // get next directory name
			destFolderPath2 = destFolderPath + gpApp->PathSeparator + aFoldername; // we know this folder exists
            // Now we must call GetFilesOnly() and GetFoldersOnly() in order to gather the
            // destination child directory's folder names and file names into the the arrays provided
			// (Each call internally resets the working directory.) First TRUE is bool
			// bSort, second TRUE is bool bSuppressMessage.
            bool bHasFolders = GetFoldersOnly(destFolderPath2, pDestSelectedFoldersArray2, TRUE, TRUE);
			bool bHasFiles = GetFilesOnly(destFolderPath2, pDestSelectedFilesArray2, TRUE, TRUE);
			bHasFolders = bHasFolders; // avoid compiler warning
			bHasFiles = bHasFiles; // avoid compiler warning

			// Reenter
//			wxLogDebug(_T("Re-entering: destPath = %s, hasFolders %d , hasFiles %d"),
//				destFolderPath2, (int)bHasFolders, (int)bHasFiles);
			RemoveFilesAndFolders(destFolderPath2, pDestSelectedFoldersArray2, pDestSelectedFilesArray2);

            // on the return of the above call srcFolderPath2 will have been emptied of
            // both files and folders, so remove it; first reset the working directory to a
            // directory not to be removed, and the removals will work
			bool bOK = ::wxSetWorkingDirectory(m_strDestFolderPath);
			bOK = ::wxRmdir(destFolderPath2);
			if (!bOK)
			{
				wxString msg;
				msg = msg.Format(_T("Failed to remove directory %s "),destFolderPath2.c_str());
				wxMessageBox(msg,_T("Could not remove directory"),wxICON_WARNING);
				m_bUserCancelled = TRUE;
			}
//			else
//			{
//				wxLogDebug(_T("Removed directory  %s  "),destFolderPath2);
//			}
				
			// clean up for this iteration
			pDestSelectedFilesArray2->Clear();
			pDestSelectedFoldersArray2->Clear();
			delete pDestSelectedFilesArray2; // don't leak it
			delete pDestSelectedFoldersArray2; // don't leak it

			// if there was a failure...
			if (m_bUserCancelled)
			{
				// exit loop immediately
				return;
			}
		} // end block for the for loop iterating over selected foldernames
	}
}

void AdminMoveOrCopy::OnBnClickedRename(wxCommandEvent& WXUNUSED(event))
{
	bool bOK = TRUE;
	if (destSelectedFoldersArray.GetCount() == 1)
	{
		wxString theFolderPath = m_strDestFolderPath + gpApp->PathSeparator;
		wxString oldName = destSelectedFoldersArray.Item(0); // old folder name
		theFolderPath += oldName; // path to selected folder which we want to rename

		// now get the user's wanted new directory name
		wxString msg = _("Type the new folder name");
		wxString caption = _("Type Name For Folder (spaces permitted)");
		wxString newName = ::wxGetTextFromUser(msg,caption,oldName,this); // use selection as default name

		// Change to new name? -- use ::wxRenameFile(), but not on open
		// or working directory, and the overwrite parameter must be set TRUE
		bOK = ::wxSetWorkingDirectory(m_strDestFolderPath); // make parent directory be the working one
		wxString theNewPath = m_strDestFolderPath + gpApp->PathSeparator + newName;
		bOK = ::wxRenameFile(theFolderPath,theNewPath,TRUE);
		// update the destination list
		SetupDestList(m_strDestFolderPath);
		if (m_strDestFolderPath == m_strSrcFolderPath)
		{
			::wxSetWorkingDirectory(m_strSrcFolderPath);
			SetupSrcList(m_strSrcFolderPath);
		}
	}
	else if (destSelectedFilesArray.GetCount() == 1)
	{
		wxString theFilePath = m_strDestFolderPath + gpApp->PathSeparator;
		wxString oldName = destSelectedFilesArray.Item(0); // old file name
		theFilePath += oldName; // path to selected file which we want to rename

		// now get the user's wanted new filename
		wxString msg = _("Type the new file name");
		wxString caption = _("Type Name For File (spaces permitted)");
		wxString newName = ::wxGetTextFromUser(msg,caption,oldName,this); // use selection as default name

		// Change to new name? -- use ::wxRenameFile(), but not on open
		// or working directory, and the overwrite parameter must be set TRUE
		bOK = ::wxSetWorkingDirectory(m_strDestFolderPath); // make parent directory be the working one
		wxString theNewPath = m_strDestFolderPath + gpApp->PathSeparator + newName;
		bOK = ::wxRenameFile(theFilePath,theNewPath,TRUE);
		// update the destination list
		SetupDestList(m_strDestFolderPath);
		if (m_strDestFolderPath == m_strSrcFolderPath)
		{
			::wxSetWorkingDirectory(m_strSrcFolderPath);
			SetupSrcList(m_strSrcFolderPath);
		}
	}
	else
	{
		::wxBell();
	}
}


// the MoveOrCopyFilesAndFolders() function is used in both the Move and the Copy button handlers
// for file moves of file copying. Moving is done by copying, and then removing the original
// source files which were copied. Copying does not do the removal step.
void AdminMoveOrCopy::MoveOrCopyFilesAndFolders(wxString srcFolderPath, wxString destFolderPath,
				wxArrayString* pSrcSelectedFoldersArray, wxArrayString* pSrcSelectedFilesArray, 
				bool bDoMove)
{
	// copy the files first, then recurse for copying the one or more folders, if any
	size_t limitSelectedFiles = pSrcSelectedFilesArray->GetCount();
	size_t limitSelectedFolders = pSrcSelectedFoldersArray->GetCount();
	size_t index;
	size_t nItemIndex = 0;
	bool bCopyWasSuccessful = TRUE;
	arrCopiedOK.Clear();

	// the passed in source folder may have no files in it - check, and only do this block
	// if there is at least one file to be copied
	if (limitSelectedFiles > 0)
	{
		arrCopiedOK.Alloc(limitSelectedFiles); // space for a flag for every 
											   // selected filename to have a flag value
		wxString aFilename = pSrcSelectedFilesArray->Item(nItemIndex);
		wxASSERT(!aFilename.IsEmpty());
		// loop across all files that were selected
		do {
			// process one file per iteration until the list of selected filenames is empty
			bCopyWasSuccessful = CopySingleFile(srcFolderPath,destFolderPath,aFilename, 
													m_bUserCancelled);
//			wxLogDebug(_T("\nCopied  %s  ,  folder path = %s "),aFilename, srcFolderPath);
			if (!bCopyWasSuccessful)
			{
				// if the copy did not succeed because the user chose to Cancel when a filename
				// conflict was detected, we want to use the returned TRUE value in
				// m_bUserCancelled to force the parent dialog to close; other failures, however,
				// need a warning to be given to the user
				arrCopiedOK.Add(0); // record the fact that the last file copy did not take place
				if (m_bUserCancelled)
				{
					// force parent dialog to close, and if Move was being attempted, abandon that
					// to without removing anything more at the source side
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

	// If Move was requested, remove each source filename for which there was a successful
	// copy from the srcSelectedFilesArray, and then clear the srcSelectedFilesArray. 
	// Also, for a Move, remove those same files from the src folder
	if (bDoMove && limitSelectedFiles > 0)
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

				// remove the file from the source folder
				bRemovedSuccessfully = ::wxRemoveFile(theSourceFilePath);
				if (!bRemovedSuccessfully)
				{
					// shouldn't ever fail, so an English message for developer will do
					wxString msg;
//					msg = msg.Format(_T("MoveOrCopyFiles: Removing the file ( %s ) for the requested move, failed"),
//							theSourceFilePath.c_str());
					wxMessageBox(msg, _T("Removing a source file after moving it, failed"),wxICON_WARNING);
				} // end block for test: if (!bRemovedSuccessfully)
//				else
//				{
//					wxLogDebug(_T("Removed file  %s  from path  %s"),aFilename, theSourceFilePath);
//				}
			} // end block for test: if (flag)
		} // end for loop block
	}  // end block for test: if (bDoMove)

	// now handle any folder selections, these will be recursive calls within the loop
	if (limitSelectedFolders > 0)
	{
		// there are one or more folders to copy or move -- do it in a loop
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
				// destination directory does not yet exist, so create it (note, on Linux,
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
_("Failed to create the destination directory  %s  for the Copy or Move operation. The parent dialog will now close. Files and folders already copied or moved will remain so. You may need to use your system's file browser to clean up before you try again."),
					destFolderPath2.c_str());
					wxMessageBox(msg, _("Error: could not make directory"), wxICON_WARNING);
					m_bUserCancelled = TRUE; // causes call of EndModal() at top level call
					delete pSrcSelectedFoldersArray2; // don't leak memory
					delete pSrcSelectedFilesArray2; // ditto
					return;
				}
			}
			// The directory has now either been successfully made, or already exists.
            // Now we must call GetFilesOnly() and GetFoldersOnly() in order to gather the
            // source director's folder names and file names into the the arrays provided
			// (Each call internally resets the working directory.) First TRUE is bool
			// bSort, second TRUE is bool bSuppressMessage.
            bool bHasFolders = GetFoldersOnly(srcFolderPath2, pSrcSelectedFoldersArray2, TRUE, TRUE);
			bool bHasFiles = GetFilesOnly(srcFolderPath2, pSrcSelectedFilesArray2, TRUE, TRUE);
			bHasFolders = bHasFolders; // avoid compiler warning
			bHasFiles = bHasFiles; // avoid compiler warning

			// Reenter
//			wxLogDebug(_T("Re-entering: srcPath = %s , destPath = %s, hasFolders %d , hasFiles %d"),
//				srcFolderPath2, destFolderPath2, (int)bHasFolders, (int)bHasFiles);
			MoveOrCopyFilesAndFolders(srcFolderPath2, destFolderPath2, pSrcSelectedFoldersArray2,
					pSrcSelectedFilesArray2, bDoMove);

			// if Move was requested, the return of the above call will indicate that
			// srcFolderPath2 might have been emptied of both files and folders, so try remove the
			// possibly empty source folder -- if will be removed only if empty
			if (bDoMove)
			{
				// the directory in srcFolderPath will be currently set as the working
				// directory, and that prevents ::wxRmdir() from removing it, so first
				// reset the working directory to its parent, and the removals will work
				bool bOK = ::wxSetWorkingDirectory(srcFolderPath);
				bOK = ::wxRmdir(srcFolderPath2);
				// ::wxRmdir() will fail if it still contains a file or folder - this can
				// happen if there was a file conflict and the user elected to do noCopy,
				// which means the conflicting file remains in the source directory. We
				// don't want to have an annoying message put up for each such 'failure'
				// so just ignore the return value bOK
				/*
				if (!bOK)
				{
					wxString msg;
					msg = msg.Format(_T("::Rmdir() failed to remove directory %s "),srcFolderPath2.c_str());
					wxMessageBox(msg,_T("Couldn't remove directory"),wxICON_WARNING);
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
			delete pSrcSelectedFilesArray2; // don't leak it
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
	}
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
/// Handler for the "Copy" button. It copies the one or more selected files
/// in the source folder's wxListCtrl to the destination folder specified by the path
/// m_strDestFolderPath; and recursively copies files and folders of any child folders.
///  File name conflicts are handled internally, similarly to how Windows Explorer handles
///  them (i.e. with a child dialog opening to display file attribute details such as name,
///  size and last modification date & time for each of the files in conflict) and the user
///  has 3 options - continue, overwriting the file at the destination folder with the one
///  from the source folder; or to not copy; or to have the name of the source folder's
///  file changed so as to remove the conflict and then the copy goes ahead - resulting in
///  two files with same or similar data being in the destination folder. Cancelling, and
///  opting to have all subsequent filename conflicts handled the same way as the current
///  one are also supported from the child dialog.
////////////////////////////////////////////////////////////////////////////////////////////
void AdminMoveOrCopy::OnBnClickedCopy(wxCommandEvent& WXUNUSED(event))
{
	// do nothing if the destination folder is not yet defined
	if (m_strDestFolderPath.IsEmpty())
	{
		wxMessageBox(
_("No destination folder is defined. Use the button 'Locate the destination folder' to first set a destination, then try again."),
		_("Cannot move or copy"), wxICON_WARNING);
		return;
	}
	if(CheckForIdenticalPaths(m_strSrcFolderPath, m_strDestFolderPath))
	{
		// identical paths, so bail out; CheckForIdenticalPaths() has put up a warning message
		return;
	}

	// on the heap, create the arrays of files and folders which are to be copied & populate them
	wxArrayString* pSrcSelectedFoldersArray	= new wxArrayString;
	wxArrayString* pSrcSelectedFilesArray	= new wxArrayString;
	size_t index;
	size_t foldersLimit = srcSelectedFoldersArray.GetCount(); // RHS populated in SetSelectionArray() call earlier
	size_t filesLimit = srcSelectedFilesArray.GetCount(); // RHS populated in SetSelectionArray() call earlier
	if (srcSelectedFilesArray.GetCount() == 0 && srcSelectedFoldersArray.GetCount() == 0)
	{
		wxMessageBox(
_("You first need to select at least one item in the left list before clicking the Copy button"),
		_("No Files Or Folders Selected"),wxICON_WARNING);
		pSrcSelectedFilesArray->Clear(); // this one is on heap
		pSrcSelectedFoldersArray->Clear(); // this one is on heap
		delete pSrcSelectedFilesArray; // don't leak it
		delete pSrcSelectedFoldersArray; // don't leak it
		return;
	}
	for (index = 0; index < foldersLimit; index++)
	{
		pSrcSelectedFoldersArray->Add(srcSelectedFoldersArray.Item(index));
	}
	for (index = 0; index < filesLimit; index++)
	{
		pSrcSelectedFilesArray->Add(srcSelectedFilesArray.Item(index));
	}

	// initialize bail out flag, and boolean for doing it the same way henceforth,
	// and set the default lastWay value to noCopy
	m_bUserCancelled = FALSE;
	m_bDoTheSameWay = FALSE;
	lastWay = noCopy; // a safe default (for whatever way the last filename conflict, 
						  // if there was one, was handled (see CopyAction enum for values)

	MoveOrCopyFilesAndFolders(m_strSrcFolderPath, m_strDestFolderPath, pSrcSelectedFoldersArray,
					pSrcSelectedFilesArray, FALSE); // FALSE is  bool bDoMove

	// clear the allocations to the heap
	pSrcSelectedFilesArray->Clear(); // this one is on heap
	pSrcSelectedFoldersArray->Clear(); // this one is on heap
	delete pSrcSelectedFilesArray; // don't leak it
	delete pSrcSelectedFoldersArray; // don't leak it

	// if the user asked for a cancel from the file conflict dialog, it cancels the parent
	// dlg too
	if (m_bUserCancelled)
	{
		// force parent dialog to close, and if Move was being attempted, abandon that
		// to without removing anything more at the source side
		EndModal(wxID_OK);
		return;
	}

	// Note: a bug in SetItemState() which DeselectSelectedFiles() call uses causes the
	// m_nCount member of wxArrayString for srcSelectedFilesArray() to be overwritten to
	// have value 1, do after emptying here, a further empty is required further below
	srcSelectedFilesArray.Clear(); // the member wxArrayString variable, for top level
	DeselectSelectedFiles(sourceSide); // loops through wxListCtrl clearing selection
									   // for each item which has it set

	// clear the source side's array of filename selections (put it here because the
	// SetItemState() call implicit in DeselectSelectedFiles() causes the emptied
	// srcSelectedFilesArray to become non-empty (its m_nCount becomes 1, even though the
	// AdminMoveOrCopy class and its members is on main thread and the wxListCtrl, or
	// wxListView class, is on a different work thread). I can just do a unilateral call
	// above of SetupSrcList() to get round this problem, but that's wasteful
	srcSelectedFilesArray.Clear(); // DO NOT DELETE THIS CALL HERE (even though its above)

	// update the destination list
	SetupDestList(m_strDestFolderPath);
}

void AdminMoveOrCopy::OnBnClickedMove(wxCommandEvent& WXUNUSED(event))
{
	// this works, but I can't get ::Rmdir() to work below
	//if (TRUE)
	//{
	//	bool bRemovedOK = ::wxRmdir(_T("C:\\Card4\\emptyfolder"));
	//	return;
	//}
	// do nothing if the destination folder is not yet defined
	if (m_strDestFolderPath.IsEmpty())
	{
		wxMessageBox(
_("No destination folder is defined. Use the button 'Locate the destination folder' to first set a destination, then try again."),
		_("Cannot move or copy"), wxICON_WARNING);
		return;
	}
	if(CheckForIdenticalPaths(m_strSrcFolderPath, m_strDestFolderPath))
	{
		// identical paths, so bail out; CheckForIdenticalPaths() has put up a warning message
		return;
	}

	// on the heap, create the arrays of files and folders which are to be copied & populate them
	wxArrayString* pSrcSelectedFoldersArray	= new wxArrayString;
	wxArrayString* pSrcSelectedFilesArray	= new wxArrayString;
	size_t index;
	size_t foldersLimit = srcSelectedFoldersArray.GetCount(); // RHS populated in SetSelectionArray() call earlier
	size_t filesLimit = srcSelectedFilesArray.GetCount(); // RHS populated in SetSelectionArray() call earlier
	if (srcSelectedFilesArray.GetCount() == 0 && srcSelectedFoldersArray.GetCount() == 0)
	{
		wxMessageBox(
_("You first need to select at least one item in the left list before clicking the Move button"),
		_("No Files Or Folders Selected"),wxICON_WARNING);
		pSrcSelectedFilesArray->Clear(); // this one is on heap
		pSrcSelectedFoldersArray->Clear(); // this one is on heap
		delete pSrcSelectedFilesArray; // don't leak it
		delete pSrcSelectedFoldersArray; // don't leak it
		return;
	}
	for (index = 0; index < foldersLimit; index++)
	{
		pSrcSelectedFoldersArray->Add(srcSelectedFoldersArray.Item(index));
	}
	for (index = 0; index < filesLimit; index++)
	{
		pSrcSelectedFilesArray->Add(srcSelectedFilesArray.Item(index));
	}

	// initialize bail out flag, and boolean for doing it the same way henceforth,
	// and set the default lastWay value to noCopy
	m_bUserCancelled = FALSE;
	m_bDoTheSameWay = FALSE;
	lastWay = noCopy; // a safe default (for whatever way the last filename conflict, 
						  // if there was one, was handled (see CopyAction enum for values)

	MoveOrCopyFilesAndFolders(m_strSrcFolderPath, m_strDestFolderPath, pSrcSelectedFoldersArray,
					pSrcSelectedFilesArray); // last param  bool bDoMove is TRUE

	// clear the allocations to the heap
	pSrcSelectedFilesArray->Clear(); // this one is on heap
	pSrcSelectedFoldersArray->Clear(); // this one is on heap
	delete pSrcSelectedFilesArray; // don't leak it
	delete pSrcSelectedFoldersArray; // don't leak it

	// if the user asked for a cancel from the file conflict dialog, it cancels the parent
	// dlg too
	if (m_bUserCancelled)
	{
		// force parent dialog to close, and if Move was being attempted, abandon that
		// to without removing anything more at the source side
		EndModal(wxID_OK);
		return;
	}

	// Note: a bug in SetItemState() which DeselectSelectedFiles() call uses causes the
	// m_nCount member of wxArrayString for srcSelectedFilesArray() to be overwritten to
	// have value 1, do after emptying here, a further empty is required further below
	srcSelectedFilesArray.Clear(); // the member wxArrayString variable, for top level
	DeselectSelectedFiles(sourceSide); // loops through wxListCtrl clearing selection
									   // for each item which has it set

	// clear the source side's array of filename selections (put it here because the
	// SetItemState() call implicit in DeselectSelectedFiles() causes the emptied
	// srcSelectedFilesArray to become non-empty (its m_nCount becomes 1, even though the
	// AdminMoveOrCopy class and its members is on main thread and the wxListCtrl, or
	// wxListView class, is on a different work thread). I can just do a unilateral call
	// above of SetupSrcList() to get round this problem, but that's wasteful
	srcSelectedFilesArray.Clear(); // DO NOT DELETE THIS CALL HERE (even though its above)

	// update the destination list and the source list
	SetupDestList(m_strDestFolderPath);
	SetupSrcList(m_strSrcFolderPath);
}



/* FIRST VERSION FUNCTIONS




	EVT_BUTTON(ID_BUTTON_COPY_FILES, AdminMoveOrCopy::OnCopyFileOrFiles)
	EVT_BUTTON(ID_BUTTON_MOVE_FILES, AdminMoveOrCopy::OnMoveFileOrFiles)
	EVT_BUTTON(ID_BUTTON_DELETE_DEST_FILES, AdminMoveOrCopy::OnBnClickedDeleteDestFiles)
	EVT_BUTTON(ID_BUTTON_DELETE_DEST_FOLDER, AdminMoveOrCopy::OnBnClickedDeleteDestFolder)
	EVT_BUTTON(ID_BUTTON_RENAME_DEST_FILE, AdminMoveOrCopy::OnBnClickedRenameDestFile)
	EVT_BUTTON(ID_BUTTON_RENAME_DEST_FOLDER, AdminMoveOrCopy::OnBnClickedRenameDestFolder)
	EVT_BUTTON(ID_BUTTON_MOVE_FOLDER, AdminMoveOrCopy::OnBnClickedMoveFolder)
	EVT_BUTTON(ID_BUTTON_COPY_FOLDER, AdminMoveOrCopy::OnBnClickedCopySrcFolder)


// *** OBSOLETE FUNCTIONS *****



////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxCommandEvent fired by the user's button press
/// \remarks
/// Handler for the "Copy File Or Files" button. It copies the one or more selected files
/// in the source folder's wxListCtrl to the destination folder specified by the path
/// m_strDestFolderPath. File name conflicts are handled internally, similarly to how Windows
/// Explorer handles them (i.e. with a child dialog opening to display file attribute
/// details such as name, size and last modification date & time for each of the files in
/// conflict) and the user has 3 options - continue, overwriting the file at the
/// destination folder with the one from the source folder; or to not copy; or to have the
/// name of the source folder's file changed so as to remove the conflict and then the
/// copy goes ahead - resulting in two files with same or similar data being in the
/// destination folder. Cancelling, and opting to have all subsequent filename conflicts
/// handled the same way as the current one are also supported from the child dialog.
////////////////////////////////////////////////////////////////////////////////////////////
void AdminMoveOrCopy::OnCopyFileOrFiles(wxCommandEvent& WXUNUSED(event)) 
{	
	MoveOrCopyFileOrFiles(FALSE); // FALSE is  bool bDoMove
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxCommandEvent fired by the user's button press
/// \remarks
/// Handler for the "Move File Or Files" button. It copies the one or more selected files
/// in the source folder's wxListCtrl to the destination folder specified by the path
/// m_strDestFolderPath, and deletes the original in the source location. 
/// File name conflicts are handled internally, similarly to how Windows
/// Explorer handles them (i.e. with a child dialog opening to display file attribute
/// details such as name, size and last modification date & time for each of the files in
/// conflict) and the user has 3 options - continue, overwriting the file at the
/// destination folder with the one from the source folder; or to not copy; or to have the
/// name of the source folder's file changed so as to remove the conflict and then the
/// copy goes ahead - resulting in two files with same or similar data being in the
/// destination folder. Cancelling, and opting to have all subsequent filename conflicts
/// handled the same way as the current one are also supported from the child dialog.
////////////////////////////////////////////////////////////////////////////////////////////
void AdminMoveOrCopy::OnMoveFileOrFiles(wxCommandEvent& WXUNUSED(event)) 
{	
	MoveOrCopyFileOrFiles(); // parameter  bool bDoMove is default TRUE
}


// the MoveOrCopyFileOrFiles() function is used in both the Move and the Copy button handlers
// for file moves of file copying. Moving is done by copying, and then removing the original
// source files which were copied. Copying does not do the removal step.
void AdminMoveOrCopy::MoveOrCopyFileOrFiles(bool bDoMove)
{
	if (srcSelectedFilesArray.GetCount() == 0)
	{
		::wxBell();
		wxMessageBox(_T("You first need to select at least one file in the left list before clicking the copy button"),_T("No File Selected"),wxICON_WARNING);
		return;
	}

	if(CheckForIdenticalPaths(m_strSrcFolderPath, m_strDestFolderPath))
	{
		// identical paths, so bail out; CheckForIdenticalPaths() has put up a warning message
		return;
	}

	size_t limitSelected = srcSelectedFilesArray.GetCount();
	size_t index;
	arrCopiedOK.Clear();
	arrCopiedOK.Alloc(limitSelected); // space for a flag for every 
									  // selected filename to have a flag value
	size_t nItemIndex = 0;
	m_bCopyWasSuccessful = TRUE;
	
	wxString aFilename = srcSelectedFilesArray.Item(nItemIndex);
	wxASSERT(!aFilename.IsEmpty());
	m_bUserCancelled = FALSE;
	m_bDoTheSameWay = FALSE;
	lastWay = noCopy; // a safe default (for whatever way the last filename conflict, 
					  // if there was one, was handled (see CopyAction enum for values)
	// loop across all files that were selected
	do {
		// process one file per iteration until the list of selected filenames is empty
		m_bCopyWasSuccessful = CopySingleFile(m_strSrcFolderPath,m_strDestFolderPath,aFilename, 
												m_bUserCancelled);
		if (!m_bCopyWasSuccessful)
		{
            // if the copy did not succeed because the user chose to Cancel when a filename
            // conflict was detected, we want to use the returned TRUE value in
			// m_bUserCancelled to force the parent dialog to close; other failures, however,
			// need a warning to be given to the user
			arrCopiedOK.Add(0); // record the fact that the copy did not take place
			if (m_bNoDestPathYet)
			{
				// a warning has been seen from CopySingleFile(), so just return
				m_bNoDestPathYet = FALSE; // restore default value
				return;
			}
			if (m_bUserCancelled)
			{
				// force parent dialog to close, and if Move was being attempted, abandon that
				// to without removing anything more at the source side
				EndModal(wxID_OK);
				return;
			}
		}
		else
		{
			arrCopiedOK.Add(1); // record the fact that the copy took place
		}

		// prepare for iteration, a new value for aFilename is required
		nItemIndex++;
		if (nItemIndex < limitSelected)
			aFilename = srcSelectedFilesArray.Item(nItemIndex);

	} while (nItemIndex < limitSelected);


	// If Move was requested, remove each source filename for which there was a successful
	// copy from the srcSelectedFilesArray, and then clear the srcSelectedFilesArray. 
	// Also, for a Move, remove those same files from the src folder
	if (bDoMove)
	{
		int flag; // will be 1 or 0, for each selected filename
		wxString aFilename;
		wxString theSourceFilePath;
		bool bRemovedSuccessfully;
		for (index = 0; index < limitSelected; index++)
		{
			flag = arrCopiedOK.Item(index); // get whether or not it was successfully copied
			if (flag)
			{
				aFilename = srcSelectedFilesArray.Item(index); // get the filename
				// make the path to it
				theSourceFilePath = m_strSrcFolderPath + gpApp->PathSeparator + aFilename;

				// remove the file from the source folder, this completes the move request
				// for this file
				bRemovedSuccessfully = ::wxRemoveFile(theSourceFilePath);
				if (!bRemovedSuccessfully)
				{
					// shouldn't ever fail, so an English message for developer will do
					wxString msg;
					msg = msg.Format(_T("MoveOrCopyFileOrFiles: Removing the file ( %s ) for the requested move, failed"),
							theSourceFilePath.c_str());
					wxMessageBox(msg, _T("Removing a source file after moving it, failed"),wxICON_WARNING);
				} // end block for test: if (!bRemovedSuccessfully)
			} // end block for test: if (flag)
		} // end for loop block
	}  // end block for test: if (bDoMove)
	
	//unsigned int i;
	//for (i=0; i < srcSelectedFilesArray.GetCount(); i++)
	//{
	//	wxString fn = srcSelectedFilesArray.Item(i);
	//	wxLogDebug(_T("Files Before:  %s   at index:  %d  for total of %d\n"), fn.c_str(),i, srcSelectedFilesArray.GetCount());
	//}
	
	// Note: a bug in SetItemState() which DeselectSelectedFiles() call uses causes the
	// m_nCount member of wxArrayString for srcSelectedFilesArray() to be overwritten to
	// have value 1, do after emptying here, a further empty is required further below
	srcSelectedFilesArray.Empty();

	// update the destination folder's list to show what has been copied or moved there
	SetupDestList(m_strDestFolderPath);

    // update the source folder's list to show what remains after a move; for a copy, we
    // just need to deselect any selected files which may remain in the source list
	if (bDoMove)
	{
		SetupSrcList(m_strSrcFolderPath); // update its contents, clears any selections too
	}
	else
	{
		DeselectSelectedFiles(sourceSide);
	}

	// clear the source side's array of filename selections (put it here because the
	// SetItemState() call implicit in DeselectSelectedFiles() causes the emptied
	// srcSelectedFilesArray to become non-empty (its m_nCount becomes 1, even though the
	// AdminMoveOrCopy class and its members is on main thread and the wxListCtrl, or
	// wxListView class, is on a different work thread). I can just do a unilateral call
	// above of SetupSrcList() to get round this problem, but that's wasteful
	srcSelectedFilesArray.Empty(); // DO NOT DELETE THIS CALL HERE (even though its above)

	// disable the Copy File Or Files button, and other relevant buttons
	wxASSERT(srcSelectedFilesArray.GetCount() == 0);
}


void AdminMoveOrCopy::SetupSelectedFilesArray(enum whichSide side)
{
	int index = 0;
	int limit = 0;
	long stateMask = 0;
	stateMask |= wxLIST_STATE_SELECTED;
	int isSelected = 0;
	if (side == sourceSide)
	{
		srcSelectedFilesArray.Empty(); // clear, we'll refill in the loop
		index = srcFoldersCount; // index of first file in the wxListCtrl
		limit = pSrcList->GetItemCount(); // index of list file is (limit - 1)
		if (srcFilesCount == 0 || pSrcList->GetSelectedItemCount() == 0)
		{
			EnableCopyFileOrFilesButton(FALSE);
			EnableMoveFileOrFilesButton(FALSE);
			if (!m_strSrcFolderPath.IsEmpty() && !m_strDestFolderPath.IsEmpty())
			{
				EnableCopyFolderButton(TRUE);
				EnableMoveFolderButton(TRUE);
			}
			else
			{
				EnableCopyFolderButton(FALSE);
				EnableMoveFolderButton(FALSE);
			}
			return; // nothing to do
		}
		srcSelectedFilesArray.Clear();
		srcSelectedFilesArray.Alloc(srcFilesCount); // enough for all files in the folder
		for (index = srcFoldersCount; index < limit; index++)
		{
			isSelected = pSrcList->GetItemState(index,stateMask);
			if (isSelected)
			{
				// this one is selected, add it's name to the list of selected files
				wxString filename = pSrcList->GetItemText(index);
				size_t itsIndex = srcSelectedFilesArray.Add(filename); // return is unused
				itsIndex = itsIndex; // avoid compiler warning
				isSelected = 0;
			}
		}
		if (!m_strSrcFolderPath.IsEmpty() && !m_strDestFolderPath.IsEmpty())
		{
			EnableCopyFolderButton(TRUE);
			EnableMoveFolderButton(TRUE);
		}
		else
		{
			EnableCopyFolderButton(FALSE);
			EnableMoveFolderButton(FALSE);
		}
		if (srcSelectedFilesArray.GetCount() > 0)
		{
			EnableCopyFileOrFilesButton(TRUE);
			EnableMoveFileOrFilesButton(TRUE);
		}
		else
		{
			// must be 0, so disable relevant buttons
			EnableCopyFileOrFilesButton(FALSE);
			EnableMoveFileOrFilesButton(FALSE);
			EnableCopyFolderButton(FALSE);
			EnableMoveFolderButton(FALSE);
		}
	}
	else
	{
		destSelectedFilesArray.Empty(); // clear, we'll refill in the loop
		index = destFoldersCount; // index of first file in the wxListCtrl
		limit = pDestList->GetItemCount(); // index of list file is (limit - 1)
		if (destFilesCount == 0 || pDestList->GetSelectedItemCount() == 0)
		{
			EnableDeleteDestFileOrFilesButton(FALSE);
			EnableRenameDestFileButton(FALSE);
			if (m_strDestFolderPath.IsEmpty())
			{
				EnableDeleteDestFolderButton(FALSE);
				EnableRenameDestFolderButton(FALSE);
			}
			else
			{
				EnableDeleteDestFolderButton(FALSE);
				EnableRenameDestFolderButton(TRUE);
			}
			return; // nothing to do
		}
		destSelectedFilesArray.Clear();
		destSelectedFilesArray.Alloc(destFilesCount); // enough for all files in the folder
		for (index = destFoldersCount; index < limit; index++)
		{
			isSelected = pDestList->GetItemState(index,stateMask);
			if (isSelected)
			{
				// this one is selected, add it's name to the list of selected files
				wxString filename = pDestList->GetItemText(index);
				size_t itsIndex = destSelectedFilesArray.Add(filename); // return is unused
				itsIndex = itsIndex; // avoid compiler warning
				isSelected = 0;
			}
		}
		if (m_strDestFolderPath.IsEmpty())
		{
			EnableDeleteDestFolderButton(FALSE);
			EnableRenameDestFolderButton(FALSE);
		}
		else
		{
			EnableDeleteDestFolderButton(FALSE);
			EnableRenameDestFolderButton(TRUE);
		}
		if (destSelectedFilesArray.GetCount() == 1)
		{
			EnableRenameDestFileButton(TRUE);
			EnableDeleteDestFileOrFilesButton(TRUE);
		}
		if (destSelectedFilesArray.GetCount() == 0 ||
			destSelectedFilesArray.GetCount() > 1)
		{
			if (destSelectedFilesArray.GetCount() == 0)
			{
				EnableDeleteDestFileOrFilesButton(FALSE);
			}
			else
			{
				EnableDeleteDestFileOrFilesButton(TRUE);
			}
			EnableRenameDestFileButton(FALSE);
		}
	}
}


void AdminMoveOrCopy::OnBnClickedDeleteDestFiles(wxCommandEvent& WXUNUSED(event))
{
	size_t index;
	size_t limitSelected = destSelectedFilesArray.GetCount();
	wxString aFilename;
	wxString theDestFilePath;
	bool bRemovedSuccessfully;
	for (index = 0; index < limitSelected; index++)
	{
		aFilename = destSelectedFilesArray.Item(index); // get the filename
		// make the path to it
		theDestFilePath = m_strDestFolderPath + gpApp->PathSeparator + aFilename;

		// remove the file from the destination folder
		bRemovedSuccessfully = ::wxRemoveFile(theDestFilePath);
		if (!bRemovedSuccessfully)
		{
			// shouldn't ever fail, so an English message for developer will do
			wxString msg;
			msg = msg.Format(_T("Delete File or Files: Deleting the file  %s  failed"),
					theDestFilePath.c_str());
			wxMessageBox(msg, _T("Deleting a file failed"),wxICON_WARNING);
		} // end block for test: if (!bRemovedSuccessfully)
	} // end for loop block
	// update the destination list
	SetupDestList(m_strDestFolderPath);
}


void AdminMoveOrCopy::OnBnClickedRenameDestFile(wxCommandEvent& WXUNUSED(event))
{
	wxString theFilePath;
	wxString theNewPath;
	wxString aFilename = destSelectedFilesArray.Item(0);
	//wxString defaultName = _T("");
	theFilePath = m_strDestFolderPath + gpApp->PathSeparator + aFilename;
	wxString msg = _("Type the new filename - including its filename extension (if required)");
	wxString caption = _("Type Filename (spaces permitted)");
	wxString newName = ::wxGetTextFromUser(msg,caption,aFilename,this); // use selection as default name
	// make the path to it
	if (newName.IsEmpty())
	{
		DeselectSelectedFiles(destinationSide);
	}
	else
	{
		theNewPath = m_strDestFolderPath + gpApp->PathSeparator + newName;
		bool bOK = ::wxRenameFile(theFilePath,theNewPath,FALSE);

		// update the destination list
		if (bOK)
			SetupDestList(m_strDestFolderPath);
		else
			DeselectSelectedFiles(destinationSide);
	}
}


*/


