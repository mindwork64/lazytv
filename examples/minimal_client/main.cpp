#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include <lazytv/client.hpp>

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);

  if (argc < 2) {
    qWarning() << "Usage: minimal_client <tv-ip>";
    return 1;
  }

  const QString ip = QString::fromUtf8(argv[1]);
  lazytv::Client client(ip);

  QObject::connect(&client, &lazytv::Client::pairingKeyResult, [](bool ok) {
    qInfo() << "Pairing key request:" << (ok ? "OK" : "FAILED");
    if (!ok)
      QCoreApplication::exit(2);
  });

  QObject::connect(&client, &lazytv::Client::pairingConfirmResult,
                   [&](const QString &session) {
                     if (session.isEmpty()) {
                       qWarning() << "Pairing rejected";
                       QCoreApplication::exit(3);
                       return;
                     }
                     qInfo() << "Paired. Session:" << session;
                     qInfo() << "Sending POWER in 500 ms...";
                     QTimer::singleShot(500, [&] {
                       client.sendCommand(lazytv::Client::Command::Power);
                     });
                   });

  QObject::connect(&client, &lazytv::Client::commandResult, [](bool ok) {
    qInfo() << "Command result:" << (ok ? "OK" : "FAILED");
    QCoreApplication::exit(ok ? 0 : 4);
  });

  qInfo() << "Requesting pairing key from" << ip;
  client.requestPairingKey();

  return app.exec();
}