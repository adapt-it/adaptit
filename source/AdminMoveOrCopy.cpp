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
	EVT_INIT_DIALOG(AdminMoveOrCopy::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_OK, AdminMoveOrCopy::OnOK)
	EVT_BUTTON(ID_BUTTON_LOCATE_SOURCE_FOLDER, AdminMoveOrCopy::OnBnClickedLocateSrcFolder)	
	EVT_BUTTON(ID_BUTTON_LOCATE_DESTINATION_FOLDER, AdminMoveOrCopy::OnBnClickedLocateDestFolder)	
	/*
	EVT_BUTTON(ID_JOIN_NOW, CJoinDialog::OnBnClickedJoinNow)
	EVT_BUTTON(IDC_BUTTON_MOVE_ALL_LEFT, CJoinDialog::OnBnClickedButtonMoveAllLeft)
	EVT_BUTTON(IDC_BUTTON_MOVE_ALL_RIGHT, CJoinDialog::OnBnClickedButtonMoveAllRight)
	EVT_BUTTON(IDC_BUTTON_ACCEPT, CJoinDialog::OnBnClickedButtonAccept)
	EVT_BUTTON(IDC_BUTTON_REJECT, CJoinDialog::OnBnClickedButtonReject)
	EVT_LISTBOX_DCLICK(IDC_LIST_ACCEPTED, CJoinDialog::OnLbnDblclkListAccepted)
	EVT_LISTBOX_DCLICK(IDC_LIST_REJECTED, CJoinDialog::OnLbnDblclkListRejected)
	EVT_LISTBOX(IDC_LIST_ACCEPTED, CJoinDialog::OnLbnSelchangeListAccepted)
	EVT_LISTBOX(IDC_LIST_REJECTED, CJoinDialog::OnLbnSelchangeListRejected)
	EVT_BUTTON(IDC_BUTTON_MOVE_DOWN, CJoinDialog::OnBnClickedButtonMoveDown)
	EVT_BUTTON(IDC_BUTTON_MOVE_UP, CJoinDialog::OnBnClickedButtonMoveUp)
	*/
END_EVENT_TABLE()

AdminMoveOrCopy::AdminMoveOrCopy(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Move or Copy Folders Or Files"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	MoveOrCopyFilesOrFoldersFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

	m_strSrcFolderPath = _T("");
	m_strDestFolderPath = _T("");

	pSrcFolderPathTextCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SOURCE_PATH);
	pSrcFolderPathTextCtrl->SetValidator(wxGenericValidator(&m_strSrcFolderPath));
	pDestFolderPathTextCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_DESTINATION_PATH);
	pDestFolderPathTextCtrl->SetValidator(wxGenericValidator(&m_strDestFolderPath));

	pLocateSrcFolderButton = (wxButton*)FindWindowById(ID_BUTTON_LOCATE_SOURCE_FOLDER);
	wxASSERT(pLocateSrcFolderButton != NULL);
	pLocateDestFolderButton = (wxButton*)FindWindowById(ID_BUTTON_LOCATE_DESTINATION_FOLDER);
	wxASSERT(pLocateDestFolderButton != NULL);

	pSrcList = (wxListCtrl*)FindWindowById(ID_LISTCTRL_SOURCE_CONTENTS);
	pDestList= (wxListCtrl*)FindWindowById(ID_LISTCTRL_DESTINATION_CONTENTS);

	srcFoldersArray.Empty();
	srcFilesArray.Empty();
	destFoldersArray.Empty();
	destFilesArray.Empty();

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
}

void AdminMoveOrCopy::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	srcFoldersCount = 0;
	srcFilesCount = 0;
	destFoldersCount = 0;
	destFilesCount = 0;

	// set up pointers to interface objects
	pUpSrcFolder = (wxBitmapButton*)FindWindowById(ID_BITMAPBUTTON_SRC_OPEN_FOLDER_UP);
	wxASSERT(pUpSrcFolder != NULL);
	pUpDestFolder = (wxBitmapButton*)FindWindowById(ID_BITMAPBUTTON_DEST_OPEN_FOLDER_UP);
	wxASSERT(pUpDestFolder != NULL);

	pMoveFolderButton = (wxButton*)FindWindowById(ID_BUTTON_MOVE_FOLDER);
	pMoveFileOrFilesButton = (wxButton*)FindWindowById(ID_BUTTON_MOVE_FILES);
	pCopyFolderButton = (wxButton*)FindWindowById(ID_BUTTON_COPY_FOLDER);
	pCopyFileOrFilesButton = (wxButton*)FindWindowById(ID_BUTTON_COPY_FILES);


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
	
	pIconImages = new wxImageList(16,14,TRUE,2);

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
	// set the destination column's width & insert it into the destination side's wxListCtrl
	pDestList->GetClientSize(&width,&height);
	pTheColumnForDestList->SetWidth(width);
	pDestList->InsertColumn(0, *pTheColumnForDestList);

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


	// initialize for the "Locate...folder" buttons
	m_strSrcFolderPath = gpApp->m_workFolderPath;
	// set up reasonable default paths so that the wxDir class's browser has directory
	// defaults to start from
	if (gpApp->m_bUseCustomWorkFolderPath  && !gpApp->m_customWorkFolderPath.IsEmpty())
	{
		m_strDestFolderPath = gpApp->m_customWorkFolderPath;
	}
	else
	{
		m_strDestFolderPath = gpApp->m_workFolderPath;
	}
	/*
	pNewFileName = (wxTextCtrl*)FindWindowById(IDC_EDIT_NEW_FILENAME);
	wxASSERT(pNewFileName != NULL);
	pAcceptedFiles = (wxListBox*)FindWindowById(IDC_LIST_ACCEPTED);
	wxASSERT(pAcceptedFiles != NULL);
	pRejectedFiles = (wxListBox*)FindWindowById(IDC_LIST_REJECTED);
	wxASSERT(pRejectedFiles != NULL);
	pMoveAllRight = (wxButton*)FindWindowById(IDC_BUTTON_MOVE_ALL_RIGHT);
	wxASSERT(pMoveAllRight != NULL);
	pMoveAllLeft = (wxButton*)FindWindowById(IDC_BUTTON_MOVE_ALL_LEFT);
	wxASSERT(pMoveAllLeft != NULL);
	pJoinNow = (wxButton*)FindWindowById(ID_JOIN_NOW);
	wxASSERT(pJoinNow != NULL);
	pClose = (wxButton*)FindWindowById(wxID_OK);
	wxASSERT(pClose != NULL);
	pMoveUp = (wxButton*)FindWindowById(IDC_BUTTON_MOVE_UP);
	wxASSERT(pMoveUp != NULL);
	pMoveDown = (wxButton*)FindWindowById(IDC_BUTTON_MOVE_DOWN);
	wxASSERT(pMoveDown != NULL);
	pJoiningWait = (wxStaticText*)FindWindowById(IDC_STATIC_JOINING_WAIT);
	wxASSERT(pJoiningWait != NULL);
	*/
	/*
	wxTextCtrl* pTextCtrlAsStaticJoin1 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_JOIN1);
	wxASSERT(pTextCtrlAsStaticJoin1 != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	pTextCtrlAsStaticJoin1->SetBackgroundColour(backgrndColor);

	wxTextCtrl* pTextCtrlAsStaticJoin2 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_JOIN2);
	wxASSERT(pTextCtrlAsStaticJoin2 != NULL);
	pTextCtrlAsStaticJoin2->SetBackgroundColour(backgrndColor);

	wxTextCtrl* pTextCtrlAsStaticJoin3 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_JOIN3);
	wxASSERT(pTextCtrlAsStaticJoin3 != NULL);
	pTextCtrlAsStaticJoin3->SetBackgroundColour(backgrndColor);

	wxTextCtrl* pTextCtrlAsStaticJoin4 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_JOIN4);
	wxASSERT(pTextCtrlAsStaticJoin4 != NULL);
	pTextCtrlAsStaticJoin4->SetBackgroundColour(backgrndColor);
	*/
	CAdapt_ItApp* pApp;
	pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp != NULL);

	emptyFolderMessage = _("The folder is empty");

	


	// make the font show the user's desired dialog font point size
	#ifdef _RTL_FLAGS
//	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, pNewFileName, NULL,
//					pAcceptedFiles, pRejectedFiles, gpApp->m_pDlgGlossFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
//	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, pNewFileName, NULL, 
//					pAcceptedFiles, pRejectedFiles, gpApp->m_pDlgGlossFont);
	#endif

	//InitialiseLists();

	// select the top item as default
	/*
	int index = 0;
	if (pAcceptedFiles->GetCount() > 0)
	{
		pAcceptedFiles->SetSelection(index);
		ListContentsOrSelectionChanged();
	}
	*/
	pApp->RefreshStatusBarInfo();

}

void AdminMoveOrCopy::GetListCtrlContents(enum whichSide side, bool& bHasFolders, bool& bHasFiles)
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

		bHasFolders = GetFoldersOnly(m_strSrcFolderPath, &srcFoldersArray); // default is to sort the array
		bHasFiles = GetFilesOnly(m_strSrcFolderPath, &srcFilesArray); // default is to sort the array
	}
	else
	{
		// right side of the dialog (destination folder's side)
		
		// clear out old content in the list and its supporting arrays
		destFoldersArray.Empty();
		destFilesArray.Empty();
		pDestList->DeleteAllItems(); // don't use ClearAll() because it clobbers any columns too

		bHasFolders = GetFoldersOnly(m_strDestFolderPath, &destFoldersArray); // default is to sort the array
		bHasFiles = GetFilesOnly(m_strDestFolderPath, &destFilesArray); // default is to sort the array
	}

	// debugging -- display what we got for source side
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
#endif
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
	
	// put the path into the edit control
	pSrcFolderPathTextCtrl->ChangeValue(m_strSrcFolderPath);


	// *** TODO *** enumerate the files and folders, insert in list ctrl & select top item
	long rv = 0L; // for a return value
	bool bHasFiles;
	bool bHasFolders;
	wxString aFolder;
	wxString aFile;
	GetListCtrlContents(sourceSide, bHasFolders, bHasFiles);
	if (bHasFolders || bHasFiles)
	{
		// Enable the move and copy buttons at the bottom
		pMoveFolderButton->Enable();
		pMoveFileOrFilesButton->Enable();
		pCopyFolderButton->Enable();
		pCopyFileOrFilesButton->Enable();
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
	
	// put the path into the edit control
	pDestFolderPathTextCtrl->ChangeValue(m_strDestFolderPath);

	//TransferDataToWindow();

	// *** TODO *** enumerate the files and folders, insert in list ctrl & select top item

	//set up the dest wxListCtrl
	pDestList->SetImageList(pIconImages, wxIMAGE_LIST_SMALL);
	int height;
	int width;
	pDestList->GetClientSize(&width,&height);
	wxListItem theColumn;
	theColumn.SetWidth(width);
	pDestList->InsertColumn(0, theColumn);

	
}

