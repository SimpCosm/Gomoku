#include "gomoku.h"
#include "ui_gomoku.h"
#include "connectdialog.h"
#include "choosecolordialog.h"
#include "waitdialog.h"

#include <QtDebug>
#include <QTime>
#include <QMessageBox>

Gomoku::Gomoku(QMainWindow *parent) :
    QMainWindow(parent),
    ui(new Ui::Gomoku),
    m_mode(Gomoku::Single),
    m_color(Piece::Black),
    m_is_started(false),
    m_is_blocked(false),
    m_can_undo(false),
    m_my_tot_time(0),
    m_opp_tot_time(0),
    m_username("Black"),
    m_opp_username("White"),
    m_type(Const::Server),
    m_is_connected(false),
    m_server(nullptr),
    m_connection(nullptr),
    m_thread(nullptr)
{
    ui->setupUi(this);

    ui->player0->setTitle(tr("Black"));
    ui->player1->setTitle(tr("White"));

    connect(ui->start, &QPushButton::clicked, this, &Gomoku::start);
    connect(ui->pause, &QPushButton::clicked, this, &Gomoku::pause);
    connect(ui->drop, &QPushButton::clicked, this, &Gomoku::drop);
    connect(ui->undo, &QPushButton::clicked, this, &Gomoku::undo);
    connect(ui->hint, &QPushButton::clicked, this, &Gomoku::hint);
    connect(ui->disconnect, &QPushButton::clicked, this, &Gomoku::disconnect);

    connect(ui->about, &QAction::triggered, this, &Gomoku::about);
    connect(ui->exit, &QAction::triggered, this, &Gomoku::exit);

    connect(ui->mode, SIGNAL(currentIndexChanged(int)), this, SLOT(setMode(int)));

    connect(ui->board, &Board::piecePlaced, this, &Gomoku::onMyMove);
    connect(ui->board, &Board::gameOver, this, &Gomoku::onGameOver);

    connect(&m_timer, &QTimer::timeout, this, &Gomoku::onTimeOut);

    initialize();
    onGameStartPrepare();
}

Gomoku::~Gomoku()
{
    delete ui;
}

void Gomoku::closeEvent(QCloseEvent* event)
{
    emit disconnected();
    qApp->exit(0);
}

void Gomoku::setBlock(bool isBlock)
{
    m_current_player = m_type;
    if (isBlock)
        m_current_player = m_type == Const::Server ? Const::Client : Const::Server;
    m_is_blocked = isBlock;
    ui->board->setBlock(isBlock);
    ui->hint->setEnabled(!isBlock);
}

void Gomoku::initialize()
{
    ui->label_info->setText(tr("Default Single mode, you can also choose network or AI mode"));
    setBlock(true);
    ui->board->setHidden(false);

    ui->drop->setEnabled(false);
    ui->start->setEnabled(false);
    ui->pause->setEnabled(false);

    ui->lcd_left0->display("00");
    ui->lcd_left1->display("00");
    ui->lcd_used0->display("00:00");
    ui->lcd_used1->display("00:00");
}

void Gomoku::setMode(int mode)
{
    onGameStartPrepare();

    if (QMessageBox::question(this, tr("Ret Mode"), tr("Do you really want to change the game mode?")) == QMessageBox::Yes)
    {
        ConnectDialog *dialog;
        switch(mode)
        {
            case 0:
                m_mode = Gomoku::Single;
                ui->player0->setTitle(tr("Black"));
                ui->player1->setTitle(tr("White"));
                break;
            case 1:
                m_mode = Gomoku::Network;
                dialog = new ConnectDialog();
                if (dialog->exec() != QDialog::Accepted)
                {
                    dialog->deleteLater();
                    break;
                }
                m_ip = dialog->getIP();
                m_port = dialog->getPort();
                m_type = dialog->getType();
                m_username = dialog->getUsername();
                if (m_type == Const::Server)
                {
                    this->setWindowTitle(tr("Gomoku - Server"));
                    ui->label_info->setText(tr("Network Mode: Waiting for connection..."));
                    ui->player1->setEnabled(false);
                    ui->player0->setTitle(m_username);
                    ui->player1->setTitle(tr("Player2"));
                    m_server = new Server(m_ip, m_port, this);
                    connect(m_server, &Server::newConnection, this, &Gomoku::createServerConnection);
                }
                else if (m_type == Const::Client)
                {
                    this->setWindowTitle(tr("Gomoku - Client"));
                    ui->player0->setEnabled(false);
                    ui->player0->setTitle(tr("Player1"));
                    ui->player1->setTitle(m_username);
                    createClientConnection();
                }
                break;
            case 2:
                m_mode = Gomoku::AI;
                ui->player0->setTitle(tr("You"));
                ui->player1->setTitle(tr("AI"));
                break;
            default:
                break;
        }
        ui->board->clear();
    }
}

void Gomoku::createServerConnection(ConnectionThread* thread)
{
    if (m_is_connected) return;
    m_is_connected = true;
    m_thread = thread;
    m_thread->setGreetingMessage(m_username);
    connect(m_thread, &ConnectionThread::connectionReady, this, &Gomoku::onConnectionReady);
    m_thread->start();
}

void Gomoku::createClientConnection()
{
    m_thread = new ConnectionThread(m_type, m_ip, m_port, this);
    m_thread->setGreetingMessage(m_username);
    connect(m_thread, &ConnectionThread::connectionReady, this, &Gomoku::onConnectionReady);
    m_thread->start();
}

void Gomoku::onConnectionReady(const QString& oppUsername)
{
    m_connection = m_thread->getConnection();

    connect(this, &Gomoku::moveSent, m_connection, &Connection::sendMove);
    connect(this, &Gomoku::messageSent, m_connection, &Connection::sendMessage);
    connect(this, &Gomoku::disconnected, m_connection, &Connection::close);

    connect(m_connection, &Connection::gameStartedReceived, this, &Gomoku::onChooseColor);
    connect(m_connection, &Connection::pauseReceived, this, &Gomoku::onPause);
    connect(m_connection, &Connection::continueReceived, this, &Gomoku::onContinue);

    connect(m_connection, &Connection::moveReceived, this, &Gomoku::onOpponentMove);
    connect(m_connection, &Connection::opponentUndoRequest, this, &Gomoku::onOpponentUndoRequest);
    connect(m_connection, &Connection::opponentDropReceived, this, &Gomoku::onOpponentDrop);
    connect(m_connection, &Connection::disconnected, this, &Gomoku::onDisConnected);

    ui->status->setText(tr("Connected"));
    ui->status->setStyleSheet("color:green");
    ui->disconnect->setEnabled(true);

    if (m_type == Const::Server)
    {
        ui->player1->setTitle(oppUsername);
        ui->ip0->setText(QString("%1:%2").arg(Const::getLocalIP()).arg(m_connection->localPort()));
        ui->ip1->setText(QString("%1:%2").arg(m_connection->peerAddress().toString()).arg(m_connection->peerPort()));
    }
    else if (m_type == Const::Client)
    {
        ui->player0->setTitle(oppUsername);
        ui->ip0->setText(QString("%1:%2").arg(m_ip).arg(m_port));
        ui->ip1->setText(QString("%1:%2").arg(Const::getLocalIP()).arg(m_connection->localPort()));
    }
    onGameStartPrepare();
}

void Gomoku::onGameStartPrepare()
{
    m_timer.stop();
    this->setBlock(true);
    m_is_started = false;
    ui->start->setText(tr("&Start"));
    ui->pause->setEnabled(false);
    ui->undo->setEnabled(false);
    ui->drop->setEnabled(false);

    m_is_started = false;
    m_is_blocked = false;
    m_can_undo = false;
    m_my_tot_time = 0;
    m_opp_tot_time = 0;

    switch (m_mode) {
    case Gomoku::AI:
    case Gomoku::Single:
        ui->start->setEnabled(true);
        break;
    case Gomoku::Network:
        if (m_type == Const::Server)
        {
            ui->start->setEnabled(true);
            ui->label_info->setText(tr("Press the start button to start a new game."));
        }
        else if (m_type == Const::Client)
            ui->label_info->setText(tr("Waiting for the server to start..."));
    default:
        break;
    }
}

void Gomoku::onGameOver(Piece::PieceColor color)
{
    if (color == Piece::Transparent)
        QMessageBox::information(this, tr("DRAW"), tr("2333333333..."));
    else
        switch (m_mode)
        {
        case Gomoku::AI:
            if (color == Piece::Black)
                QMessageBox::information(this, tr("WIN!"), tr("You win the game :-)"));
            else
                QMessageBox::information(this, tr("LOSE"), tr("You lose the game :-("));
            break;
        case Gomoku::Network:
            if (color == ui->board->getColor())
                QMessageBox::information(this, tr("WIN!"), tr("You win the game :-)"));
            else
                QMessageBox::information(this, tr("LOSE"), tr("You lose the game :-("));
            break;
        case Gomoku::Single:
            if (color == Piece::White)
                QMessageBox::information(this, tr("WIN!"), tr("White win the game :-)"));
            else if (color == Piece::Black)
                QMessageBox::information(this, tr("LOSE"), tr("Black win the game :-)"));
            break;
        default:
            break;
        }

    onGameStartPrepare();
}

void Gomoku::onDisConnected()
{
    m_is_connected = false;
    m_timer.stop();
    ui->status->setText(tr("Disconnected"));
    ui->status->setStyleSheet("color:red;");
    ui->disconnect->setEnabled(false);

    m_thread->deleteLater();

    QString info = tr("Connection has been disconnected!");
    if (m_type == Const::Client)
        info += tr("\nThe game will restart.");
    QMessageBox::information(this, tr("Disconnected"), info);
    if (m_type == Const::Server)
        initialize();
    else if (m_type == Const::Client)
        qApp->exit(233);

}

void Gomoku::onTimeOut()
{
    m_time_left--;
    int x, y;
    if ((m_mode == Gomoku::Network && m_is_blocked) ||
        (m_mode == Gomoku::Single && ui->board->getColor() == Piece::White) ||
        (m_mode == Gomoku::AI && ui->board->getColor() == Piece::White))
    {
        x = ++m_opp_tot_time;
        y = m_my_tot_time;
    }
    else
    {
        x = ++m_my_tot_time;
        y = m_opp_tot_time;
    }
    QString time = QString::number(m_time_left);
    if (m_time_left < 10) time = "0" + time;
    if ((m_mode == Gomoku::Network && m_current_player == Const::Server) ||
        (m_mode == Gomoku::Single && ui->board->getColor() == Piece::Black) ||
        (m_mode == Gomoku::AI && ui->board->getColor() == Piece::Black))
    {
        ui->lcd_used0->display(QTime(0, 0, 0).addSecs(x).toString("mm:ss"));
        ui->lcd_left0->display(time);
        ui->lcd_used1->display(QTime(0, 0, 0).addSecs(y).toString("mm:ss"));
        ui->lcd_left1->display(Const::TIME_LIMIT);
        ui->lcd_left1->setStyleSheet("");
        if (m_time_left <= 10)
            ui->lcd_left0->setStyleSheet("color:red;");
        else
            ui->lcd_left0->setStyleSheet("");
    }
    else
    {
        ui->lcd_used0->display(QTime(0, 0, 0).addSecs(y).toString("mm:ss"));
        ui->lcd_left0->display(Const::TIME_LIMIT);
        ui->lcd_left0->setStyleSheet("");
        ui->lcd_used1->display(QTime(0, 0, 0).addSecs(x).toString("mm:ss"));
        ui->lcd_left1->display(time);
        if (m_time_left <= 10)
            ui->lcd_left1->setStyleSheet("color:red;");
        else
            ui->lcd_left1->setStyleSheet("");
    }

    if (!m_time_left)
    {
        m_can_undo = false;
        nextMove();
    }
}

void Gomoku::nextMove()
{
    if (m_is_blocked)
        onOpponentMove(-1, -1, Piece::Transparent);
    else
        onMyMove(-1, -1, Piece::Transparent);
}

void Gomoku::onChooseColor()
{
    ui->board->clear();
    ui->label_info->setText(tr("Waiting for choosing color..."));

    ChooseColorDialog dialog(m_username, m_type, this);
    connect(m_connection, &Connection::prepareStateReceived, &dialog, &ChooseColorDialog::onUpdateState);
    connect(m_connection, &QTcpSocket::disconnected, &dialog, &ChooseColorDialog::onDisconnected);
    connect(&dialog, &ChooseColorDialog::prepareStateChanged, m_connection, &Connection::sendPrepareState);

    if (dialog.exec() != QDialog::Accepted)
    {
        if (dialog.isDisconnected())
            return;
        else
            this->close();
    }

    m_my_tot_time = m_opp_tot_time = 0;
    m_time_left = Const::TIME_LIMIT + 1;
    if (dialog.firstPlayer() == m_type)
    {
        setBlock(false);
        m_my_tot_time--;
        ui->board->setColor(Piece::Black);
        ui->label_info->setText(tr("Please select a position to place the pieces."));
    }
    else
    {
        setBlock(true);
        m_opp_tot_time--;
        ui->board->setColor(Piece::White);
        ui->label_info->setText(tr("Waiting for the opponent to place..."));
    }
    onTimeOut();
    m_timer.start(1000);

    ui->start->setText(tr("&Continue"));
    ui->start->setEnabled(false);
    ui->pause->setEnabled(true);
    ui->drop->setEnabled(true);
    m_is_started = true;
}

void Gomoku::onPause()
{
    m_timer.stop();
    ui->label_info->setText(tr("Pausing..."));
    ui->pause->setEnabled(false);

    ui->hint->setEnabled(false);
    ui->undo->setEnabled(false);
    ui->drop->setEnabled(false);

    ui->board->setBlock(true);
    ui->board->setHidden(true);
}

void Gomoku::onContinue()
{
    m_timer.start(1000);
    if (!m_is_blocked)
        ui->label_info->setText(tr("Please select a position to place the pieces."));
    else
        ui->label_info->setText(tr("Waiting for the opponent to place..."));
    ui->pause->setEnabled(true);

    ui->hint->setEnabled(!m_is_blocked);
    ui->undo->setEnabled(ui->board->getMyPieces() && m_can_undo);
    ui->drop->setEnabled(true);

    ui->board->setBlock(m_is_blocked);
    ui->board->setHidden(false);
}

void Gomoku::onOpponentMove(int row, int col, Piece::PieceColor color)
{
    setBlock(false);
    m_time_left = Const::TIME_LIMIT + 1;
    m_my_tot_time--;
    m_timer.start(1000);
    onTimeOut();

    ui->label_info->setText(tr("Please select a position to place the pieces."));
    if (row < 0) m_can_undo = false;
    ui->undo->setEnabled(ui->board->getMyPieces() && m_can_undo);
    ui->board->placePiece(row, col, (Piece::PieceColor)color);
}

void Gomoku::onMyMove(int row, int col, Piece::PieceColor color)
{
    switch (m_mode) {
    case Gomoku::Single:
        if (!m_is_started) return;
        ui->board->revertColor();
        m_time_left = Const::TIME_LIMIT + 1;
        m_timer.start(1000);
        onTimeOut();
        break;
    case Gomoku::Network:
        m_can_undo = row > 0;
        ui->undo->setEnabled(ui->board->getMyPieces() && m_can_undo);
        ui->label_info->setText(tr("Waiting for the opponent to place..."));
        setBlock(true);

        m_time_left = Const::TIME_LIMIT + 1;
        m_opp_tot_time--;
        m_timer.start(1000);
        onTimeOut();
        emit moveSent(row, col, color);
        break;
    case Gomoku::AI:
        if (!m_is_started) return;
        ui->board->revertColor();
        ui->board->ai(ui->board->getColor());
        ui->board->revertColor();
        m_time_left = Const::TIME_LIMIT + 1;
        m_timer.start(1000);
        onTimeOut();
        break;
    default:
        break;
    }
}

void Gomoku::onOpponentUndoRequest()
{
    int undoStep = m_is_blocked ? 2 : 1;
    m_timer.stop();
    ui->board->setHidden(true);
    if (QMessageBox::question(this, tr("Undo Request"), QString(tr("Do you allow the opponent to move back before %1 step%2")).arg(undoStep).arg(undoStep == 1 ? "" : "s")) == QMessageBox::Yes)
    {
        emit messageSent("allowundo");
        ui->board->undo(undoStep);
        ui->undo->setEnabled(ui->board->getMyPieces());
        if (undoStep == 1)
            nextMove();

        m_time_left = Const::TIME_LIMIT + 1;
        if (m_is_blocked)
            m_opp_tot_time--;
        else
            m_opp_tot_time--;
        m_timer.start(1000);
        onTimeOut();
    }
    else
        emit messageSent("disallowundo");
    ui->board->setHidden(false);
    m_timer.start(1000);
}

void Gomoku::onOpponentDrop()
{
    m_timer.stop();
    QMessageBox::information(this, tr("WIN"), tr("The opponent drop the game :)"));
    onGameStartPrepare();
}

void Gomoku::start()
{
    switch (m_mode) {
    case Gomoku::Single:
        if (m_is_started)
        {
            ui->start->setEnabled(false);
            onContinue();
        }
        else
        {
            m_my_tot_time = m_opp_tot_time = 0;
            m_time_left = Const::TIME_LIMIT + 1;
            setBlock(false);
            m_opp_tot_time--;
            onTimeOut();
            m_timer.start(1000);
            ui->board->clear();
            ui->board->setColor(Piece::Black);
            ui->start->setText(tr("&Continue"));
            ui->start->setEnabled(false);
            ui->pause->setEnabled(true);
            ui->drop->setEnabled(true);
            ui->label_info->setText(tr("Please select a position to place the pieces."));
            m_is_started = true;
        }
        break;
    case Gomoku::Network:
        if (m_is_started)
        {
            emit messageSent("continue");
            ui->start->setEnabled(false);
            onContinue();
        }
        else if (m_type == Const::Server)
        {
            emit messageSent("start");
            onChooseColor();
        }
    case Gomoku::AI:
        if (m_is_started)
        {
            ui->start->setEnabled(false);
            onContinue();
        }
        else
        {
            m_my_tot_time = m_opp_tot_time = 0;
            m_time_left = Const::TIME_LIMIT + 1;
            setBlock(false);
            m_opp_tot_time--;
            onTimeOut();
            m_timer.start(1000);
            ui->board->clear();
            ui->board->setColor(Piece::Black);
            ui->start->setText(tr("&Continue"));
            ui->start->setEnabled(false);
            ui->pause->setEnabled(true);
            ui->drop->setEnabled(true);
            ui->label_info->setText(tr("Please select a position to place the pieces."));
            m_is_started = true;
        }
        break;
    default:
        break;
    }
}

void Gomoku::pause()
{
    emit messageSent("pause");
    ui->start->setEnabled(true);
    onPause();
}


void Gomoku::undo()
{
    int undoStep = 1;
    WaitDialog *dialog;
    if (QMessageBox::question(this, tr("Undo Step"), tr("Do you want to undo?")) == QMessageBox::Yes)
    {
        switch (m_mode)
        {
        case Gomoku::AI:
        case Gomoku::Single:
            undoStep = 1;
            ui->board->undo(undoStep);
            ui->board->revertColor();
            break;
        case Gomoku::Network:
            undoStep = m_is_blocked ? 1 : 2;
            m_timer.stop();
            ui->board->setHidden(true);
            emit messageSent("undo");

            dialog = new WaitDialog(this);
            connect(m_connection, &Connection::undoAllowed, dialog, &WaitDialog::onUndoAllowed);
            connect(m_connection, &Connection::undoDisallowed, dialog, &WaitDialog::onUndoDisallowed);
            connect(dialog, &WaitDialog::undoDisallowed, this, [this]()
            {
                m_timer.start(1000);;
                ui->board->setHidden(false);
            });
            connect(dialog, &WaitDialog::undoAllowed, this, [&, this]()
            {
                ui->board->undo(undoStep);
                ui->undo->setEnabled(ui->board->getMyPieces());

                m_time_left = Const::TIME_LIMIT + 1;
                if (m_is_blocked)
                    m_opp_tot_time--;
                else
                    m_opp_tot_time--;
                m_timer.start(1000);
                ui->board->setHidden(false);
                onTimeOut();
            });
            dialog->exec();
            break;
        default:
            break;
        }
    }
}

void Gomoku::hint()
{
    ui->board->showHint();
}

void Gomoku::drop()
{
    if (QMessageBox::question(this, tr("Drop Game"), tr("Do you really want to drop the game?")) == QMessageBox::Yes)
    {
        m_timer.stop();
        emit messageSent("drop");
        QMessageBox::information(this, tr("LOSE"), tr("You drop the game :("));
        onGameStartPrepare();
    }
}

void Gomoku::disconnect()
{
    if (QMessageBox::question(this, tr("Disconnect"), tr("Do you really want to disconnect?")) == QMessageBox::Yes)
        emit disconnected();
}

void Gomoku::about()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("About Gomoku"));
    msgBox.setIconPixmap(QPixmap(":/icons/gomoku.ico"));
    msgBox.setText(QString(tr(
                      "<h2>Gomoku Cosmos 1.00<br/></h2>"
                      "<p>PKU Embeded System Project</p>"
                      "<p>Based on Qt 5.10.0<br/></p>"
                      "<p>Version: %1</p>"
                      "<p>Built time: %2 - %3<br/></p>"
                      "<p>Copyright © 2018 Houmin Wei, Peking University. All Right Reserved.<br/></p>"
                      "<p>Project Index: <a href=\"https://github.com/SimpCosm/Gomoku\">https://github.com/SimpCosm/Gomoku</a></p>"
                      )).arg("1.0.0").arg(__DATE__).arg(__TIME__));
    msgBox.exec();
}

void Gomoku::exit()
{
    emit disconnected();
    qApp->exit(0);
}
