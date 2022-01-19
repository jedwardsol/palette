#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cassert>
#include <thread>
#include <chrono>
using namespace std::literals;
#include <cmath>
#include <system_error>


#include "window.h"


namespace
{

HWND                theWindow   {};
constexpr int       WM_REFRESH  {WM_APP};
constexpr auto      windowStyle { WS_OVERLAPPED | WS_CAPTION  |  WS_SYSMENU  | WS_MINIMIZEBOX | WS_VISIBLE    };

std::atomic_bool    done{false};

constexpr int       bandWidth{8};
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

    if(Hprime < 1)
    {
        rgb.rgbRed   = 256*(chroma + m);
        rgb.rgbGreen = 256*(X + m); 
        rgb.rgbBlue  = 256*m; 
    }
    else if(Hprime < 2)
    {
        rgb.rgbRed   = 256*(X + m); 
        rgb.rgbGreen = 256*(chroma + m);
        rgb.rgbBlue  = 256*m; 
    }
    else if(Hprime < 3)
    {
        rgb.rgbRed   = 256*m; 
        rgb.rgbGreen = 256*(chroma + m);
        rgb.rgbBlue  = 256*(X + m); 
    }
    else if(Hprime < 4)
    {
        rgb.rgbRed   = 256*m; 
        rgb.rgbGreen = 256*(X + m); 
        rgb.rgbBlue  = 256*(chroma + m);
    }
    else if(Hprime < 5)
    {
        rgb.rgbRed   = 256*(X + m); 
        rgb.rgbGreen = 256*m; 
        rgb.rgbBlue  = 256*(chroma + m);
    }
    else 
    {
        rgb.rgbRed   = 256*(chroma + m);
        rgb.rgbGreen = 256*m; 
        rgb.rgbBlue  = 256*(X + m);
    }
    



    return rgb;
}



auto makeHeader()
{
    auto header = reinterpret_cast<BITMAPINFO*>( new  std::byte[ sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD)]);

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

    for(int i=0;i<256;i++)
    {
        header->bmiColors[i]= fromHSL(i/256.0,0.5,0.6);
    }

    return header;
}



BITMAPINFO     *bitmapHeader        {makeHeader()};
uint8_t         bitmapData[height][width]{};





void drawThread()
{
    while(!done)
    {
        PostMessage(theWindow,WM_REFRESH,0,0);
        std::this_thread::sleep_for(10ms);
    }
}



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

LRESULT CALLBACK proc(HWND h, UINT m, WPARAM w, LPARAM l)
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


}

void createWindow()
{
    WNDCLASSA    Class
    {
        CS_OWNDC,
        proc,
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

    std::thread{drawThread}.detach();
}



void windowMessageLoop()
{
    MSG     msg;
    while(GetMessage(&msg,0,0,0) > 0)
    {
        DispatchMessage(&msg);
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
    windowMessageLoop();
}