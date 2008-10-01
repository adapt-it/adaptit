/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			Strip.cpp
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
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
#include "Cell.h"
#include "Pile.h"
#include "SourceBundle.h"
#include "MainFrm.h"
#include "Strip.h"
#include "AdaptitConstants.h"
#include "Adapt_ItView.h"

extern bool gbIsPrinting; // whm added because wxDC does not have ::IsPrinting() method

extern wxRect	grectViewClient; // used in OnDraw() below
extern int		gnCurPage;

/// This global is defined in Adapt_It.cpp.
extern struct PageOffsets pgOffsets;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(CStrip, wxObject)

CStrip::CStrip()
{
	m_nVertOffset = 0;
	m_nPileCount = 0;
	m_nPileHeight = 20;
	m_nStripIndex = 0;
	m_nFree = 0;
	for (int i = 0; i<MAX_PILES; i++)
		m_pPile[i] = (CPile*)NULL;
}

CStrip::CStrip(CAdapt_ItDoc* pDocument, CSourceBundle* pSourceBundle)
{
	m_pDoc = pDocument;
	m_pBundle = pSourceBundle;
	m_nVertOffset = 0;
	m_nPileCount = 0;
	m_nPileHeight = 20;
	m_nStripIndex = 0;
	m_nFree = 0;
	m_rectStrip = wxRect(0,0,0,0);
	m_rectFreeTrans = wxRect(0,0,0,0); // BEW added 24Jun05, for free translation support
	for (int i = 0; i<MAX_PILES; i++)
		m_pPile[i] = (CPile*)NULL;
}

CStrip::~CStrip()
{

}

void CStrip::DestroyPiles()
{
	int count = m_nPileCount;
	wxASSERT(count <= MAX_PILES);
	for (int i=0; i<count; i++)
	{
		m_pPile[i]->DestroyCells();
		delete m_pPile[i];
		m_pPile[i] = (CPile*)NULL;
	}
}

void CStrip::Draw(wxDC* pDC)
{
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
}
