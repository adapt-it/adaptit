/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AdminMoveOrCopy.h
/// \author			Bruce Waters
/// \date_created	30 November 2009
/// \date_revised	
/// \copyright		2009 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the AdminMoveOrCopy class. 
/// The AdminMoveOrCopy class provides a dialog interface for the user (typically an administrator) to be able
/// to move or copy files or folders or both from a source location into a destination
/// folder; and also to delete files or folders, and also, one at a time, to rename a file
/// or folder. Its dialog provides a 2-list view, one list for source, another for destination.
/// \derivation		The AdminMoveOrCopy class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef AdminMoveOrCopy_h
#define AdminMoveOrCopy_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "AdminMoveOrCopy.h"
#endif

#include <wx/datetime.h>

enum whichSide {
	sourceSide,
	destinationSide
};

enum CopyAction {
	copyAndReplace, // covers both copy which overwrites file of same name, 
					// and copy of a file with unique filename (default)
	copyWithChangedName, // copy but with unique name, such as <filename>(2)
	noCopy			// don't copy (nor move) the conflicting file
};

/// The AdminMoveOrCopy class provides a dialog interface for moving or copying files or
/// folders or both. It is derived from AIModalDialog.
class AdminMoveOrCopy : public AIModalDialog
{
public:
	AdminMoveOrCopy(wxWindow* parent); // constructor
	virtual ~AdminMoveOrCopy(void); // destructor

	// flags which the Resolve Filename Conflict dialog needs to set or clear
	bool m_bUserCancelled;
	bool m_bDoTheSameWay;
	CopyAction copyType;
	CopyAction lastWay;
	wxSizer* pAdminMoveCopySizer;

	// wx version pointers for dialog controls
	wxButton* pMoveButton;
	wxButton* pCopyButton;
	wxButton* pDeleteButton;
	wxButton* pRenameButton;

	wxBitmapButton* pUpSrcFolder; 
	wxBitmapButton* pUpDestFolder;

	wxButton* pLocateSrcFolderButton;
	wxButton* pLocateDestFolderButton;
	wxTextCtrl* pSrcFolderPathTextCtrl;
	wxTextCtrl* pDestFolderPathTextCtrl;

	wxImageList* pIconImages;
	wxListItem* pTheColumnForSrcList; // has to be on heap
	wxListItem* pTheColumnForDestList; // has to be on heap
	//wxListCtrl* pSrcList; // using wxListView is easier
	//wxListCtrl* pDestList;
	wxListView* pSrcList; // a subclass of wxListCtrl
	wxListView* pDestList; // ditto
	wxString emptyFolderMessage;

	wxString m_strSrcFolderPath;
	wxString m_strDestFolderPath;
	wxString m_strSrcFolderPath_OLD;
	wxString m_strDestFolderPath_OLD;

	wxArrayString srcFoldersArray; // stores folder names (these get displayed)
	wxArrayString srcFilesArray; // stores filenames (these get displayed)
	wxArrayString destFoldersArray; // stores folder names (these get displayed)in a block
	wxArrayString destFilesArray; // stores filenames (these get displayed) in a block;
                // and use this to check for conflicts when copying or moving files with
                // names stored in srcSelectionArray
    wxArrayString srcSelectedFilesArray; // stores just the names of the selected files
    wxArrayString srcSelectedFoldersArray; // stores just the names of the selected folders
    wxArrayString destSelectedFilesArray; // stores just the names of the selected files
    wxArrayString destSelectedFoldersArray; // stores just the names of the selected folders
	wxArrayInt arrCopiedOK; // stores 1 for a successful file copy, 0 if not copied

	size_t srcFoldersCount;
	size_t srcFilesCount;
	size_t destFoldersCount;
	size_t destFilesCount;

	// next four for value passing (the double-click event doesn't know about the list)
	//size_t m_srcIndex;  // use this to pass item index to the double-click handler
	//size_t m_destIndex;  // use this to pass item index to the double-click handler
	//wxString m_srcItemText; // use this to pass text of double-clicked item to dbl click handler
	//wxString m_destItemText; // use this to pass text of double-clicked item to dbl click handler
	//bool m_bSrcListDoubleclicked; // TRUE if mouse handler detects src list was just doubleclicked
	//bool m_bDestListDoubleclicked; // TRUE if mouse handler detects destination list was just doubleclicked

	wxString BuildChangedFilenameForCopy(wxString* pFilename);

protected:
	void OnBnClickedLocateSrcFolder(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedLocateDestFolder(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedSrcParentFolder(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedDestParentFolder(wxCommandEvent& WXUNUSED(event));

	void OnBnClickedRename(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedDelete(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedCopy(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedMove(wxCommandEvent& WXUNUSED(event));

	void EnableCopyButton(bool bEnableFlag);
	void EnableMoveButton(bool bEnableFlag);
	void EnableDeleteButton(bool bEnableFlag);
	void EnableRenameButton(bool bEnableFlag);

	void OnOK(wxCommandEvent& event);
	void OnSize(wxSizeEvent& event);

	void OnSrcListSelectItem(wxListEvent& event);
	void OnSrcListDeselectItem(wxListEvent& event);
	void OnDestListSelectItem(wxListEvent& event);
	void OnDestListDeselectItem(wxListEvent& event);

	void OnSrcListDoubleclick(wxListEvent& event);
	void OnDestListDoubleclick(wxListEvent& event);


private:
	void MoveOrCopyFilesAndFolders(wxString srcFolderPath, wxString destFolderPath,
				wxArrayString* pSrcSelectedFoldersArray, wxArrayString* pSrcSelectedFilesArray, 
				bool bDoMove = TRUE);
	void RemoveFilesAndFolders(wxString destFolderPath, wxArrayString* pDestSelectedFoldersArray, 
				wxArrayString* pDestSelectedFilesArray);
	bool CopySingleFile(wxString& srcPath, wxString& destPath, wxString& filename, 
				bool& bUserCancelled);
	bool RemoveSingleFile(wxString& destPath, wxString& filename);
	void GetListCtrlContents(enum whichSide side, wxString& folderPath, 
				bool& bHasFolders, bool& bHasFiles);
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	bool IsFileConflicted(wxString& srcFile, int* pConflictedDestFile, wxArrayString* pDestFilesArr);
	void SetupDestList(wxString& folderPath);
	void SetupSrcList(wxString& folderPath);
	void SetupSelectionArrays(enum whichSide side);
	void DeselectSelectedFiles(enum whichSide side); // beware of wxWidgets bug in SetItemState()
	bool CheckForIdenticalPaths(wxString& srcPath, wxString& destPath);

	/* OBSOLETE

	//void OnCopyFileOrFiles(wxCommandEvent& WXUNUSED(event));
	//void OnMoveFileOrFiles(wxCommandEvent& WXUNUSED(event));
	//void EnableCopyFileOrFilesButton(bool bEnableFlag);
	//void EnableMoveFileOrFilesButton(bool bEnableFlag);
	//void EnableDeleteDestFileOrFilesButton(bool bEnableFlag);
	//void EnableRenameDestFileButton(bool bEnableFlag);
	//
	//void OnBnClickedDeleteDestFiles(wxCommandEvent& WXUNUSED(event));

	*/

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* AdminMoveOrCopy_h */
