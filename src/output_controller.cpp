#include "output_controller.h"

OutputController::OutputController(QObject *parent) : QObject(parent) {}

void OutputController::setItems(const QVariantList &items) {
    m_items = items;
    emit itemsChanged();
}

void OutputController::select(const QString &label) {
    if (onSelectCallback) {
        onSelectCallback(label);
    }
}

void OutputController::cancel() {
    if (onCancelCallback) {
        onCancelCallback();
    }
}
