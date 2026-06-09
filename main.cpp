#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class MainWindow : public QWidget {
public:
    MainWindow() {
        // STACK
        auto *stack = new QStackedWidget(this);

        // LOGIN PAGE
        QWidget *loginPage = new QWidget();
        auto *loginLayout = new QVBoxLayout(loginPage);

        QLabel *label = new QLabel("Password:");
        QLineEdit *password = new QLineEdit();
        password->setEchoMode(QLineEdit::Password);

        QPushButton *loginButton = new QPushButton("Login");

        QLabel *errorLabel = new QLabel("");
        errorLabel->setStyleSheet("color: red;");

        loginLayout->addWidget(label);
        loginLayout->addWidget(password);
        loginLayout->addWidget(loginButton);
        loginLayout->addWidget(errorLabel);

        // MAIN MENU PAGE
        QWidget *mainPage = new QWidget();
        auto *mainLayout = new QVBoxLayout(mainPage);

        QLabel *welcome = new QLabel("Main Menu!");
        mainLayout->addWidget(welcome);

        // PAGES
        stack->addWidget(loginPage);  // index 0
        stack->addWidget(mainPage);   // index 1

        // LAYOUT
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->addWidget(stack);

        // LOGIN LOGIC
        connect(loginButton, &QPushButton::clicked, this, [=]() {
            if (password->text() == "1234") {
                errorLabel->setText("");
                stack->setCurrentIndex(1); // Switch to Main Menu
            } else {
                errorLabel->setText("Wrong password!");
            }
        });
    }
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    MainWindow w;
    w.setWindowTitle("Login");
    w.resize(300, 150);
    w.show();

    return a.exec();
}