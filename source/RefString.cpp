/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			RefString.cpp
/// \author			Bill Martin
/// \date_created	21 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the CRefString class. 
/// The CRefString class stores the target text adaptation typed
/// by the user for a given source word or phrase. It also keeps
/// track of the number of times this translation was previously
/// chosen.
/// \derivation		The CRefString class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in RefString.cpp (in order of importance): (search for "TODO")
// 1. NONE.
//
// Unanswered questions: (search for "???")
// 1. Testing the wxWidgets implementation
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "RefString.h"
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
#include "RefString.h"
#include "AdaptitConstants.h" 
#include "TargetUnit.h"

IMPLEMENT_DYNAMIC_CLASS(CRefString, wxObject)

CRefString::CRefString()
{
	m_refCount = 0;
	m_translation = _T("");
}

// normal constructor, with a pointer to its owning CTargetUnit instance
CRefString::CRefString(CTargetUnit* pTargetUnit)
{
	m_refCount = 0;
	m_translation = _T("");
	m_pTgtUnit = pTargetUnit;
}

// copy constructor, where the target unit instance must NEVER be copied as a pointer
// but supplied by the caller (different from the one in the source CRefString), or NULL
CRefString::CRefString(const CRefString &rs, CTargetUnit* pTargetUnit)
{
	m_pTgtUnit = pTargetUnit;
	m_refCount = rs.m_refCount;
	m_translation = rs.m_translation;
}


CRefString::~CRefString()
{

}

/*
// MFC's Serialize() is replaced by LoadObject() and SaveObject() below
void CRefString::Serialize(CArchive& ar)
{
	CObject::Serialize(ar);	// serialize base class

	if(ar.IsStoring())
	{
		ar << (DWORD)m_refCount;
		ar << m_translation;
		ar << m_pTgtUnit;	// m_pTgtUnit is a pointer pointing back to the 
							// TargetUnit to which the current CRefString belongs
	}
	else
	{
		DWORD dd;
		ar >> dd;
		m_refCount = (int)dd;
		ar >> m_translation;
		ar >> (CTargetUnit*)m_pTgtUnit;
	}
}
*/

/*
wxOutputStream &CRefString::SaveObject(wxOutputStream& stream)
{
	wxDataOutputStream ar( stream );

	ar.Write32(m_refCount); //ar << wxUint32(m_refCount);
	ar << m_translation; // wxString
	// Following MFC code we'll output the m_pTgtUnit pointer even though it is 
	// not going to be used for anything 
	ar.Write32((wxUint32)m_pTgtUnit); //ar << wxUint32(m_pTgtUnit);

	// wxWidgets Notes: 
	// 1. Stream errors should be dealt with in the caller of Adapt_ItDoc::SaveObject()
	//    which would be either DoFileSave(), or DoTransformedDocFileSave().
	// 2. Streams automatically close their file descriptors when they
	//    go out of scope. 
	return stream;
}

wxInputStream &CRefString::LoadObject(wxInputStream& stream)
{
	wxDataInputStream ar( stream );

	wxUint32 dd;
	ar >> dd;
	m_refCount = (int)dd;
	ar >> m_translation;
	// We'll input the m_pTgtUnit pointer (see SaveObject above) even though in the wx 
	// version it's not going to point to a valid target unit. The caller, which is
	// CTargetUnit::LoadObject(), must insure that the m_pTgtUnit member of this 
	// CRefString object is assigned to point to that current target unit. This should
	// be accomplished by assigning m_pTgtUnit the pointer value of CTargetUnit's 'this' 
	// member.
	ar >> dd;
	m_pTgtUnit = (CTargetUnit*)dd;	// temporary - m_pTgtUnit will be reassigned to 
									// point to a valid CTargetUnit back in 
									// CTargetUnit::LoadObject().

	return stream;
}
*/

// overloaded equality operator
bool CRefString::operator==(const CRefString& rs)
{
	if (rs.m_translation.IsEmpty() && m_translation.IsEmpty())
		return TRUE; // ensure two empty strings constitutes a TRUE value for the test
	return m_translation == rs.m_translation;
}

