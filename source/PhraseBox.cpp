/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			PhraseBox.cpp
/// \author			Bill Martin
/// \date_created	11 February 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CPhraseBox class.
/// The CPhraseBox class governs the behavior of the phrase or
/// target box where the user enters and/or edits translations while adapting text.
/// \derivation		The PhraseBox class derives from the wxTextCtrl class.
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

// defines for debugging purposes
//#define _FIND_DELAY
//#define _AUTO_INS_BUG
//#define LOOKUP_FEEDBACK
//#define DROPDOWN
#define   REPOPULATE
#define   SHOWSYNC


#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

#include <wx/docview.h>

// Other includes uncomment as implemented
#include "Adapt_It.h"
#include "PhraseBox.h"
#include "Adapt_ItView.h"
#include "Adapt_ItCanvas.h"
#include "Cell.h"
#include "Pile.h"
#include "Strip.h"
#include "SourcePhrase.h"
#include "Adapt_ItDoc.h"
#include "Layout.h"
#include "RefString.h"
#include "AdaptitConstants.h"
#include "KB.h"
#include "TargetUnit.h"
#include "ChooseTranslation.h"
#include "MainFrm.h"
#include "Placeholder.h"
#include "ExportFunctions.h"
#include "helpers.h"
// Other includes uncomment as implemented

// globals

extern bool gbVerticalEditInProgress;
extern EditStep gEditStep;
extern EditRecord gEditRecord;
extern CAdapt_ItApp* gpApp; // to access it fast
extern int gnBoxCursorOffset;  // use with 'cursor_at_offset' enum value in SetCursorGlobals()


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
extern wxChar gcharNonSrcUC;

// ditto, these ones
extern wxChar gcharSrcUC;
extern wxChar gcharSrcLC;
extern wxChar gcharNonSrcLC;

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

/// This global is defined in Adapt_It.cpp.
extern bool	gbRTL_Layout;	// ANSI version is always left to right reading; this flag can only
							// be changed in the NRoman version, using the extra Layout menu

/// This global is defined in Adapt_It.cpp
extern int ID_PHRASE_BOX; // 22030 

/// This global is defined in Adapt_It.cpp.
extern int ID_BMTOGGLEBUTTON_PHRASEBOX;

/// This global is defined in Adapt_It.cpp.
extern int ID_DROP_DOWN_LIST;

IMPLEMENT_DYNAMIC_CLASS(CPhraseBox, wxTextCtrl)

BEGIN_EVENT_TABLE(CPhraseBox, wxTextCtrl)
	// whm 11Nov2022 removed the following EVT_MENU OnEditUndo entry. 
	// CPhraseBox::OnEditUndo() is only called by the View's OnEditUndo() handler.
    //EVT_MENU(wxID_UNDO, CPhraseBox::OnEditUndo)
    EVT_TEXT(ID_PHRASE_BOX, CPhraseBox::OnPhraseBoxChanged)
    EVT_CHAR(CPhraseBox::OnChar)
    EVT_KEY_DOWN(CPhraseBox::OnKeyDown)
    EVT_KEY_UP(CPhraseBox::OnKeyUp)
    EVT_LEFT_DOWN(CPhraseBox::OnLButtonDown)
    EVT_LEFT_UP(CPhraseBox::OnLButtonUp)
    // whm 12Jul2018 Note: The events for the handlers below are now caught
    // CAdapt_ItCanvas's even table at the beginning of Adapt_ItCanvas.cpp 
    // where its handlers of the same name simply call the actual handlers
    // here in CPhraseBox. Hence these even table macros are commented out.
    //EVT_BUTTON(ID_BMTOGGLEBUTTON_PHRASEBOX, CPhraseBox::OnTogglePhraseBoxButton)
    //EVT_LISTBOX(ID_DROP_DOWN_LIST,CPhraseBox::OnListBoxItemSelected)
    //EVT_LISTBOX_DCLICK(ID_DROP_DOWN_LIST, CPhraseBox::OnListBoxItemSelected)
END_EVENT_TABLE()

CPhraseBox::CPhraseBox(void)
{
    // whm 10Jan2018 Note: This default constructor will never be called in
    // the current codebase. The PhraseBox is only created in one location in
    // the codebase in the CAdapt_ItView::OnCreate() function. 
    /*
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

    m_textColor = wxColour(0, 0, 0); // default to black
    m_bCurrentCopySrcPunctuationFlag = TRUE; // default
    // whm Note: The above initializations composed all CPhraseBox constructor initializations
    // before the addition of the custom constructor below for the dropdown phrasebox.

    */
}

// whm Note: Below are some xpm images that were experimental in developing the dropdown
// button for the PhraseBox. These were used within the CPhraseBox constructor.
// Save for future reference.
//
//    // Use custom dropdown control buttons
//    //  /* XPM */
//    const char * xpm_dropbutton_hover[] = {
//        /* columns rows colors chars-per-pixel */
//        "14 15 47 1 ",
//        "  c black",
//        ". c #588EF1",
//        "X c #598EF1",
//        "o c #588FF1",
//        "O c #598FF1",
//        "+ c #5C91F1",
//        "@ c #6194F2",
//        "# c #6597F2",
//        "$ c #6A99F2",
//        "% c #6C9CF3",
//        "& c #719FF4",
//        "* c #75A2F4",
//        "= c #7AA4F3",
//        "- c #7DA7F4",
//        "; c #81AAF5",
//        ": c #85ADF5",
//        "> c #89B0F5",
//        ", c #8EB2F5",
//        "< c #91B5F6",
//        "1 c #92B6F6",
//        "2 c #96B8F7",
//        "3 c #9ABBF7",
//        "4 c #9DBDF6",
//        "5 c #9EBEF7",
//        "6 c #A2C0F7",
//        "7 c #AAC6F7",
//        "8 c #A6C3F8",
//        "9 c #AFC9F8",
//        "0 c #AEC8F9",
//        "q c #B2CBF8",
//        "w c #B6CEF9",
//        "e c #BBD1F9",
//        "r c #C3D6FA",
//        "t c #C7D9FA",
//        "y c #CBDCFA",
//        "u c #CFDEFB",
//        "i c #D3E1FC",
//        "p c #D7E4FC",
//        "a c #DAE6FC",
//        "s c #DFE9FC",
//        "d c #E3ECFD",
//        "f c #E7EEFD",
//        "g c #E8EFFD",
//        "h c #EBF1FD",
//        "j c #EFF4FD",
//        "k c #F3F7FE",
//        "l c None",
//        /* pixels */
//        "llllllllllllll",
//        "llll      llll",
//        "llll +XX+ llll",
//        "llll %@oX llll",
//        "llll -&#X llll",
//        "llll ,;*$ llll",
//        "llll 4<:= llll",
//        "llll 062> llll",
//        "l     q83    l",
//        "ll faurw74< ll",
//        "lll hsiteq lll",
//        "llll jdpy llll",
//        "lllll kg lllll",
//        "llllll  llllll",
//        "llllllllllllll"
//    };
//
//    //  /* XPM */
//    const char * xpm_dropbutton_pressed[] = {
//        /* columns rows colors chars-per-pixel */
//        "14 15 31 1 ",
//        "  c black",
//        ". c #000DBC",
//        "X c #0713BD",
//        "o c #0814BE",
//        "O c #1521C1",
//        "+ c #1722C2",
//        "@ c #232DC5",
//        "# c #252EC5",
//        "$ c #323AC9",
//        "% c #343CC9",
//        "& c #3F47CC",
//        "* c #4148CC",
//        "= c #4F55CF",
//        "- c #5056D0",
//        "; c #5C60D3",
//        ": c #5D61D3",
//        "> c #6B6ED6",
//        ", c #6C6FD6",
//        "< c #6E71D7",
//        "1 c #787ADA",
//        "2 c #7A7CDA",
//        "3 c #7B7DDB",
//        "4 c #8788DE",
//        "5 c #8889DE",
//        "6 c #9595E1",
//        "7 c #9797E1",
//        "8 c #A3A2E4",
//        "9 c #A5A2E4",
//        "0 c #AEABE7",
//        "q c #AEACE7",
//        "w c None",
//        /* pixels */
//        "wwwwwwwwwwwwww",
//        "wwww      wwww",
//        "wwww .... wwww",
//        "wwww X... wwww",
//        "wwww @Oo. wwww",
//        "wwww &$#+ wwww",
//        "wwww :=*% wwww",
//        "wwww 1>:- wwww",
//        "w     42,    w",
//        "ww 0q08652< ww",
//        "www 000097 www",
//        "wwww 0q0q wwww",
//        "wwwww qq wwwww",
//        "wwwwww  wwwwww",
//        "wwwwwwwwwwwwww"
//    };
//
//    //  /* XPM */
//    const char * xpm_dropbutton_normal[] = {
//        /* columns rows colors chars-per-pixel */
//        "14 15 3 1 ",
//        "  c black",
//        ". c gray100",
//        "X c None",
//        /* pixels */
//        "XXXXXXXXXXXXXX",
//        "XXXX      XXXX",
//        "XXXX .... XXXX",
//        "XXXX .... XXXX",
//        "XXXX .... XXXX",
//        "XXXX .... XXXX",
//        "XXXX .... XXXX",
//        "XXXX .... XXXX",
//        "X     ...    X",
//        "XX ........ XX",
//        "XXX ...... XXX",
//        "XXXX .... XXXX",
//        "XXXXX .. XXXXX",
//        "XXXXXX  XXXXXX",
//        "XXXXXXXXXXXXXX"
//    };
//
//    //  /* XPM */
//    const char * xpm_dropbutton_disabled[] = {
//        /* columns rows colors chars-per-pixel */
//        "1 15 3 1 ",  // TODO: whm - Test this. It is a one-pixel wide image of transparent pixels
//        "  c black",
//        ". c gray100",
//        "X c None",
//        /* pixels */
//        "X",
//        "X",
//        "X",
//        "X",
//        "X",
//        "X",
//        "X",
//        "X",
//        "X",
//        "X",
//        "X",
//        "X",
//        "X",
//        "X",
//        "X"
//    };
//
//    // Custom dropdown control button for blank button (no visible dropdown arror)
//    //  /* XPM */
//    const char * xpm_dropbutton_X[] = {
//        /* columns rows colors chars-per-pixel */
//        "15 18 4 1 ",  // TODO: whm - Test this. It is a one-pixel wide image of transparent pixels
//        "  c black",
//        ". c gray100",
//        "r c #FFE4E1", // misty rose
//        "X c None",
//        /* pixels */
//        //"XXXXXXXXXXXXXX",
//        //"XXXXXXXXXXX   ",
//        //"XXXXX   XX   X",
//        //"XXX          X",
//        //"XX   XXX    XX",
//        //"XX  XXX     XX",
//        //"X  XXX   XX  X",
//        //"X  XX   XXX  X",
//        //"X  X   XXXX  X",
//        //"X     XXXX  XX",
//        //"XX   XXX    XX",
//        //"X          XXX",
//        //"   X     XXXXX",
//        //"  XXXXXXXXXXXX",
//        //"XXXXXXXXXXXXXX"
//
//        "               ",
//        " rrrrrrrrrrrrr ",
//        " rrrrrrrrrrrrr ",
//        "  rrrrrrrrrrr  ",
//        " r rrrrrrrrr r ",
//        " rr rrrrrrr rr ",
//        " rrr rrrrr rrr ",
//        " rrrr rrr rrrr ",
//        " rrrrr r rrrrr ",
//        " rrrrr r rrrrr ",
//        " rrrr rrr rrrr ",
//        " rrr rrrrr rrr ",
//        " rr rrrrrrr rr ",
//        " r rrrrrrrrr r ",
//        "  rrrrrrrrrrr  ",
//        " rrrrrrrrrrrrr ",
//        " rrrrrrrrrrrrr ",
//        "               "
//    };


CPhraseBox::CPhraseBox(wxWindow * parent, wxWindowID id, const wxString & value, const wxPoint & pos, const wxSize & size, int style)
    :wxTextCtrl(parent,id,value,pos,size,style)
{
    // whm 10Jan2018 Note: This custom constructor is now the only constructor that will
    // be called in the current codebase. The PhraseBox is only created in one location in
    // the codebase in the CAdapt_ItView::OnCreate() function. 

    m_pTextCtrl = (wxTextCtrl*)NULL; // Globally, this private pointer points to App's m_pTargetBox
    m_pDropDownList = (CMyListBox*)NULL;
    m_pPhraseBoxButton = (wxBitmapToggleButton*)NULL;

    // This member repeated here from the default constructor
    m_textColor = wxColour(0,0,0); // default to black
        
    // This member repeated here from the default constructor
    m_bCurrentCopySrcPunctuationFlag = TRUE; // default
    
    // whm Note: The above members were all repeated here in the custom constructor from the
    // original default constructor. 
    // The following CPhraseBox members were moved here and renamed from global space. 
    // Some subsequently removed or moved (and commented out here) as mentioned in comments.
    // The original comments that appeared with the globals are preserved under my new comments
    // after a blank comment line.
    
    // whm 24Feb2018 The m_bMergeSucceeded member was originally named gbMergeSucceeded.
    // It was originally declared in PhraseBox.cpp's global space (but not initialized there).
    // Although I initially had it as a member of CPhraseBox, I moved it to become a member of 
    // CAdapt_ItApp and initialized it there where it still functions as intended.
    //
    // [no original documenting comment] 
    // whm Note: m_bMergeSucceeded is used in View's OnReplace() function, and in
    // CPhraseBox::OnPhraseBoxChanged() and CPhraseBox::OnChar() where it functions as intended.
    //m_bMergeSucceeded = FALSE; // whm Note: functions as intended as App member
    
    // whm 24Feb2018 The m_bSuppressDefaultAdaptation member was originally named bSuppressDefaultAdaptation.
    // It was originally declared in PhraseBox's global space (but not initialised there).
    // Although I initially had it as a member of CPhraseBox, I moved it to become a member of
    // CAdapt_ItApp and initialized it there where it still functions as intended.
    //
    // m_bSuppressDefaultAdaptation normally FALSE, but set TRUE whenever user is
    // wanting a MergeWords done by typing into the phrase box (which also
    // ensures cons.changes won't be done on the typing)- actually more complex than
    // this, see CPhraseBox OnChar()
    //m_bSuppressDefaultAdaptation = FALSE; // whm Note: functions as intended as App member
    
    // whm 24Feb2018 The pCurTargetUnit member was originally declared in 
    // PhraseBox's global space (initialized to NULL).
    // Although I initially had it as a member of CPhraseBox, I moved it to become a member of
    // CAdapt_ItAPP and initialized it there where it still functions as intended.
    //
    // when non-NULL, pCurTargetUnit is the matched CTargetUnit instance from the Choose Translation dialog
    //pCurTargetUnit = (CTargetUnit*)NULL; // whm Note: functions as intended as App member
    
    // whm 24Feb2018 moved to constructor and initialized here via .Empty(). It originally was 
    // named gSaveTargetPhrase and initialized to _T("") in PhraseBox.cpp's global space. 
    //
    // m_SaveTargetPhrase for use by the SHIFT+END shortcut for unmerging a phrase
    m_SaveTargetPhrase.Empty(); 
    
    // whm 24Feb2018 moved to constructor and initialized here to FALSE. It originally was
    // named gbRetainBoxContents and was declared in PhraseBox.cpp's global space.
    //
    // for version 1.4.2; we want deselection of copied source text to set the 
    // m_bRetainBoxContents flag true so that if the user subsequently selects words intending
    // to do a merge, then the deselected word won't get lost when he types something after
    // forming the source words selection (see OnKeyUp( ) for one place the flag is set -
    // (for a left or right arrow keypress), and the other place will be in the view's
    // OnLButtonDown I think - for a click on the phrase box itself)
    m_bRetainBoxContents = FALSE; // whm moved to constructor - originally was initialized in PhraseBox.cpp's global space
    
    // whm Note: m_bBoxTextByCopyOnly was originally named gbByCopyOnly and was 
    // declared and initialized in PhraseBox.cpp's global space, but I made it a
    // CPhraseBox member and moved its initialization here.
    //
    // will be set TRUE when the target text is the result of a
    // copy operation from the source, and if user types to modify it, it is
    // cleared to FALSE, and similarly, if a lookup succeeds, it is cleared to
    // FALSE. It is used in PlacePhraseBox() to enforce a store to the KB when
    // user clicks elsewhere after an existing location is returned to somehow,
    // and the user did not type anything to the target text retrieved from the
    // KB. In this circumstance the m_bAbandonable flag is TRUE, and the retrieved
    // target text would not be re-stored unless we have this extra flag
    // m_bBoxTextByCopyOnly to check, and when FALSE we enforce the store operation
    m_bBoxTextByCopyOnly = FALSE;
    
    // whm Note: m_bTunnellingOut was originally named gbTunnellingOut and was
    // declared and initialized in PhraseBox.cpp's global space, but I made it a
    // CPhraseBox member and moved its initialization here.
    //
    // TRUE when control needs to tunnel out of nested procedures when
    // gbVerticalEditInProgress is TRUE and a step-changing custom message
    // has been posted in order to transition to a different edit step;
    // FALSE (default) in all other circumstances
    m_bTunnellingOut = FALSE;
    
    // whm Note: m_bSavedTargetStringWithPunctInReviewingMode was originally named
    // gbSavedTargetStringWithPunctInReviewingMode and was declared and initialized 
    // in PhraseBox.cpp's global space, but I made it a CPhraseBox member and moved 
    // its initialization here.
    //
    // TRUE if either or both of m_adaption and m_targetStr is empty
    // and Reviewing mode is one (we want to preserve punctuation or
    // lack thereof if the location is a hole)
    m_bSavedTargetStringWithPunctInReviewingMode = FALSE;
    
    // whm Note: m_StrSavedTargetStringWithPunctInReviewingMode was originally named
    // gStrSavedTargetStringWithPunctInReviewingMode and was declared and initialized 
    // in PhraseBox.cpp's global space, but I made it a CPhraseBox member and moved 
    // its initialization here.
    //
    // works with the above flag, and stores whatever the m_targetStr was
    // when the phrase box, in Reviewing mode, lands on a hole (we want to
    // preserve what we found if the user has not changed it)    
    m_StrSavedTargetStringWithPunctInReviewingMode.Empty();
    
    // whm Note: m_bNoAdaptationRemovalRequested was originally named
    // gbNoAdaptationRemovalRequested and was declared and initialized 
    // in PhraseBox.cpp's global space, but I made it a CPhraseBox member and moved 
    // its initialization here.
    //
    // TRUE when user hits backspace or DEL key to try remove an earlier
    // assignment of <no adaptation> to the word or phrase at the active
    // location - (affects one of m_bHasKBEntry or m_bHasGlossingKBEntry
    // depending on the current mode, and removes the KB CRefString (if
    // the reference count is 1) or decrements the count, as the case may be)
    m_bNoAdaptationRemovalRequested = FALSE;
        
    // whm Note: m_bCameToEnd was originally named gbCameToEnd and was declared and initialized 
    // in PhraseBox.cpp's global space, but I made it a CPhraseBox member and moved its 
    // initialization here.
    //
    /// Used to delay the message that user has come to the end, until after last
    /// adaptation has been made visible in the main window; in OnePass() only, not JumpForward().
    m_bCameToEnd = FALSE;
    
    // whm Note: m_bTemporarilySuspendAltBKSP was originally named
    // gTemporarilySuspendAltBKSP and was declared and initialized 
    // in PhraseBox.cpp's global space, but I made it a CPhraseBox member and moved 
    // its initialization here.
    //
    // to enable m_bSuppressStoreForAltBackspaceKeypress flag to be turned
    // back on when <Not In KB> next encountered after being off for one
    // or more ordinary KB entry insertions; CTRL+ENTER also gives same result
    m_bTemporarilySuspendAltBKSP = FALSE;
        
    // whm Note: m_bSuppressStoreForAltBackspaceKeypress was originally named
    // gbSuppressStoreForAltBackspaceKeypress and was declared and initialized 
    // in PhraseBox.cpp's global space, but I made it a CPhraseBox member and moved 
    // its initialization here.
    //
    /// To support the ALT+Backspace key combination for advance to immediate next pile without lookup or
    /// store of the phrase box (copied, and perhaps SILConverters converted) text string in the KB or
    /// Glossing KB. When ALT+Backpace is done, this is temporarily set TRUE and restored to FALSE
    /// immediately after the store is skipped. CTRL+ENTER also can be used for the transliteration.
    m_bSuppressStoreForAltBackspaceKeypress = FALSE;
    
    // whm 24Feb2018 m_bSuppressMergeInMoveToNextPile was originally named gbSuppressMergeInMoveToNextPile
    // and was declared and initialized in PhraseBox.cpp's global space, but I made it a CPhraseBox
    // member and moved its initialization here. It was accidentally removed from code 22Feb2018, but
    // restored again 24Feb2018.
    // 
    // if a merge is done in LookAhead() so that the
    // phrase box can be shown at the correct location when the Choose Translation
    // dialog has to be put up because of non-unique translations, then on return
    // to MoveToNextPile() with an adaptation chosen in the dialog dialog will
    // come to code for merging (in the case when no dialog was needed), and if
    // not suppressed by this flag, a merge of an extra word or words is wrongly
    // done
    m_bSuppressMergeInMoveToNextPile = FALSE; 
    
    // whm 24Feb2018 m_bCompletedMergeAndMove was originally named gbCompletedMergeAndMove
    // and was declared and initialized in PhraseBox.cpp's global space, but I made it a
    // CPhraseBox member and moved ints initialization here. 
    //
    // for support of Bill Martin's wish that the phrase box
    // be at the new location when the Choose Translation dialog is shown
    m_bCompletedMergeAndMove = FALSE; 
    
    // whm 24Feb2018 m_bInhibitMakeTargetStringCall was originally named gbInhibitMakeTargetStringCall
    // and was originally declared and initialized in Adapt_ItView.cpp's global space. I renamed it and
    // moved its declaration and initialization to CAdapt_ItApp where it still functions as intended.
    //
    // Used for inhibiting multiple accesses to MakeTargetStringIncludingPunctuation when only one is needed.
    //m_bInhibitMakeTargetStringCall = FALSE; // whm Note: functions as intended as App member
    
    // whm 24Feb2018 m_nWordsInPhrase was originally named nWordsInPhrase. It was originally declared and
    // initialized in PhraseBox.cpp's global space to 0. I made it a member of CPhraseBox and initialized here.
    //
    // a matched phrase's number of words (from source phrase)
    m_nWordsInPhrase = 0;
    
    // whm 24Feb2018 m_CurKey was originally named curKey. It was originally declared and 
    // initialized to _T("") in PhraseBox.cpp's global space. I made it a member of CPhraseBox
    // and initialize it to .Empty() here. To better distinguish this CPhraseBox member from a
    // similarly named vairable in KBEditor.cpp, I renamed the variable in KBEditor.cpp to 
    // m_currentKey.
    // 
    // when non empty, it is the current key string which was matched
    m_CurKey.Empty(); 
    
    // whm 24Feb2018 m_Translation was originally named translation. It was originally 
    // declared and initialized to _T("") in PhraseBox.cpp's global space. I made it a member
    // of CPhraseBox and initialized it to .Empty() here. It appears that it was originally
    // intended to be a global that just held the result of a selection or new translation in the 
    // Choose Translation dialog - but seems to have been extended later to have a wider use.
    //
    // A wxString containing the translation for a matched source phrase key.
    m_Translation.Empty(); // = _T("") whm added 8Aug04 // translation, for a matched source phrase key
    
    // whm 24Feb2018 m_bEmptyAdaptationChosen was originally named gbEmptyAdaptationChosen and was
    // declared and initialized to FALSE in Adapt_ItView.cpp's global space. I made it a member of
    // CPhraseBox and initialized it to FALSE here.
    //
    // bool set by ChooseTranslation, when user selects <no adaptation>, then PhraseBox will
    // not use CopySource() but instead use an empty string for the adaptation
    m_bEmptyAdaptationChosen = FALSE;
    
    // BEW added 7May18, initialize the saved ref string's pointer to NULL
    InitializeComboLandingParams();

    // whm 15Jul2018 added the following bool value to determine if user presses Up or Down arrow
    // to highlight a different item in the dropdown list before pressing Enter/Tab to leave the 
    // current location. One-time initialization to FALSE is made here but it is set to FALSE at
    // each location within the Layout's PlaceBox() function.
    bUp_DownArrowKeyPressed = FALSE; // initialized to FALSE at each location - at end of Layout's PlaceBox().

    // whm 16Jul2018 added to implement undo of phrasebox changes via Esc key. We initialize it
    // to an empty string here, but it gets assigned the initial content of the phrasebox near the
    // end of the Layout's PlaceBox() function.
    initialPhraseBoxContentsOnLanding = _T("");

    ////  /* XPM */
    //const char * xpm_dropbutton_normal[] = {
    //    /* columns rows colors chars-per-pixel */
    //    "14 15 3 1 ",
    //    "  c black",
    //    ". c gray100",
    //    "X c None",
    //    /* pixels */
    //    "XXXXXXXXXXXXXX",
    //    "XXXX      XXXX",
    //    "XXXX .... XXXX",
    //    "XXXX .... XXXX",
    //    "XXXX .... XXXX",
    //    "XXXX .... XXXX",
    //    "XXXX .... XXXX",
    //    "XXXX .... XXXX",
    //    "X     ...    X",
    //    "XX ........ XX",
    //    "XXX ...... XXX",
    //    "XXXX .... XXXX",
    //    "XXXXX .. XXXXX",
    //    "XXXXXX  XXXXXX",
    //    "XXXXXXXXXXXXXX"
    //};

    /* XPM */
    const  char * xpm_dropbutton_normal[] = {
    "16 20 50 1",
    " 	c None",
    ".	c #070506",
    "+	c #070303",
    "@	c #060808",
    "#	c #044151",
    "$	c #02657E",
    "%	c #044353",
    "&	c #060607",
    "*	c #03566C",
    "=	c #0088AA",
    "-	c #025A70",
    ";	c #070406",
    ">	c #070606",
    ",	c #070707",
    "'	c #070405",
    ")	c #070505",
    "!	c #080404",
    "~	c #070809",
    "{	c #043E4D",
    "]	c #043947",
    "^	c #051E25",
    "/	c #026B86",
    "(	c #026680",
    "_	c #07090B",
    ":	c #061B22",
    "<	c #04333F",
    "[	c #070404",
    "}	c #070607",
    "|	c #06181F",
    "1	c #0085A7",
    "2	c #034759",
    "3	c #034A5C",
    "4	c #017997",
    "5	c #070708",
    "6	c #01728E",
    "7	c #05303C",
    "8	c #051F26",
    "9	c #0087A9",
    "0	c #026983",
    "a	c #035064",
    "b	c #0086A8",
    "c	c #06191F",
    "d	c #080606",
    "e	c #017693",
    "f	c #060608",
    "g	c #05242D",
    "h	c #007E9E",
    "i	c #060A0C",
    "j	c #060202",
    "k	c #026179",
    "    .++++++.    ",
    "    @#$$$$%&    ",
    "    @*====-&    ",
    "    @*====-&    ",
    "    @*====-&    ",
    "    @*====-&    ",
    "    @*====-&    ",
    ";+>,'*====-)!>~;",
    " +{]^/====(_:<[)",
    " }|1=========2> ",
    "  .3========45  ",
    "   .6=======7}  ",
    "   ,89=====0+   ",
    "    .a====bcd   ",
    "     5e===a)    ",
    "     fg9=hi     ",
    "      '*=]5     ",
    "      j_k[      ",
    "       }',      ",
    "        [       " };
    // Custom dropdown control button for blank button (no visible dropdown arror)
    ////  /* XPM */
    //const char * xpm_dropbutton_X[] = {
    //    /* columns rows colors chars-per-pixel */
    //    "15 18 4 1 ",  // TODO: whm - Test this. It is a one-pixel wide image of transparent pixels
    //    "  c black",
    //    ". c gray100",
    //    "r c #FFE4E1", // misty rose
    //    "X c None",
    //    /* pixels */
    //    "               ",
    //    " rrrrrrrrrrrrr ",
    //    " rrrrrrrrrrrrr ",
    //    "  rrrrrrrrrrr  ",
    //    " r rrrrrrrrr r ",
    //    " rr rrrrrrr rr ",
    //    " rrr rrrrr rrr ",
    //    " rrrr rrr rrrr ",
    //    " rrrrr r rrrrr ",
    //    " rrrrr r rrrrr ",
    //    " rrrr rrr rrrr ",
    //    " rrr rrrrr rrr ",
    //    " rr rrrrrrr rr ",
    //    " r rrrrrrrrr r ",
    //    "  rrrrrrrrrrr  ",
    //    " rrrrrrrrrrrrr ",
    //    " rrrrrrrrrrrrr ",
    //    "               "
    //};

    /* XPM */
const char * xpm_dropbutton_X[] = {
    "16 20 2 1",
    " 	c None",
    ".	c #D50F00",
    "................",
    "..            ..",
    ". .          . .",
    ". .          . .",
    ".  .        .  .",
    ".   .      .   .",
    ".    .    .    .",
    ".    .    .    .",
    ".     .  .     .",
    ".      ..      .",
    ".      ..      .",
    ".     .  .     .",
    ".    .    .    .",
    ".    .    .    .",
    ".   .      .   .",
    ".  .        .  .",
    ". .          . .",
    ". .          . .",
    "..            ..",
    "................" };

    bmp_dropbutton_normal = wxBitmap(xpm_dropbutton_normal);
    bmp_dropbutton_X = wxBitmap(xpm_dropbutton_X);

}

CPhraseBox::~CPhraseBox(void)
{
}

// returns number of phrases built; a return value of zero means that we have a halt condition
// and so none could be built (eg. source phrase is a merged one would halt operation)
// BEW 13Apr10, no changes needed for support of doc version 5
// BEW 23Apr15, changed to support / as a word-breaking whitespace char, if m_bFwdSlashDelimiter is TRUE
// because Lookups will, for mergers, want to test with ZWSP in strings of two or more words, since we
// store in the kb with / replaced by ZWSP for mergers, and show strings with ZWSP in the interlinear
// layout when the source and or target strings have 2 or more words
int CPhraseBox::BuildPhrases(wxString phrases[10], int nNewSequNum, SPList* pSourcePhrases)
{
	// refactored 25Mar09, -- nothing needs to be done
	// clear the phrases array
	phrases[0] = phrases[1] = phrases[2] = phrases[3] = phrases[4] = phrases[5] = phrases[6]
		= phrases[7] = phrases[8] = phrases[9] = _T("");

	// check we are within bounds
	int nMaxIndex = pSourcePhrases->GetCount() - 1;
	if (nNewSequNum > nMaxIndex)
	{
		// this is unlikely to ever happen, but play safe just in case
		wxMessageBox(_T("Index bounds error in BuildPhrases call\n"), _T(""), wxICON_EXCLAMATION | wxOK);
		wxExit();
	}

	// find position of the active pile's source phrase in the list
	SPList::Node *pos;

	pos = pSourcePhrases->Item(nNewSequNum);

	wxASSERT(pos != NULL);
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
		// BEW 6Aug13, I previously missed altering this block when I refactored in order
		// to have glossing KB use all ten tabs. So instead of putting the key into
		// phrases[0] as before, it now must be put into whichever one of the array
		// pertains to how many words, less one, are indicated by m_nSourceWords, and
		// counter needs to be set to the latter's value rather than to 1 as used to be
		// the case
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		int theIndex = pSrcPhrase->m_nSrcWords - 1;
		phrases[theIndex] = pSrcPhrase->m_key;
		return counter = pSrcPhrase->m_nSrcWords;
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
			phrases[index] = phrases[index - 1] + PutSrcWordBreak(pSrcPhrase) + pSrcPhrase->m_key;
//#if defined(FWD_SLASH_DELIM)
			CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
			if (pApp->m_bFwdSlashDelimiter)
			{
				// BEW 23Apr15 if in a merger, we want / converted to ZWSP for the source text
				// to support lookups because we will have ZWSP rather than / in the KB
				// No changes are made if app->m_bFwdSlashDelimiter is FALSE
				phrases[index] = FwdSlashtoZWSP(phrases[index]);
			}
//#endif
			counter++;
			if (pSrcPhrase->m_bBoundary || nNewSequNum + counter > nMaxIndex)
				break;
		}
		index++;
	}
	return counter;
}

CLayout* CPhraseBox::GetLayout()
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	return pApp->m_pLayout;
}

// whm 11Nov2022 added this convenience function for phrasebox width sizing. 
// Only the .x coord text extent is returned of the str input string, based on
// the main frame's canvas. The width extent of the App's m_pNavTextFont is returned
// if gbIsGlossing && gbGlossingUsesNavFont, otherwise the width extent of the App's
// m_pTargetFont is returned.
// This convenience function does essentially the same calculation as BEW's 
// CPile::CalcExtentsBasedWidth() function, but takes the input string directly.
int CPhraseBox::GetTextExtentWidth(wxString str)
{
	if (str.IsEmpty())
		return 0;
	wxClientDC dC((wxWindow*)gpApp->GetMainFrame()->canvas);
	wxFont* pFont;
	wxSize textExtent;
	if (gbIsGlossing && gbGlossingUsesNavFont)
		pFont = gpApp->m_pNavTextFont;
	else
		pFont = gpApp->m_pTargetFont;
	wxFont SaveFont = dC.GetFont();

	dC.SetFont(*pFont);
	dC.GetTextExtent(str, &textExtent.x, &textExtent.y);
	return (int)textExtent.x;
}

// returns TRUE if the phrase box, when placed at pNextEmptyPile, would not be within a
// retranslation, or FALSE if it is within a retranslation
// Side effects:
// (1) checks for vertical edit being current, and whether or not a vertical edit step
// transitioning event has just been posted (that would be the case if the phrase box at
// the new location would be in grayed-out text), and if so, returns FALSE after setting
// the global bool m_bTunnellingOut to TRUE - so that MoveToNextPile() can be exited early
// and the vertical edit step's transition take place instead
// (2) for non-vertical edit mode, if the new location would be within a retranslation, it
// shows an informative message to the user, enables the button for copying punctuation,
// and returns FALSE
// BEW 13Apr10, no changes needed for support of doc version 5
// BEW 9Apr12, changed to support discontinuous highlight spans for auto-inserts
bool CPhraseBox::CheckPhraseBoxDoesNotLandWithinRetranslation(CAdapt_ItView* pView,
												CPile* pNextEmptyPile, CPile* pCurPile)
{
	// created for refactored view layout, 24Mar09
	wxASSERT(pNextEmptyPile);
	if (gbIsGlossing)
		return TRUE; // allow phrase box to land in a retranslation when glossing mode is ON

	// the code below will only be entered when glossing mode is OFF, that is, in adapting mode
	// BEW 9Apr12,deprecated the check and clearing of the highlighting, discontinuity in the
	// highlighted spans of auto-inserted material is now supported
	pCurPile = pCurPile; // avoid compiler warning

	if (pNextEmptyPile->GetSrcPhrase()->m_bRetranslation)
	{
		// if the lookup and jump loop comes to an empty pile which is in a retranslation,
		// we halt the loop there. If vertical editing is in progress, this halt location
		// could be either within or beyond the edit span, in which case the former means
		// we don't do any step transition yet, the latter means a step transition is
		// called for. Test for these situations and act accordingly. If we transition
		// the step, there is no point in showing the user the message below because we
		// just want transition and where the jump-landing location might be is of no interest
		if (gbVerticalEditInProgress)
		{
			// bForceTransition is FALSE in the next call
			m_bTunnellingOut = FALSE; // ensure default value set
			int nSequNum = pNextEmptyPile->GetSrcPhrase()->m_nSequNumber;
			bool bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(nSequNum,nextStep);
			if (bCommandPosted)
			{
				// don't proceed further because the current vertical edit step has ended
				m_bTunnellingOut = TRUE; // caller needs to use it
				return FALSE; // use FALSE to help caller recognise need to tunnel out of the lookup loop
			}
		}
		// IDS_NO_ACCESS_TO_RETRANS
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        gpApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_(
"Sorry, to edit or remove a retranslation you must use the toolbar buttons for those operations."),
						_T(""), wxICON_INFORMATION | wxOK);
        gpApp->m_pTargetBox->SetFocusAndSetSelectionAtLanding(); // whm 13Aug2018 modified
                                                                
        // if necessary restore default button image, and m_bCopySourcePunctuation to TRUE
		wxCommandEvent event;
		if (!GetLayout()->m_pApp->m_bCopySourcePunctuation)
		{
			pView->OnToggleEnablePunctuationCopy(event);
		}
		return FALSE;
	}
	if (gbVerticalEditInProgress)
	{
		// BEW 19Oct15 No transition of vert edit modes, and not landing in a retranslation,
		// so we can store this location on the app
		gpApp->m_vertEdit_LastActiveSequNum = pNextEmptyPile->GetSrcPhrase()->m_nSequNumber;
#if defined(_DEBUG)
		wxLogDebug(_T("VertEdit PhrBox, CheckPhraseBoxDoesNotLandWithinRetranslation() storing loc'n: %d "),
			pNextEmptyPile->GetSrcPhrase()->m_nSequNumber);
#endif
	}
	return TRUE;
}

// returns nothing
// this is a helper function to do some housecleaning tasks prior to the caller (which is
// a pile movement function such as MoveToNextPile(), returning FALSE to its caller
// BEW 13Apr10, no changes needed for support of doc version 5
// BEW 21Jun10, no changes needed for support of kbVersion 2
void CPhraseBox::DealWithUnsuccessfulStore(CAdapt_ItApp* pApp, CAdapt_ItView* pView,
										   CPile* pNextEmptyPile)
{
	if (!pApp->m_bSingleStep)
	{
		pApp->m_bAutoInsert = FALSE; // cause halt, if auto lookup & inserting is ON
	}

	// if necessary restore default button image, and m_bCopySourcePunctuation to TRUE
	wxCommandEvent event;
	if (!pApp->m_bCopySourcePunctuation)
	{
		pView->OnToggleEnablePunctuationCopy(event);
	}
	if (m_bSuppressStoreForAltBackspaceKeypress)
		m_SaveTargetPhrase.Empty();
	m_bTemporarilySuspendAltBKSP = FALSE;
	m_bSuppressStoreForAltBackspaceKeypress = FALSE; // make sure it's off before returning

    // if vertical editing is in progress, the store failure may occur with the active
    // location within the editable span, (in which case we don't want a step transition),
    // or having determined the jump location's pile is either NULL (a bundle boundary was
    // reached before an empty pile could be located - in which case a step transition
    // should be forced), or a pile located which is beyond the editable span, in the gray
    // area, in which case transition is wanted; so handle these options using the value
    // for pNextEmptyPile obtained above Note: doing a transition in this circumstance
    // means the KB does not get the phrase box contents added, but the document still has
    // the adaptation or gloss, so the impact of the failure to store is minimal (ie. if
    // the box contents were unique, the adaptation or gloss will need to occur later
    // somewhere for it to make its way into the KB)
	if (gbVerticalEditInProgress)
	{
		// bForceTransition is TRUE in the next call
		m_bTunnellingOut = FALSE; // ensure default value set
		bool bCommandPosted = FALSE;
		if (pNextEmptyPile == NULL)
		{
			bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(-1,nextStep,TRUE);
		}
		else
		{
			// bForceTransition is FALSE in the next call
			int nSequNum = pNextEmptyPile->GetSrcPhrase()->m_nSequNumber;
			bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(nSequNum,nextStep);
		}
		if (bCommandPosted)
		{
			// don't proceed further because the current vertical edit step has ended
			m_bTunnellingOut = TRUE; // caller needs to use it
			// caller unilaterally returns FALSE  when this function returns,
			// this, together with m_bTunnellingOut,  enables the caller of the caller to
			// recognise the need to tunnel out of the lookup loop
		}
		else
		{
			// BEW 19Oct15 No transition of vert edit modes,
			// so we can store this location on the app
			gpApp->m_vertEdit_LastActiveSequNum = pNextEmptyPile->GetSrcPhrase()->m_nSequNumber;
#if defined(_DEBUG)
		wxLogDebug(_T("VertEdit PhrBox, DealWithUnsuccessfulStore() storing loc'n: %d "),
			pNextEmptyPile->GetSrcPhrase()->m_nSequNumber);
#endif
		}
	}
}

// return TRUE if there were no problems encountered with the store, FALSE if there were
// (this function calls DealWithUnsuccessfulStore() if there was a problem with the store)
// BEW 13Apr10, no changes needed for support of doc version 5
// BEW 21Jun10, no changes needed for support of kbVersion 2
bool CPhraseBox::DoStore_NormalOrTransliterateModes(CAdapt_ItApp* pApp, CAdapt_ItDoc* pDoc,
		 CAdapt_ItView* pView, CPile* pCurPile, bool bIsTransliterateMode)
{
	bool bOK = TRUE;
	CSourcePhrase* pOldActiveSrcPhrase = pCurPile->GetSrcPhrase();
	pApp->m_bInNormalStore = TRUE; // MakeTargetStringIncludingPunctuation() uses this

	// m_bSuppressStoreForAltBackspaceKeypress is FALSE, so either we are in normal adapting
	// or glossing mode; or we could be in transliteration mode but the global boolean
	// happened to be FALSE because the user has just done a normal save in transliteration
	// mode because the transliterator did not produce a correct transliteration

	// we are about to leave the current phrase box location, so we must try to store what is
	// now in the box, if the relevant flags allow it. Test to determine which KB to store to.
	// StoreText( ) has been ammended for auto-capitalization support (July 2003)
	if (!gbIsGlossing)
	{
#if defined(_DEBUG)
		{
			wxLogDebug(_T("%s::%s(), line %d, sn=%d, pSrcPhrase: m_key= [%s], m_srcPhrase= [%s], m_adaption= [%s], m_targetStr= [%s]"),
				__FILE__, __FUNCTION__, __LINE__, pOldActiveSrcPhrase->m_nSequNumber, pOldActiveSrcPhrase->m_key.c_str(), 
				pOldActiveSrcPhrase->m_srcPhrase.c_str(), pOldActiveSrcPhrase->m_adaption.c_str(), pOldActiveSrcPhrase->m_targetStr.c_str() );
		}
#endif


		pView->MakeTargetStringIncludingPunctuation(pOldActiveSrcPhrase, pApp->m_targetPhrase);

#if defined(_DEBUG)
		if (pOldActiveSrcPhrase->m_nSequNumber >= 12)
		{
			int halt_here = 1;
		}
#endif

		//pOldActiveSrcPhrase->m_targetStr << pOldActiveSrcPhrase->m_follPunct; // try adding ')' after the function above returns - nope, << doesn't work here either
		// If there is a problem with initial character, try reversing, and inserting before the first char, then reverse again
		//wxString reversedStr = MakeReverse(pOldActiveSrcPhrase->m_targetStr);
		//wxLogDebug(_T("DoStore_Normal...() line %d, reversedStr= [%s]"), __LINE__, reversedStr.c_str());
		// The above line showed the problem. I expected "(5000" to be reversed as "000,5(" but it didn't. It was ")" only.
		// Bill's MakeReverse() gets the size_t for the whole string (and doing this was, in MakeTarget...() one larger than expected)
		// and allocates sufficient space before doing the reversal in a backwards loop. So the actual m_targetStr must be: _T("(5,000<null>)"
		// and so the loop, working backwards, finds ')' and then a null, and thinks that's all there is. So that's what's happening
		// in the MakeTargetStringIncludingPunctuation() function itself. Trying to append to <null> won't work, nor will assigning
		/*
		reversedStr = pOldActiveSrcPhrase->m_follPunct + reversedStr;
		wxLogDebug(_T("DoStore_Normal...() line %d, reversedStr= [%s]"), __LINE__, reversedStr.c_str());
		wxString aTargetStr = MakeReverse(reversedStr);
		wxLogDebug(_T("DoStore_Normal...() line %d, aTargetStr= [%s]"), __LINE__, aTargetStr.c_str());
		// Can I assign aTargetStr to pOldActiveSrcPhrase->m_targetStr? It's probably out of scope now
		pOldActiveSrcPhrase->m_targetStr = aTargetStr;
		wxLogDebug(_T("DoStore_Normal...() line %d, pOldActiveSrcPhrase->m_targetStr= [%s]"), __LINE__, pOldActiveSrcPhrase->m_targetStr.c_str(), aTargetStr.c_str());
		*/

#if defined (_DEBUG)		
			wxLogDebug(_T("%s::%s(), line %d, sn=%d, pSrcPhrase: m_key= [%s], m_srcPhrase= [%s], m_adaption= [%s], m_targetStr= [%s]"),
				__FILE__, __FUNCTION__, __LINE__, pOldActiveSrcPhrase->m_nSequNumber, pOldActiveSrcPhrase->m_key.c_str(),
				pOldActiveSrcPhrase->m_srcPhrase.c_str(), pOldActiveSrcPhrase->m_adaption.c_str(), pOldActiveSrcPhrase->m_targetStr.c_str());
#endif	


		// BEW 14Oct22 If I want detached ] to be treated like a word, so that it goes into the
		// adapting KB, then I don't what punctuation removed, as it's normally punctuation, as
		// that would generate m_targetPhrase being an empty string - and if that were the case
		// trying to compensate in the StoreText() call below would be impossible. So for ], 
		// skip the punctuation removal

		// BEW 28Oct22 protect against m_targetPhrase being empty, at the GetChar(0) call
		int tgtPhrLen = 0; // initialize
		if (!pApp->m_targetPhrase.IsEmpty())
		{
			tgtPhrLen = pApp->m_targetPhrase.Length();
			wxChar firstChar = pApp->m_targetPhrase.GetChar(0); // first
			if (!((tgtPhrLen == 1) && (firstChar == _T(']'))))
			{
				// suppress punctuation stripping when ] is the word passed in; but allow all else
				pView->RemovePunctuation(pDoc, &pApp->m_targetPhrase, from_target_text);
			}
		}
	}
	if (gbIsGlossing)
	{
		// BEW added next line 27Jan09
		bOK = pApp->m_pGlossingKB->StoreText(pOldActiveSrcPhrase, pApp->m_targetPhrase);
	}
	else
	{
		// BEW added next line 27Jan09
		// BEW 29Feb20, the refactored MakeTargetStringIncludingPunctuation() can, with
		// user typing in a string like you(sg) into the phrase box, result in the
		// StoreText()'s call of MakeTargetStringIncludingPunctuation() getting the final
		// ) added to passed in you(sg -- with the result that the KB takes the resulting
		// you(sg) into the KB for storage, and adding to the dropdown list a bogus 
		// 'having-final-punct' entry. To fix this, suppress the inside call of 
		// MakeTargetStringIncludingPunctuation() - by setting the app member,
		// m_bInhibitMakeTargetStringCall to TRUE
		pApp->m_bInhibitMakeTargetStringCall = TRUE;

		bOK = pApp->m_pKB->StoreText(pOldActiveSrcPhrase, pApp->m_targetPhrase);

		pApp->m_bInhibitMakeTargetStringCall = FALSE; // restore default
	}

    // if in Transliteration Mode we want to cause m_bSuppressStoreForAltBackspaceKeypress
    // be immediately turned back on, in case a <Not In KB> entry is at the next lookup
    // location and we will then want the special Transliteration Mode KB storage process
    // to be done rather than a normal empty phrasebox for such an entry
	if (bIsTransliterateMode)
	{
		m_bSuppressStoreForAltBackspaceKeypress = TRUE;
	}

	pApp->m_bInNormalStore = FALSE;
	return bOK;
}

// BEW 27Mar10 a change needed for support of doc version 5
void CPhraseBox::MakeCopyOrSetNothing(CAdapt_ItApp* pApp, CAdapt_ItView* pView,
									  CPile* pNewPile, bool& bWantSelect)
{
    // BEW 14Apr10, pass back bWantSelect as TRUE, because this will be used in caller to
    // pass the TRUE value to MoveToNextPile() which uses it to set the app members
    // m_nStartChar, m_nEndChar, to -1 and -1, and then when CLayout::PlaceBox()is called,
    // it internally calls ResizeBox() and the latter is what uses the m_nStartChar and
    // m_nEndChar values to set the phrase box content's selection. BEW 14Apr10
	bWantSelect = TRUE;

	if (pApp->m_bCopySource)
	{
		if (!pNewPile->GetSrcPhrase()->m_bNullSourcePhrase)
		{
			pApp->m_targetPhrase = pView->CopySourceKey(pNewPile->GetSrcPhrase(),
									pApp->m_bUseConsistentChanges);
		}
		else
		{
            // its a null source phrase, so we can't copy anything; and if we are glossing,
            // we just leave these empty whenever we meet them
			pApp->m_targetPhrase.Empty(); // this will cause pile's m_nMinWidth to be
										   // used for box width
		}
	}
	else
	{
		// no copy of source wanted, so just make it an empty string
		pApp->m_targetPhrase.Empty();
	}
}

// BEW 13Apr10, changes needed for support of doc version 5
// whm modified 22Feb2018 to adjust for fact that the Cancel and Select button
// no longer exists in the Choose Translation dialog with the CPhraseBox being
// implemented with a dropdown list. The m_bCancelAndSelect flag was removed 
// from this function's signature and the overall app. 
// TODO: This and the following function (for AutoAdaptMode) need their logic 
// changed to account for the removals of the legacy flags that were originally 
// called m_bCancelAndSelect and gbUserCancelledChooseTranslationDlg.
// As a temporary measure, I've made two local flags m_bCancelAndSelect_temp and
// gbUserCancelledChooseTranslationDlg_temp that are set to FALSE at the beginning 
// of the function. While the function should be revised at some point, setting the 
// flag to remain always FALSE, is just a temporary measure to ensure that I don't
// mess up the logic by a quick refactor that removes the effects of the original
// bool flags.
// BEW 16Mar18 removed Bill's two temporary flags. If I understand Bill's dropdown combo
// correctly, the removal of Choose Translation dialog automatically coming up means
// that at the location where that would happen, it would be appropriate just to halt 
// and try find appropriate KB target text to insert in the combobox there - but actually
// there may not be any reason to halt so as to invoke this legacy hack function - so it 
// may never be called. Anyway, Bill can test and determine if my foggy brain is thinking right
//
// whm 16Mar2018 restored BEW changes made 16Mar18 in attempt to find why app is now
// failing to fill text within the phrasebox and have it pre-selected.
//
//void CPhraseBox::HandleUnsuccessfulLookup_InSingleStepMode_AsBestWeCan(CAdapt_ItApp* pApp,
//    CAdapt_ItView* pView, CPile* pNewPile, bool m_bCancelAndSelect, bool& bWantSelect)
void CPhraseBox::HandleUnsuccessfulLookup_InSingleStepMode_AsBestWeCan(CAdapt_ItApp* pApp,
	CAdapt_ItView* pView, CPile* pNewPile, bool& bWantSelect)
{
    bool m_bCancelAndSelect_temp = FALSE; // whm 22Feb2018 added for temp fix
    bool gbUserCancelledChooseTranslationDlg_temp = FALSE; // whm 22Feb2018 added for temp fix
#if defined (ABANDON_NOT)
	pApp->m_pTargetBox->m_bAbandonable = FALSE;
#else
    pApp->m_pTargetBox->m_bAbandonable = TRUE;
#endif
    // it is single step mode & no adaptation available, so see if we can find a
    // translation, or gloss, for the single src word at the active location, if not,
    // depending on the m_bCopySource flag, either initialize the targetPhrase to
    // an empty string, or to a copy of the sourcePhrase's key string
    bool bGotTranslation = FALSE;

	//  BEW removed 16Mar18, whm restored 16Mar2018
    if (!gbIsGlossing && m_bCancelAndSelect_temp)
    {
        // in ChooseTranslation dialog the user wants the 'cancel and select'
        // option, and since no adaptation is therefore to be retrieved, it
        // remains just to either copy the source word or nothing...
        MakeCopyOrSetNothing(pApp, pView, pNewPile, bWantSelect);

        // BEW added 1Jul09, the flag should be TRUE if nothing was found
#if defined (ABANDON_NOT)
		pApp->m_pTargetBox->m_bAbandonable = FALSE;
#else
        pApp->m_pTargetBox->m_bAbandonable = TRUE;
#endif
    }
    else
    {
        // user didn't press the Cancel and Select button in the Choose Translation
        // dialog, but he may have pressed Cancel button, or OK button - so try to
        // find a translation given these possibilities (note, nWordsInPhrase equal
        // to 1 does not distinguish between adapting or glossing modes, so handle that in
        // the next block too)
        // whm 21Feb2018 note: the legacy gbUserCancelledChooseTranslationDlg is now 
        // removed from the app, so its logical value would be FALSE (never 
        // set to TRUE); whether a dialog was Cancelled is no longer a factor in the
        // logic of the following if ... else block. I've set a local _temp bool above
        // to FALSE as a temporary fix.
        if (!gbUserCancelledChooseTranslationDlg_temp || m_nWordsInPhrase == 1)
        {
            // try find a translation for the single word (from July 2003 this supports
            // auto capitalization) LookUpSrcWord() calls RecalcLayout()
            bGotTranslation = LookUpSrcWord(pNewPile);
        }
        else
        {
            // the user cancelled the ChooseTranslation dialog
            gbUserCancelledChooseTranslationDlg_temp = FALSE; // restore default value

            // if the user cancelled the Choose Translation dialog when a phrase was
            // merged, then he will probably want a lookup done for the first word of
            // the now unmerged phrase; nWordsInPhrase will still contain the word count
            // for the formerly merged phrase, so use it; but when glossing is current,
            // the LookUpSrcWord call is done only in the first map, so nWordsInPhrase
            // will not be greater than 1 when doing glossing
            if (m_nWordsInPhrase > 1) // nWordsInPhrase is a global, set in LookAhead()
                                    // or in LookUpSrcWord()
            {
                // nWordsInPhrase can only be > 1 in adapting mode, so handle that
                bGotTranslation = LookUpSrcWord(pNewPile);
            }
        }
		
        pNewPile = pApp->m_pActivePile; // update the pointer, since LookUpSrcWord()
                                        // calls RecalcLayout() & resets m_pActivePile (in refactored code
                                        // this call is still needed because we replace the old pile with the
                                        // altered one (it has new width since its now active location)
        if (bGotTranslation)
        {
            // if it is a <Not In KB> entry we show any m_targetStr that the
            // sourcephrase instance may have, by putting it in the global
            // translation variable; when glossing is ON, we ignore
            // "not in kb" since that pertains to adapting only
            if (!gbIsGlossing && m_Translation == _T("<Not In KB>"))
            {
                // make sure asterisk gets shown, and the adaptation is taken
                // from the sourcephrase itself - but it will be empty
                // if the sourcephrase has not been accessed before
                m_Translation = pNewPile->GetSrcPhrase()->m_targetStr;
                pNewPile->GetSrcPhrase()->m_bHasKBEntry = FALSE;
                pNewPile->GetSrcPhrase()->m_bNotInKB = TRUE;
            }

            pApp->m_targetPhrase = m_Translation; // set using the global var, set in
                                                // LookUpSrcWord() call
            bWantSelect = TRUE;
        }
        else // did not get a translation, or gloss
        {
            // do a copy of the source (this never needs change of capitalization)
            MakeCopyOrSetNothing(pApp, pView, pNewPile, bWantSelect);

            // BEW added 1Jul09, the flag should be TRUE if nothing was found
#if defined (ABANDON_NOT)
			pApp->m_pTargetBox->m_bAbandonable = FALSE;
#else
            pApp->m_pTargetBox->m_bAbandonable = TRUE;
#endif
        }
    } // BEW removed 16Mar18, whm restored 16Mar2018
 }

// BEW 13Apr10, changes needed for support of doc version 5
// whm modified 22Feb2018 to adjust for fact that the Cancel and Select button
// no longer exists in the Choose Translation dialog with the CPhraseBox being
// implemented with dropdown list. The m_bCancelAndSelect flag was removed 
// from this function's signature and the overall app. 
// TODO: This and the preceding function (for SingleStepMode) need their logic 
// changed to account for the removals of the legacy flags that were originally 
// called m_bCancelAndSelect and gbUserCancelledChooseTranslationDlg.
// As a temporary measure, I've made two local flags m_bCancelAndSelect_temp and
// gbUserCancelledChooseTranslationDlg_temp that are set to FALSE at the beginning 
// of the function. While the function should be revised at some point, setting the 
// flag to remain always FALSE, is just a temporary measure to ensure that I don't
// mess up the logic by a quick refactor that removes the effects of the original
// bool flags.
 // BEW 16Mar18 refactored to simplify - combobox implementation does not require so much logic
 //
 // whm 16Mar2018 restored BEW changes made 16Mar18 in attempt to find why app is now
 // failing to fill text within the phrasebox and have it pre-selected.

void CPhraseBox::HandleUnsuccessfulLookup_InAutoAdaptMode_AsBestWeCan(CAdapt_ItApp* pApp,
	CAdapt_ItView* pView, CPile* pNewPile, bool& bWantSelect)
{
    bool m_bCancelAndSelect_temp = FALSE; // whm 22Feb2018 added for temp fix
    bool gbUserCancelledChooseTranslationDlg_temp = FALSE; // whm 22Feb2018 added for temp fix

    pApp->m_bAutoInsert = FALSE; // cause halt

	// BEW 16Mar18, refactored a bit in response to Bill's request to check logic out in the combo dropdown implementation
	// With the combobox approach, Lookup() doesn't get a chance to put up Choose Translation dialog, instead, the user
	// eyeballs the dropped down (or not) list to decide what to do. Halting the box with m_bAutoInsert set FALSE should suffice,
	// so the test below and it's true block should be removed.

    if (!gbIsGlossing && m_bCancelAndSelect_temp)
    {
        // user cancelled CChooseTranslation dialog because he wants instead to
        // select for a merger of two or more source words
#if defined (ABANDON_NOT)
		pApp->m_pTargetBox->m_bAbandonable = FALSE;
#else
        pApp->m_pTargetBox->m_bAbandonable = TRUE;
#endif

        // no adaptation available, so depending on the m_bCopySource flag, either
        // initialize the targetPhrase to an empty string, or to a copy of the
        // sourcePhrase's key string; then select the first two words ready for a
        // merger or extension of the selection
        MakeCopyOrSetNothing(pApp, pView, pNewPile, bWantSelect);

        // the DoCancelAndSelect() call is below after the RecalcLayout calls
    }
    else // user does not want a "Cancel and Select" selection; or is glossing
    {

		// BEW 16Mar18, With the combodropdown implementation, it ought not be possible at this 
		// point to know what the user will want, I've set bWantSelect to FALSE able

        // try find a translation for the single source word, use it if we find one;
        // else do the usual copy of source word, with possible cc processing, etc.
        // LookUpSrcWord( ) has been ammended (July 2003) for auto capitalization
        // support; it does any needed case change before returning, leaving the
        // resulting string in the app member: m_Translation
        // whm 21Feb2018 note: the legacy gbUserCancelledChooseTranslationDlg is now 
        // removed from the app, so its logical value would be FALSE (never 
        // set to TRUE); whether a dialog was Cancelled is no longer a factor in the
        // logic of the following if ... else block. I've set a local _temp bool above
        // to FALSE as a temporary fix.
		// BEW 16Mar18 - the code below simplifies for the combodropdown box implementation
		// because glossing uses word counts greater than 1 now, and all we want here is
		// to get something for the box, if possible
        bool bGotTranslation = FALSE;
        if (!gbUserCancelledChooseTranslationDlg_temp || m_nWordsInPhrase == 1)
        {
            bGotTranslation = LookUpSrcWord(pNewPile);
        }
        else
        {
            gbUserCancelledChooseTranslationDlg_temp = FALSE; // restore default value

            // if the user cancelled the Choose Translation dialog when a phrase was
            // merged, then he will probably want a lookup done for the first word
            // of the now unmerged phrase; nWordsInPhrase will still contain the
            // word count for the formerly merged phrase, so use it; but when glossing
            // nWordsInPhrase should never be anything except 1, so this block should
            // not get entered when glossing
            if (m_nWordsInPhrase > 1) // nWordsInPhrase is set in LookAhead()
                                    // or in LookUpSrcWord()
            {
                // must be adapting mode
                bGotTranslation = LookUpSrcWord(pNewPile);
            }
        }
		//bGotTranslation = LookUpSrcWord(pNewPile); // BEW 16Mar18, whm removed 16Mar2018
        pNewPile = pApp->m_pActivePile; // update the pointer (needed, because
                // RecalcLayout() was done by LookUpSrcWord(), and in its refactored
                // code we called ResetPartnerPileWidth() to get width updated and
                // a new copy of the pile replacing the old one at same location
                // in the list m_pileList

        if (bGotTranslation)
        {
            // if it is a <Not In KB> entry we show any m_targetStr that the
            // sourcephrase instance may have, by putting it in the global
            // translation variable; when glossing is ON, we ignore
            // "not in kb" since that pertains to adapting only
            if (!gbIsGlossing && m_Translation == _T("<Not In KB>"))
            {
                // make sure asterisk gets shown, and the adaptation is taken
                // from the sourcephrase itself - but it will be empty
                // if the sourcephrase has not been accessed before
                m_Translation = pNewPile->GetSrcPhrase()->m_targetStr;
                pNewPile->GetSrcPhrase()->m_bHasKBEntry = FALSE;
                pNewPile->GetSrcPhrase()->m_bNotInKB = TRUE;
				//bWantSelect = TRUE; // whm removed BEW's addition 16Mar2018
            }

            pApp->m_targetPhrase = m_Translation; // set using the global var,
                                                // set in LookUpSrcWord call
            bWantSelect = TRUE;
        }
        else // did not get a translation, or a gloss when glossing is current
        {
#if defined(_DEBUG) & defined(LOOKUP_FEEDBACK)
            wxLogDebug(_T("HandleUnsuccessfulLookup_InAutoAdaptMode_AsBestWeCan() Before MakeCopy...: sn = %d , key = %s , m_targetPhrase = %s"),
                pNewPile->GetSrcPhrase()->m_nSequNumber, pNewPile->GetSrcPhrase()->m_key.c_str(), pApp->m_targetPhrase.c_str());
#endif
            MakeCopyOrSetNothing(pApp, pView, pNewPile, bWantSelect);

#if defined(_DEBUG) & defined(LOOKUP_FEEDBACK)
            wxLogDebug(_T("HandleUnsuccessfulLookup_InAutoAdaptMode_AsBestWeCan() After MakeCopy...: sn = %d , key = %s , m_targetPhrase = %s"),
                pNewPile->GetSrcPhrase()->m_nSequNumber, pNewPile->GetSrcPhrase()->m_key.c_str(), pApp->m_targetPhrase.c_str());
#endif

            // BEW added 1Jul09, the flag should be TRUE if nothing was found
#if defined (ABANDON_NOT)
			pApp->m_pTargetBox->m_bAbandonable = FALSE;
#else
            pApp->m_pTargetBox->m_bAbandonable = TRUE;
#endif
        }

        // is "Accept Defaults" turned on? If so, make processing continue
        if (pApp->m_bAcceptDefaults)
        {
            pApp->m_bAutoInsert = TRUE; // revoke the halt
        }
    } // BEW 16Mar18 removed true block, whm restored 16Mar2018
}

// returns TRUE if the move was successful, FALSE if not successful
// In refactored version, transliteration mode is handled by a separate function, so
// MoveToNextPile() is called only when CAdapt_ItApp::m_bTransliterationMode is FALSE, so
// this value can be assumed. The global boolean gbIsGlossing, however, may be either FALSE
// (adapting mode) or TRUE (glossing mode)
// Ammended July 2003 for auto-capitalization support
// BEW 13Apr10, changes needed for support of doc version 5
// BEW 21Jun10, no changes for support of kbVersion 2, & removed pView from signature
bool CPhraseBox::MoveToNextPile(CPile* pCurPile)
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
    // whm added 22Mar2018 for detecting callers of PlaceBox()
    pApp->m_bMovingToDifferentPile = TRUE;

	pApp->m_preGuesserStr.Empty(); // BEW 27Nov14, in case a src string, or modified string
		// is stored ready for user's Esc keypress to restore the pre-guesser
		// form, clear it, because the box is gunna move and we want it
		// restored to default empty ready for next box landing location
	// whm modified 29Mar12. Left mouse clicks now beep only when certain parts of
	// the canvas are clicked on, and allows other clicks to act normally (such as
	// the opening/closing of the ViewFilteredMaterial dialog and the Notes dialog).

	//bool bNoError = TRUE;
	bool bWantSelect = FALSE; // set TRUE if any initial text in the new location is to be
							  // shown selected
	// store the translation in the knowledge base
	//pApp->limiter = 0; // BEW added Aug13, to support OnIdle() hack for m_targetStr non-stick bug // bug fixed 24Sept13 BEW
	CAdapt_ItView* pView = pApp->GetView();
	CAdapt_ItDoc* pDoc = pApp->GetDocument();
	bool bOK;
	m_bBoxTextByCopyOnly = FALSE; // restore default setting
	CSourcePhrase* pOldActiveSrcPhrase = pCurPile->GetSrcPhrase();

#if defined(_DEBUG)
	{
		wxLogDebug(_T("%s::%s(), line %d, sn=%d, pSrcPhrase: m_key= [%s], m_srcPhrase= [%s], m_adaption= [%s], m_targetStr= [%s]"),
			__FILE__, __FUNCTION__, __LINE__, pOldActiveSrcPhrase->m_nSequNumber, pOldActiveSrcPhrase->m_key.c_str(),
			pOldActiveSrcPhrase->m_srcPhrase.c_str(), pOldActiveSrcPhrase->m_adaption.c_str(), pOldActiveSrcPhrase->m_targetStr.c_str());
	}
#endif

	CLayout* pLayout = GetLayout();

#if defined(_DEBUG) && defined(FLAGS)
	{
		CAdapt_ItApp* pApp = &wxGetApp();
		CSourcePhrase* pSrcPhrase = pOldActiveSrcPhrase;
		wxLogDebug(_T("%s::%s(), line %d, sn=%d, m_key= [%s], m_bAbandonable %d, m_bRetainBoxContents %d, m_bUserTypedSomething %d, m_bBoxTextByCopyOnly %d, m_bAutoInsert %d"),
			__FILE__, __FUNCTION__, __LINE__, pSrcPhrase->m_nSequNumber, pSrcPhrase->m_key.c_str(), (int)pApp->m_pTargetBox->m_bAbandonable, (int)pApp->m_pTargetBox->m_bRetainBoxContents,
			(int)pApp->m_bUserTypedSomething, (int)pApp->m_pTargetBox->m_bBoxTextByCopyOnly, (int)pApp->m_bAutoInsert);
	}
#endif

	// make sure pApp->m_targetPhrase doesn't have any final spaces
	RemoveFinalSpaces(pApp->m_pTargetBox,&pApp->m_targetPhrase);

	CPile* pNextEmptyPile = pView->GetNextEmptyPile(pCurPile);
	if (pNextEmptyPile == NULL)
	{
		// no more empty piles in the current document. We can just continue at this point
		// since we do this call again below
		;
	}
	else
	{
		// don't move forward if it means moving to an empty retranslation pile, but only for
		// when we are adapting. When glossing, the box is allowed to be within retranslations
		bool bNotInRetranslation =
				CheckPhraseBoxDoesNotLandWithinRetranslation(pView, pNextEmptyPile, pCurPile);
		if (!bNotInRetranslation)
		{
            // whm added 22Mar2018 for detecting callers of PlaceBox()
            pApp->m_bMovingToDifferentPile = FALSE;

            // the phrase box landed in a retranslation, so halt the lookup and insert loop
			// so the user can do something manually to achieve what he wants in the
			// viscinity of the retranslation
			return FALSE;
		}
		// continue processing below if the phrase box did not land in a retranslation
	}

	// if the location we are leaving is a <Not In KB> one, we want to skip the store & fourth
	// line creation --- as of Dec 18, version 1.4.0, according to Susanna Imrie's
	// recommendation, I've changed this so it will allow a non-null adaptation to remain at
	// this location in the document, but just to suppress the KB store; if glossing is ON, then
	// being a <Not In KB> location is irrelevant, and we will want the store done normally - but
	// to the glossing KB of course
	bOK = TRUE;
	if (!gbIsGlossing && !pOldActiveSrcPhrase->m_bHasKBEntry && pOldActiveSrcPhrase->m_bNotInKB)
	{
		// if the user edited out the <Not In KB> entry from the KB editor, we need to put
		// it back so that the setting is preserved (the "right" way to change the setting is to
		// use the toolbar checkbox - this applies when adapting, not glossing)
		pApp->m_pKB->Fix_NotInKB_WronglyEditedOut(pCurPile);
	}
	else
	{
		// make the punctuated target string, but only if adapting; note, for auto capitalization
		// ON, the function will change initial lower to upper as required, whatever punctuation
		// regime is in place for this particular sourcephrase instance
		// in the next call, the final bool flag, bIsTransliterateMode, is default FALSE
		
//* #if defined(_DEBUG)
	wxLogDebug(_T("MoveToNextPile() before DoStore_Normal...(): sn = %d , key = [%s] , m_targetPhrase = [%s] , m_targetStr = [%s]"),
		pCurPile->GetSrcPhrase()->m_nSequNumber, pCurPile->GetSrcPhrase()->m_key.c_str(), pApp->m_targetPhrase.c_str(), 
		pCurPile->GetSrcPhrase()->m_targetStr.c_str());
//#endif */

		bOK = DoStore_NormalOrTransliterateModes(pApp, pDoc, pView, pCurPile);
		if (!bOK)
		{
            // whm added 22Mar2018 for detecting callers of PlaceBox()
            pApp->m_bMovingToDifferentPile = FALSE;

            DealWithUnsuccessfulStore(pApp, pView, pNextEmptyPile);
			return FALSE; // can't move until a valid adaption (which could be null) is supplied
		}
	}
/* #if defined(_DEBUG)
	wxLogDebug(_T("MoveToNextPile() after DoStore_Normal...: sn = %d , key = %s , m_targetPhrase = %s , m_targetStr = %s"),
		pCurPile->GetSrcPhrase()->m_nSequNumber, pCurPile->GetSrcPhrase()->m_key.c_str(), pApp->m_targetPhrase.c_str(),
		pCurPile->GetSrcPhrase()->m_targetStr.c_str());
#endif */

	// since we are moving, make sure the default m_bSaveToKB value is set
	pApp->m_bSaveToKB = TRUE;

	// move to next pile's cell which has no adaptation yet
	pApp->m_bUserTypedSomething = FALSE; // user has not typed at the new location yet
	bool bAdaptationAvailable = FALSE;
	CPile* pNewPile = pView->GetNextEmptyPile(pCurPile); // this call does not update
														 // the active sequ number

	// if necessary restore default button image, and m_bCopySourcePunctuation to TRUE
	wxCommandEvent event;
	if (!pApp->m_bCopySourcePunctuation)
	{
		pView->OnToggleEnablePunctuationCopy(event);
	}
	if (pNewPile == NULL)
	{
		// we deem vertical editing current step to have ended if control gets into this
		// block, so user has to be asked what to do next if vertical editing is currently
		// in progress; and we tunnel out before m_nActiveSequNum can be set to -1 (otherwise
		// vertical edit will crash when recalc layout is tried with a bad sequ num value)
		if (gbVerticalEditInProgress)
		{
			m_bTunnellingOut = FALSE; // ensure default value set
			bool bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(-1,
							nextStep, TRUE); // bForceTransition is TRUE
			if (bCommandPosted)
			{
                // whm added 22Mar2018 for detecting callers of PlaceBox()
                pApp->m_bMovingToDifferentPile = FALSE;

                // don't proceed further because the current vertical edit step has ended
				m_bTunnellingOut = TRUE; // so caller can use it
				return FALSE;
			}
		}

		if (!pApp->m_bSingleStep)
		{
			pApp->m_bAutoInsert = FALSE; // cause halt, if auto lookup & inserting is ON
		}

		// ensure the view knows the pile pointer is no longer valid
		pApp->m_pActivePile = (CPile*)NULL;
		pApp->m_nActiveSequNum = -1;
		if (m_bSuppressStoreForAltBackspaceKeypress)
            m_SaveTargetPhrase.Empty();
		m_bSuppressStoreForAltBackspaceKeypress = FALSE; // make sure it's off before returning
		m_bTemporarilySuspendAltBKSP = FALSE;

        // whm added 22Mar2018 for detecting callers of PlaceBox()
        pApp->m_bMovingToDifferentPile = FALSE;

        return FALSE; // we are at the end of the document
	}
	else
	{
		// the pNewPile is valid, so proceed

        // don't commit to the new pile if we are in vertical edit mode, until we've
        // checked the pile is not in the gray text area...
        // if vertical editing is currently in progress we must check if the lookup target
        // is within the editable span, if not then control has moved the box into the gray
        // area beyond the editable span and that means a step transition is warranted &
        // the user should be asked what step is next.
		// BUT!! (BEW 7Feb18) If the pNewPile has no translation (ie. target) text yet,
		// we don't want OnIdle() to keep on calling OnePass(), as then the user gets 
		// no chance to type in an adaptation at the new location. So check, and clear
		//  m_bAutoInsert to FALSE so as to halt the forward advances of the phrasebox. 
		if (gbVerticalEditInProgress)
		{
			int m_nCurrentSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
			m_bTunnellingOut = FALSE; // ensure default value set
			bool bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(
									m_nCurrentSequNum,nextStep); // bForceTransition is FALSE
			if (bCommandPosted)
			{
                // whm added 22Mar2018 for detecting callers of PlaceBox()
                pApp->m_bMovingToDifferentPile = FALSE;

                // don't proceed further because the current vertical edit step has ended
				m_bTunnellingOut = TRUE; // so caller can use it
				return FALSE; // try returning FALSE
			}
			// BEW 19Oct15 No transition of vert edit modes,
			// so we can store this location on the app
			gpApp->m_vertEdit_LastActiveSequNum = m_nCurrentSequNum;
#if defined(_DEBUG)
			wxLogDebug(_T("VertEdit PhrBox, MoveToNextPile() storing loc'n: %d "), m_nCurrentSequNum);
#endif
		}

        // set active pile, and same var on the phrase box, and active sequ number - but
        // note that only the active sequence number will remain valid if a merge is
        // required; in the latter case, we will have to recalc the layout after the merge
        // and set the first two variables again
		pApp->m_pActivePile = pNewPile;
		pApp->m_nActiveSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
		m_nCurrentSequNum = pApp->m_nActiveSequNum; // global, for use by auto-saving

#if defined(_DEBUG) && defined(FLAGS)
		{
			CAdapt_ItApp* pApp = &wxGetApp();
			CSourcePhrase* pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
			wxLogDebug(_T("%s::%s(), line %d, sn=%d, m_key= [%s], m_bAbandonable %d, m_bRetainBoxContents %d, m_bUserTypedSomething %d, m_targetStr= [%s], m_bAutoInsert %d"),
				__FILE__, __FUNCTION__, __LINE__, pSrcPhrase->m_nSequNumber, pSrcPhrase->m_key.c_str(), (int)pApp->m_pTargetBox->m_bAbandonable, 
				(int)pApp->m_pTargetBox->m_bRetainBoxContents, (int)pApp->m_bUserTypedSomething, pSrcPhrase->m_targetStr.c_str(), (int)pApp->m_bAutoInsert);
		}
#endif
		// refactored design: we want the old pile's strip to be marked as invalid and the
		// strip index added to the CLayout::m_invalidStripArray
		pDoc->ResetPartnerPileWidth(pOldActiveSrcPhrase);

#ifdef SHOW_LOOK_AHEAD_BENCHMARKS
        wxDateTime dt1 = wxDateTime::Now(),
            dt2 = wxDateTime::UNow();
#endif
        // look ahead for a match with KB phrase content at this new active location
        // LookAhead (July 2003) has been ammended for auto-capitalization support; and
        // since it does a KB lookup, it will set gbMatchedKB_UCentry TRUE or FALSE; and if
        // an entry is found, any needed case change will have been done prior to it
        // returning (the result is in the global variable: translation)
		bAdaptationAvailable = LookAhead(pNewPile);

#ifdef SHOW_LOOK_AHEAD_BENCHMARKS
        dt1 = dt2;
        dt2 = wxDateTime::UNow();
        wxLogDebug(_T("In MoveToNextPile() LookAhead() executed in %s ms"),
            (dt2 - dt1).Format(_T("%l")).c_str());
#endif

		pView->RemoveSelection();

		// check if we found a match and have an available adaptation string ready
		if (bAdaptationAvailable)
		{
			pApp->m_pTargetBox->m_bAbandonable = FALSE;

            // adaptation is available, so use it if the source phrase has only a single
            // word, but if it's multi-worded, we must first do a merge and recalc of the
            // layout
            if (!gbIsGlossing && !m_bSuppressMergeInMoveToNextPile)
			{
                // this merge is done here only if an auto-insert can be done
				if (m_nWordsInPhrase > 1) // m_nWordsInPhrase is a global, set in LookAhead()
										// or in LookUpSrcWord()
				{
					// do the needed merge, etc.
					pApp->bLookAheadMerge = TRUE; // set static flag to ON
					bool bSaveFlag = m_bAbandonable; // the box is "this"
					pView->MergeWords();
					m_bAbandonable = bSaveFlag; // preserved the flag across the merge
					pApp->bLookAheadMerge = FALSE; // restore static flag to OFF
				}
			}

			// BEW changed 9Apr12 to support discontinuous highlighting spans
            if (m_nWordsInPhrase == 1 || !m_bSuppressMergeInMoveToNextPile)
			{
                // When nWordsInPhrase > 1, since the call of LookAhead() didn't require
                // user choice of the adaptation or gloss for the merger, it wasn't done in
                // the latter function, and so is done here automatically (because there is
                // a unique adaptation available) and so it is appropriate to make this
                // location have background highlighting, since the adaptation is now to be
                // auto-inserted after the merger was done above. Note: the
                // m_bSuppressMergeInMoveToNextPile flag will be FALSE if the merger was not
                // done in the prior LookAhead() call (with ChooseTranslation() being
                // required for the user to manually choose which adaptation is wanted); we
                // use that fact in the test above.
                // When m_nWordsInPhrase is 1, there is no merger involved and the
				// auto-insert to be done now requires background highlighting (Note, we
				// set the flag when appropriate, but only suppress doing the background
				// colour change in the CCell's Draw() function, if the user has requested
				// that no background highlighting be used - that uses the
				// m_bSuppressTargetHighlighting flag with TRUE value to accomplish the
				// suppression)
                pLayout->SetAutoInsertionHighlightFlag(pNewPile);
			}
            // assign the translation text - but check it's not "<Not In KB>", if it is,
            // phrase box can have m_targetStr, turn OFF the m_bSaveToKB flag, DON'T halt
            // auto-inserting if it is on, (in the very earliest versions I made it halt)
            // -- for version 1.4.0 and onwards, this does not change because when auto
            // inserting, we must have a default translation for a 'not in kb' one - and
            // the only acceptable default is a null string. The above applies when
            // gbIsGlossing is OFF
			wxString str = m_Translation; // m_Translation set within LookAhead()

			if (!gbIsGlossing && (m_Translation == _T("<Not In KB>")))
			{
				pApp->m_bSaveToKB = FALSE;
                m_Translation = pNewPile->GetSrcPhrase()->m_targetStr; // probably empty
				pApp->m_targetPhrase = m_Translation;
				bWantSelect = FALSE;
#if defined (ABANDON_NOT)
				pApp->m_pTargetBox->m_bAbandonable = FALSE;
#else
				pApp->m_pTargetBox->m_bAbandonable = TRUE;
#endif
				pNewPile->GetSrcPhrase()->m_bHasKBEntry = FALSE;
				pNewPile->GetSrcPhrase()->m_bNotInKB = TRUE; // ensures * shows above
															 // this srcPhrase
			}
			else
			{
				pApp->m_targetPhrase = m_Translation;
				bWantSelect = FALSE;
			}
//#ifdef Highlighting_Bug
//			// BEW changed 9Apr12 for support of discontinuous highlighting spans
//			wxLogDebug(_T("PhraseBox::MoveToNextPile(), hilighting: sequnum = %d  where the user chose:  %s  for source:  %s"),
//				m_nCurrentSequNum, translation, pNewPile->GetSrcPhrase()->m_srcPhrase);
//#endif
            // treat auto insertion as if user typed it, so that if there is a
            // user-generated extension done later, the inserted translation will not be
            // removed and copied source text used instead; since user probably is going to
            // just make a minor modification
			pApp->m_bUserTypedSomething = TRUE;

            // get a widened pile pointer for the new active location, and we want the
            // pile's strip to be marked as invalid and the strip index added to the
            // CLayout::m_invalidStripArray
			if (pNewPile != NULL)
			{
				pDoc->ResetPartnerPileWidth(pNewPile->GetSrcPhrase());
			}
		}
		else // the lookup determined that no adaptation (or gloss when glossing), or
			 // <Not In KB> entry, is available
		{
			pNewPile = pApp->m_pActivePile;

			// clear all storage of the earlier location's target text
			m_Translation.Empty();
			pApp->m_targetPhrase.Empty();
			// initialize the phrase box too, so it doesn't carry the old string
			// to the next pile's cell
			this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase);

            // RecalcLayout call when there is no adaptation available from the LookAhead,
            // (or user cancelled when shown the Choose Translation dialog from within the
            // LookAhead() function, having matched) we must cause auto lookup and
            // inserting to be turned off, so that the user can do a manual adaptation; but
            // if the m_bAcceptDefaults flag is on, then the copied source (having been
            // through c.changes) is accepted without user input, the m_bAutoInsert flag is
            // turned back on, so processing will continue
            pApp->m_nActiveSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;

            if (!pApp->m_bSingleStep)
            {
                // This call internally sets m_bAutoInsert to FALSE at its first line, but
                // if in cc mode and m_bAcceptDefaults is true, then cc keeps the box moving
                // forward by resetting m_bAutoInsert to TRUE before it returns
                HandleUnsuccessfulLookup_InAutoAdaptMode_AsBestWeCan(
                    pApp, pView, pNewPile, bWantSelect);
            }
            else // it's single step mode
            {
                HandleUnsuccessfulLookup_InSingleStepMode_AsBestWeCan(
                    pApp, pView, pNewPile, bWantSelect);
            }

            // get a widened pile pointer for the new active location, and we want the
            // pile's strip to be marked as invalid and the strip index added to the
            // CLayout::m_invalidStripArray
			if (pNewPile != NULL)
			{
				pDoc->ResetPartnerPileWidth(pNewPile->GetSrcPhrase());
			}
		}

        // initialize the phrase box too, so it doesn't carry the old string to the next
        // pile's cell
        this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase); //SetWindowText(pApp->m_targetPhrase);
#if defined (_DEBUG)
		wxLogDebug(_T("MoveToNextPile: line %d , m_targetPhrase= [%s]"), __LINE__, pApp->m_targetPhrase.c_str());
#endif
        // if we merged and moved, we have to update pNewPile, because we have done a
		// RecalcLayout in the LookAhead() function; it's possible to return from
		// LookAhead() without having done a recalc of the layout, so the else block
		// should cover that situation
		if (m_bCompletedMergeAndMove)
		{
			pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
		}
		else
		{
			// whm 15Dec2022 Note. In certain circumstances (especially when the phrasebox is moving
			// from a location near the right end of a strip, but is followied by one or more piles on
			// the same strip), the alignment of piles following the phrasebox gets bunched up and they
			// overlap the source phrase once the phrasebox has moved. I think this is caused by the 
			// RecalcLayout() call below being called with keep_strips_keep_piles, rather than 
			// create_strips_keep_piles to cause the piles following the phrasebox to flow to the next
			// strip. My testing, however, reveals other alignment issues if I try using the 
			// create_strips_keep_piles argument here, so I've opted to leave the RecalcLayout()
			// call as it is, and instead add a SendSizeEvent() after the Invalidate and PlaceBox()
			// calls below. That straightens out the layout. 
#ifdef _NEW_LAYOUT
			pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
		    pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
		    // get the new active pile
		    pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
		    wxASSERT(pApp->m_pActivePile != NULL);
		}

        // if the user has turned on the sending of synchronized scrolling messages send
        // the relevant message once auto-inserting halts, because we don't want to make
        // other applications sync scroll during auto-insertions, as it could happen very
        // often and the user can't make any visual use of what would be happening anyway;
        // even if a Cancel and Select is about to be done, a sync scroll is appropriate
        // now, provided auto-inserting has halted
		if (!pApp->m_bIgnoreScriptureReference_Send && !pApp->m_bAutoInsert)
		{
			pView->SendScriptureReferenceFocusMessage(pApp->m_pSourcePhrases,
													pApp->m_pActivePile->GetSrcPhrase());
		}

		// update status bar with project name
		pApp->RefreshStatusBarInfo();

		// recreate the phraseBox using the stored information
		if (bWantSelect)
		{
			pApp->m_nStartChar = -1; // wx uses -1, not 0 as in MFC
			pApp->m_nEndChar = -1;
		}
		else
		{
			int len = GetLineLength(0); // 0 = first line, only line
			pApp->m_nStartChar = len;
			pApp->m_nEndChar = len;
		}

        // fix the m_bSaveToKB flag, depending on whether or not srcPhrase is in kb; but
        // this applies only when adapting, not glossing
		if (!gbIsGlossing && !pApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry &&
			pApp->m_pActivePile->GetSrcPhrase()->m_bNotInKB)
		{
			pApp->m_bSaveToKB = FALSE;
			pApp->m_targetPhrase.Empty();
		}
		else
		{
			pApp->m_bSaveToKB = TRUE;
		}

		m_bCompletedMergeAndMove = FALSE; // make sure it's cleared

        // BEW note 24Mar09: later we may use clipping (the comment below may not apply in
        // the new design anyway)
		pView->Invalidate(); // do the whole client area, because if target font is larger
            // than the source font then changes along the line throw words off screen and
            // they get missed and eventually app crashes because active pile pointer will
            // get set to NULL
#if defined(_DEBUG) && defined(FLAGS)
		{
			CAdapt_ItApp* pApp = &wxGetApp();
			CSourcePhrase* pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
			wxLogDebug(_T("\n%s::%s(), line %d, sn=%d, m_key= [%s], m_bAbandonable %d, m_bRetainBoxContents %d, m_bUserTypedSomething %d, m_targetStr= [%s], m_bAutoInsert %d"),
				__FILE__, __FUNCTION__, __LINE__, pSrcPhrase->m_nSequNumber, pSrcPhrase->m_key.c_str(), (int)pApp->m_pTargetBox->m_bAbandonable, (int)pApp->m_pTargetBox->m_bRetainBoxContents,
				(int)pApp->m_bUserTypedSomething, pSrcPhrase->m_targetStr.c_str(), (int)pApp->m_bAutoInsert);
		}
#endif


		pLayout->PlaceBox();

#if defined(_DEBUG) && defined(FLAGS)
		{
			CAdapt_ItApp* pApp = &wxGetApp();
			CSourcePhrase* pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
			wxLogDebug(_T("\n%s::%s(), line %d, sn=%d, m_key= [%s], m_bAbandonable %d, m_bRetainBoxContents %d, m_bUserTypedSomething %d, m_targetStr= [%s], m_bAutoInsert %d"),
				__FILE__, __FUNCTION__, __LINE__, pSrcPhrase->m_nSequNumber, pSrcPhrase->m_key.c_str(), (int)pApp->m_pTargetBox->m_bAbandonable, (int)pApp->m_pTargetBox->m_bRetainBoxContents,
				(int)pApp->m_bUserTypedSomething, pSrcPhrase->m_targetStr.c_str(), (int)pApp->m_bAutoInsert);
		}
#endif
		
		// whm 15Dec2022 added. In certain circumstances (especially when the phrasebox is moving
		// from a location near the right end of a strip, but is followied by one or more piles on
		// the same strip), the alignment of piles following the phrasebox gets bunched up and they
		// overlap the source phrase once the phrasebox has moved. This bunching up is straightened
		// out by doing a simple vertical frame window resize (triggering CMainFrame's OnSize())
		// suggesting that the RecalcLayout() has done its job, but the layout wasn't refreshed
		// properly afterwards. While this could be caused by the RecalcLayout() call above being 
		// called with keep_strips_keep_piles, rather than create_strips_keep_piles to cause the 
		// piles following the phrasebox to flow to the next strip. My testing, however, reveals 
		// other alignment issues if I try using the create_strips_keep_piles argument there, so 
		// I've opted to leave the RecalcLayout() call as it is, and instead add a SendSizeEvent() 
		// here after the Invalidate and PlaceBox() calls above. That straightens out the layout. 
		// Note that the OnSize() handler does call RecalcLayout(... create_strips_keep_piles)
		// followed by an Invalidate() and PlaceBox(noDropDownInitialization) call.
		pApp->GetMainFrame()->SendSizeEvent();

		// BEW 13Apr20, control goes thru here when TAB or Enter gets a move to next empty
		// pile done - and PlacePhraseBox() does not get called (nor Jump()), so we have
		// to check here for landing within a footnote, extended footnote, cross-marker or
		// extended cross-marker span - if so, the two placeholder insert buttons must
		// be disabled by the Update...() handler for those
		pApp->m_bDisablePlaceholderInsertionButtons = FALSE; // initialise to enabled buttons
		if (pApp->m_pActivePile != NULL)
		{
			CSourcePhrase* pSPhr = pApp->m_pActivePile->GetSrcPhrase();
			bool bProhibited = pDoc->IsWithinSpanProhibitingPlaceholderInsertion(pSPhr);
			pApp->m_bDisablePlaceholderInsertionButtons = bProhibited;
		}

		if (bWantSelect)
			SetModify(TRUE); // our own SetModify(); calls MarkDirty()
		else
			SetModify(FALSE); // our own SetModify(); calls DiscardEdits()

        // make sure gbSuppressMergeInMoveToNextPile is reset to the default value
        m_bSuppressMergeInMoveToNextPile = FALSE;

        // whm added 22Mar2018 for detecting callers of PlaceBox()
        pApp->m_bMovingToDifferentPile = FALSE;
#if defined(_DEBUG) && defined(FLAGS)
		{
			CAdapt_ItApp* pApp = &wxGetApp();
			CSourcePhrase* pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
			wxLogDebug(_T("\n%s::%s(), line %d, sn=%d, m_key= [%s], m_bAbandonable %d, m_bRetainBoxContents %d, m_targetStr= [%s], m_bAutoInsert %d"),
				__FILE__, __FUNCTION__, __LINE__, pSrcPhrase->m_nSequNumber, pSrcPhrase->m_key.c_str(), (int)pApp->m_pTargetBox->m_bAbandonable, (int)pApp->m_pTargetBox->m_bRetainBoxContents,
				pSrcPhrase->m_targetStr.c_str(), (int)pApp->m_bAutoInsert);
		}
#endif
        return TRUE;
	}
}

// returns TRUE if all was well, FALSE if something prevented the move
// Note: this is based on MoveToNextPile(), but with added code for transliterating - and
// recall that when transliterating, the extra code may be used, or the normal KB lookup
// code may be used, depending on the user's assessment of whether the transliterating
// converter did its job correctly, or not correctly, respectively. When control is in this
// function CAdapt_ItApp::m_bTransliterationMode will be TRUE, and can therefore be
// assumed; likewise the global boolean gbIsGlossing will be FALSE (because
// transliteration mode is not available when glossing mode is turned ON), and so that too
// can be assumed
// BEW 13Apr10, changes needed for support of doc version 5
// BEW 21Jun10, no changes needed for support of kbVersion 2, & removed pView from signature
bool CPhraseBox::MoveToNextPile_InTransliterationMode(CPile* pCurPile)
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	pApp->m_preGuesserStr.Empty(); // BEW 27Nov14, in case a src string, or modified string
		// is stored ready for user's Esc keypress to restore the pre-guesser
		// form, clear it, because the box is gunna move and we want it
		// restored to default empty ready for next box landing location
	// whm modified 29Mar12. Left mouse clicks now beep only when certain parts of
	// the canvas are clicked on, and allows other clicks to act normally (such as
	// the opening/closing of the ViewFilteredMaterial dialog and the Notes dialog).
	//bool bNoError = TRUE;
	bool bWantSelect = FALSE; // set TRUE if any initial text in the new location is to be
							  // shown selected
	// store the translation in the knowledge base
	//pApp->limiter = 0; // BEW added Aug13, to support OnIdle() hack for m_targetStr non-stick bug // bug fixed 24Sept13 BEW
	CAdapt_ItView* pView = pApp->GetView();
	CAdapt_ItDoc* pDoc = pApp->GetDocument();
	bool bOK;
	m_bBoxTextByCopyOnly = FALSE; // restore default setting
	CSourcePhrase* pOldActiveSrcPhrase = pCurPile->GetSrcPhrase();
	CLayout* pLayout = GetLayout();

	// make sure pApp->m_targetPhrase doesn't have any final spaces
	RemoveFinalSpaces(pApp->m_pTargetBox,&pApp->m_targetPhrase);

	CPile* pNextEmptyPile = pView->GetNextEmptyPile(pCurPile);
	if (pNextEmptyPile == NULL)
	{
		// no more empty piles in the current document. We can just continue at this point
		// since we do this call again below
		;
	}
	else
	{
		// don't move forward if it means moving to an empty retranslation pile, but only for
		// when we are adapting. When glossing, the box is allowed to be within retranslations
		bool bNotInRetranslation = CheckPhraseBoxDoesNotLandWithinRetranslation(pView,
															pNextEmptyPile, pCurPile);
		if (!bNotInRetranslation)
		{
			// the phrase box landed in a retranslation, so halt the lookup and insert loop
			// so the user can do something manually to achieve what he wants in the
			// viscinity of the retranslation
			return FALSE;
		}
		// continue processing below if the phrase box did not land in a retranslation
	}

	// if the location we are leaving is a <Not In KB> one, we want to skip the store & fourth
	// line creation --- as of Dec 18, version 1.4.0, according to Susanna Imrie's
	// recommendation, I've changed this so it will allow a non-null adaptation to remain at
	// this location in the document, but just to suppress the KB store; if glossing is ON, then
	// being a <Not In KB> location is irrelevant, and we will want the store done normally - but
	// to the glossing KB of course
	// BEW addition 21Apr06 to support transliterating better (showing transiterations)
	if (m_bSuppressStoreForAltBackspaceKeypress)
	{
		pApp->m_targetPhrase = m_SaveTargetPhrase; // set it up in advance, from last LookAhead() call
		goto c;
	}
	if (!gbIsGlossing && !pOldActiveSrcPhrase->m_bHasKBEntry && pOldActiveSrcPhrase->m_bNotInKB)
	{
		// if the user edited out the <Not In KB> entry from the KB editor, we need to put
		// it back so that the setting is preserved (the "right" way to change the setting is to
		// use the toolbar checkbox - this applies when adapting, not glossing)
		pApp->m_pKB->Fix_NotInKB_WronglyEditedOut(pCurPile);
		goto b;
	}

	// make the punctuated target string, but only if adapting; note, for auto capitalization
	// ON, the function will change initial lower to upper as required, whatever punctuation
	// regime is in place for this particular sourcephrase instance

    // BEW added 19Apr06 for support of Alt + Backspace keypress suppressing store on the
    // phrase box move which is a feature for power users requested by Bob Eaton; the code
    // in the first block is for this new (undocumented) feature - power uses will have
    // knowledge of it from an article to be placed in Word&Deed by Bob. Only needed for
    // adapting mode, so the glossing mode case is commented out
c:	bOK = TRUE;
	if (m_bSuppressStoreForAltBackspaceKeypress)
	{
		// when we don't want to store in the KB, we still have some things to do
		// to get appropriate m_adaption and m_targetStr members set up for the doc...
		// when adapting, fill out the m_targetStr member of the CSourcePhrase instance,
		// and do any needed case conversion and get punctuation in place if required
		pView->MakeTargetStringIncludingPunctuation(pOldActiveSrcPhrase, pApp->m_targetPhrase);

		// the m_targetStr member may now have punctuation, so get rid of it
		// before assigning whatever is left to the m_adaption member
		wxString strKeyOnly = pOldActiveSrcPhrase->m_targetStr;
		pView->RemovePunctuation(pDoc,&strKeyOnly,from_target_text);

		// set the m_adaption member too
		pOldActiveSrcPhrase->m_adaption = strKeyOnly;

		// let the user see the unpunctuated string in the phrase box as visual feedback
		pApp->m_targetPhrase = strKeyOnly;

        // now do a store, but only of <Not In KB>, (StoreText uses
        // m_bSuppressStoreForAltBackspaceKeypress == TRUE to get this job done rather than
        // a normal store) & sets flags appropriately (Note, while we pass in
        // pApp->m_targetPhrase, the phrase box contents string, we StoreText doesn't use
        // it when m_bSuppressStoreForAltBackspaceKeypress is TRUE, but internally sets a
        // local string to "<Not In KB>" and stores that instead) BEW 27Jan09, nothing more
        // needed here
        if (gbIsGlossing)
		{
			bOK = pApp->m_pGlossingKB->StoreText(pOldActiveSrcPhrase, pApp->m_targetPhrase);
		}
		else
		{
			pApp->m_bInhibitMakeTargetStringCall = TRUE; // MakeTargetStringIncludingPunctuation is
					// called above, followed by RemovePunctuation, so we don't need a further 
					// call of the former within StoreText(), so inhibit it
			bOK = pApp->m_pKB->StoreText(pOldActiveSrcPhrase, pApp->m_targetPhrase);
			pApp->m_bInhibitMakeTargetStringCall = FALSE;
		}
	}
	else
	{
		bOK = DoStore_NormalOrTransliterateModes(pApp, pDoc, pView, pCurPile,
												pApp->m_bTransliterationMode);
	}
	if (!bOK)
	{
		DealWithUnsuccessfulStore(pApp, pView, pNextEmptyPile);
		return FALSE; // can't move until a valid adaption (which could be null) is supplied
	}

	// since we are moving, make sure the default m_bSaveToKB value is set
b:	pApp->m_bSaveToKB = TRUE;

	// move to next pile's cell which has no adaptation yet
	pApp->m_bUserTypedSomething = FALSE; // user has not typed at the new location yet
	bool bAdaptationAvailable = FALSE;
	CPile* pNewPile = pView->GetNextEmptyPile(pCurPile); // this call does not update
														 // the active sequ number
	// if necessary restore default button image, and m_bCopySourcePunctuation to TRUE
	wxCommandEvent event;
	if (!pApp->m_bCopySourcePunctuation)
	{
		pApp->GetView()->OnToggleEnablePunctuationCopy(event);
	}
	if (pNewPile == NULL)
	{
		// we deem vertical editing current step to have ended if control gets into this
		// block, so user has to be asked what to do next if vertical editing is currently
		// in progress; and we tunnel out before m_nActiveSequNum can be set to -1 (otherwise
		// vertical edit will crash when recalc layout is tried with a bad sequ num value)
		if (gbVerticalEditInProgress)
		{
			m_bTunnellingOut = FALSE; // ensure default value set
			bool bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(-1,
							nextStep, TRUE); // bForceTransition is TRUE
			if (bCommandPosted)
			{
				// don't proceed further because the current vertical edit step has ended
				m_bTunnellingOut = TRUE; // so caller can use it
				return FALSE;
			}
			else
			{
				// BEW 19Oct15 No transition of vert edit modes,
				// so we can store this location on the app
				gpApp->m_vertEdit_LastActiveSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
#if defined(_DEBUG)
				wxLogDebug(_T("VertEdit PhrBox, MoveToNextPile_InTransliterationMode() storing loc'n: %d "), 
					pNewPile->GetSrcPhrase()->m_nSequNumber);
#endif
			}
		}

		if (!pApp->m_bSingleStep)
		{
			pApp->m_bAutoInsert = FALSE; // cause halt, if auto lookup & inserting is ON
		}

		// ensure the view knows the pile pointer is no longer valid
		pApp->m_pActivePile = (CPile*)NULL;
		pApp->m_nActiveSequNum = -1;
		if (m_bSuppressStoreForAltBackspaceKeypress)
            m_SaveTargetPhrase.Empty();
		m_bSuppressStoreForAltBackspaceKeypress = FALSE; // make sure it's off before returning
		m_bTemporarilySuspendAltBKSP = FALSE;

		return FALSE; // we are at the end of the document
	}
	else
	{
		// the pNewPile is valid, so proceed

        // don't commit to the new pile if we are in vertical edit mode, until we've
        // checked the pile is not in the gray text area...
        // if vertical editing is currently in progress we must check if the lookup target
        // is within the editable span, if not then control has moved the box into the gray
        // area beyond the editable span and that means a step transition is warranted &
        // the user should be asked what step is next
		if (gbVerticalEditInProgress)
		{
			int m_nCurrentSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
			m_bTunnellingOut = FALSE; // ensure default value set
			bool bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(
									m_nCurrentSequNum,nextStep); // bForceTransition is FALSE
			if (bCommandPosted)
			{
				// don't proceed further because the current vertical edit step has ended
				m_bTunnellingOut = TRUE; // so caller can use it
				return FALSE; // try returning FALSE
			}
			else
			{
				// BEW 19Oct15 No transition of vert edit modes,
				// so we can store this location on the app
				gpApp->m_vertEdit_LastActiveSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
#if defined(_DEBUG)
				wxLogDebug(_T("VertEdit PhrBox, MoveToNextPile_InTransliterationMode() storing loc'n: %d "), 
					pNewPile->GetSrcPhrase()->m_nSequNumber);
#endif
			}
		}

        // set active pile, and same var on the phrase box, and active sequ number - but
        // note that only the active sequence number will remain valid if a merge is
        // required; in the latter case, we will have to recalc the layout after the merge
        // and set the first two variables again
		pApp->m_pActivePile = pNewPile;
		pApp->m_nActiveSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
		m_nCurrentSequNum = pApp->m_nActiveSequNum; // global, for use by auto-saving

        // adjust width pf the pile pointer for the new active location, and we want the
        // pile's strip to be marked as invalid and the strip index added to the
        // CLayout::m_invalidStripArray
		pDoc->ResetPartnerPileWidth(pOldActiveSrcPhrase);

#ifdef SHOW_LOOK_AHEAD_BENCHMARKS
        wxDateTime dt1 = wxDateTime::Now(),
            dt2 = wxDateTime::UNow();
#endif
        // look ahead for a match with KB phrase content at this new active location
        // LookAhead (July 2003) has been ammended for auto-capitalization support; and
        // since it does a KB lookup, it will set gbMatchedKB_UCentry TRUE or FALSE; and if
        // an entry is found, any needed case change will have been done prior to it
        // returning (the result is in the global variable: translation)
		bAdaptationAvailable = LookAhead(pNewPile);

#ifdef SHOW_LOOK_AHEAD_BENCHMARKS
        dt1 = dt2;
        dt2 = wxDateTime::UNow();
        wxLogDebug(_T("In MoveToNextPile_InTransliterationMode() LookAhead() executed in %s ms"),
            (dt2 - dt1).Format(_T("%l")).c_str());
#endif

		pView->RemoveSelection();

		// check if we found a match and have an available adaptation string ready
		if (bAdaptationAvailable)
		{
			pApp->m_pTargetBox->m_bAbandonable = FALSE;

             // adaptation is available, so use it if the source phrase has only a single
            // word, but if it's multi-worded, we must first do a merge and recalc of the
            // layout
			if (!gbIsGlossing && !m_bSuppressMergeInMoveToNextPile)
			{
                // this merge is done here only if an auto-insert can be done
				if (m_nWordsInPhrase > 1) // m_nWordsInPhrase is a global, set in LookAhead()
										// or in LookUpSrcWord()
				{
					// do the needed merge, etc.
					pApp->bLookAheadMerge = TRUE; // set static flag to ON
					bool bSaveFlag = m_bAbandonable; // the box is "this"
					pView->MergeWords();
					m_bAbandonable = bSaveFlag; // preserved the flag across the merge
					pApp->bLookAheadMerge = FALSE; // restore static flag to OFF
				}
			}

			// BEW changed 9Apr12 to support discontinuous highlighting spans
			if (m_nWordsInPhrase == 1 || !m_bSuppressMergeInMoveToNextPile)
			{
                // When nWordsInPhrase > 1, since the call of LookAhead() didn't require
                // user choice of the adaptation or gloss for the merger, it wasn't done in
                // the latter function, and so is done here automatically (because there is
                // a unique adaptation available) and so it is appropriate to make this
                // location have background highlighting, since the adaptation is now to be
                // auto-inserted after the merger was done above. Note: the
                // m_bSuppressMergeInMoveToNextPile flag will be FALSE if the merger was not
                // done in the prior LookAhead() call (with ChooseTranslation() being
                // required for the user to manually choose which adaptation is wanted); we
                // use that fact in the test above.
                // When m_nWordsInPhrase is 1, there is no merger involved and the
				// auto-insert to be done now requires background highlighting (Note, we
				// set the flag when appropriate, but only suppress doing the background
				// colour change in the CCell's Draw() function, if the user has requested
				// that no background highlighting be used - that uses the
				// m_bSuppressTargetHighlighting flag with TRUE value to accomplish the
				// suppression)
                pLayout->SetAutoInsertionHighlightFlag(pNewPile);
			}
            // assign the translation text - but check it's not "<Not In KB>", if it is,
            // phrase box can have m_targetStr, turn OFF the m_bSaveToKB flag, DON'T halt
            // auto-inserting if it is on, (in the very earliest versions I made it halt)
            // -- for version 1.4.0 and onwards, this does not change because when auto
            // inserting, we must have a default translation for a 'not in kb' one - and
            // the only acceptable default is a null string. The above applies when
            // gbIsGlossing is OFF
			wxString str = m_Translation; // m_Translation set within LookAhead()

            // BEW added 21Apr06, so that when transliterating the lookup puts a fresh
            // transliteration of the source when it finds a <Not In KB> entry, since the
            // latter signals that the SIL Converters conversion yields a correct result
            // for this source text, so we want the user to get the feedback of seeing it,
            // but still just have <Not In KB> in the KB entry
			if (!pApp->m_bSingleStep && (m_Translation == _T("<Not In KB>"))
											&& m_bTemporarilySuspendAltBKSP)
			{
				m_bSuppressStoreForAltBackspaceKeypress = TRUE;
				m_bTemporarilySuspendAltBKSP = FALSE;
			}

			if (m_bSuppressStoreForAltBackspaceKeypress && (m_Translation == _T("<Not In KB>")))
			{
				pApp->m_bSaveToKB = FALSE;
 				CSourcePhrase* pSrcPhr = pNewPile->GetSrcPhrase();
				wxString str = pView->CopySourceKey(pSrcPhr, pApp->m_bUseConsistentChanges);
				bWantSelect = FALSE;
#if defined (ABANDON_NOT)
				pApp->m_pTargetBox->m_bAbandonable = FALSE;
#else
				pApp->m_pTargetBox->m_bAbandonable = TRUE;
#endif
				pSrcPhr->m_bHasKBEntry = FALSE;
				pSrcPhr->m_bNotInKB = TRUE; // ensures * shows above
				pSrcPhr->m_adaption = str;
				pSrcPhr->m_targetStr = pSrcPhr->m_precPunct + str;
				pSrcPhr->m_targetStr += pSrcPhr->m_follPunct;
                m_Translation = pSrcPhr->m_targetStr;
				pApp->m_targetPhrase = m_Translation;
                m_SaveTargetPhrase = m_Translation; // to make it available on
												 // next auto call of OnePass()
			}
			// continue with the legacy code
			else if (m_Translation == _T("<Not In KB>"))
			{
				pApp->m_bSaveToKB = FALSE;
                m_Translation = pNewPile->GetSrcPhrase()->m_targetStr; // probably empty
				pApp->m_targetPhrase = m_Translation;
				bWantSelect = FALSE;
#if defined (ABANDON_NOT)
				pApp->m_pTargetBox->m_bAbandonable = FALSE;
#else
				pApp->m_pTargetBox->m_bAbandonable = TRUE;
#endif
				pNewPile->GetSrcPhrase()->m_bHasKBEntry = FALSE;
				pNewPile->GetSrcPhrase()->m_bNotInKB = TRUE; // ensures * shows above this srcPhrase
			}
			else
			{
				pApp->m_targetPhrase = m_Translation;
				bWantSelect = FALSE;

				if (m_bSuppressStoreForAltBackspaceKeypress)
				{
                    // was the normal entry found while the
                    // m_bSuppressStoreForAltBackspaceKeypress flag was TRUE? Then we have
                    // to turn the flag off for a while, but turn it on programmatically
                    // later if we are still in Automatic mode and we come to another <Not
                    // In KB> entry. We can do this with another bool defined for this
                    // purpose
					m_bTemporarilySuspendAltBKSP = TRUE;
					m_bSuppressStoreForAltBackspaceKeypress = FALSE;
				}
			}

            // treat auto insertion as if user typed it, so that if there is a
            // user-generated extension done later, the inserted translation will not be
            // removed and copied source text used instead; since user probably is going to
            // just make a minor modification
			pApp->m_bUserTypedSomething = TRUE;

            // get a widened pile pointer for the new active location, and we want the
            // pile's strip to be marked as invalid and the strip index added to the
            // CLayout::m_invalidStripArray
			if (pNewPile != NULL)
			{
				pDoc->ResetPartnerPileWidth(pNewPile->GetSrcPhrase());
			}
		}
		else // the lookup determined that no adaptation (or gloss when glossing), or
			 // <Not In KB> entry, is available
		{
			// we're gunna halt, so this is the time to clear the flag
			if (m_bSuppressStoreForAltBackspaceKeypress)
                m_SaveTargetPhrase.Empty();
			m_bSuppressStoreForAltBackspaceKeypress = FALSE; // make sure it's off before returning

			pNewPile = pApp->m_pActivePile; 
            // whm modified 22Feb2018 to remove logic related to Cancel and Select.
            // [Original comment below]:
            // ensure its valid, we may get here after a
            // RecalcLayout call when there is no adaptation available from the LookAhead,
            // (or user cancelled when shown the Choose Translation dialog from within the
            // LookAhead() function, having matched) we must cause auto lookup and
            // inserting to be turned off, so that the user can do a manual adaptation; but
            // if the m_bAcceptDefaults flag is on, then the copied source (having been
            // through c.changes) is accepted without user input, the m_bAutoInsert flag is
            // turned back on, so processing will continue
            pApp->m_nActiveSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
			if (!pApp->m_bSingleStep)
			{
				HandleUnsuccessfulLookup_InAutoAdaptMode_AsBestWeCan(pApp, pView, pNewPile, bWantSelect);
			}
			else // it's single step mode
			{
				HandleUnsuccessfulLookup_InSingleStepMode_AsBestWeCan(pApp, pView, pNewPile, bWantSelect);
			}

            // get a widened pile pointer for the new active location, and we want the
            // pile's strip to be marked as invalid and the strip index added to the
            // CLayout::m_invalidStripArray
			if (pNewPile != NULL)
			{
				pDoc->ResetPartnerPileWidth(pNewPile->GetSrcPhrase());
			}
		}

		// initialize the phrase box too, so it doesn't carry the old string to the next
		// pile's cell
        this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase); //SetWindowText(pApp->m_targetPhrase);

		// if we merged and moved, we have to update pNewPile, because we have done a
		// RecalcLayout in the LookAhead() function; it's possible to return from
		// LookAhead() without having done a recalc of the layout, so the else block
		// should cover that situation
        if (m_bCompletedMergeAndMove)
		{
			pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
		}
		else
		{
		    // do we need this one?? I think so
    #ifdef _NEW_LAYOUT
		    pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
    #else
		    pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
    #endif
		    // in call above, m_stripArray gets rebuilt, but m_pileList is left untouched

		    // get the new active pile
		    pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
		    wxASSERT(pApp->m_pActivePile != NULL);
		}

        // if the user has turned on the sending of synchronized scrolling messages send
        // the relevant message once auto-inserting halts, because we don't want to make
        // other applications sync scroll during auto-insertions, as it could happen very
        // often and the user can't make any visual use of what would be happening anyway;
        // even if a Cancel and Select is about to be done, a sync scroll is appropriate
        // now, provided auto-inserting has halted
		if (!pApp->m_bIgnoreScriptureReference_Send && !pApp->m_bAutoInsert)
		{
			pView->SendScriptureReferenceFocusMessage(pApp->m_pSourcePhrases,
													pApp->m_pActivePile->GetSrcPhrase());
		}

		// update status bar with project name
		pApp->RefreshStatusBarInfo();

		// recreate the phraseBox using the stored information
		if (bWantSelect)
		{
			pApp->m_nStartChar = -1; // WX uses -1, not 0 as in MFC
			pApp->m_nEndChar = -1;
		}
		else
		{
			int len = GetLineLength(0); // 0 = first line, only line
			pApp->m_nStartChar = len;
			pApp->m_nEndChar = len;
		}

        // fix the m_bSaveToKB flag, depending on whether or not srcPhrase is in kb; but
        // this applies only when adapting, not glossing
		if (!gbIsGlossing && !pApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry &&
			pApp->m_pActivePile->GetSrcPhrase()->m_bNotInKB)
		{
			pApp->m_bSaveToKB = FALSE;
			pApp->m_targetPhrase.Empty();
		}
		else
		{
			pApp->m_bSaveToKB = TRUE;
		}

        m_bCompletedMergeAndMove = FALSE; // make sure it's cleared

		// BEW note 24Mar09: later we may use clipping (the comment below may not apply in
		// the new design anyway)
		pView->Invalidate(); // do the whole client area, because if target font is larger
            // than the source font then changes along the line throw words off screen and
            // they get missed and eventually app crashes because active pile pointer will
            // get set to NULL
		pLayout->PlaceBox();

		if (bWantSelect)
			SetModify(TRUE); // our own SetModify(); calls MarkDirty()
		else
			SetModify(FALSE); // our own SetModify(); calls DiscardEdits()

        // make sure gbSuppressMergeInMoveToNextPile is reset to the default value
        m_bSuppressMergeInMoveToNextPile = FALSE;

        return TRUE;
	}
}

// BEW 13Apr10, no changes needed for support of doc version 5
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
		//if (pCell->m_pPile == pActivePile)
		if (pCell->GetPile() == pActivePile)
		{
			bYes = TRUE;
			break;
		}
	}
	return bYes;
}

void CPhraseBox::CloseDropDown()
{
    // whm 11July2018 modified this CloseDropDown() function to just
    // hide the list control.
    // Note: We don't hide button
    this->GetDropDownList()->Hide();
}

void CPhraseBox::PopupDropDownList()
{
    // whm 12Apr2019 modified.
    // This PopupDropDownList() is only called from CMainFrame::OnIdle().
    // I moved the ->Show() command to end of PopupDropDownList() (below)
    // so that when painted on the screen, it will be in its final adjusted size.
    // Perhaps this change will help eliminate any unexpected selection of 
    // a different list item due to the list not being freshly painted.
    // Note: Button is not hidden, just show the list
    //this->GetDropDownList()->Show();
    CAdapt_ItApp* pApp = &wxGetApp();
	//CLayout* pLayout = pApp->GetLayout();
    wxASSERT(pApp != NULL);
	int rectWidth = this->GetTextCtrl()->GetRect().GetWidth();

    // whm 24Jul2018 added. Extend the list horizontally so it spans the whole
    // area under the edit box and the dropdown button.
    int buttonWidth = this->GetPhraseBoxButton()->GetRect().GetWidth();
    rectWidth += (buttonWidth + 1); // increment rectWidth by width of button and 1-pixel space between.

	//this->GetTextCtrl()->GetRect().GetWidth()   // TODO for BEW: Check the following addition of a canvas Refresh() call to see if
    // it helps prevent the list from registering a wrong index for list item click.
    pApp->GetMainFrame()->canvas->Refresh();

#if defined(_DEBUG)
	{
		CSourcePhrase* pSPhr = gpApp->m_pActivePile->GetSrcPhrase();
		{
			wxTextCtrl* pTxtBox = gpApp->m_pTargetBox->GetTextCtrl();
			CMyListBox* pListBox = gpApp->m_pTargetBox->GetDropDownList();
			int boxWidth = pTxtBox->GetClientRect().width;
			wxSize sizeList = pListBox->GetClientSize();
			int listWidth = sizeList.x;
			wxLogDebug(_T("%s::%s() line %d: *BEFORE* SetSizeAndH: curr boxWidth %d , rectWidth  %d , (my calc) listWidth  %d , tgt = %s"),
				__FILE__, __FUNCTION__, __LINE__, boxWidth, rectWidth, listWidth, pSPhr->m_adaption.c_str());
			wxLogDebug(_T("%s::%s() line %d: pApp -> m_bWizardIsRunning = %d and m_bJustLaunched = %d"),
				__FILE__, __FUNCTION__, __LINE__, (int)gpApp->m_bWizardIsRunning , (int)gpApp->m_bJustLaunched);

		}
	}
#endif

	this->SetSizeAndHeightOfDropDownList(rectWidth);


#if defined(_DEBUG)
	{
		CSourcePhrase* pSPhr = gpApp->m_pActivePile->GetSrcPhrase();
		{
			wxLogDebug(_T("%s::%s() line %d: *AFTER* SetSizeAndH... , rectWidth should still be: %d , tgt = %s"),
				__FILE__, __FUNCTION__, __LINE__, rectWidth, pSPhr->m_adaption.c_str());
		}
	}
#endif
    this->GetDropDownList()->Show();
	//bool bIsFrozen = gpApp->GetLayout()->m_pCanvas->IsFrozen();
	//if (bIsFrozen)
	//	wxLogDebug(_T("Canvas is Frozen at end of PopupDropDownList()"));
	//else
	//	wxLogDebug(_T("Canvas is NOT Frozen at end of PopupDropDownList()"));

}

void CPhraseBox::HidePhraseBox()
{
    // Hide all 3 parts of the new phrasebox
    this->GetTextCtrl()->Hide();
    this->GetPhraseBoxButton()->Hide();
    this->GetDropDownList()->Hide();
}

// This SetSizeAndHeightOfDropDownList() function attempts to calculate the
// vertical size needed for the phrasebox's new dropdown list to make its
// items visible on screen. 
// TODO for BEW: The function should detect how much of the logical doc is 
// available for displaying its list, as well as perhaps how much screen space  
// is available from the current scroll position. It should make adjustments to
// accommodate the best view. Note that the wxListBox does have a vertical
// scroll bar on all platforms that appears if the number of items cannot be
// shown within the current vertical height setting of the list. Some platforms
// such as Linux, also put a horizontal scroll bar at the bottom of the list
// which is not so helpful there since it appears sometimes even when all
// items in the list are easily visible without the need for horizontal 
// scrolling.
void CPhraseBox::SetSizeAndHeightOfDropDownList(int width)
{
	// BEW 27July21, refactored to change the width value to encompass the widest string of the list
	// 
	// Legacy Comment: The incoming width parameter is set by the caller in the View's ResizeBox,  
    // and is the width of the phrasebox's edit box at the time this function is called.
    // We use the width value as it comes in, but we have to calculate the height value that
    // will fit the number of items, or the space available below the phrasebox on the screen.
    // The wxListBox automatically shows a scroll bar at right if the list cannot open tall
    // enough to fit all item in the max height available.
    CAdapt_ItApp* pApp = &wxGetApp();
    wxASSERT(pApp != NULL);

	// Get the text extent height of each of the combobox items - all should have same height -
    // and get a total height needed for the list. The total height is a sum of all vertical
    // text extents, plus an approximation of the amount of leading above, below and between
    // each item in the list.
    wxClientDC dC((wxWindow*)gpApp->GetMainFrame()->canvas);
    wxFont* pFont;
    wxSize textExtent;
    if (gbIsGlossing && gbGlossingUsesNavFont)
        pFont = gpApp->m_pNavTextFont;
    else
        pFont = gpApp->m_pTargetFont;
    wxFont SaveFont = dC.GetFont();

    dC.SetFont(*pFont);
    int nItems = this->GetDropDownList()->GetCount();
    int ct;
    int totalX = 0;
    int totalY = 0;
    wxString theItem;
    for (ct = 0; ct < nItems; ct++)
    {
        theItem = this->GetDropDownList()->GetString(ct);
        dC.GetTextExtent(theItem, &textExtent.x, &textExtent.y); // measure using the current font
        totalX += textExtent.x; // we don't use this 
        totalY += textExtent.y;
    }
    // whm 13Jul2018 Note: The default leading/spacing between the items in a wxListBox varies widely between
    // Windows and Linux. On Windows a value of 3 pixels is enough to open the list with some empty space at
    // the bottom. However, on Linux a value of even 10 pixels is too small to see all the list items. 
    // What about the Mac? TODO: Graeme will need to test by experimenting with the nLeadingPixels value below.
    // For now, we'll conditionally compile an approximate value for all platforms:
    int nLeadingPixels;
#if defined (__WXMSW__)
    nLeadingPixels = 2; // whm 15Aug2018 after testing changed the windows nLeadingPixels value from 3 to 2.
#elif defined (__WXGTK__)
    nLeadingPixels = 11; // whm 15Aug2018 after texting changed the Linux nLeadingPixels value from 12 to 11.
#elif defined (__WXMAC__)
    nLeadingPixels = 4; // TODO: Graeme, experiment with various values here to see what works best for a variety of font sizes
#endif
    // Add a value of nLeadingPixels of space between list items to the totalY value. The number of leading
    // spaces is nItems + 1.
    // TODO: Test the 3-pixel leading value on Linux and Mac.
    totalY = totalY + ((nItems + 1) * nLeadingPixels);
    // Finally call SetSize() with the new values
	// Finally call SetSize() with the new value
	pApp->m_pTargetBox->GetDropDownList()->SetSize(width, totalY);

#if defined(_DEBUG) //&& defined(_OVERLAP)
	{
		CSourcePhrase* pSPhr = gpApp->m_pActivePile->GetSrcPhrase();
		{
			wxTextCtrl* pTxtBox = gpApp->m_pTargetBox->GetTextCtrl();
			//CMyListBox* pListBox = gpApp->m_pTargetBox->GetDropDownList();
			int boxWidth = pTxtBox->GetClientRect().width;
			//wxSize sizeList = pListBox->GetClientSize();
			//int listWidth = sizeList.x;
			wxLogDebug(_T("%s::%s() line %d: EXITING:  curr boxWidth %d, list rectWidth passed in: %d , tgt = %s"),
				__FILE__, __FUNCTION__, __LINE__, boxWidth, width, pSPhr->m_adaption.c_str());
		}
	}
#endif

}

// whm 17Jul2018 added the following function to return the selected dropdown list
// string, modified for insertion into the phrasebox or for comparison with existing
// phrasebox contents. The function does the following adjustments to the list item
// string before returning it:
// 1. Removes any _("<no adaptation>") list item value as it doesn't belong in the phrasebox
// 2. If the parameter bSetEmptyAdaptationChosen is TRUE, it sets the m_bEmptyAdaptationChosen
//    member to TRUE to inform code elsewhere that the empty string represents a no adaptation
//    choice on part of user.
// 3. Calls FwdSlashtoZWSP() on the list item string.
// 4. Adjusts the string for case if gbAutoCaps && gbSourceIsUpperCase.
// 5. Returns the processed string to the caller.
// Called From: GetListItemAdjustedforPhraseBox() is called from two places:
//   From: OnListBoxItemSelected() to process a list item before inserting it into the phrasebox - with TRUE parameter
//   From: CPhraseBox::OnKeyUp()'s Enter/Tab handling - to compare list item string with phrasebox contents with FALSE parameter
wxString CPhraseBox::GetListItemAdjustedforPhraseBox(bool bSetEmptyAdaptationChosen)
{
    wxString s;
    s = s.Format(_("<no adaptation>")); // get "<no adaptation>" ready in case needed

    wxString selItemStr;
    selItemStr = this->GetDropDownList()->GetStringSelection();
    wxLogDebug(_T("List Item Selected: %s"), selItemStr.c_str());
    if (selItemStr == s)
    {
        selItemStr = _T(""); // restore null string
        if (bSetEmptyAdaptationChosen)
            m_bEmptyAdaptationChosen = TRUE; // set the m_bEmptyAdaptationChosen global used by PlacePhraseBox
    }

    //#if defined(FWD_SLASH_DELIM)
    // BEW added 23Apr15 - in case the user typed a translation manually (with / as word-delimiter)
    // convert any / back to ZWSP, in case KB storage follows. If the string ends up in m_targetBox
    // then the ChangeValue() call within CPhraseBox will convert the ZWSP instances back to forward
    // slashes for display, in case the user does subsequent edits there
    selItemStr = FwdSlashtoZWSP(selItemStr);
    //#endif

    if (gbAutoCaps && gbSourceIsUpperCase)
    {
        bool bNoError = gpApp->GetDocument()->SetCaseParameters(selItemStr, FALSE);
        if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
        {
            // make it upper case
            selItemStr.SetChar(0, gcharNonSrcUC);
        }
    }

    return selItemStr;
}

// return TRUE if we made a match and there is a translation to be inserted (see static var
// below); return FALSE if there was no match. This function assumes that the pNewPile pointer
// passed in is the active pile, and that CAdapt_ItApp::m_nActiveSequNum is the correct
// index within the m_pSourcePhrases list for the CSourcePhrase instance pointed at by the
// m_pSrcPhrase member of pNewPile
// BEW 13Apr10, a small change needed for support of doc version 5
// BEW 21Jun10, updated for support of kbVersion 2
// BEW 13Nov10, changed by Bob Eaton's request, for glossing KB to use all maps
bool CPhraseBox::LookAhead(CPile* pNewPile)
{
	// refactored 25Mar09 (old code is at end of file)
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView *pView = pApp->GetView(); // <<-- BEWARE if we later support multiple views/panes
	CLayout* pLayout = GetLayout();
	CSourcePhrase* pSrcPhrase = pNewPile->GetSrcPhrase();
	int		nNewSequNum = pSrcPhrase->m_nSequNumber; // sequ number at the new location
	wxString	phrases[10]; // store built phrases here, for testing against KB stored source phrases
	int		numPhrases;  // how many phrases were built in any one call of this LookAhead function
    m_Translation.Empty(); // clear the static variable, ready for a new translation, or gloss,
						 // if one can be found
	m_nWordsInPhrase = 0;	  // the global, initialize to value for no match
	m_bBoxTextByCopyOnly = FALSE; // restore default setting

	// we should never have an active selection at this point, so ensure there is no selection
	pView->RemoveSelection();


	// build the as many as 10 phrases based on first word at the new pile and the following
	// nine piles, or as many as are valid for phrase building (there are 7 conditions which
	// will stop the builds). When adapting, all 10 can be used; when glossing, and when
	// glossing, it now also supports more than one for KB insertions (but disallows merger)
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
	// BEW 6Aug13, added a gbIsGlossing == TRUE block to the next test
	if (numPhrases == 0)
	{
		if (gbIsGlossing)
		{
			if (pSrcPhrase->m_nSrcWords > 1 && !pSrcPhrase->m_gloss.IsEmpty())
			{
                m_Translation = pSrcPhrase->m_gloss;
				m_nWordsInPhrase = 1;
				pApp->m_bAutoInsert = FALSE; // cause a halt
				return TRUE;
			}
			else
			{
                // whm Note: return empty m_Translation string when pSrcPhrase->m_nSrcWords is not > 1 OR pSrcPhrase->m_gloss is EMPTY
				return FALSE; // return empty string in the m_Translation global wxString
			}
		}
		else
		{
			if (pSrcPhrase->m_nSrcWords > 1 && !pSrcPhrase->m_adaption.IsEmpty())
			{
                m_Translation = pSrcPhrase->m_adaption;
				m_nWordsInPhrase = 1; // strictly speaking not correct, but it's the value we want
									// on return to the caller so it doesn't try a merger
				pApp->m_bAutoInsert = FALSE; // cause a halt
				return TRUE;
			}
			else
			{
                // whm Note: return empty m_Translation string when pSrcPhrase->m_nSrcWords is not > 1 OR pSrcPhrase->m_adaption is EMPTY
                return FALSE; // return empty string in the m_Translation global wxString
			}
		}
	}

	// check these phrases, longest first, attempting to find a match in the KB
	// BEW 13Nov10, changed by Bob Eaton's request, for glossing KB to use all maps
	CKB* pKB;
	int nCurLongest; // index of the map which is highest having content, maps at higher index
					 // values currently have no content
	if (gbIsGlossing)
	{
		pKB = pApp->m_pGlossingKB;
		nCurLongest = pKB->m_nMaxWords; // no matches are possible for phrases longer
										// than nCurLongest when adapting
		//nCurLongest = 1; // the caller should ensure this, but doing it explicitly here is
						 // some extra insurance to keep matching within the only map that exists
						 // when the glossing KB is in use
	}
	else
	{
		pKB = pApp->m_pKB;
		nCurLongest = pKB->m_nMaxWords; // no matches are possible for phrases longer
										// than nCurLongest when adapting
	}
	CTargetUnit* pTargetUnit = (CTargetUnit*)NULL;
	int index = nCurLongest - 1;
	bool bFound = FALSE;
	while (index > -1)
	{
		bFound = pKB->FindMatchInKB(index + 1, phrases[index], pTargetUnit);
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
		pApp->pCurTargetUnit = (CTargetUnit*)NULL; // the global pointer must be cleared
		m_CurKey.Empty(); // global var m_CurKey not needed, so clear it
		return FALSE;
	}
    // whm 10Jan2018 Note: We do not call the ChooseTranslation dialog from LookAhead()
    // now that the choose translation feature is implemented in the CPhraseBox's dropdown
    // list. Hence, we should not set pTargetUnitFromChooseTrans here, since it should only be set
    // from the View's OnButtonChooseTranslation() handler.

    // whm 5Mar2018 Note: Originally on 10Jan2018 I commented out the next line as it was
    // primarily used to store the target unit supplied by the ChooseTranslation dialog,
    // however, we can extend its usefulness for my current refactoring of the dropdown
    // setup code in Layout's PlaceBox() function.
    // I've also renamed it from pTargetUnitFromChooseTrans back to its original name of 
    // pCurTargetUnit.
	pApp->pCurTargetUnit = pTargetUnit; // set global pointer so the dialog can use it if it 
                                        // is called
	m_CurKey = phrases[index]; // set the m_CurKey so the dialog can use it if it is called
							 // m_CurKey is a global variable (the lookup of phrases[index] is
							 // done on a copy, so m_CurKey retains the case of the key as in
							 // the document; but the lookup will have set global flags
							 // such as gbMatchedKB_UCentry to TRUE or FALSE as appropriate)
	m_nWordsInPhrase = index + 1; // static variable, needed for merging source phrases in
								// the caller; when glossing, index will == 0 so no merge will
								// be asked for below while in glossing mode

	// we found a target unit in the list with a matching m_key field, so we must now set
	// the static var translation to the appropriate adaptation, or gloss, text: this
	// will be the target unit's first entry in its list if the list has only one entry, else
	// we must present the user with a dialog to put up all possible adaptations (or glosses) for
	//  the user to choose one, or type a new one, or reject all - in which case we return
	// FALSE for manual typing of an adaptation (or gloss) etc. For autocapitalization support,
	// the dialog will show things as stored in the KB (if auto caps is ON, that could be with
	// all or most entries starting with lower case) and we let any needed changes to initial
	// upper case be done automatically after the user has chosen or typed.
	TranslationsList::Node *node = pTargetUnit->m_pTranslations->GetFirst();
	wxASSERT(node != NULL);
	//BEW 21Jun10, for kbVersion 2 we want a count of all non-deleted CRefString
	//instances, not the total number in the list, because there could be deletions stored
	//int count = pTargetUnit->m_pTranslations->GetCount();
	int count = pTargetUnit->CountNonDeletedRefStringInstances();
	// For kbVersion 2, a CTargetUnit can store only deleted CRefString instances, so that
	// means count can be zero - if this is the case, this is an 'unmatched' situation,
	// and should be handled the same as the if(!bFound) test above
	if (count == 0)
	{
		pApp->pCurTargetUnit = (CTargetUnit*)NULL; // the global pointer must be cleared
		m_CurKey.Empty(); // global var m_CurKey not needed, so clear it
		return FALSE;
	}
	wxASSERT(count > 0);
	if (count > 1 || pTargetUnit->m_bAlwaysAsk)
	{
		// move to new location, merge if necessary, so user has visual feedback about which
		// element(s) is involved
		pView->RemoveSelection();

		// set flag for telling us that we will have completed the move when the dialog is shown
		m_bCompletedMergeAndMove = TRUE;

		CPile* pPile = pView->GetPile(pApp->m_nActiveSequNum);
		wxASSERT(pPile);

		// create the appropriate selection there
		CCell* pAnchorCell = pPile->GetCell(0); // always cell [0] in the refactored design

		// we do the merger here in LookAhead() rather than in the caller, such as
		// MoveToNextPile(), so that the merger can be seen by the user at the time it is
		// done (and helpful to see) rather than later
		if (m_nWordsInPhrase > 0)
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
			wxColour textColor = pAnchorCell->GetColor();
			pAnchorCell->DrawCell(&aDC,textColor);
			pApp->m_bSelectByArrowKey = FALSE;
			pAnchorCell->SetSelected(TRUE);

			// preserve record of the selection
			pSelection->Append(pAnchorCell);

			// extend the selection to the right, if more than one pile is involved
			if (m_nWordsInPhrase > 1)
			{
				// extend the selection
				pView->ExtendSelectionForFind(pAnchorCell, m_nWordsInPhrase);
			}
		}

        // whm 5Mar2018 Note about original comment below. It is no longer possible to have
        // a situation in which the "user cancels the dialog" as there is no longer any
        // possibility of the ChooseTranslation dialog appearing during LookAhead(). Also
        // the phrase box should now be shown at it new location and there is no need for
        // "the old state will have to be restored...any merge needs to be unmerged".
        //
		// the following code added to support Bill Martin's wish that the phrase box be shown
		// at its new location when the dialog is open (if the user cancels the dialog, the old
		// state will have to be restored - that is, any merge needs to be unmerged)
		pApp->m_pTargetBox->m_bAbandonable = FALSE;
		// adaptation is available, so use it if the source phrase has only a single word, but
		// if it's multi-worded, we must first do a merge and recalc of the layout

		if (m_nWordsInPhrase > 1) // m_nWordsInPhrase is a global, set in this function
								// or in LookUpSrcWord()
		{
			// do the needed merge, etc.
			pApp->bLookAheadMerge = TRUE; // set static flag to ON
			bool bSaveFlag = m_bAbandonable; // the box is "this"
			pView->MergeWords(); // calls protected OnButtonMerge() in CAdapt_ItView class
			m_bAbandonable = bSaveFlag; // preserved the flag across the merge
			pApp->bLookAheadMerge = FALSE; // restore static flag to OFF
            m_bSuppressMergeInMoveToNextPile = TRUE; // restored 24Feb2018 after being accidentally removed
        }
		else
			pView->RemoveSelection(); // glossing, or adapting a single src word only

        // whm 27Feb2018 Note: The following block that sets this m_pTargetBox to be empty
        // and the App's m_targetPhrase to _T("") needs to be removed for phrasebox contents
        // to appear in the dropdown phrasebox.
        //
		// next code is taken from end of MoveToNextPile()
		// initialize the phrase box to be empty, so as not to confuse the user
		//if (GetHandle() != NULL) // This won't happen (a NULL handle) in wx version since we don't destroy the targetbox window
		//{
		//	// wx version note: we do the following elsewhere when we hide the m_pTargetBox
		//	ChangeValue(_T(""));
		//	pApp->m_targetPhrase = _T("");
		//}

		// recalculate the layout
#ifdef _NEW_LAYOUT
		pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
		pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif

		// get the new active pile
		pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
		wxASSERT(pApp->m_pActivePile != NULL);

		// scroll into view
		pApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);

        // make what we've done visible
		pView->Invalidate();
		pLayout->PlaceBox();

        // [old comment below]
        // put up a dialog for user to choose translation from a list box, or type new one
		// (note: for auto capitalization; ChooseTranslation (which calls the CChoseTranslation
		// dialog handler, only sets the 'translation' global variable, it does not make any case
		// adjustments - these, if any, must be done in the caller)

        // whm note: The saveAutoInsert assignment below is now unnecessary.
        // since ChooseTranslation() is no longer called from LookAhead().
        // It is sufficient to just to set the App's m_bAutoInsert = FALSE.

        // [old comment below] 
        // wx version addition: In wx the OnIdle handler continues to run even when modal
        // dialogs are being shown, so we'll save the state of m_bAutoInsert before calling
        // ChooseTranslation change m_bAutoInsert to FALSE while the dialog is being shown,
        // then restore m_bAutoInsert's prior state after ChooseTranslation returns.
        // whm [old] update: I fixed the problem by using a derived AIModalDialog that turns off
        // idle processing while modal dialogs are being shown modal, but the following
        // doesn't hurt.

        // whm note: Unilaterally set the m_bAutoInsert flag to FALSE so that the movement of the phrasebox
        // will halt, giving the user opportunity to interact with the just-shown dropdown combobox.
        //bool saveAutoInsert = pApp->m_bAutoInsert;
        pApp->m_bAutoInsert = FALSE;

        // whm 10Jan2018 modified the code below by removing the call of the ChooseTranslation 
        // dialog from within this LookAhead() function.
        // The CPhraseBox is now implemented with a dropdown list, and now it will 
        // always have the available translations in its dropdown list - ready to popup 
        // from within the new phrasebox at the point a PlaceBox() call is made.
        // LookAhead() will always end at some point with a call to PlaceBox(), which
        // will determine whether the phrasebox's built-in list should be shown to the user.
        // Hence, there is no longer any need to call the CPhraseBox::ChooseTranlsation()
        // function from here.
        // Note: At any time the phrasebox is at a location, the actual modal Choose Translation 
        // dialog can still be called manually via the usual toolbar button, or by using the F8 
        // or Ctrl+L hotkeys. Now that the phrasebox implements a dropdown list, all 
        // methods of summoning the ChooseTranslation dialog make use of the 
        // Adapt_ItView::ChooseTranslation() function, rather than the old 
        // CPhraseBox::ChooseTranslation() function of the same name (now removed from the codebase).

		// BEW changed 9Apr12 for support of discontinuous highlighting spans...
		// A new protocol is needed here. The legacy code wiped out any auto-insert
		// highlighting already accumulated, and setup for a possible auto-inserted span
		// starting at the NEXT pile following the merger location. The user-choice of the
		// adaptation or gloss rightly ends the previous auto-inserted span, but we want
		// that span to be visible while the ChooseTranslation dialog is open; so here we
		// do nothing, and in the caller (which usually is MoveToNextPile() we clobber the
		// old highlighted span and setup for a potential new one
//#ifdef Highlighting_Bug
//		// BEW changed 9Apr12 for support of discontinuous highlighting spans
//		wxLogDebug(_T("PhraseBox::LookAhead(), hilited span ends at merger at: sequnum = %d  where the user chose:  %s  for source:  %s"),
//			pApp->m_nActiveSequNum, translation, pApp->m_pActivePile->GetSrcPhrase()->m_srcPhrase);
//#endif
	}
	else // count == 1 case (ie. a unique adaptation, or a unique gloss when glossing)
	{
        // BEW added 08Sep08 to suppress inserting placeholder translations for ... matches
        // when in glossing mode and within the end of a long retranslation
		if (m_CurKey == _T("..."))
		{
			// don't allow an ellipsis (ie. placeholder) to trigger an insertion,
			// leave translation empty
            m_Translation.Empty();
			return TRUE;
		}
		// BEW 21Jun10, we have to loop to find the first non-deleted CRefString instance,
		// because there may be deletions stored in the list
		CRefString* pRefStr = NULL;
		while (node != NULL)
		{
			pRefStr = node->GetData();
			node = node->GetNext();
			wxASSERT(pRefStr);
			if (!pRefStr->GetDeletedFlag())
			{
                m_Translation = pRefStr->m_translation;
				break;
			}
		}
	}

	// adjust for case, if necessary; this algorithm will not make a lower case string start
	// with upper case when the source is uppercase if the user types punctuation at the start
	// of the string. The latter is, however, unlikely - provided the auto punctuation support
	// is utilized by the user
	if (gbAutoCaps && gbSourceIsUpperCase)
	{
		bool bNoError = pApp->GetDocument()->SetCaseParameters(m_Translation,FALSE);
		if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
		{
			// make it upper case
            m_Translation.SetChar(0,gcharNonSrcUC);
		}
	}
	return TRUE;
}

// BEW 13Apr10, no changes needed for support of doc version 5
// BEW 8July10, no changes needed for support of kbVersion 2
void CPhraseBox::JumpForward(CAdapt_ItView* pView)
{

#ifdef _FIND_DELAY
		wxLogDebug(_T("9. Start of JumpForward"));
#endif
	// refactored 25Mar09
	CLayout* pLayout = GetLayout();
	CAdapt_ItApp* pApp = pLayout->m_pApp;

	// BEW 4May18 help first, and only first, CSourcePhrase to retain m_adaption or
	// m_gloss when box moves forward by Enter or Tab press
	if (pApp->m_bVertEdit_AtFirst)
	{
		// BEW 3May18 If we are in vertical editing mode, PlaceBox() only is called, there is 
		// no call to PlacePhraseBox() and hence none of the latter's logic is available for setting
		// m_pTargetBox contents, nor m_targetPhase, nor m_adaption, nor source phrase's m_adaption
		// and so we'll need to do it somewhere - here seems like a good place, because MoveToNextPile()
		// will call DoStore_NormalOrTransliterateModes(pApp, pDoc, pView, pCurPile) and we want these
		// storage members to have correct values before that happens below. Out of vertical edit mode,
		// PlacePhraseBox() normally sets the storage members updated for the KB store. If we don't do
		// the task here, then the updated pOldActiveSrcPhrase will have the old target text - before
		// any user edits could be done
		if (gbVerticalEditInProgress)
		{
			wxString theText = pApp->m_targetPhrase; // this should have been set correctly 
													 // from the prior KB store operation
            this->GetTextCtrl()->ChangeValue(theText);
			pApp->m_pTargetBox->m_Translation = theText;
			if (gbIsGlossing)
			{
				pApp->m_pActivePile->GetSrcPhrase()->m_gloss = theText;
			}
			else
			{
				CSourcePhrase* pSrcPhr = pApp->m_pActivePile->GetSrcPhrase();
				pSrcPhr->m_adaption = theText;

				// check punctuation, if any, and compute m_targetStr value 
				if (!pSrcPhr->m_precPunct.IsEmpty())
				{
					theText = pSrcPhr->m_precPunct + theText;
				}
				if (!pSrcPhr->m_follPunct.IsEmpty())
				{
					theText += pSrcPhr->m_follPunct;
				}
				if (!pSrcPhr->GetFollowingOuterPunct().IsEmpty())
				{
					theText += pSrcPhr->GetFollowingOuterPunct();
				}
				pApp->m_pActivePile->GetSrcPhrase()->m_targetStr = theText;
//#if defined(_DEBUG)
//				CSourcePhrase* mySrcPhrasePtr = pApp->m_pActivePile->GetSrcPhrase();
//				wxLogDebug(_T("VerticalEdit: JumpForward() at start: sn = %d , src key = %s , m_adaption = %s , m_targetStr = %s , m_targetPhrase = %s"),
//					mySrcPhrasePtr->m_nSequNumber, mySrcPhrasePtr->m_key.c_str(), mySrcPhrasePtr->m_adaption.c_str(),
//					mySrcPhrasePtr->m_targetStr.c_str(), pApp->m_targetPhrase.c_str());
//#endif
			}
		}
		pApp->m_pTargetBox->m_bAbandonable = FALSE;
		pApp->m_bVertEdit_AtFirst = FALSE; // reset FALSE so other CSourcePhrases don't do this fix-it hack
	}

	if (pApp->m_bDrafting)
	{
        // get an adjusted pile pointer for the new active location, and we want the
        // pile's strip to be marked as invalid and the strip index added to the
        // CLayout::m_invalidStripArray
		pLayout->m_pDoc->ResetPartnerPileWidth(pApp->m_pActivePile->GetSrcPhrase());

		// check the mode, whether or not it is single step, and route accordingly
		if (pApp->m_bSingleStep)
		{
			int bSuccessful;
			if (pApp->m_bTransliterationMode && !gbIsGlossing)
			{
				bSuccessful = MoveToNextPile_InTransliterationMode(pApp->m_pActivePile);
			}
			else
			{
				bSuccessful = MoveToNextPile(pApp->m_pActivePile);
			}
			if (!bSuccessful)
			{
				// BEW added 4Sep08 in support of transitioning steps within vertical
				// edit mode
				if (gbVerticalEditInProgress && m_bTunnellingOut)
				{
                    // MoveToNextPile might fail within the editable span while vertical
                    // edit is in progress, so we have to allow such a failure to not cause
                    // tunnelling out; hence we use the m_bTunnellingOut global to assist -
                    // it is set TRUE only when
                    // VerticalEdit_CheckForEndRequiringTransition() in the view class
                    // returns TRUE, which means that a PostMessage(() has been done to
                    // initiate a step transition
					m_bTunnellingOut = FALSE; // caller has no need of it, so clear to
											 // default value
					pLayout->m_docEditOperationType = no_edit_op;
					return;
				}

                // if in vertical edit mode, the end of the doc is always an indication
                // that the edit span has been traversed and so we should force a step
                // transition, otherwise, continue to n:(tunnel out before m_nActiveSequNum
                // can be set to -1, which crashes the app at redraw of the box and recalc
                // of the layout)
				if (gbVerticalEditInProgress)
				{
					bool bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(
										-1, nextStep, TRUE); // bForceTransition is TRUE
					if (bCommandPosted)
					{
						// don't proceed further because the current vertical edit step
						// has ended
						pLayout->m_docEditOperationType = no_edit_op;
						return;
					}
				}

				// BEW changed 9Apr12, to support discontinuous highlighting
				// spans for auto-insertions...
				// When we come to the end, we could get there having done some
				// auto-insertions, and so we'd want them to be highlighted in the normal
				// way, and so we do nothing here -- the user can manually clear them, or
				// position the box elsewhere and initiate the lookup-and-insert loop from
				// there, in which case auto-inserts would kick off with new span(s)

				pLayout->Redraw(); // remove highlight before MessageBox call below
				pLayout->PlaceBox();

				// tell the user EOF has been reached...
				// BEW added 9Jun14, don't show this message when in clipboard adapt mode, because
				// it will come up every time a string of text is finished being adapted, and that
				// soon become a nuisance - having to click it away each time
				if (!pApp->m_bClipboardAdaptMode)
				{
                    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                    gpApp->m_bUserDlgOrMessageRequested = TRUE;
                    wxMessageBox(_(
"The end. Provided you have not missed anything earlier, there is nothing more to adapt in this file."),
					_T(""), wxICON_INFORMATION | wxOK);
				}
				wxStatusBar* pStatusBar;
				CMainFrame* pFrame = pApp->GetMainFrame();
				if (pFrame != NULL)
				{
					pStatusBar = pFrame->GetStatusBar();
					wxString str = _("End of the file; nothing more to adapt.");
					pStatusBar->SetStatusText(str,0); // use first field 0
				}
				// we are at EOF, so set up safe end conditions
				pApp->m_targetPhrase.Empty();
				pApp->m_nActiveSequNum = -1;
				this->Hide(); // MFC version calls DestroyWindow()
                this->GetTextCtrl()->ChangeValue(_T("")); // need to set it to null str
													  // since it won't get recreated
				pApp->m_pActivePile = NULL; // can use this as a flag for at-EOF condition too

				pView->Invalidate();
				pLayout->PlaceBox();

                m_Translation.Empty(); // clear the static string storage for the
                    // translation save the phrase box's text, in case user hits SHIFT+END
                    // to unmerge a phrase
                m_SaveTargetPhrase = pApp->m_targetPhrase;
				pLayout->m_docEditOperationType = no_edit_op;

			    RestorePhraseBoxAtDocEndSafely(pApp, pView);  // BEW added 8Sep14
				return; // must be at EOF;
			} // end of block for !bSuccessful

			// for a successful move, a scroll into view is often needed
			pLayout->m_pCanvas->ScrollIntoView(pApp->m_nActiveSequNum);

			// get an adjusted pile pointer for the new active location, and we want the
			// pile's strip to be marked as invalid and the strip index added to the
			// CLayout::m_invalidStripArray
			pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
			CSourcePhrase* pSPhr = pApp->m_pActivePile->GetSrcPhrase();
			pLayout->m_pDoc->ResetPartnerPileWidth(pSPhr);

			// try to keep the phrase box from coming too close to the bottom of
			// the client area if possible
			int yDist = pLayout->GetStripHeight() + pLayout->GetCurLeading();
			wxPoint scrollPos;
			int xPixelsPerUnit,yPixelsPerUnit;
			pLayout->m_pCanvas->GetScrollPixelsPerUnit(&xPixelsPerUnit,&yPixelsPerUnit);

            // MFC's GetScrollPosition() "gets the location in the document to which the
            // upper left corner of the view has been scrolled. It returns values in
            // logical units." wx note: The wx docs only say of GetScrollPos(), that it
            // "Returns the built-in scrollbar position." I assume this means it gets the
            // logical position of the upper left corner, but it is in scroll units which
            // need to be converted to device (pixel) units

			wxSize maxDocSize; // renamed barSizes to maxDocSize for clarity
			pLayout->m_pCanvas->GetVirtualSize(&maxDocSize.x,&maxDocSize.y);
															// gets size in pixels

			pLayout->m_pCanvas->CalcUnscrolledPosition(0,0,&scrollPos.x,&scrollPos.y);
			// the scrollPos point is now in logical pixels from the start of the doc

			wxRect rectClient(0,0,0,0);
			wxSize canvasSize;
			canvasSize = pApp->GetMainFrame()->GetCanvasClientSize(); // with
								// GetClientRect upper left coord is always (0,0)
			rectClient.width = canvasSize.x;
			rectClient.height = canvasSize.y;


			if (rectClient.GetBottom() >= 4 * yDist) // do the adjust only if at least 4
													 // strips are showing
			{
				wxPoint pt = pApp->m_pActivePile->GetCell(1)->GetTopLeft(); // logical coords
																		// of top of phrase box

				// if there are not two full strips below the top of the phrase box, scroll down
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
							pApp->GetMainFrame()->canvas->ScrollDown(1);
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
                            // Note: MFC's ScrollWindow's 2 params specify the xAmount and
                            // yAmount to scroll in device units (pixels). The equivalent
                            // in wx is Scroll(x,y) in which x and y are in SCROLL UNITS
                            // (pixels divided by pixels per unit). Also MFC's ScrollWindow
                            // takes parameters whose value represents an "amount" to
                            // scroll from the current position, whereas the
                            // wxScrolledWindow::Scroll takes parameters which represent an
                            // absolute "position" in scroll units. To convert the amount
                            // we need to add the amount to (or subtract from if negative)
                            // the logical pixel unit of the upper left point of the client
                            // viewing area; then convert to scroll units in Scroll().
							pLayout->m_pCanvas->Scroll(0,posn);
							//Refresh(); // BEW 25Mar09, this refreshes wxWindow, that is,
							//the phrase box control - but I think we need a layout draw
							//here - so I've called view's Invalidate() at a higher level
							//- see below
						}
					}
				}
			} // end of test for more than or equal to 4 strips showing

			// refresh the view
			pLayout->m_docEditOperationType = relocate_box_op;
			pView->Invalidate(); // BEW added 25Mar09, see comment about Refresh 10 lines above
			pLayout->PlaceBox();

		} // end of block for test for m_bSingleStep == TRUE
		else // auto-inserting -- sets flags and returns, allowing the idle handler to call OnePass()
		{
			// cause auto-inserting using the OnIdle handler to commence
			pApp->m_bAutoInsert = TRUE;

			// User has pressed the Enter key  (OnChar() calls JumpForward())
			// BEW changed 9Apr12, to support discontinuous highlighting
			// spans for auto-insertions...
			// Since OnIdle() will call OnePass() and the latter will call
			// MoveToNextPile(), and it is in MoveToNextPile() that CCell's
			// m_bAutoInserted flag can get set TRUE, we only here need to ensure that the
			// current location is a kick-off one, which we can do by clearing any earlier
			// highlighting currently in effect
			pLayout->ClearAutoInsertionsHighlighting();
//#ifdef Highlighting_Bug
//			wxLogDebug(_T("\nPhraseBox::JumpForward(), kickoff, from  pile at sequnum = %d   SrcText:  "),
//				pSPhr->m_nSequNumber, pSPhr->m_srcPhrase);
//#endif
			// BEW 14Jan23, Ngalambirra got "Holy-Spirit" auto-copied from the source text, but she forgot to click in the phrasebox to make
			// the inserted text "stick" by causing m_bAbandonable to be reest to FALSE, - so when MoveToNextPile() got called, the phrasebox
			// at the kick-off location lost the "Holy-Spirit" content. She didn't notice, and the text - because of how Gupapuyngu works,
			// still made sense. This type of error is common, and potentially disasterous. I think the user should not have to click in
			// the box, when moving forward by ENTER or TAB key. OnKeyUp() handles ENTER or TAB keypress, and calls JumpForward() just before
			// exiting. So I will here check for m_bAbandonable still with TRUE value, and if so, reset it FALSE, so that a box click is
			// not required anymore. Clicking around holes in the doc will still autoclear the copied src text at each location, so my
			// original intent for such clicking still applies because JumpForward(pView) is not called in such a circumstance.
			if (pApp->m_pTargetBox->m_bAbandonable)
			{
				pApp->m_pTargetBox->m_bAbandonable = FALSE; // yep, checking and resetting FALSE here relieves the user of responsibility
															// to make the content 'stick' by first doing something, eg. a click, in the box
															// before using ENTER key or TAB key to cause an advance to a hole
			}

			pLayout->m_docEditOperationType = relocate_box_op;
		}
		// save the phrase box's text, in case user hits SHIFT+End to unmerge a
		// phrase
        m_SaveTargetPhrase = pApp->m_targetPhrase;
	} // end if for m_bDrafting
	else // we are in review mode
	{
		// we are in Review mode, so moves by the RETURN key can only be to
		// immediate next pile
		//int nOldStripIndex = pApp->m_pActivePile->m_pStrip->m_nStripIndex;

        // get an adjusted pile pointer for the new active location, and we want the
        // pile's strip to be marked as invalid and the strip index added to the
        // CLayout::m_invalidStripArray
        pApp->m_nActiveSequNum = pApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber;
		pLayout->m_pDoc->ResetPartnerPileWidth(pApp->m_pActivePile->GetSrcPhrase());

        // if vertical editing is on, don't do the move to the next pile if it lies in the
        // gray area or is at the bundle end; so just check for the sequence number going
        // beyond the edit span bound & transition the step if it has, asking the user what
        // step to do next
		if (gbVerticalEditInProgress)
		{
            //m_bTunnellingOut = FALSE; not needed here as once we are in caller, we've
            //tunnelled out
            //bForceTransition is FALSE in the next call
			int nNextSequNum = pApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber + 1;
			bool bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(
																nNextSequNum,nextStep);
			if (bCommandPosted)
			{
				// don't proceed further because the current vertical edit step has ended

                // NOTE: since step transitions call mode changing handlers, and because
                // those handlers typically do a store of the phrase box contents to the kb
                // if appropriate, we'll rely on it here and not do a store
				//m_bTunnellingOut = TRUE;
				pLayout->m_docEditOperationType = no_edit_op;
				return;
			}
			else
			{
				// BEW 19Oct15 No transition of vert edit modes,
				// so we can store this location on the app
				gpApp->m_vertEdit_LastActiveSequNum = pApp->m_nActiveSequNum;
#if defined(_DEBUG)
				wxLogDebug(_T("VertEdit PhrBox, JumpForward() storing loc'n: %d "), pApp->m_nActiveSequNum);
#endif
			}
		}
		if (pApp->m_bTransliterationMode)
		{
			::wxBell();
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            gpApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(_(
"Sorry, transliteration mode is not supported in review mode. Turn review mode off."),
			_T(""), wxICON_INFORMATION | wxOK);
			pLayout->m_docEditOperationType = no_edit_op;
			return;
		}

		// Note: transliterate mode is not supported in Review mode, so there is no
		// function such as MoveToImmedNextPile_InTransliterationMode()
		int bSuccessful = MoveToImmedNextPile(pApp->m_pActivePile);
		if (!bSuccessful)
		{
			CPile* pFwdPile = pView->GetNextPile(pApp->m_pActivePile); // old
                                // active pile pointer (should still be valid)
			//if ((!gbIsGlossing && pFwdPile->GetSrcPhrase()->m_bRetranslation) || pFwdPile == NULL)
			if (pFwdPile == NULL)
			{
				// tell the user EOF has been reached...
				// BEW added 9Jun14, don't show this message when in clipboard adapt mode, because
				// it will come up every time a string of text is finished being adapted, and that
				// soon become a nuisance - having to click it away each time
				if (!pApp->m_bClipboardAdaptMode)
				{
                    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                    gpApp->m_bUserDlgOrMessageRequested = TRUE;
                    wxMessageBox(_(
"The end. Provided you have not missed anything earlier, there is nothing more to adapt in this file."),
					_T(""), wxICON_INFORMATION | wxOK);
				}
				wxStatusBar* pStatusBar;
				CMainFrame* pFrame = pApp->GetMainFrame();
				if (pFrame != NULL)
				{
					pStatusBar = pFrame->GetStatusBar();
					wxString str = _("End of the file; nothing more to adapt.");
					pStatusBar->SetStatusText(str,0); // use first field 0
				}
				// we are at EOF, so set up safe end conditions
				this->Hide(); // whm added 12Sep04
                this->GetTextCtrl()->ChangeValue(_T("")); // need to set it to null
											// str since it won't get recreated
				pApp->m_pTargetBox->Enable(FALSE); // whm 12July2018 Note: It is re-enabled in ResizeBox()
				pApp->m_targetPhrase.Empty();
				pApp->m_nActiveSequNum = -1;
				pApp->m_pActivePile = NULL; // can use this as a flag for
											// at-EOF condition too
				// recalc the layout without any gap for the phrase box, as it's hidden
#ifdef _NEW_LAYOUT
				pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
				pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
			    RestorePhraseBoxAtDocEndSafely(pApp, pView); // BEW added 8Sep14
			}
			else // pFwdPile is valid, so must have bumped against a retranslation
			{
                // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                gpApp->m_bUserDlgOrMessageRequested = TRUE;
                wxMessageBox(_(
"Sorry, the next pile cannot be a valid active location, so no move forward was done."),
				_T(""), wxICON_INFORMATION | wxOK);
#ifdef _NEW_LAYOUT
				pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
				pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
				pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);

                pApp->m_pTargetBox->SetFocusAndSetSelectionAtLanding(); // whm 13Aug2018 modified

			}
            m_Translation.Empty(); // clear the static string storage for the translation
			// save the phrase box's text, in case user hits SHIFT+END to unmerge a phrase
            m_SaveTargetPhrase = pApp->m_targetPhrase;

			// get the view redrawn
			pLayout->m_docEditOperationType = no_edit_op;
			pView->Invalidate();
			pLayout->PlaceBox();
			return;
		} // end of block for test !bSuccessful
		else
		{
			// it was successful
			CCell* pCell = pApp->m_pActivePile->GetCell(1); // the cell
											// where the phraseBox is to be

            //added test for whether document at new
            //active location has a hole there or not; if it has, we won't permit a copy of
            //the source text to fill the hole, as that would be inappropriate in Reviewing
            //mode; since m_targetPhrase already has the box text or the copied source
            //text, we must instead check the CSourcePhrase instance explicitly to see if
            //m_adaption is empty, and if so, then we force the phrase box to remain empty
            //by clearing m_targetPhrase (later, when the box is moved to the next
            //location, we must check again in MakeTargetStringIncludingPunctuation() and
            //restore the earlier state when the phrase box is moved on)

			CSourcePhrase* pSPhr = pCell->GetPile()->GetSrcPhrase();
			wxASSERT(pSPhr != NULL);

			// get an adjusted pile pointer for the new active location, and we want the
			// pile's strip to be marked as invalid and the strip index added to the
			// CLayout::m_invalidStripArray
			pApp->m_nActiveSequNum = pApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber;
			pLayout->m_pDoc->ResetPartnerPileWidth(pSPhr);
			
			// BEW 19Oct15 No transition of vert edit modes,
			// so we can store this location on the app, provided
			// we are in bounds
			if (gbVerticalEditInProgress)
			{
				if (gEditRecord.nAdaptationStep_StartingSequNum <= pApp->m_nActiveSequNum &&
					gEditRecord.nAdaptationStep_EndingSequNum >= pApp->m_nActiveSequNum)
				{
					// BEW 19Oct15, store new active loc'n on app
					gpApp->m_vertEdit_LastActiveSequNum = pApp->m_nActiveSequNum;
#if defined(_DEBUG)
				wxLogDebug(_T("VertEdit PhrBox, JumpForward() storing loc'n: %d "), pApp->m_nActiveSequNum);
#endif
				}
			}
			
			if (pSPhr->m_targetStr.IsEmpty() || pSPhr->m_adaption.IsEmpty())
			{
				// no text or punctuation, or no text and punctuation not yet placed,
				// or no text and punctuation was earlier placed -- whichever is the case
				// we need to preserve that state
				pApp->m_targetPhrase.Empty();
				m_bSavedTargetStringWithPunctInReviewingMode = TRUE; // it gets cleared again
						// within MakeTargetStringIncludingPunctuation() at the end the block
						// it is being used in there
				m_StrSavedTargetStringWithPunctInReviewingMode = pSPhr->m_targetStr; // cleared
						// again within MakeTargetStringIncludingPunctuation() at the end of
						// the block it is being used in there
			}
			// if neither test succeeds, then let
			// m_targetPhrase contents stand unchanged

			pLayout->m_docEditOperationType = relocate_box_op;
		} // end of block for bSuccessful == TRUE

		// save the phrase box's text, in case user hits SHIFT+END to unmerge a phrase
        m_SaveTargetPhrase = pApp->m_targetPhrase;

#ifdef _NEW_LAYOUT
		#ifdef _FIND_DELAY
			wxLogDebug(_T("10. Start of RecalcLayout in JumpForward"));
		#endif
		pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
		#ifdef _FIND_DELAY
			wxLogDebug(_T("11. End of RecalcLayout in JumpForward"));
		#endif
#else
		pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
		pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
		wxASSERT(pApp->m_pActivePile != NULL);

		pLayout->m_pCanvas->ScrollIntoView(pApp->m_nActiveSequNum);

		pView->Invalidate();
		pLayout->PlaceBox();
	} // end Review mode (single src phrase move) block
	#ifdef _FIND_DELAY
		wxLogDebug(_T("12. End of JumpForward"));
	#endif
}

// This function is called for every character typed in phrase box (via OnChar() function 
// which is called for every EVT_TEXT posted to event queue); it is only called directly
// elsewhere in the app within OnEditUndo().
// BEW 13Apr10, no changes needed for support of doc version 5
// BEW 1Sep21 NOTE, this function is called for every keystroke (including backspace key)
// and it's the place where we must detect and do any box widening or contracting when
// typing or deleting indicates such a change it needed, and calculates the new box with
// when that happens, to pass to ResizeBox(), and an appropriate new value for the
// phrasebox gap width at the active location in the layout.
// whm 11Nov2022 refactored the phrasebox resizing routines, and cleaned out previous
// phrasebox resizing code.
void CPhraseBox::OnPhraseBoxChanged(wxCommandEvent& WXUNUSED(event))
{
	// whm Note: This phrasebox handler is necessary in the wxWidgets version because the
	// OnChar() handler does not have access to the changed value of the new string
	// within the control reflecting the keystroke that triggers OnChar(). Because
	// of that difference in behavior, I moved the code dependent on updating
	// pApp->m_targetPhrase from OnChar() to this OnPhraseBoxChanged() handler.
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	CAdapt_ItView* pView = (CAdapt_ItView*)pApp->GetView();
	CLayout* pLayout = pApp->GetLayout();

	// whm 3Nov2022 repurposed m_bAmWithinPhraseBoxChanged
	// This bool, while TRUE, is used to supress a call to 
	// SetFocusAndSetSelectionAtLanding() within the PlaceBox() call.
	pLayout->m_bAmWithinPhraseBoxChanged = TRUE;

	// This function is called at every pertinent keystroke, including backspaces:  OnChar() gets most keystrokes
	if (this->IsModified()) //if (this->GetTextCtrl()->IsModified()) // whm 14Feb2018 added GetTextCtrl()-> for IsModified()
	{
		// preserve cursor location, in case we merge, so we can restore it afterwards
		long nStartChar;
		long nEndChar;
		GetSelection(&nStartChar, &nEndChar); //GetTextCtrl()->GetSelection(&nStartChar, &nEndChar);

		wxPoint ptNew;
		wxRect rectClient;
		wxString thePhrase; // moved to OnPhraseBoxChanged()

		// update status bar with project name
		pApp->RefreshStatusBarInfo();

		// restore the cursor position...
		// BEW added 05Oct06; to support the alternative algorithm for setting up the
		// phrase box text, and positioning the cursor, when the selection was done
		// leftwards from the active location... that is, since the cursor position for
		// leftwards selection merges is determined within OnButtonMerge() then we don't
		// here want to let the values stored at the start of OnChar() clobber what
		// OnButtonMerge() has already done - so we have a test to determine when to
		// suppress the cursor setting call below in this new circumstance
		if (!(pApp->m_bMergeSucceeded && pApp->m_curDirection == toleft))
		{
			// whm 3Aug2018 Note: No adjustment made in SetSelection() call below.
			this->GetTextCtrl()->SetSelection(nStartChar, nEndChar);
			pApp->m_nStartChar = nStartChar;
			pApp->m_nEndChar = nEndChar;
		}

		// whm Note: Because of differences in the handling of events, in wxWidgets the
		// GetValue() call below retrieves the contents of the phrasebox after the
		// keystroke and so it includes the keyed character. OnChar() is processed before
		// OnPhraseBoxChanged(), and in that handler the key typed is not accessible.
		// Getting it here, therefore, is the only way to get it after the character has
		// been added to the box. This is in contrast to the MFC version where
		// GetWindowText(thePhrase) at the same code location in PhraseBox::OnChar there
		// gets the contents of the phrasebox including the just typed character.
		thePhrase = this->GetTextCtrl()->GetValue(); // current box text (including the character just typed)

		// BEW 6Jul09, try moving the auto-caps code from OnIdle() to here
		if (gbAutoCaps && pApp->m_pActivePile != NULL)
		{
			wxString str;
			CSourcePhrase* pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
			wxASSERT(pSrcPhrase != NULL);

			// get the string currently in the phrasebox
			// BEW 15/Sep21 moved this up from within the text of gbSourceIsUpperCase
			// because if the latter boolean is false, str does not get set to the
			// edited value of the box's text, e.g. when backspace is typed - str would
			// reflect the unedited box's string, which would be an error
			str = thePhrase;

			bool bNoError = pApp->GetDocument()->SetCaseParameters(pSrcPhrase->m_key);
			if (bNoError && gbSourceIsUpperCase)
			{
				// a change of case might be called for... first
				// make sure the box exists and is visible before proceeding further
				if (pApp->m_pTargetBox != NULL && (pApp->m_pTargetBox->IsShown()))
				{
					// get the string currently in the phrasebox
					//str = pApp->m_pTargetBox->GetValue();
					//str = thePhrase; // BEW 15Sep21 moved this up to line 3666 - see comment there

					// do the case adjustment only after the first character has been
					// typed, and be sure to replace the cursor afterwards
					int len = str.Length();
					if (len != 1)
					{
						; // don't do anything to first character if string has more than 1 char
					}
					else
					{
						// set cursor offsets
						int nStart = 1; int nEnd = 1;

						// check out its case status
						bNoError = pApp->GetDocument()->SetCaseParameters(str, FALSE); // FALSE is value for bIsSrcText

						// change to upper case if required
						if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
						{
							str.SetChar(0, gcharNonSrcUC);
							this->GetTextCtrl()->ChangeValue(str);
							pApp->m_pTargetBox->Refresh();
							//pApp->m_targetPhrase = str;
							thePhrase = str;

							// fix the cursor location
							// whm 3Aug2018 Note: No adjustment made in SetSelection() call below.
							this->GetTextCtrl()->SetSelection(nStart, nEnd);
						}
					}
				}
			}
		} // end of TRUE block for test: if (gbAutoCaps && pApp->m_pActivePile != NULL)

		// update m_targetPhrase to agree with what has been typed so far (and backspaces
		// or selecting and deleting may have shortened it - don't assume it's wider)
		pApp->m_targetPhrase = thePhrase;

		// whm Note: here we can eliminate the test for Return, BackSpace and Tab
		pApp->m_bUserTypedSomething = TRUE;
		pView->GetDocument()->Modify(TRUE);
		pApp->m_pTargetBox->m_bAbandonable = FALSE; // once we type something, it's not
												// considered abandonable
		m_bBoxTextByCopyOnly = FALSE; // even if copied, typing something makes it different so set
							// this flag FALSE
		this->MarkDirty(); 

		wxPoint ptCurBoxLocation;
		CCell* pActiveCell = pApp->m_pActivePile->GetCell(1);
		pActiveCell->TopLeft(ptCurBoxLocation); // returns the .x and .y values in the signature's ref variable
					// BEW 25Dec14, cells are 2 pixels larger vertically as of today, so move TopLeft of box
					// back up by 2 pixels, so text baseline keeps aligned
		ptCurBoxLocation.y -= 2;

		// whm 11Nov2022 Notes: The CPile::CalcPhraseBoxListWidth() call below returns 
		// the maximum text extent of the dropdown list's visible entries. It does NOT 
		// include any button width, nor slop nor any gap. 
		// When the phrasebox and dropdown list are drawn on-screen, the width of the
		// dropdown list and width of the phrasebox + button + 1 are forced to be the 
		// same width.
		int listWidth = pApp->m_pActivePile->CalcPhraseBoxListWidth();
		// BEW 14Sep21, send the value to Layout's public member, m_curListWidth
		pLayout->m_curListWidth = listWidth; // other functions can grab it from there

		// whm 11Nov2022 modified to use a direct comparison between the 
		// initialPhraseBoxContentsOnLanding and the new content of the phrase box 
		// content.
		//
		// The Pile's CalcPhraseBoxWidth() gets the width of phrasebox for the current 
		// content and any list widths. 
		// The CalcPhraseBoxWidth() guarantees that the width of the phrasebox 
		// (excluding buttonWidth, & the + 1 for spacing the button), agrees in width 
		// with the dropdown list's width - and the latter is called by a 
		// calc function too which internally widens the list to make the widest 
		// list member wholely visible in the list, if the list exists. 
		// Adding the button, etc, gets done in ResizeBox(). 
		// The CalPhraseBoxWidth() function also includes the slop at the right end 
		// of the box.
		// The returned value from CalcPhraseBoxWidth() becomes a minimum width and 
		// we must not allow any ResizeBox() operation to make the phrasebox smaller 
		// than that value. 
		// 
		// Note: Our initialPixelTabPositionOnLanding represents the reference point 
		// pixel tab at landing.
		// The initialPixelTabPositionOnLanding value was already set in the CLayout's
		// PlaceBox() call before this OnPhraseBoxChanged() got triggered by the 
		// current user edit.
		// 
		// Get the pixel extent of the current string resulting from this current edit.
		boxContentPixelExtentAtThisEdit = GetTextExtentWidth(pApp->m_pTargetBox->GetTextCtrl()->GetValue());
		int bestTabForResize = -1; // initialize to no resize is needed
		// Note: GetBestTabSettingFromArrayForBoxResize() call below returns a value for
		// bestTabForResize, in which bestTabForResize is to be the new phrase box's 
		// actual width (including slop), but not including the button + 1, nor the gap
		// following the newly sized phrasebox.
		bestTabForResize = GetBestTabSettingFromArrayForBoxResize(oldPhraseBoxWidthCached, boxContentPixelExtentAtThisEdit);

		if (bestTabForResize == -1)
		{
			// No resize of phrasebox needed
			int halt_here = 1;
			wxUnusedVar(halt_here);
			;
		}
		else // bestTabForResize is not -1, so a resize of the phrasebox IS NEEDED
		{
			// The GetBestTabSettingFromArrayForBoxResize() function came up with a pixel 
			// tab position that is a new bestTabForResize.
			// whm note: The oldPhraseBoxWidthCached should only be assigned a value just 
			// before the pLayout->m_curBoxWidth value gets changed by the bestTabForResize 
			// value below.
			oldPhraseBoxWidthCached = pLayout->m_curBoxWidth;

			// First, assign the just-determined bestTabForResize to the Layout's 
			// m_curBoxWidth, since the bestTabForResize will be the new phrasebox size. 
			// A value from CalcPhraseBoxWidth() cannot be used here because it returns 
			// a smaller instantaneous width value based on all the text extents + slop 
			// as they exist at this instant in OnPhraseBoxChanged(). The new size for 
			// the phrasebox bestTabForResize was determined above as the new phrasebox 
			// width, and is generally the next tab width available from the 
			// arrayTabPositionsInPixels array obtained from the 
			// GetBestTabSettingFromArrayForBoxResize() call above; the bestTabForResize 
			// will be a larger value than the value that CalcPhraseBoxGapWidth() would 
			// return, as that larger value allows for extra editing whitespace after the 
			// phrasebox is resized, so as not to be changing the phrasebox width after 
			// every character is typed.
			pLayout->m_curBoxWidth = bestTabForResize; 

			// Make list + button width agree with the box width.
			// The CStrip::CreateStrip() function and its override also guarantee that 
			// the m_cruListWidth and the m_curBoxWidth agree.
			pLayout->m_curListWidth = bestTabForResize;
			
			// whm 11Nov2022 Notes:
			// The phrasebox's new size may be quite different from what it was before this edit,
			// so a recalculation of the strip/pile layout and refresh of the screen layout may
			// be required. 
			// We do not need to invoke RecalcLayout(), nor Invalidate(), nor ResizeBox() here. 
			// All we need do here is just call the CMainFrame::OnSize() function which 
			// simplifies things and still makes the phrasebox behave when editing causes it to 
			// flow up to previous strip or down to the following strip.
			// Note: SendSizeEvent() invokes the main frame's OnSize() handler - which likely
			// executes in the next available tick of idle time. The frame's OnSize() in turn
			// calls RecalcLayout(..., create_strips_keep_piles), Invalidate(), and PlaceBox(),
			// and PlaceBox() internally calls ResizeBox().
			pApp->GetMainFrame()->SendSizeEvent();
			// whm 11Nov2022 Note: Not quite sure why - it could be a idle timing issue - but
			// to get the layout finalized in its near perfect position with exact phrasebox width 
			// and the pile layout in final positions, we need a second call here of SendSizeEvent()
			pApp->GetMainFrame()->SendSizeEvent();
			
			// whm 15Dec added. On Linux at this point after a phrasebox resize, the insertion
			// point moves to position 0 which is undesirable. To prevent this on Linux I'm adding 
			// a conditional compiled call to SetFocusAndSetSelectionAtLanding() here.
			// whm 25Jan2023 added. Fix the cursor location, so it doesn't jump to end of phrasebox text
			// when the box size changes, especially when backspacing in middle of a text.
			// Note: We need to update the App's m_nStartChar and m_nEndChar global members too!
			// The other change that makes the cursor behave during editing is made within the
			// SetFocusAndSetSelectionAtLanding() function where the SetSelection(len,len) calls
			// there are suppressed while OnPhraseBoxChanged() is executing.
			this->GetTextCtrl()->SetSelection(nStartChar, nEndChar);
			pApp->m_nStartChar = nStartChar;
			pApp->m_nEndChar = nEndChar;

//# if defined __WXGTK__
			//SetFocusAndSetSelectionAtLanding();
//#endif
		}
			
		// set the globals for the cursor location, ie. m_nStartChar & m_nEndChar,
		// ready for box display
		GetSelection(&pApp->m_nStartChar, &pApp->m_nEndChar);  //GetTextCtrl()->GetSelection(&pApp->m_nStartChar, &pApp->m_nEndChar);

		// save the phrase box's text, in case user hits SHIFT+END to unmerge a phrase
		m_SaveTargetPhrase = pApp->m_targetPhrase;

		GetLayout()->m_docEditOperationType = no_edit_op;

		// BEW 02May2016, thePhrase gets leaked, so clear it here
		thePhrase.clear();
	
		// whm 11Nov2022 added. Cache the old content extent value here at end of 
		// OnPhraseBoxChanged(), so they will be ready for use during the next edit 
		// action that invokes OnPhraseBoxChanged().
		// These cached values are mainly for calculating the spaceRemainingInBoxAtThisEdit 
		// value, and the contentStringIsExpanding bool within the 
		// GetBestTabSettingFromArrayForBoxResize() helper function that is
		// called earlier in OnPhraseBoxChanged() where the spaceRemainingInBoxAtThisEdit 
		// is used to determine if there is too much white space after the newly edited 
		// content requiring a box contraction from its previous width at last edit.
		// These cached values are initialized in the Layout's PlaceBox() function.
		// 
		// Cache the oldBoxContentPixelExtendCached here at end of OnPhraseBoxChanged()
		oldBoxContentPixelExtentCached = boxContentPixelExtentAtThisEdit;

	} // end of TRUE block for test: if (this->IsModified())

	// whm 3Nov2022 repurposed m_bAmWithinPhraseBoxChanged
	// This bool, while TRUE, was used to supress a call to 
	// SetFocusAndSetSelectionAtLanding() within the PlaceBox() call.
	// 
	// whm 11Nov2022 update. The m_bAmWithinPhraseBoxChanged flag is technically
	// no longer needed since we don't call PlaceBox() from here in
	// OnPhraseBoxChanged, however, it doesn't hurt to leave that
	// protection from future modifications that might end up calling
	// PlaceBox().
	pLayout->m_bAmWithinPhraseBoxChanged = FALSE; 

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
// Hence the calling order in MFC is OnKeyDown, OnChar, (OnPhraseBoxChanged - if the
// box contents got changed - at if so, altering the box width and gap width in
// the active strip may have happend) and lastly, OnKeyUp.
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
// BEW 13Apr10, no changes needed for support of doc version 5
// BEW 22Jun10, no changes needed for support of kbVersion 2
//

// whm 1Jun2018 Notes: OnChar() currently does the following key handling:
// 1. Detects if key event (wxKeyEvent) is modified with CTRL down, or ALT down, or any
//    arrow key: WXK_DOWN, WXK_UP, WXK_LEFT, or WXK_RIGHT. If so, event.Skip() is called 
//    so that OnKeyUp() will handle those special cases. Then return is called to
//    exit immediately from OnChar() before processing the list items below.
// 2. Set pApp->m_nPlacePunctDlgCallNumber = 0 initializing it on every alphanumeric key stroke.
// 3. Handle WXK_BACK key event - to effect an Undo for a backspace deletion of a selection or
//    a single char.
// 4. Call event.Skip() if the key event is NOT WXK_RETURN or WXK_TAB - to prevent bell sounding (?)
// 5. Automatically effect a merger if a selection is present - assuming the user intended to do so
//    before an OnChar() event.
// 6. Note: The WXX_TAB, WXK_NUMPAD_ENTER and WXK_RETURN keys are all now processed alike in OnKeyUp().
// 7. (BEW 31Aug21, legacy comment: Handle WXK_BACK key event - to effect box resizing on backspace.)
//    #7. now does NOT, by itself, effect box resizing. The reason for this is that the user, while
//    steadyAsSheGoes is the boxMode, may be doing editing - adding characters or removing some, or
//    both, while working within the acceptable limits provided by the slop. And every char added or
//    removed or selection replaced by a character, etc, will cause an event to fire, for which
//    OnPhraseBoxChanged( event ) is its handler. And so we must allow backspaces, delete key, adding
//    a new character, etc, to all be done - until a change is done to the phrasebox that triggers a
//    phrasebox resize - and that must go hand in hand with a calculation for an appropriately widened
//    or contracted phrasebox gap within the active strip - and for that to happen, the boxMode must be 
//    briefly changed to expanding or contracting (grabbing the value from a cache member of Layout) at 
//    the relevant instant - which in turn demands a suitable function for determining need for a width
//    alteration. That stuff is done in OnPhraseBoxChanged().

// BEW 17Sep21, OnChar needs some tweeking. Box width changes are now the job for
// OnPhraseBoxChanged(). An important task in OnChar, when control upstream is within 
// OnPhraseBoxChanged, is to respond to a backspace key press to remove the previous
// // character, or delete a selection in the phrasebox. Reentrancy has to be guarded
// against in that circumstance. This is necessary because otherwise, the change of the
// text by the character removal will trigger embedding OnPhraseBoxChanged() once more, 
// which will result in another call of ResizeBox(), and the phrasebox will expand further
// than it should, but the location of the surrounding piles won't change, with the result
// that the pile following the active location will be partly or wholely overlaid visibly 
// in the GUI by the right end of the box. 
// To help in this, Layout caches a boolean m_bAmWithinPhraseBoxChange, set on entry to
// OnPhraseBoxChanged(), and cleared to FALSE on exit. So nested calls to ResizeBox() while 
// that bool is TRUE, will do no more than just remove the backspace-deleted
// character from the phrasebox.
void CPhraseBox::OnChar(wxKeyEvent& event)
{
	// whm Note: OnChar() is called before OnPhraseBoxChanged()
//    wxLogDebug(_T("In CPhraseBox::OnChar() key code: %d"),event.GetKeyCode());

	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CLayout* pLayout = GetLayout();
	CAdapt_ItView* pView = pLayout->m_pView;
	wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));
	//CPhraseBox* pBox = pApp->m_pTargetBox;
	//wxASSERT(pBox != NULL);

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
	pApp->m_nPlacePunctDlgCallNumber = 0; // initialize on every keystroke

	pApp->m_bMergeSucceeded = FALSE; //bool bMergeWasDone = FALSE;

	// whm Note: The following code for handling the WXK_BACK key is ok to leave here in
	// the OnChar() handler, because it is placed before the Skip() call (the OnChar() base
	// class call in MFC)  
	GetSelection(&m_nSaveStart, &m_nSaveEnd);

    // BEW 1Sep21 Mostly OnChar will register keypresses for typed adaptation (or gloss)
	// characters, but a Backspace is not something easily tested for in OnPhraseBoxChanged().
	// The RecalcLayout() call in OnPhraseBoxChanged() needs to know whether or not to re-create
	// the strips - as that's where a larger gap width gets to modify the location of the piles
	// which follow the active location. We must set a new phrasebox width in OnPhraseBoxChanged(), 
	// which then gets passed as a constant int to ResizeBox. 
	// So all the calcs for box expansion or contraction must happen in OnPhraseBoxChanged() 
	// before the new or same boxWidth is passed to ResizeBox(). The resize, of course, may be 
	// to make the box wider, or less wide, but never less wide than the width of the
	// dropdown list. 
	// These protocols are implemented in OnPhraseBoxChanged(). If m_bJustKeyedBackspace is FALSE, 
	// then no backspace was typed, and so the only possibilities are that there is no expansion 
	// needed, or the text has lengthened to overlap the right boundary (3 'w' widths) before the 
	// box's end. In the latter curcumstance, there MUST be an expansion of the box, because we
	// cannot assume that the user has finished adding extra width to the adaptation. 
	// But if m_bJustKeyedBackspace is TRUE, then it is possible that the reduction of the 
	// adaptation's length might have succeeded in shortening it to the point that a reduction
	// of slops number of 'w' widths (the slop value) is possible - but only 
	// provided such a shortening does not make the adaptation text's extent.x become less than the
	// width of the dropdown list.
	// 
	// whm 11Nov2022 BEW's comments above are accurate, except that we no longer need to track
	// the App global m_bJustKeyedBackspace. Also we no longer track text removals done by the
	// backspace key in order to be able to undo such removals. But, rather provide the ESC key
	// as a way to undo all edits since phrasebox landing, replacing the phrasebox contents with
	// the contents the phrasebox had originally at landing.

    // MFC Note: CEdit's Undo() function does not undo a backspace deletion of a selection
    // or single char, so implement that here & in an override for OnEditUndo();
	if (event.GetKeyCode() == WXK_BACK)
	{
		wxString s;
		s = GetValue(); // returns wxString, from call of wxTextEntry::DoGetValue()
		if (m_nSaveStart == m_nSaveEnd)
		{
			// deleting previous character at the cursor location in the box
			if (m_nSaveStart > 0)
			{
				int index = m_nSaveStart;
				index--;
				m_nSaveStart = index;
				m_nSaveEnd = index;
				wxChar ch = s.GetChar(index); // ch is the deleted character
				m_backspaceUndoStr += ch; // if undoing, this puts it back in
			}
			else // m_nSaveStart must be zero, as when the box is empty
			{
 				if (pApp->m_pActivePile->GetSrcPhrase()->m_adaption.IsEmpty() &&
					((pApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry && !gbIsGlossing) ||
					(pApp->m_pActivePile->GetSrcPhrase()->m_bHasGlossingKBEntry && gbIsGlossing)))
				{
					m_bNoAdaptationRemovalRequested = TRUE;
					CSourcePhrase* pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
					wxString emptyStr = _T("");
					pApp->m_pKB->GetAndRemoveRefString(pSrcPhrase,emptyStr,useGlossOrAdaptationForLookup);
				}
			}
		}
		else
		{
			// deleting a selection
			int count = m_nSaveEnd - m_nSaveStart;
			if (count > 0)
			{
				m_backspaceUndoStr = s.Mid(m_nSaveStart,count);
				m_nSaveEnd = m_nSaveStart;
			}
		}
	} // end of TRUE block for test: if (event.GetKeyCode() == WXK_BACK)

    // wxWidgets Note: The wxTextCtrl does not have a virtual OnChar() method,
	// so we'll just call .Skip() for any special handling of the WXK_RETURN and WXK_TAB
	// key events. In wxWidgets, calling event.Skip() is analagous to calling
	// the base class version of a virtual function. Note: wxTextCtrl has
	// a non-virtual OnChar() method. See "wxTextCtrl OnChar event handling.txt"
	// for a newsgroup sample describing how to use OnChar() to do "auto-
	// completion" while a user types text into a text ctrl.
	if (!(event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_TAB)) // Code above
		// lets OnKeyDown() or OnKeyUp() handle these keypresses; so all we do here is
		// call event.Skip() which gets the other characters handled; so that
		// on return what follows this TRUE block gets done (i.e. backspace key, 
		// automerging if necessary, etc)
	{

		// whm 21Aug2021 Note: Here in CPhraseBox::OnChar() we implement the auto-correct feature. 
		// There are two more tests that follow this block: (1) Checking for a keypress to 
		// trigger an auto-merge when there is selected source text at the current location, 
		// and (2) Checking a second time for BackSpace keypress.
		// NOTE: The above if() test eliminates Enter and Tab key codes from this test block
		// but it is possible that other keys, especially control keys may get through into
		// this test block. Therefore the AutoCorrected() function called below uses the 
		// GetUnicodeKey() function as a further test to make sure that we are using only 
		// alphanumeric key values in our auto-correct feature. 

        // whm note: Instead of explicitly calling the OnChar() function in the base class
        // (as would normally be done for C++ virtual functions), in wxWidgets, we call
        // Skip() instead, for the event to be processed either in the base wxWidgets class
        // or the native control. This difference in event handling also means that part of
        // the code in OnChar() that can be handled correctly in the MFC version, must be
        // moved to our wx-specific OnPhraseBoxChanged() handler. This is necessitated
        // because GetValue() does not get the new value of the control's contents when
        // called from within OnChar, but received the previous value of the string as it
        // existed before the keystroke that triggers OnChar.
		// 
		// BEW 13Aug18, the above comment is correct, but it really doesn't matter if
		// there is a character pending for insertion to the phrasebox and we do our
		// tests without it yet being in the box - why? because the slop can absorb it
		// safely - all we must do is (in OnPhraseBoxChanged()) detect when the current 
		// width of the box is about to go beyond the boundary we set for an 'expanding'
		// scenario. That boundary is 3 'w' widths from the end of the box's rectangle - 
		// enough to absorb a character in any language. OnPhraseBoxChanged() will grab 
		// the pending character and add it to the phrasebox, so no need to do it here
		// 
		// whm 31Aug2021 modified to consolidate all the auto-correct modifications of
		// a target text text control into a single App bool function AutoCorrected() which 
		// returns its value to the bSkip boolean test below. Then AutoCorrected() can 
		// be used in all dialogs' OnChar() methods to do Auto Correct in each of those 
		// dialog's target text edit boxes.
		// 
		// whm 4May2022 modified to set some booleans for extra clarity and to be able to
		// inform the OnPhraseBoxChanged() method if the current character typed triggered
		// an autocorrect action on the phrasebox content.
		// We don't want event.Skip() called here in OnChar() after an autocorrect change has
		// been made to the phrasebox content, but we do want to call event.Skip() if NO
		// autocorrect action has taken place.
		bool bAutoCorrected = FALSE;
		bAutoCorrected = pApp->AutoCorrected((CPhraseBox*)this, &event);
		if (!bAutoCorrected)
		{
			event.Skip();
		}
		else
		{
			// whm 4May2022 added
			// An autocorrect action took place, now update the App's pApp->m_targetPhrase member
			// so that downstream processing will have the correct entry for any subsequent 
			// phrasebox move/relocation.
			// Testing indicates that assigning current value of phrasebox to the following global
			// variables on the App including: 
			//   pApp->m_targetPhrase
			//   pApp->m_pTargetBox->m_SaveTargetPhrase
			// fixes the issue Bruce noted of wrongly getting a semicolon (the dead-key stroke) affixed 
			// or infixed into the target when moving the phrasebox to another location after the 
			// AutoCorrected() action has taken place within the phrasebox.
			pApp->m_targetPhrase = pApp->m_pTargetBox->GetTextCtrl()->GetValue();
			pApp->m_pTargetBox->m_SaveTargetPhrase = pApp->m_pTargetBox->GetTextCtrl()->GetValue();

#if defined (_DEBUG)
			wxLogDebug("AutoCorrected took place In OnChar()");
			if (!IsModified())
			{
				wxLogDebug("Phrasebox NOT marked dirty, marking it dirty");
				this->MarkDirty(); // "mark text as modified (dirty)"
				if (!IsModified())
				{
					wxLogDebug("FAILED to mark phrasebox as dirty with this->MarkDirty() call!!");
				}
			}
			else
			{
				wxLogDebug("Phrasebox IS currently marked dirty, taking no action to mark it dirty");
			}
#endif
		}
	} // end of if (!(event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_TAB)) 

	// preserve cursor location, in case we merge, so we can restore it afterwards
	long nStartChar;
	long nEndChar;
    GetSelection(&nStartChar, &nEndChar); //GetTextCtrl()->GetSelection(&nStartChar, &nEndChar);

    // whm Note: See note above about needing to move some code from OnChar() to the
    // OnPhraseBoxChanged() handler in the wx version, because the OnChar() handler does
    // not have access to the changed value of the new string within the control reflecting
    // the keystroke that triggers OnChar().
	//
	wxSize textExtent;
	pApp->RefreshStatusBarInfo();

	// if there is a selection, and user forgets to make the phrase before typing, then do it
	// for him on the first character typed. But if glossing, no mergers are allowed.

	int theCount = pApp->m_selection.GetCount();
	if (!gbIsGlossing && theCount > 1 && (pApp->m_pActivePile == pApp->m_pAnchor->GetPile()
		|| IsActiveLocWithinSelection(pView, pApp->m_pActivePile)))
	{
		if (pView->GetSelectionWordCount() > MAX_WORDS)
		{
			pApp->GetRetranslation()->DoRetranslation();
		}
		else
		{
			if (!pApp->m_bUserTypedSomething &&
				!pApp->m_pActivePile->GetSrcPhrase()->m_targetStr.IsEmpty())
			{
				pApp->m_bSuppressDefaultAdaptation = FALSE; // we want what is already there
			}
			else
			{
				// for version 1.4.2 and onwards, we will want to check m_bRetainBoxContents
				// and two other flags, for a click or arrow key press is meant to allow
				// the deselected source word already in the phrasebox to be retained; so we
				// here want the m_bSuppressDefaultAdaptation flag set TRUE only when the
				// m_bRetainBoxContents is FALSE (- though we use two other flags too to
				// ensure we get this behaviour only when we want it)
				if (m_bRetainBoxContents && !m_bAbandonable && pApp->m_bUserTypedSomething)
				{
                    pApp->m_bSuppressDefaultAdaptation = FALSE;
				}
				else
				{
                    pApp->m_bSuppressDefaultAdaptation = TRUE; // the global boolean used for temporary
													   // suppression only
				}
			}
			pView->MergeWords(); // simply calls OnButtonMerge 
			pApp->m_bMergeSucceeded = TRUE;
			pLayout->m_docEditOperationType = merge_op;
            pApp->m_bSuppressDefaultAdaptation = FALSE;

			// we can assume what the user typed, provided it is a letter, replaces what was
			// merged together, but if tab or return was typed, we allow the merged text to
			// remain intact & pass the character on to the switch below; but since v1.4.2 we
			// can only assume this when m_bRetainBoxContents is FALSE, if it is TRUE and
			// a merge was done, then there is probably more than what was just typed, so we
			// retain that instead; also, when we have just returned from a MergeWords( ) call in
			// which the phrasebox has been set up with correct text (such as previous target text
			// plus the last character typed for an extra merge when a selection was present, we
			// don't want this wiped out and have only the last character typed inserted instead,
			// so in OnButtonMerge( ) we test the phrasebox's string and if it has more than just
			// the last character typed, we assume we want to keep the other content - and so there
			// we also set m_bRetainBoxContents
            m_bRetainBoxContents = FALSE; // turn it back off (default) until next required
		}
	}
	else
	{
        // if there is a selection, but the anchor is removed from the active location, we
        // don't want to make a phrase elsewhere, so just remove the selection. Or if
        // glossing, just silently remove the selection - that should be sufficient alert
        // to the user that the merge operation is unavailable
		pView->RemoveSelection();
		wxClientDC dC(pLayout->m_pCanvas);
		pView->canvas->DoPrepareDC(dC); // adjust origin
		pApp->GetMainFrame()->PrepareDC(dC); // wxWidgets' drawing.cpp sample also calls
											 // PrepareDC on the owning frame
		pLayout->m_docEditOperationType = no_edit_op;
		pView->Invalidate();
	}

    // whm 5Aug2018 Note: We need to do some filtering for the GetUnicodeKey() event. For example, the
    // Backspace key returns a control key value of 8 ('\b'), which within UpdatePhraseBoxWidth_Expanding()
    // below the 8 value actually gets translated to a positive width of 12 (in inStrWidth) which lengthens
    // the string within the box rather than shorten it! Ditto for the Escape key value of 27 which within
    // UpdatePhraseBoxWidth() below the 27 value gets translated to a positive width of 12 (in inStrWidth),
    // lengthening the string. Using the example in the wxWidgets docs for wxKeyEvent as a guide (see:
    // http://docs.wxwidgets.org/3.1/classwx_key_event.html#a3dccc5a254770931e5d8066ef47e7fb0 )
    // I'm instituting a filter to eliminate control char codes from being processed by the 
    // UpdatePhraseBoxWidth_Expanding() call below.
    wxChar typedChar = event.GetUnicodeKey();
    
    if (typedChar != WXK_NONE)
    {
        // It's a "normal" character. Notice that this includes
        // control characters in 1..31 range, e.g. WXK_RETURN or
        // WXK_BACK, so check for them explicitly.
        if (typedChar >= 32)
        {
            //wxLogDebug(_T("In CPhraseBox::OnKeyUp() You pressed '%c' - key code: %d"), typedChar, event.GetKeyCode());
            pLayout->m_inputString = typedChar;
			
        } // end of TRUE block for test: if (typedChar >= 32)
        else
        {
            // It's a control character. So who cares as far as expanding or contracting is concerned?
            //wxLogDebug(_T("In CPhraseBox::OnKeyUp() You pressed control char '%c' - key code: %d"), typedChar, event.GetKeyCode());
            // Allow the control char to drop through to event.Skip() below - although this OnKeyUp() 
            // handler should be the last handler processing key events.
			// BEW 26Aug21 added next line //BEW 23Sep21 commented out the return, to allow the fall through to happen
			//return;
		}
    }
	else
    {
        wxLogDebug(_T("In CPhraseBox::OnKeyUp() GetUnicodeKey() returned  '%c' - WXK_NONE"), typedChar);
        // BEW 23Sep21 commented out the return, to let the Skip() fall thru to happen
		//return; // BEW added 26Aug21
    }
	
	long keycode = event.GetKeyCode();
	switch(keycode)
	{
	case WXK_BACK: //8:		// BackSpace key
		{
			
			//bool bWasMadeDirty = TRUE;
			//pLayout->m_docEditOperationType = no_edit_op; // legacy app used this choice
			pLayout->m_docEditOperationType = no_edit_op;
		}
	default:
	{
		;
	}
	}// don't remove this closing brace, it's the end of the switch: switch(keycode)
}

// returns TRUE if the move was successful, FALSE if not successful
// Ammended July 2003 for auto-capitalization support
// BEW 13Apr10, no changes needed for support of doc version 5
// BEW 21Jun10, no changes needed for support of kbVersion 2, & removed pView from signature
// BEW 17Jul11, changed for GetRefString() to return KB_Entry enum, and use all 10 maps
// for glossing KB
bool CPhraseBox::MoveToPrevPile(CPile *pCurPile)
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

	pApp->m_preGuesserStr.Empty(); // BEW 27Nov14, in case a src string, or modified string
		// is stored ready for user's Esc keypress to restore the pre-guesser
		// form, clear it, because the box is gunna move and we want it
		// restored to default empty ready for next box landing location
	// whm modified 29Mar12. Left mouse clicks now beep only when certain parts of
	// the canvas are clicked on, and allows other clicks to act normally (such as
	// the opening/closing of the ViewFilteredMaterial dialog and the Notes dialog).
	// store the current translation, if one exists, before retreating, since each retreat
    // unstores the refString's translation from the KB, so they must be put back each time
    // (perhaps in edited form, if user changed the string before moving back again)
	wxASSERT(pApp != NULL);
	//pApp->limiter = 0; // BEW added Aug13, to support OnIdle() hack for m_targetStr non-stick bug // bug fixed 24Sept13 BEW
	CAdapt_ItView *pView = pApp->GetView();
	CAdapt_ItDoc* pDoc = pView->GetDocument();
	m_bBoxTextByCopyOnly = FALSE; // restore default setting
	CLayout* pLayout = GetLayout();
	CSourcePhrase* pOldActiveSrcPhrase = pCurPile->GetSrcPhrase();

	// make sure m_targetPhrase doesn't have any final spaces either
	RemoveFinalSpaces(pApp->m_pTargetBox,&pApp->m_targetPhrase);

	// if we are at the start, we can't move back any further
	// - but check vertical edit situation first
	int nCurSequNum = pCurPile->GetSrcPhrase()->m_nSequNumber;

	// if vertical editing is in progress, don't permit a move backwards into the preceding
	// gray text area; just beep and return without doing anything
	if (gbVerticalEditInProgress)
	{
		EditRecord* pRec = &gEditRecord;
		if (gEditStep == adaptationsStep || gEditStep == glossesStep)
		{
			if (nCurSequNum <= pRec->nStartingSequNum)
			{
				// we are about to try to move back into the gray text area before the edit span, disallow
				::wxBell();
                pApp->m_pTargetBox->SetFocusAndSetSelectionAtLanding(); // whm 13Aug2018 modified
                pLayout->m_docEditOperationType = no_edit_op;
				return FALSE;
			}
		}
		else if (gEditStep == freeTranslationsStep)
		{
			if (nCurSequNum <= pRec->nFreeTrans_StartingSequNum)
			{
                // we are about to try to move back into the gray text area before the free
                // trans span, disallow (I don't think we can invoke this function from
                // within free translation mode, but no harm to play safe)
				::wxBell();
                pApp->m_pTargetBox->SetFocusAndSetSelectionAtLanding(); // whm 13Aug2018 modified
                pLayout->m_docEditOperationType = no_edit_op;
				return FALSE;
			}
		}
	}
	if (nCurSequNum == 0)
	{
		// IDS_CANNOT_GO_BACK
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        gpApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_(
"You are already at the start of the file, so it is not possible to move back any further."),
		_T(""), wxICON_INFORMATION | wxOK);
        pApp->m_pTargetBox->SetFocusAndSetSelectionAtLanding(); // whm 13Aug2018 modified
        pLayout->m_docEditOperationType = no_edit_op;
		return FALSE;
	}
	bool bOK;

	// don't move back if it means moving to a retranslation pile; but if we are
	// glossing it is okay to move back into a retranslated section
	{
		CPile* pPrev = pView->GetPrevPile(pCurPile);
		wxASSERT(pPrev);
		if (!gbIsGlossing && pPrev->GetSrcPhrase()->m_bRetranslation)
		{
			// IDS_NO_ACCESS_TO_RETRANS
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            gpApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(_(
"To edit or remove a retranslation you must use the toolbar buttons for those operations."),
			_T(""), wxICON_INFORMATION | wxOK);
            pApp->m_pTargetBox->SetFocusAndSetSelectionAtLanding(); // whm 13Aug2018 modified
            pLayout->m_docEditOperationType = no_edit_op;
			return FALSE;
		}
	}

    // if the location is a <Not In KB> one, we want to skip the store & fourth line
    // creation --- as of Dec 18, version 1.4.0, according to Susanna Imrie's
    // recommendation, I've changed this so it will allow a non-null adaptation to remain
    // at this location in the document, but just to suppress the KB store; if glossing is
    // ON, then being a <Not In KB> location is irrelevant, and we will want the store done
    // normally - but to the glossing KB of course
	//bool bNoError = TRUE;
	if (!gbIsGlossing && !pCurPile->GetSrcPhrase()->m_bHasKBEntry
												&& pCurPile->GetSrcPhrase()->m_bNotInKB)
	{
        // if the user edited out the <Not In KB> entry from the KB editor, we need to put
        // it back so that the setting is preserved (the "right" way to change the setting
        // is to use the toolbar checkbox - this applies when adapting, not glossing)
		pApp->m_pKB->Fix_NotInKB_WronglyEditedOut(pCurPile);
		goto b;
	}

	// if the box contents is null, then the source phrase must store an empty string
	// as appropriate - either m_adaption when adapting, or m_gloss when glossing
	if (pApp->m_targetPhrase.IsEmpty())
	{
		pApp->m_bForceAsk = FALSE; // make sure it's turned off, & allow function to continue
		if (gbIsGlossing)
			pCurPile->GetSrcPhrase()->m_gloss.Empty();
		else
			pCurPile->GetSrcPhrase()->m_adaption.Empty();
	}

    // make the punctuated target string, but only if adapting; note, for auto
    // capitalization ON, the function will change initial lower to upper as required,
    // whatever punctuation regime is in place for this particular sourcephrase instance...
    // we are about to leave the current phrase box location, so we must try to store what
    // is now in the box, if the relevant flags allow it. Test to determine which KB to
    // store to. StoreText( ) has been ammended for auto-capitalization support (July 2003)
	if (gbIsGlossing)
	{
		// BEW added 27May10, to not save contents if backing back from a halt
		// location, when there is no content on the CSourcePhrase instance already
		if (pCurPile->GetSrcPhrase()->m_gloss.IsEmpty())
		{
			bOK = TRUE;
		}
		else
		{
			bOK = pApp->m_pGlossingKB->StoreTextGoingBack(pCurPile->GetSrcPhrase(), pApp->m_targetPhrase);
		}
	}
	else // adapting
	{
		// BEW added 27May10, to not save contents if backing back from a halt
		// location, when there is no content on the CSourcePhrase instance already
		if (pCurPile->GetSrcPhrase()->m_adaption.IsEmpty())
		{
			bOK = TRUE;
		}
		else
		{
			pView->MakeTargetStringIncludingPunctuation(pCurPile->GetSrcPhrase(), pApp->m_targetPhrase);
			pView->RemovePunctuation(pDoc, &pApp->m_targetPhrase, from_target_text);
			pApp->m_bInhibitMakeTargetStringCall = TRUE;
			bOK = pApp->m_pKB->StoreTextGoingBack(pCurPile->GetSrcPhrase(), pApp->m_targetPhrase);
			pApp->m_bInhibitMakeTargetStringCall = FALSE;
		}
	}
	if (!bOK)
	{
        // here, MoveToNextPile() calls DoStore_NormalOrTransliterateModes(), but for
        // MoveToPrevPile() we will keep it simple and not try to get text for the phrase
        // box
		pLayout->m_docEditOperationType = no_edit_op;
		return FALSE; // can't move if the adaptation or gloss text is not yet completed
	}

	// move to previous pile's cell
b:	CPile* pNewPile = pView->GetPrevPile(pCurPile); // does not update the view's
				// m_nActiveSequNum nor m_pActivePile pointer, so update these here,
				// provided NULL was not returned

	// restore default button image, and m_bCopySourcePunctuation to TRUE
	wxCommandEvent event;
	if (!pApp->m_bCopySourcePunctuation)
	{
		pApp->GetView()->OnToggleEnablePunctuationCopy(event);
	}
	if (pNewPile == NULL)
	{
		// we deem vertical editing current step to have ended if control gets into this
		// block, so user has to be asked what to do next if vertical editing is currently
		// in progress; and we tunnel out before m_nActiveSequNum can be set to -1 (otherwise
		// vertical edit will crash when recalc layout is tried with a bad sequ num value)
		if (gbVerticalEditInProgress)
		{
			m_bTunnellingOut = FALSE; // ensure default value set
			bool bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(-1,
							nextStep, TRUE); // bForceTransition is TRUE
			if (bCommandPosted)
			{
				// don't proceed further because the current vertical edit step has ended
				m_bTunnellingOut = TRUE; // so caller can use it
				pLayout->m_docEditOperationType = no_edit_op;
				return FALSE;
			}
		}
		pLayout->m_docEditOperationType = no_edit_op;
		return FALSE; // we are at the start of the file too, so can't go further back
	}
	else
	{
		// the previous pile ptr is valid, so proceed

        // don't commit to the new pile if we are in vertical edit mode, until we've
        // checked the pile is not in the gray text area...
        // if vertical editing is currently in progress we must check if the lookup target
        // is within the editable span, if not then control has moved the box into the gray
        // area beyond the editable span and that means a step transition is warranted &
        // the user should be asked what step is next
		if (gbVerticalEditInProgress)
		{
			int m_nCurrentSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
			m_bTunnellingOut = FALSE; // ensure default value set
			bool bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(
									m_nCurrentSequNum,nextStep); // bForceTransition is FALSE
			if (bCommandPosted)
			{
				// don't proceed further because the current vertical edit step has ended
				m_bTunnellingOut = TRUE; // so caller can use it
				pLayout->m_docEditOperationType = no_edit_op;
				return FALSE; // try returning FALSE
			}
			else
			{
				// BEW 19Oct15 No transition of vert edit modes,
				// so we can store this location on the app
				gpApp->m_vertEdit_LastActiveSequNum = m_nCurrentSequNum;
#if defined(_DEBUG)
				wxLogDebug(_T("VertEdit PhrBox, MoveToPrevPile() storing loc'n: %d "), m_nCurrentSequNum);
#endif
			}
		}

		pApp->m_bUserTypedSomething = FALSE; // user has not typed at the new location yet

		// update the active sequence number, and pile pointer
		pApp->m_nActiveSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
		pApp->m_pActivePile = pNewPile;

        // get an adjusted pile pointer for the new active location, and we want the
        // pile's strip to be marked as invalid and the strip index added to the
        // CLayout::m_invalidStripArray
		pDoc->ResetPartnerPileWidth(pOldActiveSrcPhrase);

        // since we are moving back, the prev pile is likely to have a refString
        // translation which is nonempty, so we have to put it into m_targetPhrase so that
        // ResizeBox will use it; but if there is none, the copy the source key if the
        // m_bCopySource flag is set, else just set it to an empty string. (bNeed Modify is
        // a helper flag used for setting/clearing the document's modified flag at the end
        // of this function)
		bool bNeedModify = FALSE; // reset to TRUE if we copy source
								  // because there was no adaptation
		// be careful, the pointer might point to <Not In KB>, rather than a normal entry
		CRefString* pRefString = NULL;
		KB_Entry rsEntry;
		if (gbIsGlossing)
		{
			rsEntry = pApp->m_pGlossingKB->GetRefString(pNewPile->GetSrcPhrase()->m_nSrcWords,
					pNewPile->GetSrcPhrase()->m_key, pNewPile->GetSrcPhrase()->m_gloss, pRefString);
		}
		else
		{
			rsEntry = pApp->m_pKB->GetRefString(pNewPile->GetSrcPhrase()->m_nSrcWords,
					pNewPile->GetSrcPhrase()->m_key, pNewPile->GetSrcPhrase()->m_adaption, pRefString);
		}
		if (pRefString != NULL && rsEntry == really_present)
		{
			pView->RemoveSelection(); // we won't do merges in this situation

			// assign the translation text - but check it's not "<Not In KB>", if it is, we
			// leave the phrase box empty, turn OFF the m_bSaveToKB flag -- this is changed
			// for v1.4.0 and onwards because we will want to leave any adaptation already
			// present unchanged, rather than clear it and so we will not make it abandonable
			// either
			wxString str = pRefString->m_translation; // no case change to be done here since
								// all we want to do is remove the refString or decrease its
								// reference count
			if (!gbIsGlossing && str == _T("<Not In KB>"))
			{
				pApp->m_bSaveToKB = FALSE;
				pApp->m_pTargetBox->m_bAbandonable = FALSE; // used to be TRUE;
				pNewPile->GetSrcPhrase()->m_bHasKBEntry = FALSE; // ensures * shows above this
															   // srcPhrase
				pNewPile->GetSrcPhrase()->m_bNotInKB = TRUE;
			}

            // remove the translation from the KB, in case user wants to edit it before
            // it's stored again (RemoveRefString also clears the m_bHasKBEntry flag on the
            // source phrase, or m_bHasGlossingKBEntry if gbIsGlossing is TRUE)
			if (gbIsGlossing)
			{
				pApp->m_pGlossingKB->RemoveRefString(pRefString, pNewPile->GetSrcPhrase(),
											pNewPile->GetSrcPhrase()->m_nSrcWords);
				pApp->m_targetPhrase = pNewPile->GetSrcPhrase()->m_gloss;
			}
			else
			{
				pApp->m_pKB->RemoveRefString(pRefString, pNewPile->GetSrcPhrase(),
											pNewPile->GetSrcPhrase()->m_nSrcWords);
				// since we have optional punctuation hiding, use the line with
				// the punctuation
				pApp->m_targetPhrase = pNewPile->GetSrcPhrase()->m_targetStr;
			}
		}
		else // the pointer to refString was null (ie. no KB entry) or rsEntry was present_but_deleted
		{
			if (gbIsGlossing)  // ensure the flag below is false when there is no KB entry
				pNewPile->GetSrcPhrase()->m_bHasGlossingKBEntry = FALSE;
			else
				pNewPile->GetSrcPhrase()->m_bHasKBEntry = FALSE;

			// just use an empty string, or copy the sourcePhrase's key if
			// the m_bCopySource flag is set
			if (pApp->m_bCopySource)
			{
				// whether glossing or adapting, we don't want a null source phrase
				// to initiate a copy
				if (!pNewPile->GetSrcPhrase()->m_bNullSourcePhrase)
				{
					pApp->m_targetPhrase = pView->CopySourceKey(pNewPile->GetSrcPhrase(),
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

        // initialize the phrase box too, so it doesn't carry an old string to the next
        // pile's cell
        this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase); //SetWindowText(pApp->m_targetPhrase);

        // get an adjusted pile pointer for the new active location, and we want the
        // pile's strip to be marked as invalid and the strip index added to the
        // CLayout::m_invalidStripArray
		if (pNewPile != NULL)
		{
			pDoc->ResetPartnerPileWidth(pNewPile->GetSrcPhrase());
		}

        pApp->m_nActiveSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
		pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
#ifdef _NEW_LAYOUT
		pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
		pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif

		// make sure the new active pile's pointer is reset
		pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
		pLayout->m_docEditOperationType = relocate_box_op;

		// recreate the phraseBox using the stored information
		pApp->m_nStartChar = -1; pApp->m_nEndChar = -1;

		// fix the m_bSaveToKB flag, depending on whether or not srcPhrase is in kb
		if (!gbIsGlossing && !pApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry &&
										pApp->m_pActivePile->GetSrcPhrase()->m_bNotInKB)
		{
			pApp->m_bSaveToKB = FALSE;
			pApp->m_targetPhrase.Empty();
		}
		else
		{
			pApp->m_bSaveToKB = TRUE;
		}

		// update status bar with project name
		pApp->RefreshStatusBarInfo();

		// BEW note 30Mar09: later we may set clipping.... in the meantime
		// just invalidate the lot
		pView->Invalidate();
		pLayout->PlaceBox();

		// BEW 13Apr20, control goes thru here when TAB or Enter gets a move to next empty
		// pile done - and PlacePhraseBox() does not get called (nor Jump()), so we have
		// to check here for landing within a footnote, extended footnote, cross-marker or
		// extended cross-marker span - if so, the two placeholder insert buttons must
		// be disabled by the Update...() handler for those
		pApp->m_bDisablePlaceholderInsertionButtons = FALSE; // initialise to enabled buttons
		if (pApp->m_pActivePile != NULL)
		{
			CSourcePhrase* pSPhr = pApp->m_pActivePile->GetSrcPhrase();
			bool bProhibited = pDoc->IsWithinSpanProhibitingPlaceholderInsertion(pSPhr);
			pApp->m_bDisablePlaceholderInsertionButtons = bProhibited;
		}

		if (bNeedModify)
			SetModify(TRUE); // our own SetModify(); calls MarkDirty()
		else
			SetModify(FALSE); // our own SetModify(); calls DiscardEdits();

		return TRUE;
	}
}

// returns TRUE if the move was successful, FALSE if not successful
// Ammended, July 2003, for auto capitalization support
// BEW 13Apr10, no changes needed for support of doc version 5
// BEW 21Jun10, no changes needed for support of kbVersion 2, & removed pView from signature
// BEW 17Jul11, changed for GetRefString() to return KB_Entry enum, and use all 10 maps
// for glossing KB
bool CPhraseBox::MoveToImmedNextPile(CPile *pCurPile)
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

	pApp->m_preGuesserStr.Empty(); // BEW 27Nov14, in case a src string, or modified string
		// is stored ready for user's Esc keypress to restore the pre-guesser
		// form, clear it, because the box is gunna move and we want it
		// restored to default empty ready for next box landing location
	// whm modified 29Mar12. Left mouse clicks now beep only when certain parts of
	// the canvas are clicked on, and allows other clicks to act normally (such as
	// the opening/closing of the ViewFilteredMaterial dialog and the Notes dialog).
	// store the current translation, if one exists, before moving to next pile, since each move
	// unstores the refString's translation from the KB, so they must be put back each time
	// (perhaps in edited form, if user changed the string before moving back again)
	wxASSERT(pApp != NULL);
	//pApp->limiter = 0; // BEW added Aug13, to support OnIdle() hack for m_targetStr non-stick bug // bug fixed 24Sept13 BEW
	CAdapt_ItView *pView = pApp->GetView();
	CAdapt_ItDoc* pDoc = pView->GetDocument();
	m_bBoxTextByCopyOnly = FALSE; // restore default setting
	CSourcePhrase* pOldActiveSrcPhrase = pCurPile->GetSrcPhrase();
	bool bOK;

	// make sure m_targetPhrase doesn't have any final spaces
	RemoveFinalSpaces(pApp->m_pTargetBox, &pApp->m_targetPhrase);

	// BEW changed 25Oct09, altered syntax so it no longer exits here if pFwd
	// is NULL, otherwise the document-end typed meaning doesn't 'stick' in
	// the document
	CPile* pFwd = pView->GetNextPile(pCurPile);
	if (pFwd == NULL)
	{
		// no more piles, but continue so we can make the user's typed string
		// 'stick' before we prematurely exit further below
		;
	}
	else
	{
		// when adapting, don't move forward if it means moving to a
		// retranslation pile but we don't care when we are glossing
		bool bNotInRetranslation =
			CheckPhraseBoxDoesNotLandWithinRetranslation(pView,pFwd,pCurPile);
		if (!gbIsGlossing && !bNotInRetranslation)
		{
			// BEW removed this message, because it is already embedded in the prior
			// CheckPhraseBoxDoesNotLandWithinRetranslation(pView,pFwd,pCurPile) call, and
			// will be shown from there if relevant.
			//wxMessageBox(_(
            //"Sorry, to edit or remove a retranslation you must use the toolbar buttons for those operations."),
			//_T(""), wxICON_INFORMATION | wxOK);
            pApp->m_pTargetBox->SetFocusAndSetSelectionAtLanding(); // whm 13Aug2018 modified
            GetLayout()->m_docEditOperationType = no_edit_op;
			return FALSE;
		}
	}

    // if the location is a <Not In KB> one, we want to skip the store & fourth line
    // creation but first check for user incorrectly editing and fix it
	if (!gbIsGlossing && !pCurPile->GetSrcPhrase()->m_bHasKBEntry &&
												pCurPile->GetSrcPhrase()->m_bNotInKB)
	{
        // if the user edited out the <Not In KB> entry from the KB editor, we need to put
        // it back so that the setting is preserved (the "right" way to change the setting
        // is to use the toolbar checkbox - this applies when adapting, not glossing)
		pApp->m_pKB->Fix_NotInKB_WronglyEditedOut(pCurPile);
		goto b;
	}

	if (pApp->m_targetPhrase.IsEmpty())
	{
		pApp->m_bForceAsk = FALSE; // make sure it's turned off, & allow
								   // function to continue
		if (gbIsGlossing)
			pCurPile->GetSrcPhrase()->m_gloss.Empty();
		else
			pCurPile->GetSrcPhrase()->m_adaption.Empty();
	}

    // make the punctuated target string, but only if adapting; note, for auto
    // capitalization ON, the function will change initial lower to upper as required,
    // whatever punctuation regime is in place for this particular sourcephrase instance...
    // we are about to leave the current phrase box location, so we must try to store what
    // is now in the box, if the relevant flags allow it. Test to determine which KB to
    // store to. StoreText( ) has been ammended for auto-capitalization support (July 2003)
	if (!gbIsGlossing)
	{
		pView->MakeTargetStringIncludingPunctuation(pCurPile->GetSrcPhrase(), pApp->m_targetPhrase);
		pView->RemovePunctuation(pDoc, &pApp->m_targetPhrase,from_target_text);
	}
	pApp->m_bInhibitMakeTargetStringCall = TRUE;
	if (gbIsGlossing)
		bOK = pApp->m_pGlossingKB->StoreText(pCurPile->GetSrcPhrase(), pApp->m_targetPhrase);
	else
		bOK = pApp->m_pKB->StoreText(pCurPile->GetSrcPhrase(), pApp->m_targetPhrase);
	pApp->m_bInhibitMakeTargetStringCall = FALSE;
	if (!bOK)
	{
		// restore default button image, and m_bCopySourcePunctuation to TRUE
		wxCommandEvent event;
		if (!pApp->m_bCopySourcePunctuation)
		{
			pApp->GetView()->OnToggleEnablePunctuationCopy(event);
		}
		GetLayout()->m_docEditOperationType = no_edit_op;
		return FALSE; // can't move if the storage failed
	}

b:	pDoc->ResetPartnerPileWidth(pOldActiveSrcPhrase);
    // whm 13Mar2020 Note: The above ResetPartnerPileWidth() calls pPile's SetMinWidth() 
    // which sets the m_nMinWidth value - which is the maximum extent of the src, adapt or 
    // gloss text - here for the pOldActiveSrcPhrase's pile. It also calls MarkStripInvalid(pPile)
    // to mark the strip invalid and puts the parent strip's index into CLayout::m_invalidStripArray
    // if it is not in the array already. There is also provision for a second (bool) 
    // parameter in the ResetPartnerPileWidth(), but it is currently unused internally 
    // within the function.

	// move to next pile's cell
	CPile* pNewPile = pView->GetNextPile(pCurPile); // does not update the view's
				// m_nActiveSequNum nor m_pActivePile pointer, so update these here,
				// provided NULL was not returned

	// restore default button image, and m_bCopySourcePunctuation to TRUE
	wxCommandEvent event;
	if (!pApp->m_bCopySourcePunctuation)
	{
		pApp->GetView()->OnToggleEnablePunctuationCopy(event);
	}
	if (pNewPile == NULL)
	{
        // we deem vertical editing current step to have ended if control gets into this
        // block, so user has to be asked what to do next if vertical editing is currently
        // in progress; and we tunnel out before m_nActiveSequNum can be set to -1
        // (otherwise vertical edit will crash when recalc layout is tried with a bad sequ
        // num value)
		if (gbVerticalEditInProgress)
		{
			m_bTunnellingOut = FALSE; // ensure default value set
			bool bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(-1,
							nextStep, TRUE); // bForceTransition is TRUE
			if (bCommandPosted)
			{
				// don't proceed further because the current vertical edit step has ended
				m_bTunnellingOut = TRUE; // so caller can use it
				GetLayout()->m_docEditOperationType = no_edit_op;
				return FALSE;
			}
		}

		if (!pApp->m_bSingleStep)
		{
			pApp->m_bAutoInsert = FALSE; // cause halt, if auto lookup & inserting is ON
		}

		// ensure the view knows the pile pointer is no longer valid
		pApp->m_pActivePile = NULL;
		pApp->m_nActiveSequNum = -1;
		GetLayout()->m_docEditOperationType = no_edit_op;
		return FALSE; // we are at the end of the file
	}
	else // we have a pointer to the next pile
	{
		pApp->m_bUserTypedSomething = FALSE; // user has not typed at the new location yet

        // don't commit to the new pile if we are in vertical edit mode, until we've
        // checked the pile is not in the gray text area...
        // if vertical editing is currently in progress we must check if the lookup target
        // is within the editable span, if not then control has moved the box into the gray
        // area beyond the editable span and that means a step transition is warranted &
        // the user should be asked what step is next
		if (gbVerticalEditInProgress)
		{
			int m_nCurrentSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
			m_bTunnellingOut = FALSE; // ensure default value set
			bool bCommandPosted = pView->VerticalEdit_CheckForEndRequiringTransition(
									m_nCurrentSequNum,nextStep); // bForceTransition is FALSE
			if (bCommandPosted)
			{
				// don't proceed further because the current vertical edit step has ended
				m_bTunnellingOut = TRUE; // so caller can use it
				GetLayout()->m_docEditOperationType = no_edit_op;
				return FALSE; // try returning FALSE
			}
			else
			{
				// BEW 19Oct15 No transition of vert edit modes,
				// so we can store this location on the app
				gpApp->m_vertEdit_LastActiveSequNum = m_nCurrentSequNum;
#if defined(_DEBUG)
				wxLogDebug(_T("VertEdit PhrBox, MoveToImmedNextPile() storing loc'n: %d "), m_nCurrentSequNum);
#endif
			}
		}

        // whm 13Mar2020 addition. Comparing this part of MoveToImmediateNextPile() - in Reviewing
        // mode with the corresponding part of the View's PlacePhraseBox() - where transitioning
        // from code dealing with the old location and the new clicked location, there is the
        // addition of the following line, which probably needs to be included here also.
        // YES, the following line is all that was needed to get the phrasebox width to adjust
        // wider and smaller in Reviewing mode.
        GetLayout()->m_curBoxWidth = pApp->m_nMinPileWidth; // reset small for new location

        // whm 13Mar2020 Note: The following comment from PlacePhraseBox() seems appropriate
        // here just to help document what is happening next.
        //
        // setup the layout and phrase box at the new location; in the refactored design this
        // boils down to working out what the new active location's sequence number is, and
        // then setting the active pile to be the correct one, getting an appropriate gap
        // calculated for the "hole" the box is to occupy, tweaking the layout to conform to
        // these changes (either by a RecalcLayout() call, or AdjustForUserEdits() call -
        // either of which will make a new pile pointer, appropriately sized, for that
        // location), updating the m_pActivePile pointer on the app class, and then calling the
        // view class's Invalidate() function to get the tweaked layout drawn and the box made
        // visible, appropriately sized, at the new active location

		// update the active sequence number, and pile pointer
		pApp->m_nActiveSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
		pApp->m_pActivePile = pNewPile;

        // since we are moving forward from could be anywhere, the next pile is may have a
        // refString translation which is nonempty, so we have to put it into
        // m_targetPhrase so that ResizeBox will use it; but if there is none, the copy the
        // source key if the m_bCopySource flag is set, else just set it to an empty
        // string. (bNeed Modify is a helper flag used for setting/clearing the document's
        // modified flag at the end of this function)
		bool bNeedModify = FALSE; // reset to TRUE if we copy source because there was no
								  // adaptation

        // beware, next call the pRefString pointer may point to <Not In KB>, so take that
        // into account; GetRefString has been modified for auto-capitalization support
		CRefString* pRefString = NULL;
		KB_Entry rsEntry;
		if (gbIsGlossing)
			rsEntry = pApp->m_pGlossingKB->GetRefString(pNewPile->GetSrcPhrase()->m_nSrcWords,
				pNewPile->GetSrcPhrase()->m_key, pNewPile->GetSrcPhrase()->m_gloss, pRefString);
		else
			rsEntry = pApp->m_pKB->GetRefString(pNewPile->GetSrcPhrase()->m_nSrcWords,
				pNewPile->GetSrcPhrase()->m_key, pNewPile->GetSrcPhrase()->m_adaption, pRefString);
		if (pRefString != NULL && rsEntry == really_present)
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
				pNewPile->GetSrcPhrase()->m_bHasKBEntry = FALSE; // ensures * shows above
															     // this srcPhrase
				pApp->m_targetPhrase = pNewPile->GetSrcPhrase()->m_targetStr;
				pNewPile->GetSrcPhrase()->m_bNotInKB = TRUE;
			}
			else
			{
				pApp->m_pTargetBox->m_bAbandonable = FALSE;
				pApp->m_targetPhrase = str;
			}

            // remove the translation from the KB, in case user wants to edit it before
            // it's stored again (RemoveRefString also clears the m_bHasKBEntry flag on the
            // source phrase)
			if (gbIsGlossing)
			{
				pApp->m_pGlossingKB->RemoveRefString(pRefString, pNewPile->GetSrcPhrase(),
											pNewPile->GetSrcPhrase()->m_nSrcWords);
				pApp->m_targetPhrase = pNewPile->GetSrcPhrase()->m_gloss;
			}
			else
			{
				pApp->m_pKB->RemoveRefString(pRefString, pNewPile->GetSrcPhrase(),
											pNewPile->GetSrcPhrase()->m_nSrcWords);
				pApp->m_targetPhrase = pNewPile->GetSrcPhrase()->m_targetStr;
			}
		}
		else // no kb entry or rsEntry == present_but_deleted
		{
			if (gbIsGlossing)
			{
				pNewPile->GetSrcPhrase()->m_bHasGlossingKBEntry = FALSE; // ensure it's
                                                // false when there is no KB entry
			}
			else
			{
				pNewPile->GetSrcPhrase()->m_bHasKBEntry = FALSE; // ensure it's false
												// when there is no KB entry
			}
            // try to get a suitable string instead from the sourcephrase itself, if that
            // fails then copy the sourcePhrase's key if the m_bCopySource flag is set
			if (gbIsGlossing)
			{
				pApp->m_targetPhrase = pNewPile->GetSrcPhrase()->m_gloss;
			}
			else
			{
				pApp->m_targetPhrase = pNewPile->GetSrcPhrase()->m_adaption;
			}
			if (pApp->m_targetPhrase.IsEmpty() && pApp->m_bCopySource)
			{
				if (!pNewPile->GetSrcPhrase()->m_bNullSourcePhrase)
				{
					pApp->m_targetPhrase = pView->CopySourceKey(
								pNewPile->GetSrcPhrase(),pApp->m_bUseConsistentChanges);
					bNeedModify = TRUE;
				}
				else
				{
					pApp->m_targetPhrase.Empty();
				}
			}
		}
        this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase); // initialize the phrase box too, so it doesn't
										// carry an old string to the next pile's cell

		// get the new active pile
		pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
		wxASSERT(pApp->m_pActivePile != NULL);

        // get an adjusted pile pointer for the new active location, and we want the
        // pile's strip to be marked as invalid and the strip index added to the
        // CLayout::m_invalidStripArray
		if (pNewPile != NULL)
		{
			pDoc->ResetPartnerPileWidth(pNewPile->GetSrcPhrase());
            // whm 13Mar2020 Note: The above ResetPartnerPileWidth() calls pPile's SetMinWidth() 
            // which sets the m_nMinWidth value - which is the maximum extent of the src, adapt or 
            // gloss text - here for the pNewPile. It also calls MarkStripInvalid(pPile) to mark 
            // the strip invalid and puts the parent strip's index into CLayout::m_invalidStripArray
            // if it is not in the array already. There is also provision for a second (bool) 
            // parameter in the ResetPartnerPileWidth(), but it is currently unused internally 
            // within the function.
        }

		// if the user has turned on the sending of synchronized scrolling messages
		// send the relevant message, a sync scroll is appropriate now, provided
		// reviewing mode is ON when the MoveToImmedNextPile() -- which is likely as this
		// latter function is only called when in reviewing mode (in wx and legacy
		// versions, this scripture ref message was not sent here, and should have been)
		if (!pApp->m_bIgnoreScriptureReference_Send && !pApp->m_bDrafting)
		{
			pView->SendScriptureReferenceFocusMessage(pApp->m_pSourcePhrases,
													pApp->m_pActivePile->GetSrcPhrase());
		}

		// recreate the phraseBox using the stored information
		pApp->m_nStartChar = -1; pApp->m_nEndChar = -1;
		GetLayout()->m_docEditOperationType = relocate_box_op;

		// fix the m_bSaveToKB flag, depending on whether or not srcPhrase is in KB
		if (!gbIsGlossing && !pApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry &&
										pApp->m_pActivePile->GetSrcPhrase()->m_bNotInKB)
		{
			pApp->m_bSaveToKB = FALSE;
		}
		else
		{
			pApp->m_bSaveToKB = TRUE;
		}

		// BEW note 30Mar09: later we may set clipping here or somewhere
		pView->Invalidate(); // I think this call is needed
		GetLayout()->PlaceBox();

		// BEW 13Apr20, control goes thru here when TAB or Enter gets a move to next empty
		// pile done - and PlacePhraseBox() does not get called (nor Jump()), so we have
		// to check here for landing within a footnote, extended footnote, cross-marker or
		// extended cross-marker span - if so, the two placeholder insert buttons must
		// be disabled by the Update...() handler for those
		pApp->m_bDisablePlaceholderInsertionButtons = FALSE; // initialise to enabled buttons
		if (pApp->m_pActivePile != NULL)
		{
			CSourcePhrase* pSPhr = pApp->m_pActivePile->GetSrcPhrase();
			bool bProhibited = pDoc->IsWithinSpanProhibitingPlaceholderInsertion(pSPhr);
			pApp->m_bDisablePlaceholderInsertionButtons = bProhibited;
		}

        if (bNeedModify)
			SetModify(TRUE); // our own SetModify(); calls MarkDirty()
		else
			SetModify(FALSE); // our own SetModify(); calls DiscardEdits()

		return TRUE;
	}
}

// This OnSysKeyUp() function is not called by any EVT_ handler in wxWidgets. It is only
// called by the OnKeyUp() handler when it detects the use of ALT, SHIFT, or CTRL used in
// simultaneous combination with other keys.
// whm 10Jan2018 modified key handling so that ALL AltDown(), ShiftDown(), and ControlDown()
// events are now sent to this OnSysKeyUp() function for processing.
// whm 16Feb2018 Notes: OnSysKeyUp() currently does the following key handling:
// 1. Just calls return if m_bReadOnlyAccess is TRUE
// 2. Handles ALT+ENTER to Make a Phrase if not glossing and selection count > 1.
// 3. Handles ALT+BACKSPACE to advance phrasebox without KB lookup if transliteration mode in ON
//    and not glossing.
// 4. Handles ALT+BACKSPACE if transliteration mode in OFF with error message to user.
// 5. Handles SHIFT+ALT+RIGHT for inserting a placeholder AFTER a current source selection, 
//    otherwise AFTER the active phrasebox location.
// 6. Handles SHIFT+ALT+LEFT for inserting a placeholder BEFORE a current source selection, 
//    otherwise BEFORE the active phrasebox location.
// 7. Handles ALT+RIGHT to extent selection right if not glossing.
// 8. Handles ALT+LEFT to extent selection left if not glossing.
// 9. Handles ALT+UP to summon the retranslation dialog via DoRetranslationByUpArrow().
// 10. Note: ALT+DOWN is no longer handled here (to insert placeholder BEFORE) in our key 
//     handlers because ALT+DOWN is the built in hot key to make the dropdown list open/close. 
//     See SHIFT+ALT+LEFT above.
// 11. Handle ALT+DELETE to un-merge a current merger into separate words, when not glossing.
// 12. Handle SHIFT+ENTER, SHIFT+TAB, SHIFT+NUMPAD_ENTER and SHIFT+NUMPAD_TAB to move phrasebox 
//     backwards one pile.
// 13. Handle SHIFT+UP to scroll the screen up about 1 strip. A simple WXK_UP cannot be used anymore
//     because it is reserved to move the highlight in the dropdown list.
// 14. Handle SHIFT+DOWN to scroll the screen down about 1 strip. A simple WXK_DOWN cannot be used 
//     anymore because it is reserved to move the highlight in the dropdown list.
// 15. Handle SHIFT+PAGEUP to scroll the screen up about a screen full. A simple WXK_PAGEUP cannot
//     be used anymore because it is reserved to move the highlight in the dropdown list.
// 16. Handle SHIFT+PAGEDOWN to scroll the screen down about a screen full. A simple WXK_PAGEDOWN
//     cannot be used anymore because it is reserved to move the highlight in the dropdown list.
// 17. Handle SHIFT+CTRL+SPACE to enter a ZWSP (zero width space) into the composebar's editbox; 
//     replacing a selection if there is one defined.
// 18. Handle CTRL+ENTER to Jump Forward when transliteration, or warning message if not 
//     transliteration is not turned ON.
// 19. Conditionally compiled blocks to handle accelerator key combinations using CTRL+Alphanumeric
//     key, for when the dropdown list is open/showing on the __WXGTK__ and __WXMAC__ platforms.
//     Some previous testing indicated that on Linux and the Mac, the accelerator keys were not
//     triggered when the dropdown list was open/showing, so they are conditionally compiled 
//     here in OnSysKeyUp() for those two platforms, and executed here when the dropdown list is
//     open/showing.
//
// Note: Put in this OnSysKeyUp() handler custom key handling that involve simultaneous use 
// of ALT, SHIFT, or CTRL keys.
// BEW 13Apr10, no changes needed for support of doc version 5
void CPhraseBox::OnSysKeyUp(wxKeyEvent& event)
{
	//wxLogDebug(_T("In CPhraseBox::OnSysKeyUp() key code: %d"), event.GetKeyCode());

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
		// whm added 26Mar12. Don't allow AltDown + key events when in read-only mode
		if (pApp->m_bReadOnlyAccess)
		{
			return;
		}

		// ALT key or Control/Command key is down
		if (event.AltDown() && event.GetKeyCode() == WXK_RETURN) // ALT+ENTER
		{
			// ALT key is down, and <RET> was nChar typed (ie. 13), so invoke the
			// code to turn selection into a phrase; but if glossing is ON, then
			// merging must not happen - in which case exit early
			if (gbIsGlossing || !(pApp->m_selection.GetCount() > 1))
				return;
			pView->MergeWords();
			GetLayout()->m_docEditOperationType = merge_op;

            // whm 3Aug2018 modified for latest protocol of only selecting all when
            // user has set App's m_bSelectCopiedSource var to TRUE by ticking the
            // View menu's 'Select Copied Source' toggle menu item. 
            int len = pApp->m_pTargetBox->GetTextCtrl()->GetValue().Length();
 			pApp->m_nStartChar = len;
			pApp->m_nEndChar = len;

            pApp->m_pTargetBox->SetFocusAndSetSelectionAtLanding(); // whm 13Aug2018 modified

            // set old sequ number in case required for toolbar's Back button - in this case
			// it may have been a location which no longer exists because it was absorbed in
			// the merge, so set it to the merged phrase itself
            pApp->m_nOldSequNum = pApp->m_nActiveSequNum;
			return;
		}

		// BEW added 19Apr06 to allow Bob Eaton to advance phrasebox without having any lookup of the KB done.
		// We want this to be done only for adapting mode, and provided transliteration mode is turned on
		if (!gbIsGlossing && pApp->m_bTransliterationMode && event.GetKeyCode() == WXK_BACK) // ALT+BACKSPACE (transliterating)
		{
			// save old sequ number in case required for toolbar's Back button
            pApp->m_nOldSequNum = pApp->m_nActiveSequNum;

			m_bSuppressStoreForAltBackspaceKeypress = TRUE; // suppress store to KB for this move of box
			// the value is restored to FALSE in MoveToImmediateNextPile()

			// do the move forward to next empty pile, with lookup etc, but no store due to
			// the m_bSuppressStoreForAltBackspaceKeypress global being TRUE until the StoreText()
			// call is jumped over in the MoveToNextPile() call within JumpForward()
			JumpForward(pView);
			return;
		}
		else if (!gbIsGlossing && !pApp->m_bTransliterationMode && event.GetKeyCode() == WXK_BACK) // ALT+BACKSPACE (not transliterating)
		{
			// Alt key down & Backspace key hit, so user wanted to initiate a transliteration
			// advance of the phrase box, with its special KB storage mode, but forgot to turn the
			// transliteration mode on before using this feature, so warn him to turn it on and then
			// do nothing
			//IDS_TRANSLITERATE_OFF
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            gpApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(_("Transliteration mode is not yet turned on."),_T(""),wxICON_EXCLAMATION | wxOK);

			// restore focus to the phrase box
            if (pApp->m_pTargetBox != NULL)
            {
                if (pApp->m_pTargetBox->IsShown())
                {
                    pApp->m_pTargetBox->SetFocusAndSetSelectionAtLanding();// whm 13Aug2018 modified
                }
            }
            return;
		}

        // whm 11Mar2020 Note: We now handle cases of SHIFT+ALT+RIGHT and SHIFT+ALT+LEFT separately 
        // from ALT+RIGHT and ALT+LEFT. The reason: We want to now use SHIFT+ALT+RIGHT and SHIFT+ALT+LEFT 
        // for inserting a placeholder AFTER and inserting a placeholder BEFORE, respectively.
        // In this block we've so far only detected (above) that the ALT key is down. The CTRL and/or
        // SHIFT key may also be down! To limit the code below to just handle ALT+RIGHT and ALT+LEFT we need to
        // test further to see if the CTRL and/or SHIFT keys are down. 

        if (event.ShiftDown() && !event.ControlDown()) 
        {
            // SHIFT+ALT combinations are detected here, but not CTRL simultaneously with SHIFT and ALT.
            // Here we detect SHIFT+ALT+RIGHT and SHIFT+ALT+LEFT for inserting placeholders AFTER and BEFORE
            // the current selection (if a selection is made), otherwise AFTER and BEFORE the current phrasebox location.

            // Don't allow insertion of placeholders if glossing is ON, or if read-only access is current - in 
            // those cases, just return.
            if (gbIsGlossing)
                return;
            if (pApp->m_bReadOnlyAccess)
                return;

            if (event.GetKeyCode() == WXK_RIGHT) // SHIFT+ALT+RightArrow - add placeholder AFTER
            {
                // Add Placeholder AFTER a selection (if a selection exists) or the PhraseBox position.
                // whm Note 11Mar2020: Control passes through here when a simultaneous SHIFT+ALT+RightArrow 
                // press is released. 

                // TODO: Check if SHIFT+ALT+RightArrow conflicts with any of a Mac's system key assignments,
                // and adjust code here if necessary to accommodate the Mac port.
//#ifndef __WXMAC__
                // first save old sequ number in case required for toolbar's Back button
                // If glossing is ON, we don't allow the insertion, and just return instead
                pApp->m_nOldSequNum = pApp->m_nActiveSequNum;
                pApp->GetPlaceholder()->InsertNullSrcPhraseAfter();
//#endif
                return;
            }
            else if (event.GetKeyCode() == WXK_LEFT) // SHIFT+ALT+LeftArrow - add placeholder BEFORE
            {
                // Add Placeholder BEFORE a selection (if a selection exists) or the PhraseBox position.
                // whm Note 11Mar2020: Control passes through here when a simultaneous SHIFT+ALT+LeftArrow 
                // press is released. 

                // TODO: Check if SHIFT+ALT+RightArrow conflicts with any of a Mac's system key assignments,
                // and adjust code here if necessary to accommodate the Mac port.

                // Insert of null sourcephrase but first save old sequ number in case required for toolbar's
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
                        this->GetTextCtrl()->ChangeValue(_T(""));
                    }
                }

                // now do the 'insert before'
                pApp->m_nOldSequNum = pApp->m_nActiveSequNum;
                pApp->GetPlaceholder()->InsertNullSrcPhraseBefore();
                return;
            }
        }
        else if (!event.ShiftDown() && !event.ControlDown())
        {

            // ALT key is down, and neither CTRL nor SHIFT is down simultaneously.
            // If a right/left arrow key pressed, extend/retract sel'n left or right
            // or insert a null source phrase, or open the retranslation dialog; but don't
            // allow any of this (except Alt + Backspace) if glossing is ON - in those cases, just return
            if (gbIsGlossing)
                return;
            GetSelection(&nStart, &nEnd); //GetTextCtrl()->GetSelection(&nStart, &nEnd);
            if (event.GetKeyCode() == WXK_RIGHT) // ALT+RIGHT
            {
                if (gbRTL_Layout)
                    bTRUE = pView->ExtendSelectionLeft();
                else
                    bTRUE = pView->ExtendSelectionRight();
                if (!bTRUE)
                {
                    if (gbVerticalEditInProgress)
                    {
                        ::wxBell();
                    }
                    else
                    {
                        // did not succeed - do something eg. warn user he's collided with a boundary
                        // IDS_RIGHT_EXTEND_FAIL
                        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                        gpApp->m_bUserDlgOrMessageRequested = TRUE;
                        wxMessageBox(_("Sorry, you cannot extend the selection that far to the right unless you also use one of the techniques for ignoring boundaries."),
							_T(""), wxICON_INFORMATION | wxOK);
                    }
                }
                // whm 13Aug2018 Note: SetFocus() is correctly placed before the SetSelection(nStart,nEnd) 
                // call below.
                this->GetTextCtrl()->SetFocus();
                // whm 3Aug2018 Note: Below the previous selection is being restored,
                // so no adjustment made for 'Select Copied Source'
                this->GetTextCtrl()->SetSelection(nStart, nEnd);
                pApp->m_nStartChar = nStart;
                pApp->m_nEndChar = nEnd;
            }
            else if (event.GetKeyCode() == WXK_LEFT) // ALT+LEFT
            {
                if (gbRTL_Layout)
                    bTRUE = pView->ExtendSelectionRight();
                else
                    bTRUE = pView->ExtendSelectionLeft();
                if (!bTRUE)
                {
                    if (gbVerticalEditInProgress)
                    {
                        ::wxBell();
                    }
                    else
                    {
                        // did not succeed, so warn user
                        // IDS_LEFT_EXTEND_FAIL
                        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                        gpApp->m_bUserDlgOrMessageRequested = TRUE;
                        wxMessageBox(_("Sorry, you cannot extend the selection that far to the left unless you also use one of the techniques for ignoring boundaries. "),
							_T(""), wxICON_INFORMATION | wxOK);
                    }
                }
                // whm 13Aug2018 Note: SetFocus() is correctly placed before the SetSelection(nStart,nEnd) 
                // call below.
                this->GetTextCtrl()->SetFocus();
                // whm 3Aug2018 Note: Below the previous selection is being restored,
                // so no adjustment made for 'Select Copied Source'
                this->GetTextCtrl()->SetSelection(nStart, nEnd);
                pApp->m_nStartChar = nStart;
                pApp->m_nEndChar = nEnd;
            }
            else if (event.GetKeyCode() == WXK_UP) // ALT+UP (also available with CTRL+R or ToolBar Button on current selection)
            {
                // up arrow was pressed, so get the retranslation dialog open
                if (pApp->m_pActivePile == NULL)
                {
                    return;
                    //goto a;
                }
                if (pApp->m_selectionLine != -1)
                {
                    // if there is at least one source phrase with a selection defined,
                    // then then use the selection and put up the dialog
                    pApp->GetRetranslation()->DoRetranslationByUpArrow();
                }
                else
                {
                    // no selection, so make a selection at the phrase box and invoke the
                    // retranslation dialog on it
                    //CCell* pCell = pApp->m_pActivePile->m_pCell[2];
                    CCell* pCell = pApp->m_pActivePile->GetCell(1);
                    wxASSERT(pCell);
                    pApp->m_selection.Append(pCell);
                    //pApp->m_selectionLine = 1;
                    pApp->m_selectionLine = 0;// in refactored layout, src text line is always index 0
                    wxUpdateUIEvent evt;
                    //pView->OnUpdateButtonRetranslation(evt);
                    // whm Note: calling OnUpdateButtonRetranslation(evt) here doesn't work because there
                    // is not enough idle time for the Do A Retranslation toolbar button to be enabled
                    // before the DoRetranslationByUpArrow() call below executes - which has code in it
                    // to prevent the Retranslation dialog from poping up unless the toolbar button is
                    // actually enabled. So, we explicitly enable the toolbar button here instead of
                    // waiting for it to be done in idle time.
                    CMainFrame* pFrame = pApp->GetMainFrame();
                    wxASSERT(pFrame != NULL);
                    wxAuiToolBarItem *tbi;
                    tbi = pFrame->m_auiToolbar->FindTool(ID_BUTTON_RETRANSLATION);
                    // Return if the toolbar item is hidden
                    if (tbi == NULL)
                    {
                        return;
                    }
                    // enable the toolbar button
                    pFrame->m_auiToolbar->EnableTool(ID_BUTTON_RETRANSLATION, true);
                    pApp->GetRetranslation()->DoRetranslationByUpArrow();
                }
            }
            // BEW added 26Sep05, to implement Roland Fumey's request that the shortcut for unmerging
            // not be SHIFT+End as in the legacy app, but something else; so I'll make it ALT+Delete
            // and then SHIFT+End can be used for extending the selection in the phrase box's CEdit
            // to the end of the box contents (which is Windows standard behaviour, & what Roland wants)
            if (event.GetKeyCode() == WXK_DELETE) // ALT+DELETE
            {
                // we have ALT + Delete keys held down, so unmerge the current merger - separating into
                // individual words but only when adapting, if glossing we instead just return
                CSourcePhrase* pSP;
                if (pApp->m_pActivePile == NULL)
                {
                    return;
                }
                if (pApp->m_selectionLine != -1 && pApp->m_selection.GetCount() == 1)
                {
                    CCellList::Node* cpos = pApp->m_selection.GetFirst();
                    CCell* pCell = cpos->GetData();
                    pSP = pCell->GetPile()->GetSrcPhrase();
                    if (pSP->m_nSrcWords == 1)
                        return;
                }
                else if (pApp->m_selectionLine == -1 && pApp->m_pTargetBox->GetHandle() != NULL
                    && pApp->m_pTargetBox->IsShown())
                {
                    pSP = pApp->m_pActivePile->GetSrcPhrase();
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
                    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                    gpApp->m_bUserDlgOrMessageRequested = TRUE;
                    wxMessageBox(_("This particular operation is not available when you are glossing."),
                        _T(""), wxICON_INFORMATION | wxOK);
                    return;
                }
                pView->UnmergePhrase(); // calls OnButtonRestore() - which will attempt to do a lookup,
                    // so don't remake the phrase box with the global (wxString) m_SaveTargetPhrase,
                    // otherwise it will override a successful lookkup and make the ALT+Delete give
                    // a different result than the Unmerge button on the toolbar. So we in effect
                    // are ignoring m_SaveTargetPhrase (the former is used in PlacePhraseBox() only
                GetLayout()->m_docEditOperationType = unmerge_op;

                // save old sequ number in case required for toolbar's Back button - the only safe
                // value is the first pile of the unmerged phrase, which is where the phrase box
                // now is
                pApp->m_nOldSequNum = pApp->m_nActiveSequNum;
            } // end of if (event.GetKeyCode() == WXK_DELETE)
        } // end of else if (!event.ShiftDown() && !event.ControlDown())
	} // end of if (event.AltDown())

    // whm 15Feb2018 added to process new SHIFT down scrolling behaviors (that previously were
    // processed as plain arrow keys.
    // The reason for adding ShiftDown() to the arrow and page scrolling key operations is that
    // the dropdown control needs to use the normal/unmodified arrow and page keys for use within
    // the dropdown control. Hence, scrolling operations using the keyboard now require the 
    // simultaneous holding down of the SHIFT key.
    else if (event.ShiftDown() && !event.ControlDown()) // SHIFT + other key handled here, not with CTRL simultaneously down
    {
        // whm 11Mar2020 Note: In this block we only handle SHIFT+ENTER, SHIFT+TAB, SHIFT+NUMPAD_ENTER, SHIFT+NUMPAD_TAB,
        // SHIFT+UP, SHIFT+DOWN, SHIFT+PageUp, and SHIFT+PageDown (and no simultaneous CTRL down)

        // whm 14Apr2020 BEW called attention to the fact that SHIFT+ENTER and SHIFT+TAB was not working ever
        // since I refactored the OnKeyUp() and OnSysKeyUp() handlers. Although I had suggested to him
        // that the code should go here, he was having trouble with his VS2015 stealing focus preventing
        // him from detecting/debugging the code when he tried to place it here in OnSysKeyUp(). I've tested
        // it here where it works fine, so I've moved the code for handling SHIFT+ENTER and SHIFT+TAB from 
        // OnKeyUp() where it was previously located (where I had forgotten to move it here during refactoring),
        // and where yesterday BEW moved it up above the OnSysKeyUp() call in OnKeyUp(). I've removed it from the
        // location he placed it at there, over to here in OnSysKeyUp(). He said he had tried to place it here but 
        // couldn't do so, saying he couldn't test the SHIFT+ENTER or SHIFT+TAB here in OnSysKeyUp()
        // without Visual Studio 15 stealing those key strokes and inserting a line in source code, making the 
        // debugging session stale, and thus preventing him from being able to test the aforementioned code 
        // handling here. However, I have not encountered any problem putting the code here in OnSysKeyUp() 
        // where is should logically go along with other SHIFT modified key strokes all of which are 
        // consolidated here.
        int keycode = event.GetKeyCode();
        if (keycode == WXK_TAB             // SHIFT+TAB
            || keycode == WXK_RETURN       // SHIFT+ENTER
            || keycode == WXK_NUMPAD_ENTER // SHIFT+NUMPAD_ENTER
            || keycode == WXK_NUMPAD_TAB)  // SHIFT+NUMPAD_TAB  //whm 5Jul2018 added for extended keyboard numpad ENTER and numpad TAB users
        {
            // Now handle the WXK_TAB or WXK_RETURN processing.
            // whm 24June2018 Note: The handling of WXK_TAB and WXK_RETURN should be identical.
            // So, pressing Tab within dropdown phrasebox - when the content of the dropdown's edit 
            // box is open is tantamount to having selected that dropdown list item directly.
            // It should - in the new app - do the same thing that happened in the legacy app when 
            // the Choose Translation dialog had popped up and the desired item was already
            // highlighted in its dialog list, and the user pressed the OK button. That OK button
            // press is essentially the same as the user in the new app pressing the Enter key
            // when something is contained in the dropdown's edit box. 
            // Hence, before calling MoveToPrevPile() or JumpForward() below, we should inform
            // the app to handle the contents as a change.
            wxString boxContent;
            boxContent = this->GetValue();
            gpApp->m_targetPhrase = boxContent;
            if (!this->GetTextCtrl()->IsModified()) // need to call SetModified on m_pTargetBox before calling SetValue // whm 12Jul2018 added GetTextCtrl()->
            {
                this->GetTextCtrl()->SetModified(TRUE); // Set as modified so that CPhraseBox::OnPhraseBoxChanged() will do its work // whm 12Jul2018 added GetTextCtrl()->
            }
            this->m_bAbandonable = FALSE; // this is done in CChooseTranslation::OnOK()

                                          // save old sequ number in case required for toolbar's Back button
            pApp->m_nOldSequNum = pApp->m_nActiveSequNum;

            // SHIFT+TAB is the 'universal' keyboard way to cause a move back, so implement it
            // whm Note: Beware! Setting breakpoints in OnChar() before this point can
            // affect wxGetKeyState() results making it appear that WXK_SHIFT is not detected
            // below. Solution: remove the breakpoint(s) for wxGetKeyState(WXK_SHIFT) to <- end of comment lost
            // shift key is down, so move back a pile

            if (keycode == WXK_TAB)
                wxLogDebug(_T("CPhraseBox::OnKeyUp() handling SHIFT + WXK_TAB key"));
            else if (keycode == WXK_RETURN)
                wxLogDebug(_T("CPhraseBox::OnKeyUp() handling SHIFT + WXK_RETURN key"));

            // Shift+Tab or Shift+RETURN (reverse direction) indicates user is probably
            // backing up to correct something that was perhaps automatically
            // inserted, so we will preserve any highlighting and do nothing
            // here in response to Shift+Tab.

            Freeze();

            int bSuccessful = MoveToPrevPile(pApp->m_pActivePile);
            if (!bSuccessful)
            {
                // we have come to the start of the document, so do nothing
                GetLayout()->m_docEditOperationType = no_edit_op;
            }
            else
            {
                // it was successful
                GetLayout()->m_docEditOperationType = relocate_box_op;
            }

            // scroll, if necessary
            pApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);

            // save the phrase box's text, in case user hits SHIFT+END key to unmerge
            // a phrase
            m_SaveTargetPhrase = pApp->m_targetPhrase;

            Thaw();
            return;
        }

        // does user want to unmerge a phrase?

        // user did not want to unmerge, so must want a scroll
        // preserve cursor location (its already been moved by the time this function is entered)
        int nScrollCount = 1;

        // the code below for up arrow or down arrow assumes a one strip + leading scroll. If
        // we change so as to scroll more than one strip at a time (as when pressing page up or
        // page down keys), iterate by the number of strips needing to be scrolled
        CLayout* pLayout = GetLayout();
        int yDist = pLayout->GetStripHeight() + pLayout->GetCurLeading();

        // do the scroll (CEdit also moves cursor one place to left for uparrow, right for
        // downarrow hence the need to restore the cursor afterwards; however, the values of
        // nStart and nEnd depend on whether the app made the selection, or the user; and also
        // on whether up or down was pressed: for down arrow, the values will be either 1,1 or
        // both == length of line, for up arrow, values will be either both = length of line
        // -1; or 0,0 so code accordingly
        bool bScrollFinished = FALSE;
        int nCurrentStrip;
        int nLastStrip;

        if (event.GetKeyCode() == WXK_UP) // SHIFT+UP
        {
        a:	int xPixelsPerUnit, yPixelsPerUnit;
            // whm 14Feb2018 with CPhraseBox implementing a dropdown list, we need to reserve the
            // WXK_UP key for scrolling up in the dropdown list, rather than scrolling the canvas.
            // Screen scrolling up is now donw here with SHIFT+UP.
#ifdef Do_Clipping
            pLayout->SetScrollingFlag(TRUE); // need full screen drawing, so clipping can't happen
#endif
            pLayout->m_pCanvas->GetScrollPixelsPerUnit(&xPixelsPerUnit, &yPixelsPerUnit);
            wxPoint scrollPos;
            // MFC's GetScrollPosition() "gets the location in the document to which the upper
            // left corner of the view has been scrolled. It returns values in logical units."
            // wx note: The wx docs only say of GetScrollPos(), that it
            // "Returns the built-in scrollbar position."
            // I assume this means it gets the logical position of the upper left corner, but
            // it is in scroll units which need to be converted to device (pixel) units

            pLayout->m_pCanvas->CalcUnscrolledPosition(0, 0, &scrollPos.x, &scrollPos.y);
            // the scrollPos point is now in logical pixels from the start of the doc

            if (scrollPos.y > 0)
            {
                if (scrollPos.y >= yDist)
                {
                    // up uparrow was pressed, so scroll up a strip, provided we are not at the
                    // start of the document; and we are more than one strip + leading from the start,
                    // so it is safe to scroll
                    pLayout->m_pCanvas->ScrollUp(1);
                }
                else
                {
                    // we are close to the start of the document, but not a full strip plus leading,
                    // so do a partial scroll only - otherwise phrase box will be put at the wrong
                    // place
                    yDist = scrollPos.y;
                    scrollPos.y -= yDist;

                    int posn = scrollPos.y;
                    posn = posn / yPixelsPerUnit;
                    // Note: MFC's ScrollWindow's 2 params specify the xAmount and yAmount to
                    // scroll in device units (pixels). The equivalent in wx is Scroll(x,y) in
                    // which x and y are in SCROLL UNITS (pixels divided by pixels per unit).
                    // Also MFC's ScrollWindow takes parameters whose value represents an
                    // "amount" to scroll from the current position, whereas the
                    // wxScrolledWindow::Scroll takes parameters which represent an absolute
                    // "position" in scroll units. To convert the amount we need to add the
                    // amount to (or subtract from if negative) the logical pixel unit of the
                    // upper left point of the client viewing area; then convert to scroll
                    // units in Scroll().
                    pLayout->m_pCanvas->Scroll(0, posn); //pView->ScrollWindow(0,yDist);
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
            return;
            
            // restore cursor location when done
            // whm 13Aug2018 Note: SetFocus() is correctly placed before the 
            // SetSelection(pApp->m_nStartChar, pApp->m_nEndChar) call below.
        c:	this->GetTextCtrl()->SetFocus();
            // whm 3Aug2018 Note: Below the previous selection is being restored,
            // so no adjustment made for 'Select Copied Source'
            this->GetTextCtrl()->SetSelection(pApp->m_nStartChar, pApp->m_nEndChar);
            return;
        }
        else if (event.GetKeyCode() == WXK_DOWN) // SHIFT+DOWN
        {
            // down arrow was pressed, so scroll down a strip, provided we are not at the end of
            // the bundle
        b:	wxPoint scrollPos;

            // whm 14Feb2018 with CPhraseBox implementing a dropdown list, we need to reserve the
            // WXK_DOWN key for scrolling down in the dropdown list, rather than scrolling the canvas.
            // Screen scrolling down is now donw here with SHIFT+DOWN.
#ifdef Do_Clipping
            pLayout->SetScrollingFlag(TRUE); // need full screen drawing, so clipping can't happen
#endif
            int xPixelsPerUnit, yPixelsPerUnit;
            pLayout->m_pCanvas->GetScrollPixelsPerUnit(&xPixelsPerUnit, &yPixelsPerUnit);
            // MFC's GetScrollPosition() "gets the location in the document to which the upper
            // left corner of the view has been scrolled. It returns values in logical units."
            // wx note: The wx docs only say of GetScrollPos(), that it
            // "Returns the built-in scrollbar position."
            // I assume this means it gets the logical position of the upper left corner,
            // but it is in scroll units which need to be converted to device (pixel) units

            wxSize maxDocSize; // renaming barSizes to maxDocSize for clarity
            pLayout->m_pCanvas->GetVirtualSize(&maxDocSize.x, &maxDocSize.y); // gets size in pixels

            pLayout->m_pCanvas->CalcUnscrolledPosition(0, 0, &scrollPos.x, &scrollPos.y);
            // the scrollPos point is now in logical pixels from the start of the doc

            wxRect rectClient(0, 0, 0, 0);
            wxSize canvasSize;
            canvasSize = pApp->GetMainFrame()->GetCanvasClientSize();
            rectClient.width = canvasSize.x;
            rectClient.height = canvasSize.y;

            int logicalViewBottom = scrollPos.y + rectClient.GetBottom();
            if (logicalViewBottom < maxDocSize.GetHeight())
            {
                if (logicalViewBottom <= maxDocSize.GetHeight() - yDist)
                {
                    // a full strip + leading can be scrolled safely
                    pLayout->m_pCanvas->ScrollDown(1);
                }
                else
                {
                    // we are close to the end, but not a full strip + leading can be scrolled, so
                    // just scroll enough to reach the end - otherwise position of phrase box will
                    // be set wrongly
                    wxASSERT(maxDocSize.GetHeight() >= logicalViewBottom);
                    yDist = maxDocSize.GetHeight() - logicalViewBottom; // make yDist be what's
                                                                        // left to scroll
                    scrollPos.y += yDist;

                    int posn = scrollPos.y;
                    posn = posn / yPixelsPerUnit;
                    pLayout->m_pCanvas->Scroll(0, posn);
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
            return;
            
            // restore cursor location when done
            // whm 13Aug2018 Note: SetFocus() is correctly placed before the 
            // SetSelection(pApp->m_nStartChar, pApp->m_nEndChar) call below.
        d:	this->GetTextCtrl()->SetFocus();
            // whm 3Aug2018 Note: Below the previous selection is being restored,
            // so no adjustment made for 'Select Copied Source'
            this->GetTextCtrl()->SetSelection(pApp->m_nStartChar, pApp->m_nEndChar);
            return;
        }
        else if (event.GetKeyCode() == WXK_PAGEUP) // SHIFT+PAGEUP
        {
            // Note: an overload of CLayout::GetVisibleStripsRange() does the same job, so it
            // could be used instead here and for the other instance in next code block - as
            // these two calls are the only two calls of the view's GetVisibleStrips() function
            // in the whole application ** TODO ** ??
            pView->GetVisibleStrips(nCurrentStrip, nLastStrip);
            nScrollCount = nLastStrip - nCurrentStrip;
            goto a;
        }
        else if (event.GetKeyCode() == WXK_PAGEDOWN) // SHIFT+PAGEDOWN
        {
            // Note: an overload of CLayout::GetVisibleStripsRange() does the same job, so it
            // could be used instead here and for the other instance in above code block - as
            //6 these two calls are the only two calls of the view's GetVisibleStrips() function
            // in the whole application ** TODO ** ??
            pView->GetVisibleStrips(nCurrentStrip, nLastStrip);
            nScrollCount = nLastStrip - nCurrentStrip;
            goto b;
        }
    }

    if (event.ShiftDown() && event.ControlDown() && event.GetKeyCode() == WXK_SPACE) // SHIFT+CTRL+SPACE
    {
        // BEW 8Jul14 intercept the CTRL+SHIFT+<spacebar> combination to enter a ZWSP
        // (zero width space) into the composebar's editbox; replacing a selection if
        // there is one defined
        OnCtrlShiftSpacebar(this); //OnCtrlShiftSpacebar(this->GetTextCtrl()); // see helpers.h & .cpp // whm 14Feb2018 added ->GetTextCtrl()
        return; // don't call skip - we don't want the end-of-line character entered
                // into the edit box
    }

    if (event.ControlDown() && event.GetKeyCode() == WXK_RETURN) // CTRL+ENTER
    {
        if (!gbIsGlossing && pApp->m_bTransliterationMode)
        {
            // CTRL + ENTER is a JumpForward() to do transliteration; bleed this possibility
            // out before allowing for any keypress to halt automatic insertion; one side
            // effect is that MFC rings the bell for each such key press and I can't find a way
            // to stop it. So Alt + Backspace can be used instead, for the same effect; or the
            // sound can be turned off at the keyboard if necessary. This behaviour is only
            // available when transliteration mode is turned on.

            // save old sequ number in case required for toolbar's Back button
            pApp->m_nOldSequNum = pApp->m_nActiveSequNum;

            m_bSuppressStoreForAltBackspaceKeypress = TRUE; // suppress store to KB for
                                                           // this move of box, the value is restored to FALSE in MoveToNextPile()

            // do the move forward to next empty pile, with lookup etc, but no store due to
            // the m_bSuppressStoreForAltBackspaceKeypress global being TRUE until the StoreText()
            // call is jumped over in the MoveToNextPile() call within JumpForward()
            JumpForward(pView);
            return;
        }
        else if (!gbIsGlossing && !pApp->m_bTransliterationMode)
        {
            // user wanted to initiate a transliteration advance of the phrase box, with its
            // special KB storage mode, but forgot to turn the transliteration mode on before
            // using this feature, so warn him to turn it on and then do nothing
            // IDS_TRANSLITERATE_OFF
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            gpApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(_("Transliteration mode is not yet turned on."), _T(""), wxICON_EXCLAMATION | wxOK);

            // restore focus to the phrase box
            if (pApp->m_pTargetBox != NULL)
            {
                if (pApp->m_pTargetBox->IsShown())
                {
                    pApp->m_pTargetBox->SetFocusAndSetSelectionAtLanding();// whm 13Aug2018 modified
                }
            }
            return; // whm 16Feb2018 added 
        }
    }

    // whm 1July2018 added. Handle the accelerator key commands here in OnSysKeyUp() for the Linux/Mac platform
    // for the situation when the dropdown list is open. The code below addresses this problem:
    //    When the phrasebox's dropdown list is open all accelerator keys get effectively blocked on the 
    //    Linux/Mac(?) platform - but only when the dropdown list is open. 
    // Once the dropdown list is closed, the accelerator key works normally on Linux. This behavior results in the
    // user having to type the accelerator key combination twice to get it to execute on Linux/Mac(?) anytime the 
    // dropdown list is open.
    // Note that all predefined accelerator keys work on Windows even when the dropdown list is open, and in such 
    // cases in Windows invoking the accelerator key once closes the dropdown, and executes the desired action
    // as intended. Moreover, on Windows, the accelerator key event does not come through OnSysKeyUp() to this 
    // point - regardless of whether the dropdown is open or closed.
    // However, on Linux/Mac(?) the accelerator key gets (partially) consumed when it triggers the
    // closing of the dropdown list, so that the accelerator key combination does not carry out the intended
    // action, but fortunately, the key event does pass through this point in OnSysKeyUp(). So we can handle the 
    // accelerator key combinations here by explicitly calling the functions in the application directly here.
    // When the dropdown is closed on Linux/Mac(?), the accelerator key combination works as originally intended 
    // just like the Windows version and control doesn't pass through here. Hence, we don't have to conditionally
    // compile the blocks of code below that handle the accelerator key actions for Linux/Mac.
    // 
    // The accelerator keys are defined in the CMainFrame's constructor. They are all associated with either a
    // menu item's identifier, or a tool bar button's identifier (see copied code lines below), so that when 
    // invoked, the accelerator is designed to immediately invoke that menu item or tool bar button, just as
    // though the user selected that menu item or tool bar item.
    // There are upwards of 35 accelerator keys defined there. 
    // For our purposes here we will only handle the more common ones that would be invoked during editing, 
    // including the following - copied as they look in CMainFrame:
    /*
    entries[2].Set(wxACCEL_CTRL, (int) 'L', ID_BUTTON_CHOOSE_TRANSLATION); // whm checked OK
    entries[3].Set(wxACCEL_CTRL, (int) 'E', ID_BUTTON_EDIT_RETRANSLATION); // whm checked OK
    entries[4].Set(wxACCEL_CTRL, (int) 'M', ID_BUTTON_MERGE); // whm checked OK - OnButtonMerge() needed trap door added to avoid crash
    entries[5].Set(wxACCEL_CTRL, (int) 'I', ID_BUTTON_NULL_SRC); // whm checked OK
    entries[6].Set(wxACCEL_CTRL, (int) 'D', ID_BUTTON_REMOVE_NULL_SRCPHRASE); // whm checked OK
    entries[7].Set(wxACCEL_CTRL, (int) 'U', ID_BUTTON_RESTORE); // whm checked OK
    entries[8].Set(wxACCEL_CTRL, (int) 'R', ID_BUTTON_RETRANSLATION); // whm checked OK
    ...
    entries[14].Set(wxACCEL_CTRL, (int) '2', ID_EDIT_MOVE_NOTE_BACKWARD); // whm checked OK
    entries[15].Set(wxACCEL_CTRL, (int) '3', ID_EDIT_MOVE_NOTE_FORWARD); // whm checked OK
    ...
    entries[18].Set(wxACCEL_CTRL, (int) 'Q', ID_EDIT_SOURCE_TEXT); // whm checked OK
    ...
    entries[20].Set(wxACCEL_CTRL, (int) 'Z', wxID_UNDO); // standard wxWidgets ID
    ...
    entries[25].Set(wxACCEL_CTRL, (int) 'S', wxID_SAVE); // standard wxWidgets ID // whm checked OK
    ...
    entries[27].Set(wxACCEL_CTRL, (int) 'F', wxID_FIND); // standard wxWidgets ID // whm checked OK
    entries[28].Set(wxACCEL_CTRL, (int) 'G', ID_GO_TO); // On Mac Command-G is Find Next but this is close enough
    ...
    entries[33].Set(wxACCEL_CTRL, (int) 'K', ID_TOOLS_KB_EDITOR); // whm checked OK
    ...
    entries[35].Set(wxACCEL_CTRL, (int) '7', ID_TOOLS_CLIPBOARD_ADAPT);
    */

}

// return TRUE if we traverse this function without being at the end of the file, or
// failing in the LookAhead function (such as when there was no match); otherwise, return
// FALSE so as to be able to exit from the caller's loop
// BEW 13Apr10, no changes needed for support of doc version 5
// BEW 21May15 added freeze/thaw support

bool CPhraseBox::OnePass(CAdapt_ItView *pView)
{
#ifdef _FIND_DELAY
		wxLogDebug(_T("1. Start of OnePass"));
#endif
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
    CAdapt_ItDoc* pDoc = pApp->GetDocument();
    wxASSERT(pDoc != NULL);
	CLayout* pLayout = GetLayout();
	// BEW added 18Aug18 - ensure a large former m_curBoxWidth value is not retained in the move
	pLayout->m_curBoxWidth = pApp->m_nMinPileWidth; // reset small for new location

    // whm 20May2020 added call of ResetPartnerPileWidth() below. The above m_curBoxWidth setting 
    // alone resulted in a pile with too small for an inserted '...' placeholder when the target
    // text entered was much wider than the source "..." placeholder. The current pile's width
    // invalidate the strip needs to be done by a call to ResetPartnerPileWidth() before leaving 
    // this location, at least for when we are leaving a freshly inserted placeholder that has 
    // gotten a target text adaptation that is wider than the width of the source text's '...' 
    // placeholder. If this isn't done, the target string under the placeholder is overwritten 
    // by the following pile's target text when the phrasebox moves on. 
    pDoc->ResetPartnerPileWidth(pLayout->GetPile(pApp->m_nActiveSequNum)->GetSrcPhrase());

	//CSourcePhrase* pOldActiveSrcPhrase = NULL; // set but not used
	int nActiveSequNum = pApp->m_nActiveSequNum;
	if (nActiveSequNum < 0)
		return FALSE;
	//else
	//{
	//	pOldActiveSrcPhrase = (pView->GetPile(nActiveSequNum))->GetSrcPhrase();
	//}
	// save old sequ number in case required for toolbar's Back button
    pApp->m_nOldSequNum = nActiveSequNum;
	m_bBoxTextByCopyOnly = FALSE; // restore default setting

	// BEW 21May15 add here support for freezing the canvas for a NUMINSERTS number
	// of consecutive auto-inserts from the KB; the matching thaw calls are done
	// later below, before returning FALSE, or returning TRUE. The actual Freeze()
	// call is within CLayout's Redraw() and Draw() functions, and is managed by
	// the boolean m_bDoFreeze. Recall that OnePass() is called
	// from only one place in the whole app - from OnIdle(), and even then only when
	// several flags have certain values consistent with automatic insertions being
	// currently permitted. So the logic for doing a freeze and doing it's matching
	// thaw, is encapsulated within this one OnePass() function, except for an added
	// else block in OnIdle() where the m_nInsertCount value is defaulted to 0 whenever
	// m_bAutoInsert is FALSE.
	// whm 11Jun2022 removed the support for freezing the canvas window. It wasn't working
	// correctly, and there is no need for it since the dropdown phrasebox was introduced.
	//if (pApp->m_bSupportFreeze)
	//{
	//	if (pApp->m_nInsertCount == 0 || (pApp->m_nInsertCount % (int)NUMINSERTS == 0))
	//	{
	//		// Make the freeze happen only when the felicity conditions are satisfied
	//		if (
	//			!pApp->m_bIsFrozen      // canvas must not currently be frozen
	//			&& pApp->m_bDrafting    // the GUI is in drafting mode (only then are auto-inserts possible)
	//			&& (pApp->m_bAutoInsert || !pApp->m_bSingleStep) // one or both of these conditions apply
	//			)
	//		{
	//			// Ask for the freeze
	//			pApp->m_bDoFreeze = TRUE;
	//			// Count this call of OnePass()
	//			pApp->m_nInsertCount = 1;
	//			// Do a half-second delay, if that was set to 201 ticks
	//			if (pApp->m_nCurDelay == 31)
	//			{
	//				// DoDelay() at the doc end causes DoDelay() to never exit, so
	//				// protect from calling near the app end
	//				int maxSN = pApp->m_pSourcePhrases->GetCount() - 1;
	//				int safeLocSN = maxSN - (int)NUMINSERTS;
	//				if (safeLocSN < 0)
	//				{
	//					safeLocSN = 0;
	//				}
	//				if (maxSN > 0 && nActiveSequNum <= safeLocSN)
	//				{
	//					DoDelay(); // see helpers.cpp
	//					pApp->m_nCurDelay = 0; // restore to zero, we want it only the once
	//					// at the start of each subsection of inserts
	//				}
	//			}
	//		}
	//		else
	//		{
	//			// Ensure it's not asked for
	//			pApp->m_bDoFreeze = FALSE;
	//		}
	//	}
	//	else if (pApp->m_bIsFrozen && pApp->m_nInsertCount < (int)NUMINSERTS)
	//	{
	//		// The canvas is frozen, and we've not yet halted the auto-inserts, nor
	//		// have we reached the final OnePass() call of this subsequence, so
	//		// increment the count (the final call of a subsequence should be done
	//		// with the canvas having just been thawed)
	//		pApp->m_nInsertCount++;
	//	}
	//}
	int bSuccessful;
	if (pApp->m_bTransliterationMode && !gbIsGlossing)
	{
		bSuccessful = MoveToNextPile_InTransliterationMode(pApp->m_pActivePile);
	}
	else
	{
		#ifdef _FIND_DELAY
			wxLogDebug(_T("2. Before MoveToNextPile"));
		#endif
		bSuccessful = MoveToNextPile(pApp->m_pActivePile);
		#ifdef _FIND_DELAY
			wxLogDebug(_T("3. After MoveToNextPile"));
		#endif
		// If in vertical edit mode, and the phrasebox has come to a hole, then we
		// want to halt OnePass() calls in OnIdle() so that the user gets the chance
		// to adapt at the hole
		if (gbVerticalEditInProgress && bSuccessful)
		{
			// m_pActivePile will be at the hole
			CPile* pNewPile = pApp->m_pActivePile;
			CSourcePhrase* pSrcPhrase = pNewPile->GetSrcPhrase();
			if (pSrcPhrase->m_adaption.IsEmpty())
			{
				// make sure the old location's values are not carried to this location
				m_Translation.Empty();
				pApp->m_targetPhrase.Empty();
                this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase);
				// make OnIdle() halt auto-inserting
				pApp->m_bAutoInsert = FALSE;
			}
		}
	}
	if (!bSuccessful)
	{
		// BEW added 4Sep08 in support of transitioning steps within vertical edit mode
		if (gbVerticalEditInProgress && m_bTunnellingOut)
		{
			// MoveToNextPile might fail within the editable span while vertical edit is
			// in progress, so we have to allow such a failure to not cause tunnelling out;
			// hence we use the m_bTunnellingOut global to assist - it is set TRUE only when
			// VerticalEdit_CheckForEndRequiringTransition() in the view class returns TRUE,
			// which means that a PostMessage(() has been done to initiate a step transition
			m_bTunnellingOut = FALSE; // caller has no need of it, so clear to default value
			pLayout->m_docEditOperationType = no_edit_op;

			pApp->m_bUserDlgOrMessageRequested = FALSE;
			pApp->m_bUserHitEnterOrTab = FALSE;

			return FALSE; // caller is OnIdle(), OnePass is not used elsewhere
		}


		// it will have failed because we are at eof without finding a "hole" at which to
		// land the phrase box for reaching the document's end, so we must handle eof state
		//if (pApp->m_pActivePile == NULL && pApp->m_endIndex < pApp->m_maxIndex)
		if (pApp->m_pActivePile == NULL || pApp->m_nActiveSequNum == -1)
		{
			// we got to the end of the doc...

			// BEW changed 9Apr12, support discontinuous auto-inserted spans highlighting
			//pLayout->ClearAutoInsertionsHighlighting(); <- I'm trying no call here,
			// hoping that JumpForward() will suffice, so that OnIdle()'s call of OnePass()
			// twice at the end of doc won't clobber the highlighting already established.
            // YES!! That works - the highlighting is now visible when the box has
            // disappeared and the end of doc message shows. Also, normal adapting still
            // works right despite this change, so that's a bug (or undesireable feature -
            // namely, the loss of highlighting when the doc is reached by auto-inserting)
            // now fixed.

			// BEW 21May13, first place for doing a thaw...
			// whm 11Jun2022 removed the support for freezing the canvas window. It wasn't working
			// correctly, and there is no need for it since the dropdown phrasebox was introduced.
			//if (pApp->m_bSupportFreeze && pApp->m_bIsFrozen)
			//{
			//	// We want Thaw() done, and the next call of OnPass() will then get the view 
			//	// redrawn, and the one after that (if the sequence has not halted) will start
			//	// a new subsequence of calls where the canvas has been re-frozen 
			//	pApp->m_nInsertCount = 0;
			//	pView->canvas->Thaw();
			//	pApp->m_bIsFrozen = FALSE;
			//	// don't need a delay here
			//	if (pApp->m_nCurDelay == 31)
			//	{
			//		pApp->m_nCurDelay = 0; // set back to zero
			//	}
			//}
			// remove highlight before MessageBox call below
			//pView->Invalidate();
			pLayout->Redraw(); // bFirstClear is default TRUE
			pLayout->PlaceBox();

			// tell the user EOF has been reached
			m_bCameToEnd = TRUE;
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
			// wxWidgets version Hides the target box rather than destroying it
			this->Hide(); // whm added 12Sep04 // MFC version calls DestroyWindow
            this->GetTextCtrl()->ChangeValue(_T("")); // need to set it to null str since it won't get recreated
			pApp->m_pTargetBox->Enable(FALSE); // whm 12July2018 Note: It is re-enabled in ResizeBox()
			pApp->m_targetPhrase.Empty();
			pApp->m_nActiveSequNum = -1;

#ifdef _NEW_LAYOUT
			#ifdef _FIND_DELAY
				wxLogDebug(_T("4. Before RecalcLayout"));
			#endif
			pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
			#ifdef _FIND_DELAY
				wxLogDebug(_T("5. After RecalcLayout"));
			#endif
#else
			pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
			pApp->m_pActivePile = (CPile*)NULL; // can use this as a flag for at-EOF condition too
			pLayout->m_docEditOperationType = no_edit_op;

			RestorePhraseBoxAtDocEndSafely(pApp, pView);  // BEW added 8Sep14

		} // end of TRUE block for test: if (pApp->m_pActivePile == NULL || pApp->m_nActiveSequNum == -1)
		else // we have a non-null active pile defined, and sequence number >= 0
		{
            pApp->m_pTargetBox->SetFocusAndSetSelectionAtLanding(); // whm 13Aug2018 modified

            pLayout->m_docEditOperationType = no_edit_op; // is this correct for here?
		}

		// BEW 21May15, this is the second of three places in OnePass() where a Thaw() call
		// must be initiated when conditions are right; control passes through here when the
		// sequence of auto-inserts is just coming to a halt location - we want a Thaw() done
		// in that circumstance, regardless of the m_nInsertCount value, and the latter
		// reset to default zero
		// whm 11Jun2022 removed the support for freezing the canvas window. It wasn't working
		// correctly, and there is no need for it since the dropdown phrasebox was introduced.
		//if (pApp->m_bSupportFreeze && pApp->m_bIsFrozen)
		//{
		//	pApp->m_nInsertCount = 0;
		//	pView->canvas->Thaw();
		//	pApp->m_bIsFrozen = FALSE;
		//	// Don't need a third-of-a-second delay 
		//	if (pApp->m_nCurDelay == 31)
		//	{
		//		pApp->m_nCurDelay = 0; // set back to zero
		//	}
		//}
        // whm 19Feb2018 modified. Don't empty the global m_Translation string before PlaceBox() call
        // below. The PlaceBox may need it for PopulateDropDownList() after which m_Translation.Empty()
        // will be called there.
		//m_Translation.Empty(); // clear the static string storage for the translation (or gloss)
		// save the phrase box's text, in case user hits SHIFT+END to unmerge a phrase
        m_Translation = m_Translation; // for debugging
        m_SaveTargetPhrase = pApp->m_targetPhrase;
		pApp->m_bAutoInsert = FALSE; // ensure we halt for user to type translation
		pView->Invalidate(); // added 1Apr09, since we return at next line
		pLayout->PlaceBox();

		return FALSE; // must have been a null string, or at EOF;
	} // end of TRUE block for test: if (!bSuccessful)

	// if control gets here, all is well so far, so set up the main window
	//pApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);
	#ifdef _FIND_DELAY
		wxLogDebug(_T("6. Before ScrollIntoView"));
	#endif
	pLayout->m_pCanvas->ScrollIntoView(pApp->m_nActiveSequNum);
	#ifdef _FIND_DELAY
		wxLogDebug(_T("7. After ScrollIntoView"));
	#endif

	// BEW 21May15, this is the final of the three places in OnePass() where a Thaw() call
	// must be initiated when conditions are right; control passes through here when the
	// sequence of auto-inserts has not yet come to a halt location - we want a Thaw() if
	// periodically, breaking up the auto-insert sequence with a single redraw when
	// m_nInsertCount reaches the NUMINSERTS value for each subsequence
	// whm 11Jun2022 removed the support for freezing the canvas window. It wasn't working
	// correctly, and there is no need for it since the dropdown phrasebox was introduced.
	//if (pApp->m_bSupportFreeze && pApp->m_bIsFrozen)
	//{
	//	// Are we at the penultimate OnePass() call being completed? If so, we want the
	//	// Thaw() done, and the next call of OnPass() will then get the view redrawn, and
	//	// the one after that (if the sequence has not halted) will start a new subsequence
	//	// of calls where the canvas has been re-frozen 
	//	if (pApp->m_nInsertCount > 0 && ((pApp->m_nInsertCount + 1) % (int)NUMINSERTS == 0))
	//	{
	//		// Ask for the thaw of the canvas window
	//		pApp->m_nInsertCount = 0;
	//		pView->canvas->Thaw();
	//		pApp->m_bIsFrozen = FALSE;
	//		// Give user a 1-second delay in order to get user visually acquainted with the inserted words
	//		if (pApp->m_nCurDelay == 0)
	//		{
	//			// set delay of 31 ticks, it's unlikely to be accidently set to that value;
	//			// it's 31/100 of a second
	//			pApp->m_nCurDelay = 31; 
	//		}
	//	}
	//}
	pLayout->m_docEditOperationType = relocate_box_op;
	pView->Invalidate(); // added 1Apr09, since we return at next line
	pLayout->PlaceBox();
	#ifdef _FIND_DELAY
		wxLogDebug(_T("8. End of OnePass"));
	#endif

	pApp->m_bUserDlgOrMessageRequested = FALSE;
	pApp->m_bUserHitEnterOrTab = FALSE;
	return TRUE; // all was okay
}

void CPhraseBox::RestorePhraseBoxAtDocEndSafely(CAdapt_ItApp* pApp, CAdapt_ItView *pView)
{
	CLayout* pLayout = pApp->GetLayout();
	int maxSN = pApp->GetMaxIndex();
	CPile* pEndPile = pView->GetPile(maxSN);
	wxASSERT(pEndPile != NULL);
	CSourcePhrase* pTheSrcPhrase = pEndPile->GetSrcPhrase();
	if (!pTheSrcPhrase->m_bRetranslation)
	{
		// BEW 5Oct15, Buka island. Added test for free translation mode here, because
		// coming to the end in free trans mode, putting the phrasebox at last pile
		// results in the current section's last pile getting a spurious new 1-pile
		// section created at the doc end. The fix is to put the phrasebox back at the
		// current pile's anchor, so that no new section is created
		if (pApp->m_bFreeTranslationMode)
		{
			// Find the old anchor pile, closest to the doc end; but just in case
			// something is a bit wonky, test that the last pile has a free translation -
			// if it doesn't, then it is save to make that the active location (and it
			// would become an empty new free translation - which should be free
			// translated probably, and then this function will be reentered to get that
			// location as the final phrasebox location
			CPile* pPile = pView->GetPile(maxSN);
			if (!pPile->GetSrcPhrase()->m_bHasFreeTrans)
			{
				pApp->m_nActiveSequNum = maxSN;
			}
			else
			{
				while (!pPile->GetSrcPhrase()->m_bStartFreeTrans)
				{
					pPile = pView->GetPrevPile(pPile);
					wxASSERT(pPile != NULL);
				}
				pApp->m_nActiveSequNum = pPile->GetSrcPhrase()->m_nSequNumber;
			}
			pApp->m_pActivePile = pPile;
		}
		else
		{
			// Normal mode, use the last pile as active location
			pApp->m_nActiveSequNum = maxSN;
			pApp->m_pActivePile = pEndPile;
		}
	}
	else
	{
		// The end pile is within a retranslation, so find a safe active location
		int aSafeSN = maxSN + 1; // initialize to an illegal value 1 beyond
									// maximum index, this will force the search
									// for a safe location to search backwards
									// from the document's end
		int nFinish = 0; // can be set to num of piles in the retranslation but
							// this will do, as the loop will iterate over them
		// The next call, if successful, sets m_pActivePile
		bool bGotSafeLocation =  pView->SetActivePilePointerSafely(pApp,
							pApp->m_pSourcePhrases,aSafeSN,maxSN,nFinish);
		if (bGotSafeLocation)
		{
			pApp->m_nActiveSequNum = aSafeSN;
			pTheSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
			wxASSERT(pTheSrcPhrase->m_nSequNumber == aSafeSN);
		}
		else
		{
			// failed to get a safe location, so use start of document
			pApp->m_nActiveSequNum = 0;
			pApp->m_pActivePile = pView->GetPile(0);
		}
	}
	wxString transln = pTheSrcPhrase->m_adaption;
	pApp->m_targetPhrase = transln;
    this->GetTextCtrl()->ChangeValue(transln);
	pApp->m_bAutoInsert = FALSE; // ensure we halt for user to type translation
#ifdef _NEW_LAYOUT
	pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
	pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif

    pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
    // whm 13Aug2018 reordered code here so that SetFocus() precedes the
    // SetSelection(length, length) call.
    this->GetTextCtrl()->SetFocus();
    int length = transln.Len();
    // whm 3Aug2018 Note: Below no 'select all' involved so no adjustment made.
    this->GetTextCtrl()->SetSelection(length, length);
    pLayout->m_docEditOperationType = no_edit_op;

	pView->Invalidate();
	pLayout->PlaceBox();

}

// This OnKeyUp function is called via the EVT_KEY_UP event in our CPhraseBox
// Event Table.
// OnKeyUp() is triggered after OnKeyDown() and OnChar() - if/when OnChar() is triggered.
// 
// BEW 13Apr10, no changes needed for support of doc version 5
// whm Note 15Feb2018 modified key handling so that ALL AltDown(), ShiftDown(), and ControlDown()
// events are now sent to OnSysKeyUp() for processing.
// whm 16Feb2018 Notes: OnKeyUp() currently does the following key handling:
// 1. Detects all AltDown(), all ShiftDown(), and all ControlDown() events (with key combinations) and
//    routes all processing of those events to the separate function OnSysKeyUp(), and returns 
//    without calling Skip() suppressing further handling after execution of OnSysKeyUp().
// 2. Detects ENTER, TAB, NUMPAD_ENTER and NUMPAD_TAB keys and processes them all alike to move forward to next hole.
// 3. Detects WXK_RIGHT and WXK_LEFT; if detected sets flags m_bAbandonable = FALSE, pApp->m_bUserTypedSomething = TRUE,
//    and m_bRetainBoxContents = TRUE, for use if auto-merge (OnButtonMerge) is called on a selection.
// 4. Detects WXK_F8 and if detected calls Adapt_ItView::ChooseTranslation() then return to suppress further handling.
// Note: Put in this OnKeyUp() handler custom key handling that does not involve simultaneous use 
// of ALT, SHIFT, or CTRL keys  
// BEW 14Apr20 changes: The above note has these exceptions, Shift+Tab and Shift+Enter; because
// trying to put these in OnSysKeyUp() runs foul of VisualStudio trapping them first, so that
// Shift+Enter is the shortcut for VisualStudio to insert a blank line - and that makes the
// whole file of code (PhraseBox.cpp) go "stale" - and the debug run cannot continue; and if 
// its a Unicode Release build, then these two shortcuts just do nothing.  So these two have
// to be in OnKeyUp() - and prior to the test which send control to OnSysKeyUp(). So...
// 6. Detects SHIFT+ENTER to call MoveToPrevPile() so as to move the phrasebox back one location
// 7. Detects SHIFT+TAB to do same thing as SHIFT+ENTER
void CPhraseBox::OnKeyUp(wxKeyEvent& event)
{
    // wxLogDebug(_T("In CPhraseBox::OnKeyUp() key code: %d"), event.GetKeyCode()); 

    CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
    CLayout* pLayout = GetLayout();
    CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));

    // Note: wxWidgets doesn't have a separate OnSysKeyUp() virtual method (like MFC did)
    // so we'll simply detect if the ALT key or Shift key was down modifying another key stroke
    // and call the OnSysKeyUp() method from here. The call of OnSysKeyUp() below for AltDown,
    // ShiftDown and ControlDown events needs to be located here at the beginning of the 
    // OnKeyUp() handler.
    // Notes: 
    // Pressing ALT alone doesn't trigger OnKeyUp() - only ALT + some other key combo.
    // Pressing SHIFT alone doesn't trigger OnKeyUp() - only SHIFT + some other key combo.
    // Pressing CTRL alone doesn't trigger OnKeyUp() - only CTRL + some other key combo.
    // CmdDown() is same as ControlDown on PC, and Apple Command key on Macs.
    // whm 16Feb2018 moved all AltDown() ShiftDown() and ControlDown() processing to OnSysKeyUp()
    if (event.AltDown() || event.ShiftDown() || event.ControlDown())
    {
        OnSysKeyUp(event);
        return;
    }

    // whm 1Jun2018 Moved WXK_TAB key handling from OnChar() to here in OnKeyUp(), since a tab key
    // press does not trigger OnChar(). To make Tab work exactly like Enter, we need to ensure that
    // other things done in OnChar() prior to handling WXK_TAB (and WXK_ENTER) are also here too.
    // This includes detecting if there is a selection, and doing the merger for the user on the first
    // character typed - in this case the character is Tab. We put the code for merging a selection
    // within the WXK_TAB handling block in this case, since we are specifically dealing with just
    // the Tab key event here, whereas in OnChar() it merges a selection for any key press that
    // triggers OnChar().
    // whm 24June2018 combined WXK_TAB and WXK_RETURN handling in the same block of code below.
    // The WXK_RETURN key handling was previously located in OnChar(), but that seemed to not work
    // on the Mac OSX platform (where the OnChar handler might not be called for WXK_RETURN).
    int keycode = event.GetKeyCode();
    if (keycode == WXK_TAB             // TAB
        || keycode == WXK_RETURN       // ENTER
        || keycode == WXK_NUMPAD_ENTER // NUMPAD_ENTER
        || keycode == WXK_NUMPAD_TAB)  // NUMPAD_TAB  //whm 5Jul2018 added for extended keyboard numpad ENTER and numpad TAB users
    {
        // whm 16May2020 moved the code for handling bogus Enter key presses causing
        // phrasebox run-on situation, from where BEW placed it below, up here to the beginning 
        // of the block detecting ENTER and TAB key presses. The Reason: To avoid executing the
        // auto-merging code below and the pView->RemoveSelection() call below that can cause
        // problems if a phrasebox run-on situation exists. Up here we deal with the run-on
        // situation by immediately calling return from this OnKeyUp() handler.
        pApp->m_bUserHitEnterOrTab = FALSE; // Initialize to FALSE, as if user mouse-clicked
        if (keycode == WXK_TAB) // TAB
        {
        	pApp->m_bUserHitEnterOrTab = TRUE;
        	wxLogDebug(_T("CPhraseBox::OnKeyUp() handling WXK_TAB key"));
        }
        else if (keycode == WXK_RETURN) // ENTER or RETURN
        {
        	pApp->m_bUserHitEnterOrTab = TRUE;
        	wxLogDebug(_T("CPhraseBox::OnKeyUp() handling WXK_RETURN key"));
        }
        if (pApp->m_bUserDlgOrMessageRequested && pApp->m_bUserHitEnterOrTab)
        {
            // set both global flags back to FALSE so next legit ENTER will get processed.
            pApp->m_bUserDlgOrMessageRequested = FALSE; 
            pApp->m_bUserHitEnterOrTab = FALSE;
        	// Just return from OnKeyUp(), because it is likely this is a bogus box run-on situation
            wxLogDebug(_T("Bogus Enter key press detected - exiting prematurely from CPhraseBox::OnKeyUp()"));
        	return;
        }

        // whm 15Jul2018 added code here in the Enter/Return/Tab key handling to determine if
        // the Enter/Return/Tab key press was done AFTER the user highlighted a different
        // item in the dropdown list, i.e., different from the text that was originally in 
        // the phrasebox's edit box when the phrasebox landed at its current location. If 
        // a different item has been highlighted using Up/Down arrows the Enter/Return/Tab
        // press should be interpreted as a selection action to put the new/different item
        // into the phrasebox's edit box, and stay at that location, rather than a signal 
        // to move the phrasebox forward to another location. If, after moving the highlight
        // with Up/Down arrow keys, the highlighted item's text value is the same as the
        // text that was in the edit box when the phrasebox landed at that location, the
        // Enter/Return/Tab key press should be interpreted as the signal to accept the
        // text within the edit box as the form of target text that should be put at that
        // location and move the phrasebox forward.
        if (bUp_DownArrowKeyPressed)
        {
            // The user moved the highlight up/down during the editing session at this
            // location. We get the string selection that is currently highlighted and
            // compare it to the contents of the edit box. If they are they are different
            // we interpret the Enter/Tab press as a selection action, put the newly
            // highlighted list item into the phrasebox's edit box and stay put.

            // Note that a list item may be "<no adaptation>" or not adjusted for case 
            // or FwdSlashtoZWSP considerations. We need to compare apples with apples here.
            // The function GetListItemAdjustedforPhraseBox(FALSE) makes the adjustment 
            // to the list item's string - for accurate comparison with the phrasebox 
            // contents (which has already been adjusted). The FALSE parameter is used here
            // since we are only comparing the list item string with the phrasebox content,
            // and not assigning the list item to the phrasebox as is done in the
            // OnListBoxItemSelected() handler where GetListItemAdjustedforPhraseBox(TRUE)
            // is used.
            if (this->GetListItemAdjustedforPhraseBox(FALSE) != this->GetTextCtrl()->GetValue())
            {
                // The selected item string is different from what is in the phrasebox's
                // edit box, so we interpret the Enter/Tab key press to put the current
                // list selection into the phrasebox and stay put.
                // We can just call the OnListBoxItemSelected() handler here as it
                // does exactly what we need.
                wxCommandEvent dummyevent;
                pApp->GetMainFrame()->canvas->OnListBoxItemSelected(dummyevent);
                // Note: The above OnListBoxItemSelected() call adjusts for <no adaptation> and 
                // for case before putting the string selection into the phrasebox.
                // 
                // Reset the bUp_DownArrowKeyPressed flag in case the user makes yet another
                // selection from the list to change the previous selection.
                bUp_DownArrowKeyPressed = FALSE;
                // We don't want to process the Enter/Tab key press to JumpForward()
                // so just return here.
                return;
            }
            // If we get here the string selected in the list was identical to the string
            // currently in the phrasebox's edit box, so proceed with the normal Enter/Tab
            // key processing below (to JumpForward).
        }
        
        // First handle merging of any selection - as is done in OnChar().
        // preserve cursor location, in case we merge, so we can restore it afterwards
        long nStartChar;
        long nEndChar;
        GetSelection(&nStartChar, &nEndChar); //GetTextCtrl()->GetSelection(&nStartChar, &nEndChar);

        // whm Note: See note below about meeding to move some code from OnChar() to the
        // OnPhraseBoxChanged() handler in the wx version, because the OnChar() handler does
        // not have access to the changed value of the new string within the control reflecting
        // the keystroke that triggers OnChar().
        //
        wxSize textExtent;
        pApp->RefreshStatusBarInfo();

        // if there is a selection, and user forgets to make the phrase before typing, then do it
        // for him on the first character typed. But if glossing, no merges are allowed.

        int theCount = pApp->m_selection.GetCount();
        if (!gbIsGlossing && theCount > 1 && (pApp->m_pActivePile == pApp->m_pAnchor->GetPile()
            || IsActiveLocWithinSelection(pView, pApp->m_pActivePile)))
        {
            if (pView->GetSelectionWordCount() > MAX_WORDS)
            {
                pApp->GetRetranslation()->DoRetranslation();
            }
            else
            {
                if (!pApp->m_bUserTypedSomething &&
                    !pApp->m_pActivePile->GetSrcPhrase()->m_targetStr.IsEmpty())
                {
                    pApp->m_bSuppressDefaultAdaptation = FALSE; // we want what is already there
                }
                else
                {
                    // for version 1.4.2 and onwards, we will want to check m_bRetainBoxContents
                    // and two other flags, for a click or arrow key press is meant to allow
                    // the deselected source word already in the phrasebox to be retained; so we
                    // here want the m_bSuppressDefaultAdaptation flag set TRUE only when the
                    // m_bRetainBoxContents is FALSE (- though we use two other flags too to
                    // ensure we get this behaviour only when we want it)
                    if (m_bRetainBoxContents && !m_bAbandonable && pApp->m_bUserTypedSomething)
                    {
                        pApp->m_bSuppressDefaultAdaptation = FALSE;
                    }
                    else
                    {
                        pApp->m_bSuppressDefaultAdaptation = TRUE; // the global boolean used for temporary
                                                                   // suppression only
                    }
                }
                pView->MergeWords(); // simply calls OnButtonMerge
                                     // 
                                     //
                pLayout->m_docEditOperationType = merge_op;
                pApp->m_bSuppressDefaultAdaptation = FALSE;

                // we can assume what the user typed, provided it is a letter, replaces what was
                // merged together, but if tab or return was typed, we allow the merged text to
                // remain intact & pass the character on to the switch below; but since v1.4.2 we
                // can only assume this when m_bRetainBoxContents is FALSE, if it is TRUE and
                // a merge was done, then there is probably more than what was just typed, so we
                // retain that instead; also, we we have just returned from a MergeWords( ) call in
                // which the phrasebox has been set up with correct text (such as previous target text
                // plus the last character typed for an extra merge when a selection was present, we
                // don't want this wiped out and have only the last character typed inserted instead,
                // so in OnButtonMerge( ) we test the phrasebox's string and if it has more than just
                // the last character typed, we assume we want to keep the other content - and so there
                // we also set m_bRetainBoxContents
                m_bRetainBoxContents = FALSE; // turn it back off (default) until next required
            }
        }
        else
        {
            // if there is a selection, but the anchor is removed from the active location, we
            // don't want to make a phrase elsewhere, so just remove the selection. Or if
            // glossing, just silently remove the selection - that should be sufficient alert
            // to the user that the merge operation is unavailable
            pView->RemoveSelection();
            wxClientDC dC(pLayout->m_pCanvas);
            pView->canvas->DoPrepareDC(dC); // adjust origin
            pApp->GetMainFrame()->PrepareDC(dC); // wxWidgets' drawing.cpp sample also calls
                                                 // PrepareDC on the owning frame
            pLayout->m_docEditOperationType = no_edit_op;
            pView->Invalidate();
        }

        //BEW changed 01Aug05. Some users are familiar with using TAB key to advance
        // (especially when working with databases), and without thinking do so in Adapt It
        // and expect the Lookup process to take place, etc - and then get quite disturbed
        // when it doesn't happen that way. So for version 3 and onwards, we will interpret
        // a TAB keypress as if it was an ENTER keypress
		// BEW 26Mar20 added code to prevent immediate run-on by the phrasebox when the user
		// selects the Enter or Tab key to get the default action - such as when inserting
		// a placeholder, Choose Translation, Guesser dialog, and maybe others (but not all).
		// Whether in Drafting or Reviewing mode, without the protection below, then an Enter
		// or Tab key press would cause run-on the next hole, or next pile, respectively. 
         
        // Normal jump is okay to do
        JumpForward(pView);
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
    // whm 16Feb2018 verified the following two WXK_RIGHT and WXK_LEFT execute.
    // whm TODO: Although these flags are set, the OnButtonMerge() doesn't seem to 
    // preserve the phrasebox's contents
	if (event.GetKeyCode() == WXK_RIGHT) // RIGHT arrow key
	{
		m_bAbandonable = FALSE;
		pApp->m_bUserTypedSomething = TRUE;
        m_bRetainBoxContents = TRUE;
		event.Skip();
	}
	else if (event.GetKeyCode() == WXK_LEFT) // LEFT arrow key
	{
		m_bAbandonable = FALSE;
		pApp->m_bUserTypedSomething = TRUE;
        m_bRetainBoxContents = TRUE;
		event.Skip();
	}

//#if defined(_DEBUG)
//	wxChar typedChar = (wxChar)event.GetKeyCode();
//	wxLogDebug(_T("In OnKeyUp - key = %c"), typedChar);
//#endif
//

	// does the user want to force the Choose Translation dialog open?
	// whm Note 12Feb09: This F8 action is OK on Mac (no conflict)
	if (event.GetKeyCode() == WXK_F8) // F8 function key - ChooseTranslation dialog
	{
		// whm added 26Mar12
		if (pApp->m_bReadOnlyAccess)
		{
			// Disable the F8 key invocation of the ChooseTranslation dialog
			return;
		}
		pView->ChooseTranslation();
		return;
	}
   
	event.Skip();
} // end of OnKeyUp()

// This OnKeyDown function is called via the EVT_KEY_DOWN event in our CPhraseBox
// Event Table. 
// OnKeyDown() is the first key handler generated, and is triggered for ALL key strokes.
//
// WARNING: OnKeyDown() is called REPEATEDLY when a key is held DOWN!!!
// 
// The wx docs say: "If a key down (EVT_KEY_DOWN) event is caught and the event handler 
// does not call event.Skip() then the corresponding char event (EVT_CHAR) will not happen. 
// This is by design and enables the programs that handle both types of events to avoid 
// processing the same key twice. As a consequence, if you do NOT want to suppress the 
// wxEVT_CHAR events for the keys you handle, always call event.Skip() in your 
// wxEVT_KEY_DOWN handler. Not doing so may also prevent accelerators defined using this 
// key from working... For Windows programmers: The key and char events in wxWidgets are 
// similar to but slightly different from Windows WM_KEYDOWN and WM_CHAR events. In particular, 
// Alt-x combination will generate a char event in wxWidgets (unless it is used as an accelerator) 
// and almost all keys, including ones without ASCII equivalents, generate char events too."
//
// whm 16Feb2018 Notes: OnKeyDown() currently does the following key handling:
// 1. Sets m_bALT_KEY_DOWN to TRUE when WXK_ALT is detected
// 2. When m_bReadOnlyAccess is TRUE, beeps for all alphabetic chars, and returns without calling Skip() suppressing further handling.
// 3. Sets App's m_bAutoInsert to FALSE if it was TRUE, and returns without calling Skip() suppressing further handling.
// 4. Calls RefreshStatusBarInfo().
// 5. Handles WXK_DELETE and calls Skip() for DELETE to be handled in further processing.
// 6. Handles WXK_ESCAPE (to restore phrasebox content after guess) and returns without calling Skip() suppressing further handling.
// BEW 13Apr10, no changes needed for support of doc version 5
void CPhraseBox::OnKeyDown(wxKeyEvent& event)
{
    //wxLogDebug(_T("In CPhraseBox::OnKeyDown() key code: %d"), event.GetKeyCode());

    // refactored 2Apr09
	//wxLogDebug(_T("OnKeyDown() %d called from PhraseBox"),event.GetKeyCode());
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));

	// BEW 31Jul16 added, to track ALT key down or released
    if (!pApp->m_bReadOnlyAccess)
	{
		int keyCode = event.GetKeyCode();
		if (keyCode == WXK_ALT)
		{
			pApp->m_bALT_KEY_DOWN = TRUE;
		}
		// continue on to do any further processing; for the above variable
        // whm 16Feb2018 The App global m_bALT_KEY_DOWN is now reset FALSE at end of DoSrcPhraseSelCopy().
	}

    // whm added 15Mar12. When in read-only mode don't register any key strokes
	if (pApp->m_bReadOnlyAccess)
	{
		// return without calling Skip(). Beep for read-only feedback
		int keyCode = event.GetKeyCode();
		if (keyCode == WXK_DOWN || keyCode == WXK_UP || keyCode == WXK_LEFT || keyCode == WXK_RIGHT
			|| keyCode == WXK_PAGEUP || keyCode == WXK_PAGEDOWN
			|| keyCode == WXK_CONTROL || keyCode == WXK_ALT || keyCode == WXK_SHIFT
			|| keyCode == WXK_ESCAPE || keyCode == WXK_TAB || keyCode == WXK_BACK
			|| keyCode == WXK_RETURN || keyCode == WXK_DELETE || keyCode == WXK_SPACE
			|| keyCode == WXK_HOME || keyCode == WXK_END || keyCode == WXK_INSERT
			|| keyCode == WXK_F8)
		{
			; // don't beep for the various keys above
		}
		else
		{
			::wxBell();
		}
		// don't pass on the key stroke - don't call Skip()
		return;
	}

	if (!pApp->m_bSingleStep)
	{
		// halt the auto matching and inserting, if a key is typed
		if (pApp->m_bAutoInsert)
		{
			pApp->m_bAutoInsert = FALSE;
			// Skip() should not be called here, because we can ignore the value of
			// any keystroke that was made to stop auto insertions.
			return;
		}
	}

//#if defined(_DEBUG)
//	wxChar typedChar = (wxChar)event.GetKeyCode();
//	wxLogDebug(_T("In OnKeyDown - key = %c"),typedChar);
//#endif
//

	// update status bar with project name
	pApp->RefreshStatusBarInfo();

	//event.Skip(); //CEdit::OnKeyDown(nChar, nRepCnt, nFlags);

	// [MFC Note] Delete key sends WM_KEYDOWN message only, WM_CHAR not sent
	// so we need to update things for forward deletions here (and it has to be
	// done after the call to the base class, otherwise the last deleted character
	// remains in the final target phrase text)
	// BEW added more on 20June06, so that a DEL keypress in the box when it has a <no adaptation>
	// KB or GKB entry will effect returning the CSourcePhrase instance back to a true "empty" one
	// (ie. m_bHasKBEntry or m_bHasGlossingKBEntry will be cleared, depending on current mode, and
	// the relevant KB's CRefString decremented in the count, or removed entirely if the count is 1.)
	// [wxWidget Note] In contrast to Bruce's note above OnKeyDown() Delete key does trigger
	// OnChar() in wx key handling.
	CPile* pActivePile = pView->GetPile(pApp->m_nActiveSequNum); // doesn't rely on pApp->m_pActivePile
																 // having been set already
	CSourcePhrase* pSrcPhrase = pActivePile->GetSrcPhrase();
	if (event.GetKeyCode() == WXK_DELETE)  // DELETE
	{
		if (pSrcPhrase->m_adaption.IsEmpty() && ((pSrcPhrase->m_bHasKBEntry && !gbIsGlossing) ||
			(pSrcPhrase->m_bHasGlossingKBEntry && gbIsGlossing)))
		{
			m_bNoAdaptationRemovalRequested = TRUE;
			wxString emptyStr = _T("");
			if (gbIsGlossing)
				pApp->m_pGlossingKB->GetAndRemoveRefString(pSrcPhrase,emptyStr,useGlossOrAdaptationForLookup);
			else
				pApp->m_pKB->GetAndRemoveRefString(pSrcPhrase,emptyStr,useGlossOrAdaptationForLookup);
		}
		else
		{
			// BEW 31Jul18, next four lines added so we can get a value into the global gnBoxCursorOffset,
			// which the target_box_paste_op will cause to be used by SetCursorGlobals, to keep the cursor
			// where it is while the forward deletions are being done; as the switch in PlaceBox calls
			// SetupCursorGlobals(m_pApp->m_targetPhrase, cursor_at_offset, gnBoxCursorOffset); to do the
			// job and so gnBoxCursorOffset must first be correctly set
			long from;
			long to;
			pApp->m_pTargetBox->GetSelection(&from, &to);
			gnBoxCursorOffset = (int)from;

			// legacy behaviour: handle the forward character deletion
			m_backspaceUndoStr.Empty();
			m_nSaveStart = -1;
			m_nSaveEnd = -1;

			wxString s;
			s = GetValue();
			pApp->m_targetPhrase = s; // otherwise, deletions using <DEL> key are not recorded
            if (!this->IsModified()) 
            {
                // whm added 10Jan2018 The SetModify() call below is needed for the __WXGTK__ port
                // For some unknown reason the delete key deleting the whole phrasebox contents
                // does not set the dirty flag without this.
                this->SetModify(TRUE);
            }
		}
	}
	else if (event.GetKeyCode() == WXK_ESCAPE)
	{
		// user pressed the Esc key. If a Guess is current in the phrasebox we
		// will remove the Guess-highlight background color and the guess, and
		// restore the normal copied source word to the phrasebox. We also reset
		// the App's m_bIsGuess flag to FALSE. If a Guess isn't current in the
		// phrase box the ESC key will restore the phrasebox content to the
		// content is had a phrasebox landing (at the Layout's PlaceBox() call).
		if (this->GetBackgroundColour() == pApp->m_GuessHighlightColor)
		{
			// get the pSrcPhrase at this active location
			CSourcePhrase* pSrcPhrase;
			pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
			//wxString str = pSrcPhrase->m_key; // BEW 27Bov14 deprecated, in favour of using the 
			// stored pre-guesser version of the string, because it may have been modified
			wxString str = pApp->m_preGuesserStr; // use this as the default restore string

			// It was Roland Fumey in 16July08 that wanted strings other than the source text
			// to be used for the restoration, if pApp->m_bLegacySourceTextCopy was not in effect, so
			// keep the following as it is the protocol he requested
			if (!pApp->m_bLegacySourceTextCopy)
			{
				// the user wants smart copying done to the phrase box when the active location
				// landed on does not have any existing adaptation (in adapting mode), or, gloss
				// (in glossing mode). In the former case, it tries to copy a gloss to the box
				// if a gloss is available, otherwise source text used instead; in the latter case
				// it tries to copy an adaptation as the default gloss, if an adaptation is
				// available, otherwise source text is used instead
				if (gbIsGlossing)
				{
					if (!pSrcPhrase->m_adaption.IsEmpty())
					{
						str = pSrcPhrase->m_adaption;
					}
				}
				else
				{
					if (!pSrcPhrase->m_gloss.IsEmpty())
					{
						str = pSrcPhrase->m_gloss;
					}
				}
			}
            this->GetTextCtrl()->ChangeValue(str);
			pApp->m_targetPhrase = str; // Required, otherwise the guess persists and gets 
										// used in auto-inserts subsequently, if turned on
			this->SetBackgroundColour(wxColour(255,255,255)); // white;
#if defined (ABANDON_NOT)
			this->m_bAbandonable = FALSE;
#else
			this->m_bAbandonable = TRUE;
#endif
			pApp->m_bIsGuess = FALSE;
			pApp->m_preGuesserStr.Empty(); // clear this to empty, it's job is done
			this->Refresh();
			return;
		}
        else
        {
            // The Esc key was pressed in order to effect an undo - that is restore the original
            // value to the phrasebox that it had on landing at the current location. This allows 
            // the user to change the phrasebox back to its original content after doing edits,
            // or after selecting a list item that was not intended, etc.
            this->GetTextCtrl()->ChangeValue(initialPhraseBoxContentsOnLanding); // GetTextCtrl() gets m_pTargetBox
			this->SetFocusAndSetSelectionAtLanding();
        }
	}
    // whm 13Jul2018 added handlers for WXK_DOWN and WXK_UP in order for those arrow keys
    // to be able to move the highlight within the new phrasebox's dropdown list when pressed
    // from within the focused edit box. See the SHIFT+SPACE block below for how a user can
    // select the highlighted item into the phrasebox's edit box.
    else if (event.GetKeyCode() == WXK_DOWN) // DOWN ARROW
    {
        // DOWN arrow key was pressed while focus is in the phrasebox's edit box.

        // whm 15Jul2018 added the following bool, set to TRUE here, so we can detect
        // if user highlighted a different dropdown list item while at this location.
        // The bUp_DownArrowKeyPressed value is reset to FALSE in PlaceBox().
        // It is used within the WXK_RETURN, WXK_TAB ... handler in OnKeyUp() to
        // interpret whether the Enter/Tab key press should select the different item 
        // from the dropdown list, or just function to move the phrasebox forward.
        bUp_DownArrowKeyPressed = TRUE;

        // We change the highlight to the next lower item in the list if there are > 1 
        // list items in the list and the highlight is not on the last item. 
        // If the highlighted is on the last item, we move the highlight to the first 
        // item in the list.
        // If the list is closed (and there are > 1 list items, we open the list and
        // highlight the first item.
        int itemSel;
        itemSel = this->GetDropDownList()->GetSelection();
        int numItemsInList = this->GetDropDownList()->GetCount();
        if (this->GetDropDownList()->IsShown())
        {
            if (numItemsInList > 1)
            {
                if (itemSel == numItemsInList - 1)
                {
                    // selected item is last in the list, move selection to top item
                    this->GetDropDownList()->SetSelection(0, TRUE);
                }
                else
                {
                    // move highlight to next lower item in list
                    this->GetDropDownList()->SetSelection(itemSel+1, TRUE);
                }
            }
        }
        else
        {
            // list is hidden/closed, open it and highlight first item in list
            if (numItemsInList > 1)
            {
                this->GetDropDownList()->Show();
                this->GetDropDownList()->SetSelection(0, TRUE);
            }
        }
        // Dont call event.Skip() but return here - we don't want the arrow key press to
        // be processed downstream (it would move the insertion point within the editbox 
        // text, while at the same time moving the highlight).
        return;
    }
    else if (event.GetKeyCode() == WXK_UP) // UP ARROW
    {
        // UP arrow key was pressed while focus is in the phrasebox's edit box.

        // whm 15Jul2018 added the following bool, set to TRUE here, so we can detect
        // if user highlighted a different dropdown list item while at this location.
        // The bUp_DownArrowKeyPressed value is reset to FALSE in PlaceBox().
        // It is used within the WXK_RETURN, WXK_TAB ... handler in OnKeyUp() to
        // interpret whether the Enter/Tab key press should select the different item 
        // from the dropdown list, or just function to move the phrasebox forward.
        bUp_DownArrowKeyPressed = TRUE;

        // We change the highlight to a higher item in the list if there are > 1 list items
        // in the list. If the highlighted item in the list is the last item, we move the
        // highlight to the first item in the list.
        int itemSel;
        itemSel = this->GetDropDownList()->GetSelection();
        int numItemsInList = this->GetDropDownList()->GetCount();
        if (this->GetDropDownList()->IsShown())
        {
            if (numItemsInList > 1)
            {
                if (itemSel == 0)
                {
                    // selected item is first in the list, move selection to bottom item
                    this->GetDropDownList()->SetSelection(numItemsInList - 1, TRUE);
                }
                else
                {
                    // move highlight to next higher item in list
                    this->GetDropDownList()->SetSelection(itemSel - 1, TRUE);
                }
            }
        }
        else
        {
            // list is hidden/closed, open it and highlight last item in list
            if (numItemsInList > 1)
            {
                this->GetDropDownList()->Show();
                this->GetDropDownList()->SetSelection(itemSel - 1, TRUE);
            }
        }
        // Dont call event.Skip() but return here - we don't want the arrow key press to
        // be processed downstream (it would move the insertion point within the editbox 
        // text, while at the same time moving the highlight).
        return;
    }

    // whm 13Jul2018 added a new handler for SHIFT+WXK_SPACE to provide a keyboard 
    // method of selecting a highlighted item - putting it into the phrasebox's edit box.
    else if (event.GetKeyCode() == WXK_SPACE) // SHIFT+SPACE - select highlighted item from list into edit box.
    {
        if (event.ShiftDown())
        {
            // SHIFT+SPACE key combo was pressed. This is keyboard way to select the highlighted
            // item in list into the phrasebox's edit box. It has the same effect as clicking on 
            // an item in the list with a mouse to select the item into the phrasebox's edit box.
            // For safety sake we only do the selection if the dropdown list is shown and there are
            // > 1 items in the list.
            int itemSel;
            itemSel = this->GetDropDownList()->GetSelection();
            int numItemsInList = this->GetDropDownList()->GetCount();
            if (this->GetDropDownList()->IsShown())
            {
                if (numItemsInList > 1)
                {
                    if (itemSel != -1)
                    {
                        wxCommandEvent dummyevent;
                        pApp->GetMainFrame()->canvas->OnListBoxItemSelected(dummyevent);
                    }
                }
            }
            // don't call event.Skip() - prevent further processing downstream - just return
            return;
        }
    }
    
	event.Skip(); // allow processing of the keystroke event to continue
}

void CPhraseBox::ClearDropDownList()
{
    this->GetDropDownList()->Clear();
}

void CPhraseBox::SetModify(bool modify)
{
    // Note: whm 14Feb2018 added this->GetTextCtrl->
	if (modify)
        this->GetTextCtrl()->MarkDirty(); // "mark text as modified (dirty)"
	else
        this->GetTextCtrl()->DiscardEdits(); //this->GetTextCtrl()->DiscardEdits(); // "resets the internal 'modified' flag
						// as if the current edits had been saved"
}

bool CPhraseBox::GetModify()
{
    // Note: whm 14Feb2018 added this->GetTextCtrl->
    return this->GetTextCtrl()->IsModified();
}

// This function retrieves the best pixel "tab stop" from the CPhraseBox::arrayTabPositionsInPixels array.
// The "tab stop" returned becomes the extent or width of the phrasebox's edit box, which is equivalent to
// and lines up with the extent of the Layout's m_curBoxWidth (i.e., the right side of the phrasebox).
// This function returns a -1 value if it detects that there is no need for a phrasebox resize.
// The arrayTabPositionsInPixels array is populated within the Layout's PlaceBox() function
// and contains pixel values for phrasebox pixel width "tabs". The first value in the array is always
// the tab that matches the phrasebox's edit box width in pixels at its landing (in Layout's PlaceBox())
// call. See the code comments near the end of Layout's PlaceBox() for a description of how the array 
// pixel tab values are generated for the phrasebox's content at its initial landing location.
// 
// This function is called in only one place - in the CPhraseBox::OnPhraseBoxChanged() function. 
// 
// This function uses the current phrasebox extent/width, and the current pixel extent value of
// the phrasebox's string content to analyze what has changed in the whitespace or slop at the end 
// of the box's content string. Using that analysis it determined whether the phrasebox needs 
// contracting or expanding. If a phrasebox size change is not needed, the function returns a value 
// of -1 to signal the caller that no phrasebox resize is needed.
// If the function detects that a phrasebox resize is needed, it grabs a pixel extent tab value from 
// the arrayTabPositionsInPixels array, and returns that value so the caller (OnPhraseBoxChanged) 
// can use that pixel extent tab stop as the new phrasebox width parameter when it calls the View's 
// ResizeBox() function.
// 
// Note: The spaceRemainingInBoxAtThisEdit value can suddenly be negative when the user has pasted a long segment 
// of text into the phrasebox at the time of the call to GetBestTabSettingFromArrayForBoxResize().
//
// whm Note: If the edit produces a longer content than what was established at landing, the box is
// expanding and we can calculate an editSpaceRemaining value as the difference between the 
// initialPixelTabPositionOnLanding and the boxContentPixelExtentAtThisEdit. if the editSpaceRemaining
// value gets down below about half the slop here within GetBestTabSettingFromArrayForBGoxResize()
// function, it should determine a tab stop value for a ResizeBox() call would result in a phrasebox
// size wide enough so that there is a minimum of one slop of white space after the new string
// content and the end of the box.
// If the edit produces a shorter content than what was established at landing, the box is potentially
// contracting. If the box can be made be made narrower by one or more multiples of the slop amount and
// yet still keeping at least one slop worth of editSpaceRemaining between the new content and the end
// of the box, as well as not getting narrower than the minimum width that CalcPhraseBoxWidth() would 
// produce, we determine the best tab stop value that fits within these resizing conditions.
// 
// Note: This GetBestTabSettingFromArrayForBoxResize() function will return a value every time
// it is called. If the box's content hasn't changed in length much, the value returned will be -1 to
// signal the caller that there need not be any change of phrasebox size from its size at landing. 
// A phrasebox resize is only indicated if the pixel tab value that is returned, is different from the 
// tab value/extent of the phrasebox at its initial landing.
int CPhraseBox::GetBestTabSettingFromArrayForBoxResize(int initialPixelTabPosition, int boxContentPixelExtentAtThisEdit)
{
	int newTabValue = initialPixelTabPosition;

	CLayout* pLayout = GetLayout();
	CAdapt_ItApp* pApp = &wxGetApp();

	// whm 11Nov2022 Note:
	// The Key to our phrasebox resizing routines is the calculated value spaceRemainingInBoxAtThisEdit. 
	// Calculating the spaceRemainingInBoxAtThisEdit value, however, is a bit tricky and needs to be
	// done here dynamically as a calculation of the instantaneous and actual width of the phrasebox's 
	// edit box, rather than using the value returned by the Pile's CalcPhraseBoxWidth() function. The
	// CalcPhraseBoxWidth() function gets the max width of phrasebox for the current content and any 
	// list widths in the dropdown list. It "guarantees that the width of the phrasebox (excluding 
	// buttonWidth, & the + 1 for spacing the button), agrees in width with the dropdown list's width";
	// and the latter is called by a calc function too which internally widens the list to make the 
	// widest list member wholely visible in the list, if the list exists. Moreover, the 
	// CalPhraseBoxWidth() function also includes the slop at the right end of the box, so the value
	// it returns won't work for calculating an accurate spaceRemainingInBoxAtThisEdit value. 
	// The returned value from CalcPhraseBoxWidth() is helpful as a minimum width and we must not 
	// allow any ResizeBox() operation to make the phrasebox smaller than that minimum width value.
	//
	// Get the instantaneous width of the phrasebox's edit box alone
	int phraseBoxHeight;
	int phraseBoxWidthAtThisEdit;
	pApp->m_pTargetBox->GetTextCtrl()->GetSize(&phraseBoxWidthAtThisEdit, &phraseBoxHeight); // it's the width we want

	// Note: While box text is expanding, the "running out" threshold for the space remaining is reached 
	// when there is only (3 * pApp->m_width_of_w) pixels or less of whitespace remaining in the box. 
	// At that threshold there is a need to resize the phrasebox wider.
	// While box text is shortening, the whitespace remaining is getting greater, and the threshold 
	// for "too much space remaining" is reached when there is greater than or equal to 
	// pLayout->slop + 3 * pApp->m_width_of_w pixels of white space remaining in the edit box.
	// 
	// Note: At the commencement of editing in the phrasebox, more than a slop equivalent of characters 
	// may be typed into the initial content string before a resize is called for, particularly when the 
	// phrasebox at that landing initially has few or no characters in it. This extra whitespace is present
	// because the phrasebox has a fairly wide minimum width that is calculated by the Pile's 
	// CalcPhraseBoxWidth() function. Hence, it will often be the case that the initial edit whitespace 
	// remaining in the phrasebox beyond its string content extent will be more than the default 
	// slop's 8 'w' character-widths of whitespace.
	// 
	// We test for the need for phrasebox expandion/contraction by examining the state of this 
	// whitespace now that an edit has occurred, comparing it with the whitespace at the beginning of 
	// the edit at the given location.
	int spaceRemainingInBoxAtThisEdit; 
	// The editSpaceRemaining value is the amount of slop white space that remains between end of the
	// content string and end of edit box at its current size (it may have expanded or contracted due 
	// to a previous edit). 
	// Note: In the calculation below, spaceRemainingInBoxAtThisEdit can become negative if the the 
	// user has pasted text in one go that makes the current phrasebox text extent greater than the 
	// current text extent width of the whole phrasebox.
	spaceRemainingInBoxAtThisEdit = phraseBoxWidthAtThisEdit - boxContentPixelExtentAtThisEdit;

	// The OnPhraseBoxChanged() function is called for any and all edits made within the phrasebox. 
	// It is also called by the Canvas' OnListBoxItemSelected() if the user selects an item from 
	// the dropdown list.
	// It is helpful to know if the phrasebox string content's extent is getting longer or shorter 
	// since the last call of OnPhraseBoxChanged().
	// A significant difference in this refactored code from its previous coding, is that the 
	// following bool contentStringIsExpanding is only used here within this function, so a local 
	// bool is all that is necessary. With the current phrasebox resizing code we don't need to 
	// keep track whether the phrasebox itself is expanding or contracting, allowing us to remove 
	// wider scoped bool values like m_bDoExpand and m_bDoContract, and we can eliminate 
	// pLayout->m_bCompareWidthIsLonger.
	bool contentStringIsExpanding = FALSE;
	if (boxContentPixelExtentAtThisEdit > oldBoxContentPixelExtentCached)
	{
		contentStringIsExpanding = TRUE;
	}

	int arrayTabTotal = (int)arrayTabPositionsInPixels.GetCount();
	int count;

	// If the content in the phrasebox is expanding/longer than it was at last edit, we need 
	// to check if there is sufficient whitespace at the end of the phrasebox to accommodate 
	// the expanded content. If not, we need to expand the phrasebox enough to accommodate the 
	// new content, and have a slop amount of whitespace available for more editing to take 
	// place.
	if (contentStringIsExpanding)
	{
		// The content is expanding.
		// Check if we are running out of whitespace and require a phrasebox expansion.
		// Note: spaceRemainingInBoxAtThisEdit at this point can be negative, for 
		// instance if the user pastes a long segment of test making the content extent 
		// significantly longer. White space "running out" is here defined as BEW did, 
		// to be <= 3 'w' widths of space left.
		// 
		// whm possible suggestion TODO: Seems to me that 3 'w' widths of extent is perhaps 
		// more than enough, perhaps we could try 2 'w' widths, or even 1 'w' width ???
		if (spaceRemainingInBoxAtThisEdit <= (3 * pApp->m_width_of_w))
		{
#if defined (_DEBUG)
			wxLogDebug(_T("Expanding - space remaining TOO LONG - needs resize"));
#endif
			// The content is expanding, but there is only 3 'w' extent widths or 
			// less available, so this condition triggers a resize the phrasebox to 
			// a tab unit that will accommodate the contents and new slop. The pixel 
			// tab value should be one or more pixel tab units larger if strip width
			// constraints allow it.
			// 
			// Note: The code below in this block detects when a phrasebox expansion 
			// should be triggered, for initial expansion and/or any subsequent 
			// expansions that might be necessary.
			// 
			// Grab the appropriate tab unit from the array and resize the box to 
			// that width value.
			int boxWidthNeededForNewString = boxContentPixelExtentAtThisEdit + pLayout->slop;
			// Initialize newTabValue to initialPixelTabPosition in case we don't find a new 
			// tab value in the array.
			newTabValue = initialPixelTabPosition; 
			for (count = 0; count < arrayTabTotal; count++)
			{
				int arrayIntValue = arrayTabPositionsInPixels.Item(count);
				// Note: The last item within the arrayTabPositionsInPixels functions as a 
				// maximum phrasebox right margin value, so we need not mess with testing 
				// for a maximum right margin. If the user were to type or paste enough text 
				// into the phrasebox so that the scan of the array below were to return the 
				// last array value, the phrasebox would be resized to take up a whole strip 
				// by itself. That's something not likely to be necessary, but it could happen. 
				// If that did happen, then the layout of strips would simply have the 
				// phrasebox at the active location being the only pile in that strip. Any 
				// additional text added to the phrasebox at that point would cause the 
				// phrasebox content to be pushed off to the left out of sight unless the 
				// user were to move the insertion point back to the beginning of the 
				// phrasebox (which would push the long text off the right end view). This 
				// situation would be better than having the phrasebox itself extend beyond 
				// the visible canvas.
				if (arrayIntValue >= boxWidthNeededForNewString)
				{
					newTabValue = arrayIntValue;
#if defined (_DEBUG)
					wxLogDebug(_T("Expanding - new size = %d"),newTabValue);
#endif
					break;
				}
			}
			// Note: Back in OnPhraseBoxChanged() if the returned newTabValue is not -1, we call the
			// CMainFrame's SendSizeEvent() which invokes the main frame's OnSize() handler. The frame's 
			// OnSize() handler in turn calls RecalcLayout(..., create_strips_keep_piles), Invalidate(), 
			// and PlaceBox() - and PlaceBox() internally calls ResizeBox().
		}
		else
		{
			// There is still sufficient white space for more editing while expanding, 
			// so do nothing, signaled by returning -1.
			newTabValue = -1;
#if defined (_DEBUG)
			wxLogDebug(_T("Expanding - no resize"));
#endif
		}
	}
	else // content is shortening (and whitespace at end of phrasebox is growing) 
	{
		// The content is shortening. 
		// Check if the spaceRemainingInBoxAtThisEdit is getting too long requiring a 
		// phrasebox contraction.
		// White space "getting too long" is here defined as >= the slop + 3 'w' 
		// widths of white space
		if (spaceRemainingInBoxAtThisEdit >= (pLayout->slop + 3 * pApp->m_width_of_w))
		{
#if defined (_DEBUG)
			wxLogDebug(_T("Shortening - space remaining TOO SHORT - needs resize"));
#endif
			// The content is shortening and whitespace is now >= the amount of slop 
			// plus 3 'w' widths, so, get a newTabValue for contracting the phrasebox. 
			// This value should be greater or equal to the extent needed for the 
			// current content string + slop, but also greater or equal to the greatest list 
			// item width, i.e., whichever has the greatest extent.
			int extentNeededForCurrentString = boxContentPixelExtentAtThisEdit;
			int extentNeededForlongestListItem;
			if (pLayout->m_curListWidth != -1)
				extentNeededForlongestListItem = pLayout->m_curListWidth;
			else
				extentNeededForlongestListItem = pLayout->m_curBoxWidth;
			// Initialize newTabValue to initialPixelTabPosition in case we don't find a new value in array.
			newTabValue = initialPixelTabPosition; 
			// Scan array for a new tab value to return to the caller.
			for (count = 0; count < arrayTabTotal; count++)
			{
				int arrayIntValue = arrayTabPositionsInPixels.Item(count);
				// The minimum phrasebox size value is the first tab value in the array which
				// was established at landing in PlaceBox(). The new tab has minimal 
				// constraints: We should contract the phrasebox to a width value that is 
				// greater or equal to the extentNeededForCurrentString + slop, AND is greater 
				// than or equal to the extentNeededForlongestListItem. I don't think we need
				// to consider the slop along with the extentNeededForlongestListItem.
				if (arrayIntValue >= extentNeededForCurrentString + pLayout->slop
					&& arrayIntValue >= extentNeededForlongestListItem) // + pLayout->slop)
				{					
					newTabValue = arrayIntValue;
#if defined (_DEBUG)
					wxLogDebug(_T("Shortening - new size = %d"), newTabValue);
#endif
					break;
				}
			}
		}
		else
		{
			// The whitespace isn't getting too long, so do nothing signaled by returning -1
			newTabValue = -1;
#if defined (_DEBUG)
			wxLogDebug(_T("Shortening - no resize"));
#endif
		}
		// Note: Back in OnPhraseBoxChanged() if the returned newTabValue is not -1, we call the
		// CMainFrame's SendSizeEvent() which invokes the main frame's OnSize() handler. The frame's 
		// OnSize() handler in turn calls RecalcLayout(..., create_strips_keep_piles), Invalidate(), 
		// and PlaceBox() - and PlaceBox() internally calls ResizeBox().
	}
	
	return newTabValue;
}

// whm 22Mar2018 Notes: 
// SetupDropDownPhraseBoxForThisLocation() is used once - near the end of the Layout's 
// PlaceBox() function. At that location in PlaceBox() the path of code execution may  
// often pass that point multiple times, especially when auto-inserting target entries. 
// To reduce the number of spurious calls of this function, it employs three boolean flags
// that prevent execution of the code within this function unless all three flags are FALSE.
// By allowing the execution path to do the dropdown setup and list population 
// operations below only when the App's bLookAheadMerge, m_bAutoInsert, and
// m_bMovingToDifferentPile flags are all FALSE, we are able to restrict code execution
// here to once (approximately) per final landing location of the phrasebox.
// WARNING: It is possible that the three boolean flags may not prevent all spurious
// calls of this function. Therefore, it is best to not put anything within this
// function that cannot be executed more than once, and still achieve the same desired
// result.
void CPhraseBox::SetupDropDownPhraseBoxForThisLocation()
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	//CLayout* pLayout = pApp->GetLayout();
#if defined (_DEBUG) && defined (_ABANDONABLE)
	//pApp->LogDropdownState(_T("SetupDropDownPhraseBoxForThisLocation() just now entered, line %d"), _T("PhraseBox.cpp"), __LINE__);
#endif

	int width = 0; // for use in the wxLogDebug call at function end
	int listWidth = 0;
	wxUnusedVar(width);
	wxUnusedVar(listWidth);

	// whm added 10Jan2018 to support quick selection of a translation equivalent.
	// The Layout's PlaceBox() function is always called just before the phrasebox 'rests' 
	// and can be interacted with by the user. Hence the location in PlaceBox() is the  
	// location in the application where we execute this function to set up the content 
	// and characteristics of the phrasebox's (m_pTargetBox) dropdown control:
	//
	// 1. Test for the App's bLookAheadMerge, m_bAutoInsert, or m_bMovingToDifferentPile flags. 
	//    By allowing the code in this function to execute only when all those flags are FALSE, 
	//    it eliminates extraneous calls of this block of code when PlaceBox() is called during
	//    merging, auto-inserting, and PlaceBox() calls in the midst of box moving operations.
	//
	// 2. Set boolean flags to prepare the location for the dropdown phrasebox
	//    2.a Initialize m_bChooseTransInitializePopup to TRUE so OnIdle() can set the initial
	//        state of the dropdown popup - open or closed.
	//        If in Free Translation mode initialize m_bChooseTransInitializePopup to FALSE.
	//    2.b Initialize m_bAbandonable to TRUE so that an immediate click away to a new
	//        location will not change the current location, unless user changes are made
	//        (which should set m_bAbandonable to FALSE).
	// 3. Determine a pCurTargetUnit for the present location. If a previous ChooseTranslation
	//    dialog session was just dismissed with changes made to this target unit, pCurTargetUnit
	//    will be non-NULL, and we use it to determine the non-deleted ref strings for the dropdown
	//    list. If pCurTargetUnit is NULL at this point, we determine the target unit for the 
	//    current location and use it to determine the non-deleted ref strings, if any.
	//
	// 4. Get the count of non-deleted ref string instances by calling pCurTargetUnit's 
	//    CountNonDeletedRefStringInstances() function, and proceed as follows depending
	//    on the returned count:
	//
	// When the count of non-deleted ref string instances nRefStrCount is > 1:
	//    4a. Load the contents of the phrasebox's drop down list (via calling PopulateDropDownList()).
	//        The PopulateDropDownList() function returns (via reference parameters) information about
	//        a list item that should be selected/highlighted (via selectionIndex), as well as whether
	//        there is a <no adaptation> ref string present (via bNoAdaptationFlagPresent), and if that
	//        flag is to be selected/highlighted (via indexOfNoAdaptation).
	//    4b. Call SetButtonBitMapNormal() to use the "normal" set of button bitmaps on the dropdown control,
	//        the down arrow button - indicating to user that the dropdown list has one or more items.
	//    4c. Call SetSelection(selectionIndex). The selectionIndex could be -1 which would remove any
	//        selection/highlight; or if >= 0 it highlights the selection. 
	//        Note that selecting/highlighting any list item has the side-effect of copying the item's 
	//        string at selectionIndex from the dropdown list into the combo box's edit box.
	//    4d. If the nRefStrCount == 1 we suppress OnIdle()'s opening of the popup list, by setting the 
	//        App's m_bChooseTransShowPopoup flag to FALSE, since any selection of the single item is 
	//        automatically copied into the edit box/phrasebox. 
	//        Otherwise, when nRefStrCount > 1, we set the App's m_bChooseTransInitializePopup flag to TRUE
	//        to cause the dropdown box to initially appear in an open state.
	//    4e. If the bNoAdaptationFlagPresent == TRUE we ensure the following (see and cf. 4f, 4g, 4h below): 
	//          When nRefStrCount == 1, that ref String must be an empty string, i.e., represented as
	//            "<no adaptation" as the single list item (with indexOfNoAdaptation == 0). 
	//            Here we avoid calling SetSelection() in this case to avoid the unwanted side effect of 
	//            a "<no adaptation>" string being copied into the dropdown's edit box. We just ensure that
	//            the m_targetPhrase's (empty) value is assigned in the m_pTargetBox, and that SetFocus()
	//            is on the phrasebox (SetFocus probably not needed but doesn't hurt).
	//          When nRefStrCount > 1, we know that there is more than 1 ref string in the list, and
	//            one of them should be a "<no adaptation>" item. The PopulateDropDownList() call will
	//            have determined the index stored in indexOfNoAdaptation, so we call
	//            SetSelection(indexOfNoAdaptation), and ensure that m_targetPhrase is assigned in the
	//            m_pTargetBox.
	//        If the bNoAdaptationFlagPresent == FALSE we ensure the following:
	//          When nRefStrCount == 1, there is one and only one ref string and it cannot be 
	//          a <no adaptation> type. The PopulateDropDownList() function has put the ref 
	//          string as the first string in the dropdown list. Do the following:
	//              Call GetString(0) and assign that item's string to the App's m_targetPhrase.
	//              Change the value in the m_pTargetBox to become the value now in m_targetPhrase.
	//              Call SetSelection(0) to select/highlight the list item (also copies it to the edit box).
	//          When nRefStrCount > 1, we know that there is more than 1 refString none of which are 
	//            empty ("<no adaptation>"). Do the following:
	//            If the selectionIndex is -1, set index to 0, otherwise set index to value of selectionIndex.
	//            Call GetString(index) and assign it to the App's m_targetPhrase.
	//            Change the value in the m_pTargetBox to become the value now in m_targetPhrase.
	//            Call SetSelection(index) to select/highlight the list item (also copies it to the edit box).
	//
	// When the count of non-deleted ref string instances nRefStrCount is == 0:
	// In this case - when nRefStrCount == 0, we are at a "hole", and there are no translation equivalents
	// in the dropdown's list.
	//    4a. We call ClearDropDownList() here to ensure that the list is empty. We do NOT call 
	//        PopulateDropDownList(), as we do not need selection indices returned for an empty list.
	//    4b. Call SetButtonBitMapXDisabled() to use the [X] button set of button bitmaps on the dropdown control,
	//        the [X] button image - indicating to user that there are no translation equivalents in the
	//        dropdown list to choose from. The list is not truly "disabled" - the user can still click on
	//        the [X] button, but the list will just open and be empty.
	//    4c. We do NOT call SetSelection() here as there is nothing to select/highlight.
	//    4d. With nothing in the dropdown list we set the App's m_bChooseTransInitializePopup flag to FALSE,
	//        which informs OnIdle() not to open/show the dropdown list.
	//    4e. The bNoAdaptationFlagPresent will always be FALSE when nRefStrCount == 0.
	//    4f. Call SetSelection(len,len) to deselect the content of the phrasebox - which in this case
	//        would be a copy of the source phrase if m_bCopySource == TRUE, empty otherwise.
	//    4g. Assign m_pTargetBox to contain the value of m_targetPhrase (a copy of source phrase if 
	//        m_bCopySource == TRUE).
	//    4h. Call SetFocus() to ensure phrasebox is in focus (probably not needed but doesn't hurt)
	//
	// 5. Reset the App's pCurTargetUnit to NULL to prepare for the next location or manual calling 
	//    up of the ChooseTranslation dialog.

	// Here we restrict execution for when all 3 flags below are FALSE. This reduces/eliminates
	// spurios execution when PlaceBox() is called during merging and moving operations.
	if (!pApp->bLookAheadMerge && !pApp->m_bAutoInsert && !pApp->m_bMovingToDifferentPile)
	{
		// If in Free translation mode keep the dropdown list closed by not allowing
		// OnIdle() to initialize the phrasebox's dropdown list to an open state.
		// But, when not in Free translation mode have OnIdle() initialize the phrasebox's
		// to the appropriate state - open for a list with items, closed for an empty list
		if (pApp->m_bFreeTranslationMode)
		{
			pApp->m_bChooseTransInitializePopup = FALSE;
			this->CloseDropDown(); // whm 21Aug2018 added. Unilaterally close the dropdown list when in free trans mode.
		}
		else
		{
			pApp->m_bChooseTransInitializePopup = TRUE;
		}
		// whm 10Apr2018 added. Set the initial value of m_bAbandonable to TRUE since we are setting up
		// for the dropdown phrasebox, and any content in the phrasebox should initially be considered
		// abandonable at least here when setting up the dropdown phrasebox for display to the user.
		// Certain actions at the current location may change the flag to FALSE before the phrasebox
		// moves - such as any key press that changes the phrasebox contents. 
		// whm 1Nov2022 Investigating the following assignment of m_bAbandonable to TRUE.
		// BEW reported on 31Oct2022, "If  do some adapting, and I type a meaning into the box then: 
		// if I hit Enter key, it often (or always) loses the box contents (but it does seem that what 
		// was typed goes into the KB - but the former location shows empty space). But if I typing into 
		// the box, and click to place the box elsewhere, the meaning seems to stick....if a saved 
		// meaning shows in the box, and I click on the next pile, (without first clicking on what's 
		// showing in the box first) then what was in the box disappears."
		// whm TODO: Try to repeat the issue BEW reports above, and track down which boolean flags
		// may not be set properly, and where, that could be causing the issue.
		this->m_bAbandonable = TRUE;

		// If the caller was LookAhead(), or if the ChooseTranslation dialog was just dismissed before
		// this SetupDropDownPhraseBoxForThisLocation() call, the global pCurTargetUnit will have been 
		// defined at this point, and we should use it. If it is NULL we calculate pCurTargetUnit at 
		// this location and use it.
		if (pApp->pCurTargetUnit == NULL)
		{
			// Get a local target unit pointer.
			// No pCurTargetUnit was available in the caller to populate the list,
			// So when pCurTargetUnit is NULL, we get the pCurTargetUnit directly using the 
			// appropriate KB's GetTargetUnit() method to populate the dropdown list.
			CTargetUnit* pTargetUnit = (CTargetUnit*)NULL;

			pApp->m_pTargetBox->m_nWordsInPhrase = 0;

			CKB* pKB;
			if (gbIsGlossing)
			{
				pKB = pApp->m_pGlossingKB;
			}
			else
			{
				pKB = pApp->m_pKB;
			}
			CSourcePhrase* pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
			pApp->m_pTargetBox->m_nWordsInPhrase = pSrcPhrase->m_nSrcWords;
			// Get a pointer to the target unit for the current key - could be NULL
			pTargetUnit = pKB->GetTargetUnit(pApp->m_pTargetBox->m_nWordsInPhrase, pSrcPhrase->m_key);

			// Assign the local target unit to the App's member pCurTargetUnit for use below
			pApp->pCurTargetUnit = pTargetUnit;
			//#if defined (_DEBUG) && defined (_ABANDONABLE)
			//			pApp->LogDropdownState(_T("SetupDropDownPhraseBoxForThisLocation() TRUE for  if (!pApp->bLookAheadMerge && !pApp->m_bAutoInsert && !pApp->m_bMovingToDifferentPile)"), _T("PhraseBox.cpp"), 7330);
			//#endif
		}

		// Get a count of the number of non-deleted ref string instances for the current target unit
		// (which may be adjusted by a prior instance of the ChooseTranslation dialog)
		int nRefStrCount = 0;

		if (pApp->pCurTargetUnit != NULL)
		{
			nRefStrCount = pApp->pCurTargetUnit->CountNonDeletedRefStringInstances();

			//#ifdef _DEBUG
			//			wxLogDebug(_T("PhraseBox.cpp at line %d ,nRefStringCount before a landing deletion = %d"), __LINE__, nRefStrCount);
			//#endif
		}
		//#if defined (_DEBUG) && defined (_ABANDONABLE)
		//		pApp->LogDropdownState(_T("SetupDropDownPhraseBoxForThisLocation(), line %d: TRUE for  if (!pApp->bLookAheadMerge && !pApp->m_bAutoInsert && !pApp->m_bMovingToDifferentPile)"), _T("PhraseBox.cpp"), __LINE__);
		//#endif

		if (nRefStrCount > 0)
		{
			int selectionIndex = -1;
			// whm 5Mar2018 Note: This block executes when nRefStrCount is > 0
			// whm 27Feb2018 Note: If the Choose Translation dialog was called before the execution of the 
			// PopulateDropDownList() function below, any changes, additions, reordering, deletions to the KB 
			// items listed there will pass into PopulateDropDownList() via the pCurTargetUnit 
			// parameter, and any selection of a list item from that dialog will be returned here via the
			// selectionIndex parameter, and the index of a <no adaptation> is returned in the
			// indexOfNoAdaptation parameter. (BEW 4Apr19, comment changed to reflect refactoring)
			if (pApp->pCurTargetUnit != NULL)
			{
				// BEW 30Mar19 try setting m_bLandingBox here to TRUE, otherwise lots of code internally
				// in the next call will be skipped - this is a first try, extra code may be needed to
				// govern the flag setting more carefully -- BEW 4Apr19, setting it here appears to be fine
				pApp->m_bLandingBox = TRUE;
				pApp->m_pTargetBox->PopulateDropDownList(pApp->pCurTargetUnit, selectionIndex, indexOfNoAdaptation);
#if defined (_DEBUG) && defined (_OVERLAP)
				{
					// for the ending wxLogDebug call, I want a calc of the listWidth to use in it.
					// Pile.cpp has the needed call, at the active pile location
					CPile* pPile = pApp->m_pActivePile;
					width = pPile->CalcPhraseBoxListWidth();
				}
#endif
			}

			// Clear the current target unit pointer
			pApp->pCurTargetUnit = (CTargetUnit*)NULL;

			// whm Note: Setting m_Translation to Empty() can't be done here. 
			// See end of OnButtonChooseTranslation() in the View.
			// Since PlaceBox() gets called multiple times in the course of a user action, resetting
			// variables to null or default values should not be done from this location. Hence, I 
			// commented out the m_Translation.Empty() call below. (BEW, yes, good)
			// m_pApp->m_pTargetBox->m_Translation.Empty(); 

			// If in Free Translation mode, use the "disable" button on the dropdown control
			// whm 12Jul2018 modified PopulateDropDownList() above can change the nRefStrCount
			// so base the set button bitmap on a fresh count of items in the list.
			if (pApp->m_bFreeTranslationMode)
			{
				this->SetButtonBitMapXDisabled();
				//                wxLogDebug(_T("Set button XDisabled - list count = %d in CPhraseBox::SetupDropDownPhraseBoxForThisLocation()"), (int)pApp->m_pTargetBox->GetDropDownList()->GetCount());
			}
			else if (pApp->m_pTargetBox->GetDropDownList()->GetCount() <= 1)
			{
				this->SetButtonBitMapXDisabled();
				//                wxLogDebug(_T("Set button XDisabled - list count = %d in CPhraseBox::SetupDropDownPhraseBoxForThisLocation()"), (int)pApp->m_pTargetBox->GetDropDownList()->GetCount());
			}
			else
			{
				this->SetButtonBitMapNormal();
				//                 wxLogDebug(_T("Set button Normal - list count = %d in CPhraseBox::SetupDropDownPhraseBoxForThisLocation()"), (int)pApp->m_pTargetBox->GetDropDownList()->GetCount());
			}

			// Set the dropdown's list selection to the selectionIndex determined by 
			// PopulatDropDownList above.
			// If selectionIndex ends up as -1, it removes any list selection from 
			// the dropdown list

			// ***********************************************************************
			// Note: SetSelection() with a single parameter operates to select/highlight
			// the dropdown's list item at the designated selectionIndex parameter. The
			// SetSelection() with two parameters operates to select a substring of text
			// within the dropdown's edit box, delineated by the two parameters.
			// ***********************************************************************

			pApp->m_pTargetBox->GetDropDownList()->SetSelection(selectionIndex);

			// whm 7Mar2018 addition - SetSelection() highlights the item in the list, and it
			// also has a side effect in that it automatically copies the item string from the 
			// dropdown list (matching the selectionIndex) into the dropdown's edit box.

			// The dropdown list, however, like the ChooseTranslation dialog's list, may contain
			// items and those items may not be adjusted for case - they are lower case when 
			// gbAutoCaps is TRUE. When we call SetSelection(), it results in the lower case form 
			// of the item string being put into the m_pApp->m_pTargetBox (the phrasebox). 
			// If case is not handled elsewhere we would need to handle case changes here. 
			// Note: testing indicates that the case changes are indeed handled elsewhere.

			// Within this block we know that nRefStrCount > 0 ; or it could 
			// be > 1 if the insertion of deleted (saved) adaptation of refCount 1
			// happened

//#if defined(_DEBUG) && defined(DROPDOWN)
//			pApp->LogDropdownState(_T("%s::SetupDropDownPhraseBoxForThisLocation() line %d : after SetSelection(selectionIndex))"), __FILE__, __LINE__);
//#endif
			// Note: The target unit's ref strings may include a "<no adaptation>" ref 
			// string which is stored internally as an empty m_target string member of 
			// the CRefString instance. An empty ref string is reconstituted 
			// (for display purposes to the user) as "<no adaptation>" when it  
			// resides in a list (dropdown or ChooseTranslation)

			// BEW 9May18 -- refactored to take into account the possibility of restoration, 
			// in adapting mode, of a stored copy of a deleted adaptation - note: it is
			// deleted only because before deletion it had a ref count of 1 when the phrase
			// box landed there; for higher ref counts of that adaptation, the box landing
			// only reduces the ref count by 1, so no deletion is involved.

			// Note 2: nRefStrCount was determined by counting non-deleted CRefString instances
			// in the call of CTargetUnit::CountNonDeletedRefStringInstances(); so, depending
			// on reference counts, there may be a difference of 1 between what is available
			// in the KB, and what is to get shown in the dropdown list
			if (indexOfNoAdaptation == 0)
			{
				// A <no adaptation> ref string is present at the start of
				// the pTU list, and it might be now deleted (but only if its
				// refCount was 1) - if nRefStrCount is 0, then indeed, the fact
				// it was not counted was because its m_refCount value was 1
				if (nRefStrCount == 0)
				{
					// A single ref string item, being <no adaptation>, now deleted, is the case.
					// The index of the <no adaptation> list item must be 0. We don't call the
					// SetSelection() function in this case because we don't want "<no adaptation>"
					// to be auto-copied into the dropdown's edit box, but we don't show a
					// single list item anyway, so no selection need happen
					// The App's m_targetPhrase and the phrasebox's edit box should be empty.
					wxASSERT(indexOfNoAdaptation == 0);
					// whm Note: While m_targetPhrase should be empty, the spurious calls of PlaceBox() 
					// before we get to the "real" one, the m_targetPhrase value may contain a value for
					// the previous location instead of the current one, so the following wxASSERT
					// cannot be relied on, and so to be safe I've commented it out. With the addition 
					// of the test for the 3 flags being FALSE above, the m_targetPhrase value should be
					// empty, but I'll leave the wxASSERT() test commented out to avoid any unforseen
					// spurious PlaceBox() calls where it may not be empty.
					//wxASSERT(pApp->m_targetPhrase.IsEmpty());
					pApp->m_targetPhrase = wxEmptyString;
					this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase);

#if defined (_DEBUG) && defined (_ABANDONABLE)
					pApp->LogDropdownState(_T("SetupDropDownPhraseBoxForThisLocation() line %d: end TRUE  block for if (nRefStrCount == 0)"), _T("PhraseBox.cpp"), __LINE__);
#endif
				}
				else if (nRefStrCount > 0)
				{
					// There is 1 or more ref strings, and one of them could be <no adaptation>
					// at the start of the list - (if <no adaptation> button pressed, the empty
					// list item will be top of list, and nRefStringCount will be 1).
					// BEW 30Mar19 comment changed. The <no adaptation> entry could be anywhere
					// in the list; so we want to test indexOfNoAdaptation against selectionIndex;
					// if they are the same, then the selection should be on the <no adaptation>
					// line, and we can empty the phrasebox as well (manually). If the indices are
					// different, selectionIndex wins, and the <no adaptation> list entry merely
					// sits unselected in the list.
					if (indexOfNoAdaptation == selectionIndex)
					{
						pApp->m_pTargetBox->GetDropDownList()->SetSelection(indexOfNoAdaptation);
						// The SetSelection call above has the side-effect of puting the list item 
						// <no adaptation> in the dropdown's edit box, so we have to make the box's
						// content be empty here.
						// Note: While m_targetPhrase should be empty, if any spurious calls of PlaceBox() 
						// happen before we get to the "real" one, the m_targetPhrase value may contain a  
						// value for the previous location instead of the current one, so the following 
						// wxASSERT cannot be relied on, and so to be safe, I've commented it out.
						//wxASSERT(pApp->m_targetPhrase.IsEmpty());
						pApp->m_targetPhrase = wxEmptyString;
						this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase);
					}
					else if (selectionIndex == wxNOT_FOUND) // -1
					{
						// the list has a <no adaptation> entry, but selectionIndex is
						// still defaulted to -1 (as when landing at a "hole"), so the top
						// item in the list should be made the default box value, and the
						// top list entry shown selected - whether or not it is a
						// <no adaptation> entry
						selectionIndex = 0;
						int index = selectionIndex;

						// whm 22Aug2018 modification. The index could be pointing to a "<Not In KB>" entry.
						// If so, we keep the m_targetPhrase, if any, rather than putting <Not In KB> into
						// the phrasebox's edit box. It could also be pointing at a <no adaptation> entry
						wxString tempStr = pApp->m_pTargetBox->GetDropDownList()->GetString(index);
						if (tempStr == _T("<Not In KB>"))
						{
							// Use any existing m_targetPhrase, which may be empty string.
							this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase);
						}
						else
						{
							pApp->m_targetPhrase = pApp->m_pTargetBox->GetDropDownList()->GetString(index);
							// Convert a <no adaptation> to empty string, if present
							if (pApp->m_targetPhrase == _T("<no adaptation>"))
							{
								pApp->m_targetPhrase = wxEmptyString;
							}
							this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase);
						}
						pApp->m_pTargetBox->GetDropDownList()->SetSelection(index);
					}
#if defined (_DEBUG) && defined (_ABANDONABLE)
					pApp->LogDropdownState(_T("%s::SetupDropDownPhraseBoxForThisLocation(), line %d : end TRUE  block for else if (nRefStrCount > 1)"),
						__FILE__, __LINE__);
#endif
				}
			}
			else // <no adaptation>, if present, is not at list top
			{
				// There may be a <no adaptation> ref string in the list, but
				// it is not at the top of the list
				wxASSERT(indexOfNoAdaptation != 0);
				if (nRefStrCount == 1)
				{
					// There is one and only one ref string and it cannot be a <no adaptation> type.
					//wxASSERT(pApp->m_pTargetBox->GetCount() == 1);
					int index;
					if (selectionIndex == -1)
					{
						// If we pass a selectionIndex of -1 to the SetSelection() call below it
						// would just empty the edit box and not select the item. Instead we
						// set the index to 0 to get the first item
						index = 0;
					}
					else
					{
						// The PopulateDropDownList() function determined a selectionIndex to use
						index = selectionIndex;
					}

					// whm 22Aug2018 modification. The index could be pointing to a "<Not In KB>" entry.
					// If so, we keep the m_targetPhrase, if any, rather than putting <Not In KB> into
					// the phrasebox's edit box.
					wxString tempStr = pApp->m_pTargetBox->GetDropDownList()->GetString(index);
					if (tempStr == _T("<Not In KB>"))
					{
						// Use any existing m_targetPhrase, which may be empty string.
						this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase);
					}
					else
					{
						pApp->m_targetPhrase = pApp->m_pTargetBox->GetDropDownList()->GetString(index);
						this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase);
					}
					pApp->m_pTargetBox->GetDropDownList()->SetSelection(index);
					// whm 13Jul2018 modified to remove selection and put insertion point at end
					// whm 3Aug2018 Note: The SetSelection(...,...) call is made in the outer block near the
					// end of SetupDropDownPhraseBoxForThisLocation(). See below.
#if defined (_DEBUG) && defined (_ABANDONABLE)
					pApp->LogDropdownState(_T("SetupDropDownPhraseBoxForThisLocation() line %d: end block for 'no <no adaptation> present' "), _T("PhraseBox.cpp"), __LINE__);
#endif
				}
				else // nRefStrCount > 1
				{
					// There is more than 1 ref string, none of which are <no adaptation>.
					// Here we basically just ensure that any existing phrasebox content
					// is selected in the dropdown list using the selectIndex info we got
					// back from PopulateDropDownList().
					//wxASSERT(pApp->m_pTargetBox->GetCount() > 1);
					int index;
					if (selectionIndex == -1)
					{
						// If we pass a selectionIndex of -1 to the SetSelection() call below it
						// would just empty the edit box and not select the item. Instead we
						// set the index to 0 to get the first item
						index = 0;
					}
					else
					{
						// The PopulateDropDownList() function determined a selectionIndex to use
						index = selectionIndex;
					}
					// whm 22Aug2018 modification. Although it shouldn't be the case when nRefStrCount > 1,
					// The index value could be pointing to a "<Not In KB>" entry.
					// If so, we keep the m_targetPhrase, if any, rather than putting <Not In KB> into
					// the phrasebox's edit box.
					wxString tempStr = pApp->m_pTargetBox->GetDropDownList()->GetString(index);
					if (tempStr == _T("<Not In KB>"))
					{
						// Use any existing m_targetPhrase, which may be empty string.
						this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase);
					}
					else
					{
						pApp->m_targetPhrase = pApp->m_pTargetBox->GetDropDownList()->GetString(index);
						this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase);
					}
					pApp->m_pTargetBox->GetDropDownList()->SetSelection(index);
					// whm 13Jul2018 modified to remove selection and put insertion point at end
					// whm 3Aug2018 Note: The SetSelection(...,...) call is made in the outer block near the
					// end of SetupDropDownPhraseBoxForThisLocation(). See below.

//#if defined (_DEBUG) && defined (_ABANDONABLE)
//	pApp->LogDropdownState(_T("SetupDropDownPhraseBoxForThisLocation() line %d: end block for nRefStrCount > 1, and lacking <no adaptation>"), _T("PhraseBox.cpp"), __LINE__);
//#endif
				}
			}

			/*
			// Below for debugging only!!!
			// Note: The debugging code below can also be copied into the else block (when nRefStrCount == 0) below
			// if desired.
			{
			wxString selStr = pApp->m_pTargetBox->GetTextCtrl()->GetStringSelection();
			wxString tgtBoxValue = pApp->m_pTargetBox->GetTextCtrl()->GetValue();
			wxString srcPhraseOfActivePile = pApp->m_pActivePile->m_pSrcPhrase->m_srcPhrase;
			wxString targetStrOfActivePile = pApp->m_pActivePile->m_pSrcPhrase->m_targetStr;
			wxString srcPhraseKey = pApp->m_pActivePile->m_pSrcPhrase->m_key;
			wxString srcPhraseAdaption = pApp->m_pActivePile->m_pSrcPhrase->m_adaption;
			wxString targetPhraseOnApp = pApp->m_targetPhrase;
			wxString translation = pApp->m_pTargetBox->m_Translation;
			CTargetUnit* ptgtUnitFmChooseTrans = pApp->pCurTargetUnit; ptgtUnitFmChooseTrans = ptgtUnitFmChooseTrans;
			bool hasKBEntry = pApp->m_pActivePile->m_pSrcPhrase->m_bHasKBEntry; hasKBEntry = hasKBEntry;
			bool notInKB = pApp->m_pActivePile->m_pSrcPhrase->m_bNotInKB; notInKB = notInKB;
			// [BEW note]: we use this 3-flag cocktail below elsewhere to test for these values of the three flags as
			// the condition for telling the application to retain the phrase box's contents when
			// user deselects target word, then makes a phrase and merges by typing.
			// m_bAbandonable is set FALSE, m_bUserTypedSomething is set TRUE, and m_bRetainContents is set TRUE for:
			// A click in the phrasebox (in OnLButtonDown), or Right/Left arrow key press (in OnKeyUp), or if
			// during a merge the m_targetPhrase > 1 and selections beyond the active location don't have any
			// translation (in OnButtonMerge)
			wxString alwaysAsk;
			if (pApp->pCurTargetUnit != NULL)
			if (pApp->pCurTargetUnit->m_bAlwaysAsk)
			alwaysAsk = _T("1");
			else
			alwaysAsk = _T("0");
			else
			alwaysAsk = _T("-1");
			bool abandonable = pApp->m_pTargetBox->m_bAbandonable; abandonable = abandonable;
			bool userTypedSomething = pApp->m_bUserTypedSomething; userTypedSomething = userTypedSomething;
			bool retainBoxContents = pApp->m_pTargetBox->m_bRetainBoxContents; retainBoxContents = retainBoxContents; // used in OnButtonMerge()
			wxLogDebug(_T("|***********************************************************************************"));
			wxLogDebug(_T("|***|For nRefStrCount > 0 [%d]: m_srcPhrase = %s, m_targetStr = %s, m_key = %s, m_adaption = %s"),nRefStrCount, srcPhraseOfActivePile.c_str(),targetStrOfActivePile.c_str(), srcPhraseAdaption.c_str(), srcPhraseAdaption.c_str());
			wxLogDebug(_T("|***|App's m_targetPhrase = %s, m_Translation = %s, m_pTargetBox->GetTextCtrl()->GetValue() = %s"),targetPhraseOnApp.c_str(), translation.c_str(),tgtBoxValue.c_str());
			wxLogDebug(_T("|***|  SrcPhrase Flags: m_bHasKBEntry = %d, m_bNotInKB = %d, targetUnit->m_bAlwaysAsk = %s"), (int)hasKBEntry, (int)notInKB, alwaysAsk.c_str());
			wxLogDebug(_T("|***|  TargetBox Flags: m_bAbandonable = %d, m_bUserTypedSomething = %d, m_bRetainBoxContents = %d"),(int)abandonable, (int)userTypedSomething, (int)retainBoxContents);
			wxLogDebug(_T("|***********************************************************************************"));
			}
			// Above for debugging only!!!
			*/
//#if defined (_DEBUG) && defined (_ABANDONABLE)
//			pApp->LogDropdownState(_T("SetupDropDownPhraseBoxForThisLocation() line %d: end of TRUE block for nRefStrCount > 0"), _T("PhraseBox.cpp"), __LINE__);
//#endif
		}
		else // when nRefStrCount == 0
		{
			// BEW 9May18, ammended the following note...
			// whm 5Mar2018 Note: When nRefStrCount is 0, we are at a 'hole', and there are no translation 
			// equivalents in the dropdown's list - as populated from pTU's current list of CRefStrings.
			// But if the boolean  bRemovedAdaptionReadyForInserting is TRUE, and the mode is 'not glossing',
			// then there is an adaptation ready to be re-inserted to the list, provided m_bLandingBox
			// is TRUE. We must check - so Bill's code here divides into two possibilities, (a) nothing is ready
			// for reinserting - in which case his earlier code applies; or (b) an adaptation is ready to become
			// the only entry in the list - it will be the top one once re-inserted, in which case a new block
			// of code is required here.
			// In the former (a) case we don't call PopulateDropDownList() here
			// (which automatically clears the dropdown list before re-populating it), but here also we must
			// make sure the dropdown list is empty, and set the dropdown's button image to its "disabled" 
			// state. Handle Bill's logic first. The other case, (b) we will restore to the contents of m_pTargetBox 
			// the removed value, and to the list to be shown - but only if in adapting mode and 'landing' the box.
			if (!bRemovedAdaptionReadyForInserting)
			{
				// Case (a) Bill's original logic applies
				pApp->m_pTargetBox->ClearDropDownList();
				this->SetButtonBitMapXDisabled();
				// With nothing in the dropdown list, we inform OnIdle() not to show the dropdown list
				// whm 18Apr2018 moved the setting of m_bChooseTransInitializePopup to TRUE/FALSE
				// above the if (nRefStrCount > 0) ... else blocks.
				//pApp->m_bChooseTransInitializePopup = FALSE;
				// A previous call to PlacePhraseBox() would have called DoGetSuitableText_ForPlacePhraseBox() which 
				// stored a suitable string str which was assigned to the App's m_targetPhrase member, and it would have
				// been followed by any AutoCaps processing. Hence, if m_bCopySource was TRUE the m_targetPhrase member
				// string will contain the copied source word/phrase processed by CopySourceKey(), which - if gbAutoCaps 
				// was also TRUE, the case of the copied source word/phrase will be preserved.

				//We should be able to put it in the m_pTargetBox here.
				this->GetTextCtrl()->ChangeValue(pApp->m_targetPhrase);
				// wxLogDebug(_T("Set button XDisabled - list count = %d in CPhraseBox::SetupDropDownPhraseBoxForThisLocation()"), (int)pApp->m_pTargetBox->GetDropDownList()->GetCount());
				//this->GetTextCtrl()->SetFocus(); // handled below in SetFocusAndSetSelectionAtLanding()
#if defined (_DEBUG) && defined (_ABANDONABLE)
				pApp->LogDropdownState(_T("SetupDropDownPhraseBoxForThisLocation() end of block for nRefStrCount == 0"), _T("PhraseBox.cpp"), 6348);
#endif
			}
			else
			{
				// we do the insertion only when the landing flag is TRUE, and not in glossing mode
				if (!gbIsGlossing && pApp->m_bLandingBox)
				{
					this->ClearDropDownList();
					// whm 4Jul2018 ammended SetButtonBitMapXDisabled() call below. The ClearDropDownList() has been 
					// called above, and a single entry is appended back into the list below making the list 
					// count == 1. We've decided that when there is just one item in the combobox it should
					// have the "disabled" appearance instead of the normal down arrow button.
					this->SetButtonBitMapXDisabled();
					this->GetDropDownList()->Append(strSaveListEntry);
					this->m_bAbandonable = FALSE;
					this->GetDropDownList()->SetSelection(0);
					// wxLogDebug(_T("Set button XDisabled - list count = %d in CPhraseBox::SetupDropDownPhraseBoxForThisLocation()"), (int)pApp->m_pTargetBox->GetDropDownList()->GetCount());
				}
			}
			// whm 3Aug2018 Note: The SetSelection call is made in the outer block near the
			// end of SetupDropDownPhraseBoxForThisLocation(). See below.

		}
		pApp->m_pTargetBox->SetFocusAndSetSelectionAtLanding(); // whm 13Aug2018 modified

		// Clear the current target unit pointer
		pApp->pCurTargetUnit = (CTargetUnit*)NULL; // clear the target unit in this nRefStrCount == 0 block too.
	}
}

// BEW created 30Mar19, for use in re-inserting into the dropdown
// list, a refCount = 1 entry, matching the phrasebox contents, when
// it is going to be deleted at the landing of the phrasebox at the
// active location. It sets some parameters in m_pTargetBox that
// the PopulateDropDownList will use to get the deleted entry back
// into view in the dropdown list at its former position in the list
bool CPhraseBox::RestoreDeletedRefCount_1_ItemToDropDown()
{
	// BEW 8May18 This is where I'll place the code for implementing Bill's request that 
	// the removed CRefString when landing at a location where the nRefCount for that
	// removed string was 1 (hence deleted from KB on landing) is nevertheless retained
	// in the combo control's list when it is dropped down at the PlaceBox() at end of
	// this PlacePhraseBox() function. This is a new behaviour which we will implement only
	// when not in glossing mode, and only when m_bLandingBox is TRUE, and various other 
	// constraints are satisfied as well. m_bLandingBox is a new app member bool as of 10May18
	CAdapt_ItApp* pApp = &wxGetApp();
	if (!gbIsGlossing && pApp->m_bLandingBox)
	{
		// Initializations
		wxString theBoxAdaptation = wxEmptyString; // initialize 
		wxString strTheListEntry = wxEmptyString; // what's in the KB's CRefString (matched to box contents)
		pApp->m_pTargetBox->bRemovedAdaptionReadyForInserting = FALSE;

		// Get the phrasebox entry. It could be capitalized, and gbAutoCaps may
		// ON or OFF. If ON, we must coerce any capitalized to-be-inserted adaptation
		// to lower case; but if OFF, just take the phrasebox contents as is
		if (pApp->m_pActivePile == NULL)
		{
			return FALSE;
		}
		CSourcePhrase* pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
		wxString strKey = pSrcPhrase->m_key; // it could be upper case
		if (gbAutoCaps)
		{
			bool bNoError = pApp->GetDocument()->SetCaseParameters(strKey, TRUE); // TRUE is bIsSrcText
			if (bNoError && gbSourceIsUpperCase && (gcharSrcUC != _T('\0')))
			{
				// change it to lower case
				strKey.SetChar(0, gcharSrcLC);
			}
		}

		// Get the phrasebox's current value - remember, it could be upper case;
		// or it could be empty (but its src text above it would not be empty)
		theBoxAdaptation = pApp->m_pTargetBox->GetValue(); // as is, but it may need 
														   // changing to lower case so as to match KB entries
		if (!theBoxAdaptation.IsEmpty())
		{
			if (gbAutoCaps)
			{
				// check case, and change to lower case if it is capitalized
				bool bNoError = pApp->GetDocument()->SetCaseParameters(theBoxAdaptation, FALSE); // FALSE is bIsSrcText
				if (bNoError)
				{
					if (gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
					{
						// change it to lower case
						theBoxAdaptation.SetChar(0, gcharNonSrcLC);
					}
				}
				else
				{
					return FALSE;
				}
			} // end of TRUE block for test: if (gbAutoCaps)
		} // end of TRUE block for test: if (!theBoxAdaptation.IsEmpty())
		int numberOfSrcWords = GetWordCount(strKey, NULL); // NULL means
				// that an array of words is not wanted from the call

		CTargetUnit* pTU = pApp->m_pKB->GetTargetUnit(numberOfSrcWords, strKey);
		// pTU will be NULL if the lookup failed, so check and if okay, then proceed

		// Check important flags do not disallow this location's list from
		// receiving the insertion hack
		if ((pTU != NULL) && !pSrcPhrase->m_bNotInKB &&
			!pSrcPhrase->m_bNullSourcePhrase && !pSrcPhrase->m_bRetranslation &&
			!pApp->m_bFreeTranslationMode)
		{
			// The target text (now lowercase if not already so) won't be
			// deleted yet; and is only going to be if the pTU's CRefstring
			// has a refCount of 1 and the phrasebox is landing at the
			// active pile. And we'll be inserting only provided the
			// phrasebox's theBoxAdaptation value is identical to the
			// pTU's CRefString's m_translation value. When those conditions
			// apply, we can set the relevant members of m_pTargetBox so that
			// PopulateDropDownList() can grab them. This block is the nitty
			// gritty core of this function...

			// Next, we must find the CRefString within pOwningTargetUnit which is the one
			// that is going to be deleted later when the phrasebox finishes 'landing'.
			// We are interested only in the case that it's m_refCount equals 1, because
			// higher reference counts will only get decremented by 1 and so the
			// adaptation for such as those will not get removed from the dropdown's list
			// anyway - so we'll get the CRefString and exit this block (doing no more
			// than re-initializing the hack variables) if the ref count is greater than 1.
			// But if equal to 1, then we've a lot more to do - our goal is to know what
			// the wxString is for the adaptation to be removed, and at what index it
			// would appear in the dropdown list had it not been removed - those are the
			// two bits of information that permit us to restore it to the list later
			// when PopulateDropDownList gets called at the PlaceBox() call at end of
			// this function
			bool bIsDeleted = FALSE; // initialized
			int nSelectionIndex = wxNOT_FOUND; // initialized to -1
			bool bSuccessfulMatch = FALSE; // initialized

			strTheListEntry = theBoxAdaptation; // RHS is lower case, target text
			CRefString* pRefString = pApp->m_pKB->GetMatchingRefString(pTU, strTheListEntry, bIsDeleted);
			// If no match, pRefString will be NULL, but if matched, it could be that bIsDeleted
			// is TRUE - in which case that entry would not get shown in the GUI list anyway, and so
			// would not be a candidate for re-insertion in the dropdown list. By now, it would
			// have been deleted if it had a refCount = 1
			if (pRefString == NULL || !bIsDeleted)
			{
				// Nothing qualifies, so return FALSE;
				return FALSE;
			}
			else
			{
				// We've obtained a pRefString whose m_translation should
				// be not be visible in the list, and is identical to the phrasebox's 
				// target text (adjusted for lower case if necessary). 
				// We now have to check it's refCount; if it was 1, then it will
				// the one we wish to restore to the list after the box lands.
				// (At this point, the phrasebox should already have landed and so
				// if the phrasebox's contents have already been removed from the list
				// because refCount was 1, bIsDeleted should be 1 here

				// We are interested only in a CRefString which has its m_refCount equal to 1, as pRefString
				// with m_refCount == 0 will have been auto-deleted from the KB already (such is illegal),
				// and the only other option is m_refCount > 1, and these one's adaptation string will appear
				// in the combobox's dropdown list without further intervention, because the decrement by
				// 1 will not bring their count back to zero. So do our final stuff only when the count == 1
				if (pRefString->m_refCount != 1)
				{
					; // do nothing, and return FALSE below - the hack is not to be done
				}
				else
				{
#if defined(_DEBUG) && defined(DROPDOWN)
					wxLogDebug(_T("View::%s at line %d , m_translation from CRefString that was deleted =  %s"),
						__FUNCTION__, __LINE__, strTheListEntry.c_str());
#endif
					// The adaptation we found will indeed be deleted, and removed from the KB. So we 
					// here need to work out what the index of that adaptation would be if the
					// dropdown list still retained it. The only way to work this out is to produce a
					// temporary string array list populated with adaptations drawn from the pTU we
					// we found above, and then search within it for a match with theAdaptation, including
					// the deleted one, but not other deleted ones, in a function which returns the index. 
					// Much work for a small result, but there's no other way. 
					// We'll build an arrTempComboList using the function below which clones bits of code
					// from Bill's PopulateDropDownList() function, but simplified, since we've already 
					// excluded <Not In KB>-contained CRefString instances from consideration here
					bSuccessfulMatch = pApp->BuildTempDropDownComboList(pTU, &strTheListEntry, nSelectionIndex);
					if (bSuccessfulMatch && (nSelectionIndex != wxNOT_FOUND))
					{
						// Store the matched index into m_pTargetBox
						this->nSaveComboBoxListIndex = nSelectionIndex;
						// strTheListEntry could be a stored empty string - remember
						this->strSaveListEntry = strTheListEntry; // first letter will already have 
																				// been case-adjusted if necessary
																				// Set the flag for PopulateDropDownList() to use for the re-insertion
						this->bRemovedAdaptionReadyForInserting = TRUE;
						this->nDeletedItem_refCount = 1; // has to be 1, no need for GetItemData() call
#if defined(_DEBUG) && defined(DROPDOWN)
						wxLogDebug(_T("%s:%s: line %d: nSaveComboBoxListIndex= %d , strSaveListEntry= %s , bRemovedAdaptionReadyForInserting= %d , nDeletedItem_refCount= %d"),
							__FILE__, __FUNCTION__, __LINE__, nSaveComboBoxListIndex, strSaveListEntry.c_str(),  
							(int)bRemovedAdaptionReadyForInserting, nDeletedItem_refCount);
#endif
						return TRUE; // This is the only place where we return TRUE
					} // end of TRUE block for test: 
					  // if (bSuccessfulMatch && (nSelectionIndex != wxNOT_FOUND))
				} // end of else block for test: if (pRefString->m_refCount != 1) 

			} // end of else block for test: if (pRefString == NULL || bIsDeleted)

		} // End of TRUE block for test: possibly can do the hack if pTU is not null,
		  // it's not a placeholder, it's not a retranslation, and free translation 
		  // mode is not ON
		else
		{
			// Can't do the hack
			return FALSE;
		}
	} // end of TRUE block for test: if (!gbIsGlossing && pApp->m_bLandingBox)
	return FALSE;
}

// This code based on PopulateList() in the CChooseTranslation dialog class.
// The pTU parameter is the pCurTargetUnit passed in from the dropdown setup
// code in PlaceBox(), which determined the value of pCurTargetUnit at the
// PlaceBox() location. If PlaceBox() was called immediately on the dismissal 
// of the Choose Translation dialog. pCurTargetUnit will reflect any changes 
// made to the composition or ordering of the ref strings in that dialog. 
// If pTU is not NULL, we use it here to populate the dropdown list.
// The selectionIndex is a reference parameter and will return the index
// of the user-selected item, or the index of the user-entered new translation 
// string in the populated dropdown list.
// whm 4Mar2018 moved some of the code that previously determined the value of
// pCurTargetUnit from within this function back to the caller in the Layout's
// PlaceBox() function.
// whm 5Mar2018 refactored the setup and PopulateDropDownList() function to 
// better make use of BEW's original coding as it already handles the bulk of 
// the phrasebox contents (and KB access and storage) by the time code execution 
// calls PopulateDropDownList().
// BEW 9May18, refactored (by a code addition) so that if a n_refCount == 1 KB
// entry got deleted at the 'landing' of the phrasebox, its adaptation here
// gets reinstated (but not so in the KB) at the correct location, and that
// location becomes the selected location. This feature addition requested
// by Bill Martin (about Feb 2018, plus or minus, by email) Storing the value
// to reinstate and its index in the list is done in PlacePhraseBox() before
// the value gets removed from the KB, and the needed restoration values were
// copied to m_pTargetBox, if matched successfully, to member variables here
// in PhraseBox, for that restoration-to-list purpose; namely, strSaveListEntry
// and nSaveComboBoxListIndex. The insertion is done only when the boolean
// bRemovedAdaptionReadyForInserting has been set TRUE
void CPhraseBox::PopulateDropDownList(CTargetUnit* pTU, int& selectionIndex, int& indexOfNoAdaptation)
{

#if defined(_DEBUG) && defined(DROPDOWN)
	wxLogDebug(_T("PopulateDropDownList: line %d, On Entry to PopulateDropDownList(), m_bLandingBox= %d"),
		__LINE__, (int)gpApp->m_bLandingBox);
#endif
#if defined(_DEBUG) && defined(DROPDOWN)
	wxLogDebug(_T("%s:%s(): line %d: pApp->m_pTargetBox->m_bAbandonable = %d"),
		__FILE__, __FUNCTION__, __LINE__, (int)gpApp->m_pTargetBox->m_bAbandonable);
#endif


	// BEW 30Mar19 added, so that the reinsertion of the deleted item, when the item
	// had a refCount of 1, can happen into the dropdown list at its former location
	// when gpApp->m_bLandingBox is TRUE, and it is not glossing mode
	bool bGotProcessed = FALSE; // initialization

	if (!gbIsGlossing && gpApp->m_bLandingBox)
	{
		// It a refCount = 1 list item has been deleted by the current location becoming
		// active when the phrasebox lands there, get the nSaveComboBoxListIndex value, and
		// its associated strSaveListEntry string, added to app's m_pTargetBox (ptr to
		// the phrasebox) where code further below can grab those two values so as to
		// restore the deleted item to the dropdown list, rather than the list failing
		// to have the deleted item. (KB *does* keep the deletion, at least temporarily)

		// First, initialize by calling InitializeComboLandingParams(): 	
		// nSaveComboBoxListIndex = -1;  // -1 indicates "index not yet set or known"
		// strSaveListEntry.Empty();     // empty the string
		// bRemovedAdaptionReadyForInserting = FALSE; // set to "not ready"
		// nDeletedItem_refCount = 1; // defaulted to 1
		this->InitializeComboLandingParams();

		bGotProcessed = this->RestoreDeletedRefCount_1_ItemToDropDown();
		if (!bGotProcessed)
		{
			// It failed in the attempt, so safest thing to do is to
			// ensure that the restoration parameters are initialized

			// nSaveComboBoxListIndex = -1;  // -1 indicates "index not yet set or known"
			// strSaveListEntry.Empty();     // empty the string
			// bRemovedAdaptionReadyForInserting = FALSE; // set to "not ready"
			// nDeletedItem_refCount = 1; // defaulted to 1, but obviously, is only going to be 1
			this->InitializeComboLandingParams();
		}
	}
#if defined(_DEBUG) && defined(DROPDOWN)
	wxLogDebug(_T("%s:%s(): line %d: pApp->m_pTargetBox->m_bAbandonable = %d"),
		__FILE__, __FUNCTION__, __LINE__, (int)gpApp->m_pTargetBox->m_bAbandonable);
#endif
    selectionIndex = -1; // initialize to inform caller if no selection was possible
    this->GetDropDownList()->Clear();
    wxString initialBoxContent;
    // Get the initial phrasebox content so we can match it in dropdown list if it is there
    initialBoxContent = gpApp->m_targetPhrase;
    
    if (pTU == NULL)
    {
        return; // If the incoming pTU is null then just return with empty list
    }
    wxString s = _("<no adaptation>");

	// Populate the combobox's list contents with the translation or gloss strings 
    // stored in the global variable pCurTargetUnit (passed in as pTU).
    CRefString* pRefString;
    TranslationsList::Node* pos = pTU->m_pTranslations->GetFirst();
    wxASSERT(pos != NULL);
    // initialize the reference parameters
	int nLocation = -1;
	indexOfNoAdaptation = -1;
	// BEW added 9May18 - there can only be one <no adaptation> in the list
	bool bExcludeAnother_NoAdaptation = FALSE; // initialize
	int count = 0;

	// Our approach will be to create the visible list items, accumulating
	// a count. If there is a deleted one, we'll insert it in its proper
	// location after the loop has exited, if it it was the last item in
	// the list, we'll append it after the loop has finished.
	// With this protocol - initialization, followed by working out where
	// the deleted item should be (if any was in fact deleted), and reinserting
	// or appending as the last action, we will be able to call PopulateDropDownList()
	// as many times as the latter function gets called, and whether or not
	// the ChooseTranslation() dialog has been used by the user to alter the
	// location in the list of the deleted item due to use of the Move Up or
	// Move Down buttons in that dialog.
	// One possible caveat: if the user elects to delete one of more other list item
	// at the same time that our hack code for a deleted item of refCount = 1
	// getting reinserted does it's job, what happens? I think the reinsertion will
	// happen, and the other deletions will just remain undeleted - (because the
	// reinsertion would just become a re-appending if the reinsertable item was 
	// earlier last in the list) -- so, I think it would work seamlessly & as 
	// expected by the user
    while (pos != NULL)
    {
        pRefString = (CRefString*)pos->GetData();
#if defined(_DEBUG) && defined(DROPDOWN)
		wxLogDebug(_T("PopulateDropDownList: %d, TopOfLoop, m_translation= %s , m_bDeleted= %d , m_refCount= %d"),
			__LINE__, pRefString->m_translation.c_str(), (int)pRefString->GetDeletedFlag(), pRefString->m_refCount);
#endif
        pos = pos->GetNext();
        if (!pRefString->GetDeletedFlag())
        {
            // this one is not deleted, so show it to the user
            wxString str = pRefString->m_translation;

			if (str.IsEmpty() && !bExcludeAnother_NoAdaptation)
			{
				str = s;
				bExcludeAnother_NoAdaptation = TRUE;
				nLocation = this->GetDropDownList()->Append(str);
				indexOfNoAdaptation = nLocation; // used near end, & return to caller
			}
			else
			{
				nLocation = this->GetDropDownList()->Append(str);
			}

            // whm 22Aug2018 Note: We handle the situation where <Not In KB> is present
            // within the populated dropdown list in the caller SetupDropDownPhraseBoxForThisLocation().

//#if defined(_DEBUG)
//			// Log, to see nothing got changed of what was stored on m_pTargetBox
//			bool bReadyToInsert = this->bRemovedAdaptionReadyForInserting;
//			wxString strBWSavedReinsertEntry = this->strSaveListEntry;
//			int nLocIndex = this->nSaveComboBoxListIndex;
//			wxLogDebug(_T("PopulateDropDownList: %d  In m_pTargetBox: bReadyToInsert= %d, savedTgtEntry= %s, nLocIndex= %d"),
//				__LINE__, (int)bReadyToInsert, strBWSavedReinsertEntry.c_str(), (int)nLocIndex);
//#endif

#if defined(_DEBUG) && defined(DROPDOWN)
			wxLogDebug(_T("PopulateDropDownList: %d  MID LOOP: gpApp->m_bLandingBox= %d bRemovedAdaptionReadyForInserting= %d"),
				__LINE__, (int)gpApp->m_bLandingBox, (int)bRemovedAdaptionReadyForInserting);
#endif
			if (!gbIsGlossing && gpApp->m_bLandingBox && bRemovedAdaptionReadyForInserting)
			{
#if defined(_DEBUG) && defined(DROPDOWN)
				wxLogDebug(_T("PopulateDropDownList: %d  Indices equal?: nSaveComboBoxListIndex= %d count(as an index)= %d"),
					__LINE__, (int)nSaveComboBoxListIndex, count);
#endif

			}
            count++;
            // The combobox's list is NOT sorted; the Append() command above returns the 
            // just-inserted item's index.
            // Note: the selectionIndex may be changed below if a new translation string was
            // appended to the dropdown list.
            wxASSERT(nLocation != -1); // we just added it so it must be there!
            this->GetDropDownList()->SetClientData(nLocation, &pRefString->m_refCount);
        } // end of TRUE block for test: if (!pRefString->GetDeletedFlag())

    } // end of while loop
#if defined(_DEBUG) && defined(DROPDOWN)
	wxLogDebug(_T("%s:%s(): line %d: pApp->m_pTargetBox->m_bAbandonable = %d"),
		__FILE__, __FUNCTION__, __LINE__, (int)gpApp->m_pTargetBox->m_bAbandonable);
#endif
#if defined(_DEBUG) && defined(DROPDOWN)
	wxLogDebug(_T("PopulateDropDownList: %d  AFTER LOOP, for restoring: strSaveListEntry= %s , m_bLandingBox= %d"),
		__LINE__, strSaveListEntry.c_str(), (unsigned int)gpApp->m_bLandingBox);
#endif
#if defined(_DEBUG) && defined(DROPDOWN)
	wxLogDebug(_T("PopulateDropDownList: %d  AFTER LOOP, for restoring: bRemovedAdaptionReadyForInserting= %d"),
		__LINE__, (unsigned int)bRemovedAdaptionReadyForInserting);
#endif

	if (!gbIsGlossing && gpApp->m_bLandingBox && bRemovedAdaptionReadyForInserting)
	{
		if (strSaveListEntry.IsEmpty())
		{
			// The is a <no adaptation> one - which we should show selected in the list
			strSaveListEntry = s;
			int anIndex;
			if (count < nSaveComboBoxListIndex)
			{
				// append, the item being restored was last in the list before it was deleted
				anIndex = (int)this->GetDropDownList()->Append(strSaveListEntry);
				this->GetDropDownList()->SetClientData(anIndex, &nDeletedItem_refCount);
			}
			else
			{
				anIndex = (int)this->GetDropDownList()->Insert(strSaveListEntry, (unsigned int)nSaveComboBoxListIndex);
				this->GetDropDownList()->SetClientData(anIndex, &nDeletedItem_refCount);
			}
			selectionIndex = anIndex;
#if defined(_DEBUG) && defined(DROPDOWN)
			wxLogDebug(_T("PopulateDropDownList: %d, DOING EMPTY insert: strSaveListEntry= %s , selectionIndex= %d , m_bLandingBox"),
				__LINE__, strSaveListEntry.c_str(), (int)selectionIndex, (int)gpApp->m_bLandingBox);
#endif
			count++; // count this re-inserted adaptation
			gpApp->m_pTargetBox->m_bAbandonable = FALSE;

		} // end of TRUE block for test: if (strSaveListEntry.IsEmpty())
		else
		{
			// strSaveListEntry is not empty, but another in the list might be; anyway
			// do the insertion and it should be shown selected
			int anIndex;
			if (count < nSaveComboBoxListIndex)
			{
				anIndex = (int)this->GetDropDownList()->Append(strSaveListEntry);
				this->GetDropDownList()->SetClientData(anIndex, &nDeletedItem_refCount);
			}
			else
			{
				anIndex = (int)this->GetDropDownList()->Insert(strSaveListEntry, (unsigned int)nSaveComboBoxListIndex);
				this->GetDropDownList()->SetClientData(anIndex, &nDeletedItem_refCount);
			}
			selectionIndex = anIndex;
#if defined(_DEBUG) && defined(DROPDOWN)
			wxLogDebug(_T("PopulateDropDownList: %d, DOING INSERT: strSaveListEntry= %s , selectionIndex= %d , m_bLandingBox= %d"),
				__LINE__, strSaveListEntry.c_str(), (int)selectionIndex, (int)gpApp->m_bLandingBox);
#endif
			count++; // count this re-inserted adaptation
			gpApp->m_pTargetBox->m_bAbandonable = FALSE;

		} // end of else block for test: if (strSaveListEntry.IsEmpty())
	}
#if defined(_DEBUG) && defined(DROPDOWN)
	wxLogDebug(_T("%s:%s(): line %d: pApp->m_pTargetBox->m_bAbandonable = %d"),
		__FILE__, __FUNCTION__, __LINE__, (int)gpApp->m_pTargetBox->m_bAbandonable);
#endif
    if (count > 0)
    {
        if (!initialBoxContent.IsEmpty())
        {
            // The phrasebox had an entry when we landed there (which could have been a copy of the source text)
            int indx = -1;
            indx = (int)this->GetDropDownList()->FindString(initialBoxContent, FALSE);  // FALSE - not case sensitive
            if (indx != wxNOT_FOUND)
            {
                // Select the list item - if it exists in the list - that matches what was in the 
                // phrasebox when we landed there. 
				selectionIndex = indx;
#if defined(_DEBUG) && defined(DROPDOWN)
				wxLogDebug(_T("PopulateDropDownList: %d, MATCHED BOX STR, NO INSERT: initialBoxContent= %s , selectionIndex= %d"),
					__LINE__, initialBoxContent.c_str(), (int)selectionIndex);
#endif
			}
        }
        else
        {
            // If the phrasebox had no content, then we just select first item in the list -
            // unless the phrasebox was empty because user clicked <no adaptation>, in which case 
            // we don't make a selection, but leave selectionIndex as -1 (as initialized at 
            // the beginning of this function -- BEW note 2Apr19: this convention is okay, but it
			// must not be misunderstood. It is perfectly acceptable for a <no adaptation>
			// click by the user to occur in a pTU with other non-empty adaptations, a
			// <no adaptation> can legally occur first, last, or anywhere inbetween.). 
            if (indexOfNoAdaptation != -1)
            {
                // User clicked <no adaptation>, leave selectionIndex with value of -1 
                // the indexOfNoAdaptation will have the index of the <no adaptation> item
                // to inform the caller.
                ; 
            }
            else
            {
                selectionIndex = 0; // have caller select first in list (index 0)
            }
        }
    }
#if defined(_DEBUG) && defined(DROPDOWN)
	wxLogDebug(_T("%s:%s(): line %d: pApp->m_pTargetBox->m_bAbandonable = %d"),
		__FILE__, __FUNCTION__, __LINE__, (int)gpApp->m_pTargetBox->m_bAbandonable);
#endif
    // See notes in CChooseTranslation::OnOK().
    // If the ChooseTranslation dialog was just called up and a new translation string was entered
    // in that dialog and OK'ed, the m_Translation variable (defined in CPhraseBox) will contain
    // that new translation. Although the new translation will appear in the dropdown list, and
    // will appear in the dropdown's edit box (selected), the new translation doesn't get added to 
    // the KB until the phrasebox moves on with Enter or Tab.
    if (!m_Translation.IsEmpty())
    {
        // whm updated 5Mar2018 check for if the string already exists. If not, append it and set its
        // selectionIndex. If it already exists in the list don't append it again, but update its 
        // selectionIndex to its current index in the list.
        int foundItem = -1;
        foundItem = this->GetDropDownList()->FindString(m_Translation);
        if (foundItem == wxNOT_FOUND)
        {
            selectionIndex = this->GetDropDownList()->Append(m_Translation);
        }
        else
        {
            selectionIndex = foundItem;
        }
        m_Translation.Empty(); // We've used it so clear it
    }
#if defined(_DEBUG) && defined(DROPDOWN)
	wxLogDebug(_T("%s:%s(): line %d: pApp->m_pTargetBox->m_bAbandonable = %d"),
		__FILE__, __FUNCTION__, __LINE__, (int)gpApp->m_pTargetBox->m_bAbandonable);
#endif
	//bool bIsFrozen = gpApp->GetLayout()->m_pCanvas->IsFrozen();
	//if (bIsFrozen)
	//	wxLogDebug(_T("Canvas is Frozen at end of PopulateDropDownList()"));
	//else
	//	wxLogDebug(_T("Canvas is NOT Frozen at end of PopulateDropDownList()"));
}
/* unwanted now
// BEW removed 5Apr22
// This is a cut-down version of the above PopulateDropDownList() function. It applies whether
// or not the project is a kbserver (sharing) project or not.
// Rationale? When a kbserver is in operation for the project, and the phrasebox lands at a location in
// the GUI, Leon's pseudo-delete external function, do_pseudo_delete.exe, gets called with a command
// line input file pseudo_delete.dat which contains the commandLine to guide do_pseudo_delete.exe's
// operation. do_pseudo_delete.exe will cause the entry table of kbserver to have the appropriate
// line pseudo-deleted. Out code will call RemoveReferenceString() as a post-executation operation, and
// in that we want to not require the user to have to call Choose Translation dialog, or KBEditor, to
// manually use the remove button in either, to get the dropdown list at the active location to reflect
// the (apparent) loss of the relevant item from the drop down list. It would be better to have a function
// that did this job automatically, on the user's behalf. PopulateDropDownList() has a lot of stuff in it
// that are boolean driven options not required for RepopulatedDropDownList(), hence this simplification.
// Note 1: this function does nothing if gbIsGlossing is TRUE; so in glossing mode, the user would be required
// to effect support for pseudo-deletion's list synchronization by using Choose Translation or KBEditor...
// Note 2: I'm not going to try, in the event of a pseudo-undelete when the phrasebox moves on, to restore
// a pseudo-deleted item mid list, to it's former mid-list location. (It can just get appended to the list.)
// That makes for significant simplification of the code in RepopulatedDropDownList().
// indexOfNoAdaptation value is returned to the caller, if set non-negative. It is the index in the drop down
// combo list at which an empty adaptation (if one exists there) occurs, and is used internally to set the
// string to show to the user as _("<no adaptation>" We return selectionIndex 0 to the caller if there is
// a valid string in the list, but if nothing valid, then return -1 (e.g. if <Not In KB> or only a pseudo-deleted
// string.
void  CPhraseBox::RepopulateDropDownList(CTargetUnit* pTU, int& selectionIndex, int& indexOfNoAdaptation)
{
#if defined (SHOWSYNC)
	wxLogDebug(_T("%s::%s() line %d : Entered..."), __FILE__, __FUNCTION__, __LINE__);
#endif
	if (gbIsGlossing)
		return;
	//bool bGotProcessed = FALSE; // initialization  <<-- unsure it I need to use this, at this point in development

	if (pTU == NULL)
	{
		return; // If the incoming pTU is null then just return with empty list
	}

	// First, initialize by calling InitializeComboLandingParams(): 	
	// nSaveComboBoxListIndex = -1;  // -1 indicates "index not yet set or known"
	// strSaveListEntry.Empty();     // empty the string
	// bRemovedAdaptionReadyForInserting = FALSE; // set to "not ready"
	// nDeletedItem_refCount = 1; // defaulted to 1
	this->InitializeComboLandingParams();
	wxString s = _("<no adaptation>");
	wxString notInKB = _T("<Not In KB>"); // we will check 1st entry for this, if present,
			// we exit without creating a list, below
	// Populate the combobox's list contents with the translation strings 
	// stored in the global variable pCurTargetUnit (passed in as pTU).
	CRefString* pRefString;
	TranslationsList::Node* pos = pTU->m_pTranslations->GetFirst(); // intialise iterator
	wxASSERT(pos != NULL);
	// initialize the reference parameters
	int nLocation = -1;
	indexOfNoAdaptation = -1;
	// BEW added 9May18 - there can only be one <no adaptation> in the list
	bool bExcludeAnother_NoAdaptation = FALSE; // initialise
	// Use bExcludeAnother_NoAdaptation (if FALSE) for handline a "<no adaptation>" string
	// as what gets shown to the user if the adaptation is empty
	int count = 0;
	bool bIsNotInKB = FALSE;
	// Our approach will be to create the visible list items, accumulating
	// a count. If there is a deleted one, we'll skip over it, but save to PhraseBox members
	// the index of it (in int nSaveComboBolListIndex) and the (possibly adjusted for case)
	// adaptation (or gloss) string (in wxString strSaveListEntry) so that later if the
	// phrasebox moves, PopulateDropdownList() will be able to pick up these two saved values
	// so as to restore the deleted item to it's former place.
	bool bIsDeleted = FALSE; // initialise
	wxString str = wxEmptyString; // initialise
	while (pos != NULL)
	{
		pRefString = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		// This is a real deletion (i.e removal from the local KB totally); however, the entry
		// in the entry table will remain there, but pseudo-deleted - which is just a flag change
		bIsDeleted = (bool)pRefString->GetDeletedFlag(); // 1 becomes TRUE,  0 becomes FALSE
		wxString saveStr; // use to preserve str value for resetting, if <no adaptation> replaces it
		if (!bIsDeleted)
		{
#if defined(_DEBUG) && defined(REPOPULATE)
			// m_refCount is the count of how many times in the documents has this CRefString instance been augmented
			wxLogDebug(_T("%s::%s() line %d, TopOfLoop, m_translation= %s , m_bDeleted= %d , m_refCount= %d"), __FILE__,
				__FUNCTION__, __LINE__, pRefString->m_translation.c_str(), (int)pRefString->GetDeletedFlag(), pRefString->m_refCount);
#endif
			// this one is not deleted, so show it to the user
			str = pRefString->m_translation;
			if (count == 0 && str == notInKB)
			{
				bIsNotInKB = TRUE;
				break;
				// Create no list
			}
			if (str.IsEmpty() && !bExcludeAnother_NoAdaptation)
			{
				saveStr = str;
				str = s;
				bExcludeAnother_NoAdaptation = TRUE;
				nLocation = this->GetDropDownList()->Append(str);
				indexOfNoAdaptation = nLocation; // return to caller (assume PopulateDropDownList()
												 // will need to use it, if <no adaptation> is/was in the list
				// restore str
				str = saveStr;  // if str was empty, we want the empty string in below code, not <no adaptation>
			}
			else
			{
				// Normal entry, append to the list end
				nLocation = this->GetDropDownList()->Append(str);
			}
			count++;
			// The combobox's list is NOT sorted
			wxASSERT(nLocation != -1); // we just added it so it must be there!
			if (pRefString->m_refCount < 0)
			{
				pRefString->m_refCount = 0; // ensure sanity
			}
			// nLocation (the index in list) is key, and pRefString->m_refCount records
			// the number of times in the user's history of adapting, this entry has been
			// accepted (KBEditor shows these values)
			this->GetDropDownList()->SetClientData(nLocation, &pRefString->m_refCount);
		}
		else
		{
#if defined (SHOWSYNC)
			wxLogDebug(_T("%s::%s() line %d : else block entered - should not happen"), __FILE__, __FUNCTION__, __LINE__);
#endif
			continue;
		}

	} // end of TRUE block for test: if (!bIsPseudoDeleted)

	if (bIsNotInKB || count == 0)
	{
		selectionIndex = -1; // <Not In KB> entries do not have a dropdown list, 
							 // and there can be no dropdown if there are no stored 
							 // CRefString instances in pTU
		return;
	}
	// Set selectionIndex to return to caller
	selectionIndex = count - 1;
	//this->GetDropDownList()->Refresh(); <- BEW 4Apr22 might not be needed, as 
	// PopupDropDownList() should be auto-called sometime after RemoveRefString() has finished
	
#if defined(_DEBUG) && defined(REPOPULATE)
	wxLogDebug(_T("%s::%s() line %d, Function ends. selectionIndex returned = %d (Will not be for the deleted one.)"),
		__FILE__, __FUNCTION__, __LINE__, selectionIndex);
#endif
	return;
}
*/

// BEW 13Apr10, no changes needed for support of doc version 5
void CPhraseBox::OnLButtonDown(wxMouseEvent& event)
{
    wxLogDebug(_T("CPhraseBox::::OnLButtonDown() triggered"));

	// This mouse event is only activated when user clicks mouse L button within
	// the phrase box, not elsewhere on the screen.
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	// whm added 15Mar12. When in read-only mode don't register any key strokes
	if (pApp->m_bReadOnlyAccess)
	{
		// return without calling Skip(). Beep for read-only feedback
		::wxBell();
		return;
	}

	// BEW 2May18 Inhibit dropping down (or up) if in Reviewing mode)
    // whm 11Mar2020 removed the following block since we now allow user to open/close
    // new phrasebox list. See also commented out block in Layout.cpp where the 
    // SetupDropDownPhraseBoxForThisLocation() is now called even in Review mode.
	//if (!pApp->m_bDrafting)
	//{
	//	// return without calling Skip(). Beep for Review mode feedback
	//	::wxBell();
	//	return;
	//}

    // whm addition: don't allow cursor to be placed in phrasebox when in free trans mode
    // and when it is not editable. Allows us to have a pink background in the phrase box
    // in free trans mode.
    // TODO? we could also make the text grayed out to more closely immulate MFC Windows
    // behavior (we could call Enable(FALSE) but that not only turns the text grayed out,
    // but also the background gray instead of the desired pink. It is better to do this
    // here than in OnLButtonUp since it prevents the cursor from being momemtarily seen in
    // the phrase box if clicked.
    if (pApp->m_bFreeTranslationMode && !this->IsEditable()) //if (pApp->m_bFreeTranslationMode && !this->GetTextCtrl()->IsEditable())
	{
		CMainFrame* pFrame;
		pFrame = pApp->GetMainFrame();
		wxASSERT(pFrame != NULL);
		wxASSERT(pFrame->m_pComposeBar != NULL);
		wxTextCtrl* pEdit = (wxTextCtrl*)
							pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
		wxASSERT(pEdit != NULL);
		pEdit->SetFocus();
		int len = pEdit->GetValue().Length();
		if (len > 0)
		{
            // whm 3Aug2018 Note: The SetSelection call below concerns the compose bar's edit
            // box, and no 'select all' involved here.
			pEdit->SetSelection(len,len);
		}
		::wxBell();
		// don't call Skip() as we don't want the mouse click processed elsewhere
		return;
	}
    // version 1.4.2 and onwards, we want a right or left arrow used to remove the
    // phrasebox's selection to be considered a typed character, so that if a subsequent
    // selection and merge is done then the first target word will not get lost; and so we
    // add a block of code also to start of OnChar( ) to check for entry with both
    // m_bAbandonable and m_bUserTypedSomething set TRUE, in which case we don't clear
    // these flags (the older versions cleared the flags on entry to OnChar( ) )

    // we use this flag cocktail elsewhere to test for these values of the three flags as
    // the condition for telling the application to retain the phrase box's contents when
    // user deselects target word, then makes a phrase and merges by typing
	m_bAbandonable = FALSE;
	pApp->m_bUserTypedSomething = TRUE;
    m_bRetainBoxContents = TRUE;
    //wxLogDebug(_T("CPhraseBox::OnLButtonDown() triggered with flag m_bAbandonable = FALSE"));
	event.Skip();
    GetSelection(&pApp->m_nStartChar, &pApp->m_nEndChar); //GetTextCtrl()->GetSelection(&pApp->m_nStartChar, &pApp->m_nEndChar);
}

// BEW 13Apr10, no changes needed for support of doc version 5
// whm 10Apr2018 update: With the implementation of the new dropdown phrasebox, 
// this OnLButtonUp() handler is not activated when the user clicks mouse L button 
// and releases it within the phrasebox. But, like the OnLButtonDown() handler - see 
// comment in OnLButtonDown() above - this OnLButtonUp() is triggered/executed 
// when the user clicks briefly on the dropdown control's down-arrow button and 
// releases - but OnLButtonUp() doesn't seem to be triggered after a long-press of
// the mouse on the down-arrow button. 
// Hence, currently neither this handler nor the OnLButtonDown() handler in the 
// CAdapt_ItCanvas get triggered when the user simply clicks within the phrasebox
// to remove the selection. This behavior is different than the behavior that was
// expected for a phrasebox based on wxTextCtrl in which the same handlers are
// triggered when the user clicks within the phrasebox to remove a selection.
// Hence, 
void CPhraseBox::OnLButtonUp(wxMouseEvent& event)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	// This mouse event is only activated when user clicks mouse L button within
	// the phrase box, not elsewhere on the screen
    wxLogDebug(_T("CPhraseBox::OnLButtonUp() triggered"));
    event.Skip();
    GetSelection(&pApp->m_nStartChar, &pApp->m_nEndChar); //GetTextCtrl()->GetSelection(&pApp->m_nStartChar, &pApp->m_nEndChar);
}

// return TRUE if we made a match and there is a translation to be inserted (see static var
// below); return FALSE if there was no match; based on LookAhead(), but only looks up a
// single src word at pNewPile; assumes that the app member, m_nActiveSequNum is set and
// that the CPile which is at that index is the pNewPile which was passed in
// BEW 26Mar10, no changes needed for support of doc version 5
// BEW 21Jun10: simplified signature
// BEW 21Jun10: changed to support kbVersion 2's m_bDeleted flag
// BEW 6July10, added test for converting a looked-up <Not In KB> string to an empty string
bool CPhraseBox::LookUpSrcWord(CPile* pNewPile)
{
	// refactored 2Apr09
	CAdapt_ItApp* pApp = &wxGetApp();
	CAdapt_ItView *pView = pApp->GetView(); // <<-- BEWARE if we later have multiple views/panes
	CLayout* pLayout = pApp->m_pLayout;
	wxString strNot = _T("<Not In KB>");
	//int	nNewSequNum; // set but not used
	//nNewSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber; // sequ number at the new location
	wxString	phrases[1]; // store built phrases here, for testing
							// against KB stored source phrases
	//int	numPhrases; // set but not used
	//numPhrases = 1;  // how many phrases were built in any one call of this function
    m_Translation.Empty(); // clear the static variable, ready for a new translation
						 // if one can be found
	m_nWordsInPhrase = 0;	  // assume no match
	m_bBoxTextByCopyOnly = FALSE; // restore default setting

	// we should never have an active selection at this point, so ensure it
	pView->RemoveSelection();

	// get the source word
	phrases[0] = pNewPile->GetSrcPhrase()->m_key;
	// BEW added 08Sep08: to prevent spurious words being inserted at the
	// end of a long retranslation when  mode is glossing mode
	if (pNewPile->GetSrcPhrase()->m_key == _T("..."))
	{
		// don't allow an ellipsis (ie. placeholder) to trigger an insertion,
		// leave m_Translation empty
        m_Translation.Empty();
		return TRUE;
	}

    // check this phrase (which is actually a single word), attempting to find a match in
    // the KB (if glossing, it might be a single word or a phrase, depending on what user
    // did earlier at this location before he turned glossing on)
	CKB* pKB;
	if (gbIsGlossing)
		pKB = pApp->m_pGlossingKB;
	else
		pKB = pApp->m_pKB;
	CTargetUnit* pTargetUnit = (CTargetUnit*)NULL;
	int index = 0;
	bool bFound = FALSE;
	// index is always 0 in this function, so lookup is only in the first map
	bFound = pKB->FindMatchInKB(index + 1, phrases[index], pTargetUnit);

	// if no match was found, we return immediately with a return value of FALSE
	if (!bFound)
	{
		pApp->pCurTargetUnit = (CTargetUnit*)NULL; // the global pointer must be cleared
		m_CurKey.Empty(); // global var m_CurKey not needed, so clear it
		return FALSE;
	}
    // whm 10Jan2018 Note: We do not call the ChooseTranslation dialog from LookAhead()
    // now that the choose translation feature is implemented in the CPhraseBox's dropdown
    // list. Hence, we should not set pCurTargetUnit here, since it should only be set
    // from the View's OnButtonChooseTranslation() handler.
    //pCurTargetUnit = pTargetUnit; // set global pointer so the dialog can use it if it is called
	m_CurKey = phrases[index]; // set the global m_CurKey so the dialog can use it if it is called
							 // (phrases[0] is copied for the lookup, so m_CurKey has initial case
							 // setting as in the doc's sourcephrase instance; we don't need
							 // to change it here (if ChooseTranslation( ) is called, it does
							 // any needed case changes internally)
	m_nWordsInPhrase = index + 1; // static variable, needed for merging source phrases in
								// the caller
    // BEW 21Jun10, for kbVersion 2 support, count the number of non-deleted CRefString
    // instances stored on this pTargetUnit
    int count =  pTargetUnit->CountNonDeletedRefStringInstances();
	if (count == 0)
	{
		// nothing in the KB for this key (except, possibly, one or more deleted
		// CRefString instances)
		pApp->pCurTargetUnit = (CTargetUnit*)NULL;
		m_CurKey.Empty();
		return FALSE;
	}

	// we found a target unit in the list with a matching m_key field,
	// so we must now set the static var translation to the appropriate adaptation text: this
	// will be the target unit's first entry in its list if the list has only one entry, else
	// we must present the user with a dialog to put up all possible adaptations for the user to
	// choose one, or type a new one, or reject all - in which case we return FALSE for manual
	// typing of an adaptation etc.
	// BEW 21Jun10, changed to support kbVersion 2's m_bDeleted flag. It is now possible
	// that a CTargetUnit have just a single CRefString instance and the latter has its
	// m_bDeleted flag set TRUE. In this circumstance, matching this is to be regarded as
	// a non-match, and the function then would need to return FALSE for a manual typing
	// of the required adaptation (or gloss)
	TranslationsList::Node* pos = pTargetUnit->m_pTranslations->GetFirst();
	wxASSERT(pos != NULL);

	wxASSERT(count > 0);
	if (count > 1 || pTargetUnit->m_bAlwaysAsk)
	{
		// move view to new location and select, so user has visual feedback)
		// about which element(s) is/are involved
		pView->RemoveSelection();

		// next code is taken from end of MoveToNextPile()
		// initialize the phrase box to be empty, so as not to confuse the user
        this->GetTextCtrl()->ChangeValue(_T(""));
		pApp->m_targetPhrase = _T("");

		// recalculate the layout
#ifdef _NEW_LAYOUT
		pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
		pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
		// get the new active pile
		pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
		wxASSERT(pApp->m_pActivePile != NULL);

		// scroll into view
		pApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);

		// make what we've done visible
		pView->Invalidate();
		pLayout->PlaceBox();

        // whm 10Jan2018 removed code here that originally called up the
        // Choose Translation dialog from within LookUpScrWord(). The above 
        // PlaceBox() call will display the dropdown's list of translations that can
        // be chosen from, so that calling CSourcePhrase::ChooseTranslation() was
        // no longer relevant.
	}
	else
	{
		// BEW 21Jun10, count has the value 1, but there could be deleted CRefString
		// intances also, so we must search to find the first non-deleted instance
		CRefString* pRefStr = NULL;
		while (pos != NULL)
		{
			pRefStr = pos->GetData();
			pos = pos->GetNext();
			if (!pRefStr->GetDeletedFlag())
			{
				// the adaptation string returned could be a "<Not In KB>" string, which
				// is something which never must be put into the phrase box, so check for
				// this and change to an empty string if that was what was fetched by the
				// lookup
                m_Translation = pRefStr->m_translation;
				if (m_Translation == strNot)
				{
					// change "<Not In KB>" to an empty string
                    m_Translation.Empty();
				}
				break;
			}
		}
	}

	// adjust for case, if necessary; this algorithm will not make a lower case string start
	// with upper case when the source is uppercase if the user types punctuation at the start
	// of the string. The latter is, however, unlikely - provided the auto punctuation support
	// is utilized by the user
	if (gbAutoCaps && gbSourceIsUpperCase)
	{
		bool bNoError = pApp->GetDocument()->SetCaseParameters(m_Translation, FALSE);
		if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
		{
			// make it upper case
            m_Translation.SetChar(0, gcharNonSrcUC);
		}
	}
	return TRUE;
}

// BEW 13Apr10, no changes needed for support of doc version 5
// 
// whm 11Nov2022 note: This CPhraseBox::OnEditUndo() is designed to handle
// the undo of removals that were done via the backspace key.
// This OnEditUndo() cannot be triggered by an EVT_MENU event table command;
// instead it may be called directly by the View's OnEditUndo() which DOES
// GET TRIGGERED by the View's own EVT_MENU event table entry. Hence, I've
// removed the wxCommandEvent parameter and made this be a normal function. 
// If an undo of all of the phrasebox edits done since landing at the current
// location is desired it can be done by pressing the ESC key (that undo process 
// is in the OnKeyDown() ESC_KEY handler).
// All backspace key strokes themselves invoke the OnPhraseBoxChanged() handler,
// but undo commands for the phrasebox don't trigger the OnPhraseBoxChanged()
// handler, so we invoke it here, and OnPhraseBoxChanged() will detect if a 
// phrasebox resize needs to be done due to the undo operation. 
// This change also renders the only use of the old FixBox() totally obsolete.
void CPhraseBox::OnEditUndo()
// no changes needed for support of glossing or adapting
{
	// We have to implement undo for a backspace ourselves, but for delete & edit menu
	// commands the CEdit Undo() will suffice; we use a non-empty m_backspaceUndoStr as a
	// flag that the last edit operation was a backspace
	CAdapt_ItApp* pApp = GetLayout()->m_pApp;

	if (m_backspaceUndoStr.IsEmpty())
	{
		// last operation was not a <BS> keypress,
		// so Undo() will be done instead
		;
	}
	else
	{
		if (!(m_nSaveStart == -1 || m_nSaveEnd == -1))
		{
			bool bRestoringAll = FALSE;
			wxString thePhrase;
			thePhrase = this->GetTextCtrl()->GetValue();
			int undoLen = m_backspaceUndoStr.Length();
			if (!thePhrase.IsEmpty())
			{
				thePhrase = InsertInString(thePhrase,(int)m_nSaveEnd,m_backspaceUndoStr);
			}
			else
			{
				thePhrase = m_backspaceUndoStr;
				bRestoringAll = TRUE;
			}

			// whm 11Nov2022 removed the FixBox() function and the code here that called FixBox()

			wxString inStr = wxEmptyString;
			long from;
			long to;
			pApp->m_pTargetBox->GetSelection(&from, &to);
			gnBoxCursorOffset = (int)from;

			// restore the box contents
            this->GetTextCtrl()->ChangeValue(thePhrase);
			m_backspaceUndoStr.Empty(); // clear, so it can't be mistakenly undone again

			// whm 11Nov2022 added following call of OnPhraseBoxChanged() here in 
			// OnEditUndo() which can deal with any phrasebox size changes that could
			// be in order after the edit undo.
			wxCommandEvent dummyEvent;
			this->OnPhraseBoxChanged(dummyEvent);

			// fix the cursor location
			if (bRestoringAll)
			{
				pApp->m_nStartChar = -1;
				pApp->m_nEndChar = -1;
                this->GetTextCtrl()->SetSelection(pApp->m_nStartChar,pApp->m_nEndChar); // all selected
			}
			else
			{
				pApp->m_nStartChar = pApp->m_nEndChar = (int)(m_nSaveStart + undoLen);
                this->GetTextCtrl()->SetSelection(pApp->m_nStartChar,pApp->m_nEndChar);
			}
		}
	}
}

// DoStore_ForPlacePhraseBox added 3Apr09; it factors out some of the incidental
// complexity in the PlacePhraseBox() function, making the latter's design more
// transparent and the function shorter
// BEW 22Feb10, no changes needed for support of doc version 5
// BEW 21Jun10, no changes needed for support of kbVersion 2
// BEW 17Jul11, changed for GetRefString() to return KB_Entry enum, and use all 10 maps
// for glossing KB
// BEW 9Sep22 - no substantive changes, but simplified by adding pActiveSrcPhrase line
bool CPhraseBox::DoStore_ForPlacePhraseBox(CAdapt_ItApp* pApp, wxString& targetPhrase)
{
	CAdapt_ItDoc* pDoc = pApp->GetDocument();
	bool bOK = TRUE;
	pApp->m_bInNormalStore = TRUE;
	CRefString* pRefStr = NULL;
	KB_Entry rsEntry;
	// Restore user's choice for the command bar button ID_BUTTON_NO_PUNCT_COPY
	pApp->m_bCopySourcePunctuation = pApp->m_pTargetBox->m_bCurrentCopySrcPunctuationFlag;
	CSourcePhrase* pActiveSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();

	if (gbIsGlossing)
	{
		if (targetPhrase.IsEmpty())
			pActiveSrcPhrase->m_gloss = targetPhrase;

		// store will fail if the user edited the entry out of the glossing KB, since it
		// cannot know which srcPhrases will be affected, so these will still have their
		// m_bHasKBEntry set true. We have to test for this, ie. a null pRefString but
		// the above flag TRUE is a sufficient test, and if so, set the flag to FALSE
		rsEntry = pApp->m_pGlossingKB->GetRefString(pActiveSrcPhrase->m_nSrcWords,
									pActiveSrcPhrase->m_key, targetPhrase, pRefStr);
		if ((pRefStr == NULL || rsEntry == present_but_deleted) && pActiveSrcPhrase->m_bHasGlossingKBEntry)
		{
			pActiveSrcPhrase->m_bHasGlossingKBEntry = FALSE;
		}
		bOK = pApp->m_pGlossingKB->StoreText(pActiveSrcPhrase, targetPhrase);
	}
	else // is adapting
	{
		if (targetPhrase.IsEmpty())
		{
			pActiveSrcPhrase->m_adaption = wxEmptyString;
			pActiveSrcPhrase->m_targetStr = wxEmptyString;
			pApp->m_bInhibitMakeTargetStringCall = TRUE;

			bOK = pApp->m_pKB->StoreText(pActiveSrcPhrase, targetPhrase);
			pApp->m_bInhibitMakeTargetStringCall = FALSE;
		}
		else
		{
			// re-express the punctuation
			wxString copiedTargetPhrase = targetPhrase; // copy, with any punctuation user typed as well
			// targetPhrase now lacks punctuation - so it can be used for storing in the KB
			pApp->GetView()->RemovePunctuation(pDoc, &targetPhrase, from_target_text);

			// The store would fail if the user edited the entry out of the KB, as the latter
			// cannot know which srcPhrases will be affected, so these would still have their
			// m_bHasKBEntry set true. We have to test for this, and the m_HasKBEntry flag TRUE 
			// is a sufficient test, and if so, set the flag to FALSE, to enable the store op
			rsEntry = pApp->m_pKB->GetRefString(pActiveSrcPhrase->m_nSrcWords, pActiveSrcPhrase->m_key,
				targetPhrase, pRefStr);
			if ((pRefStr == NULL || rsEntry == present_but_deleted) && pActiveSrcPhrase->m_bHasKBEntry)
			{
				pActiveSrcPhrase->m_bHasKBEntry = FALSE;
			}
			pActiveSrcPhrase->m_targetStr = wxEmptyString; // start off empty

			pApp->m_bInhibitMakeTargetStringCall = TRUE;
			bOK = pApp->m_pKB->StoreText(pActiveSrcPhrase, targetPhrase);
			pApp->m_bInhibitMakeTargetStringCall = FALSE;

#if defined (_DEBUG)
			{
				wxLogDebug(_T("DoStore_ForPlacePhraseBox(), MakeTgtStrIncPunc() BEFORE, line %d: tgtPhrase= %s , sn = %d , m_srcPhrase = %s "),
					__LINE__, targetPhrase.c_str(), pActiveSrcPhrase->m_nSequNumber, pActiveSrcPhrase->m_srcPhrase.c_str() );
			}
#endif
			pApp->m_bCopySourcePunctuation = pApp->m_pTargetBox->m_bCurrentCopySrcPunctuationFlag;
			// Now use the user's actual phrasebox contents, with any puncts he may have typed
			pApp->GetView()->MakeTargetStringIncludingPunctuation(pActiveSrcPhrase, copiedTargetPhrase);

#if defined (_DEBUG)
			{
				wxLogDebug(_T("DoStore_ForPlacePhraseBox(), MakeTgtStrIncPunc() AFTER, line %d: tgtPhrase= %s , sn = %d , m_srcPhrase = %s "),
					__LINE__, targetPhrase.c_str(), pActiveSrcPhrase->m_nSequNumber, pActiveSrcPhrase->m_srcPhrase.c_str());
			}
#endif
			// Now restore the flag to its default value
			pApp->m_bCopySourcePunctuation = TRUE;
			pApp->m_pTargetBox->m_bCurrentCopySrcPunctuationFlag = TRUE;
		}
	}
	pApp->m_bInNormalStore = FALSE;
	return bOK;
}

// BEW refactored 21Jul14 for support of ZWSP storage and replacement;
// also moved the definition to be in PhraseBox.h & .cpp (was in view class)
void CPhraseBox::RemoveFinalSpaces(CPhraseBox* pBox, wxString* pStr)
{
	// empty strings don't need anything done
	if (pStr->IsEmpty())
		return;

	// remove any phrase final space characters
	bool bChanged = FALSE;
	int len = pStr->Length();
	int nIndexLast = len-1;
	// BEW 21Jul14 refactored for ZWSP support. The legacy code can be left unchanged for
	// handling latin space; but for exotic spaces we'll use the overridden
	// RemoveFinalSpaces() as its for any string - so test here for what is at the end.
	// We'll assume the end doesn't have a mix of latin space with exotic ones
	if (pStr->GetChar(nIndexLast) == _T(' '))
	{
		// Latin space is at the end, so do the legacy code
		do {
			if (pStr->GetChar(nIndexLast) == _T(' '))
			{
				// Note: wsString::Remove must have the second param as 1 here otherwise
				// it will truncate the remainder of the string!
				pStr->Remove(nIndexLast,1);
				// can't trust the Remove's returned value, it exceeds string length by one
				len = pStr->Length();
				nIndexLast = len -1;
				bChanged = TRUE;
			}
			else
			{
				break;
			}
		} while (len > 0 && nIndexLast > -1);
	}
	else
	{
		// There is no latin space at the end, but there might be one or more exotic ones,
		// such as ZWSP. (We'll assume there's no latin spaces mixed in with them)
		wxChar lastChar = pStr->GetChar(nIndexLast);
		CAdapt_ItApp* pApp = &wxGetApp();
		CAdapt_ItDoc* pDoc = pApp->GetDocument();
		if (pDoc->IsWhiteSpace(&lastChar))
		{
			// There must be at least one exotic space at the end, perhaps a ZWSP
			bChanged = TRUE;
			wxString revStr = *pStr; // it's not yet reversed, but will be in the next call
									 // and restored to non-reversed order before its returned
			RemoveFinalSpaces(revStr); // signature is ref to wxString

			*pStr = revStr;
			// pBox will have had its contents changed by at least one wxChar being
			// chopped off the end, so let the bChanged block below do the phrasebox update
		}
		else
		{
			// There is no exotic space at the end either, so pStr needs nothing removed,
			// so just return without changing the phrasebox contents
			return;
		}
	}
	if (bChanged) // need to do this, because for some reason rubbish is getting
            // left in the earlier box when the ChooseTranslation dialog gets put up. That
            // is, a simple call of SetWindowText with parameter pStr cast to (const char
            // *) doesn't work right; but the creation & setting of str below fixes it
	{
		wxString str = *pStr;
		pBox->GetTextCtrl()->ChangeValue(str);
	}
}

// BEW added 30Apr08, an overloaded version which deletes final spaces in any CString's
// text, and if there are only spaces in the string, it reduces it to an empty string
// BEW 21Jul14, refactored to also remove ZWSP and other exotic white spaces from end of
// the string as well; and moved to be in PhaseBox.h & .cpp (was in view class)
void CPhraseBox::RemoveFinalSpaces(wxString& rStr)
{
    // whm Note: This could be done with a single line in wx, i.e., rStr.Trim(TRUE), but
    // we'll go with the MFC version for now.
	if (rStr.IsEmpty())
		return;
	rStr = MakeReverse(rStr);
	wxChar chFirst = rStr[0];
	if (chFirst == _T(' '))
	{
		// The legacy code - just remove latin spaces, we'll assume that when this is apt,
		// there are no exotics there as well, such as ZWSP
		while (chFirst == _T(' '))
		{
			rStr = rStr.Mid(1);
			chFirst = rStr[0];
		}
		if (rStr.IsEmpty())
			return;
		else
			rStr = MakeReverse(rStr);
	}
	else
	{
		// BEW 21Jul14 new code, to support ZWSP removals, etc, from end
		// we reversed rStr, so chFirst is the last
		CAdapt_ItApp* pApp = &wxGetApp();
		CAdapt_ItDoc* pDoc = pApp->GetDocument();
		if (pDoc->IsWhiteSpace(&chFirst))
		{
			// There is an exotic space at the end, remove it and do any more
			rStr = rStr.Mid(1);
			if (rStr.IsEmpty())
			{
				return;
			}
			chFirst = rStr[0];
			while (pDoc->IsWhiteSpace(&chFirst))
			{
				rStr = rStr.Mid(1);
				if (rStr.IsEmpty())
				{
					return;
				}
				chFirst = rStr[0];
			}
			rStr = MakeReverse(rStr);
			return;
		}
		else
		{
			// No exotic space at the end, so re-reverse & return string
			rStr = MakeReverse(rStr);
		}
		return;
	}
}

// BEW 23Apr15 functions for support of / as word-breaking whitespace, with
// conversion to ZWSP in strings not accessible to user editing, and replacement
// of ZWSP with / for those strings which are user editable; that is, when
// putting a string into the phrasebox, we restore / delimiters, when getting
// the phrasebox string for some purpose, we replace all / with ZWSP

void CPhraseBox::ChangeValue(const wxString& value)
{
	// uses function from helpers.cpp
	wxString convertedValue = value; // needed due to const qualifier in signature
	convertedValue = ZWSPtoFwdSlash(convertedValue); // no changes done if m_bFwdSlashDelimiter is FALSE
    wxTextCtrl::ChangeValue(convertedValue);
    // whm 12Jul2018 the above call of the base class' ChangeValue() was commented out.
    // After implementing the new phrasebox, the following call would result in 
    // infinite recursion on the stack and subsequent crash.
    // TODO: Bruce should test the present form of this override of ChangeValue() to
    // ensure that it still accomplishes the appropriate processing of ZWSPtoFwdSlash.
    //this->GetTextCtrl()->ChangeValue(convertedValue);  //this->ChangeValue(convertedValue); 
}


void CPhraseBox::InitializeComboLandingParams()
{
	nSaveComboBoxListIndex = -1;  // -1 indicates "not yet set or known"
	strSaveListEntry.Empty();
	bRemovedAdaptionReadyForInserting = FALSE; // into the combo box's dropdown list - at its former location
}

// whm 11Jul2018 added some access methods for the parts of our phrasebox
wxTextCtrl* CPhraseBox::GetTextCtrl()
{
    return m_pTextCtrl;
}

CMyListBox* CPhraseBox::GetDropDownList()
{
    return m_pDropDownList;
}

wxBitmapToggleButton* CPhraseBox::GetPhraseBoxButton()
{
    return m_pPhraseBoxButton;
}

void CPhraseBox::SetTextCtrl(wxTextCtrl* textCtrl)
{
    m_pTextCtrl = textCtrl;
}

void CPhraseBox::SetDropDownList(CMyListBox* listBox)
{
    m_pDropDownList = listBox;
}

void CPhraseBox::SetPhraseBoxButton(wxBitmapToggleButton* listButton)
{
    m_pPhraseBoxButton = listButton;

}

void CPhraseBox::SetButtonBitMapNormal()
{
    wxPoint currPosn = this->GetPhraseBoxButton()->GetPosition();
    this->GetPhraseBoxButton()->Destroy();
    // whm 18Jul2018 the Linux version needs the bitmap size to be about 10 pixels wider and taller (30, 34) as it has a wide button margin that can't be changed
#if defined (__WXGTK__)
    wxBitmapToggleButton* pBitmapBtn = new wxBitmapToggleButton(gpApp->GetMainFrame()->canvas, ID_BMTOGGLEBUTTON_PHRASEBOX, bmp_dropbutton_normal, wxDefaultPosition, wxSize(30, 34));
#else
    wxBitmapToggleButton* pBitmapBtn = new wxBitmapToggleButton(gpApp->GetMainFrame()->canvas, ID_BMTOGGLEBUTTON_PHRASEBOX, bmp_dropbutton_normal, wxDefaultPosition, wxSize(22, 26));
#endif
    gpApp->m_pTargetBox->SetPhraseBoxButton(pBitmapBtn);
    //this->GetPhraseBoxButton()->SetBitmapLabel(wxBitmap(xpm_dropbutton_normal));
    this->GetPhraseBoxButton()->SetPosition(currPosn);
    this->GetPhraseBoxButton()->Refresh(); // whm added 31May2019 in attempt to address Bruce's reports that some Windows computers are not showing the "normal" button consistently
}

void CPhraseBox::SetButtonBitMapXDisabled()
{
    wxPoint currPosn = this->GetPhraseBoxButton()->GetPosition();
    this->GetPhraseBoxButton()->Destroy();
#if defined (__WXGTK__)
    wxBitmapToggleButton* pBitmapBtn = new wxBitmapToggleButton(gpApp->GetMainFrame()->canvas, ID_BMTOGGLEBUTTON_PHRASEBOX, bmp_dropbutton_X, wxDefaultPosition, wxSize(30, 34));
#else
    wxBitmapToggleButton* pBitmapBtn = new wxBitmapToggleButton(gpApp->GetMainFrame()->canvas, ID_BMTOGGLEBUTTON_PHRASEBOX, bmp_dropbutton_X, wxDefaultPosition, wxSize(22, 26));
#endif
    gpApp->m_pTargetBox->SetPhraseBoxButton(pBitmapBtn);
    //this->GetPhraseBoxButton()->SetBitmapLabel(dropbutton_X);
    this->GetPhraseBoxButton()->SetPosition(currPosn);
    this->GetPhraseBoxButton()->Refresh(); // whm added 31May2019 in attempt to address Bruce's reports that some Windows computers are not showing the "normal" button consistently
}


// whm 13Aug2018 added. This SetFocusAndSetSelectionAtLanding() function encapsulates
// the current protocol adopted as of version 6.9.1 that specifies the phrasebox 
// should be in focus, but its text should not have any initial selection at landing
// and the edit insertion point should be at the END of the text. The new protocol allows
// for an exception; the user has the option of having all text selected, but only if the 
// phrasebox's dropdown list has 0 or 1 items (i.e., the 'Copy Source' menu item is 
// toggled TRUE), AND the user has ticked the 'Select Copied Source' toggle menu, 
// toggleing it to TRUE. 
// This function also ensures that SetFocus() is called BEFORE SetSelection(len,len) to
// avoid the Linux/Mac version's apparent default of selecting all text in the edit box
// when the SetFocus() call follows a SetSelection(len,len) call - nullifying the effect
// of the SetSelection(len,len) call on the non-Windows platforms.
//
// NOTE: This function should NOT be called for situations in which the phrasebox has
// previously landed and an editing operation is in progress within the phrasebox, and
// a previously made text selection must be preserved. That is, DO NOT USE this function
// when the SetSelection() call utilizes the App's m_nStartChar and m_nEndChar values 
// as in GetTextCtrl()->SetSelection(pApp->m_nStartChar, pApp->m_nEndChar).
void CPhraseBox::SetFocusAndSetSelectionAtLanding()
{
    // whm 13Aug2018 Ensure SetFocus() called before SetSelection, otherwise 
    // Linux/Mac versions select all text when SetFocus() is called after SetSelection.
    this->GetTextCtrl()->SetFocus();
    // whm 3Aug2018 modified for latest protocol of only selecting all when
    // user has set App's m_bSelectCopiedSource var to TRUE by ticking the
    // View menu's 'Select Copied Source' toggle menu item. 
    int len = this->GetTextCtrl()->GetValue().Length();
	// Never select phrasebox contents when there a > 1 items in list.
	if (this->GetDropDownList()->GetCount() > 1)
    {
		// The dropdown list has more than one item so it will be open and displaying its
		// list items. In this situation we set the cursor to the end of the text, but
		// not during editing - i.e., within the OnPhraseBoxChanged(), otherwise the cursor
		// won't behave during editing, deletions, backspaces, etc.
        // Set insertion point at end of text, except when this SetFocusAndSetSelectionAtLanding()
		// function is called while within the OnPhraseBoxChanged() method.
		// whm 25Jan2023 added. Fix for curson jumping to end of word while within
		// OnPhraseBoxChanged() when a phrasebox size change is done. This change
		// combined with the call of SetSelection(nStartChar, nEndChar) command, and 
		// the setting of the App's m_nStartChar and m_nEndChar to the local values 
		// nStartChar and nEndChar within OnPhraseBoxChanged() - fixed the problem.
		if (!gpApp->GetLayout()->m_bAmWithinPhraseBoxChanged)
		{
			this->GetTextCtrl()->SetSelection(len, len);
		}
    }
    else
    {
        // Only select all if user has ticked the View menu's 'Select Copied Source' toggle menu item.
		// whm 15Dec2022 modified. Don't select all if the SetFocusAndSetSelectionAtLanding() was called
		// from within the OnPhraseBoxChanged() since this can potentially cause disruption during
		// Backspace key operations when the phraseBox changes and the next Backspace key stroke would
		// then remove all of the remaining text from the control leaving it empty instead of rubbing 
		// out the character the previous character.
        if (gpApp->m_bSelectCopiedSource && !gpApp->GetLayout()->m_bAmWithinPhraseBoxChanged)
            this->GetTextCtrl()->SetSelection(-1, -1); // select it all
        else
            this->GetTextCtrl()->SetSelection(len, len); // Set insertion point at end of text.
    }
}
