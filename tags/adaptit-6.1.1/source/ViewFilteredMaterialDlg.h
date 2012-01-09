/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ViewFilteredMaterialDlg.h
/// \author			Bill Martin
/// \date_created	2 July 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CViewFilteredMaterialDlg class. 
/// The CViewFilteredMaterialDlg class provides a modeless dialog enabling the user to view
/// and edit filtered information. It is the dialog that appears when the user clicks on a
/// (green) wedge signaling the presence of filtered information hidden within the document.
/// The CViewFilteredMaterialDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// \derivation		The CViewFilteredMaterialDlg class is derived from wxScrollingDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ViewFilteredMaterialDlg_h
#define ViewFilteredMaterialDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ViewFilteredMaterialDlg.h"
#endif

/// The CViewFilteredMaterialDlg class provides a modeless dialog enabling the user to view
/// and edit filtered information. It is the dialog that appears when the user clicks on a
/// (green) wedge signaling the presence of filtered information hidden within the document.
/// The CViewFilteredMaterialDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// \derivation		The CViewFilteredMaterialDlg class is derived from wxScrollingDialog.
class CViewFilteredMaterialDlg : public wxScrollingDialog
{
public:
	CViewFilteredMaterialDlg(wxWindow* parent); // constructor
	virtual ~CViewFilteredMaterialDlg(void); // destructor

	//enum { IDD = IDD_VIEW_FILTERED_MATERIAL };
	
	// pointers to dialog controls
	wxListBox* pMarkers; 
	wxListBox* pEndMarkers; 
	wxTextCtrl* pMkrTextEdit;
	wxStaticText* pMkrDescStatic;
	wxStaticText* pMkrStatusStatic;
	wxButton* pSwitchEncodingButton;
	wxButton* pRemoveBtn;
	wxString markers;

	// For docVersion = 5, there are significant changes. The legacy code extracted all
	// markers from m_markers, both filtered and non-filtered, which produced
	// complications because the index obtained for a user click in the dialog's markers
	// list box may then not correspond to the item in AllMkrsList if the latter contained
	// non-filtered markers.
	// For docVersion 5, we don't store any filtered information in m_markers, so we can
	// ignore that member entirely. Filtered information instead is in m_filteredInfo
	// (along with marker, and any endmarker, and wrapping \~FILTER and \~FILTER* markers,
	// for each filtered information type), and/or in one, two or three of the wxString
	// members m_freeTrans, m_note, and m_collectedBackTrans - the latter three contain
	// the content strings only, no markers, and so for the dialog if any of these are
	// present, we need to generate \free \free*, \note \note*, and \bt markers,
	// respectively for showing in the relevant dialog list boxes. Version 5 also has a
	// smarter CSourcePhrase, which extracts the information in m_filteredInfo in three
	// parallel wxArrayString parameters, which simplifies getting access to the markers,
	// endmarkers and content strings as discrete information chunks.
	// All the above calls for a redesign of the code for supporting the View Filtered
	// Material dialog. Instead of 8 arrays, we only need 5 - three for the arrays of
	// markers, content text strings, endmarkers; one for the after-edit content text
	// strings, and one for the bare markers array for lookup purposes of USFM info.
	// We also define a fixed order for display in the dialog: first, if present, is the
	// free translation; second, if present, is the note; third, if present, is the
	// collected back translation; after those follow any filtered information from
	// m_filteredInfo. Having the filtered free translation first allows the user to have
	// the free translation shown in the view filtered material dialog, and a note
	// displayed in the note dialog, at the same time and with no clicking other than on
	// the respective green wedge and note icons in the main window.
	 
	// the following are indexed in parallel and contain info from all filtered markers only
	//wxArrayString AllMkrsList; // list of all markers in m_markers (filtered and non-filtered)
	//wxArrayInt AllMkrsFilteredFlags; // array of ints that flag if marker in AllMkrsList is filtered (1) or not (0)
	wxArrayString AllWholeMkrsArray; // array of all whole markers encountered in m_markers
	wxArrayString AllEndMkrsArray; // array of all end markers encountered in m_markers (contains a space if no end marker)
	wxArrayString bareMarkerArray;
	//wxArrayInt markerLBIndexIntoAllMkrList;
	wxArrayString assocTextArrayBeforeEdit;
	wxArrayString assocTextArrayAfterEdit;

	int indexIntoMarkersLB;
	int currentMkrSelection;
	int prevMkrSelection;
	int newMkrSelection;
	bool bCanRemoveBT; // TRUE if user has just clicked on a \bt or derivative \bt marker, else FALSE
	bool bCanRemoveFT; // TRUE if user has just clicked on a \free marker, else FALSE
	wxString btnStr;
	wxString ftStr;
	wxString btStr;
	wxString removeBtnTitle;
	bool bRemovalDone; // true when the Remove.. button has removed a free translation or back translation
	wxSizer* pViewFilteredMaterialDlgSizer;

	void OnCancel(wxCommandEvent& WXUNUSED(event)); // CAdapt_ItCanvas accesses this, so it's public
protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void ReinterpretEnterKeyPress(wxCommandEvent& event);
	void OnOK(wxCommandEvent& WXUNUSED(event));
	void OnLbnSelchangeListMarker(wxCommandEvent& WXUNUSED(event));
	void OnLbnSelchangeListMarkerEnd(wxCommandEvent& event);
	void OnEnChangeEditMarkerText(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRemoveBtn(wxCommandEvent& WXUNUSED(event));
	void SetRemoveButtonFlags(wxListBox* pMarkers, int nSelection, bool& bCanRemoveFT, bool& bCanRemoveBT);
	void GetAndShowMarkerDescription(int indexIntoAllMkrSelection);
#ifdef _UNICODE
	void OnButtonSwitchEncoding(wxCommandEvent& WXUNUSED(event));
#endif

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ViewFilteredMaterialDlg_h */
