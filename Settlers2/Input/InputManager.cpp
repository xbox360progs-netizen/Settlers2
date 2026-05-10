#include "StdAfx.h"
#include "InputManager.h"

namespace Input {

InputManager::InputManager()
    : m_hWnd(NULL)
    , m_device(NULL)
    , m_gamepad(NULL)
{
}

InputManager::~InputManager()
{
    Shutdown();
}

bool InputManager::Initialize(HWND hWnd, IDirect3DDevice9* device)
{
    m_hWnd = hWnd;
    m_device = device;
    if (device)
        device->AddRef();

    m_gamepad = new Gamepad();


    // Инициализируем геймпад (работает на Xbox и Windows)
    m_gamepad->Initialize(0);

    return true;
}

void InputManager::Shutdown()
{
    if (m_gamepad)
    {
        m_gamepad->Shutdown();
        delete m_gamepad;
        m_gamepad = NULL;
    }

    if (m_device)
    {
        m_device->Release();
        m_device = NULL;
    }

    m_hWnd = NULL;
}

void InputManager::Update()
{

    if (m_gamepad)
        m_gamepad->Update();
}




void InputManager::Clear()
{


    if (m_gamepad)
        m_gamepad->Clear();
}

} // namespace Input
