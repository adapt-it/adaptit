/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ReadOnlyProtection.cpp
/// \author			Bruce Waters
/// \date_created	10 November 2009
/// \date_revised	
/// \copyright		2009 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public 
///                 License (see license directory)
/// \description	Implementation file for read-only protection of a user
///					working in a project folder, and someone accesses that
///					folder remotely (such as an Administrator using the
///					Custom Work Folder Location command), and tries to do somee
///					work there which would, if there were no protection, potentially
///					lead to the user unknowingly losing data from either a document
///					or the KB, or both.
/// \derivation		The ReadOnlyProtection class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ReadOnlyProtection.h"
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
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/snglinst.h> // for wxSingleInstanceChecker
#include <wx/process.h> // for wxProcess::Exists(PID)
#include <wx/timer.h> // for wxTimer

#include "Adapt_It.h"
#include "helpers.h"
#include "ReadOnlyProtection.h"

// Define type safe pointer lists
//#include "wx/listimpl.cpp"

#define _DEBUG_ROP // comment out when wxLogDebug calls no longer needed

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(ReadOnlyProtection, wxObject)

ReadOnlyProtection::ReadOnlyProtection()
{
	m_pApp = &wxGetApp();
}

ReadOnlyProtection::ReadOnlyProtection(CAdapt_ItApp* pApp)  // use this one, it sets m_pLayout
{
	m_pApp = pApp;
}


ReadOnlyProtection::~ReadOnlyProtection()
{
	m_pApp->m_timer.Stop();
}

// implementation

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \remarks	Initialize the ReadOnlyProtection class. Specifically, for the local
/// running Adapt It instance, and local user, on the local machine, create a suitable
/// read-only protection file with .lock extension to be kept ready for use for as long as
/// this session lasts. Called from CAdapt_ItApp::OnInit() only. 
///////////////////////////////////////////////////////////////////////////////////////////
void ReadOnlyProtection::Initialize()
{
	m_strAIROP_Prefix = _T("~AIROP"); // first part of the filename
	m_strLock_Suffix = _T(".lock"); // the suffix to add to the filename's end

	// obtain the host machine's name, and the current user's id (ie. the name used
	// in path specifications);  set defaults if either or both can't be determined
	m_strLocalUsername = GetLocalUsername(); // hyphens have been filtered out
	if (m_strLocalUsername.IsEmpty())
	{
		m_strLocalUsername = _T("UnknownUser"); // not localizable, just need 'something'
	}
	m_strLocalMachinename = GetLocalMachinename(); // hyphens have been filtered out
	if (m_strLocalMachinename.IsEmpty())
	{
		m_strLocalMachinename = _T("UnknownMachine"); // ditto
	}
	m_strLocalProcessID = GetLocalProcessID();
	m_strOSHostname = GetLocalOSHostname();
	m_bOverrideROPGetWriteAccess = FALSE;	// default is FALSE, TRUE only if local user
											// decides to have write access over a project
											// that is currently locked by a remote user
											// where one or both users are on non-Windows
											// systems.
	// set up the filename for read only protection, which is for this particular user
	// and host machine and process ID; it remains unchanged for the session
	m_strReadOnlyProtectionFilename = MakeReadOnlyProtectionFilename(m_strAIROP_Prefix,
		m_strLock_Suffix, m_strLocalMachinename, m_strLocalUsername, m_strLocalProcessID,
		m_strOSHostname);

	// at launch, this one should be empty
	m_strOwningReadOnlyProtectionFilename.Empty();
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the current user id, or an empty string if it cannot be determined
/// \remarks	Obtain from wxWidgets "Network, user and OS functions" calls; if empty string
///				returned, the caller should set up a default string such as "UnknownUser"
/// BEW added 4Feb10: the filter function ChangeHyphensToUnderscores which ensures all machine
/// names and user names do not have any hyphens. (Then we can safely use hyphen for name
/// delimitation and safe parsing of the lock file's name.)
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::GetLocalUsername()
{
	wxString theName = ::wxGetUserId(); // returns empty string if not found
	if (!theName.IsEmpty())
	{
		// filter out of the name any hyphens
		theName = ChangeHyphensToUnderscores(theName);	
	}
	return theName;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the host machine's name, or an empty string if it cannot be determined
/// \remarks	Obtain from wxWidgets "Network, user and OS functions" calls; if empty string
///				returned, the caller should set up a default string such as "UnknownMachine"
/// BEW added 4Feb10: the filter function ChangeHyphensToUnderscores which ensures all machine
/// names and user names do not have any hyphens. (Then we can safely use hyphen for name
/// delimitation and safe parsing of the lock file's name.)
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::GetLocalMachinename()
{
	// get's the host machine's name (ignores domain name)
	wxString theMachine = ::wxGetHostName(); // returns empty string if not found
	if (!theMachine.IsEmpty())
	{
		// filter out of the name any hyphens
		theMachine = ChangeHyphensToUnderscores(theMachine);	
	}
	return theMachine;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the host machine's process ID for the running instance of Adapt It, or 
///             the string generated by converting 0xFFFF if there was an error
/// \remarks	Obtain from wxWidgets "Process control functions" calls
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::GetLocalProcessID()
{
	unsigned long pid = ::wxGetProcessId();
	if (pid == 0)
	{
		pid = 0xFFFF;
	}
	wxString pidValueStr = wxString::Format(_T("%i"),pid);
	return pidValueStr;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the host machine's OS name for the running instance of Adapt It
/// \remarks	Used in composing ~AIROP-machinename-username-processID-oshostname.lock file name
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::GetLocalOSHostname()
{
	wxString strHostOS;
#if defined(__WXMSW__)
	strHostOS = _("WIN");
#elif defined(__WXGTK__)
	strHostOS = _("LIN");
#elif defined(__WXMAC__)
	strHostOS = _("MAC");
#else
	strHostOS = _("UNK");
#endif
	return strHostOS;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the username part of "~AIROP-machinename-username-processID-oshostname.lock"
///	\param		strFilename		->	the read-only protect filename that was found
/// \remarks	extract the username string from the filename; pass filename by value so
///				we can play with the string internally with impunity
///				BEW note, 4Feb10, this use of hyphen as a delimiter for parsing is safe
///				because hyphens are filtered out of machinename and username strings
///				before thsy are used to construct the lock file's name
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::ExtractUsername(wxString strFilename)
{
	wxString theName = _T(""); // not localizable
	int offset = strFilename.Find(_T("-")); // first hyphen, machinename follows it
	wxASSERT(offset > 5);
	theName = strFilename.Mid(offset + 1);
	offset = theName.Find(_T("-")); // second hyphen, username follows it
	wxASSERT(offset > 0);
	theName = theName.Mid(offset + 1); // theName now starts with username
	offset = theName.Find(_T("-")); // finds the next hyphen
	wxASSERT(offset > 0);
	theName = theName.Left(offset);
	return theName;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the processID part of "~AIROP-machinename-username-processID-oshostname.lock"
///	\param		strFilename		->	the read-only protect filename that was found
/// \remarks	extract the process ID string from the filename; pass filename by value so
///				we can play with the string internally with impunity
///				BEW note, 4Feb10, this use of hyphen as a delimiter for parsing is safe
///				because hyphens are filtered out of machinename and username strings
///				before thsy are used to construct the lock file's name
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::ExtractProcessID(wxString strFilename)
{
	wxString theID = _T(""); // not localizable
	int offset = strFilename.Find(_T("-")); // first hyphen, machinename follows it
	wxASSERT(offset > 5);
	theID = strFilename.Mid(offset + 1);
	offset = theID.Find(_T("-")); // second hyphen, username follows it
	wxASSERT(offset > 0);
	theID = theID.Mid(offset + 1); // theID now starts with username
	offset = theID.Find(_T("-")); // third hyphen, processID follows it
	wxASSERT(offset > 0);
	theID = theID.Mid(offset + 1); // theID now starts with processID string
	offset = theID.Find(_T("-")); // fourth hyphen, oshostname follows it
	if (offset == -1)
	{
		// there is no oshostname, it was a lock file from pre-release version that didn't 
		// have an oshostname
		offset = theID.Find(m_strLock_Suffix); // finds the ".lock" string at end of theName
		wxASSERT(offset > 0);
		theID = theID.Left(offset);
	}
	else
	{
		theID = theID.Left(offset);
	}
	return theID;
}
///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the machinename part of "~AIROP-machinename-username-processID-oshostname.lock"
///	\param		strFilename		->	the read-only protect filename that was found
/// \remarks	extract the machinename string from the filename; pass filename by value so
///				we can play with the string internally with impunity
///				BEW note, 4Feb10, this use of hyphen as a delimiter for parsing is safe
///				because hyphens are filtered out of machinename and username strings
///				before thsy are used to construct the lock file's name
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::ExtractMachinename(wxString strFilename)
{
	wxString theMachine = _T(""); // not localizable
	int offset = strFilename.Find(_T("-")); // first hyphen, machinename follows it
	wxASSERT(offset > 5);
	theMachine = strFilename.Mid(offset + 1);
	offset = theMachine.Find(_T("-")); // second hyphen, username follows it
	wxASSERT(offset > 0);
	theMachine = theMachine.Left(offset);
	return theMachine;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the oshostname part of "~AIROP-machinename-username-processID-oshostname.lock"
///	\param		strFilename		->	the read-only protect filename that was found
/// \remarks	extract the oshostname string from the filename; pass filename by value so
///				we can play with the string internally with impunity
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::ExtractOSHostname(wxString strFilename) // whm added 17Feb10
{
	wxString theHostOS = _T(""); // not localizable
	int offset = strFilename.Find(_T("-")); // first hyphen, machinename follows it
	wxASSERT(offset > 5);
	theHostOS = strFilename.Mid(offset + 1);
	offset = theHostOS.Find(_T("-")); // second hyphen, username follows it
	wxASSERT(offset > 0);
	theHostOS = theHostOS.Mid(offset + 1); // theHostOS now starts with username
	offset = theHostOS.Find(_T("-")); // third hyphen, processID follows it
	wxASSERT(offset > 0);
	theHostOS = theHostOS.Mid(offset + 1); // theHostOS now starts with processID string
	offset = theHostOS.Find(_T("-")); // fourth hyphen, 
	if (offset == -1)
	{
		// it's a lock file from an earlier pre-release version, assume it is
		// a Windows system
		theHostOS = _T("WIN");
	}
	else
	{
		wxASSERT(offset > 0);
		theHostOS = theHostOS.Mid(offset + 1); // theHostOS now starts with oshostname string
		offset = theHostOS.Find(m_strLock_Suffix); // finds the ".lock" string at end of theName
		wxASSERT(offset > 0);
		theHostOS = theHostOS.Left(offset);
	}
	return theHostOS;
}


///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the composed filename as "~AIROP-machinename-username-processID-oshostname.lock"
///	\param		prefix		->	"~AIROP" always
/// \param		suffix		->	".lock" always
///	\param		machinename	->	local computer's name
/// \param		username	->	local user's name
/// \param		processID	->	local running instance's PID value
/// \param		oshostname	->	local running instance's os hostname
/// \remarks	compose the read-only protection filename, for the local machine & user
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::MakeReadOnlyProtectionFilename(
					const wxString prefix, // pass in m_strAIROP_Prefix
					const wxString suffix, // pass in m_strLock_Suffix
					const wxString machinename,
					const wxString username,
					const wxString processID,
					const wxString oshostname)
{
	wxString str = prefix;
	str += _T("-") + machinename;
	str += _T("-") + username;
	str += _T("-") + processID;
	str += _T("-") + oshostname;
	return str += suffix;
}


///////////////////////////////////////////////////////////////////////////////////////////
/// \return		return TRUE if users or machines or PIDs don't match; return FALSE when
///             all of users, machines and PIDs are the same (that is, FALSE means the
///             running instance making the test has ownership of the project folder
///             already, and so writing of KB and documents should not be prevented, also
///             return FALSE if noone currently owns the folder being checked
///	\param		localMachine	->	local computer's name
/// \param		localUser		->	local user's name
///	\param		theOtherMachine	->	the other computer's name (can be local machine)
/// \param		theOtherUser	->	the other user's name (can be the local user)
/// \param      theOwningProcessID -> the other machine's running instance's PID
/// \remarks	Test to determine whether or not the target folder is owned currently, and if
///				it is, then who owns it - whether myself, or someone else. Returning a FALSE
///				value tells the caller that I can safely take possession of the target folder
///				for writing
///////////////////////////////////////////////////////////////////////////////////////////
bool ReadOnlyProtection::IsDifferentUserOrMachineOrProcess(wxString& localMachine,
					wxString& localUser, wxString& localProcessID, wxString& theOwningMachine,
					wxString& theOwningUser, wxString& theOwningProcessID)
{
	// first check for nobody currently owning the folder - if that is the case
	// then return FALSE so that I can become its owner
	if (theOwningMachine.IsEmpty() || theOwningUser.IsEmpty() || theOwningProcessID.IsEmpty())
		return FALSE;
	// if we get here, someone is the owner of the target folder, so find out who
	if (localMachine != theOwningMachine)
		return TRUE;
	if (localUser != theOwningUser)
		return TRUE;
	if (localProcessID != theOwningProcessID)
		return TRUE;
    // if we get here, each is a match for the other, so I & my process already am the
    // owner of the target folder, so return FALSE
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		the absolute path to the read-only protection file in the passed in folder,
///				if it exists in the folder; otherwise, an empty string
///	\param		projectFolderPath ->	absolute path to the project folder being checked
/// \remarks	Search for a matching file in the project folder. wxRegEx class is not safe
///             to use because regular expression support for Unicode may not work - one
///             system does not support matching across character block boundaries,
///             another (eg. VIM) allows this but limits matches to 128 (enough for our
///             purposes but the wx documentation doesn't indicate if this is
///             supported). So we don't use regular expressions to obtain a match;
///             instead, we enumerate all files matching a file specification which
///             includes the prefix "~AIROP" and the suffix ".lock" and the presence of
///             a hyphen following the prefix. That much extremely unlikely to occur by
///             accident in an arbitrary filename from another unrelated source. There
///             should only be one returned. An empty string is returned if there was no
///             match made.
///////////////////////////////////////////////////////////////////////////////////////////
wxString ReadOnlyProtection::GetReadOnlyProtectionFileInProjectFolder(wxString& projectFolderPath)
{
	wxString theFilename = _T("");
	// test to make sure we are not trying to look at a file rather than a folder

	bool bIsReallyAFile = ::wxFileExists(projectFolderPath);
	if (bIsReallyAFile)
	{
		// this message should be localizable; we allow processing to continue, returning an 
		// empty string (the user should then retry to access a directory)
		wxString mssg;
		mssg = mssg.Format(
_("The project folder being tested, %s, is really a file. Adapt It will continue running, but you should next try to properly locate a project folder."),
projectFolderPath.c_str());
		wxMessageBox(mssg, _("Warning: Not a folder!"), wxICON_WARNING);
		return theFilename;
	}
	// sadly the following wxDir class function does not support a path which is a URI
	//bool bDirectoryExists = wxDir::Exists(projectFolderPath); // a static function
	bool bDirectoryExists = ::wxDirExists(projectFolderPath);
#ifdef _DEBUG_ROP
	wxLogDebug(_T("GetReadOnlyProtectionFileInProjectFolder: projectFolderPath %s ,  exists? = %s"), 
		projectFolderPath.c_str(), bDirectoryExists ? _T("TRUE") : _T("FALSE"));
#endif
	if (bDirectoryExists)
	{
		// sadly in the next call, wxDir does not support a URI, and .HasFiles() call
		// returns FALSE, but the dir.IsOpened() call works, so don't use .HasFiles()
		wxDir dir(projectFolderPath);
		bool bIsOpened = dir.IsOpened();
#ifdef _DEBUG_ROP
		wxLogDebug(_T("GetReadOnlyProtectionFileInProjectFolder:  dir.IsOpened()? = %s"), 
			bIsOpened ? _T("TRUE") : _T("FALSE"));
#endif
		if (!bIsOpened)
		{
			// it didn't open, this is unexpected and probably is something for the developer
			// to fix, so warn and abort
			wxString mssg;
			mssg = mssg.Format(
_T("GetReadOnlyProtectionFileInProjectFolder(): the directory %s failed to open for enumeration. Now aborting."), 
projectFolderPath.c_str());
			wxMessageBox(mssg, _T("wxDir() Err: directory not opened"), wxICON_ERROR);
			wxExit();
			return theFilename;
		}
		else
		{
			// it was opened okay; so return an empty string if there are no files within,
			// otherwise find (using wildcard *) the one and only read-only projection
			// file - if it exists; if found, return it to caller, it none is found, return
			// the empty string
			wxString aFile = _T("");  // store any read-only protection file we find, here

			// make the wildcarded file spec
			wxString theFileSpec = m_strAIROP_Prefix;
			theFileSpec += _T("-*");
			theFileSpec += m_strLock_Suffix;
            // use wxDir to enumerate all file in directory and look for the first which
            // begins with "~AIROP-" and ends with ".lock"; or better still, use
            // GetAllFiles() which is easier as it uses a string array & supports wildcards
			wxArrayString files;
			int numFiles = dir.GetAllFiles(projectFolderPath, &files, theFileSpec, wxDIR_FILES);
			if (numFiles > 0)
			{
				// a file matching ~AIROP-*.lock has been found, take the first such
				// (there should only be one anyway)
				wxString path = files.Item(0);
				wxFileName fn(path);
				aFile = fn.GetFullName();
			}
#ifdef _DEBUG_ROP
			wxLogDebug(_T("GetReadOnlyProtectionFileInProjectFolder: theFileSpec:  %s "),theFileSpec.c_str());
			wxLogDebug(_T("GetReadOnlyProtectionFileInProjectFolder: ::wxFindFirstFile returns:  %s [local PID = %s]"),
				aFile.IsEmpty() ? _T("Empty String") : aFile.c_str(),GetLocalProcessID().c_str());
#endif
			if (!aFile.IsEmpty())
			{
				theFilename = aFile;
			}
			else
			{
				// no match, so tell the caller so (return an empty string)
				return theFilename; // it's still the empty string
			}
		}
	}
	else
	{
		// the directory should exist - warn user. However, if setting up a custom folder location
		// and not going through with defining a project folder, then detect theFilename
		// being empty and silently return an empty string
		if (theFilename.IsEmpty())
			return theFilename;
		/*
		wxString mssg;
		mssg = mssg.Format(
_T("GetReadOnlyProtectionFileInProjectFolder(): the path, %s, to the passed in project folder does not exist! Now aborting."),
projectFolderPath.c_str());
		wxMessageBox(mssg, _T("wxDir() Err: the directory does not exist"), wxICON_ERROR);
		wxExit();
		*/
	}
	// populate the "owning" private members before returning
	m_strOwningMachinename = ExtractMachinename(theFilename);
	m_strOwningUsername = ExtractUsername(theFilename);
	m_strOwningProcessID = ExtractProcessID(theFilename);
	m_strOSHostname = ExtractOSHostname(theFilename);
	return theFilename;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the protection file (with form ~AIROP-*.lock, where * is
///             machinename-username-processID-oshostname) is a zombie lock file, left over 
///             when an instance of Adapt It died or power was lost either by the current 
///             user or another user on same or different machine on network. Return FALSE 
///             if the lock file is currently being used by Adapt It (see remakrs below on 
///             how we determine if the lock file is currently being used).
///	\param		projectFolderPath  ->  absolute path to the project folder being checked
///	\param      ropFile            ->  the read-only protection filename (see above)
/// \remarks
/// Called from: ReadOnlyProtection::IsTheProjectFolderOwnedForWriting().
/// A side effect of IsZombie is that any lock file (ropFile) located in the project 
/// folder is deleted if this function determines that it is a zombie file. This function
/// builds the path to the protection filename and attempts to determine if the lock file 
/// is a zombie by different means depending on whether the system that created the lock
/// file and the system of the current instance are both Windows systems or not. 
/// 
/// If both systems are Windows, we can use the simple test of attempting to remove the 
/// lock file; if removal was successful we know it was a zombie file, if not we know it 
/// is still owned by the Windows system that created it.
/// 
/// If one or both systems are non-Windows systems, we use a process of elimination, and
/// query the user if necessary. The process goes through the following steps: 
/// (1) Determine if the current instance of Adapt It owns the lock file. If it does the 
/// lock file is not a zombie - return FALSE;
/// (2) Determine if another local process owns the lock. If it does the lock file is not
/// a zombie - return FALSE;
/// (3) Determine if the lock file was created by a different computer accessing the 
/// project over the network. If so, we have no reliable way to know if the lock is a 
/// zombie other than asking the user if someone else on the network currently has 
/// ownership (is accessing the same project). We ask if the user wants to go ahead 
/// anyway and take ownership of the project folder. If YES, we remove the lock 
/// file and return TRUE. If NO, we don't touch the lock file and return FALSE.
/// In (3) there are potential data loss/corruption consequences if the user responds YES 
/// and forces the removal of the lock file even though another computer has ownership of 
/// the same project at the same time.
/// TODO: Add code to the remainder of the project that would handle the situation where
/// a the user mistakenly removes a lock file made by a remote instance of Adapt It that 
/// actually has current ownership of the folder (from elsewhere on the network). The 
/// implementation would need to set up a timer so that about every 5 seconds or so, the
/// remote instance (and only the remote instance) would poll the state of the lock file
/// it thinks it owns. If its ownership suddenly changes, then it needs to switch its
/// own instance of Adapt It into read-only mode, and immediately turn its window pink. 
/// The switch to read-only in this situation should only occur for the program instance 
/// which is non-local (i.e., when the path to its own lock file is a UNC type path). 
/// The timing interval for polling the state of the lock file (in the project folder of
/// the local instance) should be as frequent as possible, but not so frequent that it
/// negtatively impacts the app's performance while connected across the network.
///////////////////////////////////////////////////////////////////////////////////////////
bool ReadOnlyProtection::IsZombie(wxString& folderPath, wxString& ropFile)
{
	// IsZombie() revised by whm 13Feb10. The function originally used the technique of 
	// simply calling wxRemoveFile() on the lock file. If that call resulted in success
	// (returned true), it assumed that the removal of the lock file was sufficient proof
	// that the lock file was a "stale" left-over from a program crash or power 
	// interruption, and the project was actually "unowned" by some current local or 
	// remote process). Only if the wxRemoveFile() call failed (returned false), it 
	// assumed that another local or remote process owned the project folder. That 
	// technique, however, only works on Windows systems where the OS "protects" files 
	// open-for-writing by putting them in a "denial of access" state for any process 
	// that attempts to remove the file. On Linux and Mac systems, however, a file that
	// is open-for-writing can be successfully deleted by wxRemoveFile(). Therefore, we
	// cannot use the failure of wxRemoveFile() as a sufficient test for determining if
	// a lock file is stale (unowned). Instead, we have to take other positive steps to 
	// determine if the project folder is owned, and only when we are fairly certain that
	// no other local or remote process has ownership, we then consider the lock file to
	// be a zombie and call wxRemoveFile() to remove it.

	wxString pathToFile = folderPath + m_pApp->PathSeparator + ropFile;
	wxASSERT(::wxFileExists(pathToFile));

	if (WeAreBothWindowsProcesses(ropFile))
	{
		// When the instance that created the lock file and the local instance are/were both
		// Windows systems, we can use the wxRemoveFile() as the primary test for whether the 
		// lock file is a zombie or not.
		// On Windows systems the file can be removed only if it is a (closed) one, that is, a 
		// zombie left over from a crash or power loss (regardless of whoever was the former 
		// owner for writing)
		bool bRemoved = ::wxRemoveFile(pathToFile);
		if (bRemoved)
		{
			// we were able to remove it, so it's a zombie
			return TRUE;
		}
		else
		{
			// we could not remove it, so it is already opened by someone else's process, (even
			// the original one I initiated myself)
			return FALSE;
		}
	}
	else
	{
		// The instance that created the lock file and the local instance are/were diverse
		// operating systems (one or both of the systems are/were non-Windows). Therefore, 
		// we cannot use the same test we use in the if block above where both systems are 
		// Windows systems.
		if (IOwnTheLock(folderPath))
		{
			// I own the lock on the folder so it is not a zombie and
			// no additional tests are needed, just return FALSE.
	#ifdef _DEBUG_ROP
				wxLogDebug(_T("In IsZombie() at IOwnTheLock(folderPath): We already have ownership so return FALSE."));
	#endif
			return FALSE;
		}
		else if (AnotherLocalProcessOwnsTheLock(ropFile))
		{
			// Another instance of Adapt It owns the lock so it is not a zombie
			// and no additional tests are needed, just return FALSE.
	#ifdef _DEBUG_ROP
				wxLogDebug(_T("In IsZombie() at AnotherLocalProcessOwnsTheLock(): We can't take ownership so return FALSE."));
	#endif
			return FALSE;
		}
		else if (ADifferentMachineMadeTheLock(ropFile))
		{
			// Another machine on the network created the lock file.
			// We handle it differently depending on whether we are that remote computer or 
			// not. If folderPath has a URI, we are viewing the project folder remotely. When
			// that is the case we don't allow this remote instance to "take ownership".
			if (IamAccessingTheProjectRemotely(folderPath))
			{
				// The folderPath is a URI path (i.e., we are looking at a remote path on a
				// different computer on the network).
				// By design, we can't take ownership, so we consider the ropFile is not a zombie
				// and no additional tests are needed - return FALSE
	#ifdef _DEBUG_ROP
				wxLogDebug(_T("In IsZombie() at ADifferentMachineMadeTheLock(): The folderPath is a URI path. We can't take ownership so return FALSE."));
	#endif
				return FALSE;
			}
			else
			{
				// The folderPath is NOT a URI path, i.e., we NOT not looking at a remote path
				// but at a path on our own computer. 
				// We allow the user to verify whether we can take ownership. But only display this query
				// once
				m_pApp->m_timer.Stop();
				if (!m_bOverrideROPGetWriteAccess)
				{
					wxString message;
					message = _("Someone has your project folder open already, so you have READ-ONLY access.\nIf you need to be able to save your work, you can gain write access now.\nDoing so will force the other person to have read-only access.\n\nDo you to want to have write access now?");
					int response = wxMessageBox(message, _T("Another process owns write permission"), wxYES_NO | wxICON_WARNING);
					if (response == wxYES)
					{
						// User responded "YES", i.e., wants to have immediate write access so the
						// lock file is considered the same as a zombie. We do nothing here but 
						// set m_bOverrideROPGetWriteAccess to TRUE and allow control to fall 
						// through to the ::RemoveFile() block below.
		#ifdef _DEBUG_ROP
					wxLogDebug(_T("In IsZombie() at ADifferentMachineMadeTheLock() after YES response at Query: user wants immediate write access, lock file considered a zombie fall through to wxRemoveFile."));
		#endif
						m_bOverrideROPGetWriteAccess = TRUE;
					}
					else
					{
						// User cancelled or answered "NO" indicating that another computer 
						// owns the project and the lock file is not a zombie
		#ifdef _DEBUG_ROP
					wxLogDebug(_T("In IsZombie() at ADifferentMachineMadeTheLock() after YES (or Cancel) response at Query: user considers lock file genuine, return FALSE."));
		#endif
						m_bOverrideROPGetWriteAccess = FALSE;
						return FALSE; // 
					}
				}
			}
		}

		// If we get here the lock file is either not owned by anybody we care about, or else
		// the user wants immediate write access, so we consider it to be equivalent to a zombie 
		// file, we remove it and return TRUE
		bool bRemoved = ::wxRemoveFile(pathToFile);
		if (bRemoved)
		{
	#ifdef _DEBUG_ROP
			wxLogDebug(_T("In IsZombie after wxRemoveFile(pathToFile): bRemoved is TRUE, returning TRUE")); 
	#endif
			// we were able to remove it, so it's a zombie
			return TRUE;
		}
		else
		{
			// we don't expect this but it could happen on Windows if some other program 
			// has locked the file (denied access) in which case we have to return FALSE 
			// and consider it represents a real lock.
			// TODO: Add error message for user
	#ifdef _DEBUG_ROP
			wxLogDebug(_T("Warning: In IsZombie  after wxRemoveFile(pathToFile): bRemoved is unexpectedly FALSE, returning FALSE (need Error message?)")); 
	#endif
			return FALSE;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if a created protection file (with form ~AIROP-*.lock, where * is
///             machinename-username-processID) was removed, FALSE if it could not be
///             removed.
/// \param      projectFolderPath   ->  absolute path to the project folder which we are
///                                     attempting to find out whether it is currently 
///                                     owned for writing or not
/// \param      ropFile             ->  filename for the read-only protection file; see
///                                     other header comments for format - it's title
///                                     includes the name of the machine, user, the
///                                     running process's integer ID, and oshostname.
/// \remarks Does the work of removing a given read-only protection file (ROP file), but
/// first it checks to make sure it is entitled to do the removal. The ROP file has to be
/// owned my running Adapt It instance, and I must be the account holder, and the machine
/// mine, to qualify for removal. It the user, machine, or process is not mine, then the
/// file is left in place.
////////////////////////////////////////////////////////////////////////////////////////////
bool ReadOnlyProtection::RemoveROPFile(wxString& projectFolderPath, wxString& ropFile)
{
	wxString pathToFile = projectFolderPath + m_pApp->PathSeparator + ropFile;
	wxASSERT(::wxFileExists(pathToFile));

	// removal will only be possible if I on the localMachine in the process having
	// the localProcessID, became the owner in the first place; so we must test
	// that all the ducks line up for this running instance to be able to remove it,
	// and report back to the caller what the outcome was
	bool bRemoved = TRUE;
	bool bItsNotMe = IsItNotMe(projectFolderPath);
	if (bItsNotMe)
	{
		// not me, or not same process, so the file cannot be removed
		return FALSE;
	}
	else
	{
		// it's an opened protection file, and my process qualifies for closing it and
		// then deleting it; do so
		m_pApp->m_pROPwxFile->Close(); // ignore returned boolean, this shouldn't fail
		bRemoved = ::wxRemoveFile(pathToFile); // may fail
	}
	return bRemoved;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if user is running another instance of Adapt It locally
/// \remarks	Uses the wxSingleInstanceChecker class
///////////////////////////////////////////////////////////////////////////////////////////
bool ReadOnlyProtection::IamRunningAnotherInstance() // currently unused
{
	return m_pApp->m_pChecker->IsAnotherRunning();
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if user is accessing the project folder from a remote machine on the
/// network, FALSE otherwise
/// \param      folderPath   ->  absolute path to the project folder
/// \remarks	Tests if the projectFolderPath is a URI path. If so we know the user is
/// accessing the project folder from a remote machine on the network.
///////////////////////////////////////////////////////////////////////////////////////////
bool ReadOnlyProtection::IamAccessingTheProjectRemotely(wxString& folderPath)
{
	return m_pApp->IsURI(folderPath);
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the owning username, or owning machine, or owning process which owns
///             write permission, is different from my (local) username, machinename, and/or
///             running process; if FALSE is returned, then my process on my machine is the
///             owner of write permission
/// \param      projectFolderPath   ->  absolute path to the project folder
/// \remarks	Encapsulates the test for whether or not I and my process & machine is the
///             one that originally gained ownership of the project folder for writing
///////////////////////////////////////////////////////////////////////////////////////////
bool ReadOnlyProtection::IsItNotMe(wxString& projectFolderPath)
{
	m_strOwningReadOnlyProtectionFilename = 
							GetReadOnlyProtectionFileInProjectFolder(projectFolderPath);
	m_strOwningMachinename = ExtractMachinename(m_strOwningReadOnlyProtectionFilename);
	m_strOwningUsername = ExtractUsername(m_strOwningReadOnlyProtectionFilename);
	m_strOwningProcessID = ExtractProcessID(m_strOwningReadOnlyProtectionFilename);
	bool bItsNotMe = IsDifferentUserOrMachineOrProcess(m_strLocalMachinename,
						m_strLocalUsername, m_strLocalProcessID, m_strOwningMachinename, 
						m_strOwningUsername, m_strOwningProcessID);
	return bItsNotMe;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if user owns the lock, FALSE otherwise
/// \param      projectFolderPath   ->  absolute path to the project folder
/// \remarks Simply negates the result of the IsItNotMe() function
///////////////////////////////////////////////////////////////////////////////////////////
bool ReadOnlyProtection::IOwnTheLock(wxString& projectFolderPath)
{
	return !IsItNotMe(projectFolderPath);
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if another process on the local machine owns the lock, FALSE otherwise
/// \param      ropFile             ->  filename for the read-only protection file
/// \remarks Called from IsZombie() but only in cases where at least one non-Windows machine
/// is involved. This situation can only eventuate if an additional instance of Adapt It is
/// running on the same machine (might be a different user on a Linux system). The 
/// additional instance will have a different process id (PID).
///////////////////////////////////////////////////////////////////////////////////////////
bool ReadOnlyProtection::AnotherLocalProcessOwnsTheLock(wxString& ropFile)
{
	wxString currProcessStr;
	currProcessStr = GetLocalProcessID();
	wxString ropFileProcessStr;
	ropFileProcessStr = ExtractProcessID(ropFile);
	int nRopFilePID;
	nRopFilePID = wxAtoi(ropFileProcessStr);
	// We can use the wxSingleInstanceChecker method IsAnotherRunning() 
	// on m_pChecker. Or, we can check if the current PID and the
	// PID of the ropFile differ, AND if the PID of the ropFile currently exists 
	// as a process in the local system.
	if (IamRunningAnotherInstance()) //if (currProcessStr != ropFileProcessStr && wxProcess::Exists(nRopFilePID))
		return TRUE;
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if machinename of the lock differs from the local machinename, FALSE 
/// if the machinenames are identical.
/// \param      ropFile             ->  filename for the read-only protection file
///                                     it's title includes the name of the machine, user, 
///                                     running process's integer ID, and oshostname.
/// \remarks This simply determines if the .
///////////////////////////////////////////////////////////////////////////////////////////
bool ReadOnlyProtection::ADifferentMachineMadeTheLock(wxString& ropFile)
{
	// Test if machinenames differ. If so it was a remote machine that made the lock
	if (GetLocalMachinename() != ExtractMachinename(ropFile))
	{
		return TRUE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if oshostname of the lock is the same as the local oshostname, FALSE 
/// otherwise.
/// \param      ropFile             ->  filename for the read-only protection file
///                                     it's title includes the name of the machine, user, 
///                                     running process's integer ID, and oshostname.
/// \remarks Determines if the oshostname of the lock being examined is the same as the
/// local os host name. 
///////////////////////////////////////////////////////////////////////////////////////////
bool ReadOnlyProtection::WeAreBothWindowsProcesses(wxString& ropFile)
{
	// Test if oshostnames are the same. If so both AI instances are running on Windows
	if (GetLocalOSHostname() == ExtractOSHostname(ropFile))
	{
		return TRUE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if a created protection file (with form ~AIROP-*.lock, where * is
///             machinename-username-processID) is already present in the passed in
///             folder and is open for writing; if there is no such file in the folder
///             yet, or if there is such a file but it is not opened for writing (ie. a
///             zombie left over from a power down, or app crash), then return FALSE, and
///             if there was a zombie, automatically remove it as well.
/// \param      projectFolderPath   ->  absolute path to the project folder which we are
///                                     attempting to find out whether it is currently 
///                                     owned for writing or not
/// \remarks A private function which finds out whether or not the folder is owned for
/// writing by someone (whether me on my Adapt It instance, or someone else on some other
/// machine). A side-effect of detecting a zombie projection file is to automatically
/// remove it; only projection files currently open for writing are relevant for the
/// return value, and if one such is present.
//////////////////////////////////////////////////////////////////////////////////////////// 
bool ReadOnlyProtection::IsTheProjectFolderOwnedForWriting(wxString& projectFolderPath)
{
	m_strOwningReadOnlyProtectionFilename.Empty();
	m_strOwningUsername.Empty();
	m_strOwningMachinename.Empty();
	// get the file, if one exists in the folder, else get an empty string
	// (this call also populates the m_strOwningMachinename, m_strOwningUsername and
	// m_strOwningProcessID private strings based on the info in the protection filename)
	m_strOwningReadOnlyProtectionFilename = 
							GetReadOnlyProtectionFileInProjectFolder(projectFolderPath);
	if (m_strOwningReadOnlyProtectionFilename.IsEmpty())
	{
		// there is no read-only protection file in this project folder
#ifdef _DEBUG_ROP
		wxLogDebug(_T("In IsTheProjectFolderOwnedForWriting: There is no read-only protection file in project folder")); 
#endif
		return FALSE; // tell caller I now have qualified to own it for writing
	}
	else
	{
#ifdef _DEBUG_ROP
		wxLogDebug(_T("In IsTheProjectFolderOwnedForWriting: There is a read-only protection file in project folder")); 
#endif
		// there is a read-only protection file in this project, and so there
		// are two possibilities:
		// (1) it's a zombie left over after a crash or power loss - in which case we
		// can delete it because the project folder is not then owned by anyone, or
		// (2) someone else's process (or another one which I initiated) has the 
		//     ownership of write permission - and so we must return TRUE
		bool bIsZombie = IsZombie(projectFolderPath, m_strOwningReadOnlyProtectionFilename);
		if(bIsZombie)
		{
            // it was a zombie, and the test had the side effect of removing it...
            // so I can now become the owner for writing
			return FALSE;
		}
	}
	// write permission is currently owned by someone (it could be me though)
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if a created protection file (with form ~AIROP-*.lock, where * is
///             machinename-username-processID) is already present in the passed in
///             folder and is open for writing; if there is no such file in the folder
///             yet, or if the folder already is owned by me from my running process, then
///             return FALSE
///	\param		projectFolderPath  ->  absolute path to the project folder containing the
///	                                   read-only protection file which we are attempting
///	                                   to protect from having data lost by access by a
///	                                   remote running Adapt It instance
/// \remarks The return value always must be assigned to the flag
/// CAdapt_ItApp::m_bReadOnlyAccess. TRUE is returned if the folder is already owned by
/// someone else, FALSE is returned if the local machine succeeds in getting ownership of
/// the project folder for write purposes. If two instances of Adapt It access the same
/// projrect folder, the winner as far as getting ownership of writing permission is the
/// instance that enters the project first. Ownership is relinquished briefly while the
/// owning instance closes a document and then opens a different document in the same
/// project. This gives a remote instance a brief window of opportunity to get full
/// access, but the remote instance would have to either enter the project during that
/// brief window of opportunity, or open a document in the project during that window -
/// either possibility is unlikely unless people explicitly agree and time their actions
/// accordingly .
/// Call this function when the running instance enters a project project or opens or
/// creates a document
//////////////////////////////////////////////////////////////////////////////////////////// 
bool ReadOnlyProtection::SetReadOnlyProtection(wxString& projectFolderPath) 
{
	bool bIsOwned = IsTheProjectFolderOwnedForWriting(projectFolderPath);
#ifdef _DEBUG_ROP
	wxLogDebug(_T("\n **BEGIN** SetReadOnlyProtection:  Is the folder owned? bIsOwned = %s"), 
			bIsOwned ? _T("TRUE") : _T("FALSE"));
#endif
	if (bIsOwned)
	{
		// we need to distinguish me on my machine in my running process, from
		// another user/machine/or process (even if the process is a second one
		// which I initiated on this machine and am trying to access the same
		// project folder that my earlier process succeeded in getting access to)
		bool bItsNotMe = IsItNotMe(projectFolderPath);
#ifdef _DEBUG_ROP
		wxLogDebug(_T("SetReadOnlyProtection:  Is is NOT me?  bItsNotMe = %s"), 
				bItsNotMe ? _T("TRUE") : _T("FALSE"));
#endif
		if (bItsNotMe)
		{
			// we can call SetReadOnlyProtection() serially, eg. when entering a
			// project for the first time, and then each time a document is opened
			// or created in that project folder; so we don't want to have this
			// message shown more than once - so wrap it in a test so that once
			// in the read-only folder, the user doesn't see it again
			if (!m_pApp->m_bReadOnlyAccess && !m_bOverrideROPGetWriteAccess)
			{
				// I don't qualify to own this project folder, I can only have read-only
				// access (this message is localizable)
				// whm added 18Feb10. If another machinename created the lock and we are
				// trying to access the project folder on our own local machine, we can
				wxMessageBox(
_("Someone has your project folder open already, so you have READ-ONLY access."),_("Another process owns write permission"),
				wxICON_INFORMATION);
			}
			return TRUE; // return TRUE to app member m_bReadOnlyAccess
		}
		else
		{
            // it's me, in the same process which originally got ownership so just return
            // FALSE as I have full write permission already, and FALSE maintains that
#ifdef _DEBUG_ROP
			wxLogDebug(_T("SetReadOnlyProtection:  Its ME,  so returning FALSE"));
#endif
			if (m_pApp->IsURI(projectFolderPath))
			{
				m_pApp->m_timer.Start(2000);
			}
			else
			{
				m_pApp->m_timer.Stop();
			}
			return FALSE;  // return FALSE to app member m_bReadOnlyAccess
		}
	}
	else
	{
		// the project folder is not owned for writing - this covers the the zombie
		// case, and also when no running Adapt It instance has as yet entered this
		// project folder... so set my process up as its owner
		wxASSERT(!m_strReadOnlyProtectionFilename.IsEmpty()); // m_pROP->Initialize() 
            // should have made it already; likewise the members m_strLocalUsername &
            // m_strLocalMachinename and m_strLocalProcessID
		wxASSERT(!m_strLocalMachinename.IsEmpty());
		wxASSERT(!m_strLocalUsername.IsEmpty());
		wxASSERT(!m_strLocalProcessID.IsEmpty());

		m_strOwningMachinename = m_strLocalMachinename;
		m_strOwningUsername = m_strLocalUsername;
		m_strOwningProcessID = m_strLocalProcessID;
		m_strOwningReadOnlyProtectionFilename = m_strReadOnlyProtectionFilename;
		wxString readOnlyProtectionFilePath = projectFolderPath + m_pApp->PathSeparator + 
												m_strReadOnlyProtectionFilename;
		wxLogNull nolog; // avoid spurious messages from the system during Open() below
		bool bOpenOK; // whm added 4Feb10
		bOpenOK = m_pApp->m_pROPwxFile->Open(readOnlyProtectionFilePath,wxFile::write_excl);
#ifdef __WXDEBUG__ // whm added 4Feb10
		wxLogDebug(_T("m_pROPwxFile Open(readOnlyProtectionFilePath,wxFile::write_excl) was %u where 1=true and 0=false"),bOpenOK);
#endif
		wxASSERT(m_pApp->m_pROPwxFile->IsOpened()); // check it got opened
	}
#ifdef _DEBUG_ROP
	wxLogDebug(_T("SetReadOnlyProtection:  Nobody owns it yet,  so returning FALSE"));
#endif
	return FALSE; // return FALSE to app member m_bReadOnlyAccess
}

///////////////////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the protection file (with form ~AIROP-*.lock, where * is
///             machinename-username-processID) was successfully removed; else FALSE.
///	\param		projectFolderPath  ->  absolute path to the project folder containing the
///	                                   read-only protection file which we are attempting
///	                                   to remove
/// \remarks Removal can only be done by the process which currently has ownership of write
/// permission for the passed in project folder; a second process coming along later cannot
/// force the owner to relinquish ownership. The return value always be used for setting
/// or clearing pApp->m_bReadOnlyAccess; TRUE is returned if Removal was successful, FALSE
/// is returned if ownership is retained by someone else because this process does not qualify
/// for removing of protection.
/// Call this function when the running instance relinguishes the project
//////////////////////////////////////////////////////////////////////////////////////////// 
bool ReadOnlyProtection::RemoveReadOnlyProtection(wxString& projectFolderPath) 
{
	// get the current protection file, if there is one (this call tests for a zombie
	// as well, and if it finds one it deletes it & returns FALSE)
	bool bRemoved = FALSE;
	bool bOwned = IsTheProjectFolderOwnedForWriting(projectFolderPath);
	if (bOwned)
	{
		//  the following line effects the removal only if the protection file, when
		//  tested, turns out to be for me on my machine in the process which originally
		//  was successful in getting ownership of write privileges for this project folder
		bRemoved = RemoveROPFile(projectFolderPath, m_strOwningReadOnlyProtectionFilename);
		if (bRemoved)
		{
			m_strOwningMachinename.Empty();
			m_strOwningUsername.Empty();
			m_strOwningProcessID.Empty();
			m_strOwningReadOnlyProtectionFilename.Empty();
		}
	}
	else
	{
		// it's not owned, and this covers the zombie case too; return TRUE as these
		// circumstances is equivalent to having succeeded in removing ownership
		m_strOwningMachinename.Empty();
		m_strOwningUsername.Empty();
		m_strOwningProcessID.Empty();
		m_strOwningReadOnlyProtectionFilename.Empty();
		bRemoved = TRUE;
	}
	// whm added below 18Feb10
	m_bOverrideROPGetWriteAccess = FALSE;	// default is FALSE, TRUE only if local user
											// decides to have write access over a project
											// that is currently locked by a remote user
											// where one or both users are on non-Windows
											// systems.
	m_pApp->m_timer.Stop();
	return bRemoved; 
}
