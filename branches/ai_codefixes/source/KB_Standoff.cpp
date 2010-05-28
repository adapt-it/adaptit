/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KB_Standoff.cpp
/// \author			Bruce Waters
/// \date_created	20 May 2010
/// \date_revised	
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CKB_Standoff class.
/// The CKB_Standoff class partly defines the knowledge base class for the Adapt It data
/// storage structures; it is the in-memory class (a single instance only) which stores
/// metadata for the CKB in-memory class (which also is a single instance). (The app,
/// however, maintains one CKB class for adaptation mode, and one for glossing mode; hence
/// each of these will have a companion CKB_Standoff class instance to store metadata for
/// each knowledge base type.) The CKB class has an array of ten maps. However, since the
/// standoff does not have to have knowledge of how many source text words are involved,
/// the companion CKB_Standoff class has only one map which is a conflation for all ten of
/// the maps in CKB. The map in CKB_Standoff maps wxString keys to CTargetUnit_Standoff*
/// instances. (And the latter store a list of CRefString_Standoff instances.) The metadata
/// is datetime stamping, and a string specifying the source (IP address or host machine
/// name)of the adaptation or gloss when entered into the knowledge base.
/// \derivation		The CKB_Standoff class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KB_Standoff.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#include <wx/textfile.h>
#endif

#include <wx/hashmap.h>
#include <wx/progdlg.h> // for wxProgressDialog

#include "Adapt_It.h"
//#include "Adapt_ItView.h"
//#include "SourcePhrase.h"
#include "helpers.h"
//#include "Adapt_ItDoc.h"
#include "KB.h"
#include "AdaptitConstants.h" 
#include "TargetUnit.h"
#include "RefString.h"
//#include "LanguageCodesDlg.h"
#include "BString.h"
#include "XML.h"
#include "WaitDlg.h"
#include "KB_Standoff.h"
#include "TargetUnit_Standoff.h"
#include "CRefString_Standoff.h"


// Define type safe pointer lists
//#include "wx/listimpl.cpp"

IMPLEMENT_DYNAMIC_CLASS(CKB_Standoff, wxObject) 

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

// globals needed due to moving functions here from mainly the view class
// next group for auto-capitalization support
/*
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
*/
// next, miscellaneous needed ones
///extern bool gbNoAdaptationRemovalRequested;
//extern bool gbCallerIsRemoveButton;
//extern wxChar gSFescapechar;
//extern bool bSupportNoAdaptationButton;
//extern bool gbSuppressStoreForAltBackspaceKeypress;
//extern bool gbByCopyOnly;
//extern bool gbInhibitLine4StrCall;
//extern bool gbRemovePunctuationFromGlosses;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CKB_Standoff::CKB_Standoff()
{
	m_pApp = &wxGetApp();
	m_pMap_Standoff = new MapStrToTgtUnit_Standoff;
	wxASSERT(m_pMap_Standoff != NULL);
	m_bGlossingKB = FALSE; // default
	m_kbVersionCurrent = 2; // default
	m_pOwningKB = NULL;
}

// use this one everywhere
CKB_Standoff::CKB_Standoff(bool bGlossingKB, int kbVersion, CKB* pOwningKB)
{
	m_pApp = &wxGetApp();
	m_pMap_Standoff = new MapStrToTgtUnit_Standoff;
	wxASSERT(m_pMap_Standoff != NULL);
	m_kbVersionCurrent = kbVersion;
	m_pOwningKB = pOwningKB;
	m_bGlossingKB = bGlossingKB; // set the KB type, TRUE for GlossingKB, 
								 // FALSE for adapting KB
}

// copy constructor - check it works
CKB_Standoff::CKB_Standoff(const CKB_Standoff &kb)
{
	const CKB_Standoff* pCopy = &kb;
	m_bGlossingKB = pCopy->m_bGlossingKB; // BEW added 12May10
	m_pApp = pCopy->m_pApp; // BEW added 12May10
	m_kbVersionCurrent = pCopy->m_kbVersionCurrent; // BEW added 12May10
	m_pOwningKB = pCopy->m_pOwningKB; // if not the same owner, set it externally

	// now copy the map
	MapStrToTgtUnit_Standoff* pThisMap = m_pMap_Standoff;
	MapStrToTgtUnit_Standoff* pThatMap = pCopy->m_pMap_Standoff;
	MapStrToTgtUnit_Standoff::iterator iter;
	MapStrToTgtUnit_Standoff::iterator iter2;
	if (pThatMap->size() != 0)
	{
		for (iter = pThatMap->begin(); iter != pThatMap->end(); ++iter)
		{
			CTargetUnit_Standoff* pTU_StandoffCopy;
			wxString key; // an uuid
			key = iter->first;		
			pTU_StandoffCopy = iter->second;
			wxASSERT(!key.IsEmpty());
			wxASSERT(pTU_StandoffCopy != NULL);
			// use copy constructor
			CTargetUnit_Standoff* pNewTU_Standoff = new CTargetUnit_Standoff(*pTU_StandoffCopy);
			wxASSERT(pNewTU_Standoff != NULL);
			wxString keyCopy = key; // make a copy of the uuid key
			iter2 = pThisMap->find(keyCopy);
			if (iter2 != pThisMap->end())
			{
				// key exists in the map
				pThisMap->insert(*iter2); // overwrites value with pNewTU_Standoff value
			}
			else
			{
				// key not in the map
				(*pThisMap)[keyCopy] = pNewTU_Standoff; //  inserts default one in map then assigns RHS one
			}
		}
	}
}

CKB_Standoff::~CKB_Standoff()
{
	// WX note: this destructor is called when closing project, but not when closing doc
}

inline void CKB_Standoff::SetOwningKB(CKB* pOwningKB)
{
	m_pOwningKB = pOwningKB;
}

inline CKB* CKB_Standoff::GetOwningKB()
{
	return m_pOwningKB;
}

/*
void CKB_Standoff::Copy_Standoff(const CKB_Standoff& kb)
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
*/


