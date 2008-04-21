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
  // Header-ID's der verschiedenen blöcke
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
      entries: DWORD; // anzahl der RGDB-blöcke
  end;

  // RGKN-block (nach CREG an pos 32, enthält navigationsstruktur des hives)
  PRGKN_BLOCK = ^TRGKN_BLOCK;
  TRGKN_BLOCK = packed record
    id, // id, muss ID_RGKN "rgkn" sein
      size, // größe der struktur
      ofs_root: DWORD; // ofset des 1. TREE-eintrags (relativ zu RGKN!)
  end;

  // TREE-block (enthält informationen zu einem registry-schlüssel,
  //             mehrere nach RGKN-block)
  PTREE_BLOCK = ^TTREE_BLOCK;
  TTREE_BLOCK = packed record
    dummy1,
      hash, // hash des keynamens, siehe winreg.txt
      dummy2: DWORD;
    ofsParent, // zeiger auf übergeordneten schlüssel
      ofsSub, // zeiger auf 1. sub-key (oder $FFFFFFFF /
                       // $00000000, falls keiner vorhanden)
    ofsNext: DWORD; // zeiger auf nächsten schlüssel im gleichen
                       // level (oder $FFFFFFFF / $00000000, falls letzter)
    id: DWORD; // id des dazugehörigen RGDB-eintrages
  end;

  // RGDB-block (enthält die eigentlichen schlüssel/werte-einträge,
  //             dies ist der header)
  PRGDB_HEADER = ^TRGDB_HEADER;
  TRGDB_HEADER = packed record
    id, // id, muss ID_RGDB "rgdb" sein
      size: DWORD; // größe der struktur
  end;

  // virt. RGDB-eintrag (enthält die tatsächlichen schlüssel-infos,
  //                    _keine_ hive-struktur, wird vom programm erzeugt)
  PRGDBENTRY = ^TRGDBENTRY;
  TRGDBENTRY = packed record
    size, // größe des eintrages
      id, // die id des eintrags, korrespondiert mit
                       // id in TTREE_BLOCK
    dummy1: DWORD;
    len_key, // länge des schlüsselnamens
      num_vals: Word; // anzahl der wert-einträge
    dummy2: DWORD;
    key_name: string; // schlüssel-name
    data: PByteArray; // zeiger auf wert-daten
  end;

  // tats. RGDB-eintrag (enthält die tatsächlichen schlüssel-infos, steht
  //                     so im hive)
  // daran schließen sich der schlüsselname und die werte-daten
  // (in form von (TVALUE_ENTRY+Daten)[0..x]) an
  PRGDB_ENTRY = ^TRGDB_ENTRY;
  TRGDB_ENTRY = packed record
    size, // größe des eintrages
      id, // mit TREE_BLOCK.id korrespondierende id des eintrages
      dummy1: DWORD;
    len_key, // länge key-name
      num_vals: Word; // anzahl werte
    dummy2: DWORD;
  end;

  // VALUE-eintrag, enthält angaben zu einem registry-wert
  // daran schließen sich der wertname sowie die daten des wertes an
  PVALUE_ENTRY = ^TVALUE_ENTRY;
  TVALUE_ENTRY = packed record
    type_val, // typ des wertes (REG_NONE,REG_SZ...)
      dummy1: DWORD; // unbekannt
    len_val, // größe des wert-namens
      len_data: Word; // größe der wert-daten
  end;

implementation
end.
