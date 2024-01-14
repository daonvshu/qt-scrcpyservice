#include "videowidget.h"

#include <qevent.h>
#include <qvideosurfaceformat.h>

VideoWidget::VideoWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_aspectRatioMode(Qt::KeepAspectRatio)
    , m_updatePaintDevice(true)
{
    m_surface = new QPainterVideoSurface(this);

    connect(m_surface, &QPainterVideoSurface::frameChanged, this, &VideoWidget::frameChanged);
    connect(m_surface, &QPainterVideoSurface::surfaceFormatChanged, this, &VideoWidget::formatChanged);
}

QPainterVideoSurface *VideoWidget::videoSurface() const {
    return m_surface;
}

VideoWidget::~VideoWidget() {
    delete m_surface;
}

Qt::AspectRatioMode VideoWidget::aspectRatioMode() const
{
    return m_aspectRatioMode;
}

void VideoWidget::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    m_aspectRatioMode = mode;
    updateGeometry();
}

int VideoWidget::brightness() const
{
    return m_surface->brightness();
}

void VideoWidget::setBrightness(int brightness) const
{
    int boundedBrightness = qBound(-100, brightness, 100);
    m_surface->setBrightness(boundedBrightness);
}

int VideoWidget::contrast() const
{
    return m_surface->contrast();
}

void VideoWidget::setContrast(int contrast) const
{
    int boundedContrast = qBound(-100, contrast, 100);
    m_surface->setContrast(boundedContrast);
}

int VideoWidget::hue() const
{
    return m_surface->hue();
}

void VideoWidget::setHue(int hue) const
{
    int boundedHue = qBound(-100, hue, 100);
    m_surface->setHue(boundedHue);
}

int VideoWidget::saturation() const
{
    return m_surface->saturation();
}

void VideoWidget::setSaturation(int saturation) const
{
    int boundedSaturation = qBound(-100, saturation, 100);
    m_surface->setSaturation(boundedSaturation);
}

QSize VideoWidget::sizeHint() const
{
    return m_surface->surfaceFormat().sizeHint();
}

void VideoWidget::showEvent(QShowEvent *event)
{
}

void VideoWidget::hideEvent(QHideEvent *event)
{
    m_updatePaintDevice = true;
}

void VideoWidget::resizeEvent(QResizeEvent *event)
{
    updateRects();
}

void VideoWidget::moveEvent(QMoveEvent *event)
{
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    if (testAttribute(Qt::WA_OpaquePaintEvent)) {
        QRegion borderRegion = event->region();
        borderRegion = borderRegion.subtracted(m_boundingRect);

        QBrush brush = palette().window();

        for (const QRect &r : borderRegion)
            painter.fillRect(r, brush);
    }

    if (m_surface->isActive() && m_boundingRect.intersects(event->rect())) {
        m_surface->paint(&painter, m_boundingRect, m_sourceRect);

        m_surface->setReady(true);
    } else {
        if (m_updatePaintDevice && (painter.paintEngine()->type() == QPaintEngine::OpenGL
                                    || painter.paintEngine()->type() == QPaintEngine::OpenGL2)) {
            m_updatePaintDevice = false;

            m_surface->updateGLContext();
            if (m_surface->supportedShaderTypes() & QPainterVideoSurface::GlslShader) {
                m_surface->setShaderType(QPainterVideoSurface::GlslShader);
            } else {
                m_surface->setShaderType(QPainterVideoSurface::FragmentProgramShader);
            }
        }
    }
}

bool VideoWidget::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);

    MSG *mes = reinterpret_cast<MSG *>(message);
    if (mes->message == WM_PAINT || mes->message == WM_ERASEBKGND) {
        showEvent(nullptr);
    }

    return false;
}

void VideoWidget::formatChanged(const QVideoSurfaceFormat &format)
{
    m_nativeSize = format.sizeHint();

    updateRects();
    updateGeometry();
    update();
}

void VideoWidget::frameChanged()
{
    update(m_boundingRect);
}

void VideoWidget::updateRects()
{
    QRect rect = this->rect();

    if (m_nativeSize.isEmpty()) {
        m_boundingRect = QRect();
    } else if (m_aspectRatioMode == Qt::IgnoreAspectRatio) {
        m_boundingRect = rect;
        m_sourceRect = QRectF(0, 0, 1, 1);
    } else if (m_aspectRatioMode == Qt::KeepAspectRatio) {
        QSize size = m_nativeSize;
        size.scale(rect.size(), Qt::KeepAspectRatio);

        m_boundingRect = QRect(0, 0, size.width(), size.height());
        m_boundingRect.moveCenter(rect.center());

        m_sourceRect = QRectF(0, 0, 1, 1);
    } else if (m_aspectRatioMode == Qt::KeepAspectRatioByExpanding) {
        m_boundingRect = rect;

        QSizeF size = rect.size();
        size.scale(m_nativeSize, Qt::KeepAspectRatio);

        m_sourceRect = QRectF(
                0, 0, size.width() / m_nativeSize.width(), size.height() / m_nativeSize.height());
        m_sourceRect.moveCenter(QPointF(0.5, 0.5));
    }
}