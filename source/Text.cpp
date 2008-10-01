/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			Text.cpp
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the CText class. 
/// The CText class lowest level unit in the bundle-strip-pile-cell-text
/// hierarchy of objects forming the view displayed to the user on the
/// canvas of the main window of the Adapt It application.
/// \derivation		The CText class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. Make sure GetTextExtent() below returns logical units as does MFC's GetTextExtent()
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Text.h"
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
#include "Adapt_ItView.h"
#include "Text.h"

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbEnableGlossing; // TRUE makes Adapt It revert to Shoebox functionality only

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // want to access it fast

extern bool gbFindIsCurrent;

// whm NOTES CONCERNING RTL and LTR Rendering in wxWidgets:
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

IMPLEMENT_DYNAMIC_CLASS(CText, wxObject)

CText::CText()
{
}

CText::CText(wxPoint& start, wxPoint& end, wxFont* pFont,
			const wxString& phrase, const wxColour& color, int nCell)
{
	m_color = color; // wxColour has = operator override
	m_phrase = phrase;
	m_topLeft = start;
	m_enclosingRect = wxRect(start,end);
	m_pFont = pFont;
	m_bSelected =  FALSE;
	m_nCell = nCell;
}

CText::~CText()
{

}

void CText::Draw(wxDC* pDC)
{
	CAdapt_ItView* pView = gpApp->GetView();
	wxASSERT(pView != NULL);
	pDC->SetFont(*m_pFont);
	wxColour color(m_color);
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
	if (m_nCell == 0 || m_nCell == 1)
	{
		// it's source text
		if (gpApp->m_bSrcRTL)
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
	else // m_nCell could be 2, 3 or  4
	{
		// if glossing is allowed then it could be target text or gloss text 
		// in the cell, else must be target text
		if (!gbEnableGlossing)
		{
			// glossing not enabled, so can only be target text in the cell
			if (gpApp->m_bTgtRTL)
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
		else // glossing allowed
		{
			// m_nCell == 2  or == 5 will be gloss text, which might be
			// in the target text's direction, or navText's direction; any other option
			// must be one of the target text cells
			if ((gbIsGlossing && m_nCell == 2) || (!gbIsGlossing && m_nCell == 4))
			{
				// test to see which direction we have in operation for this cell
				if (gbGlossingUsesNavFont)
				{
					// glossing uses navText's direction
					if (gpApp->m_bNavTextRTL)
						bRTLLayout = TRUE;
					else
						bRTLLayout = FALSE;
				}
				else // glossing uses target text's direction
				{
					goto x;
				}
			}
			else // must be cell with target text
			{
x:				if (gpApp->m_bTgtRTL)
					bRTLLayout = TRUE;
				else
					bRTLLayout = FALSE;
			}
		}
	}
#endif // for _RTL_FLAGS

	// use gbFindIsCurrent  to see if I can get the empty cell to show a yellow background
	// when Find has found a legitimate null match, but inhibit the yellow background for
	// clicks in the cell at other times - the inhibiting is of course done in the
	// OnLButtonDown( ) function in the view class rather than here
	if (m_phrase.IsEmpty() && gbFindIsCurrent)
	{
		// create a string of spaces of length which will fit within the pileWidth
		// so that a click on the empty cell will still show a yellow background
		// for the selection
		wxString aChar = _T(" "); // one space
		wxString str = _T("");
		int charWidth;
		int charHeight;
		pDC->GetTextExtent(aChar,&charWidth,&charHeight);
		int nominalWidth = m_enclosingRect.GetWidth() - charWidth;
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
			// ////////// Draw RTL Blank Spaces Cell Text  /////////////////
			pView->DrawTextRTL(pDC,str,m_enclosingRect);
		}
		else
		{
			// ////////// Draw LTR Blank Spaces Cell Text  /////////////////
			pDC->DrawText(str,m_enclosingRect.GetLeft(), m_enclosingRect.GetTop());// ,nFormat);
		}
	}
	else // find is not current or there is some text to draw
	{
		// Because of different behaviors of DrawText() when drawing RTL text on
		// the different platforms, I've written a DrawTextRTL() function to account
		// for those differences (see notes in DrawTextRTL for further information).
		if (bRTLLayout)
		{
#ifdef _LOG_DEBUG_DRAWING
			wxLogDebug(_T("DrawText [%d chars] cell%d right align"),m_phrase.Length(),this->m_nCell);
#endif
			wxSize sizeOfPhrase = pDC->GetTextExtent(m_phrase);
			// ////////// Draw RTL Cell Text  /////////////////
			pView->DrawTextRTL(pDC,m_phrase,m_enclosingRect);
		}
		else
		{
#ifdef _LOG_DEBUG_DRAWING
			wxLogDebug(_T("DrawText [%d chars] cell%d left align"),m_phrase.Length(),this->m_nCell);
#endif
			// ////////// Draw LTR Cell Text  /////////////////
			pDC->DrawText(m_phrase,m_enclosingRect.GetLeft(), m_enclosingRect.GetTop());// ,nFormat);
		}
	}
}
