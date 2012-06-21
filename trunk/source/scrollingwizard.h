///////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/wizard.h
// Purpose:     declaration of generic wxWizard class
// Author:      Vadim Zeitlin
// Modified by: Robert Vazan (sizers)
// Created:     28.09.99
// RCS-ID:      $Id: scrollingwizard.h,v 1.1 2008/01/06 13:57:34 anthemion Exp $
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
// Licence:     New BSD License
///////////////////////////////////////////////////////////////////////////////

/*

To change in wxWizard in 2.9:

  - References to wxWizard in wxWizardPage should be changed to wxWizardBase
  - It should be possible to derive from wxWizard and reimplement some functionality:
    add accessors change private to protected.

Problem: page size is calculated using a bitmap that may not be the full width.

 */

#ifndef _WX_SCROLLING_WIZARD_H_
#define _WX_SCROLLING_WIZARD_H_

// whm 14Jun12 modified to use wxDialog for wxWidgets 2.9.x and later; wxScrollingDialog for pre-2.9.x
#if wxCHECK_VERSION(2,9,0)
// For wxWidgets 2.9.x and later do not compile this file into project, because
// the scrolling wizard functionality is built-in to the main wxWidgets library.
#else


#include "wx/wizard.h"

#include "scrollingdialog.h" // #include "uiutils/scrollingdialog.h"

// ----------------------------------------------------------------------------
// wxWizard
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxStaticBitmap;
class WXDLLIMPEXP_FWD_ADV wxWizardEvent;
class WXDLLIMPEXP_FWD_CORE wxBoxSizer;
class wxScrollingWizardSizer;

// Placement flags
#define wxWIZARD_VALIGN_TOP       0x01
#define wxWIZARD_VALIGN_CENTRE    0x02
#define wxWIZARD_VALIGN_BOTTOM    0x04
#define wxWIZARD_HALIGN_LEFT      0x08
#define wxWIZARD_HALIGN_CENTRE    0x10
#define wxWIZARD_HALIGN_RIGHT     0x20
#define wxWIZARD_TILE             0x40

class wxScrollingWizard : public wxWizard, public wxDialogHelper
{
public:
    // ctor
    wxScrollingWizard() { Init(); }
    wxScrollingWizard(wxWindow *parent,
             int id = wxID_ANY,
             const wxString& title = wxEmptyString,
             const wxBitmap& bitmap = wxNullBitmap,
             const wxPoint& pos = wxDefaultPosition,
             long style = wxDEFAULT_DIALOG_STYLE)
    {
        Init();
        Create(parent, id, title, bitmap, pos, style);
    }
    bool Create(wxWindow *parent,
             int id = wxID_ANY,
             const wxString& title = wxEmptyString,
             const wxBitmap& bitmap = wxNullBitmap,
             const wxPoint& pos = wxDefaultPosition,
             long style = wxDEFAULT_DIALOG_STYLE);
    void Init();
    virtual ~wxScrollingWizard();

    // implement base class pure virtuals
    virtual bool RunWizard(wxWizardPage *firstPage);
    virtual wxWizardPage *GetCurrentPage() const;
    virtual void SetPageSize(const wxSize& size);
    virtual wxSize GetPageSize() const;
    virtual void FitToPage(const wxWizardPage *firstPage);
    virtual wxSizer *GetPageAreaSizer() const;
    virtual void SetBorder(int border);

    /// set/get bitmap
    const wxBitmap& GetBitmap() const { return m_bitmap; }
    void SetBitmap(const wxBitmap& bitmap);

    // implementation only from now on
    // -------------------------------

    // is the wizard running?
    bool IsRunning() const { return m_page != NULL; }

    // show the prev/next page, but call TransferDataFromWindow on the current
    // page first and return false without changing the page if
    // TransferDataFromWindow() returns false - otherwise, returns true
    bool ShowPage(wxWizardPage *page, bool goingForward = true);

    // do fill the dialog with controls
    // this is app-overridable to, for example, set help and tooltip text
    virtual void DoCreateControls();

    /// Do the adaptation
    virtual bool DoLayoutAdaptation();

    /// Set/get bitmap background colour
    void SetBitmapBackgroundColour(const wxColour& colour) { m_bitmapBackgroundColour = colour; }
    const wxColour& GetBitmapBackgroundColour() const { return m_bitmapBackgroundColour; }

    /// Set/get bitmap placement (centred, tiled etc.)
    void SetBitmapPlacement(int placement) { m_bitmapPlacement = placement; }
    int GetBitmapPlacement() const { return m_bitmapPlacement; }

    /// Set/get minimum bitmap width
    void SetMinimumBitmapWidth(int w) { m_bitmapMinimumWidth = w; }
    int GetMinimumBitmapWidth() const { return m_bitmapMinimumWidth; }

    /// Tile bitmap
    static bool TileBitmap(const wxRect& rect, wxDC& dc, const wxBitmap& bitmap);

protected:
    // for compatibility only, doesn't do anything any more
    void FinishLayout() { }

    /// Do fit, and adjust to screen size if necessary
    virtual void DoWizardLayout();

    /// Resize bitmap if necessary
    virtual bool ResizeBitmap(wxBitmap& bmp);

    // was the dialog really created?
    bool WasCreated() const { return m_btnPrev != NULL; }

    // event handlers
    void OnCancel(wxCommandEvent& event);
    void OnBackOrNext(wxCommandEvent& event);
    void OnHelp(wxCommandEvent& event);

    void OnWizEvent(wxWizardEvent& event);

    void AddBitmapRow(wxBoxSizer *mainColumn);
    void AddStaticLine(wxBoxSizer *mainColumn);
    void AddBackNextPair(wxBoxSizer *buttonRow);
    void AddButtonRow(wxBoxSizer *mainColumn);

    // the page size requested by user
    wxSize m_sizePage;

    // the dialog position from the ctor
    wxPoint m_posWizard;

    // wizard state
    wxWizardPage *m_page;       // the current page or NULL
    wxBitmap      m_bitmap;     // the default bitmap to show

    // wizard controls
    wxButton    *m_btnPrev,     // the "<Back" button
                *m_btnNext;     // the "Next>" or "Finish" button
    wxStaticBitmap *m_statbmp;  // the control for the bitmap

    // Border around page area sizer requested using SetBorder()
    int m_border;

    // Whether RunWizard() was called
    bool m_started;

    // Whether was modal (modeless has to be destroyed on finish or cancel)
    bool m_wasModal;

    // True if pages are laid out using the sizer
    bool m_usingSizer;

    // Page area sizer will be inserted here with padding
    wxBoxSizer *m_sizerBmpAndPage;

    // Actual position and size of pages
    wxScrollingWizardSizer *m_sizerPage;

    wxColour    m_bitmapBackgroundColour;
    int         m_bitmapPlacement;
    int         m_bitmapMinimumWidth;

    friend class wxScrollingWizardSizer;

    DECLARE_DYNAMIC_CLASS(wxScrollingWizard)
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxScrollingWizard)
};

// ----------------------------------------------------------------------------
// wxWizardSizer
// ----------------------------------------------------------------------------

class wxScrollingWizardSizer : public wxSizer
{
public:
    wxScrollingWizardSizer(wxScrollingWizard *owner);

    virtual wxSizerItem *Insert(size_t index, wxSizerItem *item);

    virtual void RecalcSizes();
    virtual wxSize CalcMin();

    // get the max size of all wizard pages
    wxSize GetMaxChildSize();

    // return the border which can be either set using wxWizard::SetBorder() or
    // have default value
    int GetBorder() const;

    // hide the pages which we temporarily "show" when they're added to this
    // sizer (see Insert())
    void HidePages();

private:
    wxSize SiblingSize(wxSizerItem *child);

    wxScrollingWizard *m_owner;
    wxSize m_childSize;
};


#endif // _WX_SCROLLING_WIZARD_H_

#endif //#if wxCHECK_VERSION(2,9,0)
