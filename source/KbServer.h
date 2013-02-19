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

// temporarily, while we need #if defined(_KBSERVER) from Adapt_It.h, #include the latter
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

/// The KbServerEntry struct is used for storing a single entry of the server's database,
/// in a queue (actually an STL-based wxList<T> instance, which stores pointer to T) - periodic
/// incremental downloads are pushed to the end of the list, and rentries popped from its
/// start, during idle events. The pushes and pops need to be protected with a mutex,
/// because popping is so as to merge an entry to the KB, but another incremental download
/// might be have a push happening which has the queue in a compromised state, of vise versa.
/// Instances are created on the heap, stored in the queue until popped, and once the
/// popped instance has had it's data merged to the KB, it is deleted
struct KbServerEntry; // NOTE, omitting this forwards declaration and having the KbServerEntry
					  // definition here instead, does NOT WORK! And the WX_DEFINE_LIST() macro
					  // in the .cpp file must be somewhere AFTER the #include "KbServer.h" line
WX_DECLARE_LIST(KbServerEntry, DownloadsQueue);

// Not all values are needed from each entry, so I've commented out those the KB isn't
// interested in
struct KbServerEntry {
	long		id; // needed, because pseudo-delete or undelete are based on the record ID
	//wxString	srcLangCode;
	//wxString	tgtLangCode;
	wxString	source;
	wxString	translation; // for gloss, or tgt text, according to mode
	wxString	username;
	//wxString	timestamp;
	//int		type; // the only values allowed are 1 (adapting) or 2 (glossing)
	int			deleted; // the only values allowed are 0 (not pseudo-deleted) or 1 (pseudo-deleted)
};


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
/// BEW 4Feb13, changed the base class from wxObject to be wxThread, because in the
/// Linux version, using wxThread subclassed failed, because the vtable was not created, and
/// so the creator and destructor of the subclass could not find the vtable, despite my having
/// an explicit definition in the subclass for each virtual function of wxThread (which, according
/// to the web is supposed to fix this kind of problem - but it didn't, so I'm having to try
/// find a different waya to support threads.


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
	int		 CreateEntry(wxString srcPhrase, wxString tgtPhrase);
	int		 PseudoDeleteOrUndeleteEntry(int entryID, enum DeleteOrUndeleteEnum op);
	int		 ChangedSince(wxString timeStamp);
	int		 ChangedSince_Queued(wxString timeStamp);
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
	// public helpers
	void			ClearStrCURLbuffer();
	void			UpdateLastSyncTimestamp();
	void			EnableKBSharing(bool bEnable);
	bool			IsKBSharingEnabled();
	CKB*			GetKB(int whichType); // whichType is 1 for adapting KB, 2 for glossing KB

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
	// header; the extracted timestamp is returned as a wxString which should be
	// stored in the m_kbServerLastTimestampReceived member
	void ExtractTimestamp(std::string s, wxString& timestamp);

	// a utility for getting the HTTP status code, and human-readable result string
	void ExtractHttpStatusEtc(std::string s, int& httpstatuscode, wxString& httpstatustext);

	// a utility for getting the English error message returned to str_CURLbuffer, after a
	// failure such as no matching entry found, or, existing matching entry found, etc.
	// Call this function only after determining that a HTTP error beginning with digit
	// "4" has been received (find that out from the results of calling
	// ExtractHttpStatusEtc() beforehand)
	//wxString ExtractHumanReadableErrorMsg(std::string s);

private:
	// class variables
	CKB*		m_pKB; // whichever of the m_pKB versus m_pGlossingKB this instance is associated with
	bool		m_bUseNewEntryCaching; // eventually set this via the GUI

	int			m_httpStatusCode; // for OK it is 200, anything 400 or over is an error
	wxString    m_httpStatusText; 

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
	wxString	m_noEntryMessage; // BEW added 29Jan13
	wxString	m_existingEntryMessage; // BEW added 13Feb13

	// private member functions
	void ErasePassword(); // don't keep it around longer than necessary, when no longer needed, call this

	// public getters for the private member variables above
public:
	// Getting the kb server password is done in CMainFrame::GetKBSvrPasswordFromUser(),
	// and stored there for the session (it only changes if the project changes and even
	// then only if a different kb server was used for the other project, which would be
	// unlikely)
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
	void		UploadToKbServer();
	// these three for storing human readable error messages from the php
	//wxString	GetLastError();
	//void		EmptyErrorString();
	//void		SetErrorString(wxString errorStr);

	// rewrite later, using wxThreadHelper   void		  UploadToKbServerThreaded();

	// Functions we'll want to be able to call programmatically... (button handler
	// versions of these will be in KBSharing.cpp)
	void		DoChangedSince();
	void		DoGetAll();

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
	Array_of_long   m_arrID;
	wxArrayInt		m_arrDeleted;
	wxArrayString	m_arrTimestamp;
	wxArrayString	m_arrSource;
	wxArrayString	m_arrTarget;
	wxArrayString	m_arrUsername;

	// the incremental downloads queue
	DownloadsQueue m_queue;

	// a KbServerEntry struct, for use in downloading or uploading (via json) a
	// single entry
	KbServerEntry	m_entryStruct;

public:

	// public accessors for the private arrays (these are for general uploading and/downloading)
	Array_of_long*	GetIDsArray();
	wxArrayInt*		GetDeletedArray();
	wxArrayString*	GetTimestampArray();
	wxArrayString*	GetSourceArray();
	wxArrayString*	GetTargetArray();
	wxArrayString*	GetUsernameArray();
	void			ClearAllPrivateStorageArrays();
	//void			ClearOneIntArray(wxArrayInt* pArray); // so far unused
	//void			ClearOneStringArray(wxArrayString* pArray); // so far unused
	
	void			PushToQueueEnd(KbServerEntry* pEntryStruct); // protect with a mutex
	KbServerEntry*	PopFromQueueFront(); // protect with a mutex
	bool			IsQueueEmpty();
	void			SetEntryStruct(KbServerEntry entryStruct);
	KbServerEntry	GetEntryStruct();
	void			ClearEntryStruct();

protected:

private:
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
