#pragma once

#include <qobject.h>
#include <qthread.h>
#include <qimage.h>
#include <qmutex.h>
#include <qwaitcondition.h>
#include <qvideoframe.h>

#include "bufferreceiver.h"

struct AVCodecContext;
struct AVCodecParserContext;
struct AVPacket;
struct AVFrame;
struct SwsContext;

/**
 * @brief h264视频解码
 */
class VideoDecoder : public QThread {
    Q_OBJECT

public:
    explicit VideoDecoder(QObject *parent = nullptr);

    ~VideoDecoder() override;

    void appendBuffer(const QByteArray& buff);

    QImage convert(AVFrame* frame);

signals:
    void frameDecoded(const QVideoFrame& frame);
    void decoderProcessFailed(const QString& reason);

protected:
    void run() override;

private:
    bool codecInit(uint32_t codecId, int width, int height);
    void codecRelease();

    bool frameReceive();
    bool frameMerge();
    void frameUnpack();

private:
    AVCodecContext* codecCtx = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* decodeFrame = nullptr;
    SwsContext* swsContext = nullptr;

    uint8_t* mergeBuffer = nullptr;
    int mergedSize = 0;

    BufferReceiver bufferReceiver;
};