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

    // ── Style helper accessor ──────────────────────────────────────────────────
    readonly property var  s: output.style || {}
    readonly property bool isPieMode: root.s.layout === "pie" || root.s.layout === "wheel"

    onVisibleChanged: {
        if (visible) {
            root.raise()
            root.requestActivate()
            menuContainer.opacity = 0
            menuContainer.scale = 0.92
            openAnimation.start()
        } else {
            openAnimation.stop()
            menuContainer.opacity = 0
            menuContainer.scale = 1.0
        }
    }

    // ── Angle distribution helper for layouts & arcs ───────────────────────────
    function getItemAngleRad(idx, totalCount) {
        if (totalCount <= 0) return 0

        let layout    = root.s.layout || "circle"
        let startDeg  = root.s.startAngle  !== undefined ? root.s.startAngle  : -90
        let spreadDeg = root.s.spreadAngle !== undefined ? root.s.spreadAngle : 360

        if (layout === "shotgun") {
            if (root.s.startAngle  === undefined) startDeg  = -150
            if (root.s.spreadAngle === undefined) spreadDeg = 120
        } else if (layout === "semicircle" || layout === "semicircle-top") {
            if (root.s.startAngle  === undefined) startDeg  = -180
            if (root.s.spreadAngle === undefined) spreadDeg = 180
        } else if (layout === "semicircle-bottom") {
            if (root.s.startAngle  === undefined) startDeg  = 0
            if (root.s.spreadAngle === undefined) spreadDeg = 180
        } else if (layout === "arc-right") {
            if (root.s.startAngle  === undefined) startDeg  = -45
            if (root.s.spreadAngle === undefined) spreadDeg = 90
        } else if (layout === "arc-left") {
            if (root.s.startAngle  === undefined) startDeg  = 135
            if (root.s.spreadAngle === undefined) spreadDeg = 90
        }

        if (spreadDeg >= 360) {
            let step = 360 / totalCount
            let deg = startDeg + idx * step + step / 2
            return deg * Math.PI / 180
        } else {
            let step = totalCount > 1 ? spreadDeg / (totalCount - 1) : 0
            let deg = startDeg + idx * step
            return deg * Math.PI / 180
        }
    }

    // ── Navigation state ───────────────────────────────────────────────────────
    property var  menuStack:    []     // stack of menu names (strings)
    property bool hasParent:    false  // true when not at root

    // If spawnAtMouse is true -> move to mouse position
    // If spawnAtMouse is false & isInitial -> center on screen
    // If spawnAtMouse is false & NOT initial -> KEEP former menu's exact position!
    function updatePositionForMenu(menuData, isInitial) {
        let spawnAtMouse = (menuData && menuData.spawnAtMouse !== undefined)
            ? menuData.spawnAtMouse
            : output.spawnAtMouse

        if (spawnAtMouse) {
            let mPos = output.getMousePosition()
            if (mPos && typeof mPos.x === "number" && mPos.x >= 0 && typeof mPos.y === "number" && mPos.y >= 0) {
                root.menuX = mPos.x
                root.menuY = mPos.y
            }
        } else if (isInitial) {
            root.menuX = root.width / 2
            root.menuY = root.height / 2
        }
        // Note: when spawnAtMouse is false and isInitial is false,
        // root.menuX and root.menuY remain unchanged (exact former menu position).
    }

    function navigateTo(menuName) {
        let menuData = output.menus[menuName]
        if (!menuData) return

        let targetItems = (menuData.items !== undefined) ? menuData.items : menuData
        if (!targetItems || targetItems.length === 0) return

        updatePositionForMenu(menuData, false)

        if (menuData.style) {
            output.setStyle(menuData.style)
        }

        menuStack = menuStack.concat([menuName])
        hasParent = menuStack.length > 1
        loadItems(targetItems)
    }

    function goBack() {
        if (menuStack.length <= 1) {
            menuContainer.opacity = 0
            output.cancel()
            return
        }
        let newStack = menuStack.slice(0, menuStack.length - 1)
        menuStack = newStack
        hasParent = menuStack.length > 1
        let parentName = menuStack[menuStack.length - 1]
        let parentData = output.menus[parentName]
        let parentItems = (parentData && parentData.items !== undefined) ? parentData.items : parentData

        updatePositionForMenu(parentData, false)

        if (parentData && parentData.style) {
            output.setStyle(parentData.style)
        }

        loadItems(parentItems)
    }

    function handleEscape() {
        let currentMenuName = menuStack.length > 0 ? menuStack[menuStack.length - 1] : ""
        let menuData = currentMenuName ? output.menus[currentMenuName] : null
        let closeAll = (menuData && menuData.escapeClosesAll !== undefined)
            ? menuData.escapeClosesAll
            : output.escapeClosesAll

        if (closeAll) {
            menuContainer.opacity = 0
            output.cancel()
        } else {
            root.goBack()
        }
    }

    function triggerSelect(label) {
        menuContainer.opacity = 0
        output.select(label)
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

        function onMenusChanged() {
            if (!output.initialMenu) return
            let initData = output.menus[output.initialMenu]
            if (!initData) return
            let items = (initData.items !== undefined) ? initData.items : initData

            updatePositionForMenu(initData, true)
            if (initData.style) output.setStyle(initData.style)

            menuStack = [output.initialMenu]
            hasParent = false
            loadItems(items)
        }

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
                updatePositionForMenu(initData, true)
                if (initData.style) output.setStyle(initData.style)
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

    // ── Keyboard Shortcuts ─────────────────────────────────────────────────────
    Shortcut { sequence: "Escape"; onActivated: root.handleEscape() }

    // ── Fast, crisp opening animation (30ms) ───────────────────────────────────
    ParallelAnimation {
        id: openAnimation
        NumberAnimation { target: menuContainer; property: "scale";   from: 0.92; to: 1.0; duration: 35; easing.type: Easing.OutQuad }
        NumberAnimation { target: menuContainer; property: "opacity"; from: 0.0;  to: 1.0; duration: 25 }
    }

    ListModel { id: menuModel }

    Item {
        id: menuContainer
        anchors.fill: parent
        opacity: 0
        scale: 0.92
        focus: true

        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Escape) {
                root.handleEscape()
                event.accepted = true
            }
        }
        Component.onCompleted: forceActiveFocus()

        // ── Geometry ────────────────────────────────────────────────────────────
        property real radiusDistance: root.isPieMode ? (root.s.outerRadius || 165) : (root.s.radiusDistance || 185)
        property real margin:  radiusDistance + 110
        property real centerX: Math.max(margin, Math.min(width  - margin, root.menuX))
        property real centerY: Math.max(margin, Math.min(height - margin, root.menuY))
        property int  itemCount:    menuModel.count
        property int  hoveredIndex: -1

        // ── Dim background ───────────────────────────────────────────────────────
        Rectangle {
            anchors.fill: parent
            color: root.s.backgroundColor || "#000000"
            opacity: root.s.backgroundOpacity !== undefined ? root.s.backgroundOpacity : 0.38
        }

        // ── Mouse tracking (Left click to select, Right click to close/back) ────
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            onPositionChanged: (mouse) => {
                let dx = mouse.x - menuContainer.centerX
                let dy = mouse.y - menuContainer.centerY
                let dist = Math.sqrt(dx*dx + dy*dy)

                let minRadius = root.isPieMode ? (root.s.innerRadius || 50) : 20

                if (dist > minRadius) {
                    let mouseAngle = Math.atan2(dy, dx)
                    let closestIdx = -1
                    let minDiff = 1000

                    for (let i = 0; i < menuContainer.itemCount; ++i) {
                        let itemAngle = root.getItemAngleRad(i, menuContainer.itemCount)
                        let diff = Math.abs(mouseAngle - itemAngle)
                        while (diff > Math.PI) diff = Math.abs(diff - 2 * Math.PI)
                        if (diff < minDiff) {
                            minDiff = diff
                            closestIdx = i
                        }
                    }
                    menuContainer.hoveredIndex = closestIdx
                } else {
                    menuContainer.hoveredIndex = -1
                }
            }

            onClicked: (mouse) => {
                if (mouse.button === Qt.RightButton) {
                    root.handleEscape()
                } else if (mouse.button === Qt.LeftButton) {
                    if (menuContainer.hoveredIndex !== -1) {
                        let item = menuModel.get(menuContainer.hoveredIndex)
                        if (item.submenuName !== "") {
                            root.navigateTo(item.submenuName)
                        } else {
                            root.triggerSelect(item.label)
                        }
                    } else {
                        root.goBack()
                    }
                }
            }
        }

        // ── Directional pointer line (Pill mode only) ──────────────────────────
        Shape {
            anchors.fill: parent
            visible: !root.isPieMode && (root.s.showPointerLine !== false) && menuContainer.hoveredIndex !== -1
            layer.enabled: true
            layer.samples:  4

            ShapePath {
                strokeWidth: 2
                strokeColor: {
                    if (menuContainer.hoveredIndex === -1) return "transparent"
                    let item = menuModel.get(menuContainer.hoveredIndex)
                    if (item && item.submenuName !== "") return root.s.submenuAccent || "#c084fc"
                    return root.s.accentColor || "#e67e22"
                }
                fillColor:   "transparent"

                PathMove { x: menuContainer.centerX; y: menuContainer.centerY }
                PathLine {
                    x: {
                        if (menuContainer.hoveredIndex === -1) return menuContainer.centerX
                        let a = root.getItemAngleRad(menuContainer.hoveredIndex, menuContainer.itemCount)
                        return menuContainer.centerX + (menuContainer.radiusDistance - 22) * Math.cos(a)
                    }
                    y: {
                        if (menuContainer.hoveredIndex === -1) return menuContainer.centerY
                        let a = root.getItemAngleRad(menuContainer.hoveredIndex, menuContainer.itemCount)
                        return menuContainer.centerY + (menuContainer.radiusDistance - 22) * Math.sin(a)
                    }
                }
            }
        }

        // ── Guide ring (Pill mode only) ────────────────────────────────────────
        Rectangle {
            visible: !root.isPieMode && root.s.showGuideRing !== false
            x: menuContainer.centerX - width  / 2
            y: menuContainer.centerY - height / 2
            width:  menuContainer.radiusDistance * 2
            height: menuContainer.radiusDistance * 2
            radius: width / 2
            color: "transparent"
            border.color: root.s.guideRingColor || "#ffffff"
            border.width: 1
            opacity: root.s.guideRingOpacity !== undefined ? root.s.guideRingOpacity : 0.07
        }

        // ── MODE 1: Segmented Pie Wheel Sectors (Clear Dividers) ───────────────
        Repeater {
            model: menuModel
            delegate: Item {
                id: pieSectorDelegate
                visible: root.isPieMode

                required property int    index
                required property string label
                required property string icon
                required property string iconName
                required property string command
                required property string submenuName

                property bool isHovered: menuContainer.hoveredIndex === index
                property bool isSubmenu: submenuName !== ""
                property bool hasXdgIcon: iconName !== ""
                property bool hasEmoji:   icon !== ""

                property real innerR: root.s.innerRadius || 50
                property real outerR: isHovered ? ((root.s.outerRadius || 165) + 10) : (root.s.outerRadius || 165)

                property real count: menuContainer.itemCount
                property real sliceAngleDeg: 360 / count

                property real startDeg: index * sliceAngleDeg - 90
                property real endDeg:   (index + 1) * sliceAngleDeg - 90

                property real startRad: startDeg * Math.PI / 180
                property real endRad:   endDeg   * Math.PI / 180
                property real midRad:   (startDeg + sliceAngleDeg / 2) * Math.PI / 180

                property real contentRadius: (innerR + (root.s.outerRadius || 165)) / 2
                property real contentX: menuContainer.centerX + contentRadius * Math.cos(midRad)
                property real contentY: menuContainer.centerY + contentRadius * Math.sin(midRad)

                Shape {
                    anchors.fill: parent
                    layer.enabled: true
                    layer.samples: 4

                    ShapePath {
                        strokeWidth: root.s.delimiterWidth !== undefined ? root.s.delimiterWidth : 2
                        strokeColor: root.s.delimiterColor || "#0d0d12"

                        fillColor: pieSectorDelegate.isHovered
                            ? (pieSectorDelegate.isSubmenu ? (root.s.pieSliceSubmenuHoverColor || "#372750") : (root.s.pieSliceHoverColor || "#2c2c3a"))
                            : (pieSectorDelegate.isSubmenu ? (root.s.pieSliceSubmenuColor || "#1e172a") : (root.s.pieSliceColor || "#1a1a22"))

                        Behavior on fillColor { ColorAnimation { duration: 40 } }

                        PathMove {
                            x: menuContainer.centerX + pieSectorDelegate.innerR * Math.cos(pieSectorDelegate.startRad)
                            y: menuContainer.centerY + pieSectorDelegate.innerR * Math.sin(pieSectorDelegate.startRad)
                        }
                        PathLine {
                            x: menuContainer.centerX + pieSectorDelegate.outerR * Math.cos(pieSectorDelegate.startRad)
                            y: menuContainer.centerY + pieSectorDelegate.outerR * Math.sin(pieSectorDelegate.startRad)
                        }
                        PathAngleArc {
                            centerX: menuContainer.centerX; centerY: menuContainer.centerY
                            radiusX: pieSectorDelegate.outerR; radiusY: pieSectorDelegate.outerR
                            startAngle: pieSectorDelegate.startDeg
                            sweepAngle: pieSectorDelegate.sliceAngleDeg
                        }
                        PathLine {
                            x: menuContainer.centerX + pieSectorDelegate.innerR * Math.cos(pieSectorDelegate.endRad)
                            y: menuContainer.centerY + pieSectorDelegate.innerR * Math.sin(pieSectorDelegate.endRad)
                        }
                        PathAngleArc {
                            centerX: menuContainer.centerX; centerY: menuContainer.centerY
                            radiusX: pieSectorDelegate.innerR; radiusY: pieSectorDelegate.innerR
                            startAngle: pieSectorDelegate.endDeg
                            sweepAngle: -pieSectorDelegate.sliceAngleDeg
                        }
                    }
                }

                // Sector Content (Icon + Label centered in wedge)
                Item {
                    x: pieSectorDelegate.contentX - width / 2
                    y: pieSectorDelegate.contentY - height / 2
                    width: pieRow.implicitWidth
                    height: pieRow.implicitHeight

                    scale: pieSectorDelegate.isHovered ? 1.12 : 1.0
                    Behavior on scale { NumberAnimation { duration: 40 } }

                    Row {
                        id: pieRow
                        spacing: 6
                        anchors.centerIn: parent

                        Image {
                            visible: pieSectorDelegate.hasXdgIcon
                            width:   visible ? (root.s.iconSize || 20) : 0
                            height:  root.s.iconSize || 20
                            source:  pieSectorDelegate.hasXdgIcon ? "image://icon/" + pieSectorDelegate.iconName : ""
                            anchors.verticalCenter: parent.verticalCenter
                            smooth: true; mipmap: true
                        }

                        Text {
                            visible: !pieSectorDelegate.hasXdgIcon
                            text:    pieSectorDelegate.hasEmoji
                                     ? pieSectorDelegate.icon
                                     : (pieSectorDelegate.isSubmenu ? "☰" : pieSectorDelegate.label.charAt(0).toUpperCase())
                            font.family:    root.s.fontFamily || "Sans"
                            font.pixelSize: (root.s.fontSize || 13)
                            color: pieSectorDelegate.isHovered
                                   ? (pieSectorDelegate.isSubmenu ? (root.s.submenuAccent || "#c084fc") : (root.s.iconHoverColor || "#f39c12"))
                                   : (root.s.iconColor || "#a0a0b0")
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: pieSectorDelegate.label
                            font.family: root.s.fontFamily || "Sans"
                            font.pixelSize: root.s.fontSize || 13
                            font.bold: pieSectorDelegate.isHovered
                            color: pieSectorDelegate.isHovered ? (root.s.textHoverColor || "#ffffff") : (root.s.textColor || "#d0d0d8")
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            visible: pieSectorDelegate.isSubmenu
                            text: "▶"
                            font.pixelSize: 9
                            color: pieSectorDelegate.isHovered ? (root.s.submenuAccent || "#c084fc") : "#605075"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }

        // ── MODE 2: Floating Pill / Polygon Delegates ─────────────────────────
        Repeater {
            model: menuModel
            delegate: Item {
                id: pillDelegate
                visible: !root.isPieMode

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
                property string shapeType:   root.s.shape || "pill"

                property real   midAngleRad: root.getItemAngleRad(index, menuContainer.itemCount)
                property real   targetX: menuContainer.centerX + menuContainer.radiusDistance * Math.cos(midAngleRad)
                property real   targetY: menuContainer.centerY + menuContainer.radiusDistance * Math.sin(midAngleRad)

                x: targetX - width  / 2
                y: targetY - height / 2

                width:  shapeType === "circle" ? height : (pillRow.implicitWidth + 28)
                height: shapeType === "circle" ? (root.s.pillHeight || 48) : (root.s.pillHeight || 42)

                Behavior on scale { NumberAnimation { duration: 50; easing.type: Easing.OutQuad } }
                scale: isHovered ? 1.10 : 1.0

                Rectangle {
                    anchors.fill: parent
                    radius: {
                        if (pillDelegate.shapeType === "circle") return height / 2
                        if (pillDelegate.shapeType === "rectangle") return 0
                        if (pillDelegate.shapeType === "rounded") return 8
                        return root.s.pillRadius !== undefined ? root.s.pillRadius : height / 2
                    }

                    color: pillDelegate.isHovered
                        ? (pillDelegate.isSubmenu ? (root.s.pillSubmenuHoverColor || "#2a2438") : (root.s.pillHoverColor || "#2b2b36"))
                        : (pillDelegate.isSubmenu ? (root.s.pillSubmenuColor || "#18151f") : (root.s.pillColor || "#1a1a20"))

                    border.color: pillDelegate.isHovered
                        ? (pillDelegate.isSubmenu ? (root.s.pillSubmenuBorderHover || "#a855f7") : (root.s.pillBorderHoverColor || "#e67e22"))
                        : (pillDelegate.isSubmenu ? (root.s.pillSubmenuBorder || "#4a3060") : (root.s.pillBorderColor || "#383842"))

                    border.width: pillDelegate.isHovered
                        ? (root.s.borderHoverWidth || 2)
                        : (root.s.borderWidth || 1)

                    Behavior on color        { ColorAnimation { duration: 40 } }
                    Behavior on border.color { ColorAnimation { duration: 40 } }

                    Row {
                        id: pillRow
                        anchors.centerIn: parent
                        spacing: 7

                        Image {
                            visible: pillDelegate.hasXdgIcon
                            width:   visible ? (root.s.iconSize || 22) : 0
                            height:  root.s.iconSize || 22
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
                            font.family:    root.s.fontFamily || "Sans"
                            font.pixelSize: pillDelegate.hasEmoji ? ((root.s.fontSize || 13) + 4) : (root.s.fontSize || 13)
                            font.bold:      !pillDelegate.hasEmoji
                            color: pillDelegate.isHovered
                                   ? (pillDelegate.isSubmenu ? (root.s.submenuAccent || "#c084fc") : (root.s.iconHoverColor || "#f39c12"))
                                   : (root.s.iconColor || "#9090a0")
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            visible: pillDelegate.shapeType !== "circle" || pillDelegate.isHovered
                            text:        pillDelegate.label
                            font.family: root.s.fontFamily || "Sans"
                            font.pixelSize: root.s.fontSize || 13
                            font.bold:   pillDelegate.isHovered
                            color:       pillDelegate.isHovered
                                         ? (root.s.textHoverColor || "#ffffff")
                                         : (root.s.textColor || "#d0d0d5")
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            visible: pillDelegate.isSubmenu && pillDelegate.shapeType !== "circle"
                            text:    "▶"
                            font.pixelSize: Math.max(8, (root.s.fontSize || 13) - 4)
                            color: pillDelegate.isHovered
                                   ? (root.s.submenuAccent || "#c084fc")
                                   : (root.s.pillSubmenuBorder || "#5a4070")
                            anchors.verticalCenter: parent.verticalCenter
                            leftPadding: 1
                            Behavior on color { ColorAnimation { duration: 40 } }
                        }
                    }
                }
            }
        }

        // ── Center pivot ─────────────────────────────────────────────────────────
        Rectangle {
            x: menuContainer.centerX - width  / 2
            y: menuContainer.centerY - height / 2
            width:  root.isPieMode ? ((root.s.innerRadius || 50) * 2) : ((root.s.centerRadius || 15) * 2)
            height: width
            radius: width / 2
            color:  root.s.centerColor || "#18181c"
            border.color: menuContainer.hoveredIndex !== -1
                ? (root.s.centerBorderHover || "#e67e22")
                : (root.hasParent ? (root.s.submenuAccent || "#a855f7") : (root.s.centerBorder || "#4a4a56"))
            border.width: 2

            Behavior on border.color { ColorAnimation { duration: 50 } }

            Text {
                anchors.centerIn: parent
                visible: root.hasParent && menuContainer.hoveredIndex === -1
                text: "←"
                font.family: root.s.fontFamily || "Sans"
                font.pixelSize: (root.s.fontSize || 13) + 2
                color: root.s.submenuAccent || "#a855f7"
            }

            Rectangle {
                anchors.centerIn: parent
                visible: !(root.hasParent && menuContainer.hoveredIndex === -1)
                width:  menuContainer.hoveredIndex !== -1 ? 12 : 7
                height: width
                radius: width / 2
                color:  menuContainer.hoveredIndex !== -1
                        ? (root.s.centerDotHoverColor || "#e67e22")
                        : (root.s.centerDotColor || "#808090")

                Behavior on width { NumberAnimation { duration: 40 } }
                Behavior on color { ColorAnimation  { duration: 40 } }
            }
        }

        // ── Current menu breadcrumb label ────────────────────────────────────────
        Text {
            visible: (root.s.showBreadcrumbs !== false) && root.menuStack.length > 0
            x: menuContainer.centerX - width / 2
            y: menuContainer.centerY + (root.isPieMode ? (root.s.innerRadius || 50) : (root.s.centerRadius || 15)) + 10
            text: root.menuStack.join(" › ")
            font.family: root.s.fontFamily || "Sans"
            font.pixelSize: Math.max(9, (root.s.fontSize || 13) - 3)
            color: root.s.breadcrumbColor || "#606070"
            font.letterSpacing: 0.5
        }
    }
}
