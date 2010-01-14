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
	EVT_BUTTON(ID_BUTTON_COPY_FILES, AdminMoveOrCopy::OnCopyFileOrFiles)
	EVT_BUTTON(ID_BUTTON_MOVE_FILES, AdminMoveOrCopy::OnMoveFileOrFiles)
	EVT_BUTTON(ID_BUTTON_DELETE_DEST_FILES, AdminMoveOrCopy::OnBnClickedDeleteDestFiles)
	EVT_BUTTON(ID_BUTTON_DELETE_DEST_FOLDER, AdminMoveOrCopy::OnBnClickedDeleteDestFolder)
	EVT_BUTTON(ID_BUTTON_RENAME_DEST_FILE, AdminMoveOrCopy::OnBnClickedRenameDestFile)
	EVT_BUTTON(ID_BUTTON_RENAME_DEST_FOLDER, AdminMoveOrCopy::OnBnClickedRenameDestFolder)
	EVT_BUTTON(ID_BUTTON_COPY_FOLDER, AdminMoveOrCopy::OnBnClickedCopySrcFolder)
	EVT_BUTTON(ID_BUTTON_MOVE_FOLDER, AdminMoveOrCopy::OnBnClickedMoveSrcFolder)
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
	srcSelectedFilesArray.Empty();
	destSelectedFilesArray.Empty();
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
	srcSelectedFilesArray.Clear();
	destSelectedFilesArray.Clear();
	arrCopiedOK.Clear();
}

void AdminMoveOrCopy::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
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
	m_bNoDestPathYet = FALSE;

	m_bUserCancelled = FALSE;
	m_bCopyWasSuccessful = FALSE;
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

	pMoveFolderButton = (wxButton*)FindWindowById(ID_BUTTON_MOVE_FOLDER);
	pMoveFileOrFilesButton = (wxButton*)FindWindowById(ID_BUTTON_MOVE_FILES);
	pCopyFolderButton = (wxButton*)FindWindowById(ID_BUTTON_COPY_FOLDER);
	pCopyFileOrFilesButton = (wxButton*)FindWindowById(ID_BUTTON_COPY_FILES);
	pDeleteDestFileOrFilesButton = (wxButton*)FindWindowById(ID_BUTTON_DELETE_DEST_FILES);
	pDeleteDestFolderButton = (wxButton*)FindWindowById(ID_BUTTON_DELETE_DEST_FOLDER);
	pRenameDestFileButton = (wxButton*)FindWindowById(ID_BUTTON_RENAME_DEST_FILE);
	pRenameDestFolderButton = (wxButton*)FindWindowById(ID_BUTTON_RENAME_DEST_FOLDER);

	// start with lower buttons disabled (they rely on selections to become enabled)
	EnableCopyFileOrFilesButton(FALSE);
	EnableMoveFileOrFilesButton(FALSE);
	EnableDeleteDestFileOrFilesButton(FALSE);
	EnableDeleteDestFolderButton(FALSE);
	EnableRenameDestFileButton(FALSE);
	EnableRenameDestFolderButton(FALSE);
	EnableCopyFolderButton(FALSE);
	EnableMoveFolderButton(FALSE);


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
	SetupSelectedFilesArray(sourceSide);

	m_strDestFolderPath = _T(""); // start with no path defined
	SetupDestList(m_strDestFolderPath);
	SetupSelectedFilesArray(destinationSide);

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

/* 
// I did two versions of this, one using wxListCtrl, another using the subclass wxListView
// and both failed in the same way - see the comments below; it's a widgets problem, not
// mine; the version using wxListView is simpler so I'll use that, but it has the
// SetItemState() call implicit in its Selection() function, whereas I call it explicitly
// in this first version now commented out
void AdminMoveOrCopy::DeselectSelectedFiles(enum whichSide side)
{
	int index = 0;
	int limit = 0;
	long stateMask = 0;
	stateMask |= wxLIST_STATE_SELECTED;
	stateMask |= wxLIST_STATE_FOCUSED; // not necessary, but the wiki.wxWidgets site has it
	int isSelected = 0;
	bool bSetOK = 0;
	if (side == sourceSide)
	{
		index = srcFoldersCount; // index of first file in the wxListCtrl
		limit = pSrcList->GetItemCount(); // index of last file is (limit - 1)
		if (srcFilesCount == 0 || pSrcList->GetSelectedItemCount() == 0)
		{
			return; // nothing to do
		}
		for (index = srcFoldersCount; index < limit; index++)
		{
			isSelected = pSrcList->GetItemState(index,stateMask);
			if (isSelected)
			{
				// this one is selected, so deselect it
unsigned int aCount = srcSelectedFilesArray.GetCount(); // for debugging, the next call is spuriously giving empty srcSelectedFileArray() which is on another thread (the main thread) a m_nCount value of 1, if two or more files were selected for copying
				
				bSetOK = pSrcList->SetItemState(index,0,stateMask); // stepping thru this does nothing which should interfer with Main thread

aCount = srcSelectedFilesArray.GetCount(); // this goes to aCount = 1 after the above call!!! (even though its not on wxListCtrl's Worker Thread!!!)
				isSelected = 0;
			}
		}
	}
	else
	{
		index = destFoldersCount; // index of first file in the wxListCtrl
		limit = pDestList->GetItemCount(); // index of list file is (limit - 1)
		if (destFilesCount == 0 || pDestList->GetSelectedItemCount() == 0)
		{
			return; // nothing to do
		}
		for (index = destFoldersCount; index < limit; index++)
		{
			isSelected = pDestList->GetItemState(index,stateMask);
			if (isSelected)
			{
				// this one is selected, so deselect it
				bSetOK = pDestList->SetItemState(index,0,stateMask);
				isSelected = 0;
			}
		}
	}
}
*/

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
			EnableDeleteDestFolderButton(FALSE);
			EnableRenameDestFileButton(FALSE);
			EnableRenameDestFolderButton(FALSE);
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
		if (destSelectedFilesArray.GetCount() == 1)
		{
			EnableDeleteDestFolderButton(TRUE);
			EnableRenameDestFileButton(TRUE);
			EnableRenameDestFolderButton(TRUE);
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
			EnableDeleteDestFolderButton(FALSE);
			EnableRenameDestFileButton(FALSE);
			EnableRenameDestFolderButton(FALSE);
		}
	}
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

		// the read-only protection mechanism's lock file is not to be moved or copied
		// so check for its presence in the array and remove it if it is there (there will
		// ever only be one such filename, or none)
		size_t count = srcFilesArray.GetCount();
		size_t index;
		for (index = 0; index < count; index++)
		{
			wxString filename = srcFilesArray.Item(index);
			if (IsReadOnlyProtection_LockFile(filename))
			{
				// we found a lock file's filename, so remove it
				srcFilesArray.RemoveAt(index);
				if (srcFilesArray.GetCount() == 0)
					bHasFiles = FALSE;
				break;
			}
		}
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

		// the read-only protection mechanism's lock file is not to be seen in the
		// destination side, otherwise the user could misunderstand its purpose and delete it...
		// so check for its presence in the array and remove it if it is there (there will
		// ever only be one such filename, or none)
		size_t count = destFilesArray.GetCount();
		size_t index;
		for (index = 0; index < count; index++)
		{
			wxString filename = destFilesArray.Item(index);
			if (IsReadOnlyProtection_LockFile(filename))
			{
				// we found a lock file's filename, so remove it
				destFilesArray.RemoveAt(index);
				if (destFilesArray.GetCount() == 0)
					bHasFiles = FALSE;
				break;
			}
		}
	}

	// debugging -- display what we got for source side & destination side too
	/*
#ifdef _DEBUG
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


// event handling functions

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
		// Disable the move and copy buttons at the bottom
		pMoveFolderButton->Enable(FALSE);
		pMoveFileOrFilesButton->Enable(FALSE);
		pCopyFolderButton->Enable(FALSE);
		pCopyFileOrFilesButton->Enable(FALSE);
		srcFoldersCount = 0;
		srcFilesCount = 0;

		// now try put the lines of data in the list; first folders, then files
		int index = 0;
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
	}
	else
	{
        // no files or folders, so disable the move and copy buttons at the bottom, and put
		// a "The folder is empty" message into the list with an empty jug icon, because
		// without an explicit icon, the icon in the list with index = 0 gets shown, and
		// that is the folder icon - which would be confusing, as it would suggest a
		// folder was found with the name "The folder is empty".
        //wxListItem colInfo;
        //bool bGotColInfoOK = pSrcList->GetColumn(0,colInfo);
		//bGotColInfoOK = bGotColInfoOK;
		rv = pSrcList->InsertItem(0, emptyFolderMessage,indxEmptyIcon);
		srcFoldersCount = 0;
		srcFilesCount = 0;

		pMoveFolderButton->Disable();
		pMoveFileOrFilesButton->Disable();
		pCopyFolderButton->Disable();
		pCopyFileOrFilesButton->Disable();
	}
}

void AdminMoveOrCopy::SetupDestList(wxString& folderPath)
{
	/*
	if (folderPath.IsEmpty())
	{
		rv = pDestList->InsertItem(0, emptyFolderMessage,indxEmptyIcon);
		destFoldersCount = 0;
		destFilesCount = 0;
		return;
	}
	*/
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
		pDeleteDestFileOrFilesButton->Enable(FALSE);

		destFoldersCount = 0;
		destFilesCount = 0;

		// now try put the lines of data in the list; first folders, then files
		int index = 0;
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
	EnableDeleteDestFileOrFilesButton(FALSE);
	EnableDeleteDestFolderButton(FALSE);
	EnableRenameDestFileButton(FALSE);
	EnableRenameDestFolderButton(FALSE);
}


void AdminMoveOrCopy::OnBnClickedLocateSrcFolder(wxCommandEvent& WXUNUSED(event))
{
	CMainFrame* pFrame = gpApp->GetMainFrame();
	wxString msg = _("Locate the source folder");
	//long style = wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST | wxDD_CHANGE_DIR;
		// second param suppresses a Create button being shown, 3rd makes chose directory 
		// the working directory, first param is for default dialog style with resizable
		// border (see wxDirDialog for details)
	long style = wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST | wxDD_CHANGE_DIR;
	wxPoint pos = wxDefaultPosition;
	// in the following call, which is a wx widget which can be called as below or as
	// ::wxDirSelector(...params...), if the user cancels from the browser window the
	// returned string is empty, otherwise it is the absolute path to whatever directory
	// was shown selected in the folder hierarchy when the OK button was pressed
	m_strSrcFolderPath = wxDirSelector(msg,m_strSrcFolderPath,style,pos,(wxWindow*)pFrame);
	SetupSrcList(m_strSrcFolderPath);
	EnableCopyFileOrFilesButton(FALSE); // copy button starts off disabled until a file or
										// files are selected
	EnableMoveFileOrFilesButton(FALSE); // ditto for move button, etc
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
}

void AdminMoveOrCopy::OnBnClickedLocateDestFolder(wxCommandEvent& WXUNUSED(event))
{
	CMainFrame* pFrame = gpApp->GetMainFrame();
	wxString msg = _("Locate the destination folder");
	//long style = wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST | wxDD_CHANGE_DIR;
		// second param suppresses a Create button being shown, 3rd makes chose directory 
		// the working directory, first param is for default dialog style with resizable
		// border (see wxDirDialog for details)
	long style = wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST | wxDD_CHANGE_DIR;
	wxPoint pos = wxDefaultPosition;
	// in the following call, which is a wx widget which can be called as below or as
	// ::wxDirSelector(...params...), if the user cancels from the browser window the
	// returned string is empty, otherwise it is the absolute path to whatever directory
	// was shown selected in the folder hierarchy when the OK button was pressed
	m_strDestFolderPath = wxDirSelector(msg,m_strDestFolderPath,style,pos,(wxWindow*)pFrame);
	SetupDestList(m_strDestFolderPath);
	EnableDeleteDestFileOrFilesButton(FALSE);
	EnableDeleteDestFolderButton(FALSE);
	EnableRenameDestFileButton(FALSE);
	EnableRenameDestFolderButton(FALSE);
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
}

void AdminMoveOrCopy::OnBnClickedSrcParentFolder(wxCommandEvent& WXUNUSED(event))
{
	wxFileName* pFN = new wxFileName;
	wxChar charSeparator = pFN->GetPathSeparator();
	int offset;
	wxString path = m_strSrcFolderPath;
	//path = MakeReverse(path);
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
	EnableCopyFileOrFilesButton(FALSE); // start off disabled until a file is selected
	EnableMoveFileOrFilesButton(FALSE); // ditto
	EnableCopyFolderButton(FALSE);
	EnableMoveFolderButton(FALSE);
}

void AdminMoveOrCopy::OnBnClickedDestParentFolder(wxCommandEvent& WXUNUSED(event))
{
	wxFileName* pFN = new wxFileName;
	wxChar charSeparator = pFN->GetPathSeparator();
	int offset;
	wxString path = m_strDestFolderPath;
	//path = MakeReverse(path);
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

void AdminMoveOrCopy::OnSrcListSelectItem(wxListEvent& event)
{
	int index = event.GetIndex();
	if (index < srcFoldersCount)
	{
		// we clicked on a folder name, so drill down to that child folder and display its
		// contents in the dialog
		wxString aFolderName = event.GetText();

		// extend the path using this foldername, and then display the contents
		m_strSrcFolderPath += gpApp->PathSeparator + aFolderName;
		SetupSrcList(m_strSrcFolderPath);
		event.Skip();
		EnableCopyFileOrFilesButton(FALSE); // start with copy button disabled until a file
											// is selected
		EnableMoveFileOrFilesButton(FALSE); // ditto for move button
		EnableCopyFolderButton(FALSE);
		EnableMoveFolderButton(FALSE);
		return;
	}
	event.Skip();
	// a file has been selected
	SetupSelectedFilesArray(sourceSide); // update srcSelectedFilesArray with current selections
	/* button enabling or disabling is now done in the SetupSelectedFilesArray() call above
	if (srcSelectedFilesArray.GetCount() > 0)
	{
		EnableCopyFileOrFilesButton(TRUE);
		EnableMoveFileOrFilesButton(TRUE);
	}
	else
	{
		EnableCopyFileOrFilesButton(FALSE);
		EnableMoveFileOrFilesButton(FALSE);
	}
	*/
}

void AdminMoveOrCopy::OnDestListSelectItem(wxListEvent& event)
{
	int index = event.GetIndex();
	if (index < destFoldersCount)
	{
		// we clicked on a folder name, so drill down to that child folder and display its
		// contents in the dialog
		wxString aFolderName = event.GetText();

		// extend the path using this foldername, and then display the contents
		m_strDestFolderPath += gpApp->PathSeparator + aFolderName;
		SetupDestList(m_strDestFolderPath);
		event.Skip();
		return;
	}
	event.Skip();
	// a file has been selected
	SetupSelectedFilesArray(destinationSide); // update destSelectedFilesArray with current selections 
	/* button enabling or disabling is now done in the SetupSelectedFilesArray() call above
	if (destSelectedFilesArray.GetCount() > 0)
	{
		EnableDeleteDestFileOrFilesButton(TRUE);
		EnableDeleteDestFolderButton(TRUE);
		EnableRenameDestFileButton(TRUE);
		EnableRenameDestFolderButton(TRUE);
	}
	else
	{
		EnableDeleteDestFileOrFilesButton(FALSE);
		EnableDeleteDestFolderButton(FALSE);
		EnableRenameDestFileButton(FALSE);
		EnableRenameDestFolderButton(FALSE);
	}
	*/
}

void AdminMoveOrCopy::OnSrcListDeselectItem(wxListEvent& event)
{
	int index = event.GetIndex();
	if (index < srcFoldersCount)
	{
		// I don't expect control to go thru here, but if it does, I want feedback
		// about that fact returned audibly; testing has so far never rung the bell
		::wxBell();
		wxString aFolderName = event.GetText();

		// extend the path using this foldername, and then display the contents
		m_strSrcFolderPath += gpApp->PathSeparator + aFolderName;
		SetupSrcList(m_strSrcFolderPath);
		event.Skip();
		return;
	}
	event.Skip();
	// if no files are now selected, disable the copy buton
	SetupSelectedFilesArray(sourceSide); // update srcSelectedFilesArray with current selections 
	/* button enabling or disabling is now done in the SetupSelectedFilesArray() call above

	if (srcSelectedFilesArray.GetCount() > 0)
	{
		EnableCopyFileOrFilesButton(TRUE);
		EnableMoveFileOrFilesButton(TRUE);
	}
	else
	{
		EnableCopyFileOrFilesButton(FALSE);
		EnableMoveFileOrFilesButton(FALSE);
	}
	*/
}

void AdminMoveOrCopy::OnDestListDeselectItem(wxListEvent& event)
{
	int index = event.GetIndex();
	if (index < destFoldersCount)
	{
		// I don't expect control to go thru here, but if it does, I want feedback
		// about that fact returned audibly; testing has so far never rung the bell
		::wxBell(); 
		wxString aFolderName = event.GetText();

		// extend the path using this foldername, and then display the contents
		m_strDestFolderPath += gpApp->PathSeparator + aFolderName;
		SetupDestList(m_strDestFolderPath);
		event.Skip();
		return;
	}
	event.Skip();
	// if no files are now selected, disable the delete and rename buttons (4 buttons)
	SetupSelectedFilesArray(destinationSide); // update destSelectedFilesArray with current selections 
	/* button enabling or disabling is now done in the SetupSelectedFilesArray() call above
	if (destSelectedFilesArray.GetCount() > 0)
	{
		EnableDeleteDestFileOrFilesButton(TRUE);
		EnableDeleteDestFolderButton(TRUE);
		EnableRenameDestFileButton(TRUE);
		EnableRenameDestFolderButton(TRUE);
	}
	else
	{
		EnableDeleteDestFileOrFilesButton(FALSE);
		EnableDeleteDestFolderButton(FALSE);
		EnableRenameDestFileButton(FALSE);
		EnableRenameDestFolderButton(FALSE);
	}
	*/
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

	// do nothing if the destination folder is not yet defined
	if (destPath.IsEmpty())
	{
		wxMessageBox(_(
	"No destination folder is defined. Use the button 'Locate the destination folder' to first set a destination, then try again."),
		_("Cannot move or copy"), wxICON_WARNING);
		m_bNoDestPathYet = TRUE;
		return FALSE;
	}

	// make the path to the source file
	wxString theSourcePath = srcPath + gpApp->PathSeparator + filename;
	// check for a name conflict (including extension, same names but different extensions
	// is not to be treated as a conflict)
	int nConflictedFileIndex = wxNOT_FOUND; // -1
	bool bIsConflicted = IsFileConflicted(filename,&nConflictedFileIndex,&destFilesArray);
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
			FilenameConflictDlg dlg(this,&filename);
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

void AdminMoveOrCopy::EnableCopyFileOrFilesButton(bool bEnableFlag)
{
	if (bEnableFlag)
		pCopyFileOrFilesButton->Enable(TRUE);
	else
		pCopyFileOrFilesButton->Enable(FALSE);
}

void AdminMoveOrCopy::EnableMoveFileOrFilesButton(bool bEnableFlag)
{
	if (bEnableFlag)
		pMoveFileOrFilesButton->Enable(TRUE);
	else
		pMoveFileOrFilesButton->Enable(FALSE);
}

void AdminMoveOrCopy::EnableCopyFolderButton(bool bEnableFlag)
{
	if (bEnableFlag)
		pCopyFolderButton->Enable(TRUE);
	else
		pCopyFolderButton->Enable(FALSE);
}

void AdminMoveOrCopy::EnableMoveFolderButton(bool bEnableFlag)
{
	if (bEnableFlag)
		pMoveFolderButton->Enable(TRUE);
	else
		pMoveFolderButton->Enable(FALSE);
}


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

	// if a Move is wanted, copy the filename seletions across to the 
	// srcSelectedMoveFilesArray, giving two identical filename arrays
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
            // successful copy: if the copy was part of a move request, then delete the
            // source file that was copied and remove it from the array of selected files
            // to be moved
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
	/*
	unsigned int i;
	for (i=0; i < srcSelectedFilesArray.GetCount(); i++)
	{
		wxString fn = srcSelectedFilesArray.Item(i);
		wxLogDebug(_T("Files Before:  %s   at index:  %d  for total of %d\n"), fn.c_str(),i, srcSelectedFilesArray.GetCount());
	}
	*/
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
	/* button disabling is now done in the calls above
	EnableCopyFileOrFilesButton(FALSE);
	EnableMoveFileOrFilesButton(FALSE);
	*/
}

void AdminMoveOrCopy::EnableDeleteDestFolderButton(bool bEnableFlag)
{
	if (bEnableFlag)
		pDeleteDestFolderButton->Enable(TRUE);
	else
		pDeleteDestFolderButton->Enable(FALSE);
}

void AdminMoveOrCopy::EnableRenameDestFileButton(bool bEnableFlag)
{
	if (bEnableFlag)
		pRenameDestFileButton->Enable(TRUE);
	else
		pRenameDestFileButton->Enable(FALSE);
}

void AdminMoveOrCopy::EnableRenameDestFolderButton(bool bEnableFlag)
{
	if (bEnableFlag)
		pRenameDestFolderButton->Enable(TRUE);
	else
		pRenameDestFolderButton->Enable(FALSE);
}

void AdminMoveOrCopy::EnableDeleteDestFileOrFilesButton(bool bEnableFlag)
{
	if (bEnableFlag)
		pDeleteDestFileOrFilesButton->Enable(TRUE);
	else
		pDeleteDestFileOrFilesButton->Enable(FALSE);
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


void AdminMoveOrCopy::OnBnClickedDeleteDestFolder(wxCommandEvent& WXUNUSED(event))
{


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

void AdminMoveOrCopy::OnBnClickedRenameDestFolder(wxCommandEvent& WXUNUSED(event))
{


	// update the destination list
	SetupDestList(m_strDestFolderPath);
}

void AdminMoveOrCopy::OnBnClickedCopySrcFolder(wxCommandEvent& WXUNUSED(event))
{

	// update the destination list
	SetupDestList(m_strDestFolderPath);
}

void AdminMoveOrCopy::OnBnClickedMoveSrcFolder(wxCommandEvent& WXUNUSED(event))
{


	// update the destination list and the source list
	SetupDestList(m_strDestFolderPath);
	SetupSrcList(m_strSrcFolderPath);
}


