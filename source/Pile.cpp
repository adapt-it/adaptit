/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Pile.cpp
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \date_revised	15 January 2008
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
// Pending Implementation Items (in order of importance): (search for "TODO")
// 1. Conditional compile HWND below for other platforms in the Draw method.
//
// Unanswered questions: (search for "???")
// 1. 
// 
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
//#include "SourceBundle.h"
//#include "Adapt_ItDoc.h"
#include "SourcePhrase.h"
#include "AdaptitConstants.h"
// don't mess with the order of the following includes, Strip must precede View must
// precede Pile must precede Layout and Cell can usefully by last
#include "Strip.h"
#include "Adapt_ItView.h"
#include "Pile.h"
#include "Layout.h"
#include "Cell.h"
#include "MainFrm.h"

// Define type safe pointer lists
#include "wx/listimpl.cpp"

/// This macro together with the macro list declaration in the .h file
/// complete the definition of a new safe pointer list class called PileList.
WX_DEFINE_LIST(PileList);

// next two are for version 2.0 which includes the option of a 3rd line for glossing

// these next globals are put here when I moved CreatePile() 
// from the view to the CPile class
extern bool gbShowTargetOnly;
extern bool gbEnableGlossing;
//extern CAdapt_ItApp* gpApp;
extern const wxChar* filterMkr;

// globals relevant to the phrase box  (usually defined in Adapt_ItView.cpp)
extern short gnExpandBox; // set to 8


/// This global is defined in PhraseBox.cpp.
extern bool gbExpanding;

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

extern bool gbIsPrinting; // whm added because wxDC does not have ::IsPrinting() method

extern bool gbRTL_Layout;

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
	m_nMinWidth = 40;
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
	m_nWidth = 20;
	m_nMinWidth = 40;
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

#ifdef __WXDEBUG__
	int nDebugIndex = 4;
	wxLogDebug(_T("DebugIndex = %d  CPile, copy creator, CPile pointer  %x"),
				nDebugIndex,this);
#endif


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

bool CPile::HasFilterMarker()
{
	return m_pSrcPhrase->m_markers.Find(filterMkr) >= 0;
}

void CPile::SetIsCurrentFreeTransSection(bool bIsCurrentFreeTransSection)
{
	m_bIsCurrentFreeTransSection = bIsCurrentFreeTransSection;
}

bool CPile::GetIsCurrentFreeTransSection()
{
	return m_bIsCurrentFreeTransSection;
}


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

void CPile::SetPhraseBoxGapWidth()
{
	m_nWidth = CalcPhraseBoxGapWidth();
}

int	CPile::GetMinWidth()
{
	return m_nMinWidth;
}

int	CPile::GetPhraseBoxGapWidth()
{
	return m_nWidth;
}


// Calculates the pile's width before laying out the current pile in a strip. The function
// is not interested in the relative ordering of the glossing and adapting cells, and so
// does not access CCell instances; rather, it just examines extent of the three text
// members m_srcPhrase, m_targetStr, m_gloss on the CSourcePhrase instance pointed at by
// this particular CPile instance. The width is the maximum extent.x for the three strings
// checked.
int CPile::CalcPileWidth()
{
	int pileWidth = 40; // ensure we never get a pileWidth of zero

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
	return pileWidth;
}

int CPile::CalcPhraseBoxGapWidth()
{
    // is this pile the active one? If so, get a pile width using m_pApp->m_targetPhrase
    // (the phrase box contents) for the pile extent (plus some slop), because at the
    // active location the m_adaption & m_targetStr members of pSrcPhrase are not set, and
    // won't be until the user hits Enter key to move phrase box on or clickes to place it
    // elsewhere, so only pApp->m_targetPhrase is valid; note, for version 2 which supports
    // a glossing line, the box will contain a gloss rather than an adaptation whenever
    // gbIsGlossing is TRUE. Glossing could be using the target font, or the navText font.
	wxClientDC aDC((wxScrolledWindow*)m_pLayout->m_pCanvas); // make a temporary device context
	wxSize extent;
	int boxGapWidth = m_nMinWidth; // start with this minimum value (text-based)

    // only do this calculation provided the m_pSrcPhrase pointer is set and the partner
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
			if (boxExtent.x < 40)
				boxExtent.x = 40; // in case m_targetPhrase was empty or very small 
			wxString aChar = _T('w');
			wxSize charSize;
			aDC.GetTextExtent(aChar, &charSize.x, &charSize.y); 
			boxExtent.x += gnExpandBox*charSize.x; // add a slop factor (gnExpandBox is user settable)
			if (boxGapWidth < boxExtent.x)
			{
				boxGapWidth = boxExtent.x;
			}
			
            // again adjust the value if the box has just expanded, FixBox() has calculated
            // a new phrase box width already and stored that width in
            // CLayout::m_curBoxWidth, so we must now check, if the box is expanding, to
            // see if that stored width is greater than the one so far calculated, and if
            // so, use the greater value - and if expanding, FixBox() sets gbExpanding to
            // TRUE. If not expanding (the box could be contracting) we just use the
            // unadjusted value. FixBox() also clears the gbExpanding flag before exitting,
			// so between setting that flag TRUE and clearing it to FALSE, there has to be
			// a determination of the new box width, storage of it to
			// CLayout::m_curBoxWidth, and a recalculation of the strips to reflect the
			// correct pile population of them after the box width changed - the latter is
			// done by RecalcLayout (and also by LayoutStrip() in the legacy code), but in
			// the new layout code where we replace many calls to RecalcLayout() by
			// AdjustForUserEdits(), then the latter would have to be called instead.
			// CalcPhraseBoxGapWidth() therefore does not clear the gbExpanding flag, it
			// just uses it for the following test
			if (gbExpanding)
			{
				if (m_pLayout->m_curBoxWidth > boxGapWidth)
					boxGapWidth = m_pLayout->m_curBoxWidth;
			}
		}
	}
	else
	{
		// CSourcePhrase pointer was null, so cannot calculate a value
		return PHRASE_BOX_WIDTH_UNSET;
	}

	// before returning, put the final value back into CLayout::m_curBoxWidth
	m_pLayout->m_curBoxWidth = boxGapWidth;

	return boxGapWidth;
}

CCell** CPile::GetCellArray()
{
	return &m_pCell[0]; // return pointer to the array of CCell pointers
}

void CPile::SetPhraseBoxGapWidth(int nNewWidth)
{
	m_nWidth = nNewWidth; // a useful overload, for when the phrase box is contracting
}

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
		// of the pile depending on the gbRTL_Layout flag (FALSE or TRUE, respectively), rather
		// than using the nav text's directionality
		if (m_pSrcPhrase->m_bFirstOfType || m_pSrcPhrase->m_bVerse
			|| m_pSrcPhrase->m_bChapter || m_pSrcPhrase->m_bParagraph
			|| m_pSrcPhrase->m_bFootnoteEnd || m_pSrcPhrase->m_bHasInternalMarkers
			|| m_pSrcPhrase->m_markers.Find(filterMkr) != -1)
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
            // info in m_markers is also handled here, but independently of m_inform
			wxString str = _T("");

			if (bHasFilterMarker && !gbShowTargetOnly)
			{
                // if \~FILTERED is anywhere in the m_markers member of the sourcephrase
                // stored which has its pointer stored in m_pPile, then we require a wedge
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
				bool bFreeHasNoContent = 
					m_pLayout->m_pView->IsFreeTranslationContentEmpty(m_pSrcPhrase);
				bool bBackHasNoContent = 
					m_pLayout->m_pView->IsBackTranslationContentEmpty(m_pSrcPhrase);

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

				if (!bFreeHasNoContent && !bBackHasNoContent)
				{
					// make whole inner part of wedge green
					pDC->SetPen(*wxGREEN_PEN);	
				}
				else
				{
					if (bFreeHasNoContent && bBackHasNoContent)
					{
						// use a third colour - red grabs attention best
						pDC->SetPen(*wxRED_PEN);
					}
					else
					{
						// both are not together contentless, so only one of them is
						// so do the one which it is in the appropriate colour
						if (bFreeHasNoContent)
						{
							// colour it khaki
							pDC->SetPen(wxPen(wxColour(160,160,0), 1, wxSOLID));
						}
						if (bBackHasNoContent)
						{
							// colour it pastel blue
							pDC->SetPen(wxPen(wxColour(145,145,255), 1, wxSOLID));
						}
					}
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
			if (m_pSrcPhrase->m_bFootnoteEnd)
			{
				if (str.IsEmpty())
					str = _("end fn");
				else
				{
					str += _T(' ');
					str += _("end fn");
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
			pDC->SetBrush(wxBrush(wxColour(254,218,100),wxSOLID));
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

void CPile::PrintPhraseBox(wxDC* pDC)
{
	wxTextCtrl* pBox = m_pLayout->m_pApp->m_pTargetBox;
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

void CPile::Draw(wxDC* pDC)
{
	// draw the cells this CPile instance owns, MAX_CELLS = 3
	if (!gbShowTargetOnly)
	{
		m_pCell[0]->Draw(pDC);
	}
	m_pCell[1]->Draw(pDC); // always draw the line which has the phrase box
	if (gbEnableGlossing)
	{
		// when TRUE, that means the menu item "See Glosses" has been ticked; but we may
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
	if (gbIsPrinting)
	{
		PrintPhraseBox(pDC); // internally checks if this is active location
	}
}
