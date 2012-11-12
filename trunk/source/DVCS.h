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

	int DoDVCS ( int action, int parm );		// all DVCS operations are done via this function

private:		// instance variables
	CAdapt_ItApp*	m_pApp;
	wxString		m_user;
	wxString		hg_command, hg_options, hg_arguments;
	wxArrayString	hg_output;
	int				hg_count, hg_lineNumber;



protected:		// internal functions

	int  call_hg ( bool bDisplayOutput );
	int  init_repository ();
	int  add_file (wxString fileName);
	int  add_all_files();
	int  remove_file (wxString fileName);
	int  remove_project();
	int  get_prev_revision ( bool bFirstTime, wxString fileName );
	bool  commit_valid();
	int  commit_file (wxString fileName);
	int  commit_project();
	int  log_file (wxString fileName);
	int  log_project();
	int  revert_to_revision ( int revision );

};


#endif /* DVCS_h */
