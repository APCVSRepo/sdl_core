/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */

#include "include/blue_connect.h"
#include "iostream"
#include <unistd.h>

#define A2DP_PREFIX 0x110A
#define HFP_PREFIX 0x111F
#define HFP_PREFIX2 0x111E

std::ostream& operator<<(std::ostream& str, const QString& string) {
    return str << string.toStdString();
}



QString getProvileUUID(QDBusInterface *dev, int prefix)
{
    auto uuids = dev->property("UUIDs").toStringList();

    for (auto k = uuids.begin(); k != uuids.end(); ++k) {
        QString str = (*k);
        QStringList parts = str.split("-");
        if (parts.length() < 1) {
            qDebug() << "Ignoring " << *k;
            continue;
        }

        uint profile = parts.at(0).toUInt(Q_NULLPTR, 16);
        if (profile == prefix)
           return str;
    }

    return Q_NULLPTR;
}

blue_connect::blue_connect(QObject *parent) : QAbstractListModel(parent)
{
    
    roles[NameRole] = "name";
    roles[AddressRole] = "address";
    roles[SelectedRole] = "selected";
    roles[PairedRole] = "paired";

    connected = -1;

    enableHandsfree = false;
    enableMediaPlayer = false;
    enablePhonebook = false;

    qDBusRegisterMetaType <InterfacesMap> ();
    qDBusRegisterMetaType <ObjectsMap> ();
    qDBusRegisterMetaType <Contact> ();
    qDBusRegisterMetaType <QList<Contact> > ();
    qDBusRegisterMetaType <Modem> ();
    qDBusRegisterMetaType <QList<Modem> > ();

    auto systemBus = QDBusConnection::systemBus();
    QDBusInterface manager("org.bluez",
                           "/",
                           "org.freedesktop.DBus.ObjectManager",
                           systemBus);

    fetchObjects(manager);

    if (!systemBus.connect("org.bluez",
                     "/",
                     "org.freedesktop.DBus.ObjectManager",
                     "InterfacesAdded",
                     this,
                     SLOT(onInterfacesAdded(const QDBusObjectPath &, const InterfacesMap)))) {
        qWarning() << "Failed to connect InterfacesAdded signal";
    }

    if (!systemBus.connect("org.bluez",
                     "/",
                     "org.freedesktop.DBus.ObjectManager",
                     "InterfacesRemoved",
                     this,
                     SLOT(onInterfacesRemoved(const QDBusObjectPath &, const QStringList)))) {
        qWarning() << "Failed to connect InterfacesRemoved signal";
    }
}

blue_connect::~blue_connect()
{
}

void blue_connect::onInterfacesAdded(const QDBusObjectPath &path,
                                    const InterfacesMap interfaces)
{
    for (auto i = interfaces.begin(); i != interfaces.end(); ++i) {
        if (i.key() == "org.bluez.Device1") {
            std::cout << "'org.bluez.Device1' interface added at "
                      << path.path()
                      << std::endl;

            addDevice (path);

        } else if (i.key() == "org.bluez.MediaPlayer1") {
            std::cout << "'org.bluez.MediaPlayer1' interface added at "
                      << path.path()
                      << std::endl;

            BluePlayer *mediaPlayer = new BluePlayer(QDBusObjectPath(path.path()), NULL);
            emit mediaPlayerAdded(mediaPlayer);
        }
    }
}

void blue_connect::onInterfacesRemoved(const QDBusObjectPath &path,
                                      const QStringList interfaces)
{
    bool ignore = true;

    for (auto i = interfaces.begin(); i != interfaces.end(); ++i) {
        if (*i == "org.bluez.Device1") {
            ignore = false;
            std::cout << "'org.bluez.Device1' interface removed from "
                      << path.path()
                      << std::endl;

            break;
        }
    }

    if (ignore)
        return;

    for (int i = 0; i < devices.length(); i++) {
        if (path.path() == devices[0]->path()) {
            emit beginRemoveRows(QModelIndex(), i, i);
            devices.removeAt(i);
            emit endRemoveRows();
        }
    }
}

void blue_connect::fetchObjects(QDBusInterface &manager)
{
    QDBusReply<QMap<QDBusObjectPath,QMap<QString,QVariantMap > > > reply;
    reply = manager.call("GetManagedObjects");
    if (!reply.isValid()) {
        std::cout << "Failed to connect to bluez: " << reply.error().message() << std::endl;

        return;
    }

    auto objects = reply.value();

    for (auto i = objects.begin(); i != objects.end(); ++i) {
        auto ifaces = i.value();

        std::cout << i.key().path() << std::endl;
        for (auto j = ifaces.begin(); j != ifaces.end(); ++j) {
            if (j.key() == "org.bluez.Device1")
                addDevice (i.key());
            else if (j.key() == "org.bluez.Adapter1")
                setupAdapter (i.key());
        }
    }
}

void blue_connect::updateDevice(QDBusInterface *device)
{
    int index = getDeviceIndex(device);

    // Don't update the HMI if there is no address
    QString address = device->property("Address").toString();
    if (!address.isEmpty()) {
        emit dataChanged(createIndex(index - 1, 0), createIndex(index + 1, 0));
    }
}

void blue_connect::addDevice(QDBusObjectPath path)
{
    auto systemBus = QDBusConnection::systemBus();
    QDBusInterface *dev;
    dev = new QDBusInterface ("org.bluez",
                              path.path(),
                              "org.bluez.Device",
                              systemBus);

    if (!checkExistingDev(dev)) {
        emit beginInsertRows(QModelIndex(), devices.length(), devices.length());
        devices << dev;
        emit endInsertRows();
        new blue_connectPropertyCallback(this, dev, path.path());
    }
}

int blue_connect::getDeviceIndex(QDBusInterface *dev)
{
    for (int index = 0; index < devices.length(); ++index) {
        if (dev == devices[index]) {
            return index;
        }
    }
    return -1;
}

void blue_connect::setupAdapter(QDBusObjectPath path)
{
    QDBusInterface adapter("org.bluez",
                           path.path(),
                           "org.bluez.Adapter1",
                           QDBusConnection::systemBus());
    adapter.setProperty("Powered", true);

    QDBusReply<void> reply = adapter.call("StartDiscovery");
    if (!reply.isValid()) {
        std::cout << "Failed to start discovery: " << reply.error().message() << std::endl;

        return;
    }
}

bool blue_connect::checkExistingDev(QDBusInterface *dev)
{
    for (auto i = devices.begin(); i != devices.end(); ++i) {
        auto existing = *i;

        if (existing->path() == dev->path())
            return true;
    }

    return false;
}

BluePlayer * blue_connect::connect (uint index)
{
    if (connected >= 0)
        return Q_NULLPTR;

    auto dev = devices[index];
    auto address = dev->property("Address").toString();
    std::cout << "will connect to:" << address << std::endl;

    auto a2dp_uuid = getProvileUUID(dev, A2DP_PREFIX);
    auto hfp_uuid = getProvileUUID(dev, HFP_PREFIX);
    auto hfp_uuid2 = getProvileUUID(dev, HFP_PREFIX2);

    if (enableMediaPlayer && a2dp_uuid == Q_NULLPTR) {
        qWarning() << "A2DP UUID not found";
        return Q_NULLPTR;
    }

    if (enableHandsfree && hfp_uuid == Q_NULLPTR) {
        qWarning() << "HFP UUID not found";
        return Q_NULLPTR;
    }

    if (dev->property("Paired").toBool()) {
        std::cout << "Device already paired" << std::endl;
    } else {
        QDBusReply<void> reply = dev->call("Pair");
        if (!reply.isValid()) {
            std::cout << "Failed to pair: " << reply.error().message() << std::endl;

            return Q_NULLPTR;
        }
    }

    std::cout << "Trusting device: " << address << std::endl;
    dev->setProperty("Trusted", true);

    if (enablePhonebook) {
        m_phoneBook = new BluePhoneBook(address);
        emit phoneBookAdded(m_phoneBook);
    }

    if (enableMediaPlayer) {
       
        QDBusReply<void> reply = dev->call("ConnectProfile", a2dp_uuid);
        if (!reply.isValid()) {
        std::cout << "Failed to connect A2DP: " << reply.error().message() << std::endl;

        return Q_NULLPTR;
        }
    }

    if (enableHandsfree) {
        m_handsfree = new BlueHandsfree();
        emit handsfreeAdded(m_handsfree);

        QDBusReply<void> reply = dev->call("ConnectProfile", hfp_uuid);
        if (!reply.isValid()) {
        std::cout << "Failed to connect HFP: " << reply.error().message() << std::endl;
        }

        reply = dev->call("ConnectProfile", hfp_uuid2);
        if (!reply.isValid()) {
        std::cout << "Failed to connect secondary HFP profile: " << reply.error().message() << std::endl;
        }
    }

    QDBusReply<void> reply = dev->call("Connect");
    if (!reply.isValid()) {
        std::cout << "Failed to connect extra profiles: " << reply.error().message() << std::endl;
    }

    connected = index;
    emit connectionChanged();
    emit dataChanged(createIndex(index - 1, 0), createIndex(index + 1, 0));

    return NULL;
}

void blue_connect::disconnect ()
{
    if(connected < 0 || connected >= devices.length())
        return;

    auto dev = devices[connected];
    auto name = dev->property("Name").toString();
    std::cout << "Disconnecting " << name << std::endl;

    QDBusReply<void> reply = dev->call("Disconnect");
    if (!reply.isValid()) {
        std::cout << "Failed to disconnect: " << reply.error().message() << std::endl;

        return;
    }
    auto index = connected;
    connected = -1;
    emit connectionChanged();
    emit dataChanged(createIndex(index - 1, 0), createIndex(index + 1, 0));
}
void blue_connect::unpair (uint index)
{
    auto dev = devices[index];
    auto separator = dev->path().lastIndexOf("/");
    auto path = dev->path();
    path.chop(dev->path().length() - separator);
    QDBusObjectPath devicePath(path);

    auto adapter = new QDBusInterface ("org.bluez",
                                       devicePath.path(),
                                       "org.bluez.Adapter1",
                                       QDBusConnection::systemBus());

    QDBusReply<void> reply = adapter->call("RemoveDevice",
                                           QVariant::fromValue(QDBusObjectPath(dev->path())));
    if (!reply.isValid()) {
        qWarning() << "Failed to remove device '"<< dev->path() << "': " << reply.error();
    }

    connected = -1;
}


int blue_connect::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return devices.length();
}

QVariant blue_connect::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row < 0 || row >= devices.length())
        return QVariant();

    QDBusInterface *device = devices[row];
    switch (role) {
    case NameRole: {
        QString name = device->property("Name").toString();
        QString address = device->property("Address").toString();

        if (name.isEmpty()) {
            return address;
        }
        return QString("%1 (%2)").arg(name, address);
    }
    case AddressRole: {
        QVariant var = device->property("Address");
        return var.toString();
    }
    case PairedRole: {
        QVariant var = device->property("Paired");
        return var.toBool();
    }
    case SelectedRole: {
        return (row == connected);
    }
    default: {
        return QVariant();
    }
    }
}

QHash<int, QByteArray> blue_connect::roleNames() const
{
    return roles;
}

bool blue_connect::isConnected() const
{
    return (connected >= 0);
}