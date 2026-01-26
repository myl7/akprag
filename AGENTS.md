# Instructions for AI Agents

You are an expert C++/C/Python developer. Follow these instructions carefully.

## Code Style & Formatting

1. **Base Style**: Adhere to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
2. **Formatting**: Run `clang-format` on any file you modify.
3. **Control Structures**: For short single-line statements, omit braces.
    - _Example (If/Else)_:
        ```cpp
        if (condition) DoSth();
        else DoSthElse();
        ```
    - _Example (Switch)_:
        ```cpp
        switch (value) {
            case 0: DoSth(); break;
            default: DoSthDflt();
        }
        ```
    - _Example (Loops)_:
        ```cpp
        for (int i = 0; i < n; ++i) DoSth(i);
        ```
4. **Error Messages**: Do not capitalize error messages (start with lowercase), unless the first word is a proper noun.
5. **Includes**: **NEVER** reorder existing `#include` directives.
6. **`std::`**: Use `memcpy` instead of `std::memcpy` while `memcpy` is already in global namespace. Also for `memset`, etc.
7. Load venv by `source venv/bin/activate` before any python run.
8. Note that if you `cd` to another dir, any following command will be executed in that dir.
   If you get error msg like "cd: no such file or directory:", first run `pwd` to find out where you are.

## Build Instructions

- Always place build outputs in the `/build` directory.

## Naming Conventions & Abbreviations

Use the following abbreviations in variable and function names:

| Full Word      | Abbreviation |
| :------------- | :----------- |
| `evaluation`   | `eval`       |
| `generation`   | `gen`        |
| `less than`    | `lt`         |
| `greater than` | `gt`         |
| `temporary`    | `tmp`        |
| `buffer`       | `buf`        |
| `benchmark`    | `bench`      |
