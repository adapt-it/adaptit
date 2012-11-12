/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			StartWorkingWizard.h
/// \author			Bill Martin
/// \date_created	17 November 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CStartWorkingWizard class. 
/// The CStartWorkingWizard class implements Adapt It's Start Working Wizard.
/// \derivation		The CStartWorkingWizard class is derived from wxScrollingWizard 
/// when built with wxWidgets prior to version 2.9.x, but derived from wxWizard for 
/// version 2.9.x and later.
/////////////////////////////////////////////////////////////////////////////

#ifndef StartWorkingWizard_h
#define StartWorkingWizard_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "StartWorkingWizard.h"
#endif

class CProjectPage;
class CLanguagesPage;
//class CFontPage;
class CPunctCorrespPageWiz;
class CCaseEquivPage; 
class CDocPage;
class CUsfmFilterPageWiz;

/// The CStartWorkingWizard class implements Adapt It's Start Working Wizard.
/// \derivation		The CStartWorkingWizard class is derived from wxScrollingWizard 
/// when built with wxWidgets prior to version 2.9.x, but derived from wxWizard for 
/// version 2.9.x and later.

// whm 14Jun12 modified to use wxWizard for wxWidgets 2.9.x and later; wxScrollingWizard for pre-2.9.x
#if wxCHECK_VERSION(2,9,0)
class CStartWorkingWizard : public wxWizard
#else
class CStartWorkingWizard : public wxScrollingWizard
#endif
{
public:
	CStartWorkingWizard(wxWindow* parent); // constructor
	virtual ~CStartWorkingWizard(void); // destructor
	// other methods
    wxWizardPage *GetFirstPage();
	wxSizer* pStartWorkingWizardSizer;
 	wxSizer* pWizardPageSizer;
	void OnCancel(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);

protected:

private:

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* StartWorkingWizard_h */
