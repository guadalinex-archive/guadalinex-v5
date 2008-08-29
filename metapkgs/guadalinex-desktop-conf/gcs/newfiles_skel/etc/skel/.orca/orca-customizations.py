#!/usr/bin/python
# -*- coding: UTF-8 -*-
import orca.Gecko
import orca.braille as braille
import orca.speech as speech
import orca.keybindings as keybindings
import orca.input_event as input_event
import orca.settings as settings
import os
from orca.orca_i18n import _
import orca.pronunciation_dict
from time import localtime, strftime
orca.Gecko.controlCaretNavigation = True

def decirInfoBateria(script, inputEvent=None):
  f=os.popen("acpi")
  speech.speak(f.read())
  f=os.popen("acpi")
  braille.displayMessage(f.read(80))
  f.close()
  return True

def decirHoraYFecha(script, inputEvent=None):
  speech.speak(strftime("Son las %H y %M, %A, %d de %B de %Y",localtime()))
  braille.displayMessage(strftime("%H:%M | %d/%m/%Y",localtime()))

  return True

decirInfoBateriaHandler = input_event.InputEventHandler(decirInfoBateria, _("Lee el estado de la batería"))
decirHoraYFechaHandler = input_event.InputEventHandler(decirHoraYFecha, _("Dicela hora y fecha"))

teclas = keybindings.KeyBindings()

teclas.add(keybindings.KeyBinding("h", 1<< settings.MODIFIER_ORCA, 1 << settings.MODIFIER_ORCA, decirHoraYFechaHandler))
teclas.add(keybindings.KeyBinding("e", 1<< settings.MODIFIER_ORCA, 1 << settings.MODIFIER_ORCA, decirInfoBateriaHandler))
settings.keyBindingsMap = {}
settings.keyBindingsMap["default"] = teclas

