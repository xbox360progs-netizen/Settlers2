#include "StdAfx.h"
#include "ScreenKeyboard.h"
#include <cstring>

namespace Input {

ScreenKeyboard::ScreenKeyboard()
    : m_device(NULL)
    , m_confirmed(false)
    , m_cancelled(false)
    , m_selectedRow(0)
    , m_selectedCol(0)
    , m_cursorBlinkTime(0.0f)
    , m_showCursor(true)
{
    InitLayout();
}

ScreenKeyboard::~ScreenKeyboard()
{
    Shutdown();
}

bool ScreenKeyboard::Initialize(IDirect3DDevice9* device)
{
    if (!device)
        return false;

    m_device = device;
    m_device->AddRef();

    return true;
}

void ScreenKeyboard::Shutdown()
{
    if (m_device)
    {
        m_device->Release();
        m_device = NULL;
    }
}

void ScreenKeyboard::Show(const std::string& initialText, int maxLength)
{
    m_state.visible = true;
    m_state.text = initialText;
    m_state.cursorPosition = initialText.length();
    m_state.maxLength = maxLength;
    m_state.mode = SKM_Normal;
    m_confirmed = false;
    m_cancelled = false;
    m_selectedRow = 0;
    m_selectedCol = 0;
}

void ScreenKeyboard::Hide()
{
    m_state.visible = false;
}

void ScreenKeyboard::SetText(const std::string& text)
{
    m_state.text = text;
    m_state.cursorPosition = text.length();
}

void ScreenKeyboard::Update(float deltaTime, Gamepad* gamepad)
{
    if (!m_state.visible || !gamepad)
        return;

    // Мигание курсора
    m_cursorBlinkTime += deltaTime;
    if (m_cursorBlinkTime > 0.5f)
    {
        m_cursorBlinkTime = 0.0f;
        m_showCursor = !m_showCursor;
    }

    HandleInput(gamepad, deltaTime);
}

void ScreenKeyboard::Render(IDirect3DDevice9* device)
{
    if (!m_state.visible)
        return;

    // TODO: Реализовать рендеринг экранной клавиатуры
    // Нужно нарисовать:
    // 1. Фон клавиатуры
    // 2. Кнопки с символами
    // 3. Выбранный символ (подсветка)
    // 4. Текстовое поле с введенным текстом
    // 5. Курсор
}

void ScreenKeyboard::ResetFlags()
{
    m_confirmed = false;
    m_cancelled = false;
}

void ScreenKeyboard::InitLayout()
{
    // Обычная раскладка (QWERTY)
    const char* layout[ROWS] = {
        "1234567890",
        "QWERTYUIOP",
        "ASDFGHJKL ",
        "ZXCVBNM   "
    };

    for (int row = 0; row < ROWS; row++)
    {
        for (int col = 0; col < COLS; col++)
        {
            m_layout[row][col] = layout[row][col];
        }
    }
}

void ScreenKeyboard::HandleInput(Gamepad* gamepad, float deltaTime)
{
    // Навигация D-Pad
    if (gamepad->IsButtonPressed(GP_DPadUp))
        MoveSelection(0, -1);
    if (gamepad->IsButtonPressed(GP_DPadDown))
        MoveSelection(0, 1);
    if (gamepad->IsButtonPressed(GP_DPadLeft))
        MoveSelection(-1, 0);
    if (gamepad->IsButtonPressed(GP_DPadRight))
        MoveSelection(1, 0);

    // Левый стик для навигации
    float lx, ly;
    gamepad->GetLeftStick(lx, ly);
    if (fabsf(lx) > 0.5f || fabsf(ly) > 0.5f)
    {
        static float stickTimer = 0.0f;
        stickTimer += deltaTime;
        if (stickTimer > 0.2f)
        {
            stickTimer = 0.0f;
            if (lx > 0.5f) MoveSelection(1, 0);
            if (lx < -0.5f) MoveSelection(-1, 0);
            if (ly > 0.5f) MoveSelection(0, 1);
            if (ly < -0.5f) MoveSelection(0, -1);
        }
    }

    // Выбор символа (A)
    if (gamepad->IsButtonPressed(GP_A))
        SelectCurrentKey();

    // Удаление (B)
    if (gamepad->IsButtonPressed(GP_B))
        Backspace();

    // Удаление следующего символа (X)
    if (gamepad->IsButtonPressed(GP_X))
        DeleteChar();

    // Подтверждение (Start)
    if (gamepad->IsButtonPressed(GP_Start))
        Confirm();

    // Отмена (Back)
    if (gamepad->IsButtonPressed(GP_Back))
        Cancel();

    // Смена режима (Y)
    if (gamepad->IsButtonPressed(GP_Y))
        ToggleMode();
}

void ScreenKeyboard::MoveSelection(int dx, int dy)
{
    m_selectedCol += dx;
    m_selectedRow += dy;

    // Ограничиваем границы
    if (m_selectedCol < 0) m_selectedCol = 0;
    if (m_selectedCol >= COLS) m_selectedCol = COLS - 1;
    if (m_selectedRow < 0) m_selectedRow = 0;
    if (m_selectedRow >= ROWS) m_selectedRow = ROWS - 1;
}

void ScreenKeyboard::SelectCurrentKey()
{
    char ch = GetCharAt(m_selectedRow, m_selectedCol);
    if (ch == ' ')
        return;

    // Проверяем длину
    if (m_state.text.length() >= static_cast<size_t>(m_state.maxLength))
        return;

    // Добавляем символ
    std::string newChar(1, ch);
    
    // Применяем режим
    if (m_state.mode == SKM_Uppercase)
        newChar[0] = toupper(newChar[0]);

    m_state.text.insert(m_state.cursorPosition, newChar);
    m_state.cursorPosition++;
}

void ScreenKeyboard::DeleteChar()
{
    if (m_state.cursorPosition < static_cast<int>(m_state.text.length()))
    {
        m_state.text.erase(m_state.cursorPosition, 1);
    }
}

void ScreenKeyboard::Backspace()
{
    if (m_state.cursorPosition > 0)
    {
        m_state.cursorPosition--;
        m_state.text.erase(m_state.cursorPosition, 1);
    }
}

void ScreenKeyboard::Confirm()
{
    m_confirmed = true;
    Hide();
}

void ScreenKeyboard::Cancel()
{
    m_cancelled = true;
    Hide();
}

void ScreenKeyboard::ToggleMode()
{
    switch (m_state.mode)
    {
        case SKM_Normal:
            m_state.mode = SKM_Uppercase;
            break;
        case SKM_Uppercase:
            m_state.mode = SKM_Symbols;
            // TODO: Загрузить символьную раскладку
            m_state.mode = SKM_Normal;
            break;
        case SKM_Symbols:
            m_state.mode = SKM_Numbers;
            // TODO: Загрузить цифровую раскладку
            m_state.mode = SKM_Normal;
            break;
        case SKM_Numbers:
            m_state.mode = SKM_Normal;
            break;
    }
}

char ScreenKeyboard::GetCharAt(int row, int col) const
{
    if (row >= 0 && row < ROWS && col >= 0 && col < COLS)
        return m_layout[row][col];
    return ' ';
}

} // namespace Input
