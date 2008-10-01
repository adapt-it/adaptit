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
// Pending Implementation Items in TargetUnit.cpp (in order of importance): (search for "TODO")
// 1. Test to insure the wxWidgets version copy constructor works OK. [tested OK]
// 2. Design an assignment = operator function for the class.
//
// Unanswered questions: (search for "???")
// 1. Testing the wxWidgets implementation
// 
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
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/datstrm.h> // needed for wxDataOutputStream() and wxDataInputStream()

#include "Adapt_It.h"
#include "TargetUnit.h"
#include "AdaptitConstants.h"
#include "RefString.h"

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
		CRefString* pRefStr = (CRefString*)node->GetData();
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

/*
// MFC's Serialize is replaced by LoadObject() and SaveObject() below
void CTargetUnit::Serialize(CArchive& ar)
{
	CObject::Serialize(ar);	// serialize base class

	m_translations.Serialize(ar);

	if(ar.IsStoring())
	{
		ar <<  (WORD)m_bAlwaysAsk;
	}
	else
	{
		WORD w;
		ar >> w;
		m_bAlwaysAsk = (bool)w;
	}
}
*/

/*
wxOutputStream &CTargetUnit::SaveObject(wxOutputStream& stream)
{
	wxDataOutputStream ar( stream );

	//m_translations.Serialize(ar);
	// To achieve the Serialize IsStoring() function here:
	// 1. Use m_pTranslations.GetCount() and archive it out as wxInt32.
	// 2. Iterate through the m_pTranslations list GetCount() times and 
	// call the CRefString::SaveObject for each element in the list
	wxInt32 count = m_pTranslations->GetCount();
	ar << count;

	for (TranslationsList::Node* node = m_pTranslations->GetFirst(); node; node = node->GetNext())
	{
		CRefString* pData = (CRefString*)node->GetData();
		pData->SaveObject(stream);
	}

	ar.Write16(m_bAlwaysAsk); //ar <<  wxUint16(m_bAlwaysAsk); // MFC uses WORD

	// wxWidgets Notes: 
	// 1. Stream errors should be dealt with in the caller of Adapt_ItDoc::SaveObject()
	//    which would be either DoFileSave(), or DoTransformedDocFileSave().
	// 2. Streams automatically close their file descriptors when they
	//    go out of scope. 
	return stream;
}

wxInputStream &CTargetUnit::LoadObject(wxInputStream& stream)
{
	wxDataInputStream ar( stream );

	//m_translations.Serialize(ar);
	// To achieve the Serialize !IsStoring() function here:
	// 1. Read the wxInt32 count from the archive to know how many CRefString 
	//    objects to expect.
	// 2. Use the int value read in 1. above in a for loop iterating count times, 
	//    creating new CRefString objects, and using the CRefString::LoadObject 
	//    method to assign values to its members, and Appending them to the
	//    CTargetString::TranslationsList.
	
	wxInt32 count;
	ar >> count;
	// insure we're starting with an empty list
	if (m_pTranslations->GetCount() > 0)
		m_pTranslations->Clear(); // previously had DeleteContents(TRUE); // check this !!!
	// for each expected CRefString, create a new instance and call
	for (int ct = 0; ct < count; ct++)
	{
		CRefString* pData = new CRefString;	// create a new instance of CRefString
		wxASSERT(pData != NULL);
		pData->LoadObject(stream);			// Load the instance's data

		// The pData refstring's m_pTgtUnit member is an old pointer that, in the
		// MFC version, was serialized out with MFC's mirrors and magic. However,
		// in our wx version it no longer points to a valid CTargetUnit. Therefore 
		// we need to make sure it points to its current owning CTargetUnit 
		// (which should be 'this').
		pData->m_pTgtUnit = this; // whm added for wxWidgets serialization. 

		m_pTranslations->Append(pData);	// add the new CRefString instance 
														// to the CTargetUnit's TranslationsList
	}

	wxUint16 w; // MFC uses WORD
	ar >> w;
	if (w == 0)
		m_bAlwaysAsk = FALSE;
	else
		m_bAlwaysAsk = TRUE;

	return stream;
}
*/

