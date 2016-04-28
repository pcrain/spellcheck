#!/usr/bin/python
#Checks output of spellcheck against input and reference files

import sys

class col:
  BLN      ='\033[0m'            # Blank
  UND      ='\033[1;4m'          # Underlined
  INV      ='\033[1;7m'          # Inverted
  CRT      ='\033[1;41m'         # Critical
  BLK      ='\033[1;30m'         # Black
  RED      ='\033[1;31m'         # Red
  GRN      ='\033[1;32m'         # Green
  YLW      ='\033[1;33m'         # Yellow
  BLU      ='\033[1;34m'         # Blue
  MGN      ='\033[1;35m'         # Magenta
  CYN      ='\033[1;36m'         # Cyan
  WHT      ='\033[1;37m'         # White

if len(sys.argv) < 2:
  print("Please pass name of corpus to check")
  sys.exit(-1)

corpus=sys.argv[1]

with open(corpus,"r") as fin:
  inlines=fin.read().split("\n")

with open(corpus+".scout","r") as fin:
  outlines=fin.read().split("\n")

with open(corpus+"ref","r") as fin:
  reflines=fin.read().split("\n")

count=len(inlines)-1
good=0
train=0
lazy=0
wrong=0

for i in range(0,count):
  a = inlines[i]
  b = outlines[i]
  c = reflines[i]
  if ((a == b) and (b == c)): #All good
    print("* "+col.GRN+a+" "*(20-len(a))+b+" "*(20-len(b))+c+col.BLN)
    good += 1
  elif (b == c): #Good correction
    print("* "+a+" "*(20-len(a))+col.GRN+b+" "*(20-len(b))+c+col.BLN)
    good += 1
  elif (a == c): #Corrected when it shouldn't have been
    print("+ "+a+" "*(20-len(a))+col.MGN+b+col.BLN+" "*(20-len(b))+c)
    train += 1
  elif (a == b): #Not corrected when it should have been
    print("- "+col.YLW+a+" "*(20-len(a))+b+col.BLN+" "*(20-len(b))+c)
    lazy += 1
  else: #Corrected to the wrong thing
    print("! "+a+" "*(20-len(a))+col.RED+b+col.BLN+" "*(20-len(b))+c)
    wrong += 1

clen = len(str(count)*2)

print("-----")
print(col.GRN+
  "* "+str(good)+"/"+str(count)+" "*(clen-len(str(good)))+
  ("%.2f" % ((good*100)/count)).zfill(5)  +
  "%"+" properly corrected   (all good)"+col.BLN)
print(col.RED+
  "! "+str(wrong)+"/"+str(count)+" "*(clen-len(str(wrong)))+
  ("%.2f" % ((wrong*100)/count)).zfill(5) +
  "%"+" improperly corrected (work on heuristics / word data)"+col.BLN)
print(col.YLW+
  "- "+str(lazy)+"/"+str(count)+" "*(clen-len(str(lazy)))+
  ("%.2f" % ((lazy*100)/count)).zfill(5)  +
  "%"+" left uncorrected     (filter junk data better)"+col.BLN)
print(col.MGN+
  "+ "+str(train)+"/"+str(count)+" "*(clen-len(str(train)))+
  ("%.2f" % ((train*100)/count)).zfill(5) +
  "%"+" hastily corrected    (get more word data)"+col.BLN)
