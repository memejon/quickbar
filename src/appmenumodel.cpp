/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2016 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "appmenumodel.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusServiceWatcher>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMenu>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

// Includes for the menu search.
#include <KLocalizedString>
#include <QLineEdit>
#include <QListView>
#include <QWidgetAction>
#include <algorithm>

#include <abstracttasksmodel.h>
#include <dbusmenuimporter.h>

class KDBusMenuImporter : public DBusMenuImporter
{
public:
    KDBusMenuImporter(const QString &service, const QString &path)
        : DBusMenuImporter(service, path)
    {
    }

protected:
    QIcon iconForName(const QString &name) override
    {
        return QIcon::fromTheme(name);
    }
};

AppMenuModel::AppMenuModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_tasksModel(new TaskManager::TasksModel(this))
    , m_serviceWatcher(new QDBusServiceWatcher(this))
{
    m_tasksModel->setFilterByScreen(!m_allScreens);
    connect(m_tasksModel, &TaskManager::TasksModel::activeTaskChanged, this, &AppMenuModel::onActiveWindowChanged);
    connect(m_tasksModel,
            &TaskManager::TasksModel::dataChanged,
            [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>()) {
                Q_UNUSED(topLeft)
                Q_UNUSED(bottomRight)
                if (roles.contains(TaskManager::AbstractTasksModel::ApplicationMenuObjectPath)
                    || roles.contains(TaskManager::AbstractTasksModel::ApplicationMenuServiceName) || roles.isEmpty()) {
                    onActiveWindowChanged();
                }
            });
    connect(m_tasksModel, &TaskManager::TasksModel::activityChanged, this, &AppMenuModel::onActiveWindowChanged);
    connect(m_tasksModel, &TaskManager::TasksModel::virtualDesktopChanged, this, &AppMenuModel::onActiveWindowChanged);
    connect(m_tasksModel, &TaskManager::TasksModel::countChanged, this, &AppMenuModel::onActiveWindowChanged);
    connect(m_tasksModel, &TaskManager::TasksModel::screenGeometryChanged, this, &AppMenuModel::screenGeometryChanged);

    connect(this, &AppMenuModel::modelNeedsUpdate, this, [this] {
        if (!m_updatePending) {
            m_updatePending = true;
            QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
        }
    });

    onActiveWindowChanged();

    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    // if our current DBus connection gets lost, close the menu
    // we'll select the new menu when the focus changes
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &serviceName) {
        if (serviceName == m_serviceName) {
            setMenuAvailable(false);
            Q_EMIT modelNeedsUpdate();
        }
    });

    // X11 has funky menu behaviour that prevents this from working properly.
    if (KWindowSystem::isPlatformWayland()) {
        m_searchAction = new QAction(this);
        m_searchAction->setText(i18n("Search"));
        m_searchAction->setObjectName(QStringLiteral("appmenu"));

        m_searchMenu.reset(new QMenu);
        auto searchAction = new QWidgetAction(this);
        auto searchBar = new QLineEdit;
        searchBar->setClearButtonEnabled(true);
        searchBar->setPlaceholderText(i18n("Search…"));
        searchBar->setMinimumWidth(200);
        searchBar->setContentsMargins(4, 4, 4, 4);
        connect(m_tasksModel, &TaskManager::TasksModel::activeTaskChanged, searchBar, [searchBar]() {
            searchBar->setText(QString());
        });
        connect(searchBar, &QLineEdit::textChanged, this, [searchBar, this]() mutable {
            insertSearchActionsIntoMenu(searchBar->text());
        });
        connect(searchBar, &QLineEdit::returnPressed, this, [this]() mutable {
            if (!m_currentSearchActions.empty()) {
                m_currentSearchActions.constFirst()->trigger();
            }
        });
        connect(this, &AppMenuModel::modelNeedsUpdate, searchBar, [this, searchBar]() mutable {
            insertSearchActionsIntoMenu(searchBar->text());
        });
        searchAction->setDefaultWidget(searchBar);
        m_searchMenu->addAction(searchAction);
        m_searchMenu->addSeparator();
        m_searchAction->setMenu(m_searchMenu.get());
    }
}

AppMenuModel::~AppMenuModel() = default;

bool AppMenuModel::menuAvailable() const
{
    return m_menuAvailable;
}

void AppMenuModel::setMenuAvailable(bool set)
{
    if (m_menuAvailable != set) {
        m_menuAvailable = set;
        if (set) {
            setVisible(true);
        } else {
            setVisible(false);
        }
        Q_EMIT menuAvailableChanged();
        Q_EMIT menuForDisplayChanged();
    }
}

bool AppMenuModel::allScreens() const
{
    return m_allScreens;
}

void AppMenuModel::setallScreens(bool allScreens)
{
    if (m_allScreens == allScreens) {
        return;
    }

    m_allScreens = allScreens;
    m_tasksModel->setFilterByScreen(!m_allScreens);
    Q_EMIT allScreensChanged();
}

bool AppMenuModel::visible() const
{
    return m_visible;
}

bool AppMenuModel::menuForDisplay() const
{
    return m_menuAvailable;
}

bool AppMenuModel::stickyMenuBar() const
{
    return m_stickyMenuBar;
}

void AppMenuModel::setStickyMenuBar(bool sticky)
{
    if (m_stickyMenuBar == sticky) {
        return;
    }

    m_stickyMenuBar = sticky;
    Q_EMIT stickyMenuBarChanged();

    if (!sticky) {
        onActiveWindowChanged();
    } else {
        Q_EMIT menuForDisplayChanged();
    }
}

QString AppMenuModel::applicationName() const
{
    return m_applicationName;
}

void AppMenuModel::setApplicationName(const QString &name)
{
    if (m_applicationName == name) {
        return;
    }

    m_applicationName = name;
    Q_EMIT applicationNameChanged();
}

void AppMenuModel::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        Q_EMIT visibleChanged();
        Q_EMIT menuForDisplayChanged();
    }
}

QRect AppMenuModel::screenGeometry() const
{
    return m_tasksModel->screenGeometry();
}

void AppMenuModel::setScreenGeometry(QRect geometry)
{
    m_tasksModel->setScreenGeometry(geometry);
}

int AppMenuModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (!m_menuAvailable || !m_menu) {
        return 0;
    }

    return m_menu->actions().count() + (m_searchAction ? 1 : 0);
}

void AppMenuModel::removeSearchActionsFromMenu()
{
    for (auto action : std::as_const(m_currentSearchActions)) {
        m_searchAction->menu()->removeAction(action);
    }
    m_currentSearchActions = QList<QAction *>();
}

void AppMenuModel::insertSearchActionsIntoMenu(const QString &filter)
{
    removeSearchActionsFromMenu();
    if (filter.isEmpty()) {
        return;
    }
    const auto actions = flatActionList();
    for (const auto &action : actions) {
        if (action->text().contains(filter, Qt::CaseInsensitive)) {
            m_searchAction->menu()->addAction(action);
            m_currentSearchActions << action;
        }
    }
}

void AppMenuModel::update()
{
    beginResetModel();
    endResetModel();
    m_updatePending = false;
}

bool AppMenuModel::isDolphinTask(const QModelIndex &taskIndex, TaskManager::TasksModel *tasksModel)
{
    if (!taskIndex.isValid() || !tasksModel) {
        return false;
    }

    const QString appId = tasksModel->data(taskIndex, TaskManager::AbstractTasksModel::AppId).toString();
    if (appId.contains(QLatin1String("dolphin"), Qt::CaseInsensitive)) {
        return true;
    }

    const QUrl launcherUrl = tasksModel->data(taskIndex, TaskManager::AbstractTasksModel::LauncherUrl).toUrl();
    return launcherUrl.isValid() && launcherUrl.path().contains(QLatin1String("org.kde.dolphin"));
}

int AppMenuModel::dolphinDesktopMenuMatchScore(const QModelIndex &taskIndex, TaskManager::TasksModel *tasksModel)
{
    if (!isDolphinTask(taskIndex, tasksModel)) {
        return -1;
    }

    const QString serviceName = tasksModel->data(taskIndex, TaskManager::AbstractTasksModel::ApplicationMenuServiceName).toString();
    const QString objectPath = tasksModel->data(taskIndex, TaskManager::AbstractTasksModel::ApplicationMenuObjectPath).toString();
    if (serviceName.isEmpty() || objectPath.isEmpty()) {
        return -1;
    }

    const QString windowTitle = tasksModel->data(taskIndex, Qt::DisplayRole).toString();
    const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopPath.isEmpty()) {
        return 1;
    }

    const QFileInfo desktopInfo(desktopPath);
    const QString desktopFolderName = desktopInfo.fileName();
    if (!desktopFolderName.isEmpty()) {
        if (windowTitle.compare(desktopFolderName, Qt::CaseInsensitive) == 0) {
            return 100;
        }
        if (windowTitle.contains(desktopFolderName, Qt::CaseInsensitive)) {
            return 50;
        }
    }

    if (windowTitle.contains(desktopInfo.absoluteFilePath(), Qt::CaseInsensitive)) {
        return 50;
    }

    // Common fallback when the folder is literally named "Desktop"
    if (windowTitle.contains(QLatin1String("Desktop"), Qt::CaseInsensitive)) {
        return 25;
    }

    return 1;
}

QModelIndex AppMenuModel::findDolphinDesktopMenuTask() const
{
    QModelIndex bestMatch;
    int bestScore = -1;

    const auto consider = [&](const QModelIndex &taskIndex) {
        const int score = dolphinDesktopMenuMatchScore(taskIndex, m_tasksModel);
        if (score > bestScore) {
            bestScore = score;
            bestMatch = taskIndex;
        }
    };

    for (int i = 0; i < m_tasksModel->rowCount(); ++i) {
        const QModelIndex idx = m_tasksModel->index(i, 0);
        consider(idx);

        for (int j = 0; j < m_tasksModel->rowCount(idx); ++j) {
            consider(m_tasksModel->index(j, 0, idx));
        }
    }

    return bestMatch;
}

void AppMenuModel::setDesktopProxyMenu(const QString &serviceName, const QString &objectPath)
{
    m_desktopProxyMenuService = serviceName;
    m_desktopProxyMenuObjectPath = objectPath;
}

void AppMenuModel::clearDesktopProxyMenu()
{
    m_desktopProxyMenuService.clear();
    m_desktopProxyMenuObjectPath.clear();
}

void AppMenuModel::applyDesktopMenu()
{
    const auto useProxyMenu = [this](const QString &serviceName, const QString &objectPath) {
        if (serviceName.isEmpty() || objectPath.isEmpty()) {
            return false;
        }
        setDesktopProxyMenu(serviceName, objectPath);
        updateApplicationMenu(serviceName, objectPath);
        return true;
    };

    // Reuse the last known Desktop Dolphin menu; TasksModel often clears menu paths when unfocused.
    if (useProxyMenu(m_desktopProxyMenuService, m_desktopProxyMenuObjectPath)) {
        return;
    }

    const QModelIndex dolphinTask = findDolphinDesktopMenuTask();
    if (dolphinTask.isValid()) {
        const QString objectPath = m_tasksModel->data(dolphinTask, TaskManager::AbstractTasksModel::ApplicationMenuObjectPath).toString();
        const QString serviceName = m_tasksModel->data(dolphinTask, TaskManager::AbstractTasksModel::ApplicationMenuServiceName).toString();
        if (useProxyMenu(serviceName, objectPath)) {
            return;
        }
    }

    ensureDolphinAtDesktop();
    if (!m_menuAvailable) {
        updateApplicationMenu(QString(), QString());
    }
}

void AppMenuModel::ensureDolphinAtDesktop()
{
    if (m_dolphinDesktopLaunchPending) {
        return;
    }

    const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopPath.isEmpty()) {
        return;
    }

    const QString desktopUrl = QUrl::fromLocalFile(desktopPath).toString();
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.FileManager1"),
                                                          QStringLiteral("/org/freedesktop/FileManager1"),
                                                          QStringLiteral("org.freedesktop.FileManager1"),
                                                          QStringLiteral("ShowFolders"));
    message << QStringList{desktopUrl} << QString();

    m_dolphinDesktopLaunchPending = true;
    QDBusConnection::sessionBus().asyncCall(message);
    QTimer::singleShot(5000, this, [this] {
        m_dolphinDesktopLaunchPending = false;
    });
}

void AppMenuModel::onActiveWindowChanged()
{
    // Do not change active window when panel gets focus
    // See ShellCorona::init() in shell/shellcorona.cpp
    if (m_containmentStatus == Plasma::Types::AcceptingInputStatus) {
        return;
    }

    const QModelIndex activeTaskIndex = m_tasksModel->activeTask();

    // Always use the desktop menu (Dolphin ~/Desktop); only the label tracks focus.
    if (activeTaskIndex.isValid()) {
        setApplicationName(m_tasksModel->data(activeTaskIndex, TaskManager::AbstractTasksModel::AppName).toString());
    } else {
        setApplicationName(i18n("Plasma"));
    }

    applyDesktopMenu();
}

QHash<int, QByteArray> AppMenuModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[MenuRole] = QByteArrayLiteral("activeMenu");
    roleNames[ActionRole] = QByteArrayLiteral("activeActions");
    return roleNames;
}

QList<QAction *> AppMenuModel::flatActionList()
{
    QList<QAction *> ret;
    if (!m_menuAvailable || !m_menu) {
        return ret;
    }
    const auto actions = m_menu->findChildren<QAction *>();
    for (auto &action : actions) {
        if (action->menu() == nullptr) {
            ret << action;
        }
    }
    return ret;
}

QVariant AppMenuModel::data(const QModelIndex &index, int role) const
{
    if (!m_menuAvailable || !m_menu) {
        return {};
    }

    if (!index.isValid()) {
        if (role == MenuRole) {
            return QString();
        } else if (role == ActionRole) {
            return QVariant::fromValue(m_menu->menuAction());
        }
    }

    const auto actions = m_menu->actions();
    const int row = index.row();
    if (row == actions.count() && m_searchAction) {
        if (role == MenuRole) {
            return m_searchAction->text();
        } else if (role == ActionRole) {
            return QVariant::fromValue(m_searchAction.data());
        }
    }
    if (row >= actions.count()) {
        return {};
    }

    if (role == MenuRole) { // TODO this should be Qt::DisplayRole
        return actions.at(row)->text();
    } else if (role == ActionRole) {
        return QVariant::fromValue(actions.at(row));
    }

    return {};
}

void AppMenuModel::updateApplicationMenu(const QString &serviceName, const QString &menuObjectPath)
{
    if (m_serviceName == serviceName && m_menuObjectPath == menuObjectPath) {
        if (m_importer) {
            QMetaObject::invokeMethod(m_importer.get(), "updateMenu", Qt::QueuedConnection);
        }
        return;
    }

    if (serviceName.isEmpty() || menuObjectPath.isEmpty()) {
        if (m_menuAvailable) {
            return;
        }

        setMenuAvailable(false);
        setVisible(false);

        m_serviceName = QString();
        m_menuObjectPath = QString();
        m_serviceWatcher->setWatchedServices({});
        m_importer.reset();
    } else {
        m_serviceName = serviceName;
        m_menuObjectPath = menuObjectPath;
        m_serviceWatcher->setWatchedServices(QStringList({m_serviceName}));

        m_importer = std::make_unique<KDBusMenuImporter>(serviceName, menuObjectPath);
        QMetaObject::invokeMethod(m_importer.get(), "updateMenu", Qt::QueuedConnection);

        connect(m_importer.get(), &DBusMenuImporter::menuUpdated, this, [=, this](QMenu *menu) {
            m_menu = m_importer->menu();
            if (m_menu.isNull() || menu != m_menu) {
                return;
            }

            if (m_menu->isEmpty()) {
                // naughty apps may pretend to have a menu but then don't actually give us one.
                // let's disable it in that case
                if (m_menuAvailable) {
                    return;
                }
                setMenuAvailable(false);
                return;
            } else {
                // if it somehow comes later, make it available again
                setMenuAvailable(true);
            }

            // cache first layer of sub menus, which we'll be popping up
            const auto actions = m_menu->actions();
            for (QAction *a : actions) {
                // signal dataChanged when the action changes
                connect(a, &QAction::changed, this, [this, a] {
                    if (m_menuAvailable && m_menu) {
                        const int actionIdx = m_menu->actions().indexOf(a);
                        if (actionIdx > -1) {
                            const QModelIndex modelIdx = index(actionIdx, 0);
                            Q_EMIT dataChanged(modelIdx, modelIdx);
                        }
                    }
                });

                connect(a, &QAction::destroyed, this, &AppMenuModel::modelNeedsUpdate);

                if (a->menu()) {
                    m_importer->updateMenu(a->menu());
                }
            }

            setMenuAvailable(true);
            Q_EMIT modelNeedsUpdate();
        });

        connect(m_importer.get(), &DBusMenuImporter::actionActivationRequested, this, [this](QAction *action) {
            // TODO submenus
            if (!m_menuAvailable || !m_menu) {
                return;
            }

            const auto actions = m_menu->actions();
            auto it = std::ranges::find(actions, action);
            if (it != actions.end()) {
                Q_EMIT requestActivateIndex(it - actions.begin());
            }
        });

        setMenuAvailable(true);
        setVisible(true);

        Q_EMIT modelNeedsUpdate();
    }
}

#include "moc_appmenumodel.cpp"
