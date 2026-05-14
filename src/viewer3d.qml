import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
import QtQuick3D.AssetUtils

Item {
    id: root

    property string modelSource: ""
    property bool modelLoaded: modelSource !== ""
    property real modelSize: 200

    View3D {
        id: view3D
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#1e1e2e"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.Medium
            probeExposure: 0
            specularAAEnabled: false
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 10)
            clipNear: 1.0
            clipFar: 100000.0
        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(-45, 45, 0)
            brightness: 1.0
            castsShadow: false
        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(45, -135, 0)
            brightness: 0.4
            castsShadow: false
        }

        Node {
            id: originNode
            eulerRotation: Qt.vector3d(-20, 30, 0)

            RuntimeLoader {
                id: shipLoader
                source: root.modelLoaded ? root.modelSource : ""
                onStatusChanged: {
                    if (status !== RuntimeLoader.Ready)
                        return
                    var mn = bounds.minimum
                    var mx = bounds.maximum
                    var cx = (mn.x + mx.x) * 0.5
                    var cy = (mn.y + mx.y) * 0.5
                    var cz = (mn.z + mx.z) * 0.5
                    var sizeX = mx.x - mn.x
                    var sizeY = mx.y - mn.y

                    // centre model on orbit origin
                    shipLoader.position = Qt.vector3d(-cx, -cy, -cz)
                    originNode.position = Qt.vector3d(0, 0, 0)

                    // fit camera using X and Y extents + actual viewport aspect ratio
                    var aspect = view3D.width > 0 ? view3D.width / view3D.height : 1.5
                    var halfVFov = Math.PI / 6          // 30° — half of 60° FOV
                    var distForY = (sizeY * 0.5) / Math.tan(halfVFov) * 1.15
                    var distForX = (sizeX * 0.5) / (Math.tan(halfVFov) * aspect) * 1.15
                    camera.z = Math.max(distForX, distForY, 10) / 10

                    // reset orbit angle
                    originNode.eulerRotation = Qt.vector3d(-20, 30, 0)
                    root.modelSize = Math.max(sizeX, sizeY)
                }
            }
        }

        OrbitCameraController {
            id: orbitCtrl
            anchors.fill: parent
            camera: camera
            origin: originNode
            panEnabled: true
            xSpeed: -0.1
            ySpeed: -0.1
        }
    }

    // ── pan arrow buttons ────────────────────────────────────────────
    function panStep() { return camera.z * 0.04 }

    function doPan(dx, dy) {
        var step = panStep()
        var vel = Qt.vector3d(0, 0, 0)
        if (dx !== 0) {
            var r = originNode.right
            vel = vel.plus(Qt.vector3d(r.x * dx * step, r.y * dx * step, r.z * dx * step))
        }
        if (dy !== 0) {
            var u = originNode.up
            vel = vel.plus(Qt.vector3d(u.x * dy * step, u.y * dy * step, u.z * dy * step))
        }
        originNode.position = originNode.position.plus(vel)
    }

    // repeat timer for held buttons
    Timer {
        id: panTimer
        interval: 50
        repeat: true
        property int dx: 0
        property int dy: 0
        onTriggered: root.doPan(dx, dy)
    }

    component ArrowButton: Rectangle {
        id: btn
        width: 36; height: 36
        radius: 6
        color: ma.pressed ? "#555577" : "#33334a"
        border.color: "#666688"
        border.width: 1
        property string label: ""
        property int dx: 0
        property int dy: 0
        Text {
            anchors.centerIn: parent
            text: btn.label
            color: "white"
            font.pixelSize: 16
        }
        MouseArea {
            id: ma
            anchors.fill: parent
            onPressed: {
                root.doPan(btn.dx, btn.dy)
                panTimer.dx = btn.dx
                panTimer.dy = btn.dy
                panTimer.restart()
            }
            onReleased: panTimer.stop()
        }
    }

    // compass-rose layout in bottom-right corner
    Item {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        width: 120; height: 120
        visible: root.modelLoaded

        ArrowButton { label: "▲"; dx:  0; dy:  1; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top }
        ArrowButton { label: "▼"; dx:  0; dy: -1; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom }
        ArrowButton { label: "◀"; dx: -1; dy:  0; anchors.verticalCenter: parent.verticalCenter;   anchors.left: parent.left }
        ArrowButton { label: "▶"; dx:  1; dy:  0; anchors.verticalCenter: parent.verticalCenter;   anchors.right: parent.right }
    }

    Rectangle {
        visible: !root.modelLoaded
        anchors.centerIn: parent
        color: "transparent"
        Text {
            anchors.centerIn: parent
            text: "No ship selected"
            color: "#888888"
            font.pixelSize: 18
        }
    }
}
