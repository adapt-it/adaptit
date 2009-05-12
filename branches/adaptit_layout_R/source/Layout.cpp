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

#include "Adapt_It.h"
#include "Adapt_ItDoc.h"
#include "AdaptitConstants.h"
#include "SourcePhrase.h"
#include "MainFrm.h"
#include "helpers.h"
// don't mess with the order of the following includes, Strip must precede View must precede
// Pile must precede Layout and Cell can usefully by last
#include "Strip.h"
#include "Adapt_ItView.h"
#include "Pile.h"
#include "Cell.h"
#include "Adapt_ItCanvas.h"
#include "Layout.h"
//#include "memory.h"

/// This global is defined in Adapt_It.cpp. (default is FALSE)
extern bool gbDoingInitialSetup;

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

/// This global is defined in Adapt_It.cpp.
extern bool gbSuppressSetup;

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

//static int nDebugIndex = 0;

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
	m_stripArray.Clear();
}

CLayout::~CLayout()
{

}

// call InitializeCLayout when the application has the view, canvas, and document classes
// initialized -- we set up pointers to them here so we want them to exist first --we'll
// get a message (and an assert in debug mode) if we call this too early
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
	m_invalidStripArray.Clear();
	m_bDrawAtActiveLocation = TRUE;
	m_docEditOperationType = invalid_op_enum_value;
	m_bLayoutWithoutVisiblePhraseBox = FALSE;

    // can add more basic initializations above here - but only stuff that makes
    // the session-persistent m_pLayout pointer on the app class have the basic info it
    // needs, other document-related initializations can be done in SetupLayout()
}

// for setting or clearing the m_bLayoutWithoutVisiblePhraseBox boolean
void CLayout::SetBoxInvisibleWhenLayoutIsDrawn(bool bMakeInvisible)
{
	m_bLayoutWithoutVisiblePhraseBox = bMakeInvisible;
}

void CLayout::Draw(wxDC* pDC)
{
    // drawing is done based on the top of the first strip of a visible range of strips
    // determined by the scroll car position; to have drawing include the phrase box, a
    // caller (typically a handler for a user edit operation) has to set the active
    // location (the app's m_nActiveSequNum value) correctly first; for ScrollUp() and
    // ScrollDown() the function handlers of these ignore the active location and just
    // define a client area's worth of drawing based on the scroll car position
     
    


	int i;
	int nFirstStripIndex = -1;
	int nLastStripIndex = -1;
	int nActiveSequNum = -1;

	// *** TODO *** AdjustForUserEdits(), once we get rid of RecalcLayout(FALSE) calls,
	// will need to replace those calls in the doc editing handlers themselves, eg
	// OnButtonMerge() etc... OR, we may put AdjustForUserEdits() within RecalcLayout()
	// and have a test to choose when we use that, or instead use the older full relayout code
	
	

	// work out the range of visible strips based on the phrase box location
	nActiveSequNum = m_pApp->m_nActiveSequNum;
	// determine which strips are to be drawn  (a scrolled wxDC must be passed in)
	GetVisibleStripsRange(pDC, nFirstStripIndex, nLastStripIndex);

	// draw the visible strips (plus and extra one, if possible)
	for (i = nFirstStripIndex; i <=  nLastStripIndex; i++)
	{
		((CStrip*)m_stripArray.Item(i))->Draw(pDC);
	}

	// get the phrase box placed in the active location and made visible, and suitably
	// prepared - unless it should not be made visible (eg. when updating the layout
	// in the middle of a procedure, before the final update is done at a later time)
	if (!m_bLayoutWithoutVisiblePhraseBox)
	{
		// work out its location and resize (if necessary) and draw it
		PlacePhraseBoxInLayout(m_pApp->m_nActiveSequNum);
	}
	SetBoxInvisibleWhenLayoutIsDrawn(FALSE); // restore default

	m_invalidStripArray.Clear(); // initialize for next user edit operation
}

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
	else // normal adapting or glossing, at least 2 lines (src & tgt) but even if 3, 
		 // still those two
	{
		m_nPileHeight = m_nSrcHeight + m_nTgtHeight;

        // we've accounted for source and target lines; now handle possibility of a 3rd
        // line (note, if 3 lines, target is always one, so we've handled that above
        // already)
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
    // the pile height is now set; so in the stuff below, set the strip height too - it is
    // the m_nPileHeight value, +/-, depending on whether free translation mode is on or
    // not, the target text line height plus 3 pixels of separating space from the bottom
    // of the pile
    // (Note: the m_nCurLeading value, for the navText area, is NOT regarded as part of 
    // the strip)
	m_nStripHeight = m_nPileHeight;
	if (m_pApp->m_bFreeTranslationMode && !gbIsPrinting)
	{
        // add enough space for a single line of the height given by the target text's
        // height + 3 pixels to set it off a bit from the bottom of the pile
		m_nStripHeight += m_nTgtHeight + 3;
	}
}

void CLayout::RecalcPileWidths(PileList* pPiles)
{
	PileList::Node* pos = pPiles->GetFirst();
	wxASSERT(pos != NULL);
	CPile* pPile = NULL;
	while (pos != NULL)
	{
		pPile = pos->GetData();
		wxASSERT(pPile != NULL);
		pPile->SetMinWidth(); // the calculation of the gap for the phrase box is
							// handled within RecalcLayout(), so does not need to be
							// done here
		pos = pos->GetNext();
	}
	SetPileAndStripHeight(); // it may be changing, eg to or from "See Glosses"
}

int CLayout::GetStripCount()
{
	return (int)m_stripArray.GetCount();
}

PileList* CLayout::GetPileList()
{
	return &m_pileList;
}

wxArrayPtrVoid* CLayout::GetStripArray()
{
	return & m_stripArray;
}

// Call SetClientWindowSizeAndLogicalDocWidth() before just before strips are laid out;
// then call SetLogicalDocHeight() after laying out the strips, to get private member
// m_logicalDocSize.y set
void CLayout::SetClientWindowSizeAndLogicalDocWidth()
{
	// GetClientRect gets a rectangle in which upper left coords are always 0,0
    //pApp->GetMainFrame()->canvas->GetClientSize(&fwidth,&fheight); // get width & height
    //in pixels wx note: calling GetClientSize on the canvas produced different results in
    //wxGTK and wxMSW, so I'll use my own GetCanvasClientSize() which calculates it from
    //the main frame's client size.
	wxSize canvasViewSize;
	canvasViewSize = m_pMainFrame->GetCanvasClientSize(); // dimensions of client window of
								// wxScrollingWindow which canvas class is a subclass of
	m_sizeClientWindow = canvasViewSize; // set the private member, CLayout::m_sizeClientWindow
	wxSize docSize;
	docSize.y = 0; // can't be set yet, we call this setter before strips are laid out
	if (gbIsPrinting)
	{
		// the document width will be set by the page dimensions and margins, externally
		// to RecalcLayout(), so do nothing here except set it to zero
		docSize.x = 0;
	}
	else
	{
		// not printing, so the layout is being done for the screen
		docSize.x = m_sizeClientWindow.x - m_nCurLMargin - RH_SLOP; 
                // RH_SLOP defined in AdaptItConstants.h with a value of 40 (reduces
                // likelihood of long nav text above a narrow pile which is last in a
                // strip, having the end of the nav text drawn off-window
	}
	m_logicalDocSize = docSize; // initialize the private member, CLayout::m_logicalDocSize
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
		nDocHeight = (GetCurLeading() + GetStripHeight()) * nStripCount;
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


// SetLayoutParameters() is where we do most of the hooking up to the current state of the
// app's various view-related parameters, such as fonts, colours, text heights, and so
// forth
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
	// CStrip now does not store pointers, so the only memory it owns are the blocks for
	// the two wxArrayInt arrays - so Clear() these and then delete the strip
	CStrip* pStrip = (CStrip*)m_stripArray.Item(index);
#ifdef _ALT_LAYOUT_
	if (!pStrip->m_arrPileIndices.IsEmpty())
	{
		pStrip->m_arrPileIndices.Clear();
	}
	if (!pStrip->m_arrPileOffsets.IsEmpty())
	{
		pStrip->m_arrPileOffsets.Clear();
	}
#else
	if (!pStrip->m_arrPiles.IsEmpty())
	{
		pStrip->m_arrPiles.Clear();
	}
	if (!pStrip->m_arrPiles.IsEmpty())
	{
		pStrip->m_arrPiles.Clear();
	}
#endif
 	delete pStrip;
	// don't try to delete CCell array, because the cell objects are managed 
    // by the persistent pile pointers in the CLayout array m_pPiles, and the 
    // strip does not own these
}

void CLayout::DestroyStripRange(int nFirstStrip, int nLastStrip)
{
	if (m_stripArray.IsEmpty())
		return; // needed because DestroyStripRange() can be called when 
				// nothing is set up yet
	int index;
	for (index = nFirstStrip; index <= nLastStrip; index++)
		DestroyStrip(index);
}

void CLayout::DestroyStrips()
{
	if (m_stripArray.IsEmpty())
		return; // needed because DestroyStrips() can be called when nothing 
				// is set up yet
	int nLastStrip = m_stripArray.GetCount() - 1;
	DestroyStripRange(0, nLastStrip);
	m_stripArray.Clear();
}

void CLayout::DestroyPile(CPile* pPile, PileList* pPileList, bool bRemoveFromListToo)
{	
	PileList::Node* pos;
	int index;
	pPile->SetStrip(NULL); // sets m_pOwningStrip to NULL
	for (index = 0; index < MAX_CELLS; index++)
	{
		delete pPile->m_pCell[index];
	}
	if (bRemoveFromListToo)
	{
		pos = pPileList->Find(pPile);
		wxASSERT(pos != NULL);
		pPileList->Erase(pos);
	}
	delete pPile;
	pPile = NULL;
}

/* not used
void CLayout::DestroyPileRange(int nFirstPile, int nLastPile)
{
	if (m_pileList.IsEmpty())
		return; // needed because DestroyPileRange() can be called when nothing is set up yet
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
*/

void CLayout::DestroyPiles()
{
	if (m_pileList.IsEmpty())
		return; // needed because DestroyPiles() can be called when nothing is set up yet
	PileList::Node* pos = m_pileList.GetLast();
	wxASSERT(pos != NULL);
	CPile* pPile = NULL;
	while (pos != NULL)
	{
		pPile = pos->GetData();
		DestroyPile(pPile,&m_pileList,FALSE);
		pos = pos->GetPrevious();
	}
	m_pileList.Clear(); // ensure there are no freed pointers left over
}


// Note: never call CreatePile() if there is not yet a valid pSrcPhrase pointer to pass in;
// CreatePile() creates the CPile instance on the heap, and returns a pointer to it
CPile* CLayout::CreatePile(CSourcePhrase* pSrcPhrase)
{
	wxASSERT(pSrcPhrase != NULL);

	// create an empty pile on the heap
	CPile* pPile = new CPile;
	wxASSERT(pPile != NULL); // if tripped, must be a memory error

//#ifdef __WXDEBUG__
//	nDebugIndex = 1;
//	wxLogDebug(_T("DebugIndex = %d  CLayout, CPile pointer  %x"),nDebugIndex,pPile);
//#endif

	// assign the passed in source phrase pointer to its m_pSrcPhrase public member,
	// and also the pointer to the CLayout object
	pPile->m_pSrcPhrase = pSrcPhrase;
	pPile->m_pLayout = this;

    // set (in pixels) its minimum width based on which of the m_srcPhrase, m_adaption and
    // m_gloss strings within the passed in CSourcePhrase is the widest; the min width
    // member is thereafter a lower bound for the width of the phrase box when the latter
    // contracts, if located at this pile while the user is working
	pPile->m_nMinWidth = pPile->CalcPileWidth(); // if an empty string, it is defaulted to 
												 // 10 pixels
	if (pSrcPhrase->m_nSequNumber != m_pApp->m_nActiveSequNum)
	{
		pPile->m_nWidth = PHRASE_BOX_WIDTH_UNSET; // a default -1 value for starters, need 
                // an active sequ number and m_targetPhrase set before a value can be
                // calculated, but only at the pile which is located at the active location
	}
	else
	{
		// this pile is going to be the active one, so calculate its m_nWidth value using 
		// m_targetPhrase
		pPile->SetPhraseBoxGapWidth(); // calculates, and sets value in m_nWidth
	}

	// pile creation always creates the (fixed) array of CCells which it manages
	int index;
	CCell* pCell = NULL;
	for (index = 0; index < MAX_CELLS; index++)
	{
		pCell = new CCell();

//#ifdef __WXDEBUG__
//	nDebugIndex = 2;
//	wxLogDebug(_T("DebugIndex = %d  CLayout, CreatePile, CCell pointer  %x"),nDebugIndex,pCell);
//#endif


		// store it
		pPile->m_pCell[index] = pCell; // index ranges through 0 1 and 2 in our new design
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
		wxMessageBox(_T(
		"SPList* passed in was either NULL or devoid of pilesl in CreatePiles()"),
		_T(""), wxICON_STOP);
		wxASSERT(FALSE);
		wxExit(); // something seriously wrong in design, so don't try to go on
		return FALSE;
	}

    // we allow CreatePiles() to be called even when the list of piles is still populated,
    // so CreatePiles() has the job of seeing that the old ones are deleted before creating
    // a new set bool bIsEmpty = m_pPiles->IsEmpty();
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
    // Note: lack of memory could make the following loop fail in the release version if
    // the user is processing a very large document - but recent computers should have
    // plenty of RAM available, so we'll assume a low memory error will not arise; so keep
    // the code lean
	while (pos != NULL)
	{
		// get the CSourcePhrase pointer from the passed in list
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		wxASSERT(pSrcPhrase != NULL);

		// create the CPile instance
		//pPile = CreatePile(this,pSrcPhrase);
		pPile = CreatePile(pSrcPhrase);
		wxASSERT(pPile != NULL);

		// store it m_pPiles list of CLayout
		m_pileList.Append(pPile);

		// get ready for next iteration
		pos = pos->GetNext();
	}

    // To succeed, the count of items in each list must be identical 
	if (pSrcPhrases->GetCount() != m_pileList.GetCount())
	{
		wxMessageBox(_T(
"SPList* passed in, in CreatePiles(), has a count which is different from that of the m_pPiles list in CLayout"),
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
bool CLayout::RecalcLayout(SPList* pList, enum layout_selector selector)
{
    // RecalcLayout() is the refactored equivalent to the former view class's RecalcLayout()
    // function - the latter built only a bundle's-worth of strips, but the new design must build
	// strips for the whole document - so potentially may consume a lot of time; however, the
	// efficiency of the new design (eg. no rectangles are calculated) may compensate significantly
//#ifdef __WXDEBUG__
//	CAdapt_ItApp* pAppl = &wxGetApp();
//	wxLogDebug(_T("Location = %d  Active Sequ Num  %d"),1,pAppl->m_nActiveSequNum);
//#endif

	SPList* pSrcPhrases = pList; // the list of CSourcePhrase instances which
        // comprise the document, or a sublist copied from it, -- the CLayout instance will
        // own a parallel list of CPile instances in one-to-one correspondence with the
        // CSourcePhrase instances, and each of piles will contain a pointer to the
        // sourcePhrase it is associated with
	
	if (selector == create_strips_and_piles || selector == create_strips_keep_piles)
	{
		// any existing strips have to be destroyed before the new are build. Note: if we
		// support both modes of strip support (ie. tweaking versus destroy and rebuild all)
		// then the tweaking option must now destroy them, but just make essential adjustments
		// for the stuff currently in the visible range
		DestroyStrips();
		
		if (pSrcPhrases == NULL || pSrcPhrases->IsEmpty())
		{
			// no document is loaded, so no layout is appropriate yet, do nothing
			// except ensure m_pPiles is NULL
			if (!(m_pileList.IsEmpty()))
			{
				m_pileList.DeleteContents(TRUE); // TRUE means "delete the stored CCell instances too"
			}
			// remove this message - it gets shown about a dozen times before the Welcome
			// splash window appears!!
			// a message to us developers is needed here, in case we get the design wrong
			//wxMessageBox(_T("Warning: SetupLayout() did nothing because there are no CSourcePhrases yet."),
			//			_T(""), wxICON_WARNING);
			return TRUE;
		}
	}

	// preserve selection parameters, so it can be preserved across the recalculation
	m_pView->StoreSelection(m_pApp->m_selectionLine);

	
	// send the app the current size & position data, for saving to config file on closure
	wxRect rectFrame;
	CMainFrame *pFrame = wxGetApp().GetMainFrame();
	wxASSERT(pFrame != NULL);
	rectFrame = pFrame->GetRect(); // screen coords
	rectFrame = NormalizeRect(rectFrame); // use our own from helpers.h
	m_pApp->m_ptViewTopLeft.x = rectFrame.x;
	m_pApp->m_ptViewTopLeft.y = rectFrame.y;
	m_pApp->m_szView.SetWidth(rectFrame.GetWidth()); 
	m_pApp->m_szView.SetHeight(rectFrame.GetHeight());
	m_pApp->m_bZoomed = pFrame->IsMaximized();

	// initialize CLayout's m_logicalDocSize, and m_sizeClientWindow (except 
	// m_logicalDocSize.y is initialized to 0 here, because it can't be set until the
	// strips have been laid out - that is done below
	SetClientWindowSizeAndLogicalDocWidth();

	// set the pile height, and the strip height (the latter includes the free translation
	// height when in free translation mode; the former is always just the height of the
	// visible cells)
	SetPileAndStripHeight();

	// scroll support...
	// get a device context, and get the origin adjusted (gRectViewClient is ignored 
	// when printing)
	wxClientDC viewDC(m_pApp->GetMainFrame()->canvas);
	m_pApp->GetMainFrame()->canvas->DoPrepareDC(viewDC); //  adjust origin
	m_pApp->GetMainFrame()->canvas->CalcUnscrolledPosition(
										0,0,&grectViewClient.x,&grectViewClient.y);
	grectViewClient.width = m_sizeClientWindow.GetWidth(); // m_sizeClientWindow set in
						// the above call to SetClientWindowSizeAndLogicalDocWidth()
	grectViewClient.height = m_sizeClientWindow.GetHeight();
	// if we are printing, then we will want text extents (which use viewDC for their calculation)
	// to be done for MM_LOENGLISH mapping mode
	// whm notes:
	// wxWidgets has the following defined mapping modes:
	// wxMM_TWIPS		Each logical unit is 1/20 of a point, or 1/1440 of an inch.
	// wxMM_POINTS		Each logical unit is a point, or 1/72 of an inch.
	// wxMM_METRIC		Each logical unit is 1 mm.
	// wxMM_LOMETRIC	Each logical unit is 1/10 of a mm.
	// wxMM_TEXT		Each logical unit is 1 pixel.
	// MFC has the following defined mapping modes:
	// MM_ANISOTROPIC	Logical units are converted to arbitrary units with arbitrarily scaled axes. Setting the mapping mode to MM_ANISOTROPIC does not change the current window or viewport settings. To change the units, orientation, and scaling, call the SetWindowExt and SetViewportExt member functions. 
	// MM_HIENGLISH		Each logical unit is converted to 0.001 inch. Positive x is to the right; positive y is up. 
	// MM_HIMETRIC		Each logical unit is converted to 0.01 millimeter. Positive x is to the right; positive y is up. 
	// MM_ISOTROPIC		Logical units are converted to arbitrary units with equally scaled axes; that is, 1 unit along the x-axis is equal to 1 unit along the y-axis. Use the SetWindowExt and SetViewportExt member functions to specify the desired units and the orientation of the axes. GDI makes adjustments as necessary to ensure that the x and y units remain the same size. 
	// MM_LOENGLISH		Each logical unit is converted to 0.01 inch. Positive x is to the right; positive y is up. 
	// MM_LOMETRIC		Each logical unit is converted to 0.1 millimeter. Positive x is to the right; positive y is up. 
	// MM_TEXT			Each logical unit is converted to 1 device pixel. Positive x is to the right; positive y is down. 
	// MM_TWIPS			Each logical unit is converted to 1/20 of a point. (Because a point is 1/72 inch, a twip is 1/1440 inch.) Positive x is to the right; positive y is up. 
	// There are only 3 mapping modes that use the same units between MFC and wxWidgets which are:
	// wxMM_TWIPS is same as MM_TWIPS - in both each logical unit is converted to 1/20 of a point or 1/1440 inch. Positive x to the right, positive y is up.
	// wxMM_TEXT is same as MM_TEXT - in both each logical unit is converted to 1 device pixel. Positive x to the right, positive y is down.
	// wxMM_LOMETRIC is same as MM_LOMETRIC - in both each logical unit is converted to 1/10 of a mm
	// All other mapping modes are different units or scales
	
	// The map mode remains wxMM_TEXT for both normal screen rendering and for printing/previewing.
	// MFC had MM_LOENGLISH, in which each logical unit is converted to 0.01 inch.
	// and Positive x is to the right; positive y is up.
	// wxMM_LOENGLISH compiles OK but give a run time error/assert "unknown mapping mode in
	// SetMapMode." Therefore if we use anything other than wxMM_TEXT, we will need to use wxMM_LOMETRIC
	// for printing in the wx version. Since MFC uses MM_LOENGLISH which reverses the y axis
	// component during printing and previewing, we'll use wxMM_LOMETRIC, the closest equivalent which
	// also reverses the y axis component during printing.
	viewDC.SetMapMode(wxMM_TEXT); // equivalent to MFC's MM_TEXT for drawing to the screen


	// RecalcLayout() depends on the app's m_nActiveSequNum valuel for where the active
	// location is to be; so we'll make that dependency explicit in the next few lines,
	// obtaining the active pile pointer which corresponds to that active location as well
	bool bAtDocEnd = FALSE; // set TRUE if m_nActiveSequNum is -1 (as is the case when at 
							// the end of the document)
	CPile* pActivePile;
	pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum); // will return NULL if sn is -1
	if (!gbDoingInitialSetup)
	{
		// the above test is to exclude setting bAtDocEnd to TRUE if the pActivePile is
		// NULL which can be the case on launch or opening a doc or creating a new one, at
		// least temporarily, due to the default sequence number being -1;
		// gbDoingInitialSetup is cleared to FALSE in OnNewDocument() and OnOpenDocument()
		if (pActivePile == NULL)
			bAtDocEnd = TRUE; // provide a different program path when this is the case
	}

    // attempt the (re)creation of the m_pileList list of CPile instances if requested; if
    // not requested then the current m_pileList's contents will be retained, though their
    // contents may be adjusted in the area where the user did editing
	if (selector == create_strips_and_piles)
	{
		bool bIsOK = CreatePiles(pSrcPhrases);
		if (!bIsOK)
		{
            // something was wrong - memory error or perhaps m_pPiles is a populated list
            // already (CreatePiles()has generated an error message for the developer
            // already)
			return FALSE;
		}
	}

	int gap = m_nCurGapWidth; // distance in pixels for interpile gap
	int nStripWidth = (GetLogicalDocSize()).x; // constant for any one RecalcLayout call

    // before building or tweaking the strips, we want to ensure that the gap left for the
    // phrase box to be drawn in the layout is as wide as the phrase box is going to be
    // when it is made visible by CLayout::Draw(). When RecalcLayout() is called,
    // gbExpanding global bool may be TRUE, or FALSE (it's set by FixBox() and cleared in
    // FixBox() at the end after layout adjustments are done - including a potential call
    // to RecalcLayout(), it's also cleared to default as a safety first measure at start
    // of each OnChar() call. If we destroyed and recreated the piles in the block above,
    // CreatePile() will, at the active location, made use of the gbExpanding value and set
    // the "hole" for the phrase box to be the appropriate width. But if piles were not
    // destroyed and recreated then the box may be about to be drawn expanded, and so we
    // must make sure that the strip rebuilding about to be done below has the right box
    // width value to be used for the pile at the active location. The safest way to ensure
    // this is the case is to make a call to doc class's ResetPartnerPileWidth(), passing
    // in the CSourcePhrase pointer at the active location - this call internally calls
    // CPile:CalcPhraseBoxGapWidth() to set CPile's m_nWidth value to the right width, and
    // then the strip layout code below can use that value via a call to
    // GetPhraseBoxGapWidth() to make the active pile's width have the right value.
    if (!bAtDocEnd)
	{
		// when not at the end of the document, we will have a valid pile pointer for the
		// current active location
		if (selector == create_strips_keep_piles || selector == keep_strips_keep_piles)
		{
            // these three lines ensure that the active pile's width is based on the
            // CLayout::m_curBoxWidth value that was stored there by any adjustment to the
            // box width done by FixBox() just prior to this RecalcLayout() call ( similar
            // code will be needed in AdjustForUserEdits() when we complete the layout
            // refactoring,
			// *** TODO *** )
			pActivePile = GetPile(m_pApp->m_nActiveSequNum);
			wxASSERT(pActivePile);
			CSourcePhrase* pSrcPhrase = pActivePile->GetSrcPhrase();
			m_pDoc->ResetPartnerPileWidth(pSrcPhrase);
		}
	}
	else // control is past the end of the document
	{
		// we have no active active location currently, and the box is hidden, the active
		// pile is null and the active sequence number is -1, so we want a layout that has
		// no place provided for a phrase box, and we'll draw the end of the document
		gbExpanding = FALSE; // has to be restored to default value
		// m_pView->RestoreSelection(); // there won't be a selection in this circumstance
	}
/*
#ifdef __WXDEBUG__
	{
		PileList::Node* pos = m_pileList.GetFirst();
		CPile* pPile = NULL;
		while (pos != NULL)
		{
			pPile = pos->GetData();
			wxLogDebug(_T("m_srcPhrase:  %s  *BEFORE* pPile->m_pOwningStrip =  %x"),
				pPile->GetSrcPhrase()->m_srcPhrase,pPile->m_pOwningStrip);
			pos = pos->GetNext();
		}
	}
#endif
*/
	// the loop which builds the strips & populates them with the piles
	if (selector == create_strips_and_piles || selector == create_strips_keep_piles)
	{
		CreateStrips(nStripWidth, gap);
	}
	if (selector == keep_strips_keep_piles)
	{
		bool bLayoutTweakingWasSuccessful = TRUE;
		// for this option, strips are not destroyed and recreated, instead they are
		// just tweaked at the location where they were marked as invalid by the
		// m_bValid flag in each being cleared to FALSE (when the user edit region
		// and the active location are separated by a gap, possibly large, all the
		// strips in between are included in the updating/tweaking (since the user is
		// only likely to do a thing like this for an edit area visible from the
		// visible active location, possibly the gap would only be a dozen piles
		// at most, and updating there should be so quick as to be unnoticed)
		// Note: in the next call, if it can't successfully delineate the user's editing
		// area, then if calls RecalcLayout() to get a full strip recreation done instead,
		// and returns FALSE; otherwise TRUE is returned
		bLayoutTweakingWasSuccessful = AdjustForUserEdits(nStripWidth, gap);
		if (bLayoutTweakingWasSuccessful == FALSE)
		{
			// when FALSE was returned, RecalcLayout() will have been reentered internally
			// with the input parameter create_strips_keep_piles, and will have already
			// done the layout recalculation by destroying and recreating all the strips,
			// and so we've nothing to do here except return immediately
			return TRUE;
		}
	}
/*	
#ifdef __WXDEBUG__
	{
		PileList::Node* pos = m_pileList.GetFirst();
		CPile* pPile = NULL;
		while (pos != NULL)
		{
			pPile = pos->GetData();
			wxLogDebug(_T("m_srcPhrase:  %s  *AFTER* pPile->m_pOwningStrip =  %x"),
				pPile->GetSrcPhrase()->m_srcPhrase,pPile->m_pOwningStrip);
			pos = pos->GetNext();
		}
	}
#endif
*/
	gbExpanding = FALSE; // has to be restored to default value

	// the height of the document can now be calculated
	SetLogicalDocHeight();
	
	// next line for debugging...
	//wxSize theVirtualSize = m_pApp->GetMainFrame()->canvas->GetVirtualSize();

	// more scrolling support...
	// whm: SetVirtualSize() is the equivalent of MFC's SetScrollSizes.
	// SetVirtualSize() sets the virtual size of the window in pixels.
	m_pApp->GetMainFrame()->canvas->SetVirtualSize(m_logicalDocSize);

	// inform the application of the document's logical size... -- the canvas
	// class uses this m_docSize value for determining scroll bar parameters and whether
	// horizontal and / or vertical scroll bars are needed
	m_pApp->m_docSize = m_logicalDocSize;

	// next line for debugging...
	//theVirtualSize = m_pApp->GetMainFrame()->canvas->GetVirtualSize();

	// The MFC identifiers m_pageDev and m_lineDev are internal members of CScrollView.
	// I cannot find them anywhere in MFC docs, but looking at CScrollView's sources, it
	// is evident that they are used to set the scrolling parameters within the scrolled
	// view in the MFC app. We'll convert the values to the proper units and use
	// SetScrollbars() to set the scrolling parameters.
	//wxSize nSize = m_pageDev; // m_pageDev is "per page scroll size in device units"
	//wxSize mSize = m_lineDev; // m_lineDev is "per line scroll size in device units"
	// wx note: the m_lineDev value in MFC is equivalent to the first two parameters of
	// wxScrollWindow's SetScrollbars (pixelsPerUnitX, and pixelsPerUnitY). Both MFC and
	// WX values are in pixels.
    // The m_pageDev value in MFC is set below as twice the height of a strip (including
    // leading). In WX the height of a page of scrolled stuff should be determined
    // automatically from the length of the document (in scroll units), divided by the
    // height of the client view (also in scroll units).
	// The parameters needed for SetScrollbars are:
	int pixelsPerUnitX, pixelsPerUnitY, noUnitsX, noUnitsY;
	int xPos, yPos; // xPos and yPos default to 0 if unspecified
	bool noRefresh; // noRefresh defaults to FALSE if unspecified
	// whm note: We allow our wxScrolledWindow to govern our scroll
	// parameters based on the width and height of the virtual document
	// 
    // WX version: We only need to specify the length of the scrollbar in scroll
    // steps/units. Before we can call SetScrollbars we must calculate the size of the
    // document in scroll units, which is the size in pixels divided by the pixels per
    // unit.
	pFrame->canvas->GetScrollPixelsPerUnit(&pixelsPerUnitX,&pixelsPerUnitY);
	noUnitsX = m_pApp->m_docSize.GetWidth() / pixelsPerUnitX;
	noUnitsY = m_pApp->m_docSize.GetHeight() / pixelsPerUnitY;
    // we need to specify xPos and yPos in the SetScrollbars call, otherwise it will cause
    // the window to scroll to the zero position everytime RecalcLayout is called.
    // We'll use GetViewStart instead of CalcUnscrolledPosition here since SetScrollbars
    // below takes scroll units rather than pixels.
	m_pApp->GetMainFrame()->canvas->GetViewStart(&xPos, &yPos); // gets xOrigin and yOrigin
																// in scroll units
	noRefresh = FALSE; // do a refresh
    // Now call SetScrollbars - this is the only place where the scrolling parameters are
    // established for our wxScrolledWindow (canvas). The scrolling parameters are reset
    // everytime RecalcLayout is called. This is the only location where SetScrollbars() is
    // called on the canvas.
    
    // whm IMPORTANT NOTE: We need to use the last position of the scrolled window here. If
    // we don't include xPos and yPos here, the scrollbar immediately scrolls to the
    // zero/initial position in the doc, which fouls up the calculations in other routines
    // such as MoveToPrevPile.
    m_pApp->GetMainFrame()->canvas->SetScrollbars(
			pixelsPerUnitX,pixelsPerUnitY,	// number of pixels per "scroll step"
        	noUnitsX,noUnitsY,				// sets the length of scrollbar in scroll steps, 
											// i.e., the size of the virtual window
    		xPos,yPos,						// sets initial position of scrollbars NEEDED!
    		noRefresh);						// SetScrollPos called elsewhere
  

	// restore the selection, if there was one
	m_pView->RestoreSelection();
	
	// if free translation mode is turned on, get the current section
	// delimited and made visible - but only when not currently printing
	if (m_pApp->m_bFreeTranslationMode && !gbIsPrinting)
	{
		if (!gbSuppressSetup)
		{
			m_pView->SetupCurrentFreeTransSection(m_pApp->m_nActiveSequNum);
		}

		CMainFrame* pFrame;
		pFrame = m_pApp->GetMainFrame();
		wxASSERT(pFrame);
		wxTextCtrl* pEdit = (wxTextCtrl*)
							pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
		pEdit->SetFocus();
		if (!m_pApp->m_pActivePile->GetSrcPhrase()->m_bHasFreeTrans)
		{
			pEdit->SetSelection(-1,-1); // -1, -1 selects it all
		}
		else
		{
			int len = pEdit->GetValue().Length(); 
			if (len > 0)
			{
				pEdit->SetSelection(len,len);
			}
		}
	}
	m_lastLayoutSelector = selector; // inform Draw() about what we did here
	return TRUE;
}

wxArrayInt* CLayout::GetInvalidStripArray()
{
	return &m_invalidStripArray;
}

void CLayout::CreateStrips(int nStripWidth, int gap)
{
#ifdef _ALT_LAYOUT_
    // layout is built, we should call Shrink()
	int index = 0;
	wxASSERT(!m_pileList.IsEmpty());
	CStrip* pStrip = NULL;

	 //loop to create the strips, add them to CLayout::m_stripArray
	 int maxIndex = m_pApp->GetMaxIndex(); // determined from m_pSourcePhrases list,
					// but since there is one CPile for each CSourcePhrase, the max
					// value is appropriate for m_pileList too
	int nStripIndex = 0;
	while (index <= maxIndex)
	{
		pStrip = new CStrip(this);
		pStrip->m_nStrip = nStripIndex; // set it's index
		m_stripArray.Add(pStrip); // add the new strip to the strip array
		// set up the strip's pile (and cells) contents; return the index value which is
		// to be used for the next iteration's call of CreateStrip()
		index = pStrip->CreateStrip(index, nStripWidth, gap);	// fill out with piles 
		nStripIndex++;
	}
	m_stripArray.Shrink();
#else
/*
#ifdef __WXDEBUG__
	SPList* pSrcPhrases = m_pApp->m_pSourcePhrases;
	SPList::Node* posDebug = pSrcPhrases->GetFirst();
	int index = 0;
	while (posDebug != NULL)
	{
		CSourcePhrase* pSrcPhrase = posDebug->GetData();
		wxLogDebug(_T("Index = %d   pSrcPhrase pointer  %x"),index,pSrcPhrase);
		posDebug = posDebug->GetNext();
		index++;
	}
	PileList::Node* posPDbg = m_pileList.GetFirst();
	index = 0;
	while (posPDbg != NULL)
	{
		CPile* pPile = posPDbg->GetData();
		wxLogDebug(_T("Index = %d   pPile pointer  %x"),index,pPile);
		posPDbg = posPDbg->GetNext();
		index++;
	}
#endif
*/
    // layout is built, we should call Shrink()
	int nIndexOfFirstPile = 0;
	wxASSERT(!m_pileList.IsEmpty());
	PileList::Node* pos = m_pileList.Item(nIndexOfFirstPile);
	wxASSERT(pos != NULL);
	CStrip* pStrip = NULL;

	 //loop to create the strips, add them to CLayout::m_stripArray
	int nStripIndex = 0;
	while (pos != NULL)
	{
		pStrip = new CStrip(this);
		pStrip->m_nStrip = nStripIndex; // set it's index
//#ifdef __WXDEBUG__
//	nDebugIndex = 3;
//	wxLogDebug(_T("DebugIndex = %d  CLayout, CreateStrips, CStrip pointer  %x"),nDebugIndex,pStrip);
//#endif
		m_stripArray.Add(pStrip); // add the new strip to the strip array
		// set up the strip's pile (and cells) contents
		pos = pStrip->CreateStrip(pos, nStripWidth, gap);	// fill out with piles 
		nStripIndex++;
	}
	m_stripArray.Shrink();
#endif
}

// starting from the passed in index value, update the index of succeeding strip instances
// to be in numerically ascending order without gaps
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


// the GetPile function also has equivalent member functions of the same name in the
// CAdapt_ItView and CAdapt_ItDoc classes, for convenience's sake; return the CPile
// instance at the given index, or NULL if the index is out of bounds
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
	PileList::Node* pos = m_pileList.Item(nSequNum); // relies on parallelism of 
										// m_pSourcePhrases and m_pileList lists
	wxASSERT(pos != NULL);
	CPile* pPile = pos->GetData();
	return pPile->m_pOwningStrip->m_nStrip;
}

CStrip* CLayout::GetStrip(int nSequNum)
{
	PileList::Node* pos = m_pileList.Item(nSequNum); // relies on parallelism of 
										// m_pSourcePhrases and m_pileList lists
	wxASSERT(pos != NULL);
	CPile* pPile = pos->GetData();
	return pPile->m_pOwningStrip;
}

CStrip* CLayout::GetStripByIndex(int index)
{
	return  (CStrip*)m_stripArray[index];
}

int CLayout::GetVisibleStrips()
{
	int clientHeight;
	wxSize canvasSize;
	canvasSize = m_pApp->GetMainFrame()->GetCanvasClientSize();
	clientHeight = canvasSize.GetHeight(); // see lines 2425-2435 of 
			// Adapt_ItCanvas.cpp and then lines 2454-56 for stuff below
	int nVisStrips = clientHeight / m_nStripHeight;
	int partStrip = clientHeight % m_nStripHeight; // modulo
	if (partStrip > 0)
		nVisStrips++;
	return nVisStrips; // how many strips can appear in the client area
}

void CLayout::GetVisibleStripsRange(wxDC* pDC, int& nFirstStripIndex, int& nLastStripIndex)
{
	// BE SURE TO HANDLE active sequ num of -1 --> make it end of doc, but
	// hide box  -- I think we'll support it by just recalculating the layout without a
	// scroll, at the current thumb position for the scroll car
	
    // get the logical distance (pixels) that the scroll bar's thumb indicates to top
    // of client area
	int nThumbPosition_InPixels = pDC->DeviceToLogicalY(0);

    // for the current client rectangle of the canvas, calculate how many strips will
    // fit - a part strip is counted as an extra one
	int nVisStrips = GetVisibleStrips();

	// find the current total number of strips
	int nTotalStrips = m_stripArray.GetCount();
	
 	// estimate the index to start scanning forward from in order to find the first strip
	// which has some content visible in the client area (do it by a binary chop)
	int nIndexToStartFrom = GetStartingIndex_ByBinaryChop(nThumbPosition_InPixels,
															nVisStrips, nTotalStrips);
   // find the index of the first strip which has some content visible in the client
    // area, that is, the first strip which has a bottom coordinate greater than
    // nThumbPosition_InPixels
	int index = nIndexToStartFrom;
	int bottom;
	CStrip* pStrip;
	do {
		pStrip = (CStrip*)m_stripArray.Item(index);	
		bottom = pStrip->Top() + GetStripHeight(); // includes free trans height if 
												   // free trans mode is ON 
		if (bottom > nThumbPosition_InPixels)
		{
			// this strip is at least partly visible - so start drawing at this one
			break;
		}
		index++;
	} while(index < nTotalStrips);
	wxASSERT(index < nTotalStrips);
	nFirstStripIndex = index;

    // use nVisStrips to get the final visible strip (it may be off-window, but we
    // don't care because it will be safe to draw it off window)
	nLastStripIndex = nFirstStripIndex + (nVisStrips - 1);
	if (nLastStripIndex > nTotalStrips - 1)
		nLastStripIndex = nTotalStrips - 1; // protect from bounds error

    // check the bottom of the last visible strip is lower than the bottom of the
    // client area, if not, add an additional strip
	pStrip = (CStrip*)m_stripArray.Item(nLastStripIndex);
	bottom = pStrip->Top() + GetStripHeight();
	if (!(bottom >= nThumbPosition_InPixels + GetClientWindowSize().y ))
	{
		// add an extra one, if there is an extra one to add - it won't hurt to write
		// one strip partly or wholely off screen
		if (nLastStripIndex < nTotalStrips - 1)
			nLastStripIndex++;
	}
}

int CLayout::GetStartingIndex_ByBinaryChop(int nThumbPos_InPixels, int numVisStrips, 
										   int numTotalStrips)
{
	// "upper" partition is the one with small sequence number, "lower" has larger
	// sequence numbers
	if (numTotalStrips < 64)
		// don't need a binary chop if there are not heaps of strips
		return 0;
	int maxStripIndex = numTotalStrips - 1;
	int nTwoScreensWorthOfStrips = 2*numVisStrips; // LHS = usually about 20 or so
	int nIndexToStartFrom = 0;
	CStrip* pStrip = NULL;
	int nLowerBound = nIndexToStartFrom;
	int nUpperBound = maxStripIndex;
	int nTop = 0;
	int midway = 0;
	int half = 0;
	while ((nUpperBound - nLowerBound) > nTwoScreensWorthOfStrips)
	{
		half = (nUpperBound - nLowerBound)/2;
		midway = nLowerBound + half;
		pStrip = (CStrip*)m_stripArray.Item(midway);
		nTop = pStrip->Top();
		if (nThumbPos_InPixels <= nTop)
		{
			// thumb is in upper partition
			nUpperBound = midway;
		}
		else
		{
			// thumb is in lower partition
			nLowerBound = midway;
		}
		nIndexToStartFrom = nLowerBound;
	}
	return nIndexToStartFrom;	
}

void CLayout::GetInvalidStripRange(int& nIndexOfFirst, int& nIndexOfLast,
								   int nCountValueForTooFar)
{
	// always get the active strip - it may or may not be within the strips whole indices
	// are in the m_invalidStripArray, but we'll include it in the calculations to be sure
	// the layout is correct everwhere related to the user's last editing operation (the
	// active strip may stay the active strip, or it may not, and we don't want to waste
	// time trying to figure out which is the case)
	int nIndexOfActiveStrip = -1;
	int nIndexOfFirst_Active = -1; // indicates no value was set (caller can check for != -1)
	int nIndexOfLast_Active = -1;  // ditto
	if (m_pApp->m_pActivePile != NULL)
	{
		// there will be an active pile except when the phrase box is hidden because the
		// user advanced the box into limbo past the end of the document; but if for some
		// reason the active pile's m_pOwningStrip pointer was not set, then -1 will be 
		// returned
		nIndexOfActiveStrip = m_pApp->m_pActivePile->GetStripIndex();
	}
	if (nIndexOfActiveStrip != -1)
	{
		nIndexOfFirst_Active = nIndexOfActiveStrip;
		nIndexOfLast_Active = nIndexOfActiveStrip;
	}
	int nFirst = -1;
	int nLast = -1;
	int index = 0;
	if (m_invalidStripArray.IsEmpty())
	{
		nIndexOfFirst = nIndexOfFirst_Active;
		nIndexOfLast = nIndexOfLast_Active;
		return;
	}
	else
	{
		int nCount = m_invalidStripArray.GetCount();
		wxASSERT(nCount >= 1);
		int aValue = m_invalidStripArray.Item(index);
		nFirst = aValue;
		nLast = aValue;
		for (index = 1; index < nCount; index++)
		{
			aValue = m_invalidStripArray.Item(index);
			if (aValue > nLast)
			{
				nLast = aValue;
				continue;
			}
			if (aValue < nFirst)
			{
				nFirst = aValue;
			}
		}
	}
	// now get the final range...
	if (nFirst == -1)
	{
        // the calculations with m_invalidStripArray yielded nothing, so either nothing
        // worked and so return -1 values to signal to caller to not use what is returned;
        // or we got the active strip, and return its index
		nIndexOfFirst = nIndexOfFirst_Active; // = -1 or the index of the active strip
		nIndexOfLast = nIndexOfLast_Active; // = -1 or the index of the active strip
		return;
	}
	else
	{
		// we got an index range from m_invalidStripArray, so if the active strip is
		// within it, just return that range; but if the active strip is earlier or beyond
		// it, then extend the strip range to include the active strip
		nIndexOfFirst = nFirst;
		nIndexOfLast = nLast;
		if (nIndexOfFirst_Active < nIndexOfFirst)
		{
			if ((nIndexOfFirst - nIndexOfFirst_Active) > nCountValueForTooFar)
				return; // it's too far away from the edit location for us to be bothered, 
                        // so return the values we have found already and ignore the active
                        // strip
			// otherwise, extend the index range to include the active strip
			nIndexOfFirst = nIndexOfFirst_Active;
			return;
		}
		if (nIndexOfLast_Active > nIndexOfLast)
		{
			if ((nIndexOfLast_Active - nIndexOfLast) > nCountValueForTooFar)
				return; // it's too far away from the edit location for us to be bothered, 
                        // so return the values we have found already and ignore the active
                        // strip
			// otherwise, extend the index range to include the active strip
			nIndexOfLast = nIndexOfLast_Active;
		}
	}
}

void CLayout::SetupCursorGlobals(wxString& phrase, enum box_cursor state, 
								 int nBoxCursorOffset)
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
	if (nActiveSequNum == -1)
	{
		return; // do no phrase box placement if it is hidden, as at doc end
	}
	bool bSetModify = FALSE; // initialize, governs what is done with the wxEdit 
							 // control's dirty flag
	bool bSetTextColor = FALSE; // initialize, governs whether or not we reset 
								// the box's text colour
	
	// obtain the TopLeft coordinate of the active pile's m_pCell[1] cell, there 
	// the phrase box is to be located
	wxPoint ptPhraseBoxTopLeft;
	CPile* pActivePile = GetPile(nActiveSequNum); // could use view's m_pActivePile 
                            // instead; but this will work even if we have forgotten to
                            // update it in the edit operation's handler
	pActivePile->GetCell(1)->TopLeft(ptPhraseBoxTopLeft);

	// get the pile width at the active location, using the value in
	// CLayout::m_curBoxWidth put there by RecalcLayout() or AdjustForUserEdits() or FixBox()
	int phraseBoxWidth = pActivePile->GetPhraseBoxGapWidth(); // returns CPile::m_nWidth
    //int phraseBoxWidth = m_curBoxWidth; // I was going to use this, but I think the best
    //design is to only use the value stored in CLayout::m_curBoxWidth for the brief
    //interval within the execution of FixBox() when a box expansion happens (and
    //immediately after a call to RecalcLayout() or later to AdjustForUserEdits()), since
    //then the CalcPhraseBoxGapWidth() call in RecalcLayout() or in AdjustForUserEdits()
    //will use that stored value when gbExpanding == TRUE to test for largest of
    //m_curBoxWidth and a value based on text extent plus slop, and use the larger -
    //setting result in m_nWidth, so the ResizeBox() calls here in PlacePhraseBoxInLayout()
    //should always expect m_nWidth for the active pile will have been correctly set, and
    //so always use that for the width to pass in to the ResizeBox() call below.

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

    // make the phrase box size adjustments, set the colour of its text, tell it where it
    // is to be drawn. ResizeBox doesn't recreate the box; it just calls SetSize() and
    // causes it to be visible again; CPhraseBox has a color variable & uses reflected
    // notification
	if (gbIsGlossing && gbGlossingUsesNavFont)
	{
		m_pView->ResizeBox(&ptPhraseBoxTopLeft, phraseBoxWidth, GetNavTextHeight(), 
					m_pApp->m_targetPhrase, m_pApp->m_nStartChar, m_pApp->m_nEndChar, 
					pActivePile);
		m_pApp->m_pTargetBox->m_textColor = GetNavTextColor(); //was pApp->m_navTextColor;
	}
	else
	{
		m_pView->ResizeBox(&ptPhraseBoxTopLeft, phraseBoxWidth, GetTgtTextHeight(), 
					m_pApp->m_targetPhrase, m_pApp->m_nStartChar, m_pApp->m_nEndChar, 
					pActivePile);
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

// if the first strip in the doc is invalid, pBeforePile will be returned as NULL, if the
// last strip in the doc is invalid, pAfterPile will be NULL, if all strips in the
// document are invalid (eg. as when editing a one-strip document), both will be returned
// as NULL
bool CLayout::GetPilesBoundingTheInvalidStrips(int nFirstInvalidStrip, 
						int nLastInvalidStrip, CPile* pBeforePile, CPile* pAfterPile)
{
	if (nFirstInvalidStrip == -1 || nLastInvalidStrip == -1)
	{
		// an invalid range was not calculated, so reenter RecalcLayout() doing a full
		// strip destroy and recreation, so that we get a valid layout that way
		RecalcLayout(m_pApp->m_pSourcePhrases,create_strips_keep_piles);		
		m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);
		return FALSE;
	}
	// set either or both of these TRUE if the user edit area is at the doc start or doc
	// end - but beware, don't test for these strips being invalid, because they may
	// indeed be invalid, but not due to the current user edit operation - as the latter
	// may be well away from the document ends and invalid initial or final strips could
	// just be relics of an earlier editing operation that have not been made valid yet
	bool bFirstInDocIsInvalid = FALSE;
	bool bLastInDocIsInvalid = FALSE;

	// define pBeforeStrip and pAfterStrip to be strip pointers to the strip immediately
	// before the user's editing area, provided there is such a strip (NULL if there
	// isn't) and the strip immediately after the user's editing area, provided there is
	// such a strip (NULL if there isn't), respectively
	CStrip* pBeforeStrip = (CStrip*)m_stripArray.Item(nFirstInvalidStrip);
	CStrip* pAfterStrip = (CStrip*)m_stripArray.Last();

	// check if pBeforeStrip is the first in the doc, or pAfterStrip is the last in the
	// doc - if either is so, set the respective booleans
	int nBeforeStripIndex = pBeforeStrip->m_nStrip;
	int nAfterStripIndex = pAfterStrip->m_nStrip;
	if (nBeforeStripIndex == 0)
	{
		bFirstInDocIsInvalid = TRUE;
		pBeforePile = NULL; // there is no "before" pile, edits start at doc start
	}
	if (nAfterStripIndex == (int)(m_stripArray.GetCount() - 1))
	{
		bLastInDocIsInvalid = TRUE;
		pAfterPile = NULL; // there is no "after" pile, edits go to the doc's end
	}

	// get the pBeforePile value, providing there is a previous strip - a sufficient
	// condition for the latter qualification is that bFirstInDocIsInvalid is FALSE
	if (!bFirstInDocIsInvalid)
	{
		--nBeforeStripIndex; // this is the index for the valid strip immediately
							 // before the start of the area where user did editing
		pBeforeStrip = (CStrip*)m_stripArray.Item(nBeforeStripIndex);
		wxASSERT(pBeforeStrip);
		int nIndexToLastPileInStrip = pBeforeStrip->m_arrPiles.GetCount() - 1;
		pBeforePile = (CPile*)pBeforeStrip->m_arrPiles.Item(nIndexToLastPileInStrip);
		wxASSERT(pBeforePile);
	}

	// get the pAfterPile value, providing there is a following strip - a sufficient
	// condition for the latter qualification is that bLastInDocIsInvalid is FALSE
	if (!bLastInDocIsInvalid)
	{
		++nAfterStripIndex; // this is the index for the valid strip immediately
							 // after the end of the area where user did editing
		pAfterStrip = (CStrip*)m_stripArray.Item(nAfterStripIndex);
		wxASSERT(pAfterStrip);
		pAfterPile = (CPile*)pAfterStrip->m_arrPiles.Item(0);
		wxASSERT(pAfterPile);
	}
	return TRUE;
}

// Returns: a count of how many strips were emptied
// This function empties the pile pointers stored in the invalid strips, and the array
// containing the offsets from the strip's boundary (left for LTR text, right for RTL
// text), but it does not alter any of the other strip members except to reset the m_nFree
// member to the nStripWidth value ready for pile points to be added later
int CLayout::EmptyTheInvalidStrips(int nFirstStrip, int nLastStrip, int nStripWidth)
{
	int nCount = 0;
	int index;
	CStrip* pStrip = NULL;
	for (index = nFirstStrip; index <= nLastStrip; index++)
	{
		pStrip = (CStrip*)m_stripArray.Item(index);
		wxASSERT(pStrip);
		pStrip->m_nFree = nStripWidth;
		if (!pStrip->m_arrPiles.IsEmpty())
		{
			pStrip->m_arrPiles.Clear();
		}
		if (!pStrip->m_arrPiles.IsEmpty())
		{
			pStrip->m_arrPiles.Clear();
		}
		nCount++;
	}
	return nCount;
}

//***************************************************************************************
// / \return   a count of the final number of rebuild strips  -  this could be fewer, the
// /           same, or more than the value of nInitialStripCount passed in
// / \param
// / nFirstStrip   <-> input: ref to index of first invalid strip (it is now emptied of piles)
// /                   output: same as value input, refilling doesn't affect this value
// / nLastStrip    <-> input: ref to index of last invalid strip (it is now emptied of piles)
// /                   output: index of last rebuilt strip (it could be less than, the same,
// /                   or greater than what was input - depending on whether pile widths
// /                   have changed, and how many there are to be placed
// / nStripWidth   ->  used for initializing m_nFree member of CStrip - we will need this
// /                   if we have to create one or more extra strips to accomodate all the
// /                   new CPile pointers to be placed in the rebuilt strips
// / gap           ->  the interpile gap (in pixels) - we need this for filling the emptied
//                     strips with the appropriate gap between each pile
// / pBeforePile   ->  pointer to the CPile pointer for the pile in CLayout::m_pileList which
// /                   is the last in the (valid) strip immediately preceding the user's editing
// /                   area; but if the user's editing area includes the first strip of the
// /                   document, it will be NULL that is passed in (we use this as a flag)
// / pAfterPile   ->   pointer to the CPile pointer for the pile in CLayout::m_pileList which
// /                   is the first in the (valid) strip immediately following the user's editing
// /                   area; but if the user's editing area includes the last strip of the
// /                   document, it will be NULL that is passed in (we use this as a flag)
// / posBegin     ->   iterator specifying the beginning position in m_pileList for the
// /                   first CPile pointer to be placed in the rebuilt strips; but if
// /                   pBeforePile is NULL, then it too will be NULL and we'll find the
// /                   pile's position internally instead -- it will be the first in the doc
// / posEnd       ->   iterator specifying the position in m_pileList for the last CPile pointer
// /                   which needs to be placed in the rebuilt strips; but if pAfterPile is
// /                   NULL then it too will be NULL and we'll use this fact to just place
// /                   all the remaining piles as the ending strips of the document and if
// /                   last strip is unfilled, we set m_bValid = TRUE anyway, because we
// /                   at the document's end
// / pos          ->   the iterator which traverses m_pileList from the first CPile
// /                   pointer to be placed, to the one at posEnd, inclusive
// / nInitialStripCount    ->  count of how many strips were emptied by the last function call
// /                           in the caller; we use this to initialize the return value
// / \remarks
// / The function fills the strips with the pile pointers resulting from the user's edits,
// / adding strips if necessary, or removing empty strips left over when done. Each completed
// / strip is marked m_bValid = TRUE, but the last is marked m_bValid = TRUE only if we check
// / and find that the pAfterPile is too wide to fit in that strip, or if we are at the end
// / of the document, otherwise it is left with a FALSE value. (When Draw() sees a strip with
// / m_bValid == FALSE and the strip is in the visible area of the view, it will attempt to
// / flow one or more piles up to fill the strip, and similarly for strips lower down but 
// / visible and which become invalid because they lose one or more piles due to the upward
// / flow to fill the strip above, iterating until it comes to an off-view strip - at which
// / point it leaves that strip marked as invalid - and it remains so until at some later
// / time if becomes visible and drawn, or a full destroy all strips and rebuild is done due
// / to some operation which requires it (such as changing the font size, etc).
//***************************************************************************************
int CLayout::RebuildTheInvalidStripRange(int& nFirstStrip, int& nLastStrip, 
		int nStripWidth, int gap, CPile* pBeforePile, CPile* pAfterPile, 
		PileList::Node* posBegin, PileList::Node* posEnd, PileList::Node* pos,
		int nInitialStripCount)
{
	int nFinalStripCount = nInitialStripCount;




	return nFinalStripCount;
}


// AdjustForUserEdits() adjusts the CStrip instances in the layout, when RecalcLayout()
// was called with input enum value keep_strips_keep_piles, where the user did his last
// editing or movement of the phrase box. It includes a "fail-safe" mechanism too.
// 
// We don't try to get exactly the first and last of the edited piles delineated, that's
// too fiddly, the strips they reside in is sufficient, since a few dozen strips being
// rebuild can be done really fast. If the active strip is a ways off from where the
// editing was done, we extend the strip range to include the active strip - except when
// the active strip is more than the number of visible strips away from the editing area -
// if that is the case, we just forget about updating the old active strip because it is
// probably off-screen anyway, and the Draw() function will, if it remains invalid at the
// present time, fix this when it next becomes visible.
// 
// Return TRUE if the adjustment went without error, return FALSE if the user edits area
// was not defined and so GetPilesBoundingTheInvalidStrips() internally called
// RecalcLayout() with parameter create_strips_keep_piles in order to get a valid layout
// recalculation be destroying and recreating all the strips.
bool CLayout::AdjustForUserEdits(int nStripWidth, int gap)
{
	int nIndexWhereEditsStart = -1;
	int nIndexWhereEditsEnd = -1;
	int nVisStrips = GetVisibleStrips(); // get count of how many strips fit in client area

	// interrogate the active location and the CLayout::m_invalidStripArray to work out
	// which strips, at the last user editing operation, were marked as invalid
	GetInvalidStripRange(nIndexWhereEditsStart, nIndexWhereEditsEnd, nVisStrips);

	// use the invalid strip range to get the CPile pointers which bound either end, that
	// is, the last pile of the preceding (valid) strip, and the first pile of the
	// following (valid) strip. If the last strip in the document is invalid, there will
	// be no "after" bounding CPile pointer, so it will be returned as NULL. The "after"
	// pointer is crutial - it is found in the strip array, and being a copy of a pile
	// pointer in CLayout::m_pileList, by searching in the latter we can determine a
	// location where we know the user's edits have not changed anything at that point and
	// beyond - it is this fact alone which gives us a bound for halting the strip so as
	// to keep strip updates of short duration - otherwise we'd be updating strips to the
	// end of the document at every user edit.
	CPile* pBeforePile = NULL;
	CPile* pAfterPile = NULL;
	bool bWasSuccessful = GetPilesBoundingTheInvalidStrips(nIndexWhereEditsStart, 
											nIndexWhereEditsEnd, pBeforePile, pAfterPile);
	if (!bWasSuccessful)
		return FALSE; // enable caller to inform RecalcLayout() to exit early because layout 
					  // was done by reentering RecalcLayout() within 
					  // GetPilesBoundingTheInvalidStrips(), and the active pile reset too

	// we now have pBeforePile and pAfterPile set, remember pAfterPile will be NULL if the
	// last strip in the document was marked invalid and that strip was at the start of
	// the user's current editing location; and / or pBeforePile will be NULL if
	// the first strip in the document was marked invalid and that strip was at the end of
	// the user's current editing location

	// define an iterator for the m_pileList list, and determine the iterator values for
	// the bounding piles, pBeforePile and pAfterPile, (leave NULL if the pile pointer was
	// NULL) and define some local booleans to simplify keeping track 
	// of what situation we have
	PileList::Node* posBegin = NULL;
	PileList::Node* posEnd = NULL;
	PileList::Node* pos = NULL;
	bool bEditsGoToDocStart = TRUE;
	bool bEditsGoToDocEnd = TRUE;
	if (pBeforePile != NULL)
	{
		posBegin = m_pileList.Find(pBeforePile);
		wxASSERT(posBegin);
		bEditsGoToDocStart = FALSE;
	}
	if (pAfterPile != NULL)
	{
		posEnd = m_pileList.Find(pAfterPile);
		wxASSERT(posEnd);
		bEditsGoToDocEnd = FALSE;
	}
	pos = posBegin; // either NULL, or a valid iterator value

	// now empty the invalid strips - these are always contiguous and in a block, not a
	// discontinuous set; the m_arrPiles, and m_arrPileOffsets arrays are cleared, and the
	// m_nFree member reinitialized to the nStripWidth value, and a count of how many were
	// cleared is returned. Other members of the strip are not changed.
	int nHowManyStrips = EmptyTheInvalidStrips(nIndexWhereEditsStart, 
												nIndexWhereEditsEnd, nStripWidth);	

	// now rebuild the emptied strips, from the sublist of CPile pointers defined by the
	// posBegin and posEnd values, or by doc start or end if one or both is NULL,
	// respectively. Return a count of how many strips comprise the rebuilt area in the
	// strip array. The last rebuilt strip will usually be marked m_bValid = FALSE. (It is
	// the Draw() function's job to flow piles up as necessary to fill any strips in the
	// visible range that are marked as invalid still.)
	int nHowManyNewStrips = RebuildTheInvalidStripRange(nIndexWhereEditsStart, 
								nIndexWhereEditsEnd, nStripWidth, gap, pBeforePile, 
								pAfterPile, posBegin, posEnd, pos, nHowManyStrips);

	return TRUE;
}




