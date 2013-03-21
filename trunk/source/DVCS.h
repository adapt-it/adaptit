/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			DVCS.cpp
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
    
	int DoDVCS ( int action );		// all actual DVCS operations are done via this function

    bool AskSaveAndCommit (wxString blurb);       // dialog asking user if he/she wants to go ahead

private:		// instance variables
	CAdapt_ItApp*	m_pApp;
	wxString		m_user;
	wxString		git_command, git_options, git_arguments;
	wxArrayString	git_output;
	int				git_count, git_lineNumber;
    wxString        m_commit_comment;       // public because we set it from outside the class

protected:		// internal functions
    
	int  call_git ( bool bDisplayOutput );
	int  init_repository ();
    int  add_file ( wxString fileName );
	int  commit_file ( wxString fileName );
    int  setup_versions ( wxString fileName );
    int  get_version ( bool latest, wxString fileName );
	int  log_file ( wxString fileName );
	int  log_project();
};


//  Function to call the Save and Commit dialog:

bool AskSaveAndCommit();


#endif /* DVCS_h */
