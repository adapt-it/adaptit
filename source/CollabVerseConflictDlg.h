/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CollabVerseConflictDlg.h
/// \author			Bill Martin
/// \date_created	10 July 2015
/// \rcs_id $Id$
/// \copyright		2015 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CCollabVerseConflictDlg class. 
/// The CCollabVerseConflictDlg class provides the user with a dialog that is
/// used to choose the best version to send to the external editor when
/// conflicts have been detected at save time during collaboration.
/// \derivation		The CCollabVerseConflictDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef CollabVerseConflictDlg_h
#define CollabVerseConflictDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "CollabVerseConflictDlg.h"
#endif
#include "CollabUtilities.h"

// forward references


class CCollabVerseConflictDlg : public AIModalDialog
{
public:
	CCollabVerseConflictDlg(wxWindow* parent, wxArrayPtrVoid* pConfArr); // constructor
	virtual ~CCollabVerseConflictDlg(void); // destructor
	// other methods

protected:
	wxCheckListBox* pCheckListBoxVerseRefs;
	wxTextCtrl* pTextCtrlSourceText;
	wxTextCtrl* pTextCtrlAITargetVersion;
	wxTextCtrl* pTextCtrlPTTargetVersion;
	wxButton* pBtnSelectAllVerses;
	wxButton* pBtnUnSelectAllVerses;
	wxButton* pBtnTransferSelectedVerses;
	wxButton* pBtnCancel;
	wxRadioButton* pRadioUseAIVersion;
	wxRadioButton* pRadioRetainPTVersion;
	wxTextCtrl* pStaticTextCtrlTopInfoBox; // for substituting Paratext/Bibledit into %s and %s
	wxStaticText* pStaticInfoLine1; 
	wxStaticText* pStaticInfoLine2; // for substituting Paratext/Bibledit into %s
	wxStaticText* pStaticInfoLine3; // for substituting Paratext/Bibledit into %s
	wxStaticText* pStaticInfoLine4; // for substituting Paratext/Bibledit into %s
	wxStaticText* pStaticPTVsTitle;

	wxArrayString verseRefsArray;
	wxArrayString sourceTextVsArray;
	wxArrayString aiTargetTextVsArray;
	wxArrayString ptTargetTextVsArray;
	wxArrayString ptTargetTextVsEditedArray;

	wxArrayPtrVoid* pConflictsArray;
	int CurrentListBoxHighlightedIndex; // The index of the list box's highlighted/selected item kept current
	int lastIndex; // to give access to the old index when user has clicked elsewhere in the list
	void UpdatePTorBEtext(int index, wxArrayString* ptTargetTextVsEditedArrayPtr, wxTextCtrl* pTxtCtrl);
protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnCancel(wxCommandEvent& event);
	void SyncRadioButtonsWithHighlightedItemTickState();
	void FillEditBoxesWithVerseTextForHighlightedItem();
	void OnCheckListBoxTickChange(wxCommandEvent& event);
	void OnListBoxSelChange(wxCommandEvent& WXUNUSED(event));
	void OnRadioUseAIVersion(wxCommandEvent& WXUNUSED(event));
	void OnRadioRetainPTVersion(wxCommandEvent& WXUNUSED(event));
	void OnSelectAllVersesButton(wxCommandEvent& WXUNUSED(event));
	void OnUnSelectAllVersesButton(wxCommandEvent& WXUNUSED(event));
	//void OnLeftBtnDown(wxMouseEvent& event);
	void OnOK(wxCommandEvent& event);
	void OnPTorBEtextUpdated(wxCommandEvent& event);
	void OnRestoreBtn(wxCommandEvent& WXUNUSED(event));
private:
	CAdapt_ItApp* m_pApp;
	wxString MakeVerseReference(ConflictRes* p);

	// other class attributes

	DECLARE_EVENT_TABLE()
};
#endif /* CollabVerseConflictDlg_h */
