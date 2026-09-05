import QtQuick
import QtQuick.Window

Window {
    id: editorWindow
    visible: true
    width: 980
    height: 720
    minimumWidth: 860
    minimumHeight: 600
    title: "drmenu Configuration Editor"
    color: "#0f0f14"

    property string activeFilePath: ""
    property var availableFiles: []
    property var configDoc: ({})
    property var menusMap: ({})
    property var menuKeys: []
    property string selectedMenuKey: "main"
    property int selectedItemIndex: 0
    property string saveStatusMessage: ""

    Component.onCompleted: {
        refreshFiles()
    }

    function refreshFiles() {
        if (!configBackend) return
        availableFiles = configBackend.availableConfigFiles()
        let initial = (typeof initialConfigPath !== "undefined" && initialConfigPath !== "")
            ? initialConfigPath
            : (availableFiles.length > 0 ? availableFiles[0] : configBackend.defaultConfigPath())
        loadFromFile(initial)
    }

    function loadFromFile(path) {
        if (!configBackend) return
        activeFilePath = path
        let data = configBackend.loadConfig(path)
        configDoc = data
        menusMap = data.menus || {}
        let keys = Object.keys(menusMap)
        menuKeys = keys
        if (keys.length > 0) {
            if (keys.indexOf("main") !== -1) selectedMenuKey = "main"
            else selectedMenuKey = keys[0]
        } else {
            selectedMenuKey = ""
        }
        selectedItemIndex = 0
    }

    function currentMenuData() {
        if (!menusMap || !selectedMenuKey || !menusMap[selectedMenuKey]) return {}
        return menusMap[selectedMenuKey]
    }

    function currentItems() {
        let m = currentMenuData()
        if (m.items !== undefined) return m.items
        if (Array.isArray(m)) return m
        return []
    }

    function saveActiveConfig() {
        if (!configBackend || !activeFilePath) return
        configDoc.menus = menusMap
        let ok = configBackend.saveConfig(activeFilePath, configDoc)
        if (ok) {
            saveStatusMessage = "Saved successfully!"
            statusTimer.restart()
        } else {
            saveStatusMessage = "Failed to save!"
            statusTimer.restart()
        }
    }

    Timer {
        id: statusTimer
        interval: 2500
        onTriggered: editorWindow.saveStatusMessage = ""
    }

    Shortcut {
        sequence: "Ctrl+S"
        onActivated: saveActiveConfig()
    }

    Shortcut {
        sequence: "Ctrl+T"
        onActivated: {
            if (configBackend) configBackend.testMenu(selectedMenuKey, activeFilePath)
        }
    }

    // ── Main UI Layout ───────────────────────────────────────────────────────
    Column {
        anchors.fill: parent

        // ── Top Action Header Bar ───────────────────────────────────────────
        Rectangle {
            width: parent.width
            height: 56
            color: "#16161f"
            border.color: "#282836"
            border.width: 1

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 12

                Rectangle {
                    width: 32
                    height: 32
                    radius: 8
                    color: "#3b82f6"
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        anchors.centerIn: parent
                        text: "dr"
                        color: "#ffffff"
                        font.bold: true
                        font.pixelSize: 14
                    }
                }

                Text {
                    text: "drmenu Config"
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 16
                    anchors.verticalCenter: parent.verticalCenter
                }

                Rectangle {
                    width: 1
                    height: 22
                    color: "#303040"
                    anchors.verticalCenter: parent.verticalCenter
                }

                // File Selector Dropdown Button
                Rectangle {
                    width: 220
                    height: 32
                    radius: 6
                    color: fileSelectHover.containsMouse ? "#262638" : "#1c1c28"
                    border.color: fileDropdownMenu.visible ? "#3b82f6" : "#323246"
                    border.width: 1
                    anchors.verticalCenter: parent.verticalCenter

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 6

                        Text {
                            text: "📄"
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: {
                                if (!editorWindow.activeFilePath) return "No file"
                                let p = editorWindow.activeFilePath
                                return p.substring(p.lastIndexOf("/") + 1)
                            }
                            color: "#e0e0ee"
                            font.pixelSize: 12
                            font.bold: true
                            elide: Text.ElideMiddle
                            width: parent.width - 45
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: "▼"
                            color: "#888899"
                            font.pixelSize: 8
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        id: fileSelectHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: fileDropdownMenu.visible = !fileDropdownMenu.visible
                    }

                    // Dropdown choices popup
                    Rectangle {
                        id: fileDropdownMenu
                        visible: false
                        y: parent.height + 4
                        width: 260
                        height: Math.min(220, filesCol.height + 10)
                        color: "#1a1a26"
                        border.color: "#3a3a4e"
                        border.width: 1
                        radius: 6
                        z: 100

                        Column {
                            id: filesCol
                            anchors.fill: parent
                            anchors.margins: 4
                            spacing: 2

                            Repeater {
                                model: editorWindow.availableFiles

                                Rectangle {
                                    width: parent.width
                                    height: 30
                                    radius: 4
                                    color: fileRowHover.containsMouse ? "#2c2c40" : (editorWindow.activeFilePath === modelData ? "#252538" : "transparent")

                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 8
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData.substring(modelData.lastIndexOf("/") + 1)
                                        color: editorWindow.activeFilePath === modelData ? "#60a5fa" : "#cccccc"
                                        font.pixelSize: 12
                                    }

                                    MouseArea {
                                        id: fileRowHover
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            fileDropdownMenu.visible = false
                                            editorWindow.loadFromFile(modelData)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Right side: Status, Test, Save buttons
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 12

                Text {
                    text: editorWindow.saveStatusMessage
                    color: editorWindow.saveStatusMessage.includes("success") ? "#4ade80" : "#f87171"
                    font.bold: true
                    font.pixelSize: 13
                    anchors.verticalCenter: parent.verticalCenter
                    visible: editorWindow.saveStatusMessage !== ""
                }

                // Test Menu Button
                Rectangle {
                    width: 110
                    height: 34
                    radius: 6
                    color: testHover.containsMouse ? "#2f374a" : "#202534"
                    border.color: testHover.containsMouse ? "#60a5fa" : "#3b4560"
                    border.width: 1
                    anchors.verticalCenter: parent.verticalCenter

                    Row {
                        anchors.centerIn: parent
                        spacing: 6
                        Text { text: "▶"; color: "#60a5fa"; font.pixelSize: 11 }
                        Text { text: "Test Menu"; color: "#dbeafe"; font.pixelSize: 12; font.bold: true }
                    }

                    MouseArea {
                        id: testHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (configBackend) configBackend.testMenu(editorWindow.selectedMenuKey, editorWindow.activeFilePath)
                        }
                    }
                }

                // Save Button
                Rectangle {
                    width: 120
                    height: 34
                    radius: 6
                    color: saveHover.containsMouse ? "#2563eb" : "#3b82f6"
                    anchors.verticalCenter: parent.verticalCenter

                    Row {
                        anchors.centerIn: parent
                        spacing: 6
                        Text { text: "💾"; font.pixelSize: 12 }
                        Text { text: "Save Config"; color: "#ffffff"; font.pixelSize: 12; font.bold: true }
                    }

                    MouseArea {
                        id: saveHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: editorWindow.saveActiveConfig()
                    }
                }
            }
        }

        // ── Three-Column Main Body ──────────────────────────────────────────
        Item {
            width: parent.width
            height: parent.height - 56

            Row {
                anchors.fill: parent

                // ── Col 1: Menus Sidebar (Width 220) ────────────────────────
                Rectangle {
                    width: 220
                    height: parent.height
                    color: "#13131b"
                    border.color: "#222230"
                    border.width: 1

                    Column {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10

                        Row {
                            width: parent.width
                            Text {
                                text: "MENUS"
                                color: "#808096"
                                font.pixelSize: 11
                                font.bold: true
                                font.letterSpacing: 1.0
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Rectangle {
                                width: 22
                                height: 22
                                radius: 4
                                color: addMenuHover.containsMouse ? "#3b82f6" : "#222230"
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    anchors.centerIn: parent
                                    text: "+"
                                    color: "#ffffff"
                                    font.bold: true
                                    font.pixelSize: 14
                                }

                                MouseArea {
                                    id: addMenuHover
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        let newKey = "menu_" + (editorWindow.menuKeys.length + 1)
                                        let copy = Object.assign({}, editorWindow.menusMap)
                                        copy[newKey] = { items: [] }
                                        editorWindow.menusMap = copy
                                        editorWindow.menuKeys = Object.keys(copy)
                                        editorWindow.selectedMenuKey = newKey
                                        editorWindow.selectedItemIndex = 0
                                    }
                                }
                            }
                        }

                        ListView {
                            id: menuListView
                            width: parent.width
                            height: parent.height - 40
                            clip: true
                            spacing: 4
                            model: editorWindow.menuKeys

                            delegate: Rectangle {
                                width: menuListView.width
                                height: 38
                                radius: 6
                                color: editorWindow.selectedMenuKey === modelData ? "#27273a" : (menuRowHover.containsMouse ? "#1c1c28" : "transparent")
                                border.color: editorWindow.selectedMenuKey === modelData ? "#3b82f6" : "transparent"
                                border.width: 1

                                Row {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 8
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 8

                                    Text {
                                        text: modelData === "main" ? "🏠" : "📂"
                                        font.pixelSize: 13
                                        anchors.verticalCenter: parent.verticalCenter
                                    }

                                    Text {
                                        text: modelData
                                        color: editorWindow.selectedMenuKey === modelData ? "#ffffff" : "#cccccc"
                                        font.pixelSize: 13
                                        font.bold: editorWindow.selectedMenuKey === modelData
                                        elide: Text.ElideRight
                                        width: parent.width - 55
                                        anchors.verticalCenter: parent.verticalCenter
                                    }

                                    // Item count badge
                                    Rectangle {
                                        width: 20
                                        height: 18
                                        radius: 9
                                        color: "#20202e"
                                        anchors.verticalCenter: parent.verticalCenter

                                        Text {
                                            anchors.centerIn: parent
                                            text: {
                                                let m = editorWindow.menusMap[modelData]
                                                if (!m) return "0"
                                                let it = m.items || (Array.isArray(m) ? m : [])
                                                return it.length
                                            }
                                            color: "#888899"
                                            font.pixelSize: 10
                                        }
                                    }
                                }

                                MouseArea {
                                    id: menuRowHover
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        editorWindow.selectedMenuKey = modelData
                                        editorWindow.selectedItemIndex = 0
                                    }
                                }
                            }
                        }
                    }
                }

                // ── Col 2: Items List (Width 360) ───────────────────────────
                Rectangle {
                    width: 360
                    height: parent.height
                    color: "#111117"
                    border.color: "#222230"
                    border.width: 1

                    Column {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10

                        Row {
                            width: parent.width

                            Text {
                                text: "ITEMS IN '" + editorWindow.selectedMenuKey + "'"
                                color: "#808096"
                                font.pixelSize: 11
                                font.bold: true
                                font.letterSpacing: 0.8
                                anchors.verticalCenter: parent.verticalCenter
                                elide: Text.ElideRight
                                width: parent.width - 85
                            }

                            // Add Item Button
                            Rectangle {
                                width: 80
                                height: 26
                                radius: 4
                                color: addItemHover.containsMouse ? "#2563eb" : "#3b82f6"
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter

                                Row {
                                    anchors.centerIn: parent
                                    spacing: 4
                                    Text { text: "+"; color: "#fff"; font.bold: true; font.pixelSize: 13 }
                                    Text { text: "Add Item"; color: "#fff"; font.bold: true; font.pixelSize: 11 }
                                }

                                MouseArea {
                                    id: addItemHover
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        let m = editorWindow.currentMenuData()
                                        let items = editorWindow.currentItems().slice()
                                        items.push({
                                            label: "New App",
                                            command: "",
                                            iconName: "application-x-executable"
                                        })
                                        m.items = items
                                        let copy = Object.assign({}, editorWindow.menusMap)
                                        copy[editorWindow.selectedMenuKey] = m
                                        editorWindow.menusMap = copy
                                        editorWindow.selectedItemIndex = items.length - 1
                                    }
                                }
                            }
                        }

                        ListView {
                            id: itemsListView
                            width: parent.width
                            height: parent.height - 42
                            clip: true
                            spacing: 6
                            model: editorWindow.currentItems()

                            delegate: Rectangle {
                                width: itemsListView.width
                                height: 50
                                radius: 6
                                color: editorWindow.selectedItemIndex === index ? "#26263b" : (itemCardHover.containsMouse ? "#191924" : "#161620")
                                border.color: editorWindow.selectedItemIndex === index ? "#3b82f6" : "#242432"
                                border.width: 1

                                Row {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 8
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 10

                                    // Icon preview
                                    Rectangle {
                                        width: 32
                                        height: 32
                                        radius: 6
                                        color: "#20202e"
                                        anchors.verticalCenter: parent.verticalCenter

                                        Text {
                                            visible: modelData.icon !== undefined && modelData.icon !== ""
                                            text: modelData.icon || ""
                                            font.pixelSize: 18
                                            anchors.centerIn: parent
                                        }

                                        Image {
                                            visible: !modelData.icon && (modelData.iconName !== undefined && modelData.iconName !== "")
                                            source: modelData.iconName ? "image://icon/" + modelData.iconName : ""
                                            sourceSize.width: 24
                                            sourceSize.height: 24
                                            width: 24
                                            height: 24
                                            anchors.centerIn: parent
                                        }

                                        Text {
                                            visible: !modelData.icon && !modelData.iconName
                                            text: "⚙"
                                            font.pixelSize: 16
                                            color: "#888899"
                                            anchors.centerIn: parent
                                        }
                                    }

                                    // Label & Command / Submenu
                                    Column {
                                        width: parent.width - 120
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: 2

                                        Text {
                                            text: modelData.label || modelData.app || "(untitled)"
                                            color: "#ffffff"
                                            font.pixelSize: 13
                                            font.bold: true
                                            elide: Text.ElideRight
                                            width: parent.width
                                        }

                                        Text {
                                            text: modelData.submenu ? ("Submenu ➔ " + modelData.submenu) : (modelData.command ? modelData.command : (modelData.app ? ("App: " + modelData.app) : ""))
                                            color: modelData.submenu ? "#c084fc" : "#777788"
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                            width: parent.width
                                        }
                                    }

                                    // Key badge
                                    Rectangle {
                                        width: 20
                                        height: 20
                                        radius: 4
                                        color: "#28283a"
                                        border.color: "#38384e"
                                        border.width: 1
                                        visible: modelData.key !== undefined && modelData.key !== ""
                                        anchors.verticalCenter: parent.verticalCenter

                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.key || ""
                                            color: "#60a5fa"
                                            font.pixelSize: 11
                                            font.bold: true
                                        }
                                    }

                                    // Move Up / Down & Delete
                                    Row {
                                        spacing: 2
                                        anchors.verticalCenter: parent.verticalCenter

                                        Text {
                                            text: "▲"
                                            color: index > 0 ? "#888899" : "#333344"
                                            font.pixelSize: 10
                                            MouseArea {
                                                anchors.fill: parent
                                                enabled: index > 0
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    let m = editorWindow.currentMenuData()
                                                    let items = editorWindow.currentItems().slice()
                                                    let tmp = items[index]
                                                    items[index] = items[index - 1]
                                                    items[index - 1] = tmp
                                                    m.items = items
                                                    let copy = Object.assign({}, editorWindow.menusMap)
                                                    copy[editorWindow.selectedMenuKey] = m
                                                    editorWindow.menusMap = copy
                                                    editorWindow.selectedItemIndex = index - 1
                                                }
                                            }
                                        }

                                        Text {
                                            text: "▼"
                                            color: index < itemsListView.count - 1 ? "#888899" : "#333344"
                                            font.pixelSize: 10
                                            MouseArea {
                                                anchors.fill: parent
                                                enabled: index < itemsListView.count - 1
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    let m = editorWindow.currentMenuData()
                                                    let items = editorWindow.currentItems().slice()
                                                    let tmp = items[index]
                                                    items[index] = items[index + 1]
                                                    items[index + 1] = tmp
                                                    m.items = items
                                                    let copy = Object.assign({}, editorWindow.menusMap)
                                                    copy[editorWindow.selectedMenuKey] = m
                                                    editorWindow.menusMap = copy
                                                    editorWindow.selectedItemIndex = index + 1
                                                }
                                            }
                                        }

                                        Text {
                                            text: "✕"
                                            color: "#ef4444"
                                            font.pixelSize: 12
                                            font.bold: true
                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    let m = editorWindow.currentMenuData()
                                                    let items = editorWindow.currentItems().slice()
                                                    items.splice(index, 1)
                                                    m.items = items
                                                    let copy = Object.assign({}, editorWindow.menusMap)
                                                    copy[editorWindow.selectedMenuKey] = m
                                                    editorWindow.menusMap = copy
                                                    if (editorWindow.selectedItemIndex >= items.length) {
                                                        editorWindow.selectedItemIndex = Math.max(0, items.length - 1)
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                MouseArea {
                                    id: itemCardHover
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: editorWindow.selectedItemIndex = index
                                }
                            }
                        }
                    }
                }

                // ── Col 3: Inspector Panel (Right Flex) ──────────────────────
                Rectangle {
                    width: parent.width - 580
                    height: parent.height
                    color: "#14141d"

                    property int inspectorTab: 0 // 0: Item Inspector, 1: Menu Settings

                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 14

                        // Inspector Tabs
                        Row {
                            width: parent.width
                            spacing: 8

                            Rectangle {
                                width: 140
                                height: 32
                                radius: 6
                                color: parent.parent.inspectorTab === 0 ? "#3b82f6" : "#1f1f2d"
                                Text {
                                    anchors.centerIn: parent
                                    text: "Item Properties"
                                    color: "#ffffff"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: parent.parent.parent.inspectorTab = 0
                                }
                            }

                            Rectangle {
                                width: 140
                                height: 32
                                radius: 6
                                color: parent.parent.inspectorTab === 1 ? "#3b82f6" : "#1f1f2d"
                                Text {
                                    anchors.centerIn: parent
                                    text: "Menu Settings"
                                    color: "#ffffff"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: parent.parent.parent.inspectorTab = 1
                                }
                            }
                        }

                        // ── Tab 0: Item Properties ──────────────────────────
                        Flickable {
                            visible: parent.inspectorTab === 0
                            width: parent.width
                            height: parent.height - 50
                            contentHeight: itemFormCol.height + 30
                            clip: true

                            Column {
                                id: itemFormCol
                                width: parent.width
                                spacing: 14

                                property var activeItem: {
                                    let items = editorWindow.currentItems()
                                    if (editorWindow.selectedItemIndex >= 0 && editorWindow.selectedItemIndex < items.length) {
                                        return items[editorWindow.selectedItemIndex]
                                    }
                                    return null
                                }

                                function setItemProperty(prop, value) {
                                    let items = editorWindow.currentItems().slice()
                                    if (editorWindow.selectedItemIndex < 0 || editorWindow.selectedItemIndex >= items.length) return
                                    let it = Object.assign({}, items[editorWindow.selectedItemIndex])
                                    if (value === "") delete it[prop]
                                    else it[prop] = value
                                    items[editorWindow.selectedItemIndex] = it
                                    let m = editorWindow.currentMenuData()
                                    m.items = items
                                    let copy = Object.assign({}, editorWindow.menusMap)
                                    copy[editorWindow.selectedMenuKey] = m
                                    editorWindow.menusMap = copy
                                }

                                Text {
                                    visible: !itemFormCol.activeItem
                                    text: "Select an item to edit its properties."
                                    color: "#888899"
                                    font.pixelSize: 13
                                }

                                Column {
                                    visible: itemFormCol.activeItem !== null
                                    width: parent.width
                                    spacing: 12

                                    // Label Field
                                    Text { text: "Label / Title:"; color: "#aaaab5"; font.pixelSize: 12; font.bold: true }
                                    Rectangle {
                                        width: parent.width
                                        height: 36
                                        radius: 6
                                        color: "#1b1b26"
                                        border.color: labelInput.activeFocus ? "#3b82f6" : "#2e2e3e"
                                        border.width: 1

                                        TextInput {
                                            id: labelInput
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            text: itemFormCol.activeItem ? (itemFormCol.activeItem.label || "") : ""
                                            color: "#ffffff"
                                            font.pixelSize: 13
                                            selectByMouse: true
                                            onTextEdited: itemFormCol.setItemProperty("label", text)
                                        }
                                    }

                                    // Icon Visual Picker Card
                                    Text { text: "Icon:"; color: "#aaaab5"; font.pixelSize: 12; font.bold: true }
                                    Rectangle {
                                        width: parent.width
                                        height: 64
                                        radius: 8
                                        color: "#1b1b26"
                                        border.color: "#2e2e3e"
                                        border.width: 1

                                        Row {
                                            anchors.fill: parent
                                            anchors.margins: 12
                                            spacing: 14
                                            anchors.verticalCenter: parent.verticalCenter

                                            Rectangle {
                                                width: 40
                                                height: 40
                                                radius: 8
                                                color: "#242435"
                                                anchors.verticalCenter: parent.verticalCenter

                                                Text {
                                                    visible: itemFormCol.activeItem && itemFormCol.activeItem.icon
                                                    text: itemFormCol.activeItem ? (itemFormCol.activeItem.icon || "") : ""
                                                    font.pixelSize: 22
                                                    anchors.centerIn: parent
                                                }

                                                Image {
                                                    visible: itemFormCol.activeItem && !itemFormCol.activeItem.icon && itemFormCol.activeItem.iconName
                                                    source: (itemFormCol.activeItem && itemFormCol.activeItem.iconName) ? ("image://icon/" + itemFormCol.activeItem.iconName) : ""
                                                    sourceSize.width: 32
                                                    sourceSize.height: 32
                                                    width: 32
                                                    height: 32
                                                    anchors.centerIn: parent
                                                }
                                            }

                                            Column {
                                                anchors.verticalCenter: parent.verticalCenter
                                                spacing: 2
                                                width: parent.width - 180

                                                Text {
                                                    text: {
                                                        if (!itemFormCol.activeItem) return "None"
                                                        if (itemFormCol.activeItem.icon) return "Emoji: " + itemFormCol.activeItem.icon
                                                        if (itemFormCol.activeItem.iconName) return "Theme Icon: " + itemFormCol.activeItem.iconName
                                                        return "No icon set"
                                                    }
                                                    color: "#ffffff"
                                                    font.pixelSize: 12
                                                    font.bold: true
                                                    elide: Text.ElideRight
                                                    width: parent.width
                                                }

                                                Text {
                                                    text: "Click Browse to search 20k+ icons & apps"
                                                    color: "#777788"
                                                    font.pixelSize: 10
                                                }
                                            }

                                            Rectangle {
                                                width: 100
                                                height: 32
                                                radius: 6
                                                color: browseIconHover.containsMouse ? "#3b82f6" : "#28283c"
                                                border.color: "#3b82f6"
                                                border.width: 1
                                                anchors.verticalCenter: parent.verticalCenter

                                                Text {
                                                    anchors.centerIn: parent
                                                    text: "Browse..."
                                                    color: "#ffffff"
                                                    font.pixelSize: 12
                                                    font.bold: true
                                                }

                                                MouseArea {
                                                    id: browseIconHover
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    cursorShape: Qt.PointingHandCursor
                                                    onClicked: iconPickerModal.open()
                                                }
                                            }
                                        }
                                    }

                                    // Manual Icon Name Field
                                    Row {
                                        width: parent.width
                                        spacing: 8

                                        Column {
                                            width: parent.width / 2 - 4
                                            spacing: 4
                                            Text { text: "Icon Name (Theme):"; color: "#888899"; font.pixelSize: 11 }
                                            Rectangle {
                                                width: parent.width
                                                height: 32
                                                radius: 4
                                                color: "#181824"
                                                border.color: "#282838"
                                                TextInput {
                                                    anchors.fill: parent
                                                    anchors.margins: 6
                                                    text: itemFormCol.activeItem ? (itemFormCol.activeItem.iconName || "") : ""
                                                    color: "#ddd"
                                                    font.pixelSize: 12
                                                    selectByMouse: true
                                                    onTextEdited: itemFormCol.setItemProperty("iconName", text)
                                                }
                                            }
                                        }

                                        Column {
                                            width: parent.width / 2 - 4
                                            spacing: 4
                                            Text { text: "Emoji / Symbol:"; color: "#888899"; font.pixelSize: 11 }
                                            Rectangle {
                                                width: parent.width
                                                height: 32
                                                radius: 4
                                                color: "#181824"
                                                border.color: "#282838"
                                                TextInput {
                                                    anchors.fill: parent
                                                    anchors.margins: 6
                                                    text: itemFormCol.activeItem ? (itemFormCol.activeItem.icon || "") : ""
                                                    color: "#ddd"
                                                    font.pixelSize: 12
                                                    selectByMouse: true
                                                    onTextEdited: itemFormCol.setItemProperty("icon", text)
                                                }
                                            }
                                        }
                                    }

                                    // Command Field
                                    Text { text: "Command (Shell Exec):"; color: "#aaaab5"; font.pixelSize: 12; font.bold: true }
                                    Rectangle {
                                        width: parent.width
                                        height: 36
                                        radius: 6
                                        color: "#1b1b26"
                                        border.color: cmdInput.activeFocus ? "#3b82f6" : "#2e2e3e"
                                        border.width: 1

                                        TextInput {
                                            id: cmdInput
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            text: itemFormCol.activeItem ? (itemFormCol.activeItem.command || "") : ""
                                            color: "#ffffff"
                                            font.pixelSize: 13
                                            selectByMouse: true
                                            onTextEdited: itemFormCol.setItemProperty("command", text)
                                        }
                                    }

                                    // Submenu or Shortcut Key
                                    Row {
                                        width: parent.width
                                        spacing: 12

                                        Column {
                                            width: parent.width / 2 - 6
                                            spacing: 4
                                            Text { text: "Submenu Target:"; color: "#aaaab5"; font.pixelSize: 12; font.bold: true }
                                            Rectangle {
                                                width: parent.width
                                                height: 34
                                                radius: 6
                                                color: "#1b1b26"
                                                border.color: "#2e2e3e"

                                                TextInput {
                                                    anchors.fill: parent
                                                    anchors.margins: 6
                                                    text: itemFormCol.activeItem ? (itemFormCol.activeItem.submenu || "") : ""
                                                    color: "#c084fc"
                                                    font.pixelSize: 12
                                                    font.bold: true
                                                    selectByMouse: true
                                                    onTextEdited: itemFormCol.setItemProperty("submenu", text)
                                                }
                                            }
                                        }

                                        Column {
                                            width: parent.width / 2 - 6
                                            spacing: 4
                                            Text { text: "Shortcut Key:"; color: "#aaaab5"; font.pixelSize: 12; font.bold: true }
                                            Rectangle {
                                                width: parent.width
                                                height: 34
                                                radius: 6
                                                color: "#1b1b26"
                                                border.color: "#2e2e3e"

                                                TextInput {
                                                    anchors.fill: parent
                                                    anchors.margins: 6
                                                    text: itemFormCol.activeItem ? (itemFormCol.activeItem.key || "") : ""
                                                    color: "#60a5fa"
                                                    font.pixelSize: 12
                                                    font.bold: true
                                                    selectByMouse: true
                                                    onTextEdited: itemFormCol.setItemProperty("key", text)
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // ── Tab 1: Menu Settings ────────────────────────────
                        Flickable {
                            visible: parent.inspectorTab === 1
                            width: parent.width
                            height: parent.height - 50
                            contentHeight: menuSettingsCol.height + 30
                            clip: true

                            Column {
                                id: menuSettingsCol
                                width: parent.width
                                spacing: 14

                                function setMenuProperty(prop, value) {
                                    let m = Object.assign({}, editorWindow.currentMenuData())
                                    if (value === "") delete m[prop]
                                    else m[prop] = value
                                    let copy = Object.assign({}, editorWindow.menusMap)
                                    copy[editorWindow.selectedMenuKey] = m
                                    editorWindow.menusMap = copy
                                }

                                function setStyleProperty(prop, value) {
                                    let m = Object.assign({}, editorWindow.currentMenuData())
                                    let s = Object.assign({}, m.style || {})
                                    if (value === "") delete s[prop]
                                    else s[prop] = value
                                    m.style = s
                                    let copy = Object.assign({}, editorWindow.menusMap)
                                    copy[editorWindow.selectedMenuKey] = m
                                    editorWindow.menusMap = copy
                                }

                                Text { text: "Menu Theme Preset:"; color: "#aaaab5"; font.pixelSize: 12; font.bold: true }

                                // Theme Selector List
                                Flow {
                                    width: parent.width
                                    spacing: 6

                                    Repeater {
                                        model: configBackend ? configBackend.availableThemes() : []

                                        Rectangle {
                                            width: 120
                                            height: 28
                                            radius: 4
                                            color: {
                                                let curTheme = editorWindow.currentMenuData().theme || editorWindow.configDoc.theme || "user-glass"
                                                return curTheme === modelData ? "#3b82f6" : (themeHover.containsMouse ? "#2a2a3c" : "#1c1c28")
                                            }
                                            border.color: "#303045"

                                            Text {
                                                anchors.centerIn: parent
                                                text: modelData
                                                color: "#ffffff"
                                                font.pixelSize: 11
                                                elide: Text.ElideRight
                                                width: parent.width - 10
                                                horizontalAlignment: Text.AlignHCenter
                                            }

                                            MouseArea {
                                                id: themeHover
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: menuSettingsCol.setMenuProperty("theme", modelData)
                                            }
                                        }
                                    }
                                }

                                Text { text: "Layout Style:"; color: "#aaaab5"; font.pixelSize: 12; font.bold: true }
                                Row {
                                    spacing: 10

                                    Rectangle {
                                        width: 100
                                        height: 32
                                        radius: 6
                                        color: {
                                            let s = editorWindow.currentMenuData().style || {}
                                            return s.layout === "pie" ? "#3b82f6" : "#20202e"
                                        }
                                        Text {
                                            anchors.centerIn: parent
                                            text: "🥧 Pie Menu"
                                            color: "#ffffff"
                                            font.pixelSize: 12
                                            font.bold: true
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: menuSettingsCol.setStyleProperty("layout", "pie")
                                        }
                                    }

                                    Rectangle {
                                        width: 100
                                        height: 32
                                        radius: 6
                                        color: {
                                            let s = editorWindow.currentMenuData().style || {}
                                            return s.layout !== "pie" ? "#3b82f6" : "#20202e"
                                        }
                                        Text {
                                            anchors.centerIn: parent
                                            text: "💊 Ring / Pills"
                                            color: "#ffffff"
                                            font.pixelSize: 12
                                            font.bold: true
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: menuSettingsCol.setStyleProperty("layout", "ring")
                                        }
                                    }
                                }

                                Text { text: "Behavior Options:"; color: "#aaaab5"; font.pixelSize: 12; font.bold: true }

                                // Spawn At Mouse toggle
                                Rectangle {
                                    width: parent.width
                                    height: 36
                                    radius: 6
                                    color: "#1b1b28"
                                    border.color: "#2c2c3e"

                                    Row {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 10

                                        Text {
                                            text: (editorWindow.currentMenuData().spawnAtMouse !== false) ? "☑" : "☐"
                                            color: "#3b82f6"
                                            font.pixelSize: 16
                                        }

                                        Text {
                                            text: "Spawn menu at mouse cursor position"
                                            color: "#ffffff"
                                            font.pixelSize: 12
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            let cur = editorWindow.currentMenuData().spawnAtMouse !== false
                                            menuSettingsCol.setMenuProperty("spawnAtMouse", !cur)
                                        }
                                    }
                                }

                                // Escape Closes All toggle
                                Rectangle {
                                    width: parent.width
                                    height: 36
                                    radius: 6
                                    color: "#1b1b28"
                                    border.color: "#2c2c3e"

                                    Row {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 10

                                        Text {
                                            text: (editorWindow.currentMenuData().escapeClosesAll === true) ? "☑" : "☐"
                                            color: "#3b82f6"
                                            font.pixelSize: 16
                                        }

                                        Text {
                                            text: "Escape key closes entire menu stack"
                                            color: "#ffffff"
                                            font.pixelSize: 12
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            let cur = editorWindow.currentMenuData().escapeClosesAll === true
                                            menuSettingsCol.setMenuProperty("escapeClosesAll", !cur)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ── Icon & Application Picker Modal ──────────────────────────────────────
    IconPickerModal {
        id: iconPickerModal

        onAppSelected: function(app) {
            let items = editorWindow.currentItems().slice()
            if (editorWindow.selectedItemIndex < 0 || editorWindow.selectedItemIndex >= items.length) return
            let it = Object.assign({}, items[editorWindow.selectedItemIndex])
            it.label = app.name || app.appId
            it.command = app.exec || ""
            it.iconName = app.iconName || ""
            delete it.icon
            items[editorWindow.selectedItemIndex] = it
            let m = editorWindow.currentMenuData()
            m.items = items
            let copy = Object.assign({}, editorWindow.menusMap)
            copy[editorWindow.selectedMenuKey] = m
            editorWindow.menusMap = copy
        }

        onIconSelected: function(iconName) {
            let items = editorWindow.currentItems().slice()
            if (editorWindow.selectedItemIndex < 0 || editorWindow.selectedItemIndex >= items.length) return
            let it = Object.assign({}, items[editorWindow.selectedItemIndex])
            it.iconName = iconName
            delete it.icon
            items[editorWindow.selectedItemIndex] = it
            let m = editorWindow.currentMenuData()
            m.items = items
            let copy = Object.assign({}, editorWindow.menusMap)
            copy[editorWindow.selectedMenuKey] = m
            editorWindow.menusMap = copy
        }

        onEmojiSelected: function(emoji) {
            let items = editorWindow.currentItems().slice()
            if (editorWindow.selectedItemIndex < 0 || editorWindow.selectedItemIndex >= items.length) return
            let it = Object.assign({}, items[editorWindow.selectedItemIndex])
            it.icon = emoji
            delete it.iconName
            items[editorWindow.selectedItemIndex] = it
            let m = editorWindow.currentMenuData()
            m.items = items
            let copy = Object.assign({}, editorWindow.menusMap)
            copy[editorWindow.selectedMenuKey] = m
            editorWindow.menusMap = copy
        }
    }
}
