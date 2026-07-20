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

    property real menuX: width  / 2
    property real menuY: height / 2

    onVisibleChanged: {
        if (visible) {
            root.raise()
            root.requestActivate()
            openAnimation.start()
        }
    }

    // ── Navigation state ───────────────────────────────────────────────────────
    property var  menuStack:    []     // stack of menu names (strings)
    property bool hasParent:    false  // true when not at root

    function updatePositionForMenu(menuData) {
        let spawnAtMouse = (menuData && menuData.spawnAtMouse !== undefined)
            ? menuData.spawnAtMouse
            : output.spawnAtMouse

        if (spawnAtMouse) {
            let mPos = output.getMousePosition()
            if (mPos && typeof mPos.x === "number" && mPos.x >= 0 && typeof mPos.y === "number" && mPos.y >= 0) {
                root.menuX = mPos.x
                root.menuY = mPos.y
            }
        } else {
            root.menuX = root.width / 2
            root.menuY = root.height / 2
        }
    }

    function navigateTo(menuName) {
        let menuData = output.menus[menuName]
        if (!menuData) return

        let targetItems = (menuData.items !== undefined) ? menuData.items : menuData
        if (!targetItems || targetItems.length === 0) return

        updatePositionForMenu(menuData)

        menuStack = menuStack.concat([menuName])
        hasParent = menuStack.length > 1
        transitionOut.stopped.connect(function onStopped() {
            transitionOut.stopped.disconnect(onStopped)
            loadItems(targetItems)
            transitionIn.start()
        })
        transitionOut.start()
    }

    function goBack() {
        if (menuStack.length <= 1) { output.cancel(); return }
        let newStack = menuStack.slice(0, menuStack.length - 1)
        menuStack = newStack
        hasParent = menuStack.length > 1
        let parentName = menuStack[menuStack.length - 1]
        let parentData = output.menus[parentName]
        let parentItems = (parentData && parentData.items !== undefined) ? parentData.items : parentData

        updatePositionForMenu(parentData)

        transitionOut.stopped.connect(function onStopped() {
            transitionOut.stopped.disconnect(onStopped)
            loadItems(parentItems)
            transitionIn.start()
        })
        transitionOut.start()
    }

    function handleEscape() {
        let currentMenuName = menuStack.length > 0 ? menuStack[menuStack.length - 1] : ""
        let menuData = currentMenuName ? output.menus[currentMenuName] : null
        let closeAll = (menuData && menuData.escapeClosesAll !== undefined)
            ? menuData.escapeClosesAll
            : output.escapeClosesAll

        if (closeAll) {
            output.cancel()
        } else {
            root.goBack()
        }
    }

    function loadItems(itemList) {
        menuModel.clear()
        if (!itemList) return
        for (let i = 0; i < itemList.length; ++i) {
            let e = itemList[i]
            menuModel.append({
                label:       e.label       || "",
                icon:        e.icon        || "",
                iconName:    e.iconName    || "",
                command:     e.command     || "",
                submenuName: e.submenuName || ""
            })
        }
        menuContainer.hoveredIndex = -1
    }

    // ── Data wiring ────────────────────────────────────────────────────────────
    Connections {
        target: output

        // Config / nested menus mode
        function onMenusChanged() {
            if (!output.initialMenu) return
            let initData = output.menus[output.initialMenu]
            if (!initData) return
            let items = (initData.items !== undefined) ? initData.items : initData

            updatePositionForMenu(initData)

            menuStack = [output.initialMenu]
            hasParent = false
            loadItems(items)
        }

        // Inline / stdin mode
        function onItemsChanged() {
            menuStack = []
            hasParent = false

            if (output.spawnAtMouse) {
                let mPos = output.getMousePosition()
                if (mPos && mPos.x >= 0 && mPos.y >= 0) {
                    root.menuX = mPos.x
                    root.menuY = mPos.y
                }
            } else {
                root.menuX = root.width / 2
                root.menuY = root.height / 2
            }

            loadItems(output.items)
        }
    }

    Component.onCompleted: {
        if (typeof output === "undefined") return
        if (output.initialMenu) {
            let initData = output.menus[output.initialMenu]
            if (initData) {
                let items = (initData.items !== undefined) ? initData.items : initData
                updatePositionForMenu(initData)
                menuStack = [output.initialMenu]
                hasParent = false
                loadItems(items)
            }
        } else if (output.items && output.items.length > 0) {
            menuStack = []
            hasParent = false
            loadItems(output.items)
        }
    }

    // ── Keyboard ───────────────────────────────────────────────────────────────
    Shortcut {
        sequence: "Escape"
        onActivated: root.handleEscape()
    }

    // ── Open animation ─────────────────────────────────────────────────────────
    ParallelAnimation {
        id: openAnimation
        NumberAnimation { target: menuContainer; property: "scale";   from: 0.6; to: 1.0; duration: 180; easing.type: Easing.OutCubic }
        NumberAnimation { target: menuContainer; property: "opacity"; from: 0.0; to: 1.0; duration: 140 }
    }

    // ── Navigation transition: fade-scale out then in ──────────────────────────
    SequentialAnimation {
        id: transitionOut
        ParallelAnimation {
            NumberAnimation { target: menuContainer; property: "scale";   to: 0.75; duration: 110; easing.type: Easing.InQuad }
            NumberAnimation { target: menuContainer; property: "opacity"; to: 0.0;  duration: 90  }
        }
    }
    SequentialAnimation {
        id: transitionIn
        ParallelAnimation {
            NumberAnimation { target: menuContainer; property: "scale";   from: 1.2; to: 1.0; duration: 150; easing.type: Easing.OutCubic }
            NumberAnimation { target: menuContainer; property: "opacity"; from: 0.0; to: 1.0; duration: 130 }
        }
    }

    ListModel { id: menuModel }

    Item {
        id: menuContainer
        anchors.fill: parent
        opacity: 0
        scale: 0.6
        focus: true

        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Escape) {
                root.handleEscape()
                event.accepted = true
            }
        }
        Component.onCompleted: forceActiveFocus()

        // ── Geometry ────────────────────────────────────────────────────────────
        property real radiusDistance: Math.min(185, Math.min(root.width, root.height) * 0.22)
        property real margin:  radiusDistance + 110
        property real centerX: Math.max(margin, Math.min(width  - margin, root.menuX))
        property real centerY: Math.max(margin, Math.min(height - margin, root.menuY))
        property int  itemCount:    menuModel.count
        property int  hoveredIndex: -1

        // ── Dim background ───────────────────────────────────────────────────────
        Rectangle {
            anchors.fill: parent
            color: "#000000"
            opacity: 0.38
        }

        // ── Mouse tracking ───────────────────────────────────────────────────────
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true

            onPositionChanged: (mouse) => {
                let dx = mouse.x - menuContainer.centerX
                let dy = mouse.y - menuContainer.centerY
                let dist = Math.sqrt(dx*dx + dy*dy)

                if (dist > 20) {
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
                    if (item.submenuName !== "") {
                        root.navigateTo(item.submenuName)
                    } else {
                        output.select(item.label)
                    }
                } else {
                    root.goBack()
                }
            }
        }

        // ── Directional pointer line (Blender style) ────────────────────────────
        Shape {
            anchors.fill: parent
            visible: menuContainer.hoveredIndex !== -1
            layer.enabled: true
            layer.samples:  4

            ShapePath {
                strokeWidth: 2
                strokeColor: "#e67e22"
                fillColor:   "transparent"

                PathMove { x: menuContainer.centerX; y: menuContainer.centerY }
                PathLine {
                    x: {
                        if (menuContainer.hoveredIndex === -1) return menuContainer.centerX
                        let a = (menuContainer.hoveredIndex * 360 / menuContainer.itemCount - 90 + 180 / menuContainer.itemCount) * Math.PI / 180
                        return menuContainer.centerX + (menuContainer.radiusDistance - 22) * Math.cos(a)
                    }
                    y: {
                        if (menuContainer.hoveredIndex === -1) return menuContainer.centerY
                        let a = (menuContainer.hoveredIndex * 360 / menuContainer.itemCount - 90 + 180 / menuContainer.itemCount) * Math.PI / 180
                        return menuContainer.centerY + (menuContainer.radiusDistance - 22) * Math.sin(a)
                    }
                }
            }
        }

        // ── Guide ring ──────────────────────────────────────────────────────────
        Rectangle {
            x: menuContainer.centerX - width  / 2
            y: menuContainer.centerY - height / 2
            width:  menuContainer.radiusDistance * 2
            height: menuContainer.radiusDistance * 2
            radius: width / 2
            color: "transparent"
            border.color: "#ffffff"
            border.width: 1
            opacity: 0.07
        }

        // ── Pill delegates ──────────────────────────────────────────────────────
        Repeater {
            model: menuModel
            delegate: Item {
                id: pillDelegate

                required property int    index
                required property string label
                required property string icon
                required property string iconName
                required property string command
                required property string submenuName

                property bool   isHovered:   menuContainer.hoveredIndex === index
                property bool   isSubmenu:   submenuName !== ""
                property bool   hasXdgIcon:  iconName !== ""
                property bool   hasEmoji:    icon !== ""
                property real   sliceAngle:  360 / menuContainer.itemCount
                property real   midAngleRad: (index * sliceAngle - 90 + sliceAngle / 2) * Math.PI / 180
                property real   targetX: menuContainer.centerX + menuContainer.radiusDistance * Math.cos(midAngleRad)
                property real   targetY: menuContainer.centerY + menuContainer.radiusDistance * Math.sin(midAngleRad)

                x: targetX - width  / 2
                y: targetY - height / 2
                width:  pillRow.implicitWidth + 28
                height: 42

                Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }
                scale: isHovered ? 1.12 : 1.0

                // ── Pill box ──────────────────────────────────────────────────
                Rectangle {
                    anchors.fill: parent
                    radius: height / 2
                    color: pillDelegate.isHovered
                        ? (pillDelegate.isSubmenu ? "#2a2438" : "#2b2b36")
                        : (pillDelegate.isSubmenu ? "#18151f" : "#1a1a20")
                    border.color: pillDelegate.isHovered
                        ? (pillDelegate.isSubmenu ? "#a855f7" : "#e67e22")
                        : (pillDelegate.isSubmenu ? "#4a3060" : "#383842")
                    border.width: pillDelegate.isHovered ? 2 : 1

                    Behavior on color        { ColorAnimation { duration: 100 } }
                    Behavior on border.color { ColorAnimation { duration: 100 } }

                    Row {
                        id: pillRow
                        anchors.centerIn: parent
                        spacing: 7

                        // XDG system icon
                        Image {
                            visible: pillDelegate.hasXdgIcon
                            width:   visible ? 22 : 0
                            height:  22
                            source:  pillDelegate.hasXdgIcon ? "image://icon/" + pillDelegate.iconName : ""
                            anchors.verticalCenter: parent.verticalCenter
                            smooth: true
                            mipmap: true
                            opacity: pillDelegate.isHovered ? 1.0 : 0.72
                            Behavior on opacity { NumberAnimation { duration: 100 } }
                        }

                        // Emoji / letter fallback
                        Text {
                            visible: !pillDelegate.hasXdgIcon
                            width:   visible ? implicitWidth : 0
                            text:    pillDelegate.hasEmoji
                                     ? pillDelegate.icon
                                     : (pillDelegate.isSubmenu ? "☰" : pillDelegate.label.charAt(0).toUpperCase())
                            font.pixelSize: pillDelegate.hasEmoji ? 17 : 13
                            font.bold:      !pillDelegate.hasEmoji
                            color: pillDelegate.isHovered
                                   ? (pillDelegate.isSubmenu ? "#c084fc" : "#f39c12")
                                   : "#9090a0"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // Label
                        Text {
                            text:  pillDelegate.label
                            color: pillDelegate.isHovered ? "#ffffff" : "#d0d0d5"
                            font.bold:      pillDelegate.isHovered
                            font.pixelSize: 13
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // ▶ Submenu chevron
                        Text {
                            visible: pillDelegate.isSubmenu
                            text:    "▶"
                            font.pixelSize: 9
                            color: pillDelegate.isHovered ? "#c084fc" : "#5a4070"
                            anchors.verticalCenter: parent.verticalCenter
                            leftPadding: 1
                            Behavior on color { ColorAnimation { duration: 100 } }
                        }
                    }
                }
            }
        }

        // ── Center pivot ─────────────────────────────────────────────────────────
        Rectangle {
            x: menuContainer.centerX - width  / 2
            y: menuContainer.centerY - height / 2
            width:  30
            height: 30
            radius: 15
            color: "#18181c"
            border.color: menuContainer.hoveredIndex !== -1 ? "#e67e22"
                          : (root.hasParent ? "#a855f7" : "#4a4a56")
            border.width: 2

            Behavior on border.color { ColorAnimation { duration: 120 } }

            // Back arrow when inside a submenu, dot otherwise
            Text {
                anchors.centerIn: parent
                visible: root.hasParent && menuContainer.hoveredIndex === -1
                text: "←"
                font.pixelSize: 14
                color: "#a855f7"
            }

            Rectangle {
                anchors.centerIn: parent
                visible: !(root.hasParent && menuContainer.hoveredIndex === -1)
                width:  menuContainer.hoveredIndex !== -1 ? 12 : 7
                height: width
                radius: width / 2
                color:  menuContainer.hoveredIndex !== -1 ? "#e67e22" : "#808090"

                Behavior on width { NumberAnimation { duration: 100 } }
                Behavior on color { ColorAnimation  { duration: 100 } }
            }
        }

        // ── Current menu breadcrumb label ────────────────────────────────────────
        Text {
            visible: root.menuStack.length > 0
            x: menuContainer.centerX - width / 2
            y: menuContainer.centerY + 22
            text: root.menuStack.join(" › ")
            color: "#606070"
            font.pixelSize: 10
            font.letterSpacing: 0.5
        }
    }
}
