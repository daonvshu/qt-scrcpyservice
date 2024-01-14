#include "bufferreceiver.h"

BufferReceiver::BufferReceiver(QObject *parent)
    : QObject(parent)
    , endBufferCache(false)
{}

void BufferReceiver::sendBuffer(const QByteArray &data) {
    QMutexLocker locker(&mutex);
    receiveBuffer.append(data);
    receiveWait.notify_all();
}

void BufferReceiver::endCache() {
    QMutexLocker locker(&mutex);
    endBufferCache = true;
    receiveWait.notify_all();
}

void BufferReceiver::receive(void *data, int len) {
    mutex.lock();
    if (endBufferCache) {
        mutex.unlock();
        return;
    }
    while (receiveBuffer.size() < len && !endBufferCache) {
        receiveWait.wait(&mutex);
    }
    if (!endBufferCache) {
        memcpy(data, receiveBuffer.data(), len);
        receiveBuffer = receiveBuffer.mid(len);
    }
    mutex.unlock();
}