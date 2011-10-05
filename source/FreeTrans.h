/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FreeTrans.h
/// \author			Graeme Costin
/// \date_created	10 Februuary 2010
/// \date_revised	10 Februuary 2010
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
/// because that can safely draw outside the view's client area and no mess anything up
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
	
	// Public free translation drawing functions
	//GDLC 2010-02-12+ Moved free translation functions here from CAdapt_ItView
	wxString	ComposeDefaultFreeTranslation(wxArrayPtrVoid* arr);
	bool		ContainsFreeTranslation(CPile* pPile);
	void		DrawFreeTranslations(wxDC* pDC, CLayout* pLayout);
	void		DrawFreeTranslationsAtAnchor(wxDC* pDC, CLayout* pLayout);
	void		DrawFreeTranslationsForPrinting(wxDC* pDC, CLayout* pLayout);
	void		FixKBEntryFlag(CSourcePhrase* pSrcPhr);
	bool		HasWordFinalPunctuation(CSourcePhrase* pSP, wxString phrase, wxString& punctSet);
	bool		IsFreeTranslationEndDueToMarker(CPile* pNextPile, bool& bAtFollowingPile);
	bool		IsFreeTranslationSrcPhrase(CPile* pPile);
	void		MarkFreeTranslationPilesForColoring(wxArrayPtrVoid* pileArray);
	void		StoreFreeTranslation(wxArrayPtrVoid* pPileArray,CPile*& pFirstPile,CPile*& pLastPile, 
					enum EditBoxContents editBoxContents, const wxString& mkrStr);
	void		StoreFreeTranslationOnLeaving();
	void		ToggleFreeTranslationMode();
	void		SwitchScreenFreeTranslationMode(); // klb 9/2011 to support Print Preview

	// the next group are the 22 event handlers
	void		OnAdvanceButton(wxCommandEvent& event);
	void		OnAdvancedFreeTranslationMode(wxCommandEvent& event);
	void		OnAdvancedGlossTextIsDefault(wxCommandEvent& WXUNUSED(event));
	void		OnAdvancedTargetTextIsDefault(wxCommandEvent& WXUNUSED(event));
	void		OnAdvancedRemoveFilteredFreeTranslations(wxCommandEvent& WXUNUSED(event));
	void		OnLengthenButton(wxCommandEvent& WXUNUSED(event));
	void		OnNextButton(wxCommandEvent& WXUNUSED(event));
	void		OnPrevButton(wxCommandEvent& WXUNUSED(event));
	void		OnRadioDefineByPunctuation(wxCommandEvent& WXUNUSED(event));
	void		OnRadioDefineByVerse(wxCommandEvent& WXUNUSED(event));
	void		OnRemoveFreeTranslationButton(wxCommandEvent& WXUNUSED(event));
	void		OnShortenButton(wxCommandEvent& WXUNUSED(event));

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
	void		DoCollectBacktranslations(bool bUseAdaptationsLine);
	bool		GetNextMarker(wxChar* pBuff,wxChar*& ptr,int& mkrLen);
	bool		ContainsBtMarker(CSourcePhrase* pSrcPhrase); // BEW added 23Apr08
	wxString	WhichMarker(wxString& markers, int nAtPos); // BEW added 17Sep05, for backtranslation support
	void		InsertCollectedBacktranslation(CSourcePhrase*& pSrcPhrase, wxString& btStr); // BEW added 16Sep05
	bool		HaltCurrentCollection(CSourcePhrase* pSrcPhrase, bool& bFound_bt_mkr); // BEW 21Nov05
	// end of collecting back translations support

	// support for the Import Edited Free Translation function
	bool		IsFreeTransInArray(SPArray* pSPArray);
	bool		IsFreeTransInList(SPList* pSPList); // remove later when I make it all SPArray
	void		EraseMalformedFreeTransSections(SPArray* pSPArray);
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
	wxString	SegmentToFit(wxDC* pDC,wxString& str,wxString& ellipsis,int totalHExtent,float fScale,int& offset,
							int nIteration,int nIterBound,bool& bTryAgain,bool bUseScale);
	void		SetupCurrentFreeTransSection(int activeSequNum);
	wxString	TruncateToFit(wxDC* pDC,wxString& str,wxString& ellipsis,int totalHExtent);
	// BEW 2Oct11, added more, for better design of drawing free translations when printing
	void		GetFreeTransPileSetsForPage(CLayout* pLayout, wxArrayPtrVoid& arrPileSets, 
											wxArrayString& arrFreeTranslations);
	CPile*		FindNextFreeTransSection(CPile* pStartingPile);
	CPile*		FindFreeTransSectionEnd(CPile* pStartingPile);
	void		BuildFreeTransDisplayRects(wxArrayPtrVoid& arrPileSets);
	void		DrawFreeTransStringsInDisplayRects(wxDC* pDC, CLayout* pLayout,
											wxArrayString& arrFreeTranslations);

public:
	/// An array of pointers to CPile instances. It is created on the heap in OnInit(), 
	/// and disposed of in OnExit().
	/// Made public so OnLButtonDown() in CAdapt_ItCanvas can access it.
	/// TODO: consider moving the free translation related functionality out of canvas' OnLButtonDown.
	wxArrayPtrVoid*	m_pCurFreeTransSectionPileArray;

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
