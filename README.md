## `inspector.cpp`

`inspector.cpp` — консольный инспектор процессов Windows.

Он умеет:

- выводить список запущенных процессов;
- показывать имя процесса и его PID;
- фильтровать процессы по названию;
- автоматически обновлять список;

### Компиляция

**MSVC / Visual Studio Developer Command Prompt:**

```bat
cl /EHsc /std:c++17 inspector.cpp /Fe:inspector.exe
```

**MinGW-w64:**

```bash
g++ -std=c++17 -O2 -Wall -Wextra inspector.cpp -o inspector.exe
```

## `test.cpp`

`test.cpp` — демо для ознакомления с базовыми функциями WinAPI.

Она последовательно показывает:

1. **Работу с файлами** — создаёт `demo_file.txt`, записывает в него текст, затем открывает файл и читает содержимое.
2. **Работу с виртуальной памятью** — выделяет 4096 байт с помощью `VirtualAlloc`, записывает строку и освобождает память через `VirtualFree`.
3. **Работу с процессами** — получает PID текущего процесса, запускает дочерний `cmd.exe`, ожидает его завершения и выводит код возврата.
4. **Обработку ошибок WinAPI** — выводит код последней ошибки через `GetLastError`.

### Компиляция

**MSVC / Visual Studio Developer Command Prompt:**

```bat
cl /EHsc /std:c++17 test.cpp /Fe:test.exe
```

**MinGW-w64:**

```bash
g++ -std=c++17 -O2 -Wall -Wextra test.cpp -o test.exe
```
