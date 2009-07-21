// *****************************************************************************
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
// *****************************************************************************
// Pending Implementation Items (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 
// *****************************************************************************

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

// *******************************************************************
// Construction/Destruction
// *******************************************************************

IMPLEMENT_DYNAMIC_CLASS(CStrip, wxObject)

CStrip::CStrip()
{
	m_pLayout = NULL;
	m_nFree = -1;
	m_nStrip = -1;
	m_bValid = FALSE;
}

CStrip::CStrip(CLayout* pLayout)
{
	wxASSERT(pLayout != NULL);
	m_pLayout = pLayout;
	m_nFree = -1;
	m_nStrip = -1;
	m_bValid = FALSE;
}

CStrip::~CStrip()
{

}

#ifdef _ALT_LAYOUT_
wxArrayInt* CStrip::GetPileIndicesArray()
{
	return &m_arrPileIndices;
}
#else
wxArrayPtrVoid* CStrip::GetPilesArray()
{
	return &m_arrPiles;
}
#endif

#ifdef _ALT_LAYOUT_
int CStrip::GetPileIndicesCount()
{
	return m_arrPileIndices.GetCount();
}
#else
int CStrip::GetPileCount()
{
	return m_arrPiles.GetCount();
}
#endif

#ifdef _ALT_LAYOUT_
CPile* CStrip::GetPileByIndexInStrip(int index)
{
	int indexInPileList = m_arrPileIndices.Item(index);
	PileList::Node* pos = m_pLayout->m_pileList.Item(indexInPileList);
	return pos->GetData();
}
#else
CPile* CStrip::GetPileByIndex(int index)
{
	return (CPile*)m_arrPiles.Item(index);
}
#endif

#ifdef _ALT_LAYOUT_
void CStrip::Draw(wxDC* pDC)
{
	if (gbIsPrinting)
	{
        // whm Note: The pOffsets members nTop and nBottom were negative in the MFC
        // version, but remain positive in the wx version.
		POList* pList = &m_pLayout->m_pApp->m_pagesList;
		POList::Node* pos = pList->Item(gnCurPage-1);
		PageOffsets* pOffsets = (PageOffsets*)pos->GetData();
		if (m_nStrip < pOffsets->nFirstStrip || m_nStrip > pOffsets->nLastStrip)
			return;
	}
	int i;
	int nPileCount = m_arrPileIndices.GetCount();
	for (i = 0; i < nPileCount; i++)
	{
		CPile* pPile = GetPileByIndexInStrip(i);
		if (pPile != NULL)
			pPile->Draw(pDC);
	}
}
#else
void CStrip::Draw(wxDC* pDC)
{
	if (gbIsPrinting)
	{
		// whm Note: The pOffsets members nTop and nBottom were negative in the MFC version, but remain
		// positive in the wx version.
		POList* pList = &m_pLayout->m_pApp->m_pagesList;
		POList::Node* pos = pList->Item(gnCurPage-1);
		PageOffsets* pOffsets = (PageOffsets*)pos->GetData();
		if (m_nStrip < pOffsets->nFirstStrip || m_nStrip > pOffsets->nLastStrip)
			return;
	}
#ifdef Print_failure
	if (gbIsPrinting)
	{
		wxLogDebug(_T("CStrip::Draw() strip index %d , its rectangle (logical coords) x %d  y %d , width %d  height %d"),
			this->m_nStrip, Left(), Top(), Width(), Height());
	}
#endif
	int i;
	int nPileCount = m_arrPiles.GetCount();
	CPile* aPilePtr = NULL;
	for (i = 0; i < nPileCount; i++)
	{
		aPilePtr = ((CPile*)m_arrPiles[i]);
		aPilePtr->Draw(pDC);
#ifdef BLINKING_BUG
		wxLogDebug(_T("CStrip::Draw() AFTER drawing CPile %d  having Src Text %s "), i, 
			aPilePtr->m_pSrcPhrase->m_srcPhrase );
#endif
	}
}
#endif

// refactored CreateStrip() returns the iterator set for the CPile instance which is to be
// first when CreateStrip() is again called in order to build the next strip, or NULL if
// the document end has been reached (passing in the iterator avoids having to have a
// PileList::Item() call at the start of the function, so time is saved when setting up the
// strips for a whole document)
// if _ALT_LAYOUT_ is #defined it passes in the index to the next CPile* to be placed in
// the new strip, and returns the index of the next one which is to be replaced on next
// entry 
#ifdef _ALT_LAYOUT_
int CStrip::CreateStrip(int nIndexToFirst, int nStripWidth, int gap)
{
	m_nFree = nStripWidth;
	int nPileIndexInLayout = nIndexToFirst;
	PileList::Node* pos = m_pLayout->m_pileList.Item(nPileIndexInLayout);
	wxASSERT(pos);

	CPile* pPile = NULL;
	int pileWidth = 0;
	int nCurrentSpan = 0;

    // clear the two arrays - failure to do this leaves gargage in their members in debug mode
	m_arrPileIndices.Clear();
	m_arrPileOffsets.Clear();

	// lay out the piles
/*
#ifdef __WXDEBUG__
	SPList* pSrcPhrases = m_pLayout->m_pApp->m_pSourcePhrases;
	SPList::Node* posDebug = pSrcPhrases->GetFirst();
	int index = 0;
	while (posDebug != NULL)
	{
		CSourcePhrase* pSrcPhrase = posDebug->GetData();
		wxLogDebug(_T("Index = %d   pSrcPhrase pointer  %x"),index,pSrcPhrase);
		posDebug = posDebug->GetNext();
		index++;
	}
#endif
*/
    // this refactored code is commented out because it turned out that RTL layout and LRT
    // layout of the piles in the strip uses exactly the same code - they are appended to
    // the strip in logical order, and it is only the rectangle calculations (none of which
    // are done here) which specify the differing locations of pile[0], pile[1] etc for RTL
    // versus LTR layout -- so that information will be in the coordinate calculations -
    // specifically, in CPile::Left()
//	if (!gbRTL_Layout)
	{
		// Unicode version or Regular version, and Left to Right layout is wanted
		int nHorzOffset_FromLeft = 0;
		int pileIndex_InStrip = 0; // index into CStrip's m_arrPileIndices array of indices
								   // into the CPile* members in CLayout::m_pileList
		int nWidthOfPreviousPile = 0;
		
        // we must always have at least one pile's index in the strip in order to prevent an
        // infinite loop of empty strips if the width of the first pile should happen to
        // exceed the strip's width; so we place the first unilaterally, regardless of its
        // width
		pPile = (CPile*)pos->GetData(); // (the cast is unnecessary) this is the CPile 
										// pointer for the current nPileIndexInLayout value
		// set the pile's m_pOwningStrip member
		pPile->m_pOwningStrip = this;
		if (m_pLayout->m_pApp->m_nActiveSequNum == -1)
		{
			pileWidth = pPile->m_nMinWidth; // no "wide gap" for phrase box, as it is hidden
		}
		else if (pPile->m_pSrcPhrase->m_nSequNumber == m_pLayout->m_pApp->m_nActiveSequNum)
		{
			pileWidth = pPile->m_nWidth; // at m_nActiveSequNum, this value will be 
                    // large enough to accomodate the phrase box's width, even if just
                    // expanded due to the user's typing
		}
		else
		{
			pileWidth = pPile->m_nMinWidth;
		}
		m_arrPileIndices.Add(nPileIndexInLayout); // store in CStrip's m_arrPileIndices array
		m_arrPileOffsets.Add(nHorzOffset_FromLeft); // store offset to left boundary
		m_nFree -= pileWidth; // reduce the free space accordingly
		pPile->m_nPile = pileIndex_InStrip; // store its index within strip's 
				// m_arrPileIndices array into the CPile instance in CLayout::m_pileList
		// Note: I suspect we won't need to make use of this m_nPile member of
		// CPile, so should we delete it from Pile.h & .cpp? *** TODO *** ??

		// prepare for next iteration
		nWidthOfPreviousPile = pileWidth;
		pileIndex_InStrip++;
		pos = pos->GetNext(); // will be NULL if the pile just created was at doc end
		nPileIndexInLayout++; // index for the next CPile* instance in CLayout::m_pileList
		nHorzOffset_FromLeft = nWidthOfPreviousPile + gap;

		// if m_nFree went negative or zero, we can't fit any more piles, so declare 
		// the strip full
		if (m_nFree <= 0)
		{
			m_bValid = TRUE;
			return nPileIndexInLayout;
		}

		// append second and later piles to the strip
		while (pos != NULL  && m_nFree > 0)
		{
			pPile = (CPile*)pos->GetData();
			wxASSERT(pPile != NULL);

            // break out of the loop without including this pile in the strip if it is a
            // non-initial pile which contains a marker in its pointed at pSrcPhrase which
            // is a wrap marker for text (we want strips to wrap too, provided the view
            // menu item has that feature turned on)
			if (m_pLayout->m_pApp->m_bMarkerWrapsStrip && pileIndex_InStrip > 0)
			{
				bool bCausesWrap = pPile->IsWrapPile();
				// do the wrap only if it is a wrap marker and we have not just deal with
				// a document-initial marker, such as \id
				if (bCausesWrap && !(m_nStrip == 0 && pileIndex_InStrip == 1))
				{
					// if we need to wrap, discontinue assigning piles to this strip (the
					// nPileIndex_InList value is already correct for returning to caller)
					m_bValid = TRUE;
					return nPileIndexInLayout;
				}
			}

            // if control gets to here, the pile is a potential candidate for inclusion in
            // this strip, so work out if it will fit - and if it does, add it to the
            // m_arrPiles, etc
            if (m_pLayout->m_pApp->m_nActiveSequNum == -1)
			{
				// when at the doc end, every pile uses m_nMinWidth value, as phrase box
				// is hidden
				pileWidth = pPile->m_nMinWidth;
			}
			else if (pPile->m_pSrcPhrase->m_nSequNumber == m_pLayout->m_pApp->m_nActiveSequNum)
			{
				pileWidth = pPile->m_nWidth; // at m_nActiveSequNum, this value will be 
                                // large enough to accomodate the phrase box's width, even
                                // if just expanded due to the user's typing
			}
			else
			{
				pileWidth = pPile->m_nMinWidth;
			}
			nCurrentSpan = pileWidth + gap; // this much has to fit in the m_nFree 
                                // space for this pile to be eligible for inclusion in the
                                // strip
			if (nCurrentSpan <= m_nFree)
			{
				// this pile will fit in the strip, so add it
				m_arrPileIndices.Add(nPileIndexInLayout); // store in CStrip's array
				m_arrPileOffsets.Add(nHorzOffset_FromLeft); // store offset to left boundary
				m_nFree -= nCurrentSpan; // reduce the free space accordingly
				pPile->m_nPile = pileIndex_InStrip; // store its index within strip's
					// m_arrPileIndices array into the CPile instance in CLayout::m_pileList
			}
			else
			{
				// this pile won't fit, so the strip is full - declare it full and return
				// the pile list's index for use in the next strip's creation
				m_bValid = TRUE;
				return nPileIndexInLayout;
			}

			// set the pile's m_pOwningStrip member
			pPile->m_pOwningStrip = this;

			// update index for next iteration
			nWidthOfPreviousPile = pileWidth;
			pileIndex_InStrip++;

			// advance the iterator for the CLayout's m_pileList list of pile pointers
			pos = pos->GetNext(); // will be NULL if the pile just created was at doc end
			nPileIndexInLayout++; // index for the next CPile* instance in CLayout::m_pileList

			// set the nHorzOffset_FromLeft value ready for the next iteration of the loop
			nHorzOffset_FromLeft += nWidthOfPreviousPile + gap;
		}
	}
//	else
//	{
		// RTL code omitted here (see #else block below, where it is preserved) because it is
		// identical to LTR code except for a name change of one variable, so don't need 2 blocks
//	}
	// if the loop exits because the while test yields FALSE, then either we are at the end of the
	// document or the first pile was wider than the whole strip - in either case we must declare
	// this strip valid and we are done
	m_bValid = TRUE;
	return nPileIndexInLayout; // the index value where we start when we create the next strip
}
#else
PileList::Node* CStrip::CreateStrip(PileList::Node*& pos, int nStripWidth, int gap)
{
	m_nFree = nStripWidth;

	CPile* pPile = NULL;
	int pileWidth = 0;
	int nCurrentSpan = 0;

    // clear the two arrays - failure to do this leaves gargage in their members (such as
    // m_size a huge number & m_count a huge number) & so get app crash
	m_arrPiles.Clear();
	m_arrPileOffsets.Clear();

	// lay out the piles
/*
#ifdef __WXDEBUG__
	SPList* pSrcPhrases = m_pLayout->m_pApp->m_pSourcePhrases;
	SPList::Node* posDebug = pSrcPhrases->GetFirst();
	int index = 0;
	while (posDebug != NULL)
	{
		CSourcePhrase* pSrcPhrase = posDebug->GetData();
		wxLogDebug(_T("Index = %d   pSrcPhrase pointer  %x"),index,pSrcPhrase);
		posDebug = posDebug->GetNext();
		index++;
	}
#endif
*/
    // this refactored code is commented out because it turned out that RTL layout and LRT
    // layout of the piles in the strip uses exactly the same code - they are appended to
    // the strip in logical order, and it is only the rectangle calculations (none of which
    // are done here) which specify the differing locations of pile[0], pile[1] etc for RTL
    // versus LTR layout -- so that information will be in the coordinate calculations -
    // specifically, in CPile::Left()
//	if (!gbRTL_Layout)
	{
		// Unicode version, and Left to Right layout is wanted
		int nHorzOffset_FromLeft = 0;
		int pileIndex_InStrip = 0; // index into CStrip's m_arrPiles array of void*
		int nWidthOfPreviousPile = 0;
		
        // we must always have at least one pile in the strip in order to prevent an
        // infinite loop of empty strips if the width of the first pile should happen to
        // exceed the strip's width; so we place the first unilaterally, regardless of its
        // width
		pPile = (CPile*)pos->GetData();
		// set the pile's m_pOwningStrip member
		pPile->m_pOwningStrip = this;
		if (m_pLayout->m_pApp->m_nActiveSequNum == -1 || m_pLayout->m_bLayoutWithoutVisiblePhraseBox)
		{
			pileWidth = pPile->m_nMinWidth; // no "wide gap" for phrase box, as it is hidden
		}
		else if (pPile->m_pSrcPhrase->m_nSequNumber == m_pLayout->m_pApp->m_nActiveSequNum)
		{
			pileWidth = pPile->m_nWidth; // at m_nActiveSequNum, this value will be 
                    // large enough to accomodate the phrase box's width, even if just
                    // expanded due to the user's typing
		}
		else
		{
			pileWidth = pPile->m_nMinWidth;
		}
		m_arrPiles.Add(pPile); // store it
		m_arrPileOffsets.Add(nHorzOffset_FromLeft); // store offset to left boundary
		m_nFree -= pileWidth; // reduce the free space accordingly
		pPile->m_nPile = pileIndex_InStrip; // store its index within strip's m_arrPiles array

		// prepare for next iteration
		nWidthOfPreviousPile = pileWidth;
		pileIndex_InStrip++;
		pos = pos->GetNext(); // will be NULL if the pile just created was at doc end
		nHorzOffset_FromLeft = nWidthOfPreviousPile + gap;
	
		// if m_nFree went negative or zero, we can't fit any more piles, so declare 
		// the strip full
		if (m_nFree <= 0)
		{
			m_bValid = TRUE;
			return pos;
		}

		// append second and later piles to the strip
		while (pos != NULL  && m_nFree > 0)
		{
			pPile = (CPile*)pos->GetData();
			wxASSERT(pPile != NULL);

			// break out of the loop without including this pile in the strip if it is a
			// non-initial pile which contains a marker in its pointed at pSrcPhrase which is a
			// wrap marker for text (we want strips to wrap too, provided the view menu item has
			// that feature turned on)
			if (m_pLayout->m_pApp->m_bMarkerWrapsStrip && pileIndex_InStrip > 0)
			{
				bool bCausesWrap = pPile->IsWrapPile();
				// do the wrap only if it is a wrap marker and we have not just deal with
				// a document-initial marker, such as \id
				if (bCausesWrap && !(m_nStrip == 0 && pileIndex_InStrip == 1))
				{
					// if we need to wrap, discontinue assigning piles to this strip (the
					// nPileIndex_InList value is already correct for returning to caller)
					m_bValid = TRUE;
					return pos;
				}
			}

			// if control gets to here, the pile is a potential candidate for inclusion in this
			// strip, so work out if it will fit - and if it does, add it to the m_arrPiles, etc
			if (m_pLayout->m_pApp->m_nActiveSequNum == -1 || m_pLayout->m_bLayoutWithoutVisiblePhraseBox)
			{
				pileWidth = pPile->m_nMinWidth; // no "wide gap" for phrase box, as it is hidden
			}
			else if (pPile->m_pSrcPhrase->m_nSequNumber == m_pLayout->m_pApp->m_nActiveSequNum)
			{
				pileWidth = pPile->m_nWidth; // at m_nActiveSequNum, this value will be large enough
											 // to accomodate the phrase box's width, even if just
											 // expanded due to the user's typing
			}
			else
			{
				pileWidth = pPile->m_nMinWidth;
			}
			nCurrentSpan = pileWidth + gap; // this much has to fit in the m_nFree space for this
											// pile to be eligible for inclusion in the strip
			if (nCurrentSpan <= m_nFree)
			{
				// this pile will fit in the strip, so add it
				m_arrPiles.Add(pPile); // store it
				m_arrPileOffsets.Add(nHorzOffset_FromLeft); // store offset to left boundary
				m_nFree -= nCurrentSpan; // reduce the free space accordingly
				pPile->m_nPile = pileIndex_InStrip; // store its index within strip's m_arrPiles array
			}
			else
			{
				// this pile won't fit, so the strip is full - declare it full and return
				// the pile list's index for use in the next strip's creation
				m_bValid = TRUE;
				return pos;
			}

			// set the pile's m_pOwningStrip member
			pPile->m_pOwningStrip = this;

			// update index for next iteration
			pileIndex_InStrip++;
			nWidthOfPreviousPile = pileWidth;

			// advance the iterator for the CLayout's m_pileList list of pile pointers
			pos = pos->GetNext(); // will be NULL if the pile just created was at doc end

			// set the nHorzOffset_FromLeft value ready for the next iteration of the loop
			nHorzOffset_FromLeft += nWidthOfPreviousPile + gap;	
		}
	}
//	else
//	{
		/* unneeded code, identical to LTR code except for a cosmetic name change of one variable
		// Unicode version, and Right to Left layout is wanted
		int nHorzOffset_FromRight = 0;
		int pileIndex_InStrip = 0; // index into CStrip's m_arrPiles array of void*
		int nWidthOfPreviousPile = 0;
		
        // we must always have at least one pile in the strip in order to prevent an infinite
        // loop of empty strips if the width of the first pile should happen to exceed the
        // strip's width; so we place the first unilaterally, regardless of its width
		pPile = (CPile*)pos->GetData();
		// set the pile's m_pOwningStrip member
		pPile->m_pOwningStrip = this;
		if (m_pLayout->m_pApp->m_nActiveSequNum == -1)
		{
			pileWidth = pPile->m_nMinWidth; // no "wide gap" for phrase box, as it is hidden
		}
		else if (pPile->m_pSrcPhrase->m_nSequNumber == m_pLayout->m_pApp->m_nActiveSequNum)
		{
			pileWidth = pPile->m_nWidth; // at m_nActiveSequNum, this value will be large enough
										 // to accomodate the phrase box's width, even if just
										 // expanded due to the user's typing
		}
		else
		{
			pileWidth = pPile->m_nMinWidth;
		}
		m_arrPiles.Add(pPile); // store it
		m_arrPileOffsets.Add(nHorzOffset_FromRight); // store offset to right boundary
		m_nFree -= pileWidth; // reduce the free space accordingly
		pPile->m_nPile = pileIndex_InStrip; // store its index within strip's m_arrPiles array

		// prepare for next iteration
		nWidthOfPreviousPile = pileWidth;
		pileIndex_InStrip++;
		pos = pos->GetNext(); // will be NULL if the pile just created was at doc end
		nHorzOffset_FromRight = nWidthOfPreviousPile + gap;

		// if m_nFree went negative or zero, we can't fit any more piles, so declare 
		// the strip full
		if (m_nFree <= 0)
		{
			m_bValid = TRUE;
			//return nPileIndex_InList;
			return pos;
		}

		// append second and later piles to the strip
		while (pos != NULL  && m_nFree > 0)
		{
			pPile = (CPile*)pos->GetData();
			wxASSERT(pPile != NULL);

            // break out of the loop without including this pile in the strip if it is a
            // non-initial pile which contains a marker in its pointed at pSrcPhrase which
            // is a wrap marker for text (we want strips to wrap too, provided the view
            // menu item has that feature turned on)
			if (m_pLayout->m_pApp->m_bMarkerWrapsStrip && pileIndex_InStrip > 0)
			{
				bool bCausesWrap = pPile->IsWrapPile();
				if (bCausesWrap && !(m_nStrip == 0 && pileIndex_InStrip == 1))
				{
					// if we need to wrap, discontinue assigning piles to this strip (the
					// nPileIndex_InList value is already correct for returning to caller)
					m_bValid = TRUE;
					//return nPileIndex_InList;
					return pos;
				}
			}

            // if control gets to here, the pile is a potential candidate for inclusion in
            // this strip, so work out if it will fit - and if it does, add it to the
            // m_arrPiles, etc
			if (m_pLayout->m_pApp->m_nActiveSequNum == -1)
			{
				pileWidth = pPile->m_nMinWidth; // no "wide gap" for phrase box, as it is hidden
			}
			else if (pPile->m_pSrcPhrase->m_nSequNumber == m_pLayout->m_pApp->m_nActiveSequNum)
			{
				pileWidth = pPile->m_nWidth; // at m_nActiveSequNum, this value will be 
                                // large enough to accomodate the phrase box's width, even
                                // if just expanded due to the user's typing
			}
			else
			{
				pileWidth = pPile->m_nMinWidth;
			}
			nCurrentSpan = pileWidth + gap; // this much has to fit in the m_nFree space for this
											// pile to be eligible for inclusion in the strip
			if (nCurrentSpan <= m_nFree)
			{
				// this pile will fit in the strip, so add it
				m_arrPiles.Add(pPile); // store it
				m_arrPileOffsets.Add(nHorzOffset_FromRight); // store offset to right boundary
				m_nFree -= nCurrentSpan; // reduce the free space accordingly
				pPile->m_nPile = pileIndex_InStrip; // store its index within strip's m_arrPiles array
			}
			else
			{
				// this pile won't fit, so the strip is full - declare it full and return
				// the pile list's index for use in the next strip's creation
				m_bValid = TRUE;
				return pos;
			}

			// set the pile's m_pOwningStrip member
			pPile->m_pOwningStrip = this;

			// update index for next iteration
			pileIndex_InStrip++;
			nWidthOfPreviousPile = pileWidth;

			// advance the iterator for the CLayout's m_pileList list of pile pointers
			pos = pos->GetNext(); // will be NULL if the pile just created was at doc end

			// set the nHorzOffset_FromLeft value ready for the next iteration of the loop
			nHorzOffset_FromRight += nWidthOfPreviousPile + gap;
		}
	*/
//	}
	// if the loop exits because the while test yields FALSE, then either we are at the end of the
	// document or the first pile was wider than the whole strip - in either case we must declare
	// this strip filled and we are done	
	m_bValid = TRUE;
	return pos; // the iterator value where we start when we create the next strip
}
#endif

// the following is an overrided version to be used when refilling emptied strips, for a
// range of piles - rather than all the piles to the document's end, and so it has an
// ending index value passed in for the last pile pointer to be placed in a strip.
// The value returned is a count of how many CPile pointers were placed in the current
// strip, there will always be at least one - empty strips are illegal; the function
// should not be called if there are no more piles to be placed.
int CStrip::CreateStrip(int nInitialPileIndex, int nEndPileIndex, int nStripWidth, int gap)
{
	m_nFree = nStripWidth;

	CPile* pPile = NULL;
	int pileWidth = 0;
	int nCurrentSpan = 0;

	// establish an iterator for the pPiles list, initialize to the start of the range
	// and a count of how many CPile pointer instances are to be placed; and a counter for
	// how many get placed in the current strip - this value we return when the strip is
	// full or we have run out of piles to be placed
	PileList::Node* pos = m_pLayout->m_pileList.Item(nInitialPileIndex);
	wxASSERT(pos);
	int numPlaced = 0;
	int pileIndex = nInitialPileIndex;
/*
#ifdef __WXDEBUG__
	{
		wxLogDebug(_T("0.		CreateStrip: Remaining to be placed:  %d"),nEndPileIndex - nInitialPileIndex + 1);
	}
#endif
*/
    // clear the two arrays - failure to do this leaves gargage in their members (such as
    // m_size & m_count being left as huge numbers) & could lead to an app crash
	m_arrPiles.Clear();
	m_arrPileOffsets.Clear();

	// lay out the piles...
    // Note: RTL layout and LRT layout of the piles in the strip uses exactly the same code
    
	int nHorzOffset_FromLeft = 0;
	int pileIndex_InStrip = 0; // index into CStrip's m_arrPiles array of void*
	int nWidthOfPreviousPile = 0;
	
    // we must always have at least one pile in the strip in order to prevent an
    // infinite loop of empty strips if the width of the first pile should happen to
    // exceed the strip's width; so we place the first unilaterally, regardless of its
    // width
	pPile = (CPile*)pos->GetData();
	wxASSERT(pPile);
	// set the pile's m_pOwningStrip member
	pPile->m_pOwningStrip = this;
	if (m_pLayout->m_pApp->m_nActiveSequNum == -1)
	{
		pileWidth = pPile->m_nMinWidth; // no "wide gap" for phrase box, as it is hidden
	}
	else if (pPile->m_pSrcPhrase->m_nSequNumber == m_pLayout->m_pApp->m_nActiveSequNum)
	{
		// when tweaking strips rather than rebuilding, we won't get a larger gap
		// calculated at the active pile unless we call it here, provided the active
		// location is within the area of strips being rebuilt
		pileWidth = pPile->CalcPhraseBoxGapWidth();
		//pileWidth = pPile->m_nWidth; // at m_nActiveSequNum, this value will be 
                // large enough to accomodate the phrase box's width, even if just
                // expanded due to the user's typing
	}
	else
	{
		pileWidth = pPile->m_nMinWidth;
	}
	m_arrPiles.Add(pPile); // store it
	m_arrPileOffsets.Add(nHorzOffset_FromLeft); // store offset to left boundary
	numPlaced++; // increment counter of how many placed
	pileIndex++; // set tracker index to index of next pile to be placed
	m_nFree -= pileWidth; // reduce the free space accordingly
	pPile->m_nPile = pileIndex_InStrip; // store its index within strip's m_arrPiles array
/*
#ifdef __WXDEBUG__
	{
		wxLogDebug(_T("1.		CreateStrip: m-nStrip %d   pile[ %d ] pileWidth %d , offset %d , free left %d"),
					this->m_nStrip,pileIndex_InStrip,pileWidth,nHorzOffset_FromLeft,m_nFree);
	}
#endif
*/
	if (pileIndex > nEndPileIndex)
	{
		// we've just placed the last pile to be placed, so return numPlaced and set m_bValid
		// to TRUE, and let the caller work out if it should instead by set to FALSE
/*
#ifdef __WXDEBUG__
	{
		wxLogDebug(_T("1.1		CreateStrip: Early exit: pileIndex > nEndPileIndex is TRUE,  pile[%d] Placed %d"), pileIndex - 1, numPlaced);
	}
#endif
*/
		m_bValid = TRUE;
		return numPlaced;
	}

	// prepare for next iteration
	nWidthOfPreviousPile = pileWidth;
	pileIndex_InStrip++;
	pos = pos->GetNext(); // will be NULL if the pile just placed was at doc end
	if (pos == NULL)
	{
		// no more piles available for placement, so return
/*
#ifdef __WXDEBUG__
	{
		wxLogDebug(_T("1.2		CreateStrip: Early exit: pos == NULL is TRUE,  pile[%d] Placed %d"), pileIndex - 1, numPlaced);
	}
#endif
*/
		m_bValid = TRUE;
		return numPlaced;
	}
	nHorzOffset_FromLeft = nWidthOfPreviousPile + gap;

	// if m_nFree went negative or zero, we can't fit any more piles, so declare 
	// the strip full
	if (m_nFree <= 0)
	{
/*
#ifdef __WXDEBUG__
	{
		wxLogDebug(_T("1.3		CreateStrip: Early exit: m_nFree <= 0 is TRUE,  pile[%d] Placed %d"), pileIndex - 1, numPlaced);
	}
#endif
*/
		m_bValid = TRUE;
		return numPlaced;
	}

	// append second and later piles to this strip
	while (pos != NULL  && m_nFree > 0 && pileIndex <= nEndPileIndex)
	{
		pPile = (CPile*)pos->GetData();
		wxASSERT(pPile != NULL);

		// break out of the loop without including this pile in the strip if it is a
		// non-initial pile which contains a marker in its pointed at pSrcPhrase which is a
		// wrap marker for text (we want strips to wrap too, provided the view menu item has
		// that feature turned on)
		if (m_pLayout->m_pApp->m_bMarkerWrapsStrip && pileIndex_InStrip > 0)
		{
			bool bCausesWrap = pPile->IsWrapPile();
			// do the wrap only if it is a wrap marker and we have not just deal with
			// a document-initial marker, such as \id
			if (bCausesWrap && !(m_nStrip == 0 && pileIndex_InStrip == 1))
			{
				// if we need to wrap, discontinue assigning piles to this strip (the
				// nPileIndex_InList value is already correct for returning to caller)
/*
#ifdef __WXDEBUG__
	{
		wxLogDebug(_T("1.4		CreateStrip: Exit due to WRAP condition satisfied,  pile[%d] Placed %d"), pileIndex - 1, numPlaced);
	}
#endif
*/
				m_bValid = TRUE;
				return numPlaced;
			}
		}

		// if control gets to here, the pile is a potential candidate for inclusion in this
		// strip, so work out if it will fit - and if it does, add it to the m_arrPiles, etc
		if (m_pLayout->m_pApp->m_nActiveSequNum == -1)
		{
			pileWidth = pPile->m_nMinWidth; // no "wide gap" for phrase box, as it is hidden
		}
		else if (pPile->m_pSrcPhrase->m_nSequNumber == m_pLayout->m_pApp->m_nActiveSequNum)
		{
			pileWidth = pPile->m_nWidth; // at m_nActiveSequNum, this value will be large enough
										 // to accomodate the phrase box's width, even if just
										 // expanded due to the user's typing
		}
		else
		{
			pileWidth = pPile->m_nMinWidth;
		}
		nCurrentSpan = pileWidth + gap; // this much has to fit in the m_nFree space 
						// for this pile to be eligible for inclusion in the strip
		if (nCurrentSpan <= m_nFree)
		{
			// this pile will fit in the strip, so add it
			m_arrPiles.Add(pPile); // store it
			m_arrPileOffsets.Add(nHorzOffset_FromLeft); // store offset to left boundary
			numPlaced++; // increment counter of how many placed
			pileIndex++; // set tracker index to index of next pile to be placed
			m_nFree -= nCurrentSpan; // reduce the free space accordingly
			pPile->m_nPile = pileIndex_InStrip; // store its index within strip's 
												// m_arrPiles array
			// set the pile's m_pOwningStrip member
			pPile->m_pOwningStrip = this;
/*
#ifdef __WXDEBUG__
	{
		wxString src = pPile->GetSrcPhrase()->m_srcPhrase;
		wxLogDebug(_T("2. inloop	CreateStrip: m-nStrip %d   pile[ %d ] pileWidth %d , offset %d , free left %d, numPlaced %d srcPhrase %s"),
					this->m_nStrip,pileIndex_InStrip,pileWidth,nHorzOffset_FromLeft,m_nFree, numPlaced, src);
	}
#endif
*/
		}
		else
		{
			// this pile won't fit, so the strip is full - declare it full and return
			// the pile list's index for use in the next strip's creation
/*
#ifdef __WXDEBUG__
	{
		wxString src = pPile->GetSrcPhrase()->m_srcPhrase;
		wxLogDebug(_T("2.1	inloop	CreateStrip:  nCurrentSpan <= m_nFree is TRUE so return,  pile[%d] Placed %d , nCurrentSpan %d m_nFree %d srcPhrase %s"), 
					pileIndex - 1, numPlaced, nCurrentSpan, m_nFree, src);
	}
#endif
*/
			m_bValid = TRUE;
			return numPlaced;
		}

		// have we just dealt with the last one to be placed? (pileIndex has been
		// augmented to the next pile's index, so if we placed the last one, pileIndex
		// will have a value larger than nEndPileIndex at this point
		if (pileIndex > nEndPileIndex)
		{
			// we've just placed the last pile to be placed, so return numPlaced and set m_bValid
			// to TRUE, and let the caller work out if it should instead by set to FALSE
			m_bValid = TRUE;
/*
#ifdef __WXDEBUG__
	{
		wxString src = pPile->GetSrcPhrase()->m_srcPhrase;
		wxLogDebug(_T("2.2	inloop	CreateStrip: pileIndex > nEndPileIndex is TRUE, so return,  pile[%d] Placed %d srcPhrase %s"),
						pileIndex - 1, numPlaced, src);
	}
#endif
*/
			return numPlaced;
		}
		else
		{
			// there is at least one more to be placed, so prepare for next iteration...
			pileIndex_InStrip++;
			nWidthOfPreviousPile = pileWidth;

			// advance the iterator for the CLayout's m_pileList list of pile pointers
			pos = pos->GetNext(); // will be NULL if the pile just created was at doc end
/*
#ifdef __WXDEBUG__
	{
		if (pos != NULL)
		{
			CPile* pile = pos->GetData();
			wxString src = pile->GetSrcPhrase()->m_srcPhrase;
			wxLogDebug(_T("2.3	inloop	CreateStrip: iterating loop: next pos %x, last pile was [%d] Placed %d Trying srcPhrase %s"), 
				pos, pileIndex - 1, numPlaced, src);
		}
		else
		{
			wxLogDebug(_T("2.3	inloop	CreateStrip: iterating loop: next pos %x, last pile was [%d] Placed %d , and WILL EXIT since pos is NULL"), 
				pos, pileIndex - 1, numPlaced);
		}
	}
#endif
*/
			// set the nHorzOffset_FromLeft value ready for the next iteration of the loop
			nHorzOffset_FromLeft += nWidthOfPreviousPile + gap;
/*
#ifdef __WXDEBUG__
	{
		wxLogDebug(_T("	inloop	CreateStrip: ** Iterate Loop Now **, for next pile, nHorzOffset_FromLeft is %d"), nHorzOffset_FromLeft);
	}
#endif
*/
		}
	}

    // if the loop exits because the while test yields FALSE, then 1. either we are at the end
    // of the document, or 2. the first pile was wider than the whole strip, or 3. we have
    // reached and processed the pile at nEndPileIndex - we will declare this strip filled and
    // let the caller work out if m_bValid should instead be set to FALSE and it can then do so
	// (actually, the internal test for pileIndex >= nEndPileIndex should pick up reasons 1. and
	// 2. so we expect to get to the next line only when a pile covers or exceeds the whole
	// width of a strip)
	m_bValid = TRUE;
	return numPlaced;
}

// return the width of the strip (it's width is the maximum width it can be for the current
// client window size, less the left margin and right hand slop; the document size's .x
// value is already set to that value, so just return that; the width is NOT the width of
// any piles in the strip -- for that, use the Width() value and subtract m_nFree pixels
// from it)
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

// set the value of the validity flag 
void CStrip::SetValidityFlag(bool bValid)
{
	m_bValid = bValid;
}

// get the validity flag's value
bool CStrip::GetValidityFlag()
{
	return m_bValid;
}



// use GetStripRect_CellsOnly() to set a local strip rectangle which bounds the enclosed
// cells only, that is, omitting the free translation rectangle if gbFreeTranslationMode is
// TRUE; because for clicks on the layout, we never do anything for a click on the free
// translation area, but only for clicks on cells (and even then we ignore clicks on cells
// with index == 2); so we set the height value using GetPileHeight() rather than
// GetStripHeight()
// Note: CStrip does not have any wxRect member; use this function to generate a local
// wxRect whenever one is needed for a strip - such as when testing for a mousedown within
// the strip
void CStrip::GetStripRect_CellsOnly(wxRect& rect)
{
	rect.SetTop(Top());
	rect.SetLeft(Left());
	rect.SetWidth(Width());
	rect.SetHeight(m_pLayout->GetPileHeight());
}

// this overloaded version is more useful, it can be placed in line with other accessors
wxRect CStrip::GetStripRect_CellsOnly()
{
	wxRect rect;
	GetStripRect_CellsOnly(rect);
	return rect;
}

// this rectangle gives the same topLeft, width and height as in the legacy (ie.
// pre-refactoring) version -- we need this to avoid having to tweak the scroll calculations
wxRect CStrip::GetStripRect()
{
	wxRect rect;
	GetStripRect_CellsOnly(rect);
	rect.SetHeight(rect.GetHeight() + 3 + m_pLayout->m_nTgtHeight);
	return rect;
}

void CStrip::GetFreeTransRect(wxRect& rect)
{
	GetStripRect_CellsOnly(rect);
	// reset the Top to be 3 pixels below the bottom of the piles of the strip
	rect.SetTop(rect.GetBottom() + 3);
	// reset the Bottom to be the target text's height lower than the top
	rect.SetBottom(rect.GetTop() + m_pLayout->GetTgtTextHeight());
	// increase the width to be 50% of the RH_SLOP (which is 60) value = 30 pixels more
	rect.SetWidth(rect.GetWidth() + RH_SLOP / 2);
}

wxRect CStrip::GetFreeTransRect()
{
	wxRect rect;
	GetFreeTransRect(rect);
	return rect;
}

int CStrip::GetStripIndex()
{
	return m_nStrip;
}

