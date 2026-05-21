import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    property alias cfg_compactView: compactViewRadio.checked
    property alias cfg_allScreens: allScreensCheck.checked
    property alias cfg_fillWidth: fillWidthCheck.checked
    property alias cfg_hideWhenEmpty: hideWhenEmptyCheck.checked
    property alias cfg_showApplicationName: showApplicationNameCheck.checked
    property alias cfg_appNameMarginBefore: appNameMarginBeforeSpin.value
    property alias cfg_appNameMarginAfter: appNameMarginAfterSpin.value

    Kirigami.FormLayout {
        RadioButton {
            id: fullViewRadio
            Kirigami.FormData.label: i18n("Layout:")
            text: i18n("Full menubar (File, Edit, …)")
            checked: !compactViewRadio.checked
            onToggled: if (checked) compactViewRadio.checked = false
        }
        RadioButton {
            id: compactViewRadio
            text: i18n("Single application-menu button")
            onToggled: if (checked) fullViewRadio.checked = false
        }
        CheckBox {
            id: allScreensCheck
            Kirigami.FormData.label: i18n("Screens:")
            text: i18n("Show menus for apps on all screens")
        }
        CheckBox {
            id: fillWidthCheck
            Kirigami.FormData.label: i18n("Panel:")
            text: i18n("Fill remaining panel width")
        }
        CheckBox {
            id: hideWhenEmptyCheck
            text: i18n("Hide when no application menu is available")
        }
        CheckBox {
            id: showApplicationNameCheck
            Kirigami.FormData.label: i18n("Application:")
            text: i18n("Show application name before the menu")
        }
        SpinBox {
            id: appNameMarginBeforeSpin
            Kirigami.FormData.label: i18n("Margin before name:")
            from: 0
            to: 64
            enabled: showApplicationNameCheck.checked
        }
        SpinBox {
            id: appNameMarginAfterSpin
            Kirigami.FormData.label: i18n("Margin after name:")
            from: 0
            to: 64
            enabled: showApplicationNameCheck.checked
        }
    }
}
