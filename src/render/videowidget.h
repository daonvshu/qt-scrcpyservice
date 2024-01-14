#pragma once

#include <qwidget.h>
#include <qopenglwidget.h>

#include "qpaintervideosurface_p.h"

class VideoWidget : public QOpenGLWidget {
public:
    explicit VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget();

    QPainterVideoSurface *videoSurface() const;

    Qt::AspectRatioMode aspectRatioMode() const;

    int brightness() const;
    int contrast() const;
    int hue() const;
    int saturation() const;

    QSize sizeHint() const override;
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

public:
    void setAspectRatioMode(Qt::AspectRatioMode mode);
    void setBrightness(int brightness) const;
    void setContrast(int contrast) const;
    void setHue(int hue) const;
    void setSaturation(int saturation) const;

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void formatChanged(const QVideoSurfaceFormat &format);
    void frameChanged();

private:
    void updateRects();

private:
    QPainterVideoSurface *m_surface;
    Qt::AspectRatioMode m_aspectRatioMode;
    QRect m_boundingRect;
    QRectF m_sourceRect;
    QSize m_nativeSize;
    bool m_updatePaintDevice;
};
