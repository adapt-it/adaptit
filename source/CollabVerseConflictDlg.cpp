/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CollabVerseConflictDlg.cpp
/// \author			Bill Martin
/// \date_created	10 July 2015
/// \rcs_id $Id$
/// \copyright		2015 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CCollabVerseConflictDlg class. 
/// The CCollabVerseConflictDlg class provides the user with a dialog that is
/// used to choose the best version to send to the external editor when
/// conflicts have been detected at save time during collaboration.
/// \derivation		The CCollabVerseConflictDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "CollabVerseConflictDlg.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#include <wx/event.h>
#endif

// other includes
//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
//#include <wx/valgen.h> // for wxGenericValidator
//#include <wx/valtext.h> // for wxTextValidator
#include "Adapt_It.h"
#include "CollabUtilities.h"
#include "CollabVerseConflictDlg.h"
#include "MyTextCtrl.h"

// event handler table
BEGIN_EVENT_TABLE(CCollabVerseConflictDlg, AIModalDialog)
	EVT_INIT_DIALOG(CCollabVerseConflictDlg::InitDialog)
	// Samples:
	EVT_CHECKLISTBOX(ID_CHECKLISTBOX_VERSE_REFS, CCollabVerseConflictDlg::OnCheckListBoxTickChange)
	EVT_LISTBOX(ID_CHECKLISTBOX_VERSE_REFS, CCollabVerseConflictDlg::OnListBoxSelChange)

	EVT_RADIOBUTTON(ID_RADIOBUTTON_USE_AI_VERSION, CCollabVerseConflictDlg::OnRadioUseAIVersion)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_RETAIN_PT_VERSION, CCollabVerseConflictDlg::OnRadioRetainPTVersion)

	EVT_BUTTON(ID_BUTTON_SELECT_ALL_VS, CCollabVerseConflictDlg::OnSelectAllVersesButton)
	EVT_BUTTON(ID_BUTTON_UNSELECT_ALL_VS, CCollabVerseConflictDlg::OnUnSelectAllVersesButton)

	// ... other menu, button or control events
END_EVENT_TABLE()

CCollabVerseConflictDlg::CCollabVerseConflictDlg(wxWindow* parent, wxArrayPtrVoid* pConfArr) // dialog constructor
	: AIModalDialog(parent, -1, _("Choose The Best Verses To Transfer To Paratext"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pConflictDlgTopSizer = AI_PT_ConflictingVersesFunc(this, TRUE, TRUE);
	// The declaration is: AI_PT_ConflictingVersesFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_pApp = (CAdapt_ItApp*)&wxGetApp();

	pConflictsArray = pConfArr;
	m_bMakePTboxEditable = FALSE;

	// EvtHandler::Connect() didn't work for me, to connect the focus event to the
	// dialog's event handling table; so instead try subclassing the relevant wxTextCtrl
	// and trap the wxEVT_KILL_FOCUS there. wxTextCtrl events do not propagate up to the
	// parent because they are not wxCommandEvent type
	//this->Connect(ID_TEXTCTRL_EDITABLE_PT_VERSION, wxEVT_KILL_FOCUS, wxFocusEventHandler(CCollabVerseConflictDlg::OnKillFocus));

	// See the comment by Bill at lines 140++ in ChoseTranslation.cpp, for how to
	// substitute a subclass for a control in a wxDesigner generated dialog. We
	// have to remove the control, and substitute our new one in its place. It's tricky.
	// We need to do it below, for pTextCtrlPTTargetVersion, substituting for the
	// wxTextCtrl from wxDesigner the MyTextCtrl subclass which allows me to intercept
	// EVT_KILL_FOCUS events. (I may need to support EVT_SET_FOCUS events too, not sure
	// yet, but so far I suspect not.)

	// Setup dialog box control pointers below:
	pCheckListBoxVerseRefs = (wxCheckListBox*)FindWindowById(ID_CHECKLISTBOX_VERSE_REFS);
	wxASSERT(pCheckListBoxVerseRefs != NULL);

	pTextCtrlSourceText = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_READONLY_SOURCE_TEXT);
	wxASSERT(pTextCtrlSourceText != NULL);
	pTextCtrlSourceText->SetBackgroundColour(m_pApp->sysColorBtnFace);

	pTextCtrlAITargetVersion = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_READONLY_AI_VERSION);
	wxASSERT(pTextCtrlAITargetVersion != NULL);
	pTextCtrlAITargetVersion->SetBackgroundColour(m_pApp->sysColorBtnFace);

	pBtnSelectAllVerses = (wxButton*)FindWindowById(ID_BUTTON_SELECT_ALL_VS);
	wxASSERT(pBtnSelectAllVerses != NULL);

	pBtnUnSelectAllVerses = (wxButton*)FindWindowById(ID_BUTTON_UNSELECT_ALL_VS);
	wxASSERT(pBtnUnSelectAllVerses != NULL);

	pBtnTransferSelectedVerses = (wxButton*)FindWindowById(wxID_OK); // Note: this button acts as wxID_OK
	wxASSERT(pBtnTransferSelectedVerses != NULL);

	pBtnCancel = (wxButton*)FindWindowById(wxID_CANCEL);
	wxASSERT(pBtnCancel != NULL);

	pRadioUseAIVersion = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_USE_AI_VERSION);
	wxASSERT(pRadioUseAIVersion != NULL);

	pRadioRetainPTVersion = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_RETAIN_PT_VERSION);
	wxASSERT(pRadioRetainPTVersion != NULL);

	pStaticTextCtrlTopInfoBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_READONLY_TOP); // for substituting Paratext/Bibledit into %s and %s
	wxASSERT(pStaticTextCtrlTopInfoBox != NULL);
	pStaticTextCtrlTopInfoBox->SetBackgroundColour(m_pApp->sysColorBtnFace);

	pStaticInfoLine1 = (wxStaticText*)FindWindowById(ID_TEXT_INFO_1); 
	wxASSERT(pStaticInfoLine1 != NULL);

	pStaticInfoLine2 = (wxStaticText*)FindWindowById(ID_TEXT_INFO_2); // for substituting Paratext/Bibledit into %s
	wxASSERT(pStaticInfoLine2 != NULL);

	pStaticInfoLine3 = (wxStaticText*)FindWindowById(ID_TEXT_INFO_3); // for substituting Paratext/Bibledit into %s
	wxASSERT(pStaticInfoLine3 != NULL);

	pStaticInfoLine4 = (wxStaticText*)FindWindowById(ID_TEXT_INFO_4); // for substituting Paratext/Bibledit into %s
	wxASSERT(pStaticInfoLine4 != NULL);

	pStaticPTVsTitle = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_PT_VS_TITLE);
	wxASSERT(pStaticPTVsTitle != NULL);

	pCheckBoxMakeEditable = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PT_EDITABLE);

	CurrentListBoxHighlightedIndex = 0;

	// The following is the wxTextCtrl we want to replace
	pTextCtrlPTTargetVersion = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_EDITABLE_PT_VERSION);
	wxASSERT(pTextCtrlPTTargetVersion != NULL);
	pTextCtrlPTTargetVersion->SetBackgroundColour(m_pApp->sysColorBtnFace);

	// Get parameters relevant to the window
	wxSize size = pTextCtrlPTTargetVersion->GetSize();
	int id = pTextCtrlPTTargetVersion->GetId();
	wxPoint position = pTextCtrlPTTargetVersion->GetPosition();

	// Do the replacement... (see ChooseTranslation.cpp 140-180 for how)
	wxBoxSizer* pContSizerOfTextCtrl = (wxBoxSizer*)pTextCtrlPTTargetVersion->GetContainingSizer();
    wxASSERT(pContSizerOfTextCtrl == pPT_BoxSizer);
	// We don't have a tooltip on the text box, it would get in the user's way and be annoying
    // Delete the existing text box
	if (pTextCtrlPTTargetVersion != NULL)
	    delete pTextCtrlPTTargetVersion;

}

CCollabVerseConflictDlg::~CCollabVerseConflictDlg() // destructor
{
	
}

void CCollabVerseConflictDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	wxASSERT(pConflictsArray != NULL);
	
	// Testing Data:
	/*
	// Source text data for ROM 3:1-12 (from PT's NIV version)
\v 1 What advantage, then, is there in being a Jew, or what value is there in circumcision?
\v 2 Much in every way! First of all, they have been entrusted with the very words of God.
\p\v 3 What if some did not have faith? Will their lack of faith nullify God's faithfulness?
\v 4 Not at all! Let God be true, and every man a liar. As it is written:\q1 “So that you may be proved right when you speak\q2 and prevail when you judge.”\f + \fr 3.4 \ft PSA 51.4\f*
\p\v 5 But if our unrighteousness brings out God's righteousness more clearly, what shall we say? That God is unjust in bringing his wrath on us? (I am using a human argument.)
\v 6 Certainly not! If that were so, how could God judge the world?
\v 7 Someone might argue, “If my falsehood enhances God's truthfulness and so increases his glory, why am I still condemned as a sinner?”
\v 8 Why not say–as we are being slanderously reported as saying and as some claim that we say–“Let us do evil that good may result”? Their condemnation is deserved.\s1 No One is Righteous
\p\v 9 What shall we conclude then? Are we any better\f + \fr 3.9 \ft Or \fq worse\f*? Not at all! We have already made the charge that Jews and Gentiles alike are all under sin.
\v 10 As it is written:\q1 “There is no one righteous, not even one;
\q2\v 11 there is no one who understands,\q2 no one who seeks God.
\q1\v 12 All have turned away,\q2 they have together become worthless;\q1 there is no one who does good,\q2 not even one.”\f + \fr 3.12 \ft PSA 14.1-3; PSA 53.1-3; ECC 7.20\f*
*/

	// Initialize some wxArrayStrings for test data
/*
	// AI Translation data for ROM 3:1-12 (Dynamic Tok Pisin)
\v 1 Sapos dispela tok mi bin raitim yupela i tru, ol lain Ju i winim ol arapela manmeri olsem wanem? Pasin bilong katim skin na kamap lain Ju i save helpim ol manmeri o nogat a?
\v 2 Olaman! I gat kain kain gutpela helpim bilong en ia! Nambawan em i olsem: God em i bin givim tok bilong en long ol lain Ju long lukautim (na helpim ol manmeri i ken save long ol tok bilong en.)
\v 3 Tru, sampela ol i no bin bilip na bihainim dispela tok. Tasol yupela i ting wanem? Ating dispela pasin nogut ol i bihainim bai i mekim God i (senisim tingting bilong en na) lusim tok bilong en a?
\v 4 Nogat tru! Sapos olgeta man i mas mekim pasin bilong giaman, God wanpela tasol nogat! Bai em i save tok tru oltaim, olsem buk Sam i tok:\q<<Olgeta manmeri i ken save tok bilong yu i stret tumas na sapos ol i kotim yu, bai yu winim kot tru.>>
\p\v 5 Orait, yupela i ting mi tok wanem a? Sapos pasin nogut yumi save mekim em i soim olgeta manmeri stretpela pasin bilong God, bai yumi tok wanem? God em i save bekim pe bilong sin long yumi, ating pasin bilong God em i no stret a? (Mi bihainim tingting bilong yumi man tasol na mi mekim dispela tok.)
\v 6 (Yupela ting mi tok olsem a?) Nogat tru! Sapos God em i no stretpela, olsem wanem bai em i skelim pasin bilong olgeta manmeri? Em i no inap ia?
\v 7 (Orait i gat narapela askim.) Sapos tok giaman bilong mi i helpim ol manmeri i save tok tru bilong God. Na sapos dispela i mekim biknem bilong en i kamap moa yet, orait, bilong wanem em i kolim mi man nogut i gat sin na i bekim pe nogut long mi?
\v 8 Sapos dispela kain tok i tru, mobeta yumi tok olsem, <<Goan,yumi mekim pasin nogut, long wanem, gutpela pasin i ken kamaplong dispela samting.>> Ating pasin olsem em i orait a? Nogat tru! Tasol sampela manmeri i tok, <<Yes, tok bilong Pol i olsem tasol.>> Em i stret bai God i bekim pe nogut long ol dispelamanmeri.\s I no gat wanpela man o meri i stretpela long tingting bilong God
\p\v 9 Orait bai yumi tok wanem? Ating yumi ol lain Ju i winim ol (lain) arapela manmeri? Ating nogat ia? Long wanem, yumi tok pinis, sin i karamapim yumi olgeta lain Ju wantaim ol arapela lain.
\v 10 Dispela tok em i tru. Long wanem, buk Sam bilong Olpela Testamen (o Baibel) i tok olsem:\q<<I no gat wanpela stretpela man i stap. Nogat tru.
\q\v 11 I no gat wanpela man i gat save. I no gat wanpela i wok long painim God.
\q\v 12 Olgeta i bin lusim rot. Olgeta i bin kamap man nogut tru.\qI no gat man i save mekim gutpela pasin. Yes, i no gat wanpela.
*/

/*
	// PT Translation data for ROM 3:1-12 (From PT's UBS Tok Pisin)
	// Removed the /x ... /x* x-refs
\v 1 Sapos pasin bilong katim skin em i no samting bilong bodi tasol, orait ol Juda i winim ol arapela man olsem wanem? Wanem gutpela samting i save kamap long pasin bilong katim skin?
\v 2 Olaman! God i givim ol kain kain gutpela samting long ol Juda. Namba wan em i olsem. God i bin givim tok bilong en long ol Juda bilong lukautim.
\v 3 Tru, sampela i no bin bihainim dispela tok. Tasol olsem wanem? Sapos ol i no bilip, ating dispela bai i mekim God i no bihainim tok bilong em?
\v 4 Nogat tru. Olgeta man i save giaman, tasol God wanpela i save mekim tok tru oltaim. Olsem buk bilong God i tok,\q1 <<God, yu mekim\q3 stretpela pasin tasol\q2 na yu kotim mi.\q1 Na sapos ol i ting long kotim yu,\q2 bai yu winim kot tru.>>
\p\v 5-6 Orait olsem wanem? Sapos pasin nogut bilong yumi em i kamapim ples klia stretpela pasin bilong God, bai yumi tok wanem? Taim God i bekim pe nogut bilong sin bilong yumi, ating em i mekim pasin i no stret, a? Nogat tru. Sapos God i no bihainim stretpela pasin oltaim, olsem wanem bai em inap skelim pasin bilong olgeta manmeri? Dispela kain tingting em i tingting bilong yumi man tasol.
\v 7 Em i wankain olsem tingting bilong man i tok olsem, <<Sapos tok giaman bilong mi i mekim tok tru bilong God i kamap ples klia, na dispela i mekim biknem bilong en i kamap moa yet, orait olsem wanem na em i kolim mi man bilong mekim sin, na em i bekim pe nogut long mi?>>
\v 8 Na em i wankain olsem dispela rabis tok, <<Goan, yumi mekim pasin nogut, na long dispela rot gutpela pasin bai i kamap.>> Sampela man i save sutim tok long mi na tok olsem, tok mi yet mi save autim em i wankain olsem dispela rabis tok. God bai i kotim ol dispela man na bekim pe nogut long ol inap long pasin ol i bin mekim.\s I no gat wanpela man i save mekim stretpela pasin
\p\v 9 Orait olsem wanem? Ating yumi Juda i winim ol arapela man? Nogat tru. Yumi tok pinis, sin i karamapim yumi olgeta, yumi Juda na Grik wantaim.
\v 10 Buk bilong God i gat tok long dispela olsem,\q1 <<I no gat wanpela man\q2 i save mekim stretpela pasin.\q1 Nogat tru.
\q1\v 11 I no gat wanpela man\q2 i gat gutpela save.\q1 I no gat wanpela\q2 i wok long painim God.
\q1\v 12 Olgeta i lusim gutpela rot pinis.\q1 Olgeta i wankain tasol,\q2 ol i man nogut tru.\q1 I no gat wanpela bilong ol\q2 i save mekim gutpela pasin.\q1 Nogat tru.
*/

	/*
	// one wxArrayString will contain the verse_references of conflicted verses, 
	// one wxArrayString will contain the source_text verses, one wxArrayString 
	// will contain the ai_target_text verses and the fourth one will contain the 
	// pt_target_text verses, and the original pt target verses will be in another
	verseRefsArray.Add(_T("ROM 3:1"));
	verseRefsArray.Add(_T("ROM 3:2"));
	verseRefsArray.Add(_T("ROM 3:3"));
	verseRefsArray.Add(_T("ROM 3:4"));
	verseRefsArray.Add(_T("ROM 3:5"));
	verseRefsArray.Add(_T("ROM 3:6"));
	verseRefsArray.Add(_T("ROM 3:7"));
	verseRefsArray.Add(_T("ROM 3:8"));
	verseRefsArray.Add(_T("ROM 3:9"));
	verseRefsArray.Add(_T("ROM 3:10"));
	verseRefsArray.Add(_T("ROM 3:11"));
	verseRefsArray.Add(_T("ROM 3:12"));
	
	// Note: back slash characters \ need to be escaped as \\ within the string elements.
	sourceTextVsArray.Add(_T("\\v 1 What advantage, then, is there in being a Jew, or what value is there in circumcision?"));
	sourceTextVsArray.Add(_T("\\v 2 Much in every way! First of all, they have been entrusted with the very words of God."));
	sourceTextVsArray.Add(_T("\\p\\v 3 What if some did not have faith? Will their lack of faith nullify God's faithfulness?"));
	sourceTextVsArray.Add(_T("\\v 4 Not at all! Let God be true, and every man a liar. As it is written:\\q1 <<So that you may be proved right when you speak\\q2 and prevail when you judge.>>\\f + \\fr 3.4 \\ft PSA 51.4\\f*"));
	sourceTextVsArray.Add(_T("\\p\\v 5 But if our unrighteousness brings out God's righteousness more clearly, what shall we say? That God is unjust in bringing his wrath on us? (I am using a human argument.)"));
	sourceTextVsArray.Add(_T("\\v 6 Certainly not! If that were so, how could God judge the world?"));
	sourceTextVsArray.Add(_T("\\v 7 Someone might argue, <<If my falsehood enhances God's truthfulness and so increases his glory, why am I still condemned as a sinner?>>"));
	sourceTextVsArray.Add(_T("\\v 8 Why not say-as we are being slanderously reported as saying and as some claim that we say-<<Let us do evil that good may result>>? Their condemnation is deserved.\\s1 No One is Righteous"));
	sourceTextVsArray.Add(_T("\\p\\v 9 What shall we conclude then? Are we any better\\f + \\fr 3.9 \\ft Or \\fq worse\\f*? Not at all! We have already made the charge that Jews and Gentiles alike are all under sin."));
	sourceTextVsArray.Add(_T("\\v 10 As it is written:\\q1 <<There is no one righteous, not even one;"));
	sourceTextVsArray.Add(_T("\\q2\\v 11 there is no one who understands,\\q2 no one who seeks God."));
	sourceTextVsArray.Add(_T("\\q1\\v 12 All have turned away,\\q2 they have together become worthless;\\q1 there is no one who does good,\\q2 not even one.>>\\f + \\fr 3.12 \\ft PSA 14.1-3; PSA 53.1-3; ECC 7.20\\f*"));
	
	aiTargetTextVsArray.Add(_T("\\v 1 Sapos dispela tok mi bin raitim yupela i tru, ol lain Ju i winim ol arapela manmeri olsem wanem? Pasin bilong katim skin na kamap lain Ju i save helpim ol manmeri o nogat a?"));
	aiTargetTextVsArray.Add(_T("\\v 2 Olaman! I gat kain kain gutpela helpim bilong en ia! Nambawan em i olsem: God em i bin givim tok bilong en long ol lain Ju long lukautim (na helpim ol manmeri i ken save long ol tok bilong en.)"));
	aiTargetTextVsArray.Add(_T("\\v 3 Tru, sampela ol i no bin bilip na bihainim dispela tok. Tasol yupela i ting wanem? Ating dispela pasin nogut ol i bihainim bai i mekim God i (senisim tingting bilong en na) lusim tok bilong en a?"));
	aiTargetTextVsArray.Add(_T("\\v 4 Nogat tru! Sapos olgeta man i mas mekim pasin bilong giaman, God wanpela tasol nogat! Bai em i save tok tru oltaim, olsem buk Sam i tok:\\q<<Olgeta manmeri i ken save tok bilong yu i stret tumas na sapos ol i kotim yu, bai yu winim kot tru.>>"));
	aiTargetTextVsArray.Add(_T("\\p\\v 5 Orait, yupela i ting mi tok wanem a? Sapos pasin nogut yumi save mekim em i soim olgeta manmeri stretpela pasin bilong God, bai yumi tok wanem? God em i save bekim pe bilong sin long yumi, ating pasin bilong God em i no stret a? (Mi bihainim tingting bilong yumi man tasol na mi mekim dispela tok.)"));
	aiTargetTextVsArray.Add(_T("\\v 6 (Yupela ting mi tok olsem a?) Nogat tru! Sapos God em i no stretpela, olsem wanem bai em i skelim pasin bilong olgeta manmeri? Em i no inap ia?"));
	aiTargetTextVsArray.Add(_T("\\v 7 (Orait i gat narapela askim.) Sapos tok giaman bilong mi i helpim ol manmeri i save tok tru bilong God. Na sapos dispela i mekim biknem bilong en i kamap moa yet, orait, bilong wanem em i kolim mi man nogut i gat sin na i bekim pe nogut long mi?"));
	aiTargetTextVsArray.Add(_T("\\v 8 Sapos dispela kain tok i tru, mobeta yumi tok olsem, <<Goan,yumi mekim pasin nogut, long wanem, gutpela pasin i ken kamaplong dispela samting.>> Ating pasin olsem em i orait a? Nogat tru! Tasol sampela manmeri i tok, <<Yes, tok bilong Pol i olsem tasol.>> Em i stret bai God i bekim pe nogut long ol dispelamanmeri.\\s I no gat wanpela man o meri i stretpela long tingting bilong God"));
	aiTargetTextVsArray.Add(_T("\\p\\v 9 Orait bai yumi tok wanem? Ating yumi ol lain Ju i winim ol (lain) arapela manmeri? Ating nogat ia? Long wanem, yumi tok pinis, sin i karamapim yumi olgeta lain Ju wantaim ol arapela lain."));
	aiTargetTextVsArray.Add(_T("\\v 10 Dispela tok em i tru. Long wanem, buk Sam bilong Olpela Testamen (o Baibel) i tok olsem:\\q<<I no gat wanpela stretpela man i stap. Nogat tru."));
	aiTargetTextVsArray.Add(_T("\\q\\v 11 I no gat wanpela man i gat save. I no gat wanpela i wok long painim God."));
	aiTargetTextVsArray.Add(_T("\\q\\v 12 Olgeta i bin lusim rot. Olgeta i bin kamap man nogut tru.\\qI no gat man i save mekim gutpela pasin. Yes, i no gat wanpela."));
	
	ptTargetTextVsArray.Add(_T("\\v 1 Sapos pasin bilong katim skin em i no samting bilong bodi tasol, orait ol Juda i winim ol arapela man olsem wanem? Wanem gutpela samting i save kamap long pasin bilong katim skin?"));
	ptTargetTextVsArray.Add(_T("\\v 2 Olaman! God i givim ol kain kain gutpela samting long ol Juda. Namba wan em i olsem. God i bin givim tok bilong en long ol Juda bilong lukautim."));
	ptTargetTextVsArray.Add(_T("\\v 3 Tru, sampela i no bin bihainim dispela tok. Tasol olsem wanem? Sapos ol i no bilip, ating dispela bai i mekim God i no bihainim tok bilong em?"));
	ptTargetTextVsArray.Add(_T("\\v 4 Nogat tru. Olgeta man i save giaman, tasol God wanpela i save mekim tok tru oltaim. Olsem buk bilong God i tok,\\q1 <<God, yu mekim\\q3 stretpela pasin tasol\\q2 na yu kotim mi.\\q1 Na sapos ol i ting long kotim yu,\\q2 bai yu winim kot tru.>>"));
	ptTargetTextVsArray.Add(_T("\\p\\v 5-6 Orait olsem wanem? Sapos pasin nogut bilong yumi em i kamapim ples klia stretpela pasin bilong God, bai yumi tok wanem? Taim God i bekim pe nogut bilong sin bilong yumi, ating em i mekim pasin i no stret, a? Nogat tru. Sapos God i no bihainim stretpela pasin oltaim, olsem wanem bai em inap skelim pasin bilong olgeta manmeri? Dispela kain tingting em i tingting bilong yumi man tasol."));
	ptTargetTextVsArray.Add(_T(""));
	ptTargetTextVsArray.Add(_T("\\v 7 Em i wankain olsem tingting bilong man i tok olsem, <<Sapos tok giaman bilong mi i mekim tok tru bilong God i kamap ples klia, na dispela i mekim biknem bilong en i kamap moa yet, orait olsem wanem na em i kolim mi man bilong mekim sin, na em i bekim pe nogut long mi?>>"));
	ptTargetTextVsArray.Add(_T("\\v 8 Na em i wankain olsem dispela rabis tok, <<Goan, yumi mekim pasin nogut, na long dispela rot gutpela pasin bai i kamap.>> Sampela man i save sutim tok long mi na tok olsem, tok mi yet mi save autim em i wankain olsem dispela rabis tok. God bai i kotim ol dispela man na bekim pe nogut long ol inap long pasin ol i bin mekim.\\s I no gat wanpela man i save mekim stretpela pasin"));
	ptTargetTextVsArray.Add(_T("\\p\\v 9 Orait olsem wanem? Ating yumi Juda i winim ol arapela man? Nogat tru. Yumi tok pinis, sin i karamapim yumi olgeta, yumi Juda na Grik wantaim."));
	ptTargetTextVsArray.Add(_T("\\v 10 Buk bilong God i gat tok long dispela olsem,\\q1 <<I no gat wanpela man\\q2 i save mekim stretpela pasin.\\q1 Nogat tru."));
	ptTargetTextVsArray.Add(_T("\\q1\\v 11 I no gat wanpela man\\q2 i gat gutpela save.\\q1 I no gat wanpela\\q2 i wok long painim God."));
	ptTargetTextVsArray.Add(_T("\\q1\\v 12 Olgeta i lusim gutpela rot pinis.\\q1 Olgeta i wankain tasol,\\q2 ol i man nogut tru.\\q1 I no gat wanpela bilong ol\\q2 i save mekim gutpela pasin.\\q1 Nogat tru."));
	*/
	size_t count = pConflictsArray->GetCount();
	size_t index;
	ConflictRes* pCR = NULL;
	for (index = 0; index < count; index++)
	{
		pCR = (ConflictRes*)pConflictsArray->Item(index);
		wxASSERT(pCR != NULL);
		verseRefsArray.Add(MakeVerseReference(pCR));
		sourceTextVsArray.Add(pCR->srcText);
		aiTargetTextVsArray.Add(pCR->AIText);
		ptTargetTextVsArray.Add(pCR->PTorBEText_edited);
		ptTargetTextVsOriginalArray.Add(pCR->PTorBEText_original);
	}

	// Set font and directionality for the three edit boxes
	// For the "Source text of verse selected at left" edit box:
	#ifdef _RTL_FLAGS
	m_pApp->SetFontAndDirectionalityForDialogControl(m_pApp->m_pSourceFont, pTextCtrlSourceText, NULL,
													NULL, NULL, m_pApp->m_pDlgSrcFont, m_pApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	m_pApp->SetFontAndDirectionalityForDialogControl(m_pApp->m_pSourceFont, pTextCtrlSourceText, NULL, 
													NULL, NULL, m_pApp->m_pDlgSrcFont);
	#endif

	// For the "Translation of verse in Adapt It" edit box:
	#ifdef _RTL_FLAGS
	m_pApp->SetFontAndDirectionalityForDialogControl(m_pApp->m_pTargetFont, pTextCtrlAITargetVersion, NULL,
													NULL, NULL, m_pApp->m_pDlgTgtFont, m_pApp->m_bTgtRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	m_pApp->SetFontAndDirectionalityForDialogControl(m_pApp->m_pTargetFont, pTextCtrlAITargetVersion, NULL, 
													NULL, NULL, m_pApp->m_pDlgTgtFont);
	#endif

	// For the "Translation of verse in Paratext" edit box:
	#ifdef _RTL_FLAGS
	m_pApp->SetFontAndDirectionalityForDialogControl(m_pApp->m_pTargetFont, pTextCtrlPTTargetVersion, NULL,
													NULL, NULL, m_pApp->m_pDlgTgtFont, m_pApp->m_bTgtRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	m_pApp->SetFontAndDirectionalityForDialogControl(m_pApp->m_pTargetFont, pTextCtrlPTTargetVersion, NULL, 
													NULL, NULL, m_pApp->m_pDlgTgtFont);
	#endif

	// Set the static strings in controls that need to distinguish external editor string "Paratext" or "Bibledit"
	wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
	wxASSERT(m_pApp->m_collaborationEditor == _T("Paratext") ||m_pApp->m_collaborationEditor == _T("Bibledit"));
	// The wxDesigner resource already has "Paratext" in its string resources,
	// we need only change those to "Bibledit" if we're using Bibledit
	if (m_pApp->m_collaborationEditor == _T("Bibledit"))
	{
		wxString tempStr;
		tempStr = pStaticTextCtrlTopInfoBox->GetValue();
		tempStr.Replace(_T("Paratext"),_T("Bibledit"));
		pStaticTextCtrlTopInfoBox->ChangeValue(tempStr);

		tempStr = pStaticInfoLine2->GetLabel();
		tempStr.Replace(_T("Paratext"),_T("Bibledit"));
		pStaticInfoLine2->SetLabel(tempStr);

		tempStr = pStaticInfoLine3->GetLabel();
		tempStr.Replace(_T("Paratext"),_T("Bibledit"));
		pStaticInfoLine3->SetLabel(tempStr);

		tempStr = pStaticPTVsTitle->GetLabel();
		tempStr.Replace(_T("Paratext"),_T("Bibledit"));
		pStaticPTVsTitle->SetLabel(tempStr);

		tempStr = pRadioRetainPTVersion->GetLabel();
		tempStr.Replace(_T("Paratext"),_T("Bibledit"));
		pRadioRetainPTVersion->SetLabel(tempStr);

		tempStr = this->GetTitle(); // get the dialog title string
		tempStr.Replace(_T("Paratext"),_T("Bibledit"));
		this->SetTitle(tempStr);

	}
	
	// Load the test arrays into the appropriate dialog controls
	pCheckListBoxVerseRefs->Clear();
	pCheckListBoxVerseRefs->Append(verseRefsArray);

	// Set focus on the first item in the verse reference list; the
	// CurrentListBoxHighlightedIndex will be 0
	pCheckListBoxVerseRefs->SetFocus();
	if (pCheckListBoxVerseRefs->GetCount() > 0)
	{
		CurrentListBoxHighlightedIndex = 0;
		pCheckListBoxVerseRefs->SetSelection(CurrentListBoxHighlightedIndex);
	}
	FillEditBoxesWithVerseTextForHighlightedItem();
	SyncRadioButtonsWithHighlightedItemTickState();
}

void CCollabVerseConflictDlg::SyncRadioButtonsWithHighlightedItemTickState()
{
	// The radio buttons should always stay in sync with whatever line in the
	// list is currently highlighted,
	int nSel;
	nSel = pCheckListBoxVerseRefs->GetSelection();
	if (pCheckListBoxVerseRefs->IsChecked(nSel))
	{
		pRadioUseAIVersion->SetValue(TRUE);
		pRadioRetainPTVersion->SetValue(FALSE);
	}
	else
	{
		pRadioUseAIVersion->SetValue(FALSE);
		pRadioRetainPTVersion->SetValue(TRUE);
	}
}

void CCollabVerseConflictDlg::FillEditBoxesWithVerseTextForHighlightedItem()
{
	pTextCtrlSourceText->ChangeValue(sourceTextVsArray.Item(CurrentListBoxHighlightedIndex));
	pTextCtrlAITargetVersion->ChangeValue(aiTargetTextVsArray.Item(CurrentListBoxHighlightedIndex));
	pTextCtrlPTTargetVersion->ChangeValue(ptTargetTextVsArray.Item(CurrentListBoxHighlightedIndex));
}

void CCollabVerseConflictDlg::OnCheckListBoxTickChange(wxCommandEvent& event)
{
	// This handler is entered when a check box has been ticked or unticked
	// Note: Calling GetSelection() on the event gives the index of the check
	// box that was ticked or unticked. Calling GetSelection() on the
	// pCheckListBoxVerseRefs check list box control itself give the index of
	// the list item that is highlighted/selected. 
	// Since highlighting and ticking of check boxes can happen independently
	// it would be good to make ticking or unticking of a check box force the
	// selection to change to that line in the list. That way a user can see
	// the verse contents of source and targets that he is affecting by the
	// ticking or unticking action. We don't keep the highlight and ticking
	// action in sync going the other direction, that is, when a user changes
	// the highlighted item (using up/down arrow keys or clicking on the line
	// item, but away from the check box) we don't change any ticks in any
	// check boxes.
	int nTickSel;
	nTickSel = event.GetSelection(); // GetSelection here gets the index of the check box item in which the tick was made
	// Change the highlight selection to the tick change item
	CurrentListBoxHighlightedIndex = nTickSel; // keep the CurrentListBoxHighlightedIndex up to date
	pCheckListBoxVerseRefs->SetSelection(CurrentListBoxHighlightedIndex); // sync the highlighted to a newly ticked/unticked box - no command events emitted
	FillEditBoxesWithVerseTextForHighlightedItem();
	SyncRadioButtonsWithHighlightedItemTickState(); // sync the radio buttons to agree with the highlight
}

void CCollabVerseConflictDlg::OnListBoxSelChange(wxCommandEvent& WXUNUSED(event))
{
	// This handler is entered when a list box selection has changed (user clicks
	// on a line item in the list box, but not on the check box itself).
	//wxMessageBox(_T("List box selection changed"));
	
	// Make the radio buttons agree with the current check box selection
	int nSel;
	nSel = pCheckListBoxVerseRefs->GetSelection();
	CurrentListBoxHighlightedIndex = nSel; // keep the CurrentListBoxHighlightedIndex up to date
	// Change the displayed text in the 3 edit boxes to match the
	// selection/highlight index. Use the wxTextCtrl::ChangeValue() method rather 
	// than the deprecated SetValue() method. SetValue() generates an undesirable
	// wxEVT_COMMAND_TEXT_UPDATED event whereas ChangeValue() does not trigger
	// that event.
	FillEditBoxesWithVerseTextForHighlightedItem();
	SyncRadioButtonsWithHighlightedItemTickState(); // keep the radio buttons in sync
}

void CCollabVerseConflictDlg::OnRadioUseAIVersion(wxCommandEvent& WXUNUSED(event))
{
	// This handler is entered when user clicks on the "Use this Adapt It
	// version" radio button.
	// This action should add a tick on the current highlighted item 
	// in the verse references check list box.
	pCheckListBoxVerseRefs->Check(CurrentListBoxHighlightedIndex,TRUE);
	// Set the "Use this Adapt It version" radio button and unset the "Retain
	// this Paratext version" radio button
	SyncRadioButtonsWithHighlightedItemTickState();
}

void CCollabVerseConflictDlg::OnRadioRetainPTVersion(wxCommandEvent& WXUNUSED(event))
{
	// This handler is entered whent the user clicks on the "Retain this
	// Paratext version" radio button
	// This action should remove any tick on the current item in the verse
	// references check list box.
	pCheckListBoxVerseRefs->Check(CurrentListBoxHighlightedIndex,FALSE);
	// Unset the "Use this Adapt It version" radio button and set the "Retain
	// this Paratext version" radio button
	SyncRadioButtonsWithHighlightedItemTickState();
}

void CCollabVerseConflictDlg::OnSelectAllVersesButton(wxCommandEvent& WXUNUSED(event))
{
	// When Selecting All the check boxes in the list, we should keep the
	// current selection the same.
	int nItemCount;
	nItemCount = pCheckListBoxVerseRefs->GetCount();
	int i;
	for (i = 0; i < nItemCount; i++)
	{
		pCheckListBoxVerseRefs->Check(i,TRUE); // TRUE ticks item
	}
	// The tick state of the currently highlighted item may have changed so
	// update the radio buttons if needed.
	SyncRadioButtonsWithHighlightedItemTickState();
}

void CCollabVerseConflictDlg::OnUnSelectAllVersesButton(wxCommandEvent& WXUNUSED(event))
{
	// When Selecting All the check boxes in the list, we should keep the
	// current selection the same.
	int nItemCount;
	nItemCount = pCheckListBoxVerseRefs->GetCount();
	int i;
	for (i = 0; i < nItemCount; i++)
	{
		pCheckListBoxVerseRefs->Check(i,FALSE); // FALSE unticks item
	}
	SyncRadioButtonsWithHighlightedItemTickState();
}



// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CCollabVerseConflictDlg::OnOK(wxCommandEvent& event) 
{
	// sample code
	//wxListBox* pListBox;
	//pListBox = (wxListBox*)FindWindowById(IDC_LISTBOX_ADAPTIONS);
	//int nSel;
	//nSel = pListBox->GetSelection();
	//if (nSel == LB_ERR) // LB_ERR is #define -1
	//{
	//	wxMessageBox(_T("List box error when getting the current selection"), _T(""), wxICON_EXCLAMATION | wxOK);
	//}
	//m_projectName = pListBox->GetString(nSel);
	
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods
wxString CCollabVerseConflictDlg::MakeVerseReference(ConflictRes* p)
{
	wxString aVsRef = _T(" ");
	wxASSERT(p != NULL);
	aVsRef = p->bookCodeStr + _T(" ");
	aVsRef += p->chapterRefStr + _T(":");
	aVsRef += p->verseRefStr;
	return aVsRef;
}

