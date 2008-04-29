from os import sep, walk, path, getcwd
from shutil import rmtree
from buildbot.steps.source import SVN
from buildbot.interfaces import BuildSlaveTooOldError
from buildbot.process.buildstep import LoggedRemoteCommand

from twisted.python import log

from guadalinex import tags_dir

class SVNLastTag(SVN):
    def startVC(self, branch, revision, patch):

        # handle old slaves
        warnings = []
        slavever = self.slaveVersion("svn", "old")
        if not slavever:
            m = "slave does not have the 'svn' command"
            raise BuildSlaveTooOldError(m)

        if self.slaveVersionIsOlderThan("svn", "1.39"):
            # the slave doesn't know to avoid re-using the same sourcedir
            # when the branch changes. We have no way of knowing which branch
            # the last build used, so if we're using a non-default branch and
            # either 'update' or 'copy' modes, it is safer to refuse to
            # build, and tell the user they need to upgrade the buildslave.
            if (branch != self.branch
                and self.args['mode'] in ("update", "copy")):
                m = ("This buildslave (%s) does not know about multiple "
                     "branches, and using mode=%s would probably build the "
                     "wrong tree. "
                     "Refusing to build. Please upgrade the buildslave to "
                     "buildbot-0.7.0 or newer." % (self.build.slavename,
                                                   self.args['mode']))
                raise BuildSlaveTooOldError(m)

        if slavever == "old":
            # 0.5.0 compatibility
            if self.args['mode'] in ("clobber", "copy"):
                # TODO: use some shell commands to make up for the
                # deficiency, by blowing away the old directory first (thus
                # forcing a full checkout)
                warnings.append("WARNING: this slave can only do SVN updates"
                                ", not mode=%s\n" % self.args['mode'])
                log.msg("WARNING: this slave only does mode=update")
            if self.args['mode'] == "export":
                raise BuildSlaveTooOldError("old slave does not have "
                                            "mode=export")
            self.args['directory'] = self.args['workdir']
            if revision is not None:
                # 0.5.0 can only do HEAD. We have no way of knowing whether
                # the requested revision is HEAD or not, and for
                # slowly-changing trees this will probably do the right
                # thing, so let it pass with a warning
                m = ("WARNING: old slave can only update to HEAD, not "
                     "revision=%s" % revision)
                log.msg(m)
                warnings.append(m + "\n")
            revision = "HEAD" # interprets this key differently
            if patch:
                raise BuildSlaveTooOldError("old slave can't do patch")

        f = open("bicheando","a"); f.write(branch+"\n"); f.close()
        if self.svnurl:
            assert not branch # we need baseURL= to use branches
            self.args['svnurl'] = self.svnurl
        else:
	    tmp = self.__addLastTagPath(branch)
	    f = open("bicheando","a"); f.write(self.baseURL+tmp+"\n==END==\n\n"); f.close()
            self.args['svnurl'] = self.baseURL + tmp

        self.args['revision'] = revision
        self.args['patch'] = patch

        revstuff = []
        if branch is not None and branch != self.branch:
            revstuff.append("[branch]")
        if revision is not None:
            revstuff.append("r%s" % revision)
        if patch is not None:
            revstuff.append("[patch]")
        self.description.extend(revstuff)
        self.descriptionDone.extend(revstuff)

        cmd = LoggedRemoteCommand("svn", self.args)
        self.startCommand(cmd, warnings)

    #TODO: Implementar logica para que busque en el SVN el ultimo tag real
    # y no el ultimo tag del commit realizado, que es lo que se hace aqui
    def __addLastTagPath(self, branch):
	files = self.build.allFiles()

	# If are files commited
	if files:
	    last_tag_name = ''
	    for file in files:
		f = open("bicheando","a"); f.write(file+"\n"); f.close()
	        tag_name = file.split(sep)[0]
	        if tag_name > last_tag_name:
                    last_tag_name = tag_name

	# If we are forcing the build
	else:
	    last_tag_name = branch

	f = open("bicheando","a"); f.write(tags_dir+sep+last_tag_name+"\n"); f.close()
	return tags_dir + sep + last_tag_name 
