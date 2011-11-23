/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Cell.cpp
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public
///  License (see license directory)
/// \description	This is the implementation file for the CCell class.
/// The CCell class represents the next smaller division of a CPile, there
/// potentially being up to five CCells displaying vertically top to bottom
/// within a CPile. The CCell has the smarts for drawing the text and changing
/// background colour (eg. for selections) in its DrawCell() public function,
/// and for handling free translation colouring, green wedges, navigation text
/// etc. in its Draw() function. (CText removed 6Feb09)
/// \derivation		The CCell class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items (in order of importance): (search for "TODO")
// 1. NONE. Current with MFC version 3.10.0
//
// Unanswered questions: (search for "???")
// 1.
//
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Cell.h"
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

//#include "helpers.h" // whm added 28Mar04
#include "Adapt_It.h"
#include "AdaptitConstants.h"
#include "SourcePhrase.h"
//#include "SourceBundle.h"
//#include "Adapt_ItDoc.h"
// don't mess with the order of the following includes, Strip must precede View must
// precede Pile must precede Layout and Cell can usefully by last
#include "Strip.h"
#include "Adapt_ItView.h"
#include "Pile.h"
#include "Layout.h"
#include "Cell.h"

// globals for support of vertical editing

/// A gray color used to mark the non-editable surrounding context when vertical
/// editing of source text is in progress
wxColor gMidGray = wxColour(128,128,128); //COLORREF gMidGray = (COLORREF)RGB(128,128,128);

/// This global is defined in Adapt_It.cpp.
extern EditRecord gEditRecord;

/// This global is defined in Adapt_ItView.cpp.
extern bool gbVerticalEditInProgress;

/// This global is defined in Adapt_ItView.cpp.
extern EditStep gEditStep;

/// A local pointer to the global gEditRecord defined in Adapt_It.cpp
static EditRecord* pRec = &gEditRecord;

/// This global is defined in Adapt_It.cpp.
extern CPile* gpGreenWedgePile;

/// This global is defined in Adapt_It.cpp.
extern CPile* gpNotePile;

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

extern bool gbGlossingUsesNavFont;

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbGlossingVisible; // TRUE makes Adapt It revert to Shoebox functionality only

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

// whm NOTE: wxDC::DrawText(const wxString& text, wxCoord x, wxCoord y) does not have an
// equivalent to the nFormat parameter, but wxDC has a SetLayoutDirection(wxLayoutDirection
// dir) method to change the logical direction of the display context. In wxDC the display
// context is mirrored right-to-left when wxLayout_RightToLeft is passed as the parameter;
// While the MFC version changes the alignment and RTL reading direction of DrawText(), it
// is not the same as mirroring (in which MFC would actually call
// CDC::SetLayout(LAYOUT_RTL) to effect RTL mirroring in the display context. In wx,
// wxDC::DrawText() does not have a parameter that can be used to control Right alignment
// and/or RTL Reading of text at that level of the DC. Certain controls such as wxTextCtrl
// and wxListBox, etc., also have an undocumented method called
// SetLayoutDirection(wxLayoutDirection dir), where dir is wxLayout_LeftToRight or
// wxLayout_RightToLeft. Setting the layout to wxLayout_RightToLeft on these controls also
// involves some mirroring, so that any scrollbar that gets displayed, for example,
// displays on the left rather than on the right, etc. In the wx version we have to be
// careful about the automatic mirroring features involved in the SetLayoutDirection()
// function, since Adapt It MFC was designed to micromanage the layout direction itself in
// the coding of text, cells, piles, strips, etc.

/// This global is defined in Adapt_It.cpp.
//extern CAdapt_ItApp* gpApp; // want to access it fast

extern const wxChar* filterMkr;
extern const wxChar* filteredTextPlaceHolder;
extern EditRecord gEditRecord;

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


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(CCell, wxObject)


CCell::CCell()
{
	m_bSelected = FALSE; // changes according to user's or app's
		// selecting/deselecting activity
		// m_pPhrase = NULL; // it is pointed at a wxString only by
		// the Draw function in refactored design, m_pPhrase is not
		// needed, use GetCellText() instead
	m_pLayout = NULL; // CreateCell() will set this to m_pLayout
	m_pOwningPile = NULL; // CreateCell() will set this
	m_nCell = 0; // CreateCell() will set this to its permanent value
}

// the copy constructor does not need to be coded explicitly because it owns nothing, but
// no harm in having it
CCell::CCell(const CCell& cell)
{
	m_bSelected = cell.m_bSelected;
	m_nCell = cell.m_nCell;
	m_pPhrase = cell.m_pPhrase;
	m_pLayout = cell.m_pLayout;
	m_pOwningPile = cell.m_pOwningPile;
}

CCell::~CCell()
{

}

wxFont* CCell::GetFont()
{
	switch (m_nCell)
	{
	case 0:
		return m_pLayout->m_pSrcFont;
	case 1:
		if (gbIsGlossing && gbGlossingUsesNavFont)
			return m_pLayout->m_pNavTextFont;
		else
			return m_pLayout->m_pTgtFont;
	case 2:
		if (gbIsGlossing)
			return m_pLayout->m_pTgtFont;
		else
		{
			if (gbGlossingUsesNavFont)
				return m_pLayout->m_pNavTextFont;
			else
				return m_pLayout->m_pTgtFont;
		}
	}
	return m_pLayout->m_pTgtFont; // never accessed, it's here just to
								  // avoid a compiler warning
}

CPile* CCell::GetPile()
{
	return m_pOwningPile;
}

bool CCell::IsSelected()
{
	return m_bSelected;
}

void CCell::SetSelected(bool bValue)
{
	// set to TRUE or FALSE, explicitly (no default)
	m_bSelected = bValue;
}

int CCell::GetCellIndex()
{
	return m_nCell; // return this cell's index in the pile (values
					// 0 to 3 inclusive)
}

// BEW added 13May11 support of m_tgtDiffsTextColor, to implement a feature requested by
// Patrick Rosendall on 27Aug2009, to use a different colour for a target text word/phase
// which differs in spelling from the source text word/phrase
wxColour CCell::GetColor()
{
	CSourcePhrase* pSrcPhrase = m_pOwningPile->m_pSrcPhrase;
	switch (m_nCell)
	{
	case 0:
		{
			if (pSrcPhrase->m_bSpecialText)
			{
				return m_pLayout->GetSpecialTextColor();
			}
			else if (pSrcPhrase->m_bRetranslation)
			{
				return m_pLayout->GetRetranslationTextColor();
			}
			else
			{
				return m_pLayout->m_srcColor;
			}
		}
	case 1:
		if (gbIsGlossing)
			return m_pLayout->m_navTextColor;
		else
		{
			if (pSrcPhrase->m_key != pSrcPhrase->m_adaption)
			{
				return m_pLayout->GetTgtDiffsTextColor();
			}
			else
			{
				return m_pLayout->m_tgtColor;
			}
		}
	case 2:
		if (gbIsGlossing)
		{
			if (pSrcPhrase->m_key != pSrcPhrase->m_adaption)
			{
				return m_pLayout->GetTgtDiffsTextColor();
			}
			else
			{
				return m_pLayout->m_tgtColor;
			}
		}
		else
			return m_pLayout->m_navTextColor;
	}
	return m_pLayout->m_tgtColor; // never accessed, it's here
							// just to avoid a compiler warning
}

int CCell::Width()
{
	return m_pOwningPile->Width();
}

int CCell::Height()
{
	int height = -1;
	switch (m_nCell)
	{
	case 0:
		if (gbShowTargetOnly)
		{
			height = 0; // no source text line showing
		}
		else
		{
			height = m_pLayout->GetSrcTextHeight();
		}
		return height;
	case 1:
		if (gbIsGlossing)
		{
			if (gbGlossingUsesNavFont)
			{
				height = m_pLayout->GetNavTextHeight();
			}
			else
			{
				height = m_pLayout->GetTgtTextHeight();
			}
		}
		else
		{
			height = m_pLayout->GetTgtTextHeight();
		}
		return height;
	case 2:
		if (gbIsGlossing)
		{
			height = m_pLayout->GetTgtTextHeight();
		}
		else
		{
			if (gbGlossingUsesNavFont)
			{
				height = m_pLayout->GetNavTextHeight();
			}
			else
			{
				height = m_pLayout->GetTgtTextHeight();
			}
		}
		return height;
	}
	return height; // to avoid a compiler warning
}

int CCell::Left()
{
	return m_pOwningPile->Left();
}

int CCell::Top()
{
	int top = -1;
	switch (m_nCell)
	{
	case 0:
		if (gbShowTargetOnly)
		{
			// the source text line is suppressed, but no harm if we return
			// the pile's top
			top = m_pOwningPile->Top();
		}
		else
		{
			// source text line is not suppressed, it's top is same as
			// pile's top
			top = m_pOwningPile->Top();
		}
		return top;
	case 1:
		if (gbShowTargetOnly)
		{
			// the target, or gloss text, line is at the top of the pile when
			// source line is suppressed
			top = m_pOwningPile->Top();
		}
		else
		{
            // source text line is not suppressed, so the target line's top will be the
            // source text height lower down in the pile (whether glossing or adapting)
			top = m_pOwningPile->Top() + m_pLayout->GetSrcTextHeight();
		}
		return top;
	case 2:
		if (gbShowTargetOnly)
		{
			// this line is never shown when source line is suppressed, so a nonsense
			// value will do
			top = -1;
		}
		else
		{
            // source text line is not suppressed, so the third line's top, if the line is
            // visible, will be the second line's top, plus the height of the text in the
            // second line -- where the latter will depend on whether it is showing target
            // text font,
			top = m_pOwningPile->Top() + m_pLayout->GetSrcTextHeight();
			if (gbIsGlossing && gbGlossingUsesNavFont)
			{
				top += m_pLayout->GetNavTextHeight();
			}
			else
			{
				top += m_pLayout->GetTgtTextHeight();
			}
		}
		return top;
	}
	return top; // to avoid a compiler warning
}

wxPoint CCell::GetTopLeft()
{
	wxPoint topleft;
	TopLeft(topleft);
	return topleft;
}

void CCell::GetCellRect(wxRect& rect)
{
	rect.SetTop(Top());
	rect.SetLeft(Left());
	rect.SetWidth(Width());
	rect.SetHeight(Height());
}

void CCell::TopLeft(wxPoint& ptTopLeft)
{
	ptTopLeft.y = Top();
	ptTopLeft.x = Left();
}

void CCell::BottomRight(wxPoint& ptBottomRight)
{
	ptBottomRight.y = Top() + Height();
	ptBottomRight.x = Left() + Width();
}

void CCell::CreateCell(CLayout* pLayout, CPile* pOwnerPile, int index)
{
	m_nCell = index; // remains this value for the life of the owning pile
	m_pOwningPile = pOwnerPile; // lets the CCell access the CSourcePhrase the
								// owning pile points at
	m_pLayout = pLayout; // where drawing information can be had - eg. font
						 // pointer, font colour, etc

	// Note: m_pPhrase is set by a SetCellText() call in the Draw functions because
	// depending on the mode (either adapting or glossing) what gets drawn in *m_pCell[1]
	// will be either an m_adaption wxString, or an m_gloss wxString - so we point to the
	// right one as late as possible, which therefore means in the Draw() function itself,
	// immediately before drawing
	// the m_bSelected value must default to FALSE when the cell is first created
}


// BEW 2Aug08, additions to for gray colouring of context regions in vertical edit steps
void CCell::Draw(wxDC* pDC)
{
    // next call should not be needed now, the paint issue on wxMac had a different source,
    // but keep it for the present as it should be called before setting any clip region
    // later in the code
	//pDC->DestroyClippingRegion(); // BEW removed 23Jun09, it was destroying the clip rect
									// before it could be used for reducing flicker

	pDC->SetBrush(*wxTRANSPARENT_BRUSH); // SetBackgroundMode() requires a valid brush
        // on wxGTK ( a transparent brush results in nothing being visible - code below
        // always sets the mode to wxSOLID before doing any drawing that is to be visible,
        // including backgrounds)

	wxColour oldBkColor;
	wxColour color(GetColor());	// get the default colour to be used for drawing this
        // cell's text (it may be overridden below - eg. to gray, when vertical edit is
        // being done)

    // vertical edit: change text colour to gray if it is before or after the editable
    // span; we have to do it also in the m_pText CText member too, and beware because the
    // latter can be NULL for some CCell instances. whm: initialized the following two ints
    // to 0 to avoid "potentially uninitialized local variable" warnings, since they would
    // be uninitialized when gbVerticalEditInProgress if FALSE (albeit the present code,
    // they are not used anywhere but where gbVerticalEditInProgress tests true.
	int nStart_Span = 0;
	int nEnd_Span = 0;
	if (gbVerticalEditInProgress && (gEditStep == adaptationsStep) && pRec->bAdaptationStepEntered)
	{
		// set up the span bounding indices
		nStart_Span = pRec->nAdaptationStep_StartingSequNum;
		nEnd_Span = pRec->nAdaptationStep_EndingSequNum;
	}
	else if (gbVerticalEditInProgress && (gEditStep == glossesStep) && pRec->bGlossStepEntered)
	{
		// set up the span bounding indices
		nStart_Span = pRec->nGlossStep_StartingSequNum;
		nEnd_Span = pRec->nGlossStep_EndingSequNum;
	}
	else if (gbVerticalEditInProgress && (gEditStep == freeTranslationsStep) &&
				pRec->bFreeTranslationStepEntered)
	{
		// set up the span bounding indices
		nStart_Span = pRec->nFreeTranslationStep_StartingSequNum;
		nEnd_Span = pRec->nFreeTranslationStep_EndingSequNum;
	}
	// colour gray the appropriate cells' text
	if (gbVerticalEditInProgress
		&& (gEditStep == adaptationsStep || gEditStep == glossesStep ||
			gEditStep == freeTranslationsStep))
	{
        // the spans to be made available for work can differ in each step, so they are set
        // in the code above first
		if (m_pOwningPile->m_pSrcPhrase->m_nSequNumber < nStart_Span ||
			m_pOwningPile->m_pSrcPhrase->m_nSequNumber > nEnd_Span)
		{
			// it's either adaptations step, AND, the pile is before or after the
			// editable span
			color = gMidGray;
		}
		else
		{
			// if its not one of those, just use the normal text colour as set within the
			// CLayout instance
			;
		}
	}
    // In all the remainder of this Draw() function, only backgrounds are ever changed in
    // color, and the navigation whiteboard area's icons and text are drawn in the call to
    // DrawNavTextInfoAndIcons() at the end.
	if (m_bSelected)
	{
		oldBkColor = pDC->GetTextBackground(); // yellow
		pDC->SetBackgroundMode(m_pLayout->m_pApp->m_backgroundMode); // m_backgroundMode is
																	 // set to wxSOLID
		pDC->SetTextBackground(wxColour(255,255,0)); // yellow
	}
	else
	{
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
	}

    // BEW added 11Oct05 to have the top cell of the pile background coloured if the click
    // was on a green wedge or note icon
	if (!gbShowTargetOnly && m_nCell == 0 && !m_pLayout->m_pApp->m_bIsPrinting &&
		(m_pOwningPile == gpGreenWedgePile || m_pOwningPile == gpNotePile))
	{
		// hilight the top cell under the clicked green wedge or note, with light yellow
		pDC->SetBackgroundMode(m_pLayout->m_pApp->m_backgroundMode); // sets it to wxSOLID
		pDC->SetTextBackground(wxColour(255,255,100)); // light yellow
	}

	// Target Text Highlight
	// Check for automatically inserted target text/glosses and highlight them
	// BEW 12Jul05 add colouring for free translation sections, provided we are not
	// currently printing
    // BEW 10Oct05 added a block for colouring the source line (cell index == 0) whenever
    // there is no target (or gloss) text that can be coloured, so that there will always
    // be visual feedback about what is free translated, and what is about to be, and what
    // is not.
	if (m_nCell == 0 && m_pLayout->m_pApp->m_bFreeTranslationMode && !m_pLayout->m_pApp->m_bIsPrinting
		&& ((!gbIsGlossing && m_pOwningPile->m_pSrcPhrase->m_targetStr.IsEmpty()) ||
		(gbIsGlossing && m_pOwningPile->m_pSrcPhrase->m_gloss.IsEmpty())))
	{
        // we use pastel pink and green for the current section, and other defined
        // sections, respectively, and white (default) otherwise - ie. no free translation
        // there
		if (m_pOwningPile->m_pSrcPhrase->m_bHasFreeTrans &&
											!m_pOwningPile->m_bIsCurrentFreeTransSection)
		{
			// colour background pastel green
			pDC->SetBackgroundMode(m_pLayout->m_pApp->m_backgroundMode); // wxSOLID
			pDC->SetTextBackground(m_pLayout->m_pApp->m_freeTransDefaultBackgroundColor); // green
		}
		else if (m_pOwningPile->m_bIsCurrentFreeTransSection)
		{
			// colour background pastel pink
			pDC->SetBackgroundMode(m_pLayout->m_pApp->m_backgroundMode); // wxSOLID
			pDC->SetTextBackground(m_pLayout->m_pApp->m_freeTransCurrentSectionBackgroundColor); // pink
		}
	}
	if (m_nCell == 1)// active adapting or glossing line
	{
		if (m_pLayout->m_pApp->m_bFreeTranslationMode && !m_pLayout->m_pApp->m_bIsPrinting)
		{
            // we use pastel pink and green for the current section, and other defined
            // sections, respectively, and white (default) otherwise - ie. no free
            // translation there
			if (m_pOwningPile->m_pSrcPhrase->m_bHasFreeTrans &&
										!m_pOwningPile->m_bIsCurrentFreeTransSection)
			{
				// colour background pastel green
				pDC->SetBackgroundMode(m_pLayout->m_pApp->m_backgroundMode);
				pDC->SetTextBackground(m_pLayout->m_pApp->m_freeTransDefaultBackgroundColor); // green
			}
			else if (m_pOwningPile->m_bIsCurrentFreeTransSection)
			{
				// colour background pastel pink
				pDC->SetBackgroundMode(m_pLayout->m_pApp->m_backgroundMode);
				pDC->SetTextBackground(m_pLayout->m_pApp->m_freeTransCurrentSectionBackgroundColor); // pink
			}
		}
		else
		{
			// not free translation mode -- this is where the auto-highlighting (in light
			// purple) of automatically inserted adaptations or glosses is done (the bounding
			// sequence numbers on which it relies, however, get set elsewhere beforehand)
			if (	!(m_pLayout->m_pApp->m_bSuppressTargetHighlighting)
					&& m_pOwningPile->m_pSrcPhrase->m_nSequNumber >= gnBeginInsertionsSequNum
					&& m_pOwningPile->m_pSrcPhrase->m_nSequNumber <= gnEndInsertionsSequNum)
			{
				//Draw automatically inserted target text with selected background color
				// Some decent possibilities for default background highlight color are:
				//pDC->SetBkColor(RGB(255,255,150)); // light yellow (COLORREF)9895935
				//pDC->SetBkColor(RGB(255,200,255)); // light purple (COLORREF)16763135
				//pDC->SetBkColor(RGB(200,255,255)); // light blue = (COLORREF)16777160
				// The user can choose any reasonable color to that he/she finds usable
				// A light purple background highlight looks pretty good over the
				// default text colors and most that the user might choose
				pDC->SetBackgroundMode(m_pLayout->m_pApp->m_backgroundMode);
				pDC->SetTextBackground(m_pLayout->m_pApp->m_AutoInsertionsHighlightColor); // light purple
#ifdef Highlighting_Bug
				wxLogDebug(_T("Cell::Draw(), *PURPLE* at %d  %s , Range begins at %d  Ends at %d"),
					m_pOwningPile->m_pSrcPhrase->m_nSequNumber, m_pOwningPile->m_pSrcPhrase->m_srcPhrase,
					gnBeginInsertionsSequNum, gnEndInsertionsSequNum);
#endif
			}
		}
	}

    // finally, since the cell's background colour has been set appropriately, the
    // foreground drawing (ie. the text drawning) can be done... so draw every cell except
    // where the phrase box is going to be displayed - i.e. don't draw in the cell with
    // m_nCell index value == 1 at the active pile's location in the layout
   	if (m_nCell != 1 ||
		m_pOwningPile->m_pSrcPhrase->m_nSequNumber != m_pLayout->m_pApp->m_nActiveSequNum)
	{
#ifdef Gilaki_Wide_Margin_Bug
		if (m_pLayout->m_pApp->m_bIsPrinting && m_nCell == 0 && m_pOwningPile->m_pOwningStrip->m_nStrip <= 3)
		{
            // check where the left and right boundaries are for the piles in first 4
            // strips
			wxLogDebug(_T("CCell::Draw(), Strip[ %d ], Pile[ %d ] (in strip), Left %d Right %d"),
				m_pOwningPile->m_pOwningStrip->m_nStrip, m_pOwningPile->m_nPile, Left(), Left() + Width());
		}
#endif
		DrawCell(pDC, color);
	}
	else
	{
        // but we will draw it if a "Find Next" is current (gbFindIsCurrent gets cleared to
        // FALSE either with a click in the view, or when the Find dialog's OnCancel()
        // function is called)
		if (gbFindIsCurrent)
		{
			//m_pText->Draw(pDC);
			DrawCell(pDC, color);
		}
	}
	// restore the white background for cells -- it is the default for the next call
	// to this function
	pDC->SetBackgroundMode(m_pLayout->m_pApp->m_backgroundMode); // wxSOLID
	pDC->SetTextBackground(wxColour(255,255,255)); // white
	// Note, drawing of the nav text & icons (note icon, green wedge, etc) is now in CPile
}

void CCell::DrawCell(wxDC* pDC, wxColor color)
{
    // we assume at the time DrawCell() is called that it's owning pile's pointer to the
    // CSourcePhrase has been updated, if necessary, to comply with the user's changes to
    // the document (if it hasn't, DrawCell() will crash, so we'll find out soon enough!)
    // Also, the caller must determine the text color, because it can be temporarily
    // changed depending on what the user is currently doing (eg. to gray text when in
    // vertical edit mode)

	// assign to local var thePhrase whichever wxString should be written for this cell
	wxString* pPhrase = GetCellText();
	wxRect enclosingRect; // the rect where the text will be drawn
	GetCellRect(enclosingRect); // set it
	pDC->SetFont(*GetFont()); // pDC->SetFont(*m_pFont);
	wxColour oldColor = color; // Note: MFC code does not use oldColor anywhere
	if (!color.IsOk())
	{
		::wxBell();
		wxASSERT(FALSE);
	}
	pDC->SetTextForeground(color);
	bool bRTLLayout;
	bRTLLayout = FALSE;
#ifdef _RTL_FLAGS
	if (m_nCell == 0)
	{
		// it's source text
		if (m_pLayout->m_pApp->m_bSrcRTL)
		{
			// source text has to be RTL & aligned RIGHT
			bRTLLayout = TRUE;
		}
		else
		{
			// source text has to be LTR & aligned LEFT
			bRTLLayout = FALSE;
		}
	}
	else // m_nCellIndex could be 2, or 3
	{
        // if glossing is seen (ie. gbGlossingVisible is TRUE) then it could be target text
        // or gloss text in the cell, else must be target text
		if (!gbGlossingVisible)
		{
			// glossing not enabled (nor seen), so can only be target text in the cell
			if (m_pLayout->m_pApp->m_bTgtRTL)
			{
				// target text has to be RTL & aligned RIGHT
				bRTLLayout = TRUE;
			}
			else
			{
				// target text has to be LTR & aligned LEFT
				bRTLLayout = FALSE;
			}
		}
		else // glossing enabled (and seen, but it may or may not be in glossing mode)
		{
			// m_nCell == 1  or == 2 will be gloss text, which might be
			// in the target text's direction, or navText's direction; any other option
			// must be one of the target text cells
			if ((gbIsGlossing && m_nCell == 1) || (!gbIsGlossing && m_nCell == 2))
			{
                // it's gloss text - the direction will depend on the setting for the font
                // used test to see which direction we have in operation for this cell
				if (gbGlossingUsesNavFont)
				{
					// glossing uses navText's direction
					if (m_pLayout->m_pApp->m_bNavTextRTL)
						bRTLLayout = TRUE;
					else
						bRTLLayout = FALSE;
				}
				else // glossing uses target text's direction
				{
					if (m_pLayout->m_pApp->m_bTgtRTL)
						bRTLLayout = TRUE;
					else
						bRTLLayout = FALSE;
				}
			}
			else // must be a cell with target text's font - so use that directionality
			{
				if (m_pLayout->m_pApp->m_bTgtRTL)
					bRTLLayout = TRUE;
				else
					bRTLLayout = FALSE;
			}
		}
	}
#endif // for _RTL_FLAGS

    // use gbFindIsCurrent to see if I can get the empty cell to show a yellow background
    // when Find has found a legitimate null match, but inhibit the yellow background for
    // clicks in the cell at other times - the inhibiting is of course done in the
    // OnLButtonDown( ) function in the view class rather than here
	if ((*pPhrase).IsEmpty() && gbFindIsCurrent)
	{
		// create a string of spaces of length which will fit within the pileWidth
		// so that a click on the empty cell will still show a yellow background
		// for the selection
		wxString aChar = _T(" "); // one space
		wxString str = _T("");
		int charWidth;
		int charHeight;
		pDC->GetTextExtent(aChar,&charWidth,&charHeight);
		int nominalWidth = enclosingRect.GetWidth() - charWidth;
		int strWidth = 0;
		while (strWidth < nominalWidth)
		{
			str += _T(" ");
			strWidth += charWidth;
		}
		// Because of different behaviors of DrawText() when drawing RTL text on
		// the different platforms, I've written a DrawTextRTL() function to account
		// for those differences (see notes in DrawTextRTL for further information).
		if (bRTLLayout)
		{
			// ******** Draw RTL Blank Spaces Cell Text  *********
			DrawTextRTL(pDC,str,enclosingRect);
		}
		else
		{
			// ******** Draw LTR Blank Spaces Cell Text  *********
			pDC->DrawText(str,enclosingRect.GetLeft(), enclosingRect.GetTop());
		}
	}
	else // find is not current or there is some text to draw
	{
		// Because of different behaviors of DrawText() when drawing RTL text on
		// the different platforms, I've written a DrawTextRTL() function to account
		// for those differences (see notes in DrawTextRTL for further information).
		if (bRTLLayout)
		{
			//wxSize sizeOfPhrase = pDC->GetTextExtent(m_phrase);
			// ********* Draw RTL Cell Text  ***********
			DrawTextRTL(pDC,*pPhrase,enclosingRect);
		}
		else
		{
			// ********* Draw LTR Cell Text  **********
			pDC->DrawText(*pPhrase,enclosingRect.GetLeft(), enclosingRect.GetTop());
/*
#if defined(__WXGTK__)
			// a kludge to see if we can get anything printed below the strip in GTK build when printing free trans
			// -- Yes, it works; so I can build a function to exploit this...
			// It's now 22Nov and I've built such a function and called it from CPile::Draw() but it not only doesn't
			// draw anything on the page (even though correct DrawText() calls are made), it also clobbers the drawn
			// source and target lines of most of the strips except a little bit at top strip - their area is white too!
			wxString ftStr;
			CPile* pPile = GetPile();
			CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
			ftStr = pSrcPhrase->GetFreeTrans();
			if (m_pLayout->m_pApp->m_bIsPrinting && !m_pLayout->m_pApp->m_bIsPrintPreviewing
                && !ftStr.IsEmpty() && this->GetCellIndex() == 1)
			{
			    wxColour ftColor(m_pLayout->m_pApp->m_freeTransTextColor);
                pDC->SetTextForeground(ftColor);
			    pDC->DrawText(ftStr,enclosingRect.GetLeft(), enclosingRect.GetTop() + m_pLayout->GetTgtTextHeight() + 3);
			}
#endif
*/
		}
	}
}

void CCell::DrawTextRTL(wxDC* pDC, wxString& str, wxRect& rect)
{
	// Note: BEW 9Feb09, a copy of this function is also in CAdapt_ItView class - that
	// copy is used only when drawing RTL text in free translation mode.
	//
	// This function attempts to alieviate the limitations of wxDC::DrawText() which
	// does not have an nFormat parameter as does the MFC function CDC::DrawText()
	// and which currently (as of wxWidgets 2.8.4) has different behaviors on wxMSW,
	// wxGTK and wxMAC.
    // The challenge here is due in part to the fact that the wxDC::SetLayoutDirection()
    // method applies to the whole DC and, on wxMSW (but not the other ports)
    // SetLayoutDirection() effectively mirrors the underlying coordinate system (requiring
    // coordinate values input to DrawText and other drawing methods be reversed on the
    // x-axis while SetLayoutDirection() is set to RTL. Hence, for wxMSW, we must transform
    // the coordinates in such a way that, while mirroring is in effect, the logical 0,0
    // coordinate for drawing text is located at the upper right corner of the logical
    // document and x coordinate values increase toward the left rather than toward the
    // right. The wxGTK and wxMac ports do not require the use of SetLayoutDirection() to
    // render punctuation correctly for RTL text, but drawing RTL text in those cases
    // requires that we must adjust the drawing coordinates of RTL text to start drawing at
    // the left end of the resulting RTL text, rather than at the right end (as MFC does).
    // I've also used a wxRect as parameter rather than wxCoord x and wxCoord y coordinate
    // parameters to make the upper right corner of the wxRect more readily available to
    // the function.
	//
	// TODO: Since the wxWidgets RTL rendering behaviors are obviously still somewhat in an
	// immature state of development (and may change in future library releases to become
	// more uniform across platforms) this issue needs to be visited again upon any anticipated
	// upgrade to newer versions of wxWidgets beyond version 2.8.4.
	//
	// For wxMSW we must transform the coordinates of rect's upper right corner to
	// account for the mirroring of the underlying coordinate system on Windows
	// (the wxGTK and wxMac do not seem to do the mirroring of the underlying
	// coordinates).
	// first get the upper right coords of the drawing rect
	wxPoint urPt(rect.GetRight(),rect.GetTop());
	// transform this point to have mirrored x-axis coordinates. grectViewClient should
	// indicate the current client view's total width, so if we subtract the urPt.x value
	// from grectViewClient.GetWidth(), we should have the "mirrored" x-axis coordinate
	// value for RTL layout.
	// whm modified 7Sep09 The code below to account for differences that need to be taken
	// into account when printing or print previewing
	if (m_pLayout->m_pApp->m_bIsPrinting)
	{
		// wxGTK and wxMac need to start drawing text at the point urPt less the width/extent
		// of the text to be drawn
		wxSize sizeOfPhrase = pDC->GetTextExtent(str);
		pDC->DrawText(str,urPt.x - sizeOfPhrase.x,urPt.y);
	}
	else
	{
#ifdef __WXMSW__
		// wxMSW needs SetLayoutDirection(wxLayout_RightToLeft) to be set; in addition we
		// need to transform the urPt so it is mirrored from the right edge of
		// grectViewClient.
		wxASSERT(grectViewClient.GetWidth() >= urPt.x); // ensure grectViewClient is already set
		pDC->SetLayoutDirection(wxLayout_RightToLeft);
		pDC->DrawText(str,grectViewClient.GetWidth() - urPt.x - 16,urPt.y); // 16 pixels for scrollbar width
#else
		// wxGTK and wxMac need to start drawing text at the point urPt less the width/extent
		// of the text to be drawn
		wxSize sizeOfPhrase = pDC->GetTextExtent(str);
		pDC->DrawText(str,urPt.x - sizeOfPhrase.x,urPt.y);
#endif
	}

#ifdef __WXMSW__
	// turning off RTL layout of the DC after calling DrawText() would seem to be necessary
	// to best emulate what MFC's DrawText() does with its nFormat parameter
	pDC->SetLayoutDirection(wxLayout_LeftToRight); // need this???
#endif
}

wxString* CCell::GetCellText()
{
	switch (m_nCell)
	{
	case 0: // source text line
		{
			if (m_pLayout->m_pApp->m_bHidePunctuation)
			{
				// showing the text with punctuation stripped off
				m_pPhrase = &m_pOwningPile->m_pSrcPhrase->m_key;
			}
			else
			{
				// showing punctuation with the text
				m_pPhrase = &m_pOwningPile->m_pSrcPhrase->m_srcPhrase;
			}
			break;
		}
	case 2: // gloss text line if glossing, else adaptation text line
		if (gbIsGlossing)
		{
			// this is the adaptation line, when glossing mode is on
			if (m_pLayout->m_pApp->m_bHidePunctuation)
			{
				// showing the text with punctuation stripped off
				m_pPhrase = &m_pOwningPile->m_pSrcPhrase->m_adaption;
			}
			else
			{
				// showing punctuation with the text
				m_pPhrase = &m_pOwningPile->m_pSrcPhrase->m_targetStr;
			}
		}
		else
		{
			// this is the gloss line when glossing mode is off
			m_pPhrase = &m_pOwningPile->m_pSrcPhrase->m_gloss;
		}
		break;
	default: // adaptation line
		if (gbIsGlossing)
		{
			// this line has the glosses when glossing mode is on
			m_pPhrase = &m_pOwningPile->m_pSrcPhrase->m_gloss;
		}
		else
		{
			// this line has the adaptations, when glossing mode is off
			if (m_pLayout->m_pApp->m_bHidePunctuation)
			{
				// showing the text with punctuation stripped off
				m_pPhrase = &m_pOwningPile->m_pSrcPhrase->m_adaption;
			}
			else
			{
				// showing punctuation with the text
				m_pPhrase = &m_pOwningPile->m_pSrcPhrase->m_targetStr;
			}
		}
		break;
	}
	return m_pPhrase;
}

