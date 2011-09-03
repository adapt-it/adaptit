//------------------------------------------------------------------------------
// Source code generated by wxDesigner from file: CC_WX.wdr
// Do not modify this file, all changes will be lost!
//------------------------------------------------------------------------------

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma implementation "CC_WX_wdr.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// Include private header
#include "CC_WX_wdr.h"


// Euro sign hack of the year
#if wxUSE_UNICODE
    #define __WDR_EURO__ wxT("\u20ac")
#else
    #if defined(__WXMAC__)
        #define __WDR_EURO__ wxT("\xdb")
    #elif defined(__WXMSW__)
        #define __WDR_EURO__ wxT("\x80")
    #else
        #define __WDR_EURO__ wxT("\xa4")
    #endif
#endif

// Implement window functions

wxSizer *CCDialogAppFunc( wxWindow *parent, bool call_fit, bool set_sizer )
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer *item1 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticBox *item3 = new wxStaticBox( parent, -1, wxT("Command History") );
    wxStaticBoxSizer *item2 = new wxStaticBoxSizer( item3, wxVERTICAL );

    wxString strs4[] = 
    {
        wxT("ComboItem")
    };
    wxComboBox *item4 = new wxComboBox( parent, ID_COMBO_HISTORY, wxT(""), wxDefaultPosition, wxSize(100,-1), 1, strs4, wxCB_DROPDOWN );
    item2->Add( item4, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    item1->Add( item2, 1, wxGROW|wxALL, 0 );

    item0->Add( item1, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxBoxSizer *item5 = new wxBoxSizer( wxVERTICAL );

    wxStaticBox *item7 = new wxStaticBox( parent, -1, wxT("") );
    wxStaticBoxSizer *item6 = new wxStaticBoxSizer( item7, wxHORIZONTAL );

    wxStaticText *item8 = new wxStaticText( parent, ID_TEXT, wxT("Working Directory:"), wxDefaultPosition, wxDefaultSize, 0 );
    item6->Add( item8, 0, wxALIGN_CENTER|wxALL, 5 );

    wxBoxSizer *item9 = new wxBoxSizer( wxVERTICAL );

    wxString strs10[] = 
    {
        wxT("ComboItem")
    };
    wxComboBox *item10 = new wxComboBox( parent, ID_COMBO_WORK_DIR, wxT(""), wxDefaultPosition, wxSize(100,-1), 1, strs10, wxCB_DROPDOWN );
    item9->Add( item10, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    item6->Add( item9, 1, wxALIGN_CENTER|wxALL, 0 );

    wxButton *item11 = new wxButton( parent, ID_BUTTON_WORK_DIR_BROWSE, wxT("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
    item6->Add( item11, 0, wxALIGN_CENTER|wxALL, 5 );

    item5->Add( item6, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0 );

    item0->Add( item5, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxBoxSizer *item12 = new wxBoxSizer( wxVERTICAL );

    wxStaticBox *item14 = new wxStaticBox( parent, -1, wxT("") );
    wxStaticBoxSizer *item13 = new wxStaticBoxSizer( item14, wxHORIZONTAL );

    wxStaticText *item15 = new wxStaticText( parent, ID_TEXT, wxT("Input File:"), wxDefaultPosition, wxDefaultSize, 0 );
    item13->Add( item15, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 10 );

    wxBoxSizer *item16 = new wxBoxSizer( wxVERTICAL );

    wxString strs17[] = 
    {
        wxT("ComboItem")
    };
    wxComboBox *item17 = new wxComboBox( parent, ID_COMBO_INPUT_FILE, wxT(""), wxDefaultPosition, wxSize(100,-1), 1, strs17, wxCB_DROPDOWN );
    item16->Add( item17, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0 );

    wxBoxSizer *item18 = new wxBoxSizer( wxHORIZONTAL );

    item18->Add( 20, 20, 0, wxALIGN_CENTER|wxALL, 0 );

    wxCheckBox *item19 = new wxCheckBox( parent, ID_CHECKBOX_CONTAINS_LIST, wxT("&Input contains list of input files"), wxDefaultPosition, wxDefaultSize, 0 );
    item18->Add( item19, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0 );

    item16->Add( item18, 0, wxALIGN_CENTER|wxALL, 5 );

    item13->Add( item16, 3, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

    wxBoxSizer *item20 = new wxBoxSizer( wxVERTICAL );

    wxButton *item21 = new wxButton( parent, ID_BUTTON_INPUT_BROWSE, wxT("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
    item20->Add( item21, 0, wxALIGN_CENTER|wxALL, 0 );

    item20->Add( 20, 4, 0, wxALIGN_CENTER, 0 );

    wxButton *item22 = new wxButton( parent, ID_BUTTON_INTPUT_EDIT, wxT("Edit..."), wxDefaultPosition, wxDefaultSize, 0 );
    item20->Add( item22, 0, wxALIGN_CENTER|wxALL, 0 );

    item13->Add( item20, 0, wxALIGN_CENTER|wxALL, 5 );

    item12->Add( item13, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0 );

    item0->Add( item12, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxBoxSizer *item23 = new wxBoxSizer( wxVERTICAL );

    wxStaticBox *item25 = new wxStaticBox( parent, -1, wxT("") );
    wxStaticBoxSizer *item24 = new wxStaticBoxSizer( item25, wxHORIZONTAL );

    wxStaticText *item26 = new wxStaticText( parent, ID_TEXT, wxT("Changes file:"), wxDefaultPosition, wxDefaultSize, 0 );
    item24->Add( item26, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 10 );

    wxBoxSizer *item27 = new wxBoxSizer( wxVERTICAL );

    wxString strs28[] = 
    {
        wxT("ComboItem")
    };
    wxComboBox *item28 = new wxComboBox( parent, ID_COMBO_CHANGES_FILE, wxT(""), wxDefaultPosition, wxSize(100,-1), 1, strs28, wxCB_DROPDOWN );
    item27->Add( item28, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0 );

    wxBoxSizer *item29 = new wxBoxSizer( wxHORIZONTAL );

    item29->Add( 20, 20, 0, wxALIGN_CENTER|wxALL, 0 );

    wxCheckBox *item30 = new wxCheckBox( parent, ID_CHECKBOX_CONTAINS_UTF8, wxT("Input and Output are &UTF-8 encoded files"), wxDefaultPosition, wxDefaultSize, 0 );
    item29->Add( item30, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0 );

    item27->Add( item29, 0, wxALIGN_CENTER|wxALL, 5 );

    item24->Add( item27, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

    wxBoxSizer *item31 = new wxBoxSizer( wxVERTICAL );

    wxButton *item32 = new wxButton( parent, ID_BUTTON_CHANGES_BROWSE, wxT("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
    item31->Add( item32, 0, wxALIGN_CENTER|wxALL, 0 );

    item31->Add( 20, 4, 0, wxALIGN_CENTER, 0 );

    wxButton *item33 = new wxButton( parent, ID_BUTTON_CHANGES_EDIT, wxT("Edit..."), wxDefaultPosition, wxDefaultSize, 0 );
    item31->Add( item33, 0, wxALIGN_CENTER|wxALL, 0 );

    item24->Add( item31, 0, wxALIGN_CENTER|wxALL, 5 );

    item23->Add( item24, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0 );

    item0->Add( item23, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxBoxSizer *item34 = new wxBoxSizer( wxVERTICAL );

    wxStaticBox *item36 = new wxStaticBox( parent, -1, wxT("") );
    wxStaticBoxSizer *item35 = new wxStaticBoxSizer( item36, wxHORIZONTAL );

    wxStaticText *item37 = new wxStaticText( parent, ID_TEXT, wxT("Output file:"), wxDefaultPosition, wxDefaultSize, 0 );
    item35->Add( item37, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 10 );

    wxBoxSizer *item38 = new wxBoxSizer( wxVERTICAL );

    wxString strs39[] = 
    {
        wxT("ComboItem")
    };
    wxComboBox *item39 = new wxComboBox( parent, ID_COMBO_OUTPUT_FILE, wxT(""), wxDefaultPosition, wxSize(100,-1), 1, strs39, wxCB_DROPDOWN );
    item38->Add( item39, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0 );

    wxBoxSizer *item40 = new wxBoxSizer( wxVERTICAL );

    wxCheckBox *item41 = new wxCheckBox( parent, ID_CHECKBOX_OVERWRITE_EXISTING, wxT("&Overwrite existing output"), wxDefaultPosition, wxDefaultSize, 0 );
    item40->Add( item41, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0 );

    wxCheckBox *item42 = new wxCheckBox( parent, ID_CHECKBOX_OUTPUT_HAS_FILES, wxT("Output contains &list of output files"), wxDefaultPosition, wxDefaultSize, 0 );
    item40->Add( item42, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0 );

    wxCheckBox *item43 = new wxCheckBox( parent, ID_CHECKBOX_APPEND, wxT("&Append to existing output"), wxDefaultPosition, wxDefaultSize, 0 );
    item40->Add( item43, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0 );

    item38->Add( item40, 0, wxALIGN_CENTER|wxALL, 5 );

    item35->Add( item38, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

    wxBoxSizer *item44 = new wxBoxSizer( wxVERTICAL );

    wxButton *item45 = new wxButton( parent, ID_BUTTON_OUTPUT_BROWSE, wxT("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
    item44->Add( item45, 0, wxALIGN_CENTER|wxALL, 0 );

    item44->Add( 20, 4, 0, wxALIGN_CENTER, 0 );

    wxButton *item46 = new wxButton( parent, ID_BUTTON_OUTPUT_VIEW, wxT("View..."), wxDefaultPosition, wxDefaultSize, 0 );
    item44->Add( item46, 0, wxALIGN_CENTER|wxALL, 0 );

    item35->Add( item44, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

    item34->Add( item35, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0 );

    item0->Add( item34, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxGridSizer *item47 = new wxGridSizer( 5, 0, 0 );

    wxButton *item48 = new wxButton( parent, ID_BUTTON_PROCESS, wxT("Process"), wxDefaultPosition, wxDefaultSize, 0 );
    item47->Add( item48, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item49 = new wxButton( parent, wxID_EXIT, wxT("Exit"), wxDefaultPosition, wxDefaultSize, 0 );
    item47->Add( item49, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item50 = new wxButton( parent, ID_BUTTON_OPTIONS, wxT("Options..."), wxDefaultPosition, wxDefaultSize, 0 );
    item47->Add( item50, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item51 = new wxButton( parent, wxID_HELP, wxT("Help..."), wxDefaultPosition, wxDefaultSize, 0 );
    item47->Add( item51, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item52 = new wxButton( parent, wxID_ABOUT, wxT("About..."), wxDefaultPosition, wxDefaultSize, 0 );
    item47->Add( item52, 0, wxALIGN_CENTER|wxALL, 5 );

    item0->Add( item47, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    if (set_sizer)
    {
        parent->SetSizer( item0 );
        if (call_fit)
            item0->SetSizeHints( parent );
    }
    
    return item0;
}

// Implement menubar functions

wxMenuBar *MyMenuBarFunc()
{
    wxMenuBar *item0 = new wxMenuBar;
    
    wxMenu* item1 = new wxMenu;
    item1->Append( wxID_ABOUT, wxT("About"), wxT("") );
    item1->Append( wxID_EXIT, wxT("Quit"), wxT("") );
    item0->Append( item1, wxT("File") );
    
    return item0;
}

// Implement toolbar functions

void MyToolBarFunc( wxToolBar *parent )
{
    parent->SetMargins( 2, 2 );
    
    
    parent->Realize();
}

// Implement bitmap functions


// End of generated file
