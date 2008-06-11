/* ***** BEGIN LICENSE BLOCK *****
 *   Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is distro-mods.
 *
 * The Initial Developer of the Original Code is
 * Canonical Ltd.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 * 
 * ***** END LICENSE BLOCK ***** */

var guadafox = {
  onAddonsLoad: function () {
    this.isffox3 = false;
    var labelGetGuadalinex = document.getElementById("getGuadalinex"); // ffox 2
    var extensions = document.getElementById("extensions-view");
    this.strings = document.getElementById("guadafox-strings");

    if (!labelGetGuadalinex) {
      labelGetGuadalinex = document.getElementById("getGuadalinex3");
      this.isffox3 = true;
    }
    this.initialized = true;

    if (!guadafoxCheckExecutable("/usr/bin/gnome-app-install"))
      labelGetGuadalinex.setAttribute("hidden", "true");
    else if (!this.isffox3) {
      // this is ffox2 only because ffox3 uses a distinct overlay anchor
      if (extensions.getAttribute("selected") != "true") {
        labelGetGuadalinex.setAttribute("hidden", "true");
      }
      if (extensions.getAttribute("selected") == "true") {
        labelGetGuadalinex.setAttribute("hidden", "false");
      }
      extensions.addEventListener("DOMAttrModified", function (e) { guadafox.onAttrModified(e); }, false);
    }
  },
  onMenuItemCommand: function(e) {
    var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                                  .getService(Components.interfaces.nsIPromptService);
    promptService.alert(window, this.strings.getString("helloMessageTitle"),
                                this.strings.getString("helloMessage"));
  },
  onAttrModified: function(e) {
    var labelGetGuadalinex = document.getElementById("getGuadalinex");
    var extensions = document.getElementById("extensions-view");

    if (!guadafoxCheckExecutable("/usr/bin/gnome-app-install")) {
      labelGetGuadalinex.setAttribute("hidden", "true");
      return;
    }
    if (extensions.getAttribute("selected") != "true") {
      labelGetGuadalinex.setAttribute("hidden", "true");
    }
    if (extensions.getAttribute("selected") == "true") {
      labelGetGuadalinex.setAttribute("hidden", "false");
    }
  },
};
window.addEventListener("load", function(e) { guadafox.onAddonsLoad(e); }, false);

function startGuadalinexAddonsWizard(ev)
{
  var executable =
      Components.classes['@mozilla.org/file/local;1']
      .createInstance(Components.interfaces.nsILocalFile);

  executable.initWithPath("/usr/bin/gnome-app-install");

  if(!executable.exists() || !executable.isExecutable())
         alert('Unexpected error!');

  var procUtil =
      Components.classes['@mozilla.org/process/util;1']
      .createInstance(Components.interfaces.nsIProcess);

  var nsFile = executable.QueryInterface(Components.interfaces.nsIFile);

  procUtil.init(executable);

  var args = new Array("--xul-extensions=firefox");
  var res = procUtil.run(false, args, args.length);

  return true;
}

function guadafoxReportBug(event) {

  var executable =
      Components.classes['@mozilla.org/file/local;1']
      .createInstance(Components.interfaces.nsILocalFile);

  executable.initWithPath("/usr/bin/ubuntu-bug");

  if(!executable.exists () || !executable.isExecutable())
         alert('Unexpected error!');

  var procUtil =
      Components.classes['@mozilla.org/process/util;1']
      .createInstance(Components.interfaces.nsIProcess);

  var nsFile = executable.QueryInterface(Components.interfaces.nsIFile);

  procUtil.init(executable);

  var args = new Array("-p", "firefox-3.0" );
  var res = procUtil.run(false, args, args.length);
}


function guadafoxGetHelpOnline(event)
{
  openUILink("https://launchpad.net/distros/ubuntu/hardy/+sources/firefox-3.0/+gethelp", event, false, true);
}

function guadafoxHelpTranslateLaunchpad(event)
{
  openUILink("https://launchpad.net/distros/ubuntu/hardy/+sources/firefox-3.0/+translate", event, false, true);
}

function guadafoxCheckExecutable(filename)
{
  var executable =
      Components.classes['@mozilla.org/file/local;1']
      .createInstance(Components.interfaces.nsILocalFile);

  executable.initWithPath(filename);
  return executable.exists();
}

