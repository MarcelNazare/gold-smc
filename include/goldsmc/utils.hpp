#pragma once
#include <string>

namespace goldsmc::utils {

std::string getEnv(const std::string& name);
void        loadDotEnv(const std::string& path);

void logInfo (const std::string& msg);
void logWarn (const std::string& msg);
void logError(const std::string& msg);

std::string urlEncode(const std::string& s);
std::string nowIsoUtc();

} // namespace goldsmc::utils