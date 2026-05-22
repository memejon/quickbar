/*
    SPDX-FileCopyrightText: 2026 QuickBar contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "menushortcutsender.h"

#include <abstracttasksmodel.h>
#include <tasksmodel.h>

#include <KWindowSystem>
#include <KX11Extras>
#include <QAction>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QEventLoop>
#include <QGuiApplication>
#include <QKeyCombination>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <QVariantMap>

#include <algorithm>
#include <linux/input-event-codes.h>
#include <utility>

#if QT_CONFIG(xcb)
#include <KKeyServer>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <fixx11h.h>
#include <qguiapplication_platform.h>
#endif

namespace
{
struct LinuxKeyStroke {
    quint32 key = 0;
    Qt::KeyboardModifiers extraModifiers;
};

LinuxKeyStroke linuxKeyStrokeForQtKey(Qt::Key key)
{
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        return {quint32(KEY_A + (key - Qt::Key_A)), {}};
    }
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        return {quint32(KEY_0 + (key - Qt::Key_0)), {}};
    }
    if (key >= Qt::Key_F1 && key <= Qt::Key_F12) {
        return {quint32(KEY_F1 + (key - Qt::Key_F1)), {}};
    }

    switch (key) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return {KEY_ENTER, {}};
    case Qt::Key_Escape:
        return {KEY_ESC, {}};
    case Qt::Key_Tab:
        return {KEY_TAB, {}};
    case Qt::Key_Backspace:
        return {KEY_BACKSPACE, {}};
    case Qt::Key_Delete:
        return {KEY_DELETE, {}};
    case Qt::Key_Insert:
        return {KEY_INSERT, {}};
    case Qt::Key_Home:
        return {KEY_HOME, {}};
    case Qt::Key_End:
        return {KEY_END, {}};
    case Qt::Key_PageUp:
        return {KEY_PAGEUP, {}};
    case Qt::Key_PageDown:
        return {KEY_PAGEDOWN, {}};
    case Qt::Key_Left:
        return {KEY_LEFT, {}};
    case Qt::Key_Right:
        return {KEY_RIGHT, {}};
    case Qt::Key_Up:
        return {KEY_UP, {}};
    case Qt::Key_Down:
        return {KEY_DOWN, {}};
    case Qt::Key_Space:
        return {KEY_SPACE, {}};
    case Qt::Key_Plus:
        return {KEY_EQUAL, Qt::ShiftModifier};
    case Qt::Key_Minus:
        return {KEY_MINUS, {}};
    case Qt::Key_Equal:
        return {KEY_EQUAL, {}};
    case Qt::Key_Comma:
        return {KEY_COMMA, {}};
    case Qt::Key_Period:
        return {KEY_DOT, {}};
    case Qt::Key_Slash:
        return {KEY_SLASH, {}};
    case Qt::Key_Backslash:
        return {KEY_BACKSLASH, {}};
    case Qt::Key_BracketLeft:
        return {KEY_LEFTBRACE, {}};
    case Qt::Key_BracketRight:
        return {KEY_RIGHTBRACE, {}};
    case Qt::Key_Semicolon:
        return {KEY_SEMICOLON, {}};
    case Qt::Key_Apostrophe:
        return {KEY_APOSTROPHE, {}};
    case Qt::Key_QuoteLeft:
        return {KEY_GRAVE, {}};
    default:
        return {};
    }
}

QList<quint32> linuxModifierKeys(Qt::KeyboardModifiers modifiers)
{
    QList<quint32> keys;
    if (modifiers & Qt::ControlModifier) {
        keys << KEY_LEFTCTRL;
    }
    if (modifiers & Qt::AltModifier) {
        keys << KEY_LEFTALT;
    }
    if (modifiers & Qt::ShiftModifier) {
        keys << KEY_LEFTSHIFT;
    }
    if (modifiers & Qt::MetaModifier) {
        keys << KEY_LEFTMETA;
    }
    return keys;
}

bool isKdeWaylandSession()
{
    return QString::fromLocal8Bit(qgetenv("XDG_CURRENT_DESKTOP")).contains(QLatin1String("KDE"), Qt::CaseInsensitive);
}

QString nextPortalToken(const QString &prefix)
{
    static quint64 counter = 0;
    return QStringLiteral("%1_%2").arg(prefix).arg(++counter);
}

QVariant unwrapDBusVariant(const QVariant &value)
{
    if (value.canConvert<QDBusVariant>()) {
        return value.value<QDBusVariant>().variant();
    }
    return value;
}

class PortalResponseWaiter : public QObject
{
    Q_OBJECT

public:
    bool wait(const QDBusObjectPath &handle, int timeoutMs = 30000)
    {
        m_received = false;
        m_response = 1;
        m_results.clear();

        auto bus = QDBusConnection::sessionBus();
        const bool connected = bus.connect(QStringLiteral("org.freedesktop.portal.Desktop"),
                                           handle.path(),
                                           QStringLiteral("org.freedesktop.portal.Request"),
                                           QStringLiteral("Response"),
                                           this,
                                           SLOT(onResponse(uint, QVariantMap)));
        if (!connected) {
            return false;
        }

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(this, &PortalResponseWaiter::responseReceived, &loop, &QEventLoop::quit);
        timer.start(timeoutMs);
        loop.exec();

        bus.disconnect(QStringLiteral("org.freedesktop.portal.Desktop"),
                       handle.path(),
                       QStringLiteral("org.freedesktop.portal.Request"),
                       QStringLiteral("Response"),
                       this,
                       SLOT(onResponse(uint, QVariantMap)));

        return m_received && m_response == 0;
    }

    QVariantMap results() const
    {
        return m_results;
    }

Q_SIGNALS:
    void responseReceived();

private Q_SLOTS:
    void onResponse(uint response, const QVariantMap &results)
    {
        m_received = true;
        m_response = response;
        m_results = results;
        Q_EMIT responseReceived();
    }

private:
    bool m_received = false;
    uint m_response = 1;
    QVariantMap m_results;
};

class PortalRemoteDesktopSender : public QObject
{
public:
    explicit PortalRemoteDesktopSender(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    bool send(const QKeyCombination &combination)
    {
        if (!ensureSession()) {
            return false;
        }

        const int combined = int(combination.toCombined());
        const auto modifiers = Qt::KeyboardModifiers(combined & Qt::KeyboardModifierMask);
        const auto key = static_cast<Qt::Key>(combined & ~Qt::KeyboardModifierMask);
        const LinuxKeyStroke stroke = linuxKeyStrokeForQtKey(key);
        if (stroke.key == 0) {
            return false;
        }

        QList<quint32> modifierKeys = linuxModifierKeys(modifiers | stroke.extraModifiers);
        for (quint32 modifierKey : std::as_const(modifierKeys)) {
            notifyKeycode(modifierKey, KeyPressed);
        }

        notifyKeycode(stroke.key, KeyPressed);
        notifyKeycode(stroke.key, KeyReleased);

        std::reverse(modifierKeys.begin(), modifierKeys.end());
        for (quint32 modifierKey : std::as_const(modifierKeys)) {
            notifyKeycode(modifierKey, KeyReleased);
        }

        return true;
    }

private:
    enum KeyState : uint {
        KeyReleased = 0,
        KeyPressed = 1,
    };

    bool ensureSession()
    {
        if (m_started) {
            return true;
        }
        if (m_startAttempted) {
            return false;
        }
        m_startAttempted = true;

        return createSession() && selectKeyboardDevice() && startSession();
    }

    QDBusObjectPath callRequestMethod(const QString &method, const QList<QVariant> &arguments)
    {
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                                              QStringLiteral("/org/freedesktop/portal/desktop"),
                                                              QStringLiteral("org.freedesktop.portal.RemoteDesktop"),
                                                              method);
        message.setArguments(arguments);

        const QDBusMessage reply = QDBusConnection::sessionBus().call(message);
        if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
            return {};
        }

        return reply.arguments().constFirst().value<QDBusObjectPath>();
    }

    bool waitForRequest(const QDBusObjectPath &handle, QVariantMap *results)
    {
        if (handle.path().isEmpty()) {
            return false;
        }

        PortalResponseWaiter waiter;
        if (!waiter.wait(handle)) {
            return false;
        }

        if (results) {
            *results = waiter.results();
        }
        return true;
    }

    bool createSession()
    {
        QVariantMap options;
        options.insert(QStringLiteral("handle_token"), nextPortalToken(QStringLiteral("quickbar_create")));
        options.insert(QStringLiteral("session_handle_token"), nextPortalToken(QStringLiteral("quickbar_session")));

        QVariantMap results;
        if (!waitForRequest(callRequestMethod(QStringLiteral("CreateSession"), {options}), &results)) {
            return false;
        }

        const QVariant sessionHandle = unwrapDBusVariant(results.value(QStringLiteral("session_handle")));
        m_sessionHandle = sessionHandle.toString();
        if (m_sessionHandle.isEmpty() && sessionHandle.canConvert<QDBusObjectPath>()) {
            m_sessionHandle = sessionHandle.value<QDBusObjectPath>().path();
        }
        return !m_sessionHandle.isEmpty();
    }

    bool selectKeyboardDevice()
    {
        QVariantMap options;
        options.insert(QStringLiteral("handle_token"), nextPortalToken(QStringLiteral("quickbar_select")));
        options.insert(QStringLiteral("types"), uint(KeyboardDevice));
        options.insert(QStringLiteral("persist_mode"), uint(1));

        return waitForRequest(callRequestMethod(QStringLiteral("SelectDevices"), {QVariant::fromValue(QDBusObjectPath(m_sessionHandle)), options}), nullptr);
    }

    bool startSession()
    {
        QVariantMap options;
        options.insert(QStringLiteral("handle_token"), nextPortalToken(QStringLiteral("quickbar_start")));

        QVariantMap results;
        if (!waitForRequest(callRequestMethod(QStringLiteral("Start"), {QVariant::fromValue(QDBusObjectPath(m_sessionHandle)), QString(), options}), &results)) {
            return false;
        }

        const uint devices = unwrapDBusVariant(results.value(QStringLiteral("devices"))).toUInt();
        m_started = devices & KeyboardDevice;
        return m_started;
    }

    bool notifyKeycode(quint32 keycode, KeyState state)
    {
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                                              QStringLiteral("/org/freedesktop/portal/desktop"),
                                                              QStringLiteral("org.freedesktop.portal.RemoteDesktop"),
                                                              QStringLiteral("NotifyKeyboardKeycode"));
        message << QDBusObjectPath(m_sessionHandle) << QVariantMap() << int(keycode) << uint(state);

        const QDBusMessage reply = QDBusConnection::sessionBus().call(message);
        return reply.type() != QDBusMessage::ErrorMessage;
    }

    static constexpr uint KeyboardDevice = 1;

    bool m_startAttempted = false;
    bool m_started = false;
    QString m_sessionHandle;
};

bool sendViaPortalRemoteDesktop(const QKeyCombination &combination)
{
    static PortalRemoteDesktopSender sender;
    return sender.send(combination);
}

#if QT_CONFIG(xcb)
Display *x11Display()
{
    auto *x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    return x11 ? x11->display() : nullptr;
}

void fakeKey(Display *display, int keyCode, bool press)
{
    XTestFakeKeyEvent(display, keyCode, press, CurrentTime);
    XFlush(display);
}

void sendKeyQt(Display *display, int keyQt)
{
    uint modMask = 0;
    if (!KKeyServer::keyQtToModX(keyQt, &modMask)) {
        modMask = 0;
    }

    const QList<int> codes = KKeyServer::keyQtToCodeXs(keyQt);
    if (codes.isEmpty()) {
        return;
    }

    if (modMask & KKeyServer::modXShift()) {
        fakeKey(display, XKeysymToKeycode(display, XK_Shift_L), true);
    }
    if (modMask & KKeyServer::modXCtrl()) {
        fakeKey(display, XKeysymToKeycode(display, XK_Control_L), true);
    }
    if (modMask & KKeyServer::modXAlt()) {
        fakeKey(display, XKeysymToKeycode(display, XK_Alt_L), true);
    }
    if (modMask & KKeyServer::modXMeta()) {
        fakeKey(display, XKeysymToKeycode(display, XK_Super_L), true);
    }

    for (int code : codes) {
        fakeKey(display, code, true);
        fakeKey(display, code, false);
    }

    if (modMask & KKeyServer::modXMeta()) {
        fakeKey(display, XKeysymToKeycode(display, XK_Super_L), false);
    }
    if (modMask & KKeyServer::modXAlt()) {
        fakeKey(display, XKeysymToKeycode(display, XK_Alt_L), false);
    }
    if (modMask & KKeyServer::modXCtrl()) {
        fakeKey(display, XKeysymToKeycode(display, XK_Control_L), false);
    }
    if (modMask & KKeyServer::modXShift()) {
        fakeKey(display, XKeysymToKeycode(display, XK_Shift_L), false);
    }
}

bool sendViaX11(const QKeyCombination &combination)
{
    Display *display = x11Display();
    if (!display) {
        return false;
    }

    KKeyServer::initializeMods();

    const QList<int> codes = KKeyServer::keyQtToCodeXs(int(combination.toCombined()));
    if (codes.isEmpty()) {
        return false;
    }

    const WId active = KX11Extras::activeWindow();
    if (active) {
        KX11Extras::activateWindow(active);
    }

    sendKeyQt(display, int(combination.toCombined()));
    return true;
}
#endif

QString qtKeyToWtypeName(Qt::Key key)
{
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        return QString(QChar('a' + (key - Qt::Key_A)));
    }
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        return QString(QChar('0' + (key - Qt::Key_0)));
    }
    if (key >= Qt::Key_F1 && key <= Qt::Key_F35) {
        return QStringLiteral("F%1").arg(1 + (key - Qt::Key_F1));
    }

    switch (key) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return QStringLiteral("Return");
    case Qt::Key_Escape:
        return QStringLiteral("Escape");
    case Qt::Key_Tab:
        return QStringLiteral("Tab");
    case Qt::Key_Backspace:
        return QStringLiteral("BackSpace");
    case Qt::Key_Delete:
        return QStringLiteral("Delete");
    case Qt::Key_Insert:
        return QStringLiteral("Insert");
    case Qt::Key_Home:
        return QStringLiteral("Home");
    case Qt::Key_End:
        return QStringLiteral("End");
    case Qt::Key_PageUp:
        return QStringLiteral("Page_Up");
    case Qt::Key_PageDown:
        return QStringLiteral("Page_Down");
    case Qt::Key_Left:
        return QStringLiteral("Left");
    case Qt::Key_Right:
        return QStringLiteral("Right");
    case Qt::Key_Up:
        return QStringLiteral("Up");
    case Qt::Key_Down:
        return QStringLiteral("Down");
    case Qt::Key_Space:
        return QStringLiteral("space");
    case Qt::Key_Plus:
        return QStringLiteral("plus");
    case Qt::Key_Minus:
        return QStringLiteral("minus");
    case Qt::Key_Equal:
        return QStringLiteral("equal");
    case Qt::Key_Comma:
        return QStringLiteral("comma");
    case Qt::Key_Period:
        return QStringLiteral("period");
    case Qt::Key_Slash:
        return QStringLiteral("slash");
    case Qt::Key_Backslash:
        return QStringLiteral("backslash");
    case Qt::Key_BracketLeft:
        return QStringLiteral("bracketleft");
    case Qt::Key_BracketRight:
        return QStringLiteral("bracketright");
    case Qt::Key_Semicolon:
        return QStringLiteral("semicolon");
    case Qt::Key_Apostrophe:
        return QStringLiteral("apostrophe");
    case Qt::Key_QuoteLeft:
        return QStringLiteral("grave");
    default:
        return {};
    }
}

bool sendViaWtype(const QKeyCombination &combination)
{
    const QString wtype = QStandardPaths::findExecutable(QStringLiteral("wtype"));
    if (wtype.isEmpty()) {
        return false;
    }

    const int combined = int(combination.toCombined());
    const auto mods = Qt::KeyboardModifiers(combined & Qt::KeyboardModifierMask);
    const auto key = static_cast<Qt::Key>(combined & ~Qt::KeyboardModifierMask);

    const QString keyName = qtKeyToWtypeName(key);
    if (keyName.isEmpty()) {
        return false;
    }

    QStringList args;
    if (mods & Qt::ControlModifier) {
        args << QStringLiteral("-M") << QStringLiteral("ctrl");
    }
    if (mods & Qt::AltModifier) {
        args << QStringLiteral("-M") << QStringLiteral("alt");
    }
    if (mods & Qt::MetaModifier) {
        args << QStringLiteral("-M") << QStringLiteral("super");
    }
    if (mods & Qt::ShiftModifier) {
        args << QStringLiteral("-M") << QStringLiteral("shift");
    }
    args << QStringLiteral("-k") << keyName;

    return QProcess::startDetached(wtype, args);
}

bool sendCombination(const QKeyCombination &combination)
{
    if (KWindowSystem::isPlatformWayland()) {
        if (isKdeWaylandSession()) {
            return sendViaPortalRemoteDesktop(combination);
        }
        if (sendViaWtype(combination)) {
            return true;
        }
#if QT_CONFIG(xcb)
        return sendViaX11(combination);
#else
        return false;
#endif
    }

#if QT_CONFIG(xcb)
    if (sendViaX11(combination)) {
        return true;
    }
#endif
    return sendViaWtype(combination);
}
} // namespace

MenuShortcutBridge::MenuShortcutBridge(TaskManager::TasksModel *tasksModel, QObject *parent)
    : QObject(parent)
    , m_tasksModel(tasksModel)
{
}

void MenuShortcutBridge::wireMenu(QMenu *menu)
{
    if (!menu) {
        return;
    }

    const auto actions = menu->findChildren<QAction *>();
    for (QAction *action : actions) {
        if (action->isSeparator() || action->shortcut().isEmpty()) {
            continue;
        }

        const QKeySequence shortcut = action->shortcut();
        connect(action, &QAction::triggered, this, [this, shortcut] {
            scheduleSend(shortcut);
        });
    }
}

void MenuShortcutBridge::scheduleSend(const QKeySequence &sequence)
{
    if (sequence.isEmpty()) {
        return;
    }

    m_pendingSequence = sequence;
    activateTarget();

    QTimer::singleShot(100, this, &MenuShortcutBridge::sendPendingShortcut);
}

void MenuShortcutBridge::activateTarget()
{
    if (!m_tasksModel) {
        return;
    }

    const QModelIndex idx = m_tasksModel->activeTask();
    if (idx.isValid()) {
        m_tasksModel->requestActivate(idx);
    }
}

void MenuShortcutBridge::sendPendingShortcut()
{
    MenuShortcutSender::sendToActiveWindow(m_pendingSequence);
}

bool MenuShortcutSender::sendToActiveWindow(const QKeySequence &sequence)
{
    if (sequence.isEmpty()) {
        return false;
    }

    bool sent = false;
    for (int i = 0; i < sequence.count(); ++i) {
        sent = sendCombination(sequence[i]) || sent;
    }
    return sent;
}

#include "menushortcutsender.moc"
