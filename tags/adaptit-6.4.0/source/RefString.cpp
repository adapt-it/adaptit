/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			RefString.cpp
/// \author			Bill Martin
/// \date_created	21 March 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CRefString class. 
/// The CRefString class stores the target text adaptation typed
/// by the user for a given source word or phrase. It also keeps
/// track of the number of times this translation was previously
/// chosen.
/// \derivation		The CRefString class is derived from wxObject.
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
//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
//#include <wx/datstrm.h> // needed for wxDataOutputStream() and wxDataInputStream()

#include "Adapt_It.h"
#include "RefStringMetadata.h"
#include "RefString.h"
#include "AdaptitConstants.h" 
#include "TargetUnit.h"

IMPLEMENT_DYNAMIC_CLASS(CRefString, wxObject)

// use this one when parsing in a kbv1 KB, and maybe some other circumstances - it will
// hook up a CRefStringMetadata instance, but not set the latter's m_creationDateTime and
// m_whoCreated member variables - as those would be supplied by the xml parse and so
// default values don't need to be given at creation time; the caller will also need to
// set the m_pTgtUnit pointer to the onwing CTargetUnit* instance
CRefString::CRefString()
{
	m_refCount = 0;
	m_translation = _T("");
	m_bDeleted = FALSE;
	m_pRefStringMetadata = new CRefStringMetadata; // deliberately use the default one
	// this constructor just initializes to empty strings, and the
	// m_pRefStringOwner value is initialized to NULL, so the next line is required
	
	// now hook it up to this, its owner
	m_pRefStringMetadata->m_pRefStringOwner = this;
}

// normal constructor, with a pointer to its owning CTargetUnit instance - this version
// will set the CRefStringMetadata instance's m_creationDateTime and m_whoCreated member
// variables - which is appropriate for when, say, storing a new adaptation or gloss, etc
CRefString::CRefString(CTargetUnit* pTargetUnit)
{
	m_refCount = 0;
	m_translation = _T("");
	m_bDeleted = FALSE;
	m_pTgtUnit = pTargetUnit; // point to the owning CTargetUnit* instance
	m_pRefStringMetadata = new CRefStringMetadata(this); // this constructor will initialize
				// the m_creationDateTime and m_whoCreated member variables also
}

// copy constructor, where the target unit instance must NEVER be copied as a pointer
// but supplied by the caller (different from the one in the source CRefString), or NULL
CRefString::CRefString(const CRefString &rs, CTargetUnit* pTargetUnit)
{
	if (pTargetUnit == NULL)
	{
		m_pTgtUnit = NULL;
	}
	else
	{
		m_pTgtUnit = pTargetUnit;
	}
	m_refCount = rs.m_refCount;
	m_translation = rs.m_translation;
	m_bDeleted = rs.m_bDeleted;
	m_pRefStringMetadata = new CRefStringMetadata(this);
	// the metadata has to be copied too, so now override the newly assigned creation
	// dateTime, etc with the source once's values
	m_pRefStringMetadata->m_creationDateTime = rs.m_pRefStringMetadata->m_creationDateTime;
	m_pRefStringMetadata->m_modifiedDateTime = rs.m_pRefStringMetadata->m_modifiedDateTime;
	m_pRefStringMetadata->m_deletedDateTime = rs.m_pRefStringMetadata->m_deletedDateTime;
	m_pRefStringMetadata->m_whoCreated = rs.m_pRefStringMetadata->m_whoCreated;
}

CRefString::~CRefString()
{
}

void CRefString::DeleteRefString()
{
	// first delete the pointed at CRefStringMetadata instance
	if (m_pRefStringMetadata != NULL)
	{
		delete m_pRefStringMetadata;
	}
	// now delete the parent CRefString instance
	delete this;
}

bool CRefString::GetDeletedFlag() { return m_bDeleted; }

//inline CRefStringMetadata* CRefString::GetRefStringMetadata()
//I tried making it inline, but the linker could not resolve it
CRefStringMetadata* CRefString::GetRefStringMetadata()
{
	return m_pRefStringMetadata;
}

void CRefString::SetDeletedFlag(bool bValue)
{
	m_bDeleted = bValue;
}

// overloaded equality operator
bool CRefString::operator==(const CRefString& rs)
{
	// note, I've not involved the m_bDeleted value in the tests. So we would have
	// equality when two differ only by the value of m_bDeleted. Would this ever be
	// significant? I've no idea at the moment (May 2010) - it may need fixing later on
	// when we make use of the m_bDeleted value in future versions
	if (rs.m_translation.IsEmpty() && m_translation.IsEmpty())
		return TRUE; // ensure two empty strings constitutes a TRUE value for the test
	return m_translation == rs.m_translation;
}

