// QuickInfoTray.cpp : Defines the entry point for the application.
//

#include <string>
#include <sstream>
#include <iostream>
#include "framework.h"
#include "QuickInfoTray.h"

#pragma comment(lib, "IPHLPAPI.lib")

#define MAX_LOADSTRING 100

UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;

// Use a guid to uniquely identify our icon
class __declspec( uuid( "8DCFC718-F9D0-4813-BBB5-AAE0AF07031D" ) ) QuickInfoIcon;

// Global Variables:
HINSTANCE hInst;                                // current instance
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
   LoadStringW( hInstance, IDC_QUICKINFOTRAY, szWindowClass, MAX_LOADSTRING );
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

   PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
   while ( pAdapter != NULL )
   {
      std::string ipAddress = pAdapter->IpAddressList.IpAddress.String;
      if ( ipAddress.compare( "0.0.0.0" ) != 0 )
      {
         buffer << pAdapter->Description << ":" << pAdapter->IpAddressList.IpAddress.String << std::endl;
      }
      pAdapter = pAdapter->Next;
   }

   // Do a reduced version if need be
   if ( buffer.str().length() > length )
   {
      PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
      while ( pAdapter != NULL )
      {
         std::string ipAddress = pAdapter->IpAddressList.IpAddress.String;
         if ( ipAddress.compare( "0.0.0.0" ) != 0 )
         {
            buffer << pAdapter->IpAddressList.IpAddress.String << std::endl;
         }
         pAdapter = pAdapter->Next;
      }
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
   nid.dwInfoFlags = NIIF_INFO;
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
   switch ( message )
   {
      case WM_CREATE:
      {
         AddNotificationIcon( hWnd );
         break;
      }
      case WMAPP_NOTIFYCALLBACK:
      {
         switch( LOWORD( lParam ) )
         {
            case NIN_SELECT:
            {
               ShowQuickInfo();
               break;
            }
            case NIN_BALLOONTIMEOUT:
            {
               HideQuickInfo();
               break;
            }
            case NIN_BALLOONUSERCLICK:
            {
               HideQuickInfo();
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
      /*
      case WM_PAINT:
      {
         PAINTSTRUCT ps;
         HDC hdc = BeginPaint( hWnd, &ps );
         // TODO: Add any drawing code that uses hdc here...
         EndPaint( hWnd, &ps );
         break;
      }
       */
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

// Message handler for about box.
INT_PTR CALLBACK About( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
   UNREFERENCED_PARAMETER( lParam );
   switch ( message )
   {
      case WM_INITDIALOG:
         return (INT_PTR) TRUE;

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
