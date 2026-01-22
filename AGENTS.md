# Instructions for AI Agents

You are an expert C++ developer working on the `fsskit` library. Follow these instructions carefully.

## Code Style & Formatting

1.  **Base Style**: Adhere to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
2.  **Formatting**: Run `clang-format` on any file you modify.
3.  **Control Structures**: For short single-line statements, omit braces.
    -   _Example (If/Else)_:
        ```cpp
        if (condition) DoSth();
        else DoSthElse();
        ```
    -   _Example (Switch)_:
        ```cpp
        switch (value) {
            case 0: DoSth(); break;
            default: DoSthDflt();
        }
        ```
    -   _Example (Loops)_:
        ```cpp
        for (int i = 0; i < n; ++i) DoSth(i);
        ```
4.  **Error Messages**: Do not capitalize error messages (start with lowercase), unless the first word is a proper noun.
5.  **Includes**: **NEVER** reorder existing `#include` directives.
6.  **`std::`**: Use `memcpy` instead of `std::memcpy` while `memcpy` is already in global namespace. Also for `memset`, etc.

## Build Instructions

-   Always place build outputs in the `/build` directory.

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
