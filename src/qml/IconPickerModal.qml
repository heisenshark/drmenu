import QtQuick

Rectangle {
    id: iconPickerModal
    anchors.fill: parent
    color: "#bb000000"
    visible: false
    z: 1000

    signal iconSelected(string iconName)
    signal emojiSelected(string emoji)
    signal appSelected(var appData)
    signal closed()

    // Prevent clicks from falling through
    MouseArea {
        anchors.fill: parent
        onClicked: iconPickerModal.close()
    }

    function open() {
        searchField.text = ""
        currentTab = 0
        visible = true
        searchField.forceActiveFocus()
        refreshLists()
    }

    function close() {
        visible = false
        closed()
    }

    property int currentTab: 0 // 0: Apps, 1: System Icons, 2: Emojis
    property var allAppsList: []
    property var filteredAppsList: []
    property var filteredIconsList: []

    function refreshLists() {
        let q = searchField.text.trim().toLowerCase()
        if (allAppsList.length === 0 && configBackend) {
            allAppsList = configBackend.installedApps()
        }

        // Filter apps
        if (q === "") {
            filteredAppsList = allAppsList
        } else {
            let res = []
            for (let i = 0; i < allAppsList.length; ++i) {
                let app = allAppsList[i]
                if (app.name.toLowerCase().includes(q) ||
                    app.appId.toLowerCase().includes(q) ||
                    (app.exec && app.exec.toLowerCase().includes(q)) ||
                    (app.iconName && app.iconName.toLowerCase().includes(q))) {
                    res.push(app)
                }
            }
            filteredAppsList = res
        }

        // Filter system icons
        if (configBackend) {
            filteredIconsList = configBackend.searchSystemIcons(q, 150)
        }
    }

    Rectangle {
        id: dialogCard
        width: Math.min(parent.width - 40, 720)
        height: Math.min(parent.height - 40, 580)
        anchors.centerIn: parent
        radius: 12
        color: "#181820"
        border.color: "#353545"
        border.width: 1

        MouseArea {
            anchors.fill: parent // Absorb clicks inside dialog
        }

        Column {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 14

            // ── Dialog Header ───────────────────────────────────────────────
            Row {
                width: parent.width
                height: 28

                Text {
                    text: "Select Icon or Application"
                    color: "#ffffff"
                    font.pixelSize: 17
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item { width: 1; height: 1; property real flex: 1 }

                Rectangle {
                    width: 28
                    height: 28
                    radius: 14
                    color: closeHover.containsMouse ? "#3a3a4c" : "#242430"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        color: "#cccccc"
                        font.pixelSize: 13
                    }

                    MouseArea {
                        id: closeHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: iconPickerModal.close()
                    }
                }
            }

            // ── Search Field ────────────────────────────────────────────────
            Rectangle {
                width: parent.width
                height: 38
                radius: 8
                color: "#101015"
                border.color: searchField.activeFocus ? "#3b82f6" : "#2a2a38"
                border.width: 1

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Text {
                        text: "🔍"
                        color: "#808090"
                        font.pixelSize: 14
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    TextInput {
                        id: searchField
                        width: parent.width - 60
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#f0f0f5"
                        font.pixelSize: 14
                        clip: true
                        selectByMouse: true

                        Text {
                            anchors.fill: parent
                            text: "Search applications, icon names, keywords..."
                            color: "#555566"
                            font.pixelSize: 14
                            visible: !searchField.text && !searchField.activeFocus
                        }

                        onTextChanged: refreshTimer.restart()
                    }

                    Timer {
                        id: refreshTimer
                        interval: 120
                        repeat: false
                        onTriggered: iconPickerModal.refreshLists()
                    }
                }
            }

            // ── Tab Bar ─────────────────────────────────────────────────────
            Row {
                spacing: 8
                width: parent.width

                Repeater {
                    model: [
                        { label: "Installed Apps (" + iconPickerModal.filteredAppsList.length + ")" },
                        { label: "System Icons (" + iconPickerModal.filteredIconsList.length + ")" },
                        { label: "Emojis & Symbols" }
                    ]

                    Rectangle {
                        width: 170
                        height: 32
                        radius: 6
                        color: iconPickerModal.currentTab === index ? "#3b82f6" : (tabHover.containsMouse ? "#282836" : "#20202a")
                        border.color: iconPickerModal.currentTab === index ? "#60a5fa" : "#2e2e3e"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: modelData.label
                            color: iconPickerModal.currentTab === index ? "#ffffff" : "#aaaaaa"
                            font.pixelSize: 12
                            font.bold: iconPickerModal.currentTab === index
                        }

                        MouseArea {
                            id: tabHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: iconPickerModal.currentTab = index
                        }
                    }
                }
            }

            // ── Content Area ────────────────────────────────────────────────
            Item {
                width: parent.width
                height: dialogCard.height - 170
                clip: true

                // Tab 0: Applications
                ListView {
                    id: appsView
                    anchors.fill: parent
                    visible: iconPickerModal.currentTab === 0
                    model: iconPickerModal.filteredAppsList
                    spacing: 4

                    delegate: Rectangle {
                        width: appsView.width
                        height: 48
                        radius: 6
                        color: appRowHover.containsMouse ? "#262638" : "#1a1a24"
                        border.color: appRowHover.containsMouse ? "#3b82f6" : "#252532"
                        border.width: 1

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 12
                            anchors.verticalCenter: parent.verticalCenter

                            Image {
                                width: 32
                                height: 32
                                source: modelData.iconName ? "image://icon/" + modelData.iconName : "image://icon/application-x-executable"
                                sourceSize.width: 32
                                sourceSize.height: 32
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 2
                                width: parent.width - 50

                                Text {
                                    text: modelData.name || modelData.appId
                                    color: "#ffffff"
                                    font.pixelSize: 13
                                    font.bold: true
                                    elide: Text.ElideRight
                                    width: parent.width
                                }

                                Text {
                                    text: modelData.exec ? ("Exec: " + modelData.exec) : (modelData.iconName ? ("Icon: " + modelData.iconName) : "")
                                    color: "#777788"
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                    width: parent.width
                                }
                            }
                        }

                        MouseArea {
                            id: appRowHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                iconPickerModal.appSelected(modelData)
                                iconPickerModal.close()
                            }
                        }
                    }
                }

                // Tab 1: System Theme Icons
                GridView {
                    id: iconsGrid
                    anchors.fill: parent
                    visible: iconPickerModal.currentTab === 1
                    cellWidth: 80
                    cellHeight: 76
                    model: iconPickerModal.filteredIconsList

                    delegate: Rectangle {
                        width: 74
                        height: 70
                        radius: 6
                        color: iconTileHover.containsMouse ? "#28283e" : "#191924"
                        border.color: iconTileHover.containsMouse ? "#3b82f6" : "#232330"
                        border.width: 1

                        Column {
                            anchors.centerIn: parent
                            spacing: 4

                            Image {
                                width: 32
                                height: 32
                                source: "image://icon/" + modelData
                                sourceSize.width: 32
                                sourceSize.height: 32
                                anchors.horizontalCenter: parent.horizontalCenter
                            }

                            Text {
                                text: modelData
                                color: "#aaaaaa"
                                font.pixelSize: 9
                                width: 68
                                elide: Text.ElideMiddle
                                horizontalAlignment: Text.AlignHCenter
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }

                        MouseArea {
                            id: iconTileHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                iconPickerModal.iconSelected(modelData)
                                iconPickerModal.close()
                            }
                        }
                    }
                }

                // Tab 2: Emojis & Symbols
                Flickable {
                    anchors.fill: parent
                    visible: iconPickerModal.currentTab === 2
                    contentHeight: emojiFlow.height + 20
                    clip: true

                    Flow {
                        id: emojiFlow
                        width: parent.width
                        spacing: 8

                        property var emojis: [
                            "🌐", "⚙", "🎵", "🎮", "📺", "💻", "🎬", "📻",
                            "⏯", "⏭", "⏮", "🔊", "🔉", "🔇", "🔍", "▶", "◀",
                            "⏻", "↺", "💤", "🔒", "🚪", "⚡", "🔋", "📶", "🛡",
                            "📁", "📂", "📄", "📋", "💾", "🛠", "🔧", "🔨",
                            "🖥", "⌨", "🖱", "📦", "🚀", "💡", "📌", "✨", "🔥", "⭐",
                            "💬", "✉", "📞", "🌍", "🦊", "🐱", "🎧", "🎨", "📸",
                            "🍎", "🐧", "🚀", "🌟", "🎯", "💎", "🔮", "🍕", "☕"
                        ]

                        Repeater {
                            model: emojiFlow.emojis

                            Rectangle {
                                width: 44
                                height: 44
                                radius: 8
                                color: emojiHover.containsMouse ? "#35354a" : "#1f1f2a"
                                border.color: emojiHover.containsMouse ? "#3b82f6" : "#2a2a3a"
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.pixelSize: 22
                                }

                                MouseArea {
                                    id: emojiHover
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        iconPickerModal.emojiSelected(modelData)
                                        iconPickerModal.close()
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
