/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			MainFrm.h
/// \author			Bill Martin
/// \date_created	05 January 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This header file defines the wxWidgets
/// The CMainFrame class defines Adapt It's basic interface, including its menu bar,
/// tool bar, control bar, compose bar, and status bar.
/// \derivation		The CMainFrame class is derived from wxDocParentFrame and inherits
/// its support for the document/view framework.
/////////////////////////////////////////////////////////////////////////////

//#define KEY_2_KLUGE

#ifndef MainFrame_h
#define MainFrame_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "MainFrm.h"
#endif

//#include <wx/help.h> //(wxWidgets chooses the appropriate help controller class)
#include <wx/html/helpctrl.h> //(wxHTML based help controller: wxHtmlHelpController)

// forward declarations
class CAdapt_ItCanvas;
class wxHelpControllerBase;
class wxHtmlHelpController;
class CFreeTrans;
class CNotes;

// global functions (FormatScriptureReference() is overloaded)
class CSourcePhrase;

// Custom Event Declarations:
// MFC version: BEW added 15July08; definitions for custom events used in the vertical edit process, which typically
// starts with a source text edit, but in the wxWidgets based apps it could also be an adaptation edit,
// gloss edit, or free translation edit that starts it off; these events are UINT
//UINT CUSTOM_EVENT_ADAPTATIONS_EDIT = RegisterWindowMessage(_T("CustomEventAdaptationsEdit"));
//UINT CUSTOM_EVENT_FREE_TRANSLATIONS_EDIT = RegisterWindowMessage(_T("CustomEventFreeTranslationsEdit"));
//UINT CUSTOM_EVENT_BACK_TRANSLATIONS_EDIT = RegisterWindowMessage(_T("CustomEventBackTranslationsEdit"));
//UINT CUSTOM_EVENT_COLLECTED_BACK_TRANSLATIONS_EDIT
//							= RegisterWindowMessage(_T("CustomEventVCollectedBackTranslationsEdit")); // unused
//UINT CUSTOM_EVENT_END_VERTICAL_EDIT = RegisterWindowMessage(_T("CustomEventEndVerticalEdit"));
//UINT CUSTOM_EVENT_CANCEL_VERTICAL_EDIT = RegisterWindowMessage(_T("CustomEventCancelVerticalEdit"));
//UINT CUSTOM_EVENT_GLOSSES_EDIT = RegisterWindowMessage(_T("CustomEventGlossesEdit"));
// wxWidgets uses the following macros to set up the custom events:
BEGIN_DECLARE_EVENT_TYPES()
DECLARE_EVENT_TYPE(wxEVT_Adaptations_Edit, -1)
DECLARE_EVENT_TYPE(wxEVT_Free_Translations_Edit, -1)
DECLARE_EVENT_TYPE(wxEVT_Back_Translations_Edit, -1)
DECLARE_EVENT_TYPE(wxEVT_End_Vertical_Edit, -1)
DECLARE_EVENT_TYPE(wxEVT_Cancel_Vertical_Edit, -1)
DECLARE_EVENT_TYPE(wxEVT_Glosses_Edit, -1)
END_DECLARE_EVENT_TYPES()


//void SyncScrollSend(const CString& strThreeLetterBook, int nChap, int nVerse); // Bob's original function
void SyncScrollSend(const wxString& strThreeLetterBook, const wxString& strChapVerse); // my preferred signature
// whm changed SyncScrollReceive below to return a bool of TRUE if successful otherwise FALSE
bool SyncScrollReceive(const wxString& strThreeLetterBook, int nChap, int nVerse, const wxString& strChapVerse);
void FormatScriptureReference(const wxString& strThreeLetterBook, int nChap, int nVerse, wxString& strMsg);
void FormatScriptureReference(const wxString& strThreeLetterBook, const wxString& strChapVerse, wxString& strMsg);
void ExtractScriptureReferenceParticulars(wxString& strSantaFeMsg, wxString& str3LetterCode,
										  wxString& strChapVerse, int& nChapter, int& nVerse);
bool CheckBibleBookCode(wxArrayPtrVoid* pBooksArray, wxString& str3LetterCode);
bool Get3LetterCode(SPList* pList, wxString& strBookCode);
wxString MakeChapVerseStrForSynchronizedScroll(wxString& chapVerseStr);
int FindChapterVerseLocation(SPList* pDocList, int nChap, int nVerse, const wxString& strChapVerse);
void Code2BookFolderName(wxArrayPtrVoid* pBooksArray, const wxString& strBookCode, wxString& strFolderName,
						 int& nBookIndex);
//void DeleteSourcePhrases_ForSyncScrollReceive(CAdapt_ItDoc* pDoc, SPList* pList); // whm 27May11 removed
																					//no longer needed

/// The CMainFrame class defines Adapt It's basic interface, including its menu bar,
/// tool bar, control bar, compose bar, and status bar.
/// \derivation		The CMainFrame class is derived from wxDocParentFrame and inherits
/// its support for the document/view framework.
class CMainFrame : public wxDocParentFrame
{
	// MFC version MainFrame constructor has two virtual functions OnCreateClient()
	// and PreCreateWindow(). OnCreateClient() is used to create a m_wndSplitter window
	// wxWidgets has wxSplitterWindow. After some tinkering around, I still don't see
	// how to implement a wxSplitterWindow in the doc/view framework, since the window
	// associated with the view need to have m_wndSplitter as its parent, whereas in
	// the doc/view framework the canvas has MDIChildWindow as its parent. The splitter
	// window functionality is probably not something commonly used, so I won't
	// implement it in the wxWidgets version at this point
	////protected:
	//wxSplitterWindow* m_wndSplitter;
	public:

#ifdef _USE_SPLITTER_WINDOW
	wxSplitterWindow *splitter;
#endif

	CMainFrame(wxDocManager* manager, wxFrame* frame, wxWindowID id,
			const wxString& title,
			const wxPoint& pos, const wxSize& size, const long type);
	~CMainFrame(); // an explicit destructor, so the removed administrator menu can
				   // be restored before destruction of the frame

	// MFC Overrides
	// MFC ClassWizard generated virtual function overrides below
	//public:
	//virtual bool OnCreateClient(); // MFC uses this to create the splitter window
	//virtual bool PreCreateWindow(CREATESTRUCT& cs);
	// The MFC version overrides this PreCreateWindow() virtual function in both
	// CMainFrame and CAdapt_ItView, but the overrides don't extend/change anything,
	// rather they just call their base class constructors, so we don't need to
	// worry about it in the wxWidgets version
	//virtual void AssertValid() const;
	// The CMainFrame, CAdapt_ItView, and CAdapt_ItDoc all have overrides to this
	// AssertValid() function, but the overrides don't extend/change anything,
	// but just call their base class constructors, so we don't need to worry
	// about it in the wxWidgets version

	wxMenuBar* m_pMenuBar;

	wxStatusBar* m_pStatusBar;	// handle/pointer to the statusBar
	AIToolBar* m_pToolBar;		// handle/pointer to the toolBar
	wxPanel* m_pControlBar;		// handle/pointer to the controlBar
	wxPanel* m_pComposeBar;		// handle/pointer to the composeBar
	wxPanel* m_pRemovalsBar;	// whm added for 12Sep08 refactored source text editing
	wxPanel* m_pVertEditBar;	// whm added for 12Sep08 refactored source text editing
	//wxPanel* m_pVertEditStepTransBar; // whm added for 12Sep08 refactored source text editing
	wxTextCtrl* m_pComposeBarEditBox;
	wxComboBox* m_pRemovalsBarComboBox;
	wxTextCtrl* m_pVertEditMsgBox;
	//wxTextCtrl* m_pVertEditStepTransMsgBox;
	//
	wxSizer* pRemovalsBarSizer;
	wxSizer* pVertEditBarSizer;

	int m_toolBarHeight;		// determined in CMainFrame constructor after toolBar is created
	int m_controlBarHeight;		// determined in CMainFrame constructor after controlBar is created
	int m_composeBarHeight;		// determined in CMainFrame constructor after composeBar is created
	int m_statusBarHeight;
	int m_removalsBarHeight;
	int m_vertEditBarHeight;
	//int m_vertEditStepTransBarHeight;

	//wxHtmlHelpController m_help;

    void OnAppAbout(wxCommandEvent& WXUNUSED(event));
	// OnIdle moved here from the App. When it was in the App it was causing
	// the File | Exit and App x cancel commands to not be responsive there
	int idleCount;
	bool m_bShowScrollData;

	//bool m_bUsingHighResDPIScreen;

	void OnIdle(wxIdleEvent& event); // MFC is virtual and returns BOOL


	CAdapt_ItCanvas *canvas;	// The MainFrame holds the main pointer to our canvas
								// Note: The View also holds its own pointer to this canvas

	// whm: See font.cpp sample for example of creating a wxSplitterWindow
#ifdef _USE_SPLITTER_WINDOW
	CAdapt_ItCanvas *CreateCanvas(wxSplitterWindow *parent);
#else
	CAdapt_ItCanvas *CreateCanvas(CMainFrame *parent);
#endif
	wxSize GetCanvasClientSize(); // whm added 24Feb07

    void OnViewToolBar(wxCommandEvent& WXUNUSED(event));
    void OnUpdateViewToolBar(wxUpdateUIEvent& event);
    void OnViewStatusBar(wxCommandEvent& WXUNUSED(event));
    void OnUpdateViewStatusBar(wxUpdateUIEvent& event);
    void OnViewComposeBar(wxCommandEvent& WXUNUSED(event));
    void OnViewModeBar(wxCommandEvent& WXUNUSED(event));
	void OnViewAdminMenu(wxCommandEvent& WXUNUSED(event));
	void OnUpdateViewAdminMenu(wxUpdateUIEvent& event);
	void ComposeBarGuts(enum composeBarViewSwitch composeBarVisibility);
	void OnUpdateViewComposeBar(wxUpdateUIEvent& event);
	void OnUpdateViewModeBar(wxUpdateUIEvent& event);
	void OnActivate(wxActivateEvent& event);
	void OnSize(wxSizeEvent& WXUNUSED(event));
	void OnRemovalsComboSelChange(wxCommandEvent& WXUNUSED(event));

	//void OnHelp(wxHelpEvent& WXUNUSED(event));
	void OnAdvancedHtmlHelp(wxCommandEvent& WXUNUSED(event));
	void OnQuickStartHelp(wxCommandEvent& WXUNUSED(event));
	//void OnOnlineHelp(wxCommandEvent& WXUNUSED(event));
	//void OnUserForum(wxCommandEvent& WXUNUSED(event));
	void OnHelpReportAProblem(wxCommandEvent& WXUNUSED(event));
	void OnHelpGiveFeedback(wxCommandEvent& WXUNUSED(event));
	void OnUseToolTips(wxCommandEvent& WXUNUSED(event));
	void OnUpdateUseToolTips(wxUpdateUIEvent& event);
	void OnSetToolTipDelayTime(wxCommandEvent& WXUNUSED(event));
	void OnUpdateSetToolTipDelayTime(wxUpdateUIEvent& event);
	//void OnShowContextHelp(wxCommandEvent& event);

	// support Mike's TEST_DVCS menu item
#if defined(TEST_DVCS)
	void OnTestDVCS(wxCommandEvent& WXUNUSED(event));
	void OnUpdateTestDVCS(wxUpdateUIEvent& event);
#endif

    //void ShowHelp(int commandId, wxHelpControllerBase& helpController);
	void OnClose(wxCloseEvent& event);
	// UI handlers for the checkboxes in the controlBar
	void OnUpdateCheckKBSave(wxUpdateUIEvent& event);
	void OnUpdateCheckForceAsk(wxUpdateUIEvent& event);
	void OnUpdateCheckSingleStep(wxUpdateUIEvent& event);
#ifdef __WXMSW__
	bool DoSantaFeFocus(WXWPARAM wParam, WXLPARAM WXUNUSED(lParam));
	WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam); // we use this one in wx
#endif

	// Declare custom events for vertical edit process
	void OnCustomEventAdaptationsEdit(wxCommandEvent& WXUNUSED(event));
	void OnCustomEventGlossesEdit(wxCommandEvent& WXUNUSED(event));
	void OnCustomEventFreeTranslationsEdit(wxCommandEvent& WXUNUSED(event));
	void OnCustomEventBackTranslationsEdit(wxCommandEvent& WXUNUSED(event));
	void OnCustomEventEndVerticalEdit(wxCommandEvent& WXUNUSED(event));
	void OnCustomEventCancelVerticalEdit(wxCommandEvent& WXUNUSED(event));

    void RecreateToolBar();
	AIToolBar* GetToolBar();
	void DoCreateStatusBar();
	void OnMRUFile(wxCommandEvent& event);

  private:
	//wxHelpController		m_help;

   DECLARE_CLASS(CMainFrame);
	// Used inside a class declaration to declare that the class should
	// be made known to the class hierarchy, but objects of this class
	// cannot be created dynamically. The same as DECLARE_ABSTRACT_CLASS.
	// In the MFC version MainFrm.h uses DECLARE_DYNCREATE() which is
	// normally equivalent to the wxWidgets' DECLARE_DYNAMIC_CLASS() macro.
	// The MDI sample I modeled this after uses the non-dynamic macro.
	// Should we actually use the DECLARE_DYNAMIC_CLASS() macro here?
	// Probably not, since only a single object of the CMainFrame class
	// is created at the beginning of the program and we never need to
	// dynamically create any more objects of that class. The MFC version
    // probably didn't need the dynamic class-creation macro either.

    DECLARE_EVENT_TABLE(); // MFC uses DECLARE_MESSAGE_MAP()
};

#endif /* MainFrame_h */
