import QtQuick
import QtQuick.Shapes

Item {
    id: pieDelimitersOverlay
    visible: rootRef.isPieMode && (rootRef.s.delimiterWidth !== 0)
    z: 1
    anchors.fill: parent

    property var rootRef: root
    property var menuContainerRef: menuContainer

    property real innerR: rootRef.s.innerRadius || 65
    property real outerR: rootRef.s.outerRadius || 230
    property int  dWidth: rootRef.s.delimiterWidth !== undefined ? rootRef.s.delimiterWidth : 3
    property color dColor: rootRef.s.delimiterColor || "#383848"

    // Inner Ring Delimiter (Ring between center hole and pie slices)
    Rectangle {
        x: menuContainerRef.centerX - pieDelimitersOverlay.innerR
        y: menuContainerRef.centerY - pieDelimitersOverlay.innerR
        width:  pieDelimitersOverlay.innerR * 2
        height: pieDelimitersOverlay.innerR * 2
        radius: pieDelimitersOverlay.innerR
        color: "transparent"
        border.color: pieDelimitersOverlay.dColor
        border.width: pieDelimitersOverlay.dWidth
    }

    // Outer Ring Delimiter (Outer boundary border of pie wheel)
    Rectangle {
        x: menuContainerRef.centerX - pieDelimitersOverlay.outerR
        y: menuContainerRef.centerY - pieDelimitersOverlay.outerR
        width:  pieDelimitersOverlay.outerR * 2
        height: pieDelimitersOverlay.outerR * 2
        radius: pieDelimitersOverlay.outerR
        color: "transparent"
        border.color: rootRef.s.pieOuterBorderColor || pieDelimitersOverlay.dColor
        border.width: rootRef.s.pieOuterBorderWidth !== undefined ? rootRef.s.pieOuterBorderWidth : 3
    }

    // Radial Spokes Delimiters (Divider lines running from inner to outer circle)
    Repeater {
        model: menuContainerRef.itemCount
        delegate: Shape {
            anchors.fill: parent
            layer.enabled: true
            layer.samples: 4

            property real count: menuContainerRef.itemCount
            property real sliceAngleDeg: 360 / count
            property real startDeg: index * sliceAngleDeg - 90
            property real startRad: startDeg * Math.PI / 180

            ShapePath {
                strokeWidth: pieDelimitersOverlay.dWidth
                strokeColor: pieDelimitersOverlay.dColor
                fillColor: "transparent"

                PathMove {
                    x: menuContainerRef.centerX + pieDelimitersOverlay.innerR * Math.cos(startRad)
                    y: menuContainerRef.centerY + pieDelimitersOverlay.innerR * Math.sin(startRad)
                }
                PathLine {
                    x: menuContainerRef.centerX + pieDelimitersOverlay.outerR * Math.cos(startRad)
                    y: menuContainerRef.centerY + pieDelimitersOverlay.outerR * Math.sin(startRad)
                }
            }
        }
    }
}
