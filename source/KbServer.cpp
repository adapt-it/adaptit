/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KbServer.cpp
/// \author			Kevin Bradford, Bruce Waters
/// \date_created	26 September 2012
/// \rcs_id $Id$
/// \copyright		2012 Kevin Bradford, Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for functions involved in KBserver support.
/// This .cpp file contains member functions of the KbServer class.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KbServer.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#include <wx/wfstream.h>
#endif

#include <wx/docview.h>
#include <wx/utils.h> // for ::wxDirExists, ::wxSetWorkingDirectory, etc
#include <wx/textfile.h> // for wxTextFile

//#if defined(_KBSERVER)

// whm 4Jul2021 commented out all curl code reference
//#include <curl/curl.h>

static wxMutex s_QueueMutex; // only need one, because we cannot have
							 // glossing & adapting modes on concurrently
static wxMutex s_DoGetAllMutex; // UploadToKbServer() calls DoGetAll() which
			// fills the 7 parallel arrays with remote DB data; but a manual
			// ChangedSince_Timed() or DoChangedSince() call also fill the same
			// arrays - so we have to enforce sequentiality on the use of
			// these arrays. The latter calls ChangedSince_Timed() to do the grunt work.
wxMutex KBAccessMutex; // ChangedSince_Timed() may be entering entries into
			// the local KB while UploadToKbServer() is looping over it's entries
			// to work out which need to be sent to the remote DB
wxMutex s_BulkDeleteMutex; // Because PseudoDeleteOrUndeleteEntry() is used sometimes
			// in normal adapting work, but a lot in a bulk pseudo delete, so enforce
			// sequentiality on the use of the storage infrastructure; likewise for
			// LookupEntryFields()

#define _WANT_DEBUGLOG //  comment out to suppress entry id logging when deleting a whole KB from KBserver
#define _BULK_UPLOAD   // comment out to suppress logging for the <= 50 groups of bulk upload
// Comment out the SYNC_LOGS #define to suppress the debug logging from the XXXX functions
#define SYNC_LOGS


wxCriticalSection g_jsonCritSect; 

#include <wx/listimpl.cpp>

// =========================
//#include <string>
//using namespace std;


// BEW 5Nov21 I won't bother with a using-directive for the other two needed for our embedding of
// .c functions into the app's code for kbserver support, so those two will need to be <name.h> type
// See C++ Primmer Plus, Third Edition, Stephen Prata; pages 360 to 365 for his discussion of the issues
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include "C:\Program Files (x86)\MariaDB 10.5\include\mysql\mysql.h"

// =========================

#include "Adapt_It.h"
#include "TargetUnit.h"
#include "KB.h"
#include "Pile.h"
#include "SourcePhrase.h"
#include "RefString.h"
#include "RefStringMetadata.h"
#include "helpers.h"
#include "Xhtml.h"
#include "MainFrm.h"
#include "StatusBar.h"
#include "XML.h"
#include "KbServer.h"
#include "md5_SB.h"
#include "KBSharingMgrTabbedDlg.h"


//WX_DEFINE_LIST(DownloadsQueue);
//WX_DEFINE_LIST(UploadsList);  // for use by Thread_UploadMulti, for KBserver support
							  // (see member m_uploadsList)
WX_DEFINE_LIST(UsersList);
WX_DEFINE_LIST(UsersListForeign);    // for use by the ListUsers() client, stores KbServerUserForeign structs
//WX_DEFINE_LIST(KbsList);    // for use by the ListKbs() client, stores KbServerKb structs
//WX_DEFINE_LIST(LanguagesList);    // for use by the ListKbs() client, stores KbServerKb structs ??
//WX_DEFINE_LIST(FilteredList); // used in page 3 tab of Shared KB Manager, the LoadLanguagesListBox() call

//WX_DEFINE_LIST(UsersListForeign);    // for use by the ListUsers() client, stores KbServerUserForeign structs
// for wxJson support
#include "json_defs.h" // BEW tweaked to disable 64bit integers, else we get compile errors
#include "jsonreader.h"
#include "jsonwriter.h"
#include "jsonval.h"

extern bool		gbIsGlossing;

// A Note on the placement of SetupForKBServer(), and the protective wrapping booleans
// m_bIsKBServerProject and m_bIsGlossingKBServerProject defaulting to FALSE initially
// and then possibly set to TRUE by reading the project configuration file.
// Since the KBs must be loaded before SetupForKBServer() is called, and the function for
// setting them up is CreateAndLoadKBs(), it may appear that at the end of the latter
// would be an appropriate place, within the TRUE block of an if (m_bIsKBServerProject)
// test or an if (m_bIsGlossingKBServerProject) text.
// However, looking at which functionalities, at a higher level, call
// CreateAndLoadKBs(), many of these are too early for any association of a project with
// a KBserver to have been set up already. Therefore, possibly after a read of the
// project configuration file may be appropriate - since it's that configuration file
// which sets or clears the ywo above boolean flags. This is so: there are
// 4 contexts where the project config file is read: in DoUnpackDocument(), in
// HookUpToExistingProject() for setting up a collaboration, in the
// OpenExistingProjectDlg.cpp file, associated with the "Access an existing adaptation
// project" feature (for transforming adaptations to glosses); and most importantly, in the
// frequently called OnWizardPageChanging() function of ProjectPage.cpp. These are all
// appropriate places for calling SetupForKBServer() late in such functions, in an if TRUE
// test block using m_bIsKBServerProject, or similarly for the glossing one if the other
// flag is TRUE. (There is no need, however, to call it in OpenExistingProjectDlg() because
// the project being opened is not changed in the process, and since it's adaptations are
// being transformed to glosses, it would not be appropriate to assume that the
// being-constructed new project should be, from it's inception, a kb sharing one. The user
// can later make it one if he so desires.)
// Scrap the above comment if we choose instead to instantiate KbServer on demand and
// destroy it immediately afterwards.

//=============================== KbServer class ===================================

std::string str_CURLbuffer;
const size_t npos = (size_t)-1; // largest possible value, for use with std:find()
std::string str_CURLheaders;
std::string str_CURLbuffer_for_deletekb; // used only when deleting a KB (row by row)

// The UploadToKbServer() call, will create up to 50 threads. To permit parallel
// processing we need an array of 50 standard buffers, so that we don't have to force
// sequentiality on the processing of the threads
std::string str_CURLbuff[50];

// And the helper for doing url-encoding...
/* -- don't need it, we'll use curl_easy_escape() & curl_free() instead, along with CBString
std::string urlencode(const std::string &s)
{
    //RFC 3986 section 2.3 Unreserved Characters (January 2005)
    //const std::string unreserved = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~";
	// BEW try these instead
	const std::string unreserved = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~";
    std::string escaped="";
    for(size_t i=0; i<s.length(); i++)
    {
        if (unreserved.find_first_of(s[i]) != std::string::npos)
        {
            escaped.push_back(s[i]);
        }
        else
        {
            escaped.append("%");
            char buf[3];
            sprintf(buf, "%.2X", s[i]);
            escaped.append(buf);
        }
    }
    return escaped;
}
*/





IMPLEMENT_DYNAMIC_CLASS(KbServer, wxObject)

KbServer::KbServer()
{
	m_pApp = (CAdapt_ItApp*)&wxGetApp();

	// using the gbIsGlossing flag is the only way to set m_pKB reliably in the
	// default constructor
	if (gbIsGlossing)
	{
		m_pKB = GetKB(2);
	}
	else
	{
		m_pKB = GetKB(1);
	}
	//m_queue.clear();
	// The following English messages are hard-coded at the server end, so don't localize
	// them, they are returned for certain error conditions and we'll want to know when
	// one has been returned
	//m_noEntryMessage = _T("No matching entry found");
	//m_existingEntryMessage = _T("Existing matching entry found");
	m_bForManager = FALSE; // initialize
	m_bDoingEntryUpdate = FALSE; // initialize
}

KbServer::KbServer(int whichType)
{
	// This is the constructor we should normally use for sharing of KBs
	wxASSERT(whichType == 1 || whichType == 2);
	m_kbServerType = whichType;
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	m_pKB = GetKB(m_kbServerType);
	//m_queue.clear();
	m_bForManager = FALSE;
}

// I had to give the next contructor the int whichType param, because just having a single
// bool param isn't enough to distinguish it from KbServer(int whichType) above, and the
// compiler was wrongly calling the above, instead KbServer(bool bStateless)
// Note: NEVER CALL THIS CONSTRUCTOR WITH THE PARAMETERS (2,FALSE). But (1,TRUE) or
// (2,TRUE) are safe values.
KbServer::KbServer(int whichType, bool bForManager)
{
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	m_pKB = NULL; // There may be no KB loaded yet, and we don't need
				  // it for the KB Sharing Manager GUI's use
	// do something trivial with the parameter whichType to avoid a compiler warning
	// about an unused variable
	if (whichType != 1)
	{
		whichType = 1; // we use this constructor only for jobs where we don't
					   // need to specify a KB type, such as support for the
					   // KB sharing manager GUI, or for entire deletion of a given
					   // type of kb (whether adapting or glossing) and all its entries,
					   // so choose an adapting instantiation - but it makes no difference
					   // what choice we make
	}
	//m_queue.clear();
	m_bForManager = bForManager;
	m_kbServerLastSync = GetDateTimeNow(); // BEW added 24Feb15, an 
						// inintial value, so accesses won't fail
}


KbServer::~KbServer()
{
	; // nothing to do as yet
}

CKB* KbServer::GetKB(int whichType)
{
	if (whichType == 1)
	{
		// the adapting KB is wanted
		wxASSERT(m_pApp->m_bKBReady && m_pApp->m_pKB != NULL);
		return m_pApp->m_pKB;
	}
	else
	{
		// get the glossing KB
		wxASSERT(m_pApp->m_bGlossingKBReady && m_pApp->m_pGlossingKB != NULL);
		return m_pApp->m_pGlossingKB;
	}
}

void KbServer::ErasePassword()
{
	m_kbServerPassword.Empty();
}

CBString KbServer::ToUtf8(const wxString& str)
{
	// converts UTF-16 strings to UTF-8  No need for #ifdef _UNICODE and #else
	// define blocks.
	wxCharBuffer tempBuf = str.mb_str(wxConvUTF8);
    // BEW changed 18Jan13, it appears that the above line can insert a utf-8 BOM 0xEF 0xBB
    // 0xBF at the beginning of the tempBuf C-string; which we'll never want to happen. So
    // test for it an remove it if there. If not removed, it leads to a
    // CURLE_UNSUPPORTED_PROTOCOL (CURLCODE equals 1) error, as the https part of the URL
    // has the BOM immediately preceding it.
    CBString s = CBString(tempBuf);
	// a clumsy way to do this, but it avoids compiler truncation warnings (I tried octal
	// too, but couldn't get the test to return the correct boolean value; so the commented
	// out test works, but gives compiler warnings, but the offsets way has no warnings)
	unsigned char bom1 = (unsigned char)0xEF;
	unsigned char bom2 = (unsigned char)0xBB;
	unsigned char bom3 = (unsigned char) 0xBF;
	int offset1 = s.Find(bom1);
	int offset2 = s.Find(bom2);
	int offset3 = s.Find(bom3);
	// yep, offset1 was 0, offset2 was 1 and offset3 was 2, so it's the utf8 BOM
	//if (s[0] == (char)0xEF && s[1] == (char)0xBB && s[2] == (char)0xBF)
	// Do it differently to avoid compiler warnings about truncation
	if (offset1 == 0 && offset2 == 1 && offset3 == 2)
	{
		//wxLogDebug(_T("KbServer::ToUtf8(), There was a utf8 BOM - so it's been removed"));
		s = s.Mid(3);
		return s;
	}
	else
	{
	return CBString(tempBuf);
	}
}

wxString KbServer::ToUtf16(CBString& bstr)
{
	// whm 21Aug12 modified. No need for #ifdef _UNICODE and #else
	// define blocks.
	wxWCharBuffer buf(wxConvUTF8.cMB2WC(bstr.GetBuffer()));
	// BEW changed 18Jan13 on the basis of what the wx convertion to utf8 did, (i.e. put
	// in an initial unwanted utf8 BOM), the conversion above may put in the utf16 BOM
	// (ie. 0xFF 0xFE), so test for it and remove it if there
	wxString s = wxString(buf);
	unsigned char bom1 = (unsigned char)0xFF;
	unsigned char bom2 = (unsigned char)0xFE;
	int offset1 = s.Find(bom1);
	int offset2 = s.Find(bom2);
	//if (s[0] == (char)0xFF && s[1] == (char)0xFE)
	// Do it differently to avoid compiler warnings about truncation
	if (offset1 == 0 && offset2 == 1)
	{
		//wxLogDebug(_T("KbServer::ToUtf16(), There was a utf16 BOM - so it's been removed"));
		s = s.Mid(2);
		return s;
	}
	else
	{
		return wxString(buf);
	}
}

// the public getters

int KbServer::GetKBServerType()
{
	return m_kbServerType;
}

wxString KbServer::GetKBServerIpAddr()
{
	return m_kbServerIpAddrBase;
}
wxString KbServer::GetKBServerUsername()
{
	return m_kbServerUsername;
}
wxString KbServer::GetKBServerPassword()
{
	return m_kbServerPassword;
}
wxString KbServer::GetKBServerLastSync()
{
	return m_kbServerLastSync;
}

wxString KbServer::GetPathToPersistentDataStore()
{
	return m_pathToPersistentDataStore;
}

wxString KbServer::GetPathSeparator()
{
	return m_pathSeparator;
}

wxString KbServer::GetCredentialsFilename()
{
	return m_credentialsFilename;
}

wxString KbServer::GetLastSyncFilename()
{
	return m_lastSyncFilename;
}

// BEW 7Sep20 legacy getters for codes, Lift or xhtml may need these
// so don't remove them
wxString KbServer::GetSourceLanguageCode()
{
	return m_kbSourceLanguageCode;
}

wxString KbServer::GetTargetLanguageCode()
{
	return m_kbTargetLanguageCode;
}

wxString KbServer::GetGlossLanguageCode()
{
	return m_kbGlossLanguageCode;
}

// BEW 7Sep20 new getters for Manager's tabbed dlg, uses language
// 'Names' not codes
wxString KbServer::GetSourceLanguageName()
{
	return m_kbSourceLanguageName;
}

wxString KbServer::GetTargetLanguageName()
{
	return m_kbTargetLanguageName;
}

wxString KbServer::GetGlossLanguageName()
{
	return m_kbGlossLanguageName;
}

// the public setters & interrogatives

void KbServer::EnableKBSharing(bool bEnable)
{
	m_bEnableKBSharing = bEnable;
}

bool KbServer::IsKBSharingEnabled()
{
	return m_bEnableKBSharing;
}

void KbServer::SetKB(CKB* pKB)
{
	m_pKB = pKB;
}

void KbServer::SetKBServerType(int type)
{
	m_kbServerType = type;
}

void KbServer::SetKBServerIpAddr(wxString ipAddr)
{
	m_kbServerIpAddrBase = ipAddr;
}

void KbServer::SetKBServerUsername(wxString username)
{
	m_kbServerUsername = username;
}

void KbServer::SetKBServerPassword(wxString pw)
{
	m_kbServerPassword = pw;
}

void KbServer::SetKBServerLastSync(wxString lastSyncDateTime)
{
	m_kbServerLastSync = lastSyncDateTime;
}

// Legacy setters, which may still be viable for LIFT & xhtml
void KbServer::SetSourceLanguageCode(wxString sourceCode)
{
	m_kbSourceLanguageCode = sourceCode;
}

void KbServer::SetTargetLanguageCode(wxString targetCode)
{
	m_kbTargetLanguageCode = targetCode;
}

void KbServer::SetGlossLanguageCode(wxString glossCode)
{
	m_kbGlossLanguageCode = glossCode;
}

// BEW 7Sep20 new setters, for 'Name' strings ("Wangurri" etc) Normal adapting
void KbServer::SetSourceLanguageName(wxString sourceLangName)
{
	m_kbSourceLanguageName = sourceLangName;
}
void KbServer::SetTargetLanguageName(wxString targetLangName)
{
	m_kbTargetLanguageName = targetLangName;
}
void KbServer::SetGlossLanguageName(wxString glossLangName)
{
	m_kbGlossLanguageName = glossLangName;
}

void KbServer::SetPathToPersistentDataStore(wxString metadataPath)
{
	m_pathToPersistentDataStore = metadataPath;
}

void KbServer::SetPathSeparator(wxString separatorStr)
{
	m_pathSeparator = separatorStr;
}

void KbServer::SetCredentialsFilename(wxString credentialsFName)
{
	m_credentialsFilename = credentialsFName;
}
void KbServer::SetLastSyncFilename(wxString lastSyncFName)
{
	m_lastSyncFilename = lastSyncFName;
}

void KbServer::UpdateLastSyncTimestamp()
{
	m_kbServerLastSync.Empty();
    // BEW added 28Jan13, somehow a UTF8 BOM ended up in the dateTimeStr passed to
    // ChangedSince_Timed(), so as I've not got BOM detection and removal code in both ToUtf8()
    // and ToUtf16(), I'll round trip the dateTimeStr here to ensure that if there is a BOM
    // that has somehow crept in, then it won't find its way back to the caller!
	CBString utfStr = ToUtf8(m_kbServerLastTimestampReceived);
	m_kbServerLastTimestampReceived = ToUtf16(utfStr);
	// now proceed to update m_kbServerLastSync string with a guaranteed BOM-less timestamp
	m_kbServerLastSync = m_kbServerLastTimestampReceived;
	m_kbServerLastTimestampReceived.Empty();
	ExportLastSyncTimestamp(); // ignore the returned boolean (if FALSE, a message will have been seen)
}

void KbServer::SetLastTimestampReceived(wxString timestamp)
{
	m_kbServerLastTimestampReceived = timestamp;
}

// This function MUST be called, for any GET operation, from the headers information, which
// is returned due to a CURLOPT_HEADERFUNCTION specification & a curl_headers_callback()
// function which loads the header info into str_CURLheaders, a standard string.
// Returns nothing, after timestamp extraction; leaving the standard string unchanged, until
// we clear it with .clear() later in the caller
// BEW refactored 9Feb13, to return nothing, except the timestamp, and leave the passed in
// string unchanged in case the caller wants to extract something else, and return a 1960
// timestamp if there was an error
void KbServer::ExtractTimestamp(std::string s, wxString& timestamp)
{
	// make the standard string into a wxString
	timestamp.Empty(); // initialize
	CBString cbstr(s.c_str());
	wxString buffer(ToUtf16(cbstr));
#if defined (SYNC_LOGS)
    wxLogDebug(_T("buffer:\n%s"), buffer.c_str());
#endif
	wxString srchStr = _T("X-MySQL-Date");
	int length = srchStr.Len();
	int offset = buffer.Find(srchStr);
	if (offset == wxNOT_FOUND)
	{
        // now returning void, so if we couldn't extract the timestamp, just return
        // something early -- a 1960 date would be a way to indicate the error was here
        timestamp = _T("1960-01-01 00:00:00");
        return;
	}
	else
	{
		// throw away everything preceding the matched string
		buffer = buffer.Mid(offset + length + 2);

		// buffer now commences with the timestamp we want to extract - extract
		// everything up to the next '\r' or '\n' - whichever comes first
		int offset_to_CR = buffer.Find(_T('\r'));
		int offset_to_LF = buffer.Find(_T('\n'));
		offset = wxMin(offset_to_CR, offset_to_LF);
		wxASSERT(offset > 0);
        timestamp = buffer.Left(offset);
	}
}

// Return FALSE if pf not successfully opened, in which case the parent does not need to
// close pf; otherwise, return TRUE for a successful open & parent must close it when done
bool KbServer::GetTextFileOpened(wxTextFile* pf, wxString& path)
{
	bool bSuccess = FALSE;
	// for 2.9.4 builds, the conditional compile isn't needed I think, and for Linux and
	// Mac builds which are Unicode only, it isn't needed either but I'll keep it for now
#ifndef _UNICODE
		// ANSI
		bSuccess = pf->Open(path); // read ANSI file into memory
#else
		// UNICODE
		bSuccess = pf->Open(path, wxConvUTF8); // read UNICODE file into memory
#endif
	return bSuccess;
}

// Get and return the timestamp value last stored in persistent (i.e. on disk) storage.
// Temporarily this storage is, when working with an adaptations KB, a file called
// lastsync_adaptations.txt located in the project folder, and for working with glosses, a
// file called lastsync_glosses.txt, in the same folder.
// KbServer class has a private CBString member, m_kbServerLastSync to store the
// returned value. (This function is normally called often in the life of a KbServer
// instance, depending on what the user's chosen syncing interval is)
wxString KbServer::ImportLastSyncTimestamp()
{
	wxString dateTimeStr;
	dateTimeStr = _T("1920-01-01 00:00:00"); // initialize to a safe "early" value

	wxString path = GetPathToPersistentDataStore() + GetPathSeparator() + GetLastSyncFilename();
	bool bLastSyncFileExists = ::wxFileExists(path);
	if (!bLastSyncFileExists)
	{
		// couldn't find lastsync... .txt file in project folder
		// BEW 13Jun13, don't just show an error message, do the job for the user - make a
		// file and put an "early" timestamp in it guaranteed to be earlier than anyone's
		// actual work using a shared KB. E.g. "1920-01-01 00:00:00" as above
		wxTextFile myTimestampFile;
		bool bDidIt = myTimestampFile.Create(path);
		if (bDidIt)
		{
			// assume there will be no error on the following three calls (risk is very small)
			myTimestampFile.AddLine(dateTimeStr);
			myTimestampFile.Write();
			myTimestampFile.Close();
		}
		else
		{
			// warn developer
			wxString msg;
			msg = msg.Format(_T("Failed to create last sync file with path: %s\n\"1920-01-01 00:00:00\" will be used for the imported timestamp, so you can continue working."),
								dateTimeStr.c_str());
			wxMessageBox(msg, _T("Text File Creation Error"), wxICON_ERROR | wxOK);
			return dateTimeStr; // send something useful
		}
	}
	wxTextFile f;
	bool bSuccess = FALSE;
	// for wx 2.9.4 builds or later, the conditional compile within GetTextFileOpened() 
	// isn't needed I think, and for Linux and Mac builds which are Unicode only, it 
	// isn't needed either but I'll keep it for now
	bSuccess = GetTextFileOpened(&f, path);
	if (!bSuccess)
	{
		// warn developer that the wxTextFile could not be opened
		wxString msg = _T("GetTextFileOpened() called in ImportLastSyncTimestamp(): The wxTextFile could not be opened");
		wxMessageBox(msg, _T("Error in support for KBserver"), wxICON_ERROR | wxOK);
		return dateTimeStr; // it will be the early safe default value hard coded above, i.e. start of 2012
	}
	size_t numLines = f.GetLineCount();
	if (numLines == 0)
	{
		// warn developer that the wxTextFile is empty
		wxString msg;
		if (GetKBServerType() == 1)
		{
            msg = _T("GetTextFileOpened() called in ImportLastSyncTimestamp(): The lastsync_adaptations.txt file is empty");
            m_pApp->LogUserAction(msg);
		}
		else
		{
            msg = _T("GetTextFileOpened( )called in ImportLastSyncTimestamp(): The lastsync_glosses.txt file is empty");
            m_pApp->LogUserAction(msg);
		}
		wxMessageBox(msg, _T("Error in support for KBserver"), wxICON_ERROR | wxOK);
		f.Close();
		return dateTimeStr; // it's empty still
	}
	// whew, finally, we have the lastsync datetime string for this KBserver type
	dateTimeStr = f.GetLine(0);
    // BEW added 28Jan13, somehow a UTF8 BOM ended up in the dateTimeStr passed to
    // ChangedSince_Timed(), so as I've not got detection and removal code in both ToUtf8() and
    // ToUtf16(), I'll round trip the dateTimeStr here to ensure that if there is a BOM
    // there then it won't find its way back to the caller!
	CBString utfStr = ToUtf8(dateTimeStr);
	dateTimeStr = ToUtf16(utfStr);

	f.Close();
	return dateTimeStr;
}

// Takes the KBserver's datetime supplied with downloaded data, as stored in the
// m_kbServerLastSync member, and stores it on disk (in the file lastsync_adaptations.txt
// when the instance is dealing with adaptations, and lastsync_glosses.txt when dealing with
// glosses KB; each is located in the AI project folder)
// Return TRUE if no error, FALSE otherwise.
bool KbServer::ExportLastSyncTimestamp()
{
	wxString path = GetPathToPersistentDataStore() + GetPathSeparator() + GetLastSyncFilename();
	bool bLastSyncFileExists = ::wxFileExists(path);
	if (!bLastSyncFileExists)
	{
		// couldn't find lastsync.txt file in project folder - tell developer
		wxString msg;
		if (GetKBServerType() == 1)
        {
            // it's the adaptations KB which is being shared
            msg = _T("wxFileExists()called in ExportLastSyncTimestamp(): The wxTextFile, taking path to lastsync_adaptations.txt, does not exist");
        }
        else
        {
            // it's the glosses KB which is being shared
            msg = _T("wxFileExists()called in ExportLastSyncTimestamp(): The wxTextFile, taking path to lastsync_glosses.txt, does not exist");
        }
		wxMessageBox(msg, _T("Error in support for KBserver"), wxICON_ERROR | wxOK);
		return FALSE;
	}
	wxTextFile f;
	bool bSuccess = FALSE;
    // for 2.9.4 builds, the conditional compile within GetTextFileOpened() isn't needed I
    // think, and for Linux and Mac builds which are Unicode only, it isn't needed either
    // but I'll keep it for now
	bSuccess = GetTextFileOpened(&f, path);
	if (!bSuccess)
	{
		// warn developer that the wxTextFile could not be opened
		wxString msg;
		if (GetKBServerType() == 1)
		{
		    // it's the adaptations KB which is being shared
            msg = _T("GetTextFileOpened()called in ExportLastSyncTimestamp(): The wxTextFile, taking path to lastsync_adaptations.txt, could not be opened");
		}
		else
		{
		    // it's the glosses KB which is being shared
            msg = _T("GetTextFileOpened()called in ExportLastSyncTimestamp(): The wxTextFile, taking path to lastsync_glosses.txt, could not be opened");
		}
		wxMessageBox(msg, _T("Error in support for KBserver"), wxICON_ERROR | wxOK);
		return FALSE;
	}
	f.Clear(); // chuck earlier value
	f.AddLine(GetKBServerLastSync());
	f.Write(); // unfortunately, in the unicode build, this will add a utf8 BOM,
			   // so I've got a check in the ImportLastSyncTimestamp() that will
			   // remove the BOM if present before the rest of the string is passed
			   // to its caller
	f.Close();

	return TRUE;
}

// accessors for the private arrays

Array_of_long* KbServer::GetIDsArray()
{
	return &m_arrID;
}

wxArrayString* KbServer::GetUsernameArray()
{
	return &m_arrUsername;
}

wxArrayString* KbServer::GetTimestampArray()
{
	return &m_arrTimestamp;
}

wxArrayInt*	KbServer::GetDeletedArray()
{
	return &m_arrDeleted;
}

wxArrayString* KbServer::GetSourceArray()
{
	return &m_arrSource;
}

wxArrayString* KbServer::GetTargetArray()
{
	return &m_arrTarget;
}

// If the data download succeeds, the 'last sync' timestamp is extracted & stored in the 
// private member variable: m_kbServerLastTimestampReceived
// If there is a subsequent error then -1 is returned and the timestamp value in 
// m_kbServerLastTimestampReceived should be disregarded -- because we only transfer
// the timestamp from there to the private member variable: m_kbServerLastSync, if there
// was no error. If 0 is returned then in the caller we should merge the data 
// into the local KB, and use the public member function UpdateLastSyncTimestamp() 
// to move the timestamp from m_kbServerLastTimestampReceived into m_kbServerLastSync;
// and then use the public member function ExportLastSyncTimestamp() to export that
// m_kbServerLastSync value to persistent storage. (Of course, the next successful 
// ChangedSince_Timed() call will update what is stored in persistent storage; and 
// the timeStamp value used for that call is whatever is currently within the 
// variable m_kbServerLastSync).

// BEW 17Oct20, I will keep the overall structure, as it has important calls, progress dialog
// support, and depositing of data in 7 parallel arrays. I'll just comment out or change as
// appropriate for Leon's solution
int KbServer::ChangedSince_Timed(wxString timeStamp, bool bDoTimestampUpdate)
{
	// BEW 9Nov20 Don't allow any kbserver stuff to happen, if user is not
	// authenticated, or the relevant app member ptr, m_pKbServer[0] or [1] is
	// NULL, or if gbIsGlossing does not match with the latter ptr as set non-NULL
	if (!m_pApp->AllowSvrAccess(gbIsGlossing))
	{
		return 0; // treat as 'no error', just suppress any mysql access, etc
	}
	// ***** start progress
	CStatusBar* pStatusBar = NULL;
	pStatusBar = (CStatusBar*)m_pApp->GetMainFrame()->m_pStatusBar;
	pStatusBar->StartProgress(_("Receiving..."), _("Receiving..."), 5);
	// ***** progress *****

	wxString separator = m_pApp->PathSeparator;
	wxString execPath = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator; // whm 22Feb2021 changed execPath to m_appInstallPathOnly + PathSeparator
	int length = execPath.Len();
	wxChar lastChar = execPath.GetChar(length - 1);
	wxString strLastChar = wxString(lastChar);
	if (strLastChar != separator)
	{
		execPath += separator; // has path separator at end now
	}

	// BEW 10Mar22 we can keep name distPath, so that lower code still works, but we have to
	// rename the folder to _DATA_KB_SHARING and provide a new path to that folder
	wxString dataFolderName = m_pApp->m_dataKBsharingFolderName; // set to _T("_DATA_KB_SHARING");
	wxString distPath = execPath + dataFolderName; // execPath is where the running AI executable is

	// put the timestamp passed in, into app storage so that ConfigureDATfile() can grab it
	bool bExecutedOK;
	wxString resultsFilename;
	bool bConfiguredOK;
	bool bReportResult;
	wxString execFileName;
	m_pApp->m_ChangedSinceTimed_Timestamp = timeStamp;
	// preserve the bDoTimestampUpdate value for other calls to use
	m_pApp->m_bDoTimestampUpdate = bDoTimestampUpdate;

	bExecutedOK = FALSE; // initialise
	resultsFilename = wxEmptyString;
	m_pApp->RemoveDatFileAndEXE(changed_since_timed); // BEW 11May22 added, must precede call of ConfigureDATfile()
	bConfiguredOK = m_pApp->ConfigureDATfile(changed_since_timed); // = 8
	if (bConfiguredOK)
	{
		execFileName = _T("do_changed_since_timed.exe");
		resultsFilename = _T("changed_since_timed_results.dat");
		bReportResult = FALSE;
		bExecutedOK = m_pApp->CallExecute(changed_since_timed, execFileName, execPath, resultsFilename, 99, 99, bReportResult);
	}


	// ***** at step 2, progress
	pStatusBar->UpdateProgress(_("Receiving..."), 2);
	// ***** progress

	CKB* pKB = NULL;
	if (gbIsGlossing)
	{
		pKB = m_pApp->m_pGlossingKB; // glossing
	}
	else
	{
		pKB = m_pApp->m_pKB; // adapting
	}

	unsigned int index;
	wxString noform = _T("<noform>");
	wxString s;
	KbServerEntry* pEntryStruct = NULL;
	unsigned int listSize = 0; // temp value until I set it properly
	if (m_pApp->m_bDoTimestampUpdate)
	{
		this->m_kbServerLastTimestampReceived = wxEmptyString; //initialise, the temporary storage for incremental downloads
	}

	// Get the results .dat file, open for reading, and get line count... 1st line special
	wxString datPath = execPath + resultsFilename;
	wxString aLine = wxEmptyString; // initialise
	bool bPresent = ::FileExists(datPath);
	if (bPresent)
	{
		// BEW 2Nov20, this is where we need to un-escape any escaped single quotes
		DoUnescapeSingleQuote(execPath, resultsFilename);  // from helpers.cpp

		// Now deal with the returned entry lines - whether a file, or the whole lot in
		// kbserver for the current project
		bool bGoodArray = TRUE;
		wxTextFile f;
		bool bIsOpened = FALSE;
		f.Create(datPath);
		bIsOpened = f.Open();
		if (bIsOpened)
		{
			// BEW 4Aug22 when doing a changed-since for the first time, the last_sync_adaptations.txt file
			// (or the last_sync_glossing.txt file if in glossing mode) will be empty when
			// this function, ChangedSince_Timed(timeStamp,bDoTimestampUpdate) - [flag is default true] is
			// invoked by the always-running timer firing. That, if nothing is done, would kill the attempt
			// to do a do_changed_since_timed.exe (or .py) call. An automatic solution is required. The best
			// thing would be to do a changed_since_timed from "1920-01-01" (a bulk download), because
			// this would get any new entries for the local KB (at the cost of a bit of time wasted), but
			// importantly, the download's results.dat file, if it extracts a datetime, we'll hide it in
			// m_bulkDownload_Timestamp. We can delete it there anytime, we don't use the value anywhere.
			listSize = (unsigned int)f.GetLineCount();

			aLine = f.GetFirstLine();
			wxString tstamp = ExtractTimestamp(aLine);
			timeStamp = tstamp;
			if (!timeStamp.IsEmpty())
			{
				// BEW 17Mar23, check if aLine contains "success". If so, remove it; if not, it's a failure message and
				// and the listSize should be small, perhaps just 1. If so, set listSize = 0; so as to abort processing here
				int myoffset = wxNOT_FOUND;
				wxString strSuccess = _T("success");
				myoffset = aLine.Find(strSuccess);
				if (myoffset != wxNOT_FOUND)
				{
					bGoodArray = TRUE;
				}
				else
				{
					bGoodArray = FALSE;
				}
				// If it's a good array, remove the top line, and write out the file without that top line
				if (bGoodArray)
				{
					f.RemoveLine(0); // don't want "success,<datetime string>" in the array passed to DatFile2StringArray
					bool bWroteOK = f.Write();
					wxASSERT(bWroteOK == TRUE);
				}
				else
				{
					if (f.GetLineCount() < 2)
					{
						// probably no useful data, so empty the arrLines array, which will prevent lower down
						// processing from trying to handle garbage
						f.Clear(); // deletes all lines, sets current line number to 0
						listSize = (unsigned int)f.GetLineCount(); // will be zero
					}
				}
				if (bGoodArray)
				{
					// BEW 22Mar22, use/store the passed in timeStamp value if it is not empty, and not 1920
					this->m_kbServerLastTimestampReceived = timeStamp; // used when setting the file storage for the timestamp
					// If UpdateLastSyncTimestamp() is called below, this value is made
					// semi-permanent (ie. saved to a file for later importing) if
					// bDoTimestampUpdate is TRUE (see below)
				}
			}
			else 
			{
				// warn user, something's wrong because changed_since_timed.exe got no returned timestamp
				if (m_pApp->m_bDoTimestampUpdate)
				{
					if (bExecutedOK == FALSE || timeStamp.IsEmpty())
					{
						// empty timestamp string, or the .exe failed, so return -1 to the caller
						// and a msg in LogUserAction for the developer
						wxString msg = _T("ChangedSince_Timed(): either the do_changed_since_timed.exe call failed, or the timeStamp passed in was empty");
						wxMessageBox(msg, _T("Error"), wxICON_ERROR | wxOK); // tell user too
						m_pApp->LogUserAction(msg);
						return -1;
					}
				}
			}
			// Note, it's possible that ChangedSince_Timed() will return no data for adding to
			// the local KB, e.g. if timer's time span is short and for some reason the user 
			// is thinking rather than adapting, so that the timer trips but nothing is
			// newly added to the remote server. This is not an error situation, so don't
			// treat it like one. Just ignore and give no message back.
			// Removal of "success,<datetime>" first line, changes the array length, so
			//re-get the listSize
			int oldListSize = listSize;
			listSize = f.GetLineCount();
			int newListSize = listSize;
			if (bGoodArray)
			{
				wxASSERT(newListSize < oldListSize);
				wxUnusedVar(newListSize); // to avoid compiler warning
			}
			if (listSize > 1)
			{
				// There is at least one line of entry table data in the file
				wxArrayString arrLines;
				// If any error, a message will appear from internal code
				bool bGotThem = DatFile2StringArray(execPath, resultsFilename, arrLines);

				// **** Progress indicator, step 3 ****
				pStatusBar->UpdateProgress(_("Receiving..."), 3);
				// ***** Progress indicator *****

				if (bGotThem)
				{
					// loop over the lines
					for (index = 0; index < listSize; index++)
					{
						wxString aLine = arrLines.Item(index);
						bool bExtracted = Line2EntryStruct(aLine); // populates m_entryStruct
						if (bExtracted)
						{
							// We can extract source phrase, target phrase, deleted flag value,
							// username, and timestamp string; but for supporting the sync of a local
							// KB we need only to extract source phrase, target phrase, the value of
							// the deleted flag, and the username to be included in the KbServerEntry
							// structs
							pEntryStruct = new KbServerEntry;
							(*pEntryStruct).srcLangName = m_entryStruct.srcLangName;
							(*pEntryStruct).tgtLangName = m_entryStruct.tgtLangName;
							(*pEntryStruct).source = m_entryStruct.source;
							(*pEntryStruct).nonSource = m_entryStruct.nonSource;
							(*pEntryStruct).username = m_entryStruct.username;
							(*pEntryStruct).timestamp = m_entryStruct.timestamp;
							(*pEntryStruct).type = m_entryStruct.type;
							(*pEntryStruct).deleted = m_entryStruct.deleted;

							KBAccessMutex.Lock();

							bool bDeletedFlag = pEntryStruct->deleted == 1 ? TRUE : FALSE;

							pKB->StoreOneEntryFromKbServer(pEntryStruct->source, pEntryStruct->nonSource,
								pEntryStruct->username, bDeletedFlag);

							KBAccessMutex.Unlock();
							delete pEntryStruct;

						} // end of TRUE block for test: if (bExtracted)
					} // end of for loop: for (index = 1; index < listSize; index++)
				} // end of TRUE block for test: if (bGotThem)
				arrLines.Clear();

				// **** Progress indicator, step 3 ****
				pStatusBar->UpdateProgress(_("Receiving..."), 4);
				// ***** Progress indicator *****

				if (bDoTimestampUpdate)
				{
					UpdateLastSyncTimestamp();
				}

			} // end of TRUE block for test: if (listSize > 1)

		} // end of TRUE block for test: if (bIsOpened)
	} // end of TRUE block for test: if (bPresent)

	  // **** Progress indicator, step 3 ****
	pStatusBar->UpdateProgress(_("Receiving..."), 5);
	// ***** Progress indicator *****

	// Finish progress indicator
	pStatusBar->FinishProgress(_("Receiving..."));

	return (int)0;
}

void KbServer::ClearAllPrivateStorageArrays()
{
	m_arrID.clear();
	m_arrDeleted.clear();
	m_arrSource.clear();
	m_arrTarget.clear();
	m_arrUsername.clear();
	m_arrTimestamp.clear();
}

// BEW  added 4Aug22, if m_kbServerLastSync is empty (like when it's first time)
// or the relevant lastsync_adaptations.txt or lastsync_glosses.txt (if  glosssing) file
// does not yet exist, then CallExecute will fail for a do_changed_since_timed.exe call, so
// prevent the failure by detecting m_kbServerLastSync and calling ForstTimeGetsAll() with
// a datetime set to 1920. That's safe to do, but wastes some time maybe.
wxString KbServer::FixByGettingAll()
{
	wxString timestamp = _T("1920-01-01 00:00:00"); // earlier than everything!
	m_kbServerLastTimestampReceived = timestamp;
	UpdateLastSyncTimestamp(); // uses m_kbServerLastTimestampReceived's value and gets it stored in file
	return timestamp;
}

// BEW 14Jan21 - still relevant for downloads using Leon's solution
void KbServer::DownloadToKB(CKB* pKB, enum ClientAction action)
{
	wxASSERT(pKB != NULL);
	int rval = 0; // rval is "return value", initialize it
	wxString timestamp;
    // whm aFeb2018 note: changed name of local curKey below to currKey to avoid
    // confusion with CPhraseBox's value (the curKey there was also named to m_CurKey)
    // But, see uses of the global that might relate to KbServer considerations in
    // ChooseTranslation.cpp's OnButtonRemove() handler
    // and ChooseTranslation.cpp's InitDialog() handler
    wxString currKey;  
	switch (action)
	{
	case getForOneKeyOnly:
	{
		// I'll populate this case with minimal required code, but I'm not planning we ever
		// call this case (too slow)
		currKey = (m_pApp->m_pActivePile->GetSrcPhrase())->m_key;
		// *** NOTE *** in the above call, I've got no support for AutoCapitalization; if
		// that was wanted, more code would be needed here - or alternatively, use the
		// adjusted key from within AutoCapsLookup() which in turn would require that we
		// modify this function to pass in the adjusted string for currKey via
		// DownloadToKB's signature
		if (rval != 0)
		{
			ClearAllPrivateStorageArrays(); // don't risk passing on possibly bogus values
		}
	}
		break;
	case changedSince:
	{
		// get the last sync timestamp value
		timestamp = GetKBServerLastSync();
		if (timestamp.IsEmpty())
		{
			timestamp = FixByGettingAll();
			m_pApp->m_bTimestampWasEmpty = TRUE;
		}
		else
		{
			m_pApp->m_bTimestampWasEmpty = FALSE;
		}
#if defined(SYNC_LOGS)
		wxLogDebug(_T("Doing ChangedSince_Timed() with lastsync timestamp value = %s"), timestamp.c_str());
#endif
		rval = ChangedSince_Timed(timestamp); // bool bDoTimestampUpdate is default TRUE -- we want the returned timestamp stored
					// even if doing the whole lot of entries, on the first run for a given kbserver
		// if there was no error, update the m_kbServerLastSync value, and export it to
		// the persistent file in the project folder
		if (rval == 0)
		{
			UpdateLastSyncTimestamp();
		}
	}
		break;
		// BEW 14Mar23 getAll enum value can no longer be one of the changed_since_timed actions, it has to
		// be based on do_bulk_download.exe which is based on do_bulk_download.py
	case getAll:
	{
		timestamp = _T("1920-01-01"); // earlier than everything!
		rval = ChangedSince_Timed(timestamp, FALSE); // 2nd param, bool bDoTimestampUpdate is default TRUE
		// BEW 22Mar22, explicit FALSE in the call above, causes ChangedSince_Timed() to skip updating the
		// project folder's lastsync_adaptations.txt file's value (or glossing on if a glossing KB), because
		// a 1920 datetime would ruin the incremental data downloads protocal (getting everything instead of a few)
	}
		break;
	} // end of switch (action)
	if (rval != 0)
	{
		// there was an error, display a general error
		wxString msg;
		msg = msg.Format(_("Downloading error, line = %d: rval not zero.  Nothing was downloaded, application continues."),
						__LINE__);
		wxMessageBox(msg, _("Download unspecified failure"), wxICON_ERROR | wxOK);
		m_pApp->LogUserAction(msg);
		return;
	}
	if (action == changedSince || action == getAll)
	{
		// Get the downloaded data into the local KB
		pKB->StoreEntriesFromKbServer(this);
	}
}

//int KbServer::ListUsers(wxString ipAddr, wxString username, wxString password, wxString whichusername)
int KbServer::ListUsers(wxString ipAddr, wxString username, wxString password) // BEW 9Feb22 deprecated whichusername
{
	bool bReady = FALSE;
	bool bAllowAccess = m_pApp->m_bHasUseradminPermission; // defaulted to FALSE
	if (bAllowAccess)
	{
		// I have tested for insufficient permission - using app's public bool, m_bHasUserAdminPermission.
		// If that is TRUE, the code for executing ListUsers() can be called. However, since
		// an accessing attempt on the KB SharingManager will check that boolean too, getting to
		// this ListUsers, a further boolean m_bKBSharingMgrEntered suppresses any internal call of
		// LookupUser() when there has been successful entry to the manager. The user may want/need
		// to make calls to ListUsers() more than once while in the manager.
		m_pApp->RemoveDatFileAndEXE(list_users); // BEW 11May22 added, must precede call of ConfigureDATfile()
		// Prepare the .dat input dependency: "list_users.dat" file, into
		// the execPath folder, ready for the ::wxExecute() call below
		bReady = m_pApp->ConfigureDATfile(list_users); // arg is const int, value 3
		if (bReady)
		{
			// The input .dat file is now set up ready for do_list_users.exe
			wxString execFileName = _T("do_list_users.exe"); // this call does not use user2, just authenticates
			wxString execPath = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator; // whm 22Feb2021 changed execPath to m_appInstallPathOnly + PathSeparator
			wxString resultFile = _T("list_users_results.dat");
			bool bExecutedOK = m_pApp->CallExecute(list_users, execFileName, execPath, resultFile, 32, 33);
			if (!bExecutedOK)
			{
				// error in the call, inform user, and put entry in LogUserAction() - English will do
				wxString msg = _T("Line %d: CallExecute for enum: list_users, failed, in KbServer.cpp - cause unknown. Is kbserver turned on? Contact the developers. Adapt It will continue working, but the user list will be shown empty. ");
				msg = msg.Format(msg, __LINE__);
				wxString title = _T("ListUsers() error");
				wxMessageBox(msg,title,wxICON_WARNING | wxOK);
				m_pApp->LogUserAction(msg);
			}
		}
	} // end of TRUE block for test: if (bAllowAccess)
	return 0;
}

// Note: ipAddr, username and password are passed in, because this request can be made before
// the app's m_pKbServer[two] pointers have been instantiated
int KbServer::LookupUser(wxString ipAddr, wxString username, wxString password, wxString whichusername)
{
	// BEW 21Sep20 redo, we have to determine if username == whichusername right here, 
	// and set the app's m_bUser1IsUser2 true or false, an then set m_bUserAuthenticating
	// accordingly - so that default values get into the Athenticate2Dlg when called
	m_pApp->m_bUser1IsUser2 = FALSE; // initialise FALSE
	m_pApp->m_bUserAuthenticating = FALSE; // initialise, as if foreign user doing things in Manager
	if (username == whichusername)
	{
		m_pApp->m_bUser1IsUser2 = TRUE;
		m_pApp->m_bUserAuthenticating = TRUE;
		// BEW 25Jan22 added next line, so that the 4-field credentials dialog can be suppressed
		// from showing redundantly -- by code within ConfigureMovedDatFile
		m_pApp->m_Username2 = whichusername;
	}
	m_pApp->RemoveDatFileAndEXE(lookup_user); // BEW 11May22 added, must precede call of ConfigureDATfile()
	// Prepare the .dat input dependency: "lookup_user.dat" file, into
	// the execPath folder, ready for the (system() call below. Or, for Windows,
	// take the prepared do_user_lookup_and_credentials_check.exe into the execPath folder
	// ready for calling it.
	bool bReady = m_pApp->ConfigureDATfile(lookup_user); // arg is const int, value 2
	if (bReady)
	{
		// The input lookup_user.dat file is now set up ready for use by 
		// do_user_lookup_and_permissions_check.exe
		wxString execFilename = _T("do_user_lookup_and_permissions_check.exe");
		wxString execPath = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator; // whm 22Feb2021 changed execPath to m_appInstallPathOnly + PathSeparator
		wxString resultFile = _T("lookup_user_results.dat");
		bool bExecutedOK = m_pApp->CallExecute(lookup_user, execFilename, execPath, resultFile, 32, 33, TRUE);
		// In above call, last param, bReportResult, is default FALSE therefore omitted;
		// wait msg 32 is _("Authentication succeeded."), and msg 33 is _("Authentication failed.")

		if (bExecutedOK)
		{
			// success for the call, do subsequent code....

			// Handling post-wxExecute() tweaking code within CallExecute() ---
			// The app class needs to know if the looked up user has useradmin permission
			// level of "1", not "0", because if zero, then that user is blocked from
			// getting access to the KB Sharing Manager (which involves calling ListUsers()).
			// There is an app member variable: m_strUseradminValue; which will receive
			// the value of the useradmin flag. It will be set in the post-wxExecute()'s
			// switch for case 2, in CallExecute(), to one or the other value. The subsequent
			// call of ListUsers() will then be blocked if "0" is the value; and entry to the
			// KB Sharing Manager likewise blocked ('insufficient permission')
			;
		}
		else
		{
			// either failure; or user cancelled out (and saved params restored internally)
			; //  inner code will generate what user needs to see
		}
	} // end of TRUE block for test: if (bReady), if bReady is false, no lookup happens

	//#if defined (_DEBUG)
	//	int halt_here = 1;
	//#endif
	return 0;
}

// Authenticates to the mariaDB/kbserver mysql server, and looks up in the
// current project the row for non-deleted src/nonSrc as passed in.
// Escaping any ' in src or nonSrc is done internally before sending SQL requests
int KbServer::LookupEntryFields(wxString src, wxString nonSrc)
{
	wxString execFileName = _T("do_lookup_entry.exe");
	wxString resultFile = _T("lookup_entry_results.dat");
	wxString datFileName = _T("lookup_entry.dat");
	wxString execPath;
	if (m_pApp->m_curExecPath.IsEmpty())
	{
		m_pApp->m_curExecPath = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator; // whm 22Feb2021 changed execPath to m_appInstallPathOnly + PathSeparator
	}
	execPath = m_pApp->m_curExecPath;

	int rv = -1; // initialise
	// First, store src and nonSrc on the relevant app variables, 
	// so ConfigureDATfile(create_entry)'s ConfigureMovedDatFile() can grab them;
	// or if in glossing mode, store gloss on m_curNormalGloss
	// (It's these 'Normal' ones which get any ' escaping done on them)
	m_pApp->m_curNormalSource = src;
	if (gbIsGlossing)
	{
		m_pApp->m_curNormalGloss = nonSrc;
	}
	else
	{
		m_pApp->m_curNormalTarget = nonSrc;
	}
	m_pApp->RemoveDatFileAndEXE(lookup_entry); // BEW 11May22 added, must precede call of ConfigureDATfile()
	// Since the nonSrc content could be adaptation, or gloss, depending on whether kbType
	// currently in action is '1' or '2' respectively, the same code handles either in
	// the following blocks.
	m_pApp->ConfigureDATfile(lookup_entry); // grabs m_curNormalSource & ...Target from m_pApp

	// The create_entry.dat input file is now ready for grabbing the command
	// line (its first line) for the ::wxExecute() call in CallExecute()
	bool bOK = m_pApp->CallExecute(lookup_entry, execFileName, execPath,
		resultFile, 99, 99, FALSE); // FALSE is bReportResult

	return (rv = bOK == TRUE ? 0 : -1);
}

void KbServer::ClearEntryStruct()
{
	m_entryStruct.srcLangName.Empty();
	m_entryStruct.tgtLangName.Empty();
	m_entryStruct.source.Empty();
	m_entryStruct.nonSource.Empty();
	m_entryStruct.username.Empty();
	m_entryStruct.timestamp.Empty(); 
	m_entryStruct.type = _T('1'); // assume adapting type
	m_entryStruct.deleted = _T('0'); //assume not-deleted
}

KbServerEntry KbServer::GetEntryStruct()
{
	return m_entryStruct;
}

void KbServer::ClearUserStruct()
{
	m_structUser.id = 0;
	m_structUser.username.Empty();
	m_structUser.fullname.Empty();
	m_structUser.password.Empty();
	m_structUser.useradmin = _T('0');
	m_structUser.datetime.Empty();
}

KbServerUser KbServer::GetUserStruct()
{
	return m_structUser;
}

UsersListForeign* KbServer::GetUsersListForeign() // a 'for manager' scenario
{
	return &m_usersListForeign;
}

// deletes from the heap all KbServerUser struct ptrs within m_usersListForeign
void KbServer::ClearUsersListForeign(UsersListForeign* pUsrListForeign)
{
#if defined (_DEBUG)
	size_t listLen = pUsrListForeign->GetCount();
	wxLogDebug(_T("%s::%s(), line %d: entry count : %d"),__FILE__,__FUNCTION__,__LINE__, listLen);
#endif
	if (pUsrListForeign ==NULL || pUsrListForeign->IsEmpty())
		return;
	UsersListForeign::iterator iter;
	UsersListForeign::compatibility_iterator c_iter;
	int anIndex = -1;
	for (iter = pUsrListForeign->begin(); iter != pUsrListForeign->end(); ++iter)
	{
		anIndex++;
		c_iter = pUsrListForeign->Item((size_t)anIndex);
		KbServerUserForeign* pEntry = c_iter->GetData();
		if (pEntry != NULL)
		{
			delete pEntry; // frees its memory block
		}
	}
	// The list's stored pointers are now hanging, so clear them
	pUsrListForeign->Clear();
}

// BEW 14Jan2021 refactored and working fine, Leon's way
int KbServer::CreateEntry(KbServer* pKbSvr, wxString src, wxString nonSrc)
{
	wxUnusedVar(pKbSvr);

	wxString execFileName = _T("do_create_entry.py"); // was _T("do_create_entry.exe");
	wxString resultFile = _T("create_entry_results.dat");
	wxString datFileName = _T("create_entry.dat");
	wxString execPath;
	if (m_pApp->m_curExecPath.IsEmpty())
	{
		m_pApp->m_curExecPath = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator; // whm 22Feb2021 changed execPath to m_appInstallPathOnly + PathSeparator
	}
	execPath = m_pApp->m_curExecPath;
	int rv = -1; // initialise
	// First, store src and nonSrc on the relevant app variables, 
	// so ConfigureDATfile(create_entry)'s ConfigureMovedDatFile() can grab them;
	// or if in glossing mode, store gloss on m_curNormalGloss
	m_pApp->m_curNormalSource = src; // in glossing mode,src here is contents of pSrcPhrase->m_adaption
									 // otherwise in adapting mode, it's m_key
	if (gbIsGlossing)
	{
		m_pApp->m_curNormalGloss = nonSrc;
	}
	else
	{
		m_pApp->m_curNormalTarget = nonSrc;
	}
	// Since the nonSrc content could be adaptation, or gloss, depending on whether kbType
	// currently in action is '1' or '2' respectively, the same code handles either in the following blocks
	m_pApp->RemoveDatFileAndEXE(create_entry); // BEW 11May22 added, must precede call of ConfigureDATfile()
	m_pApp->ConfigureDATfile(create_entry); // grabs m_curNormalSource & ...Target from m_pApp

	// BEW 5Feb22 a block for debugging failure of CreateEntry to input the src/nonSrc pair to kbserver entry table
#if defined (_DEBUG)
	wxString inputDatPath = m_pApp->m_curExecPath;
	inputDatPath += datFileName; // I want to know what's in here - does it have the 2 extra fields kbadmin & kbauth
	bool bExists = ::FileExists(inputDatPath);
	wxTextFile f;
	if (bExists)
	{
		bool bOpened = f.Open(inputDatPath);
		if (bOpened)
		{
			wxString contents = f.GetFirstLine();
			wxLogDebug(_T("%s::%s() line %d: src = %s , nonSrc = %s , in CreateEntry - before CallExecute()"), __FILE__, __FUNCTION__, __LINE__,
				src.c_str(), nonSrc.c_str());
			f.Close();
		}
	}
	else
	{
		wxLogDebug(_T("%s::%s() line %d: src = %s , nonSrc = %s , in CreateEntry - FILE DOES NOT EXIST: create_entry.dat, at path: %s"),
			__FILE__, __FUNCTION__, __LINE__, src.c_str(), nonSrc.c_str(), m_pApp->m_curExecPath.c_str());
	}
#endif

	// The create_entry.dat input file is now ready for grabbing the command
	// line (its first line) for the ::wxExecute() call in CallExecute()
	bool bOK = m_pApp->CallExecute(create_entry, execFileName, execPath,resultFile, 99, 99, FALSE); // FALSE is bReportResult
	// BEW 14Jan21 retested, & it works just fine, and is quick
	return (rv = bOK == TRUE ? 0 : -1);
}

int KbServer::PseudoUndelete(KbServer* pKbSvr, wxString src, wxString nonSrc)
{
	wxUnusedVar(pKbSvr);

	int rv = -1; //initialise
	wxString execFileName = _T("do_pseudo_undelete.py");
	wxString resultFile = _T("pseudo_undelete_results.dat");
	wxString datFileName = _T("pseudo_undelete.dat");
	wxString execPath;
	if (m_pApp->m_curExecPath.IsEmpty())
	{
		m_pApp->m_curExecPath = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator; // whm 22Feb2021 changed execPath to m_appInstallPathOnly + PathSeparator
	}
	execPath = m_pApp->m_curExecPath;
	// First, store src and nonSrc on the relevant app variables, (m_curNormalAdaption),
	// so ConfigureDATfile(pseudo_undelete.dat)'s ConfigureMovedDatFile() can grab them;
	// or if in glossing mode, store gloss on m_curNormalGloss
	m_pApp->m_curNormalSource = src;
	if (gbIsGlossing)
	{
		m_pApp->m_curNormalGloss = nonSrc;
	}
	else
	{
		m_pApp->m_curNormalTarget = nonSrc;
	}
	// Since the nonSrc content could be adaptation, or gloss, depending on whether kbType
	// currently in action is '1' or '2' respectively, the same code handles either in the following blocks.
	m_pApp->RemoveDatFileAndEXE(pseudo_undelete); // BEW 11May22 added, must precede call of ConfigureDATfile()
	m_pApp->ConfigureDATfile(pseudo_undelete); // grabs m_curNormalSource & ...Target from m_pApp

	// The pseudo_undelete.dat input file is now ready for grabbing the command
	// line (its first line) for the system() call in CallExecute()
	bool bOK = m_pApp->CallExecute(pseudo_undelete, execFileName, execPath, resultFile, 99, 99, FALSE); // FALSE is bReportResult
	if (bOK)
	{
		rv = 0;
	}
	return rv;
}

int KbServer::PseudoDelete(KbServer* pKbSvr, wxString src, wxString nonSrc)
{
	wxUnusedVar(pKbSvr);

	int rv = -1; //initialise
	wxString execFileName = _T("python do_pseudo_delete.py");
	wxString resultFile = _T("pseudo_delete_results.dat");
	wxString datFileName = _T("pseudo_delete.dat");
	wxString execPath;
	if (m_pApp->m_curExecPath.IsEmpty())
	{
		m_pApp->m_curExecPath = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator; // whm 22Feb2021 changed execPath to m_appInstallPathOnly + PathSeparator
	}
	execPath = m_pApp->m_curExecPath;

	// First, store src and nonSrc on the relevant app variables, (m_curNormalAdaption),
	// so ConfigureDATfile(pseudo_delete.dat)'s ConfigureMovedDatFile() can grab them;
	// or if in glossing mode, store gloss on m_curNormalGloss
	// BEW 18Jan21, mysql accepts an empty field when adding a <no adaptation> (i.e
	// empty string) in the KB Editor's Add button handler; but when we want to 
	// pseudo-delete that line, the commandLine just having consecutive ,, for the
	// nonSrc doesn't work - mysql treats it as a "does not exist" entry. Checking
	// the Python book, an empty sting is just '' or "" (former is better), so it should
	// be possible to match and empty string if it is presented in the command line as
	// consecutive ' quotes. Let's try this.  Nope, it doesn't work... so refactor AI
	// to store '<no adaptation>' (localizable) in the entry table, but in the local
	// KB retain it as an empty string as before - changes also made at AI.cpp 21,182++ & 21,195++
	//if (nonSrc.IsEmpty())
	//{
	//	nonSrc = _T("\'\'");
	//}
	// Leon says it can be done - but it's not as simple as I thought it might be, see email of 21Jan21
	m_pApp->m_curNormalSource = src;
	if (gbIsGlossing)
	{
		m_pApp->m_curNormalGloss = nonSrc;
	}
	else
	{
		m_pApp->m_curNormalTarget = nonSrc;
	}
	// Since the nonSrc content could be adaptation, or gloss, depending on whether kbType
	// currently in action is '1' or '2' respectively, the same code handles either in the following blocks.
	m_pApp->RemoveDatFileAndEXE(pseudo_delete); // BEW 11May22 added, must precede call of ConfigureDATfile()
	m_pApp->ConfigureDATfile(pseudo_delete); // grabs m_curNormalSource & ...Target from m_pApp

	// The pseudo_delete.dat input file is now ready for grabbing the command
	// line (its first line) for the system() call in CallExecute()
	bool bOK = m_pApp->CallExecute(pseudo_delete, execFileName, execPath, resultFile, 99, 99, FALSE); // FALSE is bReportResult
	if (bOK)
	{
		rv = 0;
	}
	return rv;
}

// BEW 21Oct20, extract timestamp from .dat result file for ChangedSince_Timed(),
// first line of form  "success,<timestamp string>"
// return empty string if "success" was not present in firstLine
wxString KbServer::ExtractTimestamp(wxString firstLine)
{
	wxString timestamp = wxEmptyString;
	int offset = wxNOT_FOUND;
	wxString strSuccess = _T("success");
	offset = firstLine.Find(strSuccess);
	if (offset != wxNOT_FOUND)
	{
		int offset_to_timestamp = strSuccess.Len() + 1;
		timestamp = firstLine.Mid(offset_to_timestamp); // +1 to get past the comma
	}
	return timestamp;
}

// BEW 15Jan21, finished refactoring, tested, it's working right with Leon's solution
int KbServer::KbEditorUpdateButton(KbServer* pKbSvr, wxString src, wxString oldText, wxString newText)
{
	// BEW 14Oct20, refactor this handler for the _KBSERVER part of the KBEditor 
	// Update button's handler. There is a protocol to be observed, and it is in 
	// the code below. Tweaking it for Leon's python-based calls is pretty much 
	// all of significance that needs fixing.
	// The protocol is in two parts. 
	// First part deals with removal of the oldText (nonSrc) and handling deletion flag
	// issues arising from the entry table having the deleted flag = 0, if 0, a 
	// pseudo-deletion is needed as well to get rid of it from the GUI. 
	// Second part deals with substituting the newTxt, it requires another 
	// LookupEntryFields()call using src/newText, and then we can work out what 
	// to do.  If newText is not in an entry of the entry table, then we call
	// CreateEntry() to insert it there; if it's already there, then we look at 
	// the deleted flag value - if its deleted flag is 0, then it's already 
	// undeleted (matching what the GUI shows) and so nothing further to do; 
	// but if it's deleted flag is 1, it's pseudo-deleted and so a call of the
	// PseudoUndelete() function is needed so that it becomes visible again.
	m_bDoingEntryUpdate = TRUE;  // helps with the logic used within

	int rv = 0;

	wxASSERT(!src.IsEmpty()); // the key must never be an empty string

	rv = pKbSvr->LookupEntryFields(src, oldText); // with a view to PseudoDeleting it

	if (rv == 0)
	{
		// No error from the lookup, so try PseudoDeleting it
		
		// If the entry table for the relevant line has '0' for the deleted flag 
		// (i.e. not pseudo-deleted, which is what we expect), then go ahead and
		// pseudo-delete it. But if it has 1 already, there is nothing to 
		// do with this src & oldText  string pair, the pseudo-deletion is already
		// in place in the mysql entry table
		wxChar chUndeleted = _T('0'); // initialise value
		if (m_entryStruct.deleted == chUndeleted)
		{
			// Do a pseudo_delete (case 5) here, the oldText is still visible and 
			// not pseudo-deleted yet
			rv = PseudoDelete(pKbSvr, src, oldText);
			wxASSERT(rv == 0);
		}
	}
	//                          ***** part 2 *****
	// That removes the oldText that was to be updated. 
	// BEW replace the old logic with this one call. do_create_entry.py or .exe does it all
	rv = pKbSvr->CreateEntry(pKbSvr, src, newText);
	// If the user wants to check what is in the returned .dat file, create_entry_results.dat,
	// it will be found in the AI executable's folder, and can be read with any text editor

	/* 
	//BEW 6Apr22,removed this logic, with it's dependence on LookupEntryFields() - this is the only place where
	// the latter was needed, and the refactored do_create_entry.py or .exe handles any change of deletion
	// flag from 1 to 0, if that need is detected within it's call

	// Now deal with the scr/newText pair -- another lookup is needed...
	rv = pKbSvr->LookupEntryFields(src, newText); // returns -1 if the entry does not yet exist
	if (rv != 0) // this lookup failed - i.e. -1 returned	
	{
		// If the lookup failed, we must assume it was because there was no matching entry
		// - in which case none of the collaborating clients has seen this entry yet, and 
		// so we can now create it in the entry table
		rv = pKbSvr->CreateEntry(pKbSvr, src, newText);
		//wxASSERT(rv == 0);
		// Ignore errors, if it didn't succeed, no big deal - someone else will sooner or
		// later succeed in adding this one to the remote DB. Internally it checks that there
		// is no duplication happening, before doing the creation and adding to entry table
	}
	else
	{
		// No error from the lookup, so get the value of the deleted flag, etc
		wxChar deletedFlag = m_entryStruct.deleted;
		int nDeleted = wxAtoi(deletedFlag);

		nDeleted = nDeleted; // avoid gcc set but not used warning in release builds
		int theID = wxAtoi(m_entryStruct.id);
		theID = theID; // avoid gcc set but not used warning in release builds

#if defined(SYNC_LOGS)
		wxLogDebug(_T("2nd LookupEntryFields in KbEditorUpdateButton:  source = %s , translation = %s , deleted = %d , username = %s"),
			(m_entryStruct.source).c_str(), (m_entryStruct.nonSource).c_str(), nDeleted, (m_entryStruct.username).c_str());
#endif
		// If the entry table has '0' for the deleted flag's value - then kbserver
		// already has this as a normal entry, so we've nothing to do here. On the other
		// hand, if the flag's value is '1' (it's currently pseudo-deleted in the entry
		// table) then go ahead and undo the pseudo-deletion, making a normal entry
		if (deletedFlag == _T('1'))
		{
			// It exists in the entry table, but as a pseudo-deleted entry, so
			// do an undelete of it, so it becomes visible in the GUI
			rv = PseudoUndelete(pKbSvr, src, newText);
		}
		// Ignore errors, for the same reason as above (i.e. it's no big deal)
	}
	*/
	m_bDoingEntryUpdate = FALSE; // restore value to FALSE (default)
	return rv;
}

void KbServer::DoGetAll(bool bUpdateTimestampOnSuccess)
{
	int rv = 0; // rv is "return value", initialize it
	// get the last sync timestamp value
	wxString timestamp = _T("1920-01-01 00:00:00"); // earlier than everything!;
#if defined(SYNC_LOGS)
	wxLogDebug(_T("DoGetAll() with lastsync timestamp value = %s"), timestamp.c_str());
#endif
	rv = ChangedSince_Timed(timestamp);
	// if there was no error, update the m_kbServerLastSync value, and export it to
	// the persistent file in the project folder
	if (rv == 0)
	{
		if (bUpdateTimestampOnSuccess)
		{
			UpdateLastSyncTimestamp();
		}
	}
	else
	{
		if (bUpdateTimestampOnSuccess)
		{
			// There was an error, display it
			wxString msg;
			msg = msg.Format(_("%s() line %d,Downloading to the knowledge base: an error code (rv = %d) was returned.  Nothing was downloaded, application continues."),
				__FUNCTION__,__LINE__,rv);
			wxMessageBox(msg, _("Downloading to the knowledge base failed"), wxICON_ERROR | wxOK);
			m_pApp->LogUserAction(msg);
		}
		return;
	}
	if (bUpdateTimestampOnSuccess)
	{
        // Store in the local KB the entries received from the entry table
		m_pKB->StoreEntriesFromKbServer(this);
	}
}

#if defined(_DEBUG) && defined(SYNC_LOGS)
// BEW 21Jan21, used for debugging, but may be useful sometime - retain it
wxString KbServer::ReturnStrings(wxArrayString* pArr)
{
	int count = (int)pArr->GetCount();
	int i;
	wxString s;
	for (i = 0; i < count; i++)
	{
		s += _T("[");
		s += pArr->Item((size_t)i);
		s += _T("] ");
	}
	return s;
}
#endif

// BEW 20Dec20, The upload function for sending the local KB's entries to the 
// entry table of kbserver, as a bulk upload
void KbServer::UploadToKbServer()
{
	// BEW 9Nov20 Don't allow any kbserver stuff to happen, if user is not
	// authenticated, or the relevant app member ptr, m_pKbServer[0] or [1] is
	// NULL, or if gbIsGlossing does not match with the latter ptr as set non-NULL
	if (!m_pApp->AllowSvrAccess(gbIsGlossing))
	{
		return; // suppress any mysql access, etc
	}
	m_pApp->RemoveDatFileAndEXE(upload_local_kb); // BEW 11May22 added, must precede call of ConfigureDATfile()
	bool bConfiguredOK = m_pApp->ConfigureDATfile(upload_local_kb);
	if (bConfiguredOK)
	{
		wxString execPath = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator; // whm 22Feb2021 changed execPath to m_appInstallPathOnly + PathSeparator
		wxString execFileName = _T("do_upload_local_kb.exe");
		wxString resultFile = _T("upload_local_kb_results.dat");
		//m_pApp->CallExecute(upload_local_kb, execFileName, execPath, resultFile, 99, 99); 
		m_pApp->CallExecute(upload_local_kb, execFileName, execPath, resultFile, 99, 99);
	}
}

// BEW 10Oct20, take a results file and convert to string array.  I need this on AI.h too, for when I 
// want to process the result file in CallExecute()'s post wxExecute() call's switch, so I'll make a 
// public copy on app. Yuck, but saves time.
// BEW 22Mar22, Leon's new results file has no initial line with "success" etc, all lines are just
// downloaded rows from the entry table, and so lineIndex in the loop must now commence at 0, and I improved
// the logic a bit, for better handling of the possibility of the results file being absent
bool KbServer::DatFile2StringArray(wxString& execPath, wxString& resultFile, wxArrayString& arrLines)
{
	arrLines.Empty(); // clear contents
	wxString pathToResults = execPath + resultFile;
	bool bResultsFileExists = ::FileExists(pathToResults);
	if (bResultsFileExists)
	{
		wxTextFile f(pathToResults);
		bool bOpened = f.Open();
		int lineIndex = 0;
		int lineCount = 0; // initialise to empty
		bool bExists = FALSE;
		if (bOpened)
		{
			bExists = TRUE;
			lineCount = f.GetLineCount(); // could return 0, that's not to be interpretted as an error
		}
		else
		{
			// results file does not exist, log the error
			wxBell();
			wxString msg = _T("DatFile2StringArray()'s results file does not exist in the executable's folder: %s");
			msg = msg.Format(msg, execPath.c_str());
			m_pApp->LogUserAction(msg);
			return FALSE;
		}

		wxString strCurLine = wxEmptyString;
		if (bOpened && bExists)
		{
			for (lineIndex = 0; lineIndex < lineCount; lineIndex++)
			{
				strCurLine = f.GetLine(lineIndex);
#if defined (_DEBUG) && defined (SYNC_LOGS)
				wxLogDebug(_T("DatFile2StringArray() line %d: strCurLine = %s"),__LINE__, strCurLine.c_str());
#endif
				arrLines.Add(strCurLine);
			}
		}
	}
	return TRUE;
}

// The array has no top line with "success" & a timestamp, so the
// loop indexes from 0 to size - 1
// BEW retain this 11Jan20 -- needed for ChangedSince_Timed()
bool KbServer::Line2EntryStruct(wxString& aLine)
{
	ClearEntryStruct();
	wxString comma = _T(",");
	int offset = wxNOT_FOUND;
	int index = 0; // initialise iterator

	offset = wxNOT_FOUND;
	offset = offset; // avoid gcc warning: set but not used
	wxString left;
	//int anID = 0; // initialise
	
	int fieldCount = 9; // id,srcLangName,tgtLangName,src,tgt,username,timestamp,type,deleted
	wxString resultLine = aLine;

#if defined (_DEBUG) && defined (SYNC_LOGS)
	wxLogDebug(_T("Line2EntryStruct() line %d: resultLine = %s"),__LINE__, resultLine.c_str());
#endif

	// BEW 17Mar23 the old way is potentially dangerous, any small error would mess things up.
	// Instead, use a wxStringArray with SmartTokenize(), a delimiter string _T(","), and ignore
	// any empty strings (there should be none anyway).
	wxArrayString arrFields;
	wxString delimiters = _T(","); // the data has a final comma at end of each line, so safe
	// bStoreEmptyStringsToo is default TRUE, so give explicit FALSE;
	long numStringsInArray = SmartTokenize(delimiters, aLine, arrFields, FALSE);
	wxUnusedVar(numStringsInArray);
	/* KbServerEntry struct looks like this: (id field is ignored, so indexing arrFields starts at 1)
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
	*/
	for (index = 0; index < fieldCount; index++)
	{
		if (index == 0)
		{
			continue;
		}
		if (index == 1)
		{
			m_entryStruct.srcLangName = arrFields.Item((size_t)index);
		}
		else if (index == 2)
		{
			m_entryStruct.tgtLangName = arrFields.Item((size_t)index);
		}
		else if (index == 3)
		{
			m_entryStruct.source = arrFields.Item((size_t)index);
		}
		else if (index == 4)
		{
			m_entryStruct.nonSource = arrFields.Item((size_t)index);

#if defined (_DEBUG)  && defined (SYNC_LOGS)
			// BEW added 17Mar23 to track progress by logging .source and .nonSource
			wxLogDebug(_T("Line2EntryStruct() at line %d: [src:tgt] = [%s:%s]"), __LINE__, m_entryStruct.source.c_str(), m_entryStruct.nonSource.c_str());
#endif
		}
		else if (index == 5)
		{
			m_entryStruct.username = arrFields.Item((size_t)index);
		}
		else if (index == 6)
		{
			m_entryStruct.timestamp = arrFields.Item((size_t)index);
		}
		else if (index == 7)
		{
			wxString value = arrFields.Item((size_t)index);
			m_entryStruct.type = value.GetChar(0);
		}
		else if (index == 8)
		{
			wxString value = arrFields.Item((size_t)index);
			m_entryStruct.deleted = value.GetChar(0);
		}
	}
	// KbServer.cpp has a public m_entryStruct; it also has an accessor for it, GetEntryStruct();
	return TRUE;
}

// pass in execFolderPath with final PathSeparator; datFileName is a *_results.dat file
bool KbServer::FileToEntryStruct(wxString execFolderPath, wxString datFileName)
{
	if (m_pApp->m_bAdaptationsKBserverReady || m_pApp->m_bGlossesKBserverReady)
	{
		wxASSERT(!execFolderPath.IsEmpty());
		wxASSERT(!datFileName.IsEmpty());
		ClearEntryStruct();
		wxString filePath = execFolderPath + datFileName; // path to the datFileName
		wxString comma = _T(",");
		wxString strSuccess = _T("success"); // this should be somewhere in the file's top line
		int offset = wxNOT_FOUND;
		int count = 0; // initialise
		bool bExists = ::FileExists(filePath);
		if (bExists)
		{
			// unescape any internal \'
			DoUnescapeSingleQuote(execFolderPath, datFileName);
		}
		wxTextFile f;
		if (bExists)
		{
			bool bOpened = f.Open(filePath);
			if (bOpened)
			{
				wxString TopLine = f.GetLine((size_t)0);
				offset = TopLine.Find(strSuccess);
				wxString left;
				if (offset >= 0)
				{
					// The top line is not data, but for indicating success of the do_lookup_entry.exe call.
					
					//BEW 18Mar23 this function is only used in LookupEntry, which we no longer use.
					// Even so, the "success" first line will cause the code below to fail, and crash
					// the app if called. So we must remove the top line with "success" and then write out
					// the array to the file, to give it a hope of completing. (Better, if I also change the
					// lower for loop to not do progressive shortening (too fragile), but instead do as in
					// Line2EntryStruct() (approx 1777) which uses SmartTokenizer to build the subfields array.)
					f.RemoveLine(0); // now preserve this change by writing the changed data back to file
					bool bWroteOK = f.Write();
					wxASSERT(bWroteOK == TRUE);

					wxString resultLine = f.GetLine((size_t)1); // comma separated, in entry table LTR order
					//int fieldCount = 9; // id,srcLangName,tgtLangName,src,tgt,username,timestamp,type,deleted
					int fieldCount = 8; // srcLangName,tgtLangName,src,tgt,username,timestamp,type,deleted
					for (count = 1; count <= fieldCount; count++)
					{
						// Progressively shorten the resultLine as we extract each field
						// (the final field should have a terminating comma, so number of
						// commas should equal number of fields)
						offset = resultLine.Find(comma);
						if (offset >= 0)
						{
							if (count == 1)
							{
								left = resultLine.Left(offset);
								m_entryStruct.srcLangName = left;
								resultLine = resultLine.Mid(offset + 1); // extracted srcLangName field
							}
							else if (count == 2)
							{
								left = resultLine.Left(offset);
								m_entryStruct.tgtLangName = left;
								resultLine = resultLine.Mid(offset + 1); // extracted tgtLangName field
							}
							else if (count == 3)
							{
								left = resultLine.Left(offset);
								m_entryStruct.source = left;
								resultLine = resultLine.Mid(offset + 1); // extracted source field
							}
							else if (count == 4)
							{
								left = resultLine.Left(offset);
								m_entryStruct.nonSource = left;
								resultLine = resultLine.Mid(offset + 1); // extracted nonSource field
							}
							else if (count == 5)
							{
								left = resultLine.Left(offset);
								m_entryStruct.username = left;
								resultLine = resultLine.Mid(offset + 1); // extracted username field
							}
							else if (count == 6)
							{
								left = resultLine.Left(offset);
								m_entryStruct.timestamp = left;
								resultLine = resultLine.Mid(offset + 1); // extracted timestamp field
							}
							else if (count == 7)
							{
								left = resultLine.Left(offset);
								m_entryStruct.type = left[0];
								resultLine = resultLine.Mid(offset + 1); // extracted type field
							}
							else
							{
								// This block is for the deleted flag (count = 8). Just in case there is no
								// final comma, do it differently
								int length = resultLine.Len();
								wxChar lastChar = resultLine[length - 1];
								if (lastChar == comma)
								{
									resultLine = resultLine.Left(length - 1);
								}
								m_entryStruct.deleted = resultLine[0]; // extracted deleted field
							}

						} // end of TRUE block for test: if (offset >= 0)
					} // end of for loop
					// tell the app class what happened, so that KbServer's functions can be used
					if (gbIsGlossing)
					{
						m_pApp->m_bGlossesLookupSucceeded = TRUE;
					}
					else
					{
						m_pApp->m_bAdaptationsLookupSucceeded = TRUE;
					}
				} // end of TRUE block for test: if (offset >= 0) i.e. "success" in top line
				else
				{
					// This failure may indicate that I did not put the substring
					// "success" somewhere in the top line of the returned .dat file
					// (an error on my part in the .py python code)
					// OR (much more likely)
					// The src/nonSrc pair are not in the user table - assume the latter
					if (gbIsGlossing)
					{
						m_pApp->m_bGlossesLookupSucceeded = FALSE;
					}
					else
					{
						m_pApp->m_bAdaptationsLookupSucceeded = FALSE;
					}

					//wxBell(); deprecate - LookupEntry() may fail often due to the src/nonSrc not being in the entry table yet
					//wxString msg = _T(" FileToEntryStruct(), did not find 'success' string in top line. Probably missing entry.");
					//m_pApp->LogUserAction(msg);
				}
			} // end of TRUE block for test: if (bOpened)
			else
			{
				// unlikely to fail
				wxBell();
				wxString msg = _T("Line %s,failed to open results file, KbServer.cpp");
				msg = msg.Format(msg, __LINE__);
				m_pApp->LogUserAction(msg);
				return FALSE;
			}
		} // end of TRUE block for test: if (bExists)
		else
		{
			// the results file does not exist in the execFolderPath's folder
			wxString msg = _("The results file, %s , is missing from the executable's folder");
			msg = msg.Format(msg, datFileName.c_str());
			wxString title = _("Results file is missing");
			wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(msg);
			return FALSE;
		}
#if defined( _DEBUG )
		wxString myType = m_entryStruct.type;
		wxString delFlag = m_entryStruct.deleted;
		wxLogDebug(_T("%s() line= %d , m_entryStruct: srcLang= %s, tgtLang= %s, src= %s, nonSrc= %s, user= %s, time= %s, type= %s, deleted= %s"),
			__FUNCTION__, __LINE__, m_entryStruct.srcLangName.c_str(), m_entryStruct.tgtLangName.c_str(),
			m_entryStruct.source.c_str(), m_entryStruct.nonSource.c_str(), m_entryStruct.username.c_str(),
			m_entryStruct.timestamp.c_str(), myType.c_str(), delFlag.c_str());
#endif
		return TRUE;
	} // end of TRUE block for test:
	  // if (m_pApp->m_bAdaptationsKBserverReady || m_pApp->m_bGlossesKBserverReady)
	else
	{
		// The KbServer adaptations and glosses instantions have not been done,
		// so clear relevant things pending instatiation or one or both
		// of app's pointers m_pKbServer[0] and m_pKbServer[1]
		ClearEntryStruct();
		if (gbIsGlossing)
		{
			m_pApp->m_bGlossesLookupSucceeded = FALSE;
		}
		else
		{
			m_pApp->m_bAdaptationsLookupSucceeded = FALSE;
		}
		return FALSE;
	}
}

// BEW 26Oct20, created. Populate local_kb_lines.dat from local KB
bool KbServer::PopulateLocalKbLines(const int funcNumber, CAdapt_ItApp* pApp,
	wxString& execPath, wxString& datFilename, wxString& sourceLanguage,
	wxString nonSourceLanguage)
{
	if (funcNumber != 9)
	{
		pApp->LogUserAction(_T(" PopulateLocalKbLines() in KbServer.cpp, Wrong funcNumber, not 9"));
		return FALSE;  // wrong enum value
	}
	int type = 0; // intialise
	CKB* pKB = NULL; // initialise
	if (gbIsGlossing)
	{
		type = 2;
		pKB = GetKB(type); // returns m_pApp->m_pKB; & tests KB ptr is ready
	}
	else
	{
		type = 1;
		pKB = GetKB(type); // returns m_pApp->m_pKB; & tests KB ptr is ready
	}
	wxASSERT(pKB != NULL);
	if (pApp->m_bIsKBServerProject || pApp->m_bIsGlossingKBServerProject)
	{
		bool bTellUser = !(pApp->KbAdaptRunning() || pApp->KbGlossRunning());
		if (bTellUser)
		{
			wxString msg;
			wxString title;
			title = _("Knowledge base sharing is (temporarily) disabled");
			msg = _("This is a shared knowledge base project, but you have disabled sharing.\nSharing must be enabled, and with a working connection to the remote server, before a bulk upload of knowledge base entries can be allowed.\nTo enable sharing again, click Controls For Knowledge Base Sharing... on the Advanced menu, and click the left radio button.");
			// whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
			pApp->m_bUserDlgOrMessageRequested = TRUE;
			wxMessageBox(msg, title, wxICON_INFORMATION | wxOK);
			return FALSE;
		}
	}
	// BEW 21Jun22 calculate a const datetime, so put in each entry line
	//wxString dateTimeNow = GetDateTimeNow(forXHTML); // from helpers.cpp  <- not needed, the .py does it

	// We now have a running pKB set to the type we want, whether for 
	// adaptations, or glosses; but since a give AI project can be currently
	// running with glossing KB active, or adaptations KB active, we need to
	// set nonScrLanguage to whatever is the correct (ie. currently active) one
	wxString srcLanguage = sourceLanguage; // from signature
	wxString nonSrcLanguage = nonSourceLanguage; // from signature
	wxString comma = _T(',');
	wxString newline = _T('\n'); // needed for appending to each entryLine for the wxFile collection

	//wxTextFile text_f;
	// BEW added 5Jul22, next bit, because proper utf8 conversion and saving to a file requires wxFile, wxTextFile is incompatible
	wxFile utf8f; 
	//wxString strEntryTableData = _T("EntryTableData"); // use for the wxTextFile
	//wxString entryTablePath = execPath + strEntryTableData; // path to the EntryTableData file

	wxString datPath = execPath + datFilename; // execPath passed in, ends with PathSeparator char
	bool bFileExists = ::wxFileExists(datPath); // looking for path to local_kb_lines.dat file
	//bool bEntryTableFileExists = ::wxFileExists(entryTablePath); // looking for path to EntryTableData file, BEW 5Jul22
	
	if (bFileExists)
	{
		// delete the file and recreate
		bool bDeleted = wxRemoveFile(datPath);
		if (bDeleted)
		{
			bFileExists = utf8f.Create(datPath,wxFile::write); // for write, created empty - 
													// accumulates for local_kb_lines.dat
		}
		wxASSERT(bFileExists == TRUE);
	}
	else
	{
		// create the empty file in the execPath folder
		bFileExists = utf8f.Create(datPath, wxFile::write); // accumulates for local_kb_lines.dat
		wxASSERT(bFileExists == TRUE);
	}
	// BEW 5Jul22 added this test and if/else blocks
	/*
	if (bEntryTableFileExists)
	{
		// delete the file and recreate
		bool bDeleted = wxRemoveFile(entryTablePath);
		if (bDeleted)
		{
			bEntryTableFileExists = text_f.Create(entryTablePath); // wxTextFile, for read or write, created empty
		}
		wxASSERT(bFileExists == TRUE);
	}
	else
	{
		// create the empty EntryTableData file in the execPath folder
		bEntryTableFileExists = text_f.Create(entryTablePath); // for read or write, created empty
		wxASSERT(bEntryTableFileExists == TRUE);
	}
	*/
	CBString bstr; bstr.Empty(); // initialise
	wxString src = wxEmptyString;  // initialise these two scratch variables
	wxString nonSrc = wxEmptyString;
	// build each "srcLangName,nonSrcLangLine,src,nonSrc,username,type,deleted" in entryLine
	wxString entryLine = wxEmptyString;
	wxString placeholder = _T("...");

	// Set up the for loop that get's each pRefString's translation text, and bits of it's metadat
	CTargetUnit* pTU;
	//size_t numMapsInUse = pKB->m_nMaxWords; ,_ nope, defaults to 1, need to get it calculated 
	// without doing adapting or glossing
	size_t numMapsInUse = (size_t)pKB->GetMaxMapsInUse(); // <= 10, mostly maps 1 to 5, but can be higher
	size_t index;
	for (index = 0; index < numMapsInUse; index++)
	{
		MapKeyStringToTgtUnit* pMap = pKB->m_pMap[index];
		MapKeyStringToTgtUnit::iterator iter;
		for (iter = pMap->begin(); iter != pMap->end(); ++iter)
		{
			src = iter->first;
			// Don't send any placeholder entries
			if (src == placeholder)
			{
				continue;
			}
			pTU = iter->second;
			entryLine = wxEmptyString; // clear to empty
			TranslationsList* pTranslations = pTU->m_pTranslations;

			if (pTranslations->IsEmpty())
			{
				continue;
			}
			TranslationsList::Node* tpos = pTranslations->GetFirst();
			CRefString* pRefStr = NULL;
			CRefStringMetadata* pMetadata = NULL;
			pMetadata = pMetadata; // avoid gcc warning set but not used
			while (tpos != NULL)
			{
				pRefStr = (CRefString*)tpos->GetData();
				wxASSERT(pRefStr != NULL);
				tpos = tpos->GetNext();
				// Now get the information we want from pRefString
				// and it's metadata
				pMetadata = pRefStr->GetRefStringMetadata();

				entryLine = srcLanguage + comma + nonSrcLanguage + comma + src + comma;
				wxString nonSrc = pRefStr->m_translation; // use 'as is', 
				// BEW 27Jun22, if nonSrc is an empty string, instead of _T("<no adaptation>")
				// then play safe - don't send that entry to the remote entry table - it may
				// cause error
				if (nonSrc.IsEmpty())
				{
					continue;
				}
				entryLine += nonSrc + comma;

				// Next, the user - whoever created that entry in the local KB
				// BEW 7Apr22, getting the user string from the metadata would get
				// something like this "bwaters:BWATERS-XPS", which is unhelpfully
				// long. Better to use the app->m_strUserID username - it's likely to be
				// short, and is the user whose project this is
				//wxString user = pMetadata->GetWhoCreated();
				wxString user = m_pApp->m_strUserID;
				entryLine += user + comma;

				// Add the datetime  (BEW added 21Jun22)
				//entryLine += dateTimeNow + comma;  // No no! do_upload_local_create.py calculates
				// a constant datetime, immediately after the connection to mariaDB/kbserver has
				// been created; and it is inserted into the line to be inserted  in the .py code.
				// Doing it here makes wxExecute() fail.

				// the kbtype as a string  of length 1
				wxString strKbType = wxEmptyString;
				wxItoa(type, strKbType);
				entryLine += strKbType + comma;

				// finally, the deleted flag value, as a string of length 1
				bool bDeleted = pRefStr->GetDeletedFlag();
				if (bDeleted)
				{
					entryLine += _T('1');
				}
				else
				{
					// not deleted
					entryLine += _T('0');
				}
				// Must finish with a comma, otherwise the do_update_local_kb.exe will
				// not parse the flag correctly, but run on into stale buffer text, and
				// cause the .exe to fail
				entryLine += comma;
#if defined (_DEBUG)
				wxLogDebug(_T("%s() line= %d , entryLine= %s"), __FUNCTION__, __LINE__, entryLine.c_str());
#endif
				// Note, we don't grab any of the local timestamps, because the 
				// timestamp that kbserver wants is the time at which the bulk upload
				// is done. So we calculate that timestamp once in the python, and add
				// it to the being-uploaded line of each entry for the entry table 
				// in a single bulk upload call. That's because ChangedSince_Timed()
				// protocol must work with the time at which the data got inserted
				// into the remote kbserver's entry table.

				// Escape any single quotes in the data fields
				entryLine = DoEscapeSingleQuote(entryLine);
				
				// write out the entry line, for the wxTextFile "EntryTableData", no conversion to utf8
				if (!entryLine.IsEmpty())
				{
					// wxTextFile is okay for UTF16 strings, but is incompatible with wxFile, and doing
					// a conversion to utf8 would give corrupt utf8
					//text_f.AddLine(entryLine); // comment out later when working right
					//text_f.Write();            // comment out later when working right

					// BEW 5Jul22 now handle the wxFile data, for compiling into local_kb_lines.dat as utf8
					// (entryLine needs a '\n' added, otherwise, the .dat file isn't properly line-sequenced)
					entryLine += newline;
					bstr = ConvertToUtf8(entryLine);
					DoWrite(utf8f, bstr); // defined in XML.cpp
				}

			}
		} // end of inner for loop (for ref strings && their metadata): 
		  // for (iter = pMap->begin(); iter != pMap->end(); ++iter)
	} // end of outer for loop: for (index = 0; index < numMapsInUse; index++)
	//text_f.Close();
	// BEW 5Jul22 also flush and close the wxFile instance where the utf8 is stored in local_kb_lines.dat
	utf8f.Flush();
	utf8f.Close();
	return TRUE;
}
//=============================== end of KbServer class ============================

//#endif // for _KBSERVER

