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

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
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
	indxFileIcon
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


	srcFoldersArray.Empty();
	srcFolderPathsArray.Empty();
	srcFilesArray.Empty();
	destFoldersArray.Empty();
	destFolderPathsArray.Empty();
	destFilesArray.Empty();
}

AdminMoveOrCopy::~AdminMoveOrCopy() // destructor
{
	srcFoldersArray.Clear();
	srcFolderPathsArray.Clear();
	srcFilesArray.Clear();
	destFoldersArray.Clear();
	destFolderPathsArray.Clear();
	destFilesArray.Clear();
}

void AdminMoveOrCopy::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	// set up pointers to interface objects
	pUpSrcFolder = (wxBitmapButton*)FindWindowById(ID_BITMAPBUTTON_SRC_OPEN_FOLDER_UP);
	wxASSERT(pUpSrcFolder != NULL);
	pUpDestFolder = (wxBitmapButton*)FindWindowById(ID_BITMAPBUTTON_DEST_OPEN_FOLDER_UP);
	wxASSERT(pUpDestFolder != NULL);

    // get the folder and file icons (bitmaps actually) into the image list which the two
    // wxListCtrl instances will use
	iconImages.Create(16,16,FALSE,2); // FALSE is bool mask, and we don't need a mask
	wxBitmap folderIcon(AIMainFrameIcons(10));
	wxBitmap fileIcon(AIMainFrameIcons(11));
	iconImages.Add(folderIcon); // no mask; use enum value indxFolderIcon to access it
	iconImages.Add(fileIcon); // no mask; use enum value indxFileIcon to access it

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

	//TransferDataToWindow();

	// *** TODO *** enumerate the files and folders, insert in list ctrl & select top item
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
}

