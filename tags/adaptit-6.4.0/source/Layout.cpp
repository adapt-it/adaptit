// Deprecation List..... get rid of these later....
/*
gnSaveGap and gnSaveLeading are replaced by m_nSaveGap and m_nSaveLeading in CLayout
but the view still uses them at the moment, so I've not removed them yet

m_bSaveAsXML on app, not needed in WX version -- but not removed yet,
it's used in a lot of tests and removing it will take a bit of careful work
gbBundleChanged  defined in CAdapt_ItView.cpp

*/

//#define _OFFSETS_BUG

// for debugging support
// comment out the following when the document does not have 13 strips
// (more can be handled, up to 26 - but only data for 13, and in a few places, some more,
// will be shown by wxLogDebug) but if extras are added beware, over 26 and the app will
// hang when the debugging blocks try to read or write beyond the 26th element)
//#define _13STRIPS_DOC

/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Layout.cpp
/// \author			Bruce Waters
/// \date_created	09 February 2009
/// \rcs_id $Id$
/// \copyright		2009 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public
///                 License (see license directory)
/// \description This is the implementation file for the CLayout class. The CLayout class replaces
/// the legacy CSourceBundle class, and it encapsulated as much as possible of the visible layout
/// code for the document. It manages persistent CPile objects for the whole document - one per
/// CSourcePhrase instance. Copies of the CPile pointers are stored in CStrip objects. CStrip,
/// CPile and CCell classes are used in the refactored layout, but with a revised attribute
/// inventory; and wxPoint and wxRect members are computed on the fly as needed using functions,
/// which reduces the layout computations when the user does things by a considerable amount.
/// \derivation The CLayout class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Layout.h"
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
#include "Adapt_ItDoc.h"
#include "AdaptitConstants.h"
#include "SourcePhrase.h"
#include "MainFrm.h"
#include "helpers.h"
// don't mess with the order of the following includes, Strip must precede View must precede
// Pile must precede Layout and Cell can usefully by last
#include "Strip.h"
#include "Adapt_ItView.h"
#include "Pile.h"
#include "Cell.h"
#include "Adapt_ItCanvas.h"
#include "Layout.h"
#include "FreeTrans.h"

extern bool gbCheckInclFreeTransText;

/// This global is defined in Adapt_It.cpp. (default is FALSE)
extern bool gbDoingInitialSetup;

// for support of auto-capitalization

/// This global is defined in Adapt_It.cpp.
extern bool	gbAutoCaps;

/// This global is defined in Adapt_It.cpp.
extern bool	gbSourceIsUpperCase;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNonSourceIsUpperCase;

/// This global is defined in Adapt_It.cpp.
extern bool	gbMatchedKB_UCentry;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNoSourceCaseEquivalents;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNoTargetCaseEquivalents;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNoGlossCaseEquivalents;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharNonSrcLC;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharNonSrcUC;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharSrcLC;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharSrcUC;

/// This global is defined in Adapt_It.cpp.
extern bool gbSuppressSetup;

// globals for support of vertical editing

/// This global is defined in Adapt_It.cpp.
extern EditRecord gEditRecord;

/// This global is defined in Adapt_ItView.cpp.
extern bool gbVerticalEditInProgress;

/// This global is defined in Adapt_ItView.cpp.
extern EditStep gEditStep;

/// This global is defined in Adapt_It.cpp.
extern CPile* gpGreenWedgePile;

/// This global is defined in Adapt_It.cpp.
extern CPile* gpNotePile;

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text
extern bool gbGlossingUsesNavFont;

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbGlossingVisible; // TRUE makes Adapt It revert to Shoebox functionality only

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbFindIsCurrent;

/// This global is defined in Adapt_ItView.cpp.
extern bool gbShowTargetOnly;

/// This global is defined in Adapt_ItView.cpp.
extern wxRect grectViewClient;

/// This global is defined in Adapt_ItView.h.
extern int gnBoxCursorOffset;

/// This global is defined in Adapt_ItView.h.
extern bool gbCheckInclFreeTransText;

/// This global is defined in Adapt_ItView.h.
extern bool gbCheckInclGlossesText;

//GDLC removed 2010-02-09
// This global is defined in PhraseBox.cpp
//extern bool gbExpanding;

//GDLC removed 2010-02-09
// This global is defined in PhraseBox.cpp
//extern bool	gbContracting;

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
extern CAdapt_ItApp* gpApp; // want to access it fast

extern const wxChar* filterMkr;
extern const wxChar* filteredTextPlaceHolder;

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


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(CLayout, wxObject)


CLayout::CLayout()
{
	m_stripArray.Clear();
}

CLayout::~CLayout()
{

}

// call InitializeCLayout when the application has the view, canvas, and document classes
// initialized -- we set up pointers to them here so we want them to exist first --we'll
// get a message (and an assert in debug mode) if we call this too early
void CLayout::InitializeCLayout()
{
	// set the pointer members for the classes the layout has to be able to access on demand
	m_pApp = GetApp();
	m_pView = GetView();
	m_pCanvas = GetCanvas();
	m_pDoc = GetDoc();
	m_pMainFrame = GetMainFrame(m_pApp);
	m_stripArray.Clear();
	m_invalidStripArray.Clear();
	m_docEditOperationType = invalid_op_enum_value;
	m_bLayoutWithoutVisiblePhraseBox = FALSE;
	m_pOffsets = NULL;
	m_pSavePileList = NULL;
	m_bInhibitDraw = FALSE;
#ifdef Do_Clipping
	m_bScrolling = FALSE; // TRUE when scrolling is happening
	m_bDoFullWindowDraw = FALSE;
#endif
    // can add more basic initializations above here - but only stuff that makes
    // the session-persistent m_pLayout pointer on the app class have the basic info it
    // needs, other document-related initializations can be done in SetupLayout()
}

// for setting or clearing the m_bLayoutWithoutVisiblePhraseBox boolean
void CLayout::SetBoxInvisibleWhenLayoutIsDrawn(bool bMakeInvisible)
{
	m_bLayoutWithoutVisiblePhraseBox = bMakeInvisible;
}
#ifdef Do_Clipping
void CLayout::SetScrollingFlag(bool bIsScrolling)
{
	if (bIsScrolling)
		m_bScrolling = TRUE;
	else
		m_bScrolling = FALSE;
}

bool CLayout::GetScrollingFlag()
{
	return m_bScrolling;
}
#endif

void CLayout::Draw(wxDC* pDC)
{
	if (m_bInhibitDraw)
	{
        // BEW added 29Jul09, idea from Graeme, don't attempt any drawing if the view is
        // not in a consistent state that would allow it
		return;
	}

	// a last minute enforcment of RTL layout doesn't fix the Macaulay quotes & comma Arabic
	// puntuation bug which happens only on the Windows build, so comment it out
//#ifdef __WXMSW__
//#ifdef _RTL_FLAGS
//	if (m_pApp->m_bSrcRTL || m_pApp->m_bTgtRTL || m_pApp->m_bNavTextRTL)
//	{
//		pDC->SetLayoutDirection(wxLayout_RightToLeft);
//	}
//#endif
//#endif

    // BEW 23Jun09 - tried moving the placement of the phrase box to Invalidate() so as to
    // support clipping, but that was too early in the flow of events, and the box was not
    // uptodate and the last character typed was not "seen", so I had to move it back here.
	// Now I'll try a m_bDoFullWindowDraw flag set when Redraw() or RecalcLayout() is
	// called - yes, that turned out to be the way to do it! See CAdapt_ItView::Invalidate()
	// BEW 30Jun09 it turns out that handling the phrase box here generates a paint event
	// from within Draw() which then results in Draw() going into an infinite loop - so
	// I'm moving the PlacePhraseBoxInLayout() call and the test which precedes it out of
	// here - probably into the end of Invalidate(), and the end of Redraw() too.
#ifdef Do_Clipping
	// temporary code for debugging
	//wxSize sizePhraseBox = m_pApp->m_pTargetBox->GetClientSize(); //  pixels
	//wxLogDebug(_T("Draw() START: bFullWindowDraw is %s  and m_nCurBoxWidth  %d  and currBoxSize.x  %d"),
	//			m_bDoFullWindowDraw ? _T("TRUE") : _T("FALSE"),
	//			m_curBoxWidth, sizePhraseBox.x);
#endif
    // drawing is done based on the top of the first strip of a visible range of strips
    // determined by the scroll car position; to have drawing include the phrase box, a
    // caller has to set the active location first --typically its done by code in the
    // AdjustForUserEdits() function called within RecalcLayout(), but if rebuilding the
    // full inventory of strips then it is based on the active strip as defined by the
    // active sequence number's value -- and the idea is to set the active strip somewhere
    // in the visible area and use ScrollIntoView() to ensure the scroll car is set to the
    // appropriate value before any drawning is done; for ScrollUp() and ScrollDown() the
    // function handlers of these scroll operations ignore the active location and just
    // define a client area's worth of drawing based on the scroll car position
 	int i;
	int nFirstStripIndex = -1;
	int nLastStripIndex = -1;
	//int nActiveSequNum = -1; // set but unused

	// work out the range of visible strips based on the phrase box location
	//nActiveSequNum = m_pApp->m_nActiveSequNum;

	// determine which strips are to be drawn  (a scrolled wxDC must be passed in)
	// BEW added 10Jul09, GetVisibleStripsRange() assumes drawing is being done to the
	// client rectangle of the view, and this is not the case when printing or print
	// previewing, so code is added to access the current PageOffsets struct when printing
	// is in effect, to work out the first and last strip and the number of strips to be
	// drawn for the current printed page or previewed page
	if (m_pApp->m_bIsPrinting)
	{
		if (m_pOffsets != NULL)
		{
			// printing, or print previewing is currently in effect
			nFirstStripIndex = m_pOffsets->nFirstStrip;
			nLastStripIndex = m_pOffsets->nLastStrip;
		}
		else
		{
			// give some 'safe' values - first strip only
			nFirstStripIndex = 0;
			nLastStripIndex = 0;
		}
	}
	else
	{
		// not printing nor print previewing
		GetVisibleStripsRange(pDC, nFirstStripIndex, nLastStripIndex);
	}
	// check for any invalid strips in the range to be drawn, and if one is found, call
	// the CleanUpFromStripAt() function, doing it for a window's worth of strips, so that
	// no invalid strip will be visible to the user - either when displaying document
	// editing results, or when scrolling up or down
	//
	// BEW added 10Jul09, since RecalcLayout() with param create_strips_keep_piles is
	// called before drawing when doing print, or print preview, there won't be invalid
	// strips in existence, and so we don't need to do this block when gbIsPrinting is
	// TRUE
	if (!m_pApp->m_bIsPrinting)
	{
		CStrip* aStripPtr = NULL;
		for (i = nFirstStripIndex; i <=  nLastStripIndex; i++)
		{
			aStripPtr = (CStrip*)m_stripArray.Item(i);
			if (aStripPtr->m_bValid == FALSE)
			{
				// fix a window's worth of strips from here on - do it only once per loop
				CleanUpTheLayoutFromStripAt(aStripPtr->m_nStrip, GetNumVisibleStrips());
				break;
			}
		}
	}

	// in case the count of the inventory of strips has changed because a strip was
	// cleared and removed, check for this possibility and reset the nLastStripIndex if it
	// no longer references an element in the array
	int newLastIndex = 0;
	if (!m_pApp->m_bIsPrinting)
	{
		newLastIndex = m_stripArray.GetCount() - 1;
		if (nLastStripIndex > newLastIndex)
		{
			// NECESSARY! Here's how to induce a crash if this test and resetting of
			// nLastStripIndex was not done. Have several words of an unadapted phrase to be
			// merged at the end of the document, in such a way that a couple of the source
			// words are at the end of the penultimate strip and the final few words are at the
			// beginning of the final strip - and the final strip should have not a full
			// inventory of piles - but few enough for the new merger to later fit within it.
			// Get the box at the first of the words, make the merger. The app will make the
			// merger, it will be too long for the penultimate strip and be thrown down to an
			// inserted strip as the latter's only pile (and the strip count increases by one)
			// but the pile flow up mechanism results in the last strip's piles flowing up, and
			// they all fit, so the last strip becomes empty and gets removed. The
			// nLastStripIndex set externally to that process then references a strip which no
			// longer exists, and the app would crash when the m_stripArray is asked for a
			// CStrip* which now no longer exists. The following line fixes the problem if ever
			// it arises.
			nLastStripIndex = newLastIndex;
		}
	}

	// draw the visible strips (includes an extra one, where possible)
	for (i = nFirstStripIndex; i <=  nLastStripIndex; i++)
	{
		((CStrip*)m_stripArray.Item(i))->Draw(pDC);
	}

	m_invalidStripArray.Clear(); // initialize for next user edit operation

#ifdef Do_Clipping
	//wxLogDebug(_T("Strips Drawn: bScrolling is %s  bFullWindowDraw is %s and the latter is now about to be cleared to default FALSE"),
	//			m_bScrolling ? _T("TRUE") : _T("FALSE"),
	//			m_bDoFullWindowDraw ? _T("TRUE") : _T("FALSE") );
    // initialize the clipping support flags, and clear the clip rectangle in both the
    // device context, and the one for the active strip in CLayout
	SetScrollingFlag(FALSE);
	SetFullWindowDrawFlag(FALSE);
	pDC->DestroyClippingRegion(); // makes it default to full-window drawing again
#else
	pDC->DestroyClippingRegion(); // only full-window drawing
#endif
	// BEW added 1Jul09, to support suppressing multiple calls of MakeTargetStringIncludingPunctuation()
	// (and therefore the potential for multiple shows of the placement dialog for
	// medial punctuation) at a single active location
	m_pApp->m_nPlacePunctDlgCallNumber = 0; // clear to default value of zero
	m_pApp->m_nCurSequNum_ForPlacementDialog = -1; // reset to default -1 "undefined" value
}

// the Redraw() member function can be used in many places where, in the legacy application,
// the document is unchanged but the layout needs repainting (eg. a window temporarily covered
// part of the canvas/view); the legacy app just called RecalcLayout() to recreate the bundle, but
// now that bundles are removed, calling CLayout::RecalcLayout() is potentially too costly,
// and unnecessary (since the strips piles and cells are okay already); so we just need to redraw
// (we could call Invalidate() on the view, but with Redraw() we potentially have more control)
// bFirstClear is default TRUE; if TRUE it causes aDC to paint the client area with background
// colour (white); assumes the redraw is to be based on an unchanged active location
void CLayout::Redraw(bool bFirstClear)
{
	wxClientDC aDC((wxWindow*)m_pCanvas); // make a device context
	m_pCanvas->DoPrepareDC(aDC); // get origin adjusted (calls wxScrolledWindow::DoPrepareDC)
	wxClientDC* pDC = &aDC;
	pDC->DestroyClippingRegion();
	if (bFirstClear)
	{
		// erase the view rectangle
		pDC->Clear();
	}
	Draw(pDC);  // the CLayout::Draw() which first works out which strips need to be drawn
				// based on the active location (default param bool bAtActiveLocation is TRUE)

	SetFullWindowDrawFlag(TRUE);
}

CAdapt_ItApp* CLayout::GetApp()
{
	CAdapt_ItApp* pApp = &wxGetApp();
	if (pApp == NULL)
	{
		wxMessageBox(_T("Error: failed to get m_pApp pointer in CLayout"),_T(""),
		wxICON_ERROR | wxOK);
		wxASSERT(FALSE);
	}
	return pApp;
}

CAdapt_ItView* CLayout::GetView()
{
	CAdapt_ItView* pView = GetApp()->GetView();
	if (pView == NULL)
	{
		wxMessageBox(_T("Error: failed to get m_pView pointer in CLayout"),_T(""),
		wxICON_ERROR | wxOK);
		wxASSERT(FALSE);
	}
	return pView;
}

CAdapt_ItCanvas* CLayout::GetCanvas()
{
	CMainFrame* pFrame = GetApp()->GetMainFrame();
	CAdapt_ItCanvas* pCanvas = pFrame->canvas;
	if (pCanvas == NULL)
	{
		wxMessageBox(_T("Error: failed to get m_pCanvas pointer in CLayout"),_T(""),
		wxICON_ERROR | wxOK);
		wxASSERT(FALSE);
	}
	return pCanvas;
}

CAdapt_ItDoc* CLayout::GetDoc()
{
	CAdapt_ItDoc* pDoc = GetView()->GetDocument();
	if (pDoc == NULL)
	{
		wxMessageBox(_T("Error: failed to get m_pDoc pointer in CLayout"),_T(""),
		wxICON_ERROR | wxOK);
		wxASSERT(FALSE);
	}
	return pDoc;
}

CMainFrame*	CLayout::GetMainFrame(CAdapt_ItApp* pApp)
{
	CMainFrame* pFrame = pApp->GetMainFrame();
	if (pFrame == NULL)
	{
		wxMessageBox(_T("Error: failed to get m_pMainFrame pointer in CLayout"),
		_T(""), wxICON_ERROR | wxOK);
		wxASSERT(FALSE);
	}
	return pFrame;
}

#ifdef Do_Clipping
// Clipping support
void CLayout::SetFullWindowDrawFlag(bool bFullWndDraw)
{
	if (bFullWndDraw)
		m_bDoFullWindowDraw = TRUE;
	else
		m_bDoFullWindowDraw = FALSE;
}

bool CLayout::GetFullWindowDrawFlag()
{
	return m_bDoFullWindowDraw;
}
#endif

// BEW 22Jun10, no changes needed for support of kbVersion 2
void CLayout::PlaceBox()
{
    // BEW 30Jun09, moved PlacePhraseBoxInLayout() to here, to avoid generating a paint
	// event from within Draw() which lead to an infinite loop; we need to call PlaceBox()
	// after Invalidate() calls, and after Redraw() calls

	// get the phrase box placed in the active location and made visible, and suitably
	// prepared - unless it should not be made visible (eg. when updating the layout
	// in the middle of a procedure, before the final update is done at a later time)
	//if (!pLayout->GetBoxVisibilityFlag())
	if (!m_bLayoutWithoutVisiblePhraseBox)
	{
		int nActiveSequNum = m_pApp->m_nActiveSequNum;

		// work out its location and resize (if necessary) and draw it
		if (nActiveSequNum == -1)
		{
			return; // do no phrase box placement if it is hidden, as at doc end
		}
		bool bSetModify = FALSE; // initialize, governs what is done with the wxEdit
								 // control's dirty flag
		bool bSetTextColor = FALSE; // initialize, governs whether or not we reset
									// the box's text colour

		// obtain the TopLeft coordinate of the active pile's m_pCell[1] cell, there
		// the phrase box is to be located
		wxPoint ptPhraseBoxTopLeft;
		CPile* pActivePile = GetPile(nActiveSequNum); // could use view's m_pActivePile
								// instead; but this will work even if we have forgotten to
								// update it in the edit operation's handler
		pActivePile->GetCell(1)->TopLeft(ptPhraseBoxTopLeft);

		// get the pile width at the active location, using the value in
		// CLayout::m_curBoxWidth put there by RecalcLayout() or AdjustForUserEdits() or FixBox()
		int phraseBoxWidth = pActivePile->GetPhraseBoxGapWidth(); // returns CPile::m_nWidth
		//int phraseBoxWidth = m_curBoxWidth; // I was going to use this, but I think the best
		//design is to only use the value stored in CLayout::m_curBoxWidth for the brief
		//interval within the execution of FixBox() when a box expansion happens (and
		//immediately after a call to RecalcLayout() or later to AdjustForUserEdits()), since
		//then the CalcPhraseBoxGapWidth() call in RecalcLayout() or in AdjustForUserEdits()
		//will use the phraseBoxWidthAdjustMode parameter passed to it to test for largest of
		//m_curBoxWidth and a value based on text extent plus slop, and use the larger -
		//setting result in m_nWidth, so the ResizeBox() calls here in PlacePhraseBoxInLayout()
		//should always expect m_nWidth for the active pile will have been correctly set, and
		//so always use that for the width to pass in to the ResizeBox() call below.

		// Note: the m_nStartChar and m_nEndChar app members, for cursor placement or text selection
		// range specification get set by the SetupCursorGlobals() calls in the switch below

		// handle any operation specific parameter settings
		// this stuff may not be needed now that I've had to put PlaceBox() all over the app
		// BEW 21Jul09, I commented out currently unused parts of the switch, if we later
		// want to use any of those parts, we can just restore the wanted part below, the
		// enum values are not changed
		enum doc_edit_op opType = m_docEditOperationType;
		switch(opType)
		{
		case default_op:
			{
				SetupCursorGlobals(m_pApp->m_targetPhrase, select_all); // sets to (-1,-1)
				bSetModify = TRUE;
				break;
			}
/*		case cancel_op:
			{

				break;
			}
*/		case char_typed_op:
			{
				// don't interfere with the m_nStartChar and m_nEndChar values, just set
				// modify flag
				bSetModify = TRUE;
				break;
			}
		case target_box_paste_op:
			{
				SetupCursorGlobals(m_pApp->m_targetPhrase, cursor_at_offset, gnBoxCursorOffset);
				bSetModify = FALSE;
				break;
			}
		case relocate_box_op:
			{
				bSetModify = FALSE;
				bSetTextColor = TRUE;
				break;
			}
/*		case merge_op:
			{

				break;
			}
		case unmerge_op:
			{

				break;
			}
*/		case retranslate_op:
			{
				SetupCursorGlobals(m_pApp->m_targetPhrase, select_all); // sets to (-1,-1)
				bSetModify = FALSE;
				break;
			}
		case remove_retranslation_op:
			{
				SetupCursorGlobals(m_pApp->m_targetPhrase, select_all); // sets to (-1,-1)
				bSetModify = FALSE;
				break;
			}
		case edit_retranslation_op:
			{
				SetupCursorGlobals(m_pApp->m_targetPhrase, select_all); // sets to (-1,-1)
				bSetModify = FALSE;
				break;
			}
/*		case insert_placeholder_op:
			{

				break;
			}
*/		case remove_placeholder_op:
			{
				SetupCursorGlobals(m_pApp->m_targetPhrase, select_all); // sets to (-1,-1)
				break;
			}
		case consistency_check_op:
			{
				m_pView->RemoveSelection();
				break;
			}
/*		case split_op:
			{

				break;
			}
		case join_op:
			{

				break;
			}
		case move_op:
			{

				break;
			}
*/		case on_button_no_adaptation_op:
			{
				m_pApp->m_nStartChar = 0;
				m_pApp->m_nEndChar = 0;
				bSetModify = TRUE;
				break;
			}
/*		case edit_source_text_op:
			{

				break;
			}
		case free_trans_op:
			{

				break;
			}
		case end_free_trans_op:
			{

				break;
			}
*/		case retokenize_text_op:
			{
				SetupCursorGlobals(m_pApp->m_targetPhrase, cursor_at_text_end);
				bSetModify = FALSE;
				break;
			}
/*		case collect_back_translations_op:
			{

				break;
			}
		case vert_edit_enter_adaptions_op:
			{

				break;
			}
		case vert_edit_exit_adaptions_op:
			{

				break;
			}
		case vert_edit_enter_glosses_op:
			{

				break;
			}
		case vert_edit_exit_glosses_op:
			{

				break;
			}
		case vert_edit_enter_free_trans_op:
			{

				break;
			}
		case vert_edit_exit_free_trans_op:
			{

				break;
			}
		case vert_edit_cancel_op:
			{

				break;
			}
		case vert_edit_end_now_op:
			{

				break;
			}
		case vert_edit_previous_step_op:
			{

				break;
			}
*/		case vert_edit_exit_op:
			{
				SetupCursorGlobals(m_pApp->m_targetPhrase, select_all); // sets to (-1,-1)
				bSetModify = FALSE;
				break;
			}
		case vert_edit_bailout_op:
			{
				SetupCursorGlobals(m_pApp->m_targetPhrase, select_all); // sets to (-1,-1)
				bSetModify = FALSE;
				break;
			}
/*		case exit_preferences_op:
			{

				break;
			}
		case change_punctuation_op:
			{

				break;
			}
		case change_filtered_markers_only_op:
			{

				break;
			}
		case change_sfm_set_only_op:
			{

				break;
			}
		case change_sfm_set_and_filtered_markers_op:
			{

				break;
			}
		case open_document_op:
			{

				break;
			}
		case new_document_op:
			{

				break;
			}
		case close_document_op:
			{

				break;
			}
		case enter_LTR_layout_op:
			{

				break;
			}
		case enter_RTL_layout_op:
			{

				break;
			}
*/		default: // do the same as default_op
		case no_edit_op:
			{
				// do nothing additional
				break;
			}
		}
		//*/
		// reset m_docEditOperationType to an invalid value, so that if not explicitly set by
		// the user's editing operation, or programmatic operation, the default: case will
		// fall through to the no_edit_op case, which does nothing
		m_docEditOperationType = invalid_op_enum_value; // an invalid value

		// wx Note: we don't destroy the target box, just set its text to null
		m_pApp->m_pTargetBox->ChangeValue(_T(""));

		// make the phrase box size adjustments, set the colour of its text, tell it where it
		// is to be drawn. ResizeBox doesn't recreate the box; it just calls SetSize() and
		// causes it to be visible again; CPhraseBox has a color variable & uses reflected
		// notification
		if (gbIsGlossing && gbGlossingUsesNavFont)
		{
			m_pView->ResizeBox(&ptPhraseBoxTopLeft, phraseBoxWidth, GetNavTextHeight(),
						m_pApp->m_targetPhrase, m_pApp->m_nStartChar, m_pApp->m_nEndChar,
						pActivePile);
			m_pApp->m_pTargetBox->m_textColor = GetNavTextColor(); //was pApp->m_navTextColor;
		}
		else
		{
			m_pView->ResizeBox(&ptPhraseBoxTopLeft, phraseBoxWidth, GetTgtTextHeight(),
						m_pApp->m_targetPhrase, m_pApp->m_nStartChar, m_pApp->m_nEndChar,
						pActivePile);
			m_pApp->m_pTargetBox->m_textColor = GetTgtColor(); // was pApp->m_targetColor;
		}

		// set the color - CPhraseBox has a color variable & uses reflected notification
		if (bSetTextColor)
		{
			if (gbIsGlossing && gbGlossingUsesNavFont)
				m_pApp->m_pTargetBox->m_textColor = GetNavTextColor();
			else
				m_pApp->m_pTargetBox->m_textColor = GetTgtColor();
		}

		// whm added 20Nov10 setting of target box background color for when the Guesser
		// has provided a guess. Default m_GuessHighlightColor color is orange.
		// BEW 13Oct11, added text for m_bFreeTranslationMode so as to get the pink
		// background in the phrase box when it is at an anchor location
		if (m_pApp->m_bIsGuess || m_pApp->m_bFreeTranslationMode)
		{
			if (m_pApp->m_bFreeTranslationMode && m_pApp->m_nActiveSequNum != -1)
			{
				m_pApp->m_pTargetBox->SetBackgroundColour(m_pApp->m_freeTransCurrentSectionBackgroundColor);
			}
			else
			{
			m_pApp->m_pTargetBox->SetBackgroundColour(m_pApp->m_GuessHighlightColor);
			// Note: PlaceBox() is called twice in the process of executing PhraseBox's
			// OnePass() function (one via a MoveToNextPile call and once later in OnePass.
			// If we reset the m_pApp->m_bIsGuess flag to FALSE here in PlaceBox()
			// the second call of PlaceBox() from OnePass will reset the background color
			// to white in the else block below because the else block below would then
			// be exectuted on the second call. Instead of resetting m_bIsGuess here,
			// I've reset it at the end of the OnePass() function.
			//m_pApp->m_bIsGuess = FALSE;
			}
		}
		else
		{
			// normal background color in target box is white
			m_pApp->m_pTargetBox->SetBackgroundColour(wxColour(255,255,255)); // white
		}
		// handle the dirty flag
		if (bSetModify)
		{
			// calls our own SetModify(TRUE)(see Phrasebox.cpp)
			m_pApp->m_pTargetBox->SetModify(TRUE);
		}
		else
		{
			// call our own SetModify(FALSE) which calls DiscardEdits() (see Phrasebox.cpp)
			m_pApp->m_pTargetBox->SetModify(FALSE);
		}

		// put focus in compose bar's edit box if in free translation mode
		if (m_pApp->m_bFreeTranslationMode)
		{
			CMainFrame* pFrame = m_pApp->GetMainFrame();
			wxASSERT(pFrame != NULL);
			if (pFrame->m_pComposeBar != NULL)
				if (pFrame->m_pComposeBar->IsShown())
				{
					wxTextCtrl* pComposeBox = (wxTextCtrl*)
								pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
					wxString text;
					text = pComposeBox->GetValue();
					int len = text.Length();
					pComposeBox->SetSelection(len,len);
					pComposeBox->SetFocus();
				}
		}
	}
	m_bLayoutWithoutVisiblePhraseBox = FALSE; // restore default
}

bool CLayout::GetBoxVisibilityFlag()
{
	return m_bLayoutWithoutVisiblePhraseBox;
}

// accessors for font pointers
void CLayout::SetSrcFont(CAdapt_ItApp* pApp)
{
	m_pSrcFont = pApp->m_pSourceFont;
}

void CLayout::SetTgtFont(CAdapt_ItApp* pApp)
{
	m_pTgtFont = pApp->m_pTargetFont;
}

void CLayout::SetNavTextFont(CAdapt_ItApp* pApp)
{
	m_pNavTextFont = pApp->m_pNavTextFont;
}

// accessors and getters for src, tgt & navText colours
void CLayout::SetSrcColor(CAdapt_ItApp* pApp)
{
	m_srcColor = pApp->m_sourceColor;
}

void CLayout::SetTgtColor(CAdapt_ItApp* pApp)
{
	m_tgtColor = pApp->m_targetColor;
}

void CLayout::SetNavTextColor(CAdapt_ItApp* pApp)
{
	m_navTextColor = pApp->m_navTextColor;
}

wxColour CLayout::GetSrcColor()
{
	return m_srcColor;
}

wxColour CLayout::GetTgtColor()
{
	return m_tgtColor;
}

wxColour CLayout::GetNavTextColor()
{
	return m_navTextColor;
}

wxColour CLayout::GetSpecialTextColor()
{
	return m_pApp->m_specialTextColor; // CLayout does not yet store a copy
									   // of m_specialTextColor
}

wxColour CLayout::GetRetranslationTextColor()
{
	return m_pApp->m_reTranslnTextColor; // CLayout does not yet store a copy
									     // of m_reTranslnTextColor
}

wxColour CLayout::GetTgtDiffsTextColor()
{
	return m_pApp->m_tgtDiffsTextColor; // CLayout does not yet store a copy
									    // of m_tgtDiffsTextColor
}

// accessors for src, tgt, navText line heights
void CLayout::SetSrcTextHeight(CAdapt_ItApp* pApp)
{
	m_nSrcHeight = pApp->m_nSrcHeight;
}

void CLayout::SetTgtTextHeight(CAdapt_ItApp* pApp)
{
	m_nTgtHeight = pApp->m_nTgtHeight;
}

void CLayout::SetNavTextHeight(CAdapt_ItApp* pApp)
{
	m_nNavTextHeight = pApp->m_nNavTextHeight;
}

int CLayout::GetSrcTextHeight()
{
	return m_nSrcHeight;
}

int CLayout::GetTgtTextHeight()
{
	return m_nTgtHeight;
}

int	CLayout::GetNavTextHeight()
{
	return m_nNavTextHeight;
}

// leading for strips
void CLayout::SetCurLeading(CAdapt_ItApp* pApp)
{
	m_nCurLeading = pApp->m_curLeading;
}

int CLayout::GetCurLeading()
{
	return m_nCurLeading;
}

// set left margin for strips; for the getter, use GetStripLeft()
void CLayout::SetCurLMargin(CAdapt_ItApp* pApp)
{
	m_nCurLMargin = pApp->m_curLMargin;
}

int CLayout::GetStripLeft()
{
	return m_nCurLMargin;
}

int CLayout::GetSavedLeading()
{
	return m_nCurLeading;
}

int CLayout::GetSavedGapWidth()
{
	return m_nCurGapWidth;
}

void CLayout::SetSavedLeading(int nCurLeading)
{
	m_nCurLeading = nCurLeading;
}

void CLayout::SetSavedGapWidth(int nGapWidth)
{
	m_nCurGapWidth = nGapWidth;
}

// setter and getters for the pile height and strip height

// BEW changed 5Oct11, if special functions change the app mode without going through the
// standard calls, e.g. to do Print or Print Preview which switches to free translation
// mode if there are free trans to print or preview, then the mode change happens but the
// strip height and pile height doesn't change. So we have to call SetPileAndStripHeight()
// internally and then return the m_nStripHeight value
// BEW 5Oct11, commented it out again, I think the problem was that the print pagination
// was happening from PrintOptionsDlg::InitDialog() to get the initial params for the
// dialog to show, and at that point, SwitchScreenFreeTranslationMode() and/or
// ShowGlosses() had not been called, and so the relevant switches, e. gbIsGlossing and
// gbGlossingVisible and m_bFreeTranslationMode remain unset - and so PaginateDoc() in the
// view class uses values for the StripHeight which are too small. The solution is
// probably to get the relevant function calls done earlier before or at PaginateDoc() so
// that the height calcs are correct when paginating...
int CLayout::GetPileHeight()
{
	//SetPileAndStripHeight(); // BEW added 5Oct11
	return m_nPileHeight;
}

// BEW changed 5Oct11, if special functions change the app mode without going through the
// standard calls, e.g. to do Print or Print Preview which switches to free translation
// mode if there are free trans to print or preview, then the mode change happens but the
// strip height and pile height doesn't change. So we have to call SetPileAndStripHeight()
// internally and then return the m_nStripHeight value
// BEW 5Oct11, commented it out again, I think the problem was that the print pagination
// was happening from PrintOptionsDlg::InitDialog() to get the initial params for the
// dialog to show, and at that point, SwitchScreenFreeTranslationMode() and/or
// ShowGlosses() had not been called, and so the relevant switches, e. gbIsGlossing and
// gbGlossingVisible and m_bFreeTranslationMode remain unset - and so PaginateDoc() in the
// view class uses values for the StripHeight which are too small. The solution is
// probably to get the relevant function calls done earlier before or at PaginateDoc() so
// that the height calcs are correct when paginating...
int CLayout::GetStripHeight()
{
	//SetPileAndStripHeight(); // BEW added 5Oct11
	return m_nStripHeight;
}

// BEW changed 1Oct11; the legacy version assumes that free translations will never be
// printed, and so it excluded adding in the target text height + 3 pixels when free
// translation mode is current (for printing, to print free translations, we turn that
// mode on temporarily for the duration of the print, or print preview); so I had to fix
// this for version 6, so that the addition was made whenever free translation mode is on
void CLayout::SetPileAndStripHeight()
{
	if (gbShowTargetOnly)
	{
		if (gbIsGlossing)
		{
			if (gbGlossingUsesNavFont)
				m_nPileHeight = m_nNavTextHeight;
			else
				m_nPileHeight = m_nTgtHeight;
		}
		else // adapting mode
		{
			m_nPileHeight = m_nTgtHeight;
		}
	}
	else // normal adapting or glossing, at least 2 lines (src & tgt) but even if 3,
		 // still those two
	{
		m_nPileHeight = m_nSrcHeight + m_nTgtHeight;

        // we've accounted for source and target lines; now handle possibility of a 3rd
        // line (note, if 3 lines, target is always one, so we've handled that above
        // already)
		if ((!m_pApp->m_bIsPrinting && gbGlossingVisible) || (m_pApp->m_bIsPrinting && gbCheckInclGlossesText))
		{
			if (gbGlossingUsesNavFont)
			{
				// we are showing only the source and target lines
				m_nPileHeight += m_nNavTextHeight;
			}
			else // glossing uses tgtTextFont
			{
				m_nPileHeight += m_nTgtHeight;
			}
			// add 2 more pixels because we will space the glossing line off from
			// whatever line it follows by an extra 3 pixels, in CreatePile( ) - since
			// with many fonts it appears to encroach on the bottom of the line above
			// (MFC app, we used 3, for wxWidgets try 2)
			m_nPileHeight += 2;
		}
	} // end of else block
    // the pile height is now set; so in the stuff below, set the strip height too - it is
    // the m_nPileHeight value, +/-, depending on whether free translation mode is on or
    // not, the target text line height plus 3 pixels of separating space from the bottom
    // of the pile
    // (Note: the m_nCurLeading value, for the navText area, is NOT regarded as part of
    // the strip)
	m_nStripHeight = m_nPileHeight;
	if ((!m_pApp->m_bIsPrinting && m_pApp->m_bFreeTranslationMode) ||
		(m_pApp->m_bIsPrinting && gbCheckInclFreeTransText))
	{
        // add enough space for a single line of the height given by the target text's
        // height + 3 pixels to set it off a bit from the bottom of the pile
		m_nStripHeight += m_nTgtHeight + 3;
	}
}

void CLayout::RecalcPileWidths(PileList* pPiles)
{
	PileList::Node* pos = pPiles->GetFirst();
	wxASSERT(pos != NULL);
	CPile* pPile = NULL;
	while (pos != NULL)
	{
		pPile = pos->GetData();
		wxASSERT(pPile != NULL);
		pPile->SetMinWidth(); // the calculation of the gap for the phrase box is
							// handled within RecalcLayout(), so does not need to be
							// done here
		pos = pos->GetNext();
	}
	SetPileAndStripHeight(); // it may be changing, eg to or from "See Glosses"
}

int CLayout::GetStripCount()
{
	return (int)m_stripArray.GetCount();
}

PileList* CLayout::GetPileList()
{
	return &m_pileList;
}

PileList* CLayout::GetSavePileList()
{
	if (m_pSavePileList == NULL)
	{
		m_pSavePileList = new PileList;
		return m_pSavePileList;
	}
	else
		return m_pSavePileList;
}

void CLayout::ClearSavePileList()
{
	if (m_pSavePileList == NULL)
	{
		return;
	}
	else if (m_pSavePileList->GetCount() > 0)
	{
		// these are pointer copies, so the same pointers are in some other list, so do
		// not destroy the CPile instances, just abandon the pointers
		m_pSavePileList->Clear();
		m_pSavePileList = NULL;
		return;
	}
	else
	{
		m_pSavePileList = NULL;
	}
}


wxArrayPtrVoid* CLayout::GetStripArray()
{
	return & m_stripArray;
}

// Call SetClientWindowSizeAndLogicalDocWidth() before just before strips are laid out;
// then call SetLogicalDocHeight() after laying out the strips, to get private member
// m_logicalDocSize.y set
void CLayout::SetClientWindowSizeAndLogicalDocWidth()
{
	// GetClientRect gets a rectangle in which upper left coords are always 0,0
    //pApp->GetMainFrame()->canvas->GetClientSize(&fwidth,&fheight); // get width & height
    //in pixels wx note: calling GetClientSize on the canvas produced different results in
    //wxGTK and wxMSW, so I'll use my own GetCanvasClientSize() which calculates it from
    //the main frame's client size.
	wxSize canvasViewSize;
	canvasViewSize = m_pMainFrame->GetCanvasClientSize(); // dimensions of client window of
								// wxScrollingWindow which canvas class is a subclass of
	m_sizeClientWindow = canvasViewSize; // set the private member, CLayout::m_sizeClientWindow
	wxSize docSize;
	docSize.y = 0; // can't be set yet, we call this setter before strips are laid out
	if (m_pApp->m_bIsPrinting)
	{
		// the document width will be set by the page dimensions and margins, externally
		// to RecalcLayout(), so do nothing here except set it to zero
		docSize.x = 0;
	}
	else
	{
		// not printing, so the layout is being done for the screen
		docSize.x = m_sizeClientWindow.x - m_nCurLMargin - RH_SLOP;
                // RH_SLOP defined in AdaptItConstants.h with a value of 60 (reduces
                // likelihood of long nav text above a narrow pile which is last in a
                // strip, having the end of the nav text drawn off-window
	}
	m_logicalDocSize = docSize; // initialize the private member, CLayout::m_logicalDocSize
}

// sets m_logicalDocSize.y value to the logical height (in pixels) of the laid out strips, or to
// zero if there are no strips in m_stripArray yet; however this function should never be called
// when the latter is the case - that would be a serious design error
void CLayout::SetLogicalDocHeight()
{
	int nDocHeight = 0;
	if (!m_stripArray.IsEmpty())
	{
		int nStripCount = m_stripArray.GetCount();
		nDocHeight = (GetCurLeading() + GetStripHeight()) * nStripCount;
		// BEW kluge 20Jun09. Bill already had a 40 pixel (2 scroll units) of white space
		// added to the bottom of the logical doc here, but attempts in vertical edit mode
		// to scroll the view window to the bottom of the doc always don't go far enough,
		// by about 50 pixels. So since we've already got a Bill kluge here, I might as
		// well add a Bruce kluge too, and increase the white space at bottom to 120
		// pixels, and then when the box is in the last strip, the last strip isn't below
		// the bottom of the view window for an unknown reason...
		if (gbVerticalEditInProgress)
			nDocHeight += 120; // a larger value is needed for vertical edit mode
		else
			nDocHeight += 40; // 40 pixels for some white space at document's bottom
	}
	m_logicalDocSize.SetHeight(nDocHeight);
}

wxSize CLayout::GetClientWindowSize()
{
	return m_sizeClientWindow;
}

wxSize CLayout::GetLogicalDocSize()
{
	return m_logicalDocSize;
}

// current gap width between piles (in pixels)
void CLayout::SetGapWidth(CAdapt_ItApp* pApp)
{
	m_nCurGapWidth = pApp->m_curGapWidth; // user sets it in Preferences' View tab
}
// CLayout, CStrip, CPile & CCell are mutual friends, so they can grab m_nCurGapWidth
// directly, but outsiders will need the followinng
int CLayout::GetGapWidth()
{
	return m_nCurGapWidth;
}


// SetLayoutParameters() is where we do most of the hooking up to the current state of the
// app's various view-related parameters, such as fonts, colours, text heights, and so
// forth
void CLayout::SetLayoutParameters()
{
	m_pApp->UpdateTextHeights(m_pView);
	SetSrcFont(m_pApp);
	SetTgtFont(m_pApp);
	SetNavTextFont(m_pApp);
	SetSrcColor(m_pApp);
	SetTgtColor(m_pApp);
	SetNavTextColor(m_pApp);
	SetSrcTextHeight(m_pApp);
	SetTgtTextHeight(m_pApp);
	SetNavTextHeight(m_pApp);
	SetGapWidth(m_pApp);
	SetPileAndStripHeight();
	SetCurLeading(m_pApp);
	SetCurLMargin(m_pApp);
	SetClientWindowSizeAndLogicalDocWidth();
}

void CLayout::DestroyStrip(int index)
{
	// CStrip now does not store pointers, so the only memory it owns are the blocks for
	// the two wxArrayInt arrays - so Clear() these and then delete the strip
	CStrip* pStrip = (CStrip*)m_stripArray.Item(index);
	pStrip->m_arrPiles.Clear();
	pStrip->m_arrPileOffsets.Clear();
	if (pStrip != NULL) // whm 11Jun12 added NULL test
	 	delete pStrip;
	// don't try to delete CCell array, because the cell objects are managed
    // by the persistent pile pointers in the CLayout array m_pPiles, and the
    // strip does not own these
}

void CLayout::DestroyStripRange(int nFirstStrip, int nLastStrip)
{
	if (m_stripArray.IsEmpty())
		return; // needed because DestroyStripRange() can be called when
				// nothing is set up yet
	int index;
	for (index = nFirstStrip; index <= nLastStrip; index++)
		DestroyStrip(index);
}

void CLayout::DestroyStrips()
{
	if (m_stripArray.IsEmpty())
		return; // needed because DestroyStrips() can be called when nothing
				// is set up yet
	int nLastStrip = m_stripArray.GetCount() - 1;
	DestroyStripRange(0, nLastStrip);
	m_stripArray.Clear();
}

void CLayout::DestroyPile(CPile* pPile, PileList* pPileList, bool bRemoveFromListToo)
{
	PileList::Node* pos;
	int index;
	pPile->SetStrip(NULL); // sets m_pOwningStrip to NULL
	for (index = 0; index < MAX_CELLS; index++)
	{
		if (pPile->m_pCell[index] != NULL) // whm 11Jun12 added NULL test
			delete pPile->m_pCell[index];
	}
	if (bRemoveFromListToo)
	{
		pos = pPileList->Find(pPile);
		wxASSERT(pos != NULL);
		pPileList->Erase(pos);
	}
	if (pPile != NULL) // whm 11Jun12 added NULL test
		delete pPile;
	pPile = NULL;
}

/* not used
void CLayout::DestroyPileRange(int nFirstPile, int nLastPile)
{
	if (m_pileList.IsEmpty())
		return; // needed because DestroyPileRange() can be called when nothing is set up yet
	PileList::Node* pos = m_pileList.Item(nFirstPile);
	int index = nFirstPile;
	if (pos == NULL)
	{
		wxMessageBox(_T("nFirstPile index did not return a valid iterator in DestroyPileRange()"),
						_T(""), wxICON_STOP);
		wxASSERT(FALSE);
		wxExit();
	}
	CPile* pPile = 0;
	while (index <= nLastPile)
	{
		pPile = pos->GetData();
		wxASSERT(pPile != NULL);
		DestroyPile(pPile);
		index++;
		pos = pos->GetNext();
	}
}
*/

// The copying is done by appending. Normally pDestPileList will be empty at the start,
// but if not, whatever is copied will be appended to any CPile instances already in the
// passed in pDestPileList
void CLayout::CopyPileList_Shallow(PileList* pOrigPileList, PileList* pDestPileList)
{
	wxASSERT(pOrigPileList != NULL);
	wxASSERT(pDestPileList != NULL);
	PileList::Node* pileNode = NULL;
	for (pileNode = pOrigPileList->GetFirst(); pileNode; pileNode = pileNode->GetNext())
	{
		CPile *pData = pileNode->GetData();
		pDestPileList->Append(pData); // shallow copy the pointers across
	}
    // both list now have their saved CPile pointers pointing at the one set of CPile
    // instances on the heap
}

void CLayout::DestroyPiles()
{
	if (m_pileList.IsEmpty())
		return; // needed because DestroyPiles() can be called when nothing is set up yet
	PileList::Node* pos = m_pileList.GetLast();
	wxASSERT(pos != NULL);
	CPile* pPile = NULL;
	while (pos != NULL)
	{
		pPile = pos->GetData();
		DestroyPile(pPile,&m_pileList,FALSE);
		pos = pos->GetPrevious();
	}
	m_pileList.Clear(); // ensure there are no freed pointers left over
}


// Note: never call CreatePile() if there is not yet a valid pSrcPhrase pointer to pass in;
// CreatePile() creates the CPile instance on the heap, and returns a pointer to it
CPile* CLayout::CreatePile(CSourcePhrase* pSrcPhrase)
{
	wxASSERT(pSrcPhrase != NULL);

	// create an empty pile on the heap
	CPile* pPile = new CPile;
	wxASSERT(pPile != NULL); // if tripped, must be a memory error

	// assign the passed in source phrase pointer to its m_pSrcPhrase public member,
	// and also the pointer to the CLayout object
	pPile->m_pSrcPhrase = pSrcPhrase;
	pPile->m_pLayout = this;

    // set (in pixels) its minimum width based on which of the m_srcPhrase, m_adaption and
    // m_gloss strings within the passed in CSourcePhrase is the widest; the min width
    // member is thereafter a lower bound for the width of the phrase box when the latter
    // contracts, if located at this pile while the user is working
	pPile->m_nMinWidth = pPile->CalcPileWidth(); // if an empty string, it is defaulted to
												 // 10 pixels
	if (pSrcPhrase->m_nSequNumber != m_pApp->m_nActiveSequNum)
	{
		pPile->m_nWidth = PHRASE_BOX_WIDTH_UNSET; // a default -1 value for starters, need
                // an active sequ number and m_targetPhrase set before a value can be
                // calculated, but only at the pile which is located at the active location
	}
	else
	{
		// this pile is going to be the active one, so calculate its m_nWidth value using
		// m_targetPhrase
		pPile->SetPhraseBoxGapWidth(); // calculates, and sets value in m_nWidth
	}

	// pile creation always creates the (fixed) array of CCells which it manages
	int index;
	CCell* pCell = NULL;
	for (index = 0; index < MAX_CELLS; index++)
	{
		pCell = new CCell();

		// store it
		pPile->m_pCell[index] = pCell; // index ranges through 0 1 and 2 in our new design
		pCell->CreateCell(this,pPile,index);
	}
	// Note: to assist clarity, the extensive commented out legacy code has been removed
	// from here and placed temporarily at the end of this file; much of it no longer applies
	return pPile;
}

// return TRUE if no error, FALSE if it failed to do the creation properly for any reason
bool CLayout::CreatePiles(SPList* pSrcPhrases)
{
	// we expect pSrcPhrases list is populated, so if it is not we
	// treat that as a serious logic error
	if (pSrcPhrases == NULL || pSrcPhrases->IsEmpty())
	{
		wxMessageBox(_T(
		"SPList* passed in was either NULL or devoid of pilesl in CreatePiles()"),
		_T(""), wxICON_STOP);
		wxASSERT(FALSE);
		wxExit(); // something seriously wrong in design, so don't try to go on
		return FALSE;
	}

    // we allow CreatePiles() to be called even when the list of piles is still populated,
    // so CreatePiles() has the job of seeing that the old ones are deleted before creating
    // a new set bool bIsEmpty = m_pPiles->IsEmpty();
	bool bIsEmpty = m_pileList.IsEmpty();
	if (!bIsEmpty)
	{
		// delete the old ones
		DestroyPiles();
	}
	wxASSERT(m_pileList.IsEmpty());

	CSourcePhrase* pSrcPhrase = NULL;
	CPile* pPile = NULL;
	SPList::Node* pos = pSrcPhrases->GetFirst();
    // Note: lack of memory could make the following loop fail in the release version if
    // the user is processing a very large document - but recent computers should have
    // plenty of RAM available, so we'll assume a low memory error will not arise; so keep
    // the code lean
	while (pos != NULL)
	{
		// get the CSourcePhrase pointer from the passed in list
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		wxASSERT(pSrcPhrase != NULL);

		// create the CPile instance
		pPile = CreatePile(pSrcPhrase);
		wxASSERT(pPile != NULL);

		// store it m_pPiles list of CLayout
		m_pileList.Append(pPile);

		// get ready for next iteration
		pos = pos->GetNext();
	}

    // To succeed, the count of items in each list must be identical
	if (pSrcPhrases->GetCount() != m_pileList.GetCount())
	{
		wxMessageBox(_T(
"SPList* passed in, in CreatePiles(), has a count which is different from that of the m_pPiles list in CLayout"),
		_T(""), wxICON_STOP);
		wxASSERT(FALSE);
		wxExit();	// something seriously wrong in design probably, such an error should not
					// happen, so don't try to go on
		return FALSE;
	}
	return TRUE;
}

void CLayout::CopyLogicalDocSizeFromApp()
{
	m_logicalDocSize = m_pApp->m_docSize; // when printing, both .x and .y must have
					// been set before calling this function, so we call it in
					// OnPreparePrinting, to hook up to the resized strip width
					// and doc length for printing on paper, which RecalcLayout()
					// will need to use when it rebuilds the strips; restore
					// after printing by calling RestoreLogicalDocDizeFromSavedSize()
					// in ~AIPrintout() before RecalcLayout() is called there
}

void CLayout::RestoreLogicalDocSizeFromSavedSize()
{
	m_logicalDocSize = m_pApp->m_saveDocSize; // copy it to our class's variable
					// with similar name, as this is used in RecalcLayout() calls
	// in the destructor for AIPrintout, the app member, m_saveDocSize is used to
	// restore the app member m_docSize, so we don't need to do that here - we only
	// need to restore the CLayout::m_logicalDocSize here
}

// return TRUE if a layout was set up, or if no layout can yet be set up;
// but return FALSE if a layout setup was attempted and failed (app must then abort)
//
// Note: RecalcLayout() should be called, for best results, immediately before opening, or
// creating, an Adapt It document. At that point all of the paramaters it collects together
// should have their "final" values, such as font choices, colours, etc. It is also to be
// called whenever the layout has been corrupted by a user action, such as a font change
// (which clobbers prior text extent values stored in the piles) or a punctuation setting
// change, etc. RecalcLayout() in the refactored design is a potentially "costly"
// calculation - if the document is large, the piles and strips for the whole document have
// to be recalculated from the data in the current document - (need to consider a progress
// bar in the status bar in window bottom) The default value of the passed in flag
// bRecreatePileListAlso is FALSE The ptr value for the passed in pList is usually the
// application's m_pSourcePhrases list, but it can be a sublist copied from that
//GDLC Added third parameter 2010-02-09
bool CLayout::RecalcLayout(SPList* pList, enum layout_selector selector, enum phraseBoxWidthAdjustMode boxMode)
{
    // RecalcLayout() is the refactored equivalent to the former view class's RecalcLayout()
    // function - the latter built only a bundle's-worth of strips, but the new design must build
	// strips for the whole document - so potentially may consume a lot of time; however, the
	// efficiency of the new design (eg. no rectangles are calculated) may compensate significantly

	// every call of RecalcLayout() should calculate the number of strips currently that
	// could be shown in the client area, setting the m_numVisibleStrips private member of
	// the CLayout instance - other functions can then access it using
	// GetNumVisibleStrips() and rely on the value returned; when printing however, the
	// number of "visible" strips is determined by what can fit on the printed page, and
	// this is calculated externally in PaginateDoc()
	/*
	CAdapt_ItApp* pApp = &wxGetApp();
	if (gbIsPrinting)
		wxLogDebug(_T("\n\nPRINTING   RecalcLayout()  app m_docSize.x  %d  CLayout m_logicalDocSize.x %d"),
				pApp->m_docSize.x, m_logicalDocSize.x);
	else
		wxLogDebug(_T("\n\nNOT PRINTING   RecalcLayout()  app m_docSize.x  %d  CLayout m_logicalDocSize.x %d"),
				pApp->m_docSize.x, m_logicalDocSize.x);
	*/
	SetFullWindowDrawFlag(TRUE);
	if (!m_pApp->m_bIsPrinting)
	{
		m_numVisibleStrips = CalcNumVisibleStrips();
		//wxLogDebug(_T("RecalcLayout()  SHOULDN'T SEE THIS WHEN PRINTING,  m_numVisibleStrips  %d  "),
		//m_numVisibleStrips);
	}

	SPList* pSrcPhrases = pList; // the list of CSourcePhrase instances which
        // comprise the document, or a sublist copied from it, -- the CLayout instance will
        // own a parallel list of CPile instances in one-to-one correspondence with the
        // CSourcePhrase instances, and each of piles will contain a pointer to the
        // sourcePhrase it is associated with

	if (selector == create_strips_and_piles || selector == create_strips_keep_piles ||
		selector == create_strips_update_pile_widths)
	{
		// any existing strips have to be destroyed before the new are build. Note: if we
		// support both modes of strip support (ie. tweaking versus destroy and rebuild all)
		// then the tweaking option must now destroy them, but just make essential adjustments
		// for the stuff currently in the visible range
		DestroyStrips();

		if (pSrcPhrases == NULL || pSrcPhrases->IsEmpty())
		{
			// no document is loaded, so no layout is appropriate yet, do nothing
			// except ensure m_pPiles is NULL
			if (!(m_pileList.IsEmpty()))
			{
				m_pileList.DeleteContents(TRUE); // TRUE means "delete the stored CCell instances too"
			}
			SetFullWindowDrawFlag(FALSE);
			return TRUE;
		}
	}
	wxRect rectFrame(0,0,0,0);
	CMainFrame *pFrame = NULL;
	if (!m_pApp->m_bIsPrinting)
	{
		// send the app the current size & position data, for saving to config file on closure
		pFrame = wxGetApp().GetMainFrame();
		wxASSERT(pFrame != NULL);
		rectFrame = pFrame->GetRect(); // screen coords
		rectFrame = NormalizeRect(rectFrame); // use our own from helpers.h
		m_pApp->m_ptViewTopLeft.x = rectFrame.x;
		m_pApp->m_ptViewTopLeft.y = rectFrame.y;
		m_pApp->m_szView.SetWidth(rectFrame.GetWidth());
		m_pApp->m_szView.SetHeight(rectFrame.GetHeight());
		m_pApp->m_bZoomed = pFrame->IsMaximized();

		// initialize CLayout's m_logicalDocSize, and m_sizeClientWindow (except
		// m_logicalDocSize.y is initialized to 0 here, because it can't be set until the
		// strips have been laid out - that is done below
		SetClientWindowSizeAndLogicalDocWidth();

		// set the pile height, and the strip height (the latter includes the free translation
		// height when in free translation mode; the former is always just the height of the
		// visible cells)
		SetPileAndStripHeight();
	}
	// scroll support...
	// get a device context, and get the origin adjusted (gRectViewClient is ignored
	// when printing)
	wxClientDC viewDC(m_pApp->GetMainFrame()->canvas);
	m_pApp->GetMainFrame()->canvas->DoPrepareDC(viewDC); //  adjust origin
	// BEW 9Jul09; add test to jump grectViewClient calculation when printing, it just
	// wastes time because the values are not used when printing
	if (!m_pApp->m_bIsPrinting)
	{
		m_pApp->GetMainFrame()->canvas->CalcUnscrolledPosition(
											0,0,&grectViewClient.x,&grectViewClient.y);
		grectViewClient.width = m_sizeClientWindow.GetWidth(); // m_sizeClientWindow set in
							// the above call to SetClientWindowSizeAndLogicalDocWidth()
		grectViewClient.height = m_sizeClientWindow.GetHeight();
	}
	// if we are printing, then we will want text extents (which use viewDC for their calculation)
	// to be done for MM_LOENGLISH mapping mode
	// whm notes:
	// wxWidgets has the following defined mapping modes:
	// wxMM_TWIPS		Each logical unit is 1/20 of a point, or 1/1440 of an inch.
	// wxMM_POINTS		Each logical unit is a point, or 1/72 of an inch.
	// wxMM_METRIC		Each logical unit is 1 mm.
	// wxMM_LOMETRIC	Each logical unit is 1/10 of a mm.
	// wxMM_TEXT		Each logical unit is 1 pixel.
	// MFC has the following defined mapping modes:
	// MM_ANISOTROPIC	Logical units are converted to arbitrary units with arbitrarily scaled axes. Setting the mapping mode to MM_ANISOTROPIC does not change the current window or viewport settings. To change the units, orientation, and scaling, call the SetWindowExt and SetViewportExt member functions.
	// MM_HIENGLISH		Each logical unit is converted to 0.001 inch. Positive x is to the right; positive y is up.
	// MM_HIMETRIC		Each logical unit is converted to 0.01 millimeter. Positive x is to the right; positive y is up.
	// MM_ISOTROPIC		Logical units are converted to arbitrary units with equally scaled axes; that is, 1 unit along the x-axis is equal to 1 unit along the y-axis. Use the SetWindowExt and SetViewportExt member functions to specify the desired units and the orientation of the axes. GDI makes adjustments as necessary to ensure that the x and y units remain the same size.
	// MM_LOENGLISH		Each logical unit is converted to 0.01 inch. Positive x is to the right; positive y is up.
	// MM_LOMETRIC		Each logical unit is converted to 0.1 millimeter. Positive x is to the right; positive y is up.
	// MM_TEXT			Each logical unit is converted to 1 device pixel. Positive x is to the right; positive y is down.
	// MM_TWIPS			Each logical unit is converted to 1/20 of a point. (Because a point is 1/72 inch, a twip is 1/1440 inch.) Positive x is to the right; positive y is up.
	// There are only 3 mapping modes that use the same units between MFC and wxWidgets which are:
	// wxMM_TWIPS is same as MM_TWIPS - in both each logical unit is converted to 1/20 of a point or 1/1440 inch. Positive x to the right, positive y is up.
	// wxMM_TEXT is same as MM_TEXT - in both each logical unit is converted to 1 device pixel. Positive x to the right, positive y is down.
	// wxMM_LOMETRIC is same as MM_LOMETRIC - in both each logical unit is converted to 1/10 of a mm
	// All other mapping modes are different units or scales

	// The map mode remains wxMM_TEXT for both normal screen rendering and for printing/previewing.
	// MFC had MM_LOENGLISH, in which each logical unit is converted to 0.01 inch.
	// and Positive x is to the right; positive y is up.
	// wxMM_LOENGLISH compiles OK but give a run time error/assert "unknown mapping mode in
	// SetMapMode." Therefore if we use anything other than wxMM_TEXT, we will need to use wxMM_LOMETRIC
	// for printing in the wx version. Since MFC uses MM_LOENGLISH which reverses the y axis
	// component during printing and previewing, we'll use wxMM_LOMETRIC, the closest equivalent which
	// also reverses the y axis component during printing.
	viewDC.SetMapMode(wxMM_TEXT); // equivalent to MFC's MM_TEXT for drawing to the screen


	// RecalcLayout() depends on the app's m_nActiveSequNum valuel for where the active
	// location is to be; so we'll make that dependency explicit in the next few lines,
	// obtaining the active pile pointer which corresponds to that active location as well
	bool bAtDocEnd = FALSE; // set TRUE if m_nActiveSequNum is -1 (as is the case when at
							// the end of the document)
	CPile* pActivePile = NULL;
	pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum); // will return NULL if sn is -1
	// BEW added 2nd test, for sn == -1, because reliance on gbDoingInitialSetup is risky
	// -- for example, in collab mode the flag stayed TRUE, and so bAtDocEnd didn't get
	// set when the last bit of adapting in the file was done and the phrase box moved
	// past the doc end - giving a crash because the app thought the doc end was not yet
	// reached (at doc end, before RecalcLayout() is called, sn is set to -1, so we can
	// rely on this here)
	if (!gbDoingInitialSetup || m_pApp->m_nActiveSequNum == -1)
	{
		// the above test is to exclude setting bAtDocEnd to TRUE if the pActivePile is
		// NULL which can be the case on launch or opening a doc or creating a new one, at
		// least temporarily, due to the default sequence number being -1
		// gbDoingInitialSetup is cleared to FALSE in OnNewDocument() and OnOpenDocument()
		if (pActivePile == NULL)
			bAtDocEnd = TRUE; // provide a different program path when this is the case
	}

    // attempt the (re)creation of the m_pileList list of CPile instances if requested; if
    // not requested then the current m_pileList's contents will be retained, though their
	// contents may have been adjusted in the area where the user did editing (in which
	// case their widths should have been recalculated before RecalcLayout() is called,
	// unless many are to be done in one hit, by the else block's test below)
	if (selector == create_strips_and_piles)
	{
		bool bIsOK = CreatePiles(pSrcPhrases);
		if (!bIsOK)
		{
            // something was wrong - memory error or perhaps m_pPiles is a populated list
            // already (CreatePiles()has generated an error message for the developer
            // already)
			return FALSE;
		}
	}
	else if (selector == create_strips_update_pile_widths)
	{
		RecalcPileWidths(&m_pileList);
	}

	int gap = m_nCurGapWidth; // distance in pixels for interpile gap
	int nStripWidth = (GetLogicalDocSize()).x; // constant for any one RecalcLayout call,
	// and when printing, external functions will already have set the "logical"
	// size returned by this call to a size based on the physical page's printable width
    // before building or tweaking the strips, we want to ensure that the gap left for the
    // phrase box to be drawn in the layout is as wide as the phrase box is going to be
    // when it is made visible by CLayout::Draw(). When RecalcLayout() is called with its
	// third parameter phraseBoxWidthAdjustMode equal to expanding, if we destroyed and
	// recreated the piles in the block above,
    // CreatePile() will, at the active location, made use of the expanding value and set
    // the "hole" for the phrase box to be the appropriate width. But if piles were not
    // destroyed and recreated then the box may be about to be drawn expanded, and so we
    // must make sure that the strip rebuilding about to be done below has the right box
    // width value to be used for the pile at the active location. The safest way to ensure
    // this is the case is to make a call to doc class's ResetPartnerPileWidth(), passing
    // in the CSourcePhrase pointer at the active location - this call internally calls
    // CPile:CalcPhraseBoxGapWidth() to set CPile's m_nWidth value to the right width, and
    // then the strip layout code below can use that value via a call to
    // GetPhraseBoxGapWidth() to make the active pile's width have the right value.
	//TODO: Is the above paragraph completly correct??
    if (!bAtDocEnd)
	{
		// when not at the end of the document, we will have a valid pile pointer for the
		// current active location, provided we are not printing a range
		if (selector == create_strips_keep_piles || selector == keep_strips_keep_piles
			|| selector == create_strips_update_pile_widths)
		{
            // these three lines ensure that the active pile's width is based on the
            // CLayout::m_curBoxWidth value that was stored there by any adjustment to the
            // box width done by FixBox() just prior to this RecalcLayout() call
			pActivePile = GetPile(m_pApp->m_nActiveSequNum);
			wxASSERT(pActivePile);
			CSourcePhrase* pSrcPhrase = pActivePile->GetSrcPhrase();
			if (boxMode == contracting)
			{
				// phrase box is meant to contract for this recalculation, so suppress the
				// size calculation internally for the active location because it would be
				// larger than the contracted width we want
				m_pDoc->ResetPartnerPileWidth(pSrcPhrase,TRUE); // TRUE is the boolean
														// bNoActiveLocationCalculation
			}
			else // not contracting, could be expanding or no size change
			{
				// allow the active location gap calculation to be done
				m_pDoc->ResetPartnerPileWidth(pSrcPhrase);
			}
		}
	}
	else // control is past the end of the document
	{
		// we have no active active location currently, and the box is hidden, the active
		// pile is null and the active sequence number is -1, so we want a layout that has
		// no place provided for a phrase box, and we'll draw the end of the document
		//GDLC Removed setting of gbExpanding 2010-02-09
		//gbExpanding = FALSE; // has to be restored to default value
	}
	//GDLC Removed setting of gbContracting 2010-02-09
	//gbContracting = FALSE; // restore default value
/*
#ifdef _DEBUG
	{
		PileList::Node* pos = m_pileList.GetFirst();
		CPile* pPile = NULL;
		while (pos != NULL)
		{
			pPile = pos->GetData();
			wxLogDebug(_T("m_srcPhrase:  %s  *BEFORE* pPile->m_pOwningStrip =  %x"),
				pPile->GetSrcPhrase()->m_srcPhrase,pPile->m_pOwningStrip);
			pos = pos->GetNext();
		}
	}
#endif
*/
	// the active pile needs to be set if using the keep_strips_keep_piles option, so if
	// there is a positive m_nActiveSequNum value, use it to set a temporary m_pActivePile
	if ((m_pApp->m_pActivePile == NULL && m_pApp->m_nActiveSequNum != -1 &&
		selector != create_strips_and_piles) ||
		(selector == keep_strips_keep_piles && m_pApp->m_nActiveSequNum != -1) ||
		(selector == create_strips_update_pile_widths && m_pApp->m_nActiveSequNum != -1))
	{
		m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);
	}

	// the loop which builds the strips & populates them with the piles
	if (selector == create_strips_and_piles || selector == create_strips_keep_piles
		|| selector == create_strips_update_pile_widths)
	{
		CreateStrips(nStripWidth, gap);
	}
	if (selector == keep_strips_keep_piles)
	{
		bool bLayoutTweakingWasSuccessful = TRUE;
		// for this option, strips are not destroyed and recreated, instead they are
		// just tweaked at the location where they were marked as invalid by the
		// m_bValid flag in each being cleared to FALSE (when the user edit region
		// and the active location are separated by a gap, possibly large, all the
		// strips in between are included in the updating/tweaking (since the user is
		// only likely to do a thing like this for an edit area visible from the
		// visible active location, possibly the gap would only be a dozen piles
		// at most, and updating there should be so quick as to be unnoticed)
		// Note: in the next call, if it can't successfully delineate the user's editing
		// area, then if calls RecalcLayout() to get a full strip recreation done instead,
		// and returns FALSE; otherwise TRUE is returned
		bLayoutTweakingWasSuccessful = AdjustForUserEdits(nStripWidth, gap);
		if (bLayoutTweakingWasSuccessful == FALSE)
		{
			// when FALSE was returned, RecalcLayout() will have been reentered internally
			// with the input parameter create_strips_keep_piles, and will have already
			// done the layout recalculation by destroying and recreating all the strips,
			// and so we've nothing to do here except return immediately
			//GDLC Removed setting of gbContracting 2010-02-09
			//gbContracting = FALSE;
			return TRUE;
		}
	}
/*
#ifdef _DEBUG
	{
		PileList::Node* pos = m_pileList.GetFirst();
		CPile* pPile = NULL;
		while (pos != NULL)
		{
			pPile = pos->GetData();
			wxLogDebug(_T("m_srcPhrase:  %s  *AFTER* pPile->m_pOwningStrip =  %x"),
				pPile->GetSrcPhrase()->m_srcPhrase,pPile->m_pOwningStrip);
			pos = pos->GetNext();
		}
	}
#endif
*/

	//GDLC Removed setting of gbExpanding & gb Contracting 2010-02-09
	//gbExpanding = FALSE; // has to be restored to default value
	//gbContracting = FALSE; // restore default value (also done above)

	if (!m_pApp->m_bIsPrinting)
	{
		// the height of the document can now be calculated
		SetLogicalDocHeight();

		// next line for debugging...
		//wxSize theVirtualSize = m_pApp->GetMainFrame()->canvas->GetVirtualSize();

		// more scrolling support...
		// whm: SetVirtualSize() is the equivalent of MFC's SetScrollSizes.
		// SetVirtualSize() sets the virtual size of the window in pixels.
		m_pApp->GetMainFrame()->canvas->SetVirtualSize(m_logicalDocSize);

		// inform the application of the document's logical size... -- the canvas
		// class uses this m_docSize value for determining scroll bar parameters and whether
		// horizontal and / or vertical scroll bars are needed
		m_pApp->m_docSize = m_logicalDocSize;
	}
	// next line for debugging...
	//theVirtualSize = m_pApp->GetMainFrame()->canvas->GetVirtualSize();

	if (!m_pApp->m_bIsPrinting)
	{
		// The MFC identifiers m_pageDev and m_lineDev are internal members of CScrollView.
		// I cannot find them anywhere in MFC docs, but looking at CScrollView's sources, it
		// is evident that they are used to set the scrolling parameters within the scrolled
		// view in the MFC app. We'll convert the values to the proper units and use
		// SetScrollbars() to set the scrolling parameters.
		//wxSize nSize = m_pageDev; // m_pageDev is "per page scroll size in device units"
		//wxSize mSize = m_lineDev; // m_lineDev is "per line scroll size in device units"
		// wx note: the m_lineDev value in MFC is equivalent to the first two parameters of
		// wxScrollWindow's SetScrollbars (pixelsPerUnitX, and pixelsPerUnitY). Both MFC and
		// WX values are in pixels.
		// The m_pageDev value in MFC is set below as twice the height of a strip (including
		// leading). In WX the height of a page of scrolled stuff should be determined
		// automatically from the length of the document (in scroll units), divided by the
		// height of the client view (also in scroll units).
		// The parameters needed for SetScrollbars are:
		int pixelsPerUnitX, pixelsPerUnitY, noUnitsX, noUnitsY;
		int xPos, yPos; // xPos and yPos default to 0 if unspecified
		bool noRefresh; // noRefresh defaults to FALSE if unspecified
		// whm note: We allow our wxScrolledWindow to govern our scroll
		// parameters based on the width and height of the virtual document
		//
		// WX version: We only need to specify the length of the scrollbar in scroll
		// steps/units. Before we can call SetScrollbars we must calculate the size of the
		// document in scroll units, which is the size in pixels divided by the pixels per
		// unit.
		pFrame->canvas->GetScrollPixelsPerUnit(&pixelsPerUnitX,&pixelsPerUnitY);
		noUnitsX = m_pApp->m_docSize.GetWidth() / pixelsPerUnitX;
		noUnitsY = m_pApp->m_docSize.GetHeight() / pixelsPerUnitY;
		// we need to specify xPos and yPos in the SetScrollbars call, otherwise it will cause
		// the window to scroll to the zero position everytime RecalcLayout is called.
		// We'll use GetViewStart instead of CalcUnscrolledPosition here since SetScrollbars
		// below takes scroll units rather than pixels.
		m_pApp->GetMainFrame()->canvas->GetViewStart(&xPos, &yPos); // gets xOrigin and yOrigin
																	// in scroll units
		noRefresh = FALSE; // do a refresh
		// Now call SetScrollbars - this is the only place where the scrolling parameters are
		// established for our wxScrolledWindow (canvas). The scrolling parameters are reset
		// everytime RecalcLayout is called. This is the only location where SetScrollbars() is
		// called on the canvas.

		// whm IMPORTANT NOTE: We need to use the last position of the scrolled window here. If
		// we don't include xPos and yPos here, the scrollbar immediately scrolls to the
		// zero/initial position in the doc, which fouls up the calculations in other routines
		// such as MoveToPrevPile.
		m_pApp->GetMainFrame()->canvas->SetScrollbars(
				pixelsPerUnitX,pixelsPerUnitY,	// number of pixels per "scroll step"
        		noUnitsX,noUnitsY,				// sets the length of scrollbar in scroll steps,
												// i.e., the size of the virtual window
    			xPos,yPos,						// sets initial position of scrollbars NEEDED!
    			noRefresh);						// SetScrollPos called elsewhere
	}

	// if free translation mode is turned on, get the current section
	// delimited and made visible - but only when not currently printing
	//
	// BEW 28Nov11 added gbCheckInclFreeTrans to the test, because when there is a
	// free translation in the document, if the CPrintOptionDlg is put up, it's
	// InitDialog() function temporarily sets m_bIsPrinting to FALSE, and so if
	// m_pActivePile is invalid (as would be the case for a print range, pring page
	// range, or print selection choice), then the code below would crash without
	// the extra protection being added
#if defined(_DEBUG) && defined(Print_failure)
{
    wxLogDebug(_T("RecalcLayout() line 2024, flags at free trans block at end **********************\nm_bFreeTranslationMode %d , m_bIsPrinting %d , gbCheckInclFreeTransText"),
               m_pApp->m_bFreeTranslationMode, m_pApp->m_bIsPrinting, gbCheckInclFreeTransText);

}
#endif
	if (m_pApp->m_bFreeTranslationMode && !m_pApp->m_bIsPrinting && !gbCheckInclFreeTransText)
	{
		if (!gbSuppressSetup)
		{
			m_pApp->GetFreeTrans()->SetupCurrentFreeTransSection(m_pApp->m_nActiveSequNum);
		}
		wxTextCtrl* pEdit = (wxTextCtrl*)
							m_pMainFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
		pEdit->SetFocus();
		if (!m_pApp->m_pActivePile->GetSrcPhrase()->m_bHasFreeTrans)
		{
			pEdit->SetSelection(-1,-1); // -1, -1 selects it all
		}
		else
		{
			int len = pEdit->GetValue().Length();
			if (len > 0)
			{
				pEdit->SetSelection(len,len);
			}
		}
	}
	m_lastLayoutSelector = selector; // inform Draw() about what we did here
	return TRUE;
}

void CLayout::DoRecalcLayoutAfterPreferencesDlg()
{
	// do whatever kind of RecalcLayout is appropriate given whatever editing the user did
	// within the Preferences pages; it is possible that the user may have changed
	// settings in more than one page, for example, a font colour and perhaps a change of
	// punctuation, and so we have to ensure we cover such possibilities by calling
	// SetLayoutParameters() before whichever of the RecalcLayout() calls we make, and
	// especially before a Redraw() call, because without the colours being updated within
	// CLayout, any user colour changes won't appear in the view unless SetSrcColor(), etc
	// are called before Redraw() - and SetLayoutParameters() contains those color calls.
	// Note: the order of the blocks is important here: the most severe changes are tested
	// for first, then the lesser changes next, the minor changes to the layout last, and
	// anything not covered by those won't change the strips' pile populations and so a
	// Redraw() is done for those (eg. colour changes).
	SetLayoutParameters();
	if (m_bUSFMChanged || m_bFilteringChanged || m_bPunctuationChanged)
	{
		RecalcLayout(m_pApp->m_pSourcePhrases, create_strips_and_piles);
		return;
	}
	else if (m_bFontInfoChanged || m_bCaseEquivalencesChanged)
	{
		RecalcLayout(m_pApp->m_pSourcePhrases, create_strips_update_pile_widths);
		return;
	}
	else if (m_bViewParamsChanged)
	{
		RecalcLayout(m_pApp->m_pSourcePhrases, create_strips_keep_piles);
		return;
	}
	Redraw();
	// the flags will be cleared at the end of the view's OnEditPreferences() handler, and
	// also at the start of CEditPreferencesDlg::InitDialog(), and individually in various
	// handlers closer to the actual choice or non-choice of a given possible editing
	// change
}

// At the end of the vertical edit process, the phrase box may not be at the final active
// pile when the function OnCustomEventEndVerticalEdit() is called, but the movement of
// the phrase box into the gray area has updated the m_targetStr member of the
// CSourcePhrase instance at that pre-final active location, but the m_nMinWidth value of
// the CSourcePhrase will not have been updated - and if the target (or gloss) text in it
// is longer than the length of the source text, then the active strip won't have the
// correct pile offsets for all piles - the one at the pre-final active location will be
// wrong; so we call the document function ResetPartnerPileWidth on each CSourcePhrase
// instance in the active strip to ensure the values are uptodate when Draw() is called -
// the following function does this.
// The function currently is called only at the end of the OnCustomEventEndVerticalEdit()
// function, immediately prior to the Invalidate() call. However it is quite safe for
// calling at any other location where a user-editing operation's handler returns with the
// active strip not with all piles in it correctly updated for width - provided the app's
// m_nActiveSequNum value and the m_pActivePile value are set correctly prior to calling
// it.
void CLayout::RelayoutActiveStrip(CPile* pActivePile, int nActiveStripIndex, int gap,
								  int nStripWidth)
{
	wxASSERT(pActivePile != NULL);
	if (nActiveStripIndex < 0)
		return;
	CStrip* pActiveStrip = NULL;
	pActiveStrip = (CStrip*)m_stripArray.Item(nActiveStripIndex);
	wxASSERT(pActiveStrip);

	// we will lay them out again, so we clear m_arrPileOffsets and recalculate the
	// offsets using the pile pointers in m_arrPiles
	int count = pActiveStrip->m_arrPiles.GetCount();
	pActiveStrip->m_arrPileOffsets.Clear();
	int width = 0;
	int index = 0;
	int offset = 0;
	pActiveStrip->m_nFree = nStripWidth;
	int nextOffset;
	// first CPile instance is always at offset 0 in the strip
	CPile* pPile = (CPile*)pActiveStrip->m_arrPiles.Item(index);
	m_pDoc->ResetPartnerPileWidth(pPile->GetSrcPhrase()); // assumes m_nActiveSequNum
			// has been set correctly prior to RelayoutActiveStrip() being called
	if (pPile == pActivePile)
	{
		width = pPile->m_nWidth;
	}
	else
	{
		width = pPile->m_nMinWidth;
	}
	nextOffset = width + gap;
	pActiveStrip->m_arrPileOffsets.Add(offset);
	pActiveStrip->m_nFree -= width;
	// now do the rest of them
	for (index = 1; index < count; index++)
	{
		pPile = (CPile*)pActiveStrip->m_arrPiles.Item(index);
		m_pDoc->ResetPartnerPileWidth(pPile->GetSrcPhrase()); // assumes
			// m_nActiveSequNum has been set correctly prior to RelayoutActiveStrip()
			// being called
		if (pPile == pActivePile)
		{
			width = pPile->m_nWidth;
		}
		else
		{
			width = pPile->m_nMinWidth;
		}
		pActiveStrip->m_nFree -= gap + width;
		pActiveStrip->m_arrPileOffsets.Add(nextOffset);
		nextOffset += width + gap;
	}
}

///////////////////////////////////////////////////////////////////////////////////////
/// \return                     TRUE if background highlighting of a range of 
///                             auto-insertions is currently in effect, FALSE if that
///                             is not so or background highlighting is turned off
/// \param nStripCount          <-  returns a count of how many consecutive strips (the 
///                                 last one could be the active strip, and usually is)
///                                 are shown highlighted
/// \param bActivePileIsInLast  <-  returns TRUE if the active pile is within the strip 
///                                 with index nLast, FALSE it it is not (perhaps
///                                 it's the first pile in next strip)
/// \remarks
/// A completely new implementation is needed for the new way of supporting auto-insertion
/// hilighting, which uses a m_bAutoInserted boolean flag in CCell, rather than the legacy
/// approach which used a beginning and ending sequNum for a single contiguous range only.
/// Usage: only called in ScrollIntoView(), to supply information about how many
/// auto-inserted strips there are so as to help ScrollIntoView() to intelligently
/// position the phrase box in the vertical dimension.
/// BEW created 2010??
/// BEW refactored 9Apr12, to support discontinuous highlight spans when auto-inserting                            
///////////////////////////////////////////////////////////////////////////////////////                        
bool CLayout::GetHighlightedStripsRange(int& nStripCount, bool& bActivePileIsInLast)
{
	if (!AreAnyAutoInsertHighlightsPresent())
	{
		return FALSE;
	}
	
	// Get the index for the active strip...
	// First, get a default nActiveStrip index - use the last strip of the document
	int maxIndex = m_pApp->GetMaxIndex();
	CPile* pLastPile = m_pView->GetPile(maxIndex);
	int nActiveStrip = pLastPile->GetStripIndex(); // this will work if the phrase
							// box is beyond the doc end, i.e. is invisible

	bActivePileIsInLast = FALSE; // a default, and appropriate if the phrase box 
								 // is hidden when beyond the doc end
	if (m_pApp->m_nActiveSequNum != -1)
	{
		// there is a valid active pile
		CPile* pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);
		nActiveStrip = pActivePile->GetStripIndex();
		wxASSERT(nActiveStrip >= 0 && nActiveStrip <= 256000); // unlikely to have docs
			// bigger than about 256000 strips, but garbage values are likely to be larger
	}

	// Use stripIndicesArray to accumulate the unique strip indices of all piles having a
	// CCell[1] with m_bAutoInserted set to TRUE; if any match the nActiveStrip value,
	// preserve the index for it in the array in the local int nActiveLocationStrip_Index 
	wxArrayInt stripIndicesArray;
	stripIndicesArray.Clear();
	int nActiveLocationStrip_Index = -1; // if it remains -1, then there was no match

	CPile* pPile = NULL;
	PileList::Node* pos = m_pileList.GetFirst();
	while (pos != NULL)
	{
		pPile = pos->GetData();
		wxASSERT(pPile != NULL);
		CCell* pCell = pPile->GetCell(1); // depending on current mode, it could
										  // be an adaptation cell, or a gloss cell
		if (pCell->m_bAutoInserted)
		{
			int itsStripIndex = pPile->GetStripIndex();
			if (itsStripIndex == nActiveStrip)
			{
				// preserve which strip index has the active pile
				nActiveLocationStrip_Index = itsStripIndex;
			}
			// add it to the array if not already within it
			AddUniqueInt(&stripIndicesArray, itsStripIndex);
		}
		pos = pos->GetNext();
	}

    // when the loop finishes, the difference between the last and first strip indices
    // stored within it, plus one, gives the strip range (discontinuities may mean the
    // range is large, but that shouldn't matter); and if the last stored strip index is
    // equal to the nActiveLocationStrip_Index value, then the active location is in the
    // last strip having highlighted background in one or more piles
	int nFirstStripIndex = stripIndicesArray.Item(0);
	int nLastStripIndex = stripIndicesArray.Last(); // first and last could be the same value
	nStripCount = nLastStripIndex - nFirstStripIndex + 1;
	if (nLastStripIndex == nActiveLocationStrip_Index)
	{
		bActivePileIsInLast = TRUE;
	}
	stripIndicesArray.Clear();
	return TRUE;
}

wxArrayInt* CLayout::GetInvalidStripArray()
{
	return &m_invalidStripArray;
}

void CLayout::CreateStrips(int nStripWidth, int gap)
{
	int nIndexOfFirstPile = 0;
	wxASSERT(!m_pileList.IsEmpty());
	PileList::Node* pos = m_pileList.Item(nIndexOfFirstPile);
	wxASSERT(pos != NULL);
	CStrip* pStrip = NULL;

	 //loop to create the strips, add them to CLayout::m_stripArray
	int nStripIndex = 0;
	while (pos != NULL)
	{
		pStrip = new CStrip(this);
		pStrip->m_nStrip = nStripIndex; // set it's index
		m_stripArray.Add(pStrip); // add the new strip to the strip array
		// set up the strip's pile (and cells) contents

		//if (gbIsPrinting)
		//	wxLogDebug(_T("CreateStrips():  propulating strip with strip index  %d  ,  strip width  %d (pass in)"),
		//	 nStripIndex, nStripWidth);

		pos = pStrip->CreateStrip(pos, nStripWidth, gap);	// fill out with piles
		/*
		if (gbIsPrinting)
		{
			int nStripIndex = pStrip->m_nStrip;
			int free = pStrip->m_nFree;
			int numPiles = pStrip->m_arrPileOffsets.GetCount();
			wxLogDebug(_T("CreateStrip(): Num Piles %d  :  strip index %d, free %d"),numPiles, nStripIndex, free);
			int i;
			for (i=0;i<numPiles;i++)
			{
				wxLogDebug(_T("        index [%d] has offset %d , for srcPhrase:  %s"),
					i,pStrip->m_arrPileOffsets[i], ((CPile*)pStrip->m_arrPiles[i])->GetSrcPhrase()->m_srcPhrase);
			}
		}
		else
		{
			int nStripIndex = pStrip->m_nStrip;
			int free = pStrip->m_nFree;
			int numPiles = pStrip->m_arrPileOffsets.GetCount();
			wxLogDebug(_T("CreateStrip(): NOT PRINTING Num Piles %d  :  strip index %d, free %d"),numPiles, nStripIndex, free);
		}
		*/
		nStripIndex++;
	}
    // layout is built, we should call Shrink() to reclaim memory space unused
	m_stripArray.Shrink();
}

// starting from the passed in index value, update the index of succeeding strip instances
// to be in numerically ascending order without gaps
void CLayout::UpdateStripIndices(int nStartFrom)
{
	int nCount = m_stripArray.GetCount();
	int index = -1;
	//int newIndexValue = nStartFrom;
	CStrip* pStrip = NULL;
	if (nCount > 0)
	{
		for (index = nStartFrom; index < nCount; index++)
		{
			pStrip = (CStrip*)m_stripArray[index];
			//pStrip->m_nStrip = newIndexValue;
			pStrip->m_nStrip = index;
			//newIndexValue++;
		}
	}
}

int	CLayout::IndexOf(CPile* pPile)
{
	return m_pileList.IndexOf(pPile);
}


// the GetPile function also has equivalent member functions of the same name in the
// CAdapt_ItView and CAdapt_ItDoc classes, for convenience's sake; return the CPile
// instance at the given index, or NULL if the index is out of bounds
CPile* CLayout::GetPile(int index)
{
	int nCount = m_pileList.GetCount();
	if (index < 0 || index >= nCount)
	{
		// bounds error, so return NULL
		return (CPile*)NULL;
	}
	PileList::Node* pos = m_pileList.Item(index);
	wxASSERT(pos != NULL);
	return pos->GetData();
}

int CLayout::GetStripIndex(int nSequNum)
{
	PileList::Node* pos = m_pileList.Item(nSequNum); // relies on parallelism of
										// m_pSourcePhrases and m_pileList lists
	wxASSERT(pos != NULL);
	CPile* pPile = pos->GetData();
	return pPile->m_pOwningStrip->m_nStrip;
}

CStrip* CLayout::GetStrip(int nSequNum)
{
	PileList::Node* pos = m_pileList.Item(nSequNum); // relies on parallelism of
										// m_pSourcePhrases and m_pileList lists
	wxASSERT(pos != NULL);
	CPile* pPile = pos->GetData();
	return pPile->m_pOwningStrip;
}

CStrip* CLayout::GetStripByIndex(int index)
{
	return  (CStrip*)m_stripArray[index];
}

int CLayout::GetNumVisibleStrips()
{
	return m_numVisibleStrips; // how many strips can appear in the client area
}

int CLayout::CalcNumVisibleStrips()
{
	int clientHeight;
	wxSize canvasSize;
	canvasSize = m_pApp->GetMainFrame()->GetCanvasClientSize();
	clientHeight = canvasSize.GetHeight(); // see lines 2425-2435 of
			// Adapt_ItCanvas.cpp and then lines 2454-56 for stuff below
	int nVisStrips = clientHeight / (m_nStripHeight + m_nCurLeading);
	int partStrip = clientHeight % (m_nStripHeight + m_nCurLeading); // modulo
	if (partStrip > 0)
		nVisStrips++;
	return nVisStrips; // how many strips can appear in the client area
}


void CLayout::GetVisibleStripsRange(wxDC* pDC, int& nFirstStripIndex, int& nLastStripIndex)
{
	// BE SURE TO HANDLE active sequ num of -1 --> make it end of doc, but
	// hide box  -- I think we'll support it by just recalculating the layout without a
	// scroll, at the current thumb position for the scroll car

    // get the logical distance (pixels) that the scroll bar's thumb indicates to top
    // of client area
	int nThumbPosition_InPixels = pDC->DeviceToLogicalY(0);

    // for the current client rectangle of the canvas, calculate how many strips will
    // fit - a part strip is counted as an extra one
	int nVisStrips = GetNumVisibleStrips();

	// find the current total number of strips
	int nTotalStrips = m_stripArray.GetCount();

 	// estimate the index to start scanning forward from in order to find the first strip
	// which has some content visible in the client area (do it by a binary chop)
	int nIndexToStartFrom = GetStartingIndex_ByBinaryChop(nThumbPosition_InPixels,
															nVisStrips, nTotalStrips);
   // find the index of the first strip which has some content visible in the client
    // area, that is, the first strip which has a bottom coordinate greater than
    // nThumbPosition_InPixels
	int index = nIndexToStartFrom;
	int bottom;
	CStrip* pStrip;
	do {
		pStrip = (CStrip*)m_stripArray.Item(index);
		bottom = pStrip->Top() + GetStripHeight(); // includes free trans height if
												   // free trans mode is ON
		if (bottom > nThumbPosition_InPixels)
		{
			// this strip is at least partly visible - so start drawing at this one
			break;
		}
		index++;
	} while(index < nTotalStrips);
	wxASSERT(index < nTotalStrips);
	nFirstStripIndex = index;

    // use nVisStrips to get the final visible strip (it may be off-window, but we
    // don't care because it will be safe to draw it off window)
	nLastStripIndex = nFirstStripIndex + (nVisStrips - 1);
	if (nLastStripIndex > nTotalStrips - 1)
		nLastStripIndex = nTotalStrips - 1; // protect from bounds error

    // check the bottom of the last visible strip is lower than the bottom of the
    // client area, if not, add an additional strip
	pStrip = (CStrip*)m_stripArray.Item(nLastStripIndex);
	bottom = pStrip->Top() + GetStripHeight();
	if (!(bottom >= nThumbPosition_InPixels + GetClientWindowSize().y ))
	{
		// add an extra one, if there is an extra one to add - it won't hurt to write
		// one strip partly or wholely off screen
		if (nLastStripIndex < nTotalStrips - 1)
			nLastStripIndex++;
	}
}
// the following is an overloaded version which creates a device context internally rather
// than relying on one being passed from the caller
void CLayout::GetVisibleStripsRange(int& nFirstStripIndex, int& nLastStripIndex)
{
	// BE SURE TO HANDLE active sequ num of -1 --> make it end of doc, but
	// hide box  -- I think we'll support it by just recalculating the layout without a
	// scroll, at the current thumb position for the scroll car

	wxClientDC aDC((wxWindow*)m_pCanvas); // make a device context
	m_pCanvas->DoPrepareDC(aDC); // get origin adjusted
	wxClientDC* pDC = &aDC;

    // get the logical distance (pixels) that the scroll bar's thumb indicates to top
    // of client area
	int nThumbPosition_InPixels = pDC->DeviceToLogicalY(0);

    // for the current client rectangle of the canvas, calculate how many strips will
    // fit - a part strip is counted as an extra one
	int nVisStrips = GetNumVisibleStrips();

	// find the current total number of strips
	int nTotalStrips = m_stripArray.GetCount();

 	// estimate the index to start scanning forward from in order to find the first strip
	// which has some content visible in the client area (do it by a binary chop)
	int nIndexToStartFrom = GetStartingIndex_ByBinaryChop(nThumbPosition_InPixels,
															nVisStrips, nTotalStrips);
   // find the index of the first strip which has some content visible in the client
    // area, that is, the first strip which has a bottom coordinate greater than
    // nThumbPosition_InPixels
	int index = nIndexToStartFrom;
	int bottom;
	CStrip* pStrip;
	do {
		pStrip = (CStrip*)m_stripArray.Item(index);
		bottom = pStrip->Top() + GetStripHeight(); // includes free trans height if
												   // free trans mode is ON
		if (bottom > nThumbPosition_InPixels)
		{
			// this strip is at least partly visible - so start drawing at this one
			break;
		}
		index++;
	} while(index < nTotalStrips);
	wxASSERT(index < nTotalStrips);
	nFirstStripIndex = index;

    // use nVisStrips to get the final visible strip (it may be off-window, but we
    // don't care because it will be safe to draw it off window)
	nLastStripIndex = nFirstStripIndex + (nVisStrips - 1);
	if (nLastStripIndex > nTotalStrips - 1)
		nLastStripIndex = nTotalStrips - 1; // protect from bounds error

    // check the bottom of the last visible strip is lower than the bottom of the
    // client area, if not, add an additional strip
	pStrip = (CStrip*)m_stripArray.Item(nLastStripIndex);
	bottom = pStrip->Top() + GetStripHeight();
	if (!(bottom >= nThumbPosition_InPixels + GetClientWindowSize().y ))
	{
		// add an extra one, if there is an extra one to add - it won't hurt to write
		// one strip partly or wholely off screen
		if (nLastStripIndex < nTotalStrips - 1)
			nLastStripIndex++;
	}
}

int CLayout::GetStartingIndex_ByBinaryChop(int nThumbPos_InPixels, int numVisStrips,
										   int numTotalStrips)
{
	// "upper" partition is the one with small sequence number, "lower" has larger
	// sequence numbers
	if (numTotalStrips < 64)
		// don't need a binary chop if there are not heaps of strips
		return 0;
	int maxStripIndex = numTotalStrips - 1;
	int nOneScreensWorthOfStrips = numVisStrips + 2; // LHS = usually about 12 or so
	int nIndexToStartFrom = 0;
	CStrip* pStrip = NULL;
	int nLowerBound = nIndexToStartFrom;
	int nUpperBound = maxStripIndex;
	int nTop = 0;
	int midway = 0;
	int half = 0;
	while ((nUpperBound - nLowerBound) > nOneScreensWorthOfStrips)
	{
		half = (nUpperBound - nLowerBound)/2;
		midway = nLowerBound + half;
		pStrip = (CStrip*)m_stripArray.Item(midway);
		nTop = pStrip->Top();
		if (nThumbPos_InPixels <= nTop)
		{
			// thumb is in upper partition
			nUpperBound = midway;
		}
		else
		{
			// thumb is in lower partition
			nLowerBound = midway;
		}
		nIndexToStartFrom = nLowerBound;
	}
	return nIndexToStartFrom;
}

void CLayout::GetInvalidStripRange(int& nIndexOfFirst, int& nIndexOfLast,
								   int nCountValueForTooFar)
{
	// always get the active strip - it may or may not be within the strips whole indices
	// are in the m_invalidStripArray, but we'll include it in the calculations to be sure
	// the layout is correct everwhere related to the user's last editing operation (the
	// active strip may stay the active strip, or it may not, and we don't want to waste
	// time trying to figure out which is the case)
	int nIndexOfActiveStrip = -1;
	int nIndexOfFirst_Active = -1; // indicates no value was set (caller can check for != -1)
	int nIndexOfLast_Active = -1;  // ditto
	if (m_pApp->m_pActivePile != NULL)
	{
		// there will be an active pile except when the phrase box is hidden because the
		// user advanced the box into limbo past the end of the document; but if for some
		// reason the active pile's m_pOwningStrip pointer was not set, then -1 will be
		// returned
		// NOTE: in the event of an Unmerge operation, the active pile was the one that
		// got unmerged (and hence destroyed and its memory freed) - which; the
		// RestoreOriginalMinPhrases() function inserts the old CSourcePhrase instances
		// back into the app's m_pSourcePhrases list, and creates partner piles for these
		// with CAdapt_ItDoc::CreatePartnerPile() calls. The latter does not know about
		// what strip it will end up in, nor what position in that strip, because when
		// these creations are done the old strips are current (we could have a guess and
		// probably set the strip pointer correctly if the old strips exist, but not reliably
        // set the index within the strip's m_arrPiles array, and sometimes pile creation
        // is done when all strips are destroyed for a full layout rebuild, so there is not
        // much point in trying) - so we only go as far as having RestoreOriginalMinPhrases()
        // point the CAdapt_It::m_pActivePile at the partner pile for the first of the
		// created original minimum CSourcePhrase instances we've replaced in the list -
		// knowing full well that its m_pOwningStrip value will be NULL, and its m_nPile
		// value will be -1. That means that until the strips are updated, those members
		// will have those values, which means any function which depends on them before
		// RecalcLayout() has finished must know what to do if such a pile is the active
		// one - for instance, calling CPile::GetStripIndex() will return the invalid
		// index -1, and any attempt to Draw() such a pile would fail because m_nPile is
		// accessed in order to find its location in a strip in order to work out its
		// drawing rectangle, and garbage would be being accessed. So in the stuff below
		// we must allow for an Unmerge operation - when it happens, we'll just not try to
		// get the active location information, but rely on the strip indices in
		// CLayout::m_invalidStripArray instead.
		nIndexOfActiveStrip = m_pApp->m_pActivePile->GetStripIndex(); // returns -1 if
		// the active pile is one newly created (eg. at an Unmerge operation)
		// (because it has its m_pOwningStrip member set to NULL); otherwise returns a
		// valid strip index
	}
	bool bActiveStripSet = TRUE;
	if (nIndexOfActiveStrip != -1)
	{
		nIndexOfFirst_Active = nIndexOfActiveStrip;
		nIndexOfLast_Active = nIndexOfActiveStrip;
	}
	else
		bActiveStripSet = FALSE;

	int nFirst = -1;
	int nLast = -1;
	int index = 0;
	if (m_invalidStripArray.IsEmpty())
	{
		nIndexOfFirst = nIndexOfFirst_Active;
		nIndexOfLast = nIndexOfLast_Active;
		return;
	}
	else
	{
		int nCount = m_invalidStripArray.GetCount();
		wxASSERT(nCount >= 1);
		int aValue = m_invalidStripArray.Item(index);
		nFirst = aValue;
		nLast = aValue;
		for (index = 1; index < nCount; index++)
		{
			aValue = m_invalidStripArray.Item(index);
			if (aValue > nLast)
			{
				nLast = aValue;
				continue;
			}
			if (aValue < nFirst)
			{
				nFirst = aValue;
			}
		}
	}
	// now get the final range...
	if (nFirst == -1)
	{
        // the calculations with m_invalidStripArray yielded nothing, so either nothing
        // worked and so return -1 values to signal to caller to not use what is returned;
        // or we got the active strip, and return its index
		nIndexOfFirst = nIndexOfFirst_Active; // = -1 or the index of the active strip
		nIndexOfLast = nIndexOfLast_Active; // = -1 or the index of the active strip
		return;
	}
	else if (abs(nLast - nFirst) > nCountValueForTooFar)
	{
		// active region and where the box was are far far apart, so use the active
		// location only
		if (bActiveStripSet)
		{
			nIndexOfFirst = nIndexOfFirst_Active; // the index of the active strip
			nIndexOfLast = nIndexOfLast_Active;  // or the index of the active strip
		}
		else
		{
			// no active strip was set, (this can happen at an Unmerge operation because
			// the recreated partner piles have their m_pOwningStrip member set to NULL),
			// so we must do what we can with the huge range we've calculated - we'll just
			// ignore it and instead use the visible strips, hoping that that will cover
			// the area needing to be relaid out
a:			int nFirstStripIndex = -1;
			int nLastStripIndex = -1;
			GetVisibleStripsRange(nFirstStripIndex, nLastStripIndex);
			nIndexOfFirst = nFirstStripIndex;
			nIndexOfLast = nLastStripIndex;
		}
		return;
	}
	else // active loc and user edits area are locations close to each other
	{
		// we got an index range from m_invalidStripArray, so if the active strip is
		// within it, just return that range; but if the active strip is earlier or beyond
		// it, then extend the strip range to include the active strip
		if (!bActiveStripSet)
			goto a; // if we didn't get an active strip, return the
					// range of visible strips instead
		// we've got a valid active strip, so use it in the calculations below
		nIndexOfFirst = nFirst;
		nIndexOfLast = nLast;
		if (nIndexOfFirst_Active < nIndexOfFirst)
		{
			if ((nIndexOfFirst - nIndexOfFirst_Active) > nCountValueForTooFar)
				return; // it's too far away from the edit location for us to be bothered,
                        // so return the values we have found already and ignore the active
                        // strip
			// otherwise, extend the index range to include the active strip
			nIndexOfFirst = nIndexOfFirst_Active;
			return;
		}
		if (nIndexOfLast_Active > nIndexOfLast)
		{
			if ((nIndexOfLast_Active - nIndexOfLast) > nCountValueForTooFar)
				return; // it's too far away from the edit location for us to be bothered,
                        // so return the values we have found already and ignore the active
                        // strip
			// otherwise, extend the index range to include the active strip
			nIndexOfLast = nIndexOfLast_Active;
		}
	}
}

void CLayout::SetupCursorGlobals(wxString& phrase, enum box_cursor state,
								 int nBoxCursorOffset)
{
    // set the m_nStartChar and m_nEndChar cursor/selection globals prior to drawing the
    // phrase box

	// get the cursor set
	switch (state)
	{
	case select_all:
		{
			//m_pApp->m_nStartChar = 0; // MFC
			m_pApp->m_nStartChar = -1; // wxWidgets
			m_pApp->m_nEndChar = -1;
			break;
		}
	case cursor_at_text_end:
		{
			int len = phrase.Len();
			m_pApp->m_nStartChar = len;
			m_pApp->m_nEndChar = len;
			break;
		}
	case cursor_at_offset:
		{
			wxASSERT(nBoxCursorOffset >= 0);
			m_pApp->m_nStartChar = nBoxCursorOffset;
			m_pApp->m_nEndChar = m_pApp->m_nStartChar;
			break;
		}
	default: // do this if a garbage value is passed in
		{
			//m_pApp->m_nStartChar = 0;
			m_pApp->m_nStartChar = -1; // WX uses -1, not 0 as in MFC
			m_pApp->m_nEndChar = -1;
			break;
		}
	}
}

// Detect if the user's edit area is not adequately defined, in which case reenter
// RecalcLayout() to rebuild the doc by destroying and recreating all strips and return
// FALSE to the caller so that it can return FALSE to the original RecalcLayout() call so
// it can exit without doing any more work; otherwise return TRUE when the user's editing
// area is correctly defined
bool CLayout::GetPileRangeForUserEdits(int nFirstInvalidStrip, int nLastInvalidStrip,
										int& nFirstPileIndex, int& nEndPileIndex)
{
	if (nFirstInvalidStrip == -1 || nLastInvalidStrip == -1)
	{
        // a range of invalid strips was not calculated by the caller, so reenter
        // RecalcLayout() doing a full strip destroy and recreation, so that we get a valid
        // layout that way instead
		RecalcLayout(m_pApp->m_pSourcePhrases,create_strips_keep_piles);
		m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);
		return FALSE;
	}

	int nBeforeStripIndex = nFirstInvalidStrip - 1; // previous strip's index, or -1
	if (nFirstInvalidStrip == 0)
	{
		// there is no context preceding the user's editing area, so the first pile
		// needing to be placed in the emptied strips is the one at the start of the
		// document
		nFirstPileIndex = 0;
	}
	else
	{
		CStrip* pBeforeStrip = (CStrip*)m_stripArray.Item(nBeforeStripIndex);
		CPile* pBeforePile = (CPile*)pBeforeStrip->m_arrPiles.Last();
		wxASSERT(pBeforePile);
		nFirstPileIndex = m_pileList.IndexOf(pBeforePile) + 1;
	}

	int nAfterStripIndex = nLastInvalidStrip + 1; // following strip's index, or bounds error
	if (nAfterStripIndex > (int)(m_stripArray.GetCount() - 1))
	{
		// the "after" strip does not exist, therefore the last invalid strip is also the
		// last strip in the document; so the last pile needing to be placed must be
		// whatever is not last in m_pileList
		nEndPileIndex = m_pileList.GetCount() - 1;
		wxASSERT(nEndPileIndex >= 0);
	}
	else
	{
		// there is an "after" strip, so we can get it's first pile as the bounding one,
		// and so the last pile to be placed will be the pile immediately preceding that.
		// (we search for the bounding pile, because the old pile at the end of the
		// invalid strips may no longer exist, so we can't assume a search for it will
		// succeed)
		CStrip* pAfterStrip = (CStrip*)m_stripArray.Item(nAfterStripIndex);
		CPile* pFirstAfterPile = (CPile*)pAfterStrip->m_arrPiles.Item(0); // this one is unedited
		wxASSERT(pFirstAfterPile);
		nEndPileIndex = m_pileList.IndexOf(pFirstAfterPile) - 1;
		wxASSERT(nEndPileIndex >= 0);
	}
	return TRUE;
}

// Returns: a count of how many strips were emptied
// This function empties the pile pointers stored in the invalid strips, and the array
// containing the offsets from the strip's boundary (left for LTR text, right for RTL
// text), but it does not alter any of the other strip members except to reset the m_nFree
// member to the nStripWidth value ready for pile points to be added later
// BEW changed 20Jun09, adding content to the doc end using Edit Source Text command can
// result in a strip index which is higher than the (now possibly reduced) strip count
// less one, and so we have to test for this to avoid a crash due to a bounds error
int CLayout::EmptyTheInvalidStrips(int nFirstStrip, int nLastStrip, int nStripWidth)
{
	int nCount = 0;
	int index;
	CStrip* pStrip = NULL;
	int maxStripIndex = m_stripArray.GetCount() - 1;
	if (nLastStrip > maxStripIndex)
		nLastStrip = maxStripIndex;
	if (nLastStrip < nFirstStrip)
		return 0;
	for (index = nFirstStrip; index <= nLastStrip; index++)
	{
		pStrip = (CStrip*)m_stripArray.Item(index);
		wxASSERT(pStrip);
		pStrip->m_nFree = nStripWidth;
		pStrip->m_arrPiles.Clear();
		pStrip->m_arrPileOffsets.Clear();
		nCount++;
	}
	return nCount;
}

/////////////////////////////////////////////////////////////////////////////////***
/// \return   a count of the final number of rebuild strips  -  this could be fewer, the
///           same, or more than the value of nInitialStripCount passed in
/// \param
/// nFirstStrip       <-> input: ref to index of first invalid strip (it is now emptied of piles)
///                       output: same as value input, refilling doesn't affect this value
/// nLastStrip        <-> input: ref to index of last invalid strip (it is now emptied of piles)
///                       output: index of last rebuilt strip (it could be less than, the same,
///                       or greater than what was input - depending on whether pile widths
///                       have changed, and how many there are to be placed
/// nStripWidth       ->  used for initializing m_nFree member of CStrip - we will need this
///                       if we have to create one or more extra strips to accomodate all the
///                       new CPile pointers to be placed in the rebuilt strips
/// gap               ->  the interpile gap (in pixels) - we need this for filling the emptied
//                         strips with the appropriate gap between each pile
/// nFirstPileIndex   ->  index to the CPile pointer for the pile in CLayout::m_pileList which
///                       is the first in the user's editing area, and hence the first
///                       one to be "placed" in the emptied range of strips
/// nEndPileIndex     ->  index to the last CPile pointer for the pile in CLayout::m_pileList
///                       which is the last in the user's editing area, and hence the last
///                       one to be "placed" in the emptied range of strips
/// nInitialStripCount -> count of how many strips were emptied by the last function call
///                       in the caller; we use this to initialize the return value
/// \remarks
/// The function fills the strips with the pile pointers resulting from the user's edits,
/// adding strips if necessary, or removing empty strips left over when done. Each completed
/// strip is marked m_bValid = TRUE, but the last is marked m_bValid = TRUE only if we check
/// and find that the pile which foloows is too wide to fit in that strip, or if we are at the
/// end of the document, otherwise it is left with a FALSE value. (When Draw() sees a strip
/// with m_bValid == FALSE and the strip is in the visible area of the view, it will attempt
/// to flow one or more piles up to fill the strip, and similarly for strips lower down but
/// visible and which become invalid because they lose one or more piles due to the upward
/// flow to fill the strip above, iterating until it comes to an off-view strip - at which
/// point it leaves that strip marked as invalid - and it remains so until at some later
/// time if becomes visible and drawn, or a full destroy all strips and rebuild is done due
/// to some operation which requires it (such as changing the font size, etc).
/////////////////////////////////////////////////////////////////////////////////***
int CLayout::RebuildTheInvalidStripRange(int nFirstStrip, int nLastStrip, int nStripWidth,
		 int gap, int nFirstPileIndex, int nEndPileIndex, int nInitialStripCount)
{
	int nFinalStripCount = nInitialStripCount; // initialize to the value passed in
	int nStripCount = 0;
	int stripIndex = nFirstStrip;
	int nNumberRemoved = 0;
	int nNumberAdded = 0;
	int pileIndex = nFirstPileIndex;

	// initialize the strip pointer to the one about to be filled
	CStrip* pStrip = (CStrip*)m_stripArray.Item(stripIndex);
	wxASSERT(pStrip);

#ifdef _13STRIPS_DOC
	#ifdef _DEBUG
		{
			int indices[26];
			int count = m_stripArray.GetCount();
			int anIndex;
			for (anIndex = 0; anIndex < count; anIndex++)
			{
				//CStrip* theStripPtr = (CStrip*)m_stripArray.Item(anIndex);
				indices[anIndex] = ((CStrip*)m_stripArray.Item(anIndex))->GetStripIndex();
			}
			wxLogDebug(_T("	Rebuild: 1. strip count %d , strip indices: [0] %d [1] %d [2] %d [3] %d [4] %d [5] %d [6] %d [7] %d [8] %d [9] %d [10] %d [11] %d [12] %d"),
				count,indices[0],indices[1],indices[2],indices[3],indices[4],indices[5],indices[6],indices[7],indices[8],indices[9],indices[10],indices[11],indices[12]);
		}
	#endif
#endif
	// loop over the range of CPile pointers to be copied to the emptied strips - adding
	// strips if necessary; count any that get added
	int nNumberPlaced = 0;
	int nNumberPlacedAtThisCall = 0;
	while (pileIndex <= nEndPileIndex)
	{
		// fill the one or more strips which have been emptied
		nNumberPlacedAtThisCall = pStrip->CreateStrip(pileIndex, nEndPileIndex,
														nStripWidth, gap);
		// DebugIndexMismatch(111, 101);
		nNumberPlaced += nNumberPlacedAtThisCall;
		nStripCount++; // count this now-refilled strip
		pileIndex += nNumberPlacedAtThisCall; // update pileIndex to point at the first
											  // to be placed at the next iteration
#ifdef _13STRIPS_DOC
		#ifdef _DEBUG
			{
				int indices[26];
				int count = m_stripArray.GetCount();
				int anIndex;
				for (anIndex = 0; anIndex < count; anIndex++)
				{
					indices[anIndex] = ((CStrip*)m_stripArray.Item(anIndex))->GetStripIndex();
				}
				wxLogDebug(_T("	Rebuild: 2. strip count %d , strip indices: [0] %d [1] %d [2] %d [3] %d [4] %d [5] %d [6] %d [7] %d [8] %d [9] %d [10] %d [11] %d [12] %d"),
					count,indices[0],indices[1],indices[2],indices[3],indices[4],indices[5],indices[6],indices[7],indices[8],indices[9],indices[10],indices[11],indices[12]);
			}
		#endif
#endif
		// for the CPile instances in the user's edit area, have we placed them all?
		if (pileIndex > nEndPileIndex)
		{
            // we've placed them all, break out and check for leftover empty strips, etc
            break;
		}
		else
		{
            // there is at least one more to be placed. However, we may have just filled
            // the last of the strips that we emptied, in which case we need to insert
            // another strip, so check for this and do so, otherwise set pStrip to the next
            // strip to be filled and iterate the loop after augmenting stripIndex
			if (nStripCount >= nInitialStripCount)
			{
				// we have to create a new CStrip instance on the heap to accomodate more
				// of the piles which remain for placement
				pStrip = NULL;
				pStrip = new CStrip(this);
				wxASSERT(pStrip);
				int currentMaximumStripIndex = m_stripArray.GetCount() - 1;
				//DebugIndexMismatch(111, 102);
				if (stripIndex == currentMaximumStripIndex)
				{
					// the new strip will be at the document's end, so we append it to the
					// strip array and set the new strip's m_nStrip index member
					pStrip->m_nStrip = currentMaximumStripIndex + 1;
					m_stripArray.Add(pStrip); // append the new strip to the strip array
#ifdef _13STRIPS_DOC
					#ifdef _DEBUG
						{
							int nNewStripCount = m_stripArray.GetCount();
							wxLogDebug(_T("	Rebuild: pre-3. ADDED a strip, and piles remaining are: pileIndex %d to nEndPileIndex %d inclusive, new strip count %d"),
										pileIndex, nEndPileIndex, nNewStripCount);
						}
					#endif
#endif
				}
				else
				{
					// the new strip is not at the document's end, so we must get the next
					// strip's index and call Insert with it, to insert the new strip we
					// just created at the appropriate place - and then set the m_nStrip
					// member
					int nIndexOfNextStrip = stripIndex +1;
					pStrip->m_nStrip = nIndexOfNextStrip; // we renumber relevant strips
														  // consecutively later on
					m_stripArray.Insert(pStrip,nIndexOfNextStrip); // inserts before
#ifdef _13STRIPS_DOC
					#ifdef _DEBUG
						{
							int nNewStripCount = m_stripArray.GetCount();
							wxLogDebug(_T("	Rebuild: pre-3. INSERTED a strip, and piles remaining are: pileIndex %d to nEndPileIndex %d inclusive, new strip count %d  inserted before %d"),
									pileIndex, nEndPileIndex, nNewStripCount, nIndexOfNextStrip);
						}
					#endif
#endif
				}
				nNumberAdded++; // update count of how many strips have been added
			}
			else
			{
				// we have not yet filled all the strips we emptied, so get the next one
				// in order to be ready for next iteration of the loop (most of the time
				// control passes through this block)
				pStrip = (CStrip*)m_stripArray.Item(stripIndex + 1);
				wxASSERT(pStrip);
#ifdef _13STRIPS_DOC
				#ifdef _DEBUG
					{
						int nNewStripCount = m_stripArray.GetCount();
						wxLogDebug(_T("	Rebuild: loop end, pre-3. About to ITERATE pile placement loop; pileIndex %d to nEndPileIndex %d inclusive, for strip index %d of strip count %d"),
								pileIndex, nEndPileIndex, stripIndex + 1, nNewStripCount);
					}
				#endif
#endif
			}
			stripIndex++;
#ifdef _13STRIPS_DOC
			#ifdef _DEBUG
				{
					int indices[26];
					int count = m_stripArray.GetCount();
					int anIndex;
					for (anIndex = 0; anIndex < count; anIndex++)
					{
						indices[anIndex] = ((CStrip*)m_stripArray.Item(anIndex))->GetStripIndex();
					}
					wxLogDebug(_T("	Rebuild: 3. strip count %d , strip indices: [0] %d [1] %d [2] %d [3] %d [4] %d [5] %d [6] %d [7] %d [8] %d [9] %d [10] %d [11] %d [12] %d   stripIndex %d"),
						count,indices[0],indices[1],indices[2],indices[3],indices[4],indices[5],indices[6],indices[7],indices[8],indices[9],indices[10],indices[11],indices[12], stripIndex);
					int nExtras = count - 13;
					if (nExtras > 0)
					{
						for (anIndex = 13; anIndex < count; anIndex++)
						{
							indices[anIndex] = ((CStrip*)m_stripArray.Item(anIndex))->GetStripIndex();
							wxLogDebug(_T("	Rebuild: 3. Extras: strip with index: [%d] %d "),anIndex, indices[anIndex]);
						}
					}
				}
			#endif
#endif
		}
	} // end of while loop

#ifdef _13STRIPS_DOC
	#ifdef _DEBUG
		{
			int indices[26];
			int count = m_stripArray.GetCount();
			int anIndex;
			for (anIndex = 0; anIndex < count; anIndex++)
			{
				indices[anIndex] = ((CStrip*)m_stripArray.Item(anIndex))->GetStripIndex();
			}
			wxLogDebug(_T("	Rebuild: 4. strip count %d , strip indices: [0] %d [1] %d [2] %d [3] %d [4] %d [5] %d [6] %d [7] %d [8] %d [9] %d [10] %d [11] %d [12] %d"),
				count,indices[0],indices[1],indices[2],indices[3],indices[4],indices[5],indices[6],indices[7],indices[8],indices[9],indices[10],indices[11],indices[12]);
		}
	#endif
#endif
	// if there are empty strips left over, remove them and count how many removed; set the
    // value for nFinalStripCount
	int nIndexOfLastStripFilled = nFirstStrip + nStripCount - 1;
	CStrip* pLastStripFilled = (CStrip*)m_stripArray.Item(nIndexOfLastStripFilled);
	wxASSERT(pLastStripFilled);
#ifdef _13STRIPS_DOC
	#ifdef _DEBUG
		{
			wxLogDebug(_T("	Rebuild: 4.1   nIndexOfLastStripFilled %d   nLastStrip %d  Deletes if nIndexOfLastStripFilled < nLastStrip"),
							nIndexOfLastStripFilled, nLastStrip);
		}
	#endif
#endif

	if (nIndexOfLastStripFilled < nLastStrip)
	{
		//DebugIndexMismatch(111, 104);
		// we didn't fill all the emptied strips, so remove the remainders which are empty
		CStrip* pStripForRemoval = NULL;
		int index = nIndexOfLastStripFilled + 1; // will be constant, higher items shift down
		int iterator;
		for (iterator = nIndexOfLastStripFilled + 1; iterator <= nLastStrip; iterator++)
		{
			pStripForRemoval = (CStrip*)m_stripArray.Item(index);
			wxASSERT(pStripForRemoval);

			// only delete it if it really is an emptied strip (Note: the emptying is done
			// by a function in the caller, even though we have strip emptying calls at the
			// start of RebuildTheInvalidStripRange() itself - but any strip not filled will
			// not be emptied by those calls, and so if pStripForRemoval is not an emptied
			// strip, it must not be removed - otherwise later accesses to its piles would
			// cause the app to crash because their m_pOwningStrip would point at freed
			// memory
			if (pStripForRemoval->m_arrPiles.IsEmpty() &&
											pStripForRemoval->m_arrPileOffsets.IsEmpty())
			{
#ifdef _13STRIPS_DOC
				#ifdef _DEBUG
					{
						wxLogDebug(_T("	Rebuild: 4.2  ** DELETING ** strip[ %d ]"),  pStripForRemoval->m_nStrip);
					}
				#endif
#endif
				if (pStripForRemoval != NULL) // whm 11Jun12 added NULL test
					delete pStripForRemoval;
				m_stripArray.RemoveAt(index);
				nNumberRemoved++;
				//DebugIndexMismatch(111, 105);
			}
		}
	}
	nFinalStripCount += nNumberAdded - nNumberRemoved; // return this to the caller

#ifdef _13STRIPS_DOC
	#ifdef _DEBUG
		{
			int indices[26];
			CStrip* pointers[13];
			int count = m_stripArray.GetCount();
			int anIndex;
			for (anIndex = 0; anIndex < count; anIndex++)
			{
				indices[anIndex] = ((CStrip*)m_stripArray.Item(anIndex))->GetStripIndex();
				pointers[anIndex] = (CStrip*)m_stripArray.Item(anIndex);
			}
			wxLogDebug(_T("	Rebuild: 5. strip count %d , strip indices: [0] %d [1] %d [2] %d [3] %d [4] %d [5] %d [6] %d [7] %d [8] %d [9] %d [10] %d [11] %d [12] %d"),
				count,indices[0],indices[1],indices[2],indices[3],indices[4],indices[5],indices[6],indices[7],indices[8],indices[9],indices[10],indices[11],indices[12]);
			wxLogDebug(_T("	Rebuild: 5.1 strip POINTERS: [0] %d [1] %d [2] %d [3] %d [4] %d [5] %d [6] %d [7] %d [8] %d [9] %d [10] %d [11] %d [12] %d"),
				pointers[0],pointers[1],pointers[2],pointers[3],pointers[4],pointers[5],pointers[6],pointers[7],pointers[8],pointers[9],pointers[10],pointers[11],pointers[12]);
		}
	#endif
#endif
	// check the final strip; first, if it is the document's final strip, mark it m_bValid
	// = TRUE; otherwise, check to see if it has no room for the document's next CPile
	// pointer (ie. the first in the strip which follows the edited area) - if so, mark
	// the strip m_bValid = TRUE; otherwise, it has room and is not last in the document,
	// so mark it m_bValid = FALSE (the Draw() function may then flow piles up into it if
	// that strip gets to be in the visible part of the view)
	int nLastStripInDocument = m_stripArray.GetCount() - 1;
	if (nLastStripInDocument == nIndexOfLastStripFilled)
	{
		// this strip has to be marked valid, whether filled or not, ensure it is so...
		pLastStripFilled->m_bValid = TRUE;
	}
	else
	{
        // this is not the last strip in the document, and so we expect a CPile pointer
        // exists in a following strip, so check and see if there is room for a following
        // pile to be later moved up to the end of the last strip filled - if so, mark the
        // strip m_bValid = FALSE, but if not enough room, mark it m_bValid = TRUE
        CPile* pAfterPile = NULL;
		int nextStrip = nIndexOfLastStripFilled + 1;
		pStrip = (CStrip*)m_stripArray.Item(nextStrip);
		wxASSERT(pStrip);
		pAfterPile = (CPile*)pStrip->m_arrPiles.Item(0);
		wxASSERT(pAfterPile);
		if (pAfterPile != NULL)
		{
			// this "after" pile is outside the user's edits, and we'd expect it would not
			// be the active pile, but we cannot assume so because the phrase box might
			// get placed here because the editing involved a retranslation and this pile
			// was calculated as the safe place to locate the box, so we must check if it
			// is the active location and if so, use the larger width which can accomodate
			// the extent of the phrase box, m_nWidth, rather than the minimum width
			// m_nMinWidth which is based on the max width of the text in the cells
			// DebugIndexMismatch(111, 106);
			int sequNumOfBoundingPile = pAfterPile->m_pSrcPhrase->m_nSequNumber;
			int pileWidth = 40; // default
			if (sequNumOfBoundingPile == m_pApp->m_nActiveSequNum)
			{
				// this is the active location, so get the phrase box's "gap with" instead
				pileWidth = pAfterPile->CalcPhraseBoxGapWidth();
			}
			else
			{
				// this is not the active location, so use the m_nMinWidth valuee
				pileWidth = pAfterPile->m_nMinWidth;
			}
			if (pileWidth <= pLastStripFilled->m_nFree)
			{
				// flow of the bounding pile upwards to the end of this last filled strip
				// is possible, so mark it m_bValid = FALSE
				pLastStripFilled->m_bValid = FALSE;
			}
			else
			{
				// cannot flow the pile up to the end of this last filled strip, so mark
				// the strip valid
				pLastStripFilled->m_bValid = TRUE;
			}
		}
		else
		{
			// there are not any more piles, so this strip has to be valid
			pLastStripFilled->m_bValid = TRUE;
		}
	}
#ifdef _13STRIPS_DOC
	#ifdef _DEBUG
		{
			if (pLastStripFilled->m_bValid)
				wxLogDebug(_T("	Rebuild: 6. Last filled strip is VALID,  Final count of refilled strips is %d"),nFinalStripCount);
			else
				wxLogDebug(_T("	Rebuild: 6. Last filled strip is INVALID,  Final count of refilled strips is %d"),nFinalStripCount);
		}
	#endif
#endif
	return nFinalStripCount; // the caller will renumber strip indices if that is necessary
}


// AdjustForUserEdits() adjusts the CStrip instances in the layout, when RecalcLayout()
// was called with input enum value keep_strips_keep_piles, where the user did his last
// editing or movement of the phrase box. It includes a "fail-safe" mechanism too.
//
// We don't try to get exactly the first and last of the edited piles delineated, that's
// too fiddly, the strips they reside in is sufficient, since a few dozen strips being
// rebuild can be done really fast. If the active strip is a ways off from where the
// editing was done, we extend the strip range to include the active strip - except when
// the active strip is more than the number of visible strips away from the editing area -
// if that is the case, we just forget about updating the old active strip because it is
// probably off-screen anyway, and the Draw() function will, if it remains invalid at the
// present time, fix this when it next becomes visible.
//
// Return TRUE if the adjustment went without error, return FALSE if the user edits area
// was not defined and so GetPilesBoundingTheInvalidStrips() internally called
// RecalcLayout() with parameter create_strips_keep_piles in order to get a valid layout
// recalculation be destroying and recreating all the strips.
bool CLayout::AdjustForUserEdits(int nStripWidth, int gap)
{
	int nIndexWhereEditsStart = -1;
	int nIndexWhereEditsEnd = -1;
	int nVisStrips = GetNumVisibleStrips(); // count of how many strips fit in client area
	//DebugIndexMismatch(111, 1);

	// interrogate the active location and the CLayout::m_invalidStripArray to work out
	// which strips, at the last user editing operation, were marked as invalid
	GetInvalidStripRange(nIndexWhereEditsStart, nIndexWhereEditsEnd, nVisStrips);
	int nInitialInvalidStripCount = nIndexWhereEditsEnd - nIndexWhereEditsStart + 1;
#ifdef _13STRIPS_DOC
	#ifdef _DEBUG
		{
			wxLogDebug(_T("\nAdjust...Beginning... Invalid_Strip_Range: start index %d , end index %d , invalids count %d"),
				nIndexWhereEditsStart, nIndexWhereEditsEnd, nInitialInvalidStripCount);
		}
	#endif
#endif
#ifdef _13STRIPS_DOC
	#ifdef _DEBUG
		{
			int indices[26];
			int count = m_stripArray.GetCount();
			int anIndex;
			for (anIndex = 0; anIndex < 13; anIndex++)
			{
				indices[anIndex] = ((CStrip*)m_stripArray.Item(anIndex))->GetStripIndex();
			}
			wxLogDebug(_T("Adjust... 1. strip count %d , strip indices: [0] %d [1] %d [2] %d [3] %d [4] %d [5] %d [6] %d [7] %d [8] %d [9] %d [10] %d [11] %d [12] %d"),
				count,indices[0],indices[1],indices[2],indices[3],indices[4],indices[5],indices[6],indices[7],indices[8],indices[9],indices[10],indices[11],indices[12]);
		}
	#endif
#endif
	// use the invalid strip range to get the CPile pointers which bound either end, that
	// is, the last pile of the preceding (valid) strip, and the first pile of the
	// following (valid) strip. If the last strip in the document is invalid, there will
	// be no "after" bounding CPile pointer, so it will be returned as NULL. The "after"
	// pointer is crutial - it is found in the strip array, and being a copy of a pile
	// pointer in CLayout::m_pileList, by searching in the latter we can determine a
	// location where we know the user's edits have not changed anything at that point and
	// beyond - it is this fact alone which gives us a bound for halting the strip so as
	// to keep strip updates of short duration - otherwise we'd be updating strips to the
	// end of the document at every user edit.
	int nFirstPileIndex = -1;
	int nEndPileIndex = -1;
	bool bWasSuccessful = GetPileRangeForUserEdits(nIndexWhereEditsStart, nIndexWhereEditsEnd,
													nFirstPileIndex, nEndPileIndex);
	// BEW added 25Oct09, need to include a test for an m_nActiveSequNum value of -1 or
	// m_pActivePile == NULL, to ensure that in review mode when box advances at doc end,
	// a return FALSE will be done here even if bWasSuccessful returns TRUE (somehow, even
	// though the active sn is -1 and active pile is null, sometimes the value returned from
	// GetPileRangeForUserEdits() is TRUE, with parameter values of 0,0,0 for first three -
	// which should not happen. Instead of trying to figure out how come this can happen, it
	// is sufficient just to have the larger test to force correct premature exit, otherwise
	// the code further below will fail
	if (!bWasSuccessful || (m_pApp->m_nActiveSequNum == -1 || m_pApp->m_pActivePile == NULL))
		return FALSE; // enable caller to inform RecalcLayout() to exit early because layout
					  // was done by reentering RecalcLayout() within
					  // GetPilesBoundingTheInvalidStrips(), and the active pile reset too
#ifdef _13STRIPS_DOC
	#ifdef _DEBUG
		{
			wxLogDebug(_T("Adjust...  index of first pile for placement  %d , index of last pile for placement  %d"),
						nFirstPileIndex, nEndPileIndex );
		}
	#endif
#endif
#ifdef _13STRIPS_DOC
	#ifdef _DEBUG
		{
			int indices[26];
			int count = m_stripArray.GetCount();
			int anIndex;
			for (anIndex = 0; anIndex < count; anIndex++)
			{
				indices[anIndex] = ((CStrip*)m_stripArray.Item(anIndex))->GetStripIndex();
			}
			wxLogDebug(_T("Adjust... 2. strip count %d , strip indices: [0] %d [1] %d [2] %d [3] %d [4] %d [5] %d [6] %d [7] %d [8] %d [9] %d [10] %d [11] %d [12] %d"),
				count,indices[0],indices[1],indices[2],indices[3],indices[4],indices[5],indices[6],indices[7],indices[8],indices[9],indices[10],indices[11],indices[12]);
		}
	#endif
#endif
    // now empty the invalid strips - these are always contiguous and in a block, not a
    // discontinuous set; the m_arrPiles, and m_arrPileOffsets arrays are cleared, and the
    // m_nFree member reinitialized to the nStripWidth value, and a count of how many were
    // cleared is returned. Other members of the strip are not changed. (Note,
    // CreateStrip() empties the arrays and sets m_nFree, so strictly speaking we could
    // dispense with calling EmptyTheInvalidStrips(), but calling it here is insurance,
    // because it allows RebuildTheInvalidStripRange() to check that it removes only
    // pre-emptied strips, where the emptying is done here - this protects against data
    // loss)
	// DebugIndexMismatch(111, 2);
	int nHowManyStrips =
		   EmptyTheInvalidStrips(nIndexWhereEditsStart, nIndexWhereEditsEnd, nStripWidth);
	// adding text in Edit Source Text can result in the above passed in indices referring
	// to a range of strips which now now longer exist, or exist only in part, and
	// internal tests may have reduced the span - so we must test the nHowManyStrips value
	// and compare with nIndexWhereEditsStart and nIndexWhereEditsEnd in order to adjust
	// the latter to reflect the actual number of strips emptied
	int nExpectedCount = nIndexWhereEditsEnd - nIndexWhereEditsStart + 1;
	int maxIndex = m_stripArray.GetCount() - 1;
	if (nHowManyStrips == 0)
	{
		// best we can do is start and end at the last actually existing strip
		nIndexWhereEditsStart = maxIndex;
		nIndexWhereEditsEnd = maxIndex;
	}
	else if (nHowManyStrips < nExpectedCount)
	{
		// adjust the nIndexWhereEditsEnd value only
		nIndexWhereEditsEnd = nIndexWhereEditsStart + nHowManyStrips - 1;
	}

#ifdef _13STRIPS_DOC
	#ifdef _DEBUG
		{
			int pilesarray[26];
			int offsetsarray[26];
			int count = m_stripArray.GetCount();
			int anIndex;
			for (anIndex = 0; anIndex < count; anIndex++)
			{
				pilesarray[anIndex] = ((CStrip*)m_stripArray.Item(anIndex))->m_arrPiles.GetCount();
				offsetsarray[anIndex] = ((CStrip*)m_stripArray.Item(anIndex))->m_arrPileOffsets.GetCount();
				wxLogDebug(_T("Adjust... 2.9. strip[%d], its m_nStrip =  %d ,  piles count %d , offsets count %d"),
					anIndex,((CStrip*)m_stripArray.Item(anIndex))->m_nStrip,pilesarray[anIndex],offsetsarray[anIndex]);
			}
		}
	#endif
#endif
#ifdef _13STRIPS_DOC
	#ifdef _DEBUG
		{
			int indices[26];
			int count = m_stripArray.GetCount();
			int anIndex;
			for (anIndex = 0; anIndex < count; anIndex++)
			{
				indices[anIndex] = ((CStrip*)m_stripArray.Item(anIndex))->GetStripIndex();
			}
			wxLogDebug(_T("Adjust... 3. strip count %d , strip indices: [0] %d [1] %d [2] %d [3] %d [4] %d [5] %d [6] %d [7] %d [8] %d [9] %d [10] %d [11] %d [12] %d"),
				count,indices[0],indices[1],indices[2],indices[3],indices[4],indices[5],indices[6],indices[7]),indices[8],indices[9],indices[10],indices[11],indices[12]);
		}
	#endif
#endif
	// now rebuild the emptied strips, from the sublist of CPile pointers defined by the
	// posBegin and posEnd values, or by doc start or end if one or both is NULL,
	// respectively. Return a count of how many strips comprise the rebuilt area in the
	// strip array. The last rebuilt strip will usually be marked m_bValid = FALSE. (It is
	// the Draw() function's job to flow piles up as necessary to fill any strips in the
	// visible range that are marked as invalid still.)
	// DebugIndexMismatch(111, 3);
	int nHowManyNewStrips = RebuildTheInvalidStripRange(nIndexWhereEditsStart,
								nIndexWhereEditsEnd, nStripWidth, gap, nFirstPileIndex,
								nEndPileIndex, nInitialInvalidStripCount);
	// DebugIndexMismatch(111, 4);
	if (nHowManyNewStrips != nHowManyStrips)
	{
		// update each of the m_nStrip index members so that they match the
		// array storage index
		//DebugIndexMismatch(111, 5);
		UpdateStripIndices(nIndexWhereEditsStart);
#ifdef _13STRIPS_DOC
		#ifdef _DEBUG
			{
				wxLogDebug(_T("Adjust... 4. UpdateStripIndices() called, starting from strip[%d]"),nIndexWhereEditsStart);
			}
		#endif
#endif
#ifdef _13STRIPS_DOC
		#ifdef _DEBUG
			{
				int indices[26];
				int count = m_stripArray.GetCount();
				int anIndex;
				for (anIndex = 0; anIndex < count; anIndex++)
				{
					indices[anIndex] = ((CStrip*)m_stripArray.Item(anIndex))->GetStripIndex();
				}
				wxLogDebug(_T("Adjust... after UpdateStripIndices() strip count %d , strip indices: [0] %d [1] %d [2] %d [3] %d [4] %d [5] %d [6] %d [7] %d [8] %d [9] %d [10] %d [11] %d [12] %d and extras..."),
					count,indices[0],indices[1],indices[2],indices[3],indices[4],indices[5],indices[6],indices[7],indices[8],indices[9],indices[10],indices[11],indices[12]);
				int nExtras = count - 13;
				if (nExtras > 0)
				{
					for (anIndex = 13; anIndex < count; anIndex++)
					{
						indices[anIndex] = ((CStrip*)m_stripArray.Item(anIndex))->GetStripIndex();
						wxLogDebug(_T("	Rebuild: 3. Extras: strip with index: [%d] %d "),anIndex, indices[anIndex]);
					}
				}
			}
		#endif
#endif
	}
	// additional cleanup may be required
	int nActiveStripIndex = (m_pView->GetPile(m_pApp->m_nActiveSequNum))->GetStripIndex();

	// do any required flow up of piles from strips below the active one
	if (nActiveStripIndex != -1 && (nActiveStripIndex < (int)m_stripArray.GetCount() - 2))
	{
        // start at the active strip and flow piles from lower strips up, filling strips,
        // do up to the client area's visible strips amount, or fewer if near the document
        // end (doing as many as can be seen in the visible area of view guarantees that
        // we'll fix up all the visible strips)
		// DebugIndexMismatch(111, 6);
		int nFixHowManyStrips = GetNumVisibleStrips()/2 + 2; // a little over half a client
			// area's worth should be enough because we'll not locate the phrase box higher
			// than about mid window
		CleanUpTheLayoutFromStripAt(nActiveStripIndex + 1, nFixHowManyStrips);
		// DebugIndexMismatch(111, 7);
	}
	m_stripArray.Shrink();
	return TRUE;
}

// return TRUE if there was sufficient free space in the nUpStripIndex strip for the
// initial piles in the following strip to migrate up (and do the migration); else return
// FALSE when there is not enough space (and no migration is then attempted); also return
// FALSE if the following strip does not exist, ie. the nUpStripIndex is the last in the
// laid out document; or the first pile of the following strip was one which triggers
// strip wrap.
// Note: flowing piles up can potentially empty the "following" strip of all its piles; we
// must check for this and whenever it happens, remove the empty strip from the
// m_stripArray, and continue processing the upward flow of piles from the next strip (if
// there is one)
// Therefore, functions which call FlowInitialPileUp() must take into account that the
// count of elements in m_stripArray may dynamically change; this is of no consequence
// when the flowing is done in a part of the document not near its end, but near its end
// callers will have to take this possibility into account, failure to do so would lead to
// an app crash. To help us out, the third parameter, bDeletedFollowingStrip, returns TRUE
// to the caller when we've emptied the following strip and deleted it; else it returns FALSE
bool CLayout::FlowInitialPileUp(int nUpStripIndex, int gap, bool& bDeletedFollowingStrip)
{
	bDeletedFollowingStrip = FALSE;
	// prevent bounds error
	int lastStripIndex = m_stripArray.GetCount() - 1;
	if ( nUpStripIndex > lastStripIndex)
	{
		// index passed in was beyond the end of the array, return indicating inability to
		// perform the requested flow up
		return FALSE;
	}
	int nFollStripIndex = nUpStripIndex + 1;
	CStrip* pUpStrip = (CStrip*)m_stripArray.Item(nUpStripIndex);
	wxASSERT(pUpStrip);
	CStrip* pFollStrip = NULL;
	if (nFollStripIndex > lastStripIndex)
	{
		// there is no following strip which can supply a pile pointer for flow up
		pUpStrip->m_bValid = TRUE;
		return FALSE;
	}
	pFollStrip = (CStrip*)m_stripArray.Item(nFollStripIndex);
	CPile* pPileToFlowUp = (CPile*)pFollStrip->m_arrPiles.Item(0);
	wxASSERT(pPileToFlowUp);
	int nPileIndex_InList = pPileToFlowUp->m_pSrcPhrase->m_nSequNumber;
	bool bIsActivePile = m_pApp->m_nActiveSequNum == nPileIndex_InList; // rarely, if
															// ever, should it be TRUE
	// get the pile's width as currently set - depends on whether it is the active one or
	// not
	int width;
	if (bIsActivePile)
		width = pPileToFlowUp->m_nWidth;
	else
		width = pPileToFlowUp->m_nMinWidth;
	int totalWidth = width + gap;

	// determine if it can flow up...
	// it can't flow up if it is a wrap pile and we have wrapping checks turned on
	if (m_pApp->m_bMarkerWrapsStrip && pPileToFlowUp->IsWrapPile())
	{
		// wrapping checks are ON, and there is a marker at this pile which triggers strip
		// wrap, so this pile cannot be flowed up to the end of the "up" strip; declare the
		// "up" strip valid, too
		pUpStrip->m_bValid = TRUE;
		return FALSE;
	}
	if (totalWidth > pUpStrip->m_nFree)
	{
		// it won't fit in the free space of the up strip, so return FALSE, and mark the
		// "up" strip filled, ie. valid
		pUpStrip->m_bValid = TRUE;
		return FALSE;
	}
	else
	{
		// it will fit and it doesn't trigger strip wrap, so move it up...
		// The offset of the left boundary of the pile in its new position is calculated
		// using the previous pile's left offset plus that pile's width plus the interpile
		// gap, but this calculation will not be appropriate if the pile is being flowed
		// up to be the first of a currently empty strip - in the latter case, the new
		// position of the pile's left boundary will be 0, so we must test for whether or
		// not the pUpStrip is currently empty and act accordingly
		bool bUpStripIsEmpty = pUpStrip->m_arrPiles.IsEmpty();

		// first, remove the record of it having been in the following strip
		pFollStrip->m_arrPiles.RemoveAt(0);
		pFollStrip->m_arrPileOffsets.RemoveAt(0);
		pFollStrip->m_nFree += totalWidth;
		pFollStrip->m_bValid = FALSE; // this invalidates that strip, even if it was
									  // formerly valid
        // add the pile pointer to the end of the "up" strip and set the offset to leading
        // edge in the m_arrPileOffsets array, and decrease the free space accordingly
        if (bUpStripIsEmpty)
		{
			// we have to provide for this eventuality, but some testing returned the
			// result that this block never gets entered -- nevertheless, we'll code for
			// the possibility just in case...
			//wxLogDebug(_T("*** HALT HERE ***"));
			pUpStrip->m_arrPileOffsets.Add(0);
		}
		else // there is at least one pile already in the "up" strip
		{
			int nLastPileLeadingEdgeOffset = pUpStrip->m_arrPileOffsets.Last();
			CPile* pLastPileInStrip = (CPile*)pUpStrip->m_arrPiles.Last();
			int nLastPileWidth = 0;
			if (pLastPileInStrip->m_pSrcPhrase->m_nSequNumber == m_pApp->m_nActiveSequNum)
				nLastPileWidth = pLastPileInStrip->m_nWidth;
			else
				nLastPileWidth = pLastPileInStrip->m_nMinWidth;
			pUpStrip->m_arrPileOffsets.Add(nLastPileLeadingEdgeOffset + nLastPileWidth + gap);
		}
		pUpStrip->m_arrPiles.Add(pPileToFlowUp); // this call has to come after the above lines
		pUpStrip->m_nFree -= totalWidth;

		// the pile is now in a different strip, and at a different index in the strip
		// than before, so update these parameters too; the pPileToFlowUp is already added
		// to the strip
		pPileToFlowUp->m_pOwningStrip = pUpStrip;
		pPileToFlowUp->m_nPile = (int)(pUpStrip->m_arrPiles.GetCount() - 1);

		// the piles in the lower strip from which it flowed up have now shifted to an
		// earlier position in the m_arrPiles array, and m_arrPileOffsets needs to be
		// adjusted too; and at present each of those remaining piles still has their old
		// m_nPile index values unchanged - these are the index locations for their old
		// locations in the strip - so now they have a 1-off error. We must now loop over
		// these to reset their m_nPile values to agree with their new array locations, and
		// update their offsets in m_arrPileOffsets; m_nFree has already been augmented above
		int lastIndex = (int)(pFollStrip->m_arrPiles.GetCount() - 1);
		if (lastIndex > -1)
		{
			int indx;
			int nLastOffset = 0;
			int nLastWidth = 20;
			int newOffset = 0;
			// wxArray does not have a function for changing the value at an index, we
			// have to remove and insert
			pFollStrip->m_arrPileOffsets.RemoveAt(0);
			pFollStrip->m_arrPileOffsets.Insert(0,0);
			for (indx = 0; indx <= lastIndex; indx++)
			{
				CPile* pPile = (CPile*)pFollStrip->m_arrPiles.Item(indx);
				pPile->m_nPile = indx;
				// now the offsets in m_arrPileOffsets have to be updated, and we've
				// already done the first outside this loop
				if (indx >= 1)
				{
					pFollStrip->m_arrPileOffsets.RemoveAt(indx);
					newOffset = nLastOffset + nLastWidth + gap;
					pFollStrip->m_arrPileOffsets.Insert(newOffset, indx);
				}
				// update the nLastWidth and nLastOffset values for next iteration of loop
				bool bIsTheActivePile =
							m_pApp->m_nActiveSequNum == pPile->m_pSrcPhrase->m_nSequNumber;
				if (bIsTheActivePile)
				{
					nLastWidth = pPile->m_nWidth;
				}
				else
				{
					nLastWidth = pPile->m_nMinWidth;
				}
				nLastOffset = newOffset;
			}
		}
        // check if the flow up has just emptied the "following" strip - if so, remove the
        // empty strip from the m_stripArray, & the caller must check for the end of the
        // document and exit its loop after doing the appropriate adjustments if this strip
        // removal process resulted in the removal of the last strip of the document
		if (pFollStrip->m_arrPiles.IsEmpty() && pFollStrip->m_arrPileOffsets.IsEmpty())
		{
			// if about to delete the final strip in the document, declare the pUpStrip
			// which has just received all its piles, m_bValid = TRUE
			if ((int)(m_stripArray.GetCount() - 1) == nFollStripIndex)
			{
				pUpStrip->m_bValid = TRUE; // make sure it's valid, the last in doc always is
			}
			// now do the strip deletion (not always is it the last in the doc though)
			if (pFollStrip != NULL) // whm 11Jun12 added NULL test
				delete pFollStrip;
			m_stripArray.RemoveAt(nFollStripIndex);
			bDeletedFollowingStrip = TRUE;
			UpdateStripIndices(nUpStripIndex); // keep strip numbering consecutive
			return TRUE; // let the caller work out if more flowing is required with any
						 // strips which followed this one just deleted
		}
	}
	return TRUE;
}

// returns nothing; the layout-tweaking code can, if the user clicks around the document
// where there are some short strips, result in several consecutive strips having just a
// singleton pile within them, even though only one of the may precede a strip which
// commences with a "wrap-causing" pile; so this clean up function does flow up of piles
// to whatever extent is possible, for up to nVisibleStrips strips from the strip with
// index nIndexOfStripToStartAt -- which typically should be the strip following whichever
// is the active pile after the user's edit operation is done.
// Doing this should relieve the subsequent Draw(() of the layout from finding m_bValid ==
// FALSE strips within the view's client area and have to then do pile flow upwards to make
// the visible ones all be valid; that is, we try to this validating job within
// RecalcLayout() rather than pass some of it to Draw() to have to do on a frequent basis;
// Draw() will still make the check, but we expect it rarely to find any invalid strips
// that require pile flow.
// Note: the internal call of FlowInitialPileUp() can result in the source strip becoming
// empty of piles, and the latter function then removes any such emptied strip before
// returning, and so we must be prepared for the count of strips in m_stripArray to change
// dynamically in the code below - and particular care must be exercised near the
// document's end.
void CLayout::CleanUpTheLayoutFromStripAt(int nIndexOfStripToStartAt, int nHowManyToDo)
{
	// Works on strip pairs, the first of the pair being at the strip index passed in as
	// the first parameter. Call repetitively for each of several strips in a range of
	// strips, if cleanup of more than a single pair is required - the function for doing
	// that must take account of the possibility that cleanup will remove an empty strip
	// if pile flow from the second of the pair to the first removes all piles from the
	// second strip.
	// The nHowManyToDo is required as a parameter because, for cleanup when
	// RecalcLayout() is called, the active strip is not allowed to be higher than than
	// middle of the client window, so we only clean up about half a window's worth, but
	// when called in the context of Draw() in a scroll up or down, and an invalid strip
	// is found, we'll call it to do a full window's worth of strips
	bool bDeletedFollowingStrip;
	int nLastStrip = m_stripArray.GetCount() - 1; // index of last strip in the document

	// check for proximity to the beginning of the document - we'll want to do a full
	// window's worth of cleanup if the active location gets to be less than half the
	// number of visible strips' distance from the doc's start; don't do the adjustment if
	// the number of visible strips' value passed in is a full window's worth already
	if (nHowManyToDo < GetNumVisibleStrips())
	{
		int numOfStripsInAHalfWindow = GetNumVisibleStrips()/2;
		// when this function is called, the active strip will have been defined already, so
		// we can safely look it up
		CPile* activePilePtr = m_pView->GetPile(m_pApp->m_nActiveSequNum);
		CStrip* pActiveStrip = activePilePtr->GetStrip();
		if (pActiveStrip == NULL)
		{
			// not expected, but allow for it if, perchance, we've unmerged near the doc
			// start and the new active pile therefore doesn't have m_pOwningStrip set to a
			// value other than NULL
			nHowManyToDo = GetNumVisibleStrips(); // do a full window's worth, to be safe
		}
		else // we have a valid active strip's pointer
		{
			int nActiveStripIndex = pActiveStrip->GetStripIndex();
			if (nActiveStripIndex <= numOfStripsInAHalfWindow)
			{
				// we are close to the document's start, and so we risk leaving an invalid
				// strip visible if we use the passed in nHowManyToDo
				nHowManyToDo = GetNumVisibleStrips(); // do a full window's worth
			}
		}
	}
	// new we can calculate the index for the first of the last strip pair to be worked on
	int nLastStripToFix = nIndexOfStripToStartAt + nHowManyToDo - 1;
	if (nLastStripToFix >= nLastStrip)
		nLastStripToFix = nLastStrip - 1;
#ifdef _OFFSETS_BUG
	#ifdef _DEBUG
	{
		wxLogDebug(_T("\n ****** BEFORE ******     left margin = %d"),m_nCurLMargin);
		int index;
		wxString s;
		wxString addSpaces = _T("    ");
		wxString initialSpaces = _T("  ");
		for (index = nIndexOfStripToStartAt; index <= nLastStripToFix; index++)
		{
			s = initialSpaces;
			wxLogDebug(_T("Strip with index   %d"),index);
			CStrip* pStrip = (CStrip*)m_stripArray.Item(index);
			int lastPileIndex = pStrip->m_arrPiles.GetCount() - 1;
			int indexPiles;
			for (indexPiles = 0; indexPiles <= lastPileIndex; indexPiles++)
			{
				if (indexPiles != 0)
					s += addSpaces;
				CPile* pPile = (CPile*)pStrip->m_arrPiles.Item(indexPiles);
				int offset = pStrip->m_arrPileOffsets.Item(indexPiles);
				wxLogDebug(_T("%s index %d ,       Left() %d ,  %s  "), s, indexPiles, pPile->Left(),
							pPile->m_pSrcPhrase->m_srcPhrase );
				wxLogDebug(_T("%s index %d , margn+offset %d ,  %s  , offset itself is %d"), s, indexPiles,
							m_nCurLMargin + offset, pPile->m_pSrcPhrase->m_srcPhrase, offset);
			}
		}
	}
	#endif
#endif

	int index;
	bool bSuccessfulFlowOfASinglePileUpwards = FALSE;
	for (index = nIndexOfStripToStartAt; index <= nLastStripToFix; index++)
	{
		do
		{
			// Note: bSuccessfulFlowOfASinglePileUpwards, cannot be FALSE if the value
			// returned in bDeletedFollowingStrip is TRUE
			bSuccessfulFlowOfASinglePileUpwards =
						FlowInitialPileUp(index, m_nCurGapWidth, bDeletedFollowingStrip);

			// check for an empty strip having been removed, and adjust accordingly
			if (bDeletedFollowingStrip)
			{
				nLastStripToFix--; // there is one less strip to be fixed

			}
		} while (bSuccessfulFlowOfASinglePileUpwards && index <= nLastStripToFix);
	}
	//UpdateStripIndices(nIndexOfStripToStartAt); // removed, because we will do this
												  // internally and only when needed
#ifdef _OFFSETS_BUG
	#ifdef _DEBUG
	{
		wxLogDebug(_T("\n ****** AFTER ******     left margin = %d"),m_nCurLMargin);
		int index;
		wxString s;
		wxString addSpaces = _T("    ");
		wxString initialSpaces = _T("  ");
		for (index = nIndexOfStripToStartAt; index <= nLastStripToFix; index++)
		{
			s = initialSpaces;
			wxLogDebug(_T("Strip with index   %d"),index);
			CStrip* pStrip = (CStrip*)m_stripArray.Item(index);
			int lastPileIndex = pStrip->m_arrPiles.GetCount() - 1;
			int indexPiles;
			for (indexPiles = 0; indexPiles <= lastPileIndex; indexPiles++)
			{
				if (indexPiles != 0)
					s += addSpaces;
				CPile* pPile = (CPile*)pStrip->m_arrPiles.Item(indexPiles);
				int offset = pStrip->m_arrPileOffsets.Item(indexPiles);
				wxLogDebug(_T("%s index %d ,       Left() %d ,  %s  "), s, indexPiles, pPile->Left(),
							pPile->m_pSrcPhrase->m_srcPhrase );
				wxLogDebug(_T("%s index %d , margn+offset %d ,  %s  , offset itself is %d"), s, indexPiles,
							m_nCurLMargin + offset, pPile->m_pSrcPhrase->m_srcPhrase, offset);
			}
		}
	}
	#endif
#endif
}

/////////////////////////////////////////////////////////////////////////////////
/// \return         nothing
///
/// \remarks
///	Resets the m_bIsCurrentFreeTransSection member of CPile instances
///	through the whole doc to FALSE. Use this prior, followed by
///	MarkFreeTranslationPilesForColoring(), when the current section changes
///	to a new location, so that colouring gets done correctly at the right places
/// BEW 22Feb10 no changes needed for support of doc version 5
/// BEW 17May10, moved to CLayout from CAdapt_ItView class
/////////////////////////////////////////////////////////////////////////////////
void CLayout::MakeAllPilesNonCurrent()
{
	PileList* pList = GetPileList();
	PileList::Node* pos = pList->GetFirst();
	CPile* pPile = NULL;
	while (pos != NULL)
	{
		pPile = pos->GetData();
		pPile->SetIsCurrentFreeTransSection(FALSE);
		pos = pos->GetNext();
	}
	// makes them ALL false
}

/////////////////////////////////////////////////////////////////////////////////
/// \return         nothing
///
/// \remarks
///	Scans all piles of the current layout to find any CCell[1] which has its
///	m_bAutoInserted member boolean set to TRUE, and clears it to FALSE
///	
///	Usage:
///	Call as follows:
///	1. In OnNewDocument() to ensure correct initialization
///	2. In OnOpenDocument() to ensure correct initialization
///	3. In OnLButtonDown() - in one place to clear highlighting for a click not
///	   in any cell; in another place to clear highlighting for a click in a cell
///	   not already highlighted [uses IsClickWithinAutoInsertionsHighlightedSpan()]
///	4. In OpenDocWithMerger()
///	5. In JumpForward() 
///	
/// BEW 9Apr12 created, for the refactored auto-insert mechanism,
/// which supports discontinuous auto-insertions
/////////////////////////////////////////////////////////////////////////////////
void CLayout::ClearAutoInsertionsHighlighting()
{
	CPile* pPile = NULL;
	PileList::Node* pos = m_pileList.GetFirst();
	while (pos != NULL)
	{
		pPile = pos->GetData();
		wxASSERT(pPile != NULL);
		CCell* pCell = pPile->GetCell(1); // depending on current mode, it could
										  // be an adaptation cell, or a gloss cell
		if (pCell->m_bAutoInserted)
		{
			pCell->m_bAutoInserted = FALSE;
		}
		pos = pos->GetNext();
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return         TRUE if, at the active pile, CCell[1] has m_bAutoInserted set TRUE,
///                 otherwise return FALSE
/// \param      ->  the active sequence number (of the CSourcePhrase associated with the
///                 just-clicked CPile instance)
/// \remarks
///	Tests CCell[1] for its m_bAutoInserted member boolean set to TRUE, returning the
///	result to the caller
///	
///	Usage:
///	Call as follows:
///	1. In OnLButtonDown() - to clear highlighting for a click in a cell not already highlighted
///	
/// BEW 9Apr12 created, for the refactored auto-insert mechanism,
/// which supports discontinuos auto-insertions
/////////////////////////////////////////////////////////////////////////////////
bool CLayout::IsLocationWithinAutoInsertionsHighlightedSpan(int sequNum)
{
	CPile* pPile = GetPile(sequNum);
	if (pPile == NULL)
	{
		return FALSE;
	}
	else
	{
		return pPile->GetCell(1)->m_bAutoInserted;
	}
}

// sets pPile->m_bAutoInserted to TRUE
// BEW 9Apr12 created, for the refactored auto-insert mechanism,
// which supports discontinuos auto-insertions
void CLayout::SetAutoInsertionHighlightFlag(CPile* pPile)
{
	pPile->GetCell(1)->m_bAutoInserted = TRUE;
}

// scans whole of the piles array and returns TRUE if the m_bAutoInserted member of
// a CCell[1] somewhere within the layout is TRUE; else returns FALSE
// Used only in GetHighlightedStripsRange() [and the latter is used only in ScrollIntoView()]
bool CLayout::AreAnyAutoInsertHighlightsPresent()
{
	CPile* pPile = NULL;
	PileList::Node* pos = m_pileList.GetFirst();
	while (pos != NULL)
	{
		pPile = pos->GetData();
		wxASSERT(pPile != NULL);
		CCell* pCell = pPile->GetCell(1); // depending on current mode, it could
										  // be an adaptation cell, or a gloss cell
		if (pCell->m_bAutoInserted)
		{
			// we found a highlighted one
			return TRUE;
		}
		pos = pos->GetNext();
	}
	// there are none hilighted
	return FALSE;
}

/*
// created for identifying where some piles didn't get their m_nPile values updated -- turned out
// to be the leftover ones in the strip after flow up to the preceding strip finished
// because the preceding strip had become full; so fixed the bad code in FlowInitialPileUp()
void CLayout::DebugIndexMismatch(int nPileIndex_InList, int locator)
{
	CPile* pPile = GetPile(nPileIndex_InList);
	CStrip* pOwningStrip = pPile->m_pOwningStrip;
	int nWhereAt = pOwningStrip->m_arrPiles.Index(pPile);
	int nWhereItThinksItsAt = pPile->m_nPile;
	if (nWhereAt != nWhereItThinksItsAt)
		wxLogDebug(_T("Locator %d  m_arrPiles position [%d] m_nPile index %d  MISMATCHED"),locator,nWhereAt,nWhereItThinksItsAt);
	else
		wxLogDebug(_T("Locator %d  m_arrPiles position [%d] m_nPile index %d"),locator,nWhereAt,nWhereItThinksItsAt);
}
*/
