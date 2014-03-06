/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GuesserAffix.cpp
/// \author			Kevin Bradford
/// \date_created	12 September 2013
/// \date_revised	
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the GuesserAffix class. 
/// The CGuesserAffix class stores prefixes and suffixes which (if they exist) are input into the Guesser 
///    class (CorGuess.h) to possibly improve the accuracy of guesses. Affixes are stored in the xml files
///    GuesserPrefixes.xml and GuesserSuffixes.xml in the project directory
/// \derivation		The CGuesserAffix class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ClassName.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ClassName.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

// other includes
//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
//#include <wx/valgen.h> // for wxGenericValidator
//#include <wx/valtext.h> // for wxTextValidator
#include "GuesserAffix.h"
#include <wx/arrimpl.cpp>

// IMPLEMENT_CLASS(CClassName, wxBaseClassName)
IMPLEMENT_DYNAMIC_CLASS(CGuesserAffix, wxObject)

CGuesserAffix::CGuesserAffix() // constructor
{
	
}
CGuesserAffix::CGuesserAffix(wxString m_sInputSourceAffix, wxString m_sInputTargetAffix) // constructor
{
	m_sSourceAffix = m_sInputSourceAffix;
	m_sTargetAffix = m_sInputTargetAffix;
}

CGuesserAffix::~CGuesserAffix() // destructor
{
	
}
wxString CGuesserAffix::getSourceAffix()
{
	return m_sSourceAffix;
}
wxString CGuesserAffix::getTargetAffix()
{
	return m_sTargetAffix;
}
void CGuesserAffix::setSourceAffix(wxString m_sInputAffix)
{
	m_sSourceAffix = m_sInputAffix;
}
void CGuesserAffix::setTargetAffix(wxString m_sInputAffix)
{
	m_sTargetAffix = m_sInputAffix;
}
wxString CGuesserAffix::getCreatedBy()
{
	return m_sCreatedBy;
}
void CGuesserAffix::setCreatedBy(wxString input_cb)
{
	m_sCreatedBy = input_cb;
}

/// This macro together with the macro array declaration in the .h file
/// complete the definition of a new array class called CGuesserAffixArray.
WX_DEFINE_OBJARRAY(CGuesserAffixArray);


