import QtQuick

Window {
    id: root
    visible: false
    width: Screen.width
    height: Screen.height
    title: "drmenu"

    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.BypassWindowManagerHint
    color: "transparent"

    property real menuX: width  / 2
    property real menuY: height / 2

    onMenuXChanged: root.updateGlassOptics()
    onMenuYChanged: root.updateGlassOptics()

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
            root.updateGlassOptics()
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
            root.updateGlassOptics()
            root.triggerScreenCapture()
            menuContainer.opacity = 1.0
            openAnimation.start()
        }

        function onStyleChanged() {
            root.updateGlassOptics()
        }
    }

    property real centerHoverProgress: (menuContainer.hoveredIndex === -1) ? 1.0 : 0.0
    Behavior on centerHoverProgress {
        NumberAnimation {
            duration: (root.s && root.s.hoverDuration !== undefined) ? root.s.hoverDuration : 110
            easing.type: Easing.OutCubic
        }
    }
    onCenterHoverProgressChanged: root.updateGlassOptics()

    function updateGlassOptics() {
        if (typeof output === "undefined") return
        let enableGlass = root.s && (root.s.glass === true || root.s.useGlass === true)
        if (!enableGlass) {
            output.deactivateGlassShader()
            return
        }

        let chrom = (root.s.chromaticAberration !== undefined) ? root.s.chromaticAberration
                  : ((root.s.chromatic_aberration !== undefined) ? root.s.chromatic_aberration
                  : ((root.s.chromatic !== undefined) ? root.s.chromatic : 1.4))
        let chromHover = (root.s.chromaticHover !== undefined) ? root.s.chromaticHover
                       : ((root.s.chromatic_hover !== undefined) ? root.s.chromatic_hover
                       : ((root.s.chromaticAberrationHover !== undefined) ? root.s.chromaticAberrationHover
                       : ((root.s.chromatic_aberration_hover !== undefined) ? root.s.chromatic_aberration_hover : chrom)))

        let blurRad = (root.s.blur !== undefined) ? root.s.blur
                    : ((root.s.blurStrength !== undefined) ? root.s.blurStrength
                    : ((root.s.blur_strength !== undefined) ? root.s.blur_strength
                    : ((root.s.blurRadius !== undefined) ? root.s.blurRadius
                    : ((root.s.blur_radius !== undefined) ? root.s.blur_radius
                    : ((root.s.screencopyBlurRadius !== undefined) ? root.s.screencopyBlurRadius : 24.0)))))
        let blurHover = (root.s.blurHover !== undefined) ? root.s.blurHover
                      : ((root.s.blur_hover !== undefined) ? root.s.blur_hover
                      : ((root.s.blurStrengthHover !== undefined) ? root.s.blurStrengthHover
                      : ((root.s.blur_strength_hover !== undefined) ? root.s.blur_strength_hover
                      : ((root.s.blurRadiusHover !== undefined) ? root.s.blurRadiusHover : blurRad))))

        let vib = (root.s.vibrancy !== undefined) ? root.s.vibrancy : 1.15

        let refr = (root.s.refractionStrength !== undefined) ? root.s.refractionStrength
                 : ((root.s.refraction_strength !== undefined) ? root.s.refraction_strength
                 : ((root.s.refraction !== undefined) ? root.s.refraction : 0.85))
        let refrHover = (root.s.refractionHover !== undefined) ? root.s.refractionHover
                      : ((root.s.refraction_hover !== undefined) ? root.s.refraction_hover
                      : ((root.s.refractionStrengthHover !== undefined) ? root.s.refractionStrengthHover
                      : ((root.s.refraction_strength_hover !== undefined) ? root.s.refraction_strength_hover : refr)))

        let spec = (root.s.specular !== undefined) ? root.s.specular
                 : ((root.s.specularStrength !== undefined) ? root.s.specularStrength
                 : ((root.s.specular_strength !== undefined) ? root.s.specular_strength : 0.70))
        let specHover = (root.s.specularHover !== undefined) ? root.s.specularHover
                      : ((root.s.specular_hover !== undefined) ? root.s.specular_hover
                      : ((root.s.specularStrengthHover !== undefined) ? root.s.specularStrengthHover
                      : ((root.s.specular_strength_hover !== undefined) ? root.s.specular_strength_hover : spec)))

        let count = menuModel.count
        if (count <= 0) return

        let pills = []
        let cx = menuContainer.centerX
        let cy = menuContainer.centerY

        if (root.isPieMode) {
            let outR = root.s.outerRadius || 230
            let inR = root.s.innerRadius || 65
            let hovIdx = menuContainer.hoveredIndex

            let sliceAngleDeg = 360 / count
            let gapRad = (root.s.delimiterWidth === 0) ? 0.0 : 0.012

            // 1. Individual Liquid Glass Pie Wedge Sectors
            for (let i = 0; i < count; ++i) {
                let pItem = (typeof pieRepeater !== "undefined" && pieRepeater) ? pieRepeater.itemAt(i) : null
                let isHov = (hovIdx === i)
                let hProg = (pItem && typeof pItem.hoverProgress === "number") ? pItem.hoverProgress : (isHov ? 1.0 : 0.0)

                let startAngle = (i * sliceAngleDeg - 90) * Math.PI / 180 + gapRad
                let endAngle   = ((i + 1) * sliceAngleDeg - 90) * Math.PI / 180 - gapRad

                let sOuterR = outR + (root.s.pieHoverExpansion !== undefined ? root.s.pieHoverExpansion : 12.0) * hProg
                let sBlur = blurRad + (blurHover - blurRad) * hProg
                let sRefr = refr + (refrHover - refr) * hProg
                let sChrom = chrom + (chromHover - chrom) * hProg
                let sSpec = spec + (specHover - spec) * hProg

                let sCol = (hProg >= 0.5)
                    ? (root.s.pieSliceHoverColor || "#35ffffff")
                    : (root.s.pieSliceColor || "#15ffffff")
                let sBorderCol = (hProg >= 0.5)
                    ? (root.s.accentColor || root.s.pieOuterBorderHoverColor || "#0a84ff")
                    : (root.s.delimiterColor || root.s.pieOuterBorderColor || "#30ffffff")
                let sBW = 1.0 + (2.0 - 1.0) * hProg

                let boundSize = (sOuterR + 6.0) * 2.0

                pills.push({
                    x: cx,
                    y: cy,
                    halfWidth: boundSize / 2.0,
                    halfHeight: boundSize / 2.0,
                    radius: 2.0,
                    blur: sBlur,
                    refraction: sRefr,
                    chromatic: sChrom,
                    specular: sSpec,
                    pillColor: sCol,
                    borderColor: sBorderCol,
                    borderWidth: sBW,
                    shapeType: 1,
                    startAngle: startAngle,
                    endAngle: endAngle,
                    innerRadius: inR,
                    outerRadius: sOuterR
                })
            }

            // 2. Center Origin / Torus Hole
            let centerRad = inR
            let cHProg = root.centerHoverProgress
            let cBlur = blurRad + (blurHover - blurRad) * cHProg
            let cRefr = refr + (refrHover - refr) * cHProg
            let cChrom = chrom + (chromHover - chrom) * cHProg
            let cSpec = spec + (specHover - spec) * cHProg
            let cBW = 1.0 + (1.5 - 1.0) * cHProg

            pills.push({
                x: cx,
                y: cy,
                halfWidth: centerRad,
                halfHeight: centerRad,
                radius: centerRad,
                blur: cBlur,
                refraction: cRefr * 0.8,
                chromatic: cChrom,
                specular: cSpec,
                pillColor: (root.s.centerColor !== undefined) ? root.s.centerColor : "transparent",
                borderColor: (cHProg < 0.5 && menuContainer.hoveredIndex !== -1)
                    ? (root.s.centerBorderHoverColor || root.s.centerBorderHover || "transparent")
                    : (root.hasParent ? (root.s.submenuAccent || "#80bf5af2") : (root.s.centerBorder || root.s.centerBorderColor || root.s.centerTorusColor || "#40ffffff")),
                borderWidth: cBW,
                shapeType: 0
            })

            output.activateGlassShader(root.width, root.height, cx, cy, pills, chrom, blurRad, vib, refr, spec)
            return
        }

        let rDist = root.s.radiusDistance || 185
        let pHalfH = (root.s.pillHeight || 42) / 2
        let pRad = (root.s.pillRadius !== undefined) ? root.s.pillRadius
                 : ((root.s.shape === "rectangle") ? 0
                 : ((root.s.shape === "rounded") ? 8 : pHalfH))

        let hasLiveItems = (typeof pillRepeater !== "undefined" && pillRepeater && pillRepeater.count === count)
        if (hasLiveItems) {
            for (let i = 0; i < count; ++i) {
                let pItem = pillRepeater.itemAt(i)
                if (pItem && pItem.width > 0 && pItem.height > 0) {
                    let s = (typeof pItem.scale === "number") ? pItem.scale : 1.0
                    let hProg = (typeof pItem.hoverProgress === "number") ? pItem.hoverProgress : ((menuContainer.hoveredIndex === i) ? 1.0 : 0.0)
                    let pw = pItem.width * s
                    let ph = pItem.height * s
                    let px = pItem.x + pItem.width / 2.0
                    let py = pItem.y + pItem.height / 2.0
                    
                    let itemObj = menuModel.get(i)
                    let isSub = itemObj && itemObj.submenuName && itemObj.submenuName !== ""
                    let useSubAccent = isSub && (root.s.showSubmenuAccent !== false) && (root.s.useSubmenuAccent !== false) && (root.s.submenuAccent !== "transparent") && (root.s.submenuAccent !== "none")

                    let basePCol = (useSubAccent && root.s.pillSubmenuColor) ? root.s.pillSubmenuColor : (root.s.pillColor || root.s.pill_color || "#20ffffff")
                    let hovPCol  = (useSubAccent && root.s.pillSubmenuHoverColor) ? root.s.pillSubmenuHoverColor : (root.s.pillHoverColor || root.s.pill_hover_color || "#40ffffff")
                    let baseBCol = (useSubAccent && root.s.pillSubmenuBorder) ? root.s.pillSubmenuBorder : (root.s.borderColor || root.s.border_color || root.s.pillBorderColor || "#60ffffff")
                    let hovBCol  = (useSubAccent && root.s.pillSubmenuBorderHover) ? root.s.pillSubmenuBorderHover : (root.s.borderHoverColor || root.s.border_hover_color || root.s.pillBorderHoverColor || "#d0ffffff")

                    let pCol = (hProg >= 0.5) ? hovPCol : basePCol
                    let bCol = (hProg >= 0.5) ? hovBCol : baseBCol
                    let baseBW = (root.s.borderWidth || root.s.border_width || 1.0)
                    let hovBW  = (root.s.borderHoverWidth || root.s.border_hover_width || 2.0)
                    let bw = baseBW + (hovBW - baseBW) * hProg

                    pills.push({
                        x: px,
                        y: py,
                        halfWidth: pw / 2.0,
                        halfHeight: ph / 2.0,
                        radius: pRad * s,
                        blur: blurRad + (blurHover - blurRad) * hProg,
                        refraction: refr + (refrHover - refr) * hProg,
                        chromatic: chrom + (chromHover - chrom) * hProg,
                        specular: spec + (specHover - spec) * hProg,
                        pillColor: pCol,
                        borderColor: bCol,
                        borderWidth: bw
                    })
                }
            }
        }

        // Fallback with precise component-based sizing if items not yet laid out
        if (pills.length < count) {
            pills = []
            for (let i = 0; i < count; ++i) {
                let angle = root.getItemAngleRad(i, count)
                let isHov = (menuContainer.hoveredIndex === i)
                let hProg = isHov ? 1.0 : 0.0
                let s = isHov ? 1.09 : 1.0
                let px = cx + Math.cos(angle) * rDist
                let py = cy + Math.sin(angle) * rDist
                let itemObj = menuModel.get(i)
                let isSub = itemObj && itemObj.submenuName && itemObj.submenuName !== ""
                let useSubAccent = isSub && (root.s.showSubmenuAccent !== false) && (root.s.useSubmenuAccent !== false) && (root.s.submenuAccent !== "transparent") && (root.s.submenuAccent !== "none")

                let labelText = (itemObj && itemObj.label) ? itemObj.label : ""
                let hasIcon = (itemObj && ((itemObj.iconName && itemObj.iconName !== "") || (itemObj.icon && itemObj.icon !== "")))
                let iconW = hasIcon ? ((root.s.iconSize || 22) + 7) : 0
                let badgeW = (root.s.showNumberBadges !== false) ? 18 : 0
                let textW = labelText.length * 8.5
                let totalW = Math.max(70.0, iconW + badgeW + textW + 28.0) * s
                let totalH = (root.s.pillHeight || 42) * s

                let basePCol = (useSubAccent && root.s.pillSubmenuColor) ? root.s.pillSubmenuColor : (root.s.pillColor || root.s.pill_color || "#20ffffff")
                let hovPCol  = (useSubAccent && root.s.pillSubmenuHoverColor) ? root.s.pillSubmenuHoverColor : (root.s.pillHoverColor || root.s.pill_hover_color || "#40ffffff")
                let baseBCol = (useSubAccent && root.s.pillSubmenuBorder) ? root.s.pillSubmenuBorder : (root.s.borderColor || root.s.border_color || root.s.pillBorderColor || "#60ffffff")
                let hovBCol  = (useSubAccent && root.s.pillSubmenuBorderHover) ? root.s.pillSubmenuBorderHover : (root.s.borderHoverColor || root.s.border_hover_color || root.s.pillBorderHoverColor || "#d0ffffff")

                let pCol = (hProg >= 0.5) ? hovPCol : basePCol
                let bCol = (hProg >= 0.5) ? hovBCol : baseBCol
                let baseBW = (root.s.borderWidth || root.s.border_width || 1.0)
                let hovBW  = (root.s.borderHoverWidth || root.s.border_hover_width || 2.0)
                let bw = baseBW + (hovBW - baseBW) * hProg

                pills.push({
                    x: px,
                    y: py,
                    halfWidth: totalW / 2.0,
                    halfHeight: totalH / 2.0,
                    radius: pRad * s,
                    blur: blurRad + (blurHover - blurRad) * hProg,
                    refraction: refr + (refrHover - refr) * hProg,
                    chromatic: chrom + (chromHover - chrom) * hProg,
                    specular: spec + (specHover - spec) * hProg,
                    pillColor: pCol,
                    borderColor: bCol,
                    borderWidth: bw
                })
            }
        }

        let centerRad = (root.s.centerRadius !== undefined) ? root.s.centerRadius : (root.s.torusRadius !== undefined ? root.s.torusRadius : 20.0)
        let cHProg = root.centerHoverProgress
        let cBlur = blurRad + (blurHover - blurRad) * cHProg
        let cRefr = refr + (refrHover - refr) * cHProg
        let cChrom = chrom + (chromHover - chrom) * cHProg
        let cSpec = spec + (specHover - spec) * cHProg
        let cBW = 1.0 + (1.5 - 1.0) * cHProg

        pills.push({
            x: cx,
            y: cy,
            halfWidth: centerRad,
            halfHeight: centerRad,
            radius: centerRad,
            blur: cBlur,
            refraction: cRefr,
            chromatic: cChrom,
            specular: cSpec,
            pillColor: (root.s.centerColor !== undefined) ? root.s.centerColor : "transparent",
            borderColor: (cHProg < 0.5 && menuContainer.hoveredIndex !== -1)
                ? (root.s.centerBorderHoverColor || root.s.centerBorderHover || "transparent")
                : (root.hasParent ? (root.s.submenuAccent || "#80bf5af2") : (root.s.centerBorder || root.s.centerBorderColor || "#40ffffff")),
            borderWidth: cBW
        })

        output.activateGlassShader(root.width, root.height, cx, cy, pills, chrom, blurRad, vib, refr, spec)
    }

    function triggerScreenCapture() {
        if (typeof screenGrabber !== "undefined" && root.s && (root.s.useScreencopyGlass === true)) {
            let pad = Math.round(menuContainer.margin)
            let cx = Math.round(menuContainer.centerX)
            let cy = Math.round(menuContainer.centerY)
            let blurRad = (root.s.screencopyBlurRadius !== undefined) ? root.s.screencopyBlurRadius
                        : ((root.s.blurRadius !== undefined) ? root.s.blurRadius
                        : ((root.s.blur_radius !== undefined) ? root.s.blur_radius
                        : ((root.s.glassBlurRadius !== undefined) ? root.s.glassBlurRadius
                        : ((root.s.glass_blur_radius !== undefined) ? root.s.glass_blur_radius : 35))))
            let vib = (root.s.screencopyVibrancy !== undefined) ? root.s.screencopyVibrancy
                    : ((root.s.vibrancy !== undefined) ? root.s.vibrancy : 1.35)
            let chrom = (root.s.chromaticAberration !== undefined) ? root.s.chromaticAberration
                      : ((root.s.chromatic_aberration !== undefined) ? root.s.chromatic_aberration : 8)

            let isLive = (root.s.screencopyLive === true || root.s.screencopy_live === true ||
                          root.s.liveBlur === true || root.s.live_blur === true ||
                          root.s.liveCapture === true || root.s.live_capture === true ||
                          root.s.live === true)
            if (isLive) {
                let fps = (root.s.screencopyFps !== undefined) ? root.s.screencopyFps
                        : ((root.s.fps !== undefined) ? root.s.fps : 30)
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

    Component.onDestruction: {
        if (typeof output !== "undefined") {
            output.deactivateGlassShader()
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

    ListModel {
        id: menuModel
        onCountChanged: {
            if (count > 0) {
                root.updateGlassOptics()
            }
        }
    }

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
        onCenterXChanged: root.updateGlassOptics()
        onCenterYChanged: root.updateGlassOptics()
        property int  itemCount:            menuModel.count
        property int  hoveredIndex:         -1
        onHoveredIndexChanged: root.updateGlassOptics()
        property real currentMouseAngleDeg: 0

        // ── Dim background ───────────────────────────────────────────────────────
        Rectangle {
            anchors.fill: parent
            color: root.s.backgroundColor || "#000000"
            opacity: {
                if (root.s.backgroundOpacity !== undefined) return root.s.backgroundOpacity
                if (root.s.glass === true || root.s.useGlass === true) return 0.20
                return 0.38
            }
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
        Item {
            x: menuContainer.centerX
            y: menuContainer.centerY
            visible: !root.isPieMode && (root.s.showPointerLine !== false) && menuContainer.hoveredIndex !== -1
            rotation: menuContainer.hoveredIndex !== -1 ? root.getItemAngleRad(menuContainer.hoveredIndex, menuContainer.itemCount) * 180 / Math.PI : 0

            Rectangle {
                x: 0
                y: -1
                height: 2
                width: Math.max(0, menuContainer.radiusDistance - 22)
                color: {
                    if (menuContainer.hoveredIndex === -1) return "transparent"
                    let item = menuModel.get(menuContainer.hoveredIndex)
                    if (item && item.submenuName !== "") {
                        let useSub = (root.s.showSubmenuAccent !== false) && (root.s.useSubmenuAccent !== false) && (root.s.submenuAccent !== "transparent") && (root.s.submenuAccent !== "none")
                        if (useSub && root.s.submenuAccent) return root.s.submenuAccent
                    }
                    return root.s.accentColor || "#e67e22"
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
            visible: root.isPieMode && (opacity > 0.0)
            x: menuContainer.centerX - (root.s.outerRadius || 230)
            y: menuContainer.centerY - (root.s.outerRadius || 230)
            width:  (root.s.outerRadius || 230) * 2
            height: width
            radius: width / 2
            color:  root.s.pieBackgroundColor || ((root.s.glass === true || root.s.useGlass === true) ? "transparent" : "#14141d")
            opacity: {
                if (root.s.pieBackgroundOpacity !== undefined) return root.s.pieBackgroundOpacity
                if (root.s.glass === true || root.s.useGlass === true) return 0.0
                return 0.95
            }
        }

        // ── MODE 1: Segmented Pie Wheel Sectors ────────────────────────────────
        Repeater {
            id: pieRepeater
            model: menuModel
            delegate: PieSectorDelegate {}
        }

        // ── Pie Delimiters Overlay ──────────────────────────────────────────────
        PieDelimitersOverlay {}

        // ── MODE 2: Floating Pill / Polygon Delegates ──────────────────────────
        Repeater {
            id: pillRepeater
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
