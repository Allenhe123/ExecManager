#include "common.h"
#include <sstream>

bool file_exist(const std::string& file_name) {
    std::ifstream f(file_name.c_str());
    return f.good();
}

std::vector<std::string> split(const std::string& s, char delimiter)
{
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream(s);
   while (std::getline(tokenStream, token, delimiter))
   {
      tokens.push_back(std::move(token));
   }
   return std::move(tokens);
}