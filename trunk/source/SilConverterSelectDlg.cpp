/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SilConverterSelectDlg.cpp
/// \author			Bill Martin
/// \date_created	28 April 2006
/// \date_revised	30 December 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CSilConverterSelectDlg class. 
/// The CSilConverterSelectDlg class uses Bob Eaton's ECDriver.dll dynamic library so that 
/// Adapt It (Windows port only) can interface with SILConverters functionality as a cc-like 
/// filter for source phrases entered in the phrase box.
/// \derivation		The CSilConverterSelectDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in SilConverterSelectDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "SilConverterSelectDlg.h"
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
//#include <wx/valgen.h> // for wxGenericValidator
//#include <wx/valtext.h> // for wxTextValidator
#include <wx/dynlib.h> // for wxDynamicLibrary

#include "Adapt_It.h"
#include "SilConverterSelectDlg.h"
#ifdef USE_SIL_CONVERTERS
#include "ecdriver.h"
#endif

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // for rapid access to the app class

extern wxDynamicLibrary ecDriverDynamicLibrary;
extern const wxChar *FUNC_NAME_EC_INITIALIZE_CONVERTER_AW;
extern const wxChar *FUNC_NAME_EC_SELECT_CONVERTER_AW;
extern const wxChar *FUNC_NAME_EC_CONVERTER_DESCRIPTION_AW;

// event handler table
BEGIN_EVENT_TABLE(CSilConverterSelectDlg, AIModalDialog)
	EVT_INIT_DIALOG(CSilConverterSelectDlg::InitDialog)
    EVT_BUTTON(IDC_BTN_SELECT_SILCONVERTER, CSilConverterSelectDlg::OnBnClickedBtnSelectSilconverter)
    EVT_BUTTON(IDC_BTN_CLEAR_SILCONVERTER, CSilConverterSelectDlg::OnBnClickedBtnClearSilconverter)
END_EVENT_TABLE()


CSilConverterSelectDlg::CSilConverterSelectDlg(
						const wxString&	strConverterName, 
						BOOL            bDirectionForward,
						int				eNormalizeOutput,
						wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Choose SIL Converter"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
				, ConverterName(strConverterName)
				, DirectionForward(bDirectionForward)
				, NormalizeOutput(eNormalizeOutput)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	SilConvertersDlgFunc(this, TRUE, TRUE);
	// The declaration is: SilConvertersDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	pEditSILConverterName = (wxTextCtrl*)FindWindowById(IDC_ED_SILCONVERTER_NAME);
	wxASSERT(pEditSILConverterName != NULL);
	pEditSILConverterInfo = (wxTextCtrl*)FindWindowById(IDC_ED_SILCONVERTER_INFO); // (read only)
	wxASSERT(pEditSILConverterInfo != NULL);
	pBtnUnload = (wxButton*)FindWindowById(IDC_BTN_CLEAR_SILCONVERTER);
	wxASSERT(pBtnUnload != NULL);
	
	if (gpApp->bECDriverDLLLoaded)
	{
#ifdef USE_SIL_CONVERTERS
		// whm added 30Dec08 for SIL Converters support
		typedef int (wxSTDCALL *wxECInitConverterType)(const wxChar*,int,int);
		wxECInitConverterType pfnECInitializeConverterAorW = (wxECInitConverterType)NULL;
		// whm Note: The EncConverterInitializeConverter() function in ECDriver.dll only has A and W forms so we must
		// call GetSymbolAorW() instead of GetSymbol() here.
		pfnECInitializeConverterAorW = (wxECInitConverterType)ecDriverDynamicLibrary.GetSymbolAorW(FUNC_NAME_EC_INITIALIZE_CONVERTER_AW);
#endif	// end of if USE_SIL_CONVERTERS
	}

	// other attribute initializations
    // if one is already configured, then try to get the details (don't cache it in case it change):
    if( !ConverterName.IsEmpty() )
    {
		// if the converter isn't already connected...
#ifdef USE_SIL_CONVERTERS
		// whm added 30Dec08 for SIL Converters support
		typedef int (wxSTDCALL *wxECInitConverterType)(const wxChar*,int,int);
		wxECInitConverterType pfnECInitializeConverterAorW = (wxECInitConverterType)NULL;
		// whm Note: The EncConverterInitializeConverter() function in ECDriver.dll only has A and W forms so we must
		// call GetSymbolAorW() instead of GetSymbol() here.
		pfnECInitializeConverterAorW = (wxECInitConverterType)ecDriverDynamicLibrary.GetSymbolAorW(FUNC_NAME_EC_INITIALIZE_CONVERTER_AW);
		typedef int (wxSTDCALL *wxECConverterDescrType)(const wxChar*,wxChar*,int);
		wxECConverterDescrType pfnECConverterDescriptionAorW = (wxECConverterDescrType)NULL;
		// whm Note: The EncConverterConverterDescription() function in ECDriver.dll only has A and W forms so we must
		// call GetSymbolAorW() instead of GetSymbol() here.
		pfnECConverterDescriptionAorW = (wxECConverterDescrType)ecDriverDynamicLibrary.GetSymbolAorW(FUNC_NAME_EC_CONVERTER_DESCRIPTION_AW);
		// initialize the converter based on these stored configuration properties (but beware,
		//	it may no longer be installed! So do something in the 'else' case to inform user)
		if (pfnECInitializeConverterAorW != NULL && pfnECInitializeConverterAorW(ConverterName, DirectionForward, NormalizeOutput) != S_OK)
		{
			// this means it's no longer in the repository, so clear it out and let them configure it from scratch
			ConverterName.Empty();
			DirectionForward = TRUE;
			NormalizeOutput = 0;	// NormalizeFlags_None;
			m_strConverterDescription.Empty();
			gpApp->m_bECConnected = FALSE;
		}
		else
		{
			TCHAR szDescription[1000 + 1];
			szDescription[0] = NULL; // whm added
			if (pfnECConverterDescriptionAorW != NULL)
			{
				pfnECConverterDescriptionAorW(ConverterName, szDescription, 1000);
				m_strConverterDescription = szDescription;
			}
		}
#endif	// end of if USE_SIL_CONVERTERS
	}
	pEditSILConverterName->SetValue(ConverterName);
	pEditSILConverterInfo->SetValue(m_strConverterDescription); // (read only)
}

CSilConverterSelectDlg::~CSilConverterSelectDlg() // destructor
{
	
}

void CSilConverterSelectDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

}

// event handling functions

void CSilConverterSelectDlg::OnBnClickedBtnSelectSilconverter(wxCommandEvent& event)
{
#ifdef USE_SIL_CONVERTERS
	// whm added 30Dec08 for SIL Converters support
	typedef int (wxSTDCALL *wxECSelConverterType)(wxChar*,int&,int&);
	wxECSelConverterType pfnECSelectConverterAorW = (wxECSelConverterType)NULL;
	// whm Note: The EncConverterSelectConverter() function in ECDriver.dll only has A and W forms so we must
	// call GetSymbolAorW() instead of GetSymbol() here.
	pfnECSelectConverterAorW = (wxECSelConverterType)ecDriverDynamicLibrary.GetSymbolAorW(FUNC_NAME_EC_SELECT_CONVERTER_AW);
	typedef int (wxSTDCALL *wxECConverterDescrType)(const wxChar*,wxChar*,int);
	wxECConverterDescrType pfnECConverterDescriptionAorW = (wxECConverterDescrType)NULL;
	// whm Note: The EncConverterConverterDescription() function in ECDriver.dll only has A and W forms so we must
	// call GetSymbolAorW() instead of GetSymbol() here.
	pfnECConverterDescriptionAorW = (wxECConverterDescrType)ecDriverDynamicLibrary.GetSymbolAorW(FUNC_NAME_EC_CONVERTER_DESCRIPTION_AW);
	
	TCHAR szConverterName[1000];
	szConverterName[0] = NULL; // whm added
	if (pfnECSelectConverterAorW != NULL && pfnECSelectConverterAorW(szConverterName, DirectionForward, NormalizeOutput) == S_OK)
	{
		ConverterName = szConverterName;
		TCHAR szDescription[1000 + 1];
		szDescription[0] = NULL; // whm added
		if (pfnECConverterDescriptionAorW != NULL)
		{
			pfnECConverterDescriptionAorW(ConverterName, szDescription, 1000);
			m_strConverterDescription = szDescription;
		}

		pEditSILConverterName->SetValue(ConverterName);
		pEditSILConverterInfo->SetValue(m_strConverterDescription); // (read only)
	}
	else
	{
		// either something didn't work or the user clicked Cancel, which is the functional equivalent of
		// "Unload" for Consistent Changes), so just empty everything and set it to the default.
		OnBnClickedBtnClearSilconverter(event);
	}
#else
	int id;
	id = event.GetId(); // to avoid warning "unreferenced formal parameter"
	id = id; // avoid warning
#endif	// end of if USE_SIL_CONVERTERS
}

void CSilConverterSelectDlg::OnBnClickedBtnClearSilconverter(wxCommandEvent& WXUNUSED(event))
{
    ConverterName.Empty();
    DirectionForward = TRUE;
	NormalizeOutput = 0;	// NormalizeFlags_None;
	m_strConverterDescription.Empty();
	gpApp->m_bECConnected = FALSE;
	
    //UpdateData(false);
	pEditSILConverterName->SetValue(ConverterName);
	pEditSILConverterInfo->SetValue(m_strConverterDescription); // (read only)
}

#define strCaption  _T("SilEncConverters Error")

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CSilConverterSelectDlg::OnOK(wxCommandEvent& event) 
{
	
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

