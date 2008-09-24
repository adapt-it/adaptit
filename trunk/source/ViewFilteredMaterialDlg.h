/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			ViewFilteredMaterialDlg.h
/// \author			Bill Martin
/// \date_created	2 July 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CViewFilteredMaterialDlg class. 
/// The CViewFilteredMaterialDlg class provides a modeless dialog enabling the user to view
/// and edit filtered information. It is the dialog that appears when the user clicks on a
/// (green) wedge signaling the presence of filtered information hidden within the document.
/// The CViewFilteredMaterialDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// \derivation		The CViewFilteredMaterialDlg class is derived from wxDialog.
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
/// \derivation		The CViewFilteredMaterialDlg class is derived from wxDialog.
class CViewFilteredMaterialDlg : public wxDialog
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

	// the following are indexed in parallel and contain info from all markers, filtered and not filtered
	wxArrayString AllMkrsList; // list of all markers in m_markers (both filtered and non-filtered)
	wxArrayInt AllMkrsFilteredFlags; // array of ints that flag if marker in AllMkrsList is filtered (1) or not (0)
	wxArrayString AllWholeMkrsArray; // array of all whole markers encountered in m_markers
	wxArrayString AllEndMkrsArray; // array of all end markers encountered in m_markers (contains a space if no end marker)

	// the following are indexed in parallel
	wxArrayString bareMarkerArray;
	// BEW added comment for clarity 17Nov05; because AllMkrsList, and the parallel array AllMkrsFilteredFlags
	// potentially contain information for markers which are not filtered, and therefore which are not visible
	// in the View Filtered pMarkers dialog, we cannot assume that the index returned by a click in a marker list
	// in the dialog will also index the appropriate marker in AllMkrsList, nor the appropriate flag in
	// AllMkrsFilteredFlags; hence we maintain a CUIntArray called markerLBIndexIntoAllMkrList which, given a
	// returned index from a marker list (either the initial one, or the end markers list) entry click in the
	// dialog, we can look up the index we need for AllMkrsList in the CUIntArray markerLBIndexIntoAllMkrList.
	wxArrayInt markerLBIndexIntoAllMkrList;
	wxArrayString assocTextArrayBeforeEdit;
	wxArrayString assocTextArrayAfterEdit;
	int indexIntoAllMkrSelection;
	int indexIntoMarkersLB;
	int currentMkrSelection;
	int prevMkrSelection;
	int newMkrSelection;
	bool changesMade;
	bool bCanRemoveBT; // TRUE if user has just clicked on a \bt or derivative \bt marker, else FALSE
	bool bCanRemoveFT; // TRUE if user has just clicked on a \free marker, else FALSE
	wxString btnStr;
	wxString ftStr;
	wxString btStr;
	wxString removeBtnTitle;
	bool bRemovalDone; // true when the Remove.. button has removed a free translation or back translation
	void OnCancel(wxCommandEvent& WXUNUSED(event));
	wxSizer* pViewFilteredMaterialDlgSizer;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& WXUNUSED(event));
	void OnLbnSelchangeListMarker(wxCommandEvent& WXUNUSED(event));
	void OnLbnSelchangeListMarkerEnd(wxCommandEvent& event);
	void OnEnChangeEditMarkerText(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRemoveBtn(wxCommandEvent& WXUNUSED(event));
	void SetRemoveButtonFlags(wxListBox* pMarkers, int nSelection, bool& bCanRemoveFT, bool& bCanRemoveBT);
	void GetAndShowMarkerDescription(int indexIntoAllMkrSelection);
	void UpdateContentOnRemove();
#ifdef _UNICODE
	void OnButtonSwitchEncoding(wxCommandEvent& WXUNUSED(event));
#endif

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ViewFilteredMaterialDlg_h */
