   Skinner's Atlas 1.5
    by Jellby (et al.)
(First release: June 2002)
        May 2003
::::::::::::::::::::::::::

NOTE: This file seems to be too big to read it in the winamp skin browser (ALT+S), if you want to read it I suggest to extract the file: just open the .wsz file with winzip or any other zipping program and extract readme.txt, now you can read it with notepad or any other text program.

I'm sure there are plenty of winamp skin templates out there, this is another one. I've tried to draw every component, bugs included, and to make a skin that can be actually used as a template (either for painting over it or for using it as an overlay layer). In general all files follow the same guidelines: background is white, background of winamp is gray, components are blue, outer guides are red, comments are white-on-black. The only zones you have to change are the gray and blue zones. The red guides may be useful for those not using layers, they mark the width and height of as many components as possible.

Below there's a description of all the skin files, if you're an expert skinner there's probably nothing you don't know, but if you are a beginner you should read it. Just remember that I, like everyone, make mistakes.

* Legal notice:

It may seem obvious, but I give you permission to extract the files included and modify them, using this skin as a base for your own one. But please, don't just change colours and say that you've made a new template, which is red and yellow :P

* Acknowledgements:

First of all I want to thank Misha Klement (Cyana) for the excelent Template Amp (http://www.winamp.com/skins/detail.jhtml?componentId=71571), which I used as a base for this template. And, of course, all the regulars of the Skin Love forum (http://forums.winamp.com/forumdisplay.php?s=&forumid=5) and especially: skinme! for his CursorMultiply utility, Kenny D. for the name suggestion, ¤db¤ for his constant support, ideas and criticism (and for amarok and vidamp templates), Mr Jones for his help (actually, he did most of the work) updating to WA2.9... and all the others.

* History:

1.0.0: First version
1.0.1: Added the 'pl-winshade-button' bug in the readme.txt
1.0.2: Fixed spelling and typos of readme.txt
1.0.3: Fixed some confusion with the half-tiles in playlist and minibrowser (readme.txt and skeletons.bmp).
1.0.4: Changed to 24 bits (true colour) to make it easier working with layers.
1.1: Added skeletons2.bmp with different layout (see the readme.txt), added clutterbar 'buttons' to main.bmp, changed symbols in balance.bmp
1.1.1: Volume.bmp was 2 pixels narrower. Size is correct now (change not reflected in screenshot).
1.1.2: Deleted two spurious lines in pledit.txt
1.1.3: Updated skeletons.bmp, skeletons2.bmp and screenshot. Typos fixed in readme.txt
1.2: Added pl-stacks template to skeletons.bmp, as suggested by ¤db¤.
1.2.1: Pl-stacks template now in skeletons2.bmp too.
1.2.1b: Changed a colour.
1.3: Winshade bars (posbar, volume, balance) templated, showing where the sliders change.
1.3b: Reloaded (a couple of files were missing).
1.4: Added amarok.bmp and vidamp.bmp, by ¤db¤. Fixed 1-pixel too long winshade balance.
1.5: Added video.bmp, gen.bmp and genex.bmp support (i.e. adapt to Winamp 2.9), updated readme, this bit done mainly by Jones ;)

INDEX
¯¯¯¯¯
MAIN.BMP
TITLEBAR.BMP
CBUTTONS.BMP
SHUFREP.BMP
POSBAR.BMP
MONOSTER.BMP
PLAYPAUS.BMP
NUMS_EX.BMP & NUMBERS.BMP
TEXT.BMP
VOLUME.BMP
BALANCE.BMP
VISCOLOR.TXT
EQMAIN.BMP
EQ_EX.BMP
PLEDIT.BMP
PLEDIT.TXT
MB.BMP
AVS.BMP
VIDEO.BMP
GEN.BMP
GENEX.BMP
MIKRO.BMP
WINAMPMB.TXT
AMAROK.BMP
VIDAMP.BMP
CURSORS
SKELETONS.BMP & SKELETONS2.BMP


MAIN.BMP      (275×116)
¯¯¯¯¯¯¯¯
There's nothing really special with this file, it is the background of the main winamp window, you can paint whatever you want here. The boxes are drawn only as a guide to let you know where each component is going to be.

*visible/invisible areas

Most of the areas will be covered by the actual components found in other files, so whatever you paint here won't ever be seen in winamp. These are the only component areas that will show in main.bmp:

-The big numbers (those 5 boxes on the left) when there's no file playing.
-The posbar, (that long strip over the control buttons) when there's no file playing.
-The vis (the box under the numbers) when it's deactivated or no file is playing.
-The kbps & khz areas (the two little boxes to the right of the vis)  when there's no file playing.
-The winamp lightning bolt (bottom right corner). In WA2.9 this now triggers the Media Library plugin (if installed), it can be toggled in the main Winamp Preferences area. In versions of WA lower than 2.9 it triggers the Winamp credits box.
-The non-bitmap text box (the dark long box to the right of the numbers) if you are using bitmap fonts (see later).

You can fill all the other component areas with a plain colour, this will reduce the size of the final zip or wsz file.

*Song title

You'll notice two boxes in the song-title area (actually, you'll notice one box and two darker strips). The smallest and lighter one is the box occupied when you use bitmap font; if you uncheck the "Use bitmap font for main title display (no int. support)" option in the preferences dialogue, then the bigger and darker box will be used. Be sure to check this when you are testing your skin.


TITLEBAR.BMP  (344×87)
¯¯¯¯¯¯¯¯¯¯¯¯
In the middle zone of this file you'll find 6 'titlebars' or long gray strips, from top to bottom they are: active normal, inactive normal, active winshade, inactive winshade, active easter-egg, inactive easter-egg. The app-buttons (those little blue squares) drawn inside the bars will be shown only until you click on them (I'll explain it in a moment).

*Titlebars

The active/inactive versions are used when the main window is selected/unselected respectively. The normal titlebars are, as you could guess, the normal ones while the easter-egg versions are used when the winamp's easter-egg is activated (you can activate it by typing N-U-L-[esc]-L-[esc]-S-O-F-T while the main window is selected).

*Winshade

The winshade mode is the bar that will be shown when the main window is shrunk to winshade, and here we find an important bug in winamp: The line of lighter pixels between the active and inactive winshades is shared! That means it will appear at the bottom of the active one and at the top of the inactive one, don't forget it or your skin will have a bug too. The winshade bars have a full set of control buttons without pressed states, a mini posbar (to their right) that will be shown when no file is playing, two boxes for the time display (to their left) and a mini vis (further to the left), which behaves just like the one in main.bmp.

*App-buttons & posbar

In the top left zone of this file there's a set of app-buttons. The top row are (left to right) the winamp or main menu button, the minimize button and the close button in their unpressed (released, up..) states. Right under them are the same buttons in their pressed (depressed, pushed, down...) states. Then (3rd row) we have the winshade button in its pressed and unpressed states (left to right) and, under them, the 'return-to-normal' button in, again, both states. This last button is the one shown in the winshade mode, that permits us to return to the normal mode. While you don't click a button, the version painted inside the titlebars is used; when you click it, the pressed version in the left part will appear and, if you move the mouse without releasing the button, the unpressed version in the left will show; from now on, only the pressed and unpressed versions of the button will be seen and the titlebar version will be forgotten... until the titlebar changes (because you activate the easter-egg, select a different window, change to winshade mode...). Note too that the 'outside' buttons are the same for normal and winshade mode, so if you paint them too differently in the titlebars they could look strange when clicked.

Right under these buttons is the mini posbar, this is the version that will be shown in the winshade mode when a file is playing. To its right there are three narrow sliders, they will all be used for the posbar slider in winshade mode. The posbar is divided in three colour zones, marking where each slider will be used: the left one will be used when the center of the slider is inside the left colour zone, the center one when it's inside the middle colour zone and the right one when it's inside the right colour zone.

*Clutterbar:

The right part of the file is taken by the clutterbar. The top row contains the clutterbar in its unpressed state (left) and when it's deactivated (right). You can deactivate/activate the clutterbar by unchecking/checking the "Always show Clutterbar" option in the preferences dialogue, this will alternate between the two aforementioned forms. The blue boxes in the left clutterbar mark the active areas, where you can click the mouse and activate the buttons.

The bottom row contains the pressed states of each of the five buttons of the clutterbar, the boxes are an attempt to show the areas that change when you click on them. In fact, when you click each of the buttons the whole corresponding column is shown, except the 'A' and 'D' areas. In these areas, the blue area will be shown if the button is activated ('always on top' or 'doublesize') - but if deactivated, the corresponding areas of the top-left clutterbar will be shown. It's not easy to explain, you'd better go and try it.


CBUTTONS.BMP  (136×36)
¯¯¯¯¯¯¯¯¯¯¯¯
No tricks in this file. These are the main control buttons. Just note that the 'next' ( >| ) and 'load ( ^ ) buttons are one pixel narrower (left to right) and the 'load' button is two pixels shorter (top to bottom) than the rest. The top row is the unpressed state, the bottom row is the pressed state.


SHUFREP.BMP   (92×85)
¯¯¯¯¯¯¯¯¯¯¯
*Shuffle & Repeat

The top part contains the shuffle and repeat buttons in their active/inactive, pressed/unpressed states. Top to bottom: inactive unpressed, inactive pressed, active unpressed, active pressed. The repeat button is left, the shuffle button is right (note this is the opposite from the final layout of these buttons). Another thing to note is that the line between the two buttons (in the file it's the far left and far right line cyan line) is shared. It will normally show the pixels from the unpressed shuffle or repeat button (the last one clicked), but when you click one of the buttons it changes, it changes when you click the repeat button, and it changes when you click the shuffle button. So, the best way to fix this is to assume the shuffle button (for example) is one pixel narrower and just copy the left border of the unpressed repeat button (you better make this line the same in both active/inactive versions) into this conflictive line of the shuffle button, now reduce the shuffle button's width and you're done.

* Eq & Pl

In the bottom part you'll find another set of 8 buttons, this time the 'eq' and 'pl' buttons. They are now in the same layout they will be used ('eq' to the right, 'pl' to the left), so there are 4 pairs of buttons, each pair is one state. Again, left to right, top to bottom: inactive unpressed, inactive pressed, active unpressed, active pressed. In short: top row inactive, bottom row active, left half unpressed, right half pressed.


POSBAR.BMP    (307×10)
¯¯¯¯¯¯¯¯¯¯
This file contains the posbar that will be shown when there's a file playing, along with the slider in both unpressed and pressed states (in this order, left to right). Note that there is a one pixel line between both states of the slider, but no gap between the posbar and the unpressed slider. You can crop this file (ie. make it less than 10 pixels tall) to get a thinner posbar and slider.


MONOSTER.BMP  (56×24)
¯¯¯¯¯¯¯¯¯¯¯¯
Similarly to shufrep.bmp, the stereo and mono indicators are inverted from their positions in the main window, here stereo is to the left and mono to the right, in the main window (and final winamp) it's the other way round. The top row shows both indicators in their active state, the bottom row shows them in their inactive state. You'll only see one of them active at the same time, and both inactive in no file is playing.


PLAYPAUS.BMP  (42×9)
¯¯¯¯¯¯¯¯¯¯¯¯
This is one of the most obscure and confusing files in winamp's skin. It is shown to the left of the big numbers in the main window. It tells whether winamp is playing, paused, or stopped (light squares) and whether a file is loaded or buffering (dark columns). From left to right: playing, paused, stopped, none, blank, loaded, buffering.

When there's no file playing (stopped), the 'none' column will be shown, which is only 2 pixels wide. When a file is loaded or buffering, the appropriate 3-pixel column will show, and it'll overlap the 'play' or 'pause' indicator. I drew a dark line next to every light square, this will complete the dark column when no file is playing and will be covered otherwise, each frame goes with the line to its left. The blank part is never shown, I believe.


NUMS_EX.BMP   (108×13)
NUMBERS.BMP   (99×13)
¯¯¯¯¯¯¯¯¯¯¯
Two similar files, you only need to include one of them, it is recommended to include nums_ex.bmp, but old versions of winamp (and WA3, I've heard) might need numbers.bmp. If both are present, nums_ex.bmp will be used. The only difference is that nums_ex.bmp has an additional frame for the minus ( - ) sign (used when you set the display to "Time remaining"), in numbers.bmp the middle part of the sign for '2' is used as a minus.

The structure of this file is quite straightforward, from left to right: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, (blank), -. The blank will be used instead of the minus when the display is set to "Time elapsed".

There's a bug when nums_ex.bmp is used, but there's not much you can do about it. When you pause a playing file, the numbers blink and the leftmost character makes funny things, in fact it takes two more pixels to its right and this is not reset until you stop the file, or a new file starts playing...


TEXT.BMP      (155×18)
¯¯¯¯¯¯¯¯
In this file the 'bitmap font' I talked about in the main.bmp description is defined. The font will be used for the song title when the "Use bitmap font" option is active and in the kbps and khz areas, it's also used for the time display in winshade mode and in the playlist, and for the current song title in the pl-winshade. Every letter occupies a 5×6 rectangle. Only the top section of the file is used, so you can crop it once you have your letters. I've included a grid to help you design your own letters and a guide for you to see which letter goes in where.

When you click one of the buttons (clutterbar, volume, eq-slider...) in winamp, a description appears in the song title box and the line marked in red in the bottom section (right border of 'A') is used to fill the box from the text's end to the box's end, so if your 'A' character takes this line, strange things will happen.

There's a white box in the bottom section too (far right), this is the 'space' character. The red pixel in its left border is where the background colour for the 'non-bitmap font' mode is taken from. The foreground colour doesn't seem to be taken from a single pixel, I mean, it's not always the same pixel, it depends on the colours, contrast with the background... but, if it's possible, it will be taken from somewhere in the 'A' character.

The characters are:
A B C D E F G H I J K L M N O P Q R S T U V W X Y Z " @
0 1 2 3 4 5 6 7 8 9   . : ( ) - ' ! _ + \ / [ ] ^ & % , = $ #
Å Ö Ä ? *

Between '9' and '.' there's a character that looks like three dots but it's not used for the ALT+0133 character. As I've said, the 'space' is the rightmost character in the top row.


VOLUME.BMP    (68×433)
¯¯¯¯¯¯¯¯¯¯
This file defines the volume slider and bar. First, you'll see 28 strips, these are the bars shown when the volume is set to minimum (top), maximum (bottom) and, strangely enough, all the intermediate states in the middle. Inside these bars you see dark squares; these mark the position the slider is in each of the states (the first bar is used when the slider is to the left, as the slider moves to the right different bars are shown); this means that you will never see these dark squares because they are always covered by the sliders. Note that, because winamp is not an exact science, there are more possible slider positions than bar states, i.e. you can move the slider one or two pixels without the bar changing, that's why the dark boxes are two pixels narrower than the actual slider. If you want them to be the same size, add one pixel to each side of the position markers, then you'll see how these dark pixels show behind the slider as you move it.

Under the 28 bars, there's the slider in its pressed and unpressed state (left to right). You can crop this file (make it less than 433 pixels tall) to reduce the vertical size of the slider or even to make it completely disappear. In this case the position markers are a guide of where is the slider supposed to be, even if you can't see it.


BALANCE.BMP   (47×433)
¯¯¯¯¯¯¯¯¯¯¯
This file is very similar to volume.bmp, but it defines the balance slider and bar. If this file doesn't exist, the equivalent section of volume.bmp will be used for the balance control. The structure of this file is the same as the previous but this time the top bar is used when the slider is at the centre and lower bars are used as you move the slider to any side, the same set is used when you move to the left and to the right, that's why there are two position squares in the lower bars (in top bars there are two too, but they overlap and it looks like a wider one). This time the markers are only one pixel narrower than the slider, to make them the same size add one pixel to the centre side of the markers.

To the left of each bar you'll see up to three red symbols which won't be shown in winamp, they are there just to mark which bars are visible. The bars marked with the square are the only ones shown when you use the balance in the main. The bars marked with the left arrow are shown when you adjust the balance to the left using the eq-winshade control. The bars marked with the right arrow are shown when moving the eq-winshade control to the right. The bars with no symbol are, as far as I know, never shown.

Again, the slider is underneath the bars in its pressed (left) and upressed (right) states.


VISCOLOR.TXT
¯¯¯¯¯¯¯¯¯¯¯¯
This text file contains all the colours that will be shown in the visualization area when it is active (oscilloscope or analyzer). Each colour is defined by three decimal numbers (red, green and blue components) separated by a comma, everything between "//" and the end of a line is a comment, it won't be taken into account. Each component range between 0 (no colour, black) to 255 (full colour: red, green or blue), so 0,0,0 is black, 255,255,255 is white. The order of the colours is commented in the file: the fist two colours define the background and dots (check it to see what are the dots), the next 16 colours are the analyzer's colours from top to bottom, the next 5 colours are the oscilloscope's ones, from center to top/bottom, the last colour is for the analyzer's peak markers.


EQMAIN.BMP    (275×315)
¯¯¯¯¯¯¯¯¯¯
This file contains the main part of the equalizer window. The top part is the background and, as it happened with main.bmp, it will be mostly covered by other components, in fact, only the light gray areas and the +12/0/-12 zones will be shown in winamp, the rest can be painted with a plain colour to reduce the final size.

* Titlebar

This window has a titlebar and app-buttons too, and they are defined just like they were in titlebar.bmp. The long gray strips are the titlebar in active (top) and inactive (bottom) state. The close button inside the titlebars will be shown until you click on it, its pressed and unpressed states of the close button are painted between the background and the active titlebar, far left (unpressed is at top, pressed is at bottom). The winshade button is different, the pressed state is always the one shown in the painted in the titlebar, the unpressed state is defined in eq_ex.bmp (see later), if there is no eq_ex.bmp then it doesn't change when clicked.

* Buttons

To the right of the close button we have the 'on' and 'auto' buttons, like in shuffrep.bmp these buttons have both active and inactive states (and pressed and unpressed). They come in pairs on/auto, left to right: inactive unpressed, active unpressed, inactive pressed, active pressed. In the right part under the inactive titlebar we have the 'presets' button in its unpressed (top) and pressed (bottom) states.

* Eq-sliders

To the left of the presets button we see the beloved eq-sliders, again 28 bars with position markers that give us the opportunity to change the gain of different frequencies... that's an equalizer. Note that the same bars will be used for each of the 11 eq-sliders (10 freq, 1 preamp), so you can't have a decent background under them. As it happened with the volume, the position markers are two pixels shorter than the actual slider, you should add one pixel to the top and one pixel to the bottom of the markers to make them same size. To the left of the bars are the unpressed (top) and pressed (bottom) states of the slider, which again is the same for all 11 eq-sliders. Unfortunately, you can't crop the slider leaving the bars, so the scope of possible animations for these controls is considerably limited.

* Eq-vis

Finally, the bottom part is taken by the eq-vis, this is some kind of display that will show a curve mirroring the eq setting. The big box is the window, the line below it will show the master gain, the line to its right is a scale of colours: the lowest part of the curve will have the colour of the bottom pixels, the highest part of the curve will have the colour of the top pixels. If you don't like these lines, you can crop the file to eliminate either the bottom line or the whole eq-vis. You may want to crop the eq bars too, but this will eliminate the slider and the presets button as well, and the equalizer will be almost unusable.


EQ_EX.BMP     (275×56)
¯¯¯¯¯¯¯¯¯
Here is where the winshade mode is defined for the equalizer. The top strip is the active mode and the bottom strip is the inactive mode. The blue boxes are bars for volume (left) and balance (right). Under the winshades and to the left are the buttons. First (top row) we find the volume slider (left) and the balance slider (right). These mini sliders are similar to the posbar slider in titlebar.bmp: for each slider there are three forms, used when the slider is in the left part, middle part or right part of the bar, and, as with the winshade posbar, these bars are divided in three colour zones, marking where each slider will be used (the balance slider, however, may change without moving when it's at a zone's edge). The second row of buttons contain the pressed winshade button (this is the pressed state of the button present in eqmain.bmp) and the unpressed close button. The bottom row holds the pressed 'back to normal' button (right) and close button (left). As it happened with the winshade button in eq_main.bmp, there is no 'outside' version for the unpressed state of the 'back to normal' button, so the one in the gray bar will be always used.


PLEDIT.BMP    (280×186)
¯¯¯¯¯¯¯¯¯¯
This file contains everything needed for the playlist window, everything except the colours and font of the playlist text. The first thing you should remember about this window is that it is tiled, i.e. because the window may be resized the borders are actually made of repeated tiles, so you'd better take care so no discontinuities appear.

* Titlebar

The top border of the playlist window is taken by the titlebar, which can be found in the top left part of pledit.bmp. The first row is for the active titlebar, the second for the inactive titlebar. Each row is composed of (left to right): left corner, central section, tile, right corner. The central section will always be placed in centre of the titlebar and a number of copies of the 'tile' will be placed between each corner and the central section. If the total number of tiles is going to be odd, because the central section has to remain in the centre, a half-tile is used at both sides. To keep things weird, the width of the tile is odd, so it can't be divided in two equal halves, 12 pixels of the tile (the dark part) will be used to the right of the left corner and 13 pixels of the tile (the dark part and an additional light line) will be used to the right of the central part. This makes it even harder to design a good-looking playlist titlebar.

The known app-buttons are present in the right corner of the titlebar: close and winshade, these buttons are the unpressed states, the pressed states are found right under the inactive central part, it's not hard to find them. There is another bug here, the winshade button is displaced one pixel to the right with respect to the other windows (there's no gap between winshade and close buttons). You can still draw the button in the usual position, but keep in mind that the area that will change is a bit different.

* Side and bottom borders

The sides of the playlist are entirely (except the corners) made of tiles, and the tiles are found in the third row of this file, first the left tile, then the right tile. Marked in light grey is the zone that won't be shown due to it being covered by the playlist text. The right tile contains a blue column - this is the vertical scroll bar. To the right of this right tile, and under the pressed app-buttons you'll find the unpressed and pressed states of the vertical scroll slider.

In the fourth row we have the left and right bottom corners of the playlist window. The left corner has four stack buttons, the right corner has only one, but it has some more things: Another set of control buttons, similar to that found in titlebar.bmp; three text boxes where it will be shown the total length of the current list, the elapsed time of the current song, and so on; a resize button (or active area, it has no pressed state); two scroll-up and scroll-down buttons (or active areas again).

In the top row, far right we find two additional tiles, the smaller one is the bottom tile. When the playlist window is big enough, the wider tile will be used, but only once, between the right corner and the bottom tile(s) (or left corner if there is no room for more bottom tiles). In this additional tile there is a blue box which is an additional vis, but it will show nothing unless the vis is active and the main window is hidden, to do this you have to press ALT+W and click inside this box if nothing appears here.

* The stacks

The most important part of the playlist, apart from the actual list, are the stacks. There are five stacks, located in the left and right bottom corners of the playlist, when you click one of the buttons the associated stack appears as a... stack of buttons. These stacked buttons are defined in the bottom part of pledit.bmp. For each stack we have (left to right): the unpressed buttons, the pressed buttons and the side bar. The stacks are located in the same horizontal order they appear: add, remove, select, miscellaneous, list (left to right). The vertical layout of each stack is also the same that will be used in winamp, except for the second (remove) stack, for which the bottom button will be shown on top of the others. The side bars will be shown to the left of each stack and they have no pressed state. The buttons are (top to bottom, left to right): add url, add directory/folder, add file, remove all, crop (remove unselected), remove selected, remove other, invert selection, select none, select all, sort, file info, other, new list, save list, load list. Each with its unpressed and pressed states. The buttons or parts of buttons or bars that will appear over the play list text are slightly grayed.

* Winshade

The playlist has a winshade mode too, it is defined to the right of the pressed app-buttons and vertical scroll slider. The top row contains the left corner and the active right corner, the bottom row contains the tile and the inactive right corner. The right corner works like a titlebar, changing to active/inactive when the playlist is selected/unselected, the tile is repeated as many times as needed to fill all the length. The blue box that spans all three sections is where the current song will appear. The right corner has a horizontal resize button (active area) and unpressed close and 'back to normal' buttons, the pressed close button is the same as in normal mode, the pressed 'back to normal' button is found to the right of the active right corner. The 'back to normal' button has the same bug the winshade button has: it's one pixel to the right.


PLEDIT.TXT
¯¯¯¯¯¯¯¯¯¯
This text file defines the colours used inside the play list and in the text box of the minibrowser, as well as the font. The colours are defined in html-format: they are hexadecimal numbers, the first two places are for the red component (00-FF) the next to places for the blue component (00-FF) and the last two places for the green component (00-FF), so #000000 is black, #FFFFFF is white, #00FF00 is blue, #808080 is gray... The colours can be defined in any order, but the names must be: 'Normal' for the normal text in the play list, 'Current' for the text of the currently playing song in the play list, 'NormalBG' for the normal background of the playlist, 'SelectedBG' for the background of the selected song in the playlist, 'mbFG' for the text in the minibrowser and 'mbBG' for the background in the minibrowser. The font is defined as 'Font', you can use whatever font you want, but it is recommended to use standard fonts, so that everyone can see it fine without having to install additional fonts. Some of these standard fonts are: Arial, Times New Roman, Verdana, Tahoma, Comic Sans MS, Calisto MT, Book Antiqua, Century Gothic... You should be able to find out which fonts are intalled by default with windows.


MB.BMP        (233×119)
¯¯¯¯¯¯
This file defines the minibrowser window, and it's very similar to pledit.bmp. The minibrowser is resizeable and tiled, just like the playlist window. The top two rows of mb.bmp are the titlebar, and it works just like the playlist's titlebar, except it has no winshade mode and no winshade button. The pressed close button is located right under the line between the inactive tile and inactive right corner. Under the titlebar we find the left bottom corner and under it the right bottom corner. The left corner contains the browser buttons in their unpressed states, the right corner has a resize button (active area). To the right of the right corner there is the bottom tile, which will be placed as many times as needed between the two bottom corners. All along the bottom border there is a blue box which will hold the current location text. To the right of the left corner you'll find the two side tiles, the left one to the left and the right one to the right, they work as the playlist's ones. To the right of the pressed closed button we have the pressed browser buttons, they are (left to right): previous location, next location, stop transfer, reload, other.

In Winamp 2.9 an ugly 2-pixel border was introduced around the inside of the minibrowser, don't try to remove it, you can't (as far as I know). Well, the border can give a good effect sometimes, but it destroys some skins that were made to look good without that border (see winampmb.txt, below).


AVS.BMP       (97×188)
¯¯¯¯¯¯¯
This file defines the avs window, the simplest window in winamp, it's only a border with one button. This window is resizeable, but unlike the playlist and minibrowser it is not tiled but stretched, meaning that the appropriate sections will be used only once but horizontally or vertically resized (stretched).

In the top left corner of this file we find the pressed close button, or this is what it should be, I have never been able to make winamp show it, but there it is. To its right there are the top left corner, the top middle part and the top right corner (with the unpressed close button); right under them, their bottom counterparts. The middle sections will be the ones that will be horizontally stretched. The left part of the file is taken by the left and right borders, these will be vertically stretched.

This file is no longer used by any version of Winamp greater than WA2.9, it is however still supported in older versions, pre WA2.9, so it's probably a good idea to include it anyway.


VIDEO.BMP (233×119)
---------

This file defines the Video window, and it's very similar to mb.bmp. The Video Window is resizeable and tiled, just like the playlist window. The top two rows of video.bmp are the titlebar, and it works just like the playlist's titlebar, except it has no winshade mode and no winshade button. The pressed close button is located right under the line between the inactive tile and inactive right corner. Under the titlebar we find the left bottom corner and under it the right bottom corner. The left corner contains the browser buttons in their unpressed states, the right corner has a resize button (active area). To the right of the right corner there is the bottom tile, which will be placed as many times as needed between the two bottom corners. All along the bottom border there is a blue box which will hold the current playing text. To the right of the left corner you'll find the two side tiles, the left one to the left and the right one to the right, they work as the playlist's ones. To the right of the pressed closed button we have the pressed video buttons, they are (left to right): Full screen, Regular size, Double Size, TV browser and load file.


GEN.BMP (178(+)×103)
-------

This file defines the general purpose window, now used to create things like the media library and AVS windows in versions of Winamp greater than WA2.9. The "gen" windows is resizeable and tiled, just like the playlist window. The top two rows are the title bar , the first being the active title bar, the second the inactive. The various title bar parts read as follows from left to right. Top left is the corner of the window (1), second from left is a fixed bitmap(2), in so much as it doesn't repeat in the title bar. The middle section(3) is used as a title container for the windows main title, this piece does repeat when the window is stretched. The section fourth (4) from the left is another no repeating part of the title bar, very much like the second piece of the bar. Piece number five (5) on the titlebar tiles and fills in the gaps when the window is stretched. The last section on the title bar it the far right corner piece (6), this also features the close window button.

When the "gen" window is re-sized, the title bar sections work in the following way.

(1) (5) (5) (2) (3) (3) (3) (4) (5) (5) (6)

Immediately underneath that two tile bars are the two lower sections of the gen windows, the uppermost section of these represents the bottom left section, the lower the bottom right section. This bottom right section also features a re-sizer hot spot in the very bottom left hand corner, this section is used to resize the whole window.

To the right of the two lower sections are the side walls of the gen window, both left and right sections are tiled when stretched. To the right of these is the close window pressed button. To the right of this are two more side wall sections, these represent the very bottom of the side walls and are used to join the side walls to the two bottom sections of the window, these two sections do not tile when stretched, the right one includes a portion of the re-sizer hot spot, which is too big to fit in the bottom right tile.

Directly under these side wall sections is the tile portion of the lower window, this section is used to stretch the bottom section when the widow is tiled.

Below all of this is a a font for the titlebar, in both highlight and no-highlight modes. The font is variable width, but not variable height, and it uses the first color before the letter A as the delimiter. The no-highlight form of letter must be the same width as the highlight form. The order of the letters is the standard Latin/English alphabet (no foreign characters).

Since the width of the letters is not fixed the width of this file can vary depending on the font you choose. In this template, the font shown is the same of text.bmp.


GENEX.BMP (112×59)
---------

This file is used to create the buttons and sliders that are used in the general purpose windows, and also contains the colour controls for this window. The buttons are as follows. The top left corner contains the active button, underneath this is the inactive, or pressed state button. These buttons have 4 pixel sized edges that are not stretched (either horizontally or vertically), and the centre is stretched (so be careful with what you paint here). Underneath those are the window scrollers, again both active and inactive states, they are from left to right, top to bottom:
Scroll up unpressed
Scroll down unpressed
Scroll up pressed
Scroll down pressed
Scroll left unpressed
Scroll right unpressed
Scroll left pressed
Scroll right pressed.

Next to these are the scroll bar sliders, these behave in a similar way to the button windows, in so much as they have a 4 pixel border that doesn't stretch, while the rest of the button does (except the central portion). These are from left to right:
Scroll slider vertical unpressed
Scroll slider vertical pressed
Scroll slider horizontal unpressed
Scroll slider horizontal pressed

The very top row of genex.bmp features a row of pixels that are used to control the various colours and background of the gen window, these pixels start at (48,0) and run as follows:

 (1) x=48: item background (background to edits, listviews, etc.)
 (2) x=50: item foreground (text colour of edits, listviews, etc.)
 (3) x=52: window background (used to set the bg color for the dialog)
 (4) x=54: button text colour
 (5) x=56: window text colour
 (6) x=58: colour of dividers and sunken borders
 (7) x=60: selection colour for entries inside playlists (nothing else yet)
 (8) x=62: listview header background colour
 (9) x=64: listview header text colour
(10) x=66: listview header frame top and left colour
(11) x=68: listview header frame bottom and right colour
(12) x=70: listview header frame colour, when pressed
(13) x=72: listview header dead area colour
(14) x=74: scrollbar colour #1
(15) x=76: scrollbar colour #2
(16) x=78: pressed scrollbar colour #1
(17) x=80: pressed scrollbar colour #2
(18) x=82: scrollbar dead area colour


MIKRO.BMP     (65×124)
¯¯¯¯¯¯¯¯¯
This is the skin file for a plugin called MikroAMP, which is very popular and easy to skin. The file is structured as follows: The top left black square is what it is called a mask, this defines the shape of the large mikroamp window, in this box black is opaque and white is transparent, it is recommended that you use only pure black and pure white, but the fact is that only pure white will be transparent, all other colours will be opaque. The grey bars to the right of the mask are the mikro-titlebars, they are the top part of the big mikroamp window, the top one is active, the bottom one is inactive. Under the mask we have the three modes of the big window, top to bottom: play, stop and pause. Under the mikro-titlebars, the small versions, again (top to bottom): play, stop and pause. In the bottom part of the file, the tray icons (left to right): play, stop and pause (in Win95-98-2000 only 16 colours will be shown in the tray, although this can be fixed by hex-editing explorer.exe). In the bottom right corner of this file there are two key pixels, the bottom one (in the very corner) defines the transparent colour of the tray icons, the top one should be black, pure black, or there may be a bug in the big pause.

You can get the MikroAmp plugin here (330KB):
http://classic.winamp.com/plugins/detail.jhtml?componentId=63522


WINAMPMB.TXT
¯¯¯¯¯¯¯¯¯¯¯¯
This text file is actually an html file with .txt extension. If you are using MikroAMP this file will be shown by default inside the minibrowser, it is commonly used to show an image with the style of the rest of the skin, or a link to your homepage, or a logo, or some info about you or the skin... In this template I just use this file to show an image called MBINNER.BMP (if you want to have images, they must be .bmp - gifs and jpgs won't be extracted, remember: only BMPs). The images used in this file may be called whatever you want, just be sure to have the same name in the file and inside winampmb.txt.

Note: Due to the border introduced in Winamp 2.9 (see mb.bmp, above) the included mbinner.bmp is a bit too big for the minimum size of the minibrowser, which is now 252×54 instead of 256×58.

If you're working with a decompressed skin (you have the different files stored in a folder inside winamp's skins folder), you'll notice that a new file called winampmb.htm is created. This is the file that is actually used, so you may make changes to winampmb.txt that don't show up in the minibrowser, because the .htm file remains the same. To avoid this, delete winampmb.htm everytime you edit winampmb.txt.


AMAROK.BMP    (280×132)     template and description by ¤db¤
¯¯¯¯¯¯¯¯¯¯
Amarok is a fairly simple plugin that allows you to create "bookmarks" within songs. For example, if you have a song where your favourite bits are at 3:02 and 6:73, Amarok remembers this, and pops up whenever that song loads and displays your bookmarks at 3:02 and 6:73 that you can then click to immediately hear those sections. It has a few other options as well.

The Amarok skin is, like the Vidamp skin, heavily based on PLEDIT.BMP. This skin is quite simple, and most of it is self-explanatory if you are familiar with PLEDIT.BMP and if you have a basic understanding of how the plugin works. Two things to remember, however, are:

The colours for the inside of the plugin are taken from PLEDIT.TXT

Like in PLEDIT.BMP, there are two sections on the bottom of the window (but at the top-right of the BMP) that appear when the window is stretched - a longer section and a shorter section. When the PLAYLIST is stretched, the longer section sticks to the right side of this extended area, while the left side is filled up by the smaller section, which is tiled repeatedly. On the AMAROK skin, the reverse is true: the larger section sticks to the left side, while the tiled section fills up the right side.

When testing your Amarok skin on Winamp, remember that in order for it to load, you need to load a different skin, and then reload the one with your Amarok skin. This seems to be the case for all skinnable plugins.

You can get the Amarok plugin here (118kb):
http://classic.winamp.com/plugins/detail.jhtml?componentId=6960


VIDAMP.BMP    (280×174)     template and description by ¤db¤
¯¯¯¯¯¯¯¯¯¯
This is the skin file for a popular plugin called VidAmp, which lets you watch video files (MPG, AVI, MOV, etc.)
with Winamp. This file is heavily based on PLEDIT.BMP, so see that file's notes above for more details. There are several differences between this file and PLEDIT.BMP:

Firstly, the inside colour is taken not from PLEDIT.TXT, but from a tile in VIDAMP.BMP (it's the dark blue tile in this template). However, this does not mean that you can create a tiled pattern for VidAmp's background - it only reads one pixel and paints the entire background that colour.

Secondly, there is no windowshade mode, but a button - simliar to the windowshade button in PLEDIT.BMP - is used for toggling the "fullscreen" mode on and off. The various states of this button are shown within the template, but be warned: one of vidamp's many bugs is that it sometimes reverses the way these buttons work (eg. the "off" button might suddenly begin to turn "fullscreen" mode "on").

Thirdly, and this is the major difference, the bottom of the file is different. There are no stacks - there are 7 buttons similar to the CBUTTONS, and 2 buttons similar to the SHUFREP buttons (in that they have 4 states each). The state of each is shown in the template.

As you can clearly see, the vidamp skin was based on PLEDIT.BMP (and somewhat sloppily). A few things in VIDAMP.BMP, left over from PLEDIT.BMP, actually serve no function (as far as I can tell!) I have left them in, however, as they appear in the original VidAmp template.

When testing your VidAmp skin on Winamp, remember that in order for it to load, you need to load a different skin, and then reload the one with your VidAmp skin. This seems to be the case for all skinnable plugins.

You can get the VidAmp plugin here (112kb):
http://classic.winamp.com/plugins/detail.jhtml?componentId=22629


CURSORS
¯¯¯¯¯¯¯
Winamp lets you define custom cursors, which will be used by winamp and only by winamp (when the mouse is over any of winamp's windows, except the minibrowser and avs). If you don't want to make custom cursors, don't include any, winamp will use the default ones. These are the different cursors and where should the mouse be for them to be shown:

normal.cur:   Main window except titlebar, song title, volume, balance and posbar.
titlebar.cur: Main window, titlebar except app-buttons.
close.cur:    Main window, close button.
winbut.cur:   Main window, winshade button.
min.cur:      Main window, minimize button.
mainmenu.cur: Main window, menu button (top left).
volbal.cur:   Main window, volume and balance controls.
posbar.cur:   Main window, posbar.
songname.cur: Main window, song title.

wsnormal.cur: Main window, winshade mode except appbuttons and posbar.
wsposbar.cur: Main window, winshade mode, posbar.
mmenu.cur:    Main window, winshade mode, menu button (far left).

eqnormal.cur: Equalizer window except titlebar and sliders.
eqtitle.cur:  Equalizer window, titlebar and winshade mode except close button.
eqclose.cur:  Equalizer window, close button.
eqslid.cur:   Equalizer window, sliders.

pnormal.cur:  Playlist window except titlebar, resize button and scroll bar.
ptbar.cur:    Playlist window, titlebar except app-buttons.
pclose.cur:   Playlist window, close button.
pwinbut.cur:  Playlist window, winshade button.
pvscroll.cur: Playlist window, scroll bar.
psize.cur:    Playlist window, resize button.

pwsnorm:      Playlist window, winshade mode except app-buttons and resize button.
pwssize:      Playlist window, winshade mode, resize button.

Note: The cursors used in the playlist window are used in the minibrowser, video, media library and AVS windows too.

There are usually 4 more cursors in the skins: volbar.cur, wsclose.cur, wswinbut.cur, wsmin.cur, but they are never used, at least in the last versions of winamp, so there's no need of including them. The cursors shown when the mouse is over the app-buttons are the same in normal and winshade mode, except for the main menu button. You can make animated cursors, but you have to name them with the extension .cur (animated cursors are usually .ani files).

You may not want to make 24 different cursors but a 'normal' cursor, a 'titlebar' cursor, a 'vertical slider' cursor, a 'close' cursor... If that's the case you can use a little utility made by skinme! called CursorMultiply. I'm not explaining how this works because it's everything explained inside it, just extract the file CursorMultiply.zip and read its readme.


SKELETONS.BMP
SKELETONS2.BMP
¯¯¯¯¯¯¯¯¯¯¯¯¯¯
These files are NOT part of the skin, you shouldn't include them when you publish your skin, they are intended merely as a tool. They show all the windows of winamp so that you can paint in these files and then crop the different pieces and components into the appropriate files, although you'd still have to make the pressed state of the buttons and other things. In skeletons.bmp you'll find the playlist window horizontally and vertically stretched, so that all the different tiles and stacks are shown. In skeletons.bmp all the windows are the same size (except avs) and the layout is the "standard" one. Depending on the kind of skin you are working on, you may like one of the skeletons files better better than the other.
