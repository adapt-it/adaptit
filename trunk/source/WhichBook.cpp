/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			WhichBook.cpp
/// \author			Bill Martin
/// \date_created	3 January 2005
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CWhichBook class. 
/// The CWhichBook class declares a "Which Book Folder?" dialog that allows
/// the user to first choose one of five possible book divisions, then
/// choose which book folder from that division in which to work
/// \derivation		The CWhichBook class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in WhichBook.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "WhichBook.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
#include "Adapt_It.h"
#include "WhichBook.h"
#include "helpers.h" 
#include "Adapt_ItView.h"

// event handler table
BEGIN_EVENT_TABLE(CWhichBook, AIModalDialog)
	EVT_INIT_DIALOG(CWhichBook::InitDialog)
	EVT_COMBOBOX(IDC_COMBO_CHOOSE_BOOK, CWhichBook::OnSelchangeChooseBook)
	EVT_RADIOBUTTON(IDC_RADIO_DIV1, CWhichBook::OnRadioDivButton1)
	EVT_RADIOBUTTON(IDC_RADIO_DIV2, CWhichBook::OnRadioDivButton2)
	EVT_RADIOBUTTON(IDC_RADIO_DIV3, CWhichBook::OnRadioDivButton3)
	EVT_RADIOBUTTON(IDC_RADIO_DIV4, CWhichBook::OnRadioDivButton4)
	EVT_RADIOBUTTON(IDC_RADIO_DIV5, CWhichBook::OnRadioDivButton5)
END_EVENT_TABLE()

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // for rapid access to the app class

// CWhichBook dialog
CWhichBook::CWhichBook(wxWindow* parent)
	: AIModalDialog(parent, -1, _("Choose A Book Folder"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pWhichBookSizer = WhichBookDlgFunc(this, TRUE, TRUE);
	// The declaration is: WhichBookDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);

	m_pComboChooseBook = (wxComboBox*)FindWindowById(IDC_COMBO_CHOOSE_BOOK);
	wxASSERT(m_pComboChooseBook != NULL);
	m_pComboChooseBook->SetValidator(wxGenericValidator(&m_strBookName));

	m_pTextAsStatic = (wxTextCtrl*)FindWindowById(ID_TEXT_AS_STATIC);
	wxASSERT(m_pTextAsStatic != NULL);
	// Make the wxTextCtrl that is displaying static text have window background color
	wxColor backgrndColor = this->GetBackgroundColour();
	//m_pTextAsStatic->SetBackgroundColour(backgrndColor);
	m_pTextAsStatic->SetBackgroundColour(gpApp->sysColorBtnFace);

	// other attribute initializations
	m_strBookName.Empty();
	m_nDivIndex = -1;
}

CWhichBook::~CWhichBook(void)
{
}

// CWhichBook message handlers

void CWhichBook::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	//InitDialog() is not virtual, no call needed to a base class

	// get pointers to the radio buttons, so we can set their labels
	pRBtnDiv1 = (wxRadioButton*)FindWindowById(IDC_RADIO_DIV1);
	pRBtnDiv2 = (wxRadioButton*)FindWindowById(IDC_RADIO_DIV2);
	pRBtnDiv3 = (wxRadioButton*)FindWindowById(IDC_RADIO_DIV3);
	pRBtnDiv4 = (wxRadioButton*)FindWindowById(IDC_RADIO_DIV4);
	pRBtnDiv5 = (wxRadioButton*)FindWindowById(IDC_RADIO_DIV5);

	// set the button labels
	pRBtnDiv1->SetLabel(gpApp->m_strDivLabel[0]);
	pRBtnDiv2->SetLabel(gpApp->m_strDivLabel[1]);
	pRBtnDiv3->SetLabel(gpApp->m_strDivLabel[2]);
	pRBtnDiv4->SetLabel(gpApp->m_strDivLabel[3]);
	pRBtnDiv5->SetLabel(gpApp->m_strDivLabel[4]);

	// make the font show 12 point size in the dialog, use nav text font
	// and directionality throughout
	CopyFontBaseProperties(gpApp->m_pNavTextFont,gpApp->m_pDlgSrcFont);
	gpApp->m_pDlgSrcFont->SetPointSize(12);

	m_pComboChooseBook->SetFont(*gpApp->m_pDlgSrcFont);
	pRBtnDiv1->SetFont(*gpApp->m_pDlgSrcFont);
	pRBtnDiv2->SetFont(*gpApp->m_pDlgSrcFont);
	pRBtnDiv3->SetFont(*gpApp->m_pDlgSrcFont);
	pRBtnDiv4->SetFont(*gpApp->m_pDlgSrcFont);
	pRBtnDiv5->SetFont(*gpApp->m_pDlgSrcFont);

	// add RTL support for Unicode version
#ifdef _RTL_FLAGS
	if (gpApp->m_bSrcRTL)
	{
		m_pComboChooseBook->SetLayoutDirection(wxLayout_RightToLeft);
		pRBtnDiv1->SetLayoutDirection(wxLayout_RightToLeft);
		pRBtnDiv2->SetLayoutDirection(wxLayout_RightToLeft);
		pRBtnDiv3->SetLayoutDirection(wxLayout_RightToLeft);
		pRBtnDiv4->SetLayoutDirection(wxLayout_RightToLeft);
		pRBtnDiv5->SetLayoutDirection(wxLayout_RightToLeft);
	}
	else
	{
		m_pComboChooseBook->SetLayoutDirection(wxLayout_LeftToRight);
		pRBtnDiv1->SetLayoutDirection(wxLayout_LeftToRight);
		pRBtnDiv2->SetLayoutDirection(wxLayout_LeftToRight);
		pRBtnDiv3->SetLayoutDirection(wxLayout_LeftToRight);
		pRBtnDiv4->SetLayoutDirection(wxLayout_LeftToRight);
		pRBtnDiv5->SetLayoutDirection(wxLayout_LeftToRight);
	}
#endif

	// whm added 30Nov07 Radio buttons for any divisions that do not have labels (could
	// be the case if a custom books.xml file is used) should be hidden
	wxString wText;
	wText = pRBtnDiv1->GetLabel();
	if (wText == _T(""))
	{
		pRBtnDiv1->Show(FALSE);
	}
	wText = pRBtnDiv2->GetLabel();
	if (wText == _T(""))
	{
		pRBtnDiv2->Show(FALSE);
	}
	wText = pRBtnDiv3->GetLabel();
	if (wText == _T(""))
	{
		pRBtnDiv3->Show(FALSE);
	}
	wText = pRBtnDiv4->GetLabel();
	if (wText == _T(""))
	{
		pRBtnDiv4->Show(FALSE);
	}
	wText = pRBtnDiv5->GetLabel();
	if (wText == _T(""))
	{
		pRBtnDiv5->Show(FALSE);
	}

	// set up the combobox contents and book shown initially
	if (gpApp->m_nBookIndex < 0)
	{
		// there is no index defined, so use the default
		gpApp->m_nBookIndex = gpApp->m_nDefaultBookIndex;
	}
	m_oldIndex = gpApp->m_nBookIndex; // preserve starting index value
	SetParams(gpApp->m_nBookIndex); // set division & m_nDivIndex

	// populate the combo's list using the division sublist, and have the correct name 
	// showing & determine its folder's absolute path, and the app's m_pCurrBookNamePair 
	// which is needed for status bar messages
	SetupBooksList(division);

	// position the dialog 200 pixels down from the top of the screen
	CAdapt_ItView* pView = gpApp->GetView();
	pView->SetWhichBookPosition(this);

	FinishSetup();

	// update the status bar
	gpApp->RefreshStatusBarInfo();
	
	pWhichBookSizer->Layout();
}

void CWhichBook::SetParams(int nIndex)
// nIndex is the index into the CPtrArray m_pBibleBooks, the value determines
// what division and m_nDivIndex values are to be used for the initial combobox display
{
	// turn all five radio buttons off; we'll set them explicitly even though MFC does it for us
	pRBtnDiv1->SetValue(FALSE);
	pRBtnDiv2->SetValue(FALSE);
	pRBtnDiv3->SetValue(FALSE);
	pRBtnDiv4->SetValue(FALSE);
	pRBtnDiv5->SetValue(FALSE);

	// set division, and m_nDivIndex, and the radio button which is to be on
	if (nIndex < gpApp->m_nDivSize[0])
	{
		division = 0; // "History books"
		m_nDivIndex = gpApp->m_nBookIndex; // set dialog combo box's index value
		pRBtnDiv1->SetValue(TRUE);
	}
	else if (nIndex < gpApp->m_nDivSize[0] + gpApp->m_nDivSize[1])
	{
		division = 1; // "Wisdom books"
		m_nDivIndex = gpApp->m_nBookIndex - gpApp->m_nDivSize[0];
		pRBtnDiv2->SetValue(TRUE);
	}
	else if (nIndex < gpApp->m_nDivSize[0] + gpApp->m_nDivSize[1] + gpApp->m_nDivSize[2])
	{
		division = 2; // "Prophecy books"
		m_nDivIndex = gpApp->m_nBookIndex - gpApp->m_nDivSize[0] - gpApp->m_nDivSize[1];
		pRBtnDiv3->SetValue(TRUE);
	}
	else if (nIndex < gpApp->m_nDivSize[0] + gpApp->m_nDivSize[1] + gpApp->m_nDivSize[2]
					+ gpApp->m_nDivSize[3])
	{
		division = 3; // "New Testament books"
		m_nDivIndex = gpApp->m_nBookIndex - gpApp->m_nDivSize[0] - gpApp->m_nDivSize[1]
									- gpApp->m_nDivSize[2];
		pRBtnDiv4->SetValue(TRUE);
	}
	else
	{
		division = 4; // "Other Texts"
		m_nDivIndex = gpApp->m_nBookIndex - gpApp->m_nDivSize[0] - gpApp->m_nDivSize[1]
									- gpApp->m_nDivSize[2] - gpApp->m_nDivSize[3];
		pRBtnDiv5->SetValue(TRUE); 
	}
}

void CWhichBook::SetupBooksList(int division)
// division is a 0-based index to a division (ie. group) of Bible books
{
	int lower = 0;
	int upper = 0;
	if (division == 0)
	{
		// History books
		upper = gpApp->m_nDivSize[0];
	}
	else if (division == 1)
	{
		lower = gpApp->m_nDivSize[0];
		upper = lower + gpApp->m_nDivSize[1];
	}
	else if (division == 2)
	{
		lower = gpApp->m_nDivSize[0] + gpApp->m_nDivSize[1];
		upper = lower + gpApp->m_nDivSize[2];
	}
	else if (division == 3)
	{
		lower = gpApp->m_nDivSize[0] + gpApp->m_nDivSize[1] + gpApp->m_nDivSize[2];
		upper = lower	+ gpApp->m_nDivSize[3];
	}
	else // division must be 4
	{
		lower = gpApp->m_nDivSize[0] + gpApp->m_nDivSize[1] + gpApp->m_nDivSize[2]
					 + gpApp->m_nDivSize[3];
		upper = lower + gpApp->m_nDivSize[4];
	}

	// add the folder names
	for (int i = lower; i < upper; i++)
	{
		wxString s = ((BookNamePair*)(*gpApp->m_pBibleBooks)[i])->seeName;
		m_pComboChooseBook->Append(s);
	}
}

void CWhichBook::FinishSetup()
{
	// TODO: Determine if wxComboBox can be manipulated to force it to show the
	// number of items as MFC code does below. This is not a critical need but
	// would be a convenience, depending on how wxWidgets behaves in this case.
	m_pComboChooseBook->SetSelection(m_nDivIndex); //BOOL bOK = m_comboChooseBook.SetCurSel(m_nDivIndex);
	m_strBookName = m_pComboChooseBook->GetString(m_nDivIndex); // visible name, not for path
	wxASSERT(!m_strBookName.IsEmpty());
	gpApp->m_pCurrBookNamePair = (BookNamePair*)(*gpApp->m_pBibleBooks)[gpApp->m_nBookIndex]; 
	wxString folderName = gpApp->m_pCurrBookNamePair ->dirName;
	wxASSERT(!folderName.IsEmpty());
	gpApp->m_bibleBooksFolderPath = gpApp->m_curAdaptionsPath + gpApp->PathSeparator + folderName; //gpApp->m_bibleBooksFolderPath = gpApp->m_curAdaptionsPath + _T("\\") + folderName;
}

void CWhichBook::OnSelchangeChooseBook(wxCommandEvent& WXUNUSED(event))
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	if (m_pComboChooseBook->GetCount() == 0)
		return;

	// get the new value of the combobox & get the string for the visible name, & 
	// the paired string for folder's name
	m_nDivIndex = m_pComboChooseBook->GetSelection();
	wxASSERT(m_nDivIndex != wxNOT_FOUND);
	m_strBookName = m_pComboChooseBook->GetStringSelection();
	wxASSERT(!m_strBookName.IsEmpty());

	// calculate the app's new m_nBookIndex value
	switch (division)
	{
	case 0: // History books
		gpApp->m_nBookIndex = m_nDivIndex;
		break;
	case 1: // Widsom books
		gpApp->m_nBookIndex = gpApp->m_nDivSize[0] + m_nDivIndex;
		break;
	case 2: // Prophecy books
		gpApp->m_nBookIndex = gpApp->m_nDivSize[0] + gpApp->m_nDivSize[1] + m_nDivIndex;
		break;
	case 3: // New Testament books
		gpApp->m_nBookIndex = gpApp->m_nDivSize[0] + gpApp->m_nDivSize[1] + 
					gpApp->m_nDivSize[2] + m_nDivIndex;
		break;
	case 4: // Other Texts
	default:
		gpApp->m_nBookIndex = gpApp->m_nDivSize[0] + gpApp->m_nDivSize[1] + 
					gpApp->m_nDivSize[2] + gpApp->m_nDivSize[3] + m_nDivIndex;
	}

	// set the name pair structure, and the Bible books folder path
	gpApp->m_pCurrBookNamePair = (BookNamePair*)(*gpApp->m_pBibleBooks)[gpApp->m_nBookIndex]; 
	wxString folderName = gpApp->m_pCurrBookNamePair ->dirName;
	wxASSERT(!folderName.IsEmpty());
	gpApp->m_bibleBooksFolderPath = gpApp->m_curAdaptionsPath + gpApp->PathSeparator + folderName; //gpApp->m_bibleBooksFolderPath = gpApp->m_curAdaptionsPath + _T("\\") + folderName;

	// update the status bar
	gpApp->RefreshStatusBarInfo();
}

void CWhichBook::OnRadioDivButton1(wxCommandEvent& WXUNUSED(event)) 
{
	m_pComboChooseBook->Clear();
	gpApp->m_nBookIndex = 0; // Genesis, default for first in this division
	SetParams(gpApp->m_nBookIndex); // set division & m_nDivIndex
	SetupBooksList(division);
	FinishSetup();
}

void CWhichBook::OnRadioDivButton2(wxCommandEvent& WXUNUSED(event))
{
	m_pComboChooseBook->Clear();
	gpApp->m_nBookIndex = gpApp->m_nDivSize[0]; // first book of Wisdom division (Job)
	SetParams(gpApp->m_nBookIndex); // set division & m_nDivIndex
	SetupBooksList(division);
	FinishSetup();
}

void CWhichBook::OnRadioDivButton3(wxCommandEvent& WXUNUSED(event))
{
	m_pComboChooseBook->Clear();
	gpApp->m_nBookIndex = gpApp->m_nDivSize[0] + gpApp->m_nDivSize[1]; // first book of Prophecy division (Isaiah)
	SetParams(gpApp->m_nBookIndex); // set division & m_nDivIndex
	SetupBooksList(division);
	FinishSetup();
}

void CWhichBook::OnRadioDivButton4(wxCommandEvent& WXUNUSED(event))
{
	m_pComboChooseBook->Clear();
	gpApp->m_nBookIndex = gpApp->m_nDivSize[0] + gpApp->m_nDivSize[1]
	+ gpApp->m_nDivSize[2]; // first book of New Testament division (Matthew)
	SetParams(gpApp->m_nBookIndex); // set division & m_nDivIndex
	SetupBooksList(division);
	FinishSetup();
}

void CWhichBook::OnRadioDivButton5(wxCommandEvent& WXUNUSED(event))
{
	m_pComboChooseBook->Clear();
	gpApp->m_nBookIndex = gpApp->m_nDivSize[0] + gpApp->m_nDivSize[1]
	+ gpApp->m_nDivSize[2] + gpApp->m_nDivSize[3]; // first book of Other Texts division
	SetParams(gpApp->m_nBookIndex); // set division & m_nDivIndex
	SetupBooksList(division);
	FinishSetup();
}

void CWhichBook::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	m_pComboChooseBook->Clear();
	gpApp->m_nBookIndex = m_oldIndex; // restore old index (or default)
	SetParams(gpApp->m_nBookIndex); // set division & m_nDivIndex
	SetupBooksList(division);
	FinishSetup();

	// update the status bar
	gpApp->RefreshStatusBarInfo();

	EndModal(wxID_CANCEL); //wxDialog::OnCancel(event);
}

void CWhichBook::OnOK(wxCommandEvent& event)
{
	// update the status bar
	gpApp->RefreshStatusBarInfo();

	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event);
}
