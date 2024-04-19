# Detect Wh Home
This is a C program written for the NodeMCU Amica to detect Wh consumed in my house.

## Problem
In italy is not common to have a smart wh detector, 

## Solution
So I build a small program to:
- save each time the led of the electricity meter blinks <- that means 1Wh has been consumed
- write it on a file to compile some daily stats
- send those stats with Telegram
