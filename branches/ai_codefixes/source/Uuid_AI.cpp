/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Uuid_AI.cpp
/// \author			Bruce Waters
/// \date_created	5 May 2010
/// \date_revised	
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for UUID generation within Adapt It.
///                 It uses open source third party tools provided by the Boost software library
///                 
///                 TODO **** add proper boost license agreement here ***
///                 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Uuid_AI.h"
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

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
//#include "BString.h"
#include "Uuid_AI.h"
#include <sstream>
#include <string>

IMPLEMENT_DYNAMIC_CLASS(Uuid_AI, wxObject)

Uuid_AI::Uuid_AI()
{
	// construct the UUID here...
	
	//for convenience boost::uuids::random_generator
	//is equivalent to boost::uuids::basic_random_generator<boost::mt19937>
	boost::uuids::random_generator gen;
	u = gen(); // set private uuid member
	
}

Uuid_AI::~Uuid_AI(){}

wxString Uuid_AI::GetGUID()
{
	// return the string representation
	std::stringstream ss;
	using namespace boost;
	using namespace uuids;
	ss << u; // this needed #include <sstream> to compile it
#if defined _UNICODE
	wxString uuidStr;
	uuidStr = uuidStr.FromUTF8((ss.str()).c_str());
	uuidStr = uuidStr.Upper();
	return uuidStr;
#else
	wxString uuidStr;
	uuidStr = (ss.str()).c_str();
	uuidStr = uuidStr.Upper();
	return uuidStr;
#endif
}
