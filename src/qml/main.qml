import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

ApplicationWindow {
    id: root
    visible: false
    width: Screen.width
    height: Screen.height
    title: "drmenu"

    // Set window properties for a launcher: frameless, stays on top, transparent background, bypass window manager (forces float/above on X11)
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.BypassWindowManagerHint
    color: "transparent"

    // Default radial menu coordinate properties (overridden by C++ on active screen)
    property real menuX: width / 2
    property real menuY: height / 2

    onVisibleChanged: {
        if (visible) {
            root.raise()
            root.requestActivate()
            openAnimation.start()
        }
    }

    // Capture Escape key to close the menu
    Shortcut {
        sequence: "Escape"
        onActivated: root.close()
    }

    // Animation to scale up and fade in on startup
    ParallelAnimation {
        id: openAnimation
        NumberAnimation { target: menuContainer; property: "scale"; from: 0.7; to: 1.0; duration: 200; easing.type: Easing.OutBack }
        NumberAnimation { target: menuContainer; property: "opacity"; from: 0.0; to: 1.0; duration: 150 }
    }

    // Menu Item definition
    ListModel {
        id: menuModel
        ListElement { name: "Terminal"; icon: "💻"; command: "kitty"; desc: "Launch default terminal" }
        ListElement { name: "Browser"; icon: "🌐"; command: "firefox"; desc: "Open web browser" }
        ListElement { name: "Files"; icon: "📁"; command: "nautilus"; desc: "Browse files" }
        ListElement { name: "Editor"; icon: "📝"; command: "code"; desc: "Open VS Code / Editor" }
        ListElement { name: "Monitor"; icon: "📊"; command: "btop"; desc: "System monitor" }
        ListElement { name: "Settings"; icon: "⚙️"; command: "systemsettings"; desc: "Configure settings" }
    }

    Item {
        id: menuContainer
        anchors.fill: parent
        opacity: 0
        scale: 0.7
        focus: true

        // Capture keyboard keys directly when focused
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Escape) {
                root.close()
                event.accepted = true
            }
        }

        Component.onCompleted: {
            forceActiveFocus()
        }

        // Core math parameters
        property real margin: outerRadius + 20
        property real centerX: Math.max(margin, Math.min(width - margin, root.menuX))
        property real centerY: Math.max(margin, Math.min(height - margin, root.menuY))
        property real outerRadius: 280
        property real innerRadius: 100
        property int itemCount: menuModel.count
        property int hoveredIndex: -1

        // Mouse Area for calculation of coordinates
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true

            onPositionChanged: (mouse) => {
                let dx = mouse.x - menuContainer.centerX
                let dy = mouse.y - menuContainer.centerY
                let distance = Math.sqrt(dx*dx + dy*dy)

                if (distance >= menuContainer.innerRadius && distance <= menuContainer.outerRadius) {
                    // Compute angle in radians, offset by +90deg (so 12 o'clock is 0 degrees)
                    let angle = Math.atan2(dy, dx) + Math.PI / 2
                    if (angle < 0) angle += 2 * Math.PI

                    let angleDeg = angle * 180 / Math.PI
                    let sliceWidth = 360 / menuContainer.itemCount
                    let index = Math.floor(angleDeg / sliceWidth) % menuContainer.itemCount
                    
                    menuContainer.hoveredIndex = index
                } else {
                    menuContainer.hoveredIndex = -1
                }
            }

            onClicked: {
                if (menuContainer.hoveredIndex !== -1) {
                    let item = menuModel.get(menuContainer.hoveredIndex)
                    launcher.launch(item.command)
                    root.close()
                } else {
                    root.close() // Click outside closes the launcher
                }
            }
        }

        // Draw Slices using Shapes
        Repeater {
            model: menuModel
            delegate: Item {
                id: delegateRoot
                anchors.fill: parent
                
                property bool isHovered: menuContainer.hoveredIndex === index
                property real startAngle: index * (360 / menuContainer.itemCount) - 90
                property real sweepAngle: 360 / menuContainer.itemCount

                // Slice Rendering
                Shape {
                    anchors.fill: parent
                    layer.enabled: true
                    layer.samples: 4 // Antialiasing

                    ShapePath {
                        strokeWidth: 2
                        strokeColor: isHovered ? "#8b5cf6" : "#27272a" // Purple or Zinc-800
                        fillColor: isHovered ? "#3b0764" : "#18181b" // Deep purple or dark zinc

                        Behavior on fillColor { ColorAnimation { duration: 100 } }
                        Behavior on strokeColor { ColorAnimation { duration: 100 } }

                        PathAngleArc {
                            centerX: menuContainer.centerX
                            centerY: menuContainer.centerY
                            radiusX: menuContainer.outerRadius
                            radiusY: menuContainer.outerRadius
                            startAngle: delegateRoot.startAngle
                            sweepAngle: delegateRoot.sweepAngle
                        }
                        PathLine { x: menuContainer.centerX; y: menuContainer.centerY }
                    }
                }

                // Slice Label Content (Emoji & Label)
                Item {
                    // Position label at 65% of the radius
                    property real midAngleRad: (delegateRoot.startAngle + delegateRoot.sweepAngle / 2) * Math.PI / 180
                    property real placementRadius: (menuContainer.outerRadius + menuContainer.innerRadius) / 2 + 10
                    
                    x: menuContainer.centerX + placementRadius * Math.cos(midAngleRad) - width / 2
                    y: menuContainer.centerY + placementRadius * Math.sin(midAngleRad) - height / 2
                    width: 100
                    height: 60

                    Column {
                        anchors.centerIn: parent
                        spacing: 4

                        Text {
                            text: model.icon
                            font.pixelSize: 24
                            horizontalAlignment: Text.AlignHCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Text {
                            text: model.name
                            color: isHovered ? "#ffffff" : "#d4d4d8"
                            font.bold: isHovered
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }
        }

        // Center Ring (Donut Hole) showing description of hovered action
        Rectangle {
            x: menuContainer.centerX - width / 2
            y: menuContainer.centerY - height / 2
            width: menuContainer.innerRadius * 2 - 10
            height: menuContainer.innerRadius * 2 - 10
            radius: width / 2
            
            // Modern glassmorphism look
            color: "#09090b"
            border.color: menuContainer.hoveredIndex !== -1 ? "#8b5cf6" : "#3f3f46"
            border.width: 2

            Behavior on border.color { ColorAnimation { duration: 150 } }

            Column {
                anchors.centerIn: parent
                width: parent.width - 20
                spacing: 8

                // Center Icon/Emoji
                Text {
                    text: menuContainer.hoveredIndex !== -1 ? menuModel.get(menuContainer.hoveredIndex).icon : "🎯"
                    font.pixelSize: 36
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // Center Title
                Text {
                    text: menuContainer.hoveredIndex !== -1 ? menuModel.get(menuContainer.hoveredIndex).name : "drMenu"
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // Center Description
                Text {
                    text: menuContainer.hoveredIndex !== -1 ? menuModel.get(menuContainer.hoveredIndex).desc : "Select action"
                    color: "#a1a1aa"
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }
}
