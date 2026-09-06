import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: iconPickerDialog
    title: "Select Icon or Application"
    modal: true
    anchors.centerIn: parent
    width: Math.min(parent.width - 40, 760)
    height: Math.min(parent.height - 40, 580)
    standardButtons: Dialog.Close

    signal iconSelected(string iconName)
    signal emojiSelected(string emoji)
    signal appSelected(var appData)

    property var allAppsList: []
    property var filteredAppsList: []
    property var filteredIconsList: []

    SystemPalette {
        id: sysPalette
        colorGroup: SystemPalette.Active
    }

    function openPicker(tabIndex) {
        searchField.text = ""
        tabBar.currentIndex = (typeof tabIndex === "number") ? tabIndex : 0
        refreshLists()
        open()
        searchField.forceActiveFocus()
    }

    function refreshLists() {
        let q = searchField.text.trim().toLowerCase()
        if (allAppsList.length === 0 && configBackend) {
            allAppsList = configBackend.installedApps()
        }

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

        if (configBackend) {
            filteredIconsList = configBackend.searchSystemIcons(q, 150)
        }
    }

    contentItem: ColumnLayout {
        spacing: 10

        // Search Input
        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: "Search applications, icon names, or keywords..."
            selectByMouse: true
            onTextChanged: searchTimer.restart()

            Timer {
                id: searchTimer
                interval: 120
                repeat: false
                onTriggered: iconPickerDialog.refreshLists()
            }
        }

        // Tab Bar
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            currentIndex: 0

            TabButton {
                text: "Installed Applications (" + iconPickerDialog.filteredAppsList.length + ")"
            }
            TabButton {
                text: "System Theme Icons (" + iconPickerDialog.filteredIconsList.length + ")"
            }
            TabButton {
                text: "Emojis & Symbols"
            }
        }

        // Tab Content
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // ── Tab 0: Applications ──────────────────────────────────────────
            ScrollView {
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ListView {
                    id: appsView
                    anchors.fill: parent
                    model: iconPickerDialog.filteredAppsList
                    spacing: 2
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: ItemDelegate {
                        width: appsView.width
                        height: 50

                        contentItem: RowLayout {
                            spacing: 12

                            Image {
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: 32
                                source: modelData.iconName ? ("image://icon/" + modelData.iconName) : "image://icon/application-x-executable"
                                sourceSize.width: 32
                                sourceSize.height: 32
                                fillMode: Image.PreserveAspectFit
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: modelData.name || modelData.appId
                                    color: sysPalette.text
                                    font.bold: true
                                    font.pixelSize: 13
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: modelData.exec ? ("Exec: " + modelData.exec) : (modelData.iconName ? ("Icon: " + modelData.iconName) : "")
                                    color: sysPalette.dark
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        onClicked: {
                            iconPickerDialog.appSelected(modelData)
                            iconPickerDialog.close()
                        }
                    }
                }
            }

            // ── Tab 1: System Icons ──────────────────────────────────────────
            ScrollView {
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                GridView {
                    id: iconsGrid
                    anchors.fill: parent
                    cellWidth: 80
                    cellHeight: 76
                    model: iconPickerDialog.filteredIconsList
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: ItemDelegate {
                        width: 76
                        height: 72

                        contentItem: ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 4

                            Image {
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: 32
                                Layout.alignment: Qt.AlignHCenter
                                source: "image://icon/" + modelData
                                sourceSize.width: 32
                                sourceSize.height: 32
                                fillMode: Image.PreserveAspectFit
                            }

                            Text {
                                text: modelData
                                color: sysPalette.text
                                font.pixelSize: 9
                                Layout.preferredWidth: 70
                                elide: Text.ElideMiddle
                                horizontalAlignment: Text.AlignHCenter
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }

                        onClicked: {
                            iconPickerDialog.iconSelected(modelData)
                            iconPickerDialog.close()
                        }
                    }
                }
            }

            // ── Tab 2: Emojis & Symbols ──────────────────────────────────────
            ScrollView {
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Flow {
                    width: parent.width
                    spacing: 8

                    property var emojis: [
                        "🌐", "⚙", "🎵", "🎮", "📺", "💻", "🎬", "📻",
                        "⏯", "⏭", "⏮", "🔊", "🔉", "🔇", "🔍", "▶", "◀",
                        "⏻", "↺", "💤", "🔒", "🚪", "⚡", "🔋", "📶", "🛡",
                        "📁", "📂", "📄", "📋", "💾", "🛠", "🔧", "🔨",
                        "🖥", "⌨", "🖱", "📦", "🚀", "💡", "📌", "✨", "🔥", "⭐",
                        "💬", "✉", "📞", "🌍", "🦊", "🐱", "🎧", "🎨", "📸",
                        "🍎", "🐧", "🎯", "💎", "🔮", "🍕", "☕"
                    ]

                    Repeater {
                        model: parent.emojis

                        Button {
                            width: 46
                            height: 46
                            text: modelData
                            font.pixelSize: 20
                            onClicked: {
                                iconPickerDialog.emojiSelected(modelData)
                                iconPickerDialog.close()
                            }
                        }
                    }
                }
            }
        }
    }
}
