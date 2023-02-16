#!/bin/python3
import  subprocess, sys,os
import colorama
from colorama import Fore

import jwt



data = sys.argv[1] # get data from arg 1 

print(Fore.YELLOW+"received data in JSON =>",data)
try:
        
    encoded =jwt.decode(data, "TMC", algorithms=['HS256'])
    #jwt.decode(data,"TMC", algorithms=['HS256'])
    print(Fore.WHITE+" data decoded from JSON  => ",encoded)
    data=encoded['data']
    print(Fore.GREEN+" The  Data is  ",data)

except:
        exit(1)
