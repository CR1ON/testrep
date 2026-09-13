#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tlhelp32.h>
#include <commctrl.h>
#include <string>

#pragma comment(lib, "comctl32.lib")

constexpr int ID_LIST = 1001;
constexpr int ID_REFRESH = 1002;
constexpr int ID_SEARCH = 1003;
constexpr int ID_AUTO_REFRESH = 1004;
constexpr int ID_STATUS = 1005;
constexpr int ID_HEADER = 1006;
constexpr int ID_SUBTITLE = 1007;
constexpr int ID_TIMER = 2001;

constexpr COLORREF APP_BACKGROUND = RGB(245, 247, 250);
constexpr COLORREF APP_PANEL = RGB(255, 255, 255);
constexpr COLORREF APP_TEXT = RGB(31, 41, 55);
constexpr COLORREF APP_MUTED = RGB(107, 114, 128);

HINSTANCE g_instance = nullptr;
HWND g_list = nullptr;
HWND g_search = nullptr;
HWND g_status = nullptr;
HWND g_autoRefresh = nullptr;
HFONT g_font = nullptr;
HFONT g_titleFont = nullptr;
HFONT g_subtitleFont = nullptr;
HBRUSH g_backgroundBrush = nullptr;
HBRUSH g_panelBrush = nullptr;

std::wstring Number(DWORD value) {
    return std::to_wstring(value);
}

bool ContainsIgnoreCase(const std::wstring& text, const std::wstring& query) {
    if (query.empty()) return true;

    std::wstring a = text;
    std::wstring b = query;

    if (!a.empty()) {
        CharLowerBuffW(a.data(), static_cast<DWORD>(a.size()));
    }
    if (!b.empty()) {
        CharLowerBuffW(b.data(), static_cast<DWORD>(b.size()));
    }

    return a.find(b) != std::wstring::npos;
}

void SetControlFont(HWND control, HFONT font) {
    SendMessageW(control, WM_SETFONT,
                 reinterpret_cast<WPARAM>(font), TRUE);
}

void AddColumn(int index, const wchar_t* title, int width) {
    LVCOLUMNW column{};
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    column.iSubItem = index;
    column.cx = width;
    column.pszText = const_cast<LPWSTR>(title);
    ListView_InsertColumn(g_list, index, &column);
}

void RefreshProcesses() {
    std::wstring query;

    int searchLength = GetWindowTextLengthW(g_search);
    if (searchLength > 0) {
        query.resize(searchLength);
        GetWindowTextW(g_search, query.data(), searchLength + 1);
    }

    ListView_DeleteAllItems(g_list);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::wstring text = L"Ошибка снимка: " + Number(GetLastError());
        SetWindowTextW(g_status, text.c_str());
        return;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    int row = 0;
    int total = 0;

    if (!Process32FirstW(snapshot, &entry)) {
        std::wstring text = L"Ошибка Process32FirstW: " + Number(GetLastError());
        SetWindowTextW(g_status, text.c_str());
        CloseHandle(snapshot);
        return;
    }

    do {
        ++total;

        std::wstring processName = entry.szExeFile;
        if (!ContainsIgnoreCase(processName, query)) {
            continue;
        }

        std::wstring pid = Number(entry.th32ProcessID);
        std::wstring parentPid = Number(entry.th32ParentProcessID);
        std::wstring threads = Number(entry.cntThreads);

        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = row;
        item.iSubItem = 0;
        item.pszText = pid.data();

        int inserted = ListView_InsertItem(g_list, &item);

        if (inserted >= 0) {
            ListView_SetItemText(g_list, inserted, 1, parentPid.data());
            ListView_SetItemText(g_list, inserted, 2, threads.data());
            ListView_SetItemText(g_list, inserted, 3, processName.data());
            ++row;
        }
    } while (Process32NextW(snapshot, &entry));

    DWORD lastError = GetLastError();
    CloseHandle(snapshot);

    std::wstring status = L"Показано: " + Number(row) +
                          L" из " + Number(total) + L" процессов";

    if (lastError != ERROR_NO_MORE_FILES) {
        status += L" | Ошибка: " + Number(lastError);
    }

    SetWindowTextW(g_status, status.c_str());
}

void ResizeControls(HWND hwnd) {
    RECT rc{};
    GetClientRect(hwnd, &rc);

    int width = rc.right;
    int height = rc.bottom;
    int margin = 24;
    int statusHeight = 28;

    SetWindowPos(GetDlgItem(hwnd, ID_HEADER), nullptr,
                 margin, 18, width - margin * 2, 32, SWP_NOZORDER);

    SetWindowPos(GetDlgItem(hwnd, ID_SUBTITLE), nullptr,
                 margin, 53, width - margin * 2, 22, SWP_NOZORDER);

    SetWindowPos(g_search, nullptr,
                 margin, 91, width - 264, 38, SWP_NOZORDER);

    SetWindowPos(GetDlgItem(hwnd, ID_REFRESH), nullptr,
                 width - 210, 91, 95, 38, SWP_NOZORDER);

    SetWindowPos(g_autoRefresh, nullptr,
                 width - 105, 91, 81, 38, SWP_NOZORDER);

    SetWindowPos(g_list, nullptr,
                 margin, 143, width - margin * 2,
                 height - 143 - statusHeight - margin, SWP_NOZORDER);

    SetWindowPos(g_status, nullptr,
                 margin, height - statusHeight - 9,
                 width - margin * 2, statusHeight, SWP_NOZORDER);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message,
                            WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        INITCOMMONCONTROLSEX icc{};
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
        InitCommonControlsEx(&icc);

        g_font = CreateFontW(
            -16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        g_titleFont = CreateFontW(
            -25, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        g_subtitleFont = CreateFontW(
            -15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        g_backgroundBrush = CreateSolidBrush(APP_BACKGROUND);
        g_panelBrush = CreateSolidBrush(APP_PANEL);

        HWND header = CreateWindowW(
            L"STATIC", L"Инспектор процессов",
            WS_CHILD | WS_VISIBLE,
            24, 18, 600, 32, hwnd,
            reinterpret_cast<HMENU>(ID_HEADER), g_instance, nullptr);

        HWND subtitle = CreateWindowW(
            L"STATIC", L"Список запущенных процессов Windows",
            WS_CHILD | WS_VISIBLE,
            24, 53, 600, 22, hwnd,
            reinterpret_cast<HMENU>(ID_SUBTITLE), g_instance, nullptr);

        g_search = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            24, 91, 500, 38, hwnd,
            reinterpret_cast<HMENU>(ID_SEARCH), g_instance, nullptr);

        HWND refresh = CreateWindowW(
            L"BUTTON", L"Обновить",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            600, 91, 95, 38, hwnd,
            reinterpret_cast<HMENU>(ID_REFRESH), g_instance, nullptr);

        g_autoRefresh = CreateWindowW(
            L"BUTTON", L"Авто",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            705, 91, 81, 38, hwnd,
            reinterpret_cast<HMENU>(ID_AUTO_REFRESH), g_instance, nullptr);

        g_list = CreateWindowExW(
            WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL |
            LVS_SHOWSELALWAYS,
            24, 143, 700, 400, hwnd,
            reinterpret_cast<HMENU>(ID_LIST), g_instance, nullptr);

        ListView_SetExtendedListViewStyle(
            g_list,
            LVS_EX_FULLROWSELECT |
            LVS_EX_GRIDLINES |
            LVS_EX_DOUBLEBUFFER |
            LVS_EX_HEADERDRAGDROP);

        AddColumn(0, L"PID", 105);
        AddColumn(1, L"Родительский PID", 160);
        AddColumn(2, L"Потоки", 100);
        AddColumn(3, L"Имя процесса", 360);

        g_status = CreateWindowW(
            L"STATIC", L"Загрузка...",
            WS_CHILD | WS_VISIBLE,
            24, 520, 700, 28, hwnd,
            reinterpret_cast<HMENU>(ID_STATUS), g_instance, nullptr);

        SetControlFont(header, g_titleFont);
        SetControlFont(subtitle, g_subtitleFont);
        SetControlFont(g_search, g_font);
        SetControlFont(refresh, g_font);
        SetControlFont(g_autoRefresh, g_font);
        SetControlFont(g_list, g_font);
        SetControlFont(g_status, g_subtitleFont);

        SendMessageW(g_search, EM_SETCUEBANNER, FALSE,
                     reinterpret_cast<LPARAM>(L"Поиск по имени процесса..."));

        SetTimer(hwnd, ID_TIMER, 1000, nullptr);
        RefreshProcesses();
        return 0;
    }

    case WM_TIMER:
        if (wParam == ID_TIMER &&
            SendMessageW(g_autoRefresh, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            RefreshProcesses();
        }
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_REFRESH) {
            RefreshProcesses();
            return 0;
        }

        if (LOWORD(wParam) == ID_SEARCH && HIWORD(wParam) == EN_CHANGE) {
            RefreshProcesses();
            return 0;
        }
        break;

    case WM_NOTIFY:
        if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == g_list &&
            reinterpret_cast<LPNMHDR>(lParam)->code == NM_CUSTOMDRAW) {
            auto* draw = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

            if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) {
                return CDRF_NOTIFYITEMDRAW;
            }

            if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                draw->clrText = APP_TEXT;
                draw->clrTextBk =
                    (draw->nmcd.dwItemSpec % 2 == 0)
                    ? RGB(255, 255, 255)
                    : RGB(248, 250, 252);
                return CDRF_NEWFONT;
            }
        }
        break;

    case WM_SIZE:
        ResizeControls(hwnd);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, control == GetDlgItem(hwnd, ID_HEADER)
                            ? APP_TEXT : APP_MUTED);
        return reinterpret_cast<LRESULT>(g_backgroundBrush);
    }

    case WM_CTLCOLOREDIT: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, APP_TEXT);
        SetBkColor(dc, APP_PANEL);
        return reinterpret_cast<LRESULT>(g_panelBrush);
    }

    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER);
        DeleteObject(g_font);
        DeleteObject(g_titleFont);
        DeleteObject(g_subtitleFont);
        DeleteObject(g_backgroundBrush);
        DeleteObject(g_panelBrush);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int commandShow) {
    g_instance = instance;

    const wchar_t className[] = L"ModernProcessInspectorWindow";

    WNDCLASSW wc{};
    wc.hInstance = instance;
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        className,
        L"Process Inspector",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        920,
        650,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!hwnd) return 1;

    ShowWindow(hwnd, commandShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
