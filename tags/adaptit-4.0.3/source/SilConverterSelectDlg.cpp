/////////////////////////////////////////////////////////////////////////////
// Project:     Adapt It wxWidgets
// File Name:   SilConverterSelectDlg.cpp
// Author:      Bill Martin
// Created:     28 April 2006
// Revised:		28 April 2006
// Copyright:   2004 Bruce Waters, Bill Martin, SIL
// Description: This is the implementation file for the CSilConverterSelectDlg class. 
// The CSilConverterSelectDlg class uses COM/OLE functions to enable the SILConverters
// to access Adapt It's KBs and function as a cc-like filter for source phrases
// entered in the phrase box.
// The CSilConverterSelectDlg class is derived from wxDialog.
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

#ifdef USE_SIL_CONVERTERS
#include "wx/msw/ole/automtn.h"
#endif

#include "Adapt_It.h"
#include "SilConverterSelectDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // for rapid access to the app class

// event handler table
BEGIN_EVENT_TABLE(CSilConverterSelectDlg, AIModalDialog)
	EVT_INIT_DIALOG(CSilConverterSelectDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
    EVT_BUTTON(IDC_BTN_SELECT_SILCONVERTER, CSilConverterSelectDlg::OnBnClickedBtnSelectSilconverter)
    EVT_BUTTON(IDC_BTN_CLEAR_SILCONVERTER, CSilConverterSelectDlg::OnBnClickedBtnClearSilconverter)
END_EVENT_TABLE()


#ifdef USE_SIL_CONVERTERS
CSilConverterSelectDlg::CSilConverterSelectDlg(
						const wxString&  strConverterName, 
						bool            bDirectionForward,
						NormalizeFlags  eNormalizeOutput,
						IEC&            aEC, 
						wxWindow* parent) // dialog constructor
#else
CSilConverterSelectDlg::CSilConverterSelectDlg(
						const wxString&  strConverterName, 
						bool            bDirectionForward,
						wxWindow* parent) // dialog constructor
#endif
	: AIModalDialog(parent, -1, _("Choose SIL Converter"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
				, ConverterName(strConverterName)
				, DirectionForward(bDirectionForward)
#ifdef USE_SIL_CONVERTERS
				, NormalizeOutput(eNormalizeOutput)
				, m_aEC(aEC)
#endif
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	SilConvertersDlgFunc(this, TRUE, TRUE);
	// The declaration is: SilConvertersDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

	pEditSILConverterName = (wxTextCtrl*)FindWindowById(IDC_ED_SILCONVERTER_NAME);
	wxASSERT(pEditSILConverterName != NULL);
	pEditSILConverterInfo = (wxTextCtrl*)FindWindowById(IDC_ED_SILCONVERTER_INFO); // (read only)
	wxASSERT(pEditSILConverterInfo != NULL);
	pBtnUnload = (wxButton*)FindWindowById(IDC_BTN_CLEAR_SILCONVERTER);
	wxASSERT(pBtnUnload != NULL);

	// other attribute initializations
    // if one is already configured, then try to get the details (don't cache it in case it change):
    if( !ConverterName.IsEmpty() )
    {
 #ifdef USE_SIL_CONVERTERS
		// if the converter isn't already connected...
		// 
		// whm Note: We could just call the View's InitializeEC() function, but we don't want any error
		// messages issued here, apparently. 
		// The local m_aEC is initialized from the App's m_aEC when this dialog constructor is called
		// (see parameter initializer list above), so it will initially be the same as the App's value,
		// but will get initialized here if needed.

		// !!! up to here !!! Can't use ! with m_aEC
		if (!gpApp->m_bECConnected) // if( !m_aEC )
		{
			//CWaitCursor x;
			// first get the repository object.
			IECs    pECs;
			//pECs.CoCreateInstance(L"SilEncConverters22.EncConverters");
			//if( !!pECs )
			// Create the COM object from its prog id
			if (pECs.CreateObject(_T("SilEncConverters22.EncConverters"))) //if (pECs.CreateInstance(_T("SilEncConverters22.EncConverters")))
			{
				// we have the repository... now ask for the configured converter
				// MFC code:
				// COleVariant varName(ConverterName);
				// pECs->get_Item(varName, &m_aEC);
				// 
				// whm: Up to this point pECs was being used as a class object employing "dot" syntax 
				// to access methods of MFC's CComPtrBase class using calls such as pECs.CoCreateInstance(). 
				// But notice in the MFC line above that pECs is now being used with -> syntax, which
				// according to the CComPtrBase docs is a "pointer-to-members operator" which is used
				// "to call a method in a class pointed to by the CComPtrBase object." So, pECs now is
				// functioning as a pointer to the get_Item() method of the COM object itself.
				// In other words, the pECs pointer points to the "collection" of SIL Converters and the 
				// collection's get_Item() method is now called to retrieve a pointer to an individual 
				// converter in the collection whose pointer is associated with varName, and which 
				// get_Item() assigns by reference to m_aEC (a pointer to an IEC COM object).
				// 
				// The XYDispDriver class does not have a "pointer-to-members operator" but we should 
				// be able to call the get_Item() method by using the InvokeMethod() call.
				// 
				// TODO: Unfortunately, tracing through InvokeMethod() the first thing it does is to
				// call FindMethod() [which calls FindDispInfo()] to check for the existence of the "get_Item" method name, but it 
				// cannot find it for some unknown reason. Calling FindProperty() also does not work, 
				// so I'll have to experiment more to find what works. Note: The example used in Liu's 
				// article shows that it is not necessary to pass the arguments to the method called 
				// as variants - the InvokeMethod() and InvokeMethodV which it calls takes care of 
				// the handling of arguments in variant form.
				// Tracing through the code here are all the method and property names that are
				// available to FindMethod() and FindDispInfo():
				//0	    QueryInterface
				//1	    AddRef
				//2	    Release
				//3	    GetTypeInfoCount
				//4	    GetTypeInfo
				//5	    GetIdsOfNames
				//6	    Invoke
				//7	    Item
				//8	    Add
				//9	    AddConversionMap
				//10	AddImplementation
				//11	AddAlias
				//12	AddCompoundConverterStep
				//13	AddFont
				//14	AddUnicodeFontEncoding
				//15	AddFontMapping
				//16	AddFallbackConverter
				//17	Remove
				//18	RemoveNonPersist
				//19	RemoveAlias
				//20	RemoveImplementation
				//21	Count
				//22	ByProcessType
				//23	ByFontName
				//24	ByEncodingID
				//25	EnumByProcessType
				//26	Attributes
				//27	Encodings
				//28	Mappings
				//29	Fonts
				//30	Values
				//31	Keys
				//32	Clear
				//33	ByImplementationType
				//34	BuildConverterSpecName
				//35	UnicodeEncodingFormConvert
				//36	RepositoryFile
				//37	CodePage
				//38	GetFontMapping
				//39	AutoConfigure
				//40	AutoSelect
				//41	NewEncConverterByImplementationType
				// The above seems to indicate that get_Item() is not itself a valid method name for
				// accessing the EncConverters repository.
				
				// !!!!!!!!!!!!!!!!!!!!!!!!! testing below !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

				// In XYDispDriver it seems that it shouldn't necessary to make the parameters into variants before
				// calling the XYDispDriver methods, but Bob suggested doing so (see below).
				// whm: BobE 21Aug08 hacked the code to get InvokePropertyWithArgument() to work as follows:
				static BYTE parms[] = "\x0C" ; // 12 is the enum value of VT_VARIANT
				VARIANT var;
				var.vt = VT_BSTR; // VT_BSTR has an enum value of 8
				var.bstrVal = ::SysAllocString(ConverterName.c_str());
				VARIANT* pEC = pECs.InvokePropertyWithArgument(_T("Item"), parms, &var); // testing ConverterName is "Target Word Guesser for Lugungu to English adaptations"
				
				// MFC code:
				// COleVariant varName(ConverterName);
				// pECs->get_Item(varName, &m_aEC);
				
				if (m_aEC.Attach(pEC->pdispVal))
				{
					wxASSERT(m_aEC.GetDispatch() != NULL);
					// whm: also set the App's m_bECConnected
					gpApp->m_bECConnected = TRUE;
				}

				// temporary !!!
				gpApp->m_bECConnected = TRUE;
				// temporary !!!
				
				// TODO: Ok, the above works, now figure out now to get the pointer out of the pecs
				// VARIANT to point to m_aEC, so we can use SetProperty to call put_DirectionForward 
				// and put_NormalizeOutput below. Once we do that, we can do the original test if
				// (!m_aEC) below too.
				
				// whm testing wxVariant below: In order to use wxVariant, we would have to rewrite
				// part of XYDispDriver so that it also uses wxVariant instead of VARIANT.
				//wxVariant wxvar;
				//wxvar = ConverterName;
				//VARIANT* wxpecs = pECs.InvokePropertyWithArgument(_T("Item"), parms, &wxvar);
				// testing wxVariant above

				// !!!!!!!!!!!!!!!!!!!!!!! testing above !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

				//VARIANT* pOutput = pECs.InvokeMethod(_T("get_Item"),ConverterName, &m_aEC);
				//if (pOutput && pOutput->boolVal)
				//{
				//	wxASSERT(m_aEC.GetDispatch() != NULL);
				//	// whm: also set the App's m_bECConnected
				//	gpApp->m_bECConnected = TRUE;
				//}
			}
		}

		// check again (since we might have just initialized it).
		if (!gpApp->m_bECConnected) //if( !m_aEC )
		{
			// this means it's no longer in the repository, so clear it out and let them configure it from scratch
			ConverterName.Empty();
			DirectionForward = TRUE;
			NormalizeOutput = NormalizeFlags_None;
		}
		else
		{
			// initialize the direction and the output normalization form from what was originally configured
			bool bPutOK1, bPutOK2;
			bPutOK1 = m_aEC.SetProperty(_T("put_DirectionForward"),wxVariant(DirectionForward)); //m_aEC->put_DirectionForward((DirectionForward) ? VARIANT_TRUE : VARIANT_FALSE);
			bPutOK2 = m_aEC.SetProperty(_T("put_NormalizeOutput"),wxVariant(NormalizeOutput)); //m_aEC->put_NormalizeOutput(NormalizeOutput);
			// TODO: error checking of bPutOK1 and bPutOK2 here ???
			
			// get the "details" for display
			//CComBSTR str;
			wxString str;
			m_aEC.InvokeMethod(_T("get_ToString"), &str); //m_aEC.CallMethod(_T("get_ToString"), 1, wxVariant(str)); //m_aEC->get_ToString(&str);
			m_strConverterDescription = str;
		}
#endif
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
    //CWaitCursor x;
    IECs    pECs;
    //pECs.CoCreateInstance(L"SilEncConverters22.EncConverters");
    if (pECs.CreateObject(_T("SilEncConverters22.EncConverters"))) //if (pECs.CreateInstance(_T("SilEncConverters22.EncConverters"))) //if( !!pECs )
    {
        // if the converter *was* configured, then release it (since we're going to fetch a new one)
		IDispatch* pIDisp;
		pIDisp = m_aEC.GetDispatch();
		if (!pIDisp) //if( !!m_aEC )
            m_aEC.Clear(); //m_aEC.Detach();

		// now that we have the repository... now ask for the Configuration UI
        // For the non-roman version, we're *probably* only looking for Unicode_to(_from)_Unicode converters, 
        // however, just in case the user wants to do a Legacy->Unicode (e.g. encoding conversion) simultaneously, 
        // leave the type ambiguous to allow for this possibility. For the Ansi version, though, Unicode is out, 
        // so limit the display of converters to only those that make sense.
        ConvType eConvType = 
#ifdef  _UNICODE
            ConvType_Unknown;   // this means show all converters
#else
            ConvType_Legacy_to_Legacy;  // this means show only Legacy_to_(from_)Legacy converters
#endif

        // call the self-selection UI (NOTE: only in SC 2.2 and newer!)
        //if(     ProcessHResult(pECs->AutoSelect(eConvType, &m_aEC), pECs, __uuidof(IEncConverters))
        //    &&  !!m_aEC )
        if(pECs.InvokeMethod(_T("AutoSelect"),eConvType,&m_aEC) //ProcessHResult(pECs->AutoSelect(eConvType, &m_aEC), pECs, __uuidof(IEncConverters))
            &&  (m_aEC.GetDispatch())) //!!m_aEC )
        {
            // get the name of the configured converter
			CComBSTR str;
            m_aEC.InvokeMethod(_T("get_Name"),&str); //->get_Name(&str);  //m_aEC->get_Name(&str);  
			ConverterName = str;
            
            // get the direction
            VARIANT_BOOL bVal = VARIANT_TRUE;
            m_aEC.InvokeMethod(_T("get_DirectionForward"),&bVal); //m_aEC->get_DirectionForward(&bVal); 
            DirectionForward = (bVal ==  VARIANT_FALSE) ? FALSE : TRUE;

            // get the normalize output flag
            m_aEC.InvokeMethod(_T("get_NormalizeOutput"),&NormalizeOutput); //m_aEC->get_NormalizeOutput(&NormalizeOutput);

            // get the "rest" of the converter details to display
            m_aEC.InvokeMethod(_T("get_ToString"),&str); //m_aEC->get_ToString(&str);      
            m_strConverterDescription = str;
            //UpdateData(false);
 			pEditSILConverterName->SetValue(ConverterName);
			pEditSILConverterInfo->SetValue(m_strConverterDescription); // (read only)
           return; // don't fall thru
		}
	}
    else
    {
		// IDS_SILCONVERTERS_NO_AVAILABLE
        wxMessageBox(_("Unable to launch the SIL Converters Selection Dialog! Are you using SIL Converters version v 2.2 or greater? Otherwise, you need to re-install or reboot"),_T(""), wxICON_WARNING);
    }
#endif

    // either something didn't work or the user clicked Cancel, which is the functional equivalent of
    // "Unload" for Consistent Changes), so just empty everything and set it to the default.
    OnBnClickedBtnClearSilconverter(event);
}

void CSilConverterSelectDlg::OnBnClickedBtnClearSilconverter(wxCommandEvent& WXUNUSED(event))
{
    ConverterName.Empty();
    DirectionForward = TRUE;
 #ifdef USE_SIL_CONVERTERS
    NormalizeOutput = NormalizeFlags_None;
#endif
	m_strConverterDescription.Empty();
	
    //UpdateData(false);
	pEditSILConverterName->SetValue(ConverterName);
	pEditSILConverterInfo->SetValue(m_strConverterDescription); // (read only)
}

#define strCaption  _T("SilEncConverters Error")

#ifdef USE_SIL_CONVERTERS
// here's a helper function for displaying useful error messages
bool ProcessHResult(HRESULT hr, IUnknown* p, const IID& iid)
{
    if( hr == S_OK )
        return true;

    // otherwise, throw a _com_issue_errorex and catch it (so we can use it to get
    //  the error description out of it for us.
    try 
    {
        _com_issue_errorex(hr, p, iid);
    }
    catch(_com_error & er) 
    {
        if( er.Description().length() > 0)
        {
            ::MessageBox(NULL, er.Description(), strCaption, MB_OK);
        } 
        else 
        {
            ::MessageBox(NULL, er.ErrorMessage(), strCaption, MB_OK);
        }
    }

    return false;
}
#endif

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CSilConverterSelectDlg::OnOK(wxCommandEvent& event) 
{
	
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

