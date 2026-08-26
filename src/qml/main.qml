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
            root.updateGlassOptics()
            root.triggerScreenCapture()
            menuContainer.opacity = 0
            menuContainer.scale = 0.92
            openAnimation.start()
        } else {
            if (typeof output !== "undefined") {
                output.deactivateGlassShader()
            }
            if (typeof screenGrabber !== "undefined") {
                screenGrabber.stopLiveCapture()
            }
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
        root.updateGlassOptics()
        root.triggerScreenCapture()
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
        root.updateGlassOptics()
        root.triggerScreenCapture()
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
                submenuName: e.submenuName || "",
                key:         e.key         || ""
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

            if (menuStack.length === 1 && menuStack[0] === output.initialMenu && menuModel.count === items.length) {
                return
            }

            menuStack = [output.initialMenu]
            hasParent = false
            loadItems(items)
            root.triggerScreenCapture()
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
            root.triggerScreenCapture()
            menuContainer.opacity = 1.0
            openAnimation.start()
        }
    }

    function updateGlassOptics() {
        if (typeof output === "undefined") return
        let chrom = (root.s.chromaticAberration !== undefined) ? root.s.chromaticAberration : 14.0
        let blurRad = (root.s.screencopyBlurRadius !== undefined) ? root.s.screencopyBlurRadius : 8.0
        let vib = (root.s.screencopyVibrancy !== undefined) ? root.s.screencopyVibrancy : 1.45
        let refr = (root.s.lensRefraction !== undefined) ? root.s.lensRefraction : 0.008

        let count = menuModel.count
        if (count <= 0) return

        let pills = []
        let cx = menuContainer.centerX
        let cy = menuContainer.centerY
        let rDist = root.s.radiusDistance || 185
        let pHalfH = (root.s.pillHeight || 46) / 2
        let pRad = root.s.pillRadius !== undefined ? root.s.pillRadius : pHalfH

        for (let i = 0; i < count; ++i) {
            let angle = root.getItemAngleRad(i, count)
            let px = cx + Math.cos(angle) * rDist
            let py = cy + Math.sin(angle) * rDist
            let pHalfW = 60.0
            pills.push({
                x: px,
                y: py,
                halfWidth: pHalfW,
                halfHeight: pHalfH,
                radius: pRad
            })
        }

        pills.push({
            x: cx,
            y: cy,
            halfWidth: 24.0,
            halfHeight: 24.0,
            radius: 24.0
        })

        output.activateGlassShader(root.width, root.height, cx, cy, pills, chrom, blurRad, vib, refr)
    }

    function triggerScreenCapture() {
        if (typeof screenGrabber !== "undefined" && root.s && root.s.useScreencopyGlass === true) {
            let pad = Math.round(menuContainer.margin)
            let cx = Math.round(menuContainer.centerX)
            let cy = Math.round(menuContainer.centerY)
            let blurRad = (root.s.screencopyBlurRadius !== undefined) ? root.s.screencopyBlurRadius : 40
            let vib = (root.s.screencopyVibrancy !== undefined) ? root.s.screencopyVibrancy : 1.45
            let chrom = (root.s.chromaticAberration !== undefined) ? root.s.chromaticAberration : 14

            if (root.s.screencopyLive === true) {
                let fps = (root.s.screencopyFps !== undefined) ? root.s.screencopyFps : 30
                screenGrabber.startLiveCapture(cx - pad, cy - pad, pad * 2, pad * 2, blurRad, vib, chrom, fps)
            } else {
                screenGrabber.captureRegion(cx - pad, cy - pad, pad * 2, pad * 2, blurRad, vib, chrom)
            }
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
                root.updateGlassOptics()
                root.triggerScreenCapture()
            }
        } else if (output.items && output.items.length > 0) {
            menuStack = []
            hasParent = false
            loadItems(output.items)
            root.updateGlassOptics()
            root.triggerScreenCapture()
        }
    }

    // ── Keyboard Shortcuts ─────────────────────────────────────────────────────
    Shortcut { sequence: "Escape"; onActivated: root.handleEscape() }

    // ── Opening animation (35ms) ──────────────────────────────────────────────
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
        function selectByIndex(numIdx) {
            if (numIdx >= 0 && numIdx < menuContainer.itemCount) {
                let item = menuModel.get(numIdx)
                if (item.submenuName !== "") {
                    root.navigateTo(item.submenuName)
                } else {
                    root.triggerSelect(item.label)
                }
            }
        }

        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Escape) {
                root.handleEscape()
                event.accepted = true
                return
            }

            // 1. Check if any item has an explicit custom hotkey matching pressed key text
            let pressedKeyText = event.text ? event.text.toLowerCase() : ""
            if (pressedKeyText !== "") {
                for (let i = 0; i < menuContainer.itemCount; ++i) {
                    let item = menuModel.get(i)
                    if (item.key && item.key.toLowerCase() === pressedKeyText) {
                        menuContainer.selectByIndex(i)
                        event.accepted = true
                        return
                    }
                }
            }

            // 2. Fallback to number key 1-0 indexing
            let idx = -1
            if (event.key >= Qt.Key_1 && event.key <= Qt.Key_9) {
                idx = event.key - Qt.Key_1
            } else if (event.key === Qt.Key_0) {
                idx = 9
            } else if (event.key >= Qt.Key_Numpad1 && event.key <= Qt.Key_Numpad9) {
                idx = event.key - Qt.Key_Numpad1
            } else if (event.key === Qt.Key_Numpad0) {
                idx = 9
            }

            if (idx >= 0 && idx < menuContainer.itemCount) {
                menuContainer.selectByIndex(idx)
                event.accepted = true
            }
        }
        Component.onCompleted: forceActiveFocus()

        // ── Geometry ────────────────────────────────────────────────────────────
        property real radiusDistance: root.isPieMode ? (root.s.outerRadius || 230) : (root.s.radiusDistance || 185)
        property real margin:  radiusDistance + 110
        property real centerX: Math.max(margin, Math.min(width  - margin, root.menuX))
        property real centerY: Math.max(margin, Math.min(height - margin, root.menuY))
        property int  itemCount:            menuModel.count
        property int  hoveredIndex:         -1
        property real currentMouseAngleDeg: 0

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

                let mouseAngleRad = Math.atan2(dy, dx)
                menuContainer.currentMouseAngleDeg = mouseAngleRad * 180 / Math.PI

                let minRadius = root.isPieMode ? (root.s.innerRadius || 65) : 20

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

        // ── Pie Wheel Background Disc ──────────────────────────────────────────
        Rectangle {
            visible: root.isPieMode
            x: menuContainer.centerX - (root.s.outerRadius || 230)
            y: menuContainer.centerY - (root.s.outerRadius || 230)
            width:  (root.s.outerRadius || 230) * 2
            height: width
            radius: width / 2
            color:  root.s.pieBackgroundColor || "#14141d"
            opacity: root.s.pieBackgroundOpacity !== undefined ? root.s.pieBackgroundOpacity : 0.95
        }

        // ── MODE 1: Segmented Pie Wheel Sectors ────────────────────────────────
        Repeater {
            model: menuModel
            delegate: PieSectorDelegate {}
        }

        // ── Pie Delimiters Overlay ──────────────────────────────────────────────
        PieDelimitersOverlay {}

        // ── MODE 2: Floating Pill / Polygon Delegates ──────────────────────────
        Repeater {
            model: menuModel
            delegate: PillDelegate {}
        }

        // ── Center pivot ───────────────────────────────────────────────────────
        CenterPivot {}

        // ── Current menu breadcrumb label ────────────────────────────────────────
        Text {
            visible: (root.s.showBreadcrumbs !== false) && root.menuStack.length > 0
            x: menuContainer.centerX - width / 2
            y: menuContainer.centerY + (root.isPieMode ? (root.s.innerRadius || 65) : (root.s.centerRadius || 15)) + 10
            text: root.menuStack.join(" › ")
            font.family: root.s.fontFamily || "Sans"
            font.pixelSize: Math.max(9, (root.s.fontSize || 13) - 3)
            color: root.s.breadcrumbColor || "#606070"
            font.letterSpacing: 0.5
        }
    }
}
