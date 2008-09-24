/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			ViewPage.h
/// \author			Bill Martin
/// \date_created	17 August 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CViewPage class. 
/// The CViewPage class creates a wxPanel that allows the 
/// user to define the various view parameters. 
/// The panel becomes a "View" tab of the EditPreferencesDlg.
/// The interface resources are loaded by means of the ViewPageFunc()
/// function which was developed and is maintained by wxDesigner.
/// \derivation		The CViewPage class is derived from wxPanel.
/////////////////////////////////////////////////////////////////////////////

#ifndef ViewPage_h
#define ViewPage_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ViewPage.h"
#endif

/// The CViewPage class creates a wxPanel that allows the 
/// user to define the various view parameters. 
/// The panel becomes a "View" tab of the EditPreferencesDlg.
/// The interface resources are loaded by means of the ViewPageFunc()
/// function which was developed and is maintained by wxDesigner.
/// \derivation		The CViewPage class is derived from wxPanel.
class CViewPage : public wxPanel
{
public:
	CViewPage();
	CViewPage(wxWindow* parent); // constructor
	virtual ~CViewPage(void); // destructor // whm make all destructors virtual
	//enum { IDD = IDD_VIEW_PAGE };
   
	/// Creation
    bool Create( wxWindow* parent );

    /// Creates the controls and sizers
    void CreateControls();

	wxSizer* pViewPageSizer;

	// the following pointers to dialog controls are associated with the actual
	// controls created in Adapt_It_wdr.cpp (via wxDesigner)
	wxTextCtrl* m_pEditMaxSrcWordsDisplayed;
	wxTextCtrl*	m_pEditMinPrecContext;
	wxTextCtrl* m_pEditMinFollContext;
	wxTextCtrl* m_pEditLeading;
	wxTextCtrl* m_pEditGapWidth;
	wxTextCtrl* m_pEditLeftMargin;
	wxTextCtrl* m_pEditMultiplier;
	wxTextCtrl* m_pEditDlgFontSize;
	wxCheckBox* m_pCheckSupressFirst;
	wxCheckBox* m_pCheckSupressLast;
	wxCheckBox* m_pCheckWelcomeVisible;
	wxCheckBox* m_pCheckHighlightAutoInsertedTrans;
	int		tempMaxToDisplay;
	int		tempPrecCntxt;
	int		tempFollCntxt;
	int		tempLeading;
	int		tempGapWidth;
	int		tempLMargin;
	short	tempMultiplier;
	int		tempDlgFontSize;
	bool	tempSuppressFirst;
	bool	tempSuppressLast;
	bool	tempMakeWelcomeVisible;
	bool	tempUseStartupWizardOnLaunch;
	bool	tempHighlightAutoInsertions;
	wxColour tempAutoInsertionsHighlightColor;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& WXUNUSED(event)); 
	void OnButtonHighlightColor(wxCommandEvent& WXUNUSED(event));

private:
	// class attributes
	
	// other class attributes

    DECLARE_DYNAMIC_CLASS( CViewPage )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ViewPage_h */
