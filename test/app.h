#pragma once

#include <qwidget.h>

#include "ui_app.h"

#include "scrcpyserver.h"

class App : public QWidget {
    Q_OBJECT

public:
    explicit App(QWidget *parent = nullptr);
    ~App();

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::App ui;

    ScrcpyServer* scrcpyServer;
    QSize frameSrcSize;
    QSizeF framePixmapRatio;

private slots:
    void on_btn_connect_clicked();
    void on_btn_open_stream_clicked();
    void on_btn_close_stream_clicked();
};