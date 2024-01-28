/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Adapt_ItDoc.h
/// \author			Bill Martin
/// \date_created	05 January 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public 
///                 License (see license directory)
/// \description	This is the header file for the CAdapt_ItDoc class. 
/// Adapt It Document (CAdapt_ItDoc) class. 
/// The CAdapt_ItDoc class implements the storage structures and methods 
/// for the Adapt It application's persistent data. Adapt It's document 
/// consists mainly of a list of CSourcePhrases stored in order of occurrence 
/// of source text words. The document's data structures are kept logically 
/// separate from and independent of the view class's in-memory data structures. 
/// This schema is an implementation of the document/view framework. 
/// \derivation		The CAdapt_ItDoc class is derived from wxDocument.
/////////////////////////////////////////////////////////////////////////////

#ifndef Adapt_ItDoc_h
#define Adapt_ItDoc_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "Adapt_ItDoc.h"
#endif

// Forward declarations
class wxDataOutputStream;
class wxDataInputStream;
class wxFile;
class CAdapt_ItApp;
class CAdapt_ItDoc;
class CLayout;
class CFreeTrans;
class CNotes;
struct AutoFixRecord;
struct AutoFixRecordG;
class CStatusBar;

// Comment out to disable debugging code for the ZWSP etc support
//#define DEBUG_ZWSP

#if defined(_DEBUG) && defined(DEBUG_ZWSP)
// copied from PunctCorrespPage.cpp for use here while debugging
wxString    MakeUNNNN(wxString& chStr);
#endif

/// wxList declaration and partial implementation of the AFList class being
/// a list of pointers to AutoFixRecord objects
WX_DECLARE_LIST(AutoFixRecord, AFList); // see list definition macro in .cpp file
WX_DECLARE_LIST(AutoFixRecordG, AFGList); // see list definition macro in .cpp file
// BEW added 11Jul23
//WX_DECLARE_LIST(CSourcePhrase*, mySPList); // definition in .cpp file at 141

// for debugging, uncomment out to turn on the debugging displays (they are very helpful)
//#define CONSCHK
#define CONSCHK2

enum SaveType {
	normal_save,
	save_as
};

enum PunctChangeType {
	was_empty_stays_empty,
	was_empty_added,
	exists_removed,
	exists_replaced,
	exists_unchanged
};

enum WordParseEndsAt {
	unknownCharType,
	endoftextbuffer,
	whitespace,
	closingbracket,
	punctuationchar,
	backslashofmkr
};

// GDLC Putting this enum definition here creates 40 compile errors on MacOS
// Moving it to helpers.h eliminates the 40 errors.
//enum WhichLang {
//	sourceLang,
//	targetLang
//};

// NOTE: in these next two enums, "flag" refers to m_bHasKBEntry when in adapting mode, or
// m_bHasGlossingKBEntry when in glossing mode.
enum InconsistencyType {
	member_empty_flag_on_noPTU, // done, use ConsistencyCheck_EmptyNoTU_DlgFunc()
	member_exists_flag_on_noPTU, // done, use ConsistencyCheck_ExistsNoTU_DlgFunc()
	member_exists_flag_off_noPTU, // done, use ConsistencyCheck_ExistsNoTU_DlgFunc()
	member_empty_flag_on_PTUexists_deleted_Refstr, // done, use ConsistencyCheck_EmptyNoTU_DlgFunc()
	member_exists_flag_on_PTUexists_deleted_Refstr, // done, use revamped legacy dlg... 
													// this is the "split meaning" possibility
													member_exists_flag_off_PTUexists_deleted_RefStr, // done,  use revamped legacy dlg here too
													flag_on_NotInKB_off_hasActiveNotInKB_in_KB, // done // use dlg for either normal entry or <Not In KB> 
																	// m_bNotInKB TRUE and ensure pTU have valid <Not In KB> entry
													// BEW added next, 1Sep15, to support the "blind fix" feature Mike Hore requested
													// ?? dunno what became of that!
													// BEW 15Apr19 support Mike Hore's wish to edit KB extensively or a bit, and
													// use ConsistencyCheck() to get his new adaptation choices into the doc at the
													// right places
													member_exists_deleted_from_KB_KB_has_translations
};

// some of these enum values are not actually needed, because they all are handled in ways
// that can be deduced without the enums; we'll keep them, in case we later want to
// support them
enum FixItAction {
	no_GUI_needed,
	// next three are responses for member_empty_flag_on_noPTU, and 4th response is a
	// possiblity for the member_exists_flag_off_noPTU case; the 1st response is also a
	// possiblity for member_empty_flag_on_PTUexists_deleted_Refstr
	turn_flag_off, // make m_bHasKBEntry or m_bHasGlossingKBEntry FALSE
	make_it_Not_In_KB, // adjust flags, make CTargetUnit & add <Not In KB> (only available 
					   // in adaptation mode)
					   store_empty_meaning, // the <no adaptation> response, or <no gloss> response
					   store_nonempty_meaning, // store text for m_adaption or m_gloss
					   // next are responses (only two are possible) for member_exists_flag_off_noPTU (note, 
					   // includes also the 3rd response for the member_empty_flag_on_noPTU - but only when 
					   // m_bNotInKB was FALSE)
					   turn_flag_on, // make m_bHasKBEntry or m_bHasGlossingKBEnty TRUE, & do StoreText()

					   // next are the responses for member_empty_flag_on_PTUexists_deleted_Refstr (three
					   // only), one is turn_flag_off, another is make_it_an_empty_entry, and another is
					   restore_meaning_to_doc,

					   // BEW 15Apr19 added this one to ensure the user gets a go at choosing from what
					   // the KB has for non-deleted entries for the given pTU
					   user_list_choice
};

// struct for storing auto-fix inconsistencies when doing "Consistency Check..." menu item;
// for glossing we can use the same structure with the understanding that the oldAdaptation
// and finalAdaptation will in reality contain the old gloss and the final gloss, respectively
// BEW 29Aug11, added two enums and split into two stucts, the second being a "G" variant
// for when dealing with glossing KB (in glossing mode) -- also two processing functions
// Note: oldAdaptation and finalAdaptation are needed so that when autofixing, comparison
// can be made of these two to see if the user has changed the adaptation during the check;
// so in DoConsistencyCheck() and DoConsistencyCheckG(), the oldAdaptation member is only
// set, never used; but it IS used in helper functions such as MatchAutofixItem() and so is
// NOT redundant & removable from the code
struct	AutoFixRecord
{
	wxString	key;
	wxString	oldAdaptation;
	wxString	finalAdaptation;
	int			nWords;
	enum InconsistencyType incType;
	enum FixItAction fixAction;
};
struct	AutoFixRecordG
{
	wxString	key;
	wxString	oldGloss;
	wxString	finalGloss;
	int			nWords;
	enum InconsistencyType incType;
	enum FixItAction fixAction;
};
// Next one for support of hiding (and restoring) USFM3 
// attribute-having metadata
enum AttrMkrType {
	notAttrMkr,
	attrBeginMkr,
	attrEndMkr
};

// whm 15Nov2023 added for support of usfm struct file and
// the filtering/unfiltering of multiple adjacent usfm markers.
enum UsfmStructFileProcess {
	createNewFile,
	recreateExistingFile,
	openExistingFile,
	createFromSPList
};


// If we need another list based on CSourcePhrase, we don't declare it
// with another macro like the one above, but instead we simply use 
// the SrcPList class that is declared by the macro to declare 
// additional instance list object memebers within the class we want to
// use them. For example: SrcPList* m_pSaveList;

/// The CAdapt_ItDoc class implements the storage structures and methods 
/// for the Adapt It application's persistent data. Adapt It's document 
/// consists mainly of a list of CSourcePhrases stored in order of occurrence 
/// of source text words. The document's data structures are kept logically 
/// separate from and independent of the view class's in-memory data structures. 
/// This schema is an implementation of the document/view framework. 
/// \derivation		The CAdapt_ItDoc class is derived from wxDocument.


class CAdapt_ItDoc : public wxDocument
{
protected: // MFC version comment: create from serialization only
  //CAdapt_ItDoc();
  // Note: In the MFC version the Doc's constructor can apparently have
  // "protected" access even with the DECLARE_DYNCREATE(CAdapt_ItDoc) 
  // and IMPLEMENT_DYNCREATE(CAdapt_ItDoc, CDocument) macros attached
  // to the Doc class. Under wxWidgets we must implement serialization 
  // manually, and so cannot have a protected constructor for the document

	DECLARE_DYNAMIC_CLASS(CAdapt_ItDoc)
	// DECLARE_DYNAMIC_CLASS() is used inside a class declaration to 
	// declare that the objects of this class should be dynamically 
	// creatable from run-time type information. 
	// MFC uses DECLARE_DYNCREATE(CAdapt_ItDoc)

	// Attributes
public:
	CAdapt_ItDoc(); // moved constructor to public access for wxWidgets 
					// version (see note above)

	//////////////////////////////////////////////////////////////////////////////
	// All of the Doc's public data members were moved to the App class' public 
	// area, in Adaptit.h.
	// SINCE POINTERS TO THE CURRENT DOCUMENT CAN CHANGE, DO NOT PUT DATA MEMBERS
	// HERE, BUT RATHER PLACE THEM IN THE APP CLASS' PUBLIC AREA.
	//////////////////////////////////////////////////////////////////////////////

	// Operations
public:

	// Overrides
public:

	void OnFileNew(wxCommandEvent& event);
	virtual bool OnNewDocument();
	//virtual void Serialize(CArchive& ar); // Serialize not used in wxWidgets version
	virtual bool DeleteContents(); // virtual void in MFC and virtual bool in wx but not documented in wx
	virtual bool OnCloseDocument(); // virtual void type in MFC
	//virtual bool OnSaveDocument(const wxString& filename);// OnSaveDocument not used in the MFC version
	virtual bool OnOpenDocument(const wxString& filename, bool bShowProgress = true);
	virtual bool IsModified() const;
	virtual void Modify(bool mod);

	virtual bool OnSaveModified(); // in protected area of MFC app
	bool         DoAbsolutePathFileSave(wxString absPath); // BEW created 7Sep15

	// Implementation
protected:
	void			AddParagraphMarkers(wxString& rText, int& rTextLength);
	bool			AnalyseMarker(CSourcePhrase* pSrcPhrase, CSourcePhrase* pLastSrcPhrase,
		wxChar* pChar, int len, USFMAnalysis* pUsfmAnalysis);
	bool			BackupDocument(CAdapt_ItApp* WXUNUSED(pApp), wxString* pRenamedFilename = NULL);
	//int				ClearBuffer(); // whm 4Sep2023 removed along with App's buffer
	CBString		ConstructSettingsInfoAsXML(int nTabLevel); // BEW added 05Aug05 for XML doc output support
	int				ContainsMarkerToBeFiltered(enum SfmSet sfmSet, wxString markers, wxString filterList,
		wxString& wholeMkr, wxString& wholeShortMkr, wxString& endMkr, bool& bHasEndmarker,
		bool& bUnknownMarker, int startAt);
	void			CopyFlags(CSourcePhrase* dest, CSourcePhrase* src);
	void			DeleteListContentsOnly(SPList*& pList);
	bool			DoUnpackDocument(wxFile* pFile);
	bool			IsPreviousTextTypeWanted(wxChar* pChar, USFMAnalysis* pAnalysis);
	bool			IsPunctuationOnlyFollowedByEndmarker(wxChar* pChar, wxChar* pEnd,
		wxString& spacelessPuncts, bool bTokenizingTargetText,
		bool& bHasPunctsOnly, bool& bEndmarkerFollows,
		int& punctsCount); // BEW added 4Apr2017 for TokenizeText()
	void			GetMarkerMapFromString(MapWholeMkrToFilterStatus*& pMkrMap, wxString str); // used in SetupForSFMSetChange
	wxString		GetNextFilteredMarker(wxString& markers, int offset, int& nStart, int& nEnd);
	wxString		GetNextFilteredMarker_After(wxString& markers, wxString& filteredInfo_After,
		wxString& metadata, int& offset, int& nEnd);
	bool			IsEndingSrcPhrase(enum SfmSet sfmSet, CSourcePhrase* pSrcPhrase, wxString &filterInfo); // whm 24Oct2023 added last parameter
	bool			IsEndMarkerForTextTypeNone(wxChar* pChar);
	//bool			IsBeginMarkerForTextTypeNone(wxChar* pChar); // BEW 22Apr20
	wxString		GetLastEndMarker(wxString endMkrs); //BEW added 31May23 for use in propagation code (in TokenizeText)

	// BEW 30May17 next two for supporting inLine markers within a inLine span, such as a
	// \xt marker within an unfiltered \f ... \f* span
	bool			m_bIsWithinUnfilteredInlineSpan;
	wxString		m_strUnfilteredInlineBeginMarker;
	// BEW 9Sep16 added next four
	bool			IsInWordProper(wxChar* ptr, wxString& spacelessPuncts); // TRUE if not punct, ~, marker,  not [ or ], not whitespace etc
	inline bool		IsFixedSpace(wxChar* ptr); // TRUE if it is a ~ (tilde), the USFM fixed-space character
	wxString		m_spacelessPuncts; // populated by a TokenizeText() call (IsInWordProper() uses it)
	wxString		m_spacelessPuncts_NoTilde; // same as m_spacelessPuncts, but lacking a ~ character
public:
	// public because USFM3Support.cpp uses it too (BEW 3Sep19)
	bool			m_bTokenizingTargetText; // set by fourth parameter of a TokenizeText() call (IsInWordProper() uses it)
	bool			m_bWmkrAndHasBar; // BEW 12Sep22 added, to save value returned by IsWmkrWithBar(wxChar* ptr), used with caching some USFM 3 stuff
	wxString		m_strInitialPuncts; // BEW 23Jun23 a set of 11 pre-word punctuation chars, taken from spacelessPuncts to
										// help our ParseWord() parser to know when a post-word punct belongs in m_precPunct on next pSrcPhrase
	bool			WordBeginsHere(wxChar chFirst, wxString spacelessPuncts);
	bool			bKeepPtrFromAdvancing; // BEW 8Sep23 moved here from within TokenizeText() so that ParseWord() can access it
	wxString		ParseNumberHyphenSuffix(wxChar* pChar, wxChar* pEnd, wxString spacelessPuncts); // BEW added 16Nov23

protected:
	bool			IsFixedSpaceAhead(wxChar*& ptr, wxChar* pEnd, wxChar*& pWdStart,
		wxChar*& pWdEnd, wxString& punctBefore, wxString& endMkr,
		wxString& wordBuildersForPostWordLoc, wxString& spacelessPuncts,
		bool bTokenizingTargetText); // BEW created 11Oct10, 24Oct14 added final bool
	void			FinishOffConjoinedWordsParse(wxChar*& ptr, wxChar* pEnd, wxChar*& pWord2Start,
		wxChar*& pWord2End, wxString& punctAfter, wxString& bindingMkr,
		wxString& newPunctFrom2ndPreWordLoc, wxString& newPunctFrom2ndPostWordLoc,
		wxString& wordBuildersFor2ndPreWordLoc, wxString& wordBuildersFor2ndPostWordLoc,
		wxString& spacelessPuncts, bool bTokenizingTargetText);
	bool			IsUnstructuredPlainText(wxString& rText);
	void			MakeOutputBackupFilenames(wxString& curOutputFilename);
	bool			NotAtEnd(wxString& rText, const int nTextLength, int nFound);
	bool			ParseWordMedialSandwichedPunct(wxChar* pText, wxChar* pEnd, wxString& spacelessPuncts); // BEW added 11Sep16
	bool			ParseWordMedialSandwichedUSFMFixedSpace(wxChar* pText, wxChar* pEnd, wxString& spacelessPuncts); // BEW added 11Sep16

public:
	void			OverwriteUSFMFixedSpaces(wxString*& pstr);
	void			OverwriteUSFMDiscretionaryLineBreaks(wxString*& pstr);
	void			PutPhraseBoxAtDocEnd();
	bool			ReOpenDocument(CAdapt_ItApp* pApp,
	wxString        savedWorkFolderPath,			// for setting current working directory
	wxString        curOutputPath,					// includes filename
	wxString        curOutputFilename,				// to help get window Title remade
//      int		 curSequNum, // for resetting the box location - mrh: now not needed since the seq num is saved in the xml.
bool	 savedBookmodeFlag,				// for ensuring correct mode
bool	 savedDisableBookmodeFlag,		// ditto
BookNamePair* pSavedCurBookNamePair,  // for restoring the pointed at struct
int		 savedBookIndex,				// for restoring the book folder's index in array
bool	 bMarkAsDirty);					// might want it instantly saveable

protected:
#ifndef __WXMSW__
#ifndef _UNICODE
public:
	void			OverwriteSmartQuotesWithRegularQuotes(wxString*& pstr);
protected:
#endif
#endif
	void			RemoveVenturaOptionalHyphens(wxString*& pstr);
	bool			ReconstituteAfterPunctuationChange(CAdapt_ItView* pView, SPList*& pList,
		SPList::Node* pos, CSourcePhrase*& pSrcPhrase, wxString& fixesStr);
	bool			ReconstituteOneAfterPunctuationChange(CAdapt_ItView* pView, SPList*& WXUNUSED(pList),
		SPList::Node* WXUNUSED(pos), CSourcePhrase*& pSrcPhrase,
		wxString& WXUNUSED(fixesStr), SPList*& pNewList, bool bIsOwned);
	bool			ReconstituteAfterFilteringChange(CAdapt_ItView* pView, SPList*& pList, wxString& fixesStr);
	void			SetupForSFMSetChange(enum SfmSet oldSet, enum SfmSet newSet, wxString oldFilterMarkers,
		wxString newFilterMarkers, wxString& secondPassFilterStr, enum WhichPass pass);
	void			SmartDeleteSingleSrcPhrase(CSourcePhrase* pSrcPhrase, SPList* pOtherList);
	void			ValidateFilenameAndPath(wxString& curFilename, wxString& curPath, wxString& pathForSaveFolder);

public:
	void			AdjustSequNumbers(int nValueForFirst, SPList* pList);
	wxString& AppendFilteredItem(wxString& dest, wxString& src);
	wxString& AppendItem(wxString& dest, wxString& src, const wxChar* ptr, int itemLen);

	// partner pile functions (for refactored layout support)
	void			CreatePartnerPile(CSourcePhrase* pSrcPhrase); // added 13Mar09
	void			DeletePartnerPile(CSourcePhrase* pSrcPhrase); // added 12Mar09
	void			MarkStripInvalid(CPile* pChangedPile); // added 29Apr09, adds strip index to m_invalidStripArray
	void			ResetPartnerPileWidth(CSourcePhrase* pSrcPhrase,
	bool bNoActiveLocationCalculation = FALSE); // added 13Mar09, changed 29Apr09
// end of partner pile functions

	void			DeleteSingleSrcPhrase(CSourcePhrase* pSrcPhrase, bool bDoPartnerPileDeletionAlso = TRUE);
	void			DeleteSourcePhrases(); // deletes all the CSourcePhrase instances,in m_pSourcePhrases,
										   // but does not delete each partner pile (use DestroyPiles()
										   // defined in CLayout for that)
	void			DeleteSourcePhrases(SPList* pList, bool bDoPartnerPileDeletionAlso = FALSE);
	bool			DoFileSave_Protected(bool bShowWaitDlg, const wxString& progressTitle);
	bool			DoFileSave(bool bShowWaitDlg, enum SaveType type, wxString* pRenamedFilename,
		bool& bUserCancelled, // BEW added bUserCancelled 20Aaug10
		const wxString& progressTitle);
	bool			DoCollabFileSave(const wxString& progressTitle, wxString msgDisplayed); // whm added 17Jan12
	void			DoMarkerHousekeeping(SPList* pNewSrcPhrasesList, int WXUNUSED(nNewCount),
		TextType& propagationType, bool& bTypePropagationRequired);
	bool			DoPackDocument(wxString& exportPathUsed, bool bInvokeFileDialog = TRUE);
	bool			DoTransformedDocFileSave(wxString path);
	void			EraseKB(CKB* pKB);
	bool			FilenameClash(wxString& typedName); // BEW added 22July08 to 
														// prevent 2nd creation destroying work
	WordParseEndsAt	FindOutWhatIsAtPtr(wxChar* ptr, wxChar* pEnd, bool bTokenizingTargetText); // BEW added 12Oct16
	bool			FindMatchingEndMarker(wxChar* ptr, wxChar* pEnd, int& offsetToMatchedEndmarker, int& endMarkerLen);
	bool			FindClosingParenthesis(wxChar* pChar, wxChar* pEnd, int& nSpanLen, int nLimit, bool bAlreadyGotOpeningParen);
	CAdapt_ItApp*	GetApp();
	wxString		GetCurrentDirectory();	// BEW added 4Jan07 for saving & restoring the full path
											// to the current doc's directory across the writing of
											// the project configuration file to the project's directory
	int				GetCurrentDocVersion();

	// whm 10Nov2023 added the following to aid in insertion of marker-being-unfiltered into pList
	// when a previous adjacent marker must occur under the current marker-being-unfiltered.
	/*
	// The following are currently unused
	bool			ThisMarkerMustRelocateBeforeOfAfterAdjacentMarker(SPList::Node* currPos, 
					SPList::Node*& prevPos, wxString occursUnderStr);
	wxString		GetAdjacentUsfmMarkersAndTheirFilterStatus(wxString mkr, wxString ChVs, wxArrayString m_UsfmStructArr, 
					wxString& filterStatusStr, wxString& filterableMkrStr);
	*/
	void			ParseUsfmStructLine(wxString line, wxString& mkr, wxString& numChars, wxString& filterStatus);
	wxArrayString	m_UsfmStructArr;
	wxString		m_UsfmStructStringBuffer;
	wxString		m_usfmStructFilePathAndName;
	wxString		m_usfmStructDirName;
	wxString		m_usfmStructDirPath;
	wxString		m_usfmStructFilePath;
	wxString		m_usfmStructFileName;
	bool			SetupUsfmStructArrayAndFile(enum UsfmStructFileProcess fileProcess, wxString& 
						pInputBuffer, SPList* pList = NULL);
	bool			m_bUsfmStructEnabled;
	bool			FilteredMaterialContainsMoreThanOneItem(wxString filteredStuff);
	void			GetFilterableMarkersFromString(wxString tempMkrs, 
						wxArrayString& filteredMkrsArrayWithFilterBrackets, wxArrayString& filterableMkrsArray);
	void			UpdateCurrentFilterStatusOfUsfmStructFileAndArray(wxString usfmStructFileNameAndPath);
	wxString		ReorderFilterMaterialUsingUsfmStructData(wxString filterStr, wxString ChVs, wxArrayString m_UsfmStructArr);
	int				GetLowestIntInArrayAboveThisValue(wxArrayInt arrInt, int aboveThisValue);
	wxString		RemoveDuplicateMarkersFromMkrString(wxString markerStr);

	// **** functions involved in removing the need for having placement dialogs, DOCUMENT_VERSION 10 ********

	wxString        MakeWordAndExtras(wxChar* ptr, int itemLen); // BEW added 3May23
					// BEW added 4May23 -- GetPostwordExtras removes m_key, returns the rest
	wxString		GetPostwordExtras(CSourcePhrase* pSrcPhrase, wxString fromThisStr);

					// BEW 7Jun23 created next, for parsing final puncts, which may be all or some detached by preceding
					// whitespace(s), and getting to the puncts may require parsing over one or more endEndMarkers
	wxChar*			ParsePostWordPunctsAndEndMkrs(wxChar* pChar, wxChar* pEnd, CSourcePhrase* pSrcPhrase, int& itemLen, wxString spacelessPuncts);
	bool			IsGenuineFollPunct(wxChar chPunct); // BEW 7Jun23 created, for use in ParsePostWordPuncts() in ParseWord()
	void			CountGoodAndBadEndPuncts(wxString strEndPuncts, int& nGood, int& nBad); // BEW 7Jun23 created, for use in ParsePostWordPuncts()
					// BEW added ParseDate 16Jun23 for parsing data like 02/26/01 or 02/26/2001, or 2010/05/24 , or 12/02 etc.
	int				ParseDate(wxChar* pChar, wxChar* pEnd, wxString spacelessPuncts); 
	int				ScanToWhiteSpace(wxChar* pChar, wxChar* pEnd); // BEW 16Jun23 added, for use in ParseDate

	wxString		RemoveEndMkrsFromExtras(wxString extras); // BEW added 4May23
	bool			Qm_srcPhrasePunctsPresentAndNoResidue(CSourcePhrase* pSrcPhrase, wxString extras, 
					int& extrasLen, wxString& residue, bool& bEndPunctsModified); // BEW added 4May23 'Q' in the name means "Query"
	bool			UpdateSingleSrcPattern(CSourcePhrase* pSrcPhrase, wxString& Sstr, bool bTokenizingTargetText = FALSE); // BEW added 10May23
							// BEW 7Sep23 added 2nd param above, ref to Sstr
							// for updating m_srcSinglePattern when puncts have changes
	bool			CreateOldSrcBitsArr(CSourcePhrase* pSrcPhrase, wxArrayString& oldSrcBitsArr, wxString& spacelessPuncts);
	bool			m_bTstrFromMergerCalled; // BEW 19May23 added. So FromSingleMakeTstr() will know where to store its result
	bool			IsRedOrBindingOrNonbindingBeginMkr(wxChar* pChar, wxChar* pEnd);
	wxString		GetAccumulatedKeys(SPList* pList, int indexFirst, int indexLast); // BEW added 11Jul23 for use inParsePostWordPunctsAndEndMkrs()

	// **** end of functions for not having placement dialogs *******

	bool			IsInitialPunctPlusWhite(wxChar* pChar, wxString& spacelessPuncts, wxChar* pEnd, wxString& strReturn); // BEW 22May23 added
	wxString		GetFilteredItemBracketed(const wxChar* ptr, int itemLen);
	void			GetMarkerInventoryFromCurrentDoc(); // whm 17Nov05
	void			GetMarkerInventoryFromCurrentDoc_For_Collab(); // bew 8Oct11, simplified for collaboration needs
	CLayout* GetLayout(); // view class also has its own member function of the same name
	//bool			GetNewFile(wxString*& pstrBuffer, wxUint32& nLength, wxString titleID, wxString filter,
	//				wxString* fileTitle);
	CPile* GetPile(const int nSequNum);
	wxString		GetUnFilteredMarkers(wxString& src);
	wxString		GetWholeMarker(wxChar* pChar);
	wxString		GetWholeMarker(wxString str);
	wxString		GetMarkerWithoutBackslash(wxChar* pChar);
	wxString		GetBareMarkerForLookup(wxChar* pChar);
	wxString		GetBareMarkerForLookup(wxString wholeMkr); // whm 30Nov2023 added
	void			GetMarkersAndEndMarkersFromString(wxArrayString* pMkrList, wxString str, wxString endmarkers);
	void			GetMarkersAndFollowingWhiteSpaceFromString(wxArrayString& pMkrList, wxString str);
	void			GetUnknownMarkersFromDoc(enum SfmSet useSfmSet,	wxArrayString* pUnkMarkers, wxArrayInt* pUnkMkrsFlags,
							wxString& unkMkrsStr, enum SetInitialFilterStatus mkrInitStatus);
	wxString		GetUnknownMarkerStrFromArrays(wxArrayString* pUnkMarkers, wxArrayInt* pUnkMkrsFlags);
	// BEW 30Sep19 next one has added bool, default FALSE; function is called only twice in the app
	bool			HasMatchingEndMarker(wxString mkr, CSourcePhrase* pSrcPhrase, bool bSearchInNonbindingEndMkrs = FALSE);
	bool			IsEnd(wxChar* pChar);
	bool			IsWhiteSpace(wxChar* pChar);
	int				ParseNumber(wxChar* pChar);
	wxString		ParseNumberInStr(wxString strStartingWithNumber); // BEW 1Aug23, to get a number string without having to use wxChar*
	wxString		m_firstVerseNum; // BEW added 1Aug23, to enable knowing when a contentless pSrcPhrase is
								 // to be finished off (i.e. appended to pList*) - when a later pSrcPhrase
								 // has a verseNum greater than this one. Use ParseNumberInStr(wxString strAtNumberStart) to help decide
	int				IndexOf(CSourcePhrase* pSrcPhrase); // BEW added 17Mar09
	bool			IsVerseMarker(wxChar* pChar, int& nCount);
	bool			IsFootnoteInternalEndMarker(wxChar* pChar);
	bool			IsCrossReferenceInternalEndMarker(wxChar* pChar);
	bool			IsFootnoteOrCrossReferenceEndMarker(wxChar* pChar);
	//bool			IsFootnoteOrCrossReferenceEndMarker(wxString str); //overload, str must start with endmarker
	bool			IsFilteredBracketMarker(wxChar* pChar, wxChar* pEnd);
	bool			IsFilteredBracketEndMarker(wxChar* pChar, wxChar* pEnd);
	bool			IsChapterMarker(wxChar* pChar);
	bool			IsAmbiguousQuote(wxChar* pChar);
	bool			IsOpeningQuote(wxChar* pChar);
	bool			IsStraightQuote(wxChar* pChar);
	bool			IsClosingQuote(wxChar* pChar);
	bool			IsClosingCurlyQuote(wxChar* pChar);
	bool			IsClosingDoubleChevron(wxChar* pChar); // BEW 6Oct16 added, but no IsOpeningDoubleChevron() done yet
	bool			CannotBeClosingQuote(wxChar* pChar, wxChar* pPunctStart); // BEW added 19Oct15, for Seth's bug
	bool			IsAFilteringSFM(USFMAnalysis* pUsfmAnalysis); // whm 24Oct2023 see definition comments for warnings about use of this function!!
	bool			IsAFilteringUnknownSFM(wxString unkMkr);
	//bool			IsMarker(wxChar* pChar, wxChar* pBuffStart);
	bool			IsMarker(wxChar* pChar);
	bool			IsMarker(wxString& mkr); // overloaded version
	bool			IsWmkrWithBar(wxChar* ptr); //BEW added 12Sep22 in support 
						// of the dual identities of \w .. \w* markers - for Tokenising properly
	wxString		GetLastBeginMkr(wxString mkrs); // BEW 13Dec22 get the last one in m_markers, but the last one
						// does not always result in getting the correct TextType; but some places need this so keep it.
						// However, GetPriorityBeginMkr() -- see next line, should do better job; get \v if present
	wxString		GetPriorityBeginMkr(wxString mkrs); // BEW 2Aug23 get the priority beginMkr from m_markers, to set the TextType - \v if present
	void			GetLengthToWhitespace(wxChar* pChar, unsigned int& counter, wxChar* pEnd); // BEW 20Oct22
	bool			IsClosedParenthesisAhead(wxChar* pChar, unsigned int& count, wxChar* pEnd, CSourcePhrase* pSrcPhrase, bool& bTokenizingTargetText);
	bool			IsClosedBraceAhead(wxChar* pChar, unsigned int& count, wxChar* pEnd, CSourcePhrase* pSrcPhrase, bool& bTokenizingTargetText);
	bool			IsClosedBracketAhead(wxChar* pChar, unsigned int& count, wxChar* pEnd, CSourcePhrase* pSrcPhrase, bool& bTokenizingTargetText);
	wxString		ParseChVerseUnchanged(wxChar* pChar, wxString spacelessPuncts, wxChar* pEnd); // BEW 25Oct22 pChar should 
							// be a digit, parse over things like 4:17, or 5:4-9. but do not include the 
							// final . of 5:4-9.  (Use primarily in footnotes in the input text)
	wxString		ParseAWord(wxChar* pChar, wxString& spacelessPuncts, wxChar* pEnd, bool& bWordNotParsed); // BEW 3Aug23 added bWordNotParsed
	//CSourcePhrase*  GetPreviousSrcPhrase(CSourcePhrase* pSrcPhrase); // BEW added 13Dec22, and commented out 13Dec22 - it isn't needed yet, but is robust
	bool			CanParseForward(wxChar* pChar, wxString spacelessPunctuation, wxChar* pEnd); // BEW 26Jul23 refactored, because
						// internally the algorithm gave false positives, especially if ' (straight quote) was not in the punctuation set.
						// Legacy comment: BEW 12Dec22 added in order to handle 
						// word-internal punctuation (e.g. ' used for glottal stop) because the old parser used to parse in from both ends, 
						// but ParseAWord now only parses forwards, and so without this compensating function being in the while loop's set
						// of tests, an internal punctuation character will cause a misparse that could lead to serious error (or worse)
	//bool			SkipParseAWord(wxChar* pChar, wxChar* pEnd); // BEW 26Jul23, if \h in source text which content-less yet, except for markers,
							// has \h followed by space and then some successive periods (seen in data so far .. or ... and is rare), then
							// allowing ParseAWord() to parse the periods will, because they are puncts, not advance ptr leading to an assert.
							// This Skip... function is a hack to get ptr advance safely past the ParseAWord() call. Use after LookupSFM()
	//int				m_nHowManyPeriods; // Set when SkipParseAWord() has determined how many successive periods there are at the current 
									   // pSrcPhrase; default is zero. Reset zero at every new pSrcPhrase.
	//wxString		m_strSkipMkr; // BEW 27Jul23 added, for setting within SkipParseAWord(), stores the beginMkr ParseWord() will want to
								  // know what it is to store in m_markers, when SkipParseAWord returns TRUE
	//bool			m_bSkipRequired; // BEW 27Jul23, stores the value returned from SkipParseAWord(), ParseWord will use it
	bool			IsClosingBracketNext(wxChar* pChar);
	//bool			IsOpenParenBraceBracketWordInternal(wxChar* pChar, wxChar* pEnd, wxString punctsSet); BEW 19Nov22 I don't think I need this - deprecate
	bool			m_bClosingBracketIsNext;
	bool			IsOpenBracketAhead(wxChar* pChar);
	bool			IsDetachedWJtype_endMkr_Ahead(wxChar* pChar, wxChar* pEnd, int& countOfSpaces);

	int				CountWhitesSpan(wxChar* pChar, wxChar* pEnd);
	bool			m_bWidowedParenth; // BEW 29Dec22 when there's an isolate '(' after space in the source text end of line
	bool			m_bWidowedBracket; // BEw 31Dec22, for an isolate '[' after \p marker
	bool			IsOpenParenthesisAhead(wxChar* pChar, wxChar* pEnd); // BEW 5Nov20 added for ParseWord(): ).<space>(<space>nxtwrd 
	bool			IsOpenParenthesisAhead2(wxChar* pChar, wxChar* pEnd); // BEW 5Nov20 added for ParseWord(): .<space>(<space>nxtwrd 
	bool			IsPrevCharANewline(wxChar* ptr, wxChar* pBuffStart);
	bool			IsPunctuation(wxChar* ptr, bool bSource = TRUE);
	bool			IsPostwordFilteringRequired(wxChar* pChar, bool& bXref_Fn_orEn,
	bool&           bIsFilterStuff, wxString& wholeMkr); // BEW added 2Mar17
	bool			IsEndMarker(wxChar* pChar, wxChar* pEnd);
	bool			IsEndMarker2(wxChar* pChar); // BEW 7Nov16 This version of IsEndEndMarker() has the end-of-buffer test internal
	bool			IsTextTypeChangingEndMarker(CSourcePhrase* pSrcPhrase, wxString& typeChangingEndMkr); // BEW 3Jul23 added 2nd arg
	wxString		FindWordBreakChar(wxChar* ptr, wxChar* pBufStart); // BEW 13Jul23 changed return from wxChar, to wxString
	bool			IsInLineMarker(wxChar* pChar, wxChar* WXUNUSED(pEnd));
	bool			IsCorresEndMarker(wxString wholeMkr, wxChar* pChar, wxChar* pEnd); // whm added 10Feb05
	// Next four are tests made for what precedes pChar when parsing what follows the
	// word in ParseWord2() and a begin-marker is encountered with no preceding whitespace
	// or with whitespace (which may not be a latin space) and post word filtering is wanted
	// (it may be stuff like \f ...\f*, or \x ....\x* etc, which are filterable - so we 
	// need to give ParseWord2() the capability to do filtering when parsing input text)
	bool			EndmarkerPrecedes(wxChar* pChar, wxString& precedingEndmarker);
	bool			PunctuationPrecedes(wxChar* pChar, wxString& precedingPunct, bool bTokenizingTargetText);
	bool			WordPrecedes(wxChar* pChar, wxString& theWord, CSourcePhrase*  pSrcPhrase, wxString& spacelessPuncts);
	bool			SpacePrecedes(wxChar* pChar, wxString& precedingSpace);
	int				ScanToNextMarker(wxChar* pChar, wxChar* pEnd);

	// more unfiltering stuff goes just above, if needed

	bool			IsLegacyDocVersionForFileSaveAs();
	static SPList* LoadSourcePhraseListFromFile(wxString FilePath);
	USFMAnalysis* LookupSFM(wxChar* pChar);
	// Overloaded variant used in ParseWord()
	USFMAnalysis* LookupSFM(wxChar* pChar, wxString& tagOnly, wxString& baseOfEndMkr, bool& bIsNestedMkr);
	// Overloaded variant used when passing in the marker minus its initial backslash
	USFMAnalysis* LookupSFM(wxString bareMkr);
	// BEW created 13Jan11, pass in the CSourcePhrase instance's m_targetStr value (it may
	// have punctuation, and possibly also be a fixed-space (~) conjoined pair; internally
	// parse it & extract and return the equivalent puncuation-less string
	wxString		MakeAdaptionAfterPunctuationChange(wxString& targetStrWithPunctuation, int startingSequNum);
	bool			MarkerExistsInArrayString(wxArrayString* pUnkMarkers, wxString unkMkr, int& MkrIndex);
	bool			MarkerExistsInString(wxString MarkerStr, wxString wholeMkr, int& markerPos);
	wxString		MarkerAtBufPtr(wxChar* pChar, wxChar* pEnd);
	wxString		NormalizeToSpaces(wxString str);
	bool			OpenDocumentInAnotherProject(wxString lpszPathName);
	void			TransferFixedSpaceInfo(CSourcePhrase* pDestSrcPhrase, CSourcePhrase* pFromSrcPhrase);
	int				ParseAdditionalFinalPuncts(wxChar*& ptr, wxChar* pEnd, CSourcePhrase*& pSrcPhrase,
							wxString& spacelessPuncts, int len, bool& bExitOnReturn,
							bool& bHasPrecedingStraightQuote, wxString& additions,	bool bPutInOuterStorage);

	int				ParseFinalPuncts(wxChar* pChar, wxChar* pEnd, wxString spacelessPuncts); // BEW 7Nov22 added
	int				ParsePuncts(wxChar* pChar, wxChar* pEnd, wxString spacelessPuncts); // BEW 25Jul23 added

	// *********  NOTE ***** BEW 3Jun23 if I get a message, errorC2248: cannot access private member declared in class
	// *********  regarding operator= , when using ParseFinalPuncts() , it's because I was assuming that the function
	// *********  ParseFinaPuncts() returns a wxString, when it actually returns an int!!!!! Duh! Homer brain struck again
	// *********  I've done this a sufficient number of times, and found the explanations opaque, I need a note somewhere.
						// getting an error message that ParseFinalPuncts() is a private member of wxString
	int				ParseInlineEndMarkers(wxChar*& ptr, wxChar* pEnd, CSourcePhrase*& pSrcPhrase,
						wxString& inlineNonBindingEndMkrs, int len,
						bool& bInlineBindingEndMkrFound, bool& bInlineNonbindingEndMkrFound,
						bool& bInlineNormalEndMkrFound, wxString& endMkr);
	int				ParseOverAndIgnoreWhiteSpace(wxChar*& ptr, wxChar* pEnd, int len);
	int				ParseMarker(wxChar* pChar);
	int				ParseWhiteSpace(wxChar* pChar);
	int				ParseFilteringSFM(const wxString wholeMkr, wxChar* pChar,
	wxChar*			pBuffStart, wxChar* pEnd);
	wxChar*			FindParseHaltLocation(wxChar* ptr, wxChar* pEnd, bool* pbFoundInlineBindingEndMarker,
						bool* pbFoundFixedSpaceMarker, bool* pbFoundClosingBracket,
						bool* pbFoundHaltingWhitespace, int& nFixedSpaceOffset,
						int& nEndMarkerCount, bool bTokenizingTargetText); // BEW created 25Jan11, 24Oct14
						// and added the bTokenizingTargetText boolean
	void			ParseSpanBackwards(wxString& span, wxString& wordProper, wxString& firstFollPuncts,
						int nEndMkrsCount, wxString& inlineBindingEndMarkers,
						wxString& secondFollPuncts, wxString& ignoredWhiteSpaces,
						wxString& wordBuildersForPostWordLoc, wxString& spacelessPuncts); //BEW created 27Jan11
	wxString		SquirrelAwayMovedFormerPuncts(wxChar* ptr, wxChar* pEnd, wxString& spacelessPuncts); // BEW
								// created 31Jan11, a helper for round tripping punctuation changes
	bool			m_bIsInFigSpan;
	int				nFirstSequNumOfSpan; // used for \fig .... \fig* span

	wxString		m_currentUnfilterMkr; // used when unfiltering filtered content having attributes metadata
	bool			m_bCurrentlyFiltering; // used when filtering content that may contain attributes metadata
	bool			IsBeginMarker(wxChar* pChar, wxChar* pEnd, wxString& wholeMarker, bool& bIsEndMkr);

	// BEW 30Sep19 created this (valid for ParseWord() or ParseWord2()) to pull out
	// pre-word-proper processing into TokenizeText() - because things like \fig were
	// being stored wrongly in m_markers instead of m_inlineNonbindingMarkers.
	// The int returned is a count of the characters parsed over, so that the iterator
	// ptr can be updated correctly from an int len counter internally, which uses int
	// itemLen for parsing over things like puncts, beginMarkers, etc.
	int ParsePreWord(wxChar* pChar,
		wxChar* pEnd,
		CSourcePhrase* pSrcPhrase,
		wxString& spacelessPuncts, // caller determines whether it's src set or tgt set
		wxString& inlineNonbindingMrks, // fast access string for \wj \qt \sls \tl \fig
		wxString& inlineNonbindingEndMrks, // for their endmarkers \wj* etc
		bool& bIsInlineNonbindingMkr,
		bool& bIsInlineBindingMkr,
		bool bTokenizingTargetText);

	// BEW 11Oct10, changed contents of ParseWord() majorly, so need new signature
	//int ParseWord(wxChar *pChar, wxString& precedePunct, wxString& followPunct,wxString& SpacelessSrcPunct);
	int	ParseWord(wxChar* pChar, // pointer to next wxChar to be parsed
		const wxChar* pBufStart, // whm 28Sep2023 added to determine initial buffer start
		wxChar* pEnd, // pointer to the null at the end of the string buffer
		CSourcePhrase* pSrcPhrase, // where we store what we parse
		wxString& spacelessPuncts, // punctuationset used, with spaces removed
		wxString& inlineNonbindingMrks, // fast access string for \wj \qt \sls \tl \fig
		wxString& inlineNonbindingEndMrks, // fast access string for \wj* \qt* \sls* \tl* \fig*
		bool& bIsInlineNonbindingMkr, // TRUE if pChar is pointing at a beginmarker from
		// the set of five non-binding ones, i.e. \wj \qt \tl \sls or \fig
		bool& bIsInlineBindingMkr, // TRUE if pChar is pointing at a beginmarker
		bool bTokenizingTargetText);

	wxString		RedoNavigationText(CSourcePhrase* pSrcPhrase);
	bool			RemoveMarkerFromBoth(wxString& mkr, wxString& str1, wxString& str2);
	wxString		RemoveAnyFilterBracketsFromString(wxString str);
	//wxString		RemoveMultipleSpaces(wxString& rString); // whm 11Aug11 moved to helpers
	void			ResetUSFMFilterStructs(enum SfmSet useSfmSet, wxString filterMkrs, wxString unfilterMkrs);
	void			ResetUSFMFilterStructs(enum SfmSet useSfmSet, wxString filterMkrs, enum resetMarkers resetMkrs);
	void			RestoreCurrentDocVersion(); // BEW added 19Apr10 for Save As... support
	void			RestoreDocParamsOnInput(wxString buffer); // BEW added 08Aug05 to facilitate XML doc input
															  // as well as binary (legacy) doc input & its
															  // output equivalent function is SetupBufferForOutput
	int				RetokenizeText(bool bChangedPunctuation,
		bool bChangedFiltering, bool bChangedSfmSet);
	bool			SetCaseParameters(wxString& strText, bool bIsSrcText = TRUE);
	void			SetDocumentWindowTitle(wxString title, wxString& nameMinusExtension);
	void			SetDocVersion(int index = 0); // BEW added 19Apr10 for Save As... support
							// (using the default value of 0 causes the version number to be
							// set to whatever version number is currently #defined in
							// AdaptItConstants.h 
	wxString		SetupBufferForOutput(wxString* pCString);
	// BEW 11Oct10 (actually 11Jan11) added bTokenizingTargetText param so as to be able
	// to parse target text, with target punctuation settings, as if it was source text -
	// this is useful for handling user-changes from Preferences to the punctuation settings
	int				TokenizeText(int nStartingSequNum, SPList* pList, wxString& rBuffer,
		int nTextLength, bool bTokenizingTargetText = FALSE);
	void			UpdateFilenamesAndPaths(bool bKBFilename, bool bKBPath, bool bKBBackupPath,
		bool bGlossingKBPath, bool bGlossingKBBackupPath);
	void			UpdateSequNumbers(int nFirstSequNum, SPList* pOtherList = NULL); // BEW changed 16Jul09
	void			SetFilename(const wxString& filename, bool notifyViews);


public:
	// Destructor from the MFC version
	virtual ~CAdapt_ItDoc();

public:
	bool            CollaborationEditorAcceptsDataTransfers();        // Checks if safe to Save with regard to collaboration.
	bool            DocumentIsProtectedFromTransferringDataToEditor(); // whm added 11May2017

protected:
	void			ValidateNoteStorage(); // ensure no m_bHasNote flags are TRUE but lack
						// any note being stored there, and that every stored note has the
						// m_bHasNote flag set TRUE -- run at start of OnOpenDocument()
	bool			ForceAnEmptyUSFMBreakHere(wxString tokBuffer, CSourcePhrase* pSrcPhrase,
		wxChar* ptr); // BEW added 15Aug12

//void            Enable_DVCS_item (wxUpdateUIEvent& event);
	bool            Commit_valid();
	bool            Git_installed();

	bool			ConsistencyCheck_ClobberDoc(CAdapt_ItApp* pApp, bool& bDocIsClosed, bool& bDocForcedToClose,
		CStatusBar* pStatusBar, AFList* afListPtr, AFGList* afgListPtr); // return TRUE if
		// there was no error, otherwise return FALSE - caller should then
		// return as well
#ifdef CONSCHK
	void ListBothArrays(wxArrayString& arrSetNotInKB, wxArrayString& arrRemoveNotInKB);
#endif

public:

	// whm 23Aug2021 added support for AutoCorrect feature
	void SetupAutoCorrectHashMap();
	bool LookUpStringInAutoCorrectMap(wxString candidateEditBoxLHSStr, wxChar typedChar, wxString& newEditBoxLHSStr);

	// Generated message map functions (from MFC version) ... most of these should not be
	// public!!! (BEW 17May10)
public:

	void OnFileSave(wxCommandEvent& WXUNUSED(event));
	void OnUpdateFileSave(wxUpdateUIEvent& event);
	void DocChangedExternally();

	// functions related in some way to DVCS:
	void OnTakeOwnership(wxCommandEvent& WXUNUSED(event));
	void OnUpdateTakeOwnership(wxUpdateUIEvent& event);
	void OnSaveAndCommit(wxCommandEvent& WXUNUSED(event));
	void OnUpdateSaveAndCommit(wxUpdateUIEvent& event);
	int  DoSaveAndCommit(wxString blurb);
	void EndTrial(bool restoreBackup);
	void DoChangeVersion(int revNum);
	void DoShowPreviousVersions(bool fromLogDialog, int startHere);
	void OnShowPreviousVersions(wxCommandEvent& WXUNUSED(event));
	void DoAcceptVersion(void);
	bool FoundEsbeEndMkr(wxChar* pChar, int& whitespaceLen); // BEW 30Sep19 added, for USFM3 support
	int  StoreEsbeEndMarker(wxChar* pChar, CSourcePhrase* pSrcPhrase, int whitespaceLen); // BEW 30Sep19, ditto
	bool IsLatestVersionChanged(void);
	bool RecoverLatestVersion(void);
	void OnShowFileLog(wxCommandEvent& WXUNUSED(event));
	void OnShowProjectLog(wxCommandEvent& WXUNUSED(event));
	void OnDVCS_Version(wxCommandEvent& WXUNUSED(event));
	void OnUpdateDVCS_item(wxUpdateUIEvent& event);

	void OnFileSaveAs(wxCommandEvent& WXUNUSED(event));
	void OnUpdateFileSaveAs(wxUpdateUIEvent& event);
	void OnFileOpen(wxCommandEvent& WXUNUSED(event));
	void OnFileClose(wxCommandEvent& event);
	void OnUpdateFileClose(wxUpdateUIEvent& event);
	void OnUpdateAdvancedReceiveSynchronizedScrollingMessages(wxUpdateUIEvent& event);
	void OnAdvancedReceiveSynchronizedScrollingMessages(wxCommandEvent& event);
	void OnAdvancedSendSynchronizedScrollingMessages(wxCommandEvent& event);
	void OnUpdateAdvancedSendSynchronizedScrollingMessages(wxUpdateUIEvent& event);
	void OnEditConsistencyCheck(wxCommandEvent& WXUNUSED(event));
	void OnUpdateEditConsistencyCheck(wxUpdateUIEvent& event);
	void DoBookName(); // to access the Book Naming dialog from anywhere, calls OnBookNameDlg()
	//void UpdateDocCreationLog(CSourcePhrase* pSrcPhrase, wxString& chapter, wxString& verse);
private:
	void OnSplitDocument(wxCommandEvent& WXUNUSED(event));
	void OnUpdateSplitDocument(wxUpdateUIEvent& event);
	void OnJoinDocuments(wxCommandEvent& WXUNUSED(event));
	void OnUpdateJoinDocuments(wxUpdateUIEvent& event);
	void OnMoveDocument(wxCommandEvent& WXUNUSED(event));
	void OnUpdateMoveDocument(wxUpdateUIEvent& event);
	void OnFilePackDoc(wxCommandEvent& WXUNUSED(event));
	void OnFileUnpackDoc(wxCommandEvent& WXUNUSED(event));
	void OnUpdateFilePackDoc(wxUpdateUIEvent& event);
	void OnUpdateFileUnpackDoc(wxUpdateUIEvent& event);
	void OnChangePunctsOrMarkersPlacement(wxCommandEvent& WXUNUSED(event));
	void OnUpdateChangePunctsOrMarkersPlacement(wxUpdateUIEvent& event);

private:
	int		m_docVersionCurrent; // BEW added 19Apr10 for Save As... support
	bool	m_bLegacyDocVersionForSaveAs;
	bool	m_bDocRenameRequestedForSaveAs;
	wxChar* HandlePostBracketPunctuation(wxChar* ptr, CSourcePhrase* pSrcPhrase, bool bParsingSrcText);
	bool	IsMarkerFreeTransOrNoteOrBackTrans(const wxString& mkr, bool& bIsForeignBackTransMkr);
	bool	IsEndMarkerRequiringStorageBeforeReturning(wxChar* ptr, wxString* pWholeMkr);
	void	SetFreeTransOrNoteOrBackTrans(const wxString& mkr, wxChar* ptr,
		size_t itemLen, CSourcePhrase* pSrcPhrase);
	bool	DoConsistencyCheck(CAdapt_ItApp* pApp, CKB* pKB, CKB* pKBCopy, AFList& afList,
		int& nCumulativeTotal, wxArrayString& arrSetNotInKB, wxArrayString& arrRemoveNotInKB);
	// the "G" variant below, is for use when glossing mode is on, pKB and pKBCopy will be
	// glossing KBs, and internally, the data accessed will be m_gloss, not m_adaption, and
	// in glossing mode <Not In KB> entries are not supported
	bool	DoConsistencyCheckG(CAdapt_ItApp* pApp, CKB* pKB, CKB* pKBCopy, AFGList& afList,
		int& nCumulativeTotal);
	CCell* LayoutDocForConsistencyCheck(CAdapt_ItApp* pApp, CSourcePhrase* pSrcPhrase,
		SPList* pPhrases);
	bool	MatchAutoFixItem(AFList* pList, CSourcePhrase* pSrcPhrase, AutoFixRecord* rpRec);
	// this next one is a variant used for matching AutoFixRecordG for glosses
	bool	MatchAutoFixGItem(AFGList* pList, CSourcePhrase* pSrcPhrase, AutoFixRecordG* rpRec);
	bool	IsUpperCaseCharInFirstWord(wxString& str, int& offset, wxChar& theChar, bool bIsSrcText = TRUE); // BEW 24May16
	wxChar	GetFirstChar(wxString& strText);
	bool	IsInCaseCharSet(wxChar chTest, wxString& theCharSet, int& index);
	wxChar	GetOtherCaseChar(wxString& charSet, int nOffset);
	void	RemoveAutoFixList(AFList& afList); // for adaptating data
	void	RemoveAutoFixGList(AFGList& afgList); // for glossing data
	bool	m_bHasPrecedingStraightQuote; // default FALSE, set TRUE when a straight quote
	bool	m_bReopeningAfterClosing;	  // default FALSE - set true when we're going to reopen the doc
public:
	// Next line, BEW 3Sep19 in support of USFM3, and hiding attributes metadata
	CSourcePhrase* m_pSrcPhraseBeingCreated; // set this to the instance that TokenizeText 
										 // is currently populating
	// Prototypes for helpers in support of USFM3 markup, regarding metadata
	// which follows bar ( | ) immediately prior to certain endmarkers
	bool m_bAllowCommaInKB; // BEW add 22Apr23, default FALSE, but set TRUE if comma is in src or tgt word
	bool m_bWithinMkrAttributeSpan;
	bool m_bHiddenMetadataDone;
	bool bSaveWithinAttributesSpanValue; // If editing source text, m_bWithinMkrAttributeSpan
		// has to be FALSE until the source text edit is done with; so here cache the 
		// boolean's value across the handler for editing source text
	//bool m_bInRedSet; // TRUE if it's a marker like \rq which is of the type like \f and others   // BEW 24Aug23 removed, it's not used anywhere
	int nSequNum_ToForceVerseEtc; // to help in getting text type colouring working right in TokenizeText

	// Prototypes for determining the span of CSourcePhrase instances for \f, \ef or \x inline markers
	bool IsForbiddenMarker(wxString mkr, bool bCheckForEndMkr); // BEW 3Apr20, for refactored placeholder insertion
	bool FindProhibitiveBeginMarker(wxString& strMarkers, wxString& beginMkr); // BEW 3Apr20, for refactored placeholder insertion
	//int  ReturnSN(SPList::Node* pos, CSourcePhrase*& pSrcPhrase); // BEW 9Apr20 quick return of pSrcPhase & sequNum from loop iterator
	//bool GetMatchingEndMarker(CSourcePhrase* pInitialSrcPhrase, wxString strEndMkr, 
	//							int& atBeginSequNum, int& atMatchSequNum); // BEW 3Apr20, for 
								// refactored placeholder insertion  <<-- deprecated, unfinished,  8Apr20
	bool ForceSpanEnd(wxString& endMkr, CSourcePhrase* pSrcPhrase, bool& bStoppingRunOn); // BEW 7Apr20
	bool IsWithinSpanProhibitingPlaceholderInsertion(CSourcePhrase* pSrcPhrase); // BEW 8Apr20
	bool FindBeginningOfSpanProhibitingPlaceholderInsertion(CSourcePhrase* pSrcPhrase, wxString& beginMkr, int& nBeginSN); // BEW 8Apr20
	bool FindEndOfSpanProhibitingPlaceholderInsertion(CSourcePhrase* pSpanStart_SrcPhrase, wxString matchEndMkr, int& nEndSN);

	// BEW 11Aug23, return TRUEa if pChar points at text string or digits string (but not the digits of a versenumber
	bool IsTextAtPChar(wxChar* pChar, wxChar* pEnd, wxString spacelessPuncts, wxChar*& pNewPtr);
	
	// whm 17Jan2024 removed the EnterEmptyMkrsLoop() function. The use of IsEmptyMkr() function is 
	// sufficient test along with bIsToBeFiltered flag as to whether to enter the long block of code
	// in TokenizeText() that deal with empty content markers.
	//bool EnterEmptyMkrsLoop(wxChar* pChar, wxChar* pEnd); // BEW 12Aug23
	//bool ExitEmptyMkrsLoop(wxChar* pChar, wxChar* pEnd, wxString spacelessPuncts); // BEW 12Aug23

	// BEW 14Aug23, parses over whites at pChar and 
	// returns TRUE if backspace follows, and the length of span up to but not including the
	// the backslash; else FALSE and return -1 for nWhitesLen - a backslash must be at least
	// one white further on than pChar.
	// 
	// whm 15Jan2024 modified the IsEmptyMkr() function below so that it has three reference 
	// parameters in its signature: 
	//    bool& bHasBogusPeriods - there are bogus periods to process for the m_paragraphMkrs
	//		marker and/or chapter marker that is excluded from being treated as an empty marker.
	//    int& nWhitesLenIncludingBogusPeriods - the number of whites + bogus periods following
	//		the marker. 
	//    int& nPeriodsInWhitesLen - the number of periods within the whites followoing the 
	//		marker.
	// See the definition of IsEmptyMkr() for explanation.
	bool bHasBogusPeriods;
	int nWhitesLenIncludingBogusPeriods;
	int nPeriodsInWhitesLen;
	bool IsEmptyMkr(wxChar* pChar, wxChar* pEnd, 
		bool& bHasBogusPeriods, int& nWhitesLenIncludingBogusPeriods, int& nPeriodsInWhitesLen);
	void IteratePtrPastBogusPeriods(wxChar*& ptr, wxChar* pEnd); 
	bool m_bWithinEmptyMkrsLoop; // set TRUE on entry, FALSE on exit; init to FALSE at top of TokText()



protected:

	wxChar* m_ptr;
	wxChar* m_auxPtr; // an auxiliary ptr for use in scanning when changing 
					  // m_ptr is not wanted
	size_t   m_nBeginMkrLen; // includes the trailing whitespace character (typically latin space)
	size_t   m_nEndMarkerLen; // includes the trailing asterisk
	size_t   m_nSpanLength; // the length of the span of characters to be skipped
		// over in the input text stream, so that AI no longer "sees" the metadata while
		// running its tokenizing function
	wxString m_cachedAttributeData;  // store everything from the first bar (strBar) to the
		// end of the matching endmarker here. The CSourcePhrase which gets to
		// store the hidden stuff cached here may not be the current one - it 
		// could be further along, and not yet created because the parse has not 
		// gotten that far yet
	wxString m_cachedWordBeforeBar; // get the word preceding the bar (including any puncts)
		// back to the preceding whitespace, cache it here, to enable identifying which
		// CSourcePhrase instance is the one where we need to skip the metadata when parsing
		// but retain the count of metadata characters so that itemLen stays correct

	CSourcePhrase* m_pCachedSourcePhrase; // Submit m_pCachedWordBeforeBar to TokenizeTextSring()
	int m_nCountOfWordsToBar; // store here the number of words between the beginMkr and the bar character

		// (in CAdapt_ItView.cpp) and store the result in m_pCachedWordBeforeBar. This gives 
		// a filled out CSourcePhrase instance (on heap) quickly without writing any new code. 
		// Since this internally calls TokenizeText() at a time when the latter is processing
		// the m_pSourcePhrases document list, with the boolean m_bWithinMkrAttributeSpan
		// set to TRUE, be sure to save and restore that boolean's value across the call
		// of TokenizeTextString() because we want to call the latter with that boolean
		// set to FALSE to prevent re-entrancy to the USFM3 supporting code.
	size_t m_offsetToFirstBar; // the offset to the first bar ( | ) character, starting from
		// the begin-marker
		// If there is no bar before the endmarker is reached, then this will be -1
	size_t m_offsetToMatchingEndMkr; // the offset to whatever endmarker matches the begin-mkr,
		// as that endmarker marks the end of the attribute-having span (offset to endMkr's start)
		// The cachedAttributeData will, when all the calcs are done, contain the intervening 
		// metadata, and include the endmarker too, and add that stuff to the last word of the span.
		// So we want: lastword-of-sacredTgtText + bar + post-bar-attributes-data + matchingEndMarker

	AttrMkrType enumMkrTypeValue; // when parsing, and an attribute-having 
								  // marker type is current, store the type here
	wxString m_strAttrBeginMkr;	  // if a begin-marker is found which is one of the 
								  // set taking attributes, store here
	wxString m_strAttrEndMkr;	  // if an end-marker is found which is one of the 
								  // set taking attributes, store here
	wxString m_strMatchedMkr;	  // after parsing a marker, store it here temporarily
	int m_nCachePileSequNum;	  // BEW 20Sep22, the m_nSequNumber value for the pile on 
								  // which to cache the attributes data (hiding it from sight)
	void ClearAttributeMkrStorage(); // Before moving to a new location in the
									   // document, call this to clear the above variables
	bool CheckForAttrMarker(wxString& attrMkr, wxString& matchedMkr, AttrMkrType enumMkrType);
	// Looks up CheckForAttrMarker fast access string, or the
	// CheckForAttrEndMarker fast access string. Returns what
	// was matched, and whether a begin marker or end marker type

	// Return the word-proper, assuming there are no pre-word punctuation chars present,
	// and any ending punctuation characters in the string pointed at by pEndPuncts.
	// Assume also, the input word is from source text
	wxString SimpleWordParser(wxString word, wxString* pEndPuncts); 

	// A utility function to get the above m_cachedSourcePhrase instance populated correctly
	// from a parse of the word stored in m_cachedWordBeforeBar (see above), using 
	// TokenizeTextString() from the view class
	//void ParseWordBeforeBar2SourcePhrase(wxString& srcTextInput, CSourcePhrase& srcPhrase); //BEW removed 16Sep22 - commented out

	wxString CacheWordBeforeBar(wxChar* ptrToBar);

	wxString ConvertToEndMarker(wxString strBeginMkrAndSpace); // Change trailing space into *

	bool IsAttributeMarker(wxChar* ptr); // Test for an
		// attribute marker, and if found, return TRUE if it is one - provided it has a 
		// bar (|) within its content. Else return FALSE because no hiding of anything
		// is required. If there is a bar in the internal scan, then its a begin-marker,
		// then store it in strAttrBeginMkr; and create its matched endmarker & store it
		// in strAttrEndMkr; and the returned boolean should be set to TRUE. Return the
		// marker as well - useful for checking

	bool IsAttributeBeginMarker(wxChar* ptr, wxString& beginMkr); // does no more than 
		// check for such a marker in the relevant fast-access string, returning TRUE 
		// if found, and which it is in the signature; 
		// BEW 16Sep22 also only call it once, in TokenizeText() - because it sets number
		// of Doc member variables.
		// So if we called it in ParseWord() where the finishing off and caching of
		// the cache-string is to happen because the active pile has the cache-ending
		// endmarker, that would clobber the Doc member variables to bogus values.
		// We need a separate function, maybe, in ParseWord() - or just use the Doc
		// variables, and CacheWordBeforeBar(ptrToBar) etc to get the caching done there

	bool IsXRefNext(wxChar* ptr, wxChar* pEnd); // does a \x marker occur at ptr?

	// Return TRUE if searching pSrcPhrase's m_endMarkers string for an instance of pEndMarker
	// returns an offset which is not wxNOT_FOUND.The source phrase may be storing more than
	// one endmarker, and if so, the attribute having one should be first
	bool IsThisEndMarkerStoredHere(CSourcePhrase* pSrcPhrase, wxString* pEndMkr);

	// None of wxString's Find() or find() variants are suitable, so I need to create my own
	// Returns the offset to the matchingEndMkr, as int. wx_NOT_FOUND ( -1 ) if there is
	// no bar ( | ) in that span of text
	int FindBarWithinSpan(wxChar* auxPtr, wxString matchingEndMkr, int endMkrLen);
	int FindEndMarkerWithinSpan(wxChar* auxPtr, wxString matchingEndMkr, int endMkrLen,
		CSourcePhrase* pCurSrcPhrase);
	int CountCharsToBar(wxChar* pStart, wxChar* pEnd);
	// end of new prototypes in support of USFM3 markup

private:
	wxChar uselessDegreeChar; // whm 24Dec2019 moved initialization to doc's constructor
	wxString m_strBar;
	wxString m_strSpace;
	wxChar   m_asterisk;
	wxChar   m_barChar;

	bool IsAnUnwantedDegreeSymbolPriorToAMarker(wxChar* ptr);
	wxChar* m_pPreservePreParseWordLocation;

	// a debugging function to check initial m_nSequNumber values in the m_pSoucePhrases list
	// up to a maximum of n (a digit passed in)
	void LogSequNumbers_LimitTo(int nLimit, SPList* pList);

	DECLARE_EVENT_TABLE()
};

#endif /* Adapt_ItDoc_h */
