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
/// BEW 12Apr10 all changes needed for support of _DOVCER5 in this file are done
/// GDLC 20Apr10 Removed all local variables pApp because this class has member variable m_pApp
/////////////////////////////////////////////////////////////////////////////

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
#include "KB.h"
#include "Layout.h"
#include "MergeUpdatedSrc.h"
#include "helpers.h"
#include "MainFrm.h"
#include "Adapt_ItCanvas.h"
#include "FreeTrans.h"
#include "Adapt_ItDoc.h"
#include "CollectBacktranslations.h"

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
/// into the glossing KB unless gbGlossingVisible is also TRUE.
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

// next two only needed for making wxLogDebug calls work; when _V6PRINT is commented out
// in the header file, these two can be also
extern bool gbIsPrinting;
extern bool gbCheckInclFreeTransText;

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
	EVT_MENU(ID_ADVANCED_REMOVE_FILTERED_FREE_TRANSLATIONS, CFreeTrans::OnAdvancedRemoveFilteredFreeTranslations)
	EVT_UPDATE_UI(ID_ADVANCED_REMOVE_FILTERED_FREE_TRANSLATIONS, CFreeTrans::OnUpdateAdvancedRemoveFilteredFreeTranslations)

	// for collected back translations support
	EVT_MENU(ID_ADVANCED_REMOVE_FILTERED_BACKTRANSLATIONS, CFreeTrans::OnAdvancedRemoveFilteredBacktranslations)
	EVT_UPDATE_UI(ID_ADVANCED_REMOVE_FILTERED_BACKTRANSLATIONS, CFreeTrans::OnUpdateAdvancedRemoveFilteredBacktranslations)
	EVT_UPDATE_UI(ID_ADVANCED_COLLECT_BACKTRANSLATIONS, CFreeTrans::OnUpdateAdvancedCollectBacktranslations)
	EVT_MENU(ID_ADVANCED_COLLECT_BACKTRANSLATIONS, CFreeTrans::OnAdvancedCollectBacktranslations)
	// end collected back translations support

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
	// NOTE: This assumes that the view, the frame, and the layout were all created
	// prior to the app calling this constructor. If the order of creation is ever
	// changed, this constructor would need to be revised to suit.
	m_pView = m_pApp->GetView();
	wxASSERT(m_pView != NULL);
	m_pFrame = m_pApp->GetMainFrame();
	wxASSERT(m_pFrame != NULL);
	m_pLayout = m_pApp->GetLayout();
	wxASSERT(m_pLayout != NULL);
}

CFreeTrans::~CFreeTrans()
{
	// Clear and delete the arrays for FreeTrSections and Piles
	m_pCurFreeTransSectionPileArray->Clear();
	delete m_pCurFreeTransSectionPileArray;
	m_pFreeTransArray->Clear();
	delete m_pFreeTransArray;
}

// BEW 19Feb10 no changes needed for support of doc version 5
wxString CFreeTrans::ComposeDefaultFreeTranslation(wxArrayPtrVoid* arr)
{
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
		if (m_pApp->m_bTargetIsDefaultFreeTrans)
		{
			// get the text from the adaptation line's contents (exclude punctuation)
			theText = ((CPile*)arr->Item(index))->GetSrcPhrase()->m_adaption;
		}
		else if (m_pApp->m_bGlossIsDefaultFreeTrans)
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
/// BEW 22Feb10 changes needed for support of doc version 5. To support empty free translation
/// sections we need to also test for m_bHasFreeTrans with value TRUE; and for docVersion
/// = 5, we look for content in the m_freeTrans member, no longer in m_markers
/////////////////////////////////////////////////////////////////////////////////
bool CFreeTrans::ContainsFreeTranslation(CPile* pPile)
{
	CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
	if (pSrcPhrase->m_bHasFreeTrans || !pSrcPhrase->GetFreeTrans().IsEmpty())
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return             TRUE if anywhere in the passed in array, a CSourcePhrase
///                     instance has an indication of a free translation (even if
///                     it's empty); FALSE otherwise
/// \param	pSPArray ->	pointer to an SPArray of CSourcePhrase instances, (not necessarily
///                     formed from m_pSourcePhrases list, but often will be)
/// \remarks
/// Used in the Import Edited Source Text... feature, and also when re-loading source text
/// which has been adapted and the user has edited the source in Paratext, so that it
/// needs merging with the former adaptation document for that chapter.
/// This function detects the presence of free translations in the passed in SPArray.
/////////////////////////////////////////////////////////////////////////////////
bool CFreeTrans::IsFreeTransInArray(SPArray* pSPArray)
{
	int count = pSPArray->GetCount();
	int index;
	CSourcePhrase* pSrcPhrase = NULL;
	for (index = 0; index < count; index++)
	{
		pSrcPhrase = pSPArray->Item(index);
		if (pSrcPhrase->m_bHasFreeTrans || !pSrcPhrase->GetFreeTrans().IsEmpty())
		{
			return TRUE;
		}
	}
	// didn't find the flag value TRUE anywhere, nor free trans text member non-empty
	return FALSE;
}
// use this one only while we continue using SPList in the app, later I'll refactor to
// only use SPArray, which is much easier to use
bool CFreeTrans::IsFreeTransInList(SPList* pSPList)
{
	CSourcePhrase* pSrcPhrase = NULL;
	SPList::Node* pos = pSPList->GetFirst();
	while (pos != NULL)
	{
		pSrcPhrase = pos->GetData();
		if (pSrcPhrase->m_bHasFreeTrans || !pSrcPhrase->GetFreeTrans().IsEmpty())
		{
			return TRUE;
		}
		pos = pos->GetNext();
	}
	// didn't find the flag value TRUE anywhere, nor free trans text member non-empty
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return             nothing
/// \param	pSPArray ->	pointer to an SPArray of CSourcePhrase instances, (not necessarily
///                     formed from m_pSourcePhrases list, but often will be)
/// \remarks
///  Used in the Import Edited Source Text... feature, and also when re-loading source text
///  which has been adapted and the user has edited the source in Paratext, so that it
///  needs merging with the former adaptation document for that chapter.
///  The SetupCurrentFreeTransSection() function in FreeTrans.cpp uses the flag values
///  m_bHasFreeTrans, m_bStartFreeTrans, m_bEndFreeTrans, to delineate a pre-composed free
///  translation section, but if the user has edited the source text externally and
///  reimported it to the document, the imported new CSourcePhrase instances which are the
///  result of his edit actions won't have any free translation information within them,
///  and so when they are merged into the old adaptation document they can do the following
///  destructive things: (i) erase the start of a free translation section, (ii) erase the
///  end of a free translation section, (iii) erase the middle of a free translation
///  section (that is, insert CSourcePhrase instances there which have m_bHasFreeTrans set
///  FALSE). This function scans the inventory of CSourcePhrase instances and erases any
///  malformed free translations resulting from (i) (ii) or (iii) above. This accomplishes
///  an import need, a source text edit which replaces some existing source text (in the
///  import process) invalidates a free translation defined over that location - hence the
///  free translation should be removed, forcing the user to re-do it with the correct new
///  meaning, if one is still wanted at that location.
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::EraseMalformedFreeTransSections(SPArray* pSPArray)
{
	CSourcePhrase* pSrcPhrase = NULL;
	int count = pSPArray->GetCount();
	int arrayEndIndex = count - 1;
	int index = 0;
	int index2 = wxNOT_FOUND;
	wxString emptyStr = _T("");
	int anIndex = wxNOT_FOUND;
	int nEndsAt = wxNOT_FOUND;
	int malformedAt = wxNOT_FOUND;
	int nStartAt = index; 
	bool bHasFreeTransFlagIsUnset = FALSE;
	bool bLacksEnd = FALSE;
	bool bFoundArrayEnd = FALSE;
	bool bWellFormed = TRUE;
	//	pSrcPhrase = pSPArray->Item(index);

	while (index < count)
	{
		nStartAt = FindNextFreeTransSection(pSPArray, index);
		if (nStartAt == wxNOT_FOUND)
		{
            // There are no more instances with m_bStartFreeTrans = TRUE from the index
            // location up to the end of the array. If there were no free translations in
            // that span originally, then there is no problem. But if a user edit of the
            // source text prior to importing the changed source text caused the loss of
            // the start of a retranslation to happen within that span, we must detect it
            // and clear the relevant flags up to the end of the array.
			anIndex = FindFreeTransSectionLackingStart(pSPArray, index);
			if (anIndex == wxNOT_FOUND)
			{
				// nowhere in that span, up to the end of the array, did we find an
				// instance with m_bHasFreeTrans = TRUE. So just ensure that
				// m_bEndFreeTrans is FALSE from index to the array end, and return - we
				// are done
				int index2;
				for (index2 = index; index2 < count; index2++)
				{
					pSrcPhrase = pSPArray->Item(index2);
					pSrcPhrase->m_bEndFreeTrans = FALSE;
				}
				return;
			}
			else
			{
                // anIndex must be positive, which means we found a place where the start
                // of a free translation section has been lost, that is, m_bHasFreeTrans is
                // TRUE but m_bStartFreeTrans is FALSE. Those facts mean the beginning of
                // the last free translation was replaced by new CSourcePhrase instances
                // arising from the user's editing of the source text prior to import back
                // into Adapt It. We have to therefore clear all three flags right up to
                // the end of the array. No instance in that span will have any content in
                // m_freeTrans, but we may as well clear it each time to play safe.
				for (index2 = index; index2 < count; index2++)
				{
					pSrcPhrase = pSPArray->Item(index2);
					pSrcPhrase->m_bEndFreeTrans = FALSE;
					pSrcPhrase->m_bStartFreeTrans = FALSE;
					pSrcPhrase->m_bHasFreeTrans = FALSE;
					pSrcPhrase->SetFreeTrans(emptyStr);
				}
				return;
			}
		}
        // nStartAt returned a positive value, so a CSourcePhrase instance with
        // m_bStartFreeTrans = TRUE was found above. The next call starts from the same
        // index value and looks for m_bHasFreeTrans TRUE and m_bStartFreeTrans FALSE. If
        // such an instance precedes the one found by the FindNextFreeTransSection() call
        // above, then it is indeed a malformed free translation which we must deal with,
        // but if it just finds the instance immediately following the one which has
        // m_bStartFreeTrans TRUE (and m_bHasFreeTrans would also be TRUE), then that is
        // not an indication of malformation
		anIndex = FindFreeTransSectionLackingStart(pSPArray, index);
		if (anIndex != wxNOT_FOUND && anIndex < nStartAt )
		{
			// anIndex is positive, and it is earlier than the location at which the next
			// free translation section starts, so we've a malformation to deal with, and
			// the malformation is an old free trans section which has lost it's start.
			// We now where this malformation starts (at anIndex) and where it ends (at
			// nStartAt - 1), so we just clear all 3 flags for the instances in this span 
			for (index2 = anIndex; index2 < nStartAt; index2++)
			{
				pSrcPhrase = pSPArray->Item(index2);
				pSrcPhrase->m_bEndFreeTrans = FALSE;
				pSrcPhrase->m_bStartFreeTrans = FALSE;
				pSrcPhrase->m_bHasFreeTrans = FALSE;
				pSrcPhrase->SetFreeTrans(emptyStr);
			}
			// set index to the kick-off location for the next search forwards & iterate
			index = nStartAt;
		}
		else
		{
            // the nStartAt location is potentially the start of a well-formed free
            // translation section - so check for well-formedness etc. Since there could be
            // a malformation before the end or at the end of the free translation section
            // just located, we must check - and determine from the values passed back how
            // to fix the document if malformation is detected for this section
            index = nStartAt;
			nEndsAt = wxNOT_FOUND;
			malformedAt = wxNOT_FOUND;
			bWellFormed = CheckFreeTransStructure(pSPArray, index, nEndsAt, malformedAt,
								bHasFreeTransFlagIsUnset, bLacksEnd, bFoundArrayEnd);
			if (bWellFormed)
			{
				// no problem with this free translation section, so position index to
				// point beyond it
				if (nEndsAt + 1 <= arrayEndIndex)
				{
					index = nEndsAt + 1; // iterate
				}
				else
				{
					return; // we are done
				}
			}
			else
			{
				// the free translation section contains or ends with a malformation
				if (bHasFreeTransFlagIsUnset && bFoundArrayEnd && bLacksEnd)
				{
					// this free translation starts, but it doesn't end properly, so the
					// user's edits have messed with the end - we have to clear this one,
					// and the flags to the end of the array, & return
					for (index2 = nStartAt; index2 <= arrayEndIndex; index2++)
					{
						pSrcPhrase = pSPArray->Item(index2);
						pSrcPhrase->m_bEndFreeTrans = FALSE;
						pSrcPhrase->m_bStartFreeTrans = FALSE;
						pSrcPhrase->m_bHasFreeTrans = FALSE;
						pSrcPhrase->SetFreeTrans(emptyStr);
					}
					return;
				}
				else if (bHasFreeTransFlagIsUnset && !bLacksEnd)
				{
					// the malformation is confined within the section, so just clear this
					// section, nEndsAt will index where the instance with m_bEndFreeTrans
					// is TRUE
					for (index2 = nStartAt; index2 <= nEndsAt; index2++)
					{
						pSrcPhrase = pSPArray->Item(index2);
						pSrcPhrase->m_bEndFreeTrans = FALSE;
						pSrcPhrase->m_bStartFreeTrans = FALSE;
						pSrcPhrase->m_bHasFreeTrans = FALSE;
						pSrcPhrase->SetFreeTrans(emptyStr);
					}
					index = nEndsAt + 1; // iterate
				}
				else if (bLacksEnd)
				{
                    // clear, the same as above, but only as far as the endsAt value, then
                    // iterate after setting index to the new kick-off location
					for (index2 = nStartAt; index2 <= nEndsAt; index2++)
					{
						pSrcPhrase = pSPArray->Item(index2);
						pSrcPhrase->m_bEndFreeTrans = FALSE;
						pSrcPhrase->m_bStartFreeTrans = FALSE;
						pSrcPhrase->m_bHasFreeTrans = FALSE;
						pSrcPhrase->SetFreeTrans(emptyStr);
					}
					index = nEndsAt + 1; // iterate
				}
			}
		} // end of else block for test: if (anIndex != wxNOT_FOUND && anIndex < nStartAt )
	} // end of loop: while (index < count)
}

// return wxNOT_FOUND if a section begining with m_bStartFreeTrans set TRUE is not found,
// otherwise return the index for the location of that particular CSourcePhrase
int CFreeTrans::FindNextFreeTransSection(SPArray* pSPArray, int startFrom)
{
	int index = wxNOT_FOUND;
	int count = pSPArray->GetCount();
	for (index = startFrom; index < count; index++)
	{
		if ((pSPArray->Item(index))->m_bStartFreeTrans)
		{
			return index;
		}
	}
	return wxNOT_FOUND;
}

// variant of the above, working with pLayout and it's m_pileArray, and returning the
// CPile pointer for the next free translation anchor, or NULL if there isn't one; start
// looking at the passed in pStartingPile, and it can legally be an anchor pile - and if
// so, the next anchor pile is what is looked for
CPile* CFreeTrans::FindNextFreeTransSection(CPile* pStartingPile)
{
	CPile* pPile = pStartingPile;
	CSourcePhrase* pSrcPhrase = NULL;
	do {
		pPile = m_pView->GetNextPile(pPile);
		if (pPile == NULL)
			return pPile;
		pSrcPhrase = pPile->GetSrcPhrase();
	} while (!pSrcPhrase->m_bStartFreeTrans);
	return pPile;
}

// returning the CPile pointer for the end of the free translation section which
// pStartingPile must be in - either at it's start or at some intermediate pile, or at the
// ending pile; or NULL if an end could not be found
CPile* CFreeTrans::FindFreeTransSectionEnd(CPile* pStartingPile)
{
	CPile* pPile = pStartingPile;
	CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
	if (pSrcPhrase->m_bEndFreeTrans)
	{
		return pPile;
	}
	else
	{
		do {
			pPile = m_pView->GetNextPile(pPile);
			if (pPile == NULL)
			{
				return pPile; // remember to test for NULL in the caller
			}
			pSrcPhrase = pPile->GetSrcPhrase();
			wxASSERT(pSrcPhrase->m_bHasFreeTrans);
		} while (!pSrcPhrase->m_bEndFreeTrans);
		return pPile; // this is the section's end pile
	}
}


// return the index of the CSourcePhrase on which m_bHasRetranslation is set TRUE, but the
// value of m_bStartFreeTrans is FALSE; otherwise, return wxNOT_FOUND if a valid start of
// a later free translation section is encountered, or the doc end. The nextStartFreeTransLoc
// helps us to interpret a wxNOT_FOUND value returned by the function.
int CFreeTrans::FindFreeTransSectionLackingStart(SPArray* pSPArray, int startFrom)
{
	int index = wxNOT_FOUND;
	int count = pSPArray->GetCount();
	for (index = startFrom; index < count; index++)
	{
		if ((pSPArray->Item(index))->m_bHasFreeTrans && 
			!(pSPArray->Item(index))->m_bStartFreeTrans)
		{
			return index; // returns a +ve value
		}
		else if ((pSPArray->Item(index))->m_bStartFreeTrans)
		{
			break;
		}
	}
	return wxNOT_FOUND;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return                 TRUE if the section is a well-formed free translation section,
///                         FALSE if malformed in some way
/// \param	pSPArray    ->	pointer to an SPArray of CSourcePhrase instances, (not necessarily
///                         formed from m_pSourcePhrases list, but often will be)
/// \param  startsFrom  ->  index into pSPArray where to start checking from (typically starting
///                         at the CSourcePhrase instance which has m_bStartFreeTrans set
///                         TRUE but it could be from a later index location provided the caller
///                         is sure that no malformations are at prior index values within the
///                         section
/// \param  endsAt      <-  if TRUE is returned, endsAt will have the index of the last instance
///                         within the (well-formed) section; if FALSE is returned, index
///                         will be the index of the last CSourcePhrase instance in the
///                         malformation - that is, the index following endsAt will either
///                         be the start of a new free translation section, or the index
///                         of the final CSourcePhrase in the array
/// \param  malformedAt <-  index of the CSourcePhrase at which a malformation commences - 
///                         typically the location where an instance with expected
///                         m_bHasFreeTrans TRUE is found to have the value FALSE for that
///                         member
/// \param  bHasFlagIsUnset <-  returns TRUE if a CSourcePhrase within the section has
///                             its m_bHasFreeTrans member cleared to FALSE (and the first such
///                             will have it's index assigned to malformedAt)
/// \param  bLacksEnd       <-  TRUE if a CSourcePhrase instance is encounted in which its
///                             m_bStartFreeTrans value is TRUE, or the end of the array
///                             is reached; FALSE if an instance with m_bEndFreeTrans is
///                             TRUE is encountered before one with m_bStartFreeTrans is
///                             encountered
/// \param  bFoundEndOfArray <- TRUE if no new free translation section is encountered before
///                             the end of the array is reached when looking for the location
///                             at which a malformation ends
/// \remarks
///  Used in the Import Edited Source Text... feature, and also when re-loading source text
///  which has been adapted and the user has edited the source in Paratext, so that it
///  needs merging with the former adaptation document for that chapter.
///  Malformations can be: (i) erased the start of a free translation section, (ii) erased the
///  end of a free translation section, (iii) erased the middle of a free translation
///  section (that is, inserted CSourcePhrase instances there which have m_bHasFreeTrans set
///  FALSE). This function scans the inventory of CSourcePhrase instances within a section
///  to find and return information about any malformed locations, and returns that
///  information to the caller so it can decide how to handle the problems. It is a helper
///  for the EraseMalformedFreeTransSections() function.
/////////////////////////////////////////////////////////////////////////////////
bool CFreeTrans::CheckFreeTransStructure(SPArray* pSPArray, int startsFrom, int& endsAt, int& malformedAt,
						bool& bHasFlagIsUnset, bool& bLacksEnd, bool& bFoundArrayEnd)
{
	CSourcePhrase* pSrcPhrase = NULL;
	int count = pSPArray->GetCount();
	wxASSERT(count != 0);
	// initializations
	malformedAt = wxNOT_FOUND;
	bHasFlagIsUnset = FALSE;
	bLacksEnd = FALSE;
	bFoundArrayEnd = FALSE;
	int index = startsFrom;
	pSrcPhrase = pSPArray->Item(index);
	if (pSrcPhrase->m_bEndFreeTrans)
	{
		// ends at the initial search location, so is well-formed
		endsAt = startsFrom;
		return TRUE;
	}
	int arrayEndIndex = count - 1;
	// scan forwards so long as the m_bHasFreeTrans flag continues to be TRUE, and the
	// array end bound is not breached, and neither of the end of the free translation 
	// (nor the start of a following one is reached)
	bool bIsFirst = TRUE;
	while (index < count && !pSrcPhrase->m_bEndFreeTrans && pSrcPhrase->m_bHasFreeTrans)
	{
		if (!bIsFirst)
		{
			if (pSrcPhrase->m_bStartFreeTrans)
			{
				// we've come to the start of a later free translation without
				// encountering the end of the current one, this indicates malformation,
				// so exit the loop
				break;
			}
		}
		else
		{
			// on first iteration, set the flag to FALSE for later iterations
			bIsFirst = FALSE;
		}
		index++;
		if (index > arrayEndIndex)
		{
			break;
		}
		pSrcPhrase = pSPArray->Item(index);
	}
	// determine what halted the forwards movement...
	// First, did we come to an instance with m_bEndFreeTrans set TRUE - if so, it's a
	// well-formed section
	if (pSrcPhrase->m_bEndFreeTrans)
	{
		endsAt = index;
		return TRUE;
	}
	// We didn't come to one with m_bEndFreeTrans set TRUE, so did we get all the way to
	// the end of the array? If so, we've no instance with m_bEndFreeTrans set TRUE, which
	// means the section is malformed
	if (index > arrayEndIndex)
	{
		endsAt = arrayEndIndex;
		malformedAt = arrayEndIndex;
		bFoundArrayEnd = TRUE;
		bLacksEnd = TRUE;
		return FALSE;
	}
	// Did we instead come to the start of a new free translation section? If so, we
	// didn't encounter the end of the one we are checking, and so this too is a
	// malformation (the m_bHasFreeTrans values must have all been TRUE up to and
	// including the CSourcePhrase immediately prior to the index value on exit from the
	// loop)
	if (pSrcPhrase->m_bStartFreeTrans)
	{
		endsAt = index - 1;
		malformedAt = endsAt;
		bLacksEnd = TRUE;
		return FALSE;
	}
	// Finally, was there an embedded section of new SourcePhrase instances in which they
	// each have their m_bHasFreeTrans member cleared to FALSE? If so, the structure is
	// malformed and we'll call another function to try find where things return to
	// well-formedness - for example, the inserted material may lie wholely within the
	// free translation section so that a well-defined end-of-section location still
	// exists, or the insertions may have wiped out the end, or even encroached into one
	// or more following free translation sections (which would have lost the text in any
	// m_freeTrans members which were non-empty within that replaced subspan)
	if (!pSrcPhrase->m_bHasFreeTrans)
	{
		malformedAt = index;
		bHasFlagIsUnset = TRUE;
		// define booleans to pass in to the function which finds the end of the ruined
		// section
		bool bFoundSectionEnd;
		bool bFoundSectionStart;
		bool bFoundEndOfArray;
		int nStartAt = malformedAt; // superflous, but documents what's happening better
		int anEndIndex = FindEndOfRuinedSection(pSPArray, nStartAt, bFoundSectionEnd,
										bFoundSectionStart, bFoundEndOfArray);
		// The first thing to look for is the ruined span of instances being contained
		// wholely within the original free translation section - the condition for that
		// is that an instance with m_bEndFreeTrans = TRUE is found before finding one
		// with m_bStartFreeTrans = TRUE, or before the end of the array is reached
		if (bFoundSectionEnd)
		{
			endsAt = anEndIndex;
			return FALSE;
		}
		// Next, check if we stopped at the start of a later free translation section
		if (bFoundSectionStart)
		{
			endsAt = anEndIndex;
			bLacksEnd = TRUE;
			return FALSE;
		}
		// Finally, check if we didn't find any further free translation starting
		// CSourcePhrase instances before coming to the array end
		if (bFoundEndOfArray)
		{
			endsAt = arrayEndIndex; // same as is returned in anEndIndex
			bLacksEnd = TRUE;
			bFoundArrayEnd = TRUE;
		}
	}
	return FALSE;
}

// Returns the index of a CSourcePhrase instance in pSPArray - it's interpretation depends
// on the values returned by the booleans: if bFoundSectionEnd is TRUE, the return value
// is the index of the instance with it's m_bEndFreeTrans member set TRUE; if bFoundArrayEnd
// is TRUE, the return value is the pSPArray->GetCount()-1; if bFoundSectionStart is TRUE,
// then the return value is the index of the instance IMMEDIATELY BEFORE the instance with
// it's m_bStartFreeTrans member set TRUE. One of the 3 booleans will return TRUE, and the
// caller will use the returned integer to clear all the flags, m_bHasFreeTrans,
// m_bStartFreeTrans, and m_bEndFreeTrans, to FALSE up to and including the returned integer 
// value.
int CFreeTrans::FindEndOfRuinedSection(SPArray* pSPArray, int startFrom, bool& bFoundSectionEnd,
										bool& bFoundSectionStart, bool& bFoundArrayEnd)
{
	CSourcePhrase* pSrcPhrase = NULL;
	bFoundSectionEnd = FALSE;
	bFoundArrayEnd = FALSE;
	bFoundSectionStart = FALSE;
	int count = pSPArray->GetCount();
	wxASSERT(count != 0);
	int index;
	for (index = startFrom; index < count; index++)
	{
		pSrcPhrase = pSPArray->Item(index);
		if (pSrcPhrase->m_bEndFreeTrans)
		{
			bFoundSectionEnd = TRUE;
			return index;
		}
		else if (pSrcPhrase->m_bStartFreeTrans)
		{
			bFoundSectionStart = TRUE;
			return index;
		}
	}
	// didn't find a section end or start, and so came to the end of the array
	bFoundArrayEnd = TRUE;
	return count - 1;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return                         nothing
/// \param  pLayout            ->   pointer to the single CLayout instance created at app
///                                 initialization time (it contains members we need, such as the pile
///                                 list, the current PageOffsets struct for the page being
///                                 printed or print previewed (see m_pOffsets member), and the
///                                 piles in its m_pileList give access to the partner
///                                 CSourcePhrase instances which contain the free
///                                 translation-related flags we need to test
/// \param  arrPileSets         <-  reference to a wxArrayPtrVoid storing zero or more
///                                 wxArrayPtrVoid pointers (zero if the current page happens
///                                 to lack any free translations); each of the stored arrays
///                                 stores the consecutive set of CPile pointers which
///                                 comprise a single free translation section (beware,
///                                 there may be 'gaps' that is, sequences of CPile
///                                 pointers which have no free translation defined,
///                                 interspersed between free translation sections - so we
///                                 can't assume that the next section begins at the pile
///                                 immediately following the pile at the end of the
///                                 previous section
/// \param  arrFreeTranslations <-  the ordered set of free translation strings which are
///                                 the free translations corresponding to each of the
///                                 wxArrayPtrVoid CPile ptr collections stored in arrPileSets
/// \remarks
///  Used in the printing of Free Translations feature, so called in the function 
///  DrawFreeTranslationsForPrinting() which is called only when gbIsPrintint is TRUE.
///  The pile sets are collected, the first may overlap the boundary between the current
///  page and the previous page; the last may overlap the boundard between the end of the
///  current page and the start of the next page. 
///  This function is the first of three. After this returns, another function
///  will obtain in a similar fashion, an array of arrays, the stored arrays being for
///  "draw rectangles" for the free translations belonging to the page being printed. 
///  After that a third function will do any necessary text segmenting, and draw all the
///  free translations, or their apportioned parts, in the various rectangles belonging to
///  the current page being printed.
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::GetFreeTransPileSetsForPage(CLayout* pLayout, wxArrayPtrVoid& arrPileSets,
											 wxArrayString& arrFreeTranslations)
{
	arrPileSets.Clear();
	arrFreeTranslations.Clear();

	PileList* pPileList = pLayout->GetPileList();
	if (pPileList->GetCount() == 0)
		return;
	wxASSERT(pLayout->m_pOffsets != NULL);
	// first strip to be printed for this page
	int nIndexOfFirstStrip = pLayout->m_pOffsets->nFirstStrip; 
	// last strip to be printed for this page
	int nIndexOfLastStrip = pLayout->m_pOffsets->nLastStrip;

#ifdef _V6PRINT
#ifdef __WXDEBUG__
	{ // confine their scope to this conditional block
		wxLogDebug(_T("\nGetFreeTransPileSetsForPage(),  nIndexOfFirstStrip = %d  ,  nIndexOfLastStrip = %d"),
					nIndexOfFirstStrip, nIndexOfLastStrip); 
	}
#endif
#endif

	// various needed scratch variables
	CPile* pPile;
	CPile* pAnchorPile;
	CPile* pPileEndingFTSection; // for a given pAnchorPile, that section's ending pile 
								 // is to be stored in pPileEndingFTSection
	CStrip* pStrip;
	CSourcePhrase* pSrcPhrase; 
	int curStripIndex;
	int laterStripIndex;
	wxArrayPtrVoid* pPileSetArray = NULL; // we'll create these as needed on the heap 
                    // & store them in arrPileSets; each one will store the CPile ptrs for
                    // the piles in a single free translation whole section, which may or
                    // may not overlap into the preceding or following page
	wxArrayPtrVoid* pStripArray = pLayout->GetStripArray();

    // The first task is to find the anchor pile for the first free translation (if any,
    // the page may in fact not have any) on the page. Beware, the anchor pile may lie on
    // the previous page - we still must get it all of such the free translation, but we
    // won't draw the free translation rectangles which lie on the previous page
	pStrip = (CStrip*)(*pStripArray)[nIndexOfFirstStrip];
	curStripIndex = pStrip->GetStripIndex();
	pPile = pStrip->GetPileByIndex(0); // the first pile in the first strip of the page
	// At this point, it would be nice if pPile is also the anchor pile, both often that
	// won't be true; we may have to search backwards in the pile list to get to the
	// anchor point, if pPile is in a free translation but doesn't start it; or search
	// forwards to get the first anchor point, if pPile doesn't have a free translation
	// defined on it. Handle these options now
	pSrcPhrase = pPile->GetSrcPhrase();
	pAnchorPile = NULL;
	if (pSrcPhrase->m_bStartFreeTrans)
	{
		pAnchorPile = pPile;
	}
	else if (pSrcPhrase->m_bHasFreeTrans)
	{
		// pPile is somewhere within a free translation section, so search back for the
		// anchor - it has to be there somewhere
		CSourcePhrase* pSPhr;
		do {
			pPile = m_pView->GetPrevPile(pPile);
			pSPhr = pPile->GetSrcPhrase();
			wxASSERT(pSPhr->m_bHasFreeTrans);

		} while(!pSPhr->m_bStartFreeTrans);
		pAnchorPile = pPile;
	}
	else
	{
		// no free translation is defined at the first pile of the page, so if it exists
		// then it must lie ahead somewhere - but it might be beyond the page's end
		pPile = FindNextFreeTransSection(pPile);
		if (pPile == NULL)
		{
			// we couldn't find a free translation section anywhere ahead, so return
			// without doing anything
			return;
		}
		// pPile is an anchor location, see if it falls within the current page
		laterStripIndex = pPile->GetStripIndex();
		if (laterStripIndex > nIndexOfLastStrip)
		{
			// this free translation lies beyond the end of the current page for printing,
			// so ignore it - and return without doing anything
			return;
		}
		// it's on the current page, so set the anchor pile location
		pAnchorPile = pPile;
	}

	// Having found the first anchor pile which has free translation content in the
	// current page - we now must loop to find each successive anchor pile (each defines a
	// new free translation section) within the same print page. It's "off page" if it is
	// an anchor pile within a strip with index greater than nIndexOfLastStrip - that's
	// what will have us break out of the loop. Beware, the free translation sections may
	// not butt up against each other, so for each anchor pile, we must find where that
	// section ends - ie. at which pile it ends; then search ahead for the next, etc.
	// Within each section, we must grab the CPile pointers and store them in sequence in
	// a wxArrayPtrVoid on the heap, one such for each section, and store each such array
	// in natural order in the passed in array, arrPileSets. Then we can exit.
	wxASSERT(pAnchorPile != NULL);
	do {
		pPile = pAnchorPile; // section's first CPile, this one stores the free trans text

		pPileSetArray = new wxArrayPtrVoid; // for storing the CPile pointers of the section
											// being delineated
		pSrcPhrase = pPile->GetSrcPhrase(); // anchor pile's CSourcePhrase instance
		wxString strFreeTrans = pSrcPhrase->GetFreeTrans();
		arrFreeTranslations.Add(strFreeTrans); // preserve the free translation text
		arrPileSets.Add(pPileSetArray); // preserve the wxArrayPtrVoid* which will be
										// populated with the section's CPile pointers
		// get the section's ending CPile pointer
		pPileEndingFTSection = FindFreeTransSectionEnd(pAnchorPile);
		wxASSERT(pPileEndingFTSection != NULL);
		// inner loop, to collect the piles for this section
		while (pPile != pPileEndingFTSection)
		{
			pPileSetArray->Add(pPile);
			// get the next pile & iterate
			pPile = m_pView->GetNextPile(pPile);
		}
		// Add() the ending pile to the array
		pPileSetArray->Add(pPileEndingFTSection);

#ifdef _V6PRINT
#ifdef __WXDEBUG__
	{ // confine their scope to this conditional block
		wxLogDebug(_T("GetFreeTransPileSetsForPage(),  do loop iter: Piles in section = %d  ,  free translation = %s"),
			pPileSetArray->GetCount(), strFreeTrans.c_str()); 
	}
#endif
#endif
		// Now we need to determine if the curStripIndex, based on which strip
		// pPileEndingFTSection happens to be in, is within the Page to be printed or not.
		// If it is within the section, we'll iterate the outer loop if we can find a
		// further free translation section somewhere ahead of the pPileEndingFTSection
		// pile which happens to have its anchor pile within the Page; but if not, we've
		// no more free translation sections to be found for this print Page, and we can
		// return to the caller
		curStripIndex = pPileEndingFTSection->GetStripIndex();
		if (curStripIndex > nIndexOfLastStrip)
		{
			// the end of the just-delineated free translation section is in a strip which
			// is off-Page, so we must break from the loop, we've no more sections to find
			// for this Page
			break;
		}
		else
		{
			// the free translation end is in one of the strips belonging to this print
			// Page, so prepare for iterating the outer loop if we can find another
			// anchor pile ahead, but the anchor pile must be within the Page - if not,
			// we'll break from the outer loop
			pPile = FindNextFreeTransSection(pPileEndingFTSection);
			if (pPile == NULL)
			{
				// there are no more anchor piles in the document, so break from outer loop
				break;
			}
			else
			{
				// we've found a later anchor - check if it is in the Page's strips
				laterStripIndex = pPile->GetStripIndex();
				if (laterStripIndex > nIndexOfLastStrip)
				{
					// it's in an "off-Page" strip, so break from the outer loop
					break;
				}
				else
				{
					// the new anchor pile is within the Page, so set up to get it's CPile
					// set, and iterate the outer loop
					pAnchorPile = pPile;
#ifdef _V6PRINT
#ifdef __WXDEBUG__
	{ // confine their scope to this conditional block
		CSourcePhrase* pSP = pPile->GetSrcPhrase();
		wxLogDebug(_T("GetFreeTransPileSetsForPage(),  looping: next anchor sn = %d  ,  m_key = %s"),
			pSP->m_nSequNumber, pSP->m_key.c_str()); 
	}
#endif
#endif
				}
			} // end of else block for test: if (pPile == NULL)
		} // end of else block for test: if (curStripIndex > nIndexOfLastStrip)
	} while(laterStripIndex <= nIndexOfLastStrip);
}

//////////////////////////////////////////////////////////////////////////////////////////
/// \return                 nothing
/// \param arrPileSets  ->  An array of arrays. Each stored array contains the piles, in
///                         natural order, pertaining to a single free translation
/// \remarks
/// Internally, it builds the display rectangles associated with each free translation
/// section - and since a section may extend over more than one strip, there could be more
/// than one display rectangle per section; the display rectangle, its extent, and the
/// index of the strip it is associated with are stored in a FreeTrElement struct, and these
/// stucts are stored in a wxArrayPtrVoid - one such array per section, with one or more
/// FreeTrElement structs in it; and all these arrays are stored, in sequence, in the private
/// member array m_pFreeTransSetsArray.
//////////////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::BuildFreeTransDisplayRects(wxArrayPtrVoid& arrPileSets)
{
	wxArrayPtrVoid* pFreeTrElementsSet = NULL; // we store these in m_pFreeTransSetsArray
	
	// create m_pFreeTransSetsArray on heap, ready for receiving pFreeTrElementsSet ptrs
	m_pFreeTransSetsArray = new wxArrayPtrVoid;

	// use the next two for the outer loop, which iterates through the set of stored
	// arrays each of which contains the CPile set for a single free translation section
	int ftSectionsCount = arrPileSets.GetCount();
	int ftSectionsIndex;

	// use the following for the particular set of CPile pointers being worked with
	wxArrayPtrVoid* pPileSet = NULL;

	// needed local variables
	CPile*  pPile; // a scratch pile pointer
	CStrip* pStrip; // a scratch strip pointer
	wxRect rect; // a scratch rectangle variable
	int curStripIndex;
	int curPileIndex_in_strip;
	int curPileCount_for_strip;
	int pileSetCount; // for inner loop
	int pileIndex; // for inner loop (independent of strip, it indexes over the
				   // CPile instances in the free trans section - which could be 
				   // less than, the same, or greater than the number of piles in
				   // the strip which contains the anchor pile)
	CSourcePhrase* pSrcPhrase;
	FreeTrElement* pElement;

	// work out if we must build for RTL layout, or LTR layout
	bool bRTLLayout = FALSE;
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

    // The outer loop iterates over the sequence of CPile sets; this function doesn't
    // access the free translations themselves (that is done in the next function, which
    // does the apportioning and drawing), we just deal with the layout and the rectangle
    // sizes it gives us to work with [- these we compute so as to be ready for the next
    // function which will get the free trans text, apportion or truncate it, or do
    // neither, and then draw to them]
	for (ftSectionsIndex = 0; ftSectionsIndex < ftSectionsCount; ftSectionsIndex++)
	{
		// get the set of CPile pointers which we are to process for the next free
		// translation section
		pPileSet = (wxArrayPtrVoid*)arrPileSets.Item(ftSectionsIndex);
		pPile = (CPile*)pPileSet->Item(0); // anchor pile for this free trans section
		pSrcPhrase = pPile->GetSrcPhrase();
		pileSetCount = pPileSet->GetCount();
		wxASSERT(pileSetCount >= 1); // must be at least one pile in a free trans section

		// create an array on the heap to store the FreeTrElement struct pointers for this
		// section & add it to the array which stores each such
		pFreeTrElementsSet = new wxArrayPtrVoid;
		m_pFreeTransSetsArray->Add(pFreeTrElementsSet); // delete all this stuff at the
											// end of DrawFreeTranslationsForPrinting()

#ifdef _V6PRINT
#ifdef __WXDEBUG__
	{ // confine their scope to this conditional block
		wxLogDebug(_T("\nBuildFreeTransDisplayRects(),  ftSectionsIndex = %d  pileSetCount = %d"),
			ftSectionsIndex, pileSetCount); 
		if (ftSectionsIndex == 1)
		{
			int break_here = 1;
		}
	}
#endif
#endif
        // create the elements (each a struct containing int horizExtent,wxRect subRect,
        // and int nStripIndex of the associated strip for the rectangle) which define the
        // places where the free translation substrings are to be written out, and
        // initialize the strip and pile parameters for the loop
		pStrip = pPile->GetStrip();
		curPileCount_for_strip = pStrip->GetPileCount();
		curStripIndex = pStrip->GetStripIndex();
		curPileIndex_in_strip = pPile->GetPileIndex();
		pElement = new FreeTrElement; // this struct is defined in CAdapt_ItView.h
		pElement->nStripIndex = curStripIndex;
		rect = pStrip->GetFreeTransRect(); // start with the full rectangle, 
										   // and reduce as required below
		if (gbRTL_Layout)
		{
			// source is to be laid out right-to-left, so free translation rectangles will be
			// altered in location from what would be the case for a LTR layout
			rect.SetRight(pPile->GetPileRect().GetRight()); // this fixes where the writable 
															// area starts (it extends leftwards)
			pileIndex = 0;
			do {
				//  is this pile the ending pile for the free translation section?
				if (pSrcPhrase->m_bEndFreeTrans || pileIndex == pileSetCount - 1)
				{
					// yes, we are dealing with the last pile of the current free
					// translation section
					
                    // whether we make the left boundary of rect be the left of the pile's
                    // rectangle, or let it be the leftmost remainder of the strip's free
                    // translation rectangle, depends on whether or not this pile is the
                    // last in the strip - find out, and set the .left parameter accordingly
					if (curPileIndex_in_strip == curPileCount_for_strip - 1)
					{
						// last pile in the strip, so use the full width (so no change to rect
						// is needed)
						;
					}
					else
					{
                        // there are more piles to the left within this strip, so terminate
                        // the rectangle at the pile's left boundary (use abs to make sure)
						rect.SetLeft(pPile->GetPileRect().GetLeft()); // this only moves the
												// rect, we have to recalc the width as well,
												// do that next
						rect.SetWidth(abs(	pStrip->GetFreeTransRect().GetRight() - 
											pPile->GetPileRect().GetLeft()));																		
					}
                    // store in the pElement's subRect member (don't compute the substring
                    // yet, to save time since the rect may not be visible), add the
                    // element to the pointer array
					pElement->subRect = rect;
					pElement->horizExtent = rect.GetWidth(); 
					pFreeTrElementsSet->Add(pElement);
					break; // out of inner loop; the call is redundant, but it adds clarity
				}
				else
				{
                    // The current pile is not the ending one for the free translation
                    // section, so check if there is a strip change here, if so restart
                    // there with a new rectangle calculation, etc, after saving the
                    // current FreeTrElement, and make a new element too. If there is no
                    // strip change here, set up for the next iteration of the inner loop
                    // (ie. get the next pile and do the initial calcs)
					if (curPileIndex_in_strip == curPileCount_for_strip - 1)
					{
                        // we are at the end of the strip, so we have to close off the
                        // current rectangle (accepting all the space that remains) and
                        // store it
						pElement->subRect = rect;
						pElement->horizExtent = rect.GetWidth(); 
						pFreeTrElementsSet->Add(pElement);

                        // we are not yet at the end of the piles for this free
                        // translation, so we can be sure there is a next pile so get it,
                        // and its sourcephrase pointer
						wxASSERT(curStripIndex < m_pLayout->GetStripCount() - 1);
						pileIndex++;
						pPile = (CPile*)pPileSet->Item(pileIndex);
						wxASSERT(pPile);
						pSrcPhrase = pPile->GetSrcPhrase();

						// initialize rect to the new strip's free translation rectangle, and
						// reinitialize the strip and pile parameters for this new strip
						pStrip = pPile->GetStrip();
						curStripIndex = pStrip->GetStripIndex();
						curPileCount_for_strip = pStrip->GetPileCount();
						curPileIndex_in_strip = pPile->GetPileIndex();
						// get a new element
						pElement = new FreeTrElement;
						pElement->nStripIndex = curStripIndex;
						rect = pStrip->GetFreeTransRect(); // rect.right is already correct,
														   // since this is pile[0]
						// this new pile might be the one for the end of the free translation
						// section, so iterate
					}
					else
					{
                        // there is at least one more pile in this strip, so get it,
                        // prepare for iteration, and don't close off the present drawing
                        // rectangle
						pileIndex++;
						pPile = (CPile*)pPileSet->Item(pileIndex);
						wxASSERT(pPile);
						pSrcPhrase = pPile->GetSrcPhrase();
						curPileIndex_in_strip = pPile->GetPileIndex();
					}
				} // end of else block for test: 
				  // if (pSrcPhrase->m_bEndFreeTrans || pileIndex == pileCount - 1)

			} while (pileIndex < pileSetCount - 1); // end of do loop

		} // end RTL layout block
		else
		{
            // LTR layout, and this is the only option for the non-unicode application, and
            // use abs to make sure that this is the ending pile for the free translation
            // section
			rect.SetLeft(pPile->GetPileRect().GetLeft()); // fixes where the writable area starts
			rect.SetWidth(abs(	pStrip->GetFreeTransRect().GetRight() - 
								pPile->GetPileRect().GetLeft())); 
			pileIndex = 0;
			do {
				//  is this pile the ending pile for the free translation section?
				if (pSrcPhrase->m_bEndFreeTrans || pileIndex == pileSetCount - 1)
				{
					// yes, we are dealing with the last pile of the current free
					// translation section
					
                    // whether we make the right boundary of rect be the end of the pile's
                    // rectangle, or let it be the remainder of the strip's free
                    // translation rectangle, depends on whether or not this pile is the
                    // last in the strip - find out, and set the .right parameter accordingly
					if (curPileIndex_in_strip == curPileCount_for_strip - 1)
					{
						// last pile in the strip, so use the full width (so no change to 
						// rect is needed)
						;
					}
					else
					{
                        // there are more piles to the right, so terminate the rectangle at
                        // the pile's right boundary
						rect.SetRight(pPile->GetPileRect().GetRight());
					}
                    // store in the pElement's subRect member (don't compute the substring
                    // yet, to save time since the rect may not be visible), add the
                    // element to the pointer array
					pElement->subRect = rect;
					pElement->horizExtent = rect.GetWidth(); 
					pFreeTrElementsSet->Add(pElement);
#ifdef _V6PRINT
#ifdef __WXDEBUG__
	{ // confine their scope to this conditional block
		wxLogDebug(_T("BuildFreeTransDisplayRects(), Last: HorizExtent %d , {L %d, T %d, W %d, H %d}  stripIndex = %d , pElement = %x , curPileCount_for_strip %d"),
			pElement->horizExtent, pElement->subRect.GetX(), pElement->subRect.GetY(), pElement->subRect.GetWidth(),
			pElement->subRect.GetHeight(), curStripIndex, pElement, curPileCount_for_strip); 
	}
#endif
#endif
					break; // out of inner loop; the call is redundant, but it adds clarity
				}
				else
				{
                    // The current pile is not the ending one for the free translation
                    // section, so check if there is a strip change here, if so restart
                    // there with a new rectangle calculation, etc, after saving the
                    // current FreeTrElement, and make a new element too. If there is no
                    // strip change here, set up for the next iteration of the inner loop
                    // (ie. get the next pile and do the initial calcs)
					if (curPileIndex_in_strip == curPileCount_for_strip - 1)
					{
                        // we are at the end of the strip, so we have to close off the
                        // current rectangle (accepting all the space that remains) and
                        // store it
						pElement->subRect = rect;
						pElement->horizExtent = rect.GetWidth();
						pFreeTrElementsSet->Add(pElement);

#ifdef _V6PRINT
#ifdef __WXDEBUG__
	{ // confine their scope to this conditional block
		wxLogDebug(_T("BuildFreeTransDisplayRects(), Closing Strip: HorizExtent %d , {L %d, T %d, W %d, H %d}  stripIndex = %d , pElement = %x , curPileCount_for_strip %d"),
			pElement->horizExtent, pElement->subRect.GetX(), pElement->subRect.GetY(), pElement->subRect.GetWidth(),
			pElement->subRect.GetHeight(), curStripIndex, pElement, curPileCount_for_strip); 
	}
#endif
#endif
                       // we are not yet at the end of the piles for this free
                        // translation, so we can be sure there is a next pile so get it,
                        // and its sourcephrase pointer
						wxASSERT(curStripIndex < m_pLayout->GetStripCount() - 1);
						pileIndex++;
						pPile = (CPile*)pPileSet->Item(pileIndex);
						wxASSERT(pPile != NULL); 
						pSrcPhrase = pPile->GetSrcPhrase();

						// initialize rect to the new strip's free translation rectangle, and
						// reinitialize the strip and pile parameters for this new strip
						pStrip = pPile->GetStrip();
						curStripIndex = pStrip->GetStripIndex();
						curPileCount_for_strip = pStrip->GetPileCount();
						curPileIndex_in_strip = pPile->GetPileIndex();
						// get a new element
						pElement = new FreeTrElement;
						pElement->nStripIndex = curStripIndex;
						rect = pStrip->GetFreeTransRect(); // rect.left is already correct, 
														   // since this is pile[0]
						// this new pile might be the one for the end of the free translation
						// section, so iterate
					}
					else
					{
                        // there is at least one more pile in this strip, so get it,
                        // prepare for iteration, and don't close off the present drawing
                        // rectangle
						pileIndex++;
						pPile = (CPile*)pPileSet->Item(pileIndex);
						wxASSERT(pPile != NULL); 
						pSrcPhrase = pPile->GetSrcPhrase();
						curPileIndex_in_strip = pPile->GetPileIndex();
					}
				} // end of else block for test: 
				  // if (pSrcPhrase->m_bEndFreeTrans || pileIndex == pileCount - 1)

			} while (pileIndex < pileSetCount); // end of do loop

		} // end LTR layout block

		// rectangle calculations are finished for this free translation section, and stored in 
		// FreeTrElement structs in pFreeTrElementsSet array, which is itself stored in the
		// m_pFreeTransSetsArray (a private member of CFreeTrans class); iterate to process
		// the next free translation section

	} // end of loop: for (frSectionsIndex = 0; ftSectionsIndex < ftSectionsCount; ftSectionsIndex++)
}

/////////////////////////////////////////////////////////////////////////////////////////
/// \return                         nothing
/// \param pDC                  ->  a device context, for drawing and measuring text extents
/// \param arrFreeTranslations  ->  the ordered array of free translations to be printed,
///                                 or displayed on virtual Print Preview pages, on the 
///                                 current page. Each is the text of a whole free translation,
///                                 unsegmented (any needed segmenting is done on the fly 
///                                 internally using SegmentFreeTranslation()).
/// \remarks
/// Prints the free translations in their display rectangles, doing any needed text
/// segmenting on the fly if a free translation spans two or more strips. Truncates when
/// text is too long for a display rectangle (it will try reducing text point size by two
/// points first, to see if truncation can be avoided). The rectanges, and the indices of
/// the strips to which they belong, come from FreeTrElement structs which are stored in an
/// array or arrays of the latter, in a private member of the class, m_pFreeTransSetsArray.
void CFreeTrans::DrawFreeTransStringsInDisplayRects(wxDC* pDC, CLayout* pLayout, 
													wxArrayString& arrFreeTranslations)
{
	int ftCount = arrFreeTranslations.GetCount();
	int ftIndex;
	int nTotalHorizExtent;

	wxArrayPtrVoid* pFTElementsSet = NULL; // stores FreeTrElement stuct ptrs

	// first strip to be printed for this page
	int nIndexOfFirstStrip = pLayout->m_pOffsets->nFirstStrip; 
	// last strip to be printed for this page
	int nIndexOfLastStrip = pLayout->m_pOffsets->nLastStrip; 

	FreeTrElement* pElement;
	wxSize extent;
	wxString ellipsis = _T("...");
#ifdef _UNICODE
	ellipsis = _T('\u2026'); // use a Unicode ellipsis, exclusively, in Unicode app
#endif
	wxString ftStr;
	wxArrayString subStrings;
	wxRect rectBounding;

	bool bRTLLayout = FALSE;
#ifdef _RTL_FLAGS
	if (m_pApp->m_bTgtRTL)
	{
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
	wxFont* pFreeTransFont = m_pApp->m_pTargetFont;
	pSaveFont = pDC->GetFont();
	pDC->SetFont(*pFreeTransFont);
	wxColour color(m_pApp->m_freeTransTextColor);
	if (!color.IsOk())
	{
		::wxBell(); 
		wxASSERT(FALSE);
	}
	pDC->SetTextForeground(color); 

	bool bTextIsTooLong = FALSE;
	int totalRects = 0;
	int length = 0;
	int curStripIndex;

	// loop starts here
	for (ftIndex = 0; ftIndex < ftCount; ftIndex++)
	{
		// get the next free translation's set of FreeTrElement structs 
		pFTElementsSet = (wxArrayPtrVoid*)m_pFreeTransSetsArray->Item(ftIndex);

		// calculate the total horizontal extent of the display rectangles
		int i;
		int ftElements = pFTElementsSet->GetCount();
		nTotalHorizExtent = 0;
		for (i = 0; i < ftElements; i++)
		{
			nTotalHorizExtent += ((FreeTrElement*)pFTElementsSet->Item(i))->horizExtent;
		}

		length = 0;
		// get the associated free translation text
		ftStr = arrFreeTranslations.Item(ftIndex);
		length = ftStr.Len();

		if (length == 0)
		{
			// there is no text to be printed (or displayed in Print Preview) 
			// so continue with the next free translation section
			continue;
		}

		// trim off any leading or trailing spaces
		ftStr.Trim(FALSE); // trim left end
		ftStr.Trim(TRUE); // trim right end
		if (ftStr.IsEmpty())
		{
			// nothing to print, so move on
			continue;
		}
#ifdef _V6PRINT
#ifdef __WXDEBUG__
	{ // confine their scope to this conditional block
		wxLogDebug(_T("\nDrawFreeTransStringsInDisplayRects(), ftCount %d , ftIndex %d , ftElements (count) %d , nTotalHorizExtent %d  ftStr = %s length (char count) %d ,  first strip indx %d , last strip indx %d"),
			ftCount, ftIndex, ftElements, nTotalHorizExtent, ftStr.c_str(), length, nIndexOfFirstStrip, nIndexOfLastStrip); 
	}
#endif
#endif
		
		// get text's extent (a wxSize object) and compare to the total horizontal extent of
		// the rectangles. also determine the number of rectangles we are to write this section
		// into, and initialize other needed data
		pDC->GetTextExtent(ftStr, &extent.x, &extent.y);
		bTextIsTooLong = extent.x > nTotalHorizExtent ? TRUE : FALSE;
		totalRects = pFTElementsSet->GetCount();

		if (totalRects < 2)
		{
			// the easiest case, the whole free translation section is contained within a 
			// single strip; get the PageOffsets struct for this single-rect free translation
			pElement = (FreeTrElement*)pFTElementsSet->Item(0);

			// get the index for the current strip
			curStripIndex = pElement->nStripIndex;

			// print the text, if it is not out of bounds
			if (curStripIndex >= nIndexOfFirstStrip && curStripIndex <= nIndexOfLastStrip)
			{
				bool bReducedSizeWillFit = FALSE; // assume it won't be enough to make it all fit
				bool bUsedReducedSize = FALSE; // assume we didn't decrease the font size
				int newPointSize = 10; // we'll just try 10 point size, 
									   // provided the font is not already smaller
				wxFont curFont = pDC->GetFont();
				int oldPointSize = curFont.GetPointSize();
				if (bTextIsTooLong)
				{
					// first try a font size reduction, it it fits it all, don't truncate
					if (newPointSize < oldPointSize)
					{
						bUsedReducedSize = TRUE;
						curFont.SetPointSize(newPointSize); // it's now 10 points

						// re-measure the horiz extent
						wxSize extent2;
						pDC->GetTextExtent(ftStr, &extent2.x, &extent2.y);
						bReducedSizeWillFit = extent2.x <= nTotalHorizExtent ? TRUE : FALSE;
					}
					if ((bUsedReducedSize && !bReducedSizeWillFit) || !bUsedReducedSize)
					{
						ftStr = TruncateToFit(pDC,ftStr,ellipsis,nTotalHorizExtent);
					}
				}
				
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
					m_pView->DrawTextRTL(pDC, ftStr, pElement->subRect);
				}
				else
				{
					pDC->DrawText(ftStr, pElement->subRect.GetLeft(), pElement->subRect.GetTop());
				}

				// restore font size, if we reduced it above
				if (bUsedReducedSize)
				{
					curFont.SetPointSize(oldPointSize); // it's now the original pointsize
				}
			}
		}
		else
		{
			// the free translation is spread over at least 2 strips - so we've more work to do
			// - call SegmentFreeTranslation() to get a string array returned which has the
			// passed in frStr cut up into appropriately sized segments (whole words in each
			// segment), truncating the last segment if not all the ftStr data can be fitted
			// into the available drawing rectangles

			// Note: in the block above, if the text didn't fit, we try to make it fit by
			// doing it at 10 pointsize (unless the pointsize is already 10 or lower) -
			// smaller than that may strain reader's eyes. Doing this squeezes some more
			// text into the single rectangle for drawing. In the present block, however,
			// the rectangles are spread over at least 2 strips, and because 80% the
			// right-hand slop is included in the non-last of multiple strips rectangles,
			// this gives some extra drawing space anyhow - so I've not bothered to alter
			// SegmentFreeTranslation to cater for a font pointsize reduction in the
			// unlikely event that it might be needed.
			SegmentFreeTranslation(pDC, ftStr, ellipsis, extent.GetWidth(), nTotalHorizExtent,
									pFTElementsSet, &subStrings, totalRects);

			// draw the substrings in their respective rectangles
			int index;
			for (index = 0; index < totalRects; index++)
			{
				// get the next element
				pElement = (FreeTrElement*)pFTElementsSet->Item(index);

				// get the substring to be drawn in its rectangle, and its strip index
				wxString s = subStrings.Item(index);
				curStripIndex = pElement->nStripIndex;

				// print this substring, if it is not out of bounds
				if (curStripIndex >= nIndexOfFirstStrip && curStripIndex <= nIndexOfLastStrip)
				{
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
						m_pView->DrawTextRTL(pDC, s, pElement->subRect);
					}
					else
					{
						pDC->DrawText(s, pElement->subRect.GetLeft(), pElement->subRect.GetTop());
					}
					// Cannot call Invalidate() or SendSizeEvent from within DrawFreeTranslations
					// because it triggers a paint event which results in a Draw() which results
					// in DrawFreeTranslations() being reentered... hence a run-on condition 
					// endlessly calling the View's OnDraw.
				}
			} // end of loop: for (index = 0; index < totalRects; index++)

			subStrings.Clear(); // clear the array ready for the next iteration
		} // end of else block for test: if (nTotalRects < 2)

	} // end of loop: for (ftIndex = 0; ftIndex < ftCount; ftIndex++)
}



/////////////////////////////////////////////////////////////////////////////////////////
/// \return                 nothing
/// \param pDC          ->  a device context, for drawing and measuring text extents
/// \param pLayout      ->  the app's CLayout instance, which contains the m_pileList
///                         which gives us both the piles and their associated CSourcePhrase
///                         instances; and it's m_pOffsets member gives us the nFirstStrip
///                         and nLastStrip index values for the current page being printed
/// \remarks
/// Internally, the first child function finds the free translation sections which are
/// contained by the page, as well as those which overlap at the pages start, and/or end;
/// finds them in their natural order, and copies their CPile pointers into a set of
/// wxArrayPtrVoid arrays, one such array per free translation section, and stores these
/// arrays within an array. The latter array is then passed to another child function,
/// which inputs the array of arrays of CPile pointers and builds the display rectangles
/// associated with each free translation section - and since a section may extend over
/// more than one strip, there could be more than one display rectangle per section; the
/// display rectangle, its extent, and the index of the strip it is associated with are
/// stored in a PageOffsets struct, and these stucts are stored in a wxArrayPtrVoid - one
/// such array per section, with one or more PageOffsets structs in it; and all these
/// arrays are stored, in sequence, in the private member array m_pFreeTransSetsArray. The
/// latter array is then passed in to a third function which takes all the collected data
/// and draws it to either the physical paper's page, or to the virtual page displayed by
/// Print Preview. It also does any necessary apportioning of free translation substrings
/// when the free translation extends over more than one strip, and any trunction need if
/// the free translation is too long to display fully in the available rectangle(s) space.
/// The drawing is suppressed if any rectangle being considered for printing is determined
/// to lie outside the bounds of the Page - the strip index stored in the PageOffsets
/// struct is used for making this test.
/// BEW created 3Oct11, cloned off of DrawFreeTranslations() and simplified and gotos
/// removed.
void CFreeTrans::DrawFreeTranslationsForPrinting(wxDC* pDC, CLayout* pLayout)
{
	PageOffsets*  pPageOffsetsStruct = pLayout->m_pOffsets; // this is the current one, 
                    // OnPrintPage(int page) has set the pLayout m_pOffsets to the current
                    // PageOffsets struct for the page which is current for printing,
                    // already
	// check we are not just doing a Recalc etc for PrintOptionsDlg population of its
	// members, in the latter's InitDialog() function we set m_pOffsets to NULL. Check for
	// this and if it is NULL, return immediately
	if (pPageOffsetsStruct == NULL)
		return; // because there are no structs to be had yet

	// get the array of arrays of piles, one array of piles per free translation; and also
	// the free translation text itself for each such on the page
	wxArrayPtrVoid arrPileSets;
	wxArrayString arrFreeTranslations;
	GetFreeTransPileSetsForPage(pLayout, arrPileSets, arrFreeTranslations);

	// Build the array of arrays of rectangles, horiz extents & associated strip's indices
	// (these 3 are members of a FreeTrElement struct) from the arrPileSets calculated
	// above. They array of arrays is a CFreeTrans private member m_pFreeTransSetsArray,
	// created on the heap internally and so needs to be deleted when
	// DrawFreeTranslationsForPrinting() is about to exit
	BuildFreeTransDisplayRects(arrPileSets);

    // Display the free translations in their rectangles (if Print Previewing), or print
    // them to a physical page -- in either case, only the rectangles which lie within the
    // print bounds of the page
	DrawFreeTransStringsInDisplayRects(pDC, pLayout, arrFreeTranslations);

	// this page is done, so clean up (don't leak memory)

	// the function destructor will remove the free translation strings from
	// arrFreeTranslations; the CPile instances in arrPileSets are copies of some of those
	// which are in CLayout::m_pileList, and so must not be deleted here, just Clear()
	// the arrays within arrPileSets, and then delete the array objects themselves
	wxArrayPtrVoid* pPileSetArray = NULL;
	int count = arrPileSets.GetCount();
	int index;
	for (index = count - 1; index >= 0; index--)
	{
		pPileSetArray = (wxArrayPtrVoid*)arrPileSets.Item(index);
		pPileSetArray->Clear();
		delete pPileSetArray;
	}

	// delete all the FreeTrElement structs from this Page's printing, and their storage
	// arrays, all of which were created on the heap
	count = m_pFreeTransSetsArray->GetCount();
	wxArrayPtrVoid* pFreeTrElementsSet = NULL;
	for (index = count - 1; index >= 0; index--)
	{
		pFreeTrElementsSet = (wxArrayPtrVoid*)m_pFreeTransSetsArray->Item(index);
		DestroyElements(pFreeTrElementsSet);
		delete pFreeTrElementsSet;
	}
	delete m_pFreeTransSetsArray;
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
/// BEW 19Feb10, updated for support of doc version 5 (one change, elimination of
/// GetExistingMarkerContent() call by making GetFreeTrans() call)
/// BEW 9July10, no changes needed for support of kbVersion 2
/// BEW refactored 4Oct11, cloned off of DrawFreeTranslations() and simplified and gotos
/// removed. It "blicks" at every keystroke - at first I thought it was the
/// ScrollIntoView() calls I put in, but not so. So one day it needs some attention to set
/// up clipping rect but I've no time at present.
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::DrawFreeTranslationsAtAnchor(wxDC* pDC, CLayout* pLayout)
{
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
	CPile* m_pFirstPile;

	wxString ellipsis = _T("...");
	wxString ftStr;
	wxArrayString subStrings;

    // ready the drawing context - we must handle ANSI & Unicode, and for the former we use
    // TextOut() and for the latter we use DrawText() and the Unicode app can be LTR or RTL
    // script (we use same text rending directionality as the target text line) - code from
    // CCell.cpp and CText.cpp can be reused here
	// wx version note: wx version always uses DrawText
	wxRect rectBounding;
	bool bRTLLayout = FALSE;
	#ifdef _RTL_FLAGS
	if (m_pApp->m_bTgtRTL)
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
	wxFont* pFreeTransFont = m_pApp->m_pTargetFont;
	pSaveFont = pDC->GetFont();
	pDC->SetFont(*pFreeTransFont);
	wxColour color(m_pApp->m_freeTransTextColor);
	if (!color.IsOk())
	{
		::wxBell(); 
		wxASSERT(FALSE);
	}
	pDC->SetTextForeground(color);

	bool bTextIsTooLong = FALSE;
	int totalRects = 0;
	int offset = 0;
	int length = 0;
	pPile = m_pApp->m_pActivePile; // go straight there

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
	
	// return if we did not find a free translation section (never expect this) as the
	// phrase box is at an anchor location
	if (pPile == NULL)
	{
		// there are as yet no free translations in this doc, or we've come to its end
		DestroyElements(m_pFreeTransArray); // don't leak memory
		return;
	}
	pSrcPhrase = pPile->GetSrcPhrase();

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
	if (gbRTL_Layout)
	{
        // source is to be laid out right-to-left, so free translation rectangles will be
        // altered in location from what would be the case for a LTR layout
		rect.SetRight(pPile->GetPileRect().GetRight()); // this fixes where the writable 
														// area starts
		while (TRUE)
		{
			//  is this pile the ending pile for the free translation section?
			if (pSrcPhrase->m_bEndFreeTrans)
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
				pLayout->GetCanvas()->ScrollIntoView(m_pApp->m_nActiveSequNum);

				break; // exit the loop for constructing the drawing rectangles
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
					pLayout->GetCanvas()->ScrollIntoView(m_pApp->m_nActiveSequNum);

					// are there more strips? (we may have come to the end of the doc) (for
					// a partial section at doc end, we just show as much of it as we
					// possibly can)
					if (curStripIndex == pLayout->GetStripCount() - 1)
					{
						// there are no more strips, so this free translation section will be
						// truncated to whatever rectangles we've set up so far
						break;
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
						continue;
					}
				}
				else
				{
					// there is at least one more pile in this strip, so check it out
					pPile = m_pView->GetNextPile(pPile);
					wxASSERT(pPile);
					pSrcPhrase = pPile->GetSrcPhrase();
					curPileIndex = pPile->GetPileIndex();
					continue;
				}
			}

		} // end of loop: while (TRUE)

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
		rect.SetWidth(abs(pStrip->GetFreeTransRect().GetRight() - pPile->GetPileRect().GetLeft())); 
        // used abs to make sure is this pile the ending pile for the free translation
        // section?
		while (TRUE)
		{
			if (pSrcPhrase->m_bEndFreeTrans)
			{
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
				pLayout->GetCanvas()->ScrollIntoView(m_pApp->m_nActiveSequNum);

				break; // exit the loop for constructing the drawing rectangles
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
					pLayout->GetCanvas()->ScrollIntoView(m_pApp->m_nActiveSequNum);

					// are there more strips? (we may have come to the end of the doc) (for a
					// partial section at doc end, we just show as much of it as we possibly
					// can)
					if (curStripIndex == pLayout->GetStripCount() - 1)
					{
						// there are no more strips, so this free translation section will be
						// truncated to whatever rectangles we've set up so far
						break;
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
						continue;
					}
				}
				else
				{
					// there is at least one more pile in this strip, so check it out
					pPile = m_pView->GetNextPile(pPile);
					wxASSERT(pPile != NULL); 
					pSrcPhrase = pPile->GetSrcPhrase();
					curPileIndex = pPile->GetPileIndex();

					continue;
				}
			} // end of else block for test: if (pSrcPhrase->m_bEndFreeTrans)

		} // end of loop: while(TRUE)

	} // end LTR layout block

	// rectangle calculations are finished, and stored in 
	// FreeTrElement structs in m_pFreeTransArray

    // the whole or part of this section must be drawn, so do the
    // calculations now; first, get the free translation text
	pSrcPhrase = m_pFirstPile->GetSrcPhrase();
	offset = 0;
	length = 0;
	ftStr = pSrcPhrase->GetFreeTrans();
	length = ftStr.Len();

    // get text's extent (a wxSize object) and compare to the total horizontal extent of
    // the rectangles. also determine the number of rectangles we are to write this section
    // into, and initialize other needed data
	pDC->GetTextExtent(ftStr,&extent.x,&extent.y);
	bTextIsTooLong = extent.x > nTotalHorizExtent ? TRUE : FALSE;
	totalRects = m_pFreeTransArray->GetCount();

	if (totalRects < 2)
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
			m_pView->DrawTextRTL(pDC,ftStr,pElement->subRect);
		}
		else
		{
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
		SegmentFreeTranslation(pDC,ftStr,ellipsis,extent.GetWidth(),nTotalHorizExtent,
								m_pFreeTransArray,&subStrings,totalRects);
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
				m_pView->DrawTextRTL(pDC,s,pElement->subRect);
			}
			else
			{
				pDC->DrawText(s,pElement->subRect.GetLeft(),pElement->subRect.GetTop());
			}
            // Don't call Invalidate() or SendSizeEvent from within DrawFreeTranslations()
            // or DrawFreeTranslationAtAnchor(), because it triggers a paint event which
            // results in a Draw() which results in DrawFreeTranslations() being
            // reentered... hence a run-on condition endlessly calling the View's OnDraw.
		}

		subStrings.Clear(); // clear the array ready for the next iteration
	} // end of else block for test: if (totalRects < 2)

	// drawing of the one free translation being edited is done so return
	DestroyElements(m_pFreeTransArray); // don't leak memory
}

/* legacy version, of DrawFreeTranslations(), for reference purposes -- delete later on

void CFreeTrans::DrawFreeTranslations(wxDC* pDC, CLayout* pLayout, enum DrawFTCaller drawFTCaller)
{
	// the DrawFTCaller enum had 2 values,  call_from_edit and call_from_ondraw; it was
	// defined in Adapt_It.h,  but is now deleted
	
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

	CPile* m_pFirstPile;

    // get an offscreen pile from which to scan forwards for the anchor pile (this helps
    // keep draws snappy when docs get large; we don't draw below the bottom of the window
    // either)
	pPile = GetStartingPileForScan(m_pApp->m_nActiveSequNum);

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

    // ready the drawing context - we must handle ANSI & Unicode, and for the former we use
    // TextOut() and for the latter we use DrawText() and the Unicode app can be LTR or RTL
    // script (we use same text rending directionality as the target text line) - code from
    // CCell.cpp and CText.cpp can be reused here
	// wx version note: wx version always uses DrawText
	wxRect rectBounding;
	bool bRTLLayout = FALSE;
	#ifdef _RTL_FLAGS
	if (m_pApp->m_bTgtRTL)
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
	wxFont* pFreeTransFont = m_pApp->m_pTargetFont;
	pSaveFont = pDC->GetFont();
	pDC->SetFont(*pFreeTransFont);
	wxColour color(m_pApp->m_freeTransTextColor);
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
	//pDC->SetBackgroundMode(m_pApp->m_backgroundMode);
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
	// finds the next free translation section, scanning forward 
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
		pSrcPhrase->m_srcPhrase, pSrcPhrase->m_nSequNumber, m_pApp->m_nActiveSequNum, nThumbPosition_InPixels);
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

#ifdef DrawFT_Bug
				wxLogDebug(_T(" NO INTERSECTION block:  Testing, is  top %d still above window bottom %d ?"),
					pElement->subRect.GetTop(), logicalViewClientBottom);
#endif

		// for next line... view only, MM_TEXT mode, y-axis positive downwards
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
	} // end of TRUE block for test:  b:	if (!bSectionIntersects)

    // the whole or part of this section must be drawn, so do the
    // calculations now; first, get the free translation text
	pSrcPhrase = m_pFirstPile->GetSrcPhrase();
	offset = 0;
	length = 0;
	ftStr = pSrcPhrase->GetFreeTrans();
	length = ftStr.Len();

#ifdef DrawFT_Bug
	wxLogDebug(_T(" Drawing ftSstr =  %s ; for srcPhrase  %s  at sequ num  %d"), ftStr,
		pSrcPhrase->m_srcPhrase, pSrcPhrase->m_nSequNumber);
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
			m_pView->DrawTextRTL(pDC,ftStr,pElement->subRect);
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
				m_pView->DrawTextRTL(pDC,s,pElement->subRect);
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

*/

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
/// BEW 19Feb10, updated for support of doc version 5 (one change, elimination of
/// GetExistingMarkerContent() call by making GetFreeTrans() call)
/// BEW 9July10, no changes needed for support of kbVersion 2
/// BEW refactored 5Oct11, to remove gotos and simplify structure 
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::DrawFreeTranslations(wxDC* pDC, CLayout* pLayout)
{
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

	CPile* m_pFirstPile;

    // get an offscreen pile from which to scan forwards for the anchor pile (this helps
    // keep draws snappy when docs get large; we don't draw below the bottom of the window
    // either)
	pPile = GetStartingPileForScan(m_pApp->m_nActiveSequNum);

	// get it's CSourcePhrase instance
	pSrcPhrase = pPile->GetSrcPhrase(); // its pointed at sourcephrase

#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
	wxLogDebug(_T("\nDrawFreeTrans: starting pile sn = %d  srcPhrase = %s"),
		pSrcPhrase->m_nSequNumber, pSrcPhrase->m_srcPhrase.c_str());
#endif
#endif

	wxString ellipsis = _T("...");
	wxString ftStr;
	wxArrayString subStrings;
	wxRect testRect = grectViewClient; // need a local, because an intersect test
									  // changes the calling rectangle

    // ready the drawing context - we must handle ANSI & Unicode, and for the former we use
    // TextOut() and for the latter we use DrawText() and the Unicode app can be LTR or RTL
    // script (we use same text rending directionality as the target text line) - code from
    // CCell.cpp and CText.cpp can be reused here
	// wx version note: wx version always uses DrawText
	wxRect rectBounding;
	bool bRTLLayout = FALSE;
	#ifdef _RTL_FLAGS
	if (m_pApp->m_bTgtRTL)
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
	wxFont* pFreeTransFont = m_pApp->m_pTargetFont;
	pSaveFont = pDC->GetFont();
	pDC->SetFont(*pFreeTransFont);
	wxColour color(m_pApp->m_freeTransTextColor);
	if (!color.IsOk())
	{
		::wxBell(); 
		wxASSERT(FALSE);
	}
	pDC->SetTextForeground(color); 

    // the logicalViewClientBottom is the scrolled value for the top of the view window,
    // after the device context has been adjusted; this value is constant for any one call
    // of DrawFreeTranslations(); we need to use this value in some tests, because
    // grectViewClient.GetBottom() only gives what we want when the view is unscrolled --
    // no, see next comment
	// BEW changed 2Feb10, as the LHS value was way too far below the visible strips
	// (sluggish free translation response reported by Lisbeth Fritzell, late Jan 2010)
	//int logicalViewClientBottom = (int)pDC->DeviceToLogicalY(grectViewClient.GetBottom());
	int logicalViewClientBottom = (int)grectViewClient.GetBottom();

	// use the thumb position to adjust the Y coordinate of testRect, so it has the
	// correct logical coordinates value given the amount that the view is currently
	// scrolled 
	int nThumbPosition_InPixels = pDC->DeviceToLogicalY(0);

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

 	// THE LOOP FOR ITERATING OVER ALL FREE TRANSLATION SECTIONS IN THE DOC,
	//  STARTING FROM A PRECEDING OFFSCREEN CPile INSTANCE, BEGINS HERE

    // (BEW added additional code on 13Jul09, to prevent scanning beyond the visible extent
    // of the view window - for big documents that wasted huge slabs of time)
	while (TRUE)
	{
		while ((pPile != NULL) && (!pPile->GetSrcPhrase()->m_bStartFreeTrans))
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

            // BEW added 13Jul09, a test to determine when pPile's strip's top is greater
            // than the bottom coord (in logical coord space) of the view window - when it
            // is TRUE, we break out of the loop. Otherwise, if the document has no free
            // translations ahead, the scan goes to the end of the document - and for
            // documents with 30,000+ piles, this can take a very very long time!
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
			wxLogDebug(_T(" outer loop - scanning in visible part: srcPhrase %s , sequ num %d, strip index %d , nStripTop (logical) %d \n              grectViewClient bottom %d logicalViewClientBottom %d"),
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
				wxLogDebug(_T(" outer loop, RETURNING because pile is OFF-WINDOW;  nStripTop  %d ,  logicalViewClientBottom  %d"),
					nStripTop, logicalViewClientBottom);
#endif
#endif
				return;
			}
		}

		// return if we didn't find a free translation section
		if (pPile == NULL)
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

#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
		wxLogDebug(_T("outer loop: at next anchor, scanning ahead: srcPhrase %s , sequ num %d, active sn %d  nThumbPosition_InPixels = %d"),
			pSrcPhrase->m_srcPhrase, pSrcPhrase->m_nSequNumber, m_pApp->m_nActiveSequNum, nThumbPosition_InPixels);
#endif
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
		wxLogDebug(_T("About to create rectangles: curPileIndex  %d  , curStripIndex  %d  , curPileCount %d  "),
					curPileIndex,curStripIndex,curPileCount);
#endif
#endif

		if (gbRTL_Layout)
		{
			// source is to be laid out right-to-left, so free translation rectangles will be
			// altered in location from what would be the case for a LTR layout
			rect.SetRight(pPile->GetPileRect().GetRight()); // this fixes where the writable 
															// area starts
			while (TRUE)
			{
				// inner loop for RTL layout, scans across piles in a section
				
				//  is this pile the ending pile for the free translation section?
				if (pSrcPhrase->m_bEndFreeTrans)
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
					break; // exit the inner loop for constructing the drawing rectangles
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
							//goto b;
							break;
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
							rect = pStrip->GetFreeTransRect(); // rect.right is already correct,
															   // since this is pile[0]
							// this new pile might be the one for the end of the free translation
							// section, so iterate inner loop to check it out
							continue;
						}
					}
					else
					{
						// there is at least one more pile in this strip, so check it out
						pPile = m_pView->GetNextPile(pPile);
						wxASSERT(pPile);
						pSrcPhrase = pPile->GetSrcPhrase();
						curPileIndex = pPile->GetPileIndex();
						continue; // iterate inner loop
					}
				} // end of else block for test: if (pSrcPhrase->m_bEndFreeTrans)

			} // end of RTL inner loop: while(TRUE)

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
			rect.SetWidth(abs(pStrip->GetFreeTransRect().GetRight() - pPile->GetPileRect().GetLeft())); 
			// used abs to make sure is this pile the ending pile for the free translation
			// section?

#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
			wxLogDebug(_T(" LTR block: strip RECT: Left %d , TOP %d, WIDTH %d , Height %d  (logical coords)"),
				rect.x, rect.y, rect.width, rect.height);
#endif
#endif
			while (TRUE)
			{
				// inner loop for LTR layout, scans across the piles of the section
				
				if (pSrcPhrase->m_bEndFreeTrans)
				{
#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
					wxLogDebug(_T(" LTR block:  At end of section, so test for intersection follows:"));
#endif
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
                    // store in the pElement's subRect member (don't compute the substring
                    // yet, to save time since the rect may not be visible), add the
                    // element to the pointer array
					pElement->subRect = rect;
					pElement->horizExtent = rect.GetWidth(); 
					nTotalHorizExtent += pElement->horizExtent;
					m_pFreeTransArray->Add(pElement);

                    // determine whether or not this free translation section is going to
                    // have to be written out in whole or part
					testRect = grectViewClient;

					// BEW added 13Jul09 convert the top (ie. y) to logical coords
					testRect.SetY(nThumbPosition_InPixels);

#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
					wxLogDebug(_T(" LTR block: grectViewClient at test: L %d , T %d, W %d , H %d  (logical coords)"),
						testRect.x, testRect.y, testRect.width, testRect.height);
#endif
#endif
					if (testRect.Intersects(rect))
					{
						bSectionIntersects = TRUE; // we'll have to write out at least 
												   // this much of this section
					}
#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
					if (bSectionIntersects)
						wxLogDebug(_T(" Intersects?  TRUE  and break from rectangles-making loop (this section is done)"));
					else
						wxLogDebug(_T(" Intersects?  FALSE  and and break from rectangles-making loop (scan has gone below client rect)"));
#endif
#endif
					break; // exit the inner loop for constructing the drawing rectangles
						   // for this current free trans section
				}
				else
				{
                    // the current pile is not the ending one, so check the next - also
                    // check out when a strip changes, and restart there with a new
                    // rectangle calculation after saving the earlier element
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

                        // are there more strips? (we may have come to the end of the doc)
                        // (for a partial section at doc end, we just show as much of it as
                        // we possibly can)
						if (curStripIndex == pLayout->GetStripCount() - 1)
						{
							// there are no more strips, so this free translation section will be
							// truncated to whatever rectangles we've set up so far
							break; // exit from inner loop which defines rectangles of section
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
							// section, so iterate inner loop to check it out
							continue;
						}
					}
					else
					{
						// there is at least one more pile in this strip, so check it out
						pPile = m_pView->GetNextPile(pPile);
						wxASSERT(pPile != NULL); 
						pSrcPhrase = pPile->GetSrcPhrase();
						curPileIndex = pPile->GetPileIndex();
#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
						wxLogDebug(_T(" iterating in strips:  another pile is there, with index = %d, srcPhrase = %s , and now iterate inner loop"),
							pPile->GetPileIndex(), pPile->GetSrcPhrase()->m_srcPhrase);
#endif
#endif
						continue; // iterate the inner loop for constructing rectangles
					}
				} // end of else block for test: if (pSrcPhrase->m_bEndFreeTrans)

			} // end of LTR inner loop: while(TRUE)

		} // end LTR layout block

		// rectangle calculations are finished, and stored in 
		// FreeTrElement structs in m_pFreeTransArray		
		if (!bSectionIntersects) 
		{
			// nothing in this current section of free translation is visible in the view's
			// client rectangle so if we are not below the bottom of the latter, iterate to
			// handle the next section, but if we are below it, then we don't need to bother
			// with futher calculations & can return immediately
			pElement = (FreeTrElement*)m_pFreeTransArray->Item(0);

			// for next line... view only, MM_TEXT mode, y-axis positive downwards
			if (pElement->subRect.GetTop() > logicalViewClientBottom) 
			{
#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
				wxLogDebug(_T("No intersection, *** and below bottom of client rect, so return ***"));
#endif
#endif
				DestroyElements(m_pFreeTransArray); // don't leak memory
				return; // we are done
			}
			else
			{
#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
				wxLogDebug(_T(" NO INTERSECTION block:  top is still above grectViewClient's bottom, so prepare then iterate outer loop"));
#endif
#endif
				pPile = m_pView->GetNextPile(pPile);
				if (pPile != NULL)
					pSrcPhrase = pPile->GetSrcPhrase();
				DestroyElements(m_pFreeTransArray);

#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
				wxLogDebug(_T(" At at upper middle next pPile is --  srcphrase %s, sn = %d, iterate outer loop"),
					pSrcPhrase->m_srcPhrase, pSrcPhrase->m_nSequNumber);
#endif
#endif			
				continue; // iterate outer loop (scans over free trans sections)
			}
		} // end of TRUE block for test:  if (!bSectionIntersects)

		// the whole or part of this section must be drawn, so do the
		// calculations now; first, get the free translation text
		pSrcPhrase = m_pFirstPile->GetSrcPhrase();
		offset = 0;
		length = 0;
		ftStr = pSrcPhrase->GetFreeTrans();
		length = ftStr.Len();

#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
		wxLogDebug(_T("(Must draw...)Sequ Num  %d  ,  Free Trans:  %s "), pSrcPhrase->m_nSequNumber, ftStr.c_str());
#endif
#endif
		// whm changed 24Aug06 when called from OnEnChangedEditBox, we need to be able to allow
		// user to delete the contents of the edit box, and draw nothing, so we'll not jump out
		// early here because the new length is zero
		
		// trim off any leading or trailing spaces
		if (length > 0)
		{
			ftStr.Trim(FALSE); // trim left end
			ftStr.Trim(TRUE); // trim right end
		}
		if (length == 0 || ftStr.IsEmpty())
		{
			// nothing to write, so move on & iterate
			pPile = m_pView->GetNextPile(pPile);
			if (pPile != NULL)
				pSrcPhrase = pPile->GetSrcPhrase();
			DestroyElements(m_pFreeTransArray);

#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
			wxLogDebug(_T(" At middle, next pPile is --  srcphrase %s, sn = %d, iterate outer loop"),
				pSrcPhrase->m_srcPhrase, pSrcPhrase->m_nSequNumber);
#endif
#endif			
			continue;
		}
		
        // get text's extent (a wxSize object) and compare to the total horizontal extent
        // of the rectangles. also determine the number of rectangles we are to write this
        // section into, and initialize other needed data
		pDC->GetTextExtent(ftStr,&extent.x,&extent.y);
		bTextIsTooLong = extent.x > nTotalHorizExtent ? TRUE : FALSE;
		totalRects = m_pFreeTransArray->GetCount();

		if (totalRects < 2)
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
//			 wxSize trueSz;
//			 pDC->GetTextExtent(ftStr,&trueSz.x,&trueSz.y);
//			 wxLogDebug(_T("RTL DrawText sub.l=%d + sub.w=%d - 
//			                                     ftStrExt.x=%d, x=%d, y=%d of %s"),
//				pElement->subRect.GetLeft(),pElement->subRect.GetWidth(),trueSz.x,
//				pElement->subRect.GetLeft()+pElement->subRect.GetWidth()-trueSz.x,
//				pElement->subRect.GetTop(),ftStr.c_str());
//#endif
				m_pView->DrawTextRTL(pDC,ftStr,pElement->subRect);
			}
			else
			{

#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
				wxLogDebug(_T(" *** Drawing ftSstr at:  Left  %d   Top  %d  These are logical coords."),
					pElement->subRect.GetLeft(), pElement->subRect.GetTop());
				wxLogDebug(_T(" *** Drawing ftSstr: m_pFirstPile's Logical Rect x= %d  y= %d  width= %d  height= %d   PileHeight + 2: %d"),
					m_pFirstPile->Left(), m_pFirstPile->Top(), m_pFirstPile->Width(), m_pFirstPile->Height(), m_pFirstPile->Height() + 2);
#endif
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
//		         wxSize trueSz;
//			     pDC->GetTextExtent(s,&trueSz.x,&trueSz.y);
//			     wxLogDebug(_T("RTL DrawText sub.l=%d + sub.w=%d - sExt.x=%d, x=%d, y=%d of %s"),
//				    pElement->subRect.GetLeft(),pElement->subRect.GetWidth(),trueSz.x,
//				    pElement->subRect.GetLeft()+pElement->subRect.GetWidth()-trueSz.x,
//				    pElement->subRect.GetTop(),s.c_str());
//#endif
					m_pView->DrawTextRTL(pDC,s,pElement->subRect);
				}
				else
				{
					pDC->DrawText(s,pElement->subRect.GetLeft(),pElement->subRect.GetTop());
				}
				// Cannot call Invalidate() or SendSizeEvent from within DrawFreeTranslations
				// because it triggers a paint event which results in a Draw() which results
				// in DrawFreeTranslations() being reentered... hence a run-on condition 
				// endlessly calling the View's OnDraw
				  
			} // end of loop: for (index = 0; index < totalRects; index++)

			subStrings.Clear(); // clear the array ready for the next iteration

#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
			wxLogDebug(_T("subStrings drawn - finished them "));
#endif
#endif
		} // end of else block for test: if (totalRects < 2)

		// the section has been dealt with, get the next pile and iterate
		pPile = m_pView->GetNextPile(pPile);
		if (pPile != NULL)
			pSrcPhrase = pPile->GetSrcPhrase();
		DestroyElements(m_pFreeTransArray);

#ifdef _Trace_DrawFreeTrans
#ifdef __WXDEBUG__
		wxLogDebug(_T(" At bottom, next pPile is --  srcphrase %s, sn = %d, iterate outer loop"),
			pSrcPhrase->m_srcPhrase, pSrcPhrase->m_nSequNumber);
#endif
#endif
	} // end of outer loop: while(TRUE)
}

// when the phrase box lands at the anchor location, it may clear the m_bHasKBEntry flag,
// or the m_bHasGlossingKBEntry flag when glossing mode is on, and if there is an
// adaptation (or gloss) there when the phrase box is subsequently moved, we must make sure
// the flag has the appropriate value
// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
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
/// BEW 19Feb10, no changes needed for support of doc version 5
/// BEW 9July10, no changes needed for support of kbVersion 2
/// BEW 25Oct10, changed to use Trim() - since pSrcPhrase->m_targetStr is typically passed
/// in, the algorithm needs no change for support of additional members of CSourcePhrase
/////////////////////////////////////////////////////////////////////////////////
bool CFreeTrans::HasWordFinalPunctuation(CSourcePhrase* pSP, wxString phrase, 
											wxString& punctSet)
{
	// beware, phrase can sometimes have a final space following punctuation - so first
	// remove trailing spaces
	phrase = MakeReverse(phrase);
	phrase.Trim(FALSE); // trim from the start to remove (now initial) spaces, if any
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
///	\param pThisPile ->	pointer to the pile which is being examined as a candidate for
///	                    being the past the end of the currently-being-defined section; it
///	                    is typically one CSourcePhrase instance beyond the 'current' one
///	                    in the caller. Even so, in some circumstances the end of the section
///	                    may be the instance following pThisPile - see below. (If TRUE
///                     is returned, the passed in pile will be excluded from the current
///                     free translation section being delimited, and scanning will stop,
///                     but the halt instance is made the next one when bAtFollowingPile 
///                     is returned as TRUE, see the description for that param below.)
/// \param bAtFollowingPile <- FALSE if pThisPile is the one to halt at (ie. it is not in the
///                            section). But that won't work for \f*, \fe* or \x* storage 
///                            locations - for those, return TRUE here, and the caller will
///                            know that that pile is to be included in the section, that is,
///                            to make the halt location be after the pile storing \f* etc.
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
/// BEW modified 19Feb10, for support of doc version 5. New version does not have endmarkers
/// in m_markers any more, but in m_endMarkers (and only endmarkers there, else it is empty)
/// BEW 9July10, no changes needed for support of kbVersion 2
/// BEW 11Oct10, fixes a failure, for doc version 5, of the sectioning to halt a section
/// starting within a footnote between the CSourcePhrase storing \f* and the one follows.
/// (Before the fix, the instance with \f* was excluded from the section. The fix required
/// the introduction of the 2nd param, bAtFollowingPile.)
/////////////////////////////////////////////////////////////////////////////////
bool CFreeTrans::IsFreeTranslationEndDueToMarker(CPile* pThisPile, bool& bAtFollowingPile)
{
	bAtFollowingPile = FALSE; // default value
	USFMAnalysis* pAnalysis = NULL;
	wxString bareMkr;
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
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
	// reference section, or endnote. The halt, however, must be at the CSourcePhrase
	// which follows the one which stores the endmarker - that is, the endmarker's
	// CSourcePhrase instance is the LAST instance in that-being-defined section
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
			{
				bAtFollowingPile = TRUE;
				return TRUE; // halt scanning at next location
			}
			if (mkr == xrefMkr + _T('*'))
			{
				bAtFollowingPile = TRUE;
				return TRUE;
			}
			if (m_pApp->gCurrentSfmSet == UsfmOnly && (mkr == endnoteMkr + _T('*')))
			{
				bAtFollowingPile = TRUE;
				return TRUE;
			}
			if (m_pApp->gCurrentSfmSet == PngOnly && (mkr == _T("\\fe") || mkr == _T("\\F")))
			{
				bAtFollowingPile = TRUE;
				return TRUE;
			}
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
			bAtFollowingPile = TRUE;
			return TRUE;
		}
	}

	// now deal with the m_markers member's content - look for halt-causing markers (it's
	// sufficient to find just the first, and make our decision from that one). For these,
	// bAtFollowingPile is always returned FALSE, since the CSourcePhrase carrying any
	// halting begin-marker will always lie outside the end of the currently-being-defined
	// section
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
		pBuff = markers.GetData();
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
		wxString mkr = _T("\\") + bareMkr;
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

/////////////////////////////////////////////////////////////////////////////////
/// \return             TRUE if the sourcephrase's m_bStartFreeTrans BOOL is TRUE, 
///                     FALSE otherwise
///	\param pPile	->	pointer to the pile which stores the pSrcPhrase pointer being 
///	                    examined
///	BEW 19Feb10, no change needed for support of doc version 5
///	BEW 9July10, no changes needed for support of kbVersion 2
/////////////////////////////////////////////////////////////////////////////////
bool CFreeTrans::IsFreeTranslationSrcPhrase(CPile* pPile)
{
	return pPile->GetSrcPhrase()->m_bStartFreeTrans == TRUE;
}

// BEW 9July10, no changes needed for support of kbVersion 2
/// whm modified 21Sep10 to make safe for when selected user profile removes this menu item.
void CFreeTrans::OnAdvancedFreeTranslationMode(wxCommandEvent& event)
{
	//wxMenuBar* pMenuBar = m_pFrame->GetMenuBar();
	//wxASSERT(pMenuBar != NULL);

	// whm Note: Only log user action when user explicitly selects menu
	// item not when OnAdvancedFreeTranslationMode() is called by other
	// functions.
	if (event.GetId() == ID_ADVANCED_FREE_TRANSLATION_MODE)
	{
		if (m_pApp->m_bFreeTranslationMode)
			m_pApp->LogUserAction(_T("Free Translation Mode OFF"));
		else
			m_pApp->LogUserAction(_T("Free Translation Mode ON"));
	}

	// klb 9/2011 extracted most of the code here and moved to SwitchScreenFreeTranslationMode()
	SwitchScreenFreeTranslationMode();

}
//*****************
// klb 9/2011
//    extracted most of the code from CFreeTrans::OnAdvancedFreeTranslationMode 
//       and created this call so free translations could be drawn on print preview
//       in the background when requested (print preview relies on what is on screen)
void CFreeTrans::SwitchScreenFreeTranslationMode()
{
	wxMenuBar* pMenuBar = m_pFrame->GetMenuBar();
	wxASSERT(pMenuBar != NULL);
	wxMenuItem * pAdvancedMenuFTMode = 
						pMenuBar->FindItem(ID_ADVANCED_FREE_TRANSLATION_MODE);

	gbSuppressSetup = FALSE; // setdefault value

    // determine if the document is unstructured or not -- we'll need this set or cleared
    // as appropriate because in free translation mode the user may elect to end sections
    // at verse breaks - and we can't do that for unstructured data (in the latter case,
    // we'll just end when there is following punctuation on a word or phrase)
	SPList* pSrcPhrases = m_pApp->m_pSourcePhrases;
	gbIsUnstructuredData = m_pView->IsUnstructuredData(pSrcPhrases);

	// toggle the setting
	if (m_pApp->m_bFreeTranslationMode)
	{
		// toggle the checkmark to OFF
		if (pAdvancedMenuFTMode != NULL)
		{
			pAdvancedMenuFTMode->Check(FALSE);
		}
		m_pApp->m_bFreeTranslationMode = FALSE;

        // free translation mode is being turned off, so "fix" the current free translation
        // before the m_pCurFreeTransSectionPileArray contents are invalidated by the
        // RecalcLayout() call within ComposeBarGuts() below
		StoreFreeTranslationOnLeaving();
	}
	else
	{
		// toggle the checkmark to ON
		if (pAdvancedMenuFTMode != NULL)
		{
			pAdvancedMenuFTMode->Check(TRUE);
		}
		m_pApp->m_bFreeTranslationMode = TRUE;
	}
	if (m_pApp->m_bFreeTranslationMode)
	{
        // put the target punctuation character set into gSpacelessTgtPunctuation to be
        // used in the HasWordFinalPunctuation() function to work out when to end a span of
        // free translation (can't put this after the ComposeBarGuts() call because the
        // latter calls SetupCurrentFreeTransSection(), and it needs
        // gSpacelessTgtPunctuation set up beforehand)
		gSpacelessTgtPunctuation = m_pApp->m_punctuation[1]; // target set, with 
														   // delimiting spaces
		gSpacelessTgtPunctuation.Remove(gSpacelessTgtPunctuation.Find(_T(' ')),1); // get 
																	// rid of the spaces
	}

	// restore focus to the targetBox, if free translation mode was just turned off,
	// else to the CEdit in the Compose Bar because it has just been turned on
	// -- providing the box or bar is visible and its handle exists
	m_pFrame->ComposeBarGuts(); // open or close the Compose Bar -- it does a 
            // RecalcLayout() call, so if turning off free translation mode, the
            // m_pCurFreeTransSectionPileArray array will store hanging pointers,
            // so don't use it below

	if (m_pApp->m_bFreeTranslationMode)
	{
        // free translation mode was just turned on. The phrase box might happen to be
        // located within a previously composed section of free translation, but not at
        // that section's anchor point, so we must check for this and if so, iterate back
        // over the piles until we get to the anchor point
		CPile* pPile = m_pApp->m_pActivePile; // current box location
		CSourcePhrase* pSP = pPile->GetSrcPhrase();
		CKB* pKB;
		if (gbIsGlossing)
			pKB = m_pApp->m_pGlossingKB;
		else
			pKB = m_pApp->m_pKB;
		if (pSP->m_bHasFreeTrans && !pSP->m_bStartFreeTrans)
		{
			// save the phrase box text to the KB
			// left it here -- may need to ensure m_targetPhrase has no punct before 
			// passing to StoreTextGoingBack()
			bool bOK;
			bOK = pKB->StoreTextGoingBack(pSP,m_pApp->m_targetPhrase); // store, so we can 
																// forget this location
		}
		while (pSP->m_bHasFreeTrans && !pSP->m_bStartFreeTrans)
		{
			// iterate backwards to the anchor pile
			CPile* pPrevPile = m_pView->GetPrevPile(pPile);
			if (pPrevPile == NULL)
			{
				// got to doc start without detecting the start - should never happen
				// so just shove it at start of doc, with no lookup and a beep
				int sn = 0;
				m_pApp->m_targetPhrase.Empty();
				pPile = m_pView->GetPile(sn);
				m_pFrame->canvas->ScrollIntoView(sn);
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
		m_pApp->m_pActivePile = pPile;
		m_pApp->m_nActiveSequNum = pPile->GetSrcPhrase()->m_nSequNumber;

        // BEW changed 15Oct05; since if we have an adaptation there different than the
        // source text we want to preserve it, rather than unilaterally just use m_key; and
        // if the member is empty, then leave the box empty rather than have the source
        // copied in free translation mode
		if (gbIsGlossing)
		{
			if (pSP->m_gloss.IsEmpty())
			{
				m_pApp->m_targetPhrase.Empty();
			}
			else
			{
				m_pApp->m_targetPhrase = pSP->m_gloss;
			}
		}
		else
		{
			if (pSP->m_adaption.IsEmpty())
			{
				m_pApp->m_targetPhrase.Empty();
			}
			else
			{
				m_pApp->m_targetPhrase = pSP->m_adaption;
			}
		}
		translation = m_pApp->m_targetPhrase; // in case we just unmerged, since a 
                        //PlacePhraseBox() call with selector == 1 or 3 will set
                        //m_targetPhrase to whatever is currently in the global string
                        //translation when it jumps the block of code for removing the new
                        //location's entry from the KB
		CCell* pCell = pPile->GetCell(1); // need this for the PlacePhraseBox() call
		m_pView->PlacePhraseBox(pCell,1); // 1 = inhibit saving at old location, as we did it above
				// instead, and also don't remove the new location's KB entry (as the 
				// phrase box is disabled)

		// prevent clicks and editing being done in phrase box (do also in ResizeBox())
		if (m_pApp->m_pTargetBox->IsShown() && m_pApp->m_pTargetBox->GetHandle() != NULL)
			m_pApp->m_pTargetBox->SetEditable(FALSE);
		m_pLayout->m_pCanvas->ScrollIntoView(
								m_pApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber);

          // whm 4Apr09 moved this SetFocus below ScrollIntoView since ScrollIntoView seems
          // to remove the focus on the Compose Bar's edit box if it follows the SetFocus
          // call. now put the focus in the Compose Bar's edit box, and disable the phrase
          // box for clicks & editing, and make it able to right justify and render RTL if
          // we are in the Unicode app
		if (m_pFrame->m_pComposeBar->GetHandle() != NULL)
			if (m_pFrame->m_pComposeBar->IsShown())
			{
				#ifdef _RTL_FLAGS
					// enable complex rendering
					if (m_pApp->m_bTgtRTL)
					{
						m_pFrame->m_pComposeBarEditBox->SetLayoutDirection(
							wxLayout_RightToLeft);
					}
					else
					{
						m_pFrame->m_pComposeBarEditBox->SetLayoutDirection(
							wxLayout_LeftToRight);
					}
				#endif
				m_pFrame->m_pComposeBarEditBox->SetFocus();

			}

		// get any removed free translations in gEditRecord into the GUI list
		bool bAllsWell;
		bAllsWell = m_pView->PopulateRemovalsComboBox(freeTranslationsStep, &gEditRecord);
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
		if (m_pApp->m_pActivePile->GetSrcPhrase()->m_bRetranslation)
		{
			// we have to move the box
			CSourcePhrase* pSrcPhr = m_pApp->m_pActivePile->GetSrcPhrase();
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
			bOK = m_pView->SetActivePilePointerSafely(m_pApp,pSrcPhrases,nSaveActiveSequNum,
											m_pApp->m_nActiveSequNum,numSrcPhrases);
		}

		translation.Empty(); // don't preserve anything from a former adaptation state
		if (m_pApp->m_pTargetBox->GetHandle() != NULL)
			if (m_pApp->m_pTargetBox->IsShown())
				m_pApp->m_pTargetBox->SetFocus();

		// allow clicks and editing to be done in phrase box (do also in ResizeBox())
		if (m_pApp->m_pTargetBox->IsShown() && m_pApp->m_pTargetBox->GetHandle() != NULL)
			m_pApp->m_pTargetBox->SetEditable(TRUE);

        // get any removed adaptations in gEditRecord into the GUI list, if the restored
        // state is adapting mode; if glossing mode, get the removed glosses into the GUI
        // list
		bool bAllsWell;
		if (gbIsGlossing)
			bAllsWell = m_pView->PopulateRemovalsComboBox(glossesStep, &gEditRecord);
		else
			bAllsWell = m_pView->PopulateRemovalsComboBox(adaptationsStep, &gEditRecord);

        // BEW added 10Jun09; do a recalc of the layout, set active pile pointer, and
        // scroll into view - otherwise these are not done and box can be off-window
		m_pLayout->RecalcLayout(m_pApp->m_pSourcePhrases,keep_strips_keep_piles);
		m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);
		m_pLayout->m_pCanvas->ScrollIntoView(m_pApp->m_nActiveSequNum);
		m_pView->Invalidate();
		m_pLayout->PlaceBox();
	}
}

// BEW 9July10, no changes needed for support of kbVersion 2
void CFreeTrans::OnAdvancedRemoveFilteredFreeTranslations(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItDoc* pDoc = m_pView->GetDocument();
	m_pApp->LogUserAction(_T("Initiated OnAdvancedRemoveFilteredFreeTranslations()"));

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
		SPList* pList = m_pApp->m_pSourcePhrases;
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
		m_pApp->LogUserAction(_T("The document does not contain any free translations."));
		return;
	}

	int nResult = wxMessageBox(_(
"You are about to delete all the free translations in the document. Is this what you want to do?"),
	_T(""), wxYES_NO);
	if (nResult == wxNO)
	{
		// user clicked the command by mistake, so exit the handler
		m_pApp->LogUserAction(_T("Aborted before delete all free translations"));
		return;
	}

	// initialize variables needed for the scan over the document's 
	// sourcephrase instances
	SPList* pList = m_pApp->m_pSourcePhrases;
	SPList::Node* pos = pList->GetFirst();
	CSourcePhrase* pSrcPhrase;
	wxString emptyStr = _T("");

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
		pSrcPhrase->SetFreeTrans(emptyStr);
	} // end while loop
	m_pView->Invalidate();
	m_pLayout->PlaceBox();

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
/// BEW 22Feb10 no changes needed for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateAdvancedRemoveFilteredBacktranslations(wxUpdateUIEvent& event)
{
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pKB != NULL && (int)m_pApp->m_pSourcePhrases->GetCount() > 0)
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
/// BEW 22Feb10 no changes needed for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateAdvancedRemoveFilteredFreeTranslations(wxUpdateUIEvent& event)
{
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pKB != NULL && (int)m_pApp->m_pSourcePhrases->GetCount() > 0)
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
///	BEW 19Feb10, no changes needed for support of doc version 5 (but the function
///	IsFreeTranslationEndDueToMarker() called internally has extensive changes for
///	doc version 5)
///	BEW 9July10, no changes needed for support of kbVersion 2
///	BEW 11Oct10, extra changes for doc version 5 required no changes here, but a bug was
///	found - the section definition within a footnote span would not include the final
///	CSourcePhrase on which the \f* was stored. This required a change in 
///	IsFreeTranslationEndDueToMarker()	
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::SetupCurrentFreeTransSection(int activeSequNum)
{
	gbFreeTranslationJustRemovedInVFMdialog = FALSE; // restore to default 
           // value, in case a removed was just done in the View Filtered 
           // Material dialog

	if (activeSequNum < 0)
		// phrase box is not defined, no active location is valid, so return
		return;

#ifdef __WXDEBUG__
//	wxLogDebug(_T("\nActive SN passed in: %d"),activeSequNum);
#endif
	m_pApp->m_pActivePile = m_pView->GetPile(activeSequNum); // has to be set here, because at
							// end of RecalcLayout's legacy code it is still undefined
	bool bEditBoxHasText = FALSE; // to help with initializing the ComposeBar's contents,
                            // because we may be returning from normal mode after an
                            // editing operation and want the box text to still be there
	wxASSERT(m_pFrame->m_pComposeBar != NULL);
	wxTextCtrl* pEdit = (wxTextCtrl*)
							m_pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE); 
	// whm 24Aug06 removed gFreeTranslationStr global here and below
	wxString tempStr;
	tempStr = pEdit->GetValue(); // set tempStr to whatever is in the box

	m_pCurFreeTransSectionPileArray->Clear(); // start with an empty array

	bool bOwnsFreeTranslation = IsFreeTranslationSrcPhrase(m_pApp->m_pActivePile);
	CPile* pile;
	if (bOwnsFreeTranslation)
	{
		// it already has a free translation stored in the sourcephrase
		pile = m_pApp->m_pActivePile;
		tempStr = m_pApp->m_pActivePile->GetSrcPhrase()->GetFreeTrans();
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
//			wxLogDebug(_T("Storing sequ num %d in m_pCurFreeTransSectionPileArray, count = %d"),
//				pile->GetSrcPhrase()->m_nSequNumber, m_pCurFreeTransSectionPileArray->GetCount());
#endif
           // there might be only one pile in the section, if so, this one would also
            // have the m_bEndFreeTrans flag set TRUE, so check for this and if so,
            // break out here
			if (pile->GetSrcPhrase()->m_bEndFreeTrans)
				break;

			// get the next pile - beware, it will be NULL if we are at doc end
			pNextPile = m_pView->GetNextPile(pile);
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
			SPList* pSrcPhrases = m_pApp->m_pSourcePhrases;
			CSourcePhrase* pFirstSPhr = ((CPile*)
							m_pCurFreeTransSectionPileArray->Item(0))->GetSrcPhrase();
			CSourcePhrase* pLastSPhr = ((CPile*)
							m_pCurFreeTransSectionPileArray->Last())->GetSrcPhrase();
			bool bFreeTransPresent = TRUE;		
			bool bProbablyVerse = 
							m_pView->GetLikelyValueOfFreeTranslationSectioningFlag(pSrcPhrases,
								pFirstSPhr->m_nSequNumber, pLastSPhr->m_nSequNumber, 
								bFreeTransPresent);
			m_pApp->m_bDefineFreeTransByPunctuation = !bProbablyVerse;

			// now set the radio buttons
			wxRadioButton* pRadioButton = (wxRadioButton*)
				m_pFrame->m_pComposeBar->FindWindowById(IDC_RADIO_PUNCT_SECTION);
			// set the value
			if (m_pApp->m_bDefineFreeTransByPunctuation)
				pRadioButton->SetValue(TRUE);
			else
				pRadioButton->SetValue(FALSE);
			pRadioButton = (wxRadioButton*)
				m_pFrame->m_pComposeBar->FindWindowById(IDC_RADIO_VERSE_SECTION);
			// set the value
			if (!m_pApp->m_bDefineFreeTransByPunctuation)
				pRadioButton->SetValue(TRUE);
			else
				pRadioButton->SetValue(FALSE);
			}
	}
	else
	{
		// it does not yet have a free translation stored in this sourcephrase,
		// so work out the first guess for what the current section is to be
		pile = m_pView->GetPile(activeSequNum);
		if (pile == NULL)
			return; // something's very wrong - how can the phrase box be at 
					// a null pile?
		if (!tempStr.IsEmpty())
			bEditBoxHasText = TRUE;

        // at the current section we collect the layout information in a private array
        // member of the FreeTrans class, so that we can delay committal to the section's
        // extent until the user has made whatever manual adjustments (with Compose Bar
        // butons or clicking the phrase box elsewhere or selecting or combinations of any
        // of those) and the clicks Advance or Next> of <Prev -- since it is at that point
        // that the free translation-related flags in the affected pSrcPhrase instances
        // will get set (by a StoreFreeTranslation() call, passing in the enum value
        // remove_editbox_contents) - (& free translations not at the current location will
        // use those flags set at an earlier call to set up for writing and colouring the
        // free translation text's background in the main window)
		int wordcount = 0;
		CPile* pNextPile;
		while (pile != NULL)
		{
			CSourcePhrase* pSrcPhrase;
			// store this pile in the private array
			m_pCurFreeTransSectionPileArray->Add(pile);

#ifdef __WXDEBUG__
//			wxLogDebug(_T("Empty area:  Storing sequ num %d in m_pCurFreeTransSectionPileArray, count = %d"),
//				pile->GetSrcPhrase()->m_nSequNumber, m_pCurFreeTransSectionPileArray->GetCount());
#endif
			// count the pile's words (BEW changed 28Apr06)
			wordcount += pile->GetSrcPhrase()->m_nSrcWords;

			// test first for a following free translation section 
			// - if there is one it must halt scanning immediately
			pNextPile = m_pView->GetNextPile(pile);
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
			// BEW 19Feb10, IsFreeTranslationEndDueToMarker() modified to support
			// doc version 5 
			bool bHaltAtFollowingPile = FALSE;
			bool bIsItThisPile = IsFreeTranslationEndDueToMarker(pNextPile, bHaltAtFollowingPile);
			if (bIsItThisPile)
			{
				if (!bHaltAtFollowingPile)
				{
					break; // halt scanning, we've bumped into a SF marker which is 
						   // significant enough for us to consider that something quite
                           // different follows, or a filtered section starts at pNextPile
                           // - and that too indicates potential major change in the text
                           // at the next pile
				}
				else
				{
					// we need to go one pile further - it exists because pNextPile is it
					wordcount += pNextPile->GetSrcPhrase()->m_nSrcWords;
					bHaltAtFollowingPile = TRUE;
					m_pCurFreeTransSectionPileArray->Add(pNextPile);
					break;
				}
            }
			// determine if we can start testing for the end of the section
			// BEW 28Apr06, we are now counting words, so use to MIN_FREE_TRANS_WORDS, 
			// & still = 5 (See AdaptitConstants.h)
			if (wordcount >= MIN_FREE_TRANS_WORDS)
			{
				// test for final pile in this section
				pSrcPhrase = pile->GetSrcPhrase();
				wxASSERT(pSrcPhrase != NULL);
				if (gbIsUnstructuredData || m_pApp->m_bDefineFreeTransByPunctuation)
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
			pile = m_pView->GetNextPile(pile);
		} // end of loop block for test: while (pile != NULL)

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
			if (m_pApp->m_bTargetIsDefaultFreeTrans || m_pApp->m_bGlossIsDefaultFreeTrans)
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
///	BEW 19Feb10, updated for support of doc version 5 (changes needed)
///	BEW 9July10, no changes needed for support of kbVersion 2
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::StoreFreeTranslation(wxArrayPtrVoid* pPileArray,CPile*& pFirstPile,
	CPile*& pLastPile,enum EditBoxContents editBoxContents, const wxString& storeStr)
{
	wxPanel* pBar = m_pFrame->m_pComposeBar;
	wxASSERT(pBar);

	if (pBar != NULL && pBar->IsShown())
	{
		wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
		if (pEdit != 0 && pPileArray->GetCount() > 0) // whm added second condition 
			// 1Apr09 as wxMac gets here on frame size event and pPileArray has 0 items
		{
			pLastPile = NULL; // set null for now, it is given its value at the end
			pFirstPile = (CPile*)pPileArray->Item(0);
			CPile* pPile = pFirstPile;
			pFirstPile->GetSrcPhrase()->SetFreeTrans(storeStr);

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

                // if we are at the end of the document, our Adavance will not be fruitful,
                // so we'll just want to be able to automatically replace the phrase box
                // here - and beep to alert the user that the Advance failed.
				pFirstPile = m_pApp->m_pActivePile; // this should remain valid if we are 
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
// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
void CFreeTrans::StoreFreeTranslationOnLeaving()
{
	if (m_pFrame->m_pComposeBar != NULL)
	{
		wxTextCtrl* pEdit = (wxTextCtrl*)
							m_pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
		if (pEdit != 0)
		{
			CPile* pOldActivePile = m_pApp->m_pActivePile;

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
///	BEW 19Feb10 no change needed for support of doc version 5
///	BEW 9July10, no changes needed for support of kbVersion 2
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
///    BEW 22Feb10 no changes needed for support of doc version 5
///    BEW 9July10, no changes needed for support of kbVersion 2
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

// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
/// whm modified 21Sep10 to make safe for when selected user profile removes this menu item.
void CFreeTrans::ToggleFreeTranslationMode()
{
	if (gbVerticalEditInProgress)
	{
		wxMenuBar* pMenuBar = m_pFrame->GetMenuBar();
		wxASSERT(pMenuBar != NULL);
		wxMenuItem * pAdvancedFreeTranslation = 
							pMenuBar->FindItem(ID_ADVANCED_FREE_TRANSLATION_MODE);
		//wxASSERT(pAdvancedFreeTranslation != NULL);
		gbSuppressSetup = FALSE; // setdefault value

        // determine if the document is unstructured or not -- we'll need this set or
        // cleared as appropriate because in free translation mode the user may elect to
        // end sections at verse breaks - and we can't do that for unstructured data (in
        // the latter case, we'll just end when there is following punctuation on a word or
        // phrase)
		SPList* pSrcPhrases = m_pApp->m_pSourcePhrases;
		gbIsUnstructuredData = m_pView->IsUnstructuredData(pSrcPhrases);

		// toggle the setting
		if (m_pApp->m_bFreeTranslationMode)
		{
			// toggle the checkmark to OFF
			if (pAdvancedFreeTranslation != NULL)
			{
				pAdvancedFreeTranslation->Check(FALSE);
			}
			m_pApp->m_bFreeTranslationMode = FALSE;
		}
		else
		{
			// toggle the checkmark to ON
			if (pAdvancedFreeTranslation != NULL)
			{
				pAdvancedFreeTranslation->Check(TRUE);
			}
			m_pApp->m_bFreeTranslationMode = TRUE;
		}
		if (m_pApp->m_bFreeTranslationMode)
		{
            // put the target punctuation character set into gSpacelessTgtPunctuation to be
            // used in the HasWordFinalPunctuation() function to work out when to end a
            // span of free translation (can't put this after the ComposeBarGuts() call
            // because the latter calls SetupCurrentFreeTransSection(), and it needs
            // gSpacelessTgtPunctuation set up beforehand)
			gSpacelessTgtPunctuation = m_pApp->m_punctuation[1]; // target set, with 
															   // delimiting spaces
			gSpacelessTgtPunctuation.Replace(_T(" "),_T("")); // get rid of the spaces
		}	

        // restore focus to the targetBox, if free translation mode was just turned off,
        // else to the CEdit in the Compose Bar because it has just been turned on --
        // providing the box or bar is visible and its handle exists
		m_pFrame->ComposeBarGuts(); // open or close the Compose Bar

		if (m_pApp->m_bFreeTranslationMode)
		{
			// free translation mode was just turned on.

            // put the focus in the Compose Bar's edit box, and disable the phrase box for
            // clicks & editing, and make it able to right justify and render RTL if we are
            // in the Unicode app
			if (m_pFrame->m_pComposeBar != NULL)
				if (m_pFrame->m_pComposeBar->IsShown())
				{
					#ifdef _RTL_FLAGS
						// enable complex rendering
						if (m_pApp->m_bTgtRTL)
						{
							m_pFrame->m_pComposeBar->SetLayoutDirection(wxLayout_RightToLeft);
						}
						else
						{
							m_pFrame->m_pComposeBar->SetLayoutDirection(wxLayout_LeftToRight);
						}
					#endif
					m_pFrame->m_pComposeBar->SetFocus();
				}

			// prevent clicks and editing being done in phrase box (do also in CreateBox())
			if (m_pApp->m_pTargetBox->IsShown())
				m_pApp->m_pTargetBox->Enable(FALSE);
			m_pFrame->canvas->ScrollIntoView(
									m_pApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber);

			// get any removed free translations in gEditRecord into the GUI list
			bool bAllsWell;
			bAllsWell = m_pView->PopulateRemovalsComboBox(freeTranslationsStep, &gEditRecord);
		}
		else
		{
			// free translation mode was just turned off

			translation.Empty(); // don't preserve anything from a former adaptation state
			if (m_pApp->m_pTargetBox->IsShown())
			{
				m_pApp->m_pTargetBox->Enable(TRUE);
				m_pApp->m_pTargetBox->SetFocus();
			}

            // get any removed adaptations in gEditRecord into the GUI list,
            // if the restored state is adapting mode; if glossing mode, get
            // the removed glosses into the GUI list
			bool bAllsWell;
			if (gbIsGlossing)
				bAllsWell = m_pView->PopulateRemovalsComboBox(glossesStep, &gEditRecord);
			else
				bAllsWell = m_pView->PopulateRemovalsComboBox(adaptationsStep, &gEditRecord);
		}
	}
}

// handler for the IDC_APPLY_BUTTON, renamed Advance after first being called Apply
// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
void CFreeTrans::OnAdvanceButton(wxCommandEvent& event)
{
    // BEW added 19Oct06; if the ENTER key is pressed when not in Free Translation mode and
    // focus is in the compose bar then it would invoke the OnAdvanceButton() handler even
    // though the button is hidden, so we prevent this by detecting when it happens and
    // exiting without doing anything.
	if (m_pApp->m_bComposeBarWasAskedForFromViewMenu)
	{
        // compose bar is open, but not in Free Translation mode, so we must ignore an
        // ENTER keypress, and also return the focus to the compose bar's edit box
		wxTextCtrl* pEdit;
		wxASSERT(m_pFrame->m_pComposeBar != NULL);
		pEdit = (wxTextCtrl*)m_pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE); 
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
	if (!m_pApp->m_bDrafting)
	{
		OnNextButton(event);
		return;
	}

	// only do the following code when in Drafting mode
	gbSuppressSetup = FALSE; // restore default value, in case Shorten 
							 // or Lengthen buttons were used

	wxPanel* pBar = m_pFrame->m_pComposeBar; 
	if(pBar != NULL && pBar->IsShown())
	{
		wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
		if (pEdit != 0)
		{
			CPile* pOldActivePile = m_pApp->m_pActivePile;
			CPile* saveLastPilePtr = m_pApp->m_pActivePile; // a safe default

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
			CPile* pPile = m_pView->GetNextPile(saveLastPilePtr);

			if (pPile == NULL)
			{
				// we are probably at the end document so if so then leave this section
				// current, beep to warn the user
                if (saveLastPilePtr->GetSrcPhrase()->m_nSequNumber == m_pApp->GetMaxIndex())
				{
					::wxBell();

					if (gbVerticalEditInProgress)
					{
						// force transition to next step
						bool bCommandPosted;
						bCommandPosted = m_pView->VerticalEdit_CheckForEndRequiringTransition(
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
					pPile = m_pView->GetNextPile(pPile);
					if (gbVerticalEditInProgress && pPile != NULL)
					{
						int sn = pPile->GetSrcPhrase()->m_nSequNumber;
						bool bCommandPosted = 
							m_pView->VerticalEdit_CheckForEndRequiringTransition(sn, nextStep);
														// FALSE is bForceTransition
						if (bCommandPosted)
							return;
					}
					if (pPile == NULL)
					{
						// we are at the end of the doc
						if (pLastPile->GetSrcPhrase()->m_nSequNumber == 
							m_pApp->GetMaxIndex())
						{
							// at end of doc
							if (gbVerticalEditInProgress)
							{
								// force transition to next step
								bool bCommandPosted;
								bCommandPosted = m_pView->VerticalEdit_CheckForEndRequiringTransition(
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
					m_pView->VerticalEdit_CheckForEndRequiringTransition(sn, nextStep);
												// FALSE is bForceTransition
				if (bCommandPosted)
					return;
			}
			m_pApp->m_pActivePile = pPile;
			m_pApp->m_nActiveSequNum = pPile->GetSrcPhrase()->m_nSequNumber;
			gbSuppressSetup = FALSE; // make sure it is off

			// make m_bIsCurrentFreeTransSection FALSE on every pile
			m_pLayout->MakeAllPilesNonCurrent();

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
			m_pView->PlacePhraseBox(pCell,selector);

			// make sure we can see the phrase box
			m_pFrame->canvas->ScrollIntoView(m_pApp->m_nActiveSequNum);
			m_pView->Invalidate();
			m_pLayout->PlaceBox();
			pEdit->SetFocus(); // put focus back into compose bar's text control
		}
	}
}

// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
void CFreeTrans::OnNextButton(wxCommandEvent& WXUNUSED(event))
{
	gbSuppressSetup = FALSE; // restore default value, in case Shorten or 
							 // Lengthen buttons were used
	// for debugging
	//int ftStartSN = gEditRecord.nFreeTranslationStep_StartingSequNum;
	//int ftEndSN = gEditRecord.nFreeTranslationStep_EndingSequNum;

	wxPanel* pBar = m_pFrame->m_pComposeBar;
	if(pBar != NULL && pBar->IsShown())
	{
		wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
		if (pEdit != 0)
		{
			CPile* pOldActivePile = m_pApp->m_pActivePile;
			CPile* saveLastPilePtr = m_pApp->m_pActivePile; // a safe default initialization

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
			CPile* pPile = m_pView->GetNextPile(saveLastPilePtr);

            // check out pPile == NULL, we would then be at the doc end - fix things
            // according; if not null, then the next pile is within the document and we can
            // set it up as the active location & new anchor point - or it will be the
            // start of a predefined free translation section in which case it is already
            // the anchor point for the next free translation section
			if (pPile == NULL)
			{
                // The scroll position is at the end of the document
				if (saveLastPilePtr->GetSrcPhrase()->m_nSequNumber == m_pApp->GetMaxIndex())
				{
                    // we are at the end of the doc. So leave this section current, and
                    // beep to tell the user
                    ::wxBell();

					// BEW added 11Sep08 for support of vertical editing
					if (gbVerticalEditInProgress)
					{
						// force transition to next step
						bool bCommandPosted;
						bCommandPosted = m_pView->VerticalEdit_CheckForEndRequiringTransition(
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
					m_pView->VerticalEdit_CheckForEndRequiringTransition(sn, nextStep);
												// FALSE is bForceTransition
				if (bCommandPosted)
					return; // we've reached gray text, so step transition is wanted
			}

			m_pApp->m_pActivePile = pPile;
			m_pApp->m_nActiveSequNum = pPile->GetSrcPhrase()->m_nSequNumber;

			// make m_bIsCurrentFreeTransSection FALSE on every pile
			m_pLayout->MakeAllPilesNonCurrent();

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
			m_pView->PlacePhraseBox(pCell,selector); // forces RecalcLayout(), which gets
											// SetupCurrentFreeTransSection() called

			// make sure we can see the phrase box
			m_pFrame->canvas->ScrollIntoView(m_pApp->m_nActiveSequNum);
			m_pView->Invalidate(); // gets the view redrawn & phrase box shown
			m_pLayout->PlaceBox();
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
// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
void CFreeTrans::OnPrevButton(wxCommandEvent& WXUNUSED(event))
{
	gbSuppressSetup = FALSE; // restore default value, in case Shorten 
							 // or Lengthen buttons were used
	wxPanel* pBar = m_pFrame->m_pComposeBar;

	if(pBar != NULL && pBar->IsShown())
	{
		wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
		if (pEdit != 0)
		{
			CPile* pOldActivePile = m_pApp->m_pActivePile;

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
			pPrevPile = m_pView->GetPrevPile(pPile);
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
					pPile = m_pView->GetPrevPile(pPrevPile);
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
								m_pView->VerticalEdit_CheckForEndRequiringTransition(
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
						m_pView->PlacePhraseBox(pCell,selector);
						// make sure we can see the phrasebox at the beginning of the doc
						m_pFrame->canvas->ScrollIntoView(pPrevPile->GetSrcPhrase()->m_nSequNumber);
						m_pView->Invalidate();
						m_pLayout->PlaceBox();
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
					pPile = m_pView->GetPrevPile(pPrevPile);
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
								m_pView->VerticalEdit_CheckForEndRequiringTransition(
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
						m_pView->PlacePhraseBox(pCell,selector);
						// make sure we can see the phrasebox at the beginning of the doc
						m_pFrame->canvas->ScrollIntoView(
											pPrevPile->GetSrcPhrase()->m_nSequNumber);
						m_pView->Invalidate();
						m_pLayout->PlaceBox();
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
					// BEW added extra bool param, to fix bug, 11Oct10
					bool bHaltAtFollowingPile = FALSE;
					bool bIsItThePrevPile = IsFreeTranslationEndDueToMarker(pPrevPile,
															bHaltAtFollowingPile);
					if (bIsItThePrevPile)
					{
						if (!bHaltAtFollowingPile)
						{
							break;
						}
						else
						{
							// we only enter here if we've located a CSourcePhrase storing
							// either \f* or \fe* or \x* - in which case the halt location
							// is after it - which is the current active location. We
							// would want that previous location to be within a previous
							// section, so don't halt, set scanning continue.
							;
						}
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
					if (gbIsUnstructuredData || m_pApp->m_bDefineFreeTransByPunctuation)
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
			} // end of else block for test: if (pPrevPile->GetSrcPhrase()->m_bEndFreeTrans)

			wxASSERT(pPrevPile != NULL);
			if (gbVerticalEditInProgress)
			{
				// possibly force transition to next step
				bool bCommandPosted;
				bCommandPosted = 
					m_pView->VerticalEdit_CheckForEndRequiringTransition(
												-1, nextStep, TRUE);
				// TRUE is bForceTransition
				if (bCommandPosted)
				{
					// make it 'stick' before returning
					StoreFreeTranslationOnLeaving();
					return;
				}
			}

			m_pApp->m_pActivePile = pPrevPile;
			m_pApp->m_nActiveSequNum = pPrevPile->GetSrcPhrase()->m_nSequNumber;

			// make m_bIsCurrentFreeTransSection FALSE on every pile
			m_pLayout->MakeAllPilesNonCurrent();

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
			m_pView->PlacePhraseBox(pCell,selector); // forces a a RecalcLayout(), which gets 
											// SetupCurrentFreeTransSection() called
			// make sure we can see the phrase box
			m_pFrame->canvas->ScrollIntoView(m_pApp->m_nActiveSequNum);

			m_pView->Invalidate(); // gets the view redrawn
			m_pLayout->PlaceBox();
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

// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
void CFreeTrans::OnRemoveFreeTranslationButton(wxCommandEvent& WXUNUSED(event))
{
	wxPanel* pBar = m_pFrame->m_pComposeBar;

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
			CSourcePhrase* pSrcPhrase = m_pApp->m_pActivePile->GetSrcPhrase();

			// make sure the kb entry flag is set correctly
			FixKBEntryFlag(pSrcPhrase);

			wxString emptyStr = _T("");
			pSrcPhrase->SetFreeTrans(emptyStr);
			wxString tempStr;

			// update the navigation text
			pSrcPhrase->m_inform = m_pApp->GetDocument()->RedoNavigationText(pSrcPhrase);

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
			m_pView->Invalidate(); // cause redraw, and so a call to SetupCurrentFreeTransSection()
			m_pLayout->PlaceBox();
			pEdit->SetFocus(); // put focus in compose bar's edit control
			pEdit->SetSelection(-1,-1); // -1,-1 selects all in wx
		}
	}
}

// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
void CFreeTrans::OnLengthenButton(wxCommandEvent& WXUNUSED(event))
{
	gbSuppressSetup = TRUE; // prevent SetupCurrentFreeTransSection() from wiping
            // out the action done below at the time that the view is updated (which
            // otherwise would call that function)
	bool bEditBoxHasText = FALSE; // default
	// whm 24Aug06 reordered and modified below
	wxPanel* pBar = m_pFrame->m_pComposeBar; 
	wxASSERT(pBar != NULL);

	wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
	wxASSERT(pEdit != NULL);
	wxString tempStr;
	tempStr = pEdit->GetValue();

	if (!tempStr.IsEmpty()) 
		bEditBoxHasText = TRUE;
	// & we can rely on m_nActiveSequNum having being set correctly, 
	// and also m_pApp->m_pActivePile;

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
			pPile = m_pView->GetNextPile(pPile);
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
				if (m_pApp->m_bTargetIsDefaultFreeTrans || m_pApp->m_bGlossIsDefaultFreeTrans)
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
			m_pView->Invalidate();
			m_pLayout->PlaceBox();
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
/// BEW 22Feb10 no changes needed for support of doc version 5
/// BEW 9July10, no changes needed for support of kbVersion 2
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateShortenButton(wxUpdateUIEvent& event)
{
	if (!m_pApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_nActiveSequNum < 0 || m_pApp->m_pActivePile == NULL)
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

// BEW 22Feb10 no changes needed for support of doc version 5
void CFreeTrans::OnShortenButton(wxCommandEvent& WXUNUSED(event))
{
	gbSuppressSetup = TRUE; // prevent SetupCurrentFreeTransSection() from 
            // wiping out the action done below at the time that the view is
            // updated (which otherwise would call that function)
	bool bEditBoxHasText = FALSE; // default

	wxPanel* pBar = m_pFrame->m_pComposeBar;
	wxASSERT(pBar != NULL);
	wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
	wxASSERT(pEdit != NULL);
	wxString tempStr;
	tempStr = pEdit->GetValue();

	if (!tempStr.IsEmpty())
		bEditBoxHasText = TRUE;
    // & we can rely on m_nActiveSequNum having being set correctly, and also
    // m_pApp->m_pActivePile; and the button will only be enabled if this is a section not
    // previously defined ( we want to make it hard for the user to open up an un-free
    // translated section gap in the sequence of free translations)

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
				CPile* pPrevPile = m_pView->GetPrevPile(pPile);
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
				if (m_pApp->m_bTargetIsDefaultFreeTrans || m_pApp->m_bGlossIsDefaultFreeTrans)
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
			m_pView->Invalidate();
			m_pLayout->PlaceBox();
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
/// BEW 22Feb10 no changes needed for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateLengthenButton(wxUpdateUIEvent& event)
{
	//bool bOwnsFreeTranslation;
	if (!m_pApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_nActiveSequNum < 0 || m_pApp->m_pActivePile == NULL)
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
	pPile = m_pView->GetNextPile(pPile); // get the pile immediately after the current end
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
        bool bHaltAtFollowingPile = FALSE;
		bool bIsItThisPile = IsFreeTranslationEndDueToMarker(pPile, bHaltAtFollowingPile);
		if (bIsItThisPile)
		{
			if (!bHaltAtFollowingPile)
			{
                // markers or filtered stuff must end the section (for example, we can't
                // allow the possibility of unfiltering producing new content within a free
                // translation section)
				//wxLogDebug(_T("OnUpdateLengthenButton: exit at test for marker following"));
				event.Enable(FALSE);
				return;
			}
			else
			{
				// this is tricky -  we get here only if pPile has a CSourcePhrase which
				// stores one of \f* or \fe* or \x*, and after that is the location where
				// a section must end. Since we want this location to be included in the
				// footnote or endnote or crossReference, since pPile is after the current
				// end of the section, then this CSourcePhrase must not be in the section
				// and should be. If my anlysis is correct, we should enable the button.
				event.Enable(TRUE);
				return;
			}
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
/// BEW 22Feb10 no changes needed for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateAdvancedFreeTranslationMode(wxUpdateUIEvent& event)
{
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pSourcePhrases->GetCount() == 0)
	{
		event.Enable(FALSE);
		return;
	}
    // the !m_pApp->m_bComposeBarWasAskedForFromViewMenu test makes sure we don't try to
    // invoke free translation mode while the user already has the Compose Bar open for
    // another purpose
    if (m_pApp->m_nActiveSequNum <= (int)m_pApp->GetMaxIndex() && m_pApp->m_nActiveSequNum >= 0
		&& !m_pApp->m_bComposeBarWasAskedForFromViewMenu)
	{
		event.Enable(TRUE);
	}
	else
	{
		event.Enable(FALSE);
	}
}

// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
/// whm modified 21Sep10 to make safe for when selected user profile removes this menu item.
void CFreeTrans::OnAdvancedTargetTextIsDefault(wxCommandEvent& WXUNUSED(event))
{
	wxMenuBar* pMenuBar = m_pFrame->GetMenuBar();
	wxASSERT(pMenuBar != NULL);
	wxMenuItem* pAdvancedMenuTTextDft = 
							pMenuBar->FindItem(ID_ADVANCED_TARGET_TEXT_IS_DEFAULT);
	wxMenuItem* pAdvancedMenuGTextDft = 
							pMenuBar->FindItem(ID_ADVANCED_GLOSS_TEXT_IS_DEFAULT);
	//wxASSERT(pAdvancedMenuTTextDft != NULL);
	//wxASSERT(pAdvancedMenuGTextDft != NULL);
	m_pApp->LogUserAction(_T("Initiated OnAdvancedTargetTextIsDefault()"));

	// toggle the setting
	if (m_pApp->m_bTargetIsDefaultFreeTrans)
	{
		// toggle the checkmark to OFF
		if (pAdvancedMenuTTextDft != NULL)
		{
			m_pApp->LogUserAction(_T("Target Text Is Default OFF"));
			pAdvancedMenuTTextDft->Check(FALSE);
		}
		m_pApp->m_bTargetIsDefaultFreeTrans = FALSE;
	}
	else
	{
		// toggle the checkmark to ON
		if (pAdvancedMenuTTextDft != NULL)
		{
			m_pApp->LogUserAction(_T("Target Text Is Default ON"));
			pAdvancedMenuTTextDft->Check(TRUE);
		}
		m_pApp->m_bTargetIsDefaultFreeTrans = TRUE;

		// and ensure the glossing text command is off, and its flag cleared
		if (pAdvancedMenuGTextDft != NULL)
		{
			m_pApp->LogUserAction(_T("Gloss Text Is Default OFF"));
			pAdvancedMenuGTextDft->Check(FALSE);
		}
		m_pApp->m_bGlossIsDefaultFreeTrans = FALSE;
	}

	// restore focus to the Compose Bar
	if (m_pFrame->m_pComposeBar->GetHandle() != NULL)
		if (m_pFrame->m_pComposeBar->IsShown())
			m_pFrame->m_pComposeBarEditBox->SetFocus();
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
/// BEW 22Feb10 no changes needed for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateAdvancedTargetTextIsDefault(wxUpdateUIEvent& event)
{
	if (m_pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pSourcePhrases->GetCount() == 0)
	{
		event.Enable(FALSE);
		return;
	}
	if (!m_pApp->m_bFreeTranslationMode) // whm added 23Jan07 to wx version
	{
		// The Advanced menu item "Use Target Text As Default Free Translation"
		// should be disabled when the app is not in Free Translation Mode.
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_nActiveSequNum <= (int)m_pApp->GetMaxIndex() && 
			m_pApp->m_nActiveSequNum >= 0 &&
			!m_pApp->m_bComposeBarWasAskedForFromViewMenu)
		event.Enable(TRUE);
	else
		event.Enable(FALSE);
}

// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
/// whm modified 21Sep10 to make safe for when selected user profile removes this menu item.
void CFreeTrans::OnAdvancedGlossTextIsDefault(wxCommandEvent& WXUNUSED(event))
{
	wxMenuBar* pMenuBar = m_pFrame->GetMenuBar();
	wxASSERT(pMenuBar != NULL);
	wxMenuItem* pAdvancedMenuTTextDft = 
							pMenuBar->FindItem(ID_ADVANCED_TARGET_TEXT_IS_DEFAULT);
	wxMenuItem* pAdvancedMenuGTextDft = 
							pMenuBar->FindItem(ID_ADVANCED_GLOSS_TEXT_IS_DEFAULT);
	//wxASSERT(pAdvancedMenuTTextDft != NULL);
	//wxASSERT(pAdvancedMenuGTextDft != NULL);
	m_pApp->LogUserAction(_T("Initiated OnAdvancedGlossTextIsDefault()"));

	// toggle the setting
	if (m_pApp->m_bGlossIsDefaultFreeTrans)
	{
		// toggle the checkmark to OFF
		if (pAdvancedMenuGTextDft != NULL)
		{
			m_pApp->LogUserAction(_T("Gloss Text Is Default OFF"));
			pAdvancedMenuGTextDft->Check(FALSE);
		}
		m_pApp->m_bGlossIsDefaultFreeTrans = FALSE;
	}
	else
	{
		// toggle the checkmark to ON
		if (pAdvancedMenuGTextDft != NULL)
		{
			m_pApp->LogUserAction(_T("Gloss Text Is Default ON"));
			pAdvancedMenuGTextDft->Check(TRUE);
		}
		m_pApp->m_bGlossIsDefaultFreeTrans = TRUE;

		// ensure the target text command is toggled off (if it was on), and its flag
		if (pAdvancedMenuTTextDft != NULL)
		{
			m_pApp->LogUserAction(_T("Target Text Is Default OFF"));
			pAdvancedMenuTTextDft->Check(FALSE);
		}
		m_pApp->m_bTargetIsDefaultFreeTrans = FALSE;

	}

	// restore focus to the Compose Bar
	if (m_pFrame->m_pComposeBar->GetHandle() != NULL)
		if (m_pFrame->m_pComposeBar->IsShown())
			m_pFrame->m_pComposeBarEditBox->SetFocus();
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
/// BEW 22Feb10 no changes needed for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateAdvancedGlossTextIsDefault(wxUpdateUIEvent& event)
{
	if (m_pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pSourcePhrases->GetCount() == 0)
	{
		event.Enable(FALSE);
		return;
	}
	if (!m_pApp->m_bFreeTranslationMode) // whm added 23Jan07 to wx version
	{
		// The Advanced menu item "Use Gloss Text As Default Free Translation"
		// should be disabled when the app is not in Free Translation Mode.
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_nActiveSequNum <= (int)m_pApp->GetMaxIndex() && 
			m_pApp->m_nActiveSequNum >= 0 &&
			!m_pApp->m_bComposeBarWasAskedForFromViewMenu)
		event.Enable(TRUE);
	else
		event.Enable(FALSE);
}

// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
void CFreeTrans::OnRadioDefineByPunctuation(wxCommandEvent& WXUNUSED(event))
{
	wxPanel* pBar = m_pFrame->m_pComposeBar;
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
			m_pApp->m_bDefineFreeTransByPunctuation = TRUE;
						
			// BEW added 1Oct08: to have the butten click remove the
			// current section and reconstitute it as a Verse-based section
			gbSuppressSetup = FALSE;
			wxCommandEvent evt;
			m_pApp->GetFreeTrans()->OnRemoveFreeTranslationButton(evt); // remove current section and 
                // any Compose bar edit box test;
                // the OnRemoveFreeTranslationButton() call calls Invalidate()

            // To get SetupCurrenetFreeTranslationSection() called, we must call
            // RecalcLayout() with gbSuppressSetup == FALSE, then the section will be
            // resized smaller
#ifdef _NEW_LAYOUT
			m_pLayout->RecalcLayout(m_pApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
			m_pLayout->RecalcLayout(m_pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
			m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);

			// restore focus to the edit box
			pEdit->SetFocus();
		}
	}
}

// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
void CFreeTrans::OnRadioDefineByVerse(wxCommandEvent& WXUNUSED(event))
{
	wxPanel* pBar = m_pFrame->m_pComposeBar;
	wxASSERT(pBar != NULL);
	if(pBar != NULL && pBar->IsShown())
	{
		wxRadioButton* pRVSButton = (wxRadioButton*)
								pBar->FindWindow(IDC_RADIO_VERSE_SECTION);
		wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindow(IDC_EDIT_COMPOSE);
		if (pRVSButton != 0)
		{
			// set the radio button's BOOL to be TRUE
			m_pApp->m_bDefineFreeTransByPunctuation = FALSE;
			
			// BEW added 1Oct08: to have the butten click remove the
			// current section and reconstitute it as a Verse-based section
			gbSuppressSetup = FALSE;
			wxCommandEvent evt;
			OnRemoveFreeTranslationButton(evt); // remove current section and 
                // any Compose bar edit box test;
                // the OnRemoveFreeTranslationButton() call calls Invalidate()

            // To get SetupCurrentFreeTranslationSection() called, we must call
            // RecalcLayout() with gbSuppressSetup == FALSE, then the section will be
            // resized larger
#ifdef _NEW_LAYOUT
			m_pLayout->RecalcLayout(m_pApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
			m_pLayout->RecalcLayout(m_pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
			m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);

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
/// BEW 22Feb10 no changes needed for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateNextButton(wxUpdateUIEvent& event)
{
	if (!m_pApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_nActiveSequNum < 0 || m_pApp->m_pActivePile == NULL)
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
/// BEW 22Feb10 no changes needed for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdatePrevButton(wxUpdateUIEvent& event)
{
	if (!m_pApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_nActiveSequNum < 0 || m_pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	CPile* pPile = m_pView->GetPrevPile(m_pApp->m_pActivePile);
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
/// BEW 22Feb10 no changes needed for support of doc version 5
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateRemoveFreeTranslationButton(wxUpdateUIEvent& event)
{
	bool bOwnsFreeTranslation;
	if (!m_pApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_nActiveSequNum < 0 || m_pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	bOwnsFreeTranslation = IsFreeTranslationSrcPhrase(m_pApp->m_pActivePile);
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
/// BEW 22Feb10 no changes needed for support of doc version 5
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
///    BEW 22Feb10 no changes needed for support of doc version 5
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
/// BEW 22Feb10 no changes needed for support of doc version 5
/// BEW 9July10, no changes needed for support of kbVersion 2
/// BEW 17Jan11, fails if there is only one strip (or less than numVisibleStrips) because
/// the code assumes a document longer than a single screen's worth, so changes needed and
/// made
/////////////////////////////////////////////////////////////////////////////////
CPile* CFreeTrans::GetStartingPileForScan(int activeSequNum)
{
	CPile* pStartPile = NULL;
	if (activeSequNum < 0)
	{
		pStartPile = m_pView->GetPile(0);
		return pStartPile;
	}
	pStartPile = m_pView->GetPile(activeSequNum);
	wxASSERT(pStartPile);
	int stripCount = m_pLayout->GetStripCount();
	int numVisibleStrips = m_pLayout->GetNumVisibleStrips();
	if (stripCount < numVisibleStrips)
	{
		// the whole doc fits on the screen
		int nCurStripIndex = pStartPile->GetStripIndex();
		if (nCurStripIndex >= 1)
		{
			nCurStripIndex = 0; // start at doc start
		}
		// now get the strip pointer and find it's first pile to return to the caller
		CStrip* pStrip = (CStrip*)m_pLayout->GetStripArray()->Item(nCurStripIndex);
		pStartPile = (CPile*)pStrip->GetPilesArray()->Item(0); // ptr of 1st pile in strip
	}
	else
	{
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
		int stripCount = m_pLayout->GetStripArray()->GetCount();
		if (nCurStripIndex > stripCount - (numVisibleStrips + 1))
			nCurStripIndex = stripCount - (numVisibleStrips + 1);
		// now get the strip pointer and find it's first pile to return to the caller
		CStrip* pStrip = (CStrip*)m_pLayout->GetStripArray()->Item(nCurStripIndex);
		pStartPile = (CPile*)pStrip->GetPilesArray()->Item(0); // ptr of 1st pile in strip
	}
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
/// BEW 19Feb10, no changes needed for support of doc version 5
/// BEW 9July10, no changes needed for support of kbVersion 2
/// BEW 1Oct11, no need to use goto commands, so I change it to use a while loop
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

	while (TRUE)
	{
		if (bTryAgain || textHExtent > totalHExtent)
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
			break; // we are done
		}
		else
		{
			// we should be able to make the text fit (though this can't be guaranteed because
			// some space is wasted in each rectangle if we print whole words (which we do)) -
			// and we'll need to do scaling to ensure the best segmentation results
			for (nIteration = 0; nIteration <= nIterBound; nIteration++)
			{
				pElement = (FreeTrElement*)pElementsArray->Item(nIteration);

				// do the calculation, using fScale (hence, last parameter is TRUE); if
				// the apportioning doesn't fit all the text without truncation being
				// required, the bTryAgain will be returned as TRUE, else it will be FALSE
				subStr = SegmentToFit(pDC,remainderStr,ellipsis,pElement->horizExtent,
									fScale,offset,nIteration,nIterBound,bTryAgain,TRUE);
				pSubstrings->Add(subStr);
				remainderStr = remainderStr.Mid(offset); // shorten, for next segmentation
			}

			if (bTryAgain)
			{
				pSubstrings->Clear();
				remainderStr = str;
				continue;
			}
			else
			{
				break; // we are done
			}
		}
	} // end of while loop: while (TRUE)
}

/////////////////////////////////////////////////////////////////////////////////
///
///	   Backtranslation Support
///
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the Advanced Menu 
///                        is about to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed.
/// The "Collect Back Translations..." item on the Edit menu is enabled if the applicable
/// KB is not NULL and there are source phrases in the App's m_pSourcePhrases list,
/// otherwise the menu item is disabled.
/// BEW 2Mar10, updated for doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::OnUpdateAdvancedCollectBacktranslations(wxUpdateUIEvent& event)
{
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pKB != NULL && (int)m_pApp->m_pSourcePhrases->GetCount() > 0)
		event.Enable(TRUE);
	else
		event.Enable(FALSE);
}

// BEW 2Mar10, updated for doc version 5 (no changes needed)
// BEW 9July10, no changes needed for support of kbVersion 2
void CFreeTrans::OnAdvancedCollectBacktranslations(wxCommandEvent& WXUNUSED(event))
{
	m_pApp->LogUserAction(_T("Initiated OnAdvancedCollectBacktranslations()"));
	CCollectBacktranslations dlg(m_pFrame);
	dlg.Centre();
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	if (dlg.ShowModal() == wxID_OK)
	{
		// user clicked the OK button
		DoCollectBacktranslations(dlg.m_bUseAdaptations);

		// mark the doc as dirty, so that Save command becomes enabled
		pDoc->Modify(TRUE);
	}
	else
	{
		// user clicked the Cancel button, so do nothing
		m_pApp->LogUserAction(_T("Cancelled OnAdvancedCollectBacktranslations()"));
	}
	m_pView->Invalidate(); // get the view updated (so new icons (green wedges) get drawn)
	m_pLayout->PlaceBox();
	// for an unknown reason, despite calaling Invalidate() above, view's OnDraw() does
	// not get called, so force it with a main frame OnSize() call
	wxSizeEvent dummy;
	m_pApp->GetMainFrame()->OnSize(dummy);
}


// BEW 2Mar10; updated for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
void CFreeTrans::OnAdvancedRemoveFilteredBacktranslations(wxCommandEvent& WXUNUSED(event))
{
	m_pApp->LogUserAction(_T("Initiated OnAdvancedRemoveFilteredBacktranslations()"));
    // whm added 23Jan07 check below to determine if the doc has any back translations. If
    // not an information message is displayed saying there are no back translations; then
    // returns. Note: This check could be made in the OnIdle handler which could then
    // disable the menu item rather than issuing the info message. However, if the user
    // clicked the menu item, it may be because he/she though there might be one or more
    // back translations in the document. The message below confirms to the user the actual
    // state of affairs concerning any back translations in the current document.
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	bool bBTfound = FALSE;
	if (pDoc)
	{
		SPList* pList = m_pApp->m_pSourcePhrases;
		if (pList->GetCount() > 0)
		{
			SPList::Node* pos = pList->GetFirst();
			while (pos != NULL)
			{
				CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
				pos = pos->GetNext();
				if (!pSrcPhrase->GetCollectedBackTrans().IsEmpty())
				{
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
		m_pApp->LogUserAction(_T("The document does not contain any back translations."));
		return;
	}

	// IDS_DELETE_ALL_BT_ASK
	if( wxMessageBox(_(
"You are about to delete all the back translations in the document. Is this what you want to do?"),
	_T(""), wxYES_NO|wxICON_INFORMATION) == wxNO)
	{
		// user clicked the command by mistake, so exit the handler
		m_pApp->LogUserAction(_T("Aborted delete all back translations"));
		return;
	}

	// initialize variables needed for the scan over the document's 
	// sourcephrase instances
	SPList* pList = m_pApp->m_pSourcePhrases;
	SPList::Node* pos = pList->GetFirst(); 
	CSourcePhrase* pSrcPhrase;
	wxString emptyStr = _T("");

	// do the loop, halting to store each collection at appropriate (unfiltered) 
	// SF markers
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		if (pSrcPhrase->GetCollectedBackTrans().IsEmpty())
		{
			continue;
		}
		else
		{
			pSrcPhrase->SetCollectedBackTrans(emptyStr);
		}
	} // end while loop
	m_pView->Invalidate();
	m_pLayout->PlaceBox();

	// mark the doc as dirty, so that Save command becomes enabled
	pDoc->Modify(TRUE);
	// for an unknown reason, despite calaling Invalidate() above, view's OnDraw() does
	// not get called, so force it with a main frame OnSize() call
	wxSizeEvent dummy;
	m_pApp->GetMainFrame()->OnSize(dummy);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return                 nothing
///
///	\param bUseAdaptationsLine	->	TRUE if m_targetStr is used for the collection, 
///	                                FALSE if m_gloss is used
///
/// \remarks
///    This function changes nothing in the GUI, so a message is put up to ask the user to
///    wait for the function to finish. It then scans all the pSrcPhrase instances in the
///    document's m_pSourcePhrases list, collecting the adaptation or gloss string from
///    each, until it comes to a halt location. Halt locations are determined by calling
///    HaltCurrentCollection() which looks at the passed in m_markers string to see if the
///    string ends in a marker which should halt collection, or begin with an endmarker
///    which should halt collection (such as \f* or \x*).
///
///    On 02Jan06 BEW changed the handling of footnotes and cross references to ignore
///    their CSourcePhrase instances in the collection process (by checking for footnote or
///    crossReference TextType); because these embedded information types should not be
///    part of any backtranslation collection.
///
///    At each halt location, the collected backtranslation string is wrapped with a \bt
///    marker and filter markers if there is no collected backtranslation already there,
///    and inserted; but if there is a prexisting filtered backtranslation there, then its
///    content is removed and just the newly collected backtranslation is inserted in its
///    place.
///    On 21Nov05 this algorithm was enhanced to optionally work with a selection. The
///    collection operation is confined to the selection range - and works as follows.
///	1.	If the first sourcephrase instance already has a filtered \bt (or derivative) marker,
///     the collection erases the content of this already-collected section, and the
///     collection replaces that section with the new content - which goes up to either the
///     end of the selection, or until a sourcephrase with a new \bt marker (or derivative)
///     is encountered. (Note, if the selection is not long enough, the collection may then
///     not include all that should be there - but that is the user's responsibility to
///     fix. He can check in the View Filtered Material dialog, and manually add the extra
///     words if he wishes.) (Also note, if the selection is long enough, more than one
///     back translation section may be defined before either the selection end is
///     encountered, or another \bt marker is reached.)
///	2.	If the first sourcephrase does not have a filtered \bt (or derivative) marker, then
///     a new back translation section is commenced at that location - and ends according
///     to the same criteria as in 1. above.
///     (Note, if the first source phrase follows shortly after one on which a back
///     translation is stored but occurs before the former one's extent ends, it is quite
///     possible for the one currently being collected to have overlapping content with the
///     former one. Again, this is the user's responsibility to check for and rectify if he
///     wishes, and the View Filtered material dialog is again the way to do it.)
/// BEW 26Mar10, changes needed for support of doc version 5
/// BEW 9July10, no changes needed for support of kbVersion 2
/////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::DoCollectBacktranslations(bool bUseAdaptationsLine)
{
    // whm note: The process of collecting back translations is done so quickly that there
    // is probably no need for a wait or progress dialog. The user gets feedback by the
    // appearance of wedges appearing on the screen.
	bool bSelectionExists = FALSE;
	SPList aList;	// list of selected CSourcePhrase objects, if any; else an alias for
					// the document's m_pSourcePhrases list
	SPList* pList = &aList;

	SPList* pDocPhrases = m_pApp->m_pSourcePhrases; // the document's list

	// determine if there is a selection, and get a list of the sourcephrase instances
	// in it if it exists
	if (m_pApp->m_selection.GetCount() > 0)
	{
        // collection only within a selection is wanted (and it doesn't matter if the
        // selection has text of different types, because the collection mechanism handles
        // that automatically)
		wxString unwantedSrcText; // need this because the following call returns
								  // strings we are not interested in
		wxString unwantedOtherText; // need this because the following call returns
								    // strings we are not interested in
		m_pApp->GetRetranslation()->GetSelectedSourcePhraseInstances(pList, unwantedSrcText, unwantedOtherText);
		// pList is now populated with pointers to the selected sourcephrase instances

		bSelectionExists = TRUE;
	}
	else
	{
		// collection over the whole document is wanted, 
		// so set pList to the whole list
		pList = pDocPhrases;
	}

	// initialize variables needed for the scan over the document's 
	// sourcephrase instances
	wxString strCollect;
	wxString abit; // BEW added 26Nov05 because from gloss lines we can 
				   // collect copied ellipses, so exclude these
	SPList::Node* iteratorPos = pList->GetFirst(); 
	CSourcePhrase* pSrcPhrase;
	SPList::Node* savePos = iteratorPos; 
	bool bHalt;
	CSourcePhrase* pLastSrcPhrase = (CSourcePhrase*)iteratorPos->GetData();

	bool bHalted_at_bt_mkr = FALSE;
    // BEW addition 02Jan06: collecting ignores footnotes, endnotes & cross references of
    // any kind, so we must check if pLastSrcPhrase (which is where the collection is to be
    // stored) actually has either kind of TextType value. If so, we must advance
    // pLastSrcPhrase until it points to the first CSourcePhrase instance beyond the
    // footnote, endnote or cross reference. In the case of a selection, this potentially
    // might not be possible (if the user selects only footnote text for instance), so we
    // must allow for such a possibility.
	if (pLastSrcPhrase->m_curTextType == footnote || 
		pLastSrcPhrase->m_curTextType == crossReference)
	{
		// we need to skip this material
		while (iteratorPos != NULL)
		{
			savePos = iteratorPos; // needed for next loop after this one
			pLastSrcPhrase = (CSourcePhrase*)iteratorPos->GetData();
			iteratorPos = iteratorPos->GetNext();
			if (pLastSrcPhrase->m_curTextType == footnote || 
				pLastSrcPhrase->m_curTextType == crossReference)
			{
				pLastSrcPhrase = NULL;
				continue; // skip it, try next one
			}
			else
			{
				// we have found one which we don't need to skip over
				break;
			}
		}
		if (pLastSrcPhrase == NULL)
		{
            // we got to the end of the list without finding one which was not a footnote,
            // endnote or free translation, so don't do any collection, just remove the
            // selection (if there is one) and return
			goto b;
		}
	}

    // do the loop, halting each collection at appropriate (unfiltered) SF markers (such as
    // the start of the next verse if no other marker was encountered beforehand) and store
    // the resulting collection at the starting place for this particular part of the
    // collection (ie. at pLastSrcPhrase instance).
	// BEW changed 02Jan06 to have the code ignore instances with TextType of footnote or 
	// crossReference
	bHalted_at_bt_mkr = FALSE;
	iteratorPos = savePos; // restore POSITION for the pLastSrcPhrase instance, 
						   // which is to start the loop
	while (iteratorPos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)iteratorPos->GetData();
		iteratorPos = iteratorPos->GetNext();

		// skip any footnote or cross reference instances
		if (pSrcPhrase->m_curTextType == footnote || 
			pSrcPhrase->m_curTextType == crossReference)
			continue;

		if (pSrcPhrase == pLastSrcPhrase)
		{
			// we are just starting the next section, so don't try to halt, 
			// just start collecting
			abit = bUseAdaptationsLine ? pSrcPhrase->m_targetStr : pSrcPhrase->m_gloss;
			if (abit == _T("..."))
				abit.Empty();
			strCollect = abit;
		}
		else
		{
            // pSrcPhrase has advanced beyond the anchor instance, pLastSrcPhrase, so we
            // must check for halt condition as we do the collecting
			if (pSrcPhrase->m_markers.IsEmpty()&&
				pSrcPhrase->GetFreeTrans().IsEmpty() &&
				pSrcPhrase->GetCollectedBackTrans().IsEmpty()
				)
			{
				// an empty m_markers means no halt is possible at this pSrcPhrase, so
				// do the collection and then iterate
				if (strCollect.IsEmpty())
				{
					abit = bUseAdaptationsLine ? 
									pSrcPhrase->m_targetStr : pSrcPhrase->m_gloss;
					if (abit == _T("..."))
						abit.Empty();
					strCollect = abit;
				}
				else
				{
					abit = bUseAdaptationsLine ? 
									pSrcPhrase->m_targetStr : pSrcPhrase->m_gloss;
					if (abit == _T("..."))
					{
						abit.Empty();
					}
					else
					{
						strCollect += _T(' ');
						strCollect += abit;
					}
				}
			}
			else
			{
				bHalt = HaltCurrentCollection(pSrcPhrase,bHalted_at_bt_mkr);
				if (bHalt)
				{
					// do the insertion
					InsertCollectedBacktranslation(pLastSrcPhrase, strCollect);

					// prepare for collection in the next document section
					pLastSrcPhrase = pSrcPhrase;
					strCollect.Empty();

                    // collect this one's content before iterating, because
                    // iteratorPos is already pointing at the next POSITION
					strCollect = bUseAdaptationsLine ? 
										pSrcPhrase->m_targetStr : pSrcPhrase->m_gloss;

					if (bSelectionExists && bHalted_at_bt_mkr)
					{
						// we don't collect any further within the selection, 
						// even if not at its end
						goto b;
					}
				}
				else
				{
					// collect here
					if (strCollect.IsEmpty())
					{
						abit = bUseAdaptationsLine ? 	
										pSrcPhrase->m_targetStr : pSrcPhrase->m_gloss;
						if (abit == _T("..."))
							abit.Empty();
						strCollect = abit;
					}
					else
					{
						abit = bUseAdaptationsLine ? 
										pSrcPhrase->m_targetStr : pSrcPhrase->m_gloss;
						if (abit == _T("..."))
						{
							abit.Empty();
						}
						else
						{
							strCollect += _T(' ');
							strCollect += abit;
						}
					}
				}
			} // end block for non-empty m_markers
		} // end block for pSrcPhrase advanced past pLastSrcPhrase
	} // end while loop

	// do the final insertion
	wxASSERT(pLastSrcPhrase);
	InsertCollectedBacktranslation(pLastSrcPhrase, strCollect);

	// remove the selection, if there is one
b:	if (bSelectionExists)
		m_pView->RemoveSelection();
}

// a utility to return TRUE if pSrcPhrase contains a \bt or \bt-prefixed marker
// BEW 2Mar10, updated for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
bool CFreeTrans::ContainsBtMarker(CSourcePhrase* pSrcPhrase)
{
	wxString btMkr = _T("\\bt");
	if (!pSrcPhrase->GetCollectedBackTrans().IsEmpty())
	{
		return TRUE; // m_collectedBackTrans has content (m_filteredInfo may have
					 // derived \btmarker content too, but one or the other will do)
	}
	// no collected string, but we may be storing filtered derived bt marker content
	int nFound = pSrcPhrase->GetFilteredInfo().Find(btMkr);
	if (nFound >= 0)
	{
		// there is a marker in m_filteredInfo which commences with the 
		// 3 characters \bt, so return TRUE
		return TRUE;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////
///  WhichMarker
///
/// Returns: the full marker (including backslash) which is at the passed in location
///
/// Parameters:
///	markers		->	the pSrcPhrase->m_markers string being checked for which kind 
///					of backtranslation marker was found by the caller
///	nAtPos		->	character offset to the backslash at the beginning of the marker, 
///					the marker might be a standard \bt one, or a derived one like \btv etc; 
///					but the function will return whatever marker is there - so it is not 
///                 limited to backtranslation support
/// Remarks:
///	Comments above say it all. The function assumes and relies on there being a space
/// following whatever marker it is
/// BEW 2Mar10, updated for support of doc version 5 (no changes needed)
/// BEW 9July10, no changes needed for support of kbVersion 2
/////////////////////////////////////////////////////////////////////////////////
wxString CFreeTrans::WhichMarker(wxString& markers, int nAtPos)
{
	wxString mkr;
	int curPos = nAtPos;
	wxChar ch;
	ch = markers.GetChar(curPos);
	wxASSERT(ch == _T('\\'));
	mkr += ch;
	curPos++;
	while ((ch = markers.GetChar(curPos)) != _T(' '))
	{
		mkr += ch;
		curPos++;
	}
	return mkr;
}

/////////////////////////////////////////////////////////////////////////////////
/// \returns                nothing
///
///	\param pSrcPhrase	->	pointer to the sourcephrase instance which is to receive the
///					        collected backtranslation string (filtered)
///	\param btStr	    ->	the just-collected backtranslation content which is to be stored
///                         (regarded as "filtered") in the m_collectedBackTrans wxString
///                         member of the passed in pSrcPhrase
/// \remarks
/// In docVersion 5, we insert in that member only back translations collected from the
/// gloss or adaptation lines. If any \bt-derived markers were in a plain text SFM markup
/// used for constructing the document, such as \btv, etc, these are stored, with their
/// \bt-whatever marker, and wrapped in the \~FILTER and \~FILTER* markers, in the
/// m_filteredInfo wxString member -- the latter member may therefore contain more than one
/// such wrapped content string, it all depends what was in the plain text source text
/// document used for parsing in the text. Since the collection operation makes no use of
/// what is in m_filteredInfo (but it does halt scanning if a derived \bt marker is in
/// there), the insertion this function does is just to the m_collectedBackTrans member. If
/// there is already backtranslation content present, what is inserted replaces the content
/// already there.
/// BEW 2Mar10, updated for support of doc version 5
/// BEW 9July10, no changes needed for support of kbVersion 2
//////////////////////////////////////////////////////////////////////////////////
void CFreeTrans::InsertCollectedBacktranslation(CSourcePhrase*& pSrcPhrase, wxString& btStr)
{
	pSrcPhrase->SetCollectedBackTrans(btStr);
}

/////////////////////////////////////////////////////////////////////////////////
/// \returns        TRUE if ptr scanned back to the backslash of an SF marker, 
///                 FALSE otherwise
///
///	\param pBuff		->	pointer to the start of the pSrcPhrase->m_markers buffer
///	\param ptr			<->	iterator, a reference to a pointer to TCHAR at which scanning 
///	                        is to commence, and on exit points either at a space or the
///	                        end of the buffer
///	\param mkrLen		<-	the length, in characters and including the backslash, of the
///					        marker scanned (this parameter has no meaning if FALSE is
///					        returned, and in that case it would equal wxNOT_FOUND (-1) )
/// \remarks
///    We use this function when looking for a marker which is to halt collection of
///    adaptations or glosses as backtranslations. We scan forwards from the start of a
///    string or the end of a previously found marker (a space), and continue on till we
///    find the next marker or the end of the string. If we find a 'next' marker, we scan
///    over it and return to the caller with the offset to its backslash, and the length
///    (defined by character count up to the next space but not including the space). The
///    caller can then extract the marker and use it for testing purposes.
///    
///    BEW 2Mar10, updated for support of doc version 5 (no changes needed) (the legacy
///    version of this function was called GetPrevMarker() and it scanned backwards)
///    BEW 9July10, no changes needed for support of kbVersion 2
/////////////////////////////////////////////////////////////////////////////////
bool CFreeTrans::GetNextMarker(wxChar* pBuff,wxChar*& ptr,int& mkrLen)
{
	int totalLen = 0;
	wxChar* pEnd = pBuff;
	// get the end, so we don't overshoot
	while (*pEnd != _T('\0'))
	{
		totalLen++;
		pEnd++;
	}
	ptr = pBuff; // start at passed in location
	// get the next backslash
	while (*ptr != _T('\\') && ptr < pEnd)
	{
		ptr++;
	}
	if (ptr == pEnd)
	{
		// we didn't find a marker
		mkrLen = wxNOT_FOUND;
		return FALSE;
	}
	else
	{
		// we are at a backslash
		wxChar* p = ptr; // use this one for iterating over the marker
		mkrLen = 1;
		p++;
		// get the rest of the marker
		while (*p != _T(' ') && p < pEnd)
		{
			p++;
			mkrLen++;
		}
		if (mkrLen < 2)
		{
			// a marker consisting of just a backslash is ill-defined
			mkrLen = wxNOT_FOUND;
			return FALSE;
		}
	} // end of block for delimiting a SF marker
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
///  HaltCurrentCollection
///
/// \returns TRUE if the passed in pSrcPhrase->m_markers string contains an unfiltered
///          marker which should halt collecting of the backtranslation text at the current
///          pSrcPhrase; FALSE if collecting should continue BEW modified 21Nov05 to
///          support collecting within the range of a selection by explicitly checking for
///          \bt and immediately returning TRUE if it is in m_markers
///
///	\param pSrcPhrase		->	the current pSrcPhrase being examined, we look at whether
///	                            the m_collectedBackTrans member has text in it or not, and
///	                            if it doesn't, then we look for derived \bt markers in the
///	                            m_filteredInfo member
///	\param bFound_bt_mkr	<-	TRUE if the reason we halt is because back translation
///	                            content was encountered in m_collectedBackTrans, or
///	                            derivative \bt marker type was found in m_filteredInfo
///
/// \remarks
///    Where the halt happens is also where the collection starts for the next
///    subsection, and the caller maintains a pointer to an earlier sourcephrase
///    (pLastSrcPhrase) which is where the just-finished collected backtranslation text
///    is to be stored filtered in its m_collectedBackTrans member.
///
///
///    What halts collection is any one of the following conditions satisfied, in top down
///    order:
///    1. m_collectedBackTrans has content
///    2. m_filteredInfo contains a marker starting with "\bt"
///    3. m_endMarkers contains either "\f*" or "\x*" or "\fe*" (implies end of unfiltered section
///    of either footnote or cross reference or endnote) - anything else, can let collection
///    continue one unhalted provided none of the following obtain (or successfully
///    test for \F or \fe in the PNG SFM set when that SFM set is in use)
///    4. any unknown marker type in m_markers
///    5. any known marker type in m_markers which does not have the TextType of 'none'
///    BEW 2Mar10, updated for support of doc version 5
///    BEW 9July10, no changes needed for support of kbVersion 2
/////////////////////////////////////////////////////////////////////////////////
bool CFreeTrans::HaltCurrentCollection(CSourcePhrase* pSrcPhrase, bool& bFound_bt_mkr)
{
	// initialize
	bFound_bt_mkr = FALSE;
	int mkrLen = 0;
	wxString markers = pSrcPhrase->m_markers;
	int bufLen = markers.Len();

    // coming to an already existing collected back translation in m_collectedBackTrans, or
    // a derived \bt marker in m_filteredInfo, must immediately halt collection, so that we
    // don't encroach on the next collected section
	if (ContainsBtMarker(pSrcPhrase))
	{
		bFound_bt_mkr = TRUE;
		return TRUE;
	}
	// tests 1 and 2 are done, now do 3
	wxString endmarkers = pSrcPhrase->GetEndMarkers();
	int nFound = wxNOT_FOUND;
	if (!endmarkers.IsEmpty())
	{
		if (m_pApp->gCurrentSfmSet == PngOnly)
		{
			nFound = endmarkers.Find(_T("\\F"));
			if (nFound >= 0)
			{
				return TRUE;
			}
			nFound = endmarkers.Find(_T("\\fe"));
			if (nFound >= 0)
			{
				return TRUE;
			}
		}
		else
		{
			// must be the USFM set, or we'll assume so
			nFound = endmarkers.Find(_T("\\f*"));
			if (nFound >= 0)
			{
				return TRUE;
			}
			nFound = endmarkers.Find(_T("\\fe*"));
			if (nFound >= 0)
			{
				return TRUE;
			}
			nFound = endmarkers.Find(_T("\\x*"));
			if (nFound >= 0)
			{
				return TRUE;
			}
		}
	}
	// test 3 is finished, now do tests 4 for an unknown marker, and 5 for a marker which
	// does not have TextType equal to 'none'
	if (markers.IsEmpty())
		return FALSE; // nothing is in m_markers member
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	const wxChar* pBuff = markers.GetData();
	wxChar* pBufStart = (wxChar*)pBuff;
	wxChar* pEnd;
	pEnd = pBufStart + bufLen; // whm added
	wxASSERT(*pEnd == _T('\0')); // whm added
	wxChar* ptr = pBufStart;
	bool bGotMarkerOK = GetNextMarker(pBufStart,ptr,mkrLen);
	if (!bGotMarkerOK)
	{
		return FALSE; // keep collecting
	}
	while (bGotMarkerOK && ptr < pEnd)
	{
		// get the marker we've delineated
		wxString mkr(ptr,mkrLen);
		wxASSERT(!mkr.IsEmpty());

		// if it is an unknown marker, then it must halt collecting
		wxString mkrPlusEqual = mkr + _T('=');
		if (m_pApp->m_currentUnknownMarkersStr.Find(mkrPlusEqual) >= 0)
		{
			return TRUE; // halt
		}
		// that accounts for criterion 4. on this particular marker

		// finally, do test 5 for this marker - if it fails, iterate the loop and test the
		// next delineated marker for halt conditions 4 and 5
		wxString bareMkr = pDoc->GetBareMarkerForLookup(ptr);
		wxASSERT(!bareMkr.IsEmpty());
		USFMAnalysis* pAnalysis = pDoc->LookupSFM(bareMkr);
		if (pAnalysis)
		{
			if (pAnalysis->textType == none || pAnalysis->inLine)
			{
				; // this type does not cause a halt
			}
			else
			{
				// any other type of marker should halt collecting
				return TRUE;
			}
		}

		// prepare for iterating to find and test another marker
		pBufStart = ptr + mkrLen;
		ptr = pBufStart;
		mkrLen = 0;
		bGotMarkerOK = GetNextMarker(pBufStart,ptr,mkrLen);
	} // end of while block
	// if we haven't found a reason to halt, then continue on scanning forwards
	return FALSE;
}
