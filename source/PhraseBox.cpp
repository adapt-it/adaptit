/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			PhraseBox.cpp
/// \author			Bill Martin
/// \date_created	11 February 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the CPhraseBox class.
/// The CPhraseBox class governs the behavior of the phrase or
/// target box where the user enters and/or edits translations while adapting text.
/// \derivation		The PhraseBox class derives from the wxTextCtrl class.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in MainFrm (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////
// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "PhraseBox.h"
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

#include <wx/textctrl.h>

// Other includes uncomment as implemented
#include "Text.h"
#include "Adapt_It.h"
#include "PhraseBox.h"
#include "Adapt_ItView.h"
#include "Adapt_ItCanvas.h" 
#include "Cell.h"
#include "Pile.h" 
#include "Strip.h"
#include "SourcePhrase.h"
#include "Adapt_ItDoc.h"
#include "SourceBundle.h"
#include "RefString.h"
#include "AdaptitConstants.h"
#include "KB.h"
#include "TargetUnit.h"
#include "ChooseTranslation.h" 
#include "MainFrm.h"
#include "helpers.h"
// Other includes uncomment as implemented

// globals

/// This global is defined in MainFrm.cpp.
extern bool gbIgnoreScriptureReference_Send;

extern wxString gOldChapVerseStr;

bool gbSavedLineFourInReviewingMode = FALSE; // TRUE if either or both of m_adaption and m_targetStr is empty
											 // and Reviewing mode is one (we want to preserve punctuation or
											 // lack thereof if the location is a hole)
wxString gStrSaveLineFourInReviewingMode; // works with the above flag, and stores whatever the m_targetStr was
										 // when the phrase box, in Reviewing mode, lands on a hole (we want to
										 // preserve what we found if the user has not changed it)

bool gbNoAdaptationRemovalRequested = FALSE; // TRUE when user hits backspace or DEL key to try remove an earlier
											 // assignment of <no adaptation> to the word or phrase at the active
											 // location - (affects one of m_bHasKBEntry or m_bHasGlossingKBEntry
											 // depending on the current mode, and removes the KB CRefString (if
											 // the reference count is 1) or decrements the count, as the case may be)

/// Used to delay the message that user has come to the end, until after last
/// adaptation has been made visible in the main window; in OnePass() only, not JumpForward().
bool gbCameToEnd = FALSE; 

bool gTemporarilySuspendAltBKSP = FALSE; // to enable gbSuppressStoreForAltBackspaceKeypress flag to be turned
										 // back on when <Not In KB> next encountered after being off for one
										 // or more ordinary KB entry insertions; CTRL+ENTER also gives same result

/// To support the ALT+Backspace key combination for advance to immediate next pile without lookup or
/// store of the phrase box (copied, and perhaps SILConverters converted) text string in the KB or
/// Glossing KB. When ALT+Backpace is done, this is temporarily set TRUE and restored to FALSE
/// immediately after the store is skipped. CTRL+ENTER also can be used for the transliteration.
bool gbSuppressStoreForAltBackspaceKeypress = FALSE; 

bool gbMovingToPreviousPile = FALSE; // added for when user calls MoveToPrevPile( ) and the
	// previous pile contains a merged phrase with internal punctuation - we don't want the
	// ReDoPhraseBox( ) call to call MakeLineFourString( ) and so result in the PlaceMedialPunctuation
	// dialog being put up an unwanted couple of times. So we'll use the gbMovingToPreviousPile being
	// set to then set the gbInhibitLine4StrCall to TRUE, at start of ReDoPhraseBox( ), and turn it off at
	// the end of that function. That should fix it.


/// This global is defined in Adapt_ItView.cpp.
extern bool gbInhibitLine4StrCall; // see view for reason for this

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

/// This global is defined in Adapt_ItView.cpp.
extern int gnBeginInsertionsSequNum;

/// This global is defined in Adapt_ItView.cpp.
extern int gnEndInsertionsSequNum;

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

bool	gbRetainBoxContents = FALSE; // for version 1.4.2; we want deselection of copied source
		// text to set this flag true so that if the user subsequently selects words intending
		// to do a merge, then the deselected word won't get lost when he types something after
		// forming the source words selection (see OnKeyUp( ) for one place the flag is set -
		// (for a left or right arrow keypress), and the other place will be in the view's
		// OnLButtonDown I think - for a click on the phrase box itself)

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// This global is defined in Adapt_It.cpp.
extern bool	gbRTL_Layout;	// ANSI version is always left to right reading; this flag can only
							// be changed in the NRoman version, using the extra Layout menu
extern bool		gbSuppressRemovalOfRefString; // see the view's globals for a definition
bool			gbByCopyOnly = FALSE; // will be set TRUE when the target text is the result of a 
					// copy operation from the source, and if user types to modify it, it is 
					// cleared to FALSE, and similarly, if a lookup succeeds, it is cleared to 
					// FALSE. It is used in PlacePhraseBox() to enforce a store to the KB when
					// user clicks elsewhere after an existing location is returned to somehow, 
					// and the user did not type anything to the target text retrieved from the 
					// KB. In this circumstance the m_bAbandonable flag is TRUE, and the retrieved
					// target text would not be re-stored unless we have this extra flag 
					// gbByCopyOnly to check, and when FALSE we enforce the store operation

/// This global is defined in Adapt_ItView.cpp.
extern short	gnExpandBox;  // see start of Adapt_ItView.cpp for explanation of these two

/// This global is defined in Adapt_ItView.cpp.
extern short	gnNearEndFactor; 

bool gbUnmergeJustDone = FALSE; // this is used to inhibit a second unmerge, when OnButtonRestore()
					// is called from other than in MoveToNextPile() (eg. a click on the Unmerge 
					// A Phrase button); we want to allow the unmerge in OnButtonRestore() (which 
					// will be the first unmerge), and suppress the second that could otherwise be
					// asked for if the user Cancels the Choose Translation dialog from within the
					// ChooseTranslation() call within the LookUpSrcWord() call within 
					// OnButtonRestore(). This is the opposite situatio than for gbSuppressLookup
					// flag's use (the latter suppresses first unmerge but allows second)
bool gbSuppressMergeInMoveToNextPile = FALSE; // if a merge is done in LookAhead() so that the
					// phrase box can be shown at the correct location when the Choose Translation
					// dialog has to be put up because of non-unique translations, then on return 
					// to MoveToNextPile() with an adaptation chosen in the dialog dialog will
					// come to code for merging (in the case when no dialog was needed), and if 
					// not suppressed by this flag, a merge of an extra word or words is wrongly
					// done 
bool gbSuppressLookup = FALSE; // used to suppress the LookUpSrcWord() call in view's 
							   // OnButtonRestore() function when unmerging a merged phrase due 
							   // to Cancel or Cancel And Select being chosen in the Choose 
							   // Translation dialog
bool gbCompletedMergeAndMove = FALSE; // for support of Bill Martin's wish that the phrase box 
						// be at the new location when the Choose Translation dialog is shown
bool gbEnterTyped = FALSE; // used in BuildPhrases() to speed up finding the current srcPhrase

// A node indicating the position of the last source phrase in a list of source phrases.
//SPList::Node* gLastSrcPhrasePos = (SPList::Node*)NULL; // used in GetSrcPhrasePos() call in BuildPhrases() to speed
								   // up ....
bool gbMergeDone = FALSE;
bool gbUserCancelledChooseTranslationDlg = FALSE;
bool gbUserWantsNoMove = FALSE; // TRUE if user wants an empty adaptation to not move because 
					// some text must be supplied for the adaptation first; used in 
					// MoveToImmedNextPile() and the TAB case block of code in OnChar() to 
					// suppress warning message

/// Cursor location - needed for up & down arrow & page up & down arrows, since its
/// already wrong by the time the handlers are invoked, so it needs to be set by OnChar().
long gnStart; 

/// Cursor location - needed for up & down arrow & page up & down arrows, since its
/// already wrong by the time the handlers are invoked, so it needs to be set by OnChar().
long gnEnd;

long gnSaveStart; //int gnSaveStart; // these two are for implementing Undo() for a backspace operation
long gnSaveEnd; //int gnSaveEnd; 

bool			gbExpanding = FALSE; // set TRUE when an expansion of phrase box was just done
					// (and used in view's CalcPileWidth to enable an extra pileWidth adjustment
					// and therefore to disable this adjustment when the phrase box is contracting
					// due to deleting some content - otherwise it won't contract)

/// Contains the current sequence number of the active pile (m_nActiveSequNum) for use by auto-saving.
int nCurrentSequNum;

/// A global wxString containing the translation for a matched source phrase key.
wxString		translation = _T(""); // = _T("") whm added 8Aug04 // translation, for a matched source phrase key

CTargetUnit*	pCurTargetUnit = (CTargetUnit*)NULL; // when valid, it is the matched CTargetUnit instance
wxString		curKey = _T(""); // when non empty, it is the current key string which was matched
int				nWordsInPhrase = 0; // a matched phrase's number of words (from source phrase)
extern bool		bSuppressDefaultAdaptation; // normally FALSE, but set TRUE whenever user is
					// wanting a MergeWords done by typing into the phrase box (which also
				    // ensures cons.changes won't be done on the typing)
extern bool		gbInspectTranslations; // when TRUE suppresses the "Cancel and Select" button 
					// in the CChooseTranslation dialog
bool			gbUserWantsSelection = FALSE; // carries CChooseTranslation's 
					// m_bCancelAndSelect value back to the caller of LookAhead()
extern bool		gbBundleChanged;

/// This global is defined in Adapt_ItView.cpp.
extern int		gnOldSequNum;

extern bool		gbMergeSucceeded;
wxString		gSaveTargetPhrase = _T(""); // used by the SHIFT+END shortcut for unmerging 
											// a phrase



IMPLEMENT_DYNAMIC_CLASS(CPhraseBox, wxTextCtrl)

BEGIN_EVENT_TABLE(CPhraseBox, wxTextCtrl)
	EVT_MENU(wxID_UNDO, CPhraseBox::OnEditUndo)
	EVT_TEXT(-1, CPhraseBox::OnPhraseBoxChanged)
	EVT_CHAR(CPhraseBox::OnChar)
	EVT_KEY_DOWN(CPhraseBox::OnKeyDown)
	EVT_KEY_UP(CPhraseBox::OnKeyUp)
	EVT_LEFT_DOWN(CPhraseBox::OnLButtonDown)
	EVT_LEFT_UP(CPhraseBox::OnLButtonUp)
END_EVENT_TABLE()

CPhraseBox::CPhraseBox(void)
{
	// Problem: The MFC version destroys and recreates the phrasebox every time
	// the box is moved, layout changes, screen is redrawn, etc. In fact, it seems
	// often to be the case that the phrase box contents can remain unchanged, and
	// yet the phrase box itself can go through multiple deletions, and recreations.
	// The MFC design makes it impossible to keep track of a phrase box "dirty" flag 
	// from here within the CPhraseBox class. It seems I could either keep a "dirty"
	// flag on the App, or else redesign the TargetBox/PhraseBox in such a way that
	// it doesn't need to be destroyed and recreated all the time, but can exist
	// at least for the life of a view (on the heap), and be hidden, moved, and 
	// shown when needed. I've chosen the latter.

	m_textColor = wxColour(0,0,0); // default to black
	m_bMergeWasDone = FALSE;
}

CPhraseBox::~CPhraseBox(void)
{
}

bool CPhraseBox::IsActiveLocWithinSelection(const CAdapt_ItView* WXUNUSED(pView), const CPile* pActivePile)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	bool bYes = FALSE;
	const CCellList* pList = &pApp->m_selection;
	if (pList->GetCount() == 0)
		return bYes;
	CCellList::Node* pos = pList->GetFirst();
	while (pos != NULL)
	{
		CCell* pCell = (CCell*)pos->GetData();
		pos = pos->GetNext();
		if (pCell->m_pPile == pActivePile)
		{
			bYes = TRUE;
			break;
		}
	}
	return bYes;
}

void CPhraseBox::JumpForward(CAdapt_ItView* pView)
{
	if (gpApp->m_bDrafting)
	{

		gbBundleChanged = FALSE; // default condition, set TRUE if bSuccessful is 
									// FALSE

		// check the mode, whether or not it is single step, and route accordingly
		if (gpApp->m_bSingleStep)
		{
			// we have to check here if we have moved into the area where it is 
			// necessary to advance the bundle, and if so, then do it, & update
			// everything again
			bool bAdvance = pView->NeedBundleAdvance(gpApp->m_nActiveSequNum);
			if (bAdvance)
			{
				// do the advance, return a new (valid) pointer to the active pile
				gpApp->m_pActivePile = 
					pView->AdvanceBundle(gpApp->m_nActiveSequNum);
				m_pActivePile = gpApp->m_pActivePile; // put copy on the CPhraseBox 
														// too
				gbBundleChanged = TRUE;
			}
			
			// do the move, but first preserve which strip we were in, in case we 
			// jump to next strip we'll have to update the old one too
			int nOldStripIndex = gpApp->m_pActivePile->m_pStrip->m_nStripIndex;

			gbEnterTyped = TRUE; // try speed up by using GetSrcPhrasePos() call in
									// BuildPhrases()
			int bSuccessful = MoveToNextPile(pView,m_pActivePile);
			if (!bSuccessful)
			{
				// it may have failed not because we are at eof, but because there 
				// is nothing in this present bundle at higher locations which lacks
				// an adaptation, in which case the active pile pointer returned 
				// will be null, and so we must jump to an empty pile, or to eof if
				// there are none
				if (gpApp->m_pActivePile == NULL && gpApp->m_endIndex < 
																gpApp->m_maxIndex)
				{
					gbBundleChanged = TRUE; // (nOldStripIndex will be invalid once
											// the bundle is changed)
					
					// hunt for an empty srcPhrase
					CSourcePhrase* pSrcPhrase = 
									pView->GetNextEmptySrcPhrase(gpApp->m_endIndex);
					if (pSrcPhrase == NULL)
					{
						// we got to the end of the doc
						gpApp->m_curIndex = gpApp->m_endIndex = gpApp->m_maxIndex;
						gpApp->m_nActiveSequNum = -1;
						goto n;
					}
					else
					{
						// we found an empty one, so remove the non-success 
						// condition (note: gbBundleChanged will still be TRUE)
						bSuccessful = TRUE;
						gpApp->m_nActiveSequNum = pSrcPhrase->m_nSequNumber;
						pView->Jump(gpApp,pSrcPhrase);
						//int dummy = 1;
						goto m;
					}
				}

				// if we get here, we should be at the end
n:			 	if ((gpApp->m_curIndex == gpApp->m_endIndex && 
					gpApp->m_endIndex == gpApp->m_maxIndex)
					|| gpApp->m_nActiveSequNum == -1)
				{
					// At the end, we'll reset the globals to turn off any highlight
					gnBeginInsertionsSequNum = -1;
					gnEndInsertionsSequNum = -1;
					pView->Invalidate(); // remove highlight before MessageBox call below

					// tell the user EOF has been reached
					// IDS_AT_END
					wxMessageBox(_("The end. Provided you have not missed anything earlier, there is nothing more to adapt in this file."),_T(""), wxICON_INFORMATION);
					wxStatusBar* pStatusBar;
					CMainFrame* pFrame = gpApp->GetMainFrame();
					if (pFrame != NULL)
					{
						pStatusBar = pFrame->GetStatusBar();
						wxString str = _("End of the file; nothing more to adapt.");
						pStatusBar->SetStatusText(str,0); // use first field 0
					}
					// we are at EOF, so set up safe end conditions
					gpApp->m_targetPhrase.Empty();
					gpApp->m_nActiveSequNum = gpApp->m_curIndex = -1;
					gpApp->m_pTargetBox->Hide();
					if (!gbBundleChanged)
						pView->LayoutStrip(gpApp->m_pSourcePhrases,
												nOldStripIndex,gpApp->m_pBundle);
					gpApp->m_pActivePile = NULL; // can use this as a flag for 
													// at-EOF condition too
					pView->Invalidate();
				}
				else
				{
					gpApp->m_pTargetBox->SetFocus();
				}
				translation.Empty(); // clear the static string storage for the 
										// translation
				// save the phrase box's text, in case user hits SHIFT+END to 
				// unmerge a phrase
				gSaveTargetPhrase = gpApp->m_targetPhrase;
				return; // must have been a null string, or at EOF;
			}

m:			gpApp->GetMainFrame()->canvas->ScrollIntoView(gpApp->m_nActiveSequNum);
			int nCurStripIndex = gpApp->m_pActivePile->m_pStrip->m_nStripIndex;

			// if the old strip is not same as the new one, relayout the old one 
			// too, if bundle is the same
			if (nCurStripIndex != nOldStripIndex && !gbBundleChanged)
			{
				pView->RecalcLayout(gpApp->m_pSourcePhrases,0,gpApp->m_pBundle);

				// recalculate the active pile pointer, as the old one was clobbered
				// by doing the new layout
				gpApp->m_pActivePile = pView->GetPile(gpApp->m_nActiveSequNum);
				wxASSERT(gpApp->m_pActivePile != NULL);
				m_pActivePile = gpApp->m_pActivePile;
				gpApp->m_ptCurBoxLocation = m_pActivePile->m_pCell[2]->m_ptTopLeft;
				gpApp->GetMainFrame()->canvas->ScrollIntoView(gpApp->m_nActiveSequNum);
				pView->Invalidate();
			}

			// recreate the phraseBox again - whether or not we have scrolled, we 
			// must do it again after a recalc of the layout, because if a pile 
			// moves back up to end of previous strip in this recalculation and we 
			// don't redo the box, the box will end up too far to the right, so the 
			// remake call has to be unconditional; also make sure the box location 
			// is uptodate
			pView->RemakePhraseBox(gpApp->m_pActivePile,gpApp->m_targetPhrase);

			if (!bSuccessful)
			{

				// At the end, we'll reset the globals to turn off any highlight
				gnBeginInsertionsSequNum = -1;
				gnEndInsertionsSequNum = -1;
				pView->Invalidate(); // remove highlight before MessageBox call below

				// we have come to the end of the bundle, probably to the end of the
				// file's data, so we must here determine if we can advance the
				// bundle - and do it if we can (and place the phrase box in its 
				// first pile), else we are at the end of the data and can't move 
				// further so just tell the user there is no more data to adapt in
				// this file.
				if (gpApp->m_endIndex == gpApp->m_maxIndex)
				{
					// tell the user EOF has been reached
					// IDS_AT_END
					wxMessageBox(_("The end. Provided you have not missed anything earlier, there is nothing more to adapt in this file."), _T(""), wxICON_INFORMATION);
					wxStatusBar* pStatusBar;
					CMainFrame* pFrame = gpApp->GetMainFrame();
					if (pFrame != NULL)
					{
						pStatusBar = pFrame->GetStatusBar();
						wxString str = _("End of the file; nothing more to adapt.");
						pStatusBar->SetStatusText(str,0); // use first field 0
					}
					// we are at the end of the file's data, so remove the phraseBox
					// and set the sequence number and m_curIndex to -1, as flags 
					// for this at-EOF condition
					gpApp->m_targetPhrase.Empty();
					gpApp->m_nActiveSequNum = gpApp->m_curIndex = -1;
					gpApp->m_pTargetBox->Hide();
					if (!gbBundleChanged)
						pView->LayoutStrip(gpApp->m_pSourcePhrases,
												nOldStripIndex,gpApp->m_pBundle);
					gpApp->m_pActivePile = NULL; // can use this as a flag for 
													// at-EOF condition too
					pView->Invalidate();
				}
			}

			// try to keep the phrase box from coming too close to the bottom of 
			// the client area if possible
			int yDist = gpApp->m_curPileHeight + gpApp->m_curLeading;
			wxPoint scrollPos;
			int xPixelsPerUnit,yPixelsPerUnit;
			gpApp->GetMainFrame()->canvas->GetScrollPixelsPerUnit(&xPixelsPerUnit,&yPixelsPerUnit);
			
			// MFC's GetScrollPosition() "gets the location in the document to which the upper
			// left corner of the view has been scrolled. It returns values in logical units."
			// wx note: The wx docs only say of GetScrollPos(), that it "Returns the built-in scrollbar position."
			// I assume this means it gets the logical position of the upper left corner, but it is in scroll 
			// units which need to be converted to device (pixel) units
			//scrollPos.x = gpApp->GetMainFrame()->canvas->GetScrollPos(wxHORIZONTAL); //wxPoint scrollPos = GetScrollPosition();
			//scrollPos.y = gpApp->GetMainFrame()->canvas->GetScrollPos(wxVERTICAL); //wxPoint scrollPos = GetScrollPosition();
			//scrollPos.x *= xPixelsPerUnit; wxASSERT(scrollPos.x == 0);
			//scrollPos.y *= yPixelsPerUnit;
			
//#ifdef _DEBUG
//			int xOrigin, yOrigin;
//			// the view start is effectively the scroll position, but GetViewStart returns scroll units
//			gpApp->GetMainFrame()->canvas->GetViewStart(&xOrigin, &yOrigin); // gets xOrigin and yOrigin in scroll units
//			xOrigin = xOrigin * xPixelsPerUnit; // number pixels is ScrollUnits * pixelsPerScrollUnit
//			yOrigin = yOrigin * yPixelsPerUnit;
//			wxASSERT(xOrigin == scrollPos.x);
//			wxASSERT(yOrigin == scrollPos.y);
//#endif


			//CSize barSizes = pView->GetTotalSize();
			// MFC's GetTotalSize() gets "the total size of the scroll view in logical
			// units." WxWidgets has GetVirtualSize() which gets the size in device units (pixels).
			wxSize maxDocSize; // renamed barSizes to maxDocSize for clarity
			gpApp->GetMainFrame()->canvas->GetVirtualSize(&maxDocSize.x,&maxDocSize.y); // gets size in pixels

			// Can't use dc.DeviceToLogicalX below because OnPrepareDC is not called
			//wxClientDC dc(gpApp->GetMainFrame()->canvas);
			//dc.DeviceToLogicalX(scrViewSizeX); // now logical coords
			//dc.DeviceToLogicalY(scrViewSizeY);

//#ifdef _DEBUG
//			int newXPos,newYPos;
//			gpApp->GetMainFrame()->canvas->CalcUnscrolledPosition(0,0,&newXPos,&newYPos);
//			wxASSERT(newXPos == scrollPos.x); // scrollPos.x = newXPos;
//			wxASSERT(newYPos == scrollPos.y); // scrollPos.y = newYPos;
//#endif

			gpApp->GetMainFrame()->canvas->CalcUnscrolledPosition(0,0,&scrollPos.x,&scrollPos.y);
			// the scrollPos point is now in logical pixels from the start of the doc
			
			wxRect rectClient(0,0,0,0);
			wxSize canvasSize;
			canvasSize = gpApp->GetMainFrame()->GetCanvasClientSize(); // with GetClientRect upper left coord is always (0,0)
			rectClient.width = canvasSize.x;
			rectClient.height = canvasSize.y;
			
			
			if (rectClient.GetBottom() >= 4 * yDist) // do the adjust only if at least 
													// 4 strips are showing
			{
				wxPoint pt = gpApp->m_ptCurBoxLocation; // logical coords of top of phrase box

				// if there are not two full strips below the top of the phrase box,
				// scroll down
				wxASSERT(scrollPos.y + rectClient.GetBottom() >= 2*yDist);
				if (pt.y > scrollPos.y + rectClient.GetBottom() - 2*yDist)
				{
					int logicalViewBottom = scrollPos.y + rectClient.GetBottom();
																	// is always 0
					if (logicalViewBottom < maxDocSize.GetHeight())
					{
						if (logicalViewBottom <= maxDocSize.GetHeight() - yDist)
						{
							// a full strip + leading can be scrolled safely
							gpApp->GetMainFrame()->canvas->ScrollDown(1);
							// what ScrollDown does could be done with the following 5 lines:
							//scrollPos.y += yDist; // yDist is here exactly one strip (including leading)
							//int posn = scrollPos.y;
							//posn = posn / yPixelsPerUnit;
							//gpApp->GetMainFrame()->canvas->Scroll(0,posn);
							//gpApp->GetMainFrame()->canvas->Refresh(); // ScrollDown(1) calls Refresh - needed?
						}
						else
						{
							// we are close to the end, but not a full strip + 
							// leading can be scrolled, so just scroll enough to 
							// reach the end - otherwise position of phrase box will
							// be set wrongly
							wxASSERT(maxDocSize.GetHeight() >= logicalViewBottom);
							yDist = maxDocSize.GetHeight() - logicalViewBottom;
							scrollPos.y += yDist;

							int posn = scrollPos.y;
							posn = posn / yPixelsPerUnit;
							//gpApp->GetMainFrame()->canvas->SetScrollPos(wxVERTICAL,posn,TRUE); //pView->SetScrollPos(SB_VERT,scrollPos.y,TRUE);
							// Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount
							// to scroll in device units (pixels). The equivalent in wx is Scroll(x,y) in which x and y are in
							// SCROLL UNITS (pixels divided by pixels per unit). Also MFC's ScrollWindow takes parameters whose value
							// represents an "amount" to scroll from the current position, whereas the wxScrolledWindow::Scroll
							// takes parameters which represent an absolute "position" in scroll units. To convert the
							// amount we need to add the amount to (or subtract from if negative) the logical pixel unit
							// of the upper left point of the client viewing area; then convert to scroll units in Scroll().
							gpApp->GetMainFrame()->canvas->Scroll(0,posn); //pView->ScrollWindow(0,-yDist);
							Refresh(); 
						}
					}
				}
			}
		}
		else
		{
			// cause auto-inserting using the OnIdle handler to commence
			gpApp->m_bAutoInsert = TRUE;
			gbEnterTyped = TRUE;

			// User has pressed the Enter key so set the globals to the
			// active sequence number plus one, so we can track any 
			// automatically inserted target/gloss text
			gnBeginInsertionsSequNum = gpApp->m_nActiveSequNum + 1;
			// BEW modified 20June06; the starting value for gnEndInsertionsSequNum
			// must be one less than gnBeginInsertionsSequNum so that at the first
			// auto insert location the incrementation of gnEndInsertionsSequNum will
			// become equal in value to gnBeginInsertionsSequNum, and ony for a second
			// and subsequent insertions will the end one become greater than the
			// beginning one. This fixes an error which resulted in the pile AFTER
			// the halted phrase box having background highlighting even though the
			// phrase box has not yet got that far (because gnEndInsertionsSequNum
			// was set one too large)
			gnEndInsertionsSequNum = gnBeginInsertionsSequNum - 1;
		}
		// save the phrase box's text, in case user hits SHIFT+End to unmerge a
		// phrase
		gSaveTargetPhrase = gpApp->m_targetPhrase;
	} // end if for m_bDrafting
	else
	{
		// we are in Review mode, so moves by the RETURN key can only be to 
		// immediate next pile
		int nOldStripIndex = gpApp->m_pActivePile->m_pStrip->m_nStripIndex;

		// we have to check here if we have moved into the area where it is 
		// necessary to move the bundle down, and if so, then do it, & update 
		// everything again
		bool bNeedAdvance = pView->NeedBundleAdvance(gpApp->m_nActiveSequNum);
		if (bNeedAdvance)
		{
			// do the advance, return a new (valid) pointer to the active pile
			gpApp->m_pActivePile = pView->AdvanceBundle(gpApp->m_nActiveSequNum);
			m_pActivePile = gpApp->m_pActivePile; // put copy on the CPhraseBox too
		}

		int bSuccessful = MoveToImmedNextPile(pView,m_pActivePile);
		if (!bSuccessful)
		{
			if (gbUserWantsNoMove)
			{
				gbUserWantsNoMove = FALSE;
			}
			else
			{
				if ((gpApp->m_curIndex == gpApp->m_endIndex && 
							gpApp->m_endIndex == gpApp->m_maxIndex)
							|| gpApp->m_pActivePile == NULL 
							|| gpApp->m_nActiveSequNum == -1)
				{
					// tell the user EOF has been reached
					// IDS_AT_END
					wxMessageBox(_("The end. Provided you have not missed anything earlier, there is nothing more to adapt in this file."), _T(""), wxICON_INFORMATION);
					wxStatusBar* pStatusBar;
					CMainFrame* pFrame = gpApp->GetMainFrame();
					if (pFrame != NULL)
					{
						pStatusBar = pFrame->GetStatusBar();
						wxString str = _("End of the file; nothing more to adapt.");
						pStatusBar->SetStatusText(str,0); // use first field 0
					}
					// we are at EOF, so set up safe end conditions
					gpApp->m_pTargetBox->Hide(); // whm added 12Sep04
					gpApp->m_pTargetBox->Enable(FALSE); // whm added 12Sep04
					gpApp->m_targetPhrase.Empty();
					gpApp->m_nActiveSequNum = gpApp->m_curIndex = -1;
					pView->LayoutStrip(gpApp->m_pSourcePhrases,
											nOldStripIndex,gpApp->m_pBundle);
					gpApp->m_pActivePile = NULL; // can use this as a flag for 
													// at-EOF condition too
					pView->Invalidate();
				}
				else
				{
					// IDS_CANNOT_GO_FORWARD
					wxMessageBox(_("Sorry, the next pile cannot be a valid active location, so no move forward was done."), _T(""), wxICON_INFORMATION);
					gpApp->m_pTargetBox->SetFocus();
				}
			}
			translation.Empty(); // clear the static string storage for the 
									// translation
			// save the phrase box's text, in case user hits SHIFT+END to unmerge 
			// a phrase
			gSaveTargetPhrase = gpApp->m_targetPhrase;
			return;
		}
		else
		{
			// it was successful
			CCell* pCell = gpApp->m_pActivePile->m_pCell[2]; // the cell where the
																// phraseBox is to be
			//pView->ReDoPhraseBox(pCell); // like PlacePhraseBox, but calculations 
											// based on m_targetPhrase
			// BEW commented out above call 19Dec07, in favour of RemakePhraseBox() call below.
			// Also added test for whether document at new active location has a hole there or
			// not; if it has, we won't permit a copy of the source text to fill the hole, as
			// that would be inappropriate in Reviewing mode; since m_targetPhrase already has
			// the box text or the copied source text, we must instead check the CSourcePhrase
			// instance explicitly to see if m_adaption is empty, and if so, then we force
			// the phrase box to remain empty by clearing m_targetPhrase (later, when the
			// box is moved to the next location, we must check again in MakeLineFourString() and
			// restore the earlier state when the phrase box is moved on)					
			CSourcePhrase* pSPhr = pCell->m_pPile->m_pSrcPhrase;
			wxASSERT(pSPhr != NULL);
			if (pSPhr->m_targetStr.IsEmpty() || pSPhr->m_adaption.IsEmpty())
			{
				// no text or punctuation, or no text and punctuation not yet placed,
				// or no text and punctuation was earlier placed -- whichever is the case
				// we need to preserve that state
				gpApp->m_targetPhrase.Empty();
				gbSavedLineFourInReviewingMode = TRUE; // it gets cleared again at end of MakeLineFourString()
				gStrSaveLineFourInReviewingMode = pSPhr->m_targetStr; // cleared at end of MakeLineFourString()
			}
			// if neither test succeeds, then let m_targetPhrase contents stand unchanged

			gbEnterTyped = FALSE;
			// reset the gLastSrcPhrasePos value
			/* //BEW removed 31Jan08 because value can't be relied on in all circumstances, FindIndex()
			   // is now used universally for finding a POSITION value from a given sequ number value
			SPList::Node* posTemp = gLastSrcPhrasePos;
			if (posTemp != NULL)
			{
				posTemp = posTemp->GetNext();
				if (posTemp != NULL)
					gLastSrcPhrasePos = posTemp;
			}
			*/

			// BEW added 19Dec07: force immediate draw of window (an UpdateWindow() call doesn't
			// do it for us, so just do a RemakePhraseBox() call here, as for drafting mode case
			// (this addition is needed because if app is launched, doc opened in drafting mode,
			// phrase box placed by a click at the pile before a hole and then Enter pressed, the
			// view is recalculated and drawn and the box created, but it remains hidden. The addition
			// of the RemakePhraseBox() call gets it shown. I remember having to do this years ago
			// for the Drafting mode block above. Why the UpdateWindow() call won't do it is a mystery.
			//pView->m_targetBox.UpdateWindow();
			pView->RemakePhraseBox(gpApp->m_pActivePile,gpApp->m_targetPhrase);
		}
		gpApp->GetMainFrame()->canvas->ScrollIntoView(gpApp->m_nActiveSequNum);

		// save the phrase box's text, in case user hits SHIFT+END to unmerge a phrase
		gSaveTargetPhrase = gpApp->m_targetPhrase;

		/* whm Note: The following kluge is not needed for the wx version:
		// BEW added 21Dec07: get everything redrawn (invalidating the view and phrase box here does
		// not redraw the phrase box when using the Enter key in Reviewing mode to step the
		// phrase box through consecutive holes(but it is created in its correct position each time
		// and each time has the input focus, it just can't be seen!); but for an unknown reason 
		// destroying it and recreating it within a single function works. Since RedrawEverything()
		// already does these things plus calls Invalidate() on the view, it fixes the glitch as well
		// as handling all that the older code handled. Another kluge for an incomprehensible reason.
		//pView->Invalidate();
		//pView->m_targetBox.Invalidate();
		pView->RedrawEverything(pView->m_nActiveSequNum);
		*/

	} // end Review mode (single src phrase move) block
}

void CPhraseBox::OnPhraseBoxChanged(wxCommandEvent& WXUNUSED(event))
{
	// whm Note: This phrasebox handler is necessary in the wxWidgets version because the
	// OnChar() handler does not have access to the changed value of the new string
	// within the control reflecting the keystroke that triggers OnChar(). Because
	// of that difference in behavior, I moved the code dependent on updating
	// gpApp->m_targetPhrase from OnChar() to this OnPhraseBoxChanged() handler. 
	if (this->IsModified())
	{
		CAdapt_ItView* pView = (CAdapt_ItView*) gpApp->GetView();
		// preserve cursor location, in case we merge, so we can restore it afterwards
		long nStartChar;
		long nEndChar;
		GetSelection(&nStartChar,&nEndChar);

		wxPoint ptNew;
		wxRect rectClient;
		wxSize textExtent;
		wxString thePhrase; // moved to OnPhraseBoxChanged()

		// update status bar with project name
		gpApp->RefreshStatusBarInfo();

		// restore the cursor position...
		// BEW added 05Oct06; to support the alternative algorithm for setting up
		// the phrase box text, and positioning the cursor, when the selection was
		// done leftwards from the active location... that is, since the cursor
		// position for leftwards selection merges is determined within OnButtonMerge()
		// then we don't here want to let the values stored at the start of OnChar()
		// clobber what OnButtonMerge() has already done - so we have a test to determine
		// when to suppress the cursor setting call below in this new circumstance
		if (!(gbMergeSucceeded && gpApp->m_curDirection == left))
		{
			SetSelection(nStartChar,nEndChar);
			gnStart = nStartChar;
			gnEnd = nEndChar;
		}

		// ensure the phraseBox's pointer to the active pile is in synch with that on the view
		m_pActivePile = gpApp->m_pActivePile;

		// always check text extent to see if box needs resizing
		wxSize currBoxSize(gpApp->m_curBoxWidth,gpApp->m_nTgtHeight);

		// whm Note: Because of differences in the handling of events, in wxWidgets the GetValue() 
		// call below retrieves the contents of the phrasebox as it existed before the keystroke 
		// that triggered this present OnChar() event handler. Hence, the text returned via GetValue()
		// is not updated to reflect the current keystroke until AFTER the pesent OnChar() call.
		// This is in contrast to the MFC version where GetWindowText(thePhrase) at the same
		// code location in PhraseBox::OnChar there gets the contents of the phrasebox including the 
		// just typed character.
		thePhrase = GetValue(); // current box text

		// update m_targetPhrase to agree with what has been typed so far
		gpApp->m_targetPhrase = thePhrase;
		
		bool bWasMadeDirty = FALSE;

		// whm Note: here we can eliminate the test for Return, BackSpace and Tab
		gpApp->m_bUserTypedSomething = TRUE;
		pView->GetDocument()->Modify(TRUE);
		gpApp->m_pTargetBox->m_bAbandonable = FALSE; // once we type something, it's not 
												// considered abandonable
		gbByCopyOnly = FALSE; // even if copied, typing something makes it different so set 
							// this flag FALSE
		MarkDirty();

		// adjust box size
		FixBox(pView,thePhrase,bWasMadeDirty,textExtent,0); // selector = 0 for incrementing 
															// box extent

		// set the globals for the cursor location
		GetSelection(&gnStart,&gnEnd);

		// save the phrase box's text, in case user hits SHIFT+END to unmerge a phrase
		gSaveTargetPhrase = gpApp->m_targetPhrase;

	}
}

// MFC docs say about CWnd::OnChar "The framework calls this member function when
// a keystroke translates to a nonsystem character. This function is called before
// the OnKeyUp member function and after the OnKeyDown member function are called.
// OnChar contains the value of the keyboard key being pressed or released. This 
// member function is called by the framework to allow your application to handle 
// a Windows message. The parameters passed to your function reflect the parameters 
// received by the framework when the message was received. If you call the 
// base-class implementation of this function, that implementation will use the 
// parameters originally passed with the message and not the parameters you supply 
// to the function."
// Hence the calling order in MFC is OnKeyDown, OnChar, OnKeyUp.
// The wxWidgets docs say about wxKeyEvent "Notice that there are three different 
// kinds of keyboard events in wxWidgets: key down and up events and char events. 
// The difference between the first two is clear - the first corresponds to a key 
// press and the second to a key release - otherwise they are identical. Just note 
// that if the key is maintained in a pressed state you will typically get a lot 
// of (automatically generated) down events but only one up so it is wrong to 
// assume that there is one up event corresponding to each down one. Both key 
// events provide untranslated key codes while the char event carries the 
// translated one. The untranslated code for alphanumeric keys is always an upper 
// case value. For the other keys it is one of WXK_XXX values from the keycodes 
// table. The translated key is, in general, the character the user expects to 
// appear as the result of the key combination when typing the text into a text 
// entry zone, for example.

// OnChar is called via EVT_CHAR(OnChar) in our CPhraseBox Event Table.
void CPhraseBox::OnChar(wxKeyEvent& event)
{
	// whm Note: OnChar() is called before OnPhraseBoxChanged()

	//wxLogDebug(_T("OnChar() %d called from PhraseBox"),event.GetKeyCode());
	// wx version note: In MFC this OnChar() function is NOT called for system key events,
	// however in the wx version it IS called for combination system key events such as
	// ALT+Arrow-key. We will immediately return if the CTRL or ALT key is down; Under 
	// wx's wxKeyEvent we can test CTRL or ALT down with HasModifiers(), and we must also
	// prevent any arrow keys.
	if (event.HasModifiers() || event.GetKeyCode() == WXK_DOWN || event.GetKeyCode() == WXK_UP
		|| event.GetKeyCode() == WXK_LEFT || event.GetKeyCode() == WXK_RIGHT)
	{
		event.Skip(); // to allow OnKeyUp() to handle the event
		return;
	}

	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	m_bMergeWasDone = FALSE; //bool bMergeWasDone = FALSE;
	gbEnterTyped = FALSE;
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));

	// whm Note: The following code for handling the WXK_BACK key is ok to leave here in
	// the OnChar() handler, because it is placed before the Skip() call (the OnChar() base
	// class call in MFC)

	GetSelection(&gnSaveStart,&gnSaveEnd);

	// CEdit's Undo() function does not undo a backspace deletion of a selection or single 
	// char, so implement that here & in an override for OnEditUndo();
	if (event.GetKeyCode() == WXK_BACK)
	{
		m_backspaceUndoStr.Empty();
		wxString s;
		s = GetValue();
		if (gnSaveStart == gnSaveEnd)
		{
			// deleting previous character
			if (gnSaveStart > 0)
			{
				int index = gnSaveStart;
				index--;
				gnSaveStart = index;
				gnSaveEnd = index;
				wxChar ch = s.GetChar(index);
				m_backspaceUndoStr += ch;
			}
			else // gnSaveStart must be zero, as when the box is empty
			{
				// BEW added 20June06, because if the CSourcePhrase has m_bHasKBEntry TRUE, but
				// an empty m_adaption (because the user specified <no adaptation> there earlier)
				// then there was no way in earlier versions to "remove" the <no adaptation> - so we
				// want to allow a backspace keypress, with the box empty, to effect the clearing of
				// m_bHasKBEntry to FALSE (or if glossing, m_bHasGlossingKBEntry to FALSE), and to
				// decement the <no adaptation> ref count or remove the entry entirely if the count is 1
				// from the KB entry
				if (gpApp->m_pActivePile->m_pSrcPhrase->m_adaption.IsEmpty() &&
					((gpApp->m_pActivePile->m_pSrcPhrase->m_bHasKBEntry && !gbIsGlossing) ||
					(gpApp->m_pActivePile->m_pSrcPhrase->m_bHasGlossingKBEntry && gbIsGlossing)))
				{
					gbNoAdaptationRemovalRequested = TRUE;
					CKB* pKB = pView->GetKB();
					CSourcePhrase* pSrcPhrase = gpApp->m_pActivePile->m_pSrcPhrase;
					CRefString* pRefString = pView->GetRefString(pKB, pSrcPhrase->m_nSrcWords,
						pSrcPhrase->m_key,pSrcPhrase->m_adaption);
					if (pRefString != NULL)
					{
						pView->RemoveRefString(pRefString,pSrcPhrase,pSrcPhrase->m_nSrcWords);
					}
				}
			}
		}
		else
		{
			// deleting a selection
			int count = gnSaveEnd - gnSaveStart;
			if (count > 0)
			{
				m_backspaceUndoStr = s.Mid(gnSaveStart,count);
				gnSaveEnd = gnSaveStart;
			}
		}
	}

	gbExpanding = FALSE;

	// wxWidgets Note: The wxTextCtrl does not have a virtual OnChar() method, 
	// so we'll just skip any special handling of the WXK_RETURN and WXK_TAB 
	// key events. In wxWidgets, calling event.Skip() is analagous to calling 
	// the base class version of a virtual function. Note: wxTextCtrl has
	// a non-virtual OnChar() method. See "wxTextCtrl OnChar event handling.txt"
	// for a newsgroup sample describing how to use OnChar() to do "auto-
	// completion" while a user types text into a text ctrl.
	if (!(event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_TAB)) // I don't want a bell when RET key or HT key typed
	{
		// whm note: Instead of explicitly calling the OnChar() function in the base class (as would 
		// normally be done for C++ virtual functions), in wxWidgets, we call Skip() instead, for the 
		// event to be processed either in the base wxWidgets class or the native control.
		// This difference in event handling also means that part of the code in OnChar() that can
		// be handled correctly in the MFC version, must be moved to our wx-specific OnPhraseBoxChanged()
		// handler. This is necessitated because GetValue() does not get the new value of the control's
		// contents when called from within OnChar, but received the previous value of the string as it
		// existed before the keystroke that triggers OnChar.
		event.Skip(); // CEdit::OnChar(nChar, nRepCnt, nFlags);
	}

	// whm Note: The original MFC code below has moved to the OnPhraseBoxChanged() handler

	wxSize textExtent;
	// if there is a selection, and user forgets to make the phrase before typing, then do it
	// for him on the first character typed. But if glossing, no merges are allowed.

	// ??? Weird. I had to remove pApp->m_selection.GetCount() out of the if (...) test because
	// the code for version 2.3.0 was unaccountably failing because CPtrList could not find the 
	// GetCount() member when compiled that way; for some unknown reason, just setting up the 
	// int theCount local variable and setting it and using theCount in the test instead makes 
	// this error no longer happen. (Previously to this, the older code has worked fine for years!
	// See also the CAdapt_ItApp( ) creator's code block, where I had to put some dummy code 
	// involving CPtrList to avoid a similar error when I click the view's close box.

	int theCount = pApp->m_selection.GetCount();
	if (!gbIsGlossing && theCount > 1 && (pApp->m_pActivePile == pApp->m_pAnchor->m_pPile
		|| IsActiveLocWithinSelection(pView,pApp->m_pActivePile)))
	{
		if (pView->GetSelectionWordCount() > MAX_WORDS)
		{
			pView->DoRetranslation();
		}
		else
		{
			if (!pApp->m_bUserTypedSomething && 
				!pApp->m_pActivePile->m_pSrcPhrase->m_targetStr.IsEmpty())
			{
				bSuppressDefaultAdaptation = FALSE; // we want what is already there
			}
			else
			{
				// for version 1.4.2 and onwards, we will want to check gbRetainBoxContents
				// and two other flags, for a click or arrow key press is meant to allow
				// the deselected source word already in the phrasebox to be retained; so we
				// here want the bSuppressDefaultAdaptation flag set TRUE only when the
				// gbRetainBoxContents is FALSE (- though we use two other flags too to
				// ensure we get this behaviour only when we want it)
				if (gbRetainBoxContents && !m_bAbandonable && pApp->m_bUserTypedSomething)
				{
					bSuppressDefaultAdaptation = FALSE;
				}
				else
				{
					bSuppressDefaultAdaptation = TRUE; // the global BOOLEAN used for temporary 
												// suppression only
				}
			}
			pView->MergeWords(); // simply calls OnButtonMerge
			m_bMergeWasDone = TRUE;
			bSuppressDefaultAdaptation = FALSE;

			// we can assume what the user typed, provided it is a letter, replaces what was 
			// merged together, but if tab or return was typed, we allow the merged text to 
			// remain intact & pass the character on to the switch below; but since v1.4.2 we
			// can only assume this when gbRetainBoxContents is FALSE, if it is TRUE and 
			// a merge was done, then there is probably more than what was just typed, so we
			// retain that instead; also, we we have just returned from a MergeWords( ) call in
			// which the phrasebox has been set up with correct text (such as previous target text
			// plus the last character typed for an extra merge when a selection was present, we
			// don't want this wiped out and have only the last character typed inserted instead,
			// so in OnButtonMerge( ) we test the phrasebox's string and if it has more than just
			// the last character typed, we assume we want to keep the other content - and so there
			// we also set gbRetainBoxContents
			if (gbRetainBoxContents && m_bMergeWasDone)
			{
				; // do nothing - note, exiting via here leaves the cursor after whatever
				// character the user typed, so if that was a hyphen then the cursor will
				// be preceding the concatenating space which usually the user will then
				// want to delete; so leave the cursor location unchanged as this 
				// fortuitous location is precisely where we would want to be
			}
			else
			{
				if (event.GetKeyCode() != WXK_TAB 
					&& event.GetKeyCode() != WXK_RETURN 
					&& event.GetKeyCode() != WXK_BACK)
				{
					long keycode = event.GetKeyCode();
					wxString key;
					key.Printf(_T("'%c'"),(unsigned char)keycode);
					SetValue(key);
				}
			}
			gbRetainBoxContents = FALSE; // turn it back off (default) until next required
		}
	}
	else
	{
		// if there is a selection, but the anchor is removed from the active location, we don't
		// want to make a phrase elsewhere, so just remove the selection.
		// Or if glossing, just silently remove the selection - that should be sufficient alert
		// to the user that the merge operation is unavailable
		pView->RemoveSelection();
		wxClientDC dC(pApp->GetMainFrame()->canvas);
		pView->canvas->DoPrepareDC(dC); // adjust origin
		gpApp->GetMainFrame()->PrepareDC(dC); // wxWidgets' drawing.cpp sample also calls PrepareDC on the owning frame
		pView->Invalidate();
	}

	long keycode = event.GetKeyCode();
	switch(keycode)
	{
	case WXK_RETURN: //13:	// RETURN key
		{
			// save old sequ number in case required for toolbar's Back button
			gnOldSequNum = pApp->m_nActiveSequNum;

			if (wxGetKeyState(WXK_SHIFT))
			{
				gbBundleChanged = FALSE; // default

				// shift key is down, so move back a pile

				// we have to check here if we have moved into the area where it is necessary 
				// to move the bundle back up, and if so, then do it, & update everything again
				bool bRetreat = pView->NeedBundleRetreat(pApp->m_nActiveSequNum);
				if (bRetreat)
				{
					// do the retreat, return a new (valid) pointer to the active pile
					pApp->m_pActivePile = pView->RetreatBundle(pApp->m_nActiveSequNum);
					m_pActivePile = pApp->m_pActivePile; // put copy on the CPhraseBox too
				}

				int bSuccessful = MoveToPrevPile(pView,pApp->m_pActivePile);
				if (!bSuccessful)
				{
					// we have come to the start of the bundle, so do nothing
					gbBundleChanged = TRUE;
				}
				else
				{
					// it was successful
					CCell* pCell = pApp->m_pActivePile->m_pCell[2]; // the cell where the 
																	 // phraseBox is to be
					pView->ReDoPhraseBox(pCell); // like PlacePhraseBox, but calculations 
												 // based on m_targetPhrase						
					// reset the gLastSrcPhrasePos value
					/* BEW removed 31Jan08, because the value cannot be relied on in all circumstances
					gbEnterTyped = FALSE;
					SPList::Node* posTemp = gLastSrcPhrasePos;
					if (posTemp != NULL)
					{
						CSourcePhrase* pSrcPhrase;
						pSrcPhrase = (CSourcePhrase*)posTemp->GetData();
						posTemp = posTemp->GetPrevious();
						if (posTemp != NULL)
							gLastSrcPhrasePos = posTemp;
					}
					*/
				}

				gpApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);

				// save the phrase box's text, in case user hits SHIFT+End to unmerge a phrase
				gSaveTargetPhrase = pApp->m_targetPhrase;
				return;
			} // end keyState < 0 block
			else // we are moving forwards rather than backwards
			{
z:				JumpForward(pView);
			} // end keyState >= 0 block
		} // end case 13: block
		return;
	case WXK_TAB: //9:		// TAB key - use this for going one location forwards or back
		{
			// save old sequ number in case required for toolbar's Back button
			gnOldSequNum = pApp->m_nActiveSequNum;

			// SHIFT+TAB is the 'universal' keyboard way to cause a move back, so implement it
			if (wxGetKeyState(WXK_SHIFT))
			{
				// shift key is down, so move back a pile

				// Shift+Tab (reverse direction) indicates user is probably
				// backing up to correct something that was perhaps automatically
				// inserted, so we will preserve any highlighting and do nothing
				// here in response to Shift+Tab.

				Freeze();

				// we have to check here if we have moved into the area where it is necessary to
				// move the bundle back up, and if so, then do it, & update everything again
				bool bRetreat = pView->NeedBundleRetreat(pApp->m_nActiveSequNum);
				if (bRetreat)
				{
					// do the retreat, return a new (valid) pointer to the active pile
					pApp->m_pActivePile = pView->RetreatBundle(pApp->m_nActiveSequNum);
					m_pActivePile = pApp->m_pActivePile; // put copy on the CPhraseBox too
				}

				int bSuccessful = MoveToPrevPile(pView,pApp->m_pActivePile);
				if (!bSuccessful)
				{
					// we have come to the start of the bundle, so do nothing
					;
				}
				else
				{
					// it was successful
					CCell* pCell = pApp->m_pActivePile->m_pCell[2]; // the cell where the 
																	 // phraseBox is to be

					// if moving back we don't want any MakeLineFourString( ) call to be made, because
					// if that was a merged phrase with internal punctuation, we'd see the Place Punctuation
					// dialog again, which we don't want. Ditto for next call about 20 lines below. So
					// if gbMovingToPreviousPile is set, the call won't be made
					pView->ReDoPhraseBox(pCell); // like PlacePhraseBox, but calculations based 
												 // on m_targetPhrase

					// reset the gLastSrcPhrasePos value
					gbEnterTyped = FALSE;
					/* BEW removed 31Jan05 because value cannot always be relied upon
					SPList::Node* posTemp = gLastSrcPhrasePos;
					if (posTemp != NULL)
					{
						CSourcePhrase* pSrcPhrase;
						pSrcPhrase = (CSourcePhrase*)posTemp->GetData();
						posTemp = posTemp->GetPrevious();
						if (posTemp != NULL)
							gLastSrcPhrasePos = posTemp;
					}
					*/
				}

				// scroll, if necessary
				gpApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);

				// recreate the phraseBox again (this is a kludge, as the preceding scroll 
				// sometimes leaves the box offset by how many strips the extra scroll 
				// adjustment within the ScrollIntoView code moves the box, so redoing the 
				// calculation after the scroll fixes the problem -- a bit inelegant, but it works)
				CCell* pCell = pApp->m_pActivePile->m_pCell[2]; // the cell of the phraseBox
				pView->ReDoPhraseBox(pCell);

				// okay, now we can remove the suppression of the MakeLineFourString( ) call.
				gbMovingToPreviousPile = FALSE;

				// save the phrase box's text, in case user hits SHIFT+END key to unmerge 
				// a phrase
				gSaveTargetPhrase = pApp->m_targetPhrase;
				Thaw();
				return;
			}
			else
			{
				//BEW changed 01Aug05. Some users are familiar with using TAB key to advance
				// (especially when working with databases), and without thinking do so in Adapt It
				// and expect the Lookup process to take place, etc - and then get quite disturbed
				// when it doesn't happen that way. So for version 3 and onwards, we will interpret
				// a TAB keypress as if it was an ENTER keypress
				goto z;

				/* Code for 2.4.1g and earlier versions

				// jump to immediate next pile, for an unshifted tab press
					int nOldStripIndex = pApp->m_pActivePile->m_pStrip->m_nStripIndex;

				// we have to check here if we have moved into the area where it is necessary 
				// to move the bundle down, and if so, then do it, & update everything again
				bool bNeedAdvance = pView->NeedBundleRetreat(pApp->m_nActiveSequNum);
				if (bNeedAdvance)
				{
					// do the advance, return a new (valid) pointer to the active pile
					pApp->m_pActivePile = pView->AdvanceBundle(pApp->m_nActiveSequNum);
					//m_pActivePile = pApp->m_pActivePile; // put copy on the CPhraseBox too
				}

				int bSuccessful = MoveToImmedNextPile(pView,pApp->m_pActivePile);
				if (!bSuccessful)
				{
					if (gbUserWantsNoMove)
					{
						gbUserWantsNoMove = FALSE;
					}
					else
					{
						if ((pApp->m_curIndex == pApp->m_endIndex && 
								pApp->m_endIndex == pApp->m_maxIndex)
								|| pApp->m_pActivePile == NULL 
								|| pApp->m_nActiveSequNum == -1)
						{
							// tell the user EOF has been reached
							// IDS_AT_END
							wxMessageBox(_("The end. Provided you have not missed anything earlier, there is nothing more to adapt in this file."), _T(""), wxICON_INFORMATION);
							wxStatusBar* pStatusBar;
							//CFrameWnd* pFrame = pView->GetParentFrame();
							CMainFrame *pFrame = pApp->GetMainFrame();
							if (pFrame != NULL)
							{
								//pStatusBar = &((CMainFrame*)pFrame)->m_wndStatusBar;
								pStatusBar = pFrame->GetStatusBar();
								wxASSERT(pStatusBar != NULL);
								wxString str;
								// IDS_FINISHED_ADAPTING
								str = str.Format(_("End of the file; nothing more to adapt."));
								pStatusBar->SetStatusText(str,0);
							}
							// we are at EOF, so set up safe end conditions
							pApp->m_pTargetBox->Hide(); // whm added 12Sep04
							pApp->m_pTargetBox->Enable(FALSE); // whm added 12Sep04
							pApp->m_targetPhrase.Empty();
							pApp->m_nActiveSequNum = pApp->m_curIndex = -1;
							//pApp->m_targetBox.Destroy(); //pApp->m_targetBox.DestroyWindow();
							pApp->m_pTargetBox->SetValue(_T(""));
							pView->LayoutStrip(pApp->m_pSourcePhrases,
														nOldStripIndex,pApp->m_pBundle);
							pApp->m_pActivePile = (CPile*)NULL; // can use this as a flag for 
														 // at-EOF condition too
							pView->Invalidate();
// TRACE0("PhraseBox 5 OnChar\n");
						}
						else
						{
							// IDS_CANNOT_GO_FORWARD
							wxMessageBox(_("Sorry, the next pile cannot be a valid active location, so no move forward was done."),_T(""), wxICON_INFORMATION);
							pApp->m_pTargetBox->SetFocus();
						}
					}
					translation.Empty(); // clear the static string storage for the translation
					// save the phrase box's text, in case user hits SHIFT+END to unmerge a
					// phrase
					gSaveTargetPhrase = pApp->m_targetPhrase;
					return;
				}
				else
				{
					// MoveToImmedNextPile was successful

					// Horizontal Tab indicates user is moving stepwise in a forward direction.
					// Leaving the highlight in place seems to afford the best effect during
					// such stepwise movement.

					CCell* pCell = pApp->m_pActivePile->m_pCell[2]; // the cell where the
																	 // phraseBox is to be
					pView->ReDoPhraseBox(pCell); // like PlacePhraseBox, but calculations based
												 // on m_targetPhrase

					// reset the gLastSrcPhrasePos value
					gbEnterTyped = FALSE;
					SPList::Node* posTemp = gLastSrcPhrasePos; //POSITION posTemp = gLastSrcPhrasePos;
					if (posTemp != NULL)
					{
						CSourcePhrase* pSrcPhrase;
						pSrcPhrase = (CSourcePhrase*)posTemp->GetNext();
						posTemp = posTemp->GetNext();
						if (posTemp != NULL)
							gLastSrcPhrasePos = posTemp;
					}
				}

				pView->ScrollIntoView(pApp->m_nActiveSequNum);

				// save the phrase box's text, in case user hits SHIFT+END to unmerge a phrase
				gSaveTargetPhrase = pApp->m_targetPhrase;
				*/
			}
			return;
		}
	case WXK_BACK: //8:		// BackSpace key
		{
			bool bWasMadeDirty = TRUE;
			// whm Note: pApp->m_targetPhrase is updated in OnPhraseBoxChanged, so the wx version uses
			// the global below, rather than a value determined in OnChar(), which would not be current.
			FixBox(pView,pApp->m_targetPhrase,bWasMadeDirty,textExtent,2); // selector = 2 for contracting
		}
	default:
		;
	}
}

bool CPhraseBox::MoveToNextPile(CAdapt_ItView* pView, CPile* pCurPile)
// returns TRUE if the move was successful, FALSE if not successful
// Ammended July 2003 for auto-capitalization support
{
	bool bNoError = TRUE;
	bool bWantSelect = FALSE; // set TRUE if any initial text in the new location is to be 
							  // shown selected
	// store the translation in the knowledge base
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	bool bOK;
	gbByCopyOnly = FALSE; // restore default setting

	// make sure pApp->m_targetPhrase doesn't have any final spaces
	pView->RemoveFinalSpaces(pApp->m_pTargetBox,&pApp->m_targetPhrase);

	// don't move forward if it means moving to an empty retranslation pile, but only for
	// when we are adapting. When glossing, the box is allowed to be within retranslations
	{
		CPile* pNext = pView->GetNextEmptyPile(pCurPile);
		if (pNext == NULL)
		{
			// no more empty piles (in the current bundle). We can just continue at this point
			// since we do this call again below
			;
		}
		else
		{
			wxASSERT(pNext);

			if (pNext->m_pSrcPhrase->m_nSequNumber > pCurPile->m_pSrcPhrase->m_nSequNumber + 1)
			{
				// The next empty pile is not contiguous to the last pile where
				// the phrasebox was located, so don't highlight target/gloss text
				gnBeginInsertionsSequNum = -1;
				gnEndInsertionsSequNum = -1;
			}

			if (!gbIsGlossing && pNext->m_pSrcPhrase->m_bRetranslation)
			{
				// IDS_NO_ACCESS_TO_RETRANS
				wxMessageBox(_("Sorry, to edit or remove a retranslation you must use the toolbar buttons for those operations."), _T(""), wxICON_INFORMATION);
				pApp->m_pTargetBox->SetFocus();
				gbEnterTyped = FALSE;
				// if necessary restore default button image, and m_bCopySourcePunctuation to TRUE
				wxCommandEvent event;
				gpApp->GetView()->OnButtonEnablePunctCopy(event);
				return FALSE;
			}
		}
	}

	// if the location we are leaving is a <Not In KB> one, we want to skip the store & fourth
	// line creation --- as of Dec 18, version 1.4.0, according to Susanna Imrie's 
	// recommendation, I've changed this so it will allow a non-null adaptation to remain at 
	// this location in the document, but just to suppress the KB store; if glossing is ON, then
	// being a <Not In KB> location is irrelevant, and we will want the store done normally - but
	// to the glossing KB of course
	// BEW addition 21Apr06 to support transliterating better (showing transiterations)
	if (!gbIsGlossing && gpApp->m_bTransliterationMode && gbSuppressStoreForAltBackspaceKeypress)
	{
		gpApp->m_targetPhrase = gSaveTargetPhrase; // set it up in advance, from last LookAhead() call
		goto c;
	}
	if (!gbIsGlossing && !pCurPile->m_pSrcPhrase->m_bHasKBEntry 
					  && pCurPile->m_pSrcPhrase->m_bNotInKB)
	{
		// if the user edited out the <Not In KB> entry from the KB editor, we need to put
		// it back so that the setting is preserved (the "right" way to change the setting is to
		// use the toolbar checkbox - this applies when adapting, not glossing)
		wxString str = _T("<Not In KB>");
		CRefString* pRefStr = pView->GetRefString(pApp->m_pKB,pCurPile->m_pSrcPhrase->m_nSrcWords,
											pCurPile->m_pSrcPhrase->m_key,str);
		if (pRefStr == NULL)
		{
			pApp->m_bSaveToKB = TRUE; // it will be off, so we must turn it back on to get 
									   // the string restored
			// don't inhibit the call to MakeLineFourString( ) here, since the phrase passed
			// in is the non-punctuated one
			bool bOK;
			bOK = pView->StoreText(pApp->m_pKB,pCurPile->m_pSrcPhrase,str);
			// set the flags to ensure the asterisk shows above the pile, etc.
			pCurPile->m_pSrcPhrase->m_bHasKBEntry = FALSE;
			pCurPile->m_pSrcPhrase->m_bNotInKB = TRUE; 
		}

		// for version 1.4.0 and onwards, we have to permit the construction of the punctuated 
		// target string; for auto caps support, we may have to change to UC here too
		wxString str1 = pApp->m_targetPhrase;
		pView->RemovePunctuation(pDoc,&str1,1 /*from tgt*/);
		if (gbAutoCaps)
		{
			bNoError = pView->SetCaseParameters(pCurPile->m_pSrcPhrase->m_key);
			if (bNoError && gbSourceIsUpperCase && !gbMatchedKB_UCentry)
			{
				bNoError = pView->SetCaseParameters(str1,FALSE);
				if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
				{
					// a change to upper case is called for
					str1.SetChar(0,gcharNonSrcUC);
				}
			}
		}
		pCurPile->m_pSrcPhrase->m_adaption = str1;

		// the following function is now smart enough to capitalize m_targetStr in the context
		// of preceding punctuation, etc, if required. No store done here, of course, since we
		// are just restoring a <Not In KB> entry.
		pView->MakeLineFourString(pCurPile->m_pSrcPhrase,pApp->m_targetPhrase);
		goto b;
	}

	// make the punctuated target string, but only if adapting; note, for auto capitalization
	// ON, the function will change initial lower to upper as required, whatever punctuation
	// regime is in place for this particular sourcephrase instance

	// BEW added 19Apr06 for support of Alt + Backspace keypress suppressing store on the phrase box move
	// which is a feature for power users requested by Bob Eaton; the code in the first block is for
	// this new (undocumented) feature - power uses will have knowledge of it from an article to be
	// placed in Word&Deed by Bob. Only needed for adapting mode, so the glossing mode case is commented out
c:	bOK = TRUE;
	if (!gbIsGlossing && gpApp->m_bTransliterationMode && gbSuppressStoreForAltBackspaceKeypress)
	{
		// when we don't want to store in the KB, we still have some things to do
		// to get appropriate m_adaption and m_targetStr members set up for the doc
		//if (!gbIsGlossing)
		//{
			// when adapting, fill out the m_targetStr member of the CSourcePhrase instance,
			// and do any needed case conversion and get punctuation in place if required
			pView->MakeLineFourString(pCurPile->m_pSrcPhrase,gpApp->m_targetPhrase);

			// the m_targetStr member may now have punctuation, so get rid of it
			// before assigning whatever is left to the m_adaption member
			wxString strKeyOnly = pCurPile->m_pSrcPhrase->m_targetStr;
			pView->RemovePunctuation(pDoc,&strKeyOnly,1 /*from tgt*/);

			// set the m_adaption member too
			pCurPile->m_pSrcPhrase->m_adaption = strKeyOnly;

			// let the user see the unpunctuated string in the phrase box as visual feedback
			gpApp->m_targetPhrase = strKeyOnly;
		//}
		//else
		//{
			// when glossing, we just take the box contents 'as is', punctuation included
		//	pCurPile->m_pSrcPhrase->m_gloss = pView->m_targetPhrase;
		//}

		// now do a store, but only of <Not In KB>, (StoreText uses gbSuppressStoreForAltBackspaceKeypress
		// == TRUE to get this job done rather than a normal store) & sets flags appropriately
		gbInhibitLine4StrCall = TRUE;
		bOK = pView->StoreText(pView->GetKB(),pCurPile->m_pSrcPhrase,gpApp->m_targetPhrase);
		gbInhibitLine4StrCall = FALSE;
	}
	else
	{
		// gbSuppressStoreForAltBackspaceKeypress is FALSE, so we are in normal adapting
		// or glossing mode
		if (!gbIsGlossing)
			pView->MakeLineFourString(pCurPile->m_pSrcPhrase,gpApp->m_targetPhrase);

		// we are about to leave the current phrase box location, so we must try to store what is 
		// now in the box, if the relevant flags allow it. Test to determine which KB to store to.
		// StoreText( ) has been ammended for auto-capitalization support (July 2003)
		if (!gbIsGlossing)
			pView->RemovePunctuation(pDoc,&pApp->m_targetPhrase,1 /*from tgt*/);
		if (gbIsGlossing)
			bOK = pView->StoreText(pApp->m_pGlossingKB,pCurPile->m_pSrcPhrase,
																		pApp->m_targetPhrase);
		else
		{
			gbInhibitLine4StrCall = TRUE;
			bOK = pView->StoreText(pApp->m_pKB,pCurPile->m_pSrcPhrase,pApp->m_targetPhrase);
			gbInhibitLine4StrCall = FALSE;
		}

		// if in Transliteration Mode we want to cause gbSuppressStoreForAltBackspaceKeypress
		// be immediately turned back on, in case a <Not In KB> entry is at the next lookup location
		// and we will then want the special Transliteration Mode KB storage process to be done rather
		// than a normal empty phrasebox for such an entry
		if (gpApp->m_bTransliterationMode)
		{
			gbSuppressStoreForAltBackspaceKeypress = TRUE;
		}
	}
	if (!bOK)
	{
		if (!pApp->m_bSingleStep)
		{
			gbEnterTyped = FALSE;
			pApp->m_bAutoInsert = FALSE; // cause halt, if auto lookup & inserting is ON
		}
		gbEnterTyped = FALSE;
		// if necessary restore default button image, and m_bCopySourcePunctuation to TRUE
		wxCommandEvent event;
		gpApp->GetView()->OnButtonEnablePunctCopy(event);
		if (gbSuppressStoreForAltBackspaceKeypress)
			gSaveTargetPhrase.Empty();
		gTemporarilySuspendAltBKSP = FALSE;
		gbSuppressStoreForAltBackspaceKeypress = FALSE; // make sure it's off before returning
		return FALSE; // can't move until a valid adaption (which could be null) is supplied
	}

	// since we are moving, make sure the default m_bSaveToKB value is set
b:	pApp->m_bSaveToKB = TRUE;

	// store the current strip index, for update purposes
	int nCurStripIndex;
	nCurStripIndex = pCurPile->m_pStrip->m_nStripIndex;

	// move to next pile's cell which has no adaptation yet
	pApp->m_bUserTypedSomething = FALSE; // user has not typed at the new location yet
	bool bAdaptationAvailable = FALSE;
	CPile* pNewPile = pView->GetNextEmptyPile(pCurPile); // this call does not update the active
														 // sequ number
	// if necessary restore default button image, and m_bCopySourcePunctuation to TRUE
	wxCommandEvent event;
	gpApp->GetView()->OnButtonEnablePunctCopy(event);
	if (pNewPile == NULL)
	{
		if (!pApp->m_bSingleStep)
		{
			pApp->m_bAutoInsert = FALSE; // cause halt, if auto lookup & inserting is ON
		}

		// ensure the view knows the pile pointer is no longer valid
		pApp->m_pActivePile = (CPile*)NULL;
		pApp->m_nActiveSequNum = -1;
		gbEnterTyped = FALSE;
		if (gbSuppressStoreForAltBackspaceKeypress)
			gSaveTargetPhrase.Empty();
		gbSuppressStoreForAltBackspaceKeypress = FALSE; // make sure it's off before returning
		gTemporarilySuspendAltBKSP = FALSE;
		return FALSE; // we are at the end of the bundle (possibly end of file too), 
					  // so can't move further in this bundle (caller should check to see if the
					  // bundle can be advanced, and do so etc. if possible)
	}
	else
	{
		// the pNewPile is valid, so proceed
		// set active pile, and same var on the phrase box, and active sequ number - but note 
		// that only the active sequence number will remain valid if a merge is required; in the
		// latter case, we will have to recalc the layout after the merge and set the first two 
		// variables again
		pApp->m_pActivePile = pNewPile;
		m_pActivePile = pNewPile; // put a copy on CPhraseBox too (we use this below)
		pApp->m_nActiveSequNum = pNewPile->m_pSrcPhrase->m_nSequNumber;
		nCurrentSequNum = pApp->m_nActiveSequNum; // global, for use by auto-saving

		// look ahead for a match at this new active location
		// LookAhead (July 2003) has been ammended for auto-capitalization support; and since
		// it does a KB lookup, it will set gbMatchedKB_UCentry TRUE or FALSE; and if an
		// entry is found, any needed case change will have been done prior to it returning
		// (the result is in the global variable: translation)
		bAdaptationAvailable = LookAhead(pView,pNewPile);
		pView->RemoveSelection();

		// check if we found a match and have an available adaptation string ready
		if (bAdaptationAvailable)
		{
			pApp->m_pTargetBox->m_bAbandonable = FALSE;
			// adaptation is available, so use it if the source phrase has only a single word,
			// but if it's multi-worded, we must first do a merge and recalc of the layout
			if (!gbIsGlossing && !gbSuppressMergeInMoveToNextPile)
			{
				// this merge is suppressed if we get here after doing a merge in LookAhead() in
				// order to see the phrase box at the correct location when the Choose 
				// Translation dialog is up
				if (nWordsInPhrase > 1) // nWordsInPhrase is a global, set in LookAhead() or in
										// LookUpSrcWord()
				{
					// do the needed merge, etc.
					pApp->bLookAheadMerge = TRUE; // set static flag to ON
					bool bSaveFlag = m_bAbandonable; // the box is "this"
					pView->MergeWords();
					m_bAbandonable = bSaveFlag; // preserved the flag across the merge
					pApp->bLookAheadMerge = FALSE; // restore static flag to OFF
				}
			}
			else
			{
				// clear the flag to false
				gbSuppressMergeInMoveToNextPile = FALSE; // make sure it's reset to default 
														 // value
			}

			// For automatically inserted target/gloss text highlighting, we 
			// increment the Ending Sequence Number as long as the Beginning 
			// Sequence Number is non-negative. If the Beginning Sequence Number
			// is negative, the Ending Sequence Number must also be negative.
			// Each time a new insertion is done, the test below checks for a zero
			// or positive value of gnBeginInsertionsSequNum, and if it finds that 
			// is the case, the True block just increments the gnEndInsertionsSequNum 
			// value, the False block insures both globals have -1 values. So after 
			// the 2nd insertion, the two globals have values differing by 1 
			// and so the previous two insertions get the highlighting, etc. 
			if (gnBeginInsertionsSequNum >= 0)
			{
				gnEndInsertionsSequNum++;
			}
			else
			{
				gnEndInsertionsSequNum = gnBeginInsertionsSequNum;
			}

			// assign the translation text - but check it's not "<Not In KB>", if it is, phrase box 
			// can have m_targetStr, turn OFF the m_bSaveToKB flag, DON'T halt 
			// auto-inserting if it is on, (in the very earliest versions I made it halt) -- 
			// for version 1.4.0 and onwards, this does not change because when auto inserting,
			// we must have a default translation for a 'not in kb' one - and the only 
			// acceptable default is a null string. The above applies when gbIsGlossing is OFF
			wxString str = translation; // translation set within LookAhead()

			// BEW added 21Apr06, so that when transliterating the lookup puts a fresh transliteration
			// of the source when it finds a <Not In KB> entry, since the latter signals that the 
			// SIL Converters conversion yields a correct result for this source text, so we want the
			// user to get the feedback of seeing it, but still just have <Not In KB> in the KB entry
			if (!gpApp->m_bSingleStep && (translation == _T("<Not In KB>")) && gTemporarilySuspendAltBKSP
				&& !gbIsGlossing && gpApp->m_bTransliterationMode)
			{
				gbSuppressStoreForAltBackspaceKeypress = TRUE;
				gTemporarilySuspendAltBKSP = FALSE;
			}

			if (!gbIsGlossing && gpApp->m_bTransliterationMode &&  gbSuppressStoreForAltBackspaceKeypress 
				&& (translation == _T("<Not In KB>")))
			{
				gpApp->m_bSaveToKB = FALSE;
				// CopySourceKey checks m_bUseSILConverter internally, & calls DoSilConvert() if TRUE,
				// returning the converted string, or if the BOOL is FALSE, returning the key unchanged
				wxString str = pView->CopySourceKey(pNewPile->m_pSrcPhrase,gpApp->m_bUseConsistentChanges);
				bWantSelect = FALSE;
				gpApp->m_pTargetBox->m_bAbandonable = TRUE;
				pNewPile->m_pSrcPhrase->m_bHasKBEntry = FALSE; 
				pNewPile->m_pSrcPhrase->m_bNotInKB = TRUE; // ensures * shows above
				pNewPile->m_pSrcPhrase->m_adaption = str;
				pNewPile->m_pSrcPhrase->m_targetStr = pNewPile->m_pSrcPhrase->m_precPunct + str;
				pNewPile->m_pSrcPhrase->m_targetStr += pNewPile->m_pSrcPhrase->m_follPunct;
				translation = pNewPile->m_pSrcPhrase->m_targetStr;
				gpApp->m_targetPhrase = translation;
				gSaveTargetPhrase = translation; // to make it available on next auto call of OnePass()

				// don't turn the gbSuppressStoreForAltBackspaceKeypress flag back off yet
				// because we want it on while there are autoinsertions happening, and we turn it
				// off only when we have to halt because there is no KB entry
				//gbSuppressStoreForAltBackspaceKeypress = FALSE;
			}
			// continue with the legacy code
			else if (!gbIsGlossing && (translation == _T("<Not In KB>")))
			{
				gpApp->m_bSaveToKB = FALSE;
				translation = pNewPile->m_pSrcPhrase->m_targetStr; // probably empty
				pApp->m_targetPhrase = translation;
				bWantSelect = FALSE;
				pApp->m_pTargetBox->m_bAbandonable = TRUE;
				pNewPile->m_pSrcPhrase->m_bHasKBEntry = FALSE; 
				pNewPile->m_pSrcPhrase->m_bNotInKB = TRUE; // ensures * shows above
														   // this srcPhrase
			}
			else
			{
				pApp->m_targetPhrase = translation;
				bWantSelect = FALSE;

				if (gbSuppressStoreForAltBackspaceKeypress && gpApp->m_bTransliterationMode)
				{
					// was the normal entry found while the gbSuppressStoreForAltBackspaceKeypress
					// flag was TRUE? Then we have to turn the flag off for a while, but turn it
					// on programmatically later if we are still in Automatic mode and we come to
					// another <Not In KB> entry. We can do this with another BOOL defined for this
					// purpose
					gTemporarilySuspendAltBKSP = TRUE;
					gbSuppressStoreForAltBackspaceKeypress = FALSE;
				}
			}

			// treat auto insertion as if user typed it, so that if there is a user-generated
			// extension done later, the inserted translation will not be removed and copied
			// source text used instead; since user probably is going to just make a minor 
			// modification
			gpApp->m_bUserTypedSomething = TRUE;
		}
		else // the lookup determined that no adaptation (or gloss when glossing), or
			 // <Not In KB> entry, is available
		{
			// we're gunna halt, so this is the time to clear the flag
			if (!gbIsGlossing && gpApp->m_bTransliterationMode && gbSuppressStoreForAltBackspaceKeypress)
				gSaveTargetPhrase.Empty();
			gbSuppressStoreForAltBackspaceKeypress = FALSE; // make sure it's off before returning

			pNewPile = gpApp->m_pActivePile; // ensure its valid, we may get here after a 
			// RecalcLayout call when there is no adaptation available from the LookAhead, (or
			// user cancelled when shown the Choose Translation dialog from within the 
			// LookAhead() function, having matched) we must cause auto lookup and inserting to 
			// be turned off, so that the user can do a manual adaptation; but if the 
			// m_bAcceptDefaults flag is on, then the copied source (having been through 
			// c.changes) is accepted without user input, the m_bAutoInsert flag is turned back
			// on, so processing will continue; while if gbUserWantsSelection is TRUE, then the
			// first two words are selected instead ready for a merger or for extending the 
			// selection - if both flags are TRUE, the gbUserWantsSelection is to have priority
			// - but the code for handling it will have to be in OnChar() because recalcs wipe
			//  out any selections
			if (!gpApp->m_bSingleStep)
			{
				gpApp->m_bAutoInsert = FALSE; // cause halt

				if (!gbIsGlossing && gbUserWantsSelection)
				{
					// user cancelled CChooseTranslation dialog because he wants instead to
					// select for a merger of two or more source words
					gpApp->m_pTargetBox->m_bAbandonable = TRUE;

					// no adaptation available, so depending on the m_bCopySource flag, either
					// initialize the targetPhrase to an empty string, or to a copy of the 
					// sourcePhrase's key string; then select the first two words ready for a 
					// merger or extension of the selection
					if (gpApp->m_bCopySource)
					{
						if (!pNewPile->m_pSrcPhrase->m_bNullSourcePhrase)
						{
							gpApp->m_targetPhrase 
								= pView->CopySourceKey(pNewPile->m_pSrcPhrase,
														gpApp->m_bUseConsistentChanges);
							bWantSelect = TRUE;
						}
						else
						{
							// its a null source phrase, so we can't copy anything
							gpApp->m_targetPhrase.Empty(); // this will cause pile's m_nMinWidth
										// to be used to set the pApp->m_curBoxWidth value on the view
						}
					}
					else
					{
						// no copy of source wanted, so just make it an empty string
						gpApp->m_targetPhrase.Empty(); // ditto
					}
					// the DoCancelAndSelect() call is below after the RecalcLayout calls
				}
				else // user does not want a "Cancel and Select" selection; or is glossing
				{
					// try find a translation for the single source word, use it if we find one;
					// else do the usual copy of source word, with possible cc processing, etc.
					// LookUpSrcWord( ) has been ammended (July 2003) for auto capitalization 
					// support; it does any needed case change before returning, leaving the 
					// resulting string in the global variable: translation
					bool bGotTranslation = FALSE; 
					if (!gbUserCancelledChooseTranslationDlg)
					{
						bGotTranslation = LookUpSrcWord(pView,pNewPile);
					}
					else
					{
						gbUserCancelledChooseTranslationDlg = FALSE;

						// if the user cancelled the Choose Translation dialog when a phrase was
						// merged, then he will probably want a lookup done for the first word 
						// of the now unmerged phrase; nWordsInPhrase will still contain the 
						// word count for the formerly merged phrase, so use it; but when glossing
						// nWordsInPhrase should never be anything except 1, so this block should
						// not get entered when glossing
						if (nWordsInPhrase > 1) // nWordsInPhrase is a global, set in LookAhead()
												// or in LookUpSrcWord()
						{
							bGotTranslation = LookUpSrcWord(pView,pNewPile);
						}
					}
					pNewPile = gpApp->m_pActivePile; // update the pointer

					if (bGotTranslation)
					{
						// if it is a <Not In KB> entry we show any m_targetStr that the
						// sourcephrase instance may have, by putting it in the global
						// translation variable; when glossing is ON, we ignore
						// "not in kb" since that pertains to adapting only
						if (!gbIsGlossing && translation == _T("<Not In KB>"))
						{
							// make sure asterisk gets shown, and the adaptation is taken
							// from the sourcephrase itself - but it will be empty
							// if the sourcephrase has not been accessed before
							translation = pNewPile->m_pSrcPhrase->m_targetStr;
							pNewPile->m_pSrcPhrase->m_bHasKBEntry = FALSE;
							pNewPile->m_pSrcPhrase->m_bNotInKB = TRUE;
						}

						gpApp->m_targetPhrase = translation; // set using the global var, 
															 // set in LookUpSrcWord call
						bWantSelect = TRUE;
					}
					else // did not get a translation, or a gloss when glossing is current
					{
						if (gpApp->m_bCopySource)
						{
							// copy source key only provided this is not a null source phrase,
							// don't want "..." copied! No case changes need be done when
							// a copy is performed.
							gpApp->m_targetPhrase =
										pView->CopySourceKey(pNewPile->m_pSrcPhrase,
										gpApp->m_bUseConsistentChanges);
							bWantSelect = TRUE;
							gpApp->m_pTargetBox->m_bAbandonable = TRUE;
						}
						else // leave the phrase box blank
						{
							gpApp->m_targetPhrase.Empty();
							bWantSelect = FALSE;
						}
					}

					// is "Accept Defaults" turned on? If so, make processing continue
					if (gpApp->m_bAcceptDefaults)
					{
						gpApp->m_bAutoInsert = TRUE; // revoke the halt
					}
				}
			}
			else // it's single step mode
			{
				gpApp->m_pTargetBox->m_bAbandonable = TRUE;

				// it is single step mode & no adaptation available, so see if we can find a 
				// translation, or gloss, for the single src word at the active location, if not, 
				// depending on the m_bCopySource flag, either initialize the targetPhrase to
				// an empty string, or to a copy of the sourcePhrase's key string
				bool bGotTranslation = FALSE;
				if (!gbIsGlossing && gbUserWantsSelection)
					goto f; // in ChooseTranslation dialog the user wants the 'cancel and select'
							// option, and since no adaptation is therefore to be retreived, it
							// remains just to either copy the source word or nothing

				if (!gbUserCancelledChooseTranslationDlg)
				{
					bGotTranslation = LookUpSrcWord(pView,pNewPile); // try find a translation 
								// for the single word; July 2003 supports auto capitalization
				}
				else
				{
					gbUserCancelledChooseTranslationDlg = FALSE;

					// if the user cancelled the Choose Translation dialog when a phrase was 
					// merged, then he will probably want a lookup done for the first word of
					// the now unmerged phrase; nWordsInPhrase will still contain the word count
					// for the formerly merged phrase, so use it; but when glossing is current,
					// the LookUpSrcWord call is done only in the first map, so nWordsInPhrase
					// will not be greater than 1 when doing glossing
					if (nWordsInPhrase > 1) // nWordsInPhrase is a global, set in LookAhead()
											// or in LookUpSrcWord()
					{
						bGotTranslation = LookUpSrcWord(pView,pNewPile);
					}
				}
				pNewPile = gpApp->m_pActivePile; // update the pointer, since LookUpSrcWord() 
												 // calls RecalcLayout()
				if (bGotTranslation)
				{
					// if it is a <Not In KB> entry we show any m_targetStr that the
					// sourcephrase instance may have, by putting it in the global
					// translation variable; when glossing is ON, we ignore
					// "not in kb" since that pertains to adapting only
					if (!gbIsGlossing && translation == _T("<Not In KB>"))
					{
						// make sure asterisk gets shown, and the adaptation is taken
						// from the sourcephrase itself - but it will be empty
						// if the sourcephrase has not been accessed before
						translation = pNewPile->m_pSrcPhrase->m_targetStr;
						pNewPile->m_pSrcPhrase->m_bHasKBEntry = FALSE;
						pNewPile->m_pSrcPhrase->m_bNotInKB = TRUE;
					}

					gpApp->m_targetPhrase = translation; // set using the global var, set in 
														 // LookUpSrcWord() call
					bWantSelect = TRUE;
				}
				else // did not get a translation, or gloss
				{
					// do a copy of the source; this never needs change of capitalization
f:					if (gpApp->m_bCopySource)
					{
						if (!pNewPile->m_pSrcPhrase->m_bNullSourcePhrase)
						{
							gpApp->m_targetPhrase = 
												pView->CopySourceKey(pNewPile->m_pSrcPhrase,
												gpApp->m_bUseConsistentChanges);
							bWantSelect = TRUE;
						}
						else
						{
							// its a null source phrase, so we can't copy anything; and if
							// we are glossing, we just leave these empty whenever we meet them
							gpApp->m_targetPhrase.Empty(); // this will cause pile's m_nMinWidth
									// to be used to set the pApp->m_curBoxWidth value on the view
						}
					}
					else
					{
						// no copy of source wanted, so just make it an empty string
						gpApp->m_targetPhrase.Empty(); // ditto
					}
				}
			}
		}

		// initialize the phrase box too, so it doesn't carry the old string to the next 
		// pile's cell
		SetValue(gpApp->m_targetPhrase); //SetWindowText(pApp->m_targetPhrase); 

		// if we merged and moved, we have to update pNewPile, because we have done a 
		// RecalcLayout in the LookAhead() function
		if (gbCompletedMergeAndMove)
		{
			pNewPile = gpApp->m_pActivePile; // safe, whether glossing or not
		}

		// recalculate the layout from sequence number = 0 now to be safe
		gpApp->m_curBoxWidth = 2; // make very small so it doesn't push the next word/phrase to
								  // the right before the next word/phrase can be measured
		//pView->RecalcLayout(pApp->m_pSourcePhrases, 0, pView->m_pBundle);
		pView->RecalcLayout(gpApp->m_pSourcePhrases, 0, gpApp->m_pBundle);

		// get the new active pile
		gpApp->m_pActivePile = pView->GetPile(gpApp->m_nActiveSequNum);
		wxASSERT(gpApp->m_pActivePile != NULL);

		// update the copy held on the CPhraseBox too
		m_pActivePile = gpApp->m_pActivePile;

		// if the user has turned on the sending of synchronized scrolling messages
		// send the relevant message once auto-inserting halts, because we don't want to make
		// other applications sync scroll during auto-insertions, as it could happen very often
		// and the user can't make any visual use of what would be happening anyway; even if a
		// Cancel and Select is about to be done, a sync scroll is appropriate now, provided
		// auto-inserting has halted
		if (!gbIgnoreScriptureReference_Send && !gpApp->m_bAutoInsert)
		{
			pView->SendScriptureReferenceFocusMessage(gpApp->m_pSourcePhrases,gpApp->m_pActivePile->m_pSrcPhrase);
		}
		
		// we had to delay the call of DoCancelAndSelect() until now because earlier 
		// RecalcLayout() calls will clobber any selection we try to make beforehand, so do the 
		// selecting now; do it also before recalculating the phrase box, since if anything 
		// moves, we want m_ptCurBoxLocation to be correct. When glossing, Cancel and Select
		// is not allowed, so we skip this block
		if (!gbIsGlossing && gbUserWantsSelection)
		{
			DoCancelAndSelect(pView,gpApp->m_pActivePile);
			gbUserWantsSelection = FALSE; // must be turned off before we do anything else!
			gpApp->m_bSelectByArrowKey = TRUE; // so it is ready for extending
		}
		
		// update status bar with project name
		gpApp->RefreshStatusBarInfo();

		// recreate the phraseBox using the stored information
		if (bWantSelect)
		{
			gpApp->m_nStartChar = 0; gpApp->m_nEndChar = -1;
		}
		else
		{
			int len = GetLineLength(0); // 0 = first line, only line
			gpApp->m_nStartChar = gpApp->m_nEndChar = len;
		}
		gpApp->m_ptCurBoxLocation = gpApp->m_pActivePile->m_pCell[2]->m_ptTopLeft;
		pView->RemakePhraseBox(gpApp->m_pActivePile,gpApp->m_targetPhrase);

		// fix the m_bSaveToKB flag, depending on whether or not srcPhrase is in kb; but this
		// applies only when adapting, not glossing
		if (!gbIsGlossing && !gpApp->m_pActivePile->m_pSrcPhrase->m_bHasKBEntry && 
											gpApp->m_pActivePile->m_pSrcPhrase->m_bNotInKB)
		{
			gpApp->m_bSaveToKB = FALSE;
			gpApp->m_targetPhrase.Empty();
		}
		else
		{
			gpApp->m_bSaveToKB = TRUE;
		}

		gbCompletedMergeAndMove = FALSE; // make sure it's cleared
		pView->Invalidate(); // do the whole client area, because if target font is larger than
		// the source font then changes along the line throw words off screen and they get 
		// missed and eventually app crashes because active pile pointer will get set to NULL

		if (bWantSelect)
			SetModify(TRUE); // our own SetModify(); calls MarkDirty()
		else
			SetModify(FALSE); // our own SetModify(); calls DiscardEdits()

		return TRUE;
	}
}

bool CPhraseBox::MoveToPrevPile(CAdapt_ItView *pView, CPile *pCurPile)
// returns TRUE if the move was successful, FALSE if not successful
// Ammended July 2003 for auto-capitalization support
{
	gbMovingToPreviousPile = TRUE; // set here, but we clear it after the ReDoPhraseBox( ) calls in 
															 // the relevant part (2 calls) in OnChar( ) after the suppression
															 // of the MakeLineFourString( ) call has been effected
	// store the current translation, if one exists, before retreating, since each retreat
	// unstores the refString's translation from the KB, so they must be put back each time
	// (perhaps in edited form, if user changed the string before moving back again)
	CAdapt_ItDoc* pDoc = pView->GetDocument();
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	gbByCopyOnly = FALSE; // restore default setting

	// make sure m_targetPhrase doesn't have any final spaces either
	pView->RemoveFinalSpaces(pApp->m_pTargetBox,&pApp->m_targetPhrase);

	// if we are at the start, we can't move back any further
	int nCurSequNum = pCurPile->m_pSrcPhrase->m_nSequNumber;
	if (nCurSequNum == 0)
	{
		// IDS_CANNOT_GO_BACK
		wxMessageBox(_("Sorry, you are already at the start of the file, so it is not possible to move back any further."), _T(""), wxICON_INFORMATION);
		pApp->m_pTargetBox->SetFocus();
		return FALSE;
	}
	bool bOK;

	// don't move back if it means moving to a retranslation pile; but if we are
	// glossing it is okay to move back into a retranslated section
	{
		CPile* pPrev = pView->GetPrevPile(pCurPile);
		wxASSERT(pPrev);
		if (!gbIsGlossing && pPrev->m_pSrcPhrase->m_bRetranslation)
		{
			// IDS_NO_ACCESS_TO_RETRANS
			wxMessageBox(_("Sorry, to edit or remove a retranslation you must use the toolbar buttons for those operations."),_T(""), wxICON_INFORMATION);
			pApp->m_pTargetBox->SetFocus();
			return FALSE;
		}
	}

	// if the location is a <Not In KB> one, we want to skip the store & fourth line creation
	// --- as of Dec 18, version 1.4.0, according to Susanna Imrie's 
	// recommendation, I've changed this so it will allow a non-null adaptation to remain at 
	// this location in the document, but just to suppress the KB store; if glossing is ON, then
	// being a <Not In KB> location is irrelevant, and we will want the store done normally - but
	// to the glossing KB of course	
	bool bNoError = TRUE;
	if (!gbIsGlossing && !pCurPile->m_pSrcPhrase->m_bHasKBEntry && pCurPile->m_pSrcPhrase->m_bNotInKB)
	{
		// in case the user edited out the <Not In KB> entry from the KB editor, we need to put 
		// it back so that the setting is preserved (the "right" way to change the setting is to
		// use the toolbar checkbox)
		wxString str = _T("<Not In KB>");
		CRefString* pRefStr = pView->GetRefString(pApp->m_pKB,pCurPile->m_pSrcPhrase->m_nSrcWords,
											pCurPile->m_pSrcPhrase->m_key,str);
		if (pRefStr == NULL)
		{
			pApp->m_bSaveToKB = TRUE; // it will be off, so we must turn it back on to get 
									   // the string restored
			bool bOK;
			bOK = pView->StoreText(pApp->m_pKB,pCurPile->m_pSrcPhrase,str);
			// set the flags to ensure the asterisk shows above the pile, etc.
			pCurPile->m_pSrcPhrase->m_bHasKBEntry = FALSE;
			pCurPile->m_pSrcPhrase->m_bNotInKB = TRUE; 
		}
		
		// make the punctuated target string 
		wxString str1 = pApp->m_targetPhrase;
		pView->RemovePunctuation(pDoc,&str1,1 /*from tgt*/);
		if (gbAutoCaps)
		{
			bNoError = pView->SetCaseParameters(pCurPile->m_pSrcPhrase->m_key);
			if (bNoError && gbSourceIsUpperCase && !gbMatchedKB_UCentry)
			{
				bNoError = pView->SetCaseParameters(str1,FALSE);
				if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
				{
					// a change to upper case is called for
					str1.SetChar(0,gcharNonSrcUC);
				}
			}
		}
		pCurPile->m_pSrcPhrase->m_adaption = str1;

		// the following function is now smart enough to capitalize m_targetStr in the context
		// of preceding punctuation, etc, if required. (No store needed here, we are just fixing
		// a <Not In KB> entry)
		pView->MakeLineFourString(pCurPile->m_pSrcPhrase,pApp->m_targetPhrase);
		goto b;
	}

	// if the box contents is null, then the source phrase must store an empty string
	// as appropriate - either m_adaption when adapting, or m_gloss when glossing
	if (pApp->m_targetPhrase.IsEmpty())
	{
		pApp->m_bForceAsk = FALSE; // make sure it's turned off, & allow function to continue
		if (gbIsGlossing)
			pCurPile->m_pSrcPhrase->m_gloss.Empty();
		else
			pCurPile->m_pSrcPhrase->m_adaption.Empty();
	}

	// make the punctuated target string, but only if adapting; note, for auto capitalization
	// ON, the function will change initial lower to upper as required, whatever punctuation
	// regime is in place for this particular sourcephrase instance
	if (!gbIsGlossing)
		pView->MakeLineFourString(pCurPile->m_pSrcPhrase,pApp->m_targetPhrase);

	// we are about to leave the current phrase box location, so we must try to store what is 
	// now in the box, if the relevant flags allow it. Test to determine which KB to store to.
	// StoreText( ) has been ammended for auto-capitalization support (July 2003)
	if (!gbIsGlossing)	
		pView->RemovePunctuation(pDoc,&pApp->m_targetPhrase,1 /*from tgt*/);
	gbInhibitLine4StrCall = TRUE;
	bOK = pView->StoreTextGoingBack(pView->GetKB(),pCurPile->m_pSrcPhrase,
																pApp->m_targetPhrase);
	gbInhibitLine4StrCall = FALSE;
	if (!bOK)
		return FALSE; // can't move if the adaptation text is not yet completed

	// store the current strip index, for update purposes
b:	int nCurStripIndex;
	nCurStripIndex = pCurPile->m_pStrip->m_nStripIndex;

	// move to previous pile's cell
	CPile* pNewPile = pView->GetPrevPile(pCurPile); // does not update the view's 
				// m_nActiveSequNum nor m_pActivePile pointer, so update these here, 
				// provided NULL was not returned

	// restore default button image, and m_bCopySourcePunctuation to TRUE
	wxCommandEvent event;
	gpApp->GetView()->OnButtonEnablePunctCopy(event);
	if (pNewPile == NULL)
		return FALSE; // we are at the start of the bundle (possibly start of file too), 
					  // so can't retreat further in this bundle (caller should check to
					  // see if the bundle can be retreated, and do so etc. if possible)
	else
	{
		pApp->m_bUserTypedSomething = FALSE; // user has not typed at the new location yet

		// update the active sequence number, and pile pointer
		pApp->m_nActiveSequNum = pNewPile->m_pSrcPhrase->m_nSequNumber;
		pApp->m_pActivePile = pNewPile;
		m_pActivePile = pNewPile; // put a copy on CPhraseBox too (we use this below)

		// since we are moving back, the prev pile is likely to have a refString translation
		// which is nonempty, so we have to put it into m_targetPhrase so that ResizeBox will
		// use it; but if there is none, the copy the source key if the m_bCopySource flag is 
		// set, else just set it to an empty string. (bNeed Modify is a helper flag used for
		// setting/clearing the document's modified flag at the end of this function)
		bool bNeedModify = FALSE; // reset to TRUE if we copy source because there was 
								  // no adaptation

		// be careful, the pointer might point to <Not In KB>, rather than a normal entry
		CRefString* pRefString;
		if (gbIsGlossing)
		{
			pRefString = pView->GetRefString(pView->GetKB(), 1,
				pNewPile->m_pSrcPhrase->m_key, pNewPile->m_pSrcPhrase->m_gloss);
		}
		else
		{
			pRefString = pView->GetRefString(pView->GetKB(),
				pNewPile->m_pSrcPhrase->m_nSrcWords, pNewPile->m_pSrcPhrase->m_key,
				pNewPile->m_pSrcPhrase->m_adaption);
		}

		if (pRefString != NULL)
		{
			pView->RemoveSelection(); // we won't do merges in this situation
			
			// assign the translation text - but check it's not "<Not In KB>", if it is, we 
			// leave the phrase box empty, turn OFF the m_bSaveToKB flag -- this is changed 
			// for v1.4.0 and onwards because we will want to leave any adaptation already 
			// present unchanged, rather than clear it and so we will not make it abandonable
			// either
			wxString str = pRefString->m_translation; // no case change to be done here since
													 // all we want to do is remove the refString
													 // or decrease its reference count
			if (!gbIsGlossing && str == _T("<Not In KB>"))
			{
				pApp->m_bSaveToKB = FALSE;
				pApp->m_pTargetBox->m_bAbandonable = FALSE; // used to be TRUE;
				pNewPile->m_pSrcPhrase->m_bHasKBEntry = FALSE; // ensures * shows above this
															   // srcPhrase
				pNewPile->m_pSrcPhrase->m_bNotInKB = TRUE;
			}
			
			// remove the translation from the KB, in case user wants to edit it before it's 
			// stored again (RemoveRefString also clears the m_bHasKBEntry flag on the source
			// phrase)
			if (gbIsGlossing)
			{
				pView->RemoveRefString(pRefString, pNewPile->m_pSrcPhrase, 1);
				pApp->m_targetPhrase = pNewPile->m_pSrcPhrase->m_gloss;
			}
			else
			{
				pView->RemoveRefString(pRefString,pNewPile->m_pSrcPhrase,
														pNewPile->m_pSrcPhrase->m_nSrcWords);
				// since we have optional punctuation hiding, use the line with the punctuation
				pApp->m_targetPhrase = pNewPile->m_pSrcPhrase->m_targetStr;
			}
		}
		else // the pointer to refString was null (ie. no KB entry)
		{
			if (gbIsGlossing)  // ensure the flag below is false when there is no KB entry
				pNewPile->m_pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
			else
				pNewPile->m_pSrcPhrase->m_bHasKBEntry = FALSE; 

			// just use an empty string, or copy the sourcePhrase's key if 
			// the m_bCopySource flag is set
			if (pApp->m_bCopySource)
			{
				// whether glossing or adapting, we don't want a null source phrase
				// to initiate a copy
				if (!pNewPile->m_pSrcPhrase->m_bNullSourcePhrase)
				{
					pApp->m_targetPhrase = pView->CopySourceKey(pNewPile->m_pSrcPhrase,
											pApp->m_bUseConsistentChanges);
					bNeedModify = TRUE;
				}
				else
					pApp->m_targetPhrase.Empty();
			}
			else
				pApp->m_targetPhrase.Empty(); // this will cause pile's m_nMinWidth to be used
												// to set the m_curBoxWidth value on the view
		}
		SetValue(pApp->m_targetPhrase); //SetWindowText(pApp->m_targetPhrase); // initialize the phrase box too, so it doesn't
											  // carry an old string to the next pile's cell

		// recalculate from the beginning
		pApp->m_curBoxWidth = 2; // make very small so it doesn't push the next word/phrase
									// to the right before the next word/phrase can be measured
		pView->RecalcLayout(pApp->m_pSourcePhrases,0,pApp->m_pBundle);

		// get the new active pile (from the sequ num, since old pointers are now clobbered)
		pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);

		// update the copy held on the CPhraseBox too
		m_pActivePile = pApp->m_pActivePile;		
		
		// recreate the phraseBox using the stored information
		pApp->m_nStartChar = 0; pApp->m_nEndChar = -1;
		pApp->m_ptCurBoxLocation = pApp->m_pActivePile->m_pCell[2]->m_ptTopLeft;
		pView->RemakePhraseBox(pApp->m_pActivePile,pApp->m_targetPhrase);

		// fix the m_bSaveToKB flag, depending on whether or not srcPhrase is in kb
		if (!gbIsGlossing && !pApp->m_pActivePile->m_pSrcPhrase->m_bHasKBEntry && 
										pApp->m_pActivePile->m_pSrcPhrase->m_bNotInKB)
		{
			pApp->m_bSaveToKB = FALSE;
			pApp->m_targetPhrase.Empty();
		}
		else
		{
			pApp->m_bSaveToKB = TRUE;
		}

		// update status bar with project name
		gpApp->RefreshStatusBarInfo();

		// just invalidate the lot
		pView->Invalidate();

		if (bNeedModify)
			SetModify(TRUE); // our own SetModify(); calls MarkDirty()
		else
			SetModify(FALSE); // our own SetModify(); calls DiscardEdits();

		return TRUE;
	}
}

bool CPhraseBox::MoveToImmedNextPile(CAdapt_ItView *pView, CPile *pCurPile)
// returns TRUE if the move was successful, FALSE if not successful
// Ammended, July 2003, for auto capitalization support
{
	// store the current translation, if one exists, before moving to next pile, since each move
	// unstores the refString's translation from the KB, so they must be put back each time
	// (perhaps in edited form, if user changed the string before moving back again)
	CAdapt_ItDoc* pDoc = pView->GetDocument();
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	gbByCopyOnly = FALSE; // restore default setting

	// make sure m_targetPhrase doesn't have any final spaces
	pView->RemoveFinalSpaces(pApp->m_pTargetBox,&pApp->m_targetPhrase);

	// when adapting, we can't move to the next pile when the next pile can't be found
	CPile* pFwd = pView->GetNextPile(pCurPile);
	if (pFwd == NULL)
		return FALSE;
	bool bOK;

	// when adapting, don't move forward if it means moving to a retranslation pile
	// but we don't care when we are glossing
	if (pFwd != NULL)
	{
		if (!gbIsGlossing && pFwd->m_pSrcPhrase->m_bRetranslation)
		{
			// IDS_NO_ACCESS_TO_RETRANS
			wxMessageBox(_("Sorry, to edit or remove a retranslation you must use the toolbar buttons for those operations."), _T(""), wxICON_INFORMATION);
			pApp->m_pTargetBox->SetFocus();
			return FALSE;
		}
	}

	// if the location is a <Not In KB> one, we want to skip the store & fourth line creation
	// but first check for user incorrectly editing and fix it
	bool bNoError = TRUE;
	if (!gbIsGlossing && !pCurPile->m_pSrcPhrase->m_bHasKBEntry && pCurPile->m_pSrcPhrase->m_bNotInKB)
	{
		// in case the user edited out the <Not In KB> entry from the KB editor, we need to put
		// it back so that the setting is preserved (the "right" way to change the setting is to
		// use the toolbar checkbox)
		wxString str = _T("<Not In KB>");
		CRefString* pRefStr = pView->GetRefString(pApp->m_pKB,pCurPile->m_pSrcPhrase->m_nSrcWords,
											pCurPile->m_pSrcPhrase->m_key,str);
		if (pRefStr == NULL)
		{
			pApp->m_bSaveToKB = TRUE; // it will be off, so we must turn it back on to get 
									   // the string restored
			bool bOK;
			bOK = pView->StoreText(pApp->m_pKB,pCurPile->m_pSrcPhrase,str);
			// set the flags to ensure the asterisk shows above the pile, etc.
			pCurPile->m_pSrcPhrase->m_bHasKBEntry = FALSE;
			pCurPile->m_pSrcPhrase->m_bNotInKB = TRUE; 
		}
		
		// for version 1.4.0 and onwards, we have to permit the construction of the punctuated 
		// target string; for auto caps support, we may have to change to UC here too
		wxString str1 = pApp->m_targetPhrase;
		pView->RemovePunctuation(pDoc,&str1,1 /*from tgt*/);
		if (gbAutoCaps)
		{
			bNoError = pView->SetCaseParameters(pCurPile->m_pSrcPhrase->m_key);
			if (bNoError && gbSourceIsUpperCase && !gbMatchedKB_UCentry)
			{
				bNoError = pView->SetCaseParameters(str1,FALSE);
				if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
				{
					// a change to upper case is called for
					str1.SetChar(0,gcharNonSrcUC);
				}
			}
		}
		pCurPile->m_pSrcPhrase->m_adaption = str1;

		// the following function is now smart enough to capitalize m_targetStr in the context
		// of preceding punctuation, etc, if required. (Fixing <Not In KB> entry, so no store
		// needs to be done here.)
		pView->MakeLineFourString(pCurPile->m_pSrcPhrase,pApp->m_targetPhrase);
		goto b;
	}
	
	if (pApp->m_targetPhrase.IsEmpty())
	{
		gbUserWantsNoMove = FALSE;
		pApp->m_bForceAsk = FALSE; // make sure it's turned off, & allow function to continue
		if (gbIsGlossing)
			pCurPile->m_pSrcPhrase->m_gloss.Empty();
		else
			pCurPile->m_pSrcPhrase->m_adaption.Empty();
	}

	// make the punctuated target string, but only if adapting; note, for auto capitalization
	// ON, the function will change initial lower to upper as required, whatever punctuation
	// regime is in place for this particular sourcephrase instance
	if (!gbIsGlossing)
		pView->MakeLineFourString(pCurPile->m_pSrcPhrase,pApp->m_targetPhrase);

	// we are about to leave the current phrase box location, so we must try to store what is 
	// now in the box, if the relevant flags allow it. Test to determine which KB to store to.
	// StoreText( ) has been ammended for auto-capitalization support (July 2003)
	if (!gbIsGlossing)
		pView->RemovePunctuation(pDoc,&pApp->m_targetPhrase,1 /*from tgt*/);
	gbInhibitLine4StrCall = TRUE;
	bOK = pView->StoreText(pView->GetKB(),pCurPile->m_pSrcPhrase,pApp->m_targetPhrase,FALSE); 
	gbInhibitLine4StrCall = FALSE;
	if (!bOK)
		return FALSE; // can't move if the storage failed

	// store the current strip index, for update purposes
b:	int nCurStripIndex;
	nCurStripIndex = pCurPile->m_pStrip->m_nStripIndex;

	// move to next pile's cell
	CPile* pNewPile = pView->GetNextPile(pCurPile); // does not update the view's 
				// m_nActiveSequNum nor m_pActivePile pointer, so update these here, 
				// provided NULL was not returned

	// restore default button image, and m_bCopySourcePunctuation to TRUE
	wxCommandEvent event;
	gpApp->GetView()->OnButtonEnablePunctCopy(event);
	if (pNewPile == NULL)
	{
		if (!pApp->m_bSingleStep)
		{
			pApp->m_bAutoInsert = FALSE; // cause halt, if auto lookup & inserting is ON
		}

		// ensure the view knows the pile pointer is no longer valid
		pApp->m_pActivePile = NULL;
		pApp->m_nActiveSequNum = -1;
		return FALSE; // we are at the end of the bundle (possibly end of file too), 
					  // so can't move further in this bundle (caller should check to see if the
					  // bundle can be advanced, and do so etc. if possible)
	}
	else // we have a pointer to the next pile
	{
		pApp->m_bUserTypedSomething = FALSE; // user has not typed at the new location yet

		// update the active sequence number, and pile pointer
		pApp->m_nActiveSequNum = pNewPile->m_pSrcPhrase->m_nSequNumber;
		pApp->m_pActivePile = pNewPile;
		m_pActivePile = pNewPile; // put a copy on CPhraseBox too (we use this below)

		// since we are moving forward from could be anywhere, the next pile is may have a 
		// refString translation which is nonempty, so we have to put it into m_targetPhrase so
		// that ResizeBox will use it; but if there is none, the copy the source key if the
		// m_bCopySource flag is set, else just set it to an empty string. (bNeed Modify is a 
		// helper flag used for setting/clearing the document's modified flag at the end of this
		// function)
		bool bNeedModify = FALSE; // reset to TRUE if we copy source because there was no 
								  // adaptation

		// beware, next call the pRefString pointer may point to <Not In KB>, so take that into
		// account; GetRefString has been modified for auto-capitalization support
		CRefString* pRefString;
		if (gbIsGlossing)
			pRefString = pView->GetRefString(gpApp->m_pGlossingKB, 1,
				pNewPile->m_pSrcPhrase->m_key, pNewPile->m_pSrcPhrase->m_gloss);
		else
			pRefString = pView->GetRefString(gpApp->m_pKB, pNewPile->m_pSrcPhrase->m_nSrcWords,
				pNewPile->m_pSrcPhrase->m_key, pNewPile->m_pSrcPhrase->m_adaption);

		if (pRefString != NULL)
		{
			pView->RemoveSelection(); // we won't do merges in this situation
			
			// assign the translation text - but check it's not "<Not In KB>", if it is, we 
			// leave the phrase box unchanged (rather than empty as formerly), but turn OFF
			// the m_bSaveToKB flag 
			wxString str = pRefString->m_translation;
			if (!gbIsGlossing && str == _T("<Not In KB>"))
			{
				pApp->m_bSaveToKB = FALSE;
				pApp->m_pTargetBox->m_bAbandonable = FALSE; 
				pNewPile->m_pSrcPhrase->m_bHasKBEntry = FALSE; // ensures * shows above 
															   // this srcPhrase
				pApp->m_targetPhrase = pNewPile->m_pSrcPhrase->m_targetStr;
				pNewPile->m_pSrcPhrase->m_bNotInKB = TRUE;
			}
			else
			{
				pApp->m_pTargetBox->m_bAbandonable = FALSE; 
				pApp->m_targetPhrase = str;
			}

			// remove the translation from the KB, in case user wants to edit it before it's 
			// stored again (RemoveRefString also clears the m_bHasKBEntry flag on the source
			// phrase)
			if (gbIsGlossing)
			{
				pView->RemoveRefString(pRefString, pNewPile->m_pSrcPhrase, 1);
				pApp->m_targetPhrase = pNewPile->m_pSrcPhrase->m_gloss;
			}
			else
			{
				pView->RemoveRefString(pRefString,pNewPile->m_pSrcPhrase,
														pNewPile->m_pSrcPhrase->m_nSrcWords);
				pApp->m_targetPhrase = pNewPile->m_pSrcPhrase->m_targetStr;
			}
		}
		else // no kb entry
		{
			if (gbIsGlossing)
			{
				pNewPile->m_pSrcPhrase->m_bHasGlossingKBEntry = FALSE; // ensure it's false when
																	   // there is no KB entry
			}
			else
			{
				pNewPile->m_pSrcPhrase->m_bHasKBEntry = FALSE; // ensure it's false when there is
															   // no KB entry
			}
			// try to get a suitable string instead from the sourcephrase itself, if that fails
			// then copy the sourcePhrase's key if the m_bCopySource flag is set
			if (gbIsGlossing)
			{
				pApp->m_targetPhrase = pNewPile->m_pSrcPhrase->m_gloss;
			}
			else
			{
				pApp->m_targetPhrase = pNewPile->m_pSrcPhrase->m_adaption;
			}
			if (pApp->m_targetPhrase.IsEmpty() && pApp->m_bCopySource)
			{
				if (!pNewPile->m_pSrcPhrase->m_bNullSourcePhrase)
				{
					pApp->m_targetPhrase = pView->CopySourceKey(pNewPile->m_pSrcPhrase,
															pApp->m_bUseConsistentChanges);
					bNeedModify = TRUE;
				}
				else
				{
					pApp->m_targetPhrase.Empty();
				}
			}
		}
		SetValue(pApp->m_targetPhrase); // initialize the phrase box too, so it doesn't
											  // carry an old string to the next pile's cell

		int nSNSaved = pNewPile->m_pSrcPhrase->m_nSequNumber; // save the sequ number, so we can
										 // get the pile pointer after the recalculate of layout
		// recalculate from the beginning
		pApp->m_curBoxWidth = 2; // make very small so it doesn't push the next word/phrase to
								  // the right before the next word/phrase can be measured
		pView->RecalcLayout(pApp->m_pSourcePhrases,0,pApp->m_pBundle);

		// pointers will have been clobbered, so get pNewPile again for the new layout
		pNewPile = pView->GetPile(nSNSaved);
		wxASSERT(pNewPile);

		if (pNewPile->m_nPileIndex == 0) // did we move to the start of next strip?
		{
			// moved to start of next strip, so scroll
			pApp->GetMainFrame()->canvas->ScrollUp(1);
		}

		// get the new active pile (from the sequ num, since old pointers are now clobbered)
		pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);

		// update the copy held on the CPhraseBox too
		m_pActivePile = pApp->m_pActivePile;		
		
		// recreate the phraseBox using the stored information
		pApp->m_nStartChar = 0; pApp->m_nEndChar = -1;
		pApp->m_ptCurBoxLocation = pApp->m_pActivePile->m_pCell[2]->m_ptTopLeft;
		pView->RemakePhraseBox(pApp->m_pActivePile,pApp->m_targetPhrase);

		// fix the m_bSaveToKB flag, depending on whether or not srcPhrase is in kb
		if (!gbIsGlossing && !pApp->m_pActivePile->m_pSrcPhrase->m_bHasKBEntry && 
										pApp->m_pActivePile->m_pSrcPhrase->m_bNotInKB)
		{
			pApp->m_bSaveToKB = FALSE;
		}
		else
		{
			pApp->m_bSaveToKB = TRUE;
		}

		// whm Note: In BEW's modifications of 20Dec07 he commented out the following
		// Invalidate call with the following comment:
		// invalidate the view's layout (doesn't invalidate the phrase box though)
		//pView->Invalidate();

		if (bNeedModify)
			SetModify(TRUE); // our own SetModify(); calls MarkDirty()
		else
			SetModify(FALSE); // our own SetModify(); calls DiscardEdits()

		return TRUE;
	}
}

void CPhraseBox::OnSysKeyUp(wxKeyEvent& event) 
{
	// wx Note: This routine handles Adapt It's AltDown key events
	// and CmdDown events (= ControlDown on PCs; Apple Command key events on Macs).
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));
	long nStart;
	long nEnd;

	bool bTRUE = FALSE;
	if (event.AltDown())// || event.CmdDown()) // CmdDown is same as ControlDown on PCs; Apple Command key on Macs.
	{
		// ALT key or Control/Command key is down
		if (event.AltDown() && event.GetKeyCode() == WXK_RETURN)
		{
			// ALT key is down, and <RET> was nChar typed (ie. 13), so invoke the
			// code to turn selection into a phrase; but if glossing is ON, then
			// merging must not happen - in which case exit early
			if (gbIsGlossing || !(pApp->m_selection.GetCount() > 1))
				return;
			pView->MergeWords();

			// select the lot
			SetSelection(-1,-1);// -1,-1 selects all
			gnStart = 0;
			gnEnd = -1;

			// set old sequ number in case required for toolbar's Back button - in this case
			// it may have been a location which no longer exists because it was absorbed in
			// the merge, so set it to the merged phrase itself
			gnOldSequNum = pApp->m_nActiveSequNum;
			return; 
		}

		// BEW added 19Apr06 to allow Bob Eaton to advance phrasebox without having any lookup of the KB done.
		// We want this to be done only for adapting mode, and provided transliteration mode is turned on
		if (!gbIsGlossing && gpApp->m_bTransliterationMode && event.GetKeyCode() == WXK_BACK) //nChar == VK_BACK)
		{
			// save old sequ number in case required for toolbar's Back button
			gnOldSequNum = gpApp->m_nActiveSequNum;

			gbSuppressStoreForAltBackspaceKeypress = TRUE; // suppress store to KB for this move of box
			// the value is restored to FALSE in MoveToImmediateNextPile()
			
			// do the move forward to next empty pile, with lookup etc, but no store due to 
			// the gbSuppressStoreForAltBackspaceKeypress global being TRUE until the StoreText()
			// call is jumped over in the MoveToNextPile() call within JumpForward()
			JumpForward(pView);
			return;
		}
		else if (!gbIsGlossing && !gpApp->m_bTransliterationMode && event.GetKeyCode() == WXK_BACK)
		{
			// Alt key down & Backspace key hit, so user wanted to initiate a transliteration 
			// advance of the phrase box, with its special KB storage mode, but forgot to turn the 
			// transliteration mode on before using this feature, so warn him to turn it on and then do nothing
			//IDS_TRANSLITERATE_OFF
			wxMessageBox(_("Transliteration mode is not yet turned on."),_T(""),wxICON_WARNING);

			// restore focus to the phrase box
			if (pApp->m_pTargetBox != NULL)
				if (pApp->m_pTargetBox->IsShown())
					pApp->m_pTargetBox->SetFocus();
			return;
		}

		// ALT key is down, if an arrow key pressed, extend/retract sel'n left or right
		// or insert a null source phrase, or open the retranslation dialog; but don't
		// allow any of this (except Alt + Backspace) if glossing is ON - in those cases, just return
		if (gbIsGlossing)
			return;
		GetSelection(&nStart,&nEnd);
		if (event.GetKeyCode() == WXK_RIGHT)
		{
			if (gbRTL_Layout)
				bTRUE = pView->ExtendSelectionLeft();
			else
				bTRUE = pView->ExtendSelectionRight();
			if(!bTRUE)
			{
				// did not succeed - do something eg. warn user he's collided with a boundary
				// IDS_RIGHT_EXTEND_FAIL
				wxMessageBox(_("Sorry, you cannot extend the selection that far to the right unless you also use one of the techniques for ignoring boundaries."),_T(""), wxICON_INFORMATION);
			}
			SetFocus();
			SetSelection(nStart,nEnd);
			gnStart = nStart;
			gnEnd = nEnd;
		}
		else if (event.GetKeyCode() == WXK_LEFT)
		{
			if (gbRTL_Layout)
				bTRUE = pView->ExtendSelectionRight();
			else
				bTRUE = pView->ExtendSelectionLeft();
			if(!bTRUE)
			{
				// did not succeed, so warn user
				// IDS_LEFT_EXTEND_FAIL
				wxMessageBox(_("Sorry, you cannot extend the selection that far to the left unless you also use one of the techniques for ignoring boundaries. "), _T(""), wxICON_INFORMATION);
			}
			SetFocus();
			SetSelection(nStart,nEnd);
			gnStart = nStart;
			gnEnd = nEnd;
		}
		else if (event.GetKeyCode() == WXK_UP)
		{
			// up arrow was pressed, so get the retranslation dialog open
			if (pApp->m_pActivePile == NULL)
			{
				goto a;
			}
			if (pApp->m_selectionLine != -1)
			{
				// if there is at least one source phrase with m_bRetranslation == TRUE, 
				// then then use the selection and put up the dialog
				pView->DoRetranslationByUpArrow();
			}
			else
			{
				// no selection, so make a selection at the phrase box and invoke the
				// retranslation dialog on it
				CCell* pCell = pApp->m_pActivePile->m_pCell[2];
				wxASSERT(pCell);
				pApp->m_selection.Append(pCell);
				pApp->m_selectionLine = 1;
				wxUpdateUIEvent evt;
				//pView->OnUpdateButtonRetranslation(evt);
				// whm Note: calling OnUpdateButtonRetranslation(evt) here doesn't work because there
				// is not enough idle time for the Do A Retranslation toolbar button to be enabled
				// before the DoRetranslationByUpArrow() call below executes - which has code in it
				// to prevent the Retranslation dialog from poping up unless the toolbar button is
				// actually enabled. So, we explicitly enable the toolbar button here instead of
				// waiting for it to be done in idle time.
				CMainFrame* pFrame = pApp->GetMainFrame();
				wxToolBarBase* pToolBar = pFrame->GetToolBar();
				wxASSERT(pToolBar != NULL);
				pToolBar->EnableTool(ID_BUTTON_RETRANSLATION,TRUE);
				pView->DoRetranslationByUpArrow();
			}
		}
		else if (event.GetKeyCode() == WXK_DOWN)
		{
			// down arrow was pressed, (& CTRL key is not pressed) so do insert of null 
			// sourcephrase but first save old sequ number in case required for toolbar's 
			// Back button (this one is activated when CTRL key is not down, so it does the
			// default "insert before" case; the "insert after" case is done in the 
			// OnKeyUp() handler)

			// Bill wanted the behaviour modified, so that if the box's m_bAbandonable flag is TRUE
			// (ie. a copy of source text was done and nothing typed yet) then the current pile
			// would have the box contents abandoned, nothing put in the KB, and then the placeholder
			// inserion - the advantage of this is that if the placeholder is inserted immediately
			// before the phrasebox's location, then after the placeholder text is typed and the user
			// hits ENTER to continue looking ahead, the former box location will get the box and the
			// copy of the source redone, rather than the user usually having to edit out an unwanted
			// copy from the KB, or remember to clear the box manually. A sufficient thing to do here
			// is just to clear the box's contents.
			if (pApp->m_pTargetBox->m_bAbandonable)
			{
				pApp->m_targetPhrase.Empty();
				if (pApp->m_pTargetBox != NULL 
					&& (pApp->m_pTargetBox->IsShown()))
				{
					pApp->m_pTargetBox->SetValue(_T(""));
				}
			}

			// now do the 'insert before'
			gnOldSequNum = pApp->m_nActiveSequNum;
			pView->InsertNullSrcPhraseBefore();
		}
		// BEW added 26Sep05, to implement Roland Fumey's request that the shortcut for unmerging
		// not be SHIFT+End as in the legacy app, but something else; so I'll make it ALT+Delete
		// and then SHIFT+End can be used for extending the selection in the phrase box's CEdit 
		// to the end of the box contents (which is Windows standard behaviour, & what Roland wants)
		if (event.GetKeyCode() == WXK_DELETE)
		{
			// we have ALT + Delete keys held down, so unmerge the current merger - separating into 
			// individual words but only when adapting, if glossing we instead just return
			CSourcePhrase* pSP;
			if (gpApp->m_pActivePile == NULL)
			{
				return;
			}
			if (gpApp->m_selectionLine != -1 && gpApp->m_selection.GetCount() == 1)
			{
				CCellList::Node* cpos = gpApp->m_selection.GetFirst();
				CCell* pCell = cpos->GetData();
				pSP = pCell->m_pPile->m_pSrcPhrase;
				if (pSP->m_nSrcWords == 1)
					return;
			}
			else if (gpApp->m_selectionLine == -1 && gpApp->m_pTargetBox->GetHandle() != NULL 
									&& gpApp->m_pTargetBox->IsShown())
			{
				pSP = gpApp->m_pActivePile->m_pSrcPhrase;
				if (pSP->m_nSrcWords == 1)
					return;
			}
			else
			{
				return;
			}

			// if we get to here, we can go ahead & remove the merger, if we are adapting
			// but if glossing, the user should be explicitly warned the op is no available
			if (gbIsGlossing)
			{
				// IDS_NOT_WHEN_GLOSSING
				wxMessageBox(_("This particular operation is not available when you are glossing."),_T(""),wxICON_INFORMATION);
				return;
			}
			pView->UnmergePhrase(); // calls OnButtonRestore() - which will attempt to do a 
				// lookup, so don't remake the phrase box with the global (CString) gSaveTargetPhrase,
				// otherwise it will override a successful lookkup and make the ALT+Delete give
				// a different result than the Unmerge button on the toolbar. So we in effect
				// are ignoring gSaveTargetPhrase (the former is used in PlacePhraseBox() only

			// save old sequ number in case required for toolbar's Back button - the only safe
			// value is the first pile of the unmerged phrase, which is where the phrase box 
			// now is
			gnOldSequNum = gpApp->m_nActiveSequNum;
		}
	}
	
a:	;//event.Skip(); //CEdit::OnSysKeyUp(nChar, nRepCnt, nFlags);
}

bool CPhraseBox::OnePass(CAdapt_ItView *pView)
// return TRUE if we traverse this function without being at the end of the file, or failing in
// the LookAhead function (such as when there was no match); otherwise, return FALSE so as to
// be able to exit from the caller's loop
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	// save old sequ number in case required for toolbar's Back button
	gnOldSequNum = pApp->m_nActiveSequNum;
	gbByCopyOnly = FALSE; // restore default setting

	gbBundleChanged = FALSE; // default
	// we have to check here if we have moved into the area where it is necessary to advance
	// the bundle, and if so, then do it, & update everything again
	bool bAdvance = pView->NeedBundleAdvance(pApp->m_nActiveSequNum);
	if (bAdvance)
	{
		// do the advance, return a new (valid) pointer to the active pile
		pApp->m_pActivePile = pView->AdvanceBundle(pApp->m_nActiveSequNum);
		m_pActivePile = pApp->m_pActivePile; // put copy on the CPhraseBox too
		gbBundleChanged = TRUE;
	}
	
	// do the move, but first preserve which strip we were in, in case we jump to next 
	// strip we'll have to update the old one too
	int nOldStripIndex = pApp->m_pActivePile->m_pStrip->m_nStripIndex;

	int bSuccessful = MoveToNextPile(pView,pApp->m_pActivePile);
	if (!bSuccessful)
	{
		// it may have failed not because we are at eof, but because there is nothing
		// in this present bundle at higher locations which lacks an adaptation (or gloss)
		// in which case the active pile pointer returned will be null, and so we must
		// jump to an empty pile, or to eof if there are none
		if (pApp->m_pActivePile == NULL && pApp->m_endIndex < pApp->m_maxIndex)
		{
			gbBundleChanged = TRUE;

			// hunt for an empty srcPhrase
			CSourcePhrase* pSrcPhrase = pView->GetNextEmptySrcPhrase(pApp->m_endIndex);
			if (pSrcPhrase == NULL)
			{
				// we got to the end of the doc
				pApp->m_curIndex = pApp->m_endIndex = pApp->m_maxIndex;
				pApp->m_nActiveSequNum = -1;
				goto n;
			}
			else
			{
				bSuccessful = TRUE; // remove non-success condition, since we found an empty one
				pApp->m_nActiveSequNum = pSrcPhrase->m_nSequNumber;
				pView->Jump(&wxGetApp(),pSrcPhrase);
				goto m;
			}
		}

		// if we get here, we should be at the end
n:		if ((pApp->m_curIndex == pApp->m_endIndex && pApp->m_endIndex == pApp->m_maxIndex)
															|| pApp->m_nActiveSequNum == -1)
		{

			// At the end, we'll reset the globals to turn off any highlight
			gnBeginInsertionsSequNum = -1;
			gnEndInsertionsSequNum = -1;
			pView->Invalidate(); // remove highlight before MessageBox call below

			// tell the user EOF has been reached
			gbCameToEnd = TRUE;
			wxStatusBar* pStatusBar;
			CMainFrame* pFrame = (CMainFrame*)pView->GetFrame();
			if (pFrame != NULL)
			{
				pStatusBar = ((CMainFrame*)pFrame)->m_pStatusBar;
				wxASSERT(pStatusBar != NULL);
				wxString str;
				if (gbIsGlossing)
					//IDS_FINISHED_GLOSSING
					str = _("End of the file; nothing more to gloss.");
				else
					//IDS_FINISHED_ADAPTING
					str = _("End of the file; nothing more to adapt.");
				pStatusBar->SetStatusText(str,0);
			}
			// we are at EOF, so set up safe end conditions
			pApp->m_pTargetBox->Hide(); // whm added 12Sep04
			pApp->m_pTargetBox->Enable(FALSE); // whm added 12Sep04
			pApp->m_targetPhrase.Empty();
			pApp->m_nActiveSequNum = pApp->m_curIndex = -1;
			// wxWidgets version Hides the target box rather than destroying it
			pApp->m_pTargetBox->Hide();
			pApp->m_pTargetBox->SetValue(_T(""));
			pView->LayoutStrip(pApp->m_pSourcePhrases,nOldStripIndex,
																		pApp->m_pBundle);
			pApp->m_pActivePile = (CPile*)NULL; // can use this as a flag for at-EOF condition too
			pView->Invalidate();
		}
		else
		{
			pApp->m_pTargetBox->SetFocus();
		}
		translation.Empty(); // clear the static string storage for the translation (or gloss)
		// save the phrase box's text, in case user hits SHIFT+END to unmerge a phrase
		gSaveTargetPhrase = pApp->m_targetPhrase;
		pApp->m_bAutoInsert = FALSE; // ensure we halt for user to 
															 // type transl'n
		return FALSE; // must have been a null string, or at EOF;
	}

m:	pApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);
	int nCurStripIndex = pApp->m_pActivePile->m_pStrip->m_nStripIndex;

	// if the old strip is not same as the new one, relayout the old one too
	if (nCurStripIndex != nOldStripIndex && !gbBundleChanged)
	{
		pView->LayoutStrip(pApp->m_pSourcePhrases,nOldStripIndex,
																	pApp->m_pBundle);
	}
	// recreate the box (BEW added 06Jun06, because when the box moves to start of new line
	// if fails to show if program counter comes through this block; similarly at a detached
	// punctuation CSourcePhrase instance at the end of the doc (with key empty); so we really
	// need to unlaterally do this box creation here and the invalidate
	// WX Note: ResizeBox() doesn't recreate the box, but cause it to be visible again
	pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
	wxASSERT(pApp->m_pActivePile);
	pApp->m_pTargetBox->m_pActivePile = pApp->m_pActivePile; // put copy in the CPhraseBox too
	pApp->m_ptCurBoxLocation = pApp->m_pActivePile->m_pCell[2]->m_ptTopLeft;
	pApp->m_curBoxWidth = pView->RecalcPhraseBoxWidth(pApp->m_targetPhrase);
	pApp->m_nCurPileMinWidth = pApp->m_curBoxWidth;
	if (gbIsGlossing && gbGlossingUsesNavFont)
	{
		// wx Note: ResizeBox doesn't recreate the box; it just calls SetSize and causes it to be visible again
		pView->ResizeBox(&pApp->m_ptCurBoxLocation,pApp->m_curBoxWidth,pApp->m_nNavTextHeight,
					pApp->m_targetPhrase,pApp->m_nStartChar,pApp->m_nEndChar,pApp->m_pActivePile);
		// set the color - CPhraseBox has a color variable & uses reflected notification
		pApp->m_pTargetBox->m_textColor = gpApp->m_navTextColor; // MFC uses navTextColor;
	}
	else
	{
		// wx Note: ResizeBox doesn't recreate the box; it just calls SetSize and causes it to be visible again
		pView->ResizeBox(&pApp->m_ptCurBoxLocation,pApp->m_curBoxWidth,pApp->m_nTgtHeight,
				pApp->m_targetPhrase,pApp->m_nStartChar,pApp->m_nEndChar,pApp->m_pActivePile);
		// set the color - CPhraseBox has a color variable & uses reflected notification
		pApp->m_pTargetBox->m_textColor = gpApp->m_targetColor; 
	}
	pView->Invalidate(); // refreshes the canvas

	if (!bSuccessful)
	{	
		// we have come to the end of the bundle, probably to the end of the file's data, so
		// we must here determine if we can advance the bundle - and do it if we can (and
		// place the phrase box in its first pile), else we are at the end of the data and
		// can't move further so just tell the user there is no more data to adapt in this file.
		if (pApp->m_pActivePile == NULL || pApp->m_nActiveSequNum == -1)
		{

			// At the end, we'll reset the globals to turn off any highlight
			gnBeginInsertionsSequNum = -1;
			gnEndInsertionsSequNum = -1;
			pView->Invalidate(); // remove highlight before MessageBox call below

			// tell the user EOF has been reached
			gbCameToEnd = TRUE;
			wxStatusBar* pStatusBar;
			CMainFrame* pFrame = (CMainFrame*)pView->GetFrame();
			if (pFrame != NULL)
			{
				pStatusBar = ((CMainFrame*)pFrame)->m_pStatusBar;
				wxASSERT(pStatusBar != NULL);
				wxString str;
				if (gbIsGlossing)
					//IDS_FINISHED_GLOSSING
					str = _("End of the file; nothing more to gloss.");
				else
					//IDS_FINISHED_ADAPTING
					str = _("End of the file; nothing more to adapt.");
				pStatusBar->SetStatusText(str,0);
			}
			// we are at the end of the file's data, so remove the phraseBox and set the
			// sequence number and pApp->m_curIndex to -1, as two flags for this at-EOF condition
			pApp->m_targetPhrase.Empty();
			pApp->m_nActiveSequNum = pApp->m_curIndex = -1;
			pApp->m_pTargetBox->SetValue(_T(""));
			pApp->m_pTargetBox->Hide(); // whm added 12Sep04
			pApp->m_pTargetBox->Enable(FALSE); // whm added 12Sep04 
			if (!gbBundleChanged)
				pView->LayoutStrip(pApp->m_pSourcePhrases,nOldStripIndex,
																		pApp->m_pBundle);
			pApp->m_pActivePile = (CPile*)NULL; // can use this as a flag for at-EOF condition too
			pView->Invalidate();
		}
		return FALSE; // must return, or we'll hang or crash
	}
	gbEnterTyped = TRUE; // keep it continuing to use the faster GetSrcPhrasePos() call in
						 // BuildPhrases()
	return TRUE; // all was okay
}

// This OnKeyUp function is called via the EVT_KEY_UP event in our CPhraseBox
// Event Table.
void CPhraseBox::OnKeyUp(wxKeyEvent& event)
{
	//wxLogDebug(_T("OnKeyUp() %d called from PhraseBox"),event.GetKeyCode());
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));

	// wxWidgets doesn't have a separate OnSysKeyUp() virtual method
	// so we'll simply detect if the ALT key was down and call the
	// OnSysKeyUp() method from here
	if (event.AltDown())// || event.CmdDown()) // CmdDown() is same as ControlDown on PC, and Apple Command key on Macs.
	{
		OnSysKeyUp(event);
		return;
	}

	// version 1.4.2 and onwards, we want a right or left arrow used to remove the
	// phrasebox's selection to be considered a typed character, so that if a subsequent
	// selection and merge is done then the first target word will not get lost; and so
	// we add a block of code also to start of OnChar( ) to check for entry with both
	// m_bAbandonable and m_bUserTypedSomething set TRUE, in which case we don't
	// clear these flags (the older versions cleared the flags on entry to OnChar( ) )

	// we use this flag cocktail to test for these values of the three flags as the
	// condition for telling the application to retain the phrase box's contents
	// when user deselects target word, then makes a phrase and merges by typing.
	// (When glossing, we don't need to do anything here - suppressions handled elsewhere)
	if (event.GetKeyCode() == WXK_RIGHT)
	{
		m_bAbandonable = FALSE;
		pApp->m_bUserTypedSomething = TRUE;
		gbRetainBoxContents = TRUE;
		event.Skip(); //CEdit::OnKeyUp(nChar, nRepCnt, nFlags);
	}
	else if (event.GetKeyCode() == WXK_LEFT)
	{
		m_bAbandonable = FALSE;
		pApp->m_bUserTypedSomething = TRUE;
		gbRetainBoxContents = TRUE;
		event.Skip(); //CEdit::OnKeyUp(nChar, nRepCnt, nFlags);
	}

	// does the user want to force the Choose Translation dialog open?
	if (event.GetKeyCode() == WXK_F8)
	{
		pView->ChooseTranslation();
		return;
	}

	// does the user want to open the retranslation dialog in order to make one?
	if (event.GetKeyCode() == WXK_F9)
	{
		if (gbIsGlossing)
			return;
		else
			pView->NewRetranslation();
		return;
	}

	// does user want to unmerge a phrase?
	/* //BEW removed 26Sep05 to implement Roland Fumey's request that SHIFT + End be retained with
	//the default Windows behaviour of extending a selection to the end of a field or CEdit, so I've
	//made the accelerator for an unmerge in version 3 be ALT+Delete
	if (nChar == VK_END)
	{
		short keyState = ::GetKeyState(VK_SHIFT);
		if (keyState < 0)
		{


		}
		return;
	}
	*/

	//SHORT ctrlKeyState;

	// user did not want to unmerge, so must want a scroll
	// preserve cursor location (its already been moved by the time this function is entered)
	// so I've done it with 2 globals, gnStart and gnEnd, and handlers for LButtonDown and Up etc.
	int nScrollCount = 1;

	// the code below for up arrow or down arrow assumes a one strip + leading scroll. If we 
	// change so as to scroll more than one strip at a time (as when pressing page up or page 
	// down keys), iterate by the number of strips needing to be scrolled 
	int yDist = pApp->m_curPileHeight + pApp->m_curLeading;

	// do the scroll (CEdit also moves cursor one place to left for uparrow, right for downarrow
	// hence the need to restore the cursor afterwards; however, the values of nStart and nEnd
	// depend on whether the app made the selection, or the user; and also on whether up or down
	// was pressed: for down arrow, the values will be either 1,1 or both == length of line, for
	// up arrow, values will be either both = length of line -1; or 0,0   so code accordingly
	bool bScrollFinished = FALSE;
	int nCurrentStrip;
	int nLastStrip;
	if (event.GetKeyCode() == WXK_UP)
	{
a:		int xPixelsPerUnit,yPixelsPerUnit;
		pApp->GetMainFrame()->canvas->GetScrollPixelsPerUnit(&xPixelsPerUnit,&yPixelsPerUnit);
		wxPoint scrollPos;
		// MFC's GetScrollPosition() "gets the location in the document to which the upper
		// left corner of the view has been scrolled. It returns values in logical units."
		// wx note: The wx docs only say of GetScrollPos(), that it "Returns the built-in scrollbar position."
		// I assume this means it gets the logical position of the upper left corner, but it is in scroll 
		// units which need to be converted to device (pixel) units
		//scrollPos.x = pApp->GetMainFrame()->canvas->GetScrollPos(wxHORIZONTAL); //wxPoint scrollPos = GetScrollPosition();
		//scrollPos.y = pApp->GetMainFrame()->canvas->GetScrollPos(wxVERTICAL); //wxPoint scrollPos = GetScrollPosition();
		//scrollPos.x *= xPixelsPerUnit; wxASSERT(scrollPos.x == 0);
		//scrollPos.y *= yPixelsPerUnit;
			
//#ifdef _DEBUG
//		int xOrigin, yOrigin;
//		// the view start is effectively the scroll position, but GetViewStart returns scroll units
//		pApp->GetMainFrame()->canvas->GetViewStart(&xOrigin, &yOrigin); // gets xOrigin and yOrigin in scroll units
//		xOrigin = xOrigin * xPixelsPerUnit; // number pixels is ScrollUnits * pixelsPerScrollUnit
//		yOrigin = yOrigin * yPixelsPerUnit;
//		wxASSERT(xOrigin == scrollPos.x);
//		wxASSERT(yOrigin == scrollPos.y);
//#endif

		gpApp->GetMainFrame()->canvas->CalcUnscrolledPosition(0,0,&scrollPos.x,&scrollPos.y);
		// the scrollPos point is now in logical pixels from the start of the doc
		
		if (scrollPos.y > 0)
		{
			if (scrollPos.y >= yDist)
			{
				// up uparrow was pressed, so scroll up a strip, provided we are not at the 
				// start of the bundle; and we are more than one strip + leading from the start,
				// so it is safe to scroll
				pApp->GetMainFrame()->canvas->ScrollUp(1);
			}
			else
			{
				// we are close to the start of the bundle, but not a full strip plus leading, 
				// so do a partial scroll only - otherwise phrase box will be put at the wrong
				// place
				yDist = scrollPos.y;
				scrollPos.y -= yDist;

				int posn = scrollPos.y;
				posn = posn / yPixelsPerUnit;
				//pApp->GetMainFrame()->canvas->SetScrollPos(wxVERTICAL,posn,TRUE); //pView->SetScrollPos(SB_VERT,scrollPos.y,TRUE);
				// Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount
				// to scroll in device units (pixels). The equivalent in wx is Scroll(x,y) in which x and y are in
				// SCROLL UNITS (pixels divided by pixels per unit). Also MFC's ScrollWindow takes parameters whose value
				// represents an "amount" to scroll from the current position, whereas the wxScrolledWindow::Scroll
				// takes parameters which represent an absolute "position" in scroll units. To convert the
				// amount we need to add the amount to (or subtract from if negative) the logical pixel unit
				// of the upper left point of the client viewing area; then convert to scroll units in Scroll().
				pApp->GetMainFrame()->canvas->Scroll(0,posn); //pView->ScrollWindow(0,yDist);
				Refresh();
				bScrollFinished = TRUE;
			}
		}
		else
		{
			::wxBell();
			bScrollFinished = TRUE;
		}

		if (bScrollFinished)
			goto c;
		else
		{
			--nScrollCount;
			if (nScrollCount == 0)
				goto c;
			else
			{
				goto a;
			}
		}

		// restore cursor location when done
c:		SetFocus();
		SetSelection(gnStart,gnEnd);
		return;
	}
	else if (event.GetKeyCode() == WXK_DOWN)
	{
		if (event.ControlDown()) 
		{
			// CTRL + down arrow was pressed - asking for an "insert after" of a null srcphrase
			// (ALT key is ignored, so CTRL + ALT + down arrow also gives the same result)
			// first save old sequ number in case required for toolbar's Back button
			// If glossing is ON, we don't allow the insertion, and just return instead
			gnOldSequNum = pApp->m_nActiveSequNum;
			if (!gbIsGlossing)
				pView->InsertNullSrcPhraseAfter();
			return;
		}

		// down arrow was pressed, so scroll down a strip, provided we are not at the end of
		// the bundle
b:		wxPoint scrollPos;
		int xPixelsPerUnit,yPixelsPerUnit;
		pApp->GetMainFrame()->canvas->GetScrollPixelsPerUnit(&xPixelsPerUnit,&yPixelsPerUnit);
		// MFC's GetScrollPosition() "gets the location in the document to which the upper
		// left corner of the view has been scrolled. It returns values in logical units."
		// wx note: The wx docs only say of GetScrollPos(), that it "Returns the built-in scrollbar position."
		// I assume this means it gets the logical position of the upper left corner, but it is in scroll 
		// units which need to be converted to device (pixel) units
		//scrollPos.x = pApp->GetMainFrame()->canvas->GetScrollPos(wxHORIZONTAL); //wxPoint scrollPos = GetScrollPosition();
		//scrollPos.y = pApp->GetMainFrame()->canvas->GetScrollPos(wxVERTICAL); //wxPoint scrollPos = GetScrollPosition();
		//scrollPos.x *= xPixelsPerUnit; wxASSERT(scrollPos.x == 0);
		//scrollPos.y *= yPixelsPerUnit;
			
//#ifdef _DEBUG
//		int xOrigin, yOrigin;
//		// the view start is effectively the scroll position, but GetViewStart returns scroll units
//		gpApp->GetMainFrame()->canvas->GetViewStart(&xOrigin, &yOrigin); // gets xOrigin and yOrigin in scroll units
//		xOrigin = xOrigin * xPixelsPerUnit; // number pixels is ScrollUnits * pixelsPerScrollUnit
//		yOrigin = yOrigin * yPixelsPerUnit;
//		wxASSERT(xOrigin == scrollPos.x); // scrollPos.x = xOrigin;
//		wxASSERT(yOrigin == scrollPos.y); // scrollPos.y = yOrigin;
//#endif

		//CSize barSizes = pView->GetTotalSize();
		// MFC's GetTotalSize() gets "the total size of the scroll view in logical
		// units." WxWidgets has GetVirtualSize() which gets the size in device units (pixels).
		wxSize maxDocSize; // renaming barSizes to maxDocSize for clarity
		pApp->GetMainFrame()->canvas->GetVirtualSize(&maxDocSize.x,&maxDocSize.y); // gets size in pixels

		// Can't use dc.DeviceToLogicalX because OnPrepareDC not called
		//wxClientDC dc(pApp->GetMainFrame()->canvas);
		//dc.DeviceToLogicalX(scrViewSizeX); // now logical coords
		//dc.DeviceToLogicalY(scrViewSizeY);

		gpApp->GetMainFrame()->canvas->CalcUnscrolledPosition(0,0,&scrollPos.x,&scrollPos.y);
		// the scrollPos point is now in logical pixels from the start of the doc
		
		wxRect rectClient(0,0,0,0);
		wxSize canvasSize;
		canvasSize = pApp->GetMainFrame()->GetCanvasClientSize();
		rectClient.width = canvasSize.x;
		rectClient.height = canvasSize.y;
		
		int logicalViewBottom = scrollPos.y + rectClient.GetBottom();		// rectClient.top is always 0
		if (logicalViewBottom < maxDocSize.GetHeight())
		{
			if (logicalViewBottom <= maxDocSize.GetHeight() - yDist)
			{
				// a full strip + leading can be scrolled safely
				pApp->GetMainFrame()->canvas->ScrollDown(1);
				//// what ScrollDown does could be done with the following 5 lines:
				//scrollPos.y += yDist; // yDist is here exactly one strip (including leading)
				//int posn = scrollPos.y;
				//posn = posn / yPixelsPerUnit;
				//gpApp->GetMainFrame()->canvas->Scroll(0,posn);
				//gpApp->GetMainFrame()->canvas->Refresh(); // ScrollDown(1) calls Refresh - needed?
			}
			else
			{
				// we are close to the end, but not a full strip + leading can be scrolled, so
				// just scroll enough to reach the end - otherwise position of phrase box will
				// be set wrongly
				wxASSERT(maxDocSize.GetHeight() >= logicalViewBottom);
				yDist = maxDocSize.GetHeight() - logicalViewBottom; // make yDist be what's left to scroll
				scrollPos.y += yDist;

				int posn = scrollPos.y;
				posn = posn / yPixelsPerUnit;
				//pApp->GetMainFrame()->canvas->SetScrollPos(wxVERTICAL,posn,TRUE); //pView->SetScrollPos(SB_VERT,scrollPos.y,TRUE);
				// Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount
				// to scroll in device units (pixels). The equivalent in wx is Scroll(x,y) in which x and y are in
				// SCROLL UNITS (pixels divided by pixels per unit). Also MFC's ScrollWindow takes parameters whose value
				// represents an "amount" to scroll from the current position, whereas the wxScrolledWindow::Scroll
				// takes parameters which represent an absolute "position" in scroll units. To convert the
				// amount we need to add the amount to (or subtract from if negative) the logical pixel unit
				// of the upper left point of the client viewing area; then convert to scroll units in Scroll().
				pApp->GetMainFrame()->canvas->Scroll(0,posn); //pView->ScrollWindow(0,-yDist);
				Refresh();
				bScrollFinished = TRUE;
			}
		}
		else
		{
			bScrollFinished = TRUE;
			::wxBell();
		}

		if (bScrollFinished)
			goto d;
		else
		{
			--nScrollCount;
			if (nScrollCount == 0)
				goto d;
			else
			{
				goto b;
			}
		}
		// restore cursor location when done
d:		SetFocus();
		SetSelection(gnStart,gnEnd);
		return;
	}
	else if (event.GetKeyCode() == WXK_PRIOR)
	{
		pView->GetVisibleStrips(nCurrentStrip,nLastStrip);
		nScrollCount = nLastStrip - nCurrentStrip;
		goto a;
	}
	else if (event.GetKeyCode() == WXK_NEXT)
	{
		pView->GetVisibleStrips(nCurrentStrip,nLastStrip);
		nScrollCount = nLastStrip - nCurrentStrip;
		goto b;
	}
	else if (!gbIsGlossing && gpApp->m_bTransliterationMode && event.GetKeyCode() == WXK_RETURN)
	{
		// CTRL + ENTER is a JumpForward() to do transliteration; bleed this possibility out
		// before allowing for any keypress to halt automatic insertion; one side effect is that
		// MFC rings the bell for each such key press and I can't find a way to stop it. So
		// Alt + Backspace can be used instead, for the same effect; or the sound can be turned
		// off at the keyboard if necessary. This behaviour is only available when
		// transliteration mode is turned on.
		if (event.ControlDown()) 
		{				
			// save old sequ number in case required for toolbar's Back button
			gnOldSequNum = gpApp->m_nActiveSequNum;

			gbSuppressStoreForAltBackspaceKeypress = TRUE; // suppress store to KB for this move of box
			// the value is restored to FALSE in MoveToNextPile()
			
			// do the move forward to next empty pile, with lookup etc, but no store due to 
			// the gbSuppressStoreForAltBackspaceKeypress global being TRUE until the StoreText()
			// call is jumped over in the MoveToNextPile() call within JumpForward()
			JumpForward(pView);
			return;
		}
	}
	else if (!gbIsGlossing && !gpApp->m_bTransliterationMode && event.GetKeyCode() == WXK_RETURN)
	{
		if (event.ControlDown()) 
		{
			// user wanted to initiate a transliteration advance of the phrase box, with its
			// special KB storage mode, but forgot to turn the transliteration mode on before
			// using this feature, so warn him to turn it on and then do nothing
			// IDS_TRANSLITERATE_OFF
			wxMessageBox(_("Transliteration mode is not yet turned on."),_T(""),wxICON_WARNING);

			// restore focus to the phrase box
			if (pApp->m_pTargetBox != NULL)	
				if (pApp->m_pTargetBox->IsShown())
					pApp->m_pTargetBox->SetFocus();
		}
	}
	// MFC code was commented out below:
	event.Skip(); // CEdit::OnKeyUp(nChar, nRepCnt, nFlags); CEdits do not do anything with WM_KEYUP messages
}

// This OnKeyDown function is called via the EVT_KEY_DOWN event in our CPhraseBox
// Event Table.
void CPhraseBox::OnKeyDown(wxKeyEvent& event)
{
	//wxLogDebug(_T("OnKeyDown() %d called from PhraseBox"),event.GetKeyCode());
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	//wxASSERT(pView->IsKindOf(CLASSINFO(wxScrolledWindow)));
	wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));
	if (!pApp->m_bSingleStep)
	{
		// halt the auto matching and inserting, if a key is typed
		//CAdapt_ItApp* pApp = (CAdapt_ItApp*)AfxGetApp();
		if (pApp->m_bAutoInsert)
		{
			pApp->m_bAutoInsert = FALSE;
			// Skip() should not be called here, because we can ignore the value of
			// any keystroke that was made to stop auto insertions.
			return;
		}
	}

	// update status bar with project name
	gpApp->RefreshStatusBarInfo();

	//event.Skip(); //CEdit::OnKeyDown(nChar, nRepCnt, nFlags);

	// [MFC Note] Delete key sends WM_KEYDOWN message only, WM_CHAR not sent
	// so we need to update things for forward deletions here (and it has to be
	// done after the call to the base class, otherwise the last deleted character
	// remains in the final target phrase text)
	// BEW added more on 20June06, so that a DEL keypress in the box when it has a <no adaptation>
	// KB or GKB entry will effect returning the CSourcePhrase instance back to a true "empty" one
	// (ie. m_bHasKBEntry or m_bHasGlossingKBEntry will be cleared, depending on current mode, and
	// the relevant KB's CRefString decremented in the count, or removed entirely if the count is 1.)
	// wx Note: In contrast to Bruce's note above OnKeyDown() in wx Delete key does trigger
	// OnChar()
	if (event.GetKeyCode() == WXK_DELETE)
	{
		if (gpApp->m_pActivePile->m_pSrcPhrase->m_adaption.IsEmpty() &&
			((gpApp->m_pActivePile->m_pSrcPhrase->m_bHasKBEntry && !gbIsGlossing) ||
			(gpApp->m_pActivePile->m_pSrcPhrase->m_bHasGlossingKBEntry && gbIsGlossing)))
		{
			gbNoAdaptationRemovalRequested = TRUE;
			CKB* pKB = pView->GetKB();
			CSourcePhrase* pSrcPhrase = gpApp->m_pActivePile->m_pSrcPhrase;
			CRefString* pRefString = pView->GetRefString(pKB, pSrcPhrase->m_nSrcWords,
				pSrcPhrase->m_key,pSrcPhrase->m_adaption);
			if (pRefString != NULL)
			{
				pView->RemoveRefString(pRefString,pSrcPhrase,pSrcPhrase->m_nSrcWords);
			}
		}
		else
		{
			// legacy behaviour: handle the forward character deletion
			m_backspaceUndoStr.Empty();
			gnSaveStart = gnSaveEnd = -1;

			wxString s;
			s = GetValue();
			pApp->m_targetPhrase = s; // otherwise, deletions using <DEL> key are not recorded
		}
	}
	event.Skip(); // allow processing of the keystroke event to continue
}

bool CPhraseBox::LookAhead(CAdapt_ItView *pAppView, CPile* pNewPile)
// return TRUE if we made a match and there is a translation to be inserted (see static var
// below); return FALSE if there was no match
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	int		nNewSequNum = pNewPile->m_pSrcPhrase->m_nSequNumber; // sequ number at the new location
	wxString	phrases[10]; // store built phrases here, for testing against KB stored source phrases
	int		numPhrases;  // how many phrases were built in any one call of this LookAhead function
	translation.Empty(); // clear the static variable, ready for a new translation, or gloss,
						 // if one can be found
	nWordsInPhrase = 0;	  // the global, initialize to value for no match
	gbByCopyOnly = FALSE; // restore default setting

	// we should never have an active selection at this point, so ensure there is no selection
	pAppView->RemoveSelection();

	// build the as many as 10 phrases based on first word at the new pile and the following 
	// nine piles, or as many as are valid for phrase building (there are 7 conditions which
	// will stop the builds). When adapting, all 10 can be used; when glossing, only can use
	// the first of the ten and in that case numPhrases = 1 will be returned.
	// For auto capitalization support, the 10 phrases strings are built from the document's
	// CSourcePhrase instances as before, no case changes made; and any case changes, and secondary
	// lookups if the primary (lower case) lookup fails when the source is upper case, are done
	// within the AutoCapsLookup( ) function which is called within FindMatchInKB( ) as called
	// below; so nothing needs to be done here.
	numPhrases = BuildPhrases(phrases, nNewSequNum, pApp->m_pSourcePhrases);

	// if the returned value is zero, no phrases were built, so this constitutes a non-match
	// BEW changed,9May05, to allow for the case when there is a merger at the last sourcephrase
	// of the document and it's m_bHasKBEntry flag (probably erroneously) is FALSE - the move
	// will detect the merger and BuildPhrases will not build any (so it exits returning zero)
	// and if we were to just detect the zero and return FALSE, a copy of the source would
	// overwrite the merger's translation - to prevent this, we'll detect this state of affairs
	// and cause the box to halt, but with the correct (old) adaptation showing. Then we have
	// unexpected behaviour (ie. the halt), but not an unexpected adaptation string.
	if (numPhrases == 0)
	{
		if (pNewPile->m_pSrcPhrase->m_nSrcWords > 1 && !pNewPile->m_pSrcPhrase->m_adaption.IsEmpty())
		{
			translation = pNewPile->m_pSrcPhrase->m_adaption;
			nWordsInPhrase = 1; // strictly speaking not correct, but it's the value we want
								// on return to the caller
			gpApp->m_bAutoInsert = FALSE; // cause a halt
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
		
	// check these phrases, longest first, attempting to find a match in the KB
	CKB* pKB;
	int nCurLongest;
	if (gbIsGlossing)
	{
		pKB = ((CAdapt_ItApp*)&wxGetApp())->m_pGlossingKB;
		nCurLongest = 1; // the caller should ensure this, but doing it explicitly here is
						 // some extra insurance to keep matching within the only map that exists
						 // when the glossing KB is in use
	}
	else
	{
		pKB = ((CAdapt_ItApp*)&wxGetApp())->m_pKB;
		nCurLongest = pKB->m_nMaxWords; // no matches are possible for phrases longer 
										// than nCurLongest when adapting
	}
	CTargetUnit* pTargetUnit = (CTargetUnit*)NULL;
	int index = nCurLongest - 1;
	bool bFound = FALSE;
	while (index > -1)
	{
		bFound = FindMatchInKB(pKB, index + 1,phrases[index],pTargetUnit);
		if (bFound)
		{
			// matched a source phrase which has identical key as the built phrase
			break;
		}
		else
		{
			index--; // try next-smaller built phrase
		}
	}

	// if no match was found, we return immediately with a return value of FALSE
	if (!bFound)
	{
		pCurTargetUnit = (CTargetUnit*)NULL; // the global pointer must be cleared
		curKey.Empty(); // global var curKey not needed, so clear it
		return FALSE;
	}
	pCurTargetUnit = pTargetUnit; // set global pointer so the dialog can use it if it 
								  // is called 
	curKey = phrases[index]; // set the curKey so the dialog can use it if it is called
							 // curKey is a global variable (the lookup of phrases[index] is
							 // done on a copy, so curKey retains the case of the key as in
							 // the document; but the lookup will have set global flags
							 // such as gbMatchedKB_UCentry to TRUE or FALSE as appropriate)
	nWordsInPhrase = index + 1; // static variable, needed for merging source phrases in 
								// the caller; when glossing, index will == 0 so no merge will
								// be asked for below while in glossing mode

	// we found a target unit in the list with a matching m_key field, so we must now set 
	// the static var translation to the appropriate adaptation, or gloss, text: this
	// will be the target unit's first entry in its list if the list has only one entry, else
	// we must present the user with a dialog to put up all possible adaptions (or glosses) for
	//  the user to choose one, or type a new one, or reject all - in which case we return 
	// FALSE for manual typing of an adaptation (or gloss) etc. For autocapitalization support,
	// the dialog will show things as stored in the KB (if auto caps is ON, that could be with
	// all or most entries starting with lower case) and we let any needed changes to initial
	// upper case be done automatically after the user has chosen or typed.
	TranslationsList::Node *node = pTargetUnit->m_pTranslations->GetFirst();
	wxASSERT(node != NULL);
	int count = pTargetUnit->m_pTranslations->GetCount();

	wxASSERT(count > 0);
	if (count > 1 || pTargetUnit->m_bAlwaysAsk)
	{
		// move to new location, merge if necessary, so user has visual feedback about which
		// element(s) is involved
		pAppView->RemoveSelection();

		// set flag for telling us that we will have completed the move when the dialog is shown
		gbCompletedMergeAndMove = TRUE;

		// need view's m_ptCurBoxLocation reset to where it would appear at the new active
		// location so that the AdjustDialogPosition() call will place the ChooseTranslation
		// dialog correctly
		CPile* pPile = pAppView->GetPile(pApp->m_nActiveSequNum);
		wxASSERT(pPile);

		// create the appropriate selection there
		CCell* pAnchorCell = pPile->m_pCell[1]; // 2nd cell, ie. line 2, or 1 in a 1-src
											    // line strip
		if (nWordsInPhrase > 0)
		{
			pApp->m_pAnchor = pAnchorCell;
			CCellList* pSelection = &pApp->m_selection;
			wxASSERT(pSelection->IsEmpty());
			pApp->m_selectionLine = 1;
			wxClientDC aDC(pApp->GetMainFrame()->canvas); // make a temporary device context

			// then do the new selection, start with the anchor cell
			wxColour oldBkColor = aDC.GetTextBackground();
			aDC.SetBackgroundMode(pApp->m_backgroundMode);
			aDC.SetTextBackground(wxColour(255,255,0)); // yellow
			pAnchorCell->m_pText->Draw(&aDC);
			pApp->m_bSelectByArrowKey = FALSE;
			pAnchorCell->m_pText->m_bSelected = TRUE;

			// preserve record of the selection
			pSelection->Append(pAnchorCell);

			// extend the selection to the right, if more than one pile is involved
			// When glossing, the block will be skipped because nWordsInPhrase will equal 1
			if (nWordsInPhrase > 1)
			{
				// extend the selection
				pAppView->ExtendSelectionForFind(pAnchorCell,nWordsInPhrase);
			}
		}

		// the following code added to support Bill Martin's wish that the phrase box be shown
		// at its new location when the dialog is open (if the user cancels the dialog, the old
		// state will have to be restored - that is, any merge needs to be unmerged)
		pApp->m_pTargetBox->m_bAbandonable = FALSE;
		// adaptation is available, so use it if the source phrase has only a single word, but
		// if it's multi-worded, we must first do a merge and recalc of the layout
		gbMergeDone = FALSE; //  global, used in ChooseTranslation()
		if (nWordsInPhrase > 1) // nWordsInPhrase is a global, set in this function
								// or in LookUpSrcWord()
		{
			// do the needed merge, etc.
			pApp->bLookAheadMerge = TRUE; // set static flag to ON
			bool bSaveFlag = m_bAbandonable; // the box is "this"
			pAppView->MergeWords();
			m_bAbandonable = bSaveFlag; // preserved the flag across the merge
			pApp->bLookAheadMerge = FALSE; // restore static flag to OFF
			gbMergeDone = TRUE;
			gbSuppressMergeInMoveToNextPile = TRUE;
		}
		else
			pAppView->RemoveSelection(); // glossing, or adapting a single src word only
		
		// next code is taken from end of MoveToNextPile()
		// initialize the phrase box to be empty, so as not to confuse the user
		if (GetHandle() != NULL)
		{
			SetValue(_T(""));
			pApp->m_targetPhrase = _T("");
		}

		// recalculate the layout
		pApp->m_curBoxWidth = 2; // make very small so it doesn't push the next word/phrase
									 // to the right before the next word/phrase can be measured
		pAppView->RecalcLayout(pApp->m_pSourcePhrases, 0, pApp->m_pBundle);

		// get the new active pile
		pApp->m_pActivePile = pAppView->GetPile(pApp->m_nActiveSequNum);
		wxASSERT(pApp->m_pActivePile != NULL);

		// update the copy held on the CPhraseBox too
		m_pActivePile = pApp->m_pActivePile;		
		
		// recreate the phraseBox but empty, and get its location so that we can determine
		// where the dialog is to be located
		pApp->m_ptCurBoxLocation = pApp->m_pActivePile->m_pCell[2]->m_ptTopLeft;
		pAppView->RemakePhraseBox(pApp->m_pActivePile,pApp->m_targetPhrase);

		// scroll into view
		gpApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);

		// bw 14Feb05 added the line below to try to prevent ChooseTranslation box from
		// displaying off screen as reported by John Nystrom
		// recalculate, so that AdjustDialogPosition() will have the scrolled 
		// location's coords to work with if the Choose Translation dialog needs to be put up
		gpApp->m_ptCurBoxLocation = m_pActivePile->m_pCell[2]->m_ptTopLeft;

		// make what we've done visible
		pAppView->Invalidate();

		// put up a dialog for user to choose translation from a list box, or type new one
		// (note: for auto capitalization; ChooseTranslation (which calls the CChoseTranslation
		// dialog handler, only sets the 'translation' global variable, it does not make any case
		// adjustments - these, if any, must be done in the caller)

		// wx version addition: In wx the OnIdle handler continues to run even when modal dialogs
		// are being shown, so we'll save the state of m_bAutoInsert before calling ChooseTranslation
		// change m_bAutoInsert to FALSE while the dialog is being shown, then restore m_bAutoInsert's
		// prior state after ChooseTranslation returns.
		bool saveAutoInsert = gpApp->m_bAutoInsert;
		gpApp->m_bAutoInsert = FALSE;

		bool bOK;
		if (gbIsGlossing)
			bOK = ChooseTranslation(TRUE); // TRUE causes Cancel And Select button to be hidden
		else
			bOK = ChooseTranslation(); // default makes Cancel And Select button visible

		// wx version: restore the state of m_bAutoInsert
		gpApp->m_bAutoInsert = saveAutoInsert;

		pCurTargetUnit = (CTargetUnit*)NULL; // ensure the global var is cleared after the dialog has used it
		curKey.Empty(); // ditto for the current key string (global)
		if (!bOK)
		{
			// user cancelled, so return FALSE for a 'non-match' situation; the 
			// gbUserWantsSelection global variable (set by CChooseTranslation's 
			// m_bCancelAndSelect member) will have already been set and can be used in 
			// the caller (ie. in MoveToNextPile)
			gbCompletedMergeAndMove = FALSE;

			return FALSE;
		}
		// if bOK was TRUE, translation static var will have been set via the dialog; but
		// any needed case change to get the data ready for showing in the view will not have 
		// been done within the dialog's handler code - so do them below

		// User has produced a translation from the Choose Translation dialog
		// so set the globals to the active sequence number, so we can track 
		// any automatically inserted target/gloss text, starting from the 
		// current phrase box location (where the Choose Translation dialog appears).

		// BEW modified 20June06; the starting value for gnEndInsertionsSequNum
		// must be one less than gnBeginInsertionsSequNum so that at the first
		// auto insert location the incrementation of gnEndInsertionsSequNum will
		// become equal in value to gnBeginInsertionsSequNum, and ony for a second
		// and subsequent insertions will the end one become greater than the
		// beginning one. This fixes an error which resulted in the pile AFTER
		// the halted phrase box having background highlighting even though the
		// phrase box has not yet got that far (because gnEndInsertionsSequNum
		// was set one too large)
		gnBeginInsertionsSequNum = pApp->m_nActiveSequNum + 1;
		gnEndInsertionsSequNum = gnBeginInsertionsSequNum - 1;
	}
	else // count == 1 case (ie. a unique adaptation, or a unique gloss when glossing)
	{
		translation = ((CRefString*)node->GetData())->m_translation;
	}

	// adjust for case, if necessary; this algorithm will not make a lower case string start
	// with upper case when the source is uppercase if the user types punctuation at the start
	// of the string. The latter is, however, unlikely - provided the auto punctuation support
	// is utilized by the user
	if (gbAutoCaps && gbSourceIsUpperCase)
	{
		bool bNoError = pAppView->SetCaseParameters(translation,FALSE);
		if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
		{
			// make it upper case
			translation.SetChar(0,gcharNonSrcUC);
		}
	}
	return TRUE;
}

/* // BEW removed 31Jan08 because gLastSrcPhrasePos cannot be guaranteed to be valid
   // in all circumstances (eg. edit source text word above phrase box, immediately cancel,
   // then type an adaptation and hit ENTER; gLastSrcPhrasePos value is now no longer in
   // m_pSourcePhrases list, and GetSrcPhrasePos() crashes in the do loop with pSrcPhrase = garbage)

SPList::Node* CPhraseBox::GetSrcPhrasePos(int nSequNum, SPList* pSourcePhrases)
// returns the POSITION value for nSequNum; no code changes needed for glossing vs adapting
// (this looks in only the next 100 positions - if it fails to find the POSITION value within
// that range, then the caller tries a slower method which is sure to succeed.)
{
	CSourcePhrase* pSrcPhrase;
	SPList::Node *pos;
	SPList::Node *savePos = (SPList::Node*)NULL;
	if (gLastSrcPhrasePos == NULL)
		return (SPList::Node*)NULL;
	if (nSequNum < 0 || nSequNum > (int)pSourcePhrases->GetCount())
		return (SPList::Node*)NULL;
	int limit = 100;
	int count = 0;
	pos = gLastSrcPhrasePos; // old value
	do {
		savePos = pos;
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		count++;
		if (pSrcPhrase->m_nSequNumber == nSequNum)
		{
			// we have found the new position
			return savePos;
		}
	} while (pos != NULL && count < limit);

	// if we get here, we didn't find the location, so return null
	return (SPList::Node*)NULL;
}
*/

int CPhraseBox::BuildPhrases(wxString phrases[10], int nNewSequNum, SPList* pSourcePhrases)
// returns number of phrases built; a return value of zero means that we have a halt condition
// and so none could be built (eg. source phrase is a merged one would halt operation)
// When glossing is ON, the build is constrained to phrases[0] only, and return value would then
// be 1 always.
{
	// clear the phrases array
	phrases[0] = phrases[1] = phrases[2] = phrases[3] = phrases[4] = phrases[5] = phrases[6]
		= phrases[7] = phrases[8] = phrases[9] = _T("");

	// check we are within bounds
	int nMaxIndex = pSourcePhrases->GetCount() - 1;
	if (nNewSequNum > nMaxIndex)
	{
		// this is unlikely to ever happen, but play safe just in case
		wxMessageBox(_T("Index bounds error in BuildPhrases call\n"), _T(""), wxICON_EXCLAMATION);
		wxExit();
	}

	// find position of the active pile's source phrase in the list
	SPList::Node *pos;
/* // BEW removed 31Jan08 because gLastSrcPhrasePos cannot be guaranteed to be valid
   // in all circumstances (eg. edit source text word above phrase box, immediately cancel,
   // then type an adaptation and hit ENTER; gLastSrcPhrasePos value is now no longer in
   // m_pSourcePhrases list, and GetSrcPhrasePos() crashes in the do loop with pSrcPhrase = garbage)
	if (gbEnterTyped)
	{
		// GetSrcPhrasePos searches forward from the global POSITION value gLastSrcPhrasePos
		// which often will be the immediately preceding position in the list, hence this is
		// faster
		pos = GetSrcPhrasePos(nNewSequNum,pSourcePhrases);
		if (pos == NULL)
		{
			// if we have not found it within next 100 entries in the list, then use
			// search from the start of the list (the older, slower, but safe way)
			goto d;
		}
	}
	else
	{
d:		pos = pSourcePhrases->Item(nNewSequNum);
	}
	*/
	pos = pSourcePhrases->Item(nNewSequNum);

	wxASSERT(pos != NULL);
	/* BEW removed 31Jan08, see above for reason
	gLastSrcPhrasePos = pos; // set as the "old position" ready for the next time we 
							 // visit BuildPhrases()
	*/
	int index = 0;
	int counter = 0;
	CSourcePhrase* pSrcPhrase;

	// build the phrases array, breaking if a build halt condition is encounted
	// (These are: if we reach a boundary, or a source phrase with an adaption already, or
	// a change of TextType, or a null source phrase or a retranslation, or EOF, or max
	// number of words is reached, or we reached a source phrase which has been previously
	// merged.) MAX_WORDS is defined in AdaptitConstants.h (TextType changes can be ignored
	// here because they coincide with m_bBoundary == TRUE on the previous source phrase.)
	// When glossing is ON, there are no conditions for halting, because a pile will already
	// be active, and the src word at that location will be glossable no matter what.
	if (gbIsGlossing)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		phrases[0] = pSrcPhrase->m_key;
		return counter = 1;
	}
	while (pos != NULL && index < MAX_WORDS)
	{
		// NOTE: MFC's GetNext(pos) retrieves the current pos data into 
		// pScrPhrase, then increments pos
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext(); 
		if (pSrcPhrase->m_nSrcWords > 1 || !pSrcPhrase->m_adaption.IsEmpty() ||
			pSrcPhrase->m_bNullSourcePhrase || pSrcPhrase->m_bRetranslation)
			return counter; // don't build with this src phrase, it's a merged one, etc.

		if (index == 0)
		{
			phrases[0] = pSrcPhrase->m_key;
			counter++;
			if (pSrcPhrase->m_bBoundary || nNewSequNum + counter > nMaxIndex)
				break;
		}
		else
		{
			phrases[index] = phrases[index - 1] + _T(" ") + pSrcPhrase->m_key;
			counter++;
			if (pSrcPhrase->m_bBoundary || nNewSequNum + counter > nMaxIndex)
				break;
		}
		index++;
	}
	return counter;
} 

bool CPhraseBox::FindMatchInKB(CKB* pKB, int numWords, wxString keyStr, CTargetUnit *&pTargetUnit)
// returns TRUE if a matching KB entry found; when glossing, pKB points to the glossing KB, when
// adapting it points to the normal KB
{
	CAdapt_ItView* pView = gpApp->GetView();
	MapKeyStringToTgtUnit* pMap = pKB->m_pMap[numWords-1];
	CTargetUnit* pTU;
	bool bOK = pView->AutoCapsLookup(pMap,pTU,keyStr);
	if (bOK)
		pTargetUnit = pTU;	// makes pTargetUnit point to same object pointed to by pTU
							// and shouldn't require use of an assignment operator. Since
							// pTargetUnit is a reference parameter, FindMatchInKb will
							// assign the new pointer to the caller pointer argument
	return bOK;
}

bool CPhraseBox::ChooseTranslation(bool bHideCancelAndSelectButton)
{
	// update status bar with project name
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	CAdapt_ItDoc* pDoc;
	CAdapt_ItView* pView;
	CPhraseBox* pBox;
	pApp->GetBasePointers(pDoc,pView,pBox);
	if (pView == NULL)
	{
		gbUserWantsSelection = FALSE;
		return FALSE;
	}
	wxASSERT(pView->IsKindOf(CLASSINFO(wxView)));

	CChooseTranslation dlg(pApp->GetMainFrame()); 
	dlg.Centre();

	// update status bar with project name
	gpApp->RefreshStatusBarInfo();

	// initialize m_chosenTranslation, other initialization is in InitDialog()
	dlg.m_chosenTranslation.Empty();
	dlg.m_bHideCancelAndSelectButton = bHideCancelAndSelectButton; // defaults to FALSE if 
																   // not set in caller
	// put up the dialog
	gbInspectTranslations = FALSE;

	if(dlg.ShowModal() == wxID_OK)
	{
		gbUserCancelledChooseTranslationDlg = FALSE;
		
		// set the translation static var from the member m_chosenTranslation
		translation = dlg.m_chosenTranslation;

		// do a case adjustment if necessary
		bool bNoError = TRUE;
		if (gbAutoCaps)
		{
			bNoError = pView->SetCaseParameters(pApp->m_pActivePile->m_pSrcPhrase->m_key);
			if (bNoError && gbSourceIsUpperCase)
			{
				bNoError = pView->SetCaseParameters(translation,FALSE);
				if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
				{
					translation.SetChar(0,gcharNonSrcUC);
				}
			}
		}
		// bw added block above 16Aug04
		pView->RemoveSelection();
		return TRUE;
	}
	else
	{
		// must have hit Cancel button, or the Cancel And Select button
		if (dlg.m_bCancelAndSelect)
			gbUserWantsSelection = TRUE;
		else
			gbUserWantsSelection = FALSE;

		// we have to undo any merge, but only provided the unmerge has not already
		// been done in the OnButtonRestore() function; a merge can only have been done
		// if adapting is current, so suppress the unmerge if glossing is current
		if (!gbIsGlossing && gbMergeDone && !gbUnmergeJustDone)
		{
			gbMergeDone = FALSE;
			gbSuppressLookup = TRUE; // don't want LookUpSrcWord() called from
									 // OnButtonRestore() because
			pView->UnmergePhrase(); // UnmergePhrase() otherwise calls LookUpSrcWord() 
									// which calls ChooseTranslation()
			gbSuppressMergeInMoveToNextPile = FALSE; // allow the MoveToNextPile() merge,
													 // though it won't be accessed
		}
		else
		{
			if (gbIsGlossing)
			{
				// these should not be necessary, but will keep things safe when glossing
				gbMergeDone = FALSE;
				gbSuppressLookup = TRUE;
				gbSuppressMergeInMoveToNextPile = TRUE;
			}
		}

		gbUserCancelledChooseTranslationDlg = TRUE; // use in MoveToNextEmptyPile() to 
								// suppress a second showing of dialog from LookUpSrcWord()
		pView->RemoveSelection();

		return FALSE;
	}
}

void CPhraseBox::SetModify(bool modify)
{
	if (modify)
		MarkDirty(); // "mark text as modified (dirty)"
	else
		DiscardEdits(); // "resets the internal 'modified' flag as if the current edits had been saved"
}

bool CPhraseBox::GetModify()
{
	return IsModified();
}


void CPhraseBox::DoCancelAndSelect(CAdapt_ItView* pView, CPile* pPile)
{
	if (gbIsGlossing) return; // unneeded,  but ensures correct behaviour if I goofed elsewhere
	CSourcePhrase* pSrcPhrase = pPile->m_pSrcPhrase;
	pView->SelectFoundSrcPhrases(pSrcPhrase->m_nSequNumber,2,FALSE,TRUE);
}

void CPhraseBox::OnLButtonDown(wxMouseEvent& event) 
{	
	// This mouse event is only activated when user clicks mouse L button within
	// the phrase box, not elsewhere on the screen
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	// don't allow cursor to be placed in phrasebox when in free trans mode and when 
	// it is not editable. Allows us to have a pink background in the phrase box in
	// free trans mode. TODO: we could also make the text grayed out to more closely
	// immulate MFC Windows behavior (we could call Enable(FALSE) but that not only 
	// turns the text grayed out, but also the background gray instead of the desired
	// pink. It is better to do this here than in OnLButtonUp since it prevents the
	// cursor from being momemtarily seen in the phrase box if clicked.
	if (gpApp->m_bFreeTranslationMode && !this->IsEditable())
	{
		CMainFrame* pFrame;
		pFrame = gpApp->GetMainFrame();
		wxASSERT(pFrame != NULL);
		wxASSERT(pFrame->m_pComposeBar != NULL);
		wxTextCtrl* pEdit = (wxTextCtrl*)pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
		wxASSERT(pEdit != NULL);
		pEdit->SetFocus();
		int len = pEdit->GetValue().Length();
		if (len > 0)
		{
			pEdit->SetSelection(len,len);
		}
		::wxBell();
		// don't call Skip() as we don't want the mouse click processed elsewhere
		return;
	}
	// version 1.4.2 and onwards, we want a right or left arrow used to remove the
	// phrasebox's selection to be considered a typed character, so that if a subsequent
	// selection and merge is done then the first target word will not get lost; and so
	// we add a block of code also to start of OnChar( ) to check for entry with both
	// m_bAbandonable and m_bUserTypedSomething set TRUE, in which case we don't
	// clear these flags (the older versions cleared the flags on entry to OnChar( ) )

	// we use this flag cocktail elsewhere to test for these values of the three flags as
	// the condition for telling the application to retain the phrase box's contents
	// when user deselects target word, then makes a phrase and merges by typing
	m_bAbandonable = FALSE;
	pApp->m_bUserTypedSomething = TRUE;
	gbRetainBoxContents = TRUE;

	event.Skip(); //CEdit::OnLButtonDown(nFlags, point);
	GetSelection(&gnStart,&gnEnd);
}

void CPhraseBox::OnLButtonUp(wxMouseEvent& event) 
{
	// This mouse event is only activated when user clicks mouse L button within
	// the phrase box, not elsewhere on the screen
	event.Skip(); //CEdit::OnLButtonDown(nFlags, point);
	GetSelection(&gnStart,&gnEnd);
}

bool CPhraseBox::LookUpSrcWord(CAdapt_ItView *pAppView, CPile* pNewPile)
// return TRUE if we made a match and there is a translation to be inserted (see static var
// below); return FALSE if there was no match; based on LookAhead(), but only looks up a single
// src word at pNewPile
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	int	nNewSequNum;
	nNewSequNum = pNewPile->m_pSrcPhrase->m_nSequNumber; // sequ number at the new location
	wxString	phrases[1]; // store built phrases here, for testing against KB stored source phrases
	int	numPhrases;
	numPhrases = 1;  // how many phrases were built in any one call of this function
	translation.Empty(); // clear the static variable, ready for a new translation if one can
						 // be found
	nWordsInPhrase = 0;	  // assume no match
	gbByCopyOnly = FALSE; // restore default setting

	// we should never have an active selection at this point, so ensure it
	pAppView->RemoveSelection();

	// get the source word
	phrases[0] = pNewPile->m_pSrcPhrase->m_key;
		
	// check this phrase (which is actually a single word), attempting to find a match in the KB
	// (if glossing, it might be a single word or a phrase, depending on what user did earlier
	// at this location before he turned glossing on)
	CKB* pKB;
	if (gbIsGlossing)
		pKB = ((CAdapt_ItApp*)&wxGetApp())->m_pGlossingKB;
	else
		pKB = ((CAdapt_ItApp*)&wxGetApp())->m_pKB;
	CTargetUnit* pTargetUnit = (CTargetUnit*)NULL;
	int index = 0;
	bool bFound = FALSE;
	bFound = FindMatchInKB(pKB,index + 1,phrases[index],pTargetUnit);

	// if no match was found, we return immediately with a return value of FALSE
	if (!bFound)
	{
		pCurTargetUnit = (CTargetUnit*)NULL; // the global pointer must be cleared
		curKey.Empty(); // global var curKey not needed, so clear it
		return FALSE;
	}
	pCurTargetUnit = pTargetUnit; // set global pointer so the dialog can use it if it is called
	curKey = phrases[index]; // set the global curKey so the dialog can use it if it is called
							 // (phrases[0] is copied for the lookup, so curKey has initial case
							 // setting as in the doc's sourcephrase instance; we don't need
							 // to change it here (if ChooseTranslation( ) is called, it does
							 // any needed case changes internally)
	nWordsInPhrase = index + 1; // static variable, needed for merging source phrases in 
								// the caller
	// we found a target unit in the list with a matching m_key field, 
	// so we must now set the static var translation to the appropriate adaptation text: this
	// will be the target unit's first entry in its list if the list has only one entry, else
	// we must present the user with a dialog to put up all possible adaptions for the user to
	// choose one, or type a new one, or reject all - in which case we return FALSE for manual
	// typing of an adaptation etc.
	TranslationsList::Node* pos = pTargetUnit->m_pTranslations->GetFirst();
	wxASSERT(pos != NULL);
	int count = pTargetUnit->m_pTranslations->GetCount();

	wxASSERT(count > 0);
	if (count > 1 || pTargetUnit->m_bAlwaysAsk)
	{
		// move view to new location and select, so user has visual feedback)
		// about which element(s is involved
		pAppView->RemoveSelection();

		// next code is taken from end of MoveToNextPile()
		// initialize the phrase box to be empty, so as not to confuse the user
		SetValue(_T(""));
		pApp->m_targetPhrase = _T("");

		// recalculate the layout
		pApp->m_curBoxWidth = 2; // make very small so it doesn't push the next 
						// word/phrase to the right before the next word/phrase can be measured
		CAdapt_ItApp* pApp = &wxGetApp();
		wxASSERT(pApp != NULL);
		pAppView->RecalcLayout(pApp->m_pSourcePhrases,0,pApp->m_pBundle);

		// get the new active pile
		pApp->m_pActivePile = pAppView->GetPile(pApp->m_nActiveSequNum);
		wxASSERT(pApp->m_pActivePile != NULL);

		// update the copy held on the CPhraseBox too
		m_pActivePile = pApp->m_pActivePile;		
		
		// recreate the phraseBox but empty, and get its location so that we can determine
		// where the dialog is to be located
		pApp->m_ptCurBoxLocation = pApp->m_pActivePile->m_pCell[2]->m_ptTopLeft;
		pAppView->RemakePhraseBox(pApp->m_pActivePile,pApp->m_targetPhrase);

		// scroll into view
		pApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);

		// make what we've done visible
		pAppView->Invalidate();

		// put up a dialog for user to choose translation from a list box, or type
		// new one (and set bHideCancelAndSelectButton boolean to TRUE)
		bool bOK = ChooseTranslation(TRUE);
		pCurTargetUnit = (CTargetUnit*)NULL; // ensure the global var is cleared after the dialog has used it
		curKey.Empty(); // ditto for the current key string (global)
		if (!bOK)
		{
			// user cancelled, so return FALSE for a 'non-match' situation; the 
			// gbUserWantsSelection global variable (set by CChooseTranslation's
			// m_bCancelAndSelect member) will have already been set and can be used in
			// the caller (ie. in MoveToNextPile)
			return FALSE;
		}
		// if bOK was TRUE, translation static var will have been set via the dialog; and
		// if autocaps is ON and a change to upper case was needed, it will not have been done
		// within the dialog's handler code
	}
	else
	{
		translation = ((CRefString*)pos->GetData())->m_translation;
	}

	// adjust for case, if necessary; this algorithm will not make a lower case string start
	// with upper case when the source is uppercase if the user types punctuation at the start
	// of the string. The latter is, however, unlikely - provided the auto punctuation support
	// is utilized by the user
	if (gbAutoCaps && gbSourceIsUpperCase)
	{
		bool bNoError = pAppView->SetCaseParameters(translation,FALSE);
		if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
		{
			// make it upper case
			translation.SetChar(0,gcharNonSrcUC);
		}
	}
	return TRUE;
}

void CPhraseBox::OnEditUndo(wxCommandEvent& WXUNUSED(event)) 
// no changes needed for support of glossing or adapting
{
	// We have to implement undo for a backspace ourselves, but for delete & edit menu commands
	// the CEdit Undo() will suffice; we use a non-empty m_backspaceUndoStr as a flag that
	// the last edit operation was a backspace

	CAdapt_ItView* pView = gpApp->GetView();


	if (m_backspaceUndoStr.IsEmpty())
	{
		// last operation was not a <BS> keypress, so CEdit's Undo() will be done instead
		;
	}
	else
	{
		if (!(gnSaveStart == -1 || gnSaveEnd == -1))
		{
			bool bRestoringAll = FALSE;
			wxString thePhrase;
			thePhrase = GetValue();
			int undoLen = m_backspaceUndoStr.Length();
			if (!thePhrase.IsEmpty())
			{
				thePhrase = InsertInString(thePhrase,(int)gnSaveEnd,m_backspaceUndoStr);
			}
			else
			{
				thePhrase = m_backspaceUndoStr;
				bRestoringAll = TRUE;
			}

			// rebuild the box the correct size
			wxSize textExtent;
			bool bWasMadeDirty = TRUE;
			FixBox(pView,thePhrase,bWasMadeDirty,textExtent,1); // selector = 1 for using
																// thePhrase's extent
			// restore the box contents
			SetValue(thePhrase);
			m_backspaceUndoStr.Empty(); // clear, so it can't be mistakenly undone again

			// fix the cursor location
			if (bRestoringAll)
			{
				gnStart = 0;
				gnEnd = -1;
				SetSelection(gnStart,gnEnd); // make it all be selected
			}
			else
			{
				gnStart = gnEnd = gnSaveStart + undoLen;
				SetSelection(gnStart,gnEnd);
			}
		}
	}
}

void CPhraseBox::FixBox(CAdapt_ItView* pView, wxString& thePhrase, bool bWasMadeDirty,
						wxSize& textExtent,int nSelector)
// destroys the phrase box and recreates it with a different size, depending on the 
// nSelector value.
// nSelector == 0, increment the box width using its earlier value
// nSelector == 1, recalculation the box width using the input string extent
// nSelector == 2, backspace was typed, so box may be contracting
// This new version tries to be smarter for deleting text from the box, so that we don't 
// recalculate the layout for the whole screen on each press of the backspace key - to reduce
// the blinking effect
// version 2.0 takes text extent of m_gloss into consideration, if gbIsGlossing is TRUE
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	bool bWholeScreenUpdate = TRUE;
	wxSize currBoxSize(pApp->m_curBoxWidth,pApp->m_nTgtHeight);
	wxClientDC dC(this);
	wxFont* pFont;
	if (gbIsGlossing && gbGlossingUsesNavFont)
		pFont = gpApp->m_pNavTextFont;
	else
		pFont = gpApp->m_pTargetFont;
	wxFont SaveFont = dC.GetFont(); 
	dC.SetFont(*pFont);
	dC.GetTextExtent(thePhrase, &textExtent.x, &textExtent.y); // measure using the current font
	wxSize textExtent2;
	wxSize textExtent3;
	wxSize textExtent4; // version 2.0 and onwards, for extent of m_gloss
	CStrip* pPrevStrip;

	// if close to the right boundary, then recreate the box larger (by 3 chars of width 'w')
	wxString aChar = _T('w');
	wxSize charSize;
	dC.GetTextExtent(aChar, &charSize.x, &charSize.y);
	bool bResult;
	if (nSelector < 2)
	{
		bResult = textExtent.x >= currBoxSize.x - gnNearEndFactor*charSize.x;
	}
	else
	{
		bResult = textExtent.x <= currBoxSize.x - (4*charSize.x);
	}
	if (bResult )
	{
		if (nSelector < 2)
			gbExpanding = TRUE;

		// make sure the activeSequNum is set correctly, we need it to be able
		// to restore the pActivePile pointer after the layout is recalculated
		pApp->m_nActiveSequNum = pApp->m_pActivePile->m_pSrcPhrase->m_nSequNumber;

		// calculate the new width
		if (nSelector == 0)
		{
			pApp->m_curBoxWidth += gnExpandBox*charSize.x;

			GetSelection(&pApp->m_nStartChar,&pApp->m_nEndChar); // store current selection

			// if the box's right edge goes beyond the strip's right boundary, it needs
			// to be recreated on the next line because the phrase it is below will have
			// been automatically moved to the next line
			int nCurrentStrip = pApp->m_pActivePile->m_pStrip->m_nStripIndex;
			CStrip* pStrip = pApp->m_pActivePile->m_pStrip;
			wxRect rectStrip = pStrip->m_rectStrip;
			wxRect rectPile = pApp->m_pActivePile->m_rectPile;
			int nStartPile = rectPile.GetLeft(); 
			int nEndStrip = rectStrip.GetRight(); 

			// recalculate the layout at the appropriate place
			if (nSelector < 2)
			{
				if (nStartPile + pApp->m_curBoxWidth >= nEndStrip)
				{
					int nNextStrip = nCurrentStrip + 1;

					// calculate the appropriate m_ptCurBoxLocation for the first pile in 
					// the next strip
					CSourceBundle* pBundle = pStrip->m_pBundle;
					pStrip = pBundle->m_pStrip[nNextStrip]; // the next strip
					pApp->m_ptCurBoxLocation = pStrip->m_pPile[0]->m_pCell[2]->m_ptTopLeft;
				}
			}
			pView->RecalcLayout(pApp->m_pSourcePhrases,0,pApp->m_pBundle);

			// recalculate the active pile pointer, as the old one was clobbered by doing 
			// the new layout
			pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
			wxASSERT(pApp->m_pActivePile != NULL);
			m_pActivePile = pApp->m_pActivePile; // put a copy in the phraseBox too
			pApp->m_ptCurBoxLocation = pApp->m_pActivePile->m_pCell[2]->m_ptTopLeft;
			
		}
		else
		{
			if (nSelector == 2)
			{
				pApp->m_curBoxWidth = pApp->m_pActivePile->m_nMinWidth;
				pApp->m_targetPhrase = GetValue(); // store current typed string

				//move old code into here & then modify it
				GetSelection(&pApp->m_nStartChar,&pApp->m_nEndChar); // store current selection

				// if the box's right edge goes beyond the strip's right boundary, it needs
				// to be recreated on the next line because the phrase it is below will have
				// been automatically moved to the next line
				int nCurrentStrip = pApp->m_pActivePile->m_pStrip->m_nStripIndex;

				// define some useful local variables for the tests below (code for reducing
				// screen flicker)
				int oldBoxWidth;
				int textWidth;
				int newBoxWidth;
				int boxWidthDifference;
				int nWidthOfFirstPile;
				int neededWidth;
				int actualSpace;
				int srcPhraseMaxWidth;
				int nPreviousStrip;

				// work out (1) if the active loc moves to previous strip, or if not, 
				// (2) if number of piles in strip changes - if yes to either, we have 
				// to update the whole layout; if not we can just layout the strip only
				{
					CStrip* pStrip = pApp->m_pActivePile->m_pStrip;
					int nCurPileIndex = pApp->m_pActivePile->m_nPileIndex;
					pStrip = pApp->m_pActivePile->m_pStrip;
					CSourceBundle* pBundle = pStrip->m_pBundle;
					int nNextStrip = nCurrentStrip + 1;
					if (nNextStrip < pBundle->m_nStripCount)
					{
						CStrip* pNextStrip = pBundle->m_pStrip[nNextStrip];
						CPile* pFirstPile = pNextStrip->m_pPile[0];
						CSourcePhrase* pFirstSrcPhrase = pFirstPile->m_pSrcPhrase;

						if (nCurPileIndex == 0)
						{
							// active location is at the first pile of the current strip, so
							// as the user deletes from the phrase box, this pile could jump
							// up to the end of the preceding strip - this potential behaviour
							// has precedence over the possibility that reducing the pile width
							// (by deletions) may shorten the current strip to the point that
							// the first pile of the next strip may migrate up to the end of
							// the current strip - so we check for that later, only if the 
							// current pile does not move up to the end of the previous strip

							// first, this pile can't migrate up if it is in the first strip
							nPreviousStrip = nCurrentStrip;
							--nPreviousStrip;
							if (nPreviousStrip < 0)
								goto b; // use LayoutStrip

							// second, this pile can't migrate up if it has a marker which 
							// causes wrap, so check that possibility out
							if (pApp->m_bMarkerWrapsStrip) // set by menu item on View menu,
															// default is TRUE
							{
								if (!pApp->m_pActivePile->m_pSrcPhrase->m_markers.IsEmpty())
								{
									if (pView->IsWrapMarker(pApp->m_pActivePile->m_pSrcPhrase))
										// a marker on the source phrase causes wrap, 
										// so just use LayoutStrip
										goto b;
								}
							}

							// if we get here, then the pile potentially could migrate up, 
							// so check it out
							pPrevStrip = pBundle->m_pStrip[nPreviousStrip];
							actualSpace = pPrevStrip->m_nFree; // slop at end of previous strip
							oldBoxWidth = currBoxSize.x; 
							textWidth = textExtent.x; // this value decreases continuously with 
								// presses of <BS> key but the phrase box shortens only until it
								// reaches the length of either the source text or the target 
								// text on pSrcPhrase; so we have to check for the attainment of
								// the minimum width, and when we get there, use that in the
								// calculation for newBoxWidth
							dC.GetTextExtent(pApp->m_pActivePile->m_pSrcPhrase->m_srcPhrase, 
								&textExtent2.x, &textExtent2.y);
							dC.GetTextExtent(pApp->m_pActivePile->m_pSrcPhrase->m_targetStr, 
								&textExtent3.x, &textExtent3.y);

							srcPhraseMaxWidth = textExtent3.x >= textExtent2.x ? 
															textExtent3.x : textExtent2.x;
							if (gbIsGlossing)
							{
								dC.GetTextExtent(pApp->m_pActivePile->m_pSrcPhrase->m_gloss, 
									&textExtent4.x, &textExtent4.y);
								srcPhraseMaxWidth = srcPhraseMaxWidth >= textExtent4.x ?
															srcPhraseMaxWidth : textExtent4.x;
							}
							if (srcPhraseMaxWidth > textWidth)
								textWidth = srcPhraseMaxWidth;
							newBoxWidth = textWidth + gnExpandBox*charSize.x;
							neededWidth = pApp->m_curGapWidth + newBoxWidth;
							if (neededWidth <= actualSpace)
								// it will migrate up
								goto c; // do the full update
							else
								// it won't migrate up, but perhaps first pile of following
								// strip will migrate up
								goto a;
						}
						else
						{
							// we are not at the first pile, so the only effect that shorening
							// by deletions can have is that the first pile of the next strip may
							// migrate up to the end of the current strip, so check for that now
							goto a;
						}

						// first check if first pile on next strip has a marker which forces strip
						// wrap - if so, no movement of the first pile up to the current strip will
						// be possible, and so provided the current pile is not the first of the
						// current strip, we can just do a LayoutStrip rather than a full update
a:						if (pApp->m_bMarkerWrapsStrip) // set by menu item on View menu, default
														// is TRUE
						{
							if (!pFirstSrcPhrase->m_markers.IsEmpty())
							{
								if (pView->IsWrapMarker(pFirstSrcPhrase))
									// a marker on the source phrase causes wrap, 
									// so just use LayoutStrip
									goto b;
							}
						}

						// if we get here, then the possibility of the first pile of the next strip
						// migrating up to the end of the current strip exists, so check it out now
						oldBoxWidth = currBoxSize.x;
						textWidth = textExtent.x; // this value decreases continuously with
							// presses of <BS> key but the phrase box shortens only until it 
							// reaches the length of either the source text or the target text
							// on pSrcPhrase; so we have to check for the attainment of the
							// minimum width, and when we get there, use that in the calculation
							// for newBoxWidth
						dC.GetTextExtent(pApp->m_pActivePile->m_pSrcPhrase->m_srcPhrase, 
							&textExtent2.x, &textExtent2.y);
						dC.GetTextExtent(pApp->m_pActivePile->m_pSrcPhrase->m_targetStr, 
							&textExtent3.x, &textExtent3.y);
						srcPhraseMaxWidth = textExtent3.x >= textExtent2.x ? textExtent3.x :
																				textExtent2.x; 
						if (gbIsGlossing)
						{
							dC.GetTextExtent(pApp->m_pActivePile->m_pSrcPhrase->m_gloss, 
								&textExtent4.x, &textExtent4.y);
							srcPhraseMaxWidth = srcPhraseMaxWidth >= textExtent4.x ?
														srcPhraseMaxWidth : textExtent4.x;
						}
						if (srcPhraseMaxWidth > textWidth)
							textWidth = srcPhraseMaxWidth;
						newBoxWidth = textWidth + gnExpandBox*charSize.x;
						boxWidthDifference = oldBoxWidth - newBoxWidth;

						// check first pile of next strip for its width & compute space needed,
						// then compare with actual space available to see if first pile will 
						// move up
						nWidthOfFirstPile = pFirstPile->m_rectPile.GetWidth();
						neededWidth = pApp->m_curGapWidth + nWidthOfFirstPile;
						actualSpace = pStrip->m_nFree + boxWidthDifference;
						if (neededWidth <= actualSpace)
						{
							// first pile of following strip can migrate to end of current strip,
							// so we need to update the whole layout
c:							pView->RecalcLayout(pApp->m_pSourcePhrases,nCurrentStrip,pApp->m_pBundle);
						}
						else
						{
							// no migration is possible, so just layout the strip
b:							bWholeScreenUpdate = FALSE; // used at end of this function
							pView->LayoutStrip(pApp->m_pSourcePhrases,nCurrentStrip,pApp->m_pBundle);
						}
					}
					else
					{
						// there is no next strip, so no migration up is possible, 
						// so layout the strip only
						bWholeScreenUpdate = FALSE; // used at end of this function
						pView->LayoutStrip(pApp->m_pSourcePhrases,
														nCurrentStrip,pApp->m_pBundle);
					}
				}

				// recalculate the active pile pointer, as the old one was clobbered by 
				// doing the new layout
				pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
				wxASSERT(pApp->m_pActivePile != NULL);
				m_pActivePile = pApp->m_pActivePile; // put a copy in the phraseBox too
				pApp->m_ptCurBoxLocation = pApp->m_pActivePile->m_pCell[2]->m_ptTopLeft;
				
				if (nSelector == 2)
				{
					// we are trying to delete text in the phrase box by pressing backspace key
					// shrink the box by 2 'w' widths if the space at end is >= 4 'w' widths
					if (pApp->m_curBoxWidth > pApp->m_pActivePile->m_nWidth)
					{
						int newMinWidth = pApp->m_curBoxWidth - 2 * charSize.x;
						pApp->m_curBoxWidth = pApp->m_pActivePile->m_nWidth;
						pApp->m_pActivePile->m_nMinWidth = newMinWidth;
					}
				}
			} // end block for nSelector == 2 case
			else
			{
				// nSelector == 1 case
				pApp->m_curBoxWidth = textExtent.x + gnExpandBox*charSize.x;

				// move the old code into here
				GetSelection(&pApp->m_nStartChar,&pApp->m_nEndChar); // store current selection

				// if the box's right edge goes beyond the strip's right boundary, it needs
				// to be recreated on the next line because the phrase it is below will have
				// been automatically moved to the next line
				int nCurrentStrip = pApp->m_pActivePile->m_pStrip->m_nStripIndex;
				CStrip* pStrip = pApp->m_pActivePile->m_pStrip;
				wxRect rectStrip = pStrip->m_rectStrip;
				wxRect rectPile = pApp->m_pActivePile->m_rectPile;
				int nStartPile = rectPile.GetLeft();
				int nEndStrip = rectStrip.GetRight();

				// recalculate the layout at the appropriate place
				if (nSelector < 2)
				{
					if (nStartPile + pApp->m_curBoxWidth >= nEndStrip)
					{
						int nNextStrip = nCurrentStrip + 1;

						// calculate the appropriate m_ptCurBoxLocation for the first pile
						// in the next strip
						CSourceBundle* pBundle = pStrip->m_pBundle;
						pStrip = pBundle->m_pStrip[nNextStrip]; // the next strip
						pApp->m_ptCurBoxLocation = pStrip->m_pPile[0]->m_pCell[2]->m_ptTopLeft;
					}
				}
				pView->RecalcLayout(pApp->m_pSourcePhrases,0,pApp->m_pBundle);

				// recalculate the active pile pointer, as the old one was clobbered by 
				// doing the new layout
				pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
				wxASSERT(pApp->m_pActivePile != NULL);
				m_pActivePile = pApp->m_pActivePile; // put a copy in the phraseBox too
				pApp->m_ptCurBoxLocation = pApp->m_pActivePile->m_pCell[2]->m_ptTopLeft;
				
			} // end block for nSelector == 1 case
		} // end nSelector != 0 block

		// resize using the stored information (version 1.2.3 and earlier used to recreate, but
		// this conflicted with Keyman (which sends backspace sequences to delete matched char 
		// sequences to be converted, so I changed to a resize for version 1.2.4 and onwards.
		// Everything seems fine.
		// wx version: In the MFC version there was a CreateBox function as well as a ResizeBox
		// function used here. I have simplified the code to use ResizeBox everywhere, since
		// the legacy CreateBox now no longer recreates the phrasebox each time it's called.
		pView->ResizeBox(&pApp->m_ptCurBoxLocation,pApp->m_curBoxWidth,pApp->m_nTgtHeight,
				pApp->m_targetPhrase,pApp->m_nStartChar,pApp->m_nEndChar,pApp->m_pActivePile);

		if (bWasMadeDirty)
			pApp->m_pTargetBox->MarkDirty(); // TRUE (restore modified status)

		gbExpanding = FALSE;
		if (bWholeScreenUpdate)
		{
			pView->Invalidate();
		}

	} // end bResult == TRUE block
	if (nSelector < 2)
		pApp->m_targetPhrase = thePhrase; // update the string storage on the view 
			// (do it here rather than before the resizing code else selection bounds are wrong)

	dC.SetFont(SaveFont); // restore old font (ie "System")
	gSaveTargetPhrase = pApp->m_targetPhrase;

	// whm Note re BEW's note below: the non-visible phrasebox was not a problem in the wx version.
	// BEW added 20Dec07: in Reviewing mode, the box does not always get drawn (eg. if click on a
	// strip which is all holes and then advance the box by using Enter key, the box remains invisible,
	// and stays so for subsequent Enter presses in later holes in the same and following strips:
	// same addition is at the end of the ResizeBox() function, for the same reason
	//pView->m_targetBox.Invalidate(); // hopefully this will fix it - it doesn't unfortunately
	 // perhaps the box paint occurs too early and the view point wipes it. How then do we delay
	 // the box paint? Maybe put the invalidate call into the View's OnDraw() at the end of its handler?
	//pView->RemakePhraseBox(pView->m_pActivePile, pView->m_targetPhrase); // also doesn't work.
}
