// 98VideoConverter: Converts videos into .98v format, suitable for use with 98VIDEOP.COM
// Maxim Hoxha 2022-2023
// This is the main application root
// This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)

#include "98VideoConverter.h"
#include "MainWindow.h"

static const char* aboutMessage =
"98VideoConverter, Version 0.5.0\n\
Copyright (C) Maxim Hoxha 2022-2023\n\
This software uses libraries from the FFmpeg project under the LGPL v2.1 and GPL v2\n\
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.\n\
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\n\
You should have received a copy of the GNU General Public License along with this program.  If not, see https://www.gnu.org/licenses/.";

//All GUI construction is coded into the binary because this is a small and simple (conceptually) program.

static const char* uiXML =
"<interface>"
    "<menu id='menubar'>"
        "<submenu>"
            "<attribute name='label' translatable='yes'>_File</attribute>"
            "<section>"
                "<item>"
                    "<attribute name='label' translatable='yes'>_Open...</attribute>"
                    "<atrribute name='action'>file.open</attribute>"
                "</item>"
                "<item>"
                    "<attribute name='label' translatable='yes'>_Export...</attribute>"
                    "<atrribute name='action'>file.export</attribute>"
                "</item>"
            "</section>"
            "<section>"
                "<item>"
                    "<attribute name='label' translatable='yes'>_Quit</attribute>"
                    "<atrribute name='action'>file.quit</attribute>"
                "</item>"
            "</section>"
        "</submenu>"
        "<submenu>"
            "<attribute name='label' translatable='yes'>_Edit</attribute>"
            "<section>"
                "<item>"
                    "<attribute name='label' translatable='yes'>_Output Settings...</attribute>"
                    "<atrribute name='action'>edit.settings</attribute>"
                "</item>"
                "<item>"
                    "<attribute name='label' translatable='yes'>_Palette...</attribute>"
                    "<atrribute name='action'>edit.palette</attribute>"
                "</item>"
            "</section>"
        "</submenu>"
        "<submenu>"
            "<attribute name='label' translatable='yes'>_Help</attribute>"
            "<section>"
                "<item>"
                    "<attribute name='label' translatable='yes'>_About...</attribute>"
                    "<atrribute name='action'>help.about</attribute>"
                "</item>"
            "</section>"
        "</submenu>"
    "</menu>"
"</interface>";

VideoConverter98::VideoConverter98() : Gtk::Application()
{
    Glib::set_application_name("98VideoConverter");
    conv = new VideoConverterEngine();
}

Glib::RefPtr<VideoConverter98> VideoConverter98::create()
{
    return Glib::RefPtr<VideoConverter98>(new VideoConverter98());
}

void VideoConverter98::on_startup()
{
    Gtk::Application::on_startup();

    add_action("file.open", sigc::mem_fun(*this, &VideoConverter98::OnMenuFileOpen));
    add_action("file.export", sigc::mem_fun(*this, &VideoConverter98::OnMenuFileExport));
    add_action("file.quit", sigc::mem_fun(*this, &VideoConverter98::OnMenuFileQuit));
    add_action("edit.settings", sigc::mem_fun(*this, &VideoConverter98::OnMenuEditSettings));
    add_action("edit.palette", sigc::mem_fun(*this, &VideoConverter98::OnMenuEditPalette));
    add_action("help.about", sigc::mem_fun(*this, &VideoConverter98::OnMenuHelpAbout));

    builderRef = Gtk::Builder::create();
    builderRef->add_from_string(uiXML);

    Glib::RefPtr<Glib::Object> uiobj = builderRef->get_object("menubar");
    Glib::RefPtr<Gio::Menu> menubar = Glib::RefPtr<Gio::Menu>::cast_dynamic(uiobj);
    set_menubar(menubar);
}

void VideoConverter98::on_activate()
{
    CreateWindow();
}

void VideoConverter98::CreateWindow()
{
    MainWindow* mwin = new MainWindow();
    add_window(*mwin);
    mwin->signal_hide().connect(sigc::bind<Gtk::Window*>(sigc::mem_fun(*this, &VideoConverter98::OnHideWindow), mwin));
    mwin->show_all();
}

void VideoConverter98::OnHideWindow(Gtk::Window* window)
{
    delete window;
}

void VideoConverter98::OnMenuFileOpen()
{

}

void VideoConverter98::OnMenuFileExport()
{
    
}

void VideoConverter98::OnMenuFileQuit()
{
    delete conv;
    quit();
    std::vector<Gtk::Window*> wins = get_windows();
    for (int i = 0; i < wins.size(); i++)
    {
        wins[i]->hide();
    }
}

void VideoConverter98::OnMenuEditSettings()
{
    
}

void VideoConverter98::OnMenuEditPalette()
{
    
}

void VideoConverter98::OnMenuHelpAbout()
{
    
}

int main (int argc, char* argv[])
{
    Glib::RefPtr<VideoConverter98> vconv98 = VideoConverter98::create();
    return vconv98->run(argc, argv);
}
/*/ //Former Win32 API stuffs
// Global Variables:
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
Gdiplus::Bitmap* inputbmp;
Gdiplus::Bitmap* previewbmp;
HBITMAP oinputbmp;
HBITMAP opreviewbmp;
Gdiplus::Rect scrrect = Gdiplus::Rect(0, 0, 640, 400);
HWND seekframetext;
std::wstring_convert<std::codecvt<wchar_t, char, mbstate_t>, wchar_t>* strconv = new std::wstring_convert<std::codecvt<wchar_t, char, mbstate_t>, wchar_t>();
bool isinsaving;
bool inQuit;
bool repaintPreview;

// Forward declarations of functions included in this code module:
bool ProgressReport(UINT32 frame);

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
    {L"All supported video files", L"*.avi;*.gif;*.mkv;*.mov;*.mp4;*.mpeg;*.mpg;*.wmv;*.webm"} //Can be extended to any that FFmpeg supports, but I would need to know the file extensions first
};

const COMDLG_FILTERSPEC saveFileTypes[] =
{
    {L"98V File (.98v)", L"*.98v"}
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
                                                previewbmp = new Gdiplus::Bitmap(640, 400, 640 * 4, PixelFormat32bppRGB, conv->GrabFrame(0));
                                                inputbmp = new Gdiplus::Bitmap(640, 400, 640 * 4, PixelFormat32bppRGB, conv->GetRescaledInputFrame());
                                                oinputbmp = NULL;
                                                opreviewbmp = NULL;
                                                inputbmp->GetHBITMAP(Gdiplus::Color(0, 0, 0), &oinputbmp);
                                                previewbmp->GetHBITMAP(Gdiplus::Color(0, 0, 0), &opreviewbmp);
                                                SendDlgItemMessageW(workForm, IDC_INPUTIMAGE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)oinputbmp);
                                                SendDlgItemMessageW(workForm, IDC_PREVIEWIMAGE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)opreviewbmp);
                                                SendDlgItemMessageW(workForm, IDC_SEEKBAR, TBM_SETRANGEMAX, TRUE, conv->GetOrigFrameNumber() - 1);
                                                SendDlgItemMessageW(workForm, IDC_SEEKBAR, TBM_SETPOS, TRUE, 0);
                                                curframepos = 0;
                                                Static_SetText(seekframetext, L"0\0");
                                                SetDlgItemInt(workForm, IDC_BITRATEEDIT, conv->GetBitrate()*2, FALSE);
                                                SetDlgItemInt(workForm, IDC_DITHERFACEDIT, (UINT)(conv->GetDitherFactor()*1000.0f), FALSE);
                                                SetDlgItemInt(workForm, IDC_SATDITHERFACEDIT, (UINT)(conv->GetSaturationDitherFactor() * 1000.0f), FALSE);
                                                SetDlgItemInt(workForm, IDC_HUEDITHERFACEDIT, (UINT)(conv->GetHueDitherFactor() * 1000.0f), FALSE);
                                                SetDlgItemInt(workForm, IDC_UVBEDIT, (UINT)(conv->GetUVBias() * 1000.0f), FALSE);
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
            case ID_EDIT_SWITCHSETTINGS:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_SWITCHBOARD), hWnd, SwitchboardProc);
                break;
            case ID_EDIT_PALETTE:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_PALETTE), hWnd, PaletteEditProc);
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
    previewbmp->LockBits(&scrrect, Gdiplus::ImageLockMode::ImageLockModeWrite, PixelFormat32bppRGB, bmpdat);
    memcpy(bmpdat->Scan0, conv->GetSimulatedOutput(), 640 * 400 * 4);
    previewbmp->UnlockBits(bmpdat);
    DeleteBitmap(opreviewbmp);
    previewbmp->GetHBITMAP(Gdiplus::Color(0, 0, 0), &opreviewbmp);
    inputbmp->LockBits(&scrrect, Gdiplus::ImageLockMode::ImageLockModeWrite, PixelFormat32bppRGB, bmpdat);
    memcpy(bmpdat->Scan0, conv->GetRescaledInputFrame(), 640 * 400 * 4);
    inputbmp->UnlockBits(bmpdat);
    DeleteBitmap(oinputbmp);
    inputbmp->GetHBITMAP(Gdiplus::Color(0, 0, 0), &oinputbmp);
    SendDlgItemMessageW(workForm, IDC_INPUTIMAGE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)oinputbmp);
    SendDlgItemMessageW(workForm, IDC_PREVIEWIMAGE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)opreviewbmp);
    SendDlgItemMessageW(workForm, IDC_SEEKBAR, TBM_SETPOS, TRUE, frame);
    UpdateWindow(mainWindow);
    RedrawWindow(mainWindow, NULL, NULL, RDW_INTERNALPAINT);
    GetMessageW(&dummy, NULL, 0, 0); //Kludge to stop Windows from thinking this program is not responding
    TranslateMessage(&dummy);
    DispatchMessageW(&dummy);
    delete bmpdat;
    return inQuit;
}

LRESULT CALLBACK FormProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
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
            previewbmp->LockBits(&scrrect, Gdiplus::ImageLockMode::ImageLockModeWrite, PixelFormat32bppRGB, &bmpdat);
            memcpy(bmpdat.Scan0, conv->GrabFrame(newframepos), 640 * 400 * 4);
            previewbmp->UnlockBits(&bmpdat);
            DeleteBitmap(opreviewbmp);
            previewbmp->GetHBITMAP(Gdiplus::Color(0, 0, 0), &opreviewbmp);
            inputbmp->LockBits(&scrrect, Gdiplus::ImageLockMode::ImageLockModeWrite, PixelFormat32bppRGB, &bmpdat);
            memcpy(bmpdat.Scan0, conv->GetRescaledInputFrame(), 640 * 400 * 4);
            inputbmp->UnlockBits(&bmpdat);
            DeleteBitmap(oinputbmp);
            inputbmp->GetHBITMAP(Gdiplus::Color(0, 0, 0), &oinputbmp);
            SendDlgItemMessageW(workForm, IDC_INPUTIMAGE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)oinputbmp);
            SendDlgItemMessageW(workForm, IDC_PREVIEWIMAGE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)opreviewbmp);
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
                if (conv != nullptr)
                {
                    conv->SetBitrate(GetDlgItemInt(hWnd, IDC_BITRATEEDIT, NULL, FALSE)/2);
                }
            }
            break;
        case IDC_DITHERFACEDIT:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                if (conv != nullptr)
                {
                    conv->SetDitherFactor(((float)GetDlgItemInt(hWnd, IDC_DITHERFACEDIT, NULL, FALSE)) / 1000.0f);
                    repaintPreview = true;
                }
            }
            break;
        case IDC_SATDITHERFACEDIT:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                if (conv != nullptr)
                {
                    conv->SetSaturationDitherFactor(((float)GetDlgItemInt(hWnd, IDC_SATDITHERFACEDIT, NULL, FALSE)) / 1000.0f);
                    repaintPreview = true;
                }
            }
            break;
        case IDC_HUEDITHERFACEDIT:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                if (conv != nullptr)
                {
                    conv->SetHueDitherFactor(((float)GetDlgItemInt(hWnd, IDC_HUEDITHERFACEDIT, NULL, FALSE)) / 1000.0f);
                    repaintPreview = true;
                }
            }
            break;
        case IDC_UVBEDIT:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                if (conv != nullptr)
                {
                    conv->SetUVBias(((float)GetDlgItemInt(hWnd, IDC_UVBEDIT, NULL, FALSE)) / 1000.0f);
                    repaintPreview = true;
                }
            }
            break;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

int samplerateradios[] = { IDC_SAMPLERATE_RADIO_0, IDC_SAMPLERATE_RADIO_1, IDC_SAMPLERATE_RADIO_2, IDC_SAMPLERATE_RADIO_3,
                           IDC_SAMPLERATE_RADIO_4, IDC_SAMPLERATE_RADIO_5, IDC_SAMPLERATE_RADIO_6, IDC_SAMPLERATE_RADIO_7 };

int framerateradios[] = { IDC_FRAMERATE_RADIO_0, IDC_FRAMERATE_RADIO_1, IDC_FRAMERATE_RADIO_2, IDC_FRAMERATE_RADIO_3 };

LRESULT CALLBACK SwitchboardProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        if (conv != nullptr)
        {
            CheckRadioButton(hWnd, IDC_SAMPLERATE_RADIO_0, IDC_SAMPLERATE_RADIO_7, samplerateradios[conv->GetSampleRateSpec()]);
            CheckRadioButton(hWnd, IDC_AUDIOCHANNELS_RADIO_0, IDC_AUDIOCHANNELS_RADIO_1, conv->GetIsStereo() ? IDC_AUDIOCHANNELS_RADIO_1 : IDC_AUDIOCHANNELS_RADIO_0);
            CheckRadioButton(hWnd, IDC_RESOLUTION_RADIO_0, IDC_RESOLUTION_RADIO_1, conv->GetIsHalfVerticalResolution() ? IDC_RESOLUTION_RADIO_1 : IDC_RESOLUTION_RADIO_0);
            CheckRadioButton(hWnd, IDC_FRAMERATE_RADIO_0, IDC_FRAMERATE_RADIO_3, framerateradios[conv->GetFrameSkip()]);
        }
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDCANCEL:
            if (conv != nullptr)
            {
                for (int i = 0; i < 8; i++)
                {
                    if (IsDlgButtonChecked(hWnd, samplerateradios[i]))
                    {
                        conv->SetSampleRateSpec(i);
                    }
                }
                if (IsDlgButtonChecked(hWnd, IDC_RESOLUTION_RADIO_0))
                {
                    conv->SetIsHalfVerticalResolution(false);
                }
                else if (IsDlgButtonChecked(hWnd, IDC_RESOLUTION_RADIO_1))
                {
                    conv->SetIsHalfVerticalResolution(true);
                }
                if (IsDlgButtonChecked(hWnd, IDC_AUDIOCHANNELS_RADIO_0))
                {
                    conv->SetIsStereo(false);
                }
                else if (IsDlgButtonChecked(hWnd, IDC_AUDIOCHANNELS_RADIO_1))
                {
                    conv->SetIsStereo(true);
                }
                for (int i = 0; i < 3; i++)
                {
                    if (IsDlgButtonChecked(hWnd, framerateradios[i]))
                    {
                        conv->SetFrameSkip(i);
                    }
                }
            }
            repaintPreview = true;
            EndDialog(hWnd, LOWORD(wParam));
            InvalidateRect(workForm, NULL, TRUE);
            UpdateWindow(workForm);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK PaletteEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDOK:
            repaintPreview = true;
            EndDialog(hWnd, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDCANCEL:
            EndDialog(hWnd, LOWORD(wParam));
            return (INT_PTR)TRUE;
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
//*/