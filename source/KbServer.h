/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KbServer.h
/// \author			Bruce Waters
/// \date_created	1 October 2012
/// \date_revised	
/// \copyright		2012 Kevin Bradford, Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the KbServer class. 
/// The KbServer class encapsulates the transport mechanism and client API functions for
/// communicating with a kbserver located on a lan, a remote server on the web, or on a
/// networked computer.
/// \derivation		The KbServer class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef KbServer_h
#define KbServer_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KbServer.h"
#endif

// temporarily, while we need #if defined(_KBSERVER) from Adapt_It.h, #includle the latter
// here, and when we don't need the conditional compilation, remove the next line and
// uncomment out the forward declaration class CAdapt_ItApp a little further down
#include "Adapt_It.h"

#if defined(_KBSERVER)

#include <curl/curl.h>

// forward references (we'll probably need these at some time)
class CTargetUnit;
class CRefStringMetadata;
class CRefString;
class CBString;

enum KBType
{
    adaptingKB,
    glossingKB
};

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// The CRefString class stores the target text adaptation typed
/// by the user for a given source word or phrase. It also keeps
/// track of the number of times this translation was previously
/// chosen.
/// \derivation		The CRefString class is derived from wxObject.
class KbServer : public wxObject
{
public:

	// creation & destruction
	
	KbServer(void); // constructor
	KbServer(CAdapt_ItApp* pApp); // the constructor we'll use, it lets us get m_pKB and m_pGlossingKB easily
	virtual	~KbServer(void); // destructor (should be virtual)

	// attributes
public:

	// The API which we expose
	wxString LookupEntryForSourcePhrase( wxString wxStr_SourceEntry );

	// public getters & setters (there may not ever be any)

protected:

	// helpers


private:
	// class variables
	CAdapt_ItApp* m_pApp;
	CKB* m_pMyKB;
	CKB* m_pMyGlossingKB;
	
	// the following 5 are used for setting up the https transport of data to/from the kbserver
	int			m_kbTypeForServer; // 1 for an adaptations KB, 2 for a glosses KB
	CBString	m_kbServerURLBase;
	CBString	m_kbServerUsername;
	CBString	m_kbServerPassword; // we never store this, the user has to remember it 
	CBString	m_kbServerLastSync; // stores a UTC date & time in format: YYYY-MM-DD HH:MM:SS
	// since we use ASCII for the curl calls, it makes sense to store with CBString not wxString
	/*
	wxString	m_kbServerURLBase;
	wxString	m_kbServerUsername;
	wxString	m_kbServerPassword; // we never store this, the user has to remember it 
	wxString	m_kbServerLastSync; // stores a UTC date & time in format: YYYY-MM-DD HH:MM:SS
	*/


	// private member functions
	CKB* SetKB(enum KBType currentKBType);
	CBString GetServerURL();
	CBString GetServerUsername();
	CBString GetServerPassword();
	CBString GetServerLastSync();
	CBString GetSourceLanguageCode();
	CBString GetTargetLanguageCode();
	// since we use ASCII for the curl calls, it makes sense to return a CBString not a wxString
	/*
	wxString GetServerURL();
	wxString GetServerUsername();
	wxString GetServerPassword();
	wxString GetServerLastSync();
	wxString GetSourceLanguageCode();
	wxString GetTargetLanguageCode();
	*/
	int		 GetKBTypeForServer();

	bool     SetKBTypeForServer();

	DECLARE_DYNAMIC_CLASS(KbServer) 

};
#endif // for _KBSERVER

#endif /* KbServer_h */
