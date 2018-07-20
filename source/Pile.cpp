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

#define _NEWDRAW

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
//#include "SourceBundle.h"
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
}

//GDLC 2010-02-10 Added parameter to SetPhraseBoxGapWidth with default value steadyAsSheGoes
void CPile::SetPhraseBoxGapWidth(enum phraseBoxWidthAdjustMode widthMode)
{
	m_nWidth = CalcPhraseBoxGapWidth(widthMode);
}

int	CPile::GetMinWidth()
{
	return m_nMinWidth;
}

int	CPile::GetPhraseBoxGapWidth()
{
	return m_nWidth;
}

int	CPile::GetPhraseBoxWidth() //BEW added 19Jul18, gets m_currBoxWidth value
{
	return gpApp->m_pLayout->m_curBoxWidth;
}

void CPile::SetPhraseBoxWidth(enum phraseBoxWidthAdjustMode widthMode)
{
	gpApp->m_pLayout->m_curBoxWidth = CalcPhraseBoxWidth(widthMode);
	// BEW 20Jul18 despite the claim in CalcPileWith() that setting m_nMinWidth
	// for the active location can be left to RecalcLayout(), in the refactored
	// drawing protocol (supporting dropdown phrasebox) it does not appear to 
	// be the case that that is always sufficient. So, I'll add the call here
	// to get m_nMinWidth updated for the active pile too
	SetMinWidth();
}

void CPile::SetPhraseBoxWidth(int boxwidth)
{
	gpApp->m_pLayout->m_curBoxWidth = boxwidth;
}

// Calculates the pile's width before laying out the current pile in a strip. The function
// is not interested in the relative ordering of the glossing and adapting cells, and so
// does not access CCell instances; rather, it just examines extent of the three text
// members m_srcPhrase, m_targetStr, m_gloss on the CSourcePhrase instance pointed at by
// this particular CPile instance. The width is the maximum extent.x for the three strings
// checked.
int CPile::CalcPileWidth()
{
	int pileWidth = gpApp->m_nMinPileWidth; // was 40; BEW changed 19May15 // was 40; // ensure we never get a pileWidth of zero

	// get a device context for the canvas on the stack (wont' accept uncasted definition)
	wxClientDC aDC((wxScrolledWindow*)m_pLayout->m_pCanvas); // make a temporary device context
	wxSize extent;
	aDC.SetFont(*m_pLayout->m_pSrcFont); // works, now we are friends
	aDC.GetTextExtent(m_pSrcPhrase->m_srcPhrase, &extent.x, &extent.y);
	pileWidth = extent.x; // can assume >= to key's width, as differ only by possible punctuation
	if (!m_pSrcPhrase->m_targetStr.IsEmpty())
	{
		aDC.SetFont(*m_pLayout->m_pTgtFont);
		aDC.GetTextExtent(m_pSrcPhrase->m_targetStr, &extent.x, &extent.y);
		if (extent.x > pileWidth)
		{
			pileWidth = extent.x;
		}
	}
	if (!m_pSrcPhrase->m_gloss.IsEmpty())
	{
		if (gbGlossingUsesNavFont)
			aDC.SetFont(*m_pLayout->m_pNavTextFont);
		else
			aDC.SetFont(*m_pLayout->m_pTgtFont);
		aDC.GetTextExtent(m_pSrcPhrase->m_gloss, &extent.x, &extent.y);
		if (extent.x > pileWidth)
		{
			pileWidth = extent.x;
		}
	}
	// BEW added next two lines, 14Jul11, supposedly they aren't necessary, but if
	// m_srcPhrase and m_targetStr and m_gloss are each empty, pileWidth somehow ends up
	// with a value of 0, and so this fixes that
	// BEW changed 19May15
	if (pileWidth < gpApp->m_nMinPileWidth) // was40)
		pileWidth = gpApp->m_nMinPileWidth; // was 40; BEW changed 19May15
	//if (pileWidth < 40)
	//	pileWidth = 40;
	return pileWidth;
}

int CPile::CalcPhraseBoxWidth(enum phraseBoxWidthAdjustMode widthMode)
{
	// box width is what we'll compute to return to caller. It starts out as a
	// "basic width" computed from the text extent for the adaptation, including
	// any punctuation; and then we add to that the width of the user's chosen
	// slop size for the wxTextCtrl (we compute that as a multiple of 'f' character
	// widths; and then dynamically get the width of the dropdown list's button.
	// The result should be then less (by a little bit, or maybe by much if the
	// source text is longer) than the phrasebox gap width computed separately
	// by CalcPhraseBoxGapWidth()
	int boxWidth = gpApp->m_nMinPileWidth; // ensure we never get a width of zero

	// get a device context for the canvas on the stack
	wxClientDC aDC((wxScrolledWindow*)m_pLayout->m_pCanvas); // make a temporary device context
	wxSize boxExtent;
	wxString mytext = wxEmptyString;
	if ((m_pSrcPhrase != NULL) && (m_pSrcPhrase->m_nSequNumber == gpApp->m_nActiveSequNum))
	{
		// We do these calculations only when the pile is the active pile - that is, where
		// the phrasebox is located in the interlinear layout. What we do here must work in
		// all of Adapt It's modes - adapting, glossing, free translating. The code below
		// must get an adequate with, for each mode. Also, at "holes" the m_pTargetBox will
		// be empty; so we need to provide a minimum box width into which to typing is
		// facilitated for a while without a resize; that means, the slop, and the drop down
		// button
		wxUnusedVar(widthMode); // our new protocol may make this enum value unneeded, 
								// it certainly is not needed here
		// First, what's in the wxTextCtrl? - get it's extent. This could be tgt text, or
		// gloss text; but if at a hole, the box will need to be wide enough to accomodate
		// the copied source text. Impliment that understanding.
		mytext = gpApp->m_pTargetBox->GetTextCtrl()->GetValue();
		if (mytext.IsEmpty())
		{
			// The active location is a hole.

			// Check out the source text's width, and use that instead. 
			// Then add slop and also the width of the dropdown button.
			mytext = m_pSrcPhrase->m_srcPhrase; // it's never empty in our layout
			aDC.SetFont(*m_pLayout->m_pSrcFont);
			aDC.GetTextExtent(mytext, &boxExtent.x, &boxExtent.y);
			if (boxExtent.x > boxWidth)
			{
				boxWidth = boxExtent.x;
			}

			// Calculate the phrasebox slop, based on the width of 'f' characters.
			// Our box slop (whitespace where more chars can be typed without a resize of 
			// the box being needed) is calculated by multiplying the width of an f 
			// character (less 1 pixel) by m_nExpandBox - a user-settable value in Preferences...
			wxChar aChar = _T('f'); 
			wxString fStr = aChar;
			wxSize charSize;
			aDC.GetTextExtent(fStr, &charSize.x, &charSize.y);
			// Monospaced fonts may have the width of an 'f' character the same as a 'w', so 
			// subtract 1 to ensure we are working with a width less than that of 'w'
			// (width of 'w' is used for the phrasebox gap calculation, done elsewhere)
			int slop = gpApp->m_nExpandBox*(charSize.x - 1);
			boxWidth += slop;
			
			// Finally, add the adjusted width of the dropdown button
			wxSize buttonSize = gpApp->m_pTargetBox->GetPhraseBoxButton()->GetSize();
			int adjustedButtonWidth = buttonSize.GetX() + 1; // allow 1 pixel of space before it
			if (buttonSize.x > 0)
			{
				boxWidth += adjustedButtonWidth;
			}

		} // end of TRUE block for test: if (mytext.IsEmpty())
		else
		{
			// There is text in the phrasebox, either an adaptation, or a gloss
			if (gbIsGlossing)
			{
				// Glossing mode is current
				if (gbGlossingUsesNavFont)
				{
					aDC.SetFont(*m_pLayout->m_pNavTextFont); // using NavText font
				}
				else 
				{
					aDC.SetFont(*m_pLayout->m_pTgtFont); // it's using the target font
				}

			}
			else
			{
				// Adapting mode is current
				aDC.SetFont(*m_pLayout->m_pTgtFont); // it's using the target font
			}
			aDC.GetTextExtent(mytext, &boxExtent.x, &boxExtent.y);

			// Set the boxWidth, with m_nMinPileWidth as a lower bound on width
			if (boxWidth > boxExtent.x)
			{
				boxWidth = boxExtent.x;
			}

			// Calculate the phrasebox slop, based on the width of 'f' characters.
			// Our box slop (whitespace where more chars can be typed without a resize of 
			// the box being needed) is calculated by multiplying the width of an f 
			// character (less 1 pixel) by m_nExpandBox - a user-settable value in Preferences...
			wxChar aChar = _T('f');
			wxString fStr = aChar;
			wxSize charSize;
			aDC.GetTextExtent(fStr, &charSize.x, &charSize.y);
			// Monospaced fonts may have the width of an 'f' character the same as a 'w', so 
			// subtract 1 to ensure we are working with a width less than that of 'w'
			// (width of 'w' is used for the phrasebox gap calculation, done elsewhere)
			int slop = gpApp->m_nExpandBox*(charSize.x - 1);
			boxWidth += slop;

			// Finally, add the adjusted width of the dropdown button
			wxSize buttonSize = gpApp->m_pTargetBox->GetPhraseBoxButton()->GetSize();
			int adjustedButtonWidth = buttonSize.GetX() + 1; // allow 1 pixel of space before it
			if (buttonSize.x > 0)
			{
				boxWidth += adjustedButtonWidth;
			}

			/* I think the logic below is now bogus - we don't want to augment an earlier location's width value when relocating to a shorter gap - comment out for now
			// If, due to user's typing within the phrasebox, the m_curBoxWidth value is already
			// larger than the value for boxWidth we've computed there, then add the slop amount
			// to accomodate any additional typing that may be required
			if (widthMode == expanding)
			{
				if (m_pLayout->m_curBoxWidth > boxWidth)
				{
					boxWidth = m_pLayout->m_curBoxWidth + slop;
				}
			}
			*/
		} // end of else block for test: if (mytext.IsEmpty())
	}
	return boxWidth;
}


//GDLC 2010-02-10 Added parameter to CalcPhraseBoxGapWidth with default value steadyAsSheGoes
// (0 is contracting, 1 is steadyAsSheGoes, 2 is expanding)
// BEW refactored for listbox phrasebox within sizer
int CPile::CalcPhraseBoxGapWidth(enum phraseBoxWidthAdjustMode widthMode)
{
    // is this pile the active one? If so, get a pile width using m_pApp->m_targetPhrase
    // (the phrase box contents) for the pile extent (plus some slop), because at the
    // active location the m_adaption & m_targetStr members of pSrcPhrase are not set, and
    // won't be until the user hits Enter key to move phrase box on or clicks to place it
    // elsewhere, so only pApp->m_targetPhrase is valid, or the contents of the ptr to the
	// wxTextCtrl itself; note, for version 2 which supports
    // a glossing line, the box will contain a gloss rather than an adaptation whenever
    // gbIsGlossing is TRUE. Glossing could be using the target font, or the navText font.
	wxClientDC aDC((wxScrolledWindow*)m_pLayout->m_pCanvas); // make a temporary device context
	wxSize extent;
	// boxGapWidth is what we return to the caller. When drawing for the active location,
	// the pile there has to have sufficient width -- and the drawing will need to make
	// use of the active pile's m_nWidth value which is set by asigning this function's
	// return value to m_nWidth. (A similar CPile member, m_nMinWidth is used for pile
	// drawing at non-active locations - and therefore does not need to account for the
	// dropdown list button. Pile width is the max of all text extents, src, tgt, and
	// possibly gloss. That calculation is done by CalcPileWidth(). We need to use it
	// here, and add the button width to widen the pile sufficiently for the dropdown
	// button. The interpile gap is added when a strip is created, so we don't need
	// to do anthing here with that layout parameter. I'm going to refactor the messy
	// legacy code & its logging, with a simpler version which should suffice
	int boxGapWidth = m_nMinWidth; // start with this minimum value (text-based)
	boxGapWidth = CalcPileWidth(); // calc text extents of src, tgt, gloss - sets to max
	// but the tgt calculation is based on pSrcPhrase->m_targetStr, which may at this
	// point not have content. So the value obtained so far may be correct (if source text
	// is longest), or wrong, if the eventual adaptation turns out to be longer. So here
	// we must look at the wxTextCtrl in the dropdown phrasebox, get it's extent, and if
	// the width is wider than the boxGapWidth value above, set boxGapWidth to that 
	// value instead. (We handle the dropdown button further below.)

	// Only do this calculation provided the m_pSrcPhrase pointer is set and that
	// CSourcePhrase instance is the one at the active location, if not so, return
	// PHRASE_BOX_WIDTH_UNSET which has the value -1
	if (m_pSrcPhrase != NULL)
	{
		if (m_pSrcPhrase->m_nSequNumber == m_pLayout->m_pApp->m_nActiveSequNum)
		{
			wxSize boxExtent;
			wxString boxText = wxEmptyString; // initialize
			boxText = m_pLayout->m_pApp->m_pTargetBox->GetTextCtrl()->GetValue();
			// Get text extent based on current contents of the phrase box;
			// remember, glossing mode might be current
			if (gbIsGlossing && gbGlossingUsesNavFont)
			{
				aDC.SetFont(*m_pLayout->m_pNavTextFont); // it's using the navText font
				aDC.GetTextExtent(boxText, &boxExtent.x, &boxExtent.y);
			}
			else // if not glossing, or not using nav text when glossing, it's using the target font
			{
				aDC.SetFont(*m_pLayout->m_pTgtFont);
				aDC.GetTextExtent(boxText, &boxExtent.x, &boxExtent.y);
			}
			int boxTextWidth = boxExtent.x;
			// Test for the actual phrasebox control's text being longest, if so, use that value
			if (boxGapWidth < boxTextWidth)
			{
				// The phrasebox's text is longest, so use that to ensure the gap is large enough
				boxGapWidth = boxTextWidth;
			}
			// At this point we have the width of the widest text worked out. The phrasebox will
			// have (1) user-set slop at the end for typing in, (2) a button which follows. So we have
			// to augment the gap width we are calculating by adding those values in.
			if (boxGapWidth < 40)
				boxGapWidth = 40; // in case m_targetPhrase was empty or very small
			wxChar aChar = _T('w'); // our box slop (whitespace where more chars can be typed
									// without a resize of the box being needed) is calculated
									// by multiplying the width of a w character by m_nExpandBox
									// which is a user-settable value in Preferences...
			wxString wStr = aChar;
			wxSize charSize;
			aDC.GetTextExtent(wStr, &charSize.x, &charSize.y);
			int slop = gpApp->m_nExpandBox*charSize.x;
			boxGapWidth += slop; // add the slop amount to the gap

			// Next we need to add to the above calculation of boxGapWidth the width of the new
			// button plus the 1-pixel space between its and the legacy phrasebox. This addition 
			// will be a constant value regardless of whether the phrasebox is expanding or not. 
			// Currently (13Jul2018) the button width is about 20 pixels (but its size may change 
			// if/when we redo the current xpm button with a better quality one).
			// We note that the new adjusted value for boxGapWidth is assigned to the Layout's 
			// m_curBoxWidth member and also returned by CalcPhraseBoxGapWidth() to the caller.
			//int gapWidthWithoutButton = boxGapWidth; // use in the test below
			wxSize buttonSize = gpApp->m_pTargetBox->GetPhraseBoxButton()->GetSize();
			int adjustedButtonWidth = buttonSize.GetX() + 1; // allow 1 pixels space before the button
			if (buttonSize.x > 0)
			{
				boxGapWidth += adjustedButtonWidth;
			}
			// Correct to here, as far as calculating the gap. ResizeBox() will get the phrasebox size right.
			// If expanding because the user's typing has increased the adaptation text length to the
			// degree that the box must be resized larger, the layout may have a wider textctrl with 
			// its slop & button - so we have to expand the gap to acccomodate the resize that will be done
			if (widthMode == expanding)
			{
				if (m_pLayout->m_curBoxWidth > boxGapWidth)
				{
					boxGapWidth = m_pLayout->m_curBoxWidth + slop; // increasing the gap by the slop value
						// - that should be enough of an increase to the gap, m_curBoxWidth already is a 
					    // sum of text extent + button width + the phrase box slop (as calculated by multiples
					    // of 'f' widths, less 1) - the box's slop is what got consumed by the user's edits,
					    // so adding the same amount of slop as calculated by the larger 'w' character widths,
					    // should keep the gap bigger than the phrasebox to be lodged there
				}
			}
			// BEW 18Jul18, that should do it, for this function's refactoring
		} // end of TRUE block for test: if (m_pSrcPhrase->m_nSequNumber == m_pLayout->m_pApp->m_nActiveSequNum)
	}
	else
	{
		// CSourcePhrase pointer was null, so cannot calculate a value
		return PHRASE_BOX_WIDTH_UNSET;
	} // end of else block for test: if (m_pSrcPhrase != NULL)

	/* legacy code plus logging and a tweak or two that did not solve the problem
	// BEW 17Jul18, m_pLayout->m_curBoxWidth is unset (large -ve rubbish value) so
	// set the same minimum here for it too
	m_pLayout->m_curBoxWidth = m_nMinWidth;
#if defined(_DEBUG) && defined(_NEWDRAW)
	wxLogDebug(_T("CPile::CalcPhraseBoxGapWidth() starting: m_nMinWidth (of pile) = %d, for box text: %s"),
		m_nMinWidth, m_pLayout->m_pApp->m_targetPhrase.c_str());
#endif

    // only do this calculation provided the m_pSrcPhrase pointer is set and that
    // CSourcePhrase instance is the one at the active location, if not so, return
    // PHRASE_BOX_WIDTH_UNSET which has the value -1
	if (m_pSrcPhrase != NULL)
	{
		if (m_pSrcPhrase->m_nSequNumber == m_pLayout->m_pApp->m_nActiveSequNum)
		{
			wxSize boxExtent;
			if (gbIsGlossing && gbGlossingUsesNavFont)
			{
				aDC.SetFont(*m_pLayout->m_pNavTextFont); // it's using the navText font
				aDC.GetTextExtent(m_pLayout->m_pApp->m_targetPhrase, &boxExtent.x, &boxExtent.y);
			}
			else // if not glossing, or not using nav text when glossing, it's using the target font
			{
				aDC.SetFont(*m_pLayout->m_pTgtFont);
				aDC.GetTextExtent(m_pLayout->m_pApp->m_targetPhrase, &boxExtent.x, &boxExtent.y);
			}
#if defined(_DEBUG) && defined(_NEWDRAW)
			wxLogDebug(_T("CPile::CalcPhraseBoxGapWidth(): boxExtent.x (measured, with no slop added) = %d, for box text: %s"),
				boxExtent.x, m_pLayout->m_pApp->m_targetPhrase.c_str());
#endif
			if (boxExtent.x < 40)
				boxExtent.x = 40; // in case m_targetPhrase was empty or very small
			wxChar aChar = _T('w'); // formerly wxString aChar = _T('w');, but this somehow 
								    // crashed (on 2Oct13, inexplicably) so I made it a wxChar
								    // and created a new wxString, wStr, and set it to aChar
								    // -- let's' see if this still gives a crash - it doesn't
			wxString wStr = aChar;
			wxSize charSize;
			//aDC.GetTextExtent(aChar, &charSize.x, &charSize.y);
			aDC.GetTextExtent(wStr, &charSize.x, &charSize.y);
			boxExtent.x += gpApp->m_nExpandBox*charSize.x; // add a slop factor (m_nExpandBox is user settable)

#if defined(_DEBUG) && defined(_NEWDRAW)
			wxLogDebug(_T("CPile::CalcPhraseBoxGapWidth(): m_nExpandBox multiplier for w's slop: %d , boxExtent.x (measured) = %d, for box text: %s"),
				gpApp->m_nExpandBox, boxExtent.x, m_pLayout->m_pApp->m_targetPhrase.c_str()); // <- value user-defined & set from Preferences
#endif
#if defined(_DEBUG) && defined(_NEWDRAW)
			wxLogDebug(_T("CPile::CalcPhraseBoxGapWidth(): Box Width + slop: %d , for box text: %s, boxGapWidth currently = %d"),
				boxExtent.x, m_pLayout->m_pApp->m_targetPhrase.c_str(), boxGapWidth);
#endif
			// BEW added 17Jul18, initialize a boxGapWidth big enough for the text length plus the slop
			boxGapWidth = boxExtent.x; // includes slop as defined by user in Preferences...
			// legacy code
			//if (boxGapWidth < boxExtent.x)
			//{
			//	boxGapWidth = boxExtent.x;
			//}
			//
#if defined(_DEBUG) && defined(_NEWDRAW)
			wxLogDebug(_T("CPile::CalcPhraseBoxGapWidth(): initialized boxGapWidth (includes slop): %d , for box text: %s"),
				boxGapWidth, m_pLayout->m_pApp->m_targetPhrase.c_str());
#endif
			// BEW 17Jul18 At this point, in the legacy app the gap and the box width are the same value,
			// with the box fitting neatly into the gap made for it. In the drop down phrasebox solution,
			// the button to the right of the phrasebox takes extra space, so that means the gap needs to
			// be wider. So we have to separate the box width, from the gap width - the latter being larger.
			// The code adjustments below should do this; I've made a new public member, m_nBoxOnlyWidth
			// to be set right here, to preserve the text control width; and the gap calculations can
			// go on as Bill has them below. m_BoxOnlyWidth should then be used for setting Layout's
			// m_curBoxWidth; and the calculated gap width should be returned as boxGapWidth as before,
			// on exit
			m_nBoxOnlyWidth = boxGapWidth;
			m_pLayout->m_curBoxWidth = m_nBoxOnlyWidth; // has to be set, whether expanding, contracting, or stead as she goes
#if defined(_DEBUG) && defined(_NEWDRAW)
			wxLogDebug(_T("CPile::CalcPhraseBoxGapWidth(): reset (from m_nBoxOnlyWidth) m_pLayout->m_curBoxWidth = %d, for box text: %s"),
				m_pLayout->m_curBoxWidth, m_pLayout->m_pApp->m_targetPhrase.c_str());
#endif
			//
			//if (widthMode == expanding)
			//{
			//	if (m_nBoxOnlyWidth > m_pLayout->m_curBoxWidth)
			//		m_pLayout->m_curBoxWidth = m_nBoxOnlyWidth;
			//}
			//
            // whm 13Jul2018 TODO: Bruce should verify that I have interpreted correctly what 
            // CalcPhraseBoxGapWidth()'s purpose is, and that my modification done in the coding 
            // below are appropriate to accommodate the new phrasebox with its dropdown button.
            // Problem/Issue to sort out: The value returned from CalcPhraseBoxGapWidth() function
            // seems to be used to adjust the actual size of the phrasebox (edit box part), rather
            // than merely determining the gap width that the phrasebox will be shown in. There 
            // needs to be a way for the new phrasebox to fit within the gap - including its new
            // dropdown button.
            // 
            // See and compare the code adjustment made here with the code change made in the 
            // 'hack' in the View's OnDraw() method. Also see the code and logic within the View's
            // ResizeBox() method, where relative positioning of phrasebox's button and dropdown list
            // are maintained.
            // Note that the new phrasebox now has a button that is always aligned to the right end 
            // of the wxTextCtrl that makes up the new phrasebox. There is a gap of 1 pixel
            // between the legacy phrasebox's wxTextCtrl and the bitmap dropdown button.
            // I am here assuming the CalcPhraseBoxGapWidth()'s purpose is to calculate the size of 
            // the "empty gap" that is needed for the phrasebox (and its new button component) to 
            // be shown() at the pile that is to become the active location.
            // We need to account for the new phrasebox's total width here (and in OnDraw) in order
            // to prevent the phrasebox's button from encroaching on any following target pile/cell
            // at the time the View's OnDraw() method is called, and strips and piles get drawn in
            // such a way to leave sufficient space gap for the new phrasebox and its button to be
            // shown within that gap.
            // For our CalcPhraseBoxGapWidth() the size of gap/width to accommodate the new 
            // phrasebox needs to be calculated to include the size of the legacy phrasebox (using 
            // the m_nMinWidth and/or text extent calcs already done above), and now adjusted for 
            // the size of the new button, plus the 1-pixel gap between the legacy box and the new 
            // button.
            // Note: There is a hack in the View's OnDraw() function where the same calculation needs
            // to be done for the calculation of a local boxSize variable there to keep the 
            // calculations in sync - at least for the purposes of the hack to intervene if ever
            // necessary.
            //
            // We need to add to the above calculation of boxGapWidth the width of the new button
            // plus the 1-pixel space between its and the legacy phrasebox. This addition will be a 
            // constant value regardless of whether the phrasebox is expanding or not. 
            // Currently (13Jul2018) the button width is about 20 pixels (but its size may change 
            // if/when we redo the current xpm button with a better quality one).
            // We note that the new adjusted value for boxGapWidth is assigned to the Layout's 
            // m_curBoxWidth member and also returned by CalcPhraseBoxGapWidth() to the caller.
            wxSize buttonSize = gpApp->m_pTargetBox->GetPhraseBoxButton()->GetSize();
            int adjustedButtonWidth = buttonSize.GetX() + 1; // allow 1 pixels space before the button
            if (buttonSize.x > 0)
                boxGapWidth += adjustedButtonWidth;
			// legacy test
			//if (widthMode == expanding)
			//{
			//	if (m_pLayout->m_curBoxWidth > boxGapWidth)
			//		boxGapWidth = m_pLayout->m_curBoxWidth;
			//}
			//
			// BEW 17Jul18 change the test to account for the dropdown button && its 1-pixel separation
			if (widthMode == expanding)
			{
				if (boxGapWidth < m_pLayout->m_curBoxWidth + adjustedButtonWidth)
				{
					// There would be encroachment, so widen the gap 
					// (could also add m_nCurGapWidth here, but probably unnecessary - I hope)
					boxGapWidth = m_pLayout->m_curBoxWidth + adjustedButtonWidth;
				}
			}
		}
	}
	else
	{
		// CSourcePhrase pointer was null, so cannot calculate a value
		return PHRASE_BOX_WIDTH_UNSET;
	}

	// before returning, put the final value back into CLayout::m_curBoxWidth
    // whm 13Jul2018 BEW TODO: Yes, you've interpretted the gap idea correctly - I've 
	// refactored the hack, as of 17Jul18
	// Bill's earlier Note: text continues below
    // Note: The following assignment of boxGapWidth to the Layout's m_curBoxWidth
    // makes the m_curBoxWidth be the same as the boxGapWidth, but should not be
    // so with the new phrasebox and dropdown button. It increases the effective
    // size of the edit box part of the new phrasebox by the width amount of the 
    // newly added dropdown button (plus 1-pixel spacing). We need a way to keep
    // the calculated boxGapWidth independent of the m_curBoxWidth - in order for
    // the button part of the new phrasebox to fit within the gap and not encroach
    // on any following pile. See related comments also in the View's ResizeBox()  
    // and OnDraw() functions. 
	//m_pLayout->m_curBoxWidth = boxGapWidth; // BEW 17Jul18 temporarily commented 
	// out - check it allows m_curBoxWidth to be smaller than boxGapWidth
#if defined(_DEBUG) && defined(_NEWDRAW)
	wxLogDebug(_T("CPile::CalcPhraseBoxGapWidth() at end: m_pLayout->m_curBoxWidth: %d , for box text: %s, and new boxGapWidth: %d  CHECK for valid values, and gap larger"),
		m_pLayout->m_curBoxWidth, m_pLayout->m_pApp->m_targetPhrase.c_str(), boxGapWidth);
#endif
	*/

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
	if (this == NULL)
		return;
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
			|| bHasFilterMarker)
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
			}

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
			// last CSourcPhrase of a footnote, will not show any nav text unless we
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
						rectBounding.Offset(-xOffset,-diff); // navText is RTL
					else
						rectBounding.Offset(0,-diff); // navText is LTR
				}
				else
				{
					// left to right layout
					if (m_pLayout->m_pApp->m_bNavTextRTL)
						rectBounding.Offset(0,-diff); // navText is RTL
					else
						rectBounding.Offset(xOffset,-diff); // navText is LTR
				}
#else
				rectBounding.Offset(xOffset, -diff);
#endif
			}
			else
			{
				// no filter marker, so we don't need to make extra space
				rectBounding.Offset(0,-diff);
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
				m_pCell[0]->DrawTextRTL(pDC,str,rectBounding); // any CCell pointer would do
			}
			else
			{
				// *** Draw LTR Actual Ch:Vs and/or m_inform Navigation Text ***
				pDC->DrawText(str,rectBounding.GetLeft(),rectBounding.GetTop());
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
			pDC->SetBrush(wxBrush(wxColour(254,218,100),wxBRUSHSTYLE_SOLID));
#else
			pDC->SetBrush(wxBrush(wxColour(254,218,100),wxSOLID));
#endif
			pDC->SetPen(*wxBLACK_PEN); // black - whm added 20Nov06
			wxRect insides(ptNote.x,ptNote.y,ptNote.x + 9,ptNote.y + 9);
			// MFC CDC::Rectangle() draws a rectangle using the current pen and fills the
			// interior using the current brush
			pDC->DrawRectangle(ptNote.x,ptNote.y,9,9); // rectangles are drawn with a
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
