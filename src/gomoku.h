#ifndef GOMOKU_H
#define GOMOKU_H

#include "const.h"
#include "piece.h"
#include "server.h"
#include "connectionthread.h"

#include <QMainWindow>
#include <QTimer>

namespace Ui {
class Gomoku;
}

class Gomoku : public QMainWindow
{
    Q_OBJECT

public:
    enum Mode
    {
        Single = 0,
        Network,
        AI
    };

    explicit Gomoku(QMainWindow *parent = 0);
    ~Gomoku();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void onGameStartPrepare();
    void onConnectionReady(const QString& oppUsername);

    void onDisConnected();
    void onTimeOut();
    void onChooseColor();
    void onPause();
    void onContinue();

    void onMyMove(int row, int col, Piece::PieceColor color);
    void onOpponentMove(int row, int col, Piece::PieceColor color);
    void onOpponentUndoRequest();
    void onOpponentDrop();

    void setMode(int mode);
    void onGameOver(Piece::PieceColor color);

    void start();
    void pause();
    void drop();
    void undo();
    void hint();
    void disconnect();
    void about();
    void exit();

signals:
    void disconnected();
    void gameStarted();
    void messageSent(const QString& message);
    void moveSent(int row, int col, Piece::PieceColor color);

private:
    Ui::Gomoku *ui;
    Mode m_mode;
    Piece::PieceColor m_color;
    bool m_is_started;
    bool m_is_blocked;
    bool m_can_undo;

    // time about
    QTimer  m_timer;
    int m_time_left;
    int m_my_tot_time;
    int m_opp_tot_time;

    // network about
    QString m_username;
    QString m_opp_username;
    Const::HostType m_type, m_current_player;
    QString m_ip;
    int m_port;
    bool m_is_connected;

    Server* m_server;
    Connection* m_connection;
    ConnectionThread* m_thread;

    void setBlock(bool isBlock);
    void initialize();
    void createServerConnection(ConnectionThread *thread);
    void createClientConnection();
    void nextMove();
};

#endif
