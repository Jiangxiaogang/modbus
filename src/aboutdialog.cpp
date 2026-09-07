#include "aboutdialog.h"
#include "version.h"

#include <QLabel>
#include <QFont>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>

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
    nameLbl->setAlignment(Qt::AlignCenter);

    QLabel *verLbl = new QLabel(QString("版本 %1 (基于 Qt %2 构建)").arg(APP_VERSION_STR).arg(qVersion()), this);
    verLbl->setAlignment(Qt::AlignCenter);

    QLabel *descLbl = new QLabel(APP_DESCRIPTION, this);
    descLbl->setAlignment(Qt::AlignCenter);

    QLabel *copyLbl = new QLabel(APP_COPYRIGHT, this);
    copyLbl->setAlignment(Qt::AlignCenter);
    copyLbl->setEnabled(false); // 灰色弱化显示版权行

    QPushButton *okBtn = new QPushButton("确定", this);
    okBtn->setDefault(true);
    connect(okBtn, SIGNAL(clicked()), this, SLOT(accept()));

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
