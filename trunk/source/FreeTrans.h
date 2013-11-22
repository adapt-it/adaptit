/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FreeTrans.h
/// \author			Graeme Costin
/// \date_created	10 Februuary 2010
/// \rcs_id $Id$
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General
///                 Public License (see license directory)
/// \description	This is the header file for the CFreeTrans class.
/// The CFreeTrans class presents free translation fields to the user.
/// The functionality in the CFreeTrans class was originally contained in
/// the CAdapt_ItView class.
/// \derivation		The CFreeTrans class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef FreeTrans_h
#define FreeTrans_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "FreeTrans.h"
#endif

#include <wx/gdicmn.h>

// forward declarations
class wxObject;
class CAdapt_ItApp;
class CAdapt_ItCanvas;
class CAdapt_ItView;
class CPile;
class CLayout; // make the "friend class CLayout" declaration work
class SPArray;

//GDLC 2010-02-12 Definition of FreeTrElement moved to FreeTrans.h
/// A struct containing the information relevant to writing a subpart of the free
/// translation in a single rectangle under a single strip. Struct members include:
/// horizExtent and subRect.
/// BEW 2Oct11, added int nStripIndex so as to store the index to the owning strip which
/// this particular subRect is associated with. DrawFreeTranslations() won't use it,
/// because that can safely draw outside the view's client area and not mess anything up
/// visually, but DrawFreeTranslationsForPrinting() needs it, because if a free
/// starts in the last strip to be shown on a given page, we don't want free translation
/// data which should be on the next page being printed at the bottom of the current paper
/// sheet - so the latter function will use the value to test for when the rectangle
/// belongs to a strip which should not be printed on the current page; likewise for free
/// translation material which has it's anchor at the end of the previous page
struct FreeTrElement
{
	int horizExtent;
	wxRect subRect;
	int nStripIndex; // BEW added 2Oct11
};

// comment out next line to disable the debugging wxLogDebug() calls wrapped by this
// symbol, for debugging various Print bugs noticed when handling printing of free
// translations
//#define _V6PRINT

//////////////////////////////////////////////////////////////////////////////////
/// The CFreeTrans class presents free translation fields to the user.
/// The functionality in the CFreeTrans class was originally contained in
/// the CAdapt_ItView class.
/// \derivation		The CFreeTrans class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////
class CFreeTrans : public wxEvtHandler
{
	friend class CLayout;
public:

	CFreeTrans(); // default constructor
	CFreeTrans(CAdapt_ItApp* app); // use this one

	virtual ~CFreeTrans();// destructor // whm added always make virtual

	// BEW 16Nov13, turned the local definition (declared in 4 places previously)
	// into a public member variable so that it is visible outside the class instance
	int			nTotalHorizExtent;
	int			m_curTextWidth; // we reset this with every keystroke (ignoring any
								// white space user may have left at the end of the text)
	CPile*		m_pCurAnchorPile; // during document scan for Drawing, as each section is
								  // accessed, the anchor pile is copied to here; we use
								  // this to compare with the anchor for the active section
								  // and when not equal, the popping up of the Adjust dialog
								  // when the being-drawn section shows truncated free trans
								  // is not done - because we do it only at the active section
	int			m_adjust_dlg_reentrancy_limit; // values > 1 must prevent Adjust dlg from showing
	// An array of pointers to CPile instances. It is created on the heap in OnInit(),
	// and disposed of in OnExit().
	// Made public so OnLButtonDown() in CAdapt_ItCanvas can access it.
	// TODO: consider moving the free translation related functionality out of canvas' OnLButtonDown.
	wxArrayPtrVoid*	m_pCurFreeTransSectionPileArray;
	wxArrayPtrVoid*	m_pFollowingSectionPileArray;

#if defined(__WXGTK__)
    void        AggregateOneFreeTranslationForPrinting(wxDC* pDC, CLayout* pLayout, CPile* pCurPile,
                    wxArrayPtrVoid& arrFTElementsArrays, wxArrayPtrVoid& arrFTSubstringsArrays,
                    int nStripsOffset, wxArrayPtrVoid& arrPileSet, wxArrayPtrVoid& arrRectsForOneFreeTrans);
	void		DrawFreeTransForOneStrip(wxDC* pDC, int currentStrip, int nStripsOffset,
                        wxArrayPtrVoid& arrFTElementsArrays, wxArrayPtrVoid& arrFTSubstringsArrays);
    void        DrawOneGloss(wxDC* pDC, CPile* aPilePtr, bool bRTLLayout);
#endif
	bool		ContainsBtMarker(CSourcePhrase* pSrcPhrase); // BEW added 23Apr08
	void		DoCollectBacktranslations(bool bUseAdaptationsLine);
	void		DrawFreeTranslationsAtAnchor(wxDC* pDC, CLayout* pLayout);
	void		DrawFreeTranslations(wxDC* pDC, CLayout* pLayout);
	void		DrawFreeTranslationsForPrinting(wxDC* pDC, CLayout* pLayout);
	void		EraseMalformedFreeTransSections(SPArray* pSPArray);
	bool		GetValueOfFreeTranslationSectioningFlag(SPList* pSrcPhrases,
					int nStartingFreeTransSequNum, int nEndingFreeTransSequNum);
	bool		HaltCurrentCollection(CSourcePhrase* pSrcPhrase, bool& bFound_bt_mkr); // BEW 21Nov05
	bool		IsFreeTransInArray(SPArray* pSPArray);
	bool		IsFreeTranslationSrcPhrase(CPile* pPile);
	void		MarkFreeTranslationPilesForColoring(wxArrayPtrVoid* pileArray);
	void		StoreFreeTranslation(wxArrayPtrVoid* pPileArray,CPile*& pFirstPile,CPile*& pLastPile,
					enum EditBoxContents editBoxContents, const wxString& mkrStr);
	void		StoreFreeTranslationOnLeaving();
	void		SwitchScreenFreeTranslationMode(enum freeTransModeSwitch ftModeSwitch); // klb 9/2011 to support Print Preview
	void		ToggleFreeTranslationMode();

	// the next group are the 22 event handlers
	void		OnAdvanceButton(wxCommandEvent& event);
	void		OnUpdateAdvanceButton(wxUpdateUIEvent& event);
	void		OnAdvancedFreeTranslationMode(wxCommandEvent& event);
	void		OnAdvancedGlossTextIsDefault(wxCommandEvent& WXUNUSED(event));
	void		OnAdvancedTargetTextIsDefault(wxCommandEvent& WXUNUSED(event));
	void		OnAdvancedRemoveFilteredFreeTranslations(wxCommandEvent& WXUNUSED(event));
	void		OnLengthenButton(wxCommandEvent& WXUNUSED(event));
	void		OnNextButton(wxCommandEvent& WXUNUSED(event));
	void		OnPrevButton(wxCommandEvent& WXUNUSED(event));
	void		OnRadioDefineByPunctuation(wxCommandEvent& WXUNUSED(event));
	void		OnUpdateRadioDefineByPunctuation(wxUpdateUIEvent& event);
	void		OnRadioDefineByVerse(wxCommandEvent& WXUNUSED(event));
	void		OnUpdateRadioDefineByVerse(wxUpdateUIEvent& event);
	void		OnRemoveFreeTranslationButton(wxCommandEvent& WXUNUSED(event));
	void		OnShortenButton(wxCommandEvent& WXUNUSED(event));

protected:
	// Public free translation drawing functions
	//GDLC 2010-02-12+ Moved free translation functions here from CAdapt_ItView
	wxString	ComposeDefaultFreeTranslation(wxArrayPtrVoid* arr);
	bool		ContainsFreeTranslation(CPile* pPile);
	void		FixKBEntryFlag(CSourcePhrase* pSrcPhr);
	bool		HasWordFinalPunctuation(CSourcePhrase* pSP, wxString phrase, wxString& punctSet);
	bool		IsFreeTranslationEndDueToMarker(CPile* pNextPile, bool& bAtFollowingPile);

	void		OnUpdateAdvancedFreeTranslationMode(wxUpdateUIEvent& event);
	void		OnUpdateAdvancedGlossTextIsDefault(wxUpdateUIEvent& event);
	void		OnUpdateAdvancedRemoveFilteredFreeTranslations(wxUpdateUIEvent& event);
	void		OnUpdateAdvancedTargetTextIsDefault(wxUpdateUIEvent& event);
	void		OnUpdateLengthenButton(wxUpdateUIEvent& event);
	void		OnUpdateNextButton(wxUpdateUIEvent& event);
	void		OnUpdatePrevButton(wxUpdateUIEvent& event);
	void		OnUpdateRemoveFreeTranslationButton(wxUpdateUIEvent& event);
	void		OnUpdateShortenButton(wxUpdateUIEvent& event);

	// collecting back translations support
	void		OnUpdateAdvancedRemoveFilteredBacktranslations(wxUpdateUIEvent& event);
	void		OnAdvancedRemoveFilteredBacktranslations(wxCommandEvent& WXUNUSED(event));
	void		OnUpdateAdvancedCollectBacktranslations(wxUpdateUIEvent& event);
	void		OnAdvancedCollectBacktranslations(wxCommandEvent& WXUNUSED(event));
	bool		GetNextMarker(wxChar* pBuff,wxChar*& ptr,int& mkrLen);
	wxString	WhichMarker(wxString& markers, int nAtPos); // BEW added 17Sep05, for backtranslation support
	void		InsertCollectedBacktranslation(CSourcePhrase*& pSrcPhrase, wxString& btStr); // BEW added 16Sep05
	// end of collecting back translations support

	// support for the Import Edited Free Translation function
	bool		IsFreeTransInList(SPList* pSPList); // remove later when I make it all SPArray
	int			FindEndOfRuinedSection(SPArray* pSPArray, int startFrom, bool& bFoundSectionEnd,
										bool& bFoundSectionStart, bool& bFoundArrayEnd);
	int			FindNextFreeTransSection(SPArray* pSPArray, int startFrom);
	int			FindFreeTransSectionLackingStart(SPArray* pSPArray, int startFrom);
	bool		CheckFreeTransStructure(SPArray* pSPArray, int startsFrom, int& endsAt, int& malformedAt,
						bool& bHasFlagIsUnset, bool& bLacksEnd, bool& bFoundArrayEnd);

	// Private free translation drawing functions
private:
	void		DestroyElements(wxArrayPtrVoid* pArr);
	CPile*		GetStartingPileForScan(int activeSequNum);
	void		SegmentFreeTranslation(wxDC* pDC,wxString& str, wxString& ellipsis, int textHExtent,
					int totalHExtent, wxArrayPtrVoid* pElementsArray, wxArrayString* pSubstrings, int totalRects);
	void		SingleRectFreeTranslation(	wxDC* pDC, wxString& str, wxString& ellipsis,
						wxArrayPtrVoid* pElementsArray, wxArrayString* pSubstrings);

	wxString	SegmentToFit(wxDC* pDC,wxString& str,wxString& ellipsis,int totalHExtent,float fScale,int& offset,
							int nIteration,int nIterBound,bool& bTryAgain,bool bUseScale);
	// BEW 20Nov13 next ones are refactored ones to reduce the complexity
	wxString	SegmentToFit_UseScaling(wxDC* pDC,wxString& str,int totalHorizExtent,float fScale,
									int& offset,int nIteration,int nIterBound,bool& bFittedOK);
	wxString	SegmentToFit_Tight(wxDC* pDC,wxString& str,wxString& ellipsis,int totalHorizExtent,
							        int& offset,int nIteration,int nIterBound,bool& bFittedOK);
	void		InitiateUserChoice(int selection);
public:
	void		SetupCurrentFreeTransSection(int activeSequNum); // Adapt_It.cpp DoPrintCleanup() needs it
private:
	wxString	TruncateToFit(wxDC* pDC,wxString& str,wxString& ellipsis,int totalHExtent);
	// BEW 2Oct11, added more, for better design of drawing free translations when printing
	void		GetFreeTransPileSetsForPage(CLayout* pLayout, wxArrayPtrVoid& arrPileSets,
											wxArrayString& arrFreeTranslations);
public:
	CPile*		FindNextFreeTransSection(CPile* pStartingPile);
	CPile*		FindPreviousFreeTransSection(CPile* pStartingPile);
	//BEW 27Feb12, a setup function, compliant with docV6, for the two radio buttons in
	//the GUI
	void		SetupFreeTransRadioButtons(bool bSectionByPunctsValue);

	// Support for different inter-pile gap in free translation mode
	void		SetInterPileGapBeforeFreeTranslating();
	void		RestoreInterPileGapAfterFreeTranslating();

private:
	CPile*		FindFreeTransSectionEnd(CPile* pStartingPile);
	void		BuildFreeTransDisplayRects(wxArrayPtrVoid& arrPileSets);
	void		DrawFreeTransStringsInDisplayRects(wxDC* pDC, CLayout* pLayout,
											wxArrayString& arrFreeTranslations);
	void		BuildDrawingRectanglesForSection(CPile* pFirstPile, CLayout* pLayout);
	void		BuildDrawingRectanglesForSectionAtAnchor(CPile* pFirstPile, CLayout* pLayout);
	void		FindSectionPiles(CPile* pFirstPile, wxArrayPtrVoid* pPilesArray, int& wordcount);
	void		EraseDrawRectangle(wxClientDC* pDC, wxRect* pDrawingRect);

#if defined(__WXGTK__)
    // BEW added 21Nov11, part of workaround for DrawFreeTranslationsForPrinting() not working in __WXGTK__ build
	void		GetFreeTransPileSetForOneFreeTrans(CLayout* pLayout, wxArrayPtrVoid& arrPileSet, CPile* pAnchorPile);
	void		BuildFreeTransDisplayRectsForOneFreeTrans(wxArrayPtrVoid& arrPileSet, wxArrayPtrVoid& arrRectsForOneFreeTrans);
    void        AggregateFreeTranslationsByStrip(wxDC* pDC, CLayout* pLayout,
                            wxArrayPtrVoid& arrRectsForOneFreeTrans, wxString& ftStr, int nStripsOffset,
                            wxArrayPtrVoid& arrFTElementsArrays, wxArrayPtrVoid& arrFTSubstringsArrays);
#endif

private:
	CAdapt_ItApp*	m_pApp;	// The app owns this

	CLayout*		m_pLayout;
	CAdapt_ItView*	m_pView;
	CMainFrame*		m_pFrame;

	/// An array of pointers used as a scratch array, for composing the free translations which
	/// are written to screen. Element pointers point to FreeTrElement structs - each of which
	/// contains the information relevant to writing a subpart of the free translation in a
	/// single rectangle under a single strip.
	/// BEW 3Oct11: This member is used by DrawFreeTranslations(), which processses one free
	/// translation section followed by an immediate Draw() of that section, before
	/// iterating the loop
	wxArrayPtrVoid*	m_pFreeTransArray;

    /// BEW 3Oct11: An array of arrays to FreeTrElement structs, each stored array stores
    /// the structs pertaining to one free translation section. This is used for printing
    /// of free translations, because DrawFreeTranslationsForPrinting() collects the
    /// information for all free translation sections on a single printed Page, or
    /// displayed Print Preview page, first; and then in a separate function it loops over
    /// the successive sections doing the Draw() calls and any prior needed segmentations
    /// of the free translation string when multiple strips are involved; hence an array of
    /// arrays is required for printing support
    wxArrayPtrVoid* m_pFreeTransSetsArray; // stores a set of wxArrayPtrVoid pointers

	/// Pointer to first pile in a free translation section.
	//CPile* m_pFirstPile;

	DECLARE_EVENT_TABLE()
};

#endif /* FreeTrans_h */
