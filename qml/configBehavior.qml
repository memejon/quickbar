import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    property alias cfg_filterByActive: filterByActiveCheck.checked
    property alias cfg_stickyMenuBar: stickyMenuBarCheck.checked
    property alias cfg_hoverOpensMenu: hoverOpensMenuCheck.checked
    property alias cfg_enableMenuSearch: enableMenuSearchCheck.checked

    Kirigami.FormLayout {
        CheckBox {
            id: stickyMenuBarCheck
            Kirigami.FormData.label: i18n("Menu bar:")
            text: i18n("Keep the bar visible when “hide when inactive” would hide it (menu items always use the desktop menu)")
        }
        CheckBox {
            id: filterByActiveCheck
            text: i18n("Hide when the menu’s window is not active (unless sticky menu is on)")
        }
        CheckBox {
            id: hoverOpensMenuCheck
            text: i18n("Open menus on hover while a menu is already open")
        }
        CheckBox {
            id: enableMenuSearchCheck
            text: i18n("Enable menu search on Wayland (system feature)")
            enabled: false
            ToolTip.visible: hovered
            ToolTip.text: i18n("Controlled by Plasma; always enabled on Wayland when supported.")
        }
    }
}
