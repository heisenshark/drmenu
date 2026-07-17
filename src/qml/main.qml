import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

ApplicationWindow {
    id: root
    visible: false
    width: Screen.width
    height: Screen.height
    title: "drmenu"

    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.BypassWindowManagerHint
    color: "transparent"

    // Center of the radial menu (set by C++ to cursor position, clamped to screen)
    property real menuX: width / 2
    property real menuY: height / 2

    onVisibleChanged: {
        if (visible) {
            root.raise()
            root.requestActivate()
            openAnimation.start()
        }
    }

    Shortcut {
        sequence: "Escape"
        onActivated: output.cancel()
    }

    ParallelAnimation {
        id: openAnimation
        NumberAnimation { target: menuContainer; property: "scale";   from: 0.7; to: 1.0; duration: 200; easing.type: Easing.OutBack }
        NumberAnimation { target: menuContainer; property: "opacity"; from: 0.0; to: 1.0; duration: 150 }
    }

    // ── Dynamic model from C++ (menuItems context property) ──────────────────
    // Each element is a map: { label, icon, command }
    // We wrap it in a ListModel so QML Repeater can use it with index.
    ListModel {
        id: menuModel
        Component.onCompleted: {
            for (let i = 0; i < menuItems.length; ++i) {
                let e = menuItems[i]
                menuModel.append({
                    label:   e.label   || "",
                    icon:    e.icon    || "",
                    command: e.command || ""
                })
            }
        }
    }

    Item {
        id: menuContainer
        anchors.fill: parent
        opacity: 0
        scale: 0.7
        focus: true

        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Escape) {
                output.cancel()
                event.accepted = true
            }
        }
        Component.onCompleted: forceActiveFocus()

        property real outerRadius: Math.min(280, Math.min(root.width, root.height) * 0.28)
        property real innerRadius: outerRadius * 0.36
        property real margin: outerRadius + 20
        property real centerX: Math.max(margin, Math.min(width  - margin, root.menuX))
        property real centerY: Math.max(margin, Math.min(height - margin, root.menuY))
        property int  itemCount: menuModel.count
        property int  hoveredIndex: -1

        // ── Dim overlay ───────────────────────────────────────────────────────
        Rectangle {
            anchors.fill: parent
            color: "#000000"
            opacity: 0.45
        }

        // ── Mouse tracking ────────────────────────────────────────────────────
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true

            onPositionChanged: (mouse) => {
                let dx = mouse.x - menuContainer.centerX
                let dy = mouse.y - menuContainer.centerY
                let dist = Math.sqrt(dx*dx + dy*dy)

                if (dist >= menuContainer.innerRadius && dist <= menuContainer.outerRadius) {
                    let angle = Math.atan2(dy, dx) + Math.PI / 2
                    if (angle < 0) angle += 2 * Math.PI
                    let sliceAngle = 360 / menuContainer.itemCount
                    menuContainer.hoveredIndex = Math.floor((angle * 180 / Math.PI) / sliceAngle) % menuContainer.itemCount
                } else {
                    menuContainer.hoveredIndex = -1
                }
            }

            onClicked: {
                if (menuContainer.hoveredIndex !== -1) {
                    let item = menuModel.get(menuContainer.hoveredIndex)
                    output.select(item.label)
                } else {
                    output.cancel()
                }
            }
        }

        // ── Slices ────────────────────────────────────────────────────────────
        Repeater {
            model: menuModel
            delegate: Item {
                id: slice
                anchors.fill: parent

                property bool   isHovered:  menuContainer.hoveredIndex === index
                property real   sliceAngle: 360 / menuContainer.itemCount
                property real   startAngle: index * sliceAngle - 90
                property real   midAngleRad: (startAngle + sliceAngle / 2) * Math.PI / 180
                // Effective icon: use provided icon or fall back to first letter
                property string displayIcon: model.icon !== "" ? model.icon : model.label.charAt(0).toUpperCase()

                Shape {
                    anchors.fill: parent
                    layer.enabled: true
                    layer.samples: 4

                    ShapePath {
                        strokeWidth: 2
                        strokeColor: slice.isHovered ? "#8b5cf6" : "#27272a"
                        fillColor:   slice.isHovered ? "#2d1457" : "#18181b"

                        Behavior on fillColor   { ColorAnimation { duration: 100 } }
                        Behavior on strokeColor { ColorAnimation { duration: 100 } }

                        PathAngleArc {
                            centerX:    menuContainer.centerX
                            centerY:    menuContainer.centerY
                            radiusX:    menuContainer.outerRadius
                            radiusY:    menuContainer.outerRadius
                            startAngle: slice.startAngle
                            sweepAngle: slice.sliceAngle
                        }
                        PathLine { x: menuContainer.centerX; y: menuContainer.centerY }
                    }
                }

                // Label + icon positioned at mid-arc
                Item {
                    property real placementR: (menuContainer.outerRadius + menuContainer.innerRadius) / 2
                    x: menuContainer.centerX + placementR * Math.cos(slice.midAngleRad) - width  / 2
                    y: menuContainer.centerY + placementR * Math.sin(slice.midAngleRad) - height / 2
                    width: 110
                    height: 64

                    Column {
                        anchors.centerIn: parent
                        spacing: 4

                        Text {
                            text: slice.displayIcon
                            font.pixelSize: model.icon !== "" ? 22 : 18
                            font.bold: model.icon === ""
                            color: slice.isHovered ? "#c4b5fd" : "#a1a1aa"
                            horizontalAlignment: Text.AlignHCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Text {
                            text: model.label
                            color: slice.isHovered ? "#ffffff" : "#d4d4d8"
                            font.bold: slice.isHovered
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                            elide: Text.ElideRight
                            width: 100
                        }
                    }
                }
            }
        }

        // ── Center ring ───────────────────────────────────────────────────────
        Rectangle {
            x: menuContainer.centerX - width  / 2
            y: menuContainer.centerY - height / 2
            width:  menuContainer.innerRadius * 2 - 6
            height: menuContainer.innerRadius * 2 - 6
            radius: width / 2

            color: "#09090b"
            border.color: menuContainer.hoveredIndex !== -1 ? "#8b5cf6" : "#3f3f46"
            border.width: 2
            Behavior on border.color { ColorAnimation { duration: 150 } }

            Column {
                anchors.centerIn: parent
                width: parent.width - 16
                spacing: 6

                Text {
                    text: menuContainer.hoveredIndex !== -1
                          ? (menuModel.get(menuContainer.hoveredIndex).icon !== ""
                             ? menuModel.get(menuContainer.hoveredIndex).icon
                             : menuModel.get(menuContainer.hoveredIndex).label.charAt(0).toUpperCase())
                          : "◎"
                    font.pixelSize: menuContainer.hoveredIndex !== -1
                                    && menuModel.get(menuContainer.hoveredIndex).icon !== "" ? 32 : 24
                    font.bold: true
                    color: menuContainer.hoveredIndex !== -1 ? "#c4b5fd" : "#52525b"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    text: menuContainer.hoveredIndex !== -1
                          ? menuModel.get(menuContainer.hoveredIndex).label
                          : "drmenu"
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    elide: Text.ElideRight
                    width: parent.width
                }
            }
        }
    }
}
