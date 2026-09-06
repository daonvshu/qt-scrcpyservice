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
        //分辨率跟随实际帧（session包下发，旋转时会变化），不能写死
        if (frameSrcSize != frame.size()) {
            frameSrcSize = frame.size();
            auto surface = ui.video_display->videoSurface();
            if (surface->isActive()) {
                surface->stop();
            }
            if (!surface->start(QVideoSurfaceFormat(frameSrcSize, QVideoFrame::Format_YUV420P))) {
                qDebug() << "start surface failed!" << surface->error();
                on_btn_close_stream_clicked();
                return;
            }
            qDebug() << "video surface started, frame size:" << frameSrcSize;
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
    //surface在收到第一帧时按实际分辨率启动
    scrcpyServer->openStream(ui.input_max_rate->value());
    frameSrcSize = QSize();
}

void App::on_btn_close_stream_clicked() {
    scrcpyServer->closeStream();
    ui.video_display->videoSurface()->stop();
}

bool App::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui.video_display && frameSrcSize.isValid()) {
        if (auto mouseEvent = dynamic_cast<QMouseEvent*>(event)) {
            //与VideoWidget::updateRects的KeepAspectRatio逻辑保持一致：
            //帧等比缩放到widget内并居中显示，坐标映射需加上居中偏移
            QSize scaledSize = frameSrcSize;
            scaledSize.scale(ui.video_display->size(), Qt::KeepAspectRatio);
            QRect boundingRect(QPoint(0, 0), scaledSize);
            boundingRect.moveCenter(ui.video_display->rect().center());

            auto dstPos = QPoint(
                qRound((mouseEvent->pos().x() - boundingRect.left()) * frameSrcSize.width() * 1.0 / boundingRect.width()),
                qRound((mouseEvent->pos().y() - boundingRect.top()) * frameSrcSize.height() * 1.0 / boundingRect.height()));
            //落在显示区域外（黑边）时裁剪到屏幕边缘
            dstPos.setX(qBound(0, dstPos.x(), frameSrcSize.width() - 1));
            dstPos.setY(qBound(0, dstPos.y(), frameSrcSize.height() - 1));

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
