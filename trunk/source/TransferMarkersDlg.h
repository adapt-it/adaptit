/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			TransferMarkersDlg.h
/// \author			Bill Martin
/// \date_created	13 July 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CTransferMarkersDlg class. 
/// The CTransferMarkersDlg class provides a secondary dialog to the CEditSourceTextDlg 
/// that gets called to enable the user to adjust the position of markers within a
/// span of source text being edited.
/// \derivation		The CTransferMarkersDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef TransferMarkersDlg_h
#define TransferMarkersDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "TransferMarkersDlg.h"
#endif

/// The CTransferMarkersDlg class provides a secondary dialog to the CEditSourceTextDlg 
/// that gets called to enable the user to adjust the position of markers within a
/// span of source text being edited.
/// \derivation		The CTransferMarkersDlg class is derived from AIModalDialog.
class CTransferMarkersDlg : public AIModalDialog
{
public:
	CTransferMarkersDlg(wxWindow* parent); // constructor
	virtual ~CTransferMarkersDlg(void); // destructor
	// other methods
	//enum { IDD = IDD_TRANSFER_MARKERS_DLG };
	
	// pointers to dialog's controls
	wxTextCtrl*	m_pEditAttachedMarkers;
	wxTextCtrl*	m_pEditNewWords;
	wxTextCtrl*	m_pEditOldWords;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs1;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs2;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs3;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs4;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs5;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs6;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs7;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs8;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs9;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs10;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs11;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs12;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs13;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs14;
	wxTextCtrl*	pTextCtrlAsStaticTransfMkrs15;
	wxButton*	m_pButtonTransfer;
	wxButton*	m_pButtonUpdateMkrs;
	wxButton*	m_pButtonRemoveMkrs;
	wxListBox*	m_pListOldWords; // whm redesigned 10Aug06 to use wxListBox
	wxListBox*	m_pListMarkers;  // whm redesigned 10Aug06 to use wxListBox
	wxListBox*	m_pListNewWords; // whm redesigned 10Aug06 to use wxListBox

	wxString	m_strOldSourceText;
	wxString	m_strNewSourceText;
	wxString	m_strNewMarkers;
	
	wxArrayInt m_snList; // list of sequ numbers, one for each of the m_comboOldWords words
	// whm added below array to avoid use of Client data for storage of selection info
	wxArrayInt m_oldWordsSelectionBegin;
	wxArrayInt m_oldWordsSelectionEnd;
	wxArrayInt m_newWordsSelectionBegin;
	wxArrayInt m_newWordsSelectionEnd;
	
	void ClearForTransfer(CSourcePhrase* pToSrcPhrase);
	bool SimpleCopyTransfer(CSourcePhrase* pFromSP, CSourcePhrase* pToSP); // whm changed to bool

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

	void UpdateNewSourceTextAndCombo(SPList* pNewList, int nItem);
	void OnPaint(wxPaintEvent& event);
	void OnButtonTransfer(wxCommandEvent& WXUNUSED(event));
	void OnButtonUpdateMarkers(wxCommandEvent& event);
	void OnButtonRemoveMarkers(wxCommandEvent& WXUNUSED(event));
	void OnButtonAddMarkers(wxCommandEvent& WXUNUSED(event));
	void OnSelchangeListOldWordsWithSfm(wxCommandEvent& WXUNUSED(event));
	void OnSelchangeListSfMarkers(wxCommandEvent& WXUNUSED(event));
	void OnSelchangeListNewWords(wxCommandEvent& WXUNUSED(event));

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* TransferMarkersDlg_h */
