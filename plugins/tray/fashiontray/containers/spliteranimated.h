#ifndef SPLITERANIMATED_H
#define SPLITERANIMATED_H

#include "constants.h"

#include <QWidget>
#include <QVariantAnimation>

class SpliterAnimated : public QWidget
{
    Q_OBJECT
public:
    explicit SpliterAnimated(QWidget *parent = nullptr);

    void setStartValue(const QVariant &value);
    void setEndValue(const QVariant &value);
    void startAnimation();
    void stopAnimation();

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onSizeAnimationValueChanged(const QVariant &value);

private:
    QVariantAnimation *m_sizeAnimation;

    qreal m_opacityChangeStep;
    qreal m_currentOpacity;
};

#endif // SPLITERANIMATED_H
