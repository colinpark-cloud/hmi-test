import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.12

Item {
    id: root
    anchors.fill: parent

    Timer {
        interval: 4; running: true; repeat: true
        onTriggered: root.rotationTick++
    }

    property int rotationTick: 0
    property real t: rotationTick / 40.0
    property real beat: 0.5 + 0.5 * Math.sin(root.t * 7.0)
    property real spin: root.t * 180

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#050816" }
            GradientStop { position: 0.5; color: "#111a2e" }
            GradientStop { position: 1.0; color: "#04070d" }
        }
    }

    Repeater {
        model: 8
        delegate: Rectangle {
            width: 90 + (index % 4) * 32
            height: width
            radius: width / 2
            x: (root.width * 0.12) + index * 95 + Math.sin(root.t * 4.0 + index * 0.7) * 60
            y: (index % 2 === 0 ? 20 : 90) + Math.cos(root.t * 3.6 + index) * 44
            color: index % 3 === 0 ? "#00c2ff" : (index % 3 === 1 ? "#8e24aa" : "#4cd964")
            opacity: 0.09 + beat * 0.08
            layer.enabled: true
            layer.effect: GaussianBlur { radius: 28; samples: 16 }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        Label {
            Layout.fillWidth: true
            text: "heartbeat=" + Math.round(beat * 100) + "%   spin=" + Math.round(spin) + "°"
            color: "#7ef"
            font.pixelSize: 11
            horizontalAlignment: Text.AlignRight
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Rectangle {
                width: 58; height: 58; radius: 29
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#00c2ff" }
                    GradientStop { position: 0.5; color: "#8e24aa" }
                    GradientStop { position: 1.0; color: "#4cd964" }
                }
                rotation: -root.spin * 0.7
                layer.enabled: true
                layer.effect: DropShadow { color: "#80ffffff"; radius: 18; samples: 16; verticalOffset: 0 }
                Rectangle { anchors.centerIn: parent; width: 28; height: 28; radius: 14; color: "#0b1020"; opacity: 0.85 }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Label { text: "NEON CONTROL CENTER"; color: "white"; font.pixelSize: 24; font.bold: true }
                Label { text: "Animated Qt Quick dashboard"; color: "#b8c7d6"; font.pixelSize: 12 }
            }
            Rectangle {
                width: 110; height: 34; radius: 17
                gradient: Gradient {
                    GradientStop { position: 0.0; color: beat > 0.5 ? "#1a3b6d" : "#10284a" }
                    GradientStop { position: 1.0; color: beat > 0.5 ? "#2b64b5" : "#183d70" }
                }
                border.color: beat > 0.5 ? "#74d9ff" : "#4cc9ff"
                border.width: 1
                opacity: 0.7 + beat * 0.3
                Label { anchors.centerIn: parent; text: "LIVE"; color: "#e8fbff"; font.pixelSize: 13; font.bold: true }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 18
                gradient: Gradient {
                    GradientStop { position: 0.0; color: beat > 0.5 ? "#10213a" : "#0d1a2c" }
                    GradientStop { position: 1.0; color: beat > 0.5 ? "#06111d" : "#0a111d" }
                }
                border.color: beat > 0.5 ? "#5aa9ff" : "#35608d"
                border.width: 2

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Rectangle {
                            width: 92; height: 92; radius: 46
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: "#00c2ff" }
                                GradientStop { position: 1.0; color: "#0057ff" }
                            }
                            rotation: root.spin
                            scale: 0.88 + beat * 0.24
                            layer.enabled: true
                            layer.effect: DropShadow { color: "#7a5bc0ff"; radius: 22; samples: 20; verticalOffset: 0 }
                            Canvas {
                                anchors.fill: parent
                                onPaint: {
                                    var ctx = getContext("2d")
                                    ctx.reset()
                                    ctx.lineWidth = 8
                                    ctx.strokeStyle = "#06111d"
                                    ctx.beginPath()
                                    ctx.arc(width/2, height/2, 28, 0, Math.PI * 2)
                                    ctx.stroke()
                                    ctx.lineWidth = 7
                                    ctx.strokeStyle = "#ffffff"
                                    ctx.beginPath()
                                    ctx.arc(width/2, height/2, 28, -Math.PI/2, -Math.PI/2 + Math.PI * (0.3 + beat * 1.7))
                                    ctx.stroke()
                                }
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label { text: "SYSTEM STATUS"; color: "white"; font.pixelSize: 18; font.bold: true }
                            Label { text: "High contrast, glow, motion, and live controls"; color: "#98abc4"; font.pixelSize: 11 }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 160
                        radius: 16
                        color: "#09111d"
                        border.color: "#28476a"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 9

                            Repeater {
                                model: [
                                    {name: "CPU", val: 0.78, col: "#00c2ff"},
                                    {name: "GPU", val: 0.64, col: "#8e24aa"},
                                    {name: "MEM", val: 0.53, col: "#4cd964"}
                                ]
                                delegate: ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 3
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label { text: modelData.name; color: "#e7f0ff"; font.pixelSize: 13; font.bold: true }
                                        Item { Layout.fillWidth: true }
                                        Label { text: Math.round(modelData.val * 100) + "%"; color: modelData.col; font.pixelSize: 13; font.bold: true }
                                    }
                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 14
                                        radius: 7
                                        color: "#16273b"
                                        border.color: "#2c4561"
                                        border.width: 1
                                        Rectangle {
                                            anchors.left: parent.left
                                            anchors.top: parent.top
                                            anchors.bottom: parent.bottom
                                            width: parent.width * modelData.val * (0.92 + beat * 0.08)
                                            radius: 7
                                            gradient: Gradient {
                                                GradientStop { position: 0.0; color: modelData.col }
                                                GradientStop { position: 1.0; color: modelData.col === "#00c2ff" ? "#59e3ff" : (modelData.col === "#8e24aa" ? "#d78cff" : "#b8ffcb") }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Repeater {
                            model: ["Widgets", "Quick", "Glow", "Motion", "Cards"]
                            delegate: Rectangle {
                                height: 32
                                radius: 16
                                border.width: 1
                                border.color: index % 2 === 0 ? "#00c2ff" : "#8e24aa"
                                opacity: 0.8 + beat * 0.2
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: index % 2 === 0 ? "#132a45" : "#22163e" }
                                    GradientStop { position: 1.0; color: index % 2 === 0 ? "#0f1f33" : "#19112b" }
                                }
                                Layout.preferredWidth: 70 + (modelData.length * 6)
                                Label { anchors.centerIn: parent; text: modelData; color: "white"; font.pixelSize: 11; font.bold: true }
                            }
                        }
                    }
                }
            }

            Rectangle {
                width: 200
                Layout.fillHeight: true
                radius: 18
                color: "#0c1320"
                border.color: "#395879"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10
                    Label { text: "METERS"; color: "white"; font.pixelSize: 17; font.bold: true }
                    Repeater {
                        model: [0.92, 0.68, 0.53, 0.84]
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 52
                            radius: 14
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: "#142238" }
                                GradientStop { position: 1.0; color: "#0d1726" }
                            }
                            border.width: 1
                            border.color: index === 0 ? "#00c2ff" : (index === 1 ? "#8e24aa" : (index === 2 ? "#4cd964" : "#ffb74d"))
                            opacity: 0.75 + beat * 0.25
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                Label { text: index === 0 ? "A" : (index === 1 ? "B" : (index === 2 ? "C" : "D")); color: "white"; font.pixelSize: 18; font.bold: true }
                                Item { Layout.fillWidth: true }
                                Label { text: Math.round(modelData * 100) + "%"; color: "white"; font.pixelSize: 13; font.bold: true }
                            }
                            Rectangle {
                                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                                anchors.leftMargin: 10; anchors.rightMargin: 10; anchors.bottomMargin: 8
                                height: 6; radius: 3
                                color: "#203248"
                                Rectangle {
                                    width: parent.width * modelData * (0.92 + beat * 0.08)
                                    height: parent.height
                                    radius: 3
                                    color: index === 0 ? "#00c2ff" : (index === 1 ? "#8e24aa" : (index === 2 ? "#4cd964" : "#ffb74d"))
                                }
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                    Label { text: "Smooth gradients\nShadows\nMotion"; color: "#95abc1"; font.pixelSize: 12; wrapMode: Text.WordWrap }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Rectangle {
                Layout.fillWidth: true
                height: 40; radius: 20
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#17304c" }
                    GradientStop { position: 1.0; color: "#0f1d2f" }
                }
                border.color: "#00c2ff"
                border.width: 1
                Label { anchors.centerIn: parent; text: "Qt Quick"; color: "white"; font.pixelSize: 13; font.bold: true }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 40; radius: 20
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#2a1641" }
                    GradientStop { position: 1.0; color: "#120b1d" }
                }
                border.color: "#8e24aa"
                border.width: 1
                Label { anchors.centerIn: parent; text: "Graphical Effects"; color: "white"; font.pixelSize: 13; font.bold: true }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 40; radius: 20
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#123220" }
                    GradientStop { position: 1.0; color: "#09140f" }
                }
                border.color: "#4cd964"
                border.width: 1
                Label { anchors.centerIn: parent; text: "Animated Dashboard"; color: "white"; font.pixelSize: 13; font.bold: true }
            }
        }
    }
}
