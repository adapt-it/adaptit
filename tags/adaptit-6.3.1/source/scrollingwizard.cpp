/////////////////////////////////////////////////////
// Name:        src/generic/wizard.cpp
// Purpose:     generic implementation of wxWizard class
// Author:      Vadim Zeitlin
// Modified by: Robert Cavanaugh
//              1) Added capability for wxWizardPage to accept resources
//              2) Added "Help" button handler stub
//              3) Fixed ShowPage() bug on displaying bitmaps
//              Robert Vazan (sizers)
// Created:     15.08.99
// RCS-ID:      $Id: scrollingwizard.cpp,v 1.2 2008/01/11 15:08:33 anthemion Exp $
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
// Licence:     New BSD License
/////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wx.h"

#if wxUSE_WIZARDDLG

#ifndef WX_PRECOMP
    #include "wx/dynarray.h"
    #include "wx/intl.h"
    #include "wx/statbmp.h"
    #include "wx/button.h"
    #include "wx/settings.h"
    #include "wx/sizer.h"
#endif //WX_PRECOMP

#include "wx/statline.h"
#include "wx/dcmemory.h"

#include "scrollingwizard.h" // #include "uiutils/scrollingwizard.h"

// whm 14Jun12 modified to use wxWizard for wxWidgets 2.9.x and later; wxScrollingWizard for pre-2.9.x
// Note: This conditional compile directive must follow the #include "scrollingwizard.h" statement
// above.
#if wxCHECK_VERSION(2,9,0)
// For wxWidgets 2.9.x and later do not compile this file into project, because
// the scrolling wizard functionality is built-in to the main wxWidgets library.
#else

// ----------------------------------------------------------------------------
// event tables and such
// ----------------------------------------------------------------------------

#if 0
DEFINE_EVENT_TYPE(wxEVT_WIZARD_PAGE_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_WIZARD_PAGE_CHANGING)
DEFINE_EVENT_TYPE(wxEVT_WIZARD_CANCEL)
DEFINE_EVENT_TYPE(wxEVT_WIZARD_FINISHED)
DEFINE_EVENT_TYPE(wxEVT_WIZARD_HELP)
#endif

BEGIN_EVENT_TABLE(wxScrollingWizard, wxDialog)
    EVT_BUTTON(wxID_CANCEL, wxScrollingWizard::OnCancel)
    EVT_BUTTON(wxID_BACKWARD, wxScrollingWizard::OnBackOrNext)
    EVT_BUTTON(wxID_FORWARD, wxScrollingWizard::OnBackOrNext)
    EVT_BUTTON(wxID_HELP, wxScrollingWizard::OnHelp)

    EVT_WIZARD_PAGE_CHANGED(wxID_ANY, wxScrollingWizard::OnWizEvent)
    EVT_WIZARD_PAGE_CHANGING(wxID_ANY, wxScrollingWizard::OnWizEvent)
    EVT_WIZARD_CANCEL(wxID_ANY, wxScrollingWizard::OnWizEvent)
    EVT_WIZARD_FINISHED(wxID_ANY, wxScrollingWizard::OnWizEvent)
    EVT_WIZARD_HELP(wxID_ANY, wxScrollingWizard::OnWizEvent)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(wxScrollingWizard, wxDialog)

/*
    TODO PROPERTIES :
    wxScrollingWizard
        extstyle
        title
*/

// ----------------------------------------------------------------------------
// wxScrollingWizardSizer
// ----------------------------------------------------------------------------

wxScrollingWizardSizer::wxScrollingWizardSizer(wxScrollingWizard *owner)
             : m_owner(owner),
               m_childSize(wxDefaultSize)
{
}

wxSizerItem *wxScrollingWizardSizer::Insert(size_t index, wxSizerItem *item)
{
    m_owner->m_usingSizer = true;

    if ( item->IsWindow() )
    {
        // we must pretend that the window is shown as otherwise it wouldn't be
        // taken into account for the layout -- but avoid really showing it, so
        // just set the internal flag instead of calling wxWindow::Show()
        item->GetWindow()->wxWindowBase::Show();
    }

    return wxSizer::Insert(index, item);
}

void wxScrollingWizardSizer::HidePages()
{
    for ( wxSizerItemList::compatibility_iterator node = GetChildren().GetFirst();
          node;
          node = node->GetNext() )
    {
        wxSizerItem * const item = node->GetData();
        if ( item->IsWindow() )
            item->GetWindow()->wxWindowBase::Show(false);
    }
}

void wxScrollingWizardSizer::RecalcSizes()
{
    // Effect of this function depends on m_owner->m_page and
    // it should be called whenever it changes (wxWizard::ShowPage)
    if ( m_owner->m_page )
    {
        m_owner->m_page->SetSize(wxRect(m_position, m_size));
    }
}

wxSize wxScrollingWizardSizer::CalcMin()
{
    return m_owner->GetPageSize();
}

wxSize wxScrollingWizardSizer::GetMaxChildSize()
{
#if !defined(_DEBUG)
    if ( m_childSize.IsFullySpecified() )
        return m_childSize;
#endif

    wxSize maxOfMin;

    for ( wxSizerItemList::compatibility_iterator childNode = m_children.GetFirst();
          childNode;
          childNode = childNode->GetNext() )
    {
        wxSizerItem *child = childNode->GetData();
        maxOfMin.IncTo(child->CalcMin());
        maxOfMin.IncTo(SiblingSize(child));
    }

    // No longer applicable since we may change sizes when size adaptation is done
#if 0
#ifdef _DEBUG
    if ( m_childSize.IsFullySpecified() && m_childSize != maxOfMin )
    {
        wxFAIL_MSG( _T("Size changed in wxWizard::GetPageAreaSizer()")
                    _T("after RunWizard().\n")
                    _T("Did you forget to call GetSizer()->Fit(this) ")
                    _T("for some page?")) ;

        return m_childSize;
    }
#endif // _DEBUG
#endif

    if ( m_owner->m_started )
    {
        m_childSize = maxOfMin;
    }

    return maxOfMin;
}

int wxScrollingWizardSizer::GetBorder() const
{
    return m_owner->m_border;
}

wxSize wxScrollingWizardSizer::SiblingSize(wxSizerItem *child)
{
    wxSize maxSibling;

    if ( child->IsWindow() )
    {
        wxWizardPage *page = wxDynamicCast(child->GetWindow(), wxWizardPage);
        if ( page )
        {
            for ( wxWizardPage *sibling = page->GetNext();
                  sibling;
                  sibling = sibling->GetNext() )
            {
                if ( sibling->GetSizer() )
                {
                    maxSibling.IncTo(sibling->GetSizer()->CalcMin());
                }
            }
        }
    }

    return maxSibling;
}

// ----------------------------------------------------------------------------
// generic wxWizard implementation
// ----------------------------------------------------------------------------

void wxScrollingWizard::Init()
{
    wxDialogHelper::SetDialog(this);

    m_posWizard = wxDefaultPosition;
    m_page = (wxWizardPage *)NULL;
    m_btnPrev = m_btnNext = NULL;
    m_statbmp = NULL;
    m_sizerBmpAndPage = NULL;
    m_sizerPage = NULL;
    m_border = 5;
    m_started = false;
    m_wasModal = false;
    m_usingSizer = false;
    m_bitmapBackgroundColour = *wxWHITE;
    m_bitmapPlacement = 0; // wxWIZARD_VALIGN_CENTRE|wxWIZARD_HALIGN_CENTRE;
    m_bitmapMinimumWidth = 115;
}

bool wxScrollingWizard::Create(wxWindow *parent,
                      int id,
                      const wxString& title,
                      const wxBitmap& bitmap,
                      const wxPoint& pos,
                      long style)
{
    bool result = wxDialog::Create(parent,id,title,pos,wxDefaultSize,style);

    m_posWizard = pos;
    m_bitmap = bitmap ;

    DoCreateControls();

    return result;
}

wxScrollingWizard::~wxScrollingWizard()
{
    // normally we don't have to delete this sizer as it's deleted by the
    // associated window but if we never used it or didn't set it as the window
    // sizer yet, do delete it manually
    if ( !m_usingSizer || !m_started )
    {
        delete m_sizerPage;
        m_sizerPage = NULL;
    }
}

void wxScrollingWizard::AddBitmapRow(wxBoxSizer *mainColumn)
{
    m_sizerBmpAndPage = new wxBoxSizer(wxHORIZONTAL);
    mainColumn->Add(
        m_sizerBmpAndPage,
        1, // Vertically stretchable
        wxEXPAND // Horizonal stretching, no border
    );
    mainColumn->Add(0,5,
        0, // No vertical stretching
        wxEXPAND // No border, (mostly useless) horizontal stretching
    );

#if wxUSE_STATBMP
    if ( m_bitmap.Ok() )
    {
        wxSize bitmapSize(wxDefaultSize);
        if (GetBitmapPlacement())
            bitmapSize.x = GetMinimumBitmapWidth();

        m_statbmp = new wxStaticBitmap(this, wxID_ANY, m_bitmap, wxDefaultPosition, bitmapSize);
        m_sizerBmpAndPage->Add(
            m_statbmp,
            0, // No horizontal stretching
            wxALL, // Border all around, top alignment
            5 // Border width
        );
        m_sizerBmpAndPage->Add(
            5,0,
            0, // No horizontal stretching
            wxEXPAND // No border, (mostly useless) vertical stretching
        );
    }
#endif

    // Added to m_sizerBmpAndPage later
    m_sizerPage = new wxScrollingWizardSizer(this);
}

void wxScrollingWizard::AddStaticLine(wxBoxSizer *mainColumn)
{
#if wxUSE_STATLINE
    mainColumn->Add(
        new wxStaticLine(this, wxID_ANY),
        0, // Vertically unstretchable
        wxEXPAND | wxALL, // Border all around, horizontally stretchable
        5 // Border width
    );
    mainColumn->Add(0,5,
        0, // No vertical stretching
        wxEXPAND // No border, (mostly useless) horizontal stretching
    );
#else
    (void)mainColumn;
#endif // wxUSE_STATLINE
}

void wxScrollingWizard::AddBackNextPair(wxBoxSizer *buttonRow)
{
    wxASSERT_MSG( m_btnNext && m_btnPrev,
                  _T("You must create the buttons before calling ")
                  _T("wxWizard::AddBackNextPair") );

    // margin between Back and Next buttons
#ifdef __WXMAC__
    static const int BACKNEXT_MARGIN = 10;
#else
    static const int BACKNEXT_MARGIN = 0;
#endif

    wxBoxSizer *backNextPair = new wxBoxSizer(wxHORIZONTAL);
    buttonRow->Add(
        backNextPair,
        0, // No horizontal stretching
        wxALL, // Border all around
        5 // Border width
    );

    backNextPair->Add(m_btnPrev);
    backNextPair->Add(BACKNEXT_MARGIN,0,
        0, // No horizontal stretching
        wxEXPAND // No border, (mostly useless) vertical stretching
    );
    backNextPair->Add(m_btnNext);
}

void wxScrollingWizard::AddButtonRow(wxBoxSizer *mainColumn)
{
    // the order in which the buttons are created determines the TAB order - at least under MSWindows...
    // although the 'back' button appears before the 'next' button, a more userfriendly tab order is
    // to activate the 'next' button first (create the next button before the back button).
    // The reason is: The user will repeatedly enter information in the wizard pages and then wants to
    // press 'next'. If a user uses mostly the keyboard, he would have to skip the 'back' button
    // everytime. This is annoying. There is a second reason: RETURN acts as TAB. If the 'next'
    // button comes first in the TAB order, the user can enter information very fast using the RETURN
    // key to TAB to the next entry field and page. This would not be possible, if the 'back' button
    // was created before the 'next' button.

    bool isPda = (wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA);
    int buttonStyle = isPda ? wxBU_EXACTFIT : 0;

    wxBoxSizer *buttonRow = new wxBoxSizer(wxHORIZONTAL);
#ifdef __WXMAC__
    if (GetExtraStyle() & wxWIZARD_EX_HELPBUTTON)
        mainColumn->Add(
            buttonRow,
            0, // Vertically unstretchable
            wxGROW|wxALIGN_CENTRE
            );
    else
#endif
    mainColumn->Add(
        buttonRow,
        0, // Vertically unstretchable
        wxALIGN_RIGHT // Right aligned, no border
    );

    // Desired TAB order is 'next', 'cancel', 'help', 'back'. This makes the 'back' button the last control on the page.
    // Create the buttons in the right order...
    wxButton *btnHelp=0;
#ifdef __WXMAC__
    if (GetExtraStyle() & wxWIZARD_EX_HELPBUTTON)
        btnHelp=new wxButton(this, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, buttonStyle);
#endif

    m_btnNext = new wxButton(this, wxID_FORWARD, _("&Next >"));
    wxButton *btnCancel=new wxButton(this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, buttonStyle);
#ifndef __WXMAC__
    if (GetExtraStyle() & wxWIZARD_EX_HELPBUTTON)
        btnHelp=new wxButton(this, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, buttonStyle);
#endif
    m_btnPrev = new wxButton(this, wxID_BACKWARD, _("< &Back"), wxDefaultPosition, wxDefaultSize, buttonStyle);

    if (btnHelp)
    {
        buttonRow->Add(
            btnHelp,
            0, // Horizontally unstretchable
            wxALL, // Border all around, top aligned
            5 // Border width
            );
#ifdef __WXMAC__
        // Put stretchable space between help button and others
        buttonRow->Add(0, 0, 1, wxALIGN_CENTRE, 0);
#endif
    }

    AddBackNextPair(buttonRow);

    buttonRow->Add(
        btnCancel,
        0, // Horizontally unstretchable
        wxALL, // Border all around, top aligned
        5 // Border width
    );
}

void wxScrollingWizard::DoCreateControls()
{
    // do nothing if the controls were already created
    if ( WasCreated() )
        return;

    bool isPda = (wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA);

    // Horizontal stretching, and if not PDA, border all around
    int mainColumnSizerFlags = isPda ? wxEXPAND : wxALL|wxEXPAND ;

    // wxWindow::SetSizer will be called at end
    wxBoxSizer *windowSizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *mainColumn = new wxBoxSizer(wxVERTICAL);
    windowSizer->Add(
        mainColumn,
        1, // Vertical stretching
        mainColumnSizerFlags,
        5 // Border width
    );

    AddBitmapRow(mainColumn);

    if (!isPda)
        AddStaticLine(mainColumn);

    AddButtonRow(mainColumn);

    SetSizer(windowSizer);
}

void wxScrollingWizard::SetPageSize(const wxSize& size)
{
    wxCHECK_RET(!m_started, wxT("wxScrollingWizard::SetPageSize after RunWizard"));
    m_sizePage = size;
}

void wxScrollingWizard::FitToPage(const wxWizardPage *page)
{
    wxCHECK_RET(!m_started, wxT("wxScrollingWizard::FitToPage after RunWizard"));

    while ( page )
    {
        wxSize size = page->GetBestSize();

        m_sizePage.IncTo(size);

        page = page->GetNext();
    }
}

bool wxScrollingWizard::ShowPage(wxWizardPage *page, bool goingForward)
{
    wxASSERT_MSG( page != m_page, wxT("this is useless") );

    wxSizerFlags flags(1);
    flags.Border(wxALL, m_border).Expand();

    if ( !m_started )
    {
        if ( m_usingSizer )
        {
            m_sizerBmpAndPage->Add(m_sizerPage, flags);

            // now that our layout is computed correctly, hide the pages
            // artificially shown in wxScrollingWizardSizer::Insert() back again
            m_sizerPage->HidePages();
        }
    }

    // we'll use this to decide whether we have to change the label of this
    // button or not (initially the label is "Next")
    bool btnLabelWasNext = true;

    // remember the old bitmap (if any) to compare with the new one later
    wxBitmap bmpPrev;

    // check for previous page
    if ( m_page )
    {
        // send the event to the old page
        wxWizardEvent event(wxEVT_WIZARD_PAGE_CHANGING, GetId(),
                            goingForward, m_page);
        if ( m_page->GetEventHandler()->ProcessEvent(event) &&
             !event.IsAllowed() )
        {
            // vetoed by the page
            return false;
        }

        m_page->Hide();

        btnLabelWasNext = HasNextPage(m_page);

        bmpPrev = m_page->GetBitmap();

        if ( !m_usingSizer )
            m_sizerBmpAndPage->Detach(m_page);
    }

    // set the new page
    m_page = page;

    // is this the end?
    if ( !m_page )
    {
        // terminate successfully
        if ( IsModal() )
        {
            EndModal(wxID_OK);
        }
        else
        {
            SetReturnCode(wxID_OK);
            Hide();
        }

        // and notify the user code (this is especially useful for modeless
        // wizards)
        wxWizardEvent event(wxEVT_WIZARD_FINISHED, GetId(), false, 0);
        (void)GetEventHandler()->ProcessEvent(event);

        return true;
    }

    // position and show the new page
    (void)m_page->TransferDataToWindow();

    if ( m_usingSizer )
    {
        // wxScrollingWizardSizer::RecalcSizes wants to be called when m_page changes
        m_sizerPage->RecalcSizes();
    }
    else // pages are not managed by the sizer
    {
        m_sizerBmpAndPage->Add(m_page, flags);
        m_sizerBmpAndPage->SetItemMinSize(m_page, GetPageSize());
    }

    wxBitmap bmp;

#if wxUSE_STATBMP
    // update the bitmap if it changed
    if ( m_statbmp )
    {
        bmp = m_page->GetBitmap();
        if ( !bmp.Ok() )
            bmp = m_bitmap;

        if ( !bmpPrev.Ok() )
            bmpPrev = m_bitmap;

        if (GetBitmapPlacement())
        {
            if ( !bmp.IsSameAs(bmpPrev) )
            {
                m_statbmp->SetBitmap(bmp);
            }
        }
    }
#endif // wxUSE_STATBMP


    // and update the buttons state
    m_btnPrev->Enable(HasPrevPage(m_page));

    bool hasNext = HasNextPage(m_page);
    if ( btnLabelWasNext != hasNext )
    {
        if ( hasNext )
            m_btnNext->SetLabel(_("&Next >"));
        else
            m_btnNext->SetLabel(_("&Finish"));
    }
    // nothing to do: the label was already correct

    m_btnNext->SetDefault();


    // send the change event to the new page now
    wxWizardEvent event(wxEVT_WIZARD_PAGE_CHANGED, GetId(), goingForward, m_page);
    (void)m_page->GetEventHandler()->ProcessEvent(event);

    // and finally show it
    m_page->Show();
    m_page->SetFocus();

    if ( !m_usingSizer )
        m_sizerBmpAndPage->Layout();

    if ( !m_started )
    {
        m_started = true;

        DoWizardLayout();
    }

    if (GetBitmapPlacement() && m_statbmp)
    {
        ResizeBitmap(bmp);
    
        if ( !bmp.IsSameAs(bmpPrev) )
            m_statbmp->SetBitmap(bmp);
    
        if (m_usingSizer)
            m_sizerPage->RecalcSizes();
    }

    return true;
}

// Do fit, and adjust to screen size if necessary
void wxScrollingWizard::DoWizardLayout()
{
    if ( wxSystemSettings::GetScreenType() > wxSYS_SCREEN_PDA )
    {
        if (CanDoLayoutAdaptation())
            DoLayoutAdaptation();
        else
            GetSizer()->SetSizeHints(this);

        if ( m_posWizard == wxDefaultPosition )
            CentreOnScreen();
    }

    SetLayoutAdaptationDone(true);
}

bool wxScrollingWizard::RunWizard(wxWizardPage *firstPage)
{
    wxCHECK_MSG( firstPage, false, wxT("can't run empty wizard") );

    // can't return false here because there is no old page
    (void)ShowPage(firstPage, true /* forward */);

    m_wasModal = true;

    return ShowModal() == wxID_OK;
}

wxWizardPage *wxScrollingWizard::GetCurrentPage() const
{
    return m_page;
}

wxSize wxScrollingWizard::GetPageSize() const
{
    // default width and height of the page
    int DEFAULT_PAGE_WIDTH,
        DEFAULT_PAGE_HEIGHT;
    if ( wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA )
    {
        // Make the default page size small enough to fit on screen
        DEFAULT_PAGE_WIDTH = wxSystemSettings::GetMetric(wxSYS_SCREEN_X) / 2;
        DEFAULT_PAGE_HEIGHT = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y) / 2;
    }
    else // !PDA
    {
        DEFAULT_PAGE_WIDTH =
        DEFAULT_PAGE_HEIGHT = 270;
    }

    // start with default minimal size
    wxSize pageSize(DEFAULT_PAGE_WIDTH, DEFAULT_PAGE_HEIGHT);

    // make the page at least as big as specified by user
    pageSize.IncTo(m_sizePage);

    if ( m_statbmp )
    {
        // make the page at least as tall as the bitmap
        pageSize.IncTo(wxSize(0, m_bitmap.GetHeight()));
    }

    if ( m_usingSizer )
    {
        // make it big enough to contain all pages added to the sizer
        pageSize.IncTo(m_sizerPage->GetMaxChildSize());
    }

    return pageSize;
}

wxSizer *wxScrollingWizard::GetPageAreaSizer() const
{
    return m_sizerPage;
}

void wxScrollingWizard::SetBorder(int border)
{
    wxCHECK_RET(!m_started, wxT("wxScrollingWizard::SetBorder after RunWizard"));

    m_border = border;
}

void wxScrollingWizard::OnCancel(wxCommandEvent& WXUNUSED(eventUnused))
{
    // this function probably can never be called when we don't have an active
    // page, but a small extra check won't hurt
    wxWindow *win = m_page ? (wxWindow *)m_page : (wxWindow *)this;

    wxWizardEvent event(wxEVT_WIZARD_CANCEL, GetId(), false, m_page);
    if ( !win->GetEventHandler()->ProcessEvent(event) || event.IsAllowed() )
    {
        // no objections - close the dialog
        if(IsModal())
        {
            EndModal(wxID_CANCEL);
        }
        else
        {
            SetReturnCode(wxID_CANCEL);
            Hide();
        }
    }
    //else: request to Cancel ignored
}

void wxScrollingWizard::OnBackOrNext(wxCommandEvent& event)
{
    wxASSERT_MSG( (event.GetEventObject() == m_btnNext) ||
                  (event.GetEventObject() == m_btnPrev),
                  wxT("unknown button") );

    wxCHECK_RET( m_page, _T("should have a valid current page") );

    // ask the current page first: notice that we do it before calling
    // GetNext/Prev() because the data transfered from the controls of the page
    // may change the value returned by these methods
    if ( !m_page->Validate() || !m_page->TransferDataFromWindow() )
    {
        // the page data is incorrect, don't do anything
        return;
    }

    bool forward = event.GetEventObject() == m_btnNext;

    wxWizardPage *page;
    if ( forward )
    {
        page = m_page->GetNext();
    }
    else // back
    {
        page = m_page->GetPrev();

        wxASSERT_MSG( page, wxT("\"<Back\" button should have been disabled") );
    }

    // just pass to the new page (or maybe not - but we don't care here)
    (void)ShowPage(page, forward);
}

void wxScrollingWizard::OnHelp(wxCommandEvent& WXUNUSED(event))
{
    // this function probably can never be called when we don't have an active
    // page, but a small extra check won't hurt
    if(m_page != NULL)
    {
        // Create and send the help event to the specific page handler
        // event data contains the active page so that context-sensitive
        // help is possible
        wxWizardEvent eventHelp(wxEVT_WIZARD_HELP, GetId(), true, m_page);
        (void)m_page->GetEventHandler()->ProcessEvent(eventHelp);
    }
}

void wxScrollingWizard::OnWizEvent(wxWizardEvent& event)
{
    // the dialogs have wxWS_EX_BLOCK_EVENTS style on by default but we want to
    // propagate wxEVT_WIZARD_XXX to the parent (if any), so do it manually
    if ( !(GetExtraStyle() & wxWS_EX_BLOCK_EVENTS) )
    {
        // the event will be propagated anyhow
        event.Skip();
    }
    else
    {
        wxWindow *parent = GetParent();

        if ( !parent || !parent->GetEventHandler()->ProcessEvent(event) )
        {
            event.Skip();
        }
    }

    if ( ( !m_wasModal ) &&
         event.IsAllowed() &&
         ( event.GetEventType() == wxEVT_WIZARD_FINISHED ||
           event.GetEventType() == wxEVT_WIZARD_CANCEL
         )
       )
    {
        Destroy();
    }
}

void wxScrollingWizard::SetBitmap(const wxBitmap& bitmap)
{
    m_bitmap = bitmap;
    if (m_statbmp)
        m_statbmp->SetBitmap(m_bitmap);
}

// Do the adaptation
bool wxScrollingWizard::DoLayoutAdaptation()
{
    wxWindowList windows;
    wxWindowList pages;

    // Make all the pages (that use sizers) scrollable
    for ( wxSizerItemList::compatibility_iterator node = m_sizerPage->GetChildren().GetFirst(); node; node = node->GetNext() )
    {
        wxSizerItem * const item = node->GetData();
        if ( item->IsWindow() )
        {
            wxWizardPage* page = wxDynamicCast(item->GetWindow(), wxWizardPage);
            if (page)
            {
                while (page)
                {
                    if (!pages.Find(page) && page->GetSizer())
                    {
                        // Create a scrolled window and reparent
                        wxScrolledWindow* scrolledWindow = new wxScrolledWindow(page, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL|wxVSCROLL|wxHSCROLL|wxBORDER_NONE);
                        wxSizer* oldSizer = page->GetSizer();
                        
                        wxSizer* newSizer = new wxBoxSizer(wxVERTICAL);
                        newSizer->Add(scrolledWindow,1, wxEXPAND, 0);
                        
                        page->SetSizer(newSizer, false /* don't delete the old sizer */);
                        
                        scrolledWindow->SetSizer(oldSizer);
                        
                        wxStandardDialogLayoutAdapter::DoReparentControls(page, scrolledWindow);
                        
                        pages.Append(page);
                        windows.Append(scrolledWindow);
                    }
                    page = page->GetNext();
                }
            }
        }
    }
    
    wxStandardDialogLayoutAdapter::DoFitWithScrolling(this, windows);

    // Size event doesn't get sent soon enough on wxGTK
    DoLayout();
    
    SetLayoutAdaptationDone(true);

    return true;
}

bool wxScrollingWizard::ResizeBitmap(wxBitmap& bmp)
{
    if (!GetBitmapPlacement())
        return false;

    if (bmp.Ok())
    {
        // int originalBmpWidth = bmp.GetWidth();
        // int originalBmpHeight = bmp.GetHeight();

        // Why is this (0, 0)? GetPageSizer returns a default size so is wrong.
        wxSize pageSize = m_sizerPage->GetSize();
        if (pageSize == wxSize(0,0))
            pageSize = GetPageSize();
        int bitmapWidth = wxMax(bmp.GetWidth(), GetMinimumBitmapWidth());
        int bitmapHeight = pageSize.y;

        if (!m_statbmp->GetBitmap().Ok() || m_statbmp->GetBitmap().GetHeight() != bitmapHeight)
        {
            wxBitmap bitmap(bitmapWidth, bitmapHeight);
            {
                wxMemoryDC dc;
                dc.SelectObject(bitmap);
                dc.SetBackground(wxBrush(m_bitmapBackgroundColour));
                dc.Clear();

                if (GetBitmapPlacement() & wxWIZARD_TILE)
                {
                    TileBitmap(wxRect(0, 0, bitmapWidth, bitmapHeight), dc, bmp);
                }
                else
                {
                    int x, y;

                    if (GetBitmapPlacement() & wxWIZARD_HALIGN_LEFT)
                        x = 0;
                    else if (GetBitmapPlacement() & wxWIZARD_HALIGN_RIGHT)
                        x = bitmapWidth - bmp.GetWidth();
                    else
                        x = (bitmapWidth - bmp.GetWidth())/2;

                    if (GetBitmapPlacement() & wxWIZARD_VALIGN_TOP)
                        y = 0;
                    else if (GetBitmapPlacement() & wxWIZARD_VALIGN_BOTTOM)
                        y = bitmapHeight - bmp.GetHeight();
                    else
                        y = (bitmapHeight - bmp.GetHeight())/2;

                    dc.DrawBitmap(bmp, x, y, true);
                    dc.SelectObject(wxNullBitmap);
                }
            }

            bmp = bitmap;
        }
    }

    return true;
}

bool wxScrollingWizard::TileBitmap(const wxRect& rect, wxDC& dc, const wxBitmap& bitmap)
{
    int w = bitmap.GetWidth();
    int h = bitmap.GetHeight();

    wxMemoryDC dcMem;

#if wxUSE_PALETTE
    static bool hiColour = (wxDisplayDepth() >= 16) ;
    if (bitmap.GetPalette() && !hiColour)
    {
        dc.SetPalette(* bitmap.GetPalette());
        dcMem.SetPalette(* bitmap.GetPalette());
    }
#endif // wxUSE_PALETTE

    dcMem.SelectObjectAsSource(bitmap);

    int i, j;
    for (i = rect.x; i < rect.x + rect.width; i += w)
    {
        for (j = rect.y; j < rect.y + rect.height; j+= h)
            dc.Blit(i, j, bitmap.GetWidth(), bitmap.GetHeight(), & dcMem, 0, 0);
    }
    dcMem.SelectObject(wxNullBitmap);

#if wxUSE_PALETTE
    if (bitmap.GetPalette() && !hiColour)
    {
        dc.SetPalette(wxNullPalette);
        dcMem.SetPalette(wxNullPalette);
    }
#endif // wxUSE_PALETTE

    return true;
}

#endif // wxUSE_WIZARDDLG

#endif // #if wxCHECK_VERSION(2,9,0)
