#include "aboutdialog.h"
#include "version.h"

#include <QLabel>
#include <QFont>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QString("关于 %1").arg(APP_PRODUCT_NAME));
    setModal(true);
    setMinimumWidth(280);

    QLabel *nameLbl = new QLabel(APP_PRODUCT_NAME, this);
    QFont f = nameLbl->font();
    f.setPointSize(f.pointSize() + 6);
    f.setBold(true);
    nameLbl->setFont(f);

    QLabel *verLbl  = new QLabel(QString("版本 %1 (基于 Qt %2 构建)").arg(APP_VERSION_STR).arg(qVersion()), this);
    QLabel *descLbl = new QLabel(APP_DESCRIPTION, this);
    QLabel *copyLbl = new QLabel(APP_COPYRIGHT, this);
    copyLbl->setEnabled(false);

    for (QLabel *lbl : {nameLbl, verLbl, descLbl, copyLbl})
        lbl->setAlignment(Qt::AlignCenter);

    QPushButton *okBtn = new QPushButton("确定", this);
    okBtn->setDefault(true);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);

    QHBoxLayout *btnLay = new QHBoxLayout;
    btnLay->addStretch();
    btnLay->addWidget(okBtn);
    btnLay->addStretch();

    QVBoxLayout *v = new QVBoxLayout(this);
    v->setSpacing(8);
    v->addWidget(nameLbl);
    v->addWidget(verLbl);
    v->addWidget(descLbl);
    v->addWidget(copyLbl);
    v->addLayout(btnLay);
    v->addSpacing(4);
    setFixedSize(sizeHint());
}
