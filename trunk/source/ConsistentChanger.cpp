/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ConsistentChanger.cpp
/// \author			Bill Martin
/// \date_created	12 February 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL
/// \description	This is the implementation file for the CConsistentChanger class. 
/// The CConsistentChanger class has methods to manage the consistent change process
/// within Adapt It.
/// \derivation		CConsistentChanger is not a derived class.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ConsistentChanger.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ConsistentChanger.h"
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

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include "Adapt_It.h"
#include "ConsistentChanger.h"
#include "CCModule.h"
#include "BString.h"

wxString ccErrorStr;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// The CConsistentChanger class constructor. It is called whenever up to four CConsistentChanger 
/// objects (m_pConsistentChanger[4]) are created in the OnToolsDefineCC() handler in the App.
/// The CConsistentChanger class contains an embedded pointer to an instance of the CCCModule 
/// class which CConsistentChanger creates in this constructor.
////////////////////////////////////////////////////////////////////////////////////////////
CConsistentChanger::CConsistentChanger() // constructor
{
	ccErrorStr = _T(""); // whm added 19Jun07
	ccModule = new CCCModule;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \remarks
/// The CConsistentChanger class destructor. It is called whenever up to four CConsistentChanger 
/// objects (m_pConsistentChanger[4]) are destroyed/deleted in the App's OnExit() function.
/// The CConsistentChanger class contains an embedded pointer to an instance of the CCCModule 
/// class which CConsistentChanger deletes in this destructor.
////////////////////////////////////////////////////////////////////////////////////////////
CConsistentChanger::~CConsistentChanger() // destructor
{
	if (ccModule != NULL) // whm 11Jun12 added NULL test
		delete ccModule;
	ccModule = (CCCModule*)NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     a wxString which will contain the error message for any error occuring, 
///             otherwise a null string is returned
/// \param      tablePathName   -> the path to the consistent changes (.cct) file to be loaded
/// \remarks
/// Called from: The App's OnToolsDefineCC() menu event handler.
/// The loadTableFromFile() function calls ccModule->CCLoadTable() and 
/// ccModule->CCSetUTF8Encoding() to make the consistent changes facilities available to the
/// application.
////////////////////////////////////////////////////////////////////////////////////////////
wxString CConsistentChanger::loadTableFromFile(wxString tablePathName) // caller supplies a CString, so LPCTSTR needed here
{
	// The wx version of loadTableFromFile returns any error as a formatted error string to the caller.
	//---- LOAD THE CC TABLE INTO THE BUFFER
	// wx version: we can't use MFC specific functions; so we'll just try to open the file for reading
	// and consider it an error if f.Open() returns false.
	wxFile f;
	if (!f.Open(tablePathName,wxFile::read))
	{
		ccErrorStr = ccErrorStr.Format(_("The CC-Table \"%s\" could not be found or opened.\n"), tablePathName.c_str());
		return ccErrorStr;	
	}
	else
	{
		f.Close();
	}

	// BEW 15Oct07, a long path will crash the CCLoadTable() call, so I've built a function to copy the table
	// data to a temporary "_tbl_.cct" file at the CSIDL_PERSONAL folder, which is the My Documents folder or
	// the Documents folder in Vista, and I'll have subsequent code below access that instead, and removing the
	// temporary file when done. The effect of this is that the CC32.DLL never accesses a cct file buried so
	// deep in the folder hierachy that the DLL code fails due to its small internal buffer for the path
	wxString pathToPersonalFolder = CopyTableToPersonalFolder(tablePathName);
	wxString lpPath = pathToPersonalFolder; // use this one below, not tablePathName

	// we don't expect the table file copy to the personal folder to fail, but check for it just in case and if it
	// did (the returned pathInRoot wxString will be empty) then tell the user and abort the load operation
	if (pathToPersonalFolder.IsEmpty())
	{
		// for some reason the CopyTableToPersonalFolder() function failed, a hard coded English message will suffice
		wxString aPath = tablePathName;
		wxString s;
		s = s.Format(_T("Relocating the CC table file in loadTableFromFile failed, so cc processing is disabled; original table's path was: %s"),aPath.c_str());
		wxMessageBox(s,_T(""), wxICON_EXCLAMATION | wxOK);
		return s ; // MFC note: we really need to return BOOL -- add this change later
	}

	int iResult2;
	int iResult;

#ifdef _UNICODE

	// whm: one method of conversion is to use the .mb_str() method of wxString as illustrated in the
	// two commented out lines below:
	//wxCharBuffer tempBuff = lpPath.mb_str(wxConvUTF8);
	//CBString psz(tempBuff);
	// whm: an easier method of conversion is just to use the wxString constructor with wxConvUTF8 conversion parameter
	// whm 8Jun12 modified. The lpPath is already a wxString, so it doesn't need any further conversion with wxConfUTF8
	iResult = ccModule->CCLoadTable(lpPath); //iResult = ccModule->CCLoadTable(wxString(lpPath,wxConvUTF8));

	// make the environment enabled for UTF-8 support
	iResult2 = ccModule->CCSetUTF8Encoding(TRUE);

#else // ANSI

	iResult = ccModule->CCLoadTable(lpPath);

	// make the environment disabled for UTF-8 support
	iResult2 = ccModule->CCSetUTF8Encoding(FALSE);

#endif

	// get rid of the temporary file _tbt_.cct in the data folder
	if (!::wxRemoveFile(lpPath))
	{
		wxString aPath = tablePathName;
		wxString s;
		s = s.Format(_T("CopyTableToPersonalFolder() failed to remove the temporary file for path %s (so remove it manually using Win Explorer)"),aPath.c_str());
		wxMessageBox(s,_T(""), wxICON_EXCLAMATION | wxOK);
	}

	if(iResult)
	{
		// Instead of throwing Exceptions, the wx version of loadTableFromFile() returns the error result
		// returned by the CC function.
		ccErrorStr = ccErrorStr.Format(_("CC couldn't load the table: %s.  Got error code: %d.  Check the syntax of the table."),
							tablePathName.c_str(), iResult);
		return ccErrorStr; //throw ccErrorStr;
	}

	if(iResult2)
	{
		ccErrorStr = ccErrorStr.Format(_("CC environment did not accept CCSetUTR8Encoding() call: Got error code: %d."), 
							 iResult2);
		return ccErrorStr; //throw ccErrorStr;
	}

	// if we get here all was successful and ccErrorStr should still be empty
	return ccErrorStr;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     a copy of a wxString which has the path to the CCTable file copied 
///             to the CSIDL_PERSONAL folder and given a filename _tbt_.cct; or an 
///             empty string if there was an error
/// \param      pOriginalTable   -> the path+name of the original table selected
/// \remarks
/// Called from: loadTableFromFile().
/// CopyTableToPersonalFolder() was a temporary measure used to overcome the path length
/// limitation inherent in the old Consistent Changes code used in the cc32.dll. It functions
/// to copy the table to be used to a shorter path location (m_workFolderPath), thus bypassing
/// the path length limitation. From the original Description: This is a little helper 
/// function to get round a problem with the DLL implementation for CC32.DLL, which 
/// fails at the CCLoadTable call if the first parameter, which is a pointer to the 
/// pathname for the file, is a longish path (I don't know how long it has to be to 
/// fail, but paths of about 90 characters succeed, but 127 definitely didn't, and 
/// about 100 or more seems to cause a fail). It makes a copy of the file contents at 
/// the user profile's personal data folder (Linux and Mac will need tweaking - for 
/// the equivalent in Linux and Mac) and in a file with a short unique name hard 
/// coded as "_tbl_.cct" there, copy the CCT file to that location and have the 
/// loadTableFromFile() function use that copy for the CCLoadTable() call. 
/// Note: after the CCLoadTable() call is made, the copied table file has no further
/// function and the caller must delete it before loadTableFromFile() function exits.
/// Created 17Oct07.
////////////////////////////////////////////////////////////////////////////////////////////
wxString CConsistentChanger::CopyTableToPersonalFolder(wxString pOriginalTable)
{
	wxString path = gpApp->m_workFolderPath; // determined from earlier EnsureWorkFolderPresent() call
	wxASSERT(!gpApp->PathSeparator.IsEmpty()); // whm 11Jun12 added. GetChar(0) should not be called on an empty string
	int offset = path.Find(gpApp->PathSeparator.GetChar(0),TRUE); // TRUE is find from right end in wx
	path = path.Left(offset); // the path to the My Documents folder, or in Vista, the Documents folder
	wxString fname = _T("_tbl_.cct");
	path += gpApp->PathSeparator;
	path += fname; // this folder should always have write permission, even for a standard user profile

	// in wx we'll simply use ::wxCopyFile
	if (!::wxCopyFile(pOriginalTable,path))
	{
		wxString s;
		s = s.Format(_T("CopyTableToPersonalFolder() failed while copying to %s"),path.c_str());
		wxMessageBox(s,_T(""), wxICON_EXCLAMATION | wxOK);
		path.Empty();
		return path;
	}
	return path; // returns the path to the personal directory
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return     an int which contains the error code returned from CCProcessBuffer(); a non-zero
///             return value indicates an error occurred
/// \param      lpInputBuffer   -> the buffer containing the text to be processed
/// \param      nInBufLen       -> the length of the lpInputBuffer
/// \param      lpOutputBuffer  -> the buffer receiving the text after consistent changes processing
/// \param      npOutBufLen     -> the length of the lpOutputBuffer
/// \remarks
/// Called from: The View's DoConsistentChanges() function.
/// The utf8ProcessBuffer() function is just a wrapper that calls ccModule->CCProcessBuffer passing 
/// the same parameters to it. Note that processing is done as ANSI data in the ANSI version, and 
/// as Unicode data in the Unicode version, even though the name is utf8ProcessBuffer in either case.
////////////////////////////////////////////////////////////////////////////////////////////
int CConsistentChanger::utf8ProcessBuffer(char* lpInputBuffer,int nInBufLen,char* lpOutputBuffer,int* npOutBufLen)
{
	// BEW 15Oct07 changed to just do nothing if the environment failed to be set (the environment is the compiled
	// cc table file -- and since I now relocate it at the C:\ root, that may fail and so I don't want to crash here;
	// the returned -1 causes a small English message to come up each time the table is attempted to be used; and
	// the application is otherwise unaffected. This is probably a sufficient protocol, as the user can then
	// just unload the cc table at the menu, and/or retry (possibly after doing something to make his table okay)
	//if (m_hCCEnvironment == NULL)
	//	return -1;

	// put the cc processor pointer at the start of the table
	// whm note: CCProcessBuffer() actually calls CCReinitializeTable itself, so it is unnecessary to call it 
	// separately at this point as the MFC version does.
	//ccModule->CCReinitializeTable();

	int iResult = ccModule->CCProcessBuffer(lpInputBuffer,nInBufLen,lpOutputBuffer,npOutBufLen);
	return iResult;
}
