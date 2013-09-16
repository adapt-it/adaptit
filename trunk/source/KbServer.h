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
/// incremental downloads are pushed to the end of the list, and entries popped from its
/// start, during idle events. The pushes and pops need to be protected with a mutex,
/// because popping is so as to merge an entry to the KB, but another incremental download
/// might be have a push happening which has the queue in a compromised state, or vise versa.
/// Instances are created on the heap, stored in the queue until popped, and once the
/// popped instance has had it's data merged to the KB, it is deleted
struct KbServerEntry; // NOTE, omitting this forwards declaration and having the KbServerEntry
					  // definition here instead, does NOT WORK! And the WX_DEFINE_LIST() macro
					  // in the .cpp file must be somewhere AFTER the #include "KbServer.h" line
struct KbServerUser;  // ditto, for this one
struct KbServerKb;	  // ditto again

WX_DECLARE_LIST(KbServerEntry, DownloadsQueue);
WX_DECLARE_LIST(KbServerEntry, UploadsList); // we'll need such a list in the app instance
		// because kbserver upload threads may not all be finished when the two kbserver
		// instances are released, and if they are not finished, then the KbServerEntry 
		// structs they store will need to live on as long as possible
WX_DECLARE_LIST(KbServerUser, UsersList); // stores pointers to KbServerUser structs for 
										  // the ListUsers() client
WX_DECLARE_LIST(KbServerKb, KbsList); // stores pointers to KbServerKb structs for 
										  // the ListKbs() client

// need a hashmap for quick lookup of keys for find out which src-tgt pairs are in the
// remote KB (scanning through downloaded data from the remote KB), so as not to upload
// pairs which already have a presence in the remote server; used when doing a full KB upload
WX_DECLARE_HASH_MAP(wxString, wxArrayString*, wxStringHash, wxStringEqual, UploadsMap); 
													   
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

struct KbServerUser {
	long		id; // 1-based, from the user table
	wxString	username; // the unique one, or we would like it to be unique (but it doesn't have to be)
	wxString	fullname; // the informal name, such as John Nerd
	bool		kbadmin;
	bool		useradmin;
	wxString	timestamp;
};

struct KbServerKb {
	long		id; // 1-based, from the kb table
	wxString	sourceLanguageCode;
	wxString	targetLanguageCode;
	int			kbType; // 1 for adapting KB, 2 for a glossing KB
	wxString	username; // the unique one, or we would like it to be unique (but it doesn't have to be)
	wxString	timestamp;
	int			deleted; // 0 if not deleted, 1 if deleted (i.e. 'not in use, until deleted status is changed')
						// BEW 10Jul13, Currently we do not support deleted = 1. We don't want
						// either pseudo-deletion, or real deletion of a KB at present, and
						// perhaps permanently. So deleted will always be 0.
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

class KbServer : public wxObject
{
public:

	// creation & destruction

	KbServer(void); // default constructor
	KbServer(int whichType, bool bStateless); // the constructor we'll use for the KB Sharing Manager's use
	KbServer(int whichType); // the constructor we'll use, pass 1 for adapting KB, 2 for glossingKB
	virtual	~KbServer(void); // destructor (should be virtual)


	// attributes
public:

	bool m_bStateless; // TRUE when this instance is needed for the KB Sharing Manager's use
					   // otherwise FALSE
	// ///////// The API which we expose ////////////////////////////////////////////
	// (note:  srcPhrase & tgtPhrase are often each just a single word)
	
    // By passing in a copy of the required strings, we avoid mutex problems that would
    // happen because the internal code would otherwise need to make calls to the KbServer
	// instance to get needed params; these API kbserver functions are setup within the
	// main thread before the containing thread is fired, and so the parameter accesses
	// are synchronous and no mutex is required
	
	
	int		 BulkUpload(int threadIndex, // use for choosing which buffer to return results in
					wxString url, wxString username, wxString password, CBString jsonUtf8Str);
	int		 ChangedSince(wxString timeStamp);
	int		 ChangedSince_Queued(wxString timeStamp, bool bDoTimestampUpdate = TRUE);
	int		 CreateEntry(wxString srcPhrase, wxString tgtPhrase);
	int		 CreateUser(wxString username, wxString fullname, wxString hisPassword, bool bKbadmin, bool bUseradmin);
	int		 CreateKb(wxString srcLangCode, wxString nonsrcLangCode, bool bKbTypeIsScrTgt);
	void	 DownloadToKB(CKB* pKB, enum ClientAction action);
	int		 ListKbs(wxString username, wxString password);
	int		 ListUsers(wxString username, wxString password);
	int		 LookupEntryFields(wxString sourcePhrase, wxString targetPhrase);
	int		 LookupSingleKb(wxString url, wxString username, wxString password, wxString srcLangCode,
							wxString tgtLangCode, int kbType, bool& bMatchedKB);
	int		 LookupUser(wxString url, wxString username, wxString password, wxString whichusername);
	int		 PseudoDeleteOrUndeleteEntry(int entryID, enum DeleteOrUndeleteEnum op);
	int		 DeleteSingleKbEntry(int entryID);
	int		 RemoveUser(int userID);
	int		 RemoveKb(int kbID);
	int		 UpdateUser(int userID, bool bUpdateUsername, bool bUpdateFullName, 
						bool bUpdatePassword, bool bUpdateKbadmin, bool bUpdateUseradmin, 
						KbServerUser* pEditedUserStruct, wxString password);
	int		 UpdateKb(int kbID, bool bUpdateSourceLanguageCode, bool bUpdateNonSourceLanguageCode,  
						int kbType, KbServerKb* pEditedKbStruct);
	void	 UploadToKbServer();
	//int	 LookupEntriesForSourcePhrase( wxString wxStr_SourceEntry ); <<-- currently unused,
	// it gets all tgt words and phrases for a given source text word or phrase
	/* deprecated by BEW 5Jun13
	int		 CreateEntry_Minimal(	KbServerEntry& entry,
									wxString& kbType,
									wxString& password,
									wxString& username,
									wxString& srcLangCode,
									wxString& tgtLangCode,
									wxString& url);
	*/
	// Functions we'll want to be able to call programmatically... (button handlers
	// for these will be in KBSharing.cpp)
	void		DoChangedSince();
	void		DoGetAll(bool bUpdateTimestampOnSuccess = TRUE);

	// ///////////// end of API /////////////////////////////////////////////////////
	
	void	 DeleteUploadEntries();

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
	// public getters
	DownloadsQueue* GetDownloadsQueue();


	wxString  ImportLastSyncTimestamp(); // imports the datetime ascii string literal
									     // in lastsync.txt file & returns it as wxString
	bool	  ExportLastSyncTimestamp(); // exports it to lastsync.txt file
									     // as an ascii string literal
	// public helpers
	void	ClearStrCURLbuffer();
	void	ClearAllStrCURLbuffers2();
	void	UpdateLastSyncTimestamp();
	void	EnableKBSharing(bool bEnable);
	bool	IsKBSharingEnabled();
	CKB*	GetKB(int whichType); // whichType is 1 for adapting KB, 2 for glossing KB

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

	// Extract the source and translation strings, and use the source string as key, and
	// the translation string as value, to populate the m_uploadsMap from the downloaded
	// remote DB data (stored in the 7 parallel arrays). This is mutex protected by the
	// s_DoGetAllMutex)
	void PopulateUploadsMap(KbServer* pKbSvr);
	
	// Populate the m_uploadsList - either with the help of the remote DB's data in the
	// hashmap, or without (the latter when the remote DB has no content yet for this
	// particular language pair) - pass in a flag to handle these two options
	void PopulateUploadList(KbServer* pKbSvr, bool bRemoteDBContentDownloaded);

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

    // Private storage arrays (they are wxArrayString, but deleted flag and id will use
    // wxArrayInt) for bulk entry data returned from the server synchonously.. Access to
    // these arrays is by an int iterator, and the data values pertain to a single kbserver
    // entry across the arrays for a given iterator value. Note: we don't provide storage
    // here for source language code, target language code, and kbtype - these are constant
    // for any given instance of KbServer, and their values are determinate from member
    // variables m_kbSourceLanguageCode, m_kbTargetLanguageCode, and m_kbServerType.
	// These 7 array members are used only for synchronous bulk downloads - that is, only
	// in ChangedSince(), but not in ChangedSince_Queued(). They are not used for uploads.
private:
	CAdapt_ItApp*   m_pApp;
	Array_of_long   m_arrID;
	wxArrayInt		m_arrDeleted;
	wxArrayString	m_arrTimestamp;
	wxArrayString	m_arrSource;
	wxArrayString	m_arrTarget;
	wxArrayString	m_arrUsername;

	// the incremental downloads queue; this stores KbServerEntry structs, for the
	// ChangedSince type of download
	DownloadsQueue m_queue;

	// the templated list which holds KbServerEntry structs, created on the heap, one such
	// for each KB server DB line -- for uploading the src/tgt data in each to the remote
	// DB in Thread_UploadMulti 
	UploadsList		m_uploadsList;

	// For use in full KB uploads
	UploadsMap		m_uploadsMap;

	// For use when listing all the user definitions in the kbserver
	UsersList       m_usersList;
	// For use when listing all the kb definitions in the kbserver
	KbsList         m_kbsList;

	// a KbServerEntry struct, for use in downloading or uploading (via json) a
	// single entry
	KbServerEntry	m_entryStruct;
	// Ditto, but for a single entry from the user table
	KbServerUser	m_userStruct;
	// Ditto, but for a single entry from the kb table
	KbServerKb	m_kbStruct;

public:

    // public accessors for the private arrays (these are for bulk uploading and
    // downloading; UploadToKbServer() uses DoGetAll(), and the downloaders are
    // OnBtnGetAll() and OnBtnChangedSince() which use DoChangedSince() and DownloadToKB()
    // -- and all of these use CKB's StoreEntriesFromKbServer() function which uses the
    // accessors below)
	Array_of_long*	GetIDsArray();
	wxArrayInt*		GetDeletedArray();
	wxArrayString*	GetTimestampArray();
	wxArrayString*	GetSourceArray();
	wxArrayString*	GetTargetArray();
	wxArrayString*	GetUsernameArray();
	void			ClearAllPrivateStorageArrays();
	
	void			PushToQueueEnd(KbServerEntry* pEntryStruct); // protect with a mutex
	KbServerEntry*	PopFromQueueFront(); // protect with a mutex
	bool			IsQueueEmpty();

	//void			SetEntryStruct(KbServerEntry entryStruct);
	KbServerEntry	GetEntryStruct();
	void			ClearEntryStruct();

	//void			SetUserStruct(KbServerUser userStruct);
	KbServerUser	GetUserStruct();
	void			ClearUserStruct();

	KbServerKb		GetKbStruct();
	void			ClearKbStruct();


	UsersList*		GetUsersList();
	void			ClearUsersList(UsersList* pUsrList); // deletes from the heap all KbServerUser struct ptrs within

	KbsList*		GetKbsList();
	void			ClearKbsList(KbsList* pKbsList); // deletes from the heap all KbServerKb struct ptrs within

	void			ClearUploadsMap(); // clears user data (wxStrings) from m_uploadsMap

protected:

private:
    // boolean for enabling, and temporarily disabling, KB sharing
	bool		m_bEnableKBSharing; // default is TRUE

	DECLARE_DYNAMIC_CLASS(KbServer)

};
#endif // for _KBSERVER

#endif /* KbServer_h */