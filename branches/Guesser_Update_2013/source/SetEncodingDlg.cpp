/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SetEncodingDlg.cpp
/// \author			Bill Martin
/// \date_created	5 February 2007
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CSetEncodingDlg class. 
/// The CSetEncodingDlg class provides a means for examining a font's encoding 
/// and changing that encoding to a more appropriate value if desired. See the
/// InitializeFonts() function in the app for more information about mapping
/// encodings between MFC and wxWidgets.
/// \derivation		The CSetEncodingDlg class is derived from AIModalDialog.
/// Some of the code is adapted from the wxWidgets font.cpp sample program.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in SetEncodingDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "SetEncodingDlg.h"
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
#include <wx/fontenum.h>
#include <wx/fontmap.h>
#include <wx/tokenzr.h>

#include "Adapt_It.h"
#include "SetEncodingDlg.h"
#include "helpers.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

extern wxSizer *pTestBoxStaticTextBoxSizer;
wxChar typedChar; // in global program space
wxFontEncoding gthisFontsCurrEncoding; // in global program space
FontDisplayCanvas* pCanvas;

BEGIN_EVENT_TABLE(CEncodingTestBox, wxTextCtrl)
	EVT_CHAR(CEncodingTestBox::OnChar)
END_EVENT_TABLE()

void CEncodingTestBox::OnChar(wxKeyEvent& event)
{
	// Note: probably best to allow all key CTRL+key strokes in case some keboard input
	// method needs a modifier key combination.
	typedChar = event.GetKeyCode();
	// call OnPaint() to highlight the typedChar in yellow
	//wxPaintEvent pevent;
	//pCanvas->OnPaint(pevent);
	// Note: calling OnPaint() directly does not work - actually the OnPaint() handler gets
	// called but none of the dc methods within OnPaint() do anything. Calling Refresh() works.
	pCanvas->Refresh();
	event.Skip(); // so that the base class wxTextCtrl can process it
}

// event handler table
BEGIN_EVENT_TABLE(CSetEncodingDlg, AIModalDialog)
	EVT_INIT_DIALOG(CSetEncodingDlg::InitDialog)
	EVT_BUTTON(ID_BUTTON_CHART_FONT_SIZE_INCREASE, CSetEncodingDlg::OnBtnChartFontSizeIncrease)
	EVT_BUTTON(ID_BUTTON_CHART_FONT_SIZE_DECREASE, CSetEncodingDlg::OnBtnChartFontSizeDecrease)
	EVT_LISTBOX(ID_LISTBOX_POSSIBLE_ENCODINGS, CSetEncodingDlg::OnListEncodingsChanged)
	EVT_LISTBOX(ID_LISTBOX_POSSIBLE_FACENAMES, CSetEncodingDlg::OnListFacenamesChanged)
	EVT_BUTTON(wxID_OK, CSetEncodingDlg::OnOK)
END_EVENT_TABLE()

CSetEncodingDlg::CSetEncodingDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Set or View Font Encoding"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pSetEncDlgSizer = SetEncodingDlgFunc(this, TRUE, TRUE);
	// The declaration is: SetEncodingDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	pStaticCurrEncodingIs = (wxStaticText*)FindWindowById(ID_STATIC_CURR_ENCODING_IS);
	wxASSERT(pStaticCurrEncodingIs != NULL);
	
	pStaticSetFontTitle = (wxStaticText*)FindWindowById(ID_STATIC_SET_FONT_TITLE);
	wxASSERT(pStaticSetFontTitle != NULL);

	pStaticChartFontSize = (wxStaticText*)FindWindowById(ID_STATIC_CHART_FONT_SIZE);
	wxASSERT(pStaticChartFontSize != NULL);
	
	pCurrEncoding = (wxTextCtrl*)FindWindowById(ID_TEXT_CURR_ENCODING);
	wxASSERT(pCurrEncoding != NULL);
	pCurrEncoding->SetBackgroundColour(gpApp->sysColorBtnFace);

	pTestEncodingBox = (CEncodingTestBox*)FindWindowById(ID_TEXT_TEST_ENCODING_BOX);
	wxASSERT(pTestEncodingBox != NULL);

	pPossibleEncodings = (wxListBox*)FindWindowById(ID_LISTBOX_POSSIBLE_ENCODINGS);
	wxASSERT(pPossibleEncodings != NULL);

	pPossibleFacenames = (wxListBox*)FindWindowById(ID_LISTBOX_POSSIBLE_FACENAMES);
	wxASSERT(pPossibleFacenames != NULL);

	pScrolledEncodingWindow = (wxScrolledWindow*)FindWindowById(ID_SCROLLED_ENCODING_WINDOW);
	wxASSERT(pScrolledEncodingWindow != NULL);

	// Note: The "Apply Selected Encoding and Face Name To The %s Font" button is assigned wxID_OK
	pApplyEncodingButton = (wxButton*)FindWindow(wxID_OK); // use FindWindow to find the child window here
	wxASSERT(pApplyEncodingButton != NULL);

	pIncreaseChartFontSize = (wxButton*)FindWindowById(ID_BUTTON_CHART_FONT_SIZE_INCREASE);
	wxASSERT(pIncreaseChartFontSize != NULL);

	pDecreaseChartFontSize = (wxButton*)FindWindowById(ID_BUTTON_CHART_FONT_SIZE_DECREASE);
	wxASSERT(pDecreaseChartFontSize != NULL);

	// other attribute initializations

	m_canvas = new FontDisplayCanvas(pScrolledEncodingWindow);
	pCanvas = m_canvas; // so the CEncodingTestBox class can access it
	m_canvas->SetSize(1500,600);	// 1500 pixels wide by 600 pixels tall should be enough for any
									// allowable point size for the chart
									// TODO: the actual size of the canvas could be determined programatically

}

CSetEncodingDlg::~CSetEncodingDlg() // destructor
{
	
}

void CSetEncodingDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	// put the word "Source", "Target" or "Navigation Text" into the static text strings
	wxString titleStr,staticStr1,staticStr2;
	titleStr = pStaticSetFontTitle->GetLabel(); // _("View or Set Font Encoding for the %s Language Font")
	titleStr = titleStr.Format(titleStr,langFontName.c_str());
	pStaticSetFontTitle->SetLabel(titleStr);

	staticStr1 = pStaticCurrEncodingIs->GetLabel(); //_("The current encoding for the %s font is:")
	staticStr1 = staticStr1.Format(staticStr1,langFontName.c_str());
	pStaticCurrEncodingIs->SetLabel(staticStr1);

	staticStr2 = pApplyEncodingButton->GetLabel(); // _("Apply Selected Encoding and Face Name To The %s Font")
	staticStr2 = staticStr2.Format(staticStr2,langFontName.c_str());
	pApplyEncodingButton->SetLabel(staticStr2);

	if (langFontName == _("Source"))
	{
		thisFontsCurrEncoding = gpApp->m_srcEncoding;
		gthisFontsCurrEncoding = thisFontsCurrEncoding;
		thisFontsFaceName = gpApp->m_pSourceFont->GetFaceName();
		thisFontsFamily = (wxFontFamily)gpApp->m_pSourceFont->GetFamily();
		thisFontsEncodingNameAndDescription = wxFontMapper::GetEncodingName(gpApp->m_srcEncoding)
			+ _T(" [") + wxFontMapper::GetEncodingDescription(gpApp->m_srcEncoding)
			+ _T("]");
		pCurrEncoding->SetValue(thisFontsEncodingNameAndDescription);
	}
	else if (langFontName == _("Target"))
	{
		thisFontsCurrEncoding = gpApp->m_tgtEncoding;
		gthisFontsCurrEncoding = thisFontsCurrEncoding;
		thisFontsFaceName = gpApp->m_pTargetFont->GetFaceName();
		thisFontsFamily = (wxFontFamily)gpApp->m_pTargetFont->GetFamily();
		thisFontsEncodingNameAndDescription = wxFontMapper::GetEncodingName(gpApp->m_tgtEncoding)
			+ _T(" [") + wxFontMapper::GetEncodingDescription(gpApp->m_tgtEncoding)
			+ _T("]");
		pCurrEncoding->SetValue(thisFontsEncodingNameAndDescription);
	}
	else
	{
		wxASSERT(langFontName == _("Navigation Text")); // if not it is a programming error in fontPage!
		thisFontsCurrEncoding = gpApp->m_navtextFontEncoding;
		gthisFontsCurrEncoding = thisFontsCurrEncoding;
		thisFontsFaceName = gpApp->m_pNavTextFont->GetFaceName();
		thisFontsFamily = (wxFontFamily)gpApp->m_pNavTextFont->GetFamily();
		thisFontsEncodingNameAndDescription = wxFontMapper::GetEncodingName(gpApp->m_navtextFontEncoding)
			+ _T(" [") + wxFontMapper::GetEncodingDescription(gpApp->m_navtextFontEncoding)
			+ _T("]");
		pCurrEncoding->SetValue(thisFontsEncodingNameAndDescription);
	}

	fontEncodingSelected = thisFontsCurrEncoding; // default to the current encoding of the font
	fontFaceNameSelected = thisFontsFaceName; // default to the current face name of the font
	fontFamilySelected = thisFontsFamily; // default to the current family of the font
	fontsEncodingNameAndDescriptionSelected = thisFontsEncodingNameAndDescription;
	
	// keep track of the list index of the current encoding selection
	nCurrListSelEncoding = wxNOT_FOUND; // = -1 this should be changed to a valid listbox index below
	// keep track of the list index of the current facename selection
	nCurrListSelFaceName = wxNOT_FOUND; // = -1 this should be changed to a valid listbox index below

	// populate the font face name listbox first, then choose an encoding for the face name
	// get the possible facenames
	FontFacenameEnumerator fontFacenameEnumerator; 
	wxArrayString fontNamesArray;
	fontFacenameEnumerator.EnumerateFacenames(wxFONTENCODING_SYSTEM,FALSE); // get all fonts available on the system
	// Note FALSE parameter above is for fixedWidthOnly param - we want all possible fonts
	pPossibleFacenames->Clear();
	if (fontFacenameEnumerator.GotAny())
	{
		int nFacenames = fontFacenameEnumerator.GetFacenames().GetCount();
		int ct;
		for (ct = 0; ct < nFacenames; ct++)
		{
			pPossibleFacenames->Append(fontFacenameEnumerator.GetFacenames().Item(ct));
		}
		// select by default the facename of the current font being used in the current encoding 
		// EnumerateFacenames call above should include it
		nCurrListSelFaceName = pPossibleFacenames->FindString(thisFontsFaceName);
		if (nCurrListSelFaceName != wxNOT_FOUND)
		{
			pPossibleFacenames->SetSelection(nCurrListSelFaceName);
			// scroll listbox to show it as second item in list (if it isn't the first item)
			if (nCurrListSelFaceName > 0)
				pPossibleFacenames->SetFirstItem(nCurrListSelFaceName -1);
		}
		wxASSERT(nCurrListSelFaceName != wxNOT_FOUND);
	}
	else
	{
		// error - no font available for the selected encoding - this should never happen
		// here in InitDialog where we are selecting the first item in each list
		wxMessageBox(_("No Font Face Names are available on this system!"),_T(""),wxICON_EXCLAMATION | wxOK);
	}
	
	FontEncodingEnumerator fontEncEnumerator;
	// Note: the wxFontEnumerator::EnumerateEncodings calls OnFontEncoding for each encoding supported
	// by a given font (if passed in as its parameter); or if no passed in font, for each encoding 
	// supported by at least some font (when no font is specified).
	// TODO: check this ??? Note: On Windows, the following call generates a debug assert from fontutil.cpp which says,
	// " assert "wxAssertFailure" failed in wxGetFontEncFromCharSet(): unexpected Win32 charset"
	// probably related to the "unknown--nn" encodings listed (nn is a number usually 1).
    fontEncEnumerator.EnumerateEncodings(thisFontsFaceName); // a virtual bool function that calls another virtual bool
	// function called OnFontEncoding() which returns in its ref parameters a string font facename and
	// a string encoding name, as long as it finds matches (see FontEncodingEnumerator class decl.)
	// Now can use fontEncEnumerator.GetText() to retrieve the encodings in string
	// form separated by \n characters, to parse and load into the ID_LISTBOX_POSSIBLE_ENCODINGS
	// listbox.
	// Note: The above EnumerateEncodings() call on my Windows machine gives a harmless "wxAssertFailure
	// failed in wxGetFontEncFromCharSet(): unexpected Win32 charset" [in fontutil.cpp(189)]. This is
	// probably related to the "unknown--nn" encodings that it finds. We filter these out of the
	// Encodings listbox so the user won't be worried by them.
	wxString encodingsStr;
	encodingsStr = fontEncEnumerator.GetEncodingText();
	wxStringTokenizer tkz(encodingsStr,_T("\n"));
	wxString tempToken;
	// load the possible encodings listbox
	pPossibleEncodings->Clear();

#ifdef __GNUG__
	// For ANSI builds, I want to ensure that the "WINDOWS-1252 [Windows Western European (CP 1252)]" 
	// is always listed first in the encodings listbox by default. This is because neither WINDOWS-1252 
	// nor any other Windows encoding is returned by EnumerateEncodings() on Ubuntu Linux/GTK.
	pPossibleEncodings->Append(_T("WINDOWS-1252 [Windows Western European (CP 1252)]"));
#endif

	while (tkz.HasMoreTokens())
	{
		tempToken = tkz.GetNextToken();
		// add it to the list
		// Observation: some encodings returned may be listed as unknown encodings (i.e., "unknown-nn").
		// I don't know of any reason why we should list these, so I'll filter them out of the listbox.
		// Also, when navigating the list boxes with keyboard shortcuts and arrow keys, any "unknown..."
		// item results in a message box being displayed and afterwards the selection reverts back to
		// the last selected item - making it impossible to use the arrow keys to scroll through all
		// items in the list box.
		// TODO: In ANSI builds, should we also eliminate Unicode encodings (UTF-8, etc) from list???
		if (tempToken.Find(_T("unknown")) == -1)
		{
			// add only known encodings
			// Note: on Ubuntu, "UTF-8 [Unicode 8 bit (UTF-8)]" is returned multiple times, so
			// we only add an item if it's not already in the list 
			if (pPossibleEncodings->FindString(tempToken) == wxNOT_FOUND)
				pPossibleEncodings->Append(tempToken);
		}
	}
	// make sure the current encoding is selected
	if (pPossibleEncodings->GetCount() > 0)
	{
		// if the listbox is not empty, the first listed encoding should be the default/current one
		// according to the docs on wxFontEnumerator::EnumerateEncodings
		bool bSuccess;
		bSuccess = pPossibleEncodings->SetStringSelection(thisFontsEncodingNameAndDescription);
		wxASSERT(bSuccess == TRUE);
		bSuccess = bSuccess; // avoid warning TODO: Check for failures?
		wxString EncStrInList = thisFontsEncodingNameAndDescription;
		// if above asserts pass, it is safe to assume listbox index of current encoding is 0
		nCurrListSelEncoding = pPossibleEncodings->GetSelection();
		wxASSERT(nCurrListSelEncoding != wxNOT_FOUND);
		// remove the description part of the firstEncStrInList 
		int posBracket = EncStrInList.Find(_T('['),TRUE); // TRUE finds pos from end
		wxASSERT(EncStrInList.GetChar(posBracket -1) == _T(' ')); //should be a space before the [
		EncStrInList = EncStrInList.Mid(0,posBracket -1);
	}

	// use the temporary font to display the text using the selected encoding and facename
	// and use an initial point size of 11 points
	nTempFontSize = 11;
	wxFont tempFont(nTempFontSize, // use a starting size of 11 points
					thisFontsFamily, wxFONTSTYLE_NORMAL,
					wxFONTWEIGHT_NORMAL, false, thisFontsFaceName, thisFontsCurrEncoding);
	DoChangeFont(tempFont); // calls SetFont() on m_canvas and on pTestEncodingBox
	// don't need to call tempFont.SetEncoding(thisFontsCurrEncoding) here because the font
	// with the default encoding is created above. We call it in the listbox sel change handlers.
	
	// put some sample text (including punctuation) in the Encoding Text Box
	wxString sampleTextWithPunctChars;
	//sampleTextWithPunctChars = _("The text in this edit box is displayed in the font encoding selected at left.\nShown below are the default punctuation characters, including smart quotes:\n");
	// add punct chars and \n
	if (!gpApp->m_punctuation[0].IsEmpty())
	{
		sampleTextWithPunctChars += gpApp->m_punctuation[0] + _T("\n");
	}
	pTestEncodingBox->ChangeValue(sampleTextWithPunctChars); //ChangeValue doesn't generate a wxEVT_COMMAND_TEXT_UPDATED event
	pTestEncodingBox->SetInsertionPointEnd();
	pTestEncodingBox->SetFocus();

	pSetEncDlgSizer->Layout();
}

// event handling functions
void CSetEncodingDlg::OnBtnChartFontSizeIncrease(wxCommandEvent& WXUNUSED(event))
{
	if (nTempFontSize < 19)
	{
		wxString tmpNum;
		tmpNum.Empty();
		nTempFontSize += 2;
		tmpNum << nTempFontSize;
		DoResizeFont(2);
		pStaticChartFontSize->SetLabel(tmpNum);
		m_canvas->Refresh();
		pTestEncodingBox->SetFocus(); // keep focus in test box
	}
	else
	{
		::wxBell();
	}
}

void CSetEncodingDlg::OnBtnChartFontSizeDecrease(wxCommandEvent& WXUNUSED(event))
{
	if (nTempFontSize > 5)
	{
		wxString tmpNum;
		tmpNum.Empty();
		nTempFontSize -= 2;
		tmpNum << nTempFontSize;
		DoResizeFont(-2);
		pStaticChartFontSize->SetLabel(tmpNum);
		// apply changes to the chart
		m_canvas->Refresh();
		pTestEncodingBox->SetFocus(); // keep focus in test box
	}
	else
	{
		::wxBell();
	}
}

void CSetEncodingDlg::OnListEncodingsChanged(wxCommandEvent& WXUNUSED(event))
{
	// This handler is activated when the encodings list selection is changed
	int nSelCount;
	nSelCount = pPossibleEncodings->GetCount();
	// whm: Handlers in CSetEncodingDlg already have code to deal with invalid selections so we'll not
	// add the ListBoxPassesSanityCheck to handlers in this class.
	if (nSelCount < 1)
	{
		::wxBell();
		return;
	}

	// Check that we have a valid selection
	int nSel;
	nSel = pPossibleEncodings->GetSelection();
	if (nSel == wxNOT_FOUND)
	{
		// user must have removed the current listbox item's selection, we should ensure
		// that in the Linux/GTK version this can't happen; select the current item again
		nSel = nCurrListSelEncoding;
		pPossibleEncodings->SetSelection(nSel);
		wxASSERT(pPossibleEncodings->GetSelection() != wxNOT_FOUND);
		::wxBell();
		return;
	}
	// make the selected encoding become the current encoding for our temporary font
	fontsEncodingNameAndDescriptionSelected = pPossibleEncodings->GetStringSelection();
	wxASSERT(!fontsEncodingNameAndDescriptionSelected.IsEmpty());
	wxString EncStrInList = fontsEncodingNameAndDescriptionSelected;
	nCurrListSelEncoding = pPossibleEncodings->GetSelection();
	wxASSERT(nCurrListSelEncoding != wxNOT_FOUND);
	// remove the description part of the firstEncStrInList 
	int posBracket = EncStrInList.Find(_T('['),TRUE); // TRUE finds pos from end
	wxASSERT(EncStrInList.GetChar(posBracket -1) == _T(' ')); //should be a space before the [
	EncStrInList = EncStrInList.Mid(0,posBracket -1);
	fontEncodingSelected = wxFontMapper::GetEncodingFromName(EncStrInList);
	gthisFontsCurrEncoding = fontEncodingSelected; // set the global for Paint()
	// a change in encoding and/or facename has been made so make the test box and chart
	// show their data with the new encoding and/or facename
	fontFaceNameSelected = pPossibleFacenames->GetStringSelection();
	wxFont tempFont(nTempFontSize, // use a starting size of 11 points
					fontFamilySelected, wxFONTSTYLE_NORMAL,
					wxFONTWEIGHT_NORMAL, false, fontFaceNameSelected, fontEncodingSelected);
	DoChangeFont(tempFont); // calls SetFont() on m_canvas and on pTestEncodingBox
	// don't need to call tempFont.SetEncoding(thisFontsCurrEncoding) here because the font
	// with the default encoding is created above. We call it in the listbox sel change handlers.

	// apply changes to the chart
	m_canvas->Refresh();

	nCurrListSelFaceName = nSel;

	// make the "Apply Selected Encoding..." button enabled
	pApplyEncodingButton->Enable(TRUE);

	pTestEncodingBox->SetFocus(); // keep focus in test box
}

void CSetEncodingDlg::OnListFacenamesChanged(wxCommandEvent& WXUNUSED(event))
{

	// Note: This handler is activated when the face names list is changed and also
	// it is called when the encodings list is changed.
	int nSel,nSelCount;
	nSelCount = pPossibleFacenames->GetCount();
	// whm: Handlers in CSetEncodingDlg already have code to deal with invalid selections so we'll not
	// add the ListBoxPassesSanityCheck to handlers in this class.
	if (nSelCount < 1)
	{
		::wxBell();
		return;
	}
	// there is at least one face name in list
	nSel = pPossibleFacenames->GetSelection();
	if (nSel == wxNOT_FOUND)
	{
		// user may have removed the current listbox item's selection, we should ensure
		// that in the Linux/GTK version this can't happen; select the current item again
		// if possible using the previous selection
		if (nSelCount > nCurrListSelFaceName)
		{
			nSel = nCurrListSelFaceName;
			pPossibleFacenames->SetSelection(nSel);
			wxASSERT(pPossibleFacenames->GetSelection() != wxNOT_FOUND);
		}
		else
		{
			// nSelCount is too large, so just select the first face name in list
			// Within this block there should be at least one item in the facenames list
			wxASSERT(pPossibleFacenames->GetCount() > 0);
			pPossibleFacenames->SetSelection(0);
			// beep to indicate the app had to force a selection
			::wxBell();
		}
	}

	// selection is now ok so proceed
	nCurrListSelFaceName = pPossibleFacenames->GetSelection();

	// Enumerate the encodings for the selected font face name
	FontEncodingEnumerator fontEncEnumerator;
	// Note: the wxFontEnumerator::EnumerateEncodings calls OnFontEncoding for each encoding supported
	// by a given font (if passed in as its parameter); or if no passed in font, for each encoding 
	// supported by at least some font (when no font is specified).
	// TODO: check this ??? Note: On Windows, the following call generates a debug assert from fontutil.cpp which says,
	// " assert "wxAssertFailure" failed in wxGetFontEncFromCharSet(): unexpected Win32 charset"
	// probably related to the "unknown--nn" encodings listed (nn is a number usually 1).
	fontFaceNameSelected = pPossibleFacenames->GetStringSelection();
    fontEncEnumerator.EnumerateEncodings(fontFaceNameSelected); // a virtual bool function that calls another virtual bool
	// function called OnFontEncoding() which returns in its ref parameters a string font facename and
	// a string encoding name, as long as it finds matches (see FontEncodingEnumerator class decl.)
	// Now can use fontEncEnumerator.GetText() to retrieve the encodings in string
	// form separated by \n characters, to parse and load into the ID_LISTBOX_POSSIBLE_ENCODINGS
	// listbox.
	// Note: The above EnumerateEncodings() call on my Windows machine gives a harmless "wxAssertFailure
	// failed in wxGetFontEncFromCharSet(): unexpected Win32 charset" [in fontutil.cpp(189)]. This is
	// probably related to the "unknown--nn" encodings that it finds. We filter these out of the
	// Encodings listbox so the user won't be worried by them.
	wxString encodingsStr;
	encodingsStr = fontEncEnumerator.GetEncodingText();
	wxStringTokenizer tkz(encodingsStr,_T("\n"));
	wxString tempToken;
	// load the possible encodings listbox
	pPossibleEncodings->Clear();

#ifdef __GNUG__
	// For ANSI builds, I want to ensure that the "WINDOWS-1252 [Windows Western European (CP 1252)]" 
	// is always listed first in the encodings listbox by default. This is because neither WINDOWS-1252 
	// nor any other Windows encoding is returned by EnumerateEncodings() on Ubuntu Linux/GTK.
	pPossibleEncodings->Append(_T("WINDOWS-1252 [Windows Western European (CP 1252)]"));
#endif

	while (tkz.HasMoreTokens())
	{
		tempToken = tkz.GetNextToken();
		// add it to the list
		// Observation: some encodings returned may be listed as unknown encodings (i.e., "unknown-nn").
		// I don't know of any reason why we should list these, so I'll filter them out of the listbox.
		// Also, when navigating the list boxes with keyboard shortcuts and arrow keys, any "unknown..."
		// item results in a message box being displayed and afterwards the selection reverts back to
		// the last selected item - making it impossible to use the arrow keys to scroll through all
		// items in the list box.
		// TODO: In ANSI builds, should we also eliminate Unicode encodings (UTF-8, etc) from list???
		if (tempToken.Find(_T("unknown")) == -1)
		{
			// add only known encodings
			// Note: on Ubuntu, "UTF-8 [Unicode 8 bit (UTF-8)]" is returned multiple times, so
			// we only add an item if it's not already in the list 
			if (pPossibleEncodings->FindString(tempToken) == wxNOT_FOUND)
				pPossibleEncodings->Append(tempToken);
		}
	}

	// try to use the thisFontsCurrEncoding encoding if it exists in the list; if not
	// select the first encoding now in the list
	if (pPossibleEncodings->GetCount() > 0)
	{
		// the listbox is not empty so see if thisFontsCurrEncoding exists in the list
		pPossibleEncodings->SetSelection(0);
		wxString firstEncStrInList = pPossibleEncodings->GetString(0);
		wxASSERT(firstEncStrInList == pPossibleEncodings->GetStringSelection());
		// if above asserts pass, it is safe to assume listbox index of current encoding is 0
		nCurrListSelEncoding = 0;
		// remove the description part of the firstEncStrInList 
		int posBracket = firstEncStrInList.Find(_T('['),TRUE); // TRUE finds pos from end
		wxASSERT(firstEncStrInList.GetChar(posBracket -1) == _T(' ')); //should be a space before the [
		firstEncStrInList = firstEncStrInList.Mid(0,posBracket -1);
	}

	// a change in encoding and/or facename has been made so make the test box and chart
	// show their data with the new encoding and/or facename
	fontFaceNameSelected = pPossibleFacenames->GetStringSelection();
	wxFont tempFont(nTempFontSize, // use a starting size of 11 points
					fontFamilySelected, wxFONTSTYLE_NORMAL,
					wxFONTWEIGHT_NORMAL, false, fontFaceNameSelected, fontEncodingSelected);
	DoChangeFont(tempFont); // calls SetFont() on m_canvas and on pTestEncodingBox
	// don't need to call tempFont.SetEncoding(thisFontsCurrEncoding) here because the font
	// with the default encoding is created above. We call it in the listbox sel change handlers.

	// apply changes to the chart
	m_canvas->Refresh();

	nCurrListSelFaceName = nSel;

	// make the "Apply Selected Encoding..." button enabled
	pApplyEncodingButton->Enable(TRUE);

	pTestEncodingBox->SetFocus(); // keep focus in test box
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CSetEncodingDlg::OnOK(wxCommandEvent& event) 
{
	// Nothing special to be done here. The fontPage checks for changes and
	// effects there if user presses OK button or Next in wizard
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void CSetEncodingDlg::DoResizeFont(int diff)
{
    wxFont font = m_canvas->GetTextFont();

    font.SetPointSize(font.GetPointSize() + diff);
    DoChangeFont(font);
}

void CSetEncodingDlg::DoChangeFont(const wxFont& font, const wxColour& col)
{
    m_canvas->SetTextFont(font);
    if ( col.Ok() )
        m_canvas->SetColour(col);
    m_canvas->Refresh();

    pTestEncodingBox->SetFont(font);
    if ( col.Ok() )
        pTestEncodingBox->SetForegroundColour(col);
}


// ----------------------------------------------------------------------------
// FontDisplayCanvas
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(FontDisplayCanvas, wxWindow)
    EVT_PAINT(FontDisplayCanvas::OnPaint)
END_EVENT_TABLE()

FontDisplayCanvas::FontDisplayCanvas( wxWindow *parent )
        : wxWindow( parent, wxID_ANY ),
          m_colour(*wxBLACK)//, m_font(*wxNORMAL_FONT)
{
}

void FontDisplayCanvas::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
    wxPaintDC dc(this);
    PrepareDC(dc);

    // set background
    dc.SetBackground(wxBrush(wxT("white"), wxSOLID)); // white background
	//dc.SetBackground(wxBrush(wxColour(255,255,125), wxSOLID)); // yellow background
    dc.Clear();

    // the current text origin
    wxCoord startCoordx = 5,
            startCoordy = 5;

    // prepare to draw the font
    dc.SetFont(m_font);
	if (!m_colour.IsOk())
	{
		::wxBell();
		wxASSERT(FALSE);
	}
    dc.SetTextForeground(m_colour);

    // the size of one cell (Normally biggest char + small margin)
    wxCoord maxCharWidth, maxCharHeight;
    dc.GetTextExtent(wxT("W"), &maxCharWidth, &maxCharHeight);
    int cellWidth = maxCharWidth + 5,
        cellHeight = maxCharHeight + 4;

	// print all font symbols from 32 to 256 in 7 rows of 32 chars each
	int rowCount;
	int columnCount;
	int pointNum = 0;
	for (rowCount = 0; rowCount < 7; rowCount++)
    {
        for (columnCount = 0; columnCount < 32; columnCount++)
        {
            wxChar ch = (wxChar)(32 * (rowCount + 1) + columnCount);
			if (ch == typedChar)
			{
				// Highlight the cell of the typedChar (if it is not wxChar(0)).
				// This paint event is triggered because of a typed character.
				// Draw a darker rectangle around the chart cell containing the typedChar which also has
				// a yellow background fill.
				//dc.SetBackgroundMode(wxSOLID);
				//dc.SetTextBackground(wxColour(255,255,0)); // yellow background
				//dc.Clear();
				//dc.SetBackground(wxBrush(wxColour(255,255,125), wxSOLID)); // yellow brush for the rectangle's fill color
				dc.SetPen(wxPen(wxColour(*wxRED), 3, wxSOLID)); // red outline 3 pixels thick for the rectangle
				wxColour clr(255, 255, 128);
				wxBrush yellowBrush(clr, wxSOLID);
				dc.SetBrush(yellowBrush);
				dc.DrawRectangle(startCoordx + cellWidth*columnCount-3,startCoordy + cellHeight*rowCount-3,
					cellWidth+3,cellHeight+3);
				// the pen is set below for drawing the regular matrix lines 
			}
			wxCoord charWidth, charHeight;
 			if (rowCount > 2) // rows 3 through 6, if UTF-8 font is selected cannot be displayed in non-Unicode version
			{
				if (gthisFontsCurrEncoding == wxFONTENCODING_UTF8)
				{
					// for a UTF-8 encoding we don't display the extended 8-bit characters as they
					// don't make sense, so just draw a space char
					ch = _T(' ');
					dc.GetTextExtent(ch, &charWidth, &charHeight);
					dc.DrawText
					(
						ch,
						startCoordx + cellWidth*columnCount + (maxCharWidth - charWidth) / 2 + 1,
						startCoordy + cellHeight*rowCount + (maxCharHeight - charHeight) / 2
					);
				}
				else
				{
					// use the actual char in the chart
					dc.GetTextExtent(ch, &charWidth, &charHeight);
					dc.DrawText
					(
						ch,
						startCoordx + cellWidth*columnCount + (maxCharWidth - charWidth) / 2 + 1,
						startCoordy + cellHeight*rowCount + (maxCharHeight - charHeight) / 2
					);
				}
			}
			else
			{
				// use the actual char in the chart
				dc.GetTextExtent(ch, &charWidth, &charHeight);
				dc.DrawText
				(
					ch,
					startCoordx + cellWidth*columnCount + (maxCharWidth - charWidth) / 2 + 1,
					startCoordy + cellHeight*rowCount + (maxCharHeight - charHeight) / 2
				);
			}
			pointNum++;
			dc.SetBrush(*wxWHITE_BRUSH);
        }
    }

    // draw the lines between them in blue
    dc.SetPen(wxPen(wxColour(*wxLIGHT_GREY), 1, wxSOLID)); // light grey lines
    int lineCount;

    // horizontal
    for ( lineCount = 0; lineCount < 8; lineCount++ )
    {
        int lineCoordy = startCoordy + cellHeight*lineCount - 2;
        dc.DrawLine(startCoordx - 2, lineCoordy, startCoordx + 32*cellWidth - 1, lineCoordy);
    }

    // and vertical
    for ( lineCount = 0; lineCount < 33; lineCount++ )
    {
        int lineCoordx = startCoordx + cellWidth*lineCount - 2;
        dc.DrawLine(lineCoordx, startCoordy - 2, lineCoordx, startCoordy + 7*cellHeight - 1);
    }

	typedChar = _T('\0'); // set the typed char to null for regular paint events
	dc.SetBrush(wxNullBrush);
	dc.SetPen(wxNullPen);
}

wxString CSetEncodingDlg::GetEncodingSymbolFromListStr(wxString listStr)
{
	// remove the description part of the firstEncStrInList, leaving only the wxFONTENCODING... symbol 
	int posBracket = listStr.Find(_T('['),TRUE); // TRUE finds pos from end
	wxASSERT(listStr.GetChar(posBracket -1) == _T(' ')); //should be a space before the [
	listStr = listStr.Mid(0,posBracket -1);
	return listStr;
}
