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

    onXChanged: {
        if (rootRef && rootRef.updateGlassOptics) rootRef.updateGlassOptics()
    }
    onYChanged: {
        if (rootRef && rootRef.updateGlassOptics) rootRef.updateGlassOptics()
    }
    onWidthChanged: {
        if (rootRef && rootRef.updateGlassOptics) rootRef.updateGlassOptics()
    }
    onHeightChanged: {
        if (rootRef && rootRef.updateGlassOptics) rootRef.updateGlassOptics()
    }
    onScaleChanged: {
        if (rootRef && rootRef.updateGlassOptics) rootRef.updateGlassOptics()
    }
    Component.onCompleted: {
        if (rootRef && rootRef.updateGlassOptics) rootRef.updateGlassOptics()
    }

    property int hDuration: (rootRef && rootRef.s && (rootRef.s.hoverDuration !== undefined)) ? rootRef.s.hoverDuration : 110
    property int sDuration: (rootRef && rootRef.s && (rootRef.s.hoverScaleDuration !== undefined)) ? rootRef.s.hoverScaleDuration
                           : ((rootRef && rootRef.s && (rootRef.s.scaleDuration !== undefined)) ? rootRef.s.scaleDuration
                           : Math.max(40, Math.round(pillDelegate.hDuration * 0.82)))

    Behavior on scale {
        NumberAnimation {
            duration: pillDelegate.sDuration
            easing.type: Easing.OutBack
            easing.overshoot: 1.18
        }
    }
    scale: isHovered ? 1.09 : 1.0

    property real hoverProgress: isHovered ? 1.0 : 0.0
    Behavior on hoverProgress {
        NumberAnimation {
            duration: pillDelegate.hDuration
            easing.type: Easing.OutCubic
        }
    }
    onHoverProgressChanged: {
        if (rootRef && rootRef.updateGlassOptics) rootRef.updateGlassOptics()
    }

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

        color: {
            let useSubAccent = (rootRef.s.showSubmenuAccent !== false) && (rootRef.s.useSubmenuAccent !== false) && (rootRef.s.submenuAccent !== "transparent") && (rootRef.s.submenuAccent !== "none")
            if (pillDelegate.isSubmenu && useSubAccent && (rootRef.s.pillSubmenuColor !== undefined || rootRef.s.pillSubmenuHoverColor !== undefined)) {
                return pillDelegate.isHovered
                    ? (rootRef.s.pillSubmenuHoverColor || rootRef.s.pillHoverColor || "transparent")
                    : (rootRef.s.pillSubmenuColor || rootRef.s.pillColor || "transparent")
            }
            if (rootRef.s.pillColor !== undefined || rootRef.s.pill_color !== undefined) {
                return pillDelegate.isHovered
                    ? (rootRef.s.pillHoverColor || rootRef.s.pill_hover_color || "transparent")
                    : (rootRef.s.pillColor || rootRef.s.pill_color || "transparent")
            }
            if (rootRef.s.useGlass === true || rootRef.s.useScreencopyGlass === true || rootRef.s.glass === true) {
                return pillDelegate.isHovered ? "#30ffffff" : "#12ffffff"
            }
            return pillDelegate.isHovered
                ? (pillDelegate.isSubmenu && useSubAccent ? (rootRef.s.pillSubmenuHoverColor || "#2a2438") : (rootRef.s.pillHoverColor || "#2b2b36"))
                : (pillDelegate.isSubmenu && useSubAccent ? (rootRef.s.pillSubmenuColor || "#18151f") : (rootRef.s.pillColor || "#1a1a20"))
        }

        border.color: {
            let useSubAccent = (rootRef.s.showSubmenuAccent !== false) && (rootRef.s.useSubmenuAccent !== false) && (rootRef.s.submenuAccent !== "transparent") && (rootRef.s.submenuAccent !== "none")
            if (pillDelegate.isSubmenu && useSubAccent && (rootRef.s.pillSubmenuBorder !== undefined || rootRef.s.pillSubmenuBorderHover !== undefined)) {
                return pillDelegate.isHovered
                    ? (rootRef.s.pillSubmenuBorderHover || rootRef.s.borderHoverColor || rootRef.s.pillBorderHoverColor || "transparent")
                    : (rootRef.s.pillSubmenuBorder || rootRef.s.borderColor || rootRef.s.pillBorderColor || "transparent")
            }
            if (rootRef.s.borderColor !== undefined || rootRef.s.border_color !== undefined || rootRef.s.pillBorderColor !== undefined) {
                return pillDelegate.isHovered
                    ? (rootRef.s.borderHoverColor || rootRef.s.border_hover_color || rootRef.s.pillBorderHoverColor || "transparent")
                    : (rootRef.s.borderColor || rootRef.s.border_color || rootRef.s.pillBorderColor || "transparent")
            }
            if (rootRef.s.useGlass === true || rootRef.s.useScreencopyGlass === true || rootRef.s.glass === true) {
                return pillDelegate.isHovered ? "#80ffffff" : "#38ffffff"
            }
            return pillDelegate.isHovered
                ? (pillDelegate.isSubmenu && useSubAccent ? (rootRef.s.pillSubmenuBorderHover || "#a855f7") : (rootRef.s.pillBorderHoverColor || "#e67e22"))
                : (pillDelegate.isSubmenu && useSubAccent ? (rootRef.s.pillSubmenuBorder || "#4a3060") : (rootRef.s.pillBorderColor || "#383842"))
        }

        border.width: pillDelegate.isHovered
            ? (rootRef.s.borderHoverWidth || 2)
            : (rootRef.s.borderWidth || 1)

        layer.enabled: true
        layer.smooth: true

        Behavior on color        { ColorAnimation { duration: 60; easing.type: Easing.OutQuad } }
        Behavior on border.color { ColorAnimation { duration: 60; easing.type: Easing.OutQuad } }

        // ── Screencopy Optical Glass Blur Layer ────────────────────────
        Rectangle {
            anchors.fill: parent
            anchors.margins: pillBody.border.width
            radius: Math.max(0, pillBody.radius - pillBody.border.width)
            color: "transparent"
            clip: true
            z: -1
            visible: (rootRef.s.useScreencopyGlass === true) && (typeof screenGrabber !== "undefined") && screenGrabber && (screenGrabber.revision > 0)

            layer.enabled: true
            layer.smooth: true

            Image {
                visible: parent.visible
                x: -(pillDelegate.x - (menuContainer.centerX - menuContainer.margin)) - pillBody.border.width
                y: -(pillDelegate.y - (menuContainer.centerY - menuContainer.margin)) - pillBody.border.width
                width: menuContainer.margin * 2
                height: menuContainer.margin * 2
                source: (typeof screenGrabber !== "undefined" && screenGrabber && screenGrabber.revision > 0)
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
            opacity: {
                let spec = rootRef.s.specularStrength !== undefined ? rootRef.s.specularStrength : 0.60
                return pillDelegate.isHovered ? Math.min(1.0, spec * 1.5) : spec
            }
            z: 0

            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.00; color: pillDelegate.isHovered ? "#38ffffff" : "#22ffffff" }
                GradientStop { position: 0.40; color: "#00ffffff" }
                GradientStop { position: 1.00; color: pillDelegate.isHovered ? "#18ffffff" : "#08ffffff" }
            }

            Behavior on opacity { NumberAnimation { duration: 60 } }
        }

        // ── Liquid Glass Multi-Spectral Rainbow Chromatic Prism Dispersion Rim (Qt fallback) ──
        Rectangle {
            id: chromaticRim
            anchors.fill: parent
            anchors.margins: pillBody.border.width
            radius: Math.max(0, pillBody.radius - pillBody.border.width)
            color: "transparent"
            border.width: rootRef.s.chromaticBorderWidth !== undefined ? rootRef.s.chromaticBorderWidth : 2
            visible: rootRef.s.useFakeChromatic === true
            opacity: {
                let baseOp = rootRef.s.chromaticOpacity !== undefined ? rootRef.s.chromaticOpacity : (typeof rootRef.s.chromaticAberration === "number" && rootRef.s.chromaticAberration <= 1.0 ? rootRef.s.chromaticAberration : 0.85)
                return pillDelegate.isHovered ? Math.min(1.0, baseOp + 0.15) : baseOp
            }
            z: 0

            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.00; color: "#b000e5ff" } // Vivid Electric Cyan / Azure
                GradientStop { position: 0.18; color: "#9000ff88" } // Vivid Neon Emerald
                GradientStop { position: 0.38; color: "#00ffffff" } // Crystal Center
                GradientStop { position: 0.62; color: "#00ffffff" } // Crystal Center
                GradientStop { position: 0.82; color: "#95ffaa00" } // Vivid Amber Gold
                GradientStop { position: 1.00; color: "#b5ff1744" } // Vivid Liquid Crimson / Rose
            }

            Behavior on opacity { NumberAnimation { duration: 60 } }
        }

        // ── Liquid Glass Optical Dispersion Sheen (Across entire pill, Qt fallback) ──
        Rectangle {
            id: chromaticSheen
            anchors.fill: parent
            anchors.margins: pillBody.border.width
            radius: Math.max(0, pillBody.radius - pillBody.border.width)
            color: "transparent"
            visible: rootRef.s.useFakeChromatic === true
            opacity: {
                let baseOp = rootRef.s.chromaticOpacity !== undefined ? (rootRef.s.chromaticOpacity * 0.55) : 0.45
                return pillDelegate.isHovered ? Math.min(1.0, baseOp + 0.20) : baseOp
            }
            z: 0

            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.00; color: "#5000e5ff" } // Cyan / Blue refraction
                GradientStop { position: 0.25; color: "#3500ffaa" } // Aqua / Green
                GradientStop { position: 0.50; color: "#00ffffff" } // Transparent center
                GradientStop { position: 0.75; color: "#35ffaa00" } // Amber / Orange
                GradientStop { position: 1.00; color: "#50ff1744" } // Magenta / Red refraction
            }

            Behavior on opacity { NumberAnimation { duration: 60 } }
        }

        // ── Dynamic Liquid Caustic Bloom on Hover ───────────────────────
        Rectangle {
            anchors.fill: parent
            anchors.margins: pillBody.border.width
            radius: Math.max(0, pillBody.radius - pillBody.border.width)
            visible: {
                if (!pillDelegate.isHovered) return false
                let useSubAccent = (rootRef.s.showSubmenuAccent !== false) && (rootRef.s.useSubmenuAccent !== false) && (rootRef.s.submenuAccent !== "transparent") && (rootRef.s.submenuAccent !== "none")
                let col = (pillDelegate.isSubmenu && useSubAccent) ? (rootRef.s.submenuAccent || rootRef.s.accentColor || "#0a84ff") : (rootRef.s.accentColor || "#0a84ff")
                return col !== "transparent" && col !== "none"
            }
            opacity: pillDelegate.isHovered ? (rootRef.s.causticOpacity !== undefined ? rootRef.s.causticOpacity : 0.30) : 0.0
            color: {
                let useSubAccent = (rootRef.s.showSubmenuAccent !== false) && (rootRef.s.useSubmenuAccent !== false) && (rootRef.s.submenuAccent !== "transparent") && (rootRef.s.submenuAccent !== "none")
                if (pillDelegate.isSubmenu && useSubAccent) {
                    return rootRef.s.submenuAccent || rootRef.s.accentColor || "#0a84ff"
                }
                return rootRef.s.accentColor || "#0a84ff"
            }
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
                color: {
                    let useSubAccent = (rootRef.s.showSubmenuAccent !== false) && (rootRef.s.useSubmenuAccent !== false) && (rootRef.s.submenuAccent !== "transparent") && (rootRef.s.submenuAccent !== "none")
                    if (pillDelegate.isHovered) {
                        if (pillDelegate.isSubmenu && useSubAccent && rootRef.s.submenuAccent) {
                            return rootRef.s.submenuAccent
                        }
                        return rootRef.s.iconHoverColor || rootRef.s.icon_hover_color || rootRef.s.accentColor || "#ffffff"
                    }
                    if (pillDelegate.isSubmenu && useSubAccent && rootRef.s.submenuIconColor) return rootRef.s.submenuIconColor
                    return rootRef.s.iconColor || rootRef.s.icon_color || "#9090a0"
                }
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
                visible: pillDelegate.isSubmenu && pillDelegate.shapeType !== "circle" && (rootRef.s.showSubmenuIndicator !== false)
                text:    "▶"
                font.pixelSize: Math.max(8, (rootRef.s.fontSize || 13) - 4)
                color: {
                    let useSubAccent = (rootRef.s.showSubmenuAccent !== false) && (rootRef.s.useSubmenuAccent !== false) && (rootRef.s.submenuAccent !== "transparent") && (rootRef.s.submenuAccent !== "none")
                    if (pillDelegate.isHovered) {
                        if (useSubAccent && rootRef.s.submenuAccent) return rootRef.s.submenuAccent
                        return rootRef.s.textHoverColor || "#ffffff"
                    }
                    if (useSubAccent && rootRef.s.pillSubmenuBorder && rootRef.s.pillSubmenuBorder !== "transparent") return rootRef.s.pillSubmenuBorder
                    return rootRef.s.textColor || "#808090"
                }
                anchors.verticalCenter: parent.verticalCenter
                leftPadding: 1
                Behavior on color { ColorAnimation { duration: 40 } }
            }
        }
    }
}
