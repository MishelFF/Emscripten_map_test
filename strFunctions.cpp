#include "strFunctions.h"
std::string trim(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    const auto start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {return ""; }
    const auto end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}
bool is_digits(const std::string& str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), [](unsigned char c) {
        return std::isdigit(c);
    });
}
