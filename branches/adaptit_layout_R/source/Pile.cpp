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

// these next globals are put here when I moved CreatePile() from the view to the CPile class
extern bool gbShowTargetOnly;
extern bool gbEnableGlossing;
extern CAdapt_ItApp* gpApp;
extern const wxChar* filterMkr;

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
	m_navTextColor = gpApp->m_navTextColor;
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
	m_navTextColor = gpApp->m_navTextColor;
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

bool CPile::HasFilterMarker()
{
	return m_pSrcPhrase->m_markers.Find(filterMkr) >= 0;
}

void CPile::Draw(wxDC* pDC)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = pApp->GetView();

	for (int i=0; i< MAX_CELLS; i++)
	{
		if (m_pCell[i] != NULL)
			m_pCell[i]->Draw(pDC);
	}
	
	// nav text whiteboard drawing for this pile...
	if (!gbIsPrinting)
	{
		bool bRTLLayout;
		bRTLLayout = FALSE;
		wxRect rectBounding = m_rectPile;
		wxPoint ptTopLeft = rectBounding.GetTopLeft();
		wxPoint ptBotRight = rectBounding.GetBottomRight();

		rectBounding.SetTop(ptTopLeft.y);
		rectBounding.SetLeft(ptTopLeft.x);
		rectBounding.SetBottom(ptBotRight.y);
		rectBounding.SetRight(ptBotRight.x);
		#ifdef _RTL_FLAGS
			if (gpApp->m_bNavTextRTL)
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

		// stuff below is for drawing the navText stuff above this pile of the strip
		// Note: in the wx version m_bSuppressFirst is now located in the App
		//if (((m_nCellIndex == 0 && !gpApp->m_bSuppressFirst) ||
		//	 (m_nCellIndex == 1 && gpApp->m_bSuppressFirst)) && !gbShowTargetOnly )
		if (!gbShowTargetOnly )
		{
			int xOffset = 0;
			int diff;
			//bool bHasFilterMarker = HasFilterMarker(m_pPile);
			bool bHasFilterMarker = HasFilterMarker();

			// if a message is to be displayed above this word, draw it too
			//if (m_pPile->m_pSrcPhrase->m_bNotInKB)
			if (m_pSrcPhrase->m_bNotInKB)
			{
				wxPoint pt;

				//pt = m_ptTopLeft;
				pt = ptTopLeft;
	#ifdef _RTL_FLAGS
				if (pApp->m_bNavTextRTL)
					pt.x += rectBounding.GetWidth(); // align right
	#endif
				// whm: the wx version doesn't use negative offsets
				// Note: m_nNavTextHeight and m_nSrcHeight are now located on the App
				diff = pApp->m_nNavTextHeight - (pApp->m_nSrcHeight/4);
				pt.y -= diff;
				wxString str;
				xOffset += 8;
				//if (m_pPile->m_pSrcPhrase->m_bBeginRetranslation)
				if (m_pSrcPhrase->m_bBeginRetranslation)
					str = _T("*# "); // visibly mark start of a retranslation section
				else
					str = _T("* ");
				wxFont SaveFont;
				wxFont* pNavTextFont = pApp->m_pNavTextFont;
				SaveFont = pDC->GetFont(); // save current font
				pDC->SetFont(*pNavTextFont);
				if (!m_navTextColor.IsOk())
				{
					::wxBell(); 
					wxASSERT(FALSE);
				}
				pDC->SetTextForeground(m_navTextColor);

				rectBounding.Offset(0,-diff);
				// wx version can only set layout direction directly on the whole pDC. 
				// Uniscribe in wxMSW and Pango in wxGTK automatically take care of the
				// right-to-left reading of the text, but we need to manually control 
				// the right-alignment of text when we have bRTLLayout. The upper-left
				// x coordinate for RTL drawing of the m_phrase should be 
				// m_enclosingRect.GetLeft() + widthOfCell - textExtOfPhrase.
				if (bRTLLayout)
				{
					// ////////// Draw the RTL Retranslation section marks *# or * in Navigation Text area /////////
					// whm note: nav text stuff can potentially be much wider than the width of
					// the cell where it is drawn. This would not usually be a problem since nav
					// text is not normally drawn above every cell but just at major markers like
					// at ch:vs points, section headings, etc. For RTL the nav text could extend
					// out and be clipped beyond the left margin.
					pView->DrawTextRTL(pDC,str,rectBounding);
				}
				else
				{
					// ////////// Draw the LTR Retranslation section marks *# or * in Navigation Text area /////////
					pDC->DrawText(str,rectBounding.GetLeft(),rectBounding.GetTop());
				}
				pDC->SetFont(SaveFont);
			}

			// next stuff is for the green wedge - it should be shown at the left or the right
			// of the pile depending on the gbRTL_Layout flag (FALSE or TRUE, respectively), rather
			// than the nav text's directionality
			//if (m_pPile->m_pSrcPhrase->m_bFirstOfType || m_pPile->m_pSrcPhrase->m_bVerse
			//	|| m_pPile->m_pSrcPhrase->m_bChapter || m_pPile->m_pSrcPhrase->m_bParagraph
			//	|| m_pPile->m_pSrcPhrase->m_bFootnoteEnd || m_pPile->m_pSrcPhrase->m_bHasInternalMarkers
			//	|| m_pPile->m_pSrcPhrase->m_markers.Find(filterMkr) != -1)
			if (m_pSrcPhrase->m_bFirstOfType || m_pSrcPhrase->m_bVerse
				|| m_pSrcPhrase->m_bChapter || m_pSrcPhrase->m_bParagraph
				|| m_pSrcPhrase->m_bFootnoteEnd || m_pSrcPhrase->m_bHasInternalMarkers
				|| m_pSrcPhrase->m_markers.Find(filterMkr) != -1)
			{
				wxPoint pt = ptTopLeft;
				//pt = m_ptTopLeft;

				// BEW added, for support of filter wedge icon
				if (bHasFilterMarker && !gbShowTargetOnly)
				{
					xOffset = 7;  // offset any nav text 7 pixels to the right (or to the left if RTL rendering)
									// to make room for the wedge
				}

	#ifdef _RTL_FLAGS
				if (pApp->m_bRTL_Layout) // was gpApp->m_bNavTextRTL
					pt.x += rectBounding.GetWidth(); // align right
	#endif
				// whm: the wx version doesn't use negative offsets
				// Note: m_nNavTextHeight and m_nSrcHeight are now located on the App
				diff = pApp->m_nNavTextHeight + (pApp->m_nSrcHeight/4) + 1;
				pt.y -= diff;
	#ifdef _RTL_FLAGS
				if (pApp->m_bRTL_Layout) // was gpApp->m_bNavTextRTL
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
				// m_inform should have, at most, the name or short name of the last nonchapter & nonverse
				// (U)SFM, and no information about anything filtered. All we need do with it is to append
				// this information to str containing n:m if the latter is pertinent here, or fill str with m_inform
				// if there is no chapter or verse info here. The construction of a wedge for signallying presence
				// of filtered info in m_markers is also handled here, but independently of m_inform
				wxString str = _T("");

				if (bHasFilterMarker && !gbShowTargetOnly)
				{
					// if \~FILTERED is anywhere in the m_markers member of the sourcephrase stored which
					// has its pointer stored in m_pPile, then we require a wedge to be drawn here.
					// BEW modified 18Nov05; to have variable colours, also the colours differences are
					// hard to pick up with a simple wedge, so the top of the wedge has been extended up
					// two more pixels to form a column with a point at the bottom, which can show more colour
					wxPoint ptWedge = ptTopLeft;

					// BEW added 18Nov05, to colour the wedge differently if \free is contentless (as khaki), or if
					// \bt is contentless (as pastel blue), or if both are contentless (as red)
					//bool bFreeHasNoContent = m_pBundle->m_pView->IsFreeTranslationContentEmpty(m_pPile->m_pSrcPhrase);
					//bool bBackHasNoContent = m_pBundle->m_pView->IsBackTranslationContentEmpty(m_pPile->m_pSrcPhrase);
					CAdapt_ItView* pView = pApp->GetView(); // getting the view pointer this way allows me
					// to remove the m_pView pointer from the definition of the CSourceBundle class
					//bool bFreeHasNoContent = pView->IsFreeTranslationContentEmpty(m_pPile->m_pSrcPhrase);
					//bool bBackHasNoContent = pView->IsBackTranslationContentEmpty(m_pPile->m_pSrcPhrase);
					bool bFreeHasNoContent = pView->IsFreeTranslationContentEmpty(m_pSrcPhrase);
					bool bBackHasNoContent = pView->IsBackTranslationContentEmpty(m_pSrcPhrase);

					#ifdef _RTL_FLAGS
					if (pApp->m_bRTL_Layout)
						ptWedge.x += rectBounding.GetWidth(); // align right if RTL layout
					#endif
					// get the point where the drawing is to start (from the bottom tip of the downwards pointing wedge)
					ptWedge.x += 1;
					ptWedge.y -= 2; 

					// whm note: According to wx docs, wxWidgets shows all non-white pens as black on a monochrome
					// display, i.e., OLPC screen in Black & White mode. In contrast, wxWidgets shows all brushes 
					// as white unless the colour is really black on monochrome displays.
					pDC->SetPen(*wxBLACK_PEN);

					// draw the line to the top left of the wedge
					pDC->DrawLine(ptWedge.x, ptWedge.y, ptWedge.x - 5, ptWedge.y - 5); // end points are not part of the line

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
					// we can quickly fill the wedge by brute force by drawing a few green horizontal lines
					// rather than using more complex region calls
					// BEW on 18Nov05 added extra tests and colouring code to support variable colour 
					// to indicate when free translation or back translation fields are contentless

					if (!bFreeHasNoContent && !bBackHasNoContent)
					{
						// make whole inner part of wedge green

						pDC->SetPen(*wxGREEN_PEN);
						
						// add the two extra lines for the short column's colour
a:					pDC->DrawLine(ptWedge.x - 3, ptWedge.y - 6, ptWedge.x + 4, ptWedge.y - 6);
						pDC->DrawLine(ptWedge.x - 3, ptWedge.y - 5, ptWedge.x + 4, ptWedge.y - 5);
						// now the former stuff for an actual wedge shape
						pDC->DrawLine(ptWedge.x - 3, ptWedge.y - 4, ptWedge.x + 4, ptWedge.y - 4);
						pDC->DrawLine(ptWedge.x - 2, ptWedge.y - 3, ptWedge.x + 3, ptWedge.y - 3);
						pDC->DrawLine(ptWedge.x - 1, ptWedge.y - 2, ptWedge.x + 2, ptWedge.y - 2);
						pDC->DrawLine(ptWedge.x , ptWedge.y - 1, ptWedge.x + 1, ptWedge.y - 1);
					}
					else
					{
						if (bFreeHasNoContent && bBackHasNoContent)
						{
							// use a third colour - red grabs attention best
							pDC->SetPen(*wxRED_PEN);
							goto a;
						}
						else
						{
							// both are not together contentless, so only one of them is
							// so do the one which it is in the appropriate colour
							if (bFreeHasNoContent)
							{
								// colour it khaki
								pDC->SetPen(wxPen(wxColour(160,160,0), 1, wxSOLID));
								goto a;
							}
							if (bBackHasNoContent)
							{
								// colour it pastel blue
								pDC->SetPen(wxPen(wxColour(145,145,255), 1, wxSOLID));
								goto a;
							}
						}
					}
					pDC->SetPen(wxNullPen); // wxNullPen causes
											// the current pen to be selected out of the device context, and the
											// original pen restored.
				}

				// make (for version 3) the chapter&verse information come first
				//if (m_pPile->m_pSrcPhrase->m_bVerse || m_pPile->m_pSrcPhrase->m_bChapter)
				if (m_pSrcPhrase->m_bVerse || m_pSrcPhrase->m_bChapter)
				{
					//str = m_pPile->m_pSrcPhrase->m_chapterVerse;
					str = m_pSrcPhrase->m_chapterVerse;
				}

				// now append anything which is in the m_inform member; there may not have been a chapter
				// and/or verse number already placed in str, so allow for this possibility
				//if (!m_pPile->m_pSrcPhrase->m_inform.IsEmpty())
				if (!m_pSrcPhrase->m_inform.IsEmpty())
				{
					if (str.IsEmpty())
					{
						//str = m_pPile->m_pSrcPhrase->m_inform;
						str = m_pSrcPhrase->m_inform;
					}
					else
					{
						str += _T(' ');
						//str += m_pPile->m_pSrcPhrase->m_inform;
						str += m_pSrcPhrase->m_inform;
					}
				}

				wxFont pSaveFont;
				wxFont* pNavTextFont = pApp->m_pNavTextFont;
				pSaveFont = pDC->GetFont();
				pDC->SetFont(*pNavTextFont);
				if (!m_navTextColor.IsOk())
				{
					::wxBell(); 
					wxASSERT(FALSE);
				}
				pDC->SetTextForeground(m_navTextColor);

				// BEW modified 25Nov05 to move the start of navText to just after the green wedge when
				// filtered info is present, because for fonts with not much internal leading built
				// in, the nav text overlaps the top few pixels of the wedge

				if (bHasFilterMarker)
				{
	#ifdef _RTL_FLAGS
					// we need to make extra space available for some data configurations
					if (pApp->m_bRTL_Layout)
					{
						// right to left layout
						if (pApp->m_bNavTextRTL)
							rectBounding.Offset(-xOffset,-diff); // navText is RTL
						else
							rectBounding.Offset(0,-diff); // navText is LTR
					}
					else
					{
						// left to right layout
						if (pApp->m_bNavTextRTL)
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
					// ////////// Draw RTL Actual Ch:Vs and/or m_inform Navigation Text /////////////////
					pView->DrawTextRTL(pDC,str,rectBounding);
				}
				else
				{
					// ////////// Draw LTR Actual Ch:Vs and/or m_inform Navigation Text /////////////////
					pDC->DrawText(str,rectBounding.GetLeft(),rectBounding.GetTop());
				}
				pDC->SetFont(pSaveFont);
			}

			// now note support
			//if (m_pPile->m_pSrcPhrase->m_bHasNote)
			if (m_pSrcPhrase->m_bHasNote)
			{
				//wxPoint ptNote = m_ptTopLeft;
				wxPoint ptNote = ptTopLeft;
				// offset top left (-13,-9) for regular app
				#ifdef _RTL_FLAGS
				if (pApp->m_bRTL_Layout)
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
				// whm note: According to wx docs, wxWidgets shows all brushes as white unless the colour
				// is really black on monochrome displays, i.e., the OLPC screen in Black & White mode.
				// In contrast, wxWidgets shows all non-white pens as black on monochrome displays.
				pDC->SetBrush(wxBrush(wxColour(254,218,100),wxSOLID));
				pDC->SetPen(*wxBLACK_PEN); // black - whm added 20Nov06
				wxRect insides(ptNote.x,ptNote.y,ptNote.x + 9,ptNote.y + 9);
				// MFC CDC::Rectangle() draws a rectangle using the current pen and fills the interior using the current brush
				pDC->DrawRectangle(ptNote.x,ptNote.y,9,9); // rectangles are drawn with a black border
				pDC->SetBrush(wxNullBrush); // restore original brush - wxNullBrush causes
											// the current brush to be selected out of the device context, and the
											// original brush restored.
				// now the two spirals at the top - left one, then right one
				pDC->DrawLine(ptNote.x + 3, ptNote.y + 1, ptNote.x + 3, ptNote.y + 3);
				pDC->DrawLine(ptNote.x + 2, ptNote.y + 2, ptNote.x + 2, ptNote.y + 3);

				pDC->DrawLine(ptNote.x + 6, ptNote.y + 1, ptNote.x + 6, ptNote.y + 3);
				pDC->DrawLine(ptNote.x + 5, ptNote.y + 2, ptNote.x + 5, ptNote.y + 3);
				pDC->SetPen(wxNullPen);
			}
		}
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

CPile* CPile::CreatePile(wxClientDC *pDC, CAdapt_ItApp *pApp, CSourceBundle *pBundle,
								 CStrip *pStrip, CSourcePhrase *pSrcPhrase, wxRect *pRectPile)
{
	// create the pile on the heap, initializing its cells to MAX_CELLS nulls
	//CPile* pPile = new CPile(pDoc,pBundle,pStrip,pSrcPhrase); // BEW deprecated 3Feb09
	//CPile* pPile = new CPile(pBundle,pStrip,pSrcPhrase); // BEW moved to Pile.cpp
	//wxASSERT(pPile != NULL);
	CPile* pPile = this; // this relieves me of the burden of removing the pPile-> code below
	//CAdapt_ItView* pView = pApp->GetView();

	// now set the attributes not already set by the constructor's defaults
	pPile->m_rectPile = *pRectPile;
	pPile->m_nWidth = pRectPile->GetRight() - pRectPile->GetLeft();

	// the m_nMinWidth value needs to be set to the initial text-extent-based size of the editBox,
	// so that if user removes text from the box it will contract back to this minimum size; the
	// needed value will have been placed in the app's m_nCurPileMinWidth attr by the preceding
	// GetPileWidth() call in the loop within the CreateStrip() function.
	pPile->m_nMinWidth = pApp->m_nCurPileMinWidth;

	// now we have to create the cells which this particular pile owns; this will depend on
	// MAX_CELLS ( == 5 in version 2.0 and onwards), whether or not glossing is allowed, & the user's
	// choice whether or not to suppress display of either 1st or last row (if 4 rows available);
	// and whether or not to show or hide punctuation in lines 2 & 3 (see m_bHidePunctuation flag),
	// and whether or not only the target line is to be shown. When glossing is allowed, the order
	// of the cells from top to bottom will be: if adapting, src, src, tgt, tgt, gloss; if glossing
	// src, src, gloss, tgt, tgt.
	wxPoint  topLeft;
	wxPoint  botRight;
	wxFont*  pSrcFont = pApp->m_pSourceFont;
	wxFont*  pTgtFont = pApp->m_pTargetFont;
	wxFont*  pNavTextFont = pApp->m_pNavTextFont;
	wxSize   extent;
	wxColour color; // MFC had COLORREF color;
	//m_navTextColor = pApp->m_navTextColor; // do it in class creator

	int cellIndex = -1;
	cellIndex++; // set cellIndex to zero

	// set the required color for the text, but if target only, it has only target's color
	// (which can be navText's color if we are glossing - we'll set the target text's color
	// further down in the block.)
	if (!gbShowTargetOnly)
	{
		if (pSrcPhrase->m_bSpecialText)
			color = pApp->m_specialTextColor;
		else
			color = pApp->m_sourceColor;
		if (pSrcPhrase->m_bRetranslation)
			color = pApp->m_reTranslnTextColor;
	}

	// first two src lines are same whether glossing is enabled or not
	if (pApp->m_bSuppressFirst)
	{
		// the first source line is to be suppressed
		cellIndex++; // == 1, so this will be cell[1] not cell[0]

		// suppress the cell if we want to only show target text line; otherwise
		// for gbShowTargetOnly == FALSE we want to show the source line too
		if (!gbShowTargetOnly)
		{
			// calculate the cell's topLeft and botRight points (CText calculates the CRect)
			topLeft = wxPoint(pRectPile->GetLeft(),pRectPile->GetTop());
			botRight = wxPoint(pRectPile->GetRight(), pRectPile->GetBottom());
			// whm: the wx version doesn't use negative offsets
			botRight.y = pRectPile->GetTop() + pApp->m_nSrcHeight;

			// calculate the extent of the text
			int keyWidth;
			int keyDummyHeight;
			pDC->SetFont(*pSrcFont);
			if (pApp->m_bHidePunctuation)
			{
				pDC->GetTextExtent(pSrcPhrase->m_key, &keyWidth, &keyDummyHeight);

				// create the CCell and set its attributes
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//		pSrcPhrase->m_key,keyWidth,pSrcFont,&color,&topLeft,&botRight,
				//		cellIndex); // BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_key,
						keyWidth,pSrcFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
			}
			else
			{
				pDC->GetTextExtent(pSrcPhrase->m_srcPhrase, &keyWidth, &keyDummyHeight);

				// create the CCell and set its attributes
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//		pSrcPhrase->m_srcPhrase,keyWidth,pSrcFont,&color,&topLeft,
				//		&botRight,cellIndex); // BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,
						&pSrcPhrase->m_srcPhrase,keyWidth,pSrcFont,&color,&topLeft,
						&botRight,cellIndex,&m_navTextColor);
			}
		}
	}
	else // two src lines are to be visible
	{
		// this is cell[0]

		if (!gbShowTargetOnly) // don't suppress the cell if we want to show src text line
		{
			// calculate the cell's topLeft and botRight points (CText calculates the CRect)
			topLeft = wxPoint(pRectPile->GetLeft(), pRectPile->GetTop());
			botRight = wxPoint(pRectPile->GetRight(), pRectPile->GetBottom());
			// whm: the wx version doesn't use negative offsets
			botRight.y = pRectPile->GetTop() + pApp->m_nSrcHeight;

			// calculate the extent of the text
			int spWidth;
			int spDummyHeight;
			pDC->SetFont(*pSrcFont);
			pDC->GetTextExtent(pSrcPhrase->m_srcPhrase, &spWidth, &spDummyHeight);

			//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
			//				pSrcPhrase->m_srcPhrase,spWidth,pSrcFont,
			//				&color,&topLeft,&botRight,cellIndex); // BEW deprecated 3Feb09
			CCell* pCell = new CCell();
			pPile->m_pCell[cellIndex] = pCell;
			pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_srcPhrase,spWidth,pSrcFont,
							&color,&topLeft,&botRight,cellIndex,&m_navTextColor);

			// now the second cell, cell[1]
			cellIndex++;

			// whm: the wx version doesn't use negative offsets
			topLeft.y = topLeft.y+pApp->m_nSrcHeight;
			botRight.y = botRight.y+pApp->m_nSrcHeight;

			// calculate the extent of the text
			int keyWidth;
			int keyDummyHeight;
			pDC->GetTextExtent(pSrcPhrase->m_key, &keyWidth, &keyDummyHeight);

			//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
			//				pSrcPhrase->m_key,keyWidth,pSrcFont,&color,
			//				&topLeft,&botRight,cellIndex); // BEW deprecated 3Feb09
			pCell = new CCell();
			pPile->m_pCell[cellIndex] = pCell;
			pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_key,keyWidth,pSrcFont,&color,
							&topLeft,&botRight,cellIndex,&m_navTextColor);
		}
		else
		{
			cellIndex++; // suppress of both wanted, so just ready the index for next increment
		}
	} // end of blocks for one or two source text rows

	if (gbIsGlossing)
	{
		// we are glossing and gbEnableGlossing is TRUE, so glosses will be visible
		// as the 2nd line of the strip (or third if both src lines are visible,) so set up
		// the cell with cellIndex == 2 (and note, if the flag gbShowTargetOnly is TRUE, this
		// would be the only line in the strip which is visible)
		if (gbEnableGlossing)
		{
			// set the gloss text color, and if using navText's font for the glossing
			// then set up the device context to use it; else use target font
			color = pApp->m_navTextColor; // glossing uses navText color always
			if (gbGlossingUsesNavFont)
			{
				pDC->SetFont(*pNavTextFont);
			}
			else
			{
				pDC->SetFont(*pTgtFont);
			}

			// a gloss line is wanted, so construct its cell
			cellIndex++; // cellIndex == 2

			// calculate the cell's topLeft and botRight points
			if (!gbShowTargetOnly) // this flag refers to glosses when gbIsGlossing is TRUE
			{
				if (gbGlossingUsesNavFont)
				{
					// give 3 pixel extra offset, for glossing line only
					// whm: the wx version doesn't use negative offsets
					topLeft.y = topLeft.y+pApp->m_nSrcHeight + 3;
					botRight.y = botRight.y+pApp->m_nNavTextHeight + 3;
				}
				else
				{
					// whm: the wx version doesn't use negative offsets
					topLeft.y = topLeft.y+pApp->m_nSrcHeight + 3;
					botRight.y = botRight.y+pApp->m_nTgtHeight + 3;
				}
			}
			else // we are showing only the glossing text (ie. no source and no target lines)
			{
				// calculate the cell's topLeft and botRight points (CText calculates the CRect),
				// as it is the only cell in the pile, for this mode
				topLeft = wxPoint(pRectPile->GetLeft(), pRectPile->GetTop());
				botRight = wxPoint(pRectPile->GetRight(), pRectPile->GetBottom());
				if (gbGlossingUsesNavFont)
				{
					// whm: the wx version doesn't use negative offsets
					botRight.y = pRectPile->GetTop() + pApp->m_nNavTextHeight;
				}
				else
				{
					// whm: the wx version doesn't use negative offsets
					botRight.y = pRectPile->GetTop() + pApp->m_nTgtHeight;
				}
			}

			// calculate the extent of the text -- but if there is no text, then make
			// the extent equal to the pileWidth (use m_adaption, so will be correct
			// for retranslations, etc)
			int glossWidth;
			int glossDummyHeight;
			if (pSrcPhrase->m_gloss.IsEmpty())
			{
				// no translation or retranslation, etc, so use pileWidth for extent
				glossWidth = pPile->m_nWidth;
			}
			else
			{
				// get the extent of the text
				pDC->GetTextExtent(pSrcPhrase->m_gloss, &glossWidth, &glossDummyHeight);
			}

			// create the cell and set its attributes; which font gets used will
			// depend on the gbGlossingUsesNavFont flag
			if (gbGlossingUsesNavFont)
			{
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//	pSrcPhrase->m_gloss,glossWidth,pNavTextFont,&color,&topLeft,
				//	&botRight,cellIndex); // BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_gloss,glossWidth,
					pNavTextFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
			}
			else
			{
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//	pSrcPhrase->m_gloss,glossWidth,pTgtFont,&color,&topLeft,
				//	&botRight,cellIndex); // BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_gloss,glossWidth,
					pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
			}
		}

		if (!gbShowTargetOnly)
		{
			// continue to 4th visible line (or 3nd visible line if m_bSuppressFirst is TRUE),
			// this is cell[3] and is the first of the lines which may have target text
			cellIndex++; // set cellIndex to 3

			// set the target language's text color
			color = pApp->m_targetColor;

			// calculate the cell's topLeft and botRight points
			if (gbGlossingUsesNavFont)
			{
				// whm: the wx version doesn't use negative offsets
				topLeft.y = topLeft.y+pApp->m_nNavTextHeight;
				botRight.y = botRight.y+pApp->m_nTgtHeight;
			}
			else
			{
				// whm: the wx version doesn't use negative offsets
				topLeft.y = topLeft.y+pApp->m_nTgtHeight;
				botRight.y = botRight.y+pApp->m_nTgtHeight;
			}

			// calculate the extent of the text -- but if there is no text, then make the extent
			// equal to the pileWidth (use m_adaption, so will be correct for retranslations, etc)
			// (if this cell is at the active location, displaying it will be suppressed externally)
			pDC->SetFont(*pTgtFont);
			if (!pApp->m_bHidePunctuation && pApp->m_bSuppressLast)
			{
				int tgtWidth;
				int tgtDummyHeight;
				if (pSrcPhrase->m_targetStr.IsEmpty())
				{
					// no translation or retranslation, etc, so use pileWidth for extent
					tgtWidth = pPile->m_nWidth; 
				}
				else
				{
					// get the extent of the text
					pDC->GetTextExtent(pSrcPhrase->m_targetStr, &tgtWidth, &tgtDummyHeight);
				}

				// create the cell and set its attributes
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//		pSrcPhrase->m_targetStr,tgtWidth,pTgtFont,&color,&topLeft,&botRight,
				//		cellIndex); // BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_targetStr,tgtWidth,
									pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
			}
			else // either hiding punctuation or not suppressing the last target line
				// (cellIndex == 3)still
			{
				int aWidth;
				int aDummyHeight;
				if (pSrcPhrase->m_adaption.IsEmpty())
				{
					// no translation or retranslation, etc, so use pileWidth for extent
					aWidth = pPile->m_nWidth; // don't care about extent.cy
				}
				else
				{
					// get the extent of the text
					pDC->GetTextExtent(pSrcPhrase->m_adaption, &aWidth, &aDummyHeight);
				}

				// create the cell and set its attributes
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//		pSrcPhrase->m_adaption,aWidth,pTgtFont,&color,&topLeft,&botRight,
				//		cellIndex); // BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_adaption,aWidth,
								pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
			}

			// now the 5th line, provided it is not suppressed (it's a punctuated target
			// text line, if visible)
			if (!pApp->m_bSuppressLast)
			{
				// a fifth line is wanted, construct its cell
				cellIndex++; // cellIndex == 4

				// calculate the cell's topLeft and botRight points
				// whm: the wx version doesn't use negative offsets
				topLeft.y = topLeft.y+pApp->m_nTgtHeight; 
				botRight.y = botRight.y+pApp->m_nTgtHeight;

				// calculate the extent of the text -- but if there is no text, then make
				// the extent equal to the pileWidth (use m_adaption, so will be correct
				// for retranslations, etc)
				int tgtWidth;
				int tgtDummyHeight;
				if (pSrcPhrase->m_targetStr.IsEmpty())
				{
					// no translation or retranslation, etc, so use pileWidth for extent
					//extent.cx = pPile->m_nWidth; // don't care about extent.cy
					tgtWidth = pPile->m_nWidth; // don't care about extent.cy
				}
				else
				{
					// get the extent of the text
					pDC->GetTextExtent(pSrcPhrase->m_targetStr, &tgtWidth, &tgtDummyHeight);
				}

				// create the cell and set its attributes
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//		pSrcPhrase->m_targetStr,tgtWidth,pTgtFont,&color,&topLeft,
				//		&botRight,cellIndex); //BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_targetStr,tgtWidth,
								pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
			}
		}
	}
	else // the user is adapting (rather than glossing)
	{
		// continue to 3rd visible line (or 2nd visible line if m_bSuppressFirst is TRUE),
		// this is cell[2], and it will contain target text
		cellIndex++; // set cellIndex to 2

		// set the target language's text color
		color = pApp->m_targetColor;

		if (!gbShowTargetOnly)
		{
			// calculate the cell's topLeft and botRight points
			// whm: the wx version doesn't use negative offsets
			topLeft.y = topLeft.y+pApp->m_nSrcHeight;
			botRight.y = botRight.y+pApp->m_nTgtHeight;
		}
		else // we are showing only the target text
		{
			// calculate the cell's topLeft and botRight points (CText calculates the CRect),
			// as it is the only cell in the pile, for this mode
			topLeft = wxPoint(pRectPile->GetLeft(), pRectPile->GetTop());
			botRight = wxPoint(pRectPile->GetRight(), pRectPile->GetBottom());
			// whm: the wx version doesn't use negative offsets
			botRight.y = pRectPile->GetTop() + pApp->m_nTgtHeight;
		}

		// calculate the extent of the text -- but if there is no text, then make the extent
		// equal to the pileWidth (use m_adaption, so will be correct for retranslations, etc)
		// (if this cell is at the active location, displaying it will be suppressed externally)
		pDC->SetFont(*pTgtFont);
		if (!pApp->m_bHidePunctuation && pApp->m_bSuppressLast)
		{
			int tgtWidth;
			int tgtDummyHeight;
			if (pSrcPhrase->m_targetStr.IsEmpty())
			{
				// no translation or retranslation, etc, so use pileWidth for extent
				tgtWidth = pPile->m_nWidth; // don't care about extent.cy
			}
			else
			{
				// get the extent of the text
				pDC->GetTextExtent(pSrcPhrase->m_targetStr, &tgtWidth, &tgtDummyHeight);
			}

			// create the cell and set its attributes
			//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
			//		pSrcPhrase->m_targetStr,tgtWidth,pTgtFont,&color,&topLeft,&botRight,
			//		cellIndex); // BEW deprecated 3Feb09
			CCell* pCell = new CCell();
			pPile->m_pCell[cellIndex] = pCell;
			pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_targetStr,tgtWidth,
							pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
		}
		else // either hiding punctuation or not suppressing the last target line
		{
			int aWidth;
			int aDummyHeight;
			if (pSrcPhrase->m_adaption.IsEmpty())
			{
				// no translation or retranslation, etc, so use pileWidth for extent
				aWidth = pPile->m_nWidth; // don't care about extent.cy
			}
			else
			{
				// get the extent of the text
				pDC->GetTextExtent(pSrcPhrase->m_adaption, &aWidth, &aDummyHeight);
			}

			// create the cell and set its attributes
			//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
			//		pSrcPhrase->m_adaption,aWidth,pTgtFont,&color,&topLeft,&botRight,
			//		cellIndex); // BEW deprecated 3Feb09
			CCell* pCell = new CCell();
			pPile->m_pCell[cellIndex] = pCell;
			pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_adaption,aWidth,
							pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
		}

		// now the 4th line, provided it is not suppressed
		if (!pApp->m_bSuppressLast)
		{
			if (!gbShowTargetOnly)
			{
				// a fourth line is wanted, construct its cell
				cellIndex++; // cellIndex == 3

				// calculate the cell's topLeft and botRight points
				// whm: the wx version doesn't use negative offsets
				topLeft.y = topLeft.y+pApp->m_nTgtHeight;
				botRight.y = botRight.y+pApp->m_nTgtHeight;

				// calculate the extent of the text -- but if there is no text, then make
				// the extent equal to the pileWidth (use m_adaption, so will be correct
				// for retranslations, etc)
				int tgtWidth;
				int tgtDummyHeight;
				if (pSrcPhrase->m_targetStr.IsEmpty())
				{
					// no translation or retranslation, etc, so use pileWidth for extent
					tgtWidth = pPile->m_nWidth; // don't care about extent.cy
				}
				else
				{
					// get the extent of the text
					pDC->GetTextExtent(pSrcPhrase->m_targetStr, &tgtWidth, &tgtDummyHeight);
				}

				// create the cell and set its attributes
				//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
				//		pSrcPhrase->m_targetStr,tgtWidth,pTgtFont,&color,&topLeft,
				//		&botRight,cellIndex); // BEW deprecated 3Feb09
				CCell* pCell = new CCell();
				pPile->m_pCell[cellIndex] = pCell;
				pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_targetStr,tgtWidth,
								pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
			}
			else
			{
				cellIndex++; // cellIndex == 3, to keep indices right for any gloss line
			}

		}

		// not glossing, but if gbEnableGlossing is TRUE, then glosses will be visible
		// as the last line of the strip, so set up the 5th cell to contain the gloss
		if (gbEnableGlossing)
		{
			if (!gbShowTargetOnly) // don't calculate the cell if we are showing tgt only
			{
				// set the gloss text color, and if using navText's font for the glossing
				// then set up the device context to use it
				color = pApp->m_navTextColor; // glossing always uses navText's color
				if (gbGlossingUsesNavFont)
				{
					pDC->SetFont(*pNavTextFont);
				}

				// a gloss line is wanted, so construct its cell
				cellIndex++; // cellIndex == 3 (if one tgt line) or 4

				// calculate the cell's topLeft and botRight points; we will offset the
				// gloss line an extra 3 pixels from the line above it, since there appears
				// to be some unwanted encroachment for some fonts in some sizes
				if (gbGlossingUsesNavFont)
				{
					// whm: the wx version doesn't use negative offsets
					topLeft.y = topLeft.y+pApp->m_nTgtHeight + 3;
					botRight.y = botRight.y+pApp->m_nNavTextHeight + 3;
				}
				else
				{
					// whm: the wx version doesn't use negative offsets
					topLeft.y = topLeft.y+pApp->m_nTgtHeight + 3;
					botRight.y = botRight.y+pApp->m_nTgtHeight + 3;
				}

				// calculate the extent of the text -- but if there is no text, then make
				// the extent equal to the pileWidth (use m_adaption, so will be correct
				// for retranslations, etc)
				int glossWidth;
				int glossDummyHeight;
				if (pSrcPhrase->m_gloss.IsEmpty())
				{
					// no translation or retranslation, etc, so use pileWidth for extent
					glossWidth = pPile->m_nWidth; // don't care about extent.cy
				}
				else
				{
					// get the extent of the text
					pDC->GetTextExtent(pSrcPhrase->m_gloss, &glossWidth, &glossDummyHeight);
				}

				// create the cell and set its attributes; which font gets used will
				// depend on the gbGlossingUsesNavFont flag
				if (gbGlossingUsesNavFont)
				{
					//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
					//	pSrcPhrase->m_gloss,glossWidth,pNavTextFont,&color,&topLeft,
					//	&botRight,cellIndex); / BEW deprecated 3Feb09
					CCell* pCell = new CCell();
					pPile->m_pCell[cellIndex] = pCell;
					pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_gloss,glossWidth,
								pNavTextFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
				}
				else
				{
					//pPile->m_pCell[cellIndex] = CreateCell(pDoc,pBundle,pStrip,pPile,
					//	pSrcPhrase->m_gloss,glossWidth,pTgtFont,&color,&topLeft,
					//	&botRight,cellIndex); // BEW deprecated 3Feb09
					CCell* pCell = new CCell();
					pPile->m_pCell[cellIndex] = pCell;
					pCell->CreateCell(pBundle,pStrip,pPile,&pSrcPhrase->m_gloss,glossWidth,
								pTgtFont,&color,&topLeft,&botRight,cellIndex,&m_navTextColor);
				}
			}
		}
	}
	return pPile;
}


