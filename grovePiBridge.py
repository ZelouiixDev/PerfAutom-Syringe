import time
from grovepi import *
from grove_rgb_lcd import *
import sys

# Arguments :
# 1 : screen ou led (type de composant)
# 2 : numero de l'ecran ou numero de la led
# 3 : valeur a afficher sur l'ecran ou 1 ou 0 (allumer ou eteindre la led)

def interract_led(num_led, etat_led):
    try:
        num_led = int(num_led)
        etat_led = int(etat_led)
        #print("Passage de la led " + str(num_led) + " a " + str(etat_led))
        pinMode(num_led, "OUTPUT")
        time.sleep(0.2)
        try:
            #Blink the LED
            digitalWrite(num_led, etat_led)

        except KeyboardInterrupt:	# Turn LED off before stopping
            digitalWrite(led,0)
        except IOError:				# Print "Error" if communication error encountered
            print ("Error")
    except Exception as e:
        print("Erreur led : " + str(e))

def interract_screen(valeur, quantite_restante):
    try:
        setRGB(0, 128, 64)
        setRGB(0, 255, 0)
        setText(valeur + " ml/h\nIl reste " + quantite_restante + " ml");
    except Exception as e:
        print("Erreur ecran LCD : " + str(e))

if sys.argv[1] == "led":
    interract_led(sys.argv[2], sys.argv[3])
if sys.argv[1] == "screen":
    interract_screen(sys.argv[2], sys.argv[3])

