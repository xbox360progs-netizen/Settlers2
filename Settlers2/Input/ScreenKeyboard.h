#pragma once
#include <d3d9.h>
#include <string>
#include "Gamepad.h"

namespace Input {

// Режим экранной клавиатуры
enum ScreenKeyboardMode
{
    SKM_Normal,      // Обычный режим (A-Z, 0-9)
    SKM_Uppercase,   // Заглавные буквы
    SKM_Symbols,     // Символы
    SKM_Numbers      // Только цифры
};

// Состояние экранной клавиатуры
struct ScreenKeyboardState
{
    bool visible;
    ScreenKeyboardMode mode;
    std::string text;
    int cursorPosition;
    int maxLength;
    
    ScreenKeyboardState()
        : visible(false)
        , mode(SKM_Normal)
        , cursorPosition(0)
        , maxLength(32)
    {
    }
};

// Экранная клавиатура для Xbox 360
// Позволяет вводить текст с помощью геймпада
class ScreenKeyboard
{
public:
    ScreenKeyboard();
    ~ScreenKeyboard();

    // Инициализация
    bool Initialize(IDirect3DDevice9* device);
    void Shutdown();

    // Показать/скрыть клавиатуру
    void Show(const std::string& initialText = "", int maxLength = 32);
    void Hide();
    bool IsVisible() const { return m_state.visible; }

    // Получить введенный текст
    const std::string& GetText() const { return m_state.text; }
    void SetText(const std::string& text);

    // Обновление
    void Update(float deltaTime, Gamepad* gamepad);

    // Рендеринг
    void Render(IDirect3DDevice9* device);

    // Проверка завершения ввода
    bool IsConfirmed() const { return m_confirmed; }
    bool IsCancelled() const { return m_cancelled; }
    void ResetFlags();

private:
    IDirect3DDevice9* m_device;
    ScreenKeyboardState m_state;
    bool m_confirmed;
    bool m_cancelled;

    // Навигация по клавиатуре
    int m_selectedRow;
    int m_selectedCol;
    float m_cursorBlinkTime;
    bool m_showCursor;

    // Клавиатурная раскладка
    static const int ROWS = 4;
    static const int COLS = 10;
    char m_layout[ROWS][COLS];

    // Инициализация раскладки
    void InitLayout();

    // Обработка ввода
    void HandleInput(Gamepad* gamepad, float deltaTime);
    void MoveSelection(int dx, int dy);
    void SelectCurrentKey();
    void DeleteChar();
    void Backspace();
    void Confirm();
    void Cancel();
    void ToggleMode();

    // Получить символ по позиции
    char GetCharAt(int row, int col) const;
};

} // namespace Input
