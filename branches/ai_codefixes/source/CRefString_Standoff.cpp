/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CRefString_Standoff.cpp
/// \author			Bruce Waters
/// \date_created	19 May 2010
/// \date_revised	
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CRefString_Standoff class. 
/// The CRefString_Standoff class stores metadata pertinent to the CRefString class, and
/// its persistent storage will be in a standoff (xml) file - hence the postfix
/// "_Standoff" in the name. The data it stores is date-time information, and who created
/// the instance.
/// Note: only friends should be able to access this, so there should be no need for
/// getters and setters
/// \derivation		The CRefString_Standoff class is derived from wxObject.
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

//#include "Adapt_It.h"
#include "helpers.h"
#include "CRefString_Standoff.h"
#include "AdaptitConstants.h" 
#include "TargetUnit_Standoff.h"

IMPLEMENT_DYNAMIC_CLASS(CRefString_Standoff, wxObject)

CRefString_Standoff::CRefString_Standoff()
{
	m_pTgtUnit_Standoff = NULL;
	m_creationDateTime = GetDateTimeNow(); // see helpers.cpp
	m_modifiedDateTime = GetDateTimeNow(); // ditto
	m_deletedDateTime = _T("");
	m_whoCreated = _T("");
}

// normal constructor, with a pointer to its owning CTargetUnit instance
CRefString_Standoff::CRefString_Standoff(CTargetUnit_Standoff* pTargetUnit_Standoff)
{
	m_creationDateTime = GetDateTimeNow(); // see helpers.cpp
	m_modifiedDateTime = GetDateTimeNow(); // ditto
	m_deletedDateTime = _T("");
	m_whoCreated = _T("");
	m_pTgtUnit_Standoff = pTargetUnit_Standoff;
}

// copy constructor
CRefString_Standoff::CRefString_Standoff(const CRefString_Standoff &rs, CTargetUnit_Standoff* pTgtUnit_Standoff)
{
	m_creationDateTime = rs.m_creationDateTime;
	m_modifiedDateTime = rs.m_modifiedDateTime;
	m_deletedDateTime = rs.m_deletedDateTime;
	m_whoCreated = rs.m_whoCreated;
	if (pTgtUnit_Standoff == NULL)
	{
		m_pTgtUnit_Standoff = NULL;
	}
	else
	{
		m_pTgtUnit_Standoff = pTgtUnit_Standoff;
	}
}

CRefString_Standoff::~CRefString_Standoff()
{

}

void CRefString_Standoff::SetOwningTargetUnit(CTargetUnit_Standoff* pTU_S)
{
	m_pTgtUnit_Standoff = pTU_S;
}

// overloaded equality operator
/* I can't think of any way that two of these could be considered equal - so we'll probably never test for it
bool CRefString_Standoff::operator==(const CRefString_Standoff& rs)
{
	
}
*/
