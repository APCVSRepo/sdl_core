/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */

#include <QDBusConnection>
#include <QDebug>
#include <QDBusInterface>
#include <QDBusReply>

#include "include/blue_handsfree.h"

blue_handsfree::blue_handsfree(QObject *parent) :
    QAbstractListModel(parent),
    m_selectedModem(NULL)
{
    roles[DeviceName] = "deviceName";
    roles[Selected] = "selected";

    auto systemBus = QDBusConnection::systemBus();

    if (!systemBus.connect("org.ofono",
                           "/",
                           "org.ofono.Manager",
                           "ModemAdded",
                           this,
                           SLOT(onModemAdded(const QDBusObjectPath &, const QVariantMap)))) {
        qWarning() << "Failed to connect ModemAdded signal";
    }

    if (!systemBus.connect("org.ofono",
                           "/",
                           "org.ofono.Manager",
                           "ModemRemoved",
                           this,
                           SLOT(onModemRemoved(const QDBusObjectPath &)))) {
        qWarning() << "Failed to connect ModemAdded signal";
    }

    fetchModems();
}

void blue_handsfree::fetchModems()
{
    QDBusInterface manager("org.ofono", "/", "org.ofono.Manager", QDBusConnection::systemBus());

    QDBusReply<QList<Modem> > reply;
    reply = manager.call("GetModems");
    if (!reply.isValid()) {
        qWarning() << "Failed to connect to ofono: " << reply.error().message();

        return;
    }

    beginResetModel();
    m_modems  = reply.value();
    endResetModel();
}

int blue_handsfree::rowCount(const QModelIndex &parent) const {
    return m_modems.size();
}

QVariant blue_handsfree::data(const QModelIndex &index, int role) const {
    const Modem &c = m_modems.at(index.row());

    switch (role) {
        case DeviceName:
            return c.name();
        case Selected:
            return m_selectedModem ? c.objectPath() == m_selectedModem->objectPath() : false;
        default:
            return "Unknown";
    };
}

QHash<int, QByteArray> blue_handsfree::roleNames() const
{
    return roles;
}

void blue_handsfree::dial(QString number)
{
    if (!m_selectedModem) {
        qWarning() << "No modem selected";
        return;
    }

    qDebug() << "Dialing " << number << " with " << m_selectedModem->objectPath().path();

    QDBusInterface iface("org.ofono",
                         m_selectedModem->objectPath().path(),
                         "org.ofono.VoiceCallManager",
                         QDBusConnection::systemBus());
    iface.call("Dial", number, "default");

}

void blue_handsfree::hangUp()
{
    if (!m_selectedModem) {
        qWarning() << "No modem selected";
        return;
    }

    QDBusInterface iface("org.ofono",
                         m_selectedModem->objectPath().path(),
                         "org.ofono.VoiceCallManager",
                         QDBusConnection::systemBus());
    iface.call("HangupAll");
}

void blue_handsfree::select(int idx)
{
    if (idx < m_modems.size()) {
        m_selectedModem = &m_modems[idx];
    } else {
        qWarning() << "Modem not found";
    }

    emit dataChanged(createIndex(idx - 1, 0), createIndex(idx + 1, 0));
}

void blue_handsfree::onModemAdded(const QDBusObjectPath &path,
                                      const QVariantMap modem)
{
    fetchModems();
}

void blue_handsfree::onModemRemoved(const QDBusObjectPath &path)
{
    fetchModems();
}