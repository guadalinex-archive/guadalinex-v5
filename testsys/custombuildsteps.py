from buildbot.steps.shell import ShellCommand, WithProperties
from buildbot.status.builder import SUCCESS, FAILURE, WARNINGS

from guadalinex import upload_dir, halt_on_lintian_error, halt_on_unittest_error

class BuildPkg(ShellCommand):
    command = ["debuild", "-uc", "-us"]
#    command = ["svn-buildpackage", "-us", "-uc", "--svn-lintian", "-rfakeroot", "--svn-noninteractive", "--svn-ignore"]
    description = ["buildPkg"]
    descriptionDone = ["buildPkg"]

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)

    def createSummary(self, log):
        warning_log = []
        error_log = []

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
    command = ["gcs_build"]
    description = ["gcs_build"]
    descriptionDone = ["gcs_build"]

    def __init__(self, **kwargs):
	BuildPkg.__init__(self, **kwargs)


class SetSVNRev(ShellCommand):
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
    command = ["bash", "unittests"]
    description = ["unittests"]
    descriptionDone = ["unittests"]

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)

    def createSummary(self, log):
        warning_log = []
        error_log = []

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
        if cmd.rc != 0:
	    if halt_on_unittest_error:
                return FAILURE
	    else:
		return WARNINGS
        if self.warnings or self.errors:
            return WARNINGS
        return SUCCESS


class UploadPkg(ShellCommand):
    #TODO: Fix this string ala python style
    command = ["sh", "-c", 
		    "cp ../*.dsc ../*.tar.gz ../*.deb ../*.build ../*.changes %s; \
		    rm -f ../*.dsc ../*.tar.gz ../*.deb ../*.build ../*.changes"
		    % upload_dir]
    description = ["Upload"]
    descriptionDone = ["Upload"]

    def __init__(self, **kwargs):
	ShellCommand.__init__(self, **kwargs)
