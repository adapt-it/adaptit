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

#if defined(_KBSERVER)

#include <curl/curl.h>

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


using namespace std;
#include <string>

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
#include "KbServer.h"
#include "md5_SB.h"
#include "KBSharingMgrTabbedDlg.h"


WX_DEFINE_LIST(DownloadsQueue);
WX_DEFINE_LIST(UploadsList);  // for use by Thread_UploadMulti, for KBserver support
							  // (see member m_uploadsList)
WX_DEFINE_LIST(UsersListForeign);    // for use by the ListUsers() client, stores KbServerUserForeign structs
WX_DEFINE_LIST(KbsList);    // for use by the ListKbs() client, stores KbServerKb structs
//WX_DEFINE_LIST(LanguagesList);    // for use by the ListKbs() client, stores KbServerKb structs ??
WX_DEFINE_LIST(FilteredList); // used in page 3 tab of Shared KB Manager, the LoadLanguagesListBox() call

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
	m_queue.clear();
	// The following English messages are hard-coded at the server end, so don't localize
	// them, they are returned for certain error conditions and we'll want to know when
	// one has been returned
	//m_noEntryMessage = _T("No matching entry found");
	//m_existingEntryMessage = _T("Existing matching entry found");
	m_bForManager = FALSE;
}

KbServer::KbServer(int whichType)
{
	// This is the constructor we should normally use for sharing of KBs
	wxASSERT(whichType == 1 || whichType == 2);
	m_kbServerType = whichType;
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	m_pKB = GetKB(m_kbServerType);
	m_queue.clear();
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
	m_queue.clear();
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

// BEW 7Sep20 legacy getters for codes, Life or xhtml may need these
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
/* deprecate - it's part of the JM solution no longer needed
void KbServer::ExtractHttpStatusEtc(std::string s, int& httpstatuscode, wxString& httpstatustext)
{

	// if no headers are returned, then assume the lookup failed with a 404 "Not Found"
	// error, and bleed that out first
	if (s.empty())
	{
		httpstatuscode = 404;
		httpstatustext = _T("Not Found");
		return;
	}
	// make the standard string into a wxString
	httpstatuscode = 200; // initialize to OK
	httpstatustext.Empty(); // initialize
	CBString cbstr(s.c_str());
	wxString buffer(ToUtf16(cbstr));

	//there are two HTTP lines, the first has to do with authentication, we
    // want the second one which will follow the  first "Content-Type" line.
    wxString srchStr2 = _T("Content-Type");
	int length2 = srchStr2.Len();
	int offset2 = buffer.Find(srchStr2);
	wxASSERT(offset2 != wxNOT_FOUND);
	buffer = buffer.Mid(offset2 + length2); // bleed off the earlier stuff

    // now search for the line we want
	wxString srchStr = _T("HTTP/1.1 ");
	int length = srchStr.Len();
	int offset = buffer.Find(srchStr);
	if (offset == wxNOT_FOUND)
	{
        // if we couldn't find it, return 200 and "HTTP status code absent" and keep truckin'
        // (this should never happen)
        httpstatuscode = 200;
        httpstatustext = _T("HTTP status code absent");
        return;
	}
	else
	{
		// throw away everything preceding the matched string
		buffer = buffer.Mid(offset + length);

		// buffer now commences with the HTTP 1.1 status code we want to extract - extract
		// everything up to the next space;
		int offset_to_space = buffer.Find(_T(' '));
		wxASSERT(offset_to_space != wxNOT_FOUND);
        wxString strValue = buffer.Left(offset_to_space);
		httpstatuscode = wxAtoi(strValue);

        // now we have extracted the http status code, extract the human readable text
        // following it
        buffer = buffer.Mid(offset_to_space + 1); // +1 for the space
		int offset_to_CR = buffer.Find(_T('\r'));
		int offset_to_LF = buffer.Find(_T('\n'));
		offset = wxMin(offset_to_CR, offset_to_LF);
		httpstatustext = buffer.Left(offset);
	}
	
}
*/

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

// callback functions for curl

size_t curl_read_data_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	userdata = userdata; // avoid "unreferenced formal parameter" warning
#if defined(SYNC_LOGS)
	//wxString msg;
	//msg = msg.Format(_T("In curl_read_data_callback: sending %d bytes."),(size*nmemb));
	//wxLogDebug(msg);
#endif
	str_CURLbuffer.append((char*)ptr, size*nmemb);
	return size*nmemb;
}

size_t curl_read_data_callback2(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	int threadIndex = *(int*)userdata;
	str_CURLbuff[threadIndex].append((char*)ptr, size*nmemb);
//#if defined(SYNC_LOGS)
//	wxString msg;
//	msg = msg.Format(_T("In curl_read_data_callback2: sending %d bytes, threadIndex = %d"),
//						(size*nmemb), threadIndex);
//	wxLogDebug(msg);
//#endif
	return size*nmemb;
}

// This next one is dedicated to the thread which, in loop, deletes one KB entry at a 
// time, and then the KB definition when no more entries are in the entry table for that KB
size_t curl_read_data_callback_for_deletekb(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	userdata = userdata; // avoid "unreferenced formal parameter" warning
	//wxString msg;
	//msg = msg.Format(_T("In curl_read_data_callback_for_deletekb: sending %d bytes."),(size*nmemb));
	//wxLogDebug(msg);
	str_CURLbuffer_for_deletekb.append((char*)ptr, size*nmemb);
	return size*nmemb;
}


size_t curl_update_callback(void* ptr, size_t size, size_t nitems, void* userp)
{
	// userp will be the pointer to Kevin's std:string object, strCURLbuffer
	size = 1;
	nitems = (*((std::string*)userp)).length();
	ptr = (void*)(*((std::string*)userp)).c_str();
	return nitems; // strictly speaking, nitems*size, but size is 1
}

// BEW added 9Feb12
size_t curl_headers_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	userdata = userdata; // avoid "unreferenced formal parameter" warning
	//wxString msg;
	//msg = msg.Format(_T("In curl_headers_callback: sending %s bytes."),(size*nmemb));
	//wxLogDebug(msg);
	str_CURLheaders.append((char*)ptr, size*nmemb);
	return size*nmemb;
}

// return the CURLcode value, downloaded JSON data is extracted and stored in the private
// arrays for that purpose, and other classes can obtain it using public getters which
// each return a pointer to a single one of the arrays
// BEW 28Feb13, currently, this function is not used, so until we need it, it is commented out
/*
int KbServer::LookupEntriesForSourcePhrase( wxString wxStr_SourceEntry )
{
	str_CURLbuffer.clear(); // always make sure it is cleared for accepting new data

	CURL *curl;
	CURLcode result;
	wxString aUrl; // convert to utf8 when constructed
	wxString aPwd; // ditto

	CBString charUrl;
	CBString charUserpwd;

	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString kbType;
	wxItoa(GetKBServerType(),kbType);
	wxString container = _T("entry");

	// The URL has to be url-encoded for safety
	char *encodedsource = NULL;	// Encoded source string
	// url-encode  sourcePhrase   before appending
	// to charUrl within the if (curl) block below
	CBString utf8Src = ToUtf8(sourcePhrase); // could be a phrase, so it needs to be url-encoded
	char* pUtf8Src = (char*)utf8Src; // need in this form for the curl_easy_escape() call below

	aUrl = GetKBServerURL() + slash + container + slash+ GetSourceLanguageCode() +
			slash + GetTargetLanguageCode() + slash + kbType + slash;
	charUrl = ToUtf8(aUrl);
	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	curl = curl_easy_init();

	if (curl) {
		encodedsource = curl_easy_escape(curl, pUtf8Src, strlen(pUtf8Src));
		strcat(charUrl, encodedsource);	// Append encoded source to URL

		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);
		//curl_easy_setopt(curl, CURLOPT_WRITEDATA, str_CURLbuffer); // <-- not needed
		//curl_easy_setopt(curl, CURLOPT_HEADER, 1L); // comment out when header info is
													  //not needed in the download (and below)
		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) // && defined (__WXGTK__)
        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif

		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("LookupEntryForSourcePhrase()() result code: %d Error: %s"),
				result, error.c_str());
			wxMessageBox(msg, _T("Error when looking up for SourcePhrase"), wxICON_EXCLAMATION | wxOK);

			str_CURLbuffer.clear(); // always clear it before returning
			curl_easy_cleanup(curl);
			curl_free(encodedsource);
			return (int)result;
		}
	}
	curl_easy_cleanup(curl);

	//  make the json data accessible (result is CURLE_OK if control gets to here)
	if (!str_CURLbuffer.empty())
	{
        // Before extracting the substrings from the JSON data, the storage arrays must be
        // cleared with a call of ClearAllPrivateStorageArrays()
		ClearAllPrivateStorageArrays(); // always must start off empty

		wxString myList = wxString::FromUTF8(str_CURLbuffer.c_str());
		wxJSONValue jsonval;
		wxJSONReader reader;
		int numErrors = reader.Parse(myList, &jsonval);
		if (numErrors > 0)
		{
			// a non-localizable message will do, it's unlikely to ever be seen
			wxMessageBox(_T("LookupEntriesForSourcePhrase(): reader.Parse() returned errors, so will return wxNOT_FOUND"),
				_T("KBserver error"), wxICON_ERROR | wxOK);
			str_CURLbuffer.clear(); // always clear it before returning
			curl_free(encodedsource);
			return -1;
		}
		int listSize = jsonval.Size();
		int index;
		for (index = 0; index < listSize; index++)
		{
            // We can extract id, source phrase, target phrase, deleted flag value,
            // username, and timestamp string; but for a lookup supporting a single
            // CSourcePhrase, only we need to extract source phrase, target phrase, and the
            // value of the deleted flag. So the others can be commented out.
            // BEW 12Jan13, we should also get the username - to track originator of the entry
			m_arrSource.Add(jsonval[index][_T("source")].AsString());
			m_arrTarget.Add(jsonval[index][_T("target")].AsString());
			m_arrDeleted.Add(jsonval[index][_T("deleted")].AsInt());
			//m_arrID.Add(jsonval[index][_T("id")].AsLong());
			m_arrUsername.Add(jsonval[index][_T("user")].AsString());
			//m_arrTimestamp.Add(jsonval[index][_T("timestamp")].AsString());
		}
		str_CURLbuffer.clear(); // always clear it before returning
		curl_free(encodedsource);
	}

	return 0;
}
*/
// return the CURLcode value, downloaded JSON data is extracted and stored in the private
// arrays for that purpose, and other classes can obtain it using public getters which
// each return a pointer to a single one of the arrays.
//
// If the data download succeeds, the 'last sync' timestamp is extracted from the headers
// information and stored in the private member variable: m_kbServerLastTimestampReceived
// If there is a subsequent error - such as the JSON data extraction failing, then -1 is
// returned and the timestamp value in m_kbServerLastTimestampReceived should be
// disregarded -- because we only transfer the timestamp from there to the private member
// variable: m_kbServerLastSync, if there was no error (ie. returned code was 0). If 0 is
// returned then in the caller we should merge the data into the local KB, and use the
// public member function UpdateLastSyncTimestamp() to move the timestamp from
// m_kbServerLastTimestampReceived into m_kbServerLastSync; and then use the public member
// function ExportLastSyncTimestamp() to export that m_kbServerLastSync value to persistent
// storage. (Of course, the next successful ChangedSince() call will update what is stored
// in persistent storage; and the timeStamp value used for that call is whatever is
// currently within the variable m_kbServerLastSync).

/* BEW 21Oct20 removed, it just complicates things, and is not needed any more
int KbServer::ChangedSince(wxString timeStamp)
{
	CStatusBar* pStatusBar = NULL;
	pStatusBar = (CStatusBar*)m_pApp->GetMainFrame()->m_pStatusBar;
	pStatusBar->StartProgress(_("Receiving..."), _("Receiving..."), 5);

	str_CURLbuffer.clear(); // always make sure it is cleared for accepting new data
	str_CURLheaders.clear(); // BEW added 9Feb13

	pStatusBar->UpdateProgress(_("Receiving..."), 1);

	CURL *curl;
	CURLcode result = CURLE_OK; // initialize to a harmless value
	wxString aUrl; // convert to utf8 when constructed
	wxString aPwd; // ditto

	CBString charUrl;
	CBString charUserpwd;

	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString kbType;
	int type = GetKBServerType();
	wxItoa(type,kbType);
	wxString langcode;
	if (type == 1)
	{
		langcode = GetTargetLanguageCode();
	}
	else
	{
		langcode = GetGlossLanguageCode();
	}
	wxString container = _T("entry");
	wxString changedSince = _T("/?changedsince=");

	aUrl = GetKBServerIpAddr() + slash + container + slash+ GetSourceLanguageCode() + slash +
			langcode + slash + kbType + changedSince + timeStamp;
#if defined (SYNC_LOGS) //&& defined (__WXGTK__)
	wxLogDebug(_T("ChangedSince(): wxString aUrl = %s"), aUrl.c_str());
#endif
	charUrl = ToUtf8(aUrl);
	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);

		// We want the download's timestamp, so we must ask for the headers to be added
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &curl_headers_callback); // BEW added 9Feb13

		pStatusBar->UpdateProgress(_("Receiving..."), 2);
		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) // && defined (__WXGTK__)
        // BEW added 9Feb13, check what, if anything, got added to str_CURLheaders_callback
        CBString s2(str_CURLheaders.c_str());
        wxString showit2 = ToUtf16(s2);
        wxLogDebug(_T("Returned headers: %s"), showit2.c_str());

        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif
        if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("ChangedSince() result code: %d Error: %s"),
				result, error.c_str());
			wxMessageBox(msg, _T("Error when downloading entries"), wxICON_EXCLAMATION | wxOK);

            curl_easy_cleanup(curl);

            str_CURLbuffer.clear(); // always clear it before returning
            str_CURLheaders.clear(); // BEW added 9Feb13
            pStatusBar->FinishProgress(_("Receiving..."));
            return (int)result;
        }
	}
	// no CURL error, so continue...
	curl_easy_cleanup(curl);

	pStatusBar->UpdateProgress(_("Receiving..."), 3);

	// Extract from the headers callback, the HTTP code, and the X-MySQL-Date value, the
	// HTTP status information, and the payload's content-length value (as a string)
    ExtractTimestamp(str_CURLheaders, m_kbServerLastTimestampReceived);
    ExtractHttpStatusEtc(str_CURLheaders, m_httpStatusCode, m_httpStatusText);

#if defined (SYNC_LOGS) // && defined (__WXGTK__)
        // show what ExtractHttpStatusEtc() returned
        CBString s2(str_CURLheaders.c_str());
        wxString showit2 = ToUtf16(s2);
        wxLogDebug(_T("From headers: Timestamp = %s , HTTP code = %d , HTTP msg = %s"),
                   m_kbServerLastTimestampReceived.c_str(), m_httpStatusCode,
                   m_httpStatusText.c_str());
#endif

	//  Make the json data accessible (result is CURLE_OK if control gets to here)
	//
	//  BEW 29Jan13, beware, if no new entries have been added since last time, then
	//  the payload will not have a json string, and will have an 'error' string
    //  "No matching entry found". This isn't actually an error, and ChangedSince(), in
    //  this circumstance should just benignly exit without doing or saying anything, and
    //  return 0 (CURLE_OK)
	if (!str_CURLbuffer.empty())
	{
        // json data beginswith "[{", so test for the payload starting this way, if it
        // doesn't, then there is only an error string to grab -- quite possibly
        // "No matching entry found", in which case, don't do any of the json stuff, and
        // the value in m_kbServerLastTimestampReceived will be correct and can be used
        // to update the persistent storage file for the time of the lastsync
		std::string srch = "[{";
		size_t offset = str_CURLbuffer.find(srch);

		// not npos means JSON data starts somewhere; a JSON parse ignores all before [ or {
		if (offset != string::npos)
		{
            // Before extracting the substrings from the JSON data, the storage arrays must be
            // cleared with a call of ClearAllPrivateStorageArrays()
            ClearAllPrivateStorageArrays(); // always must start off empty

            CBString jsonArray(str_CURLbuffer.c_str()); // JSON expects byte data, convert after Parse()
            wxJSONValue jsonval;
            wxJSONReader reader;
			// The Parse() function will convert any \uNNNN representations of unicode
			// characters to valid UTF-8 byte sequences. Conversion to UTF-16 or UCS-4
			// is done further below, when extracting strings from jsonval
			int numErrors = reader.Parse(ToUtf16(jsonArray), &jsonval);

            pStatusBar->UpdateProgress(_("Receiving..."), 4);
            if (numErrors > 0)
            {
				// Write to a file, in the _LOGS_EMAIL_REPORTS folder, whatever was sent,
				// the developers would love to have this info. The latest copy only is
				// retained, the "w" mode clears the earlier file if there is one, and
				// writes new content to it
				wxString aFilename = _T("ReceiveAllButton_bad_data_sent_to ") + m_pApp->m_curProjectName + _T(".txt");
				wxString workOrCustomFolderPath;
				if (::wxDirExists(m_pApp->m_logsEmailReportsFolderPath))
				{
					wxASSERT(!m_pApp->m_curProjectName.IsEmpty());

					if (!m_pApp->m_bUseCustomWorkFolderPath)
					{
						workOrCustomFolderPath = m_pApp->m_workFolderPath;
					}
					else
					{
						workOrCustomFolderPath = m_pApp->m_customWorkFolderPath;
					}
					wxString path2BadData = workOrCustomFolderPath + m_pApp->PathSeparator +
						m_pApp->m_logsEmailReportsFolderName + m_pApp->PathSeparator + aFilename;
                    wxString mode = _T('w');
					size_t mySize = str_CURLbuffer.size();
					wxFFile ff(path2BadData.GetData(), mode.GetData());
					wxASSERT(ff.IsOpened());
					ff.Write(str_CURLbuffer.c_str(), mySize);
					ff.Close();
				}

                // a non-localizable message will do, it's unlikely to ever be seen
				wxString msg;
				msg = msg.Format(_T("ChangedSince(): json reader.Parse() failed. Server sent bad data.\nThe bad data is stored in the file with name: \n%s \nLocated at the folder: %s \nSend this file to the developers please."),
					aFilename.c_str(), m_pApp->m_logsEmailReportsFolderPath.c_str());
				wxMessageBox(msg, _T("KBserver error"), wxICON_ERROR | wxOK);

				str_CURLbuffer.clear(); // always clear it before returning
                str_CURLheaders.clear(); // always clear it before returning

                pStatusBar->FinishProgress(_("Receiving..."));
                return -1;
            } // end of TRUE block for test: if (numErrors > 0)
			// There were no errors
            size_t arraySize = jsonval.Size();
#if defined (SYNC_LOGS)
            // get feedback about now many entries we got
            if (arraySize > 0)
            {
                wxLogDebug(_T("ChangedSince() returned %d entries, for data added to KBserver since %s"),
                    arraySize, timeStamp.c_str());
            }
#endif
            size_t index;
			wxString noform = _T("<noform>");
			wxString s;
            for (index = 0; index < arraySize; index++)
            {
                // We can extract id, source phrase, target phrase, deleted flag value,
                // username, and timestamp string; but for supporting the sync of a local KB we
                // need only to extract source phrase, target phrase, and the value of the
                // deleted flag. So the others can be commented out.
                // BEW changed 16Jan13, to have the username included in the arrays, so that we
                // can track who originated each of the entries in the group's various local KBs
                m_arrSource.Add(jsonval[index][_T("source")].AsString());
				// BEW 11Jun15 restore <noform> to an empty string
				s = jsonval[index][_T("target")].AsString();
				if (s == noform)
				{
					s.Empty();
				}
				m_arrTarget.Add(s);
                m_arrDeleted.Add(jsonval[index][_T("deleted")].AsInt());
				m_arrUsername.Add(jsonval[index][_T("user")].AsString());
#if defined (SYNC_LOGS)
                // list what entries were returned
                wxLogDebug(_T("Downloaded:  %s  ,  %s  ,  deleted = %d"),
                    (m_arrSource[index]).c_str(), (m_arrTarget[index]).c_str(), (m_arrDeleted[index]));
#endif
            }
            pStatusBar->UpdateProgress(_("Receiving..."), 5);

            str_CURLbuffer.clear(); // always clear it before returning
            str_CURLheaders.clear(); // BEW added 9Feb13
		}
		else
		{
			// buffer contains an error message, such as "No matching entry found",
			// but we will ignore it - the HTTP error reporting will suffice.
			if (m_httpStatusCode == 404)
            {
                // A "Not Found" error. No new entries were sent, because there were none
				// more recent than the last saved timestamp value, so do nothing here
				// except update the lastsync timestamp, there's no point in keeping the
				// earlier value unchanged
                UpdateLastSyncTimestamp();
            }
            else
            {
				// Some other situation, report it in debug mode, and don't update the
				// lastsync timestamp. Most likely, there was no JSON data to send. This
				// isn't actually an error...
#if defined (SYNC_LOGS)
				wxString msg;
				msg  = msg.Format(_T("ChangedSince(): HTTP status: %d   Probably no JSON data was returned."),
                                  m_httpStatusCode);
                wxMessageBox(msg, _T("No useful data returned"), wxICON_EXCLAMATION | wxOK);
#endif
 				str_CURLbuffer.clear();
				str_CURLheaders.clear();
				pStatusBar->FinishProgress(_("Receiving..."));
				return (int)CURLE_OK;
           }
		}
	} // end of TRUE block for test: if (!str_CURLbuffer.empty())

    str_CURLbuffer.clear();
    str_CURLheaders.clear();
	pStatusBar->FinishProgress(_("Receiving..."));
	return (int)CURLE_OK;
}
*/
// Return the CURLcode value, downloaded JSON data is parsed, and merged entry by entry
// directly into the local KB, using a function: StoreOneEntryFromKBserver() run from
// within the JSON parsing loop. (We do this for speed. An earlier version read the 
// data into structs and stored in a queue (wxList), and OnIdle() then consumed the 
// queue, doing the mergers to the local KB. This was abysmally slow. And interrupted by
// next call of ChangedSince_XXXX() if the timer interval was less than 5 mins.) So,
// we will go for speed in this version of the changedsince protocol support.
// If the data download succeeds, the 'last sync' timestamp is extracted from the headers
// information and stored in the private member variable: m_kbServerLastTimestampReceived
// If there is a subsequent error - such as the JSON data extraction failing, then -1 is
// returned and the timestamp value in m_kbServerLastTimestampReceived should be
// disregarded -- because we only transfer the timestamp from there to the private member
// variable: m_kbServerLastSync, if there was no error (ie. returned code was 0). If 0 is
// returned then in the caller we should merge the data into the local KB, and use the
// public member function UpdateLastSyncTimestamp() to move the timestamp from
// m_kbServerLastTimestampReceived into m_kbServerLastSync; and then use the public member
// function ExportLastSyncTimestamp() to export that m_kbServerLastSync value to persistent
// storage. (Of course, the next successful ChangedSince_Queued() call will update what is
// stored in persistent storage; and the timeStamp value used for that call is whatever is
// currently within the variable m_kbServerLastSync). The timestamp is not stored in a
// configuration file at present, but in a small file in the project folder.

// BEW 17Oct20, I will keep the overall structure, as it has important calls, progress dialog
// support, and depositing of data in 7 parallel arrays. I'll just comment out or change as
// appropriate for Leon's solution. The legacy code is preseved in C:\_Debug Data folder, in
// a file called DlownloadToKB - legacy code.txt
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
	wxString execPath = m_pApp->execPath;
	int length = execPath.Len();
	wxChar lastChar = execPath.GetChar(length - 1);
	wxString strLastChar = wxString(lastChar);
	if (strLastChar != separator)
	{
		execPath += separator; // has path separator at end now
	}
	wxString distPath = execPath + _T("dist"); // always a child folder of execPath
	
	// put the timestamp passed in, into app storage so that ConfigureDATfile() can grab it
	m_pApp->m_ChangedSinceTimed_Timestamp = timeStamp;

	wxString resultsFilename = wxEmptyString;
	bool bConfiguredOK = m_pApp->ConfigureDATfile(changed_since_timed);
	if (bConfiguredOK)
	{
		wxString execFileName = _T("do_changed_since_timed.exe");
		resultsFilename = _T("changed_since_timed_return_results.dat");
		bool bReportResult = FALSE;
		bool bExecutedOK = m_pApp->CallExecute(changed_since_timed, execFileName, execPath,
			resultsFilename, 99, 99, bReportResult);
		wxUnusedVar(bExecutedOK);
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
	this->m_kbServerLastTimestampReceived = wxEmptyString; //initialise, the temporary storage

// TODO  get the results .dat file, open for reading, and get line count... 1st line special
	wxString datPath = execPath + resultsFilename;
	wxString aLine = wxEmptyString; // initialise
	bool bPresent = ::FileExists(datPath);
	if (bPresent)
	{
		// BEW 2Nov20, this is where we need to un-escape any escaped single quotes;
		// top line should have only "success" substring, no single quotes, so safe
		// to leave it in the file
		DoUnescapeSingleQuote(execPath, resultsFilename);  // from helpers.cpp

		// Now deal with the returned entry lines - whether a file, or the whole lot in
		// kbserver for the current project
		wxTextFile f;
		bool bIsOpened = FALSE;
		f.Create(datPath);
		bIsOpened = f.Open();
		if (bIsOpened)
		{
			aLine = f.GetFirstLine();
			wxString tstamp = ExtractTimestamp(aLine);
			if (!tstamp.IsEmpty())
			{
				this->m_kbServerLastTimestampReceived = tstamp; // sets the temporary storage
					// If UpdateLastSyncTimestamp() is called below, this value is made
					// semi-permanent (ie. saved to a file for later importing) if
					// bDoTimestampUpdate is TRUE (see below)
			}
			// Ignore the top line of the file's list from subsequent calls, it's no longer useful

			// Get the file's line count, our loops from here on now must start
			// at the line with index = 1.
			// Note, it's possible that ChangedSince_Timed() will return no data for
			// adding to the local KB, e.g. if time span is short and for some reason the
			// user is thinking rather than adapting, so that the timer trips but nothing is
			// newly added to the remote server. This is not an error situation, so don't
			// treat it like one. Just ignore and give no message back
			listSize = (unsigned int)f.GetLineCount();  // (f is not changed by removing a line
								// unless we write it and reopen, and I can't be bothered)
			if (listSize > 1)
			{
				// There is at least one line of entry table data in the file
				// Call: bool KbServer::DatFile2StringArray(wxString& execPath, 
				//           wxString& resultFile, wxArrayString& arrLines)
				wxArrayString arrLines;
				// If any error, a message will appear from internal code
				bool bGotThem = DatFile2StringArray(execPath, resultsFilename, arrLines);

				// **** Progress indicator, step 3 ****
				pStatusBar->UpdateProgress(_("Receiving..."), 3);
				// ***** Progress indicator *****

				if (bGotThem)
				{
					// listSize will be smaller, as the top line of f was not
					// added to the list. So recalculate it, otherwise a bounds
					// error will result.
					listSize = arrLines.GetCount();

					// loop over the lines
					for (index = 0; index < listSize; index++)
					{
						wxString aLine = arrLines.Item(index);
						bool bExtracted = Line2EntryStruct(aLine); // populates m_entryStruct
						if (bExtracted)
						{
							// We can extract id, source phrase, target phrase, deleted flag value,
							// username, and timestamp string; but for supporting the sync of a local
							// KB we need only to extract source phrase, target phrase, the value of
							// the deleted flag, and the username be included in the KbServerEntry
							// structs
							pEntryStruct = new KbServerEntry;
							(*pEntryStruct).id = m_entryStruct.id;
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

							/* leave, it may be useful later, or somewhere else
#if defined (_DEBUG)
							if (
								(m_entryStruct.source == _T("laikim") && m_entryStruct.nonSource == _T("love"))
								||
								(m_entryStruct.source == _T("laikim") && m_entryStruct.nonSource == _T("fond of"))
								||
								(m_entryStruct.source == _T("bikpela") && m_entryStruct.nonSource == _T("gross"))
								||
								(m_entryStruct.source == _T("gen") && m_entryStruct.nonSource == _T("a second time"))
								)
							{
								int halt_here = 4;
							}
#endif
							*/
							pKB->StoreOneEntryFromKbServer(pEntryStruct->source, pEntryStruct->nonSource,
								pEntryStruct->username, bDeletedFlag);

							KBAccessMutex.Unlock();

#if defined (SYNC_LOGS)
							// list what fields we extracted for each line of the entry table matched
//							wxLogDebug(_T("ChangedSince_Timed: Downloaded, and storing:  %s  ,  %s  ,  deleted = %d  ,  username = %s"),
//								pEntryStruct->source.c_str(), pEntryStruct->nonSource.c_str(),
//								pEntryStruct->deleted, pEntryStruct->username.c_str());
#endif
							/* don't need these, next setting of m_entryStruct will clear old contents first

							pEntryStruct->source.Clear();
							pEntryStruct->nonSource.Clear();
							pEntryStruct->username.Clear();
							*/

							delete pEntryStruct;

						} // end of TRUE block for test: if (bExtracted)
					} // end of for loop: for (index = 1; index < listSize; index++)
				} // end of TRUE block for test: if (bGotThem)
				arrLines.Clear();

				// **** Progress indicator, step 3 ****
				pStatusBar->UpdateProgress(_("Receiving..."), 4);
				// ***** Progress indicator *****

				// since all went successfully, update the lastsync timestamp, if requested
				// (when using ChangedSince_Queued() to download entries for deleting an entire
				// KB, we don't want any timestamp update done)
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

void KbServer::ClearStrCURLbuffer()
{
	str_CURLbuffer.clear();
}

void KbServer::ClearAllStrCURLbuffers2()
{
	int i;
	for (i=0; i < 50; i++)
	{
		str_CURLbuff[i].clear();
	}
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

void KbServer::DownloadToKB(CKB* pKB, enum ClientAction action)
{
	wxASSERT(pKB != NULL);
	int rv = 0; // rv is "return value", initialize it
	wxString timestamp;
    // whm aFeb2018 note: changed name of local curKey below to currKey to avoid
    // confusion with CPhraseBox's value (the curKey there was also named to m_CurKey)
    // But, see uses of the global that might relate to KbServer considerations in
    // ChooseTranslation.cpp's OnButtonRemove() handler - about line 1487
    // and ChooseTranslation.cpp's InitDialog() handler - about line 1567-1569.
    wxString currKey;  
	s_DoGetAllMutex.Lock();
	switch (action)
	{
	case getForOneKeyOnly:
        // I'll populate this case with minimal required code, but I'm not planning we ever
        // call this case this while the user is interactively adapting or glossing,
        // because it will slow the GUI response abysmally. Instead, we'll rely on the
        // occasional ChangedSince_Timed() calls getting the local KB populated more quickly.
		currKey = (m_pApp->m_pActivePile->GetSrcPhrase())->m_key;
		// *** NOTE *** in the above call, I've got no support for AutoCapitalization; if
		// that was wanted, more code would be needed here - or alternatively, use the
		// adjusted key from within AutoCapsLookup() which in turn would require that we
		// modify this function to pass in the adjusted string for currKey via
		// DownloadToKB's signature
// LookupEntriesForSourcePhrase()'s code is currently commented out because we don't use it
// and so I've commented this call out here; if we end up using it, then remove this
// commenting out
//		rv = LookupEntriesForSourcePhrase(currKey);
		if (rv != 0)
		{
			ClearAllPrivateStorageArrays(); // don't risk passing on possibly bogus values
		}
		// If the lookup succeeded, the private arrays will have been populated with the
		// one or more translation, or gloss, possibilities for this one source text key.
		// -- So someone would need to write a function to get that info to where it needs
		// to go -- presumably into an appropriate place in CAdapt_ItApp::AutoCapsLookup(),
		// and call it after DownloadToKB() has returned
		break;
	case changedSince:
		// get the last sync timestamp value
		timestamp = GetKBServerLastSync();
#if defined(SYNC_LOGS)
		wxLogDebug(_T("Doing ChangedSince_Timed() with lastsync timestamp value = %s"), timestamp.c_str());
#endif
		rv = ChangedSince_Timed(timestamp); // bool is default TRUE
		// if there was no error, update the m_kbServerLastSync value, and export it to
		// the persistent file in the project folder
		if (rv == 0)
		{
			UpdateLastSyncTimestamp();
		}
		break;
	case getAll:
		timestamp = _T("1920-01-01 00:00:00"); // earlier than everything!
		rv = ChangedSince_Timed(timestamp); // bool is default TRUE
		// if there was no error, update the m_kbServerLastSync value, and export it to
		// the persistent file in the project folder
		if (rv == 0)
		{
			UpdateLastSyncTimestamp();
		}
		break;
	}
	if (rv != 0)
	{
		// there was a cURL error, display it
		wxString msg;
		msg = msg.Format(_("Downloading, error code ( %d ) returned.  Nothing was downloaded, application continues.\nError code 6 may just mean a temporary problem, so try again."), rv);
		wxMessageBox(msg, _("Downloading entries to the knowledge base failed"), wxICON_ERROR | wxOK);
		m_pApp->LogUserAction(msg);
		s_DoGetAllMutex.Unlock();

		return;
	}
	// Merge the data received into the local KB (either to the glossingKB or adaptingKB,
	// depending on what pKB points at). This deserves a progress indicator if there are
	// more than 50 entries in each of the parallel arrays (if fewer, it would be too
	// quick for it to be worth the bother). Having the progress indicator will give the
	// user feedback for what otherwise might be an inexplicable delay in responsiveness
	// in typing into the phrase box
	if (action == changedSince || action == getAll)
	{
		// the lookup case needs, if we ever support it, which I doubt, to call this
		// function and if the lookup succeeds, to get the one or more adaptation or gloss
		// entries and the values of the deleted flag, from the private arrays using their
		// accessors directly (ie. GetTargetArray(), GetDeletedArray()) rather than
		// calling StoreEntriesFromKbServer() here, because the chosen value would make
		// its way into the KB when the phrase box moves on - so that's why we exclude
		pKB->StoreEntriesFromKbServer(this);
	}

	s_DoGetAllMutex.Unlock();
}

/*
// Note: before running ListLanguages(), ClearStrCURLbuffer() should be called,
// and always remember to clear str_CURLbuffer before returning.
// Note 2: don't rely on CURLE_OK not being returned for a lookup failure, CURLE_OK will
// be returned even when there is no entry in the database. It's the HTTP status codes we
// need to get.
// Returns 0 (CURLE_OK) if no error, or 22 (CURLE_HTTP_RETURNED_ERROR) if there was a
// HTTP error - such as no matching entry, or a badly formed request
int KbServer::ListLanguages(wxString username, wxString password)
{
	CURL *curl;
	CURLcode result;
	wxString aUrl; // convert to utf8 when constructed
	wxString aPwd; // ditto
	str_CURLbuffer.clear();
	str_CURLheaders.clear();

	CBString charUrl;
	CBString charUserpwd;

	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString container = _T("language");

	aUrl = GetKBServerIpAddr() + slash + container;
	charUrl = ToUtf8(aUrl);
	aPwd = username + colon + password;
	charUserpwd = ToUtf8(aPwd);

	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback); // writes to str_CURLbuffer
		// We want separate storage for headers to be returned, to get the HTTP status code
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &curl_headers_callback);

		//curl_easy_setopt(curl, CURLOPT_HEADER, 1L); // comment out when collecting
		//headers separately
		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) //&& defined (__WXGTK__)
		
		CBString s2(str_CURLheaders.c_str());
		wxString showit2 = ToUtf16(s2);
		wxLogDebug(_T("ListLanguages(): Returned headers: %s"), showit2.c_str());

		CBString s(str_CURLbuffer.c_str());
		wxString showit = ToUtf16(s);
		wxLogDebug(_T("ListLanguages() str_CURLbuffer has: %s    , The CURLcode is: %d"),
			showit.c_str(), (unsigned int)result);
		
#endif
		// Get the HTTP status code, and the English message
		ExtractHttpStatusEtc(str_CURLheaders, m_httpStatusCode, m_httpStatusText);

		// If the only error was a HTTP one, then result will contain CURLE_OK, in which
		// case the next block is skipped
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("ListLanguages() result code: %d cURL Error: %s"),
				result, error.c_str());
			wxMessageBox(msg, _T("Error when listing custom language code definitions"), wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(msg);
			curl_easy_cleanup(curl);
			return (int)result;
		}
	}
	curl_easy_cleanup(curl);

	// If there was a HTTP error (typically, it would be 404 Not Found, because the
	// username is not in the entry table yet, but 100 Bad Request may also be possible I
	// guess) then exit early, there won't be json data to handle if that was the case
	if (m_httpStatusCode >= 400)
	{
		// whether 400 or 404, return CURLE_HTTP_RETURNED_ERROR (ie. 22) to the caller
		ClearUserStruct();
		str_CURLbuffer.clear();
		str_CURLheaders.clear();
		return CURLE_HTTP_RETURNED_ERROR; // 22
	}

	//  Make the json data accessible (result is CURLE_OK if control gets to here)
	//  We requested separate headers callback be used, so str_CURLbuffer should only have
	//  the json string for the constructed list of languages. All 8000+ are returned, we'll
	//  filter out the unwanted ISO639 ones in the caller of ListLanguages(), since only
	//  the custom code definitions are to be shown to the user of the KB Sharing Manager tabbed dlg
	if (!str_CURLbuffer.empty())
	{
		// json data beginswith "[{", so test for the payload starting this way, if it
		// doesn't, then there is only an error string to grab -- quite possibly
		// "No matching entry found"
		std::string srch = "[{";
		size_t offset = str_CURLbuffer.find(srch);

		// not npos means JSON data starts somewhere; a JSON parse ignores all before [ or {
		if (offset != string::npos)
		{
			CBString jsonArray(str_CURLbuffer.c_str()); // JSON expects byte data, convert after Parse()

			wxJSONValue jsonval;
			wxJSONReader reader;
			int numErrors = reader.Parse(ToUtf16(jsonArray), &jsonval);
			if (numErrors > 0)
			{
				// Write to a file, in the _LOGS_EMAIL_REPORTS folder, whatever was sent,
				// the developers would love to have this info. The latest copy only is
				// retained, the "w" mode clears the earlier file if there is one, and
				// writes new content to it
				wxString aFilename = _T("ListLanguages_bad_data_sent_to ") + m_pApp->m_curProjectName + _T(".txt");
				wxString workOrCustomFolderPath;
				if (::wxDirExists(m_pApp->m_logsEmailReportsFolderPath))
				{
					wxASSERT(!m_pApp->m_curProjectName.IsEmpty());

					if (!m_pApp->m_bUseCustomWorkFolderPath)
					{
						workOrCustomFolderPath = m_pApp->m_workFolderPath;
					}
					else
					{
						workOrCustomFolderPath = m_pApp->m_customWorkFolderPath;
					}
					wxString path2BadData = workOrCustomFolderPath + m_pApp->PathSeparator +
						m_pApp->m_logsEmailReportsFolderName + m_pApp->PathSeparator + aFilename;
                    wxString mode = _T('w');
					size_t mySize = str_CURLbuffer.size();
					wxFFile ff(path2BadData.GetData(), mode.GetData());
					wxASSERT(ff.IsOpened());
					ff.Write(str_CURLbuffer.c_str(), mySize);
					ff.Close();
				}
				// a non-localizable message will do, it's unlikely to ever be seen
				wxString msg;
				msg = msg.Format(_T("ListLanguages(): json reader.Parse() failed. Server sent bad data.\nThe bad data is stored in the file with name: \n%s \nLocated at the folder: %s \nSend this file to the developers please."),
					aFilename.c_str(), m_pApp->m_logsEmailReportsFolderPath.c_str());
				wxMessageBox(msg, _T("KBserver error"), wxICON_ERROR | wxOK);

				str_CURLbuffer.clear(); // always clear it before returning
				str_CURLheaders.clear();
				return -1;
			} // end of TRUE block for test: if (numErrors > 0)

			// We extract all but timestamp: that is, id, description, user
			ClearLanguagesList(&m_languagesList); // deletes from the heap any KbServerLanguage structs still in m_languageList
			wxASSERT(m_languagesList.empty());
			size_t arraySize = jsonval.Size();
			wxASSERT(arraySize > 0);
			size_t index;
			wxString three = _T("-x-");
			KbServerLanguage* pLanguageStruct = NULL; // initialize
			wxString aCode;
			for (index = 0; index < arraySize; index++)
			{
				aCode = jsonval[index][_T("id")].AsString();
				if (aCode.Find(three) != wxNOT_FOUND)
				{
					// Contains -x- so we want this one
					pLanguageStruct = new KbServerLanguage;
					pLanguageStruct->code = aCode;
					pLanguageStruct->username = jsonval[index][_T("user")].AsString();
					pLanguageStruct->description = jsonval[index][_T("description")].AsString();
					// to get rid of thesepLanguageStruct pointers once their job is done, if not, memory will be leaked

					
					
					pLanguageStruct->timestamp = jsonval[index][_T("timestamp")].AsString();
					// Add the pLanguageStruct to the m_languagesList stored in the KbServer instance
					// which is this (Caller should only use the adaptations instance of KbServer)
					m_languagesList.Append(pLanguageStruct); // Caller must later use ClearLanguagesList()
					// to get rid of these pointers once their job is done, if not, memory will be leaked
					#if defined (SYNC_LOGS)
					// commented out, though it works, because it slows down the processing required to put the custom codes into the list box
					wxLogDebug(_T("ListLanguages(): code = %s , description = %s , username = %s , timestamp = %s"),
					pLanguageStruct->code.c_str(), pLanguageStruct->description.c_str(), pLanguageStruct->username.c_str(),
					pLanguageStruct->timestamp.c_str());
					#endif
					
					// Add the pLanguageStruct to the m_languagesList stored in the KbServer instance
					// which is this (Caller should only use the adaptations instance of KbServer)
					m_languagesList.Append(pLanguageStruct); // Caller must later use ClearLanguagesList()
				}
				else
				{
					continue;
				}
			} // end of loop: for (index = 0; index < arraySize; index++)

		} // end of TRUE block for test: if (offset != string::npos)
		else
		{
			// There was no JSON returned, so treat this the same as if the "-x-" entries
			// were filtered at server end, and there were none of them. That is, just
			// do nothing here. If Jonathan then were to actually do the filtering at the
			// server end, then no code change is needed here
			;
		}

		str_CURLbuffer.clear(); // always clear it before returning
		str_CURLheaders.clear();
	}
	return 0;
}
*/
/* BEW 31Jul20 deprecated legacy comment
// Note: before running ListUsers(), ClearStrCURLbuffer() should be called,
// and always remember to clear str_CURLbuffer before returning.
// Note 2: don't rely on CURLE_OK not being returned for a lookup failure, CURLE_OK will
// be returned even when there is no entry in the database. It's the HTTP status codes we
// need to get.
// Returns 0 (CURLE_OK) if no error, or 22 (CURLE_HTTP_RETURNED_ERROR) if there was a
// HTTP error - such as no matching entry, or a badly formed request
*/

int KbServer::ListUsers(wxString ipAddr, wxString username, wxString password, wxString whichusername)
{
	bool bReady = FALSE;
	bool bAllowAccess = m_pApp->m_bHasUseradminPermission; // defaulted to FALSE
	if (bAllowAccess)
	{
		// I have tested for insufficient permission - using app's public bool, m_bHasUserAdminPermission.
		// If that is TRUE, the code for executing ListUsers() can be called. However, since
		// an accessing attempt on the KB SharingManager will check that boolean too, getting to
		// this ListUsers, a further boolean m_bKBSharingMgtEntered suppresses any internal call of
		// LookupUser() when there has been successful entry to the manager. The user may want/need
		// to make calls to ListUsers() more than once while in the manager.

		// Prepare the .dat input dependency: "list_users.dat" file, into
		// the execPath folder, ready for the ::wxExecute() call below
		bReady = m_pApp->ConfigureDATfile(list_users); // arg is const int, value 3
		if (bReady)
		{
			// The input .dat file is now set up ready for do_list_users.exe
			wxString execFileName = _T("do_list_users.exe"); // this call does not use user2, just authenticates
			wxString execPath = m_pApp->execPath;
			wxString resultFile = _T("list_users_return_results.dat");
			bool bExecutedOK = m_pApp->CallExecute(list_users, execFileName, execPath, resultFile, 32, 33);
			if (!bExecutedOK)
			{
				// error in the call, inform user, and put entry in LogUserAction() - English will do
				wxString msg = _T("Line %d: CallExecute for enum: list_users, failed - no match match in the user table; is kbserver turned on? Adapt It will continue working. ");
				msg = msg.Format(msg, __LINE__);
				wxString title = _T("Probable do_list_users.exe error");
				wxMessageBox(msg,title,wxICON_WARNING | wxOK);
				m_pApp->LogUserAction(msg);
			}
		}
	} // end of TRUE block for test: if (bAllowAccess)

	// TODO preprocess for setting up .dat input file, call Leon's .exe, post-process 
	// to get the user structs list
	// I've done a "LoadUSersListBox(....) at 832++ & ConvertLinesToUserStructs(), 5978++ etc

	return 0;
}

// Note: before running ListKbs(), ClearStrCURLbuffer() should be called,
// and always remember to clear str_CURLbuffer before returning.
// Note 2: don't rely on CURLE_OK not being returned for a lookup failure, CURLE_OK will
// be returned even when there is no entry in the database. It's the HTTP status codes we
// need to get.
// Returns 0 (CURLE_OK) if no error, or 22 (CURLE_HTTP_RETURNED_ERROR) if there was a
// HTTP error - such as no matching entry, or a badly formed request
int KbServer::ListKbs(wxString username, wxString password)
{

// TODO  Leon's solution...

/* legacy solution
	CURL *curl;
	CURLcode result;
	wxString aUrl; // convert to utf8 when constructed
	wxString aPwd; // ditto
	str_CURLbuffer.clear();
	str_CURLheaders.clear();

	CBString charUrl;
	CBString charUserpwd;

	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString container = _T("kb");

	aUrl = GetKBServerIpAddr() + slash + container;
	charUrl = ToUtf8(aUrl);
	aPwd = username + colon + password;
	charUserpwd = ToUtf8(aPwd);

	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback); // writes to str_CURLbuffer
		// We want separate storage for headers to be returned, to get the HTTP status code
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &curl_headers_callback);

		//curl_easy_setopt(curl, CURLOPT_HEADER, 1L); // comment out when collecting
													  //headers separately
		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) //&& defined (__WXGTK__)
        CBString s2(str_CURLheaders.c_str());
        wxString showit2 = ToUtf16(s2);
		wxLogDebug(_T("ListKbs(): Returned headers: %s"), showit2.c_str());

        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
		wxLogDebug(_T("ListKbs() str_CURLbuffer has: %s    , The CURLcode is: %d"),
					showit.c_str(), (unsigned int)result);
#endif
		// Get the HTTP status code, and the English message
		ExtractHttpStatusEtc(str_CURLheaders, m_httpStatusCode, m_httpStatusText);

		// If the only error was a HTTP one, then result will contain CURLE_OK, in which
		// case the next block is skipped
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("ListKbs() result code: %d cURL Error: %s"),
				result, error.c_str());
			wxMessageBox(msg, _T("Error looking up a shared KB definition"), wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(msg);
			m_httpStatusText.Clear(); // otherwise, a memory leak
			curl_easy_cleanup(curl);
			return (int)result;
		}
	}
	curl_easy_cleanup(curl);

	// If there was a HTTP error (typically, it would be 404 Not Found, because the
	// username is not in the entry table yet, but 100 Bad Request may also be possible I
	// guess) then exit early, there won't be json data to handle if that was the case
	if (m_httpStatusCode >= 400)
	{
		// whether 400 or 404, return CURLE_HTTP_RETURNED_ERROR (ie. 22) to the caller
		ClearKbStruct();
		str_CURLbuffer.clear();
		str_CURLheaders.clear();
		m_httpStatusText.Clear(); // otherwise, a memory leak
		return CURLE_HTTP_RETURNED_ERROR; // 22
	}

	//  Make the json data accessible (result is CURLE_OK if control gets to here)
	//  We requested separate headers callback be used, so str_CURLbuffer should only have
	//  the json string for the constructed json array of kb entries
	if (!str_CURLbuffer.empty())
	{
		CBString jsonArray(str_CURLbuffer.c_str()); // JSON expects byte data, convert after Parse()
		wxJSONValue jsonval;
		wxJSONReader reader;
		int numErrors = reader.Parse(ToUtf16(jsonArray), &jsonval);
		if (numErrors > 0)
		{
			// Write to a file, in the _LOGS_EMAIL_REPORTS folder, whatever was sent,
			// the developers would love to have this info. The latest copy only is
			// retained, the "w" mode clears the earlier file if there is one, and
			// writes new content to it
			wxString aFilename = _T("ListKbs_bad_data_sent_to ") + m_pApp->m_curProjectName + _T(".txt");
			wxString workOrCustomFolderPath;
			if (::wxDirExists(m_pApp->m_logsEmailReportsFolderPath))
			{
				wxASSERT(!m_pApp->m_curProjectName.IsEmpty());

				if (!m_pApp->m_bUseCustomWorkFolderPath)
				{
					workOrCustomFolderPath = m_pApp->m_workFolderPath;
				}
				else
				{
					workOrCustomFolderPath = m_pApp->m_customWorkFolderPath;
				}
				wxString path2BadData = workOrCustomFolderPath + m_pApp->PathSeparator +
					m_pApp->m_logsEmailReportsFolderName + m_pApp->PathSeparator + aFilename;
                wxString mode = _T('w');
                size_t mySize = str_CURLbuffer.size();
                wxFFile ff(path2BadData.GetData(), mode.GetData());
				wxASSERT(ff.IsOpened());
				ff.Write(str_CURLbuffer.c_str(), mySize);
				ff.Close();
			}

			// a non-localizable message will do, it's unlikely to ever be seen
			wxString msg;
			msg = msg.Format(_T("Listkbs(): json reader.Parse() failed. Server sent bad data.\nThe bad data is stored in the file with name: \n%s \nLocated at the folder: %s \nSend this file to the developers please."),
				aFilename.c_str(), m_pApp->m_logsEmailReportsFolderPath.c_str());
			wxMessageBox(msg, _T("KBserver error"), wxICON_ERROR | wxOK);

			str_CURLbuffer.clear(); // always clear it before returning
			str_CURLheaders.clear();
			m_httpStatusText.Clear(); // otherwise, a memory leak
			return -1;
		}
        // We extract everything: id, sourcelanguage, targetlanguage, type, username,
        // deleted flag value, and the timestamp at which the definition was added to the
        // kb table
		ClearKbsList(&m_kbsList); // deletes from the heap any KbServerKb structs still in m_kbsList
		wxASSERT(m_kbsList.empty());
        size_t arraySize = jsonval.Size();
		wxASSERT(arraySize > 0);
        size_t index;
		for (index = 0; index < arraySize; index++)
        {
			KbServerKb* pKbStruct = new KbServerKb;
			// Extract the field values, store them in pKbStruct
			pKbStruct->id = jsonval[index][_T("id")].AsLong();
			pKbStruct->sourceLanguageCode = jsonval[index][_T("sourcelanguage")].AsString();
				(jsonval[index][_T("sourcelanguage")].AsString()).Clear(); // don't leak it
			pKbStruct->targetLanguageCode = jsonval[index][_T("targetlanguage")].AsString();
				(jsonval[index][_T("targetlanguage")].AsString()).Clear(); // don't leak it
			pKbStruct->kbType = jsonval[index][_T("type")].AsLong();
			pKbStruct->username = jsonval[index][_T("user")].AsString();
				(jsonval[index][_T("user")].AsString()).Clear(); // don't leak it
			unsigned long val = jsonval[index][_T("deleted")].AsLong();
			pKbStruct->deleted = val == 1L ? TRUE : FALSE;
			pKbStruct->timestamp = jsonval[index][_T("timestamp")].AsString();
				(jsonval[index][_T("timestamp")].AsString()).Clear(); // don't leak it
			// Add the pKbStruct to the m_kbsList stored in the KbServer instance
			// which is this (Caller should only use the adaptations instance of KbServer)
			m_kbsList.Append(pKbStruct); // Caller must later use ClearKbsList() to get
									// rid of these pointers once their job is done, if not,
									// memory will be leaked
#if defined (SYNC_LOGS)
			wxLogDebug(_T("ListKbs(): id = %d , sourcelanguage = %s targetlanguage = %s , type = %d , user = %s , timestamp = %s"),
				pKbStruct->id , pKbStruct->sourceLanguageCode.c_str(), pKbStruct->targetLanguageCode.c_str(),
				pKbStruct->kbType , pKbStruct->username.c_str(), pKbStruct->timestamp.c_str());
#endif
		}

		str_CURLbuffer.clear(); // always clear it before returning
		str_CURLheaders.clear();
		m_httpStatusText.Clear(); // otherwise, a memory leak
	}
*/
	return 0;
}

/* BEW 31 Jul20 deprecated comment
// Note: before running LookupUser(), ClearStrCURLbuffer() should be called,
// and always remember to clear str_CURLbuffer before returning.
// Note 2: don't rely on CURLE_OK not being returned for a lookup failure, CURLE_OK will
// be returned even when there is no entry in the database. It's the HTTP status codes we
// need to get.
// Returns 0 (CURLE_OK) if no error, or 22 (CURLE_HTTP_RETURNED_ERROR) if there was a
// HTTP error - such as no matching entry, or a badly formed request
*/ 


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
	}

	// Prepare the .dat input dependency: "lookup_user.dat" file, into
	// the execPath folder, ready for the ::wxExecute() call below
	// BEW 24Aug20 NOTE - calling _T("do_user_lookup.exe") with an absolute path prefix
	// DOES NOT WORK! As in: wxString command = execPath + _T("do_user_lookup.exe")
	// ::wxExecute() returns the error string:  Failed to execute script ....
	// The workaround is to temporarily set the current working directory (cwd) to the
	// AI executable's folder, do the wxExecute() call on just the script filename, and
	// restore the cwd after it returns.
	bool bReady = m_pApp->ConfigureDATfile(lookup_user); // arg is const int, value 2
	if (bReady)
	{
		// The input .dat file is now set up ready for do_user_lookup.exe
		wxString execFileName = _T("do_user_lookup.exe");
		wxString execPath = m_pApp->execPath;
		wxString resultFile = _T("lookup_user_return_results.dat");
		bool bExecutedOK = m_pApp->CallExecute(lookup_user, execFileName, execPath, resultFile, 32, 33);
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
			;
// TODO  may not need a message - inner code will generate what user needs to see

		}
	} // end of TRUE block for test: if (bReady), if bReady is false, no lookup happens

#if defined (_DEBUG)
//	int halt_here = 1;
#endif
	return 0;
}
/* BEW 24Sep20 deprecated, we no longer have a kb table
// Note: I have coded LookupSingleKb() so that it does return a curlCode value, but
// actually, failure to find what we look up can happen in several ways -- there may be a
// curl error, a http error, a json data error, or none of those but rather the KB is not
// yet in the kb table of the server. So I've got a bMatchedKB param in the
// signature which will return a FALSE value if any of those errors occurs, or the KB
// entry is absent in the server. It is TRUE if there were no errors and the lookup
// succeeded. Because we don't want confusing HTTP to be shown to the user, a 404 result
// won't trigger an error message in the caller, because we'll instead return 0 (ie.
// CURLE_OK) and bMatchedKB FALSE. The caller should just look at the bMatchedKB value. A
// cURL error is a bit more important, so I will return that.
int KbServer::LookupSingleKb(wxString ipAddr, wxString username, wxString password,
					wxString srcLangName, wxString nonsrcLangName, int kbType, bool& bMatchedKB)
{
	bMatchedKB = FALSE; // initialize
	CURL *curl;
	CURLcode result;
	wxString aUrl; // convert to utf8 when constructed
	wxString aPwd; // ditto
	str_CURLbuffer.clear();
	str_CURLheaders.clear();

	CBString charUrl;
	CBString charUserpwd;

	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString container = _T("kb");

    // GDLC 25MAY16 Trim leading and trailing spaces (if any) from srcLangCode and tgtLangCode
    // Leaving them there can result in failure to find the matching KB in the KB Server
    srcLangCode.Trim();         // from right
    srcLangCode.Trim(false);    // from left
    tgtLangCode.Trim();
    tgtLangCode.Trim(false);

	aUrl = url + slash + container + slash + srcLangCode;
	charUrl = ToUtf8(aUrl);
	aPwd = username + colon + password;
	charUserpwd = ToUtf8(aPwd);

	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);
		// We want separate storage for headers to be returned, to get the HTTP status code
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &curl_headers_callback);

		//curl_easy_setopt(curl, CURLOPT_HEADER, 1L); // comment out when collecting
													//headers separately
		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) //&& defined (__WXGTK__)
        CBString s2(str_CURLheaders.c_str());
        wxString showit2 = ToUtf16(s2);
		wxLogDebug(_T("LookupSingleKb(): Returned headers: %s"), showit2.c_str());

        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
		wxLogDebug(_T("LookupSingleKb() str_CURLbuffer has: %s    , The CURLcode is: %d"),
					showit.c_str(), (unsigned int)result);
#endif
		// Get the HTTP status code, and the English message
		ExtractHttpStatusEtc(str_CURLheaders, m_httpStatusCode, m_httpStatusText);

		// If the only error was a HTTP one, then result will contain CURLE_OK, in which
		// case the next block is skipped; cURL errors should be seen by the user, because
		// they suggest some transmission problem that may need attention by an administrator
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_("Error when looking up a KB definition in the kb table. Result code: %d cURL error: %s"),
				result, error.c_str());
			wxMessageBox(msg, _("cURL error"), wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(msg);
			curl_easy_cleanup(curl);
			return (int)result;
		}
	}
	curl_easy_cleanup(curl);

	// If there was a HTTP error (typically, it would be 404 Not Found, because the
	// looked for KB is not in the kb table yet, but 100 Bad Request may also be possible I
	// guess) then exit early, there won't be json data to handle if that was the case;
	// bMatchedKB is still FALSE at this point; and we don't want the user to see an
	// alarming 404 error, so change it to CURLE_OK so the caller will behave nicely
	if (m_httpStatusCode >= 400)
	{
		// whether 400 or 404, return CURLE_OK (ie. 0) to the caller, since bMatchedKB
		// being FALSE is all that matters to the caller for a 404 Not Found error
		str_CURLbuffer.clear();
		str_CURLheaders.clear();
		return CURLE_OK;
	}

	//  Make the json data accessible (result is CURLE_OK if control gets to here)
	//  We requested separate headers callback be used, so str_CURLbuffer should only have
	//  the json string for the looked up one or more lines of the KB table
	//  (There will generally be zero, if no such entry yet, or two (one for adapting KB,
	//  the other for the parallel glossing KB), or some higher multiple of two - as when
	//  the same source language is adapted into more than one target language)
	if (!str_CURLbuffer.empty())
	{
        // json array data begins with "[{", so test for the payload starting this way, if
        // it doesn't, then there is only an error string to grab -- quite possibly "No
        // matching entry found"
		// json data beginswith "[{", so test for the payload starting this way, if it
		// doesn't, then there is only an error string to grab -- quite possibly
		// "No matching entry found", in which case, don't do any of the json stuff, and
		// the value in m_kbServerLastTimestampReceived will be correct and can be used
		// to update the persistent storage file for the time of the lastsync
		std::string srch = "[{";
		size_t offset = str_CURLbuffer.find(srch);

		// not npos means JSON data starts somewhere; a JSON parse ignores all before [ or {
		if (offset != string::npos)
		{
			CBString jsonArray(str_CURLbuffer.c_str()); // JSON expects byte data, convert after Parse()
			wxJSONValue jsonval;
			wxJSONReader reader;
			int numErrors = reader.Parse(ToUtf16(jsonArray), &jsonval);
			if (numErrors > 0)
			{
				// Write to a file, in the _LOGS_EMAIL_REPORTS folder, whatever was sent,
				// the developers would love to have this info. The latest copy only is
				// retained, the "w" mode clears the earlier file if there is one, and
				// writes new content to it
				wxString aFilename = _T("LookupSingleKb_bad_data_sent_to ") + m_pApp->m_curProjectName + _T(".txt");
				wxString workOrCustomFolderPath;
				if (::wxDirExists(m_pApp->m_logsEmailReportsFolderPath))
				{
					wxASSERT(!m_pApp->m_curProjectName.IsEmpty());

					if (!m_pApp->m_bUseCustomWorkFolderPath)
					{
						workOrCustomFolderPath = m_pApp->m_workFolderPath;
					}
					else
					{
						workOrCustomFolderPath = m_pApp->m_customWorkFolderPath;
					}
					wxString path2BadData = workOrCustomFolderPath + m_pApp->PathSeparator +
						m_pApp->m_logsEmailReportsFolderName + m_pApp->PathSeparator + aFilename;
                    wxString mode = _T('w');
                    size_t mySize = str_CURLbuffer.size();
                    wxFFile ff(path2BadData.GetData(), mode.GetData());
					wxASSERT(ff.IsOpened());
					ff.Write(str_CURLbuffer.c_str(), mySize);
					ff.Close();
				}

				// a non-localizable message will do, it's unlikely to ever be seen
				wxString msg;
				msg = msg.Format(_T("LookupSingleKb(): json reader.Parse() failed. Server sent bad data.\nThe bad data is stored in the file with name: \n%s \nLocated at the folder: %s \nSend this file to the developers please."),
					aFilename.c_str(), m_pApp->m_logsEmailReportsFolderPath.c_str());
				wxMessageBox(msg, _T("KBserver error"), wxICON_ERROR | wxOK);

				str_CURLbuffer.clear(); // always clear it before returning
				str_CURLheaders.clear();
				return -1; // this is better, neither curl nor http failed
			}
			// No errors...
			unsigned int listSize = (unsigned int)jsonval.Size();
#if defined (SYNC_LOGS)
			// get feedback about now many table lines we got, in debug mode - we expect
			// multiples of 2, from which we later must match the one with same type as was
			// passed in, and same targetlanguage code as was passed in
			if (listSize > 0)
			{
				wxLogDebug(_T("LookupSingleKb() returned %d KBs from the kb table with matching sourcelanguage code"), listSize);
			}
#endif
			wxArrayPtrVoid kbStructsArray;
			unsigned int index;
			KbServerKb* pKbStruct = NULL;
			for (index = 0; index < listSize; index++)
			{
				// We extract all fields for each of the KB lines in the kb table
				pKbStruct = new KbServerKb;
				kbStructsArray.Add(pKbStruct);
				// We extract id, username, fullname, kbadmin flag value, useradmin flag value,
				// and the timestamp at which the username was added to the entry table.
				pKbStruct->id = jsonval[index][_T("id")].AsLong();
				pKbStruct->sourceLanguageCode = jsonval[index][_T("sourcelanguage")].AsString();
				pKbStruct->targetLanguageCode = jsonval[index][_T("targetlanguage")].AsString();
				pKbStruct->kbType = jsonval[index][_T("type")].AsInt(); // could use AsLong() here instead
				pKbStruct->username = jsonval[index][_T("username")].AsString(); // this is who created
								// this particular entry in the kb table; which is not something
								// we particularly care about for the client API functions
				pKbStruct->timestamp = jsonval[index][_T("timestamp")].AsString();
				pKbStruct->deleted = jsonval[index][_T("deleted")].AsInt();

#if defined (SYNC_LOGS)
				// list what fields we extracted for each line of the kb table matched
				wxLogDebug(_T("LookupSingleKb matched: id: %d , src: %s , transln: %s , type: %d, deleted: %d , username & timestamp: (not interested)"),
					pKbStruct->id, pKbStruct->sourceLanguageCode.c_str(), pKbStruct->targetLanguageCode.c_str(),
					pKbStruct->kbType, pKbStruct->deleted);
#endif
			} // loop ends

			// Now check if the passed in kbType and tgtLangCode have a match in the structs
			// saved in the kbStructsArray. If so, the lookup succeeded. Either way, inform the
			// caller of the result using the bMatchedKB boolean param of the signature; then
			// delete the structs from the heap before returning
			size_t count = kbStructsArray.GetCount();
			size_t i;
			for (i = 0; i < count; i++)
			{
				KbServerKb* pKbStruct = (KbServerKb*)kbStructsArray.Item(i);
				wxASSERT(pKbStruct != NULL);
				if ((pKbStruct->targetLanguageCode == tgtLangCode) && (pKbStruct->kbType == kbType))
				{
					bMatchedKB = TRUE; // the looked up KB exists in the kb table of this KBserver
				}
				// no longer need the struct once it has been tested
				delete pKbStruct;
			}
			kbStructsArray.clear();
			str_CURLbuffer.clear(); // always clear it before returning
			str_CURLheaders.clear(); // ditto

		} // end of TRUE block for test: if (offset == 0
		else
		{
            // str_CURLbuffer contains an error message, such as "No matching entry found",
            // but we will ignore it, the HTTP status code should be 404. Either way, since
            // this is a lookup function which we want to fail gracefully when the looked
            // up KB is not actually present, the only thing we want to return to the
            // caller is bMatchedKB = FALSE when control enters this block. (It's that
            // already.) We return a curlcode of 0 so that no error message is triggered in
            // the caller.
#if defined (SYNC_LOGS)
            // But in debug mode we might was well log the error to check all is
			// working as expected
			wxString msg;
			msg  = msg.Format(_T("LookupSingleKb(): Found no KB lines matching the sourcelanguage code.  HTTP status: %d %s;\n   nevertheless returning 0 and bMatchedKB FALSE"),
                              m_httpStatusCode, m_httpStatusText.c_str());
            wxLogDebug(msg);
#endif
			str_CURLbuffer.clear();
			str_CURLheaders.clear();
			return 0;
		} // end of else block for test: if (offset == 0)

	} // end of TRUE block for test: if (!str_CURLbuffer.empty())
	else
	{
		// Control should not get to here, a HTTP 404 Not Found error should have
		// occurred, but just in case... str_CURLbuffer empty means no JSON data returned,
		// which means the looked for KB can't be present, so return bMatchedKB FALSE
		// via the signature - it's already defaulted to FALSE, so just make sure the
		// headers are cleared
		str_CURLheaders.clear();
	} // end of else block for test: if (!str_CURLbuffer.empty())

	return 0;
}
*/

// Authenticates to the mariaDB/kbserver mysql server, and looks up in the
// current project the row for non-deleted src/nonSrc as passed in.
// Escaping any ' in src or nonSrc is done internally before sending SQL requests
int KbServer::LookupEntryFields(wxString src, wxString nonSrc)
{
	wxString execFileName = _T("do_lookup_entry.exe");
	wxString resultFile = _T("lookup_entry_return_results.dat");
	wxString datFileName = _T("lookup_entry.dat");
	wxString execPath;
	if (m_pApp->m_curExecPath.IsEmpty())
	{
		m_pApp->m_curExecPath = m_pApp->execPath;
	}
	execPath = m_pApp->m_curExecPath;

	int* pWhichCounter = NULL; // initialise
	int rv = -1; // initialise

	// First, store src and nonSrc on the relevant app variables, 
	// so ConfigureDATfile(create_entry)'s ConfigureMovedDatFile() can grab them;
	// or if in glossing mode, store gloss on m_curNormalGloss
	// (It's these 'Normal' ones which get any ' escaping done on them)
	m_pApp->m_curNormalSource = src;
	if (gbIsGlossing)
	{
		m_pApp->m_curNormalGloss = nonSrc;
		pWhichCounter = &m_pApp->m_nLookupEntryGlossCount;
	}
	else
	{
		m_pApp->m_curNormalTarget = nonSrc;
		pWhichCounter = &m_pApp->m_nLookupEntryAdaptationCount;
	}

	// Use the counters, app's m_nCreateAdaptionCount, or if currently glossing and
	// therefore nonSrc contains glosses to go into the glossing entry table as kbType = 2,
	// then use m_nCreateGlossCount. These counters, if zero, provide a processing path
	// which take the input .dat file from the dist folder, moving it to the execPath's
	// folder, clearing the contents, and replacing with the relevant data to form
	// the commandLine to be submitted to wxExecute() - and that protocol deletes the
	// old .dat input file so moved, before those steps create a replacement filled out.
	// But if the counter is greater than zero, an alternative speedier path is followed.
	// Because the .dat file has 9 fields, and needs only two new values - the src and nonSrc.
	// In this circumstance it's silly to waste time by following the protocol above.
	// Instead, don't delete the old input .dat file in the exec folder, but refill just
	// the two fields that need refilling with the values pasted in from the signature.
	// That's faster, and important here because LookupEntryFields() will be called 
	// maybe tens of thousands of times over the life of the AI project.

	bool bOkay = MoveOrInPlace(lookup_entry, m_pApp, *pWhichCounter, execPath,
		src, nonSrc, 6, 7, 8); // last 3 are locations of 
							   // src field, nonSrc field, kbType field
	wxUnusedVar(bOkay);
	// Since the nonSrc content could be adaptation, or gloss, depending on whether kbType
	// currently in action is '1' or '2' respectively, the same code handles either in
	// the following blocks. However, we should check that the kbType value is set correctly.

	// The create_entry.dat input file is now ready for grabbing the command
	// line (its first line) for the ::wxExecute() call in CallExecute()
	bool bOK = m_pApp->CallExecute(lookup_entry, execFileName, execPath,
		resultFile, 99, 99, FALSE); // FALSE is bReportResult

	return (rv = bOK == TRUE ? 0 : -1);

	/*
	CURL *curl;
	CURLcode result;
	wxString aUrl; // convert to utf8 when constructed
	wxString aPwd; // ditto
	str_CURLbuffer.clear();
	str_CURLheaders.clear();
	// BEW 11Jun15 support "<noform>" as a standin for an empty adaptation or gloss
	wxString noform = _T("<noform>");
	if (targetPhrase.IsEmpty())
	{
		targetPhrase = noform; // this ensures uniqueness, because if left as an
							   // empty string, the lookup returns all rows which
							   // match the source field
	}
	CBString charUrl;
	CBString charUserpwd;
	// Try a CBString solution for url-encoding, and using curl_easy_encode() etc
	// (because the sourcePhrase and/or targetPhrase strings may be phrases)
	char *encodedsource = NULL;	// ptr to encoded source string, initialize
	char *encodedtarget = NULL;	// ptr to encoded target string, initialize
	CBString charSrc = ToUtf8(sourcePhrase);
	CBString charTgt = ToUtf8(targetPhrase);
	int lenSrc = charSrc.GetLength();
	int lenTgt = charTgt.GetLength();
	char* pCharSrc = (char*)charSrc;
	char* pCharTgt = (char*)charTgt;

	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString kbType;
	int type = GetKBServerType();
	wxItoa(type,kbType);
	wxString langcode;
	if (type == 1)
	{
		langcode = GetTargetLanguageCode();
	}
	else
	{
		langcode = GetGlossLanguageCode();
	}
	wxString container = _T("entry");
	// The URL has to be url-encoded -- do it later below with curl_easy_escape()
	aUrl = GetKBServerIpAddr() + slash + container + slash+ GetSourceLanguageCode() +
			slash + langcode + slash + kbType + slash; // url-encode the new 2 fields
#if defined (SYNC_LOGS) //&& defined (__WXGTK__)
	wxLogDebug(_T("LookupEntryFields(): wxString aUrl = %s"), aUrl.c_str());
#endif
	charUrl = ToUtf8(aUrl);

	// Create the username:password string
	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	curl = curl_easy_init();
	if (curl) {
		encodedsource = curl_easy_escape(curl, pCharSrc, lenSrc); // need LHS for curl_free() call below
		encodedtarget = curl_easy_escape(curl, pCharTgt, lenTgt); //  ditto
		CBString encodedSrc(encodedsource);
		CBString encodedTgt(encodedtarget);
		charUrl += encodedSrc;
		charUrl += "/";
		charUrl += encodedTgt;
#if defined (SYNC_LOGS) //&& defined (__WXGTK__)
		wxString wxencodedUrl = ToUtf16(charUrl);
		wxLogDebug(_T("LookupEntryFields(): encoded Url = %s"), wxencodedUrl.c_str());
#endif
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);
		// We want separate storage for headers to be returned, to get the HTTP status code
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &curl_headers_callback);
		//curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		//curl_easy_setopt(curl, CURLOPT_HEADER, 1L); // comment out when collecting
													  //headers separately
		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) //&& defined (__WXGTK__)
        CBString s2(str_CURLheaders.c_str());
        wxString showit2 = ToUtf16(s2);
		wxLogDebug(_T("LookupEntryFields(): Returned headers: %s"), showit2.c_str());

        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
		wxLogDebug(_T("LookupEntryFields() str_CURLbuffer has: %s    , The CURLcode is: %d"),
					showit.c_str(), (unsigned int)result);
#endif
		// Get the HTTP status code, and the English message
		ExtractHttpStatusEtc(str_CURLheaders, m_httpStatusCode, m_httpStatusText);

		// If the only error was a HTTP one, then result will contain CURLE_OK, in which
		// case the next block is skipped
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("LookupEntryFields() result code: %d Error: %s"),
				result, error.c_str());
			wxMessageBox(msg, _T("Error when looking up an entry, using LookupEntryFields"), wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(msg);
			curl_free(encodedsource);
			curl_free(encodedtarget);
			curl_easy_cleanup(curl);
			return (int)result;
		}
	}
	curl_easy_cleanup(curl);

	// If there was a HTTP error (typically, it would be 404 Not Found, because the
	// src/tgt pair are not in the DB yet, but 100 Bad Request may also be possible I
	// guess) then exit early, there won't be json data to handle if that was the case
	if (m_httpStatusCode >= 400)
	{
		// whether 400 or 404, return CURLE_HTTP_RETURNED_ERROR (ie. 22) to the caller
		ClearEntryStruct(); // if we subsequently access it, its source member is an empty string
		curl_free(encodedsource);
		curl_free(encodedtarget);
		str_CURLbuffer.clear();
		str_CURLheaders.clear();
		return CURLE_HTTP_RETURNED_ERROR;
	}

	//  Make the json data accessible (result is CURLE_OK if control gets to here)
	//  We requested separate headers callback be used, so str_CURLbuffer should only have
	//  the json string for the looked up entry
	if (!str_CURLbuffer.empty())
	{
		CBString jsonArray(str_CURLbuffer.c_str()); // JSON expects byte data, convert after Parse()
		wxJSONValue jsonval;
		wxJSONReader reader;
		int numErrors = reader.Parse(ToUtf16(jsonArray), &jsonval);
		if (numErrors > 0)
		{
			// Write to a file, in the _LOGS_EMAIL_REPORTS folder, whatever was sent,
			// the developers would love to have this info. The latest copy only is
			// retained, the "w" mode clears the earlier file if there is one, and
			// writes new content to it
			wxString aFilename = _T("LookupEntryFields_bad_data_sent_to ") + m_pApp->m_curProjectName + _T(".txt");
			wxString workOrCustomFolderPath;
			if (::wxDirExists(m_pApp->m_logsEmailReportsFolderPath))
			{
				wxASSERT(!m_pApp->m_curProjectName.IsEmpty());

				if (!m_pApp->m_bUseCustomWorkFolderPath)
				{
					workOrCustomFolderPath = m_pApp->m_workFolderPath;
				}
				else
				{
					workOrCustomFolderPath = m_pApp->m_customWorkFolderPath;
				}
				wxString path2BadData = workOrCustomFolderPath + m_pApp->PathSeparator +
					m_pApp->m_logsEmailReportsFolderName + m_pApp->PathSeparator + aFilename;
                wxString mode = _T('w');
                size_t mySize = str_CURLbuffer.size();
                wxFFile ff(path2BadData.GetData(), mode.GetData());
				wxASSERT(ff.IsOpened());
				ff.Write(str_CURLbuffer.c_str(), mySize);
				ff.Close();
			}

			// a non-localizable message will do, it's unlikely to ever be seen
			wxString msg;
			msg = msg.Format(_T("LookupEntryFields(): json reader.Parse() failed. Server sent bad data.\nThe bad data is stored in the file with name: \n%s \nLocated at the folder: %s \nSend this file to the developers please."),
				aFilename.c_str(), m_pApp->m_logsEmailReportsFolderPath.c_str());
			wxMessageBox(msg, _T("KBserver error"), wxICON_ERROR | wxOK);

			curl_free(encodedsource);
			curl_free(encodedtarget);
			str_CURLbuffer.clear(); // always clear it before returning
			str_CURLheaders.clear();
			return CURLE_HTTP_RETURNED_ERROR;
		}
		// we extract id, source phrase, target phrase, deleted flag value & username
		// for index value 0 only (there should only be one json object to deal with)
		wxString noform = _T("<noform>");
        wxString s;
		ClearEntryStruct(); // re-initializes m_entryStruct member
		m_entryStruct.id = jsonval[_T("id")].AsLong(); // needed, as there may be a
					// subsequent pseudo-delete or undelete, and those are id-based
		m_entryStruct.source = jsonval[_T("source")].AsString();
		// BEW 11Jun15 restore <noform> to an empty string
		s = jsonval[_T("target")].AsString();
		if (s == noform)
		{
			s.Empty();
		}
		m_entryStruct.translation = s;
		m_entryStruct.username = jsonval[_T("user")].AsString();
		m_entryStruct.deleted = jsonval[_T("deleted")].AsInt();

		curl_free(encodedsource);
		curl_free(encodedtarget);
		str_CURLbuffer.clear(); // always clear it before returning
		str_CURLheaders.clear();
	}
	return 0;
	*/
}

// public accessor
DownloadsQueue* KbServer::GetQueue()
{
	return &m_queue;
}

// Return the CURLcode value, downloaded JSON data is extracted and copied, entry by
// entry, into a series of KbServerEntry structs, each created on the heap, and stored at
// the end of the m_queue member (derived from wxList<T>). This ChangedSince_Queued() is
// based on ChangedSince_Timed(), and differs only in that (1) it is called in a detached thread
// (and so the thread self-destructs when done), and (2) the JSON payload is unpacked into
// the series of KbServerEntry structs mentioned above and stored in a queue. (The structs
// are removed from the queue, one per idle event, their data merged to the KB, and then
// deleted - that is all done in the main thread.)
//
// If the data download succeeds, the 'last sync' timestamp is extracted from the headers
// information and stored in the private member variable: m_kbServerLastTimestampReceived
// If there is a subsequent error - such as the JSON data extraction failing, then -1 is
// returned and the timestamp value in m_kbServerLastTimestampReceived should be
// disregarded -- because we only transfer the timestamp from there to the private member
// variable: m_kbServerLastSync, if there was no error (ie. returned code was 0). If 0 is
// returned then in the caller we should merge the data into the local KB, and use the
// public member function UpdateLastSyncTimestamp() to move the timestamp from
// m_kbServerLastTimestampReceived into m_kbServerLastSync; and then use the public member
// function ExportLastSyncTimestamp() to export that m_kbServerLastSync value to persistent
// storage. (Of course, the next successful ChangedSince_Queued() call will update what is
// stored in persistent storage; and the timeStamp value used for that call is whatever is
// currently within the variable m_kbServerLastSync).
// BEW 10May16 - using a queue is too slow for normal changedsince downloads, but when we
// want to delete a whole KB from the remote KBserver, the queue is useful, so retain this
/* BEW 21Oct20 unneeded
int KbServer::ChangedSince_Queued(wxString timeStamp, bool bDoTimestampUpdate)
{
	//  TODO leon's way   --- but I think I'll not need this, it just supported JM's slow 1-by-1 kbserver deletions

	str_CURLbuffer.clear(); // always make sure it is cleared for accepting new data
	str_CURLheaders.clear(); // BEW added 9Feb13

	CURL *curl;
	CURLcode result = CURLE_OK; // initialize to a harmless value
	wxString aUrl; // convert to utf8 when constructed
	wxString aPwd; // ditto

	CBString charUrl;
	CBString charUserpwd;

	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString kbType;
	int type = GetKBServerType();
	wxItoa(type,kbType);
	wxString langcode;
	if (type == 1)
	{
		langcode = GetTargetLanguageCode();
	}
	else
	{
		langcode = GetGlossLanguageCode();
	}
	wxString container = _T("entry");
	wxString changedSince = _T("/?changedsince=");

	aUrl = GetKBServerIpAddr() + slash + container + slash+ GetSourceLanguageCode() + slash +
			langcode + slash + kbType + changedSince + timeStamp;
#if defined (SYNC_LOGS) //&& defined (__WXGTK__)
	wxLogDebug(_T("ChangedSince_Queued(): wxString aUrl = %s"), aUrl.c_str());
#endif
	charUrl = ToUtf8(aUrl);
	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);

		// We want the download's timestamp, so we must ask for the headers to be added
		// and sent to a callback function dedicated for collecting the headers
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &curl_headers_callback);

		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) // && defined (__WXGTK__)
        // BEW added 9Feb13, check what, if anything, got added to str_CURLheaders_callback
        CBString s2(str_CURLheaders.c_str());
        wxString showit2 = ToUtf16(s2);
		wxLogDebug(_T("Queued: Returned headers: %s"), showit2.c_str());

        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
		wxLogDebug(_T("Queued: Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif
        if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("ChangedSince_Queued() result code: %d Error: %s"),
				result, error.c_str());
			wxMessageBox(msg, _T("Error when downloading entries"), wxICON_EXCLAMATION | wxOK);

            curl_easy_cleanup(curl);

            str_CURLbuffer.clear(); // always clear it before returning
            str_CURLheaders.clear(); // BEW added 9Feb13
            return (int)result;
        }
	}
	// no CURL error, so continue...
	curl_easy_cleanup(curl);

	// Extract from the headers callback, the HTTP code, and the X-MySQL-Date value,
	// and the HTTP status information
    ExtractTimestamp(str_CURLheaders, m_kbServerLastTimestampReceived);
    ExtractHttpStatusEtc(str_CURLheaders, m_httpStatusCode, m_httpStatusText);

#if defined (SYNC_LOGS) // && defined (__WXGTK__)
        // show what ExtractHttpStatusEtc() returned
        CBString s2(str_CURLheaders.c_str());
        wxString showit2 = ToUtf16(s2);
		wxLogDebug(_T("Queued: From headers: Timestamp = %s , HTTP code = %d , HTTP msg = %s"),
                   m_kbServerLastTimestampReceived.c_str(), m_httpStatusCode,
                   m_httpStatusText.c_str());
#endif

	//  Make the json data accessible (result is CURLE_OK if control gets to here)
	//
	//  BEW 29Jan13, beware, if no new entries have been added since last time, then
	//  the payload will not have a json string, and will have an 'error' string
    //  "No matching entry found". This isn't actually an error, and ChangedSince(), in
    //  this circumstance should just benignly exit without doing or saying anything, and
    //  return 0 (CURLE_OK)
	if (!str_CURLbuffer.empty())
	{
		// json data beginswith "[{", so test for the payload starting this way, if it
		// doesn't, then there is only an error string to grab -- quite possibly
		// "No matching entry found", in which case, don't do any of the json stuff, and
		// the value in m_kbServerLastTimestampReceived will be correct and can be used
		// to update the persistent storage file for the time of the lastsync
		std::string srch = "[{";
		size_t offset = str_CURLbuffer.find(srch);

		// not npos means JSON data starts somewhere; a JSON parse ignores all before [ or {
		if (offset != string::npos)
		{
			CBString jsonArray(str_CURLbuffer.c_str()); // JSON expects byte data, convert after Parse()

            wxJSONValue jsonval;
            wxJSONReader reader;
			int numErrors = reader.Parse(ToUtf16(jsonArray), &jsonval);
			if (numErrors > 0)
			{
				// Write to a file, in the _LOGS_EMAIL_REPORTS folder, whatever was sent,
				// the developers would love to have this info. The latest copy only is
				// retained, the "w" mode clears the earlier file if there is one, and
				// writes new content to it
				wxString aFilename = _T("ChangedSince_Queued_bad_data_sent_to ") + m_pApp->m_curProjectName + _T(".txt");
				wxString workOrCustomFolderPath;
				if (::wxDirExists(m_pApp->m_logsEmailReportsFolderPath))
				{
					wxASSERT(!m_pApp->m_curProjectName.IsEmpty());
					if (!m_pApp->m_bUseCustomWorkFolderPath)
					{
						workOrCustomFolderPath = m_pApp->m_workFolderPath;
					}
					else
					{
						workOrCustomFolderPath = m_pApp->m_customWorkFolderPath;
					}
					wxString path2BadData = workOrCustomFolderPath + m_pApp->PathSeparator +
						m_pApp->m_logsEmailReportsFolderName + m_pApp->PathSeparator + aFilename;
                    wxString mode = _T('w');
                    size_t mySize = str_CURLbuffer.size();
                    wxFFile ff(path2BadData.GetData(), mode.GetData());
					wxASSERT(ff.IsOpened());
					ff.Write(str_CURLbuffer.c_str(), mySize);
					ff.Close();
				}
                // a non-localizable message will do, it's unlikely to ever be seen
				// once correct utf-8 consistently comes from the remote server
				wxString msg;
				msg = msg.Format(_T("ChangedSince_Queued(): json reader.Parse() failed. Server sent bad data.\nThe bad data is stored in the file with name: \n%s \nLocated at the folder: %s \nSend this file to the developers please."),
					aFilename.c_str(), m_pApp->m_logsEmailReportsFolderPath.c_str());
				wxMessageBox(msg, _T("KBserver error"), wxICON_ERROR | wxOK);

				str_CURLbuffer.clear(); // always clear it before returning
                str_CURLheaders.clear(); // always clear it before returning
               return -1;
            }
            unsigned int listSize = jsonval.Size();
#if defined (SYNC_LOGS)
            // get feedback about now many entries we got, in debug mode
            if (listSize > 0)
            {
                wxLogDebug(_T("ChangedSince_Queued() returned %d entries, for data added to KBserver since %s"),
                    listSize, timeStamp.c_str());
            }
#endif
            unsigned int index;
			wxString noform = _T("<noform>");
			wxString s;
			KbServerEntry* pEntryStruct = NULL;
            for (index = 0; index < listSize; index++)
            {
                // We can extract id, source phrase, target phrase, deleted flag value,
                // username, and timestamp string; but for supporting the sync of a local
                // KB we need only to extract source phrase, target phrase, the value of
                // the deleted flag, and the username be included in the KbServerEntry
                // structs
                pEntryStruct = new KbServerEntry;
				pEntryStruct->id = jsonval[index][_T("id")].AsLong();
				pEntryStruct->source = jsonval[index][_T("source")].AsString();
					(jsonval[index][_T("source")].AsString()).Clear(); // BEW 10May16, don't leak it
				// BEW 11Jun15 restore <noform> to an empty string
				s = jsonval[index][_T("target")].AsString();
					(jsonval[index][_T("target")].AsString()).Clear(); // BEW 10May16, don't leak it
				if (s == noform)
				{
					s.Empty();
				}
				pEntryStruct->translation = s;
					s.Clear(); // BEW 10May16, don't leak it
				pEntryStruct->deleted = jsonval[index][_T("deleted")].AsInt();
				pEntryStruct->username = jsonval[index][_T("user")].AsString();
					(jsonval[index][_T("user")].AsString()).Clear(); // BEW 10May16, don't leak it

                // Append to the end of the queue (if the main thread is removing the first
                // struct in the queue currently, this will block until the s_QueueMutex is
                // released)
				PushToQueueEnd(pEntryStruct);
#if defined (SYNC_LOGS)
                // list what fields we extracted for each line of the entry table matched
				wxLogDebug(_T("Queued: Downloaded:  %s  ,  %s  ,  deleted = %d  ,  username = %s"),
                    pEntryStruct->source.c_str(), pEntryStruct->translation.c_str(),
					pEntryStruct->deleted, pEntryStruct->username.c_str());
#endif
            }

            str_CURLbuffer.clear(); // always clear it before returning
            str_CURLheaders.clear(); // BEW added 9Feb13

			// since all went successfully, update the lastsync timestamp, if requested
			// (when using ChangedSince_Queued() to download entries for deleting an entire
			// KB, we don't want any timestamp update done)
			if (bDoTimestampUpdate)
			{
				UpdateLastSyncTimestamp();
			}
#if defined(SYNC_LOGS)
			//wxRemoveFile(tempjsonfile); // <<-- uncomment out, if we don't want to keep the latest one
#endif
		} // end of TRUE block for test: if (offset == 0)
		else
		{
			// buffer contains an error message, such as "No matching entry found",
			// but we will ignore it, the HTTP status codes are enough
			if (m_httpStatusCode == 404)
            {
                // A "Not Found" error. No new entries were sent, because there were none
				// more recent than the last saved timestamp value, so do nothing here
				// except update the lastsync timestamp, there's no point in keeping the
				// earlier value unchanged
 				if (bDoTimestampUpdate)
				{
					UpdateLastSyncTimestamp();
				}
            }
            else
            {
				// Some other "error", so don't update the lastsync timestamp. The most
				// likely thing that happened is that there have been no new entries to
				// the entry table since the last 'changed since' sync - in which case no
				// JSON is produced. No JSON means that the test above for "[{" will not
				// find those two metacharacters for a JSON array - which means that the
				// else branch sends control to the else block above but with
				// m_httpStatusCode value of 200 (ie. success), so we don't actually have
                // an error situation at all; and testing for a possible http error fails
                // to find any error, and so the present else block is entered. So we are
                // here almost certainly because there was no data to send in the
                // transmission. This should be treated as a successful transmission, and
				// not show a 'failure' message - even if just in the debug build. It's
				// okay as well to not update the lastsync timestamp in this circumstance
#if defined (SYNC_LOGS)
				wxString msg;
				msg  = msg.Format(_T("ChangedSince_Queued():  HTTP status: %d   No JSON data was returned. (This is an advisory message shown only in the Debug build.)"),
                                  m_httpStatusCode);
                wxMessageBox(msg, _T("No data returned"), wxICON_EXCLAMATION | wxOK);
#endif
				str_CURLbuffer.clear();
				str_CURLheaders.clear();
				return (int)CURLE_OK;
            }
		}
	} // end of TRUE block for test: if (!str_CURLbuffer.empty())

    str_CURLbuffer.clear(); // always clear it before returning
    str_CURLheaders.clear(); // BEW added 9Feb13

	return (int)0;
}
*/


void KbServer::ClearEntryStruct()
{
	m_entryStruct.id.Empty(); // long was python-converted by str() to string
	m_entryStruct.srcLangName.Empty();
	m_entryStruct.tgtLangName.Empty();
	m_entryStruct.source.Empty();
	m_entryStruct.nonSource.Empty();
	m_entryStruct.username.Empty();
	m_entryStruct.timestamp.Empty(); // python will see the next two as strings
	m_entryStruct.type = _T('1'); // assume adapting type
	m_entryStruct.deleted = _T('0'); //assume not-deleted
}

//void KbServer::SetEntryStruct(KbServerEntry entryStruct)
//{
//	m_entryStruct = entryStruct;
//}

KbServerEntry KbServer::GetEntryStruct()
{
	return m_entryStruct;
}

void KbServer::ClearUserStruct()
{
	m_userStruct.id = 0;
	m_userStruct.username.Empty();
	m_userStruct.fullname.Empty();
	m_userStruct.timestamp.Empty();
	m_userStruct.kbadmin = false;
	m_userStruct.useradmin = false;
}

KbServerKb KbServer::GetKbStruct()
{
	return m_kbStruct;
}

void KbServer::ClearKbStruct()
{
	m_kbStruct.sourceLanguageName.Empty();
	m_kbStruct.targetLanguageName.Empty();
	m_kbStruct.kbType = 1; // default to adaptations KB type
	m_kbStruct.username.Empty();
}

KbServerLanguage KbServer::GetLanguageStruct()
{
	return m_languageStruct;
}

void KbServer::ClearLanguageStruct()
{
	/*
	m_languageStruct.code.Empty();
	m_languageStruct.username.Empty();
	m_languageStruct.description.Empty();
	m_languageStruct.timestamp.Empty();
	*/
}


//void KbServer::SetUserStruct(KbServerUser userStruct)
//{
//	m_userStruct = userStruct;
//}

KbServerUser KbServer::GetUserStruct()
{
	return m_userStruct;
}

UsersListForeign* KbServer::GetUsersListForeign() // a 'for manager' scenario
{
	return &m_usersListForeign;
}

//LanguagesList* KbServer::GetLanguagesList()
//{
//	return &m_languagesList;
//}
/*
// deletes from the heap all KbServerLanguage struct ptrs within m_languagesList
void KbServer::ClearLanguagesList(LanguagesList* pLanguagesList)
{
	if (pLanguagesList == NULL || pLanguagesList->empty())
		return;
	LanguagesList::iterator iter;
	LanguagesList::compatibility_iterator c_iter;
	int anIndex = -1;
	for (iter = pLanguagesList->begin(); iter != pLanguagesList->end(); ++iter)
	{
		anIndex++;
		c_iter = pLanguagesList->Item((size_t)anIndex);
		KbServerLanguage* pEntry = c_iter->GetData();
		if (pEntry != NULL)
		{
			delete pEntry; // frees its memory block
		}
	}
	// The list's stored pointers are now hanging, so clear them
	pLanguagesList->clear();
}
*/
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

/* BEW 3Nov20 deprecated
KbsList* KbServer::GetKbsList()
{
	return &m_kbsList;
}
*/
/* BEW 3Nov20 deprecated
void KbServer::DeleteDownloadsQueueEntries()
{
	DownloadsQueue::iterator iter;
	KbServerEntry* pStruct = NULL;
	if (!m_queue.IsEmpty())
	{
		for (iter = m_queue.begin(); iter != m_queue.end(); ++iter)
		{
			pStruct = *iter;
			// delete it
			delete pStruct;
		}
		// now clear the list
		m_queue.clear();
	}
}
*/
/* BEW 3Nov20 deprecated
// deletes from the heap all KbServerUser struct ptrs within m_usersList
void KbServer::ClearKbsList(KbsList* pKbsList)
{
	if (pKbsList == NULL || pKbsList->empty())
		return;
	KbsList::iterator iter;
	KbsList::compatibility_iterator c_iter;
	int anIndex = -1;
	for (iter = pKbsList->begin(); iter != pKbsList->end(); ++iter)
	{
		anIndex++;
		c_iter = pKbsList->Item((size_t)anIndex);
		KbServerKb* pEntry = c_iter->GetData();
		if (pEntry != NULL)
		{
			delete pEntry; // frees its memory block
		}
	}
	// The list's stored pointers are now hanging, so clear them
	pKbsList->clear();
}
*/

// BEW 20Oct20, 1-based counting, so the username field which is field 2
// will have a preceding comma, since the ipAddr will precede that field
// and the ipAddr field will not have a preceding comma. So if commaCount
// set to 1 is passed in, the code below will fail. So if ipAddr is to be
// changed, pass in 1, and test for that and handle it as a special case.
wxString KbServer::ReplaceFieldWithin(wxString cmdLine, int commaCount, wxString strReplacement)
{
	wxString comma = _T(",");
	int offset = wxNOT_FOUND;
	int lastPos = 0;
	// Find the comma preceding the commaCount's field (1-based counting)
	int i;
	int max = commaCount - 1;
	if (commaCount == 1)
	{
		// strReplacement is an ipAddress, replace that at start of cmdLine
		offset = cmdLine.Find(comma);
		if (offset > 0)
		{
			// There is something before the first comma in cmdLine
			cmdLine = cmdLine.Mid(offset); // remove whatever was before first comma
			cmdLine = strReplacement + cmdLine; // the ipAddr has now been replaced
		}
	}
	else
	{
		// For commaCount > 1
		for (i = 0; i < max; i++)
		{
			offset = cmdLine.find(comma, lastPos);
			lastPos = offset + 1; // get past the matched comma, no field is empty
		}
		// For max == 5, when i gets to 4, lastPos will be pointing at the
		// start of the contents for source field, and offset will be 1 less,
		// so an extra Find() is needed to advance offset to the comma at the 
		// comma at the end of source field's contents.

		wxString leftPart = cmdLine.Left(lastPos);
		leftPart += strReplacement; // it's replaced - now join the bits to reform cmdLine
		// Now get offset to the next comma
		offset = cmdLine.find(comma, lastPos);
		wxString lastPart = cmdLine.Mid(offset); // everything from the comma on, inclusive
		cmdLine = leftPart + lastPart;
	}
	return cmdLine;
}

// BEW 7May16, we are forced to synchronous calls that use openssl, because the latter
// (if put on a detached thread) leak about a kilobyte per KBserver access, but if not
// on a thread there is no leak. Joinable threads are no solution because even if they
// didn't incur openssl leaks, the calling thread has to .Wait() for the joinable thread
// to finish. So that's no different than a synchronous call such as this one.
int KbServer::CreateEntry(KbServer* pKbSvr, wxString src, wxString nonSrc)
{
	wxUnusedVar(pKbSvr);

	wxString execFileName = _T("do_create_entry.exe");
	wxString resultFile = _T("create_entry_return_results.dat");
	wxString datFileName = _T("create_entry.dat");
	wxString execPath;
	if (m_pApp->m_curExecPath.IsEmpty())
	{
		m_pApp->m_curExecPath = m_pApp->execPath;
	}
	execPath = m_pApp->m_curExecPath;

	int* pWhichCounter = NULL; // initialise
	int rv = -1; // initialise

	// First, store src and nonSrc on the relevant app variables, 
	// so ConfigureDATfile(create_entry)'s ConfigureMovedDatFile() can grab them;
	// or if in glossing mode, store gloss on m_curNormalGloss
	m_pApp->m_curNormalSource = src;
	if (gbIsGlossing)
	{
		m_pApp->m_curNormalGloss = nonSrc;
		pWhichCounter = &m_pApp->m_nCreateGlossCount;
	}
	else
	{
		m_pApp->m_curNormalTarget = nonSrc;
		pWhichCounter = &m_pApp->m_nCreateAdaptionCount;
	}

	// Use the counters, app's m_nCreateAdaptionCount, or if currently glossing and
	// therefore nonSrc contains glosses to go into the glossing entry table as kbType = 2,
	// then use m_nCreateGlossCount. These counters, if zero, provide a processing path
	// which take the input .dat file from the dist folder, moving it to the execPath's
	// folder, clearing the contents, and replacing with the relevant data to form
	// the commandLine to be submitted to wxExecute() - and that protocol deletes the
	// old .dat input file so moved, before those steps create a replacement so filled out.
	// But if the counter is greater than zero, an alternative speedier path is followed.
	// Because the .dat file has 9 fields, and only two need new values - the src and nonSrc.
	// In this circumstance it's silly to waste time by recreating by the protocol above.
	// Instead, don't delete the old input .dat file in the exec folder, but refill just
	// the two fields that need refilling with the values pasted in from the signature.
	// That's far speedier, and important here because CreateEntry() will be called 
	// maybe tens of thousands of times over the life of the AI project. So keeping this
	// call fast will pay off in user satisfaction.

	bool bOkay = MoveOrInPlace(create_entry, m_pApp, *pWhichCounter, execPath,
								src, nonSrc, 6, 7, 8); // last 3 are locations of 
								// src field, nonSrc field, kbType field
	wxUnusedVar(bOkay);
	// Since the nonSrc content could be adaptation, or gloss, depending on whether kbType
	// currently in action is '1' or '2' respectively, the same code handles either in
	// the following blocks. However, we should check that the kbType value is set correctly.

	/* BEW 6Oct20 The MoveOrInPlace() call replaces this commented out block
	if (m_pApp->m_nCreateAdaptionCount == 0)
	{
		// Setting up m_pKbSvr for adaptations has just been done, so take
		// the longer processing path for the first entry to be added to the
		// entry table in this adapting sesson
		m_pApp->ConfigureDATfile(create_entry); // grabs m_curNormalSource & ...Target from m_pApp
		//m_pApp->m_nCreateAdaptionCount++; // subsequent entry table additions use the faster route
	}
	else
	{
		// Process quicker - just replace the src and tgt field in
		// create_entry.dat, fields 6 and 7 (1-based counting), with the
		// values passed in from the signature
		wxString datPath = execPath + m_pApp->PathSeparator + datFileName;
		bool bPresent = ::FileExists(datPath);
		if (bPresent)
		{
			wxTextFile f;
			bool bIsOpened = FALSE;
			f.Create(datPath);
			bIsOpened = f.Open();
			if (bIsOpened)
			{
				wxString cmdLine = f.GetFirstLine();
				cmdLine = ReplaceFieldWithin(cmdLine, 6, src); // 6 is 1-based
				if (gbIsGlossing)
				{
					wxString kbType = _T("2");
					cmdLine = ReplaceFieldWithin(cmdLine, 8, kbType); // 8 is 1-based
				}
				else
				{
					wxString kbType = _T("1");
					cmdLine = ReplaceFieldWithin(cmdLine, 8, kbType); // 8 is 1-based
				}
				f.RemoveLine(0);
				f.AddLine(cmdLine);
				bool bWroteOK = f.Write();
				f.Close();

				if (!bWroteOK)
				{
					wxString msg = _T("CreateEntry() failed to write cmdLine with replacement src & nonSrc fields");
					m_pApp->LogUserAction(msg);
					// Unlike, so a beep will do
					wxBell();
					return;
				}
			}
			bIsOpened = FALSE;
			f.Create(datPath);
			bIsOpened = f.Open();
			if (bIsOpened)
			{
				wxString cmdLine = f.GetFirstLine();
				cmdLine = ReplaceFieldWithin(cmdLine, 7, nonSrc); // target, or gloss
				f.RemoveLine(0);
				f.AddLine(cmdLine);
				bool bWroteOK = f.Write();
				f.Close();

				if (!bWroteOK)
				{
					wxString msg = _T("CreateEntry() failed to write cmdLine with replacement src & nonSrc fields");
					m_pApp->LogUserAction(msg);
					// Unlike, so a beep will do
					wxBell();
					return;
				}
			}
		}
		else
		{
			// file is absent from the execPath folder - tell user etc
			wxString msg;
			msg = msg.Format(_("The %s file is absent from the folder %s. Inserting the values: %s and  %s into the entry table failed. Adapting work can continue."),
				datFileName.c_str(), execPath.c_str(), src.c_str(), nonSrc.c_str());
			wxMessageBox(msg, _("Error - absent file"), wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(msg);
			return;
		}
		//m_pApp->m_nCreateAdaptionCount++; // bump the counter value
	} // end of else block for test: if (m_pApp->m_nCreateAdaptionCount == 0)
	*/
	// The create_entry.dat input file is now ready for grabbing the command
	// line (its first line) for the ::wxExecute() call in CallExecute()
	bool bOK = m_pApp->CallExecute(create_entry, execFileName, execPath,
			resultFile, 99, 99, FALSE); // FALSE is bReportResult

	return (rv = bOK == TRUE ? 0 : -1);

	/* JM's way, legacy - deprecated - retain in case we sometime build something this way
	long entryID = 0;

	s_BulkDeleteMutex.Lock();

	rv = pKbSvr->CreateEntry(src, tgt); // kbType is supplied internally from pKbSvr

	if (rv == CURLE_HTTP_RETURNED_ERROR)
	{
		// we've more work to do - may need to un-pseudodelete a pseudodeleted entry
		int rv2 = pKbSvr->LookupEntryFields(src, tgt);
		KbServerEntry e = pKbSvr->GetEntryStruct();
		entryID = e.id; // an undelete of a pseudo-delete will need this value
#if defined(SYNC_LOGS)
		wxLogDebug(_T("LookupEntryFields in CreateEntry: id = %d , source = %s , translation = %s , username = %s , deleted = %d"),
			e.id, e.source.c_str(), e.translation.c_str(), e.username.c_str(), e.deleted);
#endif
		if (rv2 == CURLE_HTTP_RETURNED_ERROR)
		{
#if defined(SYNC_LOGS)
			wxBell(); // we don't expect any error
#endif
			wxLogDebug(_T("LookupEntryFields in CreateEntry: there was an error: %d"), rv2);
		}
		else
		{
			if (e.deleted == 1)
			{
				// do an un-pseudodelete here, use the entryID value above
				// (reuse rv2, because if it fails we'll attempt nothing additional
				//  here, not even to tell the user anything)
				rv2 = pKbSvr->PseudoDeleteOrUndeleteEntry(entryID, doUndelete);

				wxLogDebug(_T("PseudoDeleteOrUndeleteEntry, was called with doUndelete enum; in CreateEntry: for entryID: %d"), entryID);
			}
		}
	}
	s_BulkDeleteMutex.Unlock();

	wxUnusedVar(rv);
	wxLogDebug(_T("CreateEntry(): On return from CreateEntry(): rv = %d  for source:  %s   &   target:  %s"), rv, src.c_str(), tgt.c_str());

	return rv;
	*/
}


int KbServer::CreateEntry(wxString srcPhrase, wxString tgtPhrase)
{

	/* BEW 30Sep20 this variant, can be chucked away, 
	// entries are always created as "normal" entries, that is, not pseudo-deleted
	CURL *curl;
	CURLcode result = CURLE_OK; // initialize result code
	struct curl_slist* headers = NULL;
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString kbType;
	int type = GetKBServerType();
	wxItoa(type,kbType);
	wxString langcode;
	if (type == 1)
	{
		langcode = GetTargetLanguageCode();
	}
	else
	{
		langcode = GetGlossLanguageCode();
	}
	wxJSONValue jsonval; // construct JSON object
	CBString strVal; // to store wxString form of the jsonval object, for curl
	wxString container = _T("entry");
	wxString aUrl, aPwd;
	str_CURLbuffer.clear(); // always make sure it is cleared for accepting new data

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	// populate the JSON object
	jsonval[_T("sourcelanguage")] = GetSourceLanguageCode();
	jsonval[_T("targetlanguage")] = langcode;
	jsonval[_T("source")] = srcPhrase;
	// BEW 11Jun15 support <noform> as standin for empty string
	wxString noform = _T("<noform>");
	if (tgtPhrase.IsEmpty())
	{
		tgtPhrase = noform;
	}
	jsonval[_T("target")] = tgtPhrase;
	jsonval[_T("user")] = GetKBServerUsername();
	jsonval[_T("type")] = kbType;
	jsonval[_T("deleted")] = (long)0; // i.e. a normal entry

	// convert it to string form
	wxJSONWriter writer; wxString str;
	writer.Write(jsonval, str);

#if defined( SYNC_LOGS)
	wxLogDebug(_T("%s"), str.c_str());
	// for Leon's use
#endif
	// convert it to utf-8 stored in CBString
	strVal = ToUtf8(str);

	aUrl = GetKBServerIpAddr() + slash + container + slash;
	charUrl = ToUtf8(aUrl);

	// prepare curl
	curl = curl_easy_init();

	if (curl)
	{
		// add headers
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "Accept: application/json");
		// set data
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)strVal);
		//curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		// ask for the headers to be prepended to the body - this is a good choice here
		// because no json data is to be returned
		curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
		// get the headers stuff this way...
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);

		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) // && defined (__WXGTK__)
        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("CreateEntry() Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif
		// The kind of error we are looking for isn't a CURLcode one, but aHTTP one
		// (400 or higher)
		ExtractHttpStatusEtc(str_CURLbuffer, m_httpStatusCode, m_httpStatusText);

		curl_slist_free_all(headers);
		str_CURLbuffer.clear();

        // Typically, result will contain CURLE_OK if an error was a HTTP one and so the
        // next block won't then be entered; don't bother to localize this one, we don't
        // expect it will happen much if at all
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("CreateEntry() result code: %d Error: %s"),
				result, error.c_str());
			wxMessageBox(msg, _T("Error when trying to create an entry"), wxICON_EXCLAMATION | wxOK);

			curl_easy_cleanup(curl);
			return result;
		}
	}
	curl_easy_cleanup(curl);

    // Work out what to return, depending on whether or not a HTTP error happened, and
    // which one it was
	if (m_httpStatusCode >= 400)
	{
		// For a CreateEntry() call, 400 "Bad Request" should be the only one we get.
		// Rather than use CURLOPT_FAILONERROR in the curl request, I'll use the HTTP
		// status codes which are returned, to determine what to do, and then manually
		// return 22 i.e. CURLE_HTTP_RETURNED_ERROR, to pass back to the caller

        // We found an existing entry, so it could be a normal one, or a pseudo-deleted
        // one, and therefore we'll return CURLE_HTTP_RETURNED_ERROR (22); this allows us
        // to check for this code in the caller and when it has been returned, to do
        // further calls in the thread before it destructs. For CreateEntry() this would
        // mean a LookupFields() to determine what the deleted flag value is, and if it's a
        // pseudo deleted entry, then we can call the function for restoring it to be a
        // normal entry - 3 calls, and heaps of latency delay, but it would be a rare
        // scenario.
		return CURLE_HTTP_RETURNED_ERROR;
	}
	*/
	return 0;
}

/* remove, no longer needed
// Pass in the params, because we use this only from the KB Sharing Manager, and there's no guarantee that
// the person doing the administrative task is the computer's normal user; we con't want administrator
// temporary access to clobber the normal user's KBserver access credentials
int	KbServer::CreateLanguage(wxString url, wxString username, wxString password, wxString langCode, wxString description)
{
	CURL *curl;
	CURLcode result = CURLE_OK; // initialize result code
	struct curl_slist* headers = NULL;
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxJSONValue jsonval; // construct JSON object
	CBString strVal; // to store wxString form of the jsonval object, for curl
	wxString container = _T("language");
	wxString aPwd, aUrl;
	str_CURLbuffer.clear(); // always make sure it is cleared for accepting new data
	str_CURLheaders.clear();

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = username + colon + password;
	charUserpwd = ToUtf8(aPwd);

	// populate the JSON object
	jsonval[_T("user")] = username;
	jsonval[_T("description")] = description;
	jsonval[_T("id")] = langCode;

	// convert it to string form
	wxJSONWriter writer; wxString str;
	writer.Write(jsonval, str);
	// convert it to utf-8 stored in CBString
	strVal = ToUtf8(str);

	aUrl = url + slash + container;
	charUrl = ToUtf8(aUrl);

	// prepare curl
	curl = curl_easy_init();

	if (curl)
	{
		// add headers
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "Accept: application/json");
		// set data
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)strVal);
		// ask for the headers to be prepended to the body - this is a good choice here
		// because no json data is to be returned
		curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
		// get the headers stuff this way...
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) // && defined (__WXGTK__)
		CBString s(str_CURLbuffer.c_str());
		wxString showit = ToUtf16(s);
		wxLogDebug(_T("\n\n *** CreateLanguage() Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif
		// The kind of error we are looking for isn't a CURLcode one, but aHTTP one
		// (400 or higher)
		ExtractHttpStatusEtc(str_CURLbuffer, m_httpStatusCode, m_httpStatusText);

		curl_slist_free_all(headers);
		str_CURLbuffer.clear();

		// Typically, result will contain CURLE_OK if an error was a HTTP one and so the
		// next block won't then be entered; don't bother to localize this one, we don't
		// expect it will happen much if at all
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("CreateLanguage() result code: %d Error: %s"),
				result, error.c_str());
			wxMessageBox(msg, _T("Error when creating a language in the language table"), wxICON_EXCLAMATION | wxOK);

			curl_easy_cleanup(curl);
			return result;
		}
	}
	curl_easy_cleanup(curl);

	// Work out what to return, depending on whether or not a HTTP error happened, and
	// which one it was
	if (m_httpStatusCode >= 400)
	{
		// For a CreateEntry() call, 400 "Bad Request" should be the only one we get.
		// Rather than use CURLOPT_FAILONERROR in the curl request, I'll use the HTTP
		// status codes which are returned, to determine what to do, and then manually
		// return 22 i.e. CURLE_HTTP_RETURNED_ERROR, to pass back to the caller

		// We found an existing entry, so it could be a normal one, or a pseudo-deleted
		// one, and therefore we'll return CURLE_HTTP_RETURNED_ERROR (22); this allows us
		// to check for this code in the caller and when it has been returned, to do
		// further calls in the thread before it destructs. For CreateEntry() this would
		// mean a LookupFields() to determine what the deleted flag value is, and if it's a
		// pseudo deleted entry, then we can call the function for restoring it to be a
		// normal entry - 3 calls, and heaps of latency delay, but it would be a rare
		// scenario.
		return CURLE_HTTP_RETURNED_ERROR;
	}
	return 0; // no error
}
*/
/* BEW 9Dec20 removed, we do it a bit differently with Leon's solution
int	KbServer::CreateUser(wxString username, wxString fullname, wxString hisPassword, bool bUseradmin)
{
	// TODO? - Leon's way  -- probably when I get to work on the simpler KB Sharing Manager

	CURL *curl;
	CURLcode result = CURLE_OK; // initialize result code
	struct curl_slist* headers = NULL;
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxJSONValue jsonval; // construct JSON object
	CBString strVal; // to store wxString form of the jsonval object, for curl
	wxString container = _T("user");
	wxString aUrl, aPwd;
	str_CURLbuffer.clear(); // always make sure it is cleared for accepting new data

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	// populate the JSON object
	jsonval[_T("username")] = username;
	jsonval[_T("fullname")] = fullname;
	// *******************************************
	// Create digest hash here from password passed in; sb prefix means 'single byte (encoded)'
	CBString sbPassword = ToUtf8(hisPassword);
	CBString sbUsername = ToUtf8(username);
	CBString realm = "kbserver";
	CBString sbColon = ":";
	CBString sbDigest = sbUsername + sbColon + realm + sbColon + sbPassword;
	sbDigest = md5_SB::GetMD5(sbDigest);
	// wxJson will need it as a UTF-16 string
	wxString myDigest = ToUtf16(sbDigest); // it's null-byte extended to UTF16 format
	// *******************************************
	jsonval[_T("password")] = myDigest;
	long kbadmin = bKbadmin ? 1L : 0L;
	jsonval[_T("kbadmin")] = kbadmin;
	long useradmin = bUseradmin ? 1L : 0L;
	jsonval[_T("useradmin")] = useradmin;
	long languageadmin = bLanguageadmin ? 1L : 0L;
	jsonval[_T("languageadmin")] = languageadmin;

	// convert it to string form
	wxJSONWriter writer; wxString str;
	writer.Write(jsonval, str);
	// convert it to utf-8 stored in CBString
	strVal = ToUtf8(str);

	aUrl = GetKBServerIpAddr() + slash + container + slash;
	charUrl = ToUtf8(aUrl);

	// prepare curl
	curl = curl_easy_init();

	if (curl)
	{
		// add headers
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "Accept: application/json");
		// set data
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)strVal);
		// ask for the headers to be prepended to the body - this is a good choice here
		// because no json data is to be returned
		curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
		// get the headers stuff this way...
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) // && defined (__WXGTK__)
        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("\n\n *** CreateUser() Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif
		// The kind of error we are looking for isn't a CURLcode one, but aHTTP one
		// (400 or higher)
		ExtractHttpStatusEtc(str_CURLbuffer, m_httpStatusCode, m_httpStatusText);

		curl_slist_free_all(headers);
		str_CURLbuffer.clear();

        // Typically, result will contain CURLE_OK if an error was a HTTP one and so the
        // next block won't then be entered; don't bother to localize this one, we don't
        // expect it will happen much if at all
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("CreateUser() result code: %d Error: %s"),
				result, error.c_str());
			wxMessageBox(msg, _T("Error when creating a user in the user table"), wxICON_EXCLAMATION | wxOK);

			curl_easy_cleanup(curl);
			return result;
		}
	}
	curl_easy_cleanup(curl);

    // Work out what to return, depending on whether or not a HTTP error happened, and
    // which one it was
	if (m_httpStatusCode >= 400)
	{
		// For a CreateEntry() call, 400 "Bad Request" should be the only one we get.
		// Rather than use CURLOPT_FAILONERROR in the curl request, I'll use the HTTP
		// status codes which are returned, to determine what to do, and then manually
		// return 22 i.e. CURLE_HTTP_RETURNED_ERROR, to pass back to the caller

        // We found an existing entry, so it could be a normal one, or a pseudo-deleted
        // one, and therefore we'll return CURLE_HTTP_RETURNED_ERROR (22); this allows us
        // to check for this code in the caller and when it has been returned, to do
        // further calls in the thread before it destructs. For CreateEntry() this would
        // mean a LookupFields() to determine what the deleted flag value is, and if it's a
        // pseudo deleted entry, then we can call the function for restoring it to be a
        // normal entry - 3 calls, and heaps of latency delay, but it would be a rare
        // scenario.
		return CURLE_HTTP_RETURNED_ERROR;
	}

	return 0; // no error
}
*/
/*
// Note: url, username and password are passed in, because this request can be made before
// the app's m_pKbServer[2] pointers have been instantiated
int KbServer::ReadLanguage(wxString url, wxString username, wxString password, wxString languageCode)
{
	CURL *curl;
	CURLcode result;
	wxString aUrl; // convert to utf8 when constructed
	wxString aPwd; // ditto
	str_CURLbuffer.clear();
	str_CURLheaders.clear();

	CBString charUrl;
	CBString charUserpwd;

	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString container = _T("language");

	aUrl = url + slash + container + slash + languageCode;
	charUrl = ToUtf8(aUrl);
	aPwd = username + colon + password;
	charUserpwd = ToUtf8(aPwd);

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);
		// We want separate storage for headers to be returned, to get the HTTP status code
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &curl_headers_callback);
		//curl_easy_setopt(curl, CURLOPT_HEADER, 1L); // comment out when collecting
		//headers separately
		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) //&& defined (__WXGTK__)
		CBString s2(str_CURLheaders.c_str());
		wxString showit2 = ToUtf16(s2);
		wxLogDebug(_T("ReadLanguage(): Returned headers: %s"), showit2.c_str());

		CBString s(str_CURLbuffer.c_str());
		wxString showit = ToUtf16(s);
		wxLogDebug(_T("ReadLanguage() str_CURLbuffer has: %s    , The CURLcode is: %d"),
			showit.c_str(), (unsigned int)result);
#endif
		// Get the HTTP status code, and the English message
		//ExtractHttpStatusEtc(str_CURLheaders, m_httpStatusCode, m_httpStatusText); BEW commented out 2Nov20

		// If the only error was a HTTP one, then result will contain CURLE_OK, in which
		// case the next block is skipped
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("ReadLanguage() result code: %d cURL Error: %s"),
				result, error.c_str());
			wxMessageBox(msg, _T("Error when reading the language table to get a language definition, using ReadLanguage()"), wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(msg);
			curl_easy_cleanup(curl);
			//curl_free(encodeduser);
			return (int)result;
		}
	}
	curl_easy_cleanup(curl);

	// If there was a HTTP error -- typically, it would be 404 Not Found, because the
	// language code is not in the language table yet; if so then exit early, & there
	// won't be json data to handle if that was the case
	if (m_httpStatusCode >= 400)
	{
		// whether 400 or 404, return CURLE_HTTP_RETURNED_ERROR (ie. 22) to the caller
		ClearUserStruct();
		str_CURLbuffer.clear();
		str_CURLheaders.clear();
		//curl_free(encodeduser);
		return CURLE_HTTP_RETURNED_ERROR; // 22
	}

	// That takes care of what happens when the lookup did not find the wanted language code
	// in the table. Now we look after what happens if it succeeded. Json will be returned.

	//  Make the json data accessible (result is CURLE_OK if control gets to here)
	//  We requested separate headers callback be used, so str_CURLbuffer should only have
	//  the json string for the looked up entry
	if (!str_CURLbuffer.empty())
	{
		CBString jsonArray(str_CURLbuffer.c_str()); // JSON expects byte data, convert after Parse()
		wxJSONValue jsonval;
		wxJSONReader reader;
		int numErrors = reader.Parse(ToUtf16(jsonArray), &jsonval);
		if (numErrors > 0)
		{
			// Write to a file, in the _LOGS_EMAIL_REPORTS folder, whatever was sent,
			// the developers would love to have this info. The latest copy only is
			// retained, the "w" mode clears the earlier file if there is one, and
			// writes new content to it
			wxString aFilename = _T("ReadLanguage_bad_data_sent_to ") + m_pApp->m_curProjectName + _T(".txt");
			wxString workOrCustomFolderPath;
			if (::wxDirExists(m_pApp->m_logsEmailReportsFolderPath))
			{
				wxASSERT(!m_pApp->m_curProjectName.IsEmpty());

				if (!m_pApp->m_bUseCustomWorkFolderPath)
				{
					workOrCustomFolderPath = m_pApp->m_workFolderPath;
				}
				else
				{
					workOrCustomFolderPath = m_pApp->m_customWorkFolderPath;
				}
				wxString path2BadData = workOrCustomFolderPath + m_pApp->PathSeparator +
					m_pApp->m_logsEmailReportsFolderName + m_pApp->PathSeparator + aFilename;
                wxString mode = _T('w');
                size_t mySize = str_CURLbuffer.size();
                wxFFile ff(path2BadData.GetData(), mode.GetData());
				wxASSERT(ff.IsOpened());
				ff.Write(str_CURLbuffer.c_str(), mySize);
				ff.Close();
			}

			// a non-localizable message will do, it's unlikely to ever be seen
			wxString msg;
			msg = msg.Format(_T("ReadLanguage(): json reader.Parse() failed. Server sent bad data.\nThe bad data is stored in the file with name: \n%s \nLocated at the folder: %s \nSend this file to the developers please."),
				aFilename.c_str(), m_pApp->m_logsEmailReportsFolderPath.c_str());
			wxMessageBox(msg, _T("KBserver error"), wxICON_ERROR | wxOK);

			str_CURLbuffer.clear(); // always clear it before returning
			str_CURLheaders.clear();
			return -1;
		}
		// We extract id (the code), username, description, and the timestamp at which the definition
		// was added to the language table.
		ClearLanguageStruct(); // re-initializes private member, m_languageStruct, to be empty strings
		m_languageStruct.code = jsonval[_T("id")].AsString();
		m_languageStruct.username = jsonval[_T("username")].AsString();
		m_languageStruct.description = jsonval[_T("fullname")].AsString();
		m_userStruct.timestamp = jsonval[_T("timestamp")].AsString();

		str_CURLbuffer.clear(); // always clear it before returning
		str_CURLheaders.clear();
	}
	return 0; // CURLE_OK
}
*/

/*
int KbServer::CreateKb(wxString ipAddr, wxString username, wxString password, 
			wxString srcLangName, wxString nonsrcLangName, bool bKbTypeIsScrTgt)
{
 // no longer needed 12Oct20
	// entries are always created as "normal" entries, that is, not pseudo-deleted
	wxASSERT(!srcLangCode.IsEmpty() && !nonsrcLangCode.IsEmpty());
	CURL *curl;
	CURLcode result = CURLE_OK; // initialize result code
	struct curl_slist* headers = NULL;
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString kbType;
	if (bKbTypeIsScrTgt) { kbType = _T("1"); } else { kbType = _T("2"); }
	wxJSONValue jsonval; // construct JSON object
	CBString strVal; // to store wxString form of the jsonval object, for curl
	wxString container = _T("kb");
	wxString aUrl, aPwd;
	str_CURLbuffer.clear(); // always make sure it is cleared for accepting new data

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	// populate the JSON object
	jsonval[_T("sourcelanguage")] = srcLangCode;
	jsonval[_T("targetlanguage")] = nonsrcLangCode;
	jsonval[_T("type")] = kbType;
	jsonval[_T("user")] = GetKBServerUsername();
	// No need for the "deleted" flag, for a new KB definition it defaults to 0

	// convert it to string form
	wxJSONWriter writer; wxString str;
	writer.Write(jsonval, str);
	// convert it to utf-8 stored in CBString
	strVal = ToUtf8(str);

	aUrl = GetKBServerIpAddr() + slash + container;
	charUrl = ToUtf8(aUrl);

	// prepare curl
	curl = curl_easy_init();

	if (curl)
	{
		// add headers
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "Accept: application/json");
		// set data
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)strVal);
		// ask for the headers to be prepended to the body - this is a good choice here
		// because no json data is to be returned
		curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
		// get the headers stuff this way...
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);

		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) // && defined (__WXGTK__)
        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("CreateKb() Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif
		// The kind of error we are looking for isn't a CURLcode one, but aHTTP one
		// (400 or higher)
		ExtractHttpStatusEtc(str_CURLbuffer, m_httpStatusCode, m_httpStatusText);

		curl_slist_free_all(headers);
		str_CURLbuffer.clear();

        // Typically, result will contain CURLE_OK if an error was a HTTP one and so the
        // next block won't then be entered; don't bother to localize this one, we don't
        // expect it will happen much if at all
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("CreateKb() result code: %d Error: %s"),
				result, error.c_str());
			wxMessageBox(msg, _T("Error creating a KB definition for kb table"), wxICON_EXCLAMATION | wxOK);

			curl_easy_cleanup(curl);
			return result;
		}
	}
	curl_easy_cleanup(curl);

    // Work out what to return, depending on whether or not a HTTP error happened, and
    // which one it was
	if (m_httpStatusCode >= 400)
	{
		// For a CreateEntry() call, 400 "Bad Request" should be the only one we get.
		// Rather than use CURLOPT_FAILONERROR in the curl request, I'll use the HTTP
		// status codes which are returned, to determine what to do, and then manually
		// return 22 i.e. CURLE_HTTP_RETURNED_ERROR, to pass back to the caller
		return CURLE_HTTP_RETURNED_ERROR;
	}

	return 0;
}
*/

/* BEW 10Dec20 deprecated
int KbServer::UpdateUser(int userID, bool bUpdateUsername, bool bUpdateFullName,
						bool bUpdatePassword, bool bUpdateKbadmin, bool bUpdateUseradmin,
						KbServerUser* pEditedUserStruct, wxString password)
{

	// TODO ?? need it?  --- probably when I get to refactor the simpler KB Sharing Manager
	CURLcode result = CURLE_OK;
	wxString userIDStr;
	wxItoa(userID, userIDStr);
	CURL *curl;
	struct curl_slist* headers = NULL;
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString kbType;
	wxItoa(GetKBServerType(),kbType);
	wxJSONValue jsonval; // construct JSON object
	CBString strVal; // to store wxString form of the jsonval object, for curl
	wxString container = _T("user");
	wxString aUrl, aPwd;

	// Get the username into a local variable, even if we haven't edited it, it may be
	// needed for a digest calculation
	wxString theUsername = pEditedUserStruct->username;

	str_CURLbuffer.clear(); // use for headers return when there's no json to be returned

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	// populate the JSON object
	if (bUpdateUsername)
	{
		jsonval[_T("username")] = theUsername;
	}
	if (bUpdateFullName)
	{
		jsonval[_T("fullname")] = pEditedUserStruct->fullname;
	}
	if (bUpdatePassword)
	{
		// using digest created here from password passed in; sb
		// prefix means 'single byte (encoded)'
		CBString sbPassword = ToUtf8(password);
		CBString sbUsername = ToUtf8(theUsername);
		CBString realm = "kbserver";
		CBString sbColon = ":";
		CBString sbDigest = sbUsername + sbColon + realm + sbColon + sbPassword;
		sbDigest = md5_SB::GetMD5(sbDigest);
		// wxJson will need it as a UTF-16 string
		wxString myDigest = ToUtf16(sbDigest); // it's null-byte extended to UTF16 format
		jsonval[_T("password")] = myDigest;
	}
	if (bUpdateKbadmin)
	{
		long value = pEditedUserStruct->kbadmin ? 1L : 0L;
		jsonval[_T("kbadmin")] = value;
	}
	if (bUpdateUseradmin)
	{
		long value = pEditedUserStruct->useradmin ? 1L : 0L;
		jsonval[_T("useradmin")] = value;
	}

	// convert it to string form
	wxJSONWriter writer; wxString str;
	writer.Write(jsonval, str);
	// convert it to utf-8 stored in CBString
	strVal = ToUtf8(str);

	aUrl = GetKBServerIpAddr() + slash + container + slash + userIDStr;
	charUrl = ToUtf8(aUrl);

	// prepare curl
	curl = curl_easy_init();

	if (curl)
	{
		// add headers
		headers = curl_slist_append(headers, "Accept: application/json");
		headers = curl_slist_append(headers, "Content-Type: application/json");
		// set data
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT"); // this way avoids turning on file processing
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)strVal);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
		// get the headers stuff this way when no json is expected back...
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);

		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) // && defined (__WXGTK__)
        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("\n\n *** UpdateUser() Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif
		// The kind of error we are looking for isn't a CURLcode one, but a HTTP one
		// (400 or higher)
		ExtractHttpStatusEtc(str_CURLbuffer, m_httpStatusCode, m_httpStatusText);

		curl_slist_free_all(headers);
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("UpdateUser() result code: %d Error: %s"), result, error.c_str());
			wxMessageBox(msg, _T("Error when trying to update a user entry"), wxICON_EXCLAMATION | wxOK);
			curl_easy_cleanup(curl);
			str_CURLbuffer.clear();
			return result;
		}
	}
	curl_easy_cleanup(curl);
	str_CURLbuffer.clear();

	// handle any HTTP error code, if one was returned
	if (m_httpStatusCode >= 400)
	{
		// We may get 400 "Bad Request" or 404 Not Found (both should be unlikely)
		// Rather than use CURLOPT_FAILONERROR in the curl request, I'll use the HTTP
		// status codes which are returned, to determine what to do, and then manually
		// return 22 i.e. CURLE_HTTP_RETURNED_ERROR, to pass back to the caller
		return CURLE_HTTP_RETURNED_ERROR;
	}

	return 0;
}
*/

/* deprecated JM's way, BEW 2Nov20
// This one is like RemoveUser() and RemoveKb(), and http errors need to be checked for,
// as it is the only way to know when the deletion loop has deleted all that need to be
// deleted
int KbServer::DeleteSingleKbEntry(int entryID)
{
	wxString entryIDStr;
	wxItoa(entryID, entryIDStr);
	CURL *curl;
	CURLcode result; // result code
	struct curl_slist* headers = NULL; // get headers in case of HTTP error
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString container = _T("entry");
	wxString aUrl, aPwd;

	str_CURLbuffer_for_deletekb.clear(); // use for headers return when there's no json to be returned

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	aUrl = GetKBServerIpAddr() + slash + container + slash + entryIDStr;
	charUrl = ToUtf8(aUrl);

		// prepare curl
	curl = curl_easy_init();

	result = (CURLcode)0; // initialize
	if (curl)
	{
		// add headers
		headers = curl_slist_append(headers, "Accept: application/json");
		headers = curl_slist_append(headers, "Content-Type: application/json");
		// set data
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
		// get the headers stuff this way when no json is expected back...
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback_for_deletekb);

		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS)
        CBString s(str_CURLbuffer_for_deletekb.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("\n\n DeleteSingleEntry() Returned: %s    CURLcode %d  for entryID = %d"),
					showit.c_str(), (unsigned int)result, entryID);
#endif
		// The kind of error we are looking for isn't a CURLcode one, but a HTTP one
		// (400 or higher)
		//ExtractHttpStatusEtc(str_CURLbuffer_for_deletekb, m_httpStatusCode, m_httpStatusText); BEW commented out 2Nov20

		curl_slist_free_all(headers);
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("DeleteSingleKbEntry() result code: %d Error: %s  for entryID = %d"),
				result, error.c_str(), entryID);
			wxMessageBox(msg, _T("Error deleting a single entry"), wxICON_EXCLAMATION | wxOK);
			curl_easy_cleanup(curl);
			return result;
		}
	}
	curl_easy_cleanup(curl);
	str_CURLbuffer_for_deletekb.clear();

	// handle any HTTP error code, if one was returned
	if (m_httpStatusCode >= 400)
	{
		// Most likely, a 404 Not Found error when the parent loop has finally
		// deleted the last entry in that particular kb database
		// Rather than use CURLOPT_FAILONERROR in the curl request, I'll use the HTTP
		// status codes which are returned, to determine what to do, and then manually
		// return 22 i.e. CURLE_HTTP_RETURNED_ERROR, to pass back to the caller
		return CURLE_HTTP_RETURNED_ERROR;
	}
	return (CURLcode)0;
}
*/

/* BEW 10Dec20 deprecated, we don't allow this any more
int KbServer::RemoveUser(int userID)
{

	// TODO  Leon's way  --- probably when I get to refactor the KB Sharing Manager

	wxString userIDStr;
	wxItoa(userID, userIDStr);
	CURL *curl;
	CURLcode result; // result code
	struct curl_slist* headers = NULL; // get headers in case of HTTP error
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString container = _T("user");
	wxString aUrl, aPwd;

	str_CURLbuffer.clear(); // use for headers return when there's no json to be returned

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	aUrl = GetKBServerIpAddr() + slash + container + slash + userIDStr;
	charUrl = ToUtf8(aUrl);

	// prepare curl
	curl = curl_easy_init();

	if (curl)
	{
		// add headers
		headers = curl_slist_append(headers, "Accept: application/json");
		headers = curl_slist_append(headers, "Content-Type: application/json");
		// set data
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
		// get the headers stuff this way when no json is expected back...
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);

		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS)
        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("\n\n *** RemoveUser() Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif
		// The kind of error we are looking for isn't a CURLcode one, but a HTTP one
		// (400 or higher)
		ExtractHttpStatusEtc(str_CURLbuffer, m_httpStatusCode, m_httpStatusText);

		curl_slist_free_all(headers);
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("RemoveUser() result code: %d Error: %s"), result, error.c_str());
			wxMessageBox(msg, _T("Error when deleting a user"), wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(msg);
			curl_easy_cleanup(curl);
			str_CURLbuffer.clear();
			return result;
		}
	}
	curl_easy_cleanup(curl);
	str_CURLbuffer.clear();

	// handle any HTTP error code, if one was returned
	if (m_httpStatusCode >= 400)
	{
		// We may get 400 "Bad Request" or 404 Not Found (both should be unlikely)
		// Rather than use CURLOPT_FAILONERROR in the curl request, I'll use the HTTP
		// status codes which are returned, to determine what to do, and then manually
		// return 22 i.e. CURLE_HTTP_RETURNED_ERROR, to pass back to the caller
		return CURLE_HTTP_RETURNED_ERROR;
	}
	return (CURLcode)0; // no error
}
*/

/* BEW 2Nov20 deprecated
int KbServer::RemoveKb(int kbID)
{

	wxString kbIDStr;
	wxItoa(kbID, kbIDStr);
	CURL *curl;
	CURLcode result; // result code
	struct curl_slist* headers = NULL;
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString container = _T("kb");
	wxString aUrl, aPwd;

	str_CURLbuffer_for_deletekb.clear(); // use for headers return when there's no json to be returned

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	aUrl = GetKBServerIpAddr() + slash + container + slash + kbIDStr;
	charUrl = ToUtf8(aUrl);

		// prepare curl
	curl = curl_easy_init();

	if (curl)
	{
		// add headers
		headers = curl_slist_append(headers, "Accept: application/json");
		headers = curl_slist_append(headers, "Content-Type: application/json");
		// set data
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
		// get the headers stuff this way when no json is expected back...
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback_for_deletekb);

		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS)
        CBString s(str_CURLbuffer_for_deletekb.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("\n\n *** RemoveKb() Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif
		// The kind of error we are looking for isn't a CURLcode one, but a HTTP one
		// (400 or higher)
		//ExtractHttpStatusEtc(str_CURLbuffer_for_deletekb, m_httpStatusCode, m_httpStatusText); BEW commented out 2Nov20

		curl_slist_free_all(headers);
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("RemoveKb() result code: %d Error: %s"), result, error.c_str());
			wxMessageBox(msg, _T("Error when deleting a KB definition"), wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(msg);
			curl_easy_cleanup(curl);
			str_CURLbuffer_for_deletekb.clear();
			return result;
		}
	}
	curl_easy_cleanup(curl);
	str_CURLbuffer_for_deletekb.clear();

	// handle any HTTP error code, if one was returned
	if (m_httpStatusCode >= 400)
	{
		// We may get 400 "Bad Request" or 404 Not Found (both should be unlikely)
		// Rather than use CURLOPT_FAILONERROR in the curl request, I'll use the HTTP
		// status codes which are returned, to determine what to do, and then manually
		// return 22 i.e. CURLE_HTTP_RETURNED_ERROR, to pass back to the caller
		return CURLE_HTTP_RETURNED_ERROR;
	}
	return (CURLcode)0; // no error
}
*/

int KbServer::RemoveCustomLanguage(wxString langID)
{

	/*
	CURL *curl;
	CURLcode result; // result code
	struct curl_slist* headers = NULL;
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString container = _T("language");
	wxString aUrl, aPwd;

	str_CURLbuffer.clear(); // use for headers return when there's no json to be returned

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	aUrl = GetKBServerIpAddr() + slash + container + slash + langID;
	charUrl = ToUtf8(aUrl);

	// prepare curl
	curl = curl_easy_init();

	if (curl)
	{
		// add headers
		headers = curl_slist_append(headers, "Accept: application/json");
		headers = curl_slist_append(headers, "Content-Type: application/json");
		// set data
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
		// get the headers stuff this way when no json is expected back...
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);
		//curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS)
		CBString s(str_CURLbuffer.c_str());
		wxString showit = ToUtf16(s);
		wxLogDebug(_T("\n\n *** RemoveCustomLanguage() Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif
		// The kind of error we are looking for isn't a CURLcode one, but a HTTP one
		// (400 or higher)
		//ExtractHttpStatusEtc(str_CURLbuffer, m_httpStatusCode, m_httpStatusText); BEW commented out 2Nov20

		curl_slist_free_all(headers);
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("RemoveCustomLanguage() result code: %d Error: %s"), result, error.c_str());
			wxMessageBox(msg, _T("Error when deleting a custom language definition"), wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(msg);
			curl_easy_cleanup(curl);
			str_CURLbuffer.clear();
			return result;
		}
	}
	curl_easy_cleanup(curl);
	str_CURLbuffer.clear();

	// handle any HTTP error code, if one was returned
	if (m_httpStatusCode >= 400)
	{
		// We may get 400 "Bad Request" or 404 Not Found (both should be unlikely)
		// Rather than use CURLOPT_FAILONERROR in the curl request, I'll use the HTTP
		// status codes which are returned, to determine what to do, and then manually
		// return 22 i.e. CURLE_HTTP_RETURNED_ERROR, to pass back to the caller
		return CURLE_HTTP_RETURNED_ERROR;
	}
	return (CURLcode)0; // no error
	*/
	return 0;
}


// BEW 7May16, we are forced to synchronous calls that use openssl, because the latter
// (if put on a detached thread) leak about a kilobyte per KBserver access, but if not
// on a thread there is no leak. Joinable threads are no solution because even if they
// didn't incur openssl leaks, the calling thread has to .Wait() for the joinable thread
// to finish. So that's no different than a synchronous call such as this one.
int KbServer::PseudoUndelete(KbServer* pKbSvr, wxString src, wxString nonSrc)
{
	wxUnusedVar(pKbSvr);

	int rv = 0; //initialise
	wxString execFileName = _T("do_pseudo_undelete.exe");
	wxString resultFile = _T("pseudo_undelete_return_results.dat");
	wxString datFileName = _T("pseudo_undelete.dat");
	wxString execPath;
	if (m_pApp->m_curExecPath.IsEmpty())
	{
		m_pApp->m_curExecPath = m_pApp->execPath;
	}
	execPath = m_pApp->m_curExecPath;

	int* pWhichCounter = NULL; // initialise

	// First, store src and nonSrc on the relevant app variables, (m_curNormalAdaption),
	// so ConfigureDATfile(pseudo_undelete.dat)'s ConfigureMovedDatFile() can grab them;
	// or if in glossing mode, store gloss on m_curNormalGloss
	m_pApp->m_curNormalSource = src;

	if (gbIsGlossing)
	{
		m_pApp->m_curNormalGloss = nonSrc;
		pWhichCounter = &m_pApp->m_nPseudoUndeleteGlossCount;
	}
	else
	{
		m_pApp->m_curNormalTarget = nonSrc;
		pWhichCounter = &m_pApp->m_nPseudoUndeleteAdaptionCount;
	}
	bool bOkay = MoveOrInPlace(pseudo_undelete, m_pApp, *pWhichCounter, execPath,
		src, nonSrc, 6, 7, 8); // last 3 are locations of 
							   // src field, nonSrc field, kbType field
	// Since the nonSrc content could be adaptation, or gloss, depending on whether kbType
	// currently in action is '1' or '2' respectively, the same code handles either in
	// the following blocks. However, we should check that the kbType value is set correctly.
	wxUnusedVar(bOkay);
	// The pseudo_undelete.dat input file is now ready for grabbing the command
	// line (its first line) for the ::wxExecute() call in CallExecute()
	bool bOK = m_pApp->CallExecute(pseudo_undelete, execFileName, execPath,
		resultFile, 99, 99, FALSE); // FALSE is bReportResult
	if (!bOK)
	{
		rv = -1;
	}
	return rv;

	/*
	int rv;
	long entryID = 0;

	wxASSERT(!src.IsEmpty()); // the key must never be an empty string
	rv = pKbSvr->LookupEntryFields(src, tgt);

	s_BulkDeleteMutex.Lock();

	if (rv == CURLE_HTTP_RETURNED_ERROR)
	{
		// we've more work to do - if the lookup failed, we must assume it was because
		// there was no matching entry (ie. HTTP 404 was returned) - in which case we
		// should attempt to create it (so as to be in sync with the change just done in
		// the local KB due to the undeletion); if the creation fails, just give up
		rv = pKbSvr->CreateEntry(src, tgt); // kbType is supplied internally from m_pKbSvr
	}
	else
	{
		// no error from the lookup, so get the entry ID, and the value of the deleted flag
		KbServerEntry e = pKbSvr->GetEntryStruct(); // accesses m_entryStruct
		entryID = e.id; // an undelete of a pseudo-delete will need this value
#if defined(SYNC_LOGS)
		wxLogDebug(_T("LookupEntryFields in PseudoUndelete(): id = %d , source = %s , translation = %s , deleted = %d , username = %s"),
			e.id, e.source.c_str(), e.translation.c_str(), e.deleted, e.username.c_str());
#endif
		// If the remote entry has 1 for the deleted flag's value, then go ahead and
		// undelete it; but if it has 0 already, there is nothing to do
		if (e.deleted == 1)
		{
			// do an un-pseudodelete here, use the entryID value above (reuse rv)
			rv = pKbSvr->PseudoDeleteOrUndeleteEntry(entryID, doUndelete);
		}
	}
	s_BulkDeleteMutex.Unlock();

	wxUnusedVar(rv);
#if defined(SYNC_LOGS)
	wxLogDebug(_T("PseudoUndelete(): returning: rv = %d  for source:  %s   &   target:  %s"), rv, src.c_str(), tgt.c_str());
#endif
	return rv;
	*/
}

int KbServer::PseudoDelete(KbServer* pKbSvr, wxString src, wxString nonSrc)
{
	wxUnusedVar(pKbSvr);

	int rv = 0; //initialise
	wxString execFileName = _T("do_pseudo_delete.exe");
	wxString resultFile = _T("pseudo_delete_return_results.dat");
	wxString datFileName = _T("pseudo_delete.dat");
	wxString execPath;
	if (m_pApp->m_curExecPath.IsEmpty())
	{
		m_pApp->m_curExecPath = m_pApp->execPath;
	}
	execPath = m_pApp->m_curExecPath;

	int* pWhichCounter = NULL; // initialise

	// First, store src and nonSrc on the relevant app variables, (m_curNormalAdaption),
	// so ConfigureDATfile(pseudo_delete.dat)'s ConfigureMovedDatFile() can grab them;
	// or if in glossing mode, store gloss on m_curNormalGloss
	m_pApp->m_curNormalSource = src;

	if (gbIsGlossing)
	{
		m_pApp->m_curNormalGloss = nonSrc;
		pWhichCounter = &m_pApp->m_nPseudoDeleteGlossCount;
	}
	else
	{
		m_pApp->m_curNormalTarget = nonSrc;
		pWhichCounter = &m_pApp->m_nPseudoDeleteAdaptionCount;
	}
	bool bOkay = MoveOrInPlace(pseudo_delete, m_pApp, *pWhichCounter, execPath,
		src, nonSrc, 6, 7, 8); // last 3 are locations of 
							   // src field, nonSrc field, kbType field
	// Since the nonSrc content could be adaptation, or gloss, depending on whether kbType
	// currently in action is '1' or '2' respectively, the same code handles either in
	// the following blocks. However, we should check that the kbType value is set correctly.
	wxUnusedVar(bOkay);
	// The pseudo_delete.dat input file is now ready for grabbing the command
	// line (its first line) for the ::wxExecute() call in CallExecute()
	bool bOK = m_pApp->CallExecute(pseudo_delete, execFileName, execPath,
		resultFile, 99, 99, FALSE); // FALSE is bReportResult
	if (!bOK)
	{
		rv = -1;
	}
	return rv;

/* Legacy JM solution - remove later
	long entryID = 0; // initialize (it might not be used)
	wxASSERT(!src.IsEmpty()); // the key must never be an empty string
	int rv;

	s_BulkDeleteMutex.Lock();

	rv = pKbSvr->LookupEntryFields(src, tgt);
	if (rv == CURLE_HTTP_RETURNED_ERROR)
	{
		// If the lookup failed, we must assume it was because there was no matching entry
		// (ie. HTTP 404 was returned) - in which case we will do no more
		// The alternative would be the following, which is much too much to be
		// worth the bother....
		// 1. create a normal entry using CreateEntry()
		// 2. look it up using LookupEntryFields() to get the entry's ID
		// 3. pseudo-delete the new entry using PseudoDeleteOrUndeleteEntry(), passing in 
		// doDelete enum value -- a total of 4 latency-laden calls. No way! (The chance of
		// the initial lookup failing is rather unlikely, because pseudo-deleting is only
		// done via the KB Editor dialog, and that should have put the remote database
		// into the correct state previously, making an error here unlikely.)
		return 1; // 1 would mean 'an error of some kind' - our callers ignore them anyway
	}
	else
	{
		// No error from the lookup, so get the entry ID, and the value of the deleted flag
		KbServerEntry e = pKbSvr->GetEntryStruct(); // accesses m_entryStruct
		entryID = e.id; // an delete of a normal entry will need this value
#if defined(SYNC_LOGS)
		wxLogDebug(_T("LookupEntryFields in PseudoDelete: id = %d , source = %s , translation = %s , deleted = %d , username = %s"),
			e.id, e.source.c_str(), e.translation.c_str(), e.deleted, e.username.c_str());
#endif
		// If the remote entry has 0 for the deleted flag's value, then go ahead and
		// delete it; but if it has 1 already, there is nothing to do except let the
		// thread die
		if (e.deleted == 0)
		{
			// do a pseudo-delete here, use the entryID value above (reuse rv)
			rv = pKbSvr->PseudoDeleteOrUndeleteEntry(entryID, doDelete);
		}
	}

	s_BulkDeleteMutex.Unlock();
	return 0;
	*/
}

/* BEW 21Oct20 unneeded
int KbServer::ChangedSince_Queued(KbServer* pKbSvr) // <<-- deprecate?, is it too slow
{
	// Note: the static s_QueueMutex is used within ChangedSince_Queued() at the point
	// where an entry (in the form of a pointer to struct) is being added to the end of
	// the queue m_queue in the m_pKbSvr instance
	wxString timeStamp = pKbSvr->GetKBServerLastSync();

	s_BulkDeleteMutex.Lock();

	int rv = pKbSvr->ChangedSince_Queued(timeStamp); // 2nd param is default TRUE
	s_BulkDeleteMutex.Unlock();

	// Error handling is at a lower level, so caller ignores the returned rv value
	return rv;
}
*/

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
/* BEW 21Oct20, removed
int KbServer::ChangedSince_Timed(KbServer* pKbSvr)
{
	// Note: the static s_QueueMutex is used within ChangedSince_Queued() at the point
	// where an entry (in the form of a pointer to struct) is being added to the end of
	// the queue m_queue in the m_pKbSvr instance
	wxString timeStamp = pKbSvr->GetKBServerLastSync();

	s_BulkDeleteMutex.Lock();

	int rv = pKbSvr->ChangedSince_Timed(timeStamp); // 2nd param is default TRUE

	s_BulkDeleteMutex.Unlock();

	// Error handling is at a lower level, so caller ignores the returned rv value
	return rv;
}
*/
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

	int rv = 0;

	wxASSERT(!src.IsEmpty()); // the key must never be an empty string

	s_BulkDeleteMutex.Lock();

	rv = pKbSvr->LookupEntryFields(src, oldText);
	wxASSERT(rv == 0);
	if (m_entryStruct.id == _T('0')) // lookup failure, valid id's will be != '0'
	{
		// If the lookup failed, we must assume it was because there was no matching entry
		// - in which case none of the collaborating clients has seen this entry yet, 
		// and so the simplest thing is just not to bother creating it here and then 
		// immediately pseudo-deleting it.
		;
	}
	else
	{
		// No error from the lookup, so get the entry ID, and the value of the deleted flag
		
		// If the remote entry has '0' for the deleted flag's value (which is what we expect),
		// then go ahead and pseudo-delete it; but if it has 1 already, there is nothing to 
		// do with this src-oldTgt pair, the pseudo-deletion is already in the remote DB
		wxChar chUndeleted = _T('0');
		if (m_entryStruct.deleted == chUndeleted)
		{
			// do a pseudo-delete here, the oldText is visible and not pseudo-deleted
			// rv = pKbSvr->PseudoDeleteOrUndeleteEntry(entryID, doDelete); legacy call
			rv = PseudoDelete(pKbSvr, src, oldText);
			wxASSERT(rv == 0);
		}
	}
	//                          ***** part 2 *****
	// That removes the oldText that was to be updated; now deal with the 
	// scr/newText pair -- another lookup is needed...
	rv = pKbSvr->LookupEntryFields(src, newText);
	if (rv != 0) // this lookup failed - meaning the entry does not yet exist in the table	
	{
		// If the lookup failed, we must assume it was because there was no matching entry
		// - in which case none of the collaborating clients has seen this entry yet, and 
		// so we can now create it in the remote DB
		rv = pKbSvr->CreateEntry(pKbSvr, src, newText);  // *** I can later remove the 2-field override for this function
		wxASSERT(rv == 0);
		// Ignore errors, if it didn't succeed, no big deal - someone else will sooner or
		// later succeed in adding this one to the remote DB. Internally it checks that there
		// is no duplication happening, before doing the creation and adding to entry table
	}
	else
	{
		// No error from the lookup, so get the value of the deleted flag, etc
		wxChar deletedFlag = m_entryStruct.deleted;
		int nDeleted = wxAtoi(deletedFlag);
		int theID = wxAtoi(m_entryStruct.id); 
#if defined(SYNC_LOGS)
		wxLogDebug(_T("2nd LookupEntryFields in KbEditorUpdateButton: id = %d , source = %s , translation = %s , deleted = %d , username = %s"),
			theID, (m_entryStruct.source).c_str(), (m_entryStruct.nonSource).c_str(), 
			nDeleted, (m_entryStruct.username).c_str());
#endif
		// If the remote entry has '0' for the deleted flag's value - then the remote DB
		// already has this as a normal entry, so we've nothing to do here. On the other
		// hand, if the flag's value is '1' (it's currently pseudo-deleted in the remote DB)
		// then go ahead and undo the pseudo-deletion, making a normal entry
		if (deletedFlag == _T('1'))
		{
			// It exists in the entry table, but as a pseudo-deleted entry, so
			// do an undelete of it, so it becomes visible in the GUI
			//rv = pKbSvr->PseudoDeleteOrUndeleteEntry(entryID, doUndelete); legacy call
			rv = PseudoUndelete(pKbSvr, src, newText);
			wxASSERT(rv == 0);
		}
		// Ignore errors, for the same reason as above (i.e. it's no big deal)
	}
	s_BulkDeleteMutex.Unlock();
	return rv;
}
/* BEW 2Nov20 deprecated, this is JM's old one/by/one crawling solution
int KbServer::DoEntireKbDeletion(KbServer* pKbSvr_Persistent, long kbIDinKBtable)
{
	CURLcode rv = (CURLcode)0;
	KbServer* pKbSvr = pKbSvr_Persistent;
	size_t m_TotalEntriesToDelete = 0; // initialize
	long m_idForKbDefinitionToDelete = kbIDinKBtable;

	// Do the work in a loop. Note: the last step, deleting the kb definition, should
	// be done from this function too; but the status bar should track N of M deletions, 
	// so that if the Manager is open, the administrator will know there's no point in 
	// trying a new deletion attempt until all M have been deleted - and that might not happen
	// in the one session. Also pStatelessKbServer passed in was created on the heap, and so
	// if the kb definition gets deleted successfully in this function, we can also do the
	// deletion of the stateless KbServer instance after that, also in this function. Its
	// pointer should be set NULL as well, as that pointer is permanent in the CAdapt_ItApp
	// instantiation, and testing it for NULL is important for its management.

	// Our curl call uses a dedicated standard string for the data callback,
	// str_CURLbuffer_for_deletekb, which is used by no other functions, so
	// it will never have data in it from any user adaptation work done while
	// KB deletion is taking place. No mutex is needed.

	// Note: if this function is still running when the user shuts the machine down,
	// the app will terminate without having removed all the KB's row entries.
	// The integrity of the MySQL database is not compromised. When the user next
	// runs Adapt It, the KB will still be in the kb table, but the entry table
	// will have fewer records in it that belong to this KB. It is then possible
	// to use the KB Sharing Manager a second time, to try delete the KB. If it runs to
	// completion before the machine is again closed down, the KB will have been removed.
	// If not, repeat at a later time(s), until the KB is fully emptied and then its
	// definition is deleted from the kb table.

	// We'll do a for loop, since we know how many we need to delete
	DownloadsQueue* pQueue = pKbSvr->GetDownloadsQueue(); // Note: this m_queue instance
		// is embedded in the stateless KbServer instance pointed at by pKbSvr, which is
		// the same one as CAdapt_ItApp::m_pKbServer_Persistent points at while this
		// deletion is taking place. No other code will access this particular queue, and
		// so it needs no mutex protection should the user be doing adapting work while the
		// KB is removed
	m_TotalEntriesToDelete = pQueue->GetCount();
	wxASSERT((pQueue != NULL) && (!pQueue->IsEmpty()) && (m_TotalEntriesToDelete > 0));
	DownloadsQueue::iterator iter;
	KbServerEntry* pKbSvrEntry = NULL;
	int nonsuccessCount = 0;
	size_t successCount = 0;
	size_t counter = 0;
	// Iterate over all entry structs, and for each, do a https DELETE request to have it
	// deleted from the database. Each DeleteSingleKbEntry() call is run synchronously, 
	// and the next iteration doesn't begin until the present call has returned its error 
	// code. How quickly the job gets done depends primarily on two things: 
	// a) how many entries need to be deleted, and 
	// b) the network latency currently being experienced
	for (iter = pQueue->begin(); iter != pQueue->end(); iter++)
	{
		pKbSvrEntry = *iter;
		int id = (int)wxAtoi(pKbSvrEntry->id);
		counter++;
		rv = (CURLcode)pKbSvr->DeleteSingleKbEntry(id);
#if defined (_DEBUG) && defined(_WANT_DEBUGLOG)
//		wxLogDebug(_T("DoEntireKbDeletion: Deleting entry with ID = %d  of total = %d"),
//						id, m_TotalEntriesToDelete);
#endif

		// We don't expect any failures, but just in case there are some, count how many;
		// also, ignore any failures and keep iterating to get as many done in the one
		// session as possible. If the loop completes in the one session with a zero
		// failure count, then continue on to try removing the kb definition itself,
		// because it owns no entries and therefore no constraints would prevent it being
		// removed. But if there were failures we can't attempt the kb definition deletion
		// because the presence of owned entries would result in a foreign key constraint
		// error, which we don't want the user to see. Instead, in that case he can just
		// try the removal again at a later time - we need to put up a message to that
		// effect if so.
		if (rv != CURLE_OK)
		{
			nonsuccessCount++;
		}
		else
		{
			successCount++;
#if defined(SYNC_LOGS)
			// track what we delete and it's ID
//			wxLogDebug(_T("TDoEntireKbDeletion(): id = %d, src = %s , non-src = %s"),
//				pKbSvrEntry->id, pKbSvrEntry->source.c_str(), pKbSvrEntry->translation.c_str());
#endif
			// Copy it to the app member ready for display in main window at bottom
			m_pApp->m_nIterationCounter = successCount;
			if ((successCount / 50) * 50 == successCount)
			{
				// Update the value every 50th iteration, otherwise more frequently may bog
				// the process down if latency is very low
				wxCommandEvent eventCustom(wxEVT_KbDelete_Update_Progress);
				wxPostEvent(m_pApp->GetMainFrame(), eventCustom); // custom event handlers are in CMainFrame
			}
		}

		CStatusBar* pStatusBar = NULL;
		pStatusBar = (CStatusBar*)m_pApp->GetMainFrame()->m_pStatusBar;
		pStatusBar->UpdateProgress(_("Delete KB"), counter, _("Deleting a whole remote KB: entries deleted so far..."));
	}

	// Remove the KbServerEntry structs stored in the queue (otherwise we would
	// leak memory)
	pKbSvr->DeleteDownloadsQueueEntries();
	(pKbSvr->GetDownloadsQueue())->Clear(); 

	// If control gets to here, we've either deleted all entries of the selected database,
	// or most of them with some errors resulting in the leftover entries remaining owned by
	// the selected kb definition. Only if all entries were deleted can we now attempt the
	// removal of the KB definition itself. Otherwise, tell the user some were not
	// deleted, and he'll have to retry later - and leave the definition in the Mgr list.
	if (nonsuccessCount > 0)
	{
		// There were some failures... (and the Manager GUI may not be open -- see comment
		// in the else block for details)
#if defined(SYNC_LOGS)
		wxLogDebug(_T("DoEntireKbDeletion(): deletion errors (number of entries failing) = %d"),
			nonsuccessCount);
#endif
		// We want to make the wxStatusBar (actually, our subclass CStatusBar), go back to 
		// one unlimited field, and reset it and update the bar
		m_pApp->StatusBar_EndProgressOfKbDeletion();
	}
	else
	{
		// Yay!! No failures. So the kb pair comprising the kb database definition can now
		// be removed in the current Adapt It session. However, we can't be sure the user
		// has the Manager GUI still open, it may be many hours since he set the removal
		// running -- how do we handle this communication problem? If the GUI is still
		// open, and the kbs page is still the active page, then we want the list updated
		// to reflect the kb definition has gone. If the GUI isn't open, we just remove the
		// definition from the kb table of the mysql database, and no more needs to be done
		// other than housekeeping cleanup (eg. clearing of the queue of KbServerEntry
		// structs, and deletion of the stateless KbServer instance used for doing this
		// work. How do we proceed...? I think a boolean on the app to say that the
		// Manager GUI is open for business, and another that the kbs page is active, and
		// we also need to check that the radiobutton setting hasn't changed (to switch
		// between a adapting kbs versus glossing kbs) on that page.

		// Delete the definition
		CURLcode result = CURLE_OK;
		result = (CURLcode)pKbSvr->RemoveKb((int)m_idForKbDefinitionToDelete); // synchronous
		if (result != CURLE_OK)
		{
			// The definition should have been deleted, but wasn't. Tell the user to try
			// again later. (This error is quite unexpected, an English error message will
			// do).
			wxString msg = _("Unexpected failure to remove a shared KB definition after completing the removal of all the entries it owned.\n Try again later. The next attempt may succeed.");
			wxString title = _("Error: could not remove knowledge base definition");
			m_pApp->LogUserAction(msg);
			wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);

			// Housekeeping cleanup -- this stuff is needed whether the Manager
			// GUI is open or not
			pQueue->clear();
			delete m_pApp->m_pKbServer_Persistent; // the KbServer instance supplying services 
												   // for our deletion attempt
			m_pApp->m_pKbServer_Persistent = NULL;

			m_pApp->m_bKbSvrMgr_DeleteAllIsInProgress = FALSE;

			// No update of the KB Sharing Manager GUI is needed, whether running or not,
			// because the KB definition was not successfully removed from the kb table,
			// it will still appear in the Manager if the latter is running, and if not
			// and someone runs it, it will be shown listed in the kb page - and the user
			// could then try again to remove it.

			// We want to make the wxStatusBar (actually, our subclass CStatusBar), go back to 
			// one unlimited field, and reset it and update the bar
			m_pApp->StatusBar_EndProgressOfKbDeletion();
			m_pApp->RefreshStatusBarInfo();

			return 1; // 1 means "some kind of error"
		} // end of TRUE block for test: if (result != CURLE_OK)

		  // If control gets to here, then the kb definition was removed successfully from
		  // the kb table. It remains to get the KB Sharing Manager gui to update to show
		  // the correct result, if it is still running. If not running we have nothing more
		  // to do and the thread can die after a little housekeeping
		KBSharingMgrTabbedDlg* pGUI = m_pApp->GetKBSharingMgrTabbedDlg();
		if (pGUI != NULL)
		{
			// The KB Sharing Manager gui is running, so we've some work to do here... We
			// want the Mgr gui to update the kb page of the manager

			// Some explanation is warranted here. The GUI might have the user page active
			// currently, or the kbs page, (or, if we implement it) the languages page. Any
			// time someone, in that Manager GUI, changes to a different page, a
			// LoadDataForPage() call is made, taking as parameter the 0-based index for
			// the page which is to be loaded. That function makes https calls to the
			// remote database to get the data which is to be displayed on the page being
			// selected for viewing - in particular, the list of users, kb definitions, or
			// language codes, as the case may be. Because of that, if either the users
			// page is currently active, or the language codes page is active, we do not
			// here need to have the list in the kbs page forced to be updated - because
			// the user would have to choose that page in the GUI in order to see it, and
			// the LoadDataForPage() call that results will get the list update done. (If a
			// radio button, for kb type, is changed, that too forces a http data get from
			// the remote server to update the list box, etc.) So, we only need to update
			// the listbox in the kbs page provided that:
			// a) the kbs page is currently active, AND
			// b) the kb type of the listed kb definition entries being shown matches the
			// kb type that has just been deleted from the kb table.
			// When both a) and b) are true, the user won't see the list updated unless we
			// force the update from here. (He'd otherwise have to click on a different
			// page's tab, and then click back on the kbs page tab, and perhaps also click
			// the required radio button on that page if what was deleted was a glossing kb
			// database.) So these comments explain why we do what we do immediately
			// below...
			if (m_pApp->m_bKbPageIsCurrent)
			{
				// We may have to force the page update - it depends on whether the kb types
				// match, so check for that
				int guiCurrentlyShowsThisKbType = m_pApp->m_bAdaptingKbIsCurrent ? 1 : 2;
				if (pKbSvr->GetKBServerType() == guiCurrentlyShowsThisKbType)
				{
					// We need to force the list to be updated. The radio button setting is
					// already correct, and the page has index equal to 1, so the following
					// call should do it
					pGUI->LoadDataForPage(1);
				}
			}
		} // end of TRUE block for test:  if (pGUI != NULL)
	}
	// Housekeeping cleanup -- this stuff is needed whether the Manager GUI is open or not
	if (!m_pApp->m_pKbServer_Persistent->IsQueueEmpty())  // or, if (!m_pKbSvr->IsQueueEmpty())
	{
		// Queue is not empty, so delete the KbServerEntry structs that are 
		// on the heap still
		m_pApp->m_pKbServer_Persistent->DeleteDownloadsQueueEntries();
	}

	m_pApp->m_bKbSvrMgr_DeleteAllIsInProgress = FALSE;

	// We want to make the wxStatusBar (actually, our subclass CStatusBar), go back to 
	// one unlimited field, and reset it and update the bar
	m_pApp->StatusBar_EndProgressOfKbDeletion();
	m_pApp->RefreshStatusBarInfo();

	return rv;
}
*/

// BEW remove this, no longer needed
/*
// Return 0 (CURLE_OK) if no error, a CURLcode error code if there was an error
int KbServer::PseudoDeleteOrUndeleteEntry(int entryID, enum DeleteOrUndeleteEnum op)
{
	wxString entryIDStr;
	wxItoa(entryID, entryIDStr);
	CURL *curl;
	CURLcode result; // result code
	struct curl_slist* headers = NULL;
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString kbType;
	int type = GetKBServerType();
	wxItoa(type,kbType);
	wxJSONValue jsonval; // construct JSON object
	CBString strVal; // to store wxString form of the jsonval object, for curl
	wxString container = _T("entry");
	wxString aUrl, aPwd;

	str_CURLbuffer.clear(); // use for headers return when there's no json to be returned

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	// populate the JSON object
	switch ( op )
	{
	case doDelete:
	default:
		jsonval[_T("deleted")] = (long)1;
		break;
	case doUndelete:
		jsonval[_T("deleted")] = (long)0;
		break;
	}

	// convert it to string form
	wxJSONWriter writer; wxString str;
	writer.Write(jsonval, str);
	// convert it to utf-8 stored in CBString
	strVal = ToUtf8(str);

	aUrl = GetKBServerIpAddr() + slash + container + slash + entryIDStr;
	charUrl = ToUtf8(aUrl);

	// prepare curl
	curl = curl_easy_init();

	if (curl)
	{
		// add headers
		headers = curl_slist_append(headers, "Accept: application/json");
		headers = curl_slist_append(headers, "Content-Type: application/json");
		// set data
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT"); // this way avoids turning on file processing
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)strVal);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
		//curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		// get the headers stuff this way when no json is expected back...
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback);

		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) // && defined (__WXGTK__)
        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("PseudoDeleteOrUndeleteEntry() Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif
		// The kind of error we are looking for isn't a CURLcode one, but a HTTP one
		// (400 or higher)
		ExtractHttpStatusEtc(str_CURLbuffer, m_httpStatusCode, m_httpStatusText);

		curl_slist_free_all(headers);
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("PseudoDeleteOrUndelete() result code: %d Error: %s"),
				result, error.c_str());
			if (op == doDelete)
			{
				wxMessageBox(msg, _T("Error when trying to delete an entry"), wxICON_EXCLAMATION | wxOK);
				m_pApp->LogUserAction(_T("Deleting...") + msg);
			}
			else
			{
				wxMessageBox(msg, _T("Error when trying to undelete an entry"), wxICON_EXCLAMATION | wxOK);
				m_pApp->LogUserAction(_T("Undeleting...") + msg);
			}
			curl_easy_cleanup(curl);
			str_CURLbuffer.clear();
			return result;
		}
	}
	curl_easy_cleanup(curl);
	str_CURLbuffer.clear();

	// handle any HTTP error code, if one was returned
	if (m_httpStatusCode >= 400)
	{
		// We may get 400 "Bad Request" or 404 Not Found (both should be unlikely)
		// Rather than use CURLOPT_FAILONERROR in the curl request, I'll use the HTTP
		// status codes which are returned, to determine what to do, and then manually
		// return 22 i.e. CURLE_HTTP_RETURNED_ERROR, to pass back to the caller
		return CURLE_HTTP_RETURNED_ERROR;
	}

	return 0;
}
*/

/* BEW deprecated 2Nov20 - part of JM's solution, no longer needed - remove later
void KbServer::DoChangedSince()
{
	int rv = 0; // rv is "return value", initialize it
	wxString timestamp;
	// get the last sync timestamp value
	timestamp = GetKBServerLastSync();
#if defined(SYNC_LOGS)
	wxLogDebug(_T("DoChangedSince() with lastsync timestamp value = %s"), timestamp.c_str());
#endif
	rv = ChangedSince_Timed(timestamp);
	// if there was no error, update the m_kbServerLastSync value, and export it to
	// the persistent file in the project folder
	if (rv == 0)
	{
		UpdateLastSyncTimestamp();
	}
	else
	{
		// there was a cURL error, display it
		wxString msg;
		msg = msg.Format(_("Downloading to the knowledge base: an error code ( %d ) was returned.  Nothing was downloaded, application continues."), rv);
		wxMessageBox(msg, _("Downloading to the knowledge base failed"), wxICON_ERROR | wxOK);
		m_pApp->LogUserAction(msg);
		return;
	}
	// If control gets to here, we are ready to merge what was returning into the local KB
	// or GlossingKB, as the case may be
	m_pKB->StoreEntriesFromKbServer(this);
}
*/
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
		// This function is also used within the UploadToKbServer() function, to get a
		// temporary download of the whole remote DB (for the given language pair) to be
		// used for determining which of the local KB entries is already in the remote DB
		// and therefore should not be uploaded (if one such were uploaded, a 401 BAD
		// REQUEST http error would result, and any other entries in that thread would not
		// be attempted - the upload of that part of the data would fail). So for this
		// temporary use of DoGetAll() we want to suppress the UpadateLastSyncTimestamp()
		// call. The flag is default TRUE, to suppress the timestamp update pass in FALSE.
		if (bUpdateTimestampOnSuccess)
		{
			UpdateLastSyncTimestamp();
		}
	}
	else
	{
		if (bUpdateTimestampOnSuccess)
		{
			// There was a cURL error, display it - but only if we were trying to do a
			// DoGetAll() to update the local KB. It we were calling DoGetAll() as the
			// first step in the process required for the first upload of the local
			// KB to the remote DB on KBserver, then the DoGetAll() returns a
			// http error 404 NOT FOUND - and we don't want an error message to show in
			// that circumstance, because the upload will deduce from the 7 parallel
			// arrays being empty that the remote DB has nothing in it yet, and use that
			// fact to upload the whole contents of the local KB rather than checking for
			// only those entries in the local KB which are not already in the remote DB.
			wxString msg;
			msg = msg.Format(_("Downloading to the knowledge base: an error code ( %d ) was returned.  Nothing was downloaded, application continues."), rv);
			wxMessageBox(msg, _("Downloading to the knowledge base failed"), wxICON_ERROR | wxOK);
			m_pApp->LogUserAction(msg);
		}
		return;
	}
	if (bUpdateTimestampOnSuccess)
	{
        // If control gets to here, we are ready to merge what was returning into the local
        // KB or GlossingKB, as the case may be. We don't do this if the DoGetAll() call
        // was just for getting temporary copies of the remoteDB contents in order to help
        // the UploadToKbServer() call filter out entries which should not be sent
		m_pKB->StoreEntriesFromKbServer(this);
	}
}

// clears user data (wxArrayString instances on the heap, and their contents)
// from m_uploadsMap
void KbServer::ClearUploadsMap()
{
	if(m_uploadsMap.empty())
	{
		return; // it's already empty
	}
	UploadsMap::iterator iter;
	wxArrayString* pArrayStr = NULL;
	for (iter = m_uploadsMap.begin(); iter != m_uploadsMap.end(); ++iter)
	{
		pArrayStr = iter->second;
		pArrayStr->clear();
		delete pArrayStr;
	}
	// data has been cleared, now clear the hanging ptrs
	m_uploadsMap.clear();
}

// Populate the m_uploadsList - either with the help of the remote DB's data in the
// hashmap, or without (the latter when the remote DB has no content yet for this
// particular language pair) - pass in a flag to handle these two options
void KbServer::PopulateUploadList(KbServer* pKbSvr, bool bRemoteDBContentDownloaded)
{
	wxASSERT(m_uploadsList.IsEmpty()); // must be empty before we call this

	// scan the KB and store src-tgt pairs for normal entries only in KbServerEntry
	// structs (created on the heap), in the m_uploadsList
	wxString srcPhrase;
	wxString tgtPhrase;
	CTargetUnit* pTU;

	// We must populate the m_uploadsMap if the passed in flag is TRUE, for the loop below
	// will use it in that case; we don't bother when the flag was passed in as FALSE
	if( bRemoteDBContentDownloaded)
	{
		// The populating is done by iterating through the 7 parallel lists of strings
		// downloaded from the ChangedSince() call done in the caller, and making
		// fast access map entries so they can be looked up quickly
		PopulateUploadsMap(this);
		// Clearing the m_uploadsMap is done in the caller on return
	}

	UploadsMap::iterator upIter;
	wxString empty = _T("<empty>");
	KbServerEntry* reference;
	CKB* currKB = pKbSvr->GetKB( GetKBServerType() ); //Glossing = KB Type 2

	//Need to get each map of the local KB, & iterate through each
	for (int i = 0; i< MAX_WORDS; i++)
	{
		//for each map
		for (MapKeyStringToTgtUnit::iterator iter = currKB->m_pMap[i]->begin();
				iter != currKB->m_pMap[i]->end(); ++iter)
		{
			wxASSERT(currKB->m_pMap[i] != NULL);

			if (!currKB->m_pMap[i]->empty())
			{
				srcPhrase = iter->first;
				/*
#if defined(SYNC_LOGS)
				// the three which are repeated in the 9, have srcPhrase "bikpela", or "i
				// laikim", or "long ai bilong mipela", so break at these & step it
				if (srcPhrase == _T("bikpela") ||
					srcPhrase == _T("i laikim") ||
					srcPhrase == _T("long ai bilong mipela"))
				{
					int break_here = 1;
				}
#endif
				*/
				pTU = iter->second;
				CRefString* pRefString = NULL;
				TranslationsList::Node* pos = pTU->m_pTranslations->GetFirst();
				wxASSERT(pos != NULL);

				while (pos != NULL)
				{
					pRefString = (CRefString*)pos->GetData();
					wxASSERT(pRefString != NULL);
					pos = pos->GetNext();

					if (pRefString->GetDeletedFlag() == FALSE)
					{
						// upload only "normal" entries, ignore pseudo-deleted ones;
						// if the passed in flag is TRUE then we accept only those
						// which are not in the hashmap; if FALSE passed in, we
						// accept each one because no conflict is going to happen
						if (bRemoteDBContentDownloaded)
						{
							// accept only the ones which aren't already in the
							// remote DB
							tgtPhrase = pRefString->m_translation; // might be empty

							if (tgtPhrase.IsEmpty())
							{
								// use this instead for the search
								tgtPhrase = empty;
							}

							// do a find() in the pUploadsMap
							upIter = m_uploadsMap.find(srcPhrase);
							if (upIter == m_uploadsMap.end())
							{
								// the key is not present in the map, therefore
								// this pair should be uploaded to the remote DB
								reference = new KbServerEntry;
								reference->source = srcPhrase;
								reference->nonSource = tgtPhrase;
								// store the new struct
								m_uploadsList.Append(reference);
							}
							else
							{
                                // This key has a presence in the map - but a source key
                                // may have more than one translation, so get the array of
                                // translations and test if any match tgtPhrase - for any
                                // that are matches with src/translation pairs in
                                // m_uploadsMap, DON'T upload such pair to the remote DB.
                                // Note, I've stored empty translations as _T("<empty>"),
                                // so we are checking for that too (see above). But all the
                                // non-matches in the loop below mean that the src/tgt pair
                                // being checked isn't yet a pair in the remote DB, and so
                                // we must add an entry to uploadsList so as to later
                                // upload these to the remote DB
								wxArrayString* pArray = upIter->second;
								// Check if this adaptation (or gloss) from pRefString of
								// the local KB is the same as the one or several strings
								// in the wxArrayString formed from the downloaded remote
								// DB data
								int nIndex = pArray->Index(tgtPhrase);
								if (nIndex == wxNOT_FOUND)
								{
									// we have a src/tgt pair which has no presence in
									// the remote DB as yet, so upload (beware, check
									// for the empty string designator and restore the
									// empty string if found)
#if defined(SYNC_LOGS)
									// currently, "mi" <-> "I" is treated as an error as the
									// php does a caseless compare, and it conflicts with
									// existing "mi" <-> "i", so exclude it)
									if (srcPhrase == _T("mi") && tgtPhrase == _T("I"))
										continue;
#endif
									// generalize the above conditional compile to handle non Tok Pisin
									// languages. Any time "I" is the target entry, just refrain from
									// sending it, ever.
									if (tgtPhrase == _T("I"))
										continue; // skip this iteration

									if (tgtPhrase == empty) // empty is _T("<empty>")
									{
										tgtPhrase.Empty();
									}
									reference = new KbServerEntry;
									reference->source = srcPhrase;
									reference->nonSource = tgtPhrase;
									// store the new struct
									m_uploadsList.Append(reference);
								}
                                // if control didn't enter the above block, then we
                                // continue by looking at the next CRefString stored on the
                                // currently accessed CTargetUnit instance; because the
                                // srcPhrase/tgtPhrase pair are in the remote remote DB
							}
						}
						else
						{
							// none downloaded, so accept everything
							reference = new KbServerEntry;
							reference->source = srcPhrase;
							// Don't send upper case "I" translation, php does a caseless compare
							// at the server end, and we don't want to generate a conflict with
							// "i" if the latter gets put in the remote db
							if (pRefString->m_translation == _T("I"))
							{
								srcPhrase.Clear();
								delete reference; // don't leak it
								continue; // skip this iteration
							}
							reference->nonSource = pRefString->m_translation;
							// store the new struct
							m_uploadsList.Append(reference);
						}
					} // end of TRUE block for test: if (pRefString->GetDeletedFlag() == FALSE)
				} // end of while loop: while (pos != NULL)  (iterating over CRefString ptrs)
			} // end of TRUE block for test: if (!currKB->m_pMap[i]->empty())
		} // end of for loop:
		// 	for (MapKeyStringToTgtUnit::iterator iter = currKB->m_pMap[i]->begin();
		//	iter != currKB->m_pMap[i]->end(); ++iter)
		//	which iterates over all the CTargetUnit pointers in the ith map of the KB
	} // end of for loop: for (int i = 0; i< MAX_WORDS; i++)
	  // which iterates over all (10) of the maps which make up the CKB
}

// Extract the source and translation strings, and use the source string as key, and value
// is a wxArrayString, so add the translation string as an item to the array (because for a
// give source text key, there can be more than one translation associated with it), to
// populate the m_uploadsMap from the downloaded remote DB data (stored in the 7 parallel
// arrays). This is mutex protected by the s_DoGetAllMutex)
void KbServer::PopulateUploadsMap(KbServer* pKbSvr)
{
	wxString src;
	wxString tgt;
	wxString empty = _T("<empty>");
	size_t count = pKbSvr->m_arrSource.GetCount(); // the source array of the 7 accepting downloaded fields
#if defined(SYNC_LOGS)
	wxLogDebug(_T("\nPopulateUploadsMap() commences ..... m_arrSource.GetCount() value = %d  (all 7 arrays have this count value)"),
		count);
	int counter = 0;
#endif
	size_t i;
	wxArrayString* pMyTranslations = NULL;
	UploadsMap::iterator iter;
	for (i = 0; i < count; ++i)
	{
		// Some entries may be pseudo-deleted ones in the remote DB, we must accept these
		// along with normal entries, because CreateEntry() would fail if a pseudo-deleted
		// entry was in the DB and creation of the equivalent normal one was attempted. So
		// we don't sent a normal entry from the local KB even if there is only a
		// pseudo-deleted one in the remote DB
		src = pKbSvr->m_arrSource.Item(i);
		tgt = pKbSvr->m_arrTarget.Item(i); // could be an empty string, that's allowed
		// If we don't change empty translation strings to something non-empty, we may
		// end up excluding those unwittingly, so use _T("<empty>") for them and change
		// the latter back to an empty string when necessary
#if defined(SYNC_LOGS)
		counter++;
		wxLogDebug(_T("\nPopulateUploadsMap() loop: counter = %d  src: %s  tgt: %s       <<-- key & value for map"),
			counter, src.c_str(), tgt.c_str());
#endif
		if (tgt.IsEmpty())
		{
			tgt = empty;
		}
		// Find this key in the map; if not there, create on the heap another
		// wxArrayString, and store tgt as it's first item; if there, we can be certain no
		// other value has the same string (because the MySql database does not store
		// entry duplicates), so just add it to the array
		iter = m_uploadsMap.find(src);
		if (iter == m_uploadsMap.end())
		{
			// this key is not yet in the map, add a new wxArrayString item using it and
			// store tgt within it
			pMyTranslations = new wxArrayString;
			m_uploadsMap[src] = pMyTranslations;
			pMyTranslations->Add(tgt);
#if defined(_DEBUG) && defined(SYNC_LOGS)
			int numInArray = pMyTranslations->GetCount();
			wxLogDebug(_T("\nPopulateUploadsMap() loop: counter = %d  key is NOT IN MAP, so adding tgt: %s  entry #: %d Values: %s"),
				counter, tgt.c_str(), numInArray, (ReturnStrings(pMyTranslations)).c_str());
#endif
		}
		else
		{
			// this key is in the map, so we've got another translation string to
			// associate with it - add it to the array
			pMyTranslations = iter->second;
			pMyTranslations->Add(tgt);
#if defined(_DEBUG) && defined(SYNC_LOGS)
			int numInArray = pMyTranslations->GetCount();
			wxLogDebug(_T("\nPopulateUploadsMap() loop: counter = %d  key IS IN MAP, so adding tgt: %s  entry #: %d  Values: %s"),
				counter, tgt.c_str(), numInArray, (ReturnStrings(pMyTranslations)).c_str());
#endif
		}
	}
	// The only reason we populate this map is because once the local KB gets large, say
	// over a thousand entries, it will be much quicker to determine a give src-tgt pair
	// from the local KB is not in the map, than to do the same check with an array or list
#if defined(SYNC_LOGS)
	wxLogDebug(_T("\nPopulateUploadsMap() loop has ended\n"));
#endif
}

#if defined(_DEBUG) && defined(SYNC_LOGS)
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

// sets all 50 of the array of int to CURLE_OK
void KbServer::ClearReturnedCurlCodes()
{
	int i;
	for (i = 0; i < 50; i++)
	{
		m_returnedCurlCodes[i] = CURLE_OK; // zero
	}
}

#if defined(_DEBUG) && defined(SYNC_LOGS)
wxString KbServer::ShowReturnedCurlCodes()
{
	wxString str; str.Empty();
	int i;
	for (i = 0; i < 50; i++)
	{
		if (m_returnedCurlCodes[i] == 0)
		{
			str += _T("0");
		}
		else
		{
			str += _T("1");
		}
	}
	return str;
}
#endif

// returns TRUE if all entries in m_returnedCurlCodes array are CURLE_OK;
// FALSE if at least one is some other value
// (the current implementation permits only CURLE_HTTP_RETURNED_ERROR
// to be the 'other value', if a BulkUpload() call failed - we assume
// it was because it tried to upload an already entered db entry)
bool KbServer::AllEntriesGotEnteredInDB()
{
	int i;
	for (i = 0; i < 50; i++)
	{
		if (m_returnedCurlCodes[i] != CURLE_OK)
		{
			return FALSE;
		}
	}
	return TRUE;
}

// The upload function - we do it by multiple threads (50 of them), each thread having
// approx 1/50th of the inventory of normal entries which qualify for uploading, as payload
// (we don't upload any pseudo-deleted ones).
void KbServer::UploadToKbServer()
{
	// BEW 9Nov20 Don't allow any kbserver stuff to happen, if user is not
	// authenticated, or the relevant app member ptr, m_pKbServer[0] or [1] is
	// NULL, or if gbIsGlossing does not match with the latter ptr as set non-NULL
	if (!m_pApp->AllowSvrAccess(gbIsGlossing))
	{
		return; // suppress any mysql access, etc
	}

	bool bConfiguredOK = m_pApp->ConfigureDATfile(upload_local_kb);
	if (bConfiguredOK)
	{
		wxString execPath = m_pApp->execPath; // has PathSeparator at string end
		wxString execFileName = _T("do_upload_local_kb.exe");
		wxString resultFile = _T("upload_local_kb.exe");
		m_pApp->CallExecute(upload_local_kb, execFileName, execPath, resultFile, 99, 99);
	}
}

void KbServer::DeleteUploadEntries()
{
	UploadsList::iterator iter;
	KbServerEntry* pStruct = NULL;
	for (iter = m_uploadsList.begin(); iter != m_uploadsList.end(); ++iter)
	{
		pStruct = *iter;
		// delete it
		delete pStruct;
	}
	// now clear the list
	m_uploadsList.clear();
}

void KbServer::PushToQueueEnd(KbServerEntry* pEntryStruct) // protect with a mutex
{
	s_QueueMutex.Lock();

	m_queue.push_back(pEntryStruct);

	s_QueueMutex.Unlock();
}

KbServerEntry* KbServer::PopFromQueueFront() // protect with a mutex
{
	s_QueueMutex.Lock();

	KbServerEntry* pPopped = m_queue.front(); // gets but doesn't pop
	m_queue.pop_front(); // this pops it to limbo

	s_QueueMutex.Unlock();
	return pPopped; // the returned struct's data will be used in
					// an OnIdle() call in main thread, then deleted
}

bool KbServer::IsQueueEmpty()
{
	return m_queue.empty(); // return TRUE if m_queue is empty
}

DownloadsQueue* KbServer::GetDownloadsQueue()
{
	return &m_queue;
}

// BEW 2Nov20 remove at cleanup, it implemented JM solution with threads, 50 upload files, etc - no longer relevant
/*
int KbServer::BulkUpload(int chunkIndex, // use for choosing which buffer to return results in
						 wxString url, wxString username, wxString password,
						 CBString jsonUtf8Str)
{
	CURL *curl;
	CURLcode result = CURLE_OK; // result code, initialize to "no error" (0)
	struct curl_slist* headers = NULL;
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString container = _T("entry");
	wxString aUrl, aPwd;
	str_CURLbuff[chunkIndex].clear(); // always make sure it is cleared for accepting new data

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = username + colon + password;
	charUserpwd = ToUtf8(aPwd);

#if defined(SYNC_LOGS)
	// try do this without a mutex - it's temporary and I may get away with it, if not
	// delete it; same for the one at the end below
	wxDateTime now = wxDateTime::Now();
	wxLogDebug(_T("BulkUpload() 1-based chunk number %d , start time: %s\n"),
		chunkIndex + 1, now.Format(_T("%c"), wxDateTime::WET).c_str());
#endif
	aUrl = url + slash + container + slash;
	charUrl = ToUtf8(aUrl);

	// prepare curl
	curl = curl_easy_init();
	if (curl)
	{
		// add headers
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "Accept: application/json");
		// set data
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)jsonUtf8Str);
		// ask for the headers to be prepended to the body - this is a good choice here
		// because no json data is to be returned
		curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
		// get the headers stuff this way...
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_read_data_callback2);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunkIndex);
		// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		result = curl_easy_perform(curl);

#if defined (SYNC_LOGS) // && defined (__WXGTK__)
        CBString s(str_CURLbuff[chunkIndex].c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("BulkUpload() chunk %d , Returned: %s    CURLcode %d"),
			chunkIndex, showit.c_str(), (unsigned int)result);
#endif
		// The kind of error we are looking for isn't a CURLcode one, but aHTTP one
		// (400 or higher)
		ExtractHttpStatusEtc(str_CURLbuff[chunkIndex], m_httpStatusCode, m_httpStatusText);

		curl_slist_free_all(headers);
		str_CURLbuff[chunkIndex].clear();

        // Typically, result will contain CURLE_OK if an error was a HTTP one and so the
        // next block won't then be entered; don't bother to localize this one, we don't
		// expect it will happen much if at all (but it would be if there was no
		// connection to the remote server)
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("UploadToKbServer() result code: %d Error: %s"),
				result, error.c_str());
			wxMessageBox(msg, _T("Error when bulk-uploading part of the entries"), wxICON_EXCLAMATION | wxOK);
			m_httpStatusText.Clear();
			curl_easy_cleanup(curl);
			return result;
		}
	}
	curl_easy_cleanup(curl);
	m_httpStatusText.Clear();

#if defined(SYNC_LOGS)
	now = wxDateTime::Now();
	wxLogDebug(_T("BulkUpload() 1-based chunk %d , finish time: %s\n"),
		chunkIndex + 1, now.Format(_T("%c"), wxDateTime::WET).c_str());
#endif

	// Work out what to return, depending on whether or not a HTTP error happened, and
	// which one it was; if there was no connectivity, failure would happen earlier and a
	// curl error of 6 would be returned (CURLE_COULDNT_RESOLVE_HOST). The error that we
	// don't want is the http 400 one, which means there's an entry already in the remote
	// DB - so the caller should check for CURLE_HTTP_RETURNED_ERROR and give the user
	// some direction about what to do (e.g. try again later when noone else is uploading
	// or adapting)
	if (m_httpStatusCode >= 400)
	{
		// Probably 400 "Bad Request" should be the only one we get.
		// Rather than use CURLOPT_FAILONERROR in the curl request, I'll use the HTTP
		// status codes which are returned, I'll return 22 i.e.
		// CURLE_HTTP_RETURNED_ERROR to the caller
		return CURLE_HTTP_RETURNED_ERROR;
	}
	return 0;
}
*/
// BEW 10Oct20, take a results file (multiline, or just 2 lines - "success" and one entry's row)
// and convert to string array.  I need this on AI.h too, for when I want to process the result
// file in CallExecute()'s post wxExecute() call's switch, so I'll make a public copy on app. Yuck, but saves time.
bool KbServer::DatFile2StringArray(wxString& execPath, wxString& resultFile, wxArrayString& arrLines)
{
	arrLines.Empty(); // clear contents
	wxString pathToResults = execPath + resultFile;
	bool bResultsFileExists = ::FileExists(pathToResults);
	if (bResultsFileExists)
	{
		wxTextFile f(pathToResults);
		bool bOpened = f.Open();
		int lineIndex = 0; // ignore this one, it has "success in it" etc
		int lineCount = f.GetLineCount();

		wxString strCurLine = wxEmptyString;
		if (bOpened)
		{
			for (lineIndex = 1; lineIndex < lineCount; lineIndex++)
			{
				strCurLine = f.GetLine(lineIndex);
				arrLines.Add(strCurLine);
			}
		}
	}
	else
	{
		// results file does not exist, log the error
		wxBell();
		wxString msg = _T("DatFile2StringArray() does not exist in the executable's folder: %s");
		msg = msg.Format(msg, execPath.c_str());
		m_pApp->LogUserAction(msg);
		return FALSE;
	}
	return TRUE;
}

// BEW 18Dec20 deprecate the following, I'm using arrays in AI.h for a refactored solution
// BEW 14Nov20 updated for Leon's solution (includes password now)
void KbServer::ConvertLinesToUserStructs(wxArrayString& arrLines, UsersListForeign* pUsersListForeign)
{
	// BEW 14Nov20 for arrLines, pass in app->m_arrLines, which has been populated
	// already in the CallExecute(list_users, ..... ) call, in the switch for case 3
	// which follows the ::wxExecute() call.
	pUsersListForeign->Clear(); // start with an empty (templated) list
	size_t linesArrayCount = arrLines.GetCount();
	size_t lineIndex = 0;
	for (lineIndex = 0; lineIndex < linesArrayCount; lineIndex++)
	{
		wxString str = arrLines.Item(lineIndex); // format: username,fullname,password,useradmin,
			// where useradmin is a wxChar with value _T('1') or _T('0') presented as TRUE or FALSE
		// turn the comma-separated fields into struct member strings
		KbServerUserForeign* pStruct = new KbServerUserForeign;


		int offset = wxNOT_FOUND;
		int fieldsCount = 4;
		int pos;
		wxString field = wxEmptyString;
		wxString comma = _T(',');
		for (pos = 0; pos <= fieldsCount; pos++)
		{
			switch (pos)
			{
			case 0:
				offset = str.Find(comma);
				wxASSERT(offset >= 0);
				field = wxString(str.Left(offset));
				pStruct->username = field;
				// Shorten
				str = str.Mid(offset + 1);
				field.Empty();
				break;
			case 1:
				offset = str.Find(comma);
				wxASSERT(offset >= 0);
				field = wxString(str.Left(offset));
				pStruct->fullname = field;
				// Shorten
				str = str.Mid(offset + 1);
				field.Empty();
				break;
			case 2:
				offset = str.Find(comma);
				wxASSERT(offset >= 0);
				field = wxString(str.Left(offset));
				pStruct->password = field;
				// Shorten
				str = str.Mid(offset + 1);
				field.Empty();
				wxLogDebug(_T("%s::%s(), line %d : lookup_user = %d"),
					__FILE__, __FUNCTION__, __LINE__, lookup_user);
				break;
			case 3:
				offset = str.Find(comma);
				wxASSERT(offset >= 0);
				field = wxString(str.Left(offset));
				if (field == _T("0"))
				{
					pStruct->useradmin = FALSE;
				}
				else
				{
					// The only other possibility is useradmin == '1'
					pStruct->useradmin = TRUE;
				}
				// We are done
				wxLogDebug(_T("%s::%s(), line %d : list_users = %d"),
							__FILE__, __FUNCTION__, __LINE__, list_users);
				break;
			};
		} // end of for loop: for (pos = 0; pos <= fieldsCount; pos++)
		wxLogDebug(_T("%s::%s(), line %d : for appending to pStruct: username = %s, fullname = %s, password = %s , useradmin = %d"),
			__FILE__, __FUNCTION__, __LINE__, pStruct->username.c_str(),
			pStruct->fullname.c_str(), pStruct->password.c_str(), pStruct->useradmin == TRUE?1:0);

		// Append each filled out KbServerUserForeign to the pUsersListForeign (for Leon's sol'n)
		pUsersListForeign->Append(pStruct);

		wxLogDebug(_T("%s::%s(), line %d : appended struct for username = %s"),
					__FILE__, __FUNCTION__, __LINE__, pStruct->username.c_str());

		// put a copy of the list in Mgr's m_pUsersListForeign
	}
}

// BEW 3Oct20, The following function decides, using funcNumber's switch, whichCounter to
// use in deciding whether to process slower, by starting from the appropriate boilerplate
// input .dat file in the dist folder [a child of execPath's folder]; or alternatively, to
// go for speed by changing the file as lodged in execPath's folder after the initial
// slower way has been used once in the session, doing the changes in-place with different
// code; because each input .dat file stays in the execPath's folder forever - unless the
// user goes in and removes it, but our implementation caters for that possibility.
// If whichCounter is input as 0, the longer move up way is done and the counter incremented,
// if >= 1 is input, then the quicker edit-in-place way is used; in the calling function.
bool KbServer::MoveOrInPlace(const int funcNumber, CAdapt_ItApp* pApp, int& whichCounter,
			wxString execPath, wxString src, wxString nonSrc,
			int nFieldSrc, int nFieldNonSrc, int nFieldKbType)
{
	wxString datFileName;
	if (funcNumber == 0) // beginning of const int's set, a do-nothing value
		return FALSE;
	if (funcNumber == blanksEnd)  // end of const int's set, a do-nothing value
		return FALSE;
	//CMainFrame* pFrame = m_pApp->GetMainFrame();
	wxString whichDATfile = wxEmptyString;
	switch (funcNumber)
	{
	case noDatFile:
	{
		break;
	}
	case credentials_for_user: // = 1
	{
		datFileName = _T("credentials_for_user.dat");
		whichDATfile = datFileName;
		pApp->m_nAddUsersCount = 0; // always 0
		break;
	}
	case lookup_user: // = 2
	{
		pApp->m_nLookupUserCount = 0; // always 0
		datFileName = _T("lookup_user.dat");
		whichDATfile = datFileName;
		break;
	}
	case list_users: // = 3
	{
		pApp->m_nListUsersCount = 0; // aways 0
		datFileName = _T("list_users.dat");
		break;
	}
	case create_entry: // = 4
	{
		datFileName = _T("create_entry.dat");
		whichDATfile = datFileName;
		// whichCounter is passed in in signature, augmenting is done below
		break;
	}
	case pseudo_delete: // = 5
	{
		datFileName = _T("pseudo_delete.dat");
		whichDATfile = datFileName;
		// whichCounter is passed in in signature, augmenting is done below
		break;
	}
	case pseudo_undelete: // = 6
	{
		datFileName = _T("pseudo_undelete.dat");
		whichDATfile = datFileName;
		// whichCounter is passed in in signature, augmenting is done below
		break;
	}
	case lookup_entry: // = 7
	{
		datFileName = _T("lookup_entry_report_results.dat");
		whichDATfile = datFileName;
		// whichCounter is passed in in signature, augmenting is done below
		break;
	}
	case changed_since_timed: // = 8
	{
		datFileName = _T("changed_since_timed_report_results.dat");
		whichDATfile = datFileName;
		// signature points at the relevant counter - leave at 0
		break;
	}
	case upload_local_kb: // = 9;
	{
		datFileName = _T("upload_local_kb.dat");
		whichDATfile = datFileName;
		break;
	}
	case change_permission: // = 10
	{
		datFileName = _T("change_permission.dat");
		whichDATfile = datFileName;
		break;
	}
	case change_fullname: // = 11
	{
		datFileName = _T("change_fullname.dat");
		whichDATfile = datFileName;
		break;
	}

	case blanksEnd:
	{
		break; // do nothing
	}
	} // end of switch:  switch (funcNumber)

	// Since the nonSrc content could be adaptation, or gloss, depending on whether kbType
	// currently in action is '1' or '2' respectively, the same code handles either in
	// the following blocks. However, we should check that the kbType value is set correctly.
	if (whichCounter == 0)
	{
		// Setting up m_pKbSvr for adaptations has just been done, so take
		// the longer processing path for the first entry to be added to the
		// entry table in this adapting sesson
		pApp->ConfigureDATfile(funcNumber); // grabs m_curNormalSource & ...Target from m_pApp
		// list_users == 3, changed_since_timed == 8
		if ((funcNumber > (int)list_users) || (funcNumber < changed_since_timed)) 
		{
			// First 3 cases are not speed critical, so leave the counter for those at zero
			// BEW 20Oct20 Bulk operations use a different length input .dat file with
			// different fields, (and no src & nonSource specified), so this function is
			// inappriate for those. For bulk operations, they lie at values of funcNumber
			// equal to or greater than 8 (8 is changed_since_timed), and so I've updated
			// the above if test to have < 8 as a second subtest, because the overhead 
			// involved in bulk operations by moving a .dat file up and populating its
			// contents after deleting the # comment lines, is miniscule

			whichCounter++; // make it non-zero for subsequent calls in this AI session
		}
		else if (funcNumber > lookup_entry) // ie, > 7
		{
			whichCounter = 0; // enforces protocol that ConfigureDatfile() is used
							  // always for the counters for handlers with funcNumber > 7
		}
	} // end of TRUE block for test: if (whichCounter == 0)
	else
	{
		// Process quicker (only for funcNumbers from 4 to 7, as these
		// just replace the src, nonSrc fields, and possibly also the kbType
		// field in the input .dat file.
		// These use counters for string fields 6 and 7 and 8(1-based counting),
		// being for src, nonSrc, and kbType; use 0 for any which
		// are not wanted for changing
		// The ipAddr of the kbserver may change without warning, so always
		// set it from the config file's value, or from the authentication
		// dialog if the latter differs from the former
		// Note: an unescaped ' ( vertical single quote / apostrophe) is a
		// string wrapper for fields in SQL requests, so if it occurs in the
		// data it will clobber the mysql parsing. So unescape any which
		// may occur, before building the fields into the command line
		int len = (int)execPath.Len();
		wxString separator = m_pApp->PathSeparator; // RHS is a string, not a (wide)char
		wxString lastChar = wxString(execPath.GetChar(len - 1));
		if (lastChar != separator)
		{
			// Add the separator to execPath
			execPath += separator;
		}
		// Now we are guaranteed that execPath ends with the path separator
		wxString datPath = execPath + datFileName;
		bool bPresent = ::FileExists(datPath);
		if (bPresent)
		{
			wxTextFile f;
			bool bIsOpened = FALSE;
			f.Create(datPath);
			bIsOpened = f.Open();
			if (bIsOpened)
			{
				wxString cmdLine = f.GetFirstLine(); // This has the fields from the last
					// call, some of which may have escaped ' in them (i.e. \' ) so we
					// need to call the escaping function only on the values being replaced

				// Always get the latest ipAddr set first, it may have changed
				wxString anIpAddr = pApp->m_strKbServerIpAddr; // start with what basic config file has
				wxString discoveryIpAddr = pApp->m_chosenIpAddr; // could be empty, if user did no discovery
				if (anIpAddr == discoveryIpAddr)
				{
					// These are the same, so it's safe to use the basic config file's value
					cmdLine = ReplaceFieldWithin(cmdLine, 1, anIpAddr);
				}  // end of TRUE block for test: if (anIpAddr == discoveryIpAddr)
				else
				{
					// The two values differ. Probably best to assume that discoveryIpAddr
					// is the one to use, provided it is not empty; as it comes from having
					// done a kbservers discovery and chosen an ipAddr from that dialog
					if (!discoveryIpAddr.IsEmpty())
					{
						cmdLine = ReplaceFieldWithin(cmdLine, 1, discoveryIpAddr);
					}
					else
					{
						// It was empty, so the best we can do is to use the old value
						// and it will probably fail. No matter, as that will cause
						// authentication to fail, which in turn will cause the project
						// to cease being a KB sharing one. That will force the user to
						// call Discover KBservers from the Advanced menu, and that will
						// present the inventory of running kbservers, and he can choose
						// one. Then remake the project as a KB sharing one, and all can
						// then be expected to be fine.
						cmdLine = ReplaceFieldWithin(cmdLine, 1, anIpAddr);
					}
				} // end of else block for test: if (anIpAddr == discoveryIpAddr)

				if (nFieldSrc != 0)
				{
					// Do any needed escapting of ' if it is in src
					src = DoEscapeSingleQuote(src);
					cmdLine = ReplaceFieldWithin(cmdLine, nFieldSrc, src);
				}
				// While we are at it, we can deal with the kbType here too, before closing
				if (gbIsGlossing)
				{
					wxString kbType = _T("2");
					if (nFieldKbType != 0)
					{
						cmdLine = ReplaceFieldWithin(cmdLine, nFieldKbType, kbType);
					}
				}
				else
				{
					wxString kbType = _T("1");
					if (nFieldKbType != 0)
					{
						cmdLine = ReplaceFieldWithin(cmdLine, nFieldKbType, kbType);
					}
				}
				f.RemoveLine(0);
				f.AddLine(cmdLine);
				bool bWroteOK = f.Write();
				f.Close();

				if (!bWroteOK)
				{
					wxString msg = _T("%s failed to write cmdLine with replacement src & nonSrc fields");
					msg = msg.Format(msg, whichDATfile.c_str());
					m_pApp->LogUserAction(msg);
					// Unlike, so a beep will do
					wxBell();
					return FALSE;
				}
			}
			// Now re-open to deal with nonSrc field
			bIsOpened = FALSE;
			f.Create(datPath);
			bIsOpened = f.Open();
			if (bIsOpened)
			{
				wxString cmdLine = f.GetFirstLine();
				if (nFieldNonSrc != 0)
				{
					// First, do any needed escaping of '
					nonSrc = DoEscapeSingleQuote(nonSrc);
					cmdLine = ReplaceFieldWithin(cmdLine, nFieldNonSrc, nonSrc); // target, or gloss
				}
				f.RemoveLine(0);
				f.AddLine(cmdLine);
				bool bWroteOK = f.Write();
				f.Close();

				if (!bWroteOK)
				{
					wxString msg = _T("%s failed to write cmdLine with replacement src & nonSrc fields");
					msg = msg.Format(msg, whichDATfile.c_str());
					m_pApp->LogUserAction(msg);
					// Unlike, so a beep will do
					wxBell();
					return FALSE;
				}
				// put a copy on the app, so that LogUserAction() can grab it if the
				// the wxExecute() in CallExecute() fails
				m_pApp->m_curCommandLine = cmdLine;
#if defined (_DEBUG)
				wxLogDebug(_T("%s::%s() line %d: commandline = %s"), __FILE__, __FUNCTION__,
					__LINE__, cmdLine.c_str());
#endif
			}

		} // end of TRUE block for test: if (bPresent)
		else
		{
			// file is absent from the execPath folder - tell user etc
			wxString msg;
			msg = msg.Format(_("The %s file is absent from the folder %s. Inserting the values: %s and  %s into the entry table failed. Adapting work can continue."),
				datFileName.c_str(), execPath.c_str(), src.c_str(), nonSrc.c_str());
			wxMessageBox(msg, _("Error - absent file"), wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(msg);
			return FALSE;
		}  // end of else block for test: if (bPresent)

#if defined (_DEBUG)
		// If funcNumber is in range, for augmenting the counter, log its vallue
		if (funcNumber > 3 && funcNumber < 8)
		{
			wxLogDebug(_T("%s::%s() line %d, whichCounter value = %d"), __FILE__,
				__FUNCTION__, __LINE__, whichCounter);
		}
#endif
	} // end of else block for test: if (whichCounter == 0)
	return TRUE;
}

// The array has no top line with "success" & timestamp, so the
// loop indexes from 0 to size - 1
bool KbServer::Line2EntryStruct(wxString& aLine)
{
	ClearEntryStruct();
	wxString comma = _T(",");
	int offset = wxNOT_FOUND;
	int index = 0; // initialise iterator

	offset = wxNOT_FOUND;
	int nLen = 0; // initialize
	wxString left;
	//int anID = 0; // initialise
	int fieldCount = 9; // id,srcLangName,tgtLangName,src,tgt,username,timestamp,type,deleted
	wxString resultLine = aLine;

	for (index = 0; index < fieldCount; index++)
	{
		// Progressively shorten the resultLine as we extract each field
		// (the final field should have a terminating comma, so number of
		// commas should equal number of fields)
		offset = resultLine.Find(comma);
		if (offset >= 0)
		{
			if (index == 0)
			{
				left = resultLine.Left(offset);
				m_entryStruct.id = left; // python converted id as int, with str(), to string
											//anID = wxAtoi(left);
											//m_entryStruct.id = (long)anID;
				nLen = left.Len();
				resultLine = resultLine.Mid(nLen + 1); // extracted id field
			}
			else if (index == 1)
			{
				left = resultLine.Left(offset);
				m_entryStruct.srcLangName = left;
				nLen = left.Len();
				resultLine = resultLine.Mid(nLen + 1); // extracted srcLangName field
			}
			else if (index == 2)
			{
				left = resultLine.Left(offset);
				m_entryStruct.tgtLangName = left;
				nLen = left.Len();
				resultLine = resultLine.Mid(nLen + 1); // extracted tgtLangName field
			}
			else if (index == 3)
			{
				left = resultLine.Left(offset);
				m_entryStruct.source = left;
				nLen = left.Len();
				resultLine = resultLine.Mid(nLen + 1); // extracted source field
			}
			else if (index == 4)
			{
				left = resultLine.Left(offset);
				m_entryStruct.nonSource = left;
				nLen = left.Len();
				resultLine = resultLine.Mid(nLen + 1); // extracted nonSource field
			}
			else if (index == 5)
			{
				left = resultLine.Left(offset);
				m_entryStruct.username = left;
				nLen = left.Len();
				resultLine = resultLine.Mid(nLen + 1); // extracted username field
			}
			else if (index == 6)
			{
				left = resultLine.Left(offset);
				m_entryStruct.timestamp = left;
				nLen = left.Len();
				resultLine = resultLine.Mid(nLen + 1); // extracted timestamp field
			}
			else if (index == 7)
			{
				left = resultLine.Left(offset);
				m_entryStruct.type = left[0];
				nLen = left.Len();
				resultLine = resultLine.Mid(nLen + 1); // extracted type field
			}
			else
			{
				// This block is for the deleted flag. Just in case there is no
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
	return TRUE;
}

// pass in execFolderPath with final PathSeparator; datFileName is a *_return_results.dat file
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
				//int anID = 0; // initialise
				if (offset >= 0)
				{
					wxString resultLine = f.GetLine((size_t)1); // comma separated, in entry table LTR order
					int fieldCount = 9; // id,srcLangName,tgtLangName,src,tgt,username,timestamp,type,deleted
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
								m_entryStruct.id = left; // python converted id as int, with str(), to string
								//anID = wxAtoi(left);
								//m_entryStruct.id = (long)anID;
								resultLine = resultLine.Mid(offset + 1); // extracted id field
							}
							else if (count == 2)
							{
								left = resultLine.Left(offset);
								m_entryStruct.srcLangName = left;
								resultLine = resultLine.Mid(offset + 1); // extracted srcLangName field
							}
							else if (count == 3)
							{
								left = resultLine.Left(offset);
								m_entryStruct.tgtLangName = left;
								resultLine = resultLine.Mid(offset + 1); // extracted tgtLangName field
							}
							else if (count == 4)
							{
								left = resultLine.Left(offset);
								m_entryStruct.source = left;
								resultLine = resultLine.Mid(offset + 1); // extracted source field
							}
							else if (count == 5)
							{
								left = resultLine.Left(offset);
								m_entryStruct.nonSource = left;
								resultLine = resultLine.Mid(offset + 1); // extracted nonSource field
							}
							else if (count == 6)
							{
								left = resultLine.Left(offset);
								m_entryStruct.username = left;
								resultLine = resultLine.Mid(offset + 1); // extracted username field
							}
							else if (count == 7)
							{
								left = resultLine.Left(offset);
								m_entryStruct.timestamp = left;
								resultLine = resultLine.Mid(offset + 1); // extracted timestamp field
							}
							else if (count == 8)
							{
								left = resultLine.Left(offset);
								m_entryStruct.type = left[0];
								resultLine = resultLine.Mid(offset + 1); // extracted type field
							}
							else
							{
								// This block is for the deleted flag. Just in case there is no
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
		wxLogDebug(_T("%s() line= %d , m_entryStruct: id= %d, srcLang= %s, tgtLang= %s, src= %s, nonSrc= %s, user= %s, time= %s, type= %s, deleted= %s"),
			__FUNCTION__, __LINE__, (int)wxAtoi(m_entryStruct.id), m_entryStruct.srcLangName.c_str(), m_entryStruct.tgtLangName.c_str(),
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

	// We now have a running pKB set to the type we want, whether for 
	// adaptations, or glosses; but since a give AI project can be currently
	// running with glossing KB active, or adaptations KB active, we need to
	// set nonScrLanguage to whatever is the correct (ie. currently active) one
	wxString srcLanguage = sourceLanguage; // from signature
	wxString nonSrcLanguage = nonSourceLanguage; // from signature
	wxString comma = _T(',');

	wxString datPath = execPath + datFilename; // execPath passed in, ends with PathSeparator char
	bool bFileExists = ::wxFileExists(datPath);
	//bool bOpened = FALSE;
	wxTextFile f;
	if (bFileExists)
	{
		// delete the file and recreate
		BOOL bDeleted = ::DeleteFile(datPath);
		if (bDeleted)
		{
			bFileExists = f.Create(datPath); // for read or write, created empty
		}
		wxASSERT(bFileExists == TRUE);
	}
	else
	{
		// create the empty file in the execPath folder
		bFileExists = f.Create(datPath); // for read or write, created empty
		wxASSERT(bFileExists == TRUE);
	}

	wxString src = wxEmptyString;  // initialise these two scratch variables
	wxString nonSrc = wxEmptyString;
	// build each "srcLangName,nonSrcLangLine,src,nonSrc,username,type,deleted" in entryLine
	wxString entryLine = wxEmptyString; 
	wxString placeholder = _T("...");

	// Set up the for loop that get's each pRefString's translation text, and bits of it's metadat
	CTargetUnit* pTU;
	size_t numMapsInUse = pKB->m_nMaxWords; // <= 10, mostly maps 1 to 5, but can be higher
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
			TranslationsList*	pTranslations = pTU->m_pTranslations;

			if (pTranslations->IsEmpty())
			{
				continue;
			}
			TranslationsList::Node* tpos = pTranslations->GetFirst();
			CRefString* pRefStr = NULL;
			CRefStringMetadata* pMetadata = NULL;
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
				entryLine += nonSrc + comma;

				// Next, the user - whoever created that entry in the local KB
				wxString user = pMetadata->GetWhoCreated();
				entryLine += user + comma;

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

				// Note, we don't grab any of the local timestamps, because the 
				// timestamp that kbserver wants is the time at which the bulk upload
				// is done. So we calculate that timestamp once in the python, and add
				// it to the being-uploaded line of each entry for the entry table 
				// in a single bulk upload call. That's because ChangedSince_Timed()
				// protocol must work with the time at which the data got inserted
				// into the remote kbserver's entry table.

				// write out the entry line
				f.AddLine(entryLine);
				f.Write();
			}
		} // end of inner for loop (for ref strings && their metadata): 
		  // for (iter = pMap->begin(); iter != pMap->end(); ++iter)
	} // end of outer for loop: for (index = 0; index < numMapsInUse; index++)
	f.Close();
	return TRUE;
} 


//=============================== end of KbServer class ============================

#endif // for _KBSERVER

