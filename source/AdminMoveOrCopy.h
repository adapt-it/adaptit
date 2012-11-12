/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AdminMoveOrCopy.h
/// \author			Bruce Waters
/// \date_created	30 November 2009
/// \rcs_id $Id$
/// \copyright		2009 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the AdminMoveOrCopy class.
/// The AdminMoveOrCopy class provides a dialog interface for the user (typically an
/// administrator) to be able to move or copy files or folders or both from one pane
/// displaying a folder's contents into another displaying a different folder's contents;
/// also to delete files or folders, and also, one at a time, to rename a file or folder.
/// Its dialog provides a 2-list view, one list for left side, another for right.
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
	leftSide,
	rightSide
};

enum focusWhere {
	neitherSideHasFocus,
	leftSideHasFocus,
	rightSideHasFocus
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
	friend class CPeekAtFileDlg;

public:
	AdminMoveOrCopy(wxWindow* parent); // constructor
	virtual ~AdminMoveOrCopy(void); // destructor

	// flags which the Resolve Filename Conflict dialog needs to set or clear
	bool m_bUserCancelled;
	bool m_bDoTheSameWay;
	CopyAction copyType;
	CopyAction lastWay;
	wxSizer* pAdminMoveCopySizer;

private:
	// wx version pointers for dialog controls
	wxButton* pMoveButton;
	wxButton* pCopyButton;
	wxButton* pDeleteButton;
	wxButton* pRenameButton;
	wxButton* pPeekButton;

	wxBitmapButton* pUpLeftFolder;
	wxBitmapButton* pUpRightFolder;

	wxButton* pLocateLeftFolderButton;
	wxButton* pLocateRightFolderButton;
	wxTextCtrl* pLeftFolderPathTextCtrl;
	wxTextCtrl* pRightFolderPathTextCtrl;

	wxImageList* pIconImages;
	wxListItem* pTheColumnForLeftList; // has to be on heap
	wxListItem* pTheColumnForRightList; // has to be on heap
	wxListView* pLeftList; // a subclass of wxListCtrl
	wxListView* pRightList; // ditto
	wxString emptyFolderMessage;

	wxString m_strLeftFolderPath;
	wxString m_strRightFolderPath;
	wxString m_strLeftFolderPath_OLD;
	wxString m_strRightFolderPath_OLD;

	wxArrayString leftFoldersArray; // stores folder names (these get displayed)
	wxArrayString leftFilesArray; // stores filenames (these get displayed)
	wxArrayString rightFoldersArray; // stores folder names (these get displayed)in a block
	wxArrayString rightFilesArray; // stores filenames (these get displayed) in a block;
                // and use this to check for conflicts when copying or moving files with
                // names stored in leftSelectionArray
    wxArrayString leftSelectedFilesArray; // stores just the names of the selected files
    wxArrayString leftSelectedFoldersArray; // stores just the names of the selected folders
    wxArrayString rightSelectedFilesArray; // stores just the names of the selected files
    wxArrayString rightSelectedFoldersArray; // stores just the names of the selected folders
	wxArrayInt arrCopiedOK; // stores 1 for a successful file copy, 0 if not copied

	size_t leftFoldersCount;
	size_t leftFilesCount;
	size_t rightFoldersCount;
	size_t rightFilesCount;

	wxColour pastelgreen;
	wxColour nocolor;


public:
	wxString BuildChangedFilenameForCopy(wxString* pFilename);

protected:
	void OnBnClickedLocateLeftFolder(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedLocateRightFolder(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedLeftParentFolder(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRightParentFolder(wxCommandEvent& WXUNUSED(event));

	void OnBnClickedRename(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedDelete(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedCopy(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedMove(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedPeek(wxCommandEvent& WXUNUSED(event));

	void EnableCopyButton(bool bEnableFlag);
	void EnableMoveButton(bool bEnableFlag);
	void EnableDeleteButton(bool bEnableFlag);
	void EnableRenameButton(bool bEnableFlag);
	void EnablePeekButton(bool bEnableFlag);
	void EnableButtons();

	void OnOK(wxCommandEvent& event);
	void OnSize(wxSizeEvent& event);

	void OnLeftListSelectItem(wxListEvent& event);
	void OnLeftListDeselectItem(wxListEvent& event);
	void OnRightListSelectItem(wxListEvent& event);
	void OnRightListDeselectItem(wxListEvent& event);

	//void OnLeftListKeyDown(wxListEvent& event);
	//void OnRightListKeyDown(wxListEvent& event);

	void OnLeftListDoubleclick(wxListEvent& event);
	void OnRightListDoubleclick(wxListEvent& event);
	void NoSelectionMessage();

	void SetLeftSideHasFocus();
	void SetRightSideHasFocus();
	void SetNeitherSideHasFocus();

private:
	void MoveOrCopyFilesAndFolders(wxString leftFolderPath, wxString rightFolderPath,
				wxArrayString* pLeftSelectedFoldersArray, wxArrayString* pLeftSelectedFilesArray,
				bool bToSourceDataFolder = FALSE, bool bDoMove = TRUE);
	void RemoveFilesAndFolders(wxString rightFolderPath, wxArrayString* pRightSelectedFoldersArray,
				wxArrayString* pRightSelectedFilesArray);
	bool CopySingleFile(wxString& leftPath, wxString& rightPath, wxString& filename,
				bool& bUserCancelled);
	bool RemoveSingleFile(wxString& thePath, wxString& filename);
	void GetListCtrlContents(enum whichSide side, wxString& folderPath,
				bool& bHasFolders, bool& bHasFiles);
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	bool IsFileConflicted(wxString& leftFile, int* pConflictedRightFile, wxArrayString* pRightFilesArr);
	void SetupRightList(wxString& folderPath);
	void SetupLeftList(wxString& folderPath);
	void SetupSelectionArrays(enum whichSide side);
	void DeselectSelectedItems(enum whichSide side); // beware of wxWidgets bug in SetItemState()

	void DoDeselectionAndDefocus(enum whichSide side);

	bool CheckForIdenticalPaths(wxString& leftPath, wxString& rightPath);
	void PutUpInvalidsMessage(wxString& strAllInvalids);

	enum focusWhere sideWithFocus;

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* AdminMoveOrCopy_h */

