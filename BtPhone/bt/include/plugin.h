/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */

#ifndef PLUGIN_H
#define PLUGIN_H

#include <QQmlExtensionPlugin>

class Plugin : public QQmlExtensionPlugin
{
    Q_OBJECT
public:
    Plugin();
    void registerTypes(const char *uri);
};

#endif // PLUGIN_H