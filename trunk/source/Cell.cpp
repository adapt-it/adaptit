/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Cell.cpp
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CCell class. 
/// The CCell class represents the next smaller division of a CPile, there
/// potentially being up to five CCells displaying vertically top to bottom
/// within a CPile. Each CCell stores a CText which is for the display of 
/// the cell's text, and changing background colour (for selections, 
/// highlighting) etc.
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
#include "Text.h"
#include "Cell.h"
#include "Pile.h"
#include "Strip.h"
#include "SourceBundle.h"
#include "Adapt_ItDoc.h"
#include "AdaptitConstants.h"
#include "SourcePhrase.h"
#include "Adapt_ItView.h"

// globals for support of vertical editing
wxColor gMidGray = wxColour(128,128,128); //COLORREF gMidGray = (COLORREF)RGB(128,128,128);
extern EditRecord gEditRecord;
extern bool gbVerticalEditInProgress;
extern EditStep gEditStep;
static EditRecord* pRec = &gEditRecord;

/// This global is defined in Adapt_It.cpp.
extern CPile* gpGreenWedgePile;

/// This global is defined in Adapt_It.cpp.
extern CPile* gpNotePile;

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

// This global is defined in Adapt_ItView.cpp.
//extern bool	gbEnableGlossing; // TRUE makes Adapt It revert to Shoebox functionality only

extern bool gbIsPrinting;

/// This global is defined in Adapt_ItView.cpp.
extern int gnBeginInsertionsSequNum;

/// This global is defined in Adapt_ItView.cpp.
extern int gnEndInsertionsSequNum;

extern bool	gbFindIsCurrent;

/// This global is defined in Adapt_ItView.cpp.
extern bool gbShowTargetOnly;

// whm NOTE: wxDC::DrawText(const wxString& text, wxCoord x, wxCoord y) does not have an equivalent
// to the nFormat parameter, but wxDC has a SetLayoutDirection(wxLayoutDirection dir) method
// to change the logical direction of the display context. In wxDC the display context is mirrored
// right-to-left when wxLayout_RightToLeft is passed as the parameter;
// While the MFC version changes the alignment and RTL reading direction of DrawText(), it is not
// the same as mirroring (in which MFC would actually call CDC::SetLayout(LAYOUT_RTL) to effect RTL
// mirroring in the display context. In wx, wxDC::DrawText() does not have a parameter that can 
// be used to control Right alignment and/or RTL Reading of text at that level of the DC.
// Certain controls such as wxTextCtrl and wxListBox, etc., also have an undocumented method called
// SetLayoutDirection(wxLayoutDirection dir), where dir is wxLayout_LeftToRight or wxLayout_RightToLeft. 
// Setting the layout to wxLayout_RightToLeft on these controls also involves some mirroring, so that 
// any scrollbar that gets displayed, for example, displays on the left rather than on the right, etc.
// In the wx version we have to be careful about the automatic mirroring features involved in the
// SetLayoutDirection() function, since Adapt It MFC was designed to micromanage the layout direction
// itself in the coding of text, cells, piles, strips, etc.

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // want to access it fast

extern const wxChar* filterMkr;
extern const wxChar* filteredTextPlaceHolder;
extern EditRecord gEditRecord;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(CCell, wxObject)


CCell::CCell()
{
	;
}

CCell::CCell(CAdapt_ItDoc* pDocument, CSourceBundle* pSourceBundle,
												CStrip* pStrip, CPile* pPile)
{
	m_pDoc = pDocument;
	m_pBundle = pSourceBundle;
	m_pStrip = pStrip;
	m_pPile = pPile;;
	m_pText = (CText*)NULL;
	m_bDisplay = TRUE;
	m_nCellIndex = 0;
	m_phrase = _T("");
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	m_navColor = pApp->m_navTextColor;
}

CCell::~CCell()
{

}

// BEW 2Aug08, additions to for gray colouring of context regions in vertical edit steps
void CCell::Draw(wxDC* pDC)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = pApp->GetView();
	wxASSERT(pView != NULL);

	pDC->DestroyClippingRegion(); // whm added 2May07 to try to resolve the paint issue on wxMac

	pDC->SetBrush(*wxTRANSPARENT_BRUSH); // SetBackgroundMode() requires a valid brush on wxGTK

	wxColour oldBkColor; 

	// vertical edit: change text colour to gray if it is before or after the editable span; we have to
	// do it also in the m_pText CText member too, and beware because the latter can be NULL for some CCell
	// instances.
	// whm: initialized the following two ints to 0 to avoid "potentially uninitialized local variable"
	// warnings, since they would be uninitialized when gbVerticalEditInProgress if FALSE (albeit the
	// present code, they are not used anywhere but where gbVerticalEditInProgress tests true.
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
	else if (gbVerticalEditInProgress && (gEditStep == freeTranslationsStep) && pRec->bFreeTranslationStepEntered)
	{
		// set up the span bounding indices
		nStart_Span = pRec->nFreeTranslationStep_StartingSequNum;
		nEnd_Span = pRec->nFreeTranslationStep_EndingSequNum;
	}
	// colour gray the appropriate cells' text
	if (gbVerticalEditInProgress 
		&& (gEditStep == adaptationsStep || gEditStep == glossesStep || gEditStep == freeTranslationsStep))
	{
		// the spans to be made available for work can differ in each step, so they are set above first
		if (m_pPile->m_pSrcPhrase->m_nSequNumber < nStart_Span || m_pPile->m_pSrcPhrase->m_nSequNumber > nEnd_Span)
		{
			// it's either adaptations step, AND, the pile is before or after the editable span
			if (m_pText != NULL)
			{
				m_pText->m_color = gMidGray; // this works because Draw() is not called on the 
											 // CText member until this present Draw() call ends
			}
		}
		else
		{
			// if its not one of those, just do the normal text colour as set by CreatePile() and CreateCell()
			// in the view class
			;
		}
	}
	// In all the remainder of this Draw() function, only backgrounds are ever changed in color, and the navigation
	// whiteboard area's icons and text are drawn.
	if (m_pText->m_bSelected)
	{
		oldBkColor = pDC->GetTextBackground(); // yellow
		pDC->SetBackgroundMode(pApp->m_backgroundMode);
		pDC->SetTextBackground(wxColour(255,255,0));
		//TRACE3("Sel'n Count %d  Pile %d  Strip %d\n",m_pDoc->GetApp()->GetView()->m_selection.GetCount(),
				//m_pPile->m_nPileIndex,m_pStrip->m_nStripIndex);

	}

	// BEW added 11Oct05 to have the top cell of the pile background coloured if the click was on
	// a green wedge or note icon
	if (m_nCellIndex == 1 && !gbIsPrinting && (m_pPile == gpGreenWedgePile || m_pPile == gpNotePile))
	{
		// hilight the top cell under the clicked green wedge or note, with light yellow
		pDC->SetBackgroundMode(pApp->m_backgroundMode);
		pDC->SetTextBackground(wxColour(255,255,100)); // light yellow
	}

	// Target Text Highlight 
	// Check for automatically inserted target text/glosses and highlight them
	// Usually only one target text/gloss line (Index 2) is showing, but 
	// highlight both target text lines when they are both showing.
	// BEW 12Jul05 add colouring for free translation sections, provided we are not currently printing
	// BEW 10Oct05 added a block for colouring the source line (cell index == 1) whenever there is no
	// target (or gloss) text that can be coloured, so that there will always be visual feedback about
	// what is free translated and what is about to be and what is not.
	if (m_nCellIndex == 1 && gpApp->m_bFreeTranslationMode && !gbIsPrinting 
		&& ((!gbIsGlossing && m_pPile->m_pSrcPhrase->m_targetStr.IsEmpty()) || 
		(gbIsGlossing && m_pPile->m_pSrcPhrase->m_gloss.IsEmpty())))
	{
		// we use pastel pink and green for the current section, and other defined sections,
		// respectively, and white (default) otherwise - ie. no free translation there
		if (m_pPile->m_pSrcPhrase->m_bHasFreeTrans && !m_pPile->m_bIsCurrentFreeTransSection)
		{
			// colour background pastel green
			pDC->SetBackgroundMode(pApp->m_backgroundMode);
			pDC->SetTextBackground(gpApp->m_freeTransDefaultBackgroundColor); // green
		}
		else if (m_pPile->m_bIsCurrentFreeTransSection)
		{
			// colour background pastel pink
			pDC->SetBackgroundMode(pApp->m_backgroundMode);
			pDC->SetTextBackground(gpApp->m_freeTransCurrentSectionBackgroundColor); // pink
		}
	}
	if (m_nCellIndex == 2)// active adapting/glossing line
	{
		if (gpApp->m_bFreeTranslationMode && !gbIsPrinting)
		{
			// we use pastel pink and green for the current section, and other defined sections,
			// respectively, and white (default) otherwise - ie. no free translation there
			if (m_pPile->m_pSrcPhrase->m_bHasFreeTrans && !m_pPile->m_bIsCurrentFreeTransSection)
			{
				// colour background pastel green
				pDC->SetBackgroundMode(pApp->m_backgroundMode);
				pDC->SetTextBackground(gpApp->m_freeTransDefaultBackgroundColor); // green
			}
			else if (m_pPile->m_bIsCurrentFreeTransSection)
			{
				// colour background pastel pink
				pDC->SetBackgroundMode(pApp->m_backgroundMode);
				pDC->SetTextBackground(gpApp->m_freeTransCurrentSectionBackgroundColor); // pink
			}
		}
		else
		{
			// not free translation mode
			if (!(gpApp->m_bSuppressTargetHighlighting) 
					&& m_pPile->m_pSrcPhrase->m_nSequNumber >= gnBeginInsertionsSequNum 
					&& m_pPile->m_pSrcPhrase->m_nSequNumber <= gnEndInsertionsSequNum)
			{
				//Draw automatically inserted target text with selected background color
				// Some decent possibilities for default background highlight color are:
				//pDC->SetBkColor(RGB(255,255,150)); // light yellow (COLORREF)9895935
				//pDC->SetBkColor(RGB(255,200,255)); // light purple (COLORREF)16763135
				//pDC->SetBkColor(RGB(200,255,255)); // light blue = (COLORREF)16777160
				// The user can choose any reasonable color to that he/she finds usable
				// A light purple background highlight looks pretty good over the 
				// default text colors and most that the user might choose
				pDC->SetBackgroundMode(pApp->m_backgroundMode);
				pDC->SetTextBackground(gpApp->m_AutoInsertionsHighlightColor); // light purple
			}
		}
	}

	if (m_nCellIndex != 2 ||
		m_pPile->m_pSrcPhrase->m_nSequNumber != gpApp->m_nActiveSequNum)
	{
		// draw every cell except where the phrase box is going to be displayed - that is
		// don't draw in the cell with m_nCellIndex == 2 at the active pile
		m_pText->Draw(pDC);
	}
	else
	{
		// but we will draw it if a "Find Next" is current (gbFindIsCurrent get cleared to FALSE
		// either with click in the view, or when the Find dialog's OnCancel() function is called)
		if (gbFindIsCurrent)
		{
			m_pText->Draw(pDC);
		}
	}
	pDC->SetBackgroundMode(gpApp->m_backgroundMode);
	pDC->SetTextBackground(wxColour(255,255,255)); // white

	bool bRTLLayout;
	bRTLLayout = FALSE;
	wxRect rectBounding;
	rectBounding.SetTop(m_ptTopLeft.y);
	rectBounding.SetLeft(m_ptTopLeft.x);
	rectBounding.SetBottom(m_ptBotRight.y);
	rectBounding.SetRight(m_ptBotRight.x);
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
	if (((m_nCellIndex == 0 && !gpApp->m_bSuppressFirst) ||
		 (m_nCellIndex == 1 && gpApp->m_bSuppressFirst)) && !gbShowTargetOnly )
	{
		int xOffset = 0;
		int diff;
		bool bHasFilterMarker = HasFilterMarker(m_pPile);

		// if a message is to be displayed above this word, draw it too
		if (m_pPile->m_pSrcPhrase->m_bNotInKB)
		{
			wxPoint pt;

			pt = m_ptTopLeft;
#ifdef _RTL_FLAGS
			if (gpApp->m_bNavTextRTL)
				pt.x += rectBounding.GetWidth(); // align right
#endif
			// whm: the wx version doesn't use negative offsets
			// Note: m_nNavTextHeight and m_nSrcHeight are now located on the App
			diff = pApp->m_nNavTextHeight - (pApp->m_nSrcHeight/4);
			pt.y -= diff;
			wxString str;
			xOffset += 8;
			if (m_pPile->m_pSrcPhrase->m_bBeginRetranslation)
				str = _T("*# "); // visibly mark start of a retranslation section
			else
				str = _T("* ");
			wxFont SaveFont;
			wxFont* pNavTextFont = gpApp->m_pNavTextFont;
			SaveFont = pDC->GetFont(); // save current font
			pDC->SetFont(*pNavTextFont);
			if (!m_navColor.IsOk())
			{
				::wxBell(); 
				wxASSERT(FALSE);
			}
			pDC->SetTextForeground(m_navColor);

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
		if (m_pPile->m_pSrcPhrase->m_bFirstOfType || m_pPile->m_pSrcPhrase->m_bVerse
			|| m_pPile->m_pSrcPhrase->m_bChapter || m_pPile->m_pSrcPhrase->m_bParagraph
			|| m_pPile->m_pSrcPhrase->m_bFootnoteEnd || m_pPile->m_pSrcPhrase->m_bHasInternalMarkers
			|| m_pPile->m_pSrcPhrase->m_markers.Find(filterMkr) != -1)
		{
			wxPoint pt;
			pt = m_ptTopLeft;

			// BEW added, for support of filter wedge icon
			if (bHasFilterMarker && !gbShowTargetOnly)
			{
				xOffset = 7;  // offset any nav text 7 pixels to the right (or to the left if RTL rendering)
								// to make room for the wedge
			}

#ifdef _RTL_FLAGS
			if (gpApp->m_bRTL_Layout) // was gpApp->m_bNavTextRTL
				pt.x += rectBounding.GetWidth(); // align right
#endif
			// whm: the wx version doesn't use negative offsets
			// Note: m_nNavTextHeight and m_nSrcHeight are now located on the App
			diff = pApp->m_nNavTextHeight + 
								(pApp->m_nSrcHeight/4) + 1;
			pt.y -= diff;
#ifdef _RTL_FLAGS
			if (gpApp->m_bRTL_Layout) // was gpApp->m_bNavTextRTL
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
				wxPoint ptWedge = m_ptTopLeft;

				// BEW added 18Nov05, to colour the wedge differently if \free is contentless (as khaki), or if
				// \bt is contentless (as pastel blue), or if both are contentless (as red)
				bool bFreeHasNoContent = m_pBundle->m_pView->IsFreeTranslationContentEmpty(m_pPile->m_pSrcPhrase);
				bool bBackHasNoContent = m_pBundle->m_pView->IsBackTranslationContentEmpty(m_pPile->m_pSrcPhrase);

				#ifdef _RTL_FLAGS
				if (gpApp->m_bRTL_Layout)
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
			if (m_pPile->m_pSrcPhrase->m_bVerse || m_pPile->m_pSrcPhrase->m_bChapter)
						{
				str = m_pPile->m_pSrcPhrase->m_chapterVerse;
			}

			// now append anything which is in the m_inform member; there may not have been a chapter
			// and/or verse number already placed in str, so allow for this possibility
			if (!m_pPile->m_pSrcPhrase->m_inform.IsEmpty())
			{
				if (str.IsEmpty())
					str = m_pPile->m_pSrcPhrase->m_inform;
				else
				{
					str += _T(' ');
					str += m_pPile->m_pSrcPhrase->m_inform;
				}
			}

			wxFont pSaveFont;
			wxFont* pNavTextFont = gpApp->m_pNavTextFont;
			pSaveFont = pDC->GetFont();
			pDC->SetFont(*pNavTextFont);
			if (!m_navColor.IsOk())
			{
				::wxBell(); 
				wxASSERT(FALSE);
			}
			pDC->SetTextForeground(m_navColor);

			// BEW modified 25Nov05 to move the start of navText to just after the green wedge when
			// filtered info is present, because for fonts with not much internal leading built
			// in, the nav text overlaps the top few pixels of the wedge

			if (bHasFilterMarker)
			{
#ifdef _RTL_FLAGS
				// we need to make extra space available for some data configurations
				if (gpApp->m_bRTL_Layout)
				{
					// right to left layout
					if (gpApp->m_bNavTextRTL)
						rectBounding.Offset(-xOffset,-diff); // navText is RTL
					else
						rectBounding.Offset(0,-diff); // navText is LTR
				}
				else
				{
					// left to right layout
					if (gpApp->m_bNavTextRTL)
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
		if (m_pPile->m_pSrcPhrase->m_bHasNote)
		{
			wxPoint ptNote = m_ptTopLeft;
			// offset top left (-13,-9) for regular app
			#ifdef _RTL_FLAGS
			if (gpApp->m_bRTL_Layout)
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

bool CCell::HasFilterMarker(CPile* pPile)
{
	return pPile->m_pSrcPhrase->m_markers.Find(filterMkr) >= 0;
}
