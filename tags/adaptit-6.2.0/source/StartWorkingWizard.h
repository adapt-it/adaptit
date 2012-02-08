/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			StartWorkingWizard.h
/// \author			Bill Martin
/// \date_created	17 November 2006
/// \date_revised	13 November 2011
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CStartWorkingWizard class. 
/// The CStartWorkingWizard class implements Adapt It's Start Working Wizard.
/// \derivation		The CStartWorkingWizard class is derived from wxScrollingWizard.
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
/// \derivation		The CStartWorkingWizard class is derived from wxScrollingWizard.
class CStartWorkingWizard : public wxScrollingWizard
{
public:
	CStartWorkingWizard(wxWindow* parent); // constructor
	virtual ~CStartWorkingWizard(void); // destructor
	// other methods
    wxWizardPage *GetFirstPage();
	wxSizer* pStartWorkingWizardSizer;
 	wxSizer* pWizardPageSizer;
	

protected:

private:

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* StartWorkingWizard_h */
