#include "choosecolordialog.h"
#include "ui_choosecolordialog.h"

#include <QPainter>

ChooseColorDialog::ChooseColorDialog(const QString& username, Const::HostType type, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChooseColorDialog),
    m_username(username),
    m_type(type),
    m_is_disconnected(false)
{
    ui->setupUi(this);
    //this->setWindowFlag(Qt::Dialog | Qt::WindowCloseButtonHint);

    drawIcon();
    this->setFixedHeight(this->sizeHint().height());
    if (type == Const::Client)
        ui->start->hide();
    m_state[0] = m_state[1] = -1;

    connect(ui->player0, &QPushButton::clicked , this, &ChooseColorDialog::onFirst);
    connect(ui->player1, &QPushButton::clicked, this, &ChooseColorDialog::onSecond);
    connect(ui->start, &QPushButton::clicked, this, &ChooseColorDialog::onStart);
}

ChooseColorDialog::~ChooseColorDialog()
{
    delete ui;
}

void ChooseColorDialog::drawIcon()
{
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setBrush(Qt::black);
    painter.drawEllipse(QPoint(12, 12), 10, 10);
    ui->player0->setIcon(pixmap);

    pixmap.fill(Qt::transparent);
    painter.setBrush(Qt::white);
    painter.drawEllipse(QPoint(12, 12), 10, 10);
    ui->player1->setIcon(pixmap);
}

void ChooseColorDialog::changeInfo()
{
    if (ui->player0->isChecked() && ui->player1->isChecked())
    {
        if (m_type == Const::Server)
        {
            ui->start->setEnabled(true);
            ui->start->setFocus();
            ui->label_info->setText(tr("Press the start button to start."));
        }
        else
            ui->label_info->setText(tr("Waiting for the server to start..."));
    }
    else
    {
        ui->start->setEnabled(false);
        if (m_state[0] != (int)m_type && m_state[1] != (int)m_type)
            ui->label_info->setText(tr("Please choose one color."));
        else
            ui->label_info->setText(tr("Waiting for the opponent to choose..."));
    }
}

void ChooseColorDialog::sendData(bool isAccepted)
{
    emit prepareStateChanged(isAccepted, m_state[0], m_state[1], m_username);
}

void ChooseColorDialog::onUpdateState(bool isAccepted, int state0, int state1, const QString& oppUsername)
{
    if (m_type == Const::Server && ((state0 != m_state[0] && state0 >= 0 && m_state[0] >= 0)
                                ||  (state1 != m_state[1] && state1 >= 0 && m_state[1] >= 0)))
        sendData(isAccepted);
    else
    {
        m_state[0] = state0;
        m_state[1] = state1;
        if (state0 >= 0)
        {
            ui->label_user0_arrow->setText("▼");
            ui->player0->setChecked(true);
            if (state0 == (int)m_type)
            {
                ui->label_user0->setText(m_username);
                ui->label_user0->setStyleSheet("");
                ui->label_user0_arrow->setStyleSheet("");
                ui->player0->setEnabled(true);
            }
            else
            {
                ui->label_user0->setText(oppUsername);
                ui->label_user0->setStyleSheet("color:rgb(180,180,180);");
                ui->label_user0_arrow->setStyleSheet("color:rgb(180,180,180);");
                ui->player0->setEnabled(false);
            }
        }
        else
        {
            ui->label_user0->clear();
            ui->label_user0->setStyleSheet("");
            ui->label_user0_arrow->clear();
            ui->label_user0_arrow->setStyleSheet("");
            ui->player0->setEnabled(true);
            ui->player0->setChecked(false);
        }

        if (state1 >= 0)
        {
            ui->label_user1_arrow->setText("▼");
            ui->player1->setChecked(true);
            if (state1 == (int)m_type)
            {
                ui->label_user1->setText(m_username);
                ui->label_user1->setStyleSheet("");
                ui->label_user1_arrow->setStyleSheet("");
                ui->player1->setEnabled(true);
            }
            else
            {
                ui->label_user1->setText(oppUsername);
                ui->label_user1->setStyleSheet("color:rgb(180,180,180);");
                ui->label_user1_arrow->setStyleSheet("color:rgb(180,180,180);");
                ui->player1->setEnabled(false);
            }
        }
        else
        {
            ui->label_user1->clear();
            ui->label_user1->setStyleSheet("");
            ui->label_user1_arrow->clear();
            ui->label_user1_arrow->setStyleSheet("");
            ui->player1->setEnabled(true);
            ui->player1->setChecked(false);
        }
    }
    changeInfo();
    if (isAccepted)
    {
        m_first_player = (Const::HostType)m_state[0];
        this->accept();
    }
}

void ChooseColorDialog::onDisconnected()
{
    m_is_disconnected = true;
    this->reject();
}

void ChooseColorDialog::onFirst(bool checked)
{
    if (checked)
    {
        m_state[0] = m_type;
        ui->label_user0->setText(m_username);
        ui->label_user0_arrow->setText("▼");
        if (m_state[1] == m_type)
        {
            m_state[1] = -1;
            ui->label_user1->clear();
            ui->label_user1_arrow->clear();
            ui->player1->setChecked(false);
        }
    }
    else
    {
        m_state[0] = -1;
        ui->label_user0->clear();
        ui->label_user0_arrow->clear();
    }
    changeInfo();
    sendData(false);
}

void ChooseColorDialog::onSecond(bool checked)
{
    if (checked)
    {
        m_state[1] = m_type;
        ui->label_user1->setText(m_username);
        ui->label_user1_arrow->setText("▼");
        if (m_state[0] == m_type)
        {
            m_state[0] = -1;
            ui->label_user0->clear();
            ui->label_user0_arrow->clear();
            ui->player0->setChecked(false);
        }
    }
    else
    {
        m_state[1] = -1;
        ui->label_user1->clear();
        ui->label_user1_arrow->clear();
    }
    changeInfo();
    sendData(false);
}

void ChooseColorDialog::onStart()
{
    m_first_player = (Const::HostType)m_state[0];
    sendData(true);
    this->accept();
}
