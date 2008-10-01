/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			WhichBook.h
/// \author			Bill Martin
/// \date_created	3 January 2005
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CWhichBook class. 
/// The CWhichBook class declares a "Which Book Folder?" dialog that allows
/// the user to first choose one of five possible book divisions, then
/// choose which book folder from that division in which to work
/// \derivation		The CWhichBook class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef WhichBook_h
#define WhichBook_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "WhichBook.h"
#endif

/// The CWhichBook class declares a "Which Book Folder?" dialog that allows
/// the user to first choose one of five possible book divisions, then
/// choose which book folder from that division in which to work
/// \derivation		The CWhichBook class is derived from AIModalDialog.
class CWhichBook : public AIModalDialog
{
	//DECLARE_DYNAMIC(CWhichBook)

public:
	CWhichBook(wxWindow* parent);   // standard constructor
	virtual ~CWhichBook(void);		// destructor

	// other methods

	//enum { IDD = IDD_WHICH_BOOK };
	wxComboBox* m_pComboChooseBook; // must be pointer to wxDesigner pre-created combo box

	wxTextCtrl* m_pTextAsStatic;

	// m_nDivIndex is the selected item in the division's book name list,
	// division is the index of the chosen division itself
	int m_nDivIndex;
	int division;
	wxString m_strBookName; // the visible name only, we don't have a member for
											 // the folder name, we set the path using a local CString
	int m_oldIndex; // save the initial index value in case user cancels

	wxSizer* pWhichBookSizer;

	// pointers to the radio buttons (their labels are localizable) & the pointers
	// are initialized in the OnInitDialog()
	wxRadioButton* pRBtnDiv1;
	wxRadioButton* pRBtnDiv2;
	wxRadioButton* pRBtnDiv3;
	wxRadioButton* pRBtnDiv4;
	wxRadioButton* pRBtnDiv5;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

	void OnOK(wxCommandEvent& event);
	virtual void OnCancel(wxCommandEvent& WXUNUSED(event));
	void OnSelchangeChooseBook(wxCommandEvent& WXUNUSED(event));
	void OnRadioDivButton1(wxCommandEvent& WXUNUSED(event));
	void OnRadioDivButton2(wxCommandEvent& WXUNUSED(event));
	void OnRadioDivButton3(wxCommandEvent& WXUNUSED(event));
	void OnRadioDivButton4(wxCommandEvent& WXUNUSED(event));
	void OnRadioDivButton5(wxCommandEvent& WXUNUSED(event));

	void SetupBooksList(int division);
	void SetParams(int nIndex);
	void FinishSetup();

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
public:

};
#endif /* WhichBook_h */
