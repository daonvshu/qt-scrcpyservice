#include "app.h"

#include <qdebug.h>
#include <qthread.h>
#include <qevent.h>
#include <qvideosurfaceformat.h>

App::App(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    ui.video_display->installEventFilter(this);
    scrcpyServer = new ScrcpyServer(this);
    connect(scrcpyServer, &ScrcpyServer::initFailed, this, [] (const QString& error) {
        qCritical() << error;
    });
    connect(scrcpyServer, &ScrcpyServer::getNewVideoFrame, this, [&] (const QVideoFrame& frame) {
        if (!frameSrcSize.isValid()) {
            frameSrcSize = frame.size();
            auto targetImage = QSize(0, ui.video_display->height());
            targetImage.setWidth(qRound(targetImage.height() * frameSrcSize.width() * 1.0 / frameSrcSize.height()));
            framePixmapRatio = QSizeF(frameSrcSize.width() * 1.0 / targetImage.width(),
                                      frameSrcSize.height() * 1.0 / targetImage.height());
        }
        if (!ui.video_display->videoSurface()->present(frame)) {
            qDebug() << "present frame failed!" << ui.video_display->videoSurface()->error();
            on_btn_close_stream_clicked();
        }
    });

    QThread::create([] {
        bool ok;
        auto message = ScrcpyServer::startAdbService(ok);
        if (ok) {
            qDebug() << "adb server start finished!";
        } else {
            qDebug() << "adb server start failed:" << message;
        }
    })->start();
}

App::~App() {
    ScrcpyServer::closeAdbService();
}

void App::on_btn_connect_clicked() {
    auto address = ui.input_address->text();
    if (scrcpyServer->start(address)) {
        ui.btn_connect->setText(u8"已连接");
    }
}

void App::on_btn_open_stream_clicked() {
    ui.video_display->videoSurface()->setShaderType(QPainterVideoSurface::GlslShader);
    ui.video_display->videoSurface()->start(QVideoSurfaceFormat(QSize(1920, 1080), QVideoFrame::Format_YUV420P));
    scrcpyServer->openStream(ui.input_max_rate->value());
    frameSrcSize = QSize();
}

void App::on_btn_close_stream_clicked() {
    scrcpyServer->closeStream();
    ui.video_display->videoSurface()->stop();
}

bool App::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui.video_display) {
        if (auto mouseEvent = dynamic_cast<QMouseEvent*>(event)) {
            auto dstPos = QPoint(qRound(mouseEvent->x() * framePixmapRatio.width()), qRound(mouseEvent->y() * framePixmapRatio.height()));
            if (mouseEvent->type() == QEvent::MouseButtonPress) {
                scrcpyServer->sendControl(ControlMsg::injectTouchEvent(AMOTION_EVENT_ACTION_DOWN, AMOTION_EVENT_BUTTON_PRIMARY,
                                                                       AMOTION_EVENT_BUTTON_PRIMARY, 0,
                                                                       frameSrcSize, dstPos, 1.0));
            } else if (mouseEvent->type() == QEvent::MouseButtonRelease) {
                scrcpyServer->sendControl(ControlMsg::injectTouchEvent(AMOTION_EVENT_ACTION_UP, AMOTION_EVENT_BUTTON_PRIMARY,
                                                                       AMOTION_EVENT_BUTTON_PRIMARY, 0,
                                                                       frameSrcSize, dstPos, 0.0));
            } else if (mouseEvent->type() == QEvent::MouseMove) {
                scrcpyServer->sendControl(ControlMsg::injectTouchEvent(AMOTION_EVENT_ACTION_MOVE, AMOTION_EVENT_BUTTON_PRIMARY,
                                                                       AMOTION_EVENT_BUTTON_PRIMARY, 0,
                                                                       frameSrcSize, dstPos, 1.0));
            }
        }
    }
    return QObject::eventFilter(watched, event);
}
