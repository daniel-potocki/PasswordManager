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
#include <QInputDialog>
#include <QByteArray>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <argon2.h>
#include <QIcon>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <filesystem>

static const char *MASTER_TABLE = "master_auth";
static const char *PASSWORD_TABLE = "passwords";
static const int KEY_SIZE = 32;
static const int IV_SIZE = 12;
static const int TAG_SIZE = 16;
static const int SALT_SIZE = 16;

static QByteArray randomBytes(int size) {
    QByteArray out(size, Qt::Uninitialized);
    if (RAND_bytes(reinterpret_cast<unsigned char*>(out.data()), size) != 1) {
        return {};
    }
    return out;
}

static QByteArray hexEncode(const QByteArray &data) {
    return data.toHex();
}

static QByteArray hexDecode(const QString &text) {
    return QByteArray::fromHex(text.toUtf8());
}

static QByteArray deriveKey(const QString &masterPassword, const QByteArray &salt) {
    QByteArray key(KEY_SIZE, Qt::Uninitialized);

    const QByteArray passBytes = masterPassword.toUtf8();
    const int rc = argon2id_hash_raw(
            3,
            1 << 16,
            1,
            passBytes.constData(),
            passBytes.size(),
            salt.constData(),
            salt.size(),
            key.data(),
            key.size()
    );

    if (rc != ARGON2_OK) {
        return {};
    }

    return key;
}

static QByteArray encryptText(const QString &plainText, const QByteArray &key, QByteArray &ivOut, QByteArray &tagOut) {
    QByteArray plaintext = plainText.toUtf8();
    QByteArray ciphertext(plaintext.size() + 32, Qt::Uninitialized);

    ivOut = randomBytes(IV_SIZE);
    if (ivOut.isEmpty()) return {};

    tagOut = QByteArray(TAG_SIZE, Qt::Uninitialized);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};

    int len = 0;
    int ciphertextLen = 0;

    bool ok = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1;
    ok = ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr) == 1;
    ok = ok && EVP_EncryptInit_ex(ctx, nullptr, nullptr,
                                  reinterpret_cast<const unsigned char*>(key.constData()),
                                  reinterpret_cast<const unsigned char*>(ivOut.constData())) == 1;

    if (ok && EVP_EncryptUpdate(ctx,
                                reinterpret_cast<unsigned char*>(ciphertext.data()),
                                &len,
                                reinterpret_cast<const unsigned char*>(plaintext.constData()),
                                plaintext.size()) == 1) {
        ciphertextLen = len;
        ok = EVP_EncryptFinal_ex(ctx,
                                 reinterpret_cast<unsigned char*>(ciphertext.data()) + len,
                                 &len) == 1;
        ciphertextLen += len;
        ok = ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tagOut.data()) == 1;
    } else {
        ok = false;
    }

    EVP_CIPHER_CTX_free(ctx);

    if (!ok) return {};
    ciphertext.resize(ciphertextLen);
    return ciphertext;
}

static QString decryptText(const QByteArray &ciphertext, const QByteArray &key, const QByteArray &iv, const QByteArray &tag) {
    QByteArray plaintext(ciphertext.size() + 32, Qt::Uninitialized);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};

    int len = 0;
    int plaintextLen = 0;
    bool ok = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1;
    ok = ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr) == 1;
    ok = ok && EVP_DecryptInit_ex(ctx, nullptr, nullptr,
                                  reinterpret_cast<const unsigned char*>(key.constData()),
                                  reinterpret_cast<const unsigned char*>(iv.constData())) == 1;

    if (ok && EVP_DecryptUpdate(ctx,
                                reinterpret_cast<unsigned char*>(plaintext.data()),
                                &len,
                                reinterpret_cast<const unsigned char*>(ciphertext.constData()),
                                ciphertext.size()) == 1) {
        plaintextLen = len;
        ok = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE,
                                 const_cast<char*>(tag.constData())) == 1;
        if (ok) {
            ok = EVP_DecryptFinal_ex(ctx,
                                     reinterpret_cast<unsigned char*>(plaintext.data()) + len,
                                     &len) == 1;
            plaintextLen += len;
        }
    } else {
        ok = false;
    }

    EVP_CIPHER_CTX_free(ctx);

    if (!ok) return {};
    plaintext.resize(plaintextLen);
    return QString::fromUtf8(plaintext);
}

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

        QLabel *loginLabel = new QLabel("Type in master password:");
        loginLabel->setAlignment(Qt::AlignCenter);
        loginLayout->addWidget(loginLabel, 0, Qt::AlignHCenter);        loginLayout->addWidget(loginPasswordEdit);
        loginLayout->addWidget(loginButton);
        loginLayout->addWidget(errorLabel);

        QPushButton *closeBtnLI = new QPushButton("Close Application");
        loginLayout->addWidget(closeBtnLI);

        // =========================
        // MAIN PAGE
        // =========================
        mainPage = new QWidget();
        mainLayout = new QVBoxLayout(mainPage);

        QLabel *mainMenuLabel = new QLabel("Main Menu");
        mainMenuLabel->setAlignment(Qt::AlignHCenter);
        mainLayout->addWidget(mainMenuLabel);

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

        QLabel *addEntryLabel = new QLabel("Add Entry");
        addEntryLabel->setAlignment(Qt::AlignHCenter);
        addLayout->addWidget(addEntryLabel);
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
        // FIRST START / LOGIN
        // =========================
        ensureMasterPasswordExists();

        connect(loginButton, &QPushButton::clicked, this, [=]() {
            if (verifyMasterPassword(loginPasswordEdit->text())) {
                errorLabel->clear();
                currentMasterPassword = loginPasswordEdit->text();
                currentMasterKey = deriveCurrentKey();
                if (currentMasterKey.isEmpty()) {
                    errorLabel->setText("Key derivation failed!");
                    return;
                }
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
            if (currentMasterKey.isEmpty()) return;
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

            if (currentMasterKey.isEmpty()) {
                QMessageBox::warning(this, "Error", "No active master key.");
                return;
            }

            QByteArray iv, tag;
            QByteArray cipher = encryptText(addPassword->text(), currentMasterKey, iv, tag);
            if (cipher.isEmpty()) {
                QMessageBox::warning(this, "Error", "Encryption failed.");
                return;
            }

            QSqlQuery query;
            query.prepare(
                    "INSERT INTO passwords (website, username, password, iv, tag) "
                    "VALUES (?, ?, ?, ?, ?)"
            );

            query.addBindValue(addWebsite->text());
            query.addBindValue(addUsername->text());
            query.addBindValue(QString::fromUtf8(cipher.toBase64()));
            query.addBindValue(QString::fromUtf8(iv.toBase64()));
            query.addBindValue(QString::fromUtf8(tag.toBase64()));

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
            if (currentMasterKey.isEmpty()) return;

            QByteArray iv, tag;
            QByteArray cipher = encryptText(passwordEditInspect->text(), currentMasterKey, iv, tag);
            if (cipher.isEmpty()) {
                QMessageBox::warning(this, "Error", "Encryption failed.");
                return;
            }

            QSqlQuery query;
            query.prepare(
                    "UPDATE passwords SET website=?, username=?, password=?, iv=?, tag=? WHERE id=?"
            );

            query.addBindValue(websiteEdit->text());
            query.addBindValue(usernameEdit->text());
            query.addBindValue(QString::fromUtf8(cipher.toBase64()));
            query.addBindValue(QString::fromUtf8(iv.toBase64()));
            query.addBindValue(QString::fromUtf8(tag.toBase64()));
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

        stack->setCurrentIndex(0);
    }

private:
    void ensureMasterPasswordExists() {
        QSqlQuery query;
        query.exec("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='master_auth'");

        QSqlQuery check;
        check.exec("SELECT COUNT(*) FROM master_auth");
        bool hasMaster = false;
        if (check.next()) {
            hasMaster = check.value(0).toInt() > 0;
        }

        if (!hasMaster) {
            bool ok = false;
            QString pass1 = QInputDialog::getText(
                    this,
                    "Set Master Password",
                    "Create your master password:",
                    QLineEdit::Password,
                    "",
                    &ok
            );
            if (!ok || pass1.isEmpty()) {
                QApplication::quit();
                return;
            }

            QString pass2 = QInputDialog::getText(
                    this,
                    "Set Master Password",
                    "Confirm your master password:",
                    QLineEdit::Password,
                    "",
                    &ok
            );
            if (!ok || pass2.isEmpty() || pass1 != pass2) {
                QMessageBox::warning(this, "Error", "Passwords do not match.");
                QApplication::quit();
                return;
            }

            QByteArray salt = randomBytes(SALT_SIZE);
            QByteArray key = deriveKey(pass1, salt);
            if (key.isEmpty()) {
                QMessageBox::warning(this, "Error", "Could not create master password.");
                QApplication::quit();
                return;
            }

            QByteArray hash = QCryptographicHash::hash(pass1.toUtf8() + salt, QCryptographicHash::Sha256).toHex();

            QSqlQuery insert;
            insert.prepare("INSERT INTO master_auth (salt, password_hash) VALUES (?, ?)");
            insert.addBindValue(QString::fromUtf8(salt.toBase64()));
            insert.addBindValue(QString::fromUtf8(hash));

            if (!insert.exec()) {
                QMessageBox::warning(this, "Error", "Could not store master password.");
                QApplication::quit();
                return;
            }

            QMessageBox::information(this, "Done", "Master password created. Please log in.");
        }
    }

    bool verifyMasterPassword(const QString &password) {
        QSqlQuery query("SELECT salt, password_hash FROM master_auth LIMIT 1");
        if (!query.next()) return false;

        QByteArray salt = QByteArray::fromBase64(query.value(0).toString().toUtf8());
        QByteArray storedHash = query.value(1).toString().toUtf8();

        QByteArray candidate = QCryptographicHash::hash(password.toUtf8() + salt, QCryptographicHash::Sha256).toHex();
        return candidate == storedHash;
    }

    QByteArray deriveCurrentKey() const {
        QSqlQuery query("SELECT salt FROM master_auth LIMIT 1");
        if (!query.next()) return {};
        QByteArray salt = QByteArray::fromBase64(query.value(0).toString().toUtf8());
        return deriveKey(currentMasterPassword, salt);
    }

    void loadMainPage() {
        QLayoutItem *item;
        while ((item = listLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }

        if (currentMasterKey.isEmpty()) return;

        QSqlQuery query("SELECT id, website, username, password, iv, tag FROM passwords");

        while (query.next()) {
            int id = query.value(0).toInt();
            QString website = query.value(1).toString();
            QString username = query.value(2).toString();
            QByteArray cipher = QByteArray::fromBase64(query.value(3).toString().toUtf8());
            QByteArray iv = QByteArray::fromBase64(query.value(4).toString().toUtf8());
            QByteArray tag = QByteArray::fromBase64(query.value(5).toString().toUtf8());
            QString password = decryptText(cipher, currentMasterKey, iv, tag);

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
    QString currentMasterPassword;
    QByteArray currentMasterKey;
};

// =========================
// DB INIT
// =========================
bool initDatabase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QByteArray dataPathUtf8 = dataPath.toUtf8();
    std::filesystem::create_directories(std::string(dataPathUtf8.constData()));

    QString dbPath = dataPath + "/password_manager.db";
    qDebug() << "DB path:" << dbPath;

    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "DB error:" << db.lastError().text();
        return false;
    }

    QSqlQuery query;
    query.exec(
            "CREATE TABLE IF NOT EXISTS master_auth ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "salt TEXT NOT NULL,"
            "password_hash TEXT NOT NULL"
            ")"
    );

    query.exec(
            "CREATE TABLE IF NOT EXISTS passwords ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "website TEXT,"
            "username TEXT,"
            "password TEXT,"
            "iv TEXT,"
            "tag TEXT"
            ")"
    );

    return true;
}

// =========================
// MAIN
// =========================
int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/app.png"));

    if (!initDatabase()) return -1;

    MainWindow w;
    w.resize(450, 320);
    w.setWindowTitle("Password Manager");
    w.show();

    return a.exec();
}