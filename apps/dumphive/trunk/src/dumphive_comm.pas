unit dumphive_comm;

(* allgemeine typen,konstanten und routinen fuer dumphive
   (C)2000-2004 Markus Stephany, merkes_at_mirkes.de, BSD-lizensiert

   Microsoft, Windows, Windows NT sind Markenzeichen der Microsoft
   Corporation.
   NT ist eine Marke von Northern Telecom Limited.

   - v0.01 06. august 2000: erste version
   - v0.02 02. märz 2004:   unter delphi/win32 natives handling von
                            unicode->ansi, schnellere ausgabe von
                            grossen hex-blöcken durch zeilenweise ausgabe,
                            sddl-format von security-descriptoren unter
                            delphi/win32
   - v07-31-2004, 31. juli 2004:
                            hex-daten-ausgabe in kleinschrift
                            maskieren von backslashes und doppelten
                            anführungszeichen in wert-namen mittels backslash
                            wg. fpc-kompabilität einige variablen von integer
                            nach longint geändert

*)

// ansi-strings ein
{$H+}

interface

type
  // klappt nur, falls kein range-checking
  // "dynamisches" char-array
  PCharArray = ^TCharArray;
  TCharArray = array[0..0] of Char;

  // klappt nur, falls kein range-checking
  // "dynamisches" byte-array
  PByteArray = ^TByteArray;
{$IFDEF fpc}
  TByteArray = array[0..0] of Byte;
{$ELSE}
  TByteArray = array[0..32768] of Byte;
{$ENDIF}

  // win32-Typen
  PDWORD = ^DWORD;
  DWORD = Cardinal;
  PWORD = ^WORD;
  PInteger = ^Integer;

  // halb-dynamisches string-array
  TStringArray = record
    Count: Cardinal; // anzahl der gültigen einträge
    Strs: array[0..20] of string; // daten
  end;

  // halb-dynamisches DWORD-Array
  PDWORDArray = ^TDWORDArray;
  TDWORDArray = record
    Count: Cardinal;
    Ofsets: array[0..0] of DWORD;
  end;

  // typ einer option an kommandozeile
  TOptType = (
    otNone, // option ohne parameter und ohne ein/aus
    otSwitch, // option ohne parameter, aber mit ein/aus (-o / +o)
    otInteger, // option mit numerischem parameter
    otString // option mit string-parameter
    );

  // definition einer erlaubten option
  TOption = record
    oOpt: Char; // name der option (ohne -+/)
    oCase: Boolean; // ist die option case-sensitive ("o" <> "O" ?)
    oVal: TOptType; // typ der option
  end;

  // struktur zum ausgeben eines registry-wertes
  TVKRecord = record
    len_name, // länge des wert-namens
      len_data, // größe der daten
      type_data: DWORD; // typ des wertes
    Data: Pointer; // zeiger auf daten
  end;

  // struktur mit allgemeinen daten des hives
  PHIVE_RECORD = ^THIVE_RECORD;
  THIVE_RECORD = packed record
    FileName, // dateiname des hives
      Prefix: string; // welcher custom-path wird vorangestellt
    Size: Cardinal; // größe des hives
    RootKey: Cardinal; // ofset auf rootkey bzw. 1. RGDB-Eintrag
    hrNum: Cardinal; // anzahl der einträge
    Buffer: PByteArray; // zeiger auf hive im speicher
  end;

  // zur parameterübergabe einer <void procedure()>
  TProcedure = procedure;

const
  // diese version
  DHVER = 'DUMPHIVE0.003';

  // dos-linefeed
  LINEFEED = #13#10;

  // registry-konstanten (wert-typen)
  REG_NONE = 0;
  REG_SZ = 1;
  REG_EXPAND_SZ = 2;
  REG_BINARY = 3;
  REG_DWORD = 4;
  REG_DWORD_LITTLE_ENDIAN = 4;
  REG_DWORD_BIG_ENDIAN = 5;
  REG_LINK = 6;
  REG_MULTI_SZ = 7;
  REG_RESOURCE_LIST = 8;
  REG_FULL_RESOURCE_DESCRIPTOR = 9;
  REG_RESOURCE_REQUIREMENTS_LIST = 10;

{$IFNDEF fpc}
var
{$ELSE}
const
{$ENDIF}

  // ist TRUE, falls '-e' als option mitgegeben wurde, dann wird ein anderes ausgabeformat verwendet
  _EXTENDED: Boolean = False;


  // ist TRUE, falls '-d' als option angegeben wird, dann werden debug-infos ausgegeben
{$IFDEF _INCDEBUG_ }
  _DEBUGPR: Boolean = False;
{$ENDIF}

{$IFNDEF fpc}
  // ist TRUE, falls '-S' als option angegeben wird, dann werden security-descriptoren
  // im sddl-format ausgegeben
  _SDDL: Boolean = False;
  // ist TRUE, falls '-N' als option angegeben wird, dann werden SIDs in
  // security-descriptoren in namen aufgelöst
  _SID2NAME: Boolean = False;
{$ENDIF}

  // ist TRUE, falls es sich um einen REGF(winnt)-Hive handelt, FALSE bei win9x-hive
  // flag wird benötigt, um zu entscheiden, ob von UniCode nach ASCII gewandelt werden muss
  _ISREGF: Boolean = False;

{$IFDEF fpc}
  // von windows2000 genutzte unicode-tabelle (deutsche code-page, nur ansi0-255, low_u/high_u/ansi)
  UniTab: array[0..80] of Byte = (
    $AC, $20, $80, $1A, $20, $82, $92, $01, $83, $1E, $20, $84, $26, $20, $85,
      $20, $20, $86, $21, $20, $87,
    $C6, $02, $88, $30, $20, $89, $60, $01, $8A, $39, $20, $8B, $52, $01, $8C,
      $7D, $01, $8E,
    $18, $20, $91, $19, $20, $92, $1C, $20, $93, $1D, $20, $94, $22, $20, $95,
      $13, $20, $96, $14, $20, $97,
    $DC, $02, $98, $22, $21, $99, $61, $01, $9A, $3A, $20, $9B, $53, $01, $9C,
      $7E, $01, $9E, $78, $01, $9F
    );
{$ENDIF}

var
  // zieldatei
  fTarget: Text;

// optionen holen, argumente zurückgeben
procedure ParseArgs(const Opts: array of TOption;
  var Args: TStringArray;
  pOnError: TProcedure);

// ausführung mit fehlerausgabe abbrechen
procedure Die(const args: array of string);

// checken, ob ein wert weder $00000000 noch $ffffffff ist
function IsValid(const dw: DWORD): Boolean; overload;
function IsValid(const w: WORD): Boolean; overload;

// VKRecord-Eintrag ausgeben (wert in registry)
procedure OutputVKRec(Rec: TVKRecord; vn: string);

// einfaches konvertieren zwischen unicode und ansi, nur bei REGF ( nur für deutschen ANSI-Charset!!)
// unter delphi: verwenden der internen unicode -> ansi-konvertierung
function Uni2Ansi(s: string): string;

// key-Namen erzeugen (führende und abschließende backslashes entfernen)
function MakeKeyName(s: string): string;

// LongInt (DWORD) nach string wandeln
function ToStr(li: LongInt): string;

// string nach DWORD wandeln (oder default zurückgeben bei fehler)
function StrToDWORDDef(s: string; d: DWORD): DWORD;

// ersten anteil eines strings bis zum ersten auftreten von #0 zurückgeben
function Until0(s: string): string;

// zeile in target schreiben (dos-konvention #13#10 als abschluss)
procedure TargetLN(const s: string);

// wert in uppercase Hex wandeln
function HexString(i, min: LongInt): string;

function BinString(const iVal: LongInt; bBits: Byte): string;

implementation

// string zum TStringsArray hinzufügen

procedure AddToStrings(const s: string; var args: TStringArray);
begin
  if s <> '' then
  begin
    args.Count := args.Count + 1;
    args.Strs[Pred(args.Count)] := s;
  end;
end;

type
  TCheckOption = (coUnknown, coWrong, coSet, coNext);

// überprüfen, ob der paramstr eine gültige option ist

function CheckOption(const Opts: array of TOption;
  const Arg: string;
  const iCurrent: Integer;
  var Res: string;
  var WhatOpt: Integer): TCheckOption;
var
  i1: Integer;
  ss: string;
begin
  CheckOption := coUnknown;
  WhatOpt := -1;
  if (High(Opts) >= Low(Opts)) and (Length(Arg) > 1)
    then
    for i1 := Low(Opts) to High(Opts)
      do
      if ((not Opts[i1].oCase) and (UpCase(Arg[2]) = UpCase(Opts[i1].oOpt))) or
        ((Opts[i1].oCase) and (Arg[2] = Opts[i1].oOpt)) then
      begin
        WhatOpt := i1;
        Break;
      end;
  if WhatOpt > -1
    then
    with Opts[WhatOpt] do
    begin
      CheckOption := coWrong;
      case oVal of
        otNone:
          if Length(Arg) = 2
            then
            if Arg[1] in ['-'{$IFNDEF LINUX}, '/'{$ENDIF}] then
            begin
              Res := '1';
              CheckOption := coSet;
            end;
        otSwitch:
          if Length(Arg) = 2
            then
            case Arg[1] of
              '+', '/':
                begin
                  Res := '1';
                  CheckOption := coSet;
                end;
              '-':
                begin
                  Res := '0';
                  CheckOption := coSet;
                end;
            end;
        otInteger:
          begin
            if Length(Arg) > 2 then
            begin
              if Arg[3] <> ':'
                then
                Exit
              else
                ss := Copy(Arg, 4, MaxInt);
              CheckOption := coSet;
            end
            else
            begin
              if iCurrent = ParamCount
                then
                Exit
              else
                ss := ParamStr(iCurrent + 1);
              CheckOption := coNext;
            end;
            if StrToDWORDDef(ss, 4) = 4
              then
              if StrToDWORDDef(ss, 5) = 5
                then
                CheckOption := coWrong
              else
                Res := ss;
          end;
        otString:
          begin
            if Length(Arg) > 2 then
            begin
              if Arg[3] <> ':'
                then
                Exit
              else
                ss := Copy(Arg, 4, MaxInt);
              CheckOption := coSet;
            end
            else
            begin
              if iCurrent = ParamCount
                then
                Exit
              else
                ss := ParamStr(iCurrent + 1);
              CheckOption := coNext;
            end;
            Res := ss;
          end;
      end;
    end;
end;

// optionen holen, argumente zurückgeben

procedure ParseArgs(const Opts: array of TOption;
  var Args: TStringArray;
  pOnError: TProcedure);
var
  Vars: TStringArray;
  i, n: Integer;
  p, s: string;
  bEnd: Boolean;
begin
  Args.Count := 0;
  Vars.Count := 0;
  if ParamCount > 0 then
  begin
    i := 1;
    bEnd := False;
    while i <= ParamCount do
    begin
      if (not ((ParamStr(i) + #0)[1] in ['-', '+'{$IFNDEF LINUX}, '/'{$ENDIF}]))
        or bEnd
        then
        AddToStrings(ParamStr(i), Vars)
      else
      begin
        p := ParamStr(i);
        if p = '--'
          then
          bEnd := True
        else
        begin
          if Length(p) < 2
            then
            pOnError;
          case CheckOption(Opts, p, i, s, n) of
            coUnknown:
              begin
                writeln;
                writeln('unbekannte option "', p, '"');
                pOnError;
              end;
            coWrong:
              begin
                writeln;
                writeln('falsches argument in "', p, '"');
                pOnError;
              end;
            coSet:
              begin
                AddToStrings(Opts[n].oOpt, Args);
                AddToStrings(s, Args);
              end;
            coNext:
              begin
                AddToStrings(Opts[n].oOpt, Args);
                AddToStrings(s, Args);
                Inc(i);
              end;
          end;
        end;
      end;
      Inc(i);
    end;
  end;
  if Vars.Count > 0
    then
    for i := 0 to Pred(Vars.Count) do
    begin
      AddToStrings(ToStr(i + 1), Args);
      AddToStrings(Vars.Strs[i], Args);
    end;
end;



// systemfehler ausgeben und abbrechen

procedure Die(const args: array of string);
var
  cError: Cardinal;
  i: Integer;
begin
  cError := {$IFDEF fpc}errorcode{$ELSE}GetLastError{$ENDIF};
  writeln;
  for i := Low(args) to High(args)
    do
    write(args[i]);
  if cError <> 0
    then
    write(' systemfehler: 0x', HexString(cError, 4))
  else
    cError := 1;
  writeln;
  halt(cError);
end;

// checken, ob ein wert weder $00000000 noch $ffffffff ist

function IsValid(const dw: DWORD): Boolean; overload;
begin
  IsValid := (dw <> 0) and (dw <> $FFFFFFFF);
end;

// checken, ob ein wert weder $0000 noch $ffff ist

function IsValid(const w: WORD): Boolean; overload;
begin
  IsValid := (w <> 0) and (w <> $FFFF);
end;


{$IFDEF fpc}
// word in uinicode-tabelle suchen und passenden ansi-wert zurückgeben, falls nicht gefunden, dann '?'

function GetFromUniTab(const w: word): Char;
var
  res: Char;
  i: Integer;
begin
  res := '?';
  i := Low(UniTab);
  repeat
    if PWORD(@UniTab[i])^ = w then
    begin
      res := Char(UniTab[i + 2]);
      Break;
    end;
    Inc(i, 3);
  until i >= High(UniTab);
  GetFromUniTab := res;
end;

// einfaches konvertieren zwischen unicode und ansi, nur bei REGF ( nur für deutschen ANSI-Charset!!)

function Uni2Ansi(s: string): string;
var
  i, j: LongInt;
  res: string;
  w: word;
  ab: packed array[0..1] of Char absolute w;
begin
  if not _ISREGF then
    res := s
  else
  begin
    j := Length(s) div 2;
    SetLength(res, j);
    for i := 0 to Pred(j) do
    begin
      Move(s[(i*2)+1], w, 2);
      // überprüfen, ob hi = 0, dann einfach übernehmen
      if ab[1] = #0 then
        res[i+1] := ab[0]
      else
        // zeichen suchen in UniCodeTabelle
        res[i+1] := GetFromUniTab(w);
    end;
  end;
  Uni2Ansi := res;
end;
{$ELSE}

// unter delphi: verwenden der internen unicode -> ansi-konvertierung
function Uni2Ansi(s: string): string;
var
  ws: WideString;
begin
  if (not _ISREGF) or (Length(s) = 0) then
    Result := s
  else
  begin
    SetString(ws, PWideChar(@s[1]), Length(s) div 2);
    Result := ws;
    if (Length(s) mod 2) = 1 then
      Result := Result + s[Length(s)];
  end;
end;
{$ENDIF}

// backslahes und doppelte anführungszeichen quoten (mit backslash)

function CheckSpecialChars(sIn: string): string;
var
  i: integer;
  bQuoted: boolean;
  sRes: string;
begin
  sRes := '';
  bQuoted := False;
  if (Copy(sIn, 1, 1) = '"') and (sIn[Length(sIn)] = '"') then
  begin
    Delete(sIn, 1, 1);
    Delete(sIn, Length(sIn), 1);
    bQuoted := True;
  end;
  for i := 1 to Length(sIn) do
    if sIn[i] in ['\', '"'] then
      sRes := sRes + '\' + sIn[i]
    else
      sRes := sRes + sIn[i];
  if bQuoted then
    sRes := '"' + sRes + '"';
  CheckSpecialChars := sRes;
end;

// VKRecord-Eintrag ausgeben

procedure OutputVKRec(Rec: TVKRecord; vn: string);
var
  sData, sOut, sUni: string;
  AsHex: Boolean;
  c1, c2: Cardinal;
begin
  sOut := CheckSpecialChars(vn) + '=';
  with Rec do
  begin
    if type_data in [REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ] then
    begin
      SetString(sData, PChar(Data), len_data);
      sUni := sData;
      sData := Uni2Ansi(sData);
      if (type_data = REG_SZ) or (_EXTENDED and (type_data = REG_EXPAND_SZ))
      then
        while (Length(sData) > 0) and (sData[Length(sData)] = #0)
        do
          SetLength(sData, Pred(Length(sData)));
      if (not _EXTENDED) and (type_data = REG_SZ) then
        sData := PChar(@sData[1]);
      Data := @sData[1];
      len_data := Length(sData);
    end;
    if not _EXTENDED then
    begin
      AsHex := not (type_data in [REG_SZ, REG_DWORD]);
      if type_data = REG_SZ then
        for c1 := 1 to Length(sData) do
          if sData[c1] < #32 then
          begin
            AsHex := True;
            // unicode-daten verwenden, weil hex(1) nicht
            // (anders als REG_EXPAND_SZ) automatisch nach unicode konvertiert
            // wird
            sData := sUni;
            // trimmen
            while (Length(sData) > 0) and (sData[Length(sData)] = #0)
            do
              SetLength(sData, Pred(Length(sData)));
            // gültige unicode-länge sicherstellen
            if (Length(sData) mod 2) <> 0 then
              sData := sData+#0;
            Data := @sData[1];
            len_data := Length(sData);
            Break;
          end
    end
    else
      AsHex := not (type_data in [REG_SZ, REG_DWORD, REG_EXPAND_SZ,
        REG_MULTI_SZ]);
    if type_data = REG_DWORD then
      if len_data <> sizeof(DWORD) then
        AsHex := True;
    if AsHex then
    begin
      sOut := sOut + 'hex';
      if type_data <> REG_BINARY then
        sOut := sOut + '(' + HexString(type_data, 1) + '):'
      else
        sOut := sOut + ':';
      c2 := Length(sOut);
      if len_data > 0 then
        for c1 := 0 to Pred(len_data) do
        begin
          sOut := sOut + HexString(PByteArray(Data)^[c1], 2);
          c2 := c2 + 2;
          if c1 < Pred(len_data) then
          begin
            if c2 > 75 then
            begin
              c2 := 2;
              sOut := sOut + ',\';
              targetln(sOut);
              sOut := '  ';
            end
            else
            begin
              c2 := c2 + 1;
              sOut := sOut + ',';
            end;
          end;
        end;
    end
    else
    begin
      case type_data of
        REG_EXPAND_SZ: sOut := sOut + 'expand:';
        REG_MULTI_SZ: sOut := sOut + 'multi:';
      end;
      case type_data of
        REG_SZ,
          REG_EXPAND_SZ,
          REG_MULTI_SZ:
          begin
            sOut := sOut + '"';
            if len_data > 0
              then
              for c1 := 0 to Pred(len_data) do
              begin
                if PCharArray(Data)^[c1] < #32
                  then
                  sOut := sOut + '\' + HexString(Ord(PCharArray(Data)^[c1]), 2)
                else
                  if PCharArray(Data)^[c1] in ['\', '"']
                    then
                    sOut := sOut + '\' + PCharArray(Data)^[c1]
                  else
                    sOut := sOut + PCharArray(Data)^[c1];
              end;
            sOut := sOut + '"';
          end;
        REG_DWORD: sOut := sOut + 'dword:' + HexString(PDWORD(Data)^, 8);
      end;
    end;
  end;
  targetln(sOut);
end;

//Key-Namen erzeugen (fuehrende und abschliessende backslashes entfernen)

function MakeKeyName(s: string): string;
begin
  while (s <> '') and (s[1] = '\')
    do
    s := Copy(s, 2, MaxInt);
  while (s <> '') and (s[Length(s)] = '\')
  do
    s := Copy(s, 1, Pred(Length(s)));
  MakeKeyName := s;
end;

// LongInt (DWORD) nach string

function ToStr(li: LongInt): string;
var
  res: string;
  dw: DWORD;
begin
  dw := Cardinal(li);
  Str(dw, res);
  ToStr := res;
end;

// string || defaultwert nach DWORD

function StrToDWORDDef(s: string; d: DWORD): DWORD;
var
  c: Integer;
  res: DWORD;
begin
  Val(s, res, c);
  if c <> 0
    then
    res := d;
  StrToDWORDDef := res;
end;

// string bis zum 1. #0-zeichen zurückgeben

function Until0(s: string): string;
var
  i: Integer;
begin
  i := Pos(#0, s);
  if i = 0
    then
    Until0 := s
  else
    Until0 := Copy(s, 1, i - 1);
end;

// zeile in target schreiben (dos-konvention #13#10 als abschluss)

procedure TargetLN(const s: string);
begin
  write(fTarget, s);
  write(fTarget, LINEFEED);
end;


const
  sBaseChars = '0123456789abcdefghijklmnopqrstuvwxyz';
// base conversion

function CardinalToBaseString(c: Cardinal; const bBase: Byte): string;
var
  s: string;
begin
  s := '';
  if (bBase in [2..35])
    then
    while c <> 0 do
    begin
      s := sBaseChars[(c mod bBase) + 1] + s;
      c := c div bBase;
    end;
  CardinalToBaseString := s;
end;

// wert in uppercase Hex wandeln

function HexString(i, min: LongInt): string;
var
  s: string;
begin
  s := CardinalToBaseString(Cardinal((@i)^), 16);
  while Length(s) < min
    do
    s := '0' + s;
  HexString := s;
end;

function BinString(const iVal: LongInt; bBits: Byte): string;
var
  s: string;
begin
  s := CardinalToBaseString(Cardinal((@iVal)^), 2);
  while Length(s) < bBits
    do
    s := '0' + s;
  BinString := s;
end;

begin
end.

