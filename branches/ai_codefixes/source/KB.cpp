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

#include "Adapt_It.h"
#include "Adapt_ItView.h"
#include "SourcePhrase.h"
#include "KB.h"
#include "AdaptitConstants.h" 
#include "TargetUnit.h"
#include "RefString.h"

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
// next, for ???

extern bool gbNoAdaptationRemovalRequested;


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
}

// copy constructor - it doesn't work, see header file for explanation
/*
CKB::CKB(const CKB &kb)
{
	const CKB* pCopy = &kb;
	POSITION pos;

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

	if (gbIsGlossing)
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
CRefString* CKB::AutoCapsFindRefString(CTargetUnit* pTgtUnit,wxString adaptation)
{
	CRefString* pRefStr = (CRefString*)NULL;
	TranslationsList* pList = pTgtUnit->m_pTranslations;
	wxASSERT(pList);
	bool bNoError;
	if (gbAutoCaps && gbSourceIsUpperCase && !gbMatchedKB_UCentry && !adaptation.IsEmpty())
	{
		// possibly we may need to change the case of first character of 'adaptation'
		bNoError = m_pApp->GetView()->SetCaseParameters(adaptation, FALSE); // FALSE means it is an
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

// looks up the knowledge base to find if there is an entry in the map with index
// nSrcWords-1, for the key keyStr and then searches the list in the CTargetUnit for the
// CRefString with m_translation member identical to valueStr, and returns a pointer to
// that CRefString instance. If it fails, it returns a null pointer. 
// (Note: Jan 27 2001 changed so that returns the pRefString for a <Not In KB> entry). For
// version 2.0 and later, pKB will point to the glossing KB when gbIsGlossing is TRUE.
// Ammended, July 2003, to support auto capitalization
// BEW 11May10, moved from CAdapt_ItView class
CRefString* CKB::GetRefString(CKB *pKB, int nSrcWords, wxString keyStr, wxString valueStr)
{
	MapKeyStringToTgtUnit* pMap = pKB->m_pMap[nSrcWords-1];
	wxASSERT(pMap != NULL);
	CTargetUnit* pTgtUnit;	// wx version changed 2nd param of AutoCapsLookup() below to
							// directly use CTargetUnit* pTgtUnit
	CRefString* pRefStr;
	bool bOK = m_pApp->GetView()->AutoCapsLookup(pMap,pTgtUnit,keyStr);
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
/// object)from a CTargetUnit instance in the knowledge base.
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
void CKB::RemoveRefString(CRefString *pRefString, CSourcePhrase* pSrcPhrase, int nWordsInPhrase)
{
	if (gbIsGlossing)
		pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
	else
		pSrcPhrase->m_bHasKBEntry = FALSE;
	if (!gbIsGlossing && pSrcPhrase->m_bNotInKB)
	{
		// version 2 can do this block only when the adaption KB is in use
		pSrcPhrase->m_bHasKBEntry = FALSE;
		return; // return because nothing was put in KB for this source phrase anyway
	}
	
	//CAdapt_ItApp* pApp = GetDocument()->GetApp();

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
		bNoError = m_pApp->GetView()->SetCaseParameters(s1);
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
		if (gbIsGlossing)
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
			CKB* pKB;
			if (gbIsGlossing)
				pKB = m_pApp->m_pGlossingKB; // point to the glossing KB when glossing is on
			else
				pKB = m_pApp->m_pKB; // point to the adaptation knowledge base when adapting

			delete pRefString;
			pRefString = (CRefString*)NULL;
			// since we delete pRefString, TranslationsList::Clear() should do the job below
			pTU->m_pTranslations->Clear();

			TUList::Node* pos;

			pos = pKB->m_pTargetUnits->Find(pTU); // find position of pRefString's
												  // owning targetUnit

            // Note: A check for NULL should probably be done here anyway even if when
            // working properly a NULL return value on Find shouldn't happen.
			pTgtUnit = (CTargetUnit*)pos->GetData(); // get the targetUnit in
																	// the list
			wxASSERT(pTgtUnit != NULL);
			pKB->m_pTargetUnits->DeleteNode(pos); // remove its pointer from the list
			delete pTgtUnit; // delete its instance from the heap
			pTgtUnit = (CTargetUnit*)NULL;

			MapKeyStringToTgtUnit* pMap = pKB->m_pMap[nWordsInPhrase - 1];
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
		if (gbIsGlossing)
			pSrcPhrase->m_bHasGlossingKBEntry = FALSE;
		else
			pSrcPhrase->m_bHasKBEntry = FALSE;
	}
}

// GetKB() here is private member function of CKB() c.f. the view's public GetKB() which
// internally uses global bool gbIsGlossing to find which CKB instance is current
CKB* CKB::GetKB(bool bIsGlossing)
{
	if (bIsGlossing)
		return m_pApp->m_pGlossingKB;
	else
		return m_pApp->m_pKB;
}

// BEW created 11May10, to replace about 20 or so lines which always call GetRefString()
// and then a little later, RemoveRefString; this will enable about 600 lines of code
// spread over about 30 functions to be replaced by about 30 calls of this function.
// The bIsGlossing param is explicit as the caller will pass in global bool gbIsGlossing,
// but eventually when gbIsGlossing is removed from the app, we may adjust the signature,
// depending on how we later decide to handle the indication of adapting versus glossing 
// states. The bUsePhraseBoxContents is default TRUE if omitted. When TRUE, the
// targetPhrase value which is looked up is assumed to be the phrase box's contents (the
// caller is responsible to make sure that is so), but if FALSE is passed explicitly, then
// the targetPhrase param is ignored, and the m_gloss or m_adaption member of the passed
// in pSrcPhrase is used for lookup 
void CKB::GetAndRemoveRefString(bool bIsGlossing, CSourcePhrase* pSrcPhrase, 
								wxString& targetPhrase, bool bUsePhraseBoxContents)
{
	CRefString* pRefStr = NULL;
	// get the CKB instance (currently, the app class has that knowledge, using
	// gbIsGlossing global boolean, which is passed in here via the bIsGlossing param)
	CKB* pKB = GetKB(bIsGlossing); // pKB now points to either m_pKB, or m_pGlossingKB
	if (bIsGlossing)
	{
		if (bUsePhraseBoxContents)
		{
			pRefStr = GetRefString(pKB, 1, pSrcPhrase->m_key, targetPhrase);
		}
		else
		{
			pRefStr = GetRefString(pKB, 1, pSrcPhrase->m_key, pSrcPhrase->m_gloss);
		}
		if (pRefStr == NULL && pSrcPhrase->m_bHasGlossingKBEntry)
			pSrcPhrase->m_bHasGlossingKBEntry = FALSE; // must be FALSE for a successful lookup on return
		if (pRefStr != NULL)
			RemoveRefString(pRefStr, pSrcPhrase, 1);
	}
	else
	{
		if (bUsePhraseBoxContents)
		{
			pRefStr = GetRefString(pKB, pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key, targetPhrase);
		}
		else
		{
			pRefStr = GetRefString(pKB, pSrcPhrase->m_nSrcWords, pSrcPhrase->m_key, pSrcPhrase->m_adaption);
		}
		if (pRefStr == NULL && pSrcPhrase->m_bHasKBEntry)
			pSrcPhrase->m_bHasKBEntry = FALSE; // must be FALSE for a successful lookup on return
		if (pRefStr != NULL)
			RemoveRefString(pRefStr, pSrcPhrase, pSrcPhrase->m_nSrcWords);
	}
}




