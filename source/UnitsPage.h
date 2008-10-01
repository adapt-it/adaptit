/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			UnitsPage.h
/// \author			Bill Martin
/// \date_created	18 August 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CUnitsPage class. 
/// The CUnitsPage class creates a wxPanel that allows the 
/// user to define the various document and knowledge base saving parameters. 
/// The panel becomes a "Units" tab of the EditPreferencesDlg.
/// The interface resources are loaded by means of the UnitsPageFunc()
/// function which was developed and is maintained by wxDesigner.
/// \derivation		The CUnitsPage class is derived from wxPanel.
/////////////////////////////////////////////////////////////////////////////

#ifndef UnitsPage_h
#define UnitsPage_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "UnitsPage.h"
#endif

/// The CUnitsPage class creates a wxPanel that allows the 
/// user to define the various document and knowledge base saving parameters. 
/// The panel becomes a "Units" tab of the EditPreferencesDlg.
/// The interface resources are loaded by means of the UnitsPageFunc()
/// function which was developed and is maintained by wxDesigner.
/// \derivation		The CUnitsPage class is derived from wxPanel.
class CUnitsPage : public wxPanel
{
public:
	CUnitsPage();
	CUnitsPage(wxWindow* parent); // constructor
	virtual ~CUnitsPage(void); // destructor // whm make all destructors virtual
	
	/// Creation
    bool Create( wxWindow* parent );

    /// Creates the controls and sizers
    void CreateControls();

	wxSizer* pUnitsPageSizer;
	bool tempUseInches;
	wxRadioButton* m_pRadioUseInches;
	wxRadioButton* m_pRadioUseCentimeters;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& WXUNUSED(event)); 
	
	void OnRadioUseInches(wxCommandEvent& WXUNUSED(event));
	void OnRadioUseCentimeters(wxCommandEvent& WXUNUSED(event));

private:
	// other class attributes

    DECLARE_DYNAMIC_CLASS( CUnitsPage )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* UnitsPage_h */
