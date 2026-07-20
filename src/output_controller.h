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
    Q_PROPERTY(QVariantMap  style           READ style           NOTIFY styleChanged)

public:
    explicit OutputController(QObject *parent = nullptr);

    QVariantList items()           const { return m_items; }
    QVariantMap  menus()           const { return m_menus; }
    QString      initialMenu()     const { return m_initialMenu; }
    bool         spawnAtMouse()    const { return m_spawnAtMouse; }
    bool         escapeClosesAll() const { return m_escapeClosesAll; }
    QVariantMap  style()           const { return m_style; }

    void setItems(const QVariantList &items, const QVariantMap &style = {});
    void setMenuData(const QVariantMap &menus, const QString &initialMenu,
                     bool spawnAtMouse = true, bool escapeClosesAll = false,
                     const QVariantMap &style = {});

    std::function<void(const QString &)> onSelectCallback;
    std::function<void()>                onCancelCallback;

    Q_INVOKABLE void select(const QString &label);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void setStyle(const QVariantMap &style);
    Q_INVOKABLE QVariantMap getMousePosition();

Q_SIGNALS:
    void itemsChanged();
    void menusChanged();
    void styleChanged();

private:
    QVariantList m_items;
    QVariantMap  m_menus;
    QString      m_initialMenu;
    bool         m_spawnAtMouse    = true;
    bool         m_escapeClosesAll = false;
    QVariantMap  m_style;
};
