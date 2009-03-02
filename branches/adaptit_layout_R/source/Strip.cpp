/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Strip.cpp
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public
///  License (see license directory)
/// \description	This is the implementation file for the CStrip class. 
/// The CStrip class represents the next smaller divisions of a CBundle.
/// Each CStrip stores an ordered list of CPile instances, which are
/// displayed in LtoR languages from left to right, and in RtoL languages
/// from right to left.
/// \derivation		The CStrip class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. Test to insure Draw works correctly
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Strip.h"
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
//#include "SourceBundle.h"
#include "MainFrm.h"
#include "AdaptitConstants.h"
// don't mess with the order of the following includes, Strip must precede View must precede
// Pile must precede Layout and Cell can usefully by last
#include "Strip.h"
#include "Adapt_ItView.h"
#include "Pile.h"
#include "Layout.h"
#include "Cell.h"



// Define type safe pointer lists
#include "wx/listimpl.cpp"

/// This macro together with the macro list declaration in the .h file
/// complete the definition of a new safe pointer list class called StripList.
WX_DEFINE_LIST(StripList);


extern bool gbIsPrinting; // whm added because wxDC does not have ::IsPrinting() method
extern CAdapt_ItApp* gpApp;

extern wxRect	grectViewClient; // used in OnDraw() below
extern int		gnCurPage;
extern bool		gbRTL_Layout;

/// This global is defined in Adapt_It.cpp.
extern struct PageOffsets pgOffsets;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(CStrip, wxObject)

CStrip::CStrip()
{
	m_pLayout = NULL;
	m_nFree = 0;
	m_nStrip = 0;
	m_bFilled = FALSE;
	m_bAffected = FALSE;
	/*
	m_nVertOffset = 0;
	m_nPileCount = 0;
	// m_nPileHeight = 20;  // BEW deprecated 3Feb09
	m_nStripIndex = 0;
	m_nFree = 0;
	for (int i = 0; i<MAX_PILES; i++)
		m_pPile[i] = (CPile*)NULL;
	*/
}

//CStrip::CStrip(CAdapt_ItDoc* pDocument, CSourceBundle* pSourceBundle) // BEW deprecated 3Feb09
/*
CStrip::CStrip(CSourceBundle* pSourceBundle)
{
	//m_pDoc = pDocument; // BEW deprecated 3Feb09
	m_pBundle = pSourceBundle;
	m_nVertOffset = 0;
	m_nPileCount = 0;
	// m_nPileHeight = 20; // BEW deprecated 3Feb09
	m_nStripIndex = 0;
	m_nFree = 0;
	m_rectStrip = wxRect(0,0,0,0);
	m_rectFreeTrans = wxRect(0,0,0,0); // BEW added 24Jun05, for free translation support
	for (int i = 0; i<MAX_PILES; i++)
		m_pPile[i] = (CPile*)NULL;
}
*/
CStrip::~CStrip()
{

}
/*
void CStrip::DestroyPiles()
{
	//  *** TODO *** probably rename to RemovePiles & change code (don't destroy, layout manages them)
	int count = m_nPileCount;
	wxASSERT(count <= MAX_PILES);
	for (int i=0; i<count; i++)
	{
		m_pPile[i]->DestroyCells();
		delete m_pPile[i];
		m_pPile[i] = (CPile*)NULL;
	}
}
*/
void CStrip::Draw(wxDC* pDC)  // *** TODO *** additional parameters needed for refactored version
{


	/*
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	if (gbIsPrinting)
	{
		// whm Note: The pOffsets members nTop and nBottom were negative in the MFC version, but remain
		// positive in the wx version.
		POList* pList = &pApp->m_pagesList;
		POList::Node* pos = pList->Item(gnCurPage-1);
		PageOffsets* pOffsets = (PageOffsets*)pos->GetData();
		if (m_nStripIndex < pOffsets->nFirstStrip || m_nStripIndex > pOffsets->nLastStrip)
			return;

		for (int i=0; i< m_nPileCount; i++)
		{
			m_pPile[i]->Draw(pDC);
		}
	}
	else
	{
		// BEW changed 7Jul05; moved original block, which only needs to be done once, to view's OnDraw() function

		wxRect r;
		r = grectViewClient;
		wxRect rectTest;
		rectTest = m_rectStrip;
		rectTest.SetY(rectTest.GetY()-pApp->m_curLeading); // move the top-left point up by m_curLeading
		// wx note: The SetY() above does not change the height in wx version as in MFC version, so must
		// increment the height of rectText also
		rectTest.SetHeight(rectTest.GetHeight() + pApp->m_curLeading); // deflate the rect size in the y direction by m_curLeading
		//if (r.IntersectRect(&r,&rectTest))
		// The intersection is the largest rectangle contained in both existing 
		// rectangles. Hence, when IntersectRect(&r,&rectTest) returns TRUE, it
		// indicates that there is some client area to draw on.
#ifdef _LOG_DEBUG_DRAWING
		wxString logPileStr;
#endif
		if (r.Intersects(rectTest))
		{
			for (int i=0; i< m_nPileCount; i++)
			{
#ifdef _LOG_DEBUG_DRAWING
				logPileStr << i;
				logPileStr += _T(' ');
#endif
				m_pPile[i]->Draw(pDC);
			}
#ifdef _LOG_DEBUG_DRAWING
			wxLogDebug(_T(" Piles: %s"),logPileStr.c_str());
#endif
		}
	}
	*/
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     the vertical offset (a new nVertOffset) of the strip that was created
/// \param      pDC                 -> the display context for strip creation
/// \param      pSrcList            -> the list of source phrases from which strips are composed
/// \param      nVertOffset         -> the starting vertical offset for the strip being created
/// \param      nLastSequNumber     <- the sequence number (gets incremented by one and returned)
/// \param      nEndIndex           -> the sequence number upper limit
/// \remarks
/// Called from: the View's RecalcLayout() and LayoutStrip().
/// CreateStrip() calculates and creates a new strip for display within the main window, or on a
/// printer or print preview display context. It uses the layout parameters and indices that were
/// established by a previous call of RecalcLayout(). CreateStrip also calls CreatePile() for each 
/// pile that will fit within the strip. See CreateStrip_SimulateOnly() for a related function 
/// which only simulates the calculations made by CreateStrip().
////////////////////////////////////////////////////////////////////////////////////////////
//int CStrip::CreateStrip(wxClientDC *pDC, SPList* pSrcList, int nVertOffset, int &nLastSequNumber, int nEndIndex)



void CStrip::CreateStrip(CLayout* pLayout, int nStripWidth, int nIndexOfFirstPile)
{
	m_nFree = nStripWidth;

	// prepare for iterating over next group of CPiles to be placed in the strip
	PileList* pPiles = pLayout->m_pPiles;
	wxASSERT(!pPiles->IsEmpty());
	CAdapt_ItView* pView = pLayout->m_pView; // for calling IsWrapMarker() on a pile's CSourcePhrase pointer
	PileList::Node* pos = pPiles->Item(nIndexOfFirstPile);
	wxASSERT(pos != NULL);
	CPile* pPile = (CPile*)pos->GetData();

	// *** TODO *** the rest... prepare m_arrPiles by preallocating space - default 20?, etc





/*
// MFC Note: returns the nVertOffset value for the strip, if it creates it, or -1 if it doesn't get created
// nVertOffset will be +ve for MM_Text mapping mode, and -ve for other modes, such as MM_LOENGLISH
// the global flag gbIsPrinting selects which will be the case, TRUE for a y axis increasing
// upwards pDC will be set to MM_LOENGLISH if the gbIsPrinting flag is true, else it will be
// MM_TEXT.
// 
// whm Note: CreateStrip returns positive offsets even when printing, and in the wx version we always
// use wxMM_TEXT map mode.
// 
// BEW 12Jul05 - I don't think WYSIWYG printing of the free translation mode display is worth the
// bother - the user should get it with the interlinear export option instead, so I'll suppress
// free translation support in CreateStrip if gbIsPrinting is TRUE.
{
	wxASSERT(nLastSequNumber >= -1);
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	CAdapt_ItView* pView = pApp->GetView();
	// CAdapt_ItDoc* pDoc = gpApp->GetDocument(); // BEW deprecated 3Feb09
	int nextNumber = nLastSequNumber;
	nextNumber++; // index of first CSourcePhrase element to be placed
	wxRect rectStrip;
	wxPoint topLeft;
	wxPoint botRight;

	CSourceBundle* pBundle = pApp->m_pBundle;
	int stripIndex = pBundle->m_nStripIndex; // index of current strip
	//pBundle->m_pStrip[stripIndex] = new CStrip(pDoc,pBundle); // BEW deprecated 3Feb09
	//pBundle->m_pStrip[stripIndex] = new CStrip(pBundle); // BEW 9Feb09 moved to caller
	//wxASSERT(pBundle->m_pStrip[stripIndex] != NULL);
	CStrip* pStrip = pBundle->m_pStrip[stripIndex]; // get a convenient local pointer to it

	if (gbRTL_Layout)
	{
		// Unicode version, and Right to Left layout is wanted

		// calculate the strip's rectangle
		// wx note: the wx version does not use negative offsets, but uses the same code whether
		// gbIsPrinting is TRUE or FALSE. Also when creating a wxRect from two points, the constructor
		// normalizes the wxRect.
		topLeft = wxPoint(0, nVertOffset + pApp->m_curLeading);
		botRight = wxPoint(pApp->m_docSize.GetWidth() - pApp->m_curLMargin,
											topLeft.y + pApp->m_curPileHeight);
		pStrip->m_rectStrip = wxRect(topLeft,botRight);

		// BEW added next block 24Jun05 for free translation support - if the first flag is TRUE, and
		// we are not currently printing, then compute the m_rectFreeTrans rectangle for this strip in
		// which any free translation or part thereof can be written out
		if (gpApp->m_bFreeTranslationMode && !gbIsPrinting)
		{
			pStrip->m_rectFreeTrans = pStrip->m_rectStrip; // copy strip rect, only top is wrong
			pStrip->m_rectFreeTrans.x -= RH_SLOP/5; // there was RH_SLOP/4 (10 pixels) left margin, 
													// so use some (8 pixels) of it
			// wx note: in the line above we're decreasing the left coord of an existing rect resulting in 
			// an increase in the width of the rectangle (in MFC's CRect), so in wx we need to INCREASE 
			// the width of m_rectFreeTrans by the same amount, to achieve the same results as in the 
			// MFC version with the addition of the following line:
			pStrip->m_rectFreeTrans.width += RH_SLOP/5; // whm added for wx
			if (gbIsPrinting)
			{
				// adjust the top member (this block never used, unless we later decide to support
				// printing the free translation mode display)
				pStrip->m_rectFreeTrans.y = pStrip->m_rectStrip.GetBottom() + pApp->m_nTgtHeight;
				// wx note: In the MFC version this block can never be entered when !gbIsPrinting
				// TODO: if printing the free translation mode display is implemented
				// the m_rectFreeTrans height would need adjusting (see non-printing block below)
			}
			else
			{
				// make the top be a tgt text line height above the strip bottom - this also leaves
				// an extra 3 pixels of space below last pile, the 3 pixels was added in SetPileHeight()
				pStrip->m_rectFreeTrans.y = pStrip->m_rectStrip.GetBottom() - pApp->m_nTgtHeight;
				// wx notes:
				// 1. GetBottom() actually returns one pixel less than MFC's .bottom value, but it 
				// shouldn't matter that much.
				// 2. The effect of setting the y coord changes both the position of the rect
				// and its height in the MFC version. In the WX version it only changes the original
				// rect's position. To do the equivalent changes in the wx version, we need to also
				// adjust the height of the free trans rect to be one target line height by adding
				// the following line in the wx version:
				pStrip->m_rectFreeTrans.SetHeight(pApp->m_nTgtHeight);
				// Observation: the actual height of the m_rectFreeTrans rect is also significant for 
				// drawing free translation, because in DrawFreeTranslations, the wx version calls 
				// pDC->Clear() on a clipped rectangle (pElement->subRect) which is based on the 
				// m_rectFreeTransatext rectangular area. If m_rectFreeTrans is not the correct height
				// the clipping region will cause the top part of the next strip to be wrongly clipped.
			}
		}

		rectStrip = pStrip->m_rectStrip; // get a convenient local copy
		//pStrip->m_nPileHeight = pApp->m_curPileHeight;	// store pile height (I don't think I'll need
														// to use this)  // BEW deprecated 3Feb09

		// create the piles which belong in this strip
		int pileIndex = 0;
		int horzOffset = 0; // from start of strip rectangle to right edge of last pile (followed
							// by gap) (or to left edge of last pile if laying out right to left)
							// for RTL layout horzOffset measures offset in pixels from the right
							// of the strip's rectangle
		pStrip->m_nPileCount = 0; // initialize
		wxRect rectPile;
		while (pileIndex < MAX_PILES && nextNumber <= nEndIndex)
		{
			SPList::Node* pos = pSrcList->Item(nextNumber); // POSITION of the CSourcePhrase for
															// this pile
			CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
			wxASSERT(pSrcPhrase != NULL);

			// cause source phrases with standard format markers to begin a new strip
			if (pApp->m_bMarkerWrapsStrip) // set by menu item on View menu, defaut is TRUE
			{
				if (!pSrcPhrase->m_markers.IsEmpty())
				{
					// this a potential candidate for starting a new strip, so check it out
					if (pSrcPhrase->m_nSequNumber > 0 && pileIndex > 0)
					{
						if (pView->IsWrapMarker(pSrcPhrase))
							break;
					}
				}
			}

			// work out the pile's rectangle, this involves measuring text extents to get a max
			// width for all the phrases in the pile
			int pileWidth = pView->CalcPileWidth(pDC,pApp,pSrcPhrase);

			// check there is room for this next pile & following gap - if not, break out of the
			// loop; but we must allow at least one pile, even if it is too long, otherwise we
			// will not advance through the list of source phrases, and just create empty strips
			// until we get a bounds error
			bool bNeedOne = FALSE;
			// wx note: we'll use rectStrip.GetWidth() instead of .right - left
			if (horzOffset + pApp->m_curGapWidth + pileWidth > rectStrip.GetWidth())
			{
				if (pileIndex == 0)
				{
					// we must have at least this one
					bNeedOne = TRUE;
					goto c;
				}
				else
				{
					break; // exit the loop, this strip is full
				}
			}

			// set the rectPile for this pile instance (uses logical coordinates)
c:			topLeft = wxPoint(rectStrip.GetRight() - horzOffset - pileWidth, rectStrip.GetTop());
			botRight = wxPoint(rectStrip.GetRight() - horzOffset, rectStrip.GetBottom());
			// wx note: creating the rectPile from the two points avoids height adjustments, but
			// CAUTION: if coordinates of the points are negative (as when printing/previewing), the result
			// may be a different spatial position for the wxRect than MFC has for its CRect created with the
			// same negative coordinates!!!
			rectPile = wxRect(topLeft,botRight);

			CPile* pPile = new CPile(pBundle,pStrip,pSrcPhrase);
			wxASSERT(pPile != NULL);
			if (pPile == NULL)
			{
				wxString str = _T("Creating a CPile failed, probably out of memory");
				wxMessageBox(str,_T(""),wxICON_WARNING);
				wxExit();
			}
			// call a CreatePile function, passing it the rectangle, etc, and return the CPile
			// pointer for storage in the pile array
			//pStrip->m_pPile[pileIndex] = CreatePile(pDC,pApp,pDoc,pBundle,pStrip,pSrcPhrase,
			//										&rectPile); // BEW deprecated 3Feb09								
			pStrip->m_pPile[pileIndex] = pPile->CreatePile(pDC,pApp,pBundle,pStrip,pSrcPhrase,&rectPile);
			pStrip->m_pPile[pileIndex]->m_nPileIndex = pileIndex; // so the pile will know which
																  // it is
			pStrip->m_pPile[pileIndex]->m_nHorzOffset = horzOffset; // so it will know where to
																	// lay itself out again if
																	// user turns it into a phrase
			// increment sequNum, etc.
			nLastSequNumber = nextNumber;
			nextNumber++; // for next source phrase & its pile
			horzOffset += pileWidth + pApp->m_curGapWidth; // for next pile

			// increment pile index for next time through the loop, & count this pile
			pileIndex++;
			pStrip->m_nPileCount++;
			if (bNeedOne)
				goto d;
		} // end of while (...) loop
		// wx note: GetRight() below returns one pixel less that .right value in MFC, so we'll
		// simply substitute rectStrip.GetWidth()
		// matter here.
d:		pStrip->m_nFree = rectStrip.GetWidth() - horzOffset;	// slop remaining at
																// right (or left for RTL)
	}
	else
	{
		// NR or ANSI version, Left to Right layout is wanted

		// calculate the strip's rectangle
		// MFC note says, "(this block never used, unless we later decide to support
		// printing the free translation mode display)", but it is entered when printing.
		// wx note: the wx version does not use negative offsets, but uses the same code whether
		// gbIsPrinting is TRUE or FALSE. Also when creating a wxRect from two points, the constructor
		// normalizes the wxRect.
		topLeft = wxPoint(pApp->m_curLMargin, nVertOffset + pApp->m_curLeading);
		botRight = wxPoint(pApp->m_docSize.GetWidth(), topLeft.y + pApp->m_curPileHeight);
		pStrip->m_rectStrip = wxRect(topLeft,botRight);
		
		rectStrip = pStrip->m_rectStrip; // get a convenient local copy
		//pStrip->m_nPileHeight = pApp->m_curPileHeight;	// store pile height (I don't think I'll need to
														// use this)  // BEW deprecated 3Feb09

		// BEW added next block 24Jun05 for free translation support - if the flag is TRUE, then
		// compute the m_rectFreeTrans rectangle for this strip in which any free translation
		// or part thereof can be written out
		if (pApp->m_bFreeTranslationMode && !gbIsPrinting)
		{
			pStrip->m_rectFreeTrans = pStrip->m_rectStrip; // copy strip rect, only top is wrong
			//pStrip->m_rectFreeTrans.right += RH_SLOP/2; // there was RH_SLOP right margin, so use half of it
			// wx note: in the commented out MFC line above we're increasing the right bound of the
			// rectangle resulting in an increase in the width, so in wx we just need to INCREASE the width.
			// In either case the upper left point remains fixed
			pStrip->m_rectFreeTrans.width += RH_SLOP/2; // whm added for wx
			if (gbIsPrinting)
			{
				// adjust the top member (this block never used, unless we later decide to support
				// printing the free translation mode display
				pStrip->m_rectFreeTrans.y = pStrip->m_rectStrip.GetBottom() + pApp->m_nTgtHeight;
				// wx note: In the MFC version this block can never be entered when !gbIsPrinting
				// TODO: if printing the free translation mode display is implemented
				// the m_rectFreeTrans height would need adjusting (see non-printing block below)
			}
			else
			{
				// make the top be a tgt text line height above the strip bottom - this also leaves
				// an extra 3 pixels of space below last pile, the 3 pixels was added in SetPileHeight()
				pStrip->m_rectFreeTrans.y = pStrip->m_rectStrip.GetBottom() - pApp->m_nTgtHeight;
				// wx notes:
				// 1. GetBottom() actually returns one pixel less than MFC's .bottom value, but it 
				// shouldn't matter that much.
				// 2. The effect of setting the y coord changes both the position of the rect
				// and its height in the MFC version. In the WX version it only changes the original
				// rect's position. To do the equivalent changes in the wx version, we need to also
				// adjust the height of the free trans rect to be one target line height by adding
				// the following line in the wx version:
				pStrip->m_rectFreeTrans.SetHeight(pApp->m_nTgtHeight);
				// Observation: the actual height of the m_rectFreeTrans rect is also significant for 
				// drawing free translation, because in DrawFreeTranslations, the wx version calls 
				// pDC->Clear() on a clipped rectangle (pElement->subRect) which is based on the 
				// m_rectFreeTransatext rectangular area. If m_rectFreeTrans is not the correct height
				// the clipping region will cause the top part of the next strip to be wrongly clipped.
			}
		}

		// create the piles which belong in this strip
		int pileIndex = 0;
		int horzOffset = 0; // from start of strip rectangle to right edge of last pile
							// (followed by gap)
		pStrip->m_nPileCount = 0; // initialize
		wxRect rectPile;
		while (pileIndex < MAX_PILES && nextNumber <= nEndIndex)
		{
			
			//#ifdef __WXDEBUG__
			//wxLogTrace(" ### while loop nextNumber value = %d   nEndIndex = %d    ",nextNumber,
			//																		nEndIndex);
			//#endif
			SPList::Node* pos = pSrcList->Item(nextNumber); // POSITION of the CSourcePhrase
														    // for this pile
			CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
			wxASSERT(pSrcPhrase != NULL);

			// cause source phrases with certain standard format markers to begin a new strip
			if (pApp->m_bMarkerWrapsStrip) // set by menu item on View menu, default is TRUE
			{
				if (!pSrcPhrase->m_markers.IsEmpty())
				{
					// this a potential candidate for starting a new strip, so check it out
					if (pSrcPhrase->m_nSequNumber > 0 && pileIndex > 0)
					{
						if (pView->IsWrapMarker(pSrcPhrase))
							break; // if we need to wrap, discontinue pile creation in this strip
					}
				}
			}

			// work out the pile's rectangle, this involves measuring text extents to get a max
			// width for all the phrases in the pile
			int pileWidth = pView->CalcPileWidth(pDC,pApp,pSrcPhrase);

			// check there is room for this next pile & following gap - if not, break out of the
			// loop; but we must allow at least one pile, even if it is too long, otherwise we
			// will not advance through the list of source phrases, and just create empty strips
			// until we get a bounds error
			bool bNeedOne = FALSE;
			//if (horzOffset + m_curGapWidth + pileWidth > rectStrip.right - rectStrip.left)
			// wx note: horzOffset will be one pixel less width in wx version than in MFC if
			// do right - left calc, should be same if simply use GetWidth()
			if (horzOffset + pApp->m_curGapWidth + pileWidth > rectStrip.GetWidth())
			{
				if (pileIndex == 0)
				{
					// we must have at least this one
					bNeedOne = TRUE;
					goto a;
				}
				else
				{
					break; // exit the loop, this strip is full
				}
			}

			// set the rectPile for this pile instance (uses logical coordinates)
a:			topLeft = wxPoint(rectStrip.GetLeft() + horzOffset, rectStrip.GetTop());
			botRight = wxPoint(rectStrip.GetLeft() + horzOffset + pileWidth, rectStrip.GetBottom());
			rectPile = wxRect(topLeft,botRight);

			CPile* pPile = new CPile(pBundle,pStrip,pSrcPhrase);
			wxASSERT(pPile != NULL);
			if (pPile == NULL)
			{
				wxString str = _T("Creating a CPile failed, probably out of memory");
				wxMessageBox(str,_T(""),wxICON_WARNING);
				wxExit();
			}
			// call a CreatePile function, passing it the rectangle, etc, and return the CPile
			// pointer for storage in the pile array
			//pStrip->m_pPile[pileIndex] = CreatePile(pDC,pApp,pDoc,pBundle,pStrip,pSrcPhrase,
			//																		&rectPile);
			pStrip->m_pPile[pileIndex] = pPile->CreatePile(pDC,pApp,pBundle,pStrip,pSrcPhrase,&rectPile);
			pStrip->m_pPile[pileIndex]->m_nPileIndex = pileIndex; // so the pile will know which
																  // it is
			pStrip->m_pPile[pileIndex]->m_nHorzOffset = horzOffset; // so it will know where to
																	// lay itself out again if
																	// user turns it into a phrase
			// increment sequNum, etc.
			nLastSequNumber = nextNumber;
			nextNumber++; // for next source phrase & its pile
			horzOffset += pileWidth + pApp->m_curGapWidth; // for next pile

			// increment pile index for next time through the loop, & count this pile
			pileIndex++;
			pStrip->m_nPileCount++;
			if (bNeedOne)
				goto b;
		} // end of while (...) loop
b:		pStrip->m_nFree = rectStrip.GetRight() - rectStrip.GetLeft() - horzOffset; // slop remaining at right
	}

	// update variables ready for next one
	// whm: for wx version we don't use negative offsets during printing
	nVertOffset += pApp->m_curPileHeight + pApp->m_curLeading; // offset for bottom of this strip
	return nVertOffset;
	*/
}
// return the width of the strip (it's width is the maximum width it can be for the current client
// window size, less the left margin and right hand slop; the document size's .x value is already
// set to that value, so just return that; the width is NOT the width of any piles in the strip
// -- for that, use the Width() value and subtract m_nFree pixels from it)
int CStrip::Width()
{
	return (m_pLayout->GetLogicalDocSize()).GetWidth();
}

int CStrip::Height()
{
	return m_pLayout->GetStripHeight();
}

int CStrip::Left()
{
	return m_pLayout->GetStripLeft();
}

int CStrip::Top()
{
	return m_pLayout->GetCurLeading() + m_nStrip * (Height() + m_pLayout->GetCurLeading());
}

int CStrip::GetFree()
{
	return m_nFree;
}

void CStrip::SetFree(int nFree)
{
	m_nFree = nFree;
}

// use GetStripRect_CellsOnly() to set a local strip rectangle which bounds the enclosed cells
// only, that is, omitting the free translation rectangle if gbFreeTranslationMode is TRUE;
// because for clicks on the layout, we never do anything for a click on the free translation
// area, but only for clicks on cells (and even then we ignore clicks on cells with index == 2);
// so we set the height value using GetPileHeight() rather than GetStripHeight()
// Note: CStrip does not have any wxRect member; use this function to generate a local wxRect
// whenever one is needed for a strip - such as when testing for a mousedown within the strip
void CStrip::GetStripRect_CellsOnly(wxRect& rect)
{
	rect.SetTop(Top());
	rect.SetLeft(Left());
	rect.SetWidth(Width());
	rect.SetHeight(m_pLayout->GetPileHeight());
}


