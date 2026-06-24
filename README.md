# PasswordManager

A simple desktop password manager built with **C++**, **Qt**, and **SQLite**.

## Features

- Create and confirm a master password on first launch.
- Store website, username, and password entries in a local database.
- View saved entries in a clear list.
- Show, edit, and delete passwords when needed.
- Encrypt stored password data before saving it.

## Getting Started

1. Launch the application.
2. Create and confirm your master password.
3. Log in with your master password.
4. Click **Add new password** to save a new entry.
5. Select an entry from the list to view, edit, or delete it.

## Usage

### Add a password

- Open the main menu.
- Click **Add new password**.
- Enter the website, username, and password.
- Click **Save**.

### View a password

- Click an existing entry in the password list.
- Use **Show** to reveal the saved password.

### Edit or delete a password

- Open an existing entry.
- Change the fields and click **Save Changes**, or click **Delete** to remove the entry.

## Tech Stack

- C++
- Qt Widgets
- SQLite
- OpenSSL
- Argon2

## Notes

- Password data is stored locally on your device.
- The app uses a master password to control access.
- A working Qt setup is required to build the project from source.
