/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBServer.cpp
/// \author			Kevin Bradford, Bruce Waters
/// \date_created	26 September 20012
/// \date_revised	
/// \copyright		2012 Kevin Bradford, Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for functions involved in kbserver support. 
/// This .cpp file contains member functions of the CAdapt_ItApp class, and so is an
/// extension to the Adapt_It.cpp class.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "TargetUnit.h"
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

#include "Adapt_It.h"
#include "TargetUnit.h"
#include "AdaptitConstants.h"
#include "RefString.h"
#include "RefStringMetadata.h"
#include "helpers.h"

extern bool		gbIsGlossing;

#if defined(_KBSERVER)

// Call SetupForKBServer() when opening a project which has been designated as associating
// with a kbserver (ie. m_bKBServerProject is TRUE), or when the user, in the GUI,
// designates the current project as being a kb sharing one. Return TRUE for a successful
// setup, FALSE if something was not right and in that case don't perform a setup.
bool CAdapt_ItApp::SetupForKBServer()
{
	int aType = GetKBTypeForServer();
	if (aType == -1)
	{
		return FALSE;
	}
	m_kbTypeForServer = aType; // the value is good, either 1 or 2

	// Get the last sync datetime from where it is stored
	if (m_kbServerLastSync.IsEmpty())
	{
        // no kbserver exists before this date (my 65th birthday), so it will always result
        // in an all-records download once the connection is established
		m_kbServerLastSync = _T("2012-05-22 00:00:00"); 
	}
	else
	{
		// once there is a value in m_kbServerLastSync wxString, we'll get the latest and
		// put it where the kbserver client API functions can grab it
		m_kbServerLastSync = GetLastSyncDateTime();
		if (m_kbServerLastSync.IsEmpty())
		{
			// there was an error tying to get the lastsync datetime, a message has been
			// seen already
			return FALSE;
		}
		// if control gets to here, what's in m_kbServerLastSync should be a valid datetime
	}



	// TODO --- handle credentials.txt similarly to above; and test that m_sourceLanguageCode
	// and m_targetLanguageCode and m_glossesLanguageCode have values -- last can be
	// empty, but if so, warn user that kbserver support won't work in glossing mode if so





	return TRUE;
}

// Ff the parent function is misplaced, preceding LoadKB() and LoadGlossingKB() calls,
// then return -1 plus give the developer a message to fix this; otherwise return 1 for an
// adapting KB, 2 for a glossing KB.
int CAdapt_ItApp::GetKBTypeForServer()
{
	int type = 1; // default is to assume adapting KB is wanted

	// the two KBs must have been successfully loaded
	if (m_bKBReady && m_bGlossingKBReady)
	{
		if (gbIsGlossing)
		{
			type = 2;
		}
	}
	else
	{
		// warn developer that the logic is wrong
		wxString msg = _T("GetKBTypeForServer()called in SetupForKBServer(): Logic error, m_bKBReady and m_bGlossingKBReady are not both TRUE yet. Fix this!");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return -1;
	}
	return type;
}

// Return the dateTime value last returned from the kbserver and stored at the client;
// temporarily this storage is a file called lastsync.txt located in the project folder,
// but later something more permanent will be used (a hidden file for such metadata?)
wxString CAdapt_ItApp::GetLastSyncDateTime()
{
	wxString dateTimeStr; dateTimeStr.Empty();
	// temporary code starts
	wxString filename = _T("lastsync.txt");
	wxString path = m_curProjectPath + PathSeparator + filename;
	bool bLastSyncFileExists = ::wxFileExists(path);
	if (!bLastSyncFileExists)
	{
		// couldn't find lastsync.txt file in project folder
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
		wxString msg = _T("GetTextFileOpened()called in SetupForKBServer(): The wxTextFile, taking path to lastsync.txt, could not be opened");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		return dateTimeStr; // it's empty still
	}
	size_t numLines = f.GetLineCount();
	if (numLines == 0)
	{
		// warn developer that the wxTextFile is empty
		wxString msg = _T("GetTextFileOpened()called in SetupForKBServer(): The lastsync.txt file is empty");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
		f.Close();
		return dateTimeStr; // it's empty still
	}
	dateTimeStr = f.GetLine(0); // whew, finally, we have the lastsync datetime string
	f.Close();
	// end temporary code
	
// **** TODO ****  - the permanent GUI implementation for this goes here - comment out the above

	return dateTimeStr;
}

// Return FALSE if pf not successfully opened, in which case the parent does not need to
// close pf; otherwise, return TRUE for a successful open & parent must close it when done
bool CAdapt_ItApp::GetTextFileOpened(wxTextFile* pf, wxString& path)
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




// more functions...


#endif // for _KBSERVER

