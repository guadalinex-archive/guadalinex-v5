unit regf_procs;

(* prozeduren zum parsen von winnt-registry-hives
   (C)2000-2004 Markus Stephany, merkes_at_mirkes.de, BSD-lizensiert

   Microsoft, Windows, Windows NT sind Markenzeichen der Microsoft
   Corporation.
   NT ist eine Marke von Northern Telecom Limited.

   - v0.01 ?.?.2000:        erste version
   - 0.01a 07. august 2000: ausgabe der aces bei nicht vorhandenen flags
                            (da war eine klammer zuviel)
   - v0.02 02. märz 2004:   ausgabe des security-descriptors im SDDL-format
                            unter delphi/w2k,xp
   - v07-31-2004, 31. juli 2004:
                            hex-daten-ausgabe in kleinschrift
                            "lh"-strukturen in nt-hives werden unterstützt
                            unicode-wertnamen werden unterstützt
                            inline-vks werden nur noch bei unbenannten daten
                             nach REG_DWORD konvertiert
                            bei REG_SZ-ausgabe als hexdaten (hex(1)) in nt-hives
                             im unicode-format

   credits an petter nordahl hagen, http://home.eunet.no/~pnordahl
   (vom quellcode zu seinem chntpw habe ich viele der definitionen
    hier adoptiert)

           und den unbekannten verfasser von winreg.txt


*)

// ansi-strings
{$H+}

// kein abbruch bei dateifehlern
{$I-}

{$DEFINE debug:=//}

interface
uses
  regf_def, dumphive_comm;

// regf-hive einlesen und ausgeben
procedure OutputREGF(const sFileName, sTarget, sPrefPath: string);

// testen, ob eine datei ein erkennbarer REGF-Hive ist
function IsREGFHive(const sFileName: string): Boolean;

implementation


// NK-record parsen (forward-deklaration)
procedure Parse_NK(const Hive: THIVE_RECORD; const Ofset, Security: DWORD; const
  KeyPath: string); forward;


// LF-record parsen

procedure Parse_LF(const Hive: THIVE_RECORD; const Ofset, Security: DWORD; const
  KeyPath: string);
var
  w: Word;
begin
{$IFDEF fpc}
  debug('Parse_LF ', Ofset, ' ', KeyPath);
{$ENDIF}
  with PLF_KEY(@Hive.Buffer[Ofset])^ do
  begin
{$IFDEF _INCDEBUG_ }
    if _DEBUGPR then
    begin
      writeln('== LF_KEY an 0x', HexString(Ofset, 6));
      writeln('id: 0x', HexString(id, 4));
      writeln('no_keys: ', no_keys);
    end;
{$ENDIF}
    if IsValid(no_keys)
      then
      for w := 0 to Pred(no_keys) do
      begin
        if IsValid(HASHES[w].ofs_nk) then
        begin
          if PWord(@Hive.Buffer[HASHES[w].ofs_nk + KEY_OFS])^ = ID_NK
            then
            Parse_NK(Hive, HASHES[w].ofs_nk + KEY_OFS, Security, KeyPath);
        end;
      end;
  end;
end;

// LH-record parsen

procedure Parse_LH(const Hive: THIVE_RECORD; const Ofset, Security: DWORD; const
  KeyPath: string);
var
  w: Word;
begin
{$IFDEF fpc}
  debug('Parse_LH ', Ofset, ' ', KeyPath);
{$ENDIF}
  with PLF_KEY(@Hive.Buffer[Ofset])^ do
  begin
{$IFDEF _INCDEBUG_ }
    if _DEBUGPR then
    begin
      writeln('== LH_KEY an 0x', HexString(Ofset, 6));
      writeln('id: 0x', HexString(id, 4));
      writeln('no_keys: ', no_keys);
    end;
{$ENDIF}
    if IsValid(no_keys)
      then
      for w := 0 to Pred(no_keys) do
      begin
        if IsValid(HASHES[w].ofs_nk) then
        begin
          if PWord(@Hive.Buffer[HASHES[w].ofs_nk + KEY_OFS])^ = ID_NK
            then
            Parse_NK(Hive, HASHES[w].ofs_nk + KEY_OFS, Security, KeyPath);
        end;
      end;
  end;
end;


// LI-record parsen

procedure Parse_LI(const Hive: THIVE_RECORD; const Ofset, Security: DWORD; const
  KeyPath: string);
var
  w: Word;
begin
{$IFDEF fpc}
  debug('Parse_LI ', Ofset, ' ', KeyPath);
{$ENDIF}
  with PLI_KEY(@Hive.Buffer[Ofset])^ do
  begin
{$IFDEF _INCDEBUG_ }
    if _DEBUGPR then
    begin
      writeln('== LI_KEY an 0x', HexString(Ofset, 6));
      writeln('id: 0x', HexString(id, 4));
      writeln('no_keys: ', no_keys);
    end;
{$ENDIF}
    if IsValid(no_keys)
      then
      for w := 0 to Pred(no_keys) do
      begin
        if IsValid(HASHES[w].ofs_nk) then
        begin
          if PWord(@Hive.Buffer[HASHES[w].ofs_nk + KEY_OFS])^ = ID_NK
            then
            Parse_NK(Hive, HASHES[w].ofs_nk + KEY_OFS, Security, KeyPath);
        end
      end
  end;
end;

// RI-record parsen

procedure Parse_RI(const Hive: THIVE_RECORD; const Ofset, Security: DWORD; const
  KeyPath: string);
var
  w: Word;
  lid: word;
begin
{$IFDEF fpc}
  debug('Parse_RI ', Ofset, ' ', KeyPath);
{$ENDIF}
  with PRI_KEY(@Hive.Buffer[Ofset])^ do
  begin
{$IFDEF _INCDEBUG_ }
    if _DEBUGPR then
    begin
      writeln('== RI_KEY an 0x', HexString(Ofset, 6));
      writeln('id: 0x', HexString(id, 4));
      writeln('no_lis: ', no_lis);
    end;
{$ENDIF}
    if IsValid(no_lis)
      then
      for w := 0 to Pred(no_lis) do
      begin
        if IsValid(HASHES[w].ofs_li) then
        begin
          lid := PWord(@Hive.Buffer[HASHES[w].ofs_li + KEY_OFS])^;
          case lid of
            ID_LI: Parse_LI(Hive, HASHES[w].ofs_li + KEY_OFS, Security,
              KeyPath);
            ID_LH: Parse_LH(Hive, HASHES[w].ofs_li + KEY_OFS, Security,
              KeyPath);
          else
            Die(['Unbekannter Strukturtyp (',
              ToStr(lid), ') an ', ToStr(HASHES[w].ofs_li + KEY_OFS)]);
              // unbekannter strukturtyp
          end;
        end
      end
  end;
end;

//länge der daten in VK-Struktur holen

function get_len_data(const Key: PVK_KEY): DWORD;
var
  gl: DWORD;
begin
  gl := Key^.len_data and MASK_DATALEN;
  if gl = 0 then
    if Key^.len_name = 0 then
      gl := sizeof(DWORD);
  get_len_data := gl;
end;

//typ der daten in VK-Struktur holen

function get_val_type(const Key: PVK_KEY): DWORD;
begin
  if (DWORD(Key^.len_data) = Cardinal(not MASK_DATALEN)) and (Key^.len_name = 0)
    then
    get_val_type := REG_DWORD
  else
    get_val_type := Key^.val_type;
end;

//position der daten in VK-Struktur holen

function get_pos_data(const Hive: THIVE_RECORD; const Key: PVK_KEY): Pointer;
begin
  if IsValid(Key^.ofs_Data)
    then
    get_pos_data := @Hive.Buffer[Key^.ofs_Data + KEY_OFS]
  else
    get_pos_data := nil;
  if ((Key^.len_data and MASK_DATALEN) = 0) and (Key^.len_name = 0)
    then
    get_pos_data := @(Key^.val_type)
  else
    if (Key^.len_data and (not MASK_DATALEN)) <> 0
      then
      get_pos_data := @(Key^.ofs_data);
end;

// VK-record parsen

procedure Parse_VK(const Hive: THIVE_RECORD; const Ofset: DWORD);
var
  rVK: TVKRecord;
  ValName: string;
begin
{$IFDEF fpc}
  debug('Parse_VK ', Ofset);
{$ENDIF}
  with PVK_KEY(@Hive.Buffer[Ofset])^ do
  begin
{$IFDEF _INCDEBUG_ }
    if _DEBUGPR then
    begin
      writeln('== VK_KEY an 0x', HexString(Ofset, 6));
      writeln('id: 0x', HexString(id, 4));
      writeln('len_name: ', len_name);
      writeln('len_data: ', len_data);
      writeln('ofs_data: 0x', HexString(ofs_data, 6));
      writeln('val_type: 0x', HexString(val_type, 6));
      writeln('flag: %', BinString(flag, 1));
      writeln('dummy1: %', BinString(dummy1, 1));
    end;
{$ENDIF}
    rvk.len_name := len_name;
    rvk.len_data := get_len_data(PVK_Key(@Hive.Buffer[Ofset]));
      //len_data and MASK_DATALEN;
    rvk.type_data := get_val_type(PVK_Key(@Hive.Buffer[Ofset]));
    ValName := '';
    rvk.Data := get_pos_data(Hive, PVK_Key(@Hive.Buffer[Ofset]));
      //@Hive.Buffer[ofs_data+KEY_OFS];

    if IsValid(len_name) and Boolean(flag and $1) then
    begin
      SetString(ValName, keyname, len_name);
      ValName := '"' + Until0(ValName) + '"';
    end
    else
      if IsValid(len_name) and (not Boolean(flag and $1)) then
      begin
        // unicode-wertname, falls bit 0 in flag nicht gesetzt
        SetString(ValName, keyname, len_name);
        ValName := Uni2Ansi(ValName);
        ValName := '"' + Until0(ValName) + '"';
      end
      else
        ValName := '@';

{$IFDEF _INCDEBUG_ }
    if _DEBUGPR then
      writeln('valname: ', ValName);
{$ENDIF}

    if rvk.Data <> nil
      then
      OutputVKRec(rVK, ValName);
  end;
end;

// VL-liste parsen

procedure Parse_VL(const Hive: THIVE_RECORD; const Ofset, Count: DWORD);
var
  iC: DWORD;
  pD: PDWORD;
begin
{$IFDEF fpc}
  debug('Parse_VL ', Ofset, ' ', Count);
{$ENDIF}
  pD := @Hive.Buffer[Ofset];
  for iC := 1 to Count do
  begin
    if IsValid(pD^) then
    begin
      if PWord(@Hive.Buffer[pD^ + KEY_OFS])^ = ID_VK
        then
        Parse_VK(Hive, pD^ + KEY_OFS)
    end;
    Inc(pD);
  end;
end;

// SECURITY_DESCRIPTOR parsen

function Parse_SD(const Hive: THIVE_RECORD; const Ofset: DWORD): string;

  // control holen
  function Get_ControlStr(control: WORD): string;
  var
    res: string;
    i: Integer;
  const
    Known: array[1..13] of TFlagStr = (
      (Flag: SE_DACL_PRESENT; Str: 'SE_DACL_PRESENT '),
      (Flag: SE_SACL_PRESENT; Str: 'SE_SACL_PRESENT '),
      (Flag: SE_SELF_RELATIVE; Str: 'SE_SELF_RELATIVE '),
      (Flag: SE_OWNER_DEFAULTED; Str: 'SE_OWNER_DEFAULTED '),
      (Flag: SE_GROUP_DEFAULTED; Str: 'SE_GROUP_DEFAULTED '),
      (Flag: SE_DACL_DEFAULTED; Str: 'SE_DACL_DEFAULTED '),
      (Flag: SE_SACL_DEFAULTED; Str: 'SE_SACL_DEFAULTED '),
      (Flag: SE_DACL_AUTO_INHERIT_REQ; Str: 'SE_DACL_AUTO_INHERIT_REQ '),
      (Flag: SE_SACL_AUTO_INHERIT_REQ; Str: 'SE_SACL_AUTO_INHERIT_REQ '),
      (Flag: SE_DACL_AUTO_INHERITED; Str: 'SE_DACL_AUTO_INHERITED '),
      (Flag: SE_SACL_AUTO_INHERITED; Str: 'SE_SACL_AUTO_INHERITED '),
      (Flag: SE_DACL_PROTECTED; Str: 'SE_DACL_PROTECTED '),
      (Flag: SE_SACL_PROTECTED; Str: 'SE_SACL_PROTECTED ')
      );
  begin
    res := '';
    for i := 1 to 13 do
    begin
      if (control and Known[i].Flag) <> 0 then
      begin
        res := res + Known[i].Str;
        control := control and (not Known[i].Flag);
      end;
    end;
    if control <> 0
      then
      res := res + '0x' + HexString(control, 4) + ' ';
    Get_ControlStr := res;
  end;

{$IFNDEF fpc}
  // unter win32 account suchen
  function Lookup_SID(const SID: Pointer; var res: string): Boolean;
  var
    dom, usr: string;
    sdom, susr: DWORD;
    pe: DWORD;
  begin
    if Assigned(LookupAccountSid) then
    begin
      SetLength(dom, 1024);
      SetLength(usr, 1024);
      sdom := 1023;
      susr := 1023;
      Result := LookupAccountSid(nil, SID, @usr[1], susr, @dom[1], sdom, pe);
      if Result then
        res := MakeKeyName(Until0(dom) + '\' + Until0(usr));
    end
    else
      Result := False;
  end;
{$ENDIF}

  // sid in string-form bringen
  function Get_SIDStr(const SID: PSID): string;
  var
    res: string;
    ia: Cardinal;
  {$IFDEF WIN32}
    sr: string;
  {$ENDIF}
  begin
    res := '"Unknown"';
    with SID^ do
    begin
{$IFDEF _INCDEBUG_ }
      if _DEBUGPR then
      begin
        writeln('== SID an 0x', HexString(LongInt(Pointer(SID)) -
          LongInt(Pointer(Hive.Buffer)), 6));
        writeln('sid_revision: ', sid_revision);
        writeln('num_auths: ', num_auths);
        write('id_auth: ');
        for ia := 0 to 5
          do
          write(HexString(id_auth[ia], 2), ' ');
        writeln;
        write('sub_auth: ');
        for ia := 0 to Pred(num_auths)
          do
          write(HexString(sub_auth[ia], 6), ' ');
        writeln;
      end;
{$ENDIF}
      if sid_revision = SECURITY_REVISION then
      begin
        res := '';
        ia := id_auth[5] + (id_auth[4] shl 8) + (id_auth[3] shl 16) + (id_auth[2]
          shl 24);
        Res := 'S-' + ToStr(sid_revision) + '-' + ToStr(ia);
        if num_auths > 0
          then
          for ia := 0 to num_auths - 1
            do
            Res := Res + '-' + ToStr(sub_auth[ia]);
      end;
    end;
{$IFDEF WIN32}
    if _SID2NAME then
    begin
      if Lookup_SID(SID, sr)
        then
        res := res + ' (' + sr + ')'
      else
        res := res + ' (???)';
    end;
{$ENDIF}
    Get_SIDStr := res;
  end;

  // stringbeschreibung für ACE holen
  function Get_ACEStr(const ACE: PACE; var Ofset: DWORD): string;
  var
    res: string;
    i, j: Integer;
    flg: DWORD;
  const
    KnownTypes: array[1..4] of TFlagStr = (
      (Flag: ACCESS_ALLOWED_ACE_TYPE; Str: 'ACCESS_ALLOWED_ACE_TYPE:'),
      (Flag: ACCESS_DENIED_ACE_TYPE; Str: 'ACCESS_DENIED_ACE_TYPE:'),
      (Flag: SYSTEM_AUDIT_ACE_TYPE; Str: 'SYSTEM_AUDIT_ACE_TYPE:'),
      (Flag: SYSTEM_ALARM_ACE_TYPE; Str: 'SYSTEM_ALARM_ACE_TYPE:')
      );
    KnownFlags: array[1..7] of TFlagStr = (
      (Flag: OBJECT_INHERIT_ACE; Str: 'OBJECT_INHERIT_ACE '),
      (Flag: CONTAINER_INHERIT_ACE; Str: 'CONTAINER_INHERIT_ACE '),
      (Flag: NO_PROPAGATE_INHERIT_ACE; Str: 'NO_PROPAGATE_INHERIT_ACE '),
      (Flag: INHERIT_ONLY_ACE; Str: 'INHERIT_ONLY_ACE '),
      (Flag: INHERITED_ACE; Str: 'INHERITED_ACE '),
      (Flag: SUCCESSFUL_ACCESS_ACE_FLAG; Str: 'SUCCESSFUL_ACCESS_ACE_FLAG '),
      (Flag: FAILED_ACCESS_ACE_FLAG; Str: 'FAILED_ACCESS_ACE_FLAG ')
      );
    KnownRights: array[1..17] of TFlagStr = (
      (Flag: _DELETE; Str: 'DELETE '),
      (Flag: READ_CONTROL; Str: 'READ_CONTROL '),
      (Flag: WRITE_DAC; Str: 'WRITE_DAC '),
      (Flag: WRITE_OWNER; Str: 'WRITE_OWNER '),
      (Flag: SYNCHRONIZE; Str: 'SYNCHRONIZE '),
      (Flag: ACCESS_SYSTEM_SECURITY; Str: 'ACCESS_SYSTEM_SECURITY '),
      (Flag: MAXIMUM_ALLOWED; Str: 'MAXIMUM_ALLOWED '),
      (Flag: GENERIC_ALL; Str: 'GENERIC_ALL '),
      (Flag: GENERIC_EXECUTE; Str: 'GENERIC_EXECUTE '),
      (Flag: GENERIC_WRITE; Str: 'GENERIC_WRITE '),
      (Flag: GENERIC_READ; Str: 'GENERIC_READ '),
      (Flag: KEY_QUERY_VALUE; Str: 'KEY_QUERY_VALUE '),
      (Flag: KEY_SET_VALUE; Str: 'KEY_SET_VALUE '),
      (Flag: KEY_CREATE_SUB_KEY; Str: 'KEY_CREATE_SUB_KEY '),
      (Flag: KEY_ENUMERATE_SUB_KEYS; Str: 'KEY_ENUMERATE_SUB_KEYS '),
      (Flag: KEY_NOTIFY; Str: 'KEY_NOTIFY '),
      (Flag: KEY_CREATE_LINK; Str: 'KEY_CREATE_LINK ')
      );
  begin
    res := '(';
    with ACE^ do
    begin
{$IFDEF _INCDEBUG_ }
      if _DEBUGPR then
      begin
        writeln('== ACE an 0x', HexString(LongInt(Pointer(ACE)) -
          LongInt(Pointer(Hive.Buffer)), 6));
        writeln('ace_type: 0x', HexString(ace_type, 2));
        writeln('ace_flags: %', BinString(ace_flags, 1));
        writeln('ace_size: ', ace_size);
        writeln('mask: %', BinString(mask, 1));
      end;
{$ENDIF}
      // typ holen
      j := -1;
      for i := 1 to 4
        do
        if ace_type = KnownTypes[i].Flag then
        begin
          j := i;
          Break;
        end;
      if j = -1
        then
        res := res + '0x' + HexString(ace_type, 1) + ' '
      else
        res := res + KnownTypes[j].Str + ' ';
      // flags holen
      flg := ace_flags;
      for i := 1 to 7 do
      begin
        if (flg and KnownFlags[i].Flag) <> 0 then
        begin
          res := res + KnownFlags[i].Str;
          flg := flg and (not KnownFlags[i].Flag);
        end;
      end;
      if flg <> 0
        then
        res := res + '0x' + HexString(flg, 6) + ' ';
      res := Copy(res, 1, Pred(Length(res))) + ') (';
      // mask holen
      flg := mask;
      for i := 1 to 17 do
      begin
        if (flg and KnownRights[i].Flag) <> 0 then
        begin
          res := res + KnownRights[i].Str;
          flg := flg and (not KnownRights[i].Flag);
        end;
      end;
      if flg <> 0
        then
        res := res + '0x' + HexString(flg, 6) + ' ';
      res := Copy(res, 1, Pred(Length(res))) + ') (';
      // sid holen
      res := res + Get_SIDStr(@sid_start) + ')';
      Inc(Ofset, ace_size);
    end;
    Get_ACEStr := res + ')';
  end;

  // stringbeschreibung für ACL holen
  function Get_ACLStr(const ACL: PACL; const Ofset: DWORD): string;
  var
    res: string;
    dw, dwo: Cardinal;
  begin
    res := '';
    // NULL-ACL abfangen
    if IsValid(Ofset)
      then
      with ACL^ do
      begin
{$IFDEF _INCDEBUG_ }
        if _DEBUGPR then
        begin
          writeln('== ACL an 0x', HexString(LongInt(Pointer(ACL)) -
            LongInt(Pointer(Hive.Buffer)), 6));
          writeln('acl_revision: ', acl_revision);
          writeln('dummy1: %', BinString(dummy1, 1));
          writeln('size: ', size);
          writeln('ace_count: ', ace_count);
          writeln('dummy2: %', BinString(dummy2, 1));
        end;
{$ENDIF}
        if acl_revision = _ACL_REVISION then
        begin
          res := '(' + HexString(acl_revision, 2) + ' ' + HexString(ace_count, 4)
            + ')';
          if IsValid(ace_count) then
          begin
            dwo := (LongInt(Pointer(ACL)) - LongInt(Pointer(Hive.Buffer))) +
              sizeof(TACL);
            for dw := 0 to Pred(ace_count)
              do
              res := res + LINEFEED + '@Ace=' + Get_ACEStr(@Hive.Buffer[dwo],
                dwo);
          end
        end
        else
          res := '"Unknown ACL Revision 0x' + HexString(acl_revision, 2) + '"';
      end;
    Get_ACLStr := res;
  end;

var
  res: string;
begin
  res := '';
  with PSECURITY_DESCRIPTOR(@Hive.Buffer[Ofset])^ do
  begin
{$IFDEF _INCDEBUG_ }
    if _DEBUGPR then
    begin
      writeln('== SECURITY_DESCRIPTOR an 0x', HexString(Ofset, 6));
      writeln('sd_revision: ', sd_revision);
      writeln('dummy1: %', BinString(dummy1, 1));
      writeln('control: 0x', HexString(control, 4));
      writeln('ofs_owner: 0x', HexString(ofs_owner, 6));
      writeln('ofs_group: 0x', HexString(ofs_group, 6));
      writeln('ofs_sacl: 0x', HexString(ofs_sacl, 6));
      writeln('ofs_dacl: 0x', HexString(ofs_dacl, 6));
    end;
{$ENDIF}
{$IFNDEF fpc}
    if _SDDL and SD2String(PSECURITY_DESCRIPTOR(@Hive.Buffer[Ofset]), Result)
      then
    begin
      Result := '@SD=' + Result;
      Exit;
    end;
{$ENDIF}
    res := '@Security=(' + Get_ControlStr(control) + '(';
    if IsValid(ofs_owner)
      then
      res := res + '@Owner ';
    if IsValid(ofs_group)
      then
      res := res + '@Group ';
    if (control and SE_DACL_PRESENT) <> 0
      then
      res := res + '@DACL ';
    if (control and SE_SACL_PRESENT) <> 0
      then
      res := res + '@SACL ';
    res := Copy(res, 1, Pred(Length(res))) + '))';
    if IsValid(ofs_owner)
      then
      res := res + LINEFEED + '@Owner=' + Get_SIDStr(@Hive.Buffer[Ofset +
        ofs_owner]);
    if IsValid(ofs_group)
      then
      res := res + LINEFEED + '@Group=' + Get_SIDStr(@Hive.Buffer[Ofset +
        ofs_group]);
    if (control and SE_DACL_PRESENT) <> 0
      then
      res := res + LINEFEED + '@DACL=' + Get_ACLStr(@Hive.Buffer[Ofset +
        ofs_dacl], ofs_dacl);
    if (control and SE_SACL_PRESENT) <> 0
      then
      res := res + LINEFEED + '@SACL=' + Get_ACLStr(@Hive.Buffer[Ofset +
        ofs_sacl], ofs_sacl);
  end;
  Parse_SD := res;
end;

// Security holen

function Get_SK(const Hive: THIVE_RECORD; const Ofset: DWORD): string;
var
  res: string;
  idbg: Cardinal;
begin
  res := '@Security="Invalid"';
{$IFDEF fpc}
  debug('Get_SK ', Ofset);
{$ENDIF}
  with PSK_KEY(@Hive.Buffer[Ofset])^ do
  begin
{$IFDEF _INCDEBUG_ }
    if _DEBUGPR then
    begin
      writeln('== SK_KEY an 0x', HexString(Ofset, 6));
      writeln('id: 0x', HexString(id, 4));
      writeln('dummy1: %', BinString(dummy1, 1));
      writeln('ofs_prevsk: 0x', HexString(ofs_prevsk, 6));
      writeln('ofs_nextsk: 0x', HexString(ofs_nextsk, 6));
      writeln('no_usage: ', no_usage);
      writeln('len_sk: ', len_sk);
      write('data: ');
      for idbg := 0 to Pred(len_sk)
        do
        write(HexString(Ord(data[idbg]), 2), ' ');
      writeln;
    end;
{$ENDIF}
    if (id = ID_SK) and IsValid(len_sk) then
    begin
      res := '';
      with PSECURITY_DESCRIPTOR(@data)^ do
      begin
        if (sd_revision = SECURITY_REVISION) and ((control and SE_SELF_RELATIVE)
          <> 0) and (dummy1 = 0)
          then
          res := Parse_SD(Hive, Ofset + sizeof(TSK_KEY) - sizeof(TFOURChars))
        else
        begin
          res := '@Security=';
          for idbg := 0 to Pred(len_sk)
            do
            res := res + HexString(Ord(data[idbg]), 2) + ' ';
        end;
      end;
    end;
  end;
  Get_SK := res;
end;


// hive-namen ohne pfad und erweiterung holen

function GetHiveName(s: string): string;
var
  i: Integer;
begin
  if s <> '' then
  begin
    for i := Length(s) downto 1 do
    begin
      if s[i] = {$IFDEF LINUX} '/'{$ELSE} '\'{$ENDIF} then
      begin
        s := Copy(s, i + 1, MaxInt);
        Break;
      end;
    end;
  end;
  if s <> '' then
  begin
    for i := Length(s) downto 1 do
    begin
      if s[i] = '.' then
      begin
        s := Copy(s, 1, i - 1);
        Break;
      end;
    end;
  end;
  GetHiveName := s;
end;

// NK-record parsen

procedure Parse_NK(const Hive: THIVE_RECORD; const Ofset, Security: DWORD; const
  KeyPath: string);
var
  sKN, sCN: string;
  iOfs1{$IFDEF _INCDEBUG_ }, idbg{$ENDIF}: DWORD;
begin
{$IFDEF fpc}
  debug('Parse_NK ', Ofset, ' ', KeyPath);
{$ENDIF}
  with PNK_KEY(@Hive.Buffer[Ofset])^ do
  begin
{$IFDEF _INCDEBUG_ }
    if _DEBUGPR then
    begin
      writeln('== NK_KEY an 0x', HexString(Ofset, 6));
      writeln('id: 0x', HexString(id, 4));
      writeln('key_type: 0x', HexString(key_type, 4));
      write('timestamp/other: ');
      for idbg := 0 to 11
        do
        write(HexString(Ord(timestamp[idbg]), 2), ' ');
      writeln;
      writeln('ofs_parent: 0x', HexString(ofs_parent, 6));
      writeln('no_subkeys: ', no_subkeys);
      writeln('dummy1: %', BinString(dummy1, 1));
      writeln('ofs_lf: 0x', HexString(ofs_lf, 6));
      writeln('dummy2: %', BinString(dummy2, 1));
      writeln('no_values: ', no_values);
      writeln('ofs_vallist: 0x', HexString(ofs_vallist, 6));
      writeln('ofs_sk: 0x', HexString(ofs_sk, 6));
      writeln('ofs_classnam: 0x', HexString(ofs_classnam, 6));
      write('dummy3: ');
      for idbg := 0 to 19
        do
        write(HexString(Ord(dummy3[idbg]), 2), ' ');
      writeln;
      writeln('len_name: ', len_name);
      writeln('len_classnam: ', len_classnam);
      SetString(sKN, keyname, len_name);
      writeln('keyname: ', sKN);
    end;
{$ENDIF}
    if IsValid(len_name) then
    begin
      if KeyPath = '' then
      begin
        if _EXTENDED
          then
          TargetLN(DHVER + ' (REGF)')
        else
          TargetLN('REGEDIT4');
      end;
      SetString(sKN, keyname, len_name);
      if KeyPath = ''
        then
        if sKN = '$$$PROTO.HIV'
          then
          sKN := GetHiveName(Hive.FileName);
      // schlüsselnamen ausgeben
      skn := MakeKeyName(KeyPath + '\' + skn);
{$IFDEF fpc}
      debug(1);
{$ENDIF}
      targetln(''); // zeilenumbruch schreiben
      targetln('[' + MakeKeyName(Hive.Prefix + MakeKeyName(skn)) + ']');
        // schlüsselnamen schreiben
{$IFDEF fpc}
      debug(2);
{$ENDIF}
      // eventuell KlassenNamen ausgeben
      if _EXTENDED and IsValid(ofs_classnam) and IsValid(len_classnam) then
      begin
        SetString(sCN, PChar(@Hive.Buffer[ofs_classnam + KEY_OFS]),
          len_classnam);
        targetLN('@Class="' + Until0(Uni2Ansi(sCN)) + '"');
      end;
      // eventuell security ausgeben
      if _EXTENDED and IsValid(ofs_sk) then
      begin
        if ofs_sk = Security
          then
          targetLN('@Security="Inherited"')
        else
          targetln(Get_SK(Hive, ofs_sk + KEY_OFS));
      end;
{$IFDEF fpc}
      debug(3);
{$ENDIF}
      // werte durchpusten
      if IsValid(no_values) and IsValid(ofs_vallist) then
      begin
{$IFDEF fpc}
        debug(4);
{$ENDIF}
        Parse_VL(Hive, ofs_vallist + KEY_OFS, no_values);
{$IFDEF fpc}
        debug(5);
{$ENDIF}
      end;
      if IsValid(no_subkeys) and IsValid(ofs_lf) then
      begin
{$IFDEF fpc}
        debug(6);
{$ENDIF}
        iOfs1 := ofs_lf + KEY_OFS;
{$IFDEF fpc}
        debug(ofs_lf, ' ', KEY_OFS, ' ', iOfs1);
{$ENDIF}
        case PWord(@Hive.Buffer[iOfs1])^ of
          ID_LF: Parse_LF(Hive, iOfs1, ofs_sk, skn);
          ID_LH: Parse_LH(Hive, iOfs1, ofs_sk, skn);
          ID_LI: Parse_LI(Hive, iOfs1, ofs_sk, skn);
          ID_RI: Parse_RI(Hive, iOfs1, ofs_sk, skn);
        else
          Die(['Unbekannter Strukturtyp (', ToStr(PWord(@Hive.Buffer[iOfs1])^),
            ') an ', ToStr(iOfs1)]); // unbekannter strukturtyp
        end;
{$IFDEF fpc}
        debug(7);
{$ENDIF}
      end
    end;
  end;
end;

// Block des Root-Keys checken

procedure Check_Block(const Hive: THIVE_RECORD);
var
  pi1: PDWORD;
  pw1: PWord;
  iOfs: DWORD;
begin
{$IFDEF fpc}
  debug('Check_Block');
{$ENDIF}
  iOfs := Hive.RootKey;
  pi1 := PDWORD(@Hive.Buffer[Hive.RootKey]);
  if LongInt(pi1^) < 0 then
  begin
    pi1^ := -LongInt(pi1^);
  end;
  // um 4 inkrementieren
  iOfs := iOfs + 4;
  // ID holen
  pw1 := PWord(@Hive.Buffer[iOfs]);
  // auf art testen
  case pw1^ of
    ID_SK: ; //sollte nicht vorkommen
    ID_LF: Parse_LF(Hive, iOfs, $FFFFFFFF, '');
    ID_LH: Parse_LH(Hive, iOfs, $FFFFFFFF, '');
    ID_LI: Parse_LI(Hive, iOfs, $FFFFFFFF, '');
    ID_RI: Parse_RI(Hive, iOfs, $FFFFFFFF, '');
    ID_VK: Parse_VK(Hive, iOfs);
    ID_NK:
      begin
        Inc(pw1);
        if pw1^ = KEY_ROOT
          then
          Parse_NK(Hive, iOfs, $FFFFFFFF, '')
      end;
  else
    Die(['Unbekannter Strukturtyp (', ToStr(pw1^), ') an ', ToStr(iOfs)]);
      // unbekannter strukturtyp
  end;
end;

// REGF testen und durchlaufen

procedure Parse_REGF(var Hive: THIVE_RECORD);
begin
{$IFDEF fpc}
  debug('Parse_REGF');
{$ENDIF}
  if Hive.Size > sizeof(TREGF_HEADER) then
  begin
    with PREGF_HEADER(Hive.Buffer)^ do
    begin
      if ID <> ID_REGF
        then
        Die(['Scheint kein NT-REGF-Hive zu sein']);
      Hive.RootKey := ofs_rootKey + HBIN_FIRST;
    end;
    // nun den wert an cOfs holen, und testen, ob es ein NK-Record ist
    if (Hive.RootKey + sizeof(TNK_KEY)) > Hive.size
      then
      Die(['Ungültiger Zeiger auf ROOT-Key, größer als Datei (',
        ToStr(Hive.RootKey), '/', ToStr(Hive.size), ')']);
    Check_Block(Hive);
  end;
end;

// regf-hive einlesen und ausgeben

procedure OutputREGF(const sFileName, sTarget, sPrefPath: string);
var
  pBuffer: PByteArray;
  cSize,
    cRead: Cardinal;
  f: file of Byte;
  rHive: THive_Record;
begin
  FileMode := 0; // nur lesend
//  pBuffer := nil;
//  SKBuffer := nil;

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
  _ISREGF := True;
  Parse_REGF(rHive);

  // speicher freigeben
  if Assigned(pBuffer)
    then
    FreeMem(pBuffer);

  Close(fTarget);

end;

// testen, ob eine datei ein erkennbarer REGF-Hive ist

function IsREGFHive(const sFileName: string): Boolean;
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
  IsREGFHive := fhdr = ID_REGF;
  FileMode := 2; // filemode auf rw zurücksetzen
end;

end.

