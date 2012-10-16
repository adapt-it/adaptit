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
#include "Adapt_It.h" // this #includes json_defs.h

#if defined(_KBSERVER)

#include <curl/curl.h>

// forward references (we'll probably need these at some time)
class CTargetUnit;
class CRefStringMetadata;
class CRefString;
class CBString;

// nope, it's better to return the deleted flag's value by the signature
// in LookupEntryID()
//enum PairsFilter
//{
//    onlyUndeletedPairs,
//    onlyDeletedPairs,
//	allPairs
//};

/// This global is defined in Adapt_It.cpp.
//extern CAdapt_ItApp* gpApp; // if we want to access it fast

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

	// The API which we expose (note:  srcPhrase & tgtPhrase are often each
	// just a single word
	int		  LookupEntriesForSourcePhrase( wxString wxStr_SourceEntry );
	int		  LookupEntryFields(wxString sourcePhrase, wxString targetPhrase);
	int		  CreateEntry(wxString srcPhrase, wxString tgtPhrase);  // was SendEntry()
	//int		  LookupEntryID(wxString srcPhrase, wxString tgtPhrase, bool& bDeleted);
	//int		  LookupEntryField(wxString source, wxString target, wxString& field);
	//int		  PseudoDeleteEntry(int entryID);
	//int		  PseudoDeleteEntry(wxString srcPhrase, wxString tgtPhrase);

	// public setters
	void	 SetKBServerType(int type);
	void	 SetKBServerURL(wxString url);
	void	 SetKBServerUsername(wxString username);
	void	 SetKBServerPassword(wxString pw);
	void	 SetKBServerLastSync(wxString lastSyncDateTime);
	void	 SetSourceLanguageCode(wxString sourceCode);
	void	 SetTargetLanguageCode(wxString targetCode);
	void	 SetGlossLanguageCode(wxString glossCode);
	void	 SetPathToPersistentDataStore(wxString metadataPath);
	void	 SetPathSeparator(wxString separatorStr);
	void	 SetCredentialsFilename(wxString credentialsFName);
	void	 SetLastSyncFilename(wxString lastSyncFName);


	wxString  ImportLastSyncDateTime(); // imports the datetime ascii string literal
									   // in lastsync.txt file & returns it as wxString
	bool	  ExportLastSyncDateTime(); // exports it, temporarily, to lastsync.txt file
									   // as an ascii string literal
	// accessors for the private arrays
	wxArrayInt*		GetIDsArray();
	wxArrayInt*		GetDeletedArray();
	wxArrayString*	GetTimestampArray();
	wxArrayString*	GetSourceArray();
	wxArrayString*	GetTargetArray();
	wxArrayString*	GetUsernameArray();

	// public helpers
	void			ClearAllPrivateStorageArrays();
	void			ClearOneIntArray(wxArrayInt* pArray);
	void			ClearOneStringArray(wxArrayString* pArray);
	void			ClearStrCURLbuffer();

protected:

	// helpers

	// the following is used for opening a wxTextFile - it supports line-based read and
	// write, and [i] indexing
	bool	 GetTextFileOpened(wxTextFile* pf, wxString& path);

	// two useful utilities for string encoding conversions (Xhtml.h & .cpp has the same)
	// but we could use wxString::ToUtf8() and wxString::FromUtf8() instead, but the first
	// would give us a bit more work to do to use with CBString
	CBString ToUtf8(const wxString& str);
	wxString ToUtf16(CBString& bstr);


private:
	// class variables
	CAdapt_ItApp* m_pApp;

	// the following 8 are used for setting up the https transport of data to/from the
	// kbserver for a given KB type (their getters are further below)
	wxString	m_kbServerURLBase;
	wxString	m_kbServerUsername; // typically the email address of the user, or other unique identifier
	wxString	m_kbServerPassword; // we never store this, the user has to remember it
	wxString	m_kbServerLastSync; // stores a UTC date & time in format: YYYY-MM-DD HH:MM:SS
	int			m_kbServerType; // 1 for an adapting KB, 2 for a glossing KB
	wxString	m_kbSourceLanguageCode;
	wxString	m_kbTargetLanguageCode;
	wxString	m_kbGlossLanguageCode;
	wxString	m_pathToPersistentDataStore; // should be m_curProjectPath
	wxString	m_pathSeparator;

	wxString	m_credentialsFilename;
	wxString	m_lastSyncFilename;

	// private member functions
	void ErasePassword(); // don't keep it around longer than necessary, when no longer needed, call this

	// getters for the private member variables above
	wxString GetKBServerURL();
	wxString GetKBServerUsername();
	wxString GetKBServerPassword();
	wxString GetKBServerLastSync();
	wxString GetSourceLanguageCode();
	wxString GetTargetLanguageCode();
	wxString GetGlossLanguageCode();
	int		 GetKBServerType();
	wxString GetPathToPersistentDataStore();
	wxString GetPathSeparator();
	wxString GetCredentialsFilename();
	wxString GetLastSyncFilename();

    // Private storage arrays (they are wxArrayString, but deleted flag and id will use
    // wxArrayInt) for entries data returned from the server, and for uploads too),
	// access to these arrays is by an int iterator, and the data values pertain to a
	// single kbserver entry across the arrays for a given iterator value.
	// Note: we don't provide storage here for source language code, target language code,
	// and kbtype - these are constant for any given instance of KbServer, and their
	// values are determinate from member variables m_kbSourceLanguageCode,
	// m_kbTargetLanguageCode, and m_kbServerType, respectively.
	wxArrayInt		m_arrID;
	wxArrayInt		m_arrDeleted;
	wxArrayString	m_arrTimestamp;
	wxArrayString	m_arrSource;
	wxArrayString	m_arrTarget;
	wxArrayString	m_arrUsername;

	

	DECLARE_DYNAMIC_CLASS(KbServer)

};
#endif // for _KBSERVER

#endif /* KbServer_h */
