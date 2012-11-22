/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			EditPreferencesDlg.h
/// \author			Bill Martin
/// \date_created	13 August 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the definition file for the CEditPreferencesDlg class. 
/// The CEditPreferencesDlg class acts as a dialog wrapper for the tab pages of
/// an "Edit Preferences" wxNotebook. The interface resources for the wxNotebook 
/// dialog are defined in EditPreferencesDlgFunc(), which was created and is 
/// maintained by wxDesigner. The notebook contains up to 8 tabs labeled "Fonts", 
/// "Backups and KB", "View", "Auto-Saving", "Punctuation", "Case", "Units", and
/// "USFM and Filtering" depending on the current user workflow profile selected.
/// \derivation		The CEditPreferencesDlg class is derived from wxPropertySheetDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef EditPreferencesDlg_h
#define EditPreferencesDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "EditPreferencesDlg.h"
#endif

// forward references
class CFontPagePrefs;
class CPunctCorrespPagePrefs;
class CCaseEquivPagePrefs;
class CKBPage;
class CViewPage;
class CAutoSavingPage;
class CUnitsPage;
class CUsfmFilterPagePrefs;
class wxPropertySheetDialog;

/// The CEditPreferencesDlg class acts as a dialog wrapper for the tab pages of
/// an "Edit Preferences" wxNotebook. The interface resources for the wxNotebook 
/// dialog are defined in EditPreferencesDlgFunc(), which was created and is 
/// maintained by wxDesigner. The notebook contains up to 8 tabs labeled "Fonts", 
/// "Backups and KB", "View", "Auto-Saving", "Punctuation", "Case", "Units", 
/// "USFM and Filtering" depending on the current user workflow profile selected.
/// \derivation		The CEditPreferencesDlg class is derived from wxPropertySheetDialog.
// whm 8Jun12 changed wxScrollingPropertySheetDialog back to wxPropertySheetDialog
class CEditPreferencesDlg : public wxPropertySheetDialog
{
	//DECLARE_DYNAMIC_CLASS(CEditPreferencesDlg)
public:
	CEditPreferencesDlg();
	CEditPreferencesDlg(
		wxWindow* parent, wxWindowID id, const wxString& title,
		const wxPoint& pos, const wxSize& size,
		long style); 

	bool Create(
		wxWindow* parent, wxWindowID id, const wxString& title,
		const wxPoint& pos, const wxSize& size,
		long style);

	void CreateControls(); // creates the controls and sizers

	virtual ~CEditPreferencesDlg(void); // whm make all destructors virtual

	bool m_bDismissDialog;

	// Pointer/Handles to notebook pages
	CFontPagePrefs* fontPage;
	CPunctCorrespPagePrefs* punctMapPage;
	CCaseEquivPagePrefs* caseEquivPage;
	CKBPage* kbPage;
	CViewPage* viewPage;
	CAutoSavingPage* autoSavePage;
	CUnitsPage* unitsPage;
	CUsfmFilterPagePrefs* usfmFilterPage;

	wxBookCtrlBase* pNotebook;

	void InitDialog(wxInitDialogEvent& event);
	void OnOK(wxCommandEvent& event);
	// Wrapper handlers for fontPage
	void OnSourceFontChangeBtn(wxCommandEvent& event);
	void OnTargetFontChangeBtn(wxCommandEvent& event);
	void OnNavTextFontChangeBtn(wxCommandEvent& event);
	void OnButtonSpecTextColor(wxCommandEvent& event);
	void OnButtonRetranTextColor(wxCommandEvent& event);
	void OnButtonNavTextColor(wxCommandEvent& event);
	void OnButtonSourceTextColor(wxCommandEvent& event);
	void OnButtonTargetTextColor(wxCommandEvent& event);
	// Wrapper handlers for kbPage
	void OnCheckKbBackup(wxCommandEvent& event);
	void OnCheckBakupDoc(wxCommandEvent& event);
	// Wrapper handler for viewPage
	void OnButtonHighlightColor(wxCommandEvent& event);
	// Wrapper handlers for autoSavePage
	void OnCheckNoAutoSave(wxCommandEvent& event);
	void OnRadioByMinutes(wxCommandEvent& event);
	void OnRadioByMoves(wxCommandEvent& event); 
	void EnableAll(bool bEnable);
	// Wrapper handlers for unitsPage
	void OnRadioUseInches(wxCommandEvent& event);
	void OnRadioUseCentimeters(wxCommandEvent& event);
	// Wrapper handlers for punctMapPage
#ifdef _UNICODE
	void OnBnClickedToggleUnnnn(wxCommandEvent& event);
#endif
	// Wrapper handlers for caseEquivPage
	void OnBnClickedClearSrcList(wxCommandEvent& event);
	void OnBnClickedSrcSetEnglish(wxCommandEvent& event);
	void OnBnClickedSrcCopyToNext(wxCommandEvent& event);
	void OnBnClickedSrcCopyToGloss(wxCommandEvent& event);
	void OnBnClickedClearTgtList(wxCommandEvent& event);
	void OnBnClickedTgtSetEnglish(wxCommandEvent& event);
	void OnBnClickedTgtCopyToNext(wxCommandEvent& event);
	void OnBnClickedClearGlossList(wxCommandEvent& event);
	void OnBnClickedGlossSetEnglish(wxCommandEvent& event);
	void OnBnClickedGlossCopyToNext(wxCommandEvent& event);
	void OnBnCheckedSrcHasCaps(wxCommandEvent& event);
	void OnBnCheckedUseAutoCaps(wxCommandEvent& event);

	// Wrapper handlers for usfmPage
	void OnBnClickedRadioUseUbsSetOnlyDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseBothSetsDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event);
	void OnBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event);
	void OnBnClickedRadioUseBothSetsProj(wxCommandEvent& event);
	void OnBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& event);

	// Wrapper handlers for filterPage
	void OnLbnSelchangeListSfmsDoc(wxCommandEvent& event);
	void OnCheckListBoxToggleDoc(wxCommandEvent& event);
	void OnLbnSelchangeListSfmsProj(wxCommandEvent& event);
	void OnCheckListBoxToggleProj(wxCommandEvent& event);

	bool TabIsVisibleInCurrentProfile(wxString tabLabel);

	DECLARE_EVENT_TABLE()
};

#endif // EditPreferencesDlg_h
