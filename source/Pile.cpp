/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Pile.cpp
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public
///                 License (see license directory)
/// \description	This is the implementation file for the CPile class.
/// The CPile class represents the next smaller divisions of a CStrip. The CPile
/// instances are laid out sequentially within a CStrip. Each CPile stores a
/// stack or "pile" of CCells stacked vertically in the pile. Within a CPile
/// the top CCell, if used, displays the source word or phrase with punctuation,
/// the second CCell displays the same minus the punctuation. The third CCell
/// displays the translation, minus any pnctuation. The fourth CCell displays the
/// translation with punctuation as copied (if the user typed none) or as typed
/// by the user. The fifth CCell displays the Gloss when used.
/// \derivation		The CPile class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

// whm NOTES CONCERNING RTL and LTR Rendering in wxWidgets:
// (BEW moved here from deprecated CText)
//    1. The wxWidgets wxDC::DrawText(const wxString& text, wxCoord x, wxCoord y) function
//    does not have an nFormat parameter like MFC's CDC::DrawText(const CString& str,
//    lPRECT lpRect, UINT nFormat) text-drawing function. The MFC function utilizes the
//    nFormat parameter to control the RTL vs LTR directionality, which apparently only
//    affects the directionality of the display context WITHIN the lpRect region of the
//    display context. At present, it seems that the wxWidgets function cannot directly
//    control the directionality of the text using its DrawText() function. In both MFC and
//    wxWidgets there is a way to control the overall layout direction of the elements of a
//    whole diaplay context. In MFC it is CDC::SetLayout(DWORD dwLayout); in wxWidgets it
//    is wxDC::SetLayoutDirection(wxLayoutDirection dir). Both of these dc layout functions
//    cause the whole display context to be mirrored so that all elements drawn in the
//    display context are reversed as though seen in a mirror. For a simple application
//    that only displays a single language in its display context, perhaps layout mirroring
//    would work OK. However, Adapt It must layout several different diverse languages
//    within the same display context, some of which may have different directionality and
//    alignment. Therefore, except for possibly some widget controls, MFC's SetLayout() and
//    wxWidgets' SetLayoutDirection() would not be good choices. The MFC Adapt It sources
//    NEVER call the mirroring functions. Instead, for writing on a display context, MFC
//    uses the nFormat paramter within DrawText(str,m_enclosingRect,nFormat) to accomplish
//    two things: (1) Render the text as Right-To-Left, and (2) Align the text to the RIGHT
//    within the enclosing rectangle passed as parameter to DrawText(). The challenge
//    within wxWidgets is to determine how to get the equivalent display of RTL and LTR
//    text.
//    2. The SetLayoutDirection() function within wxWidgets can be applied to certain
//    controls containing text such as wxTextCtrl and wxListBox, etc. It is presently an
//    undocumented method with the following signature:
//    SetLayoutDirection(wxLayoutDirection dir), where dir is wxLayout_LeftToRight or
//    wxLayout_RightToLeft. It should be noted that setting the layout to
//    wxLayout_RightToLeft on these controls also involves mirroring, so that any scrollbar
//    that gets displayed, for example, displays on the left rather than on the right for
//    RTL, etc.
// CONCLUSIONS:
// Pango in wxGTK, ATSIU in wxMac and Uniscribe in wxMSW seem to do a good job of rendering
// Right-To-Left Reading text with the correct directionality in a display context without
// calling the SetLayoutDirection() method. The main thing we have to do is determine where
// the starting point for the DrawText() operation needs to be located to effect the
// correct text alignment within the cells (rectangles) of the pile for the given language
// - the upper left coordinates for LTR text, and the upper-right coordinates for RTL text.
// Therefore, in the wx version we have to be careful about the automatic mirroring
// features involved in the SetLayoutDirection() function, since Adapt It MFC was designed
// to micromanage the layout direction itself in the coding of text, cells, piles, strips,
// etc.

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Pile.h"
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
#include "helpers.h"
#include "Adapt_ItDoc.h"
#include "SourcePhrase.h"
#include "AdaptitConstants.h"
// don't mess with the order of the following includes, Strip must precede View must
// precede Pile must precede Layout and Cell can usefully by last
#include "Strip.h"
#include "Adapt_ItView.h"
#include "Pile.h"
#include "Layout.h"
#include "FreeTrans.h"
#include "Cell.h"
#include "MainFrm.h"
#include "KB.h"
#include "TargetUnit.h"
#include "RefString.h"

// Define type safe pointer lists
#include "wx/listimpl.cpp"

// uncomment out the following line for display of last pile's m_chapVerse string, and
// length of string -- the length can be bogus (eg 20 when it should be 3, resulting the
// display of bogus buffer overrun characters for the navtext above the last pile when it
// is a verse with empty content but USFM markup - as can be got from Paratext) A kluge in
// DrawNavTextInfoAndIcons() fixes the problem.
//#define _SHOW_CHAP_VERSE

/// This macro together with the macro list declaration in the .h file
/// complete the definition of a new safe pointer list class called PileList.
WX_DEFINE_LIST(PileList);

// next two are for version 2.0 which includes the option of a 3rd line for glossing

// these next globals are put here when I moved CreatePile()
// from the view to the CPile class
extern bool gbShowTargetOnly;
extern bool gbGlossingVisible;
extern const wxChar* filterMkr;
extern CAdapt_ItApp* gpApp;

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

extern bool gbCheckInclGlossesText; // klb 9/9/2011 added because we now need to know when
										  //      to draw glosses for printing, based on checkboxes in PrintOptionsDlg.cpp

extern bool gbRTL_Layout;

#if defined(__WXGTK__)
extern bool gbCheckInclFreeTransText;
extern bool gbCheckInclGlossesText;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(CPile, wxObject)

CPile::CPile()
{
	m_pCell[0] = m_pCell[1] = m_pCell[2] = (CCell*)NULL;
	m_bIsCurrentFreeTransSection = FALSE; // BEW added 24Jun05 for free translation support
	m_pSrcPhrase = (CSourcePhrase*)NULL;
	m_pLayout = (CLayout*)NULL;
	m_pOwningStrip = (CStrip*)NULL;
	m_nWidth = 20;
	m_nMinWidth = gpApp->m_nMinPileWidth; // was 40; BEW changed 19May15
	//m_nMinWidth = 40;
	m_nPile = -1; // I don't belong in any strip yet
}

CPile::CPile(CLayout* pLayout)  // use this one, it sets m_pLayout
{
	m_pLayout = pLayout;
	m_pCell[0] = m_pCell[1] = m_pCell[2] = (CCell*)NULL;
	m_bIsCurrentFreeTransSection = FALSE; // BEW added 24Jun05 for
										  // free translation support
	m_pSrcPhrase = (CSourcePhrase*)NULL;
	m_pLayout = (CLayout*)NULL;
	m_pOwningStrip = (CStrip*)NULL;
	//m_nWidth = gpApp->m_nMinPileWidth; // was 20; BEW changed 19May15
	m_nWidth = 20;
	m_nMinWidth = gpApp->m_nMinPileWidth; // was 40; BEW changed 19May15
	//m_nMinWidth = 40;
	m_nPile = -1; // I don't belong in any strip yet
}


CPile::CPile(const CPile& pile)
{
	m_pLayout = pile.m_pLayout;
	m_pSrcPhrase = pile.m_pSrcPhrase;
	m_pOwningStrip = pile.m_pOwningStrip;
	int i;
	for (i = 0; i< MAX_CELLS; i++)
	{
		m_pCell[i] = new CCell(*(pile.m_pCell[i]));
	}
	m_nPile = pile.m_nPile;
	m_nWidth = pile.m_nWidth;
	m_nMinWidth = pile.m_nMinWidth;
	m_bIsCurrentFreeTransSection = pile.m_bIsCurrentFreeTransSection;
}

CPile::~CPile()
{

}

// implementation

CCell* CPile::GetCell(int nCellIndex)
{
	return m_pCell[nCellIndex];
}

int CPile::GetPileIndex()
{
	return m_nPile;
}

// (The function is now slightly misnamed because no longer is all that is considered
// "filtered" actually stored with an SF marker. Notes, free translations and collected
// back translations are now just stored as simple strings - we only put a marker with
// these info types when/if we export them or show them in the view filtered material
// dialog. However, we'll keep the old name unchanged; but if we do someday change it, we
// should call it HasFilteredInfo())
// BEW 22Feb10 changes done for support of doc version 5
// BEW 18Apr17 add support for the new m_filteredInfo_After member
bool CPile::HasFilterMarker()
{
	if (
		!m_pSrcPhrase->GetFilteredInfo().IsEmpty() ||
#if !defined(USE_LEGACY_PARSER)
		!m_pSrcPhrase->GetFilteredInfo_After().IsEmpty() ||
#endif
		!m_pSrcPhrase->GetFreeTrans().IsEmpty() ||
		!m_pSrcPhrase->GetNote().IsEmpty() ||
		!m_pSrcPhrase->GetCollectedBackTrans().IsEmpty() ||
		m_pSrcPhrase->m_bStartFreeTrans ||
		m_pSrcPhrase->m_bHasNote
		)
		return TRUE;
	else
		return FALSE;
}

void CPile::SetIsCurrentFreeTransSection(bool bIsCurrentFreeTransSection)
{
	m_bIsCurrentFreeTransSection = bIsCurrentFreeTransSection;
}

bool CPile::GetIsCurrentFreeTransSection()
{
	return m_bIsCurrentFreeTransSection;
}

// BEW removed 20Nov12, it's dangerous -- see .h file for why (use Width() instead)
//int CPile::GetWidth()
//{
//	return m_nWidth;
//}

int CPile::Width()
{
    // Note: for this calculation to return the correct values in all circumstances, the
    // m_nWidth and m_nMinWidth values must be up to date before Draw is called on the
    // CPile instance; in practice this most importantly means that the app's
    // m_nActiveSequNum value is up to date, because the else block below is entered once
    // per layout draw, and the width of the phrase box must be known and all relevant
    // parameters set which are used for communicating its width to the pile at the active
    // location and storing that width in its m_nWidth member
	if (m_pSrcPhrase->m_nSequNumber != m_pLayout->m_pApp->m_nActiveSequNum)
	{
		// when not at the active location, set the width to m_nMinWidth;
		return m_nMinWidth;
	}
	else
	{
        // at the active location, set the width using the .x extent of the app's
        // m_targetPhrase string - the width based on this is stored in m_nWidth, but is -1
        // if the pile is not the active pile
		return m_nWidth;
	}
}

int CPile::Height()
{
	return m_pLayout->GetPileHeight();
}

int CPile::Left()
{
	if (gbRTL_Layout)
	{
		// calculated as: "distance from left of client window to right boundary of strip"
		// minus "the sum of the distance of the pile from the right boundary of the strip
		// and the width of the pile"
		return m_pOwningStrip->Left() + (m_pLayout->m_logicalDocSize).x -
				(m_pOwningStrip->m_arrPileOffsets[m_nPile] + Width());
	}
	else // left-to-right layout
	{
		// calculated as: "distance from left of client window to left boundary of strip"
		// plus "the distance of the pile from the left boundary of the strip"
		return m_pOwningStrip->Left() + m_pOwningStrip->m_arrPileOffsets[m_nPile];
	}
}

int CPile::Top()
{
	return m_pOwningStrip->Top();
}

void CPile::GetPileRect(wxRect& rect)
{
	rect.SetTop(Top());
	rect.SetLeft(Left());
	rect.SetWidth(Width());
	rect.SetHeight(m_pLayout->GetPileHeight());
}

wxRect CPile::GetPileRect()
{
	wxRect rect;
	GetPileRect(rect);
	return rect;
}

void CPile::TopLeft(wxPoint& ptTopLeft)
{
	ptTopLeft.x = Left();
	ptTopLeft.y = Top();
}

//getter
CSourcePhrase* CPile::GetSrcPhrase()
{
	return m_pSrcPhrase;
}

// setter
void CPile::SetSrcPhrase(CSourcePhrase* pSrcPhrase)
{
	m_pSrcPhrase = pSrcPhrase;
}

int CPile::GetStripIndex()
{
	if (m_pOwningStrip == NULL)
		return -1; // the owning strip pointer was not set
	else
		return m_pOwningStrip->m_nStrip;
}

CStrip* CPile::GetStrip()
{
	return m_pOwningStrip;
}

void CPile::SetStrip(CStrip* pStrip)
{
	m_pOwningStrip = pStrip; // can pass in NULL to set it to nothing
}

void CPile::SetMinWidth()
{
	m_nMinWidth = CalcPileWidth();
#if defined(_DEBUG) && defined(_EXPAND)
//	wxLogDebug(_T("%s():line %d, sets: m_nMinWidth to: %d, for box text: %s   *********** called CalcPileWidth() *****"),
//		__FUNCTION__, __LINE__, m_nMinWidth, gpApp->m_pTargetBox->GetValue().c_str());
#endif
}

// overload, for using when restoring a cached m_nMinWidth value
void CPile::SetMinWidth(int width)
{
	m_nMinWidth = width;
}

//GDLC 2010-02-10 Added parameter to SetPhraseBoxGapWidth with default value steadyAsSheGoes
void CPile::SetPhraseBoxGapWidth(enum phraseBoxWidthAdjustMode widthMode)
{
	/* Oops, when in OnOpenDocument() and creating piles and strips, m_pActivePile is NULL
#if defined (_DEBUG) && defined (_EXPAND)

	if (gpApp->m_pActivePile != NULL && gpApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber == 15)
	{
		int breakhere = 1;
	}
#endif
	*/
	m_nWidth = CalcPhraseBoxGapWidth(widthMode);


/* CalcPhraseBoxGapWidth() now takes lists width into account, so don't need this
	// BEW 27Jul18 If the gap width (m_nWidth) as calculated above is less than the max of
	// m_curBoxWidth and m_curListWidth, the reset the value to that maximum
	int max = wxMax(gpApp->GetLayout()->m_curBoxWidth, gpApp->GetLayout()->m_curListWidth);
	if (m_nWidth < max)
	{
		m_nWidth = max;
	}
*/
#if defined(_DEBUG) //&& defined(_NEWDRAW)
//	wxLogDebug(_T("%s():line %d, CalcPhraseBoxGapWidth() sets: m_nWidth (gap) = %d , for box text: %s"),
//		__FUNCTION__, __LINE__, m_nWidth, gpApp->m_pTargetBox->GetValue().c_str());
#endif
}

int	CPile::GetMinWidth()
{
	return m_nMinWidth;
}

int	CPile::GetPhraseBoxGapWidth()
{
	return m_nWidth;
}

int	CPile::GetPhraseBoxWidth() //BEW added 19Jul18, gets m_curBoxWidth value
{
	return gpApp->m_pLayout->m_curBoxWidth;
}

int	CPile::GetPhraseBoxListWidth() //BEW added 24Jul18, gets m_curListWidth value
{
	return gpApp->m_pLayout->m_curListWidth;
}

void CPile::SetPhraseBoxListWidth()
{
	gpApp->m_pLayout->m_curListWidth = CalcPhraseBoxListWidth();
#if defined(_DEBUG) //&& defined(_NEWDRAW)
//	wxLogDebug(_T("CPile::SetPhraseBoxListWidth(), sets: m_nMinWidth = %d, for box text: %s"),
//		gpApp->m_pLayout->m_curListWidth, gpApp->m_pTargetBox->GetValue().c_str());
#endif
}

// If called at a non-active location, return -1 to indicate to the caller that
// the value returned is not to be used.
int CPile::CalcPhraseBoxListWidth()
{
	int listWidth = (int)wxNOT_FOUND; //initialise to -1
	CTargetUnit* pTU = NULL;
	CRefString* pRefString = NULL;
	wxString strText = wxEmptyString;
	int nActiveSequNum = gpApp->m_nActiveSequNum;
	CPile* pActivePile = gpApp->GetLayout()->GetPile(nActiveSequNum);
	if (pActivePile != NULL)
	{
		CSourcePhrase* pSrcPhrase = pActivePile->GetSrcPhrase();
		CLayout* pLayout = gpApp->GetLayout();
		CKB* pKB = NULL;
		wxString notInKB = _T("<Not In KB>");

		// Do these calculations only when the phrasebox is at the active pile; if not
		// so, return -1 to the caller
		if ((pSrcPhrase != NULL) && (pSrcPhrase->m_nSequNumber == nActiveSequNum))
		{
			// First, get a default width, regardless of contents, based on the
			// pile's basic minimum width plus the button width. List contents
			// may increase this value of course
			listWidth = gpApp->m_nMinPileWidth; // set at 40 I think
			// add the button width
			//listWidth += m_pLayout->buttonWidth;

			// Get a temporary device context from the canvas instance
			wxClientDC aDC((wxScrolledWindow*)pLayout->m_pCanvas);
			wxSize extent;

//#if defined(_DEBUG) && defined(_NEWDRAW)
//			wxLogDebug(_T("%s():line %d, adjustedButtonWidth: %d , for box text: %s  "),
//				__FUNCTION__, __LINE__, adjustedButtonWidth, m_pLayout->m_pApp->m_pTargetBox->GetValue().c_str());
//#endif
			// Set which KB is in force, and which font to use for the measuring
			if (gbIsGlossing)
			{
				pKB = gpApp->m_pGlossingKB;
				if (gbGlossingUsesNavFont)
					aDC.SetFont(*pLayout->m_pNavTextFont);
				else
					aDC.SetFont(*pLayout->m_pTgtFont);
			}
			else
			{
				pKB = gpApp->m_pKB;
				aDC.SetFont(*pLayout->m_pTgtFont);
			}

			int lastExtent = listWidth; // the minimum value as calculated above
					// We compare against this value

			// From the KB, get the appropriate pTU pointer to active location's CTargetUnit;
			// Beware, if the active location had a CTargetUnit instance with a reference count
			// of 1 for the KB entry, then "Landing" at that location will remove that
			// CRefString instance, and then it's CTargetUnit instance goes to walkabout as well
			// if there are no other entries in the KB for that m_key string value there.
			// In such a situation, no width calculation is possible, so just detect this
			// and return our default mnimum above; pTU is NULL in this situation.
			pTU = pKB->GetTargetUnit(pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key);
			if (pTU == NULL)
			{
				return lastExtent + m_pLayout->buttonWidth;
			}
			TranslationsList::Node* pos = pTU->m_pTranslations->GetFirst();
			wxASSERT(pos != NULL);
			while (pos != NULL)
			{
				pRefString = (CRefString*)pos->GetData();
				pos = pos->GetNext();
				if (!pRefString->GetDeletedFlag())
				{
					// this one is not deleted, so add it to the string array
					strText = pRefString->m_translation;
					// Note: str might be a (quite legal) empty adaptation string;
					// but if it is a <Not In KB> entry, then abandon anything else
					// and return wxNOT_FOUND immediately
					if (strText == notInKB)
					{
						return (int)wxNOT_FOUND;
					}
					if (!strText.IsEmpty())
					{
						aDC.GetTextExtent(strText, &extent.x, &extent.y);
						if (extent.x > lastExtent)
						{
							listWidth = extent.x;
						}
					}
				}
			}
//#if defined(_DEBUG) && defined(_NEWDRAW)
//			wxLogDebug(_T("%s():line %d, text extents-based width: %d , final listWidth (adding button and slop) = %d , for box text: %s  "),
//				__FUNCTION__, __LINE__, listWidth, (listWidth + slop + adjustedButtonWidth), m_pLayout->m_pApp->m_pTargetBox->GetValue().c_str());
//#endif
			// Add in the button
			listWidth += m_pLayout->buttonWidth;
		} // end of TRUE block for test: if ((pSrcPhrase != NULL) && (pSrcPhrase->m_nSequNumber == nActiveSequNum))
	}
	else
	{
		listWidth = wxNOT_FOUND; // -1
	}
	return listWidth;
}

void CPile::SetPhraseBoxWidth(enum phraseBoxWidthAdjustMode widthMode)
{
	SetPhraseBoxListWidth();
	int listWidth = GetPhraseBoxListWidth();
	if (listWidth == wxNOT_FOUND)
	{
		// may not be at an active pile, or it's a <Not In KB> situation,
		// or there is no CTargetUnit instance in the KB for the given
		// m_key value (this will be the case if the key has only a single
		// CRefString instance in the KB, and its refCount was 1, and the
		// the phrasebox has just landed at that location - which decrements
		// the reference count to 0 which in turn removes the CTargetUnit 
		// instance from the KB until the phrasebox moves to a new location)
		// In such a circumstance, treat the phrasebox as not having a list
		gpApp->m_pLayout->m_curBoxWidth = CalcPhraseBoxWidth(widthMode); // widthMode is now unused in this function
		SetMinWidth();

#if defined(_DEBUG) //&& defined(_NEWDRAW)
//		wxLogDebug(_T("%s():line %d, returning from CalcPhraseBoxWidth() sets: m_curBoxWidth = %d, for box text: %s  [listWidth = wxNOT_FOUND]"),
//			__FUNCTION__, __LINE__, gpApp->m_pLayout->m_curBoxWidth, gpApp->m_pTargetBox->GetValue().c_str());
#endif
	}
	else
	{
		// Set Layout's m_curBoxWidth, then we will have to compare that
		// with listWidth, and set m_curBoxWidth to whichever is the larger
		// width
		gpApp->m_pLayout->m_curBoxWidth = CalcPhraseBoxWidth(widthMode);
		int boxWidth = GetPhraseBoxWidth();
		if (listWidth > boxWidth)
		{
			gpApp->m_pLayout->m_curBoxWidth = listWidth;
		}
#if defined(_DEBUG) //&& defined(_NEWDRAW)
//		wxLogDebug(_T("%s():line %d, returning from CalcPhraseBoxWidth() sets: m_curBoxWidth = %d, for box text: %s  [listWidth = %d]"),
//			__FUNCTION__, __LINE__, gpApp->m_pLayout->m_curBoxWidth, gpApp->m_pTargetBox->GetValue().c_str(), listWidth);
#endif

		// BEW 20Jul18 despite the claim in CalcPileWith() that setting m_nMinWidth
		// for the active location can be left to RecalcLayout(), in the refactored
		// drawing protocol (supporting dropdown phrasebox) it does not appear to 
		// be the case that that is always sufficient. So, I'll add the call here
		// to get m_nMinWidth updated for the active pile too
		SetMinWidth();
	}
}

void CPile::SetPhraseBoxWidth(int boxwidth)
{
	gpApp->m_pLayout->m_curBoxWidth = boxwidth;
//#if defined(_DEBUG) //&& defined(_NEWDRAW)
//	wxLogDebug(_T("CPile::SetPhraseBoxWidth() OVERLOADED manual boxwidth passed in, sets: m_curBoxWidth = %d, for box text: %s"),
//		boxwidth, gpApp->m_pTargetBox->GetValue().c_str());
//#endif
}

// Calculates the pile's width before laying out the current pile in a strip. The function
// is not interested in the relative ordering of the glossing and adapting cells, and so
// does not access CCell instances; rather, it just examines extent of the three text
// members m_srcPhrase, m_targetStr, m_gloss on the CSourcePhrase instance pointed at by
// this particular CPile instance. The width is the maximum extent.x for the three strings
// checked.
// BEW 4Aug18, Hey, it's crazy to take the gloss extent when adapting mode is on, and
// glosses are not seen either. Long glosses just make for unnatural wide phrasesbox.
// When glosses are visible, we have to take them into account so that gaps are wide
// enough, and even if in adapting mode with glosses showing, we must do so also to
// avoid sequential long glosses overwriting each other on the screen. Refactor accordingly
int CPile::CalcPileWidth()
{
	int pileWidth = gpApp->m_nMinPileWidth; // was 40; BEW changed 19May15 
					// was 40; // ensure we never get a pileWidth of zero
	int pileWidth_Tgt = pileWidth; // initialize
	int pileWidth_Gloss = pileWidth;
	// get a device context for the canvas on the stack (wont' accept uncasted definition)
	wxClientDC aDC((wxScrolledWindow*)m_pLayout->m_pCanvas); // make a temporary device context
	wxSize extent;

	// First, source text
	aDC.SetFont(*m_pLayout->m_pSrcFont); // works, now we are friends
	aDC.GetTextExtent(m_pSrcPhrase->m_srcPhrase, &extent.x, &extent.y);
	pileWidth = extent.x; // can assume >= to key's width, as differ only by possible punctuation

	// Now target text
	if (!m_pSrcPhrase->m_targetStr.IsEmpty())
	{
		aDC.SetFont(*m_pLayout->m_pTgtFont);
		aDC.GetTextExtent(m_pSrcPhrase->m_targetStr, &extent.x, &extent.y);
		if (extent.x > pileWidth_Tgt)
		{
			pileWidth_Tgt = extent.x;
		}
	}
	if (pileWidth_Tgt > pileWidth)
	{
		pileWidth = pileWidth_Tgt;
	}
	// Now gloss text, but only if glosses are seen in adapting mode, or we are
	// in glossing mode
	if (gbIsGlossing || gbGlossingVisible)
	{
		if (!m_pSrcPhrase->m_gloss.IsEmpty())
		{
			if (gbGlossingUsesNavFont)
				aDC.SetFont(*m_pLayout->m_pNavTextFont);
			else
				aDC.SetFont(*m_pLayout->m_pTgtFont);
			aDC.GetTextExtent(m_pSrcPhrase->m_gloss, &extent.x, &extent.y);
			if (extent.x > pileWidth_Gloss)
			{
				pileWidth_Gloss = extent.x;
			}
		}
		if (pileWidth_Gloss > pileWidth)
		{
			pileWidth = pileWidth_Gloss;
		}
	}
	// BEW added next two lines, 19May15, sanity test, ensure the width is never zero
	if (pileWidth < gpApp->m_nMinPileWidth)
		pileWidth = gpApp->m_nMinPileWidth;
	return pileWidth;
}

int CPile::CalcPhraseBoxWidth(enum phraseBoxWidthAdjustMode widthMode)
{
	// box width is what we'll compute to return to caller. It starts out as a
	// "basic width" computed from the text extent for the adaptation, including
	// any punctuation; and then we add to that the width of the user's chosen
	// slop size for the wxTextCtrl (computed externally as a multiple of 'w' character
	// widths); and add the slop width of the dropdown list's button.
	// Our refactored phrasebox also has a list and it has size, even if empty.
	// So our starting point for sizing has to be the maximum of the basic width,
	// the value returned by CalcPileWidth() - which internally gets max width  of src,
	// tgt, and if glosses are visible, gloss text as well, and the width of the
	// drop down list (which could contain some long adaptations - greater than all
	// the previous sources for a starting width. Once we have the max, we must
	// add the width of slop and button, and then we have it.
	// The result should be then less than the phrasebox gap width computed
	// by CalcPhraseBoxGapWidth(), because the latter takes this value and just adds
	// the interpile gap, to ensure a gap large enough for our new phrasebox

	// BEW 18Aug18, text-based width calculations are a problem, because after a widening
	// the text-based calculations lag in width to what the phrasebox width has become,
	// making more than one widening produce GUI issues (box won't widen further, text
	// moves left in the phrasebox as more is added, and so the beginning of the text
	// becomes unseen). The solution is to support what FixBox() does - it augments
	// Layout's m_curBoxWidth at a widening (by adding another slop to the width), so
	// we have to have an effective way to get the actual new width be what this
	// function returns. To do this, we will grab m_curBoxWidth from the Layout object
	// when CalcPhraseBoxWidth begins its calculations, and store it until the end
	// of the text-based calculations. Then we will compare the two, and return whichever
	// is the larger. Doing this automatically adds updates CalcPhraseBoxGapWidth() too.
	// The returned value of the latter goes to the active pile's m_nWidth parameter,
	// which is then picked up by the CreateStrip() code for laying out the piles and
	// ensuring they do not go off-screen. In this way, we keep the legacy code intact,
	// by hiding the necessary tweaks for a dropdown list phrasebox within the legacy
	// functions for calculating box and gap widths, and for laying out piles in
	// the strips, and ensuring an adequate gap at the pile which is the active one.
	int newBoxWidth = m_pLayout->m_curBoxWidth;
#if defined(_DEBUG) && defined(_EXPAND)
//	wxLogDebug(_T("%s:%s():line %d, Layout's current WIDTH (might get augmented by FixBox):  %d , for box text [ %s ]"),
//		__FILE__, __FUNCTION__, __LINE__, newBoxWidth, m_pLayout->m_pApp->m_targetPhrase.c_str());
#endif

	// The text-based calculations now follow...
	wxSize boxExtent; int boxWidth = 0;
	wxString mytext = wxEmptyString;
	if ((m_pSrcPhrase != NULL) && (m_pSrcPhrase->m_nSequNumber == gpApp->m_nActiveSequNum))
	{
		// Get a device context for the canvas on the stack
		wxClientDC aDC((wxScrolledWindow*)m_pLayout->m_pCanvas); // make a temporary device context
		// We do these calculations only when the pile is the active pile - that is, where
		// the phrasebox is located in the interlinear layout. What we do here must work in
		// all of Adapt It's modes - adapting, glossing, free translating. The code below
		// must get an adequate with, for each mode. Also, at "holes" the m_pTargetBox will
		// be empty; so we need to provide a minimum box width into which to typing is
		// facilitated for a while without a resize; that means, the slop, and the drop down
		// button
		wxUnusedVar(widthMode); // our new protocol may make this enum value unneeded, 
								// it certainly is not needed here
		int nStartingWidth = gpApp->m_nMinPileWidth; // set at 40 I think
		int pileWidth = CalcPileWidth(); // internally itself does a max, based on current text extents
		m_nMinWidth = pileWidth; // because this pile will sometime not be the active one 
								 // (calc gap width also sets this I think, no harm in some redundancy)
		int listWidth = CalcPhraseBoxListWidth();
		if (listWidth == wxNOT_FOUND)
			listWidth = 0; // effectively means we ignore it
		boxWidth = wxMax(nStartingWidth, pileWidth);
		boxWidth = wxMax(boxWidth, listWidth);
		#if defined(_DEBUG) && defined(_EXPAND)
//				wxLogDebug(_T("%s:%s():line %d, initial WIDTHS: starting (no slop,no butn)  %d, CalcPileWidth() %d, listWidth %d , use max - for box text [ %s ]"),
//					__FILE__,__FUNCTION__, __LINE__, nStartingWidth, pileWidth, listWidth, m_pLayout->m_pApp->m_targetPhrase.c_str());
		#endif
		int nSaveInitialWidth = boxWidth;
		// Sanity test
		if (boxWidth < nSaveInitialWidth)
		{
			boxWidth = nSaveInitialWidth;
		}
		
		// Add the slop width
		int slop = gpApp->GetLayout()->slop;
		boxWidth += slop;

		// Finally, add the adjusted width of the dropdown button
		int btnWidth = gpApp->GetLayout()->buttonWidth;
		boxWidth += btnWidth;

#if defined(_DEBUG) && defined(_EXPAND)
//		wxLogDebug(_T("%s:%s():line %d, WIDTHS: starting %d , final width (slop+btn) = %d ,  btn %d , slop %d , box text [ %s ]"),
//			__FILE__, __FUNCTION__, __LINE__, nSaveInitialWidth, boxWidth, btnWidth, slop, mytext.c_str());
#endif
	}
	// Now return whichever of the two widths is the larger
	if (newBoxWidth > boxWidth)
	{
		boxWidth = newBoxWidth;
		// Since CalcPhraseBoxGapWidth() calls CalcPhraseBoxWidth(),
		// this will automatically add the expanded value to the
		// former of these two
	}

// TODO -- calculation for See Glosses, and Glossing mode; also need a CalcGlossListWidth(); and get max

	return boxWidth;
}


//GDLC 2010-02-10 Added parameter to CalcPhraseBoxGapWidth with default value steadyAsSheGoes
// (0 is contracting, 1 is steadyAsSheGoes, 2 is expanding)
// BEW refactored for listbox phrasebox within sizer
int CPile::CalcPhraseBoxGapWidth(enum phraseBoxWidthAdjustMode widthMode)
{
	wxUnusedVar(widthMode);
	// This function should only be called on the active pile. Use m_pApp->m_targetPhrase
	// (the phrase box contents) for the pile extent (plus some slop), because at the
	// active location the m_adaption & m_targetStr members of pSrcPhrase are not set, and
	// won't be until the user hits Enter key to move phrase box on or clicks to place it
	// elsewhere, so only pApp->m_targetPhrase is valid, or the contents of the ptr to the
	// wxTextCtrl itself; note, for version 2 which supports
	// a glossing line, the box will contain a gloss rather than an adaptation whenever
	// gbIsGlossing is TRUE. Glossing could be using the target font, or the navText font.
	int boxGapWidth = m_pLayout->m_nCurGapWidth; // use as a default value to start from
	// Only do the following calculations provided the m_pSrcPhrase pointer is set
	// and that CSourcePhrase instance is the one at the active location, if not so,
	// return PHRASE_BOX_WIDTH_UNSET which has the value -1
	if (m_pSrcPhrase != NULL)
	{
		if (m_pSrcPhrase->m_nSequNumber == m_pLayout->m_pApp->m_nActiveSequNum)
		{
			boxGapWidth = CalcPhraseBoxWidth();
			// add the interpile gap
			boxGapWidth += m_pLayout->GetGapWidth();

#ifdef __wxGTK__
			boxGapWidth += 20; // Bill says Linux controls are wrapped in some whitespace
#endif
#if defined(_DEBUG) && defined(_EXPAND)
//			wxLogDebug(_T("%s():line %d  *** Final phrasebox gap width ***:  %d  , after adding interpile gap: %d (and +20 if Linux)"),
//				__FUNCTION__, __LINE__, boxGapWidth, m_pLayout->GetGapWidth());
#endif
			// BEW comment 14Aug17 The value we have so far obtained should fit the 
			// current box width, as the CreateStrip will will need to know the new
			// augmented value, when expanding, for CreateStrip's calculation of
			// the gap to leave for the phrasebox at the active location.
			// for the pile's nHorzOffset member (which defines where the next pile
			// We can use either create_strips_keep_piles or keep_strips_keep_piles
			// when calling RecalcLayout(). But we need to do this only when the
			// widthMode enum value passed in is 'expanding'; for steadyAsSheGoes
			// there is sufficient slop not to warrant an expansion; and for
			// 'contracting' either we don't bother, or we'll adjust the gap
			// and box width too - the legacy app didn't bother, but I'm open to
			// making the adjustment if it gives a better user experience. The
			// Note, the pileWidth is *not* the same thing as nHorzOffset, pileWidth
			// is a (semi) permanent value intrinsic to the pile, as it has to be
			// the width of the pile when the pile is not the active one
			if (widthMode == expanding)
			{
				CPile* pActivePile = gpApp->m_pActivePile;
				if ((pActivePile != NULL) && (this == pActivePile))
				{
					// Make sure CLayout has the boxMode (synonym for passed in widthMode)
					m_pLayout->m_boxMode = widthMode;
					// The active CStrip, when being re-created, 
					// will grab the enum value from CLayout
#if defined(_DEBUG) && defined(_EXPAND)
//					wxString strExp(_T("expanding"));
//					wxLogDebug(_T("%s:%s():line %d, Layout's m_boxMode:  %s, sequNum: %d , text [ %s ]"),
//						__FILE__, __FUNCTION__, __LINE__, strExp.c_str(), pActivePile->GetSrcPhrase()->m_nSequNumber,
//						m_pLayout->m_pApp->m_targetPhrase.c_str());
#endif
				}
			}
			/* uncomment out if I change my mind about 'contracting' state
			else if (widthMode == contracting)
			{
			// BEW 20Aug18 For contracting, we may need a tweak here. TODO maybe?
			// Testing says it's not needed, just like in the legacy versions
			}
			*/
		}
	}
	else
	{
		// CSourcePhrase pointer was null, so cannot calculate a value
		return PHRASE_BOX_WIDTH_UNSET;
	} // end of else block for test: if (m_pSrcPhrase != NULL)

	return boxGapWidth;
}

CCell** CPile::GetCellArray()
{
	return &m_pCell[0]; // return pointer to the array of CCell pointers
}

void CPile::SetPhraseBoxGapWidth(int nNewWidth)
{
	m_nWidth = nNewWidth; // a useful overload, for when the phrase box is contracting
	// BEW 20Nov12 added line to set m_curBoxWidth, so that a subsequent RecalcLayout()
	// call will use the new gap width just set
	m_pLayout->m_curBoxWidth = nNewWidth;
}

// BEW 22Feb10 some changes done for support of doc version 5
// BEW 18Apr17 now supports new member m_filteredInfo_After via change in HasFilterMarker() call
void CPile::DrawNavTextInfoAndIcons(wxDC* pDC)
{
	bool bRTLLayout;
	bRTLLayout = FALSE;
	wxRect rectBounding;

	//if (this == NULL) // whm 3Oct2018 removed - it generates gcc warning "nonnull argument 'this' compared to NULL
	//	return;
	GetPileRect(rectBounding); // get the bounding rectangle for this CCell instance
#ifdef _RTL_FLAGS
	if (m_pLayout->m_pApp->m_bNavTextRTL)
	{
		// navigation text has to be RTL & aligned RIGHT
		bRTLLayout = TRUE;
	}
	else
	{
		// navigation text has to be LTR & aligned LEFT
		bRTLLayout = FALSE;
	}
#endif

	// get the navText color
	wxColour navColor = m_pLayout->GetNavTextColor();

	// BEW added 18Nov09, background colour change...
	// change to light pink background if m_bReadOnlyAccess is TRUE
	wxColour oldBkColor;
	if (m_pLayout->m_pApp->m_bReadOnlyAccess)
	{
		// make the background be an insipid red colour
		wxColour backcolor(255,225,232,wxALPHA_OPAQUE);
		oldBkColor = pDC->GetTextBackground(); // white
		pDC->SetBackgroundMode(m_pLayout->m_pApp->m_backgroundMode);
		pDC->SetTextBackground(backcolor);
	}
	else
	{
		wxColour backcolor(255,255,255,wxALPHA_OPAQUE); // white
		oldBkColor = pDC->GetTextBackground(); // dunno
		pDC->SetBackgroundMode(m_pLayout->m_pApp->m_backgroundMode);
		pDC->SetTextBackground(backcolor);
	}

//#ifdef _DEBUG
//	if (m_pSrcPhrase->m_nSequNumber == 129)
//	{
//		int halt_here = 1;
//        halt_here = halt_here; // avoid gcc warning
//	}
//#endif

	// stuff below is for drawing the navText stuff above this pile of the strip
	// Note: in the wx version m_bSuppressFirst is now located in the App
	if (!gbShowTargetOnly)
	{
		int xOffset = 0;
		int diff;
		bool bHasFilterMarker = HasFilterMarker();

		// if a message is to be displayed above this word, draw it too
		if (m_pSrcPhrase->m_bNotInKB)
		{
			wxPoint pt;
			TopLeft(pt); //pt = m_ptTopLeft;
#ifdef _RTL_FLAGS
			if (m_pLayout->m_pApp->m_bNavTextRTL)
				pt.x += rectBounding.GetWidth(); // align right
#endif
			// whm: the wx version doesn't use negative offsets
			diff = m_pLayout->GetNavTextHeight() - (m_pLayout->GetSrcTextHeight()/4);
			pt.y -= diff;
			wxString str;
			xOffset += 8;
			if (m_pSrcPhrase->m_bBeginRetranslation)
				str = _T("*# "); // visibly mark start of a retranslation section
			else
				str = _T("* ");
			wxFont SaveFont;
			wxFont* pNavTextFont = m_pLayout->m_pNavTextFont;
			SaveFont = pDC->GetFont(); // save current font
			pDC->SetFont(*pNavTextFont);
			if (!navColor.IsOk())
			{
				::wxBell();
				wxASSERT(FALSE);
			}
			pDC->SetTextForeground(navColor);

			rectBounding.Offset(0,-diff);


			// wx version can only set layout direction directly on the whole pDC.
			// Uniscribe in wxMSW and Pango in wxGTK automatically take care of the
			// right-to-left reading of the text, but we need to manually control
			// the right-alignment of text when we have bRTLLayout. The upper-left
			// x coordinate for RTL drawing of the m_phrase should be
			if (bRTLLayout)
			{
				// *** Draw the RTL Retranslation section marks *# or * in Nav Text area ***
				// whm note: nav text stuff can potentially be much wider than the width of
				// the cell where it is drawn. This would not usually be a problem since nav
				// text is not normally drawn above every cell but just at major markers like
				// at ch:vs points, section headings, etc. For RTL the nav text could extend
				// out and be clipped beyond the left margin.
				m_pCell[0]->DrawTextRTL(pDC,str,rectBounding); // any CCell pointer would do here
			}
			else
			{
				// *** Draw the LTR Retranslation section marks *# or * in Navigation Text area ***
				pDC->DrawText(str,rectBounding.GetLeft(),rectBounding.GetTop());
			}
			pDC->SetFont(SaveFont);
		}

		//////////////////// Draw Green Wedge etc ////////////////////////////////

        // next stuff is for the green wedge - it should be shown at the left or the right
        // of the pile depending on the gbRTL_Layout flag (FALSE or TRUE, respectively),
        // rather than using the nav text's directionality
		if (m_pSrcPhrase->m_bFirstOfType || m_pSrcPhrase->m_bVerse || m_pSrcPhrase->m_bChapter
			|| m_pSrcPhrase->m_bFootnoteEnd || m_pSrcPhrase->m_bHasInternalMarkers
			|| bHasFilterMarker || !m_pSrcPhrase->m_inform.IsEmpty())
		{
			wxPoint pt;
			TopLeft(pt); //pt = m_ptTopLeft;

			// BEW added, for support of filter wedge icon
			if (bHasFilterMarker && !gbShowTargetOnly)
			{
				xOffset = 7;  // offset any nav text 7 pixels to the right (or to the
							  // left if RTL rendering) to make room for the wedge
			}

#ifdef _RTL_FLAGS
			if (m_pLayout->m_pApp->m_bRTL_Layout) // was gpApp->m_bNavTextRTL
				pt.x += rectBounding.GetWidth(); // align right
#endif
			// whm: the wx version doesn't use negative offsets
			diff = m_pLayout->GetNavTextHeight() + (m_pLayout->GetSrcTextHeight()/4) + 1;
			pt.y -= diff;
#ifdef _RTL_FLAGS
			if (m_pLayout->m_pApp->m_bRTL_Layout) // was gpApp->m_bNavTextRTL
			{
				pt.x -= xOffset;
			}
			else
			{
				pt.x += xOffset;
			}
#else
			pt.x += xOffset; // for ANSI version
#endif
			// BEW revised 19 Apr 05 in support of USFM and SFM Filtering
            // m_inform should have, at most, the name or short name of the last nonchapter
            // & nonverse (U)SFM, and no information about anything filtered. All we need
            // do with it is to append this information to str containing n:m if the latter
            // is pertinent here, or fill str with m_inform if there is no chapter or verse
            // info here. The construction of a wedge for signallying presence of filtered
            // info is also handled here, but independently of m_inform
			wxString str = _T("");

			if (bHasFilterMarker && !gbShowTargetOnly)
			{
               // if bHasFilteredMarker is TRUE member then we require a wedge
                // to be drawn here.
                // BEW modified 18Nov05; to have variable colours, also the colours
                // differences are hard to pick up with a simple wedge, so the top of the
                // wedge has been extended up two more pixels to form a column with a point
                // at the bottom, which can show more colour
				wxPoint ptWedge;
				TopLeft(ptWedge);

                // BEW added 18Nov05, to colour the wedge differently if \free is
                // contentless (as khaki), or if \bt is contentless (as pastel blue), or if
                // both are contentless (as red)
				// these functions are now defined in helpers.cpp
				bool bFreeHasNoContent = IsFreeTranslationContentEmpty(m_pSrcPhrase);
				bool bBackHasNoContent = IsBackTranslationContentEmpty(m_pSrcPhrase);

				#ifdef _RTL_FLAGS
				if (m_pLayout->m_pApp->m_bRTL_Layout)
					ptWedge.x += rectBounding.GetWidth(); // align right if RTL layout
				#endif
				// get the point where the drawing is to start (from the bottom tip of
				// the downwards pointing wedge)
				ptWedge.x += 1;
				ptWedge.y -= 2;

                // whm note: According to wx docs, wxWidgets shows all non-white pens as
                // black on a monochrome display, i.e., OLPC screen in Black & White mode.
                // In contrast, wxWidgets shows all brushes as white unless the colour is
                // really black on monochrome displays.
				pDC->SetPen(*wxBLACK_PEN);

				// draw the line to the top left of the wedge
				pDC->DrawLine(ptWedge.x, ptWedge.y, ptWedge.x - 5, ptWedge.y - 5); // end
														// points are not part of the line

				// reposition for the vertical stroke up for the left of the short column
				// do the vertical stroke (endpoint not part of it)
				pDC->DrawLine(ptWedge.x - 4, ptWedge.y - 5, ptWedge.x - 4, ptWedge.y - 7);

				// reposition for the horizontal stroke to the right
				// do the horizontal stroke (endpoint not part of it)
				pDC->DrawLine(ptWedge.x - 4, ptWedge.y - 7, ptWedge.x + 5, ptWedge.y - 7);


				// reposition for the vertical stroke down for the right of the short column
				// do the vertical stroke (endpoint not part of it)
				pDC->DrawLine(ptWedge.x + 4, ptWedge.y - 6, ptWedge.x + 4, ptWedge.y - 4);

				// reposition for the stroke down to the left to join up with start position
				// make the final stroke
				pDC->DrawLine(ptWedge.x + 4, ptWedge.y - 4, ptWedge.x, ptWedge.y);
                // we can quickly fill the wedge by brute force by drawing a few green
                // horizontal lines rather than using more complex region calls
                // BEW on 18Nov05 added extra tests and colouring code to support variable
                // colour to indicate when free translation or back translation fields are
                // contentless

				if (!bBackHasNoContent)
				{
					// BEW 27Mar10, a new use for pastel blue, filtered info which
					// includes collected back translation information
#if wxCHECK_VERSION(2,9,0)					
					pDC->SetPen(wxPen(wxColour(145,145,255), 1, wxPENSTYLE_SOLID));
#else
					pDC->SetPen(wxPen(wxColour(145,145,255), 1, wxSOLID));
#endif
				}
				else if (bFreeHasNoContent && bBackHasNoContent && m_pSrcPhrase->m_bHasNote)
				{
					// no free trans nor back trans, but a note -- red grabs attention best
					pDC->SetPen(*wxRED_PEN);
				}
				else if (bFreeHasNoContent && m_pSrcPhrase->m_bStartFreeTrans)
				{
					// khaki for an empty free translation
					if (bFreeHasNoContent)
					{
						// colour it khaki
#if wxCHECK_VERSION(2,9,0)					
						pDC->SetPen(wxPen(wxColour(160,160,0), 1, wxPENSTYLE_SOLID));
#else
						pDC->SetPen(wxPen(wxColour(160,160,0), 1, wxSOLID));
#endif
					}
				}
				else if (bFreeHasNoContent && bBackHasNoContent && !m_pSrcPhrase->m_bHasNote)
				{
					// a new colour, cyan wedge if the only filtered information is in the
					// m_filteredInfo member
					pDC->SetPen(*wxCYAN_PEN);
				}
				else
				{
					// if not one of the special situations, colour it green
					pDC->SetPen(*wxGREEN_PEN);
				}

				// draw the two extra lines for the short column's colour
				pDC->DrawLine(ptWedge.x - 3, ptWedge.y - 6, ptWedge.x + 4, ptWedge.y - 6);
				pDC->DrawLine(ptWedge.x - 3, ptWedge.y - 5, ptWedge.x + 4, ptWedge.y - 5);
				// draw the actual wedge (ie. triangle) shape below the column
				pDC->DrawLine(ptWedge.x - 3, ptWedge.y - 4, ptWedge.x + 4, ptWedge.y - 4);
				pDC->DrawLine(ptWedge.x - 2, ptWedge.y - 3, ptWedge.x + 3, ptWedge.y - 3);
				pDC->DrawLine(ptWedge.x - 1, ptWedge.y - 2, ptWedge.x + 2, ptWedge.y - 2);
				pDC->DrawLine(ptWedge.x , ptWedge.y - 1, ptWedge.x + 1, ptWedge.y - 1);

				pDC->SetPen(wxNullPen); // wxNullPen causes the current pen to be
                        // selected out of the device context, and the original pen
                        // restored.
			} // end of TRUE block for test: if (bHasFilterMarker && !gbShowTargetOnly)

			// make (for version 3) the chapter&verse information come first
			if (m_pSrcPhrase->m_bVerse || m_pSrcPhrase->m_bChapter)
			{
				str = m_pSrcPhrase->m_chapterVerse;

                // spurious bug: bogus wxString buffer overrun chars (anything for 20 to 60
                // or so) shown after nav text m:n of final pile's CSourcePhrase's
                // m_chapVerse text, that is, for the last CSourcePhrase instance in the
                // doc which happens to be composed of contentless USFM markup from
                // Paratext - I've never had this wxString overrun before, and it appears
                // to be a wx error, not one of ours; the error doesn't happen if the file
                // is saved and then reloaded - all is drawn properly in that case)

				// the following kludge works, so make it permanent code
				PileList* pPileList = m_pLayout->GetPileList();
				PileList::Node* pos = pPileList->GetLast();
				CPile* pLastPile = pos->GetData();
				if (this == pLastPile)
				{
                    // Chop off the bogus chars but only when the pile is the last one --
					wxString firstBit;
					int offset = str.Find(_T(":"));
					if (offset != wxNOT_FOUND)
					{
						firstBit = str.Left(offset + 1);
						str = str.Mid(offset + 1);
						int index = 0;
						// Code is safe so far, but now we have the possibility of
						// a bounds error in the loop below, so get the str length and 
						// ensure we don't try access beyond the end of the string
						int count = str.Len();
						// test for digits, letters like a, b, or c, hyphen, comma or
						// period - these are most likely the chars which will be in
						// bridged verse or part verse if the verse isn't a simple one or
						// more digits string; anything else, exit the loop
						while( index < count &&
							  (IsAnsiLetterOrDigit(str[index]) || str[index] == _T('-')
							   || str[index] == _T(',') || str[index] == _T('.'))
							 )
						{
							firstBit += str[index];
							index++;
						}
						str = firstBit;
					}
				}

#ifdef _SHOW_CHAP_VERSE
#ifdef _DEBUG
				// the wxString Len() value counts the bogus extra characters if they are
				// "there", so the wxLogDebug will display the error if the kludge above
				// is commented out
				wxLogDebug(_T("Draw Navtext: str set by m_chapterVerse = %s , sn = %d  , str Length: %d"),
					str.c_str(), m_pSrcPhrase->m_nSequNumber, str.Len());
#endif
#endif
			}

            // now append anything which is in the m_inform member; there may not have been
            // a chapter and/or verse number already placed in str, so allow for this
            // possibility
			if (!m_pSrcPhrase->m_inform.IsEmpty())
			{
				if (str.IsEmpty())
					str = m_pSrcPhrase->m_inform;
				else
				{
					str += _T(' ');
					str += m_pSrcPhrase->m_inform;
				}
			}

			// whm added for testing 12Mar09 adding "end fn" to nav text at end of footnote
			// (Requested by Wolfgang Stradner)
			// BEW refactored the block on 3Mar15 because if the background was erased but
			// a redraw of the text doesn't happen, the "end fn" disappeared. (That was
			// Reported by Warren Glover by email on 25Feb15, and I've seen it on rare
			// occasions too.) So I've put the code to add the "end fn" to the pSrcPhrase
			// which gets m_bFootnoteEnd set to TRUE in TokenizeText() -- see Adapt_ItDoc
			// at approx  line 16896. When that is done, the kludge below is not needed.
			// Kludge: old documents that do not have m_inform with "end fn" at the 
			// last CSourcePhrase of a footnote, will not show any nav text unless we
			// make a test here and if it is not present, then add it to m_inform
			// so it will be present thereafter, and also add it to str so it is
			// displayed at this draw call
			if (m_pSrcPhrase->m_bFootnoteEnd)
			{
				wxString theEndTxt = _("end fn"); // it is localizable
				int offset = m_pSrcPhrase->m_inform.Find(theEndTxt);
				if (offset == wxNOT_FOUND)
				{
					// There is no "end fn" present in the m_inform member, so add it,
					// and make the document 'dirty' so as to get it made persistent
					m_pSrcPhrase->m_inform = theEndTxt;
					m_pLayout->m_pApp->GetDocument()->Modify(TRUE);
					// And append to str to have it drawn
					if (str.IsEmpty())
						str = theEndTxt;
					else
					{
						str += _T(' ');
						str += theEndTxt;
					}
				}
			}
/* a block further above now does this
			// BEW 30Sep19 for \esbe endmarker, the nav text "study Bible sidebar end" is
			// not getting shown, even though it's in the m_inform member. So perhaps
			// a block to do the kludge above, when \esbe is stored in m_endMarkers,
			// will do the trick.

			if (!m_pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				wxString strEndMarkers = m_pSrcPhrase->GetEndMarkers();
				wxString esbeMkr = _T("\\esbe");
				int offset = strEndMarkers.Find(esbeMkr);
				if (offset != wxNOT_FOUND)
				{
					// found \esbe  in m_endMarkers member
					wxString endNavText = _("study Bible sidebar end"); // localizable
					m_pSrcPhrase->m_inform = endNavText;
					m_pLayout->m_pApp->GetDocument()->Modify(TRUE);
					// And append to str to have it drawn
					if (str.IsEmpty())
						str = endNavText;
					else
					{
						str += _T(' ');
						str += endNavText;
					}
				}
			}
*/
			wxFont aSavedFont;
			wxFont* pNavTextFont = m_pLayout->m_pNavTextFont;
			aSavedFont = pDC->GetFont();
			pDC->SetFont(*pNavTextFont);
			if (!navColor.IsOk())
			{
				::wxBell();
				wxASSERT(FALSE);
			}
			pDC->SetTextForeground(navColor);

            // BEW modified 25Nov05 to move the start of navText to just after the green
            // wedge when filtered info is present, because for fonts with not much
            // internal leading built in, the nav text overlaps the top few pixels of the
            // wedge

			if (bHasFilterMarker)
			{
#ifdef _RTL_FLAGS
// we need to make extra space available for some data configurations
				if (m_pLayout->m_pApp->m_bRTL_Layout)
				{
					// right to left layout
					if (m_pLayout->m_pApp->m_bNavTextRTL)
						rectBounding.Offset(-xOffset, -diff); // navText is RTL
					else
						rectBounding.Offset(0, -diff); // navText is LTR
				}
				else
				{
					// left to right layout
					if (m_pLayout->m_pApp->m_bNavTextRTL)
						rectBounding.Offset(0, -diff); // navText is RTL
					else
						rectBounding.Offset(xOffset, -diff); // navText is LTR
				}
#else
rectBounding.Offset(xOffset, -diff);
#endif
			}
			else
			{
			// no filter marker, so we don't need to make extra space
			rectBounding.Offset(0, -diff);
			}

			// wx version sets layout direction directly on the pDC
			if (bRTLLayout)
			{
				// whm note: nav text stuff can potentially be much wider than the width of
				// the cell where it is drawn. This would not usually be a problem since nav
				// text is not normally drawn above every cell but just at major markers like
				// at ch:vs points, section headings, etc. For RTL the nav text could extend
				// out and be clipped beyond the left margin.
				// ** Draw RTL Actual Ch:Vs and/or m_inform Navigation Text **
				m_pCell[0]->DrawTextRTL(pDC, str, rectBounding); // any CCell pointer would do
			}
			else
			{
				// *** Draw LTR Actual Ch:Vs and/or m_inform Navigation Text ***
				pDC->DrawText(str, rectBounding.GetLeft(), rectBounding.GetTop());
			}
			pDC->SetFont(aSavedFont);
		}

		// now note support
		if (m_pSrcPhrase->m_bHasNote)
		{
			wxPoint ptNote;
			TopLeft(ptNote);
			// offset top left (-13,-9) for regular app
#ifdef _RTL_FLAGS
			if (m_pLayout->m_pApp->m_bRTL_Layout)
			{
				ptNote.x += rectBounding.GetWidth(); // align right if RTL layout
				ptNote.x += 7;
			}
			else
			{
				ptNote.x -= 13;
			}
#else
			ptNote.x -= 13;
#endif
			ptNote.y -= 9;

			// create a brush
			// whm note: According to wx docs, wxWidgets shows all brushes as white unless
			// the colour is really black on monochrome displays, i.e., the OLPC screen in
			// Black & White mode. In contrast, wxWidgets shows all non-white pens as black
			// on monochrome displays.
#if wxCHECK_VERSION(2,9,0)					
			pDC->SetBrush(wxBrush(wxColour(254, 218, 100), wxBRUSHSTYLE_SOLID));
#else
			pDC->SetBrush(wxBrush(wxColour(254, 218, 100), wxSOLID));
#endif
			pDC->SetPen(*wxBLACK_PEN); // black - whm added 20Nov06
			wxRect insides(ptNote.x, ptNote.y, ptNote.x + 9, ptNote.y + 9);
			// MFC CDC::Rectangle() draws a rectangle using the current pen and fills the
			// interior using the current brush
			pDC->DrawRectangle(ptNote.x, ptNote.y, 9, 9); // rectangles are drawn with a
													   // black border
			pDC->SetBrush(wxNullBrush); // restore original brush - wxNullBrush causes
					// the current brush to be selected out of the device context, and the
					// original brush restored.
			// now the two spirals at the top - left one, then right one
			pDC->DrawLine(ptNote.x + 3, ptNote.y + 1, ptNote.x + 3, ptNote.y + 3);
			pDC->DrawLine(ptNote.x + 2, ptNote.y + 2, ptNote.x + 2, ptNote.y + 3);
			// right one
			pDC->DrawLine(ptNote.x + 6, ptNote.y + 1, ptNote.x + 6, ptNote.y + 3);
			pDC->DrawLine(ptNote.x + 5, ptNote.y + 2, ptNote.x + 5, ptNote.y + 3);
			// get rid of the pen
			pDC->SetPen(wxNullPen);
		}
	}
	// BEW 5Jul21 added the else block, so that limited amount of nav text information
	// gets displayed in the whiteboard area when no source text is visible -- e.g. 
	// chapter and verse, and the asterisks for where a retranslation is located
	else
	{
		int xOffset = 0;
		int diff;
		//bool bHasFilterMarker = HasFilterMarker();  // we are not supporting wedges display
													// when source is not shown
#if defined (_DEBUG)
		if ((gpApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber == 4842) && gbShowTargetOnly)
		{
			int halt_here = 1;
		}

#endif
		// if a message is to be displayed above this word, draw it too
		if (m_pSrcPhrase->m_bNotInKB)
		{
			wxPoint pt;
			TopLeft(pt); //pt = m_ptTopLeft;
#ifdef _RTL_FLAGS
			if (m_pLayout->m_pApp->m_bNavTextRTL)
				pt.x += rectBounding.GetWidth(); // align right
#endif
			// whm: the wx version doesn't use negative offsets
			diff = m_pLayout->GetNavTextHeight() - (m_pLayout->GetSrcTextHeight() / 4);
			pt.y -= diff;
			wxString str;
			xOffset += 8;
			if (m_pSrcPhrase->m_bBeginRetranslation)
				str = _T("*# "); // visibly mark start of a retranslation section
			else
				str = _T("* ");
			wxFont SaveFont;
			wxFont* pNavTextFont2 = m_pLayout->m_pNavTextFont;
			SaveFont = pDC->GetFont(); // save current font
			pDC->SetFont(*pNavTextFont2);
			if (!navColor.IsOk())
			{
				::wxBell();
				wxASSERT(FALSE);
			}
			pDC->SetTextForeground(navColor);

			rectBounding.Offset(0, -diff);


			// wx version can only set layout direction directly on the whole pDC.
			// Uniscribe in wxMSW and Pango in wxGTK automatically take care of the
			// right-to-left reading of the text, but we need to manually control
			// the right-alignment of text when we have bRTLLayout. The upper-left
			// x coordinate for RTL drawing of the m_phrase should be
			if (bRTLLayout)
			{
				// *** Draw the RTL Retranslation section marks *# or * in Nav Text area ***
				// whm note: nav text stuff can potentially be much wider than the width of
				// the cell where it is drawn. This would not usually be a problem since nav
				// text is not normally drawn above every cell but just at major markers like
				// at ch:vs points, section headings, etc. For RTL the nav text could extend
				// out and be clipped beyond the left margin.
				m_pCell[0]->DrawTextRTL(pDC, str, rectBounding); // any CCell pointer would do here
			}
			else
			{
				// *** Draw the LTR Retranslation section marks *# or * in Navigation Text area ***
				pDC->DrawText(str, rectBounding.GetLeft(), rectBounding.GetTop());
			}
			pDC->SetFont(SaveFont);

			wxFont aSavedFont;
			wxFont* pNavTextFont = m_pLayout->m_pNavTextFont;
			aSavedFont = pDC->GetFont();
			pDC->SetFont(*pNavTextFont);
			if (!navColor.IsOk())
			{
				::wxBell();
				wxASSERT(FALSE);
			}
			pDC->SetTextForeground(navColor);

/*
		if (bHasFilterMarker)
		{
#ifdef _RTL_FLAGS
			// we need to make extra space available for some data configurations
			if (m_pLayout->m_pApp->m_bRTL_Layout)
			{
				// right to left layout
				if (m_pLayout->m_pApp->m_bNavTextRTL)
					rectBounding.Offset(-xOffset, -diff); // navText is RTL
				else
					rectBounding.Offset(0, -diff); // navText is LTR
			}
			else
			{
				// left to right layout
				if (m_pLayout->m_pApp->m_bNavTextRTL)
					rectBounding.Offset(0, -diff); // navText is RTL
				else
					rectBounding.Offset(xOffset, -diff); // navText is LTR
			}
#else
			rectBounding.Offset(xOffset, -diff);
#endif
		}
		else
		{
*/
			// no filter marker, so we don't need to make extra space
			rectBounding.Offset(0, -diff);
//		}

		// wx version sets layout direction directly on the pDC
		if (bRTLLayout)
		{
			// whm note: nav text stuff can potentially be much wider than the width of
			// the cell where it is drawn. This would not usually be a problem since nav
			// text is not normally drawn above every cell but just at major markers like
			// at ch:vs points, section headings, etc. For RTL the nav text could extend
			// out and be clipped beyond the left margin.
			// ** Draw RTL Actual Ch:Vs and/or m_inform Navigation Text **
			m_pCell[0]->DrawTextRTL(pDC, str, rectBounding); // any CCell pointer would do
		}
		else
		{
			// *** Draw LTR Actual Ch:Vs and/or m_inform Navigation Text ***
			pDC->DrawText(str, rectBounding.GetLeft(), rectBounding.GetTop());
		}
		pDC->SetFont(aSavedFont);
		}
	}

}

// return TRUE if the CSourcePhrase pointed at by this CPile is one which has a marker
// which causes text wrap in a USFM-marked up document (eg. \p, \m, etc)
// BEW 22Feb10, no changes for support of doc version 5 (but the called function,
// IsWrapMarker() does have changes for doc version 5)
bool CPile::IsWrapPile()
{
	CSourcePhrase* pSrcPhrase = this->m_pSrcPhrase;
	if (!pSrcPhrase->m_markers.IsEmpty())
	{
		// this a potential candidate for starting a new strip, so check it out
		if (pSrcPhrase->m_nSequNumber > 0)
		{
			if (m_pLayout->m_pView->IsWrapMarker(pSrcPhrase))
			{
				return TRUE; // if we need to wrap, discontinue assigning piles to
                             // this strip (the nPileIndex_InList value is already correct
                             // for returning to caller)
			}
		}
	}
	return FALSE;
}

// BEW 22Feb10, no changes for support of doc version 5
void CPile::PrintPhraseBox(wxDC* pDC)
{
	wxTextCtrl* pBox = m_pLayout->m_pApp->m_pTargetBox->GetTextCtrl(); // whm 14Feb2018 added ->GetTextCtrl()
	wxASSERT(pBox);
	wxRect rectBox;
	int width;
	int height;
	if (pBox != NULL && m_pLayout->m_pApp->m_nActiveSequNum != -1)
	{
		if (m_pSrcPhrase->m_nSequNumber == m_pLayout->m_pApp->m_nActiveSequNum)
		{
			rectBox = pBox->GetRect();
			width = rectBox.GetWidth(); // these will be old MM_TEXT mode
										// logical coords, but
			height = rectBox.GetHeight(); // that will not matter

			wxPoint topLeft;
			m_pCell[1]->TopLeft(topLeft);
			// Note: GetMargins not supported in wxWidgets' wxTextCtrl (nor MFC's RichEdit3)
			int leftMargin = 2; // we'll hard code 2 pixels on left as above - check this ???
			wxPoint textTopLeft = topLeft;
			textTopLeft.x += leftMargin;
			int topMargin;
			if (gbIsGlossing && gbGlossingUsesNavFont)
				topMargin = abs((height - m_pLayout->GetNavTextHeight())/2); // whm this is OK
			else
				topMargin = abs((height - m_pLayout->GetTgtTextHeight())/2); // whm this is OK
			textTopLeft.y -= topMargin;
			wxFont SaveFont;
			wxFont TheFont;
			if (gbIsGlossing && gbGlossingUsesNavFont)
				TheFont = *m_pLayout->m_pNavTextFont;
			else
				TheFont = *m_pLayout->m_pTgtFont;
			SaveFont = pDC->GetFont();
			pDC->SetFont(TheFont);

			wxColor color = m_pCell[1]->GetColor(); // get the default colour of
													// this cell's text
			if (!color.IsOk())
			{
				::wxBell();
				wxASSERT(FALSE);
			}
			pDC->SetTextForeground(color); // use color for this cell's text to print
										   // the box's text

			// **** Draw the Target Text for the phrasebox ****
			pDC->DrawText(m_pLayout->m_pApp->m_targetPhrase,textTopLeft.x,textTopLeft.y);
					// MFC uses TextOut()  // Note: diff param ordering!
			pDC->SetFont(SaveFont);

			// ***** Draw the Box around the target text ******
			pDC->SetPen(*wxBLACK_PEN); // whm added 20Nov06

            // whm: wx version flips top and bottom when rect coords are negative to
            // maintain true "top" and "bottom". In the DrawLine code below MFC has -height
            // but the wx version has +height.
			pDC->DrawLine(topLeft.x, topLeft.y, topLeft.x+width, topLeft.y);
			pDC->DrawLine(topLeft.x+width, topLeft.y, topLeft.x+width, topLeft.y +height);
			pDC->DrawLine(topLeft.x+width, topLeft.y+height, topLeft.x, topLeft.y +height);
			pDC->DrawLine(topLeft.x, topLeft.y+height, topLeft.x, topLeft.y);
			pDC->SetPen(wxNullPen);

		}
	}
}

// BEW 22Feb10, no changes for support of doc version 5
// BEW addition 22Dec14, to support Seth Minkoff's request that just the selected 
// src will be hidden when the Show Target Text Only button is pressed.
void CPile::Draw(wxDC* pDC)
{
	// draw the cells this CPile instance owns, MAX_CELLS = 3; BEW changed 22Dec14 for Seth Minkoff request
	if (!gbShowTargetOnly || (m_pLayout->m_pApp->m_selectionLine == 0)) // BEW 22Dec14 added 2nd subtest
	{
		m_pCell[0]->Draw(pDC);
	}
	/* Don't think I need do it this way, tweaking CCell::GetColor() is better
    //BEW added 13May11 to implement a feature requested by Patrick Rosendall on 27Aug2009,
    //to use a different colour for a target text word/phase which differs in spelling from
    //the source text word/phrase
	CSourcePhrase* pSrcPhrase = GetSrcPhrase();
	wxColour diffColour = gpApp->m_navTextColor; // temporarily avoids a GUI addition of a button
	//wxColour diffColour = wxColour(160,30,120); // a solid pickish red, darkish but not too much
	wxColour oldColour;
	if (pSrcPhrase->m_key != pSrcPhrase->m_adaption)
	{
		// change the colour for this pile's target text
		oldColour = pDC->GetTextForeground();
		pDC->SetTextForeground(diffColour);
		m_pCell[1]->DrawCell(pDC, diffColour); // always draw the line which has the phrase box
		pDC->SetTextForeground(oldColour);
	}
	else
	{
		m_pCell[1]->Draw(pDC); // always draw the line which has the phrase box
	}
	*/
	if (!m_pLayout->m_pApp->m_bIsPrinting ||
		(m_pLayout->m_pApp->m_bIsPrinting && !gbIsGlossing))
	{
#if defined(_DEBUG)
		//if (m_pCell[1]->GetPile() == m_pLayout->m_pApp->m_pActivePile)
		//{
		//	int break_here = 1;
		//}
#endif
		m_pCell[1]->Draw(pDC); // always draw the line which has the phrase box
	}

	// klb 9/2011 - We need to draw the Gloss to screen if "See Glosses" is checked (gbGlossingVisible=true) OR
	// to printer if "Include Glosses text" (gbCheckInclGlossesText) is checked in PrintOptionsDialog - 9/9/2011
	if ((gbGlossingVisible && !m_pLayout->m_pApp->m_bIsPrinting) ||
		(m_pLayout->m_pApp->m_bIsPrinting && gbCheckInclGlossesText))
	{
		// when gbGlossingVisible=TRUE, that means the menu item "See Glosses" has been ticked; but we may
		// or may not still be in adapting mode, but a further test is not required
		// because three lines at must be shown (it's what's in middle & bottom lines
		// which changes)
		m_pCell[2]->Draw(pDC);
	}

	// nav text whiteboard drawing for this pile...
	// whm removed !gbIsPrinting from the following test to include nav text info and
	// icons in print and print preview
	if (!gbShowTargetOnly) //if (!gbIsPrinting && !gbShowTargetOnly)
	{
		DrawNavTextInfoAndIcons(pDC);
	}

	// draw the phrase box if it belongs to this pile
	if (m_pLayout->m_pApp->m_bIsPrinting)
	{
		PrintPhraseBox(pDC); // internally checks if this is active location
	}
}
