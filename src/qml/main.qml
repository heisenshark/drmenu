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
        NumberAnimation { target: menuContainer; property: "scale";   from: 0.6; to: 1.0; duration: 180; easing.type: Easing.OutCubic }
        NumberAnimation { target: menuContainer; property: "opacity"; from: 0.0; to: 1.0; duration: 140 }
    }

    ListModel {
        id: menuModel
    }

    Connections {
        target: output
        function onItemsChanged() {
            menuModel.clear()
            let list = output.items
            if (list) {
                for (let i = 0; i < list.length; ++i) {
                    let e = list[i]
                    menuModel.append({
                        label:   e.label   || "",
                        icon:    e.icon    || "",
                        command: e.command || ""
                    })
                }
            }
        }
    }

    Component.onCompleted: {
        if (typeof output !== "undefined" && output.items) {
            let list = output.items
            for (let i = 0; i < list.length; ++i) {
                let e = list[i]
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
        scale: 0.6
        focus: true

        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Escape) {
                output.cancel()
                event.accepted = true
            }
        }
        Component.onCompleted: forceActiveFocus()

        // ── Blender Radial Geometry ─────────────────────────────────────────
        property real radiusDistance: Math.min(170, Math.min(root.width, root.height) * 0.22)
        property real margin: radiusDistance + 100
        property real centerX: Math.max(margin, Math.min(width  - margin, root.menuX))
        property real centerY: Math.max(margin, Math.min(height - margin, root.menuY))
        property int  itemCount: menuModel.count
        property int  hoveredIndex: -1

        // ── Dim background ───────────────────────────────────────────────────
        Rectangle {
            anchors.fill: parent
            color: "#000000"
            opacity: 0.35
        }

        // ── Mouse tracking ────────────────────────────────────────────────────
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true

            onPositionChanged: (mouse) => {
                let dx = mouse.x - menuContainer.centerX
                let dy = mouse.y - menuContainer.centerY
                let dist = Math.sqrt(dx*dx + dy*dy)

                // Deadzone in the center (radius 18)
                if (dist > 18) {
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

        // ── Directional pointer line to hovered item (Blender style) ─────────
        Shape {
            anchors.fill: parent
            visible: menuContainer.hoveredIndex !== -1
            layer.enabled: true
            layer.samples: 4

            ShapePath {
                strokeWidth: 2
                strokeColor: "#e67e22" // Blender signature orange accent
                fillColor: "transparent"

                PathMove {
                    x: menuContainer.centerX
                    y: menuContainer.centerY
                }
                PathLine {
                    x: {
                        if (menuContainer.hoveredIndex === -1) return menuContainer.centerX
                        let sliceAngle = 360 / menuContainer.itemCount
                        let midAngleRad = (menuContainer.hoveredIndex * sliceAngle - 90 + sliceAngle / 2) * Math.PI / 180
                        return menuContainer.centerX + (menuContainer.radiusDistance - 20) * Math.cos(midAngleRad)
                    }
                    y: {
                        if (menuContainer.hoveredIndex === -1) return menuContainer.centerY
                        let sliceAngle = 360 / menuContainer.itemCount
                        let midAngleRad = (menuContainer.hoveredIndex * sliceAngle - 90 + sliceAngle / 2) * Math.PI / 180
                        return menuContainer.centerY + (menuContainer.radiusDistance - 20) * Math.sin(midAngleRad)
                    }
                }
            }
        }

        // ── Subtle radial guide ring (Blender pie background) ────────────────
        Rectangle {
            x: menuContainer.centerX - width / 2
            y: menuContainer.centerY - height / 2
            width: menuContainer.radiusDistance * 2
            height: menuContainer.radiusDistance * 2
            radius: width / 2
            color: "transparent"
            border.color: "#ffffff"
            border.width: 1
            opacity: 0.07
        }

        // ── Radial Pie Buttons (Blender floating pill style) ───────────────────
        Repeater {
            model: menuModel
            delegate: Item {
                id: pillDelegate

                property bool   isHovered:   menuContainer.hoveredIndex === index
                property real   sliceAngle:  360 / menuContainer.itemCount
                property real   midAngleRad: (index * sliceAngle - 90 + sliceAngle / 2) * Math.PI / 180
                property string displayIcon: model.icon !== "" ? model.icon : model.label.charAt(0).toUpperCase()

                // Calculate center position along radial vector
                property real targetX: menuContainer.centerX + menuContainer.radiusDistance * Math.cos(midAngleRad)
                property real targetY: menuContainer.centerY + menuContainer.radiusDistance * Math.sin(midAngleRad)

                x: targetX - width / 2
                y: targetY - height / 2
                width: pillRow.implicitWidth + 24
                height: 38

                Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }
                scale: isHovered ? 1.1 : 1.0

                // Pill Container Box
                Rectangle {
                    anchors.fill: parent
                    radius: height / 2
                    color: pillDelegate.isHovered ? "#2b2b36" : "#1a1a20"
                    border.color: pillDelegate.isHovered ? "#e67e22" : "#383842"
                    border.width: pillDelegate.isHovered ? 2 : 1

                    Behavior on color { ColorAnimation { duration: 100 } }
                    Behavior on border.color { ColorAnimation { duration: 100 } }

                    // Horizontal layout for Blender pie menu button content
                    Row {
                        id: pillRow
                        anchors.centerIn: parent
                        spacing: 8

                        Text {
                            text: pillDelegate.displayIcon
                            font.pixelSize: model.icon !== "" ? 18 : 14
                            font.bold: model.icon === ""
                            color: pillDelegate.isHovered ? "#f39c12" : "#a0a0ab"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: model.label
                            color: pillDelegate.isHovered ? "#ffffff" : "#d0d0d5"
                            font.bold: pillDelegate.isHovered
                            font.pixelSize: 13
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }

        // ── Blender Center Pivot Dot/Ring ────────────────────────────────────
        Rectangle {
            x: menuContainer.centerX - width / 2
            y: menuContainer.centerY - height / 2
            width: 26
            height: 26
            radius: 13
            color: "#18181c"
            border.color: menuContainer.hoveredIndex !== -1 ? "#e67e22" : "#4a4a56"
            border.width: 2

            Behavior on border.color { ColorAnimation { duration: 120 } }

            // Inner core indicator dot
            Rectangle {
                anchors.centerIn: parent
                width: menuContainer.hoveredIndex !== -1 ? 10 : 6
                height: width
                radius: width / 2
                color: menuContainer.hoveredIndex !== -1 ? "#e67e22" : "#808090"

                Behavior on width { NumberAnimation { duration: 100 } }
                Behavior on color { ColorAnimation { duration: 100 } }
            }
        }
    }
}
