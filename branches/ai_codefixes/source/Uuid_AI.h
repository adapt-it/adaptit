/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Uuid_AI.h
/// \author			Bruce Waters
/// \date_created	5 May 2010
/// \date_revised	
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is a header file for UUID generation within Adapt It. It uses
///                 open source third party tools provided by the Boost software library
///                 
///                 TODO **** add proper boost license agreement here ***
///                 
/////////////////////////////////////////////////////////////////////////////
//
#ifndef uuid_ai_h
#define uuid_ai_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Uuid_AI.h"
#endif

#include <boost/uuid/uuid_generators.hpp>

class Uuid_AI : public wxObject
{
public:
	Uuid_AI(void); // constructor
	~Uuid_AI(void); // destructor
	// accessor
	wxString GetGUID();
private:
	boost::uuids::uuid u;

	DECLARE_DYNAMIC_CLASS(Uuid_AI) 

};
















#endif // for uuid_ai_h