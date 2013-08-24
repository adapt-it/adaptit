/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KbServer.cpp
/// \author			Kevin Bradford, Bruce Waters
/// \date_created	26 September 2012
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
static wxMutex s_DoGetAllMutex; // UploadToKbServer() calls DoGetAll() which
			// fills the 7 parallel arrays with remote DB data; but a manual
			// ChangedSince() or DoChangedSince() call also fills the same
			// arrays - so we have to enforce sequentiality on the use of
			// these arrays
wxMutex KBAccessMutex; // ChangedSince() may be entering entries into
			// the local KB at while UploadToKbServer() is looping over it's entries
			// to work out which need to be sent to the remote DB
			
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
//#include "Thread_UploadToKBServer.h"
#include "Thread_UploadMulti.h"
#include "KbServer.h"
#include "md5_SB.h"

WX_DEFINE_LIST(DownloadsQueue);
WX_DEFINE_LIST(UploadsList);  // for use by Thread_UploadMulti, for kbserver support
							  // (see member m_uploadsList)
WX_DEFINE_LIST(UsersList);    // for use by the ListUsers() client, stores KbServerUser structs
WX_DEFINE_LIST(KbsList);    // for use by the ListKbs() client, stores KbServerKb structs

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

// The UploadToKbServer() call, will create up to 50 threads. To permit parallel
// processing we need an array of 50 standard buffers, so that we don't have to force
// sequentiality on the processing of the threads
std::string str_CURLbuff[50];

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
	m_bStateless = FALSE;
}

KbServer::KbServer(int whichType)
{
	// This is the constructor we should always use, explicitly at least
	wxASSERT(whichType == 1 || whichType == 2);
	m_kbServerType = whichType;
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	m_pKB = GetKB(m_kbServerType);
	m_queue.clear();
	m_bStateless = FALSE;
}

// I had to give the next contructor the int whichType param, because just having a single
// bool param isn't enough to distinguish it from KbServer(int whichType) above, and the
// compiler was wrongly calling the above, instead KbServer(bool bStateless)
KbServer::KbServer(int whichType, bool bStateless)
{
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	m_pKB = NULL; // There may be no KB loaded yet, and we don't need 
				  // it for the KB Sharing Manager GUI's use
	// do something trivial with the parameter whichType to avoid a compiler warning 
	// about an unused variable
	if (whichType != 1)
	{
		whichType = 1; // we don't want any CKB instance, 
					   // but if we did, we'd just use an adapting one
	}
	m_queue.clear();
	m_bStateless = bStateless;
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
	wxString dateTimeStr;
	dateTimeStr = _T("2012-01-01 00:00::00"); // initialize to a safe "early" value

	wxString path = GetPathToPersistentDataStore() + GetPathSeparator() + GetLastSyncFilename();
	bool bLastSyncFileExists = ::wxFileExists(path);
	if (!bLastSyncFileExists)
	{
		// couldn't find lastsync... .txt file in project folder
		// BEW 13Jun13, don't just show an error message, do the job for the user - make a
		// file and put an "early" timestamp in it guaranteed to be earlier than anyone's
		// actual work using a shared KB. E.g. "2012-01-01 00:00::00" as above
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
			msg = msg.Format(_T("Failed to create last sync file with path: %s\n\"2012-01-01 00:00::00\" will be used for the imported timestamp, so you can continue working."),
								dateTimeStr.c_str());
			wxMessageBox(msg, _T("Text File Creation Error"), wxICON_ERROR | wxOK);
			return dateTimeStr; // send something useful
		}

		/* deprecated 13Jun13
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
		*/
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
	//msg = msg.Format(_T("In curl_read_data_callback: sending %d bytes."),(size*nmemb));
	//wxLogDebug(msg);
	str_CURLbuffer.append((char*)ptr, size*nmemb);
	return size*nmemb;
}

size_t curl_read_data_callback2(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	int threadIndex = *(int*)userdata; 
	str_CURLbuff[threadIndex].append((char*)ptr, size*nmemb);
#if defined(_DEBUG)
	wxString msg;
	msg = msg.Format(_T("In curl_read_data_callback2: sending %d bytes, threadIndex = %d"),
						(size*nmemb), threadIndex);
	wxLogDebug(msg);
#endif
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

            wxString jsonArray = wxString::FromUTF8(str_CURLbuffer.c_str()); // I'm assuming no BOM gets inserted

            wxJSONValue jsonval;
            wxJSONReader reader;
            int numErrors = reader.Parse(jsonArray, &jsonval);
            pStatusBar->UpdateProgress(_("Receiving..."), 4);

            if (numErrors > 0)
            {
                // a non-localizable message will do, it's unlikely to ever be seen
                wxMessageBox(_T("ChangedSince(): json reader.Parse() failed. Unexpected bad data from server"),
                    _T("kbserver error"), wxICON_ERROR | wxOK);
                str_CURLbuffer.clear(); // always clear it before returning
                str_CURLheaders.clear(); // always clear it before returning
                pStatusBar->FinishProgress(_("Receiving..."));
                return -1;
            }
            size_t arraySize = jsonval.Size();
#if defined (_DEBUG)
            // get feedback about now many entries we got
            if (arraySize > 0)
            {
                wxLogDebug(_T("ChangedSince() returned %d entries, for data added to kbserver since %s"),
                    arraySize, timeStamp.c_str());
            }
#endif
            size_t index;
            for (index = 0; index < arraySize; index++)
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
                wxMessageBox(msg, _T("HTTP error"), wxICON_EXCLAMATION | wxOK);
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
	wxString curKey;
	s_DoGetAllMutex.Lock();
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

// Note: before running ListUsers(), ClearStrCURLbuffer() should be called,
// and always remember to clear str_CURLbuffer before returning.
// Note 2: don't rely on CURLE_OK not being returned for a lookup failure, CURLE_OK will
// be returned even when there is no entry in the database. It's the HTTP status codes we
// need to get.
// Returns 0 (CURLE_OK) if no error, or 22 (CURLE_HTTP_RETURNED_ERROR) if there was a
// HTTP error - such as no matching entry, or a badly formed request
int KbServer::ListUsers(wxString username, wxString password)
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
	wxString container = _T("user");

	aUrl = GetKBServerURL() + slash + container;
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

#if defined (_DEBUG) //&& defined (__WXGTK__)
        CBString s2(str_CURLheaders.c_str());
        wxString showit2 = ToUtf16(s2);
		wxLogDebug(_T("ListUsers(): Returned headers: %s"), showit2.c_str());

        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
		wxLogDebug(_T("ListUsers() str_CURLbuffer has: %s    , The CURLcode is: %d"), 
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
			msg = msg.Format(_T("LookupUser() result code: %d cURL Error: %s"), 
				result, error.c_str());
			wxMessageBox(msg, _T("Error when looking up a username"), wxICON_EXCLAMATION | wxOK);

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
	//  the json string for the constructed list of user entries
	if (!str_CURLbuffer.empty())
	{
		wxString myArray = wxString::FromUTF8(str_CURLbuffer.c_str());
		wxJSONValue jsonval;
		wxJSONReader reader;
		int numErrors = reader.Parse(myArray, &jsonval);
		if (numErrors > 0)
		{
			// A non-localizable message will do, it's unlikely to happen (we hope)
			wxMessageBox(_T("In ListUsers(): json reader.Parse() failed. Unexpected bad data from server."),
				_T("kbserver error"), wxICON_ERROR | wxOK);
			str_CURLbuffer.clear(); // always clear it before returning
			str_CURLheaders.clear();
			return CURLE_HTTP_RETURNED_ERROR;
		}
        // We extract everything: id, username, fullname, kbadmin flag value, useradmin
        // flag value, and the timestamp at which the username was added to the entry table
		ClearUsersList(&m_usersList); // deletes from the heap any KbServerUser structs still in m_userList
		wxASSERT(m_usersList.empty());
        size_t arraySize = jsonval.Size();
		wxASSERT(arraySize > 0);
        size_t index;
        for (index = 0; index < arraySize; index++)
        {
			KbServerUser* pUserStruct = new KbServerUser;
			// Extract the field values, store them in pUserStruct
			pUserStruct->id = jsonval[index][_T("id")].AsLong();
			pUserStruct->username = jsonval[index][_T("username")].AsString();
			pUserStruct->fullname = jsonval[index][_T("fullname")].AsString();
			// do the following fiddle to avoid a compiler "performance warning" 
			// if a (bool) cast was used instead
			unsigned long val = jsonval[index][_T("kbadmin")].AsLong();
			pUserStruct->kbadmin = val == 1L ? TRUE : FALSE;
			val = jsonval[index][_T("useradmin")].AsLong();
			pUserStruct->useradmin = val == 1L ? TRUE : FALSE;
			pUserStruct->timestamp = jsonval[index][_T("timestamp")].AsString();
			// Add the pUserStruct to the m_usersList stored in the KbServer instance
			// which is this (Caller should only use the adaptations instance of KbServer)
			m_usersList.Append(pUserStruct); // Caller must later use ClearUsersList() to get
									// rid of these pointers once their job is done, if not,
									// memory will be leaked
#if defined (_DEBUG)
			wxLogDebug(_T("ListUsers(): id = %d username = %s , fullname = %s useradmin = %d , kbadmin = %d  timestamp = %s"),
				pUserStruct->id, pUserStruct->username.c_str(), pUserStruct->fullname.c_str(),
				pUserStruct->useradmin ? 1 : 0, pUserStruct->kbadmin ? 1 : 0, pUserStruct->timestamp.c_str());
#endif
		}

		str_CURLbuffer.clear(); // always clear it before returning
		str_CURLheaders.clear();
	}
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

	aUrl = GetKBServerURL() + slash + container;
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

#if defined (_DEBUG) //&& defined (__WXGTK__)
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
		return CURLE_HTTP_RETURNED_ERROR; // 22
	}

	//  Make the json data accessible (result is CURLE_OK if control gets to here)
	//  We requested separate headers callback be used, so str_CURLbuffer should only have
	//  the json string for the constructed json array of kb entries
	if (!str_CURLbuffer.empty())
	{
		wxString myArray = wxString::FromUTF8(str_CURLbuffer.c_str());
		wxJSONValue jsonval;
		wxJSONReader reader;
		int numErrors = reader.Parse(myArray, &jsonval);
		if (numErrors > 0)
		{
			// A non-localizable message will do, it's unlikely to happen (we hope)
			wxMessageBox(_T("In ListKbs(): json reader.Parse() failed. Unexpected bad data from server."),
				_T("kbserver error"), wxICON_ERROR | wxOK);
			str_CURLbuffer.clear(); // always clear it before returning
			str_CURLheaders.clear();
			return CURLE_HTTP_RETURNED_ERROR;
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
			pKbStruct->targetLanguageCode = jsonval[index][_T("targetlanguage")].AsString();
			pKbStruct->kbType = jsonval[index][_T("type")].AsLong();
			pKbStruct->username = jsonval[index][_T("user")].AsString();
			unsigned long val = jsonval[index][_T("deleted")].AsLong();
			pKbStruct->deleted = val == 1L ? TRUE : FALSE;
			pKbStruct->timestamp = jsonval[index][_T("timestamp")].AsString();

			// Add the pKbStruct to the m_kbsList stored in the KbServer instance
			// which is this (Caller should only use the adaptations instance of KbServer)
			m_kbsList.Append(pKbStruct); // Caller must later use ClearKbsList() to get
									// rid of these pointers once their job is done, if not,
									// memory will be leaked
#if defined (_DEBUG)
			wxLogDebug(_T("ListKbs(): id = %d , sourcelanguage = %s targetlanguage = %s , type = %d , user = %s , timestamp = %s"),
				pKbStruct->id , pKbStruct->sourceLanguageCode.c_str(), pKbStruct->targetLanguageCode.c_str(), 
				pKbStruct->kbType , pKbStruct->username.c_str(), pKbStruct->timestamp.c_str());
#endif
		}

		str_CURLbuffer.clear(); // always clear it before returning
		str_CURLheaders.clear();
	}
	return 0;
}


// Note: before running LookupUser(), ClearStrCURLbuffer() should be called,
// and always remember to clear str_CURLbuffer before returning.
// Note 2: don't rely on CURLE_OK not being returned for a lookup failure, CURLE_OK will
// be returned even when there is no entry in the database. It's the HTTP status codes we
// need to get.
// Returns 0 (CURLE_OK) if no error, or 22 (CURLE_HTTP_RETURNED_ERROR) if there was a
// HTTP error - such as no matching entry, or a badly formed request
// Note: url, username and password are passed in, because this request can be made before
// the app's m_pKbServer[2] pointers have been instantiated
int KbServer::LookupUser(wxString url, wxString username, wxString password, wxString whichusername)
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
	wxString container = _T("user");

	aUrl = url + slash + container + slash + whichusername;
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

#if defined (_DEBUG) //&& defined (__WXGTK__)
        CBString s2(str_CURLheaders.c_str());
        wxString showit2 = ToUtf16(s2);
		wxLogDebug(_T("LookupUser(): Returned headers: %s"), showit2.c_str());

        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
		wxLogDebug(_T("LookupUser() str_CURLbuffer has: %s    , The CURLcode is: %d"), 
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
			msg = msg.Format(_T("LookupUser() result code: %d cURL Error: %s"), 
				result, error.c_str());
			wxMessageBox(msg, _T("Error when looking up a username"), wxICON_EXCLAMATION | wxOK);

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
			// A non-localizable message will do, it's unlikely to happen (we hope)
			wxMessageBox(_T("In LookupUser(): json reader.Parse() failed. Unexpected bad data from server."),
				_T("kbserver error"), wxICON_ERROR | wxOK);
			str_CURLbuffer.clear(); // always clear it before returning
			str_CURLheaders.clear();
			return CURLE_HTTP_RETURNED_ERROR;
		}
		// We extract id, username, fullname, kbadmin flag value, useradmin flag value,
		// and the timestamp at which the definition was added to the user table.
		ClearUserStruct(); // re-initializes m_userStruct member to be empty
		m_userStruct.id = jsonval[0][_T("id")].AsLong();
		m_userStruct.username = jsonval[0][_T("username")].AsString();
		m_userStruct.fullname = jsonval[0][_T("fullname")].AsString();
		// do the following fiddle to avoid a compiler "performance warning" 
		// if a (bool) cast was used instead
		unsigned long val = jsonval[0][_T("kbadmin")].AsLong();
		m_userStruct.kbadmin = val == 1L ? TRUE : FALSE;
		val = jsonval[0][_T("useradmin")].AsLong();
		m_userStruct.useradmin = val == 1L ? TRUE : FALSE;
		m_userStruct.timestamp = jsonval[0][_T("timestamp")].AsString();

		str_CURLbuffer.clear(); // always clear it before returning
		str_CURLheaders.clear();
	}
	return 0;
}

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
int KbServer::LookupSingleKb(wxString url, wxString username, wxString password,
					wxString srcLangCode, wxString tgtLangCode, int kbType, bool& bMatchedKB)
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

#if defined (_DEBUG) //&& defined (__WXGTK__)
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
        wxString strStartJSON = _T("[{");
		CBString cbstr(str_CURLbuffer.c_str());
		wxString buffer(ToUtf16(cbstr));
		int offset = buffer.Find(strStartJSON);
		if (offset == 0) // TRUE means JSON data starts at the buffer's beginning
		{		
			wxString myObject = wxString::FromUTF8(str_CURLbuffer.c_str());
			wxJSONValue jsonval;
			wxJSONReader reader;
			int numErrors = reader.Parse(myObject, &jsonval);
			if (numErrors > 0)
			{
				// A non-localizable message will do, it's unlikely to happen often
				// but if it does, there is probably a connection problem that the user should
				// deal with if possible, before going much further. 
				wxMessageBox(_T("In LookupSingleKb(): json reader.Parse() failed. Unexpected bad data from server."),
					_T("kbserver error"), wxICON_ERROR | wxOK);
				str_CURLbuffer.clear(); // always clear it before returning
				str_CURLheaders.clear();
				return -1; // this is better, neither curl nor http failed
				//return CURLE_HTTP_RETURNED_ERROR;
			}
			unsigned int listSize = jsonval.Size();
#if defined (_DEBUG)
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
				pKbStruct->kbType = jsonval[index][_T("type")].AsInt();
				pKbStruct->username = jsonval[index][_T("username")].AsString(); // this is who created
									// this particular entry in the kb table; which is not something
									// we particularly care about for the client API functions
				pKbStruct->timestamp = jsonval[index][_T("timestamp")].AsString();
				pKbStruct->deleted = jsonval[index][_T("deleted")].AsInt();

#if defined (_DEBUG)
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
					bMatchedKB = TRUE; // the looked up KB exists in the kb table of this kbserver
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
#if defined (_DEBUG)
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
			// A non-localizable message will do, it's unlikely to happen (we hope)
			wxMessageBox(_T("In LookupEntryFields(): json reader.Parse() failed. Unexpected bad data from server."),
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
	m_kbStruct.id = 0;
	m_kbStruct.sourceLanguageCode.Empty();
	m_kbStruct.targetLanguageCode.Empty();
	m_kbStruct.kbType = 1; // default to adaptations KB type
	m_kbStruct.username.Empty();
	m_kbStruct.timestamp.Empty();
	m_kbStruct.deleted = 0;
}


//void KbServer::SetUserStruct(KbServerUser userStruct)
//{
//	m_userStruct = userStruct;
//}

KbServerUser KbServer::GetUserStruct()
{
	return m_userStruct;
}

UsersList* KbServer::GetUsersList()
{
	return &m_usersList;
}

// deletes from the heap all KbServerUser struct ptrs within m_usersList
void KbServer::ClearUsersList(UsersList* pUsrList)
{
	if (pUsrList ==NULL || pUsrList->empty())
		return;
	UsersList::iterator iter;
	UsersList::compatibility_iterator c_iter;
	int anIndex = -1;
	for (iter = pUsrList->begin(); iter != pUsrList->end(); ++iter)
	{
		anIndex++;
		c_iter = pUsrList->Item((size_t)anIndex);
		KbServerUser* pEntry = c_iter->GetData();
		if (pEntry != NULL)
		{
			delete pEntry; // frees its memory block
		}
	}
	// The list's stored pointers are now hanging, so clear them
	pUsrList->clear();
}

KbsList* KbServer::GetKbsList()
{
	return &m_kbsList;
}

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



/* Not used, commented out by BEW 5Jun13
// Same as CreateEntry(), but with anything related to failure stripped out - we want this
// to succeed as quickly as possible, and we'll ignore failures
// Note: we pass in, by value, everything the function needs, so that it calls nothing
// external to itself. This makes it safe to use in a thread without a mutex being needed
// - provided the thread has public variables for those in the signature below, & those in
// the thread get set after the thread is created and before it is run
int KbServer::CreateEntry_Minimal(	KbServerEntry& entry,
									wxString& kbType,
									wxString& password,
									wxString& username,
									wxString& srcLangCode,
									wxString& translnLangCode, // tgt code or gloss code
									wxString& url)
{
	CURL *curl;
	CURLcode result = CURLE_OK; // initialize result code
	struct curl_slist* headers = NULL;
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxJSONValue jsonval; // construct JSON object
	CBString strVal; // to store wxString form of the jsonval object, for curl
	wxString container = _T("entry");
	wxString aUrl, aPwd;

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = username + colon + password;
	charUserpwd = ToUtf8(aPwd);

	// populate the JSON object
	jsonval[_T("sourcelanguage")] = srcLangCode;
	jsonval[_T("targetlanguage")] = translnLangCode;
	jsonval[_T("source")] = entry.source;
	jsonval[_T("target")] = entry.translation;
	jsonval[_T("user")] = username;
	jsonval[_T("type")] = kbType;
	jsonval[_T("deleted")] = (long)0; // i.e. a normal entry

	// convert it to string form
	wxJSONWriter writer; wxString str;
	writer.Write(jsonval, str);
	// convert it to utf-8 stored in CBString
	strVal = ToUtf8(str);

	aUrl = url + slash + container + slash;
	charUrl = ToUtf8(aUrl);

	// prepare curl
	curl = curl_easy_init();

	if (curl)
	{
		// add headers
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "Accept: application/json");
		// set data & options
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)strVal);
		// transmit
		result = curl_easy_perform(curl);		
		curl_slist_free_all(headers);
	}
	curl_easy_cleanup(curl);
//#if defined(_DEBUG)
//	wxLogDebug(_T("CreateEntry_Minimal(): src= %s  tgt= %s  result= %d"),
//		entry.source, entry.translation, result);
//#endif
	return result;
}
*/
int KbServer::CreateEntry(wxString srcPhrase, wxString tgtPhrase)
{
	// entries are always created as "normal" entries, that is, not pseudo-deleted
	CURL *curl;
	CURLcode result = CURLE_OK; // initialize result code
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

int	KbServer::CreateUser(wxString username, wxString fullname, wxString hisPassword, bool bKbadmin, bool bUseradmin)
{
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
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

		result = curl_easy_perform(curl);

#if defined (_DEBUG) // && defined (__WXGTK__)
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

int KbServer::CreateKb(wxString srcLangCode, wxString nonsrcLangCode, bool bKbTypeIsScrTgt)
{
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

	aUrl = GetKBServerURL() + slash + container;
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

int KbServer::UpdateUser(int userID, bool bUpdateUsername, bool bUpdateFullName, 
						bool bUpdatePassword, bool bUpdateKbadmin, bool bUpdateUseradmin, 
						KbServerUser* pEditedUserStruct, wxString password)
{
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

// *** TODO **** possibly I need to make a digest password here, using my md5 function,
// and username, realm, and the passed in password -- I've asked Jonathan, no
// reply yet ********************************************************************************************************************************************	
		//jsonval[_T("password")] = password;
		
		// trial code... using digest created here from password passed in; sb
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
/*
************ BE sure to verify with Jonathan that the above is the right thing to do !!!! **************
*/
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

	aUrl = GetKBServerURL() + slash + container + slash + userIDStr;
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

// Returns a CURLcode. The only fields we allow to be updated are sourcelanguage (code)
// and targetlanguage (code). We don't allow changing of the type. (However, it is
// possible to effect this in a round-about way, using the Adapt It gui - using the
// Advanced menu item "Transform adaptations into glosses..." - this takes a src-tgt
// project and makes the tgt adaptations become glosses in a new project, and it
// transforms the adapting KB in the original project into the glossing KB in the new
// project. Then if the user is authorized to make the new project a shared one and does
// so, he can do a bulk upload from the new project, and thereby populate the remote
// shared KB with adaptions-now-turned-into-glosses). 
// 
// Note: if this returns CURLcode CURLE_OK, and there's no HTTP error, then this function
// should be followed up with a function that causes the code or codes to be updated in
// the entries of the entry table -- or the php for this present function should do it
// I'm awaiting a decision from Jonathan about this....
int KbServer::UpdateKb(int kbID, bool bUpdateSourceLanguageCode, bool bUpdateNonSourceLanguageCode,  
						int kbType, KbServerKb* pEditedKbStruct)
{
	CURLcode result = CURLE_OK;
	wxString kbIDStr;
	wxItoa(kbID, kbIDStr);
	CURL *curl;
	struct curl_slist* headers = NULL;
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString kbTypeStr; // we don't actually need it, but no harm in passing in the kbType
	wxItoa(kbType,kbTypeStr);
	wxJSONValue jsonval; // construct JSON object
	CBString strVal; // to store wxString form of the jsonval object, for curl
	wxString container = _T("kb");
	wxString aUrl, aPwd;

	str_CURLbuffer.clear(); // use for headers return when there's no json to be returned

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	// populate the JSON object
	if (bUpdateSourceLanguageCode)
	{
		jsonval[_T("sourcelanguage")] = pEditedKbStruct->sourceLanguageCode;
#if defined(_DEBUG)
		wxLogDebug(_T("  UpdateKb(): updating sourcelanguage code to:  %s  ,  kbType %s"),
			pEditedKbStruct->sourceLanguageCode.c_str(), kbTypeStr.c_str()); 
#endif
	}
	if (bUpdateNonSourceLanguageCode)
	{
		jsonval[_T("targetlanguage")] = pEditedKbStruct->targetLanguageCode;
#if defined(_DEBUG)
		wxLogDebug(_T("  UpdateKb(): updating non-sourcelanguage code to:  %s  ,  kbType %s"),
			pEditedKbStruct->targetLanguageCode.c_str(), kbTypeStr.c_str()); 
#endif
	}
	// convert it to string form
	wxJSONWriter writer; wxString str;
	writer.Write(jsonval, str);
	// convert it to utf-8 stored in CBString
	strVal = ToUtf8(str);

	aUrl = GetKBServerURL() + slash + container + slash + kbIDStr;
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
        wxLogDebug(_T("\n\n *** UpdateKb() Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif
		// The kind of error we are looking for isn't a CURLcode one, but a HTTP one 
		// (400 or higher)
		ExtractHttpStatusEtc(str_CURLbuffer, m_httpStatusCode, m_httpStatusText);

		curl_slist_free_all(headers);
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("UpdateKb() result code: %d Error: %s"), result, error.c_str());
			wxMessageBox(msg, _T("Error when trying to update a KB entry"), wxICON_EXCLAMATION | wxOK);
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

// This one is like RemoveUser() and RemoveKb(), except that we'll simplify even further
// and not bother to ask for any http headers to be returned. We'll just assume the
// deletion happened without error.
int KbServer::DeleteSingleKbEntry(int entryID)
{
	wxString entryIDStr;
	wxItoa(entryID, entryIDStr);
	CURL *curl;
	CURLcode result; // result code
	wxString slash(_T('/'));
	wxString colon(_T(':'));
	wxString container = _T("entry");
	wxString aUrl, aPwd;

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	aUrl = GetKBServerURL() + slash + container + slash + entryIDStr;
	charUrl = ToUtf8(aUrl);

		// prepare curl
	curl = curl_easy_init();

	result = (CURLcode)0; // initialize
	if (curl)
	{
		// set data
		curl_easy_setopt(curl, CURLOPT_URL, (char*)charUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		curl_easy_setopt(curl, CURLOPT_USERPWD, (char*)charUserpwd);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

		result = curl_easy_perform(curl);

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
	return (CURLcode)result;
}


int KbServer::RemoveUser(int userID)
{
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

	aUrl = GetKBServerURL() + slash + container + slash + userIDStr;
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

#if defined (_DEBUG)
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

	str_CURLbuffer.clear(); // use for headers return when there's no json to be returned

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = GetKBServerUsername() + colon + GetKBServerPassword();
	charUserpwd = ToUtf8(aPwd);

	aUrl = GetKBServerURL() + slash + container + slash + kbIDStr;
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

#if defined (_DEBUG)
        CBString s(str_CURLbuffer.c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("\n\n *** RemoveKb() Returned: %s    CURLcode %d"), showit.c_str(), (unsigned int)result);
#endif
		// The kind of error we are looking for isn't a CURLcode one, but a HTTP one 
		// (400 or higher)
		ExtractHttpStatusEtc(str_CURLbuffer, m_httpStatusCode, m_httpStatusText);

		curl_slist_free_all(headers);
		if (result) {
			wxString msg;
			CBString cbstr(curl_easy_strerror(result));
			wxString error(ToUtf16(cbstr));
			msg = msg.Format(_T("RemoveKb() result code: %d Error: %s"), result, error.c_str());
			wxMessageBox(msg, _T("Error when deleting a KB definition"), wxICON_EXCLAMATION | wxOK);
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

void KbServer::DoGetAll(bool bUpdateTimestampOnSuccess)
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
			// DoGetAll() to update the local KB. It we were doing the latter for the
			// first upload to the remote DB on kbserver, then the DoGetAll() returns a
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

/*  we probably won't use this for a bulk upload
void KbServer::UploadToKbServerThreaded()
{
	// Here's where I'll test doing this on a thread
	Thread_UploadToKBServer* pUploadToKBServerThread = new Thread_UploadToKBServer;
	pUploadToKBServerThread->m_pKbSvr = this;

	// now create the runnable thread with explicit stack size of 10KB
	wxThreadError error =  pUploadToKBServerThread->Create(1024);  // was wxThreadError error =  pUploadToKBServerThread->Create(10240);
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
#if defined(_DEBUG)
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
						// if the passed in flag is TRUE, then we accept only those
						// which are not in the hashmap; if false, we accept each one
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
								reference->translation = tgtPhrase;
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
#if defined(_DEBUG)
									// currently, "mi" <-> "I" is treated as an error as the
									// php does a caseless compare, and it conflicts with
									// exiting "mi" <-> "i", so exclude it; I've asked
									// Jonathan to fix this in the PHP code at kbserver
									if (srcPhrase == _T("mi") && tgtPhrase == _T("I"))
										continue;
#endif
									if (tgtPhrase == empty) // empty is _T("<empty>")
									{
										tgtPhrase.Empty();
									}
									reference = new KbServerEntry;
									reference->source = srcPhrase;
									reference->translation = tgtPhrase;
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
							reference->translation = pRefString->m_translation;
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
	size_t count = pKbSvr->m_arrSource.GetCount();
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
		if (tgt.IsEmpty())
		{
			tgt = empty;
		}
		// Find this key in the map; if not there, create on the heap another
		// wxArrayString, and store tgt as it's first item; if there, we can be certain no
		// other value has the same string, so just add it to the array
		iter = m_uploadsMap.find(src);
		if (iter == m_uploadsMap.end())
		{
			// this key is not yet in the map, add a new wxArrayString item using it and
			// store tgt within it
			pMyTranslations = new wxArrayString;
			m_uploadsMap[src] = pMyTranslations;
		}
		else
		{
			// this key is in the map, so we've got another translation string to
			// associate with it - add it to the array
			pMyTranslations = iter->second;
		}
		pMyTranslations->Add(tgt);
	}
	// The only reason we populate this map is because once the local KB gets large, say
	// over a thousand entries, it will be much quicker to determine a give src-tgt pair
	// from the local KB is not in the map, than to do the same check with an array or list
}


// The upload function - we do it by multiple threads (50 of them), each thread having
// approx 1/50th of the inventory of normal entries which qualify for uploading, as payload
// (we don't upload any pseudo-deleted ones). 
void KbServer::UploadToKbServer()
{
#if defined(_DEBUG)
	wxDateTime now = wxDateTime::Now();
	wxLogDebug(_T("UploadToKBServer() start time: %s\n"), now.Format(_T("%c"), wxDateTime::WET).c_str());
#endif
	if (m_pApp->m_bIsKBServerProject && this->IsKBSharingEnabled())
	{
		s_DoGetAllMutex.Lock();

		ClearAllPrivateStorageArrays();
		ClearAllStrCURLbuffers2(); // clears all 50 of the str_CURLbuff[] buffers

		// populate the 7 in-parallel arrays with the remote DB contents
		DoGetAll(FALSE); 
		int iTotalEntries = 0;	// initialize

		// If the remote DB has no content for this language pair as yet, all 7 arrays
		// will still be empty. Check for this and preserve the state in a boolean flag
		// for use below
		bool bRemoteDBContentDownloaded = TRUE; // initialize
		bRemoteDBContentDownloaded = !m_arrSource.IsEmpty();

 		ClearUploadsMap();

        // The remote DB has content, so our upload will need to be smart - it must
        // upload only entries which are not yet in the remote DB, and be mutex
        // protected (the access we are protecting is that within
        // ChangedSince_Queued(), called in OnIdle() - see MainFrm.cpp)
		KBAccessMutex.Lock();

		PopulateUploadList(this, bRemoteDBContentDownloaded);

		KBAccessMutex.Unlock();

		// We've no more use for the m_uploadsMap
		ClearUploadsMap();
		// The m_uploadsList needs no mutex, UploadToKbServer() is the only function which
		// uses it; it's now been populated -- if the remote DB has no content from this
		// project yet, the whole of the local KB's normal entries will be uploaded; but
		// if it has one or more entries, only those not in the remote DB will be uploaded
		// and so m_uploadsList will contain fewer, possibly much much fewer, entries for
		// uploading

		// We've finished using the 7 in-parallel arrays, so clear them 
		// and release the mutex
		ClearAllPrivateStorageArrays();
		s_DoGetAllMutex.Unlock();

		iTotalEntries = (int)m_uploadsList.GetCount(); // we use this below
#if defined(_DEBUG)
		wxLogDebug(_T("UploadToKbServer(), number of KbServerEntry structs =  %d"), iTotalEntries);
#endif
#if defined(_DEBUG)
		{
			UploadsList::iterator it;
			UploadsList::compatibility_iterator iter2;
			int anIndex = -1;
			for (it = m_uploadsList.begin(); it != m_uploadsList.end(); ++it)
			{
				anIndex++;
				iter2 = m_uploadsList.Item((size_t)anIndex);
				KbServerEntry* pEntry = iter2->GetData();
				wxString srcPhr = pEntry->source;
				wxString transPhr = pEntry->translation;
				wxLogDebug(_T("UploadToKbServer() %d. uploadable pair =  %s / %s"), 
					anIndex + 1, srcPhr.c_str(), transPhr.c_str());
			}
		}
#endif

		// Generate and fire off the 50 threads, fewer if the entry count is not large; we
		// will use 10 entries per thread, if there are <= 500 entries to send. If more
		// than that, we'll apportion however many there are between the max of 50 threads
		// we will send
		int min_per_thread = 10;
		int numThreadsNeeded = 1; // initialize
		int numEntriesPerThread = min_per_thread; // initialize
		if (iTotalEntries <= 500)
		{
			// carve up into <= 50 threads, with 10 entries each, except the last may have
			// fewer
			numThreadsNeeded = iTotalEntries / min_per_thread;
			if (iTotalEntries % min_per_thread > 0)
			{
				// add an extra thread for the remainder of the entries
				numThreadsNeeded++;
			}
		}
		else
		{
			// use 50 threads, put as many entries in each as we need to cover the total
			// which need to be sent
			numThreadsNeeded = 50;
			numEntriesPerThread = iTotalEntries / numThreadsNeeded;
			if (iTotalEntries % numThreadsNeeded)
			{
				// add an extra entry, due to the modulo calc
				numEntriesPerThread++;
			}
		}
		// The following are copies of parameters we need to upload in the curl calls, we
		// will pass a copy of each of these into each thread, so that the thread is
		// completely autonomous and no mutex is then needed; we'll get source and
		// translation strings from the kbServerEntry structs
		//KbServer*	pKbSvr = this;
		wxString	kbType; // set it with next line
		wxItoa(this->m_kbServerType, kbType);
		wxString	password = GetKBServerPassword();
		wxString	username = GetKBServerUsername();
		wxString	srcLangCode = GetSourceLanguageCode();
		wxString	translnLangCode; // set it with next test
		if (this->m_kbServerType == 1)
		{
			translnLangCode = GetTargetLanguageCode();
		}
		else
		{
			translnLangCode = GetGlossLanguageCode();
		}
		wxString	url = GetKBServerURL();
		wxString	source;
		wxString	transln; // either a target text translation, or a gloss
		wxString	jsonStr; // the wxString containing the JSON object written as a string
		CBString	jsonUtf8Str; // we'll convert jsonStr to UTF-8 and store it here, 
								 // ready for passing in to the bulk upload API function
		// Iterate across the m_uploadsList of KbServerEntry structs, which are to have
		// their src/tgt pairs uploaded in one or more threads. NumThreadsNeeded tells us
		// how many threads we need to create as we divide up the entries, max of 50, and
		// numEntriesPerThread is what we count off to form each subset of entries to be
		// bulk uploaded
		int entryIndex = -1;
		int threadIndex = -1;
		int entryCount = 0; // counting 1 to numEntriesPerThread for EACH thread
		KbServerEntry* pEntryStruct = NULL;
		Thread_UploadMulti* pThread = NULL;
		UploadsList::iterator listIter;
		UploadsList::compatibility_iterator anIter;
		wxThreadError error = wxTHREAD_NO_ERROR;
		wxJSONValue* jsonvalPtr = NULL;

		// Outer loop loops over all the KbServerEntry structs, to get src/transln pairs
		// (ie. either src/tgt pairs, or src/gloss pairs, depending on which kbType we are)
		for (listIter = m_uploadsList.begin(); listIter != m_uploadsList.end(); ++listIter)
		{
			++entryIndex;

			// Prepare to build a JSON object
			if (entryCount == 0)
			{
				jsonvalPtr = new wxJSONValue;
				threadIndex++;
			}
			++entryCount; // DO NOT put this line above the above entryCount == 0 test!

			// collect the entries for one bulk-uploading thread		
			// (this many: numEntriesPerThread)			

			// Get the KbServerEntry struct & extract the src and transln strings
			anIter = m_uploadsList.Item((size_t)entryIndex);
			pEntryStruct = anIter->GetData();
			source = pEntryStruct->source;
			transln = pEntryStruct->translation; // an adaptation, or gloss, 
												 // depending on kbserver type
			// Build the next array of the JSON object
			int i = entryCount - 1;
			(*jsonvalPtr)[i][_T("target")] = transln;
			(*jsonvalPtr)[i][_T("source")] = source;
			(*jsonvalPtr)[i][_T("type")] = kbType;
			(*jsonvalPtr)[i][_T("user")] = username;
			(*jsonvalPtr)[i][_T("deleted")] = (long)0; 
			(*jsonvalPtr)[i][_T("sourcelanguage")] = srcLangCode;
			(*jsonvalPtr)[i][_T("targetlanguage")] = translnLangCode;

			if ((entryCount == numEntriesPerThread) || (entryCount == iTotalEntries))
			{
				// We've collected all we need for this thread, OR, we have collected all
				// that remain for uploading when collecting for the last thread
				
				// Write out the JSON string form of this jsonval object, and then to 
				// UTF8, ready for passing in to the thread
				wxJSONWriter writer;
				writer.Write((*jsonvalPtr), jsonStr);
#if defined(_DEBUG)
				wxLogDebug(_T("Data to BulkUpload() for thread with index %d of total threads %d\n Data is....\n%s\n"),
					threadIndex, numThreadsNeeded, jsonStr.c_str());
#endif
				// convert it to utf-8 stored in CBString
				jsonUtf8Str = ToUtf8(jsonStr);

				// Make the thread and populate its members -- make its stack 4KB to 
				// be safe
				pThread = new Thread_UploadMulti;
				error = pThread->Create(4096); // play safe with stack sizes, give it 4kb
											   // (half that would probably still be safe)
				if (error != wxTHREAD_NO_ERROR)
				{
					// do something, we don't expect it to fail
					wxASSERT(FALSE);
				}
				else
				{
					// successful instantiation, now fill its members
					pThread->m_pKbSvr = this; 
					pThread->m_threadIndex = threadIndex;
					pThread->m_password = password;
					pThread->m_username = username;
					pThread->m_url = url;
					pThread->m_jsonUtf8Str = jsonUtf8Str;
				
					// run the thread
					error = pThread->Run(); // ignore error
					wxASSERT(error == wxTHREAD_NO_ERROR);

					// delete this JSON object from the heap & zero the entryCount ready
					// for the next thread's creation
					delete jsonvalPtr;
					entryCount = 0;

					// TODO complete the code here, prepare for next iteration
					// (at the moment, I think we are done)
				}

			} // end of TRUE block for test: if (entryCount == numEntriesPerThread)

		} // end of for loop:
		  // for (listIter = m_uploadsList.begin(); listIter != m_uploadsList.end(); ++listIter)

		DeleteUploadEntries();
		ClearAllStrCURLbuffers2(); // clears all 50 of the str_CURLbuff[] buffers

	} // end of TRUE block for test: if (m_pApp->m_bIsKBServerProject && this->IsKBSharingEnabled())

#if defined(_DEBUG)
	now = wxDateTime::Now();
	wxLogDebug(_T("UploadToKBServer() end time: %s\n"), now.Format(_T("%c"), wxDateTime::WET).c_str());
#endif
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
        // json array data begins with "[{", so test for the payload starting this way, if
        // it doesn't, then there is only an error string to grab -- quite possibly "No
        // matching entry found", in which case, don't do any of the json stuff, and the
        // value in m_kbServerLastTimestampReceived will be correct and can be used to
        // update the persistent storage file for the time of the lastsync
        wxString strStartJSON = _T("[{");
		CBString cbstr(str_CURLbuffer.c_str());
		wxString buffer(ToUtf16(cbstr));
		int offset = buffer.Find(strStartJSON);
		if (offset == 0) // TRUE means JSON data starts at the buffer's beginning
		{
            wxString myList = wxString::FromUTF8(str_CURLbuffer.c_str()); // I'm assuming no BOM gets inserted

            wxJSONValue jsonval;
            wxJSONReader reader;
            int numErrors = reader.Parse(myList, &jsonval);

            if (numErrors > 0)
            {
                // a non-localizable message will do, it's unlikely to ever be seen
                wxMessageBox(_T("ChangedSince_Queued(): json reader.Parse() failed. Unexpected bad data from server"),
                    _T("kbserver error"), wxICON_ERROR | wxOK);
                str_CURLbuffer.clear(); // always clear it before returning
                str_CURLheaders.clear(); // always clear it before returning
               return -1;
            }
            unsigned int listSize = jsonval.Size();
#if defined (_DEBUG)
            // get feedback about now many entries we got, in debug mode
            if (listSize > 0)
            {
                wxLogDebug(_T("ChangedSince_Queued() returned %d entries, for data added to kbserver since %s"),
                    listSize, timeStamp.c_str());
            }
#endif
            unsigned int index;
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
                // list what fields we extracted for each line of the entry table matched
				wxLogDebug(_T("Queued: Downloaded:  %s  ,  %s  ,  deleted = %d  ,  username = %s"),
                    pEntryStruct->source.c_str(), pEntryStruct->translation.c_str(),
					pEntryStruct->deleted, pEntryStruct->username.c_str());
#endif
            }

            str_CURLbuffer.clear(); // always clear it before returning
            str_CURLheaders.clear(); // BEW added 9Feb13

			// since all went successfully, update the lastsync timestamp
			UpdateLastSyncTimestamp();
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
                wxMessageBox(msg, _T("HTTP error"), wxICON_EXCLAMATION | wxOK);
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


/* params for BulkUpload() refactored
CBString		m_jsonUtf8Str;
wxString		m_password;
wxString		m_username;
wxString		m_url;
*/

int KbServer::BulkUpload(int threadIndex, // use for choosing which buffer to return results in
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

	// Need 50 of these if I'm to get results back without forcing sequentiality
	str_CURLbuff[threadIndex].clear(); // always make sure it is cleared for accepting new data

	CBString charUrl; // use for curl options
	CBString charUserpwd; // ditto

	aPwd = username + colon + password;
	charUserpwd = ToUtf8(aPwd);

#if defined(_DEBUG)
	// try do this without a mutex - it's temporary and I may get away with it, if not
	// delete it; same for the one at the end below
	wxDateTime now = wxDateTime::Now();
	wxLogDebug(_T("UploadToKBServer() thread %d , start time: %s\n"), 
		threadIndex, now.Format(_T("%c"), wxDateTime::WET).c_str());
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
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&threadIndex);
		// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		result = curl_easy_perform(curl);

#if defined (_DEBUG) // && defined (__WXGTK__)
        CBString s(str_CURLbuff[threadIndex].c_str());
        wxString showit = ToUtf16(s);
        wxLogDebug(_T("UploadToKbServer() thread %d , Returned: %s    CURLcode %d"), 
			threadIndex, showit.c_str(), (unsigned int)result);
#endif
		// The kind of error we are looking for isn't a CURLcode one, but aHTTP one 
		// (400 or higher)
		ExtractHttpStatusEtc(str_CURLbuff[threadIndex], m_httpStatusCode, m_httpStatusText);
		
		curl_slist_free_all(headers);
		str_CURLbuff[threadIndex].clear();

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

			curl_easy_cleanup(curl);
			return result;
		}
	}
	curl_easy_cleanup(curl);

#if defined(_DEBUG)
	now = wxDateTime::Now();
	wxLogDebug(_T("UploadToKBServer() thread %d , finish time: %s\n"), 
		threadIndex, now.Format(_T("%c"), wxDateTime::WET).c_str());
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

//=============================== end of KbServer class ============================

#endif // for _KBSERVER

