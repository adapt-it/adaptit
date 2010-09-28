//------------------------------------------------------------------------------
// Header generated by wxDesigner from file: Adapt_It.wdr
// Do not modify this file, all changes will be lost!
//------------------------------------------------------------------------------

#ifndef __WDR_Adapt_It_H__
#define __WDR_Adapt_It_H__

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma interface "Adapt_It_wdr.h"
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

const int ID_STATICBITMAP = -1;
const int ID_TEXT = 0;
const int ID_ABOUT_VERSION_LABEL = 1;
const int ID_ABOUT_VERSION_NUM = 2;
const int ID_ABOUT_VERSION_DATE = 3;
const int ID_STATIC_UNICODE_OR_ANSI = 4;
const int ID_STATIC_WX_VERSION_USED = 5;
const int ID_LINE = 6;
const int ID_STATIC_UI_LANGUAGE = 7;
const int ID_STATIC_HOST_OS = 8;
const int ID_STATIC_SYS_LANGUAGE = 9;
const int ID_STATIC_SYS_LOCALE_NAME = 10;
const int ID_STATIC_CANONICAL_LOCALE_NAME = 11;
const int ID_STATIC_SYS_ENCODING_NAME = 12;
const int ID_STATIC_SYSTEM_LAYOUT_DIR = 13;
wxSizer *AboutDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_RADIO_DRAFTING = 14;
const int IDC_RADIO_REVIEWING = 15;
const int IDC_CHECK_SINGLE_STEP = 16;
const int IDC_CHECK_KB_SAVE = 17;
const int IDC_CHECK_FORCE_ASK = 18;
const int IDC_BUTTON_NO_ADAPT = 19;
const int IDC_STATIC = 20;
const int IDC_EDIT_DELAY = 21;
const int IDC_CHECK_ISGLOSSING = 22;
wxSizer *ControlBarFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_EDIT_COMPOSE = 23;
const int IDC_BUTTON_SHORTEN = 24;
const int IDC_BUTTON_PREV = 25;
const int IDC_BUTTON_LENGTHEN = 26;
const int IDC_BUTTON_NEXT = 27;
const int IDC_BUTTON_REMOVE = 28;
const int IDC_BUTTON_APPLY = 29;
const int IDC_STATIC_SECTION_DEF = 30;
const int IDC_RADIO_PUNCT_SECTION = 31;
const int IDC_RADIO_VERSE_SECTION = 32;
const int IDC_BUTTON_CLEAR = 33;
const int IDC_BUTTON_SELECT_ALL = 34;
wxSizer *ComposeBarFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_LISTBOX_ADAPTIONS = 35;
wxSizer *OpenExistingProjectDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_EDIT_FILENAME = 36;
const int ID_TEXT_INVALID_CHARACTERS = 37;
wxSizer *GetOutputFilenameDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_SOURCE_LANGUAGE = 38;
const int ID_EDIT_SOURCE_LANG_CODE = 39;
const int IDC_TARGET_LANGUAGE = 40;
const int ID_EDIT_TARGET_LANG_CODE = 41;
const int ID_BUTTON_LOOKUP_CODES = 42;
wxSizer *LanguagesPageFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_SOURCE_FONTNAME = 43;
const int IDC_CHECK_SRC_RTL = 44;
const int IDC_SOURCE_SIZE = 45;
const int IDC_SOURCE_LANG = 46;
const int IDC_BUTTON_SOURCE_COLOR = 47;
const int ID_BUTTON_CHANGE_SRC_ENCODING = 48;
const int IDC_TARGET_FONTNAME = 49;
const int IDC_CHECK_TGT_RTL = 50;
const int IDC_TARGET_SIZE = 51;
const int IDC_TARGET_LANG = 52;
const int IDC_BUTTON_TARGET_COLOR = 53;
const int ID_BUTTON_CHANGE_TGT_ENCODING = 54;
const int IDC_NAVTEXT_FONTNAME = 55;
const int IDC_CHECK_NAVTEXT_RTL = 56;
const int IDC_NAVTEXT_SIZE = 57;
const int IDC_CHANGE_NAV_TEXT = 58;
const int IDC_BUTTON_NAV_TEXT_COLOR = 59;
const int ID_BUTTON_CHANGE_NAV_ENCODING = 60;
const int ID_TEXTCTRL_AS_STATIC_FONTPAGE = 61;
const int IDC_BUTTON_SPECTEXTCOLOR = 62;
const int IDC_RETRANSLATION_BUTTON = 63;
wxSizer *FontsPageFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_PLEASE_WAIT = 64;
wxSizer *WaitDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_LIST_ACCEPTED = 65;
const int IDC_BUTTON_REJECT = 66;
const int IDC_BUTTON_ACCEPT = 67;
const int ID_BUTTON_REJECT_ALL_FILES = 68;
const int ID_BUTTON_ACCEPT_ALL_FILES = 69;
const int IDC_LIST_REJECTED = 70;
wxSizer *WhichFilesDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXTCTRL_TRANSFORM_TO_GLOSSES1 = 71;
const int ID_TEXTCTRL_TRANSFORM_TO_GLOSSES2 = 72;
const int ID_TEXTCTRL_TRANSFORM_TO_GLOSSES3 = 73;
wxSizer *TransformToGlossesDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_CHECK_SOURCE_USES_CAPS = 74;
const int ID_CHECK_USE_AUTO_CAPS = 75;
const int ID_TEXTCTRL_AS_CASE_PAGE_STATIC_TEXT = 76;
const int ID_TEXT_SL = 77;
const int ID_TEXT_TL = 78;
const int ID_TEXT_GL = 79;
const int IDC_EDIT_SRC_CASE_EQUIVALENCES = 80;
const int IDC_EDIT_TGT_CASE_EQUIVALENCES = 81;
const int IDC_EDIT_GLOSS_CASE_EQUIVALENCES = 82;
const int IDC_BUTTON_CLEAR_SRC_LIST = 83;
const int IDC_BUTTON_CLEAR_TGT_LIST = 84;
const int IDC_BUTTON_CLEAR_GLOSS_LIST = 85;
const int IDC_BUTTON_SRC_SET_ENGLISH = 86;
const int IDC_BUTTON_TGT_SET_ENGLISH = 87;
const int IDC_BUTTON_GLOSS_SET_ENGLISH = 88;
const int IDC_BUTTON_SRC_COPY_TO_NEXT = 89;
const int IDC_BUTTON_TGT_COPY_TO_NEXT = 90;
const int IDC_BUTTON_GLOSS_COPY_TO_NEXT = 91;
const int IDC_BUTTON_SRC_COPY_TO_GLOSS = 92;
wxSizer *CaseEquivDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_EDIT_WELCOME_TO = 93;
const int IDC_EDIT_ADAPT_IT = 94;
const int ID_TEXTCTRL_AS_STATIC_WELCOME = 95;
const int IDC_CHECK_NOLONGER_SHOW = 96;
wxSizer *WelcomeDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXTCTRL_AS_STATIC_DOCPAGE = 97;
const int IDC_STATIC_WHICH_MODE = 98;
const int IDC_BUTTON_WHAT_IS_DOC = 99;
const int IDC_STATIC_WHICH_FOLDER = 100;
const int IDC_LIST_NEWDOC_AND_EXISTINGDOC = 101;
const int IDC_CHECK_CHANGE_FIXED_SPACES_TO_REGULAR_SPACES = 102;
const int IDC_CHANGE_FOLDER_BTN = 103;
const int IDC_CHECK_FORCE_UTF8 = 104;
wxSizer *DocPageFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXTCTRL_AS_STATIC_PROJECTPAGE = 105;
const int IDC_BUTTON_WHAT_IS_PROJECT = 106;
const int IDC_LIST_NEW_AND_EXISTING = 107;
wxSizer *ProjectPageFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_EDIT_SRC = 108;
const int IDC_LIST_PUNCTS = 109;
const int ID_TEXTCTRL_AS_STATIC_PLACE_INT_PUNCT = 110;
const int IDC_EDIT_TGT = 111;
const int IDC_BUTTON_PLACE = 112;
wxSizer *PlaceInternalPunctDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_RADIO_INCHES = 113;
const int IDC_RADIO_CM = 114;
wxSizer *UnitsDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_EDIT_MATCHED_SOURCE = 115;
const int IDC_MYLISTBOX_TRANSLATIONS = 116;
const int IDC_EDIT_REFERENCES = 117;
const int IDC_BUTTON_MOVE_UP = 118;
const int IDC_BUTTON_MOVE_DOWN = 119;
const int IDC_BUTTON_CANCEL_ASK = 120;
const int IDC_BUTTON_CANCEL_AND_SELECT = 121;
const int IDC_EDIT_NEW_TRANSLATION = 122;
wxSizer *ChooseTranslationDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_STATIC_SRC = 123;
const int IDC_EDIT_PRECONTEXT = 124;
const int IDC_EDIT_SOURCE_TEXT = 125;
const int IDC_EDIT_RETRANSLATION = 126;
const int IDC_COPY_RETRANSLATION_TO_CLIPBOARD = 127;
const int IDC_BUTTON_TOGGLE = 128;
const int IDC_STATIC_TGT = 129;
const int IDC_EDIT_FOLLCONTEXT = 130;
wxSizer *RetranslationDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_EDIT_SRC_TEXT = 131;
const int IDC_EDIT_TGT_TEXT = 132;
const int IDC_SPIN_CHAPTER = 133;
const int IDC_SPIN_VERSE = 134;
const int IDC_GET_CHVERSE_TEXT = 135;
const int IDC_CLOSE_AND_JUMP = 136;
const int IDC_SHOW_MORE = 137;
const int IDC_SHOW_LESS = 138;
const int IDC_STATIC_BEGIN = 139;
const int IDC_STATIC_END = 140;
wxSizer *EarlierTransDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_CHECK_KB_BACKUP = 141;
const int IDC_CHECK_BAKUP_DOC = 142;
const int ID_TEXTCTRL_AS_STATIC_BACKUPS_AND_KB_PAGE = 143;
const int IDC_EDIT_SRC_NAME = 144;
const int IDC_EDIT_TGT_NAME = 145;
const int IDC_RADIO_ADAPT_BEFORE_GLOSS = 146;
const int IDC_RADIO_GLOSS_BEFORE_ADAPT = 147;
const int IDC_CHECK_LEGACY_SRC_TEXT_COPY = 148;
wxSizer *BackupsAndKBPageFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int STATIC_TEXT_V4 = 149;
const int IDC_EDIT_LEADING = 150;
const int STATIC_TEXT_V5 = 151;
const int IDC_EDIT_GAP_WIDTH = 152;
const int STATIC_TEXT_V6 = 153;
const int IDC_EDIT_LEFTMARGIN = 154;
const int STATIC_TEXT_V7 = 155;
const int IDC_EDIT_MULTIPLIER = 156;
const int IDC_EDIT_DIALOGFONTSIZE = 157;
const int IDC_CHECK_WELCOME_VISIBLE = 158;
const int IDC_CHECK_HIGHLIGHT_AUTO_INSERTED_TRANSLATIONS = 159;
const int IDC_BUTTON_CHOOSE_HIGHLIGHT_COLOR = 160;
const int IDC_CHECK_SHOW_ADMIN_MENU = 161;
wxSizer *ViewPageFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

wxSizer *UnitsPageFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXTCTRL_AS_STATIC_AUTOSAVE = 162;
const int IDC_CHECK_NO_AUTOSAVE = 163;
const int IDC_RADIO_BY_MINUTES = 164;
const int IDC_EDIT_MINUTES = 165;
const int IDC_SPIN_MINUTES = 166;
const int IDC_RADIO_BY_MOVES = 167;
const int IDC_EDIT_MOVES = 168;
const int IDC_SPIN_MOVES = 169;
const int IDC_EDIT_KB_MINUTES = 170;
const int IDC_SPIN_KB_MINUTES = 171;
wxSizer *AutoSavingPageFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_RADIO_DIV1 = 172;
const int IDC_RADIO_DIV2 = 173;
const int IDC_RADIO_DIV3 = 174;
const int IDC_RADIO_DIV4 = 175;
const int IDC_RADIO_DIV5 = 176;
const int IDC_COMBO_CHOOSE_BOOK = 177;
const int ID_TEXT_AS_STATIC = 178;
wxSizer *WhichBookDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_STATIC_MSG = 179;
const int IDC_EDIT_ERR_XML = 180;
const int IDC_STATIC_LBL = 181;
const int IDC_EDIT_OFFSET = 182;
wxSizer *XMLErrorDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXTCTRL_AS_STATIC_USFMPAGE = 183;
const int IDC_RADIO_USE_UBS_SET_ONLY = 184;
const int IDC_RADIO_USE_SILPNG_SET_ONLY = 185;
const int IDC_RADIO_USE_BOTH_SETS = 186;
const int IDC_RADIO_USE_UBS_SET_ONLY_PROJ = 187;
const int IDC_RADIO_USE_SILPNG_SET_ONLY_PROJ = 188;
const int IDC_RADIO_USE_BOTH_SETS_PROJ = 189;
const int IDC_RADIO_USE_UBS_SET_ONLY_FACTORY = 190;
const int IDC_RADIO_USE_SILPNG_SET_ONLY_FACTORY = 191;
const int IDC_RADIO_USE_BOTH_SETS_FACTORY = 192;
const int IDC_CHECK_CHANGE_FIXED_SPACES_TO_REGULAR_SPACES_USFM = 193;
wxSizer *USFMPageFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXTCTRL_AS_STATIC_FILTERPAGE = 194;
const int IDC_LIST_SFMS = 195;
const int IDC_LIST_SFMS_PROJ = 196;
const int IDC_LIST_SFMS_FACTORY = 197;
wxSizer *FilterPageFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_ED_SILCONVERTER_NAME = 198;
const int IDC_BTN_SELECT_SILCONVERTER = 199;
const int IDC_ED_SILCONVERTER_INFO = 200;
const int IDC_BTN_CLEAR_SILCONVERTER = 201;
wxSizer *SilConvertersDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_STATIC_FOLDER_DOCS = 202;
const int IDC_LIST_FOLDER_DOCS = 203;
const int IDC_BUTTON_NEXT_CHAPTER = 204;
const int IDC_BUTTON_SPLIT_NOW = 205;
const int IDC_STATIC_SPLITTING_WAIT = 206;
const int IDC_RADIO_PHRASEBOX_LOCATION = 207;
const int IDC_RADIO_CHAPTER_SFMARKER = 208;
const int IDC_RADIO_DIVIDE_INTO_CHAPTERS = 209;
const int IDC_STATIC_SPLIT_NAME = 210;
const int IDC_EDIT1 = 211;
const int IDC_STATIC_REMAIN_NAME = 212;
const int IDC_EDIT2 = 213;
wxSizer *SplitDialogFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_LIST_MARKERS = 214;
const int IDC_TEXTCTRL_AS_STATIC_PLACE_INT_MKRS = 215;
wxSizer *PlaceInternalMarkersDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_EDIT_NOTE = 216;
const int IDC_EDIT_FIND_TEXT = 217;
const int IDC_FIND_NEXT_BTN = 218;
const int IDC_LAST_BTN = 219;
const int IDC_NEXT_BTN = 220;
const int IDC_PREV_BTN = 221;
const int IDC_FIRST_BTN = 222;
const int IDC_DELETE_BTN = 223;
wxSizer *NoteDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXTCTRL_AS_STATIC_COLLECT_BT = 224;
const int IDC_RADIO_COLLECT_ADAPTATIONS = 225;
const int IDC_RADIO_COLLECT_GLOSSES = 226;
wxSizer *CollectBackTranslationsDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_SPIN_DELAY_TICKS = 227;
wxSizer *SetDelayDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_RADIO_EXPORT_ALL = 228;
const int IDC_RADIO_EXPORT_SELECTED_MARKERS = 229;
const int ID_TEXTCTRL_AS_STATIC_EXPORT_OPTIONS = 230;
const int IDC_BUTTON_EXCLUDE_FROM_EXPORT = 231;
const int IDC_BUTTON_INCLUDE_IN_EXPORT = 232;
const int IDC_BUTTON_UNDO = 233;
const int IDC_STATIC_SPECIAL_NOTE = 234;
const int IDC_CHECK_PLACE_FREE_TRANS = 235;
const int IDC_CHECK_INTERLINEAR_PLACE_FREE_TRANS = 236;
const int IDC_CHECK_PLACE_BACK_TRANS = 237;
const int IDC_CHECK_INTERLINEAR_PLACE_BACK_TRANS = 238;
const int IDC_CHECK_PLACE_AI_NOTES = 239;
const int IDC_CHECK_INTERLINEAR_PLACE_AI_NOTES = 240;
wxSizer *ExportOptionsDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_STATIC_TITLE = 241;
const int IDC_RADIO_EXPORT_AS_SFM = 242;
const int ID_TEXTCTRL_AS_STATIC_EXPORT_SAVE_AS_1 = 243;
const int IDC_RADIO_EXPORT_AS_RTF = 244;
const int ID_TEXTCTRL_AS_STATIC_EXPORT_SAVE_AS_2 = 245;
const int ID_TEXTCTRL_AS_STATIC_EXPORT_SAVE_AS_3 = 246;
const int IDC_BUTTON_EXPORT_FILTER_OPTIONS = 247;
wxSizer *ExportSaveAsDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_CHECK_INCLUDE_NAV_TEXT = 248;
const int IDC_CHECK_INCLUDE_SOURCE_TEXT = 249;
const int IDC_CHECK_INCLUDE_TARGET_TEXT = 250;
const int IDC_CHECK_INCLUDE_GLS_TEXT = 251;
const int ID_TEXTCTRL_AS_STATIC_EXP_INTERLINEAR = 252;
const int IDC_RADIO_PORTRAIT = 253;
const int IDC_RADIO_LANDSCAPE = 254;
const int IDC_RADIO_OUTPUT_ALL = 255;
const int IDC_RADIO_OUTPUT_CHAPTER_VERSE_RANGE = 256;
const int IDC_EDIT_FROM_CHAPTER = 257;
const int IDC_EDIT_FROM_VERSE = 258;
const int IDC_EDIT_TO_CHAPTER = 259;
const int IDC_EDIT_TO_VERSE = 260;
const int IDC_RADIO_OUTPUT_PRELIM = 261;
const int IDC_RADIO_OUTPUT_FINAL = 262;
const int ID_CHECK_NEW_TABLES_FOR_NEWLINE_MARKERS = 263;
const int ID_CHECK_CENTER_TABLES_FOR_CENTERED_MARKERS = 264;
const int IDC_BUTTON_RTF_EXPORT_FILTER_OPTIONS = 265;
wxSizer *ExportInterlinearDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_LIST_MARKER = 266;
const int IDC_EDIT_MARKER_TEXT = 267;
const int IDC_REMOVE_BTN = 268;
const int IDC_BUTTON_SWITCH_ENCODING = 269;
const int IDC_LIST_MARKER_END = 270;
const int IDC_STATIC_MARKER_DESCRIPTION = 271;
const int IDC_STATIC_MARKER_STATUS = 272;
wxSizer *ViewFilteredMaterialDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_EDIT_CHAPTER = 273;
const int IDC_EDIT_VERSE = 274;
wxSizer *GoToDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_EDIT_KEY = 275;
const int IDC_EDIT_CH_VERSE = 276;
const int IDC_EDIT_ADAPTATION = 277;
const int IDC_LIST_TRANSLATIONS = 278;
const int IDC_RADIO_LIST_SELECT = 279;
const int IDC_RADIO_ACCEPT_CURRENT = 280;
const int IDC_RADIO_TYPE_NEW = 281;
const int IDC_EDIT_TYPE_NEW = 282;
const int IDC_NOTHING = 283;
const int IDC_BUTTON_IGNORE_IT = 284;
const int IDC_CHECK_DO_SAME = 285;
wxSizer *ConsistencyCheckDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_RADIO_CHECK_OPEN_DOC_ONLY = 286;
const int IDC_RADIO_CHECK_SELECTED_DOCS = 287;
const int ID_TEXTCTRL_AS_STATIC_CHOOSE_CONSISTENCY_CHECK_TYPE = 288;
const int ID_TEXTCTRL_MSG_TWO = 289;
wxSizer *ChooseConsistencyCheckTypeDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXTCTRL_EDIT_SOURCE_AS_STATIC0 = 290;
const int ID_TEXTCTRL = 291;
const int IDC_EDIT_OLD_SOURCE_TEXT = 292;
const int ID_TEXTCTRL_EDIT_SOURCE_AS_STATIC1 = 293;
const int ID_TEXTCTRL_EDIT_SOURCE_AS_STATIC2 = 294;
const int IDC_EDIT_NEW_SOURCE = 295;
const int ID_TEXTCTRL_EDIT_SOURCE_AS_STATIC4 = 296;
wxSizer *EditSourceTextDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_EDIT_UNPACK_WARN = 297;
wxSizer *UnpackWarningDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_STATIC_COUNT = 298;
const int IDC_LIST_SRC_KEYS = 299;
const int IDC_LIST_EXISTING_TRANSLATIONS = 300;
const int IDC_EDIT_EDITORADD = 301;
const int IDC_BUTTON_UPDATE = 302;
const int IDC_BUTTON_ADD = 303;
const int IDC_ADD_NOTHING = 304;
const int IDC_EDIT_SHOW_FLAG = 305;
const int IDC_BUTTON_FLAG_TOGGLE = 306;
const int IDC_EDIT_SRC_KEY = 307;
const int IDC_EDIT_REF_COUNT = 308;
const int ID_TEXTCTRL_SEARCH = 309;
const int ID_BUTTON_GO = 310;
const int ID_BUTTON_ERASE_ALL_LINES = 311;
const int ID_COMBO_OLD_SEARCHES = 312;
wxSizer *KBEditorPanelFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_STATIC_TEXT_SELECT_A_TAB = 313;
const int ID_KB_EDITOR_NOTEBOOK = 314;
wxSizer *KBEditorDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_STATIC_DOCS_IN_FOLDER = 315;
const int IDC_LIST_SOURCE_FOLDER_DOCS = 316;
const int IDC_RADIO_TO_BOOK_FOLDER = 317;
const int IDC_RADIO_FROM_BOOK_FOLDER = 318;
const int IDC_VIEW_OTHER = 319;
const int IDC_BUTTON_RENAME_DOC = 320;
const int IDC_MOVE_NOW = 321;
wxSizer *MoveDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXTCTRL_AS_STATIC_JOIN1 = 322;
const int IDC_BUTTON_MOVE_ALL_RIGHT = 323;
const int IDC_BUTTON_MOVE_ALL_LEFT = 324;
const int ID_TEXTCTRL_AS_STATIC_JOIN2 = 325;
const int ID_TEXTCTRL_AS_STATIC_JOIN3 = 326;
const int ID_JOIN_NOW = 327;
const int IDC_STATIC_JOINING_WAIT = 328;
const int ID_TEXTCTRL_AS_STATIC_JOIN4 = 329;
const int IDC_EDIT_NEW_FILENAME = 330;
wxSizer *JoinDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

wxSizer *ListDocInOtherFolderDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_STATIC_SET_FONT_TITLE = 331;
const int ID_STATIC_CURR_ENCODING_IS = 332;
const int ID_TEXT_CURR_ENCODING = 333;
const int ID_LISTBOX_POSSIBLE_FACENAMES = 334;
const int ID_LISTBOX_POSSIBLE_ENCODINGS = 335;
extern wxSizer *pTestBoxStaticTextBoxSizer;
const int ID_TEXT_TEST_ENCODING_BOX = 336;
const int ID_BUTTON_CHART_FONT_SIZE_DECREASE = 337;
const int ID_BUTTON_CHART_FONT_SIZE_INCREASE = 338;
const int ID_STATIC_CHART_FONT_SIZE = 339;
const int ID_SCROLLED_ENCODING_WINDOW = 340;
wxSizer *SetEncodingDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_EDIT_CCT = 341;
wxSizer *CCTableEditDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_LIST_CCTABLES = 342;
const int IDC_EDIT_SELECTED_TABLE = 343;
const int IDC_BUTTON_BROWSE = 344;
const int IDC_BUTTON_CREATE_CCT = 345;
const int IDC_BUTTON_EDIT_CCT = 346;
const int IDC_BUTTON_SELECT_NONE = 347;
const int IDC_EDIT_FOLDER_PATH = 348;
wxSizer *CCTablePageFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_RADIO_SRC_ONLY_FIND = 349;
const int IDC_RADIO_TGT_ONLY_FIND = 350;
const int IDC_RADIO_SRC_AND_TGT_FIND = 351;
const int IDC_CHECK_IGNORE_CASE_FIND = 352;
const int IDC_STATIC_SRC_FIND = 353;
const int IDC_EDIT_SRC_FIND = 354;
const int IDC_STATIC_TGT_FIND = 355;
const int IDC_EDIT_TGT_FIND = 356;
const int IDC_CHECK_INCLUDE_PUNCT_FIND = 357;
const int IDC_CHECK_SPAN_SRC_PHRASES_FIND = 358;
const int IDC_STATIC_SPECIAL = 359;
extern wxSizer *IDC_STATIC_SIZER_SPECIAL;
const int IDC_RADIO_RETRANSLATION = 360;
const int IDC_RADIO_NULL_SRC_PHRASE = 361;
const int IDC_RADIO_SFM = 362;
const int IDC_STATIC_SELECT_MKR = 363;
const int IDC_COMBO_SFM = 364;
const int IDC_BUTTON_SPECIAL = 365;
wxSizer *FindDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_RADIO_SRC_ONLY_REPLACE = 366;
const int IDC_RADIO_TGT_ONLY_REPLACE = 367;
const int IDC_RADIO_SRC_AND_TGT_REPLACE = 368;
const int IDC_CHECK_IGNORE_CASE_REPLACE = 369;
const int IDC_STATIC_SRC_REPLACE = 370;
const int IDC_EDIT_SRC_REPLACE = 371;
const int IDC_STATIC_TGT_REPLACE = 372;
const int IDC_EDIT_TGT_REPLACE = 373;
const int IDC_CHECK_INCLUDE_PUNCT_REPLACE = 374;
const int IDC_CHECK_SPAN_SRC_PHRASES_REPLACE = 375;
const int IDC_STATIC_REPLACE = 376;
const int IDC_EDIT_REPLACE = 377;
const int IDC_REPLACE = 378;
const int IDC_REPLACE_ALL_BUTTON = 379;
wxSizer *ReplaceDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXTCTRL_AS_STATIC = 380;
const int IDC_EDIT_TBLNAME = 381;
wxSizer *CCTableNameDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXTCTRL_EDIT_AS_STATIC = 382;
const int ID_RADIO_ALL = 383;
const int ID_RADIO_SELECTION = 384;
const int IDC_RADIO_PAGES = 385;
const int IDC_EDIT_PAGES_FROM = 386;
const int IDC_EDIT_PAGES_TO = 387;
const int IDC_RADIO_CHAPTER_VERSE_RANGE = 388;
const int IDC_EDIT3 = 389;
const int IDC_EDIT4 = 390;
const int IDC_CHECK_SUPPRESS_PREC_HEADING = 391;
const int IDC_CHECK_INCLUDE_FOLL_HEADING = 392;
const int IDC_CHECK_SUPPRESS_FOOTER = 393;
wxSizer *PrintOptionsDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_CC_TABBED_NOTEBOOK = 394;
wxSizer *CCTabbedNotebookFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_LIST_UI_LANGUAGES = 395;
const int ID_TEXT_AS_STATIC_SHORT_LANG_NAME = 396;
const int ID_TEXT_AS_STATIC_LONG_LANG_NAME = 397;
const int IDC_LOCALIZATION_PATH = 398;
const int IDC_BTN_BROWSE_PATH = 399;
wxSizer *ChooseLanguageDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_COMBO_REMOVALS = 400;
const int ID_STATIC_TEXT_REMOVALS = 401;
const int IDC_BUTTON_UNDO_LAST_COPY = 402;
wxSizer *RemovalsBarFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_EDIT_MSG_TEXT = 403;
const int IDC_BUTTON_PREV_STEP = 404;
const int IDC_BUTTON_NEXT_STEP = 405;
const int ID_BUTTON_END_NOW = 406;
const int ID_BUTTON_CANCEL_ALL_STEPS = 407;
wxSizer *VertEditBarFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXT_TITLE = 408;
const int ID_TEXTCTRL_AS_STATIC_PUNCT_CORRESP_PAGE = 409;
const int IDC_TOGGLE_UNNNN_BTN = 410;
const int ID_NOTEBOOK = 411;
wxSizer *PunctCorrespPageFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_EDIT_SRC0 = 412;
const int IDC_EDIT_TGT0 = 413;
const int IDC_EDIT_SRC9 = 414;
const int IDC_EDIT_TGT9 = 415;
const int IDC_EDIT_SRC18 = 416;
const int IDC_EDIT_TGT18 = 417;
const int IDC_EDIT_SRC1 = 418;
const int IDC_EDIT_TGT1 = 419;
const int IDC_EDIT_SRC10 = 420;
const int IDC_EDIT_TGT10 = 421;
const int IDC_EDIT_SRC19 = 422;
const int IDC_EDIT_TGT19 = 423;
const int IDC_EDIT_SRC2 = 424;
const int IDC_EDIT_TGT2 = 425;
const int IDC_EDIT_SRC11 = 426;
const int IDC_EDIT_TGT11 = 427;
const int IDC_EDIT_SRC20 = 428;
const int IDC_EDIT_TGT20 = 429;
const int IDC_EDIT_SRC3 = 430;
const int IDC_EDIT_TGT3 = 431;
const int IDC_EDIT_SRC12 = 432;
const int IDC_EDIT_TGT12 = 433;
const int IDC_EDIT_SRC21 = 434;
const int IDC_EDIT_TGT21 = 435;
const int IDC_EDIT_SRC4 = 436;
const int IDC_EDIT_TGT4 = 437;
const int IDC_EDIT_SRC13 = 438;
const int IDC_EDIT_TGT13 = 439;
const int IDC_EDIT_SRC22 = 440;
const int IDC_EDIT_TGT22 = 441;
const int IDC_EDIT_SRC5 = 442;
const int IDC_EDIT_TGT5 = 443;
const int IDC_EDIT_SRC14 = 444;
const int IDC_EDIT_TGT14 = 445;
const int IDC_EDIT_SRC23 = 446;
const int IDC_EDIT_TGT23 = 447;
const int IDC_EDIT_SRC6 = 448;
const int IDC_EDIT_TGT6 = 449;
const int IDC_EDIT_SRC15 = 450;
const int IDC_EDIT_TGT15 = 451;
const int IDC_EDIT_SRC24 = 452;
const int IDC_EDIT_TGT24 = 453;
const int IDC_EDIT_SRC7 = 454;
const int IDC_EDIT_TGT7 = 455;
const int IDC_EDIT_SRC16 = 456;
const int IDC_EDIT_TGT16 = 457;
const int IDC_EDIT_SRC25 = 458;
const int IDC_EDIT_TGT25 = 459;
const int IDC_EDIT_SRC8 = 460;
const int IDC_EDIT_TGT8 = 461;
const int IDC_EDIT_SRC17 = 462;
const int IDC_EDIT_TGT17 = 463;
wxSizer *SinglePunctTabPageFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_EDIT_2SRC0 = 464;
const int IDC_EDIT_2TGT0 = 465;
const int IDC_EDIT_2SRC5 = 466;
const int IDC_EDIT_2TGT5 = 467;
const int IDC_EDIT_2SRC1 = 468;
const int IDC_EDIT_2TGT1 = 469;
const int IDC_EDIT_2SRC6 = 470;
const int IDC_EDIT_2TGT6 = 471;
const int IDC_EDIT_2SRC2 = 472;
const int IDC_EDIT_2TGT2 = 473;
const int IDC_EDIT_2SRC7 = 474;
const int IDC_EDIT_2TGT7 = 475;
const int IDC_EDIT_2SRC3 = 476;
const int IDC_EDIT_2TGT3 = 477;
const int IDC_EDIT_2SRC8 = 478;
const int IDC_EDIT_2TGT8 = 479;
const int IDC_EDIT_2SRC4 = 480;
const int IDC_EDIT_2TGT4 = 481;
const int IDC_EDIT_2SRC9 = 482;
const int IDC_EDIT_2TGT9 = 483;
wxSizer *DoublePunctTabPageFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

wxSizer *ControlBar2LineFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXTCTRL_MSG1 = 484;
const int ID_BUTTON_LOCATE_SOURCE_FOLDER = 485;
const int ID_BITMAPBUTTON_SRC_OPEN_FOLDER_UP = 486;
const int ID_TEXT_SOURCE_FOLDER_PATH = 487;
const int ID_TEXTCTRL_SOURCE_PATH = 488;
const int ID_LISTCTRL_SOURCE_CONTENTS = 489;
const int ID_BUTTON_LOCATE_DESTINATION_FOLDER = 490;
const int ID_BITMAPBUTTON_DEST_OPEN_FOLDER_UP = 491;
const int ID_TEXTCTRL_DESTINATION_PATH = 492;
const int ID_LISTCTRL_DESTINATION_CONTENTS = 493;
const int ID_BUTTON_MOVE = 494;
const int ID_BUTTON_COPY = 495;
const int ID_BUTTON_PEEK = 496;
const int ID_BUTTON_RENAME = 497;
const int ID_BUTTON_DELETE = 498;
wxSizer *MoveOrCopyFilesOrFoldersFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXT_MSG1 = 499;
const int ID_TEXTCTRL_SOURCE_FILE_DETAILS = 500;
const int ID_TEXTCTRL_DESTINATION_FILE_DETAILS = 501;
const int ID_RADIOBUTTON_REPLACE = 502;
const int ID_RADIOBUTTON_NO_COPY = 503;
const int ID_RADIOBUTTON_COPY_AND_RENAME = 504;
const int ID_TEXT_MODIFY_NAME = 505;
const int ID_CHECKBOX_HANDLE_SAME = 506;
wxSizer *FilenameConflictFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_LISTBOX_UPDATED = 507;
const int ID_TEXTCTRL_INFO_SOURCE = 508;
const int ID_TEXTCTRL_INFO_REFS = 509;
const int ID_LISTBOX_MATCHED = 510;
const int ID_BUTTON_UPDATE = 511;
const int ID_TEXTCTRL_LOCAL_SEARCH = 512;
const int ID_BUTTON_FIND_NEXT = 513;
const int ID_TEXTCTRL_EDITBOX = 514;
const int ID_BUTTON_RESTORE = 515;
const int ID_BUTTON_REMOVE_UPDATE = 516;
wxSizer *KBEditSearchFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_STATICTEXT_SCROLL_LIST = 517;
const int ID_LIST_LANGUAGE_CODES_NAMES = 518;
const int ID_STATICTEXT_SEARCH_FOR_LANG_NAME = 519;
const int ID_TEXTCTRL_SEARCH_LANG_NAME = 520;
const int ID_BUTTON_USE_SEL_AS_SRC = 521;
const int ID_BUTTON_USE_SEL_AS_TGT = 522;
const int ID_SRC_LANGUAGE_CODE = 523;
const int ID_TEXTCTRL_SRC_LANG_CODE = 524;
const int ID_TGT_LANGUAGE_CODE = 525;
const int ID_TEXTCTRL_TGT_LANG_CODE = 526;
wxSizer *LanguageCodesDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXTCTRL_PEEKMSG = 527;
extern wxSizer *m_pHorizBox_for_textctrl;
const int ID_TEXTCTRL_LINES100 = 528;
const int ID_BUTTON_TOGGLE_TEXT_DIRECTION = 529;
wxSizer *PeekAtFileFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_TEXTCTRL_INSTRUCTIONS = 530;
const int ID_LISTBOX_LOADABLES_FILENAMES = 531;
wxSizer *NewDocFromSourceDataFolderFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_STATIC_SELECT_A_TAB = 532;
const int ID_MENU_EDITOR_NOTEBOOK = 533;
const int ID_RADIOBUTTON_NONE = 534;
const int ID_RADIOBUTTON_USE_PROFILE = 535;
const int ID_COMBO_PROFILE_ITEMS = 536;
const int ID_BUTTON_RESET_TO_FACTORY = 537;
const int ID_TEXT_STATIC_DESCRIPTION = 538;
const int ID_TEXTCTRL_PROFILE_DESCRIPTION = 539;
wxSizer *MenuEditorDlgFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_CHECKLISTBOX_MENU_ITEMS = 540;
wxSizer *MenuEditorPanelFunc( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

// Declare menubar functions

const int ID_FILE_MENU = 541;
const int ID_SAVE_AS = 542;
const int ID_FILE_PACK_DOC = 543;
const int ID_FILE_UNPACK_DOC = 544;
const int ID_MENU = 545;
const int ID_FILE_STARTUP_WIZARD = 546;
const int ID_FILE_CLOSEKB = 547;
const int ID_FILE_CHANGEFOLDER = 548;
const int ID_FILE_SAVEKB = 549;
const int ID_FILE_BACKUP_KB = 550;
const int ID_FILE_RESTORE_KB = 551;
const int ID_EDIT_MENU = 552;
const int ID_EDIT_CUT = 553;
const int ID_EDIT_COPY = 554;
const int ID_EDIT_PASTE = 555;
const int ID_GO_TO = 556;
const int ID_EDIT_SOURCE_TEXT = 557;
const int ID_EDIT_CONSISTENCY_CHECK = 558;
const int ID_EDIT_MOVE_NOTE_FORWARD = 559;
const int ID_EDIT_MOVE_NOTE_BACKWARD = 560;
const int ID_VIEW_MENU = 561;
const int ID_VIEW_TOOLBAR = 562;
const int ID_VIEW_STATUS_BAR = 563;
const int ID_VIEW_COMPOSE_BAR = 564;
const int ID_COPY_SOURCE = 565;
const int ID_MARKER_WRAPS_STRIP = 566;
const int ID_UNITS = 567;
const int ID_CHANGE_INTERFACE_LANGUAGE = 568;
const int ID_TOOLS_MENU = 569;
const int ID_TOOLS_DEFINE_CC = 570;
const int ID_UNLOAD_CC_TABLES = 571;
const int ID_USE_CC = 572;
const int ID_ACCEPT_CHANGES = 573;
const int ID_TOOLS_DEFINE_SILCONVERTER = 574;
const int ID_USE_SILCONVERTER = 575;
const int ID_TOOLS_KB_EDITOR = 576;
const int ID_TOOLS_AUTO_CAPITALIZATION = 577;
const int ID_RETRANS_REPORT = 578;
const int ID_TOOLS_SPLIT_DOC = 579;
const int ID_TOOLS_JOIN_DOCS = 580;
const int ID_TOOLS_MOVE_DOC = 581;
const int ID_EXPORT_IMPORT_MENU = 582;
const int ID_FILE_EXPORT_SOURCE = 583;
const int ID_FILE_EXPORT = 584;
const int ID_FILE_EXPORT_TO_RTF = 585;
const int ID_EXPORT_GLOSSES = 586;
const int ID_EXPORT_FREE_TRANS = 587;
const int ID_EXPORT_OXES = 588;
const int ID_FILE_EXPORT_KB = 589;
const int ID_IMPORT_TO_KB = 590;
const int ID_ADVANCED_MENU = 591;
const int ID_ADVANCED_ENABLEGLOSSING = 592;
const int ID_ADVANCED_GLOSSING_USES_NAV_FONT = 593;
const int ID_ADVANCED_TRANSFORM_ADAPTATIONS_INTO_GLOSSES = 594;
const int ID_ADVANCED_DELAY = 595;
const int ID_ADVANCED_BOOKMODE = 596;
const int ID_ADVANCED_FREE_TRANSLATION_MODE = 597;
const int ID_ADVANCED_TARGET_TEXT_IS_DEFAULT = 598;
const int ID_ADVANCED_GLOSS_TEXT_IS_DEFAULT = 599;
const int ID_ADVANCED_REMOVE_FILTERED_FREE_TRANSLATIONS = 600;
const int ID_ADVANCED_COLLECT_BACKTRANSLATIONS = 601;
const int ID_ADVANCED_REMOVE_FILTERED_BACKTRANSLATIONS = 602;
const int ID_ADVANCED_USETRANSLITERATIONMODE = 603;
const int ID_ADVANCED_SENDSYNCHRONIZEDSCROLLINGMESSAGES = 604;
const int ID_ADVANCED_RECEIVESYNCHRONIZEDSCROLLINGMESSAGES = 605;
const int ID_LAYOUT_MENU = 606;
const int ID_ALIGNMENT = 607;
const int ID_HELP_MENU = 608;
const int ID_ONLINE_HELP = 609;
const int ID_USER_FORUM = 610;
const int ID_HELP_USE_TOOLTIPS = 611;
const int ID_ADMINISTRATOR_MENU = 612;
const int ID_CUSTOM_WORK_FOLDER_LOCATION = 613;
const int ID_LOCK_CUSTOM_LOCATION = 614;
const int ID_UNLOCK_CUSTOM_LOCATION = 615;
const int ID_LOCAL_WORK_FOLDER_MENU = 616;
const int ID_SET_PASSWORD_MENU = 617;
const int ID_MOVE_OR_COPY_FOLDERS_OR_FILES = 618;
const int ID_SOURCE_DATA_FOLDER = 619;
const int ID_EXPORT_DATA_FOLDER = 620;
const int ID_EDIT_USER_MENU_SETTINGS_PROFILE = 621;
wxMenuBar *AIMenuBarFunc();

// Declare toolbar functions

const int ID_BUTTON_CREATE_NOTE = 622;
const int ID_BUTTON_PREV_NOTE = 623;
const int ID_BUTTON_NEXT_NOTE = 624;
const int ID_BUTTON_DELETE_ALL_NOTES = 625;
const int ID_BUTTON_RESPECTING_BDRY = 626;
const int ID_BUTTON_SHOWING_PUNCT = 627;
const int ID_BUTTON_TO_END = 628;
const int ID_BUTTON_TO_START = 629;
const int ID_BUTTON_STEP_DOWN = 630;
const int ID_BUTTON_STEP_UP = 631;
const int ID_BUTTON_BACK = 632;
const int ID_BUTTON_MERGE = 633;
const int ID_BUTTON_RETRANSLATION = 634;
const int ID_BUTTON_EDIT_RETRANSLATION = 635;
const int ID_REMOVE_RETRANSLATION = 636;
const int ID_BUTTON_NULL_SRC = 637;
const int ID_BUTTON_REMOVE_NULL_SRCPHRASE = 638;
const int ID_BUTTON_CHOOSE_TRANSLATION = 639;
const int ID_SHOWING_ALL = 640;
const int ID_BUTTON_EARLIER_TRANSLATION = 641;
const int ID_BUTTON_NO_PUNCT_COPY = 642;
void AIToolBarFunc( wxToolBar *parent );

void AIToolBar32x30Func( wxToolBar *parent );

// Declare bitmap functions

wxBitmap AIToolBarBitmapsUnToggledFunc( size_t index );

const int ID_BITMAP_FOLDERAI = 643;
const int ID_BITMAP_FILEAI = 644;
const int ID_BITMAP_EMPTY_FOLDER = 645;
wxBitmap AIMainFrameIcons( size_t index );

const int ID_BUTTON_IGNORING_BDRY = 646;
const int ID_BUTTON_HIDING_PUNCT = 647;
const int ID_SHOWING_TGT = 648;
const int ID_BUTTON_ENABLE_PUNCT_COPY = 649;
wxBitmap AIToolBarBitmapsToggledFunc( size_t index );

wxBitmap WhichFilesBitmapsFunc( size_t index );

wxBitmap AIToolBarBitmapsToggled32x30Func( size_t index );

wxBitmap AIToolBarBitmapsUnToggled32x30Func( size_t index );

#endif

// End of generated file
