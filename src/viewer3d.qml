import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root

    property string modelSource: ""
    property bool modelLoaded: modelSource !== ""

    View3D {
        id: view3D
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#1e1e2e"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.Medium
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 150, 800)
            clipNear: 1.0
            clipFar: 50000.0
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
            Model {
                id: shipModel
                visible: root.modelLoaded
                source: root.modelSource
                scale: Qt.vector3d(1, 1, 1)
                eulerRotation: Qt.vector3d(0, 0, 0)
            }
        }

        OrbitCameraController {
            anchors.fill: parent
            camera: camera
            origin: originNode
            panEnabled: true
        }
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
