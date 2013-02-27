/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KbServer.cpp
/// \author			Kevin Bradford, Bruce Waters
/// \date_created	26 September 20012
/// \rcs_id $Id$
/// \copyright		2012 Kevin Bradford, Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for functions involved in kbserver support.
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
#endif

#include <wx/utils.h> // for ::wxDirExists, ::wxSetWorkingDirectory, etc
#include <wx/textfile.h> // for wxTextFile

#if defined(_KBSERVER)

static wxMutex s_QueueMutex; // only need one, because we cannot have
							 // glossing & adapting modes on concurrently
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
#include "Thread_UploadToKBServer.h"
#include "KbServer.h"

WX_DEFINE_LIST(DownloadsQueue);

// for wxJson support
#include "json_defs.h" // BEW tweaked to disable 64bit integers, else we get compile errors
#include "jsonreader.h"
#include "jsonwriter.h"
#include "jsonval.h"

extern bool		gbIsGlossing;

// A Note on the placement of SetupForKBServer(), and the protective wrapping boolean
// m_bIsKBServerProject, defaulting to FALSE initially and then possibly set to TRUE by
// reading the project configuration file.
// Since the KBs must be loaded before SetupForKBServer() is called, and the function for
// setting them up is CreateAndLoadKBs(), it may appear that at the end of the latter
// would be an appropriate place, within the TRUE block of an if (m_bIsKBServerProject)
// test. However, looking at which functionalities, at a higher level, call
// CreateAndLoadKBs(), many of these are too early for any association of a project with
// a kbserver to have been set up already. Therefore, possibly after a read of the
// project configuration file may be appropriate - since it's that configuration file 
// which sets or clears the m_bIsKBServerProject app member boolean. This is so: there are
// 4 contexts where the project config file is read: in DoUnpackDocument(), in
// HookUpToExistingProject() for setting up a collaboration, in the
// OpenExistingProjectDlg.cpp file, associated with the "Access an existing adaptation
// project" feature (for transforming adaptations to glosses); and most importantly, in the
// frequently called OnWizardPageChanging() function of ProjectPage.cpp. These are all
// appropriate places for calling SetupForKBServer() late in such functions, in an if TRUE
// test block using m_bIsKBServerProject. (There is no need, however, to call it in
// OpenExistingProjectDlg() because the project being opened is not changed in the process,
// and since it's adaptations are being transformed to glosses, it would not be appropriate
// to assume that the being-constructed new project should be, from it's inception, a kb
// sharing one. The user can later make it one if he so desires.)
// Scrap the above comment if we choose instead to instantiate KbServer on demand and
// destroy it immediately afterwards.

//=============================== KbServer class ===================================

std::string str_CURLbuffer;
const size_t npos = (size_t)-1; // largest possible value, for use with std:find()
std::string str_CURLheaders;

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
	m_noEntryMessage = _T("No matching entry found");
	m_existingEntryMessage = _T("Existing matching entry found");
}

KbServer::KbServer(int whichType)
{
	// This is the constructor we should always use, explicitly at least
	wxASSERT(whichType == 1 || whichType == 2);
	m_kbServerType = whichType;
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	m_pKB = GetKB(FALSE);
	m_queue.clear();
	//m_noEntryMessage = _T("No matching entry found");
	//m_existingEntryMessage = _T("Existing matching entry found");
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

// the private getters

int KbServer::GetKBServerType()
{
	return m_kbServerType;
}

wxString KbServer::GetKBServerURL()
{
	return m_kbServerURLBase;
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

// the public setters & interrogatives

void KbServer::EnableKBSharing(bool bEnable)
{
	m_bEnableKBSharing = bEnable;
}

bool KbServer::IsKBSharingEnabled()
{
	return m_bEnableKBSharing;
}

void KbServer::SetKBServerType(int type)
{
	m_kbServerType = type;
}

void KbServer::SetKBServerURL(wxString url)
{
	m_kbServerURLBase = url;
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
    // ChangedSince(), so as I've not got BOM detection and removal code in both ToUtf8()
    // and ToUtf16(), I'll round trip the dateTimeStr here to ensure that if there is a BOM
    // that has somehow crept in, then it won't find its way back to the caller!
	CBString utfStr = ToUtf8(m_kbServerLastTimestampReceived);
	m_kbServerLastTimestampReceived = ToUtf16(utfStr);
	// now proceed to update m_kbServerLastSync string with a guaranteed BOM-less timestamp
	m_kbServerLastSync = m_kbServerLastTimestampReceived;
	m_kbServerLastTimestampReceived.Empty();
	ExportLastSyncTimestamp(); // ignore the returned boolean (if FALSE, a message will have been seen)
}

void KbServer::ExtractHttpStatusEtc(std::string s, int& httpstatuscode, wxString& httpstatustext)
{
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
#if defined (_DEBUG)
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
// returned value. (This function may be called more than once in the life of a KbServer
// instance)
wxString KbServer::ImportLastSyncTimestamp()
{
	wxString dateTimeStr; dateTimeStr.Empty();

	wxString path = GetPathToPersistentDataStore() + GetPathSeparator() + GetLastSyncFilename();
	bool bLastSyncFileExists = ::wxFileExists(path);
	if (!bLastSyncFileExists)
	{
		// couldn't find lastsync... .txt file in project folder
		wxString msg;
		if (GetKBServerType() == 1)
		{
            msg = _T("wxFileExists() called in ImportLastSyncTimestamp(): The wxTextFile, taking path to lastsync_adaptations.txt, does not exist");
		}
		else
		{
            msg = _T("wxFileExists() called in ImportLastSyncTimestamp(): The wxTextFile, taking path to lastsync_glosses.txt, does not exist");
		}
		m_pApp->LogUserAction(msg);
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return dateTimeStr; // it's empty still
	}
	wxTextFile f;
	bool bSuccess = FALSE;
	// for 2.9.4 builds, the conditional compile isn't needed I think, and for Linux and
	// Mac builds which are Unicode only, it isn't needed either but I'll keep it for now
	bSuccess = GetTextFileOpened(&f, path);
	if (!bSuccess)
	{
		// warn developer that the wxTextFile could not be opened
		wxString msg = _T("GetTextFileOpened() called in ImportLastSyncTimestamp(): The wxTextFile could not be opened");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return dateTimeStr; // it's empty still
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
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		f.Close();
		return dateTimeStr; // it's empty still
	}
	// whew, finally, we have the lastsync datetime string for this kbserver type
	dateTimeStr = f.GetLine(0);
    // BEW added 28Jan13, somehow a UTF8 BOM ended up in the dateTimeStr passed to
    // ChangedSince(), so as I've not got detection and removal code in both ToUtf8() and
    // ToUtf16(), I'll round trip the dateTimeStr here to ensure that if there is a BOM
    // there then it won't find its way back to the caller!
	CBString utfStr = ToUtf8(dateTimeStr);
	dateTimeStr = ToUtf16(utfStr);

	f.Close();
	return dateTimeStr;
}

// Takes the kbserver's datetime supplied with downloaded data, as stored in the
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
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
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
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
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
	//wxString msg;
	//msg = msg.Format(_T("In curl_read_data_callback: sending %s bytes."),(size*nmemb));
	//wxLogDebug(msg);
	str_CURLbuffer.append((char*)ptr, size*nmemb);
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

	aUrl = GetKBServerURL() + slash + container + slash+ GetSourceLanguageCode() +
			slash + GetTargetLanguageCode() + slash + kbType + slash + wxStr_SourceEntry;
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
		//curl_easy_setopt(curl, CURLOPT_WRITEDATA, str_CURLbuffer); // <-- not needed
		//curl_easy_setopt(curl, CURLOPT_HEADER, 1L); // comment out when header info is
													  //not needed in the download (and below)
		result = curl_easy_perform(curl);

#if defined (_DEBUG) // && defined (__WXGTK__)
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
				_T("kbserver error"), wxICON_ERROR | wxOK);
			str_CURLbuffer.clear(); // always clear it before returning
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
	wxItoa(GetKBServerType(),kbType);
	wxString container = _T("entry");
	wxString changedSince = _T("/?changedsince=");

	aUrl = GetKBServerURL() + slash + container + slash+ GetSourceLanguageCode() + slash +
			GetTargetLanguageCode() + slash + kbType + changedSince + timeStamp;
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

#if defined (_DEBUG) // && defined (__WXGTK__)
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

#if defined (_DEBUG) // && defined (__WXGTK__)
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
        wxString strStartJSON = _T("[{");
		CBString cbstr(str_CURLbuffer.c_str());
		wxString buffer(ToUtf16(cbstr));
		int offset = buffer.Find(strStartJSON);
		if (offset == 0) // TRUE means JSON data starts at the buffer's beginning
		{
            // Before extracting the substrings from the JSON data, the storage arrays must be
            // cleared with a call of ClearAllPrivateStorageArrays()
            ClearAllPrivateStorageArrays(); // always must start off empty

            wxString myList = wxString::FromUTF8(str_CURLbuffer.c_str()); // I'm assuming no BOM gets inserted

            wxJSONValue jsonval;
            wxJSONReader reader;
            int numErrors = reader.Parse(myList, &jsonval);
            pStatusBar->UpdateProgress(_("Receiving..."), 4);

            if (numErrors > 0)
            {
                // a non-localizable message will do, it's unlikely to ever be seen
                wxMessageBox(_T("ChangedSince(): reader.Parse() returned errors, so will return wxNOT_FOUND"),
                    _T("kbserver error"), wxICON_ERROR | wxOK);
                str_CURLbuffer.clear(); // always clear it before returning
                str_CURLheaders.clear(); // always clear it before returning
                pStatusBar->FinishProgress(_("Receiving..."));
                return -1;
            }
            int listSize = jsonval.Size();
#if defined (_DEBUG)
            // get feedback about now many entries we got
            if (listSize > 0)
            {
                wxLogDebug(_T("ChangedSince() returned %d entries, for data added to kbserver since %s"),
                    listSize, timeStamp.c_str());
            }
#endif
            int index;
            for (index = 0; index < listSize; index++)
            {
                // We can extract id, source phrase, target phrase, deleted flag value,
                // username, and timestamp string; but for supporting the sync of a local KB we
                // need only to extract source phrase, target phrase, and the value of the
                // deleted flag. So the others can be commented out.
                // BEW changed 16Jan13, to have the username included in the arrays, so that we
                // can track who originated each of the entries in the group's various local KBs
                m_arrSource.Add(jsonval[index][_T("source")].AsString());
                m_arrTarget.Add(jsonval[index][_T("target")].AsString());
                m_arrDeleted.Add(jsonval[index][_T("deleted")].AsInt());
                //m_arrID.Add(jsonval[index][_T("id")].AsLong());
                m_arrUsername.Add(jsonval[index][_T("user")].AsString());
                //m_arrTimestamp.Add(jsonval[index][_T("timestamp")].AsString());
#if defined (_DEBUG)
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
				// some other HTTP error presumably, report it in debug mode, and return a
				// CURLE_HTTP_RETURNED_ERROR CURLcode; don't update the lastsync timestamp
#if defined (_DEBUG)
				wxString msg;
				msg  = msg.Format(_T("ChangedSince()error: HTTP status: %d   %s"),
                                  m_httpStatusCode, m_httpStatusText.c_str());
                wxLogDebug(msg, _T("HTTP error"), wxICON_EXCLAMATION | wxOK);
#endif
 				str_CURLbuffer.clear();
				str_CURLheaders.clear();
				pStatusBar->FinishProgress(_("Receiving..."));
				return CURLE_HTTP_RETURNED_ERROR; // = 22
           }
		}
	} // end of TRUE block for test: if (!str_CURLbuffer.empty())

    str_CURLbuffer.clear();
    str_CURLheaders.clear();
	pStatusBar->FinishProgress(_("Receiving..."));
	return 0;
}

void KbServer::ClearStrCURLbuffer()
{
	str_CURLbuffer.clear();
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
	wxString curKey;
	switch (action)
	{
	case getForOneKeyOnly:
        // I'll populate this case with minimal required code, but I'm not planning we ever
        // call this case this while the user is interactively adapting or glossing,
        // because it will slow the GUI response abysmally. Instead, we'll rely on the
        // occasional ChangedSince() calls getting the local KB populated more quickly.
		curKey = (m_pApp->m_pActivePile->GetSrcPhrase())->m_key;
		// *** NOTE *** in the above call, I've got no support for AutoCapitalization; if
		// that was wanted, more code would be needed here - or alternatively, use the
		// adjusted key from within AutoCapsLookup() which in turn would require that we
		// modify this function to pass in the adjusted string for curKey via
		// DownloadToKB's signature
// LookupEntriesForSourcePhrase()'s code is currently commented out because we don't use it
// and so I've commented this call out here; if we end up using it, then remove this
// commenting out
//		rv = LookupEntriesForSourcePhrase(curKey);
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
#if defined(_DEBUG)
		wxLogDebug(_T("Doing ChangedSince() with lastsync timestamp value = %s"), timestamp.c_str());
#endif
		rv = ChangedSince(timestamp);
		// if there was no error, update the m_kbServerLastSync value, and export it to
		// the persistent file in the project folder
		if (rv == 0)
		{
			UpdateLastSyncTimestamp();
		}
		break;
	case getAll:
		timestamp = _T("1920-01-01 00:00:00"); // earlier than everything!
		rv = ChangedSince(timestamp);
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
}

// Note: before running LookupEntryFields(), ClearStrCURLbuffer() should be called,
// and always remember to clear str_CURLbuffer before returning.
// Note 2: don't rely on CURLE_OK not being returned for a lookup failure, CURLE_OK will
// be returned even when there is no entry in the database. It's the HTTP status codes we
// need to get.
// Returns 0 (CURLE_OK) if no error, or 22 (CURLE_HTTP_RETURNED_ERROR) if there was a
// HTTP error - such as no matching entry, or a badly formed request
int KbServer::LookupEntryFields(wxString sourcePhrase, wxString targetPhrase)
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
	wxString kbType;
	wxItoa(GetKBServerType(),kbType);
	wxString container = _T("entry");

	aUrl = GetKBServerURL() + slash + container + slash+ GetSourceLanguageCode() +
			slash + GetTargetLanguageCode() + slash + kbType + slash + sourcePhrase +
			slash + targetPhrase;
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
		// We want separate storage for headers to be returned, to get the HTTP status code
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &curl_headers_callback);

		//curl_easy_setopt(curl, CURLOPT_HEADER, 1L); // comment out when collecting
													//headers separately
		result = curl_easy_perform(curl);

#if defined (_DEBUG) //&& defined (__WXGTK__)
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
			wxMessageBox(msg, _T("Error when looking up an entry"), wxICON_EXCLAMATION | wxOK);

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
		str_CURLbuffer.clear();
		str_CURLheaders.clear();
		return CURLE_HTTP_RETURNED_ERROR;
	}

	//  Make the json data accessible (result is CURLE_OK if control gets to here)
	//  We requested separate headers callback be used, so str_CURLbuffer should only have
	//  the json string for the looked up entry
	if (!str_CURLbuffer.empty())
	{
		// Note: normally before calling LookupEntryForSrcTgtPair() the storage arrays
		// should be cleared with a call of ClearAllPrivateStorageArrays()
		wxString myObject = wxString::FromUTF8(str_CURLbuffer.c_str());
		wxJSONValue jsonval;
		wxJSONReader reader;
		int numErrors = reader.Parse(myObject, &jsonval);
		if (numErrors > 0)
		{
			// a non-localizable message will do, it's unlikely to ever be seen
			// (but if we do get a parse error, then suspect that FromUTF8() may have
			// inserted a BOM - in which case recompile using ToUtf16() instead)
			wxMessageBox(_T("LookupEntryForSrcTgtPair(): reader.Parse() returned errors, so will return wxNOT_FOUND"),
				_T("kbserver error"), wxICON_ERROR | wxOK);
			str_CURLbuffer.clear(); // always clear it before returning
			str_CURLheaders.clear();
			return CURLE_HTTP_RETURNED_ERROR;
		}
		// we extract id, source phrase, target phrase, deleted flag value & username
		// for index value 0 only (there should only be one json object to deal with)
		ClearEntryStruct(); // re-initializes m_entryStruct member
		m_entryStruct.id = jsonval[_T("id")].AsLong(); // needed, as there may be a
					// subsequent pseudo-delete or undelete, and those are id-based
		m_entryStruct.source = jsonval[_T("source")].AsString();
		m_entryStruct.translation = jsonval[_T("target")].AsString();
		m_entryStruct.username = jsonval[_T("user")].AsString();
		m_entryStruct.deleted = jsonval[_T("deleted")].AsInt();

		str_CURLbuffer.clear(); // always clear it before returning
		str_CURLheaders.clear();
	}
	return 0;
}

void KbServer::ClearEntryStruct()
{
	m_entryStruct.id = 0;
	m_entryStruct.source.Empty();
	m_entryStruct.translation.Empty();
	m_entryStruct.username.Empty();
	m_entryStruct.deleted = 0;
}

void KbServer::SetEntryStruct(KbServerEntry entryStruct)
{
	m_entryStruct = entryStruct;
}

KbServerEntry KbServer::GetEntryStruct()
{
	return m_entryStruct;
}

int KbServer::CreateEntry(wxString srcPhrase, wxString tgtPhrase)
{
	// entries are always created as "normal" entries, that is, not pseudo-deleted
	CURL *curl;
	CURLcode result; // result code
	struct curl_slist* headers = NULL;
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString kbType;
	wxItoa(GetKBServerType(),kbType);
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
	jsonval[_T("targetlanguage")] = GetTargetLanguageCode();
	jsonval[_T("source")] = srcPhrase;
	jsonval[_T("target")] = tgtPhrase;
	jsonval[_T("user")] = GetKBServerUsername();
	jsonval[_T("type")] = kbType;
	jsonval[_T("deleted")] = (long)0; // i.e. a normal entry

	// convert it to string form
	wxJSONWriter writer; wxString str;
	writer.Write(jsonval, str);
	// convert it to utf-8 stored in CBString
	strVal = ToUtf8(str);

	aUrl = GetKBServerURL() + slash + container + slash;
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

#if defined (_DEBUG) // && defined (__WXGTK__)
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
	return 0;
}

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
	wxItoa(GetKBServerType(),kbType);
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

	aUrl = GetKBServerURL() + slash + container + slash + entryIDStr;
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

#if defined (_DEBUG) // && defined (__WXGTK__)
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
			}
			else
			{
				wxMessageBox(msg, _T("Error when trying to undelete an entry"), wxICON_EXCLAMATION | wxOK);
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

void KbServer::DoChangedSince()
{
	int rv = 0; // rv is "return value", initialize it
	wxString timestamp;
	// get the last sync timestamp value
	timestamp = GetKBServerLastSync();
#if defined(_DEBUG)
	wxLogDebug(_T("DoChangedSince() with lastsync timestamp value = %s"), timestamp.c_str());
#endif
	rv = ChangedSince(timestamp);
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

void KbServer::DoGetAll()
{
	int rv = 0; // rv is "return value", initialize it
	// get the last sync timestamp value
	wxString timestamp = _T("1920-01-01 00:00:00"); // earlier than everything!;
#if defined(_DEBUG)
	wxLogDebug(_T("DoGetAll() with lastsync timestamp value = %s"), timestamp.c_str());
#endif
	rv = ChangedSince(timestamp);
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

/*  we probably won't use threading for a bulk upload
void KbServer::UploadToKbServerThreaded()
{
	// Here's where I'll test doing this on a thread
	Thread_UploadToKBServer* pUploadToKBServerThread = new Thread_UploadToKBServer;
	pUploadToKBServerThread->m_pKbSvr = this;

	// now create the runnable thread with explicit stack size of 10KB
	wxThreadError error =  pUploadToKBServerThread->Create();  //10240
	if (error != wxTHREAD_NO_ERROR)
	{
		wxString msg;
		msg = msg.Format(_T("Thread_UploadToKBServer(): thread creation failed, error number: %d"),
			(int)error);
		wxMessageBox(msg, _T("Thread creation error"), wxICON_EXCLAMATION | wxOK);
		m_pApp->LogUserAction(msg);
	}
	// now run the thread (it will destroy itself when done)
	error = pUploadToKBServerThread->Run();
	if (error != wxTHREAD_NO_ERROR)
	{
		wxString msg;
		msg = msg.Format(_T("Thread_Run(): cannot make the thread run, error number: %d"),
			(int)error);
		wxMessageBox(msg, _T("Thread start error"), wxICON_EXCLAMATION | wxOK);
		m_pApp->LogUserAction(msg);
	}
}
*/

// TODO -- the bulk upload function here; currently it's just used for testing code
void KbServer::UploadToKbServer()
{
	wxString srcPhrase = _T("graun");
	wxString tgtPhrase = _T("earth");
	long entryID = 0; // initialize (it might not be used)
	int rv = CreateEntry(srcPhrase,tgtPhrase);
	if (rv == CURLE_HTTP_RETURNED_ERROR)
	{
		int rv2 = LookupEntryFields(srcPhrase, tgtPhrase);
		KbServerEntry e = GetEntryStruct();
		entryID = e.id; // an undelete of a pseudo-delete will need this value
#if defined(_DEBUG)
		wxLogDebug(_T("LookupEntryFields: for [%s & %s]: id = %d , source = %s , translation = %s , deleted = %d , username = %s"),
			srcPhrase.c_str(), tgtPhrase.c_str(), e.id, e.source.c_str(), e.translation.c_str(), e.deleted, e.username.c_str());
#endif
		if (rv2 == CURLE_HTTP_RETURNED_ERROR)
		{
#if defined(_DEBUG)
			wxBell(); // we don't expect any error
#endif
			;
		}
		else
		{
			if (e.deleted == 1)
			{
				// do an un-pseudodelete here, use the entryID value above
				// (reuse rv2, because if it fails we'll attempt nothing additional
				//  here, not even to tell the user anything)
				rv2 = PseudoDeleteOrUndeleteEntry(entryID, doUndelete);
			}
		}

// it all worked, and in the end {graun,earth,1} became {graun,earth,0} Yay!


	}
/*
	// test queue
	KbServerEntry* pEntry = new KbServerEntry;
	pEntry->id = 1;
	m_queue.push_back(pEntry);
	pEntry = new KbServerEntry;
	pEntry->id = 2;
	m_queue.push_back(pEntry);
	pEntry = new KbServerEntry;
	pEntry->id = 3;
	m_queue.push_back(pEntry);

	KbServerEntry* pPopped = NULL;

	pPopped = m_queue.front();
	wxASSERT(pPopped->id == 1);
	m_queue.pop_front();

	pPopped = m_queue.front();
	wxASSERT(pPopped->id == 2);
	m_queue.pop_front();

	pPopped = m_queue.front();
	wxASSERT(pPopped->id == 3);
	m_queue.pop_front();

	wxASSERT(m_queue.GetCount() == 0);
*/
/*
	wxString srcPhrase;
	CTargetUnit* cTU;
	wxString tgtPhrase;
	int iTotalSent = 0;

	CKB* currKB = this->GetKB( GetKBServerType() ); //Glossing = KB Type 2

	//Need to get each map
	for (int i = 0; i< MAX_WORDS; i++)
	{
		//for each map
		for (MapKeyStringToTgtUnit::iterator iter = currKB->m_pMap[i]->begin(); iter != currKB->m_pMap[i]->end(); ++iter)
		{
			wxASSERT(currKB->m_pMap[i] != NULL);

			if (!currKB->m_pMap[i]->empty())
			{
				srcPhrase = iter->first;

				cTU = iter->second;
				CRefString* pRefString = NULL;
				TranslationsList::Node* pos = cTU->m_pTranslations->GetFirst();
				wxASSERT(pos != NULL);

				wxDateTime now = wxDateTime::Now();
				wxLogDebug(_T("UploadToKBServer() start time: %s\n"), now.Format(_T("%c"), wxDateTime::CET).c_str());

				while (pos != NULL)
				{
					pRefString = (CRefString*)pos->GetData();
					wxASSERT(pRefString != NULL);
					pos = pos->GetNext();

					if (!pRefString->m_translation.IsEmpty())
					{
						CreateEntry(srcPhrase, pRefString->m_translation, pRefString->GetDeletedFlag());
						// test info
						wxDateTime now = wxDateTime::Now();
						iTotalSent++;
						wxLogDebug(_T("%d UploadToKBServer()->CreateEntry() [time]:  %s    [source]:  %s    [target]:  %s"),
							iTotalSent, now.Format(_T("%c"), wxDateTime::CET).c_str(),
							srcPhrase.c_str(), pRefString->m_translation.c_str());
					}
				}
			}
		}
	} // for
	wxLogDebug(_T("UploadToKBServer() Done!"));

*/
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

// Return the CURLcode value, downloaded JSON data is extracted and copied, entry by
// entry, into a series of KbServerEntry structs, each created on the heap, and stored at
// the end of the m_queue member (derived from wxList<T>). This ChangedSince_Queued() is
// based on ChangedSince(), and differs only in that (1) it is called in a detached thread
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
int KbServer::ChangedSince_Queued(wxString timeStamp)
{
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
	wxItoa(GetKBServerType(),kbType);
	wxString container = _T("entry");
	wxString changedSince = _T("/?changedsince=");

	aUrl = GetKBServerURL() + slash + container + slash+ GetSourceLanguageCode() + slash +
			GetTargetLanguageCode() + slash + kbType + changedSince + timeStamp;
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

#if defined (_DEBUG) // && defined (__WXGTK__)
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

#if defined (_DEBUG) // && defined (__WXGTK__)
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
        wxString strStartJSON = _T("[{");
		CBString cbstr(str_CURLbuffer.c_str());
		wxString buffer(ToUtf16(cbstr));
		int offset = buffer.Find(strStartJSON);
		if (offset == 0) // TRUE means JSON data starts at the buffer's beginning
		{
            // Before extracting the substrings from the JSON data, the storage arrays must be
            // cleared with a call of ClearAllPrivateStorageArrays()
            ClearAllPrivateStorageArrays(); // always must start off empty

            wxString myList = wxString::FromUTF8(str_CURLbuffer.c_str()); // I'm assuming no BOM gets inserted

            wxJSONValue jsonval;
            wxJSONReader reader;
            int numErrors = reader.Parse(myList, &jsonval);

            if (numErrors > 0)
            {
                // a non-localizable message will do, it's unlikely to ever be seen
                wxMessageBox(_T("ChangedSince_Queued(): reader.Parse() returned errors, so will return wxNOT_FOUND"),
                    _T("kbserver error"), wxICON_ERROR | wxOK);
                str_CURLbuffer.clear(); // always clear it before returning
                str_CURLheaders.clear(); // always clear it before returning
               return -1;
            }
            int listSize = jsonval.Size();
#if defined (_DEBUG)
            // get feedback about now many entries we got, in debug mode
            if (listSize > 0)
            {
                wxLogDebug(_T("ChangedSince_Queued() returned %d entries, for data added to kbserver since %s"),
                    listSize, timeStamp.c_str());
            }
#endif
            int index;
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
				pEntryStruct->translation = jsonval[index][_T("target")].AsString();
				pEntryStruct->deleted = jsonval[index][_T("deleted")].AsInt();
				pEntryStruct->username = jsonval[index][_T("user")].AsString();

                // Append to the end of the queue (if the main thread is removing the first
                // struct in the queue currently, this will block until the s_QueueMutex is
                // released)
				PushToQueueEnd(pEntryStruct);
#if defined (_DEBUG)
                // list what entries were returned
				wxLogDebug(_T("Queued: Downloaded:  %s  ,  %s  ,  deleted = %d  ,  username = %s"),
                    pEntryStruct->source.c_str(), pEntryStruct->translation.c_str(),
					pEntryStruct->deleted, pEntryStruct->username.c_str());
#endif
            }

            str_CURLbuffer.clear(); // always clear it before returning
            str_CURLheaders.clear(); // BEW added 9Feb13

			// since all went successfully, update the lastsync timestamp
			UpdateLastSyncTimestamp();
		}
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
                UpdateLastSyncTimestamp();
            }
            else
            {
				// some other HTTP error presumably, report it in debug mode, and return a
				// CURLE_HTTP_RETURNED_ERROR CURLcode; don't update the lastsync timestamp
#if defined (_DEBUG)
				wxString msg;
				msg  = msg.Format(_T("ChangedSince_Queued() error:  HTTP status: %d   %s"),
                                  m_httpStatusCode, m_httpStatusText.c_str());
                wxLogDebug(msg, _T("HTTP error"), wxICON_EXCLAMATION | wxOK);
#endif
				str_CURLbuffer.clear();
				str_CURLheaders.clear();
				return CURLE_HTTP_RETURNED_ERROR; // = 22
            }
		}
	} // end of TRUE block for test: if (!str_CURLbuffer.empty())

    str_CURLbuffer.clear(); // always clear it before returning
    str_CURLheaders.clear(); // BEW added 9Feb13
	return 0;
}

//=============================== end of KbServer class ============================

#endif // for _KBSERVER

