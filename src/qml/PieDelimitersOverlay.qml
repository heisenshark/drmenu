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

    // Outer Arc Delimiters per sector (follows each sector's dynamic outerR so no static ring stays behind)
    Repeater {
        model: menuContainerRef.itemCount
        delegate: Shape {
            anchors.fill: parent
            layer.enabled: true
            layer.samples: 4

            property real count: menuContainerRef.itemCount
            property real sliceAngleDeg: 360 / count
            property real startDeg: index * sliceAngleDeg - 90
            property real endDeg: (index + 1) * sliceAngleDeg - 90
            property real startRad: startDeg * Math.PI / 180
            property real endRad: endDeg * Math.PI / 180

            property var pItem: (typeof pieRepeater !== "undefined" && pieRepeater) ? pieRepeater.itemAt(index) : null
            property real curOuterR: (pItem && typeof pItem.outerR === "number") ? pItem.outerR : pieDelimitersOverlay.outerR
            property bool isHov: (menuContainerRef.hoveredIndex === index)

            ShapePath {
                strokeWidth: isHov
                    ? (rootRef.s.pieOuterBorderHoverWidth || (pieDelimitersOverlay.dWidth + 1))
                    : (rootRef.s.pieOuterBorderWidth !== undefined ? rootRef.s.pieOuterBorderWidth : pieDelimitersOverlay.dWidth)
                strokeColor: isHov
                    ? (rootRef.s.accentColor || rootRef.s.pieOuterBorderHoverColor || "#0a84ff")
                    : (rootRef.s.pieOuterBorderColor || pieDelimitersOverlay.dColor)
                fillColor: "transparent"

                PathMove {
                    x: menuContainerRef.centerX + curOuterR * Math.cos(startRad)
                    y: menuContainerRef.centerY + curOuterR * Math.sin(startRad)
                }
                PathArc {
                    x: menuContainerRef.centerX + curOuterR * Math.cos(endRad)
                    y: menuContainerRef.centerY + curOuterR * Math.sin(endRad)
                    radiusX: curOuterR
                    radiusY: curOuterR
                    useLargeArc: sliceAngleDeg > 180
                    direction: PathArc.Clockwise
                }
            }
        }
    }

    // Radial Spokes Delimiters (Divider lines running from inner to dynamic outer edge)
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

            property var pItemCur: (typeof pieRepeater !== "undefined" && pieRepeater) ? pieRepeater.itemAt(index) : null
            property var pItemPrev: (typeof pieRepeater !== "undefined" && pieRepeater) ? pieRepeater.itemAt((index - 1 + count) % count) : null
            property real rCur: (pItemCur && typeof pItemCur.outerR === "number") ? pItemCur.outerR : pieDelimitersOverlay.outerR
            property real rPrev: (pItemPrev && typeof pItemPrev.outerR === "number") ? pItemPrev.outerR : pieDelimitersOverlay.outerR
            property real spokeOuterR: Math.max(rCur, rPrev)

            ShapePath {
                strokeWidth: pieDelimitersOverlay.dWidth
                strokeColor: pieDelimitersOverlay.dColor
                fillColor: "transparent"

                PathMove {
                    x: menuContainerRef.centerX + pieDelimitersOverlay.innerR * Math.cos(startRad)
                    y: menuContainerRef.centerY + pieDelimitersOverlay.innerR * Math.sin(startRad)
                }
                PathLine {
                    x: menuContainerRef.centerX + spokeOuterR * Math.cos(startRad)
                    y: menuContainerRef.centerY + spokeOuterR * Math.sin(startRad)
                }
            }
        }
    }
}
