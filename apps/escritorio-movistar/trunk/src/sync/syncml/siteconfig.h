#ifndef SITECONFIG_H_
#define SITECONFIG_H_

#ifndef _MSC_VER

#include "config.h"

#else 

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#pragma warning(disable: 4786)
#include <afxtempl.h>
#include <afxole.h>
//
#include <boost/config.hpp>  // Include boosts compiler-specific "fixes"
//using std::min;              // Make them globally available
//using std::max;

#include "afx_oldtoolinfo.h"
#include "identifiers.h"
#include <afxcview.h>
#include <afxdlgs.h>

#include "ptools/ptools.h"

#endif

#endif /*SITECONFIG_H_*/
