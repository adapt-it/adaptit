// Deprecation List..... get rid of these later....
/*
bool gbSuppressLast
bool gbSuppressFirst
int curRows     in param lists only?
int m_curPileHeight     on the app
?? int m_nCurPileMinWidth  on the app ?? no, used scores of times, for resetting box width, m_curBoxWidth 
						often sets it - eg. returned from RecalcPhraseBoxWidth(); so leave the phr box
						mechanisms untouched at this time.
gnSaveGap and gnSaveLeading are replaced by m_nSaveGap and m_nSaveLeading in CLayout





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
#include "Layout.h"


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
	//m_pPiles = NULL;
	//m_pStrips = NULL;
	m_stripArray.Clear();

	// *** TODO ***   add more basic initializations above here - but only stuff that makes the
	// session-persistent m_pLayout pointer on the app class have the basic info it needs,
	// other document-related initializations can be done in SetupLayout()
}


void CLayout::Draw(wxDC* pDC)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = pApp->GetView();
	wxASSERT(pView != NULL);

	// *** TODO ***  the rest of it...
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

int CLayout::SetLogicalDocHeight()
{
	int nStripCount = m_stripArray.GetCount();
	int nDocHeight = (GetCurLeading() + GetStripHeight()) * nStripCount;
	nDocHeight += 40; // pixels for some white space at document's bottom
	m_logicalDocSize.SetHeight(nDocHeight);
	return nDocHeight;
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
/* CLayout, CStrip, CPile & CCell are mutual friends, so we'll grab m_nCurGapWidth directly
int CLayout::GetGapWidth()
{
	return m_nCurGapWidth;
}
*/





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
	//PileList::Node* pos = m_pPiles->Item(nFirstPile);
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
		pPile = (CPile*)pos->GetData();
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
CPile* CLayout::CreatePile(CLayout* pLayout, CSourcePhrase* pSrcPhrase)
{
	wxASSERT(pSrcPhrase != NULL);
	wxASSERT(pLayout != NULL);

	// create an empty pile on the heap
	CPile* pPile = new CPile;
	wxASSERT(pPile != NULL); // if tripped, must be a memory error

	// assign the passed in source phrase pointer to its m_pSrcPhrase public member,
	// and also the pointer to the CLayout object
	pPile->m_pSrcPhrase = pSrcPhrase;
	pPile->m_pLayout = pLayout;

	// set (in pixels) its minimum width based on which of the m_srcPhrase, m_adaption and m_gloss
	// strings within the passed in CSourcePhrase is the widest; the min width member is thereafter
	// a lower bound for the width of the phrase box when the latter contracts, if located at this
	// pile while the user is working
	pPile->m_nMinWidth = pPile->CalcPileWidth(); // if an empty string, it is defaulted to 10 pixels
	pPile->m_nWidth = pPile->m_nMinWidth; // a default value for starters, user edits may increase it later

	// *** TODO? *** the mechanism for phrase box support (which we are not currently refactoring) uses
	// two members on the app class as follows:
	// 	m_nCurPileMinWidth and	pApp->m_curBoxWidth
	// 	Typically these are used as follows, when building strips, etc, at the active pile a larger gap is
	// 	left for the phrase box, it's width defaults to the m_nCurPileMinWidth value, then the CSourcePhrase
	// 	is checked to see if it's extent is wider than m_nCurPileMinWidth, and if so,
	// 	m_curBoxWidth is set to that wider extent, otherwise it is set to m_nCurPileMinWidth. When
	// 	refactoring the code for strips, we must ensure that the strip building process supports
	// 	these two variables in the way expected. "support" here just means "have a correct value
	// 	for the m_nMinWidth member in the CPile object. (DoTargetBoxPaste() is a good example of use)
	 	
	// pile creation always creates the (fixed) array of CCells which it manages
	int index;
	CCell* pCell = NULL;
	for (index = 0; index < MAX_CELLS; index++)
	{
		pCell = new CCell();
		pCell->CreateCell(pLayout,pPile,index);
	}
	// Note: to assist clarity, the extensive commented out legacy code has been removed
	// from here and placed temporarily at the end of this file
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
		pPile = CreatePile(this,pSrcPhrase);
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
// creating, an Adapt It document. At that point all of the paramaters it collects together should
// have their "final" values, such as font choices, colours, etc. It is also to be called whenever
// the layout has been corrupted by a user action, such as a font change (which clobbers prior text
// extent values stored in the piles) or a punctuation setting change, etc. RecalcLayout() in the
// refactored design is a potentially "costly" calculation - if the document is large, the piles
// and strips for the whole document has to be recalculated from the data in the current document -
// (need to consider a progress bar in the status bar in window bottom)
bool CLayout::RecalcLayout(bool bRecreatePileListAlso)
{
    // RecalcLayout() is the refactored equivalent to the former view class's RecalcLayout()
    // function - the latter built only a bundle's-worth of strips, but the new design must build
	// strips for the whole document - so potentially may consume a lot of time; however, the
	// efficiency of the new design (eg. no rectangles are calculated) may compensate significantly

	SPList* pSrcPhrases = m_pApp->m_pSourcePhrases; // the list of CSourcePhrase instances which
	// comprise the document -- the CLayout instance will own a parallel list of CPile
	// instances in one-to-one correspondence with the CSourcePhrase instances, and each of piles
	// will contain a pointer to the sourcePhrase it is associated with
	
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

	// //////// first get up-to-date-values for all the needed data /////////

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

	// estimate the number of CStrip instances required - assume an average of 16, which should
	// result in more strips than needed, and so when the layout is built, we should call Shrink()
	int aCount = pSrcPhrases->GetCount();
	int anEstimate = aCount / 16;
	m_stripArray.SetCount(anEstimate,(void*)NULL);	// create possibly enough space in the array 
													// for all the piles
	//int gap = m_nCurGapWidth; // distance in pixels for interpile gap
	int nStripWidth = (GetLogicalDocSize()).x; // constant for any one RecalcLayout call
	int nPilesEndIndex = m_pileList.GetCount() - 1; // use this to terminate the strip creation loop


	// *** TODO **** code (a loop) to build the strips goes here 

	m_stripArray.Shrink();

	// the height of the document can now be calculated
	int nLogicalDocumentHeightInPixels = SetLogicalDocHeight();


	// *** TODO ***
	// set up the scroll bar to have the correct range (and it would be nice to try place the
	// phrase box at the old active location if it is still within the document, etc) -- see the
	// list of things done in the legacy CAdapt_ItView's version of this function (lines 4470++)
	
	
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
