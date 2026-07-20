#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <functional>

class OutputController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList items           READ items           NOTIFY itemsChanged)
    Q_PROPERTY(QVariantMap  menus           READ menus           NOTIFY menusChanged)
    Q_PROPERTY(QString      initialMenu     READ initialMenu     NOTIFY menusChanged)
    Q_PROPERTY(bool         spawnAtMouse    READ spawnAtMouse    NOTIFY menusChanged)
    Q_PROPERTY(bool         escapeClosesAll READ escapeClosesAll NOTIFY menusChanged)

public:
    explicit OutputController(QObject *parent = nullptr);

    QVariantList items()           const { return m_items; }
    QVariantMap  menus()           const { return m_menus; }
    QString      initialMenu()     const { return m_initialMenu; }
    bool         spawnAtMouse()    const { return m_spawnAtMouse; }
    bool         escapeClosesAll() const { return m_escapeClosesAll; }

    void setItems(const QVariantList &items);
    void setMenuData(const QVariantMap &menus, const QString &initialMenu,
                     bool spawnAtMouse = true, bool escapeClosesAll = false);

    std::function<void(const QString &)> onSelectCallback;
    std::function<void()>                onCancelCallback;

    Q_INVOKABLE void select(const QString &label);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE QVariantMap getMousePosition();

Q_SIGNALS:
    void itemsChanged();
    void menusChanged();

private:
    QVariantList m_items;
    QVariantMap  m_menus;
    QString      m_initialMenu;
    bool         m_spawnAtMouse    = true;
    bool         m_escapeClosesAll = false;
};
