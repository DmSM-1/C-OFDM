#include "parser.hpp"


ConfigMap parse_config(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) throw std::runtime_error("Cannot open config file");

    ConfigMap cfg;
    std::string line;

    while (std::getline(file, line)) {
        // убираем пробелы слева/справа
        line.erase(line.begin(), std::find_if(line.begin(), line.end(),
                    [](unsigned char ch){ return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(),
                    [](unsigned char ch){ return !std::isspace(ch); }).base(), line.end());

        if (line.empty() || line[0] == '#') continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // убираем пробелы вокруг
        key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
        value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());

        cfg[key] = std::stol(value);
    }
    return cfg;
}
