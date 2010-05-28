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

#include "Adapt_It.h"
#include "Adapt_ItView.h"
#include "SourcePhrase.h"
#include "helpers.h"
#include "Adapt_ItDoc.h"
#include "KB.h"
#include "AdaptitConstants.h" 
#include "TargetUnit.h"
#include "RefString.h"
#include "LanguageCodesDlg.h"
#include "BString.h"
#include "XML.h"
#include "WaitDlg.h"
#include <wx/textfile.h>


// Define type safe pointer lists
#include "wx/listimpl.cpp"

/// This macro together with the macro list declaration in the .h file
/// complete the definition of a new safe pointer list class called TUList.
WX_DEFINE_LIST(TUList);	// see WX_DECLARE_LIST in the .h file

IMPLEMENT_DYNAMIC_CLASS(CKB, wxObject) 

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

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
extern bool gbInhibitLine4StrCall;
extern bool gbRemovePunctuationFromGlosses;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CKB::CKB()
{
	m_pApp = &wxGetApp();
	m_nMaxWords = 1; // value for a minimal phrase

	// whm Note: I've changed the order of the following in order to create
	// the TUList before the MapStringToOjbect maps. See notes in KB.h.
	m_pTargetUnits = new TUList;
	wxASSERT(m_pTargetUnits != NULL);

	for (int i = 0; i< MAX_WORDS; i++)
	{
		m_pMap[i] = new MapKeyStringToTgtUnit;
		wxASSERT(m_pMap[i] != NULL);
	}
	m_kbVersionCurrent = 2; // current default
}

CKB::CKB(bool bGlossingKB)
{
	m_pApp = &wxGetApp();
	m_nMaxWords = 1; // value for a minimal phrase

	// whm Note: I've changed the order of the following in order to create
	// the TUList before the MapStringToOjbect maps. See notes in KB.h.
	m_pTargetUnits = new TUList;
	wxASSERT(m_pTargetUnits != NULL);

	for (int i = 0; i< MAX_WORDS; i++)
	{
		m_pMap[i] = new MapKeyStringToTgtUnit;
		wxASSERT(m_pMap[i] != NULL);
	}
	m_bGlossingKB = bGlossingKB; // set the KB type, TRUE for GlossingKB, 
								   // FALSE for adapting KB
	m_kbVersionCurrent = 2; // current default -- but add 2nd param soon for kb version
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

	CObList* pTUList = pCopy->m_pTargetUnits;

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

void CKB::Copy(const CKB& kb)
// the copy can be done efficiently only by scanning the maps one by one (so we know how many
// source words are involved each time), and for each key, construct a copy of it (because RemoveAll()
// done on a CMapStringToOb will delete the key strings, but leave the CObject* memory untouched, so
// the keys have to be CString copies of the souce CKB's key CStrings), and then use
// StoreAdaptation to create the new CKB's contents; for glossing, the copy is done only for the
// first map, and m_nMaxWords must always remain equal to 1.
{

	wxASSERT(this);
	const CKB* pCopy = &kb;
	wxASSERT(pCopy);

	//if (gbIsGlossing)
	if (pCopy->m_bGlossingKB) // BEW changed test 12May10
	{
		m_nMaxWords = 1;
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

	TUList* pTUList = pCopy->m_pTargetUnits;
	wxASSERT(pTUList);

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
				pTUList->Append(pNewTU);
			}
		}
	}
}

// the "adaptation" parameter will contain an adaptation if gbIsGlossing is FALSE, or if
// TRUE if will contain a gloss; and also depending on the same flag, the pTgtUnit will have
// come from either the adaptation KB or the glossing KB.
// BEW 11May10, moved from view class, and made private (it's only called once, in GetRefString())
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
			return pRefStr; // we found it
		else
		{
			if (adaptation.IsEmpty())
			{
				// it might be a <Not In KB> source phrase, check it out
				if (pRefStr->m_translation == _T("<Not In KB>"))
					return pRefStr; // we return the pointer to this too
			}
		}
	}
	// finding it failed so return NULL
	return (CRefString*)NULL;
}

// in this function, the keyStr parameter will always be a source string; the caller must
// determine which particular map is to be looked up and provide it's pointer as the first
// parameter; and if the lookup succeeds, pTU is the associated CTargetUnit instance's
// poiner. This function, as the name suggests, has the smarts for AutoCapitalization being
// On or Off.
// WX Note: Changed second parameter to CTargetUnit*& pTU.
// BEW 12Apr10, no changes needed for support of doc version 5
// BEW 13May10, moved here from CAdapt_ItView class
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
		bool bNoError = m_pApp->GetDocument()->SetCaseParameters(keyStr); // extra param is TRUE since
												   // it is source text
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
// (Note: Jan 27 2001 changed so that returns the pRefString for a <Not In KB> entry). For
// version 2.0 and later, pKB will point to the glossing KB when gbIsGlossing is TRUE.
// Ammended, July 2003, to support auto capitalization
// BEW 11May10, moved from CAdapt_ItView class
// BEW changed signature 12May10, because from 5.3.0 and onwards, each KB now knows whether
// it is a glossing KB or an adapting KB -- using private member boolean m_bGlossingKB
CRefString* CKB::GetRefString(int nSrcWords, wxString keyStr, wxString valueStr)
{
	// ensure nSrcWords is 1 if this is a GlossingKB access
	if (m_bGlossingKB)
		nSrcWords = 1;
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
/// version 2.0 and onwards we must test gbIsGlossing in order to set the KB pointer to
/// either the adaption KB or the glossing KB; and the caller must supply the appropriate
/// first and last parameters (ie. a pRefString from the glossing KB and nWordsInPhrase set
/// to 1 when gbIsGlossing is TRUE)
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
void CKB::RemoveRefString(CRefString *pRefString, CSourcePhrase* pSrcPhrase, int nWordsInPhrase)
{
	if (m_bGlossingKB)
	{
		pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
		nWordsInPhrase = 1; // ensure correct value for a glossing KB
	}
	else
		pSrcPhrase->m_bHasKBEntry = FALSE;
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
        // are, so m_bAlwaysAsk is unaffected. Version 2 has to test gbIsGlossing and set
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
        // from the glossing KB, depending on gbIsGlossing flag's value
		CTargetUnit* pTU = pRefString->m_pTgtUnit; // point to the owning targetUnit
		wxASSERT(pTU != NULL);
		int nTranslations = pTU->m_pTranslations->GetCount();
		wxASSERT(nTranslations > 0); // must be at least one
		if (nTranslations == 1)
		{
			// we are removing the only CRefString in the owning targetUnit, so the latter must
			// go as well
			CTargetUnit* pTgtUnit;
			delete pRefString;
			pRefString = (CRefString*)NULL;
			// since we delete pRefString, TranslationsList::Clear() should do the job below
			pTU->m_pTranslations->Clear();

			TUList::Node* pos;

			pos = m_pTargetUnits->Find(pTU); // find position of pRefString's
												  // owning targetUnit

            // Note: A check for NULL should probably be done here anyway even if when
            // working properly a NULL return value on Find shouldn't happen.
			pTgtUnit = (CTargetUnit*)pos->GetData(); // get the targetUnit in
																	// the list
			wxASSERT(pTgtUnit != NULL);
			m_pTargetUnits->DeleteNode(pos); // remove its pointer from the list
			delete pTgtUnit; // delete its instance from the heap
			pTgtUnit = (CTargetUnit*)NULL;

			MapKeyStringToTgtUnit* pMap = m_pMap[nWordsInPhrase - 1];
			int bRemoved = 0;
			if (gbAutoCaps)
			{
                // try removing the lower case one first, this is the most likely one that
                // was found by GetRefString( ) in the caller
				bRemoved = pMap->erase(s1); // also remove it from the map
			}
			if (bRemoved == 0)
			{
				// have a second shot using the unmodified string s
				bRemoved = pMap->erase(s); // also remove it from the map
			}
			wxASSERT(bRemoved == 1);
            // the above algorith will handle all possibilites except one; if earlier auto
            // caps is ON, and the user stores an entry with source text starting with
            // upper case, (which will be changed to lower case for the storage), and then
            // later on s/he turns auto caps OFF, then the entry would not be found by the
            // above line:
            // bRemoved = pMap->erase(s); and then the wxASSERT would trip; however, we
            // are saved from this happening because the pRefString passed in is provided
            // by a prior GetRefString( ) call in the caller, and that would not find the
            // pRefString, and as a consequence it would return NULL, and so the in this
            // block of RemoveRefString( ) the removal would not be asked for; so the
            // wxASSERT would not trip.
		}
		else
		{
			// there are other CRefString instances, so don't remove its owning targetUnit
			TranslationsList::Node* pos = pTU->m_pTranslations->Find(pRefString);
			wxASSERT(pos != NULL); // it must be in the list somewhere
			pTU->m_pTranslations->DeleteNode(pos); // remove that refString from the list
												   // in the targetUnit
			delete pRefString; // delete it from the heap
			pRefString = (CRefString*)NULL;
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
// BEW 12Apr10, no changes needed for support of _DOVCER5
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
void CKB::GetAndRemoveRefString(CSourcePhrase* pSrcPhrase, wxString& targetPhrase, 
								enum UseForLookup useThis)
{
	CRefString* pRefStr = NULL;
	if (m_bGlossingKB)
	{
		if (useThis == useTargetPhraseForLookup)
		{
			pRefStr = GetRefString(1, pSrcPhrase->m_key, targetPhrase);
		}
		else // useThis has the value useGlossOrAdaptationForLookup
		{
			pRefStr = GetRefString(1, pSrcPhrase->m_key, pSrcPhrase->m_gloss);
		}
		// ensure correct flag value
		if (pRefStr == NULL && pSrcPhrase->m_bHasGlossingKBEntry)
			pSrcPhrase->m_bHasGlossingKBEntry = FALSE; // must be FALSE for a successful lookup on return
		if (pRefStr != NULL)
			RemoveRefString(pRefStr, pSrcPhrase, 1);
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
			RemoveRefString(pRefStr, pSrcPhrase, pSrcPhrase->m_nSrcWords);
	}
}

// returns TRUE if a matching KB entry found; when glossing, pKB points to the glossing KB, when
// adapting it points to the normal KB
// BEW 26Mar10, no changes needed for support of doc version 5
// BEW 13May10, moved here from CPhraseBox class
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
bool CKB::IsAlreadyInKB(int nWords,wxString key,wxString adaptation)
{
	CTargetUnit* pTgtUnit = 0;

	// is there a targetunit for this key in the KB?
	bool bFound;
	if (m_bGlossingKB)
		// only check first map
		bFound = FindMatchInKB(1,key,pTgtUnit); 
	else // is adapting
		bFound = FindMatchInKB(nWords,key,pTgtUnit);
	if (!bFound)
		return FALSE;

	// check if there is a matching adaptation
	TranslationsList::Node* pos = pTgtUnit->m_pTranslations->GetFirst(); 
	while (pos != 0)
	{
		CRefString* pRefStr = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pRefStr);
		if (adaptation == pRefStr->m_translation)
			return TRUE; // adaptation, or gloss, matches this entry
	}
	return FALSE; // did not find a match
}

void CKB::DoKBImport(wxString pathName,enum KBImportFileOfType kbImportFileOfType)
{
	CSourcePhrase* pSrcPhrase = new CSourcePhrase;
	// guarantee safe value for storage of contents to KB, or glossing KB
	if (m_bGlossingKB)
		pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
	else
		pSrcPhrase->m_bHasKBEntry = FALSE;
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
	m_pApp->m_bSaveToKB = TRUE;

	if (kbImportFileOfType == KBImportFileOfLIFT_XML)
	{
		// we're importing from a LIFT file
		wxFile f;
		wxLogNull logno; // prevent unwanted system messages
		if( !f.Open( pathName, wxFile::read))
		{
			wxMessageBox(_("Unable to open import file for reading."),
		  _T(""), wxICON_WARNING);
			return;
		}
		// For LIFT import we are using wxFile and we call xml processing functions
		// from XML.cpp
		bool bReadOK;
		bReadOK = ReadLIFT_XML(pathName,m_pApp->m_pKB); // assume LIFT import goes into the regular KB

		f.Close();
	}
	else
	{
		// we're importing from an SFM text file (\lx and \ge)
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
		for (ct = 0; ct < lineCount; ct++)
		{
			line = file.GetLine(ct);
			// the data for each line is now in lineStr
			// is the line a m_key member?
			if (IsMember(line,keyMarker,nOffset) || nOffset >= 0)
			{
				// it is a valid key
				pSrcPhrase->m_key.Empty();
				if (m_bGlossingKB)
				{
					pSrcPhrase->m_gloss.Empty();
					pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
					pSrcPhrase->m_nSrcWords = 1; // temp default value
				}
				else
				{
					pSrcPhrase->m_adaption.Empty();
					pSrcPhrase->m_bHasKBEntry = FALSE;
					pSrcPhrase->m_nSrcWords = 1; // temp default value
				}
				bKeyDefined = TRUE;

				// extract the actual srcPhrase's m_key from the read in string,
				// to set the key variable
				int keyLen = line.Length();
				keyLen -= (4 + nOffset); // \lx followed by a space = 4 characters,
										 // nOffset takes care of any leading spaces
				if (keyLen > 0)
				{
					key = line.Right(keyLen);
					int nWordCount;
					if (gbIsGlossing)
						nWordCount = 1;
					else
						nWordCount = CountSourceWords(key);
					if (nWordCount == 0 || nWordCount > MAX_WORDS)
					{
						// error condition
						pSrcPhrase->m_nSrcWords = 1;
						key.Empty();
						bKeyDefined = FALSE;
					}
					else
					{
						// we have an acceptable key
						pSrcPhrase->m_nSrcWords = nWordCount;
						pSrcPhrase->m_key = key; // CountSourceWords will strip off
												 // leading or trailing spaces in key
					}
				}
				else
				{
					key.Empty();
					bKeyDefined = FALSE;
					pSrcPhrase->m_nSrcWords = 1;
				}
			}
			else
			{
				if (IsMember(line,adaptionMarker,nOffset) || nOffset >= 0)
				{
					// an adaptation member exists for this key, so get the KB updated
					// with this association provided a valid key was constructed
					if (bKeyDefined)
					{
						int adaptionLen = line.Length();
						adaptionLen -= (4 + nOffset); // \ge followed by a space = 4 characters,
													  // nOffset takes care of any leading spaces
						if (adaptionLen > 0)
						{
							adaption = line.Right(adaptionLen);
						}
						else
						{
							adaption.Empty();
						}

						// store the association in the KB, provided it is not already there
						if (m_bGlossingKB)
						{
							if (!IsAlreadyInKB(1,key,adaption)) // use 1, as m_nSrcWords could 
							{									// be set > 1 for this pSrcPhrase
								// adaption parameter is assumed to be a gloss if this is a
								// glossing KB
								StoreText(pSrcPhrase,adaption,TRUE); // BEW 27Jan09, 
														// TRUE means "allow empty string save"
							}
						}
						else
						{
							if (!IsAlreadyInKB(pSrcPhrase->m_nSrcWords,key,adaption))
								StoreText(pSrcPhrase,adaption,TRUE); // BEW 27Jan09, 
														// TRUE means "allow empty string save"
						}

						// prepare for another adaptation (or gloss) for this key
						pSrcPhrase->m_adaption.Empty();
						adaption.Empty();
					}
				}
				else
				{
					// it's neither a key nor an adaption (or gloss),
					// so probably a blank line - just ignore it
					;
				}
			}
		}
		file.Close();
	}

	// process the last line here ???
	delete pSrcPhrase;
}

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

int CKB::CountSourceWords(wxString& rStr)
{
	int len = rStr.Length();
	if (len == 0)
		return 0;

	// remove any final spaces
	wxString reverse = rStr;
	reverse = MakeReverse(reverse);
	wxChar ch = _T(' ');
a:	int nFound = reverse.Find(ch);
	if (nFound == 0)
	{
		// found one, so remove it and iterate
		reverse = reverse.Right(len-1);
		len = reverse.Length();
		goto a;
	}
	else
	{
		// restore correct order
		reverse = MakeReverse(reverse);
	}

	// remove initial spaces, if any
	len = reverse.Length();
b:	nFound = reverse.Find(ch);
	if (nFound == 0)
	{
		// found one, so remove it and iterate
		reverse = reverse.Right(len-1);
		len = reverse.Length();
		goto b;
	}

	rStr = reverse; // updates string in the caller, so it has 
					// no trailing nor leading spaces

	if (rStr.IsEmpty())
	{
		// if it was only spaces, we have no valid string
		return 0;
	}

	// now parse words
	int count = 1;
	len = reverse.Length();
c:	nFound = reverse.Find(ch);
	if (nFound > 0)
	{
		// found a space, so must be an additional word
		count++; // count it

		// reduce reverse by one word
		reverse = reverse.Right(len-nFound-1);

		// there might be a sequence of spaces, so skip any 
		// leading spaces on the reduced string
d:		nFound = reverse.Find(ch);
		if (nFound == 0)
		{
			reverse = reverse.Right(len-1);
			len = reverse.Length();
			goto d;
		}
		len = reverse.Length();
		goto c;
	}
	else
		return count;
}

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
			TranslationsList::Node* tpos = pTU->m_pTranslations->GetFirst();
			CRefString* pRefStr = (CRefString*)tpos->GetData();
			if (pRefStr->m_translation == _T("<Not In KB>"))
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}
}

// BEW modified 1Sep09 to remove a logic error, & to remove a goto command, and get rid 
// of a bDelayed boolean flag & simplify the logic
// whm modified 3May10 to add LIFT format XML export. 
// BEW 13May10 moved it here from CAdapt_ItView class
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
	//wxString outputStr; // accumulate a whole "record" here, but abandon if it contains
						// a <Not In KB> string, we don't export those
	wxString outputSfmStr; // accumulate a whole SFM "record" here, but abandon if it
						   // contains a <Not In KB> string, we don't export those
	CBString outputLIFTStr; // accumulate a whole LIFT "record" here, but abandon if it
						    // contains a <Not In KB> string, we don't export those
	wxString strNotInKB = _T("<Not In KB>");
	gloss.Empty(); // this name used for the "adaptation" when adapting,
				   // or the "gloss" when glossing

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
						langStr += _("target langugage code");
					}
					message = message.Format(_("You did not enter a language code for the following language(s):\n\n%s\n\nLIFT XML Export requires 3-letter language codes.\nDo you to try again?"),langStr.c_str());
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
		if (gbIsGlossing && numWords_sim > 1)
			continue; // when glossing we want to consider only the first map, the others
					  // are all empty
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
	


#ifdef __WXMSW__
	wxString progMsg = _("%d of %d Total entries and senses");
	wxString msgDisplayed = progMsg.Format(progMsg,1,nTotal);
	wxProgressDialog progDlg(_("Exporting the KB in LIFT format"),
							msgDisplayed,
							nTotal,    // range
							(wxWindow*)m_pApp->GetMainFrame(),   // parent
							//wxPD_CAN_ABORT |
							//wxPD_CAN_SKIP |
							wxPD_APP_MODAL |
							// wxPD_AUTO_HIDE | -- try this as well
							wxPD_ELAPSED_TIME |
							wxPD_ESTIMATED_TIME |
							wxPD_REMAINING_TIME
							| wxPD_SMOOTH // - makes indeterminate mode bar on WinXP very small
							);
#else
	// wxProgressDialog tends to hang on wxGTK so I'll just use the simpler CWaitDlg
	// notification on wxGTK and wxMAC
	// put up a Wait dialog - otherwise nothing visible will happen until the operation is done
	CWaitDlg waitDlg((wxWindow*)m_pApp->GetMainFrame());
	// indicate we want the reading file wait message
	waitDlg.m_nWaitMsgNum = 14;	// 0 "Please wait while Adapt It exports the KB..."
	waitDlg.Centre();
	waitDlg.Show(TRUE);
	waitDlg.Update();
	// the wait dialog is automatically destroyed when it goes out of scope below.
#endif
	
	int numWords;
	int counter = 0;
	MapKeyStringToTgtUnit::iterator iter;
	CTargetUnit* pTU = 0;
	CRefString* pRefStr;
	for (numWords = 1; numWords <= MAX_WORDS; numWords++)
	{
		if (gbIsGlossing && numWords > 1)
			continue; // when glossing we want to consider only the first map, the others
					  // are all empty
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
					TUList::Node* pos = m_pTargetUnits->Find(pTU); 
					wxASSERT(pos != NULL);
					m_pTargetUnits->DeleteNode(pos); // its CTargetUnit ptr is now 
													 // gone from list
					delete pTU; // and its memory chunk is freed
					continue;
				}
				else
					posRef = pTU->m_pTranslations->GetFirst(); 
				wxASSERT(posRef != 0);

				counter += pTU->m_pTranslations->GetCount();

				// if control gets here, there will be at least one non-null posRef and so
				// we can now unilaterally write out the key or basekey data. 

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
					// 
					// Get the uuid from the CTargetUnit object using pTU->GetUuid()
					// TODO: use the commented out one below after Bruce implements CTargetUnit::GetUuid()
					// next three lines are temporary...
					wxString tempGuid = GetUuid();
					const wxCharBuffer buff = tempGuid.utf8_str();
					guidForThisLexItem = buff;
					// the uuid is of the form "45a3c52b-79fd-4803-856b-207c3efdbaf8"
					//const wxCharBuffer buff = pTU->GetUuid().utf8_str();
					//guidForThisLexItem = buff;
					composeXmlStr = indent2sp;
					composeXmlStr += "<entry guid=\"";
					composeXmlStr += guidForThisLexItem;
					composeXmlStr += "\" id=\"";
#ifdef _UNICODE
					tempCBstr = m_pApp->Convert16to8(baseKey);
#else
					tempCBstr = baseKey.c_str(); // check this use of .c_str()???
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
					tempCBstr = baseKey.c_str(); // check this use of .c_str()???
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
					gloss = geSFM + gloss; // we put the proper eol char(s) below when writing
					outputSfmStr += gloss + m_pApp->m_eolStr;
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
					tempCBstr = baseGloss.c_str(); // check this use of .c_str()???
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
						gloss = geSFM + gloss; // we put the proper eol char(s) below when writing
						outputSfmStr += gloss + m_pApp->m_eolStr;
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
						tempCBstr = baseGloss.c_str(); // check this use of .c_str()???
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
#ifdef __WXMSW__
					// update the progress bar every 20th iteration
					if (counter % 200 == 0)
					{
						msgDisplayed = progMsg.Format(progMsg,counter,nTotal);
						progDlg.Update(counter,msgDisplayed);
					}
#endif
				}

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
					// reject any outputSfmStr which contains "<Not In KB>"
					if (outputSfmStr.Find(strNotInKB) == wxNOT_FOUND)
					{
							// the entry is good, so output it
							#ifndef _UNICODE // ANSI version
								pFile->Write(outputSfmStr); 
							#else // Unicode version
								m_pApp->ConvertAndWrite(wxFONTENCODING_UTF8,pFile,outputSfmStr);
							#endif
					}
				}

#ifdef __WXMSW__
				// update the progress bar every 200th iteration
				if (counter % 200 == 0)
				{
					msgDisplayed = progMsg.Format(progMsg,counter,nTotal);
					progDlg.Update(counter,msgDisplayed);
				}
#endif
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
}

// looks up the knowledge base to find if there is an entry in the map with index
// nSrcWords-1, for the key keyStr and returns the CTargetUnit pointer it finds. If it
// fails, it returns a null pointer.
// version 2.0 and onwards supports glossing, so pKB param could point to the glossing KB or the
// adapting one, as set by the caller; and for glossing it is the caller's responsibility
// to ensure that nSrcWords has the value 1 only.
// BEW 13May10, moved here from CAdapt_ItView and removed pKB param from signature
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

void CKB::DoNotInKB(CSourcePhrase* pSrcPhrase, bool bChoice)
{
	if (bChoice)
	{
        // user wants it to not be in the KB, so set it up accordingly... first thing to do
        // is to remove all existing translations from this source phrase's entry in the KB
		CTargetUnit* pTgtUnit = GetTargetUnit(pSrcPhrase->m_nSrcWords,pSrcPhrase->m_key);
		if (pTgtUnit != NULL)
		{
			TranslationsList* pList = pTgtUnit->m_pTranslations;
			if (!pList->IsEmpty())
			{
				TranslationsList::Node* pos = pList->GetFirst();
				while (pos != NULL)
				{
					CRefString* pRefString = (CRefString*)pos->GetData();
					pos = pos->GetNext();
					if (pRefString != NULL)
					{
						delete pRefString;
						pRefString = (CRefString*)NULL;
					}
				}
				pList->Clear();

				// have to get rid of the pTgtUnit too, as its m_translation list 
				// must not be empty
				TUList::Node* tupos = m_pTargetUnits->Find(pTgtUnit); // find 
										// position of the bad targetUnit in the list
				// get the targetUnit in the list
				CTargetUnit* pTU = (CTargetUnit*)tupos->GetData(); 
				wxASSERT(pTU != NULL && pTU->m_pTranslations->IsEmpty()); // have we found it?
				m_pTargetUnits->DeleteNode(tupos); // remove it from the list
				delete pTU; // delete it from the heap
				pTU = (CTargetUnit*)NULL;

				MapKeyStringToTgtUnit* pMap = m_pMap[pSrcPhrase->m_nSrcWords - 1];
				// handle auto-caps tweaking, if it is on
				wxString temp = pSrcPhrase->m_key;
				if (gbAutoCaps && !gbNoSourceCaseEquivalents)
				{
                    // auto caps is on and there are case equivalences defined, so check if
                    // we must convert the key to lower case for the removal operation (so
                    // that it succeeds, since the KB would have a lower case key stored,
                    // not upper case
					bool bNoError = m_pApp->GetDocument()->SetCaseParameters(temp); // TRUE for default 
															 // parameter = src text
					if (bNoError && gbSourceIsUpperCase && (gcharSrcLC != _T('\0')))
					{
						temp.SetChar(0,gcharSrcLC); // make first char lower case
					}
				}
				int bRemoved;
				bRemoved = pMap->erase(temp); // also remove it from the map 
				wxASSERT(bRemoved != 0);
			}
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
            // user must have already deleted the <Not In KB> while in the KB editor, so
            // our work is done
			return;
		}
		wxASSERT(pRefString);
		if (pRefString != NULL)
		{
			CTargetUnit* pTgtUnit = pRefString->m_pTgtUnit;
			wxASSERT(pTgtUnit);
			TranslationsList* pList = pTgtUnit->m_pTranslations;
			wxASSERT(!pList->IsEmpty() && pList->GetCount() == 1);
			TranslationsList::Node* pos = pList->GetFirst();
			delete pRefString; // deletes the CRefString having the text "<Not In KB>"
			pRefString = (CRefString*)NULL;
			pList->DeleteNode(pos);

            // we must also delete the target unit, since we are setting up a situation
            // where in effect the current matched item was never previously matched, ie.
            // it's a big error to have a target unit with no reference string in it.
			int index = pSrcPhrase->m_nSrcWords - 1;
			MapKeyStringToTgtUnit* pMap = m_pMap[index];
			int bRemoved;
			// handle auto-caps tweaking, if it is on
			wxString temp = pSrcPhrase->m_key;
			if (gbAutoCaps && !gbNoSourceCaseEquivalents)
			{
                // auto caps is on and there are case equivalences defined, so check if we
                // must convert the key to lower case for the removal operation (so that it
                // succeeds, since the KB would have a lower case key stored, not upper
                // case
				bool bNoError = m_pApp->GetDocument()->SetCaseParameters(temp); // TRUE for default 
														 // parameter = src text
				if (bNoError && gbSourceIsUpperCase && (gcharSrcLC != _T('\0')))
				{
					temp.SetChar(0,gcharSrcLC); // make first char lower case
				}
			}
			bRemoved = pMap->erase(temp); // remove it from the map

			// now remove the CTargetUnit instance too
			TUList::Node* tupos;
			tupos = m_pTargetUnits->Find(pTgtUnit); // find position of 
												// pRefString's owning targetUnit
			pTgtUnit = (CTargetUnit*)tupos->GetData(); // get the targetUnit
														  // in the list
			wxASSERT(pTgtUnit != NULL);
			m_pTargetUnits->DeleteNode(tupos); // remove it from the list
			delete pTgtUnit; // delete it from the heap
			pTgtUnit = (CTargetUnit*)NULL;
		}
	}
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
	int len;
	int nIndexLast;
	if (!tgtPhrase.IsEmpty())
	{
		len = tgtPhrase.Length();
		nIndexLast = len-1;
		do {
			if (tgtPhrase.GetChar(nIndexLast) == _T(' '))
			{
                // wxString.Remove must have 1 otherwise the default is to truncate the
                // remainder of the string!
				tgtPhrase.Remove(nIndexLast,1);
				len = tgtPhrase.Length();
				nIndexLast = len -1;
			}
			else
			{
				break;
			}
		} while (len > 0 && nIndexLast > -1);
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
		if (gbRemovePunctuationFromGlosses)
			m_pApp->GetView()->RemovePunctuation(m_pApp->GetDocument(),&pSrcPhrase->m_gloss,from_target_text);
	}
	else
	{
		if (tgtPhrase != _T("<Not In KB>"))
		{
			pSrcPhrase->m_adaption = tgtPhrase;
			if (!gbInhibitLine4StrCall)
				m_pApp->GetView()->MakeLineFourString(pSrcPhrase, tgtPhrase); // set m_targetStr member too
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
		if (m_bGlossingKB)
			nMapIndex = 0;
		else
			nMapIndex = pSrcPhrase->m_nSrcWords - 1; // compute the index to the map
	}
	// if there is a CTargetUnit associated with the current key, then get it; if not,
	// create one and add it to the appropriate map

    // if we have too many source words, then we cannot save to the KB, so detect this and
    // warn the user that it will not be put in the KB, then return TRUE since all is
    // otherwise okay; this check need be done only when adapting
	if (!m_bGlossingKB && pSrcPhrase->m_nSrcWords > MAX_WORDS)
	{
		pSrcPhrase->m_bNotInKB = TRUE;
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
		pRefString = new CRefString(pTU);
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

		m_pTargetUnits->Append(pTU); // add the targetUnit to the KB
		if (m_bGlossingKB)
			pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
		else
			pSrcPhrase->m_bHasKBEntry = TRUE;

		(*m_pMap[nMapIndex])[key] = pTU;
		// update the maxWords limit
		if (m_bGlossingKB)
		{
			m_nMaxWords = 1; // always 1 when glossing (ensures glossing 
				// ignores maps with indices from 1 to 9; all is in 1st map only)
		}
		else
		{
			if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
				m_nMaxWords = pSrcPhrase->m_nSrcWords;
		}
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
			pRefString = new CRefString((CTargetUnit*)pTU);
			wxASSERT(pRefString != NULL);

			pRefString->m_refCount = 1; // set the count
			pRefString->m_translation = AutoCapsMakeStorageString(tgtPhrase,FALSE);
			pTU->m_pTranslations->Append(pRefString); // store in the CTargetUnit
			if (m_pApp->m_bForceAsk)
				pTU->m_bAlwaysAsk = TRUE; // turn it on if user wants to be given 
						// opportunity to add a new refString next time its matched

			m_pTargetUnits->Append(pTU); // add the targetUnit to the KB
			if (m_bGlossingKB)
				pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
			else
				pSrcPhrase->m_bHasKBEntry = TRUE;

			(*m_pMap[nMapIndex])[key] = pTU;// store the CTargetUnit in the map 
			// update the maxWords limit
			if (m_bGlossingKB)
			{
				m_nMaxWords = 1; // for glossing it is always 1
			}
			else
			{
				if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
					m_nMaxWords = pSrcPhrase->m_nSrcWords;
			}
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
					pRefStr->m_refCount++;
					if (m_bGlossingKB)
						pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
					else
						pSrcPhrase->m_bHasKBEntry = TRUE;
					delete pRefString; // don't need this one
					pRefString = (CRefString*)NULL;
					if (m_pApp->m_bForceAsk)
						pTU->m_bAlwaysAsk = TRUE; // nTrCount might be 1, so we must 
								// ensure it gets set if that is what the user wants
					break;
				}
			}
            // if we get here with bMatched == FALSE, then there was no match, so we must
            // add the new pRefString to the targetUnit
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
					delete pRefString; // don't leak memory
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
						delete pRefString; // don't leak the memory
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
// onward tests gbIsGlossing for storing to the appropriate KB, etc. For adaptation, on
// input the tgtPhrase parameter should have the text with punctuation removed, so this is
// typically done in the caller with a call to RemovePunctuation( ). For versions prior to
// 4.1.0, in order to support the user overriding the stored punctuation for the source
// text, a call to MakeLineFourString( ) is done in the caller, and then RemovePunctuation(
// ) is called in the caller, so a second call of MakeLineFourString( ) within StoreText( )
// is not required in this circumstance - in this case, a global boolean
// gbInhibitLine4StrCall is used to jump the call within StoreText( ). For 4.1.0 and later,
// MakeLineFourString() is not now called. See below.
// 
// Ammended, July 2003, for Auto-Capitalization support
// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 13May10, moved to here from CAdapt_ItView class, and removed pKB param from signature
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
	m_pApp->GetDocument()->Modify(TRUE);

    // BEW added 20Apr06, to store <Not In KB> when gbSuppressStoreForAltBackspaceKeypress
    // flag is TRUE - as wanted by Bob Eaton; we support this only in adapting mode, not
    // glossing mode
	if (!m_bGlossingKB && gbSuppressStoreForAltBackspaceKeypress)
	{
		wxString strNot = _T("<Not In KB>");
		// rest of this block's code is a simplification of code from later in StoreText()
		int nMapIndex;
		if (m_bGlossingKB)
			nMapIndex = 0; // always an index of zero when glossing
		else
			nMapIndex = pSrcPhrase->m_nSrcWords - 1; // index to the appropriate map

        // if we have too many source words, then we cannot save to the KB, so beep - this
        // is unlikely to be the case when Bob's modification is being used.
		if (!m_bGlossingKB && pSrcPhrase->m_nSrcWords > MAX_WORDS)
		{
			::wxBell(); //MessageBeep(0);
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
			pRefString = new CRefString(pTU); // the pTU argument sets the m_pTgtUnit member
			pRefString->m_refCount = 1; // set the count
			pRefString->m_translation = strNot;
			pTU->m_pTranslations->Append(pRefString); // store in the CTargetUnit
			m_pTargetUnits->Append(pTU); // add the targetUnit to the KB
			if (m_bGlossingKB)
			{
				pSrcPhrase->m_bHasGlossingKBEntry = TRUE; // glossing KB has to treat 
														  // <Not In KB> as a 'real' gloss
				(*m_pMap[nMapIndex])[key] = pTU; // store the CTargetUnit in the map
				m_nMaxWords = 1;
			}
			else
			{
				pSrcPhrase->m_bHasKBEntry = FALSE; // it's not a 'real' entry
				pSrcPhrase->m_bNotInKB = TRUE;
				pSrcPhrase->m_bBeginRetranslation = FALSE;
				pSrcPhrase->m_bEndRetranslation = FALSE;

				(*m_pMap[nMapIndex])[key] = pTU;
				if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
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
				m_pTargetUnits->Append(pTU); // add the targetUnit to the KB
				if (m_bGlossingKB)
				{
					pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
					(*m_pMap[nMapIndex])[key] = pTU;
					m_nMaxWords = 1;
				}
				else
				{
					pSrcPhrase->m_bHasKBEntry = FALSE;
					pSrcPhrase->m_bNotInKB = TRUE;
					(*m_pMap[nMapIndex])[key] = pTU;
					if (pSrcPhrase->m_nSrcWords > m_nMaxWords)
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
				pRefString->m_refCount = 1; // set the count, assuming this will
											// be stored (it may not be)
				pRefString->m_translation = strNot;

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
						if (m_bGlossingKB)
						{
							pSrcPhrase->m_bHasGlossingKBEntry = TRUE;
						}
						else
						{
							pSrcPhrase->m_bHasKBEntry = FALSE;
							pSrcPhrase->m_bNotInKB = TRUE;
							pSrcPhrase->m_bBeginRetranslation = FALSE;
							pSrcPhrase->m_bEndRetranslation = FALSE;
						}
						delete pRefString; // don't need this one
						break;
					}
				}
                // if we get here with bMatched == FALSE, then there was no match, so this
                // must somehow be a normal entry, so we don't add anything and just return
				return TRUE;
			}
		}
		return TRUE;
	}
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
			if (!gbInhibitLine4StrCall)
				m_pApp->GetView()->MakeLineFourString(pSrcPhrase, tgtPhrase); // set m_targetStr member too
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
    // choice) then return, without saving, after setting the source phrases m_bNotInKB
    // flag to TRUE (ignore this block when glossing)
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

    // if there is a CTargetUnit associated with the current key, then get it; if not,
    // create one and add it to the appropriate map; we start by computing which map we
    // need to store to
	int nMapIndex;
	if (m_bGlossingKB)
		nMapIndex = 0; // always an index of zero when glossing
	else
		nMapIndex = pSrcPhrase->m_nSrcWords - 1; // index to the appropriate map

    // if we have too many source words, then we cannot save to the KB, so detect this and
    // warn the user that it will not be put in the KB, then return TRUE since all is
    // otherwise okay (this will be handled as a retranslation, by default) The following
    // comment is for when glossing... Note: if the source phrase is part of a
    // retranslation, we allow updating of the m_gloss attribute, and we won't change any
    // of the retranslation supporting flags; so it is therefore possible for
    // m_bRetranslation to be TRUE, and also for m_bHasGlossingKBEntry to be TRUE.
	if (!m_bGlossingKB && pSrcPhrase->m_nSrcWords > MAX_WORDS)
	{
		pSrcPhrase->m_bNotInKB = FALSE;
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
		pRefString = new CRefString(pTU);
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

		m_pTargetUnits->Append(pTU); // add the targetUnit to the KB
		if (m_bGlossingKB)
		{
			pSrcPhrase->m_bHasGlossingKBEntry = TRUE; // tell the src phrase it has
													 // an entry in the glossing KB
			(*m_pMap[nMapIndex])[key] = pTU; // store the CTargetUnit in the map

			// update the maxWords limit - for glossing it is always set to 1
			m_nMaxWords = 1;
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
			// this is an error condition, targetUnits must NEVER have an 
			// empty m_translations list
			wxMessageBox(_T(
"Warning: the current storage operation has been skipped, and a bad storage element has been deleted."),
			_T(""), wxICON_EXCLAMATION);

			// fix the error
			TUList::Node* pos = m_pTargetUnits->Find(pTU); // find position of the
														   // bad targetUnit in the list
			// get the targetUnit in the list
			CTargetUnit* pTgtUnit = (CTargetUnit*)pos->GetData();
			wxASSERT(pTgtUnit != NULL && pTgtUnit->m_pTranslations->IsEmpty()); // have we
																	// found the bad one?
			m_pTargetUnits->DeleteNode(pos); // remove it from the list
			delete pTgtUnit; // delete it from the heap
			pTgtUnit = (CTargetUnit*)NULL;

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
			pRefString = new CRefString(pTU);
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

			m_pTargetUnits->Append(pTU); // add the targetUnit to the KB
			if (m_bGlossingKB)
			{
				pSrcPhrase->m_bHasGlossingKBEntry = TRUE;

				(*m_pMap[nMapIndex])[key] = pTU; // store the CTargetUnit in 
						// the map with appropr. index (key may have been made lc)
				// update the maxWords limit - for glossing it is always set to 1
				m_nMaxWords = 1;
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
													   // could be uc
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
					bMatched = TRUE;
					pRefStr->m_refCount++;
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
					delete pRefString; // don't need this one
					pRefString = (CRefString*)NULL;
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
					delete pRefString; // don't leak memory
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
							delete pRefString; // don't leak the memory
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
		int last = nTabLevel + 1;
		for (j = 0; j < last ; j++)
		{
			aStr += "\t"; // tab the start of the line
		}
		// construct the element
		aStr += "<RS n=\"";
		aStr += numStr;
		aStr += "\" a=\"";
		aStr += bstr;
		aStr += "\"/>\r\n";
	}

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
		int last = nTabLevel + 1;
		for (j = 0; j < last ; j++)
		{
			aStr += "\t"; // tab the start of the line
		}
		// construct the element
		aStr += "<RS n=\"";
		aStr += numStr;
		aStr += "\" a=\"";
		aStr += bstr;
		aStr += "\"/>\r\n";
	}

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
/// \param      bIsGlossingKB   -> TRUE if saving a glossing KB, otherwise FALSE 
///                                (the default) for a regular KB
/// \remarks
/// Called from: the App's StoreGlossingKB() and StoreKB().
/// Structures the KB data in XML form. Builds the XML file in a wxMemoryBuffer with sorted
/// TU elements and finally writes the buffer to an external file.
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
	if (m_bGlossingKB)
	{
		maxWords = 1; // GlossingKB uses only one map
	}
	else
	{
		maxWords = (int)MAX_WORDS;
	}

	
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
	intStr << m_pApp->GetDocument()->GetCurrentDocVersion(); // versionable schema number (see AdaptitConstants.h)
#ifdef _UNICODE
	numStr = m_pApp->Convert16to8(intStr);
#else
	numStr = intStr;
#endif
	
	aStr = "<";
	if (m_bGlossingKB)
	{
		aStr += xml_kb;
		aStr += " docVersion=\""; // docVersion 4
		aStr += numStr;
		aStr += "\" max=\"1";
	}
	else
	{
		aStr += xml_kb;
		aStr += " docVersion=\""; // docVersion 4
		aStr += numStr;
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



