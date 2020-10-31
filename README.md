# LockPassword
a simple terminal password manager, using GnuPG to encrypt passwords. The application positions itself as open-source software. Distributed under the Unlicense license.

## Dependencies:
The following dependencies must be installed for work:
* tree
* gpg
* gcc

P.S: these can be installed using the package manager such as `pacman`, `apt`, `yum` etc.

## Installation:
Run the next commands:
```
git clone https://github.com/Joursoir/lockpassword
make
sudo make install
```

## Synopsis:
lpass [command] [arguments] ...

## Commands:
### init *gpg-key*
Initialize the password manager using the passed *gpg-key* as the encryption key. This command must be run first before you start working with LockPassword.
### insert [**-e, --echo**] [**-c, --copy**] [**-f, --force**] *passname*
Add the specified *passname* to the password manager. The password will be read interactively using standard input, character display is hidden. The **-e, --echo** argument enable the show of characters when typing a password; **-c, --copy** write password to clipboard; **-f, --force** ignore exist of *passname*, overwrites it without prompt.
### edit [**-t, --text-editor=text-editor**] *passname*
Open the specified *passname* in a text editor, waiting for changes. Standard text editor - vim, argument **-t, --text-editor = text-editor** allow you to change it.
### generate [**-l, --length=pass-length**] [**-c, --copy**] [**-f, --force**] *passname*
Generate a random password and write it in *passname*. The **-l, --length = pass-length** argument allow you to specify the desired password length. Without this argument, a 14 character password will be generated. **-c, --copy** write password to clipboard; **-f, --force** ignore exist of *passname*, overwrites it without prompt.
### mv/move [**-f, --force**] *old-path* *new-path*
Move/rename *old-path* to *new-path*. *old-path* must be an exist file, *new-path* can be a file/directory. The **-f, --force** argument ignore exist of *new-path* (if it's a file), overwrites it without prompt.
### rm/remove/delete *passname*
Remove the *passname* you specified from the password manager. If the directories where your *passname* was nested became empty after deletion, then they are also deleted.
### help
Print help information about commands and the application itself.
### version
Print information about the version, release date, and license of the application.

## Guide:
* Initialize the password manager:
```
[joursoir@archlin ~]$ lpass init joursoir@github.com
mkdir: created directory '/home/joursoir/.lock-password/'
LockPassword initialized successfully
```
```
[joursoir@archlin ~]$ lpass init 3BC3B37774696574B0F1C7D47B411E35F4F03E49
mkdir: created directory '/home/joursoir/.lock-password/'
LockPassword initialized successfully
```

* Add password in the password manager:
```
[joursoir@archlin ~]$ lpass insert games/chess/user
Please type your new password: [invisible input]
Please type your new password again: [invisible input]
Password added successfully for games/chess/user
```

* Print a list of exists password:
```
[joursoir@archlin ~]$ lpass
Password Manager
|-- banks
|   |-- abankpro
|   |   `-- phone_number
|   `-- ubank
|       `-- phone_number
`-- games
    `-- chess
        |-- site.com
        `-- user
```

* Print a list of exists password in some directory:
```
[joursoir@archlin ~]$ lpass banks
Password Manager/banks
|-- abankpro
|   `-- phone_number
`-- ubank
    `-- phone_number
```

* Show password:
```
[joursoir@archlin ~]$ lpass games/iko/LordOfNight
helloitismypassword123
```

* Copy password to clipboard:
```
[joursoir@archlin ~]$ lpass -c games/iko/LordOfNight
Password copied to clipboard.
```

* Generate password:
```
[joursoir@archlin ~]$ lpass generate bank/sbank/phone_number
Generated password: NsNu:+^Re(cshW
Password added successfully for bank/sbank/phone_number
```

# LockPassword -
простой терминальный менеджер паролей, использующий GnuPG для шифрования паролей. Приложение позиционирует себя, как открытое программное обеспечение. Распространяется под лицензией Unlicense.

## Зависимости:
Для работы приложения необходимо установить следующие зависимости:
* tree
* gpg
* gcc

P.S: их можно установить с помощью пакет менеджеров, таких как `pacman`, `apt`, `yum` и другие.

## Установка:
Выполните следующие команды: 
```
git clone https://github.com/Joursoir/lockpassword
make
sudo make install
```

## Синтаксис:
lpass [command] [arguments] ...

## Команды:
### init *gpg-key*
Инициализирует менеджер паролей, в качестве ключа шифрования использует переданный *gpg-key*. Это команда должна быть запущена самой первой, перед тем как вы начнете работу с LockPassword.
### insert [**-e, --echo**] [**-c, --copy**] [**-f, --force**] *passname*
Добавляет указанный *passname* в менеджер паролей. Пароль будет считан в интерактивном режиме, с помощью стандартного потока ввода данных, отображение символов скрыто. Аргумент **-e, --echo** позволяет включить отображение символов при вводе пароля; **-c, --copy** записывает пароль в буфер обмена; **-f, --force** игнорирует наличие существующего *passname*, перезаписывает его не спрашивая.
### edit [**-t, --text-editor=text-editor**] *passname*
Открывает указанный *passname* в текстовом редакторе, ожидая изменений. Стандартный текстовый редактор - vim, аргумент **-t, --text-editor=text-editor** позволяет сменить его.
### generate [**-l, --length=pass-length**] [**-c, --copy**] [**-f, --force**] *passname*
Генерирует случайный пароль и записывает его в *passname*. Аргумент **-l, --length=pass-length** позволяет указать желаемую длину пароля. Без данного аргумента будет сгенерирован пароль длиной 14 символов. Аргумент **-c, --copy** записывает пароль в буфер обмена; **-f, --force** игнорирует наличие существующего *passname*, перезаписывает его не спрашивая.
### mv/move [**-f, --force**] *old-path* *new-path*
Передвигает/переименовывает *old-path* в *new-path*. *old-path* обязательно должен быть существующим файлом, *new-path* может быть файлом/директорией. Аргумент **-f, --force** игнорирует наличие существующего *new-path* (если это файл), перезаписывает его не спрашивая.
### rm/remove/delete *passname*
Удаляет указанный вами *passname* из менеджера паролей. Если директории, куда был вложен ваш *passname* после удаления стали пусты, то они тоже удаляются.
### help
Выводит справочную информацию о командах и самом приложении.
### version
Выводит информацию о версии, дате выпуска и лицензии приложения.

## Гайд:
* Инициализация менеджера паролей:
```
[joursoir@archlin ~]$ lpass init joursoir@github.com
mkdir: created directory '/home/joursoir/.lock-password/'
LockPassword initialized successfully
```
```
[joursoir@archlin ~]$ lpass init 3BC3B37774696574B0F1C7D47B411E35F4F03E49
mkdir: created directory '/home/joursoir/.lock-password/'
LockPassword initialized successfully
```

* Добавить пароль в менеджер паролей:
```
[joursoir@archlin ~]$ lpass insert games/chess/user
Please type your new password: [невидимый ввод текста]
Please type your new password again: [невидимый ввод текста]
Password added successfully for games/chess/user
```

* Вывести список существующих паролей:
```
[joursoir@archlin ~]$ lpass
Password Manager
|-- banks
|   |-- abankpro
|   |   `-- phone_number
|   `-- ubank
|       `-- phone_number
`-- games
    `-- chess
        |-- site.com
        `-- user
```

* Вывести список существующих паролей в какой-либо директории:
```
[joursoir@archlin ~]$ lpass banks
Password Manager/banks
|-- abankpro
|   `-- phone_number
`-- ubank
    `-- phone_number
```

* Показать пароль:
```
[joursoir@archlin ~]$ lpass games/iko/LordOfNight
helloitismypassword123
```

* Скопировать пароль в буфер обмена:
```
[joursoir@archlin ~]$ lpass -c games/iko/LordOfNight
Password copied to clipboard.
```

* Сгенерировать пароль:
```
[joursoir@archlin ~]$ lpass generate bank/sbank/phone_number
Generated password: NsNu:+^Re(cshW
Password added successfully for bank/sbank/phone_number
```