/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CRefStringMetadata.h
/// \author			Bruce Waters
/// \date_created	31 May 2010
/// \rcs_id $Id$
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CRefString_Standoff class. 
/// The CRefStringMetadata class stores metadata pertinent to the CRefString class
/// \derivation		The CRefStringMetadata class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "RefStringMetadata.h"
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

//#include "Adapt_It.h"
#include "AdaptitConstants.h"
#include "helpers.h"
#include "RefString.h"
#include "RefStringMetadata.h"
//#include "TargetUnit.h"

IMPLEMENT_DYNAMIC_CLASS(CRefStringMetadata, wxObject)

CRefStringMetadata::CRefStringMetadata()
{
	// we don't want this one to set initial values other than empty strings, and the hookup to the
	// parent will need to be done by the caller
	m_pRefStringOwner = NULL;
	m_creationDateTime = _T("");
	m_modifiedDateTime = _T("");
	m_deletedDateTime = _T("");
	m_whoCreated = _T("");
}

// normal constructor, with a pointer to its owning CRefString instance; this overload
// hooks itself up to the owning CRefString instance automatically
CRefStringMetadata::CRefStringMetadata(CRefString* pRefStringOwner)
{
	m_creationDateTime = GetDateTimeNow(); // see helpers.cpp
	m_modifiedDateTime = _T("");
	m_deletedDateTime = _T("");
	m_whoCreated = SetWho(); // param bool bOriginatedFromTheWeb is default FALSE
	m_pRefStringOwner = pRefStringOwner;
}

// copy constructor
CRefStringMetadata::CRefStringMetadata(const CRefStringMetadata &rs, CRefString* pNewRefStringOwner)
{
	m_creationDateTime = rs.m_creationDateTime;
	m_modifiedDateTime = rs.m_modifiedDateTime;
	m_deletedDateTime = rs.m_deletedDateTime;
	m_whoCreated = rs.m_whoCreated;
	if (pNewRefStringOwner == NULL)
	{
		m_pRefStringOwner = NULL;
	}
	else
	{
		m_pRefStringOwner = pNewRefStringOwner;
	}
}

CRefStringMetadata::~CRefStringMetadata()
{

}

void CRefStringMetadata::SetOwningRefString(CRefString* pRefStringOwner)
{
	m_pRefStringOwner = pRefStringOwner;
}

// overloaded equality operator
/* I can't think of any way that two of these could be considered equal - so we'll probably never test for it
bool CRefStringMetadata::operator==(const CRefStringMetadata& rs)
{
	
}
*/

void CRefStringMetadata::SetCreationDateTime(wxString creationDT)
{
	m_creationDateTime = creationDT;
}

void CRefStringMetadata::SetModifiedDateTime(wxString modifiedDT)
{
	m_modifiedDateTime = modifiedDT;
}

void CRefStringMetadata::SetDeletedDateTime(wxString deletedDT)
{
	m_deletedDateTime = deletedDT;
}

void CRefStringMetadata::SetWhoCreated(wxString whoCreated)
{
	m_whoCreated = whoCreated;
}

