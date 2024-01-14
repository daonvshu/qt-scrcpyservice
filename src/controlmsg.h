#pragma once

#include <qobject.h>
#include <qpoint.h>
#include <qsize.h>

#include "android/keycodes.h"
#include "android/input.h"

class ControlMsg {
public:
    static QByteArray injectKeyCode(android_keyevent_action action, android_keycode keycode,
                                    uint32_t repeat, android_metastate metaState);

    static QByteArray injectText(const QString& text);

    static QByteArray injectTouchEvent(android_motionevent_action action, android_motionevent_buttons actionButton,
                                       android_motionevent_buttons buttons, uint64_t pointerId,
                                       const QSize& screenSize, const QPoint& point, float pressure);

    static QByteArray injectScrollEvent(const QSize& screenSize, const QPoint& point, float hScroll, float vScroll,
                                        android_motionevent_buttons buttons);
};
