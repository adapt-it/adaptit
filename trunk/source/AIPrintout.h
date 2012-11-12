/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AIPrintout.h
/// \author			Bill Martin
/// \date_created	12 February 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the AIPrintout class.
/// The AIPrintout class manages the functions for printing and print previewing from
/// within Adapt It using the File | Print and File | Print Preview menu selections.
/// \derivation		The AIPrintout class is derived from AIPrintout.
/////////////////////////////////////////////////////////////////////////////

#ifndef AIPrintout_h
#define AIPrintout_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "AIPrintout.h"
#endif

// The AIPrPreviewFrame class below /////////////////////////////////////////////////////////////

// These class methods can be uncommented if it is necessary to declare the AIPreviewFrame class in
// order to access the preview frame's methods such as OnCloseWindow().

// The AIPreviewFrame class functions primarily as a hook to be able to detect when the print preview frame
// closes so that the document and view cleanup can be performed.
// \derivation		The AIPreviewFrame class is derived from wxPreviewFrame.
//class AIPreviewFrame: public wxPreviewFrame
//{
//public:
//	AIPreviewFrame(wxPrintPreview* preview, wxWindow* parent, const wxString& title,
//		const wxPoint& pos, const wxSize& size);
//	void OnCloseWindow(wxCloseEvent& event);
//private:
//	// class attributes
//    DECLARE_EVENT_TABLE(); // MFC uses DECLARE_MESSAGE_MAP()
//};

// End of AIPrPreviewFrame class  /////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
/// The AIPrintout class manages the printing of Adapt It Documents from within
/// the doc/view framework.
/// \derivation		The AIPrintout class is derived from wxPrintout.
class CAdapt_ItApp;
class AIPrintout: public wxPrintout
{
 public:
	AIPrintout(const wxChar *title = _T("Adapt It Printout"));
	virtual ~AIPrintout();

	void OnPreparePrinting();
	void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo);
	void OnBeginPrinting();
	bool OnBeginDocument(int startPage, int endPage);
	bool HasPage(int page);
	bool OnPrintPage(int page);
	void OnEndPrinting();

private:
	// class attributes
	CAdapt_ItApp* m_pApp;

	//DECLARE_CLASS(AIPrintout);
	// Used inside a class declaration to declare that the class should
	// be made known to the class hierarchy, but objects of this class
	// cannot be created dynamically. The same as DECLARE_ABSTRACT_CLASS.

	// or, comment out above and uncomment below to
	DECLARE_DYNAMIC_CLASS(AIPrintout)
	// Used inside a class declaration to declare that the objects of
	// this class should be dynamically creatable from run-time type
	// information. MFC uses DECLARE_DYNCREATE(AIPrintout)
};

#endif /* AIPrintout_h */
