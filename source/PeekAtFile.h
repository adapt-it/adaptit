/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			PeekAtFile.h
/// \author			Bruce Waters
/// \date_created	14 July 2010
/// \rcs_id $Id$
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CPeekAtFileDlg class. 
/// The CPeekAtFileDlg class provides a simple dialog with a large multiline text
/// control for the user to be able to peek at as many as the first 16 kB of a selected
/// file (if the selection is multiple, only the first file in the list is used) from
/// either pane of the Move Or Copy Folders Or Files dialog, accessible from the
/// Administrator menu. This handler class shows the text Left to Right. We need two
/// buttons because once the control is created with a directionality for the text, the
/// directionality cannot be changed.
/// \derivation		The CPeekAtFileDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef PeekAtFileDlg_h
#define PeekAtFileDlg_h

class AdminMoveOrCopy; // CPeakAtFileDlg class is a friend of AdminMoveOrCopy

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "PeekAtFile.h"
#endif

enum TxtDir {
	itsLTR,
	itsRTL
};

/// The CPeekAtFileDlg class provides a simple dialog with a large multiline text control 
/// for the user to be able to peek at as many as the first 100 lines of a selected file
/// (if the selection is multiple, only the first file in the list is used) from the right
/// hand pane of the Move Or Copy Folders Or Files dialog, accessible from the
/// Administrator menu.
/// \derivation		The CPeekAtFileDlg class is derived from AIModalDialog.
class CPeekAtFileDlg : public AIModalDialog
{
	friend class AdminMoveOrCopy;
public:
	CPeekAtFileDlg(wxWindow* parent); // constructor
	virtual ~CPeekAtFileDlg(void); // destructor
	// other methods

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnClose(wxCommandEvent& event);
	void ChangeBtnLabelToLTR();
	void ChangeBtnLabelToRTL();
	enum TxtDir ReverseTextDirectionality(enum TxtDir currentDir);
	void OnBnClickedToggleDirectionality(wxCommandEvent& WXUNUSED(event));

private:
	// class attributes
	wxTextCtrl*			m_pEditCtrl;
	wxString			m_filePath;
	AdminMoveOrCopy*	m_pAdminMoveOrCopy;
	wxTextCtrl*			m_pMsgCtrl;
	wxString			Line1Str;
	wxString			Line2Str;

	// support for dynamic button and destruction/recreation of wxTextCtrl
	// (the text control has no tooltip, but the button does)
	wxBoxSizer*			m_pContainingHBoxSizer;
	wxButton*			m_pToggleDirBtn;
	wxString			m_btnToRTL_Label;
	wxString			m_btnToLTR_Label;
	wxString			m_viewedText;
	wxToolTip*			m_pToggleBtnTooltip;
	enum TxtDir			m_curDir;

	// font support (store colour while we use black)
	wxColour			m_storeColor;
	
	// other class attributes

	DECLARE_EVENT_TABLE()
};
#endif /* PeekAtFileDlg_h */