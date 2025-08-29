#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>

using ConfigMap = std::unordered_map<std::string, int>;

ConfigMap parse_config(const std::string& filename) ;