#include <windows.h>
#include "resource.h"

int idleSeconds = 0;          
bool sc_showed = false;
HINSTANCE g_hInstance = NULL; 

LRESULT CALLBACK ScreenSaverProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static POINT ptSprite = { 100, 100 };
    static int dx = 5, dy = 5;
    static HBRUSH hBrush = NULL;
    static POINT lastMouse = { -1, -1 };

    static HBITMAP hBitmap = NULL;
    static int bmpWidth = 0;
    static int bmpHeight = 0;

    switch (msg)
    {
    case WM_CREATE:
        hBrush = CreateSolidBrush(RGB(0, 0, 0));
        SetTimer(hwnd, 1, 30, NULL);
        GetCursorPos(&lastMouse); 

        hBitmap = (HBITMAP)LoadImage(
            NULL,                  
            L"sprite.bmp",
            IMAGE_BITMAP,
            0, 0,                 
            LR_LOADFROMFILE);

        BITMAP bm;
        GetObject(hBitmap, sizeof(bm), &bm);
        bmpWidth = bm.bmWidth;
        bmpHeight = bm.bmHeight;

        if (!hBitmap) {
            MessageBox(hwnd, L"Не удалось загрузить sprite.bmp", L"Ошибка", MB_OK | MB_ICONERROR);
        }

        return 0;

    case WM_MOUSEMOVE:
    {
        POINT cur;
        GetCursorPos(&cur);
        if (cur.x != lastMouse.x || cur.y != lastMouse.y) {
            sc_showed = false;
            DestroyWindow(hwnd);
        }
        return 0;
    }

    case WM_KEYDOWN:
    case WM_LBUTTONDOWN:
        sc_showed = false;
        DestroyWindow(hwnd);
        return 0;

    case WM_TIMER:
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        ptSprite.x += dx;
        ptSprite.y += dy;

        if (ptSprite.x < 0 || ptSprite.x > rc.right - bmpWidth) dx = -dx;
        if (ptSprite.y < 0 || ptSprite.y > rc.bottom - bmpHeight) dy = -dy;

        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        HDC hMemDC = CreateCompatibleDC(hdc);
        HBITMAP hOld = (HBITMAP)SelectObject(hMemDC, hBitmap);
        BITMAP bm;
        GetObject(hBitmap, sizeof(bm), &bm);

        TransparentBlt(hdc, ptSprite.x, ptSprite.y, bm.bmWidth, bm.bmHeight,
            hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, RGB(0, 0, 0));

        SelectObject(hMemDC, hOld);
        DeleteDC(hMemDC);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        DeleteObject(hBrush);
        sc_showed = false;
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}


void ShowScreensaver()
{
    static bool classRegistered = false;

    if (!classRegistered) {
        WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
        wc.lpfnWndProc = ScreenSaverProc;
        wc.hInstance = g_hInstance;
        wc.lpszClassName = L"ScreenSaverWindow";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

        RegisterClassEx(&wc);
        classRegistered = true;
    }

    HWND hwndSaver = CreateWindowEx(
        WS_EX_TOPMOST,
        L"ScreenSaverWindow",
        L"",
        WS_POPUP,
        0, 0,
        800,
        600,
        NULL, NULL, g_hInstance, NULL
    );

    ShowWindow(hwndSaver, SW_SHOW);
    UpdateWindow(hwndSaver);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        SetTimer(hwnd, 1, 1000, NULL);
        HMENU hMenubar = CreateMenu();
        HMENU hFile = CreateMenu();
        HMENU hEdit = CreateMenu();
        HMENU hHelp = CreateMenu();

        AppendMenu(hFile, MF_STRING, ID_FILE_OPEN, L"Открыть");
        AppendMenu(hFile, MF_STRING, ID_FILE_SAVE, L"Сохранить");
        AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
        AppendMenu(hFile, MF_STRING, ID_FILE_EXIT, L"Выход");

        AppendMenu(hEdit, MF_STRING, ID_EDIT_CUT, L"Вырезать");
        AppendMenu(hEdit, MF_STRING, ID_EDIT_COPY, L"Копировать");
        AppendMenu(hEdit, MF_STRING, ID_EDIT_PASTE, L"Вставить");

        AppendMenu(hHelp, MF_STRING, ID_HELP_ABOUT, L"О программе");

        AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hFile, L"Файл");
        AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hEdit, L"Правка");
        AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hHelp, L"Справка");

        SetMenu(hwnd, hMenubar);
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_FILE_EXIT: PostQuitMessage(0); break;
        case ID_HELP_ABOUT: MessageBox(hwnd, L"Простой редактор", L"О программе", MB_OK); break;
        }
        return 0;

    case WM_MOUSEMOVE:
    case WM_KEYDOWN:
    case WM_LBUTTONDOWN:
        idleSeconds = 0;
        return 0;

    case WM_TIMER:
        idleSeconds++;
        if (idleSeconds >= 6 && !sc_showed)
        {
            sc_showed = true;
            idleSeconds = 0;
            ShowScreensaver();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;

    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MyEditorWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassEx(&wc)) return 0;

    HWND hwnd = CreateWindowEx(
        0,
        L"MyEditorWindowClass",
        L"Моё главное окно редактора",
        WS_OVERLAPPEDWINDOW,
        0, 0,
        800, 600,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
