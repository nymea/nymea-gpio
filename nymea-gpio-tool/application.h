#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include <QCoreApplication>

class Application : public QCoreApplication
{
    Q_OBJECT
public:
    explicit Application(int &argc, char **argv);

};

#endif // APPLICATION_H
