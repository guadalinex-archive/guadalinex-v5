unit creg_def;


(* definitionen fuer's parsen der windows-9x registry-hives
   (C)2000-2004 Markus Stephany, merkes_at_mirkes.de, BSD-lizensiert

   Microsoft, Windows, Windows NT sind Markenzeichen der Microsoft
   Corporation.
   NT ist eine Marke von Northern Telecom Limited.


   - v0.01 06.august 2000

   credits an den unbekannten verfasser von winreg.txt
*)


//ansistrings
{$H+}

interface

uses
  dumphive_comm;

const
  // Header-ID's der verschiedenen bl�cke
  ID_CREG = $47455243; // ID des HIVEs (der Datei) selbst
  ID_RGKN = $4E4B4752; // ID eines RGKN-Blocks
  ID_RGDB = $42444752; // ID eines RGDB-Blocks

  RGKN_FIRST = 32; // ofset zum ersten RGKN-Block

type

  // CREG-block (am anfang des hives)
  PCREG_HEADER = ^TCREG_HEADER;
  TCREG_HEADER = packed record
    id, // id, muss ID_CREG "creg" sein
      dummy1,
      ofs_root, // ofset zum 1. RGDB-block
      dummy2,
      entries: DWORD; // anzahl der RGDB-bl�cke
  end;

  // RGKN-block (nach CREG an pos 32, enth�lt navigationsstruktur des hives)
  PRGKN_BLOCK = ^TRGKN_BLOCK;
  TRGKN_BLOCK = packed record
    id, // id, muss ID_RGKN "rgkn" sein
      size, // gr��e der struktur
      ofs_root: DWORD; // ofset des 1. TREE-eintrags (relativ zu RGKN!)
  end;

  // TREE-block (enth�lt informationen zu einem registry-schl�ssel,
  //             mehrere nach RGKN-block)
  PTREE_BLOCK = ^TTREE_BLOCK;
  TTREE_BLOCK = packed record
    dummy1,
      hash, // hash des keynamens, siehe winreg.txt
      dummy2: DWORD;
    ofsParent, // zeiger auf �bergeordneten schl�ssel
      ofsSub, // zeiger auf 1. sub-key (oder $FFFFFFFF /
                       // $00000000, falls keiner vorhanden)
    ofsNext: DWORD; // zeiger auf n�chsten schl�ssel im gleichen
                       // level (oder $FFFFFFFF / $00000000, falls letzter)
    id: DWORD; // id des dazugeh�rigen RGDB-eintrages
  end;

  // RGDB-block (enth�lt die eigentlichen schl�ssel/werte-eintr�ge,
  //             dies ist der header)
  PRGDB_HEADER = ^TRGDB_HEADER;
  TRGDB_HEADER = packed record
    id, // id, muss ID_RGDB "rgdb" sein
      size: DWORD; // gr��e der struktur
  end;

  // virt. RGDB-eintrag (enth�lt die tats�chlichen schl�ssel-infos,
  //                    _keine_ hive-struktur, wird vom programm erzeugt)
  PRGDBENTRY = ^TRGDBENTRY;
  TRGDBENTRY = packed record
    size, // gr��e des eintrages
      id, // die id des eintrags, korrespondiert mit
                       // id in TTREE_BLOCK
    dummy1: DWORD;
    len_key, // l�nge des schl�sselnamens
      num_vals: Word; // anzahl der wert-eintr�ge
    dummy2: DWORD;
    key_name: string; // schl�ssel-name
    data: PByteArray; // zeiger auf wert-daten
  end;

  // tats. RGDB-eintrag (enth�lt die tats�chlichen schl�ssel-infos, steht
  //                     so im hive)
  // daran schlie�en sich der schl�sselname und die werte-daten
  // (in form von (TVALUE_ENTRY+Daten)[0..x]) an
  PRGDB_ENTRY = ^TRGDB_ENTRY;
  TRGDB_ENTRY = packed record
    size, // gr��e des eintrages
      id, // mit TREE_BLOCK.id korrespondierende id des eintrages
      dummy1: DWORD;
    len_key, // l�nge key-name
      num_vals: Word; // anzahl werte
    dummy2: DWORD;
  end;

  // VALUE-eintrag, enth�lt angaben zu einem registry-wert
  // daran schlie�en sich der wertname sowie die daten des wertes an
  PVALUE_ENTRY = ^TVALUE_ENTRY;
  TVALUE_ENTRY = packed record
    type_val, // typ des wertes (REG_NONE,REG_SZ...)
      dummy1: DWORD; // unbekannt
    len_val, // gr��e des wert-namens
      len_data: Word; // gr��e der wert-daten
  end;

implementation
end.
