#include "stdafx.h"
#include "LanguageManager.h"
#include <cstdio>
#include <algorithm>

static std::string TrimLangString(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

LanguageManager& LanguageManager::instance() {
    static LanguageManager s_instance;
    return s_instance;
}

bool LanguageManager::LoadFromFile(const std::string& path) {
    FILE* f = NULL;
    std::wstring wpath(path.begin(), path.end());
    if (_wfopen_s(&f, wpath.c_str(), L"r") != 0 || !f)
        return false;

    char line[512];
    std::string currentSection;

    while (fgets(line, sizeof(line), f)) {
        std::string s = TrimLangString(line);
        if (s.empty() || s[0] == '#' || s[0] == ';')
            continue;

        if (s[0] == '[') {
            size_t close = s.find(']');
            if (close != std::string::npos)
                currentSection = s.substr(1, close - 1);
            continue;
        }

        size_t eq = s.find('=');
        if (eq == std::string::npos || currentSection.empty())
            continue;

        std::string key = TrimLangString(s.substr(0, eq));
        std::string val = TrimLangString(s.substr(eq + 1));
        if (!key.empty())
            m_strings[currentSection][key] = val;
    }

    fclose(f);

    if (m_currentLang.empty())
        m_currentLang = "English";

    return true;
}

void LanguageManager::SetLanguage(const std::string& lang) {
    m_currentLang = lang;
}

std::string LanguageManager::GetString(const std::string& key) const {
    auto sectionIt = m_strings.find(m_currentLang);
    if (sectionIt == m_strings.end())
        return key;

    auto it = sectionIt->second.find(key);
    if (it == sectionIt->second.end())
        return key;

    return it->second;
}
