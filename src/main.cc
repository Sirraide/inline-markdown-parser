#include <parser.hh>

int main() {
    std::string input;
    for (;;) {
        fmt::print("> ");
        if (not std::getline(std::cin, input)) break;
        if (input.empty()) continue;
        fmt::print("\033[32m{}\033[m\n", Parser(input).Print());
    }
}
