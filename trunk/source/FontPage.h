/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FontPage.h
/// \author			Bill Martin
/// \date_created	3 May 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CFontPageWiz and CFontPagePrefs classes.
/// A third class CFontPageCommon consists of the elements and methods in common to the above
/// two classes.
/// The CFontPageWiz and CFontPagePrefs classes create a panel that is used in both the 
/// Start Working wizard and the Edit Preferenced notebook dialog. The Wizard and Preferences
/// notebook, allow the user to choose/edit the three main fonts (source, target and navigation) 
/// for a project. The font panel also has buttons for selecting font colors for the source, 
/// target, navigation, special and retranslation text. The interface resources for the font
/// panel are defined in FontsPageFunc() which was developed and is maintained by wxDesigner.
/// The CFontPageWiz class is derived from wxWizardPage and the CFontPagePrefs class is derived
/// from wxPanel. This three-class design was implemented because wxNotebook under Linux/GTK will 
/// not display pages which are derived from wxWizardPage.
/// \derivation		CFontPageWiz is derived from wxWizardPage, CFontPagePrefs from wxPanel, and CFontPageCommon from wxPanel.
/////////////////////////////////////////////////////////////////////////////

#ifndef FontPage_h
#define FontPage_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "FontPage.h"
#endif

/// The CFontPageCommon class contains data and methods which are 
/// common to the CFontPageWiz and CFontPagePrefs classes.
/// \derivation The CFontPageCommon class is derived from wxPanel
class CFontPageCommon : public wxPanel
// CFontPageCommon needs to be derived from wxPanel in order to use built-in 
// functions like FindWindowById()
{
public:
	// The following variables are common to both CFontPageWiz and CFontPagePrefs
	wxSizer* pFontPageSizer;

	// pointers to the font dialog controls:
	wxTextCtrl* pSrcFontNameBox;
	wxTextCtrl* pTgtFontNameBox;
	wxTextCtrl* pNavFontNameBox;
	wxTextCtrl* pSrcFontSizeBox;
	wxTextCtrl* pTgtFontSizeBox;
	wxTextCtrl* pNavFontSizeBox;
	wxTextCtrl* pTextCtrlAsStaticFontpage;
	wxCheckBox* pSrcRTLCheckBox;
	wxCheckBox* pTgtRTLCheckBox;
	wxCheckBox* pNavRTLCheckBox;
	wxButton*	pSrcChangeEncodingBtn;
	wxButton*	pTgtChangeEncodingBtn;
	wxButton*	pNavChangeEncodingBtn;

	// font page attributes - these are temporary local attributes
	// used to store subdialog user choices until the OK button
	// on the main Preferences dialog is selected. If the OK button
	// on the main dialog is selected our OnOK routine will assign these
	// temporary attributes to the corresponding attributes on the App. 
	// If the user cancels at the main dialog, these temporary attributes
	// will be discarded without saving.
	wxString	tempSourceFontName;
	wxString	tempTargetFontName;
	wxString	tempNavTextFontName;
	int			tempSourceSize;
	int			tempTargetSize;
	int			tempNavTextSize;
	int			tempSourceFontStyle;
	int			tempTargetFontStyle;
	int			tempNavTextFontStyle;
	wxFontWeight	tempSourceFontWeight;
	wxFontWeight	tempTargetFontWeight;
	wxFontWeight	tempNavTextFontWeight;

	// Rather than having global temporary vars for the colors we are
	// using temp... forms local to each class to hold the values until
	// OnOk() or OnWizardPageChanging() sets them.
	wxColour	tempSourceColor;
	wxColour	tempTargetColor;
	wxColour	tempNavTextColor;
	wxColour	tempSpecialTextColor;
	wxColour	tempReTranslnTextColor;

	// the following bools not needed in wx version
	//bool		m_bNoSourceChange;
	//bool		m_bNoTargetChange;
	//bool		m_bNoNavTextChange;

	// wxFontData added for wxWidgets version holds font info
	// with get/set methods for font information such as
	// Get/SetColour(), Get/SetChosenFont() etc. A wxFontData
	// object is the last parameter in a wxFontDialog constructor
	// in order to interoperate with the dialog, receiving the
	// user's font choice data when the dialog is dismissed.
	// We use is mainly for GetChosenFont() and for Get/SetColour().
	wxFontData	tempSrcFontData;
	wxFontData	tempTgtFontData;
	wxFontData	tempNavFontData;

	// these "save" encoding values represent the font encodings upon entry 
	// to the font page (set in InitDialog):
	wxFontEncoding saveSrcFontEncoding;
	wxFontEncoding saveTgtFontEncoding;
	wxFontEncoding saveNavFontEncoding;

	// these "temp" encoding values start the same as the "save" values above
	// at fontPage InitDialog, but they will hold any changed encoding values
	// that the user may have made by calling the Set/View Encoding dialog on
	// one or more of the fonts. 
	// to the font page (set in InitDialog):
	wxFontEncoding tempSrcFontEncoding;
	wxFontEncoding tempTgtFontEncoding;
	wxFontEncoding tempNavFontEncoding;
	// The above variables are common to both CFontPageWiz and CFontPagePrefs

	// The following functions are common to both CFontPageWiz and CFontPagePrefs
	void DoSetDataAndPointers();
	void DoInit();

	void DoButtonNavTextColor(wxWindow* parent);
	void DoButtonRetranTextColor(wxWindow* parent);
	void DoButtonSourceTextColor(wxWindow* parent);
	void DoButtonSpecTextColor(wxWindow* parent);
	void DoButtonTargetTextColor(wxWindow* parent);
	void DoNavTextFontChangeBtn(wxWindow* parent);
	void DoSourceFontChangeBtn(wxWindow* parent);
	void DoTargetFontChangeBtn(wxWindow* parent);
	void DoButtonChangeSrcEncoding(wxWindow* parent);
	void DoButtonChangeTgtEncoding(wxWindow* parent);
	void DoButtonChangeNavEncoding(wxWindow* parent);
	// The above functions are common to both CFontPageWiz and CFontPagePrefs
//	DECLARE_DYNAMIC_CLASS( CFontPageCommon )

};

/// The CFontPageWiz class creates a wizard page for the Startup Wizard 
/// which allows the user to change the fonts settings used for the project during 
/// initial setup.
/// The interface resources for the page/panel are defined in FontsPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		CFontPageWiz is derived from wxWizardPage.
class CFontPageWiz : public wxWizardPage
{
public:
	CFontPageWiz();
	CFontPageWiz(wxWizard* parent); // constructor
	virtual ~CFontPageWiz(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD_FONT_PAGE };
   
	/// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

	/// an instance of the CFontPageCommon class for use in CFontPageWiz
	CFontPageCommon fontPgCommon;

public:
	// All the handlers need to be public since they can be called
	// from CEditPreferencesDlg.
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

	// implement wxWizardPage functions
	void OnWizardPageChanging(wxWizardEvent& event);
	void OnWizardCancel(wxWizardEvent& WXUNUSED(event));
    virtual wxWizardPage *GetPrev() const;
    virtual wxWizardPage *GetNext() const;

	void OnSourceFontChangeBtn(wxCommandEvent& WXUNUSED(event));
	void OnTargetFontChangeBtn(wxCommandEvent& WXUNUSED(event));
	void OnNavTextFontChangeBtn(wxCommandEvent& WXUNUSED(event));
	void OnButtonSourceTextColor(wxCommandEvent& WXUNUSED(event));
	void OnButtonTargetTextColor(wxCommandEvent& WXUNUSED(event));
	void OnButtonNavTextColor(wxCommandEvent& WXUNUSED(event));
	void OnButtonSpecTextColor(wxCommandEvent& WXUNUSED(event));
	void OnButtonRetranTextColor(wxCommandEvent& WXUNUSED(event));
	void OnButtonChangeSrcEncoding(wxCommandEvent& WXUNUSED(event));
	void OnButtonChangeTgtEncoding(wxCommandEvent& WXUNUSED(event));
	void OnButtonChangeNavEncoding(wxCommandEvent& WXUNUSED(event));

private:
	// class attributes
	
	// other class attributes

    DECLARE_DYNAMIC_CLASS( CFontPageWiz )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

/// The CFontPagePrefs class creates a page for the Fonts tab in the Edit Preferences property sheet
/// which allows the user to change the fonts used for the document and/or project. 
/// The interface resources for the page/panel are defined in FontsPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		CFontPagePrefs is derived from wxPanel.
class CFontPagePrefs : public wxPanel
{
public:
	CFontPagePrefs();
	CFontPagePrefs(wxWindow* parent); // constructor
	virtual ~CFontPagePrefs(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD_FONT_PAGE };
   
	/// Creation
    bool Create( wxWindow* parent );

    /// Creates the controls and sizers
    void CreateControls();

	/// an instance of the CFontPageCommon class for use in CFontPagePrefs
	CFontPageCommon fontPgCommon;

public:
	// All the handlers need to be public since they can be called
	// from CEditPreferencesDlg.
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

	// function unique to EditPreferencesDlg panel
	void OnOK(wxCommandEvent& WXUNUSED(event)); 
	 
	void OnSourceFontChangeBtn(wxCommandEvent& WXUNUSED(event));
	void OnTargetFontChangeBtn(wxCommandEvent& WXUNUSED(event));
	void OnNavTextFontChangeBtn(wxCommandEvent& WXUNUSED(event));
	void OnButtonSourceTextColor(wxCommandEvent& WXUNUSED(event));
	void OnButtonTargetTextColor(wxCommandEvent& WXUNUSED(event));
	void OnButtonNavTextColor(wxCommandEvent& WXUNUSED(event));
	void OnButtonSpecTextColor(wxCommandEvent& WXUNUSED(event));
	void OnButtonRetranTextColor(wxCommandEvent& WXUNUSED(event));
	void OnButtonChangeSrcEncoding(wxCommandEvent& WXUNUSED(event));
	void OnButtonChangeTgtEncoding(wxCommandEvent& WXUNUSED(event));
	void OnButtonChangeNavEncoding(wxCommandEvent& WXUNUSED(event));

protected:

private:
	// class attributes
	
	// other class attributes

    DECLARE_DYNAMIC_CLASS( CFontPagePrefs )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

#endif /* FontPage_h */
