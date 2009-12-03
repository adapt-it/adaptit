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

/// The AdminMoveOrCopy class provides a dialog interface for moving or copying files or
/// folders or both. It is derived from AIModalDialog.
class AdminMoveOrCopy : public AIModalDialog
{
public:
	AdminMoveOrCopy(wxWindow* parent); // constructor
	virtual ~AdminMoveOrCopy(void); // destructor

	// wx version pointers for dialog controls
	/*
	wxTextCtrl* pNewFileName;
	wxListBox* pAcceptedFiles;
	wxListBox* pRejectedFiles;
	wxButton* pMoveAllRight;
	wxButton* pMoveAllLeft;
	wxButton* pJoinNow;
	wxButton* pClose;
	wxButton* pMoveUp;
	wxButton* pMoveDown;
	wxStaticText* pJoiningWait;
	*/
	wxBitmapButton* pUpSrcFolder; 
	wxBitmapButton* pUpDestFolder;
	wxButton* pLocateSrcFolderButton;
	wxButton* pLocateDestFolderButton;
	wxTextCtrl* pSrcFolderPathTextCtrl;
	wxTextCtrl* pDestFolderPathTextCtrl;
	wxImageList iconImages;
	wxListCtrl* pSrcList;
	wxListCtrl* pDestList;
	wxString emptyFolderMessage;

	void OnBnClickedLocateSrcFolder(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedLocateDestFolder(wxCommandEvent& WXUNUSED(event));

	/*
	void OnBnClickedJoinNow(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedButtonMoveAllLeft(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedButtonMoveAllRight(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedButtonAccept(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedButtonReject(wxCommandEvent& WXUNUSED(event));
	void OnLbnDblclkListAccepted(wxCommandEvent& WXUNUSED(event));
	void OnLbnDblclkListRejected(wxCommandEvent& WXUNUSED(event));
	void OnLbnSelchangeListAccepted(wxCommandEvent& WXUNUSED(event));
	void OnLbnSelchangeListRejected(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedButtonMoveDown(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedButtonMoveUp(wxCommandEvent& WXUNUSED(event));
	*/

	wxString m_strSrcFolderPath;
	wxString m_strDestFolderPath;

	wxArrayString srcFoldersArray; // stores folder names (these get displayed)
	wxArrayString srcFilesArray; // stores filenames (these get displayed)

	wxArrayString destFoldersArray; // stores folder names (these get displayed)
	wxArrayString destFilesArray; // stores filenames (these get displayed)

protected:
	void OnOK(wxCommandEvent& event);
	/*
	void MoveSelectedItems(wxListBox& From, wxListBox& To);
	void MoveAllItems(wxListBox& From, wxListBox& To);
	void InitialiseLists();
	void ListContentsOrSelectionChanged();
	*/
private:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	bool SetListCtrlContents(enum whichSide side);

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* AdminMoveOrCopy_h */
