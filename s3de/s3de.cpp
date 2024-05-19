#include "framework.h"
#include "s3de.h"
#include <commctrl.h>
#include <stdio.h>
#include "timer.h"
#include "cmh3d/world.h"
#include "cmh3d/matrix.h"
#include "cmh3d/3d.h"
#include "cmh3d/3ds.h"
#include "cmh3d/shade.h"
#include "commctrl.h"
#include <stdint.h>
#include <windowsx.h>

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
HWND ghwnd, hwndSB, hwndTB, hwndV;
HDC hdc;
world* gworld;
int x0, y0, x1, y1;
int oldx1, oldy1;
COLORREF color = RGB(0,0,255);
int red, green, blue=255;
RECT gr;
int gvieww, gviewh;
float xRot=0.0, yRot=0.0, zRot=0.0, xTran=0.0, yTran=0.0, zTran=0.0;
uint32_t* pbuf;
HWND hwndSettings;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
void ProcessInput();
void Render();
static HWND CreateToolBar(HWND);
static HWND CreateStatusBar(HWND);
static HWND CreateView(HWND);
LRESULT CALLBACK settings_wndproc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void ppixel(int x, int y, int r, int g, int b);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
  _In_opt_ HINSTANCE hPrevInstance,
  _In_ LPWSTR lpCmdLine,
  _In_ int nCmdShow) {

  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  LoadStringW(hInstance, IDC_S3DE, szWindowClass, MAX_LOADSTRING);

  WNDCLASSEXW wcex;
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_S3DE));
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_S3DE);
  wcex.lpszClassName = szWindowClass;
  wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
  if(!RegisterClassExW(&wcex)) return FALSE;

  hInst = hInstance;
  ghwnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
  if(!ghwnd) return FALSE;

  ShowWindow(ghwnd, nCmdShow);
  UpdateWindow(ghwnd);

  HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_S3DE));

  MSG msg;

  for(;;) {
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if(!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
        if(!IsWindow(hwndSettings) || !IsDialogMessage(hwndSettings, &msg)) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
      }
    }
    if(msg.message == WM_QUIT) break;
    ProcessInput();
    Render();
  }
  return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  RECT rs, rt;
  static CHOOSECOLOR cc;
  static COLORREF custom[16] = { RGB(0,0,0) };
  char filename[512] = "";
  int rtHeight, sbHeight, x, y, width, height;
  OPENFILENAME ofn;
  char sz[256];

  switch(message) {
  case WM_CREATE:
    hwndSB = CreateStatusBar(hWnd);
    hwndTB = CreateToolBar(hWnd);
    hwndV = CreateView(hWnd);
    GetClientRect(hWnd, &gr);
    GetWindowRect(hwndSB, &rs);
    SetScreenW(gr.right);
    SetScreenH(gr.bottom - (rs.bottom - rs.top));
    Setppixel(ppixel);
    gworld = InitializeWorld(0);
    break;
  case WM_SIZE:
    SendMessage(hwndSB, WM_SIZE, wParam, lParam);
    SendMessage(hwndTB, WM_SIZE, wParam, lParam);
    GetClientRect(hWnd, &gr);
    GetWindowRect(hwndSB, &rs);
    GetWindowRect(hwndTB, &rt);
    rtHeight = rt.bottom - rt.top - 2;
    sbHeight = rs.bottom - rs.top - 1;
    x = 0;
    y = rtHeight;
    width = gr.right - gr.left;
    height = gr.bottom - gr.top - rtHeight - sbHeight - 1;
    SetWindowPos(hwndV, HWND_TOP, x, y, width, height, SWP_SHOWWINDOW);
    SendMessage(hwndV, WM_SIZE, wParam, lParam);
    SetScreenW(width);
    SetScreenH(height);
    if(pbuf) free(pbuf);
    pbuf = (uint32_t*)malloc(sizeof(uint32_t) * width * height);
    gvieww = width;
    gviewh = height;
    break;
  case WM_COMMAND:
    {
      int wmId = LOWORD(wParam);
      switch(wmId) {
      case IDM_ABOUT:
        DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
        break;
      case IDM_EXIT:
        DestroyWindow(hWnd);
        break;
      case ID_FILE_OPEN:
        FreeWorld(gworld);

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = sz;
        ofn.lpstrFile[0] = '\0';
        ofn.nMaxFile = sizeof(sz);
        ofn.lpstrFilter = "3ds (*.3ds)\0*.3ds\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrDefExt = "3ds";
        ofn.Flags = 0;
        if(!GetOpenFileName(&ofn)) return 0;

        gworld = InitializeWorld(sz);
        GetClientRect(hWnd, &gr);
        GetWindowRect(hwndSB, &rs);
        GetWindowRect(hwndTB, &rt);
        rtHeight = rt.bottom - rt.top - 2;
        sbHeight = rs.bottom - rs.top - 1;
        x = 0;
        y = rtHeight;
        width = gr.right - gr.left;
        height = gr.bottom - gr.top - rtHeight - sbHeight - 1;
        SetScreenW(width);
        SetScreenH(height);
        break;
      case ID_OPTIONS_SETTINGS:
        if(!IsWindow(hwndSettings)) {
          hwndSettings = CreateDialog(hInst, (LPCTSTR)IDD_SETTINGS, hWnd, (DLGPROC)settings_wndproc);
          ShowWindow(hwndSettings, SW_SHOW);
        }
        break;
      case ID_BUTTON_VERTEX:
        SetCam(0);
        break;
      case ID_BUTTON_WIRE:
        SetCam(1);
        break;
      case ID_BUTTON_FLAT:
        SetCam(2);
        break;
      default:
        return DefWindowProc(hWnd, message, wParam, lParam);
      }
    }
    break;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC dc = BeginPaint(hWnd, &ps);
      EndPaint(hWnd, &ps);
    }
    break;
  case WM_KEYDOWN:
    if(wParam == VK_OEM_MINUS) zTran = 1.0f;
    else if(wParam == VK_OEM_PLUS) zTran = -1.0f;
    else if(wParam == VK_UP) yTran += 1.0f;
    else if(wParam == VK_DOWN) yTran -= 1.0f;
    else if(wParam == VK_LEFT) xTran -= 1.0f;
    else if(wParam == VK_RIGHT) xTran += 1.0f;
    break;
  case WM_DESTROY:
    DestroyWindow(hwndSB);
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProcW(hWnd, message, wParam, lParam);
  }
  return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  UNREFERENCED_PARAMETER(lParam);
  switch(message) {
  case WM_INITDIALOG:
    return (INT_PTR)TRUE;
  case WM_COMMAND:
    if(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
      EndDialog(hDlg, LOWORD(wParam));
      return (INT_PTR)TRUE;
    }
    break;
  }
  return (INT_PTR)FALSE;
}

HWND CreateStatusBar(HWND hwndParent) {
  HWND hwndSB;
  int a[7] = { 100, 200, 300, 400, 500, 600, 700 };

  hwndSB = CreateWindow(STATUSCLASSNAME,
    NULL,
    WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | SBARS_SIZEGRIP,
    0, 0, 0, 0,
    hwndParent,
    0,
    hInst, NULL);
  SendMessage(hwndSB, SB_SETPARTS, 7, (LPARAM)a);
  return hwndSB;
}

HWND CreateToolBar(HWND hWndParent) {
  int numButtons = 3;
  int bitmapSize = 16;
  LRESULT lr;

  HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
    WS_CHILD | TBSTYLE_WRAPABLE,
    0, 0, 0, 0,
    hWndParent, NULL, hInst, NULL);

  if(hWndToolbar == NULL)
    return NULL;

  // Create the image list.
  HIMAGELIST hil = ImageList_Create(bitmapSize, bitmapSize, ILC_COLOR16, numButtons, 0);
  lr = SendMessage(hWndToolbar, TB_SETIMAGELIST, 0, (LPARAM)hil);

  // Load the button images.
  TBADDBITMAP tb = { hInst, IDB_BITMAP1 };
  lr = SendMessage(hWndToolbar, TB_ADDBITMAP, numButtons, (LPARAM)&tb);
  TBBUTTON tbButtons[] =
  { { 0, ID_BUTTON_VERTEX, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)"cam0" },
    { 1, ID_BUTTON_WIRE, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)"cam1" },
    { 2, ID_BUTTON_FLAT, TBSTATE_ENABLED, BTNS_AUTOSIZE, {0}, 0, (INT_PTR)"cam2" } };
  lr = SendMessage(hWndToolbar, TB_SETMAXTEXTROWS, 0, 0);
  lr = SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
  lr = SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)numButtons, (LPARAM)&tbButtons);

  lr = SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
  BOOL b = ShowWindow(hWndToolbar, TRUE);

  return hWndToolbar;
}

LRESULT CALLBACK view_wndproc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  PAINTSTRUCT ps;
  HDC hdc;
  static RECT rv, rc, rr, rs;
  static int rrHeight, sbHeight, x, y, width, height;
  static BOOLEAN bFirstPaint = TRUE;
  static int mw=0, mx0=0, mx1=0, my0=0, my1=0;

  switch(message) {
  case WM_CREATE:
    break;
  case WM_DESTROY:
    break;
  case WM_PAINT:
    hdc = BeginPaint(hWnd, &ps);
    EndPaint(hWnd, &ps);
    break;
  case WM_SIZE:
    break;
  case WM_MOUSEWHEEL:
    mw = GET_WHEEL_DELTA_WPARAM(wParam);
    zTran += mw < 0 ? 1.0f : -1.0f;
    break;
  case WM_MBUTTONDOWN:
    mx0 = GET_X_LPARAM(lParam);
    my0 = GET_Y_LPARAM(lParam);
    SetCapture(hwndV);
    break;
  case WM_MBUTTONUP:
    ReleaseCapture();
    break;
  case WM_MOUSEMOVE:
    if(wParam == MK_MBUTTON) {
      mx1 = GET_X_LPARAM(lParam);
      my1 = GET_Y_LPARAM(lParam);
      yRot += 0.1f*(mx0-mx1);
      xRot += 0.1f*(my0-my1);
      mx0 = mx1;
      my0 = my1;
    }
    break;
  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
    break;
  }
  return 0;
}

HWND CreateView(HWND hWndParent) {
  WNDCLASSEX wcex;
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.lpszClassName = "view";
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = view_wndproc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInst;
  wcex.hIcon = NULL;
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = NULL;
  wcex.lpszMenuName = NULL;
  wcex.hIconSm = NULL;
  RegisterClassEx(&wcex);
  HWND hwnd = CreateWindowEx(0, "view", "",
    WS_CHILD | WS_VISIBLE,
    0, 0, 0, 0,
    hWndParent, NULL,
    hInst, NULL);

  LRESULT lr = SendMessage(hwnd, WM_SIZE, 0, 0);

  return hwnd;
}

LRESULT CALLBACK settings_wndproc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  int st;
  char sb[512];

  switch(message) {
  case WM_INITDIALOG:
    st = GetShadeState();
    if(st & SHADE_VERTEX) CheckDlgButton(hDlg, IDC_SHADE_VERTEX, BST_CHECKED);
    if(st & SHADE_WIREFRAME) CheckDlgButton(hDlg, IDC_SHADE_WIREFRAME, BST_CHECKED);
    if(st & SHADE_FLAT) CheckDlgButton(hDlg, IDC_SHADE_FLAT, BST_CHECKED);
    if(st & SHADE_GOURAUD) CheckDlgButton(hDlg, IDC_SHADE_GOURAUD, BST_CHECKED);
    if(st & SHADE_PHONG) CheckDlgButton(hDlg, IDC_SHADE_PHONG, BST_CHECKED);
    if(st & SHADE_TEXTURE) CheckDlgButton(hDlg, IDC_TEXTURE, BST_CHECKED);
    if(st & SHADE_BILINEAR) CheckDlgButton(hDlg, IDC_BFILTER, BST_CHECKED);
    if(st & SHADE_VNORMAL) CheckDlgButton(hDlg, IDC_VNORMALS, BST_CHECKED);
    if(st & SHADE_TNORMAL) CheckDlgButton(hDlg, IDC_TNORMALS, BST_CHECKED);
    sprintf_s(sb, 512, "%f", GetVNLen());
    SetDlgItemText(hDlg, IDC_LVNORMAL, sb);
    sprintf_s(sb, 512, "%f", GetTNLen());
    SetDlgItemText(hDlg, IDC_LTNORMAL, sb);
    break;
  case WM_CLOSE:
    DestroyWindow(hDlg);
    hwndSettings = 0;
    break;
  case WM_COMMAND:
    switch(LOWORD(wParam)) {
    case IDC_SHADE_VERTEX:
      st = GetShadeState();
      st &= ~(SHADE_WIREFRAME | SHADE_FLAT | SHADE_GOURAUD | SHADE_PHONG | SHADE_SPECULAR);
      SetShadeState(st | SHADE_VERTEX);
      break;
    case IDC_SHADE_WIREFRAME:
      st = GetShadeState();
      st &= ~(SHADE_VERTEX | SHADE_FLAT | SHADE_GOURAUD | SHADE_PHONG | SHADE_SPECULAR);
      SetShadeState(st | SHADE_WIREFRAME);
      break;
    case IDC_SHADE_FLAT:
      st = GetShadeState();
      st &= ~(SHADE_VERTEX | SHADE_WIREFRAME | SHADE_GOURAUD | SHADE_PHONG | SHADE_SPECULAR);
      SetShadeState(st | SHADE_FLAT);
      break;
    case IDC_SHADE_GOURAUD:
      st = GetShadeState();
      st &= ~(SHADE_VERTEX | SHADE_WIREFRAME | SHADE_FLAT | SHADE_PHONG | SHADE_SPECULAR);
      SetShadeState(st | SHADE_GOURAUD);
      break;
    case IDC_SHADE_PHONG:
      st = GetShadeState();
      st &= ~(SHADE_VERTEX | SHADE_WIREFRAME | SHADE_FLAT | SHADE_GOURAUD);
      SetShadeState(st | SHADE_PHONG | SHADE_SPECULAR);
      break;
    case IDC_TEXTURE:
      SetShadeState(GetShadeState() ^ SHADE_TEXTURE);
      break;
    case IDC_BFILTER:
      SetShadeState(GetShadeState() ^ SHADE_BILINEAR);
      break;
    case IDC_VNORMALS:
      SetShadeState(GetShadeState() ^ SHADE_VNORMAL);
      break;
    case IDC_TNORMALS:
      SetShadeState(GetShadeState() ^ SHADE_TNORMAL);
      break;
    case IDC_LVNORMAL:
      switch(HIWORD(wParam)) {
      case EN_CHANGE:
        GetDlgItemText(hDlg, IDC_LVNORMAL, sb, 512);
        SetVNLen((float)atof(sb));
        break;
      }
      break;
    case IDC_LTNORMAL:
      switch(HIWORD(wParam)) {
      case EN_CHANGE:
        GetDlgItemText(hDlg, IDC_LTNORMAL, sb, 512);
        SetTNLen((float)atof(sb));
        break;
      }
      break;
    }
    break;
  }
  return 0;
}

void ppixel(int x, int y, int r, int g, int b) {
  /* draw to buffer */
  uint32_t c = r << 16 | g << 8 | b;
  pbuf[y * gvieww + x] = c;

  /* draw to dc */
  //SetPixel(hdc, x, gviewh-y, RGB(r, g, b));
}

void Render() {
  RECT r;
  HDC dc;
  HBITMAP bm, bmo;
  char sbt[512];
  float tt;
  static float ts = 0.0f;
  int w, h;
  int ss = GetShadeState();

  timer_start();

  /* draw to buffer */
  GetWindowRect(hwndV, &r);
  dc = GetDC(hwndV);
  hdc = CreateCompatibleDC(dc);
  bm = CreateCompatibleBitmap(dc, r.right, r.bottom);
  bmo = (HBITMAP)SelectObject(hdc, bm);
  w = r.right - r.left;
  h = r.bottom - r.top;
  memset(pbuf, 0x666666, sizeof(uint32_t) * w * h);
  DrawScene(gworld);
  BITMAPINFOHEADER bi = { sizeof(bi), w, h, 1, 32, BI_RGB };
  SetDIBitsToDevice(hdc, 0, 0, w, h, 0, 0, 0, h, pbuf,
    (BITMAPINFO*)&bi, DIB_RGB_COLORS);
  BitBlt(dc, 0, 0, r.right, r.bottom, hdc, 0, 0, SRCCOPY);
  SelectObject(hdc, bmo);
  DeleteDC(hdc);
  ReleaseDC(hwndV, dc);
  DeleteObject(bm);

  /* draw to dc */
  //hdc = GetDC(hwndV);
  //GetWindowRect(hwndV, &r);
  //r.right -= r.left;
  //r.bottom -= r.top;
  //r.left = 0;
  //r.top = 0;
  //FillRect(hdc, &r, (HBRUSH)GetStockObject(GRAY_BRUSH));
  //DrawScene(gworld);
  //ReleaseDC(hwndV, hdc);

  tt = timer_stop();
  ts += tt;
  if(ts > 1000000) {
    sprintf_s(sbt, 512, "%dx%d", GetScreenW(), GetScreenH());
    SendMessage(hwndSB, SB_SETTEXT, MAKELONG(0, 0), (LPARAM)sbt);
    sprintf_s(sbt, 512, "fps: %d", (int)(1000000.0 / tt));
    SendMessage(hwndSB, SB_SETTEXT, MAKELONG(1, 0), (LPARAM)sbt);
    sprintf_s(sbt, 512, "vertices: %d", GetVCount());
    SendMessage(hwndSB, SB_SETTEXT, MAKELONG(2, 0), (LPARAM)sbt);
    sprintf_s(sbt, 512, "triangles: %d", GetTCount());
    SendMessage(hwndSB, SB_SETTEXT, MAKELONG(3, 0), (LPARAM)sbt);
    sprintf_s(sbt, 512, "clipv: %d", GetCVCount());
    SendMessage(hwndSB, SB_SETTEXT, MAKELONG(4, 0), (LPARAM)sbt);
    sprintf_s(sbt, 512, "clipt: %d", GetCTCount());
    SendMessage(hwndSB, SB_SETTEXT, MAKELONG(5, 0), (LPARAM)sbt);
    sprintf_s(sbt, 512, "cullt: %d", GetCullTCount());
    SendMessage(hwndSB, SB_SETTEXT, MAKELONG(6, 0), (LPARAM)sbt);
    ts = 0.0f;
  }
  return;
}

void ProcessInput() {
  camera* pcam = &gworld->cameraList[gworld->currentCamIndex];
  static matrix4x4 rMatrix;
  static matrix4x4 tMatrix;

  GetRotateMatrix(rMatrix, xRot, yRot, zRot);
  GetTranslateMatrix(tMatrix, xTran, yTran, zTran);
  SetViewLookAtMatrix(pcam);
  Reset(pcam);

  //view space
  ApplyMatrix(pcam, rMatrix);
  ApplyMatrix(pcam, tMatrix);

  //world space
  ApplyMatrix(pcam, pcam->viewLookAtMatrix);
  
  xRot = 0.0;
  yRot = 0.0;
  zRot = 0.0;
  xTran = 0.0;
  yTran = 0.0;
  zTran = 0.0;
}
