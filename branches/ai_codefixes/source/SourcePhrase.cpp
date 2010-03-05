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
// Pending Implementation Items (in order of importance): (search for "TODO")
// 1. NONE. Current with MFC version 3.2.2
//
// Unanswered questions: (search for "???")
// 
// 
// 
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

#ifdef _DOCVER5
extern const wxChar* filterMkr;
extern const wxChar* filterMkrEnd;
const int filterMkrLen = 8;
const int filterMkrEndLen = 9;
#endif

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

CSourcePhrase::CSourcePhrase()
{
	m_chapterVerse = _T("");
	m_bFootnoteEnd = FALSE;
	m_bFootnote = FALSE;
	m_bChapter = FALSE;
	m_bVerse = FALSE;
	m_bParagraph = FALSE;
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

	// defaults for schema 4 (free translations, notes, bookmarks added)
	m_bHasFreeTrans = FALSE;
	m_bStartFreeTrans = FALSE;
	m_bEndFreeTrans = FALSE;
	m_bHasNote = FALSE;
	m_bHasBookmark = FALSE;

#ifdef _DOCVER5
	m_endMarkers = _T("");
	m_freeTrans = _T("");
	m_note = _T("");
	m_collectedBackTrans = _T("");
	m_filteredInfo = _T("");
#endif

	// create the stored lists, so that serialization won't crash if one is unused
	m_pSavedWords = new SPList;
	wxASSERT(m_pSavedWords != NULL);
	m_pMedialMarkers = new wxArrayString;
	wxASSERT(m_pMedialMarkers != NULL);
	m_pMedialPuncts = new wxArrayString;
	wxASSERT(m_pMedialPuncts != NULL);
}

CSourcePhrase::~CSourcePhrase()
{

}

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
	m_bParagraph = sp.m_bParagraph;
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
	m_bHasBookmark = sp.m_bHasBookmark;

	// the doc version 5 new members
	#ifdef _DOCVER5
	m_endMarkers = sp.m_endMarkers;
	m_freeTrans = sp.m_freeTrans;
	m_note = sp.m_note;
	m_collectedBackTrans = sp.m_collectedBackTrans;
	m_filteredInfo = sp.m_filteredInfo;
	#endif

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
}

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
	m_bParagraph = sp.m_bParagraph;
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
	m_bHasBookmark = sp.m_bHasBookmark;

	// the doc version 5 new members
	#ifdef _DOCVER5
	m_endMarkers = sp.m_endMarkers;
	m_freeTrans = sp.m_freeTrans;
	m_note = sp.m_note;
	m_collectedBackTrans = sp.m_collectedBackTrans;
	m_filteredInfo = sp.m_filteredInfo;
	#endif

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
	return *this;
}

// BEW added 16Apr08, to obtain copies of any saved original
// CSourcePhrases from a merger, and have pointers to the copies
// replace the pointers in the m_pSavedWords member of a new instance
// of CSourcePhrase produced with the copy constructor or operator=
// Usage: for example: 
// CSourcePhrase oldSP; ....more application code defining oldSP contents....
// CSourcePhrase* pNewSP = new CSourcePhrase(oldSP); // uses operator=
//			pNewSP.DeepCopy(); // *pNewSP is now a deep copy of oldSP
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
		m_pSavedWords = new SPList; // previously was wxList; // get a new list if there isn't one yet
		wxASSERT(m_pSavedWords != NULL);
	}

	if (m_pSavedWords->GetCount() == 0)
	{
		// save the current src phrase in it as its first element, if list is empty
		CSourcePhrase* pSP = new CSourcePhrase(*this); // use copy constructor
		wxASSERT(pSP != NULL);
		m_pSavedWords->Append(pSP); // adding single item
	}

	m_pSavedWords->Append(pSrcPhrase); // next store the pointer to the srcPhrase being merged
										// saving single item

	m_chapterVerse += pSrcPhrase->m_chapterVerse; // append ch:vs if it exists

	// if there is a marker, it will become phrase-internal, so accumulate it and set
	// the flag
	if (!pSrcPhrase->m_markers.IsEmpty())
	{
		// the marker will end up medial - so accumulate it, etc.
		m_bHasInternalMarkers = TRUE;

		// make a string list to hold it, if none exists yet
		if (m_pMedialMarkers == NULL)
			m_pMedialMarkers = new wxArrayString;
		wxASSERT(m_pMedialMarkers != NULL);

		// accumulate it
		m_pMedialMarkers->Add(pSrcPhrase->m_markers);
	}

	// for doc version 5, we'll assume any endmarkers on pSrcPhrase are not medial; but if
	// they are present at a later iteration on the CSourcePhrase which is pointed at by
	// this, then they are medial and have to be later 'placed'
#ifdef _DOCVER5
	bool bAddedSomething = FALSE;
	if (!pSrcPhrase->m_endMarkers.IsEmpty())
	{
		if (m_endMarkers.IsEmpty())
		{
			// not medial (or at least, not medial yet) so just copy it
			SetEndMarkers(pSrcPhrase->GetEndMarkers());
		}
		else
		{
			// endMarkers already on 'this' pointer, so these are medial now
			wxString oldEndMkrs = GetEndMarkers();

			// deal with the endmarkers that have just become 'old' ie. medial to the phrase
			// (the following call tests for m_pMedialMarkers = NULL, and if so creates a
			// string array for it on the heap, so the use of m_pMedialMarkers further below 
			// is safe)
			SetEndMarkersAsNowMedial(m_pMedialMarkers); // sets m_bHasInternalMarkers = TRUE;
			
			// now deal with the ones in pSrcPhrase; by this stage, m_pMedialMarkers exists
			wxArrayString newEndMarkersArray;
			pSrcPhrase->GetEndMarkersAsArray(&newEndMarkersArray);

			// add any not already in the array
			bAddedSomething = AddNewStringsToArray(m_pMedialMarkers, &newEndMarkersArray);

			// update the merged CSourcePhrase's m_endMarkers member
			wxString endMkrs = pSrcPhrase->GetEndMarkers();
			oldEndMkrs +=endMkrs;
			SetEndMarkers(oldEndMkrs);
		}
	}
	else if (!this->m_endMarkers.IsEmpty())
	{
		// these endmarkers have become medial now, but pSrcPhrase has none to contribute;
		// here we have the possibility that endmarkers have just been made non-final, and
		// so have become internal to the phrase, so we must update the medial markers array
		SetEndMarkersAsNowMedial(m_pMedialMarkers);
	}

#endif

	// if there is punctuation, some or all may become phrase-internal, so check it out and
	// accumulate as necessary and then set the flag if there is internal punctuation
	if (!m_follPunct.IsEmpty())
	{
		// the current srcPhrase has following punctuation, so adding a further srcPhrase will
		// make that punctuation become phrase-internal, so set up accordingly
		m_bHasInternalPunct = TRUE;

		// create a list, if one does not yet exist
		if (m_pMedialPuncts == NULL)
			m_pMedialPuncts = new wxArrayString;
		wxASSERT(m_pMedialPuncts != NULL);

		// put the follPunct string into the list & clear the attribute
		m_pMedialPuncts->Add(m_follPunct);
		m_follPunct = _T("");
	}

	if (!pSrcPhrase->m_precPunct.IsEmpty())
	{
		// the preceding punctuation will end up medial - so accumulate it, etc.
		m_bHasInternalPunct = TRUE;

		// make a string list to hold it, if none exists yet
		if (m_pMedialPuncts == NULL)
			m_pMedialPuncts = new wxArrayString;
		wxASSERT(m_pMedialPuncts != NULL);

		// accumulate it
		m_pMedialPuncts->Add(pSrcPhrase->m_precPunct);
	}

	// any final punctuation on the merged srcPhrase will be non-medial, so just copy it
	if (!pSrcPhrase->m_follPunct.IsEmpty())
		m_follPunct = pSrcPhrase->m_follPunct;

	// append the source phrase 
	// do I do it in reverse order for RTL layout? - I don't think so; but in case I should
	// I will code it and comment it out. I think I have to do the appending in logical order,
	// and for text which is RTL, the resulting phrase will be laid out RTL in the CEdit
	// For version 3, allow for empty strings; but m_srcPhrase cannot be empty so we
	// don't need a test here, but we do for the key
	m_srcPhrase = m_srcPhrase + _T(" ") + pSrcPhrase->m_srcPhrase; 

	// ditto for the key string, for version 3 allow for empty strings
	if (m_key.IsEmpty())
	{
		m_key = pSrcPhrase->m_key;
	}
	else
	{
		m_key = m_key + _T(" ") + pSrcPhrase->m_key;
	}

	// do the same for the m_adaption and m_targetStr fields
	if (m_adaption.IsEmpty())
		m_adaption = pSrcPhrase->m_adaption;
	else
	{
			m_adaption = m_adaption + _T(" ") + pSrcPhrase->m_adaption;
	}

	if (m_targetStr.IsEmpty())
		m_targetStr = pSrcPhrase->m_targetStr;
	else
	{
			m_targetStr = m_targetStr + _T(" ") + pSrcPhrase->m_targetStr;
	}

	// likewise for the m_gloss field in VERSION_NUMBER == 3
	if (m_gloss.IsEmpty())
		m_gloss = pSrcPhrase->m_gloss;
	else
	{
			m_gloss = m_gloss + _T(" ") + pSrcPhrase->m_gloss;
	}

	// doc version 5, these new members (an additional one is above)
	#ifdef _DOCVER5
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
	#endif


	// increment the number of words in the phrase
	m_nSrcWords += pSrcPhrase->m_nSrcWords; // do it this way, which will be correct if it is a
											// null source phrase being merged

	// if we are merging a bounding srcPhrase, then the phrase being constructed is also 
	// a boundary
	if (pSrcPhrase->m_bBoundary)
		m_bBoundary = TRUE;
	// BEW removed else branch, 10Aut06, because if the one with the boundary is not last, then
	// the last merger in the set will remove the m_bBoundary's true setting
	//else
	//	m_bBoundary = FALSE;

	// copy across the BOOL values for free translation, note, and/or bookmark, for versionable_schema 4
	// (1) if either instance has a bookmark, then the merger must have one too (BEW added 22Jul05)
	if (!m_bHasBookmark)
	{
		if (pSrcPhrase->m_bHasBookmark)
			m_bHasBookmark = TRUE;
	}
	// (2) if the first has a note, then so must the merger - we don't need to do anything because the
	// merger process will do it automatically because m_bHasNote was TRUE on the first in the merger
	// and merging across filtered material (of which note is an example) is not permitted
	// (3) handling m_bStartFreeTrans is done automatically by the merging protocol, it's m_bEndFreeTrans
	// that we must make sure it properly dealt with
	if (pSrcPhrase->m_bHasFreeTrans)
		m_bHasFreeTrans = TRUE;
	if (pSrcPhrase->m_bEndFreeTrans)
		m_bEndFreeTrans = TRUE;

	// we never merge phrases, only minimal phrases (ie. single source word objects), so it
	// will never be the case that we need to copy from m_pSaveWords in the Merge function
	// and similarly for the m_pMedialPuncts and m_pMedialMarkers lists.
	return TRUE;
}

CBString CSourcePhrase::MakeXML(int nTabLevel)
{
	// nTabLevel specifies how many tabs are to start each line,
	// nTabLevel == 1 inserts one, 2 inserts two, etc
	CBString bstr;
	bstr.Empty();
	CBString btemp;
	int i;
	wxString tempStr;
	// wx note: the wx version in Unicode build refuses assign a CBString to char numStr[24]
	// so I'll declare numStr as a CBString also
	CBString numStr; //char numStr[24];
#ifdef _UNICODE

	// first line -- element name and 4 attributes (two may be absent)
	for (i = 0; i < nTabLevel; i++)
	{
		bstr += "\t"; // tab the start of the line
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
		bstr += "\t"; // tab the start of the line
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
			bstr += "\t"; // tab the start of the line
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

    // fourth line -- 2 attributes in doc version 5, 1 in doc version 4, both possibly
    // absent, m_markers and, for vers 5, also m_endMarkers; in vers 4 it may be very long
	// (eg. it may contain filtered material), but in vers 5 it won't be (fildered info
	// will be elsewhere), so em can be on same line as m
#ifdef _DOCVER5
	if (!m_markers.IsEmpty() || !m_endMarkers.IsEmpty())
#else
	if (!m_markers.IsEmpty())
#endif
	{
		// there is something on this line, so form the line
		bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
		bool bStarted = FALSE;
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += "\t"; // tab the start of the line
		}
		if (!m_markers.IsEmpty())
		{
			bstr += "m=\"";
			btemp = gpApp->Convert16to8(m_markers);
#ifndef _DOCVER5
			InsertEntities(btemp); // not needed for version 5
#endif
			bstr += btemp; // add m_markers string
			bstr += "\"";
			bStarted = TRUE;
		}
#ifdef _DOCVER5
		if (!m_endMarkers.IsEmpty())
		{
			if (bStarted)
				bstr += " em=\"";
			else
				bstr += "em=\"";
			btemp = gpApp->Convert16to8(m_endMarkers);
			bstr += btemp; // add m_endMarkers string
			bstr += "\"";
		}
#endif
	}

#ifdef _DOCVER5
	// fifth, sixth, seventh and eighth lines -- 1 attribute each, each is possibly absent
	if (!m_freeTrans.IsEmpty() || !m_note.IsEmpty() || !m_collectedBackTrans.IsEmpty()
		|| !m_filteredInfo.IsEmpty())
	{
		// there is something in this group, so form the needed lines
		bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
		bool bStarted = FALSE;
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += "\t"; // tab the start of the line
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
				// we need to start a new line (the sixth)
				bstr += "\r\n";
				for (i = 0; i < nTabLevel; i++)
				{
					bstr += "\t"; // tab the start of the line
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
					bstr += "\t"; // tab the start of the line
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
					bstr += "\t"; // tab the start of the line
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
#endif

	// we can now close off the S opening tag
	bstr += ">";

	// there are potentially up to three further information types - each or all may be
	// empty, and each, if present, is a list of one or more things - the first two are
	// string lists, and the last is embedded sourcephrase instances - these we will
	// handle by embedding the XML representations with an extra level of tabbed indent
	int nCount = m_pMedialPuncts->GetCount();
	if (nCount > 0)
	{
		// handle the list of medial punctuation strings (these require entity insertion)
		// and there are seldom more than one or two of them, so all can go on one line
		// with no intervening spaces
		bstr += "\r\n";
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += "\t"; // tab the start of the line
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
			bstr += "\t"; // tab the start of the line
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
	SPList::Node* posSW;
	nCount = m_pSavedWords->GetCount();
	int extraIndent = nTabLevel + 1;
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
	int len = bstr.GetLength();
	if (bstr.GetAt(len-1) != '\n')
		bstr += "\r\n";
	for (i = 0; i < nTabLevel; i++)
	{
		bstr += "\t"; // tab the start of the line
	}
	bstr += "</S>\r\n";

#else // regular version

	// first line -- element name and 4 attributes (two may be absent)
	for (i = 0; i < nTabLevel; i++)
	{
		bstr += "\t"; // tab the start of the line
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
		bstr += "\t"; // tab the start of the line
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
			bstr += "\t"; // tab the start of the line
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
#ifdef _DOCVER5
	if (!m_markers.IsEmpty() || !m_endMarkers.IsEmpty())
#else
	if (!m_markers.IsEmpty())
#endif
	{
		// there is something on this line, so form the line
		bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
		bool bStarted = FALSE;
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += "\t"; // tab the start of the line
		}
		if (!m_markers.IsEmpty())
		{
			bstr += "m=\"";
			btemp = m_markers;
#ifndef _DOCVER5
			InsertEntities(btemp); // not needed for version 5
#endif
			bstr += btemp; // add m_markers string
			bstr += "\"";
			bStarted = TRUE;
		}
#ifdef _DOCVER5
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
#endif
	}

#ifdef _DOCVER5
	// fifth, sixth, seventh and eighth lines -- 1 attribute each, each is possibly absent
	if (!m_freeTrans.IsEmpty() || !m_note.IsEmpty() || !m_collectedBackTrans.IsEmpty()
		|| !m_filteredInfo.IsEmpty())
	{
		// there is something in this group, so form the needed lines
		bstr += "\r\n"; // TODO: EOL chars probably needs to be changed under Linux and Mac
		bool bStarted = FALSE;
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += "\t"; // tab the start of the line
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
					bstr += "\t"; // tab the start of the line
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
					bstr += "\t"; // tab the start of the line
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
					bstr += "\t"; // tab the start of the line
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
#endif

	// we can now close off the S opening tag
	bstr += ">";

	// there are potentially up to three further information types - each or all may be
	// empty, and each, if present, is a list of one or more things - the first two are
	// string lists, and the last is embedded sourcephrase instances - these we will
	// handle by embedding the XML representations with an extra level of tabbed indent
	int nCount = m_pMedialPuncts->GetCount();
	if (nCount > 0)
	{
		// handle the list of medial punctuation strings (these require entity insertion)
		// and there are seldom more than one or two of them, so all can go on one line
		// with no intervening spaces
		bstr += "\r\n";
		for (i = 0; i < nTabLevel; i++)
		{
			bstr += "\t"; // tab the start of the line
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
			bstr += "\t"; // tab the start of the line
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
	SPList::Node* posSW;
	nCount = m_pSavedWords->GetCount();
	int extraIndent = nTabLevel + 1;
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
	int len = bstr.GetLength();
	if (bstr.GetAt(len-1) != '\n')
		bstr += "\r\n";
	for (i = 0; i < nTabLevel; i++)
	{
		bstr += "\t"; // tab the start of the line
	}
	bstr += "</S>\r\n";
#endif
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
#ifdef _DOCVER5

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
	while (offsetToStart != wxNOT_FOUND && offsetToStart != wxNOT_FOUND && !info.IsEmpty())
	{
		offsetToStart += filterMkrLen + 1; // point at USFM or SFM, +1 is for its 
										   // trailing space
        // a wrapping \~FILTER* endmarker always has a preceding space, so use Trim() to
        // get rid of it
		int length = offsetToEnd - offsetToStart;
		wxString markersAndContent = info.Mid(offsetToStart,length);
		markersAndContent.Trim(); // trim at right hand end

		// we now have a string of the form:
        // \somemarker some textual content \somemarker* (endmarker may be absent, or \F,
        // or \fe, or a USFM marker of the type \xxx* and the space before the endmarker
        // may not be present)
 
        // Before extracting the markers and content from markersAndContent, shorten the
        // original string's copy (i.e. info) to prepare for the next extraction
		info = info.Mid(offsetToEnd + filterMkrEndLen);

		// Now process markersAndContent to extract the marker and endmarker, and the
		// content string, adding them to the respective arrays; the ParseMarkersAndContent()
		// function handles initial marker and end marker, whether SFM set or USFM set,
		// equally well
		ParseMarkersAndContent(markersAndContent, mkr, content, endMkr); // defined in helpers.cpp
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
// return FALSE if empty, else TRUE (I'm not sure we'll ever need to get individual
// endmarkers, I think we only need to add the member contents to the document export at
// some point, but I'll provide this in case)
bool CSourcePhrase::GetEndMarkersAsArray(wxArrayString* pEndmarkersArray)
{
	pEndmarkersArray->Empty();
	if (m_endMarkers.IsEmpty())
	{
		return FALSE;
	}
	else
	{
		wxString delimiter = _T(" ");
		// bStoreEmptyStringsToo is FALSE in next call 
		long count = SmartTokenize(delimiter, m_endMarkers, *pEndmarkersArray, FALSE);
		count = count; // avoid compiler warning
	}
	return TRUE;
}
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
        if (bChangeSpaceToEmpty)
		{
			// caller wants a space changed to the empty string
			if (pFilteredEndMarkers->Item(index) == aSpace)
			{
				// this content string takes no endMarker (but a delimiting space after
				// the content string is probably a good idea)
				theRest << aSpace << filterMkrEnd;
			}
		}
		else if (pFilteredEndMarkers->Item(index).IsEmpty())
		{
			// this content string takes no endMarker (add a delimiting space though)
			theRest << aSpace << filterMkrEnd;
		}
		else
		{
			theRest << pFilteredEndMarkers->Item(index) << aSpace << filterMkrEnd;
		}
		markersAndContent += theRest;
		AddToFilteredInfo(markersAndContent);
		markersAndContent.Empty();
		theRest.Empty();
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
			m_endMarkers = endMarker;
		else
			m_endMarkers += _T(" ") + endMarker;
	}
	else
	{
		// its USFM set (or combined USFM and PNG, but we treat this as USFM)
		m_endMarkers += endMarker;
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

void CSourcePhrase::SetEndMarkersAsNowMedial(wxArrayString* pMedialsArray)
{
	bool bAddedSomething = FALSE;

	// make a string array to hold it or them, if no string array exists yet
	if (pMedialsArray == NULL)
		pMedialsArray = new wxArrayString;
	wxASSERT(pMedialsArray != NULL);

	wxString oldEndMkrs = GetEndMarkers();
	m_bHasInternalMarkers = TRUE;

	wxArrayString oldEndMarkersArray;
	this->GetEndMarkersAsArray(&oldEndMarkersArray);

	// anything not already in it, add to it
	if (pMedialsArray->IsEmpty())
	{
		// it's just been created, so add the "old" ones
		int count = oldEndMarkersArray.GetCount();
		int index;
		for (index = 0; index < count; index++)
		{
			wxString endmarker = oldEndMarkersArray.Item(index);
			pMedialsArray->Add(endmarker);
		}
	}
	else
	{
		// it already exists, so add the unique ones
		bAddedSomething = AddNewStringsToArray(pMedialsArray, &oldEndMarkersArray);
	}
}


#endif	// _DOCVER5

 
