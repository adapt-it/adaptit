/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AIPrintPreviewFrame.h
/// \author			Kevin Bradford
/// \date_created	23 September 2011
/// \date_revised	
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the header file for the CAIPrintPreviewFrame class. 
/// The CAIPrintPreviewFrame class is the derived from wxPreviewFrame. 
/// It allows control of the underlying frame/window
/// during the print preview process.
/// \derivation		The CAIPrintPreviewFrame class is derived from wxPreviewFrame.
/////////////////////////////////////////////////////////////////////////////
#ifndef AIPrintPreviewFrame_h
#define AIPrintPreviewFrame_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "AIPrintPreviewFrame.h"
#endif

// forward declarations
class CAdapt_ItView;

class CAIPrintPreviewFrame :
	public wxPreviewFrame
{
public:
	CAIPrintPreviewFrame(
		CAdapt_ItView * view,
		wxPrintPreviewBase *  preview,  
		wxWindow *  parent,  
		const wxString &  title = "Print Preview",  
		const wxPoint &  pos = wxDefaultPosition,  
		const wxSize &  size = wxDefaultSize,  
		long  style = wxDEFAULT_FRAME_STYLE,  
		const wxString &  name = wxFrameNameStr);
	~CAIPrintPreviewFrame(void);

	void HideGlossesOnClose( bool );

private:
	CAdapt_ItView* pView;
	bool bHideGlossesOnClose;


};

#endif // AIPrintPreviewFrame_h
