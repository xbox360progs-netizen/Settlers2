#pragma once
#include <string>
#include <map>

class LanguageManager
{
public:
    static LanguageManager& instance();

    bool LoadFromFile(const std::string& path);
    void SetLanguage(const std::string& lang);
    std::string GetString(const std::string& key) const;
    std::string GetCurrentLanguage() const { return m_currentLang; }

private:
    LanguageManager() {}
    std::string m_currentLang;
    std::map<std::string, std::map<std::string, std::string>> m_strings;
};
