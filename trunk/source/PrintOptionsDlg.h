/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			PrintOptionsDlg.h
/// \author			Bill Martin
/// \date_created	10 November 2006
/// \date_revised	29 February 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CPrintOptionsDlg class. 
/// The CPrintOptionsDlg class provides a dialog for the user to enter a range
/// of pages, range of chapter/verse and specify how to handle margin elements 
/// such as section headings and footers at print time. This dialog pops up
/// before the standard print dialog when the user selects File | Print.
/// \derivation		The CPrintOptionsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef PrintOptionsDlg_h
#define PrintOptionsDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "PrintOptionsDlg.h"
#endif

class CAdapt_ItView;

/// The CPrintOptionsDlg class creates a supplemental dialog that pops up before the standard print
/// dialog. It allows the user to select special printing options including any page and 
/// chapter/verse ranges. It also allows the user to specify if a footer is to be printed, 
/// and how to handle any preceding and/or following section headingss in the printout. 
/// Instead of an "OK" button, the dialog has a "Print >>" button which closes the dialog but program
/// flow then proceeds to the standard print dialog.
/// The interface resources are loaded by means of the PrintOptionsDlgFunc() function which 
/// was developed and is maintained by wxDesigner.
/// \derivation		The CPrintOptionsDlg class is derived from AIModalDialog.
class CPrintOptionsDlg : public AIModalDialog
{
public:
	CPrintOptionsDlg(wxWindow* parent); //,wxPrintout* pPrintout); // constructor
	virtual ~CPrintOptionsDlg(void); // destructor
	
	CAdapt_ItView* m_pView;
	bool m_bPrintingRange;
	bool m_bSuppressFooter;

	//wxPrintout* m_pPrintout;

	wxTextCtrl* pEditChFrom;
	wxTextCtrl* pEditVsFrom;
	wxTextCtrl* pEditChTo;
	wxTextCtrl* pEditVsTo;
	wxTextCtrl* pEditPagesFrom;
	wxTextCtrl* pEditPagesTo;
	wxTextCtrl* pEditAsStatic; // the read only static text help information
	wxRadioButton* pRadioAll;
	wxRadioButton* pRadioSelection;
	wxRadioButton* pRadioPages;
	wxRadioButton* pRadioChVs;
	wxCheckBox* pCheckSuppressPrecSectHeading;
	wxCheckBox* pCheckIncludeFollSectHeading;
	wxCheckBox* pCheckSuppressPrintingFooter;


protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnSetfocus(wxFocusEvent& event);
	void OnAllPagesBtn(wxCommandEvent& WXUNUSED(event));
	void OnSelectBtn(wxCommandEvent& WXUNUSED(event));
	void OnPagesBtn(wxCommandEvent& WXUNUSED(event));
	void OnRadioChapterVerseRange(wxCommandEvent& WXUNUSED(event));
	void OnCheckSuppressFooter(wxCommandEvent& WXUNUSED(event));
	void OnCheckSuppressPrecedingHeading(wxCommandEvent& WXUNUSED(event));
	void OnCheckIncludeFollowingHeading(wxCommandEvent& WXUNUSED(event));
	void OnEditPagesFrom(wxCommandEvent& WXUNUSED(event));
	void OnEditPagesTo(wxCommandEvent& WXUNUSED(event));
	void OnEditChapterFrom(wxCommandEvent& WXUNUSED(event));
	void OnEditVerseFrom(wxCommandEvent& WXUNUSED(event));
	void OnEditChapterTo(wxCommandEvent& WXUNUSED(event));
	void OnEditVerseTo(wxCommandEvent& WXUNUSED(event));

private:
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* PrintOptionsDlg_h */
