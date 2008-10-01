/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			SourceBundle.cpp
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the CSourceBundle class. 
/// The CSourceBundle class represents the on-screen layout of a vertical series
/// of CStrip instances. The View uses some "indices" to define a "bundle" of
/// CSourcePhrase instances from the list maintained on the m_pSourcePhrases
/// attribute on the app. This bundle gets laid out by the function 
/// RecalcLayout(). LayoutStrip() is sometimes used to layout just a single
/// strip rather than the whole bundle.
/// \derivation		The CSourceBundle class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "SourceBundle.h"
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
#include "SourceBundle.h"
#include "AdaptitConstants.h"
#include "Adapt_ItView.h"
 
// next two are for version 2.0 which includes the option of a 3rd line for glossing
//extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text
//extern bool	gbEnableGlossing; // TRUE makes Adapt It revert to Shoebox functionality only

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(CSourceBundle, wxObject)

CSourceBundle::CSourceBundle()
{

	m_nLeading = 12;
	m_nLMargin = 6;
	m_nStripCount = 0;
	for (int i=0; i<MAX_STRIPS; i++)
		m_pStrip[i] = (CStrip*)NULL;
}

CSourceBundle::CSourceBundle(CAdapt_ItDoc* pDocument, CAdapt_ItView* pView)
{
	m_pDoc = pDocument;
	m_nLeading = 12;
	m_nLMargin = 6;
	m_nStripCount = 0;
	for (int i=0; i<MAX_STRIPS; i++)
		m_pStrip[i] = (CStrip*)NULL;
	m_pView = pView;
}

CSourceBundle::~CSourceBundle()
{

}

void CSourceBundle::DestroyStrips(const int nFirstStrip)
{
	wxASSERT(nFirstStrip < MAX_STRIPS);
	int count = m_nStripCount;

	if (count == 0)
		return; // do nothing if there are no strips to destroy
	for (int i=nFirstStrip; i<count; i++)
	{
		m_pStrip[i]->DestroyPiles();
		delete m_pStrip[i];
		m_pStrip[i] = (CStrip*)NULL;
	}
	m_nStripCount = 0;
	m_nStripIndex = 0;
}

void CSourceBundle::Draw(wxDC* pDC)
{
	for (int i=0; i< m_nStripCount; i++)
	{
		m_pStrip[i]->Draw(pDC);
	}
}

