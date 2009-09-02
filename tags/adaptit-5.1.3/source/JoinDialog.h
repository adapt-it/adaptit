/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			JoinDialog.h
/// \author			Jonathan Field; modified by Bill Martin for the WX version
/// \date_created	10 November 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CJoinDialog class. 
/// The CJoinDialog class provides a dialog interface for the user to be able
/// to combine Adapt It documents into larger documents.
/// \derivation		The CJoinDialog class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef JoinDialog_h
#define JoinDialog_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "JoinDialog.h"
#endif

/// The CJoinDialog class provides a dialog interface for the user to be able
/// to combine Adapt It documents into larger documents.
/// \derivation		The CJoinDialog class is derived from AIModalDialog.
class CJoinDialog : public AIModalDialog
{
public:
	CJoinDialog(wxWindow* parent); // constructor
	virtual ~CJoinDialog(void); // destructor
	//enum { IDD = IDD_JOIN_DLG };
	// other methods

	// book ID code
	wxString bookID;

	// wx version pointers for dialog controls
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
	wxBitmapButton* pReject; 
	wxBitmapButton* pAccept;

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

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void MoveSelectedItems(wxListBox& From, wxListBox& To);
	void MoveAllItems(wxListBox& From, wxListBox& To);
	void InitialiseLists();
	void ListContentsOrSelectionChanged();
private:

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* JoinDialog_h */
