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
/// to move or copy files or folders or both from a source location into a destination folder.
/// \derivation		The AdminMoveOrCopy class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef AdminMoveOrCopy_h
#define AdminMoveOrCopy_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "AdminMoveOrCopy.h"
#endif

enum whichSide {
	sourceSide,
	destinationSide
};

enum CopyAction {
	copyUnique,
	copyAndReplace,
	copyWithChangedName,
	noCopy
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
	bool m_bCopyWasSuccessful;
	bool m_bDoTheSameWay;

	// wx version pointers for dialog controls
	wxButton* pMoveFolderButton;
	wxButton* pMoveFileOrFilesButton;
	wxButton* pCopyFolderButton;
	wxButton* pCopyFileOrFilesButton;
	wxBitmapButton* pUpSrcFolder; 
	wxBitmapButton* pUpDestFolder;
	wxButton* pLocateSrcFolderButton;
	wxButton* pLocateDestFolderButton;
	wxTextCtrl* pSrcFolderPathTextCtrl;
	wxTextCtrl* pDestFolderPathTextCtrl;

	wxImageList* pIconImages;
	wxListItem* pTheColumnForSrcList; // has to be on heap
	wxListItem* pTheColumnForDestList; // has to be on heap
	wxListCtrl* pSrcList;
	wxListCtrl* pDestList;
	wxString emptyFolderMessage;

	void OnBnClickedLocateSrcFolder(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedLocateDestFolder(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedSrcParentFolder(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedDestParentFolder(wxCommandEvent& WXUNUSED(event));

	wxString m_strSrcFolderPath;
	wxString m_strDestFolderPath;

	wxArrayString srcFoldersArray; // stores folder names (these get displayed)
	wxArrayString srcFilesArray; // stores filenames (these get displayed)
	wxArrayString srcSelectedFilesArray; // stores filenames selected by user

	wxArrayString destFoldersArray; // stores folder names (these get displayed)
	wxArrayString destFilesArray; // stores filenames (these get displayed); and use
                // this to check for conflicts when copying or moving files with names
                // stored in srcSelectedFilesArray
	wxArrayString destSelectedFilesArray; // stores filenames selected by user
				// (Note: files selected in destination folder is only meaningful
				// for renaming or deleting these files, and the contents of this
				// list is ignored for moving or copying as the latter two 
				// functionalities use the destFolderAllFilesArray (below) instead

	int srcFoldersCount;
	int srcFilesCount;
	int destFoldersCount;
	int destFilesCount;

	wxString BuildChangedFilenameForCopy(wxString* pFilename);

protected:
	void EnableCopyFileOrFilesButton(bool bEnableFlag);
	void OnOK(wxCommandEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnSrcListSelectItem(wxListEvent& event);
	void OnSrcListDeselectItem(wxListEvent& event);
	void OnDestListSelectItem(wxListEvent& event);
	void OnDestListDeselectItem(wxListEvent& event);
	void OnCopyFileOrFiles(wxCommandEvent& WXUNUSED(event));
private:
	bool CopySingleFile(wxString& srcPath, wxString& destPath, wxString& filename, 
						bool& bUserCancelled, bool& bDoTheSameWay);
	void GetListCtrlContents(enum whichSide side, wxString& folderPath, 
								bool& bHasFolders, bool& bHasFiles);
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	//bool IsFileConflicted(int srcFileIndex, int* pConflictedDestFile, wxArrayString* pDestFilesArr);
	bool IsFileConflicted(wxString& srcFile, int* pConflictedDestFile, wxArrayString* pDestFilesArr);
	void SetupDestList(wxString& folderPath);
	void SetupSrcList(wxString& folderPath);
	void SetupSelectedFilesArray(enum whichSide side);
	void DeselectSelectedFiles(enum whichSide side);

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* AdminMoveOrCopy_h */
