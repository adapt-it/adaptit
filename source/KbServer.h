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
/// communicating with a KBserver located on a lan, a remote server on the web, or on a
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

//#if defined(_KBSERVER)

// A utility function to do the equivalent of curl's curl_easy_encode(), because the
// latter seems to be interfering with heap cleanup when our url-encoded API functions
// return, so that a heap error (crashing the app) occurs. Jonathan provided this
// alternative which just uses std:string class - I'll code it here as a global function
using namespace std;
#include <string>
//std::string urlencode(const std::string &s); // prototype


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
//struct KbServerKb;	  // ditto again
//struct KbServerLanguage;

struct KbServerUserForeign;

// BEW note: the following declarations are only declarations. We get a 2001 linker error
// if the implementation file does not have the needed definitions macros. They are located
// KbServer.cpp at line 73 and following. E.g. WX_DEFINE_LIST(LanguagesList); etc
//WX_DECLARE_LIST(KbServerEntry, DownloadsQueue);
//WX_DECLARE_LIST(KbServerEntry, UploadsList); // we'll need such a list in the app instance
		// because KBserver upload threads may not all be finished when the two KBserver
		// instances are released, and if they are not finished, then the KbServerEntry
		// structs they store will need to live on as long as possible
WX_DECLARE_LIST(KbServerUser, UsersList); // stores pointers to KbServerUser structs for
										  // the ListUsers() client
//WX_DECLARE_LIST(KbServerKb, KbsList); // stores pointers to KbServerKb structs for
										  // the ListKbs() client
//WX_DECLARE_LIST(KbServerLanguage, LanguagesList); // stores pointers to KbServerLanguage structs for
//// the ListLanguages() client (the latter's handler filters out ISO639 codes, leaving only custom codes)

//WX_DECLARE_LIST(KbServerLanguage, FilteredList); // stores pointers to KbServerLanguage structs
//  filtered from LanguagesList, so that only the structs for custom codes are in filteredList

WX_DECLARE_LIST(KbServerUserForeign, UsersListForeign); // stores pointers to KbServerUserForeign 
		// structs for the ListUsers() client call - Leon solution

// need a hashmap for quick lookup of keys for find out which src-tgt pairs are in the
// remote KB (scanning through downloaded data from the remote KB), so as not to upload
// pairs which already have a presence in the remote server; used when doing a full KB upload
//WX_DECLARE_HASH_MAP(wxString, wxArrayString*, wxStringHash, wxStringEqual, UploadsMap);

// BEW 12Oct20 reinstated, with some name changes, for Leon's solution
struct KbServerEntry {
	//wxString 	id; //python: str(id) converts int to string, and atoi(id) converts back to int <- no longer wanted
	wxString	srcLangName;
	wxString	tgtLangName;
	wxString	source;
	wxString	nonSource; // for gloss, or tgt text, according to mode
	wxString	username;
	wxString	timestamp;
	wxChar		type;    //  values allowed are '1' (adapting) or '2' (glossing)
	wxChar		deleted; // values allowed are '0' (not pseudo-deleted) or '1' (pseudo-deleted)
};

struct KbServerUser {
	long		id; // 1-based, from the user table
	wxString	username; // the unique one, or we would like it to be unique (but it doesn't have to be)
	wxString	fullname; // the informal name, such as John Nerd
	wxString	password; // assigned password to username
	wxChar		useradmin; // 1 or 0
	wxString	datetime;
};

// BEW 26Aug20 modified legacy structs can have "Foreign" added to their name, 
// to keep compilation well behaved while Leon and I develop our kbserver solution
struct KbServerUserForeign {
	wxString	username;  // the unique one, (but it doesn't have to be)
	wxString	fullname;  // the informal name
	wxString	password;
	wxChar		useradmin; // permission to change user table _T('1') or _T('0')
};


struct KbServerLanguage {
	wxString	code; // the 2- or 3-letter code, or a RFC5646 code <<-- primary key, since each code is unique
	wxString	username; // which user is creating or has created this language code
	wxString	description; // which language, in a way humans would understand it
	wxString	timestamp;
};

struct KbServerKb {
	//long		id; // from the kb table  (BEW changed 28Aug20, not needed )
	wxString	sourceLanguageName; // BEW 28/Aug/20 changed 'Code' to 'Name'
	wxString	targetLanguageName; // ditto
	int			kbType; // 1 for adapting KB, 2 for a glossing KB
	wxString	username; // definer, the unique one, (but it doesn't have to be)
	//wxString	timestamp; // BEW 28Aug20 deprecated, has no useful function
	//int		deleted; // 0 if not deleted, 1 if deleted (i.e. 'not in use, until deleted status is changed')
				// BEW 10Jul13, Currently we do not support deleted = 1. We don't want
				// either pseudo-deletion, or real deletion of a KB at present, and
				// perhaps permanently. So deleted will always be 0. BEW 28Aug20 never used, now irrelevant
};

enum ClientAction {
	getForOneKeyOnly,
	changedSince,
	getAll };

// whm 4Jul2021 commented out all curl code reference
//#include <curl/curl.h>
/*
#if __cplusplus
extern "C" {
#endif

	// BEW 6Nov21, the C functions for integrating the kbserver support into the AI code base, for __WXMSW__ build
	// These require toolsets: <stdio.h> <stdlib.h> <string.h> and the #include:
	// "C:\Program Files (x86)\MariaDB 10.5\include\mysql\mysql.h"
	// all of which I have located at lines 65 to 75 of KbServer.cpp
	// These functions are C compiled, using extern "C" {  ....  }. in KbServer.cpp starting at line 750
	int	do_list_users();


#if __cplusplus
}
#endif
*/


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
	KbServer(int whichType, bool bForManager); // the constructor we'll use for the KB Sharing Manager's use
	KbServer(int whichType); // the constructor we'll use, pass 1 for adapting KB, 2 for glossingKB
	virtual	~KbServer(void); // destructor (should be virtual)


	// attributes
public:
	bool m_bDoingEntryUpdate; // set TRUE when KbEditorUpdateButton() handler is invoked
	bool m_bForManager; // TRUE when this instance is needed for the KB Sharing Manager's use
					   // otherwise FALSE
	// ///////// The API which we expose ////////////////////////////////////////////
	// (note:  srcPhrase & tgtPhrase are often each just a single word)

	// The functions in this next block do the actual calls to the remote KBserver
	int		 ChangedSince_Timed(wxString timeStamp, bool bDoTimestampUpdate = TRUE);
	wxString FirstTimeGetsAll(); // BEW added 4Aug22: if the stored-in-file timestamp value is empty

	// BEW 10Oct20, take a results file and convert to string array
	bool	 DatFile2StringArray(wxString& execPath, wxString& resultFile, wxArrayString& arrLines);

	// BEW 14Jan21 refactored Populate....() earlier, we need this for Leon's solution
	bool     PopulateLocalKbLines(const int funcNumber, CAdapt_ItApp* pApp, wxString& execPath,
		wxString& datFilename, wxString& sourceLanguage, wxString nonSourceLanguage); // BEW 26Oct20, 
											// created. Populate local_kb_lines.dat from local KB
	// BEW 14Jan21 - still relevant for downloads using Leon's solution
	void	 DownloadToKB(CKB* pKB, enum ClientAction action);

	//int		 ListUsers(wxString ipAddr, wxString username, wxString password, wxString whichusername); // BEW deprecated whichusername
					// 5Sep20  needed, given same signature as LookupUser() on 11Nov20
	int		 ListUsers(wxString ipAddr, wxString username, wxString password); // BEW 9Feb22 BEW 21Mar24 no longer needed

	int		 LookupEntryFields(wxString src, wxString nonSrc); // BEW 13Oct20 refactored
	wxString ExtractTimestamp(wxString firstLine); // BEW 21Oct20, extract timestamp from
					// .dat result file for ChangedSince_Timed(), first line of form
					// "success,<timestamp string>"
	int		 LookupUser(wxString ipAddr, wxString username, wxString password, wxString whichusername);
	void	 UploadToKbServer();

	// Functions we'll want to be able to call programmatically... (button handlers
	// for these will be in KBSharing.cpp)

	void	DoGetAll(bool bUpdateTimestampOnSuccess = TRUE);

	// Functions for access to the remote server - to check if they leak memory too
	// They don't leak. We have to use these instead of detached threads, because the
	// latter incurs openssl leaks of about 1KB per KBserver access
	int		CreateEntry(KbServer* pKbSvr, wxString src, wxString nonSrc);
	int		PseudoUndelete(KbServer* pKbSvr, wxString src, wxString nonSrc);
	int		PseudoDelete(KbServer* pKbSvr, wxString src, wxString nonSrc);
	int		KbEditorUpdateButton(KbServer* pKbSvr, wxString src, wxString oldText, wxString newText);

	// public setters
	void     SetKB(CKB* pKB); // sets m_pKB to point at either the local adaptations KB or local glosses KB
	void	 SetKBServerType(int type);
	void	 SetKBServerIpAddr(wxString ipAddr);
	void	 SetKBServerUsername(wxString username);
	void	 SetKBServerPassword(wxString pw);
	void	 SetKBServerLastSync(wxString lastSyncDateTime);
	void	 SetPathToPersistentDataStore(wxString metadataPath);
	void	 SetPathSeparator(wxString separatorStr);
	void	 SetCredentialsFilename(wxString credentialsFName);
	void	 SetLastSyncFilename(wxString lastSyncFName);
	// BEW 7Sep20 legacy setters, for 'Code' strings ("tpi" etc)
	void	 SetSourceLanguageCode(wxString sourceCode);
	void	 SetTargetLanguageCode(wxString targetCode);
	void	 SetGlossLanguageCode(wxString glossCode);
	// BEW 7Sep20 new setters, for 'Name' strings ("Wangurri" etc)
	void	 SetSourceLanguageName(wxString sourceLangName);
	void	 SetTargetLanguageName(wxString targetLangName);
	void	 SetGlossLanguageName(wxString glossLangName);

	// public getters

	wxString  ImportLastSyncTimestamp(); // imports the datetime ascii string literal
									     // in lastsync.txt file & returns it as wxString
	bool	  ExportLastSyncTimestamp(); // exports it to lastsync.txt file
									     // as an ascii string literal
	// public helpers
	void	UpdateLastSyncTimestamp();
	void	EnableKBSharing(bool bEnable);
	bool	IsKBSharingEnabled();
	CKB*	GetKB(int whichType); // whichType is 1 for adapting KB, 2 for glossing KB
	wxString FixByGettingAll(); // BEW  added 4Aug22, if m_kbServerLastSync is empty (like when it's first time)
	    // or the relevant lastsync_adaptations.txt or lastsync_glosses.txt (if  glosssing) file
		// does not yet exist, then CallExecute will fail for a "python do_changed_since_timed.py" call, so
		// prevent the failure by detecting m_kbServerLastSync and calling FixByGettingAll() with
		// a datetime set to 1920. That's safe to do, but wastes some time maybe.
	// BEW 5Aug22, a public setter for the m_kbServerLastTimeStampReceived timestamp. To be used only
	// when a changed_since_timed request is done but using a 1920 timestamp because the time stamp
	// on first run was an empty string. In that case, we download the whole lot, and use this setter
	// to ensure the lastsync_adaptations.txt file gets reset to a value later than 1920
	void SetLastTimestampReceived(wxString timestamp);

	// helpers

	// the following is used for opening a wxTextFile - it supports line-based read and write,
	// and [i] indexing; BEW 6Aug22: it's public now because changed_since_timed case block needs to use it
	bool	 GetTextFileOpened(wxTextFile* pf, wxString& path);
protected:
	// two useful utilities for string encoding conversions (Xhtml.h & .cpp has the same)
	// but we could use wxString::ToUtf8() and wxString::FromUtf8() instead, but the first
	// would give us a bit more work to do to use with CBString
	CBString ToUtf8(const wxString& str);
	wxString ToUtf16(CBString& bstr);

	// a utility for getting the download's UTC timestamp which comes in the X-MySQL-Date
	// header; the extracted timestamp is returned as a wxString which should be
	// stored in the m_kbServerLastTimestampReceived member
	void ExtractTimestamp(std::string s, wxString& timestamp);

private:
	// class variables
	CKB*		m_pKB; // pointer to either the adapting KB, or glossing KB, instance

	// the following 8 are used for setting up the https transport of data to/from the
	// KBserver for a given KB type (their getters are further below)
	wxString	m_kbServerIpAddrBase;
	wxString	m_kbServerUsername; // typically the email address of the user, or other unique identifier
	wxString	m_kbServerPassword; // we never store this, the user has to remember it
	wxString	m_kbServerLastSync; // stores a UTC date & time in format: YYYY-MM-DD HH:MM:SS
	wxString	m_kbServerLastTimestampReceived; // store UTC date & time in above format received from server
		// NOTE: m_kbServerLastTimestampReceived value replaces m_kbServerLastSync
		// value only after a successful receipt of downloaded data, hence the
		// two variables (m_kbServerLastSync might be needed for more than one
		// GET request before success is achieved)
	int			m_kbServerType; // 1 for an adapting KB, 2 for a glossing KB
	wxString	m_kbSourceLanguageCode; // cull later? No, xhtml and Lift may need these
	wxString	m_kbTargetLanguageCode; // cull later?
	wxString	m_kbGlossLanguageCode;  // cull later?
	wxString	m_pathToPersistentDataStore; // should be m_curProjectPath
	wxString	m_pathSeparator;
	// BEW 7Sep20 since m_kbSourceLanguageCode, m_kbTargetLanguageCode, and
	// m_kbGlossLanguageCode may perhaps also be associated with Lift code and xhtml
	// exporting, I'll provide equivalents here with 'Name' rather than 'Code' so
	// that we use these 'Name' ones for 
	wxString	m_kbSourceLanguageName;
	wxString	m_kbTargetLanguageName;
	wxString	m_kbGlossLanguageName;


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
	// unlikely); or from the "Authenticate" dialog (KBSharingAuthenticationDlg.cpp & .h).
	// Note, when setting a 'for Manager' KbServer instance (eg. as when using the Manager), if the
	// bForManager flag is TRUE in the creator for the CKbServer instance, then 'ForManager' 
	// temporary storage strings are used for the ipAddress, password and username - and 
	// those will get stored in the relevant places in the ptr to the "ForManager" KbServer 
	// ptr instance which the Manager points at with its internal m_pKbServer member. 
	// So then the getters below will get the 'for Manager' strings, and that means
	// any user settings for access to a KBserver instance, whether same one or not, won't get
	// clobbered by some administrator person accessing the KB Sharing Manager from the user's machine.
	// Accessors list...
	wxString	GetKBServerIpAddr();
	wxString	GetKBServerUsername();
	wxString	GetKBServerPassword();
	wxString	GetKBServerLastSync();
	int			GetKBServerType();
	wxString	GetPathToPersistentDataStore();
	wxString	GetPathSeparator();
	wxString	GetCredentialsFilename();
	wxString	GetLastSyncFilename();
	// BEW 7Sep20 legacy getters for the language codes (LIFT or xhtml may need these)
	wxString	GetSourceLanguageCode();
	wxString	GetTargetLanguageCode();
	wxString	GetGlossLanguageCode();
	// BEW 7Sep20 new getters for the language names (KB Shared Manager Tabbed Dlg needs these)
	wxString	GetSourceLanguageName();
	wxString	GetTargetLanguageName();
	wxString	GetGlossLanguageName();


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

public:

	// For use when listing all the user definitions in the KBserver
	// BEW 27Aug20 For Leon's solution
	UsersListForeign m_usersListForeign;

	// BEW 12Oct20, a KbServerEntry struct, to hold field values from a 
	// LookupEntryFields() call, to determine what call is then needed after that
	KbServerEntry	m_entryStruct;
	// Ditto, but for a single entry from the user table
	KbServerUser	m_structUser;

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

#if defined(_DEBUG)
	// BEW 21Jan21 keep the following, it may be useful someday
	wxString		ReturnStrings(wxArrayString* pArr);
#endif

	// BEW 12Oct20 FileToEntryStruct() takes the returned .dat
	// file from a LookupEntryFields() call, and after clearing
	// m_entryStruct, copies each field, in order, to it.
	bool FileToEntryStruct(wxString execFolderPath, wxString datFileName);
	// This version for when in a loop the lines are being obtained from 
	// the arrLines wxArrayString
	bool Line2EntryStruct(wxString& aLine);

	KbServerEntry	GetEntryStruct();
	void			ClearEntryStruct();

	KbServerUser	GetUserStruct();
	void			ClearUserStruct();

	UsersListForeign*		GetUsersListForeign(); // Keep this
	// Keep next
	void			ClearUsersListForeign(UsersListForeign* pUsrListForeign); // deletes from the heap all KbServerUser struct ptrs within

protected:

private:
    // boolean for enabling, and temporarily disabling, KB sharing
	bool		m_bEnableKBSharing; // default is TRUE

	DECLARE_DYNAMIC_CLASS(KbServer)

};
//#endif // for _KBSERVER

#endif /* KbServer_h */
