/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			TargetUnit.cpp
/// \author			Bill Martin
/// \date_created	12 February 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CTargetUnit class. 
/// The CTargetUnit class functions as a repository of translations. Each
/// CTargetUnit object stores all the known translations for a given
/// source text word or phrase.
/// \derivation		The CTargetUnit class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "TargetUnit.h"
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

// other includes
//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
//#include <wx/datstrm.h> // needed for wxDataOutputStream() and wxDataInputStream()

#include "Adapt_It.h"
#include "TargetUnit.h"
#include "AdaptitConstants.h"
#include "RefString.h"
#include "RefStringMetadata.h"
#include "helpers.h"

// Define type safe pointer lists
#include "wx/listimpl.cpp" // this should always be included before WX_DEFINE_LIST

/// This macro together with the macro list declaration in the .h file
/// complete the definition of a new safe pointer list class called TranslationsList.
WX_DEFINE_LIST(TranslationsList); // see WX_DECLARE_LIST macro in the .h file

// IMPLEMENT_CLASS(CTargetUnit, wxObject)
IMPLEMENT_DYNAMIC_CLASS(CTargetUnit, wxObject)

CTargetUnit::CTargetUnit()
{
	m_bAlwaysAsk = FALSE; // default
	m_pTranslations = new TranslationsList; // whm added TODO: delete in OnCloseDocument or EraseKB???
	wxASSERT(m_pTranslations != NULL);
}
// Bruce had the copy constructor commented out
// MFC NOTE: The copy constructor is fragile: see the note in the code below. The m_pTranslations attribute 
// cannot be guaranteed to have been defined before this constructor attempts to use it. The first 
// version failed, but by adding the this pointer explicitly, it not just compiled but also did not
// crash when running.But doing the same thing in the CKB copy constructor did not work - it compiled
// fine but crashed at run time. So I conclude that the programmer cannot reliably force the 
// definition of a list attribute to occur in a copy constructor before it needs to be used. So I 
// will need to use a function approach instead. I will leave this stuff here to document this C++ 
// bug. And can't get round it with an initialization list either.
//
// NOTE: whm. I wonder if any/all of the following changes would guarantee that the m_pTranslations
// attribute is defined before the copy constructor uses it:
// 1. Initialize the m_pTranslations list in the CTargetUnit::CTargetUnit() default constructor
//    by adding m_pTranslations->RemoveAll() in the MFC version or .Clear() in wxWidgets version.
// 2. Changing the order of the class members in the class declaration, i.e., moving the copy
//    constructor after the declaration of m_pTranslations. The C++ standard initializes class
//    members in the order in which they appear in the class declaration.
// 3. Override the default assignment operator for CTargetUnit. Every class that deals with pointers
//    really should have both a copy constructor override as well as an assignment operator override.
//    Hence, anytime we have an already instantiated CTargetUnit, and wish to clone it (i.e., do
//    an object-to-object assignment), we need an override of the assignment operator for the 
//    CTargetUnit class.

// copy constructor
// This normal copy constructor was commented out in the MFC version
CTargetUnit::CTargetUnit(const CTargetUnit &tu)
{
	wxASSERT(m_pTranslations);
	wxASSERT(m_pTranslations->GetCount() == 0);
	m_bAlwaysAsk = tu.m_bAlwaysAsk;
	if (tu.m_pTranslations->IsEmpty())
		return;
	// WX Note: Shouldn't this return for cases where this == &tu ???
	TranslationsList::Node* pos = tu.m_pTranslations->GetFirst();
	wxASSERT(pos != NULL);
	while (pos != NULL)
	{
		CRefString* pRefStr = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pRefStr != NULL);
		CRefString* pRefStrCopy = new CRefString(*pRefStr, this); // use copy constructor
		m_pTranslations->Append(pRefStrCopy); // wx referencing it without this->
		//this->m_pTranslations->Append((CTargetUnit*)pRefStrCopy); // MFC note: had to do it 
		// this way because without an explicit reference to this, the m_translations symbol 
		// was not found & we crash when the program runs, though it compiles okay with	
		// m_translations.AddTail(pRefStrCopy);
	}
}


// MFC Note: A work-around for the inability to define a copy constructor which 
// works (it separates the CTargetUnit creation, which is done externally, from 
// the copy syntax which is same as what would be used in the copy constructor).
// WX Note: TODO: If we get the normal copy constructor working, comment this one out
// and provide an overloaded assignment operator as well.
// WX Design Note: It is normally the case ("rule of the big three") that when any one 
// of (1) Destructor, (2) Copy constructor, or (3) Overloaded assignment operator, is 
// needed for a class, then it is most likely that all three should be provided by the 
// class. User-defined copy constructor functions can also be designed as two parameter 
// friend functions. See Dale & Teague pp. 343-344. 
void CTargetUnit::Copy(const CTargetUnit &tu)
{
	wxASSERT(this);
	wxASSERT(m_pTranslations);
	wxASSERT(m_pTranslations->GetCount() == 0);
	m_bAlwaysAsk = tu.m_bAlwaysAsk;
	if (tu.m_pTranslations->IsEmpty())
		return;
	// WX Note: Shouldn't this return for cases where this == &tu ???
	TranslationsList::Node *node = tu.m_pTranslations->GetFirst();
	while (node)
	{
		CRefString* pRefStr = (CRefString*)node->GetData(); // remember it has a CRefStringMetadata too
		node = node->GetNext();
		wxASSERT(pRefStr != NULL);
		CRefString* pRefStrCopy = new CRefString(*pRefStr, this); // use copy constructor
		wxASSERT(pRefStrCopy != NULL);
		// The CTargetUnit member m_pTranslations stores a list of CRefString instances
		m_pTranslations->Append(pRefStrCopy); 
	}
}

CTargetUnit::~CTargetUnit()
{
	delete m_pTranslations;
	m_pTranslations = (TranslationsList*)NULL;
}

// returns an index to a non-deleted CRefString instance whose m_translation member matches
// the passed in translationStr; otherwise returns wxNOT_FOUND if there was no match
// 
// BEW created 25Jun10, for support of kbVersion2, since a CTargetUnit may store one or
// more deleted CRefString instances, we cannot rely on an index value from a selection in
// a list box in a dialog in order to access the CRefString which corresponds to the
// selection. So we loop over them all, ignoring deleted ones, and checking for a match
// within the others.
int CTargetUnit::FindRefString(wxString& translationStr)
{
	CRefString* pRefString = NULL;
	int anIndex = -1;
	wxString emptyStr; emptyStr.Empty();
	TranslationsList::Node* pos = m_pTranslations->GetFirst();
	wxASSERT(pos != NULL);
	while (pos != NULL)
	{
		anIndex++;
		pRefString = (CRefString*)pos->GetData();
		wxASSERT(pRefString != NULL);
		pos = pos->GetNext();
		if (!pRefString->GetDeletedFlag())
		{
			wxString str = pRefString->m_translation;
			if ( (translationStr.IsEmpty() && str.IsEmpty()) || (translationStr == str))
			{
				// we have a match
				return anIndex;
			}
		} // end of block for test: m_bDeleted == FALSE
	}
	// if control gets to here, we have no match
	return (int)wxNOT_FOUND;
}

// Checks the CRefString instances, and any with m_bDeleted cleared to FALSE, it sets it
// TRUE and sets the m_deletedDateTime value to the current datetime. If one of the
// undeleted instances stores "<Not In KB>" already, it too is made deleted, because this
// function is called before a StoreText(), and if the store then stores <Not In KB>, the
// deleted CRefString storing that is undeleted - so we don't have to take into account
// whether or not a <Not In KB> CRefString is within the list or not, we just make all
// instances be deleted.
// This function is called in the KB.cpp's DoNotInKB() called from the view's
// OnCheckKBSave() which handles the Save to knowledge base checkbox clicks, and in the
// doc's DoConsistencyCheck() to help fix "<Not In KB> inconsistencies when found
void CTargetUnit::DeleteAllToPrepareForNotInKB()
{
	TranslationsList* pList = m_pTranslations;
	if (!pList->IsEmpty())
	{
		TranslationsList::Node* pos = pList->GetFirst();
		while (pos != NULL)
		{
			CRefString* pRefString = (CRefString*)pos->GetData();
			pos = pos->GetNext();
			if (pRefString != NULL)
			{
				if (!pRefString->m_bDeleted)
				{
					pRefString->m_bDeleted = TRUE;
					pRefString->m_pRefStringMetadata->m_deletedDateTime = GetDateTimeNow();
				}
			}
		} // end of loop
	}
}

// pass in a modification choice as the modChoice param; allows values are LeaveUnchanged
// (which is the default if no param is supplied), or SetNewValue -- the latter choice
// will overwrite any earlier modification datetime value which may have been stored
// earlier 
// Currently this function is only used in the transformation process which converts
// adaptations in one project to glosses in the new (empty) project -- see the Transform
// Adaptations Into Glosses... command; and since adaptations are becoming glosses is is
// reasonable to mark the time at which that transformation took place. The copy
// constructor used for the transformations will copy all the CRefString and
// CRefStringMetadata contents unchanged, and since EraseDeletions() is called after that
// with modChoice SetNewValue, the modification datetime values are set to the datetime at
// which each new CRefString instance in the glossing KB is created in the copy process.
void CTargetUnit::EraseDeletions(enum ModifiedAction modChoice)
{
	CRefString* pRefString = NULL;
	TranslationsList::Node* pos = m_pTranslations->GetFirst();
	TranslationsList::Node* savepos = NULL;
	wxASSERT(pos != NULL);
	while (pos != NULL)
	{
		pRefString = pos->GetData();
		wxASSERT(pRefString != NULL);
		savepos = pos; // in case we need to delete this node
		pos = pos->GetNext();
		if (pRefString->GetDeletedFlag())
		{
			pRefString->DeleteRefString();
			m_pTranslations->DeleteNode(savepos);
		}
		else
		{
			if (modChoice == SetNewValue)
			{
				pRefString->m_pRefStringMetadata->m_modifiedDateTime = GetDateTimeNow();
			}
		}
	}
}

// counts the number of CRefString instances stored in this CTargetUnit instance,
// but counting only those for which m_bDeleted is FALSE;
int CTargetUnit::CountNonDeletedRefStringInstances()
{
	if (m_pTranslations->IsEmpty())
	{
		return 0;
	}
	int counter = 0;
	TranslationsList::Node* tpos = m_pTranslations->GetFirst();
	CRefString* pRefStr = NULL;
	while (tpos != NULL)
	{
		pRefStr = (CRefString*)tpos->GetData();
		wxASSERT(pRefStr != NULL);
		tpos = tpos->GetNext();
		if (!pRefStr->m_bDeleted)
		{
			counter++;
		}
	}
	return counter;
}

// BEW 7Jun10, added, as the legacy code in destructor was inadequate for all contexts (it
// didn't work for the xml LIFT parser for example)
//void CTargetUnit::DeleteTargetUnit(CTargetUnit* pTU)
void CTargetUnit::DeleteTargetUnitContents()
{
	TranslationsList::Node* tnode = NULL;
	//if (pTU->m_pTranslations->GetCount() > 0)
	if (m_pTranslations->GetCount() > 0)
	{
		//for (tnode = pTU->m_pTranslations->GetFirst(); tnode; tnode = tnode->GetNext())
		for (tnode = m_pTranslations->GetFirst(); tnode; tnode = tnode->GetNext()) 		
		{
			CRefString* pRefStr = (CRefString*)tnode->GetData();
			if (pRefStr != NULL)
			{
				pRefStr->DeleteRefString(); // deletes the CRefStringMetadata too
				pRefStr = (CRefString*)NULL;
			}
		}
	}
	m_pTranslations->clear(); // leave ~CTargetUnit() to delete the list object from heap
}
