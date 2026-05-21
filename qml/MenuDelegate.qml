/*
    Configurable menu bar button delegate for QuickBar
    Based on KDE Plasma Global Menu MenuDelegate (GPL-2.0-or-later)
*/
import QtQuick
import QtQuick.Controls

import org.kde.ksvg as KSvg
import org.kde.plasma.components as PC3
import org.kde.kirigami as Kirigami

AbstractButton {
    id: controlRoot

    property bool menuIsOpen: false
    property bool hoverOpensMenu: true
    property string layoutStyle: "native"
    property int fontSize: 0
    property string fontFamily: ""
    property int fontWeight: 0
    property string textColor: ""
    property string hoverTextColor: ""
    property bool useNativeStyling: true

    signal activated()

    hoverEnabled: true
    onHoveredChanged: {
        if (hoverOpensMenu && hovered && menuIsOpen) {
            activated()
        }
    }
    onPressed: activated()

    enum State { Rest, Hover, Down }

    property int menuState: {
        if (down) {
            return MenuDelegate.State.Down
        }
        if (hovered && !menuIsOpen) {
            return MenuDelegate.State.Hover
        }
        return MenuDelegate.State.Rest
    }

    Kirigami.MnemonicData.controlType: Kirigami.MnemonicData.SecondaryControl
    Kirigami.MnemonicData.label: text

    readonly property int menuVerticalPadding: {
        if (useNativeStyling && layoutStyle === "native") {
            return Math.max(rest.margins.top, rest.margins.bottom)
        }
        return Kirigami.Units.smallSpacing
    }
    readonly property int menuHorizontalPadding: {
        if (useNativeStyling && layoutStyle === "native") {
            return Math.max(rest.margins.left, rest.margins.right)
        }
        return Kirigami.Units.smallSpacing
    }

    topPadding: menuVerticalPadding
    leftPadding: menuHorizontalPadding
    rightPadding: menuHorizontalPadding
    bottomPadding: menuVerticalPadding

    Accessible.description: i18nc("@info:usagetip", "Open a menu")

    background: Item {
        visible: useNativeStyling && layoutStyle === "native"
        KSvg.FrameSvgItem {
            id: rest
            anchors.fill: parent
            imagePath: "widgets/menubaritem"
            prefix: switch (controlRoot.menuState) {
                case MenuDelegate.State.Down: return "pressed"
                case MenuDelegate.State.Hover: return "hover"
                case MenuDelegate.State.Rest: return "normal"
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: !useNativeStyling || layoutStyle !== "native"
        radius: layoutStyle === "pill" ? height / 2 : Kirigami.Units.smallSpacing / 2
        color: {
            if (layoutStyle === "flat") {
                return "transparent"
            }
            if (controlRoot.menuState === MenuDelegate.State.Down) {
                return Kirigami.Theme.highlightColor
            }
            if (controlRoot.menuState === MenuDelegate.State.Hover) {
                return Kirigami.ColorUtils.linearInterpolation(
                    Kirigami.Theme.backgroundColor,
                    Kirigami.Theme.hoverColor,
                    0.35)
            }
            return "transparent"
        }
    }

    contentItem: PC3.Label {
        text: controlRoot.Kirigami.MnemonicData.richTextLabel
        textFormat: Text.StyledText
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        font.family: controlRoot.fontFamily || Kirigami.Theme.defaultFont.family
        font.pointSize: controlRoot.fontSize > 0 ? controlRoot.fontSize : Kirigami.Theme.defaultFont.pointSize
        font.weight: controlRoot.fontWeight > 0 ? controlRoot.fontWeight : Kirigami.Theme.defaultFont.weight
        color: {
            const useHover = controlRoot.menuState !== MenuDelegate.State.Rest
            const custom = useHover ? controlRoot.hoverTextColor : controlRoot.textColor
            if (custom && custom.length > 0) {
                return custom
            }
            return useHover ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
        }
    }
}
