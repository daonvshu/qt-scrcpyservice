#include "scrcpyserver.h"

#include "videodecoder.h"
#include "adbcommandrunner.h"

#include <qdir.h>
#include <qdatetime.h>

ScrcpyServer::ScrcpyServer(QObject *parent)
    : QObject(parent)
{
    //tcp服务
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::acceptError, this, [] (QAbstractSocket::SocketError socketError) {
        qCritical() << "scrcpy server accept error:" << socketError;
    });
    connect(tcpServer, &QTcpServer::newConnection, this, &ScrcpyServer::handleNewConnection);
}

ScrcpyServer::~ScrcpyServer() {
    delete serverRunner;
}

QString ScrcpyServer::startAdbService(bool& ok) {
    QProcess process;
    process.start("adb/adb", {"start-server"});
    process.waitForFinished();
    if (process.exitCode() == 0 && process.exitStatus() == QProcess::NormalExit) {
        ok = true;
        return {};
    }
    ok = false;
    return process.readAll();
}

void ScrcpyServer::closeAdbService() {
    QProcess killTask;
    killTask.start("adb/adb", {"kill-server"});
    killTask.waitForFinished();
    qWarning() << "adb service closing...";
}

bool ScrcpyServer::start(const QString& devices, int port) {
    if (!tcpServer->isListening()) {
        bool success = tcpServer->listen(QHostAddress::AnyIPv4, port);
        if (!success) {
            emit initFailed("tcp server listen failed:" + tcpServer->errorString());
            return false;
        }
    }
    deviceAddress = devices;
    if (!connectToDevice()) {
        return false;
    }
    return true;
}

void ScrcpyServer::openStream(int maxFrameRate) {
    if (!pushServiceToDevice()) {
        return;
    }

    videoDecodeInit();
    createScrcpyServer(maxFrameRate);
}

void ScrcpyServer::closeStream() {
    if (serverRunner) {
        delete serverRunner;
        serverRunner = nullptr;
    }

    if (!deviceAddress.isEmpty()) {
        AdbCommandRunner runner;
        runner.runAdb({"-s", deviceAddress, "reverse", "--remove", "localabstract:scrcpy_" + scid});
    }

    //关闭解码器
    delete videoDecoder;
    videoDecoder = nullptr;
}

void ScrcpyServer::sendControl(const QByteArray &controlMsg) {
    if (controlSocket) {
        controlSocket->write(controlMsg);
    }
}

bool ScrcpyServer::connectToDevice() {
    AdbCommandRunner runner;
    runner.runAdb({"connect", deviceAddress});
    if (runner.lastFeedback.contains("cannot connect to")) {
        emit initFailed("connect device:" + deviceAddress + "failed, error:" + runner.getLastErr());
        return false;
    }
    qInfo() << "connect device:" << deviceAddress << "success!";
    return true;
}

bool ScrcpyServer::pushServiceToDevice() {
    auto scrcpyFilePath = QDir::currentPath() + "/scrcpy/scrcpy-server";
    qDebug() << "scrcpy path:" << scrcpyFilePath;

    AdbCommandRunner runner;
    runner.runAdb({"-s", deviceAddress, "push", scrcpyFilePath, "/data/local/tmp/scrcpy-server.jar"});
    if (!runner.lastFeedback.contains("1 file pushed")) {
        emit initFailed(runner.getLastErr());
        return false;
    }
    return true;
}

void ScrcpyServer::createScrcpyServer(int maxFrameRate) {
    scid = QString::asprintf("%08x", QDateTime::currentSecsSinceEpoch());

    AdbCommandRunner runner;
    runner.runAdb({"-s", deviceAddress, "reverse", "localabstract:scrcpy_" + scid, "tcp:" + QString::number(tcpServer->serverPort())});

    serverRunner = new AdbCommandRunner;
    QStringList scrcpyServiceOpt;
    scrcpyServiceOpt << "-s" << deviceAddress << "shell";
    scrcpyServiceOpt << "CLASSPATH=/data/local/tmp/scrcpy-server.jar";
    scrcpyServiceOpt << "app_process";
    scrcpyServiceOpt << "/";
    scrcpyServiceOpt << "com.genymobile.scrcpy.Server";
    scrcpyServiceOpt << SCRCPY_VERSION;
    scrcpyServiceOpt << "scid=" + scid;
    scrcpyServiceOpt << "audio=false";
    scrcpyServiceOpt << "max_fps=" + QString::number(maxFrameRate);
    scrcpyServiceOpt << "max_size=1920";
    serverRunner->runAdb(scrcpyServiceOpt, false);
}

void ScrcpyServer::videoDecodeInit() {
    videoDecoder = new VideoDecoder(this);
    connect(videoDecoder, &VideoDecoder::decoderProcessFailed, this, &ScrcpyServer::initFailed);
    connect(videoDecoder, &VideoDecoder::frameDecoded, this, &ScrcpyServer::getNewVideoFrame);
    videoDecoder->start();
}

void ScrcpyServer::handleNewConnection() {
    auto socket = tcpServer->nextPendingConnection();
    //第一个socket为视频流
    if (!videoSocket) {
        videoSocket = socket;
        connect(socket, &QTcpSocket::readyRead, this, &ScrcpyServer::receiveVideoBuffer);
        qInfo() << "video socket pending connect...";
    } else if (!controlSocket) {
        controlSocket = socket;
        connect(socket, &QTcpSocket::readyRead, this, &ScrcpyServer::receiveControlBuffer);
        qInfo() << "control socket pending connect...";
    } else {
        qWarning() << "unexpect socket appending...";
    }
    connect(socket, &QTcpSocket::stateChanged, this, [=] (QAbstractSocket::SocketState state) {
        qDebug() << "socket state changed:" << state;
        if (state == QAbstractSocket::UnconnectedState) {
            socket->deleteLater();
        }
    });
}

void ScrcpyServer::receiveVideoBuffer() {
    if (videoDecoder) {
        videoDecoder->appendBuffer(videoSocket->readAll());
    }
}

void ScrcpyServer::receiveControlBuffer() {

}
