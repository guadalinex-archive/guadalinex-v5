# -*- coding: utf-8 -*-

from buildbot.steps.shell import ShellCommand, WithProperties
from buildbot.steps.transfer import FileUpload
from buildbot.status.builder import SUCCESS, FAILURE, WARNINGS

from guadalinex import upload_dir, halt_on_lintian_error, halt_on_unittest_error
from guadalinex import path, derivative, lig, merge, buildimage

class RemoveSVN(ShellCommand):
    """ Removes the .svn directories recursively"""

    name = "RemoveSVN"
    command = "rm -rf $(find -name .svn)"
    description = [name]

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)

class BuildPkg(ShellCommand):
    """Perfoms the building of a package with debuild
    It counts and logs lintian error and lintian warnings.
    On lintian errors can return FAILURE if guadalinex.halt_on_lintian_error 
    it's True.
    """

    name = "BuildPkg"
    command = ["debuild", "--no-tgz-check", "-uc", "-us"]
    description = [name]

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)

    def createSummary(self, log):
        warning_log = []
        error_log = []
	self.descriptionDone = [self.name]

        for line in log.readlines():
            if line.strip().startswith("W:"):
                warning_log.append(line)
            if line.strip().startswith("E:"):
                error_log.append(line)

        self.warnings = len(warning_log)
        self.errors = len(error_log)

        if self.warnings:
            self.addCompleteLog('Warnings', "".join(warning_log))
            self.descriptionDone.append("warn=%d" % self.warnings)

        if self.errors:
            self.addCompleteLog('Errors', "".join(error_log))
            self.descriptionDone.append("err=%d" % self.errors)

    def evaluateCommand(self, cmd):
	"""
	Evaluates the result of debuild command.

	If lintian errors are founded it can return
	FAILURE or WARNINGS depending on guadalinex.halt_on_lintian_errors
	"""
        if cmd.rc != 0:
            return FAILURE
        if self.errors:
	    if halt_on_lintian_error:
	        return FAILURE
	    else:
		return WARNINGS
        if self.warnings:
            return WARNINGS
        return SUCCESS


class GCSBuild(BuildPkg):
    """It performs the building of a gcs package."""
    name = "GCSBuild"
    command = ["gcs_build"]
    description = [name]

    def __init__(self, **kwargs):
	BuildPkg.__init__(self, **kwargs)


class CheckBuildDeps(ShellCommand):
    """Run pbuilder-satisfydepends to supply the requiered build dependences"""
    name = "CheckBuildDeps"
    command = ["sudo", "/usr/lib/pbuilder/pbuilder-satisfydepends"]
    description = [name]

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)


class SetSVNRev(ShellCommand):
    """
    In gcs packages, it sets the svn-revision as package version/revision.
    """
    name = "SetSVNRev"
    command = ["sed", "-i", WithProperties("s/^version\:.*/version\: v5r%s/g", "got_revision"), "gcs/info"]
    description = [name]

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)


class Unittests(ShellCommand):
    """
    Run the package's unittest suite.
    It run WARNINGS if can't find the 'unittests' executable.
    It parses warnings and errors and can return FAILURE on unittest
    errors if guadalinex.halt_on_unittest_errors it's True.
    """
    name = "Unittests"
    command = ["bash", "unittests"]
    description = [name]

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)

    def createSummary(self, log):
        warning_log = []
        error_log = []
	self.descriptionDone = [self.name]

        for line in log.readlines():
            if line.strip().startswith("WARNING:"):
                warning_log.append(line)
            if line.strip().startswith("FAIL:"):
                error_log.append(line)

        self.warnings = len(warning_log)
        self.errors = len(error_log)

        if self.warnings:
            self.addCompleteLog('Warnings', "".join(warning_log))
            self.descriptionDone.append("warn=%d" % self.warnings)

        if self.errors:
            self.addCompleteLog('Errors', "".join(error_log))
            self.descriptionDone.append("err=%d" % self.errors)

    def evaluateCommand(self, cmd):
	"""
	Evaluate the status of unittests execution

	If there is no unittest executable the code 127 
	it's retured. It throws WARNINGS.
	"""

	if cmd.rc == 127:
		return WARNINGS
        elif cmd.rc != 0:
	    if halt_on_unittest_error:
                return FAILURE
	    else:
		return WARNINGS

        if self.warnings or self.errors:
            return WARNINGS
        return SUCCESS


class UploadPkg(ShellCommand):
    """
    Copies the binary and source package from the slave
    """
    #TODO: Replace this code to use FileUpload using custom
    # build properties has you can see in #87 buildbot ticket.
    name = "UploadPkg"
    #TODO: Fix this string ala python style
    command = ["sh", "-c", 
		    "cp ../*.dsc ../*.tar.gz ../*.deb ../*.build ../*.changes %s; \
		    rm -f ../*.dsc ../*.tar.gz ../*.deb ../*.build ../*.changes"
		    % upload_dir]
    description = [name]

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)

class UpdateDerivative(ShellCommand):
    name = "UpdateDerivative"
    command = derivative
    description = [name]
    error_log = None

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)

    def createSummary(self, log):
	warning_log = []

	self.descriptionDone = [self.name]

        for line in log.readlines():
            if line.strip().startswith("Error:"):
                self.error_log = line
            if line.strip().startswith("Warning:"):
		    warning_log.append(line)

	self.warnings = len(warning_log)

        if self.warnings:
            self.addCompleteLog('Warnings', "".join(warning_log))
            self.descriptionDone.append("warn=%d" % self.warnings)

        if self.error_log:
            self.descriptionDone.append("%s" % self.error_log)

    def evaluateCommand(self, cmd):
	if cmd.rc != 0:
		return FAILURE

	if self.error_log:
            return FAILURE

	if self.warnings:
	    return WARNINGS

        return SUCCESS


class MergeRepo(ShellCommand):
    name = "MergeRepo"
    command = merge
    description = [name]

    #TODO: Controlar los codigos de error de reprepro y reoverride.sh
    # y actuar en consecuencia
    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)


class Lig(ShellCommand):
    name = "Lig"
    command = lig
    description = [name]

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)

    def createSummary(self, log):
        for line in log.readlines():
            if line.strip().startswith("Error:"):
                self.error_log = line

    def evaluateCommand(self, cmd):
	self.descriptionDone = [self.name]

	if cmd.rc == 255:
		self.descriptionDone.append(self.error_log)
		return FAILURE

	if cmd.rc != 0:
		return FAILURE

        return SUCCESS

class BuildImage(ShellCommand):
    name = "BuildImage"
    command = buildimage
    description = [name]

    #TODO: Checkear las salidas de error de for-project
    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)
