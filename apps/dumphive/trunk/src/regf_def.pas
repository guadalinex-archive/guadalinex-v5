unit regf_def;

(* definitionen fuer's parsen der windows-nt registry-hives
   (C)2000-2004 Markus Stephany, merkes_at_mirkes.de, BSD-lizensiert

   Microsoft, Windows, Windows NT sind Markenzeichen der Microsoft
   Corporation.
   NT ist eine Marke von Northern Telecom Limited.

   - v0.01 06. august 2000: erste version
   - v0.02 02. märz 2004:   definitionen für windows api unter delphi
                            (ConvertSecurityDescriptorToStringSecurityDescriptor
                            funtion) und SD2String-routine
   - v07-31-2004, 31. juli 2004:
                            "lh"-strukturen in nt-hives werden unterstützt
                            unicode-wertnamen werden unterstützt


   credits an petter nordahl hagen, http://home.eunet.no/~pnordahl
   (vom quellcode zu seinem chntpw habe ich viele der definitionen
    hier adoptiert)

           und den unbekannten verfasser von winreg.txt
*)


// ansi-strings verwenden
{$H+}

interface

uses
  dumphive_comm;



const

  KEY_ROOT = $2C; // typ des root-schluessels,
                           // normale schluessel haben $20

  MASK_DATALEN = $7FFFFFFF; // datenlaenge ist geORt mit diesem wert,
                                // wird also NOT geANDet

  HBIN_FIRST = 4096; // erster hbin-block im registry-hive
  KEY_OFS = HBIN_FIRST + 4; // erster nk-eintrag im reg-hive
  MIN_REGF_SIZE = 2 * HBIN_FIRST; // so gross muss der hive mindestens sein,
                                 // um daten enthalten zu koennen


  ID_REGF = $66676572; // id "regf", kennzeichnet den hive an pos 0
  ID_HBIN = $6E696268; // id "hbin", kennzeichnet eine page im hive

  ID_SK = $6B73; // id des SK-eintrages (security-daten), "sk"
  ID_LF = $666C; // id des LF-eintrages (liste mit hashes und zeigern
                      // auf NK-eintraege), "lf"
  ID_LH = $686C; // id des LH-eintrages (liste mit hashes und zeigern
                      // auf NK-einträge, wie lf, andere hash-bildung), "lh"
  ID_LI = $696C; // id des LI-eintrages (liste mit zeigern auf
                      // NK-eintraege, ohne hashes), "li"
  ID_RI = $6972; // id des RI-eintrages (liste mit zeigern auf
                      // LI-eintraege), "ri"
  ID_VK = $6B76; // id des VK-eintrages (enthaelt die werte), "vk"
  ID_NK = $6B6E; // id des NK-eintrages (enthaelt die key-daten), "nk"


type

(* die erste seite des hives enthaelt den REGF-Eintrag, der viele
   unbekannte daten enthaelt, aber auch eine art pruefsumme,
   zeitstempel, namen, ofset auf den ersten NK-eintrag
   (-$1004, wie alle ofsets) und groesse des hives
*)

  PREGF_HEADER = ^TREGF_HEADER;
  TREGF_HEADER = packed record
    id, // id "regf"=ID_REGF
      dummy1,
      dummy2: DWORD;
    timestamp:
    array[0..7] of Byte; // zeitstempel im windowsnt-datum/zeitformat
    dummy3,
      dummy4,
      dummy5,
      dummy6,
      ofs_rootkey, // ofset auf den 1. NK-eintrag
      filesize, // groesse der daten-pages (dateigroesse -$1000)
      dummy7: DWORD;
    name:
    array[0..($1FC - $2C) - 1]
      of Char; // hier steht unter anderem der hive-name drin (???)
    checksum: DWORD; // summe aller DWORDs von $0000 bis $01FB
  end;


  (* danach kommt erstmal nix, ab $1000 kommt dann der 1. daten-block,
     (hbin-page), der auch einen zeiger auf den root-key enthaelt
  *)

(* page-header, der immer an pos (n * $1000) steht
*)

  PHBIN_PAGE = ^THBIN_PAGE;
  THBIN_PAGE = packed record
    id, // id "hbin"=ID_HBIN
      ofs_from1, // ofset zum vorherigen HBIN-block,
                              // relativ zum jetzigen (also wenn hier
         // $1000 steht, dann liegt der vorhergehende
         // block eine page (=$1000 byte) vornedran
    ofs_next: DWORD; // ofset zum naechsten HBIN-block
    dummy1:
    array[0..13] of Byte;
    len_page: DWORD; // (???) wohl doch nicht block-groesse
    data: Byte; // hier beginnen die datenbloecke,
                              // in der 2. page ($4020) der root-NK-eintrag
  end;


(* security-bloecke enthalten die sicherheits-festlegungen fuer einen
   schluessel, viele NK-eintraege koennen auf den gleichen SK-block zeigen,
   so scheint auch die vererbung der sicherheit zu funktionieren
*)

  PFourChars = ^TFourChars; // dummy-struktur
  TFourChars = array[0..3] of Char;

  PSK_KEY = ^TSK_KEY;
  TSK_KEY = packed record
    id, // id "sk"=ID_SK
      dummy1: Word;
    ofs_prevsk, // ofset zum vorhergehenden SK-eintrag
      ofs_nextsk, // ofset zum naechsten SK-eintrag
      no_usage, // referenzzaehler, gibt an, wieviele
                              // schluessel diesen SK-eintrag verwenden
    len_sk: DWORD; // groesse des eintrages
    data: TFourChars; // hier beginnt der SECURITY_DESCRIPTOR (s.u.)
  end;


(* liste mit zeigern auf NK-eintraege, enthaelt ausserdem zu jedem
   eintrag die ersten 4 buchstaben des korrespondierenden schluesselnamens,
   um direkte zugriffe auf schluesselnamen zu beschleunigen, NT 4+
*)

  PLF_HASH = ^TLF_HASH; // ein hash-eintrag
  TLF_HASH = packed record
    ofs_nk: DWORD; // ofset zum korrespondierenden NK-eintrag
    name: TFourChars; // die ersten 4 buchstaben des schluesselnamens
  end;

  PLF_HASHES = ^TLF_HASHES;
  TLF_HASHES = array[0..0] of TLF_HASH;

  PLF_KEY = ^TLF_KEY;
  TLF_KEY = packed record
    id, // id "lf"=ID_LF/"lh"=ID_LH
      no_keys: Word; // anzahl der hash-eintraege
    Hashes: TLF_HASHES; // hier kommen die hash-eintraege
  end;


(* liste mit zeigern auf NK-eintraege, enthaelt _keine_ namen, NT3.x
*)

  PLI_HASH = ^TLI_HASH; // ein solcher eintrag
  TLI_HASH = packed record
    ofs_nk: DWORD; // ofset zum korrespondierenden NK-eintrag
  end;

  PLI_HASHES = ^TLI_HASHES;
  TLI_HASHES = array[0..0] of TLI_HASH;

  PLI_KEY = ^TLI_KEY;
  TLI_KEY = packed record
    id, // id "li"=ID_LI
      no_keys: Word; // anzahl der eintraege
    Hashes: TLI_HASHES; // hier kommen die eintraege
  end;


(* liste mit zeigern auf LI-strukturen, kann anstelle von direkten
   LI-eintraegen vorkommen, wenn ein schluessel sehr viele unterschluessel
   enthaelt
*)

  PRI_HASH = ^TRI_HASH; // ein eintrag
  TRI_HASH = packed record
    ofs_li: DWORD; // zeigt auf den korrespondierenden LI-eintrag
  end;

  PRI_HASHES = ^TRI_HASHES;
  TRI_HASHES = array[0..0] of TRI_HASH;

  PRI_KEY = ^TRI_KEY;
  TRI_KEY = packed record
    id, // id "ri"=ID_RI
      no_lis: Word; // anzahl der zeiger auf LI-eintrag
    Hashes: TRI_HASHES; // hier stehen diese hash-eintraege
  end;


(* die folgende struktur beschreibt einen wert-eintrag in einem schluessel,
   die ganze struktur ist noch sehr mysterioes:
   manchmal stehen die daten im ofs_data oder val_type feld,
   das kommt darauf an, ob bit 31 im len_data feld gesetzt, oder ob
   len_data = 0 ist. nach meinen bisherigen erfahrungen sind die ausfuehrungen
   in winreg.txt als auch von pnh nicht ganz korrekt, meine vermutung
   kann man in regf_procs.pas in den get_data*-prozeduren nachlesen
   daten der werte vom typ REG_SZ,REG_EXPAND_SZ und REG_MULTI_SZ
   werden in UNICODE gespeichert!
*)

  PVK_KEY = ^TVK_KEY;
  TVK_KEY = packed record
    id, // id "vk"=ID_VK
      len_name: Word; // laenge des wert-namens (0 beim default-wert)
    len_data, // groesse der daten (siehe oben)
      ofs_data, // zeiger auf daten (s.o.)
      val_type: DWORD; // typ des wertes (s.o.), REG_SZ,...
    flag, // flag (???, wenn bit 0 nicht gesetzt, dann wertname in unicode)
      dummy1: WORD;
    Keyname: TCharArray; // hier faengt der wertname an, falls vorhanden
  end;

(* diese struktur beschreibt einen registry-schluessel
*)

  PNK_KEY = ^TNK_KEY;
  TNK_KEY = packed record
    id, // id "nk"=ID_NK
      key_type: Word; // typ des schluessels ($2c fuer root, sonst $20)
    timestamp:
    array[0..11] of Char; // zeitstempel und anderes (???)
    ofs_parent, // zeigt auf den NK-eintrag des elternschluessels
      no_subkeys: DWORD; // anzahl der unterschluessel (hier gibt es eine
                            // redundanz, da die LI/LF-eintraege einen
                         // eigenen element-zaehler enthalten
    dummy1: DWORD;
    ofs_lf: DWORD; // zeigt auf den lf/li/ri-eintrag mit subkeys
    dummy2: DWORD;
    no_values, // anzahl der werte im schluessel, dieser wert hier
                            // wird benoetigt, da es in der wert-liste keine
       // zaehler gibt!
    ofs_vallist, // ofset auf die liste mit den werteintraegen
      ofs_sk, // ofset auf den SK-eintrag
      ofs_classnam: DWORD; // zeigt auf den klassennamen des schluessels
    dummy3:
    array[0..19] of Char;
    len_name, // laenge des schluessel-namens (darf nicht 0 sein)
      len_classnam: WORD; // laenge des klassennamens (kann 0 sein)
    keyname: TCharArray; // hier faengt der schluesselname an
  end;


(* hier kommen die von mir verwendeten konstanten und typen zum parsen
   der security-informationen in SK-eintraegen
*)



const

  (* die folgenden konstanten und strukturen sind (teilweise leicht
     abgewandelt) uebernommen aus winnt.h, dieses header-file enthaelt
     folgenden copyright-hinweis:

       Copyright 1990-1998 Microsoft Corporation

  *)

  // verwendete Flags im Control-Feld des SD
  SE_DACL_PRESENT = $0004; // gibt es eine DACL?
  SE_SACL_PRESENT = $0010; // gibt es eine SACL?
  SE_SELF_RELATIVE = $8000;
    // stehen die daten direkt am Anschluss des SD-Records? (nur solche können gelesen werden)

  // die folgenden sind dumphive bekannt, deren bedeutung mir aber nicht...
  SE_OWNER_DEFAULTED = $0001;
  SE_GROUP_DEFAULTED = $0002;
  SE_DACL_DEFAULTED = $0008;
  SE_SACL_DEFAULTED = $0020;
  SE_DACL_AUTO_INHERIT_REQ = $0100;
  SE_SACL_AUTO_INHERIT_REQ = $0200;
  SE_DACL_AUTO_INHERITED = $0400;
  SE_SACL_AUTO_INHERITED = $0800;
  SE_DACL_PROTECTED = $1000;
  SE_SACL_PROTECTED = $2000;

  SECURITY_REVISION = 1; // nur elemente mit dieser rev-id können gelesen werden

  MAX_SUB_AUTHS = 15; // maximale anzahl von sub-authoritäten in einer SID

  _ACL_REVISION = 2; // nur diese ACL's können gelesen werden

  // ACE-Typen
  ACCESS_ALLOWED_ACE_TYPE = 0; // zugriff gewähren
  ACCESS_DENIED_ACE_TYPE = 1; // zugriff verweigern
  SYSTEM_AUDIT_ACE_TYPE = 2; // überwachungs-eintrag
  SYSTEM_ALARM_ACE_TYPE = 3; // (reserviert)

  // ACE-Flags
  // - vererbung
  OBJECT_INHERIT_ACE = $01;
    // (nicht verwendet, dateien erben diese berechtigung bei verzeichnis)
  CONTAINER_INHERIT_ACE = $02; // unterschlüssel erben die berechtigung
  NO_PROPAGATE_INHERIT_ACE = $04;
    // wird nur an eigene subkeys, nicht aber an deren unterschlüssel vererbt
  INHERIT_ONLY_ACE = $08;
    // wird nur vererbt, wirkt sich nicht auf den schlüssel selbst aus
  INHERITED_ACE = $10; // wird bei vererbung im "erben" gesetzt

  // - audit
  SUCCESSFUL_ACCESS_ACE_FLAG = $40; // überwachung bei erfolgreicher ausführung
  FAILED_ACCESS_ACE_FLAG = $80; // überwachung bei fehlgeschlagener überwachung


  // ACE-Berechtigungen
  _DELETE = $00010000; // darf löschen
  READ_CONTROL = $00020000; // darf berechtigungen, besitzer,gruppe lesen
  WRITE_DAC = $00040000; // darf berechtigungen schreiben
  WRITE_OWNER = $00080000; // darf besitzer, gruppe schreiben
  SYNCHRONIZE = $00100000; // thread-synchronisierung erlauben
  ACCESS_SYSTEM_SECURITY = $01000000; // darf auf SACL zugreifen
  MAXIMUM_ALLOWED = $02000000;
    // Maximaler zugriff (alle möglichen bits als gesetzt annehmen???)
  GENERIC_ALL = $10000000; // voll-zugriff
  GENERIC_EXECUTE = $20000000; // ausführen erlaubt
  GENERIC_WRITE = $40000000; // schreiben erlaubt
  GENERIC_READ = $80000000; // lesen erlaubt
  KEY_QUERY_VALUE = $00000001; // wert abfragen
  KEY_SET_VALUE = $00000002; // wert setzen
  KEY_CREATE_SUB_KEY = $00000004; // unterschlüssel erzeugen
  KEY_ENUMERATE_SUB_KEYS = $00000008;
    // REGEnumKey erlaubt (Unterschlüssel auflisten)
  KEY_NOTIFY = $00000010; // Benachrichtigung erlaubt (RegNotifyChangeKeyValue)
  KEY_CREATE_LINK = $00000020; // symbolische links erzeugen (HKLM\.. usw)

type

  // ACE = enthält einen einzelnen berechtigungs-eintrag
  PACE = ^TACE;
  TACE = packed record
    ace_type, // typ der ACE
      ace_flags: Byte; // flags
    ace_size: Word; // größe der daten
    mask: DWORD; // die eigentlichen berechtigungen
    sid_start: Byte; // der zur berechtigung gehörige account, fängt hier an
  end;

  // ACL = enthält die einzelnen berechtigungen
  PACL = ^TACL;
  TACL = packed record
    acl_revision, // versions-id, sollte "2" oder "4" sein
      dummy1: Byte; // füllbyte
    size, // größe der ACL
      ace_count, // anzahl der ACE's
      dummy2: word; // füllwort
    // _1stace:TACE
  end;

  // SID = enthält die ID eines Accounts (Gruppe, Vordefiniert, Benutzer usw)
  PSID = ^TSID;
  TSID = packed record
    sid_revision: Byte; // revision, sollte "1" sein
    num_auths: Byte; // anzahl der sub-autoritäten
    id_auth: packed array[0..5] of Byte;
      // übergeordnete aut. (id der domäne,wks,server usw)
    sub_auth: packed array[0..MAX_SUB_AUTHS] of DWORD;
      // eindeutige id des accounts im kontext der übergeordneten aut.
  end;

  // Security-Descriptor, die mutter aller SK-Daten
  PSECURITY_DESCRIPTOR = ^TSECURITY_DESCRIPTOR;
  TSECURITY_DESCRIPTOR = packed record
    sd_revision, // versions-id, nur "1" kann gelesen werden
      dummy1: Byte; // füllbyte
    control: word; // flags
    ofs_owner, // Besitzer
      ofs_group, // Gruppe
      ofs_sacl, // Audit-Liste
      ofs_dacl: DWORD; // Berechtigungen
  end;

  // struktur für zuordnung von flags/masken und deren stringrepräsentation
  TFlagStr = packed record
    Flag: DWORD;
    Str: string;
  end;

{$IFNDEF fpc}
type
  SECURITY_INFORMATION = DWORD;

var ConvertSecurityDescriptorToStringSecurityDescriptor: function(
    SecurityDescriptor: PSECURITY_DESCRIPTOR;
    RequestedStringSDRevision: DWORD;
    SecurityInformation: SECURITY_INFORMATION;
    var StringSecurityDescriptor: PChar;
    StringSecurityDescriptorLen: PDWORD): LongBool; stdcall;

var LookupAccountSid: function(
    lpSystemName: PChar; Sid: PSID; Name: PChar;
    var cchName: DWORD; ReferencedDomainName: PChar;
    var cchReferencedDomainName: DWORD; var peUse: DWORD): LongBool; stdcall;

function LoadLibrary(lpLibFileName: PChar): HMODULE; stdcall;
external 'kernel32.dll' name 'LoadLibraryA';

function FreeLibrary(hLibModule: HMODULE): LongBool; stdcall;
external 'kernel32.dll' name 'FreeLibrary';

function GetProcAddress(hModule: HMODULE; lpProcName: PChar): Pointer; stdcall;
external 'kernel32.dll' name 'GetProcAddress';

function LocalFree(hMem: THandle): THandle; stdcall;
external 'kernel32.dll' name 'LocalFree';

// konvertiere einen security descriptor in einen string
function SD2String(SD: PSECURITY_DESCRIPTOR; var SOut: string): boolean;

{$ENDIF}

implementation

{$IFNDEF fpc}
var
  hModAdvapi: HMODULE = 0;

const
  SDDL_REVISION_1 = 1;

  OWNER_SECURITY_INFORMATION = $00000001;
  GROUP_SECURITY_INFORMATION = $00000002;
  DACL_SECURITY_INFORMATION = $00000004;
  SACL_SECURITY_INFORMATION = $00000008;

// konvertiere einen security descriptor in einen string

function SD2String(SD: PSECURITY_DESCRIPTOR; var SOut: string): boolean;
var
  buf: PChar;
  lenbuf: DWORD;
begin
  buf := nil;
  Result := Assigned(ConvertSecurityDescriptorToStringSecurityDescriptor) and
    ConvertSecurityDescriptorToStringSecurityDescriptor(SD, SDDL_REVISION_1,
    OWNER_SECURITY_INFORMATION or GROUP_SECURITY_INFORMATION or
    DACL_SECURITY_INFORMATION or SACL_SECURITY_INFORMATION, buf, @lenbuf);
  if Result then
    SetString(SOut, buf, lenbuf);
  if Assigned(buf) then
    LocalFree(THandle(buf));
end;

initialization

  ConvertSecurityDescriptorToStringSecurityDescriptor := nil;
  LookupAccountSid := nil;
  hModAdvapi := LoadLibrary('advapi32.dll');
  if hModAdvapi <> 0 then
  begin
    ConvertSecurityDescriptorToStringSecurityDescriptor := GetProcAddress(
      hModAdvapi, 'ConvertSecurityDescriptorToStringSecurityDescriptorA');
    LookupAccountSid := GetProcAddress(hModAdvapi, 'LookupAccountSidA');
  end;

finalization

  ConvertSecurityDescriptorToStringSecurityDescriptor := nil;
  LookupAccountSid := nil;
  if hModAdvapi <> 0 then
    FreeLibrary(hModAdvapi);

{$ENDIF}

end.

