/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			WaitDlg.cpp
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CWaitDlg class.
/// The CWaitDlg class provides a custom "Please wait" dialog to notify the
/// user that the current process will take some time to complete.
/// The CWaitDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// BEW note, 4Feb2016: .Show(true) on Linux only shows the dialog frame, with no content visible
/// \derivation		The CWaitDlg class is derived from wxDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "WaitDlg.h"
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
//#include <wx/animate.h>
#include <wx/window.h>

#include "Adapt_It.h"
#include "Adapt_ItView.h"
#include "WaitDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg dialog

// event handler table
BEGIN_EVENT_TABLE(CWaitDlg, wxDialog)
EVT_INIT_DIALOG(CWaitDlg::InitDialog)
END_EVENT_TABLE()

CWaitDlg::CWaitDlg(wxWindow* parent) // dialog constructor
	: wxDialog(parent, -1, _("Please Wait..."),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
				, m_nWaitMsgNum(0)
{
	pWaitDlgSizer = WaitDlgFunc(this, TRUE, TRUE);
	// This dialog function is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	// The declaration is: WaitDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	pStaticText = (wxStaticText*)FindWindowById(IDC_PLEASE_WAIT);

	// whm 24Aug11 Note: The following could be used to put an animated
	// busy image, such as the throbber.gif used in a wxWidgets sample.
	// However, it is a bit difficult to get it to actually animate under
	// highly intemsive cpu activities which is usually the case with
	// situations needing a wait dialog. It could be done with a timer
	// utilizing calls to ::wxSafeYield() at the pre-determined time
	// ticks.
	//pAnimatedPanel = (wxPanel*)FindWindowById(ID_PANEL_ANIMATION);
	//wxASSERT(pAnimatedPanel != NULL);

	m_bNoTitle = FALSE;
	
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(m_pApp != NULL);
}
//#if defined(_KBSERVER)
CWaitDlg::CWaitDlg(wxWindow* parent, bool bNoTitle) // dialog constructor
	: wxDialog(parent, -1, _T(""),
	wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, m_nWaitMsgNum(0)
{
	pWaitDlgSizer = WaitDlgFunc(this, TRUE, TRUE);
	// This dialog function is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	// The declaration is: WaitDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

	pStaticText = (wxStaticText*)FindWindowById(IDC_PLEASE_WAIT);

	// whm 24Aug11 Note: The following could be used to put an animated
	// busy image, such as the throbber.gif used in a wxWidgets sample.
	// However, it is a bit difficult to get it to actually animate under
	// highly intemsive cpu activities which is usually the case with
	// situations needing a wait dialog. It could be done with a timer
	// utilizing calls to ::wxSafeYield() at the pre-determined time
	// ticks.
	//pAnimatedPanel = (wxPanel*)FindWindowById(ID_PANEL_ANIMATION);
	//wxASSERT(pAnimatedPanel != NULL);

	m_bNoTitle = bNoTitle;

	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(m_pApp != NULL);
}
//#endif
CWaitDlg::~CWaitDlg()
{
//#if defined(_KBSERVER)
	m_pApp->m_pWaitDlg = NULL;
//#endif
}

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg message handlers
void CWaitDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	// whm 24Aug11 Note: The following could be used to put an animated
	// busy image, such as the throbber.gif used in a wxWidgets sample.
	// However, it is a bit difficult to get it to actually animate under
	// highly intemsive cpu activities which is usually the case with
	// situations needing a wait dialog. It could be done with a timer
	// utilizing calls to ::wxSafeYield() at the pre-determined time
	// ticks.

    //m_pAnimationCtrl = new wxAnimationCtrl(pAnimatedPanel, wxID_ANY);
	// TODO: Use appropriate path for platform!
	//wxString m_throbberPathAndName = m_pApp->m_appInstallPathOnly + m_pApp->PathSeparator + _T("throbber.gif");
	// load animated image into a sizer created within pAnimatedPanel
	//if (m_pAnimationCtrl->LoadFile(m_throbberPathAndName))
    //    m_pAnimationCtrl->Play();
    
	switch (m_nWaitMsgNum)
	{
		// whm 28Aug11 commented out most of these since they are no longer needed
		// after implementing more instances of the wxProgressDialog.
		//case 0:
		//	WaitMsg = _("Please wait while Adapt It restores the knowledge base...");
		//	break;
		//case 1:
		//	// IDS_WAIT_FOR_RTF_OUTPUT
		//	WaitMsg = _("Please wait for Adapt It to output the RTF file. This may take a while...");
		//	break;
		case 2:
		{
		WaitMsg = _("Please wait while Adapt It opens the document...");
		}
			break;
		//case 3:
		//	WaitMsg = _("Please wait while Adapt It processes filter changes...");
		//	break;
		//case 4:
		//	WaitMsg = _("Please wait while Adapt It saves the File...");
		//	break;
		case 5: // whm 28Aug11 Note: May be useful somewhere
		{
			WaitMsg = _T("");
			pStaticText->Hide(); // this selection just hides the static text message leaving the Title "Please Wait..."
		}
			break;
		//case 6:
		//	WaitMsg = _("Please wait while Adapt It saves the KB...");
		//	break;
		//case 7:
		//	WaitMsg = _("Please wait while Adapt It saves the Glossing KB...");
		//	break;
		//case 8:
		//	WaitMsg = _("Please wait while Adapt It loads the KB...");
		//	break;
		//case 9:
		//	WaitMsg = _("Please wait while Adapt It loads the Glossing KB...");
		//	break;
		//case 10:
		//	WaitMsg = _("Please wait while Adapt It backs up the KB...");
		//	break;
		//case 11:
		//	WaitMsg = _("Please wait while Adapt It backs up the Glossing KB...");
		//	break;
		//case 12:
		//	WaitMsg = _("Please wait while Adapt It prepares the document for printing");
		//	break;
		case 13: // whm 28Aug11 Note: this is only used in KBEditSearch::InitDialog()
		{
			WaitMsg = _("Searching...");
		}
			break;
		//case 14:
		//	WaitMsg = _("Please wait while Adapt It exports the KB...");
		//	break;
		case 15:
			WaitMsg = _("Please wait while Adapt It communicates with Paratext...");
			break;
		case 16:
			WaitMsg = _("Please wait while Adapt It communicates with Bibledit...");
			break;
		//case 17:
		//	WaitMsg = _("Please wait while the free translation is sent to Paratext...");
		//	break;
		case 18:
		{
			WaitMsg = _("Please wait while Adapt It exports the source text...");
		}
			break;
		//case 19:
		//	WaitMsg = _("Exporting the translation...");
		//	break;
		//case 20:
		//	WaitMsg = _("Exporting the free translation...");
		//	break;
		//case 21:
		//	WaitMsg = _("Please wait, getting the chapter and laying out the document...");
		//	break;
		case 22:  // whm 28Aug11 Note: This is used in OnCloseDocument()
		{
			WaitMsg = _("Closing the document...");
		}
			break;
		case 23:  // BEW 4Sep15 this is used in DoGlobalRestoreOfSaveToKB() within view's OnCheckKBSave()
			WaitMsg = _("This may take a while. Identical changes are being done in all the documents...");
			break;
//#if defined(_KBSERVER)
		case 24: // feedback to user that the running KBserver was connected to successfully
		{
			WaitMsg = _("Connected to KBserver successfully");
		}
			break;
		case 25: // feedback to the user that there is now no connection to a KBserver for KB sharing
		{
			WaitMsg = _("Knowledge base sharing is OFF");
		}
			break;
		case 26: // a wait message so that the user knows KBserver discovery is taking place
		{
			WaitMsg = _("Discovery of running KBservers is happening...");
		}
			break;
//#endif
		case 27: // a wait message if the user turns on the diagnostic log file creation for a doc parse failure
		{
			//WaitMsg = _("Retrying, and making a log file in folder _LOGS_EMAIL_REPORTS. It is slow...");
			// whm 6Apr2020 reformulating the wait message below. Previously this message was presented
			// due to the long time it took to generate the doc creation log file. Log file creation now
			// however, is not the source of the delay in creating a new document in OnNewDocument, rather
			// it is the time taken to process TokenizeText(). Hence, I'm making this waitMsg a more
			// general one as shown below
			WaitMsg = _("Please wait while creating a new document...");
		}
            break;
		case 28: // a wait message if the user enters or leaves "See Glosses" mode from or to adapting mode, respectively
		{
			WaitMsg = _("Please wait: all widths are being resized, then strips created again");
		}
			break;
        case 29:
		{
			WaitMsg = _("Please wait while creating a new document - and creating a diagnostic log file in folder _LOGS_EMAIL_REPORTS...");
		}
            break;
		case 30:
		{
			WaitMsg = _("Adding a user to kbserver succeeded.");
		}
			break;
		case 31:
		{
			WaitMsg = _("Adding a user to kbserver failed.");
		}
			break;
		case 32:
		{
			WaitMsg = _("Authentication succeeded.");
		}
			break;
		case 33:
		{
			WaitMsg = _("Authentication failed.");
		}
			break;
		default: // whm 28Aug11 Note: keep as a default message
		{
			WaitMsg = _("Please wait. This may take a while...");
		}
	} // end of switch (m_nWaitMsgNum)

	// We take control of setting the window's size, based on the extents of the text
	// within it
	pStaticText->SetLabel(WaitMsg);
	pStaticText->Update();
	pWaitDlgSizer->Layout();
//#if defined(_KBSERVER)
	if (m_bNoTitle)
	{
		m_pApp->GetView()->PositionDlgNearBottomRight(this);
	}
//#endif

    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
    m_pApp->m_bUserDlgOrMessageRequested = TRUE;

}

