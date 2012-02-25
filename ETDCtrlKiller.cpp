#include "stdafx.h"
#include "ETDCtrlKiller.h"

#define MAX_LOADSTRING 100
#define WM_NOTIFYICON (WM_USER + 1)

HINSTANCE hInst;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];
TCHAR szETDCtrlPath[1024];
DWORD dwShowMessage = 1;
HANDLE hETDCtrl = INVALID_HANDLE_VALUE;
UINT_PTR nStopETDCtrlTimer = 1;
UINT uETDCtrlLifeTime = 60 * 1000;

void				LoadPreferences();
void				SavePreferences();
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
BOOL				InitNotifyIcon(HWND);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Preferences(HWND, UINT, WPARAM, LPARAM);
void				StartETDCtrl(HWND);
void				StopETDCtrl(HWND);
void				ShowMenu(HWND);
void				ShowMessage(HWND, LPCTSTR);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;

	LoadPreferences();

	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_ETDCTRLKILLER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ETDCTRLKILLER));

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

void LoadPreferences()
{
	HKEY hKey;
	DWORD dwType = REG_SZ;
	DWORD dwLen = sizeof(szETDCtrlPath);

	if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\shugo\\ETDCtrlKiller"),
			0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS) {
		return;
	}
	if (RegQueryValueEx(hKey, _T("ETDCtrlPath"), NULL, &dwType, (LPBYTE) szETDCtrlPath, &dwLen)
		!= ERROR_SUCCESS) {
		lstrcpy(szETDCtrlPath, _T("C:\\Program Files\\Elantech\\ETDCtrl.exe"));
	}
	dwType = REG_DWORD;
	dwLen = sizeof(dwShowMessage);
	if (RegQueryValueEx(hKey, _T("ShowMessage"), NULL, &dwType, (LPBYTE) &dwShowMessage, &dwLen)
		!= ERROR_SUCCESS) {
		dwShowMessage = 1;
	}
	RegCloseKey(hKey);
}

void SavePreferences()
{
	HKEY hKey;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\shugo\\ETDCtrlKiller"),
			0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS) {
		return;
	}
	RegSetValueEx(hKey, _T("ETDCtrlPath"), 0, REG_SZ, (CONST BYTE *) szETDCtrlPath, (lstrlen(szETDCtrlPath) + 1) * sizeof(TCHAR));
	RegSetValueEx(hKey, _T("ShowMessage"), 0, REG_DWORD, (CONST BYTE *) &dwShowMessage, sizeof(dwShowMessage));
	RegCloseKey(hKey);
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ETDCTRLKILLER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance;

   hWnd = CreateWindowEx(WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
	   szWindowClass, szTitle, WS_MINIMIZE,
	   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   if (!InitNotifyIcon(hWnd)) {
	   return FALSE;
   }
   StartETDCtrl(hWnd);
   return TRUE;
}

BOOL InitNotifyIcon(HWND hWnd)
{
	NOTIFYICONDATA nid;

	ZeroMemory(&nid, sizeof(nid));
	nid.cbSize = sizeof(nid);
	nid.hWnd = hWnd;
	nid.uID = 0;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_NOTIFYICON;
	nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));
	lstrcpy(nid.szTip, _T("ETDCtrlKiller"));
	for (int i = 0; i < 18; i++) {
		if (Shell_NotifyIcon(NIM_ADD, &nid)) return TRUE;
		if (GetLastError() != ERROR_TIMEOUT) return FALSE;
		Sleep(10000);
		if (Shell_NotifyIcon(NIM_MODIFY, &nid)) return TRUE;
	}
	return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	static UINT s_uTaskbarRestart;

	switch (message)
	{
	case WM_CREATE:
		s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_PREFERENCES:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_PREFERENCES), hWnd, Preferences);
			break;
		case IDM_STARTETDCTRL:
			if (hETDCtrl == INVALID_HANDLE_VALUE) {
				StartETDCtrl(hWnd);
			}
			else {
				MessageBox(NULL, _T("ETDCtrl.exe is already running"), _T("Error"), MB_OK);
			}
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		{
			NOTIFYICONDATA nid;

			StopETDCtrl(hWnd);
			ZeroMemory(&nid, sizeof(nid));
			nid.cbSize = sizeof(nid);
			nid.hWnd = hWnd;
			Shell_NotifyIcon(NIM_DELETE, &nid);
			PostQuitMessage(0);
		}
		break;
	case WM_POWERBROADCAST:
		if (wParam == PBT_APMRESUMEAUTOMATIC) {
			StartETDCtrl(hWnd);
		}
		break;
	case WM_TIMER:
		if (wParam == nStopETDCtrlTimer) {
			StopETDCtrl(hWnd);
		}
		break;
	case WM_NOTIFYICON:
		switch (lParam) {
		case WM_LBUTTONDOWN:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_PREFERENCES), hWnd, Preferences);
			break;
		case WM_RBUTTONDOWN:
			ShowMenu(hWnd);
			break;
		}
		break;
	default:
		if (message == s_uTaskbarRestart) {
			InitNotifyIcon(hWnd);
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Preferences(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_PATHEDIT, szETDCtrlPath);
		CheckDlgButton(hDlg, IDC_SHOWMSGCHECKBOX, dwShowMessage ? BST_CHECKED : BST_UNCHECKED);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			if (LOWORD(wParam) == IDOK) {
				GetDlgItemText(hDlg, IDC_PATHEDIT, szETDCtrlPath, sizeof(szETDCtrlPath) / sizeof(TCHAR));
				dwShowMessage = IsDlgButtonChecked(hDlg, IDC_SHOWMSGCHECKBOX) == BST_CHECKED;
				SavePreferences();
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void StartETDCtrl(HWND hWnd)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	if (hETDCtrl == INVALID_HANDLE_VALUE) {
		if (!CreateProcess(szETDCtrlPath, NULL, NULL, NULL, FALSE,
				NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi)) {
			MessageBox(NULL, _T("Failed to start ETDCtrl.exe"), _T("Error"), MB_OK);
			return;
		}
		CloseHandle(pi.hThread);
		hETDCtrl = pi.hProcess;
		ShowMessage(hWnd, _T("Started ETDCtrl.exe"));
	}
	SetTimer(hWnd, nStopETDCtrlTimer, uETDCtrlLifeTime, NULL);
}

void StopETDCtrl(HWND hWnd)
{
	KillTimer(hWnd, nStopETDCtrlTimer);
	if (hETDCtrl != INVALID_HANDLE_VALUE) {
		TerminateProcess(hETDCtrl, 0);
		CloseHandle(hETDCtrl);
		hETDCtrl = INVALID_HANDLE_VALUE;
		ShowMessage(hWnd, _T("Terminated ETDCtrl.exe"));
	}
}

void ShowMenu(HWND hWnd)
{
    HMENU hMenu, hSubMenu;
    POINT pt;

    hMenu = LoadMenu(hInst, (LPCTSTR) IDC_ETDCTRLKILLER);
    hSubMenu = GetSubMenu(hMenu, 0);
    GetCursorPos(&pt);
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hSubMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}

void ShowMessage(HWND hWnd, LPCTSTR message)
{
	NOTIFYICONDATA nid;

	if (!dwShowMessage) return;
	ZeroMemory(&nid, sizeof(nid));
	nid.cbSize = sizeof(nid);
	nid.hWnd = hWnd;
	nid.uFlags = NIF_INFO;
	nid.dwInfoFlags = NIIF_INFO;
	lstrcpy(nid.szInfoTitle, _T("ETDCtrlKiller"));
	lstrcpy(nid.szInfo, message);
	Shell_NotifyIcon(NIM_MODIFY, &nid);
}

// vim: set sw=4 ts=4 noexpandtab :
