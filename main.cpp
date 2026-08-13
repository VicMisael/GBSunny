#include "frontend/frontend.h"

#include <string>

int main(int argc, char* argv[])
{
	return frontend::run(argc > 1 ? std::string{ argv[1] } : std::string{});
}
