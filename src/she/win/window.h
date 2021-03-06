// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_WIN_WINDOW_H_INCLUDED
#define SHE_WIN_WINDOW_H_INCLUDED
#pragma once

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "gfx/size.h"
#include "she/event.h"
#include "she/keys.h"

#ifndef WM_MOUSEHWHEEL
  #define WM_MOUSEHWHEEL 0x020E
#endif

namespace she {

  KeyScancode win32vk_to_scancode(int vk);

  #define SHE_WND_CLASS_NAME L"Aseprite.Window"

  template<typename T>
  class Window {
  public:
    Window() {
      registerClass();
      m_hwnd = createHwnd(this);
      m_hasMouse = false;
      m_captureMouse = false;
      m_scale = 1;
    }

    void queueEvent(Event& ev) {
      static_cast<T*>(this)->queueEventImpl(ev);
    }

    int scale() const {
      return m_scale;
    }

    void setScale(int scale) {
      m_scale = scale;
    }

    void setVisible(bool visible) {
      if (visible) {
        ShowWindow(m_hwnd, SW_SHOWNORMAL);
        UpdateWindow(m_hwnd);
        DrawMenuBar(m_hwnd);
      }
      else
        ShowWindow(m_hwnd, SW_HIDE);
    }

    void maximize() {
      ShowWindow(m_hwnd, SW_MAXIMIZE);
    }

    bool isMaximized() const {
      return (IsZoomed(m_hwnd) ? true: false);
    }

    gfx::Size clientSize() const {
      return m_clientSize;
    }

    gfx::Size restoredSize() const {
      return m_restoredSize;
    }

    void setText(const std::string& text) {
      SetWindowText(m_hwnd, base::from_utf8(text).c_str());
    }

    void captureMouse() {
      m_captureMouse = true;
    }

    void releaseMouse() {
      m_captureMouse = false;
    }

    void invalidate() {
      InvalidateRect(m_hwnd, NULL, FALSE);
    }

    HWND handle() {
      return m_hwnd;
    }

  private:
    LRESULT wndProc(UINT msg, WPARAM wparam, LPARAM lparam) {
      switch (msg) {

        case WM_CLOSE: {
          Event ev;
          ev.setType(Event::CloseDisplay);
          queueEvent(ev);
          break;
        }

        case WM_PAINT: {
          PAINTSTRUCT ps;
          HDC hdc = BeginPaint(m_hwnd, &ps);
          static_cast<T*>(this)->paintImpl(hdc);
          EndPaint(m_hwnd, &ps);
          return true;
        }

        case WM_SIZE: {
          switch (wparam) {
            case SIZE_MAXIMIZED:
            case SIZE_RESTORED:
              m_clientSize.w = GET_X_LPARAM(lparam);
              m_clientSize.h = GET_Y_LPARAM(lparam);

              static_cast<T*>(this)->resizeImpl(m_clientSize);
              break;
          }

          WINDOWPLACEMENT pl;
          pl.length = sizeof(pl);
          if (GetWindowPlacement(m_hwnd, &pl)) {
            m_restoredSize = gfx::Size(
              pl.rcNormalPosition.right - pl.rcNormalPosition.left,
              pl.rcNormalPosition.bottom - pl.rcNormalPosition.top);
          }
          break;
        }

        case WM_MOUSEMOVE: {
          // Adjust capture
          if (m_captureMouse) {
            if (GetCapture() != m_hwnd)
              SetCapture(m_hwnd);
          }
          else {
            if (GetCapture() == m_hwnd)
              ReleaseCapture();
          }

          Event ev;
          ev.setPosition(gfx::Point(
              GET_X_LPARAM(lparam) / m_scale,
              GET_Y_LPARAM(lparam) / m_scale));

          if (!m_hasMouse) {
            m_hasMouse = true;

            ev.setType(Event::MouseEnter);
            queueEvent(ev);

            // Track mouse to receive WM_MOUSELEAVE message.
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = m_hwnd;
            _TrackMouseEvent(&tme);
          }

          ev.setType(Event::MouseMove);
          queueEvent(ev);
          break;
        }

        case WM_NCMOUSEMOVE:
        case WM_MOUSELEAVE:
          if (m_hasMouse) {
            m_hasMouse = false;

            Event ev;
            ev.setType(Event::MouseLeave);
            queueEvent(ev);
          }
          break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN: {
          Event ev;
          ev.setType(Event::MouseDown);
          ev.setPosition(gfx::Point(
              GET_X_LPARAM(lparam) / m_scale,
              GET_Y_LPARAM(lparam) / m_scale));
          ev.setButton(
            msg == WM_LBUTTONDOWN ? Event::LeftButton:
            msg == WM_RBUTTONDOWN ? Event::RightButton:
            msg == WM_MBUTTONDOWN ? Event::MiddleButton: Event::NoneButton);
          queueEvent(ev);
          break;
        }

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP: {
          Event ev;
          ev.setType(Event::MouseUp);
          ev.setPosition(gfx::Point(
              GET_X_LPARAM(lparam) / m_scale,
              GET_Y_LPARAM(lparam) / m_scale));
          ev.setButton(
            msg == WM_LBUTTONUP ? Event::LeftButton:
            msg == WM_RBUTTONUP ? Event::RightButton:
            msg == WM_MBUTTONUP ? Event::MiddleButton: Event::NoneButton);
          queueEvent(ev);

          // Avoid popup menu for scrollbars
          if (msg == WM_RBUTTONUP)
            return 0;

          break;
        }

        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK: {
          Event ev;
          ev.setType(Event::MouseDoubleClick);
          ev.setPosition(gfx::Point(
              GET_X_LPARAM(lparam) / m_scale,
              GET_Y_LPARAM(lparam) / m_scale));
          ev.setButton(
            msg == WM_LBUTTONDBLCLK ? Event::LeftButton:
            msg == WM_RBUTTONDBLCLK ? Event::RightButton:
            msg == WM_MBUTTONDBLCLK ? Event::MiddleButton: Event::NoneButton);
          queueEvent(ev);
          break;
        }

        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL: {
          RECT rc;
          ::GetWindowRect(m_hwnd, &rc);

          Event ev;
          ev.setType(Event::MouseWheel);
          ev.setPosition((gfx::Point(
                GET_X_LPARAM(lparam),
                GET_Y_LPARAM(lparam)) - gfx::Point(rc.left, rc.top))
            / m_scale);

          int z = ((short)HIWORD(wparam)) / WHEEL_DELTA;
          gfx::Point delta(
            (msg == WM_MOUSEHWHEEL ? z: 0),
            (msg == WM_MOUSEWHEEL ? -z: 0));
          ev.setWheelDelta(delta);

          //PRINTF("WHEEL: %d %d\n", delta.x, delta.y);

          queueEvent(ev);
          break;
        }

        case WM_HSCROLL:
        case WM_VSCROLL: {
          RECT rc;
          ::GetWindowRect(m_hwnd, &rc);

          POINT pos;
          ::GetCursorPos(&pos);

          Event ev;
          ev.setType(Event::MouseWheel);
          ev.setPosition((gfx::Point(pos.x, pos.y) - gfx::Point(rc.left, rc.top))
            / m_scale);

          int bar = (msg == WM_HSCROLL ? SB_HORZ: SB_VERT);
          int z = GetScrollPos(m_hwnd, bar);

          switch (LOWORD(wparam)) {
            case SB_LEFT:
            case SB_LINELEFT:
              --z;
              break;
            case SB_PAGELEFT:
              z -= 2;
              break;
            case SB_RIGHT:
            case SB_LINERIGHT:
              ++z;
              break;
            case SB_PAGERIGHT:
              z += 2;
              break;
            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
            case SB_ENDSCROLL:
              // Do nothing
              break;
          }

          gfx::Point delta(
            (msg == WM_HSCROLL ? (z-50): 0),
            (msg == WM_VSCROLL ? (z-50): 0));
          ev.setWheelDelta(delta);

          //PRINTF("SCROLL: %d %d\n", delta.x, delta.y);

          SetScrollPos(m_hwnd, bar, 50, FALSE);

          queueEvent(ev);
          break;
        }

        case WM_KEYDOWN: {
          Event ev;
          ev.setType(Event::KeyDown);
          ev.setScancode(win32vk_to_scancode(wparam));
          ev.setUnicodeChar(0);  // TODO
          ev.setRepeat(lparam & 15);
          queueEvent(ev);
          break;
        }

        case WM_KEYUP: {
          Event ev;
          ev.setType(Event::KeyUp);
          ev.setScancode(win32vk_to_scancode(wparam));
          ev.setUnicodeChar(0);  // TODO
          ev.setRepeat(lparam & 15);
          queueEvent(ev);
          break;
        }

        case WM_UNICHAR: {
          Event ev;
          ev.setType(Event::KeyUp);
          ev.setScancode(kKeyNil);    // TODO
          ev.setUnicodeChar(wparam);
          ev.setRepeat(lparam & 15);
          queueEvent(ev);
          break;
        }

        case WM_DROPFILES: {
          HDROP hdrop = (HDROP)(wparam);
          Event::Files files;

          int count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
          for (int index=0; index<count; ++index) {
            int length = DragQueryFile(hdrop, index, NULL, 0);
            if (length > 0) {
              std::vector<TCHAR> str(length+1);
              DragQueryFile(hdrop, index, &str[0], str.size());
              files.push_back(base::to_utf8(&str[0]));
            }
          }

          DragFinish(hdrop);

          Event ev;
          ev.setType(Event::DropFiles);
          ev.setFiles(files);
          queueEvent(ev);
          break;
        }

      }

      return DefWindowProc(m_hwnd, msg, wparam, lparam);
    }

    static void registerClass() {
      HMODULE instance = GetModuleHandle(nullptr);

      WNDCLASSEX wcex;
      if (GetClassInfoEx(instance, SHE_WND_CLASS_NAME, &wcex))
        return;                 // Already registered

      wcex.cbSize        = sizeof(WNDCLASSEX);
      wcex.style         = 0;
      wcex.lpfnWndProc   = &Window::staticWndProc;
      wcex.cbClsExtra    = 0;
      wcex.cbWndExtra    = 0;
      wcex.hInstance     = instance;
      wcex.hIcon         = nullptr;
      wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
      wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
      wcex.lpszMenuName  = nullptr;
      wcex.lpszClassName = SHE_WND_CLASS_NAME;
      wcex.hIconSm       = nullptr;

      if (RegisterClassEx(&wcex) == 0)
        throw std::runtime_error("Error registering window class");
    }

    static HWND createHwnd(Window* self) {
      HWND hwnd = CreateWindowEx(
        WS_EX_APPWINDOW | WS_EX_ACCEPTFILES,
        SHE_WND_CLASS_NAME,
        L"",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        LPVOID(self));
      if (!hwnd)
        return nullptr;

      SetWindowLongPtr(hwnd, GWLP_USERDATA, LONG_PTR(self));
      return hwnd;
    }

    static LRESULT CALLBACK staticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
      Window* wnd;

      if (msg == WM_CREATE)
        wnd = (Window*)lparam;
      else
        wnd = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

      if (wnd)
        return wnd->wndProc(msg, wparam, lparam);
      else
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    mutable HWND m_hwnd;
    gfx::Size m_clientSize;
    gfx::Size m_restoredSize;
    int m_scale;
    bool m_hasMouse;
    bool m_captureMouse;
  };

} // namespace she

#endif
