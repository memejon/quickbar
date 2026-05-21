import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: appearancePage

    property alias cfg_layoutStyle: layoutStyleCombo.currentValue
    property alias cfg_itemSpacing: itemSpacingSpin.value
    property alias cfg_fontSize: menuFontSettings.pointSize
    property alias cfg_fontFamily: menuFontSettings.family
    property alias cfg_fontWeight: menuFontSettings.weight
    property alias cfg_textColor: textColorField.text
    property alias cfg_hoverTextColor: hoverTextColorField.text
    property alias cfg_useNativeStyling: useNativeStylingCheck.checked
    property alias cfg_maxVisibleItems: maxVisibleItemsSpin.value
    property alias cfg_appNameFontSize: appNameFontSettings.pointSize
    property alias cfg_appNameFontFamily: appNameFontSettings.family
    property alias cfg_appNameFontWeight: appNameFontSettings.weight

    QtObject {
        id: menuFontSettings
        property string family: ""
        property int pointSize: 0
        property int weight: 0
    }

    QtObject {
        id: appNameFontSettings
        property string family: ""
        property int pointSize: 0
        property int weight: 700
    }

    function weightLabel(weight) {
        if (weight <= 0) {
            return i18n("default weight")
        }
        if (weight <= Font.Thin) {
            return i18n("thin")
        }
        if (weight <= Font.ExtraLight) {
            return i18n("extra light")
        }
        if (weight <= Font.Light) {
            return i18n("light")
        }
        if (weight <= Font.Normal) {
            return i18n("regular")
        }
        if (weight <= Font.Medium) {
            return i18n("medium")
        }
        if (weight <= Font.DemiBold) {
            return i18n("semi bold")
        }
        if (weight <= Font.Bold) {
            return i18n("bold")
        }
        if (weight <= Font.ExtraBold) {
            return i18n("extra bold")
        }
        return i18n("black")
    }

    function fontSummary(settings) {
        const family = settings.family || i18n("System default")
        const size = settings.pointSize > 0
            ? i18nc("@label font size in points", "%1 pt", settings.pointSize)
            : i18n("default size")
        const style = appearancePage.weightLabel(settings.weight)
        return i18nc("@info selected font, %1 family, %2 size, %3 style", "%1, %2, %3", family, size, style)
    }

    function configFont(settings) {
        const themeFont = Kirigami.Theme.defaultFont
        return Qt.font({
            family: settings.family || themeFont.family,
            pointSize: settings.pointSize > 0 ? settings.pointSize : themeFont.pointSize,
            weight: settings.weight > 0 ? settings.weight : themeFont.weight,
        })
    }

    function applyFontFromDialog(settings, chosen) {
        settings.family = chosen.family
        settings.pointSize = chosen.pointSize > 0 ? Math.round(chosen.pointSize) : 0
        settings.weight = chosen.weight > 0 ? chosen.weight : 0
    }

    function openFontDialog(dialog, settings) {
        dialog.currentFont = configFont(settings)
        dialog.open()
    }

    function resetFont(settings, defaultWeight) {
        settings.family = ""
        settings.pointSize = 0
        settings.weight = defaultWeight
    }

    readonly property string menuFontSummary: fontSummary(menuFontSettings)
    readonly property string appNameFontSummary: fontSummary(appNameFontSettings)

    Kirigami.FormLayout {
        ComboBox {
            id: layoutStyleCombo
            Kirigami.FormData.label: i18n("Style:")
            textRole: "text"
            valueRole: "value"
            model: [
                { value: "native", text: i18n("Native Plasma menubar") },
                { value: "flat", text: i18n("Flat (no background)") },
                { value: "pill", text: i18n("Pill buttons") },
            ]
        }
        CheckBox {
            id: useNativeStylingCheck
            text: i18n("Use Plasma SVG menubar item theme")
        }
        SpinBox {
            id: itemSpacingSpin
            Kirigami.FormData.label: i18n("Spacing:")
            from: 0
            to: 32
        }
        RowLayout {
            Kirigami.FormData.label: i18n("Font:")
            spacing: Kirigami.Units.smallSpacing

            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: appearancePage.menuFontSummary
            }

            Button {
                text: i18nc("@action:button", "Choose Font…")
                onClicked: appearancePage.openFontDialog(menuFontDialog, menuFontSettings)
            }

            ToolButton {
                icon.name: "edit-clear"
                display: AbstractButton.IconOnly
                ToolTip.text: i18nc("@action:button", "Use system default font")
                onClicked: appearancePage.resetFont(menuFontSettings, 0)
            }
        }
        Item {
            Kirigami.FormData.isSection: true
        }
        RowLayout {
            Kirigami.FormData.label: i18n("Application name font:")
            spacing: Kirigami.Units.smallSpacing

            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: appearancePage.appNameFontSummary
            }

            Button {
                text: i18nc("@action:button", "Choose Font…")
                onClicked: appearancePage.openFontDialog(appNameFontDialog, appNameFontSettings)
            }

            ToolButton {
                icon.name: "edit-clear"
                display: AbstractButton.IconOnly
                ToolTip.text: i18nc("@action:button", "Use system default font")
                onClicked: appearancePage.resetFont(appNameFontSettings, 700)
            }
        }
        Item {
            Kirigami.FormData.isSection: true
        }
        TextField {
            id: textColorField
            Kirigami.FormData.label: i18n("Text color:")
            placeholderText: "#RRGGBB or empty"
        }
        TextField {
            id: hoverTextColorField
            Kirigami.FormData.label: i18n("Hover text:")
            placeholderText: "#RRGGBB or empty"
        }
        SpinBox {
            id: maxVisibleItemsSpin
            Kirigami.FormData.label: i18n("Max visible items (0 = no limit):")
            from: 0
            to: 50
        }
    }

    // Native dialog (KDE-styled on Plasma); avoids the incomplete Qt Quick Dialogs QML UI.
    Platform.FontDialog {
        id: menuFontDialog

        title: i18nc("@title:window", "Choose Font")
        modality: Qt.WindowModal
        parentWindow: appearancePage.Window.window

        onAccepted: appearancePage.applyFontFromDialog(menuFontSettings, font)
    }

    Platform.FontDialog {
        id: appNameFontDialog

        title: i18nc("@title:window", "Choose Application Name Font")
        modality: Qt.WindowModal
        parentWindow: appearancePage.Window.window

        onAccepted: appearancePage.applyFontFromDialog(appNameFontSettings, font)
    }
}
