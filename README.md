Jentos_IDE [![Donate](https://www.paypalobjects.com/en_US/i/btn/btn_donate_SM.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=RGCTKTP8H3CNE)
==========
## About
[Jentos IDE](http://fingerdev.com/apps/jentos/) is a powerfull open source IDE for [Monkey](http://www.monkey-x.com) programming language.

It is based on [TED v1.17](http://www.monkey-x.com/Monkey/ted.php), the official IDE for Monkey.

[![Jentos Screenshot](http://fingerdev.com/apps/jentos/qt.png)](http://fingerdev.com/apps/jentos/)

## Download Links:
### Windows (version 1.1)
![Windows](http://fingerdev.com/img_targets/windows.png)Download Sources: [[Google Drive](https://drive.google.com/uc?id=0Bx2zoIlA6GzKQmM4cTcteGxTQzg)] - [[Yandex Disk](https://yadi.sk/d/92ucsHehUAw3Q)]
### Mac OS X (version 1.1)
![Mac OS X](http://fingerdev.com/img_targets/mac.png)Download Sources: [[Google Drive](https://drive.google.com/uc?id=0Bx2zoIlA6GzKV0RwblRnVURWVDQ)] - [[Yandex Disk](https://yadi.sk/d/PyOHi9LGUGr7P)]
### Linux (version 1.1)
![Linux](http://fingerdev.com/img_targets/linux.png)Download Sources: [[Google Drive](https://drive.google.com/uc?id=0Bx2zoIlA6GzKR1JKc0Jpbmpfb0k)] - [[Yandex Disk](https://yadi.sk/d/UokJw-WMWNm5g)]

## Features

### snippets 
@mojoapp
name by default is Game

@mojoapp{name}
```blitzmax
Import mojo

Class {name} Extends App

	 Method OnCreate()
		  SetUpdateRate 30
	 End

	 Method OnUpdate()

	 End

	 Method OnRender()
		  Cls
		  DrawText("Welcome Monkey", 302, 240)
	 End

End

Function Main()
	 New {name}()
End
```


### Code Analyzer
* Correct multiple inheritance.
* Folding for "if,while,for,select" statements; and local variables is local for analyzer inside of them.
* Analyze user's imports.
* Autoformat code with adding (or removing) nessesary spaces.
* Replacement for variables type: # $ % ? with :Int, :Float, :String, :Bool.
* Autocomplete for (), "", '', [].
* Autocomplete for function, condition, cycles, etc. by pressing Ctrl+Enter.
* All monkey's modules analyzing at startup, so you always work with actual items.

### Autocompletion list
* Works for monkey's and user's words.
* List opens when ident reachs 3 chars, or by pressing Ctrl+Space.
* Contains inherited members from base class and interfaces.

### Templates
* Allow you to insert big expression by typing just few symbols.
* Type template symbols and press Tab: fu+tab => function, me+tab => method , etc.
* Templates are stored in templates.txt, you can add your own templates.

### Smart navigation
* Improved CodeTree with icons and class/function members.
* Additional list 'Source', which contains members of selected class or function.
* Shows variable's info when hold Ctrl and mouse over.
* Go to variable's declaration by Ctrl+Left Mouse Button.
* Find Usages / Rename.
* Comment / Uncomment Block.
* Changing words case to lower and upper by hotkeys.
* 'Find and replace' panel now placing between code and debug areas (instead of popup window).
* Additional panel with line numbers, which also contains bookmark icons and highlight for edited lines.
* Go back / Go forward for code - jump to lines which were edited.
* Bookmarks.
* Highlight words which are the same as the word under cursor.
* Open files by dragging them into the editor area.
* Contextual help
* First press F1 - shows help in status bar, second press - open help page.
* Shows variable's info when hold Ctrl and mouse over.

### Themes
* Android Studio, dark.
* Qt Creator, light.
* Netbeans ,light, is default.
* Also monkey's docs have dark style when current theme is Android Studio (need to restart app).
