/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			TargetUnit_Standoff.h
/// \author			Bruce Waters
/// \date_created	20 May 2010
/// \date_revised	
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CTargetUnit_Standoff class. 
/// The CTargetUnit_Standoff class functions as the in-memory storage class for metadata
/// associated with a unique CTargetUnit instance in memory. The two are linked by sharing
/// the same uuid value. Each CTargetUnit_Standoff object stores the linking uuid, the
/// wxString containing the source text word or phrase which is also stored on the
/// matching CTargetUnit instance, and datetime plus 'who created' information for each of
/// the stored CRefString_Standoff instances in the list within the CTargetUnit_Standoff
/// instance.
/// \derivation		The CTargetUnit class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "TargetUnit_Standoff.h"
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
#include "TargetUnit_Standoff.h"
#include "AdaptitConstants.h"
#include "RefString.h"
#include "CRefString_Standoff.h"

// Define type safe pointer lists
#include "wx/listimpl.cpp" // this should always be included before WX_DEFINE_LIST

/// This macro together with the macro list declaration in the .h file
/// complete the definition of a new safe pointer list class called RefStrMetadataList.
WX_DEFINE_LIST(RefStrMetadataList); // see WX_DECLARE_LIST macro in the .h file

IMPLEMENT_DYNAMIC_CLASS(CTargetUnit_Standoff, wxObject)

CTargetUnit_Standoff::CTargetUnit_Standoff()
{
	m_pRefStrMetadataList = new RefStrMetadataList;
	wxASSERT(m_pRefStrMetadataList != NULL);
	m_uuid = _T("");
	m_fallbackKey = _T("");
}

CTargetUnit_Standoff::CTargetUnit_Standoff(wxString sourceKey)
{
	m_pRefStrMetadataList = new RefStrMetadataList;
	wxASSERT(m_pRefStrMetadataList != NULL);
	m_uuid = _T("");
	m_fallbackKey = sourceKey;
}

// copy constructor
CTargetUnit_Standoff::CTargetUnit_Standoff(const CTargetUnit_Standoff &tu)
{
	wxASSERT(m_pRefStrMetadataList);
	wxASSERT(m_pRefStrMetadataList->GetCount() == 0);
	if (tu.m_pRefStrMetadataList->IsEmpty())
		return;
	if (this == &tu)
		return;
	RefStrMetadataList::Node* pos = tu.m_pRefStrMetadataList->GetFirst();
	wxASSERT(pos != NULL);
	while (pos != NULL)
	{
		CRefString_Standoff* pRefStr_S = (CRefString_Standoff*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pRefStr_S != NULL);
		CRefString_Standoff* pRefStr_StandoffCopy = new CRefString_Standoff(*pRefStr_S, this); // use copy constructor
		m_pRefStrMetadataList->Append(pRefStr_StandoffCopy); 
	}
	m_uuid = tu.m_uuid;
	m_fallbackKey = tu.m_fallbackKey;
}

void CTargetUnit_Standoff::SetUuid(wxString uuid)
{
	m_uuid = uuid;
}



// A work-around for the inability to define a copy constructor which 
// works (it separates the CTargetUnit creation, which is done externally, from 
// the copy syntax which is same as what would be used in the copy constructor).
// WX Note: TODO: If we get the normal copy constructor working, comment this one out
// and provide an overloaded assignment operator as well.
// WX Design Note: It is normally the case ("rule of the big three") that when any one 
// of (1) Destructor, (2) Copy constructor, or (3) Overloaded assignment operator, is 
// needed for a class, then it is most likely that all three should be provided by the 
// class. User-defined copy constructor functions can also be designed as two parameter 
// friend functions. See Dale & Teague pp. 343-344.
/*
void CTargetUnit_Standoff::Copy(const CTargetUnit_Standoff &tu)
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
*/

CTargetUnit_Standoff::~CTargetUnit_Standoff()
{
	delete m_pRefStrMetadataList;
	m_pRefStrMetadataList = (RefStrMetadataList*)NULL;
}



