/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FilenameConflictDlg.h
/// \author			Bruce Waters
/// \date_created	8 December 2009
/// \date_revised	
/// \copyright		2009 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License 
///                 (see license directory)
/// \description	This is the header file for the FilenameConflictDlg class. 
/// The FilenameConflictDlg class provides a dialog interface for filename classes when
/// moving or copying a source folder's file to the destination folder where a file of the
/// same name already exists. It is modelled after the Windows dialog which performs a
/// similar set of choices for filename conflicts encountered from Win Explorer, though
/// the layout of the Adapt It version and some wordings for the options are a little
/// different. 
/// \derivation		The FilenameConflictDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef FilenameConflictDlg_h
#define FilenameConflictDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "FilenameConflictDlg.h"
#endif

#include "AdminMoveOrCopy.h" // required for the whichside enum below and for
							 // the pointer to the running instance of the
							 // AdminMoveOrCopy class with which this dialog must
							 // cooperate
/*
enum whichSide {
	sourceSide,
	destinationSide
};
*/

/// The AdminMoveOrCopy class provides a dialog interface for moving or copying files or
/// folders or both. It is derived from AIModalDialog.
class FilenameConflictDlg : public AIModalDialog
{
public:
	FilenameConflictDlg(wxWindow* parent,
		wxString* pConflictingFilename); // constructor, parent will be AdminMoveOrCopy instance
	virtual ~FilenameConflictDlg(void); // destructor

	// wx version pointers for dialog controls; the names for buttons will leave the
	// "move" word out, so the code maintainer needs to understand that in the namings
	// below, "Copy" is to be understood as "MoveOrCopy" (this choice is just a
	// convenience to keep the namings shorter)
	wxRadioButton* m_pCopyAndReplaceRadioButton;
	wxRadioButton* m_pNoCopyRadioRadioButton;
	wxRadioButton* m_pChangeNameAndCopyRadioButton;
	wxCheckBox* m_pHandleSameWayCheckbox;
	wxButton* m_pProceedButton;
	wxButton* m_pCancelButton;
	wxTextCtrl* m_pSrcFileDataBox;
	wxTextCtrl* m_pDestFileDataBox;
	wxStaticText* m_pNameChangeText;


	AdminMoveOrCopy* m_pAdminMoveOrCopy; //this is the parent dialog
	wxArrayString* m_pSrcFileArray; // we'll set this from pAdminMoveOrCopy's 
								  // srcSelectedFilesArray public string array member

	// the path to source and destination folders - we'll set these
	// using pAdminMoveOrCopy's public members
	wxString* m_pSrcFolderPath;
	wxString* m_pDestFolderPath;

protected:
	void OnBnClickedCopyAndReplace(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedNoCopy(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedChangeNameAndCopy(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedProceed(wxCommandEvent& event);
	void OnBnClickedCancel(wxCommandEvent& event);
	void OnCheckboxHandleSameWay(wxCommandEvent& WXUNUSED(event));
private:
	wxString srcFilename;
	wxString destFilename;
	wxString srcPathToFilename;
	wxString destPathToFilename;
	size_t	 srcFilesizeInBytes;
	size_t	 destFilesizeInBytes;
	float	 srcFilesizeInKB;
	float	 destFilesizeInKB;
	float	 srcFilesizeInMB;
	float	 destFilesizeInMB;
	wxDateTime srcFileModifiedTime;
	wxDateTime destFileModifiedTime;
	wxTimeSpan timeDifference;
	wxString srcDetailsStr;
	wxString destDetailsStr;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	DECLARE_EVENT_TABLE()
};
#endif /* FilenameConflictDlg_h */