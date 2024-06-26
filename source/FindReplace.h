/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FindReplace.h
/// \author			Bill Martin
/// \date_created	24 July 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for two classes: CFindDlg and CReplaceDlg.
/// These classes provide a find and/or find and replace dialog in which the user can find 
/// the location(s) of specified source and/or target text. The dialog has an "Ignore case" 
/// checkbox, a "Special Search" button, and other options. The replace dialog allows the 
/// user to specify a replacement string.
/// Both CFindDlg and CReplaceDlg are created as a Modeless dialogs. They are created on 
/// the heap and are displayed with Show(), not ShowModal().
/// \derivation		The CFindDlg and CReplaceDlg classes are derived from wxScrollingDialog 
/// when built with wxWidgets prior to version 2.9.x, but derived from wxDialog for version 
/// 2.9.x and later.
/////////////////////////////////////////////////////////////////////////////

#ifndef FindReplace_h
#define FindReplace_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "FindReplace.h"
#endif

/// Provides a find dialog in which the user can find 
/// the location(s) of specified source and/or target text. The dialog has an "Ignore case" 
/// checkbox, a "Special Search" button, and other options. 
/// CFindDlg is created as a Modeless dialog. It is created on 
/// the heap and is displayed with Show(), not ShowModal().
/// \derivation		The CFindDlg class is derived from wxScrollingDialog when built with wxWidgets
/// prior to version 2.9.x, but derived from wxDialog for version 2.9.x and later.
// whm 14Jun12 modified to use wxDialog for wxWidgets 2.9.x and later; wxScrollingDialog for pre-2.9.x
#if wxCHECK_VERSION(2,9,0)
class CFindDlg : public wxDialog // use wxScrollingDialog instead of AIModalDialog because we use wxUpdateUIEvent
#else
class CFindDlg : public wxScrollingDialog // use wxScrollingDialog instead of AIModalDialog because we use wxUpdateUIEvent
#endif
{
public:
	CFindDlg();
	CFindDlg(wxWindow* parent); // constructor
	virtual ~CFindDlg(void); // destructor // whm make all destructors virtual

	/// Creation
    bool Create( wxWindow* parent );

    /// Creates the controls and sizers
    void CreateControls();
	
	// whm: wx version has pointers for all controls that need reference elsewhere
	// These pointers are determined only in the CFindDlg constructor after call to
	// FindDlgFunc().
	wxRadioButton*	m_pRadioSrcTextOnly;
	wxRadioButton*	m_pRadioTransTextOnly;
	wxRadioButton*	m_pRadioBothSrcAndTransText;
	wxCheckBox*		m_pCheckIgnoreCase;
	wxButton*		m_pFindNext;

	wxStaticText*	m_pStaticSrcBoxLabel;
	wxTextCtrl*		m_pEditSrc;
	wxStaticText*	m_pStaticTgtBoxLabel;
	wxTextCtrl*		m_pEditTgt;
	wxButton*		m_pButtonSpecialNormal;
	wxCheckBox*		m_pCheckIncludePunct;
	wxCheckBox*		m_pCheckSpanSrcPhrases;

	wxStaticText*	m_pSpecialSearches;
	wxStaticText*	m_pSelectAnSfm;
	wxRadioButton*	m_pFindRetranslation;
	wxRadioButton*	m_pFindPlaceholder;
	wxRadioButton*	m_pFindSFM;
	wxComboBox*		m_pComboSFM; 

	int m_marker; // index of chosen marker in listed markers
	wxString m_srcStr;
	wxString m_tgtStr;
	bool m_bIncludePunct;
	bool m_bSpanSrcPhrases;
	bool m_bIgnoreCase;
	bool m_bFindDlg;
	bool m_bSrcOnly;
	bool m_bTgtOnly;
	bool m_bSrcAndTgt;
	wxString m_sfm; // the standard format marker to be used is stored here
	bool m_bFindRetranslation; // this one, for Special Search finding of retranslations
	bool m_bFindRetransln; // this one for Normal mode, must not use just one for both modes
	bool m_bFindNullSrcPhrase;
	bool m_bFindSFM;
	bool m_bSpecialSearch;
	wxString m_replaceStr;

	wxString m_markerStr;
	// hooks for placement of dialog on the screen (so as not to obscure the selection)
	wxPoint	m_ptBoxTopLeft;
	int m_nTwoLineDepth;
	// count of how many srcPhrase instances were matched (value not valid when no match)
	int m_nCount;
	bool m_bSelectionExists;

    // whm 15May2020 added
    bool m_bSearchFromDocBeginning;

	wxSizer* pFindDlgSizer;
	
	void DoFindNext();
	void DoRadioSrcOnly();
	void DoRadioTgtOnly();
	void DoRadioSrcAndTgt();

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	
	// these are unique to CFindDlg
	void OnSelchangeComboSfm(wxCommandEvent& WXUNUSED(event));
	void OnButtonSpecial(wxCommandEvent& WXUNUSED(event));
	void OnRadioSfm(wxCommandEvent& event);
	void OnRadioRetranslation(wxCommandEvent& WXUNUSED(event));
	void OnRadioNullSrcPhrase(wxCommandEvent& WXUNUSED(event));
	virtual void OnCancel(wxCommandEvent& WXUNUSED(event));

    // whm 15May2020 added
    void OnSrcEditBoxEnterKeyPress(wxCommandEvent& WXUNUSED(event));
    void OnTgtEditBoxEnterKeyPress(wxCommandEvent& WXUNUSED(event));

	void OnFindNext(wxCommandEvent& WXUNUSED(event));
	void OnRestoreFindDefaults(wxCommandEvent& WXUNUSED(event));
public:
	void OnRadioSrcOnly(wxCommandEvent& WXUNUSED(event));
	void OnRadioTgtOnly(wxCommandEvent& WXUNUSED(event));
	void OnRadioSrcAndTgt(wxCommandEvent& WXUNUSED(event));
	
private:
    DECLARE_DYNAMIC_CLASS( CFindDlg )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

/// Provides a find and replace dialog in which the user can find 
/// the location(s) of specified source and/or target text. The dialog has an "Ignore case" 
/// checkbox, a "Special Search" button, and other options. The replace dialog allows the 
/// user to specify a replacement string.
/// CReplaceDlg is created as a Modeless dialogs. It is created on 
/// the heap and is displayed with Show(), not ShowModal().
/// \derivation		The CReplaceDlg class is derived from wxScrollingDialog when built 
/// with wxWidgets prior to version 2.9.x, but derived from wxDialog for version 2.9.x 
/// and later.
// whm 14Jun12 modified to use wxDialog for wxWidgets 2.9.x and later; wxScrollingDialog for pre-2.9.x
#if wxCHECK_VERSION(2,9,0)
class CReplaceDlg : public wxDialog // use wxScrollingDialog instead of AIModalDialog because we use wxUpdateUIEvent
#else
class CReplaceDlg : public wxScrollingDialog // use wxScrollingDialog instead of AIModalDialog because we use wxUpdateUIEvent
#endif
{
public:
	CReplaceDlg();
	CReplaceDlg(wxWindow* parent); // constructor
	virtual ~CReplaceDlg(void); // destructor // whm make all destructors virtual

	/// Creation
    bool Create( wxWindow* parent );

    /// Creates the controls and sizers
    void CreateControls();
	
	// whm: wx version has pointers for all controls that need reference elsewhere
	// These pointers are determined only in the CReplaceDlg constructor after call to
	// ReplaceDlgFunc().
	wxRadioButton*	m_pRadioSrcTextOnly;
	wxRadioButton*	m_pRadioTransTextOnly;
	wxRadioButton*	m_pRadioBothSrcAndTransText;
	wxCheckBox*		m_pCheckIgnoreCase;
	wxButton*		m_pFindNext;

	wxStaticText*	m_pStaticSrcBoxLabel;
	wxTextCtrl*		m_pEditSrc_Rep;
	wxStaticText*	m_pStaticTgtBoxLabel;
	wxTextCtrl*		m_pEditTgt_Rep;
	wxButton*		m_pButtonReplace;
	wxButton*		m_pButtonReplaceAll;
	wxCheckBox*		m_pCheckIncludePunct;
	wxCheckBox*		m_pCheckSpanSrcPhrases;

	wxTextCtrl*		m_pEditReplace;

	wxString m_srcStr;
	wxString m_tgtStr;
	bool m_bIncludePunct;
	bool m_bSpanSrcPhrases;
	bool m_bIgnoreCase;
	bool m_bFindDlg;
	bool m_bReplaceDlg; // BEW added 3Apr21
	bool m_bSrcOnly;
	bool m_bTgtOnly;
	bool m_bSrcAndTgt;
	wxString m_sfm; // the standard format marker to be used is stored here
	wxString m_replaceStr;

	wxString m_markerStr;
	// hooks for placement of dialog on the screen (so as not to obscure the selection)
	wxPoint	m_ptBoxTopLeft;
	int m_nTwoLineDepth;
	// count of how many srcPhrase instances were matched (value not valid when no match)
	int m_nCount;
	bool m_bSelectionExists;

    // whm 15May2020 added
    bool m_bSearchFromDocBeginning;

    wxSizer* pReplaceDlgSizer;

	bool OnePassReplace(); // do a single Find Next & Replace, provided the Find Next succeeded

	void DoFindNext();
	void DoRadioSrcOnly();
	void DoRadioTgtOnly();
	void DoRadioSrcAndTgt();

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	// these are unique to CReplaceDlg
	void OnReplaceButton(wxCommandEvent& event);
	void OnReplaceAllButton(wxCommandEvent& event);
	virtual void OnCancel(wxCommandEvent& WXUNUSED(event));
	
    // whm 15May2020 added
    void OnSrcEditBoxEnterKeyPress(wxCommandEvent& WXUNUSED(event));
    void OnTgtEditBoxEnterKeyPress(wxCommandEvent& WXUNUSED(event));
    void OnReplaceEditBoxEnterKeyPress(wxCommandEvent& WXUNUSED(event));
    
    void OnFindNext(wxCommandEvent& WXUNUSED(event));
	void OnRadioSrcOnly(wxCommandEvent& WXUNUSED(event));
	void OnRadioTgtOnly(wxCommandEvent& WXUNUSED(event));
	void OnRadioSrcAndTgt(wxCommandEvent& WXUNUSED(event));
	void OnSpanCheckBoxChanged(wxCommandEvent& WXUNUSED(event));
    void OnUpdateReplaceAllButton(wxUpdateUIEvent& event);
    void OnUpdateReplace(wxUpdateUIEvent& event);
    void OnUpdateFindNext(wxUpdateUIEvent& event);
private:
    DECLARE_DYNAMIC_CLASS( CReplaceDlg )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

#endif /* FindReplace_h */
