/////////////////////////////////////////////////////////////////////////////
// Project:     Adapt It wxWidgets
// File Name:   SilConverterSelectDlg.h
// Author:      Bill Martin
// Created:     28 April 2006
// Revised:		28 April 2006
// Copyright:   2004 Bruce Waters, Bill Martin, SIL
// Description: This is the header file for the CSilConverterSelectDlg class. 
// The CSilConverterSelectDlg class uses COM/OLE functions to enable the SILConverters
// to access Adapt It's KBs and function as a cc-like filter for source phrases
// entered in the phrase box.
// The CSilConverterSelectDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef SilConverterSelectDlg_h
#define SilConverterSelectDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "SilConverterSelectDlg.h"
#endif

//#ifdef USE_SIL_CONVERTERS
////#include <stdafx.h>
////#include "C:\Program Files\Microsoft Visual Studio 8\VC\atlmfc\include\atlcom.h"
////#include <afxole.h> // TODO: afxole.h was also in StdAfx.h - is this needed here ???
////#include <atlcom.h> // for SIL Converters
//
//// MFC Note: the following was added to the MFC app's StdAfx.h file for Bob Eaton's 
//// SILConverters support.
//// rde: include the interfaces for SilEncConverters from the typelibrary (these
//// don't change much, so they're good for the pre-compiled header file)
//#import "SilEncConverters22.tlb"  raw_interfaces_only
//using namespace SilEncConverters22;
//wxAutomationObject IECs, IEC;
////typedef CComPtr<IEncConverters> IECs;
////typedef CComPtr<IEncConverter>  IEC;
//#endif

class CSilConverterSelectDlg : public AIModalDialog
{
public:
#ifdef USE_SIL_CONVERTERS
	CSilConverterSelectDlg(
		const wxString&  strConverterName, 
        bool            bDirectionForward,
        NormalizeFlags  eNormalizeOutput,
        IEC&            aEC,
		wxWindow* parent); // constructor
#else
	CSilConverterSelectDlg(
		const wxString&  strConverterName, 
        bool            bDirectionForward,
		wxWindow* parent); // constructor
#endif
	virtual ~CSilConverterSelectDlg(void); // destructor
	//enum { IDD = IDD_DLG_SILCONVERTERS };

    wxString         ConverterName;
    bool            DirectionForward;
	wxTextCtrl*		pEditSILConverterName; // IDC-ED_SILCONVERTER_NAME
	wxTextCtrl*		pEditSILConverterInfo; // IDC_ED_SILCONVERTER_INFO (read only)
	wxButton*		pBtnUnload; // IDC_BTN_CLEAR_SILCONVERTER
#ifdef USE_SIL_CONVERTERS
    NormalizeFlags  NormalizeOutput;
#endif
	// other methods
    void OnBnClickedBtnClearSilconverter(wxCommandEvent& WXUNUSED(event));

protected:
    wxString m_strConverterDescription;
#ifdef USE_SIL_CONVERTERS
    IEC&    m_aEC;
#endif

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
    void OnBnClickedBtnSelectSilconverter(wxCommandEvent& event);

private:
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

#ifdef USE_SIL_CONVERTERS
extern bool ProcessHResult(HRESULT hr, IUnknown* p, const IID& iid);
#endif

#endif /* SilConverterSelectDlg_h */
