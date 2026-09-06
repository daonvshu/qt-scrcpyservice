#include "videodecoder.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <qdatetime.h>
#include <qdebug.h>

#define SC_CODEC_ID_H264 UINT32_C(0x68323634) // "h264" in ASCII

//scrcpy 4.x协议：包头固定12字节
#define SC_PACKET_HEADER_SIZE 12

#define SC_PACKET_FLAG_SESSION   (UINT64_C(1) << 63)
#define SC_PACKET_FLAG_CONFIG    (UINT64_C(1) << 62)
#define SC_PACKET_FLAG_KEY_FRAME (UINT64_C(1) << 61)

#define SC_PACKET_PTS_MASK (SC_PACKET_FLAG_KEY_FRAME - 1)

namespace {
uint32_t readBE32(const uint8_t* data) {
    return (uint32_t(data[0]) << 24) | (uint32_t(data[1]) << 16) | (uint32_t(data[2]) << 8) | uint32_t(data[3]);
}

uint64_t readBE64(const uint8_t* data) {
    return (uint64_t(readBE32(data)) << 32) | readBE32(data + 4);
}
}

VideoDecoder::VideoDecoder(QObject *parent)
    : QThread(parent)
{}

VideoDecoder::~VideoDecoder() {
    bufferReceiver.endCache();
    wait();
}

void VideoDecoder::appendBuffer(const QByteArray &buff) {
    bufferReceiver.sendBuffer(buff);
}

QImage VideoDecoder::convert(AVFrame *frame) {
    QImage image = QImage(codecCtx->width, codecCtx->height, QImage::Format_RGB888);
    auto imagePtr = image.bits();
    //将YUV420p转换为RGB24
    const int lineSize[4] = {3*codecCtx->width, 0, 0, 0};
    sws_scale(swsContext, (const uint8_t* const*)frame->data, frame->linesize,
              0, codecCtx->height, (uint8_t**)&imagePtr, lineSize);
    return image;
}

void VideoDecoder::run() {
    QByteArray remoteDeviceName(64, '\0');
    bufferReceiver.receive(remoteDeviceName.data(), remoteDeviceName.size());
    auto name = QString::fromUtf8(remoteDeviceName);
    if (!name.isEmpty()) {
        qInfo() << "device name received:" << name;
    }
    if (bufferReceiver.isEndReceive()) {
        return;
    }

    //4.x协议：视频头只有4字节codec id，宽高由后续的session包下发
    auto codecId = bufferReceiver.receive<uint32_t>();
    if (bufferReceiver.isEndReceive()) {
        return;
    }

    qInfo() << "video decode is running...";
    for (;;) {
        uint8_t header[SC_PACKET_HEADER_SIZE];
        bufferReceiver.receive(header, SC_PACKET_HEADER_SIZE);
        if (bufferReceiver.isEndReceive()) {
            break;
        }

        if (header[0] & 0x80) {
            //session包：流开始或分辨率变化（旋转等）时下发，仅12字节头，无数据负载
            auto width = readBE32(header + 4);
            auto height = readBE32(header + 8);
            if (!updateVideoSize(codecId, width, height)) {
                break;
            }
            continue;
        }

        if (!frameReceive(header)) {
            break;
        }
        if (!frameMerge()) {
            av_packet_unref(packet);
            break;
        }
        frameUnpack();
        av_packet_unref(packet);
    }

    //释放资源
    codecRelease();

    qInfo() << "video decoder exit...";
}

bool VideoDecoder::updateVideoSize(uint32_t codecId, uint32_t width, uint32_t height) {
    if (codecCtx == nullptr) {
        //首个session包，初始化解码器
        if (!codecInit(codecId, int(width), int(height))) {
            qCritical() << "video decode init failed!";
            return false;
        }
        return true;
    }

    if (codecCtx->width == int(width) && codecCtx->height == int(height)) {
        return true;
    }

    //分辨率变化，重建转换上下文
    codecCtx->width = int(width);
    codecCtx->height = int(height);
    sws_freeContext(swsContext);
    swsContext = sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
                                codecCtx->width, codecCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR,
                                nullptr, nullptr, nullptr);
    if (!swsContext) {
        emit decoderProcessFailed("sws get context fail!");
        return false;
    }
    qInfo() << "video size changed:" << width << "x" << height;
    return true;
}

bool VideoDecoder::codecInit(uint32_t codecId, int width, int height) {
    if (codecId != SC_CODEC_ID_H264) {
        emit decoderProcessFailed("stream code id not h264!");
        return false;
    }
    auto codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        emit decoderProcessFailed("find codec h264 fail!");
        return false;
    }

    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        emit decoderProcessFailed("allocate codec context fail!");
        return false;
    }

    codecCtx->width = width;
    codecCtx->height = height;
    codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    int ret = avcodec_open2(codecCtx, codec, nullptr);
    if (ret < 0) {
        emit decoderProcessFailed("open codec fail!");
        return false;
    }

    packet = av_packet_alloc();
    if (!packet) {
        emit decoderProcessFailed("alloc packet fail!");
        return false;
    }

    decodeFrame = av_frame_alloc();
    if (!decodeFrame) {
        emit decoderProcessFailed("alloc frame fail!");
        return false;
    }

    swsContext = sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
                                codecCtx->width, codecCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR,
                                nullptr, nullptr, nullptr);
    if (!swsContext) {
        emit decoderProcessFailed("sws get context fail!");
        return false;
    }

    return true;
}

void VideoDecoder::codecRelease() {
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
    }
    if (packet) {
        av_packet_free(&packet);
    }
    if (decodeFrame) {
        av_frame_free(&decodeFrame);
    }
    if (swsContext) {
        sws_freeContext(swsContext);
    }
    free(mergeBuffer);
}

bool VideoDecoder::frameReceive(const uint8_t* header) {
    auto ptsFlags = readBE64(header);
    auto frameLen = int32_t(readBE32(header + 8));
    Q_ASSERT(frameLen != 0);

    if (av_new_packet(packet, frameLen)) {
        emit decoderProcessFailed("av new packet failed!");
        return false;
    }
    bufferReceiver.receive(packet->data, frameLen);
    if (bufferReceiver.isEndReceive()) {
        return false;
    }

    if (ptsFlags & SC_PACKET_FLAG_CONFIG) {
        packet->pts = AV_NOPTS_VALUE;
    } else {
        packet->pts = ptsFlags & SC_PACKET_PTS_MASK;
    }

    if (ptsFlags & SC_PACKET_FLAG_KEY_FRAME) {
        packet->flags |= AV_PKT_FLAG_KEY;
    }
    packet->dts = packet->pts;
    return true;
}

bool VideoDecoder::frameMerge() {
    bool isConfig = packet->pts == AV_NOPTS_VALUE;
    if (isConfig) {
        free(mergeBuffer);
        mergeBuffer = (uint8_t*)malloc(packet->size);
        if (!mergeBuffer) {
            emit decoderProcessFailed("merge buffer malloc failed! required size:" + QString::number(packet->size));
            return false;
        }
        memcpy(mergeBuffer, packet->data, packet->size);
        mergedSize = packet->size;
    }
    else if (mergeBuffer) {
        if (av_grow_packet(packet, mergedSize)) {
            emit decoderProcessFailed("av grow packet failed!");
            return false;
        }
        memmove(packet->data + mergedSize, packet->data, packet->size);
        memcpy(packet->data, mergeBuffer, mergedSize);

        free(mergeBuffer);
        mergeBuffer = nullptr;
    }
    return true;
}

void VideoDecoder::frameUnpack() {
    if (packet->pts == AV_NOPTS_VALUE) {
        return;
    }
    int ret = avcodec_send_packet(codecCtx, packet);
    if (ret < 0 && ret != AVERROR(EAGAIN)) {
        qCritical() << "send packet error:" << ret;
    } else {
        //循环解析数据帧
        for (;;) {
            ret = avcodec_receive_frame(codecCtx, decodeFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }

            if (ret) {
                qCritical() << "could not receive video frame:" << ret;
                break;
            }

            QVideoFrame cachedFrame(codecCtx->width * codecCtx->height * 3 / 2,
                                    QSize(codecCtx->width, codecCtx->height),
                                    codecCtx->width, QVideoFrame::Format_YUV420P);
            int imageSize = av_image_get_buffer_size(codecCtx->pix_fmt, codecCtx->width, codecCtx->height, 1);
            if (cachedFrame.map(QAbstractVideoBuffer::WriteOnly)) {
                uchar *dstData = cachedFrame.bits();
                av_image_copy_to_buffer(dstData, imageSize, decodeFrame->data, decodeFrame->linesize,
                                        codecCtx->pix_fmt,
                                        codecCtx->width, codecCtx->height, 1);
                cachedFrame.unmap();

                emit frameDecoded(cachedFrame);
            }
            av_frame_unref(decodeFrame);
        }
    }
}
