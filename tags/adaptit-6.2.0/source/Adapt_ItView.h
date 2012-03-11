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

#if !defined(__WXGTK__)
    // Windows and Mac
	void PrintFooter(wxDC* pDC, wxRect fitRect, float logicalUnitsFactor, int page);
#endif
#if defined(__WXGTK__)
    // Linux
	void PrintFooter(wxDC* pDC, wxPoint marginTopLeft, wxPoint marginBottomRight, wxPoint paperDimensions,
                    float logicalUnitsFactor, int page);
#endif

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
	CAdapt_ItDoc* m_pDoc; // BEW added 14Nov11
	void		AdjustAlignmentMenu(bool bRTL,bool bLTR);
	bool		AnalyseReference(wxString& chVerse,int& chapter,int& vFirst,int& vLast,int nWantedVerse);
	//CRefString*	AutoCapsFindRefString(CTargetUnit* pTgtUnit,wxString adaptation); //moved to CKB
	//bool		AutoCapsLookup(MapKeyStringToTgtUnit* pMap,CTargetUnit*& pTU,wxString keyStr); // moved to CKB
	//wxString	AutoCapsMakeStorageString(wxString str, bool bIsSrc = TRUE); // moved to CKB

	bool		CheckForVerticalEditBoundsError(CPile* pPile); // whm moved to public for wx version
	void		ChooseTranslation();
	void		ClearPagesList();
	void		ClobberDocument();
	void		CloseProject();

	wxString	CopySourceKey(CSourcePhrase* pSrcPhrase, bool bUseConsistentChanges = FALSE);
	void		DoConditionalStore(bool bOnlyWithinSpan = TRUE); // BEW added 1Aug08
	void		DoFileSaveKB();
	bool		DoFindNext(int nCurSequNum, bool bIncludePunct, bool bSpanSrcPhrases,
						bool bSpecialSearch,bool bSrcOnly, bool bTgtOnly, bool bSrcAndTgt,
						bool bFindRetranslation,bool bFindNullSrcPhrase, bool bFindSFM,
						wxString& src, wxString& tgt,wxString& sfm, bool bIgnoreCase,
						int& nSequNum, int& nCount);
	bool		DoReplace(int nActiveSequNum, bool bIncludePunct, wxString& tgt, wxString& replStr,
						int nCount);
	void		DoStartupWizardOnLaunch();
	void		DrawTextRTL(wxDC* pDC, wxString& str, wxRect& rect); // BEW 9Feb09, a copy is now in CCell
	void		EditSourceText(wxCommandEvent& event);
	void		ExtendSelectionForFind(CCell* pAnchorCell, int nCount);
	bool		ExtendSelectionLeft();
	bool		ExtendSelectionRight();
	bool		ExtractChapterAndVerse(wxString& s,int& nChapter,int& nVerse,bool& bHasChapters,
									bool& bIsVerseRange,int& nFinalVerse);
	void		FindNextHasLanded(int nLandingLocSequNum, bool bSuppressSelectionExtension = TRUE);
	wxPanel*	GetBar(enum VertEditBarType vertEditBarType); // BEW added 9Aug08
	wxComboBox*	GetRemovalsComboBox(); // BEW added 18July08
	wxString	GetChapterAndVerse(CSourcePhrase* pSrcPhrase);
	bool		GetChapterAndVerse(SPList* pList, CSourcePhrase* pSrcPhrase, wxString& strChapVerse); // BEW added 12Mar07
	CCell*		GetClickedCell(const wxPoint* pPoint);
	CKB*		GetKB();
	CLayout*	GetLayout();
	bool		GetLikelyValueOfFreeTranslationSectioningFlag(SPList* pSrcPhrases, int nStartingFreeTransSequNum,
							int nEndingFreeTransSequNum, bool bFreeTransPresent); // BEW added 01Oct08
							// moved to public GDLC 2010-02-15
	CStrip*		GetNearestStrip(const wxPoint *pPoint); // moved here from protected
	CPile*		GetNextEmptyPile(CPile* pPile);
	CSourcePhrase* GetNextEmptySrcPhrase(int nStartingSequNum);
	CPile*		GetNextPile(CPile* pPile);
	CPile*		GetPile(const int nSequNum);
	CPile*		GetPrevPile(CPile* pPile);
	CSourcePhrase*  GetFollSafeSrcPhrase(CSourcePhrase* pSrcPhrase);
	CSourcePhrase*  GetPrevSrcPhrase(SPList::Node*& curPos,SPList::Node*& posPrev);
	CSourcePhrase*  GetPrevSafeSrcPhrase(CSourcePhrase* pSrcPhrase);
	CSourcePhrase*  GetSrcPhrase(int nSequNum);
	bool		GetSublist(SPList* pSaveList,SPList* pOriginalList,int nBeginSequNum,
						int nEndSequNum);
	int			GetSelectionWordCount();
	void		GetVisibleStrips(int& nFirstStrip,int&nLastStrip);
	void		InitializeEditRecord(EditRecord& editRec); // BEW added 17Apr08

	bool		IsUnstructuredData(SPList* pList);
	bool		IsWrapMarker(CSourcePhrase* pSrcPhrase);
	void		Jump(CAdapt_ItApp* pApp, CSourcePhrase* pNewSrcPhrase);
	void		MakeTargetStringIncludingPunctuation(CSourcePhrase* pSrcPhrase, wxString targetStr);
	void		MergeWords();

	void		PlacePhraseBox(CCell* pCell, int selector = 0); // use selector to enable/disable code
	void		PutPhraseBoxAtSequNumAndLayout(EditRecord* pRec, int nSequNum);
	void		ReDoMerge(int nSequNum,SPList* pNewList,SPList::Node* posNext,
						CSourcePhrase* pFirstSrcPhrase, int nCount);
	void		RemoveKBEntryForRebuild(CSourcePhrase* pSrcPhrase);
	void		RemovePunctuation(CAdapt_ItDoc* pDoc, wxString* pStr, int nIndex);
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
	void		SendScriptureReferenceFocusMessage(SPList* pList, CSourcePhrase*);
	bool		SetActivePilePointerSafely(CAdapt_ItApp* pApp,
						SPList* pSrcPhrases,int& nSaveActiveSequNum,int& nActiveSequNum,int nFinish);
    bool        DoRangePrintOp(const int nBeginSequNum, const int nEndSequNum,
                               wxPrintData* WXUNUSED(pPrintData)); // BEW created 14Nov11
#if defined(__WXGTK__)
    bool        SetupPageRangePrintOp(const int nFromPage, int nToPage, wxPrintData* pPrintData);
#endif
    // BEW 14Nov11, don't know why Bill has WXUNUSED() here, his code uses these params, so I've removed the WXUNUSED()
	//bool		SetupRangePrintOp(const int nFromCh, const int nFromV, const int nToCh,
	//				const int nToV,wxPrintData* pPrintData,
	//				bool bSuppressPrecedingHeadingInRange=FALSE,
	//				bool bIncludeFollowingHeadingInRange=FALSE);
	bool		SetupRangePrintOp(const int nFromCh, const int nFromV, const int nToCh,
					const int nToV,wxPrintData* pPrintData,
					bool WXUNUSED(bSuppressPrecedingHeadingInRange=FALSE),
					bool WXUNUSED(bIncludeFollowingHeadingInRange=FALSE));
	void		SetWhichBookPosition(wxDialog* pDlg);
	void		StatusBarMessage(wxString& message);
	bool		StoreBeforeProceeding(CSourcePhrase* pSrcPhrase);
	void		StoreKBEntryForRebuild(CSourcePhrase* pSrcPhrase, wxString& targetStr, wxString& glossStr);
	void		ToggleGlossingMode(); // BEW added 19Sep08
	void		ToggleSeeGlossesMode(); // BEW added 19Sep08
	int			TokenizeTextString(SPList* pNewList,wxString& str,int nInitialSequNum);
    // BEW 11Oct10 (actually 11Jan11) overload of TokenizeTextString, to pass in a bool for
    // asking for use of m_punctuation[1] and do a tokenizing of target text with target
    // punctuation settings (useful for a smarter way to support user on-the-fly changes of
    // punctuation settings made from Preferences)
	int			TokenizeTargetTextString(SPList* pNewList, wxString& str, int nInitialSequNum,
										bool bUseTargetTextPuncts);
	bool		TransformSourcePhraseAdaptationsToGlosses(CAdapt_ItApp* pApp, SPList::Node* curPos,
										SPList::Node* nextPos, CSourcePhrase* pSrcPhrase);
	void		AdjustDialogPosition(wxDialog* pDlg);
	void		AdjustDialogPositionByClick(wxDialog* pDlg,wxPoint ptClick);
	void		RemoveFinalSpaces(CPhraseBox* pBox,wxString* pStr);
	void		UnmergePhrase();
	void		UpdateSequNumbers(int nFirstSequNum);
	bool		VerticalEdit_CheckForEndRequiringTransition(int nSequNum, ActionSelector select,
											bool bForceTransition = FALSE);
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

	// property getters / setters
	inline wxString GetSearchString() {return m_SearchStr; }
	inline wxString GetReplacementString() {return m_ReplaceStr; }
	inline void SetSearchString(wxString s) {m_SearchStr = s; }
	inline void SetReplacementString(wxString s) {m_ReplaceStr = s; }

// helper functions (protected)
// BEW changed order 19Jul05 to try have something close to alphabetic order in the listing
protected:
	void		BailOutFromEditProcess(SPList* pSrcPhrases, EditRecord* pRec); // BEW added 30Apr08
	bool		CopyCSourcePhrasesToExtendSpan(SPList* pOriginalList, SPList* pDestinationList,
					int nOldList_StartingSN, int nOldList_EndingSN); // BEW added 13May08
public: // edb 05 March 2010 - set to public (this is called from CRetranslation)
	void		DeleteTempList(SPList* pList);	// must be a list of ptrs to CSourcePhrase instances on the heap
protected:
	wxString	DoConsistentChanges(wxString& str);
	wxString	DoSilConvert(const wxString& str);
	wxString	DoGuess(const wxString& str, bool& bIsGuess);
	bool		DoExtendedSearch(int selector, SPList::Node*& pos, CAdapt_ItDoc* pDoc,
					SPList* pTempList, int nElements, bool bIncludePunct, bool bIgnoreCase, int& nCount);
	bool		DoFindSFM(wxString& sfm, int nStartSequNum, int& nSequNum, int& nCount);
	bool		DoSrcAndTgtFind(int nStartSequNum, bool bIncludePunct, bool bSpanSrcPhrases,
						wxString& src,wxString& tgt, bool bIgnoreCase, int& nSequNum, int& nCount);
	bool		DoSrcOnlyFind(int nStartSequNum, bool bIncludePunct, bool bSpanSrcPhrases,
								wxString& src,bool bIgnoreCase, int& nSequNum, int& nCount);
	void		DoGetSuitableText_ForPlacePhraseBox(CAdapt_ItApp* pApp, CSourcePhrase* pSrcPhrase,
								int selector, CPile* pActivePile, wxString& str, bool bHasNothing,
								bool bNoValidText, bool bSomethingIsCopied); // added 3Apr09
	bool		DoTgtOnlyFind(int nStartSequNum, bool bIncludePunct, bool bSpanSrcPhrases,
								wxString& tgt,bool bIgnoreCase, int& nSequNum, int& nCount);
	void		DoSrcPhraseSelCopy();
	void		DoTargetBoxPaste(CPile* pPile);
	bool		ExtendEditableSpanForFiltering(EditRecord* pRec, SPList* pSrcPhrases, wxString& strNewSource,
								MapWholeMkrToFilterStatus* WXUNUSED(pMap), bool& bWasExtended); // BEW added 5July08
	bool		ExtendEditSourceTextSelection(SPList* pSrcPhrases, int& nStartingSequNum,
								int& nEndingSequNum, bool& bWasSuccessful); // BEW added 12Apr08
public: // edb 05 March 2010 - set to public (this is called from CRetranslation)
	void		GetContext(const int nStartSequNum,const int nEndSequNum,wxString& strPre,
							wxString& strFoll,wxString& strPreTgt,wxString& strFollTgt);
protected:
	bool		GetEditSourceTextBackTranslationSpan(SPList* pSrcPhrases, int& nStartingSequNum,
							int& nEndingSequNum, int& WXUNUSED(nStartingFreeTransSequNum),
							int& WXUNUSED(nEndingFreeTransSequNum),int& nStartingBackTransSequNum,
							int& nEndingBackTransSequNum, bool& bHasBackTranslations,
							bool& bCollectedFromTargetText); // BEW added 25Apr08
	bool		GetEditSourceTextFreeTranslationSpan(SPList* pSrcPhrases, int& nStartingSequNum,
							int& nEndingSequNum, int& nStartingFreeTransSequNum,
							int& nEndingFreeTransSequNum, bool& bFreeTransPresent); // BEW added 25Apr08
	CCell*		GetNextCell(CCell* pCell,  const int cellIndex); // GetNextCell(const CCell* pCell,  const int cellIndex)
	void		GetVerseEnd(SPList::Node*& curPos,SPList::Node*& precedingPos,SPList* WXUNUSED(pList),SPList::Node*& posEnd);
	int			IncludeAPrecedingSectionHeading(int nStartingSequNum, SPList::Node* startingPos, SPList* WXUNUSED(pList));
protected:
	void		InsertSourcePhrases(CPile* pInsertLocPile, const int nCount,TextType myTextType);
	bool		DoFindNullSrcPhrase(int nStartSequNum, int& nSequNum, int&   nCount);
public:
	bool		InsertSublistAtHeadOfList(wxArrayString* pSublist, ListEnum whichList, EditRecord* pRec); // BEW added 29Apr08
protected:
	bool		IsAdaptationInformationInThisSpan(SPList* pSrcPhrases, int& nStartingSN, int& nEndingSN,
												 bool* pbHasAdaptations); // BEW added 15July08
	bool		IsFreeTranslationInSelection(SPList* pList); // BEW added 21Nov05, (for edit source text support)
	bool		IsFilteredInfoInSelection(SPList* pList); // whm added 14Aug06
	bool		IsGlossInformationInThisSpan(SPList* pSrcPhrases, int& nStartingSN, int& nEndingSN,
					bool* pbHasGlosses);  // BEW added 29Apr08
	int			IsMatchedToEnd(wxString& strSearch, wxString& strTarget);
	bool		IsFilteredMaterialNonInitial(SPList* pList);
	bool		IsSameMarker(int str1Len, int nFirstChar, const wxString& str1, const wxString& testStr);
	bool		IsSelectionAcrossFreeTranslationEnd(SPList* pList);
	void		RemoveFinalSpaces(wxString& rStr); // overload of the public function, BEW added 30Apr08
	bool		RemoveInformationDuringEdit(CSourcePhrase* pSrcPhrase, int nSequNum, EditRecord* pRec,
					wxArrayString* pAdaptList, wxArrayString* pGlossList, wxArrayString* pFTList,
					wxArrayString* pNoteList, bool remAd, bool remGl, bool remNt,
					bool remFT, bool remBT); // BEW added 27Apr08
	void		RestoreDocAfterSrcTextEditModifiedIt(SPList* pSrcPhrases, EditRecord* pRec); // BEW added 27May08
public: // edb 05 March 2010 - need this public in order to call it from CRetranslation
	int			RestoreOriginalMinPhrases(CSourcePhrase* pSrcPhrase, int nStartingSequNum);
	void		MakeSelectionForFind(int nNewSequNum, int nCount, int nSelectionLine,
									 bool bDoRecalcLayoutInternally);
	void		ToggleCopySource();
protected:
	bool		ScanSpanDoingRemovals(SPList* pSrcPhrases, EditRecord* pRec,
							wxArrayString* pAdaptList, wxArrayString* pGlossList, wxArrayString* pFTList,
							wxArrayString* pNoteList); //BEW added 30Apr08
	bool		ScanSpanDoingSourceTextReconstruction(SPList* pSrcPhrases, EditRecord* pRec,
					int nStartingSN, int nEndingSN, wxString& strSource); //BEW added 5May08
	void		TransferCompletedSrcPhrases(EditRecord* pRec, SPList* pNewSrcPhrasesList,
							SPList* pSrcPhrases, int nBeginAtSN, int nFinishAtSN);
	bool		TransportWidowedFilteredInfoToFollowingContext(SPList* pNewSrcPhrases,
							CSourcePhrase* pFollSrcPhrase, EditRecord* pRec); //BEW added 7May08
							// 22Mar10, name changed from TransportWidowedEndmarkersToFollowingContext

protected:
	void OnEditPreferences(wxCommandEvent& WXUNUSED(event));
	void OnFileSaveKB(wxCommandEvent& event);
	void OnFileCloseProject(wxCommandEvent& event);
	void OnFileStartupWizard(wxCommandEvent& WXUNUSED(event));
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
	void OnCopySource(wxCommandEvent& event);
	void OnUpdateCopySource(wxUpdateUIEvent& event);
	void OnClearContentsButton(wxCommandEvent& WXUNUSED(event));
	void OnSelectAllButton(wxCommandEvent& WXUNUSED(event));
	void OnImportEditedSourceText(wxCommandEvent& WXUNUSED(event));
	void OnUpdateImportEditedSourceText(wxUpdateUIEvent& event);

	void OnEditCopy(wxCommandEvent& WXUNUSED(event));
	void OnUpdateEditCopy(wxUpdateUIEvent& event);
	void OnEditPaste(wxCommandEvent& WXUNUSED(event));
	void OnUpdateEditPaste(wxUpdateUIEvent& event);
	void OnEditCut(wxCommandEvent& WXUNUSED(event));
	void OnUpdateEditCut(wxUpdateUIEvent& event);
	void OnButtonChooseTranslation(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonChooseTranslation(wxUpdateUIEvent& event);
	void OnFileExport(wxCommandEvent& WXUNUSED(event));
	void OnUpdateFileExport(wxUpdateUIEvent& event);
	//void OnExportOXES(wxCommandEvent& WXUNUSED(event)); // BEW removed 15Jun11, until
														  //OXES support is needed
	//void OnUpdateExportOXES(wxUpdateUIEvent& event);
	void OnToolsKbEditor(wxCommandEvent& WXUNUSED(event));
	void OnUpdateToolsKbEditor(wxUpdateUIEvent& event);
	void OnGoTo(wxCommandEvent& WXUNUSED(event));
	void OnUpdateGoTo(wxUpdateUIEvent& event);
	void OnFind(wxCommandEvent& event);
	void OnUpdateFind(wxUpdateUIEvent& event);
	void OnReplace(wxCommandEvent& event);
	void OnUpdateReplace(wxUpdateUIEvent& event);
	void OnAlignment(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAlignment(wxUpdateUIEvent& event);
	void OnSize(wxSizeEvent& event); //See OnSize in CMainFrame.
	void OnButtonFromRespectingBdryToIgnoringBdry(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonRespectBdry(wxUpdateUIEvent& event);
public: // edb 05 March 2010 - set to public (we call this from CRetranslation)
	void OnButtonFromIgnoringBdryToRespectingBdry(wxCommandEvent& WXUNUSED(event));
protected:
	void OnUpdateButtonIgnoreBdry(wxUpdateUIEvent& event);
	void OnUpdateButtonShowPunct(wxUpdateUIEvent& event);
	void OnButtonFromShowingToHidingPunct(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonHidePunct(wxUpdateUIEvent& event);
	void OnButtonFromHidingToShowingPunct(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonNoPunctCopy(wxUpdateUIEvent& event);
	void OnButtonNoPunctCopy(wxCommandEvent& event);
	void OnUpdateMarkerWrapsStrip(wxUpdateUIEvent& event);
	void OnMarkerWrapsStrip(wxCommandEvent& event);
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
	void OnRadioDrafting(wxCommandEvent& event);
	void OnRadioReviewing(wxCommandEvent& event);
	void OnUpdateButtonEarlierTranslation(wxUpdateUIEvent& event);
	void OnButtonEarlierTranslation(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonGuesserSettings(wxUpdateUIEvent& event);
	void OnButtonGuesserSettings(wxCommandEvent& WXUNUSED(event));
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
	void OnFromShowingAllToShowingTargetOnly(wxCommandEvent& WXUNUSED(event));
	void OnFromShowingTargetOnlyToShowingAll(wxCommandEvent& WXUNUSED(event));
	void OnUseConsistentChanges(wxCommandEvent& WXUNUSED(event));
	void OnUseSilConverter(wxCommandEvent& WXUNUSED(event));
	void OnAdvancedSeeGlosses(wxCommandEvent& event);
	void OnUpdateAdvancedEnableglossing(wxUpdateUIEvent& event);
	void OnAdvancedGlossingUsesNavFont(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedGlossingUsesNavFont(wxUpdateUIEvent& event);
	void OnAdvancedDelay(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedDelay(wxUpdateUIEvent& event);
	void OnUpdateButtonEnablePunctCopy(wxUpdateUIEvent& event);
	void OnButtonEnablePunctCopy(wxCommandEvent& event);
	void OnAdvancedUseTransliterationMode(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedUseTransliterationMode(wxUpdateUIEvent& event);
	void OnButtonMerge(wxCommandEvent& WXUNUSED(event));
	void ShowGlosses();

private:
	wxFrame* pCanvasFrame;

	// search and replace strings
	// These are used for inserting a replacement target text into a retranslation, when the latter is
	// wholly or partly matched (CRetranslation::OnButtonEditRetranslation() makes use of these vars)
	wxString m_SearchStr;
	wxString m_ReplaceStr;

	DECLARE_DYNAMIC_CLASS(CAdapt_ItView)
	// DECLARE_DYNAMIC_CLASS() is used inside a class declaration to
	// declare that the objects of this class should be dynamically
	// creatable from run-time type information.

	DECLARE_EVENT_TABLE()
};

inline CAdapt_ItDoc* CAdapt_ItView::GetDocument()
   { return (CAdapt_ItDoc*)m_viewDocument; }

#endif /* Adapt_ItView_h */
