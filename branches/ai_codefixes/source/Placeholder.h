/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Placeholder.h
/// \author			Erik Brommers
/// \date_created	02 April 2010
/// \date_revised	02 April 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the header file for the CPlaceholder class. 
/// The CPlaceholder class contains methods for working with placeholder 
/// elements within the translated text.
/// \derivation		The CPlaceholder class is derived from wxEvtHandler.
/////////////////////////////////////////////////////////////////////////////

#ifndef PLACEHOLDER_H
#define PLACEHOLDER_H

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "Placeholder.h"
#endif


//////////////////////////////////////////////////////////////////////////////////
/// The CPlaceholder class contains methods for working with placeholder 
/// elements within the translated text.
/// \derivation		The CPlaceholder class is derived from wxEvtHandler.
/////////////////////////////////////////////////////////////////////////////
class CPlaceholder : public wxEvtHandler
	{
	public:
		
		CPlaceholder(); // default constructor
		CPlaceholder(CAdapt_ItApp* app); // use this one
		
		virtual ~CPlaceholder();	// destructor
		
		// Utility functions
		CLayout*		CPlaceholder::GetLayout();
		CAdapt_ItView*	CPlaceholder::GetView();
		CAdapt_ItApp*	CPlaceholder::GetApp();
		
	private:
		CAdapt_ItApp*	m_pApp;	// The app owns this
		CLayout*		m_pLayout;
		CAdapt_ItView*	m_pView;
		
		DECLARE_EVENT_TABLE()
	};

#endif // _PLACEHOLDER_H
