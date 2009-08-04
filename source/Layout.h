/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Layout.h
/// \author			Bruce Waters
/// \date_created	09 February 2009
/// \date_revised	
/// \copyright		2009 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public 
///                 License (see license directory)
/// \description	This is the implementation file for the CLayout class. 
/// The CLayout class replaces the legacy CSourceBundle class, and it encapsulated as much as
/// possible of the visible layout code for the document. It manages persistent CPile objects
/// for the whole document - one per CSourcePhrase instance. Copies of the CPile pointers are
/// stored in CStrip objects. CStrip, CPile and CCell classes are used in the refactored layout,
/// but with a revised attribute inventory; and wxPoint and wxRect members are computed on the
/// fly as needed using functions, which reduces the layout computations when the user does things
/// by a considerable amount.
/// \derivation		The CLayout class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

// use this define to turn on wxLogDebug() calls in OnPrintPage() and CLayout::Draw()
//#define Print_failure


#ifndef Layout_h
#define Layout_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Layout.h"
#endif

/////////// FRIENDSHIPS in the layout functionality, and their meanings /////////////////
///
///    CLayout is a friend for CStrip, CPile & CCell
///    CStrip  is a friend for CPile & CLayout
///    CPile   is a friend for CStrip & CLayout
///    CCell   is a friend for CLayout
///    CLayout* is passed as a parameter in the CreateXXXX() function for CStrip, CPile, CCell.
///    CStrip is passed as a parameter to CPile, CPile is passed as a parameter to CCell.
///    The friendships to CLayout mean that CStrip, CPile, and CCell may access CLayout's
///    private members. Implication? We can dispense with getters in CLayout for those private
///    members. We do need CLayout to have setters for the private members, as various parts of
///    the application will, when the user changes some setting or other, need to update the
///    relevant private members of CLayout safely.
///
/////////////////////////////////////////////////////////////////////////////////////////

// forward references
class CAdapt_ItDoc;
class CSourceBundle;
class CPile;
class CFont;
class CAdapt_ItCanvas;
class CLayout;
class PileList;

#define Do_Clipping

// add one of these to the end of the handler for a user editing operation, and
// because of the possibility of nested operations (suboperations) the best
// location is to put the the relevant operatation enum value immediately before
// the Invalidate() call at the end of the function handling each particular
// sub-operation
enum doc_edit_op {
	no_edit_op, // a "do nothing" case
	default_op,
	char_typed_op,
	cancel_op,
	target_box_paste_op,
	relocate_box_op,
	merge_op,
	unmerge_op,
	retranslate_op,
	remove_retranslation_op,
	edit_retranslation_op,
	insert_placeholder_op,
	remove_placeholder_op,
	edit_source_text_op,
	free_trans_op,
	end_free_trans_op,
	retokenize_text_op,
	consistency_check_op,
	split_op,
	join_op,
	move_op,
	on_button_no_adaptation_op,
	collect_back_translations_op,
	vert_edit_enter_adaptions_op,
	vert_edit_exit_adaptions_op,
	vert_edit_enter_glosses_op,
	vert_edit_exit_glosses_op,
	vert_edit_enter_free_trans_op,
	vert_edit_exit_free_trans_op,
	vert_edit_cancel_op,
	vert_edit_end_now_op,
	vert_edit_previous_step_op,
	vert_edit_exit_op,
	vert_edit_bailout_op,
	exit_preferences_op,
	change_punctuation_op,
	change_filtered_markers_only_op,
	change_sfm_set_only_op,
	change_sfm_set_and_filtered_markers_op,
	open_document_op,
	new_document_op,
	close_document_op,
	enter_LTR_layout_op,
	enter_RTL_layout_op,
	invalid_op_enum_value // this one must always be last
};

enum layout_selector {
	create_strips_and_piles,
	create_strips_keep_piles,
	keep_strips_keep_piles,
	create_strips_update_pile_widths
};

/// The CLayout class manages the layout of the document. It's private members pull
/// together into one place parameters pertinent to dynamically laying out the strips piles
/// and cells of the layout. Setters in various parts of the application set these private
/// members, and getters are used by the layout functionalities to get the drawing done
/// correctly
/// \derivation		The CLayout class is derived from wxObject.
class CLayout : public wxObject  
{
	friend class CStrip;
	friend class CPile;
	friend class CCell;

public:
	// constructors
	CLayout(); // default constructor

	// attributes
	CAdapt_ItApp*		m_pApp;
	CAdapt_ItDoc*		m_pDoc;
	CAdapt_ItView*		m_pView;
	CAdapt_ItCanvas*	m_pCanvas;
	CMainFrame*			m_pMainFrame;

	doc_edit_op			m_docEditOperationType; // set in user doc edit handler functions
												// and used by PlacePhraseBoxInLayout() 
	int					m_curBoxWidth;

	// booleans that track what kind of changes were made within the Preferences dialog -
	// these govern which parameters we pass to RecalcLayout() - whether to keep or create
	// piles, recalc pile widths, call SetupLayoutParameters(), keep or create strips, etc
	// There is nothing to be gained by making these private
	bool		m_bViewParamsChanged; // we care only about leading, margin, gap
									  // and the phrase box slop multiplier; so
									  // update layout settings & recreate strips
	bool		m_bUSFMChanged; // if changed, full rebuilding
								// of the layout should be done
	bool		m_bFilteringChanged; // if changed, full rebuilding
									 // of the layout should be done
	bool		m_bPunctuationChanged; // if changed, full rebuilding
									   // of the layout should be done
	bool		m_bCaseEquivalencesChanged; // if changed, recalc pile widths
											// and recreate the strips
	bool		m_bFontInfoChanged; // if changed, update layout settings, recalc
									// pile widths, recreate strips
	PageOffsets* m_pOffsets; // the PageOffsets instance in use when printing current page
						     // or when print previewing a given page (set by OnPrintPage()
						     // in AIPringout.cpp; bool gbIsPrinting must be TRUE when this
						     // member is accessed by Draw()
	bool		m_bInhibitDraw; // set TRUE when a process is midstream and the view's 
								// strips and piles are not in a consistent state, else
								// it should be FALSE
private:
	PileList			m_pileList;
	PileList*			m_pSavePileList; // define on the heap, for saving the original list
										 // when doing a range print over a subset within 
										 // m_pileList
	wxArrayPtrVoid		m_stripArray;
	wxArrayInt			m_invalidStripArray;
	enum layout_selector	m_lastLayoutSelector; // RecalcLayout() sets it, Draw() uses it

private:
#ifdef Do_Clipping
	// four ints define the clip rectange top, left, width & height for erasure 
	// prior to draw (window client coordinates, (0,0) is client top left)
	int			m_nClipRectTop;
	int			m_nClipRectLeft;
	int			m_nClipRectWidth;
	int			m_nClipRectHeight;
	bool		m_bScrolling; // TRUE when scrolling is happening
	bool		m_bDoFullWindowDraw;	
#endif
    // use the following to suppress phrasebox being made visible in the middle of
    // procedures when strips have to be updated but we've not yet got to the final layout
    bool		m_bLayoutWithoutVisiblePhraseBox;

	// private copies of the src, tgt &navText colors stored in app class
	wxColour	m_srcColor;
	wxColour	m_tgtColor;
	wxColour	m_navTextColor;

	// private copy of the src, tgt and gloss text heights (from Font metrics) stored on app
	int			m_nSrcHeight;
	int			m_nTgtHeight;
	int			m_nNavTextHeight;

	// font pointers
	wxFont*		m_pSrcFont;
	wxFont*		m_pTgtFont;
	wxFont*		m_pNavTextFont;

	// user edit span delimitation
	int			m_nStartUserEdits; // set to -1 when value is not to be used
	int			m_nEndUserEdits;   // ditto

	//bool		m_bShowTargetOnly; // we won't bother just yet, retain the global

    // the pile height -- this changes in value only when one or more of the font metrics
    // are changed, such as bolding, point size, face, etc. Strip height has more in it. We
    // also have the current leading value for the strips (the nav text whiteboard height),
    // left margin for strips
	int			m_nPileHeight;
	int			m_nStripHeight;
	int			m_nCurLeading;
	int			m_nCurLMargin;
	int			m_nCurGapWidth;
	int			m_nSaveLeading;
	int			m_nSaveGap;
	int			m_numVisibleStrips;

    // client size (width & height as a wxSize) based on Bill's calculation in the
    // CMainFrame, and then as a spin off, the document width (actually m_logicalDocSize.x)
    // and we initialize docSize.y to 0, and set that value later when all the strips are
    // laid out; the setter follows code found in the legacy RecalcLayout() function on the
    // view class
	wxSize		m_sizeClientWindow; // .x is width, .y is height (control bars taken 
									// into account)
	wxSize		m_logicalDocSize;   // the m_logicalDocSize.x  value is the strip width 
                    // value to be used for filling a strip with CPile objects; the .y
                    // value, plus 40 pixels, is the range to be used for the vertical
                    // scroll bar NOTE: *** TODO *** Bill's Canvas class inherits from
                    // wxScrollingWindow, which has a virtual function SetVirtualSize()
                    // which can be used to define a virtual size different from the client
                    // window's width (and height) -- Bill sets it using that function at
                    // the end of RecalcLayout(), so the CLayout setup of the strips should
                    // end with the same
public:
	// destructor
	virtual ~CLayout();
	virtual void Draw(wxDC* pDC);

	// helpers; setters & getters
	CAdapt_ItApp*		GetApp();
	CAdapt_ItView*		GetView();
	CAdapt_ItCanvas*	GetCanvas();
	CAdapt_ItDoc*		GetDoc();
	CMainFrame*			GetMainFrame(CAdapt_ItApp* pApp);
	void		InitializeCLayout();   // called only once, in view class's OnInitialUpdate()
	void		SetLayoutParameters(); // call this to get CLayout's private parameters updated
									   // to whatever is currently set within the app, doc and
									   // view classes; that is, it hooks up CLayout to the
									   // legacy parameters wherever they were stored (it calls
									   // app class's UpdateTextHeights() function too
	// Strip destructors
	void		DestroyStrip(int index); // note: doesn't destroy piles and their cells, these 
										 // are managed by m_pPiles list & must persist
	void		DestroyStripRange(int nFirstStrip, int nLastStrip);
	void		DestroyStrips();// RecalcLayout() calls this

	// Pile destructors (for the persistent ones in CLayout::m_pPiles list) - note, 
	// destroying a pile also, in the same function, destroys its array of CCell instances
	void		DestroyPile(CPile* pPile, PileList* pPileList, bool bRemoveFromListToo = TRUE);
	void		DestroyPiles();

	// for setting or clearing the m_bLayoutWithoutVisiblePhraseBox boolean
	void		SetBoxInvisibleWhenLayoutIsDrawn(bool bMakeInvisible);
#ifdef Do_Clipping
	// getters & setters for clipping and clipping rectangle (if we do clipping, the view
	// is clipped to the active strip - but only when this makes sense, such as when the
	// phrase box is not resized by a character typed by the user, and scrolling is not
	// happening)
	void		SetFullWindowDrawFlag(bool bFullWndDraw);
	bool		GetFullWindowDrawFlag();
	void		SetScrollingFlag(bool bIsScrolling);
	bool		GetScrollingFlag();
#endif
	// setters and getters for font pointers
	void		SetSrcFont(CAdapt_ItApp* pApp);
	void		SetTgtFont(CAdapt_ItApp* pApp);
	void		SetNavTextFont(CAdapt_ItApp* pApp);

	// getters and setters for m_nCurLeading and m_nCurGapWidth
	// (these mirror the app's m_curLeading and m_curGapWidth; and the 
	// "Saved" ones are for removing the globals gnSaveGap and gnSaveLeading
	int			GetSavedLeading();
	int			GetSavedGapWidth();
	void		SetSavedLeading(int nCurLeading);
	void		SetSavedGapWidth(int nGapWidth);

	// setters and getters for text colors
	void		SetSrcColor(CAdapt_ItApp* pApp);
	void		SetTgtColor(CAdapt_ItApp* pApp);
	void		SetNavTextColor(CAdapt_ItApp* pApp);
	wxColour	GetSrcColor();
	wxColour	GetTgtColor();
	wxColour	GetNavTextColor();
	wxColour	GetSpecialTextColor();
	wxColour	GetRetranslationTextColor();

	// setters and getters for source, target and navText heights (from TEXTMETRICS), the 
	// setters access members on the app & later can bypass them when we refactor further
	void		SetSrcTextHeight(CAdapt_ItApp* pApp);
	void		SetTgtTextHeight(CAdapt_ItApp* pApp);
	void		SetNavTextHeight(CAdapt_ItApp* pApp);
	int			GetSrcTextHeight();
	int			GetTgtTextHeight();
	int			GetNavTextHeight();

	// current gap width between piles (in pixels)
	void		SetGapWidth(CAdapt_ItApp* pApp);
	int			GetGapWidth();

	// setter and getter for the pile height & strip height; 
	// also the current leading value
	void		SetPileAndStripHeight();
	int			GetPileHeight();
	int			GetStripHeight();
	void		SetCurLeading(CAdapt_ItApp* pApp);
	int			GetCurLeading();

	// left margin for strips
	void		SetCurLMargin(CAdapt_ItApp* pApp);
	int			GetStripLeft(); // use this instead of GetCurLMargin()

	void		SetClientWindowSizeAndLogicalDocWidth();
	void		SetLogicalDocHeight();	// set m_logicalDocSize.y 
										// (call after strips are built)
	wxSize		GetClientWindowSize();
	wxSize		GetLogicalDocSize();
	void		CopyLogicalDocSizeFromApp(); // copy the CAdapt_ItApp:m_docSize 
                        // wxSize value to wxSize m_logicalDocSize in CLayout,
                        // for use while printing
	void		RestoreLogicalDocSizeFromSavedSize(); // copy the wxSize value 
                        // saved in the CAdapt_ItApp:m_saveDocSize member, back to
                        // m_logicalDocSize here in CLayout, for use again in the view
                        // refreshes, after printing

	// getters for the m_pileList, and m_stripArray, and m_invalidStripArray
	PileList*		GetPileList();
	PileList*		GetSavePileList(); // also creates it on heap if its 
									   // pointer is currently NULL
	wxArrayPtrVoid* GetStripArray();
	wxArrayInt*		GetInvalidStripArray();
	void			ClearSavePileList();

	//////// public utility functions ////////
	 
	// updating the m_nStrip index values after insertion or removal of CStrip instance(s) from
	// the layout
	void		UpdateStripIndices(int nStartFrom = 0); // get the pile pointer for 
														// a given sequNumber passed in
	CPile*		GetPile(int index); // get the strip index from a passed in sequNumber 
									// for a pile in m_pileList
	int			GetStripIndex(int nSequNum); // get the strip pointer from a passed in 
											 // sequNumber for a pile in m_pileList,
	                                         // or by the index into its storage array
	CStrip*		GetStrip(int nSequNum);
	CStrip*		GetStripByIndex(int index); // get the number of visible strips plus an 
											// extra one if a non-integral number of
											// strips fit the window
	int			GetNumVisibleStrips();
	int			CalcNumVisibleStrips();
	int			IndexOf(CPile* pPile); // return the index in m_pileList of the passed 
									   // in pile pointer
	int			GetStripCount(); // return a count of how many strips are 
								 // in the current layout
	bool		GetBoxVisibilityFlag();

	// function calls relevant to laying out the view updated after user's doc-editing operation
	
	// create the list of CPile objects (it's a parallel list to document's m_pSourcePhrases
	// list, and each CPile instance has a member which points to one and only one
	// CSourcePhrase instance in pSrcPhrases)
	CPile*		CreatePile(CSourcePhrase* pSrcPhrase); // create detached, caller will store it
	bool		CreatePiles(SPList* pSrcPhrases); // RecalcLayout() calls this
	void		CreateStrips(int nStripWidth, int gap); // RecalcLayout() calls this
	bool		AdjustForUserEdits(int nStripWidth, int gap); // RecalcLayout() calls this
	bool		RecalcLayout(SPList* pList, enum layout_selector selector);
	void		RelayoutActiveStrip(CPile* pActivePile, int nActiveStripIndex, int gap,
									int nStripWidth); // doesn't change the pile composition,
											// just lays them out, ensuring  proper spacing
	void		DoRecalcLayoutAfterPreferencesDlg();
	void		RecalcPileWidths(PileList* pPiles);
	void		PlaceBox(); // call this after Invalidate() and after Redraw(); contents of 
							// PlacePhraseBoxInLayout() moved into here, and the latter removed
	void		SetupCursorGlobals(wxString& phrase, enum box_cursor state, 
							int nBoxCursorOffset = 0); // BEW added 7Apr09
	bool		GetHighlightedStripsRange(int& nStripCount, bool& bActivePileIsInLast);// BEW 
						// added 3June09, in support of a smarter ScrollIntoView() function
	
    // get the range of visible strips in the viscinity of the active location; pass in the
    // sequNum value, and return indices for the first and last visible strips (the last
    // may be partly or even wholely out lower than then the bottom of the window's client
    // area); we also try to encompass all auto-inserted material within the visible region
	// 
    // new version below, takes as input the pre-scrolled device context (after
    // DoPrepareDC() has been called, and on the basis of the scrollbar thumb's position,
    // gets the first strip which has content visible in the canvas client area, adds the
    // other visible strips, and one more for good measure if possible - this is sort of
    // equivalent to the legacy WX Adapt It's way of doing it, and in the new design the
    // visible strips are worked out according to where the top of the scrolled device
    // context is using the scrollbar thumb's position value.
	void		GetVisibleStripsRange(wxDC* pDC, int& nFirstStrip, int& nLastStrip);
	void		GetVisibleStripsRange(int& nFirstStrip, int& nLastStrip); // overloaded
					// version has the wxClientDC calculations done internally

	// calculate the range of strips marked with m_bValid = FALSE, but don't include the
	// old active location in the range if it is more than nCountValueForTooFar strips
	// away from where the user's edits commenced (latter value is # vis strips, which
	// means that we expect the old active location to be off-window, and no need to
	// update that strip as it won't be drawn)
	void		GetInvalidStripRange(int& nIndexOfFirst, int& nIndexOfLast,
										int nCountValueForTooFar); // uses m_invalidStripArray
	// search for an approximate location vertically in the logical document, where a
	// linear forwards search in the array of strips will locate the strip closest to
	// the document position indicated by the current vertical scroll bar thumb position
	int			GetStartingIndex_ByBinaryChop(int nThumbPos_InPixels, int numVisStrips,
												int numTotalStrips);
	bool		GetPileRangeForUserEdits(int nFirstInvalidStrip, int nLastInvalidStrip, 
											int& nFirstPileIndex, int& nEndPileIndex);
	int			EmptyTheInvalidStrips(int nFirstStrip, int nLastStrip, int nStripWidth);
	int			RebuildTheInvalidStripRange(int nFirstStrip, int nLastStrip, 
										int nStripWidth, int gap, int nFirstPileIndex, 
										int nEndPileIndex, int nInitialStripCount);
	bool		FlowInitialPileUp(int nUpStripIndex, int gap, bool& bDeletedFollowingStrip);
	void		CleanUpTheLayoutFromStripAt(int nIndexOfStripToStartAt, int nHowManyToDo);	

	// redraw the current visible strip range 
	void		Redraw(bool bFirstClear = TRUE);

	// a debugging function
	//void		DebugIndexMismatch(int nPileIndex_InList, int locator);

public:
	DECLARE_DYNAMIC_CLASS(CLayout) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information.
};

#endif
