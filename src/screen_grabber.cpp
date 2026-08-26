#include "screen_grabber.h"
#include "screen_detector.h"
#include <QProcess>
#include <QGuiApplication>
#include <QScreen>
#include <QDebug>
#include <QtGlobal>
#include <chrono>

ScreenGrabProvider::ScreenGrabProvider(ScreenGrabber *grabber)
    : QQuickImageProvider(QQuickImageProvider::Pixmap), m_grabber(grabber) {}

QPixmap ScreenGrabProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) {
    if (!m_grabber) return QPixmap();

    QString type = id.split('?').first();
    QImage img;
    if (type == "raw") {
        img = m_grabber->rawImage();
    } else {
        img = m_grabber->blurredImage();
    }

    if (img.isNull()) {
        if (size) *size = QSize(0, 0);
        return QPixmap();
    }

    if (size) *size = img.size();
    if (requestedSize.width() > 0 && requestedSize.height() > 0) {
        return QPixmap::fromImage(img.scaled(requestedSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }

    return QPixmap::fromImage(img);
}

ScreenGrabber::ScreenGrabber(QObject *parent) : QObject(parent) {}

ScreenGrabber::~ScreenGrabber() {
    stopLiveCapture();
}

QImage ScreenGrabber::rawImage() const {
    QMutexLocker locker(&m_mutex);
    return m_rawImage;
}

QImage ScreenGrabber::blurredImage() const {
    QMutexLocker locker(&m_mutex);
    return m_blurredImage;
}

void ScreenGrabber::clear() {
    stopLiveCapture();
    QMutexLocker locker(&m_mutex);
    m_rawImage = QImage();
    m_blurredImage = QImage();
    m_revision++;
    emit captureUpdated();
}

static void fastBoxBlurH(const QRgb *src, QRgb *dst, int w, int h, int r) {
    if (r <= 0) {
        memcpy(dst, src, w * h * sizeof(QRgb));
        return;
    }
    float iarr = 1.0f / (r + r + 1);
    for (int i = 0; i < h; i++) {
        int ti = i * w;
        int li = ti;
        int ri = ti + r;
        QRgb fv = src[ti];
        QRgb lv = src[ti + w - 1];
        int valR = (r + 1) * qRed(fv);
        int valG = (r + 1) * qGreen(fv);
        int valB = (r + 1) * qBlue(fv);
        int valA = (r + 1) * qAlpha(fv);

        for (int j = 0; j < r; j++) {
            QRgb col = src[ti + j];
            valR += qRed(col);
            valG += qGreen(col);
            valB += qBlue(col);
            valA += qAlpha(col);
        }
        for (int j = 0; j <= r; j++) {
            QRgb col = src[qMin(ri++, ti + w - 1)];
            valR += qRed(col) - qRed(fv);
            valG += qGreen(col) - qGreen(fv);
            valB += qBlue(col) - qBlue(fv);
            valA += qAlpha(col) - qAlpha(fv);
            dst[ti++] = qRgba(valR * iarr, valG * iarr, valB * iarr, valA * iarr);
        }
        for (int j = r + 1; j < w - r; j++) {
            QRgb col1 = src[ri++];
            QRgb col2 = src[li++];
            valR += qRed(col1) - qRed(col2);
            valG += qGreen(col1) - qGreen(col2);
            valB += qBlue(col1) - qBlue(col2);
            valA += qAlpha(col1) - qAlpha(col2);
            dst[ti++] = qRgba(valR * iarr, valG * iarr, valB * iarr, valA * iarr);
        }
        for (int j = w - r; j < w; j++) {
            QRgb col = src[li++];
            valR += qRed(lv) - qRed(col);
            valG += qGreen(lv) - qGreen(col);
            valB += qBlue(lv) - qBlue(col);
            valA += qAlpha(lv) - qAlpha(col);
            dst[ti++] = qRgba(valR * iarr, valG * iarr, valB * iarr, valA * iarr);
        }
    }
}

static void fastBoxBlurT(const QRgb *src, QRgb *dst, int w, int h, int r) {
    if (r <= 0) {
        memcpy(dst, src, w * h * sizeof(QRgb));
        return;
    }
    float iarr = 1.0f / (r + r + 1);
    for (int i = 0; i < w; i++) {
        int ti = i;
        int li = ti;
        int ri = ti + r * w;
        QRgb fv = src[ti];
        QRgb lv = src[ti + (h - 1) * w];
        int valR = (r + 1) * qRed(fv);
        int valG = (r + 1) * qGreen(fv);
        int valB = (r + 1) * qBlue(fv);
        int valA = (r + 1) * qAlpha(fv);

        for (int j = 0; j < r; j++) {
            QRgb col = src[ti + j * w];
            valR += qRed(col);
            valG += qGreen(col);
            valB += qBlue(col);
            valA += qAlpha(col);
        }
        for (int j = 0; j <= r; j++) {
            QRgb col = src[qMin(ri, ti + (h - 1) * w)];
            ri += w;
            valR += qRed(col) - qRed(fv);
            valG += qGreen(col) - qGreen(fv);
            valB += qBlue(col) - qBlue(fv);
            valA += qAlpha(col) - qAlpha(fv);
            dst[ti] = qRgba(valR * iarr, valG * iarr, valB * iarr, valA * iarr);
            ti += w;
        }
        for (int j = r + 1; j < h - r; j++) {
            QRgb col1 = src[ri]; ri += w;
            QRgb col2 = src[li]; li += w;
            valR += qRed(col1) - qRed(col2);
            valG += qGreen(col1) - qGreen(col2);
            valB += qBlue(col1) - qBlue(col2);
            valA += qAlpha(col1) - qAlpha(col2);
            dst[ti] = qRgba(valR * iarr, valG * iarr, valB * iarr, valA * iarr);
            ti += w;
        }
        for (int j = h - r; j < h; j++) {
            QRgb col = src[li]; li += w;
            valR += qRed(lv) - qRed(col);
            valG += qGreen(lv) - qGreen(col);
            valB += qBlue(lv) - qBlue(col);
            valA += qAlpha(lv) - qAlpha(col);
            dst[ti] = qRgba(valR * iarr, valG * iarr, valB * iarr, valA * iarr);
            ti += w;
        }
    }
}

bool ScreenGrabber::captureRegion(int x, int y, int width, int height, int blurRadius, qreal vibrancy, int chromaticShift, const QString &outputName) {
    Q_UNUSED(outputName);
    if (width <= 0 || height <= 0) return false;

    m_isCapturing = true;
    emit isCapturingChanged();

    TargetScreenInfo screenInfo = ScreenDetector::getTargetScreenInfo();
    int globalX = screenInfo.monitorX + x;
    int globalY = screenInfo.monitorY + y;

    QImage captured;

    QString geom = QString("%1,%2 %3x%4").arg(globalX).arg(globalY).arg(width).arg(height);
    QStringList args = {"-g", geom, "-t", "ppm", "-"};

    QProcess proc;
    proc.start("grim", args);
    if (proc.waitForFinished(150) && proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0) {
        QByteArray data = proc.readAllStandardOutput();
        if (!data.isEmpty()) {
            captured.loadFromData(data, "PPM");
        }
    }

    if (captured.isNull()) {
        QScreen *targetScreen = QGuiApplication::screenAt(QPoint(globalX, globalY));
        if (!targetScreen) targetScreen = QGuiApplication::primaryScreen();
        if (targetScreen) {
            captured = targetScreen->grabWindow(0, x, y, width, height).toImage();
        }
    }

    if (captured.isNull()) {
        m_isCapturing = false;
        emit isCapturingChanged();
        return false;
    }

    int r = qMax(0, blurRadius);
    QImage blurred = applyFastBlur(captured, r, vibrancy, chromaticShift);

    {
        QMutexLocker locker(&m_mutex);
        m_captureX = x;
        m_captureY = y;
        m_captureW = width;
        m_captureH = height;
        m_rawImage = captured;
        m_blurredImage = blurred;
        m_revision++;
    }

    m_isCapturing = false;
    emit isCapturingChanged();
    emit captureUpdated();
    return true;
}

void ScreenGrabber::startLiveCapture(int x, int y, int width, int height, int blurRadius, qreal vibrancy, int chromaticShift, int fps) {
    m_targetX.store(x);
    m_targetY.store(y);
    m_targetW.store(width);
    m_targetH.store(height);
    m_targetBlur.store(blurRadius);
    m_targetChromatic.store(chromaticShift);
    m_targetFps.store(qBound(1, fps, 60));
    m_targetVibrancy.store(vibrancy);

    if (m_workerRunning.load()) return;

    // Trigger immediate first synchronous frame for zero lag on open
    captureRegion(x, y, width, height, blurRadius, vibrancy, chromaticShift);

    m_workerRunning.store(true);
    emit isCapturingChanged();
    m_workerThread = std::thread(&ScreenGrabber::workerLoop, this);
}

void ScreenGrabber::stopLiveCapture() {
    if (!m_workerRunning.load()) return;

    m_workerRunning.store(false);
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    emit isCapturingChanged();
}

void ScreenGrabber::workerLoop() {
    while (m_workerRunning.load()) {
        int fps = qBound(1, m_targetFps.load(), 60);
        int intervalMs = 1000 / fps;
        auto nextTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(intervalMs);

        int x = m_targetX.load();
        int y = m_targetY.load();
        int w = m_targetW.load();
        int h = m_targetH.load();
        int blur = m_targetBlur.load();
        double vib = m_targetVibrancy.load();
        int chrom = m_targetChromatic.load();

        if (w > 0 && h > 0) {
            TargetScreenInfo screenInfo = ScreenDetector::getTargetScreenInfo();
            int globalX = screenInfo.monitorX + x;
            int globalY = screenInfo.monitorY + y;

            QString geom = QString("%1,%2 %3x%4").arg(globalX).arg(globalY).arg(w).arg(h);
            QStringList args = {"-g", geom, "-t", "ppm", "-"};

            QProcess proc;
            proc.start("grim", args);
            if (proc.waitForFinished(120) && proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0) {
                QByteArray data = proc.readAllStandardOutput();
                if (!data.isEmpty() && m_workerRunning.load()) {
                    QImage captured;
                    if (captured.loadFromData(data, "PPM")) {
                        QImage blurred = applyFastBlur(captured, blur, vib, chrom);
                        if (m_workerRunning.load()) {
                            {
                                QMutexLocker locker(&m_mutex);
                                m_rawImage = captured;
                                m_blurredImage = blurred;
                                m_captureX = x;
                                m_captureY = y;
                                m_captureW = w;
                                m_captureH = h;
                                m_revision++;
                            }
                            QMetaObject::invokeMethod(this, "captureUpdated", Qt::QueuedConnection);
                        }
                    }
                }
            }
        }

        std::this_thread::sleep_until(nextTime);
    }
}

// Feedback-stable vibrancy boost (contractive fixed-point in HSV saturation space)
static inline QRgb applyStableVibrancy(QRgb pixel, qreal vibrancy) {
    if (vibrancy <= 1.0) return pixel;

    int r = qRed(pixel);
    int g = qGreen(pixel);
    int b = qBlue(pixel);
    int a = qAlpha(pixel);

    int maxC = qMax(r, qMax(g, b));
    int minC = qMin(r, qMin(g, b));
    int delta = maxC - minC;

    if (delta == 0 || maxC == 0) return pixel;

    // Saturation in [0, 255]
    int sat = (delta * 255) / maxC;
    // Bounded target saturation (maximum ceiling of 235 prevents primary saturation blowout)
    int targetSat = qBound(0, int(sat * vibrancy), 235);
    if (targetSat <= sat) return pixel;

    int scale = (targetSat * 256) / sat;
    int newR = qBound(0, maxC - ((maxC - r) * scale) / 256, 255);
    int newG = qBound(0, maxC - ((maxC - g) * scale) / 256, 255);
    int newB = qBound(0, maxC - ((maxC - b) * scale) / 256, 255);

    return qRgba(newR, newG, newB, a);
}

QImage ScreenGrabber::applyFastBlur(const QImage &src, int blurRadius, qreal vibrancy, int chromaticShift) {
    if (src.isNull()) return src;

    int w = src.width();
    int h = src.height();

    QImage blurred = src.convertToFormat(QImage::Format_ARGB32);

    // Pass 1: True Mathematical Gaussian Frosted Blur (3-pass separable box filter in O(N))
    int r = qBound(0, blurRadius, 300);
    if (r > 0) {
        int scaleFactor = r > 16 ? qMin(4, r / 8) : 1;
        int dw = qMax(16, w / scaleFactor);
        int dh = qMax(16, h / scaleFactor);
        int scaledR = qMax(1, r / scaleFactor);

        QImage small = (scaleFactor > 1)
            ? blurred.scaled(dw, dh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
            : blurred;

        QImage tmp(dw, dh, QImage::Format_ARGB32);
        QRgb *smallBits = reinterpret_cast<QRgb*>(small.bits());
        QRgb *tmpBits = reinterpret_cast<QRgb*>(tmp.bits());

        // 3-pass Gaussian approximation
        fastBoxBlurH(smallBits, tmpBits, dw, dh, scaledR);
        fastBoxBlurT(tmpBits, smallBits, dw, dh, scaledR);
        fastBoxBlurH(smallBits, tmpBits, dw, dh, scaledR);
        fastBoxBlurT(tmpBits, smallBits, dw, dh, scaledR);
        fastBoxBlurH(smallBits, tmpBits, dw, dh, scaledR);
        fastBoxBlurT(tmpBits, smallBits, dw, dh, scaledR);

        blurred = (scaleFactor > 1)
            ? small.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
            : small;
    }

    // Pass 2: Apple Vibrancy color lift (Feedback-stable bounded saturation)
    if (vibrancy > 1.0) {
        blurred = blurred.convertToFormat(QImage::Format_ARGB32);
        QRgb *bits = reinterpret_cast<QRgb*>(blurred.bits());
        int pixelCount = w * h;
        for (int i = 0; i < pixelCount; ++i) {
            bits[i] = applyStableVibrancy(bits[i], vibrancy);
        }
    }

    // Pass 3: Pure Optical Chromatic Dispersion (100% Sharp Prism Split)
    int shift = qBound(0, chromaticShift, 100);
    if (shift == 0) return blurred;

    blurred = blurred.convertToFormat(QImage::Format_ARGB32);
    QImage result(w, h, QImage::Format_ARGB32);
    const QRgb *srcBits = reinterpret_cast<const QRgb*>(blurred.constBits());
    QRgb *dstBits = reinterpret_cast<QRgb*>(result.bits());

    int sX = shift;
    int sY = qMax(0, shift / 2);

    for (int y = 0; y < h; ++y) {
        int rY = qBound(0, y - sY, h - 1) * w;
        int gY = y * w;
        int bY = qBound(0, y + sY, h - 1) * w;

        for (int x = 0; x < w; ++x) {
            int rX = qBound(0, x + sX, w - 1);
            int gX = x;
            int bX = qBound(0, x - sX, w - 1);

            QRgb col_r = srcBits[rY + rX];
            QRgb col_g = srcBits[gY + gX];
            QRgb col_b = srcBits[bY + bX];

            dstBits[gY + x] = qRgba(qRed(col_r), qGreen(col_g), qBlue(col_b), qAlpha(col_g));
        }
    }

    return result;
}
