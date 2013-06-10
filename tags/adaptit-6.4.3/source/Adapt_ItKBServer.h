/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Adapt_ItKBServer.h
/// \author			Bill Martin
/// \date_created	12 February 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CAdapt_ItKBServer class. 
/// The CAdapt_ItKBServer class (does what)
/// \derivation		The CAdapt_ItKBServer class is derived from wxBaseAdapt_ItKBServer.
/////////////////////////////////////////////////////////////////////////////

#ifndef Adapt_ItKBServer_h
#define Adapt_ItKBServer_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Adapt_ItKBServer.h"
#endif

class CAdapt_ItKBServer : public wxObject
{
public:
	CAdapt_ItKBServer(void); // constructor
	virtual ~CAdapt_ItKBServer(void); // destructor
	// other methods

protected:

private:
	// class attributes

	//DECLARE_CLASS(CAdapt_ItKBServer);
	// Used inside a class declaration to declare that the class should 
	// be made known to the class hierarchy, but objects of this class 
	// cannot be created dynamically. The same as DECLARE_ABSTRACT_CLASS.
	
	// or, comment out above and uncomment below to
	DECLARE_DYNAMIC_CLASS(CAdapt_ItKBServer) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CAdapt_ItKBServer)
	
	//DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* Adapt_ItKBServer_h */
