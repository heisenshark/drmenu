import QtQuick

Rectangle {
    id: centerPivot

    property var rootRef: root
    property var menuContainerRef: menuContainer

    x: menuContainerRef.centerX - width  / 2
    y: menuContainerRef.centerY - height / 2
    width:  rootRef.isPieMode ? ((rootRef.s.innerRadius || 65) * 2) : ((rootRef.s.centerRadius || 15) * 2)
    height: width
    radius: width / 2
    color:  rootRef.s.centerColor || "#111116"
    border.color: menuContainerRef.hoveredIndex !== -1
        ? (rootRef.s.centerBorderHoverColor || rootRef.s.centerBorderHover || "#e67e22")
        : (rootRef.hasParent
            ? (rootRef.s.submenuAccent || "#c084fc")
            : (rootRef.s.centerBorderColor || rootRef.s.centerBorder || "#5a5a72"))
    border.width: rootRef.s.centerBorderWidth !== undefined ? rootRef.s.centerBorderWidth : 3
    z: 10

    Behavior on border.color { ColorAnimation { duration: 50 } }

    Text {
        anchors.centerIn: parent
        visible: rootRef.hasParent && menuContainerRef.hoveredIndex === -1
        text: "←"
        font.family: rootRef.s.fontFamily || "Sans"
        font.pixelSize: (rootRef.s.fontSize || 13) + 2
        color: rootRef.s.submenuAccent || "#c084fc"
    }

    Rectangle {
        anchors.centerIn: parent
        visible: !(rootRef.hasParent && menuContainerRef.hoveredIndex === -1)
        width:  menuContainerRef.hoveredIndex !== -1 ? 12 : 7
        height: width
        radius: width / 2
        color:  menuContainerRef.hoveredIndex !== -1
                ? (rootRef.s.centerDotHoverColor || "#e67e22")
                : (rootRef.s.centerDotColor || "#808090")

        Behavior on width { NumberAnimation { duration: 40 } }
        Behavior on color { ColorAnimation  { duration: 40 } }
    }
}
