/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Pile.cpp
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
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
#include "Cell.h"
#include "Pile.h"
#include "Strip.h"
#include "SourceBundle.h"
#include "Adapt_ItDoc.h"
#include "SourcePhrase.h"
#include "AdaptitConstants.h"
#include "Adapt_ItView.h"

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

extern bool gbIsPrinting; // whm added because wxDC does not have ::IsPrinting() method

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(CPile, wxObject)

CPile::CPile()
{
	m_bIsActivePile = FALSE;
	m_nPileIndex = 0;
	m_rectPile = wxRect(0,0,0,0);
	// NOTE: Difference in MFC's CRect and wxWidgets' wxRect using 4 parameters: 
	// MFC has (left, top, right, bottom); wxRect has (x, y, width, height).
	// When initializing to 0,0,0,0 it doesn't matter, but may matter when used
	// in CAdapt_ItView and CPhraseBox.
	m_nWidth = 40;
	m_nMinWidth = 40;
	m_nHorzOffset = 0;
	m_pCell[0] = m_pCell[1] = m_pCell[2] = m_pCell[3] = (CCell*)NULL;
	m_pSrcPhrase = (CSourcePhrase*)NULL;
	m_bIsCurrentFreeTransSection = FALSE; // BEW added 24Jun05 for free translation support
}

//CPile::CPile(CAdapt_ItDoc* pDocument, CSourceBundle* pSourceBundle, CStrip* pStrip,
//			CSourcePhrase* pSrcPhrase) // BEW deprecated 3Feb09
CPile::CPile(CSourceBundle* pSourceBundle, CStrip* pStrip, CSourcePhrase* pSrcPhrase)
{
	// m_pDoc = pDocument; // BEW deprecated 3Feb09
	m_pBundle = pSourceBundle;
	m_pStrip = pStrip;
	m_bIsActivePile = FALSE;
	m_bIsCurrentFreeTransSection = FALSE; // BEW added 24Jun05 for free translation support
	m_nPileIndex = 0;
	m_rectPile = wxRect(0,0,0,0);
	// NOTE: Difference in MFC's CRect and wxWidgets' wxRect using 4 parameters: 
	// MFC has (left, top, right, bottom); wxRect has (x, y, width, height).
	// When initializing to 0,0,0,0 it doesn't matter, but may matter when used
	// in CAdapt_ItView and CPhraseBox.
	m_nWidth = 20;
	m_nMinWidth = 40;
	m_nHorzOffset = 0;
	m_pSrcPhrase = pSrcPhrase;
	m_pCell[0] = m_pCell[1] = m_pCell[2] = m_pCell[3] = m_pCell[4] = (CCell*)NULL;
}

CPile::~CPile()
{

}

// implementation

void CPile::DestroyCells()
{
	for (int i=0; i<MAX_CELLS; i++)
	{
		if (m_pCell[i] != NULL)
		{
			//if (m_pCell[i]->m_pText != NULL) // BEW removed 6Feb09
			//{
			//	delete m_pCell[i]->m_pText;
			//	m_pCell[i]->m_pText = (CText*)NULL;
			//}
			delete m_pCell[i];
			m_pCell[i] = (CCell*)NULL;
		}
	}
}

void CPile::Draw(wxDC* pDC)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	for (int i=0; i< MAX_CELLS; i++)
	{
		if (m_pCell[i] != NULL)
			m_pCell[i]->Draw(pDC);
	}

	if (gbIsPrinting)
	{
		// draw the phrase box if it belongs to this pile
		wxTextCtrl* pBox = pApp->m_pTargetBox;
		wxASSERT(pBox);
		wxRect rectBox;
		int width;
		int height;
		if (pBox != NULL && pApp->m_nActiveSequNum != -1)
		{
			if (m_pSrcPhrase->m_nSequNumber == pApp->m_nActiveSequNum)
			{
				rectBox = pBox->GetRect();
				width = rectBox.GetWidth(); // these will be old MM_TEXT mode 
											// logical coords, but
				height = rectBox.GetHeight(); // that will not matter

				// this pile contains the phrase box, pApp->m_ptCurBoxLocation is still in MM_TEXT coords, so
				// get the proper coords (MM_LOENGLISH) from the CCell[2]'s rectangle
				// whm note: When printing in MFC the cell's m_ptTopLeft.y is negative, whereas
				// m_ptCurBoxLocation.y is positive (absolute value of y is the same for both).
				wxPoint topLeft = m_pCell[2]->m_ptTopLeft;
				// Note: GetMargins not supported in wxWidgets' wxTextCtrl (nor MFC's RichEdit3)
				//DWORD boxMargins = pApp->m_targetBox.GetMargins();
				//int leftMargin = (int)LOWORD(boxMargins);
				int leftMargin = 2; // we'll hard code 2 pixels on left as above - check this ???
				wxPoint textTopLeft = topLeft;
				textTopLeft.x += leftMargin;
				int topMargin;
				if (gbIsGlossing && gbGlossingUsesNavFont)
					topMargin = abs((height - pApp->m_nNavTextHeight)/2); // whm this is OK
				else
					topMargin = abs((height - pApp->m_nTgtHeight)/2); // whm this is OK
				textTopLeft.y -= topMargin;
				wxFont SaveFont;
				wxFont TheFont;
				CAdapt_ItApp* pApp = &wxGetApp(); // added for calls below
				if (gbIsGlossing && gbGlossingUsesNavFont)
					TheFont = *pApp->m_pNavTextFont;
				else
					TheFont = *pApp->m_pTargetFont;
				SaveFont = pDC->GetFont();
				pDC->SetFont(TheFont);

				if (!m_pCell[2]->m_color.IsOk())
				{
					::wxBell();
					wxASSERT(FALSE);
				}
				pDC->SetTextForeground(m_pCell[2]->m_color); // use color for this cell to print
														// the box's text
				
				// /////////////// Draw the Target Text for the phrasebox //////////////////////
				pDC->DrawText(pApp->m_targetPhrase,textTopLeft.x,textTopLeft.y);	// MFC uses TextOut()
																					// Note: diff param ordering!
				pDC->SetFont(SaveFont);

				// /////////////////// Draw the Box around the target text //////////////////////
				pDC->SetPen(*wxBLACK_PEN); // whm added 20Nov06
				
				// whm: wx version flips top and bottom when rect coords are negative to maintain true
				// "top" and "bottom". In the DrawLine code below MFC has -height but the wx version
				// has +height.
				pDC->DrawLine(topLeft.x, topLeft.y, topLeft.x+width, topLeft.y);
				pDC->DrawLine(topLeft.x+width, topLeft.y, topLeft.x+width, topLeft.y +height);
				pDC->DrawLine(topLeft.x+width, topLeft.y+height, topLeft.x, topLeft.y +height);
				pDC->DrawLine(topLeft.x, topLeft.y+height, topLeft.x, topLeft.y);
				pDC->SetPen(wxNullPen);
			}
		}
	}
}

