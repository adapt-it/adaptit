/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			MoveDialog.h
/// \author			Jonathan Field; modified by Bill Martin for the WX version
/// \date_created	10 November 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CMoveDialog class. 
/// The CMoveDialog class provides a dialog interface in which the user can
/// move documents from the Adaptations folder to the current book folder or
/// from the current book folder to the Adaptations folder. It also allows
/// the user to view the documents in the "other" folder and/or rename documents.
/// \derivation		The CMoveDialog class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef MoveDialog_h
#define MoveDialog_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "MoveDialog.h"
#endif

/// The CMoveDialog class provides a dialog interface in which the user can
/// move documents from the Adaptations folder to the current book folder or
/// from the current book folder to the Adaptations folder. It also allows
/// the user to view the documents in the "other" folder and/or rename documents.
/// \derivation		The CMoveDialog class is derived from AIModalDialog.
class CMoveDialog : public AIModalDialog
{
public:
	CMoveDialog(wxWindow* parent); // constructor
	virtual ~CMoveDialog(void); // destructor

	wxListBox* pSourceFolderDocumentListBox;
	wxRadioButton* pToBookFolderRadioButton;
	wxRadioButton* pFromBookFolderRadioButton;
	wxStaticText* pDocumentsInTheFolderLabel;

	// other methods
	void OnBnClickedMoveNow(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedButtonRenameDoc(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedViewOther(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioToBookFolder(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioFromBookFolder(wxCommandEvent& WXUNUSED(event));

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	bool bFromBookFolder;
	void MoveDirectionChanged();
	void UpdateFileList();

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CRefString)
};
#endif /* MoveDialog_h */
