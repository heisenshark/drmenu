import QtQuick
import QtQuick.Shapes

Item {
    id: centerPivot

    property var rootRef: root
    property var menuContainerRef: menuContainer

    property bool isGlass: (rootRef.s.glass === true || rootRef.s.useGlass === true)
    property bool isBlenderMode: !isGlass && (rootRef.s.centerLayout === "torus" || (rootRef.s.showCenterArc === true && rootRef.s.centerBorderWidth === 0))
    property real torusRadius: isBlenderMode ? (rootRef.s.torusRadius || 26) : (rootRef.isPieMode ? (rootRef.s.innerRadius || 65) : (rootRef.s.centerRadius || 15))

    x: menuContainerRef.centerX - width  / 2
    y: menuContainerRef.centerY - height / 2
    width:  torusRadius * 2
    height: torusRadius * 2
    z: 25

    // ── Mode A: Solid Circle Center Pivot (Standard Pie Theme) ────────────────
    Rectangle {
        anchors.fill: parent
        visible: !centerPivot.isBlenderMode && !(rootRef.s.glass === true || rootRef.s.useGlass === true)
        radius: width / 2
        color:  rootRef.s.centerColor || "#111116"
        border.color: (rootRef.s.centerBorderWidth === 0)
            ? "transparent"
            : (menuContainerRef.hoveredIndex !== -1
                ? (rootRef.s.centerBorderHoverColor || rootRef.s.centerBorderHover || "#e67e22")
                : (rootRef.hasParent
                    ? ((rootRef.s.submenuAccent && rootRef.s.submenuAccent !== "transparent") ? rootRef.s.submenuAccent : "#c084fc")
                    : (rootRef.s.centerBorderColor || rootRef.s.centerBorder || "#5a5a72")))
        border.width: rootRef.s.centerBorderWidth !== undefined ? rootRef.s.centerBorderWidth : 3

        Behavior on border.color { ColorAnimation { duration: 50 } }

        Rectangle {
            anchors.fill: parent
            anchors.margins: parent.border.width
            radius: width / 2
            color: "transparent"
            clip: true
            z: -1
            visible: (rootRef.s.useScreencopyGlass === true) && (typeof screenGrabber !== "undefined") && screenGrabber && (screenGrabber.revision > 0)
            layer.enabled: true
            layer.smooth: true

            Image {
                visible: parent.visible
                x: -(centerPivot.x - (menuContainerRef.centerX - menuContainerRef.margin))
                y: -(centerPivot.y - (menuContainerRef.centerY - menuContainerRef.margin))
                width: menuContainerRef.margin * 2
                height: menuContainerRef.margin * 2
                source: (typeof screenGrabber !== "undefined" && screenGrabber && screenGrabber.revision > 0)
                    ? ("image://screengrab/blurred?rev=" + screenGrabber.revision)
                    : ""
                smooth: true
                cache: false
            }
        }
    }

    // ── Mode B: Blender Donut Torus Ring Body ────────────────────────────────
    Shape {
        id: torusBaseShape
        visible: centerPivot.isBlenderMode
        anchors.fill: parent
        layer.enabled: true
        layer.samples: 4

        property real thick: rootRef.s.centerTorusThickness || 8
        property real r:     centerPivot.width / 2 - thick / 2

        ShapePath {
            strokeWidth: torusBaseShape.thick
            strokeColor: rootRef.s.centerTorusColor || "#383838"
            fillColor:   "transparent"

            PathAngleArc {
                centerX:    centerPivot.width / 2
                centerY:    centerPivot.height / 2
                radiusX:    torusBaseShape.r
                radiusY:    torusBaseShape.r
                startAngle: 0
                sweepAngle: 360
            }
        }
    }

    // ── Blender Mouse-Tracking 90° Blue Arc Indicator ─────────────────────────
    Shape {
        id: blenderArcShape
        visible: centerPivot.isBlenderMode && (rootRef.s.showCenterArc !== false)
        anchors.fill: parent
        layer.enabled: true
        layer.samples: 4

        property real arcSpread: rootRef.s.centerArcAngle !== undefined ? rootRef.s.centerArcAngle : 90
        property real thick:     rootRef.s.centerArcWidth || rootRef.s.centerTorusThickness || 8
        property real r:         centerPivot.width / 2 - thick / 2
        property real startDeg:  menuContainerRef.currentMouseAngleDeg - arcSpread / 2

        ShapePath {
            strokeWidth: blenderArcShape.thick
            strokeColor: rootRef.s.centerArcColor || rootRef.s.accentColor || "#3b82f6"
            fillColor:   "transparent"
            capStyle:    ShapePath.FlatCap

            PathAngleArc {
                centerX:    centerPivot.width / 2
                centerY:    centerPivot.height / 2
                radiusX:    blenderArcShape.r
                radiusY:    blenderArcShape.r
                startAngle: blenderArcShape.startDeg
                sweepAngle: blenderArcShape.arcSpread
            }
        }
    }

    // ── Center Hole Indicators (Dot / Back Arrow) ────────────────────────────
    Text {
        id: backArrowText
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 0
        visible: rootRef.hasParent && menuContainerRef.hoveredIndex === -1
        text: "←"
        font.family: rootRef.s.fontFamily || "Sans"
        font.pixelSize: (rootRef.s.fontSize || 13) + 2
        font.bold: true
        scale: (menuContainerRef.hoveredIndex === -1) ? 1.25 : 1.0

        Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.OutQuad } }

        color: {
            let isGlass = (rootRef.s.glass === true || rootRef.s.useGlass === true)
            let subAccent = (rootRef.s.showSubmenuAccent !== false) &&
                            (rootRef.s.submenuAccent && rootRef.s.submenuAccent !== "transparent" && rootRef.s.submenuAccent !== "none")
                            ? rootRef.s.submenuAccent : ""

            if (isGlass) {
                return subAccent ? subAccent : "#ffffff"
            }
            return subAccent ? subAccent : "#60a5fa"
        }

        Behavior on color { ColorAnimation { duration: 60 } }
    }

    Rectangle {
        anchors.centerIn: parent
        visible: !(rootRef.hasParent && menuContainerRef.hoveredIndex === -1)
        width:  menuContainerRef.hoveredIndex !== -1 ? 7 : 4
        height: width
        radius: width / 2
        color: {
            if (rootRef.s.glass === true || rootRef.s.useGlass === true) {
                return menuContainerRef.hoveredIndex !== -1
                    ? (rootRef.s.centerDotHoverColor || "#a0ffffff")
                    : (rootRef.s.centerDotColor || "#60ffffff")
            }
            return menuContainerRef.hoveredIndex !== -1
                ? (rootRef.s.centerDotHoverColor || "#3b82f6")
                : (rootRef.s.centerDotColor || "#808090")
        }

        Behavior on width { NumberAnimation { duration: 40 } }
        Behavior on color { ColorAnimation  { duration: 40 } }
    }
}
