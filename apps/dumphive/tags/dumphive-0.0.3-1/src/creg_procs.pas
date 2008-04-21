unit creg_procs;

(* prozeduren zum parsen von win9x-registry-hives
   (C)2000-2004 Markus Stephany, merkes_at_mirkes.de, BSD-lizensiert

   Microsoft, Windows, Windows NT sind Markenzeichen der Microsoft
   Corporation.
   NT ist eine Marke von Northern Telecom Limited.

   - v0.01 06. august 2000

   credits an den unbekannten verfasser von winreg.txt
*)

// ansistrings
{$H+}
// kein abkratzen bei io-fehlern
{$I-}
{$IFNDEF fpc}
{$R-}
{$ENDIF}

interface
uses
  creg_def, dumphive_comm;

// creg-hive einlesen und ausgeben
procedure OutputCREG(const sFileName, sTarget, sPrefPath: string);

// testen, ob eine datei ein erkennbarer CREG-Hive ist
function IsCREGHive(const sFileName: string): Boolean;

implementation

{$IFNDEF fpc}
var
{$ELSE}
const
{$ENDIF}
  // erster aufruf von Walktree, wird dann zurückgesetzt
  _1ST: Boolean = True;


// RGDB-Eintrag mit angegebener ID suchen

function Find_RGDB(const Hive: THIVE_RECORD; var rc: TRGDBENTRY; const ID:
  Cardinal; const IsFirst: Boolean): Boolean;

  function Find_in_RGDB(Ofs, Max: Cardinal): Boolean;
  var
    prg: PRGDB_ENTRY;
  {$IFDEF _INCDEBUG_ }
    s: string;
  {$ENDIF}
  begin
    Find_in_RGDB := False;
    while Max >= (Ofs + sizeof(TRGDB_ENTRY)) do
    begin
      prg := @Hive.Buffer[Ofs];
{$IFDEF _INCDEBUG_ }
      if _DEBUGPR then
      begin
        writeln('== RGDB_ENTRY an 0x', HexString(Ofs, 6));
        writeln('size: ', prg^.size);
        writeln('id: 0x', HexString(prg^.id, 4));
        writeln('dummy1: %', BinString(prg^.dummy1, 1));
        writeln('len_key: ', prg^.len_key);
        writeln('num_vals: ', prg^.num_vals);
        writeln('dummy2: %', BinString(prg^.dummy2, 1));
        SetString(s, PChar(@Hive.Buffer[Ofs + sizeof(TRGDB_ENTRY)]),
          prg^.len_key);
        writeln('key_name: ', Until0(s));
      end;
{$ENDIF}
      if prg^.id = ID then
      begin
        Move(prg^, rc, sizeof(TRGDB_ENTRY));
        SetString(rc.key_name, PChar(@Hive.Buffer[Ofs + sizeof(TRGDB_ENTRY)]),
          prg^.len_key);
        rc.key_name := Until0(rc.key_name);
        if prg^.num_vals > 0
          then
          rc.data := @Hive.Buffer[Ofs + sizeof(TRGDB_ENTRY) + prg^.len_key]
        else
          rc.data := nil;
        Find_in_RGDB := True;
        Break;
      end;
      Inc(Ofs, prg^.size);
    end;
  end;

var
  i: Cardinal;
  Ofs: Cardinal;
  res: Boolean;
begin
  res := False;
  rc.key_name := '';
  if (ID = $FFFFFFFF) and IsFirst then
  begin
    rc.num_vals := 0;
    res := True;
  end
  else
  begin
    i := 0;
    Ofs := Hive.RootKey;
    repeat
      if (Ofs + sizeof(TRGDB_HEADER)) > Hive.Size
        then
        Break;
      with PRGDB_HEADER(@Hive.Buffer[Ofs])^ do
      begin
{$IFDEF _INCDEBUG_ }
        if _DEBUGPR then
        begin
          writeln('== RGDB_HEADER an 0x', HexString(Ofs, 6));
          writeln('id: 0x', HexString(id, 6));
          writeln('size: ', size);
        end;
{$ENDIF}
        if ID <> ID_RGDB then
        begin
          Break;
        end
        else
        begin
          res := Find_in_RGDB(Ofs + RGKN_FIRST, Ofs + RGKN_FIRST + size);
          if res
            then
            Break
          else
          begin
            Ofs := Ofs + size;
            Inc(i);
          end;
        end;
      end;
    until res or (i >= Hive.hrNum);
  end;
  Find_RGDB := res;
end;

// Tree-Strukturen durchlaufen

procedure WalkTree(const Hive: THive_Record; const Ofset: Cardinal; const
  KeyPath: string);
var
  rc: TRGDBENTRY;
  rv: PVALUE_ENTRY;
  i: Cardinal;
  rVK: TVKRecord;
  valname: string;
begin
  with PTREE_BLOCK(@Hive.Buffer[Ofset])^ do
  begin
{$IFDEF _INCDEBUG_ }
    if _DEBUGPR then
    begin
      writeln('== TREE_BLOCK an 0x', HexString(Ofset, 6));
      writeln('dummy1: %', BinString(dummy1, 1));
      writeln('hash: 0x', HexString(hash, 6));
      writeln('dummy2: %', BinString(dummy2, 1));
      writeln('ofsParent: 0x', HexString(ofsParent, 6));
      writeln('ofsSub: 0x', HexString(ofsSub, 6));
      writeln('ofsNext: 0x', HexString(ofsNext, 6));
      writeln('ofsParent: 0x', HexString(ofsParent, 6));
      writeln('id: 0x', HexString(id, 4));
    end;
{$ENDIF}
    if Find_RGDB(Hive, rc, id, not IsValid(ofsParent)) then
    begin
      if _1ST then
      begin
        // titel ausgeben ("regedit4" oder so)
        _1st := False;
        if _EXTENDED
          then
          writeln(fTarget, DHVER + ' (CREG)')
        else
          writeln(fTarget, 'REGEDIT4');
      end;
      // geklammerten schlüsselnamen ausgeben
      targetln(''); // zeilenumbruch schreiben
      targetln('[' + MakeKeyName(Hive.Prefix + MakeKeyName(KeyPath + '\' +
        rc.key_name)) + ']'); // schlüsselnamen schreiben
      if IsValid(rc.num_vals) then // werte ausgeben
      begin
        for i := 0 to Pred(rc.num_vals) do
        begin
          rv := Pointer(rc.Data);
          rvk.len_name := rv^.len_val;
          rvk.len_data := rv^.len_data;
          rvk.type_data := rv^.type_val;
          if IsValid(rvk.len_data)
            then
            rvk.Data := @rc.Data[sizeof(TVALUE_ENTRY) + rv^.len_val]
          else
            rvk.Data := nil;
          if not IsValid(rv^.len_val)
            then
            ValName := '@' // default-value (kein name)
          else
          begin
            SetString(ValName, PChar(@rc.Data[sizeof(TVALUE_ENTRY)]),
              rv^.len_val);
            ValName := '"' + Until0(ValName) + '"';
          end;
          OutputVKRec(rvk, ValName); // wert ausgeben
          rc.Data := @rc.Data[rv^.len_val + rv^.len_data +
            sizeof(TVALUE_ENTRY)];
        end;
      end;
      if IsValid(ofsSub) // unterschlüssel durchlaufen
        then
        WalkTree(Hive, RGKN_FIRST + ofsSub, MakeKeyName(KeyPath + '\' +
          rc.key_name));
      if IsValid(ofsNext) // nächsten schlüssel gleicher ebene durchlaufen
        then
        WalkTree(Hive, RGKN_FIRST + ofsNext, KeyPath);
    end
{$IFDEF _INCDEBUG_ }
    else
      if _DEBUGPR
        then
        writeln('### keine RGDB-Daten mit ID 0x', HexString(id, 6),
          ' gefunden');
{$ENDIF}
  end;
end;

// RGKN-block testen und auswerten

procedure Check_Block(const Hive: THive_Record);
begin
  with PRGKN_BLOCK(@Hive.Buffer[RGKN_FIRST])^ do
  begin
{$IFDEF _INCDEBUG_ }
    if _DEBUGPR then
    begin
      writeln('== RGKN_BLOCK an 0x', HexString(RGKN_FIRST, 6));
      writeln('id: 0x', HexString(id, 4));
      writeln('size: ', size);
      writeln('ofs_root: 0x', HexString(ofs_root, 6));
    end;
{$ENDIF}
    if id <> ID_RGKN
      then
      Die(['Ungültiger RGKN-Block an 0x', HexString(RGKN_FIRST, 6)])
    else
    begin
      if (not IsValid(ofs_root)) or ((ofs_root + sizeof(TTREE_BLOCK)) >
        Hive.Size)
        then
        Die(['Zeiger auf ungültigen TREE-Eintrag in RGKN-Block an 0x',
          HexString(RGKN_FIRST, 6)])
      else
        WalkTree(Hive, ofs_root + RGKN_FIRST, ''); // TREE-blöcke auswerten
    end;
  end;
end;


// CREG testen und durchlaufen

procedure Parse_CREG(var Hive: THIVE_RECORD);
begin
  if Hive.Size > sizeof(TCREG_HEADER) then
  begin
    with PCREG_HEADER(Hive.Buffer)^ do
    begin
{$IFDEF _INCDEBUG_ }
      if _DEBUGPR then
      begin
        writeln('== CREG_HEADER an 0x000000');
        writeln('id: 0x', HexString(id, 4));
        writeln('dummy1: %', BinString(dummy1, 1));
        writeln('ofs_root: 0x', HexString(ofs_root, 6));
        writeln('dummy2: %', BinString(dummy2, 1));
        writeln('entries: ', entries);
      end;
{$ENDIF}
      if ID <> ID_CREG
        then
        Die(['Scheint kein 16 Bit-CREG-Hive zu sein']);
      Hive.RootKey := ofs_root; // eintrag des ersten RGDB-blocks
      Hive.hrNum := entries; // anzahl der RGDB-blöcke
    end;
    // nun den wert an cOfs holen, und testen, ob es ein NK-Record ist
    if (Hive.RootKey + sizeof(TRGKN_BLOCK)) > Hive.size
      then
      Die(['Ungültiger Zeiger auf 1. Key-Eintrag, größer als Datei (',
        ToStr(Hive.RootKey), ' -> ', ToStr(Hive.size), ')']);
    if Hive.hrNum > 0
      then
      Check_Block(Hive) // RGKN-block auswerten
{$IFDEF _INCDEBUG_ }
    else
      if _DEBUGPR
        then
        writeln('### der hive enthält keine schlüssel (entries=0)');
{$ENDIF}
  end;
end;

// CREG-hive einlesen und ausgeben

procedure OutputCREG(const sFileName, sTarget, sPrefPath: string);
var
  pBuffer: PByteArray;
  cSize,
    cRead: Cardinal;
  f: file of Byte;
  rHive: THive_Record;
begin
  FileMode := 0; // nur lesend
//  pBuffer := nil;

  // dateigröße herausfinden
  Assign(f, sFileName);
  Reset(f);
  cSize := FileSize(f);
  Close(f);
  if ioresult <> 0
    then
    Die(['Kann "', sFileName, '" nicht öffnen.']);

  // puffer erzeugen
  GetMem(pBuffer, cSize);

  // datei in speicher einlesen
  Assign(f, sFileName);
  Reset(f);
  BlockRead(f, pBuffer^, cSize, cRead);
  Close(f);

  // überprüfen, ob die größen miteinander übereinstimmen
  if cSize <> cRead
    then
    Die(['Kann "', sFileName, '" nicht einlesen, weniger Daten als erwartet (',
      ToStr(cRead), '/', ToStr(cSize), ')']);

  // strukturdaten setzen
  rHIVE.FileName := sFileName;
  rHIVE.Prefix := sPrefPath;
  rHive.Size := cSize;
  rHive.Buffer := pBuffer;

  // zieldatei anlegen oder beenden
  filemode := 2; // rw
  Assign(fTarget, sTarget);
  Rewrite(fTarget);
  if ioresult <> 0
    then
    Die(['Kann nicht in "', sTarget, '" schreiben.']);

  // in speicher geladenen hive abarbeiten
  Parse_CREG(rHive);

  Close(fTarget);

  // speicher freigeben
  if Assigned(pBuffer)
    then
    FreeMem(pBuffer);
end;

// testen, ob eine datei ein erkennbarer CREG-Hive ist

function IsCREGHive(const sFileName: string): Boolean;
var
  f: file of Cardinal;
  fhdr: Cardinal;
begin
  // datei readonly öffnen
  FileMode := 0;
  Assign(f, sFileName);
  Reset(f);
  // erste vier byte einlesen
  Read(f, fhdr);
  Close(f);
  if ioresult <> 0 // konnte datei nicht öffnen, fehler
    then
    Die(['Kann Datei "', sFileName, '" nicht öffnen.']);
  IsCREGHive := fhdr = ID_CREG;
  FileMode := 2; // filemode auf rw zurücksetzen
end;

end.
