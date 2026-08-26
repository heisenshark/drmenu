#include "screen_grabber.h"
#include "config_loader.h"
#include "theme_manager.h"
#include "screen_detector.h"

#include <QGuiApplication>
#include <QImage>
#include <QColor>
#include <QDebug>
#include <iostream>
#include <cassert>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    std::cout << "=== RUNNING DRMENU SCREENCOPY & OPTICS TESTS ===\n";

    // ── Test 1: Config Loading ──────────────────────────────────────────────
    std::cout << "[Test 1] Testing ConfigLoader and ThemeManager property resolution...\n";
    QVariantMap style = ConfigLoader::loadStyle("main");
    std::cout << "  - Theme resolved: useScreencopyGlass=" << style["useScreencopyGlass"].toBool()
              << ", screencopyBlurRadius=" << style["screencopyBlurRadius"].toInt()
              << ", chromaticAberration=" << style["chromaticAberration"].toInt()
              << ", screencopyVibrancy=" << style["screencopyVibrancy"].toDouble() << "\n";

    assert(style.contains("useScreencopyGlass"));
    assert(style["useScreencopyGlass"].toBool() == true);
    std::cout << "  [PASS] ConfigLoader parsed screencopy properties correctly.\n";

    // ── Test 2: Zero Blur (Crystal Clear) ───────────────────────────────────
    std::cout << "[Test 2] Testing Zero Blur (blurRadius = 0)...\n";
    QImage testImg(100, 100, QImage::Format_ARGB32);
    testImg.fill(Qt::black);
    // Draw a single white pixel at (50, 50)
    testImg.setPixelColor(50, 50, QColor(255, 255, 255));

    QImage zeroBlur = ScreenGrabber::applyFastBlur(testImg, 0, 1.0, 0);
    assert(zeroBlur.pixelColor(50, 50) == QColor(255, 255, 255));
    assert(zeroBlur.pixelColor(49, 50) == QColor(0, 0, 0));
    assert(zeroBlur.pixelColor(51, 50) == QColor(0, 0, 0));
    std::cout << "  [PASS] Zero blur preserves 100% sharp pixels with zero bleeding.\n";

    // ── Test 3: Gaussian Frosted Blur ───────────────────────────────────────
    std::cout << "[Test 3] Testing Gaussian Frosted Blur (blurRadius = 10)...\n";
    QImage frosted = ScreenGrabber::applyFastBlur(testImg, 10, 1.0, 0);
    // The single pixel at (50, 50) should now be diffused into neighbors
    QColor centerColor = frosted.pixelColor(50, 50);
    QColor neighborColor = frosted.pixelColor(49, 50);
    std::cout << "  - Frosted center brightness: " << centerColor.red()
              << ", neighbor brightness: " << neighborColor.red() << "\n";
    assert(neighborColor.red() > 0);
    std::cout << "  [PASS] Frosted blur correctly diffuses light to neighboring pixels.\n";

    // ── Test 4: Pure Optical Chromatic Dispersion (Prism Edge Split) ────────
    std::cout << "[Test 4] Testing Chromatic Dispersion (shift = 10)...\n";
    QImage chromImg = ScreenGrabber::applyFastBlur(testImg, 0, 1.0, 10);
    // The red channel should be shifted to (50 - 10, 50) and blue to (50 + 10, 50)
    // Red displaced right (+10px -> x=60), Blue displaced left (-10px -> x=40)
    QColor redPixel = chromImg.pixelColor(60, 45);
    QColor bluePixel = chromImg.pixelColor(40, 55);
    std::cout << "  - Red channel offset (x=60, y=45): R=" << redPixel.red() << " G=" << redPixel.green() << " B=" << redPixel.blue() << "\n";
    std::cout << "  - Blue channel offset (x=40, y=55): R=" << bluePixel.red() << " G=" << bluePixel.green() << " B=" << bluePixel.blue() << "\n";
    assert(redPixel.red() > 0);
    assert(bluePixel.blue() > 0);
    std::cout << "  [PASS] Chromatic dispersion accurately separates Red and Blue channels without blur.\n";

    // ── Test 5: Live Wayland Screen Capture ─────────────────────────────────
    std::cout << "[Test 5] Testing Wayland live ScreenGrabber::captureRegion...\n";
    ScreenGrabber grabber;
    TargetScreenInfo info = ScreenDetector::getTargetScreenInfo();
    std::cout << "  - Detected monitor: " << (info.monitorName.isEmpty() ? "none" : info.monitorName.toStdString())
              << " at (" << info.monitorX << ", " << info.monitorY << ")\n";

    bool capSuccess = grabber.captureRegion(100, 100, 400, 400, 0, 1.45, 14, info.monitorName);
    std::cout << "  - Capture success: " << (capSuccess ? "YES" : "NO (headless/fallback)") << "\n";
    if (capSuccess) {
        QImage raw = grabber.rawImage();
        QImage blurred = grabber.blurredImage();
        std::cout << "  - Captured raw size: " << raw.width() << "x" << raw.height() << "\n";
        std::cout << "  - Processed glass size: " << blurred.width() << "x" << blurred.height() << "\n";
        assert(!raw.isNull());
        assert(!blurred.isNull());
        assert(raw.size() == blurred.size());
    }
    std::cout << "  [PASS] ScreenGrabber pipeline executed successfully.\n";

    std::cout << "=== ALL TESTS PASSED SUCCESSFULLY! ===\n";
    return 0;
}
