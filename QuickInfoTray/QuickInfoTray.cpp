// QuickInfoTray.cpp : Defines the entry point for the application.
//

#include <string>
#include <sstream>
#include <iostream>
#include "framework.h"
#include "QuickInfoTray.h"

#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "VERSION.lib")

#define MAX_LOADSTRING 100

UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
UINT const WMAPP_HIDEFLYOUT = WM_APP + 2;
UINT_PTR const HIDEFLYOUT_TIMER_ID = 1;

struct LANGANDCODEPAGE
{
   WORD wLanguage;
   WORD wCodePage;
} *lpTranslate;

// Use a guid to uniquely identify our icon
class __declspec( uuid( "1BF1C1FA-3637-4C14-91D3-1850DB623F6E" ) ) QuickInfoIcon;

// Global Variables:
HINSTANCE hInst;                                  // current instance
WCHAR szTitle[ MAX_LOADSTRING ];                  // The title bar text
WCHAR szWindowClass[ MAX_LOADSTRING ];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass( HINSTANCE hInstance );
BOOL                InitInstance( HINSTANCE, int );
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR CALLBACK    About( HWND, UINT, WPARAM, LPARAM );

int APIENTRY wWinMain( _In_ HINSTANCE hInstance,
                       _In_opt_ HINSTANCE hPrevInstance,
                       _In_ LPWSTR    lpCmdLine,
                       _In_ int       nCmdShow )
{
   UNREFERENCED_PARAMETER( hPrevInstance );
   UNREFERENCED_PARAMETER( lpCmdLine );

   // TODO: Place code here.

   // Initialize global strings
   LoadStringW( hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING );
   LoadStringW( hInstance, IDS_QUICKINFOTRAY, szWindowClass, MAX_LOADSTRING );
   MyRegisterClass( hInstance );

   // Perform application initialization:
   if ( !InitInstance( hInstance, nCmdShow ) )
   {
      return FALSE;
   }

   HACCEL hAccelTable = LoadAccelerators( hInstance, MAKEINTRESOURCE( IDC_QUICKINFOTRAY ) );

   MSG msg;

   // Main message loop:
   while ( GetMessage( &msg, nullptr, 0, 0 ) )
   {
      if ( !TranslateAccelerator( msg.hwnd, hAccelTable, &msg ) )
      {
         TranslateMessage( &msg );
         DispatchMessage( &msg );
      }
   }

   return (int) msg.wParam;
}

BOOL PopulateInfo( WCHAR* string, size_t length )
{
   std::stringstream buffer;

   ULONG size = sizeof( IP_ADAPTER_INFO );
   PIP_ADAPTER_INFO pAdapterInfo = (PIP_ADAPTER_INFO) malloc( sizeof( IP_ADAPTER_INFO ) );

   if ( pAdapterInfo == NULL )
   {
      MessageBox( NULL, L"Cannot allocate enough memory", L"Error", MB_OK );
      return false;
   }

   ULONG result = ::GetAdaptersInfo( pAdapterInfo, &size );
   if ( result == ERROR_BUFFER_OVERFLOW )
   {
      free( pAdapterInfo );
      pAdapterInfo = (PIP_ADAPTER_INFO) malloc( size );

      if ( pAdapterInfo == NULL )
      {
         MessageBox( NULL, L"Cannot allocate enough memory", L"Error", MB_OK );
         return false;
      }

      result = ::GetAdaptersInfo( pAdapterInfo, &size );
   }

   bool limited = false;
   int count = 0;
   PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
   while ( pAdapter != NULL )
   {
      std::string ipAddress = pAdapter->IpAddressList.IpAddress.String;
      if ( ipAddress.compare( "0.0.0.0" ) != 0 )
      {
         if ( ++count == 3 )
         {
            limited = true;
            break;
         }
      }
      pAdapter = pAdapter->Next;
   }

   pAdapter = pAdapterInfo;
   while ( pAdapter != NULL )
   {
      std::string ipAddress = pAdapter->IpAddressList.IpAddress.String;
      if ( ipAddress.compare( "0.0.0.0" ) != 0 )
      {
         // TODO special case for 10. is a bit too hard-coded
         if ( !limited || ipAddress.substr(0,3) == "10." )
         {
            buffer << pAdapter->Description << ":" << pAdapter->IpAddressList.IpAddress.String << std::endl;
         }
      }
      pAdapter = pAdapter->Next;
   }

   free( pAdapterInfo );

   std::string s = buffer.str().c_str();
   std::wstring wsTmp( s.begin(), s.end() );

   wcsncpy_s( string, length, wsTmp.c_str(), _TRUNCATE );
   string[ length - 1 ] = '\0';
   return true;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass( HINSTANCE hInstance )
{
   WNDCLASSEXW wcex;

   wcex.cbSize = sizeof( WNDCLASSEX );

   wcex.style = CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc = WndProc;
   wcex.cbClsExtra = 0;
   wcex.cbWndExtra = 0;
   wcex.hInstance = hInstance;
   wcex.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_QUICKINFOTRAY ) );
   wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
   wcex.hbrBackground = (HBRUSH) ( COLOR_WINDOW + 1 );
   wcex.lpszMenuName = MAKEINTRESOURCEW( IDC_QUICKINFOTRAY );
   wcex.lpszClassName = szWindowClass;
   wcex.hIconSm = LoadIcon( wcex.hInstance, MAKEINTRESOURCE( IDI_SMALL ) );

   return RegisterClassExW( &wcex );
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance( HINSTANCE hInstance, int nCmdShow )
{
   UNREFERENCED_PARAMETER( nCmdShow );

   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW( szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr );

   if ( !hWnd )
   {
      return FALSE;
   }

   // We don't need or want to show the main window, it just exists to support messaging
   ShowWindow( hWnd, SW_HIDE );// nCmdShow );
   UpdateWindow( hWnd );

   return TRUE;
}

BOOL AddNotificationIcon( HWND hWnd )
{
   NOTIFYICONDATA nid = { sizeof( nid ) };
   nid.hWnd = hWnd;
   // add the icon, setting the icon, tooltip, and callback message.
   // the icon will be identified with the GUID
   nid.uFlags = NIF_GUID | NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
   nid.guidItem = __uuidof( QuickInfoIcon );
   nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
   nid.hIcon = ::LoadIcon( hInst, MAKEINTRESOURCE( IDI_QUICKINFOTRAY ) );
   LoadString( hInst, IDS_TOOLTIP, nid.szTip, ARRAYSIZE( nid.szTip ) );

   Shell_NotifyIcon( NIM_ADD, &nid );

   // NOTIFYICON_VERSION_4 is prefered
   nid.uVersion = NOTIFYICON_VERSION_4;
   return Shell_NotifyIcon( NIM_SETVERSION, &nid );
}

BOOL DeleteNotificationIcon()
{
   NOTIFYICONDATA nid = { sizeof( nid ) };
   nid.uFlags = NIF_GUID;
   nid.guidItem = __uuidof( QuickInfoIcon );
   return Shell_NotifyIcon( NIM_DELETE, &nid );
}

BOOL ShowQuickInfo()
{
   NOTIFYICONDATA nid = { sizeof( nid ) };
   nid.uFlags = NIF_GUID | NIF_INFO;
   nid.guidItem = __uuidof( QuickInfoIcon );
   //nid.dwInfoFlags = NIIF_INFO;
   LoadString( hInst, IDS_QUICKINFO_TITLE, nid.szInfoTitle, ARRAYSIZE( nid.szInfoTitle ) );
   //LoadString( hInst, IDS_CONNECTIONS, nid.szInfo, ARRAYSIZE( nid.szInfo ) );
   PopulateInfo( nid.szInfo, ARRAYSIZE( nid.szInfo ) );
   return Shell_NotifyIcon( NIM_MODIFY, &nid );
}

BOOL HideQuickInfo()
{
   // After the balloon is dismissed, restore the tooltip.
   NOTIFYICONDATA nid = { sizeof( nid ) };
   nid.uFlags = NIF_GUID | NIF_SHOWTIP;
   nid.guidItem = __uuidof( QuickInfoIcon );
   return Shell_NotifyIcon( NIM_MODIFY, &nid );
}

void PositionFlyout( HWND hwnd, REFGUID guidIcon )
{
   // find the position of our printer icon
   NOTIFYICONIDENTIFIER nii = { sizeof( nii ) };
   nii.guidItem = guidIcon;
   RECT rcIcon;
   HRESULT hr = Shell_NotifyIconGetRect( &nii, &rcIcon );
   if ( SUCCEEDED( hr ) )
   {
      // display the flyout in an appropriate position close to the printer icon
      POINT const ptAnchor = { ( rcIcon.left + rcIcon.right ) / 2, ( rcIcon.top + rcIcon.bottom ) / 2 };

      RECT rcWindow;
      GetWindowRect( hwnd, &rcWindow );
      SIZE sizeWindow = { rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top };

      if ( CalculatePopupWindowPosition( &ptAnchor, &sizeWindow, TPM_VERTICAL | TPM_VCENTERALIGN | TPM_CENTERALIGN | TPM_WORKAREA, &rcIcon, &rcWindow ) )
      {
         // position the flyout and make it the foreground window
         SetWindowPos( hwnd, HWND_TOPMOST, rcWindow.left, rcWindow.top, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW );
      }
   }
}

BOOL CALLBACK DlgProc( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
   switch ( message )
   {
      case WM_INITDIALOG:
         // Populate the dialog

         return TRUE;

      case WM_COMMAND:
         switch ( LOWORD( wParam ) )
         {
            case IDOK:
               return TRUE;

            case IDCANCEL:
               DestroyWindow( hwndDlg );
               return TRUE;
         }
   }
   return FALSE;
}
HWND ShowFlyout( HWND hwndMainWindow )
{
   HWND hwnd = CreateDialog( hInst, MAKEINTRESOURCE( IDD_INFOPOPUP ), hwndMainWindow, (DLGPROC) DlgProc );

   NOTIFYICONIDENTIFIER nii = { sizeof( nii ) };
   nii.guidItem = __uuidof( QuickInfoIcon );
   RECT rcIcon;
   HRESULT hr = Shell_NotifyIconGetRect( &nii, &rcIcon );
   if ( SUCCEEDED( hr ) )
   {
      // display the flyout in an appropriate position close to the printer icon
      POINT const ptAnchor = { ( rcIcon.left + rcIcon.right ) / 2, ( rcIcon.top + rcIcon.bottom ) / 2 };

      RECT rcWindow;
      GetWindowRect( hwnd, &rcWindow );
      SIZE sizeWindow = { rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top };

      if ( CalculatePopupWindowPosition( &ptAnchor, &sizeWindow, TPM_VERTICAL | TPM_VCENTERALIGN | TPM_CENTERALIGN | TPM_WORKAREA, &rcIcon, &rcWindow ) )
      {
         // position the flyout and make it the foreground window
         SetWindowPos( hwnd, HWND_TOPMOST, rcWindow.left, rcWindow.top, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW );
      }
   }

   ShowWindow( hwnd, SW_SHOW );
   return hwnd;
}

void HideFlyout( HWND hwndMainWindow, HWND hwndFlyout )
{
   if ( hwndFlyout == NULL )
      return;

   DestroyWindow( hwndFlyout );
}

void ShowContextMenu( HWND hwnd, POINT pt )
{
   HMENU hMenu = LoadMenu( hInst, MAKEINTRESOURCE( IDC_CONTEXTMENU ) );
   if ( hMenu )
   {
      HMENU hSubMenu = GetSubMenu( hMenu, 0 );
      if ( hSubMenu )
      {
         // our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
         SetForegroundWindow( hwnd );

         // respect menu drop alignment
         UINT uFlags = TPM_RIGHTBUTTON;
         if ( GetSystemMetrics( SM_MENUDROPALIGNMENT ) != 0 )
         {
            uFlags |= TPM_RIGHTALIGN;
         }
         else
         {
            uFlags |= TPM_LEFTALIGN;
         }

         TrackPopupMenuEx( hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL );
      }
      DestroyMenu( hMenu );
   }
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
   static HWND s_hwndFlyout = NULL;

   switch ( message )
   {
      case WM_CREATE:
      {
         AddNotificationIcon( hWnd );
         break;
      }
      case WMAPP_HIDEFLYOUT:
      {
         HideFlyout( hWnd, s_hwndFlyout );
         s_hwndFlyout = NULL;
         break;
      }
      case WMAPP_NOTIFYCALLBACK:
      {
         switch( LOWORD( lParam ) )
         {
            case NIN_SELECT:
            {
               if ( s_hwndFlyout == NULL )
               {
                  s_hwndFlyout = ShowFlyout( hWnd );
               }
               //ShowQuickInfo();
               break;
            }
            case NIN_BALLOONTIMEOUT:
            {
               //HideQuickInfo();
               break;
            }
            case NIN_BALLOONUSERCLICK:
            {
               HideFlyout( hWnd, s_hwndFlyout );
               s_hwndFlyout = NULL;
               //HideQuickInfo();
               break;
            }
            case WM_CONTEXTMENU:
            {
               POINT const point = { LOWORD( wParam ), HIWORD( wParam ) };
               ShowContextMenu( hWnd, point );
               break;
            }
         }
         break;
      }
      case WM_COMMAND:
      {
         int wmId = LOWORD( wParam );
         // Parse the menu selections:
         switch ( wmId )
         {
            case IDM_ABOUT:
               DialogBox( hInst, MAKEINTRESOURCE( IDD_ABOUTBOX ), hWnd, About );
               break;
            case IDM_EXIT:
               DestroyWindow( hWnd );
               break;
            default:
               return DefWindowProc( hWnd, message, wParam, lParam );
         }
      }
      break;
      case WM_DESTROY:
      {
         DeleteNotificationIcon();
         PostQuitMessage( 0 );
         break;
      }
      default:
      {
         return DefWindowProc( hWnd, message, wParam, lParam );
      }
   }
   return 0;
}

BOOL SetCopyright(HWND hDlg)
{
   TCHAR szExeFileName[ MAX_PATH ];
   GetModuleFileName( NULL, szExeFileName, MAX_PATH );

   DWORD dwHandle;
   DWORD dwSize = GetFileVersionInfoSize( szExeFileName, &dwHandle );
   LPBYTE data = new BYTE[ dwSize ];
   if ( GetFileVersionInfo( szExeFileName, 0, dwSize, data ) )
   {
      LPBYTE buffer;
      UINT length;
      if ( VerQueryValue( data, L"\\", (LPVOID*) &buffer, &length ) )
      {
         VS_FIXEDFILEINFO* fixedFileInfo = (VS_FIXEDFILEINFO*) buffer;
         if ( fixedFileInfo->dwSignature == 0xFEEF04BD )
         {
            // Can get basic version info here
         }

         LANGANDCODEPAGE* lpTranslate;
         UINT cbTranslate;
         VerQueryValue( data,
                        TEXT( "\\VarFileInfo\\Translation" ),
                        (LPVOID*) &lpTranslate,
                        &cbTranslate );

         // Read the file description for each language and code page.

         for ( unsigned int i = 0; i < ( cbTranslate / sizeof( struct LANGANDCODEPAGE ) ); i++ )
         {
            WCHAR subBlock[ 50 ];
            std::wstringstream identifier;
            if ( StringCchPrintf( subBlock, 50,
                                  TEXT( "\\StringFileInfo\\%04x%04x\\FileDescription" ),
                                  lpTranslate[ i ].wLanguage,
                                  lpTranslate[ i ].wCodePage ) == 0 )
            {
               LPVOID lpBuffer;
               UINT dwBytes;
               // Retrieve file description for language and code page "i". 
               VerQueryValue( data,
                              subBlock,
                              &lpBuffer,
                              &dwBytes );

               HWND hCtrl = GetDlgItem( hDlg, IDC_IDENTIFIER );
               if ( hCtrl != NULL )
               {
                  identifier << (LPWSTR) lpBuffer;
               }
            }
            if ( StringCchPrintf( subBlock, 50,
                                  TEXT( "\\StringFileInfo\\%04x%04x\\FileVersion" ),
                                  lpTranslate[ i ].wLanguage,
                                  lpTranslate[ i ].wCodePage ) == 0 )
            {
               LPVOID lpBuffer;
               UINT dwBytes;
               // Retrieve file description for language and code page "i". 
               VerQueryValue( data,
                              subBlock,
                              &lpBuffer,
                              &dwBytes );

               HWND hCtrl = GetDlgItem( hDlg, IDC_IDENTIFIER );
               if ( hCtrl != NULL )
               {
                  identifier << L" " << (LPWSTR) lpBuffer;
                  SendMessage( hCtrl, WM_SETTEXT, 0, (LPARAM) identifier.str().c_str() );
               }
            }
            if( StringCchPrintf( subBlock, 50,
                                 TEXT( "\\StringFileInfo\\%04x%04x\\LegalCopyright" ),
                                 lpTranslate[ i ].wLanguage,
                                 lpTranslate[ i ].wCodePage ) == 0 )
            {
               LPVOID lpBuffer;
               UINT dwBytes;
               // Retrieve file description for language and code page "i". 
               VerQueryValue( data,
                              subBlock,
                              &lpBuffer,
                              &dwBytes );

               HWND hCtrl = GetDlgItem( hDlg, IDC_COPYRIGHT );
               if ( hCtrl != NULL )
               {
                  SendMessage( hCtrl, WM_SETTEXT, 0, (LPARAM) lpBuffer );
               }
            }
         }
      }
   }

   return TRUE;
}

// Message handler for about box.
INT_PTR CALLBACK About( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
   UNREFERENCED_PARAMETER( lParam );
   switch ( message )
   {
      case WM_INITDIALOG:
      {
         SetCopyright( hDlg );
         return (INT_PTR) TRUE;
      }

      case WM_COMMAND:
         if ( LOWORD( wParam ) == IDOK || LOWORD( wParam ) == IDCANCEL )
         {
            EndDialog( hDlg, LOWORD( wParam ) );
            return (INT_PTR) TRUE;
         }
         break;
   }
   return (INT_PTR) FALSE;
}
