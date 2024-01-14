#include "controlmsg.h"

#include "byteutil.h"

///// copy from control_msg.h /////

#define SC_CONTROL_MSG_MAX_SIZE (1 << 18) // 256k

#define SC_CONTROL_MSG_INJECT_TEXT_MAX_LENGTH 300
// type: 1 byte; sequence: 8 bytes; paste flag: 1 byte; length: 4 bytes
#define SC_CONTROL_MSG_CLIPBOARD_TEXT_MAX_LENGTH (SC_CONTROL_MSG_MAX_SIZE - 14)

#define POINTER_ID_MOUSE UINT64_C(-1)
#define POINTER_ID_GENERIC_FINGER UINT64_C(-2)

// Used for injecting an additional virtual pointer for pinch-to-zoom
#define POINTER_ID_VIRTUAL_MOUSE UINT64_C(-3)
#define POINTER_ID_VIRTUAL_FINGER UINT64_C(-4)

enum sc_control_msg_type {
    SC_CONTROL_MSG_TYPE_INJECT_KEYCODE,
    SC_CONTROL_MSG_TYPE_INJECT_TEXT,
    SC_CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT,
    SC_CONTROL_MSG_TYPE_INJECT_SCROLL_EVENT,
    SC_CONTROL_MSG_TYPE_BACK_OR_SCREEN_ON,
    SC_CONTROL_MSG_TYPE_EXPAND_NOTIFICATION_PANEL,
    SC_CONTROL_MSG_TYPE_EXPAND_SETTINGS_PANEL,
    SC_CONTROL_MSG_TYPE_COLLAPSE_PANELS,
    SC_CONTROL_MSG_TYPE_GET_CLIPBOARD,
    SC_CONTROL_MSG_TYPE_SET_CLIPBOARD,
    SC_CONTROL_MSG_TYPE_SET_SCREEN_POWER_MODE,
    SC_CONTROL_MSG_TYPE_ROTATE_DEVICE,
};

enum sc_screen_power_mode {
    // see <https://android.googlesource.com/platform/frameworks/base.git/+/pie-release-2/core/java/android/view/SurfaceControl.java#305>
    SC_SCREEN_POWER_MODE_OFF = 0,
    SC_SCREEN_POWER_MODE_NORMAL = 2,
};

enum sc_copy_key {
    SC_COPY_KEY_NONE,
    SC_COPY_KEY_COPY,
    SC_COPY_KEY_CUT,
};

/**
 * Convert a float between 0 and 1 to an unsigned 16-bit fixed-point value
 */
static inline uint16_t
sc_float_to_u16fp(float f) {
    assert(f >= 0.0f && f <= 1.0f);
    uint32_t u = f * 0x1p16f; // 2^16
    if (u >= 0xffff) {
        assert(u == 0x10000); // for f == 1.0f
        u = 0xffff;
    }
    return (uint16_t) u;
}

/**
 * Convert a float between -1 and 1 to a signed 16-bit fixed-point value
 */
static inline int16_t
sc_float_to_i16fp(float f) {
    assert(f >= -1.0f && f <= 1.0f);
    int32_t i = f * 0x1p15f; // 2^15
    assert(i >= -0x8000);
    if (i >= 0x7fff) {
        assert(i == 0x8000); // for f == 1.0f
        i = 0x7fff;
    }
    return (int16_t) i;
}
//////////


QByteArray ControlMsg::injectKeyCode(android_keyevent_action action, android_keycode keycode, uint32_t repeat,
                                     android_metastate metaState) {
    char bytes[14];
    bytes[0] = SC_CONTROL_MSG_TYPE_INJECT_KEYCODE;
    bytes[1] = action;
    ByteUtil::bitConvert(*(uint32_t*)(bytes + 2), &keycode);
    ByteUtil::bitConvert(*(uint32_t*)(bytes + 6), &repeat);
    ByteUtil::bitConvert(*(uint32_t*)(bytes + 10), &metaState);
    return { bytes, 14 };
}

QByteArray ControlMsg::injectText(const QString &text) {
    auto data = text.toUtf8();
    uint32_t len = data.size();
    ByteUtil::swapBits(len);
    data.prepend((const char*)&len, 4);
    data.prepend(SC_CONTROL_MSG_TYPE_INJECT_TEXT);
    return data;
}

QByteArray ControlMsg::injectTouchEvent(android_motionevent_action action, android_motionevent_buttons actionButton,
                                        android_motionevent_buttons buttons, uint64_t pointerId,
                                        const QSize &screenSize, const QPoint &point, float pressure) {
    char bytes[32];
    bytes[0] = SC_CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT;
    bytes[1] = action;
    ByteUtil::bitConvert(*(uint64_t*)(bytes + 2), &pointerId);
    uint32_t x = point.x();
    ByteUtil::bitConvert(*(uint32_t*)(bytes + 10), &x);
    uint32_t y = point.y();
    ByteUtil::bitConvert(*(uint32_t*)(bytes + 14), &y);
    uint16_t w = screenSize.width();
    ByteUtil::bitConvert(*(uint16_t*)(bytes + 18), &w);
    uint16_t h = screenSize.height();
    ByteUtil::bitConvert(*(uint16_t*)(bytes + 20), &h);
    uint16_t pressureValue = sc_float_to_u16fp(pressure);
    ByteUtil::bitConvert(*(uint16_t*)(bytes + 22), &pressureValue);
    ByteUtil::bitConvert(*(uint32_t*)(bytes + 24), &actionButton);
    ByteUtil::bitConvert(*(uint32_t*)(bytes + 28), &buttons);
    return { bytes, 32 };
}

QByteArray ControlMsg::injectScrollEvent(const QSize &screenSize, const QPoint &point, float hScroll, float vScroll,
                                         android_motionevent_buttons buttons) {

    char bytes[21];
    bytes[0] = SC_CONTROL_MSG_TYPE_INJECT_SCROLL_EVENT;
    uint32_t x = point.x();
    ByteUtil::bitConvert(*(uint32_t*)(bytes + 1), &x);
    uint32_t y = point.y();
    ByteUtil::bitConvert(*(uint32_t*)(bytes + 5), &y);
    uint16_t w = screenSize.width();
    ByteUtil::bitConvert(*(uint16_t*)(bytes + 9), &w);
    uint16_t h = screenSize.height();
    ByteUtil::bitConvert(*(uint16_t*)(bytes + 11), &h);
    int16_t hScrollValue = sc_float_to_i16fp(hScroll);
    int16_t vScrollValue = sc_float_to_i16fp(vScroll);
    ByteUtil::bitConvert(*(uint16_t*)(bytes + 13), &hScrollValue);
    ByteUtil::bitConvert(*(uint16_t*)(bytes + 15), &vScrollValue);
    ByteUtil::bitConvert(*(uint32_t*)(bytes + 17), &buttons);
    return { bytes, 21 };
}
