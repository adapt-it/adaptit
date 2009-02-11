// Deprecation List..... get rid of these later....
/*
bool gbSuppressLast
bool gbSuppressFirst
int curRows     in param lists only?
int m_curPileHeight     on the app






*/

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Layout.cpp
/// \author			Bruce Waters
/// \date_created	09 February 2009
/// \date_revised	
/// \copyright		2009 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
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

#include "Adapt_It.h"
#include "Cell.h"
#include "Pile.h"
#include "Strip.h"
#include "Adapt_ItDoc.h"
#include "AdaptitConstants.h"
#include "SourcePhrase.h"
#include "Adapt_ItView.h"
#include "MainFrm.h"
#include "Layout.h"

// Define type safe pointer lists
#include "wx/listimpl.cpp"

/// This macro together with the macro list declaration in the .h file
/// complete the definition of a new safe pointer list class called PileList.
WX_DEFINE_LIST(PileList);

/// This macro together with the macro list declaration in the .h file
/// complete the definition of a new safe pointer list class called StripList.
WX_DEFINE_LIST(StripList);

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
	m_pPiles = NULL;
	m_pStrips = NULL;

	// *** TODO ***   add more basic initializations above here - only stuff that makes the
	// session-persistent m_pLayout pointer on the app class have the basic info it needs,
	// other document-related initializations can be done in SetupLayout()
}


void CLayout::Draw(wxDC* pDC)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = pApp->GetView();
	wxASSERT(pView != NULL);


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

// support for defining range of visible strips 
int CLayout::GetFirstVisibleStrip()
{
	return m_nFirstVisibleStrip;
}

int CLayout::GetLastVisibleStrip()
{
	return m_nLastVisibleStrip;
}

void CLayout::SetFirstVisibleStrip(int nFirstVisibleStrip)
{
	m_nFirstVisibleStrip = nFirstVisibleStrip;
}

void CLayout::SetLastVisibleStrip(int nLastVisibleStrip)
{
	m_nLastVisibleStrip = nLastVisibleStrip;
}

// accessors for navText colour
void CLayout::SetNavTextColor(CAdapt_ItApp* pApp)
{
	m_navTextColor = pApp->m_navTextColor;
}

wxColour CLayout::GetNavTextColor()
{
	return m_navTextColor;
}

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

// left margin for strips
void CLayout::SetCurLMargin(CAdapt_ItApp* pApp)
{
	m_nCurLMargin = pApp->m_curLMargin;
}

int CLayout::GetCurLMargin()
{
	return m_nCurLMargin;
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
	// the pile height is now set; so in the stuff below, set the strip height too -
	// it is the current leading value (i.e. the nav text whiteboard height) plus the
	// m_nPileHeight value, =/-, depending on whether free translation mode is on or not, the
	// target text line height plus 3 pixels of separating space from the bottom of the pile
	m_nStripHeight = m_nCurLeading + m_nPileHeight;
	if (gpApp->m_bFreeTranslationMode && !gbIsPrinting)
	{
        // add enough space for a single line of the height given by the target text's height + 3
        // pixels to set it off a bit from the bottom of the pile
		m_nStripHeight += m_nTgtHeight + 3;
	}
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

int CLayout::GetStripLeft()
{
	return m_nCurLMargin;
}


wxSize CLayout::GetClientWindowSize()
{
	return m_sizeClientWindow;
}

wxSize CLayout::GetLogicalDocSize()
{
	return m_logicalDocSize;
}




bool CLayout::CreatePiles(PileList* pPiles, SPList* pSrcPhrases)
{
	// we expect pSrcPhrases list is populated, so if it is not we
	// treat that as a serious logic error
	if (pSrcPhrases == NULL || pSrcPhrases->IsEmpty())
		return FALSE;

	// create the pile list if the pile list pointer is NULL, or if not,
	// if the list is presently not populated
	if (pPiles == NULL || (pPiles != NULL && pPiles->IsEmpty()))
	{
		// there either is no list on the heap yet, or there is but it
		// has nothing in it - in either case we can go ahead
		CSourcePhrase* pSrcPhrase = NULL;
		SPList::Node* pos = pSrcPhrases->GetFirst();
		while (pos != NULL)
		{
			pSrcPhrase = (CSourcePhrase*)pos->GetData();
			wxASSERT(pSrcPhrase != NULL);
			pos = pos->GetNext(); // jump to next Node

// *** TODO *** pile creation etc -- can't go further here until I have pinned down
// the CPile's attributes in the new design -- do that now



		}
		return TRUE;
	}
	// oops, there must be a list on the heap and it is populated, we must
	// first explicitly depopulate it and delete it's CPile instances
	// before we re-create the list from scratch -- redesign called for
	wxMessageBox(_T("Error: trying to recreate m_pPiles list when it is not empty (in CLayout)"),_T(""), wxICON_ERROR);
	wxASSERT(FALSE); // we can't recover, this is a major design fault
	return FALSE; // compiler will complain if this is absent, & release version needs it
}

// return TRUE if a layout was set up, or if no layout can yet be set up;
// but return FALSE if a layout setup was attempted and failed (app must then abort)
// 
// Note: SetupLayout() should be called, for best results, immediately before opening,
// or creating, an Adapt It document. At that point all of the paramaters it collects
// together should have their "final" values, such as font choices, colours, etc.
bool CLayout::SetupLayout(SPList* pSrcPhrases)
{
	if (pSrcPhrases == NULL || pSrcPhrases->IsEmpty())
	{
		// no document is loaded, so no layout is appropriate yet, do nothing
		// except ensure m_pPiles is NULL
		if (m_pPiles != NULL)
		{
			if (m_pPiles->IsEmpty())
			{
				delete m_pPiles;
			}
			else
			{
				m_pPiles->DeleteContents(TRUE); // TRUE means "delete the stored CCell instances too"
				delete m_pPiles;
			}
			m_pPiles = NULL;
		}
		return TRUE;
	}
	// attempt the layout setup
	bool bIsOK = CreatePiles(m_pPiles, pSrcPhrases);
	if (!bIsOK)
	{
		// something was wrong - memory error or perhaps m_pPiles is a populated list already
		// (CreatePiles()has generated an error message for the developer already)
		return FALSE;
	}

	// set the local private copy of the navText colour
	SetNavTextColor(m_pApp);

	// set the text heights for src, tgt, navText private members
	SetSrcTextHeight(m_pApp);
	SetTgtTextHeight(m_pApp);
	SetNavTextHeight(m_pApp);

	// set the m_bShowTargetOnly flag (controls whether or not to suppress src text line)
	// SetShowTargetOnlyBoolean();  // nah, don't bother to fiddle with the globals yet





// *** TODO **** more setup stuff goes here

	return TRUE;
}








