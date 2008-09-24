/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			ListDocumentsInOtherFolderDialog.h
/// \author			Bill Martin
/// \date_created	10 November 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CListDocumentsInOtherFolderDialog class. 
/// The CListDocumentsInOtherFolderDialog class is called from the CMoveDialog class to provide
/// a sorted list of documents in a different folder.
/// \derivation		The CListDocumentsInOtherFolderDialog class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ListDocumentsInOtherFolderDialog_h
#define ListDocumentsInOtherFolderDialog_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ListDocumentsInOtherFolderDialog.h"
#endif

/// The CListDocumentsInOtherFolderDialog class is called from the CMoveDialog class to provide
/// a sorted list of documents in a different folder.
/// \derivation		The CListDocumentsInOtherFolderDialog class is derived from AIModalDialog.
class CListDocumentsInOtherFolderDialog : public AIModalDialog
{
public:
	CListDocumentsInOtherFolderDialog(wxWindow* parent); // constructor
	virtual ~CListDocumentsInOtherFolderDialog(void); // destructor
	//enum { IDD = IDD_LIST_DOCUMENTS_IN_OTHER_FOLDER_DLG };
	// other methods

	// wx note: FolderPath and FolderDiaplayName are set in the caller (CMoveDialog) before
	// dialog is shown
	wxString FolderDisplayName;
	wxString FolderPath;

	// wx version uses pointers to controls
	wxStaticText* pLabel;
	wxListBox* pListBox;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ListDocumentsInOtherFolderDialog_h */
