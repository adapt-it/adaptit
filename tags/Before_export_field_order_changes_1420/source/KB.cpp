/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KB.cpp
/// \author			Bill Martin
/// \date_created	21 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CKB class.
/// The CKB class defines the knowledge base class for the App's 
/// data storage structures. The CKB class is completely independent of 
/// the list of CSourcePhrase instances stored on the App. This independence 
/// allows modifying the latter, or any of these CKB structures, independently 
/// of the other with no side effects. Links between the document structures and the 
/// CKB structures are established dynamically by the lookup engine on a 
/// "need to know" basis.
/// \derivation		The CKB class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KB.h"
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

#include <wx/hashmap.h>
#include <wx/datstrm.h> // needed for wxDataOutputStream() and wxDataInputStream()
#include <wx/progdlg.h> // for wxProgressDialog
#include <wx/wfstream.h>
#include <wx/txtstrm.h>


#include "Adapt_It.h"
#include "Adapt_ItView.h"
//#include "Adapt_ItCanvas.h"
//#include "MainFrm.h"
#include "SourcePhrase.h"
#include "helpers.h"
#include "Adapt_ItDoc.h"
#include "KB.h"
#include "AdaptitConstants.h" 
#include "Pile.h"
//#include "Layout.h"
#include "TargetUnit.h"
#include "RefStringMetadata.h"
#include "RefString.h"
#include "LanguageCodesDlg.h"
#include "BString.h"
#include "XML.h"
#include "WaitDlg.h"
#include <wx/textfile.h>

// Define type safe pointer lists
#include "wx/listimpl.cpp"

IMPLEMENT_DYNAMIC_CLASS(CKB, wxObject) 

// globals needed due to moving functions here from mainly the view class
// next group for auto-capitalization support
extern bool	gbAutoCaps;
extern bool	gbSourceIsUpperCase;
extern bool	gbNonSourceIsUpperCase;
extern bool	gbMatchedKB_UCentry;
extern bool	gbNoSourceCaseEquivalents;
extern bool	gbNoTargetCaseEquivalents;
extern bool	gbNoGlossCaseEquivalents;
extern wxChar gcharNonSrcLC;
extern wxChar gcharNonSrcUC;
extern wxChar gcharSrcLC;
extern wxChar gcharSrcUC;

// next, miscellaneous needed ones
extern bool gbNoAdaptationRemovalRequested;
extern bool gbCallerIsRemoveButton;
extern wxChar gSFescapechar;
extern bool bSupportNoAdaptationButton;
extern bool gbSuppressStoreForAltBackspaceKeypress;
extern bool gbByCopyOnly;
extern bool gbInhibitMakeTargetStringCall;
//extern bool gbRemovePunctuationFromGlosses; BEW removed 13Nov10

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CKB::CKB()
{
	m_pApp = &wxGetApp();
	m_nMaxWords = 1; // value for a minimal phrase

	for (int i = 0; i< MAX_WORDS; i++)
	{
		m_pMap[i] = new MapKeyStringToTgtUnit;
		wxASSERT(m_pMap[i] != NULL);
	}
	SetCurrentKBVersion(); // currently KB_VERSION2 which is defined as 2
}

CKB::CKB(bool bGlossingKB)
{
	m_pApp = &wxGetApp();
	m_nMaxWords = 1; // value for a minimal phrase

	for (int i = 0; i< MAX_WORDS; i++)
	{
		m_pMap[i] = new MapKeyStringToTgtUnit;
		wxASSERT(m_pMap[i] != NULL);
	}
	m_bGlossingKB = bGlossingKB; // set the KB type, TRUE for GlossingKB, 
								   // FALSE for adapting KB
	SetCurrentKBVersion(); // currently KB_VERSION2 which is defined as 2
}

// copy constructor - it doesn't work, see header file for explanation
/*
CKB::CKB(const CKB &kb)
{
	const CKB* pCopy = &kb;
	POSITION pos;

	m_bGlossingKB = pCopy->m_bGlossingKB; // BEW added 12May10
	m_pApp = pCopy->m_pApp; // BEW added 12May10
	m_kbVersionCurrent = pCopy->m_kbVersionCurrent; // BEW added 12May10
	m_nMaxWords = pCopy->m_nMaxWords;
	m_sourceLanguageName = pCopy->m_sourceLanguageName;
	m_targetLanguageName = pCopy->m_targetLanguageName;

	//CObList* pTUList = pCopy->m_pTargetUnits;
*/
/*
	// make the new list by copying source list, using copy constructor for the target units
	if (!pCopy->m_pTargetUnits->IsEmpty())
	{
		pos = pTUList->GetHeadPosition();
		ASSERT(pos != NULL);
		while (pos != NULL)
		{
			CTargetUnit* pTU = (CTargetUnit*)pTUList->GetNext(pos);
			ASSERT(pTU != NULL);
			try
			{
				CTargetUnit* pTUCopy = new CTargetUnit(); // can't use copy constructor due to C++ bug
				pTUCopy->Copy(*pTU); // simulates the copy constructor (m_translations guaranteed 
									 // to be defined)
fails here>>>>	m_pTargetUnits->AddTail(pTUCopy);
			}
			catch (CMemoryException e)
			{
AfxMessageBox("Memory Exception creating CTargetUnit copy, copying list in copy constructor for CKB.");
				ASSERT(FALSE);
				AfxAbort();
			}
		}
	}
*/
/*
	// now copy the maps
	for (int i=0; i < 10; i++)
	{
		CMapStringToOb* pThisMap = m_pMap[i];
		CMapStringToOb* pThatMap = pCopy->m_pMap[i];
		POSITION p;
		if (!pThatMap->IsEmpty())
		{
			p = pThatMap->GetStartPosition();
			ASSERT(p != NULL);
			while (p != NULL)
			{
				CTargetUnit* pTUCopy;
				CString key;
				pThatMap->GetNextAssoc(p,key,(CObject*&)pTUCopy);
				ASSERT(!key.IsEmpty());
				CTargetUnit* pNewTU = new CTargetUnit(); // can't use copy constructor
														 // (see CTargetUnit code for explanation)
				pNewTU->Copy(*pTUCopy); // get's round the problem of m_translations not being 
										// defined before use
				try
				{
					pThisMap->SetAt(key,(CObject*)pNewTU);
				}
				catch (CMemoryException e)
				{
AfxMessageBox("Memory Exception creating CTargetUnit copy, copying map in copy constructor for CKB.");
					ASSERT(FALSE);
					AfxAbort();
				}
			}
		}
	}
}
*/

CKB::~CKB()
{
	// MFC note: The destructor doesn't get called when closing an SDI doc, so use 
	// OnCloseDocument() for deleting dynamically allocated objects
	// WX note: this destructor is called when closing project, but not when closing doc
}

// the copy can be done efficiently only by scanning the maps one by one (so we know how many
// source words are involved each time), and for each key, construct a copy of it (because RemoveAll()
// done on a CMapStringToOb will delete the key strings, but leave the CObject* memory untouched, so
// the keys have to be wxString copies of the souce CKB's key CStrings), and then use
// StoreAdaptation to create the new CKB's contents; for glossing, the copy is done only for the
// first map, and its m_nMaxWords value must always remain equal to 1.
// BEW 13Nov10 changes for supporting Bob Eaton's request for using all tabs in glossing kb
void CKB::Copy(const CKB& kb)
{

	wxASSERT(this);
	const CKB* pCopy = &kb;
	wxASSERT(pCopy);

	if (pCopy->m_bGlossingKB) // BEW changed test 12May10
	{
		//m_nMaxWords = 1; removed 13Nov10
		m_nMaxWords = pCopy->m_nMaxWords;
		m_sourceLanguageName.Empty(); // don't need these
		m_targetLanguageName.Empty();
	}
	else
	{
		m_nMaxWords = pCopy->m_nMaxWords;
		m_sourceLanguageName = pCopy->m_sourceLanguageName;
		m_targetLanguageName = pCopy->m_targetLanguageName;
	}
	m_bGlossingKB = pCopy->m_bGlossingKB; // BEW added 12May10
	m_pApp = pCopy->m_pApp; // BEW added 12May10
	m_kbVersionCurrent = pCopy->m_kbVersionCurrent; // BEW added 12May10

// ***** TODO *****
	// once we have a CRefStringMetadata class, pointed at, the copy will have to make copies
	// of them and set up the mutual pointers in the copies -- so add that code later on

	// now recreate the maps (note: can't copy, as we must associate what is in our new list)
	// RemoveAll for a map deletes the key strings, but not the associated object pointers, so
	// we will have to ensure that the strings used in the following code are copies
	for (int i=0; i < MAX_WORDS; i++)
	{
		MapKeyStringToTgtUnit* pThisMap = m_pMap[i];
		wxASSERT(pThisMap);
		MapKeyStringToTgtUnit* pThatMap = pCopy->m_pMap[i];
		wxASSERT(pThatMap);

		MapKeyStringToTgtUnit::iterator iter;
		MapKeyStringToTgtUnit::iterator xter;
		if (!pThatMap->size() == 0)
		{
			for (iter = pThatMap->begin(); iter != pThatMap->end(); ++iter)
			{
				CTargetUnit* pTUCopy;
				wxString key;
				key = iter->first;		// iter->first accesses the key of the element pointed to
				pTUCopy = iter->second;	// iter->second accesses the value of the element pointed to
				wxASSERT(!key.IsEmpty());
				wxASSERT(pTUCopy);
				wxString keyCopy = key;
				CTargetUnit* pNewTU = new CTargetUnit(); // can't use copy constructor
														 // (see CTargetUnit code for explanation)
				wxASSERT(pNewTU != NULL);
				pNewTU->Copy(*pTUCopy); // get's round the problem of m_translations not being 
										// defined before use
				xter = pThisMap->find(keyCopy);
				if (xter != pThisMap->end())
				{
					// key exists in the map
					pThisMap->insert(*xter); // associates (any new) pNewTU value with the keyCopy
				}
				else
				{
					// key not in the map
					// The [] index operator can be used either of two ways in assignment statement
					(*pThisMap)[keyCopy] = pNewTU; // clearest
					// or, the following is equivalent, but not so clear
					//pThisMap->operator[](keyCopy) = pNewTU;
					// NOTE: The docs say of the wxHashMap::operator[] "Use it as an array subscript.
					// The only difference is that if the given key is not present in the hash map,
					// an element with the default value_type() is inserted in the table."
				}
				// BEW removed 29May10
				//pTUList->Append(pNewTU);
			}
		}
	}
}

// the "adaptation" parameter will contain an adaptation if m_bGlossingKB is FALSE, or if
// TRUE if will contain a gloss; and also depending on the same flag, the pTgtUnit will have
// come from either the adaptation KB or the glossing KB.
// Returns NULL if no CRefString instance matching the criterion was found
// BEW 11May10, moved from view class, and made private (it's only called once, in GetRefString())
// BEW 18Jun10, changes needed for support of kbVersion 2, if an instance is found which
// matches the adaptation parameter, but the m_bDeleted flag is TRUE, then that is
// regarded as a failure to find an instance, and NULL is returned
CRefString* CKB::AutoCapsFindRefString(CTargetUnit* pTgtUnit, wxString adaptation)
{
	CRefString* pRefStr = (CRefString*)NULL;
	TranslationsList* pList = pTgtUnit->m_pTranslations;
	wxASSERT(pList);
	bool bNoError;
	if (gbAutoCaps && gbSourceIsUpperCase && !gbMatchedKB_UCentry && !adaptation.IsEmpty())
	{
		// possibly we may need to change the case of first character of 'adaptation'
		bNoError = m_pApp->GetDocument()->SetCaseParameters(adaptation, FALSE); // FALSE means it is an
														 // adaptation or a gloss
		if (bNoError && gbNonSourceIsUpperCase && (gcharNonSrcLC != _T('\0')))
		{
			// a change to lower case is called for; 
			// otherwise leave 'adaptation' unchanged
			adaptation.SetChar(0,gcharNonSrcLC);
		}
	}
	else
	{
        // either autocapitalization is OFF, or the source text starts with a lower case
        // letter, or the first lookup for the key failed but an upper case lookup
        // succeeded (in which case we assume its a old style entry in the KB) or the
        // adaptation string is empty - any of these conditions require that no change of
        // case be done
		;
	}

	TranslationsList::Node *pos = pList->GetFirst();
	while(pos != NULL)
	{
		pRefStr = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pRefStr != NULL);
		if (pRefStr->m_translation == adaptation)
		{
			if (pRefStr->m_bDeleted)
			{
				// regard this as unmatched
				return (CRefString*)NULL;
			}
			else
			{
				return pRefStr; // we found it
			}
		}
		else
		{
			if (adaptation.IsEmpty())
			{
				// it might be a <Not In KB> CSourcePhrase, check it out
				if (pRefStr->m_translation == _T("<Not In KB>") && !pRefStr->m_bDeleted)
				{
					return pRefStr; // we return the pointer to this too
				}
			}
		}
	}
	// finding it failed so return NULL
	return (CRefString*)NULL;
}

// in this function, the keyStr parameter will always be a source string; the caller must
// determine which particular map is to be looked up and provide it's pointer as the first
// parameter; and if the lookup succeeds, pTU is the associated CTargetUnit instance's
// pointer. This function, as the name suggests, has the smarts for AutoCapitalization being
// On or Off. A failure to find an associated CTargetUnit instance for the passed in keyStr
// results in FALSE being returned, and also pTU is returned as NULL; otherwise, TRUE is
// returned and pTU is a pointer to the matched instance.
// WX Note: Changed second parameter to CTargetUnit*& pTU.
// BEW 12Apr10, no changes needed for support of doc version 5
// BEW 13May10, moved here from CAdapt_ItView class
// BEW 18Jun10, no changes needed for support of kbVersion 2
// BEW 13Nov10, no changes to support Bob Eaton's request for glosssing KB to use all maps
bool CKB::AutoCapsLookup(MapKeyStringToTgtUnit* pMap, CTargetUnit*& pTU, wxString keyStr)
{
	wxString saveKey;
	gbMatchedKB_UCentry = FALSE; // ensure it has default value 
								 // before every first lookup
	MapKeyStringToTgtUnit::iterator iter;

    // the test of gbCallerIsRemoveButton is to prevent a wrong change to lower case if
    // autocapitalization is on and the user clicked in the KB editor, or in Choose
    // Translation dlg, the Remove button; otherwise the wrong entry could get deleted
    // and the data made invalid
	if (gbAutoCaps && !gbCallerIsRemoveButton)
	{
		if (gbNoSourceCaseEquivalents)
			// this is equivalent to gbAutoCaps being off, so just do a normal lookup
			goto a;

		// auto capitalization is ON, so determine the relevant parameters etc.
		bool bNoError = m_pApp->GetDocument()->SetCaseParameters(keyStr); // extra param
													// is TRUE since it is source text
		if (!bNoError)
			goto a; // keyStr must have been empty (impossible) or the user
					// did not define any source language case correspondences
		if (gbSourceIsUpperCase && (gcharSrcLC != _T('\0')))
		{
			// we will have to change the case for the first lookup attempt
			saveKey = keyStr; // save for an upper case lookup 
							  // if the first lookup fails
			// make the first character of keyStr be the appropriate lower case one
			keyStr.SetChar(0,gcharSrcLC); // gcharSrcLC is set within the 
										  // SetCaseParameters() call
			// do the lower case lookup
			iter = pMap->find(keyStr);
			if (iter != pMap->end())
			{
				pTU = iter->second; // we have a match, pTU now points 
									// to a CTargetUnit instance
				wxASSERT(pTU != NULL);
				return TRUE;
			}
            // if we get here, then the match failed, so just in case there is an upper
            // case entry in the knowledge base (from when autocapitalization was OFF),
            // look it up; if there, then set the gbMatchedKB_UCentry to TRUE so the caller
            // will know that no restoration of upper case will be required for the gloss
            // or adaptation that it returns
			iter = pMap->find(saveKey);
			if (iter != pMap->end())
			{
				pTU = iter->second; // we have a match, pTU now points 
									// to a CTargetUnit instance
				wxASSERT(pTU != NULL);
                // found a match, so we can assume its refStrings contain upper case
                // initial strings already
				gbMatchedKB_UCentry = TRUE;
				return TRUE;
			}
			else
			{
				pTU = (CTargetUnit*)NULL;
				return FALSE;
			}
		}
		else
		{
			// first letter of source word or phrase is lower case, so do a normal lookup
			goto a;
		}
	}
	else
	{
		// auto capitalization is OFF, so just do a lookup with the keyStr as supplied
a:		iter = pMap->find(keyStr);
		if (iter != pMap->end())
		{
			pTU = iter->second;
			wxASSERT(pTU != NULL);
			return TRUE;
		}
		else
		{
			pTU = (CTargetUnit*)NULL;
			return FALSE;
		}
	}
#ifndef __VISUALC__
	return FALSE; // unreachable according to VC7.1, but gcc says it is needed!!!
#endif
}

// looks up the knowledge base to find if there is an entry in the map with index
// nSrcWords-1, for the key keyStr and then searches the list in the CTargetUnit for the
// CRefString with m_translation member identical to valueStr, and returns a pointer to
// that CRefString instance. If it fails, it returns a null pointer. 
// (Note: Jan 27 2001 changed so that it returns the pRefString for a <Not In KB> entry).
// For version 2.0 and later, pKB will point to the glossing KB when m_bGlossingKB is TRUE.
// Ammended, July 2003, to support auto capitalization
// BEW 11May10, moved from CAdapt_ItView class
// BEW 12May10, changed signature, because from 5.3.0 and onwards, each KB now knows
// whether it is a glossing KB or an adapting KB -- using the private member boolean
// m_bGlossingKB
// BEW 21Jun10, no changes needed for support of kbVersion 2, because internally
// the call of AutoCapsFindRefString() returns NULL if the only match was of a CRefString
// instance with m_bDeleted set to TRUE
// BEW 13Nov10, changes to support Bob Eaton's request for glosssing KB to use all maps
CRefString* CKB::GetRefString(int nSrcWords, wxString keyStr, wxString valueStr)
{
	// ensure nSrcWords is 1 if this is a GlossingKB access << BEW removed 13Nov10
	//if (m_bGlossingKB)
	//	nSrcWords = 1;
	MapKeyStringToTgtUnit* pMap = this->m_pMap[nSrcWords-1];
	wxASSERT(pMap != NULL);
	CTargetUnit* pTgtUnit;	// wx version changed 2nd param of AutoCapsLookup() below to
							// directly use CTargetUnit* pTgtUnit
	CRefString* pRefStr;
	bool bOK = AutoCapsLookup(pMap,pTgtUnit,keyStr);
	if (bOK)
	{
		return pRefStr = AutoCapsFindRefString(pTgtUnit,valueStr);
	}
	// lookup failed, so the KB state is different than data in the document suggests,
	// a Consistency Check operation should be done on the file(s)
	return (CRefString*)NULL;
}

// this overload of GetRefString() is useful for LIFT imports
// BEW 21Jun10, no changes needed for support of kbVersion 2, because internally
// the call of AutoCapsFindRefString() returns NULL if the only match was of a CRefString
// instance with m_bDeleted set to TRUE
// BEW 13Nov10, no changes to support Bob Eaton's request for glosssing KB to use all maps
CRefString* CKB::GetRefString(CTargetUnit* pTU, wxString valueStr)
{
	wxASSERT(pTU);
	CRefString* pRefStr = AutoCapsFindRefString(pTU,valueStr);
	if (pRefStr == NULL)
	{
		// it's not in the m_translations list, so return NULL
		return (CRefString*)NULL;
	}
	else
	{
		return pRefStr;
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      pRefString      ->  pointer to the CRefString object to be deleted
/// \param      pSrcPhrase      ->  pointer to the CSourcePhrase instance whose m_key
///                                 member supplies the key for locating the CTargetUnit
///                                 in the relevant knowledge base map
/// \param      nWordsInPhrase  ->  used in order to define which map to look up
/// \remarks
/// Removes the wxString and reference count associated with it, (embodied as a CRefString
/// object)from a CTargetUnit instance in the knowledge base, as follows:
/// If pRefString is referenced only once, remove it from KB (since the phraseBox will hold
/// a copy of its string if it is still wanted), or if it is referenced more than once,
/// then just decrement the reference count by one, and set the srcPhrase's m_bHasKBEntry
/// flag to FALSE; also make sure m_bAlwaysAsk is set or cleared as the case may be. For
/// version 2.0 and onwards we must test m_bGlossingKB in order to set the KB pointer to
/// either the adaption KB or the glossing KB; and the caller must supply the appropriate
/// first and last parameters (ie. a pRefString from the glossing KB and nWordsInPhrase set
/// to 1 when m_bGlossingKB is TRUE)
/// Ammended, July 2003, to support auto capitalization
/// BEW changed 05July2006 to fix a long-standing error where the m_bHasKBEntry flag, or
/// the m_bHasGlossingKBEntry flag, was not cleared when the phrase box lands at a location
/// which earlier contributed an entry to the KB. The reason it wasn't cleared was because
/// I put the code for that in an "if (pRefString == NULL)" test's true block, so it
/// wouldn't get called except at locations which did not as yet have any KB entry! So I
/// commented out the test. (In StoreText() and StoreTextGoingBack() I unilaterally cleared
/// the flags at its start so that the store would not fail; I've now removed that bit of
/// code & tested - seems fine now.)
/// BEW 20Mar07: code added to suppress the decrement or kb entry removal if retranslating
/// is currenty going on, or editing of a retranslation (we don't want to dumb down the KB)
/// needlessly
/// BEW 11May10, moved from CAdapt_ItView class
/// BEW changed 12May10, to use private member boolean m_bGlossingKB for tests
/// BEW 8Jun10, updated for kbVersion 2 support (legacy code kept, commented out, in case
/// at a future date we want to use it for a 'clear kb' [of deleted entries] capability)
// BEW 13Nov10, changes to support Bob Eaton's request for glosssing KB to use all maps
void CKB::RemoveRefString(CRefString *pRefString, CSourcePhrase* pSrcPhrase, int nWordsInPhrase)
{
	if (m_bGlossingKB)
	{
		pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
		nWordsInPhrase = nWordsInPhrase; // to prevent compiler warning (BEW 13Nov10)
		//nWordsInPhrase = 1; // ensure correct value for a glossing KB << BEW removed 13Nov10
	}
	else
	{
		pSrcPhrase->m_bHasKBEntry = FALSE;
	}
	if (!m_bGlossingKB && pSrcPhrase->m_bNotInKB)
	{
		// this block can only be entered when the adaption KB is in use
		pSrcPhrase->m_bHasKBEntry = FALSE;
		return; // return because no KB adaptation exists for this source phrase
	}
    // BEW added 06Sept06: the above tests handle what must be done to the document's
    // pSrcPhrase instance passed in, but it could be the case that the preceding
    // GetRefString() call did not find a KB entry with the given reference string
    // instance, in which case it would have returned NULL. In that case there is nothing
    // to remove, and no more to be done here, so test for this possibility and return
    // immediately if so.
	if (pRefString == NULL)
		return;

    // for autocapitalization support, we have to be careful that the translation (or
    // gloss) which we delete in the case when the ref count drops to zero is the actual
    // one in the KB - we can't always take it from pSrcPhrase->m_key because if auto caps
    // is ON it might be the case that the entry was put in when auto caps is OFF and then
    // we'd wrongly change to lower case and not succeed in removing the entry from the
    // map, so we have to be a bit more clever. So we'll set s to the key as on the
    // sourcephrase, and a second string s1 will have the other case equivalent, and if the
    // first does not get removed, we try the second (which should work); we still need to
    // form the second only when gbAutoCaps is TRUE, since when it is OFF, a failed
    // GetRefString( ) call will not result in an attempt to remove a lower case entry
    // stored when gbAutoCaps was ON
	wxString s = pSrcPhrase->m_key;
	wxString s1 = s;
	bool bNoError = TRUE;
	if (gbAutoCaps)
	{
		bNoError = m_pApp->GetDocument()->SetCaseParameters(s1);
		if (bNoError && gbSourceIsUpperCase && (gcharSrcLC != _T('\0')))
		{
			// make it start with lower case letter
			s1.SetChar(0,gcharSrcLC);
		}
	}
	int nRefCount = pRefString->m_refCount;
	wxASSERT(nRefCount > 0);
	if (nRefCount > 1)
	{
        // more than one reference to it, so just decrement & remove the srcPhrase's
        // knowledge of it, this does not affect the count of how many translations there
        // are, so m_bAlwaysAsk is unaffected. Version 2 has to test m_bGlossingKB and set
        // the relevant flag to FALSE.
		if (!(m_pApp->GetRetranslation()->GetIsRetranslationCurrent()))
		{
			// BEW 20Mar07: don't decrement if retranslation, or editing of same, 
			// is currently happening
			pRefString->m_refCount = --nRefCount;
		}
		if (m_bGlossingKB)
		{
			pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
		}
		else
		{
			pSrcPhrase->m_bHasKBEntry = FALSE;
		}
	}
	else
	{
        // from version 1.2.9 onwards, since <no adaptation> has to be caused manually, we
        // no longer want to automatically remove an empty adaptation whenever we land on
        // one - well, not quite true, we can remove automatically provided the ref count
        // is greater than one, but we have to block automatic decrements which would
        // result in a count of zero - else we'd lose the value from the KB altogether and
        // user would not know. We need this protection because an ENTER will not cause
        // automatic re-storing of it. So now, we will just not remove the last one. This
        // will possibly skew the ref counts a bit for empty adaptations, if the user hits
        // the <no adaptation> button more than once for an entry (making them too large)
        // or landing on an empty one several times (makes count to small), would not
        // matter anyway. To manually remove empty adaptations from the KB the user still
        // has the option of doing it in the KB Editor, or in the ChooseTranslation dialog.

        //BEW changed behaviour 20Jun06 because unilaterally returning here whenever the
        //m_translation string was empty meant that if the user wanted to remove his
        //earlier "<no adaptation>" choice, there was no way to effect it from the
        //interface. So now we have a global flag gbNoAdaptationRemovalRequested which is
        //TRUE whenever the user hits backspace or Del key in an empty phrasebox, provided
        //that locatation's CSourcePhrase has m_bHasKBEntry (when in adapting mode) set
        //TRUE, or m_bHasGlossingKBEntry (when in glossing mode) set TRUE. Hence if neither
        //BS or DEL key is pressed, we'll get the legacy (no deletion & retaining of the
        //flag value) behaviour as before.
		if (pRefString->m_translation.IsEmpty())
		{
			if (!gbNoAdaptationRemovalRequested)
				return; // never automatically reduce count to zero; if user wants to be 
                        //rid of it, he must do so manually -- no, there was no 'manual'
                        //way to do it for the document CSourcePhrase instance, so 20Jun06
                        //change introduces a backspace or DEL key press to effect the
                        //removal
		}
		gbNoAdaptationRemovalRequested = FALSE; // ensure cleared to default value, & 
                        // permit removal after the flag may have been used in previous
                        // block

		if (m_pApp->GetRetranslation()->GetIsRetranslationCurrent())
		{
            // BEW 20Mar07: don't remove if retranslation, or editing of same, is currently
            // happening because we don't want retranslations to effect the 'dumbing down'
            // of the KB by removal of perfectly valid KB entries belonging to the
            // retranslation span formerly
			return;
		}

        // this reference string is only referenced once, so remove it entirely from KB or
        // from the glossing KB, depending on m_bGlossingKB flag's value
		CTargetUnit* pTU = pRefString->m_pTgtUnit; // point to the owning targetUnit
		wxASSERT(pTU != NULL);
		int nTranslations = pTU->m_pTranslations->GetCount();
		wxASSERT(nTranslations > 0); // must be at least one
		if (nTranslations == 1)
		{
			// BEW 8Jun10, changed next section to kbVersion 2 protocol; DO NOT DELETE OLD CODE
			// because it may be required later if we provide a "clear" option for deleted
			// entries in the KB
			// kbv2 code retains the entry in the map, but sets m_bDeleted flag and deletion datetime
			pRefString->m_pRefStringMetadata->m_deletedDateTime = GetDateTimeNow();
			pRefString->m_bDeleted = TRUE;
		}
		else
		{
			// there are other CRefString instances, so don't remove its owning targetUnit
			
			// BEW 8Jun10, changed next section to kbVersion 2 protocol; DO NOT DELETE OLD CODE
			// because it may be required later if we provide a "clear" option for deleted
			// entries in the KB
			// kbv2 code retains the entry in the map, but sets m_bDeleted flag and deletion datetime
			pRefString->m_pRefStringMetadata->m_deletedDateTime = GetDateTimeNow();
			pRefString->m_bDeleted = TRUE;
		}

		// inform the srcPhrase that it no longer has a KB entry (or a glossing KB entry)
		if (m_bGlossingKB)
			pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
		else
			pSrcPhrase->m_bHasKBEntry = FALSE;
	}
}

// pass in a source string, to be converted to initial lower case;
// or a gloss or adaptation string which is to be saved to a KB, and internally
// have all the smarts for determining if a change of case for first character
// is needed, and return the string, the same or suitably ammended, back to the
// caller. bIsSrc parameter is TRUE by default.
// BEW 12Apr10, no changes needed for support of docVersion 5
// BEW 18Jun10, no changes needed for support of kbVersion 2
// BEW 13Nov10, no changes to support Bob Eaton's request for glosssing KB to use all maps
wxString CKB::AutoCapsMakeStorageString(wxString str, bool bIsSrc)
{
	bool bNoError = TRUE;
	if (str.IsEmpty())
		return str;

	// gbMatchedKB_UCentry is never relevant to storing when gbAutoCaps is on,
	// even if the former is TRUE, we'll still want to convert the source string
	// to have initial lower case when storing
	if (bIsSrc)
	{
		// SetCaseParameters( ) will already have been called, 
		// so convert if required to do so
		if (gbAutoCaps && gbSourceIsUpperCase && (gcharSrcLC != _T('\0')))
		{
			str.SetChar(0,gcharSrcLC);
		}
	}
	else
	{
		// must be a gloss or adaptation string
		if (gbAutoCaps && gbSourceIsUpperCase)
		{
			bNoError = m_pApp->GetDocument()->SetCaseParameters(str,FALSE);
			if (!bNoError)
				goto a;
			if (gbNonSourceIsUpperCase && (gcharNonSrcLC != _T('\0')))
			{
				// we need to make it start with lower case for storage in the KB
				str.SetChar(0,gcharNonSrcLC);
			}
		}
	}
a:	return str;
}

// BEW created 11May10, to replace several lines which always call GetRefString() and then
// a little later, RemoveRefString; this will enable about 300 lines of code spread over
// about 30 functions to be replaced by about 30 calls of this function. The UseForLookup
// enum values are a set of two: useGlossOrAdaptationForLookup and
// useTargetPhraseForLookup. When the latter is passed in, the targetPhrase value which is
// looked up is assumed to be the phrase box's contents (the caller is responsible to make
// sure that is so). When the former is passed is, then the targetPhrase param is ignored,
// and the m_gloss or m_adaption member of the passed in pSrcPhrase is used for lookup -
// depending on the value of the private member m_bGlossingKB, when TRUE, m_gloss is used,
// when FALSE, m_adaption is used.
// BEW 17Jun10, no changes needed for support of kbVersion 2
// BEW 13Nov10, changes to support Bob Eaton's request for glosssing KB to use all maps
void CKB::GetAndRemoveRefString(CSourcePhrase* pSrcPhrase, wxString& targetPhrase, 
								enum UseForLookup useThis)
{
	CRefString* pRefStr = NULL;
	if (m_bGlossingKB)
	{
		if (useThis == useTargetPhraseForLookup)
		{
			//pRefStr = GetRefString(1, pSrcPhrase->m_key, targetPhrase);
			pRefStr = GetRefString(pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key, targetPhrase);
		}
		else // useThis has the value useGlossOrAdaptationForLookup
		{
			//pRefStr = GetRefString(1, pSrcPhrase->m_key, pSrcPhrase->m_gloss);
			pRefStr = GetRefString(pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key, pSrcPhrase->m_gloss);
		}
		// ensure correct flag value
		if (pRefStr == NULL && pSrcPhrase->m_bHasGlossingKBEntry)
			pSrcPhrase->m_bHasGlossingKBEntry = FALSE; // must be FALSE for a successful lookup on return
		if (pRefStr != NULL)
		{
			//RemoveRefString(pRefStr, pSrcPhrase, 1);
			RemoveRefString(pRefStr, pSrcPhrase, pSrcPhrase->m_nSrcWords);
		}
	}
	else // it is an adapting KB 
	{
		if (useThis == useTargetPhraseForLookup)
		{
			pRefStr = GetRefString(pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key, targetPhrase);
		}
		else // useThis has the value useGlossOrAdaptationForLookup
		{
			pRefStr = GetRefString(pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key, pSrcPhrase->m_adaption);
		}
		// ensure correct flag value
		if (pRefStr == NULL && pSrcPhrase->m_bHasKBEntry)
			pSrcPhrase->m_bHasKBEntry = FALSE; // must be FALSE for a successful lookup on return
		if (pRefStr != NULL)
		{
			RemoveRefString(pRefStr, pSrcPhrase, pSrcPhrase->m_nSrcWords);
		}
	}
}

// returns TRUE if a matching KB entry found; when glossing, pKB points to the glossing KB, when
// adapting it points to the normal KB
// BEW 26Mar10, no changes needed for support of doc version 5
// BEW 13May10, moved here from CPhraseBox class
// BEW 21Jun10, no changes needed for support of kbVersion 2
// BEW 13Nov10, no changes to support Bob Eaton's request for glosssing KB to use all maps
bool CKB::FindMatchInKB(int numWords, wxString keyStr, CTargetUnit *&pTargetUnit)
{
	MapKeyStringToTgtUnit* pMap = m_pMap[numWords-1];
	CTargetUnit* pTU;
	bool bOK = AutoCapsLookup(pMap,pTU,keyStr);
	if (bOK)
		pTargetUnit = pTU;	// makes pTargetUnit point to same object pointed to by pTU
							// and shouldn't require use of an assignment operator. Since
							// pTargetUnit is a reference parameter, FindMatchInKb will
							// assign the new pointer to the caller pointer argument
	return bOK;
}

// Modified to work for either glossing or adapting KBs
// BEW 21Jun10, changes needed for supporting kbVersion 2's m_bDeleted flag
// BEW 13Nov10, changed by Bob Eaton's request, for glossing KB to use all maps
bool CKB::IsAlreadyInKB(int nWords,wxString key,wxString adaptation)
{
	CTargetUnit* pTgtUnit = 0;

	// is there a targetunit for this key in the KB?
	bool bFound;
	//if (m_bGlossingKB)
		// only check first map
	//	bFound = FindMatchInKB(1,key,pTgtUnit); 
	//else // is adapting
		bFound = FindMatchInKB(nWords,key,pTgtUnit);
	if (!bFound)
		return FALSE;

	// check if there is a matching adaptation
	// BEW 21Jun10, FindMatchInKB() only returns a pointer to a CTargetUnit instance, and
	// that instance may contain CRefString instances marked as deleted. So matching any
	// of these in the loop below has to be deemed a non-match, so that only matches with
	// the m_bDeleted flag with value FALSE qualify as a match
	TranslationsList::Node* pos = pTgtUnit->m_pTranslations->GetFirst(); 
	while (pos != 0)
	{
		CRefString* pRefStr = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pRefStr);
		if (adaptation == pRefStr->m_translation)
		{
			if (!pRefStr->m_bDeleted)
			{
				// this adaptation (or gloss) qualifies as a match
				return TRUE;
			}
		}
	}
	return FALSE; // did not find a match
}

// BEW 9Jun10, modified to suport kbVersion 2, and also to simplify the parsing code for
// SFM kb import (to remove use of pSrcPhrase) - using code similar to that used for the
// xml parse of a LIFT file being imported
// BEW 8Jun10, added markers and code for support of kbVersion 2 data additions, and for
// support of both LIFT import and \lx &\ge -based SFM KB import
// BEW 13Nov10 changes for supporting Bob Eaton's request for using all tabs in glossing kb
void CKB::DoKBImport(wxString pathName,enum KBImportFileOfType kbImportFileOfType)
{
	//CSourcePhrase* pSrcPhrase = new CSourcePhrase;

	// guarantee safe value for storage of contents to KB, or glossing KB
	//if (m_bGlossingKB)
	//	pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
	//else
	//	pSrcPhrase->m_bHasKBEntry = FALSE;
	wxString key;
	key.Empty();
	wxString adaption; // use for actual adaptation, or gloss when glossing is ON
	adaption.Empty();
	wxString line;
	line.Empty();
	wxString ss1 = gSFescapechar;
	wxString keyMarker = ss1 + _T("lx");
	wxString adaptionMarker = ss1 + _T("ge");
	bool bKeyDefined = FALSE;
	int nOffset = -1;
	//m_pApp->m_bSaveToKB = TRUE;

	// BEW 9Jun10 support for kbVersion 2, uses the custom markers:
	// \del  for the m_bDeleted flag value "0" or "1" values (for FALSE or TRUE)
	// \cdt  for the m_creationDateTime string
	// \mdt  for the m_modifiedDateTime string
	// \ddt  for the m_deletedDateTime string
	// \wc   for the m_whoCreated string
	// As for AI_USFM.xml, any of the above for which the string is empty, will not be
	// included in the output - because defaults will handle those cases at import
	wxString s1 = gSFescapechar;
	// note: delimiting space is not included in the next four wxString definitions
	wxString delflag = s1 + _T("del");
	wxString createDT = s1 + _T("cdt");
	wxString modDT = s1 + _T("mdt");
	wxString delDT = s1 + _T("ddt");
	wxString whoCr = s1 + _T("wc");

	if (kbImportFileOfType == KBImportFileOfLIFT_XML)
	{
		// we're importing from a LIFT file
		wxFile f;
		wxLogNull logno; // prevent unwanted system messages
		// (until wxLogNull goes out of scope, ALL log messages are suppressed - be warned)

		if( !f.Open( pathName, wxFile::read))
		{
			wxMessageBox(_("Unable to open import file for reading."),
		  _T(""), wxICON_WARNING);
			return;
		}
		// For LIFT import we are using wxFile and we call xml processing functions
		// from XML.cpp
		// BEW 4Jun10, to permit more flexibility (ie. allow LIFT import to the glossingKB
		// as well as to the adapting one), and to use the second param in the call below,
		// because kbv2 CKB class has a member bool m_bGlossingKB which knows whether it
		// is a glossingKB or not - and passing that to XML.cpp permits us to avoid using
		// a global
		bool bReadOK;
		if (m_bGlossingKB)
		{
			bReadOK = ReadLIFT_XML(pathName,m_pApp->m_pGlossingKB);
		}
		else
		{
			bReadOK = ReadLIFT_XML(pathName,m_pApp->m_pKB);
		}
		f.Close();
	}
	else
	{
		// we're importing from an SFM text file (\lx and \ge, and addition data
		// for kbVersion 2)
		wxTextFile file;
		wxLogNull logno; // prevent unwanted system messages
		bool bSuccessful;
		if (!::wxFileExists(pathName))
		{
			bSuccessful = FALSE;
		}
		else
		{
	#ifndef _UNICODE
			// ANSI
			bSuccessful = file.Open(pathName); // read ANSI file into memory
	#else
			// UNICODE
			bSuccessful = file.Open(pathName, wxConvUTF8); // read UNICODE file into memory
	#endif
		}
		if (!bSuccessful)
		{
			// assume there was no configuration file in existence yet, 
			// so nothing needs to be fixed
			wxMessageBox(_("Unable to open import file for reading."));
			return;
		}

		// For SFM import we are using wxTextFile which has already loaded its entire contents
		// into memory with the Open call in OnImportToKb() above. wxTextFile knows how to
		// handle Unicode data and the different end-of-line characters of the different
		// platforms.
		// Since the entire file is now in memory we can read the information by
		// scanning its virtual lines. In this routine we use the "direct access" method of
		// retrieving the lines from storage in memory, using GetLine(ct).
		int lineCount = file.GetLineCount();

		int ct;
		int nWordCount;
		MapKeyStringToTgtUnit* pMap; // pointer to the map to use for a given entry
		CTargetUnit* pTU = NULL;
		CRefString* pRefStr = NULL;
		int numWords = 0;
		for (ct = 0; ct < lineCount; ct++)
		{
			line = file.GetLine(ct);
			// the data for each line is now in lineStr
			// is the line a m_key member?
			if (IsMember(line,keyMarker,nOffset) || nOffset >= 0)
			{
				// it is a valid key
				// default the pMap pointer to the first map in this KB
				pMap = m_pMap[0];
				pTU = NULL;
				pRefStr = NULL;
				bKeyDefined = TRUE;
				nWordCount = 1;
				// extract the actual srcPhrase's m_key from the read in string,
				// to set the key variable
				int keyLen = line.Len();
				keyLen -= (4 + nOffset); // \lx followed by a space = 4 characters,
										 // nOffset takes care of any leading spaces
				if (keyLen > 0)
				{
					key = line.Right(keyLen);
					nWordCount = TrimAndCountWordsInString(key);

					// test we can store it in this KB
					//if (!m_bGlossingKB && (nWordCount > MAX_WORDS)) // BEW removed 13Nov10
					if (nWordCount > MAX_WORDS)
					{
						// error condition for this entry - so ignore it
						key.Empty();
						bKeyDefined = FALSE;
					}
				}
				else
				{
					key.Empty();
					bKeyDefined = FALSE;
				}
				if (bKeyDefined && !key.IsEmpty())
				{
					// define the map to be used, the default is currently
					// the first in the CKB instance, which is correct for a glossingKB
					// but may be wrong for an adaptingKB
					// BEW changed 13Nov10
					//numWords = 1;
					numWords = nWordCount;
					//if (!m_bGlossingKB)
					if (numWords > 1)
					{
						// set pMap correctly
						//numWords = TrimAndCountWordsInString(key);
						pMap = m_pMap[numWords - 1]; // for this record we lookup and store in this map
					}

					// look up the map to see if it has a matching CTargetUnit* instance,
					// & get it if it is there, else return NULL if not
					pTU = GetTargetUnit(numWords, key); // does an AutoCapsLookup()
				}
				else
				{
					pTU = NULL;
				}
			}
			else
			{
				if (IsMember(line,adaptionMarker,nOffset) || nOffset >= 0)
				{
                    // an 'adaptation' member (for a glossingKB, this is actually a gloss,
                    // but we use this adaptation name for both here) exists for this key,
                    // so get the KB updated with this association provided a valid key was
                    // constructed
                    adaption.Empty();
					if (bKeyDefined)
					{
						int adaptionLen = line.Len();
						adaptionLen -= (4 + nOffset); // \ge followed by a space = 4 characters,
													  // nOffset takes care of any leading spaces
						if (adaptionLen > 0)
						{
							adaption = line.Right(adaptionLen);
						}
						else
						{
							adaption.Empty(); // support storing empty strings
						}
						if (pTU == NULL)
						{
                            // there is no CTargetUnit pointer instance in the map for the
                            // given key, so fill out the pRefStr and store pTU in the map
                            // set the pointer to the owning CTargetUnit, and add it to the
                            // m_translations member
							pTU = new CTargetUnit;
							pRefStr = new CRefString; // default constructor doesn't set 
										// CRefStringMetadata members, we will do them
										// explicitly below					
							pRefStr->m_pTgtUnit = pTU;
							pRefStr->m_translation = adaption;
							pRefStr->m_refCount = 1;
							// the next two are defaults, set to the current datetime and
							// the local user's username:machinename; to ensure these
							// members are set in the eventuality that the file being
							// parsed in may be a kbVersion 1 \lx & \ge file (which, of
							// course, as no metadata), but if the file being parsed in is
							// a kbVersion 2 one, then the values in these two members
							// will be overridden by what is read from the file when the
							// \cdt and \wc lines are parsed in
							pRefStr->m_pRefStringMetadata->m_creationDateTime = GetDateTimeNow();
							pRefStr->m_pRefStringMetadata->m_whoCreated = SetWho();
							// append the CRefString to the CTargetUnit's list that is to
							// manage it
							pTU->m_pTranslations->Append(pRefStr);
							
							// so store it in the map (this doesn't stop us from adding
							// metadata strings from subsequent parsed lines)
							pMap = m_pMap[numWords - 1]; // set it again to avoid compiler warning
							if (pMap != NULL)
							{
								(*pMap)[key] = pTU;
							}
						}
						else
						{
                            // there is a CTargetUnit pointer in the map for the given key;
                            // so find out if there is a CRefString instance for the given
                            // adaption value
                            wxASSERT(pTU);
							adaption.Trim();
							adaption.Trim(FALSE);
							CRefString* pRefStr = GetRefString(pTU,adaption); // returns
									// NULL if there was no maatching CRefString instance
							if (pRefStr == NULL)
							{
                                // this particular adaptation or gloss is not yet in the
                                // map's CTargetUnit instance, so put create a CRefString
                                // (and CRefStringMetatdata), and add to the pTU's list
								pRefStr = new CRefString; // default constructor doesn't set 
											// CRefStringMetadata members, we will do them
											// explicitly below				
								pRefStr->m_pTgtUnit = pTU;
								pRefStr->m_translation = adaption;
								pRefStr->m_refCount = 1;
								// the next two are defaults, set to the current datetime and
								// the local user's username:machinename; to ensure these
								// members are set in the eventuality that the file being
								// parsed in may be a kbVersion 1 \lx & \ge file (which, of
								// course, as no metadata), but if the file being parsed in is
								// a kbVersion 2 one, then the values in these two members
								// will be overridden by what is read from the file when the
								// \cdt and \wc lines are parsed in
								pRefStr->m_pRefStringMetadata->m_creationDateTime = GetDateTimeNow();
								pRefStr->m_pRefStringMetadata->m_whoCreated = SetWho();
								// append the CRefString to the CTargetUnit's list that is to
								// manage it
								pTU->m_pTranslations->Append(pRefStr);
							}
							else
							{
                                // this particular adaptation or gloss is in the map's
                                // CTargetUnit pointer already, so we should ignore it; to
                                // do that we just need to set pRefStr to NULL, so that
								// parsing of subsequent metadata lines in the code
								// further down will ignore this CRefString and its metadata
								pRefStr = NULL;
							}
						}
					} // end TRUE block for test: if( bKeyDefined )
				} // end TRUE block for test that the line is a \ge line
				else
				{
					// it's neither a key nor an adaption (or gloss), so check for metadata,
					// but if the required pointer(s) are NULL, skip everything until the
					// next \lx field or next \ge field
					if (pRefStr == NULL || pTU == NULL)
					{
						// ignore anything when pRefSt is NULL, or pTU
						continue; 
					}
					else
					{
						// process any metadata lines
						if (IsMember(line,delflag,nOffset) || nOffset >= 0)
						{
							// this marker, if present, will always have either "0" or "1"
							// as its content
							wxString delStr;
							int delLen = line.Len();
							delLen -= (5 + nOffset); // \del followed by a space = 5 characters,
													 // nOffset takes care of any leading spaces
							if (delLen > 0)
							{
								delStr = line.Right(delLen);
							}
							else
							{
								delStr.Empty();
							}
							if (!delStr.IsEmpty())
							{
								wxASSERT(pRefStr);
								if (delStr.GetChar(0) == _T('1'))
								{
									pRefStr->m_bDeleted = TRUE;
								}
								else
								{
									pRefStr->m_bDeleted = FALSE;
								}
							}
						} // end of TRUE block for test: the line has a \del marker in it
						else if (IsMember(line,whoCr,nOffset) || nOffset >= 0)
						{
							wxString whoCreatedStr;
							int whoCreatedLen = line.Len();
							whoCreatedLen -= (4 + nOffset); // \wC followed by a space = 4 characters,
													 // nOffset takes care of any leading spaces
							if (whoCreatedLen > 0)
							{
								whoCreatedStr = line.Right(whoCreatedLen);
							}
							else
							{
								whoCreatedStr.Empty(); // this member should not be empty
							}
							wxASSERT(pRefStr);
							if (!whoCreatedStr.IsEmpty())
							{
								pRefStr->m_pRefStringMetadata->m_whoCreated = whoCreatedStr;
							}
							else
							{
								// if it is empty, then supply the user:localhost now
								pRefStr->m_pRefStringMetadata->m_whoCreated = SetWho();
							}
						} // end of TRUE block for test: the line has a \cdt marker in it
						else if (IsMember(line,createDT,nOffset) || nOffset >= 0)
						{
							wxString createDTStr;
							int createDTLen = line.Len();
							createDTLen -= (5 + nOffset); // \cdt followed by a space = 5 characters,
													 // nOffset takes care of any leading spaces
							if (createDTLen > 0)
							{
								createDTStr = line.Right(createDTLen);
							}
							else
							{
								createDTStr.Empty(); // this member should not be empty
							}
							wxASSERT(pRefStr);
							if (!createDTStr.IsEmpty())
							{
								pRefStr->m_pRefStringMetadata->m_creationDateTime = createDTStr;
							}
							else
							{
								// if no creation datetime was supplied, give it current datetime
								pRefStr->m_pRefStringMetadata->m_creationDateTime = GetDateTimeNow();
							}
						} // end of TRUE block for test: the line has a \cdt marker in it
						else if (IsMember(line,modDT,nOffset) || nOffset >= 0)
						{
							wxString modDTStr;
							int modDTLen = line.Len();
							modDTLen -= (5 + nOffset); // \mdt followed by a space = 5 characters,
													 // nOffset takes care of any leading spaces
							if (modDTLen > 0)
							{
								modDTStr = line.Right(modDTLen);
							}
							else
							{
								modDTStr.Empty(); // this member can be empty, & usually is
							}
							wxASSERT(pRefStr);
							if (!modDTStr.IsEmpty())
							{
								pRefStr->m_pRefStringMetadata->m_modifiedDateTime = modDTStr;
							}
						} // end of TRUE block for test: the line has a \mdt marker in it
						else if (IsMember(line,delDT,nOffset) || nOffset >= 0)
						{
							wxString delDTStr;
							int delDTLen = line.Len();
							delDTLen -= (5 + nOffset); // \ddt followed by a space = 5 characters,
													 // nOffset takes care of any leading spaces
							if (delDTLen > 0)
							{
								delDTStr = line.Right(delDTLen);
							}
							else
							{
								delDTStr.Empty(); // this member can be empty, & usually is
							}
							wxASSERT(pRefStr);
							if (!delDTStr.IsEmpty())
							{
								pRefStr->m_pRefStringMetadata->m_deletedDateTime = delDTStr;
							}
						} // end of TRUE block for test: the line has a \ddt marker in it
						else
						{
							// it's nothing that we know about, so just ignore this line
							continue; 
						}
					} // end of block for processing any CRefString metadata lines
				}
			} // end of else block for test that line has a \lx marker
		} // end of loop over all lines
		file.Close();
	}

	// process the last line here ??? (FALSE is bool bDoPartnerPileDeletionAlso)
	//m_pApp->GetDocument()->DeleteSingleSrcPhrase(pSrcPhrase, FALSE);
}

// BEW 13Nov10 no changes for all tabs glossing kb
bool CKB::IsMember(wxString& rLine, wxString& rMarker, int& rOffset)
{
	if (rLine.IsEmpty())
	{
		rOffset = -1;
		return FALSE;
	}
	int nFound = rLine.Find(rMarker);
	rOffset = nFound;
	if (nFound == 0)
		return TRUE;
	else
		return FALSE;
}

// returns TRUE if, for the pSrcPhrase->m_key key value, the KB (adaptation KB only, the
// caller will not call this function if the mode is glossing mode) contains a CTargetUnit
// instance which has a CRefString instance which is not storing "<Not In KB>". Else it
// returns FALSE.
// Called in: PlacePhraseBox(), RestoreOriginalMinPhrases() and OnRemoveRetranslation().
// BEW 21Jun10, changed for support of kbVersion 2's m_bDeleted flag; any matches of 
// <Not In KB> are deemed to be non-matches if the storing CRefString instance has its
// m_bDeleted flag set to TRUE 
// BEW 13Nov10, no changes to support Bob Eaton's request for glosssing KB to use all maps
bool CKB::IsItNotInKB(CSourcePhrase* pSrcPhrase)
{
	// determine if it is an <Not In KB> entry in the KB
	// modified, July 2003, for Auto-Capitalization support (ie. use AutoCapsLookup function)
	int nMapIndex = pSrcPhrase->m_nSrcWords - 1; // compute the index to the map

	// if we have too many source words, then it is not in the KB, 
	// but not a "<Not In KB>" entry
	if (pSrcPhrase->m_nSrcWords > MAX_WORDS)
	{
		return FALSE;
	}

	wxString key = pSrcPhrase->m_key;
	CTargetUnit* pTU;
	if (m_pMap[nMapIndex]->empty())
	{
		return FALSE;
	}
	else
	{
		bool bFound = AutoCapsLookup(m_pMap[nMapIndex], pTU, key);
		if(!bFound)
		{
			// not found
			return FALSE;
		}
		else
		{
			// pTU exists, so check its first refString to see if it stores <Not In KB>
			// BEW 21Jun10, changed, because pTU for kbVersion 2 may store one or more
			// deleted CRefString instances besides that which stores a <Not In KB>
			// string, and so we can't rely on <Not In KB> being first in the list
			TranslationsList::Node* tpos = pTU->m_pTranslations->GetFirst();
			CRefString* pRefStr = NULL;
			while (tpos != NULL)
			{
				pRefStr = (CRefString*)tpos->GetData();
				wxASSERT(pRefStr != NULL);
				tpos = tpos->GetNext();
				if (pRefStr->m_translation == _T("<Not In KB>") && !pRefStr->m_bDeleted)
				{
					return TRUE;
				}
			}
			// if control gets to here, there is no non-deleted CRefString instance
			// storing the string <Not In KB>
			return FALSE;
		}
	}
}

// BEW modified 1Sep09 to remove a logic error, & to remove a goto command, and get rid 
// of a bDelayed boolean flag & simplify the logic
// whm modified 3May10 to add LIFT format XML export. 
// BEW 13May10 moved it here from CAdapt_ItView class
// BEW 9Jun10, added markers and code for support of kbVersion 2 data additions
// BEW 13Nov10, changes to support Bob Eaton's request for glosssing KB to use all maps
void CKB::DoKBExport(wxFile* pFile, enum KBExportSaveAsType kbExportSaveAsType)
{
	wxASSERT(pFile != NULL);
	wxString s1 = gSFescapechar;
	wxString lexSFM = s1 + _T("lx ");
	wxString geSFM = s1 + _T("ge ");
	wxString key;
	key.Empty();
	wxString gloss;
	wxString baseKey;
	wxString baseGloss;
	wxString outputSfmStr; // accumulate a whole SFM "record" here, and retain
						   // <Not In KB> entries in this \lx & \ge type of export
	CBString outputLIFTStr; // accumulate a whole LIFT "record" here, but abandon if it
						    // has a <Not In KB> string, we don't export those for LIFT
	wxString strNotInKB = _T("<Not In KB>");
	gloss.Empty(); // this name used for the "adaptation" when adapting,
				   // or the "gloss" when glossing 
	
	// BEW 9Jun10 support for kbVersion 2, uses the custom markers:
	// \del  for the m_bDeleted flag value "0" or "1" values (for FALSE or TRUE)
	// \cdt  for the m_creationDateTime string
	// \mdt  for the m_modifiedDateTime string
	// \ddt  for the m_deletedDateTime string
	// \wc   for the m_whoCreated string
	// As for AI_USFM.xml, any of the above for which the string is empty, will not be
	// included in the output - because defaults will handle those cases at import
	wxString delflag = s1 + _T("del ");
	wxString createDT = s1 + _T("cdt ");
	wxString modDT = s1 + _T("mdt ");
	wxString delDT = s1 + _T("ddt ");
	wxString whoCr = s1 + _T("wc ");

    // MFC's WriteString() function automatically converts any \n embedded within the
    // buffer to the \r\n pair on output. The wxWidgets wxFile::Write() function, however,
    // does not do this so we need to here create the proper EOL sequences in the buffer.
    // The wxWidgets' wxTextFile::GetEOL() static function returns the appropriate
    // end-of-line sequence for the current platform. It would be hex sequence pair 0d 0a
    // for Windows, the single hex char 0a for Unix/Linux, and the single hex char 0d for
    // Macintosh. The GetEOL function provides the correct line termination character(s).
    // wxString eolStr = wxTextFile::GetEOL(); done in the App's OnInit()
	wxASSERT(m_pApp->m_eolStr.Find(_T('\n')) != -1 || m_pApp->m_eolStr.Find(_T('\r')) != -1);

	wxLogNull logNo; // avoid spurious messages from the system

	CBString composeXmlStr;
	CBString indent2sp = "  ";
	CBString indent4sp = "    ";
	CBString indent6sp = "      ";
	CBString indent8sp = "        ";
	CBString indent10sp = "          ";
	CBString guidForThisLexItem;
	CBString srcLangCode;
	CBString tgtLangCode;
	CBString tempCBstr; // use for calling InsertEntities()

	if (kbExportSaveAsType == KBExportSaveAsLIFT_XML)
	{
		// If the ISO639-3 language codes have not previously been entered by the user
		// we need to get them here
		// 
		// Check for existence of language codes. If they already exist we don't need
		// to automatically call up the CLanguageCodesDlg. If they don't exist yet, then
		// we call up the CLanguageCodesDlg here automatically so the user can enter them 
		// - needed for the KB Lift export.
		if (m_pApp->m_sourceLanguageCode.IsEmpty() || m_pApp->m_targetLanguageCode.IsEmpty())
		{
			wxString srcCode;
			wxString tgtCode;
			bool bCodesEntered = FALSE;
			while (!bCodesEntered)
			{
				// Call up CLanguageCodesDlg here so the user can enter language codes for
				// the source and target languages which are needed for the LIFT XML lang attribute of 
				// the <form lang="xxx"> tags (where xxx is a 3-letter ISO639-3 language/Ethnologue code)
				CLanguageCodesDlg lcDlg((wxWindow*)m_pApp->GetMainFrame());
				lcDlg.Center();
				// in this case load any lang codes stored on the App into
				// the code edit boxes
				lcDlg.m_sourceLangCode = m_pApp->m_sourceLanguageCode;
				lcDlg.m_targetLangCode = m_pApp->m_targetLanguageCode;
				int returnValue = lcDlg.ShowModal(); // MFC has DoModal()
				if (returnValue == wxID_CANCEL)
				{
					// user cancelled
					return;
				}
				srcCode = lcDlg.m_sourceLangCode;
				tgtCode = lcDlg.m_targetLangCode;
				// Check that codes have been entered in source and target boxes
				if (srcCode.IsEmpty() || tgtCode.IsEmpty())
				{
					wxString message,langStr;
					if (srcCode.IsEmpty())
					{
						langStr = _("source language code");
					}
					if (tgtCode.IsEmpty())
					{
						if (langStr.IsEmpty())
							langStr = _("target language code");
					}
					else
					{
						langStr += _T('\n');
						langStr += _("target language code");
					}
					message = message.Format(_("You did not enter a language code for the following language(s):\n\n%s\n\nLIFT XML Export requires 3-letter language codes.\nDo you want to try again?"),langStr.c_str());
					int response = wxMessageBox(message, _("Language code(s) missing"), wxYES_NO | wxICON_WARNING);
					if (response == wxNO)
					{
						// user wants to abort 
						return;
					}
					else
					{
						bCodesEntered = FALSE;
					}
				}
				else
				{
					bCodesEntered = TRUE;
				}
			} // while (!bCodesEntered)

			srcLangCode = srcCode.char_str();
			tgtLangCode = tgtCode.char_str();
			// also update the App's members
			m_pApp->m_sourceLanguageCode = srcCode;
			m_pApp->m_targetLanguageCode = tgtCode;
		}
		// get the codes from the App into CBStrings
		srcLangCode = m_pApp->m_sourceLanguageCode.char_str();
		tgtLangCode = m_pApp->m_targetLanguageCode.char_str();
		// whm Note: Following the example of the Doc's DoFileSave() we build the XML LIFT
		// format building our CBString called composeStr with manual concatenation of some
		// string literals plus some methods from XML.cpp
		CBString xmlPrologue;
		// TODO: Check if it is OK to include the standalone="yes" part of the prologue produced
		// by GetEncodingStringForSmlFiles() below as <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
		m_pApp->GetEncodingStringForXmlFiles(xmlPrologue); // builds xmlPrologue and adds "\r\n" to it
		composeXmlStr = xmlPrologue; // first string in xml file
		CBString openLiftTag = "<lift version=\"0.13\">";
		composeXmlStr += openLiftTag;
		composeXmlStr += "\r\n";
		DoWrite(*pFile,composeXmlStr);
	}

	// whm added 14May10 Put up a progress indicator since large KBs can take a noticeable while to export
	// as xml.
	// To get a better progress indicator first get a count of the KB items/entries to be exported
	int nTotal = 0;
	int numWords_sim;
	MapKeyStringToTgtUnit::iterator iter_sim;
	CTargetUnit* pTU_sim = 0;
	for (numWords_sim = 1; numWords_sim <= MAX_WORDS; numWords_sim++)
	{
		// BEW 13Nov10, commented out to support Bob Eaton request for all glossing KB
		// maps to be usable
		//if (m_bGlossingKB && numWords_sim > 1)
		//	continue; // when glossing we want to consider only the first map, the others
		//			  // are all empty
		if (m_pMap[numWords_sim-1]->size() == 0) 
			continue;
		else
		{
			iter_sim = m_pMap[numWords_sim-1]->begin();
			do 
			{
				nTotal++; // add number of <entry>s, i.e., <lexical-unit>s
				pTU_sim = (CTargetUnit*)iter_sim->second; 
				wxASSERT(pTU_sim != NULL);

				nTotal += pTU_sim->m_pTranslations->GetCount(); // add number of <sense>s


				iter_sim++;

			} while (iter_sim != m_pMap[numWords_sim-1]->end());
		}
	}
	
	wxString progMsg = _("%d of %d Total entries and senses");
	wxString msgDisplayed = progMsg.Format(progMsg,1,nTotal);
	wxString titleStr;
	if (kbExportSaveAsType == KBExportSaveAsLIFT_XML)
	{
		titleStr = _("Exporting the KB in LIFT format");
	}
	else
	{
		titleStr = _("Exporting the KB in SFM format");
	}
	wxProgressDialog progDlg(titleStr,
							msgDisplayed,
							nTotal,    // range
							(wxWindow*)m_pApp->GetMainFrame(),   // parent
							//wxPD_CAN_ABORT |
							//wxPD_CAN_SKIP |
							wxPD_APP_MODAL |
							wxPD_AUTO_HIDE //| -- try this as well
							//wxPD_ELAPSED_TIME |
							//wxPD_ESTIMATED_TIME |
							//wxPD_REMAINING_TIME
							//| wxPD_SMOOTH // - makes indeterminate mode bar on WinXP very small
							);

	bool bSuppressDeletionsInSFMexport = FALSE; // default is to export everything for SFM export
	if (kbExportSaveAsType == KBExportSaveAsSFM_TXT)
	{
		wxString message;
		message = message.Format(_("Deleted entries are kept in the knowledge base but are hidden. Do you want these included in the export?\n(Click No only if you intend to later import the data to a legacy version of Adapt It, otherwise click Yes.)"));
		int result = wxMessageBox(message,_("How should deleted entries be handled?"), wxYES_NO | wxICON_QUESTION);
		if (result == wxNO)
		{
			bSuppressDeletionsInSFMexport = TRUE;
		}
	}

	int numWords;
	int counter = 0;
	MapKeyStringToTgtUnit::iterator iter;
	CTargetUnit* pTU = 0;
	CRefString* pRefStr;
	for (numWords = 1; numWords <= MAX_WORDS; numWords++)
	{
		// BEW 13Nov10, removed, to support Bob Eaton's request
		//if (m_bGlossingKB && numWords > 1)
		//	continue; // when glossing we want to consider only the first map, the others
		//			  // are all empty
		if (m_pMap[numWords-1]->size() == 0) 
			continue;
		else
		{
			iter = m_pMap[numWords-1]->begin();
			do 
			{
				counter++;
				outputSfmStr.Empty(); // clean it out ready for next "record"
				outputLIFTStr.Empty(); // clean it out ready for next "record"
				key = iter->first; 
				pTU = (CTargetUnit*)iter->second; 
				wxASSERT(pTU != NULL);
				baseKey = key;

				// get the reference strings
				TranslationsList::Node* posRef = 0; 

				// if the data somehow got corrupted by a CTargetUnit being retained in the
				// list but which has an empty list of reference strings, this illegal
				// instance would cause a crash - so test for it and if such occurs, then
				// remove it from the list and then just continue looping
				if (pTU->m_pTranslations->IsEmpty())
				{
					m_pMap[numWords-1]->erase(baseKey); // the map now lacks this 
														// invalid association
					delete pTU; // its memory chunk is freed (don't leak memory)
					continue;
				}
				else
					posRef = pTU->m_pTranslations->GetFirst(); 
				wxASSERT(posRef != 0);

				counter += pTU->m_pTranslations->GetCount();

				// if control gets here, there will be at least one non-null posRef and so
				// we can now begin to construct the next record, but beware if the user
				// wants suppression of any deleted entries - this could mean the \lx
				// field has to be abandoned if there are no \ge lines to write. 

				// For Sfm data the key line is represented as a \lx source text field,
				// followed by the adaptation or gloss we've already found associated with it
				if (kbExportSaveAsType == KBExportSaveAsSFM_TXT)
				{
					key = lexSFM + key; // we put the proper eol char(s) below when writing
					outputSfmStr = key + m_pApp->m_eolStr;
				}

				if (kbExportSaveAsType == KBExportSaveAsLIFT_XML)
				{
					// build xml composeXmlStr for the <lexical-unit> ... </lexical-unit>
					
					// Get the uuid from the CTargetUnit object using pTU->GetUuid()
					wxString tempGuid = GetUuid();
					const wxCharBuffer buff = tempGuid.utf8_str();
					guidForThisLexItem = buff;
					composeXmlStr = indent2sp;
					composeXmlStr += "<entry guid=\"";
					composeXmlStr += guidForThisLexItem;
					composeXmlStr += "\" id=\"";
#ifdef _UNICODE
					tempCBstr = m_pApp->Convert16to8(baseKey);
#else
					tempCBstr = baseKey.c_str();
#endif
					InsertEntities(tempCBstr);
					composeXmlStr += tempCBstr;
					composeXmlStr += "-";
					composeXmlStr += guidForThisLexItem;
					composeXmlStr += "\">";
					composeXmlStr += "\r\n";
					composeXmlStr += indent4sp; // start building a new composeXmlStr
					composeXmlStr += "<lexical-unit>";
					composeXmlStr += "\r\n";
					composeXmlStr += indent6sp;
					composeXmlStr += "<form lang=\"";
					composeXmlStr += srcLangCode;
					composeXmlStr += "\">";
					composeXmlStr += "\r\n";
					composeXmlStr += indent8sp;
					composeXmlStr += "<text>";
#ifdef _UNICODE
					tempCBstr = m_pApp->Convert16to8(baseKey);
#else
					tempCBstr = baseKey.c_str();
#endif
					InsertEntities(tempCBstr);
					composeXmlStr += tempCBstr;
					composeXmlStr += "</text>";
					composeXmlStr += "\r\n";
					composeXmlStr += indent6sp;
					composeXmlStr += "</form>";
					composeXmlStr += "\r\n";
					composeXmlStr += indent4sp;
					composeXmlStr += "</lexical-unit>";
					composeXmlStr += "\r\n";
					outputLIFTStr += composeXmlStr; //DoWrite(*pFile,composeXmlStr);
				}

				pRefStr = (CRefString*)posRef->GetData();
				posRef = posRef->GetNext(); // prepare for possibility of another CRefString
				wxASSERT(pRefStr != NULL);
				gloss = pRefStr->m_translation;
				baseGloss = gloss;

				if (kbExportSaveAsType == KBExportSaveAsSFM_TXT)
				{
					if (bSuppressDeletionsInSFMexport && pRefStr->m_bDeleted)
					{
						// don't include this one in the export
						;
					}
					else
					{
						gloss = geSFM + gloss; // we put the proper eol char(s) below when writing
						outputSfmStr += gloss + m_pApp->m_eolStr;

						// BEW 9Jun10, add extra data for kbVersion 2
						// first the m_bDeleted flag value
						if (pRefStr->m_bDeleted)
						{
							outputSfmStr += delflag + _T("1") + m_pApp->m_eolStr;
						}
						else
						{
							outputSfmStr += delflag + _T("0") + m_pApp->m_eolStr;
						}
						// next, the m_whoCreated value
						outputSfmStr += whoCr + pRefStr->m_pRefStringMetadata->m_whoCreated + m_pApp->m_eolStr;
						// next, whichever of the three datetime strings which are non-empty
						if (!pRefStr->m_pRefStringMetadata->m_creationDateTime.IsEmpty())
						{
							outputSfmStr += createDT + pRefStr->m_pRefStringMetadata->m_creationDateTime + m_pApp->m_eolStr;
						}
						if (!pRefStr->m_pRefStringMetadata->m_modifiedDateTime.IsEmpty())
						{
							outputSfmStr += modDT + pRefStr->m_pRefStringMetadata->m_modifiedDateTime + m_pApp->m_eolStr;
						}
						if (!pRefStr->m_pRefStringMetadata->m_deletedDateTime.IsEmpty())
						{
							outputSfmStr += delDT + pRefStr->m_pRefStringMetadata->m_deletedDateTime + m_pApp->m_eolStr;
						}
					}
				}

				// reject any xml output which contains "<Not In KB>"
				if (kbExportSaveAsType == KBExportSaveAsLIFT_XML
					&& baseGloss.Find(strNotInKB) == wxNOT_FOUND)
				{
					// build xml composeXmlStr for the <sense> ... </sense>
					composeXmlStr = indent4sp; // start building a new composeXmlStr
					composeXmlStr += "<sense>";
					composeXmlStr += "\r\n";
					composeXmlStr += indent6sp;
					composeXmlStr += "<definition>";
					composeXmlStr += "\r\n";
					composeXmlStr += indent8sp;
					composeXmlStr += "<form lang=\"";
					composeXmlStr += tgtLangCode;
					composeXmlStr += "\">";
					composeXmlStr += "\r\n";
					composeXmlStr += indent10sp;
					composeXmlStr += "<text>";
#ifdef _UNICODE
					tempCBstr = m_pApp->Convert16to8(baseGloss);
#else
					tempCBstr = baseGloss.c_str();
#endif
					InsertEntities(tempCBstr);
					composeXmlStr += tempCBstr;
					composeXmlStr += "</text>";
					composeXmlStr += "\r\n";
					composeXmlStr += indent8sp;
					composeXmlStr += "</form>";
					composeXmlStr += "\r\n";
					composeXmlStr += indent6sp;
					composeXmlStr += "</definition>";
					composeXmlStr += "\r\n";
					composeXmlStr += indent4sp;
					composeXmlStr += "</sense>";
					composeXmlStr += "\r\n";
					outputLIFTStr += composeXmlStr; //DoWrite(*pFile,composeXmlStr);
				}

				// now deal with any additional CRefString instances within the same
				// CTargetUnit instance
				while (posRef != 0)
				{
					pRefStr = (CRefString*)posRef->GetData();
					wxASSERT(pRefStr != NULL); 
					posRef = posRef->GetNext(); // prepare for possibility of yet another
					gloss = pRefStr->m_translation;
					baseGloss = gloss;

					if (kbExportSaveAsType == KBExportSaveAsSFM_TXT)
					{
						if (bSuppressDeletionsInSFMexport && pRefStr->m_bDeleted)
						{
							// don't include this one in the export
							;
						}
						else
						{
							gloss = geSFM + gloss; // we put the proper eol char(s) below when writing
							outputSfmStr += gloss + m_pApp->m_eolStr;

							// BEW 9Jun10, add extra data for kbVersion 2
							// first the m_bDeleted flag value
							if (pRefStr->m_bDeleted)
							{
								outputSfmStr += delflag + _T("1") + m_pApp->m_eolStr;
							}
							else
							{
								outputSfmStr += delflag + _T("0") + m_pApp->m_eolStr;
							}
							// next, the m_whoCreated value
							outputSfmStr += whoCr + pRefStr->m_pRefStringMetadata->m_whoCreated + m_pApp->m_eolStr;
							// next, whichever of the three datetime strings which are non-empty
							if (!pRefStr->m_pRefStringMetadata->m_creationDateTime.IsEmpty())
							{
								outputSfmStr += createDT + pRefStr->m_pRefStringMetadata->m_creationDateTime + m_pApp->m_eolStr;
							}
							if (!pRefStr->m_pRefStringMetadata->m_modifiedDateTime.IsEmpty())
							{
								outputSfmStr += modDT + pRefStr->m_pRefStringMetadata->m_modifiedDateTime + m_pApp->m_eolStr;
							}
							if (!pRefStr->m_pRefStringMetadata->m_deletedDateTime.IsEmpty())
							{
								outputSfmStr += delDT + pRefStr->m_pRefStringMetadata->m_deletedDateTime + m_pApp->m_eolStr;
							}
						}
					}

					// reject any xml output which contains "<Not In KB>"
					// BEW 18Jun10 -- this decision to exclude <Not In KB> from
					// LIFT exports makes good sense - the LIFT file would not
					// know what to make of it, and any 3rd party app that reads
					// the LIFT file with that kind of entry would no doubt choke,
					// but we do permit such data to be export for a \lx & \ge 
					// SFM dictionary record export (see below)
					if (kbExportSaveAsType == KBExportSaveAsLIFT_XML
						&& baseGloss.Find(strNotInKB) == wxNOT_FOUND)
					{
						// build xml composeXmlStr for the <sense> ... </sense>
						composeXmlStr = indent4sp; // start building a new composeXmlStr
						composeXmlStr += "<sense>";
						composeXmlStr += "\r\n";
						composeXmlStr += indent6sp;
						composeXmlStr += "<definition>";
						composeXmlStr += "\r\n";
						composeXmlStr += indent8sp;
						composeXmlStr += "<form lang=\"";
						composeXmlStr += tgtLangCode;
						composeXmlStr += "\">";
						composeXmlStr += "\r\n";
						composeXmlStr += indent10sp;
						composeXmlStr += "<text>";
	#ifdef _UNICODE
						tempCBstr = m_pApp->Convert16to8(baseGloss);
	#else
						tempCBstr = baseGloss.c_str();
	#endif
						InsertEntities(tempCBstr);
						composeXmlStr += tempCBstr;
						composeXmlStr += "</text>";
						composeXmlStr += "\r\n";
						composeXmlStr += indent8sp;
						composeXmlStr += "</form>";
						composeXmlStr += "\r\n";
						composeXmlStr += indent6sp;
						composeXmlStr += "</definition>";
						composeXmlStr += "\r\n";
						composeXmlStr += indent4sp;
						composeXmlStr += "</sense>";
						composeXmlStr += "\r\n";
						outputLIFTStr += composeXmlStr; //DoWrite(*pFile,composeXmlStr);
					}
					// update the progress bar every 20th iteration
					if (counter % 200 == 0)
					{
						msgDisplayed = progMsg.Format(progMsg,counter,nTotal);
						progDlg.Update(counter,msgDisplayed);
					}
				} // end of inner loop for looping over CRefString instances

				if (kbExportSaveAsType == KBExportSaveAsLIFT_XML)
				{
					// add the end tag "</entry>" for this entry
					composeXmlStr = indent2sp;
					composeXmlStr += "</entry>";
					composeXmlStr += "\r\n";
					outputLIFTStr += composeXmlStr; //DoWrite(*pFile,composeXmlStr);
				}
				
				if (kbExportSaveAsType == KBExportSaveAsSFM_TXT)
				{
					// add a blank line for Sfm output for readability
					outputSfmStr += m_pApp->m_eolStr;
				}

				if (kbExportSaveAsType == KBExportSaveAsLIFT_XML)
				{
					// reject any outputSfmStr which contains "<Not In KB>"
					if (outputLIFTStr.Find(strNotInKB.char_str()) == wxNOT_FOUND)
					{
							// the entry is good, so output it
							DoWrite(*pFile,outputLIFTStr);
					}
				}
				if (kbExportSaveAsType == KBExportSaveAsSFM_TXT)
				{

                    // BEW 18Jun10: changed <Not In KB> behaviour to allow such data to be
                    // in the SFM export of the KB, because it is only Adapt It that is
                    // likely to use such an export, and we want all relevant KB data to be
                    // preserved in this type of export, because we can't do so in a LIFT
                    // export
					if (bSuppressDeletionsInSFMexport)
					{
						// check that suppression of deletions did not suppress output of
						// all the \ge fields and their associated data - if that were the
						// case, then we must not output the \lx line as an orphaned field
						if (outputSfmStr.Find(geSFM) != wxNOT_FOUND)
						{
							// there is at least on \ge field, so go ahead and export this
							// record
							#ifndef _UNICODE // ANSI version
							pFile->Write(outputSfmStr); 
							#else // Unicode version
							m_pApp->ConvertAndWrite(wxFONTENCODING_UTF8,pFile,outputSfmStr);
							#endif
						}
					}
					else
					{
						// deletions are to be included in the export, so no check needed
						#ifndef _UNICODE // ANSI version
						pFile->Write(outputSfmStr); 
						#else // Unicode version
						m_pApp->ConvertAndWrite(wxFONTENCODING_UTF8,pFile,outputSfmStr);
						#endif
					}
					// 18Jun10 retain the legacy code for the present
					//if (outputSfmStr.Find(strNotInKB) == wxNOT_FOUND)
					//{
					//		// the entry is good, so output it
					//		#ifndef _UNICODE // ANSI version
					//			pFile->Write(outputSfmStr); 
					//		#else // Unicode version
					//			m_pApp->ConvertAndWrite(wxFONTENCODING_UTF8,pFile,outputSfmStr);
					//		#endif
					//}
				}
				// update the progress bar every 200th iteration
				if (counter % 200 == 0)
				{
					msgDisplayed = progMsg.Format(progMsg,counter,nTotal);
					progDlg.Update(counter,msgDisplayed);
				}
				// point at the next CTargetUnit instance, or at end() (which is NULL) if
				// completeness has been obtained in traversing the map 
				iter++;
			} while (iter != m_pMap[numWords-1]->end());
		} // end of normal situation block ...
	} // end of numWords outer loop
	
	if (kbExportSaveAsType == KBExportSaveAsLIFT_XML)
	{
		// build xml composeXmlStr for the ending </lift> tag
		composeXmlStr = "</lift>"; // start building a new composeXmlStr
		composeXmlStr += "\r\n";
		DoWrite(*pFile,composeXmlStr);
	}

	// remove the progress indicator window
	progDlg.Destroy();
}

// looks up the knowledge base to find if there is an entry in the map with index
// nSrcWords-1, for the key keyStr, and returns the CTargetUnit pointer it finds. If it
// fails, it returns a null pointer.
// version 2.0 and onwards supports glossing too
// BEW 13May10, moved here from CAdapt_ItView and removed pKB param from signature
// BEW 21Jun10: no changes needed for support of kbVersion 2
// BEW 13Nov10, no changes to support Bob Eaton's request for glosssing KB to use all maps
CTargetUnit* CKB::GetTargetUnit(int nSrcWords, wxString keyStr)
{
	MapKeyStringToTgtUnit* pMap = m_pMap[nSrcWords-1];
	wxASSERT(pMap != NULL);
	CTargetUnit* pTgtUnit;
	bool bOK = AutoCapsLookup(pMap, pTgtUnit, keyStr);
	if (bOK)
	{
		wxASSERT(pTgtUnit);
		return pTgtUnit; // we found it
	}
    // lookup failed, so the KB state is different than data in the document suggests, a
    // Verify operation should be done on the file(s)
	return (CTargetUnit*)NULL;
}

// This is the inner workings of the handler OnCheckKBSave() -- the latter being called
// when the user clicks the GUI checkbox "Save to knowledge base" (the latter is checked
// by default, it takes a user click to uncheck it, and that results in "<Not In KB>"
// replacing the adaptations (or glosses) for the CTargetUnit instance associated with the
// source text key at that location. The other function which calls DoNotInKB() is the
// call of RedoStorage().
// BEW 18Jun10, changes made for supporting kbVersion 2
// BEW 13Nov10, no changes to support Bob Eaton's request for glosssing KB to use all maps
void CKB::DoNotInKB(CSourcePhrase* pSrcPhrase, bool bChoice)
{
	if (bChoice)
	{
        // user wants it to not be in the KB, so set it up accordingly... first thing to do
        // is to remove all existing translations from this source phrase's entry in the KB
        // BEW 18Jun10, for kbVersion 2, they are not physically removed, but m_bDeleted is
        // set TRUE for each, and m_deletedDateTime member gets the current datetime value
		CTargetUnit* pTgtUnit = GetTargetUnit(pSrcPhrase->m_nSrcWords,pSrcPhrase->m_key);
		if (pTgtUnit != NULL)
		{
			pTgtUnit->DeleteAllToPrepareForNotInKB();
		}

		// we make it's KB translation be a unique "<Not In KB>" - 
		// Adapt It will use this as a flag
		pSrcPhrase->m_bNotInKB = FALSE; // temporarily set FALSE to allow 
										// the string to go into KB
		wxString str = _T("<Not In KB>");
		bool bOK;
		bOK = StoreText(pSrcPhrase,str);

		// make the flags the correct values & save them on the source phrase
		pSrcPhrase->m_bNotInKB = TRUE;
		pSrcPhrase->m_bHasKBEntry = FALSE;

        // user can set pSrcPhrase->m_adaption to whatever he likes via phrase box, it
        // won't go into KB, and it now (no longer) will get clobbered, since we now follow
        // Susanna Imrie's recommendation that this feature should still allow a non-null
        // translation to remain in the document
	}
	else
	{
		// make translations storable from now on
		pSrcPhrase->m_bNotInKB = FALSE; // also permits finding of KB entry
		pSrcPhrase->m_bHasKBEntry = FALSE; // make sure

		wxString str = _T("<Not In KB>");
		CRefString* pRefString = GetRefString(pSrcPhrase->m_nSrcWords,
												pSrcPhrase->m_key,str);
		if (pRefString == NULL)
		{
            // it's not present, so our work is done
			return;
		}
		wxASSERT(pRefString);
		if (pRefString != NULL)
		{
			// BEW 18Jun10, for kbVersion 2, we must undo any deletions we made earlier
			// when we set up the <Not In KB> entry, and make the CRefString with
			// translation test "<Not In KB>" become the deleted one
			CTargetUnit* pTgtUnit = pRefString->m_pTgtUnit;
			wxASSERT(pTgtUnit);
			TranslationsList* pList = pTgtUnit->m_pTranslations;
			wxASSERT(!pList->IsEmpty());
			TranslationsList::Node* pos = pList->GetFirst();
			// BEW 18Jun10 the new code follows..., first, scan through all CRefString
			// instances in the list and any with m_bDeleted set TRUE, undelete them
			while (pos != NULL)
			{
				CRefString* pRefStr = (CRefString*)pos->GetData();
				pos = pos->GetNext();
				if (pRefStr != NULL && pRefStr->m_bDeleted)
				{
					pRefStr->m_bDeleted = FALSE;
					pRefStr->m_pRefStringMetadata->m_deletedDateTime.Empty();
				}
			}
			
			// finally make the <Not In KB> entry become the deleted one
			pRefString->m_bDeleted = TRUE;
			pRefString->m_pRefStringMetadata->m_deletedDateTime = GetDateTimeNow();
		}
	}
}

inline int CKB::GetCurrentKBVersion()
{
	return m_kbVersionCurrent;
}

inline void CKB::SetCurrentKBVersion()
{
	m_kbVersionCurrent = (int)KB_VERSION2; // KB_VERSION2 is defined in Adapt_ItConstants.h
}

bool CKB::IsThisAGlossingKB() { return m_bGlossingKB; }

////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      pKeys   -> pointer to the list of keys that with m_bAlwaysAsk TRUE
/// \remarks
/// Called from: the App's OnFileRestoreKb().
/// Uses the list of the keys which have a unique translation and for which the CTargetUnit
/// instance's m_bAlwaysAsk attribute was set to TRUE (the list was previously generated by
/// a call to the App's GetForceAskList() function), and insures that the appropriate
/// target unit's m_bAlwaysAsk member is set to TRUE.
/// BEW 21Jun10, no changes needed for support of kbVersion 2
/// BEW 13Nov10, no changes to support Bob Eaton's request for glosssing KB to use all maps
////////////////////////////////////////////////////////////////////////////////////////
void CKB::RestoreForceAskSettings(KPlusCList* pKeys)
{
	wxASSERT(pKeys);
	if (pKeys->IsEmpty())
		return;
	KPlusCList::Node *node = pKeys->GetFirst();
	while (node)
	{
		KeyPlusCount* pK = (KeyPlusCount*)node->GetData();
		wxASSERT(pK != NULL);
		int index = pK->count - 1; // index to map
		MapKeyStringToTgtUnit* pMap = m_pMap[index];
		wxASSERT(pMap);
		CTargetUnit* pTU = (CTargetUnit*)NULL;
        // when the RestoreForceAskSettings function is called, the KB (glossing or
        // adaptation) will have just been recreated from the documents. This recreation
        // will be done with auto-capitalization either On or Off, with differing results,
        // so the lookup done in the next line has to also be conformant with the current
        // value of the gbAutoCaps flag.
		bool bOK = AutoCapsLookup(pMap,pTU,pK->key);
		if (bOK)
		{
			wxASSERT(pTU);
			pTU->m_bAlwaysAsk = TRUE; // restore the flag setting
		}
		else
		{
			// this error is unlikely to occur, so we can leave it in English
			wxString str;
			str = str.Format(_T(
"Error (non fatal): did not find an entry for the key:  %s  in the map with index %d\n"),
			pK->key.c_str(), pK->count - 1);
			wxMessageBox(str, _T(""), wxICON_EXCLAMATION);
		}
		delete pK; // no longer needed, so ensure we don't leak memory
		pK = (KeyPlusCount*)NULL;

		node = node->GetNext();
	}
	pKeys->Clear(); // get rid of the now hanging pointers
}

// like StoreAdaption, but with different assumptions since we need to be able to move back
// when either there is nothing in the current phraseBox (in which case no store need be
// done), or when the user has finished typing the current srcPhrase's adaption (since it
// will be saved to the KB when focus moves back.) TRUE if okay to go back, FALSE
// otherwise. For glossing, pKB must point to the glossing KB, for adapting, to the normal
// KB.
// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 14May10, moved to here from CAdapt_ItView class, and removed pKB param from signature
// BEW 4Jun10, updated to support kbVersion 2
// BEW 13Nov10, changes to support Bob Eaton's request for glosssing KB to use all maps,
// including calling IsFixedSpaceSymbolWithin() to force ~ conjoinings to be stored in
// map 1 rather than map 2.
bool CKB::StoreTextGoingBack(CSourcePhrase *pSrcPhrase, wxString &tgtPhrase)
{
	// determine the auto caps parameters, if the functionality is turned on
	bool bNoError = TRUE;
	if (gbAutoCaps)
	{
		bNoError = m_pApp->GetDocument()->SetCaseParameters(pSrcPhrase->m_key); // for source word or phrase
	}

	m_pApp->GetDocument()->Modify(TRUE);

    // do not permit storage if the source phrase has an empty key (eg. user may have ...
    // ellipsis in the source text, which generates an empty key and three periods in the
    // punctuation)
	if (pSrcPhrase->m_key.IsEmpty())
	{
		gbMatchedKB_UCentry = FALSE;
		return TRUE; // this is not an error, just suppression of the store
	}

	gbByCopyOnly = FALSE; // restore default setting

	// is the m_targetPhrase empty?
	if (tgtPhrase.IsEmpty())
	{
		// it's empty, so we can go back without saving anything in the kb
		m_pApp->m_bForceAsk = FALSE; // must ensure this flag is off, no forcing of 
						// Choose Translation dialog is required when moving back
		gbMatchedKB_UCentry = FALSE;
		return TRUE;
	}

    // It's not empty, so go ahead and re-store it as-is (but if auto capitalization has
    // just been turned on, it will be stored as a lower case entry if it is an upper case
    // one in the doc) first, remove any phrase final space characters
	if (!tgtPhrase.IsEmpty())
	{
		tgtPhrase.Trim();
	}

	// always place a copy in the source phrase's m_adaption member, etc
	if (m_bGlossingKB)
	{
		wxString s = tgtPhrase;
		if (gbAutoCaps)
		{
			bool bNoError = TRUE;
			if (gbSourceIsUpperCase && !gbMatchedKB_UCentry)
			{
				bNoError = m_pApp->GetDocument()->SetCaseParameters(s,FALSE);
				if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
				{
					// change it to upper case
					s.SetChar(0,gcharNonSrcUC);
				}
			}
		}
		pSrcPhrase->m_gloss = s;
		// BEW 13Nov10, the flag below is never set TRUE so remove this bit of code
		//if (gbRemovePunctuationFromGlosses)
		//	m_pApp->GetView()->RemovePunctuation(m_pApp->GetDocument(),&pSrcPhrase->m_gloss,from_target_text);
	}
	else
	{
		if (tgtPhrase != _T("<Not In KB>"))
		{
			pSrcPhrase->m_adaption = tgtPhrase;
			if (!gbInhibitMakeTargetStringCall)
			{
				// sets m_targetStr member too, and handles auto-capitalization
				m_pApp->GetView()->MakeTargetStringIncludingPunctuation(pSrcPhrase, tgtPhrase);
			}
		}
	}
	
    // if the user doesn't want a store done (he checked the dialog bar's control for this
    // choice) then return without saving after setting the source phrase's m_bNotInKB flag
    // to TRUE
	int nMapIndex;
	if (!m_bGlossingKB && !m_pApp->m_bSaveToKB)
	{
		pSrcPhrase->m_bNotInKB = TRUE;
		m_pApp->m_bForceAsk = FALSE; // its a valid 'store op' so must turn this flag back off
		pSrcPhrase->m_bHasKBEntry = FALSE;
		gbMatchedKB_UCentry = FALSE;
		return TRUE; // we want the caller to think all is well
	}
	else // adapting or glossing
	{
		// BEW changed, 13Nov10, to support Bob Eaton's request for a 10map glossing KB
		//if (m_bGlossingKB)
		//{
			//nMapIndex = 0;
		//	nMapIndex = pSrcPhrase->m_nSrcWords - 1; // compute the index to the map
		//}
		//else
		//{
			if (IsFixedSpaceSymbolWithin(pSrcPhrase))
			{
				nMapIndex = 0;
			}
			else
			{
				nMapIndex = pSrcPhrase->m_nSrcWords - 1; // index to the appropriate map
			}
			//nMapIndex = pSrcPhrase->m_nSrcWords - 1; // compute the index to the map
		//}
	}
	// if there is a CTargetUnit associated with the current key, then get it; if not,
	// create one and add it to the appropriate map

    // if we have too many source words, then we cannot save to the KB, so detect this and
    // warn the user that it will not be put in the KB, then return TRUE since all is
    // otherwise okay; this check need be done only when adapting
	//if (!m_bGlossingKB && pSrcPhrase->m_nSrcWords > MAX_WORDS) << BEW removed 13Nov10
	if (pSrcPhrase->m_nSrcWords > MAX_WORDS)
	{
		pSrcPhrase->m_bNotInKB = TRUE;
		if (m_bGlossingKB)
			pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
		else
			pSrcPhrase->m_bHasKBEntry = FALSE;
		wxMessageBox(_(
"Warning: there are too many source language words in this phrase for this adaptation to be stored in the knowledge base.")
		, _T(""), wxICON_INFORMATION);
		gbMatchedKB_UCentry = FALSE;
		return TRUE;
	}

	// continue the storage operation
	wxString unchangedkey = pSrcPhrase->m_key; // this never has case change done to it
											  // (need this for lookups)
	wxString key = AutoCapsMakeStorageString(pSrcPhrase->m_key); // key might be made lower case
	CTargetUnit* pTU;
	CRefString* pRefString;
	if (m_pMap[nMapIndex]->empty()) 
	{
		pTU = new CTargetUnit;
		wxASSERT(pTU != NULL);
		pRefString = new CRefString(pTU); // also creates CRefStringMetadata with creation
										  // datetime and m_whoCreated members set
		wxASSERT(pRefString != NULL);

		pRefString->m_refCount = 1; // set the count
		// add the translation string, or gloss string
		if (bNoError)
		{
			pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);
		}
		else
		{
			// if something went wrong, just save as if gbAutoCaps was FALSE
			pRefString->m_translation = tgtPhrase;
		}
		pTU->m_pTranslations->Append(pRefString); // store in the CTargetUnit
		if (m_pApp->m_bForceAsk)
			pTU->m_bAlwaysAsk = TRUE; // turn it on if user wants to be given 
				// opportunity to add a new refString next time its matched
		if (m_bGlossingKB)
			pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
		else
			pSrcPhrase->m_bHasKBEntry = TRUE;

		(*m_pMap[nMapIndex])[key] = pTU; // store the CTargetUnit instance in the map
		// update the maxWords limit
		// BEW changed 13Nov10 to support Bob Eaton's request for 10map glossing KB
		//if (m_bGlossingKB)
		//{
		//	m_nMaxWords = 1; // always 1 when glossing (ensures glossing 
		//		// ignores maps with indices from 1 to 9; all is in 1st map only)
		//}
		//else
		//{
			if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
				m_nMaxWords = pSrcPhrase->m_nSrcWords;
		//}
	}
	else // do next block when the map is not empty
	{
        // there might be a pre-existing association between this key and a CTargetUnit, 
        // so check it out
		bool bFound = AutoCapsLookup(m_pMap[nMapIndex], pTU, unchangedkey); 

        // if not found, then create a targetUnit, and add the refString, etc, as above;
        // but if one is found, then check whether we add a new refString or increment the
        // refCount of an existing one
		if(!bFound)
		{
			pTU = new CTargetUnit;
			wxASSERT(pTU != NULL);
			pRefString = new CRefString((CTargetUnit*)pTU); // also creates CRefStringMetadata
									// with m_creationDateTime and m_whoCreated members set
			wxASSERT(pRefString != NULL);

			pRefString->m_refCount = 1; // set the count
			pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE); // FALSE
								// is value of bIsSrc (auto-caps needs to know whether the
								// string is source text versus adaptation (or gloss) text
			pTU->m_pTranslations->Append(pRefString); // store in the CTargetUnit
			if (m_pApp->m_bForceAsk)
				pTU->m_bAlwaysAsk = TRUE; // turn it on if user wants to be given 
						// opportunity to add a new refString next time its matched
			if (m_bGlossingKB)
				pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
			else
				pSrcPhrase->m_bHasKBEntry = TRUE;

			(*m_pMap[nMapIndex])[key] = pTU;// store the CTargetUnit instance in the map 
			// update the maxWords limit
			// BEW changed 13Nov10 to support Bob Eaton's request for 10map glossing KB
			//if (m_bGlossingKB)
			//{
			//	m_nMaxWords = 1; // for glossing it is always 1
			//}
			//else
			//{
				if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
					m_nMaxWords = pSrcPhrase->m_nSrcWords;
			//}
		}
		else // we found one
		{
            // we have a pTU for this key, so check for a matching CRefString, if there is
            // no match, then add a new one (note: no need to set m_nMaxWords for this
            // case)
			bool bMatched = FALSE;
			pRefString = new CRefString(pTU);
			wxASSERT(pRefString != NULL);
			pRefString->m_refCount = 1; // set the count, assuming this will be stored 
										// (it may not be)
            // set its gloss or adaptation string; the fancy test is required because the
            // refStr entry may have been stored in the kb when auto-caps was off, and if
            // it was upper case for the source text's first letter, then it will have been
            // looked up only on the second attempt, for which gbMatchedKB_UCentry will
            // have been set TRUE, and which means the gloss or adaptation will not have
            // been made lower case - so we must allow for this possibility
			if ((gbAutoCaps && gbMatchedKB_UCentry) || !gbAutoCaps)
				pRefString->m_translation = tgtPhrase; // use unchanged string, could be uc
			else
				pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);

			TranslationsList::Node* pos = pTU->m_pTranslations->GetFirst();
			while (pos != NULL)
			{
				CRefString* pRefStr = (CRefString*)pos->GetData();
				pos = pos->GetNext();
				wxASSERT(pRefStr != NULL);

				// does it match?
				if (*pRefStr == *pRefString) // TRUE if pRStr->m_translation ==
											 //				pRefString->m_translation
				{
					// if we get a match, then increment ref count and point to this, etc
					bMatched = TRUE;
					if (pRefStr->m_bDeleted)
					{
						// we've matched a deleted entry, so we must undelete it
						pRefStr->m_bDeleted = FALSE;
						pRefStr->m_refCount = 1;
						pRefStr->m_pRefStringMetadata->m_creationDateTime = GetDateTimeNow();
						pRefStr->m_pRefStringMetadata->m_deletedDateTime.Empty();
						pRefStr->m_pRefStringMetadata->m_modifiedDateTime.Empty();
						// in next call, param bool bOriginatedFromTheWeb is default FALSE
						pRefStr->m_pRefStringMetadata->m_whoCreated = SetWho();
					}
					else
					{
						pRefStr->m_refCount++;
					}
					if (m_bGlossingKB)
						pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
					else
						pSrcPhrase->m_bHasKBEntry = TRUE;

					// delete the just created pRefString as we won't use it
					pRefString->DeleteRefString(); // also deletes its CMetadata
					pRefString = (CRefString*)NULL;
					// ensure the user's setting is retained for 'force choice for this item'
					if (m_pApp->m_bForceAsk)
					{
						pTU->m_bAlwaysAsk = TRUE; // nTrCount might be 1, so we must 
								// ensure it gets set if that is what the user wants
					}
					break;
				} // end of block for processing a matched CRefString entry
			} // end of while loop
            // if we get here with bMatched == FALSE, then there was no match, so we must
            // add the new pRefString to the CTargetUnit instance
			if (!bMatched)
			{
				TranslationsList::Node* tpos = pTU->m_pTranslations->GetFirst();
				CRefString* pRefStr = (CRefString*)tpos->GetData();
				if (!m_bGlossingKB && pRefStr->m_translation == _T("<Not In KB>"))
				{
                    // keep it that way (the way to cancel this setting is with the toolbar
                    // checkbox) But leave m_adaption and m_targetStr (or m_gloss) having
                    // whatever the user may have typed
					pSrcPhrase->m_bHasKBEntry = FALSE;
					pSrcPhrase->m_bNotInKB = TRUE;
					pSrcPhrase->m_bBeginRetranslation = FALSE;
					pSrcPhrase->m_bEndRetranslation = FALSE;
					pRefString->DeleteRefString(); // don't leak memory
					pRefString = (CRefString*)NULL;
					m_pApp->m_bForceAsk = FALSE;
					gbMatchedKB_UCentry = FALSE;
					return TRUE; // all is well
				}
				else // either we are glossing, or we are adapting and it's a normal adaptation
				{
					// is the m_targetPhrase empty?
					if (tgtPhrase.IsEmpty())
					{
						// don't store if it is empty, and then return; but if not empty
						// then go on to do the storage
						if (!m_bGlossingKB)
						{
							pSrcPhrase->m_bBeginRetranslation = FALSE;
							pSrcPhrase->m_bEndRetranslation = FALSE;
						}
						m_pApp->m_bForceAsk = FALSE; // make sure it's turned off
						pRefString->DeleteRefString(); // don't leak the memory
						pRefString = (CRefString*)NULL;
						gbMatchedKB_UCentry = FALSE;
						return TRUE; // make caller think all is well
					}

					// recalculate the string to be stored, in case we looked up a
					// stored upper case entry earlier
					pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);
					pTU->m_pTranslations->Append(pRefString); 
					if (m_bGlossingKB)
						pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
					else
						pSrcPhrase->m_bHasKBEntry = TRUE;
				}
			}
		}
	}
	if (!m_bGlossingKB)
		pSrcPhrase->m_bNotInKB = FALSE; // ensure correct flag value, 
										// in case it was not in KB
	m_pApp->m_bForceAsk = FALSE; // must be turned off, as it applies 
							   // to one store operation only
	gbMatchedKB_UCentry = FALSE;
	return TRUE;
}

// return TRUE if all was well, FALSE if unable to store (the caller should use the FALSE
// value to block a move of the phraseBox to another pile) This function's behaviour was
// changed after version1.2.8, on May 6 2002, in order to eliminate the occurence of the
// "Empty Adaption Dialog" which would come up whenever the user deleted the contents of
// the phrase box and then moved on, or clicked elsewhere. The new default behaviour is
// that if the box is empty when the user causes it to move, then nothing is stored in the
// knowledge base and the move is acted on immediately. If the user wants to store a <no
// adaptation> empty entry in the KB, a button "<no adaptation>" has been provided on the
// control bar. Click it before moving the phrase box to cause an empty adaptation to be
// stored in the KB for the source text at the current active location. version 2.0 and
// onward tests m_bGlossingKB for storing to the appropriate KB, etc. For adaptation, on
// input the tgtPhrase parameter should have the text with punctuation removed, so this is
// typically done in the caller with a call to RemovePunctuation( ). For versions prior to
// 4.1.0, in order to support the user overriding the stored punctuation for the source
// text, a call to MakeTargetStringIncludingPunctuation( ) is done in the caller, and then
// RemovePunctuation() is called in the caller, so a second call of
// MakeTargetStringIncludingPunctuation( ) within StoreText( ) is not required in this
// circumstance - in this case, a global boolean gbInhibitMakeTargetStringCall is used to
// jump the call within StoreText( ). For 4.1.0 and later,
// MakeTargetStringIncludingPunctuation() is not now called. See below.
// 
// Ammended, July 2003, for Auto-Capitalization support
// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 13May10, moved to here from CAdapt_ItView class, and removed pKB param from signature
// BEW 4Jun10, updated to support kbVersion 2
// BEW 13Nov10, changes to support Bob Eaton's request for glosssing KB to use all maps
// including calling IsFixedSpaceSymbolWithin() to force ~ conjoinings to be stored in
// map 1 rather than map 2.
bool CKB::StoreText(CSourcePhrase *pSrcPhrase, wxString &tgtPhrase, bool bSupportNoAdaptationButton)
{
	// determine the auto caps parameters, if the functionality is turned on
	bool bNoError = TRUE;

    // do not permit storage if the source phrase has an empty key (eg. user may have ...
    // ellipsis in the source text, which generates an empty key and three periods in the
    // punctuation)
	if (pSrcPhrase->m_key.IsEmpty())
	{
		gbMatchedKB_UCentry = FALSE;
		return TRUE; // this is not an error, just suppression of the store
	}

	if (gbAutoCaps)
	{
		bNoError = m_pApp->GetDocument()->SetCaseParameters(pSrcPhrase->m_key); // for source word or phrase
	}

    // If the source word (or phrase) has not been previously encountered, then
    // m_bHasKBEntry (or the equiv flag if glossing is ON) will be false, which has to be
    // true for the StoreText call not to fail. But if we have come to an entry in the KB
    // which we are about to add a new adaptation (or gloss) to it, then the flag will be
    // TRUE (and rightly so.) The StoreText call would then fail - so we will test for this
    // possibility and clear the appropriate flag if necessary.
    // BEW 05July2006: No! The above comment confuses the KB entry with the CSourcePhrase
    // instance at the active location. The flags we are talking about declare that that
    // PARTICULAR instance in the DOCUMENT does, or does not, yet have a KB entry. When the
    // phrase box lands there, it gets its KB entry removed (or ref count decremented)
    // before a store is done, and so the flags are made false when the former happens.
    // RemoveRefString() was supposed to do that, but my logic error was there as well
    // (I've fixed it now). So before the store it done, the RemoveRefString call will now
    // clear the relevant flag; so we don't need to test and clear it in the following
    // block. (block below removed, but the comments are retained in case useful)
    // 
	// BEW 1Jun10: Okay, despite the above two comments, I still want some safety first
	// protection here. Why? Because the code which calls RefmoveRefString() and/or
	// GetAndRemoveRefString() will skip these calls if a CRetranslation class member
	// boolean is left TRUE - namely, m_bSuppressRemovalOfRefString, after being used for
	// suppression. So 'just in case'....
	if (m_bGlossingKB)
	{
		if (pSrcPhrase->m_bHasGlossingKBEntry)
		{
			pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
		}
	}
	else
	{
		if (pSrcPhrase->m_bHasKBEntry)
		{
			pSrcPhrase->m_bHasKBEntry = FALSE;
		}
	}
	m_pApp->GetDocument()->Modify(TRUE);

    // BEW added 20Apr06, to store <Not In KB> when gbSuppressStoreForAltBackspaceKeypress
    // flag is TRUE - as wanted by Bob Eaton; we support this only in adapting mode, not
    // glossing mode
	if (!m_bGlossingKB && gbSuppressStoreForAltBackspaceKeypress)
	{
		wxString strNot = _T("<Not In KB>");
		// rest of this block's code is a simplification of code from later in StoreText()
		int nMapIndex;
		// BEW 13Nov10 support Bob Eaton's request that glossing KB uses all maps
//		if (m_bGlossingKB)
//		{
			//nMapIndex = 0; // always an index of zero when glossing 
//			nMapIndex = pSrcPhrase->m_nSrcWords - 1; // index to the appropriate map
//		}
//		else
//		{
			if (IsFixedSpaceSymbolWithin(pSrcPhrase))
			{
				nMapIndex = 0;
			}
			else
			{
				nMapIndex = pSrcPhrase->m_nSrcWords - 1; // index to the appropriate map
			}
//		}

        // if we have too many source words, then we cannot save to the KB, so beep
		//if (!m_bGlossingKB && pSrcPhrase->m_nSrcWords > MAX_WORDS)
		if (pSrcPhrase->m_nSrcWords > MAX_WORDS)
		{
			::wxBell();
			return TRUE;
		}

		// continue the storage operation
		wxString unchangedkey = pSrcPhrase->m_key;
		wxString key = AutoCapsMakeStorageString(pSrcPhrase->m_key); // key might be made lower case
		CTargetUnit* pTU;
		CRefString* pRefString;
		if (m_pMap[nMapIndex]->empty())
		{
			pTU = new CTargetUnit;
			pRefString = new CRefString(pTU); // the pTU argument sets the m_pTgtUnit member, and
								// creating a CRefString automatically creates a CRefStringMetadata
								// instance & sets its m_creationDateTime and m_whoCreated members
			pRefString->m_refCount = 1; // set the count
			pRefString->m_translation = strNot; // set to "<Not In KB>"
			pTU->m_pTranslations->Append(pRefString); // store in the CTargetUnit

			// <Not In KB> in transliterate mode isn't to be regarded as an entry, but
			// as a trigger for using the transliterator instead of the KB
			pSrcPhrase->m_bHasKBEntry = FALSE; // it's not a 'real' entry
			pSrcPhrase->m_bNotInKB = TRUE;
			pSrcPhrase->m_bBeginRetranslation = FALSE;
			pSrcPhrase->m_bEndRetranslation = FALSE;

			(*m_pMap[nMapIndex])[key] = pTU;
			if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
			{
				m_nMaxWords = pSrcPhrase->m_nSrcWords;
			}
		}
		else // this block for when map is not empty
		{
			// there might be a pre-existing association between this key and a CTargetUnit,
			// so check it out
			bool bFound = AutoCapsLookup(m_pMap[nMapIndex], pTU, unchangedkey);

			// we won't check we have a valid pTU .. so a block of code omitted here

            // if not found on the lookup, then create a targetUnit, and add the refString,
            // etc, but if one is found, then check whether we add a new refString or
            // increment the refCount of an existing one
			if(!bFound)
			{
				pTU = new CTargetUnit;
				pRefString = new CRefString(pTU);
				pRefString->m_refCount = 1; // set the count
				pRefString->m_translation = strNot;
				pTU->m_pTranslations->Append(pRefString); // store in the CTargetUnit

				// <Not In KB> in transliterate mode isn't to be regarded as an entry, but
				// as a trigger for using the transliterator instead of the KB
				pSrcPhrase->m_bHasKBEntry = FALSE;
				pSrcPhrase->m_bNotInKB = TRUE;
				(*m_pMap[nMapIndex])[key] = pTU; // store in map
				if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
				{
					m_nMaxWords = pSrcPhrase->m_nSrcWords;
				}
			}
			else // we found one
			{
                // we found a pTU for this key, so check for a matching CRefString, if
                // there is no match, then add a new one (note: no need to set m_nMaxWords
                // for this case)
				bool bMatched = FALSE;
				pRefString = new CRefString(pTU);
				pRefString->m_refCount = 0; // set the count, assuming this will be
								// stored (it may not be), count is 0 for <Not In KB>
				pRefString->m_translation = strNot; // store "<Not In KB>" since we are
													// in transliterate mode
				TranslationsList::Node* pos = pTU->m_pTranslations->GetFirst();
				while (pos != NULL)
				{
					CRefString* pRefStr = (CRefString*)pos->GetData();
					pos = pos->GetNext();
					wxASSERT(pRefStr != NULL);

					// does it match?
					if (*pRefStr == *pRefString) // TRUE if pRStr->m_translation == 
												 //         pRefString->m_translation
					{
						// if we get a match, then increment ref count and point to this,
						// etc 
						bMatched = TRUE;
						pRefStr->m_refCount++;

                        // <Not In KB> in transliterate mode isn't to be regarded as an
                        // entry, but as a trigger for using the transliterator instead of
                        // the KB
						pSrcPhrase->m_bHasKBEntry = FALSE;
						pSrcPhrase->m_bNotInKB = TRUE;
						pSrcPhrase->m_bBeginRetranslation = FALSE;
						pSrcPhrase->m_bEndRetranslation = FALSE;
						
						pRefString->DeleteRefString(); // don't need this new one
						break;
					}
				}
                // if we get here with bMatched == FALSE, then there was no match, so this
                // must somehow be a normal entry, so we don't add anything and just return
				return TRUE;
			}
		}
		return TRUE;
	} // end of block for processing a store when transliterating using SILConverters transliterator

	gbByCopyOnly = FALSE; // restore default setting

	// First get rid of final spaces, if tgtPhrase has content
	if (!tgtPhrase.IsEmpty())
	{
		tgtPhrase.Trim();
	}

    // always place a copy in the source phrase's m_adaption member, unless it is 
    // <Not In KB>; when glossing always place a copy in the m_gloss member
	if (m_bGlossingKB)
	{
		wxString s = tgtPhrase;
		if (gbAutoCaps)
		{
			bool bNoError = TRUE;
			if (gbSourceIsUpperCase && !gbMatchedKB_UCentry)
			{
				bNoError = m_pApp->GetDocument()->SetCaseParameters(s,FALSE);
				if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
				{
					// change it to upper case
					s.SetChar(0,gcharNonSrcUC);
				}
			}
		}
		pSrcPhrase->m_gloss = s;
	}
	else // currently adapting
	{
		if (tgtPhrase != _T("<Not In KB>"))
		{
			pSrcPhrase->m_adaption = tgtPhrase;
			if (!gbInhibitMakeTargetStringCall)
			{
				// sets m_targetStr member too, also does auto-capitalization adjustments
				m_pApp->GetView()->MakeTargetStringIncludingPunctuation(pSrcPhrase, tgtPhrase);
			}
		}
	} 
    // if the source phrase is part of a retranslation, we allow updating of the m_adaption
    // attribute only (since this is the only place where retranslations are stored), but
    // suppress saving to the KB. For support of glossing, we must skip this block if
    // glossing is ON because glossing does not care about retranslations - the user needs
    // to be able to gloss the source words in a retranslation just like the rest of it
	if (!m_bGlossingKB && pSrcPhrase->m_bRetranslation)
	{
		if (!m_pApp->m_bSaveToKB)
			m_pApp->m_bSaveToKB = TRUE; // ensure this flag is turned back on
		m_pApp->m_bForceAsk = FALSE; // also must be cleared prior to next save attempt
		pSrcPhrase->m_bHasKBEntry = FALSE;
		gbMatchedKB_UCentry = FALSE;
		return TRUE; // the caller must treat this as a valid 'save' operation
	}

    // if the user doesn't want a store done (he checked the dialog bar's control for this
    // choice) then return, without saving, after setting the source phrase's m_bNotInKB
    // flag to TRUE (ignore this block when glossing)
    // BEW 18Jun10 change....
	// Note, DoNotInKB() is called from OnCheckKBSave() and in the former StoreText() is
	// called, and the m_bSaveToKB value is not changed to its final value until after
	// DoNotInKB() returns - so if the checkbox was ticked and the user has just unticked
	// it, m_bSaveToKB will still be TRUE at this point, and the following block won't be
	// entered ... check if we really need this block, as the normal blocks below actually
	// do the store of <Not In KB> string, and then higher level callers such as
	// DoNotInKB() and its OnCheckKBSave() caller re-set the CSourcePhrase flags to be
	// what is appropriate for a <Not In KB> location; so when the user clicks to make the
	// checkbox be unticked (ie. no store to KB) then the block below is not called. On
	// the other hand, when he clicks the unticked checkbox to have saving to the KB
	// restored for that current source text key, what does DoNotInKB() do? It does not
	// call StoreText(), and neither does it's OnCheckKBSave() caller. This means that the
	// block below is NEVER entered, and so I have commented it out
	/* I verified this block is never called, whether the checkbox is on or off
	if (!m_bGlossingKB && !m_pApp->m_bSaveToKB)
	{
		pSrcPhrase->m_bNotInKB = TRUE;
		m_pApp->m_bForceAsk = FALSE; // have to turn this off too, since 
								   // this is regarded as a valid 'store' op
		pSrcPhrase->m_bHasKBEntry = FALSE;
		pSrcPhrase->m_bBeginRetranslation = FALSE;
		pSrcPhrase->m_bEndRetranslation = FALSE;
		gbMatchedKB_UCentry = FALSE;
		return TRUE; // we want the caller to think all is well
	}
	*/
    // if there is a CTargetUnit associated with the current key, then get it; if not,
    // create one and add it to the appropriate map; we start by computing which map we
    // need to store to
    // BEW 13Nov10, changed to support Bob Eaton's request for a ten map glossing KB
	int nMapIndex;
	//if (m_bGlossingKB)
	//	nMapIndex = 0; // always an index of zero when glossing
	//else
	if (IsFixedSpaceSymbolWithin(pSrcPhrase))
	{
		nMapIndex = 0;
	}
	else
	{
		nMapIndex = pSrcPhrase->m_nSrcWords - 1; // index to the appropriate map
	}
		//nMapIndex = pSrcPhrase->m_nSrcWords - 1; // index to the appropriate map

    // if we have too many source words, then we cannot save to the KB, so detect this and
    // warn the user that it will not be put in the KB, then return TRUE since all is
    // otherwise okay (this will be handled as a retranslation, by default) The following
    // comment is for when glossing... Note: if the source phrase is part of a
    // retranslation, we allow updating of the m_gloss attribute, and we won't change any
    // of the retranslation supporting flags; so it is therefore possible for
    // m_bRetranslation to be TRUE, and also for m_bHasGlossingKBEntry to be TRUE.
    
    // BEW 13Nov10, changed to support Bob Eaton's request for a ten map glossing KB
	//if (!m_bGlossingKB && pSrcPhrase->m_nSrcWords > MAX_WORDS)
	if (pSrcPhrase->m_nSrcWords > MAX_WORDS)
	{
		pSrcPhrase->m_bNotInKB = FALSE;
		if (m_bGlossingKB)
			pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
		else
			pSrcPhrase->m_bHasKBEntry = FALSE;
		wxMessageBox(_(
"Warning: there are too many source language words in this phrase for this adaptation to be stored in the knowledge base."),
		_T(""),wxICON_INFORMATION);
		gbMatchedKB_UCentry = FALSE;
		return TRUE;
	}

	// continue the storage operation
	wxString unchangedkey = pSrcPhrase->m_key; // this never has case change done
											  // to it (need this for lookups)
	wxString key = AutoCapsMakeStorageString(pSrcPhrase->m_key); // key might be 
															// made lower case
	CTargetUnit* pTU;
	CRefString* pRefString;
	if (m_pMap[nMapIndex]->empty())
	{
		// we just won't store anything if the target phrase has no content, when
		// bSupportNoAdaptationButton has it's default value of FALSE, but if TRUE
		// then we skip this block so that we can store an empty string as a valid
		// KB "adaptation" or "gloss" - depending on which KB is active here
		if (tgtPhrase.IsEmpty())
		{
			if(!bSupportNoAdaptationButton)
			{
				if (!m_bGlossingKB)
				{
					pSrcPhrase->m_bBeginRetranslation = FALSE;
					pSrcPhrase->m_bEndRetranslation = FALSE;
				}
				m_pApp->m_bForceAsk = FALSE; // make sure it's turned off
				gbMatchedKB_UCentry = FALSE;
				return TRUE; // make caller think all is well
			}
		}
		
		// we didn't return, so continue on to create a new CTargetUnit for storing to
		pTU = new CTargetUnit;
		wxASSERT(pTU != NULL);
		pRefString = new CRefString(pTU); // automatically creates and hooks up its owned
					// CRefStringMetadata instance and sets creator and creation datetime
		wxASSERT(pRefString != NULL);

		pRefString->m_refCount = 1; // set the count
		// add the translation string
		if (bNoError)
		{
			pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);
		}
		else
		{
			// if something went wrong, just save as if gbAutoCaps was FALSE
			pRefString->m_translation = tgtPhrase;
		}
		pTU->m_pTranslations->Append(pRefString); // store in the CTargetUnit
		if (m_pApp->m_bForceAsk)
		{
			pTU->m_bAlwaysAsk = TRUE; // turn it on if user wants to be given
					// opportunity to add a new refString next time its matched
		}
		if (m_bGlossingKB)
		{
			pSrcPhrase->m_bHasGlossingKBEntry = TRUE; // tell the src phrase it has
													 // an entry in the glossing KB
			(*m_pMap[nMapIndex])[key] = pTU; // store the CTargetUnit in the map

			// update the maxWords limit,  BEW changed 13Nov10
			//m_nMaxWords = 1;
			if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
				m_nMaxWords = pSrcPhrase->m_nSrcWords;
		}
		else
		{
			pSrcPhrase->m_bHasKBEntry = TRUE; // tell the src phrase it has 
											  // an entry in the KB

			// it can't be a retranslation, so ensure the next two flags are cleared
			pSrcPhrase->m_bBeginRetranslation = FALSE;
			pSrcPhrase->m_bEndRetranslation = FALSE;

			(*m_pMap[nMapIndex])[key] = pTU; // store the CTargetUnit in the 
											 // map with appropriate index
			// update the maxWords limit
			if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
				m_nMaxWords = pSrcPhrase->m_nSrcWords;
		}
	}
	else // the block below is for when the map is not empty
	{
		// there might be a pre-existing association between this key and a CTargetUnit,
		// so check it out
		bool bFound = AutoCapsLookup(m_pMap[nMapIndex], pTU, unchangedkey);

		// check we have a valid pTU
		if (bFound && pTU->m_pTranslations->IsEmpty())
		{
			// this is an error condition, CTargetUnits must NEVER have an 
			// empty m_translations list
			wxMessageBox(_T(
"Warning: the current storage operation has been skipped, and a bad storage element has been deleted."),
			_T(""), wxICON_EXCLAMATION);

			delete pTU; // delete it from the heap
			pTU = (CTargetUnit*)NULL;

			MapKeyStringToTgtUnit* pMap = m_pMap[nMapIndex];
			int bRemoved;
			if (gbAutoCaps && gbSourceIsUpperCase && !gbMatchedKB_UCentry)
				bRemoved = pMap->erase(key); // remove the changed to lc entry from the map
			else
				bRemoved = pMap->erase(unchangedkey); // remove the unchanged one from the map
			wxASSERT(bRemoved == 1);
			m_pApp->m_bSaveToKB = TRUE; // ensure its back on (if here from a choice not 
				// save to KB, this will be cleared by OnCheckKBSave, preserving user choice)
			gbMatchedKB_UCentry = FALSE;
			return FALSE;
		}

		// if not found, then create a targetUnit, and add the refString, etc, as above;
		// but if one is found, then check whether we add a new refString or increment the
		// refCount of an existing one
		if(!bFound)
		{
			// we just won't store anything if the target phrase has no content, when
			// bSupportNoAdaptationButton has it's default value of FALSE, but if TRUE
			// then we skip this block so that we can store an empty string as a valid
			// KB "adaptation" or "gloss" - depending on which KB is active here
			if (tgtPhrase.IsEmpty())
			{
				if(!bSupportNoAdaptationButton)
				{
					if (!m_bGlossingKB)
					{
						pSrcPhrase->m_bBeginRetranslation = FALSE;
						pSrcPhrase->m_bEndRetranslation = FALSE;
					}
					m_pApp->m_bForceAsk = FALSE; // make sure it's turned off
					gbMatchedKB_UCentry = FALSE;
					return TRUE; // make caller think all is well
				}
			}

			pTU = new CTargetUnit;
			wxASSERT(pTU != NULL);
			pRefString = new CRefString(pTU); // creates its CRefStringMetadata instance too
			wxASSERT(pRefString != NULL);

			pRefString->m_refCount = 1; // set the count
			// add the translation or gloss string
			pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);
			pTU->m_pTranslations->Append(pRefString); // store in the CTargetUnit
			if (m_pApp->m_bForceAsk)
			{
				pTU->m_bAlwaysAsk = TRUE; // turn it on if user wants to be given
					// opportunity to add a new refString next time its matched
			}
			if (m_bGlossingKB)
			{
				pSrcPhrase->m_bHasGlossingKBEntry = TRUE;

				(*m_pMap[nMapIndex])[key] = pTU; // store the CTargetUnit in 
						// the map with appropr. index (key may have been made lc)
				// update the maxWords limit, BEW changed 13Nov10
				//m_nMaxWords = 1;
				if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
					m_nMaxWords = pSrcPhrase->m_nSrcWords;
			}
			else
			{
				pSrcPhrase->m_bHasKBEntry = TRUE;

				(*m_pMap[nMapIndex])[key] = pTU; // store the CTargetUnit in
						// the map with appropr. index (key may have been made lc)
				// update the maxWords limit
				if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
					m_nMaxWords = pSrcPhrase->m_nSrcWords;
			}
		}
		else // we found one
		{
            // we found a pTU for this key, so check for a matching CRefString, if there is
            // no match, then add a new one (note: no need to set m_nMaxWords for this
            // case)
            // BEW addition 17Jun10, we may match a CRefString instance for which
            // m_bDeleted is TRUE, and in that case we have to undelete it - so check for
            // this and branch accordingly
			bool bMatched = FALSE;
			pRefString = new CRefString(pTU);
			wxASSERT(pRefString != NULL);
			pRefString->m_refCount = 1; // set the count, assuming this will be stored
										// (it may not be)
            // set its gloss or adaptation string; the fancy test is required because the
            // refStr entry may have been stored in the kb when auto-caps was off, and if
            // it was upper case for the source text's first letter, then it will have been
            // looked up only on the second attempt, for which gbMatchedKB_UCentry will
            // have been set TRUE, and which means the gloss or adaptation will not have
            // been made lower case - so we must allow for this possibility
			if ((gbAutoCaps && gbMatchedKB_UCentry) || !gbAutoCaps)
				pRefString->m_translation = tgtPhrase; // use the unchanged string, 
													   // could be upper case
			else
				pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);

			TranslationsList::Node* pos = pTU->m_pTranslations->GetFirst();
			while (pos != NULL)
			{
				CRefString* pRefStr = pos->GetData();
				pos = pos->GetNext();
				wxASSERT(pRefStr != NULL);

				// does it match?
				if (*pRefStr == *pRefString) // TRUE if pRStr->m_translation ==
											 //         pRefString->m_translation
				{
					// if we get a match, then increment ref count and point to this, etc
					// - but allow for the possibility we may have matched a m_bDeleted one
					bMatched = TRUE;
					if (pRefStr->m_bDeleted)
					{
						// we've matched a deleted entry, so we must undelete it
						pRefStr->m_bDeleted = FALSE;
						pRefStr->m_refCount = 1;
						pRefStr->m_pRefStringMetadata->m_creationDateTime = GetDateTimeNow();
						pRefStr->m_pRefStringMetadata->m_deletedDateTime.Empty();
						pRefStr->m_pRefStringMetadata->m_modifiedDateTime.Empty();
						// in next call, param bool bOriginatedFromTheWeb is default FALSE
						pRefStr->m_pRefStringMetadata->m_whoCreated = SetWho();
					}
					else
					{
						pRefStr->m_refCount++;
					}
					pRefString->DeleteRefString(); // don't need this new one
					pRefString = (CRefString*)NULL;
					if (m_bGlossingKB)
					{
						pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
					}
					else
					{
						pSrcPhrase->m_bHasKBEntry = TRUE;
						pSrcPhrase->m_bBeginRetranslation = FALSE;
						pSrcPhrase->m_bEndRetranslation = FALSE;
					}
					if (m_pApp->m_bForceAsk)
					{
						pTU->m_bAlwaysAsk = TRUE; // nTrCount might be 1, so we must
								// ensure it gets set if that is what the user wants
					}
					break;
				}
			}
            // if we get here with bMatched == FALSE, then there was no match, so we must
            // add the new pRefString to the targetUnit, but if it is already an <Not In
            // KB> entry, then the latter must override (to prevent <Not In KB> and <no
            // adaptation> or a nonempty adaptation or gloss being two ref strings for the
            // one key -- when adapting; for glossing this cannot happen)
			if (!bMatched)
			{
				TranslationsList::Node* tpos = pTU->m_pTranslations->GetFirst();
				CRefString* pRefStr = (CRefString*)tpos->GetData();
				if (!m_bGlossingKB && pRefStr->m_translation == _T("<Not In KB>"))
				{
                    // keep it that way (the way to cancel this setting is with the toolbar
                    // checkbox) But leave m_adaption and m_targetStr (or m_gloss) having
                    // whatever the user may have typed
					pSrcPhrase->m_bHasKBEntry = FALSE;
					pSrcPhrase->m_bNotInKB = TRUE;
					pSrcPhrase->m_bBeginRetranslation = FALSE;
					pSrcPhrase->m_bEndRetranslation = FALSE;
					pRefString->DeleteRefString(); // don't leak memory
					pRefString = (CRefString*)NULL;
					m_pApp->m_bForceAsk = FALSE;
					gbMatchedKB_UCentry = FALSE;
					return TRUE; // all is well
				}
				else // either we are glossing, or we are adapting 
					  // and it's a normal adaptation
				{
					// is the pApp->m_targetPhrase empty?
					if (tgtPhrase.IsEmpty())
					{
                        // we just won't store anything if the target phrase has no
                        // content, when bSupportNoAdaptationButton has it's default value
                        // of FALSE, but if TRUE then we skip this block so that we can
                        // store an empty string as a valid KB "adaptation" or "gloss" -
                        // depending on which KB is active here
						if(!bSupportNoAdaptationButton)
						{
							if (!m_bGlossingKB)
							{
								pSrcPhrase->m_bBeginRetranslation = FALSE;
								pSrcPhrase->m_bEndRetranslation = FALSE;
							}
							m_pApp->m_bForceAsk = FALSE; // make sure it's turned off
							pRefString->DeleteRefString(); // don't leak the memory
							pRefString = (CRefString*)NULL;
							gbMatchedKB_UCentry = FALSE;
							return TRUE; // make caller think all is well
						}
					}

					// recalculate the string to be stored, in case we looked up a
					// stored upper case entry earlier
					pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);
					pTU->m_pTranslations->Append(pRefString);
					if (m_bGlossingKB)
					{
						pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
					}
					else
					{
						pSrcPhrase->m_bHasKBEntry = TRUE;
						pSrcPhrase->m_bBeginRetranslation = FALSE;
						pSrcPhrase->m_bEndRetranslation = FALSE;
					}
				}
			}
		}
	}
	if (!m_bGlossingKB)
		pSrcPhrase->m_bNotInKB = FALSE; // ensure correct flag value, 
										// in case it was not in the KB
	m_pApp->m_bForceAsk = FALSE; // must be turned off, as it applies
							   // to the current store operation only
	gbMatchedKB_UCentry = FALSE;
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      pKeys   <- pointer to the list of keys that is produced
/// \remarks
/// Called from: the App's OnFileRestoreKb().
/// Gets a list of the keys which have a unique translation and for which the CTargetUnit
/// instance's m_bAlwaysAsk attribute is set to TRUE. This function is only called when a
/// corrupted KB was detected, so we cannot expect to be able to recover all such keys, but
/// as many as we can, which will reduce the amount of manual setting of the flags in the
/// KB editor after a KB restore is done.
/// BEW 14May10, moved to here from CAdapt_ItApp class
/// BEW 13Nov10, no changes to support Bob Eaton's request for glosssing KB to use all maps
////////////////////////////////////////////////////////////////////////////////////////
void CKB::GetForceAskList(KPlusCList* pKeys)
// get a list of the keys which have a unique translation and for which the CTargetUnit
// instance's m_bAlwaysAsk attribute is set TRUE. (Since we are using this with a probably
// corrupted KB, we cannot expect to be able to recover all such keys, and we will have to
// check carefully for corrupt pointers, etc. We will recover as many as we can, which will
// reduce the amount of the user's manual setting of the flag in the KB editor, after the
// KB restore is done.)
// For glossing being ON, this function should work correctly without any changes needed,
// provided the caller calls this function on the glossing KB
// BEW 21Jun10, no changes needed for support of kbVersion 2
{
	wxASSERT(pKeys->IsEmpty());
	for (int i=0; i < MAX_WORDS; i++)
	{
		MapKeyStringToTgtUnit* pMap = m_pMap[i];
		if (!pMap->empty())
		{
			MapKeyStringToTgtUnit::iterator iter;
			for (iter = pMap->begin(); iter != pMap->end(); iter++)
			{
				wxString key;
				key.Empty();
				CTargetUnit* pTU = (CTargetUnit*)NULL;
				try
				{
					key = iter->first; 
					pTU = iter->second;
					if (key.Length() > 360)
						break; // almost certainly a corrupt key string, so exit the map

					// we have a valid pointer to a target unit, so see if the flag is set
					// and if the translation is unique
					if (pTU->m_pTranslations->GetCount() == 1)
					{
						if (pTU->m_bAlwaysAsk)
						{
							// we've found a setting which is to be preserved, so do so
							KeyPlusCount* pKPlusC = new KeyPlusCount;
							wxASSERT(pKPlusC != NULL);
							pKPlusC->count = i+1; // which map we are in
							pKPlusC->key = key;
							pKeys->Append(pKPlusC); // add it to the list
						}
					}	
				}
				catch (...)
				{
					break; // exit from this map, as we no longer can be sure of a valid pos
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return     a CBString (byte string) containing the composed KB element (each
///             element handles one CTargetUnit and its associated key string (the
///             source text word or phrase, minus punctuation)
/// \param      src       -> reference to the source text word or phrase
/// \param      pTU       -> pointer to the associated CTargetUnit instance
/// \param      nTabLevel -> how many tabs are to be put at the start of each line 
///                          for indenting purposes (2)
/// \remarks
/// Called from: this class's DoKBSaveAsXML() member function
/// Constructs a byte string containing a composed KB element in XML format in which one
/// CTargetUnit and its associated key string is represented (the source text word or
/// phrase, minus punctuation). This function is called once for each target unit in the
/// KB's map.
/// BEW 31May10: simplified kbv2 design to just have a CRefStringMetadata class instance
/// for each CRefString instance, stored on m_pRefStringMetadata, and the former class
/// pointers to its owner with a member m_pRefStringOwner.
/// Currently, there are four metadata wxStrings in the new class:
/// m_creationDateTime which we'll use the xml attribute cDT for 
/// m_modifiedDateTime which we'll use the xml attribute mDT for 
/// m_deletedDateTime which we'll use the xml attribute dDT for 
/// m_whoCreated which we'll use the xml attribute wC for
/// all the above will be in the <RS> tag. Put m_creationDateTime and m_whoCreated on
/// their own line, as these will be always present; the other two can be on the next line
/// as they often will not be present. We'll not make an attribute for any of these
/// strings if they are empty - defaults will handle these when the xml is parsed back in.
// BEW 13Nov10, no changes to support Bob Eaton's request for glosssing KB to use all maps
////////////////////////////////////////////////////////////////////////////////////////
CBString CKB::MakeKBElementXML(wxString& src,CTargetUnit* pTU,int nTabLevel)
{
	// Constructs
	// <TU f="boolean" k="source text key (a word or phrase)">
	// 	<RS n="reference count" a="adaptation (or gloss) word or phrase"/>
	// 	... possibly more RS elements (for other adaptations of the same source)
	// </TU>
	// The RS element handles the contents of the CRefString instances stored 
	// on the pTU instance
	CBString aStr;
	CRefString* pRefStr;
	int i;
	wxString intStr;
    // wx note: the wx version in Unicode build refuses assign a CBString to
    // char numStr[24] so I'll declare numStr as a CBString also
	CBString numStr; //char numStr[24];

#ifndef _UNICODE  // ANSI (regular) version

	// start the targetUnit element
	aStr.Empty();
	for (i = 0; i < nTabLevel; i++)
	{
		aStr += "\t"; // tab the start of the line
	}
	aStr += "<TU f=\"";
	intStr.Empty(); // needs to start empty, otherwise << will 
					// append the string value of the int
	intStr << (int)pTU->m_bAlwaysAsk;
	numStr = intStr;
	aStr += numStr;
	aStr += "\" k=\"";
	CBString s(src);
	InsertEntities(s);
	aStr += s;
	aStr += "\">\r\n";

	TranslationsList::Node* pos = pTU->m_pTranslations->GetFirst();
	CBString rsTabs; 
	while (pos != NULL)
	{
		// there will always be at least one pRefStr in a pTU
		pRefStr = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		CBString bstr(pRefStr->m_translation);
		InsertEntities(bstr);
		intStr.Empty(); // needs to start empty, otherwise << will 
						// append the string value of the int
		intStr << pRefStr->m_refCount;
		numStr = intStr;

		// construct the tabs
		int j;
		rsTabs.Empty();
		int last = nTabLevel + 1;
		for (j = 0; j < last ; j++)
		{
			aStr += "\t"; // tab the start of the line
			rsTabs += "\t"; // use rsTabs below to avoid a loop each time
		}
		// construct the element
		aStr += "<RS n=\""; // reference count
		aStr += numStr;
		aStr += "\" a=\""; // adaptation (or gloss for GlossingKB)
		aStr += bstr;

		// whm 14Jan11 modified the following to create 5.2.4 or 6.0.0+ KB xml format
		// depending on whether we are doing a Save As Legacy XML verion 3, 4, or 5
		// format.
		CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
		if (pDoc != NULL && pDoc->IsLegacyDocVersionForFileSaveAs())
		{
			// Its the older Legacy format; close off the RS element
			aStr += "\"/>\r\n";
		}
		else
		{
			// Its the newer format; add the metadata stuff
			aStr += "\" df=\"";  // m_bDeleted flag
			if (pRefStr->m_bDeleted)
			{
				aStr += "1";
			}
			else
			{
				aStr += "0";
			}

			// BEW 31May10, added code for kbv2's new attributes taken from the
			// CRefStringMetadata class instance attached to this pRefStr
			aStr += "\"\r\n"; // end the line
			aStr += rsTabs; // start a new line at the same indentation
			CBString attrStr = pRefStr->m_pRefStringMetadata->m_creationDateTime;
			// no entities are in the dateTime strings, so don't call InsertEntities()
			aStr += "cDT=\"";
			aStr += attrStr; // the m_creationDateTime value

			aStr += "\" wC=\"";
			attrStr = pRefStr->m_pRefStringMetadata->m_whoCreated;
			aStr += attrStr; // the m_whoCreated value

			// how we finish off the line depends on whether more is present or not
			if (pRefStr->m_pRefStringMetadata->m_modifiedDateTime.IsEmpty() 
				&& pRefStr->m_pRefStringMetadata->m_deletedDateTime.IsEmpty())
			{
				// there is no more to be constructed, so finish off the RS element
				aStr += "\"/>\r\n";
			}
			else
			{
				// another line is needed for one or both of the other dateTime members
				aStr += "\"\r\n"; // end the line
				aStr += rsTabs; // start a new line at the same indentation
				attrStr = pRefStr->m_pRefStringMetadata->m_modifiedDateTime;
				if (!attrStr.IsEmpty())
				{
					aStr += "mDT=\"";
					aStr += attrStr; // the m_modifiedDateTime value

					// we may have m_deletedDateTime non-empty too, so check it out and build
					// it here if needed - then close off RS element
					attrStr = pRefStr->m_pRefStringMetadata->m_deletedDateTime;
					if (!attrStr.IsEmpty())
					{
						// there is a non-empty m_deletedDateTime stamp stored, so add it to
						// the RS element
						aStr += "\" dDT=\"";
						aStr += attrStr; // the m_deletedDateTime value
					}
					else
					{
						// there is no m_deletedDateTime stamp available
						;
					}
				}
				else
				{
					// m_modifiedDateTime is empty, so it must be the case that
					// m_deletedDateTime is not empty, so unilaterally build the xml for it
					// and then close off the RS element
					aStr += "dDT=\"";
					attrStr = pRefStr->m_pRefStringMetadata->m_deletedDateTime;
					aStr += attrStr; // the m_deletedDateTime value
				}
				// close off the RS element
				aStr += "\"/>\r\n";
			} // end of else block for test of both modifed and deleted dateTime stamps empty
		} // end of else not Legacy 5.x.x format
	} // end of loop: while (pos != NULL)

	// construct the closing TU tab
	for (i = 0; i < nTabLevel; i++)
	{
		aStr += "\t"; // tab the start of the line
	}
	aStr += "</TU>\r\n";

#else  // Unicode version

	// start the targetUnit element
	aStr.Empty();
	for (i = 0; i < nTabLevel; i++)
	{
		aStr += "\t"; // tab the start of the line
	}
	aStr += "<TU f=\"";
	intStr.Empty(); // needs to start empty, otherwise << will 
					// append the string value of the int
	intStr << (int)pTU->m_bAlwaysAsk;
	numStr = m_pApp->Convert16to8(intStr);
	aStr += numStr;
	aStr += "\" k=\"";
	CBString s = m_pApp->Convert16to8(src);
	InsertEntities(s);
	aStr += s;
	aStr += "\">\r\n";

	TranslationsList::Node* pos = pTU->m_pTranslations->GetFirst();
	CBString rsTabs; 
	while (pos != NULL)
	{
		// there will always be at least one pRefStr in a pTU
		pRefStr = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		CBString bstr = m_pApp->Convert16to8(pRefStr->m_translation);
		InsertEntities(bstr);
		intStr.Empty(); // needs to start empty, otherwise << will 
						// append the string value of the int
		intStr << pRefStr->m_refCount;
		numStr = m_pApp->Convert16to8(intStr);

		// construct the tabs
		int j;
		rsTabs.Empty();
		int last = nTabLevel + 1;
		for (j = 0; j < last ; j++)
		{
			aStr += "\t"; // tab the start of the line
			rsTabs += "\t"; // use rsTabs below to avoid a loop each time
		}
		// construct the element
		aStr += "<RS n=\"";
		aStr += numStr;
		aStr += "\" a=\"";
		aStr += bstr;

		// whm 14Jan11 modified the following to create 5.2.4 or 6.0.0+ KB xml format
		// depending on whether we are doing a Save As Legacy XML verion 3, 4, or 5
		// format.
		CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
		if (pDoc != NULL && pDoc->IsLegacyDocVersionForFileSaveAs())
		{
			// it's the older Legacy format; close off the RS element
			aStr += "\"/>\r\n";
		}
		else
		{
			// it's the newer format; add the metadata stuff
			aStr += "\" df=\"";  // m_bDeleted flag
			if (pRefStr->m_bDeleted)
			{
				aStr += "1";
			}
			else
			{
				aStr += "0";
			}

			// BEW 31May10, added code for kbv2's new attributes taken from the
			// CRefStringMetadata class instance attached to this pRefStr
			aStr += "\"\r\n"; // end the line
			aStr += rsTabs; // start a new line at the same indentation
			CBString attrStr = m_pApp->Convert16to8(pRefStr->m_pRefStringMetadata->m_creationDateTime);
			// no entities are in the dateTime strings, so don't call InsertEntities()
			aStr += "cDT=\"";
			aStr += attrStr; // the m_creationDateTime value

			aStr += "\" wC=\"";
			attrStr = m_pApp->Convert16to8(pRefStr->m_pRefStringMetadata->m_whoCreated);
			aStr += attrStr; // the m_whoCreated value

			// how we finish off the line depends on whether more is present or not
			if (pRefStr->m_pRefStringMetadata->m_modifiedDateTime.IsEmpty() 
				&& pRefStr->m_pRefStringMetadata->m_deletedDateTime.IsEmpty())
			{
				// there is no more to be constructed, so finish off the RS element
				aStr += "\"/>\r\n";
			}
			else
			{
				// another line is needed for one or both of the other dateTime members
				aStr += "\"\r\n"; // end the line
				aStr += rsTabs; // start a new line at the same indentation
				attrStr = m_pApp->Convert16to8(pRefStr->m_pRefStringMetadata->m_modifiedDateTime);
				if (!attrStr.IsEmpty())
				{
					aStr += "mDT=\"";
					aStr += attrStr; // the m_modifiedDateTime value

					// we may have m_deletedDateTime non-empty too, so check it out and build
					// it here if needed - then close off RS element
					attrStr = m_pApp->Convert16to8(pRefStr->m_pRefStringMetadata->m_deletedDateTime);
					if (!attrStr.IsEmpty())
					{
						// there is a non-empty m_deletedDateTime stamp stored, so add it to
						// the RS element
						aStr += "\" dDT=\"";
						aStr += attrStr; // the m_deletedDateTime value
					}
					else
					{
						// there is no m_deletedDateTime stamp available
						;
					}
				}
				else
				{
					// m_modifiedDateTime is empty, so it must be the case that
					// m_deletedDateTime is not empty, so unilaterally build the xml for it
					// and then close off the RS element
					aStr += "dDT=\"";
					attrStr = m_pApp->Convert16to8(pRefStr->m_pRefStringMetadata->m_deletedDateTime);
					aStr += attrStr; // the m_deletedDateTime value
				}
				// close off the RS element
				aStr += "\"/>\r\n";
			} // end of else block for test of both modifed and deleted dateTime stamps empty
		} // end of else not Legacy 5.x.x format
	} // end of loop: while (pos != NULL)

	// construct the closing TU tab
	for (i = 0; i < nTabLevel; i++)
	{
		aStr += "\t"; // tab the start of the line
	}
	aStr += "</TU>\r\n";

#endif // end of Unicode version
	return aStr;
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      f               -> the wxFile instance used to save the KB file
/// \remarks
/// Called from: the App's StoreGlossingKB() and StoreKB().
/// Structures the KB data in XML form. Builds the XML file in a wxMemoryBuffer with sorted
/// TU elements and finally writes the buffer to an external file.
/// BEW 28May10: removed TUList from CKB and the member m_pTargetUnits, TUList is redundant
/// BEW 31May10: simplified kbv2 design to just have a CRefStringMetadata class instance
/// for each CRefString instance, stored on m_pRefStringMetadata, and the former class
/// pointers to its owner with a member m_pRefStringOwner.
/// Currently, there are four metadata wxStrings in the new class:
/// m_creationDateTime which we'll use the xml attribute cDT for 
/// m_modifiedDateTime which we'll use the xml attribute mDT for 
/// m_deletedDateTime which we'll use the xml attribute dDT for 
/// m_whoCreated which we'll use the xml attribute wC for
/// all the above will be in the <RS> tag.
/// In the <KB> tag we make the following modifications:
/// The inappropriate docVersion attribute will become kbVersion
/// We add a glossingKB attibute to store the m_bGlossingKB boolean value as "1" (TRUE) or
/// as "0" (FALSE). CTargetUnit has no changes, so the <TU> tag's attributes are unchanged
/// BEW 13Nov10, changes to support Bob Eaton's request for glosssing KB to use all maps
////////////////////////////////////////////////////////////////////////////////////////
void CKB::DoKBSaveAsXML(wxFile& f)
{
 	// Setup some local pointers
	CBString aStr;
	CTargetUnit* pTU;	
	int mapIndex;

   // whm modified 17Jan09 to build the entire XML file in a buffer in a wxMemoryBuffer.
    // The wxMemoryBuffer is a dynamic buffer which increases its size as needed, which
    // means we can set it up with a reasonable default size and not have to know its exact
    // size ahead of time.
	static const size_t blockLen = 2097152; // 1MB = 1048576 bytes //4096; 
    // the buff block sise doesn't matter here for writing since we will write the whole
    // buffer using whatever size it turns out to be in one Write operation.
	wxMemoryBuffer buff(blockLen);

	// The following strings can be pre-built before examining the maps.
	CBString encodingStr;
	//CBString aiKBBeginStr; //whm 31Aug09 removed at Bob Eaton's request
	CBString msWordWarnCommentStr;
	CBString kbElementBeginStr;
	// .. [calculate the total bytes for MAP and TU Elements]
	CBString kbElementEndStr;
	
	// maxWords is the max number of MapKeyStringToTgtUnit maps we're dealing with.
	int maxWords;
	// BEW changed 13Nov10, to support Bob Eaton's request
	///if (m_bGlossingKB)
	//{
	///	maxWords = 1; // GlossingKB uses only one map
	//}
	//else
	//{
		maxWords = (int)MAX_WORDS;
	//}

	
	// Now, start building the fixed strings.
    // construct the opening tag and add the list of targetUnits with their associated key
    // strings (source text)
    // prologue (Changed by BEW, 18june07, at request of Bob Eaton so as to support legacy
    // KBs using his SILConverters software, UTF-8 becomes Windows-1252 for the Regular
    // app)

	m_pApp->GetEncodingStringForXmlFiles(encodingStr);
	buff.AppendData(encodingStr,encodingStr.GetLength()); // AppendData internally uses 
											// memcpy and GetAppendBuf and UngetAppendBuf
	
    /*  //whm 31Aug09 removed at Bob Eaton's request
	//	BEW added AdaptItKnowledgeBase element, 3Apr06, for Bob Eaton's SILConverters support, 
	aiKBBeginStr = "<AdaptItKnowledgeBase xmlns=\"http://www.sil.org/computing/schemas/AdaptIt KB.xsd\">\r\n";
	buff.AppendData(aiKBBeginStr,aiKBBeginStr.GetLength());
	*/
	
    // add the comment with the warning about not opening the XML file in MS WORD 'coz is
    // corrupts it - presumably because there is no XSLT file defined for it as well. When
    // the file is then (if saved in WORD) loaded back into Adapt It, the latter goes into
    // an infinite loop when the file is being parsed in. (MakeMSWORDWarning is defined in
    // XML.cpp)
	msWordWarnCommentStr = MakeMSWORDWarning(TRUE); // the warning ends with \r\n so 
				// we don't need to add them here; TRUE gives us a note included as well
	buff.AppendData(msWordWarnCommentStr,msWordWarnCommentStr.GetLength());

	wxString srcStr;
	CBString tempStr;
	wxString intStr;
    // wx note: the wx version in Unicode build refuses assign a CBString to char
    // numStr[24] so I'll declare numStr as a CBString also
	CBString numStr; //char numStr[24];

	// the KB opening tag
	intStr.Empty();
    // wx note: The itoa() operator is Microsoft specific and not standard; unknown to g++
    // on Linux/Mac. The wxSprintf() statement below in Unicode build won't accept CBString
    // or char numStr[24] for first parameter, therefore, I'll simply do the int to string
    // conversion in UTF-16 with wxString's overloaded insertion operatior << then convert
    // to UTF-8 with Bruce's Convert16to8() method. [We could also do it here directly with
    // wxWidgets' conversion macros rather than calling Convert16to8() - see the
    // Convert16to8() function in the App.]
	//wxSprintf(numStr, 24,_T("%d"),(int)VERSION_NUMBER); 
	// BEW note 19Apr10: docVersions 4 and 5 have different xml structure, but the KB doesn't.
	// BEW note 31May10: docVersion in the xml for the KB is a mistake. We will instead
	// use an attribute called kbVersion from kbv2 and onwards, kbVersion will take the
	// current kb version number, which is KB_VERSION2 (see Adapt_ItConstants.h), defined
	// as 2.
	//
	// whm modified 14Jan11 to save KB with Legacy xml format if the app is
	// performaing a Save As operation with the save type set to "Legacy XML format, 
	// as in versions 3, 4 or 5.", i.e., the Doc's m_bLegacyDocVersionForSaveAs member 
	// is TRUE.
	CBString verStrTagName;
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	if (pDoc != NULL && pDoc->IsLegacyDocVersionForFileSaveAs())
	{
		// we are saving the KB is Legacy xml format for 5.2.4 and earlier
		verStrTagName = xml_docversion;
		intStr << 4;
	}
	else
	{
		// we are saving the KB in current xml format for 6.0.0 and later
		verStrTagName = xml_kbversion;
		intStr << GetCurrentKBVersion(); // see note above for 31May10
	}
#ifdef _UNICODE
	numStr = m_pApp->Convert16to8(intStr);
#else
	numStr = intStr;
#endif
	
	aStr = "<";
	aStr += xml_kb;
	aStr += " ";
	aStr += verStrTagName; //verStrTagName is "kbVersion" for xml 6.0.0; "docVersion" for Save As xml 5.2.4
	aStr += "=\"";
	aStr += numStr;
	if (m_bGlossingKB)
	{
		// BEW changed 13Nov10
		//aStr += "\" max=\"1";
		aStr += "\" max=\"";
		intStr.Empty(); // needs to start empty, otherwise << will 
						// append the string value of the int
		intStr << m_nMaxWords;
#ifdef _UNICODE
		numStr = m_pApp->Convert16to8(intStr);
#else
		numStr = intStr;
#endif
		aStr += numStr;
		aStr += "\" glossingKB=\"1";
	}
	else
	{
		aStr += "\" srcName=\"";
#ifdef _UNICODE
		tempStr = m_pApp->Convert16to8(m_sourceLanguageName);
		InsertEntities(tempStr);
		aStr += tempStr;
		aStr += "\" tgtName=\"";
		tempStr = m_pApp->Convert16to8(m_targetLanguageName);
#else
		tempStr = m_sourceLanguageName;
		InsertEntities(tempStr);
		aStr += tempStr;
		aStr += "\" tgtName=\"";
		tempStr = m_targetLanguageName;
#endif
		InsertEntities(tempStr);
		aStr += tempStr;
		aStr += "\" max=\"";
		intStr.Empty(); // needs to start empty, otherwise << will 
						// append the string value of the int
		intStr << m_nMaxWords;
#ifdef _UNICODE
		numStr = m_pApp->Convert16to8(intStr);
#else
		numStr = intStr;
#endif
		aStr += numStr;
		aStr += "\" glossingKB=\"0"; // give an explicit value, rather
									 // than rely on a default & not
									 // output this attribute (in the
									 // adaptation KB)
	}
	aStr += "\">\r\n";
	kbElementBeginStr = aStr;
	buff.AppendData(kbElementBeginStr,kbElementBeginStr.GetLength());

	for (mapIndex = 0; mapIndex < maxWords; mapIndex++)
	{
		if (!m_pMap[mapIndex]->empty())
		{
			aStr = "\t<MAP mn=\"";
			intStr.Empty(); // needs to start empty, otherwise << will 
							// append the string value of the int
			intStr << mapIndex + 1;
#ifdef _UNICODE
			numStr = m_pApp->Convert16to8(intStr);
#else
			numStr = intStr;
#endif
			aStr += numStr;
			aStr += "\">\r\n";
			buff.AppendData(aStr,aStr.GetLength());
			MapKeyStringToTgtUnit::iterator iter;
#ifdef SHOW_KB_I_O_BENCHMARKS
				wxDateTime dt1 = wxDateTime::Now(),
						   dt2 = wxDateTime::UNow();
#endif
			wxArrayString TUkeyArrayString;
			TUkeyArrayString.Clear();
			TUkeyArrayString.Alloc(m_pMap[mapIndex]->size()); // Preallocate 
								// enough memory to store all keys in current map
			for( iter = m_pMap[mapIndex]->begin(); iter != m_pMap[mapIndex]->end(); ++iter )
			{
				TUkeyArrayString.Add(iter->first);
			}
			TUkeyArrayString.Sort(); // sort the array in ascending 
									 // alphabetical order
			wxASSERT(TUkeyArrayString.GetCount() == m_pMap[mapIndex]->size());
            // Get the key elements from TUkeyArrayString in sequence and fetch the
            // associated pTU from the map and do the usual MakeKBElementXML() and
            // DoWrite() calls.
			int ct;
			for (ct = 0; ct < (int)TUkeyArrayString.GetCount(); ct++)
			{
				iter = m_pMap[mapIndex]->find(TUkeyArrayString[ct]);
				if (iter != m_pMap[mapIndex]->end())
				{
					srcStr = iter->first;
					pTU = iter->second;
					wxASSERT(pTU != NULL);
					aStr = MakeKBElementXML(srcStr,pTU,2); // 2 = two left-margin tabs
					buff.AppendData(aStr,aStr.GetLength());
				}
			}
#ifdef SHOW_KB_I_O_BENCHMARKS
			dt1 = dt2;
			dt2 = wxDateTime::UNow();
			wxLogDebug(_T("Sorted Map %d executed in %s ms"), mapIndex, 
								(dt2 - dt1).Format(_T("%l")).c_str());
#endif
			// close off this map
			aStr = "\t</MAP>\r\n";
			buff.AppendData(aStr,aStr.GetLength());
		}
	}
	// KB closing tag
	aStr = "</";
	aStr += xml_kb;
	aStr += ">\r\n";
	kbElementEndStr = aStr;
	buff.AppendData(kbElementEndStr,kbElementEndStr.GetLength());
	// BEW added 0Apr06 for .NET parsing support for xml
	// whm 5Sept09 Bob Eaton says we should now eliminate both aiKBEndStr and aiKBBeginStr
	//aiKBEndStr = "</AdaptItKnowledgeBase>\r\n";
	//buff.AppendData(aiKBEndStr,aiKBEndStr.GetLength());

	wxLogNull logNo; // avoid spurious messages from the system

    const char *pByteStr = wx_static_cast(const char *, buff);
	f.Write(pByteStr,buff.GetDataLen());
	// The buff wxMemoryBuffer is automatically destroyed when 
	// it goes out of scope
}

// returns nothing
// restores a correct <Not_In_KB> entry for the pile pCurPile, if the user has edited it
// out of the KB within the KB editor - which is not the correct way to do it
// Note: all calling environments have just checked that the active CSourcePhrase instance
// has its m_bNotInKB flag set TRUE, which is how we know this call is required. The
// callers are: PlacePhraseBox(), MoveToNextPile(), MoveToNextPile_InTransliterationMode(),
// MoveToPrevPile() and MoveToImmedNextPile().
// BEW 13Apr10, no changes needed for support of doc version 5
// BEW 21Jun10, moved to CKB class and signature simplified
// BEW 21Jun10, no changes needed for support of kbVersion 2
// BEW 13Nov10, no changes to support Bob Eaton's request for glosssing KB to use all maps
void CKB::Fix_NotInKB_WronglyEditedOut(CPile* pCurPile)
{
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	CAdapt_ItView* pView = m_pApp->GetView(); // <<-- BEWARE if we later support multiple views/panes
											  // as this call gets the interlinear strips view/pane
	wxString str = _T("<Not In KB>");
	CSourcePhrase* pSP = pCurPile->GetSrcPhrase();
	CRefString* pRefStr = GetRefString(pSP->m_nSrcWords, pSP->m_key, str);
	if (pRefStr == NULL)
	{
		m_pApp->m_bSaveToKB = TRUE; // it will be off, so we must turn it back on to get 
								   // the string restored
        // don't inhibit the call to MakeTargetStringIncludingPunctuation( ) here,
        // since the phrase passed in is the non-punctuated one
		bool bOK;
		bOK = StoreText(pSP, str);
		// set the flags to ensure the asterisk shows above the pile, etc.
		pSP->m_bHasKBEntry = FALSE;
		pSP->m_bNotInKB = TRUE; 
	}

    // for version 1.4.0 and onwards, we have to permit the construction of the
    // punctuated target string; for auto caps support, we may have to change to UC
    // here too
	wxString str1 = m_pApp->m_targetPhrase;
	pView->RemovePunctuation(pDoc,&str1,from_target_text);
	if (gbAutoCaps)
	{
		bool bNoError = pDoc->SetCaseParameters(pSP->m_key);
		if (bNoError && gbSourceIsUpperCase && !gbMatchedKB_UCentry)
		{
			bNoError = pDoc->SetCaseParameters(str1,FALSE);
			if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
			{
				// a change to upper case is called for
				str1.SetChar(0,gcharNonSrcUC);
			}
		}
	}
	pSP->m_adaption = str1;

	// the following function is now smart enough to capitalize m_targetStr in the context
	// of preceding punctuation, etc, if required. No store done here, of course, since we
	// are just restoring a <Not In KB> entry.
	pView->MakeTargetStringIncludingPunctuation(pSP, m_pApp->m_targetPhrase);
}

// Modified for support of glossing KB as well as adapting KB. The caller must send the
// correct KB pointer in the first parameter.
// whm modified 27Apr09 to report errors of punctuation existing in documents discovered 
// during KB Restore
// BEW 18Jun10, no changes needed for support of kbVersion 2
// BEW 11Oct10, checked for extra doc version 5 marker storage members & m_follOuterPunct
// member, but no changes needed, however changes are needed in StoreText() which it calls
// BEW 15Nov10, moved from view class to CKB class, and removed pKB from signature, as the
// this pointer will point either to the glossing KB or the adaptations KB
void CKB::RedoStorage(CSourcePhrase* pSrcPhrase, wxString& errorStr)
{
	wxASSERT(m_pApp != NULL);
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	CAdapt_ItView* pView = m_pApp->GetView();
	m_pApp->m_bForceAsk = FALSE;
	bool bOK;
	if (m_bGlossingKB)
	{
		if (!pSrcPhrase->m_bHasGlossingKBEntry)
		{
			return; // nothing to be done
		}
		pSrcPhrase->m_bHasGlossingKBEntry = FALSE; // has to be false on input to StoreText()
		bOK = StoreText(pSrcPhrase,pSrcPhrase->m_gloss,TRUE); // TRUE = support storing empty gloss
		if (!bOK)
		{
			// I don't expect any error here, but just in case ...
			wxBell();
			wxASSERT(FALSE);
		}
	}
	else // adapting
	{
		if (!pSrcPhrase->m_bRetranslation)
		{
				if (pSrcPhrase->m_bNotInKB)
				{
                    // BEW changed 2Sep09, as there may have been some adaptations
                    // "restored" before coming to this location where m_bNotInKB is set
                    // true, we must ensure any normal adaptations for the relevant
                    // CTargetUnit are removed, and <Not In KB>
                    // put in their place - the easy way to do this is to call DoNotInKB()
                    // with its last parameter set TRUE (that function is elsewhere called
                    // in the handler for the Save to KB checkbox, namely,
                    // OnCheckKBSave()); and once this <Not In KB> entry is set up, any
                    // later identical source text encountered in the span will, when
                    // StoreText() tries to add it to the KB, will reject it because of the
                    // <Not In KB> entry already in the KB
					DoNotInKB(pSrcPhrase, TRUE);
					return;
				}
			if (!pSrcPhrase->m_bHasKBEntry)
			{
				return; // nothing to be done
			}
            // BEW added 24Apr09, for a while a bug allowed m_key to have following
            // punctuation treated as part of the word, allowing punctuation to get into
            // the adaptation KB's source text, and a different bug allowed punctuation to
            // get into the KB in some m_adaption members where punctuation was not
            // stripped out beforehand. This next code block is a "heal it" block which
            // detects when punctuation has wrongly got into m_key or m_adaption members,
            // removes it, and presents puncutation-less strings for storage instead; it
            // also has two tracking booleans, each of which is TRUE whenever the
            // associated string has been found to have had punctuation removed herein
			bool bKeyHasPunct = FALSE;
			bool bAdaptionHasPunct = FALSE;
			wxString strCurKey = pSrcPhrase->m_key;
			wxString strCurAdaption = pSrcPhrase->m_adaption;
			wxString strKey(strCurKey);
			wxString strAdaption(strCurAdaption);
			if (!pSrcPhrase->m_bNullSourcePhrase)
			{
				// Don't remove "..." (in source phrase only) which represents a 
				// null source phrase (placeholder)
				pView->RemovePunctuation(pDoc,&strKey,from_source_text);
			}
			if (!pSrcPhrase->m_bNotInKB)
			{
				// Don't remove "<Not In KB>" (in target text only)
				pView->RemovePunctuation(pDoc,&strAdaption,from_target_text);
			}
			if (strKey != strCurKey)
				bKeyHasPunct = TRUE;
			if (strAdaption != strCurAdaption)
				bAdaptionHasPunct = TRUE;
            // Construct an errorStr for a log file, to report where fixes were made. This
            // errorStr is passed back to the caller DoKBRestore() where log file is
            // written out to disk.
			if (bKeyHasPunct || bAdaptionHasPunct)
			{
                // initialize the log file's entry here whm note: For each given document
                // used in the KB Restore where a correction was made, the following string
                // is added (in the caller) to introduce the change in the log file:
				//errorStr = _T("During the KB Restore, a correction involving punctuation was made to the KB and the following document:\n   %s\n   Punctuation was removed (see below) which had been wrongly stored by a previous version of Adapt It.");
				if (bKeyHasPunct)
				{
					// compose a substring for log file
					errorStr += _T("\n      ");
					errorStr += _T("\"");
					errorStr += strCurKey;
					errorStr += _T("\"");
					errorStr += _T(" was changed to ");
					errorStr += _T("\"");
					errorStr += strKey;
					errorStr += _T("\"");
				}
				if (bAdaptionHasPunct)
				{
					// extend or begin a substring for log file
					errorStr += _T("\n      ");
					errorStr += _T("\"");
					errorStr += strCurAdaption;
					errorStr += _T("\"");
					errorStr += _T(" was changed to ");
					errorStr += _T("\"");
					errorStr += strAdaption;
					errorStr += _T("\"");
				}
				// finalize the entry here, and add it to the log file
				;
			}
			// ensure a punctuation-less m_key in the CSourcePhrase instance
			pSrcPhrase->m_key = strKey;
			// ensure a punctuation-less m_adaption in the CSourcePhrase instance
			pSrcPhrase->m_adaption = strAdaption;

			// legacy code follows
			pSrcPhrase->m_bHasKBEntry = FALSE; // has to be false on input to StoreText()
			gbInhibitMakeTargetStringCall = TRUE; // prevent any punctuation placement 
												  // dialogs from showing
			bool bOK = StoreText(pSrcPhrase,pSrcPhrase->m_adaption,TRUE); // TRUE = 
													// support storing empty adaptation
			gbInhibitMakeTargetStringCall = FALSE;
			if (!bOK)
			{
				// I don't expect any error here, but just in case ...
				::wxBell();
				wxASSERT(FALSE);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      pKB               -> pointer to the affected KB
/// \param      nCount            <- the count of files in the folder being processed
/// \param      nTotal            <- the total source phrase instances in the doc 
///                                  being processed
/// \param      nCumulativeTotal  <- the accumulation of the nTotal values over all 
///                                  documents processed so far
/// \remarks
/// Called from: the App's OnFileRestoreKb(). 
/// Iterates over the document files of a project, opens them and uses the data stored
/// within them to regererate new Knowledge Base file(s) from those documents.
/// This function assumes that the current directory will have already been set correctly
/// before being called. Modified to support restoration of either the glossing KB, or the
/// adaptation KB - which one gets the restoration depends on the gbIsGlossing flag's
/// value. Allows for the possibility that Bible book folders may be in the Adaptations
/// folder.
/// BEW 15Nov10, moved from app class to CKB class (It's caller is app's OnFileRestoreKb()
/// which cannot be moved to CKB as CKB is based on wxObject and so cannot intercept
/// events. If we were to base it on wxEvtHandler instead, we could then do so... not a
/// big deal either as wxEvtHandler is based on wxObject; & we'd need to add the class to
/// the event stack -- see near the end of app's OnInit() function for where this is done
/// with CPlaceholder, etc)
////////////////////////////////////////////////////////////////////////////////////////
void CKB::DoKBRestore(int& nCount, int& nTotal, int& nCumulativeTotal)
{
	wxArrayString* pList = &m_pApp->m_acceptedFilesList;
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	CAdapt_ItView* pView = m_pApp->GetView();
	wxASSERT(pDoc);
	wxASSERT(pView);

	// variables below added by whm 27Apr09 for reporting errors in docs used for KB
	// restore 
	bool bAnyDocChanged;
	bAnyDocChanged = FALSE;
	wxArrayString errors; // for use by KBRestoreErrorLog file
	wxArrayString docIntros; // for use by KBRestoreErrorLog file
	wxString errorStr;
	wxString logName = _T("KBRestoreErrorLog.txt");
	
    // The error is not likely to have happend much, and the document text itself was not
    // changed, so an English message in the log file should suffice.
    errors.Clear();
	wxDateTime theTime = wxDateTime::Now();
	wxString dateTime = theTime.Format(_T("%a, %b %d, %H:%M, %Y")).c_str();
    wxString logFileTime;
	logFileTime = logFileTime.Format(_T("This is the %s file - created %s."),logName.c_str(),dateTime.c_str());
	errors.Add(logFileTime);
	errors.Add(_T("\n\nDuring the KB Restore operation, punctuation errors were found and corrected in the KB,"));
	errors.Add(_T("\nand changes were made to the punctuation stored in one or more documents used to restore the KB."));
	errors.Add(_T("\nPlease Note the Following:")); 
	errors.Add(_T("\n* You should no longer notice any punctuation in KB entries when viewed with the KB Editor.")); 
	errors.Add(_T("\n* With punctuation purged from the KB Adapt It should handle punctuation in your documents as you expect.")); 
	errors.Add(_T("\n* You may wish to open the document(s) below in Adapt It and check the punctuation for the items listed.")); 
	errors.Add(_T("\n\n   In the following document(s) punctuation was removed from non-punctuation fields (see below):")); 
	// iterate over the document files
	int i;
	for (i=0; i < nCount; i++)
	{
		wxString newName = pList->Item(i);
		wxASSERT(!newName.IsEmpty());

		bool bOK;
		bOK = pDoc->OnOpenDocument(newName);
        // The docview sample has a routine called SetFileName() that it uses to override
        // the automatic associations of file name/path of a doc with a view. The
        // wxDocument member that holds the file name/path is m_documentFile. I've put the
        // SetFileName() routine in the Doc so we can do this like MFC has it.
		pDoc->SetFilename(newName,TRUE); // here TRUE means "notify the views" whereas
									// in the MFC version TRUE meant "add to MRU list"
		
		// Prepare an intro string for this document in case it has errors.
		errors.Add(_T("\n   ----------------------------------------"));
		// which had been wrongly stored there by a previous version of Adapt
		wxString docStr;
		docStr = docStr.Format(_T("\n   %s:"),newName.c_str());
		errors.Add(docStr);

		nTotal = m_pApp->m_pSourcePhrases->GetCount();
		wxASSERT(nTotal > 0);
		nCumulativeTotal += nTotal;

		// put up a progress indicator
		wxString progMsg = _("%s  - %d of %d Total words and phrases");
		wxString msgDisplayed = progMsg.Format(progMsg,newName.c_str(),1,nTotal);
		wxProgressDialog progDlg(_("Restoring the Knowledge Base"),
								msgDisplayed,
								nTotal,    // range
								(wxWindow*)m_pApp->GetMainFrame(),   // parent
								//wxPD_CAN_ABORT |
								//wxPD_CAN_SKIP |
								wxPD_APP_MODAL |
								wxPD_AUTO_HIDE //| -- try this as well
								//wxPD_ELAPSED_TIME |
								//wxPD_ESTIMATED_TIME |
								//wxPD_REMAINING_TIME
								//| wxPD_SMOOTH // - makes indeterminate mode bar on WinXP very small
								);
		SPList* pPhrases = m_pApp->m_pSourcePhrases;
		SPList::Node* pos1 = pPhrases->GetFirst();
		int counter = 0;
		bool bThisDocChanged = FALSE;
		while (pos1 != NULL)
		{
			CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos1->GetData();
			pos1 = pos1->GetNext();
			counter++;

			// update the glossing or adapting KB for this source phrase
			RedoStorage(pSrcPhrase,errorStr);
			if (!errorStr.IsEmpty())
			{
				// an error was detected (punctuation in non-punctuation field of doc)
				bThisDocChanged = TRUE;
				errors.Add(errorStr);
				errorStr.Empty(); // for next iteration
			}

			// update the progress bar every 20th iteration
			if (counter % 1000 == 0) //if (20 * (counter / 20) == counter)
			{
				msgDisplayed = progMsg.Format(progMsg,newName.c_str(),counter,nTotal);
				progDlg.Update(counter,msgDisplayed);
			}
		}
		// whm added 27Apr09 to save any changes made by RedoStorage above
		if (bThisDocChanged)
		{
			bAnyDocChanged = TRUE;
			// Save the current document before proceeding
			wxCommandEvent evt;
			pDoc->OnFileSave(evt);
		}
		else
		{
			errors.Add(_T("\n      * No changes were made in this file! *"));
		}

		pView->ClobberDocument();

		// delete the buffer containing the filed-in source text
		if (m_pApp->m_pBuffer != NULL)
		{
			delete m_pApp->m_pBuffer;
			m_pApp->m_pBuffer = (wxString*)NULL;
		}

        // wx version note: Since the progress dialog is modeless we do not need to destroy
        // or otherwise end its modeless state; it will be destroyed when DoKBRestore goes
        // out of scope

		// save the KB now that we have finished this document file (FALSE = no backup wanted)
		bool bSavedOK;
		if (m_bGlossingKB)
			bSavedOK = m_pApp->SaveGlossingKB(FALSE);
		else
			bSavedOK = m_pApp->SaveKB(FALSE);
		if (!bSavedOK)
		{
			wxMessageBox(_("Warning: something went wrong doing a save of the KB"),
							_T(""), wxICON_INFORMATION);
		}
		
		// remove the progress indicator window
		progDlg.Destroy();
	}
	
	if (bAnyDocChanged)
	{
	
		wxLogNull logNo; // avoid spurious messages from the system

		// The wxArrayString errors contains all the text to be written to the log file
		errors.Add(_T("\n\nEnd of log."));
		// Write out errors to external log file.
		bool bOK;
		bOK = ::wxSetWorkingDirectory(m_pApp->m_curProjectPath);
        // Note: Since we want a text file output, we'll use wxTextOutputStream which
        // writes text files as a stream on DOS, Windows, Macintosh and Unix in their
        // native formats (concerning their line endings)
		wxFileOutputStream output(logName);
		wxTextOutputStream cout(output);

		// Write out the contents of the errors array dumping it to the wxTextOutputStream
		int ct;
		for (ct = 0; ct < (int)errors.GetCount(); ct++)
		{
            // The wxArrayString errors contains the boiler text composed above plus the
            // individual errorStr strings received from RedoStorage()
			cout << errors.Item(ct);
		}
		wxString msg;
		msg = msg.Format(_(
"Adapt It changed the punctuation in one or more of your documents.\nSee the %s file in your project folder for more information on what was changed."),
		logName.c_str());
		wxMessageBox(msg,_T(""), wxICON_INFORMATION);
	}
	errors.Clear(); // clear the array
}

