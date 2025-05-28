# Discord Gotcha Tracker Bot

This is a bot designed for a discord server. I specifically designed this bot for the Kennesaw State Clarinet section marching band discord. This bot scans a specific discord channel id specified by 

    const dpp::snowflake GOTCHA_CHANNEL_ID

When a message is sent in that specific channel the bot checks 2 conditions:

 - Does the message contain a mention?
 - Does the message contain an image?

If yes, the bot records the message author and the intended target (mentioned) and compares it to a hashmap (importing a .csv to a hashmap coming soon!)
The hashmap contains a list of the server's discord names matched up to their real names as they appear on a spreadsheet.
The bot checks this list and finds the appropriate name in the spreadsheet. We are using the OpenXLSX library to manage this. Once the name cell is located we traverse one cell to the right to the `Gotcha!` tab and increment the value by 1 as a point.
Then we go to the target's name and do the same thing there but increment the target's `Got Got!` cell by 1. 

There is a bonus category too for the Band director Dr. Brandon Meeks which is scored by triggering the bot. 

# Requirements

 - vcpkg
 - Discord++ (DPP)
 - OpenXLSX

## Compilation Instructions

 The following instructions are designed for Arch-like systems

    $ git clone https://github.com/crystalstorm466/Discord-Gotcha-Tracker.git
    $ cd Discord-Gotcha-Tracker
    $ ~/vcpkg/vcpkg install dpp
    $ sudo yay -S openxlsx
    $ make clean
    $ make

## Contributing & To-Do Lists
Feel free to contribute any code you would like!

 - Read list of DiscordNames->RealLifeNames from a .csv file into a Hashmap instead of hardcoding
 - Make it more boilerplate for other servers to use
 - Get an automatic upload going instead of using crontab and rsync.


