/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */

#ifndef BLUE_CONNECT_H
#define BLUE_CONNECT_H

#include <QObject>
#include <QtCore>
#include <QtGui>
#include <QtDBus/QtDBus>
#include "blue_handsfree.h"


class blue_connect : public QAbstractListModel
{
    Q_OBJECT

public:
    enum RoleNames {
        AddressRole = Qt::UserRole,
        NameRole = Qt::UserRole + 1,
        SelectedRole = Qt::UserRole + 2,
        PairedRole = Qt::UserRole + 3,
    };

    explicit blue_connect(QObject *parent=0);
    ~blue_connect();

    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionChanged)

    Q_PROPERTY(bool enablePhonebook MEMBER enablePhonebook)
    Q_PROPERTY(bool enableMediaPlayer MEMBER enableMediaPlayer)
    Q_PROPERTY(bool enableHandsfree MEMBER enableHandsfree)

    
    Q_INVOKABLE BluePlayer * connect (uint index);

    
    Q_INVOKABLE void disconnect ();

   
    Q_INVOKABLE void unpair (uint index);

    bool isConnected() const;

    bool enablePhonebook;
    bool enableMediaPlayer;
    bool enableHandsfree;

    
    virtual int rowCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;

    void updateDevice(QDBusInterface *device);
protected slots:
    void onInterfacesAdded(const QDBusObjectPath &path, const InterfacesMap interfaces);
    void onInterfacesRemoved(const QDBusObjectPath &path, const QStringList interfaces);

signals:
    void connectionChanged();
    void mediaPlayerAdded(BluePlayer *mediaPlayer);
    void phoneBookAdded(BluePhoneBook *phoneBook);
    void handsfreeAdded(BlueHandsfree *handsfree);

protected:
   
    virtual QHash<int, QByteArray> roleNames() const override;

private:
    void fetchObjects(QDBusInterface &manager);
    void setupAdapter(QDBusObjectPath path);
    void addDevice(QDBusObjectPath path);
    bool checkExistingDev(QDBusInterface *dev);
    void requestPhoneBookAccess(const QString &device);
    void requestPhoneBook(const QString &pbPath);
    BluePlayer *getPlayer();

    QList<QDBusInterface *> devices;
    int connected;
    QHash<int, QByteArray> roles;
    BluePhoneBook *m_phoneBook;
    BlueHandsfree *m_handsfree;
    int getDeviceIndex(QDBusInterface *dev);
};

#endif // blue_connect_H