#pragma once

#include <QObject>
#include <QVariantList>
#include <functional>

class OutputController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)
public:
    explicit OutputController(QObject *parent = nullptr);

    QVariantList items() const { return m_items; }
    void setItems(const QVariantList &items);

    std::function<void(const QString &)> onSelectCallback;
    std::function<void()> onCancelCallback;

    Q_INVOKABLE void select(const QString &label);
    Q_INVOKABLE void cancel();

Q_SIGNALS:
    void itemsChanged();

private:
    QVariantList m_items;
};
