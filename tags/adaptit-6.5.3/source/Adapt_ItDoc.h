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

/// wxList declaration and partial implementation of the AFList class being
/// a list of pointers to AutoFixRecord objects
WX_DECLARE_LIST(AutoFixRecord, AFList); // see list definition macro in .cpp file
WX_DECLARE_LIST(AutoFixRecordG, AFGList); // see list definition macro in .cpp file

// for debugging, uncomment out to turn on the debugging displays (they are very helpful)
//#define CONSCHK

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
	flag_on_NotInKB_off_hasActiveNotInKB_in_KB // done // use dlg for either normal entry or <Not In KB> 
					// m_bNotInKB TRUE and ensure pTU have valid <Not In KB> entry
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
	restore_meaning_to_doc
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

	// Implementation
protected:
	void			AddParagraphMarkers(wxString& rText, int& rTextLength);
	bool			AnalyseMarker(CSourcePhrase* pSrcPhrase, CSourcePhrase* pLastSrcPhrase,
									wxChar* pChar, int len, USFMAnalysis* pUsfmAnalysis);
	bool			BackupDocument(CAdapt_ItApp* WXUNUSED(pApp), wxString* pRenamedFilename = NULL);
	int				ClearBuffer();
	//void			ConditionallyDeleteSrcPhrase(CSourcePhrase* pSrcPhrase, SPList* pOtherList); BEW deprecated 8Mar11
	CBString		ConstructSettingsInfoAsXML(int nTabLevel); // BEW added 05Aug05 for XML doc output support
	int				ContainsMarkerToBeFiltered(enum SfmSet sfmSet, wxString markers, wxString filterList,
						wxString& wholeMkr, wxString& wholeShortMkr, wxString& endMkr, bool& bHasEndmarker,
						bool& bUnknownMarker, int startAt);
	void			CopyFlags(CSourcePhrase* dest, CSourcePhrase* src);
	void			DeleteListContentsOnly(SPList*& pList);
	bool			DoUnpackDocument(wxFile* pFile);
	bool			IsPreviousTextTypeWanted(wxChar* pChar,USFMAnalysis* pAnalysis);
	void			GetMarkerMapFromString(MapWholeMkrToFilterStatus*& pMkrMap, wxString str); // used in SetupForSFMSetChange
	wxString		GetNextFilteredMarker(wxString& markers, int offset, int& nStart, int& nEnd);
	//bool			IsEndingSrcPhrase(enum SfmSet sfmSet, wxString& markers);
	bool			IsEndingSrcPhrase(enum SfmSet sfmSet, CSourcePhrase* pSrcPhrase);
	bool			IsEndMarkerForTextTypeNone(wxChar* pChar);
	bool			IsFixedSpaceAhead(wxChar*& ptr, wxChar* pEnd, wxChar*& pWdStart, 
							wxChar*& pWdEnd, wxString& punctBefore, wxString& endMkr, 
							wxString& wordBuildersForPostWordLoc, wxString& spacelessPuncts); // BEW created 11Oct10
	void			FinishOffConjoinedWordsParse(wxChar*& ptr, wxChar* pEnd, wxChar*& pWord2Start,
							wxChar*& pWord2End, wxString& punctAfter, wxString& bindingMkr,
							wxString& newPunctFrom2ndPreWordLoc, wxString& newPunctFrom2ndPostWordLoc,
							wxString& wordBuildersFor2ndPreWordLoc, wxString& wordBuildersFor2ndPostWordLoc,
							wxString& spacelessPuncts);
	bool			IsUnstructuredPlainText(wxString& rText);
	void			MakeOutputBackupFilenames(wxString& curOutputFilename);
	bool			NotAtEnd(wxString& rText, const int nTextLength, int nFound);
public:
	void			OverwriteUSFMFixedSpaces(wxString*& pstr);
	void			OverwriteUSFMDiscretionaryLineBreaks(wxString*& pstr);
	void			PutPhraseBoxAtDocEnd();
	bool			ReOpenDocument(	CAdapt_ItApp* pApp,	
									wxString savedWorkFolderPath,			// for setting current working directory
									wxString curOutputPath,					// includes filename
									wxString curOutputFilename,				// to help get window Title remade
        //                          int		 curSequNum,		// for resetting the box location - mrh: now not needed since the seq num is saved in the xml.
									bool	 savedBookmodeFlag,				// for ensuring correct mode
									bool	 savedDisableBookmodeFlag,		// ditto
									BookNamePair*	pSavedCurBookNamePair,  // for restoring the pointed at struct
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
	wxString&		AppendFilteredItem(wxString& dest,wxString& src);
	wxString&		AppendItem(wxString& dest,wxString& src, const wxChar* ptr, int itemLen);

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
	bool			DoCollabFileSave(const wxString& progressTitle,wxString msgDisplayed); // whm added 17Jan12
	void			DoMarkerHousekeeping(SPList* pNewSrcPhrasesList,int WXUNUSED(nNewCount), 
							TextType& propagationType, bool& bTypePropagationRequired);
	bool			DoPackDocument(wxString& exportPathUsed, bool bInvokeFileDialog = TRUE);
	bool			DoTransformedDocFileSave(wxString path);
	void			EraseKB(CKB* pKB);
	bool			FilenameClash(wxString& typedName); // BEW added 22July08 to 
														// prevent 2nd creation destroying work
	CAdapt_ItApp*	GetApp();
	wxString		GetCurrentDirectory();	// BEW added 4Jan07 for saving & restoring the full path
											// to the current doc's directory across the writing of
											// the project configuration file to the project's directory
	int				GetCurrentDocVersion();
	wxString		GetFilteredItemBracketed(const wxChar* ptr, int itemLen);
	void			GetMarkerInventoryFromCurrentDoc(); // whm 17Nov05
	void			GetMarkerInventoryFromCurrentDoc_For_Collab(); // bew 8Oct11, simplified for collaboration needs
	CLayout*		GetLayout(); // view class also has its own member function of the same name
	//bool			GetNewFile(wxString*& pstrBuffer, wxUint32& nLength, wxString titleID, wxString filter,
	//				wxString* fileTitle);
	CPile*			GetPile(const int nSequNum);
	wxString		GetUnFilteredMarkers(wxString& src);
	wxString		GetWholeMarker(wxChar *pChar);
	wxString		GetWholeMarker(wxString str);
	wxString		GetMarkerWithoutBackslash(wxChar *pChar);
	wxString		GetBareMarkerForLookup(wxChar *pChar);
	void			GetMarkersAndTextFromString(wxArrayString* pMkrList, wxString str);
	void			GetUnknownMarkersFromDoc(enum SfmSet useSfmSet,
											wxArrayString* pUnkMarkers,
											wxArrayInt* pUnkMkrsFlags,
											wxString & unkMkrsStr,
											enum SetInitialFilterStatus mkrInitStatus);
	wxString		GetUnknownMarkerStrFromArrays(wxArrayString* pUnkMarkers, wxArrayInt* pUnkMkrsFlags);
	bool			HasMatchingEndMarker(wxString& mkr, CSourcePhrase* pSrcPhrase);
	bool			IsEnd(wxChar* pChar);
	bool			IsWhiteSpace(wxChar *pChar);
	int				ParseNumber(wxChar* pChar);
	int				IndexOf(CSourcePhrase* pSrcPhrase); // BEW added 17Mar09
	bool			IsVerseMarker(wxChar* pChar, int& nCount);
	bool			IsFootnoteInternalEndMarker(wxChar* pChar);
	bool			IsCrossReferenceInternalEndMarker(wxChar* pChar);
	bool			IsFootnoteOrCrossReferenceEndMarker(wxChar* pChar);
	//bool			IsFootnoteOrCrossReferenceEndMarker(wxString str); //overload, str must start with endmarker
	bool			IsFilteredBracketMarker(wxChar *pChar, wxChar* pEnd);
	bool			IsFilteredBracketEndMarker(wxChar *pChar, wxChar* pEnd);
	bool			IsChapterMarker(wxChar* pChar);
	bool			IsAmbiguousQuote(wxChar* pChar);
	bool			IsOpeningQuote(wxChar* pChar);
	bool			IsStraightQuote(wxChar* pChar);
	bool			IsClosingQuote(wxChar* pChar);
	bool			IsClosingCurlyQuote(wxChar* pChar);
	bool			IsAFilteringSFM(USFMAnalysis* pUsfmAnalysis);
	bool			IsAFilteringUnknownSFM(wxString unkMkr);
	//bool			IsMarker(wxChar* pChar, wxChar* pBuffStart);
	bool			IsMarker(wxChar* pChar);
	bool			IsMarker(wxString& mkr); // overloaded version
	bool			IsPrevCharANewline(wxChar* ptr, wxChar* pBuffStart);
	bool			IsEndMarker(wxChar *pChar, wxChar* pEnd);
	bool			IsTextTypeChangingEndMarker(CSourcePhrase* pSrcPhrase);
	bool			IsInLineMarker(wxChar *pChar, wxChar* WXUNUSED(pEnd));
	bool			IsCorresEndMarker(wxString wholeMkr, wxChar *pChar, wxChar* pEnd); // whm added 10Feb05
	bool			IsLegacyDocVersionForFileSaveAs();
	static SPList   *LoadSourcePhraseListFromFile(wxString FilePath);
	USFMAnalysis*	LookupSFM(wxChar *pChar);
	USFMAnalysis*	LookupSFM(wxString bareMkr);
	// BEW created 13Jan11, pass in the CSourcePhrase instance's m_targetStr value (it may
	// have punctuation, and possibly also be a fixed-space (~) conjoined pair; internally
	// parse it & extract and return the equivalent puncuation-less string
	wxString		MakeAdaptionAfterPunctuationChange(wxString& targetStrWithPunctuation, int startingSequNum);
	bool			MarkerExistsInArrayString(wxArrayString* pUnkMarkers, wxString unkMkr, int& MkrIndex);
	bool			MarkerExistsInString(wxString MarkerStr, wxString wholeMkr, int& markerPos);
	wxString		MarkerAtBufPtr(wxChar *pChar, wxChar *pEnd);
	wxString		NormalizeToSpaces(wxString str);
	bool			OpenDocumentInAnotherProject(wxString lpszPathName);
	void			TransferFixedSpaceInfo(CSourcePhrase* pDestSrcPhrase, CSourcePhrase* pFromSrcPhrase);
	int				ParseAdditionalFinalPuncts(wxChar*& ptr, wxChar* pEnd, CSourcePhrase*& pSrcPhrase,
								wxString& spacelessPuncts, int len, bool& bExitOnReturn, 
								bool& bHasPrecedingStraightQuote, wxString& additions,
								bool bPutInOuterStorage);
	int				ParseInlineEndMarkers(wxChar*& ptr, wxChar* pEnd, CSourcePhrase*& pSrcPhrase,
								wxString& inlineNonBindingEndMkrs, int len, 
								bool& bInlineBindingEndMkrFound, wxString& endMkr);
	int				ParseOverAndIgnoreWhiteSpace(wxChar*& ptr, wxChar* pEnd, int len);
	int				ParseMarker(wxChar* pChar);
	int				ParseWhiteSpace(wxChar *pChar);
	int				ParseFilteringSFM(const wxString wholeMkr, wxChar *pChar, 
							wxChar *pBuffStart, wxChar *pEnd);
	wxChar*			FindParseHaltLocation( wxChar* ptr, wxChar* pEnd,
											bool* pbFoundInlineBindingEndMarker,
											bool* pbFoundFixedSpaceMarker,
											bool* pbFoundClosingBracket,
											bool* pbFoundHaltingWhitespace,
											int& nFixedSpaceOffset,
											int& nEndMarkerCount); // BEW created 25Jan11
	void			ParseSpanBackwards(wxString& span, wxString& wordProper, wxString& firstFollPuncts,
							int nEndMkrsCount, wxString& inlineBindingEndMarkers,
							wxString& secondFollPuncts, wxString& ignoredWhiteSpaces,
							wxString& wordBuildersForPostWordLoc, wxString& spacelessPuncts); //BEW created 27Jan11
	wxString		SquirrelAwayMovedFormerPuncts(wxChar* ptr, wxChar* pEnd, wxString& spacelessPuncts); // BEW
								// created 31Jan11, a helper for round tripping punctuation changes
	// BEW 11Oct10, changed contents of ParseWord() majorly, so need new signature
	//int				ParseWord(wxChar *pChar, wxString& precedePunct, wxString& followPunct,wxString& SpacelessSrcPunct);
	int				ParseWord(wxChar *pChar, // pointer to next wxChar to be parsed
							wxChar* pEnd, // pointer to the null at the end of the string buffer
							CSourcePhrase* pSrcPhrase, // where we store what we parse
							wxString& spacelessPuncts, // punctuationset used, with spaces removed
							wxString& inlineNonbindingMrks, // fast access string for \wj \qt \sls \tl \fig
							wxString& inlineNonbindingEndMrks, // fast access string for \wj* \qt* \sls* \tl* \fig*
							bool& bIsInlineNonbindingMkr, // TRUE if pChar is pointing at a beginmarker from
							// the set of five non-binding ones, i.e. \wj \qt \tl \sls or \fig
							bool& bIsInlineBindingMkr); // TRUE if pChar is pointing at a beginmarker
							// from the remaining inline marker set (but excluding \f* and
							// \x* and any others beginning with \f or \x)
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
	int				RetokenizeText(	bool bChangedPunctuation,
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
	void			UpdateFilenamesAndPaths(bool bKBFilename,bool bKBPath,bool bKBBackupPath,
										   bool bGlossingKBPath, bool bGlossingKBBackupPath);
	void			UpdateSequNumbers(int nFirstSequNum, SPList* pOtherList = NULL); // BEW changed 16Jul09
	void			SetFilename(const wxString& filename, bool notifyViews);
	

public:
	// Destructor from the MFC version
	virtual ~CAdapt_ItDoc();

public:
    bool            CollaborationAllowsSaving();        // Checks if safe to Save with regard to collaboration.

protected:
	void			ValidateNoteStorage(); // ensure no m_bHasNote flags are TRUE but lack
						// any note being stored there, and that every stored note has the
						// m_bHasNote flag set TRUE -- run at start of OnOpenDocument()
	bool			ForceAnEmptyUSFMBreakHere(wxString tokBuffer, CSourcePhrase* pSrcPhrase, 
									wxChar* ptr); // BEW added 15Aug12
    
    void            Enable_DVCS_item (wxUpdateUIEvent& event);
    bool            Commit_valid();
    bool            Git_installed();

	bool			ConsistencyCheck_ClobberDoc(CAdapt_ItApp* pApp, bool& bDocIsClosed, bool& bDocForcedToClose,
						CStatusBar* pStatusBar, AFList* afListPtr, AFGList* afgListPtr); // return TRUE if
						// there was no error, otherwise return FALSE - caller should then
						// return as well
#ifdef CONSCHK
	void ListBothArrays(wxArrayString& arrSetNotInKB, wxArrayString& arrRemoveNotInKB);
#endif

	// Generated message map functions (from MFC version) ... most of these should not be
	// public!!! (BEW 17May10)
public:
	void OnFileSave(wxCommandEvent& WXUNUSED(event));
	void OnUpdateFileSave(wxUpdateUIEvent& event);
	void DocChangedExternally();

    // functions related in some way to DVCS:
	void OnTakeOwnership (wxCommandEvent& WXUNUSED(event));
    void OnUpdateTakeOwnership (wxUpdateUIEvent& event);
	void OnSaveAndCommit (wxCommandEvent& WXUNUSED(event));
    void OnUpdateSaveAndCommit (wxUpdateUIEvent& event);
	int  DoSaveAndCommit (wxString blurb);
    void EndTrial (bool restoreBackup);
    void DoChangeVersion ( int revNum );
    void DoShowPreviousVersions ( bool fromLogDialog, int startHere );
	void OnShowPreviousVersions (wxCommandEvent& WXUNUSED(event));
    void DoAcceptVersion (void);
    bool IsLatestVersionChanged (void);
    bool RecoverLatestVersion (void);
    void OnShowFileLog (wxCommandEvent& WXUNUSED(event));
    void OnShowProjectLog (wxCommandEvent& WXUNUSED(event));
    void OnDVCS_Version (wxCommandEvent& WXUNUSED(event));
    void OnUpdateDVCS_item (wxUpdateUIEvent& event);

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
	CCell* 	LayoutDocForConsistencyCheck(CAdapt_ItApp* pApp, CSourcePhrase* pSrcPhrase,
									SPList* pPhrases);
	bool	MatchAutoFixItem(AFList* pList, CSourcePhrase* pSrcPhrase, AutoFixRecord* rpRec);
			// this next one is a variant used for matching AutoFixRecordG for glosses
	bool	MatchAutoFixGItem(AFGList* pList, CSourcePhrase* pSrcPhrase, AutoFixRecordG* rpRec);
	wxChar	GetFirstChar(wxString& strText);
	bool	IsInCaseCharSet(wxChar chTest, wxString& theCharSet, int& index);
	wxChar	GetOtherCaseChar(wxString& charSet, int nOffset);
	void	RemoveAutoFixList(AFList& afList); // for adaptating data
	void	RemoveAutoFixGList(AFGList& afgList); // for glossing data
	bool	m_bHasPrecedingStraightQuote; // default FALSE, set TRUE when a straight quote
	bool	m_bReopeningAfterClosing;	  // default FALSE - set true when we're going to reopen the doc

	DECLARE_EVENT_TABLE()
};

#endif /* Adapt_ItDoc_h */
