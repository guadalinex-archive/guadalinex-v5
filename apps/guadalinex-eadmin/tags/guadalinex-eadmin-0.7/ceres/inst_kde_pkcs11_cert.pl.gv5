#!/usr/bin/perl

$pkcs11_html="/usr/share/opensc-ceres/instal_ceres/instala_modulo.htm";
$fnmt_ca_cert="/usr/share/opensc-ceres/FNMTClase2CA.crt";

# Install OpenSC PKCS#11 and FNMTClase2CA certificate on Firefox

if (-e "/usr/bin/firefox") {
  $firefox_path="/usr/bin/firefox";
  system($firefox_path, $pkcs11_html);
  system($firefox_path, $fnmt_ca_cert);
} elsif (-e "/usr/local/bin/firefox") {
  $firefox_path="/usr/local/bin/firefox";
  system($firefox_path, $pkcs11_html);
  system($firefox_path, $fnmt_ca_cert);
}


# Install OpenSC PKCS#11 and FNMTClase2CA certificate on Netscape

if (-e "/usr/bin/netscape") {
  $netscape_path="/usr/bin/netscape";
  system($netscape_path, $pkcs11_html);
  system($netscape_path, $fnmt_ca_cert);
} elsif (-e "/usr/local/netscape/netscape") {
  $netscape_path="/usr/local/netscape/netscape";
  system($netscape_path, $pkcs11_html);
  system($netscape_path, $fnmt_ca_cert);
} elsif (-e "/opt/netscape/netscape") {
  $netscape_path="/opt/netscape/netscape";
  system($netscape_path, $pkcs11_html);
  system($netscape_path, $fnmt_ca_cert);
}

exit(0);
