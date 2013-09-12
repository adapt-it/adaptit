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
//#include <wx/valgen.h> // for wxGenericValidator
//#include <wx/valtext.h> // for wxTextValidator
#include "Adapt_It.h"
#include "GuesserAffix.h"

// IMPLEMENT_CLASS(CClassName, wxBaseClassName)
IMPLEMENT_DYNAMIC_CLASS(CGuesserAffix, wxObject)

CGuesserAffix::CGuesserAffix() // constructor
{
	
}
CGuesserAffix::CGuesserAffix(wxString input_affix) // constructor
{
	m_sAffix = input_affix;	
}

CGuesserAffix::~CGuesserAffix() // destructor
{
	
}
wxString CGuesserAffix::getAffix()
{
	return m_sAffix;
}
void CGuesserAffix::setAffix(wxString input_affix)
{
	m_sAffix = input_affix;
}
wxString CGuesserAffix::getCreatedBy()
{
	return m_sCreatedBy;
}
void CGuesserAffix::setCreatedBy(wxString input_cb)
{
	m_sCreatedBy = input_cb;
}

// event handling functions

//CClassName::OnDoSomething(wxCommandEvent& event)
//{
//	// handle the event
	
//}

//CClassName::OnUpdateDoSomething(wxUpdateUIEvent& event)
//{
//	if (SomeCondition == TRUE)
//		event.Enable(TRUE);
//	else
//		event.Enable(FALSE);	
//}

// other class methods

