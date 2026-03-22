#pragma once
#include "Protocol_asmt6.h"
#include <vector>
class RequestParser
{
public:
	Request parseRequest(const std::vector<char>& buffer);
};

