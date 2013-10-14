/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ToolbarPage.h
/// \author			Erik Brommers
/// \date_created	8 March 2013
/// \rcs_id $Id:$
/// \copyright		2013 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CToolbarPagePrefs class.
/// This class creates a panel in the EditPreferencesDlg that allow the user 
/// to edit the toolbar display. The interface resources for CToolbarPagePrefs
/// are defined in ToolbarPageFunc() which was developed and is maintained by
/// wxDesigner.
/// \derivation		wxPanel.
/////////////////////////////////////////////////////////////////////////////

#ifndef ToolbarPage_h
#define ToolbarPage_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ToolbarPage.h"
#endif

#include "AdaptitConstants.h"
#include "Adapt_It.h"

/// The CToolbarPagePrefs class creates a page for the Punctuation tab in the Edit Preferences property sheet
/// which allows the user to change the punctuation correspondences used for the document and/or project. 
/// The interface resources for the page/panel are defined in ToolbarPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		CToolbarPagePrefs is derived from wxPanel.
class CToolbarPagePrefs : public wxPanel
{
public:
	wxSizer* pToolbarPageSizer;

	CToolbarPagePrefs();
	CToolbarPagePrefs(wxWindow* parent); // constructor
	virtual ~CToolbarPagePrefs(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD_DLG_PUNCT_MAP };
   
	/// Creation
    bool Create( wxWindow* parent );

    /// Creates the controls and sizers
    void CreateControls();

	// public methods
	// helper getter/setter for the wxListCtrl checkboxes
	void SetLstCheck(long lItem, bool bChecked);
	bool GetLstCheck(long lItem);

public:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event)); // needs to be public because it's called from the App

	// function unique to EditPreferencesDlg panel
	void OnOK(wxCommandEvent& WXUNUSED(event));

	// event handlers
	void OnRadioToolbarSmall(wxCommandEvent& event);
	void OnRadioToolbarMedium (wxCommandEvent& event);
	void OnRadioToolbarLarge (wxCommandEvent& event);
	void OnCboToolbarIcon (wxCommandEvent& event);
	void OnBnToolbarMinimal (wxCommandEvent& WXUNUSED(event));
	void OnBnToolbarReset (wxCommandEvent& WXUNUSED(event));
	void OnClickLstToolbarButtons(wxListEvent& event);

private:
	// class attributes
	ToolbarButtonSize m_toolbarSize;	// enum defined in adapt_it.h
	bool m_bShowToobarText;				// show icon and text, or just icon?
	bool m_bToolbarButtons[50];			// which buttons to display on the toolbar

	// private methods
	void PopulateList();

    DECLARE_DYNAMIC_CLASS( CToolbarPagePrefs )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

#endif /* ToolbarPage_h */
