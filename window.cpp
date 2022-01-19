#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "commctrl.h"

#include <cassert>
#include <cmath>
#include <system_error>

#include "print.h"
#include "window.h"
#include "resource.h"

namespace
{

HWND                theWindow   {};
HWND                theDialog   {};

constexpr int       WM_REFRESH  {WM_APP};
constexpr auto      windowStyle { WS_OVERLAPPED | WS_CAPTION  |  WS_SYSMENU  | WS_MINIMIZEBOX | WS_VISIBLE    };

std::atomic_bool    done{false};

constexpr int       bandWidth{6};
constexpr int       height{100};
constexpr int       width{256*bandWidth};


RGBQUAD fromHSL(double H, double S, double L)
{
    assert( H>= 0 && H <= 1);
    assert( S>= 0 && S <= 1);
    assert( L>= 0 && L <= 1);

    auto chroma = (1 - std::abs(2 * L - 1)) * S;
    auto Hprime = H*6;
    auto m      = L - chroma/2;

    auto X = chroma * (1 - std::abs (std::fmod(Hprime,2)  -1 ))  ;

    RGBQUAD rgb{};

    auto scale = [&](double d)
    {
        return static_cast<BYTE>(256*(d+m));
    };

    if(Hprime < 1)
    {
        rgb.rgbRed   = scale(chroma);
        rgb.rgbGreen = scale(X); 
        rgb.rgbBlue  = scale(0); 
    }
    else if(Hprime < 2)
    {
        rgb.rgbRed   = scale(X); 
        rgb.rgbGreen = scale(chroma);
        rgb.rgbBlue  = scale(0); 
    }
    else if(Hprime < 3)
    {
        rgb.rgbRed   = scale(0); 
        rgb.rgbGreen = scale(chroma);
        rgb.rgbBlue  = scale(X); 
    }
    else if(Hprime < 4)
    {
        rgb.rgbRed   = scale(0); 
        rgb.rgbGreen = scale(X); 
        rgb.rgbBlue  = scale(chroma);
    }
    else if(Hprime < 5)
    {
        rgb.rgbRed   = scale(X); 
        rgb.rgbGreen = scale(0); 
        rgb.rgbBlue  = scale(chroma);
    }
    else 
    {
        rgb.rgbRed   = scale(chroma);
        rgb.rgbGreen = scale(0); 
        rgb.rgbBlue  = scale(X);
    }

    return rgb;
}

RGBQUAD fromHSV(double H, double S, double V)
{
    assert( H>= 0 && H <= 1);
    assert( S>= 0 && S <= 1);
    assert( V>= 0 && V <= 1);

    auto chroma = V * S;
    auto Hprime = H*6;
    auto m      = V - chroma;

    auto X = chroma * (1 - std::abs (std::fmod(Hprime,2)  -1 ))  ;

    RGBQUAD rgb{};

    auto scale = [&](double d)
    {
        return static_cast<BYTE>(256*(d+m));
    };

    if(Hprime < 1)
    {
        rgb.rgbRed   = scale(chroma);
        rgb.rgbGreen = scale(X); 
        rgb.rgbBlue  = scale(0); 
    }
    else if(Hprime < 2)
    {
        rgb.rgbRed   = scale(X); 
        rgb.rgbGreen = scale(chroma);
        rgb.rgbBlue  = scale(0); 
    }
    else if(Hprime < 3)
    {
        rgb.rgbRed   = scale(0); 
        rgb.rgbGreen = scale(chroma);
        rgb.rgbBlue  = scale(X); 
    }
    else if(Hprime < 4)
    {
        rgb.rgbRed   = scale(0); 
        rgb.rgbGreen = scale(X); 
        rgb.rgbBlue  = scale(chroma);
    }
    else if(Hprime < 5)
    {
        rgb.rgbRed   = scale(X); 
        rgb.rgbGreen = scale(0); 
        rgb.rgbBlue  = scale(chroma);
    }
    else 
    {
        rgb.rgbRed   = scale(chroma);
        rgb.rgbGreen = scale(0); 
        rgb.rgbBlue  = scale(X);
    }

    return rgb;
}



auto makeHeader()
{
    auto const size = sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD);

    auto header = reinterpret_cast<BITMAPINFO*>( new  std::byte[size]);

    memset(header,0,size);

    header->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    header->bmiHeader.biWidth           =  width;
    header->bmiHeader.biHeight          = -height;
    header->bmiHeader.biPlanes          =    1;
    header->bmiHeader.biBitCount        =    8;
    header->bmiHeader.biCompression     = BI_RGB;
    header->bmiHeader.biSizeImage       = width*height,
    header->bmiHeader.biXPelsPerMeter   = 0;
    header->bmiHeader.biYPelsPerMeter   = 0;
    header->bmiHeader.biClrUsed         = 0;
    header->bmiHeader.biClrImportant    = 0;

    return header;
}



BITMAPINFO     *bitmapHeader        {makeHeader()};
uint8_t         bitmapData[height][width]{};



void paint(HWND h,WPARAM w, LPARAM l)
{
    PAINTSTRUCT paint;
    BeginPaint(h,&paint);

    RECT  r{};
    GetClientRect(h,&r);
    
    StretchDIBits(paint.hdc,
                  0,0,
                  r.right-r.left,
                  r.bottom-r.top,
                  0,0,
                  width,height,
                  bitmapData,
                  bitmapHeader,
                  DIB_RGB_COLORS,
                  SRCCOPY);

    
    EndPaint(h,&paint);
}

LRESULT CALLBACK windowProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch(m)
    {
    case WM_CREATE:
    {        
        RECT    client{0,0, width, height};

        AdjustWindowRect(&client,windowStyle,FALSE);
        SetWindowPos(h, nullptr,0,0, client.right-client.left, client.bottom-client.top, SWP_NOMOVE|SWP_NOZORDER);
        GetClientRect(h,&client);
        SetForegroundWindow(h);

        return 0;
    }

    case WM_CLOSE:
        done=true;
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        paint(h,w,l);
        return 0;

    case WM_REFRESH:
        InvalidateRect(h,nullptr,FALSE);
        return 0;
    
    case WM_NCHITTEST:
    case WM_MOUSEMOVE:
    case WM_NCMOUSEMOVE:
    case WM_SETCURSOR:
        break;

    default:
        //print("msg {:#x}\n",m);
        break;
    }

    return DefWindowProc(h,m,w,l);
}

void palette(double saturation, double other, auto converter)
{
    for(int i=0;i<256;i++)
    {
        bitmapHeader->bmiColors[i]= converter(i/256.0,saturation,other);
    }

    PostMessage(theWindow,WM_REFRESH,0,0);
}


void palette(HWND h)
{
    auto saturation = SendDlgItemMessage(h,IDC_SATURATION,TBM_GETPOS,0,0);
    auto lightness  = SendDlgItemMessage(h,IDC_LIGHTNESS, TBM_GETPOS,0,0);

    if(IsDlgButtonChecked(h,IDC_HSL))
    {
        SetDlgItemText(h,IDC_LORV,"Lightness");
        palette(saturation/101.0, lightness/101.0,fromHSL);
    }
    else
    {
        SetDlgItemText(h,IDC_LORV,"Brightness");
        palette(saturation/101.0, lightness/101.0,fromHSV);
    }

}

INT_PTR dialogProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch(m)
    {
    case WM_COMMAND:

        switch(LOWORD(w))
        {
        case IDCANCEL:
            PostQuitMessage(0);
            EndDialog(h,0);
            break;

        case IDC_HSV:
        case IDC_HSL:
            palette(h);
        }
        break;

    case WM_INITDIALOG:
        ShowWindow(h,SW_SHOW);
        CheckRadioButton(h,IDC_HSL, IDC_HSV,IDC_HSL);
        return false;

    case WM_NOTIFY:
    case WM_SETCURSOR:
    case WM_NCHITTEST:
    case WM_MOUSEMOVE:
    case WM_CTLCOLORSTATIC:
        break;

    case WM_HSCROLL:
    {
        palette(h);
        break;        
    }



    default:
        print("msg {:#x}\n",m);
        break;
    }

    return 0;
}

}

void createWindow()
{
    WNDCLASSA    Class
    {
        CS_OWNDC,
        windowProc,
        0,
        0,
        GetModuleHandle(nullptr),
        nullptr,
        LoadCursor(nullptr,IDC_ARROW),
        nullptr,
        nullptr,
        "bitmapWindowClass"
    };

    if(!RegisterClassA(&Class))
    {
        throw std::system_error{ static_cast<int>(GetLastError()), std::system_category(), "RegisterClass"};
    }

    theWindow = CreateWindowA(Class.lpszClassName,
                              "Palette",
                              windowStyle,
                              CW_USEDEFAULT,CW_USEDEFAULT,
                              10,10,
                              nullptr,
                              nullptr,
                              GetModuleHandle(nullptr),
                              nullptr);

    if(theWindow==nullptr)
    {
        throw std::system_error{ static_cast<int>(GetLastError()), std::system_category(), "RegisterClass"};
    }
}


void createDialog()
{
    theDialog=CreateDialog(nullptr,MAKEINTRESOURCE(IDD_DIALOG),nullptr,dialogProc);
}


void windowMessageLoop()
{
    MSG     msg;
    while(GetMessage(&msg,0,0,0) > 0)
    {
        if(!IsDialogMessage(theDialog, &msg))
        {
            DispatchMessage(&msg);
        }
    }
}



int main()
{
    for(int row=0;row<height;row++)
    {
        for(int column=0;column<width;column++)
        {
            bitmapData[row][column]=column/bandWidth;
        }
    }

    createWindow();
    createDialog();
    windowMessageLoop();
}