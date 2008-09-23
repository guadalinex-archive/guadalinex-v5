#!/usr/bin/perl

$user=$ENV{'LOGNAME'};

print "/home/".$user."/.dnie_lock";

if (-e "/home/".$user."/.dnie_lock") {
  exit(0);
}

$pkcs11_html="/usr/share/opensc-dnie/instal_dnie/instala_modulo.htm";
$dgp_ca_cert="/usr/share/opensc-dnie/ac_raiz_dnie.crt";

# Install OpenSC PKCS#11 and DGP CA certificate on Firefox
# 
# Removed for Guadalinex v5: Work for guadalinex-eadmin::certmanager.py
#if (-e "/usr/bin/firefox") {
#  $firefox_path="/usr/bin/firefox";
#  system($firefox_path, $pkcs11_html);
#  system($firefox_path, $dgp_ca_cert);
#} elsif (-e "/usr/local/bin/firefox") {
#  $firefox_path="/usr/bin/firefox";
#  system($firefox_path, $pkcs11_html);
#  system($firefox_path, $dgp_ca_cert);
#}

# Install OpenSC PKCS#11 and DGP CA certificate on Netscape
#
# Removed for Guadalinex v5: Work for guadalinex-eadmin::certmanager.py
#if (-e "/usr/bin/netscape") {
#  $netscape_path="/usr/bin/netscape";
#  system($netscape_path, $pkcs11_html);
#  system($netscape_path, $dgp_ca_cert);
#} elsif (-e "/usr/local/netscape/netscape") {
#  $netscape_path="/usr/local/netscape/netscape";
#  system($netscape_path, $pkcs11_html);
#  system($netscape_path, $dgp_ca_cert);
#} elsif (-e "/opt/netscape/netscape") {
#  $netscape_path="/opt/netscape/netscape";
#  system($netscape_path, $pkcs11_html);
#  system($netscape_path, $dgp_ca_cert);
#}

open(BUFFER,">/home/".$user."/.dnie_lock");
print BUFFER "DNI electronico se ha instalado correctamente.\n";
close(BUFFER);


exit(0);
