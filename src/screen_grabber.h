#pragma once

#include <QObject>
#include <QImage>
#include <QPixmap>
#include <QQuickImageProvider>
#include <QMutex>
#include <thread>
#include <atomic>

class ScreenGrabber;

class ScreenGrabProvider : public QQuickImageProvider {
public:
    explicit ScreenGrabProvider(ScreenGrabber *grabber);
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    ScreenGrabber *m_grabber;
};

class ScreenGrabber : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isCapturing READ isCapturing NOTIFY isCapturingChanged)
    Q_PROPERTY(int revision READ revision NOTIFY captureUpdated)
    Q_PROPERTY(int captureX READ captureX NOTIFY captureUpdated)
    Q_PROPERTY(int captureY READ captureY NOTIFY captureUpdated)
    Q_PROPERTY(int captureW READ captureW NOTIFY captureUpdated)
    Q_PROPERTY(int captureH READ captureH NOTIFY captureUpdated)
    Q_PROPERTY(bool isLiveRunning READ isLiveRunning NOTIFY isCapturingChanged)

public:
    explicit ScreenGrabber(QObject *parent = nullptr);
    ~ScreenGrabber() override;

    bool isCapturing() const { return m_isCapturing; }
    bool isLiveRunning() const { return m_workerRunning.load(); }
    int revision() const { return m_revision; }
    int captureX() const { return m_captureX; }
    int captureY() const { return m_captureY; }
    int captureW() const { return m_captureW; }
    int captureH() const { return m_captureH; }

    QImage rawImage() const;
    QImage blurredImage() const;

    Q_INVOKABLE bool captureRegion(int x, int y, int width, int height, int blurRadius = 40, qreal vibrancy = 1.45, int chromaticShift = 14, const QString &outputName = {});
    Q_INVOKABLE void startLiveCapture(int x, int y, int width, int height, int blurRadius = 40, qreal vibrancy = 1.45, int chromaticShift = 14, int fps = 30);
    Q_INVOKABLE void stopLiveCapture();
    Q_INVOKABLE void clear();

    static QImage applyFastBlur(const QImage &src, int blurRadius = 40, qreal vibrancy = 1.45, int chromaticShift = 14);

signals:
    void isCapturingChanged();
    void captureUpdated();

private:
    void workerLoop();

    mutable QMutex m_mutex;
    bool m_isCapturing = false;
    int m_revision = 0;
    int m_captureX = 0;
    int m_captureY = 0;
    int m_captureW = 0;
    int m_captureH = 0;
    QImage m_rawImage;
    QImage m_blurredImage;

    std::thread m_workerThread;
    std::atomic<bool> m_workerRunning{false};
    std::atomic<int> m_targetX{0};
    std::atomic<int> m_targetY{0};
    std::atomic<int> m_targetW{0};
    std::atomic<int> m_targetH{0};
    std::atomic<int> m_targetBlur{40};
    std::atomic<int> m_targetChromatic{14};
    std::atomic<int> m_targetFps{30};
    std::atomic<double> m_targetVibrancy{1.45};
};
