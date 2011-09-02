/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Adapt_ItView.h
/// \author			Bill Martin
/// \date_created	05 January 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the header file for the CAdapt_ItView class. 
/// The CAdapt_ItView class is the most complex class in the application. 
/// It controls every aspect of how the
/// data is presented to the user, and most aspects of the user interface. 
/// The data for the view is held entirely in memory and is kept logically 
/// separate from and independent of the document class's persistent data
/// structures. This schemea is an implementation of the document/view 
/// framework.
/// \derivation		The CAdapt_ItView class is derived from wxView.
/////////////////////////////////////////////////////////////////////////////

#ifndef Adapt_ItView_h
#define Adapt_ItView_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Adapt_ItView.h"
#endif

//#include "PhraseBox.h"
//#include "consistentChanger.h"
//#include "FindReplace.h"
#include "SourcePhrase.h"

// forward declarations
//class MyTextCtrl; // temp
class CAdapt_ItCanvas;
class wxFile;

class CAdapt_ItDoc;
class CAdapt_ItView;
class CSourceBundle;
class CSourcePhrase;
class CPile;
class CStrip;
class CCell;
class CRefString;
class CKB;
class MapKeyStringToTgtUnit;
class CLayout;
class PileList;

// WX: The following identifiers are for the three toggled buttons which are
// dynamically swapped into the Toolbar when user clicks the appropriate
// button. They are declared here rather than in Adapt_It_Resources.h
// because these buttons are not defined in the wxDesigner's initial
// design of the Toolbar. We assign them values just above wxID_HIGHEST 
// which is predefined to be 5999.
//#define ID_BUTTON_IGNORING_BDRY 6001
//#define ID_BUTTON_HIDING_PUNCT 6002
//#define ID_SHOW_ALL 6003
//#define ID_BUTTON_ENABLE_PUNCT_COPY 6004
#define ID_CANVAS_WINDOW 6005

struct AutoFixRecord;

/// wxList declaration and partial implementation of the AFList class being
/// a list of pointers to AutoFixRecord objects
WX_DECLARE_LIST(AutoFixRecord, AFList); // see list definition macro in .cpp file

/// wxList declaration and partial implementation of the WordList class being
/// a list of pointers to wxString objects
WX_DECLARE_LIST(wxString, WordList); // see list definition macro in .cpp file

//////////////////////////////////////////////////////////////////////////////////
/// The CAdapt_ItView class is the most complex class in the application. 
/// It controls every aspect of how the data is presented to the user, 
/// and most aspects of the user interface. 
/// The data for the view is held entirely in memory and is kept logically 
/// separate from and independent of the document class's persistent data
/// structures. This schemea is an implementation of the document/view 
/// framework.
/// Note: wxWidgets does not have an equivalent to MFC's CScrollView, so we implement
/// a canvas as a member of the View which is based on CAdapt_ItCanvas which is based 
/// on wxScrolledWindow.
/// \derivation		The CAdapt_ItView class is derived from wxView.
class CAdapt_ItView : public wxView
{
public:
	// the following are for SIL Converters support
	typedef int (wxSTDCALL *wxECInitConverterType)(const wxChar*,int,int);
	typedef int (wxSTDCALL *wxECIsInstalledType)();
	typedef int (wxSTDCALL *wxECConvertStringType)(const wxChar*,const wxChar*,wxChar*,int);

	// wx Note: All MFC coded variables except for our canvas have been moved to the App
	// Use the "canvas" of a wxScrolledWindow for depicting our view
	
	CAdapt_ItCanvas *canvas; // This canvas pointer is owned by the view, 
            // but OnCreate() sets this pointer to always point to the main canvas pointer
            // in CMainFrame

	CAdapt_ItView(); // constructor

	virtual ~CAdapt_ItView();// destructor // whm added always make virtual

	CAdapt_ItDoc* GetDocument();
	// GetMainFrame() is in the App

	////////////////////////////////////////////////////////////////////
	// Note: All of the View's other data members have moved to the App
	////////////////////////////////////////////////////////////////////

	// Below are the View's methods:
	bool OnCreate(wxDocument* doc, long flags); // a virtual method of wxView

	void OnDraw(wxDC* pDC);
	bool PaginateDoc(const int nTotalStripCount, const int nPagePrintingLength); // whm moved to public for wx
	void PrintFooter(wxDC* pDC, wxRect fitRect, float logicalUnitsFactor, int page);

protected:
	void OnPrint(wxCommandEvent& WXUNUSED(event));
	void OnPrintPreview(wxCommandEvent& WXUNUSED(event));
	// wx Note: the following MFC printing routines are methods of CView:
	//    virtual bool OnPreparePrinting(CPrintInfo* pInfo);
	//    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	//    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	// In wxWidgets the above functions are methods of the AIPrintout class based on wxPrintout.
	// The wx equivalents have no parameters in their functions - their signatures are shown below
	// but they are actually methods of wxPrintout and are overridden in the AIPrintout class.
	// Note: MFC's OnPrint() method's name sounds like it should be an event handler, but it
	// is a lower level function that draws the output for each page that is to be printed. Hence,
	// it is more or less equivalent to the wxPrintout::OnPrintPage(int pageNum) method.
	//    void OnPreparePrinting();
	//    void OnBeginPrinting();
	//    void OnEndPrinting();
	//    void OnPrintPage(int pageNum); <- this one is a little different

public:
	void OnInitialUpdate(); // called also from the App so needs to be public
	bool OnClose(bool deleteWindow);

// Implementation
public:
	void		AdjustAlignmentMenu(bool bRTL,bool bLTR);
	bool		AnalyseReference(wxString& chVerse,int& chapter,int& vFirst,int& vLast,int nWantedVerse);
	CRefString*	AutoCapsFindRefString(CTargetUnit* pTgtUnit,wxString adaptation);
	bool		AutoCapsLookup(MapKeyStringToTgtUnit* pMap,CTargetUnit*& pTU,wxString keyStr);
	wxString	AutoCapsMakeStorageString(wxString str, bool bIsSrc = TRUE);

	bool		CheckForVerticalEditBoundsError(CPile* pPile); // whm moved to public for wx version
	void		ChooseTranslation();
	void		ClearPagesList();
	void		ClobberDocument();
	void		CloseProject();
	
	wxString	CopySourceKey(CSourcePhrase* pSrcPhrase, bool bUseConsistentChanges = FALSE); 
	void		DoCollectBacktranslations(bool bUseAdaptationsLine);
	void		DoConditionalStore(bool bOnlyWithinSpan = TRUE); // BEW added 1Aug08
	void		DoConsistencyCheck(CAdapt_ItApp* pApp, CAdapt_ItDoc* pDoc);
	void		DoFileSaveKB();
	bool		DoFindNext(int nCurSequNum, bool bIncludePunct, bool bSpanSrcPhrases, 
						bool bSpecialSearch,bool bSrcOnly, bool bTgtOnly, bool bSrcAndTgt,
						bool bFindRetranslation,bool bFindNullSrcPhrase, bool bFindSFM, 
						wxString& src, wxString& tgt,wxString& sfm, bool bIgnoreCase, 
						int& nSequNum, int& nCount);
	void		DoNotInKB(CSourcePhrase* pSrcPhrase, bool bChoice = TRUE);
	bool		DoReplace(int nActiveSequNum, bool bIncludePunct, wxString& tgt, wxString& replStr,
						int nCount);
	void		DoRetranslation();
	void		DoRetranslationByUpArrow();
	void		DoStartupWizardOnLaunch();
	void		DrawFreeTranslations(wxDC* pDC, CLayout* pLayout, enum DrawFTCaller drawFTCaller);
	void		DrawTextRTL(wxDC* pDC, wxString& str, wxRect& rect); // BEW 9Feb09, a copy is now in CCell
	void		EditSourceText(wxCommandEvent& event);
	void		ExtendSelectionForFind(CCell* pAnchorCell, int nCount);
	bool		ExtendSelectionLeft();
	bool		ExtendSelectionRight();
	bool		ExtractChapterAndVerse(wxString& s,int& nChapter,int& nVerse,bool& bHasChapters,
									bool& bIsVerseRange,int& nFinalVerse);
	int			FindFilteredInsertionLocation(wxString& rStr, wxString& mkr);
	int			FindNoteSubstring(int nCurrentlyOpenNote_SequNum, WordList*& pStrList, int numWords,
									int& nStartOffset, int& nEndOffset);
	void		FindNextHasLanded(int nLandingLocSequNum, bool bSuppressSelectionExtension = TRUE);
	wxString	GetAssocTextWithoutMarkers(wxString mkrStr); // whm added 18Nov05
	wxPanel*	GetBar(enum VertEditBarType vertEditBarType); // BEW added 9Aug08
	wxComboBox*	GetRemovalsComboBox(); // BEW added 18July08
	wxString	GetChapterAndVerse(CSourcePhrase* pSrcPhrase);
	bool		GetChapterAndVerse(SPList* pList, CSourcePhrase* pSrcPhrase, wxString& strChapVerse); // BEW added 12Mar07
	CCell*		GetClickedCell(const wxPoint* pPoint);
	wxString	GetExistingMarkerContent(wxString& mkr, wxString& endMkr,
									CSourcePhrase* pSrcPhrase, int& offset, int & length);
	wxChar		GetFirstChar(wxString& strText);
	CKB*		GetKB();
	CLayout*	GetLayout();
	void		GetMarkerInventoryFromCurrentDoc(); // whm 17Nov05
	CStrip*		GetNearestStrip(const wxPoint *pPoint); // moved here from protected
	CPile*		GetNextEmptyPile(CPile* pPile);
	CSourcePhrase* GetNextEmptySrcPhrase(int nStartingSequNum);
	CPile*		GetNextPile(CPile* pPile);
	wxChar		GetOtherCaseChar(wxString& charSet, int nOffset);
	CPile*		GetPile(const int nSequNum);
	CPile*		GetPrevPile(CPile* pPile);
	CSourcePhrase*  GetFollSafeSrcPhrase(CSourcePhrase* pSrcPhrase);
	CSourcePhrase*  GetPrevSrcPhrase(SPList::Node*& curPos,SPList::Node*& posPrev);
	CSourcePhrase*  GetPrevSafeSrcPhrase(CSourcePhrase* pSrcPhrase);
	CRefString*	    GetRefString(CKB* pKB, int nSrcWords, wxString keyStr, wxString adaptation);
	CSourcePhrase*  GetSrcPhrase(int nSequNum);
	bool		GetSublist(SPList* pSaveList,SPList* pOriginalList,int nBeginSequNum,
						int nEndSequNum);
	int			GetSelectionWordCount();
	CPile*		GetStartingPileForScan(int activeSequNum);
	void		GetVisibleStrips(int& nFirstStrip,int&nLastStrip);
	wxString	GetWholeMarkerFromString(wxString mkrStr, int nBeginPos); // whm added 18Oct05
	void		InitializeEditRecord(EditRecord& editRec); // BEW added 17Apr08
	void		InsertFilteredMaterial(wxString& rMkr, wxString& rEndMkr, wxString contentStr,
					CSourcePhrase* pSrcPhrase, int offsetForInsert, bool bContentOnly); // BEW 6Jul05
	void		InsertNullSrcPhraseBefore();
	void		InsertNullSrcPhraseAfter();
	bool		IsFreeTranslationContentEmpty(CSourcePhrase* pSrcPhrase);
	bool		IsBackTranslationContentEmpty(CSourcePhrase* pSrcPhrase);
	bool		IsItNotInKB(CSourcePhrase* pSrcPhrase);
	bool		IsInCaseCharSet(wxChar chTest, wxString& theCharSet, int& index);
	bool		IsUnstructuredData(SPList* pList);
	bool		IsWrapMarker(CSourcePhrase* pSrcPhrase);
	void		Jump(CAdapt_ItApp* pApp, CSourcePhrase* pNewSrcPhrase);
	void		JumpBackwardToNote_CoreCode(int nJumpOffSequNum);
	void		JumpForwardToNote_CoreCode(int nJumpOffSequNum);
	void		RedoStorage(CKB* pKB, CSourcePhrase* pSrcPhrase, wxString& errorStr);
	void		MakeAllPilesNonCurrent(CLayout* pLayout); // moved here from protected
	void		MarkFreeTranslationPilesForColoring(wxArrayPtrVoid* pileArray); // BEW added 2Jul05
	bool		MarkerTakesAnEndMarker(wxString bareMarkerForLookup, wxString& wantedEndMkr); // whm added 18Nov05
	void		MakeLineFourString(CSourcePhrase* pSrcPhrase, wxString targetStr);
	void		MergeWords();
	void		MoveNote(CSourcePhrase* pFromSrcPhrase,CSourcePhrase* pToSrcPhrase);
	void		MoveToAndOpenFirstNote();
	void		MoveToAndOpenLastNote();
	void		NewRetranslation();
	void		OnAdvanceButton(wxCommandEvent& event); // moved to public
	void		OnPrevButton(wxCommandEvent& WXUNUSED(event)); // moved to public
	void		OnNextButton(wxCommandEvent& WXUNUSED(event)); //moved to public
	void		OnRemoveFreeTranslationButton(wxCommandEvent& WXUNUSED(event)); // moved to public
	void		OnLengthenButton(wxCommandEvent& WXUNUSED(event)); // moved to public
	void		OnShortenButton(wxCommandEvent& WXUNUSED(event)); // moved to public
	void		OnRadioDefineByPunctuation(wxCommandEvent& WXUNUSED(event)); // moved to public
	void		OnRadioDefineByVerse(wxCommandEvent& WXUNUSED(event)); // moved to public
	void		PlacePhraseBox(CCell* pCell, int selector = 0); // use selector to enable/disable code
	bool		PrecedingWhitespaceHadNewLine(wxChar* pChar, wxChar* pBuffStart); // whm added 11Nov05
	void		PutPhraseBoxAtSequNumAndLayout(EditRecord* WXUNUSED(pRec), int nSequNum);	
	CSourcePhrase*	ReDoInsertNullSrcPhrase(SPList* pList,SPList::Node*& insertPos,
											bool bForRetranslation = FALSE);
	void		ReDoMerge(int nSequNum,SPList* pNewList,SPList::Node* posNext,
						CSourcePhrase* pFirstSrcPhrase, int nCount);
	void		RemoveContentWrappers(CSourcePhrase*& pSrcPhrase, wxString mkr, int offset); // BEW 12 Sept05
	void		RemoveKBEntryForRebuild(CSourcePhrase* pSrcPhrase);
	void		RemovePunctuation(CAdapt_ItDoc* pDoc, wxString* pStr, int nIndex);
	void		RemoveRefString(CRefString* pRefString, CSourcePhrase* pSrcPhrase, int nWordsInPhrase);
	void		RemoveSelection();
	
	// Bruce put the following functions in helpers.h and .cpp, but it is only used in the View so I'm putting it
	// on the View
	void		DeepCopySublist2Sublist(SPList* pOriginalList, SPList* pCopiedSublist); // copies a list
	bool		DeepCopySourcePhraseSublist(SPList* pList,int nStartingSequNum, int nEndingSequNum,
						SPList* pCopiedSublist); // BEW added 16Apr08
	bool		PopulateRemovalsComboBox(enum EditStep step, EditRecord* pRec);  // BEW added 18Jul08
	void		RemoveFilterWrappersButLeaveContent(wxString& str);	// removes "\~FILTER" and "\~FILTER*" from
					// str, but leaves the SFM, its content, and any following endmarker followed by any whitespace etc.	
	bool		ReplaceCSourcePhrasesInSpan(SPList* pMasterList, int nStartAt, int nHowMany,
											SPList*  pReplacementsList, int nReplaceStartAt, int nReplaceCount); // BEW added 27May08
	bool		FindNote(SPList* pList, int nStartLoc, int& nFoundAt, bool bFindForwards = TRUE); // BEW added 29May08
	bool		MoveNoteLocationsLeftwardsOnce(wxArrayInt* pLocationsList, int nLeftBoundSN);
	bool		ShiftANoteRightwardsOnce(SPList* pSrcPhrases, int nNoteSN);
	bool		IsNoteStoredHere(SPList* pSrcPhrases, int nNoteSN);
	bool		ShiftASeriesOfConsecutiveNotesRightwardsOnce(SPList* pSrcPhrases, int nFirstNoteSN);
	bool		CreateNoteAtLocation(SPList* pSrcPhrases, int nLocationSN, wxString& strNote);
	bool		BunchUpUnsqueezedLocationsLeftwardsFromEndByOnePlace(int nStartOfEditSpan, int nEditSpanCount,
				wxArrayInt* pUnsqueezedArr, wxArrayInt* pSqueezedArr, int WXUNUSED(nRightBound));
	void		GetMarkerArrayFromString(wxArrayString* pStrArr, const wxString& str); // BEW added 17June08
	bool		IsMarkerInArray(wxArrayString* pStrArr, const wxString& marker); // BEW added 17June08
	bool		AreMarkerSetsDifferent(const wxString& str1, const wxString& str2, bool& bUnfilteringRequired,
							bool& bFilteringRequired);  // created 17June08 BEW
	bool		IsMarkerWithSpaceInFilterMarkersString(wxString& mkrWithSpace, wxString& strFilterMarkers); // BEW added 4July08
	void		SetVerticalEditModeMessage(wxString messageText);
	
	
	void		ResizeBox(const wxPoint* pLoc,const int nWidth,const int nHeight,wxString& text,
									int nStartChar, int nEndChar, CPile* pActivePile);
	int			RecalcPhraseBoxWidth(wxString& phrase);
	void		RestoreMode(bool WXUNUSED(bSeeGlossesEnabled), bool WXUNUSED(bIsGlossing), EditRecord* pRec); // BEW added 29July08
	bool		RestoreOriginalList(SPList* pSaveList,SPList* pOriginalList);
	void		RestoreBoxOnFinishVerticalMode(); // BEW added 8Sept08
	void		SelectDragRange(CCell* pAnchor,CCell* pCurrent);
	void		SelectAnchorOnly();
	void		SelectFoundSrcPhrases(int nNewSequNum, int nCount, bool bIncludePunct, 
									bool bSearchedInSrc, bool bDoRecalcLayout);
	void		SendScriptureReferenceFocusMessage(SPList* pList, CSourcePhrase*);
	bool		SetActivePilePointerSafely(CAdapt_ItApp* pApp,
						SPList* pSrcPhrases,int& nSaveActiveSequNum,int& nActiveSequNum,int nFinish);
	bool		SetCaseParameters(wxString& strText, bool bIsSrcText = TRUE);
	void		SetupCurrentFreeTransSection(int activeSequNum); // BEW added 24Jun05 for free translation support
	bool		SetupRangePrintOp(const int nFromCh, const int nFromV, const int nToCh,
					const int nToV,wxPrintData* WXUNUSED(pPrintData),
					bool WXUNUSED(bSuppressPrecedingHeadingInRange=FALSE), 
					bool WXUNUSED(bIncludeFollowingHeadingInRange=FALSE));
	void		SetWhichBookPosition(wxDialog* pDlg); 
	void		StatusBarMessage(wxString& message);
	bool		StoreBeforeProceeding(CSourcePhrase* pSrcPhrase);
	void		StoreFreeTranslation(wxArrayPtrVoid* pPileArray,CPile*& pFirstPile,CPile*& pLastPile, 
					enum EditBoxContents editBoxContents, const wxString& mkrStr); //moved to public
	void		StoreKBEntryForRebuild(CSourcePhrase* pSrcPhrase, wxString& targetStr, wxString& glossStr);
	bool		StoreText(CKB* pKB, CSourcePhrase* pSrcPhrase, wxString& tgtPhrase, 
										bool bSupportNoAdaptationButton = FALSE);
	bool		StoreTextGoingBack(CKB *pKB, CSourcePhrase *pSrcPhrase, wxString &tgtPhrase);
	void		ToggleFreeTranslationMode(); // BEW added 20Sep08
	void		ToggleGlossingMode(); // BEW added 19Sep08
	void		ToggleSeeGlossesMode(); // BEW added 19Sep08
	int			TokenizeTextString(SPList* pNewList,wxString& str,int nInitialSequNum);
	bool		TransformSourcePhraseAdaptationsToGlosses(SPList::Node* curPos,
										SPList::Node* nextPos, CSourcePhrase* pSrcPhrase);
	void		AdjustDialogPosition(wxDialog* pDlg);
	void		AdjustDialogPositionByClick(wxDialog* pDlg,wxPoint ptClick);
	void		RemoveFinalSpaces(CPhraseBox* pBox,wxString* pStr);
	void		UnmergePhrase();
	void		UpdateSequNumbers(int nFirstSequNum);
	bool		VerticalEdit_CheckForEndRequiringTransition(int nSequNum, ActionSelector select,
											bool bForceTransition = FALSE);
	bool		GetPrevMarker(wxChar* pBuff,wxChar*& ptr,int& mkrLen);

	// The following moved to public from protected
	CCell*		GetPrevCell(CCell* pCell, int index); // moved to public
	bool		IsBoundaryCell(CCell* pCell);
	bool		IsTypeDifferent(CCell* pAnchor, CCell* pCurrent); // moved to public
	bool		RecreateCollectedBackTranslationsInVerticalEdit(EditRecord* pRec, enum EntryPoint anEntryPoint); // BEW added 18Dec08
	void		RemovePrecedingAnchor(wxClientDC* pDC, CCell* pAnchor); // moved to public in wx
	void		RemoveEarlierSelForShortening(wxClientDC* pDC, CCell* pEndCell);
	void		RemoveFollowingAnchor(wxClientDC* pDC, CCell* pAnchor); // moved to public
	void		RemoveLaterSelForShortening(wxClientDC* pDC, CCell* pEndCell);
	void		Invalidate(); // our own for wxWidgets (see cpp file notes)
	wxString	GetConvertedPunct(const wxString& rStr); // whm 10Jan11 moved here from protected
	
// helper functions (protected)
// BEW changed order 19Jul05 to try have something close to alphabetic order in the listing
protected:
	void		AccumulateText(SPList* pList,wxString& strSource,wxString& strAdapt);
	void		BuildRetranslationSourcePhraseInstances(SPList* pRetransList,int nStartSequNum,
													int nNewLength,int nCount,int& nFinish);
	void		BailOutFromEditProcess(SPList* pSrcPhrases, EditRecord* pRec); // BEW added 30Apr08
	void		CheckAndFixNoteFlagInSpans(SPList* pSrcPhrases, EditRecord* pRec);
	void		CheckForMarkers(SPList* pList,bool& bHasInitialMarker,bool& bHasNoninitialMarker);
	void		ClearSublistKBEntries(SPList* pSublist);
	wxString	ComposeDefaultFreeTranslation(wxArrayPtrVoid* arr); // BEW added 26Jun05
	bool		ContainsBtMarker(CSourcePhrase* pSrcPhrase); // BEW added 23Apr08
	bool		ContainsFreeTranslation(CPile* pPile); // BEW added 06Jul05
	bool		CopyCSourcePhrasesToExtendSpan(SPList* pOriginalList, SPList* pDestinationList,
					int nOldList_StartingSN, int nOldList_EndingSN); // BEW added 13May08
	void		CopySourcePhraseList(SPList*& pList,SPList*& pCopiedList,bool bDoDeepCopy = FALSE); // BEW modified 16Apr08
	int			CountSourceWords(wxString& rStr);
	void		DeleteAllNotes();
	void		DeleteTempList(SPList* pList);	// must be a list of ptrs to CSourcePhrase instances on the heap 
	void		DeleteSavedSrcPhraseSublist(SPList* pSaveList); // this list's members can have members in sublists 
	void		DestroyElements(wxArrayPtrVoid* pArr);
	wxString	DoConsistentChanges(wxString& str);
	wxString	DoSilConvert(const wxString& str);
	bool		DoesTheRestMatch(WordList* pSearchList, wxString& firstWord, wxString& noteStr,
									int& nStartOffset, int& nEndOffset);
	bool		DoExtendedSearch(int selector, SPList::Node*& pos, CAdapt_ItDoc* pDoc, 
					SPList* pTempList, int nElements, bool bIncludePunct, bool bIgnoreCase, int& nCount);
	bool		DoFindNullSrcPhrase(int nStartSequNum, int& nSequNum, int& nCount);
	bool		DoFindRetranslation(int nStartSequNum, int& nSequNum, int& nCount);
	bool		DoFindSFM(wxString& sfm, int nStartSequNum, int& nSequNum, int& nCount);
	void		DoKBExport(CKB* pKB, wxFile* pFile);
	void		DoKBImport(CAdapt_ItApp* pApp, wxTextFile* pFile);
	void		DoOneDocReport(wxString& name, SPList* pList, wxFile* pFile);
	void		DoRetranslationReport(CAdapt_ItApp* pApp, CAdapt_ItDoc* pDoc, wxString& name,
									wxArrayString* pFileList,SPList* pList, wxFile* pFile);
	bool		DoSrcAndTgtFind(int nStartSequNum, bool bIncludePunct, bool bSpanSrcPhrases,
						wxString& src,wxString& tgt, bool bIgnoreCase, int& nSequNum, int& nCount);
	bool		DoSrcOnlyFind(int nStartSequNum, bool bIncludePunct, bool bSpanSrcPhrases, 
								wxString& src,bool bIgnoreCase, int& nSequNum, int& nCount);
	void		DoGetSuitableText_ForPlacePhraseBox(CAdapt_ItApp* pApp, CSourcePhrase* pSrcPhrase,
								int selector, CPile* pActivePile, wxString& str, bool bHasNothing,
								bool bNoValidText, bool bSomethingIsCopied); // added 3Apr09
	bool		DoStore_ForPlacePhraseBox(CAdapt_ItApp* pApp, wxString& targetPhrase);	// added 3Apr09
	bool		DoTgtOnlyFind(int nStartSequNum, bool bIncludePunct, bool bSpanSrcPhrases, 
								wxString& tgt,bool bIgnoreCase, int& nSequNum, int& nCount);
	void		DoSrcPhraseSelCopy();
	void		DoTargetBoxPaste(CPile* pPile);
	bool		ExtendEditableSpanForFiltering(EditRecord* pRec, SPList* pSrcPhrases, wxString& strNewSource,
								MapWholeMkrToFilterStatus* WXUNUSED(pMap), bool& bWasExtended); // BEW added 5July08
	bool		ExtendEditSourceTextSelection(SPList* pSrcPhrases, int& nStartingSequNum,
								int& nEndingSequNum, bool& bWasSuccessful); // BEW added 12Apr08
	void		FixKBEntryFlag(CSourcePhrase* pSrcPhr);
	void		GetContext(const int nStartSequNum,const int nEndSequNum,wxString& strPre,
							wxString& strFoll,wxString& strPreTgt,wxString& strFollTgt);
	//wxString	GetConvertedPunct(const wxString& rStr);
	bool		GetEditSourceTextBackTranslationSpan(SPList* pSrcPhrases, int& nStartingSequNum,
							int& nEndingSequNum, int& WXUNUSED(nStartingFreeTransSequNum), 
							int& WXUNUSED(nEndingFreeTransSequNum),int& nStartingBackTransSequNum, 
							int& nEndingBackTransSequNum, bool& bHasBackTranslations, 
							bool& bCollectedFromTargetText); // BEW added 25Apr08
	bool		GetEditSourceTextFreeTranslationSpan(SPList* pSrcPhrases, int& nStartingSequNum,
							int& nEndingSequNum, int& nStartingFreeTransSequNum, 
							int& nEndingFreeTransSequNum, bool& bFreeTransPresent); // BEW added 25Apr08
	bool		GetLikelyValueOfFreeTranslationSectioningFlag(SPList* pSrcPhrases, int nStartingFreeTransSequNum, 
							int nEndingFreeTransSequNum, bool bFreeTransPresent); // BEW added 01Oct08
	bool		GetMovedNotesSpan(SPList* pSrcPhrases, EditRecord* pRec, WhichContextEnum context); // BEW added 14Jun08
	CCell*		GetNextCell(CCell* pCell,  const int cellIndex); // GetNextCell(const CCell* pCell,  const int cellIndex)
	void		GetRetranslationSourcePhrasesStartingAnywhere(CPile* pStartingPile,
													CPile*& pFirstPile,SPList* pList);
	void		GetSelectedSourcePhraseInstances(SPList*& pList,
													wxString& strSource,wxString& strAdapt);
	CTargetUnit*  GetTargetUnit(CKB* pKB, int nSrcWords, wxString keyStr);
	void		GetVerseEnd(SPList::Node*& curPos,SPList::Node*& precedingPos,SPList* WXUNUSED(pList),SPList::Node*& posEnd);
	bool		HasWordFinalPunctuation(CSourcePhrase* pSP, wxString phrase, wxString& punctSet); // BEW modified 25Nov05
	bool		HaltCurrentCollection(CSourcePhrase* pSrcPhrase, bool& bFound_bt_mkr); // BEW 21Nov05
	int			IncludeAPrecedingSectionHeading(int nStartingSequNum, SPList::Node* startingPos, SPList* WXUNUSED(pList));
	void		InsertCollectedBacktranslation(CSourcePhrase*& pSrcPhrase, wxString& btStr); // BEW added 16Sep05
	void		InsertNullSourcePhrase(CAdapt_ItDoc* pDoc,CAdapt_ItApp* pApp,CPile* pInsertLocPile,
					const int nCount,bool bRestoreTargetBox = TRUE,bool bForRetranslation = FALSE,
					bool bInsertBefore = TRUE);
	void		InsertSourcePhrases(CPile* pInsertLocPile, const int nCount,TextType myTextType);
	bool		InsertSublistAtHeadOfList(wxArrayString* pSublist, ListEnum whichList, EditRecord* pRec); // BEW added 29Apr08
	void		InsertSublistAfter(SPList* pSrcPhrases, SPList* pSublist, int nLocationSequNum);
	bool		IsAdaptationInformationInThisSpan(SPList* pSrcPhrases, int& nStartingSN, int& nEndingSN,
												 bool* pbHasAdaptations); // BEW added 15July08
	bool		IsAlreadyInKB(int nWords,wxString key,wxString adaptation);
	bool		IsConstantType(SPList* pList);
	bool		IsContainedByRetranslation(int nFirstSequNum, int nCount, int& nSequNumFirst,
																int& nSequNumLast);
	bool		IsEndInCurrentSelection();
	bool		IsFreeTranslationEndDueToMarker(CPile* pNextPile); // BEW added 7Jul05
	bool		IsFreeTranslationInSelection(SPList* pList); // BEW added 21Nov05, (for edit source text support)
	bool		IsFilteredInfoInSelection(SPList* pList); // whm added 14Aug06
	bool		IsFreeTranslationSrcPhrase(CPile* pPile); // BEW added 24Jun05
	bool		IsGlossInformationInThisSpan(SPList* pSrcPhrases, int& nStartingSN, int& nEndingSN,
					bool* pbHasGlosses);  // BEW added 29Apr08
	bool		IsMember(wxString& rLine, wxString& rMarker, int& rOffset);
	int			IsMatchedToEnd(wxString& strSearch, wxString& strTarget);
	bool		IsNullSrcPhraseInSelection(SPList* pList);
	bool		IsRetranslationInSelection(SPList* pList);
	bool		IsFilteredMaterialNonInitial(SPList* pList);
	bool		IsSameMarker(int str1Len, int nFirstChar, const wxString& str1, const wxString& testStr);
	bool		IsSelectionAcrossFreeTranslationEnd(SPList* pList);
	void		MakeSelectionForFind(int nNewSequNum, int nCount, int nSelectionLine, 
					bool bDoRecalcLayoutInternally);
	bool		MatchAutoFixItem(AFList* pList, CSourcePhrase* pSrcPhrase, AutoFixRecord*& rpRec); // MFC CPtrList*
	void		PadOrShortenAtEnd(SPList* pSrcPhrases,
					int nStartSequNum,int nEndSequNum,int nNewLength,int nCount,TextType myTextType,
					bool& bDelayRemovals);
	void		PadWithNullSourcePhrasesAtEnd(CAdapt_ItDoc* pDoc,CAdapt_ItApp* pApp,
							SPList* pSrcPhrases,int nEndSequNum,int nNewLength,int nCount);
	void		RemoveFinalSpaces(wxString& rStr); // overload of the public function, BEW added 30Apr08
	bool		RemoveInformationDuringEdit(CSourcePhrase* pSrcPhrase, int nSequNum, EditRecord* pRec, 
					wxArrayString* pAdaptList, wxArrayString* pGlossList, wxArrayString* pFTList,
					wxArrayString* pNoteList, bool remAd, bool remGl, bool remNt,
					bool remFT, bool remBT); // BEW added 27Apr08
	void		RemoveNullSourcePhrase(CPile* pInsertLocPile, const int nCount);
	void		RemoveNullSrcPhraseFromLists(SPList*& pList,SPList*& pSrcPhrases,int& nCount,
					int& nEndSequNum,bool bActiveLocAfterSelection,int& nSaveActiveSequNum);
	void		RemoveUnwantedSourcePhraseInstancesInRestoredList(SPList* pSrcPhrases,int nCurCount,
														int nStartingSequNum,SPList* pSublist);
	void		RemoveUnwantedSrcPhrasesInDocList(int nSaveSequNum,int nNewCount,int nCount);
	void		ReplaceMatchedSubstring(wxString strSearch, wxString& strReplace, wxString& strAdapt);
	void		RestoreDocAfterSrcTextEditModifiedIt(SPList* pSrcPhrases, EditRecord* pRec); // BEW added 27May08
	int			RestoreOriginalMinPhrases(CSourcePhrase* pSrcPhrase, int nStartingSequNum);
	void		RestoreOriginalPunctuation(CSourcePhrase* pSrcPhrase);
	void		RestoreTargetBoxText(CSourcePhrase* pSrcPhrase,wxString& str);
	bool		RestoreNotesAfterSourceTextEdit(SPList* pSrcPhrases, EditRecord* pRec); // BEW added 26May08
	bool		ScanSpanDoingRemovals(SPList* pSrcPhrases, EditRecord* pRec,
							wxArrayString* pAdaptList, wxArrayString* pGlossList, wxArrayString* pFTList,
							wxArrayString* pNoteList); //BEW added 30Apr08
	bool		ScanSpanDoingSourceTextReconstruction(SPList* pSrcPhrases, EditRecord* pRec,
					int nStartingSN, int nEndingSN, wxString& strSource); //BEW added 5May08
	void		SegmentFreeTranslation(wxDC* pDC,wxString& str, wxString& ellipsis, int textHExtent,
					int totalHExtent, wxArrayPtrVoid* pElementsArray, wxArrayString* pSubstrings, int totalRects);
	wxString	SegmentToFit(wxDC* pDC,wxString& str,wxString& ellipsis,int totalHExtent,float fScale,int& offset,
							int nIteration,int nIterBound,bool& bTryAgain,bool bUseScale);
	void		SetNotInKBFlag(SPList* pList,bool bValue = TRUE);
	void		SetRetranslationFlag(SPList* pList,bool bValue = TRUE);
	void		StoreFreeTranslationOnLeaving(); // BEW added 11Sep08
	void		TransferCompletedSrcPhrases(SPList* pNewSrcPhrasesList,int nSaveSequNum);
	void		TransferCompletedSrcPhrases(EditRecord* pRec, SPList* pNewSrcPhrasesList,
							SPList* pSrcPhrases, int nBeginAtSN, int nFinishAtSN);
	bool		TransportWidowedEndmarkersToFollowingContext(SPList* pNewSrcPhrases, CSourcePhrase* pFollSrcPhrase,
							EditRecord* pRec); //BEW added 7May08
	wxString	TruncateToFit(wxDC* pDC,wxString& str,wxString& ellipsis,int totalHExtent);
	void		UnmergeMergersInSublist(SPList*& pList,SPList*& pSrcPhrases,int& nCount,
							int& nEndSequNum,bool bActiveLocAfterSelection,int& nSaveActiveSequNum,
							bool bWantRetranslationFlagSet = TRUE,bool bAlsoUpdateSublist = FALSE);
	wxString	WhichMarker(wxString& markers, int nAtPos); // BEW added 17Sep05, for backtranslation support

protected:
	void OnEditPreferences(wxCommandEvent& WXUNUSED(event));
	void OnFileSaveKB(wxCommandEvent& WXUNUSED(event));
	void OnFileCloseProject(wxCommandEvent& event);
	void OnFileStartupWizard(wxCommandEvent& event);
	void OnUpdateFileCloseKB(wxUpdateUIEvent& event);
	void OnUpdateFileNew(wxUpdateUIEvent& event);
	void OnUpdateFileSaveKB(wxUpdateUIEvent& event);
	void OnUpdateFileOpen(wxUpdateUIEvent& event);
	void OnUpdateFilePrint(wxUpdateUIEvent& event);
	void OnUpdateFilePrintPreview(wxUpdateUIEvent& event);
	void OnButtonToEnd(wxCommandEvent& event);
	void OnUpdateButtonToEnd(wxUpdateUIEvent& event);
	void OnButtonToStart(wxCommandEvent& event);
	void OnUpdateButtonToStart(wxUpdateUIEvent& event);
	void OnUpdateButtonStepDown(wxUpdateUIEvent& event);
	void OnButtonStepDown(wxCommandEvent& event);
	void OnUpdateButtonStepUp(wxUpdateUIEvent& event);
	void OnButtonStepUp(wxCommandEvent& event);
	void OnUpdateButtonMerge(wxUpdateUIEvent& event);
	void OnUpdateButtonRestore(wxUpdateUIEvent& event);
	void OnButtonRestore(wxCommandEvent& WXUNUSED(event));
	void OnUpdateEditPreferences(wxUpdateUIEvent& event);
	void OnCheckSingleStep(wxCommandEvent& WXUNUSED(event));
	void OnCheckKBSave(wxCommandEvent& WXUNUSED(event));
	void OnCheckForceAsk(wxCommandEvent& WXUNUSED(event));
	void OnCopySource(wxCommandEvent& WXUNUSED(event));
	void OnUpdateCopySource(wxUpdateUIEvent& event);
	void OnClearContentsButton(wxCommandEvent& WXUNUSED(event));
	void OnSelectAllButton(wxCommandEvent& WXUNUSED(event));
	
	void OnUpdatePrevButton(wxUpdateUIEvent& event);
	void OnUpdateNextButton(wxUpdateUIEvent& event);
	void OnUpdateRemoveFreeTranslationButton(wxUpdateUIEvent& event);
	void OnUpdateLengthenButton(wxUpdateUIEvent& event);
	void OnUpdateShortenButton(wxUpdateUIEvent& event);

	void OnEditCopy(wxCommandEvent& WXUNUSED(event));
	void OnUpdateEditCopy(wxUpdateUIEvent& event);
	void OnEditPaste(wxCommandEvent& WXUNUSED(event));
	void OnUpdateEditPaste(wxUpdateUIEvent& event);
	void OnEditCut(wxCommandEvent& WXUNUSED(event));
	void OnUpdateEditCut(wxUpdateUIEvent& event);
	void OnButtonNullSrc(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonNullSrc(wxUpdateUIEvent& event);
	void OnButtonRemoveNullSrcPhrase(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonRemoveNullSrcPhrase(wxUpdateUIEvent& event);
	void OnRemoveRetranslation(wxCommandEvent& event);
	void OnUpdateRemoveRetranslation(wxUpdateUIEvent& event);
	void OnUpdateButtonEditRetranslation(wxUpdateUIEvent& event);
	void OnButtonEditRetranslation(wxCommandEvent& event);
	void OnButtonChooseTranslation(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonChooseTranslation(wxUpdateUIEvent& event);
	void OnFileExport(wxCommandEvent& WXUNUSED(event));
	void OnUpdateFileExport(wxUpdateUIEvent& event);
	void OnEditConsistencyCheck(wxCommandEvent& WXUNUSED(event));
	void OnUpdateEditConsistencyCheck(wxUpdateUIEvent& event);
	void OnToolsKbEditor(wxCommandEvent& WXUNUSED(event));
	void OnUpdateToolsKbEditor(wxUpdateUIEvent& event);
	void OnGoTo(wxCommandEvent& WXUNUSED(event));
	void OnUpdateGoTo(wxUpdateUIEvent& event);
	void OnFind(wxCommandEvent& event);
	void OnUpdateFind(wxUpdateUIEvent& event);
	void OnReplace(wxCommandEvent& event);
	void OnUpdateReplace(wxUpdateUIEvent& event);
	void OnRetransReport(wxCommandEvent& WXUNUSED(event));
	void OnUpdateRetransReport(wxUpdateUIEvent& event);
	void OnAlignment(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAlignment(wxUpdateUIEvent& event);
	void OnSize(wxSizeEvent& event); //See OnSize in CMainFrame.
	void OnButtonFromRespectingBdryToIgnoringBdry(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonRespectBdry(wxUpdateUIEvent& event);
	void OnButtonFromIgnoringBdryToRespectingBdry(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonIgnoreBdry(wxUpdateUIEvent& event);
	void OnUpdateButtonShowPunct(wxUpdateUIEvent& event);
	void OnButtonFromShowingToHidingPunct(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonHidePunct(wxUpdateUIEvent& event);
	void OnButtonFromHidingToShowingPunct(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonNoPunctCopy(wxUpdateUIEvent& event);
	void OnButtonNoPunctCopy(wxCommandEvent& WXUNUSED(event));
	void OnUpdateMarkerWrapsStrip(wxUpdateUIEvent& event);
	void OnMarkerWrapsStrip(wxCommandEvent& WXUNUSED(event));
	void OnUpdateFileExportKb(wxUpdateUIEvent& event);
	void OnFileExportKb(wxCommandEvent& WXUNUSED(event));
	void OnUpdateShowTgt(wxUpdateUIEvent& event);
	void OnUpdateShowAll(wxUpdateUIEvent& event);
	void OnUpdateEditUndo(wxUpdateUIEvent& event);
	void OnEditUndo(wxCommandEvent& event);
	void OnUpdateImportToKb(wxUpdateUIEvent& event);
	void OnImportToKb(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonBack(wxUpdateUIEvent& event);
	void OnButtonBack(wxCommandEvent& WXUNUSED(event));
	void OnUpdateUnits(wxUpdateUIEvent& event);
	void OnUnits(wxCommandEvent& WXUNUSED(event));
	void OnUpdateUseConsistentChanges(wxUpdateUIEvent& event);
	void OnUpdateUseSilConverter(wxUpdateUIEvent& event);
	void OnUpdateAcceptChanges(wxUpdateUIEvent& event);
	void OnUpdateSelectSilConverters(wxUpdateUIEvent& event);
	void OnAcceptChanges(wxCommandEvent& WXUNUSED(event));
	void OnSelectSilConverters(wxCommandEvent& event);
	void OnRadioDrafting(wxCommandEvent& WXUNUSED(event));
	void OnRadioReviewing(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonEarlierTranslation(wxUpdateUIEvent& event);
	void OnButtonEarlierTranslation(wxCommandEvent& WXUNUSED(event));
	void OnUpdateEditSourceText(wxUpdateUIEvent& event);
	void OnEditSourceText(wxCommandEvent& WXUNUSED(event));
	void OnButtonNoAdapt(wxCommandEvent& event);
	void OnFileExportSource(wxCommandEvent& WXUNUSED(event));
	void OnUpdateFileExportSource(wxUpdateUIEvent& event);
	void OnFileExportToRtf(wxCommandEvent& WXUNUSED(event));
	void OnUpdateFileExportToRtf(wxUpdateUIEvent& event);

	void OnExportGlossesAsText(wxCommandEvent& WXUNUSED(event));
	void OnUpdateExportGlossesAsText(wxUpdateUIEvent& event);
	void OnExportFreeTranslations(wxCommandEvent& WXUNUSED(event));
	void OnUpdateExportFreeTranslations(wxUpdateUIEvent& event);

	void OnButtonCreateNote(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonCreateNote(wxUpdateUIEvent& event);
	void OnUpdateButtonPrevNote(wxUpdateUIEvent& event);
	void OnUpdateButtonNextNote(wxUpdateUIEvent& event);
	void OnButtonDeleteAllNotes(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonDeleteAllNotes(wxUpdateUIEvent& event);
	void OnUpdateButtonEndNow(wxUpdateUIEvent& event);
	void OnButtonEndNow(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonCancelAllSteps(wxUpdateUIEvent& event);
	void OnButtonCancelAllSteps(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonNextStep(wxUpdateUIEvent& event);
	void OnButtonNextStep(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonPrevStep(wxUpdateUIEvent& event);
	void OnButtonPrevStep(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonUndoLastCopy(wxUpdateUIEvent& event);
	void OnButtonUndoLastCopy(wxCommandEvent& WXUNUSED(event));
	void OnUpdateChangeInterfaceLanguage(wxUpdateUIEvent& event);
	void OnChangeInterfaceLanguage(wxCommandEvent& WXUNUSED(event));

public:
	void OnCheckIsGlossing(wxCommandEvent& WXUNUSED(event));
	void OnButtonNextNote(wxCommandEvent& WXUNUSED(event));
	void OnButtonPrevNote(wxCommandEvent& WXUNUSED(event));

	void OnFromShowingAllToShowingTargetOnly(wxCommandEvent& WXUNUSED(event));
	void OnFromShowingTargetOnlyToShowingAll(wxCommandEvent& WXUNUSED(event));
	void OnUseConsistentChanges(wxCommandEvent& WXUNUSED(event));
	void OnUseSilConverter(wxCommandEvent& WXUNUSED(event));

	void OnButtonRetranslation(wxCommandEvent& event); // whm moved to public in wx version
	void OnUpdateButtonRetranslation(wxUpdateUIEvent& event); // whm moved to public in wx version

	void OnAdvancedEnableglossing(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedEnableglossing(wxUpdateUIEvent& event);
	void OnAdvancedGlossingUsesNavFont(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedGlossingUsesNavFont(wxUpdateUIEvent& event);
	void OnAdvancedDelay(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedDelay(wxUpdateUIEvent& event);
	void OnUpdateButtonEnablePunctCopy(wxUpdateUIEvent& event);
	void OnButtonEnablePunctCopy(wxCommandEvent& WXUNUSED(event));
	
	void OnAdvancedFreeTranslationMode(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedFreeTranslationMode(wxUpdateUIEvent& event);
	void OnAdvancedTargetTextIsDefault(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedTargetTextIsDefault(wxUpdateUIEvent& event);
	void OnAdvancedGlossTextIsDefault(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedGlossTextIsDefault(wxUpdateUIEvent& event);
	void OnUpdateAdvancedCollectBacktranslations(wxUpdateUIEvent& event);
	void OnAdvancedCollectBacktranslations(wxCommandEvent& WXUNUSED(event));
	void OnAdvancedRemoveFilteredBacktranslations(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedRemoveFilteredBacktranslations(wxUpdateUIEvent& event);
	void OnAdvancedRemoveFilteredFreeTranslations(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedRemoveFilteredFreeTranslations(wxUpdateUIEvent& event);
	void OnEditMoveNoteForward(wxCommandEvent& WXUNUSED(event));
	void OnUpdateEditMoveNoteForward(wxUpdateUIEvent& event);
	void OnEditMoveNoteBackward(wxCommandEvent& WXUNUSED(event));
	void OnUpdateEditMoveNoteBackward(wxUpdateUIEvent& event);
	void OnAdvancedUseTransliterationMode(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedUseTransliterationMode(wxUpdateUIEvent& event);
	void OnButtonMerge(wxCommandEvent& WXUNUSED(event));

private:
	wxFrame* pCanvasFrame;

	DECLARE_DYNAMIC_CLASS(CAdapt_ItView)
	// DECLARE_DYNAMIC_CLASS() is used inside a class declaration to 
	// declare that the objects of this class should be dynamically 
	// creatable from run-time type information. 

	DECLARE_EVENT_TABLE()
};

inline CAdapt_ItDoc* CAdapt_ItView::GetDocument()
   { return (CAdapt_ItDoc*)m_viewDocument; }



#endif /* Adapt_ItView_h */
