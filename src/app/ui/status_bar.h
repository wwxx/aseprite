// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_STATUS_BAR_H_INCLUDED
#define APP_UI_STATUS_BAR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "base/observers.h"
#include "doc/layer_index.h"
#include "ui/base.h"
#include "ui/widget.h"

#include <string>
#include <vector>

namespace ui {
  class Box;
  class Button;
  class Entry;
  class Label;
  class Slider;
  class Window;
}

namespace app {
  class ButtonSet;
  class Editor;
  class StatusBar;

  namespace tools {
    class Tool;
  }

  class StatusBar : public ui::Widget {
    static StatusBar* m_instance;
  public:
    static StatusBar* instance() { return m_instance; }

    StatusBar();
    ~StatusBar();

    void clearText();

    bool setStatusText(int msecs, const char *format, ...);
    void showTip(int msecs, const char *format, ...);
    void showColor(int msecs, const char* text, const Color& color, int alpha);
    void showTool(int msecs, tools::Tool* tool);

    void updateUsingEditor(Editor* editor);

  protected:
    void onResize(ui::ResizeEvent& ev) override;
    void onPreferredSize(ui::PreferredSizeEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;

  private:
    void onCurrentToolChange();
    void onCelOpacityChange();
    void updateFromDocument(Editor* editor);
    void updateCurrentFrame(Editor* editor);
    void newFrame();

    enum State { SHOW_TEXT, SHOW_COLOR, SHOW_TOOL };

    int m_timeout;
    State m_state;

    // Showing a tool
    tools::Tool* m_tool;

    // Showing a color
    Color m_color;
    int m_alpha;

    // Box of main commands
    ui::Widget* m_commandsBox;
    ui::Label* m_frameLabel;
    ui::Slider* m_slider;             // Opacity slider
    ui::Entry* m_currentFrame;        // Current frame and go to frame entry
    ui::Button* m_newFrame;           // Button to create a new frame
    bool m_hasDoc;

    // Tip window
    class CustomizedTipWindow;
    CustomizedTipWindow* m_tipwindow;
  };

} // namespace app

#endif
