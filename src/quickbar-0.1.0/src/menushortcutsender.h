/*
    SPDX-FileCopyrightText: 2026 QuickBar contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QKeySequence>
#include <QMenu>
#include <QObject>

namespace TaskManager
{
class TasksModel;
}

class MenuShortcutBridge : public QObject
{
    Q_OBJECT

public:
    explicit MenuShortcutBridge(TaskManager::TasksModel *tasksModel, QObject *parent = nullptr);

    void wireMenu(QMenu *menu);

private Q_SLOTS:
    void sendPendingShortcut();

private:
    void scheduleSend(const QKeySequence &sequence);
    void activateTarget();

    TaskManager::TasksModel *m_tasksModel;
    QKeySequence m_pendingSequence;
};

class MenuShortcutSender
{
public:
    static bool sendToActiveWindow(const QKeySequence &sequence);
};
