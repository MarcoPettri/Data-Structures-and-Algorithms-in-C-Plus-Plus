// Exercise Creativity: C-2.1
/*
    Give an example of a C++ program that outputs its source code when it is
    run. Such a program is called a quine.
*/

#include <stdio.h>
#include <string>

int main() {
    std::string s = "#include <stdio.h>%c#include <string>%c%cint main() {%c    std::string s = %c%s%c;%c    printf(s.c_str(), 10, 10, 10, 10, 34, s.c_str(), 34, 10, 10, 10);%c}%c";
    printf(s.c_str(), 10, 10, 10, 10, 34, s.c_str(), 34, 10, 10, 10);
}