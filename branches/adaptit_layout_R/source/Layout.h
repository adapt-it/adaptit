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


#ifndef Layout_h
#define Layout_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Layout.h"
#endif

/////////// FRIENDSHIPS in the layout functionality, and their meanings ///////////////////////
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
////////////////////////////////////////////////////////////////////////////////////////////

// forward references
#include "Strip.h"
class CAdapt_ItDoc;
class CSourceBundle;
//class CStrip;
class CPile;
class CFont;
class CAdapt_ItCanvas;

/// The CLayout class manages the layout of the document. It's private members pull
/// together into one place parameters pertinent to dynamically laying out the strips
/// piles and cells of the layout. Setters in various parts of the application set
/// these private members, and getters are used by the layout functionalities to
/// get the drawing done correctly
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

//public:
private:
	//PileList*			m_pPiles;
	PileList			m_pileList;
	//StripList*		m_pStrips;
	wxArrayPtrVoid		m_stripArray;

private:
	// two ints define the range of strips to be drawn at next draw
	int			m_nFirstVisibleStrip;
	int			m_nLastVisibleStrip;

	// four ints define the clip rectange top, left, width & height for erasure 
	// prior to draw (window client coordinates, (0,0) is client top left)
	int			m_nClipRectTop;
	int			m_nClipRectLeft;
	int			m_nClipRectWidth;
	int			m_nClipRectHeight;

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

	//bool		m_bShowTargetOnly; // we won't bother just yet, retain the global

	// the pile height -- this changes in value only when one or more of the font metrics
	// are changed, such as bolding, point size, face, etc. Strip height has more in it.
	// We also have the current leading value for the strips (the nav text whiteboard height),
	// left margin for strips
	int			m_nPileHeight;
	int			m_nStripHeight;
	int			m_nCurLeading;
	int			m_nCurLMargin;
	int			m_nCurGapWidth;
	int			m_nSaveLeading;
	int			m_nSaveGap;


    // client size (width & height as a wxSize) based on Bill's calculation in the CMainFrame, and
    // then as a spin off, the document width (actually m_logicalDocSize.x) and we initialize
    // docSize.y to 0, and set that value later when all the strips are laid out; the setter
    // follows code found in the legacy RecalcLayout() function on the view class
	wxSize		m_sizeClientWindow; // .x is width, .y is height (control bars taken into account)
	wxSize		m_logicalDocSize; // the m_logicalDocSize.x  value is the strip width value to be
	// used for filling a strip with CPile objects; the .y value,
	// plus 40 pixels, is the range to be used for the vertical scroll bar
    // NOTE: *** TODO *** Bill's Canvas class inherits from wxScrollingWindow, which has a virtual
    // function SetVirtualSize() which can be used to define a virtual size different from the
    // client window's width (and height) -- Bill sets it using that function at the end of
    // RecalcLayout(), so the CLayout setup of the strips should end with the same.

	// ////////////////// PRIVATE HELPER FUNCTIONS ////////////////////////
	void		InitializeCLayout();


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

	// create the list of CPile objects (it's a parallel list to document's m_pSourcePhrases
	// list, and each CPile instance has a member which points to one and only one
	// CSourcePhrase instance in pSrcPhrases)
	CPile*		CreatePile(CLayout* pLayout, CSourcePhrase* pSrcPhrase); // create detached, 
																		 // caller will store it
	bool		CreatePiles(SPList* pSrcPhrases);
	bool		RecalcLayout(bool bRecreatePileListAlso = FALSE);



	// Strip destructors
	void		DestroyStrip(int index); // note: doesn't destroy piles and their cells, these 
										 // are managed by m_pPiles list & must persist
	void		DestroyStripRange(int nFirstStrip, int nLastStrip);
	void		DestroyStrips();

	// Pile destructors (for the persistent ones in CLayout::m_pPiles list) - note, 
	// destroying a pile also, in the same function, destroys its array of CCell instances
	void		DestroyPile(CPile* pPile);
	void		DestroyPileRange(int nFirstPile, int nLastPile);
	void		DestroyPiles();


	// getters for clipping rectangle
	wxRect		GetClipRect();

	// setters for clipping rectangle
	void		SetClipRectTop(int nTop);
	void		SetClipRectLeft(int nLeft);
	void		SetClipRectWidth(int nWidth);
	void		SetClipRectHeight(int nHeight);

	// setters and getters for visible strip range
	int			GetFirstVisibleStrip();
	int			GetLastVisibleStrip();
	void		SetFirstVisibleStrip(int nFirstVisibleStrip);
	void		SetLastVisibleStrip(int nLastVisibleStrip);

	// setters and getters for font pointers
	void		SetSrcFont(CAdapt_ItApp* pApp);
	void		SetTgtFont(CAdapt_ItApp* pApp);
	void		SetNavTextFont(CAdapt_ItApp* pApp);
	/* using friends, we only need the setters
	wxFont*		GetSrcFont();
	wxFont*		GetTgtFont();
	wxFont*		GetNavTextFont();
	*/

	// getters and setters for m_nCurLeading and m_nCurGapWidth
	// (these mirror the app's m_curLeading and m_curGapWidth; and the "Saved" ones are for
	// removing the globals gnSaveGap and gnSaveLeading
	// 
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
	//int			GetGapWidth(); // friendliness lets us grab this directly once set

	// setter and getter global bool gbShowTargetOnly, later remove the global
//	void		SetShowTargetOnlyBoolean();
//	bool		GetShowTargetOnlyBoolean();

	// setter and getter for the pile height & strip height; also the current leading value
	void		SetPileAndStripHeight();
	int			GetPileHeight();
	int			GetStripHeight();
	void		SetCurLeading(CAdapt_ItApp* pApp);
	int			GetCurLeading();

	// left margin for strips
	void		SetCurLMargin(CAdapt_ItApp* pApp);
	//int		GetCurLMargin();
	int			GetStripLeft(); // use this instead of GetCurLMargin()

	void		SetClientWindowSizeAndLogicalDocWidth();
	void		SetLogicalDocHeight();	// set m_logicalDocSize.y (call after strips are built)
	wxSize		GetClientWindowSize();
	wxSize		GetLogicalDocSize();

	// updating the m_nStrip index values after insertion or removal of CStrip instance(s) from
	// the layout
	void		UpdateStripIndices(int nStartFrom = 0);


	DECLARE_DYNAMIC_CLASS(CLayout) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information.
};

#endif
