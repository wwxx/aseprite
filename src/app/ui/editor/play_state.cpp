// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/play_state.h"

#include "app/handle_anidir.h"
#include "app/loop_tag.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/scrolling_state.h"
#include "app/ui_context.h"
#include "ui/system.h"

namespace app {

using namespace ui;

PlayState::PlayState()
  : m_playTimer(10)
  , m_nextFrameTime(-1)
  , m_pingPongForward(true)
{
}

void PlayState::onAfterChangeState(Editor* editor)
{
  StateWithWheelBehavior::onAfterChangeState(editor);

  m_editor = editor;
  m_nextFrameTime = editor->sprite()->frameDuration(editor->frame());
  m_curFrameTick = ui::clock();
  m_pingPongForward = true;
  m_refFrame = editor->frame();

  m_playTimer.Tick.connect(&PlayState::onPlaybackTick, this);
  m_playTimer.start();
}

EditorState::BeforeChangeAction PlayState::onBeforeChangeState(Editor* editor, EditorState* newState)
{
  m_editor->setFrame(m_refFrame);
  m_playTimer.stop();
  return KeepState;
}

bool PlayState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  if (editor->hasCapture())
    return true;

  // When an editor is clicked the current view is changed.
  UIContext* context = UIContext::instance();
  context->setActiveView(editor->getDocumentView());

  // Start scroll loop
  EditorStatePtr newState(new ScrollingState());
  editor->setState(newState);
  newState->onMouseDown(editor, msg);
  return true;
}

bool PlayState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  editor->releaseMouse();
  return true;
}

bool PlayState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  editor->moveDrawingCursor();
  editor->updateStatusBar();
  return true;
}

bool PlayState::onKeyDown(Editor* editor, KeyMessage* msg)
{
  return false;
}

bool PlayState::onKeyUp(Editor* editor, KeyMessage* msg)
{
  return false;
}

void PlayState::onPlaybackTick()
{
  if (m_nextFrameTime < 0)
    return;

  m_nextFrameTime -= (ui::clock() - m_curFrameTick);

  doc::Sprite* sprite = m_editor->sprite();
  doc::FrameTag* tag = get_animation_tag(sprite, m_refFrame);

  while (m_nextFrameTime <= 0) {
    doc::frame_t frame = calculate_next_frame(
      sprite,
      m_editor->frame(), tag,
      m_pingPongForward);

    m_editor->setFrame(frame);
    m_nextFrameTime += m_editor->sprite()->frameDuration(frame);
  }

  m_curFrameTick = ui::clock();
  m_editor->invalidate();
}

} // namespace app
