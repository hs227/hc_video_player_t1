/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_2;
    QLabel *sdlWindow;
    QHBoxLayout *horizontalLayout;
    QLabel *videoPathLabel;
    QPushButton *vOpenBtn;
    QPushButton *vPlayBtn;
    QPushButton *vCloseBtn;
    QHBoxLayout *horizontalLayout_8;
    QRadioButton *vnormalBtn;
    QRadioButton *v360Btn;
    QRadioButton *v360RBtn;
    QHBoxLayout *horizontalLayout_7;
    QHBoxLayout *horizontalLayout_4;
    QLabel *posLabel_s1;
    QHBoxLayout *horizontalLayout_6;
    QLabel *posLabelA;
    QLabel *label;
    QLabel *posLabelB;
    QHBoxLayout *horizontalLayout_5;
    QSlider *posSlider;
    QHBoxLayout *horizontalLayout_3;
    QLabel *volumeLabel_s1;
    QLabel *volumeLabel;
    QDoubleSpinBox *speedSpinBox;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1085, 787);
        MainWindow->setMinimumSize(QSize(1024, 720));
        MainWindow->setFocusPolicy(Qt::NoFocus);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        sdlWindow = new QLabel(centralwidget);
        sdlWindow->setObjectName("sdlWindow");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(sdlWindow->sizePolicy().hasHeightForWidth());
        sdlWindow->setSizePolicy(sizePolicy);
        sdlWindow->setMinimumSize(QSize(800, 600));
        sdlWindow->setFocusPolicy(Qt::NoFocus);

        horizontalLayout_2->addWidget(sdlWindow);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        videoPathLabel = new QLabel(centralwidget);
        videoPathLabel->setObjectName("videoPathLabel");
        videoPathLabel->setMinimumSize(QSize(571, 28));
        videoPathLabel->setFocusPolicy(Qt::NoFocus);

        horizontalLayout->addWidget(videoPathLabel);

        vOpenBtn = new QPushButton(centralwidget);
        vOpenBtn->setObjectName("vOpenBtn");
        vOpenBtn->setFocusPolicy(Qt::NoFocus);

        horizontalLayout->addWidget(vOpenBtn);

        vPlayBtn = new QPushButton(centralwidget);
        vPlayBtn->setObjectName("vPlayBtn");
        vPlayBtn->setFocusPolicy(Qt::NoFocus);

        horizontalLayout->addWidget(vPlayBtn);

        vCloseBtn = new QPushButton(centralwidget);
        vCloseBtn->setObjectName("vCloseBtn");
        vCloseBtn->setFocusPolicy(Qt::NoFocus);

        horizontalLayout->addWidget(vCloseBtn);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName("horizontalLayout_8");
        vnormalBtn = new QRadioButton(centralwidget);
        vnormalBtn->setObjectName("vnormalBtn");
        vnormalBtn->setFocusPolicy(Qt::NoFocus);
        vnormalBtn->setChecked(true);

        horizontalLayout_8->addWidget(vnormalBtn);

        v360Btn = new QRadioButton(centralwidget);
        v360Btn->setObjectName("v360Btn");
        v360Btn->setFocusPolicy(Qt::NoFocus);
        v360Btn->setChecked(false);

        horizontalLayout_8->addWidget(v360Btn);

        v360RBtn = new QRadioButton(centralwidget);
        v360RBtn->setObjectName("v360RBtn");
        v360RBtn->setFocusPolicy(Qt::NoFocus);

        horizontalLayout_8->addWidget(v360RBtn);


        horizontalLayout->addLayout(horizontalLayout_8);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName("horizontalLayout_7");
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        posLabel_s1 = new QLabel(centralwidget);
        posLabel_s1->setObjectName("posLabel_s1");

        horizontalLayout_4->addWidget(posLabel_s1);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName("horizontalLayout_6");
        posLabelA = new QLabel(centralwidget);
        posLabelA->setObjectName("posLabelA");

        horizontalLayout_6->addWidget(posLabelA);

        label = new QLabel(centralwidget);
        label->setObjectName("label");

        horizontalLayout_6->addWidget(label);

        posLabelB = new QLabel(centralwidget);
        posLabelB->setObjectName("posLabelB");

        horizontalLayout_6->addWidget(posLabelB);


        horizontalLayout_4->addLayout(horizontalLayout_6);


        horizontalLayout_7->addLayout(horizontalLayout_4);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        posSlider = new QSlider(centralwidget);
        posSlider->setObjectName("posSlider");
        posSlider->setFocusPolicy(Qt::NoFocus);
        posSlider->setOrientation(Qt::Horizontal);

        horizontalLayout_5->addWidget(posSlider);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        volumeLabel_s1 = new QLabel(centralwidget);
        volumeLabel_s1->setObjectName("volumeLabel_s1");

        horizontalLayout_3->addWidget(volumeLabel_s1);

        volumeLabel = new QLabel(centralwidget);
        volumeLabel->setObjectName("volumeLabel");

        horizontalLayout_3->addWidget(volumeLabel);


        horizontalLayout_5->addLayout(horizontalLayout_3);

        speedSpinBox = new QDoubleSpinBox(centralwidget);
        speedSpinBox->setObjectName("speedSpinBox");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(speedSpinBox->sizePolicy().hasHeightForWidth());
        speedSpinBox->setSizePolicy(sizePolicy1);
        speedSpinBox->setMinimumSize(QSize(80, 25));
        speedSpinBox->setFocusPolicy(Qt::NoFocus);
        speedSpinBox->setReadOnly(true);
        speedSpinBox->setDecimals(1);
        speedSpinBox->setMaximum(2.000000000000000);
        speedSpinBox->setSingleStep(0.500000000000000);
        speedSpinBox->setValue(1.000000000000000);

        horizontalLayout_5->addWidget(speedSpinBox);


        horizontalLayout_7->addLayout(horizontalLayout_5);


        verticalLayout->addLayout(horizontalLayout_7);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1085, 25));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        sdlWindow->setText(QString());
        videoPathLabel->setText(QCoreApplication::translate("MainWindow", "waiting", nullptr));
        vOpenBtn->setText(QCoreApplication::translate("MainWindow", "\346\211\223\345\274\200\350\247\206\351\242\221", nullptr));
        vPlayBtn->setText(QCoreApplication::translate("MainWindow", "\346\222\255\346\224\276", nullptr));
        vCloseBtn->setText(QCoreApplication::translate("MainWindow", "\345\205\263\351\227\255", nullptr));
        vnormalBtn->setText(QCoreApplication::translate("MainWindow", "normal", nullptr));
        v360Btn->setText(QCoreApplication::translate("MainWindow", "360", nullptr));
        v360RBtn->setText(QCoreApplication::translate("MainWindow", "360R", nullptr));
        posLabel_s1->setText(QCoreApplication::translate("MainWindow", "Time:", nullptr));
        posLabelA->setText(QCoreApplication::translate("MainWindow", "000", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "/", nullptr));
        posLabelB->setText(QCoreApplication::translate("MainWindow", "999", nullptr));
        volumeLabel_s1->setText(QCoreApplication::translate("MainWindow", "Volume:", nullptr));
        volumeLabel->setText(QCoreApplication::translate("MainWindow", "100", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
