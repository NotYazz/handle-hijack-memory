#include "memory/mem.h"
#include <iostream>

enum logType {
	GOOD = 0,
	WARN = 1,
	UNKNOWN = 2,
	BAD = 3
};

enum class Color {
	Green,
	Yellow,
	Cyan,
	Red,
	Reset
};

constexpr std::string_view getColorCode(Color color) {
	switch (color) {
	case Color::Green:  return "\033[32m";
	case Color::Yellow: return "\033[33m";
	case Color::Cyan:   return "\033[36m";
	case Color::Red:    return "\033[31m";
	case Color::Reset:  return "\033[0m";
	}
	return "";
}

void log(int type, std::string_view message) {
	if (type == GOOD) {
		std::cout << getColorCode(Color::Green) << "[+]" << getColorCode(Color::Reset) << " " << message;
	}
	else if (type == WARN) {
		std::cout << getColorCode(Color::Yellow) << "[!]" << getColorCode(Color::Reset) << " " << message;
	}
	else if (type == UNKNOWN) {
		std::cout << getColorCode(Color::Cyan) << "[?]" << getColorCode(Color::Reset) << " " << message;
	}
	else if (type == BAD) {
		std::cout << getColorCode(Color::Red) << "[-]" << getColorCode(Color::Reset) << " " << message;
	}
}

int main() {
	Memory* mem = new Memory{ "cs2.exe" };
	uintptr_t client = mem->GetModuleBase("client.dll");

	if (client) {
		log(GOOD, "Client found -> ");
		std::cout << std::hex << client << std::dec << std::endl;
	}
	else {
		log(BAD, "Client not found!\n");
	}

	uintptr_t localPlayer = mem->read<uintptr_t>(client + 0x18540D0);
	if (localPlayer) {
		log(GOOD, "Local player found -> ");
		std::cout << std::hex << localPlayer << std::dec << std::endl << std::endl;
	}
	else {
		log(BAD, "Local player not found!\n");
	}

	log(GOOD, "GOOD LOG\n");
	log(BAD, "BAD LOG\n");
	log(UNKNOWN, "UNKNOWN LOG\n");
	log(WARN, "WARNING LOG\n");

	std::cin.get();

	return 0;
}
