/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "quickbarapplet.h"
#include "appmenumodel.h"

#include <QAction>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QStyleOptionMenuItem>
#include <QTimer>
#include <QWindow>

int QuickBarApplet::s_refs = 0;
namespace
{
QString viewService()
{
    return QStringLiteral("org.kde.kappmenuview");
}

void setTransientParentIfPossible(QMenu *menu, QWindow *parentWindow)
{
    if (!menu || !parentWindow) {
        return;
    }
    if (QWindow *window = menu->windowHandle()) {
        window->setTransientParent(parentWindow);
    }
}

QString visibleMenuText(const QString &text)
{
    QString out;
    out.reserve(text.size());
    for (qsizetype i = 0; i < text.size(); ++i) {
        const QChar ch = text.at(i);
        if (ch == QLatin1Char('&')) {
            if (i + 1 < text.size() && text.at(i + 1) == QLatin1Char('&')) {
                out += QLatin1Char('&');
                ++i;
            }
            continue;
        }
        out += ch;
    }
    return out;
}

void ensureMenuSizing(QMenu *menu)
{
    if (!menu) {
        return;
    }

    for (QAction *action : menu->actions()) {
        if (QMenu *subMenu = action->menu()) {
            ensureMenuSizing(subMenu);
        }
    }

    QStyle *style = menu->style();
    int minWidth = 0;
    for (QAction *action : menu->actions()) {
        if (action->isSeparator()) {
            continue;
        }

        QStyleOptionMenuItem option;
        option.initFrom(menu);
        option.font = menu->font();
        option.fontMetrics = QFontMetrics(option.font);
        option.text = visibleMenuText(action->text());
        option.icon = action->icon();
        option.menuItemType = action->menu() ? QStyleOptionMenuItem::SubMenu : QStyleOptionMenuItem::Normal;
        option.state = action->isEnabled() ? QStyle::State_Enabled : QStyle::State_None;
        option.rect = menu->rect();

        const QSize itemSize = style->sizeFromContents(QStyle::CT_MenuItem, &option, QSize(), menu);
        minWidth = qMax(minWidth, itemSize.width());
    }

    if (minWidth > 0) {
        menu->setMinimumWidth(qMax(menu->minimumWidth(), minWidth));
    }
    menu->adjustSize();
}

void cloneMenuStructure(QMenu *sourceMenu, QMenu *destMenu)
{
    destMenu->clear();

    for (QAction *sourceAction : sourceMenu->actions()) {
        if (sourceAction->isSeparator()) {
            destMenu->addSeparator();
            continue;
        }

        const QString label = visibleMenuText(sourceAction->text());

        if (QMenu *sourceSubMenu = sourceAction->menu()) {
            QMenu *destSubMenu = destMenu->addMenu(label);
            cloneMenuStructure(sourceSubMenu, destSubMenu);
            continue;
        }

        QAction *clone = destMenu->addAction(label);
        clone->setEnabled(sourceAction->isEnabled());
        clone->setShortcut(sourceAction->shortcut());
        QObject::connect(clone, &QAction::triggered, sourceAction, &QAction::trigger);
    }

    ensureMenuSizing(destMenu);
}
} // namespace

QuickBarApplet::QuickBarApplet(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : Plasma::Applet(parent, data, args)
{
    ++s_refs;
    // if we're the first, register the service
    if (s_refs == 1) {
        QDBusConnection::sessionBus().interface()->registerService(viewService(),
                                                                   QDBusConnectionInterface::QueueService,
                                                                   QDBusConnectionInterface::DontAllowReplacement);
    }
    /*it registers or unregisters the service when the destroyed value of the applet change,
      and not in the dtor, because:
      when we "delete" an applet, it just hides it for about a minute setting its status
      to destroyed, in order to be able to do a clean undo: if we undo, there will be
      another destroyedchanged and destroyed will be false.
      When this happens, if we are the only appmenu applet existing, the dbus interface
      will have to be registered again*/
    connect(this, &Applet::destroyedChanged, this, [](bool destroyed) {
        if (destroyed) {
            // if we were the last, unregister
            if (--s_refs == 0) {
                QDBusConnection::sessionBus().interface()->unregisterService(viewService());
            }
        } else {
            // if we're the first, register the service
            if (++s_refs == 1) {
                QDBusConnection::sessionBus().interface()->registerService(viewService(),
                                                                           QDBusConnectionInterface::QueueService,
                                                                           QDBusConnectionInterface::DontAllowReplacement);
            }
        }
    });
}

QuickBarApplet::~QuickBarApplet() = default;

void QuickBarApplet::init()
{
}

QAbstractItemModel *QuickBarApplet::model() const
{
    return m_model;
}

void QuickBarApplet::setModel(QAbstractItemModel *model)
{
    if (m_model != model) {
        m_model = model;
        Q_EMIT modelChanged();
    }
}

int QuickBarApplet::view() const
{
    return m_viewType;
}

void QuickBarApplet::setView(int type)
{
    if (m_viewType != type) {
        m_viewType = type;
        Q_EMIT viewChanged();
    }
}

int QuickBarApplet::currentIndex() const
{
    return m_currentIndex;
}

void QuickBarApplet::setCurrentIndex(int currentIndex)
{
    if (m_currentIndex != currentIndex) {
        m_currentIndex = currentIndex;
        Q_EMIT currentIndexChanged();
    }
}

QQuickItem *QuickBarApplet::buttonGrid() const
{
    return m_buttonGrid;
}

void QuickBarApplet::setButtonGrid(QQuickItem *buttonGrid)
{
    if (m_buttonGrid != buttonGrid) {
        m_buttonGrid = buttonGrid;
        Q_EMIT buttonGridChanged();
    }
}

QMenu *QuickBarApplet::createMenu(int idx) const
{
    QMenu *menu = nullptr;

    if (!m_model) {
        return nullptr;
    }

    if (view() == CompactView) {
        if (auto *menuAction = m_model->data(QModelIndex(), AppMenuModel::ActionRole).value<QAction *>()) {
            menu = menuAction->menu();
        }
    } else if (view() == FullView) {
        const QModelIndex index = m_model->index(idx, 0);
        if (auto *action = m_model->data(index, AppMenuModel::ActionRole).value<QAction *>()) {
            menu = action->menu();
        }
    }

    return menu;
}

void QuickBarApplet::onMenuAboutToHide()
{
    if (m_pendingMenuSwitch) {
        return;
    }

    setCurrentIndex(-1);
}

Qt::Edges edgeFromLocation(Plasma::Types::Location location)
{
    switch (location) {
    case Plasma::Types::TopEdge:
        return Qt::TopEdge;
    case Plasma::Types::BottomEdge:
        return Qt::BottomEdge;
    case Plasma::Types::LeftEdge:
        return Qt::LeftEdge;
    case Plasma::Types::RightEdge:
        return Qt::RightEdge;
    case Plasma::Types::Floating:
    case Plasma::Types::Desktop:
    case Plasma::Types::FullScreen:
        break;
    }
    return {};
}

void QuickBarApplet::trigger(QQuickItem *ctx, int idx)
{
    if (m_currentIndex == idx) {
        return;
    }

    const bool switchingMenu = m_currentIndex >= 0;

    if (!ctx || !ctx->window() || !ctx->window()->screen()) {
        if (switchingMenu) {
            m_pendingMenuSwitch = false;
        }
        return;
    }
    if (switchingMenu) {
        m_pendingMenuSwitch = true;
    }

    QMenu *actionMenu = createMenu(idx);
    if (actionMenu) {
        // this is a workaround where Qt will fail to realize a mouse has been released
        // this happens if a window which does not accept focus spawns a new window that takes focus and X grab
        // whilst the mouse is depressed
        // https://bugreports.qt.io/browse/QTBUG-59044
        // this causes the next click to go missing

        // by releasing manually we avoid that situation
        auto ungrabMouseHack = [ctx]() {
            if (ctx && ctx->window() && ctx->window()->mouseGrabberItem()) {
                // FIXME event forge thing enters press and hold move mode :/
                ctx->window()->mouseGrabberItem()->ungrabMouse();
            }
        };

        const auto &geo = ctx->window()->screen()->availableVirtualGeometry();
        QPoint pos = ctx->window()->mapToGlobal(ctx->mapToScene(QPointF()).toPoint());
        const Qt::Edges edges = edgeFromLocation(location());
        if (location() == Plasma::Types::TopEdge) {
            pos.setY(pos.y() + ctx->height());
        }

        // Always clone into a proxy menu. Moving actions out of DBus-imported menus
        // (e.g. Firefox) corrupts the app's menu and can crash the application.
        if (!m_proxyMenu) {
            m_proxyMenu = std::make_unique<QMenu>();
            connect(m_proxyMenu.get(), &QMenu::aboutToHide, this, &QuickBarApplet::onMenuAboutToHide, Qt::UniqueConnection);
        } else if (m_currentMenu && m_currentMenu->isVisible() && !switchingMenu) {
            m_currentMenu->hide();
        }

        cloneMenuStructure(actionMenu, m_proxyMenu.get());
        m_currentMenu = m_proxyMenu.get();
        m_sourceMenu = actionMenu;

        QTimer::singleShot(0, ctx, ungrabMouseHack);
        // end workaround

        m_currentMenu->setProperty("_breeze_menu_seamless_edges", QVariant::fromValue(edges));
        m_currentMenu->adjustSize();

        pos = QPoint(qBound(geo.x(), pos.x(), geo.x() + geo.width() - m_currentMenu->width()),
                     qBound(geo.y(), pos.y(), geo.y() + geo.height() - m_currentMenu->height()));

        if (view() == FullView) {
            m_currentMenu->installEventFilter(this);
            // Reposition with popup when switching: move() leaves the menu under the
            // pointer while the cursor is still on the menubar, so Qt closes it.
            if (!m_currentMenu->isVisible() || switchingMenu) {
                m_currentMenu->popup(pos);
            } else {
                m_currentMenu->move(pos);
            }
            setTransientParentIfPossible(m_currentMenu, ctx->window());
        } else if (view() == CompactView) {
            if (m_currentMenu->isEmpty()) {
                // don't try to popup an empty menu in case the app gives us one
                if (switchingMenu) {
                    m_pendingMenuSwitch = false;
                }
                return;
            }
            m_currentMenu->popup(pos);
            setTransientParentIfPossible(m_currentMenu, ctx->window());
        }

        setCurrentIndex(idx);

        if (switchingMenu) {
            QTimer::singleShot(0, this, [this]() {
                m_pendingMenuSwitch = false;
            });
        }

        // FIXME TODO connect only once
    } else if (m_model) { // is it just an action without a menu?
        if (switchingMenu) {
            m_pendingMenuSwitch = false;
        }
        if (auto *action = m_model->index(idx, 0).data(AppMenuModel::ActionRole).value<QAction *>()) {
            Q_ASSERT(!action->menu());
            action->trigger();
        }
    } else if (switchingMenu) {
        m_pendingMenuSwitch = false;
    }
}

// FIXME TODO doesn't work on submenu
bool QuickBarApplet::eventFilter(QObject *watched, QEvent *event)
{
    auto *menu = qobject_cast<QMenu *>(watched);
    if (!menu) {
        return false;
    }

    if (event->type() == QEvent::KeyPress) {
        auto *e = static_cast<QKeyEvent *>(event);

        // TODO right to left languages
        if (e->key() == Qt::Key_Left) {
            int desiredIndex = m_currentIndex - 1;
            Q_EMIT requestActivateIndex(desiredIndex);
            return true;
        } else if (e->key() == Qt::Key_Right) {
            if (menu->activeAction() && menu->activeAction()->menu()) {
                return false;
            }

            int desiredIndex = m_currentIndex + 1;
            Q_EMIT requestActivateIndex(desiredIndex);
            return true;
        }

    } else if (event->type() == QEvent::MouseMove) {
        auto *e = static_cast<QMouseEvent *>(event);

        if (!m_buttonGrid || !m_buttonGrid->window()) {
            return false;
        }

        // FIXME the panel margin breaks Fitt's law :(
        const QPointF &windowLocalPos = m_buttonGrid->window()->mapFromGlobal(e->globalPosition());
        const QPointF &buttonGridLocalPos = m_buttonGrid->mapFromScene(windowLocalPos);
        auto *item = m_buttonGrid->childAt(buttonGridLocalPos.x(), buttonGridLocalPos.y());
        if (!item) {
            return false;
        }

        bool ok;
        const int buttonIndex = item->property("buttonIndex").toInt(&ok);
        if (!ok) {
            return false;
        }

        Q_EMIT requestActivateIndex(buttonIndex);
    }

    return false;
}

K_PLUGIN_CLASS_WITH_JSON(QuickBarApplet, "metadata.json")

#include "quickbarapplet.moc"
#include "moc_quickbarapplet.cpp"
