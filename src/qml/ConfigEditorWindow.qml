import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: editorWindow
    visible: true
    width: 980
    height: 720
    minimumWidth: 860
    minimumHeight: 600
    title: "drmenu Configuration Editor"

    SystemPalette {
        id: sysPalette
        colorGroup: SystemPalette.Active
    }

    color: sysPalette.window

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

        // Sync combo box
        for (let i = 0; i < availableFiles.length; ++i) {
            if (availableFiles[i] === path) {
                fileComboBox.currentIndex = i
                break
            }
        }
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
            saveStatusMessage = "Configuration saved successfully!"
            statusTimer.restart()
        } else {
            saveStatusMessage = "Failed to save configuration!"
            statusTimer.restart()
        }
    }

    Timer {
        id: statusTimer
        interval: 3000
        onTriggered: editorWindow.saveStatusMessage = ""
    }

    Shortcut {
        sequence: "Ctrl+S"
        onActivated: saveActiveConfig()
    }

    Shortcut {
        sequence: "Ctrl+T"
        onActivated: {
            editorWindow.saveActiveConfig()
            if (configBackend) configBackend.testMenu(selectedMenuKey, activeFilePath)
        }
    }

    Column {
        anchors.fill: parent

        // ── Top Standard Toolbar ─────────────────────────────────────────────
        ToolBar {
            width: parent.width
            height: 48

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: 12

                Label {
                    text: "Config File:"
                    anchors.verticalCenter: parent.verticalCenter
                    font.bold: true
                }

                ComboBox {
                    id: fileComboBox
                    width: 240
                    anchors.verticalCenter: parent.verticalCenter
                    model: {
                        let names = []
                        for (let i = 0; i < editorWindow.availableFiles.length; ++i) {
                            let p = editorWindow.availableFiles[i]
                            names.push(p.substring(p.lastIndexOf("/") + 1))
                        }
                        return names
                    }
                    onActivated: function(index) {
                        if (index >= 0 && index < editorWindow.availableFiles.length) {
                            editorWindow.loadFromFile(editorWindow.availableFiles[index])
                        }
                    }
                }
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10

                Label {
                    text: editorWindow.saveStatusMessage
                    color: editorWindow.saveStatusMessage.includes("success") ? "#22c55e" : "#ef4444"
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                    visible: editorWindow.saveStatusMessage !== ""
                }

                Button {
                    text: "▶ Test Menu"
                    anchors.verticalCenter: parent.verticalCenter
                    ToolTip.text: "Launch standalone test of selected menu (Ctrl+T)"
                    ToolTip.visible: hovered
                    onClicked: {
                        editorWindow.saveActiveConfig()
                        if (configBackend) configBackend.testMenu(editorWindow.selectedMenuKey, editorWindow.activeFilePath)
                    }
                }

                Button {
                    text: "💾 Save Config"
                    highlighted: true
                    anchors.verticalCenter: parent.verticalCenter
                    ToolTip.text: "Save changes to file (Ctrl+S)"
                    ToolTip.visible: hovered
                    onClicked: editorWindow.saveActiveConfig()
                }
            }
        }

        // ── Three-Pane Main Desktop Layout ───────────────────────────────────
        Item {
            width: parent.width
            height: parent.height - 48

            Row {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                // ── Pane 1: Menus ────────────────────────────────────────────
                GroupBox {
                    title: "Menus"
                    width: 210
                    height: parent.height

                    Column {
                        anchors.fill: parent
                        spacing: 6

                        ScrollView {
                            width: parent.width
                            height: parent.height - 40
                            clip: true

                            ListView {
                                id: menuListView
                                model: editorWindow.menuKeys
                                spacing: 2

                                delegate: ItemDelegate {
                                    width: menuListView.width
                                    height: 36
                                    highlighted: editorWindow.selectedMenuKey === modelData

                                    contentItem: Row {
                                        spacing: 8
                                        anchors.verticalCenter: parent.verticalCenter

                                        Text {
                                            text: modelData === "main" ? "🏠" : "📂"
                                            font.pixelSize: 13
                                            anchors.verticalCenter: parent.verticalCenter
                                        }

                                        Text {
                                            text: modelData
                                            color: parent.parent.highlighted ? sysPalette.highlightedText : sysPalette.text
                                            font.bold: parent.parent.highlighted
                                            font.pixelSize: 13
                                            elide: Text.ElideRight
                                            width: menuListView.width - 50
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }

                                    onClicked: {
                                        editorWindow.selectedMenuKey = modelData
                                        editorWindow.selectedItemIndex = 0
                                    }
                                }
                            }
                        }

                        Row {
                            width: parent.width
                            spacing: 6

                            Button {
                                text: "+ Add"
                                width: (parent.width - 6) / 2
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

                            Button {
                                text: "- Delete"
                                width: (parent.width - 6) / 2
                                enabled: editorWindow.menuKeys.length > 1 && editorWindow.selectedMenuKey !== "main"
                                onClicked: {
                                    let copy = Object.assign({}, editorWindow.menusMap)
                                    delete copy[editorWindow.selectedMenuKey]
                                    editorWindow.menusMap = copy
                                    let keys = Object.keys(copy)
                                    editorWindow.menuKeys = keys
                                    editorWindow.selectedMenuKey = keys.length > 0 ? keys[0] : ""
                                    editorWindow.selectedItemIndex = 0
                                }
                            }
                        }
                    }
                }

                // ── Pane 2: Menu Items ───────────────────────────────────────
                GroupBox {
                    title: "Items in '" + editorWindow.selectedMenuKey + "'"
                    width: 360
                    height: parent.height

                    Column {
                        anchors.fill: parent
                        spacing: 6

                        // Item Action Buttons
                        Row {
                            width: parent.width
                            spacing: 6

                            Button {
                                text: "+ Add Item"
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

                            Button {
                                text: "▲ Up"
                                enabled: editorWindow.selectedItemIndex > 0
                                onClicked: {
                                    let idx = editorWindow.selectedItemIndex
                                    let m = editorWindow.currentMenuData()
                                    let items = editorWindow.currentItems().slice()
                                    let tmp = items[idx]
                                    items[idx] = items[idx - 1]
                                    items[idx - 1] = tmp
                                    m.items = items
                                    let copy = Object.assign({}, editorWindow.menusMap)
                                    copy[editorWindow.selectedMenuKey] = m
                                    editorWindow.menusMap = copy
                                    editorWindow.selectedItemIndex = idx - 1
                                }
                            }

                            Button {
                                text: "▼ Down"
                                enabled: editorWindow.selectedItemIndex >= 0 && editorWindow.selectedItemIndex < editorWindow.currentItems().length - 1
                                onClicked: {
                                    let idx = editorWindow.selectedItemIndex
                                    let m = editorWindow.currentMenuData()
                                    let items = editorWindow.currentItems().slice()
                                    let tmp = items[idx]
                                    items[idx] = items[idx + 1]
                                    items[idx + 1] = tmp
                                    m.items = items
                                    let copy = Object.assign({}, editorWindow.menusMap)
                                    copy[editorWindow.selectedMenuKey] = m
                                    editorWindow.menusMap = copy
                                    editorWindow.selectedItemIndex = idx + 1
                                }
                            }

                            Button {
                                text: "🗑 Delete"
                                enabled: editorWindow.selectedItemIndex >= 0 && editorWindow.currentItems().length > 0
                                onClicked: {
                                    let idx = editorWindow.selectedItemIndex
                                    let m = editorWindow.currentMenuData()
                                    let items = editorWindow.currentItems().slice()
                                    items.splice(idx, 1)
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

                        ScrollView {
                            width: parent.width
                            height: parent.height - 46
                            clip: true

                            ListView {
                                id: itemsListView
                                model: editorWindow.currentItems()
                                spacing: 2

                                delegate: ItemDelegate {
                                    width: itemsListView.width
                                    height: 48
                                    highlighted: editorWindow.selectedItemIndex === index

                                    contentItem: Row {
                                        spacing: 10
                                        anchors.verticalCenter: parent.verticalCenter

                                        // Icon preview
                                        Rectangle {
                                            width: 32
                                            height: 32
                                            radius: 4
                                            color: sysPalette.base
                                            border.color: sysPalette.mid
                                            border.width: 1
                                            anchors.verticalCenter: parent.verticalCenter

                                            Text {
                                                visible: modelData.icon !== undefined && modelData.icon !== ""
                                                text: modelData.icon || ""
                                                font.pixelSize: 18
                                                anchors.centerIn: parent
                                            }

                                            Image {
                                                visible: !modelData.icon && (modelData.iconName !== undefined && modelData.iconName !== "")
                                                source: modelData.iconName ? ("image://icon/" + modelData.iconName) : ""
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
                                                color: sysPalette.dark
                                                anchors.centerIn: parent
                                            }
                                        }

                                        Column {
                                            width: itemsListView.width - 90
                                            anchors.verticalCenter: parent.verticalCenter
                                            spacing: 2

                                            Text {
                                                text: modelData.label || modelData.app || "(untitled)"
                                                color: parent.parent.parent.highlighted ? sysPalette.highlightedText : sysPalette.text
                                                font.bold: true
                                                font.pixelSize: 13
                                                elide: Text.ElideRight
                                                width: parent.width
                                            }

                                            Text {
                                                text: modelData.submenu ? ("Submenu ➔ " + modelData.submenu) : (modelData.command ? modelData.command : (modelData.app ? ("App: " + modelData.app) : ""))
                                                color: parent.parent.parent.highlighted ? sysPalette.highlightedText : sysPalette.dark
                                                font.pixelSize: 11
                                                elide: Text.ElideRight
                                                width: parent.width
                                            }
                                        }

                                        Label {
                                            text: modelData.key ? ("[" + modelData.key + "]") : ""
                                            color: parent.parent.highlighted ? sysPalette.highlightedText : sysPalette.highlight
                                            font.bold: true
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }

                                    onClicked: editorWindow.selectedItemIndex = index
                                }
                            }
                        }
                    }
                }

                // ── Pane 3: Properties Inspector ─────────────────────────────
                GroupBox {
                    title: "Properties"
                    width: parent.width - 594
                    height: parent.height

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        TabBar {
                            id: inspectorTabBar
                            Layout.fillWidth: true
                            currentIndex: 0

                            TabButton { text: "Item Details" }
                            TabButton { text: "Menu Settings" }
                        }

                        StackLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            currentIndex: inspectorTabBar.currentIndex

                            // ── Tab 0: Item Details ──────────────────────────────
                            ScrollView {
                                id: itemDetailsScroll
                                clip: true
                                contentWidth: availableWidth
                                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                                Column {
                                    id: itemDetailsPane
                                    width: itemDetailsScroll.availableWidth - 8
                                    spacing: 12

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

                                    Label {
                                        visible: !itemDetailsPane.activeItem
                                        text: "No item selected."
                                        font.italic: true
                                    }

                                    Column {
                                        id: itemFormCol
                                        visible: itemDetailsPane.activeItem !== null
                                        width: itemDetailsPane.width
                                        spacing: 10

                                        Label { text: "Label / Title:"; font.bold: true }
                                        TextField {
                                            width: itemFormCol.width
                                            text: itemDetailsPane.activeItem ? (itemDetailsPane.activeItem.label || "") : ""
                                            placeholderText: "Action label (e.g. Google Antigravity)"
                                            selectByMouse: true
                                            onTextEdited: itemDetailsPane.setItemProperty("label", text)
                                        }

                                        Label { text: "Icon Selection:"; font.bold: true }
                                        RowLayout {
                                            width: itemFormCol.width
                                            spacing: 10

                                            Rectangle {
                                                Layout.preferredWidth: 40
                                                Layout.preferredHeight: 40
                                                radius: 6
                                                color: sysPalette.base
                                                border.color: sysPalette.mid
                                                border.width: 1

                                                Text {
                                                    visible: itemDetailsPane.activeItem && itemDetailsPane.activeItem.icon
                                                    text: itemDetailsPane.activeItem ? (itemDetailsPane.activeItem.icon || "") : ""
                                                    font.pixelSize: 22
                                                    anchors.centerIn: parent
                                                }

                                                Image {
                                                    visible: itemDetailsPane.activeItem && !itemDetailsPane.activeItem.icon && itemDetailsPane.activeItem.iconName
                                                    source: (itemDetailsPane.activeItem && itemDetailsPane.activeItem.iconName) ? ("image://icon/" + itemDetailsPane.activeItem.iconName) : ""
                                                    sourceSize.width: 32
                                                    sourceSize.height: 32
                                                    width: 32
                                                    height: 32
                                                    anchors.centerIn: parent
                                                }
                                            }

                                            Button {
                                                text: "Browse Icons & Apps..."
                                                Layout.fillWidth: true
                                                onClicked: iconPickerDialog.openPicker()
                                            }
                                        }

                                        RowLayout {
                                            width: itemFormCol.width
                                            spacing: 8

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                spacing: 4
                                                Label { text: "Icon Name (Theme):"; font.pixelSize: 11; font.bold: true }
                                                TextField {
                                                    Layout.fillWidth: true
                                                    text: itemDetailsPane.activeItem ? (itemDetailsPane.activeItem.iconName || "") : ""
                                                    placeholderText: "e.g. antigravity"
                                                    selectByMouse: true
                                                    onTextEdited: itemDetailsPane.setItemProperty("iconName", text)
                                                }
                                            }

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                spacing: 4
                                                Label { text: "Emoji / Symbol:"; font.pixelSize: 11; font.bold: true }
                                                TextField {
                                                    Layout.fillWidth: true
                                                    text: itemDetailsPane.activeItem ? (itemDetailsPane.activeItem.icon || "") : ""
                                                    placeholderText: "e.g. 🚀"
                                                    selectByMouse: true
                                                    onTextEdited: itemDetailsPane.setItemProperty("icon", text)
                                                }
                                            }
                                        }

                                        Label { text: "Command (Shell Exec):"; font.bold: true }
                                        TextField {
                                            width: itemFormCol.width
                                            text: itemDetailsPane.activeItem ? (itemDetailsPane.activeItem.command || "") : ""
                                            placeholderText: "e.g. antigravity %U"
                                            selectByMouse: true
                                            onTextEdited: itemDetailsPane.setItemProperty("command", text)
                                        }

                                        RowLayout {
                                            width: itemFormCol.width
                                            spacing: 8

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                spacing: 4
                                                Label { text: "Submenu Target:"; font.bold: true }
                                                TextField {
                                                    Layout.fillWidth: true
                                                    text: itemDetailsPane.activeItem ? (itemDetailsPane.activeItem.submenu || "") : ""
                                                    placeholderText: "e.g. system"
                                                    selectByMouse: true
                                                    onTextEdited: itemDetailsPane.setItemProperty("submenu", text)
                                                }
                                            }

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                spacing: 4
                                                Label { text: "Shortcut Key:"; font.bold: true }
                                                TextField {
                                                    Layout.fillWidth: true
                                                    text: itemDetailsPane.activeItem ? (itemDetailsPane.activeItem.key || "") : ""
                                                    placeholderText: "e.g. s or 1"
                                                    selectByMouse: true
                                                    onTextEdited: itemDetailsPane.setItemProperty("key", text)
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // ── Tab 1: Menu Settings ─────────────────────────────
                            ScrollView {
                                id: menuSettingsScroll
                                clip: true
                                contentWidth: availableWidth
                                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                                Column {
                                    id: menuSettingsCol
                                    width: menuSettingsScroll.availableWidth - 8
                                    spacing: 12

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

                                    Label { text: "Theme Preset:"; font.bold: true }
                                    ComboBox {
                                        id: themeComboBox
                                        width: menuSettingsCol.width
                                        model: configBackend ? configBackend.availableThemes() : []

                                        function updateCurrentThemeIndex() {
                                            let cur = editorWindow.currentMenuData().theme || (editorWindow.configDoc ? editorWindow.configDoc.theme : "") || "user-glass"
                                            let idx = model.indexOf(cur)
                                            if (idx !== -1) currentIndex = idx
                                        }

                                        Component.onCompleted: updateCurrentThemeIndex()

                                        Connections {
                                            target: editorWindow
                                            function onSelectedMenuKeyChanged() {
                                                themeComboBox.updateCurrentThemeIndex()
                                            }
                                            function onMenusMapChanged() {
                                                themeComboBox.updateCurrentThemeIndex()
                                            }
                                        }

                                        onActivated: function(index) {
                                            let selected = model[index]
                                            let m = Object.assign({}, editorWindow.currentMenuData())
                                            m.theme = selected
                                            // Prune conflicting theme overrides from menu.style, preserving layout
                                            if (m.style) {
                                                let cleanStyle = {}
                                                if (m.style.layout) {
                                                    cleanStyle.layout = m.style.layout
                                                }
                                                m.style = cleanStyle
                                            }
                                            let copy = Object.assign({}, editorWindow.menusMap)
                                            copy[editorWindow.selectedMenuKey] = m
                                            editorWindow.menusMap = copy

                                            if (editorWindow.selectedMenuKey === "main" && editorWindow.configDoc) {
                                                editorWindow.configDoc.theme = selected
                                            }
                                        }
                                    }

                                    Label { text: "Layout Style:"; font.bold: true }
                                    Row {
                                        spacing: 16

                                        RadioButton {
                                            text: "🥧 Pie Menu"
                                            checked: {
                                                let s = editorWindow.currentMenuData().style || {}
                                                return s.layout === "pie"
                                            }
                                            onClicked: menuSettingsCol.setStyleProperty("layout", "pie")
                                        }

                                        RadioButton {
                                            text: "💊 Ring / Pills"
                                            checked: {
                                                let s = editorWindow.currentMenuData().style || {}
                                                return s.layout !== "pie"
                                            }
                                            onClicked: menuSettingsCol.setStyleProperty("layout", "ring")
                                        }
                                    }

                                    Label { text: "Behavior:"; font.bold: true }
                                    CheckBox {
                                        text: "Spawn menu at mouse cursor position"
                                        checked: editorWindow.currentMenuData().spawnAtMouse !== false
                                        onToggled: menuSettingsCol.setMenuProperty("spawnAtMouse", checked)
                                    }

                                    CheckBox {
                                        text: "Escape key closes entire menu stack"
                                        checked: editorWindow.currentMenuData().escapeClosesAll === true
                                        onToggled: menuSettingsCol.setMenuProperty("escapeClosesAll", checked)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ── Icon & Application Picker Dialog ─────────────────────────────────────
    IconPickerModal {
        id: iconPickerDialog

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
