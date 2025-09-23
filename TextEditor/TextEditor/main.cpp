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

void LoadFileToEdit(HWND hwndEdit, LPCWSTR fileName) {
    HANDLE hFile = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) { CloseHandle(hFile); return; }

    char* buffer = (char*)HeapAlloc(GetProcessHeap(), 0, fileSize + 1);
    if (!buffer) { CloseHandle(hFile); return; }

    DWORD bytesRead;
    if (ReadFile(hFile, buffer, fileSize, &bytesRead, NULL)) {
        buffer[bytesRead] = '\0';

        // преобразуем в Unicode (здесь предполагаем UTF-8)
        int lenW = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, NULL, 0);
        if (lenW > 0) {
            wchar_t* wbuf = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(wchar_t));
            if (wbuf) {
                MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wbuf, lenW);
                SetWindowTextW(hwndEdit, wbuf);
                HeapFree(GetProcessHeap(), 0, wbuf);
            }
        }
    }
    HeapFree(GetProcessHeap(), 0, buffer);
    CloseHandle(hFile);
}

void SaveEditToFile(HWND hwndEdit, LPCWSTR fileName) {
    int len = GetWindowTextLengthW(hwndEdit);
    wchar_t* buffer = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(wchar_t));
    if (!buffer) return;

    GetWindowTextW(hwndEdit, buffer, len + 1);

    // Конвертируем в UTF-8
    int utf8len = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, NULL, 0, NULL, NULL);
    char* utf8buffer = (char*)HeapAlloc(GetProcessHeap(), 0, utf8len);
    if (!utf8buffer) { HeapFree(GetProcessHeap(), 0, buffer); return; }

    WideCharToMultiByte(CP_UTF8, 0, buffer, -1, utf8buffer, utf8len, NULL, NULL);

    HANDLE hFile = CreateFileW(fileName, GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        WriteFile(hFile, utf8buffer, utf8len - 1, &bytesWritten, NULL); // без \0
        CloseHandle(hFile);
    }

    HeapFree(GetProcessHeap(), 0, buffer);
    HeapFree(GetProcessHeap(), 0, utf8buffer);
}

HWND hEdit = NULL;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {

        hEdit = CreateWindowExW(
            0,                      // dwExStyle
            L"EDIT",                // класс
            L"",                    // начальный текст
            WS_CHILD | WS_VISIBLE | WS_BORDER |
            ES_LEFT | ES_MULTILINE |
            ES_AUTOVSCROLL | ES_AUTOHSCROLL |
            ES_WANTRETURN,          // ES_WANTRETURN полезен в диалогах/окнах, чтобы Enter вставлялся
            10, 10, 760, 520,       // позиция и размер (будем менять в WM_SIZE)
            hwnd,
            (HMENU)1001,            // id контрола
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );
        // Установим системный GUI-шрифт (иначе текст будет выглядеть по-старинке)
        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);


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
        case ID_FILE_OPEN:
        {
            OPENFILENAME ofn;
            wchar_t szFile[MAX_PATH] = { 0 };
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = L"Текстовые файлы\0*.txt\0Все файлы\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            if (GetOpenFileNameW(&ofn)) {
                LoadFileToEdit(hEdit, szFile);
            }
            break;
        }
        case ID_FILE_SAVE:
        {
            OPENFILENAME ofn;
            wchar_t szFile[MAX_PATH] = { 0 };
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = L"Текстовые файлы\0*.txt\0Все файлы\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
            if (GetSaveFileNameW(&ofn)) {
                SaveEditToFile(hEdit, szFile);
            }
            break;
        }
        case ID_EDIT_CUT:
            SendMessage(hEdit, WM_CUT, 0, 0); break;
        case ID_EDIT_COPY:
            SendMessage(hEdit, WM_COPY, 0, 0); break;
        case ID_EDIT_PASTE:
            SendMessage(hEdit, WM_PASTE, 0, 0); break;
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
        if (idleSeconds >= 20 && !sc_showed)
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
