/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			RemoveSomeTgtEntries.h
/// \author			Bruce Waters
/// \date_created	14 October 2013
/// \rcs_id $Id: RemoveSomeTgtEntries.h 2883 2013-10-14 03:58:57Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the RemoveSomeTgtEntries class.
/// The RemoveSomeTgtEntries class provides a handler for the "Remove Some Translations..."
/// button in the KB Editor. It provides a wxListCtrl with each line being the target text
/// adaptation, the source text for that adaptation, and a reference count - displayed in 4
/// columns, column 0 being a picture of either an empty checkbox, or a ticked checkbox.
/// Two organising principles are offered by radio buttons, keep the groups of adaptations
/// (or glosses) together according to which source text they are associated with (but
/// alphabetizing within each such group), or simply alphabetizing the target text
/// adaptations (or glosses) altogether (which splits up the source text groupings, but may
/// be easier to use for some situations - such as when working in dialects with very few
/// adaptation variants per source text key). A button is also provided for sending the
/// currently displayed form of the list to a file. The default button closes the dialog
/// and the checked lines are used to define a deletions on the KB entry which are done as
/// soon as the dialog is dismissed. A Cancel button is also provided. The deletions done
/// are standard "pseudo-deletions" - so the items no longer are seen in the KB Editor, but
/// are present in the kb as pseudo-deletions, and so will propagate to other clients if KB
/// Sharing is turned on. This feature was added to help with mass editing when spelling or
/// diacritic changes are many, at a user's request.
/// The wxDesigner resource is Remove_Some_Tgt_Entries_Func
/// \derivation		The RemoveSomeTgtEntries class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef RemoveSomeTgtEntries_h
#define RemoveSomeTgtEntries_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "RemoveSomeTgtEntries.h"
#endif

// forward declarations
class CKBEditor;
class CKB;
class CTargetUnit;

typedef struct {
	wxString	src;
	wxString	nonsrc;
	int			numrefs;
} NonSrcListRec;

enum iconImage {
	emptyCheckbox = 0,
	tickedCheckbox
};


WX_DEFINE_SORTED_ARRAY(NonSrcListRec*, SortedNonSrcListRecsArray);
// Array comparison function, it compares the nonsrc wxString in each NonSrcListRec instance
int	CompareNonSrcListRecs(NonSrcListRec* rec1, NonSrcListRec* rec2);
// Define a non-sorted one in which we'll accumulate ptrs; it will receive pointers from
// the SortedNonSrcListRecsArray sorted one - this NonSrcListRecsArray will be what we use
// to populate our grouped-by-src list option.
WX_DEFINE_ARRAY(NonSrcListRec*, NonSrcListRecsArray); // this type is unsorted (we pre-sort externally)
// We will also create (in the .cpp file) a second sorted array called m_ungroupedArray - of
// NonSrcListRec type. It will just take all the pointers without consideration
// of their associated non-source text grouping - this will support our second radio button
// which gives a simple list sorted alphabetically by target text, or by gloss text in the
// case we are dealing with a glossing KB


// We want our source text, for the grouping array, to be in alphabetical order too. The
// source text is the key in CTargetUnit instances, so we need a sorted array of
// CTargetUnit instances in order to get the ording right before we transfer the
// NonSrcListRec-produced strings to the m_groupedArray for display. However, CTargetUnit
// does not store the associated source text key. That comes from the map iterator's pair,
// so we have to define a struct which keeps the association, so we can do the required
// sorting
typedef struct {
	wxString		src;
	CTargetUnit*	pTU;
} SrcTgtUnitPair;
WX_DEFINE_SORTED_ARRAY(SrcTgtUnitPair*, SortedSrcTgtUnitsArray);
// And a compare function for the above
int CompareSrcTargetUnits(SrcTgtUnitPair* pTU1, SrcTgtUnitPair* pTU2);

WX_DEFINE_ARRAY_INT(int,TrackingArray); // store 0 or 1 to track which pseudo-checkboxes
										// are shown as ticked within a view

class RemoveSomeTgtEntries : public AIModalDialog
{
public:
	RemoveSomeTgtEntries(wxWindow*	parent); // constructor
	virtual ~RemoveSomeTgtEntries(); // destructor
	CKB*	m_pKB;
	bool	m_bBySrcGroups;
	NonSrcListRecsArray* GetGroupedArray();
	SortedNonSrcListRecsArray* GetSortedArray();
	TrackingArray* GetLeftTrackingArray(); // Gets m_leftCheckedArray
	TrackingArray* GetRightTrackingArray();  // Gets m_rightCheckedArray

	// member variables

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnBtnSaveEntryListToFile(wxCommandEvent& WXUNUSED(event));
	void OnCancel(wxCommandEvent& event);
	void OnRadioOrganiseByKeys(wxCommandEvent& WXUNUSED(event));
	void OnRadioListTgtAlphabetically(wxCommandEvent& WXUNUSED(event));

	size_t	GetTotalKBEntries(CKB* pKB); // this counts non-deleted, non-<Not In KB>, m_translation strings
	size_t	GetTotalTargetUnits(CKB* pKB); // this counts just the CTargetUnit instances
	void	PopulateSrcTargetUnitPairsArray(CKB* pKB, SortedSrcTgtUnitsArray* pArray);

	void	PopulateOneTargetUnitGroupArray(SrcTgtUnitPair* pPair, SortedNonSrcListRecsArray* pArray);
	// Next is similar to the one above, except the param2 array is unsorted
	void	PopulateOneTargetUnitUnsortedArray(SrcTgtUnitPair* pPair, NonSrcListRecsArray* pArray);
	void	PopulateGroupsArray(SortedSrcTgtUnitsArray* pSortedTUArray, NonSrcListRecsArray* pArray);
	// Next is the one we use for getting the structs ordered top to bottom by the target
	// text, which will be shown first. (This is for the right radio button).
	void	PopulateTargetSortedArray(SortedSrcTgtUnitsArray* pSortedTUArray, SortedNonSrcListRecsArray* pArray);
	void	SetCheckboxes(bool bBySrcGroups); // restores ticks to checkboxes when view is changed
	void	DoKBExportOfListedEntries(wxFile* pFile, bool bBySrcGroups);
	long	SearchInSortedArray(SortedNonSrcListRecsArray* pArray, NonSrcListRec* pFindThis); // replaces unreliable .Index() call
	long	SearchInUnsortedArray(NonSrcListRecsArray* pArray, NonSrcListRec* pFindThis); // ditto
	void	GetPhrasePairsForBulkRemoval(TrackingArray* pCheckboxTickedArray,
				NonSrcListRecsArray* pArray, wxArrayString* pSrcPhrases, wxArrayString* pTgtPhrases);
	void	StoreChoice(bool bBySrcGroups, long myline, int nChoice); // nChoice should only be 0 or 1

	// Next 3 used to be used for wxCheckListBox, as well as the Save to File button; but
	// in the refactored version, we use them only for the Save to File button, and
	// MakeListLine() has simplifications in it - to pad by a constant 10 spaces, and to
	// show [Source] preceding the source text
	wxString MakeListLine(NonSrcListRec* pStruct);
	void	MakeLinesArray(NonSrcListRecsArray*	pGroupsArray, wxArrayString& rLinesArray);
	// Next one is similar, but takes a sorted (by target or gloss) array as first param
	void	MakeLinesForSortedArray(SortedNonSrcListRecsArray* pTargetSortedArray, wxArrayString& rLinesArray);

	// Refactored stuff uses these functions (don't need the MakeLinesForSortedArray() or
	// MakeLinesArray() functions, nor MakeListLine()
	void	PopulateListAsGroups(NonSrcListRecsArray* pGroupsArray);
	void	PopulateListAsTgtSorted(SortedNonSrcListRecsArray* pNonSrcArray);
	void	OnCheckboxSelection(wxListEvent& event);

	// Do the bulk pseudo-deletions from the local KB with this
	void	DoLocalBulkKbPseudoDeletions(bool bIsGlossingKB);

private:
	// class attributes
	bool	m_bCurrentValue; // keep hold of m_bBySrcGroups value wwhen a radio button
							 // is being clicked & therefore potentially changing it
	// For dealing with item selection clicks and pseudo-checkbox values
	long	m_nOtherLine; // the equivalentline index in the view not displayed
	int		m_nStoredCheckValue; // the 0 0r 1 value in the currently active int array (for tracking)
	NonSrcListRec* m_pSelectedRec;

	size_t	m_totalKBEntries;
	wxString m_src;
	wxString m_nonsrc;
	SortedNonSrcListRecsArray* m_pOneGroupArray; // from a single CTargetUnit instance, sorted
	NonSrcListRecsArray*	   m_pOneTUUnsortedArray; // from a single CTargetUnit instance, unsorted
	SortedNonSrcListRecsArray* m_pUngroupedTgtSortedArray; // what comes from all m_translation
														   // instances of the KB, sorted
	NonSrcListRecsArray*	   m_pGroupsArray; // stores alphabetically ordered (by src key)
           // groups of NonSrcListRec ptrs note: this is partially sorted: the groups are
           // alphabetically sorted by src text key and within the groups the NonSrcListRec
           // ptrs are alphabetically sorted by target (or gloss) text. We do an top to
           // bottom transfer of the structs as formatted lines into a wxArrayString which
           // is then used to populate the wxListCtrl
	SortedSrcTgtUnitsArray*	   m_pSortedSrcTgtUnitPairsArray; // m_groupsArray is populated from this
	wxArrayString	m_linesArray; // 1-to-1 mapping of each NonSrcListRec ptr with each line
								  // & we load the wxListCtrl from this array when in
								  // the view specified by the left radio button, the
								  // "grouped-by-source-text" list
	wxArrayString	m_linesArrayTgtSorted; // 1-to-1 mapping of each NonSrcListRec ptr with
								  // each line & we load the wxListCtrl from this array when
								  // in the view specified by the right radio button, the
								  // top-to-bottom alphabetized target(or gloss) text's list
	TrackingArray	m_leftCheckedArray; // for view seen when left radio button is on
	TrackingArray	m_rightCheckedArray; // for view seen when right radio button is on
	wxSizer*		m_pRemoveSomeSizer;
	CKBEditor*		m_pKBEditorDlg;
	wxRadioButton*	m_pRadioOrganiseByKeys;
	wxRadioButton*	m_pRadioListTgtAlphabetically;
	wxListView*		m_pCheckList;
	bool			m_bIsGlossingKB;
	wxString		m_output;
	wxString		m_ref_many;
	wxString		m_ref_once;
	wxString		m_no_adaptation;
	wxString		m_no_gloss;
	CAdapt_ItApp*	m_pApp;
	int screenHeight;
	wxImageList*    pIconImages;
	// next ones retain in case later needed in MakeListLine() if legacy format is restored
	//wxString		m_spaces[39];
	//wxString		m_firstSpaces[2]; // we want two different gaps
	//wxString		m_secondSpaces[2]; // ditto

	DECLARE_EVENT_TABLE()
};
#endif /* RemoveSomeTgtEntries_h */
