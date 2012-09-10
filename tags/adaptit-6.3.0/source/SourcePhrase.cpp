/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SourcePhrase.cpp
/// \author			Bill Martin
/// \date_created	12 February 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the Implementation file for the CSourcePhrase class. 
/// The CSourcePhrase class represents what could be called a "TranslationUnit".
/// When the input source text is parsed, each word gets stored on one instance
/// of CSourcePhrase on the heap. Mergers of source text words cause their
/// respective CSourcePhrase instances to be merged to a single one, storing
/// the resulting phrase. CSourcePhrase also stores the final associated target 
/// text adaptation/translation, along with a number of different attributes 
/// associated with the overall translation unit, including sfm markers, preceeding
/// and following punctuation, its sequence number, text type, etc.
/// \derivation		The CSourcePhrase class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "SourcePhrase.h"
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

#include "Adapt_It.h"
#include "helpers.h"
#include "SourcePhrase.h"
#include "AdaptitConstants.h"
#include "RefString.h" // needed here???
#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"
#include "BString.h"
#include "XML.h"

#include <wx/wfstream.h> // for wxFileInputStream and wxFileOutputStream
#include <wx/datstrm.h> // for wxDataInputStream and wxDataOutputStream

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

// whm modified type declaration of filterMkr and filterMkrEnd below to agree with the
// declaration in Adapt_ItDoc.cpp
extern wxString filterMkr; //extern const wxChar* filterMkr; // defined in the Doc
extern wxString filterMkrEnd; //extern const wxChar* filterMkrEnd; // defined in the Doc

const int filterMkrLen = 8;
const int filterMkrEndLen = 9;
extern wxChar gSFescapechar;

// Define type safe pointer lists
#include "wx/listimpl.cpp"

/// This macro together with the macro list declaration in the .h file
/// complete the definition of a new safe pointer list class called SPList.
WX_DEFINE_LIST(SPList);

IMPLEMENT_DYNAMIC_CLASS(CSourcePhrase, wxObject)
// MFC uses IMPLEMENT_SERIAL(CSourcePhrase, CObject, VERSIONABLE_SCHEMA | VERSION_NUMBER)
// Design Note:
// MFC's versionable schema adds the class name as a string plus 4 or more bytes 
// before every new object/class and varying amounts of bytes for repeated objects
// of the same class in the datafile. There is no easily predictable way of 
// determining what bytes or even how many bytes will be added in MFC's 
// versioning schema. Microsoft hasn't made that info readily available at 
// least in MFC docs I can find, and the critical code isn't traceable with
// the IDE debugger. Since wxWidgets does not implement a similar 
// VERSION_SCHEMA facility (probably because of portability considerations 
// between the supported platforms), it is not likely I would be able to 
// construct an exact byte-for-byte .adt data file completely compatible 
// with an existing MFC serialized archive file containing versionable 
// schema bytes embedded into it. The seemingly inevitable consequence 
// is that, in the wxWidgets version, we will not be able to directly read data 
// files produced in the MFC version. It may be possible to create a small 
// conversion program (or an export routine in any future MFC version of the app) 
// that would read an MFC-produced data file and convert it to a compatible 
// wxWidgets version, to help users of the MFC version transition to the
// wxWidgets version.

// BEW 27Feb12, replaced unused m_bHasBookmark with m_bSectionByVerse for improved free
// translation support
CSourcePhrase::CSourcePhrase()
{
	m_chapterVerse = _T("");
	m_bFootnoteEnd = FALSE;
	m_bFootnote = FALSE;
	m_bChapter = FALSE;
	m_bVerse = FALSE;
	// BEW 8Oct10, repurposed m_bParagraph as m_bUnused
	//m_bParagraph = FALSE;
	m_bUnused = FALSE;
	m_bSpecialText = FALSE;
	m_bBoundary = FALSE;
	m_bHasInternalMarkers = FALSE;
	m_bHasInternalPunct = FALSE;
	m_bFirstOfType = FALSE;
	m_curTextType = verse;
	m_pSavedWords = (SPList*)NULL; // initially, no list
	m_pMedialPuncts = (wxArrayString*)NULL; // ditto
	m_pMedialMarkers = (wxArrayString*)NULL; // ditto
	m_bRetranslation = FALSE;
	m_bNullSourcePhrase = FALSE;
	m_bHasKBEntry = FALSE;
	m_bHasGlossingKBEntry = FALSE; // VERSION_NUMBER == 3
	m_markers   = _T("");
	m_follPunct = _T("");
	m_precPunct = _T("");
	m_follOuterPunct = _T("");
	m_srcPhrase = _T("");
	m_key		= _T("");
	m_targetStr = _T("");
	m_gloss		= _T(""); // VERSION_NUMBER == 3
	m_nSrcWords = 1;
	m_nSequNumber = 0;
	m_bNotInKB = FALSE; // must be initialized or GetNextEmptyPile function will loop to end of bundle
						// without finding the next pile with an empty adaption slot

	// two new flags for version_number == 2, 
	// these flags used for preventing concatenation of retranslations
	m_bBeginRetranslation = FALSE;
	m_bEndRetranslation = FALSE;

	// defaults for schema 4 (free translations, notes, sectioning by 'verse', added)
	m_bHasFreeTrans = FALSE;
	m_bStartFreeTrans = FALSE;
	m_bEndFreeTrans = FALSE;
	m_bHasNote = FALSE;
	m_bSectionByVerse = FALSE;

	m_endMarkers = _T("");
	m_freeTrans = _T("");
	m_note = _T("");
	m_collectedBackTrans = _T("");
	m_filteredInfo = _T("");

	m_inlineBindingMarkers = _T("");
	m_inlineBindingEndMarkers = _T("");
	m_inlineNonbindingMarkers = _T("");
	m_inlineNonbindingEndMarkers = _T("");

	// create the stored lists, so that serialization won't crash if one is unused
	m_pSavedWords = new SPList;
	wxASSERT(m_pSavedWords != NULL);
	m_pMedialMarkers = new wxArrayString;
	wxASSERT(m_pMedialMarkers != NULL);
	m_pMedialPuncts = new wxArrayString;
	wxASSERT(m_pMedialPuncts != NULL);

	// for docVersion6
	m_lastAdaptionsPattern = _T("");
	m_tgtMkrPattern = _T("");
	m_glossMkrPattern = _T("");
	m_punctsPattern = _T("");

//#ifdef __WXDEBUG__
// Leave this stuff here, commented out -- see comment in destructor for why
//	wxLogDebug(_T("Creator: address = %x  first array = %x  second array = %x  SPList = %x"),
//		(unsigned int)this, (unsigned int)this->m_pMedialMarkers, (unsigned int)this->m_pMedialPuncts,
//		(unsigned int)this->m_pSavedWords);
//#endif
}

CSourcePhrase::~CSourcePhrase()
{
//#ifdef __WXDEBUG__
	// Don't remove this, just comment it out when not wanted, it is potentially valuable
	// because if anyone has a memory lapse and codes "delete pSrcPhrase;" somewhere,
	// instead of the correct "DeleteSingleSrcPhrase(pSrcPhrase);", then the former will
	// lead to the two wxArrayString instances on the heap (each 16 bytes) and the one
	// SPList on the heap (28 bytes) being leaked. This can be detected by this wxLogDebug
	// call because if the Delete...() function mentioned above is used, the first array,
	// second array, and SPList values will be written to the Output window each have a 0
	// (zero) value; but if the values are non-zero, you can be sure there is a "delete
	// pSrcPhrase;" line somewhere in the code - a breakpoint in this destructor will then
	// enable you to find where in the code the error is. (Try generate the error with a
	// short document, to save yourself a lot of tedious button pressing.)
	//wxLogDebug(_T("Destructor: address = %x  first array = %x  second array = %x  SPList = %x"),
	//	(unsigned int)this, (unsigned int)this->m_pMedialMarkers, (unsigned int)this->m_pMedialPuncts,
	//	(unsigned int)this->m_pSavedWords);
//#endif
}

// BEW 27Feb12, replaced unused m_bHasBookmark with m_bSectionByVerse for improved free
// translation support
CSourcePhrase::CSourcePhrase(const CSourcePhrase& sp)// copy constructor
{
	m_inform = sp.m_inform;
	m_chapterVerse = sp.m_chapterVerse;
	m_curTextType = sp.m_curTextType;
	m_markers = sp.m_markers;
	m_follPunct = sp.m_follPunct;
	m_precPunct = sp.m_precPunct;
	m_srcPhrase = sp.m_srcPhrase;
	m_key = sp.m_key;
	m_adaption = sp.m_adaption;
	m_nSrcWords = sp.m_nSrcWords;
	m_nSequNumber = sp.m_nSequNumber;
	m_bFirstOfType = sp.m_bFirstOfType;
	m_bFootnoteEnd = sp.m_bFootnoteEnd;
	m_bFootnote = sp.m_bFootnote;
	m_bChapter = sp.m_bChapter;
	m_bVerse = sp.m_bVerse;
	// BEW 8Oct10, repurposed m_bParagraph as m_bUnused
	//m_bParagraph = sp.m_bParagraph;
	m_bUnused = sp.m_bUnused;
	m_bSpecialText = sp.m_bSpecialText;
	m_bBoundary = sp.m_bBoundary;
	m_bHasInternalMarkers = sp.m_bHasInternalMarkers;
	m_bHasInternalPunct = sp.m_bHasInternalPunct;
	m_bRetranslation = sp.m_bRetranslation;
	m_bNotInKB = sp.m_bNotInKB;
	m_targetStr = sp.m_targetStr;
	m_gloss = sp.m_gloss; // VERSION_NUMBER == 3
	m_bHasGlossingKBEntry = sp.m_bHasGlossingKBEntry; // VERSION_NUMBER == 3
	m_bNullSourcePhrase = sp.m_bNullSourcePhrase;
	m_bHasKBEntry = sp.m_bHasKBEntry;
	
	// and the two new booleans
	m_bBeginRetranslation = sp.m_bBeginRetranslation;
	m_bEndRetranslation = sp.m_bEndRetranslation;

	// defaults for schema 4 (free translations, notes, bookmarks added)
	m_bHasFreeTrans = sp.m_bHasFreeTrans;
	m_bStartFreeTrans = sp.m_bStartFreeTrans;
	m_bEndFreeTrans = sp.m_bEndFreeTrans;
	m_bHasNote = sp.m_bHasNote;
	m_bSectionByVerse = sp.m_bSectionByVerse;

	// the doc version 4's 10 new members
	m_endMarkers = sp.m_endMarkers;
	m_freeTrans = sp.m_freeTrans;
	m_note = sp.m_note;
	m_collectedBackTrans = sp.m_collectedBackTrans;
	m_filteredInfo = sp.m_filteredInfo;

	m_inlineBindingMarkers = sp.m_inlineBindingMarkers;
	m_inlineBindingEndMarkers = sp.m_inlineBindingEndMarkers;
	m_inlineNonbindingMarkers = sp.m_inlineNonbindingMarkers;
	m_inlineNonbindingEndMarkers = sp.m_inlineNonbindingEndMarkers;
	m_follOuterPunct = sp.m_follOuterPunct;

	// create the stored lists, so that serialization won't crash if one is unused
	m_pSavedWords = new SPList;
	wxASSERT(m_pSavedWords != NULL);
	m_pMedialMarkers = new wxArrayString; // MFC uses CStringList;
	wxASSERT(m_pMedialMarkers != NULL);
	m_pMedialPuncts = new wxArrayString; // MFC uses CStringList;
	wxASSERT(m_pMedialPuncts != NULL);

	if (sp.m_pSavedWords->GetCount() > 0)
	{
		// there is a list to be copied (we can only copy pointers, so beware in any code
		// which makes a temporary copy of a list of source phrases with saved
		// sublist in m_pSavedWords, when deleting the temporary copies, the pointers in
		// m_pSavedWords list must not be deleted, only removed from the list)
		// Note: wxWidgets wxList Append() method cannot append another list
		// to the tail of an existing wxList, so we need to add the items one 
		// at a time via a for loop
		for ( SPList::Node *node = sp.m_pSavedWords->GetFirst(); node; node = node->GetNext() )
		{
			CSourcePhrase *pData = node->GetData();
			m_pSavedWords->Append(pData); // copy the pointers across
		}
	}

	// copy the list of saved medial punctuations, if any
	if (sp.m_pMedialPuncts->GetCount() > 0)
	{
		// there is a list to be copied
		// Note: wxWidgets wxArrayString Add() method doesn't take another wxArrayString 
		// as a parameter so we need to add the items one at a time via a for loop
		size_t count = sp.m_pMedialPuncts->GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
			// add the item
			m_pMedialPuncts->Add(sp.m_pMedialPuncts->Item(n));
						 //(makes duplicates of the strings, does not copy pointers)
		}
	}

	// copy the list of saved medial markers, if any
	if (sp.m_pMedialMarkers->GetCount() > 0)
	{
		// there is a list to be copied
		size_t count = sp.m_pMedialMarkers->GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
			// add the item
			m_pMedialMarkers->Add(sp.m_pMedialMarkers->Item(n));
						 //(makes duplicates of the strings, does not copy pointers)
		}
	}

	// for docVersion6
	m_lastAdaptionsPattern = sp.m_lastAdaptionsPattern;
	m_tgtMkrPattern = sp.m_tgtMkrPattern;
	m_glossMkrPattern = sp.m_glossMkrPattern;
	m_punctsPattern = sp.m_punctsPattern;
}

// BEW 27Feb12, replaced unused m_bHasBookmark with m_bSectionByVerse for improved free
// translation support
CSourcePhrase& CSourcePhrase::operator =(const CSourcePhrase &sp)
{
	if (this == &sp)
		return *this;
	m_inform = sp.m_inform;
	m_chapterVerse = sp.m_chapterVerse;
	m_curTextType = sp.m_curTextType;
	m_markers = sp.m_markers;
	m_follPunct = sp.m_follPunct;
	m_precPunct = sp.m_precPunct;
	m_srcPhrase = sp.m_srcPhrase;
	m_key = sp.m_key;
	m_gloss = sp.m_gloss; // VERSION_NUMBER == 3
	m_bHasGlossingKBEntry = sp.m_bHasGlossingKBEntry; // VERSION_NUMBER == 3
	m_nSrcWords = sp.m_nSrcWords;
	m_nSequNumber = sp.m_nSequNumber;
	m_bFirstOfType = sp.m_bFirstOfType;
	m_bFootnoteEnd = sp.m_bFootnoteEnd;
	m_bFootnote = sp.m_bFootnote;
	m_bChapter = sp.m_bChapter;
	m_bVerse = sp.m_bVerse;
	// BEW 8Oct10, repurposed m_bParagraph as m_bUnused
	//m_bParagraph = sp.m_bParagraph;
	m_bUnused = sp.m_bUnused;
	m_bSpecialText = sp.m_bSpecialText;
	m_bBoundary = sp.m_bBoundary;
	m_bHasInternalMarkers = sp.m_bHasInternalMarkers;
	m_bHasInternalPunct = sp.m_bHasInternalPunct;
	m_bRetranslation = sp.m_bRetranslation;
	m_bNotInKB = sp.m_bNotInKB;
	m_adaption = sp.m_adaption;
	m_targetStr = sp.m_targetStr;
	m_bNullSourcePhrase = sp.m_bNullSourcePhrase;
	m_bHasKBEntry = sp.m_bHasKBEntry;
	
	// and the two new booleans
	m_bBeginRetranslation = sp.m_bBeginRetranslation;
	m_bEndRetranslation = sp.m_bEndRetranslation;

	// and versionable_schema 4 five new booleans
	m_bHasFreeTrans = sp.m_bHasFreeTrans;
	m_bStartFreeTrans = sp.m_bStartFreeTrans;
	m_bEndFreeTrans = sp.m_bEndFreeTrans;
	m_bHasNote = sp.m_bHasNote;
	m_bSectionByVerse = sp.m_bSectionByVerse;

	// the doc version five's 10 new members
	m_endMarkers = sp.m_endMarkers;
	m_freeTrans = sp.m_freeTrans;
	m_note = sp.m_note;
	m_collectedBackTrans = sp.m_collectedBackTrans;
	m_filteredInfo = sp.m_filteredInfo;

	m_inlineBindingMarkers = sp.m_inlineBindingMarkers;
	m_inlineBindingEndMarkers = sp.m_inlineBindingEndMarkers;
	m_inlineNonbindingMarkers = sp.m_inlineNonbindingMarkers;
	m_inlineNonbindingEndMarkers = sp.m_inlineNonbindingEndMarkers;
	m_follOuterPunct = sp.m_follOuterPunct;

	// create the stored lists, so that serialization won't crash if one is unused
	if (m_pSavedWords == NULL)
		m_pSavedWords = new SPList;
	wxASSERT(m_pSavedWords != NULL);
	if (m_pMedialMarkers == NULL)
		m_pMedialMarkers = new wxArrayString;
	wxASSERT(m_pMedialMarkers != NULL);
	if (m_pMedialPuncts == NULL)
		m_pMedialPuncts = new wxArrayString;
	wxASSERT(m_pMedialPuncts != NULL);

	// copy the list of saved source words, if any  (NOTE: for sublists, we copy only
	// the pointers, so be careful in code which uses this operator)
	if (m_pSavedWords->GetCount() > 0)
	{
		// remove old pointers before copying new ones
		m_pSavedWords->Clear();
	}
	if (sp.m_pSavedWords->GetCount() > 0)
	{
		// there is a list to be copied
		for ( SPList::Node *node = sp.m_pSavedWords->GetFirst(); node; node = node->GetNext() )
		{
			CSourcePhrase *pData = node->GetData();
			m_pSavedWords->Append(pData); // copy the pointers across
		}
	}

	// copy the list of saved medial punctuations, if any
	if (m_pMedialPuncts->GetCount() > 0)
	{
		// remove old strings before copying new ones
		m_pMedialPuncts->Clear(); // MFC has RemoveAll();
	}
	if (sp.m_pMedialPuncts->GetCount() > 0)
	{
		// there is a list to be copied
		// Note: wxWidgets wxArrayString Add() method doesn't take another wxArrayString 
		// as a parameter so we need to add the items one at a time via a for loop
		//m_pMedialPuncts->Add(sp.m_pMedialPuncts); // copy them across
		size_t count = sp.m_pMedialPuncts->GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
			// add the item
			m_pMedialPuncts->Add(sp.m_pMedialPuncts->Item(n));
						// (makes duplicates of the strings, does not copy pointers)
		}
	}

	// copy the list of saved medial markers, if any
	if (m_pMedialMarkers->GetCount() > 0)
	{
		// remove old strings before copying new ones
		m_pMedialMarkers->Clear(); // MFC has RemoveAll();
	}
	if (sp.m_pMedialMarkers->GetCount() > 0)
	{
		// there is a list to be copied (makes duplicates of the strings, does not copy pointers)
		size_t count = sp.m_pMedialMarkers->GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
			// add the item
			m_pMedialMarkers->Add(sp.m_pMedialMarkers->Item(n));
		}
	}

	// for docVersion6
	m_lastAdaptionsPattern = sp.m_lastAdaptionsPattern;
	m_tgtMkrPattern = sp.m_tgtMkrPattern;
	m_glossMkrPattern = sp.m_glossMkrPattern;
	m_punctsPattern = sp.m_punctsPattern;

	return *this;
}

// BEW added 16Apr08, to obtain copies of any saved original
// CSourcePhrases from a merger, and have pointers to the copies
// replace the pointers in the m_pSavedWords member of a new instance
// of CSourcePhrase produced with the copy constructor or operator=
// Usage: for example: 
// CSourcePhrase oldSP; ....more application code defining oldSP contents....
// CSourcePhrase* pNewSP = new CSourcePhrase(oldSP); <<-- this uses operator=
// then do:		pNewSP->DeepCopy(); <<-- *pNewSP is now a deep copy of oldSP
// What DeepCopy() does is take the list of pointers to CSourcePhrase which
// are in the m_pSavedWords SPList, for each of them it defines a new
// CSourcePhrase instance using operator= which duplicates everything (including
// the wxStrings in the string lists) except that it copies pointers only for
// m_pSavedWords, but since original CSourcePhrase instances from m_pSavedWords
// can never contain any CSourcePhrase instances in their m_pSavedWords member,
// there is no recursion and the result of the DeepCopy() call is then a
// CSourcePhrase instance which is, in every respect, a duplicate of the
// oldSP one - that is, any saved original CSourcePhrase instances are also
// duplicates of those pointed at by the m_pSavedWords list in oldSP
// If the m_pSavedWords list is empty, the DeepCopy() operation does nothing
// & the owning CSourcePhrase instance is already a deep copy
// BEW 22Mar10, updated for support of doc version 5 (no changes needed)
void CSourcePhrase::DeepCopy(void)
{
	SPList::Node* pos = m_pSavedWords->GetFirst(); //POSITION pos = m_pSavedWords->GetHeadPosition();
	CSourcePhrase* pSrcPhrase = NULL;
	if (pos == NULL)
		return; // there are no saved CSourcePhrase instances to be copied
	while (pos != NULL)
	{
		// save the POSITION
		SPList::Node* savePos = pos; //POSITION savePos = pos;
		// get a pointer to the next of the original CSourcePhrases of a merger
		// (these never have any content in their m_pSavedWords member)
		pSrcPhrase = pos->GetData();
		pos = pos->GetNext();
		// clone it using operator= (its m_pSavedWords SPList* is created but 
		// the list is left empty)
		CSourcePhrase* pSrcPhraseDuplicate = new CSourcePhrase(*pSrcPhrase);
		// replace the list's pointer at savePos with this new pointer
		////m_pSavedWords->SetAt(savePos,pSrcPhraseDuplicate);
		// wx Note: wxList doesn't have a SetAt method, but we can use DeleteNode and Insert to achieve
		// the same effect. 
		// The following order won't work (wxList asserts):
		//  m_pSavedWords->DeleteNode(savePos))
		//	m_pSavedWords->Insert(savePos,pSrcPhraseDuplicate);
		// We need to first do the Insert (which inserts "before"), then get the
		// next position and call DeleteNode on that next position.
		// 
		// whm additional Note: It appears that the MFC in simply using SetAt() creates a 
		// memory leak because the previous pointer being replaced is not deleted. In fact, 
		// in the docs for CObList::SetAt() the example there shows that it should be 
		// invoked as follows (customized for our code situation) to avoid memory leaks:
		//    pSP = m_pSavedWords->GetAt( pos ); // Save the old pointer for deletion.
		//    m_pSavedWords->SetAt( pos, pSrcPhraseDuplicate );  // Replace the element.
		//    delete pSP;  // Deletion avoids memory leak.
		// whm update: No, we musn't delete the other object pointer that is being replaced
		// in this case.
		SPList::Node* nextPos;
		SPList::Node* insertedPos;
		//CSourcePhrase* pSPtoDelete;
		//pSPtoDelete = savePos->GetData();
		insertedPos = m_pSavedWords->Insert(savePos,pSrcPhraseDuplicate); // pSrcPhraseDuplicate is now at savePos
		nextPos = insertedPos->GetNext();
		bool deletedOK;
		deletedOK = m_pSavedWords->DeleteNode(nextPos);
		//delete pSPtoDelete;
		wxASSERT(deletedOK != FALSE);
 		deletedOK = deletedOK; // avoid warning (retain as is)
	}
}

void CSourcePhrase::CopySameTypeParams(const CSourcePhrase &sp)
{
	m_curTextType = sp.m_curTextType;
	m_bSpecialText = sp.m_bSpecialText;
	m_bRetranslation = sp.m_bRetranslation;
}

bool CSourcePhrase::Merge(CAdapt_ItView* WXUNUSED(pView), CSourcePhrase *pSrcPhrase)
{
	if (this == pSrcPhrase)
		return FALSE; // don't merge with itself
	
	// save the original in the m_pSavedWords list, in case we later want
	// to restore the original sequence of minimal CSourcePhrase instances
	if (m_pSavedWords == NULL)
	{
		m_pSavedWords = new SPList; // get a new list if there isn't one yet
		wxASSERT(m_pSavedWords != NULL);
	}

	if (m_pSavedWords->GetCount() == 0)
	{
		// save the current src phrase in it as its first element, if list is empty
		CSourcePhrase* pSP = new CSourcePhrase(*this); // use copy constructor, the
				// fact that it is not a deep copy doesn't matter, because originals
				// can never have medial puncts, nor medial markers, nor stored instances
				// of CSourcePhrase, & everything else gets copied in the shallow copy
		wxASSERT(pSP != NULL);
		m_pSavedWords->Append(pSP); // adding single item
	}

	m_pSavedWords->Append(pSrcPhrase); // next store the pointer to the srcPhrase being merged
										// saving single item

	m_chapterVerse += pSrcPhrase->m_chapterVerse; // append ch:vs if it exists on the 2nd original

	// if there is a marker, it will become phrase-internal, so accumulate it and set
	// the flag
	if (!pSrcPhrase->m_markers.IsEmpty() || !pSrcPhrase->m_inlineNonbindingMarkers.IsEmpty()
		|| !pSrcPhrase->m_inlineBindingMarkers.IsEmpty())
	{
		// any such beginmarkers will end up medial - so accumulate them, etc.
		m_bHasInternalMarkers = TRUE;

		// make a string list to hold it, if none exists yet
		if (m_pMedialMarkers == NULL)
			m_pMedialMarkers = new wxArrayString;
		wxASSERT(m_pMedialMarkers != NULL);

		// accumulate any which exist (in the order inline non-binding, those in m_markers
		// (including any verse or chapter number strings etc), then inline binding ones;
		// if any storage has multiple markers, accept them all as a unit rather than one
		// by one (that will show the user in the placement dialog which ones go together)
		wxString mkrStr = pSrcPhrase->GetInlineNonbindingMarkers();
		if (!mkrStr.IsEmpty())
		{
			m_pMedialMarkers->Add(mkrStr);
			mkrStr.Empty();
		}
		mkrStr = pSrcPhrase->m_markers;
		if (!mkrStr.IsEmpty())
		{
			m_pMedialMarkers->Add(mkrStr);
			mkrStr.Empty();
		}
		mkrStr = pSrcPhrase->GetInlineBindingMarkers();
		if (!mkrStr.IsEmpty())
		{
			m_pMedialMarkers->Add(mkrStr);
			mkrStr.Empty();
		}
	}

    // any endmarkers on pSrcPhrase are not yet medial because on this call of Merge() they
    // are at the end of the phrase; but if they are present at a later iteration on the
    // CSourcePhrase which is pointed at by the this pointer, then they become medial and
    // have to be accumulated in m_MedialMarkers - and then on export of the data the
    // placement dialog will come up so they can be 'placed' correctly
	//bool bAddedSomething = FALSE;
	if (!pSrcPhrase->m_endMarkers.IsEmpty() || !pSrcPhrase->m_inlineNonbindingEndMarkers.IsEmpty()
		|| !pSrcPhrase->m_inlineBindingEndMarkers.IsEmpty())
	{
		// the CSourcePhrase being merged to its predecessor has content in one of more of its
		// 3 storage locations which store endmarkers, i.e. m_endMarkers member, or the 2
		// for inline binding or non-binding endmarkers
		if (m_endMarkers.IsEmpty() && m_inlineNonbindingEndMarkers.IsEmpty() 
			&& m_inlineBindingEndMarkers.IsEmpty())
		{
            // these 3 locations are empty, and the ones on pSrcPhrase are not yet medial,
            // (but a subsequent call of Merge() would make them so) so just copy all 3 of
            // pSrcPhrase's endmarker storages locations, and don't add anything to
            // m_pMedialMarkers in the this pointer
			SetEndMarkers(pSrcPhrase->GetEndMarkers());
			SetInlineNonbindingEndMarkers(pSrcPhrase->GetInlineNonbindingEndMarkers());
			SetInlineBindingEndMarkers(pSrcPhrase->GetInlineBindingEndMarkers());
		}
		else
		{
			// there are endMarkers already on the 'this' pointer, so these are medial
			// now, and need to be stored in conformity to that fact; their former storage
			// locations are then to be cleard, and the endmarkers stored on pSrcPhrase
			// (those are not yet medial) are to be moved to the 3 storage strings on the
			// merged CSourcePhrase instance (the this pointer)
			
            // first deal with the endmarkers that have just become 'old' ie. medial to the
            // phrase (the following call tests for m_pMedialMarkers = NULL, and if so
            // creates a string array for it on the heap, so the use of m_pMedialMarkers
            // further below is safe)
			SetEndMarkersAsNowMedial(m_pMedialMarkers); // sets m_bHasInternalMarkers = TRUE;
	
			// now clear those locations on the merged CSourcePhrase instance
			wxString emptyStr = _T("");
			SetEndMarkers(emptyStr);
			SetInlineNonbindingEndMarkers(emptyStr);
			SetInlineBindingEndMarkers(emptyStr);

			// now transfer the endmarkers from pSrcPhrase, these are now endmarkers on
			// the merged CSourcePhrase instance too
			SetEndMarkers(pSrcPhrase->GetEndMarkers());
			SetInlineNonbindingEndMarkers(pSrcPhrase->GetInlineNonbindingEndMarkers());
			SetInlineBindingEndMarkers(pSrcPhrase->GetInlineBindingEndMarkers());
		}
	}
	else if (!this->m_endMarkers.IsEmpty() || !m_inlineNonbindingEndMarkers.IsEmpty()
		|| !m_inlineBindingEndMarkers.IsEmpty())
	{
		// these endmarkers have become medial now, but pSrcPhrase has none to contribute;
		// here we have the possibility that endmarkers have just been made non-final, and
		// so have become internal to the phrase, so we must update the medial markers array
		SetEndMarkersAsNowMedial(m_pMedialMarkers);

		// now clear those locations
		wxString emptyStr = _T("");
		SetEndMarkers(emptyStr);
		SetInlineNonbindingEndMarkers(emptyStr);
		SetInlineBindingEndMarkers(emptyStr);
	}

	// if there is punctuation, some or all may become phrase-internal, so check it out and
	// accumulate as necessary and then set the flag if there is internal punctuation
	if (!m_follPunct.IsEmpty())
	{
        // the current srcPhrase has following punctuation, so adding a further srcPhrase
        // will make that punctuation become phrase-internal, so set up accordingly
		m_bHasInternalPunct = TRUE;

		// create a list, if one does not yet exist
		if (m_pMedialPuncts == NULL)
			m_pMedialPuncts = new wxArrayString;
		wxASSERT(m_pMedialPuncts != NULL);

		// put the follPunct string into the list & clear the attribute
		m_pMedialPuncts->Add(m_follPunct);
		m_follPunct = _T("");
	}
	if (!m_follOuterPunct.IsEmpty())
	{
        // the current srcPhrase has following outer punctuation, so adding a further
        // srcPhrase will make that punctuation become phrase-internal, so set up
        // accordingly
		m_bHasInternalPunct = TRUE;

		// create a list, if one does not yet exist
		if (m_pMedialPuncts == NULL)
			m_pMedialPuncts = new wxArrayString;
		wxASSERT(m_pMedialPuncts != NULL);

		// put the m_follOuterPunct string into the list & clear the attribute
		m_pMedialPuncts->Add(GetFollowingOuterPunct());
		SetFollowingOuterPunct(_T(""));
	}

	if (!pSrcPhrase->m_precPunct.IsEmpty())
	{
		// the preceding punctuation on pSrcPhrase will end up medial - so 
		// accumulate it, etc.
		m_bHasInternalPunct = TRUE;

		// make a string list to hold it, if none exists yet
		if (m_pMedialPuncts == NULL)
			m_pMedialPuncts = new wxArrayString;
		wxASSERT(m_pMedialPuncts != NULL);

		// accumulate it, leaving the original member unchanged as pSrcPhrase will become
		// an 'old' one in the m_pSavedWords list, so we may need it later for unmerging
		m_pMedialPuncts->Add(pSrcPhrase->m_precPunct);
	}

	// any final punctuation on the merged srcPhrase will be non-medial, so just copy it
	if (!pSrcPhrase->m_follPunct.IsEmpty())
		m_follPunct = pSrcPhrase->m_follPunct;
	if (!pSrcPhrase->GetFollowingOuterPunct().IsEmpty())
		SetFollowingOuterPunct(pSrcPhrase->GetFollowingOuterPunct());

	// append the m_srcPhrase string 
	// do I do it in reverse order for RTL layout? - I don't think so; but in case I should
	// I will code it and comment it out. I think I have to do the appending in logical order,
	// and for text which is RTL, the resulting phrase will be laid out RTL in the CEdit
	// For version 3, allow for empty strings; but m_srcPhrase cannot be empty so we
	// don't need a test here, but we do for the key
	m_srcPhrase += _T(" ") + pSrcPhrase->m_srcPhrase; 

	// ditto for the key string, for version 3 allow for empty strings
	if (m_key.IsEmpty())
	{
		m_key = pSrcPhrase->m_key;
	}
	else
	{
		m_key += _T(" ") + pSrcPhrase->m_key;
	}

	// do the same for the m_adaption and m_targetStr fields
	if (m_adaption.IsEmpty())
		m_adaption = pSrcPhrase->m_adaption;
	else
	{
			m_adaption += _T(" ") + pSrcPhrase->m_adaption;
	}

	if (m_targetStr.IsEmpty())
		m_targetStr = pSrcPhrase->m_targetStr;
	else
	{
			m_targetStr += _T(" ") + pSrcPhrase->m_targetStr;
	}

	// likewise for the m_gloss field in VERSION_NUMBER == 3
	if (m_gloss.IsEmpty())
		m_gloss = pSrcPhrase->m_gloss;
	else
	{
			m_gloss += _T(" ") + pSrcPhrase->m_gloss;
	}

	// doc version 5, these new members (an additional one is above)
    // free translations, notes, collected back translations or filtered information are
    // only allowed on the first CSourcePhrase in a merger - we filter any attempt to merge
    // across a CSourcePhrase which carries such information, warn the user and abort the
    // merge attempt. But just in case something goes wrong and a merge is attempted in
    // such a scenario, here we can just append the strings involved to give a behaviour
    // that makes sense (ie. accumulating two notes, or two free translations, etc if any
    // such should slip through our net of checks to prevent this)
	if (!pSrcPhrase->m_freeTrans.IsEmpty())
		m_freeTrans = m_freeTrans + _T(" ") + pSrcPhrase->m_freeTrans;
	if (!pSrcPhrase->m_note.IsEmpty())
		m_note = m_note + _T(" ") + pSrcPhrase->m_note;
	if (!pSrcPhrase->m_collectedBackTrans.IsEmpty())
		m_collectedBackTrans = m_collectedBackTrans + _T(" ") + pSrcPhrase->m_collectedBackTrans;
	if (!pSrcPhrase->m_filteredInfo.IsEmpty())
		m_filteredInfo = m_filteredInfo + _T(" ") + pSrcPhrase->m_filteredInfo;


	// increment the number of words in the phrase
	m_nSrcWords += pSrcPhrase->m_nSrcWords;

	// if we are merging a bounding srcPhrase, then the phrase being constructed is also 
	// a boundary
	if (pSrcPhrase->m_bBoundary)
		m_bBoundary = TRUE;

    // copy across the BOOL values for free translation, note, and free translation
    // sectioning criterion, for versionable_schema 4, 5 & 6 etc (1) if the first instance
    // sections a free translation by 'verse' or 'punctuation', then the merger must
    // section the same way (unless the user explicitly changes it), so change the original
    // to agree
	if (m_bSectionByVerse)
	{
		pSrcPhrase->m_bSectionByVerse = TRUE;
	}
	else
	{
		pSrcPhrase->m_bSectionByVerse = FALSE;
	}
    // (2) if the first has a note, then so must the merger - we don't need to do anything
    // because the merger process will do it automatically because m_bHasNote was TRUE on
    // the first in the merger and merging across filtered material (of which note is an
    // example) is not permitted
    // (3) handling m_bStartFreeTrans is done automatically by the merging protocol, it's
    // m_bEndFreeTrans that we must make sure it properly dealt with
	if (pSrcPhrase->m_bHasFreeTrans)
		m_bHasFreeTrans = TRUE;
	if (pSrcPhrase->m_bEndFreeTrans)
		m_bEndFreeTrans = TRUE;

	// for the 3 new storage strings added for docVersion6, clear them all - the first two
	// are needed for tgt or gloss exports, and the 3rd for medial punctuation storage -
	// clearing them means the relevant Placement dialog will open at least once so the
	// user will get the chance to make the relevant placements once only (3rd is unused
	// at present)
	// for docVersion6
	m_lastAdaptionsPattern = _T("");
	m_tgtMkrPattern = _T("");
	m_glossMkrPattern = _T("");
	m_punctsPattern = _T("");

    // we never merge phrases, only minimal phrases (ie. single source word objects), so it
    // will never be the case that we need to copy from m_pSaveWords in the Merge function
    // and similarly for the m_pMedialPuncts and m_pMedialMarkers lists.
	return TRUE;
}

// MakeXML makes the <S> ... </S> productions which store the data in CSourcePhrase
// instances, each pass through the function makes the production for one CSourcePhrase.
// Attribute names are kept short to avoid undue bloat, but newer ones may need a little
// explaining: here are the ones for the 5 new data stores added by BEW on 11Oct10 for
// document version 5 to better handle USFM markup.
// for m_inlineBindingMarkers use iBM
// for m_inlineBindingEndMarkers use iBEM
// for m_inlineNonbindingMarkers use iNM
// for m_inlineNonbindingEndMarkers use iNEM
// for m_follOuterPunct use fop
// BEW 3Mar11, changed at Bob Eaton's request from using \t (tab char) for indenting the
// xml elements, to using "  " (two spaces) for each tabUnit indent - to comply with 3-way
// merge in Chorus, to prevent the latter from sending the whole document file as the
// change set; same change made in KB.cpp for the indents for the KB's xml output)
CBString CSourcePhrase::MakeXML(int nTabLevel)
{
	CBString tabUnit = "  "; // BEW added 3Mar11
	// get the doc version number to be used for this save
	int docVersion = gpApp->GetDocument()->GetCurrentDocVersion();

	// nTabLevel specifies how many tabs are to start each line,
	// nTabLevel == 1 inserts one, 2 inserts two, etc
	CBString bstr;
	bstr.Empty();
	CBString btemp;
	int i;
	int len;
	int extraIndent;
	int nCount;
	wxString tempStr;
	SPList::Node* posSW;
	// wx note: the wx version in Unicode build refuses assign a CBString to char numStr[24]
	// so I'll declare numStr as a CBString also
	CBString numStr; //char numStr[24];

#ifdef _UNICODE

	switch (docVersion)
	{
	default:
	case 6: // BEW added 13Feb12 for docVersion6
	case 5:
		// first line -- element name and 4 attributes (two may be absent)
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += tabUnit; // tab the start of the line
		}

		bstr += "<S s=\"";
		btemp = gpApp->Convert16to8(m_srcPhrase); // get the source string
		InsertEntities(btemp);
		bstr += btemp; // add it
		bstr += "\" k=\"";
		btemp = gpApp->Convert16to8(m_key); // get the key string (lacks punctuation, but we can't rule out it 
					   // may contain " or > so we will need to do entity insert for these
		InsertEntities(btemp);
		bstr += btemp; // add it
		bstr += "\"";

		if (!m_targetStr.IsEmpty())
		{
			bstr += " t=\"";
			btemp = gpApp->Convert16to8(m_targetStr);
			InsertEntities(btemp);
			bstr += btemp;
			bstr += "\"";
		}

		if (!m_adaption.IsEmpty())
		{
			bstr += " a=\"";
			btemp = gpApp->Convert16to8(m_adaption);
			InsertEntities(btemp);
			bstr += btemp;
			bstr += "\"";
		}

		// second line -- 4 attributes (all obligatory), starting with the flags string
		bstr += "\r\n";
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += tabUnit; // tab the start of the line
		}
		btemp = MakeFlags(this);
		bstr += "f=\"";
		bstr += btemp; // add flags string
		bstr += "\" sn=\"";
		tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
		tempStr << m_nSequNumber;
		numStr = gpApp->Convert16to8(tempStr);
		bstr += numStr; // add m_nSequNumber string
		bstr += "\" w=\"";
		tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
		tempStr << m_nSrcWords;
		numStr = gpApp->Convert16to8(tempStr);
		bstr += numStr; // add m_nSrcWords string
		bstr += "\" ty=\"";
		tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
		tempStr << (int)m_curTextType;
		numStr = gpApp->Convert16to8(tempStr);
		bstr += numStr; // add m_curTextType string
		bstr += "\""; // delay adding newline, there may be no more content

		// third line -- 6 attributes, possibly all absent
		if (!m_precPunct.IsEmpty() || !m_follPunct.IsEmpty() || !m_gloss.IsEmpty()
			|| !m_inform.IsEmpty() || !m_chapterVerse.IsEmpty() || !m_follOuterPunct.IsEmpty())
		{
			// there is something on this line, so form the line
			bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
			bool bStarted = FALSE;
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			if (!m_precPunct.IsEmpty())
			{
				bstr += "pp=\"";
				btemp = gpApp->Convert16to8(m_precPunct);
				InsertEntities(btemp);
				bstr += btemp; // add m_precPunct string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_follPunct.IsEmpty())
			{
				if (bStarted)
					bstr += " fp=\"";
				else
					bstr += "fp=\"";
				btemp = gpApp->Convert16to8(m_follPunct);
				InsertEntities(btemp);
				bstr += btemp; // add m_follPunct string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_follOuterPunct.IsEmpty())
			{
				if (bStarted)
					bstr += " fop=\"";
				else
					bstr += "fop=\"";
				btemp = gpApp->Convert16to8(m_follOuterPunct);
				InsertEntities(btemp);
				bstr += btemp; // add m_follOuterPunct string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_gloss.IsEmpty())
			{
				if (bStarted)
					bstr += " g=\"";
				else
					bstr += "g=\"";
				btemp = gpApp->Convert16to8(m_gloss);
				InsertEntities(btemp);
				bstr += btemp;
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_inform.IsEmpty())
			{
				if (bStarted)
					bstr += " i=\"";
				else
					bstr += "i=\"";
				btemp = gpApp->Convert16to8(m_inform);
				InsertEntities(btemp); // just in case, do it for unicode (eliminates potential bug)
				bstr += btemp; // add m_inform string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_chapterVerse.IsEmpty())
			{
				if (bStarted)
					bstr += " c=\"";
				else
					bstr += "c=\"";
				btemp = gpApp->Convert16to8(m_chapterVerse);
				InsertEntities(btemp); // ditto
				bstr += btemp; // add m_chapterVerse string
				bstr += "\"";
			}
		}

        // fourth line -- 6 attributes in doc version 5, 1 in doc version 4, all possibly
        // absent, m_markers and, for vers 5, also m_endMarkers, m_inlineBindingMarkers,
        // m_inlineBindingEndMarkers, m_inlineNonbindingMarkers, and
        // m_inlineNonbindingEndMarkers; in vers 4 m_markers may be very long (eg. it may
		// contain filtered material), but in vers 5 it won't have filtered material
		// because (filtered info will be elsewhere), so em etc can be on same line as m;
		// entity conversions are not needed for standard format markers of any kind
		if (!m_markers.IsEmpty() || !m_endMarkers.IsEmpty() || !m_inlineBindingMarkers.IsEmpty()
			|| !m_inlineBindingEndMarkers.IsEmpty() || !m_inlineNonbindingMarkers.IsEmpty()
			|| !m_inlineNonbindingEndMarkers.IsEmpty())
		{
			// there is something on this line, so form the line
			bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
			bool bStarted = FALSE;
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			if (!m_markers.IsEmpty())
			{
				bstr += "m=\"";
				btemp = gpApp->Convert16to8(m_markers);
				bstr += btemp; // add m_markers string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_endMarkers.IsEmpty())
			{
				if (bStarted)
					bstr += " em=\"";
				else
					bstr += "em=\"";
				btemp = gpApp->Convert16to8(m_endMarkers);
				bstr += btemp; // add m_endMarkers string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_inlineBindingMarkers.IsEmpty())
			{
				if (bStarted)
					bstr += " iBM=\"";
				else
					bstr += "iBM=\"";
				btemp = gpApp->Convert16to8(m_inlineBindingMarkers);
				bstr += btemp; // add m_inlineBindingMarkers string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_inlineBindingEndMarkers.IsEmpty())
			{
				if (bStarted)
					bstr += " iBEM=\"";
				else
					bstr += "iBEM=\"";
				btemp = gpApp->Convert16to8(m_inlineBindingEndMarkers);
				bstr += btemp; // add m_inlineBindingEndMarkers string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_inlineNonbindingMarkers.IsEmpty())
			{
				if (bStarted)
					bstr += " iNM=\"";
				else
					bstr += "iNM=\"";
				btemp = gpApp->Convert16to8(m_inlineNonbindingMarkers);
				bstr += btemp; // add m_inlineNonbindingMarkers string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_inlineNonbindingEndMarkers.IsEmpty())
			{
				if (bStarted)
					bstr += " iNEM=\"";
				else
					bstr += "iNEM=\"";
				btemp = gpApp->Convert16to8(m_inlineNonbindingEndMarkers);
				bstr += btemp; // add m_inlineNonbindingMarkers string
				bstr += "\"";
			}
		}

		// fifth, sixth, seventh and eighth lines -- 1 attribute each, each is possibly absent
		if (!m_freeTrans.IsEmpty() || !m_note.IsEmpty() || !m_collectedBackTrans.IsEmpty()
			|| !m_filteredInfo.IsEmpty())
		{
			// there is something in this group, so form the needed lines
			bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
			bool bStarted = FALSE;
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			if (!m_freeTrans.IsEmpty())
			{
				bstr += "ft=\"";
				btemp = gpApp->Convert16to8(m_freeTrans);
				InsertEntities(btemp);
				bstr += btemp; // add m_freeTrans string
				bstr += "\"";
				bStarted = TRUE;
			}
			// sixth line... (possibly, or fifth)
			if (!m_note.IsEmpty())
			{
				if (bStarted)
				{
					bstr += "\r\n";
					for (i = 0; i < nTabLevel; i++)
					{
						bstr += tabUnit; // tab the start of the line
					}
					bStarted = FALSE; // reset, so logic will work for next line
				}
				bstr += "no=\"";
				btemp = gpApp->Convert16to8(m_note);
				InsertEntities(btemp);
				bstr += btemp; // add m_note string
				bstr += "\"";
				bStarted = TRUE;
			}
			// seventh line... (possibly, or sixth, or fifth)
			if (!m_collectedBackTrans.IsEmpty())
			{
				if (bStarted)
				{
					// we need to start a new line (the seventh or sixth)
					bstr += "\r\n";
					for (i = 0; i < nTabLevel; i++)
					{
						bstr += tabUnit; // tab the start of the line
					}
					bStarted = FALSE; // reset, so logic will work for any next lines
				}
				bstr += "bt=\"";
				btemp = gpApp->Convert16to8(m_collectedBackTrans);
				InsertEntities(btemp);
				bstr += btemp; // add m_collectedBackTrans string
				bstr += "\"";
				bStarted = TRUE; // uncomment out if we add more attributes to this block
			}
			// eighth line... (possibly, or seventh or sixth, or fifth)
			if (!m_filteredInfo.IsEmpty())
			{
				if (bStarted)
				{
					// we need to start a new line (the eigth or seventh or sixth)
					bstr += "\r\n";
					for (i = 0; i < nTabLevel; i++)
					{
						bstr += tabUnit; // tab the start of the line
					}
					bStarted = FALSE; // reset, so logic will work for any next lines
				}
				bstr += "fi=\"";
				btemp = gpApp->Convert16to8(m_filteredInfo);
				InsertEntities(btemp);
				bstr += btemp; // add m_filteredInfo string
				bstr += "\"";
				//bStarted = TRUE; // uncomment out if we add more attributes to this block
			}
		}

		// ninth line -- 4 attributes each is possibly absent
		// Supporting new docVersion6 storage strings (skip this block if docVersion is 5):
		// 	m_lastAdaptionsPattern, m_tgtMkrPattern, m_glossMkrPattern, m_punctsPattern
		if ( ( docVersion >= 6) && 
			 (!m_lastAdaptionsPattern.IsEmpty() || !m_tgtMkrPattern.IsEmpty() || 
			  !m_glossMkrPattern.IsEmpty() || !m_punctsPattern.IsEmpty())
		   )
		{
			// there is something in this group, so form the needed line
			bstr += "\r\n"; // TODO?: EOL chars may need to be changed under Linux and Mac
			bool bStarted = FALSE;
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			// first string in the 9th line, for  last m_adaptions pattern
			if (!m_lastAdaptionsPattern.IsEmpty())
			{
				bstr += "lapat=\"";
				btemp = gpApp->Convert16to8(m_lastAdaptionsPattern);
				InsertEntities(btemp);
				bstr += btemp; // add m_lastAdaptionsPattern string
				bstr += "\"";
				bStarted = TRUE;
			}
			// second string in the 9th line, for  target marker pattern
			if (!m_tgtMkrPattern.IsEmpty())
			{
				if (bStarted)
					bstr += " tmpat=\"";
				else
					bstr += "tmpat=\"";
				btemp = gpApp->Convert16to8(m_tgtMkrPattern);
				InsertEntities(btemp);
				bstr += btemp; // add m_tgtMkrPattern string
				bstr += "\"";
				bStarted = TRUE;
			}
			// third string in 9th line, or second, for gloss marker pattern
			if (!m_glossMkrPattern.IsEmpty())
			{
				if (bStarted)
					bstr += " gmpat=\"";
				else
					bstr += "gmpat=\"";
				btemp = gpApp->Convert16to8(m_glossMkrPattern);
				InsertEntities(btemp);
				bstr += btemp; // add m_glossMkrPattern string
				bstr += "\"";
				bStarted = TRUE;
			}
			// fourth string in 9th line... (possibly, or third or second), for puncts pattern
			if (!m_punctsPattern.IsEmpty())
			{
				if (bStarted)
					bstr += " pupat=\"";
				else
					bstr += "pupat=\"";
				btemp = gpApp->Convert16to8(m_punctsPattern);
				InsertEntities(btemp);
				bstr += btemp; // add m_punctsPattern string
				bstr += "\"";
				//bStarted = TRUE; // uncomment out if we add more attributes to this block
			}
		}
		// we can now close off the S opening tag, the closing </S> tag is added later below
		bstr += ">";

		// there are potentially up to three further information types - each or all may be
		// empty, and each, if present, is a list of one or more things - the first two are
		// string lists, and the last is embedded sourcephrase instances - these we will
		// handle by embedding the XML representations with an extra level of tabbed indent
		nCount = m_pMedialPuncts->GetCount();
		if (nCount > 0)
		{
			// handle the list of medial punctuation strings (these require entity insertion)
			// and there are seldom more than one or two of them, so all can go on one line
			// with no intervening spaces
			bstr += "\r\n";
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			for (i = 0; i < nCount; i++)
			{
				bstr += "<MP mp=\"";
				btemp = gpApp->Convert16to8(m_pMedialPuncts->Item(i)); 
				InsertEntities(btemp);
				bstr += btemp; // add punctuation string
				bstr += "\"/>";
			}
		}
		nCount = m_pMedialMarkers->GetCount();
		if (nCount > 0)
		{
			// handle the list of medial marker strings (these don't require entity insertion)
			// and there are seldom more than one or two of them, so all can go on one line
			// with no intervening spaces
			bstr += "\r\n";
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			for (i = 0; i < nCount; i++)
			{
				bstr += "<MM mm=\"";
				btemp = gpApp->Convert16to8(m_pMedialMarkers->Item(i));
				bstr += btemp; // add marker(s) string
				bstr += "\"/>";
			}
		}
		// pos above was a wxArrayString index position, but now we are dealing with a wxList structure
		// and will use posSW as a node of the m_pSavedWords SPList.
		nCount = m_pSavedWords->GetCount();
		extraIndent = nTabLevel + 1;
		if (nCount > 0)
		{
			// handle the list of medial marker strings (these don't require entity insertion)
			// and there are seldom more than one or two of them, so all can go on one line
			// with no intervening spaces
			CSourcePhrase* pSrcPhrase;
			bstr += "\r\n"; // get a new line open
			posSW = m_pSavedWords->GetFirst();
			while(posSW != NULL)
			{
				pSrcPhrase = (CSourcePhrase*)posSW->GetData();
				posSW = posSW->GetNext();
				bstr += pSrcPhrase->MakeXML(extraIndent);
			}
		}
		// now close off the element, at the original indentation level
		len = bstr.GetLength();
		if (bstr.GetAt(len-1) != '\n')
			bstr += "\r\n";
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += tabUnit; // tab the start of the line
		}
		bstr += "</S>\r\n";

		break; // for Unicode version, building for VERSION_NUMBER (currently 5)
	case 4:
	case 3:
	case 2:
	case 1:
		// first line -- element name and 4 attributes (two may be absent)
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += tabUnit; // tab the start of the line
		}

		bstr += "<S s=\"";
		btemp = gpApp->Convert16to8(m_srcPhrase); // get the source string
		InsertEntities(btemp);
		bstr += btemp; // add it
		bstr += "\" k=\"";
		btemp = gpApp->Convert16to8(m_key); // get the key string (lacks punctuation, but we can't rule out it 
					   // may contain " or > so we will need to do entity insert for these
		InsertEntities(btemp);
		bstr += btemp; // add it
		bstr += "\"";

		if (!m_targetStr.IsEmpty())
		{
			bstr += " t=\"";
			btemp = gpApp->Convert16to8(m_targetStr);
			InsertEntities(btemp);
			bstr += btemp;
			bstr += "\"";
		}

		if (!m_adaption.IsEmpty())
		{
			bstr += " a=\"";
			btemp = gpApp->Convert16to8(m_adaption);
			InsertEntities(btemp);
			bstr += btemp;
			bstr += "\"";
		}

		// second line -- 4 attributes (all obligatory), starting with the flags string
		bstr += "\r\n";
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += tabUnit; // tab the start of the line
		}
		btemp = MakeFlags(this);
		bstr += "f=\"";
		bstr += btemp; // add flags string
		bstr += "\" sn=\"";
		tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
		tempStr << m_nSequNumber;
		numStr = gpApp->Convert16to8(tempStr);
		bstr += numStr; // add m_nSequNumber string
		bstr += "\" w=\"";
		tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
		tempStr << m_nSrcWords;
		numStr = gpApp->Convert16to8(tempStr);
		bstr += numStr; // add m_nSrcWords string
		bstr += "\" ty=\"";
		tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
		tempStr << (int)m_curTextType;
		numStr = gpApp->Convert16to8(tempStr);
		bstr += numStr; // add m_curTextType string
		bstr += "\""; // delay adding newline, there may be no more content

		// third line -- 5 attributes, possibly all absent
		if (!m_precPunct.IsEmpty() || !m_follPunct.IsEmpty() || !m_gloss.IsEmpty()
			|| !m_inform.IsEmpty() || !m_chapterVerse.IsEmpty())
		{
			// there is something on this line, so form the line
			bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
			bool bStarted = FALSE;
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			if (!m_precPunct.IsEmpty())
			{
				bstr += "pp=\"";
				btemp = gpApp->Convert16to8(m_precPunct);
				InsertEntities(btemp);
				bstr += btemp; // add m_precPunct string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_follPunct.IsEmpty())
			{
				if (bStarted)
					bstr += " fp=\"";
				else
					bstr += "fp=\"";
				btemp = gpApp->Convert16to8(m_follPunct);
				InsertEntities(btemp);
				bstr += btemp; // add m_follPunct string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_gloss.IsEmpty())
			{
				if (bStarted)
					bstr += " g=\"";
				else
					bstr += "g=\"";
				btemp = gpApp->Convert16to8(m_gloss);
				InsertEntities(btemp);
				bstr += btemp;
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_inform.IsEmpty())
			{
				if (bStarted)
					bstr += " i=\"";
				else
					bstr += "i=\"";
				btemp = gpApp->Convert16to8(m_inform);
				InsertEntities(btemp); // just in case, do it for unicode (eliminates potential bug)
				bstr += btemp; // add m_inform string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_chapterVerse.IsEmpty())
			{
				if (bStarted)
					bstr += " c=\"";
				else
					bstr += "c=\"";
				btemp = gpApp->Convert16to8(m_chapterVerse);
				InsertEntities(btemp); // ditto
				bstr += btemp; // add m_chapterVerse string
				bstr += "\"";
			}
		}

		// fourth line -- 1 attribute, possibly absent, m_markers; it can have the whole line
		// because if present it may be very long (eg. it may contain filtered material)
		if (!m_markers.IsEmpty())
		{
			// there is something on this line, so form the line
			bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			bstr += "m=\"";
			btemp = gpApp->Convert16to8(m_markers);
			InsertEntities(btemp);
			bstr += btemp; // add m_markers string
			bstr += "\"";
		}

		// we can now close off the S opening tag
		bstr += ">";

		// there are potentially up to three further information types - each or all may be
		// empty, and each, if present, is a list of one or more things - the first two are
		// string lists, and the last is embedded sourcephrase instances - these we will
		// handle by embedding the XML representations with an extra level of tabbed indent
		nCount = m_pMedialPuncts->GetCount();
		if (nCount > 0)
		{
			// handle the list of medial punctuation strings (these require entity insertion)
			// and there are seldom more than one or two of them, so all can go on one line
			// with no intervening spaces
			bstr += "\r\n";
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			for (i = 0; i < nCount; i++)
			{
				bstr += "<MP mp=\"";
				btemp = gpApp->Convert16to8(m_pMedialPuncts->Item(i)); 
				InsertEntities(btemp);
				bstr += btemp; // add punctuation string
				bstr += "\"/>";
			}
		}
		nCount = m_pMedialMarkers->GetCount();
		if (nCount > 0)
		{
			// handle the list of medial marker strings (these don't require entity insertion)
			// and there are seldom more than one or two of them, so all can go on one line
			// with no intervening spaces
			bstr += "\r\n";
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			for (i = 0; i < nCount; i++)
			{
				bstr += "<MM mm=\"";
				btemp = gpApp->Convert16to8(m_pMedialMarkers->Item(i));
				bstr += btemp; // add marker(s) string
				bstr += "\"/>";
			}
		}
		// pos above was a wxArrayString index position, but now we are dealing with a wxList structure
		// and will use posSW as a node of the m_pSavedWords SPList.
		nCount = m_pSavedWords->GetCount();
		extraIndent = nTabLevel + 1;
		if (nCount > 0)
		{
			// handle the list of medial marker strings (these don't require entity insertion)
			// and there are seldom more than one or two of them, so all can go on one line
			// with no intervening spaces
			CSourcePhrase* pSrcPhrase;
			bstr += "\r\n"; // get a new line open
			posSW = m_pSavedWords->GetFirst();
			while(posSW != NULL)
			{
				pSrcPhrase = (CSourcePhrase*)posSW->GetData();
				posSW = posSW->GetNext();
				bstr += pSrcPhrase->MakeXML(extraIndent);
			}
		}
		// now close off the element, at the original indentation level
		len = bstr.GetLength();
		if (bstr.GetAt(len-1) != '\n')
			bstr += "\r\n";
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += tabUnit; // tab the start of the line
		}
		bstr += "</S>\r\n";

		break; // for Unicode version, building for DOCVERSION4 (currently 4) or earlier
	}
#else // regular version

	switch (docVersion)
	{
	default:
	case 5:
		// first line -- element name and 4 attributes (two may be absent)
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += tabUnit; // tab the start of the line
		}

		bstr += "<S s=\"";
		btemp = m_srcPhrase; // get the source string
		InsertEntities(btemp);
		bstr += btemp; // add it
		bstr += "\" k=\"";
		btemp = m_key; // get the key string (lacks punctuation, but we can't rule out it 
					   // may contain " or > so we will need to do entity insert for these
		InsertEntities(btemp);
		bstr += btemp; // add it
		bstr += "\"";

		if (!m_targetStr.IsEmpty())
		{
			bstr += " t=\"";
			btemp = m_targetStr;
			InsertEntities(btemp);
			bstr += btemp;
			bstr += "\"";
		}

		if (!m_adaption.IsEmpty())
		{
			bstr += " a=\"";
			btemp = m_adaption;
			InsertEntities(btemp);
			bstr += btemp;
			bstr += "\"";
		}

		// second line -- 4 attributes (all obligatory), starting with the flags string
		bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += tabUnit; // tab the start of the line
		}
		btemp = MakeFlags(this);
		bstr += "f=\"";
		bstr += btemp; // add flags string
		bstr += "\" sn=\"";
		tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
		tempStr << m_nSequNumber;
		numStr = tempStr;
		bstr += numStr; // add m_nSequNumber string
		bstr += "\" w=\"";
		tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
		tempStr << m_nSrcWords;
		numStr = tempStr;
		bstr += numStr; // add m_nSrcWords string
		bstr += "\" ty=\"";
		tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
		tempStr << (int)m_curTextType;
		numStr = tempStr;
		bstr += numStr; // add m_curTextType string
		bstr += "\""; // delay adding newline, there may be no more content

		// third line -- 5 attributes, possibly all absent
		if (!m_precPunct.IsEmpty() || !m_follPunct.IsEmpty() || !m_gloss.IsEmpty()
			|| !m_inform.IsEmpty() || !m_chapterVerse.IsEmpty())
		{
			// there is something on this line, so form the line
			bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
			bool bStarted = FALSE;
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			if (!m_precPunct.IsEmpty())
			{
				bstr += "pp=\"";
				btemp = m_precPunct;
				InsertEntities(btemp);
				bstr += btemp; // add m_precPunct string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_follPunct.IsEmpty())
			{
				if (bStarted)
					bstr += " fp=\"";
				else
					bstr += "fp=\"";
				btemp = m_follPunct;
				InsertEntities(btemp);
				bstr += btemp; // add m_follPunct string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_gloss.IsEmpty())
			{
				if (bStarted)
					bstr += " g=\"";
				else
					bstr += "g=\"";
				btemp = m_gloss;
				InsertEntities(btemp);
				bstr += btemp;
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_inform.IsEmpty())
			{
				if (bStarted)
					bstr += " i=\"";
				else
					bstr += "i=\"";
				//btemp = m_inform;
				//InsertEntities(btemp); // entities not needed for this member
				bstr += m_inform; // add m_inform string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_chapterVerse.IsEmpty())
			{
				if (bStarted)
					bstr += " c=\"";
				else
					bstr += "c=\"";
				//btemp = m_chapterVerse;
				//InsertEntities(btemp); // entities not needed for this member
				bstr += m_chapterVerse; // add m_chapterVerse string
				bstr += "\"";
			}
		}

		// fourth line -- 2 attributes in doc version 5, 1 in doc version 4, both possibly
		// absent, m_markers and, for vers 5, also m_endMarkers; in vers 4 it may be very long
		// (eg. it may contain filtered material), but in vers 5 it won't be (fildered info
		// will be elsewhere), so em can be on same line as m
		if (!m_markers.IsEmpty() || !m_endMarkers.IsEmpty())
		{
			// there is something on this line, so form the line
			bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
			bool bStarted = FALSE;
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			if (!m_markers.IsEmpty())
			{
				bstr += "m=\"";
				btemp = m_markers;
				bstr += btemp; // add m_markers string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_endMarkers.IsEmpty())
			{
				if (bStarted)
					bstr += " em=\"";
				else
					bstr += "em=\"";
				//btemp = m_endMarkers;
				//bstr += btemp; // add m_endMarkers string
				bstr += m_endMarkers; // this is quicker
				bstr += "\"";
			}
		}

		// fifth, sixth, seventh and eighth lines -- 1 attribute each, each is possibly absent
		if (!m_freeTrans.IsEmpty() || !m_note.IsEmpty() || !m_collectedBackTrans.IsEmpty()
			|| !m_filteredInfo.IsEmpty())
		{
			// there is something in this group, so form the needed lines
			bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
			bool bStarted = FALSE;
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			if (!m_freeTrans.IsEmpty())
			{
				bstr += "ft=\"";
				btemp = m_freeTrans;
				InsertEntities(btemp);
				bstr += btemp; // add m_freeTrans string
				bstr += "\"";
				bStarted = TRUE;
			}
			// sixth line... (possibly, or fifth)
			if (!m_note.IsEmpty())
			{
				if (bStarted)
				{
					// we need to start a new line (the sixth)
					bstr += "\r\n";
					for (i = 0; i < nTabLevel; i++)
					{
						bstr += tabUnit; // tab the start of the line
					}
					bStarted = FALSE; // reset, so logic will work for next line
				}
				bstr += "no=\"";
				btemp = m_note;
				InsertEntities(btemp);
				bstr += btemp; // add m_note string
				bstr += "\"";
				bStarted = TRUE;
			}
			// seventh line... (possibly, or sixth, or fifth)
			if (!m_collectedBackTrans.IsEmpty())
			{
				if (bStarted)
				{
					// we need to start a new line (the seventh or sixth)
					bstr += "\r\n";
					for (i = 0; i < nTabLevel; i++)
					{
						bstr += tabUnit; // tab the start of the line
					}
					bStarted = FALSE; // reset, so logic will work for any next lines
				}
				bstr += "bt=\"";
				btemp = m_collectedBackTrans;
				InsertEntities(btemp);
				bstr += btemp; // add m_collectedBackTrans string
				bstr += "\"";
				bStarted = TRUE; // uncomment out if we add more attributes to this block
			}
			// eighth line... (possibly, or seventh or sixth, or fifth)
			if (!m_filteredInfo.IsEmpty())
			{
				if (bStarted)
				{
					// we need to start a new line (the eigth or seventh or sixth)
					bstr += "\r\n";
					for (i = 0; i < nTabLevel; i++)
					{
						bstr += tabUnit; // tab the start of the line
					}
					bStarted = FALSE; // reset, so logic will work for any next lines
				}
				bstr += "fi=\"";
				btemp = m_filteredInfo;
				InsertEntities(btemp);
				bstr += btemp; // add m_filteredInfo string
				bstr += "\"";
				//bStarted = TRUE; // uncomment out if we add more attributes to this block
			}
		}

		// ninth line -- 4 attributes each is possibly absent
		// Supporting new docVersion6 storage strings (skip this block if docVersion is 5):
		// 	m_lastAdaptionsPattern, m_tgtMkrPattern, m_glossMkrPattern, m_punctsPattern
		if ( ( docVersion == 6) && 
			 (!m_lastAdaptionsPattern.IsEmpty() || !m_tgtMkrPattern.IsEmpty() || 
			  !m_glossMkrPattern.IsEmpty() || !m_punctsPattern.IsEmpty())
		   )
		{
			// there is something in this group, so form the needed line
			bstr += "\r\n"; // TODO?: EOL chars may need to be changed under Linux and Mac
			bool bStarted = FALSE;
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			// first string in the 9th line, for  last m_adaptions pattern
			if (!m_lastAdaptionsPattern.IsEmpty())
			{
				bstr += "lapat=\"";
				btemp = m_lastAdaptionsPattern;
				InsertEntities(btemp);
				bstr += btemp; // add m_lastAdaptionsPattern string
				bstr += "\"";
				bStarted = TRUE;
			}
			// second string in the 9th line, for  target marker pattern
			if (!m_tgtMkrPattern.IsEmpty())
			{
				if (bStarted)
					bstr += " tmpat=\"";
				else
					bstr += "tmpat=\"";
				btemp = m_tgtMkrPattern;
				InsertEntities(btemp);
				bstr += btemp; // add m_tgtMkrPattern string
				bstr += "\"";
				bStarted = TRUE;
			}
			// third string in 9th line, or second, for gloss marker pattern
			if (!m_glossMkrPattern.IsEmpty())
			{
				if (bStarted)
					bstr += " gmpat=\"";
				else
					bstr += "gmpat=\"";
				btemp = m_glossMkrPattern;
				InsertEntities(btemp);
				bstr += btemp; // add m_glossMkrPattern string
				bstr += "\"";
				bStarted = TRUE;
			}
			// fourth string in 9th line... (possibly, or third or second), for puncts pattern
			if (!m_punctsPattern.IsEmpty())
			{
				if (bStarted)
					bstr += " pupat=\"";
				else
					bstr += "pupat=\"";
				btemp = m_punctsPattern;
				InsertEntities(btemp);
				bstr += btemp; // add m_punctsPattern string
				bstr += "\"";
				//bStarted = TRUE; // uncomment out if we add more attributes to this block
			}
		}

		// we can now close off the S opening tag
		bstr += ">";

		// there are potentially up to three further information types - each or all may be
		// empty, and each, if present, is a list of one or more things - the first two are
		// string lists, and the last is embedded sourcephrase instances - these we will
		// handle by embedding the XML representations with an extra level of tabbed indent
		nCount = m_pMedialPuncts->GetCount();
		if (nCount > 0)
		{
			// handle the list of medial punctuation strings (these require entity insertion)
			// and there are seldom more than one or two of them, so all can go on one line
			// with no intervening spaces
			bstr += "\r\n";
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			for (i = 0; i < nCount; i++)
			{
				bstr += "<MP mp=\"";
				btemp = m_pMedialPuncts->Item(i);
				InsertEntities(btemp);
				bstr += btemp; // add punctuation string
				bstr += "\"/>";
			}
		}
		nCount = m_pMedialMarkers->GetCount();
		if (nCount > 0)
		{
			// handle the list of medial marker strings (these don't require entity insertion)
			// and there are seldom more than one or two of them, so all can go on one line
			// with no intervening spaces
			bstr += "\r\n";
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			for (i = 0; i < nCount; i++)
			{
				bstr += "<MM mm=\"";
				btemp = m_pMedialMarkers->Item(i);
				bstr += btemp; // add marker(s) string
				bstr += "\"/>";
			}
		}
		// pos above was a wxArrayString index position, but now we are dealing with a wxList structure
		// and will use posSW as a node of the m_pSavedWords SPList.
		nCount = m_pSavedWords->GetCount();
		extraIndent = nTabLevel + 1;
		if (nCount > 0)
		{
			// handle the list of embedded CSourcePhrase instances which comprised the
			// original selection which resulted in this current merged CSourcePhrase instance
			CSourcePhrase* pSrcPhrase;
			bstr += "\r\n"; // get a new line open
			posSW = m_pSavedWords->GetFirst();
			while (posSW != NULL)
			{
				pSrcPhrase = (CSourcePhrase*)posSW->GetData();
				posSW = posSW->GetNext();
				bstr += pSrcPhrase->MakeXML(extraIndent);
			}
		}
		// now close off the element, at the original indentation level
		len = bstr.GetLength();
		if (bstr.GetAt(len-1) != '\n')
			bstr += "\r\n";
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += tabUnit; // tab the start of the line
		}
		bstr += "</S>\r\n";

		break; // for ANSI version, building for VERSION_NUMBER (currently 5)

	case 4:
	case 3:
	case 2:
	case 1:
		// first line -- element name and 4 attributes (two may be absent)
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += tabUnit; // tab the start of the line
		}

		bstr += "<S s=\"";
		btemp = m_srcPhrase; // get the source string
		InsertEntities(btemp);
		bstr += btemp; // add it
		bstr += "\" k=\"";
		btemp = m_key; // get the key string (lacks punctuation, but we can't rule out it 
					   // may contain " or > so we will need to do entity insert for these
		InsertEntities(btemp);
		bstr += btemp; // add it
		bstr += "\"";

		if (!m_targetStr.IsEmpty())
		{
			bstr += " t=\"";
			btemp = m_targetStr;
			InsertEntities(btemp);
			bstr += btemp;
			bstr += "\"";
		}

		if (!m_adaption.IsEmpty())
		{
			bstr += " a=\"";
			btemp = m_adaption;
			InsertEntities(btemp);
			bstr += btemp;
			bstr += "\"";
		}

		// second line -- 4 attributes (all obligatory), starting with the flags string
		bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += tabUnit; // tab the start of the line
		}
		btemp = MakeFlags(this);
		bstr += "f=\"";
		bstr += btemp; // add flags string
		bstr += "\" sn=\"";
		tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
		tempStr << m_nSequNumber;
		numStr = tempStr;
		bstr += numStr; // add m_nSequNumber string
		bstr += "\" w=\"";
		tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
		tempStr << m_nSrcWords;
		numStr = tempStr;
		bstr += numStr; // add m_nSrcWords string
		bstr += "\" ty=\"";
		tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
		tempStr << (int)m_curTextType;
		numStr = tempStr;
		bstr += numStr; // add m_curTextType string
		bstr += "\""; // delay adding newline, there may be no more content

		// third line -- 5 attributes, possibly all absent
		if (!m_precPunct.IsEmpty() || !m_follPunct.IsEmpty() || !m_gloss.IsEmpty()
			|| !m_inform.IsEmpty() || !m_chapterVerse.IsEmpty())
		{
			// there is something on this line, so form the line
			bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
			bool bStarted = FALSE;
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			if (!m_precPunct.IsEmpty())
			{
				bstr += "pp=\"";
				btemp = m_precPunct;
				InsertEntities(btemp);
				bstr += btemp; // add m_precPunct string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_follPunct.IsEmpty())
			{
				if (bStarted)
					bstr += " fp=\"";
				else
					bstr += "fp=\"";
				btemp = m_follPunct;
				InsertEntities(btemp);
				bstr += btemp; // add m_follPunct string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_gloss.IsEmpty())
			{
				if (bStarted)
					bstr += " g=\"";
				else
					bstr += "g=\"";
				btemp = m_gloss;
				InsertEntities(btemp);
				bstr += btemp;
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_inform.IsEmpty())
			{
				if (bStarted)
					bstr += " i=\"";
				else
					bstr += "i=\"";
				//btemp = m_inform;
				//InsertEntities(btemp); // entities not needed for this member
				bstr += m_inform; // add m_inform string
				bstr += "\"";
				bStarted = TRUE;
			}
			if (!m_chapterVerse.IsEmpty())
			{
				if (bStarted)
					bstr += " c=\"";
				else
					bstr += "c=\"";
				//btemp = m_chapterVerse;
				//InsertEntities(btemp); // entities not needed for this member
				bstr += m_chapterVerse; // add m_chapterVerse string
				bstr += "\"";
			}
		}

		// fourth line -- 1 attribute, possibly absent, m_markers; it can have the whole line
		// because if present it may be very long (eg. it may contain filtered material)
		if (!m_markers.IsEmpty())
		{
			// there is something on this line, so form the line
			bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			bstr += "m=\"";
			btemp = m_markers;
			InsertEntities(btemp);
			bstr += btemp; // add m_markers string
			bstr += "\"";
		}

		// we can now close off the S opening tag
		bstr += ">";

		// there are potentially up to three further information types - each or all may be
		// empty, and each, if present, is a list of one or more things - the first two are
		// string lists, and the last is embedded sourcephrase instances - these we will
		// handle by embedding the XML representations with an extra level of tabbed indent
		nCount = m_pMedialPuncts->GetCount();
		if (nCount > 0)
		{
			// handle the list of medial punctuation strings (these require entity insertion)
			// and there are seldom more than one or two of them, so all can go on one line
			// with no intervening spaces
			bstr += "\r\n";
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			for (i = 0; i < nCount; i++)
			{
				bstr += "<MP mp=\"";
				btemp = m_pMedialPuncts->Item(i);
				InsertEntities(btemp);
				bstr += btemp; // add punctuation string
				bstr += "\"/>";
			}
		}
		nCount = m_pMedialMarkers->GetCount();
		if (nCount > 0)
		{
			// handle the list of medial marker strings (these don't require entity insertion)
			// and there are seldom more than one or two of them, so all can go on one line
			// with no intervening spaces
			bstr += "\r\n";
			for (i = 0; i < nTabLevel; i++)
			{
				bstr += tabUnit; // tab the start of the line
			}
			for (i = 0; i < nCount; i++)
			{
				bstr += "<MM mm=\"";
				btemp = m_pMedialMarkers->Item(i);
				bstr += btemp; // add marker(s) string
				bstr += "\"/>";
			}
		}
		// pos above was a wxArrayString index position, but now we are dealing with a wxList structure
		// and will use posSW as a node of the m_pSavedWords SPList.
		nCount = m_pSavedWords->GetCount();
		extraIndent = nTabLevel + 1;
		if (nCount > 0)
		{
			// handle the list of embedded CSourcePhrase instances which comprised the
			// original selection which resulted in this current merged CSourcePhrase instance
			CSourcePhrase* pSrcPhrase;
			bstr += "\r\n"; // get a new line open
			posSW = m_pSavedWords->GetFirst();
			while (posSW != NULL)
			{
				pSrcPhrase = (CSourcePhrase*)posSW->GetData();
				posSW = posSW->GetNext();
				bstr += pSrcPhrase->MakeXML(extraIndent);
			}
		}
		// now close off the element, at the original indentation level
		len = bstr.GetLength();
		if (bstr.GetAt(len-1) != '\n')
			bstr += "\r\n";
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += tabUnit; // tab the start of the line
		}
		bstr += "</S>\r\n";

		break; // for ANSI version, building for DOCVERSION4 (currently 4) or earlier
	}

#endif
	// a dummy comment, I just want to do a new commit to svn to add a further log message
	// I forgot just now (3Mar11)
	return bstr;
}

wxString CSourcePhrase::GetChapterNumberString()
{
	if (this->m_chapterVerse.Find(_T(':')) != -1) {
		return this->m_chapterVerse.Left(this->m_chapterVerse.Find(_T(':')));  // .Trim(); << don't need Trim
	} else {
		return _T("");
	}
}

// BEW added 04Nov05
bool CSourcePhrase::ChapterColonVerseStringIsNotEmpty()
{
	return !this->m_chapterVerse.IsEmpty();
}

// some getters and setters...

/* uncomment out when we make m_markers a private member
// the app never needs to get individual markers in m_markers, but just the whole lot as a
// single string, so this is the only getter needed
wxString CSourcePhrase::GetMarkers()
{
	return m_markers;
}
*/
/* we've not yet made m_markers private, uncomment out when we do
void CSourcePhrase::SetMarkers(wxString markers)
{
	m_markers = markers;
}
*/

wxString CSourcePhrase::GetFreeTrans()
{
	return m_freeTrans;
}
wxString CSourcePhrase::GetNote()
{
	return m_note;
}
wxString CSourcePhrase::GetCollectedBackTrans()
{
	return m_collectedBackTrans;
}
wxString CSourcePhrase::GetFilteredInfo()
{
	return m_filteredInfo;
}
// return TRUE if there is something filtered, FALSE if nothing is there; the three arrays
// work in parallel - for a given index, the returned marker in the first and second are
// the wrapping markers for the content; some filtered info may have no endmarker, in which
// case the pFilteredEndMarkers will have an empty string (or space) at the appropriate index.
// If bUseSpaceForEmpty is TRUE, any empty string for an endmarker is returned in the
// pFilteredEndMarkers array as a single space character, rather than as the empty string.
// Default is to leave the empty string for an endmarker as an empty string (ie. the
// default bUseSpaceForEmpty value is FALSE)
bool CSourcePhrase::GetFilteredInfoAsArrays(wxArrayString* pFilteredMarkers, 
											wxArrayString* pFilteredEndMarkers,
											wxArrayString* pFilteredContent,
											bool bUseSpaceForEmpty)
{
	wxString mkr = _T("");
	wxString content = _T("");
	wxString endMkr = _T("");
	pFilteredMarkers->Empty();
	pFilteredContent->Empty();
	pFilteredEndMarkers->Empty();
	int offsetToStart = 0;
	int offsetToEnd = 0;
	if (m_filteredInfo.IsEmpty())
		return FALSE;

	// get the each \~FILTER ... \~FILTER*  wrapped substring, each such contains one SF
	// marked up content string, of form \marker <content> \endMarker (but no < or > chars)
	wxString info = m_filteredInfo;
	offsetToStart = info.Find(filterMkr);
	offsetToEnd = info.Find(filterMkrEnd);
	while (offsetToStart != wxNOT_FOUND && offsetToEnd != wxNOT_FOUND && !info.IsEmpty())
	{
		offsetToStart += filterMkrLen + 1; // point at USFM or SFM, +1 is for its 
										   // trailing space
        // a wrapping \~FILTER* endmarker may have a preceding space, so use Trim() to
		// get rid of it; and also use Trim(FALSE) to remove any initial space just in
		// case there was more than one space after the \~FILTER marker
		int length = offsetToEnd - offsetToStart;
		wxString markersAndContent = info.Mid(offsetToStart,length);
		markersAndContent.Trim(FALSE); // trim at left hand end
		markersAndContent.Trim(); // trim at right hand end

		// we now have a string of the form:
        // \somemarker some textual content \somemarker* (endmarker may be absent, or \F,
        // or \fe, or a USFM marker of the type \xxx* and the space before the endmarker
        // is not likely to be present, but could be)
        
        // Before extracting the markers and content from markersAndContent, shorten the
        // original string's copy (i.e. info) to prepare for the next extraction
		info = info.Mid(offsetToEnd + filterMkrEndLen);

		// BEW added 8Dec10, a markup "?error?" (it's not really an error, but it is sure
		// incompatible with filtering of things like footnotes when punctuation follows
		// the final \f* marker - which is a legal USFM markup scenario) is that we can
		// come here with having had \~FILTER \f <footnote text content here, etc>
		// \f*<punct>\~FILTER*, so that after removal of the filter brackets and trimming
		// of the ends, we have content which ends with .... \f*<punct> where <punct> is
		// one or more characters of punctuation. This defeats the internal algorithm
		// within the ParseMarkersAndContent() call below -- it will do a reversal and
		// look for an endmarker which it expects, if present, to be at the reversed
		// string's start - and so expects the first character to be * (if that isn't the
		// case, it assumes the input was not reversed and expects an initial backslash
		// which won't be there - and the assert trips, and in the release build, who
		// knows what corruption will happen from then on). What to do?
        // We don't want to abandon the punctuation following \f*, so we move it to be
        // preceding the \f*. That is a reasonable temporary solution - it makes the
        // ParseMarkersAndContent() call below robust. It should happen seldom though -
        // only when filtering out something where punctuation is both sides of \f* or \fe*
        // or \x*, and how often will that happen?!) So, the next bit of code is a kludge
        // to make the data safe for the call lower down.
		//bool bCheckFurther = FALSE; // set but not used
		int fullLen = 0;
		int lastMkrLen = 0;
		wxString firstMkr;
		int offset = wxNOT_FOUND;
		int offset2 = wxNOT_FOUND;
		offset = markersAndContent.Find(gSFescapechar); // there has to always be a beginmarker
		wxASSERT(offset != wxNOT_FOUND);
		wxString lastMkr = GetLastMarker(markersAndContent); // we may have just found the
													// first and only marker, so beware
		offset2 = markersAndContent.Find(lastMkr); // it will certainly find a marker
		if (offset == offset2)
		{
			// first and last markers are one and the same, so there is no endmarker, and
			// we need go no further with this kludge code
		}
		else
		{
			// there is at least one marker beyond the first
			 //bCheckFurther = TRUE;
			 lastMkrLen = lastMkr.Len();
			if (gpApp->gCurrentSfmSet == PngOnly)
			{
				// the only possible endmarkers are \fe or \F, both are footnote endmarker
				// alternatives
				if (lastMkr == _T("\\fe") || lastMkr == _T("\\F"))
				{
					fullLen = markersAndContent.Len();
					if (offset2 + lastMkrLen < fullLen)
					{
						// there is some content after lastMkr that we want to move to be
						// before it - do it here
						wxString firstBit = markersAndContent.Left(offset2);
						wxString theRest = markersAndContent.Mid(offset2);
						wxString liesBeyond = theRest.Mid(lastMkrLen); // move this stuff
						liesBeyond.Trim(FALSE); // don't want the space which must follow \fe or \F
						markersAndContent = firstBit + liesBeyond + lastMkr; 
						markersAndContent += _T(' '); // the \F or \fe needs a following space
					}
				}
			}
			else // assume UsfmOnly
			{
				if (lastMkr[lastMkrLen - 1] == _T('*'))
				{
					// its a USFM endmarker, so look for content beyond it, and move it to
					// precede the endmarker if there is any
					wxString firstBit = markersAndContent.Left(offset2);
					wxString theRest = markersAndContent.Mid(offset2);
					wxString liesBeyond = theRest.Mid(lastMkrLen); // move this stuff
					liesBeyond.Trim(); // don't want final space (shouldn't be any anyway)
					markersAndContent = firstBit + liesBeyond + lastMkr; // now it's safe
				}
				// if it's not *, then assume the marker was within the string and ignore
				// it, we don't have an endmarker at the end
			}
		}
 
		// Now process markersAndContent to extract the marker and endmarker, and the
		// content string, adding them to the respective arrays; the ParseMarkersAndContent()
		// function handles initial marker and end marker, whether SFM set or USFM set,
		// equally well
		ParseMarkersAndContent(markersAndContent, mkr, content, endMkr); // defined in xml.cpp
		pFilteredMarkers->Add(mkr);
		pFilteredContent->Add(content);
		if (bUseSpaceForEmpty)
		{
			if (endMkr.IsEmpty())
				endMkr = _T(" ");  // a space
		}
		pFilteredEndMarkers->Add(endMkr);

		// prepare for next iteration
		offsetToStart = info.Find(filterMkr);
		offsetToEnd = info.Find(filterMkrEnd);
	}
	return TRUE;
}
wxString CSourcePhrase::GetEndMarkers()
{
	return m_endMarkers;
}

// next ones for the following four added by BEW 11Oct10
wxString CSourcePhrase::GetInlineBindingEndMarkers()
{
	return m_inlineBindingEndMarkers;
}

wxString CSourcePhrase::GetInlineNonbindingEndMarkers()
{
	return m_inlineNonbindingEndMarkers;
}

wxString CSourcePhrase::GetInlineBindingMarkers()
{
	return m_inlineBindingMarkers;
}

wxString CSourcePhrase::GetInlineNonbindingMarkers()
{
	return m_inlineNonbindingMarkers;
}

void CSourcePhrase::SetInlineBindingMarkers(wxString mkrStr)
{
	m_inlineBindingMarkers = mkrStr;
}

void CSourcePhrase::AppendToInlineBindingMarkers(wxString mkrStr)
{
	m_inlineBindingMarkers += mkrStr;
}

void CSourcePhrase::AppendToInlineBindingEndMarkers(wxString mkrStr)
{
	m_inlineBindingEndMarkers += mkrStr;
}

void CSourcePhrase::SetInlineNonbindingMarkers(wxString mkrStr)
{
	m_inlineNonbindingMarkers = mkrStr;
}

void CSourcePhrase::SetInlineBindingEndMarkers(wxString mkrStr)
{
	m_inlineBindingEndMarkers = mkrStr;
}

void CSourcePhrase::SetInlineNonbindingEndMarkers(wxString mkrStr)
{
	m_inlineNonbindingEndMarkers = mkrStr;
}

wxString CSourcePhrase::GetFollowingOuterPunct()
{
	return m_follOuterPunct;
}

void CSourcePhrase::SetFollowingOuterPunct(wxString punct)
{
	m_follOuterPunct = punct;
}


// return FALSE if empty, else TRUE (I'm not sure we'll ever need to get individual
// endmarkers, I think we only need to add the member contents to the document export at
// some point, but I'll provide this in case) 
// BEW 11Oct10, This function only gets from m_endMarkers; to get from m_endMarkers and
// also from m_inlineNonbindingEndMarkers and m_inlineBindingEndMarkers, use
// GetAllEndMarkersAsArray()
bool CSourcePhrase::GetEndMarkersAsArray(wxArrayString* pEndmarkersArray)
{
	pEndmarkersArray->Empty();
	bool bGotSome = FALSE;
	if (m_endMarkers.IsEmpty())
	{
		return FALSE;
	}
	else
	{
		bGotSome = GetSFMarkersAsArray(m_endMarkers, *pEndmarkersArray);
	}
	return bGotSome;
}

/* wrote this then found I didn't need it
// GetAllEndMarkersAsArray gets from all storage locations for endmarkers on a
// CSourcePhrase instance - there are 3 wxStrings it then must check and collect from.
bool CSourcePhrase::GetAllEndMarkersAsArray(wxArrayString* pEndmarkersArray)
{
	pEndmarkersArray->Empty();
	wxArrayString arr1;
	wxArrayString arr2;
	wxArrayString arr3;
	bool bGotSome = FALSE;
	bool bThisHasSome = FALSE;
	if (m_endMarkers.IsEmpty() && m_inlineNonbindingEndMarkers.IsEmpty()
		&& m_inlineBindingEndMarkers.IsEmpty())
	{
		return FALSE;
	}
	else
	{
		// try all
		bThisHasSome = GetSFMarkersAsArray(m_inlineBindingEndMarkers, arr1);
		if (bThisHasSome)
		{
			bGotSome = TRUE;
		}
		bThisHasSome = GetSFMarkersAsArray(m_inlineNonbindingEndMarkers, arr2);
		if (bThisHasSome)
		{
			bGotSome = TRUE;
		}
		bThisHasSome = GetSFMarkersAsArray(m_endMarkers, arr3);
		if (bThisHasSome)
		{
			bGotSome = TRUE;
		}
	}
	// put them all together into the one array; we don't care about the returned boolean
	// for these calls as we've already got bGotSome set; in these calls the 3rd param 
	// bExcludeDuplicates is default FALSE
	if (!arr1.IsEmpty())
		AddNewStringsToArray(pEndmarkersArray, &arr1);
	if (!arr2.IsEmpty())
		AddNewStringsToArray(pEndmarkersArray, &arr2);
	if (!arr3.IsEmpty())
		AddNewStringsToArray(pEndmarkersArray, &arr3);
	return bGotSome;
}
*/

void CSourcePhrase::SetFreeTrans(wxString freeTrans)
{
	m_freeTrans = freeTrans;
}
void CSourcePhrase::SetNote(wxString note)
{
	m_note = note;
}
void CSourcePhrase::SetCollectedBackTrans(wxString collectedBackTrans)
{
	m_collectedBackTrans = collectedBackTrans;
}
void CSourcePhrase::AddToFilteredInfo(wxString filteredInfo)
{
	// Use this for appending \~FILTER ... \~FILTER* wrapped filtered information into the
	// private m_filteredInfo member string; no delimiting space is to be used between
	// each such substring. The TokenizeText() parser will use this setter function, as
	// will the other setter, SetFilteredInfoFromArrays()
	m_filteredInfo += filteredInfo;
}

// BEW changed 5Mar10, to support changing a space in the pFilteredEndMarkers .Item()
// into an empty string; default is not to attempt the change (ie. assume the array will
// have only an empty endmarker string rather than a placeholding space; it is the 
// View Filtered Information dialog which uses a placeholding space)
// BEW changed 7Dec10, to rectify a logic error which resulted in endmarker and \~FILTER*
// not being added to the end of the content string
void CSourcePhrase::SetFilteredInfoFromArrays(wxArrayString* pFilteredMarkers, 
					wxArrayString* pFilteredEndMarkers, wxArrayString* pFilteredContent,
					bool bChangeSpaceToEmpty)
{
	m_filteredInfo.Empty(); // clear out old contents
	size_t index;
	size_t count = pFilteredContent->GetCount();
	if (count == 0)
	{
		return;
	}
	wxString markersAndContent;
	markersAndContent.Empty();
	wxString theRest;
	theRest.Empty();
	wxString aSpace = _T(" ");
	for (index = 0; index < count; index++)
	{
		markersAndContent << filterMkr << aSpace << pFilteredMarkers->Item(index) \
			<< aSpace << pFilteredContent->Item(index);
        // if there is an endmarker, insert it, but with no space before it - a space is
        // allowed, but in some publication scenarios punctuation might be wanted hard up
        // against the end of a preceding word and followed after the endmarker with a
        // textual note callout character -- so it's safer to not have any space before the
        // endmarker, since parsers recognise a marker when the backslash is encountered,
        // and LSDev, Paratext and Adapt It all accept the * of an endmarker as closing off
        // an endmarker. What we won't support is a callout character positioned
        // immediately after \fe or \F footnote end markers used in the old PNG SFM marker
        // set - for these callout character or punctuation would be treated as part of the
        // marker definition - leading to an "unknown marker" for Adapt It to handle. If
        // that happened, the user would have to edit the source text (in Adapt It, using
        // the Edit / Edit Source Text... command) and add the need post-endmarker space
        // manually. However, we are a decade or more from the last use of the old 1998 PNG
        // SFM marker set, everyone uses USFM these days, so we won't bother to do anything
        // smart here to handle such a contingency within our code.
		if (pFilteredEndMarkers->Item(index).IsEmpty())
		{
			// this content string takes no endMarker (add a delimiting space though)
			theRest << aSpace << filterMkrEnd;
		}
		else
		{
			// there is something, it could be an endmarker, or a placeholding space
			if (pFilteredEndMarkers->Item(index) == aSpace)
			{
				// this content string takes no endMarker (but a delimiting space after
				// the content string is probably a good idea)
				if (bChangeSpaceToEmpty)
				{
					theRest << filterMkrEnd;
				}
				else
				{
					theRest << aSpace << filterMkrEnd;
				}
			}
			else
			{
				// this content string takes an endMarker and there is one ready for
				// appending
				theRest << pFilteredEndMarkers->Item(index) << aSpace << filterMkrEnd;
			}
		}
		markersAndContent += theRest;
		AddToFilteredInfo(markersAndContent);
		markersAndContent.Empty();
		theRest.Empty();
	}
}

void CSourcePhrase::AddFollOuterPuncts(wxString outers)
{
	if (m_follOuterPunct.IsEmpty())
	{
		m_follOuterPunct = outers;
	}
	else
	{
		m_follOuterPunct += outers;
	}
}

void CSourcePhrase::AddEndMarker(wxString endMarker)
{
	if (gpApp->gCurrentSfmSet == PngOnly)
	{
		// for this marker set, the only endmarker is the footnote marker, officially \fe,
		// but some branches (eg. PNG) used \F as an alternative. Since these have no
		// final asterisk, we can't rely on an asterisk being present at the marker's end,
		// and so we'll want to have a following space. But for USFM, which has all
		// endmarkers with a final * character, we'll omit storing a delimiting space
		// between consecutive endmarkers, because this gives the best possible results if
		// additional textual apparatus (such as callouts like a, b, c superscripts, or
		// numbers) are used in the USFM markup at the end of the section of text where
		// the endMarkers are - such as at the end of a footnote
		if (m_endMarkers.IsEmpty())
		{
			m_endMarkers = endMarker;
		}
		else
		{
			m_endMarkers += _T(" ") + endMarker;
		}
		// store a final space too, for this marker set
		m_endMarkers.Trim();
		m_endMarkers += _T(' ');
	}
	else
	{
		// its USFM set (or combined USFM and PNG, but we treat this as USFM)
		m_endMarkers += endMarker;
		// don't add a space after endmarkers in USFM, it's likely to mess with
		// punctuation in unhelpful ways
	}
}

void CSourcePhrase::SetEndMarkers(wxString endMarkers)
{
	m_endMarkers = endMarkers;
}
void CSourcePhrase::SetFilteredInfo(wxString filteredInfo)
{
	m_filteredInfo = filteredInfo;
}

// BEW 11Oct10, updated for support of additional doc version 5 changes
void CSourcePhrase::SetEndMarkersAsNowMedial(wxArrayString* pMedialsArray)
{
	bool bAddedSomething = FALSE;

	// make a string array to hold it or them, if no string array exists yet
	if (pMedialsArray == NULL)
		pMedialsArray = new wxArrayString;
	wxASSERT(pMedialsArray != NULL);

	// try ordering them in the most likely order that their appearance in a list would be
	// most helpful to the user when placing them; keep the multiple occurrences in a
	// single storage location together, so that they appear on one line in the placement
	// dialog and then the user will realize they are to be stored together when placed
	wxString oldEndMkrs1 = GetInlineBindingEndMarkers();
	wxString oldEndMkrs2 = GetEndMarkers();
	wxString oldEndMkrs3 = GetInlineNonbindingEndMarkers();
	m_bHasInternalMarkers = TRUE;

	wxArrayString oldsArray;
	if (!oldEndMkrs1.IsEmpty())
	{
		oldsArray.Add(oldEndMkrs1);
	}
	if (!oldEndMkrs2.IsEmpty())
	{
		oldsArray.Add(oldEndMkrs2);
	}
	if (!oldEndMkrs3.IsEmpty())
	{
		oldsArray.Add(oldEndMkrs3);
	}
	// add to m_pMedials, from 11Oct10 onwards, we are quite happy to have duplicates in
	// the list - if they are there, they are there because they are needed and should be
	// placed by the user if asked (i.e. he's shown the Place Internal Markers dialog)
	// (param 3, bool bExcludeDuplicates is default FALSE)
	bAddedSomething = AddNewStringsToArray(pMedialsArray, &oldsArray);
	bAddedSomething = bAddedSomething; // avoid warning (retain, as is)
}
