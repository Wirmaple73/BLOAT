#include <iostream>
#include <filesystem>
#include "BloatArchive.h"
#include "Exceptions.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
	BloatArchive archive = BloatArchive::Open(R"(D:\archive.blt)");
	archive.Extract(R"(D:\archive outputs\)", false);
}
