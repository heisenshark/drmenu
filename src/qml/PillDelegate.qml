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

    Behavior on scale { NumberAnimation { duration: 50; easing.type: Easing.OutQuad } }
    scale: isHovered ? 1.10 : 1.0

    Rectangle {
        anchors.fill: parent
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

        Behavior on color        { ColorAnimation { duration: 40 } }
        Behavior on border.color { ColorAnimation { duration: 40 } }

        Row {
            id: pillRow
            anchors.centerIn: parent
            spacing: 7

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
