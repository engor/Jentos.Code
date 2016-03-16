# Jentos_IDE [![Donate](https://www.paypalobjects.com/en_US/i/btn/btn_donate_SM.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=BRYKFMQT3DWDU)
[![Build Status](https://travis-ci.org/malublu/Jentos_IDE.svg?branch=master)](https://travis-ci.org/malublu/Jentos_IDE)
[![Build status](https://ci.appveyor.com/api/projects/status/g6wn2pcvkn39alh6/branch/master?svg=true)](https://ci.appveyor.com/project/malublu/jentos-ide/branch/master)
[![GitHub issues](https://img.shields.io/github/issues/malublu/Jentos_IDE.svg)](https://github.com/malublu/Jentos_IDE/issues)
[![GitHub license](https://img.shields.io/badge/license-GPLv3-blue.svg)](https://raw.githubusercontent.com/malublu/Jentos_IDE/master/LICENSE.TXT)

---
## About
[Jentos IDE](http://malublu.github.io/Jentos_IDE/) is a powerfull open source IDE for [Monkey](http://www.monkey-x.com) programming language.

It is based on [TED v1.17](http://www.monkey-x.com/Monkey/ted.php), the official IDE for Monkey.

## Jentos IDE 1.3.1
News

- Add View Image(popup window) in area Project.
- Add Edit Image (popup window) in area Project..
- Add Build and Run (popup window) in area Project..
- Add Build (popup window) in area Project..
- Add Build and Run (popup window) in editor Project..
- Add Run (popup window) in editor Project..

Corrected

- Corrected Theme Monokai Dark Soda.
- Corrected Theme  LightTable.
- Corrected "Open on Desktop"
- Corrected: and you can not open images in the code editor


#### Build with QTCreator

## Features

### snippets 
[![Snippers](http://i.imgur.com/oQc0ut3.gif)](http://fingerdev.com/apps/jentos/)



* **@mojoapp**   
 Create template basic, by default name Class is "Game"
* **@mojoapp{name}**  
 Create template basic, **{name} -> MyGame**
* **@m{name}**    
 Create Method **{name} -> mymethod**
* **@cc{name}**     
 Create Class **{name} -> myclass**
* **@kdd**           
 Create Input KeyDown 4 direcction Left,Right,Up,Down
* **@kd-{key}**      
 Create Input KeyDown, **{key} -> R,ENTER,A,etc.**
* **@khd**          
 Create Input KeyHit 4 direcction Left,Right,Up,Down
* **@kh-{key}**      
 Create Input KeyHit, **{key} -> R,ENTER,A,etc.**

**Strict**  
* **@:mojoapp**   
 Create template basic, by default name Class is "Game"
* **@:mojoapp{name}**  
 Create template basic, **{name} -> MyGame**
* **@vm{name}**  
 Create Method Not Return Void **{name} -> mymethod**
* **@im{name}**  
 Create Method Return Integer **{name} -> mymethod**
* **@fm{name}**  
 Create Method Return Field **{name} -> mymethod**
* **@sm{name}**  
 Create Method Return String **{name} -> mymethod**
* **@bm{name}**  
 Create Method Return Bool **{name} -> mymethod**
* **@fl{name}**  
 Create List Field **{name} -> mynamelist**
* **@ll{name}**  
 Create List Local **{name} -> mynamelist**
* **@Fore{name}**  
 Create For EachIn loop **{collection} -> namecollection**


### Examples snippers:


#### Snippet :
##### @mMyMethod

```
    Method MyMethod()

    End
```

#### Snippet :
##### @kd-Enter
```
        If KeyDown(KEY_ENTER)

        End
```
### Themes / Temas
* Android Studio, dark.
* Qt Creator, light.
* Netbeans ,light, is default.
* Monokai Dark Soda (Sublime Text) --**new Theme Code**
* Light Table --------------------------------- **new Theme Code**
* Also monkey's docs have dark style when current theme is Android Studio (need to restart app).

[![Themes](http://i.imgur.com/CvI5flx.jpg)](http://fingerdev.com/apps/jentos/)


### Code Analyzer
* Correct multiple inheritance.
* Folding for "if,while,for,select" statements; and local variables is local for analyzer inside of them.
* Analyze user's imports.
* Autoformat code with adding (or removing) nessesary spaces.
* Replacement for variables type: % # $ ? with :Int, :Float, :String, :Bool.
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
