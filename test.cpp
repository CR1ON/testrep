#include <windows.h>
#include <cstdio>

// ---------- Утилита: печатаем ошибку WinAPI по коду ----------
void PrintLastError(const char* where) {
    DWORD err = GetLastError();
    printf("[ОШИБКА] %s -> код %lu\n", where, err);
}

// ---------- Секция 1. Файлы ----------
void DemoFiles() {
    printf("\n=== 1. Работа с файлами ===\n");

    const wchar_t* fileName = L"demo_file.txt";
    const char* textToWrite = "Hello, WinAPI files!\n";

    // 1.1. Создаём файл (или перезаписываем существующий)
    HANDLE hFile = CreateFileW(
        fileName,                  // имя файла (W-версия, UTF-16)
        GENERIC_WRITE,             // хотим писать
        0,                         // никому больше не давать доступ
        nullptr,                   // атрибуты безопасности по умолчанию
        CREATE_ALWAYS,             // создать заново (перезаписать)
        FILE_ATTRIBUTE_NORMAL,     // обычный файл
        nullptr                    // шаблона нет
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        PrintLastError("CreateFileW (write)");
        return;
    }
    printf("Файл создан: demo_file.txt\n");

    // 1.2. Пишем строку
    DWORD bytesWritten = 0;
    BOOL ok = WriteFile(
        hFile,
        textToWrite,
        (DWORD)strlen(textToWrite),
        &bytesWritten,
        nullptr
    );

    if (!ok) {
        PrintLastError("WriteFile");
        CloseHandle(hFile);
        return;
    }
    printf("Записано байт: %lu\n", bytesWritten);

    // 1.3. Закрываем дескриптор записи
    CloseHandle(hFile);

    // 1.4. Открываем заново — теперь на чтение
    hFile = CreateFileW(
        fileName,
        GENERIC_READ,
        FILE_SHARE_READ,           // разрешаем другим читать
        nullptr,
        OPEN_EXISTING,             // файл уже существует
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        PrintLastError("CreateFileW (read)");
        return;
    }

    // 1.5. Читаем содержимое
    char buffer[256] = {0};
    DWORD bytesRead = 0;
    ok = ReadFile(
        hFile,
        buffer,
        sizeof(buffer) - 1,        // оставляем место под '\0'
        &bytesRead,
        nullptr
    );

    if (!ok) {
        PrintLastError("ReadFile");
        CloseHandle(hFile);
        return;
    }

    buffer[bytesRead] = '\0';      // ставим нуль-терминатор вручную
    printf("Прочитано байт: %lu\n", bytesRead);
    printf("Содержимое файла: \"%s\"\n", buffer);

    CloseHandle(hFile);
}

// ---------- Секция 2. Виртуальная память ----------
void DemoMemory() {
    printf("\n=== 2. Виртуальная память ===\n");

    // 2.1. Выделяем 4 КБ (одна страница) с правом чтения и записи
    SIZE_T size = 4096;
    LPVOID mem = VirtualAlloc(
        nullptr,                   // пусть система выберет адрес
        size,
        MEM_COMMIT | MEM_RESERVE,  // зарезервировать И выделить физически
        PAGE_READWRITE             // чтение + запись
    );

    if (mem == nullptr) {
        PrintLastError("VirtualAlloc");
        return;
    }
    printf("Выделено %zu байт по адресу %p\n", size, mem);

    // 2.2. Пишем туда строку
    const char* msg = "Данные в виртуальной памяти";
    strcpy_s((char*)mem, size, msg);
    printf("Записано в память: \"%s\"\n", (char*)mem);

    // 2.3. Читаем обратно (просто обращаемся как к обычному указателю)
    printf("Прочитано из памяти: \"%s\"\n", (char*)mem);

    // 2.4. Освобождаем
    if (!VirtualFree(mem, 0, MEM_RELEASE)) {
        PrintLastError("VirtualFree");
        return;
    }
    printf("Память освобождена\n");
}

// ---------- Секция 3. Процессы ----------
void DemoProcess() {
    printf("\n=== 3. Запуск процесса ===\n");

    // 3.1. Наш собственный PID — просто для справки
    DWORD myPid = GetCurrentProcessId();
    printf("PID текущего процесса: %lu\n", myPid);

    // 3.2. Готовим командную строку.
    // CreateProcessW может ИЗМЕНЯТЬ буфер, поэтому нужен массив, не литерал.
    wchar_t cmdLine[] = L"cmd.exe /c echo Hello from child process";

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    // 3.3. Запускаем дочерний процесс
    BOOL ok = CreateProcessW(
        nullptr,        // имя приложения — берём из командной строки
        cmdLine,        // командная строка (writable!)
        nullptr,        // атрибуты процесса
        nullptr,        // атрибуты потока
        FALSE,          // не наследовать дескрипторы
        0,              // флаги создания
        nullptr,        // окружение — наследуем
        nullptr,        // рабочая директория — текущая
        &si,
        &pi
    );

    if (!ok) {
        PrintLastError("CreateProcessW");
        return;
    }
    printf("Дочерний процесс запущен, PID = %lu\n", pi.dwProcessId);

    // 3.4. Ждём завершения
    WaitForSingleObject(pi.hProcess, INFINITE);

    // 3.5. Узнаём код возврата
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    printf("Дочерний процесс завершён, код возврата = %lu\n", exitCode);

    // 3.6. Закрываем дескрипторы (обязательно!)
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

// ---------- Точка входа ----------
int main() {
    SetConsoleOutputCP(CP_UTF8);   // консоль выводит в UTF-8
    SetConsoleCP(CP_UTF8); 
    printf("WinAPI Demo: файлы, память, процессы\n");
    printf("=====================================\n");

    DemoFiles();
    DemoMemory();
    DemoProcess();

    printf("\n=== Готово ===\n");
    printf("Нажми Enter, чтобы выйти...");
    getchar();   // чтобы консоль не закрылась при двойном клике

    return 0;
}