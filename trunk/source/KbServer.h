/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KbServer.h
/// \author			Bruce Waters
/// \date_created	1 October 2012
/// \rcs_id $Id$
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

// NOTE: we need to use long, not int, for integers if we use jsonval.cpp. The comment in
// jsonval.cpp at lines 582-4 states:
// "Note that if you are developing cross-platform applications you should never
// use int as the integer data type but \b long for 32-bits integers and
// short for 16-bits integers."
// Hence we define an array of longs which we can use rather than wxArrayInt - the latter
// leads to assertion trips in 64 bit Linux builds at runtime. So far jsonval's been happy with
// using wxArrayInt for the 1 or 0 of the deleted flag. But for entry IDs, we get asserts.
#include <wx/dynarray.h>
WX_DEFINE_ARRAY_LONG(long, Array_of_long);

#if defined(_KBSERVER)

enum ClientAction {
	getForOneKeyOnly,
	changedSince,
	getAll };

#include <curl/curl.h>

// forward references (we'll probably need these at some time)
class CTargetUnit;
class CRefStringMetadata;
class CRefString;
class CBString;
class CKB;
class CAdapt_ItApp;

enum DeleteOrUndeleteEnum
{
    doDelete,
    doUndelete
};

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

	KbServer(void); // default constructor
	KbServer(int whichType); // the constructor we'll use, pass 1 for adapting KB, 2 for glossingKB
	virtual	~KbServer(void); // destructor (should be virtual)

	void	DownloadToKB(CKB* pKB, enum ClientAction action);

	// attributes
public:

	// The API which we expose (note:  srcPhrase & tgtPhrase are often each
	// just a single word
	int		 LookupEntriesForSourcePhrase( wxString wxStr_SourceEntry );
	int		 LookupEntryFields(wxString sourcePhrase, wxString targetPhrase);
	int		 CreateEntry(wxString srcPhrase, wxString tgtPhrase, bool bDeletedFlag);  // was SendEntry()
	int		 PseudoDeleteOrUndeleteEntry(int entryID, enum DeleteOrUndeleteEnum op);
	int		 ChangedSince(wxString timeStamp);
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


	wxString  ImportLastSyncTimestamp(); // imports the datetime ascii string literal
									     // in lastsync.txt file & returns it as wxString
	bool	  ExportLastSyncTimestamp(); // exports it to lastsync.txt file
									     // as an ascii string literal
	// accessors for the private arrays
	//wxArrayInt*		GetIDsArray();
	Array_of_long*	GetIDsArray();
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
	void			UpdateLastSyncTimestamp();
	void			EnableKBSharing(bool bEnable);
	bool			IsKBSharingEnabled();

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
	// a utility for getting the download's UTC timestamp which comes in the X-MySQL-Date
	// header; all the headers precede the json payload, so after removing the pre-payload
	// material, what's left is returned to be assigned back to the global str_CURLbuffer
	// in the caller; the extracted timestamp is returned as a wxString which should be
	// stored in the m_kbServerLastTimestampReceived member
	std::string ExtractTimestampThenRemoveHeaders(std::string s, wxString& timestamp);


private:
	// class variables

	// the following 8 are used for setting up the https transport of data to/from the
	// kbserver for a given KB type (their getters are further below)
	wxString	m_kbServerURLBase;
	wxString	m_kbServerUsername; // typically the email address of the user, or other unique identifier
	wxString	m_kbServerPassword; // we never store this, the user has to remember it
	wxString	m_kbServerLastSync; // stores a UTC date & time in format: YYYY-MM-DD HH:MM:SS
	wxString	m_kbServerLastTimestampReceived; // store UTC date & time in above format received from server
					// NOTE: m_kbServerLastTimestampReceived value replaces m_kbServerLastSync
					// value only after a successful receipt of downloaded data, hence the
					// two variables (m_kbServerLastSync might be needed for more than one
					// GET request before success is achieved)
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

	// public getters for the private member variables above
public:
	wxString	GetKBServerURL();
	wxString	GetKBServerUsername();
	wxString	GetKBServerPassword();
	wxString	GetKBServerLastSync();
	wxString	GetSourceLanguageCode();
	wxString	GetTargetLanguageCode();
	wxString	GetGlossLanguageCode();
	int			GetKBServerType();
	wxString	GetPathToPersistentDataStore();
	wxString	GetPathSeparator();
	wxString	GetCredentialsFilename();
	wxString	GetLastSyncFilename();
	// Getting the kb server password is done in CMainFrame::GetKBSvrPasswordFromUser(),
	// and stored there for the session (it only changes if the project changes and even
	// then only if a different kb server was used for the other project, which would be
	// unlikely)

    // Private storage arrays (they are wxArrayString, but deleted flag and id will use
    // wxArrayInt) for entries data returned from the server, and for uploads too),
	// access to these arrays is by an int iterator, and the data values pertain to a
	// single kbserver entry across the arrays for a given iterator value.
	// Note: we don't provide storage here for source language code, target language code,
	// and kbtype - these are constant for any given instance of KbServer, and their
	// values are determinate from member variables m_kbSourceLanguageCode,
	// m_kbTargetLanguageCode, and m_kbServerType, respectively.
private:
	CAdapt_ItApp*   m_pApp;
	//wxArrayInt		m_arrID;
	Array_of_long   m_arrID;
	wxArrayInt		m_arrDeleted;
	wxArrayString	m_arrTimestamp;
	wxArrayString	m_arrSource;
	wxArrayString	m_arrTarget;
	wxArrayString	m_arrUsername;

	// boolean for enabling, and temporarily disabling, KB sharing. Temporary disabling is
	// potentially needed for the following scenario, for example. If downloading all entries in kbserver for the
	// current project, and syncing the local KB using that data, calls to StoreText()
	// would, try to test for the presence of each such entry in the remote KB - which
	// must not happen as it accomplishes nothing except to waste a heap of time. So we
	// need a way to turn of sharing while the local KB is being synced to the
	// remote-sourced data. I've provided this protection, but probably I'll code a
	// StoreTextFromKbServer() function which will handle syncing from full KB downloads
	// and from ChangedSince() requests, and it won't of course try to do any uploading.
	bool		m_bEnableKBSharing; // default is TRUE, and only FALSE temporarily when required

	DECLARE_DYNAMIC_CLASS(KbServer)

};
#endif // for _KBSERVER

#endif /* KbServer_h */
