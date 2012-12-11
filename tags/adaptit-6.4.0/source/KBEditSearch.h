/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBEditSearch.h
/// \author			Bruce Waters
/// \date_created	25 January 2010
/// \rcs_id $Id$
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the KBEditSearch class. 
/// The KBEditSearch class provides a dialog interface for the user (typically an administrator) to be able
/// to move or copy files or folders or both from a source location into a destination folder.
/// \derivation		The KBEditSearch class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef KBEditSearch_h
#define KBEditSearch_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KBEditSearch.h"
#endif

//#include <wx/datetime.h>

/// The following struct (one of two) supports the search functionality within the KB
/// Editor dialog; instances of this struct are stored in their own wxSortedArray
typedef struct
{
	wxString	updatedString;	  // either an 'adaptation' or a 'gloss' depending on which KB
	wxUint32	nMatchRecordIndex; // index to the KBMatchRecord instance which resulted from
                                  // a successful match in the KB; the KBMatchRecord has a
                                  // constant index in the array which stores it
} KBUpdateRecord;

/// Define a sorted array of void* for storing instances of KBUpdateRecord
WX_DEFINE_SORTED_ARRAY(KBUpdateRecord*, KBUpdateRecordArray);


/// The following struct (one of two) supports the search functionality within the KB
/// Editor dialog;  instances of this struct are stored in their own wxSortedArray
typedef struct
{
	wxString		strOriginal;	 // adapatation (or gloss) which was matched, before
									 // any spelling changes have been done
	KBUpdateRecord*	pUpdateRecord;	 // pointer to the KBUpdateRecord which stores the
									 // respelling of this entry; we use a pointer as
									 // the pointed at struct will flop around in the
									 // sorted array as the user edits matched items, so
									 // while the index of it will flop about, the pointer 
									 // in memory will remain constant
	wxString		strMapKey;		 // key string (i.e. source text) for the 
									 // CTargetUnit* which stores the matched adaptation
									 // (or gloss)
	CRefString*		pRefString;		 // pointer to the CRefString instance matched in the
									 // CTargetUnit instance which stores the former
} KBMatchRecord;

/// Define a sorted array of void* for storing instances of KBMatchRecord
WX_DEFINE_SORTED_ARRAY(KBMatchRecord*, KBMatchRecordArray);

int CompareMatchRecords(KBMatchRecord* struct1Ptr, KBMatchRecord* struct2Ptr);
int CompareUpdateRecords(KBUpdateRecord* struct1Ptr, KBUpdateRecord* struct2Ptr);

/// The KBEditSearch class provides a dialog interface for searching the knowledge base
/// and updating spellings for adaptations or glosses. It is derived from AIModalDialog.
class KBEditSearch : public AIModalDialog
{
public:
	KBEditSearch(wxWindow* parent); // constructor
	virtual ~KBEditSearch(void); // destructor

	CKBEditor* pKBEditorDlg; // pointer to the parent KB Editor dialog

	// flags 

	// pointers for dialog controls
	wxButton* m_pOKButton;
	wxButton* m_pCancelButton;
	wxButton* m_pRemoveUpdateButton;
	wxButton* m_pUpdateButton;
	wxButton* m_pFindNextButton;
	wxButton* m_pRestoreOriginalSpellingButton;

	wxTextCtrl* m_pSrcPhraseBox;
	wxTextCtrl* m_pNumReferencesBox;
	wxTextCtrl* m_pLocalSearchBox;
	wxTextCtrl* m_pEditBox;

	wxListBox* m_pMatchListBox;
	wxListBox* m_pUpdateListBox;

	CKB* m_pKB; // whichever KB the parent CKBEditor instance is opened on (m_pKB of 
				// Adapt_It.h, which becomes pKB of parent CKBEditor instance)
	// BEW removed 28May10, because TUList is redundant & now removed
	//TUList*		m_pTUList; // pointer to the list of CTargetUnit pointers stored in m_pKB

	// in the next 4 members, the wxArrayString members are for the labels in the
	// listboxes, which will be seen by the user; the KBMatchRecordArray and
	// KBUpdateRecordArray pointers hold the KBMatchRecord struct, and KBUpdateRecord
	// struct, the "data" which are associated with the labels which the user sees
	KBMatchRecordArray* m_pMatchRecordArray; // from WX_DEFINE_SORTED_ARRAY
				// macro and storing struct KBMatchRecord pointers (see Adapt_It.h)
	KBUpdateRecordArray*  m_pUpdateRecordArray; // from WX_DEFINE_SORTED_ARRAY
				// macro and storing struct KBUpdateRecord pointers (see Adapt_It.h)
	wxArrayString* m_pMatchStrArray; // on heap, label strings for the Matched listbox
									 // (obtained from KBMatchRecord.strOriginal values)
	KBMatchRecord* m_pDummyMatchRecord;
	 
	// data transfer of the user's choice of search string, or search strings, is via a
	// wxArrayString defined in Adapt_It.h, called m_arrSearches. In the same place is a
	// second wxArrayString which stores, (accumulating during whole duration of work
	// within the project folder) all earlier search strings from previous searches; it is
	// called m_arrOldSearches. The latter is used for quickly re-doing a search done
	// earlier. 

	wxString m_strLocalSearch; // current local search string from pLocalSearchBox
	wxString m_strEditBox; // adaption (or gloss) in the pEditBox which is potentially 
						   // to be respelled
	wxString m_strSourceText; // source text associated with the selected matched adaption 
							  // (or gloss)
	wxString m_strNumRefs; // number of references to the matched adaption (or gloss) string

	KBMatchRecord* m_pCurKBMatchRec;
	KBUpdateRecord* m_pCurKBUpdateRec;
	int	m_nCurMatchListIndex;
	int m_nCurUpdateListIndex;

protected:

	void EnableFindNextButton(bool bEnableFlag);
	void EnableUpdateButton(bool bEnableFlag);
	void EnableRestoreOriginalSpellingButton(bool bEnableFlag);
	void EnableRemoveUpdateButton(bool bEnableFlag);

	void OnOK(wxCommandEvent& event);
	void OnBnClickedCancel(wxCommandEvent& event);
	void OnBnClickedUpdate(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedFindNext(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRestoreOriginalSpelling(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRemoveUpdate(wxCommandEvent& WXUNUSED(event));

	void OnEnterInEditBox(wxCommandEvent& WXUNUSED(event));
	void OnChangeLocalSearchText(wxCommandEvent& WXUNUSED(event));

	void OnMatchListSelectItem(wxCommandEvent& event);
	void OnMatchListDoubleclickItem(wxCommandEvent& event);

	void OnUpdateListSelectItem(wxCommandEvent& event);
	void OnUpdateListDoubleclickItem(wxCommandEvent& event);

private:
	bool m_bMatchesExist;
	unsigned int GetUpdateListIndexFromDataPtr(KBUpdateRecord* pCurRecord);
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void SetupMatchArray(wxArrayString* pArrSearches,	// pass in pointer to app's m_arrSearches
						CKB* pKB, // the knowledge base instance whose contents we are testing
						KBMatchRecordArray* pMatchRecordArray, // return matched strings here
						bool* pbIsGlossing); // need this boolean, it defines how many maps we test
	bool TestForMatch(wxArrayPtrVoid* pSubStringSet, wxString& testStr);
	void PopulateMatchedList(wxArrayString* pMatchStrArray, KBMatchRecordArray* pMatchRecordArray, 
						wxListBox* pListBox);
	bool PopulateMatchLabelsArray(KBMatchRecordArray* pMatchRecordArray, wxArrayString* pMatchStrArray);
	void SetMatchListSelection(int nSelectionIndex, bool bUserClicked = TRUE);

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* KBEditSearch_h */
