from buildbot.steps.shell import ShellCommand, WithProperties
from buildbot.status.builder import SUCCESS, FAILURE, WARNINGS

from guadalinex import upload_dir, halt_on_lintian_error, halt_on_unittest_error
from guadalinex import path, derivative, derivative_env, lig, lig_env, merge, merge_env

class RemoveSVN(ShellCommand):
    name = "RemoveSVN"
    command = "rm -rf $(find -name .svn)"
    description = "removeSVN"
    descriptionDone = "removeSVN"

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)

class BuildPkg(ShellCommand):
    name = "BuildPkg"
    command = ["debuild", "-uc", "-us"]
#    command = ["svn-buildpackage", "-us", "-uc", "--svn-lintian", "-rfakeroot", "--svn-noninteractive", "--svn-ignore"]
    description = ["buildPkg"]
    descriptionDone = ["buildPkg"]

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)

    def createSummary(self, log):
        warning_log = []
        error_log = []
	self.descriptionDone = self.description

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
    name = "GCSBuild"
    command = ["gcs_build"]
    description = ["gcs_build"]
    descriptionDone = ["gcs_build"]

    def __init__(self, **kwargs):
	BuildPkg.__init__(self, **kwargs)


class SetSVNRev(ShellCommand):
    name = "SetSVNRev"
    command = ["sed", "-i", WithProperties("s/^version\:.*/version\: v5r%s/g", "got_revision"), "gcs/info"]
    description = ["setSVNRevision"]
    descriptionDone = ["setSVNRevision"]

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)

    """
    def start(self):
	self.setCommand(["sed", "-i", 
		"s/^version\:.*/version\: v5r%s/g" % self.getProperty("got_revision"), 
		"gcs/info"])
	ShellCommand.start(self)
    """

class Unittests(ShellCommand):
    name = "Unittests"
    command = ["bash", "unittests"]
    description = ["unittests"]
    descriptionDone = ["unittests"]

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)

    def createSummary(self, log):
        warning_log = []
        error_log = []
	self.descriptionDone = self.description

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
    name = "UploadPkg"
    #TODO: Fix this string ala python style
    command = ["sh", "-c", 
		    "cp ../*.dsc ../*.tar.gz ../*.deb ../*.build ../*.changes %s; \
		    rm -f ../*.dsc ../*.tar.gz ../*.deb ../*.build ../*.changes"
		    % upload_dir]
    description = ["Upload"]
    descriptionDone = ["Upload"]

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

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)


class Lig(ShellCommand):
    name = "Lig"
    command = lig
    description = [name]

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)

    def evaluateCommand(self, cmd):
	self.descriptionDone = [self.name]

	if cmd.rc == 255:
		self.descriptionDone.append("Error controlado")
		return FAILURE

	if cmd.rc != 0:
		return FAILURE

        return SUCCESS
