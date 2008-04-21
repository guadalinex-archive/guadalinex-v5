{$IFNDEF FPC}
{$APPTYPE CONSOLE}
{$ENDIF}

program dumphive;


(* dumphive: parsen von win9x/nt-registry-hives und textuelle ausgabe
   deren inhalt in eine datei mit DOS-zeilenenden

   weitere informationen finden sich in LICENSE, winreg.txt
   und DUMPHIVE.txt.

   (C)2000-2004 Markus Stephany, merkes_at_mirkes.de, BSD-lizensiert

   Microsoft, Windows, Windows NT sind Markenzeichen der Microsoft
   Corporation.
   NT ist eine Marke von Northern Telecom Limited.

   - v0.01 06. august 2000

   - v0.0.2 august 2001
     - kompiliert nun auch unter delphi
     - alle 32 bit integer-werte werden nun als vorzeichenlose 32 bit-werte ausgegeben

   - v0.0.3 02. märz 2004
     - delphi 6-kompatibel
     - schnellere ausgabe von grossen hexblöcken
     - natives handling von unicode->ansi unter delphi/win32
     - optionale ausgabe des security-descriptors im sddl-format in der
       delphi-version (schalter -S)
     - optionale auflösung der sids nach namen in der delphi-version (schalter -N)

   - v 06-28-2004, 28. juni 2004
     - delphi 7-kompatibel

   - v 07-31-2004, 31. juli 2004
     - lizenz von gpl nach bsd geändert
     - hex-daten-ausgabe in kleinschrift
     - maskieren von backslashes und doppelten anführungszeichen in wert-namen
       mittels backslash
     - "lh"-strukturen in nt-hives werden unterstützt
     - unicode-wertnamen werden unterstützt
     - inline-vks werden nur noch bei unbenannten daten nach REG_DWORD
       konvertiert
     - bei REG_SZ-ausgabe als hexdaten (hex(1)) in nt-hives im unicode-format
     - wg. fpc-kompabilität einige variablen von integer nach longint geändert

   credits an petter nordahl hagen, http://home.eunet.no/~pnordahl
   (vom quellcode zu seinem chntpw habe ich viele der definitionen
    hier adoptiert)

           und den unbekannten verfasser von winreg.txt
*)

// ansistrings
{$H+}

uses
{$IFNDEF FPC}
  dumphive_comm in 'dumphive_comm.pas',
  creg_def in 'creg_def.pas',
  creg_procs in 'creg_procs.pas',
  regf_def in 'regf_def.pas',
  regf_procs in 'regf_procs.pas';
{$ELSE}
  dumphive_comm,
  creg_def,
  creg_procs,
  regf_def,
  regf_procs;
{$ENDIF}

const
{$IFDEF fpc}
  BUILD = '-fpc';
{$ELSE}
{$IFDEF VER140}
  BUILD = '-d6';
{$ELSE}
{$IFDEF VER150}
  BUILD = '-d7';
{$ELSE}
  BUILD = '';
{$ENDIF}
{$ENDIF}
{$ENDIF}

// titel ausgeben

procedure PrintTitle;
begin
  writeln;
  writeln('dumphive v 07-31-2004' + BUILD +
    ': dumpt einen win9x/nt-registry-hive in eine textdatei');
  writeln('  (c)2000-2004 Markus Stephany, merkes_at_mirkes.de');
end;

// verwendung ausgeben

procedure PrintUsage;
begin
  PrintTitle;
  writeln;
{$IFDEF _INCDEBUG_ }
{$IFNDEF fpc}
  writeln('Usage: dumphive -V | -h | [-d] [-e] [-S|-N] [--] <hive> <target> [<prefix>]');
{$ELSE}
  writeln('Usage: dumphive -V | -h | [-d] [-e] [--] <hive> <target> [<prefix>]');
{$ENDIF}
{$ELSE}
{$IFNDEF fpc}
  writeln('Usage: dumphive -V | -h | [-e] [-S|-N] [--] <hive> <target> [<prefix>]');
{$ELSE}
  writeln('Usage: dumphive -V | -h | [-e] [--] <hive> <target> [<prefix>]');
{$ENDIF}
{$ENDIF}
end;

procedure Usage;
begin
  PrintUsage;
  Halt(1);
end;


//hilfe ausgeben

procedure Help;
begin
  PrintUsage;
  writeln;
  writeln('dumphive exportiert den inhalt eines windows-registry-hives in eine textdatei.');
  writeln;
  writeln('dumphive kann (oder versucht es zumindest) win9x-hives (wie z.b. system.dat');
  writeln('oder user.dat) und winnt-hives (software, ntuser.dat usw.) lesen und deren');
  writeln('inhalt in eine regedit-kompatible textdatei ausgeben.');
  writeln;
  writeln('** kommandozeilenparameter:');
  writeln('-V: gibt die version aus. -h: gibt diese hilfeseite aus.');
  writeln('-e    : verwendet ein nicht regedit-kompatibles format, das mehr');
  writeln('        informationen enthaelt (klassennamen der schluessel usw.)');
{$IFNDEF fpc}
  writeln('-S    : ausgabe des security-descriptors im sddl-format (nur mit -e)');
  writeln('-N    : auflösung der sid''s nach namen (nur mit -e, ohne -S)');
{$ENDIF}
{$IFDEF _INCDEBUG_ }
  writeln('-d    : ausgabe von debugging-informationen');
{$ENDIF}
  writeln('hive  : name des quellhives (= der registrierungsdatei)');
  writeln('target: name der zu erzeugenden text-datei (enthaelt immer dos-zeilenenden)');
  writeln('prefix: optionaler name, der den schluesselnamen vorangestellt wird.');
  writeln('[mit ENTER geht''s weiter.]');
  readln;
  writeln;
  PrintTitle;
  writeln;
  writeln('** copyright:');
  writeln('das copyright an dumphive liegt bei markus stephany, merkes_at_mirkes.de');
  writeln('(C)2000-2004 M. Stephany (BSD lizensiert)');
  writeln;
  writeln('Microsoft, Windows, Windows NT sind Markenzeichen der Microsoft Corporation.');
  writeln('NT ist eine Marke von Northern Telecom Limited.');
  writeln;
  writeln('** credits:');
  writeln('- an den unbekannten verfasser von winreg.txt');
  writeln('- petter nordahl hagen, dessen quellcode zu chntpw mir sehr weitergeholfen hat');
  writeln('  (http://home.eunet.ho/~pnordahl)');
  writeln;
  writeln('weitere informationen finden sich in DUMPHIVE.txt.');
  writeln;
  Halt(0);
end;

var
  args: TStringArray; // ausgewertete parameter
  i: LongInt; // laufveraiable

{$IFNDEF fpc}
var
{$ELSE}
const
{$ENDIF}
  szFile: string = ''; // dateiname des einzulesenden hives
  szPrefix: string = ''; // pfad-präfix (falls angegeben)
  szTarget: string = ''; // zieldateiname zur aufnahme des geparsten hives

const
  MyOpts:
    array[0..{$IFDEF _INCDEBUG_ }{$IFNDEF fpc}5{$ELSE}3{$ENDIF}{$ELSE}{$IFNDEF fpc}4{$ELSE}2{$ENDIF}{$ENDIF}] of TOption = (
    (oOpt: 'e'; oCase: False; oVal: otNone), //extended
    (oOpt: 'V'; oCase: True; oVal: otNone), //Version
    (oOpt: 'h'; oCase: False; oVal: otNone){$IFDEF _INCDEBUG_ }, //help
    (oOpt: 'd'; oCase: False; oVal: otNone){$ENDIF}{$IFNDEF fpc}, //debug
    (oOpt: 'S'; oCase: True; oVal: otNone), // sddl security output (mit -e)
    (oOpt: 'N'; oCase: True; oVal: otNone)
      // auflösung sid->name (mit -e, ohne -S)
{$ENDIF}

    ); // an kommandozeile verwendete optionen

begin
  ParseArgs(MyOpts, args, @Usage); // argumente auswerten

  if args.Count > 0 then
  begin
    i := 0;
    while Cardinal(i) < args.Count do
    begin
      case args.Strs[i][1] of
{$IFDEF _INCDEBUG_ }
        'd': _DEBUGPR := True;
{$ENDIF}
{$IFNDEF fpc}
        'S': _SDDL := True;
        'N': _SID2NAME := True;
{$ENDIF}
        'e': _EXTENDED := True;
        'h': help;
        'V':
          begin
            writeln(DHVER);
            Halt(0);
          end;
        '1': szFile := args.Strs[i + 1];
        '2': szTarget := args.Strs[i + 1];
        '3': szPrefix := args.Strs[i + 1];
      else
        Usage;
      end;
      Inc(i, 2);
    end;
  end;

  if szFile = '' // kein quelldateiname angegeben
    then
    Usage;

  if szTarget = '' // kein zieldateiname angegeben
    then
    Usage;

  if szPrefix <> '' // backslash an evtl. präfix anhängen
    then
    if szPrefix[Length(szPrefix)] <> '\'
      then
      szPrefix := szPrefix + '\';

  PrintTitle; // titel ausgeben

  if IsREGFHive(szFile) // falls REGF-signatur erkannt wird (bei nt-hives)
    then
    OutputREGF(szFile, szTarget, szPrefix) // inhalt dumpen
  else
    if IsCREGHive(szFile) // falls CREG-signatur erkannt wird (bei win9x-hives)
      then
      OutputCREG(szFile, szTarget, szPrefix) // inhalt dumpen
    else
      Die(['Unbekannter Dateityp in "', szFile, '"']);
        // keine bekannte signatur gefunden
end.

