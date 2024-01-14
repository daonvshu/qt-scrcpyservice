#pragma once

#include <qobject.h>
#include <qtcpserver.h>
#include <qtcpsocket.h>
#include <qpointer.h>
#include <qvideoframe.h>

#include "controlmsg.h"

class VideoDecoder;
class AdbCommandRunner;

class ScrcpyServer : public QObject {
    Q_OBJECT

public:
    explicit ScrcpyServer(QObject *parent = nullptr);
    ~ScrcpyServer() override;

    /**
     * @brief 开启服务
     * @param devices 设备地址，包括adb连接端口号（如果设置）
     * @param port scrcpy服务监听端口，非adb连接端口
     */
    bool start(const QString& devices, int port = 27183);

    /**
     * @brief 打开视频推流
     * @param maxFrameRate
     */
    void openStream(int maxFrameRate = 120);

    /**
     * @brief 关闭视频推流
     */
    void closeStream();

    /**
     * @brief 发送控制命令
     * @param controlMsg
     */
    void sendControl(const QByteArray& controlMsg);

    /**
     * @brief 开启adb服务
     * @param ok
     * @return
     */
    static QString startAdbService(bool& ok);

    /**
     * @brief 关闭adb服务
     */
    static void closeAdbService();

signals:
    void initFailed(const QString& error);
    void getNewVideoFrame(const QVideoFrame& frame);

private:
    QTcpServer* tcpServer;
    QPointer<QTcpSocket> videoSocket;
    QPointer<QTcpSocket> controlSocket;

    VideoDecoder* videoDecoder = nullptr;

    QString deviceAddress;
    QString scid;
    AdbCommandRunner* serverRunner = nullptr;

private:
    /**
     * @brief 使用adb连接到设备
     * @return
     */
    bool connectToDevice();

    /**
     * @brief 推送服务到设备
     * @return
     */
    bool pushServiceToDevice();

    /**
     * @brief 在设备端启动服务
     * @param maxFrameRate
     */
    void createScrcpyServer(int maxFrameRate);

    /**
     * @brief 接收scrcpy服务socket
     */
    void handleNewConnection();

    /**
     * @brief 初始化解码器
     */
    void videoDecodeInit();

    /**
     * @brief 处理视频流
     */
    void receiveVideoBuffer();

    /**
     * @brief 处理控制数据流
     */
    void receiveControlBuffer();
};
