import QtQuick

Item {
    id: pillDelegate
    visible: !rootRef.isPieMode

    required property int    index
    required property string label
    required property string icon
    required property string iconName
    required property string command
    required property string submenuName
    required property string key

    property var rootRef: root
    property var menuContainerRef: menuContainer

    property bool   isHovered:   menuContainerRef.hoveredIndex === index
    property bool   isSubmenu:   submenuName !== ""
    property bool   hasXdgIcon:  iconName !== ""
    property bool   hasEmoji:    icon !== ""
    property string shapeType:   rootRef.s.shape || "pill"

    property real   midAngleRad: rootRef.getItemAngleRad(index, menuContainerRef.itemCount)
    property real   targetX: menuContainerRef.centerX + menuContainerRef.radiusDistance * Math.cos(midAngleRad)
    property real   targetY: menuContainerRef.centerY + menuContainerRef.radiusDistance * Math.sin(midAngleRad)

    x: targetX - width  / 2
    y: targetY - height / 2

    width:  shapeType === "circle" ? height : (pillRow.implicitWidth + 28)
    height: shapeType === "circle" ? (rootRef.s.pillHeight || 48) : (rootRef.s.pillHeight || 42)

    Behavior on scale {
        NumberAnimation {
            duration: 90
            easing.type: Easing.OutBack
            easing.overshoot: 1.18
        }
    }
    scale: isHovered ? 1.09 : 1.0

    // ── Main Glass Body ────────────────────────────────────────────────
    Rectangle {
        id: pillBody
        anchors.fill: parent
        antialiasing: true
        smooth: true
        radius: {
            if (pillDelegate.shapeType === "circle") return height / 2
            if (pillDelegate.shapeType === "rectangle") return 0
            if (pillDelegate.shapeType === "rounded") return 8
            return rootRef.s.pillRadius !== undefined ? rootRef.s.pillRadius : height / 2
        }

        color: pillDelegate.isHovered
            ? (pillDelegate.isSubmenu ? (rootRef.s.pillSubmenuHoverColor || "#2a2438") : (rootRef.s.pillHoverColor || "#2b2b36"))
            : (pillDelegate.isSubmenu ? (rootRef.s.pillSubmenuColor || "#18151f") : (rootRef.s.pillColor || "#1a1a20"))

        border.color: pillDelegate.isHovered
            ? (pillDelegate.isSubmenu ? (rootRef.s.pillSubmenuBorderHover || "#a855f7") : (rootRef.s.pillBorderHoverColor || "#e67e22"))
            : (pillDelegate.isSubmenu ? (rootRef.s.pillSubmenuBorder || "#4a3060") : (rootRef.s.pillBorderColor || "#383842"))

        border.width: pillDelegate.isHovered
            ? (rootRef.s.borderHoverWidth || 2)
            : (rootRef.s.borderWidth || 1)

        layer.enabled: true
        layer.smooth: true

        Behavior on color        { ColorAnimation { duration: 60; easing.type: Easing.OutQuad } }
        Behavior on border.color { ColorAnimation { duration: 60; easing.type: Easing.OutQuad } }

        // ── Screencopy Optical Glass Blur Layer ────────────────────────
        Item {
            anchors.fill: parent
            anchors.margins: pillBody.border.width
            visible: (rootRef.s.useScreencopyGlass === true) && (typeof screenGrabber !== "undefined") && (screenGrabber.revision > 0)
            clip: true
            z: -1

            Image {
                visible: parent.visible
                x: -(pillDelegate.x - (menuContainer.centerX - menuContainer.margin)) - pillBody.border.width
                y: -(pillDelegate.y - (menuContainer.centerY - menuContainer.margin)) - pillBody.border.width
                width: menuContainer.margin * 2
                height: menuContainer.margin * 2
                source: (typeof screenGrabber !== "undefined" && screenGrabber.revision > 0)
                    ? ("image://screengrab/blurred?rev=" + screenGrabber.revision)
                    : ""
                smooth: true
                cache: false
            }
        }

        // ── Specular Fresnel Gloss Sheen (Inset inside border) ─────────
        Rectangle {
            anchors.fill: parent
            anchors.margins: pillBody.border.width
            radius: Math.max(0, pillBody.radius - pillBody.border.width)
            color: "transparent"
            opacity: pillDelegate.isHovered ? 0.90 : 0.60
            z: 0

            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.00; color: pillDelegate.isHovered ? "#35ffffff" : "#20ffffff" }
                GradientStop { position: 0.40; color: "#00ffffff" }
                GradientStop { position: 1.00; color: pillDelegate.isHovered ? "#18ffffff" : "#08ffffff" }
            }

            Behavior on opacity { NumberAnimation { duration: 60 } }
        }

        // ── Dynamic Liquid Caustic Bloom on Hover ───────────────────────
        Rectangle {
            anchors.fill: parent
            anchors.margins: pillBody.border.width
            radius: Math.max(0, pillBody.radius - pillBody.border.width)
            visible: pillDelegate.isHovered
            opacity: pillDelegate.isHovered ? 0.30 : 0.0
            color: pillDelegate.isSubmenu ? (rootRef.s.submenuAccent || "#bf5af2") : (rootRef.s.accentColor || "#0a84ff")
            z: 0
            Behavior on opacity { NumberAnimation { duration: 60 } }
        }

        Row {
            id: pillRow
            anchors.centerIn: parent
            spacing: 7

            Text {
                visible: rootRef.s.showNumberBadges !== false
                text: (pillDelegate.key && pillDelegate.key !== "")
                    ? pillDelegate.key.toUpperCase()
                    : (pillDelegate.index < 9 ? (pillDelegate.index + 1) : (pillDelegate.index === 9 ? "0" : ""))
                font.family: rootRef.s.fontFamily || "Sans"
                font.pixelSize: Math.max(9, (rootRef.s.fontSize || 13) - 3)
                font.bold: true
                color: pillDelegate.isHovered
                       ? (rootRef.s.accentColor || "#e67e22")
                       : (rootRef.s.numberBadgeColor || "#686878")
                anchors.verticalCenter: parent.verticalCenter
            }

            Image {
                visible: pillDelegate.hasXdgIcon
                width:   visible ? (rootRef.s.iconSize || 22) : 0
                height:  rootRef.s.iconSize || 22
                source:  pillDelegate.hasXdgIcon ? "image://icon/" + pillDelegate.iconName : ""
                anchors.verticalCenter: parent.verticalCenter
                smooth: true; mipmap: true
                opacity: pillDelegate.isHovered ? 1.0 : 0.75
                Behavior on opacity { NumberAnimation { duration: 40 } }
            }

            Text {
                visible: !pillDelegate.hasXdgIcon
                text:    pillDelegate.hasEmoji
                         ? pillDelegate.icon
                         : (pillDelegate.isSubmenu ? "☰" : pillDelegate.label.charAt(0).toUpperCase())
                font.family:    rootRef.s.fontFamily || "Sans"
                font.pixelSize: pillDelegate.hasEmoji ? ((rootRef.s.fontSize || 13) + 4) : (rootRef.s.fontSize || 13)
                font.bold:      !pillDelegate.hasEmoji
                color: pillDelegate.isHovered
                       ? (pillDelegate.isSubmenu ? (rootRef.s.submenuAccent || "#c084fc") : (rootRef.s.iconHoverColor || "#f39c12"))
                       : (rootRef.s.iconColor || "#9090a0")
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                visible: pillDelegate.shapeType !== "circle" || pillDelegate.isHovered
                text:        pillDelegate.label
                font.family: rootRef.s.fontFamily || "Sans"
                font.pixelSize: rootRef.s.fontSize || 13
                font.bold:   pillDelegate.isHovered
                color:       pillDelegate.isHovered
                             ? (rootRef.s.textHoverColor || "#ffffff")
                             : (rootRef.s.textColor || "#d0d0d5")
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                visible: pillDelegate.isSubmenu && pillDelegate.shapeType !== "circle"
                text:    "▶"
                font.pixelSize: Math.max(8, (rootRef.s.fontSize || 13) - 4)
                color: pillDelegate.isHovered
                       ? (rootRef.s.submenuAccent || "#c084fc")
                       : (rootRef.s.pillSubmenuBorder || "#5a4070")
                anchors.verticalCenter: parent.verticalCenter
                leftPadding: 1
                Behavior on color { ColorAnimation { duration: 40 } }
            }
        }
    }
}
