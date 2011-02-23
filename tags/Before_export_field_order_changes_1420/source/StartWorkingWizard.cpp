/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			StartWorkingWizard.cpp
/// \author			Bill Martin
/// \date_created	17 November 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CStartWorkingWizard class. 
/// The CStartWorkingWizard class implements Adapt It's Start Working Wizard.
/// \derivation		The CStartWorkingWizard class is derived from wxWizard.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in StartWorkingWizard.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "StartWorkingWizard.h"
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
#include <wx/valgen.h> // for wxGenericValidator
#include <wx/wizard.h> // for wxWizard
#include <wx/display.h> // for wxDisplay
#include "Adapt_It.h"
#include "StartWorkingWizard.h"

#include "FontPage.h"
#include "LanguagesPage.h"
#include "UsfmFilterPage.h"
#include "PunctCorrespPage.h"
#include "CaseEquivPage.h"
#include "ProjectPage.h"
#include "DocPage.h" 

// This global is defined in Adapt_It.cpp.
//extern bool gbWizardNewProject;

/// This global is defined in Adapt_It.cpp.
extern CProjectPage* pProjectPage;

/// This global is defined in Adapt_It.cpp.
extern CLanguagesPage* pLanguagesPage;

/// This global is defined in Adapt_It.cpp.
extern CFontPageWiz* pFontPageWiz;

/// This global is defined in Adapt_It.cpp.
extern CPunctCorrespPageWiz* pPunctCorrespPageWiz;

/// This global is defined in Adapt_It.cpp.
extern CCaseEquivPageWiz* pCaseEquivPageWiz;

/// This global is defined in Adapt_It.cpp.
extern CUsfmFilterPageWiz* pUsfmFilterPageWiz;

/// This global is defined in Adapt_It.cpp.
extern CDocPage* pDocPage;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// This global is defined in Adapt_It.cpp.
extern CStartWorkingWizard* pStartWorkingWizard;

// event handler table
BEGIN_EVENT_TABLE(CStartWorkingWizard, wxWizard)
END_EVENT_TABLE()


CStartWorkingWizard::CStartWorkingWizard(wxWindow* parent) // dialog constructor
	: wxWizard(parent, wxID_ANY, _("Start Working"), wxNullBitmap,
				wxDefaultPosition, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// The Start Working Wizard is not generated in wxDesigner.
	
	// Create the wizard pages
    
	pStartWorkingWizard = this;
	// projectPage establishes its GetNext() page internally using either the gobal
	// docPage pointer or the languagesPage pointer depending on whether the user selects 
	// an existing project or <New Project>.
	pProjectPage = new CProjectPage(this);
	wxASSERT(pProjectPage != NULL);
	pLanguagesPage = new CLanguagesPage(this);
	wxASSERT(pLanguagesPage != NULL);
	pFontPageWiz = new CFontPageWiz(this);
	wxASSERT(pFontPageWiz != NULL);
	pPunctCorrespPageWiz = new CPunctCorrespPageWiz(this);
	wxASSERT(pPunctCorrespPageWiz != NULL);
	pCaseEquivPageWiz = new CCaseEquivPageWiz(this);
	wxASSERT(pCaseEquivPageWiz != NULL);
	pUsfmFilterPageWiz = new CUsfmFilterPageWiz(this);
	wxASSERT(pUsfmFilterPageWiz != NULL);
	// pDocPage establishes its GetPrev() page internally using either the gobal
	// pProjectPage pointer or the pFilterPage pointer depending on whether the user had 
	// previously selected an existing project or <New Project>.
	pDocPage = new CDocPage(this);
	wxASSERT(pDocPage != NULL);

	pWizardPageSizer = GetPageAreaSizer();
	pWizardPageSizer->Add(pProjectPage);
	pWizardPageSizer->Add(pLanguagesPage);
	pWizardPageSizer->Add(pFontPageWiz);
	pWizardPageSizer->Add(pPunctCorrespPageWiz);
	pWizardPageSizer->Add(pCaseEquivPageWiz);
	pWizardPageSizer->Add(pUsfmFilterPageWiz);
	pWizardPageSizer->Add(pDocPage);
	
	// Note: Each of the wizard page's 
	//InitDialog() handlers is NOT automatically 
	// called when the pages are created or shown in the Start Working Wizard. 
	// They must be called explicitly in program code to execute. We've made them
	// public functions in their class declarations so we can call them explicitly 
	// here in the App's DoStartWorkingWizard().
	// TODO: Add note here to same effect if this behavior is also true for the 
	// edit preferences dialog in the View's OnEditPreferencesDlg() function.
	//wxInitDialogEvent ievent = wxEVT_INIT_DIALOG;
	// Note: We must not call the pProjectPage's InitDialog here at the wizard
	// startup because it will call OnCloseProject if the project is already open.
	// Since the startup wizard can be called while a project is open, we want the
	// project to remain open and for the wizard to just present the docPage for
	// the user to select a different (or new) document within the same open project.
	// The pProjectPage's InitDialog() is called below just before RunWizard is
	// called with the projectPage as the starting page (when no project is open),
	// and also from within the pDocPage's OnWizardPageChanging's moving backward
	// block.
	//pProjectPage->InitDialog(ievent);
	// The other page's InitDialog() handlers can be called from here 
	// testing 25May07 commented out the other InitDialog calls also
	//pLanguagesPage->InitDialog(ievent);
	//pFontPageWiz->InitDialog(ievent);
	//pPunctCorrespPageWiz->InitDialog(ievent);
	//pCaseEquivPageWiz->InitDialog(ievent);
	//pUsfmPageWiz->InitDialog(ievent);
	//pFilterPageWiz->InitDialog(ievent);

	// Check if the Wizard is going to be too big. 
	// TODO: If necessary, make the taller pages such as the pPunctCorrespPageWiz 
	// and the pCaseEquivPageWiz scrollable if we are on a small screen.
	// Get the largest minimum page size needed for all pages of the wizard to display fully
	// (we don't really need to concern ourselves with the x components)
	wxSize neededSize;
	//neededSize.IncTo(pProjectPage->GetSize());
	//neededSize.IncTo(pLanguagesPage->GetSize());
	//neededSize.IncTo(pFontPageWiz->GetSize());
	//neededSize.IncTo(pPunctCorrespPageWiz->GetSize());
	//neededSize.IncTo(pCaseEquivPageWiz->GetSize());
	//neededSize.IncTo(pUsfmPageWiz->GetSize());
	//neededSize.IncTo(pFilterPageWiz->GetSize());
	//neededSize.IncTo(pDocPage->GetSize());
	// Note: Calling GetMinSize() on the pWizardPageSizer get the same value as the 8 lines of calling
	// IncTo above.
	neededSize = pWizardPageSizer->GetMinSize(); // GetMinSize is supposed to return the wizard's minimal client size
	// Check the display size to see if we need to make size adjustments in
	// the Wizard.
	// whm 31Aug10 added test below to validate results from wxDisplay after finding some problems with
	// an invalid index on a Linux machine that had dual monitors.
	int indexOfDisplay,numDisplays;
	numDisplays = wxDisplay::GetCount();
	indexOfDisplay = wxDisplay::GetFromWindow(this);
	if (numDisplays >= 1 && numDisplays <= 4 && indexOfDisplay != wxNOT_FOUND && indexOfDisplay >= 0 && indexOfDisplay <=3)
	{
		wxSize displaySize = wxDisplay(wxDisplay::GetFromWindow(this)).GetClientArea().GetSize();
		wxSize wizardSize = this->GetSize();
		wxSize wizardClientSize = this->GetClientSize();
		int wizFrameHeight = abs(wizardSize.GetY() - wizardClientSize.GetY());
		//int wizFrameWidth = abs(wizardSize.GetX() - wizardClientSize.GetX());
		if (neededSize.GetHeight() + wizFrameHeight > displaySize.GetHeight())
		{
			// We fit the prefs to the neededSize but it will be too big to fit in the display
			// window, so we will have to limit the size of the prefs dialog and possibly make the 
			// taller pages such as the pPunctCorrespPageWiz a scrolling pane.
			wxSize maxSz;
			maxSz.SetHeight(displaySize.GetHeight() - 50);
			this->SetMaxSize(maxSz);
		}
	}

	pWizardPageSizer->Layout();

	// Note: Since all the above wizard pages have wizard as parent window
	// they will be destroyed automatically when wizard is destroyed below.
}

CStartWorkingWizard::~CStartWorkingWizard() // destructor
{
	
}

wxWizardPage* CStartWorkingWizard::GetFirstPage() 
{
	wxInitDialogEvent ievent = wxEVT_INIT_DIALOG;
	if (gpApp->m_bKBReady && gpApp->m_pKB != NULL)
	{
		// KBs are loaded, so start with the docPage
		// insure docPage is initialized
		pDocPage->InitDialog(ievent);
		return pDocPage;
	}
	else
	{
		// KBs are not loaded so start with the projectPage
		// insure the projectPage is initialized
		pProjectPage->InitDialog(ievent);
		return pProjectPage;
	}
}
