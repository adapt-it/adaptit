//------------------------------------------------------------------------------
// Header generated by wxDesigner from file: CC_WX.wdr
// Do not modify this file, all changes will be lost!
//------------------------------------------------------------------------------

#ifndef __WDR_CC_WX_H__
#define __WDR_CC_WX_H__

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma interface "CC_WX_wdr.h"
#endif

// Include wxWidgets' headers

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/image.h>
#include <wx/statline.h>
#include <wx/spinbutt.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/notebook.h>
#include <wx/grid.h>
#include <wx/toolbar.h>
#include <wx/tglbtn.h>

// Declare window functions

const int ID_COMBO_HISTORY = 10000;
const int ID_TEXT = 10001;
const int ID_COMBO_WORK_DIR = 10002;
const int ID_BUTTON_WORK_DIR_BROWSE = 10003;
const int ID_COMBO_INPUT_FILE = 10004;
const int ID_CHECKBOX_CONTAINS_LIST = 10005;
const int ID_BUTTON_INPUT_BROWSE = 10006;
const int ID_BUTTON_INTPUT_EDIT = 10007;
const int ID_COMBO_CHANGES_FILE = 10008;
const int ID_CHECKBOX_CONTAINS_UTF8 = 10009;
const int ID_BUTTON_CHANGES_BROWSE = 10010;
const int ID_BUTTON_CHANGES_EDIT = 10011;
const int ID_COMBO_OUTPUT_FILE = 10012;
const int ID_CHECKBOX_OVERWRITE_EXISTING = 10013;
const int ID_CHECKBOX_OUTPUT_HAS_FILES = 10014;
const int ID_CHECKBOX_APPEND = 10015;
const int ID_BUTTON_OUTPUT_BROWSE = 10016;
const int ID_BUTTON_OUTPUT_VIEW = 10017;
const int ID_BUTTON_PROCESS = 10018;
const int ID_BUTTON_OPTIONS = 10019;
wxSizer *CCDialogAppFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

// Declare menubar functions

const int ID_MENU = 10020;
wxMenuBar *MyMenuBarFunc();

// Declare toolbar functions

void MyToolBarFunc( wxToolBar *parent );

// Declare bitmap functions

#endif

// End of generated file
