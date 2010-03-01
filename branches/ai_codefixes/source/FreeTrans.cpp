/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FreeTrans.cpp
/// \author			Graeme Costin
/// \date_created	10 Februuary 2010
/// \date_revised	10 Februuary 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the implementation file for the CFreeTrans class. 
/// The CFreeTrans class presents free translation fields to the user. 
/// The functionality in the CFreeTrans class was originally contained in
/// the CAdapt_ItView class.
/// \derivation		The CFreeTrans class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifdef	_FREETR

#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "FreeTrans.h"
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

#if defined(__VISUALC__) && __VISUALC__ >= 1400
#pragma warning(disable:4428)	// VC 8.0 wrongly issues warning C4428: universal-character-name 
								// encountered in source for a statement like 
								// ellipsis = _T('\u2026');
								// which contains a unicode character \u2026 in a string literal.
								// The MSDN docs for warning C4428 are also misleading!
#endif

#include <wx/object.h>

#include "AdaptitConstants.h"
#include "Adapt_It.h"
#include "Adapt_ItView.h"
#include "SourcePhrase.h"
#include "Strip.h"
#include "Pile.h"
#include "Cell.h"
#include "Layout.h"
#include "FreeTrans.h"
#include "helpers.h"
#include "MainFrm.h"
#include "Adapt_ItCanvas.h"
#include "Adapt_ItDoc.h"

/// This global is defined in Adapt_It.cpp.
extern bool	gbRTL_Layout;	// ANSI version is always left to right reading; this flag can only
							// be changed in the Unicode version, using the extra Layout menu

/// This global is defined in Adapt_It.cpp.
extern bool gbSuppressSetup;

/// When TRUE the main window only displays the target text lines.
extern bool gbShowTargetOnly;

/// This global is defined in Adapt_It.cpp.
extern wxChar gSFescapechar; // the escape char used for start of a standard format marker

/// This flag is used to indicate that the text being processed is unstructured, i.e.,
/// not containing the standard format markers (such as verse and chapter) that would 
/// otherwise make the document be structured. This global is used to restore paragraphing 
/// in unstructured data, on export of source or target text.
/// Defined in Adapt_ItView.cpp
extern bool gbIsUnstructuredData; 

/// This global is defined in Adapt_It.cpp.
extern wxString	gSpacelessTgtPunctuation; // contents of app's m_punctuation[1] string with spaces removed

/// When TRUE it indicates that the application is in the "See Glosses" mode. In the 
/// "See Glosses" mode any existing glosses are visible in a separate glossing line in 
/// the main window, but words and phrases entered into the phrasebox are not entered 
/// into the glossing KB unless gbEnableGlossing is also TRUE.
/// Defined in Adapt_ItView.cpp
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in PhraseBox.cpp.
extern wxString		translation; // translation, for a matched source phrase key

/// This global provides a persistent location during the current session for storage of 
/// vertical edit information
extern	EditRecord gEditRecord; // store info pertinent to generalized editing with entry 
		// point for an Edit Source Text request, in this global structure

/// This global is defined in Adapt_It.cpp.
extern bool gbFreeTranslationJustRemovedInVFMdialog;

extern bool gbVerticalEditInProgress;

/// This flag is used to indicate that the text being processed is unstructured, i.e.,
/// not containing the standard format markers (such as verse and chapter) that would 
/// otherwise make the document be structured. This global is used to restore paragraphing 
/// in unstructured data, on export of source or target text.
extern bool	gbIsUnstructuredData; 

extern const wxChar* filterMkr; // defined in the Doc, used here in OnLButtonDown() & free translation code, etc
extern const wxChar* filterMkrEnd; // defined in the Doc, used in free translation code, etc

// grectViewClient is defined as a global in Adapt_ItView.cpp and is set and used there.
//TODO: Try to find a better way of handling this than a global.
extern	wxRect			grectViewClient;

// *******************************************************************
// Event handlers
// *******************************************************************

BEGIN_EVENT_TABLE(CFreeTrans, wxEvtHandler)

	EVT_MENU(ID_ADVANCED_FREE_TRANSLATION_MODE, CFreeTrans::OnAdvancedFreeTranslationMode)
	EVT_UPDATE_UI(ID_ADVANCED_FREE_TRANSLATION_MODE, CFreeTrans::OnUpdateAdvancedFreeTranslationMode)
	EVT_MENU(ID_ADVANCED_TARGET_TEXT_IS_DEFAULT, CFreeTrans::OnAdvancedTargetTextIsDefault)
	EVT_UPDATE_UI(ID_ADVANCED_TARGET_TEXT_IS_DEFAULT, CFreeTrans::OnUpdateAdvancedTargetTextIsDefault)
	EVT_MENU(ID_ADVANCED_GLOSS_TEXT_IS_DEFAULT, CFreeTrans::OnAdvancedGlossTextIsDefault)
	EVT_UPDATE_UI(ID_ADVANCED_GLOSS_TEXT_IS_DEFAULT, CFreeTrans::OnUpdateAdvancedGlossTextIsDefault)
	// for collected back translations support
	EVT_MENU(ID_ADVANCED_REMOVE_FILTERED_BACKTRANSLATIONS, CFreeTrans::OnAdvancedRemoveFilteredBacktranslations)
	EVT_UPDATE_UI(ID_ADVANCED_REMOVE_FILTERED_BACKTRANSLATIONS, CFreeTrans::OnUpdateAdvancedRemoveFilteredBacktranslations)
	EVT_MENU(ID_ADVANCED_REMOVE_FILTERED_FREE_TRANSLATIONS, CFreeTrans::OnAdvancedRemoveFilteredFreeTranslations)
	EVT_UPDATE_UI(ID_ADVANCED_REMOVE_FILTERED_FREE_TRANSLATIONS, CFreeTrans::OnUpdateAdvancedRemoveFilteredFreeTranslations)
	// end collected back translations support
	EVT_BUTTON(IDC_BUTTON_APPLY, CFreeTrans::OnAdvanceButton)
	EVT_UPDATE_UI(IDC_BUTTON_NEXT, CFreeTrans::OnUpdateNextButton)
	EVT_BUTTON(IDC_BUTTON_NEXT, CFreeTrans::OnNextButton)
	EVT_UPDATE_UI(IDC_BUTTON_PREV, CFreeTrans::OnUpdatePrevButton)
	EVT_BUTTON(IDC_BUTTON_PREV, CFreeTrans::OnPrevButton)
	EVT_UPDATE_UI(IDC_BUTTON_REMOVE, CFreeTrans::OnUpdateRemoveFreeTranslationButton)
	EVT_BUTTON(IDC_BUTTON_REMOVE, CFreeTrans::OnRemoveFreeTranslationButton)
	EVT_UPDATE_UI(IDC_BUTTON_LENGTHEN, CFreeTrans::OnUpdateLengthenButton)
	EVT_BUTTON(IDC_BUTTON_LENGTHEN, CFreeTrans::OnLengthenButton)
	EVT_UPDATE_UI(IDC_BUTTON_SHORTEN, CFreeTrans::OnUpdateShortenButton)
	EVT_BUTTON(IDC_BUTTON_SHORTEN, CFreeTrans::OnShortenButton)
	EVT_RADIOBUTTON(IDC_RADIO_PUNCT_SECTION, CFreeTrans::OnRadioDefineByPunctuation)
	EVT_RADIOBUTTON(IDC_RADIO_VERSE_SECTION, CFreeTrans::OnRadioDefineByVerse)

END_EVENT_TABLE()


// *******************************************************************
// Construction/Destruction
// *******************************************************************

CFreeTrans::CFreeTrans()
{
}

CFreeTrans::CFreeTrans(CAdapt_ItApp* app)
{
	wxASSERT(app != NULL);
	m_pApp = app;

	// Create array for pointers to CPile instances
	m_pCurFreeTransSectionPileArray = new wxArrayPtrVoid;
	// Create array for pointers to FreeTrElements
	m_pFreeTransArray = new wxArrayPtrVoid;

	// get needed private pointers to important external classes
	m_pView = app->GetView();
	m_pFrame = m_pView->canvas->pFrame;
	m_pLayout = app->GetLayout();
}

CFreeTrans::~CFreeTrans()
{
	// Clear and delete the arrays for FreeTrSections and Piles
	m_pCurFreeTransSectionPileArray->Clear();
	delete m_pCurFreeTransSectionPileArray;
	m_pFreeTransArray->Clear();
	delete m_pFreeTransArray;
}

// BEW 19Feb10 no changes needed for support of _DOCVER5
wxString CFreeTrans::ComposeDefaultFreeTranslation(wxArrayPtrVoid* arr)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxString str;
	str.Empty();
	int nCount = arr->GetCount();
	if (nCount == 0)
		return str;
	int index;
	wxString theText;
	theText.Empty();
	for (index = 0; index < nCount; index++)
	{
		if (pApp->m_bTargetIsDefaultFreeTrans)
		{
			// get the text from the adaptation line's contents (exclude punctuation)
			theText = ((CPile*)arr->Item(index))->GetSrcPhrase()->m_adaption;
		}
		else if (pApp->m_bGlossIsDefaultFreeTrans)
		{
			// get the text from the glossing line's contents
			theText = ((CPile*)arr->Item(index))->GetSrcPhrase()->m_gloss; 
		}
		str += theText;
		str += _T(" "); // delimit with a single space
	}
	str = MakeReverse(str);
	str = str.Mid(1); // remove trailing space
	str = MakeReverse(str);
	return str; // if neither flag was on, an empty string is returned
}

/////////////////////////////////////////////////////////////////////////////////
/// \return             TRUE if the sourcephrase's m_markers member contains a \free 
///                     marker FALSE otherwise
/// \param	pPile	->	pointer to the pile which stores the pSrcPhrase pointer being 
///                     examined
/// \remarks
/// BEW 22Feb10 changes needed for support of _DOCVER5. To support empty free translation
/// sections we need to also test for m_bHasFreeTrans with value TRUE; and for docVersion
/// = 5, we look for content in the m_freeTrans member, no longer in m_markers
/////////////////////////////////////////////////////////////////////////////////
bool CFreeTrans::ContainsFreeTranslation(CPile* pPile)
{
#if defined (_DOCVER5)
	CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
	if (pSrcPhrase->m_bHasFreeTrans || !pSrcPhrase->GetFreeTrans().IsEmpty())
		return TRUE;
	else
		return FALSE;
#else
	wxString markers = pPile->GetSrcPhrase()->m_markers;
	if (markers.IsEmpty())
		return FALSE;
	int curPos = markers.Find(_T("\\free"));
	return curPos > 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////////
/// \return                 nothing
///
/// Parameters:
///	\param pDC		       ->	pointer to the device context used for drawing the view
///	\param pLayout	       ->	pointer to the CLayout instance, which manages all the 
///	                            strips, and piles.
///	\param drawFTCaller    ->   enum value either call_from_ondraw, or call_from_edit - 
///                             when call_from_ondraw all free translations within the view
///                             are drawn; when call_from_edit, only the free translation
///                             being edited is redrawn as editing is being done
/// \remarks
/// Called in the view's OnDraw() function, which gets invoked whenever a paint message has
/// been received, but DrawFreeTranslations is only done when free translation mode is
/// turned on, otherwise it is skipped. Internally, it intersects each rectangle, and the
/// whole of each free translation section (which may span several strips), with the client
/// rectangle for the view - and when the intersection is null, it skips further
/// calculations at that point and draws nothing; furthermore, then the function determines
/// that all further drawing will be done below the bottom of the client rect, it exits.
/// The data structures and variables the function requires are, for the most part, within
///	the CLayout instance, but there are also some member functions of CFreeTrans.
///	It does either one or two passes. A second pass is tried, with tighter fitting of data
///	to available space, if the first pass does not fit it all in.
///
/// whm: With its six jump labels, and thirteen gotos, the logic of this function is very
/// convoluted and difficult to follow - BEWARE!
///   TODO: Rewrite with simpler logic!
/// whm added additional parameters on 24Aug06 and later on 31May07
/// BEW 19Feb10, updated for support of _DOCVER5 (one change, elimination of
/// GetExistingMarkerContent() call by making GetFreeTrans() call)
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::DrawFreeTranslations(wxDC* pDC, CLayout* pLayout, 
										 enum DrawFTCaller drawFTCaller)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	CAdapt_ItView* pView = pApp->GetView();
	wxASSERT(pView != NULL);
	DestroyElements(m_pFreeTransArray);
	CPile*  pPile; // a scratch pile pointer
	CStrip* pStrip; // a scratch strip pointer
	wxRect rect; // a scratch rectangle variable
	int curStripIndex;
	int curPileIndex;
	int curPileCount;
	int nTotalHorizExtent; // the sum of the horizonal extents of the subrectangles 
                           // which make up the laid out possible writable areas
                           // for the current free trans section
	wxPoint topLeft;
	wxPoint botRight;
	CSourcePhrase* pSrcPhrase;
	FreeTrElement* pElement;
	wxSize extent;
	bool bSectionIntersects = FALSE;

    // get an offscreen pile from which to scan forwards for the anchor pile (this helps
    // keep draws snappy when docs get large; we don't draw below the bottom of the window
    // either)
	pPile = GetStartingPileForScan(pApp->m_nActiveSequNum);

	// get it's CSourcePhrase instance
	pSrcPhrase = pPile->GetSrcPhrase(); // its pointed at sourcephrase

#ifdef __WXDEBUG__
#ifdef _Trace_DrawFreeTrans
	wxLogDebug(_T("\nDrawFreeTrans: starting pile sn = %d  srcPhrase = %s"),
		pSrcPhrase->m_nSequNumber, pSrcPhrase->m_srcPhrase.c_str());
#endif
#endif

	wxString ellipsis = _T("...");
	wxString ftStr;
	wxArrayString subStrings;
	wxRect testRect = grectViewClient; // need a local, because an intersect test
									  // changes the calling rectangle
	wxString theMkr = _T("\\free");
	wxString theEndMkr = _T("\\free*");

    // ready the drawing context - we must handle ANSI & Unicode, and for the former we use
    // TextOut() and for the latter we use DrawText() and the Unicode app can be LTR or RTL
    // script (we use same text rending directionality as the target text line) - code from
    // CCell.cpp and CText.cpp can be reused here
	// wx version note: wx version always uses DrawText
	wxRect rectBounding;
	bool bRTLLayout = FALSE;
	#ifdef _RTL_FLAGS
	if (pApp->m_bTgtRTL)
	{
		ellipsis = _T('\u2026'); // use a unicode ellipsis for RTL
		// free translation has to be RTL & aligned RIGHT
		bRTLLayout = TRUE; //nFormat = gnRTLFormat;
	}
	else
	{
		// free translation has to be LTR & aligned LEFT
		bRTLLayout = FALSE; //nFormat = gnLTRFormat;
	}
	#endif

	// set up a new colour - make it a purple, 
	// hard coded in app as m_freetransTextColor
	wxFont pSaveFont;
	wxFont* pFreeTransFont = pApp->m_pTargetFont;
	pSaveFont = pDC->GetFont();
	pDC->SetFont(*pFreeTransFont);
	wxColour color(pApp->m_freeTransTextColor);
	if (!color.IsOk())
	{
		::wxBell(); 
		wxASSERT(FALSE);
	}
	pDC->SetTextForeground(color); 

	// the logicalViewClientBottom is the scrolled value for the top of the view
	// window, after the device context has been adjusted; this value is constant for any
	// one call of DrawFreeTranslations(); we need to use this value in some tests,
	// because grectViewClient.GetBottom() only gives what we want when the view is
	// unscrolled
	// 
	// BEW changed 2Feb10, as the LHS value was way too far below the visible strips
	// (sluggish free translation response reported by Lisbeth Fritzell, late Jan 2010)
	//int logicalViewClientBottom = (int)pDC->DeviceToLogicalY(grectViewClient.GetBottom());
	int logicalViewClientBottom = (int)grectViewClient.GetBottom();


	// use the thumb position to adjust the Y coordinate of testRect, so it has the
	// correct logical coordinates value given the amount that the view is currently
	// scrolled 
	int nThumbPosition_InPixels = pDC->DeviceToLogicalY(0);


    // for wx testing we make the background yellow in order to verify the extent of each
    // DrawText and clearing of remaining free translation's rect segment
	//pDC->SetBackgroundMode(pApp->m_backgroundMode);
	//pDC->SetTextBackground(wxColour(255,255,0));
	// wx testing above

	// THE LOOP FOR ITERATING OVER ALL FREE TRANSLATION SECTIONS IN THE DOC,
	//  STARTING FROM A PRECEDING OFFSCREEN CPile INSTANCE, BEGINS HERE

	#ifdef _Trace_DrawFreeTrans
	#ifdef __WXDEBUG__
	wxLogDebug(_T("\n BEGIN - Loop About To Start in DrawFreeTranslations()\n"));
	#endif
	#endif

	// whm: I moved the following declarations and initializations here
	// from way below to avoid compiler warnings:
	bool bTextIsTooLong = FALSE;
	int totalRects = 0;
	int offset = 0;
	int length = 0;

    // whm added 25Aug06 if we are being called from the OnEnChangeEditBox handler we can
    // assume that the screen display has been drawn with any existing free translations,
    // including the one we may be editing. Since we are editing a single free translation,
    // we can just update the free translation associated with the first pile in
    // m_pCurFreeTransSectionPileArray. So, we can go directly there without having to worry
    // about drawing any other free translations.
	if (drawFTCaller == call_from_edit)
	{
        // if we are not presently at the pile where the phrase box currently is, keep
        // scanning piles until we are (or encounter the end of the doc - an error)
		while (pPile != NULL && !pPile->GetIsCurrentFreeTransSection())
		{
			pPile = m_pView->GetNextPile(pPile);
		}
        // when DrawFreeTranslations is called from the composebar's editbox, there should
        // certainly be a valid pPile to be found
		wxASSERT(pPile != NULL && pPile->GetIsCurrentFreeTransSection());
        // if this is a new free translation which has not been entered at this location
        // before, and the user just typed the first character, the free trans flags on the
        // source phrases will not have been set, but they must be set for the code below
        // to properly define this free translation element
		CPile* pCurrentPile;
		int j;
		for (j = 0; j < (int)m_pCurFreeTransSectionPileArray->GetCount(); j++)
		{
			// set the common flags
			pCurrentPile = (CPile*)m_pCurFreeTransSectionPileArray->Item(j);
			pCurrentPile->GetSrcPhrase()->m_bHasFreeTrans = TRUE;
			pCurrentPile->GetSrcPhrase()->m_bEndFreeTrans = FALSE;
			pCurrentPile->GetSrcPhrase()->m_bStartFreeTrans = FALSE;
		}
		// set the beginning one
		pCurrentPile = (CPile*)m_pCurFreeTransSectionPileArray->Item(0);
		pCurrentPile->GetSrcPhrase()->m_bStartFreeTrans = TRUE;
		// set the ending one
		pCurrentPile = (CPile*)m_pCurFreeTransSectionPileArray->Item(
									m_pCurFreeTransSectionPileArray->GetCount()-1);
		pCurrentPile->GetSrcPhrase()->m_bEndFreeTrans = TRUE;
		goto ed;
	}

    // The a: labeled while loop below is skipped whenever drawFTCaller == call_from_edit
	// find the next free translation section, scanning forward 
    // (BEW added additional code on 13Jul09, to prevent scanning beyond the visible extent
    // of the view window - for big documents that wasted huge slabs of time)
a:	while ((pPile != NULL) && (!pPile->GetSrcPhrase()->m_bStartFreeTrans))
	{
		pPile = m_pView->GetNextPile(pPile);

		#ifdef _Trace_DrawFreeTrans
		#ifdef __WXDEBUG__
		if (pPile)
		{
			wxLogDebug(_T("while   sn = %d  ,  word =  %s"), pPile->GetSrcPhrase()->m_nSequNumber,
						pPile->GetSrcPhrase()->m_srcPhrase.c_str());
			wxLogDebug(_T("  Has? %d , Start? %d\n"), pPile->GetSrcPhrase()->m_bHasFreeTrans,
								pPile->GetSrcPhrase()->m_bStartFreeTrans);
		}
		#endif
		#endif

		// BEW added 13Jul09, a test to determine when pPile's strip's top is greater than
		// the bottom coord (in logical coord space) of the view window - when it is TRUE,
		// we break out of the loop. Otherwise, if the document has no free translations
		// ahead, the scan goes to the end of the document - and for documents with
		// 30,000+ piles, this can take a very very long time!
		if (pPile == NULL)
		{
			// at doc end, so destroy the elements and we are done, so return
			DestroyElements(m_pFreeTransArray); // don't leak memory
			return;
		}
		CStrip* pStrip = pPile->GetStrip();
		int nStripTop = pStrip->Top();

		#ifdef _Trace_DrawFreeTrans
		#ifdef __WXDEBUG__
		wxLogDebug(_T(" a: scan in visible part: srcPhrase %s , sequ num %d, strip index %d , nStripTop (logical) %d \n              grectViewClient bottom %d logicalViewClientBottom %d"),
			pPile->GetSrcPhrase()->m_srcPhrase, pPile->GetSrcPhrase()->m_nSequNumber, pPile->GetStripIndex(),
			nStripTop,grectViewClient.GetBottom(),logicalViewClientBottom);
		#endif
		#endif

		// when scrolled, grecViewClient is unchanged as it is device coords, so we have
		// to convert the Top coord (ie. y value) to logical coords for the tests
		if (nStripTop >= logicalViewClientBottom)
		{
			// the strip is below the bottom of the view rectangle, stop searching forward
			DestroyElements(m_pFreeTransArray); // don't leak memory
			
			#ifdef _Trace_DrawFreeTrans
			#ifdef __WXDEBUG__
			wxLogDebug(_T(" a: RETURNING because OFF WINDOW now;  nStripTop  %d ,  logicalViewClientBottom  %d"),
				nStripTop, logicalViewClientBottom);
			#endif
			#endif
			return;
		}
	}

	// did we find a free translation section?
ed:	if (pPile == NULL)
	{
		#ifdef _Trace_DrawFreeTrans
		#ifdef __WXDEBUG__
		wxLogDebug(_T("** EXITING due to null pile while scanning ahead **\n"));
		#endif
		#endif

		// there are as yet no free translations in this doc, or we've come to its end
		DestroyElements(m_pFreeTransArray); // don't leak memory
		return;
	}
	pSrcPhrase = pPile->GetSrcPhrase();

#ifdef DrawFT_Bug
	wxLogDebug(_T(" ed: scan ahead: srcPhrase %s , sequ num %d, active sn %d  nThumbPosition_InPixels = %d"),
		pSrcPhrase->m_srcPhrase, pSrcPhrase->m_nSequNumber, pApp->m_nActiveSequNum, nThumbPosition_InPixels);
#endif

    // if we get here, we've found the next one's start - save the pile for later on (we
    // won't use it until we are sure it's free translation data is to be written within
    // the client rectangle of the view)
	m_pFirstPile = pPile;

    // create the elements (each a struct containing int horizExtent and wxRect subRect)
    // which define the places where the free translation substrings are to be written out,
    // and initialize the strip and pile parameters for the loop
	pStrip = pPile->GetStrip();
	curStripIndex = pStrip->GetStripIndex();
	curPileIndex = pPile->GetPileIndex();
	curPileCount = pStrip->GetPileCount();
	pElement = new FreeTrElement; // this struct is defined in CAdapt_ItView.h
	rect = pStrip->GetFreeTransRect(); // start with the full rectangle, 
									   // and reduce as required below
	nTotalHorizExtent = 0;
	bSectionIntersects = FALSE; // TRUE when the section being laid out intersects 
								// the window's client area
	#ifdef _Trace_DrawFreeTrans
	#ifdef __WXDEBUG__
	wxLogDebug(_T("curPileIndex  %d  , curStripIndex  %d  , curPileCount %d  "),
				curPileIndex,curStripIndex,curPileCount);
	#endif
	#endif

	if (gbRTL_Layout)
	{
        // source is to be laid out right-to-left, so free translation rectangles will be
        // altered in location from what would be the case for a LTR layout
		rect.SetRight(pPile->GetPileRect().GetRight()); // this fixes where the writable 
														// area starts
        //  is this pile the ending pile for the free translation section?
e:		if (pSrcPhrase->m_bEndFreeTrans)
		{
            // whether we make the left boundary of rect be the left of the pile's
            // rectangle, or let it be the leftmost remainder of the strip's free
            // translation rectangle, depends on whether or not this pile is the last in
            // the strip - found out, and set the .left parameter accordingly
			if (curPileIndex == curPileCount - 1)
			{
				// last pile in the strip, so use the full width (so no change to rect
				// is needed)
				;
			}
			else
			{
                // more piles to the left, so terminate the rectangle at the pile's left
                // boundary REMEMBER!! When an upper left coordinate of an existing wxRect
                // is set to a different value (with intent to change the rect's size as
                // well as its position), we must also explicitly change the width/height
                // by the same amount. Here the correct width of rect is critical because
                // in RTL we want to use the upper right coord of rect, and transform its
                // value to the mirrored coordinates of the underlying canvas.
				rect.SetLeft(pPile->GetPileRect().GetLeft()); // this only moves the rect
				rect.SetWidth(abs(pStrip->GetFreeTransRect().GetRight() - 
								                       pPile->GetPileRect().GetLeft()));
																// used abs to make sure
			}
            // store in the pElement's subRect member (don't compute the substring yet, to
            // save time since the rect may not be visible), add the element to the pointer
            // array
			pElement->subRect = rect;
			pElement->horizExtent = rect.GetWidth(); 
			nTotalHorizExtent += pElement->horizExtent;
			m_pFreeTransArray->Add(pElement);

            // determine whether or not this free translation section is going to have to
            // be written out in whole or part
			testRect = grectViewClient; // need a scratch testRect, since its values are 
                            // changed in the following test for intersection, so can't use
                            // grectViewClient 
			// BEW added 13Jul09 convert the top (ie. y) to logical coords; x, width and
			// height need no conversion as they are unchanged by scrolling
            testRect.SetY(nThumbPosition_InPixels);
            
			// The intersection is the largest rectangle contained in both existing
			// rectangles. Hence, when IntersectRect(&r,&rectTest) returns TRUE, it
			// indicates that there is some client area to draw on.
			if (testRect.Intersects(rect)) //if (bIntersects)
				bSectionIntersects = TRUE; // we'll have to write out at least this much
										   // of this section
			goto b; // exit the loop for constructing the drawing rectangles
		}
		else
		{
            // the current pile is not the ending one, so check the next - also check out
            // when a strip changes, and restart there with a new rectangle calculation
            // after saving the earlier element
			if (curPileIndex == curPileCount - 1)
			{
				// we are at the end of the strip, so we have to close off the current 
				// rectangle and store it
				pElement->subRect = rect;
				pElement->horizExtent = rect.GetWidth(); 
				nTotalHorizExtent += pElement->horizExtent;
				m_pFreeTransArray->Add(pElement);

				// will we have to draw this rectangle's content?
				testRect = grectViewClient;
				testRect.SetY(nThumbPosition_InPixels); // correct if scrolled
				if (testRect.Intersects(rect)) 
					bSectionIntersects = TRUE;

                // are there more strips? (we may have come to the end of the doc) (for
                // a partial section at doc end, we just show as much of it as we
                // possibly can)
				if (curStripIndex == pLayout->GetStripCount() - 1)
				{
                    // there are no more strips, so this free translation section will be
                    // truncated to whatever rectangles we've set up so far
					goto b;
				}
				else
				{
                    // we are not yet at the end of the strips, so we can be sure there is
                    // a next pile so get it, and its sourcephrase pointer
					wxASSERT(curStripIndex < pLayout->GetStripCount() - 1);
					pPile = m_pView->GetNextPile(pPile);
					wxASSERT(pPile);
					pSrcPhrase = pPile->GetSrcPhrase();

					// initialize rect to the new strip's free translation rectangle, and
					// reinitialize the strip and pile parameters for this new strip
					pStrip = pPile->GetStrip();
					curStripIndex = pStrip->GetStripIndex();
					curPileCount = pStrip->GetPileCount();
					curPileIndex = pPile->GetPileIndex();
					// get a new element
					pElement = new FreeTrElement;
					//rect = pStrip->m_rectFreeTrans; 
					rect = pStrip->GetFreeTransRect(); // rect.right is already correct,
													   // since this is pile[0]
                    // this new pile might be the one for the end of the free translation
                    // section, so check it out
					goto e;
				}
			}
			else
			{
				// there is at least one more pile in this strip, so check it out
				pPile = m_pView->GetNextPile(pPile);
				wxASSERT(pPile);
				pSrcPhrase = pPile->GetSrcPhrase();
				curPileIndex = pPile->GetPileIndex();
				goto e;
			}
		}
	} // end RTL layout block
	else
	{
        // LTR layout, and this is the only option for the non-unicode application
        // REMEMBER!! When an upper left coordinate of an existing wxRect is set to a
        // different value (with intent to change the rect's size as well as its position,
        // we must also explicitly change the width/height by the same amount. The ending
        // width of the rect here has no apparent affect on the resulting text being
        // displayed because only the upper left coordinates in LTR are significant in
        // DrawText operations below
		rect.SetLeft(pPile->GetPileRect().GetLeft()); // fixes where the writable area starts
		rect.SetWidth(abs(pStrip->GetFreeTransRect().GetRight() - 
												pPile->GetPileRect().GetLeft())); 
        // used abs to make sure is this pile the ending pile for the free translation
        // section?

#ifdef DrawFT_Bug
		wxLogDebug(_T(" LTR block: RECT: Left %d , TOP %d, WIDTH %d , Height %d  (logical coords)"),
			rect.x, rect.y, rect.width, rect.height);
#endif

d:		if (pSrcPhrase->m_bEndFreeTrans)
		{

 #ifdef DrawFT_Bug
			wxLogDebug(_T(" after d:  At end of section, so test for intersection follows:"));
#endif
            // whether we make the right boundary of rect be the end of the pile's
            // rectangle, or let it be the remainder of the strip's free translation
            // rectangle, depends on whether or not this pile is the last in the strip -
            // found out, and set the .right parameter accordingly
			if (curPileIndex == curPileCount - 1)
			{
				// last pile in the strip, so use the full width (so no change to 
				// rect is needed)
				;
			}
			else
			{
				// more piles to the right, so terminate the rectangle at the pile's 
				// right boundary
				rect.SetRight(pPile->GetPileRect().GetRight());
			}
            // store in the pElement's subRect member (don't compute the substring yet, to
            // save time since the rect may not be visible), add the element to the pointer
            // array
			pElement->subRect = rect;
			pElement->horizExtent = rect.GetWidth(); 
			nTotalHorizExtent += pElement->horizExtent;
			m_pFreeTransArray->Add(pElement);

            // determine whether or not this free translation section is going to have to
            // be written out in whole or part
			testRect = grectViewClient;

			// BEW added 13Jul09 convert the top (ie. y) to logical coords
			testRect.SetY(nThumbPosition_InPixels);

#ifdef DrawFT_Bug
		wxLogDebug(_T(" LTR block: grectViewClient at test: L %d , T %d, W %d , H %d  (logical coords)"),
			testRect.x, testRect.y, testRect.width, testRect.height);
#endif

			if (testRect.Intersects(rect))
			{
				bSectionIntersects = TRUE; // we'll have to write out at least this much 
										   // of this section
			}

#ifdef DrawFT_Bug
			if (bSectionIntersects)
				wxLogDebug(_T(" Intersects?  TRUE  and goto b:"));
			else
				wxLogDebug(_T(" Intersects?  FALSE  and goto b:"));
#endif
			goto b; // exit the loop for constructing the drawing rectangles
		}
		else
		{
            // the current pile is not the ending one, so check the next - also check out
            // when a strip changes, and restart there with a new rectangle calculation
            // after saving the earlier element
			if (curPileIndex == curPileCount - 1)
			{
				// we are at the end of the strip, so we have to close off the current 
				// rectangle and store it
				pElement->subRect = rect;
				pElement->horizExtent = rect.GetWidth();
				nTotalHorizExtent += pElement->horizExtent;
				m_pFreeTransArray->Add(pElement);

				// will we have to draw this rectangle's content?
				testRect = grectViewClient;
				testRect.SetY(nThumbPosition_InPixels);
				if (testRect.Intersects(rect))
					bSectionIntersects = TRUE;

                // are there more strips? (we may have come to the end of the doc) (for a
                // partial section at doc end, we just show as much of it as we possibly
                // can)
				if (curStripIndex == pLayout->GetStripCount() - 1)
				{
                    // there are no more strips, so this free translation section will be
                    // truncated to whatever rectangles we've set up so far
					goto b;
				}
				else
				{
                    // we are not yet at the end of the strips, so we can be sure there is
                    // a next pile so get it, and its sourcephrase pointer
					wxASSERT(curStripIndex < pLayout->GetStripCount() - 1);
					pPile = m_pView->GetNextPile(pPile);
					wxASSERT(pPile != NULL); 
					pSrcPhrase = pPile->GetSrcPhrase();

					// initialize rect to the new strip's free translation rectangle, and
					// reinitialize the strip and pile parameters for this new strip
					pStrip = pPile->GetStrip();
					curStripIndex = pStrip->GetStripIndex();
					curPileCount = pStrip->GetPileCount();
					curPileIndex = pPile->GetPileIndex();
					// get a new element
					pElement = new FreeTrElement;
					rect = pStrip->GetFreeTransRect(); // rect.left is already correct, 
													   // since this is pile[0]
                    // this new pile might be the one for the end of the free translation
                    // section, so check it out
					goto d;
				}
			}
			else
			{
				// there is at least one more pile in this strip, so check it out
				pPile = m_pView->GetNextPile(pPile);
				wxASSERT(pPile != NULL); 
				pSrcPhrase = pPile->GetSrcPhrase();
				curPileIndex = pPile->GetPileIndex();

#ifdef DrawFT_Bug
				wxLogDebug(_T(" iterating in strips:  another pile is there, with index = %d, srcPhrase = %s , and now going to d:"),
					pPile->GetPileIndex(), pPile->GetSrcPhrase()->m_srcPhrase);
#endif
				goto d;
			}
		}
	} // end LTR layout block

	// rectangle calculations are finished, and stored in 
	// FreeTrElement structs in m_pFreeTransArray
b:	if (!bSectionIntersects)
	{
        // nothing in this current section of free translation is visible in the view's
        // client rectangle so if we are not below the bottom of the latter, iterate to
        // handle the next section, but if we are below it, then we don't need to bother
        // with futher calculations & can return immediately
		pElement = (FreeTrElement*)m_pFreeTransArray->Item(0);
		// for next line... view only, MM_TEXT mode, y-axis positive downwards
		
#ifdef DrawFT_Bug
				wxLogDebug(_T(" NO INTERSECTION block:  Testing, is  top %d still above window bottom %d ?"),
					pElement->subRect.GetTop(), logicalViewClientBottom);
#endif

		if (pElement->subRect.GetTop() > logicalViewClientBottom) 
		{
			#ifdef _Trace_DrawFreeTrans
			#ifdef __WXDEBUG__
			wxLogDebug(_T("No intersection, *** and below bottom of client rect, so return ***"));
			#endif
			#endif
			DestroyElements(m_pFreeTransArray); // don't leak memory

#ifdef DrawFT_Bug
			wxLogDebug(_T(" NO INTERSECTION block:  Returning, as top is below grectViewClient's bottom"));
#endif
			return; // we are done
		}
		else
		{
			#ifdef _Trace_DrawFreeTrans
			#ifdef __WXDEBUG__
			wxLogDebug(_T(" NO INTERSECTION block:  top is still above grectViewClient's bottom, so goto c: then to a: and iterate"));
			//wxLogDebug(_T("No intersection, so iterating loop..."));
			#endif
			#endif

			DestroyElements(m_pFreeTransArray); // don't leak memory

#ifdef DrawFT_Bug
			wxLogDebug(_T(" NO INTERSECTION block:  top is still above grectViewClient's bottom, so goto c: then to a: and iterate"));
#endif
			goto c;
		}
	}

    // the whole or part of this section must be drawn, so do the
    // calculations now; first, get the free translation text
	pSrcPhrase = m_pFirstPile->GetSrcPhrase();
	offset = 0;
	length = 0;
#ifdef _DOCVER5
	ftStr = pSrcPhrase->GetFreeTrans();
	length = ftStr.Len();
#else
	ftStr = GetExistingMarkerContent(theMkr, theEndMkr, pSrcPhrase, offset, length);
#endif

#ifdef DrawFT_Bug
	wxLogDebug(_T(" Drawing ftSstr =  %s ; for srcPhrase  %s  at sequ num  %d"), ftStr,
		pSrcPhrase->m_srcPhrase, pSrcPhrase->m_nSequNumber);
#endif

#ifndef _DOCVER5

	// whm note: length is the length of the free trans string within the m_markers member.
    // Since Bruce has globals tracking these, and since under certain circumstances (i.e.,
    // when an editing change is made by typing over and replacing a selection in which
    // case it is two successive edit operations - first delete of the selection and second
    // addition of the replacement) there can be two calls to our OnEditBoxChanged for a
    // single editing action, we need to keep the globals in sync here The length needs to
    // be the length BEFORE padding with spaces (done below)
	m_nLengthInMarkersStr = length;
	// offset is the position of the free trans string within the m_markers member
	m_nOffsetInMarkersStr = offset;
#endif

	#ifdef _Trace_DrawFreeTrans
	#ifdef __WXDEBUG__
	wxLogDebug(_T("(Line 40726)Sequ Num  %d  ,  Free Trans:  %s "), pSrcPhrase->m_nSequNumber, ftStr.c_str());
	#endif
	#endif

    // whm changed 24Aug06 when called from OnEnChangedEditBox, we need to be able to allow
    // user to delete the contents of the edit box, and draw nothing, so we'll not jump out
    // early here because the new length is zero.
	if (drawFTCaller == call_from_ondraw)
	{
		if (length == 0)
		{
			// there is no text to be written to the screen, 
			// so continue with the next free translation section
			goto c;
		}

		// trim off any leading or trailing spaces
		ftStr.Trim(FALSE); // trim left end
		ftStr.Trim(TRUE); // trim right end
		if (ftStr.IsEmpty())
		{
			// nothing to write, so move on
			goto c;
		}
	}
    // get text's extent (a wxSize object) and compare to the total horizontal extent of
    // the rectangles. also determine the number of rectangles we are to write this section
    // into, and initialize other needed data
	pDC->GetTextExtent(ftStr,&extent.x,&extent.y);
	bTextIsTooLong = extent.x > nTotalHorizExtent ? TRUE : FALSE;
	totalRects = m_pFreeTransArray->GetCount();

	if (totalRects == 1)
	{
		// the easiest case, the whole free translation section is contained within a 
		// single strip
		pElement = (FreeTrElement*)m_pFreeTransArray->Item(0);
		if (bTextIsTooLong)
		{
			ftStr = TruncateToFit(pDC,ftStr,ellipsis,nTotalHorizExtent);
		}

		// next section:   Draw Single Strip Free Translation Text
		
        // clear only the subRect; this effectively allows for the erasing from the display
        // of any deleted text from the free translation string; even though this clearing
        // of the subRect is only technically needed before deletion edits, it doesn't hurt
        // to do it before every edit/keystroke. It works for either RTL or LTR text
        // displays.
		pDC->DestroyClippingRegion();
		pDC->SetClippingRegion(pElement->subRect);
		pDC->Clear();
		pDC->DestroyClippingRegion();
		if (bRTLLayout)
		{
//#ifdef __WXDEBUG__
//			wxSize trueSz;
//			pDC->GetTextExtent(ftStr,&trueSz.x,&trueSz.y);
//			wxLogDebug(_T("RTL DrawText sub.l=%d + sub.w=%d - 
//			                                     ftStrExt.x=%d, x=%d, y=%d of %s"),
//				pElement->subRect.GetLeft(),pElement->subRect.GetWidth(),trueSz.x,
//				pElement->subRect.GetLeft()+pElement->subRect.GetWidth()-trueSz.x,
//				pElement->subRect.GetTop(),ftStr.c_str());
//#endif
			pView->DrawTextRTL(pDC,ftStr,pElement->subRect);
		}
		else
		{

#ifdef DrawFT_Bug
			wxLogDebug(_T(" *** Drawing ftSstr at:  Left  %d   Top  %d  These are logical coords."),
				pElement->subRect.GetLeft(), pElement->subRect.GetTop());
			wxLogDebug(_T(" *** Drawing ftSstr: m_pFirstPile's Logical Rect x= %d  y= %d  width= %d  height= %d   PileHeight + 2: %d"),
				m_pFirstPile->Left(), m_pFirstPile->Top(), m_pFirstPile->Width(), m_pFirstPile->Height(), m_pFirstPile->Height() + 2);
#endif

			pDC->DrawText(ftStr,pElement->subRect.GetLeft(),pElement->subRect.GetTop());
		}
	}
	else
	{
        // the free translation is spread over at least 2 strips - so we've more work to do
        // - call SegmentFreeTranslation() to get a string array returned which has the
        // passed in frStr cut up into appropriately sized segments (whole words in each
        // segment), truncating the last segment if not all the ftStr data can be fitted
        // into the available drawing rectangles
		#ifdef _Trace_DrawFreeTrans
		#ifdef __WXDEBUG__
		wxLogDebug(_T("call  ** SegmentFreeTranslation() **  Free Trans:  %s "), ftStr.c_str());
		#endif
		#endif

		SegmentFreeTranslation(pDC,ftStr,ellipsis,extent.GetWidth(),nTotalHorizExtent,
								m_pFreeTransArray,&subStrings,totalRects);

		#ifdef _Trace_DrawFreeTrans
		#ifdef __WXDEBUG__
			wxLogDebug(_T("returned from:  SegmentFreeTranslation() *** MultiStrip Draw *** "));
		#endif
		#endif


#ifdef DrawFT_Bug
			wxLogDebug(_T(" Drawing ftSstr *** MultiStrip Draw ***"));
#endif

		// draw the substrings in their respective rectangles
		int index;
		for (index = 0; index < totalRects; index++)
		{
			// get the next element
			pElement = (FreeTrElement*)m_pFreeTransArray->Item(index);
			// get the string to be drawn in its rectangle
			wxString s = subStrings.Item(index);

			// draw this substring
			// this section:  Draw Multiple Strip Free Translation Text
			
            // clear only the subRect; this effectively allows for the erasing from the
            // display of any deleted text from the free translation string; even though
            // this clearing of the subRect is only technically needed before deletion
            // edits, it doesn't hurt to do it before every edit/keystroke. It works for
            // either RTL or LTR text displays.
			pDC->DestroyClippingRegion();
			pDC->SetClippingRegion(pElement->subRect);
			pDC->Clear();
			pDC->DestroyClippingRegion();
			if (bRTLLayout)
			{
//#ifdef __WXDEBUG__
//				wxSize trueSz;
//				pDC->GetTextExtent(s,&trueSz.x,&trueSz.y);
//				wxLogDebug(_T("RTL DrawText sub.l=%d + sub.w=%d - sExt.x=%d, x=%d, y=%d of %s"),
//					pElement->subRect.GetLeft(),pElement->subRect.GetWidth(),trueSz.x,
//					pElement->subRect.GetLeft()+pElement->subRect.GetWidth()-trueSz.x,
//					pElement->subRect.GetTop(),s.c_str());
//#endif
				pView->DrawTextRTL(pDC,s,pElement->subRect);
			}
			else
			{
				pDC->DrawText(s,pElement->subRect.GetLeft(),pElement->subRect.GetTop());
			}
			// Cannot call Invalidate() or SendSizeEvent from within DrawFreeTranslations
			// because it triggers a paint event which results in a Draw() which results
			// in DrawFreeTranslations() being reentered... hence a run-on condition 
			// endlessly calling the View's OnDraw.
		}

		subStrings.Clear(); // clear the array ready for the next iteration
		#ifdef _Trace_DrawFreeTrans
		#ifdef __WXDEBUG__
			wxLogDebug(_T("subStrings drawn - finished them "));
		#endif
		#endif

	}

	if (drawFTCaller == call_from_edit)
	{
		// drawing of the one free translation being edited is done so return
		DestroyElements(m_pFreeTransArray); // don't leak memory
		return;
	}

	// the section has been dealt with, get the next pile and iterate
c:	pPile = m_pView->GetNextPile(pPile);
	if (pPile != NULL)
		pSrcPhrase = pPile->GetSrcPhrase();
	DestroyElements(m_pFreeTransArray);

		#ifdef DrawFT_Bug
			wxLogDebug(_T(" At c: next pPile is --  srcphrase %s, sn = %d, going now to a:"),
				pSrcPhrase->m_srcPhrase, pSrcPhrase->m_nSequNumber);
		#endif
		#ifdef _Trace_DrawFreeTrans
		#ifdef __WXDEBUG__
			wxLogDebug(_T(" At c: next pPile is --  srcphrase %s, sn = %d, going now to a:"),
				pSrcPhrase->m_srcPhrase, pSrcPhrase->m_nSequNumber);
		#endif
		#endif

	goto a;
}

// when the phrase box lands at the anchor location, it may clear the m_bHasKBEntry flag,
// or the m_bHasGlossingKBEntry flag when glossing mode is on, and if there is an
// adaptation (or gloss) there when the phrase box is subsequently moved, we must make sure
// the flag has the appropriate value
// BEW 22Feb10 no changes needed for support of _DOCVER5
void CFreeTrans::FixKBEntryFlag(CSourcePhrase* pSrcPhr)
{
	if (gbIsGlossing)
	{
		if (pSrcPhr->m_bHasGlossingKBEntry == FALSE)
		{
			// might be wrong value, so check and set it if necessary
			if (!pSrcPhr->m_gloss.IsEmpty())
			{
				// we need to reset it
				pSrcPhr->m_bHasGlossingKBEntry = TRUE;
			}
		}
	}
	else // we are in adapting mode
	{
		if (pSrcPhr->m_bHasKBEntry == FALSE)
		{
			// might be wrong value, so check and set it if necessary
			if (!pSrcPhr->m_targetStr.IsEmpty() && !pSrcPhr->m_bNotInKB)
			{
				// we need to reset it
				pSrcPhr->m_bHasKBEntry = TRUE;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return             TRUE if the passed in word or phrase has word-final punctuation 
///                     at its end, else FALSE
///
///	\param pSP		->	pointer to the CSourcePhrase instance which stores the phrase 
///	                    parameter as a member
///	\param phrase	->	the word or phrase being considered (actually, pSP->m_targetStr)
///	\param punctSet ->	reference to a string of target language punctuation characters 
///	                    containing no spaces
/// \remarks
/// We can't simply search for a non-empty m_follPunct member of pSrcPhrase in order to end
/// a free translation section, because if we have a lot of typing in a retranslation, then
/// there will be several final placeholders in the source text line, and these will have
/// their m_follPunct members empty; so we must check for final punctuation in the target
/// text line instead, and the only way to do this is to look for final punctuation in the
/// m_targetStr member of pSrcPhrase - this will have content, even throughout a
/// retranslation
/// BEW modified 25Nov05; the above algorithm breaks down in document sections which have
/// not yet been adapted, because then there is no target text to examine! So when the
/// m_targetStr member is empty, we will indeed instead check for a non-empty m_follPunct
/// member!
/// BEW 19Feb10, no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
bool CFreeTrans::HasWordFinalPunctuation(CSourcePhrase* pSP, wxString phrase, 
											wxString& punctSet)
{
	// beware, phrase can sometimes have a final space following punctuation - so first
	// remove trailing spaces
	phrase = MakeReverse(phrase);
	while (phrase.GetChar(0) == _T(' ')) // remove (now initial) spaces, if any
		phrase.Remove(0,1); // need second param of 1 otherwise will truncate
	wxString endingPuncts;
	if (phrase.IsEmpty())
	{
		if (!pSP->m_follPunct.IsEmpty())
			return TRUE;
		else
			return FALSE;
	}
	else
	{
		endingPuncts = SpanIncluding(phrase, punctSet); 
		return !endingPuncts.IsEmpty();
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return             TRUE if the 'next' sourcephrase pointed at by the passed in 
///                     pile pointer contains in its m_markers member a SF marker which
///                     should halt forward scanning for determining the end of the current
///                     section to be free translated; FALSE otherwise
///
///	\param pNextPile	->	pointer to the pile which is one position further along in the 
///                         list than where control happens to be in the caller (so if TRUE
///                         is returned, that passed in pile will be excluded from the
///                         current free translation section being delimited, and scanning
///                         will stop)
/// \remarks
/// We don't want a situation, such as in introductory material at the start of a book
/// where there are no verses defined, and perhaps limited or no punctuation as well, for
/// scanning to find an endpoint for the current section to fail to find some criterion for
/// termination of the section - which would easily happen if we ignored SF markers - and
/// we'd get overrun of the section into quite different kinds of information. So we'll
/// halt scanning when there is a marker, but not when the marker is an endmarker for a
/// marker with TextType none, nor when it is a beginning marker which has a TextType of
/// none - the latter we want Adapt It to treat as if they are 'not there' for most
/// purposes. And we'll not halt at embedded markers within a footnote (\f) or cross
/// reference (\x) section either, but certainly halt when there is \f* or (PNG set's \fe
/// or \F) or \x* on the 'next' sourcephrase passed in. BEW changed 22Dec07: a filtered
/// Note can be anywhere, and we don't want these to needlessly halt section delineation,
/// so we'll ignore \note and \note* as well.
/// 
/// BEW modified 19Feb10, for support of _DOCVER5. New version does not have endmarkers in
/// m_markers any more, but in m_endMarkers (and only endmarkers there, else it is empty)
/////////////////////////////////////////////////////////////////////////////////
#ifdef _DOCVER5
bool CFreeTrans::IsFreeTranslationEndDueToMarker(CPile* pThisPile)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	USFMAnalysis* pAnalysis = NULL;
	wxString bareMkr;
	CAdapt_ItDoc* pDoc = pApp->GetDocument();
	CSourcePhrase* pSrcPhrase = pThisPile->GetSrcPhrase();
	wxString markers = pSrcPhrase->m_markers;
	wxString endMarkers = pSrcPhrase->GetEndMarkers();
	if (markers.IsEmpty() && endMarkers.IsEmpty())
		return FALSE;

	// anything filtered must halt scanning, except for a note's content (in m_note)
	if (
		!pSrcPhrase->GetFilteredInfo().IsEmpty() ||
		!pSrcPhrase->GetFreeTrans().IsEmpty() ||
		!pSrcPhrase->GetCollectedBackTrans().IsEmpty() )
	{
		return TRUE;
	}

	// handle any endmarker - it causes a halt unless it has TextType
	// of none, or is an endmarker for embedded markers in a footnote or cross
	// reference section
	int mkrLen = 0;
	wxString ftnoteMkr = _T("\\f");
	wxString xrefMkr = _T("\\x");
	wxString endnoteMkr = _T("\\fe"); // BEW added 16Jan06
	wxArrayString arrEndMarkers;
	pSrcPhrase->GetEndMarkersAsArray(&arrEndMarkers);
	wxString mkr;
	if (!arrEndMarkers.IsEmpty())
	{
		int count = arrEndMarkers.GetCount();
		int index;
		for (index = 0; index < count; index++)
		{
			mkr = arrEndMarkers.Item(index);

			// is it \f* or \x* ? (or if PngOnly is current, \F or \fe ?)
			if (mkr == ftnoteMkr + _T('*'))
				return TRUE; // halt scanning
			if (mkr == xrefMkr + _T('*'))
				return TRUE;
			if (pApp->gCurrentSfmSet == UsfmOnly && 
					(mkr == endnoteMkr + _T('*')))
				return TRUE;
			if (pApp->gCurrentSfmSet == PngOnly && (mkr == _T("\\fe") || 
					mkr == _T("\\F")))
				return TRUE;

			// find out if it is an embedded marker with TextType of none
			// - we don't halt for these
			mkrLen = mkr.Length();
			bareMkr = mkr;
			bareMkr = bareMkr.Left(mkrLen - 1);
			bareMkr = bareMkr.Mid(1);
			pAnalysis = pDoc->LookupSFM(bareMkr);
			if (pAnalysis == NULL)
				return TRUE; // halt for an unknown endmarker 
							 // (never should be such a thing anyway)
			if (pAnalysis->textType == none)
				continue; // don't halt free translation scanning for these

			// we don't halt for embedded endmarkers in footnotes or cross references 
			// (USFM set only) either
			int nFound = mkr.Find(ftnoteMkr); // if >= 0, \f is contained within mkr
			if (nFound >= 0 && mkrLen > 2)
				continue; // must be \fr*, \fk*, etc - so don't halt
			nFound = mkr.Find(xrefMkr); // if >= 0, \x is contained within mkr
			if (nFound >= 0 && mkrLen > 2)
				continue; // must be \xr*, \xt*, \xo*, etc - so don't halt

			// halt for any other endmarker
			return TRUE;
		}
	}

	// now deal with the m_markers member's content - look for halt-causing markers (it's
	// sufficient to find just the first, and make our decision from that one)
	if (!markers.IsEmpty())
	{
		// some kind of non-endmarker(s) is/are present so check it/them out
		const wxChar* pBuff;
		int bufLen = markers.Length();
		wxChar* pBufStart;
		wxChar* ptr;
		int curPos = wxNOT_FOUND;
		curPos = markers.Find(gSFescapechar);
		if (curPos == wxNOT_FOUND)
		{
			// there are no SF markes left in the string
			return FALSE; // don't halt free translation scanning
		}

		// we've a marker to deal with, and its not a filtered one 
		// - so we'll halt now unless the marker is one like \k , or 
		// \it , or \sc , or \bd etc - these have TextType none
		bufLen = markers.Length();
		pBuff = markers.GetData(); //GetWriteBuf(bufLen + 1);
		pBufStart = (wxChar*)pBuff;
		wxChar* pEnd;
		pEnd = pBufStart + bufLen; // whm added
		wxASSERT(*pEnd == _T('\0')); // whm added
		ptr = pBufStart + curPos;
		bareMkr = pDoc->GetBareMarkerForLookup(ptr);
		pAnalysis = pDoc->LookupSFM(bareMkr);
		if (pAnalysis == NULL)
			// an unknown marker should halt scanning
			return TRUE;
		if (pAnalysis->textType == none)
			return FALSE; // don't halt scanning for these

		// if it's an embedded marker in a footnote or cross reference section,
		// then these don't halt scanning
		wxString mkr = _T("\\") + bareMkr; // this is genuine backslash not PathSeparator
		mkrLen = mkr.Length();
		int nFound = mkr.Find(ftnoteMkr); // if true, \f is contained within mkr
		if (nFound >= 0 && mkrLen > 2)
			return FALSE; // must be \fr, \fk, etc - so don't halt
		nFound = mkr.Find(xrefMkr); // if true, \x is contained within mkr
		if (nFound >= 0 && mkrLen > 2)
			return FALSE; // must be \xr*, \xt*, \xo*, etc - so don't halt

		return TRUE; // anything else should halt scanning
	}
	// otherwise we assume there is nothing of significance for causing a halt
	return FALSE;
}
#else
bool CFreeTrans::IsFreeTranslationEndDueToMarker(CPile* pNextPile)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxString noteMkr = _T("\\note"); // BEW added 22Dec07
	USFMAnalysis* pAnalysis = NULL;
	wxString bareMkr;
	CAdapt_ItDoc* pDoc = pApp->GetDocument();
	wxString markers = pNextPile->GetSrcPhrase()->m_markers;
	if (markers.IsEmpty())
		return FALSE;

	// anything filtered must halt scanning, exept for a \note & its content
	wxString fltr = filterMkr; // \~FILTER
	int fltrPos = FindFromPos(markers,fltr,0);
	if (fltrPos != -1)
	{
        // since \~FILTER has been found, it could be for a Note; but it might be something
        // else which is filtered, or there might be a Note present plus other filtered
        // information (in which case the presence of the other filtered stuff must halt
        // the section delineation). So only when there is a single bit of filtered info
        // and it is a Note do we let processing continue, otherwise halt. It is sufficient
        // to test for \note, then branch according to whether or not there are least 3
        // instances of \~FILTER, in markers.
		if (FindFromPos(markers,noteMkr,0) != -1)
		{
            // there is a note filtered here; check for other filtered information too,
            // there needs to be more than two instances of \~FILTER (first is the
            // beginning marker, the second is its endmarker \~FILTER*) for that to be TRUE
			fltrPos = FindFromPos(markers,fltr,++fltrPos);
			if (fltrPos == -1)
			{
				// could happen, eg. a filtered \bt has no matching endmarker, 
				// & it halts progress
				return TRUE;
			}
			else
			{
				// matched two instances of \~FILTER, try for a third
				fltrPos = FindFromPos(markers,fltr,++fltrPos);
				if (fltrPos != -1)
				{
					return TRUE; // there is other filtered info present
				}
				else
				{
					// there were only two, so they were the 
					// beginning marker and its endmarker
					return FALSE;
				}
			}
		}
		else
			// no \note marker is in markers, so the filtered material
			// is something else and so we must halt here
			return TRUE;
	}

	const wxChar* pBuff;
	int bufLen = markers.Length();
	wxChar* pBufStart;
	wxChar* ptr;
	int itemLen;
	int curPos = -1;
	wxString ftnoteMkr = _T("\\f");
	wxString xrefMkr = _T("\\x");
	wxString endnoteMkr = _T("\\fe"); // BEW added 16Jan06
	int mkrLen = 0;

	// get rid of any initial spaces
	while (markers.GetChar(0) == _T(' '))
	{
		markers.Remove(0,1);
	}

	// handle any initial endmarker - it causes a halt unless it has TextType
	// of none, or is an endmarker for embedded markers in a footnote or cross
	// reference section, (BEW added:) or is \note*
	if (markers.GetChar(0) == gSFescapechar)
	{
		pBuff = markers.GetData();
		pBufStart = (wxChar*)pBuff;
		wxChar* pEnd;
		pEnd = pBufStart + bufLen; // whm added
		wxASSERT(*pEnd == _T('\0')); // whm added
		itemLen = pDoc->ParseMarker(pBufStart);
		wxString mkr(pBufStart,itemLen);
		mkr = MakeReverse(mkr);
		bool bIsEndMkr = FALSE;
		if (mkr.GetChar(0) == _T('*'))
		{
			// markers begins with an endmarker, determine
			// whether it is one which halts scanning or not
			itemLen++; // increment to encompass the following space
			bIsEndMkr = TRUE;
		}
		mkr = MakeReverse(mkr);
		if (bIsEndMkr)
		{
			// is it \f* or \x* ? (or if PngOnly is current, \F or \fe ?)
			if (mkr == ftnoteMkr + _T('*'))
				return TRUE; // halt scanning
			if (mkr == xrefMkr + _T('*'))
				return TRUE;
			if (pApp->gCurrentSfmSet == UsfmOnly && 
					(mkr == endnoteMkr + _T('*')))
				return TRUE;
			if (pApp->gCurrentSfmSet == PngOnly && (mkr == _T("\\fe") || 
					mkr == _T("\\F")))
				return TRUE;

			// find out if it is an embedded marker with TextType of none
			// - we don't halt for these nor for a \note*
			if (mkr == noteMkr + _T('*'))
				return FALSE; // don't halt scanning for this
			mkrLen = mkr.Length();
			bareMkr = mkr;
			bareMkr = bareMkr.Left(mkrLen - 1);
			bareMkr = bareMkr.Mid(1);
			pAnalysis = pDoc->LookupSFM(bareMkr);
			if (pAnalysis == NULL)
				return TRUE; // halt for an unknown endmarker 
							 // (never should be such a thing anyway)
			if (pAnalysis->textType == none)
				return FALSE; // don't halt scanning for these

			// we don't halt for embedded endmarkers in footnotes or cross references 
			// (USFM set only) either
			int nFound = mkr.Find(ftnoteMkr); // if true, \f is contained within mkr
			if (nFound >= 0 && mkrLen > 2)
				return FALSE; // must be \fr*, \fk*, etc - so don't halt
			nFound = mkr.Find(xrefMkr); // if true, \x is contained within mkr
			if (nFound >= 0 && mkrLen > 2)
				return FALSE; // must be \xr*, \xt*, \xo*, etc - so don't halt

			// halt for any other endmarker
			return TRUE;
		}
	}

	// something else is present, so check it out
	curPos = markers.Find(gSFescapechar);
	if (curPos == -1)
		// there are no SF markes left in the string
		return FALSE; // don't halt scanning

    // we've a marker to deal with, and its not a filtered one 
    // - so we'll halt now unless the marker is one like \k , or 
    // \it , or \sc , or \bd etc - these have TextType none
	bufLen = markers.Length();
	pBuff = markers.GetData(); //GetWriteBuf(bufLen + 1);
	pBufStart = (wxChar*)pBuff;
	wxChar* pEnd;
	pEnd = pBufStart + bufLen; // whm added
	wxASSERT(*pEnd == _T('\0')); // whm added
	ptr = pBufStart + curPos;
	bareMkr = pDoc->GetBareMarkerForLookup(ptr);
	//markers.ReleaseBuffer();
	pAnalysis = pDoc->LookupSFM(bareMkr);
	if (pAnalysis == NULL)
		// an unknown marker should halt scanning
		return TRUE;
	if (pAnalysis->textType == none)
		return FALSE; // don't halt scanning for these

	// if it's an embedded marker in a footnote or cross reference section,
	// then these don't halt scanning
	wxString mkr = _T("\\") + bareMkr; // this is genuine backslash not PathSeparator
	mkrLen = mkr.Length();
	int nFound = mkr.Find(ftnoteMkr); // if true, \f is contained within mkr
	if (nFound >= 0 && mkrLen > 2)
		return FALSE; // must be \fr, \fk, etc - so don't halt
	nFound = mkr.Find(xrefMkr); // if true, \x is contained within mkr
	if (nFound >= 0 && mkrLen > 2)
		return FALSE; // must be \xr*, \xt*, \xo*, etc - so don't halt

	return TRUE; // anything else should halt scanning
}
#endif

/////////////////////////////////////////////////////////////////////////////////
/// \return             TRUE if the sourcephrase's m_bStartFreeTrans BOOL is TRUE, 
///                     FALSE otherwise
///	\param pPile	->	pointer to the pile which stores the pSrcPhrase pointer being 
///	                    examined
///	BEW 19Feb10, no change needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
bool CFreeTrans::IsFreeTranslationSrcPhrase(CPile* pPile)
{
	return pPile->GetSrcPhrase()->m_bStartFreeTrans == TRUE;
}

// utility functions for hooking up to view, app, layout, and frame
CAdapt_ItApp* CFreeTrans::GetApp()
{
	return m_pApp;
}
CAdapt_ItView* CFreeTrans::GetView()
{
	return m_pView;
}
CMainFrame*	CFreeTrans::GetFrame()
{
	return m_pFrame;
}
CLayout* CFreeTrans::GetLayout()
{
	return m_pLayout;
}

void CFreeTrans::OnAdvancedFreeTranslationMode(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = GetApp();
	CMainFrame* pMainFrm = GetFrame();
	CAdapt_ItView* pView = GetView();
	CLayout* pLayout = GetLayout();

	wxASSERT(pMainFrm != NULL);
	wxMenuBar* pMenuBar = pMainFrm->GetMenuBar();
	wxASSERT(pMenuBar != NULL);
	wxMenuItem * pAdvancedMenuFTMode = 
						pMenuBar->FindItem(ID_ADVANCED_FREE_TRANSLATION_MODE);
	wxASSERT(pAdvancedMenuFTMode != NULL);
	gbSuppressSetup = FALSE; // setdefault value

    // determine if the document is unstructured or not -- we'll need this set or cleared
    // as appropriate because in free translation mode the user may elect to end sections
    // at verse breaks - and we can't do that for unstructured data (in the latter case,
    // we'll just end when there is following punctuation on a word or phrase)
	SPList* pSrcPhrases = pApp->m_pSourcePhrases;
	gbIsUnstructuredData = pView->IsUnstructuredData(pSrcPhrases);

	// toggle the setting
	if (pApp->m_bFreeTranslationMode)
	{
		// toggle the checkmark to OFF
		pAdvancedMenuFTMode->Check(FALSE);
		pApp->m_bFreeTranslationMode = FALSE;

        // free translation mode is being turned off, so "fix" the current free translation
        // before the m_pCurFreeTransSectionPileArray contents are invalidated by the
        // RecalcLayout() call within ComposeBarGuts() below
		StoreFreeTranslationOnLeaving();
	}
	else
	{
		// toggle the checkmark to ON
		pAdvancedMenuFTMode->Check(TRUE);
		pApp->m_bFreeTranslationMode = TRUE;
	}
	if (pApp->m_bFreeTranslationMode)
	{
        // put the target punctuation character set into gSpacelessTgtPunctuation to be
        // used in the HasWordFinalPunctuation() function to work out when to end a span of
        // free translation (can't put this after the ComposeBarGuts() call because the
        // latter calls SetupCurrentFreeTransSection(), and it needs
        // gSpacelessTgtPunctuation set up beforehand)
		gSpacelessTgtPunctuation = pApp->m_punctuation[1]; // target set, with 
														   // delimiting spaces
		gSpacelessTgtPunctuation.Remove(gSpacelessTgtPunctuation.Find(_T(' ')),1); // get 
																	// rid of the spaces
	}

	// restore focus to the targetBox, if free translation mode was just turned off,
	// else to the CEdit in the Compose Bar because it has just been turned on
	// -- providing the box or bar is visible and its handle exists
	pMainFrm->ComposeBarGuts(); // open or close the Compose Bar -- it does a 
            // RecalcLayout() call, so if turning off free translation mode, the
            // m_pCurFreeTransSectionPileArray array will store hanging pointers,
            // so don't use it below

	if (pApp->m_bFreeTranslationMode)
	{
        // free translation mode was just turned on. The phrase box might happen to be
        // located within a previously composed section of free translation, but not at
        // that section's anchor point, so we must check for this and if so, iterate back
        // over the piles until we get to the anchor point
		CPile* pPile = pApp->m_pActivePile; // current box location
		CSourcePhrase* pSP = pPile->GetSrcPhrase();
		CKB* pKB;
		if (gbIsGlossing)
			pKB = pApp->m_pGlossingKB;
		else
			pKB = pApp->m_pKB;
		if (pSP->m_bHasFreeTrans && !pSP->m_bStartFreeTrans)
		{
			// save the phrase box text to the KB
			// left it here -- may need to ensure m_targetPhrase has no punct before 
			// passing to StoreTextGoingBack()
			bool bOK;
			bOK = pView->StoreTextGoingBack(pKB,pSP,pApp->m_targetPhrase); // store, so we can 
																// forget this location
		}
		while (pSP->m_bHasFreeTrans && !pSP->m_bStartFreeTrans)
		{
			// iterate backwards to the anchor pile
			CPile* pPrevPile = pView->GetPrevPile(pPile);
			if (pPrevPile == NULL)
			{
				// got to doc start without detecting the start - should never happen
				// so just shove it at start of doc, with no lookup and a beep
				int sn = 0;
				pApp->m_targetPhrase.Empty();
				pPile = pView->GetPile(sn);
				pMainFrm->canvas->ScrollIntoView(sn);
				::wxBell();
				break;
			}
			else
			{
				pPile = pPrevPile;
			}
			pSP = pPile->GetSrcPhrase();
		}
		// we are at the anchor location
		pApp->m_pActivePile = pPile;
		pApp->m_nActiveSequNum = pPile->GetSrcPhrase()->m_nSequNumber;

        // BEW changed 15Oct05; since if we have an adaptation there different than the
        // source text we want to preserve it, rather than unilaterally just use m_key; and
        // if the member is empty, then leave the box empty rather than have the source
        // copied in free translation mode
		if (gbIsGlossing)
		{
			if (pSP->m_gloss.IsEmpty())
			{
				pApp->m_targetPhrase.Empty();
			}
			else
			{
				pApp->m_targetPhrase = pSP->m_gloss;
			}
		}
		else
		{
			if (pSP->m_adaption.IsEmpty())
			{
				pApp->m_targetPhrase.Empty();
			}
			else
			{
				pApp->m_targetPhrase = pSP->m_adaption;
			}
		}
		translation = pApp->m_targetPhrase; // in case we just unmerged, since a 
                        //PlacePhraseBox() call with selector == 1 or 3 will set
                        //m_targetPhrase to whatever is currently in the global string
                        //translation when it jumps the block of code for removing the new
                        //location's entry from the KB
		CCell* pCell = pPile->GetCell(1); // need this for the PlacePhraseBox() call
		pView->PlacePhraseBox(pCell,1); // 1 = inhibit saving at old location, as we did it above
				// instead, and also don't remove the new location's KB entry (as the 
				// phrase box is disabled)

		// prevent clicks and editing being done in phrase box (do also in ResizeBox())
		if (pApp->m_pTargetBox->IsShown() && pApp->m_pTargetBox->GetHandle() != NULL)
			pApp->m_pTargetBox->SetEditable(FALSE);
		pLayout->m_pCanvas->ScrollIntoView(
								pApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber);

          // whm 4Apr09 moved this SetFocus below ScrollIntoView since ScrollIntoView seems
          // to remove the focus on the Compose Bar's edit box if it follows the SetFocus
          // call. now put the focus in the Compose Bar's edit box, and disable the phrase
          // box for clicks & editing, and make it able to right justify and render RTL if
          // we are in the Unicode app
		if (pMainFrm->m_pComposeBar->GetHandle() != NULL)
			if (pMainFrm->m_pComposeBar->IsShown())
			{
				#ifdef _RTL_FLAGS
					// enable complex rendering
					if (pApp->m_bTgtRTL)
					{
						pMainFrm->m_pComposeBarEditBox->SetLayoutDirection(
							wxLayout_RightToLeft);
					}
					else
					{
						pMainFrm->m_pComposeBarEditBox->SetLayoutDirection(
							wxLayout_LeftToRight);
					}
				#endif
				pMainFrm->m_pComposeBarEditBox->SetFocus();

			}

		// get any removed free translations in gEditRecord into the GUI list
		bool bAllsWell;
		bAllsWell = pView->PopulateRemovalsComboBox(freeTranslationsStep, &gEditRecord);
	}
	else
	{
        // if the user exits the mode while the phrase box is within a retranslation, we
        // don't want the box left there (though the app would handle it just fine, no
        // crash or other problem), so check for this and if so, move the active location
        // to a safe place nearby
		int numSrcPhrases;
		int nCountForwards = 0;
		int nCountBackwards = 0;
		int nSaveActiveSequNum;
		if (pApp->m_pActivePile->GetSrcPhrase()->m_bRetranslation)
		{
			// we have to move the box
			CSourcePhrase* pSrcPhr = pApp->m_pActivePile->GetSrcPhrase();
			SPList::Node* pos = pSrcPhrases->Find(pSrcPhr);
			wxASSERT(pos);
			SPList::Node* savePos = pos;
			if (pSrcPhr->m_bBeginRetranslation)
			{
				// we are at the start of the section
				nCountForwards = 1;
				pSrcPhr = (CSourcePhrase*)pos->GetData(); // counted this one
				pos = pos->GetNext();
				while (pos != NULL)
				{
					pSrcPhr = (CSourcePhrase*)pos->GetData();
					pos = pos->GetNext();
					nCountForwards++;
					if (pSrcPhr->m_bEndRetranslation)
						break;
				}
				nSaveActiveSequNum = pSrcPhr->m_nSequNumber + 1;
				numSrcPhrases = nCountForwards;
			}
			else if (pSrcPhr->m_bEndRetranslation)
			{
				nSaveActiveSequNum = pSrcPhr->m_nSequNumber + 1;
				nCountBackwards = 1;
				pSrcPhr = (CSourcePhrase*)pos->GetData(); // counted this one
				pos = pos->GetPrevious();
				while (pos != NULL)
				{
					pSrcPhr = (CSourcePhrase*)pos->GetData();
					pos = pos->GetPrevious();
					nCountBackwards++;
					if (pSrcPhr->m_bBeginRetranslation)
						break;
				}
				numSrcPhrases = nCountBackwards;
			}
			else
			{
				// somewhere in the middle of the retranslation span
				nCountForwards = 1;
				pSrcPhr = (CSourcePhrase*)pos->GetData(); // counted this one
				pos = pos->GetNext();
				while (pos != NULL)
				{
					pSrcPhr = (CSourcePhrase*)pos->GetData();
					pos = pos->GetNext();
					nCountForwards++;
					if (pSrcPhr->m_bEndRetranslation)
						break;
				}
				nSaveActiveSequNum = pSrcPhr->m_nSequNumber + 1;
				pos = savePos; // restore original position
				nCountBackwards = 0;
				pSrcPhr = (CSourcePhrase*)pos->GetData(); // already counted
				pos = pos->GetPrevious();
				while (pos != NULL)
				{
					pSrcPhr = (CSourcePhrase*)pos->GetData();
					pos = pos->GetPrevious();
					nCountBackwards++;
					if (pSrcPhr->m_bBeginRetranslation)
						break;
				}
				numSrcPhrases = nCountForwards + nCountBackwards;
			}
			bool bOK;
			bOK = pView->SetActivePilePointerSafely(pApp,pSrcPhrases,nSaveActiveSequNum,
											pApp->m_nActiveSequNum,numSrcPhrases);
		}

		translation.Empty(); // don't preserve anything from a former adaptation state
		if (pApp->m_pTargetBox->GetHandle() != NULL)
			if (pApp->m_pTargetBox->IsShown())
				pApp->m_pTargetBox->SetFocus();

		// allow clicks and editing to be done in phrase box (do also in ResizeBox())
		if (pApp->m_pTargetBox->IsShown() && pApp->m_pTargetBox->GetHandle() != NULL)
			pApp->m_pTargetBox->SetEditable(TRUE);

        // get any removed adaptations in gEditRecord into the GUI list, if the restored
        // state is adapting mode; if glossing mode, get the removed glosses into the GUI
        // list
		bool bAllsWell;
		if (gbIsGlossing)
			bAllsWell = pView->PopulateRemovalsComboBox(glossesStep, &gEditRecord);
		else
			bAllsWell = pView->PopulateRemovalsComboBox(adaptationsStep, &gEditRecord);

        // BEW added 10Jun09; do a recalc of the layout, set active pile pointer, and
        // scroll into view - otherwise these are not done and box can be off-window
		pLayout->RecalcLayout(pApp->m_pSourcePhrases,keep_strips_keep_piles);
		pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
		pLayout->m_pCanvas->ScrollIntoView(pApp->m_nActiveSequNum);
		pView->Invalidate();
		pApp->GetLayout()->PlaceBox();
	}
}

void CFreeTrans::OnAdvancedRemoveFilteredFreeTranslations(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = GetApp();
	CAdapt_ItView* pView = GetView();
	CAdapt_ItDoc* pDoc = GetView()->GetDocument();

    // whm added 23Jan07 check below to determine if the doc has any free translations. If
    // not an information message is displayed saying there are no free translations; then
    // returns. Note: This check could be made in the OnIdle handler which could then
    // disable the menu item rather than issuing the info message. However, if the user
    // clicked the menu item, it may be because he/she though there might be one or more
    // free translations in the document. The message below confirms to the user the actual
    // state of affairs concerning any free translations in the current document.
	bool bFTfound = FALSE;
	if (pDoc)
	{
		SPList* pList = pApp->m_pSourcePhrases;
		if (pList->GetCount() > 0)
		{
			SPList::Node* pos = pList->GetFirst();
			while (pos != NULL)
			{
				CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
				pos = pos->GetNext();
				if (pSrcPhrase->m_bHasFreeTrans)
				{
					// set the flag on the app
					bFTfound = TRUE; 
					break; // don't need to check further
				}
			}
		}
	}
	if (!bFTfound)
	{
		// there are no free translations in the document, 
		// so tell the user and return
		wxMessageBox(_(
		"The document does not contain any free translations."),
		_T(""),wxICON_INFORMATION);
		return;
	}

	// IDS_DELETE_ALL_FT_ASK
	int nResult = wxMessageBox(_(
"You are about to delete all the free translations in the document. Is this what you want to do?"),
	_T(""), wxYES_NO);
	if (nResult == wxNO)
	{
		// user clicked the command by mistake, so exit the handler
		return;
	}

	// initialize variables needed for the scan over the document's 
	// sourcephrase instances
	SPList* pList = pApp->m_pSourcePhrases;
	SPList::Node* pos = pList->GetFirst();
	CSourcePhrase* pSrcPhrase;

#if !defined (_DOCVER5)
	wxString mkr = _T("\\free"); // enough for standard or derived 
								 // backtranslation markers
#else
	wxString emptyStr = _T("");
#endif

    // do the loop, removing the free translations, their filter marker wrappers also, and
    // clearing the document's free translation flags on the CSourcePhrase instances
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();

		// clear the flags
		pSrcPhrase->m_bHasFreeTrans = FALSE;
		pSrcPhrase->m_bStartFreeTrans = FALSE;
		pSrcPhrase->m_bEndFreeTrans = FALSE;

#if defined (_DOCVER5)
		pSrcPhrase->SetFreeTrans(emptyStr);
#else
		// handle removal from m_markers member
		if (pSrcPhrase->m_markers.IsEmpty())
		{
			continue;
		}
		else
		{
			int nFound = pSrcPhrase->m_markers.Find(mkr);
			if (nFound > 0)
			{
				// there is a filtered free translation section to be deleted
				pView->RemoveContentWrappers(pSrcPhrase,mkr,nFound + 5); // + 5 to ensure 
															// pointing past \free
			}
		} // end block for non-empty m_markers
#endif
	} // end while loop
	pView->Invalidate();
	pApp->GetLayout()->PlaceBox();

	// mark the doc as dirty, so that Save command becomes enabled
	pDoc->Modify(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the Advanced Menu 
///                        is about to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed. The "Remove Filtered Back Translations" item on the
/// Advanced menu is disabled if there are no source phrases in the App's m_pSourcePhrases
/// list, or the active KB pointer is NULL, otherwise the menu item is enabled.
/// BEW 22Feb10 no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateAdvancedRemoveFilteredBacktranslations(wxUpdateUIEvent& event)
{
	CAdapt_ItApp* pApp = GetApp();

	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_pKB != NULL && (int)pApp->m_pSourcePhrases->GetCount() > 0)
		event.Enable(TRUE);
	else
		event.Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the Advanced Menu 
///                        is about to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed. The "Remove Filtered Free Translations" item on the
/// Advanced menu is disabled if there are no source phrases in the App's m_pSourcePhrases
/// list, or the active KB pointer is NULL, otherwise the menu item is enabled.
/// BEW 22Feb10 no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateAdvancedRemoveFilteredFreeTranslations(wxUpdateUIEvent& event)
{
	CAdapt_ItApp* pApp = GetApp();
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_pKB != NULL && (int)pApp->m_pSourcePhrases->GetCount() > 0)
		event.Enable(TRUE);
	else
		event.Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return             nothing
///
///	\param  activeSequNum	->	the sequence number value at the phrase box location
/// \remarks
///	Called at the end of the RecalcLayout() function, provided the app flag
///	m_bFreeTranslationMode is TRUE. The function is responsible for determining
///	how many of the piles, starting at the active location, are to be deemed as
///	constituting the 'current' free translation section - the piles will be
///	shown with light pink background, and the user, after the function exits,
///	will be able to use buttons in the compose bar to alter the section - either
///	lengthening or shortening it, or recomposing it elsewhere, but these operations
///	are not handled by this function.
///	Note: any call made in this function which results in a RecalcLayout() call
///	being done, will recursively cause SetupCurrentFreeTransSection() to be entered
///	
///	BEW 19Feb10, no changes needed for support of _DOCVER5 (but the function
///	IsFreeTranslationEndDueToMarker() called internally has extensive changes for
///	_DOCVER5)
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::SetupCurrentFreeTransSection(int activeSequNum)
{
	CAdapt_ItApp* pApp = GetApp();
	CAdapt_ItView* pView = GetView();

	gbFreeTranslationJustRemovedInVFMdialog = FALSE; // restore to default 
           // value, in case a removed was just done in the View Filtered 
           // Material dialog

	if (activeSequNum < 0)
		// phrase box is not defined, no active location is valid, so return
		return;

#ifdef __WXDEBUG__
	wxLogDebug(_T("\nActive SN passed in: %d"),activeSequNum);
#endif
	pApp->m_pActivePile = pView->GetPile(activeSequNum); // has to be set here, because at
							// end of RecalcLayout's legacy code it is still undefined
	bool bEditBoxHasText = FALSE; // to help with initializing the ComposeBar's contents,
                            // because we may be returning from normal mode after an
                            // editing operation and want the box text to still be there
	CMainFrame* pFrame;
	pFrame = pApp->GetMainFrame();
	wxASSERT(pFrame != NULL);
	wxASSERT(pFrame->m_pComposeBar != NULL);
	wxTextCtrl* pEdit = (wxTextCtrl*)
							pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE); 
	// whm 24Aug06 removed gFreeTranslationStr global here and below
	wxString tempStr;
	tempStr = pEdit->GetValue(); // set tempStr to whatever is in the box

	m_pCurFreeTransSectionPileArray->Clear(); // start with an empty array

	bool bOwnsFreeTranslation = IsFreeTranslationSrcPhrase(pApp->m_pActivePile);
	CPile* pile;
	if (bOwnsFreeTranslation)
	{
		// it already has a free translation stored in the sourcephrase
		pile = pApp->m_pActivePile;

#ifdef _DOCVER5
		tempStr = pApp->m_pActivePile->GetSrcPhrase()->GetFreeTrans();
#else
        // before moving on from this pile, get the free translation text, and set the
        // globals for its offset and length so we can later easily update the filtered
        // free translation with any user edits done on it in the edit box of the
        // Compose Bar
		wxString theMkr = _T("\\free");
		wxString theEndMkr = _T("\\free*");
		tempStr = GetExistingMarkerContent(theMkr, theEndMkr, 
										pApp->m_pActivePile->GetSrcPhrase(),
										m_nOffsetInMarkersStr, m_nLengthInMarkersStr);
#endif
		pEdit->ChangeValue(tempStr);	// show it in the ComposeBar's edit box, but
				// don't have it selected - too easy for user to mistakenly lose it

        // now collect the array of piles in this section - since it's a predefined
        // section, we can use the bool values on each pSrcPhrase to determine the
        // section's extent
		CPile* pNextPile;
		while (pile != NULL)
		{
			// store this pile in the global array
			m_pCurFreeTransSectionPileArray->Add(pile);

#ifdef __WXDEBUG__
			wxLogDebug(_T("Storing sequ num %d in m_pCurFreeTransSectionPileArray, count = %d"),
				pile->GetSrcPhrase()->m_nSequNumber, m_pCurFreeTransSectionPileArray->GetCount());
#endif
           // there might be only one pile in the section, if so, this one would also
            // have the m_bEndFreeTrans flag set TRUE, so check for this and if so,
            // break out here
			if (pile->GetSrcPhrase()->m_bEndFreeTrans)
				break;

			// get the next pile - beware, it will be NULL if we are at doc end
			pNextPile = pView->GetNextPile(pile);
			if (pNextPile == NULL)
			{
                // we are at the doc end
				break;
			}
			else
			{
                // pNextPile is not null, so check out if this pile is the end of the
                // section
				wxASSERT(pNextPile != NULL);
				pile = pNextPile;
				wxASSERT(pile->GetSrcPhrase()->m_bHasFreeTrans); // must be TRUE 
																 // for a defined section
				if (pile->GetSrcPhrase()->m_bEndFreeTrans)
				{
					// we've found the ending pile
					m_pCurFreeTransSectionPileArray->Add(pile); // store this one too
					break; // exit the loop
				}
			}
		} // end of loop

		// attempt to set the appropriate radio button, "Punctuation" or "Verse" based
		// section choice, by analysis of the section determined by the above loop
		if (m_pCurFreeTransSectionPileArray->GetCount() > 0)
		{
			SPList* pSrcPhrases = pApp->m_pSourcePhrases;
			CSourcePhrase* pFirstSPhr = ((CPile*)
							m_pCurFreeTransSectionPileArray->Item(0))->GetSrcPhrase();
			CSourcePhrase* pLastSPhr = ((CPile*)
							m_pCurFreeTransSectionPileArray->Last())->GetSrcPhrase();
			bool bFreeTransPresent = TRUE;		
			bool bProbablyVerse = 
							pView->GetLikelyValueOfFreeTranslationSectioningFlag(pSrcPhrases,
								pFirstSPhr->m_nSequNumber, pLastSPhr->m_nSequNumber, 
								bFreeTransPresent);
			pApp->m_bDefineFreeTransByPunctuation = !bProbablyVerse;

			// now set the radio buttons
			wxRadioButton* pRadioButton = (wxRadioButton*)
				pApp->GetMainFrame()->m_pComposeBar->FindWindowById(
															IDC_RADIO_PUNCT_SECTION);
			// set the value
			if (pApp->m_bDefineFreeTransByPunctuation)
				pRadioButton->SetValue(TRUE);
			else
				pRadioButton->SetValue(FALSE);
			pRadioButton = (wxRadioButton*)
				pApp->GetMainFrame()->m_pComposeBar->FindWindowById(
															IDC_RADIO_VERSE_SECTION);
			// set the value
			if (!pApp->m_bDefineFreeTransByPunctuation)
				pRadioButton->SetValue(TRUE);
			else
				pRadioButton->SetValue(FALSE);
			}
	}
	else
	{
		// it does not yet have a free translation stored in this sourcephrase,
		// so work out the first guess for what the current section is to be
		pile = pView->GetPile(activeSequNum);
		if (pile == NULL)
			return; // something's very wrong - how can the phrase box be at 
					// a null pile?
		if (!tempStr.IsEmpty())
			bEditBoxHasText = TRUE;

        // at the current section we collect the layout information in globals, so that we
        // can delay committal to the section's extent until the user has made whatever
        // manual adjustments (with Compose Bar butons or clicking the phrase box elsewhere
        // or selecting or combinations of any of those) and the clicks Advance or Next>
        // of <Prev -- since it is at that point that the globals in the affected
        // pSrcPhrase instances will get set - (free translations not at the current
        // location will use those globals to set up for writing the free translation text
        // to the main window)
		int wordcount = 0;
		CPile* pNextPile;
		while (pile != NULL)
		{
			CSourcePhrase* pSrcPhrase;
			// store this pile in the global array
			m_pCurFreeTransSectionPileArray->Add(pile);

#ifdef __WXDEBUG__
			wxLogDebug(_T("Empty area:  Storing sequ num %d in m_pCurFreeTransSectionPileArray, count = %d"),
				pile->GetSrcPhrase()->m_nSequNumber, m_pCurFreeTransSectionPileArray->GetCount());
#endif
			// count the pile's words (BEW changed 28Apr06)
			wordcount += pile->GetSrcPhrase()->m_nSrcWords;

			// test first for a following free translation section 
			// - if there is one it must halt scanning immediately
			pNextPile = pView->GetNextPile(pile);
			if (pNextPile == NULL)
			{
				// we are at the doc end
				break;
			}
			else
			{
				if (pNextPile->GetSrcPhrase()->m_bStartFreeTrans)
					break; // halt scanning, we've bumped into a pre-existing 
						   // free trans section, else continue the battery of tests
			}
			// BEW 19Feb10, IsFreeTranslationEndDueToMarker() modified to support _DOCVER5
			if (IsFreeTranslationEndDueToMarker(pNextPile))
				break; // halt scanning, we've bumped into a SF marker which is 
                       // significant enough for us to consider that something quite
                       // different follows, or a filtered section starts at pNextPile
                       // - and that too indicates potential major change in the text
                       // at the next pile
			// determine if we can start testing for the end of the section
			// BEW 28Apr06, we are now counting words, so use to MIN_FREE_TRANS_WORDS, 
			// & still = 5 (See AdaptitConstants.h)
			if (wordcount >= MIN_FREE_TRANS_WORDS)
			{
				// test for final pile in this section
				pSrcPhrase = pile->GetSrcPhrase();
				wxASSERT(pSrcPhrase != NULL);
				if (gbIsUnstructuredData || pApp->m_bDefineFreeTransByPunctuation)
				{
					// the verse option is not available if the data has no SF markers
					if (HasWordFinalPunctuation(pSrcPhrase,pSrcPhrase->m_targetStr,
												gSpacelessTgtPunctuation))
					{
						// there is word-final punctuation, so this is a suitable place
						// to close off this section
						break;
					}
				}
				else
				{
					// we can assume the user wants the criterion to be the start of a
					// following verse (or end of document) or a text type change
					if (pNextPile == NULL)
					{
                        // we are at the end of the document
						break;
					}
					else if (pNextPile->GetSrcPhrase()->m_bVerse || 
								pNextPile->GetSrcPhrase()->m_bFirstOfType)
					{
						// this "next pile" is the start of a new verse, so we must 
						// break out here
						break;
					}
					// otherwise, continue iterating across successive piles
				}
			}

			// not enough piles to permit section to end, or end criteria not yet
			// satisfied, so keep iterating
			pile = pView->GetNextPile(pile);
		}

        // Other calculations re strip and rects and composing default ft text -- all based
        // on the array as filled out by the above loop - these calculations should be done
        // as function calls with the array as parameter, since these calcs will be needed
        // in other places too

		// compose default free translation text, if appropriate...
        // this is a new location, so use the box contents if there is already something
        // there, otherwise check the app's flags m_bTargetIsDefaultFreeTrans and
        // m_bGlossIsDefaultFreeTrans and if one is true (both can't be true at the same
        // time) then compose a default free translation string from the target text, or
        // glossing text, in the section as currently defined
		if (bEditBoxHasText)
		{
			// Compose Bar's edit box has text, so leave that as the default
			;
		}
		else
		{
			// no text there, so check the app flag
			if (pApp->m_bTargetIsDefaultFreeTrans || pApp->m_bGlossIsDefaultFreeTrans)
			{
				// do the composition from the section's target text, or glossing text
				tempStr = ComposeDefaultFreeTranslation(m_pCurFreeTransSectionPileArray);
				pEdit->ChangeValue(tempStr); // show it in the ComposeBar's edit box
			}
		}
	}
	// colour the current section
	MarkFreeTranslationPilesForColoring(m_pCurFreeTransSectionPileArray);
	pEdit->SetFocus();
	pEdit->SetSelection(-1,-1); // -1,-1 selects all
}

/////////////////////////////////////////////////////////////////////////////////
/// \return         nothing
///
///	\param     pPileArray	     ->	pointer to the array of piles which comprise the 
///                                 current section which is to have its free translation
///                                 stored as a filtered \free ... \free* marker section
///                                 within the m_markers member of the pSrcPhrase at the
///                                 anchor location (ie. at pFirstPile's sourcephrase)
///	\param     pFirstPile	     <-	pointer to the first pile in this free translation
///	                                section 
///	\param     pLastPile	     <-	pointer to the last pile in this free translation 
///	                                section
///	\param     editBoxContents   ->	enum value can be remove_editbox_contents or 
///	                                retain_editbox_contents. When remove_editbox_contents 
///                                 the source phrase's flags are adjusted and pPileArray
///                                 is emptied; otherwise (during mere editing stores) the
///                                 flags and pPileArray are not changed.
///	\param     storeStr		     ->	const ref string containing the free trans string to 
///	                                store in m_markers
/// \remarks
///    Gets the free translation text from the Compose Bar's edit box,
///    constructs the bracketed filtered string and inserts it
///    in the m_markers member of the CSourcePhrase instance at the anchor pile, and
///    returns pointers to the first and last piles in the section so that the buttons
///    <Prev, Next> or Advance can obtain the jumping off place for the movement back or
///    forwards.
///    whm clarification: In the MFC version, StoreFreeTranslation got the free translation
///    text directly from the composebar's edit box, and not via the global CString
///    gFreeTranslationStr. The MFC version originally set the value of gFreeTranslationStr
///    here from what it found in the edit box, but with my revision StoreFreeTranslation()
///    always gets the string to be stored from the input parameter storeStr.
///	whm 23Aug06 added the last two parameters
///	BEW 19Feb10, updated for support of _DOCVER5 (changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::StoreFreeTranslation(wxArrayPtrVoid* pPileArray,CPile*& pFirstPile,
	CPile*& pLastPile,enum EditBoxContents editBoxContents, const wxString& storeStr)
{
	CAdapt_ItApp* pApp = GetApp();
	CMainFrame* pMainFrm = GetFrame();
	CAdapt_ItView* pView = GetView();

	wxASSERT(pMainFrm);
	wxPanel* pBar = pMainFrm->m_pComposeBar;
	wxASSERT(pBar);
	wxASSERT(pView);

	if (pBar != NULL && pBar->IsShown())
	{
		wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
		if (pEdit != 0 && pPileArray->GetCount() > 0) // whm added second condition 
			// 1Apr09 as wxMac gets here on frame size event and pPileArray has 0 items
		{
#ifdef _DOCVER5
			pLastPile = NULL; // set null for now, it is given its value at the end
			pFirstPile = (CPile*)pPileArray->Item(0);
			CPile* pPile = pFirstPile;
			pFirstPile->GetSrcPhrase()->SetFreeTrans(storeStr);
#else
            // get the box's current contents & remove spaces at end or start (one will
            // be added by the InsertFilteredMaterial() call, at the end of the string,
            // automatically, if there is not already one at the end of the string)
			
            // whm changed 23Aug06. We now get the string to be stored as parameter to
            // StoreFreeTranslation we can probably get rid of the gFreeTranslationStr,
            // m_nOffsetInMarkersStr, and m_nLengthInMarkersStr globals, but I'll leave the
            // gFreeTranslationStr assignment here until that happens.
			wxString mkr = _T("\\free");
			wxString endMkr = _T("\\free*");
			pFirstPile = (CPile*)pPileArray->Item(0);
			// call GetExistingMarkerContent to get offset and length
			wxString tempStr = GetExistingMarkerContent(mkr,endMkr,
									pFirstPile->GetSrcPhrase(),
									m_nOffsetInMarkersStr,m_nLengthInMarkersStr);
			tempStr.Trim(FALSE); // trim left end 
			tempStr.Trim(TRUE); // trim right end

			// remove the earlier free translation text from m_markers, if there was any
			CPile* pPile;
			pPile = pFirstPile;
			wxString markers = pPile->GetSrcPhrase()->m_markers;
			if (!markers.IsEmpty())
			{
				// delete only the text, leave the markers there
				if (m_nLengthInMarkersStr > 0)
				{
					markers.Remove(m_nOffsetInMarkersStr,m_nLengthInMarkersStr); 
					m_nLengthInMarkersStr = 0;
					m_nOffsetInMarkersStr = 0;
				}
			}

            // find the insertion point (this works, regardless of what may have happened
            // above) and update with the new content for the free translation
			int nFreeTransInsertionOffset = pView->FindFilteredInsertionLocation(markers, mkr); 
			pPile->GetSrcPhrase()->m_markers = markers; // update with the prepared string
			wxASSERT(nFreeTransInsertionOffset >= 0); 
			bool bContainsFreeTrans = ContainsFreeTranslation(pPile); // TRUE if m_markers 
                // contains \free already 
                // whm revision: InsertFilteredMaterial now always uses storeStr instead of
                // gFreeTranslationStr. It also uses a local nFreeTransInsertionOffset
                // rather than the otherwise unused gnFreeTransInsertionOffset.
			pView->InsertFilteredMaterial(mkr, endMkr, storeStr, pPile->GetSrcPhrase(),
									nFreeTransInsertionOffset, bContainsFreeTrans);
#endif
            // whm added 22Aug06 the test below to remove or retain the contents of the
            // composebar's edit box and the items in pPileArray. The contents of the edit
            // box is cleared if the enum is remove_editbox_contents. The test evaluates to
            // true when StoreFreeTranslation is called from OnNextButton(), OnPrevButton()
            // or OnAdvanceButton(). It is false when called from the ComposeBarEditBox's
            // OnEditBoxChanged() handler where StoreFreeTranslation is merely storing
            // real-time edits of the string.
			if (editBoxContents == remove_editbox_contents)
			{
				// mark this sourcephrase appropriately
				// whm note: The source phrase's flags only need updating when
				// StoreFreeTranslation is called from the view's free translation
				// navigation button handlers.
				pPile->GetSrcPhrase()->m_bHasFreeTrans = TRUE;
				pPile->GetSrcPhrase()->m_bStartFreeTrans = TRUE;
				pPile->GetSrcPhrase()->m_bEndFreeTrans = FALSE;

                // clear the compose bar's edit box too, otherwise default text at the next
                // location can't be composed even if wanted
				wxString tempStr;
				tempStr.Empty(); //FreeTranslationStr.Empty();
				// whm changed 24Aug06 - update edit box with updated string
				pEdit->ChangeValue(tempStr);
				pPileArray->RemoveAt(0); // first is dealt with
#ifndef _DOCVER5
                // do a reality check on m_markers - I noticed that the test document had
                // sometimes an m_markers string ending with a SF marker, and no trailing
                // space. The space should be present - so if we detect this, we can do an
                // automatic silent correction now (I also do this when saving a note in OK
                // button's handler)
				int markersLen = pPile->GetSrcPhrase()->m_markers.Length();
				if (markersLen >= 2)
				{
					// at least a backslash and a single character should be present, 
					// otherwise don't bother
					if (pPile->GetSrcPhrase()->m_markers.GetChar(markersLen - 1) != _T(' '))
					{
						pPile->GetSrcPhrase()->m_markers += _T(' ');
					}
				}
#endif
                // if we are at the end of the document, our Adavance will not be fruitful,
                // so we'll just want to be able to automatically replace the phrase box
                // here - and beep to alert the user that the Advance failed.
				pFirstPile = pApp->m_pActivePile; // this should remain valid if we are 
												  // at doc end already
				// now handle the rest in the array
				int lastIndex = 0;
				pLastPile = pPile; // default
				int nSize = pPileArray->GetCount(); 

                // if nSize is now 0, then there was only the one pSrcPhrase in the
                // section, and so that one has to be given m_bEndFreeTrans = TRUE value
                // too
				if (nSize == 0)
				{
					pPile->GetSrcPhrase()->m_bEndFreeTrans = TRUE;
					return;
				}
                // if there is more than one pile pointer in the array, then there is at
                // least another one needing to be dealt with
				if (nSize > 0)
				{
					lastIndex = nSize - 1;
					pPile = (CPile*)(*pPileArray)[lastIndex];
					pLastPile = pPile; // we can step ahead from here, the last one, in the caller
					pPile->GetSrcPhrase()->m_bEndFreeTrans = TRUE;
					pPile->GetSrcPhrase()->m_bHasFreeTrans = TRUE;
					pPile->GetSrcPhrase()->m_bStartFreeTrans = FALSE;
					pPileArray->RemoveAt(lastIndex); // dealt with the last one
					nSize--;
				}
                // finally, any other pile pointers which are neither the first or last -
                // set the flags appropriately
				if (nSize > 0)
				{
					int index;
					CPile* ptrPile;
					for (index = 0; index < nSize; index++)
					{
						ptrPile = (CPile*)(*pPileArray)[index];
						ptrPile->GetSrcPhrase()->m_bEndFreeTrans = FALSE;
						ptrPile->GetSrcPhrase()->m_bStartFreeTrans = FALSE;
						ptrPile->GetSrcPhrase()->m_bHasFreeTrans = TRUE;
					}
					pPileArray->Clear();
				}
			} // end of if (editBoxContents == remove_editbox_contents)
		} // end of TRUE block for test: if (pEdit != 0 && pPileArray->GetCount() > 0)
	} //end of TRUE block for test: if (pBar != NULL && pBar->IsShown())
}

// the following is based on StoreFreeTranslation() and OnPrevButton() but tweaked for use
// at the point in the vertical edit process where control is about to leave the
// freeTranslationsStep and so the current free translation needs to be made to 'stick'
// BEW 22Feb10 no changes needed for support of _DOCVER5
void CFreeTrans::StoreFreeTranslationOnLeaving()
{
	CAdapt_ItApp* pApp = GetApp();
	CMainFrame* pFrame = GetFrame();
	wxASSERT(pFrame != NULL);
	if (pFrame->m_pComposeBar != NULL)
	{
		wxTextCtrl* pEdit = (wxTextCtrl*)
							pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
		if (pEdit != 0)
		{
			CPile* pOldActivePile = pApp->m_pActivePile;

            // do this store unilaterally, as we can make the free translation 'stick' by
            // calling this function also in the OnAdvancedFreeTranslationMode() hander,
            // when leaving free translation mode & not in vertical edit process, as well
            // as when we are

			// do the save & pointer calculation in the StoreFreeTranslation() call
			if (m_pCurFreeTransSectionPileArray->GetCount() > 0)
			{
				CPile* saveLastPilePtr = 
					(CPile*)m_pCurFreeTransSectionPileArray->Item(
										m_pCurFreeTransSectionPileArray->GetCount()-1);
				wxString editedText;
				editedText = pEdit->GetValue();
				StoreFreeTranslation(m_pCurFreeTransSectionPileArray,pOldActivePile,
								saveLastPilePtr,remove_editbox_contents, editedText);
			}

			// make sure the kb entry flag is set correctly
			CSourcePhrase* pSrcPhr = pOldActivePile->GetSrcPhrase();
			FixKBEntryFlag(pSrcPhr);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return             a CString which is the truncated text with an ellipsis (...) 
///                     at the end
///
///	\param pDC			->	pointer to the device context used for drawing the view
///	\param str			->	the string which is to be elided to fit the available drawing 
///	                        rectangle
///	\param ellipsis		->	the ellipsis text (three dots)
///	\param totalHExtent	->	the total horizontal extent (pixels) available in the drawing 
///                         rectangle to be used for drawing the elided text. It is the
///                         caller's responsibility to work out when this function needs
///                         to be called.
/// \remarks
///	Called in DrawFreeTranslations() when there is a need to shorten a text substring to fit
///	within the available drawing space in the layout
///	BEW 19Feb10 no change needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
wxString CFreeTrans::TruncateToFit(wxDC* pDC,wxString& str,wxString& ellipsis,
									  int totalHExtent)
{
	wxSize extent;
	wxString text = str;
	wxString textPlus;
a:	text.Remove(text.Length() - 1,1); 
	textPlus = text + ellipsis;
	pDC->GetTextExtent(textPlus,&extent.x,&extent.y); 
	if (extent.x <= totalHExtent) 
		return textPlus; // return truncated text with ellipsis 
						 // at the end, as soon as it fits
	else
	{
		if (text.Length() > 0)
			goto a;
		else
		{
			text.Empty();
			return text;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return             a CString which is the segmented input text (integral number of 
///                     whole words) that will fit within the passed in extent
///
///	\param pDC				->	pointer to the device context used for drawing the view
///	\param str				->	the string which is to be segmented to fit the available 
///	                            drawing rectangle
///	\param ellipsis		    ->	the ellipsis text (three dots)
///	\param horizRectExtent	->	the horizontal extent (pixels) available in the drawing 
///	                            rectangle to be used for drawing the segmented text
///	\param fScale			->	scaling factor to be used if the text is smaller than 
///                             the available total space (ie. all rectangles), we use
///                             fScale if bUseScale is TRUE, and with it we scale the
///                             horizontal extent (horizExtent) to be a lesser number of
///                             whole pixels when the text is comparatively short, so that
///                             we get a better distribution of words between the available
///                             drawing rectangles. (bUseScale is passed in as FALSE if we
///                             know in the caller that the total text is too long for the
///                             sum of the available drawing rectangles for it all to fit)
///	\param offset			<-	pass back to the caller the offset of the first character 
///                             in str which is not included in the returned CString - the
///                             caller will use this offset to do a .Mid(offset) call on
///                             the passed in string, to shorten it for the next
///                             iteration's call of SegmentToFit()
///	\param nIteration		->	the iteration count for this particular rectangle
///	\param nIterBound		->	the highest value that nIteration can take (equal to the 
///                             total number of drawing rectangles for this free
///                             translation section, less one)
///	\param bTryAgain		<->	passing in FALSE allows fScale to be used, passing in TRUE 
///	                            prevents it being used
///	\param bUseScale		->	whether or not to do scaling of the rectangle extents to 
///                             give a better segmentation results - ie. distributing words
///                             more evenly than would be the case if unscaled rectangle
///                             extents were used for the calculations
/// \remarks
///    Called in DrawFreeTranslations() when there is a need to work out what the suitable
///    substring should be for the drawing rectangle with the passed in horizExtent value.
///    Note: bUseScale will be ignored on the last iteration (ie.for the last drawing
///    rectangle) because the function must try to get all of the remaining string text
///    drawn within this last rectangle if possible, so for the last rectangle we try fit
///    what remains and if it won't go, then we truncate the text. The bTryAgain parameter
///    enables a TRUE value to be sent back to the caller (SegmentFreeTranslation()) so
///    that the caller can request a complete recalculation without any rectangle scaling
///    by fScale being done - we want to do this when scaling has cut a free translation
///    string too early and the last rectangle's text got truncated - so we want a second
///    run with no scaling so that we minimize the possibility of truncation being needed
///    BEW 22Feb10 no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
wxString CFreeTrans::SegmentToFit(wxDC*		pDC,
									 wxString&	str,
									 wxString&	ellipsis,
									 int		horizRectExtent,
									 float		fScale,
									 int&		offset,
									 int		nIteration,
									 int		nIterBound,
									 bool&		bTryAgain,
									 bool		bUseScale)
{
	wxString subStr;
	wxSize extent;
	pDC->GetTextExtent(str,&extent.x,&extent.y); 
	int nStrExtent = extent.x; // the passed in substring str's text extent (horiz)
	int len = str.Length();
	int nHExtent = horizRectExtent;
	int ncount;
	int nShortenBy;
	if (bUseScale && !bTryAgain)
	{
        // don't use the scaling factor if bTryAgain is TRUE, but if FALSE it can be used
        // provided bUseScale is TRUE (and the latter will be the case if the caller knows
        // the text is shorter than the total rectangle horizontal extents)
		nHExtent = (int)(horizRectExtent * fScale); // this is a lesser number of pixels 
                        // than horizRectExtent the scaling effectively gives us shorter
                        // rectangles for our segmenting calculations
	}

	// work out how much will fit - start at 5 characters, 
	// since we can be sure that much is fittable
	if (nIteration < nIterBound)
	{
		ncount = 5;
		subStr = str.Left(ncount);
		pDC->GetTextExtent(subStr,&extent.x,&extent.y); 
		while (extent.x < nHExtent && ncount < len) 
		{
			ncount++;
			subStr = str.Left(ncount);
			pDC->GetTextExtent(subStr,&extent.x,&extent.y); 
		}

		// did we get to the end of the str and it all fits?
		if (extent.x < nHExtent)
		{
			offset = len;
			return subStr;
		}

		// we didn't get to the str's end, so work backwards 
		// until we come to a space
		subStr = MakeReverse(subStr);
		int nFind = (int)subStr.Find(_T(' '));
		if (nFind == -1)
		{
            // there was no space character found, so this rectangle can't have anything
            // drawn in it - that is, we can't make a whole word fit within it
			subStr.Empty();
			offset = 0;
		}
		else
		{
			nShortenBy = nFind;
			wxASSERT( nShortenBy >= 0);
			ncount -= nShortenBy;
			subStr = str.Left(ncount); // this includes a trailing space, 
									   // even if nShortenBy was 0
			offset = ncount; // return the offset value that ensures the caller's 
                        //.Mid() call will remove the trailing space as well (beware, the
                        //resulting shortened string may still begin with a space, because
                        //the user may have typed more than one space between words, so the
                        //caller must do a Trim() anyway
			// remove the final space, so we are sure it will fit
			subStr.Trim(FALSE); // trim left end
			subStr.Trim(TRUE); // trim right end
		}
		return subStr;
	}
	else
	{
		// we are at the last rectangle, so do the best we can and ignore scaling
		offset = len;
		subStr = str;
		subStr.Trim(FALSE); // trim left end
		subStr.Trim(TRUE); // trim right end
		// recalculate, in case lopping off a trailing space 
		// has now made it able to fit
		pDC->GetTextExtent(subStr,&extent.x,&extent.y);
		nStrExtent = extent.x; 
		if (nStrExtent < horizRectExtent)
		{
			// it's all gunna fit, so just return it
			;
		}
		else
		{
			// it ain't gunna fit, so truncate
			subStr = TruncateToFit(pDC,str,ellipsis,horizRectExtent);

			// here is where we can set bTryAgain to force a recalculation 
			// without the scaling factor
			if (!bTryAgain && bUseScale)
			{
				bTryAgain = TRUE; // tell the caller to initiate a recalculation
			}
		}
		return subStr;
	}
}

// BEW 22Feb10 no changes needed for support of _DOCVER5
void CFreeTrans::ToggleFreeTranslationMode()
{
	CAdapt_ItApp* pApp = GetApp();
	CAdapt_ItView* pView = GetView();

	if (gbVerticalEditInProgress)
	{

		CMainFrame* pFrame = pApp->GetMainFrame();
		wxASSERT(pFrame != NULL);
		wxMenuBar* pMenuBar = pApp->GetMainFrame()->GetMenuBar();
		wxASSERT(pMenuBar != NULL);
		wxMenuItem * pAdvancedFreeTranslation = 
							pMenuBar->FindItem(ID_ADVANCED_FREE_TRANSLATION_MODE);
		wxASSERT(pAdvancedFreeTranslation != NULL);
		gbSuppressSetup = FALSE; // setdefault value

        // determine if the document is unstructured or not -- we'll need this set or
        // cleared as appropriate because in free translation mode the user may elect to
        // end sections at verse breaks - and we can't do that for unstructured data (in
        // the latter case, we'll just end when there is following punctuation on a word or
        // phrase)
		SPList* pSrcPhrases = pApp->m_pSourcePhrases;
		gbIsUnstructuredData = pView->IsUnstructuredData(pSrcPhrases);

		// toggle the setting
		if (pApp->m_bFreeTranslationMode)
		{
			// toggle the checkmark to OFF
			pAdvancedFreeTranslation->Check(FALSE);
			pApp->m_bFreeTranslationMode = FALSE;
		}
		else
		{
			// toggle the checkmark to ON
			pAdvancedFreeTranslation->Check(TRUE);
			pApp->m_bFreeTranslationMode = TRUE;
		}
		if (pApp->m_bFreeTranslationMode)
		{
            // put the target punctuation character set into gSpacelessTgtPunctuation to be
            // used in the HasWordFinalPunctuation() function to work out when to end a
            // span of free translation (can't put this after the ComposeBarGuts() call
            // because the latter calls SetupCurrentFreeTransSection(), and it needs
            // gSpacelessTgtPunctuation set up beforehand)
			gSpacelessTgtPunctuation = pApp->m_punctuation[1]; // target set, with 
															   // delimiting spaces
			gSpacelessTgtPunctuation.Replace(_T(" "),_T("")); // get rid of the spaces
		}	

        // restore focus to the targetBox, if free translation mode was just turned off,
        // else to the CEdit in the Compose Bar because it has just been turned on --
        // providing the box or bar is visible and its handle exists
		pFrame->ComposeBarGuts(); // open or close the Compose Bar

		if (pApp->m_bFreeTranslationMode)
		{
			// free translation mode was just turned on.

            // put the focus in the Compose Bar's edit box, and disable the phrase box for
            // clicks & editing, and make it able to right justify and render RTL if we are
            // in the Unicode app
			if (pFrame->m_pComposeBar != NULL)
				if (pFrame->m_pComposeBar->IsShown())
				{
					#ifdef _RTL_FLAGS
						// enable complex rendering
						if (pApp->m_bTgtRTL)
						{
							pFrame->m_pComposeBar->SetLayoutDirection(wxLayout_RightToLeft);
						}
						else
						{
							pFrame->m_pComposeBar->SetLayoutDirection(wxLayout_LeftToRight);
						}
					#endif
					pFrame->m_pComposeBar->SetFocus();
				}

			// prevent clicks and editing being done in phrase box (do also in CreateBox())
			if (pApp->m_pTargetBox->IsShown())
				pApp->m_pTargetBox->Enable(FALSE);
			pApp->GetMainFrame()->canvas->ScrollIntoView(
									pApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber);

			// get any removed free translations in gEditRecord into the GUI list
			bool bAllsWell;
			bAllsWell = pView->PopulateRemovalsComboBox(freeTranslationsStep, &gEditRecord);
		}
		else
		{
			// free translation mode was just turned off

			translation.Empty(); // don't preserve anything from a former adaptation state
			if (pApp->m_pTargetBox->IsShown())
			{
				pApp->m_pTargetBox->Enable(TRUE);
				pApp->m_pTargetBox->SetFocus();
			}

            // get any removed adaptations in gEditRecord into the GUI list,
            // if the restored state is adapting mode; if glossing mode, get
            // the removed glosses into the GUI list
			bool bAllsWell;
			if (gbIsGlossing)
				bAllsWell = pView->PopulateRemovalsComboBox(glossesStep, &gEditRecord);
			else
				bAllsWell = pView->PopulateRemovalsComboBox(adaptationsStep, &gEditRecord);
		}
	}
}

// handler for the IDC_APPLY_BUTTON, renamed Advance after first being called Apply
// BEW 22Feb10 no changes needed for support of _DOCVER5
void CFreeTrans::OnAdvanceButton(wxCommandEvent& event)
{
	CAdapt_ItApp* pApp = GetApp();
	CMainFrame* pMainFrm = GetFrame();
	CAdapt_ItView* pView = GetView();

    // BEW added 19Oct06; if the ENTER key is pressed when not in Free Translation mode and
    // focus is in the compose bar then it would invoke the OnAdvanceButton() handler even
    // though the button is hidden, so we prevent this by detecting when it happens and
    // exiting without doing anything.
	if (pApp->m_bComposeBarWasAskedForFromViewMenu)
	{
        // compose bar is open, but not in Free Translation mode, so we must ignore an
        // ENTER keypress, and also return the focus to the compose bar's edit box
		wxTextCtrl* pEdit;
		wxASSERT(pMainFrm != NULL); 
		wxASSERT(pMainFrm->m_pComposeBar != NULL);
		pEdit = (wxTextCtrl*)pMainFrm->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE); 
		wxASSERT(pEdit != NULL);
		wxString str;
		str = pEdit->GetValue();
		int len = str.Length();
		pEdit->SetFocus();
		pEdit->SetSelection(len,len); 
		return;
	}

    // In FT mode and if also in Review mode, the Advance button should not move the user a
    // long way ahead to an empty section, instead it should act like the phrase box does
    // in this mode, hence it instead invokes the handler for the Next> button, which makes
    // the immediate next section the current one
	if (!pApp->m_bDrafting)
	{
		OnNextButton(event);
		return;
	}

	// only do the following code when in Drafting mode
	gbSuppressSetup = FALSE; // restore default value, in case Shorten 
							 // or Lengthen buttons were used

	wxPanel* pBar = pMainFrm->m_pComposeBar; 
	if(pBar != NULL && pBar->IsShown())
	{
		wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
		if (pEdit != 0)
		{
			CPile* pOldActivePile = pApp->m_pActivePile;
			CPile* saveLastPilePtr = pApp->m_pActivePile; // a safe default

			// The current free translation was not just removed so do the 
			// StoreFreeTranslation() call
			if (m_pCurFreeTransSectionPileArray->GetCount() > 0)
			{
                // whm added 24Aug06 passing of current edits to StoreFreeTranslation()
                // via the editedText parameter along with enum remove_editbox_contents
                // to maintain legacy behavior when called from this handler
				saveLastPilePtr =
					(CPile*)m_pCurFreeTransSectionPileArray->Item(
										m_pCurFreeTransSectionPileArray->GetCount()-1);
				if (!gbFreeTranslationJustRemovedInVFMdialog)
				{
					wxString editedText;
					editedText = pEdit->GetValue();
					StoreFreeTranslation(m_pCurFreeTransSectionPileArray,
						pOldActivePile,saveLastPilePtr,remove_editbox_contents, editedText);
				}
			}

            // make sure the active location we are about to leave has the correct value
            // for m_bHasKBEntry (or m_bHasGlossingKBEntry if we are in glossing mode) set
			CSourcePhrase* pSrcPhr = pOldActivePile->GetSrcPhrase();
			FixKBEntryFlag(pSrcPhr);

			// get the next pile which does not have any free translation yet
			CPile* pPile = pView->GetNextPile(saveLastPilePtr);

			if (pPile == NULL)
			{
				// we are probably at the end document so if so then leave this section
				// current, beep to warn the user
                if (saveLastPilePtr->GetSrcPhrase()->m_nSequNumber == pApp->GetMaxIndex())
				{
					::wxBell();

					if (gbVerticalEditInProgress)
					{
						// force transition to next step
						bool bCommandPosted;
						bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(
																	-1, nextStep, TRUE);
						// TRUE is bForceTransition
					}
					// make it 'stick' before returning
						StoreFreeTranslationOnLeaving();
					return;
				}
			}
			else
			{
                // not a null pile pointer, so loop until we come to a section
                // which is not free translated
				while( pPile->GetSrcPhrase()->m_bHasFreeTrans)
				{
					CPile* pLastPile = pPile;
					pPile = pView->GetNextPile(pPile);
					if (gbVerticalEditInProgress && pPile != NULL)
					{
						int sn = pPile->GetSrcPhrase()->m_nSequNumber;
						bool bCommandPosted = 
							pView->VerticalEdit_CheckForEndRequiringTransition(sn, nextStep);
														// FALSE is bForceTransition
						if (bCommandPosted)
							return;
					}
					if (pPile == NULL)
					{
						// we are at the end of the doc
						if (pLastPile->GetSrcPhrase()->m_nSequNumber == 
							pApp->GetMaxIndex())
						{
							// at end of doc
							if (gbVerticalEditInProgress)
							{
								// force transition to next step
								bool bCommandPosted;
								bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(
																		-1, nextStep, TRUE);
																// TRUE is bForceTransition
							}
							// make it 'stick' before returning
							StoreFreeTranslationOnLeaving();
							return;
						}
					}
					else
					{
						// the pile is good, so iterate to test it
						;
					}
				} // end of while loop's block
			} // end of block for a non-NULL pPile
			if (gbVerticalEditInProgress && pPile != NULL)
			{
				int sn = pPile->GetSrcPhrase()->m_nSequNumber;
				bool bCommandPosted = 
					pView->VerticalEdit_CheckForEndRequiringTransition(sn, nextStep);
												// FALSE is bForceTransition
				if (bCommandPosted)
					return;
			}
			pApp->m_pActivePile = pPile;
			pApp->m_nActiveSequNum = pPile->GetSrcPhrase()->m_nSequNumber;
			gbSuppressSetup = FALSE; // make sure it is off

			// make m_bIsCurrentFreeTransSection FALSE on every pile
			pView->MakeAllPilesNonCurrent(pApp->GetLayout());

			// place the phrase box at the next anchor location
			CCell* pCell = pPile->GetCell(1); // whatever is the phrase box's 
											  // line in the strip
			if (gbIsGlossing)
			{
				translation = pCell->GetPile()->GetSrcPhrase()->m_gloss;
			}
			else
			{
				translation = pCell->GetPile()->GetSrcPhrase()->m_adaption;
			}
			int selector = 1; // this selector inhibits both intial and final code
							  // blocks (ie. no save to KB and no removal from KB 
							  // at the new location)
			pView->PlacePhraseBox(pCell,selector);

			// make sure we can see the phrase box
			pMainFrm->canvas->ScrollIntoView(pApp->m_nActiveSequNum);
			pView->Invalidate();
			pApp->GetLayout()->PlaceBox();
			pEdit->SetFocus(); // put focus back into compose bar's text control
		}
	}
}

// BEW 22Feb10 no changes needed for support of _DOCVER5
void CFreeTrans::OnNextButton(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = GetApp();
	CMainFrame* pMainFrm = GetFrame();
	CAdapt_ItView* pView = GetView();

	gbSuppressSetup = FALSE; // restore default value, in case Shorten or 
							 // Lengthen buttons were used
	// for debugging
	//int ftStartSN = gEditRecord.nFreeTranslationStep_StartingSequNum;
	//int ftEndSN = gEditRecord.nFreeTranslationStep_EndingSequNum;

	wxPanel* pBar = pMainFrm->m_pComposeBar;
	if(pBar != NULL && pBar->IsShown())
	{
		wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
		if (pEdit != 0)
		{
			CPile* pOldActivePile = pApp->m_pActivePile;
			CPile* saveLastPilePtr = pApp->m_pActivePile; // a safe default initialization

			// The current free translation was not just removed so do the 
			// StoreFreeTranslation() call
			if (m_pCurFreeTransSectionPileArray->GetCount() > 0)
			{
                // whm added 24Aug06 passing of current edits to StoreFreeTranslation() via
                // the editedText parameter along with enum remove_editbox_contents to
                // maintain legacy behavior when called from this handler
				saveLastPilePtr = (CPile*)m_pCurFreeTransSectionPileArray->Item(
									m_pCurFreeTransSectionPileArray->GetCount()-1);
				if (!gbFreeTranslationJustRemovedInVFMdialog)
				{
					wxString editedText;
					editedText = pEdit->GetValue();
					StoreFreeTranslation(m_pCurFreeTransSectionPileArray, pOldActivePile,
						saveLastPilePtr, remove_editbox_contents, editedText);
				}
			}
			// make sure the kb entry flag is set correctly
			CSourcePhrase* pSrcPhr = pOldActivePile->GetSrcPhrase();
			FixKBEntryFlag(pSrcPhr);

			// get the next pile
			CPile* pPile = pView->GetNextPile(saveLastPilePtr);

            // check out pPile == NULL, we would then be at the doc end - fix things
            // according; if not null, then the next pile is within the document and we can
            // set it up as the active location & new anchor point - or it will be the
            // start of a predefined free translation section in which case it is already
            // the anchor point for the next free translation section
			if (pPile == NULL)
			{
                // The scroll position is at the end of the document
				if (saveLastPilePtr->GetSrcPhrase()->m_nSequNumber == pApp->GetMaxIndex())
				{
                    // we are at the end of the doc. So leave this section current, and
                    // beep to tell the user
                    ::wxBell();

					// BEW added 11Sep08 for support of vertical editing
					if (gbVerticalEditInProgress)
					{
						// force transition to next step
						bool bCommandPosted;
						bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(
																	-1, nextStep, TRUE);
															// TRUE is bForceTransition
						return;
					}
					// make it 'stick' before returning
					StoreFreeTranslationOnLeaving();
					return;
				}
			}
			// the pPile pointer is not NULL, so continue processing
			if (gbVerticalEditInProgress)
			{
				int sn = pPile->GetSrcPhrase()->m_nSequNumber;
				bool bCommandPosted = 
					pView->VerticalEdit_CheckForEndRequiringTransition(sn, nextStep);
												// FALSE is bForceTransition
				if (bCommandPosted)
					return; // we've reached gray text, so step transition is wanted
			}

			pApp->m_pActivePile = pPile;
			pApp->m_nActiveSequNum = pPile->GetSrcPhrase()->m_nSequNumber;

			// make m_bIsCurrentFreeTransSection FALSE on every pile
			pView->MakeAllPilesNonCurrent(pApp->GetLayout());

			// place the phrase box at the next anchor location
			CCell* pCell = pPile->GetCell(1); // whatever is the phrase box's 
											  // line in the strip
			if (gbIsGlossing)
			{
				translation = pCell->GetPile()->GetSrcPhrase()->m_gloss;
			}
			else
			{
				translation = pCell->GetPile()->GetSrcPhrase()->m_adaption;
			}
			int selector = 1; // this selector inhibits both intial and final code blocks 
						// (ie. no save to KB and no removal from KB at the new location)
			pView->PlacePhraseBox(pCell,selector); // forces RecalcLayout(), which gets
											// SetupCurrentFreeTransSection() called

			// make sure we can see the phrase box
			pApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);
			pView->Invalidate(); // gets the view redrawn & phrase box shown
			pApp->GetLayout()->PlaceBox();
			pEdit->SetFocus(); // put focus back into the compose bar's edit control

			// if there is text in the pEdit box, put the cursor after it
			wxString editedText;
			editedText = pEdit->GetValue();
			int len = editedText.Length(); 
			if (len > 0)
				pEdit->SetSelection(len,len); 
			else
				pEdit->SetSelection(-1,-1); // -1,-1 selects all in wx
		}
	}
}

// whm revised 24Aug06 to allow Prev button to move back to the previous actual or
// potential free translation segment in the text
// BEW 22Feb10 no changes needed for support of _DOCVER5
void CFreeTrans::OnPrevButton(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = GetApp();
	CMainFrame* pMainFrm = GetFrame();
	CAdapt_ItView* pView = GetView();

	gbSuppressSetup = FALSE; // restore default value, in case Shorten 
							 // or Lengthen buttons were used
	wxPanel* pBar = pMainFrm->m_pComposeBar;

	if(pBar != NULL && pBar->IsShown())
	{
		wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
		if (pEdit != 0)
		{
			CPile* pOldActivePile = pApp->m_pActivePile;

            // do not do StoreFreeTranslation() call if the current free translation was
            // just deleted by operator pressing on the Delete button (either in View
            // Filtered Material dialog or using the composebar button for that purpose
			if (!gbFreeTranslationJustRemovedInVFMdialog)
			{
				// do the save & pointer calculation in the StoreFreeTranslation() call
				if (m_pCurFreeTransSectionPileArray->GetCount() > 0)
				{
                    // whm added 24Aug06 passing of current edits to StoreFreeTranslation()
                    // via the editedText parameter along with enum remove_editbox_contents
                    // to maintain legacy behavior when called from this handler
					CPile* saveLastPilePtr;
					saveLastPilePtr =
						(CPile*)m_pCurFreeTransSectionPileArray->Item(
										m_pCurFreeTransSectionPileArray->GetCount()-1);
					wxString editedText;
					editedText = pEdit->GetValue();
					StoreFreeTranslation(m_pCurFreeTransSectionPileArray,
										pOldActivePile,saveLastPilePtr,
						remove_editbox_contents, editedText);
				}
			}

			// make sure the kb entry flag is set correctly
			CSourcePhrase* pSrcPhr = pOldActivePile->GetSrcPhrase();
			FixKBEntryFlag(pSrcPhr);

			CPile* pPile = pOldActivePile;
            // The Prev button should not be activated if the active pile is already at the
            // beginning of the document. However, for safety sake, I'll check to prevent
            // this handler doing anything if we get called from the beginning of the
            // document.
			if (pPile->GetSrcPhrase()->m_nSequNumber == 0)
			{
                // we are at the beginning of the doc and cannot go to any previous free
                // trans segment, so beep and return;
				::wxBell();
				return;
			}
            // At this point there should be at least one pile before pPile to compose a
            // free translation segment. We call GetPrevPile once to start examining the
            // pile immediately before the current one (i.e., which potentially is the last
            // pile of a free translation segment before the current segment we're
            // leaving). The current pile is always assigned to pPile.
            // Then, as we scan through piles toward the beginning of the document, we
            // examine the attributes of pPile and the attributes of its previous pile
            // pPrevPile looking for a halting point for the beginning of a free
            // translation segment.
			CPile* pPrevPile = NULL;
			pPrevPile = pView->GetPrevPile(pPile);
			wxASSERT(pPrevPile != NULL);
			if (gbVerticalEditInProgress)
			{
				int sn = pPrevPile->GetSrcPhrase()->m_nSequNumber;
				if (sn < gEditRecord.nFreeTranslationStep_StartingSequNum ||
					sn > gEditRecord.nFreeTranslationStep_EndingSequNum)
				{
					// IDS_CLICK_IN_GRAY_ILLEGAL
					wxMessageBox(_(
"Attempting to put the active location within the gray text area while updating information in Vertical Edit mode is illegal. The attempt has been ignored."),
					_T(""), wxICON_WARNING);
					return;
				}
			}


            // If the last pile before the current free trans segment (i.e., now pPrevPile)
            // is the last pile of a previously adjoining free translation segment, we want
            // to scan back to the first pile of that existing segment (regardless of any
            // potential intervening halting points)
			if (pPrevPile->GetSrcPhrase()->m_bEndFreeTrans)
			{
                // the previous pile is already within an existing free translation
                // segment. In this situation we need to scan back to find the beginning
                // pile of that existing segment.
				while (pPrevPile != NULL)
				{
					pPile = pView->GetPrevPile(pPrevPile);
                    // Check out if this pPile == NULL, we could be at the bundle start or
                    // the doc start. Handle things according to whichever is the case.
					if (pPile == NULL)
					{
                        // we are at the start of the doc, leave this section current
                        // (with phrasebox at pPrevPile) and return. The Prev button
                        // gets disabled once we get here, so this block will only get
                        // entered when the Prev button first gets us back to the
                        // beginning of the document. No bell sound is needed here.
						wxASSERT(pPrevPile->GetSrcPhrase()->m_nSequNumber == 0);

						if (gbVerticalEditInProgress)
						{
							// force transition to next step
							bool bCommandPosted;
							bCommandPosted = 
								pView->VerticalEdit_CheckForEndRequiringTransition(
															-1, nextStep, TRUE);
													// TRUE is bForceTransition
							if (bCommandPosted)
							{
								// make it 'stick' before returning
								StoreFreeTranslationOnLeaving();
								return;
							}
						}

						CCell* pCell = pPrevPile->GetCell(1);
						int selector = 1;
						pView->PlacePhraseBox(pCell,selector);
						// make sure we can see the phrasebox at the beginning of the doc
						pApp->GetMainFrame()->canvas->ScrollIntoView(
											pPrevPile->GetSrcPhrase()->m_nSequNumber);
						pView->Invalidate();
						pApp->GetLayout()->PlaceBox();
						pEdit->SetFocus(); // put focus back into 
										   // compose bar's edit control
						return;
					}
                    // Criteria for halting scanning and establishing the anchor for a free
                    // translation segment: If the source pharase at pPile is already the
                    // start of a free translation (m_bStartFreeTrans). We can ignore
                    // checking for other halting conditions here.
					if (pPrevPile->GetSrcPhrase()->m_bStartFreeTrans)
					{
						break;
					}
					pPrevPile = pPile;
				} // end of loop for test: while (pPrevPile not NULL)
			}
			else
			{
                // the previous pile is not already within an existing free translation
                // segment (i.e., it is part of a hole). This is a situation in which we
                // need to examine halting criteria to determine the halting point.
				while (pPrevPile != NULL)
				{
					pPile = pView->GetPrevPile(pPrevPile);
                    // if this pPile is NULL, we are at the doc's start.
					if (pPile == NULL)
					{
                        // we are at the start of the doc, leave this section current
                        // (with phrasebox at pPrevPile) and return. The Prev button
                        // gets disabled once we get here, so this block will only get
                        // entered when the Prev button first gets us back to the
                        // beginning of the document. No bell sound is needed here.
						wxASSERT(pPrevPile->GetSrcPhrase()->m_nSequNumber == 0);

						if (gbVerticalEditInProgress)
						{
							// force transition to next step
							bool bCommandPosted;
							bCommandPosted = 
								pView->VerticalEdit_CheckForEndRequiringTransition(
															-1, nextStep, TRUE);
													// TRUE is bForceTransition
							if (bCommandPosted)
							{
								// make it 'stick' before returning
								StoreFreeTranslationOnLeaving();
								return;
							}
						}

						CCell* pCell = pPrevPile->GetCell(1);
						int selector = 1;
						pView->PlacePhraseBox(pCell,selector);
						// make sure we can see the phrasebox at the beginning of the doc
						pMainFrm->canvas->ScrollIntoView(
											pPrevPile->GetSrcPhrase()->m_nSequNumber);
						pView->Invalidate();
						pApp->GetLayout()->PlaceBox();
						pEdit->SetFocus(); // put focus back into the 
										   // compose bar's edit control
						return;
					}
					// Criteria for halting scanning and establishing the anchor for a 
					// free translation segment:
                    // (Note: These are the same criteria used by
                    // SetupCurrentFreeTransSection()) 
                    // Unconditionally halt scanning, if we
                    // encounter:
					//   1. An SF marker significant enough for us to consider that a 
					//     logical break in content starts at pPrevPile 
					//     (IsFreeTranslationEndDueToMarker also returns TRUE if a 
					//     filtered section starts there);
					//   2. If the source phrase at pPile is already the start of a 
					//     free translation (m_bStartFreeTrans).
                    // The additional conditions for halting scanning depend if we
                    // encounter the following halting criteria within a pile already
                    // marked for free translation or not already marked for free
                    // translation.
					// If we have unstructured data or if m_bDefineFreeTransByPunctuation, 
					//      halt scanning back if HasWordFinalPunctuation() returns TRUE, 
					//      unless the pile is within an existing free translation 
					//      segment, and that pile is not the first pile of the existing
					//      free translation (in which case we want to continue scanning 
					//      back until we reach the first pile of the existing segment).
					// If m_bDefineFreeTransByPunctuation is FALSE, we halt scanning if 
                    //     the source phrase at pPrevPile marks the beginning of a new
                    //     verse (m_bVerse), unless the pile is within an existing free
                    //     translation segment, and that pile is not the first pile of the
                    //     existing free translation (here also we want to continue
                    //     scanning back until we reach the first pile of the existing
                    //     segment - this might not happen often but some verses begin in
                    //     strange places!).
					// or, if the source phrase at pPrevPile marks a change of text type 
                    //     (m_bFirstOfType), unless, like the other criteria above, an
                    //     existing free translation somehow managed to span a text type
                    //     boundary, in which case we would continue scanning back until we
                    //     found the first pile of that existing free translation.
					
					if (IsFreeTranslationEndDueToMarker(pPrevPile))
					{
						break;
					}
					if (pPrevPile->GetSrcPhrase()->m_bStartFreeTrans)
					{
						break;
					}
					// Check if pPile is the (potential) last pile of a previous free 
					// trans section according to user's choice of verse or punctuation 
					// criteria.
					CSourcePhrase* pPrevSrcPhrase = pPile->GetSrcPhrase();
					wxASSERT(pPrevSrcPhrase != NULL);
					if (gbIsUnstructuredData || pApp->m_bDefineFreeTransByPunctuation)
					{
						// the verse option is not available if the data has no SF markers
						if (HasWordFinalPunctuation(pPrevSrcPhrase,
									pPrevSrcPhrase->m_targetStr,gSpacelessTgtPunctuation))
						{
                            // there is word-final punctuation on the previous pile's
                            // source phrase, so the current pile is a suitable place to
                            // begin this section
							break;
						}
					}
					else if (pPrevPile->GetSrcPhrase()->m_bVerse || 
												pPrevPile->GetSrcPhrase()->m_bFirstOfType)
					{
						break;
					}
                    // If we get here, we've not found an actual or potential anchor point
                    // based on inspecting the current pile nor the last pile
                    // (pOldActivePile), so save the current pile in pPrevPile, and get
                    // another preceding pile and examine it to see if it has criteria for
                    // establishing the beginning of a free translation segment.
					pPrevPile = pPile;
				}
			}

			wxASSERT(pPrevPile != NULL);
			if (gbVerticalEditInProgress)
			{
				// possibly force transition to next step
				bool bCommandPosted;
				bCommandPosted = 
					pView->VerticalEdit_CheckForEndRequiringTransition(
												-1, nextStep, TRUE);
				// TRUE is bForceTransition
				if (bCommandPosted)
				{
					// make it 'stick' before returning
					StoreFreeTranslationOnLeaving();
					return;
				}
			}

			pApp->m_pActivePile = pPrevPile;
			pApp->m_nActiveSequNum = pPrevPile->GetSrcPhrase()->m_nSequNumber;

			// make m_bIsCurrentFreeTransSection FALSE on every pile
			pView->MakeAllPilesNonCurrent(pApp->GetLayout());

			// place the phrase box at the next anchor location
			CCell* pCell = pPrevPile->GetCell(1);
			if (gbIsGlossing)
			{
				translation = pCell->GetPile()->GetSrcPhrase()->m_gloss;
			}
			else 
			{
				translation = pCell->GetPile()->GetSrcPhrase()->m_adaption;
			}
			int selector = 1; // this selector inhibits both intial and final code blocks
                              // (ie. no save to KB and no removal from KB at the new
                              // location)
			pView->PlacePhraseBox(pCell,selector); // forces a a RecalcLayout(), which gets 
											// SetupCurrentFreeTransSection() called
			// make sure we can see the phrase box
			pApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);

			pView->Invalidate(); // gets the view redrawn
			pApp->GetLayout()->PlaceBox();
			pEdit->SetFocus(); // put focus back into compose bar's edit control

			// if there is text in the pEdit box, put the cursor after it
			wxString editStr;
			editStr = pEdit->GetValue();
			int len = editStr.Length();
			if (len > 0)
				pEdit->SetSelection(len,len);
			else
				pEdit->SetSelection(-1,-1); // -1,-1 selection entire text;
		}
	}
}

// BEW 22Feb10 no changes needed for support of _DOCVER5
void CFreeTrans::OnRemoveFreeTranslationButton(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = GetApp();
	CMainFrame* pMainFrm = GetFrame();
	CAdapt_ItView* pView = GetView();

	wxPanel* pBar = pMainFrm->m_pComposeBar;

	if (pBar != NULL && pBar->IsShown())
	{
		wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
		if (pEdit != 0)
		{
            // BEW added 29Apr06, to inform a subsequent <Prev, Next> or Advance button
            // click that the free translation at the current section has been removed, so
            // that those buttons will not insert a filtered pair of \free \free* markers
            // with no content at the current location when the one of those three buttons'
            // handler is invoked
			gbFreeTranslationJustRemovedInVFMdialog = TRUE;

			// get the anchor pSrcPhrase
			CSourcePhrase* pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();

			// make sure the kb entry flag is set correctly
			FixKBEntryFlag(pSrcPhrase);

#ifdef _DOCVER5
			wxString emptyStr = _T("");
			pSrcPhrase->SetFreeTrans(emptyStr);
			wxString tempStr;
#else
			// the next part of the code aims to remove the
			// "\~FILTER \free <free trans text> \free* \~FILTER* " string from m_markers
			wxString theMkr = _T("\\free");
			wxString theEndMkr = _T("\\free*");
			// whm 24Aug06 modified below
			wxString tempStr = GetExistingMarkerContent(theMkr, theEndMkr, pSrcPhrase,
											m_nOffsetInMarkersStr, m_nLengthInMarkersStr);
            // BEW added 1Oct08; for supporting the use of this function to clear the
            // current free translation section when the user changes the section extent by
            // clicking Punctuation or Verse radio button (typically there may not yet by
            // any filtered free translation in pSrcPhrase yet, so we check for that and
            // skip the stuff below which assumes a free translation is present
			if (!(m_nOffsetInMarkersStr == 0 && m_nLengthInMarkersStr == 0 && 
				tempStr.IsEmpty()))
			{
                // the above call gives us m_nOffsetInMarkersStr (start of the free trans
                // text) and m_nLengthInMarkersStr (its length, including the trailing
                // space), so we have to get pointers, starting from these locations, to
                // the preceding and following filter bracket markers
				wxString markersStr = pSrcPhrase->m_markers;
				int totalLen = markersStr.Length();
                // start by looking for the preceding \~FILTER marker, we'll iterate a
                // pointer backwards until we find \~FILTER
				wxString fltrMkr = filterMkr;
				int fmkrLen = fltrMkr.Length();
                // wx version note: Since we require a read-only buffer we use GetData
                // which just returns a const wxChar* to the data in the string.
				const wxChar* pBuff = markersStr.GetData();
				wxChar* pBufStart = (wxChar*)pBuff;
				wxChar* pEnd;
				pEnd = pBufStart + totalLen;
				wxASSERT(*pEnd == _T('\0'));
				wxChar* ptr = pBufStart + m_nOffsetInMarkersStr; // point to start of the 
														// free translation text itself
				--ptr;
				while ((wxStrncmp(ptr,filterMkr,fmkrLen) != 0) && ptr > pBufStart)
				{
					--ptr;
				}
				int nStartingOffset = (int)(ptr - pBufStart);
				wxASSERT(nStartingOffset >= 0);
				wxASSERT( m_nOffsetInMarkersStr + m_nLengthInMarkersStr < totalLen);
				int nFound = FindFromPos(markersStr,filterMkrEnd,
										m_nOffsetInMarkersStr + m_nLengthInMarkersStr);
				// it must be present further along, after the \free* endmarker of 
				// length 6 & trailing space
				wxASSERT(nFound > m_nOffsetInMarkersStr + m_nLengthInMarkersStr + 6);
				// the final offset is nFound plus the length of \~FILTER* plus 1 for 
				// its trailing space
				nFound += fmkrLen + 2; // 2 because we are counting the * and then 
									   // the following space

				// delete this text material from the m_markers string
				markersStr.Remove(nStartingOffset,nFound - nStartingOffset); 
				pSrcPhrase->m_markers = markersStr;
			}
#endif
			// update the navigation text
			pSrcPhrase->m_inform = pApp->GetDocument()->RedoNavigationText(pSrcPhrase);

			// clear the Compose Bar's edit box
			// whm 24Aug06 modified below
			tempStr.Empty();
			pEdit->ChangeValue(tempStr);

            // clear the bool members on the source phrases in the array, but leave the
            // array elements themselves since they correctly define this section's extent
            // at the time the button was pressed
			int nSize = (int)m_pCurFreeTransSectionPileArray->GetCount();
			CPile* pPile;
			int index;
			for (index = 0; index < nSize; index++)
			{
				pPile = (CPile*)m_pCurFreeTransSectionPileArray->Item(index);
				wxASSERT(pPile);
				pPile->GetSrcPhrase()->m_bStartFreeTrans = FALSE;
				pPile->GetSrcPhrase()->m_bHasFreeTrans = FALSE;
				pPile->GetSrcPhrase()->m_bEndFreeTrans = FALSE;
			}
			pView->Invalidate(); // cause redraw, and so a call to SetupCurrentFreeTransSection()
			pApp->GetLayout()->PlaceBox();
			pEdit->SetFocus(); // put focus in compose bar's edit control
			pEdit->SetSelection(-1,-1); // -1,-1 selects all in wx
		}
	}
}

// BEW 22Feb10 no changes needed for support of _DOCVER5
void CFreeTrans::OnLengthenButton(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = GetApp();
	CMainFrame* pMainFrm= GetFrame();
	CAdapt_ItView* pView = GetView();

	gbSuppressSetup = TRUE; // prevent SetupCurrentFreeTransSection() from wiping
            // out the action done below at the time that the view is updated (which
            // otherwise would call that function)
	bool bEditBoxHasText = FALSE; // default
	// whm 24Aug06 reordered and modified below
	wxPanel* pBar = pMainFrm->m_pComposeBar; 
	wxASSERT(pBar != NULL);
	wxASSERT(pView != NULL);

	wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
	wxASSERT(pEdit != NULL);
	wxString tempStr;
	tempStr = pEdit->GetValue();

	if (!tempStr.IsEmpty()) 
		bEditBoxHasText = TRUE;
	// & we can rely on m_nActiveSequNum having being set correctly, 
	// and also pApp->m_pActivePile;

	if(pBar != NULL && pBar->IsShown())
	{
		if (pEdit != 0)
		{
			CPile* pPile;
			int end = (int)m_pCurFreeTransSectionPileArray->GetCount() - 1;
			pPile = (CPile*)m_pCurFreeTransSectionPileArray->Item(end); // pile at end 
                // of current section the OnUpdateLengthenButton() handler will have
                // already disabled the button if there is no next pile in the bundle, so
                // we can procede with confidence
			pPile = pView->GetNextPile(pPile);
			wxASSERT(pPile != NULL);
			pPile->SetIsCurrentFreeTransSection(TRUE); // this will make it's background 
													   // go light pink
			m_pCurFreeTransSectionPileArray->Add(pPile); // add it to the array

            // if there is text in the Compose Bar's edit box (ie. gFreeTranslationStr is
            // not empty) then we'll lengthen without making any change to it; but if there
            // is no text, then will either leave the box empty or put in default text
            // contructed from the new current (shorter) section, according to whatever the
            // relevant flag setting currently is
			if (!bEditBoxHasText)
			{
				if (pApp->m_bTargetIsDefaultFreeTrans || pApp->m_bGlossIsDefaultFreeTrans)
				{
					// do the composition from the section's target text
					tempStr = ComposeDefaultFreeTranslation(m_pCurFreeTransSectionPileArray);
					pEdit->ChangeValue(tempStr); // show it in the ComposeBar's edit box
				}
			}

			// colour the current section & select the text
			MarkFreeTranslationPilesForColoring(m_pCurFreeTransSectionPileArray);
			pEdit->SetFocus();
			pEdit->SetSelection(-1,-1); //-1,-1 selects all in wx

			// get the window updated
			pView->Invalidate();
			pApp->GetLayout()->PlaceBox();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the Update Idle
///                        mechanism 
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the free translation navigation buttons
/// are visible. The "Shorten" button used in free translation mode is disabled if the
/// application is not in Free Translation mode, or if the active pile pointer is NULL, or
/// if the active sequence number is negative (-1). But the button is enabled as long as
/// there is at least one pile left.
/// BEW 22Feb10 no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateShortenButton(wxUpdateUIEvent& event)
{
	CAdapt_ItApp* pApp = GetApp();
	if (!pApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_nActiveSequNum < 0 || pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	// BEW changed 06Mar06, so user can shorten to a single pile
	int nSize = (int)m_pCurFreeTransSectionPileArray->GetCount();
    // BEW changed 15Oct05, because we want to allow the user to shorten to less than 5
    // piles when he really wants too - such as when there are 4 piles, each a merger, and
    // so the automatic sectioning gets lots of extra piles too - in such a circumstance
    // the user may want to shorten to just get the first four piles.
	if (nSize <= 1)
		event.Enable(FALSE);
	else
		event.Enable(TRUE);
}

// BEW 22Feb10 no changes needed for support of _DOCVER5
void CFreeTrans::OnShortenButton(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = GetApp();
	CMainFrame* pMainFrm = GetFrame();
	CAdapt_ItView* pView = GetView();

	gbSuppressSetup = TRUE; // prevent SetupCurrentFreeTransSection() from 
            // wiping out the action done below at the time that the view is
            // updated (which otherwise would call that function)
	bool bEditBoxHasText = FALSE; // default

	wxPanel* pBar = pMainFrm->m_pComposeBar;
	wxASSERT(pBar != NULL);
	wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
	wxASSERT(pEdit != NULL);
	wxString tempStr;
	tempStr = pEdit->GetValue();

	if (!tempStr.IsEmpty())
		bEditBoxHasText = TRUE;
    // & we can rely on m_nActiveSequNum having being set correctly, and also
    // pApp->m_pActivePile; and the button will only be enabled if this is a section not
    // previously defined ( we want to make it hard for the user to open up an un-free
    // tranlated section gap in the sequence of free translations)

	if (pBar != NULL && pBar->IsShown())
	{
		if (pEdit != 0)
		{
			int end = (int)m_pCurFreeTransSectionPileArray->GetCount() - 1;
			if (end >= 1) // BEW changed 06Mar06 to allow user 
						  // to shorten to one pile only
			{
				// remove the last pile from the array after making sure
				// it is no longer regarded as within the current section
				CPile* pPile = (CPile*)m_pCurFreeTransSectionPileArray->Item(end);
				pPile->SetIsCurrentFreeTransSection(FALSE); // this will mean 
										// the cell background will go white
                // whm corrected by addition 20Sep06: When shortening an existing free
                // trans segment, the cell did not go white, but stayed green, because the
                // source phrase associated with the end pile had not yet had its
                // m_bHasFreeTrans reset to FALSE; so, the following lines update the flags
                // to correct the situation.
				pPile->GetSrcPhrase()->m_bHasFreeTrans = FALSE;
				pPile->GetSrcPhrase()->m_bEndFreeTrans = FALSE;
				// the pile previous to pPile becomes the new 
				// end pile of the active segment
				CPile* pPrevPile = pView->GetPrevPile(pPile);
				if (pPrevPile != NULL)
					pPrevPile->GetSrcPhrase()->m_bEndFreeTrans = TRUE;

				m_pCurFreeTransSectionPileArray->RemoveAt(end);
			}
			else
			{
				// can't remove index 0
				return;
			}

            // if there is text in the Compose Bar's edit box (ie. gFreeTranslationStr is
            // not empty) then we'll shorten without making any change; but if there is no
            // text, then will either leave the box empty or put in default text contructed
            // from the new current (shorter) section, according to whatever the relevant
            // flag setting currently is
			if (!bEditBoxHasText)
			{
				if (pApp->m_bTargetIsDefaultFreeTrans || pApp->m_bGlossIsDefaultFreeTrans)
				{
					// do the composition from the section's target text or glossing text
					tempStr = ComposeDefaultFreeTranslation(m_pCurFreeTransSectionPileArray);
					pEdit->ChangeValue(tempStr); // show it in the ComposeBar's edit box
				}
			}

			// colour the current section & select the text
			MarkFreeTranslationPilesForColoring(m_pCurFreeTransSectionPileArray);
			pEdit->SetFocus();
			pEdit->SetSelection(-1,-1); // -1,-1 selects all in wx

			// get the window updated
			pView->Invalidate();
			pApp->GetLayout()->PlaceBox();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the Update Idle 
///                        mechanism
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the free translation navigation buttons
/// are visible. The "Lengthen" button used in free translation mode is disabled if the
/// application is not in Free Translation mode, or if the active pile pointer is NULL, or
/// if the active sequence number is negative (-1). But the button is enabled if it won't
/// extend the next free translation segment past the end of a bundle or the doc, and if it
/// won't extend beyond some significant marker, or encroach on an already defined free
/// translation.
/// BEW 22Feb10 no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateLengthenButton(wxUpdateUIEvent& event)
{
	CAdapt_ItApp* pApp = GetApp();
	CAdapt_ItView* pView = GetView();

	//bool bOwnsFreeTranslation;
	if (!pApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_nActiveSequNum < 0 || pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
    // BEW addition 11Sep08; in vertical editing mode, this is called when
    // freeTranslationStep is initialized at a former free translation section which
    // has been cleared, and so the pile array is empty; GetAt() calls then fail and
    // crash the app, so we won't allow lengthening if there is no array defined yet
	if (m_pCurFreeTransSectionPileArray->IsEmpty()) // && !IsFreeTranslationSrcPhrase(m_pActivePile))
	{
		//wxLogDebug(_T("OnUpdateLengthenButton: exit at test for empty pile array"));
		event.Enable(FALSE);
		return;
	}
	int end = (int)m_pCurFreeTransSectionPileArray->GetCount() - 1;
	CPile* pPile = (CPile*)m_pCurFreeTransSectionPileArray->Item(end);
	wxASSERT(pPile);
	pPile = pView->GetNextPile(pPile); // get the pile immediately after the current end
	if (pPile == NULL)
	{
		//wxLogDebug(_T("OnUpdateLengthenButton: exit at test for next pile empty"));
		// if at the end of bundle or doc, disable the button
		event.Enable(FALSE);
		return;
	}
	else
	{
        // whm observation: Here we only restrict the lengthening of the free trans
        // segment if the next pile contains a significant sfm; but, if it contains
        // punctuation that initially established the length of the segment, we allow
        // the user to lengthen beyond that punctuation, but we never allow lengthening
        // past the start of an existing free translation.
		if (IsFreeTranslationEndDueToMarker(pPile))
		{
            // markers or filtered stuff must end the section (for example, we can't
            // allow the possibility of unfiltering producing new content within a free
            // translation section)
			//wxLogDebug(_T("OnUpdateLengthenButton: exit at test for marker following"));
			event.Enable(FALSE);
			return;
		}
		// also, we can't lengthen if there is a defined section following
		if (pPile->GetSrcPhrase()->m_bStartFreeTrans)
		{
			//wxLogDebug(_T("OnUpdateLengthenButton: exit at test for ft section starting at next pile"));
			event.Enable(FALSE);
		}
		else
		{
			event.Enable(TRUE); // but we can lengthen provided it is extending the 
                            // section into an undefined free translation area and none
                            // of the above end-conditions applies
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the Advanced Menu 
///                        is about to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed. The "Free Translation Mode" item on the Advanced menu
/// is disabled if the active pile pointer is NULL, or the application is only showing the
/// target text, or there are no source phrases in the App's m_pSourcePhrases list. But, if
/// m_curIndex is within a valid range and the composeBar was not already opened for
/// another purpose (called from the View), the menu item is enabled.
/// BEW 22Feb10 no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateAdvancedFreeTranslationMode(wxUpdateUIEvent& event)
{
	CAdapt_ItApp* pApp = GetApp();
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_pSourcePhrases->GetCount() == 0)
	{
		event.Enable(FALSE);
		return;
	}
    // the !pApp->m_bComposeBarWasAskedForFromViewMenu test makes sure we don't try to
    // invoke free translation mode while the user already has the Compose Bar open for
    // another purpose
    if (pApp->m_nActiveSequNum <= (int)pApp->GetMaxIndex() && pApp->m_nActiveSequNum >= 0
		&& !pApp->m_bComposeBarWasAskedForFromViewMenu)
	{
		event.Enable(TRUE);
	}
	else
	{
		event.Enable(FALSE);
	}
}

// BEW 22Feb10 no changes needed for support of _DOCVER5
void CFreeTrans::OnAdvancedTargetTextIsDefault(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = GetApp();
	CMainFrame* pMainFrm = GetFrame();

	wxASSERT(pMainFrm != NULL);
	wxMenuBar* pMenuBar = pMainFrm->GetMenuBar();
	wxASSERT(pMenuBar != NULL);
	wxMenuItem* pAdvancedMenuTTextDft = 
							pMenuBar->FindItem(ID_ADVANCED_TARGET_TEXT_IS_DEFAULT);
	wxMenuItem* pAdvancedMenuGTextDft = 
							pMenuBar->FindItem(ID_ADVANCED_GLOSS_TEXT_IS_DEFAULT);
	wxASSERT(pAdvancedMenuTTextDft != NULL);
	wxASSERT(pAdvancedMenuGTextDft != NULL);

	// toggle the setting
	if (pApp->m_bTargetIsDefaultFreeTrans)
	{
		// toggle the checkmark to OFF
		pAdvancedMenuTTextDft->Check(FALSE);
		pApp->m_bTargetIsDefaultFreeTrans = FALSE;
	}
	else
	{
		// toggle the checkmark to ON
		pAdvancedMenuTTextDft->Check(TRUE);
		pApp->m_bTargetIsDefaultFreeTrans = TRUE;

		// and ensure the glossing text command is off, and its flag cleared
		pAdvancedMenuGTextDft->Check(FALSE);
		pApp->m_bGlossIsDefaultFreeTrans = FALSE;
	}

	// restore focus to the Compose Bar
	if (pMainFrm->m_pComposeBar->GetHandle() != NULL)
		if (pMainFrm->m_pComposeBar->IsShown())
			pMainFrm->m_pComposeBarEditBox->SetFocus();
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the Advanced Menu 
///                        is about to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed. The "Use Target Text As Default Free Translation"
/// item on the Advanced menu is disabled if the application is not in Free Translation
/// mode, or if the active pile pointer is NULL, or if there are no source phrases in the
/// App's m_pSourcePhrases list. But, if m_curIndex is within a valid range and the
/// composeBar was not already opened for another purpose (called from the View), the menu
/// item is enabled.
/// BEW 22Feb10 no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateAdvancedTargetTextIsDefault(wxUpdateUIEvent& event)
{
	CAdapt_ItApp* pApp = GetApp();
	if (pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_pSourcePhrases->GetCount() == 0)
	{
		event.Enable(FALSE);
		return;
	}
	if (!pApp->m_bFreeTranslationMode) // whm added 23Jan07 to wx version
	{
		// The Advanced menu item "Use Target Text As Default Free Translation"
		// should be disabled when the app is not in Free Translation Mode.
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_nActiveSequNum <= (int)pApp->GetMaxIndex() && 
			pApp->m_nActiveSequNum >= 0 &&
			!pApp->m_bComposeBarWasAskedForFromViewMenu)
		event.Enable(TRUE);
	else
		event.Enable(FALSE);
}

// BEW 22Feb10 no changes needed for support of _DOCVER5
void CFreeTrans::OnAdvancedGlossTextIsDefault(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = GetApp();
	CMainFrame* pMainFrm = GetFrame();

	wxASSERT(pMainFrm != NULL);
	wxMenuBar* pMenuBar = pMainFrm->GetMenuBar();
	wxASSERT(pMenuBar != NULL);
	wxMenuItem* pAdvancedMenuTTextDft = 
							pMenuBar->FindItem(ID_ADVANCED_TARGET_TEXT_IS_DEFAULT);
	wxMenuItem* pAdvancedMenuGTextDft = 
							pMenuBar->FindItem(ID_ADVANCED_GLOSS_TEXT_IS_DEFAULT);
	wxASSERT(pAdvancedMenuTTextDft != NULL);
	wxASSERT(pAdvancedMenuGTextDft != NULL);

	// toggle the setting
	if (pApp->m_bGlossIsDefaultFreeTrans)
	{
		// toggle the checkmark to OFF
		pAdvancedMenuGTextDft->Check(FALSE);
		pApp->m_bGlossIsDefaultFreeTrans = FALSE;
	}
	else
	{
		// toggle the checkmark to ON
		pAdvancedMenuGTextDft->Check(TRUE);
		pApp->m_bGlossIsDefaultFreeTrans = TRUE;

		// ensure the target text command is toggled off (if it was on), and its flag
		pAdvancedMenuTTextDft->Check(FALSE);
		pApp->m_bTargetIsDefaultFreeTrans = FALSE;

	}

	// restore focus to the Compose Bar
	if (pMainFrm->m_pComposeBar->GetHandle() != NULL)
		if (pMainFrm->m_pComposeBar->IsShown())
			pMainFrm->m_pComposeBarEditBox->SetFocus();
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the Advanced Menu
///                        is about to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed. The "Use Gloss Text As Default Free Translation" item
/// on the Advanced menu is disabled if the application is not in Free Translation mode, or
/// if the active pile pointer is NULL, or if there are no source phrases in the App's
/// m_pSourcePhrases list. But, if m_curIndex is within a valid range and the composeBar
/// was not already opened for another purpose (called from the View), the menu item is
/// enabled.
/// BEW 22Feb10 no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateAdvancedGlossTextIsDefault(wxUpdateUIEvent& event)
{
	CAdapt_ItApp* pApp = GetApp();
	if (pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_pSourcePhrases->GetCount() == 0)
	{
		event.Enable(FALSE);
		return;
	}
	if (!pApp->m_bFreeTranslationMode) // whm added 23Jan07 to wx version
	{
		// The Advanced menu item "Use Gloss Text As Default Free Translation"
		// should be disabled when the app is not in Free Translation Mode.
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_nActiveSequNum <= (int)pApp->GetMaxIndex() && 
			pApp->m_nActiveSequNum >= 0 &&
			!pApp->m_bComposeBarWasAskedForFromViewMenu)
		event.Enable(TRUE);
	else
		event.Enable(FALSE);
}

// BEW 22Feb10 no changes needed for support of _DOCVER5
void CFreeTrans::OnRadioDefineByPunctuation(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = GetApp();
	CMainFrame* pMainFrm = GetFrame();
	CAdapt_ItView* pView = GetView();

	wxASSERT(pMainFrm != NULL);
	wxPanel* pBar = pMainFrm->m_pComposeBar;
	wxASSERT(pBar != NULL);
	if(pBar != NULL && pBar->IsShown())
	{
		// FindWindow() finds a child of the current window
		wxRadioButton* pRPSButton = (wxRadioButton*)
								pBar->FindWindow(IDC_RADIO_PUNCT_SECTION);
		wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
		if (pRPSButton != 0)
		{
			// set the radio button's BOOL to be TRUE
			pApp->m_bDefineFreeTransByPunctuation = TRUE;
						
			// BEW added 1Oct08: to have the butten click remove the
			// current section and reconstitute it as a Verse-based section
			gbSuppressSetup = FALSE;
			wxCommandEvent evt;
			pApp->GetFreeTrans()->OnRemoveFreeTranslationButton(evt); // remove current section and 
                // any Compose bar edit box test;
                // the OnRemoveFreeTranslationButton() call calls Invalidate()

            // To get SetupCurrenetFreeTranslationSection() called, we must call
            // RecalcLayout() with gbSuppressSetup == FALSE, then the section will be
            // resized smaller
#ifdef _NEW_LAYOUT
			pApp->GetLayout()->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
			GetLayout()->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
			pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);

			// restore focus to the edit box
			pEdit->SetFocus();
		}
	}
}

// BEW 22Feb10 no changes needed for support of _DOCVER5
void CFreeTrans::OnRadioDefineByVerse(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = GetApp();
	CMainFrame* pMainFrm = GetFrame();
	CAdapt_ItView* pView = GetView();

	wxASSERT(pMainFrm != NULL);
	wxPanel* pBar = pMainFrm->m_pComposeBar;
	wxASSERT(pBar != NULL);
	if(pBar != NULL && pBar->IsShown())
	{
		wxRadioButton* pRVSButton = (wxRadioButton*)
								pBar->FindWindow(IDC_RADIO_VERSE_SECTION);
		wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
		if (pRVSButton != 0)
		{
			// set the radio button's BOOL to be TRUE
			pApp->m_bDefineFreeTransByPunctuation = FALSE;
			
			// BEW added 1Oct08: to have the butten click remove the
			// current section and reconstitute it as a Verse-based section
			gbSuppressSetup = FALSE;
			wxCommandEvent evt;
#ifdef	_FREETR
			OnRemoveFreeTranslationButton(evt); // remove current section and 
                // any Compose bar edit box test;
                // the OnRemoveFreeTranslationButton() call calls Invalidate()
#else	// _FREETR
			OnRemoveFreeTranslationButton(evt); // remove current section and 
				// any Compose bar edit box test
				// the OnRemoveFreeTranslationButton() call calls Invalidate()
#endif	// _FREETR

            // To get SetupCurrentFreeTranslationSection() called, we must call
            // RecalcLayout() with gbSuppressSetup == FALSE, then the section will be
            // resized larger
#ifdef _NEW_LAYOUT
			pApp->GetLayout()->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
			GetLayout()->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
			pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);

			// restore focus to the edit box
			pEdit->SetFocus();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the Update Idle
///                        mechanism 
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the Free Translation navigation
/// buttons are visible. The "Next >" button used for navigation in free translation mode
/// is disabled if the application is not in Free Translation mode, or if the active pile
/// pointer is NULL, or if the active sequence number is negative (-1), otherwise the
/// button is enabled.
/// BEW 22Feb10 no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateNextButton(wxUpdateUIEvent& event)
{
	CAdapt_ItApp* pApp = GetApp();
	if (!pApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_nActiveSequNum < 0 || pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	event.Enable(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the Update Idle 
///                        mechanism
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the Free Translation navigation buttons
/// are visible. The "< Prev" button used for navigation in free translation mode is
/// disabled if the application is not in Free Translation mode, or if the active pile
/// pointer is NULL, or if the active sequence number is negative (-1), or if the pile
/// previous to the active pile is NULL, otherwise the button is enabled.
/// BEW 22Feb10 no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdatePrevButton(wxUpdateUIEvent& event)
{
	CAdapt_ItApp* pApp = GetApp();
	CAdapt_ItView* pView = GetView();

	if (!pApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_nActiveSequNum < 0 || pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	CPile* pPile = pView->GetPrevPile(pApp->m_pActivePile);
	if (pPile == NULL)
	{
		// probably we are at the start of the document
		event.Enable(FALSE);
	}
	else
		event.Enable(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return	nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the Update 
///                        Idle mechanism
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the Free Translation navigation buttons
/// are visible. The "Remove" button used in free translation mode is disabled if the
/// application is not in Free Translation mode, or if the active pile pointer is NULL, or
/// if the active sequence number is negative (-1), or if the active pile does not own the
/// free translation, otherwise the button is enabled.
/// BEW 22Feb10 no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateRemoveFreeTranslationButton(wxUpdateUIEvent& event)
{
	CAdapt_ItApp* pApp = GetApp();
	bool bOwnsFreeTranslation;
	if (!pApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_nActiveSequNum < 0 || pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	bOwnsFreeTranslation = IsFreeTranslationSrcPhrase(pApp->m_pActivePile);
	if (!bOwnsFreeTranslation)
	{
		event.Enable(FALSE);
	}
	else
	{
		event.Enable(TRUE); // it's a defined section, so we can remove it
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return         nothing
///
///	\param pileArray	->	pointer to the array of piles which are to have their
///	                        m_bIsCurrentFreeTransSection BOOL member set to TRUE
/// \remarks
///	This will turn on light pastel pink colouring of the phrase box line's
///	rectangles which lie within the current free translation section, when
///	Draw() is called on the CCell instances) -- use after making a call to
///	MakeAllPilesNonCurrent() when the current section moves to a new location
///	or is changed in size
/// BEW 22Feb10 no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::MarkFreeTranslationPilesForColoring(wxArrayPtrVoid* pileArray)
{
	int nCount = pileArray->GetCount(); 
	int index;
	CPile* pile;
	for (index = 0; index < nCount; index++)
	{
		pile = (CPile*)pileArray->Item(index); 
		wxASSERT(pile); 
		pile->SetIsCurrentFreeTransSection(TRUE);
	}
    // we now have to bother with clearing this bool member because not every
    // RecalcLayout() call builds CPile instances from scratch, and when that is
    // the case the default value is not reset FALSE for each unless we explicitly
    // do so (but we don't need to do it here)
}

/////////////////////////////////////////////////////////////////////////////////
/// \return             nothing
///
///	\param pArr	   ->  pointer to the m_pFreeTransArray which contains FreeTrElement 
///                    structs - each of which contains the information relevant to
///                    writing a subpart of the free translation in a single rectangle
///                    under a single strip
/// Remarks:
///    The structures and variables are used over and over while writing out the
///    free translation text in the client area, and so we need this function to clear out
///    the array each time we come to the next section of the free translation. 
///    Used by DrawFreeTranslations().
///    BEW 22Feb10 no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::DestroyElements(wxArrayPtrVoid* pArr)
{
	int size = pArr->GetCount();
	if (size == 0)
		return;
	FreeTrElement* pElem;
	int i;
	for (i = 0; i < size; i++)
	{
		pElem = (FreeTrElement*)pArr->Item(i);
		delete pElem;
	}
	pArr->Clear();
}

/////////////////////////////////////////////////////////////////////////////////
/// \return             the CPile instance, safely earlier than where we are interested in
///                     and where the caller will start scanning ahead from to get the
///                     'real' starting location we want
///	\param activeSequNum	->	index of the current active location where the free 
///	                            translation's current section commences (ie. the 
///	                            anchor CPile instance)
/// \remarks
/// Called early in the view's DrawFreeTranslations() function, to return an arbitrary but
/// off-screen CPile instance guaranteed to lie somewhere within the document and preceding
/// the start of the current free translation section being drawn. This pile is used as the
/// kick off point for scanning forward to determine which CPile instance is actually to be
/// the start (ie. anchor) for the current free translation section. In the legacy
/// application where we segmented the document into "bundles" and only laid out a bundle
/// at a time, it was easy to start the forward scan from the start of the current bundle.
/// But in the refactored application, it would be a waste of time to start the scan from
/// the beginning of the document. So we work out a suitable location given the current
/// active location (& its anchor pile) - that works right even if the user has scrolled
/// the active location off screen. Since we need to dynamically work this out for each
/// call of DrawFreeTranslations(), there is no need to store this starting pile's pointer
/// in a global for use at a later time
/// Note: in July 09 (about 12th?) BEW changed the forward scanning in
/// DrawFreeTranslations() to not scan forward more than as many strips as fit in the
/// visible window, otherwise we were getting whole-document scans over thousands of strips
/// which tied up the app for a minute or more. Therefore, the kick off point for scanning
/// forward has to be able to find its target location within a window height's amount of
/// strips from the kick off location, so care must be exercised in coding the free
/// translation functionality to ensure this constraint is never violated. (see change of
/// 14July below, for example)
/// BEW 22Feb10 no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
CPile* CFreeTrans::GetStartingPileForScan(int activeSequNum)
{
	CAdapt_ItView* pView = GetView();
	CLayout* pLayout = GetLayout();

	CPile* pStartPile = NULL;
	if (activeSequNum < 0)
	{
		pStartPile = pView->GetPile(0);
		return pStartPile;
	}
	pStartPile = pView->GetPile(activeSequNum);
	wxASSERT(pStartPile);
	int numVisibleStrips = pLayout->GetNumVisibleStrips();
	if (numVisibleStrips < 1)
		numVisibleStrips = 2; // we don't want to use 0 or 1, not a big enough jump
	int nCurStripIndex = pStartPile->GetStripIndex();
    // BEW changed 14Jul09, we want to start the off-window scan no more than a strip or
    // two from the start of the visible area, otherwise our caller, DrawFreeTranslations()
    // may exit early without drawing anything - so from the active strip we go back a
    // half-window and then two more strips for good measure
	nCurStripIndex = nCurStripIndex - (numVisibleStrips / 2 + 2);
	if (nCurStripIndex < 0)
		nCurStripIndex = 0;
	// protect also, at doc end - ensure we start drawing before whatever is visible in
	// the client area
	int stripCount = pLayout->GetStripArray()->GetCount();
	if (nCurStripIndex > stripCount - (numVisibleStrips + 1))
		nCurStripIndex = stripCount - (numVisibleStrips + 1);
	// now get the strip pointer and find it's first pile to return to the caller
	CStrip* pStrip = (CStrip*)pLayout->GetStripArray()->Item(nCurStripIndex);
	pStartPile = (CPile*)pStrip->GetPilesArray()->Item(0); // ptr of 1st pile in strip
	wxASSERT(pStartPile);
	return pStartPile;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return             nothing
///
///	\param pDC				->	pointer to the device context used for drawing the view
///	\param str				->	the string which is to be segmented to fit the available 
///	                            drawing rectangles
///	\param ellipsis		    ->	the ellipsis text (three dots)
///	\param textHExtent		->	horizontal extent of the section's free translation text 
///	                            (unsegmented)
///	\param totalHExtent	    ->  the total horizontal extent (pixels) available - calculated 
///                             by summing the horizontal extents of all the drawing
///                             rectangles to be used for drawing the subtext strings.
///	\param pElementsArray	->	array of FreeTrElement structs, one per drawing rectangle 
///	                            for this section
///	\param pSubstrings		<-	array of substrings formed by segmenting str into substrings 
///                             which will fit, one per rectangle, in the rectangles stored
///                             in pElementsArray (the caller will do the drawing of these
///                             substrings in the appropriate rectangles)
///	\param totalRects		->	the total number of drawing rectangles available for this 
///                             section (equals pElementsArray->GetSize() - which is how
///                             it was calculated in the caller)
/// \remarks
/// Called in DrawFreeTranslations() when there is a need distribute the typed free
/// translation string passed in via the str parameter over a number of drawing rectangles
/// in two or more consecutive strips - hence the size of the pSubstrings array must be the
/// same as or less than totalRects.
/// BEW 19Feb10, no changes needed for support of _DOCVER5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::SegmentFreeTranslation(	wxDC*			pDC,
											wxString&		str, 
											wxString&		ellipsis, 
											int				textHExtent, 
											int				totalHExtent, 
											wxArrayPtrVoid*	pElementsArray, 
											wxArrayString*	pSubstrings, 
											int				totalRects)
{
	float fScale = (float)(textHExtent / totalHExtent); // calculate the scale factor

    // adjustments are needed -- if the text is much shorter than the allowed space
    // (considering the two or more rectangles it has to be distributed over) then it isn't
    // any good to split short text across wide rectangles - instead it looks better to
    // bunch it to the left, if necessary drawing it all in the one rectangle. So for
    // smaller and smaller values of fScale, we need to bump up fScale by bigger and bigger
    // increments; the particular values below have been determined by experimentation and
    // appear to give optimal results in terms of appearance and synchonizing meaning
    // chunks with the layout parts to which they pertain
	if (fScale > (float)0.95)
		; // make no change
	else if (fScale > (float)0.9)
		fScale = (float)0.95;
	else if (fScale > (float)0.8)
		fScale = (float)0.9;
	else if (fScale > (float)0.7)
		fScale = (float)0.87;
	else if (fScale > (float)0.6)
		fScale = (float)0.83;
	else
		fScale = (float)0.8;

	wxString remainderStr = str; // we shorten this for each iteration
	wxString subStr; // what we work out as the first part of remainderStr 
					 // which will fit the current rect
	wxASSERT(pSubstrings->GetCount() == 0);
	FreeTrElement* pElement;
	int offset;
	int nIteration;
	int nIterBound = totalRects - 1;
	bool bTryAgain = FALSE; // if we use the scaling factor and we get truncation in 
            // the last rectangle then we'll use a TRUE value for this flag to force a
            // second segmentation which does not use the scaling factor

a:	if (bTryAgain || textHExtent > totalHExtent)
	{
        // the text is longer than the available space for drawing it, so there is
        // no point to doing any scaling -- instead, get as much as will fit into
        // each each rectange, and the last rectangle will have to have its text
        // elided using TruncateToFit()
		for (nIteration = 0; nIteration <= nIterBound; nIteration++)
		{
			pElement = (FreeTrElement*)pElementsArray->Item(nIteration);

			// do the calculation, ignoring fScale (hence, last parameter is FALSE)
			subStr = SegmentToFit(pDC,remainderStr,ellipsis,pElement->horizExtent,
							fScale,offset,nIteration,nIterBound,bTryAgain,FALSE);
			pSubstrings->Add(subStr);
			remainderStr = remainderStr.Mid(offset); // shorten, 
													 // for next segmentation
		}
	}
	else
	{
        // we should be able to make the text fit (though this can't be guaranteed because
        // some space is wasted in each rectangle if we print whole words (which we do)) -
        // and we'll need to do scaling to ensure the best segmentation results
		for (nIteration = 0; nIteration <= nIterBound; nIteration++)
		{
			pElement = (FreeTrElement*)pElementsArray->Item(nIteration);

			// do the calculation, using fScale (hence, last parameter is TRUE)
			subStr = SegmentToFit(pDC,remainderStr,ellipsis,pElement->horizExtent,
								fScale,offset,nIteration,nIterBound,bTryAgain,TRUE);
			pSubstrings->Add(subStr);
			remainderStr = remainderStr.Mid(offset); // shorten, for next segmentation
		}

		if (bTryAgain)
		{
			pSubstrings->Clear();
			remainderStr = str;
			goto a;
		}
	}
}

void CFreeTrans::OnAdvancedRemoveFilteredBacktranslations(wxCommandEvent& WXUNUSED(event))
{
    // whm added 23Jan07 check below to determine if the doc has any back translations. If
    // not an information message is displayed saying there are no back translations; then
    // returns. Note: This check could be made in the OnIdle handler which could then
    // disable the menu item rather than issuing the info message. However, if the user
    // clicked the menu item, it may be because he/she though there might be one or more
    // back translations in the document. The message below confirms to the user the actual
    // state of affairs concerning any back translations in the current document.
	CAdapt_ItApp* pApp = GetApp();
	CAdapt_ItDoc* pDoc = GetApp()->GetDocument();
	bool bBTfound = FALSE;
	if (pDoc)
	{
		SPList* pList = pApp->m_pSourcePhrases;
		if (pList->GetCount() > 0)
		{
			SPList::Node* pos = pList->GetFirst();
			while (pos != NULL)
			{
				CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
				pos = pos->GetNext();
				if (!pSrcPhrase->m_markers.IsEmpty())
				{
					if (pSrcPhrase->m_markers.Find(_T("\\bt")) != -1)
					bBTfound = TRUE; 
					break; // don't need to check further
				}
			}
		}
	}
	if (!bBTfound)
	{
		// there are no free translations in the document, so tell the user and return
		wxMessageBox(_(
		"The document does not contain any back translations."),
		_T(""),wxICON_INFORMATION);
		return;
	}

	// IDS_DELETE_ALL_BT_ASK
	if( wxMessageBox(_(
"You are about to delete all the back translations in the document. Is this what you want to do?"),
	_T(""), wxYES_NO|wxICON_INFORMATION) == wxNO)
	{
		// user clicked the command by mistake, so exit the handler
		return;
	}

	// initialize variables needed for the scan over the document's 
	// sourcephrase instances
	SPList* pList = pApp->m_pSourcePhrases;
	SPList::Node* pos = pList->GetFirst(); 
	CSourcePhrase* pSrcPhrase;
	wxString mkr = _T("\\bt"); // enough for standard or derived 
							   // backtranslation markers
	// do the loop, halting to store each collection at appropriate (unfiltered) 
	// SF markers
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		if (pSrcPhrase->m_markers.IsEmpty())
		{
			continue;
		}
		else
		{
			int nFound = pSrcPhrase->m_markers.Find(mkr);
			if (nFound > 0)
			{
				// there is a filtered backtranslation section to be deleted
				GetView()->RemoveContentWrappers(pSrcPhrase,mkr,nFound + 3); // + 3 to 
													// ensure pointing past \bt
			}
		} // end block for non-empty m_markers
	} // end while loop
	GetView()->Invalidate();
	GetLayout()->PlaceBox();

	// mark the doc as dirty, so that Save command becomes enabled
	pDoc->Modify(TRUE);
}


#endif	// _FREETR