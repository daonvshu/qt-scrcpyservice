#pragma once

#include <qobject.h>
#include <qmutex.h>
#include <qwaitcondition.h>

#include "byteutil.h"

class BufferReceiver : public QObject {
public:
    explicit BufferReceiver(QObject *parent = nullptr);

    void sendBuffer(const QByteArray& data);

    void endCache();

    template<typename T>
    T receive() {
        enum {
            T_Size = sizeof(T)
        };

        T value = T();
        receive((void*)&value, T_Size);
        ByteUtil::swapBits(value);
        return value;
    }

    void receive(void* data, int len);

    bool isEndReceive() const {
        return endBufferCache;
    }

private:
    QByteArray receiveBuffer;
    QMutex mutex;
    QWaitCondition receiveWait;

    bool endBufferCache;
};
