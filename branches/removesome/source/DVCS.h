/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			DVCS.h
/// \author			Mike Hore
/// \date_created	22 December 2011
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public
///                 License (see license directory)
/// \description
/////////////////////////////////////////////////////////////////////////////


#ifndef DVCS_h
#define DVCS_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "DVCS.h"
#endif

/*	As of Oct 2012 we're trying to make the project as nicely OOP as we can, so although the
	DVCS stuff is very simple conceptually and doesn't need to maintain state between calls,
	we'll make it a class, which should only have one object.
*/

class DVCS : public wxObject
{
public:
	DVCS (void);		// constructor
	~DVCS (void);		// destructor
    
	int         DoDVCS ( int action, int parm );            // all actual DVCS operations are done via this function

    bool        AskSaveAndCommit (wxString blurb);          // dialog asking user if he/she wants to go ahead

    wxString        m_version_comment;                      // these 3 are public so the Nav dialog can get them
    wxString        m_version_date;
    wxString        m_version_committer;
    
    int             m_version_to_open;                      // used by our custom event to open a doc via the log dialog
    
private:		// instance variables
	CAdapt_ItApp*	m_pApp;
	wxString		m_user;
	wxString		git_command, git_options, git_arguments;
	wxArrayString	git_output;
	int				git_count;

protected:		// internal functions
    
	int  call_git ( bool bDisplayOutput );
	int  init_repository ();
    int  update_user_details ();
    int  add_file ( wxString fileName );
	int  commit_file ( wxString fileName );
    int  setup_versions ( wxString fileName );
    int  get_version ( int version_num, wxString fileName );
    int  any_diffs ( wxString fileName );
	int  log_file ( wxString fileName );
//	int  log_project();
};


//  Function to call the Save and Commit dialog:

bool AskSaveAndCommit();


#endif /* DVCS_h */
