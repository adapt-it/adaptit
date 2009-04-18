// Deprecation List..... get rid of these later....
/*
bool gbSuppressLast
bool gbSuppressFirst
int curRows     in param lists only?
int m_curPileHeight     on the app
int m_nCurPileMinWidth  on the app
gnSaveGap and gnSaveLeading are replaced by m_nSaveGap and m_nSaveLeading in CLayout
grectViewClient	(was used for testing rect intersections for drawing in the legacy app)
m_maxIndex, and the other indices -- use GetCount() of m_pSourcePhrases wherever needed
wxPoint m_ptCurBoxLocation on the app
int m_curDocWidth on the app
m_bSaveAsXML on app, not needed in WX version
gbBundleChanged  defined in CAdapt_ItView.cpp



*/

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Layout.cpp
/// \author			Bruce Waters
/// \date_created	09 February 2009
/// \date_revised	
/// \copyright		2009 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public 
///                 License (see license directory)
/// \description This is the implementation file for the CLayout class. The CLayout class replaces
/// the legacy CSourceBundle class, and it encapsulated as much as possible of the visible layout
/// code for the document. It manages persistent CPile objects for the whole document - one per
/// CSourcePhrase instance. Copies of the CPile pointers are stored in CStrip objects. CStrip,
/// CPile and CCell classes are used in the refactored layout, but with a revised attribute
/// inventory; and wxPoint and wxRect members are computed on the fly as needed using functions,
/// which reduces the layout computations when the user does things by a considerable amount.
/// \derivation The CLayout class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Layout.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument

// Define type safe pointer lists
//#include "wx/listimpl.cpp"
//#include "wx/list.h"
//#include "wx/debug.h" 
//
#include "Adapt_It.h"
#include "Adapt_ItDoc.h"
#include "AdaptitConstants.h"
#include "SourcePhrase.h"
#include "MainFrm.h"
// don't mess with the order of the following includes, Strip must precede View must precede
// Pile must precede Layout and Cell can usefully by last
#include "Strip.h"
#include "Adapt_ItView.h"
#include "Pile.h"
#include "Cell.h"
#include "Adapt_ItCanvas.h"
#include "Layout.h"

// for support of auto-capitalization

/// This global is defined in Adapt_It.cpp.
extern bool	gbAutoCaps;

/// This global is defined in Adapt_It.cpp.
extern bool	gbSourceIsUpperCase;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNonSourceIsUpperCase;

/// This global is defined in Adapt_It.cpp.
extern bool	gbMatchedKB_UCentry;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNoSourceCaseEquivalents;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNoTargetCaseEquivalents;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNoGlossCaseEquivalents;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharNonSrcLC;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharNonSrcUC;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharSrcLC;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharSrcUC;

// globals for support of vertical editing

/// This global is defined in Adapt_It.cpp.
extern EditRecord gEditRecord;

/// This global is defined in Adapt_ItView.cpp.
extern bool gbVerticalEditInProgress;

/// This global is defined in Adapt_ItView.cpp.
extern EditStep gEditStep;

/// This global is defined in Adapt_ItView.cpp.
extern EditRecord gEditRecord;

/// A local pointer to the global gEditRecord defined in Adapt_It.cpp
static EditRecord* pRec = &gEditRecord;

/// This global is defined in Adapt_It.cpp.
extern CPile* gpGreenWedgePile;

/// This global is defined in Adapt_It.cpp.
extern CPile* gpNotePile;

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text
extern bool gbGlossingUsesNavFont;

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbEnableGlossing; // TRUE makes Adapt It revert to Shoebox functionality only

extern bool gbIsPrinting;

/// This global is defined in Adapt_ItView.cpp.
extern int gnBeginInsertionsSequNum;

/// This global is defined in Adapt_ItView.cpp.
extern int gnEndInsertionsSequNum;

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbFindIsCurrent;

/// This global is defined in Adapt_ItView.cpp.
extern bool gbShowTargetOnly;

/// This global is defined in Adapt_ItView.cpp.
extern wxRect grectViewClient;

/// This global is defined in Adapt_ItView.h.
extern int gnBoxCursorOffset;

/// This global is defined in PhraseBox.cpp
extern bool gbExpanding;

// whm NOTE: wxDC::DrawText(const wxString& text, wxCoord x, wxCoord y) does not have an equivalent
// to the nFormat parameter, but wxDC has a SetLayoutDirection(wxLayoutDirection dir) method
// to change the logical direction of the display context. In wxDC the display context is mirrored
// right-to-left when wxLayout_RightToLeft is passed as the parameter;
// While the MFC version changes the alignment and RTL reading direction of DrawText(), it is not
// the same as mirroring (in which MFC would actually call CDC::SetLayout(LAYOUT_RTL) to effect RTL
// mirroring in the display context. In wx, wxDC::DrawText() does not have a parameter that can 
// be used to control Right alignment and/or RTL Reading of text at that level of the DC.
// Certain controls such as wxTextCtrl and wxListBox, etc., also have an undocumented method called
// SetLayoutDirection(wxLayoutDirection dir), where dir is wxLayout_LeftToRight or wxLayout_RightToLeft. 
// Setting the layout to wxLayout_RightToLeft on these controls also involves some mirroring, so that 
// any scrollbar that gets displayed, for example, displays on the left rather than on the right, etc.
// In the wx version we have to be careful about the automatic mirroring features involved in the
// SetLayoutDirection() function, since Adapt It MFC was designed to micromanage the layout direction
// itself in the coding of text, cells, piles, strips, etc.

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // want to access it fast

extern const wxChar* filterMkr;
extern const wxChar* filteredTextPlaceHolder;

// whm NOTES CONCERNING RTL and LTR Rendering in wxWidgets: (BEW moved here from deprecated CText)
//    1. The wxWidgets wxDC::DrawText(const wxString& text, wxCoord x, wxCoord y) function does not 
// have an nFormat parameter like MFC's CDC::DrawText(const CString& str, lPRECT lpRect, UINT nFormat) 
// text-drawing function. The MFC function utilizes the nFormat parameter to control the RTL vs LTR 
// directionality, which apparently only affects the directionality of the display context WITHIN the
// lpRect region of the display context. At present, it seems that the wxWidgets function cannot 
// directly control the directionality of the text using its DrawText() function. In both MFC and 
// wxWidgets there is a way to control the overall layout direction of the elements of a whole diaplay
// context. In MFC it is CDC::SetLayout(DWORD dwLayout); in wxWidgets it is 
// wxDC::SetLayoutDirection(wxLayoutDirection dir). Both of these dc layout functions cause the whole 
// display context to be mirrored so that all elements drawn in the display context are reversed as 
// though seen in a mirror. For a simple application that only displays a single language in its display
// context, perhaps layout mirroring would work OK. However, Adapt It must layout several different
// diverse languages within the same display context, some of which may have different directionality
// and alignment. Therefore, except for possibly some widget controls, MFC's SetLayout() and wxWidgets'
// SetLayoutDirection() would not be good choices. The MFC Adapt It sources NEVER call the mirroring 
// functions. Instead, for writing on a display context, MFC uses the nFormat paramter within 
// DrawText(str,m_enclosingRect,nFormat) to accomplish two things: (1) Render the text as Right-To-Left, 
// and (2) Align the text to the RIGHT within the enclosing rectangle passed as parameter to DrawText().
// The challenge within wxWidgets is to determine how to get the equivalent display of RTL and LTR text.
//    2. The SetLayoutDirection() function within wxWidgets can be applied to certain controls containing
// text such as wxTextCtrl and wxListBox, etc. It is presently an undocumented method with the following
// signature: SetLayoutDirection(wxLayoutDirection dir), where dir is wxLayout_LeftToRight or 
// wxLayout_RightToLeft. It should be noted that setting the layout to wxLayout_RightToLeft on these 
// controls also involves mirroring, so that any scrollbar that gets displayed, for example, displays 
// on the left rather than on the right for RTL, etc.
// CONCLUSIONS:
// Pango in wxGTK, ATSIU in wxMac and Uniscribe in wxMSW seem to do a good job of rendering Right-To-Left 
// Reading text with the correct directionality in a display context without calling the 
// SetLayoutDirection() method. The main thing we have to do is determine where the starting point for 
// the DrawText() operation needs to be located to effect the correct text alignment within the cells 
// (rectangles) of the pile for the given language - the upper left coordinates for LTR text, and the
// upper-right coordinates for RTL text.
// Therefore, in the wx version we have to be careful about the automatic mirroring features involved 
// in the SetLayoutDirection() function, since Adapt It MFC was designed to micromanage the layout 
// direction itself in the coding of text, cells, piles, strips, etc.


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(CLayout, wxObject)


CLayout::CLayout()
{

}

CLayout::~CLayout()
{

}

// call InitializeCLayout when the application has the view, canvas, and document classes
// initialized -- we set up pointers to them here so we want them to exist first --we'll get
// a message (and an assert in debug mode) if we call this too early
void CLayout::InitializeCLayout()
{
	// set the pointer members for the classes the layout has to be able to access on demand
	m_pApp = GetApp();
	m_pView = GetView();
	m_pCanvas = GetCanvas();
	m_pDoc = GetDoc();
	m_pMainFrame = GetMainFrame(m_pApp);
	//m_pPiles = NULL;
	//m_pStrips = NULL;
	m_stripArray.Clear();
	m_bDrawAtActiveLocation = TRUE;
	m_docEditOperationType = invalid_op_enum_value;
	m_bLayoutWithoutVisiblePhraseBox = FALSE;

	// *** TODO ***   add more basic initializations above here - but only stuff that makes the
	// session-persistent m_pLayout pointer on the app class have the basic info it needs,
	// other document-related initializations can be done in SetupLayout()
}

	// for setting or clearing the m_bLayoutWithoutVisiblePhraseBox boolean
void CLayout::SetBoxInvisibleWhenLayoutIsDrawn(bool bMakeInvisible)
{
	m_bLayoutWithoutVisiblePhraseBox = bMakeInvisible;
}

void CLayout::Draw(wxDC* pDC, bool bDrawAtActiveLocation)
{
	// m_bDrawAtActiveLocation is default TRUE; pass explicit FALSE to have drawing done based on the
	// top of the first strip of a visible range of strips determined by the scroll car position
	int i;
	int nFirstStripIndex = -1;
	int nLastStripIndex = -1;
	int nActiveSequNum = -1;

	// *** TODO *** AdjustForUserEdits(), once we get rid of RecalcLayout(FALSE) calls,
	// will need to replace those calls *** in the doc editing handlers themselves, eg
	// OnButtonMerge() etc...***)
	/*
	// make any alterations needed to the strips because of user edit operations on the doc
	AdjustForUserEdits(m_userEditsSpanCheckType); // replaces most of the legacy RecalcLayout() calls
	m_userEditsSpanCheckType = scan_from_doc_ends; // reset to the safer default value for next time
	*/

	if (bDrawAtActiveLocation)
	{
		// work out the range of visible strips based on the phrase box location
		nActiveSequNum = m_pApp->m_nActiveSequNum;
		// determine which strips are to be drawn  (a scrolled wxDC must be passed in)
		GetVisibleStripsRange(pDC, nFirstStripIndex, nLastStripIndex, bDrawAtActiveLocation);

		// draw the visible strips (plus and extra one, if possible)
		for (i = nFirstStripIndex; i <=  nLastStripIndex; i++)
		{
			((CStrip*)m_stripArray[i])->Draw(pDC);
		}

		// get the phrase box placed in the active location and made visible, and suitably
		// prepared - unless it should not be made visible (eg. when updating the layout
		// in the middle of a procedure, before the final update is done at a later time)
		if (!m_bLayoutWithoutVisiblePhraseBox)
		{
			// work out its location and resize (if necessary) and draw it
			PlacePhraseBoxInLayout(m_pApp->m_nActiveSequNum);
		}
	}
	else
	{
		// draw at scroll position
		
		// *** TODO *** the code
		 
		//if (!m_bLayoutWithoutVisiblePhraseBox) // shouldn't be needed for this situation
			
	}
	SetBoxInvisibleWhenLayoutIsDrawn(FALSE); // restore default
}


/* keep for later, if I refactor the scrolling support
void CLayout::Draw(wxDC* pDC, bool bAtActiveLocation)
{
	// bAtActiveLocation is default TRUE; pass explicit FALSE to have drawing done based on the
	// top of the first strip of a visible range of strips determined by the scroll car position
	int i;
	int nFirstStripIndex = -1;
	int nLastStripIndex = -1;
	int nActiveSequNum = -1;

	if (bAtActiveLocation)
	{
		// work out the range of visible strips based on the phrase box location
		nActiveSequNum = m_pApp->m_nActiveSequNum;
		// determine which strips are to be drawn
		GetVisibleStripsRange(nActiveSequNum, nFirstStripIndex, nLastStripIndex);
	}
	else
	{
		// work out the visible strips based on the vertical scroll bar's current car position -
		// use this block when, say, scrolling the view up or down

		// *** TODO ***
	}

	// since a pre-offset device context is passed in (DoPrepareDC() will have already been
	// called), if our draw location is different (eg. the box moved to a pile on next strip) we
	// cannot assume that the device context's origin will be set to match the visible strips
	// above; so we must set the DC origin back to (0,0), and use the Top() value for the first strip
	// to be drawn to work out what the scroll bar should be, set the bar, and then call
	// DoPrepareDC() again to have the right offset done. (This relieves us of the burden of
	// having to call a function like ScrollIntoView().)
	//pDC->SetDeviceOrigin(0,0); // remove the preset origin offset
	//CStrip* pFirstStrip = (CStrip*)m_stripArray[nFirstStripIndex];
	//int nVertScrollPos = pFirstStrip->Top() - GetCurLeading();
	//wxASSERT(nVertScrollPos >= 0);
	// next set the scroll thumb & then call DoPrepareDC() anew... but...
	// *********************************************************************************************
    // I've rethought this and decided that it isn't a reasonable task to try to redesign scrolling
    // at the same time as removing bundles - those need to be separate refactoring jobs; so
    // support scrolling "as is" for the present
    // *********************************************************************************************


	// draw the visible strips (plus and extra one, if possible)
	for (i = nFirstStripIndex; i <=  nLastStripIndex; i++)
	{
		((CStrip*)m_stripArray[i])->Draw(pDC);
	}
}
*/

// the Redraw() member function can be used in many places where, in the legacy application,
// the document is unchanged but the layout needs repainting (eg. a window temporarily covered
// part of the canvas/view); the legacy app just called RecalcLayout() to recreate the bundle, but
// now that bundles are removed, calling CLayout::RecalcLayout(FALSE) is potentially to costly,
// and unnecessary (since the strips piles and cells are okay already); so we just need to redraw
// (we could call Invalidate() on the view, but with Redraw() we potentially have more control)
// bFirstClear is default TRUE; if TRUE it causes aDC to paint the client area with background
// colour (white); assumes the redraw is to be based on an unchanged active location
void CLayout::Redraw(bool bFirstClear)
{
	wxClientDC aDC((wxWindow*)m_pCanvas); // make a device context
	m_pCanvas->DoPrepareDC(aDC); // get origin adjusted (calls wxScrolledWindow::DoPrepareDC)
	wxClientDC* pDC = &aDC;
	pDC->DestroyClippingRegion();
	if (bFirstClear)
	{
		// erase the view rectangle
		pDC->Clear();
	}
	Draw(pDC);  // the CLayout::Draw() which first works out which strips need to be drawn
				// based on the active location (default param bool bAtActiveLocation is TRUE)
}

CAdapt_ItApp* CLayout::GetApp()
{
	CAdapt_ItApp* pApp = &wxGetApp();
	if (pApp == NULL)
	{
		wxMessageBox(_T("Error: failed to get m_pApp pointer in CLayout"),_T(""), wxICON_ERROR);
		wxASSERT(FALSE);
	}
	return pApp;
}

CAdapt_ItView* CLayout::GetView()
{
	CAdapt_ItView* pView = GetApp()->GetView();
	if (pView == NULL)
	{
		wxMessageBox(_T("Error: failed to get m_pView pointer in CLayout"),_T(""), wxICON_ERROR);
		wxASSERT(FALSE);
	}
	return pView;
}

CAdapt_ItCanvas* CLayout::GetCanvas()
{
	CMainFrame* pFrame = GetApp()->GetMainFrame();
	CAdapt_ItCanvas* pCanvas = pFrame->canvas;
	if (pCanvas == NULL)
	{
		wxMessageBox(_T("Error: failed to get m_pCanvas pointer in CLayout"),_T(""), wxICON_ERROR);
		wxASSERT(FALSE);
	}
	return pCanvas;
}

CAdapt_ItDoc* CLayout::GetDoc()
{
	CAdapt_ItDoc* pDoc = GetView()->GetDocument();
	if (pDoc == NULL)
	{
		wxMessageBox(_T("Error: failed to get m_pDoc pointer in CLayout"),_T(""), wxICON_ERROR);
		wxASSERT(FALSE);
	}
	return pDoc;
}

CMainFrame*	CLayout::GetMainFrame(CAdapt_ItApp* pApp)
{
	CMainFrame* pFrame = pApp->GetMainFrame();
	if (pFrame == NULL)
	{
		wxMessageBox(_T("Error: failed to get m_pMainFrame pointer in CLayout"),_T(""), wxICON_ERROR);
		wxASSERT(FALSE);
	}
	return pFrame;
}


// Clipping support 
wxRect CLayout::GetClipRect()
{
	wxRect rect(m_nClipRectLeft,m_nClipRectTop,m_nClipRectWidth,m_nClipRectHeight);
	return rect;
}

void CLayout::SetClipRectTop(int nTop)
{
	m_nClipRectTop = nTop;
}

void CLayout::SetClipRectLeft(int nLeft)
{
	m_nClipRectLeft = nLeft;
}

void CLayout::SetClipRectWidth(int nWidth)
{
	m_nClipRectWidth = nWidth;
}

void CLayout::SetClipRectHeight(int nHeight)
{
	m_nClipRectHeight = nHeight;
}

// accessors for font pointers
void CLayout::SetSrcFont(CAdapt_ItApp* pApp)
{
	m_pSrcFont = pApp->m_pSourceFont;
}

void CLayout::SetTgtFont(CAdapt_ItApp* pApp)
{
	m_pTgtFont = pApp->m_pTargetFont;
}

void CLayout::SetNavTextFont(CAdapt_ItApp* pApp)
{
	m_pNavTextFont = pApp->m_pNavTextFont;
}

/* using friends, we only need the setters
wxFont*	CLayout::GetSrcFont()
{
	return m_pSrcFont;
}
wxFont* CLayout::GetTgtFont()
{
	return m_pTgtFont;
}
wxFont* CLayout::GetNavTextFont()
{
	return m_pNavTextFont;
}
*/


// accessors and getters for src, tgt & navText colours
void CLayout::SetSrcColor(CAdapt_ItApp* pApp)
{
	m_srcColor = pApp->m_sourceColor;
}

void CLayout::SetTgtColor(CAdapt_ItApp* pApp)
{
	m_tgtColor = pApp->m_targetColor;
}

void CLayout::SetNavTextColor(CAdapt_ItApp* pApp)
{
	m_navTextColor = pApp->m_navTextColor;
}

wxColour CLayout::GetSrcColor()
{
	return m_srcColor;
}

wxColour CLayout::GetTgtColor()
{
	return m_tgtColor;
}

wxColour CLayout::GetNavTextColor()
{
	return m_navTextColor;
}
/* use CCell::GetColor(), syntax wxColour* pColor = pCell->GetColor();
wxColour CLayout::GetCurColor()
{
	if (gbIsGlossing && gbGlossingUsesNavFont)
		return m_navTextColor;
	else
		return m_tgtColor;
}
*/
// accessors for src, tgt, navText line heights
void CLayout::SetSrcTextHeight(CAdapt_ItApp* pApp)
{
	m_nSrcHeight = pApp->m_nSrcHeight;
}

void CLayout::SetTgtTextHeight(CAdapt_ItApp* pApp)
{
	m_nTgtHeight = pApp->m_nTgtHeight;
}

void CLayout::SetNavTextHeight(CAdapt_ItApp* pApp)
{
	m_nNavTextHeight = pApp->m_nNavTextHeight;
}

int CLayout::GetSrcTextHeight()
{
	return m_nSrcHeight;
}

int CLayout::GetTgtTextHeight()
{
	return m_nTgtHeight;
}

int	CLayout::GetNavTextHeight()
{
	return m_nNavTextHeight;
}

// setter & getter for m_bShowTargetOnly
/*
void CLayout::SetShowTargetOnlyBoolean()
{
	m_bShowTargetOnly = gbShowTargetOnly;
}
bool CLayout::GetShowTargetOnlyBoolean()
{
	return m_bShowTargetOnly;
}
*/

// leading for strips
void CLayout::SetCurLeading(CAdapt_ItApp* pApp)
{
	m_nCurLeading = pApp->m_curLeading;
}

int CLayout::GetCurLeading()
{
	return m_nCurLeading;
}

// set left margin for strips; for the getter, use GetStripLeft()
void CLayout::SetCurLMargin(CAdapt_ItApp* pApp)
{
	m_nCurLMargin = pApp->m_curLMargin;
}

/*
int CLayout::GetCurLMargin()
{
	return m_nCurLMargin;
}
*/

int CLayout::GetStripLeft()
{
	return m_nCurLMargin;
}

int CLayout::GetSavedLeading()
{
	return m_nCurLeading;
}

int CLayout::GetSavedGapWidth()
{
	return m_nCurGapWidth;
}

void CLayout::SetSavedLeading(int nCurLeading)
{
	m_nCurLeading = nCurLeading;
}

void CLayout::SetSavedGapWidth(int nGapWidth)
{
	m_nCurGapWidth = nGapWidth;
}

// setter and getters for the pile height and strip height
int CLayout::GetPileHeight()
{
	return m_nPileHeight;
}

int CLayout::GetStripHeight()
{
	return m_nStripHeight;
}

void CLayout::SetPileAndStripHeight()
{
	if (gbShowTargetOnly)
	{
		if (gbIsGlossing)
		{
			if (gbGlossingUsesNavFont)
				m_nPileHeight = m_nNavTextHeight;
			else
				m_nPileHeight = m_nTgtHeight;
		}
		else // adapting mode
		{
			m_nPileHeight = m_nTgtHeight;
		}
	}
	else // normal adapting or glossing, at least 2 lines (src & tgt) but even if 3, still those two
	{
		m_nPileHeight = m_nSrcHeight + m_nTgtHeight;

		// we've accounted for source and target lines; now handle possibility of a 3rd line
		// (note, if 3 lines, target is always one, so we've handled that above already)
		if (gbEnableGlossing)
		{
			if (gbGlossingUsesNavFont)
			{
				// we are showing only the source and target lines
				m_nPileHeight += m_nNavTextHeight;
			}
			else // glossing uses tgtTextFont
			{
				m_nPileHeight += m_nTgtHeight;
			}
			// add 2 more pixels because we will space the glossing line off from
			// whatever line it follows by an extra 3 pixels, in CreatePile( ) - since
			// with many fonts it appears to encroach on the bottom of the line above
			// (MFC app, we used 3, for wxWidgets try 2)
			m_nPileHeight += 2; 
		}
	} // end of else block
    // the pile height is now set; so in the stuff below, set the strip height too - it is the
    // m_nPileHeight value, +/-, depending on whether free translation mode is on or not, the
    // target text line height plus 3 pixels of separating space from the bottom of the pile
    // (Note: the m_nCurLeading value, for the navText area, is NOT regarded as part of the strip)
	m_nStripHeight = m_nPileHeight;
	if (m_pApp->m_bFreeTranslationMode && !gbIsPrinting)
	{
        // add enough space for a single line of the height given by the target text's height + 3
        // pixels to set it off a bit from the bottom of the pile
		m_nStripHeight += m_nTgtHeight + 3;
	}
}

PileList* CLayout::GetPileList()
{
	return &m_pileList;
}

wxArrayPtrVoid* CLayout::GetStripArray()
{
	return & m_stripArray;
}

// Call SetClientWindowSizeAndLogicalDocWidth() before just before strips are laid out; then call
// SetLogicalDocHeight() after laying out the strips, to get private member m_logicalDocSize.y set 
void CLayout::SetClientWindowSizeAndLogicalDocWidth()
{
	wxSize viewSize;
	viewSize = m_pMainFrame->GetCanvasClientSize(); // dimensions of client window of wxScrollingWindow
													// which canvas class is a subclass of
	m_sizeClientWindow = viewSize; // set the private member
	wxSize docSize;
	docSize.y = 0; // can't be set yet, we call this setter before strips are laid out
	docSize.x = m_sizeClientWindow.x - m_nCurLMargin - RH_SLOP; // RH_SLOP defined in AdaptItConstant.h
					// with a value of 40 (reduces likelihood of long nav text above a narrow pile
					// which is last in a strip, having the end of the nav text drawn off-window
	m_logicalDocSize = docSize;
}

// sets m_logicalDocSize.y value to the logical height (in pixels) of the laid out strips, or to
// zero if there are no strips in m_stripArray yet; however this function should never be called
// when the latter is the case - that would be a serious design error
void CLayout::SetLogicalDocHeight()
{
	int nDocHeight = 0;
	if (!m_stripArray.IsEmpty())
	{
		int nStripCount = m_stripArray.GetCount();
		int nDocHeight = (GetCurLeading() + GetStripHeight()) * nStripCount;
		nDocHeight += 40; // pixels for some white space at document's bottom
	}
	m_logicalDocSize.SetHeight(nDocHeight);
}

wxSize CLayout::GetClientWindowSize()
{
	return m_sizeClientWindow;
}

wxSize CLayout::GetLogicalDocSize()
{
	return m_logicalDocSize;
}

	// current gap width between piles (in pixels)
void CLayout::SetGapWidth(CAdapt_ItApp* pApp)
{
	m_nCurGapWidth = pApp->m_curGapWidth; // user sets it in Preferences' View tab
}
// CLayout, CStrip, CPile & CCell are mutual friends, so they can grab m_nCurGapWidth
// directly, but outsiders will need the followinng
int CLayout::GetGapWidth()
{
	return m_nCurGapWidth;
}


// SetLayoutParameters() is where we do most of the hooking up to the current state of the app's
// various view-related parameters, such as fonts, colours, text heights, and so forth
void CLayout::SetLayoutParameters()
{
	InitializeCLayout(); // sets the app, doc, view, canvas & frame pointers
	m_pApp->UpdateTextHeights(m_pView);
	SetSrcFont(m_pApp);
	SetTgtFont(m_pApp);
	SetNavTextFont(m_pApp);
	SetSrcColor(m_pApp);
	SetTgtColor(m_pApp);
	SetNavTextColor(m_pApp);
	SetSrcTextHeight(m_pApp);
	SetTgtTextHeight(m_pApp);
	SetNavTextHeight(m_pApp);
	SetGapWidth(m_pApp);
	SetPileAndStripHeight();
	SetCurLeading(m_pApp);
	SetCurLMargin(m_pApp);
	SetClientWindowSizeAndLogicalDocWidth();
}

void CLayout::DestroyStrip(int index)
{
	//CStrip* pStrip = (CStrip*)(*m_pStrips)[index];
	CStrip* pStrip = (CStrip*)m_stripArray[index];
	//WX_CLEAR_ARRAY(pStrip->m_arrPiles); << no, macro tries to use delete operator, use Clear()
	pStrip->m_arrPiles.Clear();
	pStrip->m_arrPileOffsets.Clear();
	// don't try to delete CCell array, because the cell objects are managed by the persistent
	// pile pointers in the CLayout array m_pPiles, and the strip does not own these
	delete pStrip;
}

void CLayout::DestroyStripRange(int nFirstStrip, int nLastStrip)
{
	int index;
	for (index = nFirstStrip; index <= nLastStrip; index++)
		DestroyStrip(index);
}

void CLayout::DestroyStrips()
{
	int nLastStrip = m_stripArray.GetCount() - 1;
	DestroyStripRange(0, nLastStrip);
}

void CLayout::DestroyPile(CPile* pPile)
{
	int index;
	for (index = 0; index < MAX_CELLS; index++)
		delete pPile->m_pCell[index];
	delete pPile;
}

void CLayout::DestroyPileRange(int nFirstPile, int nLastPile)
{
	PileList::Node* pos = m_pileList.Item(nFirstPile);
	int index = nFirstPile;
	if (pos == NULL)
	{
		wxMessageBox(_T("nFirstPile index did not return a valid iterator in DestroyPileRange()"),
						_T(""), wxICON_STOP);
		wxASSERT(FALSE);
		wxExit();
	}
	CPile* pPile = 0;
	while (index <= nLastPile)
	{
		pPile = pos->GetData();
		wxASSERT(pPile != NULL);
		DestroyPile(pPile);
		index++;
		pos = pos->GetNext();
	}
}

void CLayout::DestroyPiles()
{
	int nLastPile = m_pileList.GetCount() - 1;
	DestroyPileRange(0, nLastPile);
}

// Note: never call CreatePile() if there is not yet a valid pSrcPhrase pointer to pass in;
// CreatePile() creates the CPile instance on the heap, and returns a pointer to it
CPile* CLayout::CreatePile(CSourcePhrase* pSrcPhrase)
{
	wxASSERT(pSrcPhrase != NULL);

	// create an empty pile on the heap
	CPile* pPile = new CPile;
	wxASSERT(pPile != NULL); // if tripped, must be a memory error

	// assign the passed in source phrase pointer to its m_pSrcPhrase public member,
	// and also the pointer to the CLayout object
	pPile->m_pSrcPhrase = pSrcPhrase;
	pPile->m_pLayout = this;

	// set (in pixels) its minimum width based on which of the m_srcPhrase, m_adaption and m_gloss
	// strings within the passed in CSourcePhrase is the widest; the min width member is thereafter
	// a lower bound for the width of the phrase box when the latter contracts, if located at this
	// pile while the user is working
	pPile->m_nMinWidth = pPile->CalcPileWidth(); // if an empty string, it is defaulted to 10 pixels
	if (pSrcPhrase->m_nSequNumber != m_pApp->m_nActiveSequNum)
	{
		pPile->m_nWidth = PHRASE_BOX_WIDTH_UNSET; // a default -1 value for starters, need an active sequ
				// number and m_targetPhrase set before a value can be calculated, but only at the pile
				// which is located at the active location
	}
	else
	{
		// this pile is going to be the active one, so calculate its m_nWidth value using m_targetPhrase
		pPile->SetPhraseBoxGapWidth(); // calculates, and sets value in m_nWidth
	}

	// pile creation always creates the (fixed) array of CCells which it manages
	int index;
	CCell* pCell = NULL;
	for (index = 0; index < MAX_CELLS; index++)
	{
		pCell = new CCell();
		pCell->CreateCell(this,pPile,index);
	}
	// Note: to assist clarity, the extensive commented out legacy code has been removed
	// from here and placed temporarily at the end of this file; much of it no longer applies
	return pPile;
}

// return TRUE if no error, FALSE if it failed to do the creation properly for any reason
bool CLayout::CreatePiles(SPList* pSrcPhrases)
{
	// we expect pSrcPhrases list is populated, so if it is not we
	// treat that as a serious logic error
	if (pSrcPhrases == NULL || pSrcPhrases->IsEmpty())
	{
		wxMessageBox(_T("SPList* passed in was either NULL or devoid of pilesl in CreatePiles()"),
						_T(""), wxICON_STOP);
		wxASSERT(FALSE);
		wxExit(); // something seriously wrong in design, so don't try to go on
		return FALSE;
	}

	// we allow CreatePiles() to be called even when the list of piles is still populated, so
	// CreatePiles() has the job of seeing that the old ones are deleted before creating a new set
	//bool bIsEmpty = m_pPiles->IsEmpty();
	bool bIsEmpty = m_pileList.IsEmpty();
	if (!bIsEmpty)
	{
		// delete the old ones
		DestroyPiles();
	}
	wxASSERT(m_pileList.IsEmpty());

	CSourcePhrase* pSrcPhrase = NULL;
	CPile* pPile = NULL;
	SPList::Node* pos = pSrcPhrases->GetFirst();
	PileList::Node* posInPilesList = m_pileList.GetFirst();
	// Note: lack of memory could make the following loop fail in the release version if the
	// user is processing a very large document - but recent computers should have plenty of
	// RAM available, so we'll assume a low memory error will not arise; so keep the code lean
	while (pos != NULL)
	{
		// get the CSourcePhrase pointer from the passed in list
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		wxASSERT(pSrcPhrase != NULL);

		// create the CPile instance
		//pPile = CreatePile(this,pSrcPhrase);
		pPile = CreatePile(pSrcPhrase);
		wxASSERT(pPile != NULL);

		// store it in the node of the m_pPiles list of CLayout
		posInPilesList->SetData(pPile);

		// get ready for next iteration
		pos = pos->GetNext();
		posInPilesList = posInPilesList->GetNext();
	}

    // To succeed, the count of items in each list must be identical 
	if (pSrcPhrases->GetCount() != m_pileList.GetCount())
	{
		wxMessageBox(_T("SPList* passed in, in CreatePiles(), has a count which is different from that of the m_pPiles list in CLayout"),
						_T(""), wxICON_STOP);
		wxASSERT(FALSE);
		wxExit();	// something seriously wrong in design probably, such an error should not 
					// happen, so don't try to go on
		return FALSE;
	}
	return TRUE;
}

// return TRUE if a layout was set up, or if no layout can yet be set up;
// but return FALSE if a layout setup was attempted and failed (app must then abort)
// 
// Note: RecalcLayout() should be called, for best results, immediately before opening, or
// creating, an Adapt It document. At that point all of the paramaters it collects together
// should have their "final" values, such as font choices, colours, etc. It is also to be
// called whenever the layout has been corrupted by a user action, such as a font change
// (which clobbers prior text extent values stored in the piles) or a punctuation setting
// change, etc. RecalcLayout() in the refactored design is a potentially "costly"
// calculation - if the document is large, the piles and strips for the whole document has
// to be recalculated from the data in the current document - (need to consider a progress
// bar in the status bar in window bottom) The default value of the passed in flag
// bRecreatePileListAlso is FALSE The ptr value for the passed in pList is usually the
// application's m_pSourcePhrases list, but it can be a sublist copied from that
bool CLayout::RecalcLayout(SPList* pList, bool bRecreatePileListAlso)
{
    // RecalcLayout() is the refactored equivalent to the former view class's RecalcLayout()
    // function - the latter built only a bundle's-worth of strips, but the new design must build
	// strips for the whole document - so potentially may consume a lot of time; however, the
	// efficiency of the new design (eg. no rectangles are calculated) may compensate significantly

	SPList* pSrcPhrases = pList; // the list of CSourcePhrase instances which comprise the
	// document, or a sublist copied from it, -- the CLayout instance will own a parallel list of
    // CPile instances in one-to-one correspondence with the CSourcePhrase instances, and
    // each of piles will contain a pointer to the sourcePhrase it is associated with
	
	if (pSrcPhrases == NULL || pSrcPhrases->IsEmpty())
	{
		// no document is loaded, so no layout is appropriate yet, do nothing
		// except ensure m_pPiles is NULL
		if (!(m_pileList.IsEmpty()))
		{
			m_pileList.DeleteContents(TRUE); // TRUE means "delete the stored CCell instances too"
		}
		// a message to us developers is needed here, in case we get the design wrong
		wxMessageBox(_T("Warning: SetupLayout() did nothing because there are no CSourcePhrases yet."),
					_T(""), wxICON_WARNING);
		return TRUE;
	}

	// preserve selection parameters, so it can be preserved across the recalculation
	m_pView->StoreSelection(m_pApp->m_selectionLine);

	// //////// first get up-to-date-values for all the needed data /////////
	/* do it instead in caller using SetLayoutParameters() call
	// set the latest wxFont pointers...
	SetSrcFont(m_pApp);
	SetTgtFont(m_pApp);
	SetNavTextFont(m_pApp);

	// set the local private copie of the colours
	SetNavTextColor(m_pApp);
	SetTgtColor(m_pApp);
	SetSrcColor(m_pApp);

	// set the text heights for src, tgt, navText private members
	SetSrcTextHeight(m_pApp);
	SetTgtTextHeight(m_pApp);
	SetNavTextHeight(m_pApp);

	// set left margin and vertical leading for strips, also pile and strip heights,
	// and client window's dimensions - and hence the logical doc width (in pixels)
	SetCurLMargin(m_pApp);
	SetCurLeading(m_pApp);
	SetPileAndStripHeight();
	SetClientWindowSizeAndLogicalDocWidth(); // height gets set after strips are laid out
	SetGapWidth(m_pApp); // gap (in pixels) between piles when laid out in strips, m_nCurGapWidth
	*/
	// *** ?? TODO ?? **** more parameter setup stuff goes here, if needed

	// attempt the (re)creation of the m_pileList list of CPile instances if requested; if not
	// requested then the current m_pileList's contents are valid still and will be used unchanged
	if (bRecreatePileListAlso)
	{
		bool bIsOK = CreatePiles(pSrcPhrases);
		if (!bIsOK)
		{
			// something was wrong - memory error or perhaps m_pPiles is a populated list already
			// (CreatePiles()has generated an error message for the developer already)
			return FALSE;
		}
	}

    // estimate the number of CStrip instances required - assume an average of 16 piles per strip,
    // which should result in more strips than needed, and so when the layout is built, we should
    // call Shrink()
	int aCount = pSrcPhrases->GetCount();
	int anEstimate = aCount / 16;
	m_stripArray.SetCount(anEstimate,(void*)NULL);	// create possibly enough space in the array 
													// for all the piles
	int gap = m_nCurGapWidth; // distance in pixels for interpile gap
	int nStripWidth = (GetLogicalDocSize()).x; // constant for any one RecalcLayout call
	//int nPilesEndIndex = m_pileList.GetCount() - 1; // use this to terminate the strip creation loop
	int nIndexOfFirstPile = 0;
	wxASSERT(!m_pileList.IsEmpty());
	PileList::Node* pos = m_pileList.Item(nIndexOfFirstPile);
	wxASSERT(pos != NULL);
	CStrip* pStrip = NULL;

	// before building the strips, we want to ensure that the gap left for the phrase box
	// to be drawn in the layout is as wide as the phrase box is going to be when it is
	// made visible by CLayout::Draw(). When RecalcLayout() is called, gbExpanding global
	// bool may be TRUE, or FALSE (it's set by FixBox() and cleared in FixBox() at the end
	// after layout adjustments are done - including a potential call to RecalcLayout(),
	// it's also cleared to default as a safety first measure at start of each OnChar()
	// call. If we destroyed and recreated the piles in the block above, CreatePile()
	// will, at the active location, made use of the  gbExpanding value and set the "hole"
	// for the phrase box to be the appropriate width. But if piles were not destroyed and
	// recreated then the box may be about to be drawn expanded, and so we must make sure
	// that the strip rebuilding about to be done below has the right box width value to
	// be used for the pile at the active location. The safest way to ensure this is the
	// case is to make a call to doc class's ResetPartnerPileWidth(), passing in the
	// CSourcePhrase pointer at the active location - this call internally calls
	// CPile:CalcPhraseBoxGapWidth() to set CPile's m_nWidth value to the right width, and
	// then the strip layout code below can use that value via a call to
	// GetPhraseBoxGapWidth() to make the active pile's width have the right value.
	if (!bRecreatePileListAlso)
	{
		// these four lines ensure that the active pile's width is based on the
		// CLayout::m_curBoxWidth value that was stored there by any adjustment to the box
		// width done by FixBox() just prior to this RecalcLayout() call ( similar code
		// will be needed in AdjustForUserEdits() when we complete the layout refactoring,
		// *** TODO *** )
		CPile* pActivePile = GetPile(m_pApp->m_nActiveSequNum);
		wxASSERT(pActivePile);
		CSourcePhrase* pSrcPhrase = pActivePile->GetSrcPhrase();
		m_pDoc->ResetPartnerPileWidth(pSrcPhrase);
	}

	// the loop which builds the strips
	while (pos != NULL)
	{
		pStrip = new CStrip;
		pos = pStrip->CreateStrip(pos, nStripWidth, gap);
	}
	m_stripArray.Shrink();

	// the height of the document can now be calculated
	SetLogicalDocHeight();

	gbExpanding = FALSE; // has to be restored to default value

	// restore the selection, if there was one
	m_pView->RestoreSelection();


	// *** TODO 1 ***
	// set up the scroll bar to have the correct range (and it would be nice to try place the
	// phrase box at the old active location if it is still within the document, etc) -- see the
	// list of things done in the legacy CAdapt_ItView's version of this function (lines 4470++)
	

	// *** TODO 2 *** free translation support - in the legacy code this just involved creating and
	// storing the free translation rectangle in the m_rectFreeTrans member of the CStrip
	// instance, but for the refactored design we will need to create this rectangle on the fly
	// and from a function member, and other changes will be involved no doubt, because in the new
	// design switching to or from free translation mode will not have RecalcLayout() called
	
	return TRUE;
}

// starting from the passed in index value, update the index of succeeding strip instances to be
// in numerically ascending order without gaps
void CLayout::UpdateStripIndices(int nStartFrom)
{
	int nCount = m_stripArray.GetCount();
	int index = -1;
	int newIndexValue = nStartFrom;
	CStrip* pStrip = NULL;
	if (nCount > 0)
	{
		for (index = nStartFrom; index < nCount; index++)
		{
			pStrip = (CStrip*)m_stripArray[index];
			pStrip->m_nStrip = newIndexValue;
			newIndexValue++;
		}
	}
}

int	CLayout::IndexOf(CPile* pPile)
{
	return m_pileList.IndexOf(pPile);
}


// the GetPile function also has equivalent member functions of the same name in the CAdapt_ItView
// and CAdapt_ItDoc classes, for convenience's sake; return the CPile instance at the
// given index, or NULL if the index is out of bounds
CPile* CLayout::GetPile(int index)
{
	int nCount = m_pileList.GetCount();
	if (index < 0 || index >= nCount)
	{
		// bounds error, so return NULL
		return (CPile*)NULL;
	}
	PileList::Node* pos = m_pileList.Item(index);
	wxASSERT(pos != NULL);
	return pos->GetData();
}

int CLayout::GetStripIndex(int nSequNum)
{
	PileList::Node* pos = m_pileList.Item(nSequNum); // relies on parallelism of m_pSourcePhrases 
												  // and m_pileList lists
	wxASSERT(pos != NULL);
	CPile* pPile = pos->GetData();
	return pPile->m_pOwningStrip->m_nStrip;
}

CStrip* CLayout::GetStrip(int nSequNum)
{
	PileList::Node* pos = m_pileList.Item(nSequNum); // relies on parallelism of m_pSourcePhrases 
												  // and m_pileList lists
	wxASSERT(pos != NULL);
	CPile* pPile = pos->GetData();
	return pPile->m_pOwningStrip;
}

int CLayout::GetVisibleStrips()
{
	int clientHeight;
	wxSize canvasSize;
	canvasSize = m_pApp->GetMainFrame()->GetCanvasClientSize();
	clientHeight = canvasSize.GetHeight(); // see lines 2425-2435 of Adapt_ItCanvas.cpp
										   // and then lines 2454-56 for stuff below
	int nVisStrips = clientHeight / m_nStripHeight;
	int partStrip = clientHeight % m_nStripHeight; // modulo
	if (partStrip > 0)
		nVisStrips++;
	return nVisStrips;
}

void CLayout::GetVisibleStripsRange(wxDC* pDC, int& nFirstStripIndex, int& nLastStripIndex, int bDrawAtActiveLocation)
{
	// *** TODO ****  BE SURE TO HANDLE active sequ num of -1 --> make it end of doc, but
	// hide box
	if (bDrawAtActiveLocation)
	{
		// get the logical distance (pixels) that the scroll bar's thumb indicates to top of client area
		int nThumbPosition_InPixels = pDC->DeviceToLogicalY(0);

		// for the current client rectangle of the canvas, calculate how many strips will fit - a part
		// strip is counted as an extra one
		int nVisStrips = GetVisibleStrips();

		// initialilze the values for the return parameters
		nFirstStripIndex = -1;
		nLastStripIndex = -1;

		// find the current total number of strips
		int nTotalStrips = m_stripArray.GetCount();
		
		// find the index of the first strip which has some content visible in the client area,
		// that is, the first strip which has a bottom coordinate greater than nThumbPosition_InPixels
		int index = 0;
		int bottom;
		CStrip* pStrip;
		do {
			pStrip = (CStrip*)m_stripArray[index];	
			bottom = pStrip->Top() + GetStripHeight(); // includes free trans height if free trans mode is ON 
			if (bottom > nThumbPosition_InPixels)
			{
				// this strip is at least partly visible - so start drawing at this one
				break;
			}
			index++;
		} while(index < nTotalStrips);
		wxASSERT(index < nTotalStrips);
		nFirstStripIndex = index;

		// use nVisStrips to get the final visible strip (it may be off-window, but we don't care
		// because it will be safe to draw it off window)
		nLastStripIndex = nFirstStripIndex + (nVisStrips - 1);

		// check the bottom of the last visible strip is lower than the bottom of the client area, if
		// not, add an additional strip
		pStrip = (CStrip*)m_stripArray[nLastStripIndex];
		bottom = pStrip->Top() + GetStripHeight();
		if (!(bottom >= nThumbPosition_InPixels + GetClientWindowSize().y ))
		{
			// add an extra one
			nLastStripIndex++;
		}
	}
	else
	{
		// **** TODO ****  similar calculations based on scroll bar thumb position

	}
}

// return TRUE if the function checked for the start and end of the user edit span,
// return FALSE is no check was done (ie. no_scan_needed) value passed in
bool CLayout::AdjustForUserEdits(enum update_span type)
{
    // scan forwards and backwards in m_pileList matching CSourcePhrase pointer instances with
    // those stored in the CPile copies in the strips in m_stripArray - and when there is a
    // mismatch in the pointer values, we will have found a boundary for changes to the CPile
    // pointers in m_pileList due to the user's editing changes to the document. Use these
    // beginning and ending locations for updating the piles in the strips between those locations
    // just prior to drawing the updated layout The passed in enum value, which can be
    // scan_from_doc_ends (= 0), or scan_in_active_area_proximity (= 1), or scan_from_big_jump,
    // determines how extensive a scan is done - whether from the start and end of the document, or
    // from locations prior to and after the active location but in its proximity; in the later
    // case the jump distance from the current active location is given by the #define
    // nJumpDistanceForUserEditsSpanDetermination which is currently set at 80, or from a bigger
    // jump nBigJumpDistance, currently set at 300 (CSourcePhrase instances)
	
	// Note: the pileList stripArray discrepancy can also be used for working out the clip
	// rectangle. First get the editing span located in each, and then perhaps get an
	// array of pointers to the first pile in each of the old strips at that location
	// (until strip is a few off screen), and then compare, after new layout is done, the
	// pointers to first pile in each of the visible strips, and if no scrollintoview was
	// done, then where the first pile pointers don't match, that strip area has to be in
	// the clip rectangle; but where they match, that can be out of the clip rectangle.

	// **** TODO **** ... the locating and stripArray tweaking...  -- not sure yet where
	// to put the PrepareForLayout() call - whether in this function or this function in that?
	switch (type)
	{
	case no_scan_needed:
		{
			m_nStartUserEdits = -1; // set to -1 when value is not to be used
			m_nEndUserEdits = -1;   // ditto

		 return FALSE;
		}
	case scan_in_active_area_proximity:
		{

			break;
		}
	case scan_from_big_jump:
		{

			break;
		}
	case scan_from_doc_ends:
		{

			break;
		}
	}
	return TRUE;
}

/*
//retain this -- I coded it assuming I'd be refactoring the scrolling mechanism at same time as
//removing the bundles, but it got too complex to do both jobs at once. Instead, for now, work out
//the first and last strip indices using the passed in wxDC pre-scrolled location, and support
//ScrollIntoView()
void CLayout::GetVisibleStripsRange(int nSequNum, int& nFirstStrip, int& nLastStrip)
{
	int nVisStrips = GetVisibleStrips();
	int nActiveStrip = GetStripIndex(nSequNum);
	bool bHasAutoInsertRange = FALSE;
	int nCountAutoInsertionStrips = 0;
	int nBeginStripIndex = -1;
	int nEndStripIndex = -1;
	if (gnBeginInsertionsSequNum != -1)
	{
		// a range of auto insertions is currently in effect
		bHasAutoInsertRange = TRUE;
		nBeginStripIndex = GetStripIndex(gnBeginInsertionsSequNum);
		nEndStripIndex = GetStripIndex(gnEndInsertionsSequNum);
		nCountAutoInsertionStrips = nEndStripIndex - nBeginStripIndex + 1;
	}

	// establish values, assuming no auto insertions are pertinent, then
	// later on adjust values if necessary
	int nTotalStrips = m_stripArray.GetCount();
	bool bAddedExtraOne = FALSE;
	int nHalf = nVisStrips / 2; // keep the active strip about mid-window if possible
	// if too close to the document start, start from the document start
	if (nActiveStrip + 1 < nHalf)
	{
		nFirstStrip = 0;
		nLastStrip = nVisStrips - 1;
		if (nLastStrip < nTotalStrips - 1)
		{
			nLastStrip++; // add an extra strip at the bottom, to play safe, provided it fits
			bAddedExtraOne = TRUE;
		}
	}
	else
	{
		// if too close to the document end, adjust to show the document end
		if (nActiveStrip + (nVisStrips - nHalf) > nTotalStrips)
		{
			nLastStrip = nTotalStrips - 1;
			nFirstStrip = nTotalStrips - nVisStrips;
			if (nFirstStrip < 0)
			{
				// document has fewer strips than can fill the client rectangle, so
				// set the first one to be the first one in the document
				nFirstStrip = 0;
			}
			else
			{
				// add an extra strip, provided it fits, so as to play safe
				if (nFirstStrip > 0)
				{
					nFirstStrip--;
					bAddedExtraOne = TRUE;
				}
			}
		}
		else
		{
			// not close to either end of the document
			nFirstStrip = nActiveStrip + 1 - nHalf;
			nLastStrip = nFirstStrip + nVisStrips - 1;
			// if possible, add an extra strip at the end so as to play safe
			if (nLastStrip < nTotalStrips - 1)
			{
				nLastStrip++; // this one most likely will be off-window
				bAddedExtraOne = TRUE;
			}
		}
	}

	// do adjustments to accomodate auto-insertions as much as possible
	if (bHasAutoInsertRange)
	{
		// auto insertions exist
		if (nCountAutoInsertionStrips > nVisStrips)
		{
			// too many auto insertions for us to display all their strips in the visible
			// rectangle, so we'll display the ones at the end, and the user can scroll back up
			// manually if he needs to see the rest
			int numToMove = bAddedExtraOne ? nLastStrip - 1 - nActiveStrip : nLastStrip - nActiveStrip;
			int numMovesPossible = nFirstStrip;
			if (numMovesPossible >= numToMove)
			{
				// we can do the needed move up
				nFirstStrip -= numToMove;
				nLastStrip -= numToMove;
			}
			else
			{
				// do as much as we can
				nFirstStrip -= numMovesPossible;
				if (nTotalStrips > nVisStrips)
				{
					// don't change the index of the last visible strip to be less if the total
					// strips in the document is less than the number which would fit in the
					// visible rectangle; but when the total is greater, then do so
					nLastStrip -= numMovesPossible;
				}
			}
		}
		else
		{
			// all of them will fit in the visible rectangle; if some overlap the start,
			// then make the start of the visible strips be earlier
			if (nBeginStripIndex < nFirstStrip)
			{
				// all the autoinserted material is not all in the visible area already
				// so an adjustment is required, if possible, to display the active strip 
				// lower down
				int nFreeStrips = nVisStrips - nCountAutoInsertionStrips;
				int nOverlapped = nFirstStrip - nBeginStripIndex;
				if (nFreeStrips >= nOverlapped)
				{
					// they can be fitted in the visible area - make the adjustment
					nFirstStrip -= nOverlapped;
					nLastStrip -= nOverlapped;
				}
				else
				{
					// do the best we can (probably this block won't ever be entered)
					nFirstStrip -= nFreeStrips;
					nLastStrip -= nFreeStrips;
				}
			}
			else
			{
				// all visible, nothing to be one
				;
			}
		}
	}
}
*/
/* 
	//legacy code forCreatePile() (from wxWidgets code base 4.1.1 as tweaked by BEW before 
	// starting refactor job (chuck later on)

	// create the pile on the heap, initializing its cells to MAX_CELLS nulls
	//CPile* pPile = new CPile(pDoc,pBundle,pStrip,pSrcPhrase); // BEW deprecated 3Feb09
	//CPile* pPile = new CPile(pBundle,pStrip,pSrcPhrase); // BEW moved to Pile.cpp
	//wxASSERT(pPile != NULL);
	CPile* pPile = this; // this relieves me of the burden of removing the pPile-> code below
	//CAdapt_ItView* pView = pApp->GetView();

	// now set the attributes not already set by the constructor's defaults
	pPile->m_rectPile = *pRectPile;
	pPile->m_nWidth = pRectPile->GetRight() - pRectPile->GetLeft();

	// the m_nMinWidth value needs to be set to the initial text-extent-based size of the editBox,
	// so that if user removes text from the box it will contract back to this minimum size; the
	// needed value will have been placed in the app's m_nCurPileMinWidth attr by the preceding
	// GetPileWidth() call in the loop within the CreateStrip() function.
	pPile->m_nMinWidth = pApp->m_nCurPileMinWidth;

	// now we have to create the cells which this particular pile owns; this will depend on
	// MAX_CELLS ( == 5 in version 2.0 and onwards), whether or not glossing is allowed, & the user's
	// choice whether or not to suppress display of either 1st or last row (if 4 rows available);
	// and whether or not to show or hide punctuation in lines 2 & 3 (see m_bHidePunctuation flag),
	// and whether or not only the target line is to be shown. When glossing is allowed, the order
	// of the cells from top to bottom will be: if adapting, src, src, tgt, tgt, gloss; if glossing
	// src, src, gloss, tgt, tgt.
	wxPoint  topLeft;
	wxPoint  botRight;
	wxFont*  pSrcFont = pApp->m_pSourceFont;
	wxFont*  pTgtFont = pApp->m_pTargetFont;
	wxFont*  pNavTextFont = pApp->m_pNavTextFont;
	wxSize   extent;
	wxColour color; // MFC had COLORREF color;
	//m_navTextColor = pApp->m_navTextColor; // do it in class creator

	int cellIndex = -1;
	cellIndex++; // set cellIndex to zero

	// set the required color for the text, but if target only, it has only target's color
	// (which can be navText's color if we are glossing - we'll set the target text's color
	// further down in the block.)
	if (!gbShowTargetOnly)
	{
		if (pSrcPhrase->m_bSpecialText)
			color = pApp->m_specialTextColor;
		else
			color = pApp->m_sourceColor;
		if (pSrcPhrase->m_bRetranslation)
			color = pApp->m_reTranslnTextColor;
	}

	// first two src lines are same whether glossing is enabled or not
	if (pApp->m_bSuppressFirst)
	{
		// the first source line is to be suppressed
		cellIndex++; // == 1, so this will be cell[1] not cell[0]

		// suppress the cell if we want to only show target text line; otherwise
		// for gbShowTargetOnly == FALSE we want to show the source line too
		if (!gbShowTargetOnly)
		{
			// calculate the cell's topLeft and botRight points (CText calculates the CRect)
			topLeft = wxPoint(pRectPile->GetLeft(),pRectPile->GetTop());
			botRight = wxPoint(pRectPile->GetRight(), pRectPile->GetBottom());
			// whm: the wx version doesn't use negative offsets
			botRight.y = pRectPile->GetTop() + pApp->m_nSrcHeight;

			// calculate the extent of the text
			int keyWidth;
			int keyDummyHeight;
			pDC->SetFont(*pSrcFont);
			if (pApp->m_bHidePunctuation)
			{
				pDC->GetTextExtent(pSrcPhrase->m_key, &keyWidth, &keyDummyHeight);

				// create the CCell and set its attributes
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//		pSrcPhrase->m_key,keyWidth,pSrcFont,&color,&topLeft,&botRight,
				//		cellIndex); // BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_key,
						keyWidth,pSrcFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
			}
			else
			{
				pDC->GetTextExtent(pSrcPhrase->m_srcPhrase, &keyWidth, &keyDummyHeight);

				// create the CCell and set its attributes
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//		pSrcPhrase->m_srcPhrase,keyWidth,pSrcFont,&color,&topLeft,
				//		&botRight,cellIndex); // BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,
						&pSrcPhrase->m_srcPhrase,keyWidth,pSrcFont,&color,&topLeft,
						&botRight,cellIndex,&m_navTextColor);
			}
		}
	}
	else // two src lines are to be visible
	{
		// this is cell[0]

		if (!gbShowTargetOnly) // don't suppress the cell if we want to show src text line
		{
			// calculate the cell's topLeft and botRight points (CText calculates the CRect)
			topLeft = wxPoint(pRectPile->GetLeft(), pRectPile->GetTop());
			botRight = wxPoint(pRectPile->GetRight(), pRectPile->GetBottom());
			// whm: the wx version doesn't use negative offsets
			botRight.y = pRectPile->GetTop() + pApp->m_nSrcHeight;

			// calculate the extent of the text
			int spWidth;
			int spDummyHeight;
			pDC->SetFont(*pSrcFont);
			pDC->GetTextExtent(pSrcPhrase->m_srcPhrase, &spWidth, &spDummyHeight);

			//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
			//				pSrcPhrase->m_srcPhrase,spWidth,pSrcFont,
			//				&color,&topLeft,&botRight,cellIndex); // BEW deprecated 3Feb09
			CCell* pCell = new CCell();
			pPile->m_pCell[cellIndex] = pCell;
			pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_srcPhrase,spWidth,pSrcFont,
							&color,&topLeft,&botRight,cellIndex,&m_navTextColor);

			// now the second cell, cell[1]
			cellIndex++;

			// whm: the wx version doesn't use negative offsets
			topLeft.y = topLeft.y+pApp->m_nSrcHeight;
			botRight.y = botRight.y+pApp->m_nSrcHeight;

			// calculate the extent of the text
			int keyWidth;
			int keyDummyHeight;
			pDC->GetTextExtent(pSrcPhrase->m_key, &keyWidth, &keyDummyHeight);

			//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
			//				pSrcPhrase->m_key,keyWidth,pSrcFont,&color,
			//				&topLeft,&botRight,cellIndex); // BEW deprecated 3Feb09
			pCell = new CCell();
			pPile->m_pCell[cellIndex] = pCell;
			pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_key,keyWidth,pSrcFont,&color,
							&topLeft,&botRight,cellIndex,&m_navTextColor);
		}
		else
		{
			cellIndex++; // suppress of both wanted, so just ready the index for next increment
		}
	} // end of blocks for one or two source text rows

	if (gbIsGlossing)
	{
		// we are glossing and gbEnableGlossing is TRUE, so glosses will be visible
		// as the 2nd line of the strip (or third if both src lines are visible,) so set up
		// the cell with cellIndex == 2 (and note, if the flag gbShowTargetOnly is TRUE, this
		// would be the only line in the strip which is visible)
		if (gbEnableGlossing)
		{
			// set the gloss text color, and if using navText's font for the glossing
			// then set up the device context to use it; else use target font
			color = pApp->m_navTextColor; // glossing uses navText color always
			if (gbGlossingUsesNavFont)
			{
				pDC->SetFont(*pNavTextFont);
			}
			else
			{
				pDC->SetFont(*pTgtFont);
			}

			// a gloss line is wanted, so construct its cell
			cellIndex++; // cellIndex == 2

			// calculate the cell's topLeft and botRight points
			if (!gbShowTargetOnly) // this flag refers to glosses when gbIsGlossing is TRUE
			{
				if (gbGlossingUsesNavFont)
				{
					// give 3 pixel extra offset, for glossing line only
					// whm: the wx version doesn't use negative offsets
					topLeft.y = topLeft.y+pApp->m_nSrcHeight + 3;
					botRight.y = botRight.y+pApp->m_nNavTextHeight + 3;
				}
				else
				{
					// whm: the wx version doesn't use negative offsets
					topLeft.y = topLeft.y+pApp->m_nSrcHeight + 3;
					botRight.y = botRight.y+pApp->m_nTgtHeight + 3;
				}
			}
			else // we are showing only the glossing text (ie. no source and no target lines)
			{
				// calculate the cell's topLeft and botRight points (CText calculates the CRect),
				// as it is the only cell in the pile, for this mode
				topLeft = wxPoint(pRectPile->GetLeft(), pRectPile->GetTop());
				botRight = wxPoint(pRectPile->GetRight(), pRectPile->GetBottom());
				if (gbGlossingUsesNavFont)
				{
					// whm: the wx version doesn't use negative offsets
					botRight.y = pRectPile->GetTop() + pApp->m_nNavTextHeight;
				}
				else
				{
					// whm: the wx version doesn't use negative offsets
					botRight.y = pRectPile->GetTop() + pApp->m_nTgtHeight;
				}
			}

			// calculate the extent of the text -- but if there is no text, then make
			// the extent equal to the pileWidth (use m_adaption, so will be correct
			// for retranslations, etc)
			int glossWidth;
			int glossDummyHeight;
			if (pSrcPhrase->m_gloss.IsEmpty())
			{
				// no translation or retranslation, etc, so use pileWidth for extent
				glossWidth = pPile->m_nWidth;
			}
			else
			{
				// get the extent of the text
				pDC->GetTextExtent(pSrcPhrase->m_gloss, &glossWidth, &glossDummyHeight);
			}

			// create the cell and set its attributes; which font gets used will
			// depend on the gbGlossingUsesNavFont flag
			if (gbGlossingUsesNavFont)
			{
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//	pSrcPhrase->m_gloss,glossWidth,pNavTextFont,&color,&topLeft,
				//	&botRight,cellIndex); // BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_gloss,glossWidth,
					pNavTextFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
			}
			else
			{
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//	pSrcPhrase->m_gloss,glossWidth,pTgtFont,&color,&topLeft,
				//	&botRight,cellIndex); // BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_gloss,glossWidth,
					pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
			}
		}

		if (!gbShowTargetOnly)
		{
			// continue to 4th visible line (or 3nd visible line if m_bSuppressFirst is TRUE),
			// this is cell[3] and is the first of the lines which may have target text
			cellIndex++; // set cellIndex to 3

			// set the target language's text color
			color = pApp->m_targetColor;

			// calculate the cell's topLeft and botRight points
			if (gbGlossingUsesNavFont)
			{
				// whm: the wx version doesn't use negative offsets
				topLeft.y = topLeft.y+pApp->m_nNavTextHeight;
				botRight.y = botRight.y+pApp->m_nTgtHeight;
			}
			else
			{
				// whm: the wx version doesn't use negative offsets
				topLeft.y = topLeft.y+pApp->m_nTgtHeight;
				botRight.y = botRight.y+pApp->m_nTgtHeight;
			}

			// calculate the extent of the text -- but if there is no text, then make the extent
			// equal to the pileWidth (use m_adaption, so will be correct for retranslations, etc)
			// (if this cell is at the active location, displaying it will be suppressed externally)
			pDC->SetFont(*pTgtFont);
			if (!pApp->m_bHidePunctuation && pApp->m_bSuppressLast)
			{
				int tgtWidth;
				int tgtDummyHeight;
				if (pSrcPhrase->m_targetStr.IsEmpty())
				{
					// no translation or retranslation, etc, so use pileWidth for extent
					tgtWidth = pPile->m_nWidth; 
				}
				else
				{
					// get the extent of the text
					pDC->GetTextExtent(pSrcPhrase->m_targetStr, &tgtWidth, &tgtDummyHeight);
				}

				// create the cell and set its attributes
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//		pSrcPhrase->m_targetStr,tgtWidth,pTgtFont,&color,&topLeft,&botRight,
				//		cellIndex); // BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_targetStr,tgtWidth,
									pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
			}
			else // either hiding punctuation or not suppressing the last target line
				// (cellIndex == 3)still
			{
				int aWidth;
				int aDummyHeight;
				if (pSrcPhrase->m_adaption.IsEmpty())
				{
					// no translation or retranslation, etc, so use pileWidth for extent
					aWidth = pPile->m_nWidth; // don't care about extent.cy
				}
				else
				{
					// get the extent of the text
					pDC->GetTextExtent(pSrcPhrase->m_adaption, &aWidth, &aDummyHeight);
				}

				// create the cell and set its attributes
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//		pSrcPhrase->m_adaption,aWidth,pTgtFont,&color,&topLeft,&botRight,
				//		cellIndex); // BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_adaption,aWidth,
								pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
			}

			// now the 5th line, provided it is not suppressed (it's a punctuated target
			// text line, if visible)
			if (!pApp->m_bSuppressLast)
			{
				// a fifth line is wanted, construct its cell
				cellIndex++; // cellIndex == 4

				// calculate the cell's topLeft and botRight points
				// whm: the wx version doesn't use negative offsets
				topLeft.y = topLeft.y+pApp->m_nTgtHeight; 
				botRight.y = botRight.y+pApp->m_nTgtHeight;

				// calculate the extent of the text -- but if there is no text, then make
				// the extent equal to the pileWidth (use m_adaption, so will be correct
				// for retranslations, etc)
				int tgtWidth;
				int tgtDummyHeight;
				if (pSrcPhrase->m_targetStr.IsEmpty())
				{
					// no translation or retranslation, etc, so use pileWidth for extent
					//extent.cx = pPile->m_nWidth; // don't care about extent.cy
					tgtWidth = pPile->m_nWidth; // don't care about extent.cy
				}
				else
				{
					// get the extent of the text
					pDC->GetTextExtent(pSrcPhrase->m_targetStr, &tgtWidth, &tgtDummyHeight);
				}

				// create the cell and set its attributes
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//		pSrcPhrase->m_targetStr,tgtWidth,pTgtFont,&color,&topLeft,
				//		&botRight,cellIndex); //BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_targetStr,tgtWidth,
								pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
			}
		}
	}
	else // the user is adapting (rather than glossing)
	{
		// continue to 3rd visible line (or 2nd visible line if m_bSuppressFirst is TRUE),
		// this is cell[2], and it will contain target text
		cellIndex++; // set cellIndex to 2

		// set the target language's text color
		color = pApp->m_targetColor;

		if (!gbShowTargetOnly)
		{
			// calculate the cell's topLeft and botRight points
			// whm: the wx version doesn't use negative offsets
			topLeft.y = topLeft.y+pApp->m_nSrcHeight;
			botRight.y = botRight.y+pApp->m_nTgtHeight;
		}
		else // we are showing only the target text
		{
			// calculate the cell's topLeft and botRight points (CText calculates the CRect),
			// as it is the only cell in the pile, for this mode
			topLeft = wxPoint(pRectPile->GetLeft(), pRectPile->GetTop());
			botRight = wxPoint(pRectPile->GetRight(), pRectPile->GetBottom());
			// whm: the wx version doesn't use negative offsets
			botRight.y = pRectPile->GetTop() + pApp->m_nTgtHeight;
		}

		// calculate the extent of the text -- but if there is no text, then make the extent
		// equal to the pileWidth (use m_adaption, so will be correct for retranslations, etc)
		// (if this cell is at the active location, displaying it will be suppressed externally)
		pDC->SetFont(*pTgtFont);
		if (!pApp->m_bHidePunctuation && pApp->m_bSuppressLast)
		{
			int tgtWidth;
			int tgtDummyHeight;
			if (pSrcPhrase->m_targetStr.IsEmpty())
			{
				// no translation or retranslation, etc, so use pileWidth for extent
				tgtWidth = pPile->m_nWidth; // don't care about extent.cy
			}
			else
			{
				// get the extent of the text
				pDC->GetTextExtent(pSrcPhrase->m_targetStr, &tgtWidth, &tgtDummyHeight);
			}

			// create the cell and set its attributes
			//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
			//		pSrcPhrase->m_targetStr,tgtWidth,pTgtFont,&color,&topLeft,&botRight,
			//		cellIndex); // BEW deprecated 3Feb09
			CCell* pCell = new CCell();
			pPile->m_pCell[cellIndex] = pCell;
			pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_targetStr,tgtWidth,
							pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
		}
		else // either hiding punctuation or not suppressing the last target line
		{
			int aWidth;
			int aDummyHeight;
			if (pSrcPhrase->m_adaption.IsEmpty())
			{
				// no translation or retranslation, etc, so use pileWidth for extent
				aWidth = pPile->m_nWidth; // don't care about extent.cy
			}
			else
			{
				// get the extent of the text
				pDC->GetTextExtent(pSrcPhrase->m_adaption, &aWidth, &aDummyHeight);
			}

			// create the cell and set its attributes
			//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
			//		pSrcPhrase->m_adaption,aWidth,pTgtFont,&color,&topLeft,&botRight,
			//		cellIndex); // BEW deprecated 3Feb09
			CCell* pCell = new CCell();
			pPile->m_pCell[cellIndex] = pCell;
			pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_adaption,aWidth,
							pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
		}

		// now the 4th line, provided it is not suppressed
		if (!pApp->m_bSuppressLast)
		{
			if (!gbShowTargetOnly)
			{
				// a fourth line is wanted, construct its cell
				cellIndex++; // cellIndex == 3

				// calculate the cell's topLeft and botRight points
				// whm: the wx version doesn't use negative offsets
				topLeft.y = topLeft.y+pApp->m_nTgtHeight;
				botRight.y = botRight.y+pApp->m_nTgtHeight;

				// calculate the extent of the text -- but if there is no text, then make
				// the extent equal to the pileWidth (use m_adaption, so will be correct
				// for retranslations, etc)
				int tgtWidth;
				int tgtDummyHeight;
				if (pSrcPhrase->m_targetStr.IsEmpty())
				{
					// no translation or retranslation, etc, so use pileWidth for extent
					tgtWidth = pPile->m_nWidth; // don't care about extent.cy
				}
				else
				{
					// get the extent of the text
					pDC->GetTextExtent(pSrcPhrase->m_targetStr, &tgtWidth, &tgtDummyHeight);
				}

				// create the cell and set its attributes
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//		pSrcPhrase->m_targetStr,tgtWidth,pTgtFont,&color,&topLeft,
				//		&botRight,cellIndex); // BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_targetStr,tgtWidth,
								pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
			}
			else
			{
				cellIndex++; // cellIndex == 3, to keep indices right for any gloss line
			}

		}

		// not glossing, but if gbEnableGlossing is TRUE, then glosses will be visible
		// as the last line of the strip, so set up the 5th cell to contain the gloss
		if (gbEnableGlossing)
		{
			if (!gbShowTargetOnly) // don't calculate the cell if we are showing tgt only
			{
				// set the gloss text color, and if using navText's font for the glossing
				// then set up the device context to use it
				color = pApp->m_navTextColor; // glossing always uses navText's color
				if (gbGlossingUsesNavFont)
				{
					pDC->SetFont(*pNavTextFont);
				}

				// a gloss line is wanted, so construct its cell
				cellIndex++; // cellIndex == 3 (if one tgt line) or 4

				// calculate the cell's topLeft and botRight points; we will offset the
				// gloss line an extra 3 pixels from the line above it, since there appears
				// to be some unwanted encroachment for some fonts in some sizes
				if (gbGlossingUsesNavFont)
				{
					// whm: the wx version doesn't use negative offsets
					topLeft.y = topLeft.y+pApp->m_nTgtHeight + 3;
					botRight.y = botRight.y+pApp->m_nNavTextHeight + 3;
				}
				else
				{
					// whm: the wx version doesn't use negative offsets
					topLeft.y = topLeft.y+pApp->m_nTgtHeight + 3;
					botRight.y = botRight.y+pApp->m_nTgtHeight + 3;
				}

				// calculate the extent of the text -- but if there is no text, then make
				// the extent equal to the pileWidth (use m_adaption, so will be correct
				// for retranslations, etc)
				int glossWidth;
				int glossDummyHeight;
				if (pSrcPhrase->m_gloss.IsEmpty())
				{
					// no translation or retranslation, etc, so use pileWidth for extent
					glossWidth = pPile->m_nWidth; // don't care about extent.cy
				}
				else
				{
					// get the extent of the text
					pDC->GetTextExtent(pSrcPhrase->m_gloss, &glossWidth, &glossDummyHeight);
				}

				// create the cell and set its attributes; which font gets used will
				// depend on the gbGlossingUsesNavFont flag
				if (gbGlossingUsesNavFont)
				{
					//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
					//	pSrcPhrase->m_gloss,glossWidth,pNavTextFont,&color,&topLeft,
					//	&botRight,cellIndex); / BEW deprecated 3Feb09
					CCell* pCell = new CCell();
					pPile->m_pCell[cellIndex] = pCell;
					pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_gloss,glossWidth,
								pNavTextFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
				}
				else
				{
					//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
					//	pSrcPhrase->m_gloss,glossWidth,pTgtFont,&color,&topLeft,
					//	&botRight,cellIndex); // BEW deprecated 3Feb09
					CCell* pCell = new CCell();
					pPile->m_pCell[cellIndex] = pCell;
					pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_gloss,glossWidth,
								pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
				}
			}
		}
	}
*/

void CLayout::SetupCursorGlobals(wxString& phrase, enum box_cursor state, int nBoxCursorOffset)
{
	// set the m_nStartChar and m_nEndChar cursor/selection globals prior to drawing the
	// phrase box

	// get the cursor set
	switch (state)
	{
	case select_all:
		{
			//m_pApp->m_nStartChar = 0; // MFC
			m_pApp->m_nStartChar = -1; // wxWidgets
			m_pApp->m_nEndChar = -1;
			break;
		}
	case cursor_at_text_end:
		{
			int len = phrase.Len();
			m_pApp->m_nStartChar = len;
			m_pApp->m_nEndChar = len;
			break;
		}
	case cursor_at_offset:
		{
			wxASSERT(nBoxCursorOffset >= 0);
			m_pApp->m_nStartChar = nBoxCursorOffset;
			m_pApp->m_nEndChar = m_pApp->m_nStartChar;
			break;
		}
	default: // do this if a garbage value is passed in
		{
			m_pApp->m_nStartChar = 0;
			m_pApp->m_nEndChar = -1;
			break;
		}
	}
}

void CLayout::PlacePhraseBoxInLayout(int nActiveSequNum)
{
	// Call this function in CLayout::Draw() after strips are drawn
	bool bSetModify = FALSE; // initialize, governs what is done with the wxEdit control's dirty flag
	bool bSetTextColor = FALSE; // initialize, governs whether or not we reset the box's text colour
	
	// obtain the TopLeft coordinate of the active pile's m_pCell[1] cell, there the phrase box is
	// to be located
	wxPoint ptPhraseBoxTopLeft;
	CPile* pActivePile = GetPile(nActiveSequNum); // could use view's m_pActivePile instead; but this
					// will work even if we have forgotten to update it in the edit operation's handler
	pActivePile->TopLeft(ptPhraseBoxTopLeft);

	// get the pile width at the active location, using the value in
	// CLayout::m_curBoxWidth put there by RecalcLayout() or AdjustForUserEdits() or FixBox()
	int phraseBoxWidth = pActivePile->GetPhraseBoxGapWidth(); // returns CPile::m_nWidth
    //int phraseBoxWidth = m_curBoxWidth; // I was going to use this, but I think the best
    // design is to only use the value stored in CLayout::m_curBoxWidth for the brief
    // interval within the execution of FixBox() when a box expansion happens (and
    // immediately after a call to RecalcLayout() or later to AdjustForUserEdits()), since
    // then the CalcPhraseBoxGapWidth() call in RecalcLayout() or in AdjustForUserEdits()
    // will use that stored value when gbExpanding == TRUE to test for largest of
    // m_curBoxWidth and a value based on text extent plus slop, and use the larger -
    // setting result in m_nWidth, so the ResizeBox() calls here in PlacePhraseBoxInLayout()
    // should always expect m_nWidth for the active pile will have been correctly set, and
    // so always use that for the width to pass in to the ResizeBox() call below.

	// Note: the m_nStartChar and m_nEndChar app members, for cursor placement or text selection
	// range specification get set by the SetupCursorGlobals() calls in the switch below

	// handle any operation specific parameter settings
	enum doc_edit_op opType = m_docEditOperationType;
	switch(opType)
	{
	case default_op:
		{
			SetupCursorGlobals(m_pApp->m_targetPhrase, select_all); // sets to (-1,-1)
			bSetModify = TRUE;
			break;
		}
	case cancel_op:
		{

			break;
		}
	case char_typed_op:
		{
			// don't interfere with the m_nStartChar and m_nEndChar values, just set
			// modify flag
			bSetModify = TRUE;
			break;
		}
	case target_box_paste_op:
		{
			SetupCursorGlobals(m_pApp->m_targetPhrase, cursor_at_offset, gnBoxCursorOffset);
			bSetModify = FALSE;
			break;
		}
	case relocate_box_op:
		{
			bSetModify = FALSE;
			bSetTextColor = TRUE;
			break;
		}
	case merge_op:
		{

			break;
		}
	case unmerge_op:
		{

			break;
		}
	case retranslate_op:
		{
			SetupCursorGlobals(m_pApp->m_targetPhrase, select_all); // sets to (-1,-1)
			bSetModify = FALSE;
			break;
		}
	case remove_retranslation_op:
		{
			SetupCursorGlobals(m_pApp->m_targetPhrase, select_all); // sets to (-1,-1)
			bSetModify = FALSE;
			break;
		}
	case edit_retranslation_op:
		{
			SetupCursorGlobals(m_pApp->m_targetPhrase, select_all); // sets to (-1,-1)
			bSetModify = FALSE;
			break;
		}
	case insert_placeholder_op:
		{

			break;
		}
	case remove_placeholder_op:
		{ 
			SetupCursorGlobals(m_pApp->m_targetPhrase, select_all); // sets to (-1,-1)
			break;
		}
	case consistency_check_op:
		{
			m_pView->RemoveSelection();
			break;
		}
	case split_op:
		{

			break;
		}
	case join_op:
		{

			break;
		}
	case move_op:
		{

			break;
		}
	case on_button_no_adaptation_op:
		{
			m_pApp->m_nStartChar = 0;
			m_pApp->m_nEndChar = 0;
			bSetModify = TRUE;
			break;
		}
	case edit_source_text_op:
		{

			break;
		}
	case free_trans_op:
		{

			break;
		}
	case end_free_trans_op:
		{

			break;
		}
	case retokenize_text_op:
		{
			SetupCursorGlobals(m_pApp->m_targetPhrase, cursor_at_text_end);
			bSetModify = FALSE;
			break;
		}
	case collect_back_translations_op:
		{

			break;
		}
	case vert_edit_enter_adaptions_op:
		{

			break;
		}
	case vert_edit_exit_adaptions_op:
		{

			break;
		}
	case vert_edit_enter_glosses_op:
		{

			break;
		}
	case vert_edit_exit_glosses_op:
		{

			break;
		}
	case vert_edit_enter_free_trans_op:
		{

			break;
		}
	case vert_edit_exit_free_trans_op:
		{

			break;
		}
	case vert_edit_cancel_op:
		{

			break;
		}
	case vert_edit_end_now_op:
		{

			break;
		}
	case vert_edit_previous_step_op:
		{

			break;
		}
	case vert_edit_exit_op:
		{
			SetupCursorGlobals(m_pApp->m_targetPhrase, select_all); // sets to (-1,-1)
			bSetModify = FALSE;
			break;
		}
	case vert_edit_bailout_op:
		{
			SetupCursorGlobals(m_pApp->m_targetPhrase, select_all); // sets to (-1,-1)
			bSetModify = FALSE;
			break;
		}
	case exit_preferences_op:
		{

			break;
		}
	case change_punctuation_op:
		{

			break;
		}
	case change_filtered_markers_only_op:
		{

			break;
		}
	case change_sfm_set_only_op:
		{

			break;
		}
	case change_sfm_set_and_filtered_markers_op:
		{

			break;
		}
	case open_document_op:
		{

			break;
		}
	case new_document_op:
		{

			break;
		}
	case close_document_op:
		{

			break;
		}
	case enter_LTR_layout_op:
		{

			break;
		}
	case enter_RTL_layout_op:
		{

			break;
		}
	default: // do the same as default_op	
	case no_edit_op:
		{
			// do nothing additional
			break;
		}
	}
	// reset m_docEditOperationType to an invalid value, so that if not explicitly set by
	// the user's editing operation, or programmatic operation, the default: case will
	// fall through to the no_edit_op case, which does nothing
	m_docEditOperationType = invalid_op_enum_value; // an invalid value

	// do any required auto capitalization...
	// if auto capitalization is on, determine the source text's case properties
	bool bNoError = TRUE;
	if (gbAutoCaps)
	{
		bNoError = m_pView->SetCaseParameters(pActivePile->GetSrcPhrase()->m_key);
	}
	// now set the m_targetPhrase contents accordingly
	if (gbAutoCaps)
	{
		if (bNoError && gbSourceIsUpperCase)
		{
			// in the next call, FALSE is the value for param bool bIsSrcText
			bNoError = m_pView->SetCaseParameters(m_pApp->m_targetPhrase,FALSE);
			if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
			{
				// change to upper case initial letter
				m_pApp->m_targetPhrase.SetChar(0,gcharNonSrcUC);
			}
		}
	}

	// wx Note: we don't destroy the target box, just set its text to null
	m_pApp->m_pTargetBox->SetValue(_T(""));

	// make the phrase box size adjustments, set the colour of its text, tell it where
	// it is to be drawn. ResizeBox doesn't recreate the box; it just calls SetSize()
	// and causes it to be visible again; CPhraseBox has a color variable & uses 
	// reflected notification
	if (gbIsGlossing && gbGlossingUsesNavFont)
	{
		m_pView->ResizeBox(&ptPhraseBoxTopLeft, phraseBoxWidth, GetNavTextHeight(), m_pApp->m_targetPhrase,
					m_pApp->m_nStartChar, m_pApp->m_nEndChar, pActivePile);
		m_pApp->m_pTargetBox->m_textColor = GetNavTextColor(); //was pApp->m_navTextColor;
	}
	else
	{
		m_pView->ResizeBox(&ptPhraseBoxTopLeft, phraseBoxWidth, GetTgtTextHeight(), m_pApp->m_targetPhrase,
					m_pApp->m_nStartChar, m_pApp->m_nEndChar, pActivePile);
		m_pApp->m_pTargetBox->m_textColor = GetTgtColor(); // was pApp->m_targetColor;
	}

	// set the color - CPhraseBox has a color variable & uses reflected notification
	if (bSetTextColor)
	{
		if (gbIsGlossing && gbGlossingUsesNavFont)
			m_pApp->m_pTargetBox->m_textColor = GetNavTextColor();
		else
			m_pApp->m_pTargetBox->m_textColor = GetTgtColor();
	}
	// handle the dirty flag
	if (bSetModify)
	{
		// calls our own SetModify(TRUE)(see Phrasebox.cpp)
		m_pApp->m_pTargetBox->SetModify(TRUE); 
	}
	else
	{
		// call our own SetModify(FALSE) which calls DiscardEdits() (see Phrasebox.cpp)
		m_pApp->m_pTargetBox->SetModify(FALSE); 
	}

}

