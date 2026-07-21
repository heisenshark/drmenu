import QtQuick
import QtQuick.Shapes

Item {
    id: pieSectorDelegate
    visible: root.isPieMode
    z: isHovered ? 5 : 0

    required property int    index
    required property string label
    required property string icon
    required property string iconName
    required property string command
    required property string submenuName
    required property string key

    property var rootRef: root
    property var menuContainerRef: menuContainer

    property bool isHovered: menuContainerRef.hoveredIndex === index
    property bool isSubmenu: submenuName !== ""
    property bool hasXdgIcon: iconName !== ""
    property bool hasEmoji:   icon !== ""

    property real innerR: rootRef.s.innerRadius || 65
    property real outerR: isHovered ? ((rootRef.s.outerRadius || 230) + 12) : (rootRef.s.outerRadius || 230)

    property real count: menuContainerRef.itemCount
    property real sliceAngleDeg: 360 / count

    property real startDeg: index * sliceAngleDeg - 90
    property real endDeg:   (index + 1) * sliceAngleDeg - 90

    property real startRad: startDeg * Math.PI / 180
    property real endRad:   endDeg   * Math.PI / 180
    property real midRad:   (startDeg + sliceAngleDeg / 2) * Math.PI / 180

    property real contentRadius: (innerR + (rootRef.s.outerRadius || 230)) / 2
    property real contentX: menuContainerRef.centerX + contentRadius * Math.cos(midRad)
    property real contentY: menuContainerRef.centerY + contentRadius * Math.sin(midRad)

    property string wedgeSvgPath: {
        let x0 = menuContainerRef.centerX + innerR * Math.cos(startRad)
        let y0 = menuContainerRef.centerY + innerR * Math.sin(startRad)
        let x1 = menuContainerRef.centerX + outerR * Math.cos(startRad)
        let y1 = menuContainerRef.centerY + outerR * Math.sin(startRad)
        let x2 = menuContainerRef.centerX + outerR * Math.cos(endRad)
        let y2 = menuContainerRef.centerY + outerR * Math.sin(endRad)
        let x3 = menuContainerRef.centerX + innerR * Math.cos(endRad)
        let y3 = menuContainerRef.centerY + innerR * Math.sin(endRad)
        let largeArc = sliceAngleDeg > 180 ? 1 : 0

        return "M " + x0.toFixed(2) + " " + y0.toFixed(2) +
               " L " + x1.toFixed(2) + " " + y1.toFixed(2) +
               " A " + outerR.toFixed(2) + " " + outerR.toFixed(2) + " 0 " + largeArc + " 1 " + x2.toFixed(2) + " " + y2.toFixed(2) +
               " L " + x3.toFixed(2) + " " + y3.toFixed(2) +
               " A " + innerR.toFixed(2) + " " + innerR.toFixed(2) + " 0 " + largeArc + " 0 " + x0.toFixed(2) + " " + y0.toFixed(2) +
               " Z"
    }

    Shape {
        width: menuContainerRef.width
        height: menuContainerRef.height

        ShapePath {
            strokeWidth: (rootRef.s.delimiterWidth === 0) ? 0 : (pieSectorDelegate.isHovered ? 3 : (rootRef.s.delimiterWidth !== undefined ? rootRef.s.delimiterWidth : 2))
            strokeColor: (rootRef.s.delimiterWidth === 0) ? "transparent" : (pieSectorDelegate.isHovered
                ? (rootRef.s.accentColor || "#e67e22")
                : (rootRef.s.delimiterColor || "#383848"))

            fillColor: pieSectorDelegate.isHovered
                ? (rootRef.s.pieSliceHoverColor || "#323246")
                : (rootRef.s.pieSliceColor || "#1e1e2a")

            Behavior on fillColor { ColorAnimation { duration: 40 } }

            PathSvg {
                path: pieSectorDelegate.wedgeSvgPath
            }
        }
    }

    // Sector Content (Icon + Label centered in wedge)
    Item {
        x: pieSectorDelegate.contentX - width / 2
        y: pieSectorDelegate.contentY - height / 2
        width: pieRow.implicitWidth + (rootRef.s.highlightOptionRect ? 18 : 0)
        height: pieRow.implicitHeight + (rootRef.s.highlightOptionRect ? 10 : 0)

        scale: pieSectorDelegate.isHovered ? 1.06 : 1.0
        Behavior on scale { NumberAnimation { duration: 40 } }

        Rectangle {
            anchors.fill: parent
            visible: (rootRef.s.highlightOptionRect === true) || (rootRef.s.showOptionRectAlways === true)
            radius: rootRef.s.optionRectRadius !== undefined ? rootRef.s.optionRectRadius : 6
            color: pieSectorDelegate.isHovered
                ? (rootRef.s.optionRectHoverColor || "#2b5b88")
                : (rootRef.s.optionRectColor || "#222226")
            border.color: pieSectorDelegate.isHovered
                ? (rootRef.s.optionRectHoverBorder || "#3b82f6")
                : (rootRef.s.optionRectBorder || "#333338")
            border.width: 1

            Behavior on color        { ColorAnimation { duration: 40 } }
            Behavior on border.color { ColorAnimation { duration: 40 } }
        }

        Row {
            id: pieRow
            spacing: 6
            anchors.centerIn: parent

            Text {
                visible: rootRef.s.showNumberBadges !== false
                text: (pieSectorDelegate.key && pieSectorDelegate.key !== "")
                    ? pieSectorDelegate.key.toUpperCase()
                    : (pieSectorDelegate.index < 9 ? (pieSectorDelegate.index + 1) : (pieSectorDelegate.index === 9 ? "0" : ""))
                font.family: rootRef.s.fontFamily || "Sans"
                font.pixelSize: Math.max(9, (rootRef.s.fontSize || 13) - 3)
                font.bold: true
                color: pieSectorDelegate.isHovered
                       ? (rootRef.s.accentColor || "#3b82f6")
                       : (rootRef.s.numberBadgeColor || "#686878")
                anchors.verticalCenter: parent.verticalCenter
            }

            Image {
                visible: pieSectorDelegate.hasXdgIcon
                width:   visible ? (rootRef.s.iconSize || 20) : 0
                height:  rootRef.s.iconSize || 20
                source:  pieSectorDelegate.hasXdgIcon ? "image://icon/" + pieSectorDelegate.iconName : ""
                anchors.verticalCenter: parent.verticalCenter
                smooth: true; mipmap: true
            }

            Text {
                visible: !pieSectorDelegate.hasXdgIcon
                text:    pieSectorDelegate.hasEmoji
                         ? pieSectorDelegate.icon
                         : (pieSectorDelegate.isSubmenu ? "☰" : pieSectorDelegate.label.charAt(0).toUpperCase())
                font.family:    rootRef.s.fontFamily || "Sans"
                font.pixelSize: (rootRef.s.fontSize || 13)
                color: pieSectorDelegate.isHovered
                       ? (pieSectorDelegate.isSubmenu ? (rootRef.s.submenuAccent || "#c084fc") : (rootRef.s.iconHoverColor || "#f39c12"))
                       : (rootRef.s.iconColor || "#a0a0b0")
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: pieSectorDelegate.label
                font.family: rootRef.s.fontFamily || "Sans"
                font.pixelSize: rootRef.s.fontSize || 13
                font.bold: pieSectorDelegate.isHovered
                color: pieSectorDelegate.isHovered ? (rootRef.s.textHoverColor || "#ffffff") : (rootRef.s.textColor || "#d0d0d8")
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                visible: pieSectorDelegate.isSubmenu
                text: "▶"
                font.pixelSize: 9
                color: pieSectorDelegate.isHovered ? (rootRef.s.submenuAccent || "#c084fc") : "#605075"
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
