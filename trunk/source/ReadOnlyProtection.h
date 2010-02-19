/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ReadOnlyProtection.h
/// \author			Bruce Waters
/// \date_created	10 November 2009
/// \date_revised	
/// \copyright		2009 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the header file for the ReadOnlyProtection class. 
/// The ReadOnlyProtection class protects a remote user from losing data
/// if someone, such as an administrator, accesses the same project folder
/// and does otherwise potentially data-losing operations such as opening, editing
/// and saving a document, or the KB, in that project. Whoever gets to the folder
/// first gets write permission, anyone else coming to it while the folder is still
/// open for work, gets only read-only access (any saves have their handlers entered
/// but a set flag, m_bReadOnlyAccess, will be TRUE and causes immediate exit of
/// those handlers)
/// document
/// \derivation		The ReadOnlyProtection class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef ReadOnlyProtection_h
#define ReadOnlyProtection_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ReadOnlyProtection.h"
#endif

// forward references:
class CAdapt_ItApp;

class ReadOnlyProtection : public wxObject  
{
public:
	// constructors
	ReadOnlyProtection();
	ReadOnlyProtection(CAdapt_ItApp* pApp);
	// don't need a copy constructor, there'll only be one instance
	// of this class, at m_pROP in CAdapt_ItApp instance

	// destructor
	virtual ~ReadOnlyProtection();

	// attributes
	CAdapt_ItApp* m_pApp; // pointer to the running application instance
private:
	wxString	m_strLocalUsername; // whoever is the user running this instance (hyphens removed)
	wxString	m_strLocalMachinename; // name of the machine running this Adapt It instance (hyphens removed)
	wxString	m_strLocalProcessID; // the number which is the running instance's PID converted to a wxString

	wxString	m_strOwningUsername; // a holder for the Username parsed from the filename, and
				// set to empty string whenever the target project folder is owned by nobody
	wxString	m_strOwningMachinename; // a holder for the Machinename parsed from the filename,
				// and set to empty string whenever the target project folder is owned by nobody
	wxString	m_strOwningProcessID; // the owning process's PID converted to a wxString

	wxString	m_strAIROP_Prefix; // first part of the filename (see OnInit())
	wxString	m_strLock_Suffix; // the suffix to add to the filename's end (see OnInit())
	wxString	m_strReadOnlyProtectionFilename; // has the form ~AIROP*.lock where * will
							// be machinename followed by username, delimited by single
							// hyphens (and hyphens have been filtered from the machinename
							// and username, if present on the owning machine and user names)
	wxString	m_strOwningReadOnlyProtectionFilename; // stores the one found in a targetted
						// project folder - possibly on a remote machine, but could be on the
						// local machine and so could be the same as what is in the member
						// m_strReadOnlyProtectionFilename; we must test to find out if same or not
				// The above first two names are obtained from the system using two getter functions
				// called GetLocalUsername() & GetLocalMachinename; and when a file specification
				// matching ~AIROP*.lock is found, a pair of getters called ExtractUserName() &
				// ExtractMachinename() return the Username and Machinename within the filename
				// and these are stored in the second pair of string members above. If either of
				// the Get... functions fail, we supply a default name - either "UnknownUser" or
				// "UnknownMachine", as required. (If a remote machine's Adapt It also fails in the
				// same way, this could lead to Adapt It thinking the remote user is myself - and
				// granting ownership leading to potential data loss, but this dual failure is
				// hopefully unlikely.)
				// Comparison is provided by a function called IsDifferentUserOrMachine() which 
				// takes 4 wxString arguments, and returns TRUE if user or machine or both are 
				// different, FALSE if the usernames and machinenames are the same
	wxString	m_strOSHostname; // whm added 17Feb10 will be "WIN", "LIN" or "MAC" depending on the 
					// host operating system. It is used as a component substring in the 
					// m_strReadOnlyProtectionFilename and used to determine what type of OS
					// created the ~AIROP*.lock file.
	bool		m_bOverrideROPGetWriteAccess; // whm added 18Feb10

	// member functions supporting Read-Only access
public:
	void		Initialize();
	bool		SetReadOnlyProtection(wxString& projectFolderPath); // call when entering project
					// and the returned bool value must be used to set (TRUE) or clear (FALSE) the 
					// app member m_bReadOnlyAccess
	bool		RemoveReadOnlyProtection(wxString& projectFolderPath); // call when leaving project
                    // and the returned bool value must always be used to clear to FALSE or
                    // set to TRUE the app member m_bReadOnlyAccess, according to whether
                    // the attempt at removal succeeded or not, respectively

	// whm moved IsItNotMe to public for use in the App's CheckLockFileOwnership()
	bool		IsItNotMe(wxString& projectFolderPath); // test if the user, machine, or 
                        // process which owns the write privilege to the project folder is
                        // different from me on my machine running my process which gained
                        // ownership of write privileges earlier
private:
	wxString	GetLocalUsername(); // return empty string if the local username isn't got
	wxString	GetLocalMachinename(); // return empty string if the local machinename isn't got
	wxString	GetLocalProcessID(); // return 0xFFFF if the PID fails to be got
	wxString	GetLocalOSHostname(); // whm added 17Feb10

	wxString	ExtractUsername(wxString strFilename); 
	wxString	ExtractMachinename(wxString strFilename); 
	wxString	ExtractProcessID(wxString strFilename);
	wxString	ExtractOSHostname(wxString strFilename); // whm added 17Feb10

	wxString	MakeReadOnlyProtectionFilename(
					const wxString prefix, // pass in m_strAIROP_Prefix
					const wxString suffix, // pass in m_strLock_Suffix
					const wxString machinename,
					const wxString username,
					const wxString processID,
					const wxString oshostname); // return str of form ~AIROP*.lock where * will
						// be machinename followed by username, followed by processID,
						// followed by oshostname, delimited by single hyphens
	bool		IsDifferentUserOrMachineOrProcess(
					wxString& localMachine,
					wxString& localUser,
					wxString& localProcessID,
					wxString& theOwningMachine,
					wxString& theOwningUser,
					wxString& theOwningProcessID); // return TRUE if users or machines or 
                        // or processes don't match; but return FALSE when all three are
                        // the same (that is, FALSE means the running instance making the
                        // test has ownership of the project folder already, and so writing
                        // of KB and documents should not be prevented)

	bool		IamRunningAnotherInstance(); // whm added 10Feb10  // currently unused
	bool		IamAccessingTheProjectRemotely(wxString& folderPath); // whm added 18Feb10

	bool		IOwnTheLock(wxString& projectFolderPath); // whm added 13Feb10
	bool		AnotherLocalProcessOwnsTheLock(wxString& ropFile); // whm added 13Feb10
	bool		ADifferentMachineMadeTheLock(wxString& ropFile); // whm added 13Feb10
	bool		WeAreBothWindowsProcesses(wxString& ropFile); // whm added 17Feb10

	wxString	GetReadOnlyProtectionFileInProjectFolder(wxString& projectFolderPath);
	bool		IsZombie(wxString& folderPath, wxString& ropFile); // return
						// TRUE if it is not open(it must be left over after an abnormal
						// machine shutdown or an app crash), or if my running process created
						// the open read-only protection file - in either case, my process can
						// remove it and I can become the owner for writing (again perhaps);
						// but returning FALSE means some other process has the folder open:
						// the other process could be another user's running AI, or even
						// myself opening AI again and accessing the same project
 
	bool		RemoveROPFile(wxString& folderPath, wxString& ropFile); // return TRUE
						// if removed okay, but if the return value is FALSE then another process
						// has the file open
	
	bool		IsTheProjectFolderOwnedForWriting(wxString& projectFolderPath); // TRUE if it
						// is owned, FALSE if no protection file is in the project folder
						// yet, or if there was such a protection file but it was a zombie
						// (ie. not open for writing, so was left open by some kind of failure)

	DECLARE_DYNAMIC_CLASS(ReadOnlyProtection) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information

};

#endif // ReadOnlyProtection_h
