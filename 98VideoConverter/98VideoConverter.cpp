// 98VideoConverter: Converts videos into .98v format, suitable for use with 98VIDEOP.COM
// Maxim Hoxha 2022
// This is just Windows API bullshit
// This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)

#include "framework.h"
#include "98VideoConverter.h"
#include "ConverterEngine.h"
#include <shlwapi.h>
#include <shobjidl_core.h>
#include <wingdi.h>
#include <gdiplus.h>
#include <windowsx.h>
#include <charconv>
#include <string>
#include <locale>
#include <cstdlib>


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND mainWindow;
HWND workForm;
HDC hDc;
const BITMAPINFOHEADER stddisplaybmpinfo = { 640 * 400 * 3,
                                    640,
                                    -400,
                                    1,
                                    24,
                                    BI_RGB,
                                    640 * 400 * 3,
                                    1000,
                                    1000,
                                    0,
                                    0};
RGBQUAD colourtable[1] = { {0,0,0,0} };
int curframepos = 0;
Gdiplus::Bitmap* bmp;
HBITMAP obmp;
Gdiplus::Rect scrrect = Gdiplus::Rect(0, 0, 640, 400);
HWND seekframetext;
std::wstring_convert<std::codecvt<wchar_t, char, mbstate_t>, wchar_t>* strconv = new std::wstring_convert<std::codecvt<wchar_t, char, mbstate_t>, wchar_t>();
bool isinsaving;
bool inQuit;
bool repaintPreview;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    FormProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK       resizeForm(HWND hwndChild, LPARAM lParam);
BOOL CALLBACK       FindSlider(HWND hWnd, LPARAM lParam);
bool ProgressReport(UINT32 frame);

VideoConverterEngine* conv;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    Gdiplus::GdiplusStartupInput si;
    ULONG_PTR gdit;
    hDc = CreateCompatibleDC(NULL);
    Gdiplus::GdiplusStartup(&gdit, &si, NULL);
    // TODO: Place code here.
    isinsaving = false;
    inQuit = false;
    repaintPreview = false;
    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY98VIDEOCONVERTER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY98VIDEOCONVERTER));

    MSG msg;

    conv = new VideoConverterEngine();

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    Gdiplus::GdiplusShutdown(gdit);
    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY98VIDEOCONVERTER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MY98VIDEOCONVERTER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
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
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   mainWindow = hWnd;
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   workForm = CreateDialogW(hInst, MAKEINTRESOURCE(IDD_FORMVIEW), hWnd, FormProc);
   EnumChildWindows(workForm, FindSlider, NULL);

   return TRUE;
}

BOOL CALLBACK FindSlider(HWND hWnd, LPARAM lParam)
{
    int idChild = GetWindowLong(hWnd, GWL_ID);
    if (idChild == IDC_SEEKFRAME)
    {
        seekframetext = hWnd;
    }
    return TRUE;
}

const COMDLG_FILTERSPEC openFileTypes[] =
{
    {L"All supported video files", L"*.avi;*.mp4;*.mkv;*.mov"}
};

const COMDLG_FILTERSPEC saveFileTypes[] =
{
    {L"98V File (.98v)", L"*.98v"}
};

class OpenDialogEventHandler : public IFileDialogEvents
{
public:
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        if (riid == IID_IFileDialogEvents)
        {
            *ppv = this;
        }
        else return ResultFromScode(E_NOINTERFACE);
        AddRef();
        return NULL;
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cref);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        long cRef = InterlockedDecrement(&_cref);
        if (!cRef)
            delete this;
        return cRef;
    }

    IFACEMETHODIMP OnFileOk(IFileDialog*) { return S_OK; };
    IFACEMETHODIMP OnFolderChange(IFileDialog*) { return S_OK; };
    IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) { return S_OK; };
    IFACEMETHODIMP OnHelp(IFileDialog*) { return S_OK; };
    IFACEMETHODIMP OnSelectionChange(IFileDialog*) { return S_OK; };
    IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; };
    IFACEMETHODIMP OnTypeChange(IFileDialog* pfd) { return S_OK; };
    IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) { return S_OK; };

    OpenDialogEventHandler() : _cref(1) {};
private:
    ~OpenDialogEventHandler() {};
    long _cref;
};

class SaveDialogEventHandler : public IFileDialogEvents
{
public:
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        if (riid == IID_IFileDialogEvents)
        {
            *ppv = this;
        }
        else return ResultFromScode(E_NOINTERFACE);
        AddRef();
        return NULL;
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cref);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        long cRef = InterlockedDecrement(&_cref);
        if (!cRef)
            delete this;
        return cRef;
    }

    IFACEMETHODIMP OnFileOk(IFileDialog*) { return S_OK; };
    IFACEMETHODIMP OnFolderChange(IFileDialog*) { return S_OK; };
    IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) { return S_OK; };
    IFACEMETHODIMP OnHelp(IFileDialog*) { return S_OK; };
    IFACEMETHODIMP OnSelectionChange(IFileDialog*) { return S_OK; };
    IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; };
    IFACEMETHODIMP OnTypeChange(IFileDialog* pfd) { return S_OK; };
    IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) { return S_OK; };

    SaveDialogEventHandler() : _cref(1) {};
private:
    ~SaveDialogEventHandler() {};
    long _cref;
};

void TryOpenFile(HWND hWnd) //Just look at how many safety checks there are!
{
    IFileOpenDialog* dialog = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (SUCCEEDED(hr))
    {
        IFileDialogEvents* dEvents = NULL;
        OpenDialogEventHandler* oevents = new OpenDialogEventHandler();
        hr = oevents->QueryInterface(IID_PPV_ARGS(&dEvents));
        if (SUCCEEDED(hr))
        {
            DWORD dwCookie;
            hr = dialog->Advise(dEvents, &dwCookie);
            if (SUCCEEDED(hr))
            {
                DWORD dFlags;
                hr = dialog->GetOptions(&dFlags);
                if (SUCCEEDED(hr))
                {
                    hr = dialog->SetOptions(dFlags & (!FOS_ALLOWMULTISELECT));
                    if (SUCCEEDED(hr))
                    {
                        hr = dialog->SetFileTypes(ARRAYSIZE(openFileTypes), openFileTypes);
                        if (SUCCEEDED(hr))
                        {
                            hr = dialog->SetFileTypeIndex(0);
                            if (SUCCEEDED(hr))
                            {
                                hr = dialog->SetDefaultExtension(L"avi");
                                if (SUCCEEDED(hr))
                                {
                                    hr = dialog->Show(NULL);
                                    if (SUCCEEDED(hr))
                                    {
                                        IShellItem* outItem;
                                        hr = dialog->GetResult(&outItem);
                                        if (SUCCEEDED(hr))
                                        {
                                            PWSTR filePath = NULL;
                                            hr = outItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
                                            if (SUCCEEDED(hr))
                                            {
                                                conv->OpenForDecodeVideo(filePath);
                                                bmp = new Gdiplus::Bitmap(640, 400, 640 * 4, PixelFormat32bppRGB, conv->GrabFrame(0));
                                                obmp = NULL;
                                                bmp->GetHBITMAP(Gdiplus::Color(0, 0, 0), &obmp);
                                                SendDlgItemMessageW(workForm, IDC_PREVIEWIMAGE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)obmp);
                                                SendDlgItemMessageW(workForm, IDC_SEEKBAR, TBM_SETRANGEMAX, TRUE, conv->GetOrigFrameNumber() - 1);
                                                SendDlgItemMessageW(workForm, IDC_SEEKBAR, TBM_SETPOS, TRUE, 0);
                                                curframepos = 0;
                                                Static_SetText(seekframetext, L"0\0");
                                            }
                                            outItem->Release();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                dialog->Unadvise(dwCookie);
            }
            oevents->Release();
            dEvents->Release();
        }
        dialog->Release();
    }
}

void TryExportFile(HWND hWnd) //Here too!
{
    IFileSaveDialog* dialog = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (SUCCEEDED(hr))
    {
        IFileDialogEvents* dEvents = NULL;
        SaveDialogEventHandler* sevents = new SaveDialogEventHandler();
        hr = sevents->QueryInterface(IID_PPV_ARGS(&dEvents));
        if (SUCCEEDED(hr))
        {
            DWORD dwCookie;
            hr = dialog->Advise(dEvents, &dwCookie);
            if (SUCCEEDED(hr))
            {
                DWORD dFlags;
                hr = dialog->GetOptions(&dFlags);
                if (SUCCEEDED(hr))
                {
                    hr = dialog->SetOptions(dFlags & (!FOS_ALLOWMULTISELECT));
                    if (SUCCEEDED(hr))
                    {
                        hr = dialog->SetFileTypes(ARRAYSIZE(saveFileTypes), saveFileTypes);
                        if (SUCCEEDED(hr))
                        {
                            hr = dialog->SetFileTypeIndex(0);
                            if (SUCCEEDED(hr))
                            {
                                hr = dialog->SetDefaultExtension(L"98v");
                                if (SUCCEEDED(hr))
                                {
                                    hr = dialog->Show(NULL);
                                    if (SUCCEEDED(hr))
                                    {
                                        IShellItem* outItem;
                                        hr = dialog->GetResult(&outItem);
                                        if (SUCCEEDED(hr))
                                        {
                                            PWSTR filePath = NULL;
                                            hr = outItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
                                            if (SUCCEEDED(hr))
                                            {
                                                isinsaving = true;
                                                conv->EncodeVideo(filePath, ProgressReport);
                                                isinsaving = false;
                                            }
                                            outItem->Release();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                dialog->Unadvise(dwCookie);
            }
            sevents->Release();
            dEvents->Release();
        }
        dialog->Release();
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
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rcClient;
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_OPENFILE:
                TryOpenFile(hWnd);
                break;
            case IDM_EXPORTFILE:
                TryExportFile(hWnd);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_SIZE:
        GetClientRect(hWnd, &rcClient);
        break;
    case WM_DESTROY:
        inQuit = true;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

//Used to provide a rolling preview
bool ProgressReport(UINT32 frame)
{
    std::wstring widetext;
    char numbertext[32];
    MSG dummy;
    Gdiplus::BitmapData* bmpdat = new Gdiplus::BitmapData();
    _itoa_s(frame, numbertext, 32, 10);
    widetext = strconv->from_bytes(numbertext);
    Static_SetText(seekframetext, widetext.c_str());
    curframepos = frame;
    bmp->LockBits(&scrrect, Gdiplus::ImageLockMode::ImageLockModeWrite, PixelFormat32bppRGB, bmpdat);
    memcpy(bmpdat->Scan0, conv->GetSimulatedOutput(), 640 * 400 * 4);
    bmp->UnlockBits(bmpdat);
    DeleteBitmap(obmp);
    bmp->GetHBITMAP(Gdiplus::Color(0, 0, 0), &obmp);
    SendDlgItemMessageW(workForm, IDC_PREVIEWIMAGE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)obmp);
    SendDlgItemMessageW(workForm, IDC_SEEKBAR, TBM_SETPOS, TRUE, frame);
    UpdateWindow(mainWindow);
    RedrawWindow(mainWindow, NULL, NULL, RDW_INTERNALPAINT);
    GetMessageW(&dummy, NULL, 0, 0); //Kludge to stop Windows from thinking this program is not responding
    TranslateMessage(&dummy);
    DispatchMessageW(&dummy);
    delete bmpdat;
    return inQuit;
}

INT_PTR CALLBACK FormProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    std::wstring widetext;
    int newframepos;
    switch (message)
    {
    case WM_PAINT:
        if (isinsaving) break;
        newframepos = SendDlgItemMessageW(hWnd, IDC_SEEKBAR, TBM_GETPOS, NULL, NULL);
        char numbertext[32];
        Gdiplus::BitmapData bmpdat = Gdiplus::BitmapData();
        if (newframepos != curframepos || repaintPreview)
        {
            _itoa_s(newframepos, numbertext, 32, 10);
            widetext = strconv->from_bytes(numbertext);
            Static_SetText(seekframetext, widetext.c_str());
            curframepos = newframepos;
            bmp->LockBits(&scrrect, Gdiplus::ImageLockMode::ImageLockModeWrite, PixelFormat32bppRGB, &bmpdat);
            memcpy(bmpdat.Scan0, conv->GrabFrame(newframepos), 640 * 400 * 4);
            bmp->UnlockBits(&bmpdat);
            DeleteBitmap(obmp);
            bmp->GetHBITMAP(Gdiplus::Color(0, 0, 0), &obmp);
            SendDlgItemMessageW(workForm, IDC_PREVIEWIMAGE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)obmp);
        }
        repaintPreview = false;
        break;
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDC_BITRATEEDIT:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                conv->SetBitrate(GetDlgItemInt(hWnd, IDC_BITRATEEDIT, NULL, FALSE));
            }
            break;
        case IDC_DITHERFACEDIT:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                conv->SetDitherFactor(((float)GetDlgItemInt(hWnd, IDC_DITHERFACEDIT, NULL, FALSE)) / 1000.0f);
                repaintPreview = true;
            }
            break;
        case IDC_SATDITHERFACEDIT:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                conv->SetSaturationDitherFactor(((float)GetDlgItemInt(hWnd, IDC_SATDITHERFACEDIT, NULL, FALSE)) / 1000.0f);
                repaintPreview = true;
            }
            break;
        case IDC_HUEDITHERFACEDIT:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                conv->SetHueDitherFactor(((float)GetDlgItemInt(hWnd, IDC_HUEDITHERFACEDIT, NULL, FALSE)) / 1000.0f);
                repaintPreview = true;
            }
            break;
        case IDC_UVBEDIT:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                conv->SetUVBias(((float)GetDlgItemInt(hWnd, IDC_UVBEDIT, NULL, FALSE)) / 1000.0f);
                repaintPreview = true;
            }
            break;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for about box.
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
