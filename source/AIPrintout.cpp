/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AIPrintout.cpp
/// \author			Bill Martin
/// \date_created	28 February 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the AIPrintout class.
/// The AIPrintout class manages the functions for printing and print previewing from
/// within Adapt It using the File | Print and File | Print Preview menu selections.
/// \derivation		The AIPrintout class is derived from wxPrintout.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in AIPrintout.cpp (in order of importance): (search for "TODO")
// 1.
//
// Unanswered questions: (search for "???")
// 1.
//
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "AIPrintout.h"
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

//#define Print_failure

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/print.h>

#include "Adapt_It.h"
#include "Adapt_ItView.h"
#include "Strip.h"
#include "Pile.h"
#include "Cell.h"
#include "MainFrm.h"
#include "Adapt_ItCanvas.h"
#include "Layout.h"
#include "FreeTrans.h"
#include "AIPrintout.h"


/// This global is defined in Adapt_It.cpp.
//extern CAdapt_ItApp* gpApp; // for rapid access to the app class

/// This global is defined in Adapt_ItView.cpp.
extern bool gbPrintFooter;

extern int gnFromChapter;
extern int gnFromVerse;
extern int gnToChapter;
extern int gnToVerse;
extern bool gbIsBeingPreviewed;
extern bool gbSuppressPrecedingHeadingInRange;
extern bool gbIncludeFollowingHeadingInRange;
extern int gnPrintingLength;
extern int gnTopGap;
extern int gnBottomGap;
extern int gnFooterTextHeight;

extern bool gbCheckInclFreeTransText;
extern bool gbCheckInclGlossesText;
extern bool gbIsGlossing;

// This global is defined in Adapt_It.cpp.
//extern bool	gbRTL_Layout;	// ANSI version is always left to right reading; this flag can only
							// be changed in the Unicode version, using the extra Layout menu

// The AIPrPreviewFrame class below /////////////////////////////////////////////////////////////

// These class methods can be uncommented if it is necessary to implement the AIPreviewFrame class in
// order to access the preview frame's methods such as OnCloseWindow().
//
//BEGIN_EVENT_TABLE(AIPreviewFrame, wxPreviewFrame)
//	EVT_CLOSE(AIPreviewFrame::OnCloseWindow)
//END_EVENT_TABLE()
//
//AIPreviewFrame::AIPreviewFrame(wxPrintPreview* preview, wxWindow* parent, const wxString& title,
//		const wxPoint& pos, const wxSize& size)
//	:wxPreviewFrame(preview,parent,title,pos,size)
//{
//}
//
//void AIPreviewFrame::OnCloseWindow(wxCloseEvent& event)
//{
//	wxPreviewFrame::OnCloseWindow(event); // must call the base class method for the window to be destroyed
//}

// End of AIPrPreviewFrame class  /////////////////////////////////////////////////////////////

// The AIPrintout class below /////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC_CLASS(AIPrintout, wxPrintout)

// WX Documentation Note: The following comments utilize a nifty Visual Studio Addin called
// HyperAddin which is freely available from http://www.codeplex.com/hyperAddin/. HyperAddin also has
// built-in automatic comment wrapping support and other features that enhance the commenting process
// within Visual Studio 2005 or 2008 (it does not work with Visual Studio 2003). To jump to the
// underlined hyperlink reference just use CTRL + left mouse click on the link. The functions with these
// links also have hyperlinks back to the print_flow reference below.

//////////////////// Order of called methods in the wxWidgets printing framework ///////////////////
// whm Note: The order of function calls when printing or print-previewing in the wxWidgets printing
// framework is:
// #print_flow
// 0. code:CAdapt_ItView::OnPrint()
//    or code:CAdapt_ItView::OnPrintPreview() event handler [invoked by File | Print or File | Print Preview command]
// 1. code:AIPrintout::AIPrintout() constructor [executed once for File | Print; twice for File | Print Preview]
// 2. The wxPrinter printer object calls printer.Print() see code:#printer_Print() [puts up platform specific print dialog]
// 3. code:AIPrintout::OnPreparePrinting()
// 4. code:AIPrintout::GetPageInfo()
// 5. code:AIPrintout::OnBeginPrinting()
//    6. code:AIPrintout::OnBeginDocument()
//       7. code:AIPrintout::HasPage()
//       8. code:AIPrintout::OnPrintPage()
//    9. AIPrintout::OnEndDocument() [not currently overridden in either MFC or WX]
// 10. code:AIPrintout::OnEndPrinting()
// In Step 2 if printer.Print() returns FALSE, printing aborts and no other steps are completed.
// Steps 6-9 are done for each copy of a document when multiple copies are requested
// Steps 7-8 are done for each page to be printed in a given document
//////////////////// Order of called methods in the wxWidgets printing framework ///////////////////

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// Called to construct an AIPrintout object. This constructor freezes the canvas which is kept frozen
/// until the corresponding destructor is called.
////////////////////////////////////////////////////////////////////////////////////////////
AIPrintout::AIPrintout(const wxChar *title):wxPrintout(title)
{
	// refactored 6Apr09
	m_pApp = &wxGetApp();

	// See code:#print_flow for the order of calling of this AIPrintout constructor.

    // whm: to avoid problems with calls to the View's Draw() method we should freeze the
    // canvas here at the beginning of the print preview routine, and unfreeze it in the
    // AIPrintout destructor after printing ends. For non-preview printing it is not
    // necessary to freeze the canvas.
	m_pApp->GetMainFrame()->canvas->Freeze();

	// in the refactored design, not all strips may be fully filled, so since printing is
	// likely to print one or more incomplete strips, the best way to prevent that is to
	// do a fill recalc of the layout (but leave piles untouched) before printing, so that
	// all strips are properly filled
	m_pApp->m_nSaveActiveSequNum = m_pApp->m_nActiveSequNum;
	m_pApp->m_docSize = m_pApp->m_pLayout->GetLogicalDocSize(); // copy m_logicalDocSize value
															// back to app's member
	m_pApp->m_saveDocSize = m_pApp->m_docSize; // store original size (can dispense with this
			// here if we wish, because OnPreparePrinting() will make same call)
	// the following, defined in the app class, is a kluge to prevent problems which would
	// happen due to the ~AIPrintout() destructor being called twice after a Print
	// Preview, so I'm counting times reentered, and only letting the function do any work
	// on the first time entered
	m_pApp->m_nAIPrintout_Destructor_ReentrancyCount = 1;
	m_pApp->pAIPrintout = this;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// Called when the AIPrintout object is about to be destroyed. The main clean up code for
/// restoring the document to its previous state it had before printing is executed here in
/// the destructor. The cleanup involves: Calling RestoreOriginalList() after printing a
/// selection or range, and RemoveSelection() when printing a selection or range, but not
/// previewing; resetting some global printing flags (including gbIsPrinting to FALSE);
/// calling the View's RestoreIndices(), ClearPagesList(), and RecalcLayout(); the canvas'
/// ScrollIntoView(), etc., and thaws out the canvas which was frozen in the corresponding
/// constructor.
/// BEW 25May09: For some reason known only to wxWidgets, the function is called twice
/// when closing a Print Preview; and setting the active sequence number to the saved
/// value on the app, and then setting the save app value to -1 is bad news unless we do
/// something about it, because the RecalcLayout() the second time round will muck up the
/// active location, since the active sequence number gets wrongly reset to -1 on the
/// second call! So, I'll try a global to count the number of times reentered, and for the
/// second or later reenters, just exist immediately without doing anything. Crude, but it
/// will have to do. 18Jul09 BEW moved the code, including the reentracy protection
/// global, into a function called DoPrintCleanup() and put the latter in the app class.
////////////////////////////////////////////////////////////////////////////////////////////
AIPrintout::~AIPrintout()
{
	CAdapt_ItApp* pApp = &wxGetApp();

#ifdef Print_failure
#if defined(_DEBUG) && defined(__WXGTK__)
    wxLogDebug(_T("AIPrintout ~AIPrintout() line 199 before DoPrintCleanup(): gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d , m_bFreeTranslationMode = %d"),
               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)pApp->m_bFreeTranslationMode);
#endif
#endif

	pApp->DoPrintCleanup();
	pApp->pAIPrintout = NULL;
	pApp->m_pLayout->m_pOffsets = NULL; // BEW 1Occt11 added, to protection insurance for
        // DrawFreeTranslationsForPrinting() not to fail if a RecalcLayout() is called and
        // free translations are to be printed and printing is turned on -
        // PrintOptionsDlg::InitDialog() will also set it to NULL for the same reason; it's
        // to be non-null only during actual printing or print previewing of a defined page
        // as determined by a currently active PageOffsets struct being pointed at by
        // m_pOffsets, having been set by OnPrintPage() beforehand. If DrawFreeTranslations()
        // is called when m_pOffsets is NULL, it will immediately return without doing
        // anything
#ifdef Print_failure
#if defined(_DEBUG) && defined(__WXGTK__)
    wxLogDebug(_T("AIPrintout ~AIPrintout() line 217 after DoPrintCleanup(): gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d , m_bFreeTranslationMode = %d"),
               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)pApp->m_bFreeTranslationMode);
#endif
#endif

	// BEW 18Nov13, Next two lines needed if a range was printed, DoPrintCleanup() in that
	// circumstance left the doc width at A4 page print-area width, because m_docSize and
	// m_saveDocSize have same (narrow) width value for all of the printing, and 0 for the
	// height value.
	//pApp->GetLayout()->RecalcLayout(pApp->m_pSourcePhrases,create_strips_keep_piles);
	//pApp->GetLayout()->Redraw();
    // BEW 18Nov13, Removed the need for these by use of new boolean
    // m_bSuppressFreeTransRestoreAfterPrint in the DoPrintCleanup() function
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if the wxDC associated with printing is valid, otherwise FALSE
/// \param      page      -> the current page being rendered by this OnPrintPage()
/// \remarks
/// Called from: Called by the framework to draw the current page while printing - it is called
/// once for each page that needs to be printed. Returning false cancels the print job.
/// OnPrintPage() is called after OnBeginDocument() and once for each time HasPage()
/// returns true for a given document. The application can use wxPrintout::GetDC to obtain
/// a device context to validate and draw on.
/// BEW 28Oct11, added turning on and off the app member variable, m_bPagePrintInProgress, which
/// when TRUE, diverts a CStrip::Draw() to draw a page of the list of printed pages, rather than
/// to print strips of the view's canvas window.
////////////////////////////////////////////////////////////////////////////////////////////
bool AIPrintout::OnPrintPage(int page)
{
	CAdapt_ItApp* pApp = &wxGetApp();
#ifdef Print_failure
#if defined(_DEBUG) && defined(__WXGTK__)
//    wxLogDebug(_T("\n\n************************\nAIPrintout OnPrintPage() line 240 at start: gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d , m_bFreeTranslationMode = %d"),
//               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)pApp->m_bFreeTranslationMode);
#endif
#endif

	// refactored 6Apr09, and the special code for the __WXGTK__ build added in Oct-Nov 2011
	// See code:#print_flow for the order of calling of OnPrintPage().
	CAdapt_ItView* pView = pApp->GetView();
	CLayout* pLayout = pApp->m_pLayout;
    wxDC *pDC = GetDC();

#if defined(__WXGTK__)
    // Required, because the wxPostScriptDC is ill-behaved, or more likely, wxGnomeprinter
    // framework is broken - the device context won't handle drawing which is not top-down
    // and leftwards , or top-down and rightwards; violation of those constraints wipes
    // out the data on the page. And wx functions from the framework return bogus values,
    // in particular, the y coord of the origin offset - I need to do my own calculation.
    // The code below for the __WXGTK__ build is a workaround for the DC problems so that
    // the free translation strips are printed by interleaving one line's worth between the
    // printing of each pair of strips.
    if (pDC)
    {
		pApp->m_bPagePrintInProgress = TRUE; // BEW 28Oct11 added

		pApp->m_nCurPage = page; // set the app member for use by CStrip's Draw() function

        wxArrayPtrVoid* pStripArray = pLayout->GetStripArray();
        int i;
        wxString ftStr;

		bool bRTLLayout;
		bRTLLayout = FALSE; // default, for ANSI build
#ifdef _RTL_FLAGS
		if (m_pApp->m_bTgtRTL)
		{
			// free translation has to be RTL & aligned RIGHT
			bRTLLayout = TRUE;
		}
		else
		{
			// free translation has to be LTR & aligned LEFT
			bRTLLayout = FALSE;
		}
#endif

        // pass these in to the aggregation function - they are used early in the composition of
        // the free translation data for printing
        wxArrayPtrVoid arrPileSet;
        wxArrayPtrVoid arrRectsForOneFreeTrans;

        // the next two arrays store, respectively, arrays of FreeTrElement structs created on the
        // heap, and arrays of associated free translation strings (or substrings if a free translation
        // extends over more than one strip). The intent is to compose all the free translations prior
        // to drawing anything on the page, and aggregate all the draw rectanges, and associated free
        // translation substrings, together if they belong to a given strip - so that drawing of each
        // such aggregate can be done after the piles of the strip are drawn, and we can be sure that
        // nothing in the aggregate being drawn belongs with data in a following strip. This protocol
        // enables a strict top-down drawing regime as follows:
        // draw strip 0
        // draw all free translations which have their display rects immediately below strip 0
        // draw strip 1
        // draw all free translations which have their display rects immediately below strip 1
        // draw strip 2
        // draw all free translations which have their display rects immediately below strip 2
        // and so forth until all the data (except for the footer) has been drawn for the page,
        // and the footer is then drawn last

        // the data in their arrays is associated in parallel by index; the index is the index for the
        // strip on this page, starting from zero
        wxArrayPtrVoid arrFTElementsArrays; // stores arrays of FreeTRElement structs, each array and
            // its structs are created on the heap, and so must be deleted before OnPrintPage() returns
        wxArrayPtrVoid arrFTSubstringsArrays; // stores arrays of wxString, a free translation or a part
            // of same, each array is created on the heap, & must be deleted before OnPrintPage() returns

		// The code block below properly scales the text to appear the correct size within both
		// the print preview and on the printer.

		// Scale the display context to the correct size for either the preview or for printer.
		// OnPrintPage() is called for both print preview and for actual printing to printer.

		// Get the logical pixels per inch of screen and printer
		int ppiScreenX, ppiScreenY;
		GetPPIScreen(&ppiScreenX, &ppiScreenY);
		int ppiPrinterX, ppiPrinterY;
		GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
        // This scales the DC so that the printout roughly represents the the screen scaling. The
        // wxPrintout docs say, "The text point size _should_ be the right size but in fact is too small
        // for some reason. This is a detail that will need to be addressed at some point but can be
        // fudged for the moment."
		float scale = (float)((float)ppiPrinterX/(float)ppiScreenX);

		// Now we have to check in case our real page size is reduced (e.g. because
		// we're drawing to a print preview memory DC)
		int pageWidthInPixels, pageHeightInPixels;
		int w, h;
		pDC->GetSize(&w, &h);
		this->GetPageSizePixels(&pageWidthInPixels, &pageHeightInPixels);
        // If printer pageWidth == current DC width, then this doesn't change. But w might be the
        // preview bitmap width, so scale down.
		float overallScaleX = scale * (float)(w/(float)pageWidthInPixels);
		float overallScaleY = scale * (float)(h/(float)pageHeightInPixels);
		pDC->SetUserScale(overallScaleX, overallScaleY);

		// Calculate the scaling factor for passing to PrintFooter. This factor must be multiplied by
		// any absolute displacement distance desired which is expressed in mm. For example, we want
		// the footer to be placed 12.7mm (half inch) below the bottom printing margin. In logical
		// units a 12.7mm displacement below the bottom printing margin is 12.7*logicalUnitsFactor
		// added to the y axis coordinate.
		float logicalUnitsFactor = (float)(ppiPrinterX/(scale*25.4));

		POList* pList = &pApp->m_pagesList;
		POList::Node* pos = pList->Item(pApp->m_nCurPage-1);
		// whm 27Oct11 added test to return FALSE if pos == NULL
		// This test is needed to prevent crash on Linux because the Draw()
		// function can get triggered by the Linux system on that platform
		// before App's m_nCurPage is calculated by OnPreparePrinting()'s
		// call of PaginateDoc() in the printing framework (see notes on
		// calling order of print routines starting at line 108 of
		// AIPrintout.cpp).
		// BEW 28Oct11, the addition of m_bPagePrintInProgress should make the protection
		// of the next 8 line block unneeded, but we retain it as a safety-first measure
		if (pos == NULL)
		{
			// Most likely this would be the result of a programming error
			pApp->m_bPagePrintInProgress = FALSE; // BEW 28Oct11 added
			wxString msg = _T("The value of the App's m_nCurPage is %d in OnPrintPage()");
			msg = msg.Format(msg,pApp->m_nCurPage);
			wxASSERT_MSG(FALSE,msg);
			return FALSE; // note: this ends the printing job
		}
		PageOffsets* pOffsets = (PageOffsets*)pos->GetData();

        // BEW added 10Jul09; inform CLayout of the PageOffsets instance which is current
        // for the page being printed
		pLayout->m_pOffsets = pOffsets; // still needed in the __WXGTK__ build to
                                        // communicate start and end strip indices
                                        // to various functions

		this->SetLogicalOrigin(0,0);
		wxRect fitRect = this->GetLogicalPageMarginsRect(*pApp->pPgSetupDlgData);

#if defined(_DEBUG) && defined(Print_failure)
		int internalDC_Y;
		internalDC_Y = pDC->DeviceToLogicalY(0);
		//wxLogDebug(_T("*****  PAGE = %d   ********   DC offset: %d , pOffsets: nTop %d  nBottom %d , nFirstStrip %d  nLastStrip %d"),
		//	pApp->m_nCurPage, internalDC_Y, pOffsets->nTop, pOffsets->nBottom, pOffsets->nFirstStrip, pOffsets->nLastStrip);
		wxLogDebug(_T("\n\n          *****  PAGE = %d   ********   nFirstStrip %d  nLastStrip %d  ;  nTop %d  nBottom %d  (in logical units)"),
			pApp->m_nCurPage, pOffsets->nFirstStrip, pOffsets->nLastStrip, pOffsets->nTop , pOffsets->nBottom);
		//wxLogDebug(_T("overallScale  x %4.6f  y %4.6f "), overallScaleX, overallScaleY);
#endif

        // Set the upper left starting point of the drawn page to the point where the upper
        // margin and left margin intersect. We call SetLogicalOrigin() called on
        // AIPrintout below to set the initial drawing point for what's drawn on this page
        // to the upper left corner - beginning at the point where the printout's logical
        // top and left page margins intersect.
        // This gets the paper "device" set up right, then below we use the top left of the
        // first strip to be printed for the setting of the origin for the pDC we pass to
        // CLayout for strip printing -- this makes the strip print at the right location
        // in the paper "device".
		this->SetLogicalOrigin(fitRect.x, fitRect.y);

#if defined(Print_failure) && defined(_DEBUG)
		wxLogDebug(_T("this->SetLogicalOrigin(), this = AIPrintout:wxPrintout x %d , y %d"),
			fitRect.x, fitRect.y );
#endif

        // SetLogicalOrigin is only documented as a method of wxPrintout, but it is also
        // available for wxDC. Since the "Strips" that will be drawn in OnDraw() below
        // store their logical coordinates based on their position in the whole virtual
        // document, we need to set the logical origin so that strips will start drawing
        // from the top of our printout/preview page. (Unfortunately, in the Linux build
		// this generates bogus values - a further tweak is done just a little further down)
		pDC->SetLogicalOrigin(0,pOffsets->nTop);

#if defined(Print_failure) && defined(_DEBUG)
		wxLogDebug(_T("pDC->SetLogicalOrigin(),   x %d  y %d (Linux y-value is unreliable),  m_bIsPrinting = %d"),
			0, pOffsets->nTop, pApp->m_bIsPrinting);
#endif

    // A simple solution to get data printed at the right device offset on pages 2 and higher
    // - it overrides some of the above settings (which don't generate correct values)
    // if (!IsPreview()) <<-- the old test, prior to 29Aug12
    // BEW changed 29Aug12 to the test below: the OR followed by the preview test ANDed with
    // the gbIsBeingPreviewed flag ( which remains FALSE whenever a Print Preview button click
    // is done, because it's set TRUE only for a Print Preview menu item choice - the latter's
    // handler in the view class is where it gets set, and that is bypassed when the Print
    // Preview command comes from within the wx printing framework's native Print dialog for
    // the Linux platform) -- was vital. I used this flag to ensure the logical origin tweak is
    // also done when the Print Preview button was pressed in the native Print control. That
    // turned out to be all that was needed to stop all the pages being printed at the first
    // page shown in print preview frame's window, and subsequent pages all being blank except
    // for the footer. Apparently, the wrong offset was in effect, and the tweak was needed to
    // get it right. Then everything became normal. A nice bit of serendipity, as I wasn't
    // expecting such a trivial solution to Kim's reported bug of 17July12!
    if (!IsPreview()
        ||
        (IsPreview() && !gbIsBeingPreviewed)
        )
    {
        // the following appears to be a satisfactory way for getting the Linux print to work right
        // on a 'real' page, as far as where the strips are to be on the page; it doesn't fix the
        // footer - that requires a separate recalculation which we do at the end, when the pDC is
        // not needed for anything else on this page
        int Yoffset = 0;
        int leading = pLayout->GetCurLeading(); // in pixels -- subtract this from nTop to get logical Yoffset
        Yoffset = -(pOffsets->nTop - leading); // this puts the device's topleft of page printing area at the
                                               // topleft of the page's first strip, plus the amount of
                                               // leading required for the nav text area
        pDC->SetLogicalOrigin(0,Yoffset); // doing it outside of the wxPrintout subclass presumably leaves
                                          // the latter's left margin setting intact, and so the margin
                                          // setting remains correct
#if defined(Print_failure) && defined(_DEBUG)
        wxLogDebug(_T("Linux solution's Logical Origin corrected setting: pDC->SetLogicalOrigin(0,Yoffset)  x %d  y %d  for Page %d"),
			0, Yoffset, page);
#endif
    }

    // Do the aggregation of the drawing rectangles and their association with a single
    // strip for each, and storage in appropriate arrays for use in the interleaving
    // of drawing free translations between strips in the second for loop below
    int index;
    CSourcePhrase* pSrcPhrase = NULL;
    int nFirstStrip = pOffsets->nFirstStrip;
    int nLastStrip = pOffsets->nLastStrip;
    int nStripsOffset = 0;
    CStrip* pStrip = NULL;
    CStrip* pFirstStrip = NULL;
    CStrip* pLastStrip = NULL;
    wxArrayPtrVoid* pPilesArray = NULL; // pile array stored by a given strip
    int nPileCount = 0; // a count of how many piles are in the above pPilesArray
    CPile* pPile = NULL;
    CPile* pFirstPile = NULL; // first pile (an anchor pile) which has any free
                              // translation material on the page -- this pile may
                              // actually be in a strip at or near the end of the
                              // previous page
    CPile* pLastPile = NULL; // last pile of last strip on the page

    // BEW changed 29Aug12, make use of gbIsBeingPreviewed which is set TRUE only when
    // print preview is entered by File > Print Preview menu item; if entered by the
    // Print Preview button of the native Linux Print dialog, it remains FALSE
    if ((gbCheckInclFreeTransText && !pApp->m_bIsPrintPreviewing)
        ||
        (!gbCheckInclFreeTransText && (pApp->m_bIsPrintPreviewing && !gbIsBeingPreviewed)) // no free translation, but preview by native dlg button
        ||
        (gbCheckInclFreeTransText && (pApp->m_bIsPrintPreviewing && !gbIsBeingPreviewed)) // has free translation, but preview by native dlg button
        )
    {
        nStripsOffset = nFirstStrip; // subtracting this from the strip index gives the
                                     // Item() index for the array stored in either
                                     // arrFTElementsArrays or arrFTSubstringsArrays
        pFirstStrip = (CStrip*)(pStripArray->Item(nFirstStrip));
        pLastStrip = (CStrip*)(pStripArray->Item(nLastStrip));
        pFirstPile = pFirstStrip->GetPileByIndex(0); // first on page, but it may not
                // be an anchor pile, so check and if it isn't, search back to the
                // nearest anchor pile - doesn't matter if it's on previous page, because
                // we'll eventually only grab as much of it's free translation as lies
                // on the start of the current page -- beware, if processing a selection
                // or a page-range choice, in the GTK build these both temporarily become
                // the whole document, and so it's quite possible that there is no
                // "previous" page, and so possible no previous anchor to be found -- if
                // this is the case, the GetPrevPile() call would return NULL. Use this
                // to build some safety-first code below
        pPile = pFirstPile; // if it's an anchor pile, the next block won't be entered
        if (!pApp->GetFreeTrans()->IsFreeTranslationSrcPhrase(pPile))
        {
            CPile* pKickOffPile = pPile; // in case we need to return to it
            pPile = pApp->GetFreeTrans()->FindPreviousFreeTransSection(pPile);
            if (pPile == NULL)
            {
                // didn't find one, so instead search ahead for the next anchor
                pPile = pKickOffPile;
                pPile = pApp->GetFreeTrans()->FindNextFreeTransSection(pPile);
                if (pPile == NULL)
                {
                    // we couldn't find a free translation section anywhere ahead, set
                    // pFirstPile to NULL and use it as a flag to skip further processing
                    pFirstPile = NULL;
                }
                if (pFirstPile != NULL)
                {
                    // pPile is an anchor location, see if it falls within the current page
                    int aStripIndex = pPile->GetStripIndex();
                    if (aStripIndex > nLastStrip)
                    {
                        // this free translation lies beyond the end of the current page for printing,
                        // so ignore it - and skip the free translation stuff for this page
                        pFirstPile = NULL;
                    }
                    else
                    {
                        // we have an anchor pile on the current page
                        pFirstPile = pPile;
                    }
                }
            }
            else
            {
                // we have an anchor pile - it may or may not be on the current page, but
                // at minimum it's free translation span extends onto the current page
                pFirstPile = pPile;
            }
        }

        // If we didn't find an anchor pile, skip looking for the free trans end
        if (pFirstPile != NULL)
        {
            // we have the first anchor pile which potentially has material on the
            // current page, now find the last pile on the page - whether it's the
            // end of a free translation section or not
            pPilesArray = pLastStrip->GetPilesArray();
            nPileCount = pPilesArray->GetCount();
            pLastPile = pLastStrip->GetPileByIndex(nPileCount - 1);
            wxASSERT(pLastPile != NULL);

            // do the aggregation loop, but only if there is at least one free translation
            // was found on the page, and it's not empty
            pPile = pFirstPile;
            bool bIsFTrAnchor = FALSE;
            do {
                bIsFTrAnchor = pApp->GetFreeTrans()->IsFreeTranslationSrcPhrase(pPile);
                if (bIsFTrAnchor)
                {
                    // deal with this particular free translation, if it's not empty
                     if (!pPile->GetSrcPhrase()->GetFreeTrans().IsEmpty())
                    {
                        // it's a non-empty free translation section, aggregate it
                        pApp->GetFreeTrans()->AggregateOneFreeTranslationForPrinting(
                                    pDC, pLayout, pPile, arrFTElementsArrays,
                                    arrFTSubstringsArrays, nStripsOffset, arrPileSet,
                                    arrRectsForOneFreeTrans);
                        // clean up
                        arrPileSet.Clear(); // empty to be ready for next iteration
                        arrRectsForOneFreeTrans.Clear(); // ditto
                    }
                    // if it's empty, there's no point in trying to set up for printing this
                    // particular free trans section, so just scan on to the next
                }
                pPile = pView->GetNextPile(pPile);
                if (pPile == NULL)
                break;
            } while (pPile != pLastPile);
            // deal with the last one
            if (pPile != NULL && pPile == pLastPile)
            {
                pSrcPhrase = pPile->GetSrcPhrase();
                if (!pSrcPhrase->GetFreeTrans().IsEmpty())
                {
                    // it's a new non-empty free translation section, aggregate it
                    pApp->GetFreeTrans()->AggregateOneFreeTranslationForPrinting(
                                pDC, pLayout, pPile, arrFTElementsArrays,
                                arrFTSubstringsArrays, nStripsOffset, arrPileSet,
                                arrRectsForOneFreeTrans);
                    // clean up
                    arrPileSet.Clear(); // empty to be ready for next iteration
                    arrRectsForOneFreeTrans.Clear(); // ditto
                }
            }
        }
    } // end of TRUE block for test: if (gbCheckInclFreeTransText && !pApp->m_bIsPrintPreviewing ... etc

#if defined(Print_failure) && defined(_DEBUG)
    {
    int i;
    int cnt;
    if (!arrFTElementsArrays.IsEmpty())
    {
        cnt = arrFTElementsArrays.GetCount();
        wxLogDebug(_T("\n\n    OnPrintPage(): ++++++  PAGE = %d  ++++++, arrFTElementsArrays and  arrFTSubstringsArrays "), page);
        for (i=0; i<cnt; i++)
        {
            wxArrayPtrVoid* pAPV = (wxArrayPtrVoid*)arrFTElementsArrays.Item(i);
            if (pAPV->IsEmpty())
                break;
            wxArrayString* pAS = (wxArrayString*)arrFTSubstringsArrays.Item(i);
            if (pAS)
                break;
            int numFTs = pAPV->GetCount();
            int numStrs = pAS->GetCount();
            wxString lastStr = pAS->Item(numStrs - 1);
            wxLogDebug(_T("    OnPrintPage() strip index = %d  ,  num FreeTrElement structs = %d ,  num substrings = %d , last substring = %s"),
                       i, numFTs, numStrs, lastStr.c_str());
        }
    }
    }
#endif

    // For the Linux build, we needed to draw the piles within OnPrintPage() rather
    // than from within pView->OnDraw(pDC) -- probably because doing this below I
    // didn't have bogus pDC->Clear() involved - that was part of the problem. However
    // although I've removed the clipping calls, that didn't make the legacy code
    // functional in the Linux build. So the conditional __WXGTK__ block is needed.
    CPile* aPilePtr = NULL;
    for (index = nFirstStrip; index <= nLastStrip; index++)
    {
        pStrip = (CStrip*)(pStripArray->Item(index));
        pPilesArray = pStrip->GetPilesArray();
        nPileCount = pPilesArray->GetCount();
        aPilePtr = NULL;
        for (i = 0; i < nPileCount; i++)
        {
            aPilePtr = (CPile*)pPilesArray->Item(i);
            aPilePtr->Draw(pDC); // this refuses to draw the gloss cell data in print
                                 // preview and also in a print of real pages, so the
                                 // direct draw with the next block immediately below
                                 // is the work-around needed
            // both real page print and print preview need the following line; it
            // internally tests for gCheckInclGlossesText TRUE and m_bIsPrinting TRUE,
            // and draws a gloss only if the pile has one to be drawn
            pApp->GetFreeTrans()->DrawOneGloss(pDC, aPilePtr, bRTLLayout);
        }
#if defined(Print_failure) && defined(_DEBUG)
        wxLogDebug(_T("OnPrintPage(): just printed strip  %d  for page  %d  ; nFirstStrip = %d   nLastStrip = %d  "),
                   index, page , nFirstStrip, nLastStrip);
#endif

        // Test interleaving of the print of the free translations between
        // printing of the strips - this keeps a top-down printing order, which
        // the wxPostScriptDC seems to demand before it will behave... it works!
        // But call the function only if not previewing, and the user has requested that
        // free translations be drawn, and that there actually are some on the page to
        // be drawn; or when previewing due to a click on the Print Preview button in
		// the native Linux Print dialog (gbIsBeingPreviewed is FALSE in the latter case)
        //if (!pApp->m_bIsPrintPreviewing && gbCheckInclFreeTransText && !arrFTElementsArrays.IsEmpty())
        if ((!pApp->m_bIsPrintPreviewing && gbCheckInclFreeTransText && !arrFTElementsArrays.IsEmpty())
            ||
            (pApp->m_bIsPrintPreviewing && gbCheckInclFreeTransText && !gbIsBeingPreviewed && !arrFTElementsArrays.IsEmpty())
            )
        {
#if defined(Print_failure) && defined(_DEBUG)
        wxLogDebug(_T("OnPrintPage(): about to draw free translations: passing in currentStrip = %d , nStripsOffset  %d  arrFTElementsArrays count = %d  arrFTSubstringsArrays count = %d"),
                   index, nStripsOffset, arrFTElementsArrays.GetCount(), arrFTSubstringsArrays.GetCount());
#endif
           // draw the free translations for the just-drawn strip in the free translation
            // rectangle below it
            pApp->GetFreeTrans()->DrawFreeTransForOneStrip(pDC, index, nStripsOffset,
                                        arrFTElementsArrays, arrFTSubstringsArrays);
        }
    }
    // Print Previewing (which uses a memoryDC which draws to a bitmap, works fine with
    // the legacy code for drawing of the free translations on preview pages - so we'll
    // continue to use it; for real pages however, it fails to print any free translations

    if ((gbCheckInclFreeTransText && pApp->m_bIsPrintPreviewing && gbIsBeingPreviewed) // for previewing by File > Print Preview menu item
       )
    {
        // this legacy function works only for Print Preview; for print to real pages, the
        // 'interleaving' solution above works instead
        pApp->GetFreeTrans()->DrawFreeTranslationsForPrinting(pDC, pApp->GetLayout());
    }

    // Now draw the footer for the page (logical origin for the printout page is at 0,0)
    // but for the Linux build, (0,0) is the paper top left
    if (gbPrintFooter)
    {
        // we use a different version of the function, and calculate a local fitRect
        // internally from the different passed in parameters
        wxPoint paperTopLeft;
        wxPoint paperBottomRight;
        wxPoint paperDimensions;
        paperTopLeft = pApp->pPgSetupDlgData->GetMarginTopLeft(); // values are mm
        paperBottomRight = pApp->pPgSetupDlgData->GetMarginBottomRight(); // values are mm
        // convert to pixels
        paperTopLeft.x = wxRound((float)(float)paperTopLeft.x * ((float)ppiScreenX / 25.4));
        paperTopLeft.y = wxRound((float)(float)paperTopLeft.y * ((float)ppiScreenY / 25.4));
        paperBottomRight.x = wxRound((float)(float)paperBottomRight.x * ((float)ppiScreenX / 25.4));
        paperBottomRight.y = wxRound((float)(float)paperBottomRight.y * ((float)ppiScreenY / 25.4));
        wxRect paperRect = this->GetLogicalPaperRect(); // bad values are returned, but width
                                                        // and height are correct (mm)
        paperDimensions.x = paperRect.GetWidth();
        paperDimensions.y = paperRect.GetHeight();
        pDC->SetLogicalOrigin(0,0);
        pView->PrintFooter(pDC,paperTopLeft, paperBottomRight, paperDimensions, logicalUnitsFactor, page);
    }

#if defined(_DEBUG) && defined(Print_failure)
    wxLogDebug(_T("AIPrintout OnPrintPage() __WXGTK__ block, at its end: gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d , m_bFreeTranslationMode = %d"),
               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)pApp->m_bFreeTranslationMode);
#endif

        // Do here the cleanup of the temporary arrays for printing strips, and free
        // translation
        if (gbCheckInclFreeTransText)
        {
			if (!arrFTElementsArrays.IsEmpty())
			{
				int count = arrFTElementsArrays.GetCount();
				int index;
				for (index = 0; index < count; index++)
				{
					wxArrayPtrVoid* pAPV = (wxArrayPtrVoid*)arrFTElementsArrays.Item(index);
					int index2;
					int count2 = pAPV->GetCount();
					for (index2 = 0; index2 < count2; index2++)
					{
						FreeTrElement* pElement = (FreeTrElement*)pAPV->Item(index2);
 						if (pElement != NULL) // whm 11Jun12 added NULL test
						   delete pElement;
					}
					pAPV->Clear();
					if (pAPV != NULL) // whm 11Jun12 added NULL test
						delete pAPV;
				}
            }
            // arrFTElementsArrays is local to the OnPrintPage() function, and will be destroyed
            // by it's destructor automatically

            // for the following array, the strings can be removed by a Clear() call, but
            // the contained arrays were created on the heap and so must be deleted here
            int arraysCount = arrFTSubstringsArrays.GetCount();
            for (index = 0; index < arraysCount; index++)
            {
                wxArrayString* pAS = (wxArrayString*)arrFTSubstringsArrays.Item(index);
                pAS->Clear();
 				if (pAS != NULL) // whm 11Jun12 added NULL test
	               delete pAS;
            }
        }

		pApp->m_bPagePrintInProgress = FALSE; // BEW 28Oct11 added
		return TRUE;
    }
    else
        return FALSE;
    // end of code for the __WXGTK__ build

#else

    // start of (legacy) code, which now is only for the Windows and Mac builds
    if (pDC)
    {
		pApp->m_bPagePrintInProgress = TRUE; // BEW 28Oct11 added

		pApp->m_nCurPage = page; // set the app member for use by CStrip's Draw() function

		// The code block below properly scales the text to appear the correct size within both
		// the print preview and on the printer.

		// Scale the display context to the correct size for either the preview or for printer.
		// OnPrintPage() is called for both print preview and for actual printing to printer.

		// Get the logical pixels per inch of screen and printer
		int ppiScreenX, ppiScreenY;
		GetPPIScreen(&ppiScreenX, &ppiScreenY);
		int ppiPrinterX, ppiPrinterY;
		GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
        // This scales the DC so that the printout roughly represents the the screen scaling. The
        // wxPrintout docs say, "The text point size _should_ be the right size but in fact is too small
        // for some reason. This is a detail that will need to be addressed at some point but can be
        // fudged for the moment."
		float scale = (float)((float)ppiPrinterX/(float)ppiScreenX);

		// Now we have to check in case our real page size is reduced (e.g. because
		// we're drawing to a print preview memory DC)
		int pageWidthInPixels, pageHeightInPixels;
		int w, h;
		pDC->GetSize(&w, &h);
		this->GetPageSizePixels(&pageWidthInPixels, &pageHeightInPixels);
        // If printer pageWidth == current DC width, then this doesn't change. But w might be the
        // preview bitmap width, so scale down.
		float overallScaleX = scale * (float)(w/(float)pageWidthInPixels);
		float overallScaleY = scale * (float)(h/(float)pageHeightInPixels);
		pDC->SetUserScale(overallScaleX, overallScaleY);

		// Calculate the scaling factor for passing to PrintFooter. This factor must be multiplied by
		// any absolute displacement distance desired which is expressed in mm. For example, we want
		// the footer to be placed 12.7mm (half inch) below the bottom printing margin. In logical
		// units a 12.7mm displacement below the bottom printing margin is 12.7*logicalUnitsFactor
		// added to the y axis coordinate.
		float logicalUnitsFactor = (float)(ppiPrinterX/(scale*25.4));

		POList* pList = &pApp->m_pagesList;
		// check the m_nCurPage value has not become too big -- this can happen if the user
		// wants to print to a certain page, but turns off free translation and or glosses
		// printing (which shortens the number of pages needed to less than what he chose
		// for the last page) So check and return if such a page isn't available
		int how_many_pages;
		how_many_pages = pList->GetCount();
		if (pApp->m_nCurPage > how_many_pages)
		{
			return FALSE;
		}
		POList::Node* pos = pList->Item(pApp->m_nCurPage-1);
		// whm 27Oct11 added test to return FALSE if pos == NULL
		// This test is needed to prevent crash on Linux because the Draw()
		// function can get triggered by the Linux system on that platform
		// before App's m_nCurPage is calculated by OnPreparePrinting()'s
		// call of PaginateDoc() in the printing framework (see notes on
		// calling order of print routines starting at line 108 of
		// AIPrintout.cpp).
		// BEW 28Oct11, the addition of m_bPagePrintInProgress should make the protection
		// of the next 8 line block unneeded, but we retain it as a safety-first measure
		if (pos == NULL)
		{
			// Most likely this would be the result of a programming error
			pApp->m_bPagePrintInProgress = FALSE; // BEW 28Oct11 added
			wxString msg = _T("The value of the App's m_nCurPage is %d in OnPrintPage()");
			msg = msg.Format(msg,pApp->m_nCurPage);
			wxASSERT_MSG(FALSE,msg);
			return FALSE; // note: this ends the printing job
		}

		PageOffsets* pOffsets = (PageOffsets*)pos->GetData();

        // BEW added 10Jul09; inform CLayout of the PageOffsets instance which is current
        // for the page being printed - CStrip::Draw() needs this information in order to
        // work out the strips to be drawn
		pLayout->m_pOffsets = pOffsets;

        // Note: Printing of headers and/or footers needs to be done before we call
        // pDC->SetLogicalOrigin(0,pOffsets->nTop) below because headers and footers are always placed
        // on the page in constant coordinates in relation to the top and bottom page margins (scaled
        // for the appropriate pDC display context).

		// For drawing of footer, whose position is in relation to the whole page, margins, etc., it is easiest
		// to set the AIPrintout logical origin at 0,0 and draw the footer with x and y coords in
		// relation to the upper left corner of the paper.

		// BEW changed 15Jul09: wxPrintout is not documented, and the Printing Guidlines
		// are very sparse. It turns out that the printed page, from the top left point
		// where the margins intersect, functions like the client rectangle when printing
		// to the view window - that is, it is in "device" coordinates. That means that we
		// have to reset the AIPrintout DC's origin to (0,0) before we call
		// GetLogicalPageMarginsRect() because the latter needs to get the top left x and
		// y coordinates relative to a (0,0) "page" margin (the printing area, not the
		// physical page top left) -- for our default margins in Adapt It, that means
		// we'll get (x=72, y=78) for the x & y values in fitRect -- we have to do this
		// resetting of the origin to (0,0) adjustment for every page printed, (has no
		// effect when print previewing, the latter stays correct). Then, after printing
		// the footer using that (0,0) origin for the AIPrintout DC, we then have to make
		// the printing of strips start at the (fitRect.x,fitRect.y) as origin, so we do a
		// call to SetLogicalOrigin() with those coordinates. Then the printing comes out
		// right for each page. Finally!
		this->SetLogicalOrigin(0,0);
		wxRect fitRect = this->GetLogicalPageMarginsRect(*pApp->pPgSetupDlgData);

#if defined(_DEBUG) && defined(Print_failure)
		wxLogDebug(_T("fitRect = this->GetLogicalPageMarginsRect() x %d  y %d , width %d  height %d"),
			fitRect.x, fitRect.y, fitRect.width, fitRect.height);
#endif

/* some rects I investigated to see what they x, y, width and height values are, but we don't need them
		wxRect paperRectPixels = this->GetPaperRectPixels();
#ifdef Print_failure
		wxLogDebug(_T("paperRectPixels = this->GetPaperRectPixels() x %d  y %d , width %d  height %d"),
			paperRectPixels.x, paperRectPixels.y, paperRectPixels.width, paperRectPixels.height);
#endif

		wxRect paperRectLogical = this->GetLogicalPaperRect();
#ifdef Print_failure
		wxLogDebug(_T("paperRectLogical = this->GetLogicalPaperRect() x %d  y %d , width %d  height %d"),
			paperRectLogical.x, paperRectLogical.y, paperRectLogical.width, paperRectLogical.height);
#endif

		wxRect pageRectLogical = this->GetLogicalPageRect();
#ifdef Print_failure
		wxLogDebug(_T("pageRectLogical = this->GetLogicalPageRect() x %d  y %d , width %d  height %d"),
			pageRectLogical.x, pageRectLogical.y, pageRectLogical.width, pageRectLogical.height);
#endif
*/
		// Now draw the footer for the page (logical origin for the printout page is at 0,0)
		if (gbPrintFooter)
		{
			pView->PrintFooter(pDC,fitRect,logicalUnitsFactor,page);
		}
        // Set the upper left starting point of the drawn page to the point where the upper
        // margin and left margin intersect. We call SetLogicalOrigin() called on
        // AIPrintout below to set the initial drawing point for what's drawn on this page
        // to the upper left corner - beginning at the point where the printout's logical
        // top and left page margins intersect.
        // This gets the paper "device" set up right, then below we use the top left of the
        // first strip to be printed for the setting of the origin for the pDC we pass to
        // CLayout for strip printing -- this makes the strip print at the right location
        // in the paper "device".
		this->SetLogicalOrigin(fitRect.x, fitRect.y);

#if defined(Print_failure) && defined(_DEBUG)
		wxLogDebug(_T("this->SetLogicalOrigin(), this = AIPrintout:wxPrintout x %d , y %d"),
			fitRect.x, fitRect.y );
#endif

        // SetLogicalOrigin is only documented as a method of wxPrintout, but it is also
        // available for wxDC. Since the "Strips" that will be drawn in OnDraw() below
        // store their logical coordinates based on their position in the whole virtual
        // document, we need to set the logical origin so that strips will start drawing
        // from the top of our printout/preview page.
		pDC->SetLogicalOrigin(0,pOffsets->nTop);

		pView->OnDraw(pDC);

		pApp->m_bPagePrintInProgress = FALSE; // BEW 28Oct11 added
		return TRUE;
    }
    else
        return FALSE;

    // end of (legacy) code,  which now is only for the Windows and Mac builds
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// Called from: Called by the framework after OnPreparePrinting() and GetPageInfo() but before
/// OnBeginDocument(). OnBeginPrinting() is only called once at the beginning of printing.
/// It is not called when the user selects a different page from print preview.
////////////////////////////////////////////////////////////////////////////////////////////
void AIPrintout::OnBeginPrinting()
{
	// refactored 6Apr09 -- do nothing except ensure the gbIsPrinting flag is TRUE
	m_pApp->m_bIsPrinting = TRUE;

	// set the mapping mode
	// The highest resolution choices are wxMM_TWIPS and wxMM_LOMETRIC.
	// With wxMM_TWIPS each logical unit is 1/20th point or 1/1440th of an inch.
	// With wxMM_LOMETRIC each logical unit is 1/10th of a mm or roughly 1/250th of an inch.
	// With both wxMM_TWIPS and wxMM_LOMETRIC positive x is to the right, positive y is up.
	// See code:#print_flow for the order of calling of OnBeginPrinting().
	// whm Note: It turns out that we maintain the wxMM_TEXT map mode even when printing in the wx
	// version, therefore the SetMapMode call below is commented out.
	//GetDC()->SetMapMode(wxMM_LOMETRIC); // MFC uses MM_LOENGLISH in which each logical unit is 1/100th of an inch
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// Called from: CAUTION: OnEndPrinting() works differently from the function of the same name
/// in MFC!! Like MFC, OnEndPrinting() is called by the framework after a given document has
/// printed. However, surprisingly, OnEndPrinting() is also called after each page is displayed
/// in print preview mode. Also, note that when more than one copy of a document is printed,
/// OnEndPrinting() is called at the end of each document copy, and before the OnEndPrinting()
/// method of wxPrintout is executed. Therefore, the behavior of OnEndPrinting in the wx version
/// makes it unsuitable as a location for the cleanup code that is done in MFC's OnEndPrinting().
/// The cleanup code has moved to AIPrintout's destructor in the wx version.
////////////////////////////////////////////////////////////////////////////////////////////
void AIPrintout::OnEndPrinting()
{
    // See code:#print_flow for the order of calling of OnEndPrinting().	
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if the page will be printed; FALSE if the printing of pages for the current
///             document copy should stop (OnPrintPage will no longer be called for the current
///             document), and print control will proceed to OnEndDocument().
/// \param      pageNum     ->
/// \remarks
/// Called from: Called the first time after OnBeginDocument(). It is called just before each
/// page is printed. By default, HasPage() behaves as if the document has only one page.
////////////////////////////////////////////////////////////////////////////////////////////
bool AIPrintout::HasPage(int pageNum)
{
	CAdapt_ItApp* pApp = &wxGetApp();
#ifdef Print_failure
#if defined(_DEBUG) && defined(__WXGTK__)
//    wxLogDebug(_T("AIPrintout HasPage() line 968 at start: gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d , m_bFreeTranslationMode = %d"),
//               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)pApp->m_bFreeTranslationMode);
#endif
#endif
 	// refactored 6Apr09
    // See code:#print_flow for the order of calling of HasPage().
	wxPrintDialogData printDialogData(*pApp->pPrintData);
	// for ease of debugging, use some local vars here
	// BEW changed 29Nov11, because the printing framework in wxWidgets is broken
	// -- it doesn't get the max page value, and defaults it to 9999. So the
	// fix below is needed to avoid the crash described in the next comment.
	int myMinPage;
	int myMaxPage;
	myMinPage = printDialogData.GetMinPage();
	//myMaxPage = printDialogData.GetMaxPage(); <<-- this is always 9999, even in
	//the Windows & Mac builds, which is useless for preventing crash when the
	//page range shortens because the user chose a page range with a high ending
	//value but disallows printing of free translations and glosses which were used
	//in the estimation of the total number of pages, and in so doing the number for
	//the max page becomes greater than the actual numbers of pages available, so try
	//getting the value from the app's m_pagesList's count, which is always up to
	//date at this point
	myMaxPage = pApp->m_pagesList.GetCount();
	if (pageNum >= myMinPage && pageNum <= myMaxPage)
		return TRUE;
	else
		return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     FALSE if the user has cancelled printing
/// \param      startPage     -> the minimum page value that the user can select for printing or previewing
/// \param      endPage       -> the maximum page value that the user can select for printing or previewing
/// \remarks
/// Called from: the printing framework after OnBeginPrinting() and once just before each copy of a
/// multi-copy printout (it is only called once if only 1 copy of a document is printed.
////////////////////////////////////////////////////////////////////////////////////////////
bool AIPrintout::OnBeginDocument(int startPage, int endPage)
{
	// refactored 6Apr09
	// See code:#print_flow for the order of calling of OnBeginDocument().

    if (!wxPrintout::OnBeginDocument(startPage, endPage))
        return false;

	// this initialization of gbIsPrinting need to be done because OnEndPrinting is called after each
	// page is rendered in print preview and m_bIsPrinting is set FALSE there.
	m_pApp->m_bIsPrinting = TRUE;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      minPage       <- the minimum page value that the user can select for printing or previewing
/// \param      maxPage       <- the maximum page value that the user can select for printing or previewing
/// \param      selPageFrom   <- the starting page of any selection of pages made by the user
/// \param      selPageTo     <- the ending page of any selection of pages made by the user
/// \remarks
/// Called from: The printing framework; OnPreparePrinting() is called immediately before this
/// GetPageInfo(). In the preceding call to OnPreparePrinting(), PaginateDoc() is called which
/// populates the App's m_pagesList which this GetPageInfo accesses to determine its four parameter
/// values which are returned to the caller. GetPageInfo() is only called once at the beginning of
/// the print or preview; it is not called when the user selects a different page from within print
/// preview. Note: if minPage is zero, the page number controls in the print dialog will be disabled.
////////////////////////////////////////////////////////////////////////////////////////////
void AIPrintout::GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo)
{
 	// refactored 6Apr09
	// See code:#print_flow for the order of calling of GetPageInfo().
 	CAdapt_ItApp* pApp = &wxGetApp();

	wxPrintDialogData printDialogData(*pApp->pPrintData);
	*minPage = 1;
	*maxPage = pApp->m_pagesList.GetCount();
	if (printDialogData.GetAllPages())
	{
		*selPageFrom = 1;
		*selPageTo = pApp->m_pagesList.GetCount();
	}
	else
	{
        // the selPageFrom and selPageTo can't be changed from here, so we can give any values
        // OnPrint() has to get the user's page range choice
		*selPageFrom = printDialogData.GetMinPage();
		*selPageTo = printDialogData.GetMaxPage();

	}
#ifdef Print_failure
#if defined(_DEBUG) && defined(__WXGTK__)
//    wxLogDebug(_T("AIPrintout GetPageInfo() line 1057 at end: gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d , m_bFreeTranslationMode = %d"),
//               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)pApp->m_bFreeTranslationMode);
#endif
#endif

}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return         nothing
/// \remarks
/// Called from: OnPreparePrinting() is called automatically by the framework when an
/// OnPrint() event is handled (via wxID_PRINT standard Identifier). According to the docs,
/// "It is called once by the framework before any other demands are made of the wxPrintout
/// object. This gives the object an opportunity to calculate the number of pages in the
/// document, and any other print preparations that are necessary before actual printing or
/// print preview is done." OnPreparePrinting() is always called once just before a print
/// preview dialog is displayed, and it is not called again when a different page is
/// selected in print preview mode. When File | Print is selected OnPreparePrinting() is
/// not called until after the the OK button is pressed at the print dialog, so, if user
/// cancels the print dialog OnPreparePrinting() is never called in that case. Unlike the
/// MFC version, which actually calls up the print dialog from within its version of
/// OnPreparePrinting (by calling the MFC DoPreparePrinting(pInfo) function), the wx
/// version works differently. The print dialog is called up in its high level OnPrint()
/// handler at the point where the wxPrinter object's Print() method is called. There is
/// therefore no need for cleanup code to fix indices, call RecalcLayout, etc, here in
/// OnPreparePrinting. All that is done in the AIPrintout's destructor, rather than here in
/// OnPreparePrinting(). See notes within OnPreparePrinting() for more detail.
////////////////////////////////////////////////////////////////////////////////////////////
void AIPrintout::OnPreparePrinting()
{
 	//CAdapt_ItApp* pApp;
 	//pApp = &wxGetApp();
	//CAdapt_ItView* pView = pApp->GetView();

#ifdef Print_failure
#if defined(_DEBUG) && defined(__WXGTK__)
//    wxLogDebug(_T("AIPrintout OnPreparePrinting() line 712 at start: gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d , m_bFreeTranslationMode = %d"),
//               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)pApp->m_bFreeTranslationMode);
#endif
#endif

	m_pApp->m_bIsPrintPreviewing = IsPreview(); // BEW added 5Oct11, so I can do kludges which
			// differ, depending on whether we are print previewing, or printing to paper

	int nPagePrintingWidthLU = 0;
	int nPagePrintingLengthLU = 0;
	bool bAllsWell = m_pApp->CalcPrintableArea_LogicalUnits(nPagePrintingWidthLU, nPagePrintingLengthLU);
	if (!bAllsWell)
	{
		// document state is not changed yet
		return;
	}

	// ensure m_bIsPrinting is turned on - it gets turned off when the Print Options Dlg is
	// shown so that RecalcLayout() calls will draw the view correctly (the user may move
	// the dialog relative to the visible part of the document in the view window), so we
	// must ensure it is back on when OnPreparePrinting()is next called, because the
	// LayoutAndPaginate() call below will need to redo the layout to the strip width as
	// required for the physical page, rather than the view
	m_pApp->m_bIsPrinting = TRUE;

	// use the page printing dimensions to paginate, calling RecalcLayout(), and if there
	// is a selection, to make the selection temporarily become the whole document, and
	// then paginate using PaginateDoc(), a view function, so whatever is then in
	// m_pSourcePhrases is apportioned into a list of PageOffsets structs which specify
	// the beginning and end of what is to be printed on each page. This function is used
	// for both pagination of a print preview, and pagination of an actual real (paper)
	// printout, so that the number of printed pages is always the same number as shown in
	// the print preview. These structs are then used by OnPrintPage() for printing or
	// previewing.
	bAllsWell = m_pApp->LayoutAndPaginate(nPagePrintingWidthLU,nPagePrintingLengthLU);
	if (!bAllsWell)
	{
		// just return, ~AIPrintout() will restore the document's original state
		return;
	}

	// get a pointer to the wxPrintDialogData object (from printing sample)
	wxPrintDialogData printDialogData(*m_pApp->pPrintData);

	// pagination succeeded, so set the initial values for pInfo
	int nTotalPages;
	nTotalPages = m_pApp->m_pagesList.GetCount();
	printDialogData.SetMaxPage(nTotalPages);
	printDialogData.SetMinPage(1);
	printDialogData.SetFromPage(1);
	printDialogData.SetToPage(nTotalPages);

    // whm Note: The MFC version calls a DoPreparePrinting(pInfo) function at this point
    // which puts up the print dialog. The ordering of printing events in the two
    // frameworks is different. In the wx version, the print dialog is put up when the
    // printer.Print() method is called in the higher level OnPrint() handler in the view -
    // this happens BEFORE OnPreparePrinting() is ever called. Hence, in the wx version the
    // OnPrint() handler (and the print dialog) exit before OnPreparePrinting() starts
    // executing. In the MFC version, however, DoPreparePrinting() is called first by MFC's
    // default OnFilePrint() handler (not overridden in MFC code); then, within
    // DoPreparePrinting() the print dialog gets called by DoPreparePrinting() towards the
    // end of OnPreparePrinting. These order differences mandate a radical change in the
    // contents of the various printing functions to implement similar functionality in the
    // wxWidgets version.

    // The MFC version also has a code here (near the end of OnPreparePrinting and long
    // after print dialog is dismissed) to set up things for any range print called for
    // from the print dialog. The setup above sets things up for printing a selection. The
    // setup that the MFC version does at this point sets things up for a chapter:verse
    // range to print. In the wx version that code is placed at the beginning of the View's
    // OnPrint() higher level handler.
    //
    // Also, in the MFC version, if printing had to abort, the remainder of
    // OnPreparePrinting there was devoted to cleaning up indices, flags, doing
    // RecalcLayout, etc - things which the wx version does in the AIPrintout's class
    // destructor.
    //wxLogDebug(_T("OnPreparePrint() END"));

#ifdef Print_failure
#if defined(_DEBUG) && defined(__WXGTK__)
//    wxLogDebug(_T("AIPrintout OnPreparePrinting() line 793 at end: gbCheckInclFreeTransText = %d , gbCheckInclGlossesText = %d , m_bFreeTranslationMode = %d"),
//               (int)gbCheckInclFreeTransText, (int)gbCheckInclGlossesText, (int)m_pApp->m_bFreeTranslationMode);
#endif
#endif
} // end of OnPreparePrinting()



