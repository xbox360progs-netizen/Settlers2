#pragma once
#include "UiMessageId.h"

namespace UI {

    enum Language {
        LANG_EN,
        LANG_RU,
        LANG_COUNT
    };

    class LocalizationService {
    public:
        LocalizationService();
        ~LocalizationService();

        void SetLanguage(Language lang);
        Language GetLanguage() const { return m_lang; }

        const char* Get(UiMessageId id) const;

        // Format a message with arguments into a buffer.
        // Returns number of bytes written (not counting null terminator).
        int Format(UiMessageId id, const UiFormatArgs& args, char* out, int capacity) const;

    private:
        Language m_lang;
    };

} // namespace UI
