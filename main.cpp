#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>

class MainWindow : public QWidget {
public:
    MainWindow() {
        stack = new QStackedWidget(this);

        // =========================
        // LOGIN PAGE
        // =========================
        loginPage = new QWidget();
        auto *loginLayout = new QVBoxLayout(loginPage);

        loginPasswordEdit = new QLineEdit();
        loginPasswordEdit->setEchoMode(QLineEdit::Password);

        QPushButton *loginButton = new QPushButton("Login");
        errorLabel = new QLabel();
        errorLabel->setStyleSheet("color: red;");

        loginLayout->addWidget(new QLabel("Password:"));
        loginLayout->addWidget(loginPasswordEdit);
        loginLayout->addWidget(loginButton);
        loginLayout->addWidget(errorLabel);

        QPushButton *closeBtnLI = new QPushButton("Close Application");
        loginLayout->addWidget(closeBtnLI);

        // =========================
        // MAIN PAGE
        // =========================
        mainPage = new QWidget();
        mainLayout = new QVBoxLayout(mainPage);

        mainLayout->addWidget(new QLabel("Main Menu"));

        QPushButton *openAddBtn = new QPushButton("Add new password");
        mainLayout->addWidget(openAddBtn);

        scroll = new QScrollArea();
        scroll->setWidgetResizable(true);

        container = new QWidget();
        listLayout = new QVBoxLayout(container);

        scroll->setWidget(container);
        mainLayout->addWidget(scroll);

        QPushButton *closeBtn = new QPushButton("Close Application");
        mainLayout->addWidget(closeBtn);

        // =========================
        // INSPECT PAGE
        // =========================
        inspectPage = new QWidget();
        auto *inspectLayout = new QVBoxLayout(inspectPage);

        auto *grid = new QGridLayout();

        QLabel *websiteLbl = new QLabel("Website:");
        QLabel *usernameLbl = new QLabel("Username:");
        QLabel *passwordLbl = new QLabel("Password:");

        websiteEdit = new QLineEdit();
        usernameEdit = new QLineEdit();
        passwordEditInspect = new QLineEdit();
        passwordEditInspect->setEchoMode(QLineEdit::Password);

        QPushButton *showBtn = new QPushButton("Show");

        QPushButton *backBtn = new QPushButton("Back");
        QPushButton *deleteBtn = new QPushButton("Delete");
        QPushButton *editBtn = new QPushButton("Save Changes");

        grid->addWidget(websiteLbl, 0, 0);
        grid->addWidget(websiteEdit, 0, 1);

        grid->addWidget(usernameLbl, 1, 0);
        grid->addWidget(usernameEdit, 1, 1);

        grid->addWidget(passwordLbl, 2, 0);
        grid->addWidget(passwordEditInspect, 2, 1);
        grid->addWidget(showBtn, 2, 2);

        inspectLayout->addLayout(grid);
        inspectLayout->addWidget(backBtn);
        inspectLayout->addWidget(deleteBtn);
        inspectLayout->addWidget(editBtn);

        // =========================
        // ADD PAGE
        // =========================
        addPage = new QWidget();
        auto *addLayout = new QVBoxLayout(addPage);

        auto *addGrid = new QGridLayout();

        addWebsite = new QLineEdit();
        addUsername = new QLineEdit();
        addPassword = new QLineEdit();
        addPassword->setEchoMode(QLineEdit::Password);
        QPushButton *showAddBtn = new QPushButton("Show");

        addGrid->addWidget(new QLabel("Website:"), 0, 0);
        addGrid->addWidget(addWebsite, 0, 1);

        addGrid->addWidget(new QLabel("Username:"), 1, 0);
        addGrid->addWidget(addUsername, 1, 1);

        addGrid->addWidget(new QLabel("Password:"), 2, 0);
        addGrid->addWidget(addPassword, 2, 1);
        addGrid->addWidget(showAddBtn, 2, 2);

        QPushButton *saveBtn = new QPushButton("Save");
        QPushButton *cancelBtn = new QPushButton("Cancel");

        addLayout->addWidget(new QLabel("Add Entry"));
        addLayout->addLayout(addGrid);
        addLayout->addWidget(saveBtn);
        addLayout->addWidget(cancelBtn);

        // =========================
        // PAGE-STACK
        // =========================
        stack->addWidget(loginPage);
        stack->addWidget(mainPage);
        stack->addWidget(inspectPage);
        stack->addWidget(addPage);

        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->addWidget(stack);

        // =========================
        // LOGIN - CLOSE
        // =========================
        connect(loginButton, &QPushButton::clicked, this, [=]() {
            if (loginPasswordEdit->text() == "1234") {
                errorLabel->clear();
                loadMainPage();
                stack->setCurrentIndex(1);
            } else {
                errorLabel->setText("Wrong password!");
            }
        });

        connect(closeBtn, &QPushButton::clicked, this, [=]() {
            QApplication::quit();
        });
        connect(closeBtnLI, &QPushButton::clicked, this, [=]() {
            QApplication::quit();
        });

        // =========================
        // INSERT INTO DB
        // =========================
        connect(openAddBtn, &QPushButton::clicked, this, [=]() {
            stack->setCurrentIndex(3);
        });

        connect(saveBtn, &QPushButton::clicked, this, [=]() {

            if (addWebsite->text().trimmed().isEmpty() ||
                addUsername->text().trimmed().isEmpty() ||
                addPassword->text().trimmed().isEmpty()) {

                QMessageBox::warning(
                        this,
                        "Missing Data",
                        "Please fill out all fields."
                );
                return;
            }

            QSqlQuery query;
            query.prepare(
                    "INSERT INTO passwords (website, username, password) "
                    "VALUES (?, ?, ?)"
            );

            query.addBindValue(addWebsite->text());
            query.addBindValue(addUsername->text());
            query.addBindValue(addPassword->text());

            if (!query.exec()) {
                qDebug() << "Insert failed:" << query.lastError().text();
                return;
            }

            addWebsite->clear();
            addUsername->clear();
            addPassword->clear();

            loadMainPage();
            stack->setCurrentIndex(1);
        });

        connect(cancelBtn, &QPushButton::clicked, this, [=]() {
            stack->setCurrentIndex(1);
        });

        // =========================
        // INSPECT BACK
        // =========================
        connect(backBtn, &QPushButton::clicked, this, [=]() {
            loadMainPage();
            stack->setCurrentIndex(1);
        });

        // =========================
        // SHOW / HIDE PASSWORD
        // =========================
        connect(showBtn, &QPushButton::clicked, this, [=]() {
            if (passwordEditInspect->echoMode() == QLineEdit::Password) {
                passwordEditInspect->setEchoMode(QLineEdit::Normal);
                showBtn->setText("Hide");
            } else {
                passwordEditInspect->setEchoMode(QLineEdit::Password);
                showBtn->setText("Show");
            }
        });

        connect(showAddBtn, &QPushButton::clicked, this, [=]() {

            if (addPassword->echoMode() == QLineEdit::Password) {
                addPassword->setEchoMode(QLineEdit::Normal);
                showAddBtn->setText("Hide");
            } else {
                addPassword->setEchoMode(QLineEdit::Password);
                showAddBtn->setText("Show");
            }

        });

        // =========================
        // UPDATE DB
        // =========================
        connect(editBtn, &QPushButton::clicked, this, [=]() {
            QSqlQuery query;
            query.prepare(
                    "UPDATE passwords SET website=?, username=?, password=? WHERE id=?"
            );

            query.addBindValue(websiteEdit->text());
            query.addBindValue(usernameEdit->text());
            query.addBindValue(passwordEditInspect->text());
            query.addBindValue(currentId);

            if (!query.exec()) {
                qDebug() << "Update failed:" << query.lastError().text();
            }

            loadMainPage();
            stack->setCurrentIndex(1);
        });

        // ======================
        // DELETE DATA SET
        // ======================

        connect(deleteBtn, &QPushButton::clicked, this, [=]() {

            QMessageBox::StandardButton reply;

            reply = QMessageBox::question(
                    this,
                    "Delete Entry",
                    "Are you sure you want to delete this entry?",
                    QMessageBox::Yes | QMessageBox::No
            );

            if (reply == QMessageBox::No)
                return;

            QSqlQuery query;
            query.prepare("DELETE FROM passwords WHERE id = ?");
            query.addBindValue(currentId);

            if (!query.exec()) {
                qDebug() << "Delete failed:" << query.lastError().text();
                return;
            }

            loadMainPage();
            stack->setCurrentIndex(1);
        });
    }

private:
    void loadMainPage() {
        QLayoutItem *item;
        while ((item = listLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }

        QSqlQuery query("SELECT id, website, username, password FROM passwords");

        while (query.next()) {
            int id = query.value(0).toInt();
            QString website = query.value(1).toString();
            QString username = query.value(2).toString();
            QString password = query.value(3).toString();

            QPushButton *btn = new QPushButton(website);
            listLayout->addWidget(btn);

            connect(btn, &QPushButton::clicked, this, [=]() {
                currentId = id;

                websiteEdit->setText(website);
                usernameEdit->setText(username);
                passwordEditInspect->setText(password);
                passwordEditInspect->setEchoMode(QLineEdit::Password);

                stack->setCurrentIndex(2);
            });
        }
    }

private:
    QStackedWidget *stack;

    QWidget *loginPage;
    QWidget *mainPage;
    QWidget *inspectPage;
    QWidget *addPage;

    QVBoxLayout *mainLayout;
    QVBoxLayout *listLayout;

    QScrollArea *scroll;
    QWidget *container;

    // login
    QLineEdit *loginPasswordEdit;
    QLabel *errorLabel;

    // add
    QLineEdit *addWebsite;
    QLineEdit *addUsername;
    QLineEdit *addPassword;

    // inspect
    QLineEdit *websiteEdit;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEditInspect;

    int currentId = -1;
};

// =========================
// DB INIT
// =========================
bool initDatabase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("password_manager.db");

    if (!db.open()) {
        qDebug() << "DB error:" << db.lastError().text();
        return false;
    }

    QSqlQuery query;
    query.exec(
            "CREATE TABLE IF NOT EXISTS passwords ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "website TEXT,"
            "username TEXT,"
            "password TEXT"
            ")"
    );

    return true;
}

// =========================
// MAIN
// =========================
int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    if (!initDatabase()) return -1;

    MainWindow w;
    w.resize(450, 320);
    w.setWindowTitle("Password Manager");
    w.show();

    return a.exec();
}