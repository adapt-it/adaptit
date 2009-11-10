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
	bool	m_bReadOnlyAccess; // default FALSE, set TRUE if your running instances accesses
				// a remote project folder which is already "owned" by the remote user
				// (ownership is dynamically assigned to whoever accesses the project folder
				// first; a file with name ~AIROP-machinename-username.lock will be opened for
				// writing, but never written to, by whoever gets to have ownership; and is
				// closed for writing and removed when ownership of the project folder is
				// relinquished - that then gives whoever came second and so only got read-only
				// access the chance to re-open the project folder and so gain ownership)
				// AIROP has no deep significance other than providing a few ascii characters
				// to decrease the likelihood of an accidental name clash when searching for
				// the file within a project folder; AIROP means "Adapt It Read Only Protection"
	wxString	m_strLocalUsername; // whoever is the user running this instance
	wxString	m_strLocalMachinename; // name of the machine running this Adapt It instance
	wxString	m_strTheOtherUsername; // a holder for the Username parsed from the filename, and
				// set to empty string whenever the target project folder is owned by nobody
	wxString	m_strTheOtherMachinename; // a holder for the Machinename parsed from the filename,
				// and set to empty string whenever the target project folder is owned by nobody
	wxString	m_strAIROP_Prefix; // first part of the filename (see OnInit())
	wxString	m_strLock_Suffix; // the suffix to add to the filename's end (see OnInit())
	wxString	m_strReadOnlyProtectionFilename; // has the form ~AIROP*.lock where * will
						// be machinename followed by username, delimited by single hyphens
	wxString	m_strTheOtherReadOnlyProtectionFilename; // stores the one found in a targetted
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



	// member functions supporting Read-Only access (all public: at this stage)
public:
	void		Initialize();
	wxString	GetLocalUsername(); // return empty string if the local username isn't got
	wxString	GetLocalMachinename(); // return empty string if the local machinename isn't got
	wxString	ExtractUsername(wxString strFilename); // pass filename by value so we can play
													   // internally with impunity
	wxString	ExtractMachinename(wxString strFilename); // pass filename by value so we can play
													   // internally with impunity
	wxString	MakeReadOnlyProtectionFilename(
					const wxString prefix, // pass in m_strAIROP_Prefix
					const wxString suffix, // pass in m_strLock_Suffix
					const wxString machinename,
					const wxString username); // return str of form ~AIROP*.lock where * will
						// be machinename followed by username, delimited by single hyphens
	bool		IsDifferentUserOrMachine(
					wxString& localMachine,
					wxString& localUser,
					wxString& theOtherMachine,
					wxString& theOtherUser); // return TRUE if users or machines or both don't match,
						// return FALSE when both users and machines are the same (that is, FALSE
						// means the running instance making the test has ownership of the project
						// folder already, and so writing of KB and documents should not be prevented)
	wxString	GetReadOnlyProtectionFileInProjectFolder(wxString& projectFolderPath);

	DECLARE_DYNAMIC_CLASS(ReadOnlyProtection) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information
};

#endif // ReadOnlyProtection_h
