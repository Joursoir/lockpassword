# LockPassword
a simple terminal password manager, using GnuPG to encrypt passwords. Distributed under the GNU General Public License, version 3.

## Dependencies:
* GnuPG
* [GPGME](gnupg.org/software/gpgme/) - GnuPG high-level crypto API

## Installation:
Run the next commands:
```
make
sudo make install
```

## Synopsis:
lpass [command] [options]

## Commands:
### init *gpg-key*
Initialize the password manager using the passed *gpg-key* as the encryption key. This command must be run first before you start working with LockPassword.
### insert [**-e, --echo**] [**-c, --copy**] [**-f, --force**] *passname*
Add the specified *passname* to the password manager. The password will be read interactively using standard input, character display is hidden. The **-e, --echo** argument enable the show of characters when typing a password; **-c, --copy** write password to clipboard; **-f, --force** ignore exist of *passname*, overwrites it without prompt.
### generate [**-l, --length=pass-length**] [**-c, --copy**] [**-f, --force**] *passname*
Generate a random password and write it in *passname*. The **-l, --length = pass-length** argument allow you to specify the desired password length. Without this argument, a 14 character password will be generated. **-c, --copy** write password to clipboard; **-f, --force** ignore exist of *passname*, overwrites it without prompt.
### mv [**-f, --force**] *old-path* *new-path*
Move/rename *old-path* to *new-path*. *old-path* must be an exist file, *new-path* can be a file/directory. The **-f, --force** argument ignore exist of *new-path* (if it's a file), overwrites it without prompt.
### rm *passname*
Remove the *passname* you specified from the password manager. If the directories where your *passname* was nested became empty after deletion, then they are also deleted.
### help
Print help information about commands and the application itself.
### version
Print information about the version, release date, and license of the application.

## Guide:
* Initialize the password manager:
```
$ lpass init joursoir@joursoir.net
LockPassword initialized for joursoir@joursoir.net
```
```
$ lpass init 3BC3B37774696574B0F1C7D47B411E35F4F03E49
LockPassword initialized for 3BC3B37774696574B0F1C7D47B411E35F4F03E49
```

* Add password in the password manager:
```
$ lpass insert games/chess/user
Please type your new password: [invisible input]
Please type your new password again: [invisible input]
Password added successfully for games/chess/user
```

* Print a list of exists password:
```
$ lpass
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
$ lpass banks
Password Manager/banks
|-- abankpro
|   `-- phone_number
`-- ubank
    `-- phone_number
```

* Show password:
```
$ lpass games/iko/LordOfNight
helloitismypassword123
```

* Copy password to clipboard:
```
$ lpass -c games/iko/LordOfNight
```

* Generate password:
```
$ lpass generate bank/sbank/phone_number
Generated password: NsNu:+^Re(cshW
Password added successfully for bank/sbank/phone_number
```
