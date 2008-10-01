/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			KB.cpp
/// \author			Bill Martin
/// \date_created	21 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
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
#include "KB.h"
#include "AdaptitConstants.h" 
#include "TargetUnit.h"

// Define type safe pointer lists
#include "wx/listimpl.cpp"

/// This macro together with the macro list declaration in the .h file
/// complete the definition of a new safe pointer list class called TUList.
WX_DEFINE_LIST(TUList);	// see WX_DECLARE_LIST in the .h file

IMPLEMENT_DYNAMIC_CLASS(CKB, wxObject) 

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CKB::CKB()
{
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

/*
// Implementations of LoadObject and SaveObject below take the place of this 
// MFC Serialize function
void CKB::Serialize(CArchive& ar)
{
	CObject::Serialize(ar);	// serialize base class

	m_pTargetUnits->Serialize(ar);

	for (int i=0; i<MAX_WORDS; i++)
	{
		m_pMap[i]->Serialize(ar);
	}

	if(ar.IsStoring())
	{
		ar << m_sourceLanguageName;
		ar << m_targetLanguageName;

		ar << (DWORD)m_nMaxWords;

	}
	else
	{
		ar >> m_sourceLanguageName;
		ar >> m_targetLanguageName;

		DWORD dd;
		ar >> dd;
		m_nMaxWords = (int)dd;

	}
}
*/
/*
wxOutputStream &CKB::SaveObject(wxOutputStream& stream)
{
	wxDataOutputStream ar( stream );

	// Note on CKB Serialization in wxWidgets:
	//In the wxWidgets version there is no built-in serialization
	//capability for the collection classes wxList and wxHashMap, so
	//we will need to provide our own way of maintaining persistence
	//in our association or mapping of source text words and phrases 
	//with our list of CTargetUnit objects. We will do it in a way
	//that does not serialize the list of CTargetUnits separately from
	//our map of source string (key) - CTargetUnit* pointer pairs 
	//stored in our MapKeyStringToTgtUnit hash map. Rather than serializing
	//them separately, we'll serialize the data more directly by 
	//serializing each key followed directly by the data members of
	//the CTargetUnit associated with that key. Then, when serializing
	//the data back in, we can use LoadObject to create the appropriate
	//objects, and rebuild our map of source string (key) and TU object
	//pointer associations, and our list of TUs. In essence then, our
	//map associations are being preserved by embedding of object data
	//in the stream, rather than by a generalized scheme of object pointer 
	//creation and tracking by means of "manifest constants" and 
	//persistent identifiers, or PIDs assigned to every unique object 
	//and every unique class name that is saved in the context of the 
	//archive. The MFC solution is generalized in the sense that it 
	//allows the programmer to store the objects and collections
	//of objects in almost any order in an archive, whereas our method
	//makes the ordering of objects within the archive reflect the
	//actual programatic associations between our objects in the case
	//of the collection classes.

	// The MFC "IsStoring" has the following basic code for CKB:
	//CObject::Serialize(ar);	// serialize base class
	//m_pTargetUnits->Serialize(ar);
	//for (int i=0; i<MAX_WORDS; i++)
	//{
	//	m_pMap[i]->Serialize(ar);
	//}
	//if(ar.IsStoring())
	//{
	//	ar << m_sourceLanguageName;
	//	ar << m_targetLanguageName;
	//	ar << (DWORD)m_nMaxWords;
	//}

	// For wxWidgets we won't serialize the TUList separately, but
	// rather we'll serialize each of the m_pMap[i] hash maps
	// embedding the associated m_pTargetUnit's object data as we go.

	wxString keyStr;
	for (int i=0; i<MAX_WORDS; i++)
	{
		// call SaveObject on each key/value pair in m_pMap[i]
		MapKeyStringToTgtUnit::iterator iter;
		wxInt32 count = m_pMap[i]->size();
		ar << count; // write out the number of items in this map
		for (iter = m_pMap[i]->begin(); iter != m_pMap[i]->end(); ++iter)
		{
			keyStr = iter->first; // get the key string iter->first is the key string
			ar << keyStr; // serialize out the key string

			// iter->second holds a pointer to the associated CTargetUnit in
			// the TUList. We won't serialize out the pointer, but instead we
			// will serialize out to the stream the unique CTargetUnit that is 
			// associated with the keyStr.
			CTargetUnit* pAssocTU = iter->second;
			//TUList::Node* node = m_pTargetUnits->Find(keyStr);
			//wxASSERT(node != NULL); // this must be true
			//CTargetUnit* pTU = (CTargetUnit*)node->GetData();
			// pTU now points to the CTargetUnit we want to serialize in
			// association with keyStr, so call CTargetUnit's SaveObject()
			// on it
			pAssocTU->SaveObject(stream);
			// Note: When the data is read back in with LoadObject, we will
			// then reassociate the newly created CTargetUnit in the map and
			// add it back to our TUList.
		}
	}

	// Serialize our CKB's other members
	ar << m_sourceLanguageName;
	ar << m_targetLanguageName;
	ar.Write32(m_nMaxWords); //ar << wxUint32(m_nMaxWords); // MFC uses DWORD

	// wxWidgets Notes: 
	// 1. Stream errors should be dealt with in the caller of CKB::SaveObject()
	//    which would be App's StoreKB(), StoreGlossingKB(), and 
	//    AccessOtherAdaptionProject().
	// 2. Streams automatically close their file descriptors when they
	//    go out of scope. 
	return stream;
}

wxInputStream &CKB::LoadObject(wxInputStream& stream)
{
	wxDataInputStream ar( stream );

	// (see note on SaveObject() above)

	// When MFC version of Serialize is loading rather
	// that storing, it has the following basic code for CKB:
	//CObject::Serialize(ar);	// serialize base class
	//m_pTargetUnits->Serialize(ar);
	//for (int i=0; i<MAX_WORDS; i++)
	//{
	//	m_pMap[i]->Serialize(ar);
	//}
	//if(ar.IsStoring())
	//{
	//	...
	//}
	//else
	//{
	//	ar >> m_sourceLanguageName;
	//	ar >> m_targetLanguageName;
	//	DWORD dd;
	//	ar >> dd;
	//	m_nMaxWords = (int)dd;
	//}

	// To achieve this loading functionality in wxWidgets we do the 
	// complement of what SaveObject() does above:
	// 1. Initialize the TUList to empty.
	// 2. Iterate through each of the 10 maps. For each map[i]:
	// 3. Clear the given map[i] starting empty for each iteration
	// 4. Read the wxInt32 count from the archive to know how many 
	//    mapped associations to expect for the given map. Use that
	//    count to set up a for loop to iterate count times. Each loop:
	// 5. Read the inStr key string from the archive.
	// 6. Create a new CTargetUnit, and call the CTargetUnit->LoadObject()
	//    method to load the object's data from the archive.
	// 7. Add the newly created CTargetUnit to CKB's TUList.
	// 8. Add the inStr (key), and the newly created CTargetUnit* (pointer)
	//    to the CKB's MapKeyStringToTgtUnit hash map.

	// Insure we're starting with empty TUList
	// Note: Originally I had this TUList initialization embedded within  
	// the for loop below. That was, of course, and error since the TUList
	// was then being emptied at the beginning of each of the 10 map
	// serializaions, thus effectively leaving the TUList empty (except 
	// for the rare case when map[9] would have some 10-word source phrases 
	// mapped).
	int tucount = m_pTargetUnits->GetCount();
	if (tucount > 0)
		m_pTargetUnits->Clear();

	wxString keyStr;
	for (int i=0; i<MAX_WORDS; i++)
	{
		// Insure we're starting with empty m_pMap[i]
		//int tucount = m_pTargetUnits->GetCount();
		//if (tucount > 0)
		//	m_pTargetUnits->Clear();
		int mcount = m_pMap[i]->size();
		if ( mcount > 0)
			m_pMap[i]->clear();

		// Read the wxInt32 nMapSize value so we'll know how many key/value 
		// pairs are to be stored in the map.
		wxInt32 nMapSize;
		ar >> nMapSize;

		// Use nMapSize to construct a for loop to read in the key/value
		// pairs.
		for (int ct = 0; ct < nMapSize; ct++)
		{
			// Read the key string stored in the stream
			ar >> keyStr;
			// Create a new CTargetUnit instance
			CTargetUnit* pData = new CTargetUnit;
			wxASSERT(pData != NULL);
			// Load the CTargetUnit's object data from the archive
			pData->LoadObject(stream);
			// Add the newly created CTargetUnit to CKB's TUList
			m_pTargetUnits->Append(pData);	// add the new CTargetUnit instance 
			// enter the key/value pair into the map
			(*m_pMap[i])[keyStr] = (CTargetUnit*)pData;
		}
	}

	// Serialize in the remaining CKB members
	ar >> m_sourceLanguageName;
	ar >> m_targetLanguageName;

	wxUint32 dd;
	ar >> dd;
	m_nMaxWords = (int)dd;

	return stream;
}
*/
