/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KB.cpp
/// \author			Bill Martin
/// \date_created	21 March 2004
/// \rcs_id $Id$
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
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/filename.h>

//#define _ENTRY_DUP_BUG


#include "KbServer.h"
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
#include "XML.h"
#include "WaitDlg.h"
#include <wx/textfile.h>
#include "MainFrm.h"
#include "StatusBar.h"
#include "Thread_CreateEntry.h"

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
// TRUE if will contain a gloss; and also depending on the same flag, the pTgtUnit will
// have come from either the adaptation KB or the glossing KB.
// Returns NULL if no CRefString instance matching the criterion was found
// BEW 11May10, moved from view class, and made private (it's only called once,
// in GetRefString())
// BEW 18Jun10, changes needed for support of kbVersion 2, if an instance is found which
// matches the adaptation parameter, but the m_bDeleted flag is TRUE, then that is
// regarded as a failure to find an instance, and NULL is returned
// BEW changed 17Jul11, pass the pRefStr value back via signature, and return one of
// three enum values: absent, or present_but_deleted, or really_present
//CRefString* CKB::AutoCapsFindRefString(CTargetUnit* pTgtUnit, wxString adaptation)
enum KB_Entry CKB::AutoCapsFindRefString(CTargetUnit* pTgtUnit, wxString adaptation, CRefString*& pRefStr)
{
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
				// regard this as present but deleted, & pRefStr non-NULL
				//return (CRefString*)NULL;
				return present_but_deleted;
			}
			else
			{
				// tell caller it is really present, pRefStr non-NULL
				return really_present; // we found it
			}
		}
		else
		{
			if (adaptation.IsEmpty())
			{
				// it might be a <Not In KB> CSourcePhrase, check it out
				if (pRefStr->m_translation == _T("<Not In KB>") && !pRefStr->m_bDeleted)
				{
					// this is regarded as really present, pRefStr non_NULL
					//return pRefStr; // we return the pointer to this too
					return really_present;
				}
			}
		}
	}
	// finding it failed so return NULL for pRefStr, and absent for KB_ENTRY value
	//return (CRefString*)NULL;
	pRefStr = NULL;
	return absent;
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
// BEW added 29Jul11, removed the alternative uppercase lookup which formerly was done if
// autocaps was on and the lowercase lookup failed to find a matching CTargetUnit
// instance. This gives a more transparent management performance, in conjunction with some
// changes of same date within StoreText() etc, of data entries in the KB maps.
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
			//saveKey = keyStr; // save for an upper case lookup << BEW removed 29Jul11
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
            // if we get here, then the match failed...

			// BEW added 29Jul11, if the lowercase lookup failed, return FALSE with pTU
			// NULL so that caller (eg. StoreText() etc) will create a new CTargetUnit to
			// carry the converted to initial lowercase translation keyed to the
			// lowercase key value. This is better. Formerly if auto-caps was off and a
			// entry with key uppercase, such as "Kristus" was added to the KB, then the
			// user's choice to later use the KB with auto-caps on would not manage the
			// entries nicely. A further "Kristus" would have the first lookup done with
			// "kristus" and that would fail as there is no lowercase entry yet, and then
			// the second lookup would be attempted with "Kristus" - and that would
			// succeed. Then the translation, converted from "Christ" as received from the
			// tgtPhrase value (as typed within the phrase box) to "christ" is added to
			// this entry with uppercase key. There is then no way to get a KB entry with
			// key lowercase, "kristus" (except by turning off autocaps and doing
			// something clever which the average user wouldn't think to do); whereas
			// other entries with source text beginning with uppercase, with auto-caps ON,
			// DO get their key converted to lower case -- so there is a noticeable
			// assymmetry in the management of KB entries in such a scenario. The way to
			// avoid this is to force a lowercase lookup every time autocaps is ON, and if
			// it fails, (ie. the "Kristus" <-> "Christ" entry is not seen by the lookup)
			// then just return FALSE to the caller so that a new CTargetUnit instance is
			// created with key "kristus" and it's CRefString will then have "christ" in
			// the normal way. The negative in this is that it could result in a few extra
			// uppercase entries laying about unused in the KB. But that's a small price
			// to pay for management transparency. Besides, the user can see these
			// uppercase entries and if he wants, he can remove them using the KB editor.
			pTU = (CTargetUnit*)NULL;
			return FALSE;


            /* BEW removed 29Jul11
            // in case there is an upper case entry in the knowledge base (from when
            // autocapitalization was OFF), look it up; if there, then set the
            // gbMatchedKB_UCentry to TRUE so the caller will know that no restoration of
            // upper case will be required for the gloss or adaptation that it returns
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
			*/
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

#if defined(_KBSERVER)
// Return TRUE if a CTargetUnit instance is found with the passed in keyStr, FALSE is not.
// In this function, the keyStr parameter will always be a source string; the caller must
// determine which particular map is to be looked up and provide it's pointer as the first
// parameter; and if the lookup succeeds, pTU is the associated CTargetUnit instance's
// pointer. This function has no smarts for AutoCapitalization support, because KbServer
// database simply takes whatever is passed to it, and returns same when requested. A
// failure to find an associated CTargetUnit instance for the passed in keyStr also results
// in pTU being returned as NULL; otherwise.
bool CKB::LookupForKbSharing(MapKeyStringToTgtUnit* pMap, CTargetUnit*& pTU, wxString keyStr)
{
	wxString saveKey;
	MapKeyStringToTgtUnit::iterator iter;
	iter = pMap->find(keyStr);
	if (iter != pMap->end())
	{
		pTU = iter->second;
		wxASSERT(pTU != NULL);
	}
	else
	{
		pTU = (CTargetUnit*)NULL;
		return FALSE;
	}
	return TRUE;
}
#endif

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
// BEW changed 17Jul11, pass the pRefStr value back via signature, and return one of
// three enum values: absent, or present_but_deleted, or really_present
//CRefString* CKB::GetRefString(int nSrcWords, wxString keyStr, wxString valueStr)
enum KB_Entry CKB::GetRefString(int nSrcWords, wxString keyStr, wxString valueStr, CRefString*& pRefStr)
{
	// ensure nSrcWords is 1 if this is a GlossingKB access << BEW removed 13Nov10
	//if (m_bGlossingKB)
	//	nSrcWords = 1;
	MapKeyStringToTgtUnit* pMap = this->m_pMap[nSrcWords-1];
	wxASSERT(pMap != NULL);
	CTargetUnit* pTgtUnit;	// wx version changed 2nd param of AutoCapsLookup() below to
							// directly use CTargetUnit* pTgtUnit
	//CRefString* pRefStr;
	bool bOK = AutoCapsLookup(pMap,pTgtUnit,keyStr);
	if (bOK)
	{
		//return pRefStr = AutoCapsFindRefString(pTgtUnit,valueStr);
		KB_Entry rsEntry = AutoCapsFindRefString(pTgtUnit, valueStr, pRefStr);
		return rsEntry;
	}
    // lookup failed, so the KB state is different than data in the document
    // suggests, a Consistency Check operation should be done on the file(s)
	//return (CRefString*)NULL;
	pRefStr = NULL;
	return absent;
}

// this overload of GetRefString() is useful for LIFT imports BEW 21Jun10, no
// changes needed for support of kbVersion 2, because internally the call of
// AutoCapsFindRefString() returns NULL if the only match was of a CRefString
// instance with m_bDeleted set to TRUE
// BEW 13Nov10, no changes to support Bob Eaton's request for glosssing KB to
// use all maps
// BEW changed 17Jul11, pass the pRefStr value back via signature, and return
// one of three enum values: absent, or present_but_deleted, or really_present
//CRefString* CKB::GetRefString(CTargetUnit* pTU, wxString valueStr)
KB_Entry CKB::GetRefString(CTargetUnit* pTU, wxString valueStr, CRefString*& pRefStr)
{
	wxASSERT(pTU);
	KB_Entry rsEntry = AutoCapsFindRefString(pTU, valueStr, pRefStr);
	//if (pRefStr == NULL)
	//{
		// it's not in the m_translations list, so return NULL
	//	return (CRefString*)NULL;
	//
	//else
	//{
	//	return pRefStr;
	//}
	return rsEntry;
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
	{
		return;
	}
	// also return if it is a deleted one...
	if (pRefString->m_bDeleted)
	{
		return;
	}
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
// sure that is so). When the former is passed in, then the targetPhrase param is ignored,
// and the m_gloss or m_adaption member of the passed in pSrcPhrase is used for lookup -
// depending on the value of the private member m_bGlossingKB, when TRUE, m_gloss is used,
// when FALSE, m_adaption is used.
// BEW 17Jun10, no changes needed for support of kbVersion 2
// BEW 13Nov10, changes to support Bob Eaton's request for glosssing KB to use all maps
// BEW changed 17Jul11, comply with newer versions of GetRefString(), and
// AutoCapsFindRefString() where one of three enum values are returned: absent, or
// present_but_deleted, or really_present
void CKB::GetAndRemoveRefString(CSourcePhrase* pSrcPhrase, wxString& targetPhrase,
								enum UseForLookup useThis)
{
	CRefString* pRefStr = NULL;
	KB_Entry rsEntry;
	if (m_bGlossingKB)
	{
		if (useThis == useTargetPhraseForLookup)
		{
			//pRefStr = GetRefString(pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key, targetPhrase);
			rsEntry = GetRefString(pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key, targetPhrase, pRefStr);
		}
		else // useThis has the value useGlossOrAdaptationForLookup
		{
			//pRefStr = GetRefString(pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key, pSrcPhrase->m_gloss);
			rsEntry = GetRefString(pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key, pSrcPhrase->m_gloss, pRefStr);
		}
		// ensure correct flag value
		if ((pRefStr == NULL && pSrcPhrase->m_bHasGlossingKBEntry) ||
			(rsEntry == present_but_deleted && pSrcPhrase->m_bHasGlossingKBEntry))
		{
			// must be FALSE for a successful lookup on return
			pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
		}
		if (pRefStr != NULL && rsEntry == really_present)
		{
			// there's a non-deleted entry there which is to be removed now
			RemoveRefString(pRefStr, pSrcPhrase, pSrcPhrase->m_nSrcWords);
		}
	}
	else // it is an adapting KB
	{
		if (useThis == useTargetPhraseForLookup)
		{
			//pRefStr = GetRefString(pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key, targetPhrase);
			rsEntry = GetRefString(pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key, targetPhrase, pRefStr);
		}
		else // useThis has the value useGlossOrAdaptationForLookup
		{
			//pRefStr = GetRefString(pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key, pSrcPhrase->m_adaption);
			rsEntry = GetRefString(pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key, pSrcPhrase->m_adaption, pRefStr);
		}
		// ensure correct flag value
		if ((pRefStr == NULL && pSrcPhrase->m_bHasKBEntry) ||
			(rsEntry == present_but_deleted && pSrcPhrase->m_bHasKBEntry))
		{
			// must be FALSE for a successful lookup on return
			pSrcPhrase->m_bHasKBEntry = FALSE;
		}
		if (pRefStr != NULL && rsEntry == really_present)
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
	CTargetUnit* pTgtUnit = NULL;

	// is there a targetunit for this key in the KB?
	bool bFound = FindMatchInKB(nWords,key,pTgtUnit);
	if (!bFound)
	{
		return FALSE;
	}
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
//*
#if defined (_KBSERVER)

/// \return             nothing
/// \param pKbServer    ->  pointer to the particular KbServer instance which received
///                         the entry or entries from the server
 // \param whichEntries ->  an enum value; either forOneCTargetUnit (0) or mixedEntries (1)  --  deprecated
/// \remarks
/// This is a handler for storing either a downloaded entry from the server, or a series
/// of entries from the server - the latter expects that mixedEntries will be passed in.
/// I suspect I won't be supporting lookup of single entries in the remote server - it's
/// too time costly; but will rely on new entries being periodically downloaded instead.
/// Nevertheless, I'll code for the forOneCTargetUnit case, in case we later change my
/// mind on this point.
/// Populates either a CTargetUnit instance in the local KB with anything new in the
/// download, or many CTargetUnit instances arising from a timestamp-based download -
/// either of the whole KB, or of those entries newly added subsequent to a stored
/// timestamp value. The KbServer instance passed in has for it's public interface, a set
/// of seven in-parallel arrays, 6 are wxArrayString, and one is WxArrayInt - the latter
/// just stores the deleted flags values, which are either 0 or 1 and never anything else.
/// When this function is called, the remote server will have completed it's download and
/// the arrays mentioned above will have been populated ready for this function to access
/// them and use their data to update the local KB.
/// NOTE: json requires, for x-platform apps, that we use long or short, not int. So far,
/// json is confined to the KbServer class; and only entryIDs are stored and retrieved as
/// longs, for example using .AsLong(). The deleted flag currently is handled okay by
/// wxArrayInt. The latter is seen by CKB class; but our custom array of longs,
/// Array_of_long (defined near the top of KbServer.h) is not yet exposed to CKB class. If
/// that situation changes, then KB.h will need a similar Array_of_long defined in KB.h
void CKB::StoreEntriesFromKbServer(KbServer* pKbServer)
{
	if (pKbServer == NULL)
	{
		// we don't expect this, but just in case...
		wxString msg;
		msg = msg.Format(_T("StoreEntriesFromKbServer() Error: NULL passed in for pKbServer pointer.\n(Downloaded entries will not be stored in the local KB."));
		wxMessageBox(msg,_T("Error - but program will continue robustly, except for this feature failing"), wxICON_EXCLAMATION | wxOK);
		return;
	}
    // We need the source, target, and deleted flag's arrays - get them; but also get
    // m_arrUsername because we want the local KB to preserve the username for whoever
    // contributed any kbserver's entry which makes it into the local KB from kbserver
	wxArrayInt* pDeletedFlagArray = pKbServer->GetDeletedArray();
	wxArrayString* pKeyArray = pKbServer->GetSourceArray();
	wxArrayString* pTgtArray = pKbServer->GetTargetArray();
	wxArrayString* pUsernameArray = pKbServer->GetUsernameArray();
	wxASSERT(
		pKeyArray->GetCount() == pTgtArray->GetCount() &&
		pTgtArray->GetCount() == pDeletedFlagArray->GetCount() &&
		pTgtArray->GetCount() == pUsernameArray->GetCount()
		); // gotta all be the same count!

	// get the size of any of the above arrays - that's our loop bound; define an iterator
	size_t size = pKeyArray->GetCount();
	size_t index;

	// scratch variables for key, tgtPhrase, and deleted flag, for a single entry
	wxString key;
	wxString tgtPhrase; // could be an adaptation, or in glossing mode, a gloss
	wxString username; // who created a given entry received from kbserver
	bool bDeletedFlag;

	/* don't need this, the this pointer points at the correct CKB already
	// bGlossingKB will be TRUE if the glossing KB is currently in use (ie. glossing mode
	// is on), and FALSE if not in use (ie. adapting mode is on)
	bool bGlossingKB = pKbServer->GetKBServerType() == 2 ? TRUE : FALSE;
	// get a pointer to the CKB instance currently being used
	CKB* pKB = bGlossingKB ? m_pApp->m_pGlossingKB : m_pApp->m_pKB;
	*/
	CKB* pKB = this;

	// local variables needed for storing to the CKB instance
	CTargetUnit* pTU = NULL;
	CRefString* pRefString = NULL;
	int nMapIndex = 0;
	int nWordCount = 0;
	MapKeyStringToTgtUnit* pMap = NULL; // set which map we lookup using nWordCount
	MapKeyStringToTgtUnit::iterator iter;

	// support a progress indicator if size is > 50
	wxString msgDisplayed = _("Merging entries from server");
	wxString progressItem = _T("DownloadingFromKB"); // the user never sees this string
	const int nTotal = size;
	CStatusBar* pStatusBar = NULL;
	pStatusBar = (CStatusBar*)m_pApp->GetMainFrame()->m_pStatusBar;
	int rangeDiv = 10; // divide the range into 10 increments, to show max of 10 updates
	if (size > 50)
	{
		pStatusBar->StartProgress(progressItem, msgDisplayed, nTotal);
	}
	for (index = 0; index < size; index++)
	{
		// handle progress
		if (size > 50)
		{
			if ( (index / rangeDiv) * rangeDiv == index)
			{
				// show progress
				pStatusBar->UpdateProgress(progressItem, index);
			}
		}

		// do the merging of the data
		key = pKeyArray->Item(index);
		tgtPhrase = pTgtArray->Item(index);
		username = pUsernameArray->Item(index);
		bDeletedFlag = pDeletedFlagArray->Item(index) == 1 ? TRUE : FALSE;
		// count how many words there are in key (uses a utility function from helpers.cpp)
		nWordCount = CountSpaceDelimitedWords(key);
		if (nWordCount > 0)
		{
            // NOTE: we do NOT support auto-capitalization in this function by design.
            // Instead, we just look up the key string "as is" to see if a paired
            // CTargetUnit instance is in the map, whether or not autocapitalization is on.
            // So it's possible for one user sharing the KB to have auto caps on, and
            // another have it off, nevertheless we just lookup without checking for case
            // and then doing any needed case conversions. This may result in one user not
            // receiving entries for upper case keys that he'd like to receive but which
            // aren't in the kbserver and so can't be sent, or alternatively, receiving
            // entries for upper case keys which are in kbserver but which he'll never use
            // while he has autocaptalization turned on. So be it. But no damage is done
            // either way.
            nMapIndex = nWordCount - 1;
			pMap = pKB->m_pMap[nMapIndex]; // get the map
			if (pMap != NULL)
			{
				// lookup for a paired pTU, using the string in key...
				pTU = NULL; // default to a failure to find a matching entry
				iter = pMap->find(key); // failure returns pMap->end()
				if (iter != pMap->end())
				{
					pTU = iter->second; // pTU points at a matched CTargetUnit instance
				}

				if (pTU == NULL)
				{
					// The local KB does not yet have an entry for this key; so create it
					pTU = new CTargetUnit;
					MakeAndStoreNewRefString(pTU, tgtPhrase, username, bDeletedFlag);
					// Add the new pTU to the appropriate map
					(*pMap)[key] = pTU;
                    // This new CRefString may have more source text words in it than
                    // any other entry in the local CKB instance; test for this, and if
                    // so, update pKB->m_nMaxWords so that the newly added entry is
                    // lookup-able from now on
					if (pKB->m_nMaxWords < nWordCount)
					{
						pKB->m_nMaxWords = nWordCount;
					}

				} // end of TRUE block for test: if (pTU == NULL)
				else
				{
					// The local KB has this CTargetUnit - so we need to find out if the
					// key is matched by the m_translation member of one of it's
					// CRefString instances. An empty list of CRefString instances in pTU
					// is normally an error, but we'll just add the new entry and that
					// will 'fix' it
					if (pTU->m_pTranslations->IsEmpty())
					{
                        // See the block above's comments for what we are doing here...
                        // the only difference from what's above is that we don't need to
                        // first create a CTargetUnit instance in this current block
						MakeAndStoreNewRefString(pTU, tgtPhrase, username, bDeletedFlag);
						(*pMap)[key] = pTU;
						if (pKB->m_nMaxWords < nWordCount)
						{
							pKB->m_nMaxWords = nWordCount;
						}
					}
					else
					{
						// We must take <Not In KB> entries into account. These
						// can occur in the local KB, but we never propagate them to a
						// kbserver, and so the latter's downloaded entries will never
						// contain them. However, the entry we are about to deal with
						// may be, in this particular local KB, a <Not In KB> entry.
						// In which case, the <Not In KB> status must win, until such
						// time as the user of the local KB removes that status on
						// that entry. So we much check for <Not In KB> and if the pTU
						// current stores such an entry, then we abandon the non-<Not
						// In KB> entry coming from the kbserver
						wxString strNotInKB = _T("<Not In KB>");
						if (pTU->IsItNotInKB())
						{
							// we should not override the user's choice, so we must
							// abandon this entry received from the kbserver
							continue;
						}
						// The m_pTranslations list is not empty, so check if the
						// kbserver entry is already present with the same value of
						// the deleted flag - if so, we abandon this entry as it's
						// already in the local KB; but if absent, or the deleted flag
						// value is different, then we have to update the local KB
						// accordingly.
						bool bIsDeleted = FALSE; // default, actual value returned in next call
						pRefString = GetMatchingRefString(pTU, tgtPhrase, bIsDeleted);

                        // If no match was made, then pRefString will be returned as
						// NULL and bIsDeleted will be FALSE - and so we need to add
						// the new entry to the local KB
						if (pRefString == NULL)
						{
							// No match. So we can added the new entry, and give it's
							// m_bDeleted flag whatever value is in the bDeletedFlag
							// as passed from the kbserver entry's download
							MakeAndStoreNewRefString(pTU, tgtPhrase, username, bDeletedFlag);
							// pTU is already in pMap, and the value of m_nMaxWords is
							// already set correctly, so we've no more to do here
							continue;
						}
                        // On the other hand, if a match of the adaptation or gloss was
                        // made, then pRefString is the matched CRefString instance
                        // which stores it in the local KB, and bIsDeleted will hold
                        // the value of it's m_bDeleted flag which tells us whether or
                        // not this is a normal or a deleted entry in the local KB.
                        // So if we got a match, and the two flags are equal, then the
                        // local KB has this entry from the kbserver, and we can throw
                        // it away and iterate the loop; but if the flags differ, then
                        // we either have to pseudo-delete this local KB entry, or
                        // undelete this local KB entry.
						if (pRefString->m_bDeleted == bDeletedFlag)
						{
							// the local KB has this entry, therefore iterate the loop
							continue;
						}
						else
						{
							// the flags differ in value, so make the required update
							// to the local KB
							if (pRefString->m_bDeleted)
							{
								// the local KB has this entry currently
								// pseudo-deleted, so here we need to undelete it
								pRefString->m_bDeleted = FALSE;
								pRefString->m_refCount = 1;
								pRefString->m_pRefStringMetadata->m_creationDateTime = GetDateTimeNow();
								pRefString->m_pRefStringMetadata->m_deletedDateTime.Empty();
								pRefString->m_pRefStringMetadata->m_modifiedDateTime.Empty();
								pRefString->m_pRefStringMetadata->m_whoCreated = username;
							}
							else
							{
								// the local KB has this entry currently undeleted, so
								// here we need to make it a deleted entry (I'm
								// uncertain which values to retain and which to
								// clear; I think I'll just clear creation and
								// modification times, and whoever caused the deletion
								// should be blameable presumably)
								pRefString->m_bDeleted = TRUE;
								//pRefString->m_refCount = 1; <- leave the old count intact
								pRefString->m_pRefStringMetadata->m_creationDateTime.Empty();
								pRefString->m_pRefStringMetadata->m_deletedDateTime = GetDateTimeNow();
								pRefString->m_pRefStringMetadata->m_modifiedDateTime.Empty();
								pRefString->m_pRefStringMetadata->m_whoCreated = username; // make her blameable
							} // end of else block for test: if (pRefString->m_bDeleted)
						} // end of else block for test: if (pRefString->m_bDeleted == bDeletedFlag)
					} // end of else block for test: if (pTU->m_pTranslations->IsEmpty())
				} // end of else block for test: if (pTU == NULL)
			} // end of TRUE block for test: if (pMap != NULL)
		} // end of TRUE block for test: if (nWordCount > 0)
	} // end of the loop: for (index = 0; index < size; index++)
	pKbServer->ClearAllPrivateStorageArrays(); // we are done with the downloaded kbserver entries

	// clean up the progress indicator
	if (size > 50)
	{
		pStatusBar->FinishProgress(progressItem);
	}

}

// App's m_pKbServer[0] is associated with app's m_pKB; and m_pKbServer[1] is
// associated with m_pGlossingKB. Each CKB has a m_bGlossingKB member, FALSE for an
// adapting CKB, TRUE for a glossing CKB. The latter is used for returning whichever
// of m_pKbServer[0] or [1] is to be associated with the current CKB instance.
// Return NULL if, for any reason, a pointer to the appropriate instantiated KbServer
// instance cannot be obtained
KbServer* CKB::GetMyKbServer()
{
	KbServer* pMyKbSvr = NULL;
	if (m_pApp->m_bIsKBServerProject)
	{
		if (this->m_bGlossingKB)
		{
			// it's a a glossing KB
			if (m_pApp->GetKbServer(2) != NULL)
			{
				return m_pApp->GetKbServer(2);
			}
		}
		else
		{
			// it's an adapting KB
			if (m_pApp->GetKbServer(1) != NULL)
			{
				return m_pApp->GetKbServer(1);
			}
		}
	}
	return pMyKbSvr;
}

/// \return                 nothing
/// \param pTU          ->  pointer to the CTargetUnit instance which is to store
///                         the new CRefString
/// \param tgtPhrase    ->  the adaptation or gloss which is associated with pTU's key
/// \param username     ->  a record of who contributed the entry originally
/// \param bDeletedFlag ->  passes in the value of the kbserver entry's deleted flag
///                         as a boolean (kbserver entry's 0 is here passed as FALSE,
///                          and 1 is passed here as TRUE)
/// \remarks
/// Encapsulates the making of a CRefString added because of a new entry from kbserver.
/// We need this in more than one place, so made a function of it
void CKB::MakeAndStoreNewRefString(CTargetUnit* pTU, wxString& tgtPhrase,
								   wxString& username, bool bDeletedFlag)
{
	CRefString* pRefString = new CRefString(pTU); // automatically creates and hooks
	// up it's owned CRefStringMetadata instance, and sets its creator
	// and creation datetime (creator is initialized to the user
	// account on the local machine, but we then overwrite now with the
	// username received in the kbserver entry)
	pRefString->GetRefStringMetadata()->SetWhoCreated(username);
	// default is to create a normal entry, but if the kbserver
	// entry is a pseudo-deleted one, we have extra work to do
	if (bDeletedFlag)
	{
		pRefString->SetDeletedFlag(TRUE);
		// and give it a deletion datetime of 'now'
		pRefString->GetRefStringMetadata()->SetDeletedDateTime(GetDateTimeNow());
	}
	// set the lowest possible reference count value (a 0 value would
	// result in Adapt It removing the entry, so smallest for persistence
	// is 1)
	pRefString->m_refCount = 1;
	// set the tgtPhrase value (remember, it will be a gloss if
	// glossing mode is current, otherwise it will be an adaptation)
	pRefString->m_translation = tgtPhrase;
	// add the new CRefString instance to this pTU
	pTU->m_pTranslations->Append(pRefString);
	// NOTE, since the entry is not coming from the user's actions in
	// the document, we accept the default value for the m_bForceAsk
	// flag, which is the value FALSE - so nothing to do here
}

/// \return                 the pointer to the matched CRefString instance, or NULL if
///                         none was matched in this particular pTU
/// \param pTU          ->  pointer to the CTargetUnit instance which has one or more
///                         CRefString pointer instances in it's m_pTranslations member
/// \param tgtPhrase    ->  the adaptation or gloss which we are trying to look up in
///                         the list of CRefString instances (their m_translation member)
/// \param bIsDeleted   <-  for passing back the local CRefString instance's m_bDeleted
///                         flag's value ( a boolean, TRUE or FALSE)
/// \remarks
/// Finds if the kbserver entry has a matching <key,tgtPhrase> pair stored in the passed
/// in pTU (the latter is a pointer to a CTargetUnit instance), and it also returns the
/// m_bDeleted flag's value, since the local KB may have this entry as a normal entry, or
/// as a pseudo-deleted one.
CRefString*	CKB::GetMatchingRefString(CTargetUnit* pTU, wxString& tgtPhrase, bool& bIsDeleted)
{
	CRefString* pRefStr = NULL;
	TranslationsList::Node* pos = pTU->m_pTranslations->GetFirst();
	while (pos != NULL)
	{
		pRefStr = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pRefStr != NULL);
		wxString thePhrase = pRefStr->m_translation; // get the local KB's adapt'n or gloss
		// does it match?
		if (thePhrase == tgtPhrase)
		{
			// if we get a match, then also return the value of the m_bDeleted flag
			bIsDeleted = pRefStr->m_bDeleted;
			return pRefStr;
		}
	}
	// if control gets to here, we failed to find a matching entry
	pRefStr = NULL;
	bIsDeleted = FALSE;
	return pRefStr;
}



#endif // for _KBSERVER
//*/

// BEWw added 29Aug11: overloaded version below, for use when Consistency Check
// is being done (return pTU, pRefStr, m_bDeleted flag value by ref)
// Return in the last 3 variables NULL, NULL, and FALSE is no CTargetUnit matched, or no
// matching non-deleted pRefStr matched (returning these saves a second lookup when the
// function returns FALSE)
bool CKB::IsAlreadyInKB(int nWords, wxString key, wxString adaptation,
						CTargetUnit*& pTgtUnit, CRefString*& pRefStr, bool& bDeleted)
{
	pTgtUnit = NULL;
	bDeleted = FALSE;

	// is there a targetunit for this key in the KB?
	bool bFound = FindMatchInKB(nWords,key,pTgtUnit);
	if (!bFound)
	{
		pRefStr = NULL;
		bDeleted = FALSE;
		return FALSE;
	}
	// check if there is a matching adaptation (or gloss if we are calling on a glossing KB)
	// BEW 21Jun10, FindMatchInKB() only returns a pointer to a CTargetUnit instance, and
	// that instance may contain CRefString instances marked as deleted. So matching any
	// of these in the loop below has to be deemed a non-match, so that only matches with
	// the m_bDeleted flag with value FALSE qualify as a match
	TranslationsList::Node* pos = pTgtUnit->m_pTranslations->GetFirst();
	while (pos != 0)
	{
		pRefStr = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pRefStr);
		if (adaptation == pRefStr->m_translation)
		{
			if (!pRefStr->m_bDeleted)
			{
				// not deleted, so this adaptation (or gloss) qualifies as a match,
				// return the pTgtUnit and pRefStr values, and bDeleted as default FALSE
				return TRUE;
			}
			else
			{
				// it's deleted, but return pTgtUnit & pRefStr values, and bDeleted as TRUE
				bDeleted = TRUE;
				return FALSE;
			}
		}
	}
	// return pTgtUnit value, but we didn't make any CRefString, so NULL for pRefStr
	pRefStr = NULL;
	bDeleted = FALSE; // default value
	return FALSE; // did not find a match
}


// BEW 9Jun10, modified to suport kbVersion 2, and also to simplify the parsing code for
// SFM kb import (to remove use of pSrcPhrase) - using code similar to that used for the
// xml parse of a LIFT file being imported
// BEW 8Jun10, added markers and code for support of kbVersion 2 data additions, and for
// support of both LIFT import and \lx &\ge -based SFM KB import
// BEW 13Nov10 changes for supporting Bob Eaton's request for using all tabs in glossing kb
// BEW 17Jul11, bug fix for duplicate entries when an entry has m_deleted flag TRUE
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
	// BEW added 17Jul11
	bool bUndeleting = FALSE;

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
	CStatusBar* pStatusBar = NULL;
	pStatusBar = (CStatusBar*)m_pApp->GetMainFrame()->m_pStatusBar;

	if (kbImportFileOfType == KBImportFileOfLIFT_XML)
	{
		// we're importing from a LIFT file
		wxFile f;
		wxLogNull logno; // prevent unwanted system messages
		// (until wxLogNull goes out of scope, ALL log messages are suppressed - be warned)

		// whm 26Aug11 Open a wxProgressDialog instance here for importing LIFT operations.
		// The dialog's pProgDlg pointer is passed along through various functions that
		// get called in the process.
		// whm WARNING: The maximum range of the wxProgressDialog (nTotal below) cannot
		// be changed after the dialog is created. So any routine that gets passed the
		// pProgDlg pointer, must make sure that value in its Update() function does not
		// exceed the same maximum value (nTotal).
		wxString msgDisplayed;
		wxString progMsg;
		// add 1 chunk to insure that we have enough after int division above
		const int nTotal = m_pApp->GetMaxRangeForProgressDialog(XML_Input_Chunks) + 1;
		// Only show the progress dialog when there is at lease one chunk of data
		// Only create the progress dialog if we have data to progress
		if (nTotal > 0)
		{
			progMsg = _("Reading file %s - part %d of %d");
			wxFileName fn(pathName);
			msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str(),1,nTotal);
			pStatusBar->StartProgress(_("Importing LIFT Records to the Knowledge Base"), msgDisplayed, nTotal);
		}

		if( !f.Open( pathName, wxFile::read))
		{
			wxMessageBox(_("Unable to open import file for reading."),
		  _T(""), wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(_T("Unable to open import file for reading."));
			if (nTotal > 0)
			{
				pStatusBar->FinishProgress(_("Importing LIFT Records to the Knowledge Base"));
			}
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
			bReadOK = ReadLIFT_XML(pathName,m_pApp->m_pGlossingKB,(nTotal > 0) ? _("Importing LIFT Records to the Knowledge Base") : _T(""),nTotal);
		}
		else
		{
			bReadOK = ReadLIFT_XML(pathName,m_pApp->m_pKB,(nTotal > 0) ? _("Importing LIFT Records to the Knowledge Base") : _T(""),nTotal);
		}
		if (!bReadOK)
		{
			f.Close();
			if (nTotal > 0)
			{
				pStatusBar->FinishProgress(_("Importing LIFT Records to the Knowledge Base"));
			}
		}
		wxCHECK_RET(bReadOK, _T("DoKBImport(): ReadLIFT_XML() returned FALSE, line 10,97 or 1101 in KB.cpp, KB LIFT import was not done"));
		f.Close();
		// remove the progress dialog
		if (nTotal > 0)
		{
			pStatusBar->FinishProgress(_("Importing LIFT Records to the Knowledge Base"));
		}
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
			m_pApp->LogUserAction(_T("Unable to open import file for reading."));
			return;
		}

		// whm 26Aug11 Open a wxProgressDialog instance here for save operations.
		// The dialog's pProgDlg pointer is passed along through various functions that
		// get called in the process.
		// whm WARNING: The maximum range of the wxProgressDialog (nTotal below) cannot
		// be changed after the dialog is created. So any routine that gets passed the
		// pProgDlg pointer, must make sure that value in its Update() function does not
		// exceed the same maximum value (nTotal).
		wxString msgDisplayed;
		const int nTotal = file.GetLineCount() + 1; // total count of lines is the maximum value + 1
		wxString progMsg = _("%s  - %d of %d Total Input Lines");
		wxFileName fn(pathName);
		msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str(),0,nTotal);
		pStatusBar->StartProgress(_("Importing SFM Records to the Knowledge Base"), msgDisplayed, nTotal);

		// For SFM import we are using wxTextFile which has already loaded its entire contents
		// into memory with the Open call in OnImportToKb() above. wxTextFile knows how to
		// handle Unicode data and the different end-of-line characters of the different
		// platforms.
		// Since the entire file is now in memory we can read the information by
		// scanning its virtual lines. In this routine we use the "direct access" method of
		// retrieving the lines from storage in memory, using GetLine(ct).
		int lineCount = file.GetLineCount();

		// whm added 15Jul11 counts for report at end of import
		int nLexItemsProcessed = 0;
		int nAdaptationsProcessed = 0;
		int nAdaptationsAdded = 0;
		int nAdaptationsUnchanged = 0;
		int nDelItems = 0;
		int nUndeletions = 0;

		int ct;
		int nWordCount;
		MapKeyStringToTgtUnit* pMap; // pointer to the map to use for a given entry
		CTargetUnit* pTU = NULL;
		CRefString* pRefStr = NULL;
		int numWords = 0;
		for (ct = 0; ct < lineCount; ct++)
		{
			line = file.GetLine(ct);

			// update the progress bar every 20th line read
			if (ct % 20 == 0)
			{
				msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str(),ct,nTotal);
				pStatusBar->UpdateProgress(_("Importing SFM Records to the Knowledge Base"), ct, msgDisplayed);
			}

			// for debugging
//#ifdef _ENTRY_DUP_BUG
//#ifdef _DEBUG
//			int anOffset1 = line.Find(_T("the"));
//			int anOffset2 = line.Find(_T("jHN"));
//			if  (anOffset1 != wxNOT_FOUND || anOffset2 != wxNOT_FOUND)
//			{
//				int halt_here = 1;
//			}
//#endif
//#endif
			// the data for each line is now in lineStr
			// is the line a m_key member?
			if (IsMember(line,keyMarker,nOffset) || nOffset >= 0)
			{
				nLexItemsProcessed++;

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
					bUndeleting = FALSE; // after ending the parse of CRefString this flag
										 // must be reinitialized to FALSE - so do it here
					nAdaptationsProcessed++;

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
                            nAdaptationsAdded++;

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
							KB_Entry rsEntry = GetRefString(pTU, adaption, pRefStr); // returns
									// pRefStr NULL if there was no CRefString instance with
									// matching m_translation == adaption, and returns in rsEntry
									// either absent, present_but_deleted, or really_present
							if (pRefStr == NULL && rsEntry == absent)
							{
								nAdaptationsAdded++;

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
							else if (pRefStr != NULL && rsEntry == present_but_deleted)
							{
								// this one was deleted, so now we must undelete it --
								// we'll handle all metadata fields here right now, since
								// some may be lacking (such as \mdt and/or \ddt) in the
								// imported file's sfm data and the metadata block blow
								// won't make the required changes for those in that case,
								// so we do it all here, and let the metadata block
								// redundantly redo any of those that occur in the import
								// record (the nUndeletions count, however, has to be done
								// in the metadata block below, as nDelItems is done there
								// too)
								pRefStr->m_refCount = 1; // initialize to 1
								bUndeleting = TRUE; // this entry is being undeleted
								// additional changes are done redundantly in the metadata
								// block below, using the bUndeletions == TRUE value
								pRefStr->m_bDeleted = FALSE;
								pRefStr->m_pRefStringMetadata->m_whoCreated = SetWho(); // will be overrided below
								pRefStr->m_pRefStringMetadata->m_creationDateTime = GetDateTimeNow();
								pRefStr->m_pRefStringMetadata->m_modifiedDateTime.Empty(); // initialize to empty
								pRefStr->m_pRefStringMetadata->m_deletedDateTime.Empty(); // initialize to empty
							}
							else
							{
                                // this particular adaptation or gloss is in the map's
                                // CTargetUnit pointer already, so we should ignore it; to
                                // do that we just need to set pRefStr to NULL, so that
								// parsing of subsequent metadata lines in the code
								// further down will ignore this CRefString and its metadata
								wxASSERT(rsEntry == really_present);
								nAdaptationsUnchanged++;
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
								if (bUndeleting)
								{
									// mark it as not deleted
									nUndeletions++; // count undeletions with a separate counter than nDelItems
									pRefStr->m_bDeleted = FALSE;
								}
								else
								{
									// it remains a deleted entry, or undeleted, as the
									// case may be
									// whm 11Jun12 added !delStr.IsEmpty() && to test below. GetChar(0)
									// should not be called on an empty string.
									if (!delStr.IsEmpty() && delStr.GetChar(0) == _T('1'))
									{
										nDelItems++; // count deletions that remain deletions
										pRefStr->m_bDeleted = TRUE;
									}
									else
									{
										pRefStr->m_bDeleted = FALSE; // don't count undeleted items
												// which remain undeleted, as these are counted
												// in the Number of Adaptations/Glosses count
									}
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
							if (bUndeleting)
							{
								// when undeleting, give it the current time
								pRefStr->m_pRefStringMetadata->m_creationDateTime = GetDateTimeNow();
							}
							else
							{
								if (!createDTStr.IsEmpty())
								{
									pRefStr->m_pRefStringMetadata->m_creationDateTime = createDTStr;
								}
								else
								{
									// if no creation datetime was supplied, give it current datetime
									pRefStr->m_pRefStringMetadata->m_creationDateTime = GetDateTimeNow();
								}
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
							if (bUndeleting)
							{
								// if undeleting, any modification dateTime would be
								// meaningless, so empty it
								pRefStr->m_pRefStringMetadata->m_modifiedDateTime.Empty();
							}
							else
							{
								if (!modDTStr.IsEmpty())
								{
									pRefStr->m_pRefStringMetadata->m_modifiedDateTime = modDTStr;
								}
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
							if (bUndeleting)
							{
								// remove the deletion dateTime now that we are making
								// this a really_present entry
								pRefStr->m_pRefStringMetadata->m_deletedDateTime.Empty();
							}
							else
							{
								if (!delDTStr.IsEmpty())
								{
									pRefStr->m_pRefStringMetadata->m_deletedDateTime = delDTStr;
								}
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
		pStatusBar->FinishProgress(_("Importing SFM Records to the Knowledge Base"));

		// provide the user with a statistics summary
		wxString msg = _("Summary:\n\nNumber of lexical items processed %d\nNumber of Adaptations/Glosses Processed %d\nNumber of Adaptations/Glosses Added %d\nNumber of Adaptations Unchanged %d\nNumber of Deleted Items Unchanged %d\nNumber of Undeletions done %d ");
		msg = msg.Format(msg,nLexItemsProcessed, nAdaptationsProcessed, nAdaptationsAdded, nAdaptationsUnchanged, nDelItems, nUndeletions);
		wxMessageBox(msg,_T("KB Import Results"),wxICON_INFORMATION | wxOK);
		m_pApp->LogUserAction(msg);
	} // end importing from an SFM text file
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

// The following is a helper for Consistency Check. If an adaptation was deleted from the
// KB because the location was made <Not In KB>, then m_bHasKBEntry is FALSE, m_bNotInKB
// is TRUE, and if the doc retains the word or phrase in its m_adaption member, there can
// be a false positive happen in the consistency check -- the lookup of the KB finds the
// word or phrase's CTargetUnit, and detects that pRefStr returns as NULL because bDeleted
// returns TRUE - which would trigger the revamped legacy ConsistencyCheckDlg to be shown
// if not for the following function which allows us to check for an active (ie.
// non-deleted) "<Not In KB"> entry in this pTU instance
// Returns TRUE if a non-deleted <Not IN KB> entry is found in this pTU, else FALSE
// BEW created 8 Sept 2011
bool CKB::IsNot_In_KB_inThisTargetUnit(CTargetUnit* pTU)
{
	wxASSERT(pTU != NULL);
	// the following call only searches the non-deleted CRefString instances in pTU
	int nFound =  pTU->FindRefString(m_pApp->m_strNotInKB);
	return nFound != wxNOT_FOUND;
}

// The following is a helper for Consistency Check. Supporting the basic protocol that a
// choice to make a source text word or phrase be "Not In KB" results in any non-<Not In
// KB> entries in the associated CTargetUnit instance become all deleted, means that we
// need a function which allows us to take a returned CTargetUnit pointer and determine if
// there is a deleted CRefString in it with the m_translation value <Not In KB>. That's
// what this function does. It returns TRUE if that is the case, it returns FALSE if a
// non-deleted <Not In KB> is in the target unit, or any other CRefString with any other
// m_translation value as well.
// BEW created 9 Sept 2011
bool CKB::IsDeleted_Not_In_KB_inThisTargetUnit(CTargetUnit* pTU)
{
	wxASSERT(pTU != NULL);
	// the following call only searches the deleted CRefString instances in pTU for a match
	int nFound =  pTU->FindDeletedRefString(m_pApp->m_strNotInKB);
	return nFound != wxNOT_FOUND;
}

// Checks the CRefString instances in whatever CTargetUnit instance is looked up by using
// the pSrcPhrase->m_key, the pSrcPhrase->m_nSrcWords to select the map, and whatever
// adaptation or gloss is passed in in the str param.
//
// Returns TRUE if there was an undeletion made (or a non-deleted matching CRefString -
// which must not be a <Not In KB> one) was found; FALSE otherwise. If a CTargetUnit is
// found for the key, then any <Not In KB> CRefString within it is guaranteed to have
// been rendered "deleted". A value of FALSE returned should be interpretted as meaning
// that there was no CTargetUnit matched, or if one was matched, no deleted matching
// CRefString was found. In either case, a StoreText() should then be done - and then you
// can be sure that <Not In KB> will have been appropriately made deleted if it was
// present. If not present, all is well anyway.
//
// The pTU parameter is the pointer to the CTargetUnit to be acted on. If known in advance,
// pass it in as the second param; if not known in advance, then pass in NULL - in which
// case a lookup will be done in order to get the correct instance
//
// Warning 1: don't call this function with "<Not In KB>" as the value in str
//
// Warning 2: it's up to you to ensure that a gloss is passed in for the str parameter when
// a glossing KB is being accessed, or an adaptation is pass when the KB is an adaptation
// one
//
// Usage: make the call, and if FALSE is returned, call StoreText(pSrcPhrase,str), or if
// str is an empty string, StoreText(pSrcPhrase,str,TRUE) to get your store done; and you
// can be certain that any "<Not In KB>" has been appropriately dealt with.
//
// Needed for doc's DoConsistencyCheck() to help fix "<Not In KB>" inconsistencies when
// found, when the user requests that a former <Not In KB> choice be changed to a "do a
// normal store" on a CTargetUnit which has <Not In KB> either as the only non-deleted
// CRefString, or a deleted <Not In KB> as one of the CRefStrings, or even no CRefString
// with <Not In KB> in its m_translation member.
// Note: this function is intended for adaptation mode, but can be safely called on a
// glossing KB's CTargetUnit - the "<Not In KB>" support would just waste a bit of time
bool CKB::UndeleteNormalEntryAndDeleteNotInKB(CSourcePhrase* pSrcPhrase, CTargetUnit* pTU,
											  wxString& str)
{
	wxASSERT(pSrcPhrase != NULL && !pSrcPhrase->m_key.IsEmpty() &&
			str != m_pApp->m_strNotInKB);
	wxString theKey = pSrcPhrase->m_key;
	wxString thePhrase;
	// which CKB type are we?
	if (m_bGlossingKB)
	{
		thePhrase = pSrcPhrase->m_gloss; // can be empty
	}
	else
	{
		thePhrase = pSrcPhrase->m_adaption; // can be empty
	}
	if (pTU == NULL)
	{
		// look it up (returns NULL if a matching one can't be found in the map)
		// (internally, GetTargetUnit() does any needed auto-caps adjustments
		pTU = GetTargetUnit(pSrcPhrase->m_nSrcWords, thePhrase);
	}
	if (pTU == NULL)
		return FALSE;
	else
	{
		// the call below requires auto-caps adjustments be done here first
	wxString s = str;
	bool bSetOK = m_pApp->GetDocument()->SetCaseParameters(thePhrase, FALSE); // FALSE is bIsSrc
	if (bSetOK)
	{
		// if gbAutoCaps is FALSE, it will just return thePhrase unchanged
		thePhrase = AutoCapsMakeStorageString(thePhrase, FALSE);
	}
		return pTU->UndeleteNormalCRefStrAndDeleteNotInKB(thePhrase);
	}
}

// returns TRUE if, for the pSrcPhrase->m_key key value, the KB (adaptation KB only, the
// caller will not call this function if the mode is glossing mode) contains a CTargetUnit
// instance which has a CRefString instance which is storing "<Not In KB>" in a CRefString
// which is not marked as deleted. Else it returns FALSE.
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
	CTargetUnit* pTU = NULL;
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
			return pTU->IsItNotInKB() == TRUE;
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
	CStatusBar* pStatusBar = NULL;
	pStatusBar = (CStatusBar*)m_pApp->GetMainFrame()->m_pStatusBar;

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
		if ( m_pApp->m_sourceLanguageCode.IsEmpty() || m_pApp->m_targetLanguageCode.IsEmpty()
			|| m_pApp->m_sourceLanguageCode == NOCODE || m_pApp->m_targetLanguageCode == NOCODE )
														// mrh June 2012 - account for NOCODE as well as empty
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
                // BEW comment added 25Jul12, the language code dialog allows the user to
                // setup codes for src, tgt, gloss and/or free translation languages; but
                // here we will not pick up and store any code settings he may make for the
                // latter two possibilities because it is only the src & tgt codes we are
                // interested in for KB exports
				int returnValue = lcDlg.ShowModal();
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
					int response = wxMessageBox(message, _("Language code(s) missing"), wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);
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
		// by GetEncodingStringForSmlFiles() below as <?xml version="1.0" encoding="utf-8" standalone="yes"?>
		m_pApp->GetEncodingStringForXmlFiles(xmlPrologue); // builds xmlPrologue and adds "\r\n" to it
		composeXmlStr = xmlPrologue; // first string in xml file
		CBString openLiftTag = "<lift version=\"0.13\">";
		composeXmlStr += openLiftTag;
		composeXmlStr += "\r\n";
		DoWrite(*pFile,composeXmlStr);
	}

	// whm 26Aug11 Open a wxProgressDialog instance here for exporting kb operations.
	// The dialog's pProgDlg pointer is passed along through various functions that
	// get called in the process.
	// whm WARNING: The maximum range of the wxProgressDialog (nTotal below) cannot
	// be changed after the dialog is created. So any routine that gets passed the
	// pProgDlg pointer, must make sure that value in its Update() function does not
	// exceed the same maximum value (nTotal).
	wxString titleStr;
	if (kbExportSaveAsType == KBExportSaveAsLIFT_XML)
	{
		titleStr = _("Exporting the KB in LIFT format");
	}
	else
	{
		titleStr = _("Exporting the KB in SFM format");
	}
	wxString msgDisplayed;
	int nTotal;
	wxString fileNamePath;
	if (m_bGlossingKB)
	{
		nTotal = m_pApp->GetMaxRangeForProgressDialog(Glossing_KB_Item_Count) + 1;
		fileNamePath = m_pApp->m_curGlossingKBPath;
	}
	else
	{
		nTotal = m_pApp->GetMaxRangeForProgressDialog(Adapting_KB_Item_Count) + 1;
		fileNamePath = m_pApp->m_curKBPath;
	}
	wxString progMsg = _("Exporting KB %s  - %d of %d Total entries and senses");
	wxFileName fn(fileNamePath);
	msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str(),1,nTotal);
	pStatusBar->StartProgress(titleStr, msgDisplayed, nTotal);

	bool bSuppressDeletionsInSFMexport = FALSE; // default is to export everything for SFM export
	if (kbExportSaveAsType == KBExportSaveAsSFM_TXT)
	{
		wxString message;
		message = message.Format(_("Deleted entries are kept in the knowledge base but are hidden. Do you want these included in the export?\n(Click No only if you intend to later import the data to a legacy version of Adapt It, otherwise click Yes.)"));
		int result = wxMessageBox(message,_("How should deleted entries be handled?"), wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);
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
					if (pTU != NULL) // whm 11Jun12 added NULL test
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
					if (counter % 20 == 0)
					{
						msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str(),counter,nTotal);
						pStatusBar->UpdateProgress(titleStr, counter, msgDisplayed);
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
				if (counter % 20 == 0)
				{
					msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str(),counter,nTotal);
					pStatusBar->UpdateProgress(titleStr, counter, msgDisplayed);
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
	pStatusBar->FinishProgress(titleStr);
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

#if defined(_KBSERVER)
// Looks up the local knowledge base to find if there is a CTargetUnit pointer entry in the
// map with index nSrcWords-1, for the key keyStr, and returns the CTargetUnit pointer it
// finds. If it fails, it returns a null pointer. Since no CSourcePhrase is involved, the
// number of words is determined by tokenizing the keyStr, and finding out how many tokens
// are involved. Since we deal with keys, empty string is not going to ever be present, so
// we pass FALSE to SmartTokenize() for its final parameter.
// BEW created 14Nov12
CTargetUnit* CKB::GetTargetUnitForKbSharing(wxString keyStr)
{
	int nSrcWords = 1;
	wxString delimiters = _T(' '); // the only delimiter we need is space character
	wxArrayString arrStr;
	arrStr.Clear();
	long numWordTokens = SmartTokenize(delimiters, keyStr, arrStr, FALSE);
	nSrcWords = (int)numWordTokens;
	MapKeyStringToTgtUnit* pMap = m_pMap[nSrcWords-1];
	wxASSERT(pMap != NULL);
	CTargetUnit* pTgtUnit;
	bool bOK = LookupForKbSharing(pMap, pTgtUnit, keyStr);
	if (bOK)
	{
		wxASSERT(pTgtUnit);
		return pTgtUnit; // we found it
	}
    // lookup failed, so return NULL pointer
	return (CTargetUnit*)NULL;
}

/*
/////////////////////////////////////////////////////////////////////////////////////////
/// \return         TRUE if translation param && deletedFlag params' values are
///                 identical to those in a single CRefString within pTU, FALSE
///                 if one or both do not match.
/// \param pTU  ->  pointer to a CTargetUnit looked up in the caller by a successful
///                 call to LookupForKbSharing() - it may contain one or more CRefString
///                 instances
/// \param translation -> ref to a string (it may have multiple words, space delimited, or
///                 even might be an empty string - for a <no adaptation> entry) which is
///                 paired to the source text key that the caller used for the lookup; the
///                 translation string could be target text, or glossing text (if the latter
///                 then it may have punctuation in it - we allow punctuation only in the
///                 glossing KB) - the origin of the translation string most likely is
///                 from a kbserver on the LAN or the web and some kind of GET has just
///                 been done
/// \param deletedFlag -> the value (0 is FALSE, 1 is TRUE) of the deleted flag for the
///                 pair being considered for a match (pair being the key and the
///                 translation string, but we also want to check for a match or non-match
///                 of the deleted flag as well - the latter info is needed so we can
///                 determine what the appropriate action is to be in the local KB
/// \param pRefString <- Set to pointer to the matched CRefString instance in pTU, or NULL
///                 if no match could be made to any CRefString in pTU.
/// \param bMatchedTranslation <- Set TRUE if the passed in translation string matches
///                 the m_translation member's contents within a matched CRefString within
///                 the passed in pTU; FALSE if the two strings did not match.
/// \remarks        If TRUE is returned, then both translation and deletedFlag values
///                 are matched - so we've then matched either a normal entry, or a deleted
///                 entry. If FALSE is returned, we must test further, and check the value
///                 of bMatchedTranslation. If the latter is TRUE, then we have matched a
///                 CRefString in which the m_bDeleted flag has the opposite value to
///                 whatever value the deletedFlag param had on entry; otherwise, if
///                 bMatchedTranslation is FALSE, we've not matched any CRefString, and
///                 the value of bMatchedTranslation should be ignored. (In the latter
///                 case, the caller should use the translation value to create a new
///                 CRefString instance and Append() it to the list in pTU, and give the
///                 new CRefString instance's m_bDeleted flag the value within deletedFlag
///                 after casting the latter to bool.)
/////////////////////////////////////////////////////////////////////////////////////////
bool CKB::IsMatchForKbSharing(CTargetUnit* pTU, wxString& translation,
					int deletedFlag, CRefString*& pRefString, bool& bMatchedTranslation)
{


// so far, I don't need this. Use CTargetUnit::FindRefStringForKbSharing() and
// FindDeletedRefStringForKbSharing() instead
	return TRUE;
}
*/

#endif // for _KBSERVER #defined

// This is the inner workings of the handler OnCheckKBSave() -- the latter being called
// when the user clicks the GUI checkbox "Save to knowledge base" (the latter is checked
// by default, it takes a user click to uncheck it, and that results in "<Not In KB>"
// replacing the adaptations (or glosses) for the CTargetUnit instance associated with the
// source text key at that location. The other function which calls DoNotInKB() is the
// call of RedoStorage().
// BEW 18Jun10, changes made for supporting kbVersion 2
// BEW 13Nov10, no changes to support Bob Eaton's request for glosssing KB to use all maps
// BEW 17Jul11, changed to support the return of enum KB_Entry, with values absent,
// present_but_deleted, or really_present, from GetRefString()
// BEW 14Sep11, cleaned up misleading comments & faulty code, and StoreText() similarly
// fixed up internally at same time
void CKB::DoNotInKB(CSourcePhrase* pSrcPhrase, bool bChoice)
{
	wxString strNotInKB = m_pApp->m_strNotInKB; // "<Not In KB>"
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

		// we make it's KB translation be a unique "<Not In KB>" - Adapt It will use this
		// as a flag
		bool bOK;
		bOK = StoreText(pSrcPhrase,strNotInKB);
		bOK = bOK; // avoid warning
		// make the flags the correct values & save them on the source phrase
		// BEW 14Sep11, next 2 lines are redundant, StoreText() now gets it right
		pSrcPhrase->m_bNotInKB = TRUE;
		pSrcPhrase->m_bHasKBEntry = FALSE;

        // user can set pSrcPhrase->m_adaption to whatever he likes via phrase box, it
        // won't go into KB, and it now (no longer) will get clobbered, since we now follow
        // Susanna Imrie's recommendation that this feature should still allow a non-null
        // translation to remain in the document
	}
	else
	{
		pSrcPhrase->m_bNotInKB = FALSE;
		pSrcPhrase->m_bHasKBEntry = FALSE; // make sure, because then when the phrase box
										   // moves, the adaptation will be stored and
										   // this flag set TRUE
		CRefString* pRefString = NULL;
		// get the currently non-deleted "<Not In KB>" string's CRefString
		KB_Entry rsEntry = GetRefString(pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key,
										strNotInKB, pRefString);
		if (pRefString == NULL && rsEntry == absent)
		{
            // it's not present, so our work is done
			return;
		}
		wxASSERT(pRefString);
		// the two possibilities are that rsEntry is present_but_deleted, or rsEntry is
		// really_present. If the former is the case, it's already "deleted" so we need
		// only undelete any other refstring instances, if the latter, we must make it be
		// deleted and undelete the rest (if any)
		if (pRefString != NULL)
		{
			// BEW 18Jun10, for kbVersion 2, we must undo any deletions we made earlier
			// when we set up the <Not In KB> entry, and make the CRefString with
			// translation text string "<Not In KB>" become the deleted one

			// get the parent CTargetUnit instance
			CTargetUnit* pTgtUnit = pRefString->m_pTgtUnit;
			wxASSERT(pTgtUnit);
			TranslationsList* pList = pTgtUnit->m_pTranslations;
			wxASSERT(!pList->IsEmpty());
			TranslationsList::Node* pos = pList->GetFirst();
			// BEW 18Jun10 the new code follows..., first, scan through all CRefString
			// instances in the list and any with m_bDeleted set TRUE, undelete them; and
			// the one which matches pRefString above must be given special tests
			while (pos != NULL)
			{
				CRefString* pRefStr = (CRefString*)pos->GetData();
				pos = pos->GetNext();
				if (pRefStr == pRefString)
				{
					if (rsEntry == present_but_deleted)
					{
						// our work is done for this one, it's deleted already
						;
					}
					else
					{
						wxASSERT(rsEntry == really_present);

						// make the <Not In KB> entry become the deleted one
						pRefString->m_bDeleted = TRUE;
						pRefString->m_pRefStringMetadata->m_deletedDateTime = GetDateTimeNow();
					}
				}
				else
				{
					// it's not the "<Not In KB>" one we matched above
					if (pRefStr != NULL && pRefStr->m_bDeleted)
					{
						pRefStr->m_bDeleted = FALSE;
						pRefStr->m_pRefStringMetadata->m_deletedDateTime.Empty();
						// we could leave the old creation datetime intact, but since the
						// entry was 'deleted' it is probably more appropriate to give it
						// the current time as it's (re-)creation datetime
						pRefString->m_pRefStringMetadata->m_creationDateTime = GetDateTimeNow();
					}
				}
			} // end of while loop
		} // end of TRUE block for test: if (pRefString != NULL)
	} // end of else block for test: if (bChoice)
}

inline int CKB::GetCurrentKBVersion()
{
	return m_kbVersionCurrent;
}

inline void CKB::SetCurrentKBVersion()
{
	m_kbVersionCurrent = (int)KB_VERSION3; // KB_VERSION3 is defined in Adapt_ItConstants.h
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
			wxMessageBox(str, _T(""), wxICON_EXCLAMATION | wxOK);
		}
		if (pK != NULL) // whm 11Jun12 added NULL test
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
// BEW 29Jul11, removed a cause for duplicate entries to be formed in a CTargetUnit instance
// BEW 14Sep11, updated to reflect the improved code in StoreText()
// BEW 17Oct11, updated to turn off app flag m_bForceAsk before returning (but always
// after having used the TRUE value if it's value on entry was TRUE)
bool CKB::StoreTextGoingBack(CSourcePhrase *pSrcPhrase, wxString &tgtPhrase)
{
	// determine the auto caps parameters, if the functionality is turned on
	bool bNoError = TRUE;
	wxString strNot = m_pApp->m_strNotInKB;
	bool bStoringNotInKB = (strNot == tgtPhrase);
	if (gbAutoCaps)
	{
		bNoError = m_pApp->GetDocument()->SetCaseParameters(pSrcPhrase->m_key); // for source word or phrase
	}

	m_pApp->GetDocument()->Modify(TRUE);

    // do not permit storage, when going back, if the source phrase has an empty key
	if (pSrcPhrase->m_key.IsEmpty())
	{
		gbMatchedKB_UCentry = FALSE;
		m_pApp->m_bForceAsk = FALSE; // must be turned off before next location arrived at
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
	// for safety, make this check first
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
	}
	else
	{
		if (tgtPhrase != strNot)
		{
      // BEW 29Jul11 added auto-caps code here so that m_adaption gets set in the
      // same way that MakeTargetStringIncludingPunctuation() will do the
      // capitalization for m_targetStr, formerly m_adaption was just set to
      // tgtPhrase no matter whether auto-caps was on or off (and we didn't notice
      // because the view only shows m_targetStr)
			wxString s = tgtPhrase;
			if (gbAutoCaps)
			{
				bool bNoError = TRUE;
				if (gbSourceIsUpperCase && !gbMatchedKB_UCentry)
				{
					bNoError = m_pApp->GetDocument()->SetCaseParameters(s,FALSE); // FALSE is bIsSrcText
					if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
					{
						// change it to upper case
						s.SetChar(0,gcharNonSrcUC);
					}
				}
				pSrcPhrase->m_adaption = s;
			}
			else
			{
				pSrcPhrase->m_adaption = tgtPhrase;
			}
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
		if (IsFixedSpaceSymbolWithin(pSrcPhrase))
		{
			nMapIndex = 0;
		}
		else
		{
			nMapIndex = pSrcPhrase->m_nSrcWords - 1; // index to the appropriate map
		}
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
		m_pApp->m_bForceAsk = FALSE; // make sure it is now turned off
		wxMessageBox(_(
"Warning: there are too many source language words in this phrase for this adaptation to be stored in the knowledge base.")
		, _T(""), wxICON_INFORMATION | wxOK);
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
			if (gbAutoCaps)
			{
				pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);
			}
			else
			{
				pRefString->m_translation = tgtPhrase;
			}
		}
		else
		{
			// if something went wrong, just save as if gbAutoCaps was FALSE
			pRefString->m_translation = tgtPhrase;
		}
//*
#if defined(_KBSERVER)
    // BEW added 5Oct12, here is a suitable place for kbserver support of
    // CreateEntry(), since both the key and the translation (both possibly with a case
    // adjustment for the first letter) are defined.
    // Note: we can't reliably assume that the newly typed translation or gloss has not
    // been, independently by some other user, added to the kbserver already, and also
    // subsequently deleted by him before the present; therefore we must test for the
    // absence of this src/tgt pair and only upload if the entry really is going to be
    // a new one.
		if (m_pApp->m_bIsKBServerProject &&
			m_pApp->GetKbServer(m_pApp->GetKBTypeForServer())->IsKBSharingEnabled())
		{
			KbServer* pKbSvr = m_pApp->GetKbServer(m_pApp->GetKBTypeForServer());

			// don't send to kbserver if it's a <Not In KB> entry
			if (!bStoringNotInKB)
			{
				// Here's where I'll test doing this on a thread
        /* no -- too restrictive
				// BEW changed 4Feb13 to use KbServer sublassed off of wxThreadHelper (to avoid link errors)
        pKbSvr->m_bUseCache = pKbSvr->IsCachingON(); // currently it's OFF (ie. FALSE)
				pKbSvr->m_source = key;
				pKbSvr->m_translation = pRefString->m_translation;
				// now create the runnable thread with explicit stack size of 10KB; it's a default
				// kind of thread ("detached") so will destroy itself when it's done
				wxThreadError error =  pKbSvr->Create(10240);
				if (error != wxTHREAD_NO_ERROR)
				{
					wxString msg;
					msg = msg.Format(_T("Thread_CreateEntry(): thread creation failed, error number: %d"),
						(int)error);
					wxMessageBox(msg, _T("Thread creation error"), wxICON_EXCLAMATION | wxID_OK);
					m_pApp->LogUserAction(msg);
				}
				else
				{
          // no error, so run it
          error = pKbSvr->GetThread()->Run();
          if (error != wxTHREAD_NO_ERROR)
          {
            wxString msg;
            msg = msg.Format(_T("Thread_Run(): cannot make the thread run, error number: %d"),
              (int)error);
            wxMessageBox(msg, _T("Thread start error"), wxICON_EXCLAMATION | wxID_OK);
            m_pApp->LogUserAction(msg);
          }
				}
        */
				Thread_CreateEntry* pCreateEntryThread = new Thread_CreateEntry;
				// populate it's public members (it only has public ones anyway)
				pCreateEntryThread->m_pKB = this;
				//pCreateEntryThread->m_pKbSvr = pKbSvr;
				pCreateEntryThread->m_kbServerType = m_pApp->GetKBTypeForServer();
				pCreateEntryThread->m_bUseCache = pKbSvr->IsCachingON(); // currently it's OFF (ie. FALSE)
				pCreateEntryThread->m_source = key;
				pCreateEntryThread->m_translation = pRefString->m_translation;
				// now create the runnable thread with explicit stack size of 10KB
				wxThreadError error =  pCreateEntryThread->Create(10240);
				if (error != wxTHREAD_NO_ERROR)
				{
					wxString msg;
					msg = msg.Format(_T("Thread_CreateEntry(): thread creation failed, error number: %d"),
						(int)error);
					wxMessageBox(msg, _T("Thread creation error"), wxICON_EXCLAMATION | wxID_OK);
					m_pApp->LogUserAction(msg);
				}
        else
        {
          // no error, so now run the thread (it will destroy itself when done)
          error = pCreateEntryThread->Run();
          if (error != wxTHREAD_NO_ERROR)
          {
            wxString msg;
            msg = msg.Format(_T("Thread_Run(): cannot make the thread run, error number: %d"),
              (int)error);
            wxMessageBox(msg, _T("Thread start error"), wxICON_EXCLAMATION | wxID_OK);
            m_pApp->LogUserAction(msg);
          }
        }

				/*
				bool bHandledOK = HandleNewPairCreated(m_pApp->GetKBTypeForServer(),
							key, pRefString->m_translation, pKbSvr->IsCachingON());

				// I've not yet decided what to do with the return value, at present we'll
				// just ignore it even if FALSE (an internally generated message would have
				// been seen anyway in that event)
				bHandledOK = bHandledOK; // avoid compiler warning
				*/
			}
		}
#endif
//*/
		pTU->m_pTranslations->Append(pRefString); // store in the CTargetUnit
		if (m_pApp->m_bForceAsk)
			pTU->m_bAlwaysAsk = TRUE; // turn it on if user wants to be given
				// opportunity to add a new refString next time its matched
		if (m_bGlossingKB)
			pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
		else
		{
			if (bStoringNotInKB)
			{
				pSrcPhrase->m_bHasKBEntry = FALSE;
				pSrcPhrase->m_bNotInKB = TRUE;
			}
			else
			{
				pSrcPhrase->m_bHasKBEntry = TRUE;
			}
		}
		(*m_pMap[nMapIndex])[key] = pTU; // store the CTargetUnit instance in the map
		if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
		{
			m_nMaxWords = pSrcPhrase->m_nSrcWords;
		}
	}
	else // do next block when the map is not empty
	{
    // there might be a pre-existing association between this key and a CTargetUnit,
    // so check it out
		// BEW 29Jul11, the following function had changes made today too, I removed the
		// alternate uppercase lookup which used to be done when auto-caps is ON and the
		// lowercase lookup (tried first) failed
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
			if (gbAutoCaps)
			{
				pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE); // FALSE
								// is value of bIsSrc (auto-caps needs to know whether the
								// string is source text versus adaptation (or gloss) text
			}
			else
			{
				pRefString->m_translation = tgtPhrase;
			}
//*
#if defined(_KBSERVER)
			// BEW added 5Oct12, here is a suitable place for kbserver support of CreateEntry(),
			// since both the key and the translation (both possibly with a case adjustment
			// for the first letter) are defined.
      // Note: we can't reliably assume that the newly typed translation or gloss has
      // not been, independently by some other user, added to the kbserver already,
      // and also subsequently deleted by him before the present; therefore we must
      // test for the absence of this src/tgt pair and only upload if the entry
      // really is going to be a new one.
			if (m_pApp->m_bIsKBServerProject &&
				m_pApp->GetKbServer(m_pApp->GetKBTypeForServer())->IsKBSharingEnabled())
			{
				KbServer* pKbSvr = m_pApp->GetKbServer(m_pApp->GetKBTypeForServer());

				// don't send to kbserver if it's a <Not In KB> entry
				if(!bStoringNotInKB)
				{
					// Here's where I'll test doing this on a thread
          /* no --too restrictive
          // BEW changed 4Feb13 to use KbServer sublassed off of wxThreadHelper (to avoid link errors)
          pKbSvr->m_bUseCache = pKbSvr->IsCachingON(); // currently it's OFF (ie. FALSE)
          pKbSvr->m_source = key;
          pKbSvr->m_translation = pRefString->m_translation;
          // now create the runnable thread with explicit stack size of 10KB; it's a default
          // kind of thread ("detached") so will destroy itself when it's done
          wxThreadError error =  pKbSvr->Create(10240);
          if (error != wxTHREAD_NO_ERROR)
          {
            wxString msg;
            msg = msg.Format(_T("Thread_CreateEntry(): thread creation failed, error number: %d"),
              (int)error);
            wxMessageBox(msg, _T("Thread creation error"), wxICON_EXCLAMATION | wxID_OK);
            m_pApp->LogUserAction(msg);
          }
          else
          {
            // no error, so run it
            error = pKbSvr->GetThread()->Run();
            if (error != wxTHREAD_NO_ERROR)
            {
              wxString msg;
              msg = msg.Format(_T("Thread_Run(): cannot make the thread run, error number: %d"),
                (int)error);
              wxMessageBox(msg, _T("Thread start error"), wxICON_EXCLAMATION | wxID_OK);
              m_pApp->LogUserAction(msg);
            }
          }
          */

					Thread_CreateEntry* pCreateEntryThread = new Thread_CreateEntry;
					// populate it's public members (it only has public ones anyway)
					pCreateEntryThread->m_pKB = this;
					//pCreateEntryThread->m_pKbSvr = pKbSvr;
					pCreateEntryThread->m_kbServerType = m_pApp->GetKBTypeForServer();
					pCreateEntryThread->m_bUseCache = pKbSvr->IsCachingON(); // currently it's OFF (ie. FALSE)
					pCreateEntryThread->m_source = key;
					pCreateEntryThread->m_translation = pRefString->m_translation;
					// now create the runnable thread with explicit stack size of 10KB
					wxThreadError error =  pCreateEntryThread->Create(10240);
					if (error != wxTHREAD_NO_ERROR)
					{
						wxString msg;
						msg = msg.Format(_T("Thread_CreateEntry(): thread creation failed, error number: %d"),
							(int)error);
						wxMessageBox(msg, _T("Thread creation error"), wxICON_EXCLAMATION | wxID_OK);
						m_pApp->LogUserAction(msg);
					}
					else
					{
            // no error, so now run the thread (it will destroy itself when done)
            error = pCreateEntryThread->Run();
            if (error != wxTHREAD_NO_ERROR)
            {
              wxString msg;
              msg = msg.Format(_T("Thread_Run(): cannot make the thread run, error number: %d"),
                (int)error);
              wxMessageBox(msg, _T("Thread start error"), wxICON_EXCLAMATION | wxID_OK);
              m_pApp->LogUserAction(msg);
            }
					}

					/*
					bool bHandledOK = HandleNewPairCreated(m_pApp->GetKBTypeForServer(),
								key, pRefString->m_translation, pKbSvr->IsCachingON());

					// I've not yet decided what to do with the return value, at present we'll
					// just ignore it even if FALSE (an internally generated message would have
					// been seen anyway in that event)
					bHandledOK = bHandledOK; // avoid compiler warning
					*/
				}
			}
#endif
//*/
			// continue with the store to the local KB
			pTU->m_pTranslations->Append(pRefString); // store in the CTargetUnit
			if (m_pApp->m_bForceAsk)
				pTU->m_bAlwaysAsk = TRUE; // turn it on if user wants to be given
						// opportunity to add a new refString next time its matched
			if (m_bGlossingKB)
				pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
			else
			{
				if (bStoringNotInKB)
				{
					pSrcPhrase->m_bNotInKB = TRUE;
					pSrcPhrase->m_bHasKBEntry = FALSE;
				}
				else
				{
					pSrcPhrase->m_bHasKBEntry = TRUE;
				}
			}
			(*m_pMap[nMapIndex])[key] = pTU;// store the CTargetUnit instance in the map
			// update the maxWords limit
			if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
			{
				m_nMaxWords = pSrcPhrase->m_nSrcWords;
			}
			// BEW 17Oct11, added next line (fixes bug in version 6, the checkbox, once set,
			// was staying on at each new phrase box location, and should be off)
			m_pApp->m_bForceAsk = FALSE; // must be turned off before next location arrived at
			return TRUE;
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
			if (gbAutoCaps)
			{
				pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);
			}
			else
			{
				pRefString->m_translation = tgtPhrase; // use unchanged string, could be uc
			}

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
//*
#if defined(_KBSERVER)
            // BEW added 18Oct12, call HandleUndelete() Note: we can't reliably
            // assume that the kbserver entry is also currently stored as a
            // deleted entry, because some other connected user may have
            // already just undeleted it. So we must first determine that an
            // entry with the same src/tgt string is in the remote database,
            // and that it's currently pseudo-deleted. If that's the case, we
            // undelete it. If it's not in the remote database at all yet, then
            // we add it instead as a normal entry. If it's in the remote
            // database already as a normal entry, then we make no change.
						if (m_pApp->m_bIsKBServerProject &&
							m_pApp->GetKbServer(m_pApp->GetKBTypeForServer())->IsKBSharingEnabled())
						{
							if (!pTU->IsItNotInKB() || !bStoringNotInKB)
							{
								bool bHandledOK = HandlePseudoUndelete(m_pApp->GetKBTypeForServer(),
														key, pRefString->m_translation);

								// I've not yet decided what to do with the return value, at
								// present we'll just ignore it even if FALSE (an internally
								// generated message would have been seen anyway in that event)
								bHandledOK = bHandledOK; // avoid compiler warning
							}
						}
#endif
//*/
					}
					else
					{
						pRefStr->m_refCount++;
					}
					if (m_bGlossingKB)
						pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
					else
					{
						if (bStoringNotInKB)
						{
							// storing <Not In KB>
							pSrcPhrase->m_bHasKBEntry = FALSE;
							pSrcPhrase->m_bNotInKB = TRUE;
						}
						else
						{
							pSrcPhrase->m_bHasKBEntry = TRUE;
						}
					}

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
			if (bMatched)
			{
				m_pApp->m_bForceAsk = FALSE; // must be turned off before next location arrived at
				return TRUE;
			}
			else
			{
				// no match made in the above loop
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
					 // and it's a normal adaptation or <Not In KB>
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

					// recalculate the string to be stored
					// BEW changed 29Jul11
					if (gbAutoCaps)
					{
						pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);
					}
					else
					{
						pRefString->m_translation = tgtPhrase;
					}
//*
#if defined(_KBSERVER)
          // BEW added 5Oct12, here is a suitable place for kbserver support of
          // CreateEntry(), since both the key and the translation (both possibly
          // with a case adjustment for the first letter) are defined.
          // Note: we can't reliably assume that the newly typed translation or
          // gloss has not been, independently by some other user, added to the
          // kbserver already, and also subsequently deleted by him before the
          // present; therefore we must test for the absence of this src/tgt pair
          // and only upload if the entry really is going to be a new one.
					if (m_pApp->m_bIsKBServerProject &&
						m_pApp->GetKbServer(m_pApp->GetKBTypeForServer())->IsKBSharingEnabled())
					{
						KbServer* pKbSvr = m_pApp->GetKbServer(m_pApp->GetKBTypeForServer());

						if (!bStoringNotInKB)
						{
							// Here's where I'll test doing this on a thread
              /*
              // BEW changed 4Feb13 to use KbServer sublassed off of wxThreadHelper (to avoid link errors)
              pKbSvr->m_bUseCache = pKbSvr->IsCachingON(); // currently it's OFF (ie. FALSE)
              pKbSvr->m_source = key;
              pKbSvr->m_translation = pRefString->m_translation;
              // now create the runnable thread with explicit stack size of 10KB; it's a default
              // kind of thread ("detached") so will destroy itself when it's done
              wxThreadError error =  pKbSvr->Create(10240);
              if (error != wxTHREAD_NO_ERROR)
              {
                wxString msg;
                msg = msg.Format(_T("Thread_CreateEntry(): thread creation failed, error number: %d"),
                  (int)error);
                wxMessageBox(msg, _T("Thread creation error"), wxICON_EXCLAMATION | wxID_OK);
                m_pApp->LogUserAction(msg);
              }
              else
              {
                // no error, so run it
                error = pKbSvr->GetThread()->Run();
                if (error != wxTHREAD_NO_ERROR)
                {
                  wxString msg;
                  msg = msg.Format(_T("Thread_Run(): cannot make the thread run, error number: %d"),
                    (int)error);
                  wxMessageBox(msg, _T("Thread start error"), wxICON_EXCLAMATION | wxID_OK);
                  m_pApp->LogUserAction(msg);
                }
              }
              */

							Thread_CreateEntry* pCreateEntryThread = new Thread_CreateEntry;
							// populate it's public members (it only has public ones anyway)
							pCreateEntryThread->m_pKB = this;
							//pCreateEntryThread->m_pKbSvr = pKbSvr;
							pCreateEntryThread->m_kbServerType = m_pApp->GetKBTypeForServer();
							pCreateEntryThread->m_bUseCache = pKbSvr->IsCachingON(); // currently it's OFF (ie. FALSE)
							pCreateEntryThread->m_source = key;
							pCreateEntryThread->m_translation = pRefString->m_translation;
							// now create the runnable thread with explicit stack size of 10KB
							wxThreadError error =  pCreateEntryThread->Create(10240);
							if (error != wxTHREAD_NO_ERROR)
							{
								wxString msg;
								msg = msg.Format(_T("Thread_CreateEntry(): thread creation failed, error number: %d"),
									(int)error);
								wxMessageBox(msg, _T("Thread creation error"), wxICON_EXCLAMATION | wxID_OK);
								m_pApp->LogUserAction(msg);
							}
              else
              {
                // no error, so now run the thread (it will destroy itself when done)
                error = pCreateEntryThread->Run();
                if (error != wxTHREAD_NO_ERROR)
                {
                  wxString msg;
                  msg = msg.Format(_T("Thread_Run(): cannot make the thread run, error number: %d"),
                    (int)error);
                  wxMessageBox(msg, _T("Thread start error"), wxICON_EXCLAMATION | wxID_OK);
                  m_pApp->LogUserAction(msg);
                }
              }

							/*
							bool bHandledOK = HandleNewPairCreated(m_pApp->GetKBTypeForServer(),
										key, pRefString->m_translation, pKbSvr->IsCachingON());

							// I've not yet decided what to do with the return value, at present we'll
							// just ignore it even if FALSE (an internally generated message would have
							// been seen anyway in that event)
							bHandledOK = bHandledOK; // avoid compiler warning
							*/
						}
					}
#endif
//*/
					// continue with the store to the local KB
					pTU->m_pTranslations->Append(pRefString);
					if (m_bGlossingKB)
						pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
					else
					{
						if (bStoringNotInKB)
						{
							pSrcPhrase->m_bNotInKB = TRUE;
							pSrcPhrase->m_bHasKBEntry = FALSE;
						}
						else
						{
							pSrcPhrase->m_bHasKBEntry = TRUE;
						}
					}
				}
			}
		}
	}
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
// BEW 29Jul11, removed a cause for duplicate entries to be formed in a CTargetUnit instance
// BEW 14Sep11, fixed up embarrasingly bad slack code and failure to take into account
// that the string passed in may be <Not In KB> (the legacy version got the doc flags
// wrong!)
// BEW 17Oct11, updated to turn of the app flag, m_bForceAsk, if it was TRUE on entry
// before returning, (but always after having used the TRUE value of course, if passed in
// as TRUE)
bool CKB::StoreText(CSourcePhrase *pSrcPhrase, wxString &tgtPhrase, bool bSupportNoAdaptationButton)
{
	// determine the auto caps parameters, if the functionality is turned on
	bool bNoError = TRUE;
	wxString strNot = m_pApp->m_strNotInKB;
	bool bStoringNotInKB = (strNot == tgtPhrase);

    // do not permit storage if the source phrase has an empty key
	if (pSrcPhrase->m_key.IsEmpty())
	{
		gbMatchedKB_UCentry = FALSE;
		m_pApp->m_bForceAsk = FALSE; // must be turned off before next location arrived at
		return TRUE; // this is not an error, just suppression of the store
	}

	if (gbAutoCaps)
	{
		bNoError = m_pApp->GetDocument()->SetCaseParameters(pSrcPhrase->m_key); // for source word or phrase
	}

    // If the source word (or phrase) has not been previously encountered, then
    // m_bHasKBEntry (or the equiv flag if glossing is ON) will be false, which has to be
    // true for the StoreText call not to fail. BEW 05July2006: No! The m_bHasKBEntry (if
    // adapting) or m_bHasGlossingKBEntry (f glossing) flags we are talking about declare,
    // if FALSE, that that PARTICULAR instance in the DOCUMENT does, or does not, yet have
    // a KB entry. When the phrase box lands there, on the other hand, the relevant flag
    // may be TRUE. It then gets its KB entry removed (or ref count decremented) before a
    // store is done, and so the relevant flag is made false when the former happens.
    // However, for safety first, we'll test the flag below and turn it all if it is on, so
    // that the store may go ahead
	// BEW 1Jun10: Okay, despite the above comments, I still want some safety first
	// protection here. Why? Because the code which calls RemoveRefString() and/or
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
		// rest of this block's code is a simplification of code from later in StoreText()
		int nMapIndex;
		// BEW 13Nov10 support Bob Eaton's request that glossing KB uses all maps
		if (IsFixedSpaceSymbolWithin(pSrcPhrase))
		{
			nMapIndex = 0;
		}
		else
		{
			nMapIndex = pSrcPhrase->m_nSrcWords - 1; // index to the appropriate map
		}

        // if we have too many source words, then we cannot save to the KB, so beep
		//if (!m_bGlossingKB && pSrcPhrase->m_nSrcWords > MAX_WORDS)
		if (pSrcPhrase->m_nSrcWords > MAX_WORDS)
		{
			::wxBell();
			m_pApp->m_bForceAsk = FALSE; // must be turned off before next location arrived at
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
                if (!bMatched)
				{
					pRefString->DeleteRefString(); // don't leak memory
				}
				m_pApp->m_bForceAsk = FALSE; // must be turned off before next location arrived at
				return TRUE;
			}
		}
		m_pApp->m_bForceAsk = FALSE; // must be turned off before next location arrived at
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
		if (tgtPhrase != strNot)
		{
            // BEW 29Jul11 added auto-caps code here so that m_adaption gets set in the
            // same way that MakeTargetStringIncludingPunctuation() will do the
            // capitalization for m_targetStr, formerly m_adaption was just set to
            // tgtPhrase no matter whether auto-caps was on or off (and we didn't notice
            // because the view only shows m_targetStr)
			wxString s = tgtPhrase;
			if (gbAutoCaps)
			{
				bool bNoError = TRUE;
				if (gbSourceIsUpperCase && !gbMatchedKB_UCentry)
				{
					bNoError = m_pApp->GetDocument()->SetCaseParameters(s,FALSE); // FALSE is bIsSrcText
					if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
					{
						// change it to upper case
						s.SetChar(0,gcharNonSrcUC);
					}
				}
				pSrcPhrase->m_adaption = s;
			}
			else
			{
				pSrcPhrase->m_adaption = tgtPhrase;
			}
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

    // if there is a CTargetUnit associated with the current key, then get it; if not,
    // create one and add it to the appropriate map; we start by computing which map we
    // need to store to
    // BEW 13Nov10, changed to support Bob Eaton's request for a ten map glossing KB
	int nMapIndex;
	if (IsFixedSpaceSymbolWithin(pSrcPhrase))
	{
		nMapIndex = 0;
	}
	else
	{
		nMapIndex = pSrcPhrase->m_nSrcWords - 1; // index to the appropriate map
	}

    // if we have too many source words, then we cannot save to the KB, so detect this and
    // warn the user that it will not be put in the KB, then return TRUE since all is
    // otherwise okay (this will be handled as a retranslation, by default) The following
    // comment is for when glossing... Note: if the source phrase is part of a
    // retranslation, we allow updating of the m_gloss attribute, and we won't change any
    // of the retranslation supporting flags; so it is therefore possible for
    // m_bRetranslation to be TRUE, and also for m_bHasGlossingKBEntry to be TRUE.

    // BEW 13Nov10, changed to support Bob Eaton's request for a ten map glossing KB
	if (pSrcPhrase->m_nSrcWords > MAX_WORDS)
	{
		pSrcPhrase->m_bNotInKB = FALSE;
		if (m_bGlossingKB)
			pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
		else
			pSrcPhrase->m_bHasKBEntry = FALSE;
		wxMessageBox(_(
"Warning: there are too many source language words in this phrase for this adaptation to be stored in the knowledge base."),
		_T(""),wxICON_INFORMATION | wxOK);
		gbMatchedKB_UCentry = FALSE;
		m_pApp->m_bForceAsk = FALSE; // must be turned off before next location arrived at
		return TRUE;
	}

	// continue the storage operation
	wxString unchangedkey = pSrcPhrase->m_key; // this never has case change done
											   // to it (need this for lookups)
	wxString key = AutoCapsMakeStorageString(pSrcPhrase->m_key); // key might be
															// made lower case
	CTargetUnit* pTU = NULL;
	CRefString* pRefString =  NULL;
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
				m_pApp->m_bForceAsk = FALSE; // must be turned off before next location arrived at
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
			// FALSE in next call is  bool bIsSrc
			pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);
		}
		else
		{
			// if something went wrong, just save as if gbAutoCaps was FALSE
			pRefString->m_translation = tgtPhrase;
		}

#if defined(_KBSERVER)
    // BEW added 5Oct12, here is a suitable place for kbserver support of
    // CreateEntry(), since both the key and the translation (both possibly with a case
    // adjustment for the first letter) are defined.
    // Note: we can't reliably assume that the newly typed translation or gloss has not
    // been, independently by some other user, added to the kbserver already, and also
    // subsequently deleted by him before the present; therefore we must test for the
    // absence of this src/tgt pair and only upload if the entry really is going to be
    // a new one.
		if (m_pApp->m_bIsKBServerProject &&
			m_pApp->GetKbServer(m_pApp->GetKBTypeForServer())->IsKBSharingEnabled())
		{
			KbServer* pKbSvr = m_pApp->GetKbServer(m_pApp->GetKBTypeForServer());

			// don't send to kbserver if it's a <Not In KB> entry
			if (!bStoringNotInKB)
			{
				// Here's where I'll test doing this on a thread
        /* no, too restrictive - I need Entry() to have different contents from time to time
				// BEW changed 4Feb13 to use KbServer sublassed off of wxThreadHelper (to avoid link errors)
        pKbSvr->m_bUseCache = pKbSvr->IsCachingON(); // currently it's OFF (ie. FALSE)
				pKbSvr->m_source = key;
				pKbSvr->m_translation = pRefString->m_translation;
				// now create the runnable thread with explicit stack size of 10KB; it's a default
				// kind of thread ("detached") so will destroy itself when it's done
				wxThreadError error =  pKbSvr->Create(10240);
				if (error != wxTHREAD_NO_ERROR)
				{
					wxString msg;
					msg = msg.Format(_T("Thread_CreateEntry(): thread creation failed, error number: %d"),
						(int)error);
					wxMessageBox(msg, _T("Thread creation error"), wxICON_EXCLAMATION | wxID_OK);
					m_pApp->LogUserAction(msg);
				}
				else
				{
          // no error, so run it
          error = pKbSvr->GetThread()->Run();
          if (error != wxTHREAD_NO_ERROR)
          {
            wxString msg;
            msg = msg.Format(_T("Thread_Run(): cannot make the thread run, error number: %d"),
              (int)error);
            wxMessageBox(msg, _T("Thread start error"), wxICON_EXCLAMATION | wxID_OK);
            m_pApp->LogUserAction(msg);
          }
				}
        */

				Thread_CreateEntry* pCreateEntryThread = new Thread_CreateEntry;
				// populate it's public members (it only has public ones anyway)
				pCreateEntryThread->m_pKB = this;
				//pCreateEntryThread->m_pKbSvr = pKbSvr;
				pCreateEntryThread->m_kbServerType = m_pApp->GetKBTypeForServer();
				pCreateEntryThread->m_bUseCache = pKbSvr->IsCachingON(); // currently it's OFF (ie. FALSE)
				pCreateEntryThread->m_source = key;
				pCreateEntryThread->m_translation = pRefString->m_translation;
				// now create the runnable thread with explicit stack size of 10KB
				wxThreadError error =  pCreateEntryThread->Create(10240);
				if (error != wxTHREAD_NO_ERROR)
				{
					wxString msg;
					msg = msg.Format(_T("Thread_CreateEntry(): thread creation failed, error number: %d"),
						(int)error);
					wxMessageBox(msg, _T("Thread creation error"), wxICON_EXCLAMATION | wxID_OK);
					m_pApp->LogUserAction(msg);
				}
        else
        {
          // no error, so now run the thread (it will destroy itself when done)
          error = pCreateEntryThread->Run();
          if (error != wxTHREAD_NO_ERROR)
          {
            wxString msg;
            msg = msg.Format(_T("Thread_Run(): cannot make the thread run, error number: %d"),
              (int)error);
            wxMessageBox(msg, _T("Thread start error"), wxICON_EXCLAMATION | wxID_OK);
            m_pApp->LogUserAction(msg);
          }
        }

				/*
				bool bHandledOK = HandleNewPairCreated(m_pApp->GetKBTypeForServer(),
							key, pRefString->m_translation, pKbSvr->IsCachingON());

				// I've not yet decided what to do with the return value, at present we'll
				// just ignore it even if FALSE (an internally generated message would have
				// been seen anyway in that event)
				bHandledOK = bHandledOK; // avoid compiler warning
				*/
			}
		}
#endif

		// continue with the store to the local KB
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
			if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
			{
				m_nMaxWords = pSrcPhrase->m_nSrcWords;
			}
		}
		else
		{
			if (bStoringNotInKB)
			{
				// we are storing <Not In KB>; tell the src phrase it has this
				// entry in the KB
				pSrcPhrase->m_bHasKBEntry = FALSE;
				pSrcPhrase->m_bNotInKB = TRUE;
			}
			else
			{
				// we are storing an adaptation; tell the src phrase it has an adaptation
				// entry in the KB
				pSrcPhrase->m_bHasKBEntry = TRUE;
			}

			// it can't be a retranslation, so ensure the next two flags are cleared
			pSrcPhrase->m_bBeginRetranslation = FALSE;
			pSrcPhrase->m_bEndRetranslation = FALSE;

			(*m_pMap[nMapIndex])[key] = pTU; // store the CTargetUnit in the
											 // map with appropriate index
			// update the maxWords limit
			if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
			{
				m_nMaxWords = pSrcPhrase->m_nSrcWords;
			}
		}
		m_pApp->m_bForceAsk = FALSE; // must be turned off before next location arrived at
		return TRUE;
	}
	else // the block below is for when the map is not empty
	{
		// there might be a pre-existing association between this key and a CTargetUnit,
		// so check it out
        // BEW 29Jul11, AutoCapsLookup() has been changed on this date too, it now doesn't
        // (when auto-caps is ON) do an uppercase lookup if the lowercase lookup fails.
        // This gives better behaviour - the reason is explained in the heading comments for
        // that function.
		bool bFound = AutoCapsLookup(m_pMap[nMapIndex], pTU, unchangedkey);

		// check we have a valid pTU
		if (bFound && pTU->m_pTranslations->IsEmpty())
		{
			// this is an error condition, CTargetUnits must NEVER have an
			// empty m_translations list
			wxMessageBox(_T(
"Warning: the current storage operation has been skipped, and a bad storage element has been deleted."),
			_T(""), wxICON_EXCLAMATION | wxOK);

			if (pTU != NULL)
			{
				delete pTU; // delete it from the heap, if set
			}
			pTU = (CTargetUnit*)NULL;

			MapKeyStringToTgtUnit* pMap = m_pMap[nMapIndex];
			int bRemoved;
			if (gbAutoCaps && gbSourceIsUpperCase && !gbMatchedKB_UCentry)
				bRemoved = pMap->erase(key); // remove the changed to lc entry from the map
			else
				bRemoved = pMap->erase(unchangedkey); // remove the unchanged one from the map
			wxASSERT(bRemoved == 1);
			bRemoved = bRemoved; // avoid warning (BEW 3Jan12, retain unchanged)
			m_pApp->m_bSaveToKB = TRUE; // ensure its back on (if here from a choice not
				// save to KB, this will be cleared by OnCheckKBSave, preserving user choice)
			gbMatchedKB_UCentry = FALSE;
			m_pApp->m_bForceAsk = FALSE; // must be turned off before next location arrived at
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
					// don't make any change to pSrcPhrase's flag values
					return TRUE; // make caller think all is well
				}
			}

			// continue with store: we can be storing gloss (empty or not), or an
			// adaptation (empty or not), or (in adapting mode only) a <Not In KB> string
			pTU = new CTargetUnit;
			wxASSERT(pTU != NULL);
			pRefString = new CRefString(pTU); // creates its CRefStringMetadata instance too
			wxASSERT(pRefString != NULL);

			pRefString->m_refCount = 1; // set the count
			// add the translation or gloss string
			// BEW 29Jul11, what we add should depend on auto-caps setting, if ON, then
			// force first character to lowercase if it is upper case; if OFF, then store
			// whatever string tgtPhrase has - whether starting with ucase or lcase
			if (gbAutoCaps)
			{
				pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);
			}
			else
			{
				pRefString->m_translation = tgtPhrase;
			}
//*
#if defined(_KBSERVER)
			// BEW added 5Oct12, here is a suitable place for kbserver support of CreateEntry(),
			// since both the key and the translation (both possibly with a case adjustment
			// for the first letter) are defined.
      // Note: we can't reliably assume that the newly typed translation or gloss has
      // not been, independently by some other user, added to the kbserver already,
      // and also subsequently deleted by him before the present; therefore we must
      // test for the absence of this src/tgt pair and only upload if the entry
      // really is going to be a new one.
			if (m_pApp->m_bIsKBServerProject &&
				m_pApp->GetKbServer(m_pApp->GetKBTypeForServer())->IsKBSharingEnabled())
			{
				KbServer* pKbSvr = m_pApp->GetKbServer(m_pApp->GetKBTypeForServer());

				// don't send to kbserver if it's a <Not In KB> entry
				if(!bStoringNotInKB)
				{
				// Here's where I'll test doing this on a thread
        /* no -- too restrictive
				// BEW changed 4Feb13 to use KbServer sublassed off of wxThreadHelper (to avoid link errors)
        pKbSvr->m_bUseCache = pKbSvr->IsCachingON(); // currently it's OFF (ie. FALSE)
				pKbSvr->m_source = key;
				pKbSvr->m_translation = pRefString->m_translation;
				// now create the runnable thread with explicit stack size of 10KB; it's a default
				// kind of thread ("detached") so will destroy itself when it's done
				wxThreadError error =  pKbSvr->Create(10240);
				if (error != wxTHREAD_NO_ERROR)
				{
					wxString msg;
					msg = msg.Format(_T("Thread_CreateEntry(): thread creation failed, error number: %d"),
						(int)error);
					wxMessageBox(msg, _T("Thread creation error"), wxICON_EXCLAMATION | wxID_OK);
					m_pApp->LogUserAction(msg);
				}
				else
				{
          // no error, so run it
          error = pKbSvr->GetThread()->Run();
          if (error != wxTHREAD_NO_ERROR)
          {
            wxString msg;
            msg = msg.Format(_T("Thread_Run(): cannot make the thread run, error number: %d"),
              (int)error);
            wxMessageBox(msg, _T("Thread start error"), wxICON_EXCLAMATION | wxID_OK);
            m_pApp->LogUserAction(msg);
          }
				}
        */

				Thread_CreateEntry* pCreateEntryThread = new Thread_CreateEntry;
				// populate it's public members (it only has public ones anyway)
				pCreateEntryThread->m_pKB = this;
				//pCreateEntryThread->m_pKbSvr = pKbSvr;
				pCreateEntryThread->m_kbServerType = m_pApp->GetKBTypeForServer();
				pCreateEntryThread->m_bUseCache = pKbSvr->IsCachingON(); // currently it's OFF (ie. FALSE)
				pCreateEntryThread->m_source = key;
				pCreateEntryThread->m_translation = pRefString->m_translation;
				// now create the runnable thread with explicit stack size of 10KB
				wxThreadError error =  pCreateEntryThread->Create(10240);
				if (error != wxTHREAD_NO_ERROR)
				{
					wxString msg;
					msg = msg.Format(_T("Thread_CreateEntry(): thread creation failed, error number: %d"),
						(int)error);
					wxMessageBox(msg, _T("Thread creation error"), wxICON_EXCLAMATION | wxID_OK);
					m_pApp->LogUserAction(msg);
				}
        else
        {
          // no error, so now run the thread (it will destroy itself when done)
          error = pCreateEntryThread->Run();
          if (error != wxTHREAD_NO_ERROR)
          {
            wxString msg;
            msg = msg.Format(_T("Thread_Run(): cannot make the thread run, error number: %d"),
              (int)error);
            wxMessageBox(msg, _T("Thread start error"), wxICON_EXCLAMATION | wxID_OK);
            m_pApp->LogUserAction(msg);
          }
        }

				/*
				bool bHandledOK = HandleNewPairCreated(m_pApp->GetKBTypeForServer(),
							key, pRefString->m_translation, pKbSvr->IsCachingON());

				// I've not yet decided what to do with the return value, at present we'll
				// just ignore it even if FALSE (an internally generated message would have
				// been seen anyway in that event)
				bHandledOK = bHandledOK; // avoid compiler warning
				*/
				}
			}
#endif
//*/
			// continue with the store to the local KB
			pTU->m_pTranslations->Append(pRefString); // store in the CTargetUnit
			if (m_pApp->m_bForceAsk)
			{
				pTU->m_bAlwaysAsk = TRUE; // turn it on if user wants to be given
					// opportunity to add a new refString next time it's matched
			}
			if (m_bGlossingKB)
			{
				pSrcPhrase->m_bHasGlossingKBEntry = TRUE;

				(*m_pMap[nMapIndex])[key] = pTU; // store the CTargetUnit in
						// the map with appropr. index (key may have been made lc)
				// update the maxWords limit, BEW changed 13Nov10
				//m_nMaxWords = 1;
				if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
				{
					m_nMaxWords = pSrcPhrase->m_nSrcWords;
				}
			}
			else
			{
				// distinguish between normal adaptation versus <Not In KB>
				if (bStoringNotInKB)
				{
					pSrcPhrase->m_bNotInKB = TRUE;
					pSrcPhrase->m_bHasKBEntry = FALSE;
				}
				else
				{
					pSrcPhrase->m_bHasKBEntry = TRUE;
				}
				(*m_pMap[nMapIndex])[key] = pTU; // store the CTargetUnit in
						// the map with appropr. index (key may have been made lc)
				// update the maxWords limit
				if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
				{
					m_nMaxWords = pSrcPhrase->m_nSrcWords;
				}
			}
			m_pApp->m_bForceAsk = FALSE; // must be turned off before next location arrived at
			return TRUE;
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
      //
			// BEW 29Jul11, removed 3 lines here, so that we now always use lower case
			// for what is put into m_translation when auto-caps is turned on. The legacy
			// code would check auto-caps on, and for an upper case src like "Kristus" it
			// would use the unchanged phrase box value -- which, even if typed as
			// "christ" it will have been auto-caps changed to "Christ" before getting to
			// here, and so m_translation is given "Christ" wrongly as the adaptation.
      // Then, when the loop which follows searches for a match, if there is a
      // deleted or non-deleted entry already present and made with m_translation =
      // "christ", no match gets made with "Christ", and so the if (!bMatched) block
      // is then entered, and a new entry created and added to pTU, and the new entry
      // would be a second value "christ". In this way, the KB has for some years,
			// unfortunately, been wrongly adding a second duplicate entry in this
			// particular circumstance. The fix is to unilaterally make the entry with
			// AutoCapsMakeStorageString() when auto-caps is on, and unilaterally make it
			// with the unchanged value of tgtPhrase when it is not on. Trying to be
			// smarter than that because the user may have only recently turned auto-caps
			// on leads to these unnecessary errors.
			if (gbAutoCaps)
				pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);
			else
				pRefString->m_translation = tgtPhrase;

			// we are storing either a gloss, or adaptation, or pseudo-adaption "<Not In KB>"
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

#if defined(_KBSERVER)
            // BEW added 18Oct12, call HandleUndelete() Note: we can't reliably
            // assume that the kbserver entry is also currently stored as a
            // deleted entry, because some other connected user may have
            // already just undeleted it. So we must first determine that an
            // entry with the same src/tgt string is in the remote database,
            // and that it's currently pseudo-deleted. If that's the case, we
            // undelete it. If it's not in the remote database at all yet, then
            // we add it instead as a normal entry. If it's in the remote
            // database already as a normal entry, then we make no change.
						// BEW 15Nov12, we don't store <Not In KB> as kbserver entries,
						// and when user locally unticks the Save in KB checkbox to make
						// that key have only <Not In KB> as the pseudo-adaptation, it
						// makes any normal adaptations for that key become pseudo-deleted
						// but we don't inform kbserver of that fact. Therefore, an
						// attempt to undelete any of those pseudo-deleted entries needs
            // to be stopped from sending anything to kbserver also. We want to
            // keep use of <Not In KB> restricted to the particular user who
            // wants to do that, and not propagate it and deletions /
            // undeletions that may happen as part of it, to the kbserver.
						if (m_pApp->m_bIsKBServerProject &&
							m_pApp->GetKbServer(m_pApp->GetKBTypeForServer())->IsKBSharingEnabled())
						{
							if (!pTU->IsItNotInKB() || !bStoringNotInKB)
							{
								bool bHandledOK = HandlePseudoUndelete(m_pApp->GetKBTypeForServer(),
														key, pRefString->m_translation);

								// I've not yet decided what to do with the return value, at
								// present we'll just ignore it even if FALSE (an internally
								// generated message would have been seen anyway in that event)
								bHandledOK = bHandledOK; // avoid compiler warning
							}
						}
#endif
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
						// distinguish normal adaptation from <Not In KB> passed in
						if (bStoringNotInKB)
						{
							// storing <Not In KB>
							pSrcPhrase->m_bHasKBEntry = FALSE;
							pSrcPhrase->m_bNotInKB = TRUE;
						}
						else
						{
							// normal adaptation
							pSrcPhrase->m_bHasKBEntry = TRUE;
						}
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
      // adaptation> or a nonempty adaptation being two ref strings for the one key
      // -- that's only when adapting; for glossing this cannot happen because
      // glossing mode does not support <Not In KB> entries for the glossing KB)
			if (bMatched)
			{
				m_pApp->m_bForceAsk = FALSE; // must be turned off before next location arrived at
				return TRUE;
			}
			else
			{
				// no match was made in the above loop
				TranslationsList::Node* tpos = pTU->m_pTranslations->GetFirst();
				CRefString* pRefStr = (CRefString*)tpos->GetData();
				// check that there isn't somehow an undeleted <Not In KB> CRefString as
				// the only one - if there is, and <Not In KB> was passed in, then we've
				// no store to make and can just set the flags and return TRUE
				if (!m_bGlossingKB && pRefStr->m_translation == strNot)
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
					if (m_pApp->m_bForceAsk)
					{
						pTU->m_bAlwaysAsk = TRUE; // nTrCount might be 1, so we must
								// ensure it gets set if that is what the user wants
					}
					m_pApp->m_bForceAsk = FALSE;
					gbMatchedKB_UCentry = FALSE;
					return TRUE; // all is well
				}
				else // either we are glossing; or we are adapting
					 // and it's a normal adaptation or <Not In KB> and
					 // there is no <Not In KB> CRefString in pTU already
				{
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
							if (m_pApp->m_bForceAsk)
							{
								pTU->m_bAlwaysAsk = TRUE; // nTrCount might be 1, so we must
										// ensure it gets set if that is what the user wants
							}
							m_pApp->m_bForceAsk = FALSE; // make sure it's turned off
							pRefString->DeleteRefString(); // don't leak the memory
							pRefString = (CRefString*)NULL;
							gbMatchedKB_UCentry = FALSE;
							// don't change any of the flags on pSrcPhrase
							return TRUE; // make caller think all is well
						}
					}

					// calculate the string to be stored...
					// BEW 29Jul11, force lowercase if gbAutoCaps is ON, if OFF, use
					// whatever is in the phrase box (unchanged)
					if (gbAutoCaps)
					{
						pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);
					}
					else
					{
						pRefString->m_translation = tgtPhrase;
					}

#if defined(_KBSERVER)
          // BEW added 5Oct12, here is a suitable place for kbserver support of
          // CreateEntry(), since both the key and the translation (both possibly
          // with a case adjustment for the first letter) are defined.
          // Note: we can't reliably assume that the newly typed translation or
          // gloss has not been, independently by some other user, added to the
          // kbserver already, and also subsequently deleted by him before the
          // present; therefore we must test for the absence of this src/tgt pair
          // and only upload if the entry really is going to be a new one.
					if (m_pApp->m_bIsKBServerProject &&
						m_pApp->GetKbServer(m_pApp->GetKBTypeForServer())->IsKBSharingEnabled())
					{
						KbServer* pKbSvr = m_pApp->GetKbServer(m_pApp->GetKBTypeForServer());

						// don't send a <Not In KB> entry to kbserver
						if (!bStoringNotInKB)
						{
							// Here's where I'll test doing this on a thread
              /* no -- too restrictive
              // BEW changed 4Feb13 to use KbServer sublassed off of wxThreadHelper (to avoid link errors)
              pKbSvr->m_bUseCache = pKbSvr->IsCachingON(); // currently it's OFF (ie. FALSE)
              pKbSvr->m_source = key;
              pKbSvr->m_translation = pRefString->m_translation;
              // now create the runnable thread with explicit stack size of 10KB; it's a default
              // kind of thread ("detached") so will destroy itself when it's done
              wxThreadError error =  pKbSvr->Create(10240);
              if (error != wxTHREAD_NO_ERROR)
              {
                wxString msg;
                msg = msg.Format(_T("Thread_CreateEntry(): thread creation failed, error number: %d"),
                  (int)error);
                wxMessageBox(msg, _T("Thread creation error"), wxICON_EXCLAMATION | wxID_OK);
                m_pApp->LogUserAction(msg);
              }
              else
              {
                // no error, so run it
                error = pKbSvr->GetThread()->Run();
                if (error != wxTHREAD_NO_ERROR)
                {
                  wxString msg;
                  msg = msg.Format(_T("Thread_Run(): cannot make the thread run, error number: %d"),
                    (int)error);
                  wxMessageBox(msg, _T("Thread start error"), wxICON_EXCLAMATION | wxID_OK);
                  m_pApp->LogUserAction(msg);
                }
              }
              */

							Thread_CreateEntry* pCreateEntryThread = new Thread_CreateEntry;
							// populate it's public members (it only has public ones anyway)
							pCreateEntryThread->m_pKB = this;
							//pCreateEntryThread->m_pKbSvr = pKbSvr;
							pCreateEntryThread->m_kbServerType = m_pApp->GetKBTypeForServer();
							pCreateEntryThread->m_bUseCache = pKbSvr->IsCachingON(); // currently it's OFF (ie. FALSE)
							pCreateEntryThread->m_source = key;
							pCreateEntryThread->m_translation = pRefString->m_translation;
							// now create the runnable thread with explicit stack size of 10KB
							wxThreadError error =  pCreateEntryThread->Create(10240);
							if (error != wxTHREAD_NO_ERROR)
							{
								wxString msg;
								msg = msg.Format(_T("Thread_CreateEntry(): thread creation failed, error number: %d"),
									(int)error);
								wxMessageBox(msg, _T("Thread creation error"), wxICON_EXCLAMATION | wxID_OK);
								m_pApp->LogUserAction(msg);
							}
              else
              {
                // no error, so now run the thread (it will destroy itself when done)
                error = pCreateEntryThread->Run();
                if (error != wxTHREAD_NO_ERROR)
                {
                  wxString msg;
                  msg = msg.Format(_T("Thread_Run(): cannot make the thread run, error number: %d"),
                    (int)error);
                  wxMessageBox(msg, _T("Thread start error"), wxICON_EXCLAMATION | wxID_OK);
                  m_pApp->LogUserAction(msg);
                }
              }

							/*
							bool bHandledOK = HandleNewPairCreated(m_pApp->GetKBTypeForServer(),
										key, pRefString->m_translation, pKbSvr->IsCachingON());

							// I've not yet decided what to do with the return value, at present we'll
							// just ignore it even if FALSE (an internally generated message would have
							// been seen anyway in that event)
							bHandledOK = bHandledOK; // avoid compiler warning
							*/
						}
					}
#endif
					// continue with the store to the local KB
					pTU->m_pTranslations->Append(pRefString);
					if (m_bGlossingKB)
					{
						pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
					}
					else
					{
						// distinguish a normal adaptation store from a <Not In KB> one
						if (bStoringNotInKB)
						{
							pSrcPhrase->m_bNotInKB = TRUE;
							pSrcPhrase->m_bHasKBEntry = FALSE;
						}
						else
						{
							pSrcPhrase->m_bHasKBEntry = TRUE;
						}
						pSrcPhrase->m_bBeginRetranslation = FALSE;
						pSrcPhrase->m_bEndRetranslation = FALSE;
					}
					if (m_pApp->m_bForceAsk)
					{
						pTU->m_bAlwaysAsk = TRUE; // nTrCount might be 1, so we must
								// ensure it gets set if that is what the user wants
					}
				} // end of else block for test: if (!m_bGlossingKB && pRefStr->m_translation == strNot)
			} // end of else block for test: if (bMatched)
		} // end of else block for test: if(!bFound) i.e. we actually matched a stored pTU
	} // end of else block for test: if (m_pMap[nMapIndex]->empty()) i.e. th tested map wasn't empty

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
// BEW 3Mar11, at Bob Eaton's request, changed from indent using one or more \t (tab char)
// to "  " (two spaces) per nTabLevel unit instead (to comply with 3-way merge in Chorus)
////////////////////////////////////////////////////////////////////////////////////////
CBString CKB::MakeKBElementXML(wxString& src,CTargetUnit* pTU,int nTabLevel)
{
	CBString tabUnit = "  "; // BEW 3Mar11, the two spaces to be used instead of "\t" herein
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
		aStr += tabUnit; // tab the start of the line
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
			aStr += tabUnit; // tab the start of the line
			rsTabs += tabUnit; // use rsTabs below to avoid a loop each time
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
		aStr += tabUnit; // tab the start of the line
	}
	aStr += "</TU>\r\n";

#else  // Unicode version

	// start the targetUnit element
	aStr.Empty();
	for (i = 0; i < nTabLevel; i++)
	{
		aStr += tabUnit; // tab the start of the line
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
			aStr += tabUnit; // tab the start of the line
			rsTabs += tabUnit; // use rsTabs below to avoid a loop each time
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
		aStr += tabUnit; // tab the start of the line
	}
	aStr += "</TU>\r\n";

#endif // end of Unicode version
	return aStr;
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      f               -> the wxFile instance used to save the KB file
/// \param      pProgDlg        <-> pointer to the caller's wxProgressDialog
/// \param      nTotal          -> the max range value for the progress dialog
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
void CKB::DoKBSaveAsXML(wxFile& f, const wxString& progressItem, int nTotal)
{
 	// Setup some local pointers
	CBString aStr;
	CTargetUnit* pTU;
	int mapIndex;
	int counter = 0;

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
	maxWords = (int)MAX_WORDS;

	// Now, start building the fixed strings.
    // construct the opening tag and add the list of targetUnits with their associated key
    // strings (source text)
    // prologue (Changed by BEW, 18june07, at request of Bob Eaton so as to support legacy
    // KBs using his SILConverters software, UTF-8 becomes Windows-1252 for the Regular
    // app)

	m_pApp->GetEncodingStringForXmlFiles(encodingStr);
	buff.AppendData(encodingStr,encodingStr.GetLength()); // AppendData internally uses
											// memcpy and GetAppendBuf and UngetAppendBuf

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

		// mrh June 2012 - new fields for kbVersion 3 - not active yet
		if (m_pApp->m_sourceLanguageCode.IsEmpty())
			m_pApp->m_sourceLanguageCode = NOCODE;
		if (m_pApp->m_targetLanguageCode.IsEmpty())
			m_pApp->m_targetLanguageCode = NOCODE;						// ensure we output something
		aStr += "\" ";
		aStr += xml_srccod;
	#ifdef _UNICODE
		tempStr = m_pApp->Convert16to8 (m_pApp->m_sourceLanguageCode);		// source language code
	#else
		tempStr = m_pApp->m_sourceLanguageCode;
	#endif
		aStr += "=\"";
		aStr += tempStr;

		aStr += "\" ";
		aStr += xml_tgtcod;
	#ifdef _UNICODE
		tempStr = m_pApp->Convert16to8 (m_pApp->m_targetLanguageCode);		// target language code
	#else
		tempStr = m_pApp->m_targetLanguageCode;
	#endif
		aStr += "=\"";
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

	wxString msgDisplayed;
	wxString progMsg = _("Saving KB %s - %d of %d Total entries and senses");
	wxString kbPath;
	if (this->IsThisAGlossingKB())
		kbPath = m_pApp->m_curGlossingKBPath;
	else
		kbPath = m_pApp->m_curKBPath;
	wxFileName fn(kbPath);
	msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str(),1,nTotal);

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
					counter++;
					pTU = iter->second;
					wxASSERT(pTU != NULL);
					int i;
					for (i = 0; i < (int)pTU->m_pTranslations->GetCount(); i++)
					{
						if (counter % 200 == 0)
						{
							if (!progressItem.IsEmpty())
							{
								msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str(),counter,nTotal);
								CStatusBar *pStatusBar = (CStatusBar*)m_pApp->GetMainFrame()->m_pStatusBar;
								pStatusBar->UpdateProgress(progressItem, counter, msgDisplayed);
							}
						}
						counter++;
					}
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
// BEW 17Jul11, changed to support the return of enum KB_Entry, with values absent,
// present_but_deleted, or really_present, from GetRefString()
void CKB::Fix_NotInKB_WronglyEditedOut(CPile* pCurPile)
{
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	CAdapt_ItView* pView = m_pApp->GetView(); // <<-- BEWARE if we later support multiple views/panes
											  // as this call gets the interlinear strips view/pane
	wxString str = _T("<Not In KB>");
	CSourcePhrase* pSP = pCurPile->GetSrcPhrase();
	CRefString* pRefStr = NULL;
	KB_Entry rsEntry = GetRefString(pSP->m_nSrcWords, pSP->m_key, str, pRefStr);
	// Note: pRefStr returned NULL only happens when the value absent is returned in rsEntry
	if (pRefStr == NULL || rsEntry == present_but_deleted)
	{
		m_pApp->m_bSaveToKB = TRUE; // it will be off, so we must turn it
								    // back on to get the string restored
        // don't inhibit the call to MakeTargetStringIncludingPunctuation( ) here,
        // since the phrase passed in is the non-punctuated one
		bool bOK;
		bOK = StoreText(pSP, str);
		bOK = bOK; // avoid warning
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
void CKB::DoKBRestore(int& nCount, int& nCumulativeTotal)
{
	wxArrayString* pList = &m_pApp->m_acceptedFilesList;
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	CAdapt_ItView* pView = m_pApp->GetView();
	wxASSERT(pDoc);
	wxASSERT(pView);

	// whm 25Aug11 Note: The caller of DoKBRestore() [OnFileRestoreKb()] had its own
	// wxProgressDialog based on the range of source phrases for saving the current
	// document in preparation for the kb restore operation. It also continues to
	// show during most of the remainder of the OnFileRestoreKb() operation. While
	// it is showing, it also calls this DoKBRestore() function. This function, in
	// turn calls other functions that thselves put up progress dialogs that overlay
	// the original one from OnFileRestoreKb(). These include the OnOpenDocument()
	// function, and the KB saving routines.

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
	bool bDontDoIt; // BEW added 20Apr12, see further below for comments
	bDontDoIt = m_pApp->m_bCollaboratingWithParatext || m_pApp->m_bCollaboratingWithBibledit;
	// add some progress to the KB restore
	CStatusBar *pStatusBar = NULL;
	pStatusBar = (CStatusBar*)m_pApp->GetMainFrame()->m_pStatusBar;
	pStatusBar->StartProgress(_("DoKBRestore"), _("Restoring KB"), nCount);
	for (i=0; i < nCount; i++)
	{
		wxString newName = pList->Item(i);
		wxASSERT(!newName.IsEmpty());

		bool bOK;
		bOK = pDoc->OnOpenDocument(newName, false);
		wxCHECK_RET(bOK, _T("DoKBRestore(): OnOpenDocument() failed, line 4760 in KB.cpp, the KB restoration was abandoned"));

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

		wxString msgDisplayed;
		msgDisplayed = msgDisplayed.Format(_("Restoring KB for File: %s (%d of %d)"), newName.c_str(), (i + 1), nCount);
		pStatusBar->UpdateProgress(_("DoKBRestore"), i, msgDisplayed);

		// whm 26Aug11 modified. If the just-opened doc has no source phrases, there
		// is not point in processing the m_pSourcePhrases, so just continue to the
		// next document.
		//wxASSERT(nTotal > 0);
		if (m_pApp->m_pSourcePhrases->GetCount() == 0)
			continue;

		// whm 25Aug11 Note: I removed the progress dialog Update call from this function
		// because each document has a different number of m_pSourcePhrases, and a
		// single instance of a wxProgressDialog cannot handle more than one range of
		// values, hence we cannot use the pointer of a single instance of wxProgressDialog
		// to track the progress of documents of varied numbers of m_pSourcePhrases.
		nCumulativeTotal += m_pApp->m_pSourcePhrases->GetCount();

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
		}
		// whm added 27Apr09 to save any changes made by RedoStorage above
		// BEW 20Apr12, the OnFileSave() call, if collaboration mode is currently on, will
		// try to transfer data to PT or BE, but the document is not currently properly
		// set up for this, and so errors in indices result in the attempt failing and the
		// app crashing. The possibility of the RedoStorage() call actually modifying the
		// document is quite small, and the easiest thing to do is to here refrain from
		// saving the doc if collaboration is in effect; and further below, to supress the
		// logging block just before the function ends; we'll use bDon'tDoIt for these two
		// purposes
		if (bThisDocChanged && !bDontDoIt)
		{
			bAnyDocChanged = TRUE;
			// Save the current document before proceeding
			wxCommandEvent evt;
			m_pApp->m_bShowProgress = false;	// edb 16Oct12: explicitly set m_bShowProgress before OnFileSave()
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
			bSavedOK = m_pApp->SaveKB(FALSE, FALSE);
		if (!bSavedOK)
		{
			wxMessageBox(_("Warning: something went wrong doing a save of the KB"),
							_T(""), wxICON_INFORMATION | wxOK);
			m_pApp->LogUserAction(_T("Warning: something went wrong doing a save of the KB"));
		}
	}
	pStatusBar->FinishProgress(_("DoKBRestore"));

	// BEW 20Apr12, added !bDontDoIt -- see comment above for why
	if (bAnyDocChanged && !bDontDoIt)
	{

		wxLogNull logNo; // avoid spurious messages from the system

		// The wxArrayString errors contains all the text to be written to the log file
		errors.Add(_T("\n\nEnd of log."));
		// Write out errors to external log file.
		bool bOK;
		bOK = ::wxSetWorkingDirectory(m_pApp->m_curProjectPath);
		wxASSERT(bOK);
        bOK = bOK; // avoid warning (we don't expect this function to fail)
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
		wxMessageBox(msg,_T(""), wxICON_INFORMATION | wxOK);
		m_pApp->LogUserAction(msg);
	}
	// whm Note: the pProgDlg->Destroy() is done back in the caller function OnFileRestoreKb() on the App
	errors.Clear(); // clear the array
}
//*
#if defined(_KBSERVER)

// Return TRUE if there was no error, FALSE otherwise
// Use for phrasebox typed adaptations or glosses, and for KBEditor's Add button
// Implements the following logic:
// 1. Determine if the src/tgt pair is in the kbserver database or not
// 2. If it's not present, upload and create a new entry with deleted flag set to 0
// 3. If it's present, it could be a normal entry or a pseudo-deleted one, so use the
//    result from 1. to find out which is the case
// 4. If it's a pseudo-deleted entry, then it needs to be undeleted, so do a PUT to
//    effect that change.
// 5. If it a normal entry, refrain from accessing kbserver as there is nothing to do
// 6. When transliteration mode is active, the local KBs are being used for a special
// purpose and must not be allowed to put heaps of <Not In KB> entries in a normal shared
// KB and so we test for the app's flag being TRUE and silently return when it is
// BEW 26Jan13, added a cache option, with default being to have it turned ON
bool CKB::HandleNewPairCreated(int kbServerType, wxString srcKey, wxString translation,
									bool bUseCache)
{
	if (m_pApp->m_bTransliterationMode)
	{
		// disallow KB sharing support for transliteration mode
		return TRUE;
	}
	bool rv = TRUE;
//* disable temporarily, 7Feb13

	KbServer* pKBSvr = m_pApp->GetKbServer(kbServerType);
	if (pKBSvr != NULL)
	{
		if (bUseCache)
		{
			// With caching we are relying on the code at the remote server implimenting
			// the following protocol:
			// 1. take the incoming source key, and it's paired translation string, and
			// query the database to see if there is a matching record.
			// 2. If 1. provides no match, then create a new record and give it whatever
			// value of the deleted flag is received from the client machine.
			// 3. If 1 provides a match, then compare the database entries deleted flag
			// value with the value of the deleted flag received from the client machine.
			// If the two values are identical, then the database already has the required
			// record, so abandon the data that came in; if the two values differ, then
			// change the database entry to have whatever value for the deleted flag that
			// came from the client machine.
			wxArrayInt* pCacheDeletedArray = pKBSvr->GetCacheDeletedArray();
			wxArrayString* pCacheSourceArray = pKBSvr->GetCacheSourceArray();
			wxArrayString* pCacheTargetArray = pKBSvr->GetCacheTargetArray();
			pCacheDeletedArray->Add(0); // 0 is 'false', i.e. a normal entry
			pCacheSourceArray->Add(srcKey);
			pCacheTargetArray->Add(translation);
			return rv;
		}
		int responseCode = pKBSvr->LookupEntryFields(srcKey, translation);
		if (responseCode != 0) // entry is not in the kbserver if test yields TRUE
		{
			//  POST the new entry to the kbserver, with bDeletedFlag set to FALSE
			responseCode = pKBSvr->CreateEntry(srcKey, translation, FALSE);
			if (responseCode != CURLE_OK)
			{
				// TODO a function to show the error code and a meaningful
				// explanation
				wxString msg;
				msg = msg.Format(_T("HandleNewPairTyped(), call of CreateEntry() failed: responseCode = %d"), responseCode);
				wxMessageBox(msg, _T("Error in CreateEntry"), wxICON_EXCLAMATION | wxOK);
				m_pApp->LogUserAction(msg);
				rv = FALSE; // but don't abort
			}
		}
		else if (responseCode == 0)
		{
			// An entry for the src/tgt pair is in the kbserver, but it may be
			// pseudo-deleted, or it may be undeleted. If the former, then we must
			// now undelete it. If the latter, we refrain from further action.
			if ((*pKBSvr->GetDeletedArray())[0] == 1)
			{
				// It's currently pseudo-deleted, so do a PUT to undelete it.
				// The first param is the kbserver database's entryID value gleaned from
				// the id field in the entry returned by the LookupEntryFields() call above)
				responseCode = pKBSvr->PseudoDeleteOrUndeleteEntry(((int)(*pKBSvr->GetIDsArray())[0]), doUndelete);
				if (responseCode != CURLE_OK)
				{
					// TODO a function to show the error code and a meaningful
					// explanation
					wxString msg;
					msg = msg.Format(_T("HandleNewPairTyped(), call of PseudoDeleteOrUndeleteEntry()failed for doUndelete: responseCode = %d"), responseCode);
					wxMessageBox(msg, _T("Error in PseudoDeleteOrUndeleteEntry"), wxICON_EXCLAMATION | wxOK);
					m_pApp->LogUserAction(msg);
					rv = FALSE; // but don't abort
				}
			}
		}
	}
	else
	{
		// Tell developer: logic error elsewhere has m_pKbServer still NULL, fix it.
		wxString msg = _T("HandleNewPairTyped(), call of CreateEntry() not done because m_pKbServer is NULL");
		wxMessageBox(msg);
		m_pApp->LogUserAction(msg);
		rv = FALSE; // but don't abort
	}
//*/
	return rv;
}

// Return TRUE if there was no error, FALSE otherwise
// Use for KBEditor's Update button, which pseudo-deletes the wrongly spelled entry and
// creates a new one with correct spelling in the local KB, so reproduce this logic in
// the kbserver database, but don't assume that the old wrongly spelled entry is actually
// there already (there's a small possibility it may not be)
// TODO -- the rest for a pseudo-delete (including sending the new respelled entry)


// Return TRUE if there was no error, FALSE otherwise
// Use within KB Editor's handler for the Remove button which pseudo-deletes in the local
// KB a src/tgt pair (i.e. a CRefString instance within the list in the current pTgtUnit). A
// pseudo-delete therefore needs to be reproduced in the kbserver database, but don't
// assume that the kbserver entry actually is in the database already (it might not be) nor
// that if it is, it is currently a normal one (it may in fact be a pseudo-deleted one,
// though the likelihood is small, due to the actions of another connected user)
// Implements the following logic:
// 1. Determine if the src/tgt pair is in the kbserver database or not
// 2. If it's not present, upload and create a new entry but with its deleted flag set to 1
// 3. If it's present, it could be a normal entry or a pseudo-deleted one, so use the
//    result from 1. to find out which is the case
// 4. If it's a normal entry, then it needs to be pseudo-deleted, do a PUT to effect
//    that change.
// 5. If it a pseudo-deleted entry already, refrain from accessing kbserver as there
//    is nothing to do
// 6. When transliteration mode is active, the local KBs are being used for a special
// purpose and must not be allowed to put heaps of <Not In KB> entries in a normal shared
// KB and so we test for the app's flag being TRUE and silently return when it is
// BEW 26Jan13, added a cache option, with default being to have it turned ON
bool CKB::HandlePseudoDelete(int kbServerType, wxString srcKey, wxString translation,
									bool bUseCache)
{
	if (m_pApp->m_bTransliterationMode)
	{
		// disallow KB sharing support for transliteration mode
		return TRUE;
	}
	bool rv = TRUE;
	KbServer* pKBSvr = m_pApp->GetKbServer(kbServerType);
	if (pKBSvr != NULL)
	{
		if (bUseCache)
		{
			// With caching we are relying on the code at the remote server implimenting
			// the following protocol:
			// 1. take the incoming source key, and it's paired translation string, and
			// query the database to see if there is a matching record.
			// 2. If 1. provides no match, then create a new record and give it whatever
			// value of the deleted flag is received from the client machine.
			// 3. If 1 provides a match, then compare the database entries deleted flag
			// value with the value of the deleted flag received from the client machine.
			// If the two values are identical, then the database already has the required
			// record, so abandon the data that came in; if the two values differ, then
			// change the database entry to have whatever value for the deleted flag that
			// came from the client machine.
			wxArrayInt* pCacheDeletedArray = pKBSvr->GetCacheDeletedArray();
			wxArrayString* pCacheSourceArray = pKBSvr->GetCacheSourceArray();
			wxArrayString* pCacheTargetArray = pKBSvr->GetCacheTargetArray();
			pCacheDeletedArray->Add(1); // 1 is 'true' i.e. we want pseudo-deletion
			pCacheSourceArray->Add(srcKey);
			pCacheTargetArray->Add(translation);
			return rv;
		}
		pKBSvr->ClearAllPrivateStorageArrays();
		// note: pKBSvr knows whether itself is an adapting KB server, or a glossing one
		int responseCode = pKBSvr->LookupEntryFields(srcKey, translation);
		if (responseCode != 0) // entry is not in the kbserver if test yields TRUE
		{
			//  POST the new entry to the kbserver - but with bDeletedFlag value of TRUE
			responseCode = pKBSvr->CreateEntry(srcKey, translation, TRUE);
			if (responseCode != CURLE_OK)
			{
				// TODO a function to show the error code and a meaningful
				// explanation
				wxString msg;
				msg = msg.Format(_T("HandlePseudoDelete(), call of CreateEntry() failed: responseCode = %d"), responseCode);
				m_pApp->LogUserAction(msg);
				wxMessageBox(msg, _T("Error in CreateEntry"), wxICON_EXCLAMATION | wxOK);
				rv = FALSE; // but don't abort
			}
		}
		else if (responseCode == 0)
		{
            // An entry for the src/tgt pair is in the kbserver, but it may be normal (i.e.
            // not pseudo-deleted), or it may be pseudo-deleted. If the former, then we
            // must now pseudo-delete it. If the latter, we refrain from further action.
			if ((*pKBSvr->GetDeletedArray())[0] == 0)
			{
				// It's currently not pseudo-deleted, so do a PUT to pseudo-delete it.
				// The first param is the kbserver database's entryID value gleaned from
				// the id field in the entry returned by the LookupEntryFields() call above)
				responseCode = pKBSvr->PseudoDeleteOrUndeleteEntry((*pKBSvr->GetIDsArray())[0], doDelete);
				if (responseCode != CURLE_OK)
				{
					// TODO a function to show the error code and a meaningful
					// explanation
					wxString msg;
					msg = msg.Format(_T("HandlePseudoDelete(), call of PseudoDeleteOrUndeleteEntry() failed for doDelete: responseCode = %d"), responseCode);
					m_pApp->LogUserAction(msg);
					wxMessageBox(msg, _T("Error in PseudoDeleteOrUndeleteEntry"), wxICON_EXCLAMATION | wxOK);
					rv = FALSE; // but don't abort
				}
			}
		}
	}
	else
	{
		// Tell developer: logic error elsewhere has m_pKbServer still NULL, fix it.
		wxString msg = _T("HandlePseudoDelete(), nothing done in the call because m_pKbServer is NULL");
		wxMessageBox(msg);
		m_pApp->LogUserAction(msg);
		rv = FALSE; // but don't abort
	}
	return rv;
}

// Return TRUE if there was no error, FALSE otherwise
// Use for phrase box typing which creates a src/tgt pair to be treated as a normal local
// KB entry when the same pair has been earlier pseudo-deleted. A local undelete is needed.
// So reproduce this outcome also in the kbserver database, but don't assume that the entry
// actually is in the database already (it might not be) nor that if it is, it is currently
// a pseudo-deleted one (it may in fact be a normal one, though the likelihood is small,
// due to the actions of another connected user)
// Implements the following logic:
// 1. Determine if the src/tgt pair is in the kbserver database or not
// 2. If it's not present, upload and create a new normal entry
// 3. If it's present, it could be a normal entry or a pseudo-deleted one, so use the
//    result from 1. to find out which is the case
// 4. If it's a pseudo-deleted entry, then it needs to be undeleted, do a PUT to effect
//    that change.
// 5. If it a normal entry, refrain from accessing kbserver as there is nothing to do
// 6. When transliteration mode is active, the local KBs are being used for a special
// purpose and must not be allowed to put heaps of <Not In KB> entries in a normal shared
// KB and so we test for the app's flag being TRUE and silently return when it is
// BEW 26Jan13, added a cache option, with default being to have it turned ON
bool CKB::HandlePseudoUndelete(int kbServerType, wxString srcKey, wxString translation,
									bool bUseCache)
{
	if (m_pApp->m_bTransliterationMode)
	{
		// disallow KB sharing support for transliteration mode
		return TRUE;
	}
	bool rv = TRUE;
	KbServer* pKBSvr = m_pApp->GetKbServer(kbServerType);
	if (pKBSvr != NULL)
	{
		if (bUseCache)
		{
			// With caching we are relying on the code at the remote server implimenting
			// the following protocol:
			// 1. take the incoming source key, and it's paired translation string, and
			// query the database to see if there is a matching record.
			// 2. If 1. provides no match, then create a new record and give it whatever
			// value of the deleted flag is received from the client machine.
			// 3. If 1 provides a match, then compare the database entries deleted flag
			// value with the value of the deleted flag received from the client machine.
			// If the two values are identical, then the database already has the required
			// record, so abandon the data that came in; if the two values differ, then
			// change the database entry to have whatever value for the deleted flag that
			// came from the client machine.
			wxArrayInt* pCacheDeletedArray = pKBSvr->GetCacheDeletedArray();
			wxArrayString* pCacheSourceArray = pKBSvr->GetCacheSourceArray();
			wxArrayString* pCacheTargetArray = pKBSvr->GetCacheTargetArray();
			pCacheDeletedArray->Add(0); // 0 is 'false' i.e. we want undeletion giving a normal entry
			pCacheSourceArray->Add(srcKey);
			pCacheTargetArray->Add(translation);
			return rv;
		}
		pKBSvr->ClearAllPrivateStorageArrays();
		int responseCode = pKBSvr->LookupEntryFields(srcKey, translation);
		if (responseCode != 0) // entry is not in the kbserver if test yields TRUE
		{
			//  POST the new entry to the kbserver, with bDeletedFlag set to FALSE
			responseCode = pKBSvr->CreateEntry(srcKey, translation, FALSE);
			if (responseCode != CURLE_OK)
			{
				// TODO a function to show the error code and a meaningful
				// explanation
				wxString msg;
				msg = msg.Format(_T("HandleNewPairTyped(), call of CreateEntry() failed: responseCode = %d"), responseCode);
				m_pApp->LogUserAction(msg);
				wxMessageBox(msg, _T("Error in CreateEntry"), wxICON_EXCLAMATION | wxOK);
				rv = FALSE; // but don't abort
			}
		}
		else if (responseCode == 0)
		{
			// An entry for the src/tgt pair is in the kbserver, but it may be
			// pseudo-deleted, or it may be undeleted. If the former, then we must
			// now undelete it. If the latter, we refrain from further action.
			if ((*pKBSvr->GetDeletedArray())[0] == 1)
			{
				// It's currently pseudo-deleted, so do a PUT to undelete it.
				// The first param is the kbserver database's entryID value gleaned from
				// the id field in the entry returned by the LookupEntryFields() call above)
				responseCode = pKBSvr->PseudoDeleteOrUndeleteEntry((*pKBSvr->GetIDsArray())[0], doUndelete);
				if (responseCode != CURLE_OK)
				{
					// TODO a function to show the error code and a meaningful
					// explanation
					wxString msg;
					msg = msg.Format(_T("HandleNewPairTyped(), call of PseudoDeleteOrUndeleteEntry() failed for doUndelete: responseCode = %d"), responseCode);
					m_pApp->LogUserAction(msg);
					wxMessageBox(msg, _T("Error in PseudoDeleteOrUndeleteEntry"), wxICON_EXCLAMATION | wxOK);
					rv = FALSE; // but don't abort
				}
			}
		}
	}
	else
	{
		// Tell developer: logic error elsewhere has m_pKbServer still NULL, fix it.
		wxMessageBox(_T("HandleUndelete(), did nothing because m_pKbServer is NULL"));
		rv = FALSE; // but don't abort
	}
	return rv;
}

// Use the next when in the KB Editor for the local KB, the user corrects an
// incorrectly spelled adaptation or gloss. Internally this is implemented as a
// pseudo-delete of the old incorrectly spelled entry, together with creation of a new
// entry with the corrected src/tgt or src/gloss pair. Hence, the kbserver support can
// simply do HandlePseudoDelete() using the old pair, followed by
// HandleNewPairCreated() using the new src/tgt or src/gloss pair.
// NOTE: When transliteration mode is active, the local KBs are being used for a special
// purpose and must not be allowed to put heaps of <Not In KB> entries in a normal shared
// KB and so we test for the app's flag being TRUE and silently return when it is
bool  CKB::HandlePseudoDeleteAndNewPair(int kbServerType, wxString srcKey,
						wxString oldTranslation, wxString newTranslation,
									bool bUseCache)
{
	if (m_pApp->m_bTransliterationMode)
	{
		// disallow KB sharing support for transliteration mode
		return TRUE;
	}
	bool rv = TRUE;
	rv = HandlePseudoDelete(kbServerType, srcKey, oldTranslation, bUseCache);
	if (rv)
	{
		rv = HandleNewPairCreated(kbServerType, srcKey, newTranslation, bUseCache);
		if (!rv)
		{
			// if there was an error, log the strings, but do no more and let
			// processing continue
			wxString msg = _T("pseudo-delted OLD: ");
			msg += srcKey + _T(" : ") + oldTranslation;
			msg += _T("Error when creating NEW: ");
			msg += srcKey + _T(" : ") + newTranslation;
			m_pApp->LogUserAction(msg);
		}
	}
	else
	{
		// if there was an error, log the strings, but do no more and let
		// processing continue
		wxString msg = _T("Error in pseudo-deleting OLD: ");
		msg += srcKey + _T(" : ") + oldTranslation;
		msg += _T("Creation of new pair untried for NEW: ");
		msg += srcKey + _T(" : ") + newTranslation;
		m_pApp->LogUserAction(msg);
	}
	return rv;
}

// Use the next when in the KB Editor for the local KB, the user changes an an adaptation
// or gloss by editing and then using the Update button, and the edited entry has become
// identical to an existing pseudo-deleted entry not shown in the list box. Internally this
// is implemented as a pseudo-delete of the old entry (the "oldTranslation" parameter in
// the signature), together with creation of a new entry with undeletion (and display in
// place of the old translation in the list box) of the "newTranslation" parameter. Hence,
// the kbserver support can simply do HandlePseudoDelete() using the 3rd argument, followed
// by HandleUndelete() using the 4th parameter.
// NOTE: When transliteration mode is active, the local KBs are being used for a special
// purpose and must not be allowed to put heaps of <Not In KB> entries in a normal shared
// KB and so we test for the app's flag being TRUE and silently return when it is
bool  CKB::HandlePseudoDeleteAndUndeleteDeletion(int kbServerType, wxString srcKey,
						wxString oldTranslation, wxString newTranslation,
									bool bUseCache)
{
	if (m_pApp->m_bTransliterationMode)
	{
		// disallow KB sharing support for transliteration mode
		return TRUE;
	}
	bool rv = TRUE;
	rv = HandlePseudoDelete(kbServerType, srcKey, oldTranslation, bUseCache);
	if (rv)
	{
		rv = HandlePseudoUndelete(kbServerType, srcKey, newTranslation, bUseCache);
		if (!rv)
		{
			// if there was an error, log the strings, but do no more and let
			// processing continue
			wxString msg = _T("pseudo-deleted OLD: ");
			msg += srcKey + _T(" : ") + oldTranslation;
			msg += _T("Error in undelete NEW: ");
			msg += srcKey + _T(" : ") + newTranslation;
			m_pApp->LogUserAction(msg);
		}
	}
	else
	{
		// if there was an error, log the strings, but do no more and let
		// processing continue
		wxString msg = _T("Error in pseudo-delete OLD: ");
		msg += srcKey + _T(" : ") + oldTranslation;
		msg += _T("did not try undelete of NEW: ");
		msg += srcKey + _T(" : ") + newTranslation;
		m_pApp->LogUserAction(msg);
	}
	return rv;
}





#endif // for _KBSERVER
//*/
