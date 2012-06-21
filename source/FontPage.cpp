/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FontPage.cpp
/// \author			Bill Martin
/// \date_created	3 May 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CFontPageWiz and CFontPagePrefs classes.
/// A third class CFontPageCommon consists of the elements and methods in common to the above
/// two classes.
/// The CFontPageWiz and CFontPagePrefs classes create a panel that is used in both the 
/// Start Working wizard and the Edit Preferenced notebook dialog. The Wizard and Preferences
/// notebook, allow the user to choose/edit the three main fonts (source, target and navigation) 
/// for a project. The font panel also has buttons for selecting font colors for the source, 
/// target, navigation, special and retranslation text. The interface resources for the font
/// panel are defined in FontsPageFunc() which was developed and is maintained by wxDesigner.
/// The CFontPageWiz class is derived from wxWizardPage and the CFontPagePrefs class is derived
/// from wxPanel. This three-class design was implemented because wxNotebook under Linux/GTK will 
/// not display pages which are derived from wxWizardPage.
/// \derivation		CFontPageWiz is derived from wxWizardPage, CFontPagePrefs from wxPanel, and CFontPageCommon from wxPanel.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in FontPage.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "FontPage.h"
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

// whm 14Jun12 modified to #include <wx/fontdate.h> for wxWidgets 2.9.x and later
#if wxCHECK_VERSION(2,9,0)
#include <wx/fontdata.h>
#endif

#include <wx/fontdlg.h>
#include <wx/valgen.h> // for wxGenericValidator
#include <wx/font.h>
#include <wx/fontenum.h>

#include <wx/colordlg.h>
#include <wx/wizard.h>
#include "Adapt_It.h"
#include "FontPage.h"
#include "LanguagesPage.h"

#include "KB.h"
#include "Adapt_ItView.h"
#include "ProjectPage.h"
#include "PunctCorrespPage.h"
#include "AdaptitConstants.h" // for MIN_FONT_SIZE and MAX_FONT_SIZE
#include "SetEncodingDlg.h"
#include "Pile.h"
#include "Layout.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// This global is defined in Adapt_It.cpp.
//extern CProjectPage* pProjectPage;

/// This global is defined in Adapt_It.cpp.
extern CLanguagesPage* pLanguagesPage;

/// This global is defined in Adapt_It.cpp.
extern CPunctCorrespPageWiz* pPunctCorrespPageWiz;

/// This global is defined in Adapt_It.cpp.
extern struct fontInfo SrcFInfo;

/// This global is defined in Adapt_It.cpp.
extern struct fontInfo TgtFInfo;

/// This global is defined in Adapt_It.cpp.
extern struct fontInfo NavFInfo;

bool gbLTRLayout = TRUE; // whm added initialization to TRUE here in global space
bool gbRTLLayout = FALSE; // whm added initialization to FALSE here in global space

void CFontPageCommon::DoSetDataAndPointers()
{
	tempSourceFontName = _T("");
	tempTargetFontName = _T("");
	tempNavTextFontName = _T("");
	tempSourceSize = 0;
	tempTargetSize = 0;
	tempNavTextSize = 0;
	tempSourceFontStyle = wxFONTSTYLE_NORMAL;
	tempTargetFontStyle = wxFONTSTYLE_NORMAL;
	tempNavTextFontStyle = wxFONTSTYLE_NORMAL;
	tempSourceFontWeight = wxFONTWEIGHT_NORMAL;
	tempTargetFontWeight = wxFONTWEIGHT_NORMAL;
	tempNavTextFontWeight = wxFONTWEIGHT_NORMAL;


	//wxColour sysColorBtnFace = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	// Initialize the font page edit controls to be read only with system window
	// background color
	// Note: I'm having trouble getting wxGenericValidator to work with dialogs
	// such as this one which have nested controls, so we'll do data transfer
	// manually.
	pSrcFontNameBox = (wxTextCtrl*)FindWindowById(IDC_SOURCE_FONTNAME);
	pSrcFontNameBox->SetEditable(FALSE);
	pSrcFontNameBox->SetBackgroundColour(gpApp->sysColorBtnFace);

	pTgtFontNameBox = (wxTextCtrl*)FindWindowById(IDC_TARGET_FONTNAME);
	pTgtFontNameBox->SetEditable(FALSE);
	pTgtFontNameBox->SetBackgroundColour(gpApp->sysColorBtnFace);

	pNavFontNameBox = (wxTextCtrl*)FindWindowById(IDC_NAVTEXT_FONTNAME);// whm added 10Apr04 not in MFC
	pNavFontNameBox->SetEditable(FALSE);
	pNavFontNameBox->SetBackgroundColour(gpApp->sysColorBtnFace);

	pSrcFontSizeBox = (wxTextCtrl*)FindWindowById(IDC_SOURCE_SIZE);
	pSrcFontSizeBox->SetEditable(FALSE);
	pSrcFontSizeBox->SetBackgroundColour(gpApp->sysColorBtnFace);

	pTgtFontSizeBox = (wxTextCtrl*)FindWindowById(IDC_TARGET_SIZE);
	pTgtFontSizeBox->SetEditable(FALSE);
	pTgtFontSizeBox->SetBackgroundColour(gpApp->sysColorBtnFace);

	pNavFontSizeBox = (wxTextCtrl*)FindWindowById(IDC_NAVTEXT_SIZE);// whm added 10Apr04 not in MFC
	pNavFontSizeBox->SetEditable(FALSE);
	pNavFontSizeBox->SetBackgroundColour(gpApp->sysColorBtnFace);

	pTextCtrlAsStaticFontpage = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_FONTPAGE);
	wxASSERT(pTextCtrlAsStaticFontpage != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	//pTextCtrlAsStaticFontpage->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticFontpage->SetBackgroundColour(gpApp->sysColorBtnFace);

	// hide the RTL Reading checkboxes, if this is an ANSI build
	if (!gpApp->m_bShowRTL_GUI)
	{
		// its an ANSI build, so get each checkbox control and hide it
		wxCheckBox* pCheck = (wxCheckBox*)FindWindowById(IDC_CHECK_SRC_RTL);
		pCheck->Hide();
		pCheck = (wxCheckBox*)FindWindowById(IDC_CHECK_TGT_RTL);
		pCheck->Hide();
		pCheck = (wxCheckBox*)FindWindowById(IDC_CHECK_NAVTEXT_RTL);
		pCheck->Hide();
	}

	pSrcChangeEncodingBtn = (wxButton*)FindWindowById(ID_BUTTON_CHANGE_SRC_ENCODING);
	wxASSERT(pSrcChangeEncodingBtn != NULL);
	pTgtChangeEncodingBtn = (wxButton*)FindWindowById(ID_BUTTON_CHANGE_TGT_ENCODING);
	wxASSERT(pTgtChangeEncodingBtn != NULL);
	pNavChangeEncodingBtn = (wxButton*)FindWindowById(ID_BUTTON_CHANGE_NAV_ENCODING);
	wxASSERT(pNavChangeEncodingBtn != NULL);

	// Hide the Change Encoding buttons for Unicode version since they should appear only
	// in the ANSI build
#ifdef _UNICODE
	pSrcChangeEncodingBtn->Hide();
	pTgtChangeEncodingBtn->Hide();
	pNavChangeEncodingBtn->Hide();
#endif

}

// the following are common functions - used by the CFontPageWiz class and the CFontPagePrefs class
void CFontPageCommon::DoInit()
{
	gpApp->m_pLayout->m_bFontInfoChanged = FALSE;

	// Load font page member data into text controls
	// the following font page text controls had their SetEditable to FALSE and
	// their validators set in the constructor above.
	tempSourceFontName = gpApp->m_pSourceFont->GetFaceName();
	pSrcFontNameBox->SetValue(tempSourceFontName);

	tempTargetFontName = gpApp->m_pTargetFont->GetFaceName();
	pTgtFontNameBox->SetValue(tempTargetFontName);

	tempNavTextFontName = gpApp->m_pNavTextFont->GetFaceName();
	pNavFontNameBox->SetValue(tempNavTextFontName);

	tempSourceSize = gpApp->m_pSourceFont->GetPointSize();
	wxString strTemp;
	strTemp.Printf(_T("%d"),tempSourceSize);
	pSrcFontSizeBox->SetValue(strTemp);

	tempTargetSize = gpApp->m_pTargetFont->GetPointSize();
	strTemp.Printf(_T("%d"),tempTargetSize);
	pTgtFontSizeBox->SetValue(strTemp);

	tempNavTextSize = gpApp->m_pNavTextFont->GetPointSize();
	strTemp.Printf(_T("%d"),tempNavTextSize);
	pNavFontSizeBox->SetValue(strTemp);

	tempSourceFontStyle = gpApp->m_pSourceFont->GetStyle();
	tempTargetFontStyle = gpApp->m_pTargetFont->GetStyle();
	tempNavTextFontStyle = gpApp->m_pNavTextFont->GetStyle();

	tempSourceFontWeight = (wxFontWeight)gpApp->m_pSourceFont->GetWeight();
	tempTargetFontWeight = (wxFontWeight)gpApp->m_pTargetFont->GetWeight();
	tempNavTextFontWeight = (wxFontWeight)gpApp->m_pNavTextFont->GetWeight();

	// Initialize the local temp colors from the App's values
	tempSourceColor = gpApp->m_sourceColor;
	tempTargetColor = gpApp->m_targetColor;
	tempNavTextColor = gpApp->m_navTextColor;

	// here on entry to InitDialog, the temp color values should be the same as those held
	// in the font data objects on the app
    // BEW 5Jun09, created local colour variables here in order to see the values when
    // debugging; I was getting an assert trip if I changed the navText color & reentered
    // Preferences, the assert 6 lines down tripped
	wxColour srcCol = gpApp->m_pSrcFontData->GetColour();
	wxColour tgtCol = gpApp->m_pTgtFontData->GetColour();
	wxColour navCol = gpApp->m_pNavFontData->GetColour();
	wxASSERT(tempSourceColor == srcCol);
	wxASSERT(tempTargetColor == tgtCol);
	wxASSERT(tempNavTextColor == navCol);

	tempSpecialTextColor = gpApp->m_specialTextColor;
	tempReTranslnTextColor = gpApp->m_reTranslnTextColor;
	tempTgtDiffsTextColor = gpApp->m_tgtDiffsTextColor;

	// Initialize the local temp fontdata vars from the App's values
	tempSrcFontData = *gpApp->m_pSrcFontData;
	tempTgtFontData = *gpApp->m_pTgtFontData;
	tempNavFontData = *gpApp->m_pNavFontData;
	
	// these "save" encoding values represent the font encodings upon entry 
	// to the font page (set in InitDialog):
	saveSrcFontEncoding = gpApp->m_pSourceFont->GetEncoding();
	saveTgtFontEncoding = gpApp->m_pTargetFont->GetEncoding();
	saveNavFontEncoding = gpApp->m_pNavTextFont->GetEncoding();

	// these "temp" encoding values start the same as the "save" values above
	// at fontPage InitDialog, but they will hold any changed encoding values
	// that the user may have made by calling the Set/View Encoding dialog on
	// one or more of the fonts. 
	// to the font page (set in InitDialog):
	tempSrcFontEncoding = gpApp->m_pSourceFont->GetEncoding();
	tempTgtFontEncoding = gpApp->m_pTargetFont->GetEncoding();
	tempNavFontEncoding = gpApp->m_pNavTextFont->GetEncoding();


#ifdef _RTL_FLAGS
	// Load the RTL checkboxes with current values
	wxCheckBox* pCheck;
	pCheck = (wxCheckBox*)FindWindowById(IDC_CHECK_SRC_RTL);
	pCheck->SetValue(gpApp->m_bSrcRTL);

	pCheck = (wxCheckBox*)FindWindowById(IDC_CHECK_TGT_RTL);
	pCheck->SetValue(gpApp->m_bTgtRTL);

	pCheck = (wxCheckBox*)FindWindowById(IDC_CHECK_NAVTEXT_RTL);
	pCheck->SetValue(gpApp->m_bNavTextRTL);

#endif
	pFontPageSizer->Layout(); // in case Encoding buttons hide/appear
}

void CFontPageCommon::DoButtonNavTextColor(wxWindow* parent)
{
	// change navigation text colour
	wxColourData colorData;
	colorData.SetColour(tempNavTextColor);
	colorData.SetChooseFull(TRUE);
	wxColourDialog colorDlg(parent,&colorData);
	if(colorDlg.ShowModal() == wxID_OK)
	{
		colorData = colorDlg.GetColourData();
		tempNavTextColor = colorData.GetColour();
		// whm added 5Jun09 to keep tempNavTextColor and tempNavFontData in sync
		tempNavFontData.SetColour(tempNavTextColor);
	}	
}

void CFontPageCommon::DoButtonRetranTextColor(wxWindow* parent)
{
	// change retranslation text colour
	wxColourData colorData;
	colorData.SetColour(tempReTranslnTextColor);
	colorData.SetChooseFull(TRUE);
	wxColourDialog colorDlg(parent,&colorData);
	if(colorDlg.ShowModal() == wxID_OK)
	{
		colorData = colorDlg.GetColourData();
		tempReTranslnTextColor = colorData.GetColour();
	}	
}

void CFontPageCommon::DoButtonTgtDiffsTextColor(wxWindow* parent)
{
	// change Target Text Differences colour (in target line)
	wxColourData colorData;
	colorData.SetColour(tempTgtDiffsTextColor);
	colorData.SetChooseFull(TRUE);
	wxColourDialog colorDlg(parent,&colorData);
	if(colorDlg.ShowModal() == wxID_OK)
	{
		colorData = colorDlg.GetColourData();
		tempTgtDiffsTextColor = colorData.GetColour();
	}	
}

void CFontPageCommon::DoButtonSourceTextColor(wxWindow* parent)
{
	wxColourData colorData;
	colorData.SetColour(tempSourceColor);
	colorData.SetChooseFull(TRUE);
	wxColourDialog colorDlg(parent,&colorData);
	if(colorDlg.ShowModal() == wxID_OK)
	{
		colorData = colorDlg.GetColourData();
		tempSourceColor = colorData.GetColour();
		// whm added 5Jun09 to keep tempSourceColor and tempSrcFontData in sync
		tempSrcFontData.SetColour(tempSourceColor);
	}	
}

void CFontPageCommon::DoButtonSpecTextColor(wxWindow* parent)
{
	wxColourData colorData;
	colorData.SetColour(tempSpecialTextColor);
	colorData.SetChooseFull(TRUE);
	wxColourDialog colorDlg(parent,&colorData);
	if(colorDlg.ShowModal() == wxID_OK)
	{
		colorData = colorDlg.GetColourData();
		tempSpecialTextColor = colorData.GetColour();
	}
}

void CFontPageCommon::DoButtonTargetTextColor(wxWindow* parent)
{
	wxColourData colorData;
	colorData.SetColour(tempTargetColor);
	colorData.SetChooseFull(TRUE);
	wxColourDialog colorDlg(parent,&colorData);
	if(colorDlg.ShowModal() == wxID_OK)
	{
		colorData = colorDlg.GetColourData();
		tempTargetColor = colorData.GetColour();
		// whm added 5Jun09 to keep tempTargetColor and tempTgtFontData in sync
		tempTgtFontData.SetColour(tempTargetColor);
	}	
}

void CFontPageCommon::DoSourceFontChangeBtn(wxWindow* parent)
{
	// set up initial values from the App
	tempSrcFontData.SetColour(gpApp->m_sourceColor);
	tempSrcFontData.SetInitialFont(*gpApp->m_pSourceFont);
	// Size must be between 6pt and 72pt.
	tempSrcFontData.SetRange(6,72);

	wxFontDialog fontDlg(parent,tempSrcFontData);
	if(fontDlg.ShowModal() == wxID_OK)
	{
		// Notes: 
		// I have not used the wxGenericValidator to transfer to/from
		// the controls and the member variables, but I manually update 
		// the data in the controls.

		// Validate the font data. Face name cannot be blank; 
		// update the read-only edit boxes to reflect user choices
		// Note: the user does not enter data by hand, only data
		// via the font chooser dialog, so data validation is less
		// important here.

		// Note: The wxFontData does not have any way to access the font encoding 
		// but there is an undocumented method of wxFont::GetEncoding() which does
		// get the current encoding of the font. It is equivalent to what is 
		// accessible in the Windows standard font dialog via drop down list called 
		// "Script". The "View/Set Encoding" buttons we provide and the ensuing dialog 
		// allow the user to actually set the encoding for the font (which is then 
		// reflected in the drop down "Script" list once the choice was saved in the 
		// fontPage).
		// The standard font dialog in Ubuntu Linux does not have a way to select
		// encodings.
		tempSrcFontData = fontDlg.GetFontData();
		// get the font 
		wxFont tempSrcFont = tempSrcFontData.GetChosenFont(); // destructor deletes the font returned here
		tempSourceColor = tempSrcFontData.GetColour();

		tempSourceFontName = tempSrcFont.GetFaceName();
		tempSrcFontEncoding = tempSrcFont.GetEncoding();
		pSrcFontNameBox->SetValue(tempSourceFontName);

		// For IDC_SOURCE_SIZE, IDC_TARGET_SIZE, and IDC_NAVTEXT_SIZE the SetRange() 
		// function called before the ShowModal() call above rejects any size values 
		// that are not between 6pt and 72pt
		tempSourceSize = tempSrcFont.GetPointSize();
		wxString strTemp;
		strTemp.Empty(); 
		strTemp << tempSourceSize;
		pSrcFontSizeBox->SetValue(strTemp);

		// get the style and weight too
		tempSourceFontStyle = tempSrcFont.GetStyle();
		tempSourceFontWeight = (wxFontWeight)tempSrcFont.GetWeight();
	}
}

void CFontPageCommon::DoTargetFontChangeBtn(wxWindow* parent)
{
	// See notes in DoSourceFontChangeBtn() above.
	// set up initial values from the App
	tempTgtFontData.SetColour(gpApp->m_targetColor);
	tempTgtFontData.SetInitialFont(*gpApp->m_pTargetFont);
	// Size must be between 6pt and 72pt.
	tempTgtFontData.SetRange(6,72);

	wxFontDialog fontDlg(parent,tempTgtFontData);
	if(fontDlg.ShowModal() == wxID_OK)
	{
		// See notes in DoSourcetFontChangeBtn() handler above.
		// update the read-only edit boxes to reflect user choices
		tempTgtFontData = fontDlg.GetFontData();
		// get the font 
		wxFont tempTgtFont = tempTgtFontData.GetChosenFont(); // destructor deletes the font returned here
		tempTargetColor = tempTgtFontData.GetColour();

		tempTargetFontName = tempTgtFont.GetFaceName();
		tempTgtFontEncoding = tempTgtFont.GetEncoding();
		pTgtFontNameBox->SetValue(tempTargetFontName);

		tempTargetSize = tempTgtFont.GetPointSize();
		wxString strTemp;
		strTemp.Empty();
		strTemp << tempTargetSize;
		pTgtFontSizeBox->SetValue(strTemp);

		// get the style and weight too
		tempTargetFontStyle = tempTgtFont.GetStyle();
		tempTargetFontWeight = (wxFontWeight)tempTgtFont.GetWeight();
	}
}

void CFontPageCommon::DoNavTextFontChangeBtn(wxWindow* parent)
{
	// See notes in DoSourceFontChangeBtn() above.
	// set up initial values from the App
	tempNavFontData.SetColour(gpApp->m_navTextColor);
	tempNavFontData.SetInitialFont(*gpApp->m_pNavTextFont);
	// Size must be between 6pt and 72pt.
	tempNavFontData.SetRange(6,72);

	wxFontDialog fontDlg(parent,tempNavFontData);
	if(fontDlg.ShowModal() == wxID_OK)
	{
		// See notes in DoSourcetFontChangeBtn() handler above.
		// update the read-only edit boxes to reflect user choices
		tempNavFontData = fontDlg.GetFontData();
		// get the font 
		wxFont tempNavFont = tempNavFontData.GetChosenFont(); // destructor deletes the font returned here
		tempNavTextColor = tempNavFontData.GetColour();

		tempNavTextFontName = tempNavFont.GetFaceName();
		tempNavFontEncoding = tempNavFont.GetEncoding();
		pNavFontNameBox->SetValue(tempNavTextFontName);

		tempNavTextSize = tempNavFont.GetPointSize();
		wxString strTemp;
		strTemp.Empty();
		strTemp << tempNavTextSize;
		pNavFontSizeBox->SetValue(strTemp);

		// get the style and weight too
		tempNavTextFontStyle = tempNavFont.GetStyle();
		tempNavTextFontWeight = (wxFontWeight)tempNavFont.GetWeight();
	}
}

void CFontPageCommon::DoButtonChangeSrcEncoding(wxWindow* parent)
{
	CSetEncodingDlg dlg(parent);
	dlg.Centre();
	dlg.langFontName = _("Source");
	if(dlg.ShowModal() == wxID_OK)
	{
		// See notes in DoSourcetFontChangeBtn() handler above.
		// get updated settings for font encoding, face name and family
		// and apply these to the temporary font setting here in the fontPage
		if (dlg.fontEncodingSelected != dlg.thisFontsCurrEncoding)
		{
			// user selected a different font encoding in the Set/View Encoding dialog
			tempSrcFontEncoding = dlg.fontEncodingSelected;
			// Note: in the OnOK() handlers the "temp" encodings will
			// be compared to the "save" encodings to see if an encoding
			// change needs to be made when the OK button is selected
		}
		// it is sufficient to simply change the face name in the edit box here
		// the actual change of the font's face name occurs in the OnOK() handlers
		pSrcFontNameBox->ChangeValue(dlg.fontFaceNameSelected);
		// ignore the fontFamilySelected value (???)
		// leave the point size the same as its previous setting
		// leave the font color the same as its previous setting
		// don't made any additional changes to fontPgCommon.tempSrcFontData
	}
}

void CFontPageCommon::DoButtonChangeTgtEncoding(wxWindow* parent)
{
	CSetEncodingDlg dlg(parent);
	dlg.Centre();
	dlg.langFontName = _("Target");
	if(dlg.ShowModal() == wxID_OK)
	{
		// get updated settings for font encoding, face name and family
		// and apply these to the temporary font setting here in the fontPage
		if (dlg.fontEncodingSelected != dlg.thisFontsCurrEncoding)
		{
			// user selected a different font encoding in the Set/View Encoding dialog
			tempTgtFontEncoding = dlg.fontEncodingSelected;
			// Note: in the OnOK() handlers the "temp" encodings will
			// be compared to the "save" encodings to see if an encoding
			// change needs to be made when the OK button is selected
		}
		// it is sufficient to simply change the face name in the edit box here
		// the actual change of the font's face name occurs in the OnOK() handlers
		pTgtFontNameBox->ChangeValue(dlg.fontFaceNameSelected);
		// ignore the fontFamilySelected value (???)
		// leave the point size the same as its previous setting
		// leave the font color the same as its previous setting
		// don't made any additional changes to fontPgCommon.tempTgtFontData
	}
}

void CFontPageCommon::DoButtonChangeNavEncoding(wxWindow* parent)
{
	CSetEncodingDlg dlg(parent);
	dlg.Centre();
	dlg.langFontName = _("Navigation Text");
	if(dlg.ShowModal() == wxID_OK)
	{
		// get updated settings for font encoding, face name and family
		// and apply these to the temporary font setting here in the fontPage
		if (dlg.fontEncodingSelected != dlg.thisFontsCurrEncoding)
		{
			// user selected a different font encoding in the Set/View Encoding dialog
			tempNavFontEncoding = dlg.fontEncodingSelected;
			// Note: in the OnOK() handlers the "temp" encodings will
			// be compared to the "save" encodings to see if an encoding
			// change needs to be made when the OK button is selected
		}
		// it is sufficient to simply change the face name in the edit box here
		// the actual change of the font's face name occurs in the OnOK() handlers
		pNavFontNameBox->ChangeValue(dlg.fontFaceNameSelected);
		// ignore the fontFamilySelected value (???)
		// leave the point size the same as its previous setting
		// leave the font color the same as its previous setting
		// don't made any additional changes to fontPgCommon.tempNavFontData
	}
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! end of common functions !!!!!!!!!!!!!!!!!!!!!!!!!!1

// the CFontPageWiz class //////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS( CFontPageWiz, wxWizardPage )

// event handler table
BEGIN_EVENT_TABLE(CFontPageWiz, wxWizardPage)
	EVT_INIT_DIALOG(CFontPageWiz::InitDialog)
    EVT_WIZARD_PAGE_CHANGING(-1, CFontPageWiz::OnWizardPageChanging) // handles MFC's OnWizardNext() and OnWizardBack
    EVT_WIZARD_CANCEL(-1, CFontPageWiz::OnWizardCancel)

	EVT_BUTTON(IDC_SOURCE_LANG, CFontPageWiz::OnSourceFontChangeBtn)
	EVT_BUTTON(IDC_TARGET_LANG, CFontPageWiz::OnTargetFontChangeBtn)
	EVT_BUTTON(IDC_CHANGE_NAV_TEXT, CFontPageWiz::OnNavTextFontChangeBtn)
	EVT_BUTTON(IDC_BUTTON_SPECTEXTCOLOR, CFontPageWiz::OnButtonSpecTextColor)
	EVT_BUTTON(IDC_RETRANSLATION_BUTTON, CFontPageWiz::OnButtonRetranTextColor)
	EVT_BUTTON(ID_BUTTON_TEXT_DIFFS, CFontPageWiz::OnButtonTgtDiffsTextColor)
	EVT_BUTTON(IDC_BUTTON_NAV_TEXT_COLOR, CFontPageWiz::OnButtonNavTextColor)
	EVT_BUTTON(IDC_BUTTON_SOURCE_COLOR, CFontPageWiz::OnButtonSourceTextColor)
	EVT_BUTTON(IDC_BUTTON_TARGET_COLOR, CFontPageWiz::OnButtonTargetTextColor)
	EVT_BUTTON(ID_BUTTON_CHANGE_SRC_ENCODING, CFontPageWiz::OnButtonChangeSrcEncoding)
	EVT_BUTTON(ID_BUTTON_CHANGE_TGT_ENCODING, CFontPageWiz::OnButtonChangeTgtEncoding)
	EVT_BUTTON(ID_BUTTON_CHANGE_NAV_ENCODING, CFontPageWiz::OnButtonChangeNavEncoding)
END_EVENT_TABLE()


CFontPageWiz::CFontPageWiz()
{
}

CFontPageWiz::CFontPageWiz(wxWizard* parent) // dialog constructor
{
	Create( parent );
	fontPgCommon.DoSetDataAndPointers();
}

CFontPageWiz::~CFontPageWiz() // destructor
{
	
}

bool CFontPageWiz::Create( wxWizard* parent)
{
	wxWizardPage::Create( parent ); // CFontPageWiz is based on wxWizardPage
	CreateControls();
	return TRUE;
}

void CFontPageWiz::CreateControls()
{
	// The function that creates the CFontPagePrefs panel is generated in wxDesigner, 
	// and defines the controls and sizers for the dialog. 
	// The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	fontPgCommon.pFontPageSizer = FontsPageFunc(this, TRUE, TRUE);
	// The declaration is: FontsPageFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	//m_scrolledWindow = new wxScrolledWindow( this, -1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxHSCROLL|wxVSCROLL );
	//m_scrolledWindow->SetSizer(fontPgCommon.pFontPageSizer);
}

// implement wxWizardPage functions
wxWizardPage* CFontPageWiz::GetPrev() const 
{ 
	// add code here to determine the previous page to show in the wizard
	return pLanguagesPage; 
}
wxWizardPage* CFontPageWiz::GetNext() const
{
	// add code here to determine the next page to show in the wizard
    return pPunctCorrespPageWiz; //m_checkbox->GetValue() ? m_next->GetNext() : m_next;
}

void CFontPageWiz::OnWizardCancel(wxWizardEvent& WXUNUSED(event))
{
    //if ( wxMessageBox(_T("Do you really want to cancel?"), _T("Question"),
    //                    wxICON_QUESTION | wxYES_NO, this) != wxYES )
    //{
    //    // not confirmed
    //    event.Veto();
    //}
}

void CFontPageWiz::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	fontPgCommon.DoInit(); // we dont use Init() because it is a method of wxPanel itself

}// end of InitDialog()

// the following function is only needed in the wizard's font page
void CFontPageWiz::OnWizardPageChanging(wxWizardEvent& event)
{
	// OnWizardPageChanging() is called when the CFontPageWiz is used
	// within the Start Working Wizard (during the creation of a
	// <New Project>); 
	// OnWizardPageChanging() is NOT called when
	// used as a panel within the Edit Preferences Notebook, when
	// the user clicks on a different tab. Within the Edit Preferences
	// Notebook, changes to the data structures are effected by
	// the OnOK() handler below which is called when the user clicks
	// on the "OK" button in Edit Preferences.
	//
	// Can put any code that needs to execute regardless of whether
	// Next or Prev button was pushed here.

	// Determine which direction we're going and implement
	// the MFC equivalent of OnWizardNext() and OnWizardBack() here
	bool bMovingForward = event.GetDirection();

	if (bMovingForward)
	{
		// Validate the font data and veto the page change
		// if the font face names are blank or the font sizes
		// are not within the 6pt to 72pt range. 
		// We should veto bad data for a page change going
		// either forward or backward, always offering to 
		// substitute some default values until the user
		// enters valid ones or cancels.

		// Don't accept blank face names for any of the 3 fonts
		if (fontPgCommon.pSrcFontNameBox->GetValue().IsEmpty())
		{
			wxMessageBox(_("Sorry, the source font name cannot be left blank."), _T(""), wxICON_INFORMATION | wxOK);
			fontPgCommon.pSrcFontNameBox->SetFocus();
			event.Veto();
			return;
		}
		if (fontPgCommon.pTgtFontNameBox->GetValue().IsEmpty())
		{
			wxMessageBox(_("Sorry, the target font name cannot be left blank."), _T(""), wxICON_INFORMATION | wxOK);
			fontPgCommon.pTgtFontNameBox->SetFocus();
			event.Veto();
			return;
		}
		if (fontPgCommon.pNavFontNameBox->GetValue().IsEmpty())
		{
			wxMessageBox(_("Sorry, the navigation font name cannot be left blank."), _T(""), wxICON_INFORMATION | wxOK);
			fontPgCommon.pNavFontNameBox->SetFocus();
			event.Veto();
			return;
		}

		// Don't accept font sizes outside the 6pt to 72pt range
		// These font size validity checks should be unnecessary because we use SetRange on the 
		// wxFontData before showing the font dialog
		int fSize;
		wxString strTemp, subStr, msg;
		strTemp = fontPgCommon.pSrcFontSizeBox->GetValue();
		fSize = wxAtoi(strTemp); //fSize = _ttoi(strTemp);
		subStr = _("Sorry, the %s font size must be between %d and %d points. A default size of 12 points will be used instead.");
		if (fSize < MIN_FONT_SIZE || fSize > MAX_FONT_SIZE)
		{
			msg = msg.Format(subStr,_T("source"),MIN_FONT_SIZE,MAX_FONT_SIZE);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			fontPgCommon.pSrcFontSizeBox->SetValue(_T("12"));
			fontPgCommon.pSrcFontSizeBox->SetFocus();
			fontPgCommon.tempSourceSize = 12;
			event.Veto();
			return;
		}
		strTemp = fontPgCommon.pTgtFontSizeBox->GetValue();
		fSize = wxAtoi(strTemp); //fSize = _ttoi(strTemp);
		if (fSize < MIN_FONT_SIZE || fSize > MAX_FONT_SIZE)
		{
			msg = msg.Format(subStr,_T("target"),MIN_FONT_SIZE,MAX_FONT_SIZE);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			fontPgCommon.pTgtFontSizeBox->SetValue(_T("12"));
			fontPgCommon.pTgtFontSizeBox->SetFocus();
			fontPgCommon.tempTargetSize = 12;
			event.Veto();
			return;
		}
		strTemp = fontPgCommon.pNavFontSizeBox->GetValue();
		fSize = wxAtoi(strTemp); //fSize = _ttoi(strTemp);
		if (fSize < MIN_FONT_SIZE || fSize > MAX_FONT_SIZE)
		{
			msg = msg.Format(subStr,_T("navigation"),MIN_FONT_SIZE,MAX_FONT_SIZE);
			wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
			fontPgCommon.pNavFontSizeBox->SetValue(_T("12"));
			fontPgCommon.pNavFontSizeBox->SetFocus();
			fontPgCommon.tempNavTextSize = 12;
			event.Veto();
			return;
		}

		// if we get here the source and language name edits are valid,
		// so we can determine which direction we're going and implement
		// the MFC equivalent of OnWizardNext() and OnWizardBack() here
		// in our wxWidgets' OnWizardPageChanging() method.

		// This block contains some WX-specific code plus MFC's OnWizardNext() code:
		// According to the MFC design, in moving forward toward docPage, it assumes
		// that by pressing Next, the user wishes current values to be saved.
		// Using local member values (which have been set by user interaction 
		// with the controls), set the corresponding App members and global members. 

		// First set any new encoding values if the Set/View Encoding dialog was used
		// to modify the encoding. Since this value is not extracted from a fontPage
		// edit box, we test the "save" encoding values for the three fonts against 
		// the temp encoding values (that would change if the Set/View Encoding dialog
		// made any changes). 
		if (fontPgCommon.saveSrcFontEncoding != fontPgCommon.tempSrcFontEncoding)
		{
			// source font encoding changed via the Set/View Encoding dialog
			gpApp->m_srcEncoding = fontPgCommon.tempSrcFontEncoding; // set the value on the app
			gpApp->m_pSourceFont->SetEncoding(fontPgCommon.tempSrcFontEncoding);
		}
		if (fontPgCommon.saveTgtFontEncoding != fontPgCommon.tempTgtFontEncoding)
		{
			// source font encoding changed via the Set/View Encoding dialog
			gpApp->m_tgtEncoding = fontPgCommon.tempTgtFontEncoding; // set the value on the app
			gpApp->m_pTargetFont->SetEncoding(fontPgCommon.tempTgtFontEncoding);
		}
		if (fontPgCommon.saveNavFontEncoding != fontPgCommon.tempNavFontEncoding)
		{
			// source font encoding changed via the Set/View Encoding dialog
			gpApp->m_navtextFontEncoding = fontPgCommon.tempNavFontEncoding; // set the value on the app
			gpApp->m_pNavTextFont->SetEncoding(fontPgCommon.tempNavFontEncoding);
		}

		// Retrieve values from the controls and set the corresponding App 
		// members. 
		// WX specific code below:
		gpApp->m_pSourceFont->SetFaceName(fontPgCommon.pSrcFontNameBox->GetValue());
		gpApp->m_pTargetFont->SetFaceName(fontPgCommon.pTgtFontNameBox->GetValue());
		gpApp->m_pNavTextFont->SetFaceName(fontPgCommon.pNavFontNameBox->GetValue());

		// TODO: for IDC_SOURCE_SIZE, IDC_TARGET_SIZE, and IDC_NAVTEXT_SIZE need to reject
		// any size values that are not between 6pt and 72pt
		int nVal;
		strTemp = fontPgCommon.pSrcFontSizeBox->GetValue();
		nVal = wxAtoi(strTemp);
		gpApp->m_pSourceFont->SetPointSize(nVal);

		strTemp = fontPgCommon.pTgtFontSizeBox->GetValue();
		nVal = wxAtoi(strTemp);
		gpApp->m_pTargetFont->SetPointSize(nVal);

		strTemp = fontPgCommon.pNavFontSizeBox->GetValue();
		nVal = wxAtoi(strTemp);
		gpApp->m_pNavTextFont->SetPointSize(nVal);

		gpApp->m_pSourceFont->SetStyle(fontPgCommon.tempSourceFontStyle);
		gpApp->m_pTargetFont->SetStyle(fontPgCommon.tempTargetFontStyle);
		gpApp->m_pNavTextFont->SetStyle(fontPgCommon.tempNavTextFontStyle);
		
		gpApp->m_pSourceFont->SetWeight(fontPgCommon.tempSourceFontWeight);
		gpApp->m_pTargetFont->SetWeight(fontPgCommon.tempTargetFontWeight);
		gpApp->m_pNavTextFont->SetWeight(fontPgCommon.tempNavTextFontWeight);
		
		// Ensure that the colors for the 3 main fonts and the
		// colors for the 3 buttons at bottom of dialog get stored
		// store the App's copy of the font colors
		gpApp->m_sourceColor = fontPgCommon.tempSourceColor;
		gpApp->m_targetColor = fontPgCommon.tempTargetColor;
		gpApp->m_navTextColor = fontPgCommon.tempNavTextColor;
		gpApp->m_specialTextColor = fontPgCommon.tempSpecialTextColor;
		gpApp->m_reTranslnTextColor = fontPgCommon.tempReTranslnTextColor;
		gpApp->m_tgtDiffsTextColor = fontPgCommon.tempTgtDiffsTextColor;

		// and set the fontData objects with the current colors
		gpApp->m_pSrcFontData->SetColour(fontPgCommon.tempSourceColor);
		gpApp->m_pTgtFontData->SetColour(fontPgCommon.tempTargetColor);
		gpApp->m_pNavFontData->SetColour(fontPgCommon.tempNavTextColor);

		*gpApp->m_pSrcFontData = fontPgCommon.tempSrcFontData;
		*gpApp->m_pTgtFontData = fontPgCommon.tempTgtFontData;
		*gpApp->m_pNavFontData = fontPgCommon.tempNavFontData;

		// While we continue to use fontInfo struct member values in the
		// config files, we need to update them when the fonts change
		gpApp->UpdateFontInfoStruct(gpApp->m_pSourceFont, SrcFInfo);
		gpApp->UpdateFontInfoStruct(gpApp->m_pTargetFont, TgtFInfo);
		gpApp->UpdateFontInfoStruct(gpApp->m_pNavTextFont, NavFInfo);

		// set text heights from font metrics, for source and target languages
		// first, get the view, then pass it to the UpdateTextHeights function
		CAdapt_ItView* pView = gpApp->GetView();
		wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));
		gpApp->UpdateTextHeights(pView);

	#ifdef _RTL_FLAGS
		// for the NonRoman version, get the user's choices for RTL rendering for source, target & navtext
		// and set the member booleans on the app for these values
		if (gpApp->m_bShowRTL_GUI)
		{
			wxCheckBox* pCheck;
			pCheck = (wxCheckBox*)FindWindowById(IDC_CHECK_SRC_RTL);
			gpApp->m_bSrcRTL = pCheck->GetValue();
			pCheck = (wxCheckBox*)FindWindowById(IDC_CHECK_TGT_RTL);
			gpApp->m_bTgtRTL = pCheck->GetValue();
			pCheck = (wxCheckBox*)FindWindowById(IDC_CHECK_NAVTEXT_RTL);
			gpApp->m_bNavTextRTL = pCheck->GetValue();

			gbLTRLayout = TRUE; 
			gbRTLLayout = FALSE; // default is LTR layout
			if (gpApp->m_bSrcRTL == TRUE && gpApp->m_bTgtRTL == TRUE)
			{
				gbLTRLayout = FALSE; // use these to set layout direction on user's behalf, when possible
				gbRTLLayout = TRUE;
			}
			else if (gpApp->m_bSrcRTL == FALSE && gpApp->m_bTgtRTL == FALSE)
			{
				gbLTRLayout = TRUE; // use these to set layout direction on user's behalf, when possible
				gbRTLLayout = FALSE;
			}

			pView->AdjustAlignmentMenu(gbRTLLayout,gbLTRLayout); // fix the menu, if necessary
			// Note: AdjustAlignmentMenu above also sets the m_bRTL_Layout to match gbRTL_Layout
		}
	#endif	// for _RTL_FLAGS
		// Code above parallels code in EditPreferencesDlg::OnOk() 
		////////////////////////////////////////////////////////////

		// whm: The deletion of any KB stubs and calling of SetupDirectories() should not
		// go here but in the bMovingForward block of LanguagePage's OnWizardPageChanging().
		
		// have the name for the new project into the projectPage's listBox
		// whm comment: Why add the name of the project to the project page's list box here?
		// I'm moving the following statement to the project page itself.
		//pProjectPage->m_pListBox->Append(gpApp->m_curProjectName);
		
		// Movement through wizard pages is sequential - the next page is the punctCorrespPage.
		// The pPunctCorrespPageWiz's InitDialog need to be called here just before going to it
		wxInitDialogEvent idevent;
		pPunctCorrespPageWiz->InitDialog(idevent);
	}
	else
	{
		// Don't require valid font name and size data going backwards

		// This block contains MFC's OnWizardBack() code:
		// moving backward toward languagesPage, so we may need to undo some values
		// if we move back, having just made a new project & set up directories & a KB
		// we will have to clobber the app's knowledge of the KB in case we later use Next>
		// to set things up with same language names again. The directory-code will not break,
		// but to have a KB loaded would break things, so we have to ensure that can't happen.
		if (gpApp->m_pKB != NULL)
		{
			delete gpApp->m_pKB;
			gpApp->m_bKBReady = FALSE;
			gpApp->m_pKB = (CKB*)NULL;
		}
		if (gpApp->m_pGlossingKB != NULL)
		{
			delete gpApp->m_pGlossingKB;
			gpApp->m_bGlossingKBReady = FALSE;
			gpApp->m_pGlossingKB = (CKB*)NULL;
		}
	}
}

void CFontPageWiz::OnSourceFontChangeBtn(wxCommandEvent& WXUNUSED(event)) // top right Change... button
{
	// change font and or size etc
	fontPgCommon.DoSourceFontChangeBtn(this);
}

void CFontPageWiz::OnTargetFontChangeBtn(wxCommandEvent& WXUNUSED(event)) // middle right Change... button
{
	// change font and or size etc
	fontPgCommon.DoTargetFontChangeBtn(this);
}

void CFontPageWiz::OnNavTextFontChangeBtn(wxCommandEvent& WXUNUSED(event)) // lower right Change... button
{
	// change font and or size etc
	fontPgCommon.DoNavTextFontChangeBtn(this);
}

void CFontPageWiz::OnButtonSpecTextColor(wxCommandEvent& WXUNUSED(event)) // bottom left button
{
	// change special text colour
	fontPgCommon.DoButtonSpecTextColor(this);
}

void CFontPageWiz::OnButtonRetranTextColor(wxCommandEvent& WXUNUSED(event)) // bottom center button
{
	fontPgCommon.DoButtonRetranTextColor(this);
}

void CFontPageWiz::OnButtonTgtDiffsTextColor(wxCommandEvent& WXUNUSED(event)) // bottom right button
{
	fontPgCommon.DoButtonTgtDiffsTextColor(this);
}


void CFontPageWiz::OnButtonNavTextColor(wxCommandEvent& WXUNUSED(event)) // bottom right button
{
	fontPgCommon.DoButtonNavTextColor(this);
}

// added in version 2.3.0
void CFontPageWiz::OnButtonSourceTextColor(wxCommandEvent& WXUNUSED(event)) 
{
	// change source text colour
	fontPgCommon.DoButtonSourceTextColor(this);
}

// added in version 2.3.0
void CFontPageWiz::OnButtonTargetTextColor(wxCommandEvent& WXUNUSED(event)) 
{
	// change target text colour
	fontPgCommon.DoButtonTargetTextColor(this);
}

void CFontPageWiz::OnButtonChangeSrcEncoding(wxCommandEvent& WXUNUSED(event))
{
	fontPgCommon.DoButtonChangeSrcEncoding(this);
}

void CFontPageWiz::OnButtonChangeTgtEncoding(wxCommandEvent& WXUNUSED(event))
{
	fontPgCommon.DoButtonChangeTgtEncoding(this);
}

void CFontPageWiz::OnButtonChangeNavEncoding(wxCommandEvent& WXUNUSED(event))
{
	fontPgCommon.DoButtonChangeNavEncoding(this);
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CFontPagePrefs !!!!!!!!!!!!!!!!!!!!!!!!!!!
IMPLEMENT_DYNAMIC_CLASS( CFontPagePrefs, wxPanel )

// event handler table
BEGIN_EVENT_TABLE(CFontPagePrefs, wxPanel)
	// wx Note: InitDialog() is called automatically when CFontPagePrefs is created as a wxPanel
	//EVT_INIT_DIALOG(CFontPagePrefs::InitDialog)// not needed for CFontPagePrefs which is based on wxPanel
	EVT_BUTTON(IDC_SOURCE_LANG, CFontPagePrefs::OnSourceFontChangeBtn)
	EVT_BUTTON(IDC_TARGET_LANG, CFontPagePrefs::OnTargetFontChangeBtn)
	EVT_BUTTON(IDC_CHANGE_NAV_TEXT, CFontPagePrefs::OnNavTextFontChangeBtn)
	EVT_BUTTON(IDC_BUTTON_SPECTEXTCOLOR, CFontPagePrefs::OnButtonSpecTextColor)
	EVT_BUTTON(IDC_RETRANSLATION_BUTTON, CFontPagePrefs::OnButtonRetranTextColor)
	EVT_BUTTON(ID_BUTTON_TEXT_DIFFS, CFontPagePrefs::OnButtonTgtDiffsTextColor)
	EVT_BUTTON(IDC_BUTTON_NAV_TEXT_COLOR, CFontPagePrefs::OnButtonNavTextColor)
	EVT_BUTTON(IDC_BUTTON_SOURCE_COLOR, CFontPagePrefs::OnButtonSourceTextColor)
	EVT_BUTTON(IDC_BUTTON_TARGET_COLOR, CFontPagePrefs::OnButtonTargetTextColor)
	EVT_BUTTON(ID_BUTTON_CHANGE_SRC_ENCODING, CFontPagePrefs::OnButtonChangeSrcEncoding)
	EVT_BUTTON(ID_BUTTON_CHANGE_TGT_ENCODING, CFontPagePrefs::OnButtonChangeTgtEncoding)
	EVT_BUTTON(ID_BUTTON_CHANGE_NAV_ENCODING, CFontPagePrefs::OnButtonChangeNavEncoding)
END_EVENT_TABLE()


CFontPagePrefs::CFontPagePrefs()
{
}

CFontPagePrefs::CFontPagePrefs(wxWindow* parent) // dialog constructor
{
	Create( parent );
	fontPgCommon.DoSetDataAndPointers();
}

CFontPagePrefs::~CFontPagePrefs() // destructor
{
	
}

bool CFontPagePrefs::Create( wxWindow* parent)
{
	wxPanel::Create( parent ); // CFontPagePrefs is based on wxPanel
	CreateControls();
	GetSizer()->Fit(this);
	return TRUE;
}

void CFontPagePrefs::CreateControls()
{
	// The function that creates the CFontPagePrefs panel is generated in wxDesigner, 
	// and defines the controls and sizers for the dialog. 
	// The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	fontPgCommon.pFontPageSizer = FontsPageFunc(this, TRUE, TRUE);
	// The declaration is: FontsPageFunc( wxWindow *parent, bool call_fit, bool set_sizer );
}

// InitDialog here is called explicitly by EditPreferencesDlg's InitDialog() method
void CFontPagePrefs::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxPanel - takes no parameter
{
	//InitDialog() is not virtual, no call needed to a base class

	fontPgCommon.DoInit(); // we dont use Init() because it is a method of wxPanel itself
}

void CFontPagePrefs::OnOK(wxCommandEvent& WXUNUSED(event))
{
	// OnOK() is called when the CFontPagePrefs is used as a panel within 
	// the Edit Preferences Notebook and the user clicks on the Notebook's
	// "OK" button. By pressing OK, the user wishes current values to be saved.
	//
	// Note: For CFontPagePrefs::OnOK, validation of font 
	// data should be done in EditPreferencesDlg's OnOK() 
	// method before calling CFontPagePrefs::OnOK().

	// First set any new encoding values if the Set/View Encoding dialog was used
	// to modify the encoding. Since this value is not extracted from a fontPage
	// edit box, we test the "save" encoding values for the three fonts against 
	// the temp encoding values (that would change if the Set/View Encoding dialog
	// made any changes). 
	if (fontPgCommon.saveSrcFontEncoding != fontPgCommon.tempSrcFontEncoding)
	{
		// source font encoding changed via the Set/View Encoding dialog
		gpApp->m_srcEncoding = fontPgCommon.tempSrcFontEncoding; // set the value on the app
		gpApp->m_pSourceFont->SetEncoding(fontPgCommon.tempSrcFontEncoding);
		gpApp->m_pLayout->m_bFontInfoChanged = TRUE;
	}
	if (fontPgCommon.saveTgtFontEncoding != fontPgCommon.tempTgtFontEncoding)
	{
		// source font encoding changed via the Set/View Encoding dialog
		gpApp->m_tgtEncoding = fontPgCommon.tempTgtFontEncoding; // set the value on the app
		gpApp->m_pTargetFont->SetEncoding(fontPgCommon.tempTgtFontEncoding);
		gpApp->m_pLayout->m_bFontInfoChanged = TRUE;
	}
	if (fontPgCommon.saveNavFontEncoding != fontPgCommon.tempNavFontEncoding)
	{
		// source font encoding changed via the Set/View Encoding dialog
		gpApp->m_navtextFontEncoding = fontPgCommon.tempNavFontEncoding; // set the value on the app
		gpApp->m_pNavTextFont->SetEncoding(fontPgCommon.tempNavFontEncoding);
		gpApp->m_pLayout->m_bFontInfoChanged = TRUE;
	}

	// Retrieve values from the controls and set the corresponding font members
	// on the App. 
	gpApp->m_pSourceFont->SetFaceName(fontPgCommon.pSrcFontNameBox->GetValue());
	gpApp->m_pTargetFont->SetFaceName(fontPgCommon.pTgtFontNameBox->GetValue());
	gpApp->m_pNavTextFont->SetFaceName(fontPgCommon.pNavFontNameBox->GetValue());

	int nVal;
	wxString strTemp;
	strTemp = fontPgCommon.pSrcFontSizeBox->GetValue();
	nVal = wxAtoi(strTemp);
	if (nVal != gpApp->m_pSourceFont->GetPointSize())
		gpApp->m_pLayout->m_bFontInfoChanged = TRUE;
	gpApp->m_pSourceFont->SetPointSize(nVal);

	strTemp = fontPgCommon.pTgtFontSizeBox->GetValue();
	nVal = wxAtoi(strTemp);
	if (nVal != gpApp->m_pTargetFont->GetPointSize())
		gpApp->m_pLayout->m_bFontInfoChanged = TRUE;
	gpApp->m_pTargetFont->SetPointSize(nVal);

	strTemp = fontPgCommon.pNavFontSizeBox->GetValue();
	nVal = wxAtoi(strTemp);
	if (nVal != gpApp->m_pNavTextFont->GetPointSize())
		gpApp->m_pLayout->m_bFontInfoChanged = TRUE;
	gpApp->m_pNavTextFont->SetPointSize(nVal);

	if (fontPgCommon.tempSourceFontStyle != gpApp->m_pSourceFont->GetStyle())
		gpApp->m_pLayout->m_bFontInfoChanged = TRUE;
	gpApp->m_pSourceFont->SetStyle(fontPgCommon.tempSourceFontStyle);

	if (fontPgCommon.tempTargetFontStyle != gpApp->m_pTargetFont->GetStyle())
		gpApp->m_pLayout->m_bFontInfoChanged = TRUE;
	gpApp->m_pTargetFont->SetStyle(fontPgCommon.tempTargetFontStyle);

	if (fontPgCommon.tempNavTextFontStyle != gpApp->m_pNavTextFont->GetStyle())
		gpApp->m_pLayout->m_bFontInfoChanged = TRUE;
	gpApp->m_pNavTextFont->SetStyle(fontPgCommon.tempNavTextFontStyle);
	

	if (fontPgCommon.tempSourceFontWeight != gpApp->m_pSourceFont->GetWeight())
		gpApp->m_pLayout->m_bFontInfoChanged = TRUE;
	gpApp->m_pSourceFont->SetWeight(fontPgCommon.tempSourceFontWeight);

	if (fontPgCommon.tempTargetFontWeight != gpApp->m_pTargetFont->GetWeight())
		gpApp->m_pLayout->m_bFontInfoChanged = TRUE;
	gpApp->m_pTargetFont->SetWeight(fontPgCommon.tempTargetFontWeight);

	if (fontPgCommon.tempNavTextFontWeight != gpApp->m_pNavTextFont->GetWeight())
		gpApp->m_pLayout->m_bFontInfoChanged = TRUE;
	gpApp->m_pNavTextFont->SetWeight(fontPgCommon.tempNavTextFontWeight);

	// Ensure that the colors for the 3 main fonts and the
	// colors for the 4 buttons at bottom of dialog get stored
	// store the App's copy of the font colors
	gpApp->m_sourceColor = fontPgCommon.tempSourceColor;
	gpApp->m_targetColor = fontPgCommon.tempTargetColor;
	gpApp->m_navTextColor = fontPgCommon.tempNavTextColor;
	gpApp->m_specialTextColor = fontPgCommon.tempSpecialTextColor;
	gpApp->m_reTranslnTextColor = fontPgCommon.tempReTranslnTextColor;
	gpApp->m_tgtDiffsTextColor = fontPgCommon.tempTgtDiffsTextColor;

	// and set the fontData objects with the current colors
	gpApp->m_pSrcFontData->SetColour(fontPgCommon.tempSourceColor);
	gpApp->m_pTgtFontData->SetColour(fontPgCommon.tempTargetColor);
	gpApp->m_pNavFontData->SetColour(fontPgCommon.tempNavTextColor);

	*gpApp->m_pSrcFontData = fontPgCommon.tempSrcFontData;
	*gpApp->m_pTgtFontData = fontPgCommon.tempTgtFontData;
	*gpApp->m_pNavFontData = fontPgCommon.tempNavFontData;

	// While we continue to use fontInfo struct member values in the
	// config files, we need to update them when the fonts change
	gpApp->UpdateFontInfoStruct(gpApp->m_pSourceFont, SrcFInfo);
	gpApp->UpdateFontInfoStruct(gpApp->m_pTargetFont, TgtFInfo);
	gpApp->UpdateFontInfoStruct(gpApp->m_pNavTextFont, NavFInfo);

	// set text heights from font metrics, for source and target languages
	// first, get the view, then pass it to the UpdateTextHeights function
	CAdapt_ItView* pView = gpApp->GetView();
	wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));
	gpApp->UpdateTextHeights(pView);

#ifdef _RTL_FLAGS
	// for the NonRoman version, get the user's choices for RTL rendering for source, target & navtext
	// and set the member booleans on the app for these values
	if (gpApp->m_bShowRTL_GUI)
	{
		wxCheckBox* pCheck;
		pCheck = (wxCheckBox*)FindWindowById(IDC_CHECK_SRC_RTL);
		gpApp->m_bSrcRTL = pCheck->GetValue();
		pCheck = (wxCheckBox*)FindWindowById(IDC_CHECK_TGT_RTL);
		gpApp->m_bTgtRTL = pCheck->GetValue();
		pCheck = (wxCheckBox*)FindWindowById(IDC_CHECK_NAVTEXT_RTL);
		gpApp->m_bNavTextRTL = pCheck->GetValue();

		gbLTRLayout = TRUE; 
		gbRTLLayout = FALSE; // default is LTR layout
		if (gpApp->m_bSrcRTL == TRUE && gpApp->m_bTgtRTL == TRUE)
		{
			gbLTRLayout = FALSE; // use these to set layout direction on user's behalf, when possible
			gbRTLLayout = TRUE;
		}
		else if (gpApp->m_bSrcRTL == FALSE && gpApp->m_bTgtRTL == FALSE)
		{
			gbLTRLayout = TRUE; // use these to set layout direction on user's behalf, when possible
			gbRTLLayout = FALSE;
		}

		pView->AdjustAlignmentMenu(gbRTLLayout,gbLTRLayout); // fix the menu, if necessary
		// Note: AdjustAlignmentMenu above also sets the m_bRTL_Layout to match gbRTL_Layout

		// if we've entered this block, any changes made should not affect pile widths,
		// nor strip populations, therefore m_bFontInfoChanged should not be set TRUE here
		//gpApp->m_pLayout->m_bFontInfoChanged = TRUE;
	}
	// enable complex rendering
	// whm note for wx version: Right-to-left reading is handled automatically in Uniscribe and
	// Pango, but they differ in how they handle Unicode text chars that are from the first 128
	// point positions. In wxMSW SetLayoutDirection() aligns these to the right in the phrasebox
	// but in wxGTK (under Pango) SetLayoutDirection() aligns these to the left within the
	// phrasebox.
	if (gpApp->m_bTgtRTL)	// MFC code has m_bSrcRTL but it should be m_bTgtRTL (in the MFC version it 
							// probably gets overridden by ResizeBox() which actually uses m_bTgtRTL 
							// when it sets the alignment for the target box
	{
		gpApp->m_pTargetBox->SetLayoutDirection(wxLayout_RightToLeft);
// whm Note: Pango overrides the following SetStyle() command
//#ifndef __WXMSW__
//		gpApp->m_pTargetBox->SetStyle(-1,-1,wxTextAttr(wxTEXT_ALIGNMENT_RIGHT));
//#endif
	}
	else
	{
		gpApp->m_pTargetBox->SetLayoutDirection(wxLayout_LeftToRight);
// whm Note: Pango overrides the following SetStyle() command
//#ifndef __WXMSW__
//		gpApp->m_pTargetBox->SetStyle(-1,-1,wxTextAttr(wxTEXT_ALIGNMENT_LEFT));
//#endif
	}
#endif	// for _RTL_FLAGS

	// Since OnOK is called from the Edit Preferences dialog rather than the startup 
	// wizard, we can assume that there currently is an existing project open.
	// Therefore we don't need to add any code that deletes any KB stubs, calls SetupDirectories,
	// etc:
}


void CFontPagePrefs::OnSourceFontChangeBtn(wxCommandEvent& WXUNUSED(event)) // top right Change... button
{
	// change font and or size etc
	fontPgCommon.DoSourceFontChangeBtn(this);
}

void CFontPagePrefs::OnTargetFontChangeBtn(wxCommandEvent& WXUNUSED(event)) // middle right Change... button
{
	// change font and or size etc
	fontPgCommon.DoTargetFontChangeBtn(this);
}

void CFontPagePrefs::OnNavTextFontChangeBtn(wxCommandEvent& WXUNUSED(event)) // lower right Change... button
{
	// change font and or size etc
	fontPgCommon.DoNavTextFontChangeBtn(this);
}

void CFontPagePrefs::OnButtonSpecTextColor(wxCommandEvent& WXUNUSED(event)) // bottom left button
{
	// change special text colour
	fontPgCommon.DoButtonSpecTextColor(this);
}

void CFontPagePrefs::OnButtonRetranTextColor(wxCommandEvent& WXUNUSED(event)) // bottom center button
{
	fontPgCommon.DoButtonRetranTextColor(this);
}

void CFontPagePrefs::OnButtonTgtDiffsTextColor(wxCommandEvent& WXUNUSED(event)) // bottom right button
{
	fontPgCommon.DoButtonTgtDiffsTextColor(this);
}

void CFontPagePrefs::OnButtonNavTextColor(wxCommandEvent& WXUNUSED(event)) // bottom right button
{
	fontPgCommon.DoButtonNavTextColor(this);
}

// added in version 2.3.0
void CFontPagePrefs::OnButtonSourceTextColor(wxCommandEvent& WXUNUSED(event)) 
{
	// change source text colour
	fontPgCommon.DoButtonSourceTextColor(this);
}

// added in version 2.3.0
void CFontPagePrefs::OnButtonTargetTextColor(wxCommandEvent& WXUNUSED(event)) 
{
	// change target text colour
	fontPgCommon.DoButtonTargetTextColor(this);
}

void CFontPagePrefs::OnButtonChangeSrcEncoding(wxCommandEvent& WXUNUSED(event))
{
	fontPgCommon.DoButtonChangeSrcEncoding(this);
}

void CFontPagePrefs::OnButtonChangeTgtEncoding(wxCommandEvent& WXUNUSED(event))
{
	fontPgCommon.DoButtonChangeTgtEncoding(this);
}

void CFontPagePrefs::OnButtonChangeNavEncoding(wxCommandEvent& WXUNUSED(event))
{
	fontPgCommon.DoButtonChangeNavEncoding(this);
}
