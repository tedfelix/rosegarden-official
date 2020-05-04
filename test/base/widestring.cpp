#include <iostream>
#include <string>

int main()
{
    std::wstring widestring(L"This is a test");
    widestring += L" of wide character strings";

    for (size_t i = 0; i < widestring.length(); ++i) {
        if (widestring[i] == L'w' ||
            widestring[i] == L'c') {
            widestring[i] = toupper(widestring[i]);
        }
    }

    std::cout << "Testing wide string...\n";
    std::cout << "String value is \"";
    std::wcout << widestring;
    std::cout << "\"\n";
    std::cout << "String's length is " << widestring.length() << '\n';
    std::cout << "and storage space is "
         << (widestring.length() * sizeof(widestring[0])) << '\n';

    std::cout << "Characters are:\n";
    for (size_t i = 0; i < widestring.length(); ++i) {
        std::cout << widestring[i];
        if (i < widestring.length()-1)
            std::cout << " ";
    }

    std::cout << std::endl;

    return 0;
}

