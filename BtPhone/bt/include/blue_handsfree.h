/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */

#ifndef BLUE_HANDSFREE_H
#define BLUE_HANDSFREE_H

#include <QObject>
#include <QAbstractListModel>
#include <QDBusInterface>
#include "plugin.h"


class blue_handsfree : public QAbstractListModel
{
    Q_OBJECT
public:
    enum RoleNames {
        DeviceName = Qt::UserRole,
        Selected = Qt::UserRole + 1,
    };

    explicit blue_handsfree(QObject *parent = 0);

    Q_INVOKABLE void dial(QString number);
    Q_INVOKABLE void hangUp();
    Q_INVOKABLE void select(int id);

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

    
    void requestPhoneBookAccess(const QString &device);

signals:

private slots:
    void onModemAdded(const QDBusObjectPath &path,
                           const QVariantMap modem);

    void onModemRemoved(const QDBusObjectPath &path);
private:
    QHash<int, QByteArray> roles;
    QList<Modem> m_modems;
    Modem *m_selectedModem;
    void requestPhoneBook(const QString &pbPath);
    void fetchModems();
};

#endif // blue_handsfree_H